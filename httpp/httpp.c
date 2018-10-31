/* Httpp.c
**
** http parsing engine
**
** Copyright (C) 2014       Michael Smith <msmith@icecast.org>,
**                          Ralph Giles <giles@xiph.org>,
**                          Ed "oddsock" Zaleski <oddsock@xiph.org>,
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

#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <igloo/avl.h>
#include <igloo/httpp.h>

#define MAX_HEADERS 32

/* internal functions */

/* misc */
static char *_lowercase(char *str);

/* for avl trees */
static int igloo__compare_vars(void *compare_arg, void *a, void *b);
static int igloo__free_vars(void *key);

/* For avl tree manipulation */
static void parse_query(igloo_avl_tree *tree, const char *query, size_t len);
static const char *_httpp_get_param(igloo_avl_tree *tree, const char *name);
static void _httpp_set_param_nocopy(igloo_avl_tree *tree, char *name, char *value, int replace);
static void _httpp_set_param(igloo_avl_tree *tree, const char *name, const char *value);
static igloo_http_var_t *igloo__httpp_get_param_var(igloo_avl_tree *tree, const char *name);

static const struct http_method {
    const char *name;
    const igloo_httpp_request_type_e req;
    const igloo_httpp_request_info_t info;
} igloo_httpp__methods[] = {
    /* RFC 7231 */
    {"GET",                 igloo_httpp_req_get,
        igloo_HTTPP_REQUEST_IS_SAFE|igloo_HTTPP_REQUEST_IS_IDEMPOTENT|igloo_HTTPP_REQUEST_IS_CACHEABLE|igloo_HTTPP_REQUEST_HAS_RESPONSE_BODY|igloo_HTTPP_REQUEST_HAS_OPTIONAL_REQUEST_BODY
    },
    {"HEAD",                igloo_httpp_req_head,
        igloo_HTTPP_REQUEST_IS_SAFE|igloo_HTTPP_REQUEST_IS_IDEMPOTENT|igloo_HTTPP_REQUEST_IS_CACHEABLE
    },
    {"POST",                igloo_httpp_req_post,
        igloo_HTTPP_REQUEST_IS_CACHEABLE|igloo_HTTPP_REQUEST_HAS_RESPONSE_BODY|igloo_HTTPP_REQUEST_HAS_REQUEST_BODY
    },
    {"PUT",                 igloo_httpp_req_put,
        igloo_HTTPP_REQUEST_IS_IDEMPOTENT|igloo_HTTPP_REQUEST_HAS_RESPONSE_BODY|igloo_HTTPP_REQUEST_HAS_REQUEST_BODY
    },
    {"DELETE",              igloo_httpp_req_delete,
        igloo_HTTPP_REQUEST_IS_IDEMPOTENT|igloo_HTTPP_REQUEST_HAS_RESPONSE_BODY
    },
    {"CONNECT",             igloo_httpp_req_connect,
        igloo_HTTPP_REQUEST_HAS_RESPONSE_BODY|igloo_HTTPP_REQUEST_HAS_REQUEST_BODY
    },
    {"OPTIONS",             igloo_httpp_req_options,
        igloo_HTTPP_REQUEST_IS_SAFE|igloo_HTTPP_REQUEST_IS_IDEMPOTENT|igloo_HTTPP_REQUEST_HAS_RESPONSE_BODY|igloo_HTTPP_REQUEST_HAS_OPTIONAL_REQUEST_BODY
    },
    {"TRACE",               igloo_httpp_req_trace,
        igloo_HTTPP_REQUEST_IS_SAFE|igloo_HTTPP_REQUEST_IS_IDEMPOTENT|igloo_HTTPP_REQUEST_HAS_RESPONSE_BODY
    },
    /* RFC 5789 */
    {"PATCH",               igloo_httpp_req_patch,
        igloo_HTTPP_REQUEST_HAS_REQUEST_BODY|igloo_HTTPP_REQUEST_HAS_RESPONSE_BODY
    },
    /* Icecast specific */
    {"SOURCE",              igloo_httpp_req_source,
        igloo_HTTPP_REQUEST_HAS_RESPONSE_BODY|igloo_HTTPP_REQUEST_HAS_REQUEST_BODY
    },
    {"PLAY",                igloo_httpp_req_play,
        igloo_HTTPP_REQUEST_IS_SAFE|igloo_HTTPP_REQUEST_HAS_RESPONSE_BODY|igloo_HTTPP_REQUEST_HAS_OPTIONAL_REQUEST_BODY
    },
    {"STATS",               igloo_httpp_req_stats,
        igloo_HTTPP_REQUEST_IS_SAFE|igloo_HTTPP_REQUEST_HAS_RESPONSE_BODY|igloo_HTTPP_REQUEST_HAS_OPTIONAL_REQUEST_BODY
    },

    /* Other RFCs */
    /*
     * tr -d \\r < www.iana.org/assignments/http-methods/methods.csv | while IFS=, read m s i r; do printf "    {\"%-21s igloo_httpp_req_%s,\n        /%c  Safe: %s, Idempotent: %s, Reference: %s %c/\n        igloo_HTTPP_REQUEST_INVALID\n    },\n" "$m"\", $(tr A-Z- a-z_ <<<"$m") '*' "$s" "$i" "$r" '*'; done
     */
    {"ACL",                 igloo_httpp_req_acl,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC3744, Section 8.1]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"BASELINE-CONTROL",    igloo_httpp_req_baseline_control,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC3253, Section 12.6]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"BIND",                igloo_httpp_req_bind,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC5842, Section 4]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"CHECKIN",             igloo_httpp_req_checkin,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC3253, Section 4.4, Section 9.4]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"CHECKOUT",            igloo_httpp_req_checkout,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC3253, Section 4.3, Section 8.8]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"COPY",                igloo_httpp_req_copy,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC4918, Section 9.8]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"LABEL",               igloo_httpp_req_label,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC3253, Section 8.2]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"LINK",                igloo_httpp_req_link,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC2068, Section 19.6.1.2]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"LOCK",                igloo_httpp_req_lock,
        /*  Safe: no, Idempotent: no, Reference: "[RFC4918, Section 9.10]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"MERGE",               igloo_httpp_req_merge,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC3253, Section 11.2]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"MKACTIVITY",          igloo_httpp_req_mkactivity,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC3253, Section 13.5]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"MKCALENDAR",          igloo_httpp_req_mkcalendar,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC4791, Section 5.3.1][RFC8144, Section 2.3]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"MKCOL",               igloo_httpp_req_mkcol,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC4918, Section 9.3][RFC5689, Section 3][RFC8144, Section 2.3]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"MKREDIRECTREF",       igloo_httpp_req_mkredirectref,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC4437, Section 6]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"MKWORKSPACE",         igloo_httpp_req_mkworkspace,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC3253, Section 6.3]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"MOVE",                igloo_httpp_req_move,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC4918, Section 9.9]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"ORDERPATCH",          igloo_httpp_req_orderpatch,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC3648, Section 7]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"PRI",                 igloo_httpp_req_pri,
        /*  Safe: yes, Idempotent: yes, Reference: "[RFC7540, Section 3.5]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"PROPFIND",            igloo_httpp_req_propfind,
        /*  Safe: yes, Idempotent: yes, Reference: "[RFC4918, Section 9.1][RFC8144, Section 2.1]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"PROPPATCH",           igloo_httpp_req_proppatch,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC4918, Section 9.2][RFC8144, Section 2.2]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"REBIND",              igloo_httpp_req_rebind,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC5842, Section 6]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"REPORT",              igloo_httpp_req_report,
        /*  Safe: yes, Idempotent: yes, Reference: "[RFC3253, Section 3.6][RFC8144, Section 2.1]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"SEARCH",              igloo_httpp_req_search,
        /*  Safe: yes, Idempotent: yes, Reference: "[RFC5323, Section 2]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"UNBIND",              igloo_httpp_req_unbind,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC5842, Section 5]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"UNCHECKOUT",          igloo_httpp_req_uncheckout,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC3253, Section 4.5]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"UNLINK",              igloo_httpp_req_unlink,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC2068, Section 19.6.1.3]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"UNLOCK",              igloo_httpp_req_unlock,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC4918, Section 9.11]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"UPDATE",              igloo_httpp_req_update,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC3253, Section 7.1]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"UPDATEREDIRECTREF",   igloo_httpp_req_updateredirectref,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC4437, Section 7]" */
        igloo_HTTPP_REQUEST_INVALID
    },
    {"VERSION-CONTROL",     igloo_httpp_req_version_control,
        /*  Safe: no, Idempotent: yes, Reference: "[RFC3253, Section 3.5]" */
        igloo_HTTPP_REQUEST_INVALID
    },
};

