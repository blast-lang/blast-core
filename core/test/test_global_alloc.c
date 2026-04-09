#include "mem.h"
#include "unity.h"

void test_alloc_returns_non_null(void) {
    void *ptr = BLAST_alloc(8);
    TEST_ASSERT_NOT_NULL(ptr);
    BLAST_free(ptr);
}

void test_alloc_is_writable(void) {
    uint32_t *ptr = (uint32_t*)BLAST_alloc(sizeof(uint32_t));
    TEST_ASSERT_NOT_NULL(ptr);
    *ptr = 42;
    TEST_ASSERT_EQUAL_UINT32(42, *ptr);
    BLAST_free(ptr);
}

void test_alloc_multiple_no_alias(void) {
    void *a = BLAST_alloc(8);
    void *b = BLAST_alloc(8);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_EQUAL(a, b);
    BLAST_free(a);
    BLAST_free(b);
}

void test_free_returns_no_error(void) {
    void *ptr = BLAST_alloc(8);
    BLAST_Error e = BLAST_free(ptr);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
}

void test_alloc_free_realloc(void) {
    void *a = BLAST_alloc(8);
    BLAST_free(a);
    void *b = BLAST_alloc(8);
    TEST_ASSERT_NOT_NULL(b);
    BLAST_free(b);
}

void test_alloc_different_sizes(void) {
    void *a = BLAST_alloc(8);
    void *b = BLAST_alloc(64);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_EQUAL(a, b);
    BLAST_free(a);
    BLAST_free(b);
}
