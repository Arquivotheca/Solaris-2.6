#ifndef lint
#pragma ident "@(#)pfgdsr_media.c 1.10 96/09/03 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgdsr_media.c
 * Group:	installtool
 * Description:
 */

#include "pfg.h"
#include "pfgDSRMedia_ui.h"

void
dsr_media_toggle_arm_cb(
	Widget w, XtPointer client_data, XtPointer call_data);

static void dsr_media_device_save(void);
static Widget _convert_media_type_to_widget(TDSRALMedia media_type);

static WidgetList widget_list;
static Widget dsr_media_dialog;

/*
 * *************************************************************************
 *	Code for the main DSR Media screen
 * *************************************************************************
 */

Widget
pfgCreateDsrMedia(void)
{
	char *str;
	TDSRALMedia curr_media_type;
	DsrSLListExtraData *LLextra;

	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);

	/* get the dialog widget & the dialog widget list from teleuse */
	dsr_media_dialog = tu_dsr_media_dialog_widget(
		"dsr_media_dialog", pfgTopLevel, &widget_list);

	/* set up exit callback off window manager close */
	XmAddWMProtocolCallback(pfgShell(dsr_media_dialog), pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	/* set title */
	XtVaSetValues(pfgShell(dsr_media_dialog),
		XtNtitle, TITLE_DSR_MEDIA,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);

	str = (char *) xmalloc(strlen(MSG_DSR_MEDIA) +
		UI_FS_SIZE_DISPLAY_LENGTH + 1);
	(void) sprintf(str, MSG_DSR_MEDIA,
		UI_FS_SIZE_DISPLAY_LENGTH,
		(int) bytes_to_mb(LLextra->archive_size));
	pfgSetWidgetString(widget_list, "panelhelpText", str);
	free(str);

	/* button labels */
	pfgSetStandardButtonStrings(widget_list,
		ButtonContinue, ButtonGoback, ButtonExit, ButtonHelp,
		NULL);

	/* media option setup */
	pfgSetWidgetString(widget_list, "mediaLabel",
		LABEL_DSR_MEDIA_MEDIA);


	/* save whatever the current media type is */
	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaType, &curr_media_type,
		NULL);

	/*
	 * temporarily set the media type and then get the correct
	 * toggle label
	 */
	LLextra->history.media_type = DSRALFloppy;
	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaTypeStr, &str,
		NULL);
	pfgSetWidgetString(widget_list, "floppyToggle", str);
	free(str);

	LLextra->history.media_type = DSRALTape;
	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaTypeStr, &str,
		NULL);
	pfgSetWidgetString(widget_list, "tapeToggle", str);
	free(str);

	LLextra->history.media_type = DSRALDisk;
	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaTypeStr, &str,
		NULL);
	pfgSetWidgetString(widget_list, "diskToggle", str);
	free(str);

	LLextra->history.media_type = DSRALNFS;
	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaTypeStr, &str,
		NULL);
	pfgSetWidgetString(widget_list, "nfsToggle", str);
	free(str);

	LLextra->history.media_type = DSRALRsh;
	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaTypeStr, &str,
		NULL);
	pfgSetWidgetString(widget_list, "rshToggle", str);
	free(str);

	/* replace the current media type */
	LLextra->history.media_type = curr_media_type;

	/* give each option user data of its actual typedef */
	XtVaSetValues(pfgGetNamedWidget(widget_list, "floppyToggle"),
		XmNuserData, DSRALFloppy,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(widget_list, "tapeToggle"),
		XmNuserData, DSRALTape,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(widget_list, "diskToggle"),
		XmNuserData, DSRALDisk,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(widget_list, "nfsToggle"),
		XmNuserData, DSRALNFS,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(widget_list, "rshToggle"),
		XmNuserData, DSRALRsh,
		NULL);

	/* set initial media defaults */
	dsr_media_toggle_arm_cb((Widget) NULL,
		(XtPointer) NULL, (XtPointer) NULL);

	/* manage it... */
	xm_SetNoResize(pfgTopLevel, dsr_media_dialog);
	XtManageChild(dsr_media_dialog);

	/*
	 * If there's a text field value, set process traversal
	 * to the Continue Button, o/w set process traversal to
	 * the text field.
	 */
	if (LLextra->history.media_device) {
		(void) XmProcessTraversal(
			pfgGetNamedWidget(widget_list, "continueButton"),
			XmTRAVERSE_CURRENT);
	} else {
		(void) XmProcessTraversal(
			pfgGetNamedWidget(widget_list, "deviceTextField"),
			XmTRAVERSE_CURRENT);
	}

	return (dsr_media_dialog);
}

