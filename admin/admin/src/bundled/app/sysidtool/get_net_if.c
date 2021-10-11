/*
 *  Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)get_net_if.c	1.12	96/06/07 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "sysidtool.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <sys/stropts.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netinet/in.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include "prompt.h"

#if defined(SYSV) && !defined(SIOCGIFCONF_FIXED)
#define	MAXIFS	32	/* results in a bufsize of 1024 */
#else
#define	MAXIFS	256
#endif
#define	SOCKET_AF(af) 	(((af) == AF_UNSPEC) ? AF_INET : (af))

extern int errno;

void
get_net_name_num(char *if_name, char *netnum)
{
	char		ip_addr[MAX_IPADDR+2];
	struct in_addr	in;
	int		tmp_int;
	char		*tmp_ptr;

	fprintf(debugfp, "get_net_name_num\n");

	(void) get_net_if_name(if_name);
	netnum[0] = NULL;
	if (get_net_ipaddr(if_name, ip_addr) == SUCCESS) {
		in.s_addr = inet_addr(ip_addr);
		tmp_int = inet_netof(in);
		in = inet_makeaddr(tmp_int, 0);
		tmp_ptr = inet_ntoa(in);
		(void) strcpy(netnum, tmp_ptr);
	} else {
		for (;;) {
			prompt_error(SYSID_ERR_NO_IPADDR, if_name);
		}
	}

	fprintf(debugfp, "get_net_name_num: netnum=%s\n", netnum);
}

int
get_net_if_name(char *nm)
{
	int status;
	int	s;
	int n;
	char *buf;
	struct ifconf ifc;
	register struct ifreq *ifrp;
	int numifs;
	unsigned bufsize;

	fprintf(debugfp, "get_net_if_name\n");

	if (testing) {
		status = (*sim_handle())(SIM_GET_FIRSTUP_IF, nm);
		return (status);
	}

	s = socket(SOCKET_AF(AF_INET), SOCK_DGRAM, 0);
	if (s < 0) {
		fprintf(debugfp, "get_net_if_name: socket %s\n",
			strerror(errno));
		return (0);
	}

	if (ioctl(s, SIOCGIFNUM, (char *)&numifs) < 0) {
		fprintf(debugfp, "get_net_if_name: ioctl %s\n",
			strerror(errno));
		numifs = MAXIFS;
	}
	bufsize = numifs * sizeof (struct ifreq);
	buf = (char *)malloc(bufsize);
	if (buf == NULL) {
		fprintf(debugfp, "get_net_if_name: out of memory\n");
		close(s);
		return (0);
	}
	ifc.ifc_len = bufsize;
	ifc.ifc_buf = buf;
	if (ioctl(s, SIOCGIFCONF, (char *)&ifc) < 0) {
		fprintf(debugfp, "get_net_if_name: ioctl %s\n",
			strerror(errno));
		close(s);
		(void) free(buf);
		return (0);
	}

	ifrp = ifc.ifc_req;
	for (n = ifc.ifc_len / sizeof (struct ifreq); n > 0; n--, ifrp++) {
		struct	ifreq ifr;
		int flags;

		memset((char *) &ifr, '\0', sizeof (ifr));
		strncpy(ifr.ifr_name, ifrp->ifr_name, sizeof (ifr.ifr_name));

		if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
			continue;

		flags = ifr.ifr_flags;

		if (flags & (1 << 3))	/* loopback */
			continue;
		if (!(flags & 1))	/* ! up */
			continue;

		strcpy(nm, ifrp->ifr_name);
		(void) free(buf);
		close(s);
		fprintf(debugfp, "get_net_if_name: %s\n", nm);
		return (1);
	}

	(void) free(buf);
	close(s);
	return (0);
}

int
get_net_if_list(struct if_list **list)
{
	int cnt = 0;
	struct if_list *tmp, *p;
	int n, s;
	char *buf;
	struct ifconf ifc;
	register struct ifreq *ifrp;
	struct ifreq ifr;
	int numifs;
	unsigned bufsize;

	fprintf(debugfp, "get_net_if_list\n");

	*list = NULL;

	/*
	 * Now that ifconfig -a plumb will guarantee that all
	 * the interfaces are plumbed, get_net_if_list only has
	 * to read the plumbed interfaces to generate the list
	 * of names to present to the user
	 */

	if (testing) {
		int status;

		status = (*sim_handle())(SIM_GET_IFLIST, list);
		return (status);
	}
	s = socket(SOCKET_AF(AF_INET), SOCK_DGRAM, 0);
	if (s < 0) {
		perror("get_net_if_list: socket");
		return (0);
	}

	if (ioctl(s, SIOCGIFNUM, (char *)&numifs) < 0) {
		perror("ioctl (get number of interfaces)");
		numifs = MAXIFS;
	}
	bufsize = numifs * sizeof (struct ifreq);
	buf = (char *)malloc(bufsize);
	if (buf == NULL) {
		fprintf(stderr, "out of memory\n");
		close(s);
		return (0);
	}
	ifc.ifc_len = bufsize;
	ifc.ifc_buf = buf;
	if (ioctl(s, SIOCGIFCONF, (char *)&ifc) < 0) {
		fprintf(stderr, "Unable to get interface list\n");
		close(s);
		(void) free(buf);
		return (0);
	}

	ifrp = ifc.ifc_req;
	for (n = ifc.ifc_len / sizeof (struct ifreq); n > 0; n--, ifrp++) {
		/* Get the flags so that we can skip the loopback interface */
		memset((char *) &ifr, '\0', sizeof (ifr));
		strncpy(ifr.ifr_name, ifrp->ifr_name, sizeof (ifr.ifr_name));

		if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0) {
			/* Error */
			fprintf(debugfp,
			    "Unable to get flags for %s interface, errno %d\n",
			    ifrp->ifr_name, errno);
			return (cnt);
		}
		fprintf(debugfp, "Interface %s, flags %x\n", ifr.ifr_name,
			ifr.ifr_flags);
		if (ifr.ifr_flags & IFF_LOOPBACK)
			continue;
		if ((tmp = (struct if_list *) malloc(sizeof (struct if_list)))
		    == NULL)
			/* out of memory */
			return (cnt);

		tmp->next = NULL;
		strcpy(tmp->name, ifrp->ifr_name);
		if (*list == NULL)
			*list = tmp;
		else {
			for (p = *list; p->next; p = p->next);
			p->next = tmp;
		}
		cnt++;
	}
	close(s);
	(void) free(buf);
	fprintf(debugfp, "get_net_if_list: cnt %d\n", cnt);
	return (cnt);
}
