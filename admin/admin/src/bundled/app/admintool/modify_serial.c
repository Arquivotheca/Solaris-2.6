/* Copyright (c) 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)modify_serial.c	1.23 96/08/26 Sun Microsystems"

/*******************************************************************************
	modify_serial.c

*******************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <libintl.h>
#include <nl_types.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/MenuShell.h>

#include <Xm/PushB.h>
#include <Xm/LabelG.h>
#include <Xm/TextF.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleBG.h>
#include <Xm/Label.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>

#include "UxXt.h"
#include "util.h"

#define	BAUD_RATE_DEFAULT_VALUE		"9600"

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

/*******************************************************************************
       Includes, Defines, and Global variables from the Declarations Editor:
*******************************************************************************/

Widget 	modifyserialdialog = NULL;

Widget portMonitorMenu;
Widget baudRateMenu;

void portMonTagCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cb
	);
	
void baudRateCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cb
	);	

extern 	void dialogCB(
		Widget w, 
		int* answer, 
		XmAnyCallbackStruct* cbs
	);
extern void stringcopy (
		char * dest, 
		char * src, 
		int maxlen
	);
extern void display_info (
	);


/*******************************************************************************
       The following header file defines the context structure.
*******************************************************************************/

#define CONTEXT_MACRO_ACCESS 1
#include "modify_serial.h"
#undef CONTEXT_MACRO_ACCESS


/*******************************************************************************
       The following are Auxiliary functions.
*******************************************************************************/
int
check_for_colon(
		char	*value
	)
{
	if ( strchr(value, ':') == NULL)
		return (FALSE);
	else
		return (TRUE);
}


void
check_menu(
	Widget  optionMenu,
	char	*value,
	void 	(*callbackProc)(Widget wgt, XtPointer cd, XtPointer cb)	
   )
{
	int			num_children;
	int			found_it=FALSE;
	WidgetList		children;
	XmString		motifStr;
	char			*label_str;
	XmString		xstr;
	Widget			w;
	Widget			menu;
	int			i;

	/* Check if the value is currently in the menu. */
	XtVaGetValues(optionMenu,
		XmNsubMenuId, &menu,
		NULL);
	XtVaGetValues(menu, 
		XmNnumChildren, &num_children,
		XmNchildren, &children,
		NULL);
	for (i = 0; i < num_children && !found_it; i++)
	{
		XtVaGetValues(children[i], 
			XmNlabelString, &motifStr,
			NULL);
		XmStringGetLtoR(motifStr, XmSTRING_DEFAULT_CHARSET, 
			&label_str);

		if (strcmp(label_str, value) == 0)
		{
			/* We found the value in the menu.
			 * Show the found value.
			 */
			XtVaSetValues(optionMenu, 
				XmNmenuHistory, children[i],
				NULL);
			found_it = TRUE;
		}
		XmStringFree(motifStr);
		XtFree(label_str);
	}
	
	if (found_it == FALSE)
	{
		/* Add the value to the menu and show the new widget. */

		/* Destroy "Other" button. */
		XtDestroyWidget(children[num_children-1]);

		/* Add new button. */
		xstr = XmStringCreateLocalized(value);
		w = XtVaCreateManagedWidget( "",
			xmPushButtonGadgetClass,
			menu,
			XmNlabelString, xstr,
			NULL );
		XmStringFree(xstr);

		/* Display the new value. */
		XtVaSetValues(optionMenu, 
			XmNmenuHistory, w,
			NULL);

		/* Add "Other" button. */
		xstr = XmStringCreateLocalized(catgets(_catd, 8, 310, "Other..."));
		w = XtVaCreateManagedWidget( "",
			xmPushButtonGadgetClass,
			menu,
			XmNlabelString, xstr,
			NULL );
		XmStringFree(xstr);
		XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc) callbackProc,
			NULL );

	}
}


int
modifyInit(
	Widget			widget,
	SysmanSerialArg*	serial
)
{
	_UxCmodifyDialog	*modifyWin;
	XmString		xstr;
	Widget 			port_mon_widget;
	Widget			baud_rate_widget;
	int			sts;

	modifyWin = (_UxCmodifyDialog *) UxGetContext(widget);

	SetBusyPointer(True);

	sts = sysman_get_serial(serial, errbuf, ERRBUF_SIZE);

	if (sts != 0) {
		display_error(modifyserialdialog, errbuf);
		SetBusyPointer(False);
		return 0;
	}

	/* Set the port name. */
	xstr = XmStringCreateLocalized((char*)serial->port);
	XtVaSetValues(modifyWin->Uxlabel7, 
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	/* Set the service tag. */
	xstr = XmStringCreateLocalized((char*)serial->svctag);
	XtVaSetValues(modifyWin->Uxlabel9, 
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	/* Is the service enabled? */
	if (serial->service_enabled == s_disabled) {
		XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget4, FALSE,
			 TRUE);
	}
	else {
		XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget4, TRUE,
			 TRUE);
	}

	/* Check if the port monitor tag is in the option menu.
	 * If not, create a new entry for it.
	 * Set the menu to display the option.
	 */
	check_menu(modifyWin->Uxmenu4, (char*)serial->pmtag, portMonTagCB);

	/* Set the default to the initial value.  We store this as client
	 * data to the menu and use it if the user cancels out of
	 * the Port Monitor Dialog dialog.
	 */
	XtVaGetValues(modifyWin->Uxmenu4, 
		XmNmenuHistory, &port_mon_widget,
		NULL);
	XtVaSetValues(modifyWin->Uxmenu4, 
		XmNuserData, port_mon_widget,
		NULL);

	/* Check if the baud rate is in the option menu.  If not 
	 * create a new entry for it.
	 * Set the menu to display the option.
	 */
	check_menu(modifyWin->Uxmenu3, (char*)serial->baud_rate, baudRateCB);

	/* Set the default to the initial value.  We store this as client
	 * data to the menu and use it if the user cancels out of
	 * the baud rate dialog
	 */
	XtVaGetValues(modifyWin->Uxmenu3, 
		XmNmenuHistory, &baud_rate_widget,
		NULL);
	XtVaSetValues(modifyWin->Uxmenu3, 
		XmNuserData, baud_rate_widget,
		NULL);
	
	/* Set the 'more panel' fields. */
	XmTextSetString(modifyWin->UxtextField2, (char*)serial->prompt);
	XmTextSetString(modifyWin->UxtextField3, (char*)serial->comment);
	XmTextSetString(modifyWin->UxtextField1, (char*)serial->termtype);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget5,
		serial->initialize_only, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget6,
		serial->bidirectional, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget7,
		serial->softcar, FALSE);
	
	/* Set the 'expert panel' fields. */
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget8,
		serial->create_utmp_entry, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget9,
		serial->connect_on_carrier, FALSE);
	XmTextSetString(modifyWin->UxtextField4, (char*)serial->service);
	XmTextSetString(modifyWin->UxtextField5, (char*)serial->modules);

	if (strcmp(serial->timeout, "30") == 0)
		XtVaSetValues(modifyWin->Uxmenu5, 
			XmNmenuHistory, modifyWin->Uxmenu3_p1_b12,
			NULL);
	else if (strcmp(serial->timeout, "60") == 0)
		XtVaSetValues(modifyWin->Uxmenu5, 
			XmNmenuHistory, modifyWin->Uxmenu3_p3_b3,
			NULL);
	else if (strcmp(serial->timeout, "120") == 0)
		XtVaSetValues(modifyWin->Uxmenu5, 
			XmNmenuHistory, modifyWin->Uxmenu3_p3_b4,
			NULL);
	else
		XtVaSetValues(modifyWin->Uxmenu5, 
			XmNmenuHistory, modifyWin->Uxmenu3_p1_b11,
			NULL);

	SetBusyPointer(False);

	return (1);
}