igloo_httpp_request_info_t igloo_httpp_request_info(igloo_httpp_request_type_e req)
{
    size_t i;

    for (i = 0; i < (sizeof(igloo_httpp__methods)/sizeof(*igloo_httpp__methods)); i++) {
        if (igloo_httpp__methods[i].req == req) {
            return igloo_httpp__methods[i].info;
        }
    }

    return igloo_HTTPP_REQUEST_INVALID;
}

igloo_http_parser_t *igloo_httpp_create_parser(void)
{
    igloo_http_parser_t *parser = calloc(1, sizeof(igloo_http_parser_t));

    parser->refc = 1;
    parser->req_type = igloo_httpp_req_none;
    parser->uri = NULL;
    parser->vars = igloo_avl_tree_new(igloo__compare_vars, NULL);
    parser->queryvars = igloo_avl_tree_new(igloo__compare_vars, NULL);
    parser->postvars = igloo_avl_tree_new(igloo__compare_vars, NULL);

    return parser;
}

void igloo_httpp_initialize(igloo_http_parser_t *parser, igloo_http_varlist_t *defaults)
{
    igloo_http_varlist_t *list;

    /* now insert the default variables */
    list = defaults;
    while (list != NULL) {
        size_t i;

        for (i = 0; i < list->var.values; i++) {
            igloo_httpp_setvar(parser, list->var.name, list->var.value[i]);
        }

        list = list->next;
    }
}

