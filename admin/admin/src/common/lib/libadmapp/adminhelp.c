
/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)adminhelp.c	1.17 95/01/04 Sun Microsystems"

/*	adminhelp.c	*/


#include <X11/Intrinsic.h>
#include <Xm/Form.h>
#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/PanedW.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleBG.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>
#include <nl_types.h>
#include "adminhelp.h"

/* these three definitions are used for selecting */
/* one of the three buttons in the radio widget	  */
#define TOP	0
#define HOW	1
#define REF	2

/* used for return values */
#define OK	0
#define NOTOPEN	1
#define EMPTYFILE 2
#define NOSEEK	3
#define NOREAD	4

#define DEFAULT_HELP_DIR "/usr/snadm/classes/locale"
#define FILETYPE "help"

/* used to create a linked list of filenames */
/* associated with a list of subjects        */
typedef struct list_t_tag {
	struct list_t_tag *next;
	char *s;
} list_t;

static void doneCB(void);
static void prevCB(void);
static void change_subjectCB(Widget, char*, XmListCallbackStruct*);
static void change_categoryCB(Widget, char, XmToggleButtonCallbackStruct*);

static int set_list(char, char*);
static int set_data(void);
static void syntax(char*, int, char*);
static Widget create_data(Widget);
static int create_widgets(Widget, char, char*);
static int read_subjfile(char*, int);

extern int errno;

/* these are used in callbacks and it was simplest to make 'em global */
static Widget AdminHelpShell;
static Widget radio[3];
static Widget subjlist;
static Widget helptext;

/* used to determine whether the AdminHelpShell was destroyed */
/* the last time help was called 			 */
static int help_exists = FALSE;

static char *Ghelphome;
static char *Gsubjfile;
static char *Gdir;
static int Gtoggle;
static char *Gcategory;
static int Gselection;
static char Grespath[256];
static char Gappname[64];
static char *Glang;

/* these are used for the previous button */
static int Gtoggle_old;
static char *Gsubjfile_old;
static char *Gdir_old;
static char *Gcategory_old;
static int Gselection_old;

/* for i18n messaging, catopen() and catgets() */
nl_catd		catdhelp;

/*
 * adminhelp is a simple form of online help.  it can display information
 * in the following categories:
 *	Topics		provides conceptual information about the
 *			program or task
 *	How To		provides step by step instructions for a task
 *	Reference	defines terms, describes values, etc.
 *
 * adminhelp expects the following files to live in the directory
 * indicated by the ADMINHELPHOME environment variable:
 *
 *		    $ADMINHELPHOME
 *		          |
 *	topics		howto		reference
 *	  |		  |		    |
 *	Topics		Howto		Reference
 *	datafiles	datafiles	datafiles
 *
 * Topics, Howto, and Reference are index files that contain a list of
 * subject and filename pairs:
 *	
 *	subject_string
 *		filename
 *
 * datafiles are the text files containing the help text - the filenames
 * must correspond to the filenames specified in the index files.
 *
 * adminhelp takes 3 parameters:
 *	a Widget	which will be the parent Widget of the popup
 *			shell that is created for the help window
 *	a char		which is one of 'C', 'P', or 'R' and indicates
 *			the help category (Topics, How To, and
 *			Reference, respectively).  there are #defines
 *			in adminhelp.h for these characters.
 *	a char *	which is the filename of the file that contains
 *			text on the desired subject.
 */

int
adminhelp(
	Widget	parent,
	char	help_category,
	char*	textfile
)
{
	static Widget previous_parent;
	static	catopened;
	int r;

	if (!catopened) {
		catdhelp = catopen("libadmapp.cat", 0);
		catopened = TRUE;
	}
	if (previous_parent != parent) {
		/* new parent widget - create a new shell */
		/* destroying the old one if necessary	  */
		if (help_exists)
			XtDestroyWidget(AdminHelpShell);

		if ((r = create_widgets(parent, help_category, textfile)) != OK)
			return(r);

		previous_parent = parent;
		help_exists = TRUE;
	}
	else if (!help_exists) {
		/* same parent - but no existing shell */
		/* so create one		       */
		if ((r = create_widgets(parent, help_category, textfile)) != OK)
			return(r);

		previous_parent = parent;
		help_exists = TRUE;
	}
	else {
		/* same parent, just re-do the subject list */
		/* and the displayed text		    */
		if (help_category) {
			XtVaSetValues(radio[Gtoggle], XmNset, FALSE, NULL);

			if ((r = set_list(help_category, textfile)) != OK)
				return(r);
			if ((r = set_data()) != 0)
				return(r);
		}
		/* if no category specified, use TOPIC */
		else {
			XtVaSetValues(radio[Gtoggle], XmNset, FALSE, NULL);

			if ((r = set_list(TOPIC, textfile)) != OK)
				return(r);
			if ((r = set_data()) != 0)
				return(r);
		}
	}
	return(OK);
}

	int
