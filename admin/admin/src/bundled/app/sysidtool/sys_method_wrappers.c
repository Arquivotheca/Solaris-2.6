#pragma	ident	"@(#)sys_method_wrappers.c	1.62	96/10/01 SMI"

/*
 *  Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <sys/param.h>
#include <dirent.h>
#include <sys/dirent.h>
#include <fcntl.h>
#include <sys/procfs.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/openpromio.h>
#include <time.h>
#include <setjmp.h>
#include "cl_database_parms.h"
#include "sysidtool.h"
#include "sysid_msgs.h"
#include "prompt.h"
#include "admutil.h"

#define	PROC_DIR "/proc"
#define	ROUTE_DAEMON	"in.routed"
#define	SLEEP_INTERVALS	5
#define	INTERVAL	5
#define	HALT		"0"

#define	TRANS_LIST	"/etc/transfer_list"

static int check_daemon(char *, pid_t *);
static void add_2_xfer(char *name, char *pkg);
static int ctok(int clicks);
static int replace_hostname(char *, char *, int);

static char sig_name[80];

/*ARGSUSED*/
static void
bug(int sig)
{
	fprintf(debugfp, "Signal! %s %d\n", sig_name, sig);
	fclose(debugfp);
	abort();
}

FILE *
open_log(char *name)
{
	FILE *fp;

	strcpy(sig_name, name);

	(void) signal(SIGSEGV, bug);
	(void) signal(SIGBUS, bug);

	if ((fp = fopen(DEBUGFILE, "a")) == NULL) {
		if ((fp = fopen("/tmp/sysidtool.log", "a")) == NULL) {
			fprintf(stderr, "Unable to open debugfile %s\n",
				DEBUGFILE);
			fp = stderr;
		}
	}
	else
		chmod(DEBUGFILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	fprintf(fp, "%s start\nfree VM: %d\n", name, free_vm());

	return (fp);
}

void
log_time()
{
	time_t	clock_val;
	struct tm *tp;

	clock_val = time(NULL);
	tp = localtime(&clock_val);
	fprintf(debugfp, "%ld %ld %02d:%02d:%02d\n", clock_val, timezone,
		tp->tm_hour, tp->tm_min, tp->tm_sec);
}

/*
 * Get the IP address, if it is set on the specified network interface.
 */

int
get_net_ipaddr(if_name, ip_addr)
char *if_name, *ip_addr;
{
	int		status;

	fprintf(debugfp, "get_net_ipaddr: %s\n", if_name);

	/*
	 * Get the IP address, if it is set on the i/f. This
	 * method takes the i/f name as input.
	 */

	if (testing)
		status = (*sim_handle())(SIM_GET_NET_IF_IP_ADDR_MTHD,
		    if_name, ip_addr, (char *)NULL);
	else if ((status = get_net_if_ip_addr(if_name, ip_addr)) != 0)
		fprintf(debugfp, "get_net_ipaddr: %s\n", strerror(errno));

	/* If the ip address is all zeros, it wasn't really set. */
	if (status == 0 && strcmp(ip_addr, "0.0.0.0") != 0) {
		fprintf(debugfp, "get_net_ipaddr: got %s\n", ip_addr);
		return (SUCCESS);
	}

	*ip_addr = NULL;
	return (FAILURE);
}

/*
 * This routine sets the hostname in the following places
 * in the following order:
 *
 * 	hostname set in /etc/nodename
 * 	hostname set in loopback transport mapping files
 *	loopback set in /etc/hosts
 *
 * If the system is networked (i.e. if_name != NULL):
 *
 *	hostname and ip addr set in /etc/hosts
 *	hostname set in /etc/hostname.<if_name>
 *
 * Returns TRUE if successful, and FALSE otherwise.
 */


int
set_net_hostname(char *hostname, char *ip_addr, char *if_name, int rw)
{
	char		errmess[MAXPATHLEN+1];
	char		aliases[MAX_HOSTNAME+MAX_HOSTNAME+5];
	char		*aliasp;
	int		status;

	fprintf(debugfp, "set_net_hostname: %s, %s, %s\n", hostname,
		(ip_addr ? ip_addr : "NULL"), if_name);

	/*
	 * Set the hostname. If the syntax is not valid,
	 * show the error to the user.  This method takes
	 * the hostname, and IP addr as input, as well as a
	 * parameter that specifies where this should be
	 * placed. We want this hostname set in the UFS
	 * files.
	 *
	 */

	/*
	 * First set the hostname in /etc/nodename.
	 */

	if (testing)
		status = (*sim_handle())(SIM_SET_NODENAME_MTHD,
		    hostname, TE_NOWANDBOOT_BITS, errmess);
	else if ((status = set_nodename(hostname, TE_NOWANDBOOT_BITS)) != 0) {
		switch (status) {
		case ADMUTIL_SETNN_BAD:
			sprintf(errmess, gettext("Invalid hostname: %s"),
				hostname);
			break;

		case ADMUTIL_SETNN_SYS:
			sprintf(errmess, gettext("sysinfo failed: %s"),
				strerror(errno));
			break;

		default:
			sprintf(errmess, gettext("replace_db failed: %s"),
				strerror(status));
			break;
		}
	}

	if (status != SUCCESS) {
		fprintf(debugfp, "set_net_hostname: set_nodename failed: %s\n",
			errmess);
		prompt_error(SYSID_ERR_BAD_NODENAME, hostname,
			errmess);
		return (FALSE);
	}

	/*
	 * Up date the loopback transport
	 * name-to-address mapping files.
	 */

	if (testing)
		status = (*sim_handle())(SIM_SET_LB_NTOA_MTHD,
		    hostname, errmess);
	else if ((status = set_lb_ntoa_entry(hostname)) != 0) {
		switch (status) {
		case ADMUTIL_SETLB_BAD:
			sprintf(errmess, gettext("Invalid hostname: %s"),
				hostname);
			break;

		default:
			sprintf(errmess, gettext("set_lb_ntoa_entry: %s"),
				strerror(errno));
			break;
		}
	}

	if (status != SUCCESS) {
		fprintf(debugfp,
			"set_net_hostname: set_lb_ntoa_entry failed: %s\n",
			errmess);
		prompt_error(SYSID_ERR_BAD_LOOPBACK, hostname, errmess);
		return (FALSE);
	}

	/*
	 * Next set the loopback entry in the UFS hosts file.
	 */

	if (ip_addr == NULL) {
		(void) strcpy(aliases, LOG_HOST);
		(void) strcat(aliases, " ");
		(void) strcat(aliases, hostname);
		aliasp = aliases;
	} else {
		aliasp = NULL;
	}
	if (set_ent_hosts(LOOPBACK_IP, LOCAL_HOST, aliasp, errmess)
	    != SUCCESS) {

		prompt_error(SYSID_ERR_BAD_HOSTS_ENT, LOCAL_HOST, errmess);
		return (FALSE);
	}

	/*
	 * If there is no network, then we're done.
	 */

	if (ip_addr == NULL)
	    return (TRUE);

	/*
	 * Next set the hostname in the UFS hosts file.
	 */
	(void) strcpy(aliases, LOG_HOST);
	aliasp = aliases;

	if (set_ent_hosts(ip_addr, hostname, aliasp, errmess) != SUCCESS) {

		prompt_error(SYSID_ERR_BAD_HOSTS_ENT, hostname, errmess);
		return (FALSE);
	}

	/*
	 * Now set the hostname in /etc/hostname.<if_name>
	 */

	if ((status = replace_hostname(if_name, hostname, rw)) != 0) {
		fprintf(debugfp, "replace_hostname failed: %d\n", status);
	} else {
		char nm[256];

		sprintf(nm, "/etc/hostname.%s", if_name);

		add_2_xfer(nm, "SUNWcsr");
	}

	return (TRUE);
}

/*
 * try /etc version first.  This works for diskless.  If it fails, that's
 * because we're booted off of CD image which is ro.  Use /tmp/root instead.
 */
static int
replace_hostname(char *if_name, char *newent, int rw)
{
	FILE *ifp, *ofp;	/* Output file */
	char *tmp;		/* Temp file name and location */
	char ufs_db[MAXPATHLEN];
	int serrno;		/* Saved errno for cleanup cases */
	int status;

	fprintf(debugfp, "replace_hostname: %s %s %d\n", if_name, newent, rw);

	/*
	 * Generate temporary file name to use.  We make sure it's in the same
	 * directory as the db we're processing so that we can use rename to
	 * do the replace later.  Otherwise we run the risk of being on the
	 * wrong filesystem and having rename() fail for that reason.
	 */
	if (testing) {
		status = (*sim_handle())(SIM_REPLACE_HOSTNAME, if_name, newent);
		return (status);
	}

	if (rw) {
		sprintf(ufs_db, "/etc/hostname.%s", if_name);
		tmp = tempfile("/etc");
	} else {
		sprintf(ufs_db, "/tmp/root/etc/hostname.%s", if_name);
		tmp = tempfile("/tmp");
	}

	fprintf(debugfp, "replace_hostname: open %s\n", tmp);
	ofp = fopen(tmp, "w");
	if (ofp == NULL) {
		fprintf(debugfp, "replace_hostname: open failed %d\n", errno);
		(void) free(tmp);
		return (errno);
	}

	if (chmod(tmp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1) {
		serrno = errno;
		(void) fclose(ofp);
		(void) unlink(tmp);
		return (serrno);
	}

	/* Quick check to make sure we have read & write rights to the file */
	if ((ifp = fopen(ufs_db, "r+")) != NULL)
		(void) fclose(ifp);
	else if (errno != ENOENT) {
		serrno = errno;
		(void) fclose(ofp);
		(void) unlink(tmp);
		return (serrno);
	}

	/*
	 * Write out the data, close and then rename to overwrite old file.
	 */
	if (fprintf(ofp, "%s\n", newent) == EOF) {
		serrno = errno;
		(void) fclose(ofp);
		(void) unlink(tmp);
		(void) free(tmp);
		return (serrno);
	}

	if (fsync(ofp->_file)) {
		serrno = errno;
		(void) unlink(tmp);
		(void) free(tmp);
		return (serrno);
	}

	(void) fclose(ofp);

	fprintf(debugfp, "replace_hostname: rename %s %s\n", tmp, ufs_db);
	if (rename(tmp, ufs_db) != 0) {
		serrno = errno;
		(void) unlink(tmp);
		return (serrno);
	} else {
		(void) free(tmp);
		return (0);
	}
}

static int
check_daemon(char *daemon, pid_t *pid)
{
	DIR *dirptr;
	dirent_t *direntptr;
	prpsinfo_t proc_info;
	int proc_fd;
	int retval;
	char buf[MAXPATHLEN];
	int zombie_process_found = 0;

	/*
	 * Check to see if daemon passed as input is running
	 */
	dirptr = opendir(PROC_DIR);
	if (dirptr == (DIR *) NULL) {
		return (-1);
	}
	while ((direntptr = readdir(dirptr)) != NULL) {
		(void) sprintf(buf, PROC_DIR"/%s", direntptr->d_name);
		if ((proc_fd = open(buf, O_RDONLY)) < 0) {
			continue;	/* skip this one */
		}
		retval = ioctl(proc_fd, PIOCPSINFO, &proc_info);
		if (retval != 0) {
			(void) close(proc_fd);
			continue;	/* skip this one also */
		}

		/*
		 * We look for a match of the process name to
		 * what gets passed in. If we find a match,
		 * we save the pid and exit with proper status.
		 * If we find a match, and that process is in
		 * the zombie, we save the pid and keep looking
		 * just in case there is another version there
		 * that is not a zombie. If there is, that one
		 * gets returned. If there isn't the zombie
		 * gets returned with the proper return code.
		 */

		if (strstr(proc_info.pr_psargs, daemon)
			!= (char *) NULL) {
			fprintf(debugfp,
			    "\"%s\" matches \"%s\", pid = %d, zomb = %d\n",
			    daemon, proc_info.pr_psargs, proc_info.pr_pid,
			    proc_info.pr_zomb);
			if (proc_info.pr_zomb == 0) {
				if (pid != (pid_t *) NULL)
					*pid = proc_info.pr_pid;
				(void) close(proc_fd);
				(void) closedir(dirptr);
				return (0);
			} else {
				if (pid != (pid_t *) NULL)
					*pid = proc_info.pr_pid;
				zombie_process_found++;	/* keep looking */
			}
		}
		(void) close(proc_fd);
	}
	(void) closedir(dirptr);
	return (-1);
}

int
do_netmask(char *netmask, char *netnum, char *if_name)
{
	char errmess[1024];

	fprintf(debugfp, "do_netmask: %s %s %s\n", netmask, netnum, if_name);

	if (set_ent_netmask(netnum, netmask, errmess) != SUCCESS) {
		prompt_error(SYSID_ERR_STORE_NETMASK, netmask, netnum, errmess);
		return (0);
	}

	if (set_net_netmask(if_name, netmask, errmess) != SUCCESS) {
		prompt_error(SYSID_ERR_BAD_NETMASK, netmask, if_name, errmess);
		return (0);
	}
	return (1);
}

/*
 * Set the netmask on the network interface.
 */

int
set_net_netmask(if_name, netmask, errmess)
char *if_name, *netmask, *errmess;
{
	pid_t		pid;
	int		i;
	char 		buff[80];
	Prompt_t	prompt_id = (Prompt_t)0;
	int		status;

	fprintf(debugfp, "set_net_netmask: %s, %s\n", if_name, netmask);

	if (testing)
		status = (*sim_handle())(SIM_SET_NET_IF_IP_NETMASK_MTHD,
		    if_name, netmask, errmess);
	else if ((status = set_net_if_ip_netmask(if_name, netmask)) != 0) {
		switch (status) {
		case ADMUTIL_SETMASK_BAD:
			sprintf(errmess, gettext("Invalid netmask: %s"),
				netmask);
			break;

		case ADMUTIL_SETMASK_SOCK:
			sprintf(errmess, gettext("socket: %s"),
				strerror(errno));
			break;

		default:
			sprintf(errmess, gettext("ioctl: %s"),
				strerror(errno));
			break;
		}
	}
	else
		errmess[0] = '\0';

	if (status != SUCCESS) {
		fprintf(debugfp, "set_net_netmask: method failed\n");
		return (status);
	}

	if (testing)
		status = (*sim_handle())(SIM_CHECK_DAEMON, ROUTE_DAEMON, &pid);
	else
		status = check_daemon(ROUTE_DAEMON, &pid);

	if (status == 0) {
		if (testing)
			status = (*sim_handle())(SIM_KILL, pid, SIGHUP);
		else
			status = kill(pid, SIGHUP);
		if (status != 0) {
			fprintf(debugfp, "kill %s\n", strerror(errno));
		} else {
			prompt_id = (Prompt_t)0;
			for (i = SLEEP_INTERVALS; i > 0; --i) {
				(void) sprintf(buff, PLEASE_WAIT,
					(i * INTERVAL));
				prompt_id = prompt_message(prompt_id, buff);
				(void) sleep(INTERVAL);
			}
			prompt_dismiss(prompt_id);
		}
	} else {
		fprintf(debugfp, "set_net_netmask: couldn't find %s in %s\n",
		    ROUTE_DAEMON, PROC_DIR);
	}

	return (SUCCESS);
}

/*
 * Routines used to set the date using rdate
 */

int
set_rdate()
{
	int	status;
	char	*a[3];

	fprintf(debugfp, "set_rdate\n");
	log_time();

	a[0] = "/usr/bin/rdate";
	a[1] = "timehost";
	a[2] = NULL;
	if (testing)
		status = (*sim_handle())(SIM_SYNC_DATE_MTHD, "timehost");
	else
		status = WEXITSTATUS(run(a));

	log_time();
	return (status);
}

/*
 * Set the timezone environment variable.
 */

int
set_env_timezone(char *timezone, char *errmess)
{
	int		status;
	static char	env_tz[256];

	fprintf(debugfp, "set_env_timezone: %s\n", timezone);
	log_time();
	errmess[0] = '\0';

	if (testing)
		status = (*sim_handle())(SIM_SET_TIMEZONE_MTHD,
		    timezone, errmess);
	else {
		if ((status = set_timezone(timezone)) != 0) {
			fprintf(debugfp, "set_timezone error %d\n", status);
			if (status == ADMUTIL_SETTZ_BAD)
				sprintf(errmess,
					gettext("Invalid timezone: %s"),
					timezone);
			else
				status = 0;
		}
	}

	(void) sprintf(env_tz, "TZ=:%s", timezone);
	(void) putenv(env_tz);

	log_time();
	return (status);
}

/*
 * Check to see if the nodename is set in the kernel and in /etc/hosts.
 */

int
nodename_set()
{
	int		status;
	char curr_nodename[SYS_NMLN];
	char perm_nodename[SYS_NMLN];

	fprintf(debugfp, "nodename_set\n");

	if (testing)
		status = (*sim_handle())(SIM_GET_NODENAME_MTHD, (char *)NULL);
	else
		status = get_nodename(curr_nodename, perm_nodename);

	if (status == SUCCESS) {
		fprintf(debugfp, "nodename_set: true\n");
		return (TRUE);
	} else
		return (FALSE);

}

/*
 * This routine sets the ip address on the network interface.
 *
 * Returns TRUE if successful, FALSE otherwise.
 */

int
set_net_ipaddr(if_name, ip_addr)
char *if_name, *ip_addr;
{
	struct sockaddr_in sin;
	int status;

	fprintf(debugfp, "set_net_ipaddr: %s, %s\n", if_name, ip_addr);

	sin.sin_family = AF_INET;    /* Address will be an Internet address */
	sin.sin_addr.s_addr = inet_addr(ip_addr);

	if (testing) {
		int serrno;

		status = (*sim_handle())(SIM_SET_IF_ADDR, if_name, &sin,
			&serrno);
		errno = serrno;
	} else {
		int sd;			/* socket descriptor */

		if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			fprintf(debugfp, "bad socket call\n");
			status = -1;
		} else {
			struct ifreq ifr;	/* interface request block */

			memcpy(&ifr.ifr_addr, &sin, sizeof (ifr.ifr_addr));
			strcpy(ifr.ifr_name, if_name);
			status = ioctl(sd, SIOCSIFADDR, (char *) &ifr);
			(void) close(sd);
		}
	}

	if (status != 0) {
		fprintf(debugfp, "set_net_ipaddr: status %d, errno %d (%s)\n",
		    status, errno, strerror(errno));
		prompt_error(SYSID_ERR_BAD_IP_ADDR, ip_addr, if_name);
		return (FALSE);
	}

	return (TRUE);
}

/*
 * This routine sets the "UP" flag on the network interface.
 *
 * Returns TRUE if successful, FALSE otherwise.
 */

int
set_net_ifup(char *if_name)
{
	int		status, ret_stat;
	char		errmess[MAXPATHLEN+1];

	fprintf(debugfp, "set_net_ifup: %s\n", if_name);

	if (testing)
		status = (*sim_handle())(SIM_SET_NET_IF_STATUS_MTHD,
		    if_name, ADMUTIL_UP, ADMUTIL_NO, errmess);
	else if ((status =
	    set_net_if_status(if_name, ADMUTIL_UP, ADMUTIL_NO, "", "")) != 0) {
		switch (status) {
		case ADMUTIL_SETIFS_SOCK:
			sprintf(errmess, gettext("socket: %s"),
				strerror(errno));
			break;

		default:
			sprintf(errmess, gettext("ioctl: %s"),
				strerror(errno));
			break;
		}
	} else
		errmess[0] = '\0';

	if (status != SUCCESS) {

		/*
		 * We couldn't set the "UP" flag appropriately.
		 * Display the error.
		 */
		prompt_error(SYSID_ERR_BAD_UP_FLAG, ADMUTIL_UP, if_name,
			errmess);
		ret_stat = FALSE;
	} else {
		/*
		 * We were able to sucessfully set
		 * the "UP" or "DOWN" flag.
		 */
		ret_stat = TRUE;
	}
	return (ret_stat);
}

/*
 * Get the hostname set on the network interface.
 */
int
get_net_hostname(hostname)
char *hostname;
{
	long status;

	fprintf(debugfp, "get_net_hostname\n");

	/*
	 * Now get the hostname set on the i/f.
	 */
	if (testing)
		if ((*sim_handle())(SIM_SI_HOSTNAME, hostname, MAX_HOSTNAME+1,
		    (char *)NULL) == SUCCESS)
			status = MAX_HOSTNAME;
		else
			status = -1;
	else
		status = sysinfo(SI_HOSTNAME, hostname, MAX_HOSTNAME+1);

	if (status > 1L) {

		fprintf(debugfp, "get_net_hostname: %s\n", hostname);
		if (strcmp(hostname, "localhost") == 0)
			return (FAILURE);
		else
			return (SUCCESS);
	}
	else
		return (FAILURE);
}

/*
 * Get the domainname set on the network interface.
 */
void
get_net_domainname(domainname)
char *domainname;
{
	long status;

	fprintf(debugfp, "get_net_domainname\n");

	/*
	 * Now get the domainname set on the i/f.
	 */
	if (testing)
		status = (*sim_handle())(SIM_SI_SRPC_DOMAIN,
		    domainname, MAX_DOMAINNAME+1, (char *)NULL);
	else
		status = sysinfo(SI_SRPC_DOMAIN, domainname, MAX_DOMAINNAME+1);
	if (status == -1)
		domainname[0] = NULL;

	fprintf(debugfp, "get_net_domainname: %s\n", domainname);
}

/*
 * Set the NIS domainname in the kernel.
 */

int
set_domainname(domainname, errmess)
char *domainname, *errmess;
{
	int		status;

	fprintf(debugfp, "set_domainname: %s\n", domainname);

	if (testing)
		status = (*sim_handle())(SIM_SET_DOMAIN_MTHD,
		    domainname, TE_NOWANDBOOT_BITS, errmess);
	else if ((status = set_domain(domainname, TE_NOWANDBOOT_BITS)) != 0) {
		switch (status) {
		case ADMUTIL_SETDM_BAD:
			sprintf(errmess, gettext("Invalid domain name: %s"),
				domainname);
			break;

		case ADMUTIL_SETDM_SYS:
			sprintf(errmess, gettext("sysinfo failed: %s"),
				strerror(errno));
			break;

		default:
			sprintf(errmess, gettext("replace_db failed: %s"),
				strerror(status));
			break;
		}
		fprintf(debugfp, "set_domainname: %s\n", errmess);
		fflush(debugfp);
	}

	return (status);
}

/*
 * Initialize the aliases file of YP.
 */

int
init_yp_aliases(domain, errmess)
char *domain, *errmess;
{
	int 		status;

	fprintf(debugfp, "init_yp_aliases: %s\n", domain);

	if (testing)
		status = (*sim_handle())(SIM_YP_INIT_ALIASES_MTHD,
		    domain, errmess);
	else {
		struct stat yp_stat;
		char	cmd[1024];

		if (stat("/var/yp/aliases", &yp_stat) != 0) {
			strcpy(errmess, "/var/yp/aliases does not exist");
			status = -1;
		} else {
			sprintf(cmd, "cp /var/yp/aliases /tmp/ypaliases.sysid;\
				echo ypservers `/usr/sbin/ypalias ypservers`\
					>> /tmp/ypaliases.sysid;\
				echo %s %s >> /tmp/ypaliases.sysid;\
				sort /tmp/ypaliases.sysid | uniq \
					> /tmp/ypaliases2.sysid;\
				rm /tmp/ypaliases.sysid;\
				mv /tmp/ypaliases2.sysid /var/yp/aliases;\
				chmod 644 /var/yp/aliases", domain, domain);

			status = system(cmd);
			strcpy(errmess, "shell script failed");
		}
	}

	if (status != 0) {
		fprintf(debugfp, "init_yp_aliases: %s\n", errmess);
		fflush(debugfp);
	}

	return (status);
}

/*
 * Initialize the list of servers for the specified YP domain.
 */

int
init_yp_binding(char *domain, int bcast, char *servers, char *errmess)
{
	int 		status;

	fprintf(debugfp, "init_yp_binding: %s, %d, %s\n", domain, bcast,
		servers);

	if (testing)
		status = (*sim_handle())(SIM_YP_INIT_BINDING_MTHD,
		    domain, bcast, servers,  errmess);
	else {
		struct stat yp_stat;
		char	name[1024];
		int	err;

		status = -1;

		if (stat("/var/yp", &yp_stat) != 0 ||
		    !S_ISDIR(yp_stat.st_mode)) {
			strcpy(errmess, "/var/yp does not exist");
			goto yp_bind_done;
		}

		if (stat("/var/yp/binding", &yp_stat) != 0) {
			if ((err = mkdir("/var/yp/binding", 0755)) != 0) {
				sprintf(errmess,
		"Unable to create binding directory /var/yp/binding: %d",
					err);
				goto yp_bind_done;
			}
		}

		sprintf(name, "/var/yp/binding/%s", domain);
		if (stat(name, &yp_stat) != 0) {
			if ((err = mkdir(name, 0755)) != 0) {
				sprintf(errmess,
				"Unable to create binding directory %s: %d",
					name, err);
				goto yp_bind_done;
			}
		}
		add_2_xfer(name, "SUNWnisr");

		if (! bcast) {
			int fd;

			sprintf(name, "/var/yp/binding/%s/ypservers", domain);

			if ((fd = open(name, O_WRONLY | O_CREAT, 0644)) < 0 ||
			    write(fd, servers, strlen(servers)) < 0) {
				sprintf(errmess,
					"Unable to update binding file %s: %d",
					name, errno);
				goto yp_bind_done;
			}
			close(fd);

			add_2_xfer(name, "SUNWnisr");
		}

		status = 0;
	}

yp_bind_done:

	if (status != 0) {
		fprintf(debugfp, "init_yp_binding: %s\n", errmess);
		fflush(debugfp);
	}

	return (status);
}

static jmp_buf env;

/* ARGSUSED */
static void
sigalarm_handler(int sig)
{
	longjmp(env, 1);
}

/*
 * Initialize NIS+ as the name service of choice on this host.
 */

int
init_nis_plus(int bcast, char *server, char *errmess)
{
	char	cmd[1024];
	int 	status;
	void (*savesig) (int);

	fprintf(debugfp, "init_nis_plus: %d, %s\n", bcast, server);

	savesig = signal(SIGALRM, sigalarm_handler);
	(void) alarm(60);

	if (setjmp(env) != 0) {
		fprintf(debugfp, "init_nis_plus: timed out!\n");
		(void) signal(SIGALRM, savesig);
		return (1);
	}

	if (bcast)
		strcpy(cmd, "/usr/sbin/nisinit -c -B 2>&1 1>/dev/null");
	else
		sprintf(cmd, "/usr/sbin/nisinit -c -H %s 2>&1 1>/dev/null",
			server);

	if (testing)
		status = (*sim_handle())(SIM_NIS_INIT_MTHD,
		    bcast, server, errmess);
	else {
		char buf[BUFSIZ];
		FILE *p;

		buf[0] = 0;
		if ((p = popen(cmd, "r")) != NULL)
			while (fgets(buf, BUFSIZ, p) != NULL);

		status = pclose(p);

		if (buf[0] != '\0')
			strcpy(errmess, buf);

		add_2_xfer("/var/nis/NIS_COLD_START", "SUNWnisr");
	}

	if (status != 0) {
		fprintf(debugfp, "init_nis_plus: %s\n", errmess);
		fflush(debugfp);
	}

	/* restore any SIGALRM handler */
	(void) alarm(0);
	(void) signal(SIGALRM, savesig);
	return (status);
}

/*
 * Set up the /etc/nsswitch.conf file for the specified name serice.
 */

int
setup_nsswitch(nstype, errmess)
char *nstype;
char *errmess;
{
	char		*nsval;
	int		status;

	fprintf(debugfp, "setup_nsswitch: %s\n", nstype);

	nsval = (strcmp(nstype, NIS_VERSION_3) == 0 ? TEMPLATE_NIS_PLUS :
		(strcmp(nstype, NIS_VERSION_2) == 0 ? TEMPLATE_NIS :
		(strcmp(nstype, NO_NAMING_SERVICE) == 0 ? TEMPLATE_FILES :
		nstype)));

	if (testing)
		status = (*sim_handle())(SIM_CONFIG_NSSWITCH_MTHD,
		    nsval, errmess);
	else if ((status = config_nsswitch(nsval)) != 0) {
		sprintf(errmess,
			gettext("config_nsswitch error %d, errno %d, %s"),
			status, errno, strerror(errno));
		fprintf(debugfp, "setup_nsswitch: %s\n", errmess);
		fflush(debugfp);
	}

	return (status);
}

static void
add_2_xfer(char *name, char *pkg)
{
	struct stat s;
	FILE *fp;

	if (stat(TRANS_LIST, &s) == -1)
		return;

	fprintf(debugfp, "add %s %s to xfer list\n", name, pkg);

	if ((fp = fopen(TRANS_LIST, "a")) == NULL) {
		fprintf(debugfp, "unable to open xfer list\n");
		return;
	}

	if (fprintf(fp, "%s %s\n", name, pkg) < 0)
		fprintf(debugfp, "unable to write xfer list\n");

	fclose(fp);
}

/* return free vm size in K */
int
free_vm()
{
	struct anoninfo ai;
	int available;

	if (swapctl(SC_AINFO, &ai) == -1)
		return (0);

	available = ai.ani_max - ai.ani_resv;

	return (ctok(available));
}

static int
ctok(int clicks)
{
	static int factor = -1;

	if (factor == -1)
		factor = sysconf(_SC_PAGESIZE) >> 10;
	return (clicks*factor);
}

/*
 * return 1 if "name" is a boot argument, 0 otherwise
 */
int
bootargset(char *name)
{

#if defined(__i386)
/* x86 specifc - from rootprop_io.h, which is not in build env. */
#define ROOTPROP_LEN    1
#define ROOTPROP_PROP   2
 
	int fd;
	int status = 0;
	struct {
		int pnamelen;
		char * pname;
		int pbuflen;
		char * pbuf;
	} args;
	char *p;

	if ((fd = open("/dev/rootprop", O_RDONLY)) == -1)
		return (0);

	args.pname = "boot-args";
	args.pnamelen = strlen("boot-args")+1;

	if (ioctl(fd, ROOTPROP_LEN, &args) != -1) {
		if ((args.pbuf = (char *)malloc(args.pbuflen)) != NULL) {
			args.pname = "boot-args";
			args.pnamelen = strlen("boot-args")+1;

			if (ioctl(fd, ROOTPROP_PROP, &args) != -1) {
				p = strtok(args.pbuf, " \t");
				while (p) {
					if (strcmp(p, name) == 0) {
						status = 1;
						break;
					}
					p = strtok(NULL, " \t");
				}
			}
		}
	}
	close(fd);
	return (status);
#else
	int fd;
	int status = 0;
	char foo[256];
	struct openpromio *op = (struct openpromio *) foo;
	char *p;

	op->oprom_size = 128;

	if ((fd = open("/dev/openprom", O_RDONLY)) == -1)
		return (0);

	if (ioctl(fd, OPROMGETBOOTARGS, (caddr_t)op) != -1) {
		p = strtok(op->oprom_array, " \t");
		while (p) {
			if (strcmp(p, name) == 0) {
				status = 1;
				break;
			}
			p = strtok(NULL, " \t");
		}
	}
	close(fd);
	return (status);
#endif
}

void
halt()
{
	set_run_level(HALT);
}

run(char *const argv[])
{
	int	stat, fd;
	void (*savesig) (int);

	fprintf(debugfp, "run: %s\n", argv[0]);
	fprintf(debugfp, "free VM: %d\n", free_vm());

	if (testing)
		return (0);

	savesig = signal(SIGCHLD, SIG_DFL);

	switch (fork()) {
	case -1:
		fprintf(debugfp, "run: fork failed\n");
		return (-1);

	default:
		wait(&stat);
		(void) signal(SIGCHLD, savesig);
		fprintf(debugfp, "run status: %d\n", WEXITSTATUS(stat));
		fprintf(debugfp, "free VM: %d\n", free_vm());
		return (stat);

	case 0: /* child */
		break;
	}

	/* child */

	for (fd = 0; fd < 64; fd++)
		close(fd);

	if (open("/dev/null", O_RDONLY) == -1)
		exit(-1);

	if (open("/dev/null", O_WRONLY) == -1)
		exit(-1);

	if (open("/dev/null", O_WRONLY) == -1)
		exit(-1);

	execv(argv[0], argv);
	exit(-1);
	/*NOTREACHED*/
}
