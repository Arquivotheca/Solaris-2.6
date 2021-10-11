


/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)util.c	1.10 94/11/28 Sun Microsystems"

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
#include <Xm/ToggleBG.h>

#include "adminhelp.h"

#include "util.h"
#include "launcher.h"
#include "xpm.h"

#ifdef USE_XPG4_WCS
/*
 * Note: If strwidth() is not adopted in CDE/s495, convert x to wide char
 * string and use wcswidth(x).
 * Note: If euccol() or an equivalent function is not in CDE/s495,
 * do the following where EUCCOL(from_ptr) is called:
 *
 * wchar_t	wchar;
 *
 * (void)mbtowc(&wchar, from_ptr, MB_CUR_MAX);
 * column_count += wcwidth(wchar);
 */
extern int			strwidth(char *);
#define STRWIDTH(x)		strwidth(x)
#else
extern int			eucscol(char *);
extern int			euccol(char *);
#define STRWIDTH(x)		eucscol(x)
#define EUCCOL(x)		euccol(x)
#endif

typedef struct {
	int	button;
	char	*sel;
} response;

static char * fit_width(char *, int);
static char *copy_and_format(char *, int);

extern nl_catd	catd;	/* for catgets(), defined in main.cc */

extern Cursor busypointer;

static Widget* windowlist[] = {
	&launchermain,
	&propertyDialog,
	&fileSelectionDialog,
	NULL
};

static Widget errordialog;

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
		XmNtitle, catgets(catd, 1, 76, "Launcher: Warning"),
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
			RSC_CVT(XmNlabelString, catgets(catd, 1, 77, "Delete Home Directory")),
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

#define METHODFAIL	catgets(catd, 1, 78, "The operation failed with this status:")

