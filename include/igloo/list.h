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

/* To create lists use: igloo_ro_new(igloo_list_t)
 */

/* Clear a list (remove all elements). */
int                     igloo_list_clear(igloo_list_t *list);
/* Preallocate space for later mass-adding of elements. */
void                    igloo_list_preallocate(igloo_list_t *list, size_t request);
/* Limit elements to those of the given type. */
int                     igloo_list_set_type__real(igloo_list_t *list, const igloo_ro_type_t *type);
#define                 igloo_list_set_type(list,type) igloo_list_set_type__real((list),(igloo_ro__type__ ## type))
/* Add an element at the end of the list. */
int                     igloo_list_push(igloo_list_t *list, igloo_ro_t element);
/* Add an element at the begin of a list. */
int                     igloo_list_unshift(igloo_list_t *list, igloo_ro_t element);
/* Get and remove the first element from the list. */
igloo_ro_t              igloo_list_shift(igloo_list_t *list);
/* Get and remove the last element from the list. */
igloo_ro_t              igloo_list_pop(igloo_list_t *list);

/* Merge the content of the list elements into the list list. The list elements is not changed. */
int                     igloo_list_merge(igloo_list_t *list, igloo_list_t *elements);

/* Creates a new iterator that can be used to walk the list.
 * The memory pointed to by storage of size storage_length is used to store the iterator's internal
 * values. It must be allocated (e.g. on stack) untill igloo_list_iterator_end() is called.
 * igloo_list_iterator_storage_t is provided to be used as storage object.
 * Example:
 *  igloo_list_iterator_storage_t storage;
 *  igloo_list_iterator_t *iterator = igloo_list_iterator_start(list, &storage, sizeof(storage));
 */
igloo_list_iterator_t * igloo_list_iterator_start(igloo_list_t *list, void *storage, size_t storage_length);
/* Get next element from iterator. */
igloo_ro_t              igloo_list_iterator_next(igloo_list_iterator_t *iterator);
/* Destory iterator. */
void                    igloo_list_iterator_end(igloo_list_iterator_t *iterator);
/* Rewind iterator. The next call to igloo_list_iterator_next() will return the first element again. */
int                     igloo_list_iterator_rewind(igloo_list_iterator_t *iterator);

/* Go thru all elements in the list.
 * Parameters:
 *  list: the list to use.
 *  type: the type of elements in the list
 *  var: the variable to store the current element in.
 *  code: the code block to be used in the loop.
 *
 * Note: This can only be used on lists that contain only one type of objects.
 *       See also: igloo_list_set_type()
 */
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
