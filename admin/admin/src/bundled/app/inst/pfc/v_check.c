#ifndef lint
#pragma ident "@(#)v_check.c 1.38 96/08/13 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc. All rights reserved.
 */
/*
 * Module:	v_check.c
 * Group:	ttinstall
 * Description:
 */

#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <libintl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "pf.h"
#include "v_types.h"
#include "v_disk.h"
#include "v_check.h"
#include "v_misc.h"
#include "v_sw.h"
#include "v_lfs.h"
#include "v_rfs.h"

/* typedefs and defines ... */

/* structure definition ... */

/* global variables ... */

/* private procedure decls ... */
static Depend *_sort_dpnds(Depend *);

/* static variables ... */
static int _n_dependencies = -1;
static int _n_small_filesys = -1;
static Space *	_small_filesys = NULL;
static Depend *	_dependencies = NULL;

/*
 * v_check_disks(void)
 *
 * View layer entry point to routines which perform the pre-install
 * disk & bootdrive consistency checks.
 *
 * input:
 *
 * returns:
 * 	Config_Status_t enum:
 * 		CONFIG_WARNING if problems were detected
 * 		CONFIG_OK	if things look OK.
 */
Config_Status_t
v_check_disks(void)
{
	int		i;
	Errmsg_t *	elp;

	/*
	 * current checks:
	 *
	 * is any drive selected? is root configured on a drive? is there
	 * sufficient swap?
	 *
	 */

	for (i = 0; i < v_get_n_disks(); i++)
		if (v_get_disk_selected(i) != 0)
			break;

	if (i == v_get_n_disks()) {
		v_errno = V_NODISK_SELECTED;
		return (CONFIG_WARNING);
	}
	if (find_mnt_pnt((Disk_t *) 0, (char *) 0, ROOT, (Mntpnt_t *) NULL,
		CFG_CURRENT) == 0) {

		v_errno = V_NO_ROOTFS;
		return (CONFIG_WARNING);
	}
	if (check_disks() > 0) {
		/*
		 * ignore everything except boot configuration and root
		 * file system > 1023 (out-of-reach) check
		 */
		WALK_LIST(elp, get_error_list()) {
			v_errno = elp->code;
			if ((v_errno == D_OUTOFREACH) ||
					(v_errno == D_BOOTCONFIG)) {
				free_error_list();
				return (CONFIG_WARNING);
			}
		}

		free_error_list();
	}
	return (CONFIG_OK);
}

/*
 * v_check_part(void)
 *
 * View layer entry point to routines which perform the pre-install
 * partitioning and file system size checks.
 *
 * input:
 *
 * returns:
 * 	Config_Status_t enum:
 * 		CONFIG_WARNING	if problems were detected
 * 		CONFIG_OK	if things look OK.
 */
Config_Status_t
v_check_part(void)
{

	/*
	 * Make sure each file system has at least the minimum amount
	 * of space we require to lay it out.
	 */
	if ((_small_filesys = ResobjIsComplete(RESSIZE_MINIMUM)) != NULL) {
		_n_small_filesys = -1;
		return (CONFIG_WARNING);
	} else {
		_n_small_filesys = 0;
		return (CONFIG_OK);
	}
}

/*
 * v_get_n_small_filesys(void)
 *
 * View layer entry point to function which returns the number of
 * undersized filesystems.
 *
 * input:
 *
 * returns:
 * 	number of undersized file systems/partitions
 *
 * algorithm:
 * 	count the number of array entries returned by filesys_ok.
 */
int
v_get_n_small_filesys(void)
{
	int i;

	if (_n_small_filesys == -1) {
		for (i = 0; _small_filesys[i].name[0] != '\0'; i++);
		_n_small_filesys = i;
	}

	return (_n_small_filesys);
}

/*
 * v_get_small_filesys(int i)
 *
 * View layer entry point to function which returns the name of the i'th
 * undersized filesystem.
 *
 * input:
 *
 * returns:
 * 	pointer to static buffer containing the name of the i'th small
 * 	file system
 *
 * algorithm:
 */
char *
v_get_small_filesys(int i)
{
	static char buf[128];

	buf[0] = '\0';

	if (i >= 0 && i <= _n_small_filesys)
		(void) strcpy(buf, _small_filesys[i].name);

	return (buf);
}

/*
 * v_get_small_filesys_reqd(int i)
 *
 * View layer entry point to function which returns the required size
 * of the i'th undersized filesystem.
 *
 * input:
 *
 * returns:
 * 	pointer to static buffer containing the required size of the
 * 	i'th small file system (units: Mbytes)
 *
 * algorithm:
 */
