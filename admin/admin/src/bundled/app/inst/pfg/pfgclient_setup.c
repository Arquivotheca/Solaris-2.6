#ifndef lint
#pragma ident "@(#)pfgclient_setup.c 1.11 96/06/28 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgclient_setup.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include "pfgInstallClients_ui.h"

WidgetList	table_entry_widget_list;
Widget		table_item[50];
WidgetList	widget_list;
Widget		root_entry[50], swap_entry[50], swap_on_root_entry[50];
int		last;
char		NumClients[5], RootSize[5], SwapSize[5];
int		NumberOfClients, SwapPerClient, RootPerClient;
int		menu_choice;
Widget		Number, SSize, RSize;

static void	psAlignTableHeadings(void);
static void     do_alignmentCB(Widget, XtPointer, XtPointer);
void	verify_text_is_digit(Widget, XtPointer, XtPointer);
void	reset_to_none(Widget, XtPointer, XtPointer);
void	set_separate_swap(Widget, XtPointer, XtPointer);
void	fill_with_default_values(Widget, XtPointer, XtPointer);
void	show_root_only(Widget, XtPointer, XtPointer);
void	show_root_and_swap(Widget, XtPointer, XtPointer);
void	show_swap_on_root(Widget, XtPointer, XtPointer);
void	compute_size_total(Widget, XtPointer, XtPointer);
void	clientContinueCB(Widget, XtPointer, XtPointer);
void	clientGobackCB(Widget, XtPointer, XtPointer);
void	clientCancelCB(Widget, XtPointer, XtPointer);
void	clientExitCB(Widget, XtPointer, XtPointer);
void	clientHelpCB(Widget, XtPointer, XtPointer);
void	show_root_entry(Widget, XtPointer, XtPointer);
void	show_swap_entry(Widget, XtPointer, XtPointer);
void	show_root_and_swap_entries(Widget, XtPointer, XtPointer);
void	setSwapSize_traverse(Widget, XtPointer, XtPointer);
void	setSwapSize(Widget, XtPointer, XtPointer);
void	setRootSize_traverse(Widget, XtPointer, XtPointer);
void	setRootSize(Widget, XtPointer, XtPointer);
void	setNumberOfClients_traverse(Widget, XtPointer, XtPointer);
void	setNumberOfClients(Widget, XtPointer, XtPointer);
void	set_current_menu_choice(void);
void	set_last_menu_choice(unsigned int);
void	compute_root_total(char *, char *);
void	compute_swap_total(char *, char *);
void	doNumberOfClients(Widget);
void	doSwapSize(Widget);
void	doRootSize(Widget);

