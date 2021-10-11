
#pragma ident "@(#)util.c	1.44 96/09/19 Sun Microsystems"

/*	util.c	*/


#include <stdio.h>
#include <stdarg.h>
#include <nl_types.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/SelectioB.h>
#include <Xm/MessageB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#include <Xm/PushB.h>

#include <wchar.h>
#include <widec.h>
#include <ctype.h>

#include "adminhelp.h"

#include "UxXt.h"
#include "util.h"
#include "sysman_iface.h"

#ifdef USE_CSI
/*
 * Note: Even strwidth() is incorporated into Solaris 2.5, it may be
 * Sun only.  Thus, if this program (admintool and its applications)
 * goes to CDE, convert s to wide char string and use wcswidth(ws).
 *
 * wchar_t	wchar;
 *
 * wchar = allocate 4 * (strlen(from_ptr) + 1)
 * (void)mbtowc(&wchar, from_ptr, MB_CUR_MAX);
 * column_count += wcwidth(wchar);
 */
extern int 			strwidth(char *);
#define STRWIDTH(s)		strwidth(s)
#else
#include <euc.h>
/* extern int 			eucscol(char *); */
/* extern int 			euccol(char *); */
/* #define STRWIDTH(s)		eucscol(s) */
/* #define EUCCOL(c)		euccol(c) */
#endif

typedef struct {
	int	button;
	char	*sel;
} response;

static char* filter_errmsg(char *);

#ifdef OLD_FILTER
static char* fit_width(char *, int);
static char* copy_and_format(char *, int);
#else
static char* filter_errmsg(char *);
static wchar_t* _fit_width(wchar_t *, int);
static wchar_t* _copy_and_format(wchar_t *, int);
#endif

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

extern Widget sysmgrmain;
extern Widget adduserdialog;
extern Widget addgroupdialog;
extern Widget addhostdialog;
extern Widget addlocaldialog;
extern Widget addremotedialog;
extern Widget addsoftwaredialog;
extern Widget modifyuserdialog;
extern Widget modifygroupdialog;
extern Widget modifyhostdialog;
extern Widget modifyprinterdialog;
extern Widget modifyserialdialog;
extern Cursor busypointer;

#ifdef SW_INSTALLER

static Widget* windowlist[] = {
	&addsoftwaredialog,
	NULL
};

#else

static Widget* windowlist[] = {
	&sysmgrmain,
	&adduserdialog,
	&addgroupdialog,
	&addhostdialog,
	&addlocaldialog,
	&addremotedialog,
	&addsoftwaredialog,
	&modifyuserdialog,
	&modifygroupdialog,
	&modifyhostdialog,
	&modifyprinterdialog,
	&modifyserialdialog,
	NULL
};

#endif

static Widget	errordialog;
static Widget   infoDialog;

char	errbuf[ERRBUF_SIZE];

/*********************************************************************/
/*********************************************************************/

void
free_mem(void* ptr)
{
	if (ptr != NULL)
		free((char*)ptr);
}

void
copy_user(
	SysmanUserArg* d,
	SysmanUserArg* s
)
{
	d->username = s->username ? strdup(s->username) : NULL;
	d->uid = s->uid ? strdup(s->uid) : NULL;
	d->comment = s->comment ? strdup(s->comment) : NULL;
	d->username_key = s->username ? strdup(s->username) : NULL;
}

void
free_user(
	SysmanUserArg* u_ptr
)
{
	free_mem((void*)u_ptr->username);
	free_mem((void*)u_ptr->uid);
	free_mem((void*)u_ptr->comment);
	free_mem((void*)u_ptr->username_key);
}

void
copy_group(
	SysmanGroupArg* d,
	SysmanGroupArg* s
)
{
	d->groupname = s->groupname ? strdup(s->groupname) : NULL;
	d->gid = s->gid ? strdup(s->gid) : NULL;
	d->members = s->members ? strdup(s->members): NULL;
	d->groupname_key = s->groupname ? strdup(s->groupname) : NULL;
	d->gid_key = s->gid ? strdup(s->gid) : NULL;
}

void
free_group(
	SysmanGroupArg* g_ptr
)
{
	free_mem((void*)g_ptr->groupname);
	free_mem((void*)g_ptr->gid);
	free_mem((void*)g_ptr->members);
	free_mem((void*)g_ptr->groupname_key);
	free_mem((void*)g_ptr->gid_key);
}

void
copy_host(
	SysmanHostArg* d,
	SysmanHostArg* s
)
{
	d->hostname = s->hostname ? strdup(s->hostname) : NULL;
	d->ipaddr = s->ipaddr ? strdup(s->ipaddr) : NULL;
	d->hostname_key = s->hostname ? strdup(s->hostname) : NULL;
	d->ipaddr_key = s->ipaddr ? strdup(s->ipaddr) : NULL;
}

void
free_host(
	SysmanHostArg* h_ptr
)
{
	free_mem((void*)h_ptr->hostname);
	free_mem((void*)h_ptr->ipaddr);
	free_mem((void*)h_ptr->hostname_key);
	free_mem((void*)h_ptr->ipaddr_key);
}

void
copy_printer(
	SysmanPrinterArg* d,
	SysmanPrinterArg* s
)
{
	d->printername = s->printername ? strdup(s->printername) : NULL;
	d->printertype = s->printertype ? strdup(s->printertype) : NULL;
	d->printserver = s->printserver ? strdup(s->printserver) : NULL;
	d->file_contents = s->file_contents ? strdup(s->file_contents) : NULL;
	d->comment = s->comment ? strdup(s->comment) : NULL;
	d->device = s->device ? strdup(s->device) : NULL;
	d->notify = s->notify ? strdup(s->notify) : NULL;
	d->protocol = s->protocol ? strdup(s->protocol) : NULL;
	d->num_restarts = s->num_restarts;
	d->default_p = s->default_p;
	d->banner_req_p = s->banner_req_p;
	d->enable_p = s->enable_p;
	d->accept_p = s->accept_p;
	d->user_allow_list =
	    s->user_allow_list ? strdup(s->user_allow_list) : NULL;
}

void
free_printer(
	SysmanPrinterArg* p_ptr
)
{
	free_mem((void*)p_ptr->printername);
	free_mem((void*)p_ptr->printertype);
	free_mem((void*)p_ptr->printserver);
	free_mem((void*)p_ptr->file_contents);
	free_mem((void*)p_ptr->comment);
	free_mem((void*)p_ptr->device);
	free_mem((void*)p_ptr->notify);
	free_mem((void*)p_ptr->protocol);
	free_mem((void*)p_ptr->user_allow_list);
}

