#ifndef lint
#pragma ident "@(#)common_boolean.c 1.3 96/04/23 SMI"
#endif

/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	common_boolean.c
 * Group:	libspmicommon
 * Description:	This module contains common utilities used by
 *		all spmi applications to determine if a given
 *		object is in an expected configuration.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/openpromio.h>
#include <sys/param.h>
#include <sys/types.h>

#include "spmicommon_lib.h"

/* constants and macros */

#define MAXPROPSIZE     128
#define MAXVALSIZE      (4096 - MAXPROPSIZE - sizeof (u_int))
#define PROPBUFSIZE     (MAXPROPSIZE + MAXVALSIZE + sizeof (u_int))

/* data structures */

typedef union {
        char	buf[PROPBUFSIZE];
        struct openpromio       opp;
} OpenProm;

typedef struct {
        char    device[MAXNAMELEN];
        char    targets[32];
} PromParam;

/* public prototypes */

int		IsIsa(char *);
int		is_allnums(char *);
int		is_disk_name(char *);
int		is_hex_numeric(char *);
int		is_hostname(char *);
int		is_ipaddr(char *);
int		is_numeric(char *);
int		_is_openprom(int);
int		is_slice_name(char *);
int		is_part_name(char *);

/*---------------------- public functions -----------------------*/

/*
 * Function:	is_allnums
 * Description:	Check if a string syntactically represents a decimal
 *		number sequence.
 * Scope:	public
 * Parameters:	str     - [RO]
 *			  string to be validated
 * Return:	0       - invalid numeric sequence
 *      	1	- valid numeric sequence
 */
int
is_allnums(char *str)
{
	if (str == NULL || *str == '\0')
		return (0);

	if (str && *str) {
		for (; *str; str++) {
			if (isdigit(*str) == 0)
				return (0);
		}
	}

	return (1);
}

/*
 * Function:	is_disk_name
 * Description:	Check if a string syntactically represents a cannonical
 *		disk name (e.g. c0t0d0).
 * Scope:	public
 * Parameters:	str     - [RO]
 *			  string to be validated
 * Return:	0       - invalid disk name syntax
 *		1       - valid disk name syntax
 */
int
is_disk_name(char *str)
{
	if (str) {
		must_be(str, 'c');
		skip_digits(str);
		if (*str == 't') {
			str++;
			skip_digits(str);
		}

		must_be(str, 'd');
		skip_digits(str);
	}

	if (str != NULL && *str == '\0')
		return (1);

	return (0);
}

/*
 * Function:	is_hex_numeric
 * Description:	Check if a string syntactically represents a hexidecimal
 *		number sequence.
 * Scope:	public
 * Parameters:	str     - [RO]
 *			  string to be validated
 * Return:	0       - invalid hexidecimal digit sequence
 *		1       - valid hexidecimal digit sequence
 */
int
is_hex_numeric(char *str)
{
	if (str == NULL || *str == '\0')
		return (0);

	if (strlen(str) > 2U && *str++ == '0' && strchr("Xx", *str)) {
		for (++str; *str; str++) {
			if (!isxdigit(*str))
				return (0);
		}

		return (1);
	}

	return (0);
}

/*
 * Function:	is_hostname
 * Description:	Check if a string syntactically represents a host name
 *		conforming to the RFC 952/1123 specification.
 * Scope:	public
 * Parameters:	str	- [RO]
 *			  string to be validated
 * Return:	0       - invalid host name syntax
 *		1       - valid host name syntax
 */
int
is_hostname(char *str)
{
	char	*seg;
	char	*cp;
	int	length;
	char	buf[MAXNAMELEN] = "";

	/* validate parameter */
	if (str == NULL)
		return (0);

	(void) strcpy(buf, str);
	if ((seg = strchr(buf, '.')) != NULL) {
		*seg++ = '\0';
		/* recurse with next segment */
		if (is_hostname(seg) == 0)
			return (0);
	}

	/*
	 * length must be 2 to 63 characters (255 desireable, but not
	 * required by RFC 1123)
	 */
	length = (int) strlen(buf);
	if (length < 2 || length > 63)
		return (0);

	/* first character must be alphabetic or numeric */
	if (isalnum((int) buf[0]) == 0)
			return (0);

	/* last character must be alphabetic or numeric */
	if (isalnum((int) buf[length - 1]) == 0)
		return (0);

	/* names must be comprised of alphnumeric or '-' */
	for (cp = buf; *cp; cp++) {
		if (isalnum((int)*cp) == 0 && *cp != '-')
			return (0);
	}

	return (1);
}

/*
 * Function:	is_ipaddr
 * Description:	Check if a string syntactically represents an Internet
 *		address.
 * Scope:	public
 * Parameters:	str	- [RO]
 *			  string containing textual form of an Internet
 *			  address
 * Return:	0       - invalid address syntax
 *		1       - valid address syntax
 */
