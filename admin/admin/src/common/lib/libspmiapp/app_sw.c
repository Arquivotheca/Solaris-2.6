#ifndef lint
#pragma ident "@(#)app_sw.c 1.2 96/04/29 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	app_sw.c
 * Group:	libspmiapp
 * Description:
 *	SW lib - app level code
 */
#include <stdlib.h>
#include <string.h>

#include "spmiapp_api.h"

/*
 * Function: initNativeArch
 * Description:
 *	Initialize the software library to the current native machine
 *	architecture.
 * Scope:	public
 * Parameters: none
 * Return:	[void]
 * Globals:	none
 * Notes:
 */
void
initNativeArch(void)
{
	char *nativeArch = NULL;

	Module *prod = get_current_product();

	nativeArch = get_default_arch();
	select_arch(prod, nativeArch);
	mark_arch(prod);
}