char *
v_get_small_filesys_reqd(int i)
{
	double reqdswap;
	static char buf[16];

	buf[0] = '\0';

	if (i >= 0 && i <= _n_small_filesys) {

		if (streq(_small_filesys[i].name, "swap")) {
			v_get_mntpt_req_size(_small_filesys[i].name,
			    &reqdswap);
			(void) sprintf(buf, "%8.2f", reqdswap);
		} else {
			(void) sprintf(buf, "%8.2f",
			    (float) sectors_to_mb(_small_filesys[i].required));
		}
	}
	return (buf);
}

/*
 * v_get_small_filesys_avail(int i)
 *
 * View layer entry point to function which returns the available space
 * in the i'th undersized filesystem.
 *
 * input:
 *
 * returns:
 * 	pointer to static buffer containing the available space in the
 * 	i'th small file system (units: MBytes)
 *
 * algorithm:
 */
char *
v_get_small_filesys_avail(int i)
{
	static char buf[16];
	struct mnt_pnt d;

	buf[0] = '\0';

	if (i >= 0 && i <= _n_small_filesys) {

		if (find_mnt_pnt((Disk_t *) NULL, (char *) NULL,
			_small_filesys[i].name, &d, CFG_CURRENT)) {

			(void) sprintf(buf, "%8.2f",
				(float) blocks_to_mb(d.dp,
				slice_size(d.dp, d.slice)));
		} else
			(void) sprintf(buf, "%8.2f", 0.0);
	}
	return (buf);
}

/*
 * v_check_sw_depends(void)
 *
 * View layer entry point to routines which perform the pre-install
 * software dependency checks.
 *
 * input:
 *
 * returns:
 *	Config_Status_t enum:
 * 		CONFIG_WARNING	if dependency problems were detected
 * 		CONFIG_OK	if things look OK.
 *
 * algorithm:
 *	call the check_sw_depends() function in the software library.
 * 	interpret the return.
 */
Config_Status_t
v_check_sw_depends(void)
{

	_dependencies = (Depend *) NULL;

	if (check_sw_depends()) {

		_dependencies = get_depend_pkgs();
		_dependencies = _sort_dpnds(_dependencies);
		return (CONFIG_WARNING);

	} else
		return (CONFIG_OK);
}

/*
 * ahhhh, this is a hack!  need to get package name into Depend struct...
 */
static Depend *
_sort_dpnds(Depend * list)
{
	return (list);
}

/*
 * v_get_n_depends(void)
 *
 * If any dependency problems were detected, need to notify the user. Need to
 * know how many 'problems' were found...
 *
 * input:
 *
 * returns: number of unsatisfied software _dependencies found by
 * check_sw_depends()
 *
 * algorithm:
 *	retrieve the linked list of problematic packages,
 *	set the global variable '_dependencies' to the head of the list.
 *	traverse the list and count the number of nodes on it,
 *	set the globl variable '_n_dependencies' to the number computed.
 *	return the number computed.
 */
int
v_get_n_depends(void)
{
	int i;
	Depend *dpnd;

	if (_dependencies != (Depend *) NULL) {

		dpnd = _dependencies;
		for (i = 0; dpnd; dpnd = dpnd->d_next, ++i);

		_n_dependencies = i;

	} else
		_n_dependencies = 0;

	return (_n_dependencies);

}

/*
 * v_get_depends_pkgid(int i)
 *
 * As part of displaying the software dependency problems, need to show the
 * package id of the pacakge with the dependency problem.
 *
 * input: index of package of interest.
 *
 * returns: character pointer to static buffer containing the pkgid of i'th
 * dependency package.
 *
 * NB: v_get_depends_pkgname() calls this function to retrieve the pkgid so
 * that we can look up the package name.  However, the UI code also calls
 * this function to get the package name. At the moment, this shouldn't
 * cause any problems, since the UI calls this function and immediately
 * calls ..._pkgname(). (the point is that the pkgid should not change out
 * from under the UI code...) this is kinda of risky, maybe fix later.
 *
 * algorithm: check index validity (> 0 && < _n_dependencies)
 *	traverse list of depdency packages until reaching i'th one.
 *	copy this pacakge's pkgid into buffer.
 *	return buffer.
 */
char *
v_get_depends_pkgid(int i)
{
	Depend *dpnd = get_depend_pkgs();
	static char buf[32];

	buf[0] = '\0';

	if (i >= 0 && i <= v_get_n_depends()) {

		for (; i && dpnd; dpnd = dpnd->d_next, i--);

		if (dpnd)
			(void) strcpy(buf, dpnd->d_pkgid);

	}
	return (buf);

}

