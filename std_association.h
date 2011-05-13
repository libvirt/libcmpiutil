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
#include <cmpift.h>

struct std_assoc;
struct std_assoc_info;

typedef CMPIStatus (*assoc_handler_t)(const CMPIObjectPath *ref,
                                      struct std_assoc_info *info,
                                      struct inst_list *list);

typedef CMPIInstance *(*make_ref_t)(const CMPIObjectPath *ref,
                                    const CMPIInstance *inst,
                                    struct std_assoc_info *info,
                                    struct std_assoc *assoc);

/*
 * std_assoc is the definition that the developer puts in their source file. It
 * defines an association relationship between a set of source and target
 * classes, through a named (set of) association classes and the function
 * handler which does the work.
 * It must be registered using the macro STDA_AssocMIStub.
 */
struct std_assoc {
        /* Defines the list of possible classes that can be passed to the
           association for this case */
        char **source_class;


        /* Defines the property of the association class that refers
           to the input (source class) of this case. This must match
           that of the schema, and is used for automatic generation of
           the reference object in the References() or ReferenceNames()
           operation */
        char *source_prop;

        /* Defines a list of possible classes that can be returned by the
           association for a given source_class list */
        char **target_class;

        /* Same as source_prop, applied for target */
        char *target_prop;

        /* Defines the list of association classes which are implemented by
           this handler */
        char **assoc_class;

        /* Function handler responsible for doing the association and
           returning the list of target instances of the association.
           The handler function receives the reference of the source
           class of the association and must map it to a list of
           CMPIInstance objects (targets of the association). */
        assoc_handler_t handler;

        /* Function handler responsible for creating an instance of the
           association class.
           The handler function receives the source object path,
           and the target instance, so it can create the reference which is
           returned by the function. */
        make_ref_t make_ref;
};

/*
 * The std_assoc_info is used to hold the query components of the query done
 * All members of this structure contain the corresonding formal CIM association
 * query components.
 */
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
