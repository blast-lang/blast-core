#include "vector.h"
#include "mem.h"
#include <string.h>

BLAST_Error BLAST_Vector_create(BLAST_Vector **v, size_t elem_size) {
    *v = (BLAST_Vector*)BLAST_alloc(sizeof(BLAST_Vector));
    if (*v == NULL) return BLAST_BAD_ALLOC;
    (*v)->elem_size = elem_size;
    (*v)->size      = 0;
    (*v)->capacity  = BLAST_VECTOR_INITIAL_CAPACITY;
    (*v)->data      = BLAST_alloc(elem_size * BLAST_VECTOR_INITIAL_CAPACITY);
    if ((*v)->data == NULL) {
        BLAST_free(*v);
        *v = NULL;
        return BLAST_BAD_ALLOC;
    }
    return BLAST_NO_ERROR;
}

BLAST_Error BLAST_Vector_destroy(BLAST_Vector **v) {
    if (v == NULL || *v == NULL) return BLAST_BAD_ALLOCATOR;
    BLAST_free((*v)->data);
    BLAST_free(*v);
    *v = NULL;
    return BLAST_NO_ERROR;
}

BLAST_Error BLAST_Vector_push(BLAST_Vector *v, const void *elem) {
    if (!v) return BLAST_BAD_ALLOCATOR;
    if (v->size == v->capacity) {
        // Growth factor of 2
        size_t new_capacity = v->capacity * 2;
        void *new_data = BLAST_alloc(new_capacity * v->elem_size);
        if (!new_data) return BLAST_BAD_ALLOC;
        memcpy(new_data, v->data, v->size * v->elem_size);
        BLAST_free(v->data);
        v->data     = new_data;
        v->capacity = new_capacity;
    }
    // Casting for pointer arythmetic
    memcpy((char*)v->data + v->size * v->elem_size, elem, v->elem_size);
    v->size++;
    return BLAST_NO_ERROR;
}

BLAST_Error BLAST_Vector_pop(BLAST_Vector *v) {
    if (!v) return BLAST_BAD_ALLOCATOR;
    if (v->size == 0) return BLAST_BAD_BLOCK;
    v->size--;
    return BLAST_NO_ERROR;
}

BLAST_Error BLAST_Vector_get(const BLAST_Vector *v, size_t index, void **out) {
    if (!v) return BLAST_BAD_ALLOCATOR;
    if (index >= v->size) return BLAST_BAD_BLOCK;
    *out = (char*)v->data + index * v->elem_size;
    return BLAST_NO_ERROR;
}

size_t BLAST_Vector_size(const BLAST_Vector *v) {
    return v ? v->size : 0;
}