/*******************************************************************************
       The following are callback functions.
*******************************************************************************/

static	void	
cancelCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cb)
{
	_UxCmodifyDialog       *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxModifyDialogContext;
	UxModifyDialogContext = UxContext =
			(_UxCmodifyDialog *) UxGetContext( UxWidget );
	{	
		UxPopdownInterface(UxContext->UxmodifyDialog);
	}
	UxModifyDialogContext = UxSaveCtx;
}

static	void	
resetCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cb)
{
	_UxCmodifyDialog       *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxModifyDialogContext;
	UxModifyDialogContext = UxContext =
			(_UxCmodifyDialog *) UxGetContext( UxWidget );
	{
		modifyInit(UxWidget, &UxContext->serial);	
	}
	UxModifyDialogContext = UxSaveCtx;
}

static	void	
modifyCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cb)
{
	_UxCmodifyDialog       *modifyWin;
	Widget                 	UxWidget = wgt;
	XtPointer              	UxClientData = cd;
	XtPointer              	UxCallbackArg = cb;

	modifyWin = (_UxCmodifyDialog *) UxGetContext( UxWidget );
	{

	char		*port_name = NULL;
	char 		*port_mon_tag = NULL;
	char		*service_tag = NULL;
	char 		*baud_rate = NULL;
	char 		*term_type = NULL;
	char 		*comment = NULL;
	char 		*prompt = NULL;
	char 		*service = NULL;
	char 		*modules = NULL;
	char 		*timeout = NULL;
	int		init_only;
	int		bidirect;
	int		soft_carrier;
	int		create_utmp;
	int		connect_carrier;
	int		enabled;
	Widget		portMonTagWidget;
	Widget		baudRateWidget;
	Widget		timeoutWidget;
	XmString	motifStr;
	Boolean		basic, more, expert;
	int		sts;
	SysmanSerialArg	serial;

	extern void update_entry(void*);


	SetBusyPointer(True);

	/* Get the values from the display. */

	/* Port Name */
	XtVaGetValues(modifyWin->Uxlabel7, 
		XmNlabelString, &motifStr,
		NULL);
	XmStringGetLtoR(motifStr, XmSTRING_DEFAULT_CHARSET, 
		&port_name);
	XmStringFree(motifStr);

	/* Service Tag */
	XtVaGetValues(modifyWin->Uxlabel9, 
		XmNlabelString, &motifStr,
		NULL);
	XmStringGetLtoR(motifStr, XmSTRING_DEFAULT_CHARSET, 
		&service_tag);
	XmStringFree(motifStr);

	/* Port Monitor Tag. */
	XtVaGetValues(modifyWin->Uxmenu4, 
		XmNmenuHistory, &portMonTagWidget,
		NULL);
	XtVaGetValues(portMonTagWidget, 
		XmNlabelString, &motifStr,
		NULL);
	XmStringGetLtoR(motifStr, XmSTRING_DEFAULT_CHARSET, 
		(char **)&port_mon_tag);
	XmStringFree(motifStr);

	/* Service enabled or disabled? */
	enabled  = XmToggleButtonGetState(modifyWin->UxtoggleButtonGadget4);

	/* Baud Rate. */
	XtVaGetValues(modifyWin->Uxmenu3, 
		XmNmenuHistory, &baudRateWidget,
		NULL);
	XtVaGetValues(baudRateWidget, 
		XmNlabelString, &motifStr,
		NULL);
	XmStringGetLtoR(motifStr, XmSTRING_DEFAULT_CHARSET, 
		(char **)&baud_rate);
	XmStringFree(motifStr);

	/* Terminal type. */
	term_type    = XmTextGetString(modifyWin->UxtextField1);

	basic = XmToggleButtonGetState(modifyWin->UxtoggleButtonGadget1);
	more = XmToggleButtonGetState(modifyWin->UxtoggleButtonGadget2);
	expert = XmToggleButtonGetState(modifyWin->UxtoggleButtonGadget3);

	if (expert) {
		/* Expert setup - read everything. */

		init_only = XmToggleButtonGetState(
			modifyWin->UxtoggleButtonGadget5);
		bidirect = XmToggleButtonGetState(
			modifyWin->UxtoggleButtonGadget6);
		soft_carrier = XmToggleButtonGetState(
			modifyWin->UxtoggleButtonGadget7);
		prompt     = XmTextGetString(modifyWin->UxtextField2);
		comment    = XmTextGetString(modifyWin->UxtextField3);
		service    = XmTextGetString(modifyWin->UxtextField4);
		modules    = XmTextGetString(modifyWin->UxtextField5);
		/* Timeout. */
		XtVaGetValues(modifyWin->Uxmenu5, 
			XmNmenuHistory, &timeoutWidget,
			NULL);
		if ( timeoutWidget == modifyWin->Uxmenu3_p1_b11) {
			timeout = (char*)XtMalloc(32);
			strcpy(timeout, "");
		}
		else {
			XtVaGetValues(timeoutWidget, 
				XmNlabelString, &motifStr,
				NULL);
			XmStringGetLtoR(motifStr, XmSTRING_DEFAULT_CHARSET, 
				&timeout);
			XmStringFree(motifStr);
		}

		create_utmp = XmToggleButtonGetState(
			modifyWin->UxtoggleButtonGadget8);
		connect_carrier = XmToggleButtonGetState(
			modifyWin->UxtoggleButtonGadget9);
	}
	else if (more) {
		/* More setup. */

		init_only = XmToggleButtonGetState(
			modifyWin->UxtoggleButtonGadget5);
		bidirect = XmToggleButtonGetState(
			modifyWin->UxtoggleButtonGadget6);
		soft_carrier = XmToggleButtonGetState(
			modifyWin->UxtoggleButtonGadget7);
		prompt    = XmTextGetString(modifyWin->UxtextField2);
		comment   = XmTextGetString(modifyWin->UxtextField3);
		service = (char*)XtMalloc(32);
		strcpy(service, "/usr/bin/login\0");
		modules = (char*)XtMalloc(32);
		strcpy(modules, "ldterm,ttcompat\0");
		timeout = (char*)XtMalloc(32);
		strcpy((char *)timeout, "");
		create_utmp = TRUE;
		connect_carrier = FALSE;

	}
	else if (basic) {
		/* Basic setup. */

		init_only = XmToggleButtonGetState(
			modifyWin->UxtoggleButtonGadget5);
		bidirect = XmToggleButtonGetState(
			modifyWin->UxtoggleButtonGadget6);
		soft_carrier = XmToggleButtonGetState(
			modifyWin->UxtoggleButtonGadget7);
		comment  = XmTextGetString(modifyWin->UxtextField3);
		prompt = (char*)XtMalloc(32);
		strcpy(prompt, "login: \0");
		service = (char*)XtMalloc(32);
		strcpy(service, "/usr/bin/login\0");
		modules = (char*)XtMalloc(32);
		strcpy(modules, "ldterm,ttcompat\0");
		timeout = (char*)XtMalloc(32);
		strcpy(timeout, "");
		create_utmp = TRUE;
		connect_carrier = FALSE;
	}

	if (strchr(prompt, '#') != NULL) {
		display_error(modifyWin->UxmodifyDialog,
			catgets(_catd, 8, 313, "Invalid character '#' found in Login Prompt field."));
		SetBusyPointer(False);
		return;
	}

	memset((void *)&serial, 0, sizeof (serial));

	serial.pmtag_key = modifyWin->serial.pmtag_key;
	/*
	 * It's possible that the sysman_get_serial() call that
	 * filled in the modifyWin->serial structure has a NULL
	 * svctag_key in it; this would be the case if the get
	 * call was retrieving the data for a port with no service
	 * on it, where only the pmtag_key had been specified.
	 * The modify call will now need to pass down a svctag_key,
	 * and should use the one that was constructed by the get
	 * call and stuffed into the servicetag widget in the modify
	 * screen.
	 */
	if (modifyWin->serial.svctag_key != NULL) {
		serial.svctag_key = modifyWin->serial.svctag_key;
	} else {
		serial.svctag_key = service_tag;
	}
	serial.port = port_name;
	serial.svctag = service_tag;
	serial.pmtag = port_mon_tag;
	serial.service_enabled = enabled ? s_enabled : s_disabled;
	serial.comment = comment;
	serial.prompt = prompt;
	serial.modules = modules;
	serial.service = service;
	serial.termtype = term_type;
	serial.baud_rate = baud_rate;
	serial.initialize_only = init_only;
	serial.bidirectional = bidirect;
	serial.softcar = soft_carrier;
	serial.timeout = timeout;
	serial.create_utmp_entry = create_utmp;
	serial.connect_on_carrier = connect_carrier;
	serial.identity = "root";
	serial.pmadm = NULL;
	serial.pmtype = NULL;
	serial.device = NULL;
	serial.portflags = NULL;
	serial.ttyflags = NULL;

	sts = sysman_modify_serial(&serial, errbuf, ERRBUF_SIZE);

	if (sts == 0) {
		/* Enable or disable the service. */
		if (enabled == TRUE)
			sts = sysman_enable_serial(&serial, errbuf, ERRBUF_SIZE);
		else 
			sts = sysman_disable_serial(&serial, errbuf, ERRBUF_SIZE);
		update_entry(&serial);

		free_serial(&modifyWin->serial);
		copy_serial(&modifyWin->serial, &serial);

		/* If the OK button was pressed, dismiss the windows. */
		if (UxWidget == modifyWin->UxpushButton2) {
			UxPopdownInterface(modifyWin->UxmodifyDialog);
		} 
	}
	else {
		display_error(modifyWin->UxmodifyDialog, errbuf);
	}

	if (port_name != NULL)
		XtFree(port_name);
	if (service_tag != NULL)
		XtFree(service_tag);
	if (port_mon_tag != NULL)
		XtFree(port_mon_tag);
	if (baud_rate != NULL)
		XtFree(baud_rate);
	if (term_type != NULL)
		XtFree(term_type);
	if (prompt != NULL)
		XtFree(prompt);
	if (comment != NULL)
		XtFree(comment);
	if (service != NULL)
		XtFree(service);
	if (modules != NULL)
		XtFree(modules);
	if (timeout != NULL)
		XtFree(timeout);

	SetBusyPointer(False);

	}
}


