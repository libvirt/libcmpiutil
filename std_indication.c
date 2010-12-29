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

void stdi_free_ind_args (struct ind_args **args)
{
        free((*args)->ns);
        free((*args)->classname);
        free(*args);
        *args = NULL;
}

static struct std_ind_filter *get_ind_filter(struct std_ind_filter **list,
                                             const char *ind_name)
{
        int i;
        struct std_ind_filter *filter = NULL;

        for (i = 0; list[i] != NULL; i++) {
                if (STREQC((list[i])->ind_name, ind_name)) {
                        filter = list[i];
                        break;
                }
        }
        
        if (filter == NULL)
                CU_DEBUG("get_ind_filter: failed to find %s", ind_name);

        return filter;
}

static bool is_ind_enabled(struct std_indication_ctx *ctx,
                           const char *ind_name,
                           CMPIStatus *s)
{
        bool ret = false;
        struct std_ind_filter *filter;

        if (!ctx->enabled) {
                CU_DEBUG("Indications disabled for this provider");
                ret = false;
                goto out;
        }

        filter = get_ind_filter(ctx->filters, ind_name);
        if (filter == NULL) {
                cu_statusf(ctx->brkr, s,
                           CMPI_RC_ERR_FAILED,
                           "No std_ind_filter for %s", ind_name);
                goto out;
        }
        
        ret = filter->active;
        if (!ret)
                CU_DEBUG("Indication '%s' not in active filter", ind_name);
 out:
        return ret;
}

static CMPIStatus trigger(struct std_indication_ctx *ctx,
                          const CMPIContext *context)
{
        if (ctx->handler == NULL || ctx->handler->trigger_fn == NULL)
                return (CMPIStatus){CMPI_RC_OK, NULL};

        return ctx->handler->trigger_fn(context);
}

static CMPIStatus default_raise(const CMPIBroker *broker,
                                const CMPIContext *context,
                                const CMPIObjectPath *ref,
                                CMPIInstance *ind)
{
        CMPIStatus s = {CMPI_RC_OK, NULL};

        s = CMSetObjectPath(ind, ref);
        if (s.rc != CMPI_RC_OK) 
                return s;

        CBDeliverIndication(broker,
                            context, 
                            NAMESPACE(ref), 
                            ind);
        return s;
}

static CMPIStatus raise(struct std_indication_ctx *ctx,
                        const CMPIContext *context,
                        const CMPIArgs *argsin,
                        const CMPIObjectPath *ref)
{
        bool enabled;
        CMPIInstance *inst;
        CMPIStatus s = {CMPI_RC_OK, NULL};
        const char *ind_name = NULL;

        CU_DEBUG("In raise");
        if (cu_get_inst_arg(argsin, "TheIndication", &inst) != CMPI_RC_OK) {
                cu_statusf(ctx->brkr, &s,
                           CMPI_RC_ERR_FAILED,
                           "Could not get indication to raise");
                goto out;
        }

        ind_name = cu_classname_from_inst(inst);
        if (ind_name == NULL) {
                cu_statusf(ctx->brkr, &s,
                           CMPI_RC_ERR_FAILED,
                           "Couldn't get indication name for enable check.");
                goto out;
        }

        CU_DEBUG("Indication is %s", ind_name);

        enabled = is_ind_enabled(ctx, ind_name, &s);
        if (s.rc != CMPI_RC_OK) {
                CU_DEBUG("Problem checking enabled: '%s'", CMGetCharPtr(s.msg));
                goto out;
        }

        if (!enabled)
                goto out;

        if (ctx->handler == NULL || ctx->handler->raise_fn == NULL)
                s = default_raise(ctx->brkr, context, ref, inst);
        else
                s = ctx->handler->raise_fn(ctx->brkr, context, ref, inst);

 out:
        return s;
}

CMPIStatus stdi_deliver(const CMPIBroker *broker,
                        const CMPIContext *ctx,
                        struct ind_args *args,
                        CMPIInstance *ind)
{
        bool enabled;
        const char *ind_name;
        CMPIStatus s = {CMPI_RC_OK, NULL};

        ind_name = cu_classname_from_inst(ind);
        if (ind_name == NULL) {
                cu_statusf(broker, &s,
                           CMPI_RC_ERR_FAILED,
                           "Couldn't get indication name for enable check.");
                goto out;
        }

        enabled = is_ind_enabled(args->_ctx, ind_name, &s);
        if (s.rc != CMPI_RC_OK) {
                CU_DEBUG("Problem checking enabled: '%s'", CMGetCharPtr(s.msg));
                goto out;
        }

        CU_DEBUG("Indication %s is%s enabled",
                 ind_name,
                 enabled ? "" : " not");

        if (enabled)
                s = CBDeliverIndication(broker, ctx, args->ns, ind);
        else
                cu_statusf(broker, &s,
                           CMPI_RC_ERR_METHOD_NOT_AVAILABLE,
                           "Indication not enabled");

 out:
        return s;
}

