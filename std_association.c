/*
 * Copyright IBM Corp. 2007
 *
 * Authors:
 *  Dan Smith <danms@us.ibm.com>
 *  Jay Gagnon <grendel@linux.vnet.ibm.com>
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
#include <string.h>

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>

#include "libcmpiutil.h"

#include "std_association.h"

#define CLASSNAME(op) (CMGetCharPtr(CMGetClassName(op, NULL)))
#define NAMESPACE(op) (CMGetCharPtr(CMGetNameSpace(op, NULL)))

#define STREQ(a, b) (strcmp(a, b) == 0)
#define STREQC(a, b) (strcasecmp(a, b) == 0)

void set_reference(struct std_assoc *assoc,
                   CMPIInstance *inst,
                   const CMPIObjectPath *source,
                   const CMPIObjectPath *target)
{
        CMSetProperty(inst, assoc->source_prop,
                      (CMPIValue *)&source, CMPI_ref);
        CMSetProperty(inst, assoc->target_prop,
                      (CMPIValue *)&target, CMPI_ref);
}

static bool match_op(const CMPIBroker *broker,
                     CMPIObjectPath *op,
                     const char *filter_class)
{
        if ((filter_class == NULL) ||
            CMClassPathIsA(broker, op, filter_class, NULL))
                return true;
        else
                return false;
}

static bool match_class(const CMPIBroker *broker,
                        const char *ns,
                        const char *test_class,
                        const char *comp_class)
{
        CMPIObjectPath *rop;

        rop = CMNewObjectPath(broker, ns, test_class, NULL);

        if ((test_class == NULL) ||
            (comp_class == NULL) ||
            match_op(broker, rop, comp_class))
                return true;
        else
                return false;
}

static CMPIStatus filter_results(struct inst_list *list,
                                 const char *ns,
                                 const char *filter_class,
                                 const CMPIBroker *broker)
{
        struct inst_list tmp_list;
        CMPIStatus s = {CMPI_RC_OK, NULL};
        CMPIObjectPath *op;
        int i;

        tmp_list = *list;
        inst_list_init(list);

        for (i = 0; tmp_list.list[i] != NULL; i++) {
                op = CMGetObjectPath(tmp_list.list[i], &s);
                if ((s.rc != CMPI_RC_OK) || CMIsNullObject(op))
                          goto out;

                s = CMSetNameSpace(op, ns);
                if (s.rc != CMPI_RC_OK)
                          goto out;

                if (!match_op(broker, op, filter_class))
                          continue;

                inst_list_add(list, tmp_list.list[i]);
        }

out:
        inst_list_free(&tmp_list);

        return s;
}

static struct std_assoc *
std_assoc_get_handler(const struct std_assoc_ctx *ctx,
                      const CMPIObjectPath *ref)
{
        struct std_assoc *ptr;
        int i;

        for (i = 0; ctx->handlers[i]; i++) {
                ptr = ctx->handlers[i];

                if (CMClassPathIsA(ctx->brkr, ref, ptr->source_class, NULL))
                        return ptr;
        }

        return NULL;
}

static CMPIStatus do_assoc(struct std_assoc_ctx *ctx,
                           struct std_assoc_info *info,
                           const CMPIResult *results,
                           const CMPIObjectPath *ref,
                           bool names_only)
{
        struct inst_list list;
        CMPIStatus s;
        struct std_assoc *handler;
        bool rc;

        inst_list_init(&list);

        CU_DEBUG("Getting handler...");

        handler = std_assoc_get_handler(ctx, ref);
        if (handler == NULL) {
                CU_DEBUG("No handler found.");
                cu_statusf(ctx->brkr, &s,
                           CMPI_RC_ERR_FAILED,
                           "Unable to handle this association");
                goto out;
        }

        CU_DEBUG("OK.\n\tsource: '%s'\n\ttarget: '%s'\n\tassoc_class: '%s'",
              handler->source_class, 
              handler->target_class, 
              handler->assoc_class);
        CU_DEBUG("Calling match_class: \n\tinfo->result_class: '%s'",
              info->result_class);

        rc = match_class(ctx->brkr, 
                        NAMESPACE(ref), 
                        info->result_class, 
                        handler->target_class);
        if (!rc) {
                CU_DEBUG("Match_class failed.");
                cu_statusf(ctx->brkr, &s,
                           CMPI_RC_ERR_FAILED,
                           "Result class is not valid for this association");
                goto out;
        }
        CU_DEBUG("Match_class succeeded.");

        CU_DEBUG("Calling match_class: \n\tinfo->assoc_class: '%s'",
              info->assoc_class);
        rc = match_class(ctx->brkr, 
                        NAMESPACE(ref), 
                        info->assoc_class, 
                        handler->assoc_class);
        if (!rc) {
                CU_DEBUG("Match_class failed.");
                cu_statusf(ctx->brkr, &s,
                           CMPI_RC_ERR_FAILED,
                           "Association class given is not valid for"
                           "this association");
                goto out;
        }
        CU_DEBUG("Match_class succeeded, calling handler->handler...");

        s = handler->handler(ref, info, &list);
        
        if (s.rc != CMPI_RC_OK) {
                CU_DEBUG("Handler did not return CMPI_RC_OK.");
                goto out;
        } else {
                CU_DEBUG("Handler returned CMPI_RC_OK.");
        }

        if (list.list == NULL) {
                CU_DEBUG("List is empty.");
                goto out;
        }

        s = filter_results(&list,
                           NAMESPACE(ref),
                           info->result_class,
                           ctx->brkr);
        if (s.rc != CMPI_RC_OK) {
                CU_DEBUG("filter_results did not return CMPI_RC_OK.");
                goto out;
        }

        if (list.list == NULL) {
                CU_DEBUG("List is empty.");
                goto out;
        } else {
                CU_DEBUG("Returned %u instance(s).", list.cur);
        }

        if (names_only)
                cu_return_instance_names(results, &list);
        else
                cu_return_instances(results, &list);

 out:
        inst_list_free(&list);

        return s;
}

static CMPIStatus do_ref(struct std_assoc_ctx *ctx,
                         struct std_assoc_info *info,
                         const CMPIResult *results,
                         const CMPIObjectPath *ref,
                         bool names_only)
{
        struct inst_list list;
        CMPIStatus s;
        int i;
        struct std_assoc *handler;
        bool rc;

        inst_list_init(&list);

        handler = std_assoc_get_handler(ctx, ref);
        if (handler == NULL) {
                cu_statusf(ctx->brkr, &s,
                           CMPI_RC_ERR_FAILED,
                           "Unable to handle this association");
                goto out;
        }

        rc = match_class(ctx->brkr, 
                        NAMESPACE(ref), 
                        info->result_class, 
                        handler->assoc_class);
        if (!rc) {
                cu_statusf(ctx->brkr, &s,
                           CMPI_RC_ERR_FAILED,
                           "Result class is not valid for this association");
                goto out;
        }


        s = handler->handler(ref, info, &list);
        if ((s.rc != CMPI_RC_OK) || (list.list == NULL))
                goto out;

        for (i = 0; i < list.cur; i++) {
                CMPIInstance *refinst;

                refinst = handler->make_ref(ref, list.list[i], info, handler);
                if (refinst == NULL)
                        continue;

                if (names_only)
                        cu_return_instance_name(results, refinst);
                else
                        CMReturnInstance(results, refinst);
        }

        CMSetStatus(&s, CMPI_RC_OK);
 out:
        inst_list_free(&list);

        return s;
}

CMPIStatus stda_AssociationCleanup(CMPIAssociationMI *self,
                                   const CMPIContext *context,
                                   CMPIBoolean terminating)
{
        RETURN_UNSUPPORTED();
}
CMPIStatus stda_AssociatorNames(CMPIAssociationMI *self,
                                const CMPIContext *context,
                                const CMPIResult *results,
                                const CMPIObjectPath *reference,
                                const char *assocClass,
                                const char *resultClass,
                                const char *role,
                                const char *resultRole)
{
        struct std_assoc_info info = {
                assocClass,
                resultClass,
                role,
                resultRole,
                NULL,
                context,
                self->ft->miName,
        };

        return do_assoc(self->hdl,
                        &info,
                        results,
                        reference,
                        true);
}

CMPIStatus stda_Associators(CMPIAssociationMI *self,
                            const CMPIContext *context,
                            const CMPIResult *results,
                            const CMPIObjectPath *reference,
                            const char *assocClass,
                            const char *resultClass,
                            const char *role,
                            const char *resultRole,
                            const char **properties)
{
        struct std_assoc_info info = {
                assocClass,
                resultClass,
                role,
                resultRole,
                properties,
                context,
                self->ft->miName,
        };

        return do_assoc(self->hdl,
                        &info,
                        results,
                        reference,
                        false);
}

CMPIStatus stda_ReferenceNames(CMPIAssociationMI *self,
                               const CMPIContext *context,
                               const CMPIResult *results,
                               const CMPIObjectPath *reference,
                               const char *resultClass,
                               const char *role)
{
        struct std_assoc_info info = {
                NULL,
                resultClass,
                role,
                NULL,
                NULL,
                context,
                self->ft->miName,
        };

        return do_ref(self->hdl, &info, results, reference, true);
}

CMPIStatus stda_References(CMPIAssociationMI *self,
                           const CMPIContext *context,
                           const CMPIResult *results,
                           const CMPIObjectPath *reference,
                           const char *resultClass,
                           const char *role,
                           const char **properties)
{
        struct std_assoc_info info = {
                NULL,
                resultClass,
                role,
                NULL,
                properties,
                context,
                self->ft->miName,
        };

        return do_ref(self->hdl, &info, results, reference, false);
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