static void
templateHardwiredCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cbs)
{
	_UxCmodifyDialog	*modifyWin;
	Widget 			baud_rate_widget;
	

	modifyWin = (_UxCmodifyDialog *) UxGetContext(wgt);


	/* Check if the baud rate is in the option menu.  If not 
	 * create a new entry for it.
	 * Set the menu to display the option.
	 */
	check_menu(modifyWin->Uxmenu3,
		BAUD_RATE_DEFAULT_VALUE,
		baudRateCB);

	/* Set the default to the initial value.  We store this as client
	 * data to the menu and use it if the user cancels out of
	 *  the baud rate dialog
	 */
	XtVaGetValues(modifyWin->Uxmenu3, 
		XmNmenuHistory, &baud_rate_widget,
		NULL);
	XtVaSetValues(modifyWin->Uxmenu3, 
		XmNuserData, baud_rate_widget,
		NULL);

	/* Set the 'more panel' fields. */
	XmTextSetString(modifyWin->UxtextField3, "Terminal - Hardwired"); 

	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget5, FALSE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget6, FALSE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget7, TRUE, FALSE);
	
	/* Set the 'expert panel' fields. */
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget8, TRUE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget9, FALSE, FALSE);
}


static void
templateDialInCB(
		Widget wgt,  
		XtPointer cd, 
		XtPointer cbs)
{
	_UxCmodifyDialog	*modifyWin;
	

	modifyWin = (_UxCmodifyDialog *) UxGetContext(wgt);

	/* Set the 'more panel' fields. */
	XmTextSetString(modifyWin->UxtextField3, "Modem - Dial-In Only");
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget5, FALSE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget6, FALSE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget7, FALSE, FALSE);
	
	/* Set the 'expert panel' fields. */
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget8, TRUE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget9, FALSE, FALSE);
}


static void
templateDialOutCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cbs)
{
	_UxCmodifyDialog	*modifyWin;
	

	modifyWin = (_UxCmodifyDialog *) UxGetContext(wgt);

	/* Set the 'more panel' fields. */
	XmTextSetString(modifyWin->UxtextField3, "Modem - Dial-Out Only");
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget5, TRUE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget6, FALSE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget7, FALSE, FALSE);
	
	/* Set the 'expert panel' fields. */
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget8, TRUE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget9, FALSE, FALSE);


}


static void
templateBidirectCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cbs)
{
	_UxCmodifyDialog	*modifyWin;
	

	modifyWin = (_UxCmodifyDialog *) UxGetContext(wgt);

	/* Set the 'more panel' fields. */
	XmTextSetString(modifyWin->UxtextField3, "Modem - Bidirectional");
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget5, FALSE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget6, TRUE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget7, FALSE, FALSE);
	
	/* Set the 'expert panel' fields. */
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget8, TRUE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget9, FALSE, FALSE);

}


static void
templateInitOnlyCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cbs)
{
	_UxCmodifyDialog	*modifyWin;
	

	modifyWin = (_UxCmodifyDialog *) UxGetContext(wgt);

	/* Set the 'more panel' fields. */
	XmTextSetString(modifyWin->UxtextField3, "Initialize Only - No Connection");
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget5, TRUE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget6, FALSE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget7, FALSE, FALSE);
	
	/* Set the 'expert panel' fields. */
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget8, TRUE, FALSE);
	XmToggleButtonSetState(modifyWin->UxtoggleButtonGadget9, FALSE, FALSE);
}