CMPIStatus stdi_set_ind_filter_state(struct std_indication_ctx *ctx,
                                     const char *ind_name,
                                     bool state)
{
        CMPIStatus s = {CMPI_RC_OK, NULL};
        struct std_ind_filter *filter;

        filter = get_ind_filter(ctx->filters, ind_name);
        if (filter == NULL) {
                cu_statusf(ctx->brkr, &s,
                           CMPI_RC_ERR_FAILED,
                           "Provider has no indication '%s'", ind_name);
                goto out;
        }
        
        filter->active = state;

 out:
        return s;
}

CMPIStatus stdi_activate_filter(CMPIIndicationMI* mi,
                                const CMPIContext* ctx,
                                const CMPISelectExp* se,
                                const char *ns,
                                const CMPIObjectPath* op,
                                CMPIBoolean first)
{
        CMPIStatus s = {CMPI_RC_OK, NULL};
        struct std_indication_ctx *_ctx;
        const char *cn = NULL;
        
        _ctx = (struct std_indication_ctx *)mi->hdl;
        cn = CLASSNAME(op);
        s = stdi_set_ind_filter_state(_ctx, cn, true);

        if (_ctx->handler != NULL && _ctx->handler->activate_fn != NULL) {
                CU_DEBUG("Calling handler->activate_fn");
                s = _ctx->handler->activate_fn(mi, ctx, se, ns, op, first);
                goto out;
        }

 out:
        return s;
}

CMPIStatus stdi_deactivate_filter(CMPIIndicationMI* mi,
                                  const CMPIContext* ctx,
                                  const CMPISelectExp* se,
                                  const  char *ns,
                                  const CMPIObjectPath* op,
                                  CMPIBoolean last)
{
        CMPIStatus s = {CMPI_RC_OK, NULL};
        struct std_indication_ctx *_ctx;
        const char *cn = NULL;

        _ctx = (struct std_indication_ctx *)mi->hdl;
        cn = CLASSNAME(op);
        s = stdi_set_ind_filter_state(_ctx, cn, false);

        if (_ctx->handler != NULL && _ctx->handler->deactivate_fn != NULL) {
                s = _ctx->handler->deactivate_fn(mi, ctx, se, ns, op, last);
                goto out;
        }

 out:
        return s;
}

_EI_RTYPE stdi_enable_indications (CMPIIndicationMI* mi,
                                   const CMPIContext *ctx)
{
        struct std_indication_ctx *_ctx;
        _ctx = (struct std_indication_ctx *)mi->hdl;

        CU_DEBUG("%s: indications enabled", mi->ft->miName);
        _ctx->enabled = true;

        if (_ctx->handler != NULL && _ctx->handler->enable_fn != NULL)
                return _ctx->handler->enable_fn(mi, ctx);

        _EI_RET();
}

_EI_RTYPE stdi_disable_indications (CMPIIndicationMI* mi,
                                    const CMPIContext *ctx)
{
        struct std_indication_ctx *_ctx;
        _ctx = (struct std_indication_ctx *)mi->hdl;

        CU_DEBUG("%s: indications disabled", mi->ft->miName);
        _ctx->enabled = false;

        if (_ctx->handler != NULL && _ctx->handler->disable_fn != NULL)
                return _ctx->handler->disable_fn(mi, ctx);

        _EI_RET();
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
                s = raise(ctx, context, argsin, reference);
        else
                cu_statusf(ctx->brkr, &s,
                           CMPI_RC_ERR_FAILED,
                           "Invalid method");

        CMReturnDone(results);
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
        if ((op == NULL) || (s.rc != CMPI_RC_OK)) {
                CU_DEBUG("Unable to create path for indication %s",
                         type);
                cu_statusf(broker, &s,
                           CMPI_RC_ERR_FAILED,
                           "Unable to create path for indication %s",
                           type);
                return s;
        }

        argsin = CMNewArgs(broker, &s);
        if (s.rc != CMPI_RC_OK)
                return s;

        argsout = CMNewArgs(broker, &s);
        if (s.rc != CMPI_RC_OK)
                return s;

        s = CMAddArg(argsin, "TheIndication", &ind, CMPI_instance);
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

