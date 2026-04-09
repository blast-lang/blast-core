#include "unity.h"

#include "test_fixed_arena.c"
#include "test_flex_arena.c"
#include "test_multi_arena.c"
#include "test_global_alloc.c"

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
    RUN_TEST(test_FixedArena_availablemem_on_create);
    RUN_TEST(test_FixedArena_availablemem_after_alloc);
    RUN_TEST(test_FixedArena_availablemem_when_full);
    RUN_TEST(test_FixedArena_availablemem_after_free);

    // flex arena
    RUN_TEST(test_FlexArena_create);
    RUN_TEST(test_FlexArena_alloc);
    RUN_TEST(test_FlexArena_alloc_expands);
    RUN_TEST(test_FlexArena_alloc_full);
    RUN_TEST(test_FlexArena_footprint);
    RUN_TEST(test_FlexArena_free_keeps_chunk_with_busy_neighbor);
    RUN_TEST(test_FlexArena_free);
    RUN_TEST(test_FlexArena_availablemem_on_create);
    RUN_TEST(test_FlexArena_availablemem_after_alloc);
    RUN_TEST(test_FlexArena_availablemem_expands);

    // multi arena
    RUN_TEST(test_MultiArena_create);
    RUN_TEST(test_MultiArena_create_limit);
    RUN_TEST(test_MultiArena_alloc_small);
    RUN_TEST(test_MultiArena_alloc_routes_to_correct_slot);
    RUN_TEST(test_MultiArena_alloc_creates_slot_lazily);
    RUN_TEST(test_MultiArena_alloc_within_limit);
    RUN_TEST(test_MultiArena_alloc_exceeds_limit);
    RUN_TEST(test_MultiArena_alloc_second_slot_exceeds_limit);
    RUN_TEST(test_MultiArena_alloc_multiple_sizes);
    RUN_TEST(test_MultiArena_footprint_empty);
    RUN_TEST(test_MultiArena_footprint_one_slot);
    RUN_TEST(test_MultiArena_footprint_two_slots);
    RUN_TEST(test_MultiArena_free);
    RUN_TEST(test_MultiArena_free_null_arena);
    RUN_TEST(test_MultiArena_free_allows_realloc);
    RUN_TEST(test_MultiArena_availablemem_empty);
    RUN_TEST(test_MultiArena_availablemem_after_alloc);
    RUN_TEST(test_MultiArena_availablemem_after_free);
    RUN_TEST(test_MultiArena_availablemem_two_slots);

    // global allocator
    RUN_TEST(test_alloc_returns_non_null);
    RUN_TEST(test_alloc_is_writable);
    RUN_TEST(test_alloc_multiple_no_alias);
    RUN_TEST(test_free_returns_no_error);
    RUN_TEST(test_alloc_free_realloc);
    RUN_TEST(test_alloc_different_sizes);

    return UNITY_END();
}
