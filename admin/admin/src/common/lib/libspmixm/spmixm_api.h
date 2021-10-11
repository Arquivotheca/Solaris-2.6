#ifndef lint
#pragma ident "@(#)spmixm_api.h 1.4 96/10/16 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	spmixm_api.h
 * Group:	libspmixm
 * Description:
 *	Header file for common Motif routines.
 */

#ifndef _LIBSPMIXM_API_H
#define	_LIBSPMIXM_API_H

/* gui toolkit header files */
#include <Xm/Xm.h>
#include <Xm/DragDrop.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/FileSB.h>
#include <Xm/MessageB.h>
#include <Xm/DialogS.h>
#include <Xm/PanedW.h>
#include <Xm/CascadeBG.h>
#include <Xm/DrawingA.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/Frame.h>
#include <Xm/List.h>
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>

#include "spmiapp_api.h" /* required */

/*
 * Message Dialog defines/typedefs...
 */

/*
 * structure to hold xm specific info needed by xm message functions.
 */
typedef struct {
	Widget	toplevel;
	Widget	parent;
	XtAppContext    app_context;
	Atom	delete_atom;
	void (*delete_func)(void);
} xm_MsgAdditionalInfo;

/*
 * Motif help
 */

/* the following defines are used for second argument to xm_adminhelp() */
#define TOPIC   'C'
#define HOWTO   'P'
#define REFER   'R'


/*
 * Function prototypes
 */
#ifdef __cplusplus
extern "C" {
#endif

/* xm_msg.c */
/*
 * function with motif implementation if UI message dialogs
 */
extern void xm_MsgFunction(UI_MsgStruct *msg_info);

/* xm_adminhelp.c */
extern int xm_adminhelp(Widget parent, char help_category, char *textfile);
extern void xm_adminhelp_reinit(int destroy);

/* xm_utils.c */
extern Widget xm_ChildWidgetFindByClass(Widget widget, WidgetClass class);
extern Widget xm_GetShell(Widget w);
extern void xm_SetNoResize(Widget toplevel, Widget w);
extern void xm_ForceDisplayUpdate(Widget toplevel, Widget dialog);
extern void xm_ForceEventUpdate(XtAppContext app_context, Widget toplevel);
extern int xm_SetWidgetString(Widget widget, char *message_text);
extern Boolean xm_IsDescendent(Widget base, Widget w);
extern void xm_AlignWidgetCols(Widget base, Widget *col);
extern int xm_SizeScrolledWindowToWorkArea(Widget w, Boolean doWidth, Boolean doHeight);
#ifdef __cplusplus
}
#endif

#endif /* _LIBSPMIXM_API_H */
