#ifndef lint
#pragma ident "@(#)soft_swi_locale.c 1.2 96/04/30 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "spmisoft_api.h"
#include "sw_swi.h"

Module *
get_all_locales(void)
{
	Module	*m;

	enter_swlib("get_all_locales");
	m = swi_get_all_locales();
	exit_swlib();
	return (m);
}

int
select_locale(Module * mod, char * locale)
{
	int	i;

	enter_swlib("select_locale");
	i = swi_select_locale(mod, locale);
	exit_swlib();
	return (i);
}

int
deselect_locale(Module *mod, char *locale)
{
	int	i;

	enter_swlib("deselect_locale");
	i = swi_deselect_locale(mod, locale);
	exit_swlib();
	return (i);
}

int
valid_locale(Module *mod, char *locale)
{
	int i;

	enter_swlib("valid_locale");
	i = swi_valid_locale(mod, locale);
	exit_swlib();
	return (i);
}
