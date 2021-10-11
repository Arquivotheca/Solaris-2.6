#ifndef lint
#pragma ident "@(#)soft_version.h 1.1 95/10/20 SMI"
#endif  /* lint */

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

#define	ERR_STR_TOO_LONG	-101

#define	V_NOT_UPGRADEABLE	-2
#define	V_LESS_THEN		-1
#define	V_EQUAL_TO		0
#define	V_GREATER_THEN		1

int	prod_vcmp(char *, char *);
int	pkg_vcmp(char *, char *);

int	is_patch(Modinfo *);
int	is_patch_of(Modinfo *, Modinfo *);
