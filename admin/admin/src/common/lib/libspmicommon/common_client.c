#ifndef lint
#pragma ident "@(#)common_client.c 1.3 95/11/27 SMI"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.
 * All Rights Reserved.
 */
#include <errno.h>
#include <netdb.h>
#include <netdir.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/systeminfo.h>
#include "spmicommon_lib.h"

/* public prototypes */

char *		name2ipaddr(char *);
int		test_mount(Remote_FS *, int);
TestMount	get_rfs_test_status(Remote_FS *);
int		set_rfs_test_status(Remote_FS *, TestMount);

/* private prototypes */

static void	_alarm_handler(int);

/*---------------------- public functions -----------------------*/

/*
 * Function:	name2ipaddr
 * Description:	Try to convert a host name to an ip address string
 *		of the form "###.###.###.###". The ip address
 *		string is store in a local static variable which
 *		is overwritten on subsequent calls.
 * Scope:	public
 * Parameters:	name	- [RO] non-NULL string specifying hostname
 * Return:	""	- no ip address translation was found
 *		<###.###.###.###> - string format for IP address
 *			  translation
 */
char *
name2ipaddr(char *name)
{
	struct hostent	*hinfo;
	static char	ipaddr[IP_ADDR];
	char		*cp;

	(void)memset(ipaddr, '\0', IP_ADDR);

	/*
	 * look up the host name using that databases as defined
	 * by /etc/nsswitch.conf
	 */
	if ((hinfo = gethostbyname(name)) != NULL) {
		/*LINTED [alignment ok]*/
		if ((cp = (char *)inet_ntoa(*((struct in_addr *)hinfo->h_addr))) != NULL)
			(void)strcpy(ipaddr, cp);
	}

	return(ipaddr);
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
test_mount(Remote_FS * rfs, int sec)
{
	char	cmd[MAXPATHLEN *2 + 16];
	void	(*func)();

	/* Let's test mount the paths */

	if(path_is_readable("/tmp/a") != SUCCESS) {
		(void) sprintf(cmd, "mkdir /tmp/a");
		if(system(cmd))
			return(ERR_NOMOUNT);
	}

	(void) sprintf(cmd,
		"/usr/sbin/mount -o retry=0 %s:%s /tmp/a >/dev/null 2>&1",
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
get_rfs_test_status(Remote_FS * rfs)
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
set_rfs_test_status(Remote_FS * rfs, TestMount status)
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
/*ARGSUSED0*/
static void
_alarm_handler(int val)
{
	return;
}
