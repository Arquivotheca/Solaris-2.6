#ifndef lint
#pragma ident "@(#)app_utils.h 1.2 96/07/02 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	app_utils.h
 * Group:	libspmiapp
 * Description:
 */

#ifndef	_APP_UTILS_H
#define	_APP_UTILS_H

#include "spmiapp_api.h"
#include "app_utils.h"

/*
 * macros usefule for debugging
 */
#define	SPMI_APPLIB_NAME	"LIBSPMIAPP"
#define	APP_DEBUG	\
	LOGSCR, (get_trace_level() > 0), SPMI_APPLIB_NAME, DEBUG_LOC
#define	APP_DEBUG_NOHD	\
	LOGSCR, (get_trace_level() > 0), NULL, DEBUG_LOC
#define	APP_DEBUG_L1		APP_DEBUG, LEVEL1
#define	APP_DEBUG_L1_NOHD	APP_DEBUG_NOHD, LEVEL1

#endif /* _APP_UTILS_H */
