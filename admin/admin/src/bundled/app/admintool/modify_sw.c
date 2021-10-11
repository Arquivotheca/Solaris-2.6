/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)modify_sw.c	1.12 96/08/06 Sun Microsystems"

/*	modify_sw.c */

#include <stdlib.h>
#include <libintl.h>
#include <nl_types.h>
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Form.h>
#include <Xm/DialogS.h>
#include <Xm/LabelG.h>
#include <Xm/SeparatoG.h>
#include <Xm/PushBG.h>
#include <Xm/MessageB.h>

#include "sysman_iface.h"
#include "util.h"
#include "software.h"
#include "UxXt.h"
/* #include "valid.h" */

#define SIZE_FMT "%14.14s   %5.5s"

typedef	struct
{
	Widget	modifySwDialog;
	Widget	nameForm;
	Widget	nameLabel;
	Widget	nameText;
	Widget	abbrevForm;
	Widget	abbrevLabel;
	Widget	abbrevText;
	Widget	productForm;
	Widget	productLabel;
	Widget	productText;
	Widget  instForm;
	Widget  instLabel;
	Widget	instText;
	Widget	vendorForm;
	Widget	vendorLabel;
	Widget	vendorText;
	Widget	archForm;
	Widget	archLabel;
	Widget	archText;
	Widget	dateForm;
	Widget	dateLabel;
	Widget	dateText;
	Widget	baseDirForm;
	Widget	baseDirLabel;
	Widget	baseDirText;
	Widget	descForm;
	Widget	descLabel;
	Widget	descText;
	Widget	sizeForm;
	Widget	sizeLabel;
	Widget	fsSizeLabel[MAX_SPACE_FS];
#if 0
	Widget	rootSizeLabel;
	Widget	usrSizeLabel;
	Widget	optSizeLabel;
	Widget	varSizeLabel;
	Widget	exportSizeLabel;
	Widget	openwinSizeLabel;
#endif
	Widget	buttonBox;
	Widget	separator;
	Widget	okPushbutton;
	Widget	applyPushbutton;
	Widget	resetPushbutton;
	Widget	cancelPushbutton;
	Widget	helpPushbutton;

} modifySwCtxt;

static void init_dialog(Widget dialog, SWStruct* sw);

static char	errstr[1024] = "";

Widget 	modifyswdialog = NULL;

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

/*******************************************************************************
       The following are callback functions.
*******************************************************************************/


static void	CancelPushbutton_activateCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cbs)
{
	modifySwCtxt* ctxt = (modifySwCtxt*)cd;

	XtPopdown(XtParent(ctxt->modifySwDialog));
}


