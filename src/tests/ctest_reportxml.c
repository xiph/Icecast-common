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

#include <igloo/reportxml.h>

static void test_create_unref(void)
{
    igloo_reportxml_t *report;
    igloo_reportxml_node_t *node;
    igloo_reportxml_database_t *database;

    report = igloo_ro_new(igloo_reportxml_t);
    ctest_test("report created", !igloo_RO_IS_NULL(report));
    ctest_test("un-referenced", igloo_ro_unref(report) == 0);

    node = igloo_reportxml_node_new(igloo_REPORTXML_NODE_TYPE_REPORT, NULL, NULL, NULL);
    ctest_test("node created", !igloo_RO_IS_NULL(node));
    ctest_test("un-referenced", igloo_ro_unref(node) == 0);

    database = igloo_ro_new(igloo_reportxml_database_t);
    ctest_test("database created", !igloo_RO_IS_NULL(database));
    ctest_test("un-referenced", igloo_ro_unref(database) == 0);
}

int main (void)
{
    ctest_init();

    test_create_unref();

    ctest_fin();

    return 0;
}
