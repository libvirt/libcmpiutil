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

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include "libcmpiutil.h"
#include "std_invokemethod.h"

#define STREQ(a,b) (strcmp(a, b) == 0)

static int validate_arg_type(struct method_arg *arg, 
                             const CMPIArgs *args,
                             CMPIStatus *s)
{
        CMPIData argdata;

        argdata = CMGetArg(args, arg->name, s);
        if ((s->rc != CMPI_RC_OK) || (CMIsNullValue(argdata))) {
                CMSetStatus(s, CMPI_RC_ERR_INVALID_PARAMETER);
                CU_DEBUG("Method parameter `%s' missing",
                         arg->name);
                return 0;
        }

        if (argdata.type != arg->type) {
                CMSetStatus(s, CMPI_RC_ERR_TYPE_MISMATCH);
                CU_DEBUG("Method parameter `%s' type check failed",
                         arg->name);
                return 0;
        }

        CU_DEBUG("Method parameter `%s' validated type 0x%x",
                 arg->name,
                 arg->type);

        return 1;
}

static int validate_args(struct method_handler *h, 
                         const CMPIArgs *args,
                         CMPIStatus *s)
{
        int i;

        for (i = 0; h->args[i].name; i++) {
                int ret;
                struct method_arg *arg = &h->args[i];

                ret = validate_arg_type(arg, args, s);
                if (!ret)
                        return 0;
        }

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

        for (hi = 0; ctx->handlers[hi]->name; hi++) {
                if (STREQ(methodname, ctx->handlers[hi]->name)) {
                        h = ctx->handlers[hi];
                        break;
                }
        }

        if (h == NULL) {
                CMSetStatusWithChars(ctx->broker, &s,
                                     CMPI_RC_ERR_FAILED,
                                     "Unknown Method");
                goto exit;
        }

        ret = validate_args(h, argsin, &s);
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