static void
showBasicDetailCB(
		Widget widget,
		XtPointer cd,
		XmToggleButtonCallbackStruct *cbs)
{

_UxCmodifyDialog	*modifyWin;
Dimension		height;
Dimension		width;


modifyWin = (_UxCmodifyDialog *) UxGetContext(widget);

if (cbs->set == TRUE)
{
	Widget		wa[3];

	/* Unset min/max dimensions */
	XtVaSetValues(XtParent(modifyWin->UxmodifyDialog),
			XmNminHeight, XtUnspecifiedShellInt,
			XmNmaxHeight, XtUnspecifiedShellInt,
			XmNminWidth, XtUnspecifiedShellInt,
			NULL );

	wa[0] = modifyWin->Uxform6;
	wa[1] = modifyWin->Uxform7;
	XtUnmanageChildren(wa, 2);

	/* The current width and height are used as window minimums.
	 * Also, do not allow the user to grow the window longer.
	 */
	XtVaGetValues(XtParent(modifyWin->UxmodifyDialog),
			XmNheight, &height,
			XmNwidth, &width,
			NULL );
	XtVaSetValues(XtParent(modifyWin->UxmodifyDialog),
			XmNminHeight, height,
			XmNmaxHeight, height,
			XmNminWidth, width,
			NULL );
}

}

static void
showMoreDetailCB(		
		Widget widget,
		XtPointer cd,
		XmToggleButtonCallbackStruct *cbs)
{

_UxCmodifyDialog	*modifyWin;
Dimension		height;
Dimension		width;

modifyWin = (_UxCmodifyDialog *) UxGetContext(widget);

if (cbs->set == TRUE)
{
	/* Unset min/max dimensions */
	XtVaSetValues(XtParent(modifyWin->UxmodifyDialog),
			XmNminHeight, XtUnspecifiedShellInt,
			XmNmaxHeight, XtUnspecifiedShellInt,
			XmNminWidth, XtUnspecifiedShellInt,
			NULL );

	XtManageChild(modifyWin->Uxform6);
	XtUnmanageChild(modifyWin->Uxform7);

	/* The current width and height are used as window minimums.
	 * Also, do not allow the user to grow the window longer.
	 */
	XtVaGetValues(XtParent(modifyWin->UxmodifyDialog),
		XmNheight, &height,
		XmNwidth, &width,
		NULL );
	XtVaSetValues(XtParent(modifyWin->UxmodifyDialog),
		XmNminHeight, height,
		XmNmaxHeight, height,
		XmNminWidth, width,
		NULL );
}

}

static void
showExpertDetailCB(
		Widget widget,
		XtPointer cd,
		XmToggleButtonCallbackStruct *cbs)

{

_UxCmodifyDialog	*modifyWin;
Dimension		height;
Dimension		width;

modifyWin = (_UxCmodifyDialog *) UxGetContext(widget);

if (cbs->set == TRUE)
{
	Widget		wa[3];

	/* Unset min/max dimensions */
	XtVaSetValues(XtParent(modifyWin->UxmodifyDialog),
			XmNminHeight, XtUnspecifiedShellInt,
			XmNmaxHeight, XtUnspecifiedShellInt,
			XmNminWidth, XtUnspecifiedShellInt,
			NULL );

	wa[0] = modifyWin->Uxform6;
	wa[1] = modifyWin->Uxform7;
	XtManageChildren(wa, 2);

	/* The current width and height are used as window minimums.
	 * Also, do not allow the user to grow the window longer.
	 */
	XtVaGetValues(XtParent(modifyWin->UxmodifyDialog),
			XmNheight, &height,
			XmNwidth, &width,
			NULL );
	XtVaSetValues(XtParent(modifyWin->UxmodifyDialog),
			XmNminHeight, height,
			XmNmaxHeight, height,
			XmNminWidth, width,
			NULL );
}

}

static void
enableServiceCB(
		Widget widget, 
		XtPointer cd, 
		XmToggleButtonCallbackStruct *cbs)
{
	_UxCmodifyDialog	*modifyWin;

	modifyWin = (_UxCmodifyDialog *) UxGetContext(widget);

	switch (cbs->set)
	{
	case TRUE:
		XtSetSensitive(modifyWin->Uxmenu1, TRUE);
		XtSetSensitive(modifyWin->Uxmenu3, TRUE);
		XtSetSensitive(modifyWin->Uxlabel8, TRUE);
		XtSetSensitive(modifyWin->UxtextField1, TRUE);
		XtSetSensitive(modifyWin->UxlabelGadget1, TRUE);
		XtSetSensitive(modifyWin->UxtoggleButtonGadget5, TRUE);
		XtSetSensitive(modifyWin->UxtoggleButtonGadget6, TRUE);
		XtSetSensitive(modifyWin->UxtoggleButtonGadget7, TRUE);
		XtSetSensitive(modifyWin->UxlabelGadget2, TRUE);
		XtSetSensitive(modifyWin->UxtextField2, TRUE);
		XtSetSensitive(modifyWin->UxlabelGadget3, TRUE);
		XtSetSensitive(modifyWin->UxtextField3, TRUE);
		XtSetSensitive(modifyWin->Uxmenu4, TRUE);
		XtSetSensitive(modifyWin->UxlabelGadget5, TRUE);
		XtSetSensitive(modifyWin->UxtoggleButtonGadget8, TRUE);
		XtSetSensitive(modifyWin->UxtoggleButtonGadget9, TRUE);
		XtSetSensitive(modifyWin->UxlabelGadget6, TRUE);
		XtSetSensitive(modifyWin->UxtextField4, TRUE);
		XtSetSensitive(modifyWin->UxlabelGadget7, TRUE);
		XtSetSensitive(modifyWin->UxtextField5, TRUE);
		XtSetSensitive(modifyWin->Uxmenu5, TRUE);
		XtSetSensitive(modifyWin->Uxmenu1, TRUE);
		break;

	case FALSE:
		XtSetSensitive(modifyWin->Uxmenu1, FALSE);
		XtSetSensitive(modifyWin->Uxmenu3, FALSE);
		XtSetSensitive(modifyWin->Uxlabel8, FALSE);
		XtSetSensitive(modifyWin->UxtextField1, FALSE);
		XtSetSensitive(modifyWin->UxlabelGadget1, FALSE);
		XtSetSensitive(modifyWin->UxtoggleButtonGadget5, FALSE);
		XtSetSensitive(modifyWin->UxtoggleButtonGadget6, FALSE);
		XtSetSensitive(modifyWin->UxtoggleButtonGadget7, FALSE);
		XtSetSensitive(modifyWin->UxlabelGadget2, FALSE);
		XtSetSensitive(modifyWin->UxtextField2, FALSE);
		XtSetSensitive(modifyWin->UxlabelGadget3, FALSE);
		XtSetSensitive(modifyWin->UxtextField3, FALSE);
		XtSetSensitive(modifyWin->Uxmenu4, FALSE);
		XtSetSensitive(modifyWin->UxlabelGadget5, FALSE);
		XtSetSensitive(modifyWin->UxtoggleButtonGadget8, FALSE);
		XtSetSensitive(modifyWin->UxtoggleButtonGadget9, FALSE);
		XtSetSensitive(modifyWin->UxlabelGadget6, FALSE);
		XtSetSensitive(modifyWin->UxtextField4, FALSE);
		XtSetSensitive(modifyWin->UxlabelGadget7, FALSE);
		XtSetSensitive(modifyWin->UxtextField5, FALSE);
		XtSetSensitive(modifyWin->Uxmenu5, FALSE);
		XtSetSensitive(modifyWin->Uxmenu1, FALSE);
		break;
	}
}

