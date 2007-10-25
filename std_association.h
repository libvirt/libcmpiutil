/*
 * Copyright IBM Corp. 2007
 *
 * Authors:
 *  Dan Smith <danms@us.ibm.com>
 *  Kaitlin Rupert <karupert@us.ibm.com>
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
#ifndef __STD_ASSOCIATION_H
#define __STD_ASSOCIATION_H

#include <cmpidt.h>

struct std_assoc;
struct std_assoc_info;

typedef CMPIStatus (*assoc_handler_t)(const CMPIObjectPath *ref,
                                      struct std_assoc_info *info,
                                      struct inst_list *list);

typedef CMPIInstance *(*make_ref_t)(const CMPIObjectPath *,
                                    const CMPIInstance *,
                                    struct std_assoc_info *info,
                                    struct std_assoc *);

struct std_assoc {
        char *source_class;
        char *source_prop;

        char *target_class;
        char *target_prop;
    
        char *assoc_class;

        assoc_handler_t handler;
        make_ref_t make_ref;
};

struct std_assoc_info {
        const char *assoc_class;
        const char *result_class;
        const char *role;
        const char *result_role;
        const char **properties;
        const CMPIContext *context;
        const char *provider_name;
};

struct std_assoc_ctx {
        const CMPIBroker *brkr;
        struct std_assoc **handlers;
};

void set_reference(struct std_assoc *assoc,
                   CMPIInstance *inst,
                   const CMPIObjectPath *source,
                   const CMPIObjectPath *target);

CMPIStatus stda_AssociationCleanup(CMPIAssociationMI *self,
                                   const CMPIContext *context,
                                   CMPIBoolean terminating);

CMPIStatus stda_AssociatorNames(CMPIAssociationMI *self,
                                const CMPIContext *context,
                                const CMPIResult *results,
                                const CMPIObjectPath *reference,
                                const char *assocClass,
                                const char *resultClass,
                                const char *role,
                                const char *resultRole);

CMPIStatus stda_Associators(CMPIAssociationMI *self,
                            const CMPIContext *context,
                            const CMPIResult *results,
                            const CMPIObjectPath *reference,
                            const char *assocClass,
                            const char *resultClass,
                            const char *role,
                            const char *resultRole,
                            const char **properties);

CMPIStatus stda_ReferenceNames(CMPIAssociationMI *self,
                               const CMPIContext *context,
                               const CMPIResult *results,
                               const CMPIObjectPath *reference,
                               const char *resultClass,
                               const char *role);

CMPIStatus stda_References(CMPIAssociationMI *self,
                           const CMPIContext *context,
                           const CMPIResult *results,
                           const CMPIObjectPath *reference,
                           const char *resultClass,
                           const char *role,
                           const char **properties);

#define STDA_AssocMIStub(pfx, pn, _broker, hook, funcs)                 \
        static CMPIAssociationMIFT pn##assocMIFT__ = {                  \
                CMPICurrentVersion,                                     \
                CMPICurrentVersion,                                     \
                "association" #pn,                                      \
                stda_AssociationCleanup,                                \
                stda_Associators,                                       \
                stda_AssociatorNames,                                   \
                stda_References,                                        \
                stda_ReferenceNames                                     \
        };                                                              \
                                                                        \
        CMPIAssociationMI *pn##_Create_AssociationMI(const CMPIBroker *, \
                                                     const CMPIContext *, \
                                                     CMPIStatus *);     \
                                                                        \
        CMPI_EXTERN_C                                                   \
        CMPIAssociationMI *pn##_Create_AssociationMI(const CMPIBroker *brkr, \
                                                     const CMPIContext *ctx, \
                                                     CMPIStatus *rc)    \
        {                                                               \
                static CMPIAssociationMI mi;                            \
                static struct std_assoc_ctx _ctx;                       \
                _ctx.brkr = brkr;                                       \
                _ctx.handlers = (struct std_assoc **)funcs;             \
                mi.hdl = (void *)&_ctx;                                 \
                mi.ft = &pn##assocMIFT__;                               \
                _broker = brkr;                                         \
                hook;                                                   \
                return &mi;                                             \
        }                                                               \

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
