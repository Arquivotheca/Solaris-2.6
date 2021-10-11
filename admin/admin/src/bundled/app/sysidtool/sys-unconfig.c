/*
 * Copyright (c) 1991-1996 by Sun Microsystems, Inc.  All rights reserved.
 */

#pragma	ident	"@(#)sys-unconfig.c 1.14 96/06/14"

/*
 * sys-unconfig - utility to reverse the configuration process performed by
 *	sysIDtool during boot time and shut the system down.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mnttab.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <locale.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <sys/systeminfo.h>
#include "admutil.h"
#include "findconf.h"
#include "sysidtool.h"
#include "sysid_preconfig.h"

#define	ROOT_UID	0
#define	LOOPBACK	"lo0"

extern int  get_net_if_names(struct ifconf *);

struct mnttab mpref = {NULL, "/usr", NULL, NULL};

FILE *debugfp;
int  keep_preconfig_file = 0;

/*ARGSUSED*/
main(int argc, const char **argv)
{
	char confirm[80];
	FILE *fp;
	struct mnttab mp;
	int	status;
	int	c;
	char	*progname;

	init_cfg_err_msgs();

	(void) setlocale(LC_ALL, "");
	(void) textdomain("SUNW_INSTALL_SYSUN");

	/*
	 * Make sure that only root executes this.
	 */
	if (geteuid() != ROOT_UID) {
		fprintf(stderr,
		gettext("Only root is allowed to execute this program.\n"));
		exit(1);
	}

	if ((debugfp = fopen("/var/sadm/system/logs/sysidtool.log", "a"))
	    == NULL)
		debugfp = stderr;

	progname = (char *)argv[0];

	while ((c = getopt(argc, (char **) argv, "d")) != EOF) {
		switch (c) {
		case 'd':
			keep_preconfig_file = 1;
			break;
		default:
			/* Ignore unknown options */
			break;
		}
	}
	/*
	 * Make sure that we're not on a diskless or dataless client.  While
	 * this program will work, the system won't be able to boot after this
	 * because it won't have the hosts entry for its /usr server.
	 */
	if ((fp = fopen(MNTTAB, "r")) == NULL) {
		perror("fopen");
		exit(1);
	}
	if (getmntany(fp, &mp, &mpref) == 0) {
		(void) fclose(fp);
		if (strcmp(mp.mnt_fstype, "ufs")) {
			fprintf(stderr,
gettext("This program may not be run on diskless or dataless clients.\n"));
			exit(1);
		}
	} else {
			(void) fclose(fp);
	}

	printf(gettext("\t\t\tWARNING\n\n\
This program will unconfigure your system.  It will cause it\n\
to revert to a \"blank\" system - it will not have a name or know\n\
about other systems or networks.\n\n\
This program will also halt the system.\n\n\
Do you want to continue (y/n) ? "));
	if ((gets(confirm) == NULL) ||
	    (strcmp(confirm, "y") && strcmp(confirm, "Y") &&
	    strcmp(confirm, "yes") && strcmp(confirm, "YES"))) {
		printf(gettext("Aborted\n"));
		exit(0);
	}

	/*
	 * Unconfigure the drivers that have application-level unconfig
	 * chores (like unconfig the keyboard and display and mouse in x86)
	 */
	execAllCfgApps(progname, DO_UNCONFIG);

	if ((status = unconfig_files()) != 0)
		fprintf(stderr, gettext("Error in unconfig_files: %d\n"),
			status);

	sync();	/* Ensure that the changes made above get written */

	halt();

	pause();	/* We want to hang around until the system stops */
	/*NOTREACHED*/
	return (SUCCESS);
}

#define	NIS_COLD_START		"/var/nis/NIS_COLD_START"
#define	NETMASKS_FILE		"/etc/inet/netmasks"
#define	HOSTS_FILE		"/etc/inet/hosts"
#define	HOSTS_FILE_OLD		"/etc/inet/hosts.saved"
#define	FACTORY_TIMEZONE	"PST8PDT"
#define	YPSERV_FILE		"ypservers"
#define	YPCACHE_FILE		"cache_binding"
#define	YP_BINDING_DIR		"/var/yp/binding"
#define	NODENAME_FILE		"/etc/nodename"
#define	DEFAULTDOMAIN_FILE	"/etc/defaultdomain"
#define	DEFAULTROUTER_FILE	"/etc/defaultrouter"
#define	IF_NAME_FMT		"/etc/hostname.%s"
#define	ROOT_NAME		"root"
#define	UNCONFIGURED_FILE	"/etc/.UNCONFIGURED"
#define	RECONFIGURE_FILE	"/reconfigure"

