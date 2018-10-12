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

#include <avl/avl.h>

#define HTTPP_VAR_PROTOCOL "__protocol"
#define HTTPP_VAR_VERSION "__version"
#define HTTPP_VAR_URI "__uri"
#define HTTPP_VAR_RAWURI "__rawuri"
#define HTTPP_VAR_QUERYARGS "__queryargs"
#define HTTPP_VAR_REQ_TYPE "__req_type"
#define HTTPP_VAR_ERROR_MESSAGE "__errormessage"
#define HTTPP_VAR_ERROR_CODE "__errorcode"
#define HTTPP_VAR_ICYPASSWORD "__icy_password"

typedef enum {
    HTTPP_NS_VAR,
    HTTPP_NS_HEADER,
    HTTPP_NS_QUERY_STRING,
    HTTPP_NS_POST_BODY
} httpp_ns_t;

typedef enum httpp_request_type_tag {
    /* Initial and internally used state of the engine */
    httpp_req_none = 0,
    /* Part of HTTP standard: GET, POST, PUT and HEAD */
    httpp_req_get,
    httpp_req_post,
    httpp_req_put,
    httpp_req_head,
    httpp_req_options,
    httpp_req_delete,
    httpp_req_trace,
    httpp_req_connect,
    /* Icecast SOURCE, to be replaced with PUT some day */
    httpp_req_source,
    /* XXX: ??? */
    httpp_req_play,
    /* Icecast 2.x STATS, to request a live stream of stats events */
    httpp_req_stats,
    /* Used if request method is unknown. MUST BE LAST ONE IN LIST. */
    httpp_req_unknown
} httpp_request_type_e;

typedef unsigned int httpp_request_info_t;
#define HTTPP_REQUEST_IS_SAFE                       ((httpp_request_info_t)0x0001U)
#define HTTPP_REQUEST_IS_IDEMPOTENT                 ((httpp_request_info_t)0x0002U)
#define HTTPP_REQUEST_IS_CACHEABLE                  ((httpp_request_info_t)0x0004U)
#define HTTPP_REQUEST_HAS_RESPONSE_BODY             ((httpp_request_info_t)0x0010U)
#define HTTPP_REQUEST_HAS_REQUEST_BODY              ((httpp_request_info_t)0x0100U)
#define HTTPP_REQUEST_HAS_OPTIONAL_REQUEST_BODY     ((httpp_request_info_t)0x0200U)

typedef struct http_var_tag http_var_t;
struct http_var_tag {
    char *name;
    size_t values;
    char **value;
};

typedef struct http_varlist_tag {
    http_var_t var;
    struct http_varlist_tag *next;
} http_varlist_t;

typedef struct http_parser_tag {
    size_t refc;
    httpp_request_type_e req_type;
    char *uri;
    avl_tree *vars;
    avl_tree *queryvars;
    avl_tree *postvars;
} http_parser_t;

httpp_request_info_t igloo_httpp_request_info(httpp_request_type_e req);

http_parser_t *igloo_httpp_create_parser(void);
void igloo_httpp_initialize(http_parser_t *parser, http_varlist_t *defaults);
int igloo_httpp_parse(http_parser_t *parser, const char *http_data, unsigned long len);
int httpp_parse_icy(http_parser_t *parser, const char *http_data, unsigned long len);
int igloo_httpp_parse_response(http_parser_t *parser, const char *http_data, unsigned long len, const char *uri);
int igloo_httpp_parse_postdata(http_parser_t *parser, const char *body_data, size_t len);
void igloo_httpp_setvar(http_parser_t *parser, const char *name, const char *value);
void igloo_httpp_deletevar(http_parser_t *parser, const char *name);
const char *igloo_httpp_getvar(http_parser_t *parser, const char *name);
void igloo_httpp_set_query_param(http_parser_t *parser, const char *name, const char *value);
const char *igloo_httpp_get_query_param(http_parser_t *parser, const char *name);
void igloo_httpp_set_post_param(http_parser_t *parser, const char *name, const char *value);
const char *igloo_httpp_get_post_param(http_parser_t *parser, const char *name);
const char *igloo_httpp_get_param(http_parser_t *parser, const char *name);
const http_var_t *igloo_httpp_get_param_var(http_parser_t *parser, const char *name);
const http_var_t *igloo_httpp_get_any_var(http_parser_t *parser, httpp_ns_t ns, const char *name);
char ** igloo_httpp_get_any_key(http_parser_t *parser, httpp_ns_t ns);
void igloo_httpp_free_any_key(char **keys);
int igloo_httpp_addref(http_parser_t *parser);
int igloo_httpp_release(http_parser_t *parser);
#define httpp_destroy(x) igloo_httpp_release((x))

/* util functions */
httpp_request_type_e igloo_httpp_str_to_method(const char * method);
 
#endif
