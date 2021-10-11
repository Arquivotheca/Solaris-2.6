#ifndef lint
#pragma ident "@(#)v_rfs.c 1.40 96/06/23 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_rfs.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <netdb.h>
#include <netdir.h>
#include <libintl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include "pf.h"
#include "v_types.h"
#include "v_rfs.h"
#include "v_misc.h"

/*
 * This file contains the View interface layer to the remote file system
 * library.  It provides an abstraction layer for accessing the functionality
 * that defines and manages remote file systems.
 *
 * The exposed abstraction is an array of remote file systems.  Each
 * remote file system is represented by a 4-tuple:
 *	a local mount point
 *	a server (host) name
 * 	an IP address for the server
 *	a file system exported by the server
 *
 * The interface maintains a 'current' remote file system, the operations
 * which can be performed on the current rfs are:
 * 	get/set current_rfs's status
 * 	get/set current_rfs's local mount point
 *	get/set current_rfs's server name
 * 	get/set current_rfs's server IP address
 * 	get/set current_rfs's server file system
 * 	test mount current_rfs
 * 	undo edits to all fields for current_rfs
 *
 * As a ease-of-use enhancement, we will attempt to dynamically construct a
 * list of file sysem exported by a server.  Hence, need to provide
 * operations:
 * 	to find an IP addr given a hostname.
 * 	to find file systems exported by a server
 * 	to access (next/prev) the list of exported file systems
 * 	to get the name of the i'th exported file system
 *
 */

/* typedefs and defines */

/* Static Globals: */

/*
 * Array of ClientFS structures for dataless client's filesystems
 *
 * currently track 3 of them: /usr, /opt1, /opt2
 *
 * array is exported so that suninstall code can get hold of them
 */

static Remote_FS *_current_rfs = (Remote_FS *) NULL;
static Remote_FS *_head_rfs = (Remote_FS *) NULL;
static int _current_rfs_index = -1;

/* Forward declarations: */
static char *_v_conv(char, char, char *);
static int _v_add_rfs(Remote_FS *, char *, char *, char *, char *);
static void sigalarm_handler(int);

Remote_FS *
v_get_first_rfs(void)
{
	return (_head_rfs);
}

/*
 * adds a new Remote_FS struct to the list of remote file systems. Pass in
 * the head of the list, and the 4 bits of important info.
 *
 * creates a new struct and adds to the end of the list pointed to by head. if
 * head is null, the head pointer to the list of remote file systems is
 * initialized to the node just created.
 */
static int
_v_add_rfs(Remote_FS * head, char *srvr, char *ip, char *path, char *mount)
{
	Remote_FS *tmp;
	Remote_FS *newrfs;
	int n = v_get_n_rfs();

	newrfs = (Remote_FS *) xcalloc(sizeof (Remote_FS));

	if (srvr)
		newrfs->c_hostname = xstrdup(srvr);
	if (ip)
		newrfs->c_ip_addr = xstrdup(ip);
	if (path)
		newrfs->c_export_path = xstrdup(path);
	if (mount)
		newrfs->c_mnt_pt = xstrdup(mount);

	if (head == (Remote_FS *) NULL)
		_head_rfs = newrfs;
	else {
		for (n--, tmp = head; n && tmp; n--, tmp = tmp->c_next);

		tmp->c_next = newrfs;
	}

	return (V_OK);

}

void
v_delete_all_rfs(void)
{
	int i;

	for (i = 0; i < v_get_n_rfs(); i++)
		v_delete_rfs(i);
}

/*
 * Delete the i'th remote file system from the list.
 */
void
v_delete_rfs(int i)
{
	Remote_FS *ptr;
	Remote_FS *prev;
	int n = v_get_n_rfs();

	if (i >= n || i < 0)
		return;

	if (i == 0) {

		if (_current_rfs == _head_rfs) {

			/*
			 * We're deleting the first elt, advance current as
			 * well as head.
			 */

			_current_rfs = _current_rfs->c_next;
		}
		ptr = _head_rfs;
		_head_rfs = _head_rfs->c_next;
		free((void *) ptr);

		return;
	}

	for (ptr = _head_rfs; i && ptr; i--) {

		prev = ptr;
		ptr = ptr->c_next;

	}

	if (ptr) {

		prev->c_next = ptr->c_next;

		if (_current_rfs == ptr)
			_current_rfs = ptr->c_next;

		free((void *) ptr);

	}
}

