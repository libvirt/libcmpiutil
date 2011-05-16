/*
 * Copyright IBM Corp. 2007
 *
 * Authors:
 *  Heidi Eckhart <heidieck@linux.vnet.ibm.com>
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

#ifndef __STD_INSTANCE_H
#define __STD_INSTANCE_H

/**
 * Generates the function table and initialization stub for an
 * instance provider.
 * @param pfx    The prefix for all mandatory association provider
 *               functions.
 * @param pn     The provider name under which this provider is
 *               registered.
 * @param broker The CMPIBroker pointer.
 * @param hook   Perform additional initialization functions.
 * @return       The function table of this instance provider.
 */
#define STD_InstanceMIStub(pfx, pn, broker, hook)                \
        static CMPIInstanceMIFT instMIFT__={                     \
                CMPICurrentVersion,                              \
                CMPICurrentVersion,                              \
                "instance" #pn,                                  \
                pfx##Cleanup,                                    \
                pfx##EnumInstanceNames,                          \
                pfx##EnumInstances,                              \
                pfx##GetInstance,                                \
                pfx##CreateInstance,                             \
                pfx##ModifyInstance,                             \
                pfx##DeleteInstance,                             \
                pfx##ExecQuery,                                  \
        };                                                       \
                                                                 \
        CMPIInstanceMI *                                         \
        pn##_Create_InstanceMI(const CMPIBroker *,               \
                               const CMPIContext *,              \
                               CMPIStatus *rc);                  \
                                                                 \
                                                                 \
        CMPI_EXTERN_C                                            \
        CMPIInstanceMI* pn##_Create_InstanceMI(const CMPIBroker* brkr,  \
                                               const CMPIContext *ctx,  \
                                               CMPIStatus *rc)          \
        {                                                               \
                static CMPIInstanceMI mi={                              \
                        NULL,                                           \
                        &instMIFT__,                                    \
                };                                                      \
                broker=brkr;                                            \
                hook;                                                   \
                return &mi;                                             \
        }

 #endif