static int igloo_split_headers(char *data, unsigned long len, char **line)
{
    /* first we count how many lines there are 
    ** and set up the line[] array     
    */
    int lines = 0;
    unsigned long i;
    line[lines] = data;
    for (i = 0; i < len && lines < MAX_HEADERS; i++) {
        if (data[i] == '\r')
            data[i] = '\0';
        if (data[i] == '\n') {
            lines++;
            data[i] = '\0';
            if (lines >= MAX_HEADERS)
                return MAX_HEADERS;
            if (i + 1 < len) {
                if (data[i + 1] == '\n' || data[i + 1] == '\r')
                    break;
                line[lines] = &data[i + 1];
            }
        }
    }

    i++;
    while (i < len && data[i] == '\n') i++;

    return lines;
}

static void igloo_parse_headers(igloo_http_parser_t *parser, char **line, int lines)
{
    int i, l;
    int whitespace, slen;
    char *name = NULL;
    char *value = NULL;

    /* parse the name: value lines. */
    for (l = 1; l < lines; l++) {
        whitespace = 0;
        name = line[l];
        value = NULL;
        slen = strlen(line[l]);
        for (i = 0; i < slen; i++) {
            if (line[l][i] == ':') {
                whitespace = 1;
                line[l][i] = '\0';
            } else {
                if (whitespace) {
                    whitespace = 0;
                    while (i < slen && line[l][i] == ' ')
                        i++;

                    if (i < slen)
                        value = &line[l][i];
                    
                    break;
                }
            }
        }
        
        if (name != NULL && value != NULL) {
            igloo_httpp_setvar(parser, _lowercase(name), value);
            name = NULL; 
            value = NULL;
        }
    }
}

