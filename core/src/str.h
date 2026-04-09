#ifndef BLAST_STRING_H
#define BLAST_STRING_H

#include "err.h"
#include "vector.h"
#include <stddef.h>

// Dynamic string built on top of BLAST_Vector.
// Invariant: data[len] == '\0' at all times.
// buf->size  = string length (excludes null terminator)
// buf->data  = raw char buffer (may realloc)
typedef struct BLAST_String {
    BLAST_Vector *buf;
} BLAST_String;

// Create an empty string
BLAST_Error BLAST_String_create(BLAST_String **s);
// Create a string from an existing char buffer of length `len`
BLAST_Error BLAST_String_create_from(BLAST_String **s, const char *src, size_t len);
// Destroy the string and free its memory
BLAST_Error BLAST_String_destroy(BLAST_String **s);
// Append a single character
BLAST_Error BLAST_String_push(BLAST_String *s, char c);
// Append `len` bytes from `src`
BLAST_Error BLAST_String_append(BLAST_String *s, const char *src, size_t len);
// Pointer into the internal buffer
const char* BLAST_String_data(const BLAST_String *const s);
// String length, excluding null terminator, O(1)
size_t BLAST_String_len(const BLAST_String *const s);
// Copy 'src' into 'dest'
BLAST_Error BLAST_String_concat(BLAST_String *dest, const BLAST_String *const src);

// Trick to get string literals size before they decay into a pointer
// The "" s concatenation ensures s is a string literal
#define BLAST_STRING_CAPTURE(s) ("" s), (sizeof(s)-1)
#define BLAST_STRING(ptr, s)    BLAST_String_create_from(ptr, BLAST_STRING_CAPTURE(s))

#endif /* BLAST_STRING_H */
