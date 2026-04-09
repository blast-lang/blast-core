#include "mem.h"
#include <stdlib.h>

//#define BLAST_GLOBAL_ALLOCATOR_SIZE (1024 * 1024) // 1 MB
#define BLAST_GLOBAL_ALLOCATOR_SIZE SIZE_MAX
BLAST_MultiArena *BLAST_GlobalAllocator = NULL;

__attribute__((constructor))
static void BLAST_GlobalAllocator_init(void) {
    if (!BLAST_GlobalAllocator) {
        BLAST_MultiArena_create_limit(&BLAST_GlobalAllocator, BLAST_GLOBAL_ALLOCATOR_SIZE);
    }
}

__attribute__((destructor))
static void BLAST_GlobalAllocator_destroy(void) {
    if (BLAST_GlobalAllocator) {
        BLAST_MultiArena_destroy(&BLAST_GlobalAllocator);
    }
}

void* BLAST_alloc(size_t size) {
    void *ptr = NULL;
    if (BLAST_GlobalAllocator) {
        BLAST_MultiArena_alloc(BLAST_GlobalAllocator, &ptr, size);
    }
    return ptr;
}

BLAST_Error BLAST_free(void* ptr) {
    return BLAST_MultiArena_free(BLAST_GlobalAllocator, &ptr);
}


BLAST_Error BLAST_FixedArena_create(BLAST_FixedArena **arena, size_t s_block, size_t n_block) {
    *arena = (BLAST_FixedArena*)malloc(sizeof(BLAST_FixedArena));
    if (*arena == NULL) return BLAST_BAD_ALLOC;
    // All blocks are free
    (*arena)->free_map = ~(size_t)0;
    // Block size are multiple of 16 bytes to ease SIMD and cache hit
    (*arena)->s_block = BLAST_ALIGN(s_block, MIN(s_block, 16));
    // TODO: error if n_block > sizeof(free_map)
    (*arena)->n_block = n_block;
    // Setup memory buffer
    (*arena)->memory = (char*)malloc(sizeof(char)*(*arena)->s_block*(*arena)->n_block);
    if ((*arena)->memory == NULL) return BLAST_BAD_ALLOC;
    return BLAST_NO_ERROR;
}

BLAST_Error BLAST_FixedArena_destroy(BLAST_FixedArena **arena) {
    if ((arena == NULL) || (*arena == NULL)) return BLAST_BAD_ALLOCATOR;
    if((*arena)->memory) {
        free((*arena)->memory);
        (*arena)->memory = NULL;
    }
    free(*arena);
    *arena = NULL;
    return BLAST_NO_ERROR;
}

BLAST_Error BLAST_FixedArena_alloc(BLAST_FixedArena *const arena, void **to_alloc) {
    if(!arena) {
        return BLAST_BAD_ALLOCATOR;
    }
    // We need to find the first available block in the arena
    for (size_t i = 0; i < arena->n_block; i++) {
        if (arena->free_map & ((size_t)1 << (i % arena->s_block))) {
            // Mark block as now used
            arena->free_map &= ~((size_t)1 << (i % arena->s_block));
            // Return a pointer to this block
            (*to_alloc) = arena->memory + (i * arena->s_block);
            return BLAST_NO_ERROR;
        }
    }
    return BLAST_BAD_BLOCK;
}

BLAST_Error BLAST_FixedArena_free(BLAST_FixedArena *const arena, void **to_free) {
    if(!arena) {
        return BLAST_BAD_ALLOCATOR;
    }
    // Deduce block number from ptr argument and arena start pointer
    size_t offset = (char*)(*to_free) - arena->memory;
    size_t block = offset / arena->s_block;

    // Bounds check
    if ((block < 0) || (block >= arena->n_block)) {
        return BLAST_BAD_BLOCK;
    }
    // Don't free anything really, but mark block as free
    arena->free_map |= ((size_t)1 << block);
    (*to_free) = NULL;
    // TODO: Use `memset` to 0-out block data?
    return BLAST_NO_ERROR;
}