int igloo_httpp_parse_response(igloo_http_parser_t *parser, const char *http_data, unsigned long len, const char *uri)
{
    char *data;
    char *line[MAX_HEADERS];
    int lines, slen,i, whitespace=0, where=0,code;
    char *version=NULL, *resp_code=NULL, *message=NULL;
    
    if(http_data == NULL)
        return 0;

    /* make a local copy of the data, including 0 terminator */
    data = (char *)malloc(len+1);
    if (data == NULL) return 0;
    memcpy(data, http_data, len);
    data[len] = 0;

    lines = igloo_split_headers(data, len, line);

    /* In this case, the first line contains:
     * VERSION RESPONSE_CODE MESSAGE, such as HTTP/1.0 200 OK
     */
    slen = strlen(line[0]);
    version = line[0];
    for(i=0; i < slen; i++) {
        if(line[0][i] == ' ') {
            line[0][i] = 0;
            whitespace = 1;
        } else if(whitespace) {
            whitespace = 0;
            where++;
            if(where == 1)
                resp_code = &line[0][i];
            else {
                message = &line[0][i];
                break;
            }
        }
    }

    if(version == NULL || resp_code == NULL || message == NULL) {
        free(data);
        return 0;
    }

    igloo_httpp_setvar(parser, igloo_HTTPP_VAR_ERROR_CODE, resp_code);
    code = atoi(resp_code);
    if(code < 200 || code >= 300) {
        igloo_httpp_setvar(parser, igloo_HTTPP_VAR_ERROR_MESSAGE, message);
    }

    igloo_httpp_setvar(parser, igloo_HTTPP_VAR_URI, uri);
    igloo_httpp_setvar(parser, igloo_HTTPP_VAR_REQ_TYPE, "NONE");

    igloo_parse_headers(parser, line, lines);

    free(data);

    return 1;
}

int igloo_httpp_parse_postdata(igloo_http_parser_t *parser, const char *body_data, size_t len)
{
    const char *header = igloo_httpp_getvar(parser, "content-type");

    if (strcasecmp(header, "application/x-www-form-urlencoded") != 0) {
        return -1;
    }

    parse_query(parser->postvars, body_data, len);

    return 0;
}

static int hex(char c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    else if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else
        return -1;
}

static char *igloo_url_unescape(const char *src, size_t len)
{
    unsigned char *decoded;
    size_t i;
    char *dst;
    int done = 0;

    decoded = calloc(1, len + 1);

    dst = (char *)decoded;

    for(i=0; i < len; i++) {
        switch(src[i]) {
        case '%':
            if(i+2 >= len) {
                free(decoded);
                return NULL;
            }
            if(hex(src[i+1]) == -1 || hex(src[i+2]) == -1 ) {
                free(decoded);
                return NULL;
            }

            *dst++ = hex(src[i+1]) * 16  + hex(src[i+2]);
            i+= 2;
            break;
        case '+':
            *dst++ = ' ';
            break;
        case '#':
            done = 1;
            break;
        case 0:
            free(decoded);
            return NULL;
            break;
        default:
            *dst++ = src[i];
            break;
        }
        if(done)
            break;
    }

    *dst = 0; /* null terminator */

    return (char *)decoded;
}

static void parse_query_element(igloo_avl_tree *tree, const char *start, const char *mid, const char *end)
{
    size_t keylen;
    char *key;
    size_t valuelen;
    char *value;

    if (start >= end)
        return;

    if (!mid)
        return;

    keylen = mid - start;
    valuelen = end - mid - 1;

    /* REVIEW: We ignore keys with empty values. */
    if (!keylen || !valuelen)
        return;

    key = malloc(keylen + 1);
    memcpy(key, start, keylen);
    key[keylen] = 0;

    value = igloo_url_unescape(mid + 1, valuelen);

    _httpp_set_param_nocopy(tree, key, value, 0);
}

static void parse_query(igloo_avl_tree *tree, const char *query, size_t len)
{
    const char *start = query;
    const char *mid = NULL;
    size_t i;

    if (!query || !*query)
        return;

    for (i = 0; i < len; i++) {
        switch (query[i]) {
            case '&':
                parse_query_element(tree, start, mid, &(query[i]));
                start = &(query[i + 1]);
                mid = NULL;
            break;
            case '=':
                mid = &(query[i]);
            break;
        }
    }

    parse_query_element(tree, start, mid, &(query[i]));
}

