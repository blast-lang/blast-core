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

void test_MultiArena_heapsize_empty(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    // No slots active — heapsize is 0
    TEST_ASSERT_EQUAL(0, BLAST_MultiArena_heapsize(a));
    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_footprint_empty(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    // footprint = 0 + sizeof(max_mem) + BLAST_MULTIARENA_SLOTS * sizeof(FlexArena*)
    //           = 0 + 8 + 64*8 = 520
    TEST_ASSERT_EQUAL(520, BLAST_MultiArena_footprint(a));
    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_heapsize_one_slot(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    void *block = NULL;
    // Alloc 8 bytes — activates slot 3 (s_block=8, n_block=1+(8/1024)=1)
    // FlexArena footprint = FixedArena footprint + 40 = (8*8*1+24) + 40 = 128
    BLAST_MultiArena_alloc(a, &block, 8);
    TEST_ASSERT_EQUAL(128, BLAST_MultiArena_heapsize(a));
    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_footprint_one_slot(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    void *block = NULL;
    BLAST_MultiArena_alloc(a, &block, 8);
    // footprint = heapsize + 520 = 128 + 520 = 648
    TEST_ASSERT_EQUAL(648, BLAST_MultiArena_footprint(a));
    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_heapsize_two_slots(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    void *b8 = NULL, *b64 = NULL;
    // Slot 3 (s_block=8,  n_block=1): FlexArena footprint = (8*8*1+24)  + 40 = 128
    // Slot 6 (s_block=64, n_block=1): FlexArena footprint = (8*64*1+24) + 40 = 576
    BLAST_MultiArena_alloc(a, &b8,  8);
    BLAST_MultiArena_alloc(a, &b64, 64);
    TEST_ASSERT_EQUAL(128 + 576, BLAST_MultiArena_heapsize(a));
    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_footprint_two_slots(void) {
    BLAST_MultiArena *a = NULL;
    BLAST_MultiArena_create(&a);
    void *b8 = NULL, *b64 = NULL;
    BLAST_MultiArena_alloc(a, &b8,  8);
    BLAST_MultiArena_alloc(a, &b64, 64);
    // footprint = (128 + 576) + 520 = 1224
    TEST_ASSERT_EQUAL(1224, BLAST_MultiArena_footprint(a));
    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_alloc_within_limit(void) {
    BLAST_MultiArena *a = NULL;
    // max_mem of 8 — just enough for one 8-byte alloc
    BLAST_MultiArena_create_limit(&a, 8);

    void *block = NULL;
    BLAST_Error e = BLAST_MultiArena_alloc(a, &block, 8);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NOT_NULL(block);

    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_alloc_exceeds_limit(void) {
    BLAST_MultiArena *a = NULL;
    // max_mem of 7 — not enough for an 8-byte alloc
    BLAST_MultiArena_create_limit(&a, 7);

    void *block = NULL;
    BLAST_Error e = BLAST_MultiArena_alloc(a, &block, 8);
    TEST_ASSERT_EQUAL(BLAST_ARENA_FULL_CODE, e.code);
    TEST_ASSERT_NULL(block);

    BLAST_MultiArena_destroy(&a);
}

void test_MultiArena_alloc_second_slot_exceeds_limit(void) {
    BLAST_MultiArena *a = NULL;
    // Allow first slot (8 bytes) but not a second different-sized slot
    BLAST_MultiArena_create_limit(&a, 8);

    void *b8 = NULL;
    BLAST_MultiArena_alloc(a, &b8, 8);

    // Now try to open a second slot for 16 bytes — heapsize + 16 > 8
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
