#ifndef BLAST_ERR_H
#define BLAST_ERR_H

#include <stdint.h>

#define MIN(x,y) ((x) <= (y) ? (x) : (y))

typedef struct BLAST_Error {
    size_t code;
    const char* msg;
} BLAST_Error;

#define BLAST_MAKE_ERROR(err_code, err_msg) \
    (BLAST_Error){ \
        .code = err_code, \
        .msg = err_msg \
    }

#define BLAST_NO_ERROR_CODE 0
#define BLAST_NO_ERROR BLAST_MAKE_ERROR(BLAST_NO_ERROR_CODE, "")
#define BLAST_ERROR_IS_ERROR(error) error.code =! BLAST_NO_ERROR_CODE

// Memory error
#define BLAST_BAD_ALLOC_CODE BLAST_NO_ERROR_CODE+1
#define BLAST_BAD_ALLOC BLAST_MAKE_ERROR(BLAST_BAD_ALLOC_CODE, "Call to `malloc` failed")

#define BLAST_BAD_ALLOCATOR_CODE BLAST_BAD_ALLOC_CODE+1
#define BLAST_BAD_ALLOCATOR BLAST_MAKE_ERROR(BLAST_BAD_ALLOCATOR_CODE, "Cannot allocate in NULL allocator")

#define BLAST_BAD_BLOCK_CODE BLAST_BAD_ALLOCATOR_CODE+1
#define BLAST_BAD_BLOCK BLAST_MAKE_ERROR(BLAST_BAD_BLOCK_CODE, "Cannot find memory block")

#define BLAST_ARENA_FULL_CODE BLAST_BAD_BLOCK_CODE+1
#define BLAST_ARENA_FULL BLAST_MAKE_ERROR(BLAST_ARENA_FULL_CODE, "Arena is full")

#endif /* BLAST_ERR_H */