/*
 * Copyright IBM Corp. 2007
 *
 * Authors:
 *  Jay Gagnon <grendel@linux.vnet.ibm.com>
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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

#include "libcmpiutil.h"

static int log_init = 0;
static FILE *log = NULL;

void debug_print(char *fmt, ...)
{
        char *log_file = NULL;
        pthread_t id;
        struct timeval tv;
        struct timezone tz;
        time_t timep;
        struct tm *p;

        va_list ap;

        va_start(ap, fmt);

        if (log_init == 0) {
                log_file = getenv("CU_DEBUG");
                if (log_file) {
                        if (STREQ(log_file, "stdout")) {
                                log = stdout;
                        } else {
                                log = fopen(log_file, "a");
                                setlinebuf(log);
                        }
                }
                log_init = 1;
        }

        if (log != NULL) {
                gettimeofday(&tv, &tz);
                time(&timep);
                p=localtime(&timep);
                id = pthread_self();
                fprintf(log, "[%d-%02d-%02d %02d:%02d:%02d.%06d] [%ld]: ",
                (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour,
                p->tm_min, p->tm_sec, (int)tv.tv_usec, id);
                vfprintf(log, fmt, ap);
        }

        va_end(ap);
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
