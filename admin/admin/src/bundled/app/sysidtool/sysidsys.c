
/*
 *  Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 *  All rights reserved.
 */

/*
 * sysIDtool_sysinit - system configuration tool.
 *
 * sysIDtool_sysinit is called directly from the system startup scripts.
 * Its default mode of behaviour is to check files in the /root filesystem
 * for the system's configuration parameters. If these parameters are not set,
 * syIDtool consults network services first, and then prompts the user for
 * the configuration information for this system.
 */

#pragma	ident	"@(#)sysidsys.c	1.66	96/10/07 SMI"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/fcntl.h>
#include <sys/systeminfo.h>
#include <time.h>
#include <sys/time.h>
#include "cl_database_parms.h"
#include "sysidtool.h"
#include "sysid_msgs.h"
#include "prompt.h"

#include "sysid_preconfig.h"

#define	BAD_USAGE	-1
#define	SEPARATOR	'.'
#define	UNCONFIG_FILE	"/etc/.UNCONFIGURED"

/*
 * Local variables
 */
static	char	errmess[1024];

/*
 * Local routines
 */
static	void	done(int status);
static	int	system_configured(int *, int *, int *, int *, int *, int *,
			char *);
static	void	remove_unconfig_file();
static	int	strip_netnum();

FILE *debugfp;

/*
 * Globals referenced
 */
extern	char	*strrchr();
extern	int	errno;
extern	int	settimeofday();

char 	*progname;

