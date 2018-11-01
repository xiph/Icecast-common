/* Copyright (C) 2018       Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "../include/igloo/typedef.h"
typedef struct igloo_instance_tag igloo_instance_t;

#define igloo_RO_PRIVATETYPES igloo_RO_TYPE(igloo_instance_t)

#include "../include/igloo/ro.h"
#include "private.h"

struct igloo_instance_tag {
    igloo_ro_base_t __base;
};

static size_t igloo_initialize__refc;

static void igloo_initialize__free(igloo_ro_t self)
{
    igloo_initialize__refc--;
    if (igloo_initialize__refc)
        return;

    igloo_resolver_shutdown();
    igloo_sock_shutdown();
    igloo_thread_shutdown();
    igloo_log_shutdown();
}

igloo_RO_PRIVATE_TYPE(igloo_instance_t,
        igloo_RO_TYPEDECL_FREE(igloo_initialize__free)
        );

igloo_ro_t     igloo_initialize(void)
{
    igloo_instance_t *ret;
    char name[128];

    if (!igloo_initialize__refc) {
        igloo_log_initialize();
        igloo_thread_initialize();
        igloo_sock_initialize();
        igloo_resolver_initialize();
    }

    snprintf(name, sizeof(name), "<libigloo instance %zu>", igloo_initialize__refc);

    ret = igloo_ro_new_raw(igloo_instance_t, name, igloo_RO_NULL);
    if (!ret)
        return igloo_RO_NULL;

    igloo_initialize__refc++;

    return ret;
}
