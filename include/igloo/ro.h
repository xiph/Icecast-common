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

#ifndef _LIBIGLOO__RO_H_
#define _LIBIGLOO__RO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

#include "types.h"
#include "thread.h"

/* Type used for callback called then the object is actually freed
 * That is once all references to it are gone.
 *
 * This function must not try to deallocate or alter self.
 */
typedef void (*igloo_ro_free_t)(igloo_ro_t self);

/* Type used for callback called then the object is created
 * using the generic igloo_ro_new().
 *
 * Additional parameters passed to igloo_ro_new() are passed
 * in the list ap. All limitations of <stdarg.h> apply.
 *
 * This function must return zero in case of success and
 * non-zero in case of error. In case of error igloo_ro_unref()
 * is called internally to clear the object.
 */
typedef int (*igloo_ro_new_t)(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap);

/* Meta type used to defined types.
 * DO NOT use any of the members in here directly!
 */

/* ---[ PRIVATE ]--- */
/*
 * Those types are defined here as they must be known to the compiler.
 * Nobody should ever try to access them directly.
 */
struct igloo_ro_type_tag {
    /* Size of this control structure */
    size_t              control_length;
    /* ABI version of this structure */
    int                 control_version;

    /* Total length of the objects to be created */
    size_t              type_length;
    /* Name of type */
    const char *        type_name;
    /* Callback to be called on final free() */
    igloo_ro_free_t    	type_freecb;
    /* Callback to be callback by igloo_ro_new() */
    igloo_ro_new_t     	type_newcb;
};
struct igloo_ro_base_tag {
	/* Type of the object */
    const igloo_ro_type_t * type;
	/* Reference counter */
    size_t refc;
	/* Mutex for igloo_ro_*(). */
    igloo_mutex_t lock;
	/* Name of the object. */
    char * name;
	/* Associated objects */
    igloo_ro_t associated;
};
int igloo_ro_new__return_zero(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap);
/* ---[ END PRIVATE ]--- */

#ifdef igloo_HAVE_TYPE_ATTRIBUTE_TRANSPARENT_UNION
#define igloo_RO__GETBASE(x)		(((igloo_ro_t)(x)).subtype__igloo_ro_base_t)
#define igloo_RO_NULL				((igloo_ro_t)(igloo_ro_base_t*)NULL)
#define igloo_RO_IS_NULL(x)			(igloo_RO__GETBASE((x)) == NULL)
#define	igloo_RO_TO_TYPE(x,type)    (igloo_RO_IS_VALID((x),type)  ? NULL : ((igloo_ro_t)(x)).subtype__ ## type)
#else
#define igloo_RO__GETBASE(x)		((igloo_ro_base_t*)(x))
#define igloo_RO_NULL				NULL
#define igloo_RO_IS_NULL(x)			((x) == NULL)
#define igloo_RO_TO_TYPE(x,type)	((type*)(x))
#endif

#define igloo_RO_GET_TYPE(x)		(igloo_RO__GETBASE((x)) == NULL ? NULL : igloo_RO__GETBASE((x))->type)
#define igloo_RO_GET_TYPENAME(x)	(igloo_RO_GET_TYPE((x)) == NULL ? NULL : igloo_RO_GET_TYPE((x))->type_name)
#define igloo_RO_IS_VALID(x,type)	(!igloo_RO_IS_NULL((x)) && igloo_RO_GET_TYPE((x)) == (igloo_ro__type__ ## type))
#define igloo_RO_HAS_TYPE(x,type)	(!igloo_RO_IS_NULL((x)) && igloo_RO_GET_TYPE((x)) == (type))

/* Create a new refobject
 * The type argument gives the type for the new object,
 * the name for the object is given by name, and
 * the associated refobject is given by associated.
 */

igloo_ro_t		igloo_ro_new__raw(const igloo_ro_type_t *type, const char *name, igloo_ro_t associated);
#define			igloo_ro_new_raw(type, name, associated)  igloo_RO_TO_TYPE(igloo_ro_new__raw((igloo_ro__type__ ## type), (name), (associated)), type)

igloo_ro_t		igloo_ro_new__simple(const igloo_ro_type_t *type, const char *name, igloo_ro_t associated, ...);
#define			igloo_ro_new(type, ...)  igloo_RO_TO_TYPE(igloo_ro_new__simple((igloo_ro__type__ ## type), NULL, igloo_RO_NULL, ## __VA_ARGS__), type)
#define			igloo_ro_new_ext(type, name, associated, ...)  igloo_RO_TO_TYPE(igloo_ro_new__simple((igloo_ro__type__ ## type), (name), (associated), ## __VA_ARGS__), type)

/* This increases the reference counter of the object */
int             igloo_ro_ref(igloo_ro_t self);
/* This decreases the reference counter of the object.
 * If the object's reference counter reaches zero the object is freed.
 */
int             igloo_ro_unref(igloo_ro_t self);

/* This gets the object's name */
const char *    igloo_ro_get_name(igloo_ro_t self);

/* This gets the object's associated object. */
igloo_ro_t     	igloo_ro_get_associated(igloo_ro_t self);
int				igloo_ro_set_associated(igloo_ro_t self, igloo_ro_t associated);

#ifdef __cplusplus
}
#endif

#endif
