/*
 * Copyright IBM Corp. 2007
 *
 * Authors:
 *  Dan Smith <danms@us.ibm.com>
 *  Zhengang Li <lizg@cn.ibm.com>
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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>

#include "libcmpiutil.h"

#define CU_WEAK_TYPES 1

CMPIrc cu_get_str_path(const CMPIObjectPath *reference,
                       const char *key,
                       const char **val)
{
        CMPIData data;
        CMPIStatus s;
        const char *value;
        
        data = CMGetKey(reference, key, &s);
        if ((s.rc != CMPI_RC_OK) || 
            CMIsNullValue(data) || 
            CMIsNullObject(data.value.string))
                return CMPI_RC_ERR_FAILED;

        value = CMGetCharPtr(data.value.string);
        if ((value == NULL) || (*value == '\0'))
                return CMPI_RC_ERR_TYPE_MISMATCH;

        *val = value;

        return CMPI_RC_OK;
}

CMPIrc cu_get_u16_path(const CMPIObjectPath *reference,
                       const char *key,
                       uint16_t *target)
{
        CMPIData data;
        CMPIStatus s;

        data = CMGetKey(reference, key, &s);
        if ((s.rc != CMPI_RC_OK) ||
            CMIsNullValue(data))
                return CMPI_RC_ERR_FAILED;

        *target = data.value.uint16;

        return CMPI_RC_OK;
}

const char *cu_check_args(const CMPIArgs *args, const char **names)
{
        int i;

        for (i = 0; names[i]; i++) {
                const char *argname = names[i];
                CMPIData argdata;
                CMPIStatus s;

                argdata = CMGetArg(args, argname, &s);
                if ((s.rc != CMPI_RC_OK) || CMIsNullValue(argdata))
                        return argname;
        }

        return NULL;
}

CMPIrc cu_get_str_arg(const CMPIArgs *args, const char *name, const char **val)
{
        CMPIData argdata;
        CMPIStatus s;

        argdata = CMGetArg(args, name, &s);
        if ((s.rc != CMPI_RC_OK) || (CMIsNullValue(argdata)))
                return CMPI_RC_ERR_INVALID_PARAMETER;

        if ((argdata.type != CMPI_string) || 
            CMIsNullObject(argdata.value.string))
                return CMPI_RC_ERR_TYPE_MISMATCH;

        *val = CMGetCharPtr(argdata.value.string);

        return CMPI_RC_OK;
}

CMPIrc cu_get_ref_arg(const CMPIArgs *args,
                      const char *name,
                      CMPIObjectPath **op)
{
        CMPIData argdata;
        CMPIStatus s;

        argdata = CMGetArg(args, name, &s);
        if ((s.rc != CMPI_RC_OK) || (CMIsNullValue(argdata)))
                return CMPI_RC_ERR_INVALID_PARAMETER;

        if ((argdata.type != CMPI_ref) || CMIsNullObject(argdata.value.ref))
                return CMPI_RC_ERR_TYPE_MISMATCH;

        *op = argdata.value.ref;

        return CMPI_RC_OK;
}

CMPIrc cu_get_inst_arg(const CMPIArgs *args,
                       const char *name,
                       CMPIInstance **inst)
{
        CMPIData argdata;
        CMPIStatus s;

        argdata = CMGetArg(args, name, &s);
        if ((s.rc != CMPI_RC_OK) || (CMIsNullValue(argdata)))
                return CMPI_RC_ERR_INVALID_PARAMETER;

        if ((argdata.type != CMPI_instance) ||
            CMIsNullObject(argdata.value.inst))
                CMPI_RC_ERR_TYPE_MISMATCH;

        *inst = argdata.value.inst;

        return CMPI_RC_OK;
}

CMPIrc cu_get_array_arg(const CMPIArgs *args,
                        const char *name,
                        CMPIArray **array)
{
        CMPIData argdata;
        CMPIStatus s;
        
        argdata = CMGetArg(args, name, &s);
        if ((s.rc != CMPI_RC_OK) || CMIsNullValue(argdata))
                return CMPI_RC_ERR_INVALID_PARAMETER;

        if (!CMIsArray(argdata) || CMIsNullObject(argdata.value.array))
                return CMPI_RC_ERR_TYPE_MISMATCH;

        *array = argdata.value.array;

        return CMPI_RC_OK;
}

CMPIrc cu_get_u16_arg(const CMPIArgs *args, const char *name, uint16_t *target)
{
        CMPIData argdata;
        CMPIStatus s;

        argdata = CMGetArg(args, name, &s);
        if ((s.rc != CMPI_RC_OK) || CMIsNullValue(argdata))
                return CMPI_RC_ERR_INVALID_PARAMETER;

#ifdef CU_WEAK_TYPES
        if (!(argdata.type & CMPI_INTEGER))
#else
        if (argdata.type != CMPI_uint16)
#endif
                return CMPI_RC_ERR_TYPE_MISMATCH;

        *target = argdata.value.uint16;

        return CMPI_RC_OK;
}

#define REQUIRE_PROPERTY_DEFINED(i, p, pv, s)                           \
        if (i == NULL || p == NULL)                                     \
                return CMPI_RC_ERR_NO_SUCH_PROPERTY;                    \
        pv = CMGetProperty(i, p, s);                                    \
        if ((s)->rc != CMPI_RC_OK || CMIsNullValue(pv))                 \
                return CMPI_RC_ERR_NO_SUCH_PROPERTY;

CMPIrc cu_get_array_prop(const CMPIInstance *inst,
                         const char *prop,
                         CMPIArray **array)
{
        CMPIData value;
        CMPIStatus s;

        REQUIRE_PROPERTY_DEFINED(inst, prop, value, &s);

        value = CMGetProperty(inst, prop, &s);
        if ((s.rc != CMPI_RC_OK) || CMIsNullValue(value))
                return s.rc;

        if (!CMIsArray(value) || CMIsNullObject(value.value.array))
                return CMPI_RC_ERR_TYPE_MISMATCH;

        *array = value.value.array;

        return CMPI_RC_OK;
}

CMPIrc cu_get_str_prop(const CMPIInstance *inst,
                       const char *prop,
                       const char **target)
{
        CMPIData value;
        CMPIStatus s;
        const char *prop_val;

        *target = NULL;
        
        REQUIRE_PROPERTY_DEFINED(inst, prop, value, &s);

        if (value.type != CMPI_string)
                return CMPI_RC_ERR_TYPE_MISMATCH;

        if ((prop_val = CMGetCharPtr(value.value.string)) == NULL)
                return CMPI_RC_ERROR;

        *target = prop_val;

        return CMPI_RC_OK;
}

CMPIrc cu_get_bool_prop(const CMPIInstance *inst,
                        const char *prop,
                        bool *target)
{
        CMPIData value;
        CMPIStatus s;

        REQUIRE_PROPERTY_DEFINED(inst, prop, value, &s);

        if (value.type != CMPI_boolean)
                return CMPI_RC_ERR_TYPE_MISMATCH;

        *target = (bool)value.value.boolean;

        return CMPI_RC_OK;
}

CMPIrc cu_get_u16_prop(const CMPIInstance *inst,
                       const char *prop,
                       uint16_t *target)
{
        CMPIData value;
        CMPIStatus s;

        REQUIRE_PROPERTY_DEFINED(inst, prop, value, &s);

#ifdef CU_WEAK_TYPES
        if (!(value.type & CMPI_INTEGER))
#else
        if (value.type != CMPI_uint16)
#endif
                return CMPI_RC_ERR_TYPE_MISMATCH;

        *target = value.value.uint16;

        return CMPI_RC_OK;
}

CMPIrc cu_get_u32_prop(const CMPIInstance *inst,
                       const char *prop,
                       uint32_t *target)
{
        CMPIData value;
        CMPIStatus s;

        REQUIRE_PROPERTY_DEFINED(inst, prop, value, &s);

#ifdef CU_WEAK_TYPES
        if (!(value.type & CMPI_INTEGER))
#else
        if (value.type != CMPI_uint32)
#endif
                return CMPI_RC_ERR_TYPE_MISMATCH;

        *target = value.value.uint32;

        return CMPI_RC_OK;
}

CMPIrc cu_get_u64_prop(const CMPIInstance *inst,
                       const char *prop,
                       uint64_t *target)
{
        CMPIData value;
        CMPIStatus s;

        REQUIRE_PROPERTY_DEFINED(inst, prop, value, &s);

#ifdef CU_WEAK_TYPES
        if (!(value.type & CMPI_INTEGER))
#else
        if (value.type != CMPI_uint64)
#endif
                return CMPI_RC_ERR_TYPE_MISMATCH;

        *target = value.value.uint64;

        return CMPI_RC_OK;
}

int cu_statusf(const CMPIBroker *broker,
               CMPIStatus *s,
               CMPIrc rc,
               char *fmt, ...)
{
        va_list ap;
        char *msg = NULL;
        int ret;

        va_start(ap, fmt);
        ret = vasprintf(&msg, fmt, ap);
        va_end(ap);

        if (ret != -1) {
                CMSetStatusWithChars(broker, s, rc, msg);
                free(msg);
        } else {
                CMSetStatus(s, rc);
        }

        return ret;
}

CMPIType cu_prop_type(const CMPIInstance *inst, const char *prop)
{
        CMPIData value;
        CMPIStatus s;

        value = CMGetProperty(inst, prop, &s);
        if ((s.rc != CMPI_RC_OK) || (CMIsNullValue(value)))
                return CMPI_null;

        return value.type;
}

CMPIType cu_arg_type(const CMPIArgs *args, const char *arg)
{
        CMPIData value;
        CMPIStatus s;

        value = CMGetArg(args, arg, &s);
        if ((s.rc != CMPI_RC_OK) || (CMIsNullValue(value)))
                return CMPI_null;

        return value.type;
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
