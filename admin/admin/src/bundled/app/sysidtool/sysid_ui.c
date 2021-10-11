/*
 * Copyright (c) 1993-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)sysid_ui.c 1.24 96/02/05"

/*
 * sysid_ui - network interface configuration tool simulator
 *	(user interface only).
 *
 * sysid_ui is a test program that only cycles thru the user interface
 * screens.  Useful for testing/debugging the user interface without
 * changing the configuration of the system.
 */

/*
 * USED ONLY FOR RUNNING THE USER INTERFACE CODE EXCLUSIVELY.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "sysidtool.h"
#include "sysid_msgs.h"
#include "prompt.h"
#include "message.h"

#define	SLEEP_INTERVALS	5	/* 5 in sys_method_wrappers.c */
#define	INTERVAL	2	/* 5 in sys_method_wrappers.c */

FILE *debugfp;

main(int argc, char **argv)
{
	char	host[MAX_HOSTNAME+3];
	char	ip[MAX_IPADDR+3];
	char	nis[10];
	char	role[10];
	char	servername[MAX_HOSTNAME+3];
	char	server_addr[MAX_IPADDR+3];
	char	netmask[MAX_NETMASK+3];
	char	year[MAX_YEAR+3];
	char	month[MAX_MONTH+3];
	char	day[MAX_DAY+3];
	char	hour[MAX_HOUR+3];
	char	minute[MAX_MINUTE+3];
	char	timezone[MAX_TZ+3];
	char	domain[MAX_DOMAINNAME+3];
	char	termtype[MAX_TERM];
	char	locale[MAX_LOCALE];
	int	lang_item;
	int	locale_item;
	char	passwd[MAX_PASSWORD+3];
	char	e_passwd[256];
	char	confirm_date[64];
	int	tmp_int;
	time_t	clock_val;
	struct tm *tp;
	int	subnet_item;
	int	net_item;
	int	ns_item;
	int	region_item;
	int	tz_item;
	int	ifc_item;
	int	bcast_item;
	Confirm_data list_data[9];
	char	*locales[5];
	char	*ns[4];
	char	*ifc[2];
	char	*cp;
	int	i, c;
	int	confirmed;
	char	buf[256];
	int	stage[7];
	int	badarg;
	int	doerrs;

	/*
	 * Need to do setlocale() even lookuplocale() returns 0 later.
	 * This setlocale() may be overwritten later.
	 */
	(void) setlocale(LC_ALL, "");
	(void) textdomain(TEXT_DOMAIN);

	(void) strcpy(netmask, "255.255.255.0");

	servername[0] = server_addr[0] = host[0] = ip[0] = timezone[0] =
			termtype[0] = nis[0] = role[0] = domain[0] = NULL;

	bcast_item = net_item = subnet_item = ns_item = ifc_item = 0;

	stage[0] = 1;
	for (i = 1; i < 6; i++)
		stage[i] = 0;

	region_item = -1;
	tz_item = -1;

	lang_item = -1;
	locale_item = -1;

	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);

	badarg = 0;
	doerrs = 0;