Widget	build_modifySwDialog(Widget parent)
{
	modifySwCtxt*	ctxt;
	Widget		shell;
	Widget		menushell;
	Widget		pulldown;
	Widget		bigrc;
	Widget		size_rc;
	char **		s;
	XmString	xstr;
	Widget		w;
	int		i;
	int		wnum;
	Widget		wlist[10];
	Widget		maxlabel;
	Dimension	width;
	Dimension	maxwidth = 0;
	XmFontList	fontlist;
	Arg		args[10];


	ctxt = (modifySwCtxt*) malloc(sizeof(modifySwCtxt));


	if (parent == NULL)
	{
		parent = GtopLevel;
	}

	shell = XtVaCreatePopupShell( "ModSwDialog_shell",
		xmDialogShellWidgetClass, parent,
		XmNshellUnitType, XmPIXELS,
		XmNallowShellResize, True,
		XmNminWidth, 350,
		XmNminHeight, 300,
		NULL );

	ctxt->modifySwDialog = XtVaCreateWidget( "ModSwDialog",
		xmFormWidgetClass,
		shell,
		XmNunitType, XmPIXELS,
		RES_CONVERT(XmNdialogTitle, catgets(_catd, 8, 362, "Admintool: Software Details")),
		NULL );

	XtVaSetValues(ctxt->modifySwDialog,
		XmNuserData, (XtPointer)ctxt,
		NULL);

	bigrc = XtVaCreateManagedWidget( "bigrc",
		xmRowColumnWidgetClass,
		ctxt->modifySwDialog,
		XmNorientation, XmVERTICAL,
		XmNnumColumns, 1,
		XmNspacing, 2,
		XmNmarginHeight, 1,
		XmNmarginWidth, 1,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 10,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 10,
		NULL );

	ctxt->nameForm = XtVaCreateManagedWidget( "nameForm",
		xmFormWidgetClass,
		bigrc,
		NULL );

	ctxt->nameLabel = XtVaCreateManagedWidget( "nameLabel",
		xmLabelGadgetClass,
		ctxt->nameForm,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 363, "Name:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->nameText = XtVaCreateManagedWidget( "nameText",
		xmLabelGadgetClass,
		ctxt->nameForm,
		XmNalignment, XmALIGNMENT_BEGINNING,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->nameLabel,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->abbrevForm = XtVaCreateManagedWidget( "abbrevForm",
		xmFormWidgetClass,
		bigrc,
		NULL );

	ctxt->abbrevLabel = XtVaCreateManagedWidget( "abbrevLabel",
		xmLabelGadgetClass,
		ctxt->abbrevForm,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 364, "Abbreviation:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->abbrevText = XtVaCreateManagedWidget( "abbrevText",
		xmLabelGadgetClass,
		ctxt->abbrevForm,
		XmNalignment, XmALIGNMENT_BEGINNING,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->abbrevLabel,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->productForm = XtVaCreateManagedWidget( "productForm",
		xmFormWidgetClass,
		bigrc,
		NULL );

	ctxt->productLabel = XtVaCreateManagedWidget( "productLabel",
		xmLabelGadgetClass,
		ctxt->productForm,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 365, "Product:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->productText = XtVaCreateManagedWidget( "productText",
		xmLabelGadgetClass,
		ctxt->productForm,
		XmNalignment, XmALIGNMENT_BEGINNING,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->productLabel,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->instForm = XtVaCreateManagedWidget( "instForm",
		xmFormWidgetClass,
		bigrc,
		NULL );

	ctxt->instLabel = XtVaCreateManagedWidget( "instLabel",
		xmLabelGadgetClass,
		ctxt->instForm,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 782, "Package Instance:") ),
		XmNleftAttachment, XmATTACH_FORM,
/* FIX ME this offset should not be necessary */
  		XmNleftOffset, 20,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->instText = XtVaCreateManagedWidget( "instText",
		xmLabelGadgetClass,
		ctxt->instForm,
		XmNalignment, XmALIGNMENT_BEGINNING,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->instLabel,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->vendorForm = XtVaCreateManagedWidget( "vendorForm",
		xmFormWidgetClass,
		bigrc,
		NULL );

	ctxt->vendorLabel = XtVaCreateManagedWidget( "vendorLabel",
		xmLabelGadgetClass,
		ctxt->vendorForm,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 366, "Vendor:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->vendorText = XtVaCreateManagedWidget( "vendorText",
		xmLabelGadgetClass,
		ctxt->vendorForm,
		XmNalignment, XmALIGNMENT_BEGINNING,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->vendorLabel,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->descForm = XtVaCreateManagedWidget( "descForm",
		xmFormWidgetClass,
		bigrc,
		NULL );

	ctxt->descLabel = XtVaCreateManagedWidget( "descLabel",
		xmLabelGadgetClass,
		ctxt->descForm,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 367, "Description:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		NULL );

	XtVaGetValues(ctxt->descLabel,
		XmNfontList, &fontlist,
		NULL);

	XtSetArg(args[0], XmNrows, 2);
	XtSetArg(args[1], XmNcolumns, 30);
	XtSetArg(args[2], XmNeditable, False);
	XtSetArg(args[3], XmNeditMode, XmMULTI_LINE_EDIT);
	XtSetArg(args[4], XmNwordWrap, True);
	XtSetArg(args[5], XmNscrollHorizontal, False);
	XtSetArg(args[6], XmNblinkRate, 0);
	XtSetArg(args[7], XmNautoShowCursorPosition, True);
	XtSetArg(args[8], XmNcursorPositionVisible, False);
	XtSetArg(args[9], XmNfontList, fontlist);
	ctxt->descText = XmCreateScrolledText(ctxt->descForm, catgets(_catd, 8, 368, "desc"), args, 10);

	XtVaSetValues(XtParent(ctxt->descText),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->descLabel,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtManageChild(ctxt->descText);

	ctxt->archForm = XtVaCreateManagedWidget( "archForm",
		xmFormWidgetClass,
		bigrc,
		NULL );

	ctxt->archLabel = XtVaCreateManagedWidget( "archLabel",
		xmLabelGadgetClass,
		ctxt->archForm,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 369, "Architecture:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->archText = XtVaCreateManagedWidget( "archText",
		xmLabelGadgetClass,
		ctxt->archForm,
		XmNalignment, XmALIGNMENT_BEGINNING,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->archLabel,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->dateForm = XtVaCreateManagedWidget( "dateForm",
		xmFormWidgetClass,
		bigrc,
		NULL );

	ctxt->dateLabel = XtVaCreateManagedWidget( "dateLabel",
		xmLabelGadgetClass,
		ctxt->dateForm,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 370, "Date Installed:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->dateText = XtVaCreateManagedWidget( "dateText",
		xmLabelGadgetClass,
		ctxt->dateForm,
		XmNalignment, XmALIGNMENT_BEGINNING,
		RES_CONVERT( XmNlabelString, "" ),
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->dateLabel,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );

	ctxt->baseDirForm = XtVaCreateManagedWidget( "baseDirForm",
		xmFormWidgetClass,
		bigrc,
		NULL );

	ctxt->baseDirLabel = XtVaCreateManagedWidget("baseDirLabel",
		xmLabelGadgetClass,
		ctxt->baseDirForm,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 371, "Base Directory:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
                NULL );

	ctxt->baseDirText = XtVaCreateManagedWidget("baseDirText",
		xmLabelGadgetClass,
                ctxt->baseDirForm,
		XmNalignment, XmALIGNMENT_BEGINNING,
                RES_CONVERT( XmNlabelString, "" ),
                XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->baseDirLabel,
		XmNleftOffset, 10,
                XmNrightAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );
		
	ctxt->sizeLabel = XtVaCreateManagedWidget( "sizeLabel",
		xmLabelGadgetClass,
		ctxt->modifySwDialog,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 372, "Estimated Size (MB):") ),
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, bigrc,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 10,
		NULL );

	size_rc = XtVaCreateManagedWidget( "size_rc",
		xmRowColumnWidgetClass,
		ctxt->modifySwDialog,
		XmNorientation, XmVERTICAL,
		XmNpacking, XmPACK_COLUMN,
		XmNnumColumns, 2,
		XmNentryAlignment, XmALIGNMENT_END,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, bigrc,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->sizeLabel,
		XmNleftOffset, 10,
		NULL );

	i = 0;
	while (installFileSystems[i]) {
		ctxt->fsSizeLabel[i] = XtVaCreateManagedWidget( "fsSizeLabel",
			xmLabelGadgetClass,
			size_rc,
			RES_CONVERT( XmNlabelString, "/xxxxxxxxxx" ),
			NULL );
		i++;
   	}
#if 0
	ctxt->usrSizeLabel = XtVaCreateManagedWidget( "usrSizeLabel",
		xmLabelGadgetClass,
		size_rc,
		RES_CONVERT( XmNlabelString, "/usr" ),
		NULL );
	ctxt->optSizeLabel = XtVaCreateManagedWidget( "optSizeLabel",
		xmLabelGadgetClass,
		size_rc,
		RES_CONVERT( XmNlabelString, "/opt" ),
		NULL );
	ctxt->varSizeLabel = XtVaCreateManagedWidget( "varSizeLabel",
		xmLabelGadgetClass,
		size_rc,
		RES_CONVERT( XmNlabelString, "/var" ),
		NULL );
	ctxt->exportSizeLabel = XtVaCreateManagedWidget( "exportSizeLabel",
		xmLabelGadgetClass,
		size_rc,
		RES_CONVERT( XmNlabelString, "/export" ),
		NULL );
	ctxt->openwinSizeLabel = XtVaCreateManagedWidget( "openwinSizeLabel",
		xmLabelGadgetClass,
		size_rc,
		RES_CONVERT( XmNlabelString, "/openwin" ),
		NULL );
#endif

	wnum = 9;
	wlist[0] = ctxt->nameLabel;
	wlist[1] = ctxt->abbrevLabel;
	wlist[2] = ctxt->productLabel;
	wlist[3] = ctxt->vendorLabel;
	wlist[4] = ctxt->descLabel;
	wlist[5] = ctxt->archLabel;
	wlist[6] = ctxt->dateLabel;
	wlist[7] = ctxt->sizeLabel;
	wlist[8] = ctxt->baseDirLabel;
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

	ctxt->buttonBox = XtVaCreateManagedWidget( "",
		xmFormWidgetClass,
		ctxt->modifySwDialog,
		XmNresizable, False,
		XmNfractionBase, 31,
		XmNheight, 60,
#if 0
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, size_rc,
		XmNtopOffset, 10,
#endif
		XmNbottomAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 2,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 2,
		NULL );

	XtVaSetValues(size_rc, XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, ctxt->buttonBox,
			XmNbottomOffset, 10,
			NULL);

	ctxt->separator = XtVaCreateManagedWidget( "Separator",
		xmSeparatorGadgetClass,
		ctxt->buttonBox,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL );

	ctxt->cancelPushbutton = XtVaCreateManagedWidget( "CancelPushbutton",
		xmPushButtonGadgetClass,
		ctxt->buttonBox,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 13,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 18,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, FORM_OFFSET,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 373, "Cancel") ),
		NULL );



	XtAddCallback( ctxt->cancelPushbutton, XmNactivateCallback,
		(XtCallbackProc) CancelPushbutton_activateCB,
		(XtPointer) ctxt);

	XtVaSetValues(ctxt->modifySwDialog,
		XmNdefaultButton, ctxt->cancelPushbutton,
		NULL);

			
	return ( ctxt->modifySwDialog );
}


void	show_modifyswdialog(Widget parent, SWStruct* sw,
	sysMgrMainCtxt * mgrctxt
)
{
	modifySwCtxt*	ctxt;
	Widget		type_widget;


	if (modifyswdialog && XtIsManaged(XtParent(modifyswdialog))) {
		XtPopup(XtParent(modifyswdialog), XtGrabNone);
		return;
	}

	SetBusyPointer(True);

	if (modifyswdialog == NULL) {
		modifyswdialog = build_modifySwDialog(parent);
	}

	mgrctxt->currDialog = modifyswdialog;
	XtVaGetValues(modifyswdialog,
		XmNuserData, &ctxt,
		NULL);

	init_dialog(modifyswdialog, sw);

	XtManageChild(modifyswdialog);
	XtPopup(XtParent(modifyswdialog), XtGrabNone);
	SetBusyPointer(False);
}

static void
init_dialog(Widget dialog, SWStruct* sw)
{
	modifySwCtxt	*ctxt;
	XmString	xstr;
	char		size[16];
	char		buf[256];
	int		i, j;

	if (sw == NULL)
		return;

	XtVaGetValues(dialog,
		XmNuserData, &ctxt,
		NULL);

	xstr = XmStringCreateLocalized(sw->sw_name ? (char*)sw->sw_name : "");
	XtVaSetValues(ctxt->nameText,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	sprintf(buf, "%s %s", (sw->sw_id ? sw->sw_id : ""),
			      (sw->version ? sw->version : ""));
	xstr = XmStringCreateLocalized(buf);
	XtVaSetValues(ctxt->abbrevText,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	sprintf(buf, "%s %s", (sw->prodname ? sw->prodname : ""),
			      (sw->prodvers ? sw->prodvers : ""));
	xstr = XmStringCreateLocalized(buf);
	XtVaSetValues(ctxt->productText,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	xstr = XmStringCreateLocalized((char*)(sw->instance ? sw->instance : 
						(sw->sw_id ? sw->sw_id : "")));
	XtVaSetValues(ctxt->instText,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	xstr = XmStringCreateLocalized(sw->vendor ? (char*)sw->vendor : "");
	XtVaSetValues(ctxt->vendorText,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	xstr = XmStringCreateLocalized(sw->arch ? (char*)sw->arch : "");
	XtVaSetValues(ctxt->archText,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	xstr = XmStringCreateLocalized(sw->date ? (char*)sw->date : "");
	XtVaSetValues(ctxt->dateText,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	xstr = XmStringCreateLocalized(sw->basedir ? (char*)sw->basedir : "");
	XtVaSetValues(ctxt->baseDirText,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	XmTextSetString(ctxt->descText, sw->desc ? (char*)sw->desc : "");

	if (sw->install_reqs) {
	    i = 0;
	    while (installFileSystems[i]) {
                int mb_size = sw->install_reqs[i].size / 1024;
                if ((sw->install_reqs[i].size != 0) && mb_size == 0)
		    sprintf(size, "%s", "<1");
                else
		    sprintf(size, "%d", mb_size);
		sprintf(buf, SIZE_FMT, sw->install_reqs[i].mountp, size);
		xstr = XmStringCreateLocalized(buf);
		XtVaSetValues(ctxt->fsSizeLabel[i],
			XmNlabelString, xstr,
			NULL);
		XtManageChild(ctxt->fsSizeLabel[i]);
		XmStringFree(xstr);
		i++;
	    }
	}

#if 0
	sprintf(size, "%d", 0);
	sprintf(buf, SIZE_FMT, "/", size);
	xstr = XmStringCreateLocalized(buf);
	XtVaSetValues(ctxt->rootSizeLabel,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	sprintf(size, "%d", 0);
	sprintf(buf, SIZE_FMT, "/usr", size);
	xstr = XmStringCreateLocalized(buf);
	XtVaSetValues(ctxt->usrSizeLabel,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	sprintf(size, "%d", 0);
	sprintf(buf, SIZE_FMT, "/opt", size);
	xstr = XmStringCreateLocalized(buf);
	XtVaSetValues(ctxt->optSizeLabel,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	sprintf(size, "%d", 0);
	sprintf(buf, SIZE_FMT, "/var", size);
	xstr = XmStringCreateLocalized(buf);
	XtVaSetValues(ctxt->varSizeLabel,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	sprintf(size, "%d", 0);
	sprintf(buf, SIZE_FMT, "/export", size);
	xstr = XmStringCreateLocalized(buf);
	XtVaSetValues(ctxt->exportSizeLabel,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	sprintf(size, "%d", 0);
	sprintf(buf, SIZE_FMT, "/usr/openwin", size);
	xstr = XmStringCreateLocalized(buf);
	XtVaSetValues(ctxt->openwinSizeLabel,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);
#endif

}


