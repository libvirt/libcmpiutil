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
#ifndef __STD_INVOKEMETHOD_H
#define __STD_INVOKEMETHOD_H

#include <cmpidt.h>
#include <cmpift.h>

typedef CMPIStatus (*method_handler_fn)(CMPIMethodMI *self,
                                        const CMPIContext *context,
                                        const CMPIResult *results,
                                        const CMPIObjectPath *reference,
                                        const CMPIArgs *argsin,
                                        CMPIArgs *argsout);

#define ARG_END {NULL, 0}

enum {IM_RC_OK,
      IM_RC_NOT_SUPPORTED,
      IM_RC_FAILED,
      IM_RC_TIMED_OUT,
      IM_RC_SYS_NOT_FOUND,
      IM_RC_SYS_NOT_DESTROYABLE,
      IM_RC_ASYNC=4096};

struct method_arg {
        char *name;
        CMPIType type;
        bool optional;
};

struct method_handler {
        char *name;
        method_handler_fn handler;
        struct method_arg args[];
};

CMPIStatus _std_invokemethod(CMPIMethodMI *self,
                             const CMPIContext *context,
                             const CMPIResult *results,
                             const CMPIObjectPath *reference,
                             const char *methodname,
                             const CMPIArgs *argsin,
                             CMPIArgs *argsout);

CMPIStatus _std_cleanup(CMPIMethodMI *self,
                        const CMPIContext *context,
                        CMPIBoolean terminating);


struct std_invokemethod_ctx {
        const CMPIBroker *broker;
        struct method_handler **handlers;
};

#define STDIM_MethodMIStub(pfx, pn, _broker, hook, funcs)    \
  static CMPIMethodMIFT methMIFT__ = {                       \
    CMPICurrentVersion,                                      \
    CMPICurrentVersion,                                      \
    "method" #pn,                                            \
    _std_cleanup,                                            \
    _std_invokemethod,                                       \
  };                                                         \
                                                             \
  /* Function prototype, to avoid warnings */                \
  CMPIMethodMI *pn##_Create_MethodMI(const CMPIBroker *,     \
                                     const CMPIContext *,    \
                                     CMPIStatus *);          \
                                                             \
  CMPI_EXTERN_C                                              \
  CMPIMethodMI *pn##_Create_MethodMI(const CMPIBroker *brkr, \
                                     const CMPIContext *ctx, \
                                     CMPIStatus *rc) {       \
    static CMPIMethodMI mi;                                  \
    static struct std_invokemethod_ctx _ctx;                 \
    _ctx.broker = brkr;                                      \
    _ctx.handlers = (struct method_handler **)funcs;         \
    mi.hdl = (void *)&_ctx;                                  \
    mi.ft = &methMIFT__;                                     \
    _broker = brkr;                                          \
    hook;                                                    \
    return &mi;                                              \
  }

#endif

/*
  Example usage:

  static CMPIStatus test_foo(CMPIMethodMI *self,
                             CMPIContext *context,
                             CMPIResult *results,
                             CMPIObjectPath *reference,
                             CMPIArgs *argsin,
                             CMPIArgs *argsout)
  {
          char *arg = cu_get_str_arg(argsin, "myarg");

          printf("Myarg: %s\n", arg);

          return (CMPIStatus){CMPI_RC_OK, NULL};
  }

  static struct method_handler my_func = {
          .name = "MyFunc",
          .handler = test_foo,
          .args = {{"myarg", CMPI_string},
                   ARG_END
          }
  };

  struct method_handler my_handlers[] = {
          &my_func,
          NULL,
  };

  STDIM_MethodMIStub(, Foo_TestMethodProvider, _BROKER, CMNoHook, my_handlers);

*/

/*
 * Local Variables:
 * mode: C
 * c-set-style: "K&R"
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * End:
 */

