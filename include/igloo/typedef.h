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

#ifndef _LIBIGLOO__TYPEDEF_H_
#define _LIBIGLOO__TYPEDEF_H_

#ifdef __cplusplus
extern "C" {
#endif

/* This header includes only macros needed to define types.
 * This header must be included before "types.h" and "ro.h" if used.
 */

typedef struct igloo_ro_type_tag igloo_ro_type_t;

#define igloo_RO_TYPE(type)                         type * subtype__ ## type;

#define igloo_RO__CONTROL_VERSION	1
#define igloo_RO__DEFINE_TYPE(type, suffix, ...) \
static const igloo_ro_type_t igloo_ro__typedef__ ## type = \
{ \
    .control_length = sizeof(igloo_ro_type_t), \
    .control_version = igloo_RO__CONTROL_VERSION, \
    .type_length = sizeof(type), \
    .type_name = # type suffix \
    , ## __VA_ARGS__ \
}

#define igloo_RO_FORWARD_TYPE(type)                 extern const igloo_ro_type_t *igloo_ro__type__ ## type
#define igloo_RO_PUBLIC_TYPE(type, ...)             igloo_RO__DEFINE_TYPE(type, "", ## __VA_ARGS__); const igloo_ro_type_t * igloo_ro__type__ ## type = &igloo_ro__typedef__ ## type
#define igloo_RO_PRIVATE_TYPE(type, ...)            igloo_RO__DEFINE_TYPE(type, " (private)", ## __VA_ARGS__); static const igloo_ro_type_t * igloo_ro__type__ ## type = &igloo_ro__typedef__ ## type
#define igloo_RO_TYPEDECL_FREE(cb)                  .type_freecb = (cb)
#define igloo_RO_TYPEDECL_NEW(cb)                   .type_newcb = (cb)
#define igloo_RO_TYPEDECL_NEW_NOOP()                .type_newcb = igloo_ro_new__return_zero

#ifdef __cplusplus
}
#endif

#endif