void
display_error(
	Widget parent,
	char * msg
)
{
	XmString	text;
	static int ok_answer = -1;


	/* Create error dialog */
	parent = parent ? 
			((parent == launchermain) ? 
				launchermain : 
				get_shell_ancestor(parent)) 
			:
			(launchermain ? 
				launchermain : 
				GtopLevel);

	errordialog = XmCreateErrorDialog(parent, "error", NULL, 0);
	XtVaSetValues(XtParent(errordialog),
		XtNtitle, catgets(catd, 1, 79, "Launcher: Error"),
		NULL);
	XtUnmanageChild(XmMessageBoxGetChild(errordialog, 
		XmDIALOG_HELP_BUTTON));
	XtAddCallback(errordialog, XmNokCallback,
		(XtCallbackProc) errordialogCB, &ok_answer);

	XtUnmanageChild(XmMessageBoxGetChild(errordialog, 
		XmDIALOG_CANCEL_BUTTON));

	text = XmStringCreateLocalized(msg);
	XtVaSetValues(errordialog,
		XmNautoUnmanage, False,
		XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
		XmNmessageAlignment, XmALIGNMENT_CENTER,
		XmNmessageString, text,
		NULL);
	XmStringFree(text);

	ok_answer = -1;
	XtManageChild(errordialog);
	XtPopup(XtParent(errordialog), XtGrabNone);

	while (ok_answer == -1)
		XtAppProcessEvent(GappContext, XtIMAll);
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

void
display_warning(
	Widget parent,
	char* msg
)
{
	Widget		warning_dialog;
	XmString	text;


	/* Create warning dialog */
	parent = (parent == NULL) ? GtopLevel : get_shell_ancestor(parent);

	warning_dialog = XmCreateWarningDialog(parent, "warn", NULL, 0);
	XtVaSetValues(XtParent(warning_dialog),
		XtNtitle, catgets(catd, 1, 80, "Launcher: Warning"),
		NULL);
	XtUnmanageChild(XmMessageBoxGetChild(warning_dialog, 
		XmDIALOG_HELP_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(warning_dialog, 
		XmDIALOG_CANCEL_BUTTON));
	text = XmStringCreateLocalized(msg);
	XtVaSetValues(warning_dialog,
		XmNautoUnmanage, False,
		XmNmessageAlignment, XmALIGNMENT_CENTER,
		XmNmessageString, text,
		NULL);
	XmStringFree(text);

	XtAddCallback(warning_dialog, XmNokCallback,
		(XtCallbackProc)warning_dialogCB, NULL);

	XtManageChild(warning_dialog);
	XtPopup(XtParent(warning_dialog), XtGrabNone);

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

void
yesnoCB(Widget w, XtPointer answer, XmAnyCallbackStruct * cbs)
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

	if (parent ==  NULL)
		parent = launchermain;

	if (!askdialog) {
		askdialog = XmCreateQuestionDialog(parent, "askUserDialog",
				NULL, 0);
		XtVaSetValues(askdialog,
			XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
			XmNmessageAlignment, XmALIGNMENT_CENTER,
			NULL);
		XtVaSetValues(XtParent(askdialog),
			XmNtitle, catgets(catd, 1, 81, "Launcher: Notice"),
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

void
free_mem(void * ptr) 
{
	if (ptr != NULL)
		free((char *)ptr);
}

void
fatal(char * msg)
{
	fprintf(stderr, catgets(catd, 1, 82, "FATAL: %s\n"), msg);
	exit(-1);
}

void
ForceDialog(Widget w)
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

int
load_XPM(Widget w, char * pmfile, Pixel bg, Pixmap *pm)
{
        XpmAttributes   attrs;
        XpmColorSymbol  colors[2];
        Pixmap          pix;
        Pixmap          mask;
        int             ret;
        Display*        display;
        Screen*         screen;
	String 		filename;

        /* teleuse bitmap editor uses this symbolic name for transparent */

        colors[0].name = "transparent";
        colors[0].value = NULL;
        colors[0].pixel = bg;

        /* CDE bitmap editor uses this symbolic name for transparent */

        colors[1].name = "none";
        colors[1].value = NULL;
        colors[1].pixel = bg;

        /* load pixmap */
        attrs.valuemask = XpmVisual | XpmColorSymbols;
        attrs.visual    = DefaultVisualOfScreen(XtScreen(w));
        attrs.colorsymbols = colors;
        attrs.numsymbols = 2;


        display = XtDisplay(w);
        screen = XtScreen(w);

	filename = XtResolvePathname(display,
                                        "bitmaps", 
                                        (String) pmfile,
                                        "",
					NULL,
                                        NULL, 
                                        0,  
                                        NULL);

        if (filename == NULL)
                filename = XtNewString((String) pmfile);

        ret = XpmReadFileToPixmap(display, RootWindowOfScreen(screen),
            filename, &pix, &mask, &attrs);

        XtFree(filename);

        XpmFreeAttributes(&attrs);
        if (mask != (Pixmap) NULL)
                XFreePixmap(display, mask);

        if (ret == XpmSuccess)
                (*pm) = pix;

        return (ret);
}

void
xpm_error(int code, char * pmfile)
{
	char 		err_msg[256];
        switch (code) {
        case XpmOpenFailed:
                sprintf(err_msg,
                        catgets(catd, 1, 83, "Unable to open pixmap file %s"), pmfile ? pmfile : "");
                break;
        case XpmFileInvalid:
                sprintf(err_msg,
                        catgets(catd, 1, 84, "invalid pixmap file %s"), pmfile ? pmfile : "");
                break;
        case XpmNoMemory:
                sprintf(err_msg,
                        catgets(catd, 1, 85, "no memory to load pixmap %s"), pmfile ? pmfile : "");
                break;

        case XpmColorError:
                sprintf(err_msg,
                        catgets(catd, 1, 86, "changed pixmap color(s) for %s"), pmfile ? pmfile : "");
                break;

        case XpmColorFailed:
                sprintf(err_msg,
                        catgets(catd, 1, 87, "Unable to allocate color(s) for %s"), pmfile ? pmfile : "");
                break;
        default:
                break;
        }
	display_error(launchermain, err_msg);
}


int
create_XPM(Widget w, char **data, Pixmap bg, Pixmap * pm)
{
	int ret;
	XpmAttributes  attrs;
        XpmColorSymbol  colors[2];
        Display*        display;
        Screen*         screen;
	Pixmap 		mask;
	Pixmap		pix;

        display = XtDisplay(w);
        screen = XtScreen(w);

        /* teleuse bitmap editor uses this symbolic name for transparent */

        colors[0].name = "transparent";
        colors[0].value = NULL;
        colors[0].pixel = bg;

        /* CDE bitmap editor uses this symbolic name for transparent */

        colors[1].name = "none";
        colors[1].value = NULL;
        colors[1].pixel = bg;

        /* load pixmap */
        attrs.valuemask = XpmVisual | XpmColorSymbols;
        attrs.visual    = DefaultVisualOfScreen(XtScreen(w));
        attrs.colorsymbols = colors;
        attrs.numsymbols = 2;

	ret = XpmCreatePixmapFromData(display, RootWindowOfScreen(screen), 
			data, &pix, &mask, &attrs);

        XpmFreeAttributes(&attrs);
        if (mask != (Pixmap) NULL)
                XFreePixmap(display, mask);

        if (ret == XpmSuccess)
                (*pm) = pix;

        return (ret);
}

