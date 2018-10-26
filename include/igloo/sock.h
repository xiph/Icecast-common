/* sock.h
 * - General Socket Function Headers
 *
 * Copyright (C) 2014       Michael Smith <msmith@icecast.org>,
 *                          Brendan Cully <brendan@xiph.org>,
 *                          Karl Heyes <karl@xiph.org>,
 *                          Jack Moffitt <jack@icecast.org>,
 *                          Ed "oddsock" Zaleski <oddsock@xiph.org>,
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

#ifndef _LIBIGLOO__SOCK_H_
#define _LIBIGLOO__SOCK_H_

#include <stdarg.h>

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#elif _WIN32
#include <compat.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#else
#ifndef _SYS_UIO_H
struct iovec
{
    void   *iov_base;
    size_t iov_len;
};
#endif
#endif

#if !defined(HAVE_INET_ATON) && defined(HAVE_INET_PTON)
#define inet_aton(a,b) inet_pton(AF_INET, (a), (b))
#endif

#ifdef INET6_ADDRSTRLEN
#define MAX_ADDR_LEN INET6_ADDRSTRLEN
#else
#define MAX_ADDR_LEN 46
#endif

#ifndef igloo_sock_t
#define igloo_sock_t int
#endif

/* The following values are based on unix avoiding errno value clashes */
#define igloo_SOCK_SUCCESS 0
#define igloo_SOCK_ERROR (igloo_sock_t)-1
#define igloo_SOCK_TIMEOUT -2

/* sock connect macro */
#define igloo_sock_connect(h, p) igloo_sock_connect_wto(h, p, 0)

/* Misc socket functions */
void igloo_sock_initialize(void);
void igloo_sock_shutdown(void);
char *igloo_sock_get_localip(char *buff, int len);
int igloo_sock_error(void);
int igloo_sock_recoverable(int error);
int igloo_sock_stalled(int error);
int igloo_sock_valid_socket(igloo_sock_t sock);
int igloo_sock_active (igloo_sock_t sock);
int igloo_sock_set_blocking(igloo_sock_t sock, int block);
int igloo_sock_set_nolinger(igloo_sock_t sock);
int igloo_sock_set_keepalive(igloo_sock_t sock);
int igloo_sock_set_nodelay(igloo_sock_t sock);
void igloo_sock_set_send_buffer (igloo_sock_t sock, int win_size);
void igloo_sock_set_error(int val);
int igloo_sock_close(igloo_sock_t  sock);

/* Connection related socket functions */
igloo_sock_t igloo_sock_connect_wto(const char *hostname, int port, int timeout);
igloo_sock_t igloo_sock_connect_wto_bind(const char *hostname, int port, const char *bnd, int timeout);
igloo_sock_t igloo_sock_connect_non_blocking(const char *host, unsigned port);
int igloo_sock_connected(igloo_sock_t sock, int timeout);

/* Socket write functions */
int igloo_sock_write_bytes(igloo_sock_t sock, const void *buff, size_t len);
int igloo_sock_write(igloo_sock_t sock, const char *fmt, ...);
int igloo_sock_write_fmt(igloo_sock_t sock, const char *fmt, va_list ap);
int igloo_sock_write_string(igloo_sock_t sock, const char *buff);
ssize_t igloo_sock_writev (igloo_sock_t sock, const struct iovec *iov, size_t count);


/* Socket read functions */
int igloo_sock_read_bytes(igloo_sock_t sock, char *buff, size_t len);
int igloo_sock_read_line(igloo_sock_t sock, char *string, const int len);

/* server socket functions */
igloo_sock_t igloo_sock_get_server_socket(int port, const char *sinterface);
int igloo_sock_listen(igloo_sock_t serversock, int backlog);
igloo_sock_t igloo_sock_accept(igloo_sock_t serversock, char *ip, size_t len);

#ifdef _WIN32
int inet_aton(const char *s, struct in_addr *a);
#endif

#endif  /* __SOCK_H */
