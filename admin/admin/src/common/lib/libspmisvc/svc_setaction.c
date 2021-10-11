#ifndef lint
#pragma ident "@(#)svc_setaction.c 1.2 95/11/27 SMI"
#endif

/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	svc_setaction.c
 * Group:	libspmisvc
 * Description: Routines to set the action codes for initial install.
 */

#include "spmisvc_lib.h"
#include "spmicommon_api.h"
#include "spmisoft_api.h"

/* public function prototypes */
void	set_action_for_machine_type(Module *);

/*-------------------------------------------------------------------*/
/*								     */
/*		Public Functions 				     */
/*								     */
/*-------------------------------------------------------------------*/

/*
 * set_action_for_machine_type()
 *	Called whenever the machine type changes.  Sets up the necessary
 *	fields so that the space code correctly calculates the needed space.
 *	Only used by initial install - glue for space calculations.
 * Parameters:
 *	prod	- product module pointer
 * Return:
 *	none
 * Status:
 *	public
 */
void
set_action_for_machine_type(Module * prod)
{
	static int	machtype = 0;
	Module  	*med = NULL;

	if ((machtype != MT_SERVER) &&
	    (get_machinetype() != MT_SERVER)) {
		machtype = get_machinetype();
		return;
	}

	if (machtype == MT_SERVER && get_machinetype() == MT_SERVER)
		return;

	if (machtype == MT_SERVER || get_machinetype() == MT_SERVER) {
		if (prod->info.prod->p_next_view)
			med = prod->info.prod->p_next_view->p_view_from;
		if (med == NULL)
			med = prod->info.prod->p_view_from;
		if (get_machinetype() == MT_SERVER)
			med->info.media->med_flags = 0;
		else {
			med->info.media->med_flags = SVC_TO_BE_REMOVED;
			/* reset the client expansion space to '0' */
	
			/*
			 * NOTE:  not clear whether we should be doing
			 * something with the error code from this
			 * function.
			 */
			(void) set_client_space(0, 0, 0);
		}
	}
	machtype = get_machinetype();
}