int igloo_httpp_parse(igloo_http_parser_t *parser, const char *http_data, unsigned long len)
{
    char *data, *tmp;
    char *line[MAX_HEADERS]; /* limited to 32 lines, should be more than enough */
    int i;
    int lines;
    char *req_type = NULL;
    char *uri = NULL;
    char *version = NULL;
    int whitespace, where, slen;

    if (http_data == NULL)
        return 0;

    /* make a local copy of the data, including 0 terminator */
    data = (char *)malloc(len+1);
    if (data == NULL) return 0;
    memcpy(data, http_data, len);
    data[len] = 0;

    lines = igloo_split_headers(data, len, line);

    /* parse the first line special
    ** the format is:
    ** REQ_TYPE URI VERSION
    ** eg:
    ** GET /index.html HTTP/1.0
    */
    where = 0;
    whitespace = 0;
    slen = strlen(line[0]);
    req_type = line[0];
    for (i = 0; i < slen; i++) {
        if (line[0][i] == ' ') {
            whitespace = 1;
            line[0][i] = '\0';
        } else {
            /* we're just past the whitespace boundry */
            if (whitespace) {
                whitespace = 0;
                where++;
                switch (where) {
                    case 1:
                        uri = &line[0][i];
                    break;
                    case 2:
                        version = &line[0][i];
                    break;
                    case 3:
                        /* There is an extra element in the request line. This is not HTTP. */
                        free(data);
                        return 0;
                    break;
                }
            }
        }
    }

    parser->req_type = igloo_httpp_str_to_method(req_type);

    if (uri != NULL && strlen(uri) > 0) {
        char *query;
        if((query = strchr(uri, '?')) != NULL) {
            igloo_httpp_setvar(parser, igloo_HTTPP_VAR_RAWURI, uri);
            igloo_httpp_setvar(parser, igloo_HTTPP_VAR_QUERYARGS, query);
            *query = 0;
            query++;
            parse_query(parser->queryvars, query, strlen(query));
        }

        parser->uri = strdup(uri);
    } else {
        free(data);
        return 0;
    }

    if ((version != NULL) && ((tmp = strchr(version, '/')) != NULL)) {
        tmp[0] = '\0';
        if ((strlen(version) > 0) && (strlen(&tmp[1]) > 0)) {
            igloo_httpp_setvar(parser, igloo_HTTPP_VAR_PROTOCOL, version);
            igloo_httpp_setvar(parser, igloo_HTTPP_VAR_VERSION, &tmp[1]);
        } else {
            free(data);
            return 0;
        }
    } else {
        free(data);
        return 0;
    }

    if (parser->req_type != igloo_httpp_req_none && parser->req_type != igloo_httpp_req_unknown) {
		const char *method = igloo_httpp_method_to_str(parser->req_type);
        if (method) {
            igloo_httpp_setvar(parser, igloo_HTTPP_VAR_REQ_TYPE, method);
        }
    } else {
        free(data);
        return 0;
    }

    if (parser->uri != NULL) {
        igloo_httpp_setvar(parser, igloo_HTTPP_VAR_URI, parser->uri);
    } else {
        free(data);
        return 0;
    }

    igloo_parse_headers(parser, line, lines);

    free(data);

    return 1;
}

void igloo_httpp_deletevar(igloo_http_parser_t *parser, const char *name)
{
    igloo_http_var_t var;

    if (parser == NULL || name == NULL)
        return;
    memset(&var, 0, sizeof(var));

    var.name = (char*)name;

    igloo_avl_delete(parser->vars, (void *)&var, igloo__free_vars);
}

void igloo_httpp_setvar(igloo_http_parser_t *parser, const char *name, const char *value)
{
    igloo_http_var_t *var;

    if (name == NULL || value == NULL)
        return;

    var = (igloo_http_var_t *)calloc(1, sizeof(igloo_http_var_t));
    if (var == NULL) return;

    var->value = calloc(1, sizeof(*var->value));
    if (!var->value) {
        free(var);
        return;
    }

    var->name = strdup(name);
    var->values = 1;
    var->value[0] = strdup(value);

    if (igloo_httpp_getvar(parser, name) == NULL) {
        igloo_avl_insert(parser->vars, (void *)var);
    } else {
        igloo_avl_delete(parser->vars, (void *)var, igloo__free_vars);
        igloo_avl_insert(parser->vars, (void *)var);
    }
}

const char *igloo_httpp_getvar(igloo_http_parser_t *parser, const char *name)
{
    igloo_http_var_t var;
    igloo_http_var_t *found;
    void *fp;

    if (parser == NULL || name == NULL)
        return NULL;

    fp = &found;
    memset(&var, 0, sizeof(var));
    var.name = (char*)name;

    if (igloo_avl_get_by_key(parser->vars, &var, fp) == 0) {
        if (!found->values)
            return NULL;
        return found->value[0];
    } else {
        return NULL;
    }
}

