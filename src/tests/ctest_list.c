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
#include <igloo/list.h>

static void test_create_ref_unref(void)
{
    igloo_list_t *a;

    a = igloo_ro_new(igloo_list_t);
    ctest_test("list created", !igloo_RO_IS_NULL(a));

    ctest_test("un-referenced", igloo_ro_unref(a) == 0);
}

static void test__create_push_unshift(igloo_list_t **list, igloo_ro_base_t **a, igloo_ro_base_t **b)
{
    *list = NULL;
    *a = NULL;
    *b = NULL;

    *list = igloo_ro_new(igloo_list_t);

    ctest_test("list created", !igloo_RO_IS_NULL(*list));

    *a = igloo_ro_new(igloo_ro_base_t);
    ctest_test("test object a created", !igloo_RO_IS_NULL(*a));

    *b = igloo_ro_new(igloo_ro_base_t);
    ctest_test("test object b created", !igloo_RO_IS_NULL(*b));

    ctest_test("test object a pushed", igloo_list_push(*list, *a) == 0);
    ctest_test("test object b unshifted", igloo_list_unshift(*list, *b) == 0);
}

static void test_list_push_unshift(void)
{
    igloo_list_t *list;
    igloo_ro_base_t *a;
    igloo_ro_base_t *b;

    test__create_push_unshift(&list, &a, &b);

    ctest_test("un-referenced a", igloo_ro_unref(a) == 0);
    ctest_test("un-referenced b", igloo_ro_unref(b) == 0);
    ctest_test("un-referenced list", igloo_ro_unref(list) == 0);
}

static void test_list_push_unshift_pop(void)
{
    igloo_list_t *list;
    igloo_ro_base_t *a;
    igloo_ro_base_t *b;
    igloo_ro_t ret;

    test__create_push_unshift(&list, &a, &b);

    ret = igloo_list_pop(list);
    ctest_test("popped element", !igloo_RO_IS_NULL(ret));
    ctest_test("popped element matches a", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == a);
    ctest_test("un-referenced popped element", igloo_ro_unref(ret) == 0);

    ctest_test("un-referenced a", igloo_ro_unref(a) == 0);
    ctest_test("un-referenced b", igloo_ro_unref(b) == 0);
    ctest_test("un-referenced list", igloo_ro_unref(list) == 0);
}

static void test_list_push_unshift_pop_pop(void)
{
    igloo_list_t *list;
    igloo_ro_base_t *a;
    igloo_ro_base_t *b;
    igloo_ro_t ret;

    test__create_push_unshift(&list, &a, &b);

    ret = igloo_list_pop(list);
    ctest_test("popped element", !igloo_RO_IS_NULL(ret));
    ctest_test("popped element matches a", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == a);
    ctest_test("un-referenced popped element", igloo_ro_unref(ret) == 0);

    ret = igloo_list_pop(list);
    ctest_test("popped element", !igloo_RO_IS_NULL(ret));
    ctest_test("popped element matches b", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == b);
    ctest_test("un-referenced popped element", igloo_ro_unref(ret) == 0);

    ret = igloo_list_pop(list);
    ctest_test("popped no element", igloo_RO_IS_NULL(ret));
    igloo_ro_unref(ret); /* just in case we got an element */

    ctest_test("un-referenced a", igloo_ro_unref(a) == 0);
    ctest_test("un-referenced b", igloo_ro_unref(b) == 0);
    ctest_test("un-referenced list", igloo_ro_unref(list) == 0);
}

static void test_list_push_unshift_shift(void)
{
    igloo_list_t *list;
    igloo_ro_base_t *a;
    igloo_ro_base_t *b;
    igloo_ro_t ret;

    test__create_push_unshift(&list, &a, &b);

    ret = igloo_list_shift(list);
    ctest_test("shifted element", !igloo_RO_IS_NULL(ret));
    ctest_test("shifted element matches b", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == b);
    ctest_test("un-referenced shifted element", igloo_ro_unref(ret) == 0);

    ctest_test("un-referenced a", igloo_ro_unref(a) == 0);
    ctest_test("un-referenced b", igloo_ro_unref(b) == 0);
    ctest_test("un-referenced list", igloo_ro_unref(list) == 0);
}

