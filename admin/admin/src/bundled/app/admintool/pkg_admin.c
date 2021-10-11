
/* Copyright 1995 Sun Microsystems, Inc. */

#pragma ident "@(#)pkg_admin.c	1.4 96/08/06 Sun Microsystems"

/*	pkg_admin.c */

#include <stdlib.h>
#include <stdio.h>
#include <libintl.h>
#include <nl_types.h>
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/DialogS.h>
#include <Xm/LabelG.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/PushBG.h>
#include <Xm/ScrolledW.h>
#include <Xm/List.h>
#include <Xm/TextF.h>

#include "sysman_iface.h"
#include "util.h"
#include "UxXt.h"

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

typedef	struct {
	Widget	pkgAdminDialog;
	Widget	mailListLabel;
	Widget	mailList;
	Widget	mailText;
	Widget	addButton;
	Widget	deleteButton;
	Widget	existFilesLabel;
	Widget	existFilesOptionMenu;
	Widget	existPkgLabel;
	Widget	existPkgOptionMenu;
	Widget	existPartialLabel;
	Widget	existPartialOptionMenu;
	Widget	installSetuidLabel;
	Widget	installSetuidOptionMenu;
	Widget	runSetuidLabel;
	Widget	runSetuidOptionMenu;
	Widget	installDependLabel;
	Widget	installDependOptionMenu;
	Widget	removeDependLabel;
	Widget	removeDependOptionMenu;
	Widget	runLevelLabel;
	Widget	runLevelOptionMenu;
	Widget	insufficientSpaceLabel;
	Widget	insufficientSpaceOptionMenu;
	Widget	showCopyrightsLabel;
	Widget	showCopyrightsOptionMenu;
	Widget	interactiveLabel;
	Widget	interactiveOptionMenu;
	Widget  removeModeLabel;	
	Widget  removeModeOptionMenu;
	Widget	okPushbutton;
	Widget	resetPushbutton;
	Widget	cancelPushbutton;
	Widget	helpPushbutton;

	Widget	save_exist_files;
	Widget	save_exist_pkgs;
	Widget	save_exist_partial;
	Widget	save_install_setuid;
	Widget	save_run_setuid;
	Widget	save_install_depend;
	Widget	save_remove_depend;
	Widget	save_runlevel;
	Widget	save_insuf_space;
	Widget	save_show_cr;
	Widget	save_interactive;
	int		save_mail_list_size;
	XmStringTable	save_mail_list;
} pkgAdminCtxt;

static void init_dialog(Widget dialog);
static void save_dialog_values(Widget dialog);

static char	errstr[1024] = "";

Widget 	pkgadmindialog = NULL;

/* Label strings */
#define ASK_L		catgets(_catd, 8, 770, "Ask")
#define OVERWRITE_L	catgets(_catd, 8, 771, "Overwrite")
#define UNIQUE_L	catgets(_catd, 8, 772, "Install Unique")
#define SKIP_L		catgets(_catd, 8, 773, "Skip")
#define IGNORE_L	catgets(_catd, 8, 774, "Ignore")
#define ABORT_L		catgets(_catd, 8, 775, "Abort")
#define YES_L		catgets(_catd, 8, 776, "Yes")
#define NO_L		catgets(_catd, 8, 777, "No")

/* Special Labels for remove modes */

#define ALL_L           catgets(_catd, 8, 780, "All Instances")
#define SINGLE_L        catgets(_catd, 8, 781, "Single Instance")

/* admin file token strings */
#define ASK_T		"ask"
#define OVERWRITE_T	"overwrite"
#define UNIQUE_T	"unique"
#define NOCHANGE_T	"nochange"
#define NOCHECK_T	"nocheck"
#define QUIT_T		"quit"
#define YES_T		"yes"
#define NO_T		"no"
#define DEFAULT_T	"default"

/* Special tokens for remove modes */
#define ALL_T		"all"
#define SINGLE_T	"single"

/*
 * The following label and token array pairs MUST match in size and
 * must agree with corresponding settings in init_strings().
 */
char * ExistFilesLabels[5];
char * ExistFilesTokens[5];

char * ExistPkgsLabels[5];
char * ExistPkgsTokens[5];

char * ExistPartialLabels[4];
char * ExistPartialTokens[4];

char * InstallSetuidLabels[5];
char * InstallSetuidTokens[5];

char * RunSetuidLabels[4];
char * RunSetuidTokens[4];

char * InstallDependLabels[4];
char * InstallDependTokens[4];

char * RemoveDependLabels[4];
char * RemoveDependTokens[4];

char * RunLevelLabels[4];
char * RunLevelTokens[4];

char * InsufSpaceLabels[4];
char * InsufSpaceTokens[4];

char * ShowCopyrightsLabels[3];
char * ShowCopyrightsTokens[3];

char * InteractiveLabels[3];
char * InteractiveTokens[3];

char * RemoveModeLabels[3];
char * RemoveModeTokens[3];


static PkgAdminProps adminFile = {
	NULL,		/* mail        */
	UNIQUE_T,	/* instance    */
	ASK_T,		/* partial     */
	ASK_T,		/* runlevel    */
	ASK_T,		/* idepend     */
	ASK_T,		/* rdepend     */
	ASK_T,		/* space       */
	ASK_T,		/* setuid      */
	ASK_T,		/* confilct    */
	ASK_T,		/* action      */
	DEFAULT_T,	/* basedir     */
	YES_T,		/* showcr      */
	YES_T		/* interactive */
};


