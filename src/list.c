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

#include <string.h>
#include <stdlib.h>

#include <igloo/list.h>

struct igloo_list_tag {
    igloo_ro_base_t __base;
    size_t offset;
    size_t fill;
    size_t length;
    igloo_ro_t *elements;
    const igloo_ro_type_t *type;
};

static void __free(igloo_ro_t self);

igloo_RO_PUBLIC_TYPE(igloo_list_t,
        igloo_RO_TYPEDECL_FREE(__free),
        igloo_RO_TYPEDECL_NEW_NOOP()
        );

static void __free(igloo_ro_t self)
{
    igloo_list_t *list = igloo_RO_TO_TYPE(self, igloo_list_t);
    size_t i;

    for (i = list->offset; i < list->fill; i++) {
        igloo_ro_unref(list->elements[i]);
    }
}

static inline void igloo_list_preallocate__realign(igloo_list_t *list)
{
    if (!list->offset)
        return;

    memmove(list->elements, list->elements + list->offset, (list->fill - list->offset)*sizeof(*list->elements));
}

void                    igloo_list_preallocate(igloo_list_t *list, size_t request)
{
    size_t new_len;

    if (!igloo_RO_IS_VALID(list, igloo_list_t))
        return;

    new_len = (list->fill - list->offset) + request;

    if (new_len > list->length || (new_len + 32) > list->length) {
        igloo_ro_t *n;

        igloo_list_preallocate__realign(list);

        n = realloc(list->elements, new_len*sizeof(*list->elements));
        if (!n)
            return;

        list->elements = n;
        list->length = new_len;
    }

    if (list->offset > 16)
        igloo_list_preallocate__realign(list);
}

int                     igloo_list_set_type__real(igloo_list_t *list, const igloo_ro_type_t *type)
{
    size_t i;

    if (!igloo_RO_IS_VALID(list, igloo_list_t))
        return -1;

    for (i = list->offset; i < list->fill; i++) {
        if (!igloo_RO_HAS_TYPE(list->elements[i], type)) {
            return -1;
        }
    }

    list->type = type;
    return 0;
}

int                     igloo_list_push(igloo_list_t *list, igloo_ro_t element)
{
    if (!igloo_RO_IS_VALID(list, igloo_list_t))
        return -1;

    if ((list->type && !igloo_RO_HAS_TYPE(element, list->type)) || igloo_RO_IS_NULL(element))
        return -1;

    igloo_list_preallocate(list, 1);

    if (list->fill == list->length)
        return -1;

    if (igloo_ro_ref(element) != 0)
        return -1;

    list->elements[list->fill++] = element;

    return 0;
}
int                     igloo_list_unshift(igloo_list_t *list, igloo_ro_t element)
{
    if (!igloo_RO_IS_VALID(list, igloo_list_t))
        return -1;

    if (list->offset == list->fill)
        return igloo_list_push(list, element);

    if ((list->type && !igloo_RO_HAS_TYPE(element, list->type)) || igloo_RO_IS_NULL(element))
        return -1;

    if (!list->offset) {
        igloo_list_preallocate(list, 1);

        if (list->fill == list->length)
            return -1;

        memmove(list->elements + 1, list->elements, sizeof(*list->elements)*list->fill);
        list->offset++;
        list->fill++;
    }

    if (!list->offset)
        return -1;

    if (igloo_ro_ref(element) != 0)
        return -1;

    list->elements[--list->offset] = element;

    return 0;
}

igloo_ro_t              igloo_list_shift(igloo_list_t *list)
{
    igloo_ro_t ret;

    if (!igloo_RO_IS_VALID(list, igloo_list_t))
        return igloo_RO_NULL;

    if (list->offset == list->fill)
        return igloo_RO_NULL;

    ret = list->elements[list->offset++];

    igloo_list_preallocate(list, 0);

    return ret;
}

igloo_ro_t              igloo_list_pop(igloo_list_t *list)
{
    igloo_ro_t ret;

    if (!igloo_RO_IS_VALID(list, igloo_list_t))
        return igloo_RO_NULL;

    if (list->offset == list->fill)
        return igloo_RO_NULL;

    ret = list->elements[--list->fill];

    igloo_list_preallocate(list, 0);

    return ret;
}

static inline int igloo_list_copy_elements(igloo_list_t *list, igloo_list_t *elements)
{
    size_t i;

    for (i = elements->offset; i < elements->fill; i++) {
        if (list->fill == list->length) {
            igloo_list_preallocate(list, 1);

            if (list->fill == list->length)
                return -1;
        }

        if (igloo_ro_ref(elements->elements[i]) != 0)
            return -1;

        list->elements[list->fill++] = elements->elements[i];
    }

    return 0;
}

int                     igloo_list_merge(igloo_list_t *list, igloo_list_t *elements)
{
    size_t old_fill;

    if (!igloo_RO_IS_VALID(list, igloo_list_t) || !igloo_RO_IS_VALID(elements, igloo_list_t))
        return -1;

    if (list->type) {
        size_t i;

        for (i = elements->offset; i < elements->fill; i++) {
            if (!igloo_RO_HAS_TYPE(elements->elements[i], list->type))
                return -1;
        }
    }

    igloo_list_preallocate(list, elements->fill - elements->offset);

    old_fill = list->fill;
    if (igloo_list_copy_elements(list, elements) != 0) {
        size_t i;

        for (i = old_fill; i < list->fill; i++) {
            igloo_ro_unref(list->elements[i]);
        }

        list->fill = old_fill;

        return -1;
    }

    return 0;
}

igloo_list_iterator_t * igloo_list_iterator_start(igloo_list_t *list, void *storage, size_t storage_length)
{
    igloo_list_iterator_t *iterator;

    if (!igloo_RO_IS_VALID(list, igloo_list_t))
        return NULL;

    if (!storage || storage_length != sizeof(igloo_list_iterator_t))
        return NULL;

    iterator = storage;
    memset(iterator, 0, sizeof(*iterator));

    if (igloo_ro_ref(list) != 0)
        return NULL;

    iterator->list = list;

    igloo_list_iterator_rewind(iterator);

    return iterator;
}

igloo_ro_t              igloo_list_iterator_next(igloo_list_iterator_t *iterator)
{
    size_t physical;

    physical = iterator->idx + iterator->list->offset;

    if (physical >= iterator->list->fill)
        return igloo_RO_NULL;

    if (igloo_ro_unref(iterator->list->elements[physical]) == 0)
        return igloo_RO_NULL;

    iterator->idx++;

    return iterator->list->elements[physical];
}
void                    igloo_list_iterator_end(igloo_list_iterator_t *iterator)
{
    if (!iterator)
        return;

    igloo_ro_unref(iterator->list);
}
int                     igloo_list_iterator_rewind(igloo_list_iterator_t *iterator)
{
    if (!iterator)
        return -1;

    iterator->idx = 0;

    return 0;
}
