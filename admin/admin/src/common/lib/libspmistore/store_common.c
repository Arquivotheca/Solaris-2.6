#ifndef lint
#pragma ident "@(#)store_common.c 1.1 95/10/20 SMI"
#endif

/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	store_common.c
 * Group:	libspmistore
 * Description: 
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include "spmistore_lib.h"
#include "store_strings.h"

/* module globals */

static Units_t	display_units = D_BLOCK;

/* public prototypes */

u_int		blocks2size(Disk_t *, u_int, int);
Units_t		get_units(void);
char *		library_error_msg(int);
Units_t		set_units(Units_t);
u_int		size2blocks(Disk_t *, u_int);

/*---------------------- public functions -----------------------*/

/*
 * Function:	blocks2size
 * Description:	Convert the number of 512 byte blocks to the current unit
 *		(see get_units()/set_units()). The returned value is rounded
 *		to the nearest cylinder boundary, and up or down to the nearest
 *		target unit (see parameters). The physical geometry for
 *		the disk must exist because it is used in the rounding
 *		calculations.
 * Scope: 	public
 * Parameters:	dp	- non-NULL pointer to disk structure with valid sdisk
 *		  	  geometry pointer (state: okay)
 *		size 	- number of 512 byte blocks
 *		round	- Boolean indicating if the value should be rounded up
 *			  to the nearest unit (e.g. MB):
 *				ROUNDDOWN - truncate the unit value
 *				ROUNDUP   - round up the unit value
 *
 *		  NOTE: This does not affect cylinder rounding which is
 *			mandatory
 * Return:	0	- 'dp' is NULL, or disk state is not okay
 *		# >= 0	- converted size
 */
u_int
blocks2size(Disk_t *dp, u_int size, int round)
{
	if (dp == NULL || disk_not_okay(dp))
		return (0);

	switch (get_units()) {
	    case D_MBYTE:
		if (round == ROUNDDOWN)
			size = blocks_to_mb_trunc(dp, size);
		else
			size = blocks_to_mb(dp, size);
		break;

	    case D_KBYTE:
		if (round == ROUNDDOWN)
			size = blocks_to_kb_trunc(dp, size);
		else
			size = blocks_to_kb(dp, size);
		break;

	    case D_BLOCK:
		size = blocks_to_blocks(dp, size);
		break;

	    case D_CYLS:
		size = blocks_to_cyls(dp, size);
		break;

	    default:	/* no action taken */
		break;
	}

	return (size);
}

/*
 * Function:	get_units
 * Description:	Get the current size unit.
 * Scope:	public
 * Parameters:	none
 * Return:	Units_t	 - unit (D_MBYTE, D_KBYTE, D_CYLS, D_BLOCK)
 */
Units_t
get_units(void)
{
	return (display_units);
}

/*
 * Function:	library_error_msg
 * Description: Assemble an error message based on an install library function
 *		return code. The disk name which failed is prepended to the
 *		front of the message. No newline is appended.
 * Scope:	public
 * Parameters:	status	- return status code from library function call
 * Return:	char *	- pointer to local static buffer containing the assembled
 *		  	  message
 */