void	
baudRateCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cb)
{
	_UxCmodifyDialog       *UxSaveCtx, *UxContext;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxModifyDialogContext;
	UxModifyDialogContext = UxContext =
			(_UxCmodifyDialog *) UxGetContext(modifyserialdialog);
	{
	char*  baud;
	Widget init_widget;

	baud = GetPromptInput(modifyserialdialog,
		catgets(_catd, 8, 319, "Admintool: Set Baud Rate"), catgets(_catd, 8, 320, "/etc/ttydefs Entry:"));

	if (baud != NULL) {
		/* Check if the baud rate is in the option menu.
		 *  If not, create a new entry for it.
		 */
		check_menu(baudRateMenu, baud, (XtCallbackProc)baudRateCB);
		XtFree(baud);
	}
	else {
		/* Put the baud rate back to the initial setting. */
		XtVaGetValues(UxContext->Uxmenu3, 
			XmNuserData,  &init_widget,
			NULL);
		XtVaSetValues(UxContext->Uxmenu3, 
			XmNmenuHistory, init_widget,
			NULL);
	}

	}
	UxModifyDialogContext = UxSaveCtx;
}

void	
portMonTagCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cb)
{
	_UxCmodifyDialog       *UxSaveCtx, *UxContext;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxModifyDialogContext;
	UxModifyDialogContext = UxContext =
			(_UxCmodifyDialog *) UxGetContext(modifyserialdialog);
	{
	char*  tag;
	Widget init_widget;

	tag = GetPromptInput(modifyserialdialog,
		catgets(_catd, 8, 321, "Admintool: Set Port Monitor Tag"), catgets(_catd, 8, 322, "Port Monitor Tag:"));

	if (tag != NULL) {
		/* Check if the port monitor tag is in the option menu.
		 *  If not, create a new entry for it.
		 */
		check_menu(portMonitorMenu, tag, (XtCallbackProc)portMonTagCB);
		XtFree(tag);
	}
	else {
		/* Put the port monitor tag back to the initial setting. */
		XtVaGetValues(UxContext->Uxmenu4, 
			XmNuserData,  &init_widget,
			NULL);
		XtVaSetValues(UxContext->Uxmenu4, 
			XmNmenuHistory, init_widget,
			NULL);
	}

	}
	UxModifyDialogContext = UxSaveCtx;
}



/*******************************************************************************
       The 'build_' function creates all the widgets
       using the resource values specified in the Property Editor.
*******************************************************************************/

