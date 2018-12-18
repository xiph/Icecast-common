/* Copyright (C) 2018       Marvin Scholz <epirat07@gmail.com>
 * Copyright (C) 2012       Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
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

#include <stdlib.h>
#include <string.h>

#include <igloo/ro.h>

/* This is not static as it is used by igloo_RO_TYPEDECL_NEW_NOOP() */
int igloo_ro_new__return_zero(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap)
{
    (void)self, (void)type, (void)ap;
    return 0;
}

igloo_RO_PUBLIC_TYPE(igloo_ro_base_t,
        igloo_RO_TYPEDECL_NEW_NOOP()
        );

static inline int check_type(const igloo_ro_type_t *type)
{
    return type->control_length == sizeof(igloo_ro_type_t) && type->control_version == igloo_RO__CONTROL_VERSION &&
           type->type_length >= sizeof(igloo_ro_base_t);
}

igloo_ro_t      igloo_ro_new__raw(const igloo_ro_type_t *type, const char *name, igloo_ro_t associated)
{
    igloo_ro_base_t *base;

    if (!check_type(type))
        return igloo_RO_NULL;

    base = calloc(1, type->type_length);
    if (!base)
        return igloo_RO_NULL;

    base->type = type;
    base->refc = 1;

    igloo_thread_mutex_create(&(base->lock));

    if (name) {
        base->name = strdup(name);
        if (!base->name) {
            igloo_ro_unref(base);
            return igloo_RO_NULL;
        }
    }

    if (!igloo_RO_IS_NULL(associated)) {
        if (igloo_ro_ref(associated) != 0) {
            igloo_ro_unref(base);
            return igloo_RO_NULL;
        }

        base->associated = associated;
    }

    return (igloo_ro_t)base;
}

igloo_ro_t      igloo_ro_new__simple(const igloo_ro_type_t *type, const char *name, igloo_ro_t associated, ...)
{
    igloo_ro_t ret;
    int res;
    va_list ap;

    if (!check_type(type))
        return igloo_RO_NULL;

    if (!type->type_newcb)
        return igloo_RO_NULL;

    ret = igloo_ro_new__raw(type, name, associated);
    if (igloo_RO_IS_NULL(ret))
        return igloo_RO_NULL;

    va_start(ap, associated);
    res = type->type_newcb(ret, type, ap);
    va_end(ap);

    if (res != 0) {
        igloo_ro_unref(ret);
        return igloo_RO_NULL;
    }

    return ret;
}

int             igloo_ro_ref(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);

    if (!base)
        return -1;

    igloo_thread_mutex_lock(&(base->lock));
    if (!base->refc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return -1;
    }
    base->refc++;
    igloo_thread_mutex_unlock(&(base->lock));

    return 0;
}

static inline void igloo_ro__destory(igloo_ro_base_t *base)
{
    igloo_thread_mutex_unlock(&(base->lock));
    igloo_thread_mutex_destroy(&(base->lock));

    free(base);
}

int             igloo_ro_unref(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);

    if (!base)
        return -1;

    igloo_thread_mutex_lock(&(base->lock));

    if (!base->refc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return -1;
    }

    base->refc--;

    if (base->refc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return 0;
    }

    if (base->type->type_freecb)
        base->type->type_freecb(self);

    igloo_ro_unref(base->associated);

    if (base->name)
        free(base->name);

    if (base->wrefc) {
        /* only clear the object */
        base->associated = igloo_RO_NULL;
        base->name = NULL;
    } else {
        igloo_ro__destory(base);
    }

    return 0;
}

int             igloo_ro_weak_ref(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);

    if (!base)
        return -1;

    igloo_thread_mutex_lock(&(base->lock));
    base->wrefc++;
    igloo_thread_mutex_unlock(&(base->lock));

    return 0;
}

int             igloo_ro_weak_unref(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);

    if (!base)
        return -1;

    igloo_thread_mutex_lock(&(base->lock));
    base->wrefc--;

    if (base->refc || base->wrefc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return 0;
    }

    igloo_ro__destory(base);

    return 0;
}

const char *    igloo_ro_get_name(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    const char *ret;

    if (!base)
        return NULL;

    igloo_thread_mutex_lock(&(base->lock));
    if (!base->refc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return NULL;
    }
    ret = base->name;
    igloo_thread_mutex_unlock(&(base->lock));

    return ret;
}

igloo_ro_t      igloo_ro_get_associated(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    igloo_ro_t ret;

    if (!base)
        return igloo_RO_NULL;

    igloo_thread_mutex_lock(&(base->lock));
    if (!base->refc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return igloo_RO_NULL;
    }
    ret = base->associated;
    if (!igloo_RO_IS_NULL(ret)) {
        if (igloo_ro_ref(ret) != 0) {
            igloo_thread_mutex_unlock(&(base->lock));
            return igloo_RO_NULL;
        }
    }
    igloo_thread_mutex_unlock(&(base->lock));

    return ret;
}

int             igloo_ro_set_associated(igloo_ro_t self, igloo_ro_t associated)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    igloo_ro_t old;

    if (!base)
        return 0;

    /* We can not set ourself to be our associated. */
    if (base == igloo_RO__GETBASE(associated))
        return -1;

    if (!igloo_RO_IS_NULL(associated)) {
        if (igloo_ro_ref(associated) != 0) {
            /* Could not get a reference on the new associated object. */
            return -1;
        }
    }

    igloo_thread_mutex_lock(&(base->lock));
    if (!base->refc) {
        igloo_ro_unref(associated);
        igloo_thread_mutex_unlock(&(base->lock));
        return -1;
    }
    old = base->associated;
    base->associated = associated;
    igloo_thread_mutex_unlock(&(base->lock));

    igloo_ro_unref(old);

    return 0;
}
