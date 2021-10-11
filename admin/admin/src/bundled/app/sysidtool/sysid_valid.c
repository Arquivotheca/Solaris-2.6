/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.  All rights reserved.
 */

#pragma ident   "@(#)sysid_valid.c 1.3 96/06/14"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <arpa/nameser.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <userdefs.h>
#include <tzfile.h>
#include "sysidtool.h"

#define IPADDR_CHARS	"0123456789."

#define TERMINFO_DIR "/usr/share/lib/terminfo"
#define NLSPATH "/usr/lib/locale"

#define valid_ip_fmt(ia, a) \
((strspn(ia, IPADDR_CHARS) == strlen(ia)) && (ia[strlen(ia) - 1] != '.') && \
 (sscanf(ia, "%d.%d.%d.%d.%d", &a[0], &a[1], &a[2], &a[3], &a[4]) == 4))

#define	NETMASK_SIZE 32

/*
 * Function:	sysid_valid_host_ip_addr
 * Description:	Validate an ip address verifying that it is
 *		a member of either a class A, B, or C network.
 * Scope:	Public
 * Parameters:	ip_addr [RO] (char *)
 *		A pointer to the character string containing the 
 *		ip address.
 * Return:	1 - if valid ip address
 *		0 - otherwise
 */
int 
sysid_valid_host_ip_addr(const char *ip_addr)
{
    int aa[5];
    unsigned long addr, host;

    if (valid_ip_fmt(ip_addr, aa)) {
	if ((aa[0] > 255) || (aa[1] > 255) || (aa[2] > 255) || (aa[3] > 255))
	    return (0);
	addr = (aa[0] << 24) | (aa[1] << 16) | (aa[2] << 8) | aa[3] ;
	if (IN_CLASSA(addr) && ((host = IN_CLASSA_HOST & addr) != 0) &&
		(host != IN_CLASSA_HOST))
		    return (1);
	else if (IN_CLASSB(addr) && 
		((host = IN_CLASSB_HOST & addr) != 0) &&
		(host != IN_CLASSB_HOST))
		    return (1);
	else if (IN_CLASSC(addr) &&
		((host = IN_CLASSC_HOST & addr) != 0) &&
		(host != IN_CLASSC_HOST))
		    return (1);
    }
    return (0);
}

/*
 * Function:	sysid_valid_host_ip_netmask
 * Description:	Validate a netmask.
 * Scope:	Public
 * Parameters:	netmask [RO] (char *)
 *		A pointer to the character string containing the 
 *		netmask.
 * Return:	1 - if valid netmask
 *		0 - otherwise
 */
int
sysid_valid_ip_netmask(const char *netmask)
{
	int aa[5];
	int i;
	unsigned n, msb_mask, msb;

	if (!valid_ip_fmt(netmask, aa))
		return (0);

	for (i = 0; i < 4; i++)
		if (aa[i] > 255)
			return (0);

	if (aa[3] == 255)
		return (0);

	/* check for contiguous bits */
	n = aa[0];
	for (i = 1; i < 4; i++)
		n = (n << 8) | aa[i];

	msb_mask = 1;
	msb_mask <<= (NETMASK_SIZE - 1);
	for (; n != 0; n <<= 1) {
		msb = n & msb_mask;
		if (msb == 0)
			if (n != 0) 
				return (0);
			else
				break;
        }

	return (1);
}


/*
 * Function:	sysid_valid_domainname
 * Description:	Verify that the input domain name adheres to
 *		the recommended syntax from RFC 1035, Section 2.3.1
 * Scope:	Public
 * Parameters:	domain [RO] (char *)
 *		A pointer to the character string containing the 
 *		domain name.
 * Return:	1 - if valid domain name
 *		0 - otherwise
 */
int
sysid_valid_domainname(const char *domain)
{
	char str[MAXDNAME];
	char *cp;
	int l;

	if (((l = strlen(domain)) < sizeof(str)) && (l > 0) &&
	    (sscanf(domain, "%[0-9a-zA-Z._-]", str) == 1) &&
	    (*domain != '-') && (domain[l - 1] != '-') &&
	    (strcmp(str, domain) == 0)) {
		for (cp = strtok(str, "."); cp != NULL; cp = strtok(NULL, "."))
			if (strlen(cp) > (size_t)MAXLABEL)
				return (0);
		return (1);
	} else
		return (0);
}

/*
 * valid_hostname -
 *    RFC 952 which states:
 *
 *    A host drawn from the alphabet (A-Z), digits (0-9), minus sign (-),  
 *    and  period  (.).  Additionally we allow underscore (_).  Note  that
 *    periods  are  only  allowed  when they serve to delimit components 
 *    of "domain style names". (See RFC 921, "Domain  Name System  
 *    Implementation  Schedule," for background). No blank or space characters
 *    are permitted as part of a name. The first character must be an alpha  
 *    character.  The  last  character must not be a minus sign, period or
 *    underscore.
 *	
 *    Ensure that a hostname is compliant with RFC 952+1123.  Summary:
 *    Hostname must be less than MAXHOSTNAMELEN and greater than
 *    1 char in length.  
 *
 *    Notes:
 *    RFC 952 has been modified by RFC 1123 to relax the restriction on the 
 *    first character being a digit.  However NISPLUS, secure RPC and 
 *    gethostbyname do not conform to this standard. Therefore we disallow
 *    hostnames that start with a decimal character.  
 *
 *    Please refer to bug #1209402 that reports this nonconformance.
 *
 */

