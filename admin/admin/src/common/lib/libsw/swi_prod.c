#ifndef lint
#pragma ident "@(#) swi_prod.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

char *
get_clustertoc_path(Module * mod)
{
	char *c;

	enter_swlib("get_clustertoc_path");
	c = swi_get_clustertoc_path(mod);
	exit_swlib();
	return (c);
}

int
path_is_readable(char * fn)
{
	int i;

	enter_swlib("path_is_readable");
	i = swi_path_is_readable(fn);
	exit_swlib();
	return (i);
}

void
media_category(Module * mod)
{
	enter_swlib("media_category");
	swi_media_category(mod);
	exit_swlib();
	return;
}
