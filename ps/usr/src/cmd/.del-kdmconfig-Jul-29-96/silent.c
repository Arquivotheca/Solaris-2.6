/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 *
 * silent.c: Action routines for silent mode configuration
 *
 * Description:
 *
 *  These routines perform the silent mode setup of the
 *  window system configuration. If they fail, then the controlling
 *  routine will execute the interactive mode action routines.
 *
 * External Routines:
 *  int get_keyboard_silent(void)
 *  int get_display_silent(void)
 *  int get_pointer_silent(void)
 *
 * These routines are declared in the silent.h file, where their interfaces
 * are also documented.
 */

#pragma ident "@(#)silent.c 1.11 94/08/10 SMI"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windvc.h"
#include "util.h"

/*
 * This is the workhorse of the silent mode. It controls getting the
 * data, getting the node, and filling it's attributes in.
 */

static int xin = 0;

static int
_fill_node(cat_t catname)
{
	char *buf = NULL;
	char cmd[80];
	int res;
	char dummy[1024];
	char devstr[1024];
	char *devstrp = devstr;
	char *name;
	NODE node;
	ATTRIB attr;
	VAL  value;

	(void) sprintf(cmd, "/sbin/bpgetfile -R 0 %s", catname);
	res = read_generic(cmd, &buf, PROCESS_READ);
	if (res) return (res);
		(void) sscanf(buf, "%s %s %s", dummy, dummy, devstrp);
	if (devstrp[strlen(devstrp)] == '\n') devstrp[strlen(devstrp)] = '\0';

	/* Turn this into a usable node */
	node = parse_node(devstrp, catname);

	/* Commit the device to the configured devices list */
	set_selected_device(node);

	if (!strncmp(catname,DISPLAY_CAT,strlen(DISPLAY_CAT)))
		xin = is_xinside(node); 

	free(buf);
	return (0);
}

/*
 * ---------------------------------
 *  Public (external) routines
 * ---------------------------------
 */

int
get_display_silent(void)
{
		return (_fill_node(DISPLAY_CAT));
}

int
get_keyboard_silent(void)
{
		return (_fill_node(KEYBOARD_CAT));
}

int
get_pointer_silent(void)
{
		return (_fill_node(POINTER_CAT));
}

int
get_monitor_silent(void)
{
		return (_fill_node(MONITOR_CAT));
}

int
get_xin_silent(void)
{
		return(xin);
}
