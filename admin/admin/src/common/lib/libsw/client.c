#ifndef lint
#pragma ident "@(#)client.c 1.17 95/02/10 SMI"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */
#include "sw_lib.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netdir.h>
#include <signal.h>
#include <errno.h>

/* Local Statics and Constants */

#define HOSTSFILE	"/etc/hosts"
#define DOMAINNAMEFILE  "/etc/defaultdomain"

int			dataless_fs_ok = 0;

/* Public Function Prototypes */

char *		swi_name2ipaddr(char *);
int		swi_test_mount(Remote_FS *, int);
TestMount	swi_get_rfs_test_status(Remote_FS *);
int		swi_set_rfs_test_status(Remote_FS *, TestMount);

/* Local Function Prototypes */

static void	_alarm_handler(int);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * name2ipaddr()
 *
 * Parameters:
 *
 * Return:
 *
 * Status:
 *	public
 */
char    *
swi_name2ipaddr(char * hostname)
{
	FILE           *fp;
	char            buf[BUFSIZ + 1];
	char           *cp;
	static char     ipaddr[IP_ADDR];
	struct hostent *hostinfo;
	char           *hostaddr;

	/*
	 * if name service is running, use hostinfo = gethostbyname(hostname)
	 * hostaddr = inet_ntoa(*hostinfo->h_addr_list))
	 * 
	 * if no name service, check /etc/hosts
	 */

	(void) memset(ipaddr, '\0', IP_ADDR);
	if ((fp = fopen(DOMAINNAMEFILE, "r")) != (FILE *) NULL) {
		/* name service is possible */
		if (fgets(buf, BUFSIZ, fp))
			/* is there a 'real' domainname? */
			if (!strstr(buf, "none") && !strstr(buf, "noname"))
				/* host name obtainable from a name service? */
				if (hostinfo = gethostbyname(hostname))
					/* can we get the ip addr of the host?*/
					if (hostaddr = (char *) inet_ntoa(*((struct in_addr *)hostinfo->h_addr)))
						(void) strcpy(ipaddr, hostaddr);
		fclose(fp);
	}
	if (*ipaddr == '\0') {
		if ((fp = fopen(HOSTSFILE, "r")) == (FILE *) NULL) {
			return(NULL);
		}
		cp = '\0';
		while (fgets(buf, BUFSIZ, fp) != (char *) NULL) {
			/* does this line have 'hostname' in it? */
			if (strstr(buf, hostname)) {
				/* 
				 * chop line at first space or tab after ipaddr
				 */
				for (cp = buf; cp && *cp && *cp != ' ' && 
							*cp != '\t'; ++cp)
					;

				if (cp && *cp)
					*cp = '\0';
				(void) strcpy(ipaddr, buf);
				break;
			}
		}
	
		fclose(fp);
	}
	return (ipaddr);
}

/*
 * test_mount
 *	This function test mounts the /usr, /usr/kvm, and optional pathnames
 *	found in the Clientfs structure.
 * Parameters:
 *	rfs	- name of remote file system to test
 *	sec	- second timeout on mount attempt:
 *			0	- default
 *			# > 0	- # of seconds to wait before interrupt
 * Return Value :
 *	!0	- if one of the paths is invalid
 *	 0	- if both of the paths are valid
 * Status:
 *	public
 */
int
swi_test_mount(Remote_FS * rfs, int sec)
{
	char	cmd[MAXPATHLEN *2 + 16];
	void	(*func)();

	/* Let's test mount the paths */

	if(path_is_readable("/tmp/a") != SUCCESS) {
		sprintf(cmd, "mkdir /tmp/a");
		if(system(cmd))
			return(ERR_NOMOUNT);
	}

	sprintf(cmd, "/usr/sbin/mount -o retry=0 %s:%s /tmp/a >/dev/null 2>&1",
				rfs->c_ip_addr, rfs->c_export_path);

	/* set a timeout on the mount tests */

	if (sec > 0) {
		func = signal(SIGALRM, _alarm_handler);
		(void) alarm(sec);
	}

	if (system(cmd) != 0) {
		if (sec > 0) {
			(void) alarm(0);
			(void) signal(SIGALRM, func);
		}
		return(ERR_NOMOUNT);
	}

	if (sec > 0) {
		(void) alarm(0);
		(void) signal(SIGALRM, func);
	}

	(void) sprintf(cmd, "/usr/sbin/umount /tmp/a");
	if (system(cmd))
		return(ERR_NOMOUNT);

	return(SUCCESS);
}

/*
 * get_rfs_test_status()
 *	
 * Parameters:
 *	rfs	-
 * Return:
 *	other	-
 *	ERR_INVALID
 * Status:
 *	public
 */
TestMount
swi_get_rfs_test_status(Remote_FS * rfs)
{
	if (rfs != NULL)
		return rfs->c_test_mounted;
	else
		return(ERR_INVALID);	
}

/*
 * set_rfs_test_status()
 *
 * Parameters:
 *	rfs	-
 *	status	-
 * Return:
 *	SUCCESS
 *	ERR_INVALID
 * Status:
 *	publice
 */
int
swi_set_rfs_test_status(Remote_FS * rfs, TestMount status)
{
	if (rfs == (Remote_FS *)NULL)
		return(ERR_INVALID);

	rfs->c_test_mounted = status;
	return(SUCCESS);
}

/* ******************************************************************** */
/*			LOCAL SUPPORT FUNCTIONS				*/
/* ******************************************************************** */

/*
 * _alarm_handler()
 * Parameters:
 *	val	- signal value
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_alarm_handler(int val)
{
	return;
}
