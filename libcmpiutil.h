/**
 * CMPI Utility library
 *
 * Copyright 2007 IBM Corp.
 * @author Dan Smith <danms@us.ibm.com>
 *
 * @file libcmpiutil.h
 */

#ifndef __LIBCMPIUTIL_H
#define __LIBCMPIUTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>

#define CLASSNAME(op) (CMGetCharPtr(CMGetClassName(op, NULL)))
#define NAMESPACE(op) (CMGetCharPtr(CMGetNameSpace(op, NULL)))

#define STREQ(a, b) (strcmp(a, b) == 0)
#define STREQC(a, b) (strcasecmp(a, b) == 0)

/**
 * Shortcut for unsupported intrinsics
 *
 * @returns A CMPIStatus indicating an unsupported intrinsic function
 */
#define RETURN_UNSUPPORTED() return (CMPIStatus) {CMPI_RC_ERR_NOT_SUPPORTED, \
                                                  NULL};

/**
 * Determine if a string starts with a given prefix
 *
 * @param str The string
 * @param pfx The prefix
 * @returns true if str starts with pfx, false otherwise
 */
#define STARTS_WITH(str, pfx) (strncasecmp(str, pfx, strlen(pfx)) == 0)

#define STREQ(a, b) (strcmp(a, b) == 0)
#define STREQC(a, b) (strcasecmp(a, b) == 0)
#define _CU_STRINGIFY(x) #x
#define CU_STRINGIFY(x) _CU_STRINGIFY(x)

/**
 * Dispatch macro for debug_print, fills in the function name and line number 
 * of caller.
 */