static void test_list_push_unshift_shift_shift(void)
{
    igloo_list_t *list;
    igloo_ro_base_t *a;
    igloo_ro_base_t *b;
    igloo_ro_t ret;

    test__create_push_unshift(&list, &a, &b);

    ret = igloo_list_shift(list);
    ctest_test("shifted element", !igloo_RO_IS_NULL(ret));
    ctest_test("shifted element matches b", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == b);
    ctest_test("un-referenced shifted element", igloo_ro_unref(ret) == 0);

    ret = igloo_list_shift(list);
    ctest_test("shifted element", !igloo_RO_IS_NULL(ret));
    ctest_test("shifted element matches a", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == a);
    ctest_test("un-referenced shifted element", igloo_ro_unref(ret) == 0);

    ret = igloo_list_shift(list);
    ctest_test("shifted no element", igloo_RO_IS_NULL(ret));
    igloo_ro_unref(ret); /* just in case we got an element */

    ctest_test("un-referenced a", igloo_ro_unref(a) == 0);
    ctest_test("un-referenced b", igloo_ro_unref(b) == 0);
    ctest_test("un-referenced list", igloo_ro_unref(list) == 0);
}


static void test_list_push_unshift_pop_shift(void)
{
    igloo_list_t *list;
    igloo_ro_base_t *a;
    igloo_ro_base_t *b;
    igloo_ro_t ret;

    test__create_push_unshift(&list, &a, &b);

    ret = igloo_list_pop(list);
    ctest_test("popped element", !igloo_RO_IS_NULL(ret));
    ctest_test("popped element matches a", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == a);
    ctest_test("un-referenced popped element", igloo_ro_unref(ret) == 0);

    ret = igloo_list_shift(list);
    ctest_test("shifted element", !igloo_RO_IS_NULL(ret));
    ctest_test("shifted element matches b", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == b);
    ctest_test("un-referenced shifted element", igloo_ro_unref(ret) == 0);

    ctest_test("un-referenced a", igloo_ro_unref(a) == 0);
    ctest_test("un-referenced b", igloo_ro_unref(b) == 0);
    ctest_test("un-referenced list", igloo_ro_unref(list) == 0);
}
static void test_list_push_unshift_shift_pop(void)
{
    igloo_list_t *list;
    igloo_ro_base_t *a;
    igloo_ro_base_t *b;
    igloo_ro_t ret;

    test__create_push_unshift(&list, &a, &b);

    ret = igloo_list_shift(list);
    ctest_test("shifted element", !igloo_RO_IS_NULL(ret));
    ctest_test("shifted element matches b", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == b);
    ctest_test("un-referenced shifted element", igloo_ro_unref(ret) == 0);

    ret = igloo_list_pop(list);
    ctest_test("popped element", !igloo_RO_IS_NULL(ret));
    ctest_test("popped element matches a", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == a);
    ctest_test("un-referenced popped element", igloo_ro_unref(ret) == 0);

    ctest_test("un-referenced a", igloo_ro_unref(a) == 0);
    ctest_test("un-referenced b", igloo_ro_unref(b) == 0);
    ctest_test("un-referenced list", igloo_ro_unref(list) == 0);
}

static void test_list_merge(void)
{
    igloo_list_t *list_a;
    igloo_list_t *list_b;
    igloo_ro_base_t *a;
    igloo_ro_base_t *b;
    igloo_ro_t ret;

    list_a = igloo_ro_new(igloo_list_t);
    ctest_test("list a created", !igloo_RO_IS_NULL(list_a));

    list_b = igloo_ro_new(igloo_list_t);
    ctest_test("list a created", !igloo_RO_IS_NULL(list_b));

    a = igloo_ro_new(igloo_ro_base_t);
    ctest_test("test object a created", !igloo_RO_IS_NULL(a));

    b = igloo_ro_new(igloo_ro_base_t);
    ctest_test("test object b created", !igloo_RO_IS_NULL(b));

    ctest_test("test object a pushed to list a", igloo_list_push(list_a, a) == 0);
    ctest_test("test object b pushed to list b", igloo_list_push(list_b, b) == 0);

    ctest_test("merged list b into list a", igloo_list_merge(list_a, list_b) == 0);

    ret = igloo_list_shift(list_a);
    ctest_test("shifted element", !igloo_RO_IS_NULL(ret));
    ctest_test("shifted element matches a", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == a);
    ctest_test("un-referenced shifted element", igloo_ro_unref(ret) == 0);

    ret = igloo_list_shift(list_a);
    ctest_test("shifted element", !igloo_RO_IS_NULL(ret));
    ctest_test("shifted element matches b", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == b);
    ctest_test("un-referenced shifted element", igloo_ro_unref(ret) == 0);

    ret = igloo_list_shift(list_a);
    ctest_test("shifted no element", igloo_RO_IS_NULL(ret));
    igloo_ro_unref(ret); /* just in case we got an element */

    ret = igloo_list_shift(list_b);
    ctest_test("shifted element", !igloo_RO_IS_NULL(ret));
    ctest_test("shifted element matches b", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == b);
    ctest_test("un-referenced shifted element", igloo_ro_unref(ret) == 0);

    ret = igloo_list_shift(list_b);
    ctest_test("shifted no element", igloo_RO_IS_NULL(ret));
    igloo_ro_unref(ret); /* just in case we got an element */

    ctest_test("un-referenced a", igloo_ro_unref(a) == 0);
    ctest_test("un-referenced b", igloo_ro_unref(b) == 0);
    ctest_test("un-referenced list a", igloo_ro_unref(list_a) == 0);
    ctest_test("un-referenced list b", igloo_ro_unref(list_b) == 0);
}

