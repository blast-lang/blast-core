#include "mem.h"
#include "unity.h"

void test_FlexArena_create(void) {
    BLAST_FlexArena *a = NULL;
    BLAST_Error e = BLAST_FlexArena_create(&a, 64, 4, 8);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(a->chunk);
    TEST_ASSERT_EQUAL(1, a->n_chunk);
    TEST_ASSERT_EQUAL(8, a->max_chunk);
    TEST_ASSERT_NULL(a->previous);
    TEST_ASSERT_NULL(a->next);
    TEST_ASSERT_EQUAL_PTR(a, a->start);
    BLAST_FlexArena_destroy(&a);
    TEST_ASSERT_NULL(a);
}

void test_FlexArena_alloc(void) {
    BLAST_FlexArena *a = NULL;
    BLAST_FlexArena_create(&a, 64, 4, 8);

    void *block = NULL;
    BLAST_Error e = BLAST_FlexArena_alloc(a, &block);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NOT_NULL(block);

    BLAST_FlexArena_destroy(&a);
}

void test_FlexArena_alloc_expands(void) {
    BLAST_FlexArena *a = NULL;
    // Arena with 1 block per chunk, max 2 chunks
    BLAST_FlexArena_create(&a, 64, 1, 2);

    void *b0 = NULL, *b1 = NULL;
    BLAST_Error e0 = BLAST_FlexArena_alloc(a, &b0);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e0.code);
    TEST_ASSERT_NOT_NULL(b0);

    // This should trigger expansion into a second chunk
    BLAST_Error e1 = BLAST_FlexArena_alloc(a, &b1);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e1.code);
    TEST_ASSERT_EQUAL(2, a->n_chunk);
    TEST_ASSERT_NOT_NULL(a->next);

    BLAST_FlexArena_destroy(&a);
}

void test_FlexArena_alloc_full(void) {
    BLAST_FlexArena *a = NULL;
    // Arena with 1 block per chunk, max 1 chunk
    BLAST_FlexArena_create(&a, 64, 1, 1);

    void *b0 = NULL;
    BLAST_FlexArena_alloc(a, &b0);

    void *b1 = NULL;
    BLAST_Error e = BLAST_FlexArena_alloc(a, &b1);
    TEST_ASSERT_EQUAL(BLAST_ARENA_FULL_CODE, e.code);
    TEST_ASSERT_NULL(b1);

    BLAST_FlexArena_destroy(&a);
}

void test_FlexArena_free(void) {
    BLAST_FlexArena *a = NULL;
    BLAST_FlexArena_create(&a, 64, 4, 8);

    void *block = NULL;
    BLAST_FlexArena_alloc(a, &block);
    TEST_ASSERT_NOT_NULL(block);

    BLAST_Error e = BLAST_FlexArena_free(a, &block);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NULL(block);

    BLAST_FlexArena_destroy(&a);
}
