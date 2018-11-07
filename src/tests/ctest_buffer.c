/* Icecast
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2018,      Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>,
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "ctest_lib.h"

#include <igloo/ro.h>
#include <igloo/buffer.h>

static void test_create_ref_unref(void)
{
    igloo_buffer_t *a;

    a = igloo_buffer_new(-1, NULL, igloo_RO_NULL);
    ctest_test("buffer created", a != NULL);
    ctest_test("un-referenced", igloo_ro_unref(a) == 0);

    a = igloo_ro_new(igloo_buffer_t);
    ctest_test("buffer created", a != NULL);
    ctest_test("un-referenced", igloo_ro_unref(a) == 0);

}

static void test_name(void)
{
    igloo_buffer_t *a;
    const char *name = "test object name";
    const char *ret;

    a = igloo_buffer_new(-1, name, igloo_RO_NULL);
    ctest_test("buffer created", a != NULL);

    ret = igloo_ro_get_name(a);
    ctest_test("get name", ret != NULL);
    ctest_test("name match", strcmp(name, ret) == 0);

    ctest_test("un-referenced", igloo_ro_unref(a) == 0);
}

static void test_associated(void)
{
    igloo_ro_base_t *a;
    igloo_buffer_t *b;

    a = igloo_ro_new(igloo_ro_base_t);
    ctest_test("refobject created", !igloo_RO_IS_NULL(a));


    b = igloo_buffer_new(-1, NULL, a);
    ctest_test("buffer created with associated", !igloo_RO_IS_NULL(b));

    ctest_test("un-referenced (1 of 2)", igloo_ro_unref(b) == 0);
    ctest_test("un-referenced (2 of 2)", igloo_ro_unref(a) == 0);
}

static void test_empty(void)
{
    igloo_buffer_t *a;
    const void *data = &data;
    size_t length = 5;
    const char *string;
    int ret;

    a = igloo_ro_new(igloo_buffer_t);
    ctest_test("buffer created", a != NULL);

    ret = igloo_buffer_get_data(a, &data, &length);
    ctest_test("got data and length from buffer", ret == 0);
    if (ret == 0) {
        ctest_test("data is updated", data != &data);
        ctest_test("length is zero", length == 0);
    }

    data = &data;
    ret = igloo_buffer_get_data(a, &data, NULL);
    ctest_test("got data from buffer", ret == 0);
    if (ret == 0) {
        ctest_test("data is updated", data != &data);
    }

    length = 5;
    ret = igloo_buffer_get_data(a, NULL, &length);
    ctest_test("got length from buffer", ret == 0);
    if (ret == 0) {
        ctest_test("length is zero", length == 0);
    }

    ret = igloo_buffer_get_string(a, &string);
    ctest_test("got string from buffer", ret == 0);
    if (ret == 0) {
        ctest_test("string is non-NULL", string != NULL);
        if (string != NULL) {
            ctest_test("string is empty", *string == 0);
        }
    }

    ctest_test("un-referenced", igloo_ro_unref(a) == 0);
}

static void test_string(void)
{
    igloo_buffer_t *a;
    const char *hw = "Hello World!";
    const char *count = "0 1 2 3 4";
    const char *combined = "Hello World!" "0 1 2 3 4";
    const char *string = NULL;
    int ret;

    a = igloo_ro_new(igloo_buffer_t);
    ctest_test("buffer created", a != NULL);
    ctest_test("pushed string", igloo_buffer_push_string(a, hw) == 0);
    ret = igloo_buffer_get_string(a, &string);
    ctest_test("got strong", ret == 0);
    if (ret == 0) {
        ctest_test("string is non-NULL", string != NULL);
        if (string != NULL) {
            ctest_test("string matches input", strcmp(string, hw) == 0);
        }
    }

    ctest_test("pushed string", igloo_buffer_push_string(a, count) == 0);
    string = NULL;
    ret = igloo_buffer_get_string(a, &string);
    ctest_test("got strong", ret == 0);
    if (ret == 0) {
        ctest_test("string is non-NULL", string != NULL);
        if (string != NULL) {
            ctest_test("string matches combined input", strcmp(string, combined) == 0);
        }
    }

    ctest_test("un-referenced", igloo_ro_unref(a) == 0);
}

static void test_binary(void)
{
    igloo_buffer_t *a;
    char pattern_a[8] = {0x01, 0x10, 0x80, 0xFF,  0x00, 0x55, 0xEE, 0xAA};
    char pattern_b[9] = {0x02, 0x03, 0xF0, 0x80,  0x0F, 0x04, 0x1A, 0x7F, 0x33};
    int ret;
    size_t length;
    const void *data;

    a = igloo_ro_new(igloo_buffer_t);
    ctest_test("buffer created", a != NULL);

    ctest_test("pushed data pattern a", igloo_buffer_push_data(a, pattern_a, sizeof(pattern_a)) == 0);
    length = sizeof(pattern_a) + 42;
    data = &data;
    ret = igloo_buffer_get_data(a, &data, &length);
    ctest_test("got data", ret == 0);
    if (ret == 0) {
        ctest_test("correct length was returned", length == sizeof(pattern_a));
        ctest_test("data is non-NULL", data != NULL);
        ctest_test("data has been set", data != &data);
        if (length == sizeof(pattern_a) && data != NULL && data != &data) {
            ctest_test("data matches pattern", memcmp(data, pattern_a, sizeof(pattern_a)) == 0);
        }
    }

    ctest_test("pushed data pattern b", igloo_buffer_push_data(a, pattern_b, sizeof(pattern_b)) == 0);
    length = sizeof(pattern_a) + sizeof(pattern_b) + 42;
    data = &data;
    ret = igloo_buffer_get_data(a, &data, &length);
    ctest_test("got data", ret == 0);
    if (ret == 0) {
        ctest_test("correct length was returned", length == (sizeof(pattern_a) + sizeof(pattern_b)));
        ctest_test("data is non-NULL", data != NULL);
        ctest_test("data has been set", data != &data);
        if (length == (sizeof(pattern_a) + sizeof(pattern_b)) && data != NULL && data != &data) {
            ctest_test("data matches combined pattern", memcmp(data, pattern_a, sizeof(pattern_a)) == 0 && memcmp(data + sizeof(pattern_a), pattern_b, sizeof(pattern_b)) == 0);
        }
    }


    ctest_test("un-referenced", igloo_ro_unref(a) == 0);
}

static void test__compare_to_string(igloo_buffer_t *a, const char *testname, const char *pattern)
{
    const char *string = NULL;
    int ret;

    ret = igloo_buffer_get_string(a, &string);
    ctest_test("got strong", ret == 0);
    if (ret == 0) {
        ctest_test("string is non-NULL", string != NULL);
        if (string != NULL) {
            ctest_test(testname, strcmp(string, pattern) == 0);
            ctest_diagnostic_printf("string=\"%s\", pattern=\"%s\"", string, pattern);
        }
    }
}

static void test_shift(void)
{
    igloo_buffer_t *a;
    const char *pattern = "AABBBCC";

    a = igloo_ro_new(igloo_buffer_t);
    ctest_test("buffer created", a != NULL);

    ctest_test("pushed string", igloo_buffer_push_string(a, pattern) == 0);
    test__compare_to_string(a, "string matches input", pattern);
    ctest_test("shifted data by 0 bytes", igloo_buffer_shift(a, 0) == 0);
    test__compare_to_string(a, "string matches input (no shift happened)", pattern);
    ctest_test("shifted data by 2 bytes", igloo_buffer_shift(a, 2) == 0);
    test__compare_to_string(a, "string matches shifted input", pattern + 2);
    ctest_test("shifted data by 3 bytes", igloo_buffer_shift(a, 3) == 0);
    test__compare_to_string(a, "string matches shifted input", pattern + 2 + 3);
    ctest_test("shifted data by 3 bytes", igloo_buffer_shift(a, 2) == 0);
    test__compare_to_string(a, "string matches shifted input", pattern + 2 + 3 + 2);
    ctest_test("shifted data beyond end (42 bytes)", igloo_buffer_shift(a, 42) != 0);

    ctest_test("un-referenced", igloo_ro_unref(a) == 0);
}

static void test_length(void)
{
    igloo_buffer_t *a;
    const char *pattern = "AABBBCC";
    const char *match_a = "AABBB";
    const char *match_b = "AABB";
    const char *match_c = "";

    a = igloo_ro_new(igloo_buffer_t);
    ctest_test("buffer created", a != NULL);

    ctest_test("pushed string", igloo_buffer_push_string(a, pattern) == 0);
    test__compare_to_string(a, "string matches input", pattern);
    ctest_test("Set length to match pattern a", igloo_buffer_set_length(a, strlen(match_a)) == 0);
    test__compare_to_string(a, "string matches pattern a", match_a);
    ctest_test("Set length to match pattern b", igloo_buffer_set_length(a, strlen(match_b)) == 0);
    test__compare_to_string(a, "string matches pattern a", match_b);
    ctest_test("Set length to match pattern c", igloo_buffer_set_length(a, strlen(match_c)) == 0);
    test__compare_to_string(a, "string matches pattern a", match_c);
    ctest_test("Set length to match pattern a (again)", igloo_buffer_set_length(a, strlen(match_a)) != 0);
    test__compare_to_string(a, "string still matches pattern c", match_c);

    ctest_test("un-referenced", igloo_ro_unref(a) == 0);
}

static void test_printf(void)
{
    igloo_buffer_t *a;
    const char *str = "Hello World!";
    const int num = -127;
    const char *match_a = ":Hello World!:";
    const char *match_b = ":Hello World!:<-127 >";
    const char *match_c = ":Hello World!:<-127 >? +127?";

    a = igloo_ro_new(igloo_buffer_t);
    ctest_test("buffer created", a != NULL);

    ctest_test("Set length to match pattern a", igloo_buffer_push_printf(a, ":%s:", str) == 0);
    test__compare_to_string(a, "string matches pattern a", match_a);
    ctest_test("Set length to match pattern a", igloo_buffer_push_printf(a, "<%-5i>", num) == 0);
    test__compare_to_string(a, "string matches pattern b", match_b);
    ctest_test("Set length to match pattern a", igloo_buffer_push_printf(a, "?%+5i?", -num) == 0);
    test__compare_to_string(a, "string matches pattern c", match_c);

    ctest_test("un-referenced", igloo_ro_unref(a) == 0);
}

static void test_push_buffer(void)
{
    igloo_buffer_t *a;
    igloo_buffer_t *b;
    const char *pattern = "AABBBCC";
    const char *match_a = "AABBBCCAABBBCC";

    a = igloo_ro_new(igloo_buffer_t);
    ctest_test("buffer a created", a != NULL);
    b = igloo_ro_new(igloo_buffer_t);
    ctest_test("buffer b created", b != NULL);

    ctest_test("pushed string", igloo_buffer_push_string(a, pattern) == 0);
    test__compare_to_string(a, "string matches input", pattern);

    ctest_test("pushed buffer a to b", igloo_buffer_push_buffer(b, a) == 0);
    test__compare_to_string(b, "string matches input", pattern);

    ctest_test("pushed buffer a to b", igloo_buffer_push_buffer(b, a) == 0);
    test__compare_to_string(b, "string matches pattern a", match_a);

    ctest_test("un-referenced b", igloo_ro_unref(b) == 0);
    ctest_test("un-referenced a", igloo_ro_unref(a) == 0);
}

int main (void)
{
    ctest_init();


    test_create_ref_unref();

    test_name();
    test_associated();

    test_empty();
    test_string();
    test_binary();

    test_shift();
    test_length();

    test_printf();
    test_push_buffer();

    ctest_fin();

    return 0;
}