static void test_list_set_type(void)
{
    igloo_list_t *list;
    igloo_ro_base_t *a;
    igloo_list_t *b;
    igloo_ro_t ret;

    list = igloo_ro_new(igloo_list_t);
    ctest_test("list created", !igloo_RO_IS_NULL(list));

    a = igloo_ro_new(igloo_ro_base_t);
    ctest_test("test object a created", !igloo_RO_IS_NULL(a));

    b = igloo_ro_new(igloo_list_t);
    ctest_test("test object b created", !igloo_RO_IS_NULL(b));

    ctest_test("pinned list to type igloo_ro_base_t", igloo_list_set_type(list, igloo_ro_base_t) == 0);
    ctest_test("pinned list to type igloo_list_t", igloo_list_set_type(list, igloo_list_t) == 0);
    ctest_test("pinned list to type igloo_ro_base_t", igloo_list_set_type(list, igloo_ro_base_t) == 0);

    ctest_test("test object a pushed to list", igloo_list_push(list, a) == 0);

    ctest_test("pinned list to type igloo_ro_base_t", igloo_list_set_type(list, igloo_ro_base_t) == 0);
    ctest_test("can not pin list to type igloo_list_t", igloo_list_set_type(list, igloo_list_t) != 0);
    ctest_test("pinned list to type igloo_ro_base_t", igloo_list_set_type(list, igloo_ro_base_t) == 0);

    ctest_test("test object b failed to pushed to list", igloo_list_push(list, b) != 0);

    ctest_test("list cleared", igloo_list_clear(list) == 0);

    ctest_test("pinned list to type igloo_ro_base_t", igloo_list_set_type(list, igloo_ro_base_t) == 0);
    ctest_test("can not pin list to type igloo_list_t", igloo_list_set_type(list, igloo_list_t) == 0);
    ctest_test("pinned list to type igloo_ro_base_t", igloo_list_set_type(list, igloo_ro_base_t) == 0);

    ctest_test("un-referenced list a", igloo_ro_unref(list) == 0);
    list = igloo_ro_new(igloo_list_t);
    ctest_test("list created", !igloo_RO_IS_NULL(list));

    ctest_test("test object a pushed to list", igloo_list_push(list, a) == 0);
    ctest_test("test object b pushed to list", igloo_list_push(list, b) == 0);

    ctest_test("pinned list to type igloo_ro_base_t", igloo_list_set_type(list, igloo_ro_base_t) != 0);
    ctest_test("pinned list to type igloo_list_t", igloo_list_set_type(list, igloo_list_t) != 0);
    ctest_test("pinned list to type igloo_ro_base_t", igloo_list_set_type(list, igloo_ro_base_t) != 0);

    ret = igloo_list_shift(list);
    ctest_test("shifted element", !igloo_RO_IS_NULL(ret));
    ctest_test("shifted element matches a", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == a);
    ctest_test("un-referenced shifted element", igloo_ro_unref(ret) == 0);

    ctest_test("pinned list to type igloo_ro_base_t", igloo_list_set_type(list, igloo_ro_base_t) != 0);
    ctest_test("can not pin list to type igloo_list_t", igloo_list_set_type(list, igloo_list_t) == 0);
    ctest_test("pinned list to type igloo_ro_base_t", igloo_list_set_type(list, igloo_ro_base_t) != 0);

    ctest_test("un-referenced a", igloo_ro_unref(a) == 0);
    ctest_test("un-referenced b", igloo_ro_unref(b) == 0);
    ctest_test("un-referenced list a", igloo_ro_unref(list) == 0);
}

