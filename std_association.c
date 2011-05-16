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
                        char **comp_class_list)
{
        CMPIObjectPath *rop;
        char *comp_class;
        int i;

        if (test_class == NULL)
                return true;

        if (comp_class_list == NULL)
                return true;

        for (i = 0; comp_class_list[i]; i++) {
                comp_class = comp_class_list[i];
                rop = CMNewObjectPath(broker, ns, comp_class, NULL);

                if (CMClassPathIsA(broker, rop, test_class, NULL))
                        return true;
        }

        return false;
}

static bool match_source_class(const CMPIBroker *broker,
                               const CMPIObjectPath *ref,
                               struct std_assoc *ptr)
{
        char *source_class;
        int i;

        for (i = 0; ptr->source_class[i]; i++) {
                source_class = ptr->source_class[i];

                if (CMClassPathIsA(broker,
                                   ref,
                                   source_class,
                                   NULL))
                        return true;
        }

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
                      struct std_assoc_info *info,
                      const CMPIObjectPath *ref)
{
        struct std_assoc *ptr = NULL;
        int i;
        bool rc;

        CU_DEBUG("Calling Provider: '%s'",
                 info->provider_name);

        for (i = 0; ctx->handlers[i]; i++) {
                ptr = ctx->handlers[i];

                if (!match_source_class(ctx->brkr, ref, ptr)) {
                        CU_DEBUG("Source class doesn't match");
                        continue;
                }

                if (!ptr) {
                        CU_DEBUG("Invalid pointer");
                        continue;
                }

                if (info->assoc_class) {
                        CU_DEBUG("Check client's assocClass: '%s'",
                                 info->assoc_class);

                        rc = match_class(ctx->brkr,
                                         NAMESPACE(ref),
                                         info->assoc_class,
                                         ptr->assoc_class);

                        if (!rc) {
                                CU_DEBUG("AssocClass not valid.");
                                continue;
                        }
                        CU_DEBUG("AssocClass valid.");
                }

                if (info->result_class) {
                        CU_DEBUG("Check client's resultClass: '%s'",
                                 info->result_class);

                        rc = match_class(ctx->brkr,
                                         NAMESPACE(ref),
                                         info->result_class,
                                         ptr->target_class);

                        if (!rc) {
                                CU_DEBUG("ResultClass not valid.");
                                continue;
                        }
                        CU_DEBUG("ResultClass valid.");
                }

                if (info->role) {
                        CU_DEBUG("Check client's role: '%s'",
                                 info->role);

                        if (!STREQC(info->role, ptr->source_prop)) {
                                CU_DEBUG("Invalid role");
                                continue;
                        }
                        CU_DEBUG("Role valid.");
                }

                if (info->result_role) {
                        CU_DEBUG("Check client's resultRole: '%s'",
                                 info->result_role);

                        if (!STREQC(info->result_role, ptr->target_prop)) {
                                CU_DEBUG("ResultRole not valid.");
                                continue;
                        }
                        CU_DEBUG("ResultRole valid.");
                }

                goto out;
        }

        CU_DEBUG("No valid handler found");
        ptr = NULL;

 out:
        return ptr;
}

static CMPIStatus prepare_ref_return_list(struct std_assoc *handler,
                                          struct std_assoc_info *info,
                                          const CMPIObjectPath *ref,
                                          struct inst_list *list)
{
        CMPIStatus s = {CMPI_RC_OK, NULL};
        struct inst_list tmp_list;
        int i;

        tmp_list = *list;
        if (tmp_list.list == NULL)
                return s;

        inst_list_init(list);

        for (i = 0; i < tmp_list.cur; i++) {
                CMPIInstance *refinst;

                refinst = handler->make_ref(ref, tmp_list.list[i], info, handler);
                if (refinst == NULL)
                        continue;

                inst_list_add(list, refinst);
        }

        inst_list_free(&tmp_list);
        return s;
}

static CMPIStatus prepare_assoc_return_list(const CMPIBroker *broker,
                                            struct std_assoc_info *info,
                                            const CMPIObjectPath *ref,
                                            struct inst_list *list)
{
        CMPIStatus s = {CMPI_RC_OK, NULL};

