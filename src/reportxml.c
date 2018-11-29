/* Icecast
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2018,      Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>,
 */

/**
 * Special fast event functions
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <igloo/reportxml.h>
#include <igloo/thread.h>
#include <igloo/avl.h>

#define XMLSTR(str) ((xmlChar *)(str))

/* The report XML document type */
struct igloo_reportxml_tag {
    /* base object */
    igloo_ro_base_t __base;
    /* the root report XML node of the document */
    igloo_reportxml_node_t *root;
};

/* The report XML node type */
struct igloo_reportxml_node_tag {
    /* base object */
    igloo_ro_base_t __base;
    /* an XML node used to store the attributes */
    xmlNodePtr xmlnode;
    /* the type of the node */
    igloo_reportxml_node_type_t type;
    /* the report XML childs */
    igloo_reportxml_node_t **childs;
    size_t childs_len;
    /* the XML childs (used by <extension>) */
    xmlNodePtr *xml_childs;
    size_t xml_childs_len;
    /* the node's text content (used by <text>) */
    char *content;
};

/* The report XML database type */
struct igloo_reportxml_database_tag {
    /* base object */
    igloo_ro_base_t __base;
    /* the lock used to ensure the database object is thread safe. */
    igloo_mutex_t lock;
    /* The tree of definitions */
    igloo_avl_tree *definitions;
};

/* The nodeattr structure is used to store definition of node attributes */
struct nodeattr;
struct nodeattr {
    /* name of the attribute */
    const char *name;
    /* the type of the attribute. This is based on the DTD */
    const char *type;
    /* the default value for the attribute if any */
    const char *def;
    /* whether the attribute is required or not */
    int required;
    /* a function that can be used to check the content of the attribute if any */
    int (*checker)(const struct nodeattr *attr, const char *str);
    /* NULL terminated list of possible values (if enum) */
    const char *values[32];
};

/* The type of the content an node has */
enum nodecontent {
    /* The node may not have any content */
    NC_NONE,
    /* The node may have children */
    NC_CHILDS,
    /* The node may have a text content */
    NC_CONTENT,
    /* The node may have XML children */
    NC_XML
};

/* This structure is used to define a node */
struct nodedef {
    /* the type of the node */
    igloo_reportxml_node_type_t type;
    /* the name of the corresponding XML node */
    const char *name;
    /* The type of the content the node may have */
    enum nodecontent content;
    /* __attr__eol terminated list of attributes the node may have */
    const struct nodeattr *attr[12];
    /* igloo_REPORTXML_NODE_TYPE__ERROR terminated list of child node types the node may have */
    const igloo_reportxml_node_type_t childs[12];
};

static void __report_free(igloo_ro_t self);
static int  __report_new(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap);
static void __report_node_free(igloo_ro_t self);
static void __database_free(igloo_ro_t self);
static int  __database_new(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap);

igloo_RO_PUBLIC_TYPE(igloo_reportxml_t,
        igloo_RO_TYPEDECL_FREE(__report_free),
        igloo_RO_TYPEDECL_NEW(__report_new)
        );

igloo_RO_PUBLIC_TYPE(igloo_reportxml_node_t,
        igloo_RO_TYPEDECL_FREE(__report_node_free)
        );

igloo_RO_PUBLIC_TYPE(igloo_reportxml_database_t,
        igloo_RO_TYPEDECL_FREE(__database_free),
        igloo_RO_TYPEDECL_NEW(__database_new)
        );

/* Prototypes */
static int __attach_copy_of_node_or_definition(igloo_reportxml_node_t *parent, igloo_reportxml_node_t *node, igloo_reportxml_database_t *db, ssize_t depth);
static igloo_reportxml_node_t *      __reportxml_database_build_node_ext(igloo_reportxml_database_t *db, const char *id, ssize_t depth, igloo_reportxml_node_type_t *acst_type_ret);