int BLAST_FixedArena_is_empty(const BLAST_FixedArena *const arena) {
    return ~(arena->free_map) == 0;
}

int BLAST_FixedArena_is_full(const BLAST_FixedArena *const arena) {
    // Check if the firsts `n_block` bits are 0
    size_t mask = ((size_t)1 << arena->n_block) - 1;                                                         
    return (arena->free_map & mask) == 0;
}

size_t BLAST_FixedArena_footprint(const BLAST_FixedArena *const arena) {
    size_t block_size = sizeof(arena->memory) * arena->s_block * arena->n_block;
    return block_size +
        sizeof(arena->s_block) +
        sizeof(arena->n_block) +
        sizeof(arena->free_map);
}

// TODO: Cache this information in BLAST_FixedArena ?
size_t BLAST_FixedArena_availablemem(const BLAST_FixedArena *const arena) {
    // Compute the total number of free blocks in the arena!
    size_t mask = (arena->n_block < 8*sizeof(size_t)) ? (((size_t)1 << arena->n_block) - 1) : ~(size_t)0;
    size_t bits = arena->free_map & mask;
    size_t free_blocks = 0;
    while (bits) { bits &= bits - 1; free_blocks++; }
    return free_blocks * arena->s_block;
}

BLAST_Error BLAST_FlexArena_create(BLAST_FlexArena **arena, size_t s_block, size_t n_block, size_t max_chunk) {
    *arena = (BLAST_FlexArena*)malloc(sizeof(BLAST_FlexArena));
    if (*arena == NULL) return BLAST_BAD_ALLOC;
    // Init
    (*arena)->n_chunk = 1;
    (*arena)->max_chunk = max_chunk;
    (*arena)->chunk = NULL;
    (*arena)->previous = NULL;
    (*arena)->next = NULL;
    // Set start pointer
    (*arena)->start = *arena;
    // Pre-allocate the first chunk
    return BLAST_FixedArena_create(&(*arena)->chunk, s_block, n_block);
}

BLAST_Error BLAST_FlexArena_destroy(BLAST_FlexArena **arena) {
    if ((arena == NULL) || (*arena == NULL)) return BLAST_BAD_ALLOCATOR;
    
        // We only work from the start of the list to the end
    if ((*arena) == (*arena)->start) {
        BLAST_FlexArena *current = *arena;
        *arena = NULL;
        BLAST_FlexArena *next = NULL;
        while(current != NULL) {
            // Detroy chunk content
            BLAST_FixedArena_destroy(&current->chunk);
            next = current->next;
            free(current);
            current = next;
        }
        return BLAST_NO_ERROR;
    } else {
        return BLAST_FlexArena_destroy(&(*arena)->start);
    }
}

BLAST_Error BLAST_FlexArena_alloc(BLAST_FlexArena *const arena, void **to_alloc) {
    if(!arena) {
        return BLAST_BAD_ALLOCATOR;
    }
    // We only work from the start of the list to the end
    if (arena == arena->start) {
        // Find the first chunk that can accommodate the memory needed
        BLAST_FlexArena *current = arena;
        BLAST_FlexArena *last = arena;
        int chunk_found = 0;
        while(current != NULL && !chunk_found) {
            if (BLAST_FixedArena_is_full(current->chunk)) {
                last = current;
                current = current->next;
            } else {
                BLAST_FixedArena_alloc(current->chunk, to_alloc);
                chunk_found = 1;
            }
        }
        if (chunk_found) return BLAST_NO_ERROR;
        // No available chunk found, create a new one
        if (arena->n_chunk == arena->max_chunk) {
            return BLAST_ARENA_FULL;
        }
        BLAST_FlexArena_create(
            &(last->next), arena->chunk->s_block, arena->chunk->n_block, arena->max_chunk
        );
        // Wire up the doubly-linked list and set start pointer
        last->next->previous = last;
        last->next->start = arena;
        arena->n_chunk += 1;
        BLAST_FixedArena_alloc(last->next->chunk, to_alloc);
        return BLAST_NO_ERROR;
    } else {
        return BLAST_FlexArena_alloc(arena->start, to_alloc);
    }
}

