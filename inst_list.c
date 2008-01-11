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
#include <stdlib.h>

#include <cmpidt.h>

#include <libcmpiutil.h>

static int resize(struct inst_list *list, int newmax)
{
        CMPIInstance **newlist;
        int i;

        newlist = realloc(list->list, newmax * sizeof(CMPIInstance *));
        if (!newlist)
                return 0;

        list->max = newmax;
        list->list = newlist;

        for (i = list->cur; i < list->max; i++)
                list->list[i] = NULL;

        return 1;
}

void inst_list_init(struct inst_list *list)
{
        list->list = NULL;
        list->cur = list->max = 0;
}

void inst_list_free(struct inst_list *list)
{
        free(list->list);
        inst_list_init(list);
}

int inst_list_add(struct inst_list *list, CMPIInstance *inst)
{
        if ((list->cur + 1) >= list->max) {
                int ret;

                ret = resize(list, list->max + 10);

                if (!ret)
                        return 0;
        }

        list->list[list->cur] = inst;

        list->cur++;

        return 1;
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
