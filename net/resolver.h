/*
 * resolver.h
 *
 * name resolver library header
 *
 * Copyright (C) 2014       Brendan Cully <brendan@xiph.org>,
 *                          Jack Moffitt <jack@icecast.org>,
 * Copyright (C) 2012-2018  Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
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
 *
 */

#ifndef _LIBIGLOO__RESOLVER_H_
#define _LIBIGLOO__RESOLVER_H_


/*
** resolver_lookup
**
** resolves a hosts name from it's ip address
** or
** resolves an ip address from it's host name
**
** returns a pointer to buff, or NULL if an error occured
**
*/

void igloo_resolver_initialize(void);
void igloo_resolver_shutdown(void);

char *igloo_resolver_getname(const char *ip, char *buff, int len);
char *igloo_resolver_getip(const char *name, char *buff, int len);

#endif