Widget
pfgCreateClientSetup(void)
{
	Widget		client_dialog;
	int		index;
	int		initial_entries;
	Widget		client_table, named_widget;
	char		clients[5];

/*
 * only want to create one entry for now
 *	initial_entries = 7;
 */
	initial_entries = 1;
	index = 0;

	menu_choice = LastServiceChoice;

	(void) sprintf(NumClients, "%d", DEFAULT_NUMBER_OF_CLIENTS);

	/* call generated function tu_clientDialog_widget to create screen */
	client_dialog =  tu_clientDialog_widget("clientDialog",
		pfgTopLevel, &widget_list);

	XmAddWMProtocolCallback(pfgShell(client_dialog), pfgWMDeleteAtom,
			(XtCallbackProc)pfgExit, NULL);

	XtVaSetValues(pfgShell(client_dialog),
		"mwmDecorations", MWM_DECOR_BORDER | MWM_DECOR_TITLE,
		XmNdeleteResponse, XmDO_NOTHING,
		XmNtitle, TITLE_CLIENTALLOC,
		XmNallowShellResize, True,
		NULL);
	XtAddCallback(pfgShell(client_dialog), XmNpopupCallback, 
		do_alignmentCB, (XtPointer)NULL);

	/* set the XmNlabelStrings for the client_dialog pushbuttons */
	pfgSetWidgetString(widget_list, "clientContinueButton", PFG_CONTINUE);
	pfgSetWidgetString(widget_list, "clientGobackButton", PFG_GOBACK);
	pfgSetWidgetString(widget_list, "clientCancelButton", PFG_CANCEL);
	pfgSetWidgetString(widget_list, "clientExitButton", PFG_EXIT);
	pfgSetWidgetString(widget_list, "clientHelpButton", PFG_HELP);

	/* set the XmNlabelStrings for the client_dialog column headings */
/*
 * change this for now
 *	pfgSetWidgetString(widget_list, "client_types_label", PFG_CL_SWAPSVC);
 */
	pfgSetWidgetString(widget_list, "platforms_label", PFG_CL_ROOTSVC);
	pfgSetWidgetString(widget_list, "client_types_label", PFG_SERVICE);
	pfgSetWidgetString(widget_list, "root_or_swap_label", PFG_CL_SVCTYPE);
	pfgSetWidgetString(widget_list, "numClients_label", PFG_CL_NUMCL);
	pfgSetWidgetString(widget_list, "blank_label1", PFG_CL_MULTIPLY);
	pfgSetWidgetString(widget_list, "size_per_label", PFG_CL_SIZEPER);
	pfgSetWidgetString(widget_list, "blank_label2", PFG_CL_EQUALS);
	pfgSetWidgetString(widget_list, "total_size_label", PFG_CL_TOTAL);
	pfgSetWidgetString(widget_list, "mount_point_label", PFG_CL_MNTPT);
/*
	Stuff to better test with...

	pfgSetWidgetString(widget_list, "root_or_swap_label", "Root Or Swap");
        pfgSetWidgetString(widget_list, "numClients_label", "Num clients");
        pfgSetWidgetString(widget_list, "blank_label1", "Multiply");
        pfgSetWidgetString(widget_list, "size_per_label", "Size");
        pfgSetWidgetString(widget_list, "blank_label2", "Equals");
        pfgSetWidgetString(widget_list, "total_size_label", "Total size");
        pfgSetWidgetString(widget_list, "mount_point_label", "Mount Point Label");
*/

	/* set the XmNvalue for the client_dialog text string */
	pfgSetWidgetString(widget_list, "panelhelpText", MSG_CLIENTSETUP);

	/* get the widget id of the main row column in the client_dialog */
	named_widget = pfgGetNamedWidget(widget_list, "main_table_rc");
	client_table = named_widget;

	last = 0;
	for (index = 0; index < initial_entries; index++) {
		/* create the first initial_entries entries in the table */
		table_item[index] =
			tu_client_setup_table_widget("client_setup_table",
			client_table, &table_entry_widget_list);
		root_entry[index] = pfgGetNamedWidget(table_entry_widget_list,
					"client_setup");
		swap_entry[index] = pfgGetNamedWidget(table_entry_widget_list,
					"swap_setup");
		swap_on_root_entry[index] =
			pfgGetNamedWidget(table_entry_widget_list,
			"s_on_r_setup");

/* *** take this out until 2.6 *** */
/*
		pfgSetWidgetString(table_entry_widget_list, "no_swap",
			PFG_NONE_CHOICE);
		pfgSetWidgetString(table_entry_widget_list, "sep_swap",
			PFG_SEPSWAP_CHOICE);
		pfgSetWidgetString(table_entry_widget_list, "swap_on_root",
			PFG_SWAPONROOT_CHOICE);
 */
		pfgSetWidgetString(table_entry_widget_list, "root_only",
			PFG_ROOT);
		pfgSetWidgetString(table_entry_widget_list, "swap_only",
			PFG_SWAP);
		pfgSetWidgetString(table_entry_widget_list, "root_and_swap",
			PFG_SWAPANDROOT);
		pfgSetWidgetString(table_entry_widget_list, "multiply",
			PFG_CL_MULTIPLY);
		pfgSetWidgetString(table_entry_widget_list, "equals",
			PFG_CL_EQUALS);
		pfgSetWidgetString(table_entry_widget_list, "swap_multiply",
			PFG_CL_MULTIPLY);
		pfgSetWidgetString(table_entry_widget_list, "swap_equals",
			PFG_CL_EQUALS);
		pfgSetWidgetString(table_entry_widget_list, "sonr_multiply",
			PFG_CL_MULTIPLY);
		pfgSetWidgetString(table_entry_widget_list, "sonr_equals",
			PFG_CL_EQUALS);
		pfgSetWidgetString(table_entry_widget_list, "root_label",
			PFG_ROOT_LABEL);
		pfgSetWidgetString(table_entry_widget_list, "swap_label",
			PFG_SWAP_LABEL);
		pfgSetWidgetString(table_entry_widget_list, "sonr_label",
			PFG_SWAP_LABEL);
		pfgSetWidgetString(table_entry_widget_list, "root_mount_point",
			"/export/root");

		NumberOfClients = getNumClients();
		SwapPerClient = getSwapPerClient();
		RootPerClient = getRootPerClient();
		/* set the default number of clients */
		(void) sprintf(clients, "%d", NumberOfClients);
		(void) sprintf(NumClients, "%s", clients);
		Number = pfgGetNamedWidget(table_entry_widget_list,
			"numClients");
		pfgSetWidgetString(table_entry_widget_list, "numClients",
			clients);
		/* set the default root size */
		RSize = pfgGetNamedWidget(table_entry_widget_list,
			"root_size_per");
		(void) sprintf(RootSize, "%d", RootPerClient);
		pfgSetWidgetString(table_entry_widget_list, "root_size_per",
			RootSize);
		/* set the default swap size */
		SSize = pfgGetNamedWidget(table_entry_widget_list,
			"swap_size_per");
		(void) sprintf(SwapSize, "%d", SwapPerClient);
		pfgSetWidgetString(table_entry_widget_list, "swap_size_per",
			SwapSize);
		compute_root_total(clients, RootSize);
		compute_swap_total(clients, SwapSize);

		/*
		 *    Last must be incremented here. 
		 *    set_current_menu_choice uses it.
		 */
		last++;
		set_current_menu_choice();


		if (debug) {
			(void) printf("number of clients is %d\n",
				NumberOfClients);
			(void) printf("swap size is %d\n", SwapPerClient);
			(void) printf("root size is %d\n", RootPerClient);
		}
	}


	XtManageChild(client_dialog);

	(void) XmProcessTraversal(
		pfgGetNamedWidget(widget_list, "clientContinueButton"),
		XmTRAVERSE_CURRENT);

	pfgUnbusy(XtParent(client_dialog));

	return (client_dialog);


}

