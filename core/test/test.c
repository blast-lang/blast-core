#include "unity.h"

#include "test_fixed_arena.c"
#include "test_flex_arena.c"

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();

    // mem
    RUN_TEST(test_FixedArena_create);
    RUN_TEST(test_FixedArena_alloc);
    RUN_TEST(test_FixedArena_alloc_null_arena);
    RUN_TEST(test_FixedArena_free);
    RUN_TEST(test_FixedArena_alloc_full);
    RUN_TEST(test_FixedArena_realloc_after_free);
    RUN_TEST(test_FixedArena_set_memory);
    RUN_TEST(test_FixedArena_footprint);
    RUN_TEST(test_FixedArena_footprint_with_alignment);
    RUN_TEST(test_FixedArena_is_full_on_create);
    RUN_TEST(test_FixedArena_is_full_after_all_alloc);
    RUN_TEST(test_FixedArena_is_full_after_partial_alloc);
    RUN_TEST(test_FixedArena_is_full_after_free);
    RUN_TEST(test_FixedArena_is_empty_on_create);
    RUN_TEST(test_FixedArena_is_empty_after_alloc);
    RUN_TEST(test_FixedArena_is_empty_after_free);

    // flex arena
    RUN_TEST(test_FlexArena_create);
    RUN_TEST(test_FlexArena_alloc);
    RUN_TEST(test_FlexArena_alloc_expands);
    RUN_TEST(test_FlexArena_alloc_full);
    RUN_TEST(test_FlexArena_heapsize);
    RUN_TEST(test_FlexArena_heapsize_expands);
    RUN_TEST(test_FlexArena_footprint);
    RUN_TEST(test_FlexArena_free);

    return UNITY_END();
}