char *
library_error_msg(int status)
{
	static char	buf[BUFSIZ * 2];

	switch (status) {
	    case D_ALIGNED:
		    (void) sprintf(buf, MSG0_ALIGNED);
		    break;
	    case D_ALTSLICE:
		    (void) sprintf(buf, MSG0_ALTSLICE);
		    break;
	    case D_BADARG:
		    (void) sprintf(buf, MSG0_BADARGS);
		    break;
	    case D_BADDISK:
		    (void) sprintf(buf, MSG0_BADDISK);
		    break;
	    case D_BADORDER:
		    (void) sprintf(buf, MSG0_BADORDER);
		    break;
	    case D_BOOTCONFIG:
		    (void) sprintf(buf, MSG0_BOOTCONFIG);
		    break;
	    case D_BOOTFIXED:
		    (void) sprintf(buf, MSG0_BOOTFIXED);
		    break;
	    case D_CANTPRES:
		    (void) sprintf(buf, MSG0_CANTPRES);
		    break;
	    case D_CHANGED:
		    (void) sprintf(buf, MSG0_CHANGED);
		    break;
	    case D_DUPMNT:
		    (void) sprintf(buf, MSG0_DUPMNT);
		    break;
	    case D_GEOMCHNG:
		    (void) sprintf(buf, MSG0_GEOMCHNG);
		    break;
	    case D_IGNORED:
		    (void) sprintf(buf, MSG0_IGNORED);
		    break;
	    case D_ILLEGAL:
		    (void) sprintf(buf, MSG0_ILLEGAL);
		    break;
	    case D_LOCKED:
		    (void) sprintf(buf, MSG0_LOCKED);
		    break;
	    case D_NODISK:
		    (void) sprintf(buf, MSG0_DISK_INVALID);
		    break;
	    case D_NOFIT:
		    (void) sprintf(buf, MSG0_NOFIT);
		    break;
	    case D_NOGEOM:
		    (void) sprintf(buf, MSG0_NOGEOM);
		    break;
	    case D_NOSOLARIS:
		    (void) sprintf(buf, MSG0_NOSOLARIS);
		    break;
	    case D_NOSPACE:
		    (void) sprintf(buf, MSG0_NOSPACE);
		    break;
	    case D_NOTSELECT:
		    (void) sprintf(buf, MSG0_NOTSELECT);
		    break;
	    case D_OFF:
		    (void) sprintf(buf, MSG0_OFF);
		    break;
	    case D_OK:
		    (void) sprintf(buf, MSG0_OK);
		    break;
	    case D_OUTOFREACH:
		    (void) sprintf(buf, MSG0_OUTOFREACH);
		    break;
	    case D_OVER:
		    (void) sprintf(buf, MSG0_OVER);
		    break;
	    case D_PRESERVED:
		    (void) sprintf(buf, MSG0_PRESERVED);
		    break;
	    case D_ZERO:
		    (void) sprintf(buf, MSG0_ZERO);
		    break;
	    case D_FAILED:
		    (void) sprintf(buf, MSG0_FAILED);
		    break;
	    default:
		    (void) sprintf(buf, MSG1_STD_UNKNOWN_ERROR, status);
		    break;
	}

	return (buf);
}

/*
 * Function:	set_units
 * Description: Set the current (default) size unit. The initial unit
 *		size is D_BLOCK. Return the old unit.
 * Scope:	public
 * Parameters:	u	 - new unit (D_MBYTE, D_KBYTE, D_CYLS, D_BLOCK)
 * Return:	Units_t	 - old unit (D_MBYTE, D_KBYTE, D_CYLS, D_BLOCK)
 */
Units_t
set_units(Units_t u)
{
	Units_t	s;

	s = display_units;
	display_units = u;
	return (s);
}

/*
 * Function:	size2blocks
 * Description: Convert the size in the current units (see set_units()
 *		or get_units()) to 512 byte blocks, rounded to the
 *		nearest cylinder boundary. The physical geometry for
 *		the disk must exist because it is used in the rounding
 *		calculations.
 * Scope:	public
 * Parameters:	dp	- non-NULL pointer to disk structure
 *			  (state: okay)
 *		size	- size of current unit to be converted to blocks
 * Returns:	0	- 'dp' is NULL, the disk state is not "okay", or the
 *			  converted size is '0'
 *		# >= 0	- 'size' converted to sectors
 */
u_int
size2blocks(Disk_t *dp, u_int size)
{
	if (dp == NULL || disk_not_okay(dp))
		return (0);

	switch (get_units()) {

	case D_MBYTE:
		size = mb_to_blocks(dp, size);
		break;

	case D_KBYTE:
		size = kb_to_blocks(dp, size);
		break;

	case D_BLOCK:
		size = blocks_to_blocks(dp, size);
		break;

	case D_CYLS:
		size = cyls_to_blocks(dp, size);
		break;
	}
	return (size);
}
