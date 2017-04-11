/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _IFC_UTILS_H_
#define _IFC_UTILS_H_
/* 用于存储ip,mask,gateway,dns的数组，大小最小为16bytes */
int ifc_up(const char *name);
int ifc_down(const char *name);
int ifc_disable(const char *ifname);

int ifc_add_host_route(const char *name, const char *addr);
int ifc_remove_host_routes(const char *name);

int ifc_remove_default_route(const char *ifname);
//下面的set和get,不需要的填入NULL
int ifc_set_configure(const char *ifname, const char *mac, const char *address, const char *netmask, const char *gateway, const char *dns);
int ifc_get_configure(const char *ifname, char *mac, char *address, char *netmask, char *gateway, char *dns);

int net_wire_connected(const char *dev);

#endif
