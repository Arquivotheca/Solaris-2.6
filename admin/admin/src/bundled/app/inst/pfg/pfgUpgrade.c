#ifndef lint
#pragma ident "@(#)pfgupgrade.c 1.57 96/08/07 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgupgrade.c
 * Group:	installtool
 * Description:
 */
#include "pfg.h"
#include "pfgprogressbar.h"
#include "pfgUpgrade_ui.h"

/* for upgrade progress display */

/* private functions */
void restartCB(Widget, XtPointer, XtPointer);
void initialCB(Widget, XtPointer, XtPointer);

Widget
pfgCreateUpgrade(void)
{
	Widget upgrade_dialog;
/* 	Widget upgrade_button; */
	WidgetList widget_list;

	if ((SliceIsSystemUpgradeable(UpgradeSlices)) == FALSE)
		return (NULL);

	upgrade_dialog = tu_upgrade_dialog_widget("upgrade_dialog",
		pfgTopLevel, &widget_list);

	XmAddWMProtocolCallback(pfgShell(upgrade_dialog), pfgWMDeleteAtom,
	    (XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(upgrade_dialog),
	    XmNtitle, TITLE_UPGRADE,
	    XmNdeleteResponse, XmDO_NOTHING,
	    NULL);
	xm_SetNoResize(pfgTopLevel, upgrade_dialog);

	pfgSetWidgetString(widget_list, "panelhelpText", MSG_UPGRADE);

	/* set the buttons */
	pfgSetStandardButtonStrings(widget_list, ButtonExit, ButtonHelp, NULL);
	if (history_prev()) {
		XtManageChild(pfgGetNamedWidget(widget_list, "gobackButton"));
		pfgSetStandardButtonStrings(widget_list, ButtonGoback, NULL);
	} else {
		XtUnmanageChild(pfgGetNamedWidget(widget_list, "gobackButton"));
	}
	pfgSetWidgetString(widget_list, "upgradeButton", LABEL_UPGRADE_BUTTON);
	pfgSetWidgetString(widget_list, "initialButton", LABEL_INITIAL_BUTTON);

	/*
	 * set the Upgrade/OK button to insensitive to disallow Upgrade
	 * use the -u runtime option to turn this button back on for now
	 */

	/* **	take this out for now, we want the upgrade button enabled ** */
	/*
	if (upgradeEnabled != 1) {
		upgrade_button =
			pfgGetNamedWidget(widget_list, "upgradeButton");
		XtSetSensitive(upgrade_button, False);
	}
	*/

	free(widget_list);

	XtManageChild(upgrade_dialog);

	return (upgrade_dialog);
}


/* ARGSUSED */
void
upgradeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* set upgrade state */
	pfgState |= AppState_UPGRADE;
	/* set the busy cursor ON */
	pfgBusy(pfgShell(w));
	pfgSetAction(parAUpgrade);
}

/* ARGSUSED */
void
initialCB(Widget initial, XtPointer client_data, XtPointer call_data)
{
	pfgSetAction(parAInitial);
}

/* ARGSUSED */
void
upgrade_goback_cb(
	Widget w, XtPointer client_data, XtPointer call_data)
{
	pfgSetAction(parAGoback);
}

