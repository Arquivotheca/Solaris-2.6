/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)valid.c	1.46	95/10/13 SMI"

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

#define IPADDR_CHARS	"0123456789."
#define	ETHERADDR_CHARS	"0123456789abcdefABCDEF:"

#define valid_ip_fmt(ia, a) \
((strspn(ia, IPADDR_CHARS) == strlen(ia)) && (ia[strlen(ia) - 1] != '.') && \
 (sscanf(ia, "%d.%d.%d.%d.%d", &a[0], &a[1], &a[2], &a[3], &a[4]) == 4))

#define	NETMASK_SIZE 32
#define MAXDESCRIPTIONLEN  256
#define MAXPRINTERNAMELEN 14


/*
**+++
**	valid_description
**	*****************
**
**	FUNCTIONAL DESCRIPTION:
**		This function verifies that a description that is input by the user is syntactically
**		valid.
**
**	FORMAL PARAMETERS:
**		description( const char *, read only)
**			Comment to validate. Valid syntax includes all but: &,: and "". Must start with
**			alpha char.
**---
*/
int valid_description(
const char *				description)
{
	char	str[MAXDESCRIPTIONLEN];

	memset(str,0,sizeof(str));

	if(strlen(description) == 0)
		return(1);

	if((strlen(description) < sizeof(str)) &&
	(sscanf(description,"%[^&:#=\"]",str)) &&
	 (!strcmp(str,description))) 
		return(1);
	else
		return(0);
		
}
int 
valid_host_ip_addr(const char *ip_addr)
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
 * If a valid IP address, normalize it.  In this case just make sure
 * there are no leading zeros.
 */
char *
normalize_ip(char *ip_addr) {
    int			 aa[5];
    unsigned long	 addr;
    char		*ret;
    char		 buf[4*3+3+1];

    if (valid_host_ip_addr(ip_addr)) {
	if (valid_ip_fmt(ip_addr, aa)) {
	    sprintf(buf, "%d.%d.%d.%d", aa[0], aa[1], aa[2], aa[3]);
	    ret = (char *)malloc(strlen(buf) + 1);
	    strcpy(ret, buf);
	    return (ret);
	}
    } else return (strdup(ip_addr));
}

int
valid_ip_netmask(char *netmask)
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

int
valid_ip_netnum(const char *netnum)
{
	int aa[5];
	struct in_addr ia;
	int local;
	int i;
	char buf[30];

	strcpy(buf, netnum);
	if (!valid_ip_fmt(buf, aa)) {
		for (i = 3; i > 0; --i) {
			strcat(buf, ".0");
			if (valid_ip_fmt(buf, aa))
				break;
		}
		if (i == 0)
			return (0);
	}
	for (i = 0; i < 4; i++)
		if (aa[i] >= 255)
			return (0);
	return (1);
}

int
valid_host_ether_addr(const char *eaddr)
{
	int i;
	int addr[7];
	int mask = 0xff;

	if ((strspn(eaddr, ETHERADDR_CHARS) != strlen(eaddr)) ||
	    (eaddr[strlen(eaddr) - 1] == ':') ||
	    (sscanf(eaddr, "%x:%x:%x:%x:%x:%x:%x", &addr[0], &addr[1], &addr[2],
	      &addr[3], &addr[4], &addr[5], &addr[6]) != 6))
		return (0);
	else {
		for (i = 0; i < 6; i++)
			if (addr[i] > 0xff)
				return (0);
			else
				mask &= addr[i];
		if (mask == 0xff)
			return (0);
	}

	return (1);
}

char *
normalize_ether(char *eaddr) {
	int addr[7];
	char *ret;

	if ((strspn(eaddr, ETHERADDR_CHARS) != strlen(eaddr)) ||
	    (eaddr[strlen(eaddr) - 1] == ':') ||
	    (sscanf(eaddr, "%x:%x:%x:%x:%x:%x:%x", &addr[0], &addr[1], &addr[2],
	      &addr[3], &addr[4], &addr[5], &addr[6]) != 6))
		return (strdup(eaddr));
	else {
		/*
		 * Allocate space for each 2 chars in each of the 6 octects
		 * including 5 colons and 1 NULL.
		 */
		ret = (char *)malloc( 6 * 2 + 5 + 1);
		sprintf(ret, "%x:%x:%x:%x:%x:%x",
			addr[0], addr[1], addr[2],
			addr[3], addr[4], addr[5]);
		return(ret);
	}
}
/*
 * valid_domainname - ensure that a domain name follows the recommended syntax from
 * 	RFC 1035, section 2.3.1.
 */

