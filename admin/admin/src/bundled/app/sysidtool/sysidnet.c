
/*
 *  Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * sysIDtool_netinit - network interface configuration tool.
 *
 * sysIDtool_netinit is called directly from the system startup scripts.
 * Its default mode of behaviour is to check files in the /root filesystem
 * for the system's configuration parameters. If these parameters are not set,
 * syIDtool_netinit consults network services first, and then prompts the
 * user for the configuration information for this system.
 */

#pragma ident   "@(#)sysidnet.c 1.78 96/10/10"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sysidtool.h"
#include "cl_database_parms.h"
#include "sysid_msgs.h"
#include "prompt.h"
#include "sysid_preconfig.h"


extern char name_server_name[];
extern char name_server_addr[];

#define	SUN_CONSOLE	"sun"

/*
 * Local routines
 */
static	void	done(int status, int call_done, int ypkill, int locale_only);
static	int	system_configured(int *nis_done, int *sys_locale,
			char *termtype, int locale_only);
static void	fix_inittab(int, char *);

FILE *debugfp;

/*
 * Externals referenced.
 */
extern	int	errno;

int
main(int argc, const char *argv[])
{
	char	if_name[17];
	int		auto_network_interface;
	int		auto_all_network_info;
	int		auto_networked;
	int		auto_netmask;
	int		auto_locale;
	int		auto_terminal;
	int		c, try_autobind = 0;
	int		is_standalone;
	int		networked;
	int		auto_ip_addr;
	char	ip_addr[MAX_IPADDR+2];
	int		auto_hostname;
	char	hostname[MAX_HOSTNAME+2];
	char	domainname[MAX_DOMAINNAME+2];
	int		confirmed;
	int		configured;
	Confirm_data	list_data[3];
	int		num_items;
	char		termtype[MAX_TERM+2];
	char		tmp_str[MAX_TERM+11];
	char		tmpstr[MAX_TERM+11];
	char		*tp;
	char		ns_type[40];
	char		ns[40];
	int		nis_done = FALSE;
	int		must_ask_locale = 0;
	int		locale_done = FALSE;
	char		locale[MAX_LOCALE+1];
	int		lang_item;
	int		locale_item;
	int		num_ifs;
	struct if_list	*net_ifs, *pifs;
	char		**if_names;
	int		i, j, if_choice = 0;
	Prompt_t	prompt_id;
	char		*test_input = (char *)0, *test_output = (char *)0;
	char		errmess[1024];
	char		netmask[MAX_NETMASK+3];
	int		locale_only = 0;
	int		parse_only = 0;
	int		ypkill = 1;
	int		no_multibyte = 0;
	char		arch[100];
	int     nlocales;
	int     nlangs;
	char    **langp;
	int		noauto;
	char	*ni_ptr;
	char	*netmask_ptr;
	char	*hostname_ptr;
	char	*ip_addr_ptr;
	char	*locale_ptr;
	char	*terminal_ptr;

	if_name[0] = NULL;
	auto_network_interface = FALSE;
	auto_all_network_info = FALSE;
	auto_networked = FALSE;
	auto_netmask = FALSE;
	auto_locale = FALSE;
	auto_terminal = FALSE;
	is_standalone = 0;
	networked = FALSE;
	auto_ip_addr = FALSE;
	ip_addr[0] = NULL;
	auto_hostname = FALSE;
	hostname[0] = NULL;
	netmask[0] = NULL;
	locale[0] = NULL;
	lang_item = -1;
	locale_item = -1;
	

	prompt_id = (Prompt_t)0;

	debugfp = open_log("sysidnet");

	/*
	 * Need to do setlocale() even lookuplocale() returns 0 later.
	 * This setlocale() may be overwritten later.
	 */
	(void) setlocale(LC_ALL, "");

	while ((c = getopt(argc, (char **) argv, "ymlpO:i:I:")) != EOF) {
		switch (c) {
		case 'y':
			ypkill = 0;
			break;
		case 'm':
			no_multibyte = 1;
			break;
		case 'l':
			locale_only++;
			break;
		case 'p':
			parse_only = 1;
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

	if (parse_only) {
		read_config_file();
		exit(0);
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

	/*
	 * First check to see if this system is configured. For now this is a
	 * black and white decision; if the hostname is set (and sysIDtool
	 * previously ran), then we exit sysIDtool. In future versions, we
	 * will check each parameter so that you could have a partially
	 * configured system.
	 */
	termtype[0] = '\0';
	if (system_configured(&nis_done, &locale_done, termtype, locale_only)
	    == TRUE) {
		done(FAILURE, 0, ypkill, locale_only);
	}

	fprintf(debugfp, "locale_done: %d nis_done: %d term: %s\n",
		locale_done, nis_done,
		(termtype[0] == '\0' ? "Null" : termtype));

	/*
	 * This system is not configured. Proceed with the configuration.
	 *
	 * Try to automatically determine as much configuration
	 * information as possible. First, get the name of the first
	 * ethernet interface.  Then, check to see if an IP address has
	 * been set for that interface. If so, then the RARP for this
	 * address succeeded; this RARP is done by the booting procedure
	 * that occurs before sysIDtool is called.  If the RARP succeeded,
	 * then also try to get other associated parameters (such as the
	 * hostname set on the I/F).
	 */

	 noauto = bootargset("noauto");

	/* 
     * Attempt to read in the sysidtool configuration
     * file and determine if there are any configuration
     * variables that are pertinent to this application
     *
     */
    if (!noauto && read_config_file() == SUCCESS) {

		fprintf(debugfp, "sysidnet: Using configuration file\n");

        /* Get the network interface if it has been specified */
        ni_ptr = get_preconfig_value(CFG_NETWORK_INTERFACE,NULL,NULL);
		if (ni_ptr != NULL) {
			auto_networked = TRUE;
			if (strcasecmp(ni_ptr, "none") == 0) {
				auto_networked = FALSE;
			}

			/* Only get the rest of the network information if a */
			/* has been specified and the system will be networked */
			if (auto_networked) {
				auto_network_interface = TRUE;
				strcpy(if_name,ni_ptr);

				ip_addr_ptr = get_preconfig_value(CFG_NETWORK_INTERFACE,
					if_name,CFG_IP_ADDRESS);
				if (ip_addr_ptr != NULL) {
					networked = TRUE;
					auto_ip_addr = TRUE;
					strcpy(ip_addr,ip_addr_ptr);
				}

				hostname_ptr = get_preconfig_value(
					CFG_NETWORK_INTERFACE,if_name,CFG_HOSTNAME);
				if (hostname_ptr != NULL) {
					auto_hostname = TRUE;
					strcpy(hostname,hostname_ptr);
				}

				netmask_ptr = get_preconfig_value(CFG_NETWORK_INTERFACE,
					if_name,CFG_NETMASK);
				if (netmask_ptr != NULL) {
					auto_netmask = TRUE;
					strcpy(netmask,netmask_ptr);
				}
				
				if (auto_ip_addr && auto_hostname)
					auto_all_network_info = TRUE;
			}
        }

        /* Get the system locale if it has been specified */
        locale_ptr = get_preconfig_value(CFG_SYSTEM_LOCALE,NULL,NULL);
        if (locale_ptr != NULL) {
		auto_locale = TRUE;
		locale_done = 1;
		strcpy(locale,locale_ptr);
		/*
		 * If the multibyte flag is passed in do not
		 * set locale in the init file.  It will be
		 * done in the next call to sysidnet.
		 */
		if (no_multibyte) {
			fprintf(debugfp, "no multibyte\n");
		} else {
			save_locale(locale);
		}
	}
        /* Get the console terminal type if it has been specified */
        terminal_ptr = get_preconfig_value(CFG_TERMINAL,NULL,NULL);
        if (terminal_ptr != NULL) {
            auto_terminal = TRUE;
            strcpy(termtype,terminal_ptr);
		}


		fprintf(debugfp,"sysidnet: Configuration values specified:\n");
		if (auto_networked) {
			fprintf(debugfp,"  network interface: %s\n",ni_ptr);
			if (auto_ip_addr)
				fprintf(debugfp,"    ip address: %s\n",ip_addr);
			if (auto_hostname)
				fprintf(debugfp,"    hostname: %s\n",hostname);
			if (auto_netmask)
				fprintf(debugfp,"    netmask: %s\n",netmask);

		} else {
			if (ni_ptr != NULL)
				fprintf(debugfp,"  network interface: none\n");
		}

		if (auto_locale)
			fprintf(debugfp,"    locale: %s\n",locale);
		if (auto_terminal)
			fprintf(debugfp,"    terminal: %s\n",termtype);

    }
       

	/*
	 * First get the I/F name.  This will return the name of the first
	 * "up" interface.  At this point, an interface will be "up"
	 * if the system was booted from the interface or the hostconfig
	 * succeeded.  This applies for diskless clients, network installs
	 * and locally booted systems with ice cream configured.
	 */
	if (!noauto ) {

		/*
		** If the network interface has been specifed in the config
		** file or if there is an interface physically present then
		** attempt to determine the hostname and ip address
		*/
		if ((auto_network_interface && auto_networked) 
			|| get_net_if_name(if_name)) {
			/*
			 * Now get the IP address, if it is set on the i/f. This
			 * method takes the i/f name as input.
			 */
			if (!auto_ip_addr && 
				get_net_ipaddr(if_name, ip_addr) == SUCCESS) {

				/*
				 * Got the IP address. This means the RARP worked.
				 */
				networked = TRUE;
				auto_ip_addr = TRUE;
			}

			/*
			 * Now get the hostname from the i/f.
			 */
			if (!auto_hostname && auto_ip_addr &&
				get_net_hostname(hostname) == SUCCESS) {

				auto_hostname = TRUE;
				auto_all_network_info = TRUE;

				/*
				 * Since we got the hostname, the bootparams
				 * done by the booting procedure before
				 * sysIDtool was called succeeded. Get the
				 * domainname and associated parameters.
				 */
				get_net_domainname(domainname);

				try_autobind = 1;
			}
		}
	}

	/*
	 * Ask the user for any configuration information that could
	 * not be determined from the network, and then configure the
	 * network interface.  If an error occurs in the configuration,
	 * re-prompt for the information.
	 *
	 * First get the users locale, then the system's default terminal
	 * type for the console.  Check the environment and also probe to
	 * see if this is a Sun frame buffer.
	 */

	if (!locale_done && lookup_locale())
		locale_done = 1;

	do_locale();
	nlocales = get_num_locales();

	sysinfo(SI_ARCHITECTURE, arch, (long) 100);

	if (locale_done && strcmp(arch, "sparc") != 0 && !locale_only &&
	    nlocales > 1 && getenv("DISPLAY")) {
		/*
		 * We're running on an Intel or PPC machine in the second call
		 * to sysidnet with a localized image and kdmconfig has been
		 * run so that we are now in the window system.  The first
		 * call will be with the locale_only flag set, so
		 * that we can get the locale to use for kdmconfig.  However,
		 * if we are on a multi-byte image, then the first call will
		 * always use C as the locale, since we can't use multi-byte
		 * in a CUI.  Now that we are in the second phase of sysidnet,
		 * we should retry the locale stuff if we are on a multi-byte
		 * image, because we can now properly display the multi-byte
		 * prompts in the window system.
		 */

		nlangs = get_lang_strings(&langp);
		for (i = 0; i < nlangs; i++)
			for (j = 0; mb_locales[j]; j++)
				if (strcmp(langp[i], mb_locales[j]) == 0) {
					/* force retry of L10N prompt */
					locale_done = 0;
					i = nlangs;
					break;
				}
		free_lang_strings(&langp);
	}

	/*
	 * If the multibyte flag is passed in, we're not supposed to prompt
	 * for a locale.
	 */
	if (no_multibyte) {
		fprintf(debugfp, "no multibyte\n");
		locale_done = 1;
	}

	if (!locale_done && nlocales > 1) {
		int	autobound = 0;

		must_ask_locale = 1;

		if (nis_done) {
			system_namesrv(domainname, ns);
			if (get_entry(conv_ns(ns), LOCALE_TBL,
			    hostname, locale) == SUCCESS)
				must_ask_locale = 0;
			else if (get_entry(conv_ns(ns), LOCALE_TBL,
			    domainname, locale) == SUCCESS)
				must_ask_locale = 0;

		} else {
			/*
			 * see if a name service is available to
			 * use for locale
			 */
			if (try_autobind) {
				autobound = autobind(domainname, if_name,
					ns_type, netmask);
			}
			if (autobound) {
				if (netmask[0]) {
					char	netnum[MAX_NETMASK+2];

					get_net_name_num(if_name, netnum);
					do_netmask(netmask, netnum, if_name);
				}

				switch (init_ns(ns_type, domainname,
				    (autobound < 0) ? 0 : 1, name_server_name,
				    name_server_addr, errmess)) {
				case INIT_NS_OK:
					nis_done = TRUE;
					/*
					 * Try to get the system's locale based
					 * on it's hostname or domainname.
					 */
					if (strcmp(ns_type, NIS_VERSION_3) == 0)
						(void) strcpy(ns,
							DB_VAL_NS_NIS_PLUS);
					else
						(void) strcpy(ns,
							DB_VAL_NS_NIS);

					if (get_entry(conv_ns(ns), LOCALE_TBL,
					    hostname, locale) == SUCCESS)
						must_ask_locale = 0;
					else if (get_entry(conv_ns(ns),
					    LOCALE_TBL, domainname, locale)
					    == SUCCESS)
						must_ask_locale = 0;
					break;

				case INIT_NS_NISP_ACC:
					unlink("/var/nis/NIS_COLD_START");
					break;

				case INIT_NS_YPSRV:
					{
					char yp_file[MAXPATHLEN];

					sprintf(yp_file,
					    "/var/yp/binding/%s/cache_binding",
						domainname);
					unlink(yp_file);
					sprintf(yp_file, "/var/yp/binding/%s",
						domainname);
					rmdir(yp_file);
					}

					kill_ypbind();
					break;
				}
			}
		}

		/* If we get a locale from the name service and we are not
		 * in the window system, then check if we got a multi-byte
		 * locale.  If so, don't use it, since we can't display
		 * multi-byte locales.
		 */
		if (must_ask_locale == 0 && !getenv("DISPLAY")) {
			for (i = 0; mb_locales[i]; i++)
				if (strcmp(locale, mb_locales[i]) == 0) {
					must_ask_locale = 1;
					locale[0] = 0;
					break;
				}
		}
	}

	/*
	 * If we're on an Asian image, set the locale before starting
	 * the UI process.  Note that this relies on the Asian images
	 * consisting of only English and the one Asian locale.
	 * Setting it now insures that the UI will be run with the right
	 * font set loaded for all prompts.
	 */
	if (!no_multibyte && getenv("DISPLAY")) {
		nlangs = get_lang_strings(&langp);
		for (i = 0; i < nlangs; i++)
			for (j = 0; mb_locales[j]; j++)
				if (strcmp(langp[i], mb_locales[j]) == 0) {
					set_lang(mb_locales[j]);
					i = nlangs;
					break;
				}
		free_lang_strings(&langp);
	}

	/*
	 * Start the user interface
	 */
	if (prompt_open(&argc, (char **)argv) < 0) {
		(void) fprintf(stderr, "%s: Couldn't start user interface\n",
		    argv[0]);
		exit(1);
	}

	if (must_ask_locale) {
		prompt_locale(locale, &lang_item, &locale_item);
	}

	if (!locale_done && locale[0])
		save_locale(locale);

	/*
	 * If sysidnet -l has been run, the terminal type will already
	 * be available.
	 * If we are running in the window system, the console (sun-cmd)
	 * will have set the terminal type in the environment.
	 */
	if (termtype[0] != '\0') {
		prompt_terminal(termtype);
		(void) sprintf(tmpstr, "TERM=%s", termtype);
		(void) putenv(tmpstr);
	} else if ((tp = getenv("TERM")) == NULL) {

		fprintf(debugfp, "TERM not set\n");

		/*
		 * termtype[0] is '\0' here. ws_is_cons fills in termtype
		 * with the correct type if we're on the console.
		 * call prompt_terminal so the UI process can either
		 * prompt or simply be notified.
		 */
		(void)ws_is_cons(termtype);

		prompt_terminal(termtype);
		(void) sprintf(tmp_str, "TERM=%s", termtype);
		fprintf(debugfp, "putting \"%s\" into environment\n", tmp_str);
		(void) putenv(tmp_str);
	} else {
		(void) strcpy(termtype, tp);
	}

	{
		char	*a[3];

		a[0] = "/usr/bin/tput";
		a[1] = "init";
		a[2] = NULL;
		run(a);
	}

	fix_inittab(ypkill, termtype);

	/*
	 * "locale only" also includes determining the terminal type
	 */
	if (locale_only) {
		/*
		 * Write the state file and indicate that sysIDtool has
		 * not completed.
		 */

		put_state(FALSE, auto_hostname, networked, nis_done,
		    (netmask[0]) ? 1 : 0, FALSE, 1, termtype);

		/* Clean up the terminal interface and exit. */
		if (prompt_id != (Prompt_t)0)
			prompt_dismiss(prompt_id);

		done(SUCCESS, 1, ypkill, locale_only);

		/* NOTREACHED */
	}

	/*
	 * get the list of network interfaces
	 */
	num_ifs = get_net_if_list(&net_ifs);
	/* And construct the if_names list here */
	if_names = (char **)malloc(sizeof (char *) * num_ifs);
	for (i = 0, pifs = net_ifs; i < num_ifs; pifs = pifs->next, i++) {
		if_names[i] = (char *)malloc(strlen(pifs->name)+1);
		fprintf(debugfp, "Adding %s to if_names list\n", pifs->name);
		strcpy(if_names[i], pifs->name);
	}

	/*
	 * If we get here, the locale and term stuff is guaranteed to
	 * have been done
	 */

	configured = FALSE;

	while (!configured) {
		/*
		 * Prompt for any information that we could not determine
		 * from the network.  Keep re-prompting until the user
		 * positively confirms their input.
		 */

		sync();
		sync();
		confirmed = FALSE;
		while (!confirmed) {

			num_items = 0;

			/*
			 * Prompt for hostname.
			 */
			if (!auto_hostname) {
				prompt_hostname(hostname);
				list_data[num_items].field_attr = ATTR_HOSTNAME;
				list_data[num_items].value = hostname;
				list_data[num_items++].flags = 0L;
			}

			/*
			 * Prompt to see if system will be using network.
			 * Only applies to systems with a network I/F.
			 */
			if (num_ifs > 0 && !auto_all_network_info) {

				if (auto_network_interface || !(is_standalone =
				    prompt_isit_standalone(is_standalone))) {

					networked = TRUE;
					list_data[num_items].field_attr =
					    ATTR_NETWORKED;
					list_data[num_items].value = YES;
					list_data[num_items++].flags = 0L;

					/*
					 * if the system has only 1 network
					 * interface, just use it.  Otherwise,
					 * we need to prompt for the primary
					 * interface.  (eventually this
					 * could be made into a loop
					 * to configure each interface)
					 */
					if (!auto_network_interface) {
						if (num_ifs == 1)
							strcpy(if_name, net_ifs->name);
						else {
							if_choice = prompt_primary_net(
								if_names, num_ifs,
								if_choice);
							strcpy(if_name,
								if_names[if_choice]);
							list_data[num_items].field_attr
								= ATTR_PRIMARY_NET;
							list_data[num_items].value
								= if_name;
							list_data[num_items++].flags
								= 0L;
						}
					}

					/*
					 * Prompt for IP address.
					 */

					if (!auto_ip_addr) {
						prompt_hostIP(ip_addr);

						list_data[num_items].field_attr =
							ATTR_HOSTIP;
						list_data[num_items].value = ip_addr;
						list_data[num_items++].flags = 0L;
					}

				} else {

					networked = FALSE;
					if (!auto_network_interface) {
						list_data[num_items].field_attr =
							ATTR_NETWORKED;
						list_data[num_items].value = NO;
						list_data[num_items++].flags = 0L;
					}

				}
			}

			/*
			 * Confirm input if it was not all taken from the
			 * network.  Only confirm the information manually
			 * entered by the user.
			 */

			if (num_items > 0) {
				confirmed = prompt_confirm(list_data,
				    num_items);
			} else {
				confirmed = TRUE;
			}
		}

		/*
		 * Put up a status message telling the user we're working on it
		 */
		prompt_id = prompt_message((Prompt_t)0, GOODBYE_NETINIT);

		/*
		 * Write the state file and indicate that sysIDtool has
		 * not completed.
		*/

		put_state(FALSE, auto_hostname, networked, nis_done,
		    (netmask[0]) ? 1 : 0, FALSE, 1, termtype);

		/*
		 * Configure the network interface using the information
		 * gathered above.  If an error occurs in the configuration,
		 * re-prompt for the configuration information.
		 */

		if (networked) {
			if (!set_net_ipaddr(if_name, ip_addr)) {
				continue;
			}
			if (!set_net_ifup(if_name)) {
				continue;
			}
		} else {
			char	errmess[1024];

			setup_nsswitch(NO_NAMING_SERVICE, errmess);
		}

		if (!set_net_hostname(hostname, (networked ? ip_addr : NULL),
		    if_name, ypkill)) {
			continue;
		}

		configured = TRUE;
	}

	if (prompt_id != (Prompt_t)0)
		prompt_dismiss(prompt_id);

	/*
	 * Clean up the terminal interface and exit.
	 */
	done(SUCCESS, 1, ypkill, locale_only);

	/* NOTREACHED */
	return (SUCCESS);
}

static int
system_configured(int *nis_done, int *sys_localep, char *termtype,
	int locale_only)
{
	int sys_configured;
	int sys_bootparamed;
	int sys_networked;
	int sys_subnetted;
	int sys_passwd;
	int err_num;

	/*
	 * To see if this system is configured, check to see if the hostname
	 * is set both on the network interface and in the local /etc/hosts
	 * file, and that sysIDtool had previously completed.
	 *
	 * If a hostname has been configured on the system, but there is
	 * no state file, then the system must have been configured
	 * through some other mechanism (such as netinstall), and so
	 * sysIDtool should not attempt to configure it.
	 */

	if (nodename_set()) {
		if (get_state(&sys_configured, &sys_bootparamed,
			    &sys_networked, nis_done, &sys_subnetted,
			    &sys_passwd, sys_localep, termtype,
			    &err_num) == SUCCESS) {

			if (sys_configured) {
				return (TRUE);
			}
		} else {
			if (err_num == ENOENT) {
				put_state(TRUE, FALSE, FALSE, FALSE, FALSE,
					FALSE, FALSE, "");
				return (TRUE);
			}
		}
	} else {
		/*
		 * Even if the nodename is set, still get the termtype
		 * since we may have been invoked with the -l flag
		 * the first time
		 * ACKK, GROSS. If we are being invoked with the -l flag,
		 * we know that we must ignore the state file,
		 * since sys-unconfig doesn't delete it (WHY?)
		 */
		if (!locale_only)
			get_state(&sys_configured, &sys_bootparamed,
			    &sys_networked, nis_done, &sys_subnetted,
			    &sys_passwd, sys_localep, termtype, &err_num);
	}

	return (FALSE);

}

static void
fix_inittab(int rw, char *term)
{
	char cmd[1024];

	if (testing)
		return;

	if (!rw)
		return;

	sprintf(cmd, "nawk '/^co:/{\
		sub(\"[ \t]-T[ \t][ \t]*[a-zA-Z0-9][^ \t]*\", \" -T %s\");\
		print $0;\
		next}\
		{print $0}' /etc/inittab >/tmp/inittab.sys 2> /dev/null;\
		cp /tmp/inittab.sys /etc/inittab;\
		rm -f /tmp/inittab.sys;\
		init q", term);

	system(cmd);
}

void
usage()
{
	(void) fprintf(stderr, "system config error: bad usage.\n");
}

static void
done(int status, int call_close, int ypkill, int locale_only)
{
	if (ypkill)
		kill_ypbind();
	fprintf(debugfp, "netinit: done\n");
	/*
	 * Clean up the curses interface.
	 */
	if (call_close) {
		if (locale_only)
			/*
			 * On Intel, sysidnet gets called with the
			 * locale_only option before the window system
			 * is started.  So that subsequent invocations
			 * of sysid* are not forced to use the tty
			 * interface, we must shut down the UI here.
			 */
			(void) prompt_close("", 1);
		else
			(void) prompt_close("", 0);
	}
	fprintf(debugfp, "sysidnet end\nfree VM: %d\n\n", free_vm());
	sync();
	sync();
	exit(status);

	/* NOTREACHED */
}