void
copy_serial(
	SysmanSerialArg* d,
	SysmanSerialArg* s
)
{
	d->port = s->port ? strdup(s->port) : NULL;
	d->pmtag = s->pmtag ? strdup(s->pmtag) : NULL;
	d->svctag = s->svctag ? strdup(s->svctag) : NULL;
	d->comment = s->comment ? strdup(s->comment) : NULL;
	d->pmtag_key = s->pmtag ? strdup(s->pmtag) : NULL;
	d->svctag_key = s->svctag ? strdup(s->svctag) : NULL;
	d->service_enabled = s->service_enabled;
}

void
free_serial(
	SysmanSerialArg* s_ptr
)
{
	free_mem((void*)s_ptr->port);
	free_mem((void*)s_ptr->pmtag);
	free_mem((void*)s_ptr->svctag);
	free_mem((void*)s_ptr->comment);
	free_mem((void*)s_ptr->pmtag_key);
	free_mem((void*)s_ptr->svctag_key);
}

void
copy_software(
	SysmanSWArg* d,
	SysmanSWArg* s
)
{
}

void
free_software(
	SWStruct* sw_ptr
)
{
	int i;

	free_mem((void*)sw_ptr->sw_name);
	free_mem((void*)sw_ptr->sw_id);
	free_mem((void*)sw_ptr->version);
	free_mem((void*)sw_ptr->desc);
	free_mem((void*)sw_ptr->category);
	free_mem((void*)sw_ptr->arch);
	free_mem((void*)sw_ptr->date);
	free_mem((void*)sw_ptr->vendor);
	free_mem((void*)sw_ptr->prodname);
	free_mem((void*)sw_ptr->prodvers);
	free_mem((void*)sw_ptr->basedir);
	free_mem((void*)sw_ptr->locale);
	i = 0;
	while (installFileSystems[i])  {
	    free_mem((void*)sw_ptr->install_reqs[i].mountp);
	    i++;
	}
	free_mem((void*)sw_ptr->install_reqs);
}

void
SetBusyPointer(
	Boolean	busystate
)
{
	Widget** wgt;
	Window  win;

	for (wgt=windowlist; *wgt != NULL; wgt++) {
		if (**wgt && XtIsRealized(**wgt)) {
			win = XtWindow(**wgt);
			if (busystate)
        			XDefineCursor(Gdisplay, win, busypointer);
			else
        			XUndefineCursor(Gdisplay, win);
		}
	}

	XFlush(Gdisplay);
}

Widget
get_shell_ancestor(Widget w)
{
	while (w && !XtIsWMShell(w))
		w = XtParent(w);
	return (w);
}

int
min2 (
	int a, 
	int b)
{
	return (a < b ? a : b);
}

void
stringcopy (
	char * dest, 
	char * src, 
	int maxlen)
{
	if (dest == NULL) {
		(void)fprintf(stderr, 
		"Attempt to copy a string into a null pointer. Aborting.\n");
		abort();
	}

	if (src == NULL) {
		dest[0] = NULL;
	}
	else {
		(void)strncpy (dest, src, maxlen);
		dest[maxlen-1] = NULL;
	}
}

void
format_the_date (
	int	day, 
	int	month, 
	int	year, 
	char *	result_p)
{
	if (day == 0 || month == 0 || year == 0) {
		stringcopy (result_p, "", 10);
	}
	else {
		(void)sprintf (result_p, "%2.2d%2.2d%4.4d", day, month, year);
	}
}

static void
warningCB(
	Widget w,
	int* answer,
	XmAnyCallbackStruct* cbs)
{
	switch (cbs->reason) {
		case XmCR_OK:
			*answer = 1;
			break;
		case XmCR_CANCEL:
			*answer = 0;
			break;
	}
}

int
Confirm(Widget parent, char* msg, int* del_homedir, char* verb)
{
	static int	answer;
	Widget		dialog;
	XmString	text;
	XmString	xstr;
	int		j=0;
	Widget		listtext;
	Widget		cbox;
	Widget		del_toggle;
	Arg		args[10];

	/* Create warning dialog */
	parent = (parent == NULL) ? GtopLevel : get_shell_ancestor(parent);

	dialog = XmCreateWarningDialog(parent, "warndialog", NULL, 0);
	XtVaSetValues(XtParent(dialog),
#ifdef SW_INSTALLER
		XmNtitle, catgets(_catd, 8, 510, "Warning"),
#else
		XmNtitle, catgets(_catd, 8, 460, "Admintool: Warning"),
#endif
		NULL);
	text = XmStringCreateLocalized(msg);
	xstr = XmStringCreateLocalized(verb);
	XtVaSetValues(dialog,
		XmNautoUnmanage, False,
		XmNdialogStyle,	XmDIALOG_FULL_APPLICATION_MODAL,
		XmNdefaultButtonType, XmDIALOG_CANCEL_BUTTON,
		XmNmessageAlignment, XmALIGNMENT_CENTER,
		XmNmessageString, text,
		XmNokLabelString, xstr,
		RES_CONVERT(XmNcancelLabelString, CANCEL_STRING),
		NULL);
	XmStringFree(text);
	XmStringFree(xstr);
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	XtAddCallback(dialog, XmNokCallback,
			(XtCallbackProc) warningCB,
			(XtPointer) &answer);
	XtAddCallback(dialog, XmNcancelCallback,
			(XtCallbackProc) warningCB,
			(XtPointer) &answer);

	if (del_homedir != NULL) {
		cbox = XmCreateForm(dialog, "cbox", args, j);
		del_toggle = XtVaCreateManagedWidget( "del",
			xmToggleButtonGadgetClass, cbox,
			RSC_CVT(XmNlabelString, catgets(_catd, 8, 461, "Delete Home Directory")),
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 60,
			NULL);
		XtManageChild(cbox);
	}

	answer = -1;
	XtManageChild(dialog);
	XtPopup(XtParent(dialog), XtGrabNone);

	while (answer == -1)
		XtAppProcessEvent(GappContext, XtIMAll);

	if (del_homedir != NULL) {
		*del_homedir = XmToggleButtonGadgetGetState(del_toggle);
	}

	XtRemoveAllCallbacks(dialog, XmNdestroyCallback);
	XtDestroyWidget(XtParent(dialog));

	return (answer == 1) ? True : False;
}

static void
selectCB(
	Widget w,
	response *answer,
	XmSelectionBoxCallbackStruct *cbs)
{
	char* tmp;

	answer->button = cbs->reason;
	switch (cbs->reason) {
		case XmCR_OK:
			XmStringGetLtoR(cbs->value,
				XmSTRING_DEFAULT_CHARSET, &tmp);
			if (*tmp == '\0') {
				answer->sel = NULL;
				XtFree(tmp);
			}
			else {
				answer->sel = tmp;
			}
			break;
		case XmCR_CANCEL:
			answer->sel = NULL;
			break;
	}
}

