#include "vector.h"
#include "unity.h"

void test_Vector_create(void) {
    BLAST_Vector *v = NULL;
    BLAST_Error e = BLAST_Vector_create(&v, sizeof(int));
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_NOT_NULL(v->data);
    TEST_ASSERT_EQUAL(0, v->size);
    TEST_ASSERT_EQUAL(BLAST_VECTOR_INITIAL_CAPACITY, v->capacity);
    TEST_ASSERT_EQUAL(sizeof(int), v->elem_size);
    BLAST_Vector_destroy(&v);
}

void test_Vector_destroy(void) {
    BLAST_Vector *v = NULL;
    BLAST_Vector_create(&v, sizeof(int));
    BLAST_Error e = BLAST_Vector_destroy(&v);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NULL(v);
}

void test_Vector_push(void) {
    BLAST_Vector *v = NULL;
    BLAST_Vector_create(&v, sizeof(int));
    int val = 42;
    BLAST_Error e = BLAST_Vector_push(v, &val);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_EQUAL(1, v->size);
    BLAST_Vector_destroy(&v);
}

void test_Vector_push_preserves_data(void) {
    BLAST_Vector *v = NULL;
    BLAST_Vector_create(&v, sizeof(int));
    int a = 1, b = 2, c = 3;
    BLAST_Vector_push(v, &a);
    BLAST_Vector_push(v, &b);
    BLAST_Vector_push(v, &c);
    int *out = NULL;
    BLAST_Vector_get(v, 0, (void**)&out); TEST_ASSERT_EQUAL_INT(1, *out);
    BLAST_Vector_get(v, 1, (void**)&out); TEST_ASSERT_EQUAL_INT(2, *out);
    BLAST_Vector_get(v, 2, (void**)&out); TEST_ASSERT_EQUAL_INT(3, *out);
    BLAST_Vector_destroy(&v);
}

void test_Vector_push_grows(void) {
    BLAST_Vector *v = NULL;
    BLAST_Vector_create(&v, sizeof(int));
    // Fill to capacity then push one more to trigger growth
    for (int i = 0; i < (int)BLAST_VECTOR_INITIAL_CAPACITY + 1; i++) {
        BLAST_Vector_push(v, &i);
    }
    TEST_ASSERT_EQUAL(BLAST_VECTOR_INITIAL_CAPACITY * 2, v->capacity);
    TEST_ASSERT_EQUAL(BLAST_VECTOR_INITIAL_CAPACITY + 1, v->size);
    BLAST_Vector_destroy(&v);
}

void test_Vector_push_grows_preserves_data(void) {
    BLAST_Vector *v = NULL;
    BLAST_Vector_create(&v, sizeof(int));
    for (int i = 0; i < (int)BLAST_VECTOR_INITIAL_CAPACITY + 1; i++) {
        BLAST_Vector_push(v, &i);
    }
    // Verify all elements survived the realloc
    for (int i = 0; i < (int)BLAST_VECTOR_INITIAL_CAPACITY + 1; i++) {
        int *out = NULL;
        BLAST_Vector_get(v, i, (void**)&out);
        TEST_ASSERT_EQUAL_INT(i, *out);
    }
    BLAST_Vector_destroy(&v);
}

void test_Vector_pop(void) {
    BLAST_Vector *v = NULL;
    BLAST_Vector_create(&v, sizeof(int));
    int val = 99;
    BLAST_Vector_push(v, &val);
    BLAST_Error e = BLAST_Vector_pop(v);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_EQUAL(0, v->size);
    BLAST_Vector_destroy(&v);
}

void test_Vector_pop_empty(void) {
    BLAST_Vector *v = NULL;
    BLAST_Vector_create(&v, sizeof(int));
    BLAST_Error e = BLAST_Vector_pop(v);
    TEST_ASSERT_EQUAL(BLAST_BAD_BLOCK_CODE, e.code);
    BLAST_Vector_destroy(&v);
}

void test_Vector_get(void) {
    BLAST_Vector *v = NULL;
    BLAST_Vector_create(&v, sizeof(int));
    int val = 7;
    BLAST_Vector_push(v, &val);
    int *out = NULL;
    BLAST_Error e = BLAST_Vector_get(v, 0, (void**)&out);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_EQUAL_INT(7, *out);
    BLAST_Vector_destroy(&v);
}

void test_Vector_get_out_of_bounds(void) {
    BLAST_Vector *v = NULL;
    BLAST_Vector_create(&v, sizeof(int));
    void *out = NULL;
    BLAST_Error e = BLAST_Vector_get(v, 0, &out);
    TEST_ASSERT_EQUAL(BLAST_BAD_BLOCK_CODE, e.code);
    TEST_ASSERT_NULL(out);
    BLAST_Vector_destroy(&v);
}

void test_Vector_size(void) {
    BLAST_Vector *v = NULL;
    BLAST_Vector_create(&v, sizeof(int));
    TEST_ASSERT_EQUAL(0, BLAST_Vector_size(v));
    int val = 1;
    BLAST_Vector_push(v, &val);
    TEST_ASSERT_EQUAL(1, BLAST_Vector_size(v));
    BLAST_Vector_pop(v);
    TEST_ASSERT_EQUAL(0, BLAST_Vector_size(v));
    BLAST_Vector_destroy(&v);
}
