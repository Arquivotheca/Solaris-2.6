#ifndef lint
#pragma ident "@(#)pfgdsr_al.c 1.13 96/09/03 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgdsr_media_progress.c
 * Group:	installtool
 * Description:
 */
#include "pfg.h"
#include "pfgprogressbar.h"

static char *prev_detail_label = NULL;
static char *curr_main_label = NULL;
static void _clear_label(char **prev_label);

/*
 * *************************************************************************
 *	Code for the DSR Media Backup list Progress screen
 * *************************************************************************
 */

Widget
pfgCreateDsrALGenerateProgress(void)
{
	TDSRALError err;
	TList slhandle;
	char *envp;
	DsrSLListExtraData *LLextra;
	Widget dsr_media_progress_dialog;
	UIProgressBarInitData	init_data;
	pfgProgressBarDisplayData *display_data;

	/* create the progress bar */
	init_data.title = TITLE_DSR_ALGEN;
	init_data.main_msg = MSG_DSR_ALGEN;
	init_data.main_label = LABEL_DSR_ALGEN_FS;
	init_data.detail_label = NULL;
	init_data.percent = 0;

	dsr_media_progress_dialog =
		pfgProgressBarCreate(&init_data, &display_data, 1);
	display_data->scale_info[PROGBAR_ALGEN_INDEX].start = 0;
	display_data->scale_info[PROGBAR_ALGEN_INDEX].factor = 1;

	/* create the archive list handle if we don't already have one */
	if (!DsrALHandle) {
		(void) DSRALCreate(&DsrALHandle);
	}

	/* get list level user data */
	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);

	/* make the generate backup list call */
	if ((envp = getenv("SYS_LOCAL_AL")) != NULL) {
		/* simulate search on one local file filesystem */
		char slice_str[16];

		(void) strcpy(slice_str, envp);
		write_debug(GUI_DEBUG_L1,
			"testing DSR AL on local system: (%s)", slice_str);

		if (LLCreateList(&slhandle, (TLLData) NULL) != SLSuccess) {
			write_debug(GUI_DEBUG_L1,
				"creating test SL list for failed");
			pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
		}

		if (SLAdd(slhandle,
			slice_str,
			0,
			NULL,
			TRUE,
			SLUnknown,
			SLChangeable,
			(ulong) 0,
			(FSspace *) NULL,
			(void *) NULL,
			NULL) != SLSuccess) {

			write_debug(GUI_DEBUG_L1,
				"adding %s to test SL list failed", slice_str);
			pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
		}

		set_rootdir("/");
	} else {
		/* do the real thing */
		slhandle = DsrSLHandle;
	}
/* 	slhandle = DsrSLHandle; */

	/*
	 * 1st - generate the upgrade script
	 * The /a/var/sadm/system/admin/upgrade_script must be in
	 * existence before the archive list is generated.
	 */
	err = gen_upgrade_script();
	write_debug(GUI_DEBUG_L1,
		"gen_upgrade_script returned = %d", err);
	if (err != SUCCESS) {
		/*
		 * failure here is fatal -
		 * give them the usual useless "inconsistent state" message
		 */
		pfgExitError(pfgTopLevel, pfErUNMOUNT);
	}

	/* generate the archive list */
	err = DSRALGenerate(DsrALHandle, slhandle,
		dsr_al_progress_cb, (void *)display_data,
		&LLextra->archive_size);
	write_debug(GUI_DEBUG_L1, "Archive Size: %lld", LLextra->archive_size);

	/*
	 * generate complete - update the display w/success or failure
	 * message, pause, and exit
	 */
	xm_SetWidgetString(display_data->detail_label, NULL);
	if (err == DSRALSuccess) {
		if (LLextra->archive_size == 0) {
			write_debug(GUI_DEBUG_L1,
				"DSRALGenerate - no files to backup");
		}
		pfgSetAction(parAContinue);
	} else {
		write_debug(GUI_DEBUG_L1, "DSRALGenerate failed with err = %d",
			err);
		xm_SetWidgetString(display_data->detail_label,
			LABEL_DSR_ALGEN_FAIL);

		/* on failure - we drop back to the previous screen */
		pfgSetAction(parAGoback);
	}
	pfgProgressBarUpdate(display_data, TRUE);

	pfgProgressBarUpdate(display_data, TRUE);
	pfgProgressBarCleanup(display_data);
	return (dsr_media_progress_dialog);
}