#define CU_DEBUG(fmt, args...) {                                        \
                debug_print(__FILE__ "(" CU_STRINGIFY(__LINE__)"): "    \
                            fmt "\n", ##args);}

/**
 * Helper for DEBUG macro.  Checks environmental variable CU_DEBUG and:
 *     If unset, does nothing.
 *     If set to "stdout", acts as fprintf to stdout.
 *     If set to anything else, uses value as name of log file to fprintf to.
 */
void debug_print(char *fmt, ...);

/**
 * Copies a property from one CMPIInstance to another.  If dest_name is NULL, 
 * it is assumed to be the same as src_name.
 *
 * @param broker CIM broker, needed for status calls
 * @param src_inst Instance to copy from
 * @param dest_inst Instance to copy to
 * @param src_name Name of property to be copied from src_inst
 * @param dest_name Name of property to be copied to dest_inst
 */
CMPIStatus cu_copy_prop(const CMPIBroker *broker, 
                        CMPIInstance *src_inst, CMPIInstance *dest_inst,
                        char *src_name, char *dest_name);

/**
 * Check arguments (names and count)
 *
 * @param args The argument list to check
 * @param names A NULL-terminated list of argument names
 * @returns NULL if all arguments are present, or a pointer to
 *          the name of the first missing parameter
 */
const char *cu_check_args(const CMPIArgs *args, const char **names);

/**
 * Get a string argument
 *
 * @param args The argument list to search
 * @param name The name of the argument
 * @param val The value of the argument (must be free()'d)
 * @returns CMPI_RC_OK if successful
 */
CMPIrc cu_get_str_arg(const CMPIArgs *args, const char *name, const char **val);

/**
 * Get a reference argument
 *
 * @param args The argument list to search
 * @param name The name of the argument
 * @param op The reference argument's value
 * @returns CMPI_RC_OK if successful
 */
CMPIrc cu_get_ref_arg(const CMPIArgs *args,
                      const char *name,
                      CMPIObjectPath **op);

/**
 * Get an instance argument
 *
 * @param args The argument list to search
 * @param name The name of the argument
 * @returns The instance argument's value, or NULL if error
 */
CMPIrc cu_get_inst_arg(const CMPIArgs *args,
                       const char *name,
                       CMPIInstance **inst);

/**
 * Get an array argument
 *
 * @param args The argument list to search
 * @param name The name of the argument
 * @returns The array argument's value, or NULL if error
 */
CMPIrc cu_get_array_arg(const CMPIArgs *args,
                        const char *name,
                        CMPIArray **array);

/**
 * Get a uint16 argument
 *
 * @param args The argument list to search
 * @param name The name of the argument
 * @param target The uint16_t to reflect the argument value
 * @returns CMPI_RC_OK if successful
 */
CMPIrc cu_get_u16_arg(const CMPIArgs *args, const char *name, uint16_t *target);

/**
 * Get a string component of an object path
 *
 * @param ref The object path
 * @param key The name of the component to return
 * @param val The value of the component, or NULL if error (must be
 *            free()'d)
 * @returns CMPI_RC_OK on success
 */
CMPIrc cu_get_str_path(const CMPIObjectPath *ref,
                       const char *key,
                       const char **val);

/**
 * Get a uint16 component of an object path
 *
 * @param reference The object path
 * @param key The name of the component to return
 * @param target A pointer to the uint16 to set
 * @returns CMPI_RC_OK if successful
 */
CMPIrc cu_get_u16_path(const CMPIObjectPath *reference,
                       const char *key,
                       uint16_t *target);

/**
 * Merge src and dest instances to dest.
 *
 * @param src Source instance
 * @param dest Destination instance
 * @returns {CMPI_RC_OK, NULL} if success, CMPI_RC ERR_FAILED and 
 *          error message otherwise
 */
CMPIStatus cu_merge_instances(CMPIInstance *src,
                                 CMPIInstance *dest);

/**
 * Create a copy of an instance
 *
 * @param src Source instance
 * @param dest Destination instance
 * @returns {CMPI_RC_OK, NULL} if success, CMPI_RC ERR_FAILED and 
 *          error message otherwise
 */
CMPIInstance *cu_dup_instance(const CMPIBroker *broker,
                              CMPIInstance *src,
                              CMPIStatus *s);

/* Forward declaration */
struct inst_list;

/**
 * Return a list of instances
 *
 * @param results The result list to populate
 * @param list A list of instances to return
 * @returns The number of instances returned
 */
unsigned int cu_return_instances(const CMPIResult *results,
                                 const struct inst_list *list);

/**
 * Return an instance object path
 *
 * @param inst The instance
 * @param results The result list to populate
 * @param ns The namespace
 * @returns true if successful, false otherwise
 */
bool cu_return_instance_name(const CMPIResult *results,
                             const CMPIInstance *inst);

/**
 * Return the object paths of a list of instances
 *
 * @param results The result list to populate
 * @param list A list of instances to return (names of)
 * @returns The number of instance names returned
 */
unsigned int cu_return_instance_names(const CMPIResult *results,
                                      const struct inst_list *list);

/**
 * Get an array property of an instance
 *
 * @param inst The instance
 * @param prop The property name
 * @param target A pointer to a CMPIarray that will be set
 *               if successful
 * @returns 
 *        - CMPI_RC_OK on success, 
 *        - CMPI_RC_ERR_NO_SUCH_PROPERTY if prop is not present,
 *        - CMPI_RC_ERR_TYPE_MISMATCH if prop is not an array,
 *        - CMPI_RC_ERROR otherwise
 */
CMPIrc cu_get_array_prop(const CMPIInstance *inst,
                         const char *prop,
                         CMPIArray **array);

/**
 * Get a string property of an instance
 *
 * @param inst The instance
 * @param prop The property name
 * @param target A pointer to a char* that will be set to a malloc'd string
 *               if successful
 * @returns 
 *        - CMPI_RC_OK on success, 
 *        - CMPI_RC_ERR_NO_SUCH_PROPERTY if prop is not present,
 *        - CMPI_RC_ERR_TYPE_MISMATCH if prop is not a string,
 *        - CMPI_RC_ERROR otherwise
 */
CMPIrc cu_get_str_prop(const CMPIInstance *inst,
                       const char *prop,
                       const char **target);

/**
 * Get a boolean property of an instance
 *
 * @param inst The instance
 * @param prop The property name
 * @param target A pointer to a bool that will reflect the property status
 * @returns 
 *        - CMPI_RC_OK on success, 
 *        - CMPI_RC_ERR_NO_SUCH_PROPERTY if prop is not present,
 *        - CMPI_RC_ERR_TYPE_MISMATCH if prop is not a boolean,
 *        - CMPI_RC_ERROR otherwise
 */
CMPIrc cu_get_bool_prop(const CMPIInstance *inst,
                        const char *prop,
                        bool *target);

/**
 * Get an unsigned 16-bit int property of an instance
 *
 * @param inst The instance
 * @param prop The property name
 * @param target A pointer to a uint16_t that will reflect the property value
 * @returns
 *        - CMPI_RC_OK on success,
 *        - CMPI_RC_ERR_NO_SUCH_PROPERTY if prop is not present,
 *        - CMPI_RC_ERR_TYPE_MISMATCH if prop is not a uint16,
 *        - CMPI_RC_ERROR otherwise
 */
CMPIrc cu_get_u16_prop(const CMPIInstance *inst,
                       const char *prop,
                       uint16_t *target);

/**
 * Get an unsigned 32-bit int property of an instance
 *
 * @param inst The instance
 * @param prop The property name
 * @param target A pointer to a uint32_t that will reflect the property value
 * @returns 
 *        - CMPI_RC_OK on success, 
 *        - CMPI_RC_ERR_NO_SUCH_PROPERTY if prop is not present,
 *        - CMPI_RC_ERR_TYPE_MISMATCH if prop is not a uint32,
 *        - CMPI_RC_ERROR otherwise
 */
CMPIrc cu_get_u32_prop(const CMPIInstance *inst,
                       const char *prop,
                       uint32_t *target);

/**
 * Get an unsigned 64-bit int property of an instance
 *
 * @param inst The instance
 * @param prop The property name
 * @param target A pointer to a uint64_t that will reflect the property value
 * @returns 
 *        - CMPI_RC_OK on success, 
 *        - CMPI_RC_ERR_NO_SUCH_PROPERTY if prop is not present,
 *        - CMPI_RC_ERR_TYPE_MISMATCH if prop is not a uint64,
 *        - CMPI_RC_ERROR otherwise
 */
CMPIrc cu_get_u64_prop(const CMPIInstance *inst,
                       const char *prop,
                       uint64_t *target);

/**
 * Get the type of an instance property
 *
 * @param inst The instance
 * @param prop The property name
 * @returns The CMPIType of the given property, or CMPI_null if
 *          the property does not exist
 */
CMPIType cu_prop_type(const CMPIInstance *inst, const char *prop);

/**
 * Get the type of an argument
 *
 * @param args The argument set
 * @param arg The argument name
 * @returns The CMPIType of the given argument, or CMPI_null if
 *          the argument does not exist
 */
CMPIType cu_arg_type(const CMPIArgs *args, const char *arg);

/**
 * Parse an MOF-formatted embedded instance string
 *
 * @param eo The embedded instance string
 * @param broker A pointer to the current broker
 * @param ns The namespace for the new instance
 * @param instance A pointer to an instance pointer to hold the result
 * @returns nonzero on success
 *
 */
int cu_parse_embedded_instance(const char *eo,
                               const CMPIBroker *broker,
                               const char *ns,
                               CMPIInstance **instance);

/**
 * Set CMPIStatus with format string
 *
 * @param broker A pointer to the current broker
 * @param s A pointer to the status object to set
 * @param rc A the return code to set
 * @param fmt A C format specification
 * @returns A positive number of characters in the message field
 *          or -1 on failure
 *
 * Note: If the internal memory allocation needed to format the
 *       string fails, the return code will still be set in the status
 */
int cu_statusf(const CMPIBroker *broker,
               CMPIStatus *s,
               CMPIrc rc,
               char *fmt, ...);

/**
 * Growable instance list
 */
struct inst_list {
        CMPIInstance **list;
        unsigned int max;
        unsigned int cur;
};

/**
 * Initialize an instance list
 *
 * @param list A pointer to the list structure to initialize
 */
void inst_list_init(struct inst_list *list);

/**
 * Clean up an instance list
 * After calling this on a list, inst_list_init() must be called again
 * before the list can be re-used.
 *
 * @param list A pointer to the list structure to clean up
 */
void inst_list_free(struct inst_list *list);

/**
 * Add an instance to a list
 *
 * @param list A pointer to the list to add the instance too
 * @param inst A pointer to the instance to be added
 * @returns nonzero on success, zero on failure
 */
int inst_list_add(struct inst_list *list, CMPIInstance *inst);

/**
 * Define a pre-initialized inst_list
 *
 * @param x The variable name for the list.
 */
#define DECLARE_INST_LIST(x) struct inst_list x = {NULL, 0, 0};

/**
 * Compare key values in a reference to properties in an instance,
 * making sure they are identical.
 *
 * @param ref The ObjectPath to examine
 * @param inst The Instance to compare
 * @returns A pointer to the name of the first non-matching property,
 *          or NULL if all match
 */
const char *cu_compare_ref(const CMPIObjectPath *ref,
                           const CMPIInstance *inst);

/**
 * Validate a client given reference against the system instance.
 * This is done by comparing the key values of the reference 
 * against the key properties found in the system instance.
 *
 * @param broker A pointer to the current broker
 * @param ref The ObjectPath to examine
 * @param inst The Instance to compare
 * @returns The status of the comparision:
 *          CMPI_RC_OK in case of match
 *          CMPI_RC_ERR_NOT_FOUND in case of not matching
 */
CMPIStatus cu_validate_ref(const CMPIBroker *broker,
                           const CMPIObjectPath *ref,
                           const CMPIInstance *inst);

/**
 * Returns the classname from an instance without forcing user to get 
 * ObjectPath first.
 *
 * @param inst Instance to examine
 * @returns Classname of instance , NULL on failure
 */
const char *cu_classname_from_inst(CMPIInstance *inst);

#define DEFAULT_EIN(pn)                                                 \
        static CMPIStatus pn##EnumInstanceNames(CMPIInstanceMI *self,   \
                                                const CMPIContext *c,   \
                                                const CMPIResult *r,    \
                                                const CMPIObjectPath *o) \
        { RETURN_UNSUPPORTED(); }