void
set_last_menu_choice(unsigned int which_choice)
{
	LastServiceChoice = which_choice;
}

void
set_current_menu_choice(void)
{
	Widget last_widget, client_menu;
	Widget root_choice, swap_choice, both_choice;
	unsigned int last_choice;

	root_choice = pfgGetNamedWidget(table_entry_widget_list, "root_only");
	swap_choice = pfgGetNamedWidget(table_entry_widget_list, "swap_only");
	both_choice =
		pfgGetNamedWidget(table_entry_widget_list, "root_and_swap");
	client_menu =
		pfgGetNamedWidget(table_entry_widget_list, "client_types");

	last_choice = LastServiceChoice;

	if (last_choice == 0)
		last_widget = both_choice;
	if (last_choice == BOTH_CHOICE)
		last_widget = both_choice;
	if (last_choice == ROOT_CHOICE)
		last_widget = root_choice;
	if (last_choice == SWAP_CHOICE)
		last_widget = swap_choice;


	if (last_widget == root_choice) {
		XtVaSetValues(client_menu,
			XmNmenuHistory, root_choice,
			NULL);
		show_root_entry(root_choice, NULL, NULL);
		menu_choice = ROOT_CHOICE;
	} else if (last_widget == swap_choice) {
		XtVaSetValues(client_menu,
			XmNmenuHistory, swap_choice,
			NULL);
		show_swap_entry(swap_choice, NULL, NULL);
		menu_choice = SWAP_CHOICE;
	} else {
		XtVaSetValues(client_menu,
			XmNmenuHistory, both_choice,
			NULL);
		show_root_and_swap_entries(both_choice, NULL, NULL);
		menu_choice = BOTH_CHOICE;
	}

}


