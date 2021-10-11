
/*
 *  Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * sysIDtool_nisinit - nis/nis+ configuration tool.
 *
 * sysIDtool_nisinist is called directly from the system startup scripts.
 * Its default mode of behaviour is to check files in the /root filesystem
 * for the * system's configuration parameters. If these parameters are not
 * set, syIDtool consults network services first, and then prompts the user
 * for the configuration information for this system.
 */

#pragma	ident	"@(#)sysidnis.c	1.66	96/06/14 SMI"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <sys/socket.h>
#include <string.h>
#include "cl_database_parms.h"
#include "sysidtool.h"
#include "sysid_msgs.h"
#include "prompt.h"
#include "sysid_preconfig.h"

#define	BAD_USAGE	-1

#define	NSSWITCH_CONF		"/etc/nsswitch.conf"
#define	NSSWITCH_FILES		"/etc/nsswitch.files"
#define	NSSWITCH_NIS		"/etc/nsswitch.nis"
#define	NSSWITCH_NISPLUS	"/etc/nsswitch.nisplus"

extern char name_server_name[];
extern char name_server_addr[];

FILE *debugfp;

/*
 * Local variables
 */

/* used for name service initialization & error handling */
static int		ns_configuration_done = 0;
static char		ns_type[40];
static char		domainname[MAX_DOMAINNAME+2];

/*
 * Local routines
 */
static	void	done(int status, int call_close, int ypkill);
static	int	system_configured(int *,int *,int *,int *,
		int *,int *,char *);
static	void	prt_switch_err(char *);

