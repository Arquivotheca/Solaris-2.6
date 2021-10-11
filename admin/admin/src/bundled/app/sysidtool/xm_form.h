#ifndef XM_FORM_H
#define	XM_FORM_H

/*
 * Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
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

#pragma	ident	"@(#)xm_form.h 1.4 95/10/06"

#include "ui_form.h"

typedef struct xm_field	XmField;
struct xm_field {
	Field_desc	*xf_desc;	/* generic description of field */
	Widget		xf_label;	/* value widget (text, button, etc.) */
	Widget		xf_value;	/* value widget (text, button, etc.) */
	struct xm_field	*xf_next;	/* next on list */
};

#define	XM_DEFAULT_COLUMNS	60

extern Widget	toplevel;

extern void	form_intro(Widget, Field_desc *, int,
	char *, char *, Field_help *);
extern Widget	form_create(Widget, char *, XmString, XmString, XmString);
extern void	form_common(Widget, char *, Field_desc *, int);
extern void	form_destroy(Widget);

extern void	update_summary(XmField *);

#endif	/* !XM_FORM_H */
