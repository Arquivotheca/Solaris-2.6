#ifndef lint
#pragma ident "@(#)soft_walktree.c 1.1 95/10/20 SMI"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */
#include "spmisoft_lib.h"

#include <signal.h>
#include <fcntl.h>

/* Public Function Prototypes */

extern void     walktree(Module *, int (*)(Modinfo *, caddr_t), caddr_t);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * walktree()
 *	Do a depth-first search of the module tree, calling the provided
 *	function for each module encountered. Recursively traverse the tree, 
 *	processing local instances, then the children.
 * Parameters:
 *	mod	- pointer to head of module tree
 *	proc	- pointer to function to be invoked by 'walktree'.
 *		  Function must take the following parameters:
 *			Modinfo *	- pointer to module
 *			caddr_t		- pointer to data structure
 *					  passed in with walktree
 *	data	- data argument for parameter function
 * Return:
 *	none
 * Status:
 *	public
 * Note:
 *	recursive
 */
void
walktree(Module * mod, int (*proc)(Modinfo *, caddr_t), caddr_t data)
{
	Modinfo *mi;
	Module	*child;
	int	errs = 0;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("walktree");
#endif

	/* parameter check */
	if (mod == (Module *)NULL)
		return;

	mi = mod->info.mod;
	if (proc(mi, data) != 0)
		errs++;

	while ((mi = next_inst(mi)) != NULL) {
		if (proc(mi, data) != 0)
			errs++;
	}

	child = mod->sub;
	while (child) {
		walktree(child, proc, data);
		child = child->next;
	}
	return;
}
