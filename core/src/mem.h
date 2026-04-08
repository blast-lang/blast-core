#ifndef BLAST_MEM_H
#define BLAST_MEM_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

typedef struct BLAST_MemBlock {
    struct BLAST_MemBlock *next;
    struct BLAST_MemBlock *previous;
    size_t size;
    char* start;
    int free;
} BLAST_MemBlock;

typedef struct BLAST_MemAllocator {
    char* memory;
    size_t size;
    // The first block is always used to pre-allocate
    struct BLAST_MemBlock start;
} BLAST_MemAllocator;


BLAST_MemAllocator *BLAST_MemAllocator_init(size_t initial_size);

extern BLAST_MemAllocator *BLAST_GlobalAllocator;

/* Given a number 'm' (typically a size), align it to n-byte size
  BLAST_ALIGN(17, 8)   // → 24 (8-byte aligned)
  BLAST_ALIGN(17, 16)  // → 32 (16-byte aligned)
  BLAST_ALIGN(17, 64)  // → 64 (64-byte aligned)
*/
#define BLAST_ALIGN(m, n) (m+n-1) & ~(n-1)

// Fixed size Arena with fixed size blocks
typedef struct BLAST_FixedArena {
    char *memory; // Total fixed memory (s_block * n_block)
    size_t s_block; 
    size_t n_block;
    size_t free_map;  // Bitmap: 1 bit per block
} BLAST_FixedArena;

// Create the Arena
BLAST_Error BLAST_FixedArena_create(BLAST_FixedArena **arena, size_t s_block, size_t n_block);
// Detroy the Arena and its memory
BLAST_Error BLAST_FixedArena_destroy(BLAST_FixedArena **arena);
// Get an allocated pointer for the Arena
BLAST_Error BLAST_FixedArena_alloc(BLAST_FixedArena *const arena, void **to_alloc);
// Free a given pointer from the Arena
BLAST_Error BLAST_FixedArena_free(BLAST_FixedArena *const arena, void **to_free);

#endif /* BLAST_MEM_H */