static void
psAlignTableHeadings(void)
{
	Dimension	margin_width;
	Dimension	spacing;
	Dimension	offset;
	Widget		temp_widget[5];
	Widget		label_widget;

	/*
	 * get the width of the widget, then set the width of the
	 * center aligned label to the width of the widget
	 */

	temp_widget[0] = pfgGetNamedWidget(widget_list, "clientDialog");
	XtVaGetValues(temp_widget[0],
		XmNmarginWidth, &margin_width,
		NULL);

	temp_widget[0] =
		pfgGetNamedWidget(table_entry_widget_list, "client_setup");
	XtVaGetValues(temp_widget[0],
		XmNhorizontalSpacing, &spacing,
		NULL);

	offset = spacing;

/*
 * take this out until 2.6
 *
 *	temp_widget = pfgGetNamedWidget(table_entry_widget_list, "platforms");
 *	XtVaGetValues(temp_widget,
 *		XmNwidth, &width,
 *		NULL);
 *	label_widget = pfgGetNamedWidget(widget_list, "platforms_label");
 *	XtVaSetValues(label_widget,
 *		XmNwidth, width,
 *		XmNleftOffset, margin_width,
 *		NULL);
 */
	temp_widget[0] =
		pfgGetNamedWidget(table_entry_widget_list, "client_types_frame");
	temp_widget[1] = NULL;
	label_widget = pfgGetNamedWidget(widget_list, "client_types_label");
	xm_AlignWidgetCols(label_widget, temp_widget);
	XtVaSetValues(label_widget,
		XmNleftOffset, offset,
		NULL);

	temp_widget[0] = 
		pfgGetNamedWidget(table_entry_widget_list, "root_label");
	temp_widget[1] =
		pfgGetNamedWidget(table_entry_widget_list, "swap_label");
        temp_widget[2] = NULL;
	label_widget = pfgGetNamedWidget(widget_list, "root_or_swap_label");
	xm_AlignWidgetCols(label_widget, temp_widget);
	XtVaSetValues(label_widget,
		XmNleftOffset, offset,
		NULL);

	temp_widget[0] = 
		pfgGetNamedWidget(table_entry_widget_list, "numClients");
	temp_widget[1] = 
		pfgGetNamedWidget(table_entry_widget_list, "swap_numClients");
	temp_widget[2] = NULL;
	label_widget = pfgGetNamedWidget(widget_list, "numClients_label");
	xm_AlignWidgetCols(label_widget, temp_widget);
	XtVaSetValues(label_widget,
		XmNleftOffset, offset,
		NULL);

	temp_widget[0] = 
		pfgGetNamedWidget(table_entry_widget_list, "multiply");
	temp_widget[1] = 
		pfgGetNamedWidget(table_entry_widget_list, "swap_multiply");
	temp_widget[2] = NULL;
	label_widget = pfgGetNamedWidget(widget_list, "blank_label1");
	xm_AlignWidgetCols(label_widget, temp_widget);
	XtVaSetValues(label_widget,
		XmNleftOffset, offset,
		NULL);

	temp_widget[0] =
		pfgGetNamedWidget(table_entry_widget_list, "root_size_per");
	temp_widget[1] =
		pfgGetNamedWidget(table_entry_widget_list, "swap_size_per");
	temp_widget[2] = NULL;
	label_widget = pfgGetNamedWidget(widget_list, "size_per_label");
	xm_AlignWidgetCols(label_widget, temp_widget);
	XtVaSetValues(label_widget,
		XmNleftOffset, offset,
		NULL);


	temp_widget[0] = 
		pfgGetNamedWidget(table_entry_widget_list, "equals");
	temp_widget[1] = 
		pfgGetNamedWidget(table_entry_widget_list, "swap_equals");
	temp_widget[2] = NULL;
	label_widget = pfgGetNamedWidget(widget_list, "blank_label2");
	xm_AlignWidgetCols(label_widget, temp_widget);
	XtVaSetValues(label_widget,
		XmNleftOffset, offset,
		NULL);

	temp_widget[0] =
		pfgGetNamedWidget(table_entry_widget_list, "total_root_size");
	temp_widget[1] =
		pfgGetNamedWidget(table_entry_widget_list, "total_swap_size");
	temp_widget[2] = NULL;
	label_widget = pfgGetNamedWidget(widget_list, "total_size_label");
	xm_AlignWidgetCols(label_widget, temp_widget);
	XtVaSetValues(label_widget,
		XmNleftOffset, offset,
		NULL);

	temp_widget[0] =
		pfgGetNamedWidget(table_entry_widget_list, "root_mount_point");
	temp_widget[1] =
		pfgGetNamedWidget(table_entry_widget_list, "swap_mount_point");
	temp_widget[2] = NULL;
	label_widget = pfgGetNamedWidget(widget_list, "mount_point_label");
	xm_AlignWidgetCols(label_widget, temp_widget);
	XtVaSetValues(label_widget,
		XmNleftOffset, offset,
		NULL);
}

/* ARGSUSED */
static void
do_alignmentCB(Widget swap, XtPointer clientD, XtPointer callD)
{
	psAlignTableHeadings();

	xm_SizeScrolledWindowToWorkArea(
		pfgGetNamedWidget(widget_list, "client_scroll"), True, False);

}


void
compute_root_total(char *number, char *size)
{
	int	number_of_clients, per_client_size;
	long	total;
	char	buf[12];

	number_of_clients = atoi(number);
	per_client_size = atoi(size);
	total = number_of_clients * per_client_size;
	(void) sprintf(buf, "%ld", total);
	pfgSetWidgetString(table_entry_widget_list, "total_root_size", buf);
}

void
compute_swap_total(char *number, char *size)
{
	int	number_of_clients, per_client_size;
	long	total;
	char	buf[12];

	number_of_clients = atoi(number);
	per_client_size = atoi(size);
	total = number_of_clients * per_client_size;
	(void) sprintf(buf, "%ld", total);
	pfgSetWidgetString(table_entry_widget_list, "total_swap_size", buf);
}