int
main(int argc, const char *argv[])
{
	int		sys_bootparamed, sys_networked, autobound;
	char	termtype[MAX_TERM+1];
	char	ns_type[11];
	char	domainname[MAX_DOMAINNAME+2];
	char	hostname[MAX_HOSTNAME+2];
	int		auto_netmask;
	int		subnet_item;
	int		subnetted = 0;
	int		syspasswd;
	int		syslocale;
	char	netmask[MAX_NETMASK+3];
	char	*netmask_ptr;
	char	netnum[MAX_NETMASK+2], if_name[17];
	int		auto_timezone;
	int		region_item;
	int		tz_item;
	char	timezone[MAX_TZ+2];
	char	*tz_ptr = NULL;
	int		auto_date;
	int		set_time = 1;
	char	confirm_date[64];
	char	date[13];
	int		num_items;
	Confirm_data	list_data[5];
	int		configured;
	int		confirmed;
	int		tmp_int;
	char	tmpstr[MAX_TERM+11];
	time_t	clock_val;
	struct tm	tm;
	struct timeval	tv;
	Prompt_t	prompt_id;
	extern char	*optarg;
	char		*test_input = (char *)0, *test_output = (char *)0;
	int		c;
	int		noauto;
	int		must_do_netmask = 0;
	int		subnet_done;
	int		net_stat = 0;

	subnet_item = 0;
	auto_netmask = FALSE;
	(void) strcpy(netmask, DEFAULT_NETMASK);
	region_item = 0;
	tz_item = 0;
	auto_timezone = FALSE;
	timezone[0] = NULL;
	auto_date = FALSE;
	date[0] = NULL;

	prompt_id = (Prompt_t)0;

	/* need to setlocale even though lookup_locale() returns later. */
	(void) setlocale(LC_ALL, "");

	/*
	 * First check to see if this system is configured. For now this is a
	 * black and white decision; if the hostname is set, then we exit
	 * sysIDtool. In future versions, we will check each parameter so
	 * that you could have a partially configured system.
	 */
	debugfp = open_log("sysidsys");

	progname = (char *)argv[0];

	while ((c = getopt(argc, (char **) argv, "O:i:I:")) != EOF) {
		switch (c) {
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
	    "%s: -O <name> must be accompanied by -I <name> and -i <name>",
			progname);
		exit(1);
	}

	if (testing) {
		if (sim_init(test_input, test_output) < 0) {
			fprintf(stderr,
			    "%s: Unable to initialize simulator\n",
			    progname);
			exit(1);
		}
	}

	if (system_configured(&sys_bootparamed, &sys_networked,
	    &autobound, &subnet_done, &syspasswd, &syslocale,
	    termtype) == TRUE) {
		remove_unconfig_file();
		done(SUCCESS);
	}

	fprintf(debugfp, "sysinit: system is not configured\n");

	/*
	 * This system is not configured. Proceed with the configuration.
	 */

	noauto = bootargset("noauto");

	/*
	 * Set the terminal type.
	 */

	(void) sprintf(tmpstr, "TERM=%s", termtype);
	(void) putenv(tmpstr);

	(void) lookup_locale();

	/* Start the user interface */
	if (prompt_open(&argc, (char **)argv) < 0) {
		(void) fprintf(stderr, "%s: Couldn't start user interface\n",
		    argv[0]);
		exit(1);
	}

	/*
	 * Determine the system's host name.
	 */

	get_net_hostname(hostname);

	ns_type[0] = NULL;
	domainname[0] = NULL;


	/* 
	 * Attempt to read in the sysidtool configuration
	 * file and determine if there are any configuration
	 * variables that are pertinent to this application
	 *
	 */
	if (!noauto && read_config_file() == SUCCESS) {

		fprintf(debugfp,"sysidsys:  Using configuration file\n");

		/* Get the timezone information if it has been specified */
		tz_ptr = get_preconfig_value(CFG_TIMEZONE,NULL,NULL);
		if (tz_ptr != NULL) {
			auto_timezone = TRUE;
			strcpy(timezone,tz_ptr);
		}

		/* Get the netmask information from the default interface */
		/* Default means primary.  It is currently possible to    */
		/* configure only the primary.  The ability to configure  */
		/* all of the network interaces is planned                */
		netmask_ptr = get_preconfig_value(CFG_NETWORK_INTERFACE,
								CFG_DEFAULT_VALUE,CFG_NETMASK);
		if (netmask_ptr != NULL) {
			auto_netmask = TRUE;
			strcpy(netmask,netmask_ptr);
		}

		if (auto_timezone)
			fprintf(debugfp,"  timezone: %s\n",timezone);
		if (auto_netmask)
			fprintf(debugfp,"  netmask: %s\n",netmask);
	}

	if (sys_networked == TRUE) {

		/*
		 * Determine if a name service is in use.
		 */

		get_net_domainname(domainname);
		system_namesrv(domainname, ns_type);

		/*
		 * Next try to automatically determine as much configuration
		 * information as possible.  Get the system's IP address from
		 * its network I/F.  If a name service is in use and the netmask
		 * has not already been configured (it may have been configured
		 * in order to contact a name server that potentially resides on
		 * a non-local subnet), try to get the system's netmask (based
		 * on its network number) and timezone from the name service.
		 * If a name service is in use, also try to get the time from
		 * the server.
		 */

		/*
		 * Get network number here from the ip addr.
		 */
		get_net_name_num(if_name, netnum);

		must_do_netmask = !subnet_done;

		if (!noauto && autobound) {
			if (must_do_netmask && !auto_netmask) {

				/*
				 * Get netmask from the NIS database.
				 */
				if ((net_stat = get_entry(conv_ns(ns_type),
				    NETMASKS_TBL, netnum, netmask)) == SUCCESS)
					auto_netmask = TRUE;
				else {
					char tmpnum[MAX_NETMASK+2];

					(void) memcpy(tmpnum, netnum,
						(size_t) MAX_NETMASK+1);
					if (strip_netnum(tmpnum) == TRUE)
						if ((net_stat =
						    get_entry(conv_ns(ns_type),
						    NETMASKS_TBL, tmpnum,
						    netmask)) == SUCCESS)
							auto_netmask = TRUE;
				}
			}

			/*
			 * Try to get the system's timezone based on
			 * it's hostname or domainname.
			 */
			if (!noauto && !auto_timezone) {
				if ((net_stat = get_entry(conv_ns(ns_type),
					TIMEZONE_TBL, hostname, timezone)) == SUCCESS)
					auto_timezone = TRUE;
				else if ((net_stat = get_entry(conv_ns(ns_type),
					TIMEZONE_TBL, domainname, timezone)) == SUCCESS)
					auto_timezone = TRUE;
			}
		}

	}

	/*
	 * Ask the user for any configuration information that could
	 * not be determined from the network, and then configure the
	 * system.  If an error occurs during the configuration,
	 * re-prompt the user for the information.
	 */

	configured = FALSE;
	while (!configured) {

		/*
		 * Prompt for any information that could not be determined
		 * from the network.  Keep re-prompting until the user
		 * positively confirms their input.
		 */

		sync();
		sync();
		confirmed = FALSE;
		while (!confirmed) {

			num_items = 0;

			/*
			 * Prompt for the netmask.
			 */

			if (must_do_netmask && !auto_netmask) {
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
			 * Set the netmask on the network interface.  This is
			 * done here so that we can contact the tiemhost if
			 * it is not on the local subnet.
			 */

			if (must_do_netmask && (auto_netmask || subnetted)) {
				if (set_net_netmask(if_name, netmask,
				    errmess) != SUCCESS) {
					prompt_error(SYSID_ERR_BAD_NETMASK,
					    netmask, if_name, errmess);
					continue;
				}
			}

			/*
			 * Prompt for timezone.
			 */

			if (!auto_timezone) {
				prompt_timezone(timezone, &region_item,
				    &tz_item);
				list_data[num_items].field_attr = ATTR_TIMEZONE;
				list_data[num_items].value = timezone;
				list_data[num_items++].flags = 0L;
			}

			/*
			 * Set the timezone in /etc/TIMEZONE.
			 */

			if (set_env_timezone(timezone, errmess) != SUCCESS) {
				prompt_error(SYSID_ERR_BAD_TIMEZONE, timezone,
					errmess);
				auto_timezone = FALSE;
				continue;
			}

			/*
			 * Trying using rdate to determine the date and time.
			 * If unsuccessful, prompt for the date and time.
			 */

			if (! noauto && sys_networked &&
			    strcmp(ns_type, DB_VAL_NS_UFS)) {
				/*
				 * It may take a while for this to time out,
				 * so tell the user we're thinking.
				 */
				prompt_id = prompt_message((Prompt_t)0,
				    PLEASE_WAIT_S);

				if (net_stat != NS_TRYAGAIN &&
				    set_rdate() == SUCCESS) {
					auto_date = TRUE;
				} else {
					auto_date = FALSE;
				}

				prompt_dismiss(prompt_id);
				prompt_id = (Prompt_t)0;
			}

			if (!auto_date) {
				struct tm	*tp;
				char	year[MAX_YEAR+2],
					orig_y[MAX_YEAR+2],
					month[MAX_MONTH+2],
					orig_m[MAX_MONTH+2],
					day[MAX_DAY+2],
					orig_d[MAX_DAY+2],
					hour[MAX_HOUR+2],
					orig_h[MAX_HOUR+2],
					minute[MAX_MINUTE+2],
					orig_min[MAX_MINUTE+2];

				(void) time(&clock_val);
				tp = localtime(&clock_val);
				tmp_int = tp->tm_mon + 1;

				/*
				 *    tp->tm_year is the number of years past
				 *    1900.
				 */
				(void) sprintf(year, "%04d", tp->tm_year+1900);
				(void) sprintf(month, "%02d", tmp_int);
				(void) sprintf(day, "%02d", tp->tm_mday);
				(void) sprintf(hour, "%02d", tp->tm_hour);
				(void) sprintf(minute, "%02d", tp->tm_min);

				strcpy(orig_y, year);
				strcpy(orig_m, month);
				strcpy(orig_d, day);
				strcpy(orig_h, hour);
				strcpy(orig_min, minute);

				log_time();
				prompt_date(year, month, day, hour, minute);

				if (strcmp(orig_y, year) == 0 &&
				    strcmp(orig_m, month) == 0 &&
				    strcmp(orig_d, day) == 0 &&
				    strcmp(orig_h, hour) == 0 &&
				    strcmp(orig_min, minute) == 0)
					set_time = 0;
				else
					set_time = 1;

				tm.tm_year = (atoi(year) - 1900);
				tm.tm_mon = (atoi(month)-1);
				tm.tm_mday = atoi(day);
				tm.tm_hour = atoi(hour);
				tm.tm_min = atoi(minute);
				tm.tm_sec = 0;
				(void) strftime(confirm_date,
				    sizeof (confirm_date), "%x %X", &tm);
				list_data[num_items].field_attr =
					    ATTR_DATE_AND_TIME;
				list_data[num_items].value = confirm_date;
				list_data[num_items].flags = 0L;

				/* don't confirm if this is the only data */
				if (set_time || num_items)
					num_items++;
			}

			/*
			 * Confirm any input entered by the user.
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
		prompt_id = prompt_message((Prompt_t)0, PLEASE_WAIT_S);

		/*
		 * Configure the system using the information gathered above.
		 * If an error occurs, re-prompt for the configuration
		 * information.
		 */

		/*
		 * Set the netmask in the /etc/netmasks file, unless
		 * the netmask has already previously been set.  The
		 * netmask has already been set on the network interface
		 * above.
		 */

		if (must_do_netmask && (auto_netmask || subnetted)) {
			if (set_ent_netmask(netnum, netmask, errmess)
			    != SUCCESS) {
				prompt_error(SYSID_ERR_STORE_NETMASK, netmask,
				    netnum, errmess);
				continue;
			}
		}

		/*
		 * Set the date and time on the system.
		 */

		if (!auto_date && set_time) {
			time_t	new_clock;
			long	delta;

			(void) time(&new_clock);
			delta = new_clock - clock_val;

			tm.tm_isdst = -1;

			/* let mktime do the calculation */
			tv.tv_usec = 0;
			tv.tv_sec = (long) mktime(&tm);

			/*
			 * Add the time difference from when we put up the
			 * prompt, in case they took a long time in the
			 * confirmation process.
			 */
			tv.tv_sec += delta;

			(void) strftime(confirm_date, sizeof (confirm_date),
			    "%x %X", &tm);
			fprintf(debugfp, "settimeofday date %s, delta %ld\n",
				confirm_date, delta);

			if (tv.tv_sec == (long) -1) {
				prompt_error(SYSID_ERR_BAD_DATE, confirm_date,
					"Unable to convert given date");
				continue;
			}
			if (testing)
				(*sim_handle())(SIM_SET_DATE_MTHD, &tv);
			else {
				void *p = NULL;

				if (settimeofday(&tv, p) < 0) {
					prompt_error(SYSID_ERR_BAD_DATE,
						confirm_date,
						strerror(errno));
					continue;
				}
			}
		}
		sync();
		sync();

		/*
		 * Remove /etc/.UNCONFIGURED and mark sysIDtool as
		 * completed in the state file.
		 */

		remove_unconfig_file();
		put_state(TRUE, sys_bootparamed,
			(net_stat != NS_TRYAGAIN) ? sys_networked : 0,
			autobound, TRUE, FALSE, syslocale,
			termtype);

		configured = TRUE;
	}

	if (prompt_id != (Prompt_t)0)
		prompt_dismiss(prompt_id);

	/*
	 * Clean up the terminal interface and exit.
	 */
	done(SUCCESS);

	/*
	 * Never reached.
	 */
	return (SUCCESS);
}

static int
system_configured(int *sys_bootparamed, int *sys_networked, int *sys_autobound,
    int *sys_subnetted, int *sys_passwd, int *sys_locale, char *term_type)
{
	int sys_configured;
	int err_num;

	/*
	 * To see if this system is configured, check the state kept by
	 * the sysIDtool_netinit program. This checks to see if the hostname
	 * was set, both on the network interface, and in the local /etc/hosts
	 * file when the system was booted.
	 */
	(void) get_state(&sys_configured, sys_bootparamed, sys_networked,
		sys_autobound, sys_subnetted, sys_passwd, sys_locale,
		term_type, &err_num);

	return (sys_configured);
}

/*
 * Search the netnum for trailing zeros, and strip if found.
 */
int
strip_netnum(netnum)
char *netnum;
{
	char *ptr;
	int flag = FALSE;

	while ((ptr = strrchr(netnum, SEPARATOR)) != NULL) {

		if (strcmp(ptr+1, "0") == 0 || strcmp(ptr+1, "00") == 0 ||
			strcmp(ptr+1, "000") == 0) {

			*ptr = '\0';
			flag = TRUE;
		} else
			break;
	}
	return (flag);
}


static void
remove_unconfig_file()
{
	/*
	 * We don't check the error here because this will fail on the CD, but
	 * we can't easily distinguish that situation so don't worry about it.
	 */
	(void) unlink(UNCONFIG_FILE);
}

void
usage()
{
	(void) fprintf(stderr, "system config error: bad usage.\n");
}

static void
done(int status)
{
	log_time();
	fprintf(debugfp, "sysinit: done\n");

	/*
	 * For sysidsys, always call prompt close to make sure
	 * that the UI process shuts down cleanly
	 */
	(void) prompt_close(GOODBYE_SYSINIT, 1);
	fprintf(debugfp, "sysidsys end\nfree VM: %d\n\n", free_vm());
	sync();
	sync();
	exit(status);
	/* NOTREACHED */
}
