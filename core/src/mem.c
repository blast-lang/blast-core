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
    (*arena)->free_map = ~0u;
    // Block size are mutiple of 16 bytes to ease SIMD and cache hit
    (*arena)->s_block = BLAST_ALIGN(s_block, 16);
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
        if (arena->free_map & (1u << (i % arena->s_block))) {
            // Mark block as now used
            arena->free_map &= ~(1u << (i % arena->s_block));
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
    arena->free_map |= (1u << block);
    (*to_free) = NULL;
    // TODO: Use `memset` to 0-out block data?
    return BLAST_NO_ERROR;
}