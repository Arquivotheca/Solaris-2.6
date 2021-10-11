#ifndef lint
#pragma ident "@(#)xm_adminhelp.h 1.1 96/04/02 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	xm_adminhelp.h
 * Group:	libspmixm
 * Description:
 */

#ifndef	_ADMINHELP_H_
#define	_ADMINHELP_H_

#include "xm_strings.h"

#define	XM_HELP_TITLE	ILIBSTR("Help")
#define	XM_AH_TOPICS	ILIBSTR("Topics")
#define	XM_AH_HOWTO	ILIBSTR("How To")
#define	XM_AH_REFER	ILIBSTR("Reference")
#define	XM_AH_PREV	ILIBSTR("Previous")
#define	XM_AH_DONE	ILIBSTR("Done")
#define	XM_AH_CANTOP	ILIBSTR("Can't open %s\n")
#define	XM_AH_LDTAB	ILIBSTR("unexpected leading tab")
#define	XM_AH_BLANK	ILIBSTR("blank line")
#define	XM_AH_TOOMANY	ILIBSTR("too many tokens")
#define	XM_AH_EMPTY	ILIBSTR("empty file")
#define	XM_AH_SEEK	ILIBSTR("can't seek in file: %s\n")
#define	XM_AH_READ	ILIBSTR("can't read file: %s\n")
#define	XM_AH_SYNTAX	\
	ILIBSTR("syntax error, file=%s\n\tline number=%d, \t=%s=\n")

#endif	/* _ADMINHELP_H_ */