BLAST_Error BLAST_FlexArena_free(BLAST_FlexArena *const arena, void **to_free) {
    if(!arena) {
        return BLAST_BAD_ALLOCATOR;
    }
    // We only work from the start of the list to the end
    if (arena == arena->start) {
        // We need to find in which chunk `to_free` belongs
        BLAST_FlexArena *current = arena;
        int chunk_found = 0;
        while(current != NULL && !chunk_found) {
            if (BLAST_FixedArena_free(current->chunk, to_free).code == BLAST_BAD_BLOCK_CODE) {
                // Current chunk full, we move forward
                current = current->next;
            } else {
                chunk_found = 1;
            }
        }
        // We found the chunk ? Check if its now empty
        if (!chunk_found) return BLAST_BAD_BLOCK;
        int prev_free = (current->previous == NULL) || BLAST_FixedArena_is_empty(current->previous->chunk);
        int next_free = (current->next == NULL)     || BLAST_FixedArena_is_empty(current->next->chunk);
        // Remove the chunk only when it and both its neighbors are empty/absent
        if (current != arena->start && BLAST_FixedArena_is_empty(current->chunk) && prev_free && next_free) {
            if (current->previous) current->previous->next = current->next;
            if (current->next)     current->next->previous = current->previous;
            BLAST_FixedArena_destroy(&current->chunk);
            free(current);
            arena->start->n_chunk -= 1;
        }
        return BLAST_NO_ERROR;
    } else {
        return BLAST_FlexArena_free(arena->start, to_free);
    }
}

size_t BLAST_FlexArena_footprint(const BLAST_FlexArena *const arena) {
    size_t chunks_size = arena->n_chunk * BLAST_FixedArena_footprint(arena->chunk);
    return chunks_size +
        sizeof(arena->n_chunk) +
        sizeof(arena->max_chunk) +
        sizeof(arena->previous) +
        sizeof(arena->next) +
        sizeof(arena->start);
}

size_t BLAST_FlexArena_availablemem(const BLAST_FlexArena *const arena) {
    if(arena == NULL || arena->chunk == NULL) return 0;
    // We only work from the start of the list to the end
    if (arena == arena->start) {
        size_t total = 0;
        BLAST_FlexArena *current = arena;
        while(current != NULL) {
            total += BLAST_FixedArena_availablemem(current->chunk);
            current = current->next;
        }
        return total;
    } else {
        return BLAST_FlexArena_availablemem(arena->start);
    }
}


BLAST_Error BLAST_MultiArena_create_limit(BLAST_MultiArena **arena, size_t max_mem) {
    *arena = (BLAST_MultiArena*)malloc(sizeof(BLAST_MultiArena));
    if (*arena == NULL) return BLAST_BAD_ALLOC;
    (*arena)->max_mem = max_mem;
    (*arena)->budget = max_mem;
    // All slots start NULL — created lazily on first allocation
    for (size_t i = 0; i < BLAST_MULTIARENA_SLOTS; i++) {
        (*arena)->slots[i] = NULL;
    }
    return BLAST_NO_ERROR;
}

BLAST_Error BLAST_MultiArena_create(BLAST_MultiArena **arena) {
    return BLAST_MultiArena_create_limit(arena, SIZE_MAX);
}

BLAST_Error BLAST_MultiArena_destroy(BLAST_MultiArena **arena) {
    if ((arena == NULL) || (*arena == NULL)) return BLAST_BAD_ALLOCATOR;
    for (size_t i = 0; i < BLAST_MULTIARENA_SLOTS; i++) {
        if ((*arena)->slots[i] != NULL) {
            BLAST_FlexArena_destroy(&(*arena)->slots[i]);
            (*arena)->slots[i] = NULL;
        }
    }
    free(*arena);
    *arena = NULL;
    return BLAST_NO_ERROR;
}

