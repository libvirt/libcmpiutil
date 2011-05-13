/*
 * Copyright IBM Corp. 2007
 *
 * Authors:
 *  Dan Smith <danms@us.ibm.com>
 *  Zhengang Li <lizg@cn.ibm.com>
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cmpimacs.h>

#include "libcmpiutil.h"
#include "eo_util_parser.h"
#include "eo_parser_xml.h"

#define TEMPLATE "/tmp/tmp_eo_parse.XXXXXX"

void eo_parse_restart(FILE *);
int eo_parse_parseinstance(const CMPIBroker *, CMPIInstance **, const char *ns);

static int write_temp(const char *eo)
{
        int fd;
        int ret;
        char tmpl[sizeof(TEMPLATE)] = TEMPLATE;

        fd = mkstemp(tmpl);
        if (fd < 0)
                return fd;

        ret = write(fd, eo, strlen(eo));

        unlink(tmpl);

        if (ret != strlen(eo)) {
                close(fd);
                return -1;
        }

        fcntl(fd, F_SETFL, O_RDONLY);
        lseek(fd, 0, SEEK_SET);

        return fd;
}

#ifdef HAVE_EOPARSER
static int parse_ei_mof(const CMPIBroker *broker,
                        const char *ns,
                        const char *eo,
                        CMPIInstance **instance)
{
        int ret;
        int fd;
        FILE *fp;

        fd = write_temp(eo);
        if (fd < 0)
                return 0;

        fp = fdopen(fd, "r");
        eo_parse_restart(fp);
        ret = eo_parse_parseinstance(broker, instance, ns);

        close(fd);

        return ret;
}
#endif

int cu_parse_embedded_instance(const char *eo,
                               const CMPIBroker *broker,
                               const char *ns,
                               CMPIInstance **instance)
{
        if (strcasestr(eo, "<instance")) {
                CU_DEBUG("Parsing CIMXML-style EI");
                return cu_parse_ei_xml(broker, ns, eo, instance);
        } else {
#ifdef HAVE_EOPARSER
                CU_DEBUG("Parsing MOF-style EI");
                return parse_ei_mof(broker, ns, eo, instance);
#else
                CU_DEBUG("EI parser did not see `<instance', "
                         "assuming not CIMXML");
                return 0;
#endif
        }
}

static int _set_int_prop(CMPISint64 value,
                         char *prop,
                         CMPIType type,
                         CMPIInstance *inst)
{
        CMPIStatus s;
        uint64_t unsigned_val = 0;
        int64_t signed_val = 0;

        switch(type) {
        case CMPI_uint64:
        case CMPI_uint32:
        case CMPI_uint16:
        case CMPI_uint8:
                unsigned_val = (uint64_t) value;
                s = CMSetProperty(inst,
                                  prop,
                                  (CMPIValue *) &(unsigned_val),
                                  type);
                break;
        case CMPI_sint64:
        case CMPI_sint32:
        case CMPI_sint16:
        case CMPI_sint8:
        default:
                signed_val = (int64_t) value;
                s = CMSetProperty(inst,
                                  prop,
                                  (CMPIValue *) &(signed_val),
                                  type);
        }

        if (s.rc == CMPI_RC_OK)
               return 1;

        return 0;
}

CMPIType set_int_prop(CMPISint64 value,
                      char *prop,
                      CMPIInstance *inst)
{
        if (_set_int_prop(value, prop, CMPI_uint64, inst) == 1)
               return CMPI_uint64;
        else if (_set_int_prop(value, prop, CMPI_uint32, inst) == 1)
               return CMPI_uint32;
        else if (_set_int_prop(value, prop, CMPI_uint16, inst) == 1)
               return CMPI_uint16;
        else if (_set_int_prop(value, prop, CMPI_uint8, inst) == 1)
               return CMPI_uint8;
        else if (_set_int_prop(value, prop, CMPI_sint64, inst) == 1)
               return CMPI_sint64;
        else if (_set_int_prop(value, prop, CMPI_sint32, inst) == 1)
               return CMPI_sint32;
        else if (_set_int_prop(value, prop, CMPI_sint16, inst) == 1)
               return CMPI_sint16;
        else
               _set_int_prop(value, prop, CMPI_sint8, inst);

        return CMPI_sint8;
}

inline CMPIStatus ins_chars_into_cmstr_arr(const CMPIBroker *broker,
                                           CMPIArray *arr,
                                           CMPICount index,
                                           char *str)
{
        CMPIString *cm_str;
        CMPIStatus s = {CMPI_RC_OK, NULL};

        cm_str = CMNewString(broker, str, &s);
        if (s.rc != CMPI_RC_OK || CMIsNullObject(cm_str)) {
                CU_DEBUG("Error creating CMPIString");
                goto out;
        }

        s = CMSetArrayElementAt(arr, index, &cm_str, CMPI_string);
        if (s.rc != CMPI_RC_OK)
                CU_DEBUG("Error setting array element %u\n"
                         "Error code: %d", index, s.rc);

 out:
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
