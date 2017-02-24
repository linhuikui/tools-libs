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

int ifc_up(const char *name);
int ifc_down(const char *name);
int ifc_disable(const char *ifname);

int ifc_set_addr(const char *name, const char *addr);
int ifc_set_mask(const char *name, const char *mask);
int ifc_set_default_route(const char *ifname, const char *gateway);
int ifc_set_dns(const char *dns);

int ifc_get_info(const char *name, char *addr, char *mask, unsigned *flags);
int ifc_get_default_route(const char *ifname, char *gateway);
int ifc_get_dns(char *dns);


int ifc_create_default_route(const char *name, const char *addr);
int ifc_remove_default_route(const char *ifname);

int ifc_configure(const char *ifname, const char *address, const char *netmask, const char *gateway, const char *dns);

#endif