/*******************************************************************************
*******************************************************************************/

void
get_admin_file_values(PkgAdminProps* props) {
	props->mail        = adminFile.mail;
	props->instance    = adminFile.instance;
	props->partial     = adminFile.partial;
	props->runlevel    = adminFile.runlevel;
	props->idepend     = adminFile.idepend;
	props->rdepend     = adminFile.rdepend;
	props->space       = adminFile.space;
	props->setuid      = adminFile.setuid;
	props->conflict    = adminFile.conflict;
	props->action      = adminFile.action;
	props->basedir     = adminFile.basedir;
	props->showcr      = adminFile.showcr;
	props->interactive = adminFile.interactive;
}

/*
 * Read a file in admin(4) format and populate global adminFile
 * struct. I allow whitespace which is actually more flexible than
 * admin(4) allows.
 *
 * Return 0 if succesful, otherwise -1
 */

int
load_admin_file(char* file)
{
    int n;
    char* p, *v;
    char parm[64], *value;

    FILE * fp = fopen(file, "r");
    if (fp == NULL)
        return(-1);
    while (!feof(fp)) {
	if (fgets(parm, 64, fp) == NULL)
                continue;

        parm[strlen(parm)-1] = '\0'; /* overwrite the nl */
        p = parm;

        while (isspace(*p)) /* strip leading white */
                p++;

        v = value = strchr(p, '=');
        if (value == NULL)
            continue;

        v--;
        while (isspace(*v)) /* skip trailing white on 'parm' */
            v--;

        *(v+1) = '\0'; /* terminate parm string */

        *value = '\0';
        value++;

        while (isspace(*value)) /* strip leading white off value */
                value++;

        v = value + strlen(value) - 1;

        while (isspace(*v))   /* strip trailing white off value */
                v--;

        *(v+1) = '\0';    /* terminate value string */

	if (strcmp(p, "mail") == 0)
            adminFile.mail = strdup(value);
        else if (strcmp(p, "instance") == 0)
            adminFile.instance = strdup(value);
        else if (strcmp(p, "partial") == 0)
            adminFile.partial = strdup(value);
        else if (strcmp(p, "runlevel") == 0)
            adminFile.runlevel = strdup(value);
        else if (strcmp(p, "idepend") == 0)
            adminFile.idepend = strdup(value);
        else if (strcmp(p, "rdepend") == 0)
            adminFile.rdepend = strdup(value);
        else if (strcmp(p, "space") == 0)
            adminFile.space = strdup(value);
        else if (strcmp(p, "setuid") == 0)
            adminFile.setuid = strdup(value);
        else if (strcmp(p, "conflict") == 0)
            adminFile.conflict = strdup(value);
        else if (strcmp(p, "action") == 0)
            adminFile.action = strdup(value);
        else if (strcmp(p, "basedir") == 0)
            adminFile.basedir = strdup(value);
    }
    return (0);
}


static void
okCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	pkgAdminCtxt* ctxt = (pkgAdminCtxt*)cd;
	Widget		w;
	int		i;
	char*		tmp;
	int		buflen;
	char *		maillist = NULL;
	int		listcount = 0;
	XmStringTable	list = NULL;


	SetBusyPointer(True);

	XtVaGetValues(ctxt->existFilesOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &adminFile.conflict,
		NULL);

	XtVaGetValues(ctxt->existPkgOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &adminFile.instance,
		NULL);

	XtVaGetValues(ctxt->existPartialOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &adminFile.partial,
		NULL);

	XtVaGetValues(ctxt->installSetuidOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &adminFile.setuid,
		NULL);

	XtVaGetValues(ctxt->runSetuidOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &adminFile.action,
		NULL);

	XtVaGetValues(ctxt->installDependOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &adminFile.idepend,
		NULL);

	XtVaGetValues(ctxt->removeDependOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &adminFile.rdepend,
		NULL);

	XtVaGetValues(ctxt->runLevelOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &adminFile.runlevel,
		NULL);

	XtVaGetValues(ctxt->insufficientSpaceOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &adminFile.space,
		NULL);

	XtVaGetValues(ctxt->showCopyrightsOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &adminFile.showcr,
		NULL);

	XtVaGetValues(ctxt->interactiveOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &adminFile.interactive,
		NULL);

	XtVaGetValues(ctxt->mailList,
		XmNitemCount, &listcount,
		XmNitems, &list,
		NULL);
	if (adminFile.mail != NULL) {
		free(adminFile.mail);
		adminFile.mail = NULL;
	}
	if (list && listcount) {
		buflen = 0;
		for (i=0; i<listcount; i++) {
			XmStringGetLtoR(
				list[i], XmSTRING_DEFAULT_CHARSET, &tmp);
			buflen += strlen(tmp) + 1;
			XtFree(tmp);
		}

		maillist = (char*)malloc(buflen + 1);
		maillist[0] = '\0';

		for (i=0; i<listcount; i++) {
			XmStringGetLtoR(
				list[i], XmSTRING_DEFAULT_CHARSET, &tmp);
			if (i > 0) {
				strcat(maillist, ",");
			}
			strcat(maillist, tmp);
			XtFree(tmp);
		}

		adminFile.mail = maillist;
	}

	SetBusyPointer(False);

	XtPopdown(XtParent(ctxt->pkgAdminDialog));
}

