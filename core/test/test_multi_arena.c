#include "mem.h"
#include "unity.h"

void test_MultiArena_create(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_Error e = BLAST_MultiArena_create(&a);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NOT_NULL(a);
    for (size_t i = 0; i < BLAST_MULTIARENA_SLOTS; i++) {
        TEST_ASSERT_NULL(a->slots[i]);
    }
    BLAST_MultiArena_destroy(&a);
    TEST_ASSERT_NULL(a);
}

void test_MultiArena_create_limit(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_Error e = BLAST_MultiArena_create_limit(&a, 1024);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_EQUAL(1024, a->max_mem);
    BLAST_MultiArena_destroy(&a);
    TEST_ASSERT_NULL(a);
}

void test_MultiArena_alloc_small(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);

    // Allocate 1 byte — should land in slot 0 (2^0 = 1)
    void *block = NULL;
    BLAST_Error e = BLAST_MultiArena_alloc(a, &block, 1);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NOT_NULL(block);
    TEST_ASSERT_NOT_NULL(a->slots[0]);

    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_alloc_routes_to_correct_slot(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);

    // 64 bytes — should land in slot 6 (2^6 = 64)
    void *block = NULL;
    BLAST_Error e = BLAST_MultiArena_alloc(a, &block, 64);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NOT_NULL(block);
    TEST_ASSERT_NOT_NULL(a->slots[6]);
    // Smaller slots should remain unused
    for (size_t i = 0; i < 6; i++) {
        TEST_ASSERT_NULL(a->slots[i]);
    }

    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_alloc_creates_slot_lazily(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);

    // Before any allocation, all slots are NULL
    for (size_t i = 0; i < BLAST_MULTIARENA_SLOTS; i++) {
        TEST_ASSERT_NULL(a->slots[i]);
    }

    void *block = NULL;
    BLAST_MultiArena_alloc(a, &block, 8);

    // Only slot 3 (2^3 = 8) should be populated
    TEST_ASSERT_NOT_NULL(a->slots[3]);
    for (size_t i = 0; i < BLAST_MULTIARENA_SLOTS; i++) {
        if (i != 3) TEST_ASSERT_NULL(a->slots[i]);
    }

    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_footprint_empty(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    // footprint = 0 + sizeof(max_mem) + sizeof(budget) + BLAST_MULTIARENA_SLOTS * sizeof(FlexArena*)
    //           = 0 + 8 + 8 + 64*8 = 528
    TEST_ASSERT_EQUAL(528, BLAST_MultiArena_footprint(a));
    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_footprint_one_slot(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    void *block = NULL;
    // Slot 3 (s_block=8, n_block=1): FlexArena footprint = (8*8*1+24) + 40 = 128
    BLAST_MultiArena_alloc(a, &block, 8);
    // footprint = 128 + 528 = 656
    TEST_ASSERT_EQUAL(656, BLAST_MultiArena_footprint(a));
    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_footprint_two_slots(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    void *b8 = NULL, *b64 = NULL;
    BLAST_MultiArena_alloc(a, &b8,  8);
    BLAST_MultiArena_alloc(a, &b64, 64);
    // footprint = (128 + 576) + 528 = 1232
    TEST_ASSERT_EQUAL(1232, BLAST_MultiArena_footprint(a));
    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_alloc_within_limit(void) {
    BLAST_MultiArena *a = NULL;
    // chunk_space for 8-byte slot = 88; actual FlexArena footprint deducted = 128
    // budget of 128 passes the check and avoids underflow after deduction
    BLAST_MultiArena_create_limit(&a, 128);

    void *block = NULL;
    BLAST_Error e = BLAST_MultiArena_alloc(a, &block, 8);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NOT_NULL(block);

    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_alloc_exceeds_limit(void) {
    BLAST_MultiArena *a = NULL;
    // chunk_space for 8-byte slot = 88; budget of 87 fails the check
    BLAST_MultiArena_create_limit(&a, 87);

    void *block = NULL;
    BLAST_Error e = BLAST_MultiArena_alloc(a, &block, 8);
    TEST_ASSERT_EQUAL(BLAST_ARENA_FULL_CODE, e.code);
    TEST_ASSERT_NULL(block);

    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_alloc_second_slot_exceeds_limit(void) {
    BLAST_MultiArena *a = NULL;
    // budget of 128: allows first slot (8 bytes, deducts 128 → budget = 0)
    // chunk_space for 16-byte slot = 96; 0 < 96, so second slot is rejected
    BLAST_MultiArena_create_limit(&a, 128);

    void *b8 = NULL;
    BLAST_Error e8 = BLAST_MultiArena_alloc(a, &b8, 8);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e8.code);
    TEST_ASSERT_NOT_NULL(b8);

    void *b16 = NULL;
    BLAST_Error e = BLAST_MultiArena_alloc(a, &b16, 16);
    TEST_ASSERT_EQUAL(BLAST_ARENA_FULL_CODE, e.code);
    TEST_ASSERT_NULL(b16);

    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_alloc_multiple_sizes(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);

    void *b8 = NULL, *b64 = NULL;
    BLAST_Error e8  = BLAST_MultiArena_alloc(a, &b8,  8);
    BLAST_Error e64 = BLAST_MultiArena_alloc(a, &b64, 64);

    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e8.code);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e64.code);
    TEST_ASSERT_NOT_NULL(b8);
    TEST_ASSERT_NOT_NULL(b64);
    // Different slots, so pointers must not alias
    TEST_ASSERT_NOT_EQUAL(b8, b64);

    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_free(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);

    void *block = NULL;
    BLAST_MultiArena_alloc(a, &block, 8);
    TEST_ASSERT_NOT_NULL(block);

    BLAST_Error e = BLAST_MultiArena_free(a, &block);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NULL(block);

    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_free_null_arena(void) {
    void *block = (void*)0x1;
    BLAST_Error e = BLAST_MultiArena_free(NULL, &block);
    TEST_ASSERT_EQUAL(BLAST_BAD_ALLOCATOR_CODE, e.code);
}

void test_MultiArena_free_allows_realloc(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);

    void *b0 = NULL, *b1 = NULL;
    BLAST_MultiArena_alloc(a, &b0, 8);
    BLAST_MultiArena_free(a, &b0);

    // After freeing, the slot should accept a new allocation
    BLAST_Error e = BLAST_MultiArena_alloc(a, &b1, 8);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NOT_NULL(b1);

    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_availablemem_empty(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    // No active slots — nothing available
    TEST_ASSERT_EQUAL(0, BLAST_MultiArena_availablemem(a));
    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_availablemem_after_alloc(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    void *block = NULL;
    // Slot 3 (s_block=8, n_block=1): 1 block allocated, none free
    BLAST_MultiArena_alloc(a, &block, 8);
    TEST_ASSERT_EQUAL(0, BLAST_MultiArena_availablemem(a));
    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_availablemem_after_free(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    void *block = NULL;
    BLAST_MultiArena_alloc(a, &block, 8);
    BLAST_MultiArena_free(a, &block);
    // Slot 3: 1 free block of 8 bytes
    TEST_ASSERT_EQUAL(8, BLAST_MultiArena_availablemem(a));
    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_availablemem_two_slots(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    void *b8 = NULL, *b64 = NULL;
    // Slot 3 (s_block=8, n_block=1), slot 6 (s_block=64, n_block=1): both full
    BLAST_MultiArena_alloc(a, &b8,  8);
    BLAST_MultiArena_alloc(a, &b64, 64);
    TEST_ASSERT_EQUAL(0, BLAST_MultiArena_availablemem(a));

    BLAST_MultiArena_free(a, &b8);
    // Slot 3 free (8 bytes), slot 6 still used (0) → 8
    TEST_ASSERT_EQUAL(8, BLAST_MultiArena_availablemem(a));

    BLAST_MultiArena_destroy(&a);
}
