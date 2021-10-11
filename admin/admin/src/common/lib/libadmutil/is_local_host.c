#ident "@(#)is_local_host.c 1.5 96/06/06 SMI"

/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>

/*
 * Given a host name, check to see if it points to the local host.
 * If it does, return 1, else return 0.
 *
 * This is done by getting all the names of the local network interfaces
 * and comparing them against the passed in host name.
 */
int
is_local_host(char *host) {
    struct ifconf	 ifc;		/* interface config buffer	*/
    struct ifreq	*ifr;		/* ptr to interface request	*/
    struct hostent	*hp;
    u_long		 addr;
    int			 status;
    int			 i;
    char		 ip_addr[4*4];
    char	       **q;
    int			 ret = 0;

    if ((status = get_net_if_names(&ifc)) != -1) {
       	for (ifr = ifc.ifc_req, i = (ifc.ifc_len / sizeof (struct ifreq));
		    i > 0; --i, ++ifr) {
	    get_net_if_ip_addr(ifr->ifr_name, ip_addr);
#ifdef LOCAL_TESTING
	    printf("Interface '%s' %s\n", ifr->ifr_name, ip_addr);
#endif
	    addr = inet_addr(ip_addr);
	    hp = gethostbyaddr((char *)&addr, sizeof (addr), AF_INET);
	    /*
	     * Ignore interface if it doesn't have a name.
	     */
	    if (hp != NULL) {
#ifdef LOCAL_TESTING
		printf("\tChecking '%s'\n", hp->h_name);
#endif
		if (strcmp(host, hp->h_name) == 0) {
		    ret = 1;
		    break;
		}
		for (q = hp->h_aliases; *q != 0; q++) {
#ifdef LOCAL_TESTING
		    if(*q) printf("\tChecking alias '%s'\n", *q);
#endif
		    if (strcmp(host, *q) == 0) {
			ret = 1;
			break;
		    }
		}
	    }
	}
	free(ifc.ifc_buf);
    }
    return (ret);
}

#ifdef LOCAL_TESTING
main() {
    char **names[] = {"foo", "admin-8", "bar", "admin-8qe3", NULL};
    char **c = names;

    while(*c) {
	printf("name=%s -> %d\n", *c, is_local_host(*c));
	c++;
    }
}
#endif