static void _httpp_set_param_nocopy(igloo_avl_tree *tree, char *name, char *value, int replace)
{
    igloo_http_var_t *var, *found;
    char **n;

    if (name == NULL || value == NULL)
        return;

    found = igloo__httpp_get_param_var(tree, name);

    if (replace || !found) {
        var = (igloo_http_var_t *)calloc(1, sizeof(igloo_http_var_t));
        if (var == NULL) {
            free(name);
            free(value);
            return;
        }

        var->name = name;
    } else {
        free(name);
        var = found;
    }

    n = realloc(var->value, sizeof(*n)*(var->values + 1));
    if (!n) {
        if (replace || !found) {
            free(name);
            free(var);
        }
        free(value);
        return;
    }

    var->value = n;
    var->value[var->values++] = value;

    if (replace && found) {
        igloo_avl_delete(tree, (void *)found, igloo__free_vars);
        igloo_avl_insert(tree, (void *)var);
    } else if (!found) {
        igloo_avl_insert(tree, (void *)var);
    }
}

static void _httpp_set_param(igloo_avl_tree *tree, const char *name, const char *value)
{
    if (name == NULL || value == NULL)
        return;

    _httpp_set_param_nocopy(tree, strdup(name), igloo_url_unescape(value, strlen(value)), 1);
}

static igloo_http_var_t *igloo__httpp_get_param_var(igloo_avl_tree *tree, const char *name)
{
    igloo_http_var_t var;
    igloo_http_var_t *found;
    void *fp;

    fp = &found;
    memset(&var, 0, sizeof(var));
    var.name = (char *)name;

    if (igloo_avl_get_by_key(tree, (void *)&var, fp) == 0)
        return found;
    else
        return NULL;
}
static const char *_httpp_get_param(igloo_avl_tree *tree, const char *name)
{
    igloo_http_var_t *res = igloo__httpp_get_param_var(tree, name);

    if (!res)
        return NULL;

    if (!res->values)
        return NULL;

    return res->value[0];
}

void igloo_httpp_set_query_param(igloo_http_parser_t *parser, const char *name, const char *value)
{
    return _httpp_set_param(parser->queryvars, name, value);
}

const char *igloo_httpp_get_query_param(igloo_http_parser_t *parser, const char *name)
{
    return _httpp_get_param(parser->queryvars, name);
}

void igloo_httpp_set_post_param(igloo_http_parser_t *parser, const char *name, const char *value)
{
    return _httpp_set_param(parser->postvars, name, value);
}

const char *igloo_httpp_get_post_param(igloo_http_parser_t *parser, const char *name)
{
    return _httpp_get_param(parser->postvars, name);
}

const igloo_http_var_t *igloo_httpp_get_param_var(igloo_http_parser_t *parser, const char *name)
{
    igloo_http_var_t *ret = igloo__httpp_get_param_var(parser->postvars, name);

    if (ret)
        return ret;

    return igloo__httpp_get_param_var(parser->queryvars, name);
}

const igloo_http_var_t *igloo_httpp_get_any_var(igloo_http_parser_t *parser, igloo_httpp_ns_t ns, const char *name)
{
    igloo_avl_tree *tree = NULL;

    if (!parser || !name)
        return NULL;

    switch (ns) {
        case igloo_HTTPP_NS_VAR:
            if (name[0] != '_' || name[1] != '_')
                return NULL;
            tree = parser->vars;
        break;
        case igloo_HTTPP_NS_HEADER:
            if (name[0] == '_' && name[1] == '_')
                return NULL;
            tree = parser->vars;
        break;
        case igloo_HTTPP_NS_QUERY_STRING:
            tree = parser->queryvars;
        break;
        case igloo_HTTPP_NS_POST_BODY:
            tree = parser->postvars;
        break;
    }

    if (!tree)
        return NULL;

    return igloo__httpp_get_param_var(tree, name);
}

