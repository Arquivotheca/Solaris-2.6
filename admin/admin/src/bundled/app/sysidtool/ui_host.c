/*
 * Copyright (c) 1991,1992,1993 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
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

#pragma	ident	"@(#)ui_host.c 1.6 95/10/06"

/*
 *	Name:		host.c
 *
 *	Description:	This file contains the generic user-interface
 *			routines needed to get the host name and host
 *			IP address.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <malloc.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sysid_ui.h"
#include "sysid_msgs.h"

/*
 * ui_valid_host_ip_addr:
 *
 *	validation routine for checking the validity of a host
 *	IP address.  This routine is based on the one found in the
 *	libadmutil library of the administrative class hierarchy.
 *
 * modified to conform to RFC1166.  Allow 255 in network number,
 * no 255 in host number.
 */

#define	valid_ip_fmt(ia, a, ap) (sscanf(ia, "%d.%d.%d.%d%n", &a[0], &a[1], \
				&a[2], &a[3], ap) == 4)

int
ui_valid_host_ip_addr(Field_desc *f)
{
	Sysid_err	status = SYSID_SUCCESS;
	char		*input = (char *)f->value;
	char		*ip_addr;
	char		*ip_buf;
	int		aa[4];
	struct in_addr	ia;
	int		net, local;
	u_long		netmask, hostmask, addr;
	int		nbytes;
	char		*cp;
	int		i;

	/*
	 * Get the field value and remove leading and trailing spaces.
	 */

	ip_buf = xstrdup(input);
	for (ip_addr = ip_buf; *ip_addr != NULL; ip_addr++) {
		if (!isspace((u_int) *ip_addr)) {
			break;
		}
	}
	for (cp = (char *) (ip_addr + strlen(ip_addr)); cp > ip_addr; cp--) {
		if (!isspace((u_int) *(char *)(cp - 1))) {
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
			if (aa[i] > 255) {
				status = SYSID_ERR_IPADDR_MAX;
				free(ip_buf);
				return (status);
			}
			if (aa[i] < 0) {
				status = SYSID_ERR_IPADDR_MIN;
				free(ip_buf);
				return (status);
			}
		}
		if (aa[0] <= 126 && aa[1] == 255 && aa[2] == 255 &&
		    aa[3] == 255) {
			free(ip_buf);
			return (SYSID_ERR_IPADDR_MAX);
		}
		if (aa[0] > 126 && aa[0] <= 191 && aa[2] == 255 &&
		    aa[3] == 255) {
			free(ip_buf);
			return (SYSID_ERR_IPADDR_MAX);
		}
		if (aa[0] > 191 && aa[3] == 255) {
			free(ip_buf);
			return (SYSID_ERR_IPADDR_MAX);
		}
		ia.s_addr = inet_addr(ip_addr);	/* network byte order */
		net = inet_netof(ia);		/* host byte order */
		local = inet_lnaof(ia);		/* host byte order */

		/*
		 * The IN_CLASS* and related
		 * macros require host byte
		 * order.
		 */
		addr = ntohl(ia.s_addr);

		if (IN_CLASSA(addr)) {
			netmask = (u_long)IN_CLASSA_NET;
			hostmask = IN_CLASSA_HOST;
		} else if (IN_CLASSB(addr)) {
			netmask = (u_long)IN_CLASSB_NET;
			hostmask = IN_CLASSB_HOST;
		} else if (IN_CLASSC(addr)) {
			netmask = (u_long)IN_CLASSC_NET;
			hostmask = IN_CLASSC_HOST;
		} else if (IN_CLASSD(addr)) {
			netmask = (u_long)IN_CLASSD_NET;
			hostmask = IN_CLASSD_HOST;
		} else {
			netmask = (u_long)~0;
			hostmask = (u_long)~0;
		}
		if ((net > INADDR_ANY) &&
			(addr < (INADDR_BROADCAST & netmask)) &&
		    (local > INADDR_ANY) &&
			(local < (INADDR_BROADCAST & hostmask))) {
				if (addr >= INADDR_UNSPEC_GROUP)
					status = SYSID_ERR_IPADDR_UNSPEC;
				/* else status = SYSID_SUCCESS */
		} else
			status = SYSID_ERR_IPADDR_RANGE;
	} else {
		status = SYSID_ERR_IPADDR_FMT;
	}

	free(ip_buf);
	return (status);
}


