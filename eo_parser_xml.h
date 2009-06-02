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

#ifndef __EO_PARSER_XML_H
#define __EO_PARSER_XML_H

int cu_parse_ei_xml(const CMPIBroker *broker,
                    const char *ns,
                    const char *xml,
                    CMPIInstance **instance);

CMPIType set_int_prop(CMPISint64 value,
                      char *prop,
                      CMPIInstance *inst);

inline CMPIStatus ins_chars_into_cmstr_arr(const CMPIBroker *broker,
                                           CMPIArray *arr,
                                           CMPICount index,
                                           char *str);

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
