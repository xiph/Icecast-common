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

#include <igloo/typedef.h>

typedef struct ctest_test_type_t ctest_test_type_t;
typedef struct ctest_test_type_free_t ctest_test_type_free_t;

typedef struct ctest_test_type_a_t ctest_test_type_a_t;
typedef struct ctest_test_type_b_t ctest_test_type_b_t;
typedef struct ctest_test_type_c_t ctest_test_type_c_t;
typedef struct ctest_test_type_d_t ctest_test_type_d_t;

#define igloo_RO_PRIVATETYPES \
    igloo_RO_TYPE(ctest_test_type_t) \
    igloo_RO_TYPE(ctest_test_type_free_t) \
    igloo_RO_TYPE(ctest_test_type_a_t) \
    igloo_RO_TYPE(ctest_test_type_b_t) \
    igloo_RO_TYPE(ctest_test_type_c_t) \
    igloo_RO_TYPE(ctest_test_type_d_t)

#include <igloo/ro.h>

struct ctest_test_type_t {
    igloo_ro_base_t __base;
};

struct ctest_test_type_free_t{
    igloo_ro_base_t __base;
};

struct ctest_test_type_a_t {
    igloo_ro_base_t __base;
    char padding[1024];
};

struct ctest_test_type_b_t {
    igloo_ro_base_t __base;
    char padding[131072];
};

struct ctest_test_type_c_t {
    char padding[sizeof(igloo_ro_base_t) - 1];
};

struct ctest_test_type_d_t {
    char padding[0];
};

igloo_RO_PRIVATE_TYPE(ctest_test_type_t);

static void test_freecb__freecb(igloo_ro_t self);
igloo_RO_PRIVATE_TYPE(ctest_test_type_free_t,
        igloo_RO_TYPEDECL_FREE(test_freecb__freecb),
        igloo_RO_TYPEDECL_NEW_NOOP()
        );

igloo_RO_PRIVATE_TYPE(ctest_test_type_a_t,
        igloo_RO_TYPEDECL_NEW_NOOP()
        );
igloo_RO_PRIVATE_TYPE(ctest_test_type_b_t,
        igloo_RO_TYPEDECL_NEW_NOOP()
        );
igloo_RO_PRIVATE_TYPE(ctest_test_type_c_t,
        igloo_RO_TYPEDECL_NEW_NOOP()
        );
igloo_RO_PRIVATE_TYPE(ctest_test_type_d_t,
        igloo_RO_TYPEDECL_NEW_NOOP()
        );

static void test_ptr(void)
{
    igloo_ro_t a;

    a = igloo_RO_NULL;
    ctest_test("NULL is NULL", igloo_RO_IS_NULL(a));

    if (!igloo_RO_IS_NULL(a))
        ctest_bailed_out();
}

static void test_create_ref_unref(void)
{
    igloo_ro_base_t *a;

    a = igloo_ro_new(igloo_ro_base_t);
    ctest_test("refobject created", !igloo_RO_IS_NULL(a));

    ctest_test("referenced", igloo_ro_ref(a) == 0);
    ctest_test("un-referenced (1 of 2)", igloo_ro_unref(a) == 0);
    ctest_test("un-referenced (2 of 2)", igloo_ro_unref(a) == 0);
}

static void test_typename(void)
{
    igloo_ro_base_t *a;
    const char *typename;

    a = igloo_ro_new(igloo_ro_base_t);
    ctest_test("refobject created", !igloo_RO_IS_NULL(a));

    typename = igloo_RO_GET_TYPENAME(a);
    ctest_test("got typename", typename != NULL);
    ctest_test("typename matches", strcmp(typename, "igloo_ro_base_t") == 0);

    ctest_test("un-referenced", igloo_ro_unref(a) == 0);
}