/*
 * expose a function to add new remote file systems
 */
int
v_new_rfs(char *server, char *ipaddr, char *remotepath, char *localpath)
{

	return (_v_add_rfs(_head_rfs, server, ipaddr, remotepath, localpath));
}

/*
 * return number of remtoe file systems
 */
int
v_get_n_rfs(void)
{
	Remote_FS *tmp = _head_rfs;
	int i = 0;

	while (tmp) {

		++i;
		tmp = tmp->c_next;

	}

	return (i);
}

/*
 * return TRUE if a remote file system has been configured
 */
int
v_any_rfs_configed(void)
{
	int n = v_get_n_rfs();
	int i;

	for (i = 0; i < n; i++)
		if (v_rfs_configed(i))
			return (TRUE);

	return (FALSE);
}

/*
 * return TRUE if a remote file system has been configured
 */
int
v_rfs_configed(int i)
{
	int n = v_get_n_rfs();
	Remote_FS *rfs = v_get_first_rfs();

	if (i < n) {

		for (; i && rfs; rfs = rfs->c_next, i--);

		if (rfs &&
		    (rfs->c_mnt_pt != '\0') &&
		    (rfs->c_hostname != '\0') &&
		    (rfs->c_ip_addr != '\0') &&
		    (rfs->c_export_path != '\0'))
			return (TRUE);
		else
			return (FALSE);

	}
	return (FALSE);
}

int
v_set_current_rfs(int i)
{
	Remote_FS *rfs = v_get_first_rfs();
	int n = v_get_n_rfs();

	if (i < n) {

		for (; i && rfs; rfs = rfs->c_next, i--);

		if (rfs) {

			_current_rfs = rfs;
			return (V_OK);

		} else
			return (V_FAILURE);

	}
	return (V_FAILURE);
}

int
v_get_current_rfs(void)
{
	return (_current_rfs_index);
}

/*
 * get i'th remote file system's local mount point
 */
char *
v_get_rfs_mnt_pt(int i)
{
	int n = v_get_n_rfs();
	static char buf[BUFSIZ];
	Remote_FS *rfs = v_get_first_rfs();

	if (i >= 0 && i <= n) {

		for (; i && rfs; rfs = rfs->c_next, i--);

		buf[0] = '\0';
		(void) strcpy(buf, rfs->c_mnt_pt);
		return (buf);

	} else
		return (NULL);

}

/*
 * set i'th remote file system's local mount point to val
 */
int
v_set_rfs_mnt_pt(int i, char *val)
{
	int n = v_get_n_rfs();
	Remote_FS *rfs = v_get_first_rfs();

	if (i >= 0 && i <= n) {

		for (; i && rfs; rfs = rfs->c_next, i--);

		if (rfs->c_mnt_pt)
			free(rfs->c_mnt_pt);

		rfs->c_mnt_pt = xstrdup(val);
		return (V_OK);

	} else
		return (V_FAILURE);
}

/*
 * get i'th file systems server
 */
char *
v_get_rfs_server(int i)
{
	int n = v_get_n_rfs();
	static char buf[BUFSIZ];
	Remote_FS *rfs = v_get_first_rfs();

	if (i >= 0 && i <= n) {

		for (; i && rfs; rfs = rfs->c_next, i--);

		buf[0] = '\0';

		(void) strcpy(buf, rfs->c_hostname);
		return (buf);

	} else
		return (NULL);
}

/*
 * set i'th file systems server
 */
int
v_set_rfs_server(int i, char *val)
{

	int n = v_get_n_rfs();
	Remote_FS *rfs = v_get_first_rfs();

	if (i >= 0 && i <= n) {

		for (; i && rfs; rfs = rfs->c_next, i--);

		if (rfs->c_hostname)
			free(rfs->c_hostname);

		rfs->c_hostname = xstrdup(val);
		return (V_OK);

	} else
		return (V_FAILURE);
}

