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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/route.h>
#include <linux/wireless.h>

#include <stdio.h>
#include <string.h>

static int ifc_ctl_sock = -1;

static char *ipaddr_to_string(uint32_t addr, char *dest)
{
    struct in_addr in_addr;
    in_addr.s_addr = addr;
	unsigned char *a = (void *)&in_addr;

	if(dest == NULL)
		return NULL;
	
	snprintf(dest, 16 * sizeof(char), "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
	
	return dest;
}

/*only supply for ipv4*/
static in_addr_t string_to_ipaddr(int af, const char *addr)
{
	af = af;//make compiler happy
	
    return inet_addr(addr);
}

static int ifc_init(void)
{
    if (ifc_ctl_sock == -1) {
        ifc_ctl_sock = socket(AF_INET, SOCK_DGRAM, 0);    
        if (ifc_ctl_sock < 0) {
            printf("socket() failed: %s\n", strerror(errno));
        }
    }
    return ifc_ctl_sock < 0 ? -1 : 0;
}

static void ifc_close(void)
{
    if (ifc_ctl_sock != -1) {
        (void)close(ifc_ctl_sock);
        ifc_ctl_sock = -1;
    }
}

static void ifc_init_ifr(const char *name, struct ifreq *ifr)
{
    memset(ifr, 0, sizeof(struct ifreq));
    strncpy(ifr->ifr_name, name, IFNAMSIZ);
    ifr->ifr_name[IFNAMSIZ - 1] = 0;
}

static int ifc_set_flags(const char *name, unsigned set, unsigned clr)
{
    struct ifreq ifr;
    ifc_init_ifr(name, &ifr);

    if(ioctl(ifc_ctl_sock, SIOCGIFFLAGS, &ifr) < 0) return -1;
    ifr.ifr_flags = (ifr.ifr_flags & (~clr)) | set;
    return ioctl(ifc_ctl_sock, SIOCSIFFLAGS, &ifr);
}

int ifc_up(const char *name)
{
    return ifc_set_flags(name, IFF_UP, 0);
}

int ifc_down(const char *name)
{
    return ifc_set_flags(name, 0, IFF_UP);
}

static void init_sockaddr_in(struct sockaddr *sa, in_addr_t addr)
{
    struct sockaddr_in *sin = (struct sockaddr_in *) sa;
    sin->sin_family = AF_INET;
    sin->sin_port = 0;
    sin->sin_addr.s_addr = addr;
}

int ifc_set_addr(const char *name, const char *addr)
{
    struct ifreq ifr;
	in_addr_t iaddr;

	iaddr = string_to_ipaddr(AF_INET, addr);

    ifc_init_ifr(name, &ifr);
    init_sockaddr_in(&ifr.ifr_addr, iaddr);
    
    return ioctl(ifc_ctl_sock, SIOCSIFADDR, &ifr);
}

int ifc_set_mask(const char *name, const char *mask)
{
    struct ifreq ifr;
	in_addr_t imask;

	imask = string_to_ipaddr(AF_INET, mask);

    ifc_init_ifr(name, &ifr);
    init_sockaddr_in(&ifr.ifr_addr, imask);
    
    return ioctl(ifc_ctl_sock, SIOCSIFNETMASK, &ifr);
}

int ifc_get_info(const char *name, char *addr, char *mask, unsigned *flags)
{
    struct ifreq ifr;

	ifc_init();
    ifc_init_ifr(name, &ifr);
	
	in_addr_t iaddr, imask;
	
    if (addr != NULL) {
        if(ioctl(ifc_ctl_sock, SIOCGIFADDR, &ifr) < 0) {
            iaddr = 0;
        } else {
            iaddr = ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr;
        }
		ipaddr_to_string(iaddr, addr);
    }
    
    if (mask != NULL) {
        if(ioctl(ifc_ctl_sock, SIOCGIFNETMASK, &ifr) < 0) {
            imask = 0;
        } else {
            imask = ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr;
        }
		ipaddr_to_string(imask, mask);
    }
	
    if (flags != NULL) {
        if(ioctl(ifc_ctl_sock, SIOCGIFFLAGS, &ifr) < 0) {
            *flags = 0;
        } else {
            *flags = ifr.ifr_flags;
        }
    }

	ifc_close();

    return 0;
}

int ifc_create_default_route(const char *name, const char *addr)
{
    struct rtentry rt;
	in_addr_t iaddr;

	iaddr = string_to_ipaddr(AF_INET, addr);

    memset(&rt, 0, sizeof(rt));
    
    rt.rt_dst.sa_family = AF_INET;
    rt.rt_flags = RTF_UP | RTF_GATEWAY;
    rt.rt_dev = (void*) name;
    init_sockaddr_in(&rt.rt_genmask, 0);
    init_sockaddr_in(&rt.rt_gateway, iaddr);
    
    return ioctl(ifc_ctl_sock, SIOCADDRT, &rt);
}

int ifc_add_host_route(const char *name, const char *addr)
{
    struct rtentry rt;
    int result;
	in_addr_t iaddr;

	iaddr = string_to_ipaddr(AF_INET, addr);

    memset(&rt, 0, sizeof(rt));
    
    rt.rt_dst.sa_family = AF_INET;
    rt.rt_flags = RTF_UP | RTF_HOST;
    rt.rt_dev = (void*) name;
    init_sockaddr_in(&rt.rt_dst, iaddr);
    init_sockaddr_in(&rt.rt_genmask, 0);
    init_sockaddr_in(&rt.rt_gateway, 0);
    
    ifc_init();
    result = ioctl(ifc_ctl_sock, SIOCADDRT, &rt);
    if (result < 0 && errno == EEXIST) {
        result = 0;
    }
    ifc_close();
    return result;
}

int ifc_disable(const char *ifname)
{
    int result;

    ifc_init();
    result = ifc_down(ifname);
    ifc_set_addr(ifname, 0);
    ifc_close();
    return result;
}

/*
 * Remove the routes associated with the named interface.
 */
int ifc_remove_host_routes(const char *name)
{
    char ifname[64];
    in_addr_t dest, gway, mask;
    int flags, refcnt, use, metric, mtu, win, irtt;
    struct rtentry rt;
    FILE *fp;
    struct in_addr addr;

    fp = fopen("/proc/net/route", "r");
    if (fp == NULL)
        return -1;
    /* Skip the header line */
    if (fscanf(fp, "%*[^\n]\n") < 0) {
        fclose(fp);
        return -1;
    }
    ifc_init();
    for (;;) {
        int nread = fscanf(fp, "%63s%X%X%X%d%d%d%X%d%d%d\n",
                           ifname, &dest, &gway, &flags, &refcnt, &use, &metric, &mask,
                           &mtu, &win, &irtt);
        if (nread != 11) {
            break;
        }
        if ((flags & (RTF_UP|RTF_HOST)) != (RTF_UP|RTF_HOST)
                || strcmp(ifname, name) != 0) {
            continue;
        }
        memset(&rt, 0, sizeof(rt));
        rt.rt_dev = (void *)name;
        init_sockaddr_in(&rt.rt_dst, dest);
        init_sockaddr_in(&rt.rt_gateway, gway);
        init_sockaddr_in(&rt.rt_genmask, mask);
        addr.s_addr = dest;
        if (ioctl(ifc_ctl_sock, SIOCDELRT, &rt) < 0) {
            printf("failed to remove route for %s to %s: %s",
                 ifname, inet_ntoa(addr), strerror(errno));
        }
    }
    fclose(fp);
    ifc_close();
    return 0;
}

/*
 * Return the address of the default gateway
 *
 * TODO: factor out common code from this and remove_host_routes()
 * so that we only scan /proc/net/route in one place.
 */
int ifc_get_default_route(const char *ifname, char *gateway)
{
    char name[64];
    in_addr_t dest, gway, mask;
    int flags, refcnt, use, metric, mtu, win, irtt;
    int result;
    FILE *fp;

    fp = fopen("/proc/net/route", "r");
    if (fp == NULL)
        return 0;
    /* Skip the header line */
    if (fscanf(fp, "%*[^\n]\n") < 0) {
        fclose(fp);
        return 0;
    }
    ifc_init();
    result = 0;
    for (;;) {
        int nread = fscanf(fp, "%63s%X%X%X%d%d%d%X%d%d%d\n",
                           name, &dest, &gway, &flags, &refcnt, &use, &metric, &mask,
                           &mtu, &win, &irtt);
        if (nread != 11) {
            break;
        }
        if ((flags & (RTF_UP|RTF_GATEWAY)) == (RTF_UP|RTF_GATEWAY)
                && dest == 0
                && strcmp(ifname, name) == 0) {
            result = gway;
			ipaddr_to_string(gway, gateway);
            break;
        }
    }
    fclose(fp);
    ifc_close();
    return result;
}

/*
 * Sets the specified gateway as the default route for the named interface.
 */
int ifc_set_default_route(const char *ifname, const char *gateway)
{
    struct in_addr addr;
    int result;
	in_addr_t igateway;

	igateway = string_to_ipaddr(AF_INET, gateway);

    ifc_init();
    addr.s_addr = igateway;
    if ((result = ifc_create_default_route(ifname, gateway)) < 0) {
        printf("failed to add %s as default route for %s: %s",
             inet_ntoa(addr), ifname, strerror(errno));
    }
    ifc_close();
    return result;
}

/*
 * Removes the default route for the named interface.
 */
int ifc_remove_default_route(const char *ifname)
{
    struct rtentry rt;
    int result;

    ifc_init();
    memset(&rt, 0, sizeof(rt));
    rt.rt_dev = (void *)ifname;
    rt.rt_flags = RTF_UP|RTF_GATEWAY;
    init_sockaddr_in(&rt.rt_dst, 0);
    if ((result = ioctl(ifc_ctl_sock, SIOCDELRT, &rt)) < 0) {
        printf("failed to remove default route for %s: %s", ifname, strerror(errno));
    }
    ifc_close();
    return result;
}

int ifc_set_dns(const char *dns)
{
	FILE *fp = NULL;
	int ret;
	
	fp = fopen("/etc/resolv.conf", "w");
	if(fp == NULL)
		return errno;

	ret = fputs("nameserver ", fp);
	ret += fputs(dns, fp);
	ret += fputs("\r\n", fp);
	if(ret < 0)
		return errno;
	
	fflush(fp);
	
	fclose(fp);
	if(fp)
		fp = NULL;
	
	return ret;
}

int ifc_get_dns(char *dns)
{
	FILE *fp = NULL;
	char buf[64];
	int ret;
	
	fp = fopen("/etc/resolv.conf", "r");
	if(fp == NULL)
		return errno;

	ret = fscanf(fp, "%s%s", buf, dns);
	if(ret < 0)
		return errno;
	
	fflush(fp);
	
	fclose(fp);
	if(fp)
		fp = NULL;
	
	return ret;
}

int
ifc_configure(const char *ifname,
        const char *address,
        const char *netmask,
        const char *gateway,
        const char *dns) {

    int ret;

    ifc_init();

    if (ifc_up(ifname)) {
        printf("failed to turn on interface %s: %s\n", ifname, strerror(errno));
        ifc_close();
        return -1;
    }
    if (ifc_set_addr(ifname, address)) {
        printf("failed to set ipaddr %s: %s\n", address, strerror(errno));
        ifc_close();
        return -1;
    }
    if (ifc_set_mask(ifname, netmask)) {
        printf("failed to set netmask %s: %s\n", netmask, strerror(errno));
        ifc_close();
        return -1;
    }
    if (ifc_create_default_route(ifname, gateway)) {
        printf("failed to set default route %s: %s\n", gateway, strerror(errno));
        ifc_close();
        return -1;
    }

    ifc_close();

	ret = ifc_set_dns(dns);
	if(ret <= 0) {
		printf("failed to set dns %s: %s\n", dns, strerror(ret));
		return -1;
	}

    return 0;
}

#if 0
main()
{
	int ret = 1;
		
	//ret = ifc_configure("eth0", "10.0.20.120", "255.255.255.0", "10.0.20.1", "10.10.0.2");
	if(ret < 0)
		fprintf(stderr, "error on ifc_configure\n");

	char addr[16], mask[16], gway[16], dns[16];
	char *p = NULL;

	memset(addr, 0, 16);
	memset(mask, 0, 16);
	memset(gway, 0, 16);
	memset(dns, 0, 16);
	
	if(ifc_get_info("eth0", addr, mask, NULL) != 0)
		fprintf(stderr, "error on get info\n");

	if(ifc_get_default_route("eth0", gway) <= 0)
		fprintf(stderr, "error on get gway\n");

	if(ifc_get_dns(dns) < 0)
		fprintf(stderr, "error on get dns\n");

	fprintf(stderr, "addr:%s\nmask:%s\ngway:%s\ndns:%s\n", addr, mask, gway, dns);
}

#endif