BLAST_Error BLAST_MultiArena_alloc(BLAST_MultiArena *const arena, void **to_alloc, size_t alloc_size) {
    if (!arena) return BLAST_BAD_ALLOCATOR;
    // Find the smallest slot that fits alloc_size (slot i holds blocks of 2^i bytes)
    int slotsize = -1;
    for (size_t i = 0; i < BLAST_MULTIARENA_SLOTS; i++) {
        if (((size_t)1 << i) >= alloc_size) {
            slotsize = (int)i;
            break;
        }
    }
    // If no slot could be found, this is a catastrophic error
    if (slotsize == -1) return BLAST_BAD_ALLOC;

    // Compute the footprint of one FlexArena chunk analytically,
    // matching BLAST_FixedArena_footprint + BLAST_FlexArena_footprint overhead
    size_t s = (size_t)1 << slotsize;
    size_t n = 1 + s / 1024;
    size_t fixed_footprint = sizeof(char*) * s * n + 3 * sizeof(size_t);
    size_t chunk_footprint = fixed_footprint + 5 * sizeof(size_t);

    if (arena->slots[slotsize] == NULL) {
        // Slot doesn't exist yet — check budget before creating it
        if (arena->budget < chunk_footprint) {
            return BLAST_ARENA_FULL;
        }
        BLAST_FlexArena *slot = NULL;
        // We don't want `s_block`*`n_block` <= 1024 to avoid too many allocs
        BLAST_FlexArena_create(&slot, s, n, SIZE_MAX);
        arena->budget -= chunk_footprint;
        arena->slots[slotsize] = slot;
        // Slot is new, so memory space is guaranteed
        return BLAST_FlexArena_alloc(arena->slots[slotsize], to_alloc);
    } else {
        // Slot exists — use it if a free block is already available
        if (BLAST_FlexArena_availablemem(arena->slots[slotsize]) >= alloc_size) {
            return BLAST_FlexArena_alloc(arena->slots[slotsize], to_alloc);
        }
        // Slot is full — a new chunk is needed, check budget
        if (arena->budget < chunk_footprint) {
            return BLAST_ARENA_FULL;
        }
        arena->budget -= chunk_footprint;
        return BLAST_FlexArena_alloc(arena->slots[slotsize], to_alloc);
    }
}

BLAST_Error BLAST_MultiArena_free(BLAST_MultiArena *const arena, void **to_free) {
    if(!arena) {
        return BLAST_BAD_ALLOCATOR;
    }
    // Try to free from every slots, only the slot managing the pointer will actually free space
    for (size_t i = 0; i < BLAST_MULTIARENA_SLOTS; i++) {
        if (arena->slots[i] != NULL) {
            BLAST_FlexArena_free(arena->slots[i], to_free);
        }
    }
    return BLAST_NO_ERROR;
}

size_t BLAST_MultiArena_footprint(const BLAST_MultiArena *const arena) {
    size_t footprint = 0;
    for (size_t i = 0; i < BLAST_MULTIARENA_SLOTS; i++) {
        if (arena->slots[i] != NULL) {
            footprint += BLAST_FlexArena_footprint(arena->slots[i]);
        }
    }
    return footprint +
        sizeof(arena->max_mem) + sizeof(arena->budget) +
        BLAST_MULTIARENA_SLOTS * sizeof(struct BLAST_FlexArena*);
}

size_t BLAST_MultiArena_availablemem(const BLAST_MultiArena *const arena) {
    size_t total = 0;
    for (size_t i = 0; i < BLAST_MULTIARENA_SLOTS; i++) {
        if (arena->slots[i] != NULL) {
            total += BLAST_FlexArena_availablemem(arena->slots[i]);
        }
    }
    return total;
}