static Widget	_Uxbuild_modifyDialog(Widget parent)
{
	Widget		_UxParent;
	Widget		menu1_p1_shell;
	Widget		menu3_p1_shell;
	Widget		menu3_p2_shell;
	Widget		menu3_p3_shell;
	Widget		bigrc;

	_UxParent = parent;
	if ( _UxParent == NULL )
	{
		_UxParent = GtopLevel;
	}

	_UxParent = XtVaCreatePopupShell( "modifyDialog_shell",
			xmDialogShellWidgetClass, _UxParent,
			XmNshellUnitType, XmPIXELS,
			XmNtitle, "modifyDialog",
			NULL );

	modifyDialog = XtVaCreateWidget( "modifyDialog",
			xmFormWidgetClass,
			_UxParent,
			XmNunitType, XmPIXELS,
			RES_CONVERT( XmNdialogTitle, catgets(_catd, 8, 323, "Admintool: Modify Serial Port") ),
			NULL );
	UxPutContext( modifyDialog, (char *) UxModifyDialogContext );

	bigrc = XtVaCreateManagedWidget( "bigrc",
		xmRowColumnWidgetClass,
		modifyDialog,
		XmNorientation, XmVERTICAL,
		XmNnumColumns, 1,
		XmNspacing, 2,
		XmNmarginHeight, 1,
		XmNmarginWidth, 1,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 1,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 1,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 1,
		NULL );

	form2 = XtVaCreateManagedWidget( "form2",
			xmFormWidgetClass,
			bigrc,
			XmNmarginWidth, 10,
			NULL );
	UxPutContext( form2, (char *) UxModifyDialogContext );


	menu1_p1_shell = XtVaCreatePopupShell ("menu1_p1_shell",
			xmMenuShellWidgetClass, form2,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu1_p1 = XtVaCreateWidget( "menu1_p1",
			xmRowColumnWidgetClass,
			menu1_p1_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu1_p1, (char *) UxModifyDialogContext );

	menu1_p1_b1 = XtVaCreateManagedWidget( "menu1_p1_b1",
			xmPushButtonGadgetClass,
			menu1_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 324, "Terminal - Hardwired") ),
			NULL );
	UxPutContext( menu1_p1_b1, (char *) UxModifyDialogContext );

	menu1_p1_b2 = XtVaCreateManagedWidget( "menu1_p1_b2",
			xmPushButtonGadgetClass,
			menu1_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 325, "Modem - Dial in Only") ),
			NULL );
	UxPutContext( menu1_p1_b2, (char *) UxModifyDialogContext );

	menu1_p1_b3 = XtVaCreateManagedWidget( "menu1_p1_b3",
			xmPushButtonGadgetClass,
			menu1_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 326, "Modem - Dial out Only") ),
			NULL );
	UxPutContext( menu1_p1_b3, (char *) UxModifyDialogContext );

	menu1_p1_b4 = XtVaCreateManagedWidget( "menu1_p1_b4",
			xmPushButtonGadgetClass,
			menu1_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 327, "Modem - Bidirectional") ),
			NULL );
	UxPutContext( menu1_p1_b4, (char *) UxModifyDialogContext );

	menu1_p1_b5 = XtVaCreateManagedWidget( "menu1_p1_b5",
			xmPushButtonGadgetClass,
			menu1_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 328, "Initialize Only - No Connection") ),
			NULL );
	UxPutContext( menu1_p1_b5, (char *) UxModifyDialogContext );

	menu1 = XtVaCreateManagedWidget( "menu1",
			xmRowColumnWidgetClass,
			form2,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu1_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 329, "Template:") ),
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 35,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 15,
			NULL );
	UxPutContext( menu1, (char *) UxModifyDialogContext );

	label5 = XtVaCreateManagedWidget( "label5",
			xmLabelWidgetClass,
			form2,
			XmNalignment, XmALIGNMENT_END,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 330, "Detail:") ),
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 21,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, menu1,
			XmNleftOffset, 80,
			NULL );
	UxPutContext( label5, (char *) UxModifyDialogContext );

	rowColumn1 = XtVaCreateManagedWidget( "rowColumn1",
			xmRowColumnWidgetClass,
			form2,
			XmNradioBehavior, TRUE,
			XmNorientation, XmHORIZONTAL,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 15,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label5,
			XmNleftOffset, 5,
			NULL );
	UxPutContext( rowColumn1, (char *) UxModifyDialogContext );


	toggleButtonGadget1 = XtVaCreateManagedWidget( "toggleButtonGadget1",
			xmToggleButtonGadgetClass,
			rowColumn1,
			RES_CONVERT( XmNlabelString,
				catgets(_catd, 8, 331, "Basic") ),
			XmNset, TRUE,
			NULL );
	UxPutContext( toggleButtonGadget1, (char *) UxModifyDialogContext );

	XtAddCallback( toggleButtonGadget1, XmNvalueChangedCallback,
		(XtCallbackProc) showBasicDetailCB,
		(XtPointer) UxModifyDialogContext );

	toggleButtonGadget2 = XtVaCreateManagedWidget( "toggleButtonGadget2",
			xmToggleButtonGadgetClass,
			rowColumn1,
			RES_CONVERT( XmNlabelString,
				catgets(_catd, 8, 332, "More") ),
			NULL );
	UxPutContext( toggleButtonGadget2, (char *) UxModifyDialogContext );

	XtAddCallback( toggleButtonGadget2, XmNvalueChangedCallback,
		(XtCallbackProc) showMoreDetailCB,
		(XtPointer) UxModifyDialogContext );

	toggleButtonGadget3 = XtVaCreateManagedWidget( "toggleButtonGadget3",
			xmToggleButtonGadgetClass,
			rowColumn1,
			RES_CONVERT( XmNlabelString,
				catgets(_catd, 8, 333, "Expert") ),
			NULL );
	UxPutContext( toggleButtonGadget3, (char *) UxModifyDialogContext );

	XtAddCallback( toggleButtonGadget3, XmNvalueChangedCallback,
		(XtCallbackProc) showExpertDetailCB,
		(XtPointer) UxModifyDialogContext );

	separatorGadget1 = XtVaCreateManagedWidget( "separatorGadget1",
			xmSeparatorGadgetClass,
			form2,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, menu1,
			XmNtopOffset, 15,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNleftOffset, 0,
			XmNrightOffset, 0,
			NULL );
	UxPutContext( separatorGadget1, (char *) UxModifyDialogContext );

	form5 = XtVaCreateManagedWidget( "form5",
			xmFormWidgetClass,
			bigrc,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 0,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 0,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, form2,
			NULL );
	UxPutContext( form5, (char *) UxModifyDialogContext );

	label6 = XtVaCreateManagedWidget( "label6",
			xmLabelWidgetClass,
			form5,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 70,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 20,
			XmNalignment, XmALIGNMENT_END,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 334, "Port:") ),
			NULL );
	UxPutContext( label6, (char *) UxModifyDialogContext );

	label7 = XtVaCreateManagedWidget( "label7",
			xmLabelWidgetClass,
			form5,
			XmNalignment, XmALIGNMENT_BEGINNING,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 335, "multiple") ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label6,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 20,
			XmNleftOffset, 7,
			NULL );
	UxPutContext( label7, (char *) UxModifyDialogContext );


	toggleButtonGadget4 = XtVaCreateManagedWidget( "toggleButtonGadget4",
			xmToggleButtonGadgetClass,
			form5,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label6,
			XmNtopOffset, 2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 336, "Service Enable") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftWidget, label7,
			NULL );
	UxPutContext( toggleButtonGadget4, (char *) UxModifyDialogContext );

	menu3_p1_shell = XtVaCreatePopupShell ("menu3_p1_shell",
			xmMenuShellWidgetClass, form5,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu3_p1 = XtVaCreateWidget( "menu3_p1",
			xmRowColumnWidgetClass,
			menu3_p1_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu3_p1, (char *) UxModifyDialogContext );


	menu3_p1_b1 = XtVaCreateManagedWidget( "menu3_p1_b1",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, "38400" ),
			NULL );
	UxPutContext( menu3_p1_b1, (char *) UxModifyDialogContext );

	menu3_p1_b2 = XtVaCreateManagedWidget( "menu3_p1_b2",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, "19200" ),
			NULL );
	UxPutContext( menu3_p1_b2, (char *) UxModifyDialogContext );

	menu3_p1_b3 = XtVaCreateManagedWidget( "menu3_p1_b3",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, "9600" ),
			NULL );
	UxPutContext( menu3_p1_b3, (char *) UxModifyDialogContext );

	menu3_p1_b4 = XtVaCreateManagedWidget( "menu3_p1_b4",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, "2400" ),
			NULL );
	UxPutContext( menu3_p1_b4, (char *) UxModifyDialogContext );

	menu3_p1_b5 = XtVaCreateManagedWidget( "menu3_p1_b5",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, "1200" ),
			NULL );
	UxPutContext( menu3_p1_b5, (char *) UxModifyDialogContext );

	menu3_p1_b6 = XtVaCreateManagedWidget( "menu3_p1_b6",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, "300" ),
			NULL );
	UxPutContext( menu3_p1_b6, (char *) UxModifyDialogContext );

	menu3_p1_b7 = XtVaCreateManagedWidget( "menu3_p1_b7",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, "auto" ),
			NULL );
	UxPutContext( menu3_p1_b7, (char *) UxModifyDialogContext );

	menu3_p1_b8 = XtVaCreateManagedWidget( "menu3_p1_b8",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 338, "Other...") ),
			NULL );
	UxPutContext( menu3_p1_b8, (char *) UxModifyDialogContext );

	menu3 = XtVaCreateManagedWidget( "menu3",
			xmRowColumnWidgetClass,
			form5,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu3_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 339, "Baud Rate:") ),
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 15,
			XmNnumColumns, 4,
			XmNorientation, XmHORIZONTAL,
			XmNpacking, XmPACK_COLUMN,
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 55,
			NULL );
	UxPutContext( menu3, (char *) UxModifyDialogContext );

	baudRateMenu = menu3;

	label8 = XtVaCreateManagedWidget( "label8",
			xmLabelWidgetClass,
			form5,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 340, "Terminal Type:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftOffset, -18,
			XmNleftWidget, menu3,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, menu3,
			XmNtopOffset, 5,
			NULL );
	UxPutContext( label8, (char *) UxModifyDialogContext );

	textField1 = XtVaCreateManagedWidget( "textField1",
			xmTextFieldWidgetClass,
			form5,
			XmNmarginHeight, 1,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label8,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 20,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label8,
			NULL );
	UxPutContext( textField1, (char *) UxModifyDialogContext );

	separatorGadget2 = XtVaCreateManagedWidget( "separatorGadget2",
			xmSeparatorGadgetClass,
			form5,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, textField1,
			XmNtopOffset, 15,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( separatorGadget2, (char *) UxModifyDialogContext );

	form6 = XtVaCreateWidget( "form6",
			xmFormWidgetClass,
			bigrc,
			NULL );
	UxPutContext( form6, (char *) UxModifyDialogContext );

	labelGadget1 = XtVaCreateManagedWidget( "labelGadget1",
			xmLabelGadgetClass,
			form6,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 341, "Options:") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 50,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 15,
			NULL );
	UxPutContext( labelGadget1, (char *) UxModifyDialogContext );

	toggleButtonGadget5 = XtVaCreateManagedWidget( "toggleButtonGadget5",
			xmToggleButtonGadgetClass,
			form6,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftOffset, 4,
			XmNleftWidget, labelGadget1,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, labelGadget1,
			XmNbottomOffset, -5,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 342, "Initialize Only") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			NULL );
	UxPutContext( toggleButtonGadget5, (char *) UxModifyDialogContext );

	toggleButtonGadget6 = XtVaCreateManagedWidget( "toggleButtonGadget6",
			xmToggleButtonGadgetClass,
			form6,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 343, "Bidirectional") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, toggleButtonGadget5,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 5,
			XmNtopWidget, toggleButtonGadget5,
			NULL );
	UxPutContext( toggleButtonGadget6, (char *) UxModifyDialogContext );

	toggleButtonGadget7 = XtVaCreateManagedWidget( "toggleButtonGadget7",
			xmToggleButtonGadgetClass,
			form6,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 344, "Software Carrier") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, toggleButtonGadget6,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 5,
			XmNtopWidget, toggleButtonGadget6,
			NULL );
	UxPutContext( toggleButtonGadget7, (char *) UxModifyDialogContext );

	labelGadget2 = XtVaCreateManagedWidget( "labelGadget2",
			xmLabelGadgetClass,
			form6,
			XmNalignment, XmALIGNMENT_END,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 345, "Login Prompt:") ),
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 54,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 15,
			NULL );
	UxPutContext( labelGadget2, (char *) UxModifyDialogContext );

	textField2 = XtVaCreateManagedWidget( "textField2",
			xmTextFieldWidgetClass,
			form6,
			XmNmarginHeight, 1,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, labelGadget2,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, labelGadget2,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 20,
			NULL );
	UxPutContext( textField2, (char *) UxModifyDialogContext );

	labelGadget3 = XtVaCreateManagedWidget( "labelGadget3",
			xmLabelGadgetClass,
			form6,
			XmNalignment, XmALIGNMENT_END,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 346, "Comment:") ),
			XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightWidget, labelGadget2,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, labelGadget2,
			XmNtopOffset, 5,
			NULL );
	UxPutContext( labelGadget3, (char *) UxModifyDialogContext );

	textField3 = XtVaCreateManagedWidget( "textField3",
			xmTextFieldWidgetClass,
			form6,
			XmNmarginHeight, 1,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, labelGadget3,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, labelGadget3,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 20,
			NULL );
	UxPutContext( textField3, (char *) UxModifyDialogContext );

	labelGadget4 = XtVaCreateManagedWidget( "labelGadget4",
			xmLabelGadgetClass,
			form6,
			XmNalignment, XmALIGNMENT_END,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 347, "Service Tag:") ),
			XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightWidget, labelGadget3,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 5,
			XmNtopWidget, labelGadget3,
			NULL );
	UxPutContext( labelGadget4, (char *) UxModifyDialogContext );

	label9 = XtVaCreateManagedWidget( "label9",
			xmLabelWidgetClass,
			form6,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, labelGadget4,
			XmNleftOffset, 5,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, labelGadget4,
			XmNalignment, XmALIGNMENT_BEGINNING,
			NULL );
	UxPutContext( label9, (char *) UxModifyDialogContext );

	menu3_p2_shell = XtVaCreatePopupShell ("menu3_p2_shell",
			xmMenuShellWidgetClass, form6,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu3_p2 = XtVaCreateWidget( "menu3_p2",
			xmRowColumnWidgetClass,
			menu3_p2_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu3_p2, (char *) UxModifyDialogContext );


	menu3_p1_b9 = XtVaCreateManagedWidget( "menu3_p1_b9",
			xmPushButtonGadgetClass,
			menu3_p2,
			RES_CONVERT( XmNlabelString, "zsmon" ),
			NULL );
	UxPutContext( menu3_p1_b9, (char *) UxModifyDialogContext );

	menu3_p1_b10 = XtVaCreateManagedWidget( "menu3_p1_b10",
			xmPushButtonGadgetClass,
			menu3_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 348, "Other...") ),
			NULL );
	UxPutContext( menu3_p1_b10, (char *) UxModifyDialogContext );

	menu4 = XtVaCreateManagedWidget( "menu4",
			xmRowColumnWidgetClass,
			form6,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu3_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 349, "Port Monitor Tag:") ),
			XmNnumColumns, 4,
			XmNorientation, XmHORIZONTAL,
			XmNpacking, XmPACK_COLUMN,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, labelGadget4,
			XmNleftOffset, -33,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, labelGadget4,
			NULL );
	UxPutContext( menu4, (char *) UxModifyDialogContext );

	portMonitorMenu = menu4;

	separatorGadget3 = XtVaCreateManagedWidget( "separatorGadget3",
			xmSeparatorGadgetClass,
			form6,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, menu4,
			XmNtopOffset, 15,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( separatorGadget3, (char *) UxModifyDialogContext );

	form7 = XtVaCreateWidget( "form7",
			xmFormWidgetClass,
			bigrc,
			NULL );
	UxPutContext( form7, (char *) UxModifyDialogContext );

	labelGadget5 = XtVaCreateManagedWidget( "labelGadget5",
			xmLabelGadgetClass,
			form7,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 350, "Expert Options:") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 10,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 15,
			NULL );
	UxPutContext( labelGadget5, (char *) UxModifyDialogContext );

	toggleButtonGadget8 = XtVaCreateManagedWidget( "toggleButtonGadget8",
			xmToggleButtonGadgetClass,
			form7,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftOffset, 4,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, labelGadget5,
			XmNbottomOffset, -5,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 351, "Create utmp Entry") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftWidget, labelGadget5,
			NULL );
	UxPutContext( toggleButtonGadget8, (char *) UxModifyDialogContext );

	toggleButtonGadget9 = XtVaCreateManagedWidget( "toggleButtonGadget9",
			xmToggleButtonGadgetClass,
			form7,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 352, "Connect on Carrier") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, toggleButtonGadget8,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 5,
			XmNtopWidget, toggleButtonGadget8,
			NULL );
	UxPutContext( toggleButtonGadget9, (char *) UxModifyDialogContext );

	labelGadget6 = XtVaCreateManagedWidget( "labelGadget6",
			xmLabelGadgetClass,
			form7,
			XmNalignment, XmALIGNMENT_END,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 353, "Service:") ),
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 58,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 15,
			NULL );
	UxPutContext( labelGadget6, (char *) UxModifyDialogContext );

	textField4 = XtVaCreateManagedWidget( "textField4",
			xmTextFieldWidgetClass,
			form7,
			XmNmarginHeight, 1,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, labelGadget6,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, labelGadget6,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 20,
			NULL );
	UxPutContext( textField4, (char *) UxModifyDialogContext );

	labelGadget7 = XtVaCreateManagedWidget( "labelGadget7",
			xmLabelGadgetClass,
			form7,
			XmNalignment, XmALIGNMENT_END,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 354, "Streams Modules:") ),
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, labelGadget6,
			XmNtopOffset, 5,
			XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightWidget, labelGadget6,
			NULL );
	UxPutContext( labelGadget7, (char *) UxModifyDialogContext );

	textField5 = XtVaCreateManagedWidget( "textField5",
			xmTextFieldWidgetClass,
			form7,
			XmNmarginHeight, 1,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, labelGadget7,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 20,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, labelGadget7,
			NULL );
	UxPutContext( textField5, (char *) UxModifyDialogContext );

	menu3_p3_shell = XtVaCreatePopupShell ("menu3_p3_shell",
			xmMenuShellWidgetClass, form7,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu3_p3 = XtVaCreateWidget( "menu3_p3",
			xmRowColumnWidgetClass,
			menu3_p3_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu3_p3, (char *) UxModifyDialogContext );

	menu3_p1_b11 = XtVaCreateManagedWidget( "menu3_p1_b11",
			xmPushButtonGadgetClass,
			menu3_p3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 355, "Never") ),
			NULL );
	UxPutContext( menu3_p1_b11, (char *) UxModifyDialogContext );

	menu3_p1_b12 = XtVaCreateManagedWidget( "menu3_p1_b12",
			xmPushButtonGadgetClass,
			menu3_p3,
			RES_CONVERT( XmNlabelString, "30" ),
			NULL );
	UxPutContext( menu3_p1_b12, (char *) UxModifyDialogContext );

	menu3_p3_b3 = XtVaCreateManagedWidget( "menu3_p3_b3",
			xmPushButtonGadgetClass,
			menu3_p3,
			RES_CONVERT( XmNlabelString, "60" ),
			NULL );
	UxPutContext( menu3_p3_b3, (char *) UxModifyDialogContext );

	menu3_p3_b4 = XtVaCreateManagedWidget( "menu3_p3_b4",
			xmPushButtonGadgetClass,
			menu3_p3,
			RES_CONVERT( XmNlabelString, "120" ),
			NULL );
	UxPutContext( menu3_p3_b4, (char *) UxModifyDialogContext );

	menu5 = XtVaCreateManagedWidget( "menu5",
			xmRowColumnWidgetClass,
			form7,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu3_p3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 356, "Timeout (secs):") ),
			XmNnumColumns, 4,
			XmNorientation, XmHORIZONTAL,
			XmNpacking, XmPACK_COLUMN,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, labelGadget7,
			XmNleftOffset, 7,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, labelGadget7,
			XmNtopOffset, 5,
			NULL );
	UxPutContext( menu5, (char *) UxModifyDialogContext );

	separatorGadget4 = XtVaCreateManagedWidget( "separatorGadget4",
			xmSeparatorGadgetClass,
			form7,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, menu5,
			XmNtopOffset, 20,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( separatorGadget4, (char *) UxModifyDialogContext );

	form8 = XtVaCreateManagedWidget( "form8",
			xmFormWidgetClass,
			modifyDialog,
			XmNmarginHeight, 10,
			XmNmarginWidth, 10,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 1,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 1,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, bigrc,
			NULL );
	UxPutContext( form8, (char *) UxModifyDialogContext );

	pushButton2 = XtVaCreateManagedWidget( "pushButton2",
			xmPushButtonWidgetClass,
			form8,
			XmNwidth, 104,
			XmNheight, 28,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 357, "OK") ),
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 20,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset, 20,
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 4,
			NULL );
	UxPutContext( pushButton2, (char *) UxModifyDialogContext );

	pushButton3 = XtVaCreateManagedWidget( "pushButton3",
			xmPushButtonWidgetClass,
			form8,
			XmNwidth, 104,
			XmNheight, 28,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 358, "Apply") ),
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 20,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset, 20,
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 23,
			NULL );
	UxPutContext( pushButton3, (char *) UxModifyDialogContext );

	pushButton9 = XtVaCreateManagedWidget( "pushButton9",
			xmPushButtonWidgetClass,
			form8,
			XmNwidth, 104,
			XmNheight, 28,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 359, "Reset") ),
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 20,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset, 20,
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 42,
			NULL );
	UxPutContext( pushButton9, (char *) UxModifyDialogContext );

	pushButton10 = XtVaCreateManagedWidget( "pushButton10",
			xmPushButtonWidgetClass,
			form8,
			XmNwidth, 104,
			XmNheight, 28,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 360, "Cancel") ),
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 20,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset, 20,
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 61,
			NULL );
	UxPutContext( pushButton10, (char *) UxModifyDialogContext );

	pushButton11 = XtVaCreateManagedWidget( "pushButton11",
			xmPushButtonWidgetClass,
			form8,
			XmNwidth, 104,
			XmNheight, 28,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 361, "Help") ),
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 20,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset, 20,
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 80,
			NULL );
	UxPutContext( pushButton11, (char *) UxModifyDialogContext );

	XtVaSetValues(modifyDialog,
			XmNdefaultButton, pushButton2,
			NULL );
	XtVaSetValues(form8,
			XmNdefaultButton, pushButton2,
			NULL );


	XtAddCallback( toggleButtonGadget4, XmNvalueChangedCallback,
		(XtCallbackProc) enableServiceCB,
		(XtPointer) UxModifyDialogContext );

	XtAddCallback( pushButton2, XmNactivateCallback,
		(XtCallbackProc) modifyCB,
		(XtPointer) UxModifyDialogContext );

	XtAddCallback( pushButton3, XmNactivateCallback,
		(XtCallbackProc) modifyCB,
		(XtPointer) UxModifyDialogContext );

	XtAddCallback( pushButton9, XmNactivateCallback,
		(XtCallbackProc) resetCB,
		(XtPointer) UxModifyDialogContext );

	XtAddCallback( pushButton10, XmNactivateCallback,
		(XtCallbackProc) cancelCB,
		(XtPointer) UxModifyDialogContext );

	XtAddCallback( pushButton11, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"serial_window.r.hlp" );

	XtAddCallback( menu3_p1_b10, XmNactivateCallback,
		(XtCallbackProc) portMonTagCB,
		(XtPointer) UxModifyDialogContext );

	XtAddCallback( menu3_p1_b8, XmNactivateCallback,
		(XtCallbackProc) baudRateCB,
		(XtPointer) UxModifyDialogContext );

	XtAddCallback( menu1_p1_b1, XmNactivateCallback,
		(XtCallbackProc) templateHardwiredCB,
		(XtPointer) UxModifyDialogContext );

	XtAddCallback( menu1_p1_b2, XmNactivateCallback,
		(XtCallbackProc) templateDialInCB,
		(XtPointer) UxModifyDialogContext );

	XtAddCallback( menu1_p1_b3, XmNactivateCallback,
		(XtCallbackProc) templateDialOutCB,
		(XtPointer) UxModifyDialogContext );

	XtAddCallback( menu1_p1_b4, XmNactivateCallback,
		(XtCallbackProc) templateBidirectCB,
		(XtPointer) UxModifyDialogContext );

	XtAddCallback( menu1_p1_b5, XmNactivateCallback,
		(XtCallbackProc) templateInitOnlyCB,
		(XtPointer) UxModifyDialogContext );

	XtAddCallback( modifyDialog, XmNdestroyCallback,
		(XtCallbackProc) UxDestroyContextCB,
		(XtPointer) UxModifyDialogContext);


	return ( modifyDialog );
}