void
doSwapSize(Widget swap_size)
{
	char *string;
	int swapSize;
	Widget	client_number;
	char	*num_clients;

	if (debug)
		(void) printf("calling doSwapSize\n");
	string = XmTextFieldGetString(swap_size);
	client_number =
		pfgGetNamedWidget(table_entry_widget_list, "numClients");
	num_clients = XmTextFieldGetString(client_number);

	if (string[0] == '/0') {
		swapSize = 0;
	} else {
		swapSize = atoi(string);
	}

	if (debug)
		(void) printf("\tswap size = %d\n", swapSize);

	compute_swap_total(num_clients, string);
	(void) sprintf(SwapSize, "%s", string);
	setSwapPerClient(swapSize);
	SwapPerClient = swapSize;

}

/* ARGSUSED */
void
setSwapSize_traverse(Widget swap, XtPointer clientD, XtPointer callD)
{

	if (debug)
		(void) printf("calling setSwapSize_traverse\n");
	doSwapSize(swap);

	/* move to the next widget in the tab group */
	XmProcessTraversal(Number, XmTRAVERSE_CURRENT);
}

/* ARGSUSED */
void
setSwapSize(Widget swap, XtPointer clientD, XtPointer callD)
{

	if (debug)
		(void) printf("calling setSwapSize\n");
	doSwapSize(swap);

	/* do not move to the next widget */
}

void
doRootSize(Widget root_size)
{
	char *string;
	int rootSize;
	Widget client_number;
	char *num_clients;

	if (debug)
		(void) printf("calling doRootSize\n");

	string = XmTextFieldGetString(root_size);
	client_number =
		pfgGetNamedWidget(table_entry_widget_list, "numClients");
	num_clients = XmTextFieldGetString(client_number);

	if (string[0] == '\0') {
		rootSize = 0;
	} else {
		rootSize = atoi(string);
	}

	if (debug)
		(void) printf("\troot size = %d\n", rootSize);

	compute_root_total(num_clients, string);
	(void) sprintf(RootSize, "%s", string);
	setRootPerClient(rootSize);
	RootPerClient = rootSize;

}

/* ARGSUSED */
void
setRootSize_traverse(Widget root, XtPointer clientD, XtPointer callD)
{

	if (debug)
		(void) printf("calling setRootSize_traverse\n");
	doRootSize(root);

	/* move to the next widget in the tab group */
	if (XmIsTraversable(SSize) == True) {
		XmProcessTraversal(SSize, XmTRAVERSE_CURRENT);
	} else if (XmIsTraversable(Number) == True) {
		XmProcessTraversal(Number, XmTRAVERSE_CURRENT);
	}

}

/* ARGSUSED */
void
setRootSize(Widget root, XtPointer clientD, XtPointer callD)
{

	if (debug)
		(void) printf("calling setRootSize\n");
	doRootSize(root);

	/* do not move to the next widget */

}


/* ARGSUSED */
void
verify_text_is_digit(Widget w, XtPointer clientD, XtPointer callD)
{
	int i;

	/* LINTED [pointer cast] */
	XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *) callD;


	for (i = 0; i < cbs->text->length; i++) {
		if (!isdigit(cbs->text->ptr[i])) {
			cbs->doit = False;
			if (debug) {
				(void) printf("for shame - enter a number\n");
			}
			break;
		}
	}
}

void
doNumberOfClients(Widget number)
{
	char *string;
	int numClientsSize;
	Widget client_size, client_size1;
	char *root_size, *swap_size;

	if (debug)
		(void) printf("calling doNumberOfClients\n");

	string = XmTextFieldGetString(number);
	client_size =
		pfgGetNamedWidget(table_entry_widget_list, "root_size_per");
	root_size = XmTextFieldGetString(client_size);
	client_size1 =
		pfgGetNamedWidget(table_entry_widget_list, "swap_size_per");
	swap_size = XmTextFieldGetString(client_size1);
	if (string[0] == '\0') {
		numClientsSize = 0;
	} else {
		numClientsSize = atoi(string);
	}
	compute_root_total(string, root_size);
	compute_swap_total(string, swap_size);
	/* the values gotten from the number of clients field */
	(void) sprintf(NumClients, "%s", string);
	setNumClients(numClientsSize);

}

/* ARGSUSED */
void
setNumberOfClients_traverse(
	Widget num_clients, XtPointer clientD, XtPointer callD)
{
	if (debug)
		(void) printf("calling setNumberOfClients_traverse\n");

	doNumberOfClients(num_clients);

	/* move to the next widget */
	XmProcessTraversal(RSize, XmTRAVERSE_CURRENT);

}