#define DEFAULT_EI(pn)                                                  \
        static CMPIStatus pn##EnumInstances(CMPIInstanceMI *self,       \
                                            const CMPIContext *c,       \
                                            const CMPIResult *r,        \
                                            const CMPIObjectPath *o,    \
                                            const char **p)             \
        { RETURN_UNSUPPORTED(); }

#define DEFAULT_GI(pn)                                                  \
        static CMPIStatus pn##GetInstance(CMPIInstanceMI *self,         \
                                          const CMPIContext *c,         \
                                          const CMPIResult *r,          \
                                          const CMPIObjectPath *o,      \
                                          const char **p)               \
        { RETURN_UNSUPPORTED(); }

#define DEFAULT_CI(pn)                                                  \
        static CMPIStatus pn##CreateInstance(CMPIInstanceMI *self,      \
                                             const CMPIContext *c,      \
                                             const CMPIResult *r,       \
                                             const CMPIObjectPath *o,   \
                                             const CMPIInstance *n)     \
        { RETURN_UNSUPPORTED(); }

#define DEFAULT_MI(pn)                                                  \
        static CMPIStatus pn##ModifyInstance(CMPIInstanceMI *self,      \
                                             const CMPIContext *c,      \
                                             const CMPIResult *r,       \
                                             const CMPIObjectPath *o,   \
                                             const CMPIInstance *n,     \
                                             const char **p)            \
        { RETURN_UNSUPPORTED(); }

