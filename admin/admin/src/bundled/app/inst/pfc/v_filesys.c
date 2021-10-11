#ifndef lint
#pragma ident "@(#)v_filesys.c 1.10 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1992-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_filesys.c
 * Group:	ttinstall
 * Description:
 */

#include <locale.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <libintl.h>

#include "spmistore_api.h"
#include "v_types.h"
#include "v_filesys.h"

/*
 * exported functions
 */

int             any_filesys(void);

/*
 * local functions
 */

int 
any_filesys(void)
{
    int             i;

    for (i = 0; i < N_LOCAL_FS; i++)
	if (mnt_pnt_size(lfs[i]))
	    return (1);

    return (0);
}