/*
 * get i'th file systems IP addr
 */
char *
v_get_rfs_ip_addr(int i)
{
	int n = v_get_n_rfs();
	static char buf[BUFSIZ];
	Remote_FS *rfs = v_get_first_rfs();

	if (i >= 0 && i <= n) {

		for (; i && rfs; rfs = rfs->c_next, i--);

		buf[0] = '\0';

		(void) strcpy(buf, rfs->c_ip_addr);
		return (buf);

	} else
		return (NULL);
}

/*
 * set i'th file systems IP addr
 */
int
v_set_rfs_ip_addr(int i, char *val)
{
	int n = v_get_n_rfs();
	Remote_FS *rfs = v_get_first_rfs();

	if (i >= 0 && i <= n) {

		for (; i && rfs; rfs = rfs->c_next, i--);

		if (rfs->c_ip_addr)
			free(rfs->c_ip_addr);

		rfs->c_ip_addr = xstrdup(val);
		return (V_OK);

	} else
		return (V_FAILURE);
}

/*
 * get i'th file systems full path on server
 */
char *
v_get_rfs_server_path(int i)
{
	int n = v_get_n_rfs();
	static char buf[BUFSIZ];
	Remote_FS *rfs = v_get_first_rfs();

	if (i >= 0 && i <= n) {

		for (; i && rfs; rfs = rfs->c_next, i--);

		buf[0] = '\0';
		(void) strcpy(buf, rfs->c_export_path);
		return (buf);

	} else
		return (NULL);
}

/*
 * set i'th file systems full path on server
 */
int
v_set_rfs_server_path(int i, char *val)
{
	int n = v_get_n_rfs();
	Remote_FS *rfs = v_get_first_rfs();

	if (i >= 0 && i <= n) {

		for (; i && rfs; rfs = rfs->c_next, i--);

		if (rfs->c_export_path)
			free(rfs->c_export_path);

		rfs->c_export_path = xstrdup(val);

		return (V_OK);

	} else
		return (V_FAILURE);
}


/*
 * test mount i'th rfs
 */
int
v_test_rfs_mount(int i)
{
	Remote_FS *rfs = v_get_first_rfs();

	if (i >= 0 && i <= v_get_n_rfs()) {

		for (; i && rfs; rfs = rfs->c_next, i--);

	}
	if (test_mount(rfs, 20) == SUCCESS)
	    return (V_OK);
	else
	    return (V_FAILURE);
}


/*
 * get the test mount status for the i'th rfs
 */
V_TestMount_t
v_get_rfs_test_status(int i)
{

	Remote_FS *rfs = v_get_first_rfs();
	V_TestMount_t retval = V_NOT_TESTED;


	if (i >= 0 && i <= v_get_n_rfs()) {

		for (; i && rfs; rfs = rfs->c_next, i--);

		switch (get_rfs_test_status(rfs)) {
		case TEST_SUCCESS:
			retval = V_TEST_SUCCESS;
			break;

		case TEST_FAILURE:
			retval = V_TEST_FAILURE;
			break;

		case NOT_TESTED:
		default:
			retval = V_NOT_TESTED;
			break;

		}
	}
	return (retval);
}


/*
 * set the test mount status for the i'th rfs FIX FIX FIX -- maybe this
 * should be rolled into v_test_rfs_mount() ?
 */
int
v_set_rfs_test_status(int i, V_TestMount_t status)
{

	Remote_FS *rfs = v_get_first_rfs();
	TestMount test_val = NOT_TESTED;
	int retval = V_FAILURE;


	if (i >= 0 && i <= v_get_n_rfs()) {

		for (; i && rfs; rfs = rfs->c_next, i--);

		switch (status) {
		case V_TEST_SUCCESS:
			test_val = TEST_SUCCESS;
			break;

		case V_TEST_FAILURE:
			test_val = TEST_FAILURE;
			break;

		case V_NOT_TESTED:
		default:
			test_val = NOT_TESTED;
			break;

		}

		if (set_rfs_test_status(rfs, test_val) == SUCCESS)
			retval = V_OK;
		else
			retval = V_FAILURE;
	}
	return (retval);

}


