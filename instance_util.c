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
#include <stdbool.h>
#include <string.h>

#include "libcmpiutil.h"

unsigned int cu_return_instances(const CMPIResult *results,
                                 const struct inst_list *list)
{
        unsigned int i;

        if (list == NULL)
                return 0;

        for (i = 0; i < list->cur; i++)
                CMReturnInstance(results, list->list[i]);

        return i;
}

bool cu_return_instance_name(const CMPIResult *results,
                             const CMPIInstance *inst)

{
        CMPIObjectPath *op;
        CMPIStatus s;

        op = CMGetObjectPath(inst, &s);
        if ((s.rc != CMPI_RC_OK) || CMIsNullObject(op))
                return false;

        CMReturnObjectPath(results, op);

        return true;
}

unsigned int cu_return_instance_names(const CMPIResult *results,
                                      const struct inst_list *list)
{
        unsigned int i;

        if (list == NULL)
                return 0;

        for (i = 0; i < list->cur; i++)
                cu_return_instance_name(results, list->list[i]);

        return i;
}

static bool _compare_data(const CMPIData *a, const CMPIData *b)
{
        if (a->type != b->type)
                return false;

        if (a->type & CMPI_string) {
                const char *as = CMGetCharPtr(a->value.string);
                const char *bs = CMGetCharPtr(b->value.string);

                return STREQC(as, bs);
        } else if (a->type & CMPI_INTEGER) {
                return memcmp(&a->value, &b->value, sizeof(a->value)) == 0;
        }

        CU_DEBUG("Unhandled CMPI type: `%i'", a->type);

        return false;
}

static bool _compare_classname(const CMPIObjectPath *ref,
                               const CMPIObjectPath *op)
{
        const char *ref_cn;
        const char *op_cn;
        
        ref_cn = CLASSNAME(ref);
        if (ref_cn == NULL)
                return false;
        
        op_cn = CLASSNAME(op);
        if (op_cn == NULL)
                return false;
        
        return STREQC(op_cn, ref_cn);
}

const char *cu_compare_ref(const CMPIObjectPath *ref,
                           const CMPIInstance *inst)
{
        CMPIStatus s = {CMPI_RC_OK, NULL};
        CMPIObjectPath *op; 
        const char *prop = NULL;
        int i;
        int count;

        op = CMGetObjectPath(inst, &s);
        if ((op == NULL) || (s.rc != CMPI_RC_OK))
                return NULL;

        if (!_compare_classname(ref, op))
                return "CreationClassName";
        
        count = CMGetKeyCount(op, &s);
        if (s.rc != CMPI_RC_OK) {
                CU_DEBUG("Unable to get key count");
                return NULL;
        }
        CU_DEBUG("Number of keys: %i", count);
        
        for (i = 0; i < count; i++) {
                CMPIData kd, pd;
                CMPIString *str;

                kd = CMGetKeyAt(op, i, &str, &s);
                if (s.rc != CMPI_RC_OK) {
                        CU_DEBUG("Failed to get key %i", i);
                        goto out;
                }

                prop = CMGetCharPtr(str);
                CU_DEBUG("Comparing key %i: `%s'", i, prop);

                pd = CMGetKey(ref, prop, &s);
                if (s.rc != CMPI_RC_OK) {
                        CU_DEBUG("Failed to get property `%s'", prop);
                        goto out;
                }

                if (!_compare_data(&kd, &pd)) {
                        CU_DEBUG("No data match for `%s'", prop);
                        goto out;
                }
        }

        prop = NULL;
 out:
        return prop;
}

CMPIStatus cu_validate_ref(const CMPIBroker *broker,
                           const CMPIObjectPath *ref,
                           const CMPIInstance *inst)
{
        CMPIStatus s = {CMPI_RC_OK, NULL};
        const char *prop;
        
        prop = cu_compare_ref(ref, inst);
        if (prop != NULL) {
                cu_statusf(broker, &s,
                           CMPI_RC_ERR_NOT_FOUND,
                           "No such instance (%s)", prop);
        }
        
        return s;
}

CMPIStatus cu_copy_prop(const CMPIBroker *broker,
                        CMPIInstance *src_inst, CMPIInstance *dest_inst,
                        char *src_name, char *dest_name)
{
        CMPIData data;
        CMPIStatus s = {CMPI_RC_OK, NULL};

        if (src_name == NULL) {
                cu_statusf(broker, &s, 
                           CMPI_RC_ERR_FAILED,
                           "No property name given");
                goto out;
        }

        if (dest_name == NULL)
                dest_name = src_name;

        data = CMGetProperty(src_inst, src_name, &s);
        if (s.rc != CMPI_RC_OK || CMIsNullValue(data)) {
                cu_statusf(broker, &s, 
                           CMPI_RC_ERR_FAILED,
                           "Copy failed.  Could not get prop '%s'.", src_name);
                goto out;
        }

        CMSetProperty(dest_inst, dest_name, &(data.value), data.type);

 out:
        return s;        
}

CMPIInstance *cu_dup_instance(const CMPIBroker *broker,
                              CMPIInstance *src,
                              CMPIStatus *s)
{
        int i;
        int prop_count;
        CMPIData data;
        CMPIObjectPath *ref;
        CMPIInstance *dest = NULL;

        ref = CMGetObjectPath(src, NULL);
        if ((s->rc != CMPI_RC_OK) || CMIsNullObject(ref)) {
                cu_statusf(broker, s,
                           CMPI_RC_ERR_FAILED,
                           "Could not get objectpath from instance");
                goto out;
        }
     
        dest = CMNewInstance(broker, ref, s);

        prop_count = CMGetPropertyCount(src, s);
        if (s->rc != CMPI_RC_OK) {
                cu_statusf(broker, s,
                           CMPI_RC_ERR_FAILED,
                           "Could not get property count for copy");
                goto out;
        }

        for (i = 0; i < prop_count; i++) {
                CMPIString *prop;
                const char *prop_name;

                data = CMGetPropertyAt(src, i, &prop, s);
                prop_name = CMGetCharPtr(prop);
                if (s->rc != CMPI_RC_OK) {
                        goto out;
                }
                
                *s = CMSetProperty(dest, prop_name, 
                                   &(data.value), data.type);
                if (s->rc != CMPI_RC_OK) {
                        goto out;
                }
        }

 out:
        return dest;
}

const char *cu_classname_from_inst(CMPIInstance *inst)
{
        const char *ret = NULL;

        CMPIObjectPath *ref;
        ref = CMGetObjectPath(inst, NULL);
        if (CMIsNullObject(ref))
                goto out;
        
        ret = CLASSNAME(ref);

 out:
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