int
main(int argc, const char *argv[])
{
	int		sys_bootparamed, sys_networked;
	char		errmess[1024];
	char		if_name[17], netnum[MAX_NETMASK+2];
	int		ns_item = 0;
	char		*name_services[4];
	int		num_name_services;
	int		subnetted = 0;
	int		subnet_item;
	char		netmask[MAX_NETMASK+3];
	int		syspasswd;
	int		syslocale;
	char		termtype[MAX_TERM+2];
	char		tmpstr[MAX_TERM+11];
	int		confirmed;
	int		configured;
	int		num_items;
	Confirm_data	list_data[7];
	Prompt_t	prompt_id;
	extern char	*optarg;
	char		*test_input = (char *)0;
	char		*test_output = (char *)0;
	int		c;
	int		status;
	int		bcast_to_find_name_server = 1;
	int		domain_set = 0;
	int		use_ns = 0;
	int		manual_override;
	int		subnet_done;
	int		ypkill = 1;
	char		*ns_ptr;
	char		*domain_ptr;
	char		*name_server_ptr;
	char		*netmask_ptr;

	/* need to call setlocale() even though lookup_locale() returns later */
	(void) setlocale(LC_ALL, "");

	while ((c = getopt(argc, (char **) argv, "yO:i:I:")) != EOF) {
		switch (c) {
		case 'y':
			ypkill = 0;
			break;
		case 'O':
			test_enable();
			sim_load(optarg);
			break;
		case 'i':
			test_input = optarg;
			break;
		case 'I':
			test_output = optarg;
			break;
		default:
			/* Ignore unknown options */
			break;
		}
	}

	if (testing && (test_input == (char *)0 || test_output == (char *)0)) {
		fprintf(stderr,
	    "%s: -O <name> must be accompanied by -I <name> and -i <name>\n",
			argv[0]);
		exit(1);
	}

	if (testing) {
		if (sim_init(test_input, test_output) < 0) {
			fprintf(stderr,
			    "%s: Unable to initialize simulator\n",
			    argv[0]);
			exit(1);
		}
	}

	debugfp = open_log("sysidnis");

	(void) lookup_locale();

	if (system_configured(&sys_bootparamed, &sys_networked, 
		&ns_configuration_done,
	    &subnetted, &syspasswd, &syslocale, termtype) == TRUE) {
		done(SUCCESS, 0, ypkill);
	}
	if (sys_networked == FALSE) {
		done(SUCCESS, 0, ypkill);
	}

	fprintf(debugfp, "nisinit: system is not configured\n");

	if (ns_configuration_done) {
		fprintf(debugfp, "nisinit: NS already configured\n");
		done(SUCCESS, 0, ypkill);
	}

	/*
	 * Set the terminal type.
	 */

	(void) sprintf(tmpstr, "TERM=%s", termtype);
	(void) putenv(tmpstr);

	/* Start the user interface (AFTER reading the term and locale) */
	if (prompt_open(&argc, (char **)argv) < 0) {
		(void) fprintf(stderr, 
			"%s: Couldn't start user interface\n",argv[0]);
		exit(1);
	}
	/*
	 * Set up name_services array
	 */
	name_services[0] = NIS_VERSION_3;
	name_services[1] = NIS_VERSION_2;
	name_services[2] =  OTHER_NAMING_SERVICE;
	name_services[3] =  NO_NAMING_SERVICE;
	num_name_services = 4;

	/*
	 * Name service is not configured. Proceed with the configuration.
	 */

	subnet_item = 0;
	netmask[0] = 0;
	ns_item = 0;
	ns_type[0] = NULL;
	name_server_name[0] = NULL;
	name_server_addr[0] = NULL;

	prompt_id = (Prompt_t)0;

	/*
	 * Get the system's IP address and network number.
	 */

	get_net_name_num(if_name, netnum);

	manual_override = bootargset("noautons");

 
    /* 
     * Attempt to read in the sysidtool configuration
     * file and determine if there are any configuration
     * variables that are pertinent to this application
     *
     */
    if (!manual_override && read_config_file() == SUCCESS) {

	fprintf(debugfp, "sysidnis:  Using configuration file\n");

        /* Get the name service information if it has been specified */
        ns_ptr = get_preconfig_value(CFG_NAME_SERVICE,NULL,NULL);
        if (ns_ptr != NULL) {
		ns_configuration_done = 1;
		if (strcasecmp(ns_ptr, "nis") == 0) {
			(void) strcpy(ns_type, NIS_VERSION_2);
			use_ns = 1;
		}
		else if (strcasecmp(ns_ptr, "nisplus") == 0) {
			(void) strcpy(ns_type, NIS_VERSION_3);
			use_ns = 1;
		}
		else if (strcasecmp(ns_ptr, "none") == 0 ||
				 strcasecmp(ns_ptr,"other") == 0) {
			(void) strcpy(ns_type, NO_NAMING_SERVICE);
			use_ns = 0;
		}

		/* Access the dependent parameters only if a name */
		/* service is to be used                          */
		if (use_ns) {
			/* attempt to get the domain name */
			domain_ptr = get_preconfig_value(CFG_NAME_SERVICE, 
				ns_ptr, CFG_DOMAIN_NAME);
			if (domain_ptr != NULL) {
				strcpy(domainname, domain_ptr);
				domain_set = 1;
			} else {
				get_net_domainname(domainname);
				if (domainname[0])
					domain_set = 1;
			}

			/* attempt to get the name server if specified */
			name_server_ptr = get_preconfig_value(CFG_NAME_SERVICE, 
					ns_ptr, CFG_NAME_SERVER_NAME);
			if (name_server_ptr != NULL) {
				ns_configuration_done = -1;
				strcpy(name_server_name, name_server_ptr);
				name_server_ptr = get_preconfig_value(
					CFG_NAME_SERVICE,ns_ptr,CFG_NAME_SERVER_ADDR);
				if (name_server_ptr != NULL) {
					strcpy(name_server_addr, name_server_ptr);
				}
				bcast_to_find_name_server = 0;
			}
		}

        }

        /* Get the netmask information from the default interface */
        /* Default means primary.  It is currently possible to    */
        /* configure only the primary.  The ability to configure  */
        /* all of the network interaces is planned                */
        netmask_ptr = get_preconfig_value(CFG_NETWORK_INTERFACE,
				CFG_DEFAULT_VALUE,CFG_NETMASK);
        if (netmask_ptr != NULL) {
            strcpy(netmask,netmask_ptr);
        }

	/* Print the preconfiguration values found to the 
	 * sysidtool log file for use by test to validate
	 * that the correct information is being passed in
	 */
	if (ns_configuration_done) {
		fprintf(debugfp,"  name service: %s\n",ns_type);

		if (use_ns) {
			if (domain_ptr != NULL)
				fprintf(debugfp,"    domain name: %s\n",
					 domainname);
			if (name_server_ptr != NULL) {
				fprintf(debugfp,
					"    name server name: %s\n",
					name_server_name);
				fprintf(debugfp,
					"    name server addr: %s\n",
					name_server_addr);
			}
		}

		if (netmask_ptr != NULL)
			fprintf(debugfp,"  netmask: %s\n",netmask);
	}
    }


	/*
	 * Get the domainname from the network interface, if this
	 * was retrieved via the bootparams.  If a domainname is found,
	 * they try autobind to find a server.
	 */
	if (sys_bootparamed == TRUE) {
		if (!domain_set)
			get_net_domainname(domainname);

		if (domainname[0]) {
			domain_set = 1;
			if (! manual_override) {
			/*
			** autobind will return the following values
			** 0 - no name service information determined
			** 1 - name service determined and possibly netmask
			** -1 - name service and name server found and
			**      possibly netmask
			*/
				ns_configuration_done = 
					autobind(domainname, 
					if_name, ns_type, netmask);
			}

			if (ns_configuration_done && 
			   strcmp(ns_type, NO_NAMING_SERVICE) != 0)
				use_ns = 1;
			if (ns_configuration_done < 0)
				bcast_to_find_name_server = 0;
		}
	} else {
		(void) strcpy(domainname, NO_DOMAIN);
	}

	if (netmask[0])
		subnetted = 1;
	else
		(void) strcpy(netmask, DEFAULT_NETMASK);

	subnet_done = subnetted;

	/*
	 * Ask the user for any name service configuration information that
	 * could not be determined from the network, and then configure
	 * the name service (if any).  If an error occurs during the
	 * configuration, re-prompt for the configuration information.
	 */

	configured = FALSE;
	while (!configured) {

		/*
		 * Prompt for any information not available on the
		 * network.  Keep re-prompting until the user positively
		 * confirms their input.
		 */

		sync();
		confirmed = FALSE;
		while (!confirmed) {
			int must_prompt;

			must_prompt = manual_override || !ns_configuration_done;

			num_items = 0;

			/*
			 * Prompt for name service type.
			 */

			if (must_prompt) {
				ns_item = prompt_name_service(name_services,
				    num_name_services, ns_item);
				list_data[num_items].field_attr =
				    ATTR_NAME_SERVICE;
				list_data[num_items].value =
				    name_services[ns_item];
				list_data[num_items++].flags = 0L;
				/*
				 * XXX [shumway]
				 *
				 * This is really horrible.  Items 2 and 3 are
				 * "Other" and "None" respectively.
				 * Programmatically they are the same but
				 * they need to be distinct
				 * choices from the user's point of view.
				 * To make * them behave the same, we
				 * "translate" choice 2 into choice 3.
				 * We leave ns_item alone so that the user
				 * sees their original choice on subsequent 
				 * cycles through the program.
				 */
				if (ns_item == 2)
					strcpy(ns_type, name_services[3]);
				else
					strcpy(ns_type, name_services[ns_item]);

				if (ns_item < 2)
					use_ns = 1;
				else
					use_ns = 0;

			} /* Else ns_type was set up by autobind */

			/*
			 * Prompt for domainname.
			 */

			if (! domain_set && use_ns) {
				prompt_domain(domainname);
				list_data[num_items].field_attr = ATTR_DOMAIN;
				list_data[num_items].value = domainname;
				list_data[num_items++].flags = 0L;

				if (strcmp(domainname, NO_DOMAIN) != 0)
					domain_set = 1;
			}

			/*
			 * Prompt to determine if we should use broadcast
			 * to locate the server.
			 */

			if (use_ns && must_prompt) {
			/*
			 * choice 0 is for bcast_to_find_name_server, 
			 * 1 is select, thus
			 * the negation on the bcast_to_find_name_server flag.
			 */
				bcast_to_find_name_server = 
					!prompt_broadcast(
					!bcast_to_find_name_server);
				fprintf(debugfp, "broadcast %d\n", 
					bcast_to_find_name_server);
				list_data[num_items].field_attr =
				    ATTR_BROADCAST;
				list_data[num_items].value =
				    (bcast_to_find_name_server ? BROADCAST_S : SPECNAME_S);
				list_data[num_items++].flags = 0L;
			}

			/*
			 * Prompt for server name and IP address.
			 */

			if (use_ns && ! bcast_to_find_name_server && must_prompt) {
				prompt_nisservers(name_server_name,
				    name_server_addr);
				list_data[num_items].field_attr =
				    ATTR_NISSERVERNAME;
				list_data[num_items].value = name_server_name;
				list_data[num_items++].flags = 0L;
				list_data[num_items].field_attr =
				    ATTR_NISSERVERADDR;
				list_data[num_items].value = name_server_addr;
				list_data[num_items++].flags = 0L;
			}

			/*
			 * Prompt for netmask (so that we can contact the name
			 * server in case it is not on the local subnet).
			 * Only prompt if we can't get the netmask from the
			 * name service automicatlly in sysidsys.  This would
			 * happen if server is not on local subnet.
			 */

			if (must_prompt && 
				!bcast_to_find_name_server && !subnet_done) {

				subnet_done = 1;

				subnetted = subnet_item =
				    prompt_isit_subnet(subnet_item);
				if (subnetted) {
					list_data[num_items].field_attr =
					    ATTR_SUBNETTED;
					list_data[num_items].value = YES;
					list_data[num_items++].flags = 0L;
					prompt_netmask(netmask);
					list_data[num_items].field_attr =
					    ATTR_NETMASK;
					list_data[num_items].value = netmask;
					list_data[num_items++].flags = 0L;
				} else {
					list_data[num_items].field_attr =
					    ATTR_SUBNETTED;
					list_data[num_items].value = NO;
					list_data[num_items++].flags = 0L;
				}
			}

			/*
			 * Confirm the name service information 
			 * entered by the user.
			 */

			if (num_items > 0) {
				confirmed = prompt_confirm(list_data,
				    num_items);
			} else {
				confirmed = TRUE;
			}

			if (!confirmed) {
				domain_set = 0;
			}
		}

		/*
		 * Put up a status message telling the user 
		 * we're working on it
		 */
		prompt_id = prompt_message((Prompt_t)0, GOODBYE_NISINIT);

		/*
		 * Now configure the system using the information just
		 * collected.
		 */

		/*
		 * Configure the system's netmask (if potentially needed
		 * to contact a name server not on the local subnet).
		 * Set the netmask in the /etc/netmasks file, and on the
		 * network interface.
		 */

		if (subnetted)
			do_netmask(netmask, netnum, if_name);

		/*
		 * Save any state changes to the sysIDtool state file.
		 */

		put_state(FALSE, sys_bootparamed, sys_networked, use_ns,
			subnet_done, syspasswd, syslocale, termtype);

		if ((sys_bootparamed && !domain_set) || ! use_ns) {
			ns_type[0] = NULL;
			use_ns = 0;
		}

		/* if we autobound, then broadcast works */
		/* bcast_to_find_name_server |= ns_configuration_done; */

		if ((status = init_ns(ns_type, domainname, 
			bcast_to_find_name_server,
		    name_server_name, name_server_addr, errmess))
		    != INIT_NS_OK) {
			int reprompt = 1;

			switch (status) {
			case INIT_NS_SWITCH:
				prt_switch_err(errmess);
				break;
			case INIT_NS_DOMAIN:
				prompt_error(SYSID_ERR_BAD_DOMAIN, domainname,
					errmess);
				break;
			case INIT_NS_HOSTS:
				prompt_error(SYSID_ERR_BAD_NISSERVER_ENT,
				    name_server_name, name_server_addr,
				    errmess);
				break;
			case INIT_NS_ALIASES:
				prompt_error(SYSID_ERR_BAD_YP_ALIASES,
					domainname, errmess);
				break;
			case INIT_NS_BIND:
				if (bcast_to_find_name_server)
					prompt_error(SYSID_ERR_BAD_YP_BINDINGS1,
					    domainname, errmess);
				else
					reprompt = prompt_bad_nis(
					    SYSID_ERR_BAD_YP_BINDINGS2,
					    domainname, errmess);
				break;
			case INIT_NS_NISP:
				if (bcast_to_find_name_server)
					reprompt = prompt_bad_nis(
					    SYSID_ERR_BAD_NIS_SERVER1,
					    domainname, errmess,
					    name_server_name);
				else
					reprompt = prompt_bad_nis(
					    SYSID_ERR_BAD_NIS_SERVER2,
					    name_server_name, domainname,
					    errmess, name_server_name);
				break;
			case INIT_NS_NISP_ACC:
				reprompt = prompt_bad_nis(
				    SYSID_ERR_NIS_SERVER_ACCESS,
				    domainname);

				if (reprompt)
					unlink("/var/nis/NIS_COLD_START");
				break;
			case INIT_NS_YPSRV:
				reprompt = prompt_bad_nis(
					    SYSID_ERR_BAD_YP_BINDINGS2,
					    domainname, errmess);

				if (reprompt) {
					char yp_file[MAXPATHLEN];

					sprintf(yp_file,
					    "/var/yp/binding/%s/cache_binding",
					    domainname);
					unlink(yp_file);
					sprintf(yp_file, "/var/yp/binding/%s",
					    domainname);
					rmdir(yp_file);

					kill_ypbind();
				}
				break;
			}

			/* init failed, prompt for all info & retry */
			if (reprompt) {
				sys_bootparamed = FALSE;
				ns_configuration_done = FALSE;
				domain_set = 0;
				continue;
			}
		}

		configured = TRUE;
	}

	if (prompt_id != (Prompt_t)0)
		prompt_dismiss(prompt_id);

	/*
	 * Clean up the terminal interface and exit.
	 */
	done(SUCCESS, 1, ypkill);

	/*
	 * Never reached.
	 */
	return (SUCCESS);
}

