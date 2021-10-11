#ifndef lint
#pragma ident "@(#)svc_sp_free_results.c 1.1 95/10/20 SMI"
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
#include "spmicommon_lib.h"
#include "spmisoft_lib.h"
#include "spmisvc_lib.h"
#include "sw_space.h"
#include <libintl.h>
#include <stdlib.h>

/*	Public Function Prototypes	*/

void	free_final_space_report(SW_space_results *);

void
free_final_space_report(SW_space_results *fsr)
{
	SW_space_results *fsr_next;

	while (fsr != NULL) {
		fsr_next = fsr->next;
		free(fsr->sw_mountpnt);
		free(fsr->sw_devname);
		free(fsr);
		fsr = fsr_next;
	}
}
