/*
 * Copyright IBM Corp. 2007
 *
 * Authors:
 *  Dan Smith <danms@us.ibm.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>

#include "libcmpiutil.h"

#include "eo_parser_xml.h"

static char *get_attr(xmlNode *node, const char *name)
{
        return (char *)xmlGetProp(node, (xmlChar *)name);
}

static char *get_content(xmlNode *node)
{
        return (char *)xmlNodeGetContent(node);
}

static CMPIType parse_string_property(const CMPIBroker *broker,
                                      const char *string,
                                      const char *type,
                                      CMPIValue *val)
{
        CMPIString *str;
        CMPIStatus s;

        str = CMNewString(broker, string, &s);
        if ((str == NULL) || (s.rc != CMPI_RC_OK))
                return CMPI_null;

        val->string = str;
        return CMPI_string;
}

static CMPIType parse_bool_property(const char *string,
                                    const char *type,
                                    CMPIValue *val)
{
        val->boolean = STREQC(string, "true");

        return CMPI_boolean;
}

static CMPIType parse_int_property(const char *string,
                                   const char *type,
                                   CMPIValue *val)
{
        int size;
        bool sign;
        CMPIType t;
        int ret;

        if (sscanf(type, "uint%i", &size) == 1)
                sign = false;
        else if (sscanf(type, "int%i", &size) == 1)
                sign = true;
        else {
                CU_DEBUG("Unknown integer type: `%s'", type);
                return CMPI_null;
        }

        if (sign) {
                int64_t _val;
                ret = sscanf(string, "%" SCNi64, &_val);
                val->sint64 = _val;
        } else {
                uint64_t _val;
                ret = sscanf(string, "%" SCNu64, &_val);
                val->uint64 = _val;
        }

        if (ret != 1) {
                CU_DEBUG("Failed to scan value `%s'\n", string);
                return CMPI_null;
        }

        switch (size) {
        case 8:  t = sign ? CMPI_sint8  : CMPI_uint8;  break;
        case 16: t = sign ? CMPI_sint16 : CMPI_uint16; break;
        case 32: t = sign ? CMPI_sint32 : CMPI_uint32; break;
        default:
        case 64: t = sign ? CMPI_sint64 : CMPI_uint64; break;
        };

        return t;
}

static CMPIType get_property_value(const CMPIBroker *broker,
                                   const char *tstr,
                                   const char *content,
                                   CMPIValue *value)
{
        CMPIType type = CMPI_null;

        if (STREQC(tstr, "string"))
                type = parse_string_property(broker, content, tstr, value);
        else if (STREQC(tstr, "boolean"))
                type = parse_bool_property(content, tstr, value);
        else if (strstr(tstr, "int"))
                type = parse_int_property(content, tstr, value);
        else {
                CU_DEBUG("Unhandled type: %s\n", tstr);
                goto out;
        }

        if (type == CMPI_null) {
                CU_DEBUG("Unable to parse type %s\n", tstr);
                goto out;
        }

 out:
        return type;
}

static CMPIType get_node_value(const CMPIBroker *broker,
                               xmlNode *node,
                               const char *tstr,
                               CMPIValue *val)
{
        CMPIType type = CMPI_null;
        char *content = NULL;

        if (node->type != XML_ELEMENT_NODE) {
                CU_DEBUG("Non-element node: %s", node->name);
                goto out;
        }

        if (!STREQC((char *)node->name, "value") &&
            (node->ns == NULL)) {
                CU_DEBUG("Expected <VALUE> but got <%s>", node->name);
                goto out;
        }

        content = get_content(node);
        CU_DEBUG("Node content: %s", content);
        type = get_property_value(broker, tstr, content, val);
        free(content);
 out:
        return type;
}

static CMPIType parse_array(const CMPIBroker *broker,
                            const char *tstr,
                            xmlNode *root,
                            CMPIArray **array)
{
        xmlNode *value;
        CMPIValue *list = NULL;
        int size = 0;
        int cur = 0;
        CMPIStatus s;
        CMPIType type = CMPI_null;
        int i;

        for (value = root->children; value; value = value->next) {

                if (value->type != XML_ELEMENT_NODE)
                        continue;

                if (cur == size) {
                        CMPIValue *tmp;

                        size *= 2;
                        tmp = realloc(list, sizeof(CMPIValue) * size);
                        if (tmp == NULL) {
                                CU_DEBUG("Failed to alloc %i",
                                         sizeof(CMPIValue) * size);
                                goto out;
                        }

                        list = tmp;
                }

                type = get_node_value(broker, value, tstr, &list[cur]);
                if (type == CMPI_null) {
                        CU_DEBUG("Got nothing from child");
                } else {
                        CU_DEBUG("Array value type %i", type);
                        cur++;
                }
        }

        if (cur == 0)
                return CMPI_null;

        *array = CMNewArray(broker, cur, type, &s);
        if ((*array == NULL) || (s.rc != CMPI_RC_OK)) {
                CU_DEBUG("Failed to alloc CMPIArray of %i", cur);
                goto out;
        }

        for (i = 0; i < cur; i++)
                CMSetArrayElementAt((*array), i, &list[i], type);

 out:
        free(list);

        return type;
}

static bool parse_array_property(const CMPIBroker *broker,
                                 xmlNode *node,
                                 CMPIInstance *inst)
{
        char *name = NULL;
        char *tstr = NULL;
        bool ret = false;
        xmlNode *val_arr;
        CMPIArray *array;
        CMPIType type;

        name = get_attr(node, "NAME");
        if (name == NULL) {
                CU_DEBUG("Unnamed property\n");
                goto out;
        }

        tstr = get_attr(node, "TYPE");
        if (tstr == NULL) {
                CU_DEBUG("No type\n");
                goto out;
        }

        CU_DEBUG("Array property `%s' of type `%s'\n", name, tstr);

        for (val_arr = node->children; val_arr; val_arr = val_arr->next) {
                if (val_arr->type != XML_ELEMENT_NODE)
                        continue;

                if (!STREQC((char *)val_arr->name, "value.array")) {
                        CU_DEBUG("Expected <value.array> but got <%s>",
                               val_arr->name);
                        val_arr = NULL;
                        goto out;
                }

                break;
        }

        if (val_arr != NULL) {
                type = parse_array(broker, tstr, val_arr, &array);
                if (type != CMPI_null) {
                        CU_DEBUG("Setting array property");
                        CMSetProperty(inst, name, &array, (CMPI_ARRAY | type));
                }
        }

 out:
        free(name);
        free(tstr);

        return ret;
}

static bool parse_property(const CMPIBroker *broker,
                           xmlNode *node,
                           CMPIInstance *inst)
{
        char *name = NULL;
        char *tstr = NULL;
        CMPIValue value;
        CMPIType type = CMPI_null;
        xmlNode *ptr;

        name = get_attr(node, "NAME");
        if (name == NULL) {
                CU_DEBUG("Unnamed property\n");
                goto out;
        }

        tstr = get_attr(node, "TYPE");
        if (tstr == NULL) {
                CU_DEBUG("No type\n");
                goto out;
        }

        CU_DEBUG("Property %s: %s", name, tstr);

        for (ptr = node->children; ptr; ptr = ptr->next) {
                type = get_node_value(broker, ptr, tstr, &value);
                if (type != CMPI_null) {
                        CMSetProperty(inst, name, &value, type);
                        break;
                }
        }
 out:
        free(name);
        free(tstr);

        return type != CMPI_null;
}

static CMPIStatus parse_instance(const CMPIBroker *broker,
                                 const char *ns,
                                 xmlNode *root,
                                 CMPIInstance **inst)
{
        char *class = NULL;
        xmlNode *child;
        CMPIStatus s = {CMPI_RC_OK, NULL};
        CMPIObjectPath *op;

        if (root->type != XML_ELEMENT_NODE) {
                CU_DEBUG("First node is not <INSTANCE>");
                cu_statusf(broker, &s,
                           CMPI_RC_ERR_FAILED,
                           "First node of object is not <INSTANCE");
                goto out;
        }

        if (!STREQC((char *)root->name, "instance")) {
                CU_DEBUG("Got node %s, expecting INSTANCE", root->name);
                cu_statusf(broker, &s,
                           CMPI_RC_ERR_FAILED,
                           "First node of object is not <INSTANCE");
                goto out;
        }

        class = get_attr(root, "CLASSNAME");
        if (class == NULL) {
                CU_DEBUG("No classname in object");
                cu_statusf(broker, &s,
                           CMPI_RC_ERR_FAILED,
                           "Missing CLASSNAME attribute of INSTANCE");
                goto out;
        }

        CU_DEBUG("Instance of %s", class);

        op = CMNewObjectPath(broker, ns, class, &s);
        if ((op == NULL) || (s.rc != CMPI_RC_OK)) {
                CU_DEBUG("Unable to create path for %s:%s", ns, class);
                cu_statusf(broker, &s,
                           CMPI_RC_ERR_FAILED,
                           "Unable to create path for %s:%s", ns, class);
                goto out;
        }

        *inst = CMNewInstance(broker, op, &s);
        if ((*inst == NULL) || (s.rc != CMPI_RC_OK)) {
                CU_DEBUG("Unable to create inst for %s:%s", ns, class);
                cu_statusf(broker, &s,
                           CMPI_RC_ERR_FAILED,
                           "Unable to create instance for %s:%s", ns, class);
                goto out;
        }

        for (child = root->children; child; child = child->next) {
                if ((child->type == XML_ELEMENT_NODE) &&
                    (child->ns == NULL)) {
                        if (STREQC((char *)child->name, "property"))
                                parse_property(broker, child, *inst);
                        else if (STREQC((char *)child->name, "property.array"))
                                parse_array_property(broker, child, *inst);
                        else
                                CU_DEBUG("Unexpected node: %s\n", child->name);
                }
        }

 out:
        free(class);

        return s;
}

#if 0

/* This isn't currently used but I think I might need it for nested
 * instances, depending on how they look.
 */