/* ARGSUSED */
void
setNumberOfClients(Widget num_clients, XtPointer clientD, XtPointer callD)
{

	if (debug)
		(void) printf("calling setNumberOfClients\n");
	doNumberOfClients(num_clients);

	/* do not move to the next widget */

}


/* ARGSUSED */
void
show_root_entry(Widget w, XtPointer clientD, XtPointer callD)
{
	Boolean found;
	int i, found_index;
	char		clients[5];

	menu_choice = ROOT_CHOICE;

	if (debug) {
		(void) printf(" the current widget is of type %s\n", XtName(w));
	}

	/*
	 * we want the widget id of the form that contains the
	 * child from which the callback was initiated, three
	 * levels of XtParent give up the correct widget id from
	 * the widget hierarchy, if the form is ever put into a different
	 * parent, it may be necessary to add yet another XtParent level
	 * to this test
	 */

	found = False;
	for (i = 0; i < last; i++) {
		if(xm_IsDescendent(root_entry[i], w)) {
			found_index = i;
			found = True;
		}
	}

	if (found == True) {
		if (debug) {
			(void) printf(" the parent widget is of type %s\n",
			XtName(root_entry[found_index]));
		}

		if (XtIsManaged(swap_entry[found_index]) == True) {
			XtUnmanageChild(swap_entry[found_index]);
		}
		/*
		 * just change the label on the root entry to Root
		 */
		pfgSetWidgetString(table_entry_widget_list, "root_label",
			PFG_ROOT_LABEL);
		/* set the default mount point to /export/root */
		pfgSetWidgetString(table_entry_widget_list, "root_mount_point",
			"/export/root");
		/* set the default number of clients */
		(void) sprintf(clients, "%s", NumClients);
		pfgSetWidgetString(table_entry_widget_list, "numClients",
			clients);
		/* set the default root size */
		(void) sprintf(RootSize, "%d", RootPerClient);
		pfgSetWidgetString(table_entry_widget_list, "root_size_per",
			RootSize);
		compute_root_total(clients, RootSize);
	}

	if (debug) {
		(void) printf("show root entry:\n");
		(void) printf("\troot size = %d\n", getRootPerClient());
		(void) printf("\tswap size = %d\n", getSwapPerClient());
	}
}

/* ARGSUSED */
void
show_swap_entry(Widget w, XtPointer clientD, XtPointer callD)
{
	Boolean		found;
	int		i, found_index;
	char		clients[5];

	menu_choice = SWAP_CHOICE;

	found = False;
	for (i = 0; i < last; i++) {
		if(xm_IsDescendent(root_entry[i], w)) {
			found_index = i;
			found = True;
		}
	}

	if (found == True) {

		if (XtIsManaged(swap_entry[found_index]) == True) {
			XtUnmanageChild(swap_entry[found_index]);
		}
		/*
	 	* just change the label on the root entry to Swap
	 	*/
		pfgSetWidgetString(table_entry_widget_list, "root_label",
			PFG_SWAP_LABEL);
		pfgSetWidgetString(table_entry_widget_list, "root_mount_point",
			"/export/swap");
		/* set default number of clients to the last number entered */
		(void) sprintf(clients, "%s", NumClients);
		pfgSetWidgetString(table_entry_widget_list, "numClients",
			clients);
		/* set the default swap size */
		(void) sprintf(SwapSize, "%d", SwapPerClient);
		pfgSetWidgetString(table_entry_widget_list, "root_size_per",
			SwapSize);
		compute_root_total(clients, SwapSize);
	}

	if (debug) {
		(void) printf("show swap entry:\n");
		(void) printf("\troot size = %d\n", getRootPerClient());
		(void) printf("\tswap size = %d\n", getSwapPerClient());
	}
}

