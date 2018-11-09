/* Copyright (C) 2018       Marvin Scholz <epirat07@gmail.com>
 * Copyright (C) 2018       Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
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

#ifndef _LIBIGLOO__TYPES_H_
#define _LIBIGLOO__TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/* For NULL */
#include <stddef.h>
/* For size_t and ssize_t */
#include <sys/types.h>

/* Included in case is not yet included */
#include "typedef.h"

typedef struct igloo_buffer_tag igloo_buffer_t;
typedef struct igloo_list_tag igloo_list_t;

typedef struct igloo_reportxml_tag igloo_reportxml_t;
typedef struct igloo_reportxml_node_tag igloo_reportxml_node_t;
typedef struct igloo_reportxml_database_tag igloo_reportxml_database_t;

/*
 * This header includes forward declarations for several basic types.
 */

typedef struct igloo_ro_type_tag igloo_ro_type_t;
typedef struct igloo_ro_base_tag igloo_ro_base_t;
igloo_RO_FORWARD_TYPE(igloo_ro_base_t);

#ifdef igloo_HAVE_TYPE_ATTRIBUTE_TRANSPARENT_UNION
typedef union __attribute__ ((__transparent_union__)) {
    /* Those are libigloo's own types */
    igloo_RO_TYPE(igloo_ro_base_t)
    igloo_RO_TYPE(igloo_buffer_t)
    igloo_RO_TYPE(igloo_list_t)
    igloo_RO_TYPE(igloo_reportxml_t)
    igloo_RO_TYPE(igloo_reportxml_node_t)
    igloo_RO_TYPE(igloo_reportxml_database_t)

    /* Now we add the current compilation unit's private types if any */
#ifdef igloo_RO_PRIVATETYPES
    igloo_RO_PRIVATETYPES
#endif

    /* Next are the application's types if any */
#ifdef igloo_RO_APPTYPES
    igloo_RO_APPTYPES
#endif

    /* And finnally all the types that are used by dependencies if any */
#ifdef igloo_RO_LIBTYPES
    igloo_RO_LIBTYPES
#endif
} igloo_ro_t;
#else
typedef void * igloo_ro_t;
#endif

#ifdef __cplusplus
}
#endif

#endif
