/* 
** Logging framework.
**
** Copyright (C) 2014       Michael Smith <msmith@icecast.org>,
**                          Ralph Giles <giles@xiph.org>,
**                          Ed "oddsock" Zaleski <oddsock@xiph.org>,
**                          Karl Heyes <karl@xiph.org>,
**                          Jack Moffitt <jack@icecast.org>,
**                          Thomas Ruecker <thomas@ruecker.fi>,
** Copyright (C) 2012-2018  Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Library General Public
** License as published by the Free Software Foundation; either
** version 2 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.
**
** You should have received a copy of the GNU Library General Public
** License along with this library; if not, write to the
** Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
** Boston, MA  02110-1301, USA.
**
*/

#ifndef _LIBIGLOO__LOG_H_
#define _LIBIGLOO__LOG_H_

#include <stdio.h>

#define LOG_EINSANE -1
#define LOG_ENOMORELOGS -2
#define LOG_ECANTOPEN -3
#define LOG_ENOTOPEN -4
#define LOG_ENOTIMPL -5

#ifdef _WIN32
#define IO_BUFFER_TYPE _IONBF
#else
#define IO_BUFFER_TYPE _IOLBF
#endif

void igloo_log_initialize(void);
int igloo_log_open_file(FILE *file);
int igloo_log_open(const char *filename);
int igloo_log_open_with_buffer(const char *filename, int size);
void igloo_log_set_level(int log_id, unsigned level);
void igloo_log_set_trigger(int id, unsigned trigger);
int  igloo_log_set_filename(int id, const char *filename);
void igloo_log_set_lines_kept (int log_id, unsigned int count);
void igloo_log_contents (int log_id, char **_contents, unsigned int *_len);
int igloo_log_set_archive_timestamp(int id, int value);
void igloo_log_flush(int log_id);
void igloo_log_reopen(int log_id);
void igloo_log_close(int log_id);
void igloo_log_shutdown(void);

void igloo_log_write(int log_id, unsigned priority, const char *cat, const char *func, 
        const char *fmt, ...);
void igloo_log_write_direct(int log_id, const char *fmt, ...);

#endif  /* __LOG_H__ */