/* definition of known attributes */
static const struct nodeattr __attr__eol[1]             = {{NULL,           NULL,           NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_version[1]          = {{"version",      "CDATA",        "0.0.1",  1,  NULL, {"0.0.1", NULL}}};
static const struct nodeattr __attr_xmlns[1]            = {{"xmlns",        "URI",          "http://icecast.org/specs/reportxml-0.0.1",    1,  NULL, {"http://icecast.org/specs/reportxml-0.0.1", NULL}}};
static const struct nodeattr __attr_id[1]               = {{"id",           "ID",           NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_definition[1]       = {{"definition",   "UUID",         NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr__definition[1]      = {{"_definition",  "UUID",         NULL,     0,  NULL, {NULL}}}; /* for internal use only */
static const struct nodeattr __attr_akindof[1]          = {{"akindof",      "UUIDs",        NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_lang[1]             = {{"lang",         "LanguageCode", NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_dir[1]              = {{"dir",          NULL,           NULL,     0,  NULL, {"ltr", "rtl", NULL}}};
static const struct nodeattr __attr_template[1]         = {{"template",     "UUID",         NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_defines[1]          = {{"defines",      "UUID",         NULL,     1,  NULL, {NULL}}};
static const struct nodeattr __attr_function[1]         = {{"function",     "CDATA",        NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_filename[1]         = {{"filename",     "CDATA",        NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_line[1]             = {{"line",         "CDATA",        NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_binary[1]           = {{"binary",       "CDATA",        NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_offset[1]           = {{"offset",       "CDATA",        NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_absolute[1]         = {{"absolute",     "iso8601",      NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_relative[1]         = {{"relative",     "iso8601",      NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_name[1]             = {{"name",         "CDATA",        NULL,     1,  NULL, {NULL}}};
static const struct nodeattr __attr_member[1]           = {{"member",       "CDATA",        NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_value[1]            = {{"value",        "CDATA",        NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_state[1]            = {{"state",        NULL,           "set",    1,  NULL, {"declared", "set", "uninitialized", "missing", "unset", "removed", NULL}}};
static const struct nodeattr __attr_href[1]             = {{"href",         "URI",          NULL,     0,  NULL, {NULL}}};
static const struct nodeattr __attr_application[1]      = {{"application",  "URI",          NULL,     1,  NULL, {NULL}}};
static const struct nodeattr __attr__action_type[1]     = {{"type",         NULL,           NULL,     1,  NULL, {"retry", "choice", "see-other", "authenticate", "pay", "change-protocol", "slow-down", "ask-user", "ask-admin", "bug", NULL}}};
static const struct nodeattr __attr__resource_type[1]   = {{"type",         NULL,           NULL,     1,  NULL, {"actor", "manipulation-target", "helper", "related", "result", "parameter", "state", NULL}}};
static const struct nodeattr __attr__value_type[1]      = {{"type",         NULL,           NULL,     1,  NULL, {"null", "int", "float", "uuid", "string", "structure", "uri", "pointer", "version", "protocol", "username", "password", "boolean", NULL}}};
static const struct nodeattr __attr__reference_type[1]  = {{"type",         NULL,           NULL,     1,  NULL, {"documentation", "log", "report", "related", NULL}}};

/* definition of known nodes */
/* Helper:
 * grep '^ *igloo_REPORTXML_NODE_TYPE_' reportxml.h | sed 's/^\( *igloo_REPORTXML_NODE_TYPE_\([^,]*\)\),*$/\1 \2/;' | while read l s; do c=$(tr A-Z a-z <<<"$s"); printf "    {%-32s \"%-16s 0, {__attr__eol}},\n" "$l," "$c\","; done
 */
#define __BASIC_ELEMENT __attr_id, __attr_definition, __attr_akindof, __attr__definition
static const struct nodedef __nodedef[] = {
    {igloo_REPORTXML_NODE_TYPE_REPORT,      "report",         NC_CHILDS,  {__attr_id, __attr_version, __attr_xmlns, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_INCIDENT, igloo_REPORTXML_NODE_TYPE_DEFINITION, igloo_REPORTXML_NODE_TYPE_TIMESTAMP, igloo_REPORTXML_NODE_TYPE_REFERENCE, igloo_REPORTXML_NODE_TYPE_EXTENSION, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_DEFINITION,  "definition",     NC_CHILDS,  {__BASIC_ELEMENT, __attr_template, __attr_defines, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_INCIDENT, igloo_REPORTXML_NODE_TYPE_STATE, igloo_REPORTXML_NODE_TYPE_TIMESTAMP, igloo_REPORTXML_NODE_TYPE_RESOURCE, igloo_REPORTXML_NODE_TYPE_REFERENCE, igloo_REPORTXML_NODE_TYPE_FIX, igloo_REPORTXML_NODE_TYPE_RESOURCE, igloo_REPORTXML_NODE_TYPE_REASON, igloo_REPORTXML_NODE_TYPE_TEXT, igloo_REPORTXML_NODE_TYPE_EXTENSION, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_INCIDENT,    "incident",       NC_CHILDS,  {__BASIC_ELEMENT, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_STATE, igloo_REPORTXML_NODE_TYPE_TIMESTAMP, igloo_REPORTXML_NODE_TYPE_RESOURCE, igloo_REPORTXML_NODE_TYPE_REFERENCE, igloo_REPORTXML_NODE_TYPE_FIX, igloo_REPORTXML_NODE_TYPE_REASON, igloo_REPORTXML_NODE_TYPE_EXTENSION, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_STATE,       "state",          NC_CHILDS,  {__BASIC_ELEMENT, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_TEXT, igloo_REPORTXML_NODE_TYPE_TIMESTAMP, igloo_REPORTXML_NODE_TYPE_BACKTRACE, igloo_REPORTXML_NODE_TYPE_EXTENSION, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_BACKTRACE,   "backtrace",      NC_CHILDS,  {__BASIC_ELEMENT, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_POSITION, igloo_REPORTXML_NODE_TYPE_MORE, igloo_REPORTXML_NODE_TYPE_TEXT, igloo_REPORTXML_NODE_TYPE_REFERENCE, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_POSITION,    "position",       NC_CHILDS,  {__BASIC_ELEMENT, __attr_function, __attr_filename, __attr_line, __attr_binary, __attr_offset, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_TEXT, igloo_REPORTXML_NODE_TYPE_REFERENCE, igloo_REPORTXML_NODE_TYPE_EXTENSION, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_MORE,        "more",           NC_CHILDS,  {__BASIC_ELEMENT, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_TEXT, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_FIX,         "fix",            NC_CHILDS,  {__BASIC_ELEMENT, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_ACTION, igloo_REPORTXML_NODE_TYPE_EXTENSION, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_ACTION,      "action",         NC_CHILDS,  {__BASIC_ELEMENT, __attr__action_type, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_TEXT, igloo_REPORTXML_NODE_TYPE_TIMESTAMP, igloo_REPORTXML_NODE_TYPE_VALUE, igloo_REPORTXML_NODE_TYPE_EXTENSION, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_REASON,      "reason",         NC_CHILDS,  {__BASIC_ELEMENT, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_TEXT, igloo_REPORTXML_NODE_TYPE_RESOURCE, igloo_REPORTXML_NODE_TYPE_REFERENCE, igloo_REPORTXML_NODE_TYPE_EXTENSION, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_TEXT,        "text",           NC_CONTENT, {__BASIC_ELEMENT, __attr_lang, __attr_dir, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_TIMESTAMP,   "timestamp",      NC_NONE,    {__BASIC_ELEMENT, __attr_absolute, __attr_relative, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_RESOURCE,    "resource",       NC_CHILDS,  {__BASIC_ELEMENT, __attr__resource_type, __attr_name, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_VALUE, igloo_REPORTXML_NODE_TYPE_REFERENCE, igloo_REPORTXML_NODE_TYPE_EXTENSION, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_VALUE,       "value",          NC_CHILDS,  {__BASIC_ELEMENT, __attr_member, __attr_value, __attr_state, __attr__value_type, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_TEXT, igloo_REPORTXML_NODE_TYPE_REFERENCE, igloo_REPORTXML_NODE_TYPE_VALUE, igloo_REPORTXML_NODE_TYPE_POSITION, igloo_REPORTXML_NODE_TYPE_EXTENSION, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_REFERENCE,   "reference",      NC_CHILDS,  {__BASIC_ELEMENT, __attr__reference_type, __attr_href, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE_TEXT, igloo_REPORTXML_NODE_TYPE_EXTENSION, igloo_REPORTXML_NODE_TYPE__ERROR}},
    {igloo_REPORTXML_NODE_TYPE_EXTENSION,   "extension",      NC_XML,     {__BASIC_ELEMENT, __attr_application, __attr__eol},
        {igloo_REPORTXML_NODE_TYPE__ERROR}}
};

static const struct nodedef * __get_nodedef(igloo_reportxml_node_type_t type)
{
    size_t i;

    for (i = 0; i < (sizeof(__nodedef)/sizeof(*__nodedef)); i++) {
        if (__nodedef[i].type == type) {
            return &(__nodedef[i]);
        }
    }

    return NULL;
}

static const struct nodedef * __get_nodedef_by_name(const char *name)
{
    size_t i;

    for (i = 0; i < (sizeof(__nodedef)/sizeof(*__nodedef)); i++) {
        if (strcmp(__nodedef[i].name, name) == 0) {
            return &(__nodedef[i]);
        }
    }

    return NULL;
}

static const struct nodeattr * __get_nodeattr(const struct nodedef * nodedef, const char *key)
{
    size_t i;

    if (!nodedef || !key)
        return NULL;

    for (i = 0; i < (sizeof(nodedef->attr)/sizeof(*nodedef->attr)); i++) {
        if (nodedef->attr[i]->name == NULL) {
            /* we reached the end of the list */
            return NULL;
        }

        if (strcmp(nodedef->attr[i]->name, key) == 0) {
            return nodedef->attr[i];
        }
    }

    return NULL;
}


static void __report_free(igloo_ro_t self)
{
    igloo_reportxml_t *report = igloo_RO_TO_TYPE(self, igloo_reportxml_t);

    igloo_ro_unref(report->root);
}

static int  __report_new(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap)
{
    igloo_reportxml_t *ret = igloo_RO_TO_TYPE(self, igloo_reportxml_t);
    igloo_reportxml_node_t *root = igloo_reportxml_node_new(igloo_REPORTXML_NODE_TYPE_REPORT, NULL, NULL, NULL);

    if (!root)
        return -1;

    ret->root = root;

    return 0;
}

static igloo_reportxml_t *    reportxml_new_with_root(igloo_reportxml_node_t *root)
{
    igloo_reportxml_t *ret;

    if (!root)
        return NULL;

    ret = igloo_ro_new_raw(igloo_reportxml_t, NULL, igloo_RO_NULL);
    if (!ret)
        return NULL;

    ret->root = root;

    return ret;
}

igloo_reportxml_node_t *      igloo_reportxml_get_root_node(igloo_reportxml_t *report)
{
    if (!report)
        return NULL;

    if (igloo_ro_ref(report->root) != 0)
        return NULL;

    return report->root;
}

igloo_reportxml_node_t *      igloo_reportxml_get_node_by_attribute(igloo_reportxml_t *report, const char *key, const char *value, int include_definitions)
{
    if (!report || !key || !value)
        return NULL;

    return igloo_reportxml_node_get_child_by_attribute(report->root, key, value, include_definitions);
}

igloo_reportxml_node_t *      igloo_reportxml_get_node_by_type(igloo_reportxml_t *report, igloo_reportxml_node_type_t type, int include_definitions)
{
    if (!report)
        return NULL;

    return igloo_reportxml_node_get_child_by_type(report->root, type, include_definitions);
}

igloo_reportxml_t *           igloo_reportxml_parse_xmldoc(xmlDocPtr doc)
{
    igloo_reportxml_node_t *root;
    igloo_reportxml_t *ret;
    xmlNodePtr xmlroot;

    if (!doc)
        return NULL;

    xmlroot = xmlDocGetRootElement(doc);
    if (!xmlroot)
        return NULL;

    root = igloo_reportxml_node_parse_xmlnode(xmlroot);
    if (!root)
        return NULL;

    if (igloo_reportxml_node_get_type(root) != igloo_REPORTXML_NODE_TYPE_REPORT) {
        igloo_ro_unref(root);
        return NULL;
    }

    ret = reportxml_new_with_root(root);
    if (!ret) {
        igloo_ro_unref(root);
        return NULL;
    }

    return ret;
}

xmlDocPtr               igloo_reportxml_render_xmldoc(igloo_reportxml_t *report)
{
    xmlDocPtr ret;
    xmlNodePtr node;

    if (!report)
        return NULL;

    node = igloo_reportxml_node_render_xmlnode(report->root);
    if (!node)
        return NULL;

    ret = xmlNewDoc(XMLSTR("1.0"));
    if (!ret) {
        xmlFreeNode(node);
        return NULL;
    }

    xmlDocSetRootElement(ret, node);

    return ret;
}

static void __report_node_free(igloo_ro_t self)
{
    size_t i;

    igloo_reportxml_node_t *node = igloo_RO_TO_TYPE(self, igloo_reportxml_node_t);
    xmlFreeNode(node->xmlnode);

    for (i = 0; i < node->childs_len; i++) {
        igloo_ro_unref(node->childs[i]);
    }

    for (i = 0; i < node->xml_childs_len; i++) {
        xmlFreeNode(node->xml_childs[i]);
    }

    free(node->content);
    free(node->childs);
    free(node->xml_childs);
}

igloo_reportxml_node_t *      igloo_reportxml_node_new(igloo_reportxml_node_type_t type, const char *id, const char *definition, const char *akindof)
{
    igloo_reportxml_node_t *ret;
    const struct nodedef *nodedef = __get_nodedef(type);
    size_t i;

    if (!nodedef)
        return NULL;

    ret = igloo_ro_new_raw(igloo_reportxml_node_t, NULL, igloo_RO_NULL);

    if (ret == NULL)
        return NULL;

    ret->type = type;

    ret->xmlnode = xmlNewNode(NULL, XMLSTR(nodedef->name));

    if (!ret->xmlnode) {
        igloo_ro_unref(ret);
        return NULL;
    }

    for (i = 0; i < (sizeof(nodedef->attr)/sizeof(*nodedef->attr)); i++) {
        const struct nodeattr *nodeattr = nodedef->attr[i];
        if (nodeattr->name == NULL)
            break;

        if (nodeattr->def) {
            if (igloo_reportxml_node_set_attribute(ret, nodeattr->name, nodeattr->def) != 0) {
                igloo_ro_unref(ret);
                return NULL;
            }
        }
    }

#define _set_attr(x) \
    if ((x)) { \
        if (igloo_reportxml_node_set_attribute(ret, # x , (x)) != 0) { \
            igloo_ro_unref(ret); \
            return NULL; \
        } \
    }

    _set_attr(id)
    _set_attr(definition)
    _set_attr(akindof)

    return ret;
}

igloo_reportxml_node_t *      igloo_reportxml_node_parse_xmlnode(xmlNodePtr xmlnode)
{
    igloo_reportxml_node_t *node;

    const struct nodedef *nodedef;

    if (!xmlnode)
        return NULL;

    nodedef = __get_nodedef_by_name((const char *)xmlnode->name);
    if (!nodedef) {
        return NULL;
    }

    node = igloo_reportxml_node_new(nodedef->type, NULL, NULL, NULL);
    if (!node)
        return NULL;

    if (xmlnode->properties) {
        xmlAttrPtr cur = xmlnode->properties;

        do {
            xmlChar *value = xmlNodeListGetString(xmlnode->doc, cur->children, 1);
            if (!value) {
                igloo_ro_unref(node);
                return NULL;
            }

            if (igloo_reportxml_node_set_attribute(node, (const char*)cur->name, (const char*)value) != 0) {
                igloo_ro_unref(node);
                return NULL;
            }

            xmlFree(value);
        } while ((cur = cur->next));
    }

    if (xmlnode->xmlChildrenNode) {
        xmlNodePtr cur = xmlnode->xmlChildrenNode;

        do {
            if (nodedef->content == NC_XML) {
                if (igloo_reportxml_node_add_xml_child(node, cur) != 0) {
                    igloo_ro_unref(node);
                    return NULL;
                }
            } else {
                igloo_reportxml_node_t *child;

                if (xmlIsBlankNode(cur))
                    continue;

                if (cur->type == XML_COMMENT_NODE)
                    continue;

                if (cur->type == XML_TEXT_NODE) {
                    xmlChar *value = xmlNodeListGetString(xmlnode->doc, cur, 1);

                    if (!value) {
                        igloo_ro_unref(node);
                        return NULL;
                    }

                    if (igloo_reportxml_node_set_content(node, (const char *)value) != 0) {
                        igloo_ro_unref(node);
                        return NULL;
                    }

                    xmlFree(value);
                    continue;
                }

                child = igloo_reportxml_node_parse_xmlnode(cur);
                if (!child) {
                    igloo_ro_unref(node);
                    return NULL;
                }

                if (igloo_reportxml_node_add_child(node, child) != 0) {
                    igloo_ro_unref(child);
                    igloo_ro_unref(node);
                    return NULL;
                }
                igloo_ro_unref(child);
            }
        } while ((cur = cur->next));
    }

    return node;
}

static igloo_reportxml_node_t *      __reportxml_node_copy_with_db(igloo_reportxml_node_t *node, igloo_reportxml_database_t *db, ssize_t depth)
{
    igloo_reportxml_node_t *ret;
    ssize_t child_count;
    ssize_t xml_child_count;
    size_t i;

    if (!node)
        return NULL;

    child_count = igloo_reportxml_node_count_child(node);
    if (child_count < 0)
        return NULL;

    xml_child_count = igloo_reportxml_node_count_xml_child(node);
    if (xml_child_count < 0)
        return NULL;

    ret = igloo_reportxml_node_parse_xmlnode(node->xmlnode);
    if (!ret)
        return NULL;

    if (node->content) {
        if (igloo_reportxml_node_set_content(ret, node->content) != 0) {
            igloo_ro_unref(ret);
            return NULL;
        }
    }

    for (i = 0; i < (size_t)child_count; i++) {
        igloo_reportxml_node_t *child = igloo_reportxml_node_get_child(node, i);

        if (db && depth > 0) {
            if (__attach_copy_of_node_or_definition(ret, child, db, depth) != 0) {
                igloo_ro_unref(child);
                igloo_ro_unref(ret);
                return NULL;
            }
            igloo_ro_unref(child);
        } else {
            igloo_reportxml_node_t *copy = __reportxml_node_copy_with_db(child, NULL, -1);

            igloo_ro_unref(child);

            if (!copy) {
                igloo_ro_unref(ret);
                return NULL;
            }

            if (igloo_reportxml_node_add_child(ret, copy) != 0) {
                igloo_ro_unref(copy);
                igloo_ro_unref(ret);
                return NULL;
            }

            igloo_ro_unref(copy);
        }
    }

    for (i = 0; i < (size_t)xml_child_count; i++) {
        xmlNodePtr child = igloo_reportxml_node_get_xml_child(node, i);
        if (igloo_reportxml_node_add_xml_child(ret, child) != 0) {
            xmlFreeNode(child);
            igloo_ro_unref(ret);
            return NULL;
        }
        xmlFreeNode(child);
    }

    return ret;
}

igloo_reportxml_node_t *      igloo_reportxml_node_copy(igloo_reportxml_node_t *node)
{
    return __reportxml_node_copy_with_db(node, NULL, -1);
}

xmlNodePtr              igloo_reportxml_node_render_xmlnode(igloo_reportxml_node_t *node)
{
    xmlNodePtr ret;
    ssize_t child_count;
    ssize_t xml_child_count;
    size_t i;
    xmlChar *definition;

    if (!node)
        return NULL;

    child_count = igloo_reportxml_node_count_child(node);
    if (child_count < 0)
        return NULL;

    xml_child_count = igloo_reportxml_node_count_xml_child(node);
    if (xml_child_count < 0)
        return NULL;

    ret = xmlCopyNode(node->xmlnode, 2);
    if (!ret)
        return NULL;

    definition = xmlGetProp(ret, XMLSTR("_definition"));
    if (definition) {
        xmlSetProp(ret, XMLSTR("definition"), definition);
        xmlUnsetProp(ret, XMLSTR("_definition"));
        xmlFree(definition);
    }

    for (i = 0; i < (size_t)child_count; i++) {
        igloo_reportxml_node_t *child = igloo_reportxml_node_get_child(node, i);
        xmlNodePtr xmlchild;

        if (!child) {
            xmlFreeNode(ret);
            return NULL;
        }

        xmlchild = igloo_reportxml_node_render_xmlnode(child);
        igloo_ro_unref(child);
        if (!xmlchild) {
            xmlFreeNode(ret);
            return NULL;
        }

        xmlAddChild(ret, xmlchild);
    }

    if (node->content) {
        xmlNodePtr xmlchild = xmlNewText(XMLSTR(node->content));

        if (!xmlchild) {
            xmlFreeNode(ret);
            return NULL;
        }

        xmlAddChild(ret, xmlchild);
    }

    for (i = 0; i < (size_t)xml_child_count; i++) {
        xmlNodePtr xmlchild = igloo_reportxml_node_get_xml_child(node, i);

        if (!xmlchild) {
            xmlFreeNode(ret);
            return NULL;
        }

        xmlAddChild(ret, xmlchild);
    }


    return ret;
}

igloo_reportxml_node_type_t   igloo_reportxml_node_get_type(igloo_reportxml_node_t *node)
{
    if (!node)
        return igloo_REPORTXML_NODE_TYPE__ERROR;

    return node->type;
}

int                     igloo_reportxml_node_set_attribute(igloo_reportxml_node_t *node, const char *key, const char *value)
{
    const struct nodedef *nodedef;
    const struct nodeattr *nodeattr;
    size_t i;

    if (!node || !key || !value)
        return -1;

    nodedef = __get_nodedef(node->type);
    nodeattr = __get_nodeattr(nodedef, key);
    if (!nodeattr)
        return -1;

    if (nodeattr->values[0] != NULL) {
        int found = 0;
        for (i = 0; i < (sizeof(nodeattr->values)/sizeof(*nodeattr->values)); i++) {
            if (nodeattr->values[i] == NULL)
                break;

            if (strcmp(nodeattr->values[i], value) == 0) {
                found = 1;
                break;
            }
        }

        if (!found)
            return -1;
    }

    if (nodeattr->checker) {
        if (nodeattr->checker(nodeattr, value) != 0) {
            return -1;
        }
    }

    xmlSetProp(node->xmlnode, XMLSTR(key), XMLSTR(value));

    return 0;
}

char *                  igloo_reportxml_node_get_attribute(igloo_reportxml_node_t *node, const char *key)
{
    xmlChar *k;
    char *n;

    if (!node || !key)
        return NULL;

    /* We do this super complicated thing as we do not know if we can use free() to free a xmlChar* object. */
    k = xmlGetProp(node->xmlnode, XMLSTR(key));
    if (!k)
        return NULL;

    n = strdup((char*)k);
    xmlFree(k);

    return n;
}

int                     igloo_reportxml_node_add_child(igloo_reportxml_node_t *node, igloo_reportxml_node_t *child)
{
    const struct nodedef *nodedef;
    igloo_reportxml_node_t **n;
    size_t i;
    int found;

    if (!node || !child)
        return -1;

    nodedef = __get_nodedef(node->type);

    if (nodedef->content != NC_CHILDS)
        return -1;

    found = 0;
    for (i = 0; nodedef->childs[i] != igloo_REPORTXML_NODE_TYPE__ERROR; i++) {
        if (nodedef->childs[i] == child->type) {
            found = 1;
            break;
        }
    }

    if (!found)
        return -1;

    n = realloc(node->childs, sizeof(*n)*(node->childs_len + 1));
    if (!n)
        return -1;

    node->childs = n;

    if (igloo_ro_ref(child) != 0)
        return -1;

    node->childs[node->childs_len++] = child;

    return 0;
}

ssize_t                 igloo_reportxml_node_count_child(igloo_reportxml_node_t *node)
{
    if (!node)
        return -1;

    return node->childs_len;
}

igloo_reportxml_node_t *      igloo_reportxml_node_get_child(igloo_reportxml_node_t *node, size_t idx)
{
    if (!node)
        return NULL;

    if (idx >= node->childs_len)
        return NULL;

    if (igloo_ro_ref(node->childs[idx]) != 0)
        return NULL;

    return node->childs[idx];
}

igloo_reportxml_node_t *      igloo_reportxml_node_get_child_by_attribute(igloo_reportxml_node_t *node, const char *key, const char *value, int include_definitions)
{
    igloo_reportxml_node_t *ret;
    xmlChar *k;
    size_t i;

    if (!node || !key ||!value)
        return NULL;

    k = xmlGetProp(node->xmlnode, XMLSTR(key));
    if (k != NULL) {
        if (strcmp((const char*)k, value) == 0) {
            xmlFree(k);

            if (igloo_ro_ref(node) != 0)
                return NULL;

            return node;
        }
        xmlFree(k);
    }

    if (node->type == igloo_REPORTXML_NODE_TYPE_DEFINITION && !include_definitions)
        return NULL;

    for (i = 0; i < node->childs_len; i++) {
        ret = igloo_reportxml_node_get_child_by_attribute(node->childs[i], key, value, include_definitions);
        if (ret != NULL)
            return ret;
    }

    return NULL;
}

igloo_reportxml_node_t *      igloo_reportxml_node_get_child_by_type(igloo_reportxml_node_t *node, igloo_reportxml_node_type_t type, int include_definitions)
{
    size_t i;

    if (!node)
        return NULL;

    if (node->type == type) {
        if (igloo_ro_ref(node) != 0)
            return NULL;
        return node;
    }

    if (node->type == igloo_REPORTXML_NODE_TYPE_DEFINITION && !include_definitions)
        return NULL;

    for (i = 0; i < node->childs_len; i++) {
        igloo_reportxml_node_t *ret;

        ret = igloo_reportxml_node_get_child_by_type(node->childs[i], type, include_definitions);
        if (ret != NULL)
            return ret;
    }

    return NULL;
}

int                     igloo_reportxml_node_set_content(igloo_reportxml_node_t *node, const char *value)
{
    const struct nodedef *nodedef;
    char *n = NULL;

    if (!node)
        return -1;

    nodedef = __get_nodedef(node->type);

    if (nodedef->content != NC_CONTENT)
        return -1;

    if (value) {
        n = strdup(value);
        if (!n)
            return -1;
    }

    free(node->content);
    node->content = n;

    return 0;
}

char *              igloo_reportxml_node_get_content(igloo_reportxml_node_t *node)
{
    if (!node)
        return NULL;

    if (node->content) {
        return strdup(node->content);
    } else {
        return NULL;
    }
}

int                     igloo_reportxml_node_add_xml_child(igloo_reportxml_node_t *node, xmlNodePtr child)
{
    const struct nodedef *nodedef;
    xmlNodePtr *n;

    if (!node || !child)
        return -1;

    nodedef = __get_nodedef(node->type);

    if (nodedef->content != NC_XML)
        return -1;

    n = realloc(node->xml_childs, sizeof(*n)*(node->xml_childs_len + 1));
    if (!n)
        return -1;

    node->xml_childs = n;

    node->xml_childs[node->xml_childs_len] = xmlCopyNode(child, 1);
    if (node->xml_childs[node->xml_childs_len] == NULL)
        return -1;

    node->xml_childs_len++;

    return 0;
}

ssize_t                 igloo_reportxml_node_count_xml_child(igloo_reportxml_node_t *node)
{
    if (!node)
        return -1;

    return node->xml_childs_len;
}

xmlNodePtr              igloo_reportxml_node_get_xml_child(igloo_reportxml_node_t *node, size_t idx)
{
    xmlNodePtr ret;

    if (!node)
        return NULL;

    if (idx >= node->xml_childs_len)
        return NULL;

    ret = xmlCopyNode(node->xml_childs[idx], 1);

    return ret;
}

static void __database_free(igloo_ro_t self)
{
    igloo_reportxml_database_t *db = igloo_RO_TO_TYPE(self, igloo_reportxml_database_t);

    if (db->definitions)
        igloo_avl_tree_free(db->definitions, (igloo_avl_free_key_fun_type)igloo_ro_unref);

    igloo_thread_mutex_destroy(&(db->lock));
}

static int __compare_definitions(void *arg, void *a, void *b)
{
    char *id_a, *id_b;
    int ret = 0;

    id_a = igloo_reportxml_node_get_attribute(a, "defines");
    id_b = igloo_reportxml_node_get_attribute(b, "defines");

    if (!id_a || !id_b || id_a == id_b) {
        ret = 0;
    } else {
        ret = strcmp(id_a, id_b);
    }

    free(id_a);
    free(id_b);

    return ret;
}

static int  __database_new(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap)
{
    igloo_reportxml_database_t *ret = igloo_RO_TO_TYPE(self, igloo_reportxml_database_t);

    ret->definitions = igloo_avl_tree_new(__compare_definitions, NULL);
    if (!ret->definitions) {
        return -1;
    }

    igloo_thread_mutex_create(&(ret->lock));

    return 0;
}

int                     igloo_reportxml_database_add_report(igloo_reportxml_database_t *db, igloo_reportxml_t *report)
{
    igloo_reportxml_node_t *root;
    ssize_t count;
    size_t i;

    if (!db || !report)
        return -1;

    root = igloo_reportxml_get_root_node(report);
    if (!root)
        return -1;

    count = igloo_reportxml_node_count_child(root);
    if (count < 0)
        return -1;

    igloo_thread_mutex_lock(&(db->lock));

    for (i = 0; i < (size_t)count; i++) {
        igloo_reportxml_node_t *node = igloo_reportxml_node_get_child(root, i);
        igloo_reportxml_node_t *copy;

        if (igloo_reportxml_node_get_type(node) != igloo_REPORTXML_NODE_TYPE_DEFINITION) {
            igloo_ro_unref(node);
            continue;
        }

        copy = igloo_reportxml_node_copy(node);
        igloo_ro_unref(node);

        if (!copy)
            continue;

        igloo_avl_insert(db->definitions, copy);
    }

    igloo_thread_mutex_unlock(&(db->lock));

    igloo_ro_unref(root);

    return 0;
}

static int __attach_copy_of_node_or_definition(igloo_reportxml_node_t *parent, igloo_reportxml_node_t *node, igloo_reportxml_database_t *db, ssize_t depth)
{
    igloo_reportxml_node_t *copy;
    igloo_reportxml_node_t *def = NULL;
    char *definition;

    if (!parent || !node || !db)
        return -1;


    if (depth >= 2) {
        definition = igloo_reportxml_node_get_attribute(node, "definition");
        if (definition) {
            def = igloo_reportxml_database_build_node(db, definition, depth - 1);
            free(definition);
        }
    }

    if (def) {
        ssize_t count = igloo_reportxml_node_count_child(def);
        size_t i;

        if (count < 0) {
            igloo_ro_unref(def);
            return -1;
        }

        for (i = 0; i < (size_t)count; i++) {
            igloo_reportxml_node_t *child = igloo_reportxml_node_get_child(def, i);

            if (__attach_copy_of_node_or_definition(parent, child, db, depth - 1) != 0) {
                igloo_ro_unref(child);
                igloo_ro_unref(def);
                return -1;
            }

            igloo_ro_unref(child);
        }

        igloo_ro_unref(def);

        return 0;
    } else {
        int ret;

        copy = __reportxml_node_copy_with_db(node, db, depth - 1);
        if (!copy) {
            return -1;
        }

        ret = igloo_reportxml_node_add_child(parent, copy);

        igloo_ro_unref(copy);

        return ret;
    }
}

static igloo_reportxml_node_t *      __reportxml_database_build_node_ext(igloo_reportxml_database_t *db, const char *id, ssize_t depth, igloo_reportxml_node_type_t *acst_type_ret)
{
    igloo_reportxml_node_t *search;
    igloo_reportxml_node_t *found;
    igloo_reportxml_node_t *ret;
    enum {
        ACST_FIRST,
        ACST_YES,
        ACST_NO,
    } all_childs_same_type = ACST_FIRST;
    igloo_reportxml_node_type_t acst_type = igloo_REPORTXML_NODE_TYPE__ERROR;
    char *template;
    ssize_t count;
    size_t i;

    if (!db || !id)
        return NULL;

    /* Assign default depth in case it's set to -1 */
    if (depth < 0)
        depth = 8;

    if (!depth)
        return NULL;

    search = igloo_reportxml_node_new(igloo_REPORTXML_NODE_TYPE_DEFINITION, NULL, NULL, NULL);
    if (!search)
        return NULL;

    if (igloo_reportxml_node_set_attribute(search, "defines", id) != 0) {
        igloo_ro_unref(search);
        return NULL;
    }

    igloo_thread_mutex_lock(&(db->lock));
    if (igloo_avl_get_by_key(db->definitions, igloo_RO_TO_TYPE(search, void *), (void**)&found) != 0) {
        igloo_thread_mutex_unlock(&(db->lock));
        igloo_ro_unref(search);
        return NULL;
    }

    igloo_ro_unref(search);

    if (igloo_ro_ref(found) != 0) {
        igloo_thread_mutex_unlock(&(db->lock));
        return NULL;
    }
    igloo_thread_mutex_unlock(&(db->lock));

    count = igloo_reportxml_node_count_child(found);
    if (count < 0) {
        igloo_ro_unref(found);
        return NULL;
    }

    template = igloo_reportxml_node_get_attribute(found, "template");
    if (template) {
        igloo_reportxml_node_t *tpl = igloo_reportxml_database_build_node(db, template, depth - 1);

        free(template);

        if (tpl) {
            ret = igloo_reportxml_node_copy(tpl);
            igloo_ro_unref(tpl);
        } else {
            ret = NULL;
        }
    } else {
        ret = igloo_reportxml_node_new(igloo_REPORTXML_NODE_TYPE_DEFINITION, NULL, NULL, NULL);
    }

    if (!ret) {
        igloo_ro_unref(found);
        return NULL;
    }

    for (i = 0; i < (size_t)count; i++) {
        /* TODO: Look up definitions of our childs and childs' childs. */

        igloo_reportxml_node_t *node = igloo_reportxml_node_get_child(found, i);
        igloo_reportxml_node_type_t type = igloo_reportxml_node_get_type(node);

        switch (all_childs_same_type) {
            case ACST_FIRST:
                acst_type = type;
                all_childs_same_type = ACST_YES;
            break;
            case ACST_YES:
                if (acst_type != type)
                    all_childs_same_type = ACST_NO;
            break;
            case ACST_NO:
                /* noop */
            break;
        }

        /* We want depth, not depth - 1 here. __attach_copy_of_node_or_definition() takes care of this for us. */
        if (__attach_copy_of_node_or_definition(ret, node, db, depth) != 0) {
            igloo_ro_unref(node);
            igloo_ro_unref(found);
            igloo_ro_unref(ret);
            return NULL;
        }

        igloo_ro_unref(node);
    }

    igloo_ro_unref(found);

    if (all_childs_same_type == ACST_YES) {
        count = igloo_reportxml_node_count_child(ret);
        if (count < 0) {
            igloo_ro_unref(ret);
            return NULL;
        }

        for (i = 0; i < (size_t)count; i++) {
            igloo_reportxml_node_t *node = igloo_reportxml_node_get_child(ret, i);

            if (!node) {
                igloo_ro_unref(ret);
                return NULL;
            }

            if (igloo_reportxml_node_set_attribute(node, "_definition", id) != 0) {
                igloo_ro_unref(node);
                igloo_ro_unref(ret);
                return NULL;
            }

            igloo_ro_unref(node);
        }
    }

    if (acst_type_ret) {
        if (all_childs_same_type == ACST_YES) {
            *acst_type_ret = acst_type;
        } else {
            *acst_type_ret = igloo_REPORTXML_NODE_TYPE__ERROR;
        }
    }

    return ret;
}

igloo_reportxml_node_t *      igloo_reportxml_database_build_node(igloo_reportxml_database_t *db, const char *id, ssize_t depth)
{
    return __reportxml_database_build_node_ext(db, id, depth, NULL);
}

/* We try to build a a report from the definition. Exat structure depends on what is defined. */
igloo_reportxml_t *           igloo_reportxml_database_build_report(igloo_reportxml_database_t *db, const char *id, ssize_t depth)
{
    igloo_reportxml_node_t *definition;
    igloo_reportxml_node_t *child;
    igloo_reportxml_node_t *root;
    igloo_reportxml_node_t *attach_to;
    igloo_reportxml_node_type_t type;
    igloo_reportxml_t *ret;
    ssize_t count;
    size_t i;

    if (!db || !id)
        return NULL;

    /* first find the definition itself.  This will be some igloo_REPORTXML_NODE_TYPE_DEFINITION node. */
    definition = __reportxml_database_build_node_ext(db, id, depth, &type);
    if (!definition) {
        return NULL;
    }

    /* Let's see how many children we have. */
    count = igloo_reportxml_node_count_child(definition);
    if (count < 0) {
        igloo_ro_unref(definition);
        return NULL;
    } else if (count == 0) {
        /* Empty definition? Not exactly an exciting report... */
        igloo_ro_unref(definition);
        return igloo_ro_new(igloo_reportxml_t);
    }

    if (type == igloo_REPORTXML_NODE_TYPE__ERROR) {
        /* Now the hard part: find out what level we are. */
        child = igloo_reportxml_node_get_child(definition, 0);
        if (!child) {
            igloo_ro_unref(definition);
            return NULL;
        }

        type = igloo_reportxml_node_get_type(child);
        igloo_ro_unref(child);
    }

    /* check for supported configurations */
    switch (type) {
        case igloo_REPORTXML_NODE_TYPE_INCIDENT:
        case igloo_REPORTXML_NODE_TYPE_STATE:
        break;
        default:
            igloo_ro_unref(definition);
            return NULL;
        break;
    }

    ret = igloo_ro_new(igloo_reportxml_t);
    if (!ret) {
        igloo_ro_unref(definition);
        return NULL;
    }

    root = igloo_reportxml_get_root_node(ret);
    if (!ret) {
        igloo_ro_unref(definition);
        igloo_ro_unref(ret);
        return NULL;
    }

    if (type == igloo_REPORTXML_NODE_TYPE_INCIDENT) {
        igloo_ro_ref(attach_to = root);
    } else if (type == igloo_REPORTXML_NODE_TYPE_STATE) {
        attach_to = igloo_reportxml_node_new(igloo_REPORTXML_NODE_TYPE_INCIDENT, NULL, NULL, NULL);
        if (attach_to) {
            if (igloo_reportxml_node_add_child(root, attach_to) != 0) {
                igloo_ro_unref(attach_to);
                attach_to = NULL;
            }
        }
    } else {
        attach_to = NULL;
    }

    igloo_ro_unref(root);

    if (!attach_to) {
        igloo_ro_unref(definition);
        igloo_ro_unref(ret);
        return NULL;
    }

    /* now move nodes. */

    for (i = 0; i < (size_t)count; i++) {
        child = igloo_reportxml_node_get_child(definition, i);

        if (igloo_reportxml_node_get_type(child) == type) {
            /* Attach definition to all childs that are the same type.
             * As long as we work to-the-specs all childs are of the same type.
             * But if we work in relaxed mode, there might be other tags.
             */
            igloo_reportxml_node_set_attribute(child, "definition", id);
        }

        /* we can directly attach as it's a already a copy. */
        if (igloo_reportxml_node_add_child(attach_to, child) != 0) {
            igloo_ro_unref(definition);
            igloo_ro_unref(attach_to);
            igloo_ro_unref(ret);
            return NULL;
        }

        igloo_ro_unref(child);
    }

    igloo_ro_unref(definition);
    igloo_ro_unref(attach_to);

    return ret;
}
