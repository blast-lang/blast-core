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
// At most, this object allocates `s_block` * `n_block` bytes of memory
typedef struct BLAST_FixedArena {
    char *memory; // Total fixed memory (s_block * n_block)
    size_t s_block; // Size (in bytes) of a single block
    size_t n_block; // Number of blocks in the arena
    size_t free_map;  // Bitmap: 1 bit per block
} BLAST_FixedArena;

// Create the Arena
// At most, this function allocates `s_block` * `n_block` bytes of memory
BLAST_Error BLAST_FixedArena_create(BLAST_FixedArena **arena, size_t s_block, size_t n_block);
// Destroy the Arena and its memory
BLAST_Error BLAST_FixedArena_destroy(BLAST_FixedArena **arena);
// Get an allocated pointer for the Arena
BLAST_Error BLAST_FixedArena_alloc(BLAST_FixedArena *const arena, void **to_alloc);
// Free a given pointer from the Arena
BLAST_Error BLAST_FixedArena_free(BLAST_FixedArena *const arena, void **to_free);
// Tells if the blocks are all empty
int BLAST_FixedArena_is_empty(const BLAST_FixedArena *const arena);
// Tells if the blocks are all used
int BLAST_FixedArena_is_full(const BLAST_FixedArena *const arena);
// Get the heap memory used by the block data, in bytes
size_t BLAST_FixedArena_heapsize(const BLAST_FixedArena *const arena);
// Get the total memory footprint of the arena (heap data + struct metadata), in bytes
size_t BLAST_FixedArena_footprint(const BLAST_FixedArena *const arena);


// Resizable arena
// Resizes when a given arena if full by creating a similar arena
// Resizes until maximum allowed memory is reached 
typedef struct BLAST_FlexArena {
    size_t n_chunk; // Number of chunks already in the arena
    size_t max_chunk; // Maximum number of chunks allowed in the arena
    struct BLAST_FixedArena *chunk; // Actual chunk of data
    // Recursive structure to allow for multiple chunks created dynamically
    // Double linked-list to avoid defragmentation
    struct BLAST_FlexArena *previous;
    struct BLAST_FlexArena *next;
    // Also point to start to ease with management and metrics
    struct BLAST_FlexArena *start;
} BLAST_FlexArena;

// Create the Arena
// This function pre-allocates a chunk of `s_block` * `n_block` bytes of memory
// The Arena max size is thus `max_chunk` * `s_block` * `n_block` bytes of memory
BLAST_Error BLAST_FlexArena_create(BLAST_FlexArena **arena, size_t s_block, size_t n_block, size_t max_chunk);
// Destroy the Arena and its memory
BLAST_Error BLAST_FlexArena_destroy(BLAST_FlexArena **arena);
// Get an allocated pointer for the Arena
BLAST_Error BLAST_FlexArena_alloc(BLAST_FlexArena *const arena, void **to_alloc);
// Free a given pointer from the Arena
BLAST_Error BLAST_FlexArena_free(BLAST_FlexArena *const arena, void **to_free);
// Get the actual heap memory used by the chunks, in bytes
size_t BLAST_FlexArena_heapsize(const BLAST_FlexArena *const arena);
// Get the total footprint of the arena
size_t BLAST_FlexArena_footprint(const BLAST_FlexArena *const arena);

// Multipool of arenas, dynamicaly create new ones depending on allocation needs
typedef struct BLAST_MultiArena {
} BLAST_MultiArena;

#endif /* BLAST_MEM_H */