/* ARGSUSED */
void
dsr_media_toggle_arm_cb(
	Widget w, XtPointer client_data, XtPointer call_data)
{
	char *device_label;
	char *info_text = "";
	char *add_text = NULL;
	TDSRALMedia media_type;
	char *media_device;
	DsrSLListExtraData *LLextra;
	XmToggleButtonCallbackStruct *cb_data =
		/* LINTED [pointer cast] */
		(XmToggleButtonCallbackStruct *)call_data;

	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);

	/*
	 * If widget is null, then we are in the process of (re)creating
	 * the screen. We want to set initial defaults, or maintain
	 * history in the settings if they've been here before.
	 */
	if (!w) {
		media_type = LLextra->history.media_type;
		media_device = LLextra->history.media_device;

		/* default the media type */
		w = _convert_media_type_to_widget(media_type);
		XtVaSetValues(w,
/* 			pfgGetNamedWidget(widget_list, "mediaOptionMenu"), */
/* 			XmNmenuHistory, w, */
			XmNset, w,
			NULL);

		/* default the media device */
#if 0
	/*
	 * If the media type is floppy or tape, then default the
	 * media device string the first time in.
	 */
	if (!LLextra->history.media_device) {
		if (LLextra->history.media_type == DSRALFLOPPY) {
			LLextra->history.media_type =
				xstrdup(TEXT_DSR_MEDIA_ORIG_FLOPPY);
		} else if (LLextra->history.media_type == DSRALTape) {
			LLextra->history.media_type =
				xstrdup(TEXT_DSR_MEDIA_ORIG_TAPE);
		}
	}
#endif

		if (media_device) {
			pfgSetWidgetString(widget_list, "deviceTextField",
				media_device);
			XtVaSetValues(pfgGetNamedWidget(widget_list,
				"deviceTextField"),
				XmNcursorPosition,
					media_device ? strlen(media_device) : 0,
				NULL);
		}

	} else {
		if (cb_data->reason != XmCR_ARM)
				return;

		/* find out what type of media they selected */
		XtVaGetValues(w,
			XmNuserData, &media_type,
			NULL);
	}

	/* set the on-screen text accordingly */
	write_debug(GUI_DEBUG_L1,
		"Media choice\n\twidget %s w/user data = %d",
		XtName(w), media_type);

	/* so that this media type is in the link data */
	LLextra->history.media_type = media_type;

	DsrSLListGetAttr(DsrSLHandle,
		DsrSLAttrMediaTypeDeviceStr, &device_label,
		NULL);

	switch (media_type) {
	case  DSRALFloppy:
		add_text = LABEL_DSR_MEDIA_MFLOPPY;
		break;
	case DSRALTape:
		add_text = LABEL_DSR_MEDIA_MTAPES;
		break;
	}
	pfgSetWidgetString(widget_list, "deviceLabel", device_label);

	if (add_text) {
		info_text = (char *) xmalloc(strlen(MSG_DSR_MEDIA) +
			(2 * strlen(add_text)) + 1);
		(void) sprintf(info_text, MSG_DSR_MEDIA_MULTIPLE,
			add_text, add_text);
	}
	pfgSetWidgetString(widget_list, "infoText", info_text);
}