/*******************************************************************************
       The following is the 'Interface function' which is the
       external entry point for creating this interface.
       This function should be called from your application or from
       a callback function.
*******************************************************************************/

Widget	create_modifyDialog( swidget _UxUxParent )
{
	Widget                  rtrn;
	_UxCmodifyDialog        *UxContext;

	UxModifyDialogContext = UxContext =
		(_UxCmodifyDialog *) UxNewContext( sizeof(_UxCmodifyDialog), False );

	rtrn = _Uxbuild_modifyDialog(_UxUxParent);

	/* Set detail to Basic. */
	XmToggleButtonSetState(UxContext->UxtoggleButtonGadget1, TRUE, TRUE);

	/* null structure */
	memset(&UxContext->serial, 0, sizeof(SysmanSerialArg));

	return(rtrn);
}

void
show_modifyserialdialog(
	Widget			parent,
	SysmanSerialArg*	serial,
	sysMgrMainCtxt * ctxt
)
{
	_UxCmodifyDialog       *UxContext;
	XmString		xstr;


	SetBusyPointer(True);

	if (modifyserialdialog == NULL)
		modifyserialdialog = create_modifyDialog(parent);
	
	ctxt->currDialog = modifyserialdialog;
        UxContext = (_UxCmodifyDialog *) UxGetContext( modifyserialdialog );

	free_serial(&UxContext->serial);
	copy_serial(&UxContext->serial, serial);

	if (modifyInit(modifyserialdialog, &UxContext->serial)) {
		UxPopupInterface(modifyserialdialog, no_grab);
	}

	SetBusyPointer(False);
}

/*******************************************************************************
       END OF FILE
*******************************************************************************/