/*
 * v_get_depends_pkgname(int i)
 *
 * As part of displaying the software dependency problems, need to
 * show the name of the package with the dependency problem.
 *
 * input:
 *	index of dependency problem of interest.
 *
 * returns:
 *	character pointer to static buffer containing the
 *	full pacakge name of i'th dependency package.
 *
 * algorithm:
 *	check index validity (> 0 && < _n_dependencies)
 *	traverse list of depdency packages until reaching i'th one.
 *	copy this pacakge's name into buffer.
 *	return buffer.
 */
char *
v_get_depends_pkgname(int i)
{
	char *pkgid = (char *) NULL;
	char *pkgname;
	static char buf[256];

	buf[0] = '\0';

	if (i >= 0 && i <= v_get_n_depends()) {

		pkgid = v_get_depends_pkgid(i);

		if (pkgid && (pkgname = v_get_pkgname_from_pkgid(pkgid)))
			(void) strcpy(buf, pkgname);

	}
	return (buf);

}

/*
 * v_get_dependson_pkgid(int i)
 *
 * As part of displaying the software dependency problems, need to show the
 * depended on package's id.
 *
 * input: index of dependency problem of interest.
 *
 * returns: character pointer to static buffer containing the pkgid of i'th
 * dependency's depended on package.
 *
 * NB: v_get_depends_pkgname() calls this function to retrieve the pkgid so
 * that we can look up the package name. However, the UI code also calls
 * this function to get the package name. At the moment, this shouldn't
 * cause any problems, since the UI calls this function and immediately
 * calls ..._pkgname(). the point is that the pkgid should not change out
 * from under the UI code...) this is kinda of risky, maybe fix later.
 *
 * algorithm: check index validity (> 0 && < _n_dependencies)
 *	traverse list of depdency packages until reaching i'th one.
 *	copy this package's pkgidb into buffer.
 *	return buffer.
 */
char *
v_get_dependson_pkgid(int i)
{
	Depend *dpnd = get_depend_pkgs();
	static char buf[32];

	buf[0] = '\0';

	if (i >= 0 && i <= v_get_n_depends()) {

		for (; i && dpnd; dpnd = dpnd->d_next, i--);

		if (dpnd && dpnd->d_pkgidb[0])
			(void) strcpy(buf, dpnd->d_pkgidb);

	}
	return (buf);

}

/*
 * v_get_dependson_pkgname(int i)
 *
 * As part of displaying the software dependency problems, need to
 * show the depended on package's name.
 *
 * input:
 *	index of dependency problem of interest.
 *
 * returns:
 *	character pointer to static buffer containing the
 *	full package name of i'th dependency's dependened on package.
 *
 * algorithm:
 *	check index validity (> 0 && < _n_dependencies)
 *	traverse list of depdency packages until reaching i'th one.
 *	copy this pacakge's name into buffer.
 *	return buffer.
 */
char *
v_get_dependson_pkgname(int i)
{
	char *pkgid;
	char *pkgname;
	static char buf[256];

	buf[0] = '\0';

	if (i >= 0 && i <= v_get_n_depends()) {

		pkgid = v_get_dependson_pkgid(i);

		if (pkgid && (pkgname = v_get_pkgname_from_pkgid(pkgid)))
			(void) strcpy(buf, pkgname);

	}
	return (buf);

}

/*
 * copied from sysidtool's validation routines.
 *
 * v_valid_host_ip_addr:
 *
 * validation routine for checking the validity of a host IP address.  This
 * routine is based on the one found in the libadmutil library of the
 * administrative class hierarchy.
 *
 * "An IP address must contain four sets of numbers separated by "
 * "periods (example 129.200.9.1).")
 *
 * "Each component of an IP address must be between 0 and 254."
 *
 * "The IP address is out of the range of valid addresses."
 *
 * "IP addresses in the range 224.x.x.0 to 254.x.x.255 are reserved."
 *
 */

#define	valid_ip_fmt(ia, a, ap) (sscanf(ia, "%d.%d.%d.%d%n", &a[0], &a[1], \
				&a[2], &a[3], ap) == 4)

