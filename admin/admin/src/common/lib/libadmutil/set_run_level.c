/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)set_run_level.c	1.1	94/08/31 SMI"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <libintl.h>
#include "admutil.h"

set_run_level(char *run_level)
{
	char command[20];
	int cond;
	
	strcpy(command,"/sbin/init ");
	strcat(command,run_level);
	cond=system(command);
	return (0);
}