#ifdef DEV
	while ((c = getopt(argc, argv, "e123456f:u:")) != EOF) {
#else
	while ((c = getopt(argc, argv, "e123456u:")) != EOF) {
#endif
		switch (c) {
		case '1':	/* sysidloc */
		case '2':	/* sysidnet */
		case '3':	/* sysidnis */
		case '4':	/* sysidsys */
		case '5':	/* sysidroot */
		case '6':	/* test of prompt_close before prompt_open */
			buf[0] = (char)c;
			buf[1] = '\0';
			i = atoi(buf);
			stage[i] = 1;
			stage[0] = 0;
			break;
		case 'e':	/* errors */
			doerrs++;
			break;
#ifdef DEV
		case 'f':	/* pass through to prompt_open() */
			break;
#endif
		case 'u':	/* pass through to init_display() */
			break;
		default:
			badarg++;
			break;
		}
	}
	if (badarg) {
		(void) fprintf(stderr,
			"usage: %s [ -1 | -2 | -3 | -4 ] [ -e ]\n", argv[0]);
		exit(1);
	}

	debugfp = fopen("/dev/null", "a");
	if (debugfp == NULL) {
		fprintf(stderr, " Unable to open debugfile %s\n", DEBUGFILE);
		exit(1);
	}

	/*
	 * Stage 1 - sysidloc (locale, term-type iniitialization)
	 */
	if (stage[0] || stage[1]) {
		if (prompt_open(&argc, argv) < 0) {
			(void) fprintf(stderr, "can't start user interface\n");
			exit(1);
		}

		cp = getenv("LC_MESSAGES");
		if (cp == (char *)0) {
			i = 0;
			locales[i++] = "de";
			locales[i++] = "fr";
			locales[i++] = "it";
			locales[i++] = "sv";
			locales[i++] = "ja";

			do_locale_simulate(i, locales);
			/* prompt_locale(locale); */
			prompt_locale(locale, &lang_item, &locale_item);

#ifdef DEV
			(void) setlocale(LC_MESSAGES, locale);
#else
			(void) setlocale(LC_ALL, locale);
#endif
		} else
#ifdef DEV
			(void) setlocale(LC_MESSAGES, cp);
#else
			(void) setlocale(LC_ALL, cp);
#endif

		cp = getenv("TERM");
		if (cp == (char *)0)
			prompt_terminal(termtype);
		else
			(void) strcpy(termtype, cp);

		if (stage[1])
			(void) prompt_close((char *)0, 0);
	}

	/*
	 * Stage 2 - sysidnet.
	 */
	if (stage[0] || stage[2]) {
		if (stage[2] && prompt_open(&argc, argv) < 0) {
			(void) fprintf(stderr, "can't start user interface\n");
			exit(1);
		}

		i = 0;
		ifc[i++] = "le0";
		ifc[i++] = "le1";

		confirmed = 1;
		do {
			Confirm_data	*cd;
			int		n_items;

			cd = list_data;

			prompt_hostname(host);

			cd->field_attr = ATTR_HOSTNAME;
			cd->flags = 0L;
			cd->value = host;
			cd++;

			net_item = prompt_isit_standalone(net_item);

			cd->field_attr = ATTR_NETWORKED;
			cd->flags = 0L;
			cd->value = net_item ? NO : YES;
			cd++;

			if (net_item == 0) {
				ifc_item = prompt_primary_net(ifc, 2, ifc_item);

				cd->field_attr = ATTR_PRIMARY_NET;
				cd->flags = 0L;
				cd->value = ifc[ifc_item];
				cd++;

				prompt_hostIP(ip);

				cd->field_attr = ATTR_HOSTIP;
				cd->flags = 0L;
				cd->value = ip;
				cd++;
			}

			n_items = cd - list_data;

			if (n_items > 0)
				confirmed = prompt_confirm(list_data, n_items);
		} while (!confirmed);

		prompt_message((Prompt_t)0, GOODBYE_NETINIT);
		sleep(2);

		if (stage[2]) {
			(void) prompt_close("", 0);
			(void) sleep(2);
		}
	} else
		net_item = 0;

	/*
	 * Stage 3 - sysidnis
	 */
	if (stage[0] || stage[3]) {
		if (stage[3] && prompt_open(&argc, argv) < 0) {
			(void) fprintf(stderr, "can't start user interface\n");
			exit(1);
		}

		i = 0;
		ns[i++] = NIS_VERSION_3;
		ns[i++] = NIS_VERSION_2;
		ns[i++] = OTHER_NAMING_SERVICE;
		ns[i++] = NO_NAMING_SERVICE;

		confirmed = 1;
		do {
			Confirm_data	*cd;
			int		n_items;

			cd = list_data;

			if (net_item == 0) {
				ns_item = prompt_name_service(ns, 4, ns_item);

				cd->field_attr = ATTR_NAME_SERVICE;
				cd->flags = 0L;
				cd->value = ns[ns_item];

				/*
				 * XXX [shumway]
				 *
				 * This is really horrible.  Items 2 and 3
				 * are "Other" and "None" respectively.
				 * Programmatically they are the same but
				 * they need to be distinct choices from the
				 * user's point of view.  To make them behave
				 * the same, we "translate" choice 2 into
				 * choice 3.  We leave ns_item alone so that
				 * the user sees their original choice on
				 * subsequent cycles through the program.
				 */
				if (ns_item == 2)
					(void) strcpy(nis, ns[3]);
				else
					(void) strcpy(nis, ns[ns_item]);

				cd++;
			}

			if (net_item == 0 && strcmp(nis, NO_NAMING_SERVICE)) {
				prompt_domain(domain);

				cd->field_attr = ATTR_DOMAIN;
				cd->flags = 0L;
				cd->value = domain;
				cd++;

				bcast_item = prompt_broadcast(bcast_item);

				cd->field_attr = ATTR_BROADCAST;
				cd->flags = 0L;
				cd->value = bcast_item ?
						SPECNAME_S : BROADCAST_S;
				cd++;

				if (bcast_item == 1) {
					prompt_nisservers(
						servername, server_addr);

					cd->field_attr = ATTR_NISSERVERNAME;
					cd->flags = 0L;
					cd->value = servername;
					cd++;

					cd->field_attr = ATTR_NISSERVERADDR;
					cd->flags = 0L;
					cd->value = server_addr;
					cd++;
				}
			}

			if (net_item == 0) {
				subnet_item = prompt_isit_subnet(subnet_item);

				cd->field_attr = ATTR_SUBNETTED;
				cd->flags = 0L;
				cd->value = subnet_item ? YES : NO;
				cd++;
			}

			if (net_item == 0 && subnet_item) {
				prompt_netmask(netmask);

				cd->field_attr = ATTR_NETMASK;
				cd->flags = 0L;
				cd->value = netmask;
				cd++;
			}

			n_items = cd - list_data;

			if (n_items > 0)
				confirmed = prompt_confirm(list_data, n_items);
		} while (!confirmed);

		if (subnet_item) {
			Prompt_t prompt_id;

			prompt_id = 0;
			for (i = SLEEP_INTERVALS; i > 0; i--) {
				(void) sprintf(buf, PLEASE_WAIT,
					(i * INTERVAL));
				prompt_id = prompt_message(prompt_id, buf);
				(void) sleep(INTERVAL);
			}
			prompt_dismiss(prompt_id);
		} else {
			prompt_message((Prompt_t)0, GOODBYE_NISINIT);
			sleep(2);
		}

		if (stage[3]) {
			(void) prompt_close("", 0);
			(void) sleep(2);
		}
	}

	/*
	 * Stage 4 - sysidsys
	 */
	if (stage[0] || stage[4]) {
		if (stage[4] && prompt_open(&argc, argv) < 0) {
			(void) fprintf(stderr, "can't start user interface\n");
			exit(1);
		}

		confirmed = 1;
		do {
			Confirm_data	*cd;
			int		n_items;

			cd = list_data;

			prompt_timezone(timezone, &region_item, &tz_item);

			prompt_message((Prompt_t)0, PLEASE_WAIT_S);
			sleep(5);

			cd->field_attr = ATTR_TIMEZONE;
			cd->flags = 0L;
			cd->value = timezone;
			cd++;

			(void) time(&clock_val);
			tp = localtime(&clock_val);
			tmp_int = tp->tm_mon + 1;
			(void) sprintf(year, "19%02d", tp->tm_year);
			(void) sprintf(month, "%02d", tmp_int);
			(void) sprintf(day, "%02d", tp->tm_mday);
			(void) sprintf(hour, "%02d", tp->tm_hour);
			(void) sprintf(minute, "%02d", tp->tm_min);

			prompt_date(year, month, day, hour, minute);

			tp->tm_year = atoi(year+2);
			tp->tm_mon = atoi(month)-1;
			tp->tm_mday = atoi(day);
			tp->tm_hour = atoi(hour);
			tp->tm_min = atoi(minute);

			(void) strftime(
				confirm_date, sizeof (confirm_date),
				"%x %R", tp);

			cd->field_attr = ATTR_DATE_AND_TIME;
			cd->flags = 0L;
			cd->value = confirm_date;
			cd++;

			n_items = cd - list_data;

			if (n_items > 0)
				confirmed = prompt_confirm(list_data, n_items);
		} while (!confirmed);

		prompt_message((Prompt_t)0, PLEASE_WAIT_S);
		sleep(2);

		if (stage[4]) {
			(void) prompt_close(GOODBYE_SYSINIT, 1);
			(void) sleep(2);
		}
	}

	/*
	 * Stage 5 - sysidroot
	 */
	if (stage[0] || stage[5]) {
		(void) putenv("DISPLAY=");	/* force tty operation */

		if (stage[5] && prompt_open(&argc, argv) < 0) {
			(void) fprintf(stderr, "can't start user interface\n");
			exit(1);
		}

		prompt_password(passwd, e_passwd);

		(void) prompt_close((char *)0, 1);
	}

	/*
	 * Stage 6 - test of prompt_close before prompt_open
	 */
	if (stage[6])
		(void) prompt_close(GOODBYE_SYSINIT, 1);

	if (doerrs) {

#define	NSSWITCH_CONF		"/etc/nsswitch.conf"
#define	NSSWITCH_FILES		"/etc/nsswitch.files"
#define	LOCAL_HOST		"localhost"

		char	*if_name = "<ifc>";
		char	*netnum = "<netnum>";
		char	*ether_addr = "<ether>";
		char	*errmess = "<error_message>";
		char	*ifup = "<ifc_up>";
		char	*tzname = "<tz_name>";

		(void) strcpy(host, "<host>");
		(void) strcpy(ip, "<ip_addr>");
		(void) strcpy(servername, "<server_name>");
		(void) strcpy(server_addr, "<server_addr>");
		(void) strcpy(netmask, "<netmask>");
		(void) strcpy(domain, "<domain>");
		(void) strcpy(timezone, "<time_zone>");
		(void) strcpy(confirm_date, "<date>");

		(void) prompt_open(&argc, argv);

		(void) prompt_bad_nis(SYSID_ERR_BAD_YP_BINDINGS2,
				domain, errmess);

		(void) prompt_bad_nis(SYSID_ERR_BAD_NIS_SERVER1,
				domain, errmess, servername);

		(void) prompt_bad_nis(SYSID_ERR_BAD_NIS_SERVER2,
				servername, domain, errmess, servername);

		(void) prompt_error(SYSID_ERR_NO_IPADDR, if_name);
		(void) prompt_error(SYSID_ERR_STORE_NETMASK,
				netmask, netnum, errmess);
		(void) prompt_error(SYSID_ERR_BAD_NETMASK,
				netmask, if_name, errmess);
		(void) prompt_error(SYSID_ERR_BAD_DOMAIN, domain, errmess);
		(void) prompt_error(SYSID_ERR_NSSWITCH_FAIL1,
				errmess, NSSWITCH_FILES, NSSWITCH_CONF);
		(void) prompt_error(SYSID_ERR_NSSWITCH_FAIL2, errmess);
		(void) prompt_error(SYSID_ERR_BAD_NISSERVER_ENT,
				servername, server_addr, errmess);
		(void) prompt_error(SYSID_ERR_BAD_YP_ALIASES, domain, errmess);
		(void) prompt_error(SYSID_ERR_BAD_YP_BINDINGS1,
				domain, errmess);
		(void) prompt_error(SYSID_ERR_CANT_DO_PASSWORD_PLUS, errmess);
		(void) prompt_error(SYSID_ERR_CANT_DO_PASSWORD, errmess);
		(void) prompt_error(SYSID_ERR_NO_IPADDR, if_name);
		(void) prompt_error(SYSID_ERR_NO_NETMASK, netnum);
		(void) prompt_error(SYSID_ERR_BAD_NETMASK,
				netmask, if_name, errmess);
		(void) prompt_error(SYSID_ERR_STORE_NETMASK,
				netmask, netnum, errmess);
		(void) prompt_error(SYSID_ERR_BAD_TIMEZONE,
				timezone, errmess);
		(void) prompt_error(SYSID_ERR_BAD_DATE,
				confirm_date, errmess);
		(void) prompt_error(SYSID_ERR_GET_ETHER,
				if_name, errmess);
		(void) prompt_error(SYSID_ERR_BAD_ETHER,
				ether_addr, host, errmess);
		(void) prompt_error(SYSID_ERR_BAD_HOSTS_ENT, host, errmess);
		(void) prompt_error(SYSID_ERR_BAD_BOOTP, host, errmess);
		(void) prompt_error(SYSID_ERR_BAD_NETMASK_ENT,
				netmask, errmess);
		(void) prompt_error(SYSID_ERR_BAD_TIMEZONE_ENT,
				timezone, errmess);
		(void) prompt_error(SYSID_ERR_BAD_NODENAME, host, errmess);
		(void) prompt_error(SYSID_ERR_BAD_LOOPBACK, host, errmess);
		(void) prompt_error(SYSID_ERR_BAD_HOSTS_ENT,
				LOCAL_HOST, errmess);
		(void) prompt_error(SYSID_ERR_BAD_HOSTS_ENT, host, errmess);
		(void) prompt_error(SYSID_ERR_BAD_IP_ADDR, ip, if_name);
		(void) prompt_error(SYSID_ERR_BAD_IP_ADDR, ip, if_name);
		(void) prompt_error(SYSID_ERR_BAD_UP_FLAG,
				ifup, if_name, errmess);
		(void) prompt_error(SYSID_ERR_BAD_TZ_FILE_NAME, tzname);

		(void) prompt_close("Error messages completed", 1);
	}

	exit(0);

	/* NOTREACHED */
#ifdef lint
	return (0);
#endif
}