void
pfgCreateUpgradeProgress(void)
{
	int ret;
	UIProgressBarInitData init_data;
	pfgProgressBarDisplayData *disp_data;
	DsrSLListExtraData *LLextra;
	TSUData su_data;

	/*
	 * Initialize the progress bar display
	 */
	init_data.title = TITLE_UPG_PROGRESS;
	init_data.main_msg = MSG_UPG_PROGRESS;
	init_data.main_label = LABEL_UPG_PROGRESS;
	init_data.detail_label = NULL;
	init_data.percent = 0;

	if (pfgState & AppState_UPGRADE_DSR) {
		(void) pfgProgressBarCreate(
			&init_data, &disp_data, PROGBAR_PROGRESS_CNT);

		AppUpgradeGetProgressBarInfo(PROGBAR_ALBACKUP_INDEX,
			pfgState,
			&disp_data->scale_info[PROGBAR_ALBACKUP_INDEX].start,
			&disp_data->scale_info[PROGBAR_ALBACKUP_INDEX].factor);

		AppUpgradeGetProgressBarInfo(PROGBAR_ALRESTORE_INDEX,
			pfgState,
			&disp_data->scale_info[PROGBAR_ALRESTORE_INDEX].start,
			&disp_data->scale_info[PROGBAR_ALRESTORE_INDEX].factor);

		AppUpgradeGetProgressBarInfo(PROGBAR_UPGRADE_INDEX,
			pfgState,
			&disp_data->scale_info[PROGBAR_UPGRADE_INDEX].start,
			&disp_data->scale_info[PROGBAR_UPGRADE_INDEX].factor);
	} else if (pfgState & AppState_UPGRADE_RECOVER) {
		/* only the upgrade info might get used here... */
		(void) pfgProgressBarCreate(
			&init_data, &disp_data, 2);

		AppUpgradeGetProgressBarInfo(PROGBAR_ALRESTORE_INDEX,
			pfgState,
			&disp_data->scale_info[PROGBAR_ALRESTORE_INDEX].start,
			&disp_data->scale_info[PROGBAR_ALRESTORE_INDEX].factor);

		AppUpgradeGetProgressBarInfo(PROGBAR_UPGRADE_INDEX,
			pfgState,
			&disp_data->scale_info[PROGBAR_UPGRADE_INDEX].start,
			&disp_data->scale_info[PROGBAR_UPGRADE_INDEX].factor);
	} else {
		(void) pfgProgressBarCreate(
			&init_data, &disp_data, 1);

		AppUpgradeGetProgressBarInfo(PROGBAR_UPGRADE_INDEX,
			pfgState,
			&disp_data->scale_info[PROGBAR_UPGRADE_INDEX].start,
			&disp_data->scale_info[PROGBAR_UPGRADE_INDEX].factor);
	}
	pfgProgressBarUpdate(disp_data, FALSE);

	/*
	 * register backend data
	 */
	if (pfgState & AppState_UPGRADE_DSR) {
		(void) LLGetSuppliedListData(DsrSLHandle, NULL,
			(TLLData *)&LLextra);

		su_data.Operation = SI_ADAPTIVE;
		su_data.Info.AdaptiveUpgrade.ArchiveCallback =
			dsr_al_progress_cb;
		su_data.Info.AdaptiveUpgrade.ArchiveData =
			(void *) disp_data;
		su_data.Info.AdaptiveUpgrade.ScriptCallback =
			pfg_upgrade_progress_cb;
		su_data.Info.AdaptiveUpgrade.ScriptData =
			(void *) disp_data;

		/* DSR upgrade path has already generated the upgrade script */

	} else if (pfgState & AppState_UPGRADE_RECOVER) {
		su_data.Operation = SI_RECOVERY;
		su_data.Info.UpgradeRecovery.ArchiveCallback =
			dsr_al_progress_cb;
		su_data.Info.UpgradeRecovery.ArchiveData =
			(void *) disp_data;
		su_data.Info.UpgradeRecovery.ScriptCallback =
			pfg_upgrade_progress_cb;
		su_data.Info.UpgradeRecovery.ScriptData =
			(void *) disp_data;
	} else {
		su_data.Operation = SI_UPGRADE;
		su_data.Info.Upgrade.ScriptCallback =
			pfg_upgrade_progress_cb;
		su_data.Info.Upgrade.ScriptData =
			(void *) disp_data;
	}

	/*
	 * call the backend
	 */
	ret = SystemUpdate(&su_data);
	if (ret == SUSuccess) {
		/* upgrade succeeded */
		xm_SetWidgetString(disp_data->main_label,
			LABEL_UPGRADE_PROGRESS_COMPLETE);
		xm_SetWidgetString(disp_data->detail_label, NULL);

		pfgSetAction(parAContinue);
	} else {
		/* the installation failed, exit */
		pfAppError(NULL, SUGetErrorText(ret));
		pfgSetAction(parAUpgradeFail);
	}

	pfgProgressBarUpdate(disp_data, TRUE);
	pfgProgressBarCleanup(disp_data);

	return;
}
ay[WI_INITIALBUTTON]); n++;
  XtSetValues(widget_array[WI_UPGRADE_DIALOG], args, n);


  /***************** panelhelpText : XmText *****************/
  n = 0;
  ttbl = XtParseTranslationTable("#override\n\
~Ctrl ~Meta<BtnDown>:\n\
~Ctrl ~Meta<BtnUp>:");
  XtOverrideTranslations(widget_array[WI_PANELHELPTEXT], ttbl);
  unregister_as_dropsite(widget_array[WI_PANELHELPTEXT],
                         NULL,
                         NULL);

  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX]); n++;
  XtSetValues(widget_array[WI_PANELHELPTEXT], args, n);

  XtManageChild(widget_array[WI_PANELHELPTEXT]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, widget_array[WI_INITIALBUTTON]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX], args, n);

  XtAddCallback(widget_array[WI_UPGRADEBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)upgradeCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_UPGRADEBUTTON]);
  XtAddCallback(widget_array[WI_INITIALBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)initialCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_INITIALBUTTON]);
  XtAddCallback(widget_array[WI_GOBACKBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)upgrade_goback_cb,
                (XtPointer)NULL);

  XtAddCallback(widget_array[WI_EXITBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)pfgExit,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_EXITBUTTON]);
  XtAddCallback(widget_array[WI_HELPBUTTON],
                XmNactivateCallback,
                pfgHelp,
                (XtPointer)"upgrade.t");

  XtManageChild(widget_array[WI_HELPBUTTON]);
  XtManageChild(widget_array[WI_MESSAGEBOX]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*9);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*9);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_UPGRADE_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_UPGRADE_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_UPGRADE_DIALOG];
}



/****************************************************************
 * create_method:
 *     This function creates a widget hierarchy using the
 *     functions generated above.
 ****************************************************************/
static Widget create_method(char               * temp,
                            char               * name,
                            Widget               parent,
                            Display            * disp,
                            Screen             * screen,
                            tu_template_descr  * retval)
{
  Widget w;

  sDisplay = disp;
  sScreen = screen;

  /* check each node against its name and call its
   * create function if appropriate */
  w = NULL;
  if (strcmp(temp, "upgrade_dialog") == 0){
    w = tu_upgrade_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