static void
resetCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	pkgAdminCtxt* ctxt = (pkgAdminCtxt*)cd;


	XtVaSetValues(ctxt->existFilesOptionMenu,
		XmNmenuHistory, ctxt->save_exist_files,
		NULL);
	XtVaSetValues(ctxt->existPkgOptionMenu,
		XmNmenuHistory, ctxt->save_exist_pkgs,
		NULL);
	XtVaSetValues(ctxt->existPartialOptionMenu,
		XmNmenuHistory, ctxt->save_exist_partial,
		NULL);
	XtVaSetValues(ctxt->installSetuidOptionMenu,
		XmNmenuHistory, ctxt->save_install_setuid,
		NULL);
	XtVaSetValues(ctxt->runSetuidOptionMenu,
		XmNmenuHistory, ctxt->save_run_setuid,
		NULL);
	XtVaSetValues(ctxt->installDependOptionMenu,
		XmNmenuHistory, ctxt->save_install_depend,
		NULL);
	XtVaSetValues(ctxt->removeDependOptionMenu,
		XmNmenuHistory, ctxt->save_remove_depend,
		NULL);
	XtVaSetValues(ctxt->runLevelOptionMenu,
		XmNmenuHistory, ctxt->save_runlevel,
		NULL);
	XtVaSetValues(ctxt->insufficientSpaceOptionMenu,
		XmNmenuHistory, ctxt->save_insuf_space,
		NULL);
	XtVaSetValues(ctxt->showCopyrightsOptionMenu,
		XmNmenuHistory, ctxt->save_show_cr,
		NULL);
	XtVaSetValues(ctxt->interactiveOptionMenu,
		XmNmenuHistory, ctxt->save_interactive,
		NULL);

	XtVaSetValues(ctxt->mailList,
		XmNitemCount, ctxt->save_mail_list_size,
		XmNitems, ctxt->save_mail_list,
		NULL);
}

static void
cancelCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	pkgAdminCtxt* ctxt = (pkgAdminCtxt*)cd;

	XtPopdown(XtParent(ctxt->pkgAdminDialog));
}

static void
TextFocusCB(
	Widget wgt, 
	XtPointer cd, 
	XmAnyCallbackStruct* cbs)
{
	pkgAdminCtxt* ctxt = (pkgAdminCtxt*)cd;

	/* When the text field gets focus, disable the default
	 * pushbutton for the dialog.  This allows the text field's
	 * activate callback to work without dismissing the dialog.
	 * Re-enable default pushbutton when losing focus.
	 */

	if (cbs->reason == XmCR_FOCUS) {
		XtVaSetValues(ctxt->pkgAdminDialog,
			XmNdefaultButton, NULL,
			NULL);
	}
	else if (cbs->reason == XmCR_LOSING_FOCUS) {
		XtVaSetValues(ctxt->pkgAdminDialog,
			XmNdefaultButton, ctxt->okPushbutton,
			NULL);
	}

}

static Widget
make_option_menu(Widget parent, char** labels, char** tokens)
{
	Widget		menushell;
	Widget		pulldown;
	Widget		w;
	XmString	xstr;
	char**		l;
	char**		t;

	menushell = XtVaCreatePopupShell ("menushell",
		xmMenuShellWidgetClass, parent,
		XmNwidth, 1,
		XmNheight, 1,
		XmNallowShellResize, TRUE,
		XmNoverrideRedirect, TRUE,
		NULL );

	pulldown = XtVaCreateWidget( "pulldown",
		xmRowColumnWidgetClass,
		menushell,
		XmNrowColumnType, XmMENU_PULLDOWN,
		NULL );

	for (l = labels, t = tokens; *l != NULL; l++, t++) {
		xstr = XmStringCreateLocalized(*l);
		w = XtVaCreateManagedWidget(*t,
			xmPushButtonGadgetClass,
			pulldown,
			XmNlabelString, xstr,
			XmNuserData, *t,
			NULL );
		XmStringFree(xstr);
	}

	return pulldown;
}

/*
 * Strings values must be initialized dynamically due to catgets() call.
 */
