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

void test_FixedArena_alloc_full(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 1);

    void *block = NULL;
    BLAST_FixedArena_alloc(a, &block);

    void *block2 = NULL;
    BLAST_Error e = BLAST_FixedArena_alloc(a, &block2);
    TEST_ASSERT_EQUAL(BLAST_BAD_BLOCK_CODE, e.code);
    TEST_ASSERT_NULL(block2);

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

void test_FixedArena_set_memory(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 1);

    uint32_t *block = NULL;
    BLAST_FixedArena_alloc(a, (void**)&block);
    *block = 15;
    TEST_ASSERT_EQUAL_UINT32(*block, 15);
    BLAST_FixedArena_free(a, (void**)&block);
    TEST_ASSERT_NULL(block);

    uint32_t *block2 = NULL;
    BLAST_Error e = BLAST_FixedArena_alloc(a, (void**)&block2);
    TEST_ASSERT_EQUAL_UINT32(0, e.code);
    TEST_ASSERT_NOT_NULL(block2);
    // Memory from block is still set from previous usage
    TEST_ASSERT_EQUAL_UINT32(*block2, 15);

    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_footprint(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);
    // footprint: sizeof(char*) * s_block * n_block + sizeof(s_block) + sizeof(n_block) + sizeof(free_map) = 2048 + 24 = 2072
    TEST_ASSERT_EQUAL(2072, BLAST_FixedArena_footprint(a));
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_footprint_with_alignment(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 17, 4);
    // s_block aligned to 16 bytes: BLAST_ALIGN(17, 16) = 32
    // footprint: sizeof(char*) * 32 * 4 + 24 = 1024 + 24 = 1048
    TEST_ASSERT_EQUAL(1048, BLAST_FixedArena_footprint(a));
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_is_full_on_create(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);
    TEST_ASSERT_FALSE(BLAST_FixedArena_is_full(a));
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_is_full_after_all_alloc(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);
    void *b0 = NULL, *b1 = NULL, *b2 = NULL, *b3 = NULL;
    BLAST_FixedArena_alloc(a, &b0);
    BLAST_FixedArena_alloc(a, &b1);
    BLAST_FixedArena_alloc(a, &b2);
    BLAST_FixedArena_alloc(a, &b3);
    TEST_ASSERT_TRUE(BLAST_FixedArena_is_full(a));
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_is_full_after_partial_alloc(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);
    void *b = NULL;
    BLAST_FixedArena_alloc(a, &b);
    TEST_ASSERT_FALSE(BLAST_FixedArena_is_full(a));
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_is_full_after_free(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);
    void *b0 = NULL, *b1 = NULL, *b2 = NULL, *b3 = NULL;
    BLAST_FixedArena_alloc(a, &b0);
    BLAST_FixedArena_alloc(a, &b1);
    BLAST_FixedArena_alloc(a, &b2);
    BLAST_FixedArena_alloc(a, &b3);
    BLAST_FixedArena_free(a, &b0);
    TEST_ASSERT_FALSE(BLAST_FixedArena_is_full(a));
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_is_empty_on_create(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);
    TEST_ASSERT_TRUE(BLAST_FixedArena_is_empty(a));
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_is_empty_after_alloc(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);
    void *block = NULL;
    BLAST_FixedArena_alloc(a, &block);
    TEST_ASSERT_FALSE(BLAST_FixedArena_is_empty(a));
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_is_empty_after_free(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);
    void *block = NULL;
    BLAST_FixedArena_alloc(a, &block);
    BLAST_FixedArena_free(a, &block);
    TEST_ASSERT_TRUE(BLAST_FixedArena_is_empty(a));
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_availablemem_on_create(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);
    // All 4 blocks free: 4 * 64 = 256
    TEST_ASSERT_EQUAL(256, BLAST_FixedArena_availablemem(a));
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_availablemem_after_alloc(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);
    void *b = NULL;
    BLAST_FixedArena_alloc(a, &b);
    // 3 of 4 blocks free: 3 * 64 = 192
    TEST_ASSERT_EQUAL(192, BLAST_FixedArena_availablemem(a));
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_availablemem_when_full(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);
    void *b0 = NULL, *b1 = NULL, *b2 = NULL, *b3 = NULL;
    BLAST_FixedArena_alloc(a, &b0);
    BLAST_FixedArena_alloc(a, &b1);
    BLAST_FixedArena_alloc(a, &b2);
    BLAST_FixedArena_alloc(a, &b3);
    TEST_ASSERT_EQUAL(0, BLAST_FixedArena_availablemem(a));
    BLAST_FixedArena_destroy(&a);
}

void test_FixedArena_availablemem_after_free(void) {
    BLAST_FixedArena *a = NULL;
    BLAST_FixedArena_create(&a, 64, 4);
    void *b0 = NULL, *b1 = NULL, *b2 = NULL, *b3 = NULL;
    BLAST_FixedArena_alloc(a, &b0);
    BLAST_FixedArena_alloc(a, &b1);
    BLAST_FixedArena_alloc(a, &b2);
    BLAST_FixedArena_alloc(a, &b3);
    BLAST_FixedArena_free(a, &b2);
    // 1 of 4 blocks free: 1 * 64 = 64
    TEST_ASSERT_EQUAL(64, BLAST_FixedArena_availablemem(a));
    BLAST_FixedArena_destroy(&a);
}