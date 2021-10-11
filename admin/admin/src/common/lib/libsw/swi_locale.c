#ifndef lint
#pragma ident "@(#) swi_locale.c 1.2 95/02/14"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
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

void
update_l10n_package_status(Module * prod)
{
	enter_swlib("update_l10n_package_status");
	swi_update_l10n_package_status(prod);
	exit_swlib();
	return;
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

void
mark_locales(Module * mod, ModStatus status)
{
	enter_swlib("mark_locales");
	swi_mark_locales(mod, status);
	exit_swlib();
	return;
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