static void
init_strings(void)
{
	int n;

	n = 0;
	ExistFilesLabels[n] = ASK_L;       ExistFilesTokens[n++] = ASK_T;
	ExistFilesLabels[n] = OVERWRITE_L; ExistFilesTokens[n++] = NOCHECK_T;
	ExistFilesLabels[n] = SKIP_L;      ExistFilesTokens[n++] = NOCHANGE_T;
	ExistFilesLabels[n] = ABORT_L;     ExistFilesTokens[n++] = QUIT_T;
	ExistFilesLabels[n] = NULL;        ExistFilesTokens[n++] = NULL;

	n = 0;
	ExistPkgsLabels[n] = ASK_L;       ExistPkgsTokens[n++] = ASK_T;
	ExistPkgsLabels[n] = OVERWRITE_L; ExistPkgsTokens[n++] = OVERWRITE_T;
	ExistPkgsLabels[n] = UNIQUE_L;    ExistPkgsTokens[n++] = UNIQUE_T;
	ExistPkgsLabels[n] = ABORT_L;     ExistPkgsTokens[n++] = QUIT_T;
	ExistPkgsLabels[n] = NULL;        ExistPkgsTokens[n++] = NULL;

	n = 0;
	ExistPartialLabels[n] = ASK_L;    ExistPartialTokens[n++] = ASK_T;
	ExistPartialLabels[n] = IGNORE_L; ExistPartialTokens[n++] = NOCHECK_T;
	ExistPartialLabels[n] = ABORT_L;  ExistPartialTokens[n++] = QUIT_T;
	ExistPartialLabels[n] = NULL;     ExistPartialTokens[n++] = NULL;

	n = 0;
	InstallSetuidLabels[n] = ASK_L;   InstallSetuidTokens[n++] = ASK_T;
	InstallSetuidLabels[n] = YES_L;   InstallSetuidTokens[n++] = NOCHECK_T;
	InstallSetuidLabels[n] = NO_L;    InstallSetuidTokens[n++] = NOCHANGE_T;
	InstallSetuidLabels[n] = ABORT_L; InstallSetuidTokens[n++] = QUIT_T;
	InstallSetuidLabels[n] = NULL;    InstallSetuidTokens[n++] = NULL;

	n = 0;
	RunSetuidLabels[n] = ASK_L;   RunSetuidTokens[n++] = ASK_T;
	RunSetuidLabels[n] = YES_L;   RunSetuidTokens[n++] = NOCHECK_T;
	RunSetuidLabels[n] = ABORT_L; RunSetuidTokens[n++] = QUIT_T;
	RunSetuidLabels[n] = NULL;    RunSetuidTokens[n++] = NULL;

	n = 0;
	InstallDependLabels[n] = ASK_L;    InstallDependTokens[n++] = ASK_T;
	InstallDependLabels[n] = IGNORE_L; InstallDependTokens[n++] = NOCHECK_T;
	InstallDependLabels[n] = ABORT_L;  InstallDependTokens[n++] = QUIT_T;
	InstallDependLabels[n] = NULL;     InstallDependTokens[n++] = NULL;

	n = 0;
	RemoveDependLabels[n] = ASK_L;    RemoveDependTokens[n++] = ASK_T;
	RemoveDependLabels[n] = IGNORE_L; RemoveDependTokens[n++] = NOCHECK_T;
	RemoveDependLabels[n] = ABORT_L;  RemoveDependTokens[n++] = QUIT_T;
	RemoveDependLabels[n] = NULL;     RemoveDependTokens[n++] = NULL;

	n = 0;
	RunLevelLabels[n] = ASK_L;    RunLevelTokens[n++] = ASK_T;
	RunLevelLabels[n] = IGNORE_L; RunLevelTokens[n++] = NOCHECK_T;
	RunLevelLabels[n] = ABORT_L;  RunLevelTokens[n++] = QUIT_T;
	RunLevelLabels[n] = NULL;     RunLevelTokens[n++] = NULL;

	n = 0;
	InsufSpaceLabels[n] = ASK_L;    InsufSpaceTokens[n++] = ASK_T;
	InsufSpaceLabels[n] = IGNORE_L; InsufSpaceTokens[n++] = NOCHECK_T;
	InsufSpaceLabels[n] = ABORT_L;  InsufSpaceTokens[n++] = QUIT_T;
	InsufSpaceLabels[n] = NULL;     InsufSpaceTokens[n++] = NULL;

	n = 0;
	ShowCopyrightsLabels[n] = YES_L; ShowCopyrightsTokens[n++] = YES_T;
	ShowCopyrightsLabels[n] = NO_L;  ShowCopyrightsTokens[n++] = NO_T;
	ShowCopyrightsLabels[n] = NULL;  ShowCopyrightsTokens[n++] = NULL;

	n = 0;
	InteractiveLabels[n] = YES_L; InteractiveTokens[n++] = YES_T;
	InteractiveLabels[n] = NO_L;  InteractiveTokens[n++] = NO_T;
	InteractiveLabels[n] = NULL;  InteractiveTokens[n++] = NULL;

	n = 0;
	RemoveModeLabels[n] = SINGLE_L; RemoveModeTokens[n++] = SINGLE_T;
	RemoveModeLabels[n] = ALL_L;  RemoveModeTokens[n++] = ALL_T;
	RemoveModeLabels[n] = NULL;  RemoveModeTokens[n++] = NULL;
}

