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
#ifndef __STD_INDICATION_H
#define __STD_INDICATION_H

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>
#include <stdio.h>

#include "config.h"

#include "libcmpiutil.h"
#include "std_invokemethod.h"

#ifdef CMPI_EI_VOID
# define _EI_RTYPE void
# define _EI_RET() return
#else
# define _EI_RTYPE CMPIStatus
# define _EI_RET() return (CMPIStatus){CMPI_RC_OK, NULL}
#endif

typedef CMPIStatus (*raise_indication_t)(const CMPIBroker *broker,
                                         const CMPIContext *ctx,
                                         const CMPIInstance *ind);

typedef CMPIStatus (*trigger_indication_t)(const CMPIContext *ctx);

typedef CMPIStatus (*activate_function_t) (CMPIIndicationMI* mi,
                                           const CMPIContext* ctx,
                                           const CMPISelectExp* se,
                                           const char *ns,
                                           const CMPIObjectPath* op,
                                           CMPIBoolean first);

typedef CMPIStatus (*deactivate_function_t) (CMPIIndicationMI* mi,
                                             const CMPIContext* ctx,
                                             const CMPISelectExp* se,
                                             const  char *ns,
                                             const CMPIObjectPath* op,
                                             CMPIBoolean last);

typedef _EI_RTYPE (*enable_function_t) (CMPIIndicationMI* mi,
                                        const CMPIContext *ctx);

typedef _EI_RTYPE (*disable_function_t) (CMPIIndicationMI* mi,
                                         const CMPIContext *ctx);

struct std_indication_handler {
        raise_indication_t raise_fn;
        trigger_indication_t trigger_fn;
        activate_function_t activate_fn;
        deactivate_function_t deactivate_fn;
        enable_function_t enable_fn;
        disable_function_t disable_fn;
};

struct std_ind_filter {
        char *ind_name;
        bool active;
};

struct std_indication_ctx {
        const CMPIBroker *brkr;
        struct std_indication_handler *handler;
        struct std_ind_filter **filters;
        bool enabled;
};

struct ind_args {
        CMPIContext *context;
        char *ns;
        char *classname;
        struct std_indication_ctx *_ctx;
};

void stdi_free_ind_args (struct ind_args **args);

CMPIStatus stdi_trigger_indication(const CMPIBroker *broker,
                                   const CMPIContext *context,
                                   const char *type,
                                   const char *ns);

CMPIStatus stdi_raise_indication(const CMPIBroker *broker,
                                 const CMPIContext *context,
                                 const char *type,
                                 const char *ns,
                                 const CMPIInstance *ind);

CMPIStatus stdi_deliver(const CMPIBroker *broker,
                        const CMPIContext *ctx,
                        struct ind_args *args,
                        CMPIInstance *ind);

CMPIStatus stdi_activate_filter(CMPIIndicationMI* mi,
                                const CMPIContext* ctx,
                                const CMPISelectExp* se,
                                const char *ns,
                                const CMPIObjectPath* op,
                                CMPIBoolean first);

CMPIStatus stdi_deactivate_filter(CMPIIndicationMI* mi,
                                  const CMPIContext* ctx,
                                  const CMPISelectExp* se,
                                  const  char *ns,
                                  const CMPIObjectPath* op,
                                  CMPIBoolean last);

_EI_RTYPE stdi_enable_indications (CMPIIndicationMI* mi,
                                   const CMPIContext *ctx);

_EI_RTYPE stdi_disable_indications (CMPIIndicationMI* mi,
                                    const CMPIContext *ctx);

CMPIStatus stdi_handler(CMPIMethodMI *self,
                        const CMPIContext *context,
                        const CMPIResult *results,
                        const CMPIObjectPath *reference,
                        const char *methodname,
                        const CMPIArgs *argsin,
                        CMPIArgs *argsout);

CMPIStatus stdi_cleanup(CMPIMethodMI *self,
                        const CMPIContext *context,
                        CMPIBoolean terminating);

CMPIStatus stdi_set_ind_filter_state(struct std_indication_ctx *ctx,
                                     const char *ind_name,
                                     bool state);

/* This doesn't work, but should be made to. */
#define DECLARE_FILTER(ident, name)                     \
        static struct std_ind_filter ident = {          \
                .ind_name = name,                       \
                .active = false,                        \
        };                                              \

#define STDI_IndicationMIStub(pfx, pn, _broker, hook, _handler, filters)\
        static struct std_indication_ctx pn##_ctx = {                   \
                .brkr = NULL,                                           \
                .handler = _handler,                                    \
                .filters = filters,                                     \
                .enabled = false,                                       \
        };                                                              \
                                                                        \
        static CMPIIndicationMIFT indMIFT__ = {                         \
                CMPICurrentVersion,                                     \
                CMPICurrentVersion,                                     \
                "Indication" #pn,                                       \
                pfx##IndicationCleanup,                                 \
                pfx##AuthorizeFilter,                                   \
                pfx##MustPoll,                                          \
                stdi_activate_filter,                                   \
                stdi_deactivate_filter,                                 \
                stdi_enable_indications,                                \
                stdi_disable_indications,                               \
        };                                                              \
        CMPIIndicationMI *                                              \
        pn##_Create_IndicationMI(const CMPIBroker *,                    \
                                 const CMPIContext *,                   \
                                 CMPIStatus *);                         \
        CMPIIndicationMI *                                              \
        pn##_Create_IndicationMI(const CMPIBroker *brkr,                \
                                  const CMPIContext *ctx,               \
                                  CMPIStatus *rc) {                     \
                static CMPIIndicationMI mi = {                          \
                        &pn##_ctx,                                      \
                        &indMIFT__,                                     \
                };                                                      \
                pn##_ctx.brkr = brkr;                                   \
                _broker = brkr;                                         \
                hook;                                                   \
                return &mi;                                             \
        }                                                               \
                                                                        \
        static CMPIMethodMIFT methMIFT__ = {                            \
                CMPICurrentVersion,                                     \
                CMPICurrentVersion,                                     \
                "method" #pn,                                           \
                stdi_cleanup,                                           \
                stdi_handler,                                           \
        };                                                              \
        CMPI_EXTERN_C                                                   \
        CMPIMethodMI *pn##_Create_MethodMI(const CMPIBroker *,          \
                                           const CMPIContext *,         \
                                           CMPIStatus *rc);             \
        CMPI_EXTERN_C                                                   \
        CMPIMethodMI *pn##_Create_MethodMI(const CMPIBroker *brkr,      \
                                           const CMPIContext *ctx,      \
                                           CMPIStatus *rc) {            \
                static CMPIMethodMI mi = {                              \
                        &pn##_ctx,                                      \
                        &methMIFT__,                                    \
                };                                                      \
                pn##_ctx.brkr = brkr;                                   \
                _broker = brkr;                                         \
                hook;                                                   \
                return &mi;                                             \
        }

#endif

/*
 * Local Variables:
 * mode: C
 * c-set-style: "K&R"
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * End:
 */

