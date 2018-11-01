/* Copyright (C) 2018   Marvin Scholz <epirat07@gmail.com>
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

#ifndef _LIBIGLOO__IGLOO_H_
#define _LIBIGLOO__IGLOO_H_
/**
 * @file
 * Put a good description of this file here
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Put stuff here */

#include "ro.h"

/*
 * This initializes libigloo. This MUST BE called before any
 * other functions can be called.
 *
 * Returns a refobject on success or igloo_RO_NULL on failure.
 * This can be called multiple times (e.g. by the application
 * and by the libraries it uses independently).
 *
 * The library is deinitialized when the last reference
 * to a returned object is gone. This happens by
 * calling igloo_ro_unref() on the last reference.
 *
 * All igloo_ro_*() functions can be used on this object.
 */
igloo_ro_t     igloo_initialize(void);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__IGLOO_H_ */
