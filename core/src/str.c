#include "str.h"
#include "mem.h"
#include <string.h>

// Write the null terminator at the current end of the buffer.
// Safe to call after any BLAST_Vector_push: post-push, size < capacity always holds.
static void terminate(BLAST_String *s) {
    ((char*)s->buf->data)[s->buf->size] = '\0';
}

BLAST_Error BLAST_String_create(BLAST_String **s) {
    *s = (BLAST_String*)BLAST_alloc(sizeof(BLAST_String));
    if (!*s) return BLAST_BAD_ALLOC;
    BLAST_Error e = BLAST_Vector_create(&(*s)->buf, sizeof(char));
    if (e.code != BLAST_NO_ERROR_CODE) {
        BLAST_free(*s);
        *s = NULL;
        return e;
    }
    terminate(*s);
    return BLAST_NO_ERROR;
}

BLAST_Error BLAST_String_create_from(BLAST_String **s, const char *src, size_t len) {
    BLAST_Error e = BLAST_String_create(s);
    if (e.code != BLAST_NO_ERROR_CODE) return e;
    return BLAST_String_append(*s, src, len);
}

BLAST_Error BLAST_String_destroy(BLAST_String **s) {
    if (!s || !*s) return BLAST_BAD_ALLOCATOR;
    BLAST_Vector_destroy(&(*s)->buf);
    BLAST_free(*s);
    *s = NULL;
    return BLAST_NO_ERROR;
}

BLAST_Error BLAST_String_push(BLAST_String *s, char c) {
    if (!s) return BLAST_BAD_ALLOCATOR;
    BLAST_Error e = BLAST_Vector_push(s->buf, &c);
    if (e.code != BLAST_NO_ERROR_CODE) return e;
    terminate(s);
    return BLAST_NO_ERROR;
}

BLAST_Error BLAST_String_append(BLAST_String *s, const char *src, size_t len) {
    if (!s) return BLAST_BAD_ALLOCATOR;
    for (size_t i = 0; i < len; i++) {
        BLAST_Error e = BLAST_Vector_push(s->buf, &src[i]);
        if (e.code != BLAST_NO_ERROR_CODE) return e;
    }
    terminate(s);
    return BLAST_NO_ERROR;
}

const char *BLAST_String_data(const BLAST_String *s) {
    return s ? (const char*)s->buf->data : NULL;
}

size_t BLAST_String_len(const BLAST_String *s) {
    return s ? BLAST_Vector_size(s->buf) : 0;
}
