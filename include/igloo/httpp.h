/* httpp.h
**
** http parsing library
**
** Copyright (C) 2014       Michael Smith <msmith@icecast.org>,
**                          Ralph Giles <giles@xiph.org>,
**                          Karl Heyes <karl@xiph.org>,
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

#ifndef _LIBIGLOO__HTTPP_H_
#define _LIBIGLOO__HTTPP_H_

#include "avl.h"

#define igloo_HTTPP_VAR_PROTOCOL "__protocol"
#define igloo_HTTPP_VAR_VERSION "__version"
#define igloo_HTTPP_VAR_URI "__uri"
#define igloo_HTTPP_VAR_RAWURI "__rawuri"
#define igloo_HTTPP_VAR_QUERYARGS "__queryargs"
#define igloo_HTTPP_VAR_REQ_TYPE "__req_type"
#define igloo_HTTPP_VAR_ERROR_MESSAGE "__errormessage"
#define igloo_HTTPP_VAR_ERROR_CODE "__errorcode"
#define igloo_HTTPP_VAR_ICYPASSWORD "__icy_password"

typedef enum {
    igloo_HTTPP_NS_VAR,
    igloo_HTTPP_NS_HEADER,
    igloo_HTTPP_NS_QUERY_STRING,
    igloo_HTTPP_NS_POST_BODY
} igloo_httpp_ns_t;

typedef enum httpp_request_type_tag {
    /* Initial and internally used state of the engine */
    igloo_httpp_req_none = 0,
    /* Part of HTTP standard: GET, POST, PUT and HEAD */
    igloo_httpp_req_get,
    igloo_httpp_req_post,
    igloo_httpp_req_put,
    igloo_httpp_req_head,
    igloo_httpp_req_options,
    igloo_httpp_req_delete,
    igloo_httpp_req_trace,
    igloo_httpp_req_connect,
    /* Icecast SOURCE, to be replaced with PUT some day */
    igloo_httpp_req_source,
    /* XXX: ??? */
    igloo_httpp_req_play,
    /* Icecast 2.x STATS, to request a live stream of stats events */
    igloo_httpp_req_stats,
    /* Used if request method is unknown. MUST BE LAST ONE IN LIST. */
    igloo_httpp_req_unknown
} igloo_httpp_request_type_e;

typedef unsigned int igloo_httpp_request_info_t;
#define igloo_HTTPP_REQUEST_IS_SAFE                       ((igloo_httpp_request_info_t)0x0001U)
#define igloo_HTTPP_REQUEST_IS_IDEMPOTENT                 ((igloo_httpp_request_info_t)0x0002U)
#define igloo_HTTPP_REQUEST_IS_CACHEABLE                  ((igloo_httpp_request_info_t)0x0004U)
#define igloo_HTTPP_REQUEST_HAS_RESPONSE_BODY             ((igloo_httpp_request_info_t)0x0010U)
#define igloo_HTTPP_REQUEST_HAS_REQUEST_BODY              ((igloo_httpp_request_info_t)0x0100U)
#define igloo_HTTPP_REQUEST_HAS_OPTIONAL_REQUEST_BODY     ((igloo_httpp_request_info_t)0x0200U)

typedef struct igloo_http_var_tag igloo_http_var_t;
struct igloo_http_var_tag {
    char *name;
    size_t values;
    char **value;
};

typedef struct igloo_http_varlist_tag {
    igloo_http_var_t var;
    struct igloo_http_varlist_tag *next;
} igloo_http_varlist_t;

typedef struct igloo_http_parser_tag {
    size_t refc;
    igloo_httpp_request_type_e req_type;
    char *uri;
    igloo_avl_tree *vars;
    igloo_avl_tree *queryvars;
    igloo_avl_tree *postvars;
} igloo_http_parser_t;

igloo_httpp_request_info_t igloo_httpp_request_info(igloo_httpp_request_type_e req);

igloo_http_parser_t *igloo_httpp_create_parser(void);
void igloo_httpp_initialize(igloo_http_parser_t *parser, igloo_http_varlist_t *defaults);
int igloo_httpp_parse(igloo_http_parser_t *parser, const char *http_data, unsigned long len);
int httpp_parse_icy(igloo_http_parser_t *parser, const char *http_data, unsigned long len);
int igloo_httpp_parse_response(igloo_http_parser_t *parser, const char *http_data, unsigned long len, const char *uri);
int igloo_httpp_parse_postdata(igloo_http_parser_t *parser, const char *body_data, size_t len);
void igloo_httpp_setvar(igloo_http_parser_t *parser, const char *name, const char *value);
void igloo_httpp_deletevar(igloo_http_parser_t *parser, const char *name);
const char *igloo_httpp_getvar(igloo_http_parser_t *parser, const char *name);
void igloo_httpp_set_query_param(igloo_http_parser_t *parser, const char *name, const char *value);
const char *igloo_httpp_get_query_param(igloo_http_parser_t *parser, const char *name);
void igloo_httpp_set_post_param(igloo_http_parser_t *parser, const char *name, const char *value);
const char *igloo_httpp_get_post_param(igloo_http_parser_t *parser, const char *name);
const char *igloo_httpp_get_param(igloo_http_parser_t *parser, const char *name);
const igloo_http_var_t *igloo_httpp_get_param_var(igloo_http_parser_t *parser, const char *name);
const igloo_http_var_t *igloo_httpp_get_any_var(igloo_http_parser_t *parser, igloo_httpp_ns_t ns, const char *name);
char ** igloo_httpp_get_any_key(igloo_http_parser_t *parser, igloo_httpp_ns_t ns);
void igloo_httpp_free_any_key(char **keys);
int igloo_httpp_addref(igloo_http_parser_t *parser);
int igloo_httpp_release(igloo_http_parser_t *parser);
#define httpp_destroy(x) igloo_httpp_release((x))

/* util functions */
igloo_httpp_request_type_e igloo_httpp_str_to_method(const char * method);
 
#endif