Widget	build_pkgAdminDialog(Widget parent)
{
	pkgAdminCtxt*	ctxt;
	Widget		shell;
	Widget		menushell;
	Widget		pulldown;
	Widget		bigrc;
	Widget		form;
	Widget		scrollwin;
	Widget		tmp;
	Widget		bbox;
	char **		s;
	XmString	xstr;
	Widget		w;
	int		i;
	int		wnum;
	Widget		wlist[16];
	Widget		maxlabel;
	Dimension	width;
	Dimension	maxwidth = 0;


	init_strings();

	ctxt = (pkgAdminCtxt*) malloc(sizeof(pkgAdminCtxt));
	memset((void*)ctxt, 0, sizeof(pkgAdminCtxt));

	if (parent == NULL) {
		parent = GtopLevel;
	}

	shell = XtVaCreatePopupShell( "AddHostDialog_shell",
		xmDialogShellWidgetClass, parent,
		XmNshellUnitType, XmPIXELS,
		XmNallowShellResize, True,
		NULL );

	ctxt->pkgAdminDialog = XtVaCreateWidget( "AddHostDialog",
		xmFormWidgetClass,
		shell,
		XmNunitType, XmPIXELS,
		RES_CONVERT(XmNdialogTitle,
		  catgets(_catd, 8, 750, "Admintool: Package Adminstration")),
		NULL );

	XtVaSetValues(ctxt->pkgAdminDialog,
		XmNuserData, (XtPointer)ctxt,
		NULL);

	bigrc = XtVaCreateManagedWidget( "bigrc",
		xmRowColumnWidgetClass,
		ctxt->pkgAdminDialog,
		XmNorientation, XmVERTICAL,
		XmNnumColumns, 1,
		XmNspacing, 2,
		XmNmarginHeight, 1,
		XmNmarginWidth, 1,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 10,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 1,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 10,
		NULL );

	/* Existing Files */
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		NULL );

	ctxt->existFilesLabel = XtVaCreateManagedWidget( "existFilesLabel",
		xmLabelGadgetClass,
		form,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 751, "Existing Files:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	pulldown = make_option_menu(ctxt->pkgAdminDialog,
		ExistFilesLabels, ExistFilesTokens);

	ctxt->existFilesOptionMenu = XtVaCreateManagedWidget("existFilesOptionMenu",
		xmRowColumnWidgetClass,
		form,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->existFilesLabel,
		XmNleftOffset, 0,
		NULL );

	/* Existing Packages */
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		NULL );

	ctxt->existPkgLabel = XtVaCreateManagedWidget( "existPkgLabel",
		xmLabelGadgetClass,
		form,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 752, "Existing Packages:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	pulldown = make_option_menu(ctxt->pkgAdminDialog,
		ExistPkgsLabels, ExistPkgsTokens);

	ctxt->existPkgOptionMenu = XtVaCreateManagedWidget("existPkgOptionMenu",
		xmRowColumnWidgetClass,
		form,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->existPkgLabel,
		XmNleftOffset, 0,
		NULL );

	/* Existing Partial Installations */
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		NULL );

	ctxt->existPartialLabel = XtVaCreateManagedWidget( "existPartialLabel",
		xmLabelGadgetClass,
		form,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString,
		   catgets(_catd, 8, 753, "Existing Partial Installations:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	pulldown = make_option_menu(ctxt->pkgAdminDialog,
		ExistPartialLabels, ExistPartialTokens);

	ctxt->existPartialOptionMenu = XtVaCreateManagedWidget("existPartialOptionMenu",
		xmRowColumnWidgetClass,
		form,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->existPartialLabel,
		XmNleftOffset, 0,
		NULL );

	/* Install setuid/setgid Files */
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		NULL );

	ctxt->installSetuidLabel = XtVaCreateManagedWidget("installSetuidLabel",
		xmLabelGadgetClass,
		form,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 754, "Install setuid/setgid Files:")),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	pulldown = make_option_menu(ctxt->pkgAdminDialog,
		InstallSetuidLabels, InstallSetuidTokens);

	ctxt->installSetuidOptionMenu = XtVaCreateManagedWidget("installSetuidOptionMenu",
		xmRowColumnWidgetClass,
		form,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->installSetuidLabel,
		XmNleftOffset, 0,
		NULL );

	/* Run setuid/setgid Scripts */
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		NULL );

	ctxt->runSetuidLabel = XtVaCreateManagedWidget("runSetuidLabel",
		xmLabelGadgetClass,
		form,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 755, "Run setuid/setgid Scripts:")),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	pulldown = make_option_menu(ctxt->pkgAdminDialog,
		RunSetuidLabels, RunSetuidTokens);

	ctxt->runSetuidOptionMenu = XtVaCreateManagedWidget("runSetuidOptionMenu",
		xmRowColumnWidgetClass,
		form,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->runSetuidLabel,
		XmNleftOffset, 0,
		NULL );

	/* Installation Dependencies Not Met */
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		NULL );

	ctxt->installDependLabel = XtVaCreateManagedWidget("installDependLabel",
		xmLabelGadgetClass,
		form,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 756, "Installation Dependencies Not Met:")),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	pulldown = make_option_menu(ctxt->pkgAdminDialog,
		InstallDependLabels, InstallDependTokens);

	ctxt->installDependOptionMenu = XtVaCreateManagedWidget("installDependOptionMenu",
		xmRowColumnWidgetClass,
		form,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->installDependLabel,
		XmNleftOffset, 0,
		NULL );

	/* Removal Dependencies Not Met */
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		NULL );

	ctxt->removeDependLabel = XtVaCreateManagedWidget("removeDependLabel",
		xmLabelGadgetClass,
		form,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 757, "Removal Dependencies Not Met:")),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	pulldown = make_option_menu(ctxt->pkgAdminDialog,
		RemoveDependLabels, RemoveDependTokens);

	ctxt->removeDependOptionMenu = XtVaCreateManagedWidget("removeDependOptionMenu",
		xmRowColumnWidgetClass,
		form,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->removeDependLabel,
		XmNleftOffset, 0,
		NULL );

	/* Incorrect Run Level */
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		NULL );

	ctxt->runLevelLabel = XtVaCreateManagedWidget("runLevelLabel",
		xmLabelGadgetClass,
		form,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 758, "Incorrect Run Level:")),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	pulldown = make_option_menu(ctxt->pkgAdminDialog,
		RunLevelLabels, RunLevelTokens);

	ctxt->runLevelOptionMenu = XtVaCreateManagedWidget("runLevelOptionMenu",
		xmRowColumnWidgetClass,
		form,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->runLevelLabel,
		XmNleftOffset, 0,
		NULL );

	/* Insufficient Space */
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		NULL );

	ctxt->insufficientSpaceLabel = XtVaCreateManagedWidget("insufficientSpaceLabel",
		xmLabelGadgetClass,
		form,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 759, "Insufficient Space:")),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	pulldown = make_option_menu(ctxt->pkgAdminDialog,
		InsufSpaceLabels, InsufSpaceTokens);

	ctxt->insufficientSpaceOptionMenu = XtVaCreateManagedWidget("insufficientSpaceOptionMenu",
		xmRowColumnWidgetClass,
		form,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->insufficientSpaceLabel,
		XmNleftOffset, 0,
		NULL );

	/* Show Copyrights */
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		NULL );

	ctxt->showCopyrightsLabel = XtVaCreateManagedWidget("showCopyrightsLabel",
		xmLabelGadgetClass,
		form,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 760, "Show Copyrights:")),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	pulldown = make_option_menu(ctxt->pkgAdminDialog,
		ShowCopyrightsLabels, ShowCopyrightsTokens);

	ctxt->showCopyrightsOptionMenu = XtVaCreateManagedWidget("showCopyrightsOptionMenu",
		xmRowColumnWidgetClass,
		form,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->showCopyrightsLabel,
		XmNleftOffset, 0,
		NULL );

	/* Install/Remove Interactively */
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		NULL );

	ctxt->interactiveLabel = XtVaCreateManagedWidget("interactiveLabel",
		xmLabelGadgetClass,
		form,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 761, "Install/Remove Interactively:")),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	pulldown = make_option_menu(ctxt->pkgAdminDialog,
		InteractiveLabels, InteractiveTokens);

	ctxt->interactiveOptionMenu = XtVaCreateManagedWidget("interactiveOptionMenu",
		xmRowColumnWidgetClass,
		form,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->interactiveLabel,
		XmNleftOffset, 0,
		NULL );

	/* Removal mode */