create_widgets(Widget parent, char help_category, char *textfile)
{
	Widget form, rowcol1, form2;
	Widget pane, sep;
	Widget prev, done;
	XmString xmstr;
	Atom wmdelete;
	int i, r;
	char*	appname;
	char*	appclass;

	XtGetApplicationNameAndClass(XtDisplay(parent), &appname, &appclass);
	strcpy(Gappname, appname);

	Ghelphome = getenv("ADMINHELPHOME");
	Glang = getenv("LANG") ? getenv("LANG") : "C";

	sprintf(Grespath, "%s/%%L/%%T/%s/%%N:%s/C/%%T/%s/%%N",
		(Ghelphome != NULL) ? Ghelphome : DEFAULT_HELP_DIR, Gappname,
		(Ghelphome != NULL) ? Ghelphome : DEFAULT_HELP_DIR, Gappname);

	AdminHelpShell = XtCreatePopupShell("AdminHelp",
		transientShellWidgetClass, parent, NULL, 0);
	XtVaSetValues(AdminHelpShell,
		XmNminWidth, 300,
		XmNminHeight, 155,
		XmNtitle, catgets(catdhelp, 10, 17, "Admin Help"),
		NULL);

	/* add a protocol callback to call the doneCB when */
	/* the window manager is used to quit the shell    */
	wmdelete = XmInternAtom(XtDisplay(AdminHelpShell),
		"WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(AdminHelpShell, wmdelete,
		(XtCallbackProc) doneCB, NULL);

	form = XtVaCreateWidget("helpform",
		xmFormWidgetClass, AdminHelpShell, NULL);

	/* create a rowcolumn for the category selection buttons */
	/* Topics, How To, and Reference			*/
	rowcol1 = XtVaCreateWidget("rowcol",
		xmRowColumnWidgetClass, form,
		XmNradioBehavior, True,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 4,
		XmNleftOffset, 4,
		NULL);

	/* create the category selection buttons */
	for (i = 0; i < 3; i++) {
		char c;
		char *label;

		switch (i) {
		  case TOP:
			c = TOPIC;
			label = catgets(catdhelp, 10, 11, "Topics");
			break;
		  case HOW:
			c = HOWTO;
			label = catgets(catdhelp, 10, 12, "How To");
			break;
		  case REF:
			c = REFER;
			label = catgets(catdhelp, 10, 13, "Reference");
			break;
		  default:
			break;
		}
		xmstr = XmStringCreateSimple(label);
		radio[i] = XtVaCreateManagedWidget("radio",
			xmToggleButtonGadgetClass, rowcol1,
			XmNlabelString, xmstr,
			NULL);
		XtAddCallback(radio[i], XmNvalueChangedCallback,
			(XtCallbackProc)change_categoryCB, (XtPointer) c);
	}

	XtManageChild(rowcol1);

	/* draw a vertical line next to the toggle buttons */
	sep = XtVaCreateManagedWidget("sep",
		xmSeparatorGadgetClass, form,
		XmNorientation, XmVERTICAL,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, rowcol1,
		XmNborderWidth, 12,
		NULL);

	/* create a rowcolumn thingy for the done and previous buttons */
	form2 = XtVaCreateWidget("form",
		/*xmRowColumnWidgetClass, form,*/
		xmFormWidgetClass, form,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 4,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, sep,
		XmNrightOffset, 4,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, rowcol1,
		XmNtopOffset, 4,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 4,
		NULL);

	/* create done and previous buttons */
	xmstr = XmStringCreateSimple(catgets(catdhelp, 10, 1, "Previous"));
	prev = XtVaCreateManagedWidget("previous",
		xmPushButtonGadgetClass, form2,
		XmNlabelString, xmstr,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XmStringFree(xmstr);
	XtAddCallback(prev, XmNactivateCallback, (XtCallbackProc)prevCB, NULL);

	xmstr = XmStringCreateSimple(catgets(catdhelp, 10, 2, "Done"));
	done = XtVaCreateManagedWidget("done",
		xmPushButtonGadgetClass, form2,
		XmNlabelString, xmstr,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XmStringFree(xmstr);
	XtAddCallback(done, XmNactivateCallback, (XtCallbackProc)doneCB, NULL);

	XtVaSetValues(prev,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, done,
		NULL);

	XtManageChild(form2);

	/* create a pane to hold the help subjects and help text */
	pane = XtVaCreateWidget("pane",
		xmPanedWindowWidgetClass, form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, sep,
		XmNspacing, 20,
		NULL);

	subjlist = XmCreateScrolledList(pane, "helpsubjs", NULL, 0);
	XtAddCallback(subjlist, XmNbrowseSelectionCallback,
		(XtCallbackProc)change_subjectCB, NULL);

	helptext = create_data(pane);

	if (help_category) {
		r = set_list(help_category, textfile);
	}
	else r = set_list(TOPIC, textfile);

	if (r == OK) {
		if ((r = set_data()) != OK)
			return(r);
	}
	else if (r != EMPTYFILE)
		return(r);

	XtManageChild(subjlist);
	XtManageChild(pane);

	XtManageChild(form);

	/* get it all displayed */
	XtRealizeWidget(AdminHelpShell);
	XtPopup(AdminHelpShell, XtGrabNone);
	return(OK);
}


	static 	int
set_list(char help_category, char *textfile)
{
	int r;

	/* save stuff for the previous button */
	Gdir_old = Gdir;
	Gtoggle_old = Gtoggle;
	Gcategory_old = Gcategory;
	Gselection_old = Gselection;

	/* set the appropriate toggle button and the */
	/* filename for the toc file		     */
	switch ((int)help_category) {
	  case TOPIC:
		Gtoggle = TOP;
		Gdir = "topics";
		Gcategory = "Topics";
		XtVaSetValues(radio[TOP], XmNset, TRUE, NULL);
		break;
	  case HOWTO:
		Gtoggle = HOW;
		Gdir = "howto";
		Gcategory = "Howto";
		XtVaSetValues(radio[HOW], XmNset, TRUE, NULL);
		break;
	  case REFER:
		Gtoggle = REF;
		Gdir = "reference";
		Gcategory = "Reference";
		XtVaSetValues(radio[REF], XmNset, TRUE, NULL);
		break;
	  default:
		break;
	}
	
	if ((r = read_subjfile(textfile, 0)) != OK)
		return(r);
	return(OK);
}

	int
read_subjfile(char *textfile, int selected)
{
	char tocfile[1000], label[1000], filename[1000], buf[1000];
	XmString xmstrlist[1000];
	list_t *head = NULL, *tail;
	int i, count;
	int is_an_error;
	FILE *fp;
	char* pn;
	int	top, visible;


	(void) sprintf(tocfile, "%s/%s", Gdir, Gcategory);
	pn = XtResolvePathname(XtDisplay(AdminHelpShell),
		FILETYPE, tocfile, NULL, Grespath, NULL, 0, NULL);
	if ((pn == NULL) || !(fp = fopen(pn, "r"))) {
		char* s;
		strcpy(buf, Grespath);
		s = strstr(buf, "%L");
		sprintf(s, "%s/%s/%s/%s", Glang, FILETYPE, Gappname, tocfile);
		(void) fprintf(stderr, catgets(catdhelp, 10, 3, "Can't open %s\n"), buf);
		return(NOTOPEN);
	}

	count = 0;
	for (i = 1; fgets(label, 1000, fp); i++) {
		if (*label == '!')
			continue;	/* it's a comment */
		if (*label == '\t') {
			syntax(tocfile, i, catgets(catdhelp, 10, 4, "unexpected leading tab"));
			continue;
		}
		if (*label == '\n') {	/* skip blank lines too */
			syntax(tocfile, i, catgets(catdhelp, 10, 5, "blank line"));
			continue;
		}
		*strchr(label, '\n') = '\0';
		i++;
		if (!fgets(buf, 1000, fp)) {
			syntax(tocfile, i, catgets(catdhelp, 10, 14, "unexpected end-of-file"));
			break;
		}
		/* keep reading 'til a valid filename line is found */
		is_an_error = 0;
		while (*buf != '\t') {
			i++;
			if (!fgets(buf, 1000, fp)) {
				syntax(tocfile, i, catgets(catdhelp, 10, 15, "unexpected end-of-file"));
				is_an_error = 1;
				break;
			}
		}
		if (is_an_error) break;

		if (sscanf(buf, "%s", filename) != 1)
			syntax(tocfile, i, catgets(catdhelp, 10, 6, "too many tokens"));

		/* if the caller did not supply a filename for the text    */
		/* of the subject, save the filename of the first subject. */
		/*otherwise, save the specified filename 		   */
		if (!selected) {
			if (!textfile) {
				Gsubjfile_old = Gsubjfile;
				Gsubjfile = strdup(filename);
				Gselection = selected = 1;
			}
			else if (strcmp(textfile, filename) == 0) {
				Gsubjfile_old = Gsubjfile;
				Gsubjfile = strdup(filename);
				Gselection = selected = count + 1;
			}	
		}

		xmstrlist[count] = XmStringCreateSimple(label);
		if(!head) {
			head = tail = (list_t *)malloc(sizeof(list_t));
			head->s = strdup(filename);
		} else {
			tail->next = (list_t *)malloc(sizeof(list_t));
			tail = tail->next;
			tail->s = strdup(filename);
		}
		tail->next = NULL;
		count++;
	}

	if (!selected) {
		if (count > 0) {
			/* filename was specified, but doesn't match	*/
			/* any entry in the file list.			*/
			Gsubjfile_old = Gsubjfile;
			Gsubjfile = strdup(textfile);
			selected = 1;
		}
		else {
			syntax(tocfile, 0, catgets(catdhelp, 10, 7, "empty file"));
			return(EMPTYFILE);
		}
	}

	XtVaSetValues(subjlist,
		XmNitems, xmstrlist,
		XmNitemCount, count,
		XmNuserData, head,
		NULL);
		
	/* select list entry, then scroll to make visible (if necessary) */
	XmListSelectPos(subjlist, selected, False);
	XtVaGetValues(subjlist,
		XmNtopItemPosition, &top,
		XmNvisibleItemCount, &visible,
		NULL);
	if (selected < top)
		XmListSetPos(subjlist, selected);
	else if (selected >= top + visible)
		XmListSetBottomPos(subjlist, selected);

	for(i = 0; i < count; ++i)
		XmStringFree(xmstrlist[i]);

	(void) fclose(fp);
	XtFree(pn);
	return(OK);
}

	static Widget
create_data(Widget parent)
{
	Arg args[10];
	int n = 0;
	Widget text;
	
	XtSetArg(args[n], XmNeditable, False); n++;
	XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
	XtSetArg(args[n], XmNscrollHorizontal, False); n++;
	XtSetArg(args[n], XmNcursorPositionVisible, False); n++;

	/* these next three don't seem to be working */
	/* a scrolled text widget appears to force   */
	/* different values so that automatic scroll */
	/* don't work				     */
	XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;
	XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmAS_NEEDED); n++;
	XtSetArg(args[n], XmNvisualPolicy, XmCONSTANT); n++;

	text = XmCreateScrolledText(parent, "helptext", args, n);

	XtManageChild(text);

	return(text);
}

	static int
set_data(void)
{
	int ret;
	char *buf;
	char textfile[1000], errbuf[256], tmp[256];
	FILE *fp;
	long len;
	char* pn;

	(void) sprintf(textfile, "%s/%s", Gdir, Gsubjfile);
	pn = XtResolvePathname(XtDisplay(AdminHelpShell),
		FILETYPE, textfile, NULL, Grespath, NULL, 0, NULL);
	if ((pn == NULL) || !(fp = fopen(pn, "r"))) {
		char* s;
		strcpy(tmp, Grespath);
		s = strstr(tmp, "%L");
		sprintf(s, "%s/%s/%s/%s", Glang, FILETYPE, Gappname, textfile);
		(void) sprintf(errbuf, catgets(catdhelp, 10, 16, "File not found:\n  %s\n"), tmp);
		XtVaSetValues(helptext, XmNvalue, errbuf, NULL);
	} else {
		if (fseek(fp, 0L, SEEK_END) == -1) {
			(void) fprintf(stderr,
				catgets(catdhelp, 10, 8, "can't seek in file: %s\n"),
				textfile);
			return(NOSEEK);
		}
		len = ftell(fp);
		rewind(fp);
		buf = (char *)malloc((size_t)len+1);
		ret = fread(buf, (size_t)len+1, (size_t)1, fp);
		if ((ret == 0) && ferror(fp)) {
			(void) fprintf(stderr,
				catgets(catdhelp, 10, 9, "can't read file: %s\n"),
				textfile);
			return(NOREAD);
		}
		buf[len] = '\0';
		XtVaSetValues(helptext, XmNvalue, buf, NULL);
		free(buf);
		(void) fclose(fp);
		XtFree(pn);
	}
	return(OK);
}

	static void
doneCB(void)
{
	XtDestroyWidget(AdminHelpShell);
	help_exists = FALSE;
}

	static void
prevCB(void)
{
	int i;
	char *tmpdir, *tmpfn, *tmpcat;
	int tmpselection;

	if (Gdir_old != NULL) {
		i = Gtoggle_old;
		tmpdir = Gdir_old;
		tmpfn = Gsubjfile_old;
		tmpcat = Gcategory_old;
		tmpselection = Gselection_old;

		Gtoggle_old = Gtoggle;
		Gdir_old = Gdir;
		Gsubjfile_old = Gsubjfile;
		Gcategory_old = Gcategory;
		Gselection_old = Gselection;

		Gtoggle = i;
		Gdir = tmpdir;
		Gsubjfile = tmpfn;
		Gcategory = tmpcat;
		Gselection = tmpselection;

		if (read_subjfile(Gcategory, Gselection) == OK)
			(void) set_data();
		XtVaSetValues(radio[Gtoggle_old], XmNset, FALSE, NULL);
		XtVaSetValues(radio[Gtoggle], XmNset, TRUE, NULL);
		XmListSetBottomPos(subjlist, Gselection);
	}
}

	static void
change_categoryCB(Widget w, char c, XmToggleButtonCallbackStruct *cbs)
{
	list_t *ptr, *sptr;

	if (!cbs->set)
		return;

	if ( (((int)c == TOPIC) && (Gtoggle == TOP)) ||
	     (((int)c == HOWTO) && (Gtoggle == HOW)) ||
	     (((int)c == REFER) && (Gtoggle == REF)) )
		return;

	/* get the pointer to the list of filenames */
	/* for the current contents of the widget   */
	XtVaGetValues(subjlist, XmNuserData, &ptr, NULL);

	if (set_list(c, (char *)NULL) == OK) {
		(void) set_data();

		/* free the list of filenames previously */
		/* attached to the list widget		 */
		sptr = ptr;
		while (sptr != NULL) {
			ptr = sptr->next;
			free(sptr);
			sptr = ptr;
		}
	}
}
	
	static void
change_subjectCB(Widget w, char *unused, XmListCallbackStruct *cbs)
{
	list_t *ptr;
	int i;

	if (cbs->item_position == Gselection)
		return;

	/* save things in case the previous button is used */
	Gtoggle_old = Gtoggle;
	Gsubjfile_old = Gsubjfile;
	Gdir_old = Gdir;
	Gcategory_old = Gcategory;
	Gselection_old = Gselection;

	Gselection = cbs->item_position;

	/* get the pointer to the list of filenames */
	XtVaGetValues(w, XmNuserData, &ptr, NULL);

	/* find the filename for the current */
	/* position in the list		     */
	for (i = 1; i < cbs->item_position; i++)
		ptr = ptr->next;

	Gsubjfile = ptr->s;

	(void) set_data();
}

	static void
syntax(char *filename, int line, char *string)
{
	(void) fprintf(stderr,
	catgets(catdhelp, 10, 10, "syntax error, file=%s\n\tline number=%d, \t=%s=\n"),
		filename, line, string);
}

