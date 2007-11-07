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
                                 CMPIInstance ** const list)
{
        unsigned int i;

        if (list == NULL)
                return 0;

        for (i = 0; list[i] != NULL; i++)
                CMReturnInstance(results, list[i]);

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
                                      CMPIInstance ** const list)
{
        unsigned int i;
        unsigned int c = 0;

        if (list == NULL)
                return 0;

        for (i = 0; list[i] != NULL; i++)
                if (cu_return_instance_name(results, list[i]))
                        c++;

        return c;
}

static bool _compare_data(const CMPIData *a, const CMPIData *b)
{
        if (a->type != b->type)
                return false;

        if (a->type & CMPI_string) {
                const char *as = CMGetCharPtr(a->value.string);
                const char *bs = CMGetCharPtr(b->value.string);

                return STREQ(as, bs);
        } else if (a->type & CMPI_INTEGER) {
                return memcmp(&a->value, &b->value, sizeof(a->value)) == 0;
        }

        CU_DEBUG("Unhandled CMPI type: `%i'", a->type);

        return false;
}

const char *cu_compare_ref(const CMPIObjectPath *ref,
                           const CMPIInstance *inst)
{
        int i;
        CMPIStatus s;
        int count;
        const char *prop = NULL;

        count = CMGetKeyCount(ref, &s);
        if (s.rc != CMPI_RC_OK) {
                CU_DEBUG("Unable to get key count");
                return NULL;
        }

        for (i = 0; i < count; i++) {
                CMPIData kd, pd;
                CMPIString *str;

                kd = CMGetKeyAt(ref, i, &str, &s);
                if (s.rc != CMPI_RC_OK) {
                        CU_DEBUG("Failed to get key %i", i);
                        goto out;
                }

                prop = CMGetCharPtr(str);
                CU_DEBUG("Comparing key `%s'", prop);

                pd = CMGetProperty(inst, prop, &s);
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

/*
 * Local Variables:
 * mode: C
 * c-set-style: "K&R"
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * End:
 */
