/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)script.c	1.4	94/06/30 SMI"

#include <sys/dktp/ncrs/ncr.h>


caddr_t	ncr_scriptp;
paddr_t	ncr_script_physp;
paddr_t	ncr_scripts[NSS_FUNCS];
paddr_t ncr_do_list;
paddr_t ncr_di_list;

/*
 * Include the output of the NASM program. NASM is a DOS program
 * which takes the script.ss file and turns it into a series of
 * C data arrays and initializers.
 */
#include "scr.out"

static	size_t	ncr_script_size = sizeof SCRIPT;

/*
 * Offsets of SCRIPT routines. These get turned into physical
 * addresses before they're written into the DSP register. Writing
 * the DSA register starts the program.
 */
static int
ncr_script_offset( int func )
{
	switch (func) {
	case NSS_STARTUP:	/* select a target and start a request */
		return (Ent_start_up);
	case NSS_CONTINUE:	/* continue with current target (no select) */
		return (Ent_continue);
	case NSS_WAIT4RESELECT:	/* wait for reselect */
		return (Ent_resel_m);
	case NSS_CLEAR_ACK:
		return (Ent_clear_ack);
	case NSS_SYNC_OUT:
		return (Ent_sync_out);
	case NSS_ERR_MSG:
		return (Ent_errmsg);
	case NSS_BUS_DEV_RESET:
		return (Ent_dev_reset);
	case NSS_ABORT:
		return (Ent_abort);
	default:
		cmn_err(CE_PANIC, "ncr_script_offset: func=%d\n", func);
	}
}


/*
 * ncr_script_init()
 *
 *	Make certain the SCRIPT	fits in a single page and doesn't
 *	span a page boundary. The SCRIPT and the chip can't deal
 *	with discontiguous memory.
 */
bool_t
ncr_script_init(void)
{
	static	bool_t	done_init = FALSE;
	caddr_t		memp;
	int		func;

	NDBG1(("ncr_script_init\n"));

	if (done_init)
		return (TRUE);

	if (btopr(ncr_script_size) != 1) {
		cmn_err(CE_WARN, "ncr_script_init: Too big %d\n"
				, ncr_script_size);
		return (FALSE);
	}
	/* alloc twice as much as needed to be certain
	 * I can fit it into a single page
	 */
	memp = kmem_zalloc(ncr_script_size * 2, KM_NOSLEEP);
	if (memp == NULL) {

		cmn_err(CE_WARN
			, "ncr_script_init: Unable to allocate memory\n");
		return (FALSE);
	}

	ncr_scriptp = memp;

	/* shift the pointer if necessary */
	memp = PageAlignPtr(memp, ncr_script_size);

	/* copy the script into the buffer we just allocated */
	bcopy((caddr_t)SCRIPT, (caddr_t)memp, ncr_script_size);

	/* save the physical addresses */
	ncr_script_physp = NCR_KVTOP(memp);
	for (func = 0; func < NSS_FUNCS; func++)
		ncr_scripts[func] = ncr_script_physp
					+ ncr_script_offset(func);

	ncr_do_list = ncr_script_physp + Ent_do_list;
	ncr_di_list = ncr_script_physp + Ent_di_list;

	done_init = TRUE;
	NDBG1(("ncr_script_init: okay\n"));
	return (TRUE);
}


/*
 * Free the script buffer
 */
void
ncr_script_fini( void )
{
	if (ncr_scriptp) {
		NDBG1(("ncr_script_fini: free-ing buffer\n"));
		kmem_free(ncr_scriptp, 2 * ncr_script_size);
		ncr_scriptp = NULL;
	}
	return;
}