char *
GetPromptInput(
	Widget parent,
	char* title,
	char* msg)
{
	static response		answer;
	Widget			prompt;
	XmString		text;

	parent = (parent == NULL) ? GtopLevel : get_shell_ancestor(parent);

	prompt = XmCreatePromptDialog(parent, "prompt", NULL, 0);
	XtVaSetValues(XtParent(prompt),
		XmNtitle, title,
		NULL);
	XtVaSetValues(prompt,
		XmNautoUnmanage, False,
		XmNdialogStyle,	XmDIALOG_FULL_APPLICATION_MODAL,
		XmNdefaultButtonType, XmDIALOG_OK_BUTTON,
		RES_CONVERT(XmNokLabelString, OK_STRING),
		RES_CONVERT(XmNcancelLabelString, CANCEL_STRING),
		NULL);
	XtVaSetValues(XmSelectionBoxGetChild(prompt, XmDIALOG_TEXT),
		XmNcolumns, 30,
		NULL);
	XtUnmanageChild(XmSelectionBoxGetChild(prompt, XmDIALOG_HELP_BUTTON));
	XtAddCallback(prompt, XmNokCallback,
			(XtCallbackProc) selectCB,
			(XtPointer) &answer);
	XtAddCallback(prompt, XmNcancelCallback,
			(XtCallbackProc) selectCB,
			(XtPointer) &answer);

	text = XmStringCreateLocalized(msg);
	XtVaSetValues(prompt,
		XmNselectionLabelString, text,
		NULL);
	XmStringFree(text);

	answer.button = 0;
	answer.sel = NULL;
	XtManageChild(prompt);
	XtPopup(XtParent(prompt), XtGrabNone);

	while (answer.button == 0)
		XtAppProcessEvent(GappContext, XtIMAll);

	XtRemoveAllCallbacks(prompt, XmNdestroyCallback);
	XtDestroyWidget(XtParent(prompt));
	XFlush(Gdisplay);

	return answer.sel;
}

static void
errordialogCB(
	Widget w,
	int	* answer,
	XmAnyCallbackStruct* cbs
)
{
	switch (cbs->reason) {
		case XmCR_OK:
			XtRemoveAllCallbacks(errordialog, XmNdestroyCallback);
			XtDestroyWidget(XtParent(errordialog));
			*answer = 1;
			break;
	}
}

Widget
find_parent(Widget parent)
{
	Widget mainwin;

#ifdef SW_INSTALLER
	mainwin = addsoftwaredialog;
#else
	mainwin = sysmgrmain;
#endif

	return parent ? 
		((parent == mainwin) ?
			mainwin :
			get_shell_ancestor(parent)) 
		:
		(mainwin ?
			mainwin :
			GtopLevel);
}

void
display_error(
	Widget parent,
	char * msg
)
{
	XmString	text;
	char*		f_msg;
	static int ok_answer = -1;


	f_msg = filter_errmsg(msg);

	/* Create error dialog */
	parent = find_parent(parent);

	errordialog = XmCreateErrorDialog(parent, "error", NULL, 0);
	XtVaSetValues(XtParent(errordialog),
#ifdef SW_INSTALLER
		XtNtitle, catgets(_catd, 8, 511, "Error"),
#else
		XtNtitle, catgets(_catd, 8, 463, "Admintool: Error"),
#endif
		NULL);
	XtUnmanageChild(XmMessageBoxGetChild(errordialog, 
		XmDIALOG_HELP_BUTTON));
	XtAddCallback(errordialog, XmNokCallback,
		(XtCallbackProc) errordialogCB, &ok_answer);

	XtUnmanageChild(XmMessageBoxGetChild(errordialog, 
		XmDIALOG_CANCEL_BUTTON));

	text = XmStringCreateLocalized(f_msg);
	XtVaSetValues(errordialog,
		XmNautoUnmanage, False,
		XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
		XmNmessageAlignment, XmALIGNMENT_CENTER,
		XmNmessageString, text,
		RES_CONVERT(XmNokLabelString, OK_STRING),
		NULL);
	XmStringFree(text);
	if (f_msg != msg) {
		free(f_msg);
	}

	ok_answer = -1;
	XtManageChild(errordialog);
	XtPopup(XtParent(errordialog), XtGrabNone);

	while (ok_answer == -1)
		XtAppProcessEvent(GappContext, XtIMAll);
}

static void
infodialogCB(
	Widget w,
	int	* answer,
	XmAnyCallbackStruct* cbs
)
{
	switch (cbs->reason) {
		case XmCR_OK:
			XtRemoveAllCallbacks(infoDialog, XmNdestroyCallback);
			XtDestroyWidget(XtParent(infoDialog));
			*answer = 1;
			break;
	}
}

