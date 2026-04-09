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

void test_FlexArena_footprint(void) {
    BLAST_FlexArena *a = NULL;
    BLAST_FlexArena_create(&a, 64, 4, 8);
    // footprint = n_chunk * FixedArena_footprint + sizeof(n_chunk) + sizeof(max_chunk) + sizeof(previous) + sizeof(next) + sizeof(start)
    // = 2072 + 8 + 8 + 8 + 8 + 8 = 2112
    TEST_ASSERT_EQUAL(2112, BLAST_FlexArena_footprint(a));
    BLAST_FlexArena_destroy(&a);
}

void test_FlexArena_free_keeps_chunk_with_busy_neighbor(void) {
    BLAST_FlexArena *a = NULL;
    // 1 block per chunk, max 3 chunks: forces chunks 0, 1, 2 to be created
    BLAST_FlexArena_create(&a, 64, 1, 3);

    void *b0 = NULL, *b1 = NULL, *b2 = NULL;
    BLAST_FlexArena_alloc(a, &b0);
    BLAST_FlexArena_alloc(a, &b1);
    BLAST_FlexArena_alloc(a, &b2);
    TEST_ASSERT_EQUAL(3, a->n_chunk);

    // Free chunk 1 (middle). Its neighbors (chunk 0 and chunk 2) are still busy,
    // so the chunk must NOT be removed.
    BLAST_FlexArena_free(a, &b1);
    TEST_ASSERT_EQUAL(3, a->n_chunk);

    // Free chunk 2 (tail). Its neighbors are chunk 1 (empty) and NULL — both
    // free/absent — so chunk 2 IS removed immediately.
    BLAST_FlexArena_free(a, &b2);
    TEST_ASSERT_EQUAL(2, a->n_chunk);

    // Free chunk 0 (head). The head is never removed. Chunk 1's situation has
    // changed (prev now empty) but the implementation only evaluates the chunk
    // that was just freed — no cascade — so chunk 1 stays.
    BLAST_FlexArena_free(a, &b0);
    TEST_ASSERT_EQUAL(2, a->n_chunk);

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

void test_FlexArena_availablemem_on_create(void) {
    BLAST_FlexArena *a = NULL;
    BLAST_FlexArena_create(&a, 64, 4, 8);
    // 1 chunk, 4 free blocks: 4 * 64 = 256
    TEST_ASSERT_EQUAL(256, BLAST_FlexArena_availablemem(a));
    BLAST_FlexArena_destroy(&a);
}

void test_FlexArena_availablemem_after_alloc(void) {
    BLAST_FlexArena *a = NULL;
    BLAST_FlexArena_create(&a, 64, 4, 8);
    void *b = NULL;
    BLAST_FlexArena_alloc(a, &b);
    // 3 of 4 blocks free: 3 * 64 = 192
    TEST_ASSERT_EQUAL(192, BLAST_FlexArena_availablemem(a));
    BLAST_FlexArena_destroy(&a);
}

void test_FlexArena_availablemem_expands(void) {
    BLAST_FlexArena *a = NULL;
    // 1 block per chunk, max 2 chunks
    BLAST_FlexArena_create(&a, 64, 1, 2);

    void *b0 = NULL, *b1 = NULL;
    BLAST_FlexArena_alloc(a, &b0); // chunk 0 full
    BLAST_FlexArena_alloc(a, &b1); // chunk 1 created and full
    // 0 free blocks across both chunks
    TEST_ASSERT_EQUAL(0, BLAST_FlexArena_availablemem(a));

    BLAST_FlexArena_free(a, &b0);
    // chunk 0: 1 free block (64), chunk 1: 0 free → 64
    TEST_ASSERT_EQUAL(64, BLAST_FlexArena_availablemem(a));

    BLAST_FlexArena_destroy(&a);
}