#ifdef REMOVE_PKG_INSTANCES
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		NULL );

	ctxt->removeModeLabel = XtVaCreateManagedWidget("removeModeLabel",
		xmLabelGadgetClass,
		form,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 779, "Remove Mode:")),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	pulldown = make_option_menu(ctxt->pkgAdminDialog,
		RemoveModeLabels, RemoveModeTokens);

	ctxt->removeModeOptionMenu = XtVaCreateManagedWidget("removeModeOptionMenu",
		xmRowColumnWidgetClass,
		form,
		XmNrowColumnType, XmMENU_OPTION,
		XmNsubMenuId, pulldown,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->removeModeLabel,
		XmNleftOffset, 0,
		NULL );

#endif /* remove pkg instances */

	/* Mail Recipients */
	form = XtVaCreateManagedWidget( "form",
		xmFormWidgetClass,
		ctxt->pkgAdminDialog,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, bigrc,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 15,
		NULL );

	ctxt->mailListLabel = XtVaCreateManagedWidget( "mailListLabel",
		xmLabelGadgetClass,
		form,
		RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 762, "Mail Recipients:") ),
		XmNalignment, XmALIGNMENT_END,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		NULL );

	scrollwin = XtVaCreateManagedWidget( "scrollwin",
		xmScrolledWindowWidgetClass,
		form,
		XmNscrollingPolicy, XmAPPLICATION_DEFINED,
		XmNvisualPolicy, XmVARIABLE,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNshadowThickness, 0,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->mailListLabel,
		XmNleftOffset, 10,
		NULL );

	ctxt->mailList = XtVaCreateManagedWidget( "mailList",
		xmListWidgetClass,
		scrollwin,
		XmNvisibleItemCount, 3,
		XmNselectionPolicy, XmEXTENDED_SELECT,
		NULL );

	ctxt->mailText = XtVaCreateManagedWidget( "mailText",
		xmTextFieldWidgetClass,
		form,
		XmNmaxLength, MAXUSERNAMELEN,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->mailListLabel,
		XmNleftOffset, 10,
		NULL );
	XtAddCallback( ctxt->mailText, XmNfocusCallback,
		(XtCallbackProc) TextFocusCB,
		(XtPointer) ctxt );
	XtAddCallback( ctxt->mailText, XmNlosingFocusCallback,
		(XtCallbackProc) TextFocusCB,
		(XtPointer) ctxt );
	XtAddCallback( ctxt->mailText, XmNactivateCallback,
		(XtCallbackProc) addListEntryCB,
		(XtPointer) ctxt->mailText );

	XtVaSetValues(scrollwin,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, ctxt->mailText,
		XmNbottomOffset, 0,
		NULL);

	tmp = XtVaCreateManagedWidget( "",
		xmFormWidgetClass,
		form,
		XmNfractionBase, 7,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->mailListLabel,
		XmNleftOffset, 10,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	XtVaSetValues(ctxt->mailText,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, tmp,
		XmNuserData, ctxt->mailList,
		NULL);

	ctxt->addButton = XtVaCreateManagedWidget( "addButton",
		xmPushButtonGadgetClass,
		tmp,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 57, "Add") ),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 1,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 3,
		NULL );
	XtAddCallback( ctxt->addButton, XmNactivateCallback,
		(XtCallbackProc) addListEntryCB,
		(XtPointer) ctxt->mailText );

	ctxt->deleteButton = XtVaCreateManagedWidget( "deleteButton",
		xmPushButtonGadgetClass,
		tmp,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 58, "Delete") ),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 4,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 6,
		NULL );
	XtAddCallback( ctxt->deleteButton, XmNactivateCallback,
		(XtCallbackProc) deleteListEntryCB,
		(XtPointer) ctxt->mailList );


	wnum = 11;
	wlist[0] = ctxt->existFilesLabel;
	wlist[1] = ctxt->existPkgLabel;
	wlist[2] = ctxt->existPartialLabel;
	wlist[3] = ctxt->installSetuidLabel;
	wlist[4] = ctxt->runSetuidLabel;
	wlist[5] = ctxt->installDependLabel;
	wlist[6] = ctxt->removeDependLabel;
	wlist[7] = ctxt->runLevelLabel;
	wlist[8] = ctxt->insufficientSpaceLabel;
	wlist[9] = ctxt->showCopyrightsLabel;
	wlist[10] = ctxt->interactiveLabel;
	for (i=0; i<wnum; i++) {
		XtVaGetValues(wlist[i],
			XmNwidth, &width,
			NULL);
		if (width > maxwidth) {
			maxwidth = width;
			maxlabel = wlist[i];
		}
	}
	for (i=0; i<wnum; i++) {
		XtVaSetValues(wlist[i],
			XmNwidth, maxwidth,
			NULL);
	}

	bbox = create_button_box(ctxt->pkgAdminDialog, NULL, NULL,
		&ctxt->okPushbutton, NULL,
		&ctxt->resetPushbutton, &ctxt->cancelPushbutton,
		&ctxt->helpPushbutton);

	XtVaSetValues(form,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, bbox,
		NULL);

	XtAddCallback(ctxt->okPushbutton, XmNactivateCallback,
		(XtCallbackProc) okCB,
		(XtPointer) ctxt);

	XtAddCallback(ctxt->helpPushbutton, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"pkg_admin.t.hlp" );

	XtAddCallback( ctxt->resetPushbutton, XmNactivateCallback,
		(XtCallbackProc) resetCB,
		(XtPointer) ctxt);

	XtAddCallback( ctxt->cancelPushbutton, XmNactivateCallback,
		(XtCallbackProc) cancelCB,
		(XtPointer) ctxt);

	XtVaSetValues(ctxt->pkgAdminDialog,
		XmNdefaultButton, ctxt->okPushbutton,
		NULL);

	XtVaSetValues(ctxt->pkgAdminDialog,
		XmNinitialFocus, ctxt->existFilesOptionMenu,
		NULL);
	
	return ( ctxt->pkgAdminDialog );
}