unconfig_files()
{

	char yp_serv_file[MAXPATHLEN];
	char t_hostname[MAXPATHLEN];
	char curr_nodename[SYS_NMLN];
	char curr_domain[SYS_NMLN];
	char perm_domain[SYS_NMLN];
	char		errmess[1024];
	int fd;
	struct ifconf ifc;	/* interface config buffer */
	struct ifreq *ifr;	/* ptr to interface request */
	int i, status;

	if (set_timezone(FACTORY_TIMEZONE) != 0)
		return (ADMUTIL_UNCONF_TZ);

	save_locale("C");

	/*
	 * Now reset the root entry in the passwd/shadow table.  First do a
	 * method call to retrieve the entry, then send back everything but the
	 * passwd field, resulting in clearing it.
	 */
	if (set_root_password("", errmess) != SUCCESS)
		return (ADMUTIL_UNCONF_PW);

	/*
	 * Remove NIS+ cold start file, disabling NIS+ client side
	 */
	if (unlink(NIS_COLD_START) && (errno != ENOENT))
		return (ADMUTIL_UNCONF_COLD);

	/*
	 * Retrieve the domain for the system.  Use the method because we want
	 * both the kernel's notion and the filesystem's notion so we will still
	 * work in single-user mode, when the kernel has no domain set.
	 */
	if (get_domain(curr_domain, perm_domain) != 0)
		return (ADMUTIL_UNCONF_DOM);

	if (strlen(curr_domain) == 0)
		strcpy(curr_domain, perm_domain);

	if (strlen(curr_domain)) {
		/*
		 * Now remove the YP servers file for the current domain, as
		 * well as the binding directory for the current domain,
		 * disabling NIS.
		 */
		sprintf(yp_serv_file, "%s/%s/%s", YP_BINDING_DIR, curr_domain,
			YPSERV_FILE);
		if (unlink(yp_serv_file) && (errno != ENOENT))
			return (ADMUTIL_UNCONF_YP);

		sprintf(yp_serv_file, "%s/%s/%s", YP_BINDING_DIR, curr_domain,
			YPCACHE_FILE);
		if (unlink(yp_serv_file) && (errno != ENOENT))
			return (ADMUTIL_UNCONF_YP);

		sprintf(yp_serv_file, "%s/%s", YP_BINDING_DIR, curr_domain);
		if (rmdir(yp_serv_file) && (errno != ENOENT))
			return (ADMUTIL_UNCONF_YP);
	}

	/*
	 * Restore the nameservice switch to its initial default
	 */
	if (config_nsswitch(TEMPLATE_NIS) != 0)
		return (ADMUTIL_UNCONF_NS);

	/*
	 * Delete the defaultdomain file.
	 */
	if (unlink(DEFAULTDOMAIN_FILE) && (errno != ENOENT))
		return (ADMUTIL_UNCONF_DFD);

	/*
	 * Delete the defaultrouter file.
	 */
	if (unlink(DEFAULTROUTER_FILE) && (errno != ENOENT))
		return (ADMUTIL_UNCONF_DFR);

	/*
	 * Remove the subnet mask table.
	 */
	if (unlink(NETMASKS_FILE) && (errno != ENOENT))
		return (ADMUTIL_UNCONF_NTM);

	/*
	 * Remove the file containing our nodename.
	 */
	if (unlink(NODENAME_FILE) && (errno != ENOENT))
		return (ADMUTIL_UNCONF_NN);

	/*
	 * Remove the sysid configuration file
	 */
	if (!keep_preconfig_file && 
	    unlink(PRECONFIG_FILE) && (errno != ENOENT))
		return (ADMUTIL_UNCONF_PF);

	/*
	 * Now remove the entries for this host from the loopback transport
	 * name-to-address mapping files.
	 */
	if (sysinfo(SI_HOSTNAME, curr_nodename, sizeof (curr_nodename)) == -1)
		return (ADMUTIL_UNCONF_SI);

	if ((status = remove_lb_ntoa_entry(curr_nodename)) != 0)
		fprintf(debugfp, "Error in remove loopback entry: %d\n",
			status);

	/*
	 * Save the current hosts file, then reset it to initial state.
	 */
	if (rename(HOSTS_FILE, HOSTS_FILE_OLD) && (errno != ENOENT))
		return (ADMUTIL_UNCONF_SH);

	if ((status = set_ent_hosts(LOOPBACK_IP, LOCAL_HOST, LOG_HOST, errmess))
	    != SUCCESS)
		fprintf(debugfp, "Error in remove host entry: %d\n", status);

	/*
	 * try to clean out nfs mounts from vfstab
	 */
	system("cp /etc/vfstab /etc/vfstab.orig;\
		nawk '{if (substr($1, 1, 1) == \"#\") print $0;\
			else if ($4 != \"nfs\") print $0}'\
			/etc/vfstab >/etc/vfstab.sys-unconfig;\
		mv /etc/vfstab.sys-unconfig /etc/vfstab");

	/*
	 * Get the list of network interfaces currently installed and
	 * remove the /etc/hostname.xx files.
	 */
	if (get_net_if_names(&ifc) != 0)
		return (ADMUTIL_UNCONF_IF);

	for (ifr = ifc.ifc_req, i = (ifc.ifc_len / sizeof (struct ifreq));
	    i > 0; --i, ++ifr) {
		if (strcmp(ifr->ifr_name, LOOPBACK) != 0) {
			sprintf(t_hostname, IF_NAME_FMT, ifr->ifr_name);
			unlink(t_hostname);
		}
	}

	/*
	 * Now create the file used by bcheckrc to control retries on
	 * RARP & bootparams requests during boot.
	 */
	if ((fd = open(UNCONFIGURED_FILE, O_CREAT)) == -1)
		return (ADMUTIL_UNCONF_OP);

	(void) close(fd);

	/*
	 * Now create the devfs reconfiguration file.
	 */
	if ((fd = open(RECONFIGURE_FILE, O_CREAT)) == -1)
		return (ADMUTIL_UNCONF_OP);

	(void) close(fd);

	return (0);
}
