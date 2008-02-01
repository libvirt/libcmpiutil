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
#include <string.h>
#include <stdbool.h>

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>

#include <libcmpiutil.h>

#define STREQ(a, b) (strcmp(a, b) == 0)
#define STREQC(a, b) (strcasecmp(a, b) == 0)

#include "std_indication.h"

static CMPIStatus trigger(struct std_indication_ctx *ctx,
                          const CMPIContext *context)
{
        if (ctx->handler->trigger_fn == NULL)
                return (CMPIStatus){CMPI_RC_OK, NULL};

        return ctx->handler->trigger_fn(context);
}

static CMPIStatus default_raise(const CMPIBroker *broker,
                                const CMPIContext *context,
                                CMPIInstance *ind)
{
        CMPIObjectPath *ref;
        CMPIStatus s = {CMPI_RC_OK, NULL};

        ref = CMGetObjectPath(ind, NULL);
        
        CBDeliverIndication(broker,
                            context, 
                            NAMESPACE(ref), 
                            ind);
        return s;
}

static CMPIStatus raise(struct std_indication_ctx *ctx,
                        const CMPIContext *context,
                        const CMPIArgs *argsin)
{
        CMPIInstance *inst;

        if (!ctx->enabled) {
                CU_DEBUG("Indication disabled, not raising.");
                return (CMPIStatus) {CMPI_RC_OK, NULL};
        }

        if (cu_get_inst_arg(argsin, "Indication", &inst) != CMPI_RC_OK)
                return (CMPIStatus){CMPI_RC_ERR_FAILED, NULL};

        if (ctx->handler->raise_fn == NULL)
                return default_raise(ctx->brkr, context, inst);

        return ctx->handler->raise_fn(ctx->brkr, context, inst);
}

CMPIStatus stdi_handler(CMPIMethodMI *self,
                        const CMPIContext *context,
                        const CMPIResult *results,
                        const CMPIObjectPath *reference,
                        const char *methodname,
                        const CMPIArgs *argsin,
                        CMPIArgs *argsout)
{
        CMPIStatus s;
        struct std_indication_ctx *ctx = self->hdl;

        if (STREQC(methodname, "TriggerIndications"))
                s = trigger(ctx, context);
        else if (STREQ(methodname, "RaiseIndication"))
                s = raise(ctx, context, argsin);
        else
                cu_statusf(ctx->brkr, &s,
                           CMPI_RC_ERR_FAILED,
                           "Invalid method");

        return s;
}

CMPIStatus stdi_cleanup(CMPIMethodMI *self,
                        const CMPIContext *context,
                        CMPIBoolean terminating)
{
        RETURN_UNSUPPORTED();
}

CMPIStatus stdi_trigger_indication(const CMPIBroker *broker,
                                   const CMPIContext *context,
                                   const char *type,
                                   const char *ns)
{
        CMPIObjectPath *op;
        CMPIStatus s;
        const char *method = "TriggerIndications";
        CMPIArgs *in;
        CMPIArgs *out;

        in = CMNewArgs(broker, &s);
        if (s.rc != CMPI_RC_OK)
                return s;

        out = CMNewArgs(broker, &s);
        if (s.rc != CMPI_RC_OK)
                return s;

        op = CMNewObjectPath(broker, ns, type, &s);
        if ((op == NULL) || (s.rc != CMPI_RC_OK)) {
                CU_DEBUG("Unable to create path for indication %s",
                         type);
                cu_statusf(broker, &s,
                           CMPI_RC_ERR_FAILED,
                           "Unable to create path for indication %s",
                           type);
                goto out;
        }

        CBInvokeMethod(broker, context, op, method, in, out, &s);
 out:
        return s;
}

CMPIStatus stdi_raise_indication(const CMPIBroker *broker,
                                 const CMPIContext *context,
                                 const char *type,
                                 const char *ns,
                                 const CMPIInstance *ind)
{
        CMPIObjectPath *op;
        CMPIStatus s;
        const char *method = "RaiseIndication";
        CMPIArgs *argsin;
        CMPIArgs *argsout;

        op = CMNewObjectPath(broker, ns, type, &s);
        if (s.rc != CMPI_RC_OK)
                return s;

        argsin = CMNewArgs(broker, &s);
        if (s.rc != CMPI_RC_OK)
                return s;

        argsout = CMNewArgs(broker, &s);
        if (s.rc != CMPI_RC_OK)
                return s;

        s = CMAddArg(argsin, "Indication", &ind, CMPI_instance);
        if (s.rc != CMPI_RC_OK)
                return s;

        CBInvokeMethod(broker, context, op, method, argsin, argsout, &s);

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

