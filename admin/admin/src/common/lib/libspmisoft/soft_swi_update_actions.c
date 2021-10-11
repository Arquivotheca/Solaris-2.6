#ifndef lint
#pragma ident "@(#)soft_swi_update_actions.c 1.1 95/10/20 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "spmisoft_api.h"
#include "sw_swi.h"

int
load_clients(void)
{
	int i;

	enter_swlib("load_clients");
	i = swi_load_clients();
	exit_swlib();
	return (i);
}

void
update_action(Module * toggled_mod)
{
	enter_swlib("update_action");
	swi_update_action(toggled_mod);
	exit_swlib();
	return;
}

void
upg_select_locale(Module *prodmod, char *locale)
{
	enter_swlib("upg_select_locale");
	swi_upg_select_locale(prodmod, locale);
	exit_swlib();
	return;
}

void
upg_deselect_locale(Module * prodmod, char * locale)
{
	enter_swlib("upg_deselect_locale");
	swi_upg_deselect_locale(prodmod, locale);
	exit_swlib();
	return;
}
