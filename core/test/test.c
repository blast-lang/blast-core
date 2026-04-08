#include "unity.h"

#include "test_mem.c"

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();

    // mem
    RUN_TEST(test_FixedArena_create);
    RUN_TEST(test_FixedArena_alloc);
    RUN_TEST(test_FixedArena_alloc_null_arena);
    RUN_TEST(test_FixedArena_free);
    RUN_TEST(test_FixedArena_realloc_after_free);

    return UNITY_END();
}
