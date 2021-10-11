/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vgroup.c	1.4	96/09/06 SMI"       /* SVr4.0 1.2 */

#include	<sys/types.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<grp.h>
#include	<users.h>

extern int valid_gname(), valid_gid();
extern long strtol();

/*
	validate a group name or number and return the appropriate
	group structure for it.
*/
int
valid_group( group, gptr )
char *group;
struct group **gptr;
{
	register gid_t gid;
	char *ptr;


	if( isdigit(*group) ) {

		gid = (gid_t) strtol(group, &ptr, (int) 10);
		if (! *ptr)
		return( valid_gid( gid, gptr ) );
	}
	for (ptr = group; *ptr != NULL; ptr++) {
		if (!isprint(*ptr) || (*ptr == ':'))
			return (INVALID);
	}
	return (valid_gname(group,gptr));
}