char ** igloo_httpp_get_any_key(igloo_http_parser_t *parser, igloo_httpp_ns_t ns)
{
    igloo_avl_tree *tree = NULL;
    igloo_avl_node *avlnode;
    char **ret;
    size_t len;
    size_t pos = 0;

    if (!parser)
        return NULL;

    switch (ns) {
        case igloo_HTTPP_NS_VAR:
        case igloo_HTTPP_NS_HEADER:
            tree = parser->vars;
        break;
        case igloo_HTTPP_NS_QUERY_STRING:
            tree = parser->queryvars;
        break;
        case igloo_HTTPP_NS_POST_BODY:
            tree = parser->postvars;
        break;
    }

    if (!tree)
        return NULL;

    ret = calloc(8, sizeof(*ret));
    if (!ret)
        return NULL;

    len = 8;

    for (avlnode = igloo_avl_get_first(tree); avlnode; avlnode = igloo_avl_get_next(avlnode)) {
        igloo_http_var_t *var = avlnode->key;

        if (ns == igloo_HTTPP_NS_VAR) {
            if (var->name[0] != '_' || var->name[1] != '_') {
                continue;
            }
        } else if (ns == igloo_HTTPP_NS_HEADER) {
            if (var->name[0] == '_' && var->name[1] == '_') {
                continue;
            }
        }

        if (pos == (len-1)) {
            char **n = realloc(ret, sizeof(*ret)*(len + 8));
            if (!n) {
                igloo_httpp_free_any_key(ret);
                return NULL;
            }
            memset(n + len, 0, sizeof(*n)*8);
            ret = n;
            len += 8;
        }

        ret[pos] = strdup(var->name);
        if (!ret[pos]) {
            igloo_httpp_free_any_key(ret);
            return NULL;
        }

        pos++;
    }

    return ret;
}

void igloo_httpp_free_any_key(char **keys)
{
    char **p;

    if (!keys)
        return;

    for (p = keys; *p; p++) {
        free(*p);
    }
    free(keys);
}

const char *igloo_httpp_get_param(igloo_http_parser_t *parser, const char *name)
{
    const char *ret = _httpp_get_param(parser->postvars, name);

    if (ret)
        return ret;

    return _httpp_get_param(parser->queryvars, name);
}

static void httpp_clear(igloo_http_parser_t *parser)
{
    parser->req_type = igloo_httpp_req_none;
    if (parser->uri)
        free(parser->uri);
    parser->uri = NULL;
    igloo_avl_tree_free(parser->vars, igloo__free_vars);
    igloo_avl_tree_free(parser->queryvars, igloo__free_vars);
    igloo_avl_tree_free(parser->postvars, igloo__free_vars);
    parser->vars = NULL;
}

int igloo_httpp_addref(igloo_http_parser_t *parser)
{
    if (!parser)
        return -1;

    parser->refc++;

    return 0;
}

int igloo_httpp_release(igloo_http_parser_t *parser)
{
    if (!parser)
        return -1;

    parser->refc--;
    if (parser->refc)
        return 0;

    httpp_clear(parser);
    free(parser);

    return 0;
}

static char *_lowercase(char *str)
{
    char *p = str;
    for (; *p != '\0'; p++)
        *p = tolower(*p);

    return str;
}

static int igloo__compare_vars(void *compare_arg, void *a, void *b)
{
    igloo_http_var_t *vara, *varb;

    vara = (igloo_http_var_t *)a;
    varb = (igloo_http_var_t *)b;

    return strcmp(vara->name, varb->name);
}

static int igloo__free_vars(void *key)
{
    igloo_http_var_t *var = (igloo_http_var_t *)key;
    size_t i;

    free(var->name);

    for (i = 0; i < var->values; i++) {
        free(var->value[i]);
    }
    free(var->value);
    free(var);

    return 1;
}

igloo_httpp_request_type_e igloo_httpp_str_to_method(const char * method)
{
    size_t i;

    for (i = 0; i < (sizeof(igloo_httpp__methods)/sizeof(*igloo_httpp__methods)); i++) {
        if (strcasecmp(igloo_httpp__methods[i].name, method) == 0) {
            return igloo_httpp__methods[i].req;
        }
    }

    return igloo_httpp_req_unknown;
}

const char *               igloo_httpp_method_to_str(igloo_httpp_request_type_e method)
{
    size_t i;

    for (i = 0; i < (sizeof(igloo_httpp__methods)/sizeof(*igloo_httpp__methods)); i++) {
        if (igloo_httpp__methods[i].req ==  method) {
            return igloo_httpp__methods[i].name;
        }
    }

    return NULL;
}