int
is_ipaddr(char *str)
{
	int	num;
	char	*p;

	if ((p = strchr(str, '.')) == NULL)
		return (0);
	*p = '\0';
	num = atoi(str);
	if (num < 0 || num > 255 || is_allnums(str) == 0)
		return (0);
	*p = '.';
	str = p + 1;
	if ((p = strchr(str, '.')) == NULL)
		return (0);

	*p = '\0';
	num = atoi(str);
	if (num < 0 || num > 255 || is_allnums(str) == 0)
		return (0);

	*p = '.';
	str = p + 1;

	if ((p = strchr(str, '.')) == NULL)
		return (0);
	*p = '\0';
	num = atoi(str);
	if (num < 0 || num > 255 || is_allnums(str) == 0)
		return (0);
	*p = '.';
	str = p + 1;
	num = atoi(str);
	if (num < 0 || num > 255 || is_allnums(str) == 0)
		return (0);

	return (1);
}

/*
 * Function:	is_numeric
 * Description:	Check a character string and ensure that it represents
 *		either a hexidecimal or decimal number.
 * Scope:	public
 * Parameters:	str	- [RO]
 *			  string to be validated
 * Return:	0       - invalid hex/dec sequence
 *		1       - valid hex/dec sequence
 */
int
is_numeric(char *str)
{
	if (str && *str) {
		if (strlen(str) > 2U &&
				str[0] == '0' && strchr("Xx", str[1])) {
			str += 2;
			while (*str) {
				if (!isxdigit(*str))
					return (0);
				else
					str++;
			}
			return (1);
		} else {
			while (*str) {
				if (!isdigit(*str))
					return (0);
				else
					str++;
			}
			return (1);
		}
	}
	return (0);
}

/*
 * Function:	_is_openprom
 * Description:	Boolean test to see if a device is an openprom device.
 *		The test is based on whether the OPROMGETCONS ioctl()
 *		works, and if the OPROMCONS_OPENPROM bits are set.
 * Scope:	public
 * Parameters:	fd	- [RO]
 *			  open file descriptor for openprom device
 * Return:	0	- the device is not an openprom device
 *		1	- the device is an openprom device
 */
int
_is_openprom(int fd)
{
	OpenProm	  pbuf;
	struct openpromio *opp = &(pbuf.opp);
	u_char		  mask;

	opp->oprom_size = MAXVALSIZE;

	if (ioctl(fd, OPROMGETCONS, opp) == 0) {
		mask = (u_char)opp->oprom_array[0];
		if ((mask & OPROMCONS_OPENPROM) == OPROMCONS_OPENPROM)
			return (1);
	}

	return (0);
}

/*
 * Function:	is_slice_name
 * Description:	Check to see a string syntactically represents a
 *		cannonical slice device name (e.g. c0t0d0s3).
 * Scope:	public
 * Parameters:	str     - [RO]
 *			  string to be validated
 * Return:	0       - invalid slice name syntax
 *		1       - valid slice name syntax
 */
int
is_slice_name(char *str)
{
	if (str) {
		must_be(str, 'c');
		skip_digits(str);
		if (*str == 't') {
			str++;
			skip_digits(str);
		}
		must_be(str, 'd');
		skip_digits(str);
		must_be(str, 's');
		skip_digits(str);
	}

	if (str != NULL && *str == '\0')
		return (1);

	return (0);
}

/*
 * Function:	is_part_name
 * Description:	Check to see a string syntactically represents a
 *		cannonical fdisk partition device name (e.g. c0t0d0p2).
 * Scope:	public
 * Parameters:	str     - [RO]
 *			  string to be validated
 * Return:	0       - invalid partition name syntax
 *		1       - valid partition name syntax
 */
int
is_part_name(char *str)
{
	if (str) {
		must_be(str, 'c');
		skip_digits(str);
		if (*str == 't') {
			str++;
			skip_digits(str);
		}
		must_be(str, 'd');
		skip_digits(str);
		must_be(str, 'p');
		skip_digits(str);
	}

	if (str != NULL && *str == '\0')
		return (1);

	return (0);
}

/*
 * Function:	IsIsa
 * Description:	Boolean function indicating whether the instruction set
 *		architecture of the executing system matches the name provided.
 *		The string must match a system defined architecture (e.g.
 *		"i386", "ppc, "sparc") and is case sensitive.
 * Scope:	public
 * Parameters:	name	- [RO, *RO]
 *		string representing the name of instruction set architecture
 *		being tested
 * Return:	0 - the system instruction set architecture is different from
 *			the one specified
 * 		1 - the system instruction set architecture is the same as the
 *			one specified
 */
int
IsIsa(char *name)
{
	return (streq(get_default_inst(), name) ? 1 : 0);
}
