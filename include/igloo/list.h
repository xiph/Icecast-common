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

#ifndef _LIBIGLOO__LIST_H_
#define _LIBIGLOO__LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <igloo/ro.h>

/* About thread safety:
 * This set of functions is intentinally not thread safe.
 */

typedef struct igloo_list_iterator_tag igloo_list_iterator_t;
typedef struct igloo_list_iterator_tag igloo_list_iterator_storage_t;

igloo_RO_FORWARD_TYPE(igloo_list_t);

/* ---[ PRIVATE ]--- */
/*
 * Those types are defined here as they must be known to the compiler.
 * Nobody should ever try to access them directly.
 */
struct igloo_list_iterator_tag {
    igloo_list_t *list;
    size_t idx;
};
/* ---[ END PRIVATE ]--- */

int                     igloo_list_clear(igloo_list_t *list);
void                    igloo_list_preallocate(igloo_list_t *list, size_t request);
int                     igloo_list_set_type__real(igloo_list_t *list, const igloo_ro_type_t *type);
#define                 igloo_list_set_type(list,type) igloo_list_set_type__real((list),(igloo_ro__type__ ## type))
int                     igloo_list_push(igloo_list_t *list, igloo_ro_t element);
int                     igloo_list_unshift(igloo_list_t *list, igloo_ro_t element);
igloo_ro_t              igloo_list_shift(igloo_list_t *list);
igloo_ro_t              igloo_list_pop(igloo_list_t *list);

int                     igloo_list_merge(igloo_list_t *list, igloo_list_t *elements);

igloo_list_iterator_t * igloo_list_iterator_start(igloo_list_t *list, void *storage, size_t storage_length);
igloo_ro_t              igloo_list_iterator_next(igloo_list_iterator_t *iterator);
void                    igloo_list_iterator_end(igloo_list_iterator_t *iterator);
int                     igloo_list_iterator_rewind(igloo_list_iterator_t *iterator);

#define                 igloo_list_foreach(list,type,var,code) \
do { \
    igloo_list_iterator_storage_t __igloo_list_iterator_storage; \
    igloo_list_iterator_t *__igloo_list_iterator = igloo_list_iterator_start((list), &__igloo_list_iterator_storage, sizeof(__igloo_list_iterator_storage)); \
    type * var; \
    for (; !igloo_RO_IS_NULL((var) = igloo_RO_TO_TYPE(igloo_list_iterator_next(__igloo_list_iterator),type));) { \
        code; \
        igloo_ro_unref((var)); \
    } \
    igloo_list_iterator_end(__igloo_list_iterator); \
} while (0);

#ifdef __cplusplus
}
#endif

#endif