/* note that a \n in media device brings us here also */
/* ARGSUSED */
void
dsr_media_continue_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	DsrSLListExtraData *LLextra;
	TDSRALError err;
	char *str;
	char buf[PATH_MAX];
	char buf1[PATH_MAX];

	pfgBusy(dsr_media_dialog);

	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);

	dsr_media_device_save();

	/* did they enter a media device at all? */
	if (!LLextra->history.media_device ||
		!strlen(LLextra->history.media_device)) {
		pfAppWarn(0, APP_ER_DSR_MEDIA_NODEVICE);
		pfgUnbusy(dsr_media_dialog);
		return;
	}

	/*
	 * Validate the media type/device entries.
	 *
	 * If it's a tape or floppy tell them to enter the media now
	 * so we can test writing to it to make sure everything's really
	 * ok with the media first.
	 */
	if (LLextra->history.media_type == DSRALFloppy) {
		(void) sprintf(buf1, MSG_DSR_MEDIA_INSERT_FLOPPY_NOTE,
			LABEL_DSR_MEDIA_FLOPPY);
		(void) sprintf(buf, MSG_DSR_MEDIA_INSERT_FIRST,
			LABEL_DSR_MEDIA_FLOPPY,
			buf1);

		pfAppWarn(0, buf);
	} else if (LLextra->history.media_type == DSRALTape) {
		(void) sprintf(buf1, MSG_DSR_MEDIA_INSERT_TAPE_NOTE,
			LABEL_DSR_MEDIA_TAPE);
		(void) sprintf(buf, MSG_DSR_MEDIA_INSERT_FIRST,
			LABEL_DSR_MEDIA_TAPE,
			buf1);

		pfAppWarn(0, buf);
	}

	err = DSRALSetMedia(DsrALHandle, DsrSLHandle,
		LLextra->history.media_type,
		LLextra->history.media_device);
	if (err != DSRALSuccess) {
		if (err == DSRALUnableToWriteMedia &&
			IsIsa("sparc") &&
			LLextra->history.media_type == DSRALFloppy) {
			(void) system("eject floppy");
		}
		str = DsrALMediaErrorStr(
			LLextra->history.media_type,
			LLextra->history.media_device,
			err);
		pfAppWarn(0, str);
		if (DsrALMediaErrorIsFatal(err))
			pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
		free(str);
		pfgUnbusy(dsr_media_dialog);
		return;
	}

	/*
	 * make sure the media they chose is big enough
	 */
	err = DSRALCheckMediaSpace(DsrALHandle);
	if (err != DSRALSuccess) {
		str = DsrALMediaErrorStr(
			LLextra->history.media_type,
			LLextra->history.media_device,
			err);
		pfAppWarn(0, str);
		if (DsrALMediaErrorIsFatal(err))
			pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
		free(str);
		pfgUnbusy(dsr_media_dialog);
		return;
	}

	/*
	 * continue on to where the media progress display will
	 * do the DSRALGenerate() call...
	 */
	pfgSetAction(parAContinue);

	/* free the teleuse widget list */
	free(widget_list);
	pfgUnbusy(dsr_media_dialog);
}

/* ARGSUSED */
void
dsr_media_goback_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{

	dsr_media_device_save();

	pfgSetAction(parAGoback);

	/* free the teleuse widget list */
	free(widget_list);
}

static void
dsr_media_device_save(void)
{
	DsrSLListExtraData *LLextra;
	char *media_device;

	(void) LLGetSuppliedListData(DsrSLHandle, NULL,
		(TLLData *)&LLextra);

	/* save the text field entry */
	XtVaGetValues(pfgGetNamedWidget(widget_list, "deviceTextField"),
		XmNvalue, &media_device,
		NULL);
	if (LLextra->history.media_device)
		free(LLextra->history.media_device);
	if (!media_device || !strlen(media_device))
		LLextra->history.media_device = NULL;
	else
		LLextra->history.media_device = xstrdup(media_device);
}

static Widget
_convert_media_type_to_widget(TDSRALMedia media_type)
{
	Widget w;

	switch (media_type) {
	case  DSRALFloppy:
		w = pfgGetNamedWidget(widget_list, "floppyToggle");
		break;
	case DSRALTape:
		w = pfgGetNamedWidget(widget_list, "tapeToggle");
		break;
	case DSRALDisk:
		w = pfgGetNamedWidget(widget_list, "diskToggle");
		break;
	case DSRALNFS:
		w = pfgGetNamedWidget(widget_list, "nfsToggle");
		break;
	case DSRALRsh:
		w = pfgGetNamedWidget(widget_list, "rshToggle");
		break;
	}
	return (w);
}
