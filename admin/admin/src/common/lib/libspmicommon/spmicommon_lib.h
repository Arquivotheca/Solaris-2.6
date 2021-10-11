#ifndef lint
#pragma ident "@(#)spmicommon_lib.h 1.3 96/07/17 SMI"
#endif

/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	spmicommon_lib.h
 * Group:	libspmicommon
 * Description:	This module contains the libspmicommon internal data structures,
 *		constants, and function prototypes.
 */

#ifndef _SPMICOMMON_LIB_H
#define	_SPMICOMMON_LIB_H

#include "spmicommon_api.h"
#include <ctype.h>

/* constants */

#define	TMPLOGFILE	"/tmp/install_log"

/* macros */

#define	must_be(s, c)  	if (*s++ != c) return (0)
#define	skip_digits(s)	if (!isdigit(*(s))) return (-1); \
			while (isdigit(*(s))) (s)++;

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  The following functions can be used by other spmi libs, but not
 *  by applications.
 */

/* common_scriptwrite.c */
void		scriptwrite(FILE *, uint, char **, ...);

/* common_util.c */
int		SystemGetMemsize(void);

#ifdef __cplusplus
}
#endif

#endif	/* _SPMICOMMON_LIB_H */