/*
 * --------- Server Exports Stuff ---------------
 */
static char _exported_fs[100][128];
static int _nexports = 0;
static int _current_export = 0;

void
v_clear_export_fs()
{
	_nexports = 0;
	_current_export = -1;
}

int
v_get_n_exports(void)
{
	return (_nexports);
}

int
v_get_current_export_fs(void)
{
	return (_current_export);
}

char *
v_get_export_fs_name(int index)
{
	if (index >= 0 && index < _nexports)
		return (_exported_fs[index]);
	else
		return ((char *) NULL);
}

/*
 * test mount i'th exported fs
 */
int
v_test_export_mount(char *ip_addr, char *export_path)
{

	Remote_FS rfs;

	if (ip_addr && export_path) {

		rfs.c_ip_addr = ip_addr;
		rfs.c_export_path = export_path;

		if (test_mount(&rfs, 20) == SUCCESS)
		    return (V_OK);

	}
	return (V_FAILURE);
}

/*
 * give the showmount command 60 seconds to time out
 */
#define	SHOWMOUNT_TIMEOUT 60
static FILE *fp;

/* ARGSUSED */
static void
sigalarm_handler(int sig)
{
	(void) pclose(fp);
}


/*
 * this is a quick & dirty hack, should probably go rip out the relevant
 * code from showmount.c, but at least this way someone else maintains the
 * underlying code...
 */
int
v_init_server_exports(char *server)
{
	char *cmd = "/usr/sbin/showmount -e";
	char cmdbuf[256], buf[256];
	char *cp;
	int i = 0;
	void (*savesig) (int);

	/*
	 * set up a mechanism to 'break' out of the popen() if the server is
	 * not responding...
	 */
	savesig = signal(SIGALRM, sigalarm_handler);
	(void) alarm(SHOWMOUNT_TIMEOUT);

	/* which file systems is server exporting? */
	(void) sprintf(cmdbuf, "%s %s 2>& 1", cmd, server);

	if ((fp = popen(cmdbuf, "r")) != NULL) {

		while (fgets(buf, 255, fp) != NULL) {

			/*
			 * first line of showmount -e command is:
			 *
			 * `export list for ...:'
			 *
			 * or
			 * 'showmount: 255.255.255.255: RPC: Name to address
			 * translation 'showmount: HOSTNAME: RPC: Program
			 * not registered' 'showmount: HOSTNAME: RPC:
			 * Rpcbind failure - RPC: Timed out'
			 *
			 */
			if (buf[0] == '\0')
				continue;
			else if (strstr(buf, "export list"))
				continue;
			else if (strstr(buf, "Rpcbind failure"))
				break;
			else if (strstr(buf, "not registered"))
				break;
			else if (strstr(buf, "Name to address translation"))
				break;
			else if (strstr(buf, "showmount:"))
				break;

			/*
			 * trim trailing junk off of exported filesystem
			 * junk in SVR4 looks like: /fsname  (host, ...)
			 *
			 * junk in 4.x looks like: /fsname  host, netgroup...
			 */
			if ((cp = strrchr(buf, '(')) != (char *) NULL ||
			    (cp = strrchr(buf, ' ')) != (char *) NULL) {

				*cp-- = '\0';
				while (*cp == ' ')
					*cp-- = '\0';

			}
			/*
			 * save off file system name in array of exported
			 * file systems
			 */

			(void) strcpy(_exported_fs[i++], buf);

		}
		(void) pclose(fp);
	}
	/* restore any SIGALRM handler */
	(void) alarm(0);
	(void) signal(SIGALRM, savesig);

	_nexports = i;

	if (_nexports > 0)
		return (V_OK);
	else
		return (V_FAILURE);
}

char *
v_ipaddr_from_hostname(char *server)
{
	if (server != (char *) NULL && *server != '\0')
		/* from existing ttinstall code (client.c) */
		return (name2ipaddr(server));

	return ((char *) NULL);
}

static char *
_v_conv(char from, char to, char *str)
{
	register char *bp;

	for (bp = str; bp && *bp; ++bp)
		if (*bp == from)
			*bp = to;

	return (str);
}
