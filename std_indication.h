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

#include "libcmpiutil.h"
#include "std_invokemethod.h"

CMPIStatus stdi_trigger_indication(const CMPIBroker *broker,
                                   const CMPIContext *context,
                                   const char *type,
                                   const char *ns);

CMPIStatus stdi_raise_indication(const CMPIBroker *broker,
                                 const CMPIContext *context,
                                 const char *type,
                                 const char *ns,
                                 const CMPIInstance *ind);

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

typedef CMPIStatus (*raise_indication_t)(const CMPIBroker *broker,
                                         const CMPIContext *ctx,
                                         const CMPIInstance *ind);

typedef CMPIStatus (*trigger_indication_t)(const CMPIContext *ctx);

struct std_indication_handler {
        raise_indication_t raise_fn;
        trigger_indication_t trigger_fn;
};

struct std_indication_ctx {
        const CMPIBroker *brkr;
        struct std_indication_handler *handler;
        bool enabled;
};

#define STDI_IndicationMIStub(pfx, pn, _broker, hook, _handler)         \
        static struct std_indication_ctx _ctx = {                       \
                .brkr = NULL,                                           \
                .handler = _handler,                                    \
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
                pfx##ActivateFilter,                                    \
                pfx##DeActivateFilter,                                  \
                CMIndicationMIStubExtensions(pfx)                       \
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
                        &_ctx,                                          \
                        &indMIFT__,                                     \
                };                                                      \
                _ctx.brkr = brkr;                                       \
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
                        &_ctx,                                          \
                        &methMIFT__,                                    \
                };                                                      \
                _ctx.brkr = brkr;                                       \
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