void
display_infomsg (
	Widget  parent,
	char *  const msg
)
{
	static int	ans = -1;
	char*		after;
	XmString	text;


	after = filter_errmsg(msg);

	/* Create error dialog */
	parent = find_parent(parent);

	infoDialog = XmCreateInformationDialog(parent, "info", NULL, 0);
	XtVaSetValues(XtParent(infoDialog),
#ifdef SW_INSTALLER
		XtNtitle, catgets(_catd, 8, 512, "Information"),
#else
		XtNtitle, catgets(_catd, 8, 506, "Admintool: Information"),
#endif
		NULL);
	XtUnmanageChild(XmMessageBoxGetChild(infoDialog, 
		XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(infoDialog, 
		XmDIALOG_HELP_BUTTON));
	XtAddCallback(infoDialog, XmNokCallback, 
		(XtCallbackProc) infodialogCB, &ans);

	text = XmStringCreateLocalized(after);
	XtVaSetValues(infoDialog,
		XmNmessageString, text,
		RES_CONVERT(XmNokLabelString, OK_STRING),
		NULL);
	XmStringFree(text);
	if (after != msg) {
		free(after);
	}

	ans = -1;
	XtManageChild(infoDialog);
	XtPopup(XtParent(infoDialog), XtGrabNone);
	while (ans == -1)
		XtAppProcessEvent(GappContext, XtIMAll);
	XtDestroyWidget(infoDialog);
	 

	return;
}

static void
warning_dialogCB(
	Widget wgt,
	XtPointer cd,
	XmAnyCallbackStruct* cbs
)
{
	switch (cbs->reason) {
		case XmCR_OK:
			XtRemoveAllCallbacks(wgt, XmNdestroyCallback);
			XtDestroyWidget(XtParent(wgt));
			break;
	}
}

Widget
display_warning(
	Widget parent,
	char* msg
)
{
	char*		f_msg;
	XmString	text;
	Widget		warning_dialog;


	f_msg = filter_errmsg(msg);

	/* Create warning dialog */
	parent = (parent == NULL) ? GtopLevel : get_shell_ancestor(parent);

	warning_dialog = XmCreateWarningDialog(parent, "warn", NULL, 0);
	XtVaSetValues(XtParent(warning_dialog),
#ifdef SW_INSTALLER
		XmNtitle, catgets(_catd, 8, 510, "Warning"),
#else
		XmNtitle, catgets(_catd, 8, 460, "Admintool: Warning"),
#endif
		NULL);
	XtUnmanageChild(XmMessageBoxGetChild(warning_dialog, 
		XmDIALOG_HELP_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(warning_dialog, 
		XmDIALOG_CANCEL_BUTTON));
	text = XmStringCreateLocalized(f_msg);
	XtVaSetValues(warning_dialog,
		XmNautoUnmanage, False,
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		XmNmessageAlignment, XmALIGNMENT_CENTER,
		XmNmessageString, text,
		RES_CONVERT(XmNokLabelString, OK_STRING),
		NULL);
	XmStringFree(text);
	if (f_msg != msg) {
		free(f_msg);
	}

	XtAddCallback(warning_dialog, XmNokCallback,
		(XtCallbackProc)warning_dialogCB, NULL);

	XtManageChild(warning_dialog);
	XtPopup(XtParent(warning_dialog), XtGrabNone);
	return(warning_dialog);
}

void
MakePosVisible(
	Widget	list,
	int	item)
{
	int	top, visible;

	XtVaGetValues(list,
		XmNtopItemPosition, &top,
		XmNvisibleItemCount, &visible,
		NULL);

	if (item < top)
		XmListSetPos(list, item);
	else if (item >= top + visible)
		XmListSetBottomPos(list, item);
}

/*
 * Help Callback
 */
void
helpCB(
	Widget	w, 
	char*	helpfile, 
	XtPointer cbs
)
{

	char* s = strchr(helpfile, '.');
	char type = s ? *(s+1) : 't';
	switch (type) {
	  case 'h':
		type = HOWTO;
		break;
	  case 'r':
		type = REFER;
		break;
	  default:
		type = TOPIC;
		break;
	};
	adminhelp(GtopLevel, type, helpfile);

}

/*
 * Wildcard matching routine by Karl Heuer.  Public Domain.
 *
 * Test whether string s is matched by pattern p.
 * Supports "?", "*", "[", each of which may be escaped with "\";
 * Character classes may use "!" for negation and "-" for range.
 * Not yet supported: internationalization; "\" inside brackets.
 */


int
wildmatch(char *s, char *p)
{
    char c;

    while ((c = *p++) != '\0') {	/* While there's still string */
	if (c == '?') {
	    if (*s++ == '\0') return (NO);
					/* Else ok so far: loop again */

	} else if (c == '[') {		/* Start of choice */
	    int wantit = YES;
	    int seenit = NO;
	    if (*p == '!') {
		wantit = NO;
		++p;
	    }
	    c = *p++;
	    do {
		if (c == '\0') return (NO);
		if (*p == '-' && p[1] != '\0') {
		    if (*s >= c && *s <= p[1]) seenit = YES;
		    p += 2;
		} else {
		    if (c == *s) seenit = YES;
		}
	    } while ((c = *p++) != ']');
	    if (wantit != seenit) return (NO);
	    ++s;

	} else if (c == '*') {			/* Wildcard */
	    if (*p == '\0') return (YES); 	/* optimize common case */
	    do {
		if (wildmatch(s, p)) return (YES);
	    } while (*s++ != '\0');
	    return (NO);

	} else if (c == '\\') {
	    if (*p == '\0' || *p++ != *s++) return (NO);
	} else {
	    if (c != *s++) return (NO);
	}
    }
    return (*s == '\0');
}

XmFontList   
ConvertFontList( char *fontlist_str )
{
	XrmValue	from, to;
	XmFontList	fontlist = NULL;
	Boolean		status;

	from.size = strlen( fontlist_str ) + 1;
	from.addr = fontlist_str;

	to.size = sizeof(XmFontList);
	to.addr = (caddr_t) &fontlist;

	status = XtConvertAndStore( GtopLevel,
				    XmRString, &from,
				    XmRFontList, &to );

	return ( fontlist );
}

void
debug_log(FILE * fp, char * fname, char * format, ...)
{
        va_list ap;
        va_start(ap, format);
        fprintf(fp, "%s: ", fname);
        vfprintf(fp, format, ap);
        fprintf(fp, "\n");
        va_end(ap);
}

print_sw_list(Module *m, int lev)
{
  Module * mtmp = m;
  int i;

  do {
    for (i=0; i<lev; i++)
	printf("    ");

    if (mtmp->type == PRODUCT) {
	    printf("P %s\n", m->info.prod->p_name);
    }
    else {
	    printf("%s %s %s %s\n",
		mtmp->info.mod->m_name,
		mtmp->info.mod->m_pkgid,
		mtmp->info.mod->m_category ? mtmp->info.mod->m_category : "",
		mtmp->info.mod->m_vendor ? mtmp->info.mod->m_vendor : ""
	    	);
/*
		mtmp->info.mod->m_prodname ? mtmp->info.mod->m_prodname : "",
		mtmp->info.mod->m_prodvers ? mtmp->info.mod->m_prodvers : "",
		mtmp->info.mod->m_arch ? mtmp->info.mod->m_arch : "",
		mtmp->info.mod->m_desc ? mtmp->info.mod->m_desc : ""
*/
    }


    if (mtmp->type == CLUSTER || mtmp->type == METACLUSTER ||
        mtmp->type == PRODUCT) {
        print_sw_list(get_sub(mtmp), lev+1);
    }
  } while ((mtmp = get_next(mtmp)));
}

static int listnum;
static int listsize;
#define ADJUST_LIST() { listsize = 2 * listsize; \
	*list = (struct modinfo **)realloc(*list, listsize * sizeof(struct modinfo *)); if (list == NULL) fatal(catgets(_catd, 8, 494, "get_selected_modules: unable to realloc")); }

int
get_selected_modules(Module *m, struct modinfo *** list)
{
	Module * mtmp = m;
	ModStatus mstat;
	L10N * l10n;

	if (*list == NULL) {
		listnum = 0;
		listsize = 256;
		*list = (struct modinfo **)malloc(listsize * sizeof(struct modinfo *));
		if (list == NULL)
			fatal(catgets(_catd, 8, 495, "get_selected_modules: unable to malloc"));
	}

	do {
		mstat = mtmp->info.mod->m_status;
		if ((mtmp->type == PACKAGE) &&
		    (mstat == SELECTED)) {
			if (listnum >= listsize) 
				ADJUST_LIST();
			(*list)[listnum++] = mtmp->info.mod;
			mtmp->info.mod->m_status = -1;

			/* traverse l10n list */
			l10n = mtmp->info.mod->m_l10n;
			while (l10n) {
				if (l10n->l10n_package->m_status == SELECTED) {
					if (listnum >= listsize) 
						ADJUST_LIST();
					(*list)[listnum++] = l10n->l10n_package;
					l10n->l10n_package->m_status = -1;
				}
				l10n = l10n->l10n_next;
			}
		/* this is an indication that pkg has been selected
                 * and not to add it to list again.
		 * reset_selected_modules will set this back to SELECTED.
  	         */
		}

		if ((mtmp->type == CLUSTER || mtmp->type == METACLUSTER ||
		    mtmp->type == PRODUCT) && 
		    (mstat == PARTIALLY_SELECTED || mstat == SELECTED)) {
			get_selected_modules(get_sub(mtmp), list);
		}
	} while ((mtmp = get_next(mtmp)));

	return listnum;
}

/*
 * To avoid adding duplicated pkgs, get_selected_pkgs()
 * sets the status field to -1 so if a pkg appears
 * in multiple metaclusters it is not added to "to add"
 * list more than once. reset_select_modules() simply
 * traverses tree and set -1 status values to SELECTED.
 */ 

void
reset_selected_modules(Module * m)
{
	Module * mtmp = m, * m0;
	L10N * l10n;

	do {
		if ((mtmp->type == PACKAGE) &&
		    (mtmp->info.mod->m_status == -1)) {
			mtmp->info.mod->m_status = SELECTED;
			l10n = mtmp->info.mod->m_l10n;
			while (l10n) {
				if (l10n->l10n_package->m_status == -1)
					l10n->l10n_package->m_status = SELECTED;
				l10n = l10n->l10n_next;
			}
		}

		if (mtmp->type == CLUSTER || 
                    mtmp->type == METACLUSTER || 
		    mtmp->type == PRODUCT) {
			reset_selected_modules(get_sub(mtmp));
		}

	} while ((mtmp = get_next(mtmp)));
}

/*
 * reset_selected_pkgs does the same (more or less) as above
 * but can be used as arg to walklist. At a time when willing
 * to introduce more risk, the two should be combined. (maybe).
 * FIX ME FIX ME
 */
int
reset_selected_pkgs(Node * n, caddr_t arg)
{
    Modinfo * mi = (Modinfo *)n->data;
    if (mi->m_status == -1)
	mi->m_status = SELECTED;
}

void
yesnoCB(Widget w, int * answer, XmAnyCallbackStruct * cbs)
{
	if (cbs->reason == XmCR_OK)
		*answer = YES;
	else if (cbs->reason == XmCR_CANCEL)
		*answer = NO;
}

int
AskUser(Widget parent, char * question, char * yes, char * no, int def)
{
	static Widget askdialog = NULL;
	XmString text, ystr, nstr;
	static int answer = -1;
	extern void yesnoCB();

	parent = find_parent(parent);

	if (!askdialog) {
		askdialog = XmCreateQuestionDialog(parent, "askUserDialog",
				NULL, 0);
		XtVaSetValues(askdialog,
			XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
			XmNmessageAlignment, XmALIGNMENT_CENTER,
			NULL);
		XtVaSetValues(XtParent(askdialog),
#ifdef SW_INSTALLER
			XmNtitle, catgets(_catd, 8, 513, "Notice"),
#else
			XmNtitle, catgets(_catd, 8, 465, "Admintool: Notice"),
#endif
			NULL);
		XtUnmanageChild(XmMessageBoxGetChild(askdialog, 
				XmDIALOG_HELP_BUTTON));
		XtAddCallback(askdialog, XmNokCallback, yesnoCB, &answer);
		XtAddCallback(askdialog, XmNcancelCallback, yesnoCB, &answer);
	}
	text = XmStringCreateLocalized(question);
	ystr = XmStringCreateLocalized(yes);	
	nstr = XmStringCreateLocalized(no);	
	XtVaSetValues(askdialog,
		XmNmessageString, text,
		XmNokLabelString, ystr,
		XmNcancelLabelString, nstr,
		XmNdefaultButtonType, def == YES ?
			XmDIALOG_OK_BUTTON : XmDIALOG_CANCEL_BUTTON,
		NULL);

	XmStringFree(text);
	XmStringFree(ystr);
	XmStringFree(nstr);
	XtManageChild(askdialog);
	XtPopup(XtParent(askdialog), XtGrabNone);
	answer = -1;
	while (answer == -1) {
		XtAppProcessEvent(GappContext, XtIMAll);
	}
	XtUnmanageChild(askdialog);
	XmUpdateDisplay(askdialog);
	return(answer);
}

FSspace* 
get_fs(FSspace **sp, char* fs)
{
        int     i;
 
        for (i = 0; sp[i]; i++) {
                if (strcmp(sp[i]->fsp_mntpnt, fs) == 0)
                        return (sp[i]);
        }
        return (NULL);
}

unsigned long
get_fs_space(FSspace **sp, char *fs)
{
        int     i;
 
        for (i = 0; sp[i]; i++) {
                if (strcmp(sp[i]->fsp_mntpnt, fs) == 0)
                        return (sp[i]->fsp_reqd_contents_space);
        }
        return (0L);
}


/* 
 * When user has a handle on a Modinfo struct and wants the
 * space occupied on a filesystem by that pkg, this routine
 * will search application list of filesystems and map that
 * onto the s/w library (libsw) value. That value can be 
 * used as an index into m_deflt_fs field so get size.
 */
    
unsigned long
get_pkg_fs_space(Modinfo* mi, char* fs)
{
    static struct fs_map_entry {
        char** fs_name;
        FileSys   swlib_id;
    } fs_map[] = {
        { &installFileSystems[0], ROOT_FS    }, /* "/" */
        { &installFileSystems[1], USR_FS     }, /* "/usr" */
        { &installFileSystems[2], OPT_FS     }, /* "/opt" */
        { &installFileSystems[3], VAR_FS     }, /* "/var" */
        { &installFileSystems[4], EXPORT_FS  }, /* "/export" */
        { &installFileSystems[5], USR_OWN_FS }, /* "/usr/openwin" */
    };

    int i;

    for (i=0; i < sizeof(fs_map) / sizeof(struct fs_map_entry); i++) {
        if (strcmp(fs, *fs_map[i].fs_name) == 0)
	    return(mi->m_deflt_fs[fs_map[i].swlib_id]);
    }
    return(0);
}

#define	MAXLEN	80

#ifdef OLD_FILTER

/*
 * (char *)fit_width(char *str, int maxcol) returns a pointer
 * such that the width of substring [str, return_ptr) <= maxcol and
 * the width of [str, return_ptr] > maxcol.
 * precondition: strwidth(str) > maxcol.
 */
static char *
fit_width(char *str, int maxcol)
{
	register char	*ptr = str;
	register int	column_count = 0;

	while ((column_count += EUCCOL(ptr)) <= maxcol)
		ptr += mblen(ptr, sizeof (int));
	
	return ptr;
}

/*
 * (char *)copy_and_format(char *orig_str, int maxcol) allocates memory,
 * copies and inserts newlines to a new_str.
 *
 * It is called when there is no newline or space in the first
 * MAXLEN characters.  This will (practically) never happen in C or
 * European locales but could potentially happen in Asian locales.
 *
 * We do a simply formatting by inserting newline about every maxcol columns.
 * If there is a newline (after the first MAXLEN characters), we don't
 * insert another newline but start to process from the next character.
 */
static char *
copy_and_format(char *orig_str, int maxcol)
{
	static char	*allocated_chars = (char *)NULL;
	register int	column_count, len;
	register char	*from_ptr, *to_ptr;

	if (allocated_chars != (char *)NULL) {
		/* free the char array allocated last time */
		free(allocated_chars);
		allocated_chars = (char *)NULL;
	}

	/* allocate more than enough memory */
	allocated_chars = (char *)malloc(
		(int)(strlen(orig_str) + (strlen(orig_str) / maxcol) + 2) );
	
	/* Do a simple formatting */
	from_ptr = orig_str;
	to_ptr = allocated_chars;
	while (*from_ptr != NULL) {
		column_count = 0;

		/* format up to maxcol columns */
		while ((*from_ptr != NULL) && (*from_ptr != '\n') &&
		    (column_count < maxcol)) {

			len = mblen(from_ptr, sizeof (int));
			strncpy(to_ptr, from_ptr, len);
			from_ptr += len;
			to_ptr += len;
			column_count += EUCCOL(from_ptr);
		}

		/* if there is a newline, do newline and continue */
		if (*from_ptr == '\n')
			from_ptr++;
		*to_ptr++ = '\n'; /* insert a newline for formatting */
	}
	*to_ptr = NULL;
	return allocated_chars;
}

static char*
filter_errmsg (char * original_message)
{
	char *		working_pointer = original_message;
	char *		ptr;
	int		len;


	/* detect and handle degenerate call */
	if ((original_message == NULL) || (strlen(original_message) == 0))
		return(working_pointer);

	/* If the message length is long, make sure there are
	 *  new lines in the text.  
	 *  If new lines are not found, add them.
	 */
	ptr = working_pointer;
	len = STRWIDTH(working_pointer);
	while (len > MAXLEN) {
		/* Check for new lines. */
		if (memchr(ptr, '\n', MAXLEN) == NULL) {
			/* No new line found.
			 * Add one at precceeding space.
			 */
			register char		*end_ptr;

			end_ptr = fit_width(ptr, MAXLEN);
			while (*end_ptr != ' ' && end_ptr != ptr)
				end_ptr--; /* find space. */
			if (*end_ptr == ' ') {
				/* found space, replac with nl */
				*end_ptr = '\n';
				ptr = end_ptr + 1;
			} else {
				/*
				 * no space found.  copy to a new
				 * u_char[] with nl inserted.
				 */
				 working_pointer =
				     copy_and_format(working_pointer,
				     MAXLEN);
				break; /* out of while loop */
			}
			
		} else {
			/* Set ptr to one past the found new line. */
			ptr = strchr(ptr, '\n') +1;
		}
		len = STRWIDTH(ptr);
	}

	return(working_pointer);
}

#endif

size_t
l_wcslen (const wchar_t *s)
{
	const wchar_t *s0 = s + 1;

	while (*s++)
		;
	return (s - s0);
}

int
l_wcwidth (wchar_t wc)
{
	if (wc) {
		if (iswprint(wc) == 0)
			return (-1);
		switch (wcsetno(wc)) {
		case	0:
			return (1);
		case	1:
			return (scrw1);
		case	2:
			return (scrw2);
		case	3:
			return (scrw3);
		}
	}
	return (0);
}

/* static size_t wcs_width (wchar_t *) is similar to wcswidth() except it
 * works even if there are unprintable characters in the string 
 * (such as '\n').
 */

size_t
_wcs_width (wchar_t       *wsrc)
{
        wchar_t *wptr = wsrc;
        size_t  width = 0;

        while (wptr != NULL && *wptr != L'\0') {
                if (iswprint(*wptr))
                        width += l_wcwidth(*wptr);
                wptr++;
        }

        return width;
}

/* static wchar_t * wc_memchr(wchar_t *, wchar_t , size_t) is similar to
 * memchr() execpt it works on wide characters.  It will search for the
 * first instance of the wide character until it either finds the character,
 * reaches a null terminator or runs out of positions, n.
 */

wchar_t  *
_wc_memchr (
          wchar_t       *wsrc,
          wchar_t       wchar,
          size_t        n)
{
        wchar_t *wptr = wsrc;
        int     num_positions = 0;

        while (wptr != NULL) {
                if (*wptr == wchar)      /* Found match! */
                        break;
                if (iswprint(*wptr))
                        num_positions += l_wcwidth(*wptr);
                if (num_positions < n) {
                        wptr++;
                        continue;
                }
                else {                  /* No match in n positions. */
                        wptr = NULL;
                        break;
                }
        }

        return wptr;
}

/*
 * (wchar_t *)copy_and_format(wchar_t *orig_str, int maxcol) allocates memory,
 * copies and inserts newlines to a new_str.
 *
 * It is called when there is no newline or space in the first
 * MAXLEN characters.  This will (practically) never happen in C or
 * European locales but could potentially happen in Asian locales.
 *
 * We do a simply formatting by inserting newline about every maxcol columns.
 * If there is a newline (after the first MAXLEN characters), we don't
 * insert another newline but start to process from the next character.
 */
wchar_t *
_copy_and_format(wchar_t *orig_str, int maxcol)
{
        static wchar_t          *allocated_chars = (wchar_t *)NULL;
        register int            column_count, len;
        register wchar_t        *from_ptr, *to_ptr;

        if (allocated_chars != (wchar_t *)NULL) {
                /* free the char array allocated last time */
                free(allocated_chars);
                allocated_chars = (wchar_t *)NULL;
        }

        /* allocate more than enough memory */
        allocated_chars = (wchar_t *)
            malloc(((2 * l_wcslen(orig_str)) / (maxcol / 2) + 2) *
                   sizeof(wchar_t));

        /* Do a simple formatting */
        from_ptr = orig_str;
        to_ptr = allocated_chars;
        while (*from_ptr != NULL) {
                column_count = 0;

                /* format up to maxcol columns */
                while ((*from_ptr != NULL) &&
                       (*from_ptr != L'\n') &&
                       (column_count < maxcol)) {

                        len = sizeof(wchar_t);
                        wcsncpy(to_ptr, from_ptr, len);
                        from_ptr += len;
                        to_ptr += len;
                        column_count += l_wcwidth(*from_ptr);
                }
               /* if there is a newline, do newline and continue */
                if (*from_ptr == L'\n')
                        from_ptr++;
                *to_ptr++ = L'\n'; /* insert a newline for formatting */
        }
        *to_ptr = NULL;
        return allocated_chars;
}

/*
 * (wchar_t *)_fit_width(wchar_t *str, int maxcol) returns a pointer
 * such that the width of substring [str, return_ptr) <= maxcol and
 * the width of [str, return_ptr] > maxcol.
 * precondition: strwidth(str) > maxcol.
 */
wchar_t *
_fit_width(wchar_t *str, int maxcol)
{
        wchar_t         *wptr;
        register int    column_count = 0;

        for (wptr = str; (column_count <= maxcol) && wptr; wptr++) {
                if (iswprint(*wptr)) {
                        column_count += l_wcwidth(*wptr);
                }
        }

        return wptr;
}


static char*
filter_errmsg (char * original_message)
{
        wchar_t         *working_pointer;
        wchar_t         *wbuf;
        int             working_counter;
        int             msglen;
        wchar_t         *last_pointer;
        wchar_t         *ptr;
        int             len;
        enum            {SEARCHFOR_WHITESPACE_OR_COLON,
                           SEARCHFOR_NON_WHITESPACE} forward_state;
        static char     emptystring[] = "";
        typedef struct {
	    char* trace;
	    char* text;
        } split_errmsg;
        split_errmsg    result = {emptystring, emptystring};
	Boolean		new_text_memory = False;


        /* detect and handle degenerate call */
        if ((original_message == NULL) || (strlen(original_message) == 0))
                return(result.text);

        /* initialize */
        msglen = strlen(original_message);
        wbuf = (wchar_t *) malloc (sizeof(wchar_t) * (msglen + 1));
        msglen = mbstowcs(wbuf, original_message, msglen + 1);
        working_pointer = wbuf;
        last_pointer = wbuf + msglen;
        forward_state = SEARCHFOR_WHITESPACE_OR_COLON;

        /* clear trailing line feed if any */
        if (*last_pointer == L'\n')
                *last_pointer = L'\0';

        /* search forward to first whitespace (except following colon) */
        /* may search all the way to end of string if no whitespace found*/
        for (working_counter = 0; working_counter < msglen;
            ++working_counter, ++working_pointer) {
                if (forward_state == SEARCHFOR_WHITESPACE_OR_COLON) {
                        if (iswspace(*working_pointer))
                                break;
                        if (*working_pointer == L':')
                                forward_state = SEARCHFOR_NON_WHITESPACE;
                } else if (forward_state == SEARCHFOR_NON_WHITESPACE) {
                        if (!iswspace(*working_pointer))
                                forward_state = SEARCHFOR_WHITESPACE_OR_COLON;
                }
        }
        /* back up to previous colon */
        /* may back up clear to beginning of string if no colon encountered */
        if (working_counter <= msglen)
                for (; working_counter >= 0;
                    --working_counter, --working_pointer) {
                        if (*working_pointer == L':')
                                break;
                }
        /* if we have a trace part, frame it */
        if (working_counter > 0) {
                /* zip out the colon */
                *working_pointer = L'\0';
                result.trace = (char*)original_message;
		new_text_memory = True;
        }

        /* find beginning of real text */
        for (++working_pointer, ++working_counter ; working_counter < msglen;
            ++working_counter, ++working_pointer) {
                if (!iswspace(*working_pointer))
                        break;
        }

	if (new_text_memory) {
	  /*
	   *  We need to reallocate the memory for working_pointer so 
	   *  so we can deallocate from the correct starting point.
	   */
	  len = l_wcslen(working_pointer) + 1;
	  ptr = (wchar_t *) malloc(len * sizeof(wchar_t));
	  (void) memcpy((void *) ptr, (const void *) working_pointer, 
			len * sizeof(wchar_t));
	  free (wbuf);
	  working_pointer = ptr;
	  working_counter -= msglen - len + 1;
	  msglen = len - 1;  /* Don't include the null terminator. */
	}

        /* allow for empty text part */
        if (working_counter < msglen) {
                /* If the message length is long, make sure there are
                 *  new lines in the text.
                 *  If new lines are not found, add them.
                 */
                ptr = working_pointer;
                len = _wcs_width(working_pointer);
                while (len > MAXLEN) {

                        wchar_t *nl_ptr;

                        /* Check for new lines. */
                        if ((nl_ptr = _wc_memchr(ptr, L'\n', MAXLEN)) == NULL)
{
                                /* No new line found.
                                 * Add one at precceeding space.
                                 */
                                register wchar_t        *end_ptr;

                                end_ptr = _fit_width(ptr, MAXLEN);
                                while ((!iswspace(*end_ptr) &&
                                        *end_ptr != L'\n') &&
                                       end_ptr != ptr)
                                        end_ptr--; /* find space. */
                                if (iswspace (*end_ptr)) {
                                        /* found space, replace with nl */
                                        *end_ptr = L'\n';
                                        ptr = end_ptr + 1;
                               } else {
                                        /*
                                         * no space found.  copy to a new
                                         * wchar_t[] with nl inserted.
                                         */
                                        working_pointer =
                                             _copy_and_format(working_pointer,
                                                             MAXLEN);
                                        break; /* out of while loop */
                                }
                        } else {
                                /* Set ptr to one past the found new line. */
                                ptr = nl_ptr + 1;
                        }

                        len = _wcs_width(ptr);
                }

                len = l_wcslen(working_pointer) + 1;
                result.text = (char *) malloc(len * sizeof(wchar_t));
                (void) wcstombs(result.text, working_pointer,
                                len * sizeof(wchar_t));
                free (working_pointer);
        }

        return(result.text);
}


char * 
get_mod_name(Module * m)
{
	if (m == NULL)
		return(NULL);
	if (m->type == PRODUCT)
		return(m->info.prod->p_name);
	else
		return(m->info.mod->m_name);
}

char*
get_prodvers(Module *m)
{
    if (m == NULL)
 return(NULL);
    if (m->type == PRODUCT || m->type == NULLPRODUCT)
 return(m->info.prod->p_version);
    else if (m->type == METACLUSTER || m->type == CLUSTER)
	return(m->info.mod->m_version);
    else
        return(m->info.mod->m_prodvers);
}


/*
 * This procedure will ensure that, if a dialog window is being mapped,
 * its contents become visible before returning.  It is intended to be
 * used just before a bout of computing that doesn't service the display.
 * You should still call XmUpdateDisplay() at intervals during this
 * computing if possible.
 *
 * The monitoring of window states is necessary because attempts to map
 * the dialog are redirected to the window manager (if there is one) and
 * this introduces a significant delay before the window is actually mapped
 * and exposed.  This code works under mwm, twm, uwm, and no-wm.  It
 * doesn't work (but doesn't hang) with olwm if the mainwindow is iconified.
 *
 * The argument to ForceDialog is any widget in the dialog (often it
 * will be the BulletinBoard child of a DialogShell).
 */

ForceDialog(w)
     Widget w;
{
  Widget diashell, topshell;
  Window diawindow, topwindow;
  Display *dpy;
  XWindowAttributes xwa;
  XEvent event;
  XtAppContext cxt;

/* Locate the shell we are interested in.  In a particular instance, you
 * may know these shells already.
 */

  for (diashell = w;
       !XtIsShell(diashell);
       diashell = XtParent(diashell))
    ;

/* Locate its primary window's shell (which may be the same) */

  for (topshell = diashell;
       !XtIsTopLevelShell(topshell);
       topshell = XtParent(topshell))
    ;

  if (XtIsRealized(diashell) && XtIsRealized(topshell)) {
    dpy = XtDisplay(topshell);
    diawindow = XtWindow(diashell);
    topwindow = XtWindow(topshell);
    cxt = XtWidgetToApplicationContext(diashell);

/* Wait for the dialog to be mapped.  It's guaranteed to become so unless... */

    while (XGetWindowAttributes(dpy, diawindow, &xwa),
           xwa.map_state != IsViewable) {

/* ...if the primary is (or becomes) unviewable or unmapped, it's
   probably iconified, and nothing will happen. */

      if (XGetWindowAttributes(dpy, topwindow, &xwa),
          xwa.map_state != IsViewable)
        break;

/* At this stage, we are guaranteed there will be an event of some kind.
   Beware; we are presumably in a callback, so this can recurse. */

      XtAppNextEvent(cxt, &event);
      XtDispatchEvent(&event);
    }
  }

/* The next XSync() will get an expose event if the dialog was unmapped. */

  XmUpdateDisplay(topshell);
}


Widget
create_button_box(
	Widget  parent,
	Widget  top_widget,
	void*   context,
	Widget* ok,
	Widget* apply,
	Widget* reset,
	Widget* cancel,
	Widget* help)
{
	/*
	 * Create a generic dialog button box containing OK, Apply, Reset,
	 * Cancel, and Help pushbuttons.  If the parent dialog is a form
	 * widget, attach to the bottom.
	 */

	Widget bbox = XmCreateMessageBox(parent, "bbox", NULL, 0);
	XtVaSetValues(bbox,
		XmNdefaultButtonType, XmDIALOG_OK_BUTTON,
		RES_CONVERT(XmNokLabelString, OK_STRING),
		RES_CONVERT(XmNcancelLabelString, CANCEL_STRING),
		RES_CONVERT(XmNhelpLabelString, HELP_STRING),
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 1,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 1,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 1,
		NULL);

	if (top_widget != NULL) {
		XtVaSetValues(bbox,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, top_widget,
			XmNtopOffset, 5,
			NULL);
	}

	if (XmIsForm(parent)) {
		XtVaSetValues(bbox,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 1,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 1,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset, 1,
			NULL);

		XtVaSetValues(parent,
			XmNdefaultButton,
				XmMessageBoxGetChild(bbox, XmDIALOG_OK_BUTTON),
			NULL);
	}

	if (ok != NULL) {
		*ok = XmMessageBoxGetChild(bbox, XmDIALOG_OK_BUTTON);
		if (context)
			UxPutContext( *ok, (char *) context );
	}
	else {
		XtUnmanageChild(XmMessageBoxGetChild(bbox, XmDIALOG_OK_BUTTON));
	}

	if (cancel != NULL) {
		*cancel = XmMessageBoxGetChild(bbox, XmDIALOG_CANCEL_BUTTON);
		if (context)
			UxPutContext( *cancel, (char *) context );
	}
	else {
		XtUnmanageChild(XmMessageBoxGetChild(bbox, XmDIALOG_CANCEL_BUTTON));
	}

	if (help != NULL) {
		*help = XmMessageBoxGetChild(bbox, XmDIALOG_HELP_BUTTON);
		if (context)
			UxPutContext( *help, (char *) context );
	}
	else {
		XtUnmanageChild(XmMessageBoxGetChild(bbox, XmDIALOG_HELP_BUTTON));
	}

	if (apply != NULL) {
		*apply = XtVaCreateManagedWidget( "Apply",
			xmPushButtonWidgetClass,
			bbox,
			RES_CONVERT( XmNlabelString, APPLY_STRING),
			NULL );
		if (context)
			UxPutContext( *apply, (char *) context );
	}

	if (reset != NULL) {
		*reset = XtVaCreateManagedWidget( "Reset",
			xmPushButtonWidgetClass,
			bbox,
			RES_CONVERT( XmNlabelString, RESET_STRING),
			NULL );
		if (context)
			UxPutContext( *reset, (char *) context );
	}

	XtUnmanageChild(XmMessageBoxGetChild(bbox, XmDIALOG_MESSAGE_LABEL));
	XtManageChild(bbox);

	return bbox;
}

/*
 * The following callbacks are used for an editable scrolling list.
 * The add callback assumes that the TextField's widget id is passed in
 * as callback data, and that it's userData is the widget id of the
 * scrolling list.  The delete callback has the list's id passed in
 * as callback data.
 */
void
addListEntryCB(
	Widget    wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	Widget		inputText = (Widget)cd;
	Widget		scrList;
	char *		entry;
	char *		tmp;
	int		count, i;
	XmString *	itemlist;
	XmString	xstr;

	XtVaGetValues(inputText,
		XmNuserData, &scrList,
		NULL);

	entry = XmTextFieldGetString(inputText);
	xstr = XmStringCreateLocalized(entry);
	if ((entry[0] == '\0') || XmListItemExists(scrList, xstr)) {
		XtFree(entry);
		XmStringFree(xstr);
		return;
	}

	XtVaGetValues(scrList,
		XmNitemCount, &count,
		XmNitems, &itemlist,
		NULL);
	
	/* insertion sort */
	for (i=0; i<count; i++) {
		XmStringGetLtoR(itemlist[i],
			XmSTRING_DEFAULT_CHARSET, &tmp);
		if (strcmp(entry, tmp) < 0) {
			XtFree(tmp);
			break;
		}
		XtFree(tmp);
	}
	i++;	/* Motif list indices begin at 1 */

	XmListAddItemUnselected(scrList, xstr, i);
	XmListDeselectAllItems(scrList);
	XmListSelectPos(scrList, i, False);
	MakePosVisible(scrList, i);
	XtFree(entry);
	XmStringFree(xstr);

	/* Clear User text field */
	XtVaSetValues(inputText,
		XmNvalue, "",
		NULL);

}

void
deleteListEntryCB(
	Widget    wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	Widget		scrList = (Widget)cd;
        XmStringTable   sellist;
	int		selcount;

        XtVaGetValues(scrList,
                XmNselectedItems, &sellist,
		XmNselectedItemCount, &selcount,
                NULL);
        if (selcount > 0) {
		XmListDeleteItems(scrList, (XmString*)sellist, selcount);
	}
}

