#ifndef lint
#pragma ident "@(#)tty_utils.h 1.4 96/07/13 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	tty_utils.h
 * Group:	libspmitty
 * Description:
 */

#ifndef	_TTY_UTILS_H
#define	_TTY_UTILS_H

#include "spmitty_api.h"

/*
 * macros usefule for debugging
 */
#define	SPMI_TTYLIB_NAME	"LIBSPMITTY"
#define	TTY_DEBUG	\
	LOGSCR, (get_trace_level() > 0), SPMI_TTYLIB_NAME, DEBUG_LOC
#define	TTY_DEBUG_NOHD	\
	LOGSCR, (get_trace_level() > 0), NULL, DEBUG_LOC
#define	TTY_DEBUG_L1		TTY_DEBUG, LEVEL1
#define	TTY_DEBUG_L1_NOHD	TTY_DEBUG_NOHD, LEVEL1

/* globals */
extern int erasech;
extern int killch;

/*
 * tty_color.c
 */
extern void wcolor_init(void);

extern Fkey_check_func	_fkey_notice_check_func;
extern Fkey_check_func	_fkey_mvwgets_check_func;
extern Fkeys_init_func _fkeys_init_func;
extern Fkey *_fkeys;
extern int _num_fkeys;

/*
 * tty_utils.c
 */
extern int tty_GetForceAlternates(void);

#endif /* _TTY_UTILS_H */