        if (list->list == NULL)
                return s;

        s = filter_results(list,
                           NAMESPACE(ref),
                           info->result_class,
                           broker);

        if (s.rc != CMPI_RC_OK) {
                CU_DEBUG("filter_results did not return CMPI_RC_OK.");
                return s;
        }

        return s;
}

static bool do_generic_assoc_call(struct std_assoc_info *info,
                                  struct std_assoc *handler)
{
        int i;

        if (info->assoc_class == NULL) {
                return true;
        } else {
                for (i = 0; handler->assoc_class[i]; i++) {
                        if (STREQ(info->assoc_class, handler->assoc_class[i]))
                                return false;
                }
        }

        return true;
}

static CMPIStatus handle_assoc(struct std_assoc_info *info,
                               const CMPIObjectPath *ref,
                               struct std_assoc *handler,
                               struct inst_list *list)
{
        CMPIStatus s = {CMPI_RC_OK, NULL};
        int i;

        if (do_generic_assoc_call(info, handler)) {
                for (i = 0; handler->assoc_class[i]; i++) {
                        info->assoc_class = handler->assoc_class[i];

                        CU_DEBUG("Calling handler ...");
                        s = handler->handler(ref, info, list);
                        if (s.rc != CMPI_RC_OK) {
                                CU_DEBUG("Handler did not return CMPI_RC_OK.");
                                goto out;
                        }
                }
        } else {
                CU_DEBUG("Calling handler ...");
                s = handler->handler(ref, info, list);
                if (s.rc != CMPI_RC_OK) {
                        CU_DEBUG("Handler did not return CMPI_RC_OK.");
                        goto out;
                }
        }
        CU_DEBUG("Handler returned CMPI_RC_OK.");

 out:
        return s;
}

static CMPIStatus do_assoc(struct std_assoc_ctx *ctx,
                           struct std_assoc_info *info,
                           const CMPIResult *results,
                           const CMPIObjectPath *ref,
                           bool ref_rslt,
                           bool names_only)
{
        CMPIStatus s = {CMPI_RC_OK, NULL};
        struct inst_list list;
        struct std_assoc *handler;
        int i;

        CU_DEBUG("Getting handler ...");
        handler = std_assoc_get_handler(ctx, info, ref);
        if (handler == NULL) {
                CU_DEBUG("No handler found.");
                return s;
        }
        CU_DEBUG("Getting handler succeeded.");

        inst_list_init(&list);

        if (do_generic_assoc_call(info, handler)) {
                for (i = 0; handler->assoc_class[i]; i++) {
                        info->assoc_class = handler->assoc_class[i];

                        s = handle_assoc(info, ref, handler, &list);
                        if (s.rc != CMPI_RC_OK) {
                                CU_DEBUG("Failed to handle association");
                                goto out;
                        }
                }
        } else {
                s = handle_assoc(info, ref, handler, &list);
                if (s.rc != CMPI_RC_OK) {
                        CU_DEBUG("Failed to handle association");
                        goto out;
                }
        }

        /* References and ReferenceNames */
        if (ref_rslt)
                s = prepare_ref_return_list(handler,
                                            info,
                                            ref,
                                            &list);
        /* Associators and AssociatorNames */
        else
                s = prepare_assoc_return_list(ctx->brkr,
                                              info,
                                              ref,
                                              &list);

        if (s.rc != CMPI_RC_OK) {
                CU_DEBUG("Prepare return list did not return CMPI_RC_OK.");
                goto out;
        }

        CU_DEBUG("Returned %u instance(s).", list.cur);

        if (names_only)
                cu_return_instance_names(results, &list);
        else
                cu_return_instances(results, &list);

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
                        false,
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
                        false,
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
                resultClass,
                NULL,
                role,
                NULL,
                NULL,
                context,
                self->ft->miName,
        };

        return do_assoc(self->hdl,
                        &info,
                        results,
                        reference,
                        true,
                        true);
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
                resultClass,
                NULL,
                role,
                NULL,
                properties,
                context,
                self->ft->miName,
        };

        return do_assoc(self->hdl,
                        &info,
                        results,
                        reference,
                        true,
                        false);
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