static void test_valid(void)
{
    igloo_ro_base_t *a;

    ctest_test("NULL is not valid", !igloo_RO_IS_VALID(igloo_RO_NULL, igloo_ro_base_t));

    a = igloo_ro_new(igloo_ro_base_t);
    ctest_test("refobject created", !igloo_RO_IS_NULL(a));

    ctest_test("is valid", igloo_RO_IS_VALID(a, igloo_ro_base_t));
    ctest_test("is valid as diffrent type", !igloo_RO_IS_VALID(a, ctest_test_type_t));

    ctest_test("un-referenced", igloo_ro_unref(a) == 0);
}

static void test_sizes(void)
{
    igloo_ro_t a;

    a = igloo_ro_new(ctest_test_type_a_t);
    ctest_test("refobject created with size=sizeof(igloo_ro_base_t) + 1024", !igloo_RO_IS_NULL(a));
    ctest_test("un-referenced", igloo_ro_unref(a) == 0);

    a = igloo_ro_new(ctest_test_type_b_t);
    ctest_test("refobject created with size=sizeof(igloo_ro_base_t) + 131072", !igloo_RO_IS_NULL(a));
    ctest_test("un-referenced", igloo_ro_unref(a) == 0);

    a = igloo_ro_new(ctest_test_type_c_t);
    ctest_test("refobject created with size=sizeof(igloo_ro_base_t) - 1", igloo_RO_IS_NULL(a));
    if (!igloo_RO_IS_NULL(a)) {
        ctest_test("un-referenced", igloo_ro_unref(a) == 0);
    }

    a = igloo_ro_new(ctest_test_type_d_t);
    ctest_test("refobject created with size=0", igloo_RO_IS_NULL(a));
    if (!igloo_RO_IS_NULL(a)) {
        ctest_test("un-referenced", igloo_ro_unref(a) == 0);
    }
}

static void test_name(void)
{
    igloo_ro_base_t *a;
    const char *name = "test object name";
    const char *ret;

    a = igloo_ro_new_ext(igloo_ro_base_t, name, igloo_RO_NULL);
    ctest_test("refobject created", !igloo_RO_IS_NULL(a));

    ret = igloo_ro_get_name(a);
    ctest_test("get name", ret != NULL);
    ctest_test("name match", strcmp(name, ret) == 0);

    ctest_test("un-referenced", igloo_ro_unref(a) == 0);
}

static void test_associated(void)
{
    igloo_ro_base_t *a, *b;

    a = igloo_ro_new(igloo_ro_base_t);
    ctest_test("refobject created", !igloo_RO_IS_NULL(a));

    b = igloo_ro_new_ext(igloo_ro_base_t, NULL, a);
    ctest_test("refobject created with associated", !igloo_RO_IS_NULL(b));

    ctest_test("un-referenced (1 of 2)", igloo_ro_unref(b) == 0);
    ctest_test("un-referenced (2 of 2)", igloo_ro_unref(a) == 0);
}

static size_t test_freecb__called;
static void test_freecb__freecb(igloo_ro_t self)
{
    test_freecb__called++;
}

static void test_freecb(void)
{
    ctest_test_type_free_t *a;

    test_freecb__called = 0;
    a = igloo_ro_new(ctest_test_type_free_t);
    ctest_test("refobject created", a != NULL);
    ctest_test("un-referenced", igloo_ro_unref(a) == 0);
    ctest_test("freecb called", test_freecb__called == 1);

    test_freecb__called = 0;
    a = igloo_ro_new(ctest_test_type_free_t);
    ctest_test("refobject created", a != NULL);
    ctest_test("referenced", igloo_ro_ref(a) == 0);
    ctest_test("freecb uncalled", test_freecb__called == 0);
    ctest_test("un-referenced (1 of 2)", igloo_ro_unref(a) == 0);
    ctest_test("freecb uncalled", test_freecb__called == 0);
    ctest_test("un-referenced (2 of 2)", igloo_ro_unref(a) == 0);
    ctest_test("freecb called", test_freecb__called == 1);
}

int main (void)
{
    ctest_init();

    test_ptr();

    if (ctest_bailed_out()) {
        ctest_fin();
        return 1;
    }

    test_create_ref_unref();

    test_typename();
    test_valid();

    test_sizes();

    test_name();
    test_associated();
    test_freecb();

    ctest_fin();

    return 0;
}
