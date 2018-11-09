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

#ifndef _LIBIGLOO__PRIVATE_H_
#define _LIBIGLOO__PRIVATE_H_

/* init/shutdown of the library */
void igloo_thread_initialize(void);
void igloo_thread_initialize_with_log_id(int log_id);
void igloo_thread_shutdown(void);
void igloo_sock_initialize(void);
void igloo_sock_shutdown(void);
void igloo_resolver_initialize(void);
void igloo_resolver_shutdown(void);
void igloo_log_initialize(void);
void igloo_log_shutdown(void);


#endif
