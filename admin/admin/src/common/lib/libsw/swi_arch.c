#ifndef lint
#pragma ident "@(#) swi_arch.c 1.2 95/05/08"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

Arch	*
get_all_arches(Module *mod)
{
	Arch	*ap;

	enter_swlib("get_all_arches");
	ap = swi_get_all_arches(mod);
	exit_swlib();
	return (ap);
}

int
package_selected(Node * np, char * foo)
{
	int	a;

	enter_swlib("package_selected");
	a = swi_package_selected(np, foo);
	exit_swlib();
	return (a);
}

char  *
get_default_arch(void)
{
	char	*cp;

	enter_swlib("get_default_arch");
	cp = swi_get_default_arch();
	exit_swlib();
	return (cp);
}

char *
get_default_impl(void)
{
	char	*cp;

	enter_swlib("get_default_impl");
	cp = swi_get_default_impl();
	exit_swlib();
	return (cp);
}

char *
get_default_inst(void)
{
	char	*cp;

	enter_swlib("get_default_inst");
	cp = swi_get_default_inst();
	exit_swlib();
	return (cp);
}

char *
get_default_machine(void)
{
	char	*cp;

	enter_swlib("get_default_machine");
	cp = swi_get_default_machine();
	exit_swlib();
	return (cp);
}

char *
get_default_platform(void)
{
	char	*cp;

	enter_swlib("get_default_platform");
	cp = swi_get_default_platform();
	exit_swlib();
	return (cp);
}

char *
get_actual_platform(void)
{
	char	*cp;

	enter_swlib("get_actual_platform");
	cp = swi_get_actual_platform();
	exit_swlib();
	return (cp);
}

int
select_arch(Module * prod, char * arch)
{
	int	i;

	enter_swlib("select_arch");
	i = swi_select_arch(prod, arch);
	exit_swlib();
	return (i);
}

int
valid_arch(Module *prod, char *arch)
{
	int	i;

	enter_swlib("valid_arch");
	i = swi_valid_arch(prod, arch);
	exit_swlib();
	return (i);
}

int
deselect_arch(Module * prod, char * arch)
{
	int	i;

	enter_swlib("deselect_arch");
	i = swi_deselect_arch(prod, arch);
	exit_swlib();
	return (i);
}

void
mark_arch(Module * prod)
{
	enter_swlib("mark_arch");
	swi_mark_arch(prod);
	exit_swlib();
	return;
}
