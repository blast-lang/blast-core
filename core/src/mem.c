#include "mem.h"
#include <stdlib.h>

#define BLAST_GLOBAL_ALLOCATOR_SIZE (1024 * 1024) // 1 MB
BLAST_MemAllocator *BLAST_GlobalAllocator = NULL;

BLAST_MemAllocator *BLAST_MemAllocator_init(size_t initial_size) {}

static void BLAST_MemAllocator_destroy(BLAST_MemAllocator *a) {}

__attribute__((constructor))
static void BLAST_GlobalAllocator_init(void) {
    if (!BLAST_GlobalAllocator) {
        BLAST_GlobalAllocator = BLAST_MemAllocator_init(BLAST_GLOBAL_ALLOCATOR_SIZE);
    }
}

__attribute__((destructor))
static void BLAST_GlobalAllocator_destroy(void) {
    if (BLAST_GlobalAllocator) {
        BLAST_MemAllocator_destroy(BLAST_GlobalAllocator);
        BLAST_GlobalAllocator = NULL;
    }
}


BLAST_Error BLAST_FixedArena_create(BLAST_FixedArena **arena, size_t s_block, size_t n_block) {
    *arena = (BLAST_FixedArena*)malloc(sizeof(BLAST_FixedArena));
    if (*arena == NULL) return BLAST_BAD_ALLOC;
    // All blocks are free
    (*arena)->free_map = ~(size_t)0;
    // Block size are multiple of 16 bytes to ease SIMD and cache hit
    (*arena)->s_block = BLAST_ALIGN(s_block, MIN(s_block, 16));
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

size_t BLAST_FixedArena_heapsize(const BLAST_FixedArena *const arena) {
    return (sizeof(arena->memory) * arena->s_block * arena->n_block);
}

size_t BLAST_FixedArena_footprint(const BLAST_FixedArena *const arena) {
    return BLAST_FixedArena_heapsize(arena) +
        sizeof(arena->s_block) +
        sizeof(arena->n_block) +
        sizeof(arena->free_map);
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
    // Destroy current Arena content
    if((*arena)->chunk) {
        BLAST_FixedArena_destroy(&(*arena)->chunk);
        (*arena)->chunk = NULL;
    }
    // Destroy neighbor chunks
    if((*arena)->previous) {
        BLAST_FlexArena_destroy(&(*arena)->previous);
    }
    if((*arena)->next) {
        BLAST_FlexArena_destroy(&(*arena)->next);
    }
    free(*arena);
    *arena = NULL;
    return BLAST_NO_ERROR;
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
        // TODO: coalesce with neighbors if they are also empty ?
        if (BLAST_FixedArena_is_empty(current->chunk)) {
            // TODO: Remove the chunk ?
        }
        return BLAST_NO_ERROR;
    } else {
        return BLAST_FlexArena_free(arena->start, to_free);
    }
}

size_t BLAST_FlexArena_heapsize(const BLAST_FlexArena *const arena) {
    // All chunks are the same size
    return arena->n_chunk * BLAST_FixedArena_footprint(arena->chunk);
}

size_t BLAST_FlexArena_footprint(const BLAST_FlexArena *const arena) {
    return BLAST_FlexArena_heapsize(arena) +
        sizeof(arena->n_chunk) +
        sizeof(arena->max_chunk) +
        sizeof(arena->previous) +
        sizeof(arena->next) +
        sizeof(arena->start);
}