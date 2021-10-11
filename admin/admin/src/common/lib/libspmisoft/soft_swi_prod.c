#ifndef lint
#pragma ident "@(#)soft_swi_prod.c 1.1 95/10/20 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "spmisoft_api.h"
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

void
media_category(Module * mod)
{
	enter_swlib("media_category");
	swi_media_category(mod);
	exit_swlib();
	return;
}