int
v_valid_host_ip_addr(char *input)
{
	int status = 1;
	char *ip_addr;
	char *ip_buf;
	int aa[4];
	struct in_addr ia;
	int net, local;
	u_long netmask, hostmask, addr;
	int nbytes;
	char *cp;
	int i;

	/*
	 * Get the field value and remove leading and trailing spaces.
	 */

	ip_buf = xstrdup(input);
	for (ip_addr = ip_buf; *ip_addr != NULL; ip_addr++) {
		if (!isspace((u_int) * ip_addr)) {
			break;
		}
	}
	for (cp = (char *) (ip_addr + strlen(ip_addr)); cp > ip_addr; cp--) {
		if (!isspace((u_int) * (char *) (cp - 1))) {
			break;
		}
	}
	*cp = NULL;

	/*
	 * Validate the IP address.
	 */

	if ((valid_ip_fmt(ip_addr, aa, &nbytes)) &&
	    (nbytes == strlen(ip_addr))) {
		for (i = 0; i < 4; i++) {
			if (aa[i] >= 255) {
				status = 0 /* SYSID_ERR_IPADDR_MAX */;
				free(ip_buf);
				return (status);
			}
			if (aa[i] < 0) {
				status = 0 /* SYSID_ERR_IPADDR_MIN */;
				free(ip_buf);
				return (status);
			}
		}
		ia.s_addr = inet_addr(ip_addr);
		net = inet_netof(ia);
		local = inet_lnaof(ia);

		/*
		 * The IN_CLASS* and related
		 * macros require host byte
		 * order.
		 */
		addr = ntohl(ia.s_addr);

		if (IN_CLASSA(addr)) {
			netmask = (u_long) IN_CLASSA_NET;
			hostmask = IN_CLASSA_HOST;
		} else if (IN_CLASSB(addr)) {
			netmask = (u_long) IN_CLASSB_NET;
			hostmask = IN_CLASSB_HOST;
		} else if (IN_CLASSC(addr)) {
			netmask = (u_long) IN_CLASSC_NET;
			hostmask = IN_CLASSC_HOST;
		} else if (IN_CLASSD(addr)) {
			netmask = (u_long) IN_CLASSD_NET;
			hostmask = IN_CLASSD_HOST;
		} else {
			netmask = (u_long) ~ 0;
			hostmask = (u_long) ~ 0;
		}
		if ((net > INADDR_ANY) &&
			(addr < (INADDR_BROADCAST & netmask)) &&
			(local > INADDR_ANY) &&
			(local < (INADDR_BROADCAST & hostmask))) {
			if (addr >= INADDR_UNSPEC_GROUP)
				status = 0 /* SYSID_ERR_IPADDR_UNSPEC */;
			/* else status = SYSID_SUCCESS */
		} else
			status = 0 /* SYSID_ERR_IPADDR_RANGE */;
	} else {
		status = 0 /* SYSID_ERR_IPADDR_FMT */;
	}

	free(ip_buf);
	return (status);
}

/*
 * copied from sysidtool's validation routines.
 *
 * v_valid_hostname:
 *
 * Validation routine for checking the validity of a hostname. Ensure that a
 * hostname is compliant with RFC 952+1123.  Summary: Hostname must be less
 * than MAXHOSTNAMELEN and greater than 1 char in length.  Must contain only
 * alphanumerics plus '-', and may not begin or end with '-'.
 *
 * This routine is based on the one found in the libadmutil library of the
 * administrative class hierarchy.
 *
 * "A hostname may only contain letters, digits, and minus signs (-)."
 * "A hostname may not begin or end with a minus sign (-)."
 */
int
v_valid_hostname(char *input)
{
	char *hostname;
	char *host_buf;
	char str[MAXHOSTNAMELEN];
	char *cp;
	int l;

	/*
	 * Get the field value and remove leading and trailing spaces.
	 */
	host_buf = xstrdup(input);
	for (hostname = host_buf; *hostname != NULL; hostname++) {
		if (!isspace((int) *hostname)) {
			break;
		}
	}
	for (cp = (char *) (hostname + strlen(hostname)); cp > hostname; cp--) {
		if (!isspace((int) *(char *) (cp - 1))) {
			break;
		}
	}
	*cp = NULL;

	/*
	 * Validate the hostname.
	 */

	if (((l = strlen(hostname)) >= sizeof (str)) || (l < 2)) {
		free(host_buf);
		return (0 /* SYSID_ERR_HOSTNAME_LEN */);
	}
	if ((sscanf(hostname, "%[0-9a-zA-Z-.]", str) != 1) ||
	    (strcmp(str, hostname) != 0)) {
		free(host_buf);
		return (0 /* SYSID_ERR_HOSTNAME_CHARS */);
	}
	if ((*hostname == '-') || (hostname[l - 1] == '-')) {
		free(host_buf);
		return (0 /* SYSID_ERR_HOSTNAME_MINUS */);
	}
	free(host_buf);
	return (1 /* SYSID_SUCCESS */);
}

/*
 * copied from disk library.
 *
 * a predicate function to determine if the input file system mount
 * point name is valid
 */
int
v_valid_filesys_name(char *fs)
{
	if (slice_name_ok(fs) == D_OK)
		return (1);
	else
		return (0);
}