/* ARGSUSED */
void
show_root_and_swap_entries(Widget w, XtPointer clientD, XtPointer callD)
{
	Boolean found;
	int i, found_index;
	char		clients[5];

	menu_choice = BOTH_CHOICE;

	if (debug) {
		(void) printf(" the current widget is of type %s\n", XtName(w));
	}

	/*
	 * we want the widget id of the form that contains the
	 * child from which the callback was initiated, three
	 * levels of XtParent give up the correct widget id from
	 * the widget hierarchy, if the form is ever put into a different
	 * parent, it may be necessary to add yet another XtParent level
	 * to this test
	 */

	found = False;
	for (i = 0; i < last; i++) {
		if (debug) 
			(void) printf("show_root_and_swap_entries(): checking if %s is descendent of %s\n", XtName(root_entry[i]), XtName(w));
		if(xm_IsDescendent(root_entry[i], w)) {
			found_index = i;
			found = True;
		}
	}

	if (debug)
		printf("show_root_and_swap_entries(): found = %d\n", found);

	if (found == True) {
		if (debug) {
			(void) printf(" the parent widget is of type %s\n",
			XtName(root_entry[found_index]));
		}

		pfgSetWidgetString(table_entry_widget_list, "root_label",
			PFG_ROOT_LABEL);
		pfgSetWidgetString(table_entry_widget_list, "root_mount_point",
					"/export/root");
		pfgSetWidgetString(table_entry_widget_list, "swap_label",
			PFG_SWAP_LABEL);
		pfgSetWidgetString(table_entry_widget_list, "swap_mount_point",
					"/export/swap");
		/* set default number of clients to the last number entered */
		(void) sprintf(clients, "%s", NumClients);
		pfgSetWidgetString(table_entry_widget_list, "numClients",
			clients);
		/* set the default swap size */
		(void) sprintf(SwapSize, "%d", SwapPerClient);
		pfgSetWidgetString(table_entry_widget_list, "swap_size_per",
					SwapSize);
		/* set the default root size */
		(void) sprintf(RootSize, "%d", RootPerClient);
		pfgSetWidgetString(table_entry_widget_list, "root_size_per",
					RootSize);
		compute_root_total(clients, RootSize);
		compute_swap_total(clients, SwapSize);

		if (XtIsManaged(root_entry[found_index]) == False) {
			XtManageChild(root_entry[found_index]);
		}
		if (XtIsManaged(swap_entry[found_index]) == False) {
			XtManageChild(swap_entry[found_index]);
		}
	}

	if (debug) {
		(void) printf("show root and swap entry:\n");
		(void) printf("\troot size = %d\n", getRootPerClient());
		(void) printf("\tswap size = %d\n", getSwapPerClient());
	}
}

/* ARGSUSED */
void
clientContinueCB(Widget w, XtPointer clientD, XtPointer callD)
{
	Widget num_clients;
	char	*clients;
	int	number_of_clients;

	if (debug) {
		(void) printf("start continue CB:\n");
		(void) printf("\troot size = %d\n", getRootPerClient());
		(void) printf("\tswap size = %d\n", getSwapPerClient());
	}

	/* number of clients */
	num_clients = pfgGetNamedWidget(table_entry_widget_list, "numClients");
	clients = XmTextFieldGetString(num_clients);
	number_of_clients = atoi(clients);
	setNumClients(number_of_clients);

	/*
	 * it is not necessary to get root size and swap size out of the
	 * widgets at this point.  They are already correctly set given
	 * that they are set correctly in all activate and losing focus
	 * callbacks...
	 */

	set_last_menu_choice(menu_choice);

	pfgBusy(pfgShell(w));
	/* free the widget lists used by this window */
	free(table_entry_widget_list);
	free(widget_list);
	pfgSetAction(parAContinue);

	if (debug) {
		(void) printf("end continue CB:\n");
		(void) printf("\troot size = %d\n", getRootPerClient());
		(void) printf("\tswap size = %d\n", getSwapPerClient());
	}
}

/* ARGSUSED */
void
clientGobackCB(Widget w, XtPointer clientD, XtPointer callD)
{
	pfgBusy(pfgShell(w));
	set_last_menu_choice(menu_choice);
	/* free the widget lists used by this window */
	free(table_entry_widget_list);
	free(widget_list);
	pfgSetAction(parAGoback);
}

/* ARGSUSED */
void
clientCancelCB(Widget w, XtPointer clientD, XtPointer callD)
{
	char		clients[5], root_client_size[5], swap_client_size[5];

	/* reset the values to their defaults */

	NumberOfClients = DEFAULT_NUMBER_OF_CLIENTS;
	RootPerClient = DEFAULT_ROOT_PER_CLIENT;
	SwapPerClient = DEFAULT_SWAP_PER_CLIENT;

	/* reset the table entries to their default values */
	pfgSetWidgetString(table_entry_widget_list, "root_label",
		PFG_ROOT_LABEL);
	pfgSetWidgetString(table_entry_widget_list, "root_mount_point",
				"/export/root");
	pfgSetWidgetString(table_entry_widget_list, "swap_label",
		PFG_SWAP_LABEL);
	pfgSetWidgetString(table_entry_widget_list, "swap_mount_point",
				"/export/swap");
	/* set the default number of clients to the last number entered */
	(void) sprintf(clients, "%d", NumberOfClients);
	(void) sprintf(NumClients, "%s", clients);
	pfgSetWidgetString(table_entry_widget_list, "numClients", clients);
	/* set the default swap size */
	(void) sprintf(swap_client_size, "%d", SwapPerClient);
	pfgSetWidgetString(table_entry_widget_list, "swap_size_per",
				swap_client_size);
	/* set the default root size */
	(void) sprintf(root_client_size, "%d", RootPerClient);
	pfgSetWidgetString(table_entry_widget_list, "root_size_per",
				root_client_size);
	compute_root_total(clients, root_client_size);
	compute_swap_total(clients, swap_client_size);

}

