#ifndef XM_DEFS_H
#define	XM_DEFS_H

/*
 * Copyright (c) 1993-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)xm_defs.h 1.11 96/02/05"

#ifndef NeedFunctionPrototypes
#define	NeedFunctionPrototypes 1
#endif

#include <stdarg.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include "sysid_ui.h"
#include "xm_form.h"
#include "xm_msgs.h"

#define	HELPDIR		"sysidxm.help"

#define	XM_MAX_ITEMS	7	/* max number of non-scrollable items */

/*
 * The following group of routines implement the
 * external interface for the Motif dynamically-
 * loadable user interface module.
 */
extern Sysid_err	do_init(int *, char **, int, int);
extern void		do_cleanup(char *, int *);
extern Form_proc	do_form;
extern Form_proc	do_confirm;
extern void		do_error(char *, int);
extern void		do_message(MSG *, int);
extern void		do_dismiss(MSG *, int);
extern TZ_proc		do_get_timezone;
extern LL_proc		do_get_locale;

extern char		*get_err_string(int, int, va_list);
extern char		*get_attr_title(Sysid_attr);
extern char		*get_attr_text(Sysid_attr);
extern char		*get_attr_prompt(Sysid_attr);
extern char		*get_attr_name(Sysid_attr);
extern Field_help	*get_attr_help(Sysid_attr, Field_help *);

extern Sysid_err	xm_validate_value(Widget, Field_desc *);
extern void		xm_popup_error(Widget, const char *, char *);
extern void		xm_get_value(Widget, Field_desc *);
extern XmString		xm_format_text(char *, int, int);
extern Widget		xm_create_working(Widget, char *);
extern void		xm_destroy_working(void);
extern void		xm_help(Widget, XtPointer, XtPointer);
extern Widget		xm_get_shell(Widget w);


extern XmString		*xm_create_list(Menu *, int);
extern void		xm_destroy_list(Widget, XtPointer, XtPointer);

extern void		xm_busy(Widget);
extern void		xm_unbusy(Widget);

extern void		xm_set_traversal(Widget dialog, Field_desc *fields,
				int numfields);

extern char		*_get_err_string(int, int, ...);

#endif	/* !XM_DEFS_H */
