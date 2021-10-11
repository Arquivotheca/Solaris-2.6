#ifndef lint
#pragma ident "@(#)v_upgrade.c 1.53 96/07/09 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_upgrade.c
 * Group:	ttinstall
 * Description:
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <libintl.h>

#include "pf.h"
#include "inst_msgs.h"
#include "v_types.h"
#include "v_misc.h"
#include "v_sw.h"
#include "v_lfs.h"
#include "v_disk.h"
#include "v_upgrade.h"

/* This file contains the View interface layer to the upgrade functions. */

/*
 * state variable for upgrade/install
 */
static int upgrading = FALSE;

int
v_is_upgrade(void)
{
	return (upgrading);
}