static char *parse_esc(const char *start, char *result)
{
        char *delim;
        char *escape = NULL;
        int len = 1;

        delim = strchr(start, ';');
        if (delim == NULL)
                goto out;

        escape = strndup(start, delim - start + 1);
        if (escape == NULL) {
                CU_DEBUG("Memory alloc failed (%i)", delim-start);
                return NULL;
        }

        CU_DEBUG("Escape is: %s", escape);

        if (STREQC(escape, "&lt;"))
                *result = '<';
        else if (STREQC(escape, "&gt;"))
                *result = '>';
        else if (STREQC(escape, "&quot;"))
                *result = '\"';
        else if (STREQC(escape, "&amp;"))
                *result = '&';
        else if (STREQC(escape, "&apos;"))
                *result = '\'';
        else
                CU_DEBUG("Unhandled escape: `%s'", escape);

        len = strlen(escape);

 out:
        free(escape);

        return (char *)start + len;
}

static char *xml_decode(const char *input)
{
        const char *iptr = input;
        char *optr;
        char *res = NULL;

        res = malloc(strlen(input) + 1);
        if (res == NULL)
                return res;

        optr = res;

        while ((iptr != NULL) && (*iptr != '\0')) {
                if (*iptr == '&') {
                        iptr = parse_esc(iptr, optr);
                } else {
                        *optr = *iptr;
                        iptr++;
                }

                optr++;
        }

        return res;
}

#endif

int cu_parse_ei_xml(const CMPIBroker *broker,
                    const char *ns,
                    const char *xml,
                    CMPIInstance **instance)
{
        xmlDoc *doc = NULL;
        xmlNode *root = NULL;
        int ret = 1;
        CMPIStatus s;

        doc = xmlReadMemory(xml,
                            strlen(xml),
                            NULL,
                            NULL,
                            (XML_PARSE_NOENT |
                             XML_PARSE_NOCDATA |
                             XML_PARSE_NONET));
        if (doc == NULL) {
                CU_DEBUG("Error reading decoded XML from memory");
                goto out;
        }

        root = xmlDocGetRootElement(doc);
        if (root == NULL) {
                CU_DEBUG("Error getting root XML node");
                goto out;
        }

        s = parse_instance(broker, ns, root, instance);
        if (s.rc != CMPI_RC_OK) {
                *instance = NULL;
                CU_DEBUG("CIMXML EI Parsing failed: %s",
                         CMGetCharPtr(s.msg));
                goto out;
        }

        ret = 0;

 out:
        xmlFreeDoc(doc);

        return ret;
}

/*
 * Local Variables:
 * mode: C
 * c-set-style: "K&R"
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * End:
 */
