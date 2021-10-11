/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * This is the glue file that connects the calls from sysidtool
 * programs to the actual associated method calls
 * The definitions in sysidtool_test.h and here need to match
 * and the varargs stuff needs to match between the calls in
 * the sysidtool code and here.
 */

#pragma ident   "@(#)simglue.cc 1.13     95/03/24 SMI"

#include <iostream.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/*
 * adm_log.h (included by somebody in the next four lines
 * uses class as a variable name (illegal in C++)
 */
#define _adm_log_h
#include "simstate.h"
#include "sysidtool.h"
#include "sysidtool_test.h"
#include "sysid_msgs.h"
#include "cl_database_parms.h"
#include "admutil.h"
#include "admldb.h"
#include <iostream.h>
#include <fstream.h>

extern Simstate *simparse(char *fname);
extern void simparse_debug();
extern void simparse_verbose();
static Simstate *ssp;
static simhost *shp;
static nameservicelist *nlp;
static int debug;
static char nullstring[] = "";
static char no_state_object[] = "No state object in simulator";
static char no_map_str[] = "No %s map in %s nameservice";
ofstream dest_out;
enum ni_sgattr { nia_Ipaddr, nia_Enetaddr, nia_Baddr, nia_Netmask,
    nia_Upflag };
static int get_net_attr(simhost *shp, enum ni_sgattr attr, 
    char *ifname, char *attrstr, char *errmess);
static int set_net_attr(simhost *shp, enum ni_sgattr attr, 
    char *ifname, char *attrstr, char *errmess);
static const char *ni_sgattr_to_str(enum ni_sgattr);
enum nsmaptype dbtabletype_to_nsmaptype(int tabletype);
static enum ns_type ns_dbtype_to_type(int ns_db_type);
static char *type_to_nsvaltype(enum ns_type type);

extern "C" {

extern FILE *debugfp;

int
sim_init(char *source_state, char *dest_state)
{
	// Remember the dest_state file
	dest_out.open(dest_state, ios::out, 0644);
	// And now parse the source state 
	ssp = simparse(source_state);
	if (ssp == (Simstate *)0)
		return(-1);
	else {
		shp = ssp->get_simhost();
		nlp = ssp->get_nslist();
		return(0);
	}
}

void
sim_shutdown(void)
{
	// Write out simulator state to dest_state file
	ssp->print(dest_out);
	return;
}

int simulate(Sim_type type ...)
{
	va_list ap;
	int status;

	va_start(ap, type);

	switch(type) {
	case SIM_GET_NODENAME_MTHD:
		{
		status = FAILURE;

		char *errmessp = va_arg(ap, char *);
		if(errmessp != (char *)0)
			strcpy(errmessp, nullstring);
		if(debug)
			cout << "SIM_GET_NODENAME_MTHD" << endl;
		if (shp) {

			const char *hostname = shp->get_hostname();

			if (hostname != (char *)0 && hostname[0] != '\0')
				status = SUCCESS;
			else {
				status = FAILURE;
				if(errmessp != (char *)0)
					strcpy(errmessp, 
					    "Hostname not set in simulator");
			}
		} else {
			if(errmessp != (char *)0)
				strcpy(errmessp, no_state_object);
			status = FAILURE;
		}
		}
		break;

	/* char *name */
	case SIM_GET_FIRSTUP_IF:
		{

		char *name = va_arg(ap, char *);
		ni_list *nilp;
		net_interface *netp;

		status = 0;
		if(shp) {
			nilp = shp->get_ni_list();;
			if (nilp) {
				nilist_iter my_iter(*nilp);

				while(netp = my_iter()) {
					if (netp->is_loopback())
						continue;
					if (netp->is_up()) {
						strcpy(name, netp->get_name());
						status = 1;
						break;
					}
				}
			}
		}
		}
		break;

	/* struct if_list **list */
	case SIM_GET_IFLIST:
		{

		struct if_list **list = va_arg(ap, struct if_list **);
		struct if_list *tmp, *p;
		ni_list *nilp;
		net_interface *netp;

		status = 0;
		if(shp) {
			nilp = shp->get_ni_list();;
			if (nilp) {
				nilist_iter my_iter(*nilp);

				while(netp = my_iter()) {
					if (netp->is_loopback())
						continue;
					if ((tmp = (struct if_list *)
					    malloc(sizeof(struct if_list)))
					    == 0)
						return(status);
					tmp->next = NULL;
					strcpy(tmp->name, netp->get_name());
					if (*list == NULL)
						*list = tmp;
					else {
						for (p = *list; p->next;
						    p = p->next);
						p->next = tmp;
					}
					status++;
				}
			}
		}
		}
		break;
	/* char *domainname <input>, char *ns_type <output> */
	case SIM_GET_NSINFO:
		{

		char *domainname = va_arg(ap, char *);
		char *ns_type = va_arg(ap, char *);
		status = 0; // Ignored
		if (shp) {
			strcpy(ns_type, type_to_nsvaltype(shp->get_domtype()));
		}
		}
		break;

	/* char *domainname <input>, char *ns_type <output> */
	case SIM_AUTOBIND:
		{

		char *domain = va_arg(ap, char *);
		char *ns_type = va_arg(ap, char *);
		status = 0;

		// autobind can have two 12 sec. timeouts, one waiting for
		// NIS+ and one waiting for NIS.

		if (! nlp) {
			sleep(24);
			break;
		}

		nameservicelist_iter nlip(*nlp);
		nameservice *nep;
		while(nep = nlip()) {
			if (nep->get_type() != NS_NISPLUS)
				continue;
			if (strcmp(nep->get_domainname(), domain) == 0 &&
			    nep->is_broadcast()) {
				strcpy(ns_type, NIS_VERSION_3);
				status = 1;
				break;
			}
		}

		if (status)
			break;

		sleep(12);		// NIS+ timeout

		nlip.reset();
		while(nep = nlip()) {
			if (nep->get_type() != NS_NIS)
				continue;
			if (strcmp(nep->get_domainname(), domain) == 0 &&
			    nep->is_broadcast()) {
				strcpy(ns_type, NIS_VERSION_2);
				status = 1;
				break;
			}
		}

		if (status)
			break;

		sleep(12);		// NIS timeout

		}
		break;

	/* char *key <input>, char *val <output> */
	case SIM_BPGETFILE:
		{
		sleep(12);
		status = 10;
		}
		break;

	case SIM_GET_NET_IF_ETHER_MTHD:
		{

		char *if_name = va_arg(ap, char *);
		char *ether_addr = va_arg(ap, char *);
		char *errmess = va_arg(ap, char *);
		status = FAILURE;

		if (shp) {
			status = get_net_attr(shp, nia_Enetaddr, 
			    if_name, ether_addr, errmess);
		} else {
			strcpy(errmess, no_state_object);
			status = FAILURE;
		}
		}
		break;
	case SIM_GET_NET_IF_IP_ADDR_MTHD:
		{

		char *if_name = va_arg(ap, char *);
		char *ip_addr = va_arg(ap, char *);
		char *errmess = va_arg(ap, char *);
		status = FAILURE;

		if (shp) {
			status = get_net_attr(shp, nia_Ipaddr,
			    if_name, ip_addr, errmess);
		} else {
			strcpy(errmess, no_state_object);
			status = FAILURE;
		}
		}
		break;
	case SIM_SET_NODENAME_MTHD:
		{

		char *hostname = va_arg(ap, char *);
		int when = va_arg(ap, int);
		char *errmess = va_arg(ap, char *);

		/*
		 * XXX: Currently we don't check the when variable.
		 * since it is only ever called with TE_NOWANDBOOT
		 * from sysidtool
		 */
		status = FAILURE;
		if (shp) {
			shp->set_hostname(hostname);
			status = SUCCESS;
		} else {
			strcpy(errmess, no_state_object);
		}
		}
		break;

	case SIM_SET_LB_NTOA_MTHD:
		{

		char *hostname = va_arg(ap, char *);
		char *errmess = va_arg(ap, char *);

		// XXX: Currently we don't check the when variable.
		status = FAILURE;
		if (shp) {
			shp->set_hostname(hostname);
			status = SUCCESS;
		} else {
			strcpy(errmess, no_state_object);
		}
		}
		break;

	/* char *ifname, char *newent (return 0 for success, errno for not) */
	case SIM_REPLACE_HOSTNAME:
		{

		char *ifname = va_arg(ap, char *);
		char *newent = va_arg(ap, char *);

		// XXX: Currently we don't check the when variable.
		status = ENXIO;
		if (shp) {
			status = 0;
		} 
		}
		break;

	/* char *ifname, char *netmask, char *ermessp */
	case SIM_SET_NET_IF_IP_NETMASK_MTHD:
		{

		char *if_name = va_arg(ap, char *);
		char *ip_addr = va_arg(ap, char *);
		char *errmess = va_arg(ap, char *);
		status = FAILURE;

		if (shp) {
			status = set_net_attr(shp, nia_Netmask,
			    if_name, ip_addr, errmess);
		} else {
			strcpy(errmess, no_state_object);
			status = FAILURE;
		}
		}
		break;

	/* char *timehost, char *errmess */
	case SIM_SYNC_DATE_MTHD:
		{

		char *timehost = va_arg(ap, char *);

		if (! shp) {
			status = FAILURE;
			break;
		}

		enum ns_type net_ns = shp->get_domtype();

		if (net_ns == NS_NONE) {
			status = FAILURE;
			break;
		}

		int conv_ns;

		switch (net_ns) {
		case NS_NONE:
		case NS_UNSPECIFIED:
			conv_ns = UFS;
			break;
		case NS_NISPLUS:
			conv_ns = NISPLUS;
			break;
		case NS_NIS:
			conv_ns = NIS;
			break;
		}

		char name[80], ipaddr[80];
		status = simulate(SIM_DB_LOOKUP, conv_ns, HOSTS_TBL,
			timehost, &name, &ipaddr);
		}
		break;

	/* char *date, char *errmess */
	case SIM_SET_DATE_MTHD:
		{
			struct timeval  *ptv = va_arg(ap, struct timeval  *);
			struct tm       *tp;
			char date[256];

			tp = localtime(&(ptv->tv_sec));
			strftime(date, sizeof (date), "%x %X", tp);

			fprintf(debugfp, "SIM_SET_DATE_MTHD: %s\n", date);

			// XXX: Provide failure injection here
			if (shp) {
				status = SUCCESS;
			} else {
				status = FAILURE;
			}
		}
		break;

	/* char *timezone, char *errmess */
	case SIM_SET_TIMEZONE_MTHD:
		{

		char *timezone = va_arg(ap, char *);
		char *errmess = va_arg(ap, char *);

		// XXX: Provide failure injection here
		status = FAILURE;
		if (shp) {
			shp->set_timezone(timezone);
			status = SUCCESS;
		} else {
			strcpy(errmess, no_state_object);
		}
		}
		break;

	/* char *if_name, char *ifup, char *y_or_n, char *errmess */
	case SIM_SET_NET_IF_STATUS_MTHD:
		{
		char *ifname = va_arg(ap, char *);
		char *up_or_down = va_arg(ap, char *);
		char *y_or_n = va_arg(ap, char *);
		char *errmess = va_arg(ap, char *);

		status = FAILURE;

		if (shp) {
			status = set_net_attr(shp, nia_Upflag,
			    ifname, up_or_down, errmess);
		} else {
			strcpy(errmess, no_state_object);
			status = FAILURE;
		}
		}
		break;

	/* char *ifname, const struct sockaddr_in *sap, int *errno */
	case SIM_SET_IF_ADDR:
		{
		char *ifname = va_arg(ap, char *);
		struct sockaddr_in *sinp = va_arg(ap, struct sockaddr_in *);
		int *errnum = va_arg(ap, int *);
		char *ipaddr = (char *)malloc(strlen(
		    inet_ntoa(sinp->sin_addr) + 2));
		char errmess[50];

		strcpy(ipaddr, inet_ntoa(sinp->sin_addr));

		status = -1;

		if (shp) {
			status = set_net_attr(shp, nia_Ipaddr,
			    ifname, ipaddr, errmess);
			if(status == SUCCESS)
				status = 0;
			else {
				status = -1;
				*errnum = ENXIO;
			}
		} else {
			*errnum = ENOENT;
		}
		}
		break;

	/* char *hostname, int hlen, char *errmess */
	case SIM_SI_HOSTNAME:
		{

		status = FAILURE;
		char *hostname = va_arg(ap, char *);
		int hlen = va_arg(ap, int);
		char *errmessp = va_arg(ap, char *);
		if (errmessp != (char *) 0)
			strcpy(errmessp, nullstring);
		if(debug)
			cout << "SIM_GET_NODENAME_MTHD" << endl;
		if (shp) {

			const char *hname = shp->get_hostname();

			if (hname != (char *)0 && hname[0] != '\0') {
				strncpy(hostname, hname, hlen);
				status = SUCCESS;
			} else {
				status = FAILURE;
				if (errmessp != (char *) 0)
					strcpy(errmessp, 
					    "Hostname not set in simulator");
			}
		} else {
			if (errmessp != (char *) 0)
				strcpy(errmessp, no_state_object);
			status = FAILURE;
		}
		}
		break;

	/* char *domainname, int dlen, char *errmess */
	case SIM_SI_SRPC_DOMAIN:
		{

		status = FAILURE;
		char *domainname = va_arg(ap, char *);
		int dlen = va_arg(ap, int);
		char *errmessp = va_arg(ap, char *);
		if (errmessp != (char *)0)
			strcpy(errmessp, nullstring);
		if(debug)
			cout << "SIM_GET_NODENAME_MTHD" << endl;
		if (shp) {

			const char *dname = shp->get_domainname();

			if (dname != (char *)0 && dname[0] != '\0') {
				strncpy(domainname, dname, dlen);
				status = SUCCESS;
			} else {
				status = FAILURE;
				if (errmessp != (char *)0)
					strcpy(errmessp, 
					    "Domainname not set in simulator");
			}
		} else {
			if (errmessp != (char *)0)
				strcpy(errmessp, no_state_object);
			status = FAILURE;
		}
		}
		break;

	/* char *domainname, char *when, char *errmess */
	case SIM_SET_DOMAIN_MTHD:
		{

		char *domainname = va_arg(ap, char *);
		int when = va_arg(ap, int);
		char *errmess = va_arg(ap, char *);

		/*
		 * XXX: Currently we don't check the when variable.
		 * since it is only ever called with TE_NOWANDBOOT
		 * from sysidtool
		 */
		status = FAILURE;
		if (shp) {
			shp->set_domainname(domainname);
			status = SUCCESS;
		} else {
			strcpy(errmess, no_state_object);
		}
		}
		break;

	/* char *domain, char *errmess */
	case SIM_YP_INIT_ALIASES_MTHD:
		{

		char *domain = va_arg(ap, char *);
		char *errmess = va_arg(ap, char *);

		/*
		 * Currently, the only way that the SNAG method
		 * can fail is if the distribution doesn't contain
		 * /var/yp/{aliases} or the ypalias command
		 * XXX: Do we need error injection here?
		 */
		status = FAILURE;
		if (shp) {
			status = SUCCESS;
		} else {
			strcpy(errmess, no_state_object);
		}
		}
		break;

	/* char *domain, int bcast, char *servers, char *errmess */
	case SIM_YP_INIT_BINDING_MTHD:
		{

		char *domain = va_arg(ap, char *);
		int bcast = va_arg(ap, int);
		char *servers = va_arg(ap, char *);
		char *errmess = va_arg(ap, char *);
		int set_errmess = 0;

		/*
		 * Succeed if there is actually a 
		 * nameservice of the appropriate type and having
		 * the appropriate reachability (broadcast or
		 * via a server)
		 */
		status = FAILURE;
		if (nlp) {
			nameservicelist_iter nlip(*nlp);
			nameservice *nep;
			while(nep = nlip()) {
				if (nep->get_type() != NS_NIS)
					continue;
				if (strcmp(nep->get_domainname(), domain) != 0)
					continue;
				if (bcast) {
					if (nep->is_broadcast()) {
						shp->set_domtype(NS_NIS);
						status = SUCCESS;
						break;
					}
					set_errmess = 1;
					sprintf(errmess, 
						"NIS server not responding");
					break;
				}
				if (strcmp(nep->get_serverhname(), servers)
				    == 0) {
					shp->set_domtype(NS_NIS);
					status = SUCCESS;
					break;
				}
				set_errmess = 1;
				sprintf(errmess, 
			"NIS domain %s, configured server %s, server %s",
				    domain, nep->get_serverhname(), servers);
				break;
			}
			if(!set_errmess)
				sprintf(errmess, "No NIS domain for %s found",
				    domain);
		} else {
			strcpy(errmess, no_state_object);
		}
		}
		break;

	/* int bcast, char *server, char *errmess */
	case SIM_NIS_INIT_MTHD:
		{

		int bcast = va_arg(ap, int);
		char *server = va_arg(ap, char *);
		char *errmess = va_arg(ap, char *);
		int set_errmess = 0;
		int found_ns = 0;

		/*
		 * Succeed if there is actually a 
		 * nameservice of the appropriate type and having
		 * the appropriate reachability (broadcast or
		 * via a server)
		 */
		status = FAILURE;
		if (nlp) {
			nameservicelist_iter nlip(*nlp);
			nameservice *nep;
			while(nep = nlip()) {
				if(nep->get_type() != NS_NISPLUS)
					continue;
				// Fill in server so error message
				// that comes up on error makes sense
				if(bcast)
					strcpy(server, nep->get_domainname());
				if(strcmp(shp->get_domainname(), 
				    nep->get_domainname()) != 0)
					continue;
				if(nep->is_broadcast()) {
					if(bcast) {
						shp->set_domtype(NS_NISPLUS);
						status = SUCCESS;
						break;
					} else {
						set_errmess = 1;
						sprintf(errmess, 
			    "NIS+ domain %s is broadcast, binding as if not",
						    nep->get_domainname());
						break;
					}
				}
				if(strcmp(nep->get_serverhname(), 
				    server) == 0) {
					shp->set_domtype(NS_NISPLUS);
					status = SUCCESS;
					found_ns = 1;
					break;
				}
				set_errmess = 1;
				sprintf(errmess, 
				    "NIS+ domain %s server %s you think %s",
				    nep->get_domainname(), 
				    nep->get_serverhname(), server);
				break;
			}
			if(found_ns == 0 && set_errmess == 0)
				sprintf(errmess, "No NIS+ domain for %s found",
				    shp->get_domainname());
		} else {
			strcpy(errmess, no_state_object);
		}
		}
		break;

	/* char *nsval, char *errmess */
	case SIM_CONFIG_NSSWITCH_MTHD:
		{
			char *nsval = va_arg(ap, char *);
			char *errmess = va_arg(ap, char *);

			// XXX: Provide failure injection here
			if (shp) {
				status = SUCCESS;
			} else {
				status = FAILURE;
			}
		}
		break;
	/*
	 * int *sys_configured, int *sys_bootparamed, int *sys_networked,
	 * int *sys_autobound, int *sys_subnetted, int *sys_passwd,
	 * char *term_type, int *err_num
	 */
	case SIM_GET_STATE:
		{
		int *sys_configured = va_arg(ap, int *);
		int *sys_bootparamed = va_arg(ap, int *);
		int *sys_networked = va_arg(ap, int *);
		int *sys_autobound = va_arg(ap, int *);
		int *sys_subnetted = va_arg(ap, int *);
		int *sys_passwd = va_arg(ap, int *);
		int *sys_locale = va_arg(ap, int *);
		char *term_type = va_arg(ap, char *);
		int *err_num = va_arg(ap, int *);

		if (shp) {
			simsysidstate *ssp = shp->get_simsysidstate();

			*sys_configured = ssp->get_configured();
			*sys_bootparamed = ssp->get_bootparamed();
			*sys_networked = ssp->get_networked();
			*sys_autobound = ssp->get_autobound();
			*sys_subnetted = ssp->get_subnetted();
			*sys_passwd = ssp->get_rootpass();
			*sys_locale = ssp->get_locale();
			strcpy(term_type, ssp->get_termtype());
			*err_num = 0;
			status = SUCCESS;
		} else {
			*err_num = ENXIO;
			status = FAILURE;
		}
		}
		break;
	/*
	 * int sys_configured, int sys_bootparamed, int sys_networked,
	 * int sys_autobound, int sys_subnetted, int sys_passwd,
	 * char *term_type
	 */
	case SIM_PUT_STATE:
		{
		int sys_configured = va_arg(ap, int);
		int sys_bootparamed = va_arg(ap, int);
		int sys_networked = va_arg(ap, int);
		int sys_autobound = va_arg(ap, int);
		int sys_subnetted = va_arg(ap, int);
		int sys_passwd = va_arg(ap, int);
		int sys_locale = va_arg(ap, int);
		char *term_type = va_arg(ap, char *);

		if (shp) {
			simsysidstate *ssp = shp->get_simsysidstate();

			ssp->set_configured(sys_configured);
			ssp->set_bootparamed(sys_bootparamed);
			ssp->set_networked(sys_networked);
			ssp->set_autobound(sys_autobound);
			ssp->set_subnetted(sys_subnetted);
			ssp->set_rootpass(sys_passwd);
			ssp->set_locale(sys_locale);
			ssp->set_termtype(term_type);
			status = SUCCESS;
		} else {
			status = FAILURE;
		}
		}
		break;

	/* char *name, pid_t *pid, none, 0 success, 1 failure */
	case SIM_CHECK_DAEMON:
		{
			// For now, always return failure
			// routed is normally only running for diskless
			// clients (started in S69inet).  sysidsys runs
			// on diskless clients in S71sysid.sys.
			status = 1;
		}
		break;
	/* int pid, int signal (set errno if return != 0) */
	case SIM_KILL:
		{
			// For now, always return 0
			status = 0;
		}
		break;
	/* char *ns_type, char *table_type, ... */
	case SIM_DB_LOOKUP:
		{
			int ns_type = va_arg(ap, int);
			int tabletype = va_arg(ap, int);

			enum nsmaptype table_type = 
			    dbtabletype_to_nsmaptype(tabletype);

			enum ns_type ns_passed_type = 
			    ns_dbtype_to_type(ns_type);

			int found_nameservice = 0;

			// Look for a nameservice of the appropriate
			// type and fail if we can't find one
			nameservice *nep;

			if (nlp) {
				nameservicelist_iter nlip(*nlp);
				while(!found_nameservice && 
				    ((nep = nlip()) != (nameservice *)0)) {
					if(nep->get_type() != ns_passed_type)
						continue;
					if (ns_passed_type != NS_NONE) {
						// Check that domainname
						// and type match
						if((shp->get_domtype() ==
						    ns_passed_type) &&
					    (strcmp(nep->get_domainname(),
						    shp->get_domainname()) == 0)
						    ) {
							status = 0;
							found_nameservice = 1;
						}
					} else {
						status = 0;
						found_nameservice = 1;
					}
				}
			}
			if(!found_nameservice && ns_passed_type != NS_NONE) {
				status = 1;
				break;
			}
			switch(table_type) {
			// char *null, char *netnum, char **num, char **mask
			// char **comment
			case MAP_NETMASK:
				{
				netmaskmap *nmp = nep->get_netmaskmap();

				if (nmp == (netmaskmap *)0) {
					status = 1;
					break;
				}

				char *netnum = va_arg(ap, char *);
				netmaskmap_iter nmip(*nmp);
				netmask_entry *nmep;
				
				nmep = nmip.find_by_key(netnum);
				if (nmep != (netmask_entry *)0) {
					status = 0;
					char *mask = va_arg(ap, char *);
					// *numret = nmep->get_netnum();
					strcpy(mask, nmep->get_netmask());
					// *com = nmep->get_comment();

				} else {
					status = 1;
				}
				}
				break;
			// char *null, char *hostname, char **name,
			// char **ipaddr
			case MAP_HOST:
				{
				hostmap *hmp = nep->get_hostmap();

				if (hmp == (hostmap *)0) {
					status = 1;
					break;
				}

				char *hostname = va_arg(ap, char *);

				hostmap_iter hip(*hmp);
				host_entry *hep, t(hostname, NULL, NULL);

				hep = hip.find_by_hentry(&t);
				if (hep != (host_entry *)0) {
					status = 0;
					char **name = va_arg(ap, char **);
					char **ipaddr = va_arg(ap, char **);
					*name = hep->get_hname();
					*ipaddr = hep->get_ipaddr();
				} else {
					status = 1;
				}
				}
				break;
			// char *null, char *hostname, char **name,
			// char **tz, char **comment
			case MAP_TIMEZONE:
				{
				timezonemap *tzmp = nep->get_timezonemap();

				if (tzmp == (timezonemap *)0) {
					status = 1;
					break;
				}

				char *hostname = va_arg(ap, char *);

				timezonemap_iter tzip(*tzmp);
				timezone_entry *tzep;

				tzep = tzip.find_by_key(hostname);
				if (tzep != (timezone_entry *)0) {
					status = 0;
					char *tz = va_arg(ap, char *);
					// *name = tzep->get_hname();
					strcpy(tz, tzep->get_tzone());
					// *com = nullstring;
				} else {
					status = 1;
				}
				}
				break;
			// char *null, char *hostname, char **name,
			// char **lc, char **comment
			case MAP_LOCALE:
				{
				localemap *lmp = nep->get_localemap();

				if (lmp == (localemap *)0) {
					status = 1;
					break;
				}

				char *hostname = va_arg(ap, char *);

				localemap_iter lmip(*lmp);
				locale_entry *lep;

				lep = lmip.find_by_key(hostname);
				if (lep != (locale_entry *)0) {
					status = 0;
					char *lc = va_arg(ap, char *);
					// *name = lep->get_hname();
					strcpy(lc, lep->get_locale());
					// *com = nullstring;
				} else {
					status = 1;
				}
				}
				break;

			// char *rootname, char **name, char **password,
			// char **uid, char **gid, char **gcos, char **path
			// char **shell, char **last, char **min
			// char **max, char **warn, char **inactive
			// char **expire, char **flag;
			case MAP_PASSWD:
				{
				passwordmap *pwmp = nep->get_passwordmap();

				if (pwmp == (passwordmap *)0) {
					status = 1;
					break;
				}

				char *rootname = va_arg(ap, char *);

				passwordmap_iter pwip(*pwmp);
				password_entry *pwep;

				pwep = pwip.find_by_login(rootname);
				if (pwep != (password_entry *)0) {
					status = 0;
					char **name = va_arg(ap, char **);
					char **password = va_arg(ap, char **);
					char **uid = va_arg(ap, char **);
					char **gid = va_arg(ap, char **);
					char **gcos = va_arg(ap, char **);
					char **path = va_arg(ap, char **);
					char **shell = va_arg(ap, char **);

					*name = pwep->get_login();
					*password = pwep->get_password();
					*uid = pwep->get_uidstring();
					*gid = pwep->get_gidstring();
					*path = pwep->get_homedir();
					*shell = pwep->get_shell();
				} else {
					status = 1;
				}
				}
				break;

			// Not called currently
			case MAP_ETHER:
			case MAP_BOOTPARAM:
			case MAP_UNSPECIFIED:
				status = 1;
				break;
			}
		}
		break;

	/* char *ns_type, int tableno, ... */
	case SIM_DB_MODIFY:
		{
			int ns_type = va_arg(ap, int);
			int tabletype = va_arg(ap, int);

			enum nsmaptype table_type = 
			    dbtabletype_to_nsmaptype(tabletype);

			enum ns_type ns_passed_type = 
			    ns_dbtype_to_type(ns_type);

			int found_nameservice = 0;
			nameservice *nep;

			// Look for a nameservice of the appropriate
			// type and fail if we can't find one
			if (nlp) {
				nameservicelist_iter nlip(*nlp);
				while (nep = nlip()) {
					if(nep->get_type() != ns_passed_type)
						continue;
					if (ns_passed_type == NS_NONE) {
						status = 0;
						found_nameservice = 1;
						break;
					}
					// Check that domainname and type match
					if ((shp->get_domtype() ==
					    ns_passed_type) &&
					    (strcmp(nep->get_domainname(),
					    shp->get_domainname()) == 0)) {
						status = 0;
						found_nameservice = 1;
						break;
					}
				}
			}
			if (!found_nameservice) {
				if (ns_passed_type != NS_NONE) {
					status = 1;
					break;
				} else {
					// UFS specified
					// Create a UFS name service
					// and populate it with empty maps
					nep = new nameservice(NS_NONE, "");
					ns_server *nssp = new ns_server;
					nep->set_ns_server(nssp);
					if (! nlp) {
						nlp = new nameservicelist;
						ssp->set_nslist(nlp);
					}
					nlp->insert(nep);
				}
			}

			switch(table_type) {
			// char *errmess, char *netnum, char **netnum,
			// char **netmask;
			case MAP_NETMASK:
				{
				netmaskmap *nmp = nep->get_netmaskmap();

				if (nmp == (netmaskmap *)0) {
					if (ns_passed_type == NS_NONE) {
						nmp = new netmaskmap;
						nep->set_netmaskmap(nmp);
					} else {
						status = 1;
						break;
					}
				}

				char *netnum = va_arg(ap, char *);
				char *val = va_arg(ap, char *);

				char *copy = strdup(val);
				char *netmask = strtok(copy, " \t\n");
				netmask = strtok(NULL, " \t\n");

				netmask_entry *nmep = new 
				    netmask_entry(netnum, netmask);
				
				nmp->insert(nmep);
				status = 0;
				}
				break;
				
			// char *errmess, char *hostname, char **hostname,
			// char **timezone
			case MAP_TIMEZONE:
				{
				timezonemap *tzmp = nep->get_timezonemap();

				char *errmess = va_arg(ap, char *);
				if (tzmp == (timezonemap *)0) {
					if (ns_passed_type == NS_NONE) {
						tzmp = new timezonemap;
						nep->set_timezonemap(tzmp);
					} else {
						sprintf(errmess, no_map_str,
						    nsmaptype_to_string(
						    table_type),
						    nstype_to_string(
						    ns_passed_type)
						    );
						status = 1;
						break;
					}
				}

				char *hostname = va_arg(ap, char *);
				char **hostp = va_arg(ap, char **);
				char **timezonep = va_arg(ap, char **);

				timezone_entry *tzep = new 
				    timezone_entry(hostname, *timezonep);
				
				tzmp->insert(tzep);
				status = 0;
				}
				break;


			// char *errmess, char *root_name, char **name,
			// char **pw, char **uid, char **gid, char **gcos
			// char **path, char **shell, char *last, char **min
			// char **max, char **warn, char **inactive
			// char **expire, char **flag
			case MAP_PASSWD:
				{
				passwordmap *pwmp = nep->get_passwordmap();

				char *errmess = va_arg(ap, char *);
				if (pwmp == (passwordmap *)0) {
					if (ns_passed_type == NS_NONE) {
						pwmp = new passwordmap;
						nep->set_passwordmap(pwmp);
					} else {
						sprintf(errmess, no_map_str,
						    nsmaptype_to_string(
						    table_type),
						    nstype_to_string(
						    ns_passed_type)
						    );
						status = 1;
						break;
					}
				}

				char *root_name = va_arg(ap, char *);
				char **name = va_arg(ap, char **);
				char **pw = va_arg(ap, char **);
				char **uidp = va_arg(ap, char **);
				char **gidp = va_arg(ap, char **);
				char **gcos = va_arg(ap, char **);
				char **path = va_arg(ap, char **);
				char **shell = va_arg(ap, char **);

				int uid = atoi(*uidp);
				int gid = atoi(*gidp);

				password_entry *pwep = new 
				    password_entry(root_name,
				    *pw, uid, gid, *gcos, *path, *shell);
				
				pwmp->insert(pwep);
				status = 0;
				}
				break;

			// char *errmess, char *ether_addr, char *hostname,
			// char **ether_addr, char **hostname
			case MAP_ETHER:
				{
				ethermap *emp = nep->get_ethermap();

				char *errmess = va_arg(ap, char *);
				if (emp == (ethermap *)0) {
					if (ns_passed_type == NS_NONE) {
						emp = new ethermap;
						nep->set_ethermap(emp);
					} else {
						sprintf(errmess, no_map_str,
						    nsmaptype_to_string(
						    table_type),
						    nstype_to_string(
						    ns_passed_type)
						    );
						status = 1;
						break;
					}
				}

				char *ether_addr = va_arg(ap, char *);
				char *hostname = va_arg(ap, char *);
				char **etherpp = va_arg(ap, char **);
				char **hostnamep = va_arg(ap, char **);

				ether_entry *eep = new 
				    ether_entry(hostname, ether_addr);
				
				emp->insert(eep);
				status = 0;
				}
				break;

			// char *errmess, char *hostname, char **ip_addr,
			// char **hostname, char **alias
			case MAP_HOST:
				{
				hostmap *hmp = nep->get_hostmap();

				if (hmp == (hostmap *)0) {
					if (ns_passed_type == NS_NONE) {
						hmp = new hostmap;
						nep->set_hostmap(hmp);
					} else {
						status = 1;
						break;
					}
				}

				char *hostname = va_arg(ap, char *);
				char *val = va_arg(ap, char *);

				char *copy = strdup(val);
				char *ip_addr = strtok(copy, " \t\n");
				char *alias = strtok(NULL, " \t\n");
				alias = strtok(NULL, " \t\n");

				host_entry *hep = new 
				    host_entry(hostname, ip_addr, alias);
				
				hmp->insert(hep);
				status = 0;
				}
				break;

			// char *errmess, char *hostname, char **hostname
			case MAP_BOOTPARAM:
				{
				bpmap *bpmp = nep->get_bpmap();

				char *errmess = va_arg(ap, char *);
				if (bpmp == (bpmap *)0) {
					if (ns_passed_type == NS_NONE) {
						bpmp = new bpmap;
						nep->set_bpmap(bpmp);
					} else {
						sprintf(errmess, no_map_str,
						    nsmaptype_to_string(
						    table_type),
						    nstype_to_string(
						    ns_passed_type)
						    );
						status = 1;
						break;
					}
				}

				char *hostname = va_arg(ap, char *);
				char **hostnamep = va_arg(ap, char **);

				bootparam_entry *bpep = new 
				    bootparam_entry(hostname, "");
				
				bpmp->insert(bpep);
				status = 0;
				}
				break;

			// Not called
			case MAP_LOCALE:
			case MAP_UNSPECIFIED:
				status = 1;
				break;
			}
		}
		break;
	default:
		fprintf(stderr, "simulate: got unknown Sim_type %d\n", type);
		status = FAILURE;
		break;
	}
	va_end(ap);
	return(status);
}
}
static int
get_net_attr(simhost *shp, enum ni_sgattr attr, char *ifname, char *attrstr,
    char *errmess)
{
	const char *lattrstring;
	ni_list *nilp;
	net_interface interface_to_match(ifname);
	net_interface *netp;
	int status;

	nilp = shp->get_ni_list();;

	nilist_iter my_iter(*nilp);
	netp = my_iter.find_by_interface(&interface_to_match);
	if (netp == (net_interface *)0) {
		sprintf(errmess, 
		    "%s network interface not found",
		    ifname);
		status = FAILURE;
	} else {
		switch(attr) {
		case nia_Enetaddr:
			lattrstring = netp->get_eaddr();
			break;
		case nia_Baddr:
			lattrstring = netp->get_baddr();
			break;
		case nia_Ipaddr:
			lattrstring = netp->get_ipaddr();
			break;
		case nia_Netmask:
			lattrstring = netp->get_netmask();
			break;
		case nia_Upflag:
			lattrstring = (const char *)malloc(80);
			sprintf((char *)lattrstring, 
			    "Upflag %d\n", netp->is_up());
			break;
		}

		if (lattrstring != (char *)0 && lattrstring[0] != '\0') {
			strcpy(attrstr, lattrstring);
			status = SUCCESS;
		} else {
			sprintf(errmess, 
		    "%s has no %s assigned",
			    ifname, ni_sgattr_to_str(attr));
			status = FAILURE;
		}
	}
	return(status);
}

static int
set_net_attr(simhost *shp, enum ni_sgattr attr, char *ifname, char *attrstr,
    char *errmess)
{
	ni_list *nilp;
	net_interface interface_to_match(ifname);
	net_interface *netp;
	int status;

	nilp = shp->get_ni_list();;

	nilist_iter my_iter(*nilp);
	netp = my_iter.find_by_interface(&interface_to_match);
	if (netp == (net_interface *)0) {
		sprintf(errmess, 
		    "%s network interface not found",
		    ifname);
		status = FAILURE;
	} else {
		switch(attr) {
		case nia_Enetaddr:
			netp->set_ether(attrstr);
			break;
		case nia_Baddr:
			netp->set_broadcast(attrstr);
			break;
		case nia_Ipaddr:
			netp->set_ipaddr(attrstr);
			break;
		case nia_Netmask:
			netp->set_netmask(attrstr);
			break;
		case nia_Upflag:
			if (strcmp(attrstr, ADMUTIL_UP) == 0)
				netp->up();
			else
				netp->down();
		}
		status = SUCCESS;
	}
	return(status);
}

static const char *
ni_sgattr_to_str(enum ni_sgattr attr)
{
	char *str;

	switch(attr)
	{
	case nia_Enetaddr:
		str = "ethernet address";
		break;
	case nia_Baddr:
		str = "broadcast address";
		break;
	case nia_Ipaddr:
		str = "ip address";
		break;
	case nia_Netmask:
		str = "netmask";
		break;
	case nia_Upflag:
		str = "upflag";
		break;
	}
	return (str);
}
static enum ns_type 
ns_dbtype_to_type(int ns_db_type)
{
	switch(ns_db_type) {
	case NISPLUS:
		return(NS_NISPLUS);
	case NIS:
		return(NS_NIS);
	case UFS:
		return(NS_NONE);
	case DB_NS_ALL:
	default:
		return(NS_UNSPECIFIED);
	}
	/* NOTREACHED */
}
static char *
type_to_nsvaltype(enum ns_type type)
{
	switch(type) {
	case NS_NISPLUS:
		return(DB_VAL_NS_NIS_PLUS);
	case NS_NIS:
		return(DB_VAL_NS_NIS);
	case NS_UNSPECIFIED:
	case NS_NONE:
		return(DB_VAL_NS_UFS);
	}
}

enum nsmaptype 
dbtabletype_to_nsmaptype(int tabletype)
{
	switch(tabletype){
	case PASSWD_TBL:
		return(MAP_PASSWD);
	case NETMASKS_TBL:
		return(MAP_NETMASK);
	case TIMEZONE_TBL:
		return(MAP_TIMEZONE);
	case LOCALE_TBL:
		return(MAP_LOCALE);
	case ETHERS_TBL:
		return(MAP_ETHER);
	case HOSTS_TBL:
		return(MAP_HOST);
	case BOOTPARAMS_TBL:
		return(MAP_BOOTPARAM);
	default:
		return(MAP_UNSPECIFIED);
	}
	/* NOTREACHED */
}
