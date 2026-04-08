#include "mem.h"
#include "unity.h"

void test_FixedArena_create(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_Error e = BLAST_FixedArena_create(&a, 64, 4);
    TEST_ASSERT_EQUAL_UINT32(0, e.code);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(a->memory);
    TEST_ASSERT_EQUAL(64, a->s_block);
    TEST_ASSERT_EQUAL(4, a->n_block);
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_alloc(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);

    void *block = NULL;
    BLAST_Error e = BLAST_FixedArena_alloc(a, &block);
    TEST_ASSERT_EQUAL_UINT32(0, e.code);
    TEST_ASSERT_NOT_NULL(block);

    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_alloc_null_arena(void) {
    void *block = NULL;
    BLAST_Error e = BLAST_FixedArena_alloc(NULL, &block);
    TEST_ASSERT_NOT_EQUAL(0, e.code);
}

void test_FixedArena_free(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);

    void *block = NULL;
    BLAST_FixedArena_alloc(a, &block);
    TEST_ASSERT_NOT_NULL(block);

    BLAST_Error e = BLAST_FixedArena_free(a, &block);
    TEST_ASSERT_EQUAL_UINT32(0, e.code);
    TEST_ASSERT_NULL(block);

    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_realloc_after_free(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 1);

    void *block = NULL;
    BLAST_FixedArena_alloc(a, &block);
    BLAST_FixedArena_free(a, &block);

    void *block2 = NULL;
    BLAST_Error e = BLAST_FixedArena_alloc(a, &block2);
    TEST_ASSERT_EQUAL_UINT32(0, e.code);
    TEST_ASSERT_NOT_NULL(block2);

    BLAST_FixedArena_destroy(&a);
}
