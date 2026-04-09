#include "str.h"
#include "unity.h"

void test_String_create(void) {
    BLAST_String *s = NULL;
    BLAST_Error e = BLAST_String_create(&s);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL(0, BLAST_String_len(s));
    TEST_ASSERT_EQUAL_STRING("", BLAST_String_data(s));
    BLAST_String_destroy(&s);
}

void test_String_destroy(void) {
    BLAST_String *s = NULL;
    BLAST_String_create(&s);
    BLAST_Error e = BLAST_String_destroy(&s);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_NULL(s);
}

void test_String_create_from(void) {
    BLAST_String *s = NULL;
    BLAST_Error e = BLAST_String_create_from(&s, "hello", 5);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_EQUAL(5, BLAST_String_len(s));
    TEST_ASSERT_EQUAL_STRING("hello", BLAST_String_data(s));
    BLAST_String_destroy(&s);
}

void test_String_create_from_partial(void) {
    BLAST_String *s = NULL;
    BLAST_String_create_from(&s, "hello", 3);
    TEST_ASSERT_EQUAL(3, BLAST_String_len(s));
    TEST_ASSERT_EQUAL_STRING("hel", BLAST_String_data(s));
    BLAST_String_destroy(&s);
}

void test_String_push(void) {
    BLAST_String *s = NULL;
    BLAST_String_create(&s);
    BLAST_Error e = BLAST_String_push(s, 'x');
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_EQUAL(1, BLAST_String_len(s));
    TEST_ASSERT_EQUAL_STRING("x", BLAST_String_data(s));
    BLAST_String_destroy(&s);
}

void test_String_push_null_terminated(void) {
    BLAST_String *s = NULL;
    BLAST_String_create(&s);
    BLAST_String_push(s, 'a');
    BLAST_String_push(s, 'b');
    BLAST_String_push(s, 'c');
    TEST_ASSERT_EQUAL(3, BLAST_String_len(s));
    TEST_ASSERT_EQUAL('\0', BLAST_String_data(s)[3]);
    BLAST_String_destroy(&s);
}

void test_String_append(void) {
    BLAST_String *s = NULL;
    BLAST_String_create(&s);
    BLAST_Error e = BLAST_String_append(s, "world", 5);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_EQUAL(5, BLAST_String_len(s));
    TEST_ASSERT_EQUAL_STRING("world", BLAST_String_data(s));
    BLAST_String_destroy(&s);
}

void test_String_append_multiple(void) {
    BLAST_String *s = NULL;
    BLAST_String_create(&s);
    BLAST_String_append(s, "foo", 3);
    BLAST_String_append(s, "bar", 3);
    TEST_ASSERT_EQUAL(6, BLAST_String_len(s));
    TEST_ASSERT_EQUAL_STRING("foobar", BLAST_String_data(s));
    BLAST_String_destroy(&s);
}

void test_String_append_grows(void) {
    BLAST_String *s = NULL;
    BLAST_String_create(&s);
    // Exceed the initial vector capacity (8) to trigger a realloc
    BLAST_String_append(s, "abcdefghi", 9);
    TEST_ASSERT_EQUAL(9, BLAST_String_len(s));
    TEST_ASSERT_EQUAL_STRING("abcdefghi", BLAST_String_data(s));
    TEST_ASSERT_EQUAL('\0', BLAST_String_data(s)[9]);
    BLAST_String_destroy(&s);
}

void test_String_len_empty(void) {
    BLAST_String *s = NULL;
    BLAST_String_create(&s);
    TEST_ASSERT_EQUAL(0, BLAST_String_len(s));
    BLAST_String_destroy(&s);
}

void test_String_data_is_valid_c_string(void) {
    BLAST_String *s = NULL;
    BLAST_String_create_from(&s, "test", 4);
    TEST_ASSERT_EQUAL(4, BLAST_String_len(s));
    TEST_ASSERT_EQUAL_STRING("test", BLAST_String_data(s));
    BLAST_String_destroy(&s);
}

void test_String_concat(void) {
    BLAST_String *a = NULL, *b = NULL;
    BLAST_String_create_from(&a, "foo", 3);
    BLAST_String_create_from(&b, "bar", 3);
    BLAST_Error e = BLAST_String_concat(a, b);
    TEST_ASSERT_EQUAL(BLAST_NO_ERROR_CODE, e.code);
    TEST_ASSERT_EQUAL(6, BLAST_String_len(a));
    TEST_ASSERT_EQUAL_STRING("foobar", BLAST_String_data(a));
    BLAST_String_destroy(&a);
    BLAST_String_destroy(&b);
}

void test_String_concat_empty_src(void) {
    BLAST_String *a = NULL, *b = NULL;
    BLAST_String_create_from(&a, "hello", 5);
    BLAST_String_create(&b);
    BLAST_String_concat(a, b);
    TEST_ASSERT_EQUAL(5, BLAST_String_len(a));
    TEST_ASSERT_EQUAL_STRING("hello", BLAST_String_data(a));
    BLAST_String_destroy(&a);
    BLAST_String_destroy(&b);
}

void test_String_concat_empty_dst(void) {
    BLAST_String *a = NULL, *b = NULL;
    BLAST_String_create(&a);
    BLAST_String_create_from(&b, "world", 5);
    BLAST_String_concat(a, b);
    TEST_ASSERT_EQUAL(5, BLAST_String_len(a));
    TEST_ASSERT_EQUAL_STRING("world", BLAST_String_data(a));
    BLAST_String_destroy(&a);
    BLAST_String_destroy(&b);
}

void test_String_concat_null_terminated(void) {
    BLAST_String *a = NULL, *b = NULL;
    BLAST_String_create_from(&a, "ab", 2);
    BLAST_String_create_from(&b, "cd", 2);
    BLAST_String_concat(a, b);
    TEST_ASSERT_EQUAL('\0', BLAST_String_data(a)[4]);
    BLAST_String_destroy(&a);
    BLAST_String_destroy(&b);
}