int
sysid_valid_hostname(const char *hostname)
{	
	char str[MAXHOSTNAMELEN];
	int l;
	char *cp;
	
	if (((l = strlen(hostname)) < sizeof(str)) && (l > 1) &&
	    (sscanf(hostname, "%[0-9a-zA-Z._-]", str) == 1) &&
	    (strcmp(str, hostname) == 0) && 
	    (*hostname != '-') && (hostname[l - 1] != '-') &&
	    (*hostname != '_') && (hostname[l - 1] != '_') &&
	    (*hostname != '.') && (hostname[l - 1] != '.') &&
	    (!(isdigit(hostname[0])))) {
		for (cp = strtok(str, "."); cp != NULL; cp = strtok(NULL, "."))
			if (strlen(cp) > (size_t)MAXLABEL)
				return (0);
		return (1);
	} else
		return (0);
}

/*
 * valid_timezone - ensure that we have clock adjustment rules for the requested
 * timezone.
 */
/*
 * Function:	sysid_valid_timezone
 * Description:	Verify that the specified timezone exists on
 *		the currently running system in the directory
 *		/usr/share/lib/zoneinfo.
 * Scope:	Public
 * Parameters:	timezone [RO] (char *)
 *		A pointer to the character string containing the 
 *		timezone.
 * Return:	1 - if valid timezone
 *		0 - otherwise
 */
int
sysid_valid_timezone(const char *timezone)
{
	char path[MAXPATHLEN];
	struct stat stat_buf;

	sprintf(path, "%s/%s", TZDIR, timezone);
	if ((stat(path, &stat_buf) == 0) && ((stat_buf.st_mode & S_IFMT) == S_IFREG))
		return (1);
	else
		return (0);
}


/*
 * Function:	sysid_valid_system_locale
 * Description:	Verify that the specified locale exists on the
 *		currently running system in /usr/lib/locale as
 *		either a full or partial locale.
 * Scope:	Public
 * Parameters:	locale [RO] (char *)
 *		A pointer to the character string containing the 
 *		locale.
 * Return:	1 - if valid locale
 *		0 - otherwise
 */
int
sysid_valid_system_locale(const char *locale)
{
	/* This is the only way that we can check for post install */
	/* locale values since it will be dependent on the choices */
	/* made at install time                                    */
	return(sysid_valid_install_locale(locale));
}

/*
 * Function:	sysid_valid_install_locale
 * Description:	Verify that the specified locale exists on the
 *		currently running system in /usr/lib/locale as
 *		either a full or partial locale.
 * Scope:	Public
 * Parameters:	locale [RO] (char *)
 *		A pointer to the character string containing the 
 *		locale.
 * Return:	1 - if valid locale
 *		0 - otherwise
 */
int
sysid_valid_install_locale(const char *locale)
{
	char path[MAXPATHLEN];
	struct stat stat_buf;


	/* Check to see if locale is a full locale by checking */
	/* to see if the sysidtool message file exists         */
	sprintf(path, "%s/%s/LC_MESSAGES/%s.mo", 
		NLSPATH, locale, TEXT_DOMAIN);
	if ((stat(path, &stat_buf) == 0) && 
			((stat_buf.st_mode & S_IFMT) == S_IFREG))
		return(1);

	/* Check to see of locale is a partial locale by checking */
	/* for the existence of the locale description file       */
	sprintf(path, "%s/%s/locale_description", NLSPATH, locale);
	if ((stat(path, &stat_buf) == 0) && 
			((stat_buf.st_mode & S_IFMT) == S_IFREG))
		return(1);
	else
		return(0);
}

/*
 * Function:	sysid_valid_network_interface
 * Description:	Verify that the specified network interface
 *		exists on the currently running system by 
 *		getting a list of plumbed interfaces and 
 *		comparing against it.
 * Scope:	Public
 * Parameters:	network_interface [RO] (char *)
 *		A pointer to the character string containing the 
 *		network interface.
 * Return:	1 - if valid network interface
 *		0 - otherwise
 */
int
sysid_valid_network_interface(const char *network_interface)
{
	int i;
	int num_ifs;
	struct if_list *net_ifs;

	/*
	 * get the list of network interfaces
	 */
	num_ifs = get_net_if_list(&net_ifs);
	for (i = 0; net_ifs != NULL && i < num_ifs; 
			net_ifs = net_ifs->next, i++) {
		if (strcmp(net_ifs->name, network_interface) == 0)
			return(1);
	}

	return(0);
}

/*
 * Function:	sysid_valid_terminal
 * Description:	Verify that the specified terminal type exists 
 *		in the 	currently running system's terminfo database
 * Scope:	Public
 * Parameters:	terminal [RO] (char *)
 *		A pointer to the character string containing the 
 *		terminal type.
 * Return:	1 - if valid terminal type
 *		0 - otherwise
 */
int
sysid_valid_terminal(const char *terminal)
{
	char path[MAXPATHLEN];
	struct stat stat_buf;


	sprintf(path, "%s/%c/%s", TERMINFO_DIR, *terminal, terminal);
	if ((stat(path, &stat_buf) == 0) && ((stat_buf.st_mode & S_IFMT) == S_IFREG))
		return (1);
	else
		return (0);
}

/*
 * valid_passwd - ensure that a password is valid.
 */
int
sysid_valid_passwd(const char *pw)
{
	return ((strchr(pw, ':') == NULL) ? 1 : 0);
}
