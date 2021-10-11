#ifndef lint
#pragma ident "@(#) swi_sp_free_results.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

void
free_final_space_report(SW_space_results *fsr)
{
	enter_swlib("free_final_space_report");
	swi_free_final_space_report(fsr);
	exit_swlib();
	return;
}