/*
 * print error from configuring the /etc/nsswitch.conf file.
 */
static void
prt_switch_err(char *errmess)
{
	if ((ns_type[0] == NULL) || (strcmp(ns_type, NO_NAMING_SERVICE) == 0)) {
		prompt_error(SYSID_ERR_NSSWITCH_FAIL1, errmess,
			NSSWITCH_FILES, NSSWITCH_CONF);
	} else if (strcmp(ns_type, NIS_VERSION_2) == 0) {
		prompt_error(SYSID_ERR_NSSWITCH_FAIL1, errmess,
			NSSWITCH_NIS, NSSWITCH_CONF);
	} else if (strcmp(ns_type, NIS_VERSION_3) == 0) {
		prompt_error(SYSID_ERR_NSSWITCH_FAIL1, errmess,
			NSSWITCH_NISPLUS, NSSWITCH_CONF);
	} else {
		prompt_error(SYSID_ERR_NSSWITCH_FAIL2, errmess);
	}
}

static int
system_configured(int *sys_bootparamed, int *sys_networked, int *sys_autobound,
    int *sys_subnetted, int *sys_passwd, int *sys_locale, char *term_type)
{
	int sys_configured;
	int err_num;

	/*
	 * To see if this system is configured, check the state kept by
	 * the sysIDtool_netinit program.
	 */
	if (get_state(&sys_configured, sys_bootparamed, sys_networked,
	    sys_autobound, sys_subnetted, sys_passwd, sys_locale,
	    term_type, &err_num) == FAILURE)
		sys_configured = 0;

	return (sys_configured);
}

void
usage()
{
	(void) fprintf(stderr, "system config error: bad usage.\n");
}

static void
done(int status, int call_close, int ypkill)
{
	if (ypkill)
		kill_ypbind();
	fprintf(debugfp, "nisinit: done\n");
	/*
	 * Clean up the curses interface.
	 */
	if (call_close)
		(void) prompt_close("", 0);
	fprintf(debugfp, "sysidnis end\nfree VM: %d\n\n", free_vm());
	sync();
	exit(status);
}
