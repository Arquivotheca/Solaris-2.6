#ifndef lint
#pragma ident "@(#)soft_swi_module.c 1.2 96/01/23 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "spmisoft_api.h"
#include "sw_swi.h"

int
set_current(Module * mod)
{
	int i;

	enter_swlib("set_current");
	i = swi_set_current(mod);
	exit_swlib();
	return (i);
}

int
set_default(Module * mod)
{
	int i;

	enter_swlib("set_default");
	i = swi_set_default(mod);
	exit_swlib();
	return (i);
}

Module *
get_current_media(void)
{
	Module *m;

	enter_swlib("get_current_media");
	m = swi_get_current_media();
	exit_swlib();
	return (m);
}

Module *
get_current_service(void)
{
	Module *m;

	enter_swlib("get_current_service");
	m = swi_get_current_service();
	exit_swlib();
	return (m);
}

Module *
get_current_product(void)
{
	Module *m;

	enter_swlib("get_current_product");
	m = swi_get_current_product();
	exit_swlib();
	return (m);
}

Module *
get_current_category(ModType type)
{
	Module *m;

	enter_swlib("get_current_category");
	m = swi_get_current_category(type);
	exit_swlib();
	return (m);
}

Module *
get_current_metacluster(void)
{
	Module *m;

	enter_swlib("get_current_metacluster");
	m = swi_get_current_metacluster();
	exit_swlib();
	return (m);
}

Module *
get_local_metacluster(void)
{
	Module *m;

	enter_swlib("get_local_metacluster");
	m = swi_get_local_metacluster();
	exit_swlib();
	return (m);
}

Module *
get_current_cluster(void)
{
	Module *m;

	enter_swlib("get_current_cluster");
	m = swi_get_current_cluster();
	exit_swlib();
	return (m);
}

Module *
get_current_package(void)
{
	Module *m;

	enter_swlib("get_current_package");
	m = swi_get_current_package();
	exit_swlib();
	return (m);
}

Module *
get_default_media()
{
	Module *m;

	enter_swlib("get_default_media");
	m = swi_get_default_media();
	exit_swlib();
	return (m);
}

Module *
get_default_service()
{
	Module *m;

	enter_swlib("get_default_service");
	m = swi_get_default_service();
	exit_swlib();
	return (m);
}

Module *
get_default_product(void)
{
	Module *m;

	enter_swlib("get_default_product");
	m = swi_get_default_product();
	exit_swlib();
	return (m);
}

Module *
get_default_category(ModType type)
{
	Module *m;

	enter_swlib("get_default_category");
	m = swi_get_default_category(type);
	exit_swlib();
	return (m);
}

Module *
get_default_metacluster(void)
{
	Module *m;

	enter_swlib("get_default_metacluster");
	m = swi_get_default_metacluster();
	exit_swlib();
	return (m);
}

Module *
get_default_cluster(void)
{
	Module *m;

	enter_swlib("get_default_cluster");
	m = swi_get_default_cluster();
	exit_swlib();
	return (m);
}

Module *
get_default_package(void)
{
	Module *m;

	enter_swlib("get_default_package");
	m = swi_get_default_package();
	exit_swlib();
	return (m);
}

Module *
get_next(Module * mod)
{
	Module *m;

	enter_swlib("get_next");
	m = swi_get_next(mod);
	exit_swlib();
	return (m);
}

Module *
get_sub(Module * mod)
{
	Module *m;

	enter_swlib("get_sub");
	m = swi_get_sub(mod);
	exit_swlib();
	return (m);
}

Module *
get_prev(Module * mod)
{
	Module *m;

	enter_swlib("get_prev");
	m = swi_get_prev(mod);
	exit_swlib();
	return (m);
}

Module *
get_head(Module * mod)
{
	Module *m;

	enter_swlib("get_head");
	m = swi_get_head(mod);
	exit_swlib();
	return (m);
}

int
mark_required(Module * modp)
{
	int i;

	enter_swlib("mark_required");
	i = swi_mark_required(modp);
	exit_swlib();
	return (i);
}

int
mark_module(Module * modp, ModStatus status)
{
	int i;

	enter_swlib("mark_module");
	i = swi_mark_module(modp, status);
	exit_swlib();
	return (i);
}

int
mod_status(Module * mod)
{
	int i;

	enter_swlib("mod_status");
	i = swi_mod_status(mod);
	exit_swlib();
	return (i);
}

int
partial_status(Module * mod)
{
	int i;

	enter_swlib("partial_status");
	i = swi_partial_status(mod);
	exit_swlib();
	return (i);
}

int
toggle_module(Module * mod)
{
	int i;

	enter_swlib("toggle_module");
	i = swi_toggle_module(mod);
	exit_swlib();
	return (i);
}

char *
get_current_locale(void)
{
	char *c;

	enter_swlib("get_current_locale");
	c = swi_get_current_locale();
	exit_swlib();
	return (c);
}

void
set_current_locale(char * loc)
{
	enter_swlib("set_current_locale");
	swi_set_current_locale(loc);
	exit_swlib();
	return;
}

char *
get_default_locale(void)
{
	char *c;

	enter_swlib("get_default_locale");
	c = swi_get_default_locale();
	exit_swlib();
	return (c);
}

int
toggle_product(Module * mod, ModStatus type)
{
	int i;

	enter_swlib("toggle_product");
	i = swi_toggle_product(mod, type);
	exit_swlib();
	return (i);
}