void	show_pkgadmindialog(Widget parent, sysMgrMainCtxt * mgrctxt)
{
	pkgAdminCtxt*	ctxt;
	Widget		type_widget;


	if (pkgadmindialog && XtIsManaged(XtParent(pkgadmindialog))) {
		XtPopup(XtParent(pkgadmindialog), XtGrabNone);
		return;
	}

	SetBusyPointer(True);

	if (pkgadmindialog == NULL) {
		pkgadmindialog = build_pkgAdminDialog(parent);
	}

	init_dialog(pkgadmindialog);
	save_dialog_values(pkgadmindialog);

	XtManageChild(pkgadmindialog);
	XtPopup(XtParent(pkgadmindialog), XtGrabNone);

	SetBusyPointer(False);
}

static void
init_dialog(Widget dialog)
{
	pkgAdminCtxt	*ctxt;
	Widget		pulldown;
	char*		users;
	char*		user_p;
	XmString	xstr;

	XtVaGetValues(dialog,
		XmNuserData, &ctxt,
		NULL);

	XtVaGetValues(ctxt->existFilesOptionMenu,
		XmNsubMenuId, &pulldown,
		NULL);
	XtVaSetValues(ctxt->existFilesOptionMenu,
		XmNmenuHistory, XtNameToWidget(pulldown, adminFile.conflict),
		NULL);

	XtVaGetValues(ctxt->existPkgOptionMenu,
		XmNsubMenuId, &pulldown,
		NULL);
	XtVaSetValues(ctxt->existPkgOptionMenu,
		XmNmenuHistory, XtNameToWidget(pulldown, adminFile.instance),
		NULL);

	XtVaGetValues(ctxt->existPartialOptionMenu,
		XmNsubMenuId, &pulldown,
		NULL);
	XtVaSetValues(ctxt->existPartialOptionMenu,
		XmNmenuHistory, XtNameToWidget(pulldown, adminFile.partial),
		NULL);

	XtVaGetValues(ctxt->installSetuidOptionMenu,
		XmNsubMenuId, &pulldown,
		NULL);
	XtVaSetValues(ctxt->installSetuidOptionMenu,
		XmNmenuHistory, XtNameToWidget(pulldown, adminFile.setuid),
		NULL);

	XtVaGetValues(ctxt->runSetuidOptionMenu,
		XmNsubMenuId, &pulldown,
		NULL);
	XtVaSetValues(ctxt->runSetuidOptionMenu,
		XmNmenuHistory, XtNameToWidget(pulldown, adminFile.action),
		NULL);

	XtVaGetValues(ctxt->installDependOptionMenu,
		XmNsubMenuId, &pulldown,
		NULL);
	XtVaSetValues(ctxt->installDependOptionMenu,
		XmNmenuHistory, XtNameToWidget(pulldown, adminFile.idepend),
		NULL);

	XtVaGetValues(ctxt->removeDependOptionMenu,
		XmNsubMenuId, &pulldown,
		NULL);
	XtVaSetValues(ctxt->removeDependOptionMenu,
		XmNmenuHistory, XtNameToWidget(pulldown, adminFile.rdepend),
		NULL);

	XtVaGetValues(ctxt->runLevelOptionMenu,
		XmNsubMenuId, &pulldown,
		NULL);
	XtVaSetValues(ctxt->runLevelOptionMenu,
		XmNmenuHistory, XtNameToWidget(pulldown, adminFile.runlevel),
		NULL);

	XtVaGetValues(ctxt->insufficientSpaceOptionMenu,
		XmNsubMenuId, &pulldown,
		NULL);
	XtVaSetValues(ctxt->insufficientSpaceOptionMenu,
		XmNmenuHistory, XtNameToWidget(pulldown, adminFile.space),
		NULL);

	XtVaGetValues(ctxt->showCopyrightsOptionMenu,
		XmNsubMenuId, &pulldown,
		NULL);
	XtVaSetValues(ctxt->showCopyrightsOptionMenu,
		XmNmenuHistory, XtNameToWidget(pulldown, adminFile.showcr),
		NULL);

	XtVaGetValues(ctxt->interactiveOptionMenu,
		XmNsubMenuId, &pulldown,
		NULL);
	XtVaSetValues(ctxt->interactiveOptionMenu,
		XmNmenuHistory, XtNameToWidget(pulldown, adminFile.interactive),
		NULL);

	XmListDeleteAllItems(ctxt->mailList);
	XmTextFieldSetString(ctxt->mailText, "");

	if (adminFile.mail != NULL) {
		users = strdup(adminFile.mail);
		user_p = strtok(users, ",");
		while (user_p != NULL) {
			xstr = XmStringCreateLocalized(user_p);
			XmListAddItemUnselected(ctxt->mailList, xstr, 0);
			XmStringFree(xstr);
			user_p = strtok(NULL, ",");
		}
		free(users);
	}

	XmProcessTraversal(ctxt->pkgAdminDialog, XmTRAVERSE_HOME);
}