int
dsr_al_progress_cb(void *client_data, void *call_data)
{
	TDSRALStateData *media_data = (TDSRALStateData *) call_data;
	pfgProgressBarDisplayData *display_data =
		(pfgProgressBarDisplayData *)client_data;
	char *buf;
	char buf1[PATH_MAX];
	char note[PATH_MAX];
	int ind;

	pfgProgressBarUpdate(display_data, FALSE);
	if (!display_data)
		return (FAILURE);

	switch (media_data->State) {
	case DSRALNewMedia:
		/*
		 * Backup or restore: they need to insert another media
		 */
		write_debug(GUI_DEBUG_L1, "DSRALNewMedia: %s #%d",
			media_data->Data.NewMedia.Operation == DSRALBackup ?
				"Backup" : "Restore",
			media_data->Data.NewMedia.MediaNumber);

		/* set up the "make sure it's not write protected" note */
		if (media_data->Data.NewMedia.Operation == DSRALRestore) {
			(void) strcpy(note, "");
		} else {
			(void) sprintf(note,
				media_data->Data.NewMedia.Media == DSRALFloppy ?
					MSG_DSR_MEDIA_INSERT_FLOPPY_NOTE :
					MSG_DSR_MEDIA_INSERT_TAPE_NOTE,
				media_data->Data.NewMedia.Media == DSRALFloppy ?
					LABEL_DSR_MEDIA_FLOPPY :
					LABEL_DSR_MEDIA_TAPE);
		}

		/* the main "insert media" message */
		(void) sprintf(buf1, MSG_DSR_MEDIA_ANOTHER,
			media_data->Data.NewMedia.Media == DSRALFloppy ?
				LABEL_DSR_MEDIA_FLOPPY :
				LABEL_DSR_MEDIA_TAPE,
			media_data->Data.NewMedia.MediaNumber,
			note);

		pfAppWarn(0, buf1);
		break;
	case DSRALBackupBegin:
		xm_SetWidgetString(display_data->main_label,
			LABEL_DSR_ALBACKUP_PROGRESS);
		curr_main_label = LABEL_DSR_ALBACKUP_PROGRESS;
		xm_SetWidgetString(display_data->detail_label, NULL);
		_clear_label(&prev_detail_label);
		break;
	case DSRALBackupEnd:
		xm_SetWidgetString(display_data->main_label,
			LABEL_DSR_ALBACKUP_COMPLETE);
		xm_SetWidgetString(display_data->detail_label, NULL);
		_clear_label(&prev_detail_label);
		pfgProgressBarUpdate(display_data, TRUE);

		/*
		 * Now, since there are currently no newfs update
		 * callbacks, stick up the newfs label...
		 */
		xm_SetWidgetString(display_data->main_label,
			LABEL_PROGRESS_PARTITIONING);
		xm_SetWidgetString(display_data->detail_label, NULL);
		_clear_label(&prev_detail_label);
		curr_main_label = NULL;
		pfgProgressBarUpdate(display_data, TRUE);

		break;
	case DSRALRestoreBegin:
		xm_SetWidgetString(display_data->main_label,
			LABEL_DSR_ALRESTORE_PROGRESS);
		curr_main_label = LABEL_DSR_ALRESTORE_PROGRESS;
		xm_SetWidgetString(display_data->detail_label, NULL);
		_clear_label(&prev_detail_label);
		break;
	case DSRALRestoreEnd:
		xm_SetWidgetString(display_data->main_label,
			LABEL_DSR_ALRESTORE_COMPLETE);
		xm_SetWidgetString(display_data->detail_label, NULL);
		_clear_label(&prev_detail_label);
		curr_main_label = NULL;
		pfgProgressBarUpdate(display_data, TRUE);
		break;
	case DSRALBackupUpdate:
	case DSRALRestoreUpdate:
		/*
		 * Backup or restore: progress update
		 */
		write_debug(GUI_DEBUG_L1, "DSRALFileUpdate:");
		write_debug(GUI_DEBUG_NOHD, LEVEL2, "file: %s",
			media_data->Data.FileUpdate.FileName);
		write_debug(GUI_DEBUG_NOHD, LEVEL2, "percent complete: %d",
			media_data->Data.FileUpdate.PercentComplete);

		if (media_data->State == DSRALBackupUpdate)
			ind = PROGBAR_ALBACKUP_INDEX;
		else
			ind = PROGBAR_ALRESTORE_INDEX;

		/*
		 * update the detail (file name) label only if it is
		 * different from last time.
		 */
		buf = xstrdup(media_data->Data.FileUpdate.FileName);
		UI_ProgressBarTrimDetailLabel(
			curr_main_label,
			buf,
			APP_UI_UPG_PROGRESS_STR_LEN);
		if (!prev_detail_label || !streq(prev_detail_label, buf)) {
			xm_SetWidgetString(display_data->detail_label, buf);
			_clear_label(&prev_detail_label);
			prev_detail_label = buf;
		} else {
			free(buf);
		}

		/* update the scale */
		XmScaleSetValue(display_data->scale,
			UI_ScalePercent(
				media_data->Data.FileUpdate.PercentComplete,
				display_data->scale_info[ind].start,
				display_data->scale_info[ind].factor));
		break;
	case DSRALGenerateBegin:
		_clear_label(&prev_detail_label);
		curr_main_label = LABEL_DSR_ALGEN_FS;
		break;
	case DSRALGenerateEnd:
		xm_SetWidgetString(display_data->main_label,
			LABEL_DSR_ALGEN_COMPLETE);
		xm_SetWidgetString(display_data->detail_label, NULL);
		_clear_label(&prev_detail_label);
		curr_main_label = NULL;
		pfgProgressBarUpdate(display_data, TRUE);
		break;
	case DSRALGenerateUpdate:
		/*
		 * Archive list Generation: progress update
		 */
		write_debug(GUI_DEBUG_L1, "DSRALGenerateUpdate:");
		write_debug(GUI_DEBUG_NOHD, LEVEL2, "contents file: %s",
			media_data->Data.GenerateUpdate.ContentsFile);
		write_debug(GUI_DEBUG_NOHD, LEVEL2, "file system: %s",
			media_data->Data.GenerateUpdate.FileSystem);
		write_debug(GUI_DEBUG_NOHD, LEVEL2, "percent complete: %d",
			media_data->Data.GenerateUpdate.PercentComplete);

		/*
		 * update the detail (file sys) label only if it is
		 * different from last time.
		 */
		buf = xstrdup(media_data->Data.GenerateUpdate.FileSystem);
		UI_ProgressBarTrimDetailLabel(
			curr_main_label,
			buf,
			APP_UI_UPG_PROGRESS_STR_LEN);
		if (!prev_detail_label || !streq(prev_detail_label, buf)) {
			xm_SetWidgetString(display_data->detail_label, buf);
			_clear_label(&prev_detail_label);
			prev_detail_label = buf;
		} else {
			free(buf);
		}

		/* update the scale */
		XmScaleSetValue(display_data->scale,
			UI_ScalePercent(
			media_data->Data.GenerateUpdate.PercentComplete,
			display_data->scale_info[PROGBAR_ALGEN_INDEX].start,
			display_data->scale_info[PROGBAR_ALGEN_INDEX].factor));
		break;
	}

	pfgProgressBarUpdate(display_data, FALSE);
	write_debug(GUI_DEBUG_L1, "progress callback succesful");
	return (SUCCESS);
}

static void
_clear_label(char **prev_label)
{
	if (prev_label && *prev_label)
		free(*prev_label);
	*prev_label = NULL;
}