static void test_list_iterator(void)
{
    igloo_list_t *list;
    igloo_ro_base_t * elements[3] = {NULL, NULL, NULL};
    igloo_list_iterator_storage_t storage;
    igloo_list_iterator_t *iterator;
    size_t i;

    test__create_push_unshift(&list, &(elements[1]), &(elements[0]));

    iterator = igloo_list_iterator_start(list, &storage, sizeof(storage));
    ctest_test("iterator created", iterator != NULL);

    for (i = 0; i < (sizeof(elements)/sizeof(*elements)); i++) {
        igloo_ro_t ret = igloo_list_iterator_next(iterator);
        ctest_test("shifted element matches corresponding element in list", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == elements[i]);
        if (!igloo_RO_IS_NULL(ret)) {
            ctest_test("un-referenced element returned by iterator", igloo_ro_unref(ret) == 0);
        }
    }

    ctest_test("rewinded iterator", igloo_list_iterator_rewind(iterator) == 0);

    for (i = 0; i < (sizeof(elements)/sizeof(*elements)); i++) {
        igloo_ro_t ret = igloo_list_iterator_next(iterator);
        ctest_test("shifted element matches corresponding element in list", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == elements[i]);
        if (!igloo_RO_IS_NULL(ret)) {
            ctest_test("un-referenced element returned by iterator", igloo_ro_unref(ret) == 0);
        }
    }

    ctest_test("rewinded iterator", igloo_list_iterator_rewind(iterator) == 0);

    for (i = 0; i < 1; i++) {
        igloo_ro_t ret = igloo_list_iterator_next(iterator);
        ctest_test("shifted element matches corresponding element in list", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == elements[i]);
        if (!igloo_RO_IS_NULL(ret)) {
            ctest_test("un-referenced element returned by iterator", igloo_ro_unref(ret) == 0);
        }
    }

    ctest_test("rewinded iterator", igloo_list_iterator_rewind(iterator) == 0);

    for (i = 0; i < (sizeof(elements)/sizeof(*elements)); i++) {
        igloo_ro_t ret = igloo_list_iterator_next(iterator);
        ctest_test("shifted element matches corresponding element in list", igloo_RO_TO_TYPE(ret, igloo_ro_base_t) == elements[i]);
        if (!igloo_RO_IS_NULL(ret)) {
            ctest_test("un-referenced element returned by iterator", igloo_ro_unref(ret) == 0);
        }
    }

    igloo_list_iterator_end(iterator);

    ctest_test("un-referenced a", igloo_ro_unref(elements[0]) == 0);
    ctest_test("un-referenced b", igloo_ro_unref(elements[1]) == 0);
    ctest_test("un-referenced list", igloo_ro_unref(list) == 0);
}

static void test_list_foreach(void)
{
    igloo_list_t *list;
    igloo_ro_base_t * elements[3] = {NULL, NULL, NULL};
    size_t i = 0;

    test__create_push_unshift(&list, &(elements[1]), &(elements[0]));

    igloo_list_foreach(list, igloo_ro_base_t, ret, {
        ctest_test("foreach returned element matches corresponding element in list", ret == elements[i]);
        i++;
    });

    ctest_test("un-referenced a", igloo_ro_unref(elements[0]) == 0);
    ctest_test("un-referenced b", igloo_ro_unref(elements[1]) == 0);
    ctest_test("un-referenced list", igloo_ro_unref(list) == 0);
}

int main (void)
{
    ctest_init();

    test_create_ref_unref();

    test_list_push_unshift();
    test_list_push_unshift_pop();
    test_list_push_unshift_pop_pop();
    test_list_push_unshift_shift();
    test_list_push_unshift_shift_shift();
    test_list_push_unshift_pop_shift();
    test_list_push_unshift_shift_pop();

    test_list_merge();

    test_list_set_type();

    test_list_iterator();

    test_list_foreach();

    ctest_fin();

    return 0;
}