static XmStringTable
copy_XmStringTable(int size, XmStringTable table)
{
	int i;
	XmStringTable tmp = (XmStringTable)XtMalloc(size * sizeof(XmString));

	for (i = 0; i < size; i++) {
		tmp[i] = XmStringCopy(table[i]);
	}

	return tmp;
}

static void
free_XmStringTable(int size, XmStringTable table)
{
	int i;

	if (table == NULL) {
		return;
	}

	for (i = 0; i < size; i++) {
		XmStringFree(table[i]);
	}

	XtFree(table);
}

static void
save_dialog_values(Widget dialog)
{
	pkgAdminCtxt	*ctxt;
	int		listcount = 0;
	XmStringTable	list = NULL;

	XtVaGetValues(dialog,
		XmNuserData, &ctxt,
		NULL);

	XtVaGetValues(ctxt->existFilesOptionMenu,
		XmNmenuHistory, &ctxt->save_exist_files,
		NULL);
	XtVaGetValues(ctxt->existPkgOptionMenu,
		XmNmenuHistory, &ctxt->save_exist_pkgs,
		NULL);
	XtVaGetValues(ctxt->existPartialOptionMenu,
		XmNmenuHistory, &ctxt->save_exist_partial,
		NULL);
	XtVaGetValues(ctxt->installSetuidOptionMenu,
		XmNmenuHistory, &ctxt->save_install_setuid,
		NULL);
	XtVaGetValues(ctxt->runSetuidOptionMenu,
		XmNmenuHistory, &ctxt->save_run_setuid,
		NULL);
	XtVaGetValues(ctxt->installDependOptionMenu,
		XmNmenuHistory, &ctxt->save_install_depend,
		NULL);
	XtVaGetValues(ctxt->removeDependOptionMenu,
		XmNmenuHistory, &ctxt->save_remove_depend,
		NULL);
	XtVaGetValues(ctxt->runLevelOptionMenu,
		XmNmenuHistory, &ctxt->save_runlevel,
		NULL);
	XtVaGetValues(ctxt->insufficientSpaceOptionMenu,
		XmNmenuHistory, &ctxt->save_insuf_space,
		NULL);
	XtVaGetValues(ctxt->showCopyrightsOptionMenu,
		XmNmenuHistory, &ctxt->save_show_cr,
		NULL);
	XtVaGetValues(ctxt->interactiveOptionMenu,
		XmNmenuHistory, &ctxt->save_interactive,
		NULL);

	if (ctxt->save_mail_list != NULL) {
		free_XmStringTable(ctxt->save_mail_list_size,
			ctxt->save_mail_list);
		ctxt->save_mail_list = NULL;
	}
	XtVaGetValues(ctxt->mailList,
		XmNitemCount, &listcount,
		XmNitems, &list,
		NULL);
	ctxt->save_mail_list_size = listcount;
	ctxt->save_mail_list = copy_XmStringTable(listcount, list);
}

/*
 * Write the admin file used by pkgadd and pkgrm.
 */
char*
write_admin_file(PkgAdminProps* admin)
{
	char	* tfile = NULL;
	FILE	* fp = NULL;

	/* Create a tmp file name for admin file */
	tfile = tmpnam(NULL);
	if (tfile && (fp = fopen(tfile, "w"))) {
		fprintf(fp, "mail=%s\n", admin->mail ? admin->mail : "");
		fprintf(fp, "instance=%s\n", admin->instance);
		fprintf(fp, "partial=%s\n", admin->partial);
		fprintf(fp, "runlevel=%s\n", admin->runlevel);
		fprintf(fp, "idepend=%s\n", admin->idepend);
		fprintf(fp, "rdepend=%s\n", admin->rdepend);
		fprintf(fp, "space=%s\n", admin->space);
		fprintf(fp, "setuid=%s\n", admin->setuid);
		fprintf(fp, "conflict=%s\n", admin->conflict);
		fprintf(fp, "action=%s\n", admin->action);
		fprintf(fp, "basedir=%s\n", admin->basedir);

		fclose(fp);
		return (tfile);
	}

	return NULL;
}

