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
#include <string.h>

#include "libcmpiutil.h"
#include "std_invokemethod.h"

#define STREQ(a,b) (strcmp(a, b) == 0)

static CMPIType check_for_eo(CMPIType type, CMPIType exp_type)
{
     if ((exp_type == CMPI_instance) && (type == CMPI_string)) {
             return CMPI_instance;
     }

     if ((exp_type == CMPI_instanceA) && (type == CMPI_stringA)) {
             return CMPI_instanceA;
     }

     return CMPI_null;
}

static int parse_eo_inst_arg(CMPIString *string_in,
                             CMPIInstance **instance_out,
                             const CMPIBroker *broker,
                             const char *ns,
                             CMPIStatus *s)
{
        int ret;
        const char *str;

        str = CMGetCharPtr(string_in);

        if (str == NULL) {
                cu_statusf(broker, s,
                           CMPI_RC_ERR_INVALID_PARAMETER,
                           "Method parameter value is NULL");
                return 0;
        }

        ret = cu_parse_embedded_instance(str,
                                         broker,
                                         ns,
                                         instance_out);

        /* cu_parse_embedded_instance() returns 0 on success */
        if ((ret != 0) || CMIsNullObject(instance_out)) {
                cu_statusf(broker, s,
                           CMPI_RC_ERR_FAILED,
                           "Unable to parse embedded object");
                return 0;
        }

        return 1;
}

static int parse_eo_array(CMPIArray *strings_in,
                          CMPIArray **instances_out,
                          const CMPIBroker *broker,
                          const char *ns,
                          CMPIStatus *s)
{
        int i;
        int ret;
        int count;

        if (CMIsNullObject(strings_in)) {
                cu_statusf(broker, s,
                           CMPI_RC_ERR_INVALID_PARAMETER,
                           "Method parameter is NULL");
                return 0;
        }

        count = CMGetArrayCount(strings_in, NULL);

        *instances_out = CMNewArray(broker,
                                    count,
                                    CMPI_instance,
                                    s);

        for (i = 0; i < count; i++) {
                CMPIData item;
                CMPIInstance *inst;

                item = CMGetArrayElementAt(strings_in, i, NULL);

                ret = parse_eo_inst_arg(item.value.string,
                                        &inst,
                                        broker,
                                        ns,
                                        s);

                if (ret != 1) {
                        CU_DEBUG("Parsing argument type %d failed", item.type);
                        return 0;
                }

                CMSetArrayElementAt(*instances_out, i,
                                    (CMPIValue *)&inst,
                                    CMPI_instance);
        }

        return 1;
}

static int parse_eo_param(CMPIArgs *args,
                          CMPIData data,
                          CMPIType type,
                          const char *arg_name,
                          const CMPIBroker *broker,
                          const char *ns,
                          CMPIStatus *s)
{
        CMPIData newdata;
        int ret = 0;

        if (type == CMPI_instance) {
                ret = parse_eo_inst_arg(data.value.string,
                                        &newdata.value.inst,
                                        broker,
                                        ns,
                                        s);
        } else if (type == CMPI_instanceA) {
                ret = parse_eo_array(data.value.array,
                                     &newdata.value.array,
                                     broker,
                                     ns,
                                     s);
        } else
                cu_statusf(broker, s,
                           CMPI_RC_ERR_FAILED,
                           "Unable to parse argument type %d",
                           type);

        if (ret != 1)
                return 0;


        *s = CMAddArg(args,
                      arg_name,
                      &(newdata.value),
                      type);

        if (s->rc != CMPI_RC_OK) {
                CU_DEBUG("Unable to update method argument");
                return 0;
        }

        return 1;
}

static int validate_arg_type(struct method_arg *arg,
                             const CMPIArgs *args,
                             const CMPIBroker *broker,
                             const char *ns,
                             CMPIArgs *new_args,
                             CMPIStatus *s)
{
        CMPIData argdata;
        CMPIType type;
        int ret;

        argdata = CMGetArg(args, arg->name, s);
        if (((s->rc != CMPI_RC_OK) || (CMIsNullValue(argdata)))
             && !arg->optional) {
                CMSetStatus(s, CMPI_RC_ERR_INVALID_PARAMETER);
                CU_DEBUG("Method parameter `%s' missing",
                         arg->name);
                return 0;
        }

        if (argdata.type != arg->type) {
                type = check_for_eo(argdata.type, arg->type);
                if (type != CMPI_null) {
                        ret = parse_eo_param(new_args,
                                             argdata,
                                             type,
                                             arg->name,
                                             broker,
                                             ns,
                                             s);

                        if (ret != 1)
                                return 0;
                } else if (!arg->optional) {
                        CMSetStatus(s, CMPI_RC_ERR_TYPE_MISMATCH);
                        CU_DEBUG("Method parameter `%s' type check failed",
                                 arg->name);
                        return 0;
                } else
                        CU_DEBUG("No optional parameter supplied for `%s'",
                                 arg->name);
        } else {
                *s = CMAddArg(new_args,
                              arg->name,
                              &(argdata.value),
                              argdata.type);

                if (s->rc != CMPI_RC_OK) {
                        CU_DEBUG("Unable to update method argument");
                        return 0;
                }
        }

        CU_DEBUG("Method parameter `%s' validated type 0x%x",
                 arg->name,
                 arg->type);

        return 1;
}

static int validate_args(struct method_handler *h,
                         const CMPIArgs **args,
                         const CMPIObjectPath *ref,
                         const CMPIBroker *broker,
                         CMPIStatus *s)
{
        CMPIArgs *new_args;
        int i;

        new_args = CMNewArgs(broker, s);

        for (i = 0; h->args[i].name; i++) {
                int ret;
                struct method_arg *arg = &h->args[i];

                ret = validate_arg_type(arg,
                                        *args,
                                        broker,
                                        NAMESPACE(ref),
                                        new_args,
                                        s);
                if (!ret)
                        return 0;
        }

        *args = new_args;

        return 1;
}

CMPIStatus _std_invokemethod(CMPIMethodMI *self,
                             const CMPIContext *context,
                             const CMPIResult *results,
                             const CMPIObjectPath *reference,
                             const char *methodname,
                             const CMPIArgs *argsin,
                             CMPIArgs *argsout)
{
        struct std_invokemethod_ctx *ctx = self->hdl;
        struct method_handler *h = NULL;
        int hi;
        int ret;
        CMPIStatus s;

        CU_DEBUG("Method `%s' execution attempted", methodname);

        for (hi = 0; ctx->handlers[hi]; hi++) {
                if (STREQ(methodname, ctx->handlers[hi]->name)) {
                        h = ctx->handlers[hi];
                        break;
                }
        }

        if (h == NULL) {
                cu_statusf(ctx->broker, &s,
                           CMPI_RC_ERR_FAILED,
                           "Unknown Method");
                goto exit;
        }

        ret = validate_args(h,
                            &argsin,
                            reference,
                            ctx->broker,
                            &s);
        if (!ret)
                goto exit;

        CU_DEBUG("Executing handler for method `%s'", methodname);
        s = h->handler(self, context, results, reference, argsin, argsout);
        CU_DEBUG("Method `%s' returned %i", methodname, s.rc);
 exit:
        CMReturnDone(results);

        return s;
}

CMPIStatus _std_cleanup(CMPIMethodMI *self,
                        const CMPIContext *context,
                        CMPIBoolean terminating)
{
        CMPIStatus s = {CMPI_RC_OK, NULL};

        return s;
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

