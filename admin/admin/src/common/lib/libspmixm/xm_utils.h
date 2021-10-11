#ifndef lint
#pragma ident "@(#)xm_utils.h 1.2 96/07/11 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	xm_utils.h
 * Group:	libspmixm
 * Description:
 */

#ifndef _XM_UTILS_H
#define	_XM_UTILS_H

#include "spmixm_api.h"

/* for debugging */
#define	SPMI_XMLIB_NAME	"LIBSPMIXM"
#define	XM_DEBUG	\
	LOGSCR, (get_trace_level() > 0), SPMI_XMLIB_NAME, DEBUG_LOC
#define	XM_DEBUG_NOHD	\
	LOGSCR, (get_trace_level() > 0), NULL, DEBUG_LOC
#define	XM_DEBUG_L1	XM_DEBUG, LEVEL1
#define	XM_DEBUG_L1_NOHD	XM_DEBUG_NOHD, LEVEL1

/*
 * Help
 */
typedef struct {
	Widget toplevel;
	char *text;
} xm_HelpClientData;

/* xm_adminhelp.c */
extern void xm_HelpCB(Widget w, XtPointer client, XmAnyCallbackStruct *cbs);

#endif /* _XM_UTILS_H */