int
valid_domainname(const char *domain)
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
**+++
**	valid_printer_name
**	******************
**
**	FUNCTIONAL DESCRIPTION:
**		This function checks to be sure that a printer name that is input by the user
**		is syntactically correct. The correct syntax is as follows: A-Z,a-z,0-9,-,_. Should
**		start with an alpha char. No longer than 14 characters.
**
**	FORMAL PARAMETERS:
**		printername( const char *, read only )
**			Printername to validate.
**
**---
*/
int valid_printer_name(
const char *			printername)
{
	char 	str[MAXPRINTERNAMELEN +1];

	memset(str,0,sizeof(str));
	if((strlen(printername) <= MAXPRINTERNAMELEN)  &&
 	(sscanf(printername,"%[0-9a-zA-Z_-]",str) == 1) &&
	(!strcmp(str,printername)))
		return(1);
	else
		return(0);			        	
}
/*
**+++
**	valid_printerport
**
**	FUNCTIONAL DESCRIPTION:
**	  This function validates the printer port the user inputs if they use the "other"
**        command. This is done with the stat() command.
**
**	FORMAL PARAMETERS:
**		printerport( const char *, read only )
**			User input printer port path.
**
**---
*/
int valid_printerport(
const char *			printerport)
{
	int status = -1;
	if(printerport != NULL)
	{
		struct stat st;
	
		memset(&st,0,sizeof(struct stat));		
		status = stat(printerport,&st);
	}
	return(status);
}
/*
**+++
**
**	valid_printertype
**	*****************
**
**	FUNCTIONAL DESCRIPTION:
**		This function validates the syntax only of a user input printer type.
**		The acceptable syntax is: A-Z, a-z, 0-9, -, _ ,+, ' '. It must start with
**		an alpha character and cannot contain & or ;.
**
**	FORMAL PARAMETERS:
**		printertype( const char *, read only )
**			Printer type to validate.
**
**--
*/
int valid_printertype(
const char *			printertype)
{
	int status = -1;

	if (printertype != NULL)
	{
		char 		path[BUFSIZ];
		struct stat 	st;

		memset(path,0,sizeof(path));
		memset(&st,0,sizeof(struct stat));
		sprintf(path,"/usr/share/lib/terminfo/%c/%s",*printertype,printertype);
		status = stat(path,&st);
	}
	return(status);
	
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
valid_hostname(const char *hostname)
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
int
valid_timezone(const char *timezone)
{
	char path[MAXPATHLEN];
	struct stat stat_buf;

	sprintf(path, "%s/%s", TZDIR, timezone);
	if ((stat(path, &stat_buf) == 0) && ((stat_buf.st_mode & S_IFMT) == S_IFREG))
		return (1);
	else
		return (0);
}

int
valid_policy(const char *policy)
{
	return (1);
}

/*
 * valid_gname - ensure that a group name has valid syntax.  Keep consistent with
 * grpck(1).
 */
int
valid_gname(const char *gname)
{
	char *cp;
	
	for(cp = (char *)gname; *cp != '\0'; ++cp)
            if (!(islower(*cp) || isdigit(*cp)))
	        return (0);

	return (1);
}


/*
 * valid_uname - ensure that a user name is valid.  Rules should be consistent with pwck(1).
 */
int
valid_uname(const char *uname)
{
	int lc = 0;
	int i;

	if (!isalnum(*uname))
	    return (0);
	if (islower(*uname))
	    ++lc;

	if (strlen(uname) >= (size_t) MAXGLEN)
	    return (0);

	for (i = 1; uname[i] != '\0'; ++i) {
	        if (!isalnum(uname[i]) && uname[i] != '.' && uname[i] != '_' &&
		      uname[i] != '-')
		    return (0);
		
		if (islower(uname[i]))
		    ++lc;
        }

	if (lc == 0)
            return (0);
	else
	   return (1);
}

int
valid_autohome_name(const char *name)
{
	if (valid_uname(name))
		return (1);

	if (*name == '+' && valid_uname(name+1))
		return (1);

	return (0);
}

/*
 * valid_gid - ensure that a group id is valid
 */
int
valid_gid(const char *gid)
{
	long val;
	char *cp;
	
	while (*gid != '\0' && isspace((u_char)*gid))
		gid++;

	if (*gid == '\0') {
	    return (0);
	}

	if (*gid == '+') {
	    return (0);
	}

	val = strtol(gid, &cp, 10);
	if ((cp == &gid[strlen(gid)]) && (0 <= val) && (val <= MAXUID))
		return (1);
	else
		return (0);
}

/*
 * normalize_gid - normalize a group-ID.  The returned string is
 * stripped of all leading white space.  If the argument string
 * passes the checks implemented in valid_gid(), the returned
 * string will be stripped of all leading 0s.  The returned string
 * is malloc'ed; the caller is responsible for freeing this memory.
 */
char *
normalize_gid(const char *gid)
{
	long val;
	char buf[11];	/* 2**32 is 10 digits */
	char *ret;

	if (valid_gid(gid)) {
		/*
		 * The token is a valid long integer; return a copy
		 * of its ascii-based representation.  The strtol/printf
		 * combination produces a normalized representation.
		 */
		val = strtol(gid, NULL, 10);
		(void) sprintf(buf, "%ld", val);

		ret = strdup(buf);
	} else {
		/*
		 * The argument string was not a valid integer, contained
		 * no tokens, or contained more than one token.  Return a
		 * copy of it the string, stripped of leading white space.
		 * The caller is responsible for calliing valid_gid to catch
		 * any error.
		 */
		while (*gid != '\0' && isspace((u_char)*gid))
			gid++;
		ret = strdup(gid);
	}

	return (ret);
}

/*
 * _valid_pw_path - ensure that a path is OK for the passwd file.
 * This must be an absolute path, which means it starts with /.
 */
int
_valid_pw_path(const char *path)
{
	char *cp;

	cp = (char *)path;
	if (*cp != '/')
		return (0);
	for ( ; *cp != '\0'; ++cp)
		if (!isprint(*cp) || (*cp == ':'))
			return (0);
	return (1);
}

/*
 * valid_gcos - ensure that a gcos field is valid for the passwd file.
 */
int
valid_gcos(const char *gcos)
{

	if (strchr(gcos, ':') == NULL)
		return (1);
	else
		return (0);
}

/*
 * valid_int - ensure that a string parameter is a valid integer
 */
int
valid_int(const char *string)
{
	long val;
	char *cp;
	
	val = strtol(string, &cp, 10);
	if (cp == &string[strlen(string)])
		return (1);
	else
		return (0);
}

/*
 * valid_unsigned_int - ensure that a string parameter is a valid unsigned integer
 */
int
valid_unsigned_int(const char *string)
{
	boolean_t is_all_digits();
	
	if (is_all_digits(string) == B_TRUE )
		return (1);
	else
		return (0);

}

static
boolean_t
is_all_digits(const char *group)
{

	int	i;
	int	len;
	int	retval;


	len = strlen(group);

	for (i = 0; i < len; i++) {
		if (isdigit(group[i]) == 0) {
			break;
		}
	}

	if (i == len) {
		retval = B_TRUE;
	} else {
		retval = B_FALSE;
	}

	return (retval);
}

/*
 * valid_proto_name - ensure that a protocol name is valid for the protocols file.
 */
int
valid_proto_name(const char *name)
{

	char *cp;
	
	for (cp = (char *)name; *cp != '\0'; ++cp)
		if (!isprint(*cp) || isspace(*cp) || (*cp == '#'))
			return (0);
	return (1);
}

/*
 * valid_uid - ensure that a uid is valid.
 */
int
valid_uid(const char *uid)
{
	return (valid_gid(uid));
}

/*
 * normalize_uid - normalize a user-ID
 */
char *
normalize_uid(const char *uid)
{
	return (normalize_gid(uid));
}

int
valid_shell(const char *shell)
{
	return (_valid_pw_path(shell));
}

/*
 * valid_proto_num - ensure that a protocol number is valid
 */
int
valid_proto_num(const char *pnum)
{
	return (valid_int(pnum));
}

/*
 * valid_rpc_num - ensure that an rpc number is valid.
 */
int
valid_rpc_num(const char *rnum)
{
	return (valid_int(rnum));
}

/*
 * valid_rpc_name - ensure that an rpc program name is valid.
 */
int
valid_rpc_name(const char *rpcname)
{
	return (valid_proto_name(rpcname));
}

/*
 * valid_port_num - ensure that a service port number is valid.
 */
int
valid_port_num(const char *pnum)
{
	return (valid_int(pnum));
}

/*
 * valid_service_name - ensure that a service name is valid.
 */
int
valid_service_name(const char *servname)
{
	return (valid_proto_name(servname));
}

/*
 * valid_home_path - ensure that a home directory pathname is valid.
 */
int
valid_home_path(const char *path)
{
	return (_valid_pw_path(path));
}

/*
 * valid_path - ensure that a pathname is valid.
 */
int
valid_path(const char *path)
{
	return (_valid_pw_path(path));
}

/*
 * valid_auto_home_path - ensure that a pathname for auto_home is valid.
 */
int
valid_auto_home_path(const char *path)
{
	char *cp, *pp;
	
	pp = strdup(path);
	cp = strtok(pp, ":");
	if ((cp == NULL) || !valid_hostname(cp)) {
		free(pp);
		return (-1);
	}
	cp = strtok(NULL, ":");
	if ((cp == NULL) || !_valid_pw_path(cp)) {
		free(pp);
		return (-1);
	}
	free(pp);
	return (0);
}

/*
 * valid_bootparams_key - ensure that the key value for bootparams is valid.
 */
int
valid_bootparams_key(const char *key)
{
	if (!strcmp(key, "*"))
		return (1);
	else
		return (valid_hostname(key));
}

/*
 * valid_group_members - ensure that a members list for the group table is
 * valid and condensed.
 */
int
valid_group_members(const char *members)
{
  char *cp, *mlist;

    mlist = (char *)malloc (strlen(members)+1);
    if (mlist)
        strcpy(mlist, members);
    else
        return(0);

    for (cp = strtok(mlist, ","); cp != NULL; cp = strtok(NULL, ","))
        if ((valid_uname(cp)) == 0) {
	    free(mlist);
            return (0);
	}
	    
    free(mlist);
    return (1);
}

/*
* 	valid_printer_allow_list
*	
*	FUNCTIONAL DESCRIPTION:
*		This function verifies the user allow list for printmgr.
*
*	FORMAL PARAMETERS:
*		users( const char *, read only)
*			List of user names that input.
*
*/

int
valid_printer_allow_list(
const char *			users)
{
	int 	status = 0;

	status = valid_group_members(users);
	return(status);
}	

/*
 * valid_mail_alias - ensure that a mail alias is valid.
 */
int
valid_mail_alias(const char *alias)
{
	if ((strchr(alias, '#') != NULL) || (strchr(alias, ':') != NULL))
		return (0);
	else
		return (1);
}

/*
 * valid_netname - ensure that a network name is valid.
 */
int
valid_netname(const char *net)
{
	return (valid_hostname(net));
}

/*
 * valid_passwd - ensure that a password is valid.
 */
int
valid_passwd(const char *pw)
{
	return ((strchr(pw, ':') == NULL) ? 1 : 0);
}

/*
 * valid_shell - ensure that a shell name is valid.
 */
#ifdef TESTING
main() {
    char **c;
    char *ip[] = {  "1.2.3.4",
		    "01.02.03.04",
		    "001.002.003.004",
		    " 1.2.3.4  ",
		    "02.03.04",
		    "1.2.3.4.5",
		    "0x1.0x2.0x3.0x4",
		    "0x1.2.3.4",
		    "1.0x2.3.4",
		    "1.2.0x3.4",
		    "1.2.3.0x4",
		    "1.2.3.-4",
		    "1.2.-3.4",
		    "-1.2.3.4",
		    "255.255.255.255",
		    "1.2.3.255",
		    "1.2.255.4",
		    "1.255.3.4",
		    "255.2.3.4",
		    "1.2.3.256",
		    "1.2.256.4",
		    "1.256.3.4",
		    "256.2.3.4",
		    NULL
		 };
    c = ip;
    while(*c) {
	printf("Checking %20s -- ", *c); fflush(stdout);
	printf("%d -- ", valid_host_ip_addr(*c));
	printf("%s\n", normalize_ip(*c));
	c++;
    }
}
#endif
