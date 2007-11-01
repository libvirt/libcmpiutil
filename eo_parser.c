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

#include "cmpidt.h"

#include "libcmpiutil.h"
#include "eo_util_parser.h"

#define TEMPLATE "/tmp/tmp_eo_parse.XXXXXX"

void eo_parse_restart(FILE *);
int eo_parse_parseinstance(const CMPIBroker *, CMPIInstance **);

static int write_temp(char *eo)
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

int cu_parse_embedded_instance(const char *eo,
                               const CMPIBroker *broker,
                               CMPIInstance **instance)
{
        int ret;
#ifdef HAVE_EOPARSER
        int fd;
        FILE *fp;

        fd = write_temp(eo);
        if (fd < 0)
                return 0;

        fp = fdopen(fd, "r");
        eo_parse_restart(fp);
        ret = eo_parse_parseinstance(broker, instance);

        close(fd);
#else
        ret = 0;
#endif
        return ret;
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
