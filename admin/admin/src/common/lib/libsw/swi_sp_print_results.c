#ifndef lint
#pragma ident "@(#) swi_sp_print_results.c 1.1 95/02/13"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "sw_api.h"
#include "sw_swi.h"

void
print_final_results(char * outfile)
{
	enter_swlib("print_final_results");
	swi_print_final_results(outfile);
	exit_swlib();
	return;
}

SW_space_results *
gen_final_space_report(void)
{
	SW_space_results *s;

	enter_swlib("gen_final_space_report");
	s = swi_gen_final_space_report();
	exit_swlib();
	return (s);
}