/* ARGSUSED */
void
clientExitCB(Widget w, XtPointer clientD, XtPointer callD)
{
	pfgExit();
}

/* ARGSUSED */
void
reset_to_none(Widget w, XtPointer clientD, XtPointer callD)
{
}

/* ARGSUSED */
void
set_separate_swap(Widget w, XtPointer clientD, XtPointer callD)
{
}

/* ARGSUSED */
void
fill_with_default_values(Widget w, XtPointer clientD, XtPointer callD)
{
}

/* ARGSUSED */
void
show_root_only(Widget w, XtPointer clientD, XtPointer callD)
{
	Boolean found;
	int i, found_index;

	if (debug) {
		(void) printf(" the current widget is of type %s\n", XtName(w));
	}

	/*
	 * we want the widget id of the form that contains the
	 * child from which the callback was initiated, three
	 * levels of XtParent give up the correct widget id from
	 * the widget hierarchy, if the form is ever put into a different
	 * parent, it may be necessary to add yet another XtParent level
	 * to this test
	 */

	found = False;
	for (i = 0; i < last; i++) {
		if(xm_IsDescendent(root_entry[i], w)) {
			found_index = i;
			found = True;
		}
	}


	if (found == True) {
		if (debug) {
			(void) printf(" the parent widget is of type %s\n",
			XtName(root_entry[found_index]));
		}

		if (XtIsManaged(swap_on_root_entry[found_index]) == True) {
			XtUnmanageChild(swap_on_root_entry[found_index]);
		}
		if (XtIsManaged(swap_entry[found_index]) == True) {
			XtUnmanageChild(swap_entry[found_index]);
		}
	}
}

/* ARGSUSED */
void
show_root_and_swap(Widget w, XtPointer clientD, XtPointer callD)
{
	Boolean found;
	int i, found_index;

	if (debug) {
		(void) printf(" the current widget is of type %s\n", XtName(w));
	}

	/*
	 * we want the widget id of the form that contains the
	 * child from which the callback was initiated, three
	 * levels of XtParent give up the correct widget id from
	 * the widget hierarchy, if the form is ever put into a different
	 * parent, it may be necessary to add yet another XtParent level
	 * to this test
	 */

	found = False;
	for (i = 0; i < last; i++) {
		if(xm_IsDescendent(root_entry[i], w)) {
			found_index = i;
			found = True;
		}
	}


	if (found == True) {
		if (debug) {
			(void) printf(" the parent widget is of type %s\n",
			XtName(root_entry[found_index]));
		}

		XtManageChild(swap_entry[found_index]);
		if (XtIsManaged(swap_on_root_entry[found_index]) == True) {
			XtUnmanageChild(swap_on_root_entry[found_index]);
		}
	}
}

/* ARGSUSED */
void
show_swap_on_root(Widget w, XtPointer clientD, XtPointer callD)
{
	Boolean found;
	int i, found_index;

	if (debug) {
		(void) printf(" the current widget is of type %s\n", XtName(w));
	}

	/*
	 * we want the widget id of the form that contains the
	 * child from which the callback was initiated, three
	 * levels of XtParent give up the correct widget id from
	 * the widget hierarchy, if the form is ever put into a different
	 * parent, it may be necessary to add yet another XtParent level
	 * to this test
	 */

	found = False;
	for (i = 0; i < last; i++) {
		if(xm_IsDescendent(root_entry[i], w)) {
			found_index = i;
			found = True;
		}
	}


	if (found == True) {
		if (debug) {
			(void) printf(" the parent widget is of type %s\n",
			XtName(root_entry[found_index]));
		}

		XtManageChild(swap_on_root_entry[found_index]);
		if (XtIsManaged(swap_entry[found_index]) == True) {
			XtUnmanageChild(swap_entry[found_index]);
		}
	}
}

/* ARGSUSED */
void
compute_size_total(Widget w, XtPointer clientD, XtPointer callD)
{
}
