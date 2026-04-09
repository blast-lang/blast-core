#ifndef BLAST_VECTOR_H
#define BLAST_VECTOR_H

#include "err.h"
#include <stddef.h>

#define BLAST_VECTOR_INITIAL_CAPACITY 8

// Generic dynamic array. Elements are stored by value, copied in/out by elem_size bytes.
typedef struct BLAST_Vector {
    void   *data;      // Contiguous element buffer
    size_t  size;      // Number of elements currently stored
    size_t  capacity;  // Number of allocated slots
    size_t  elem_size; // Size of each element in bytes
} BLAST_Vector;

// Create a vector for elements of size `elem_size`
BLAST_Error BLAST_Vector_create(BLAST_Vector **v, size_t elem_size);
// Destroy the vector and free its memory
BLAST_Error BLAST_Vector_destroy(BLAST_Vector **v);
// Append a copy of `elem` to the end of the vector; grows if needed
BLAST_Error BLAST_Vector_push(BLAST_Vector *v, const void *elem);
// Remove the last element
BLAST_Error BLAST_Vector_pop(BLAST_Vector *v);
// Set `*out` to a pointer to the element at `index` (points into the vector buffer)
BLAST_Error BLAST_Vector_get(const BLAST_Vector *const v, size_t index, void **out);
// Number of elements in the vector
size_t BLAST_Vector_size(const BLAST_Vector *const v);

#endif /* BLAST_VECTOR_H */