/*
 * ui_valid_hostname:
 *
 *	Validation routine for checking the validity of a hostname.
 *	Ensure that a hostname is compliant with RFC 952+1123.  Summary:
 *	Hostname must be less than MAXHOSTNAMELEN and greater than 1 char
 *	in length.  Must contain only alphanumerics plus '-', and may not
 *	begin or end with '-'.
 *
 *	This routine is based on the one found in the libadmutil library
 *	of the administrative class hierarchy.
 */

int
ui_valid_hostname(Field_desc *f)
{
	char	*input = (char *)f->value;
	char	*hostname;
	char	*host_buf;
	char	str[MAXHOSTNAMELEN];
	char	*cp;
	int	l;

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
		if (!isspace((int) *(char *)(cp - 1))) {
			break;
		}
	}
	*cp = NULL;

	/*
	 * Validate the hostname.
	 */

	if (((l = strlen(hostname)) >= sizeof (str)) || (l < 2)) {
		free(host_buf);
		return (SYSID_ERR_HOSTNAME_LEN);
	}
	if ((sscanf(hostname, "%[0-9a-zA-Z-.]", str) != 1) ||
	    (strcmp(str, hostname) != 0)) {
		free(host_buf);
		return (SYSID_ERR_HOSTNAME_CHARS);
	}
	if ((*hostname == '-') || (hostname[l-1] == '-')) {
		free(host_buf);
		return (SYSID_ERR_HOSTNAME_MINUS);
	}

	free(host_buf);
	return (SYSID_SUCCESS);
}

static	Field_desc	host_info[] = {
	{ FIELD_TEXT, (void *)ATTR_HOSTIP, NULL, NULL, NULL,
		15, MAX_IPADDR, -1, -1,
		FF_LAB_ALIGN | FF_LAB_RJUST | FF_KEYFOCUS,
		ui_valid_host_ip_addr },
	{ FIELD_TEXT, (void *)ATTR_HOSTNAME, NULL, NULL, NULL,
		32, MAX_HOSTNAME, -1, -1,
		FF_LAB_ALIGN | FF_LAB_RJUST | FF_KEYFOCUS,
		ui_valid_hostname },
};


/*
 * ui_get_hostIP:
 *
 * 	This routine is the client interface routine used for
 *	retrieving the host IP address from the user.
 *
 *	Input:  pointer to character buffer in which to place
 *		the IP address.
 */

void
ui_get_hostIP(MSG *mp, int reply_to)
{
	static char ip_address[MAX_IPADDR+1];
	static Field_help help;

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)ip_address, sizeof (ip_address));
	msg_delete(mp);

	host_info[0].help = dl_get_attr_help(ATTR_HOSTIP, &help);
	host_info[0].label = dl_get_attr_name(ATTR_HOSTIP);
	host_info[0].value = ip_address;

	dl_do_form(
		dl_get_attr_title(ATTR_HOSTIP),
		dl_get_attr_text(ATTR_HOSTIP),
		&host_info[0], 1, reply_to);
}


/*
 * ui_get_hostname:
 *
 * 	This routine is the client interface routine used for
 *	retrieving the host name from the user.
 *
 *	Input:  pointers to character buffer in which to place
 *		the host name.
 */

void
ui_get_hostname(MSG *mp, int reply_to)
{
	static char hostname[MAX_HOSTNAME+1];
	static Field_help help;

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)hostname, sizeof (hostname));
	msg_delete(mp);

	host_info[1].help = dl_get_attr_help(ATTR_HOSTNAME, &help);
	host_info[1].label = dl_get_attr_name(ATTR_HOSTNAME);
	host_info[1].value = hostname;

	dl_do_form(
		dl_get_attr_title(ATTR_HOSTNAME),
		dl_get_attr_text(ATTR_HOSTNAME),
		&host_info[1], 1, reply_to);
}