#define DEFAULT_DI(pn)                                                  \
        static CMPIStatus pn##DeleteInstance(CMPIInstanceMI *self,      \
                                             const CMPIContext *c,      \
                                             const CMPIResult *r,       \
                                             const CMPIObjectPath *o)   \
        { RETURN_UNSUPPORTED(); }

#define DEFAULT_EQ(pn)                                                  \
        static CMPIStatus pn##ExecQuery(CMPIInstanceMI *self,           \
                                        const CMPIContext *c,           \
                                        const CMPIResult *r,            \
                                        const CMPIObjectPath *o,        \
                                        const char *l,                  \
                                        const char *q)                  \
        { RETURN_UNSUPPORTED(); }

#define DEFAULT_INST_CLEANUP(pn)                                        \
        static CMPIStatus pn##Cleanup(CMPIInstanceMI *mi,               \
                                           const CMPIContext *c,        \
                                           CMPIBoolean terminating)     \
        { RETURN_UNSUPPORTED(); }

#define DEFAULT_IND_CLEANUP(pn)                                         \
        static CMPIStatus pn##IndicationCleanup(CMPIIndicationMI *mi,   \
                                           const CMPIContext *c,        \
                                           CMPIBoolean terminating)     \
        { RETURN_UNSUPPORTED(); }

#define DEFAULT_AF(pn)                                                  \
        static CMPIStatus pn##AuthorizeFilter(CMPIIndicationMI *mi,     \
                                              const CMPIContext *ctx,   \
                                              const CMPISelectExp *se,  \
                                              const char *ns,           \
                                              const CMPIObjectPath *op, \
                                              const char *user)         \
        { return (CMPIStatus){CMPI_RC_OK, NULL}; }

#define DEFAULT_MP(pn)                                                  \
        static CMPIStatus pn##MustPoll(CMPIIndicationMI *mi,            \
                                       const CMPIContext *ctx,          \
                                       const CMPISelectExp *se,         \
                                       const char *ns,                  \
                                       const CMPIObjectPath *op)        \
        { RETURN_UNSUPPORTED(); }

#define DEFAULT_ACTF(pn)                                                \
        static CMPIStatus pn##ActivateFilter(CMPIIndicationMI *mi,      \
                                             const CMPIContext *ctx,    \
                                             const CMPISelectExp *se,   \
                                             const char *ns,            \
                                             const CMPIObjectPath *op,  \
                                             CMPIBoolean first)         \
        { return (CMPIStatus){CMPI_RC_OK, NULL}; }

#define DEFAULT_DF(pn)                                                  \
        static CMPIStatus pn##DeActivateFilter(CMPIIndicationMI *mi,    \
                                               const CMPIContext *ctx,  \
                                               const CMPISelectExp *se, \
                                               const char *ns,          \
                                               const CMPIObjectPath *o, \
                                               CMPIBoolean last)        \
        { return (CMPIStatus){CMPI_RC_OK, NULL}; }

#define DEFAULT_ENAIND(pn)                                              \
        static void pn##EnableIndications(CMPIIndicationMI *mi,         \
                                          const CMPIContext *ctx)       \
        { return; }

#define DEFAULT_DISIND(pn)                      \
        static void pn##DisableIndications(CMPIIndicationMI *mi,        \
                                           const CMPIContext *ctx)      \
        { return; }

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

