#ifndef lint
#pragma ident "@(#)inst_dsr_al.c 1.16 96/09/03 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_dsr_al.c
 * Group:	ttinstall
 * Description:
 *	DSR archive list module
 */
#include <malloc.h>

#include "pf.h"
#include "inst_progressbar.h"
#include "inst_msgs.h"

static char *prev_detail_label = NULL;
static char *curr_main_label = NULL;
static void _clear_label(char **prev_label);

/*
 * *************************************************************************
 *	Code for the DSR Media Backup list Progress screen
 * *************************************************************************
 */

parAction_t
do_dsr_al_progress(void)
{
	UIProgressBarInitData	init_data;
	pfcProgressBarDisplayData *display_data;
	DsrSLListExtraData *LLextra;
	TDSRALError err;
	parAction_t action;

	/* create the progress bar */
	init_data.title = TITLE_DSR_ALGEN;
	init_data.main_msg = MSG_DSR_ALGEN;
	init_data.main_label = LABEL_DSR_ALGEN_FS;
	init_data.detail_label = NULL;
	init_data.percent = 0;

	pfcProgressBarCreate(&init_data, &display_data, NULL, 1);
	display_data->scale_info[PROGBAR_ALGEN_INDEX].start = 0;
	display_data->scale_info[PROGBAR_ALGEN_INDEX].factor = 1;
	wstatus_msg(stdscr, PLEASE_WAIT_STR);
	(void) wnoutrefresh(stdscr);

	/* create the archive list handle if we don't already have one */
	if (!DsrALHandle) {
		(void) DSRALCreate(&DsrALHandle);
	}

	/* get list level user data */
	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);

	/*
	 * 1st - generate the upgrade script
	 * The /a/var/sadm/system/admin/upgrade_script must be in
	 * existence before the archive list is generated.
	 */
	err = gen_upgrade_script();
	write_debug(CUI_DEBUG_L1,
		"gen_upgrade_script returned = %d", err);
	if (err != SUCCESS) {
		/*
		 * failure here is fatal -
		 */
		pfcCleanExit(EXIT_INSTALL_FAILURE, (void *) 1);
	}

#if 0
/* DUMMY DATA!!! */
slentry = DsrSLGetEntry(DsrSLHandle, "/usr", 0);
slentry->State = SLMoveable;
#endif

	/* generate the archive list */
	err = DSRALGenerate(DsrALHandle, DsrSLHandle,
		dsr_al_progress_cb, (void *)display_data,
		&LLextra->archive_size);
	write_debug(CUI_DEBUG_L1, "Archive Size: %lld", LLextra->archive_size);

	/*
	 * generate complete - update the display w/success or failure
	 * message, pause, and exit
	 */
	if (err == DSRALSuccess) {
		if (LLextra->archive_size == 0) {
			write_debug(CUI_DEBUG_L1,
				"DSRALGenerate - no files to backup");
		}
		pfcProgressBarUpdate(display_data, TRUE,
			PROGRESSBAR_DETAIL_LABEL, NULL,
			PROGRESSBAR_PERCENT, 100,
			NULL);
		action = parAContinue;
	} else {
		write_debug(CUI_DEBUG_L1, "DSRALGenerate failed with err = %d",
			err);
		pfcProgressBarUpdate(display_data, TRUE,
			PROGRESSBAR_DETAIL_LABEL, LABEL_DSR_ALGEN_FAIL,
			PROGRESSBAR_PERCENT, 100,
			NULL);

		/* on failure - we drop back to the previous screen */
		action =  parAGoback;
	}
	pfcProgressBarCleanup(display_data);

	/* flush any user input befor we leave here */
	flush_input();

	return (action);
}

int
dsr_al_progress_cb(void *client_data, void *call_data)
{
	TDSRALStateData *media_data = (TDSRALStateData *) call_data;
	pfcProgressBarDisplayData *display_data =
		(pfcProgressBarDisplayData *)client_data;
	char *buf;
	char buf1[PATH_MAX];
	char note[PATH_MAX];
	int ind;

	if (!display_data)
		return (FAILURE);

	switch (media_data->State) {
	case DSRALNewMedia:
		/*
		 * Backup or restore: they need to insert another media
		 */
		write_debug(CUI_DEBUG_L1, "DSRALNewMedia: %s #%d",
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

		(void) simple_notice(display_data->win,
			F_OKEYDOKEY,
			TITLE_DSR_MEDIA_INSERT,
			buf1);
		break;
	case DSRALBackupBegin:
		ind = PROGBAR_ALBACKUP_INDEX;
		pfcProgressBarUpdate(display_data, FALSE,
			PROGRESSBAR_MAIN_LABEL, LABEL_DSR_ALBACKUP_PROGRESS,
			PROGRESSBAR_DETAIL_LABEL, NULL,
			PROGRESSBAR_PERCENT,
			UI_ScalePercent(
				media_data->Data.FileUpdate.PercentComplete,
				display_data->scale_info[ind].start,
				display_data->scale_info[ind].factor),
			NULL);
		curr_main_label = LABEL_DSR_ALBACKUP_PROGRESS;
		_clear_label(&prev_detail_label);
		break;
	case DSRALBackupEnd:
		ind = PROGBAR_ALBACKUP_INDEX;
		pfcProgressBarUpdate(display_data, TRUE,
			PROGRESSBAR_MAIN_LABEL, LABEL_DSR_ALBACKUP_COMPLETE,
			PROGRESSBAR_DETAIL_LABEL, NULL,
			PROGRESSBAR_PERCENT,
			UI_ScalePercent(
				media_data->Data.FileUpdate.PercentComplete,
				display_data->scale_info[ind].start,
				display_data->scale_info[ind].factor),
			NULL);
		_clear_label(&prev_detail_label);
		curr_main_label = NULL;

		/*
		 * Now, since there are currently no newfs update
		 * callbacks, stick up the newfs label...
		 */
		pfcProgressBarUpdate(display_data, TRUE,
			PROGRESSBAR_MAIN_LABEL, LABEL_PROGRESS_PARTITIONING,
			PROGRESSBAR_DETAIL_LABEL, NULL,
			NULL);
		_clear_label(&prev_detail_label);

		break;
	case DSRALRestoreBegin:
		ind = PROGBAR_ALRESTORE_INDEX;
		pfcProgressBarUpdate(display_data, FALSE,
			PROGRESSBAR_MAIN_LABEL, LABEL_DSR_ALRESTORE_PROGRESS,
			PROGRESSBAR_DETAIL_LABEL, NULL,
			PROGRESSBAR_PERCENT,
			UI_ScalePercent(
				media_data->Data.FileUpdate.PercentComplete,
				display_data->scale_info[ind].start,
				display_data->scale_info[ind].factor),
			NULL);
		curr_main_label = LABEL_DSR_ALRESTORE_PROGRESS;
		_clear_label(&prev_detail_label);
		break;
	case DSRALRestoreEnd:
		ind = PROGBAR_ALRESTORE_INDEX;
		pfcProgressBarUpdate(display_data, TRUE,
			PROGRESSBAR_MAIN_LABEL, LABEL_DSR_ALRESTORE_COMPLETE,
			PROGRESSBAR_DETAIL_LABEL, NULL,
			PROGRESSBAR_PERCENT,
			UI_ScalePercent(
				media_data->Data.FileUpdate.PercentComplete,
				display_data->scale_info[ind].start,
				display_data->scale_info[ind].factor),
			NULL);
		_clear_label(&prev_detail_label);
		curr_main_label = NULL;
		break;
	case DSRALBackupUpdate:
	case DSRALRestoreUpdate:
		/*
		 * Backup or restore: progress update
		 */
		write_debug(CUI_DEBUG_L1, "DSRALFileUpdate:");
		write_debug(CUI_DEBUG_NOHD, LEVEL2, "file: %s",
			media_data->Data.FileUpdate.FileName);
		write_debug(CUI_DEBUG_NOHD, LEVEL2, "percent complete: %d",
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
			pfcProgressBarUpdate(display_data, FALSE,
				PROGRESSBAR_DETAIL_LABEL, buf,
				NULL);
			_clear_label(&prev_detail_label);
			prev_detail_label = buf;
		} else {
			free(buf);
		}

		/* update the scale */
		pfcProgressBarUpdate(display_data, FALSE,
			PROGRESSBAR_PERCENT,
			UI_ScalePercent(
				media_data->Data.FileUpdate.PercentComplete,
				display_data->scale_info[ind].start,
				display_data->scale_info[ind].factor),
			NULL);
		break;
	case DSRALGenerateBegin:
		_clear_label(&prev_detail_label);
		curr_main_label = LABEL_DSR_ALGEN_FS;
		break;
	case DSRALGenerateEnd:
		ind = PROGBAR_ALGEN_INDEX;
		pfcProgressBarUpdate(display_data, TRUE,
			PROGRESSBAR_MAIN_LABEL, LABEL_DSR_ALGEN_COMPLETE,
			PROGRESSBAR_DETAIL_LABEL, NULL,
			PROGRESSBAR_PERCENT,
			UI_ScalePercent(
				media_data->Data.FileUpdate.PercentComplete,
				display_data->scale_info[ind].start,
				display_data->scale_info[ind].factor),
			NULL);
		_clear_label(&prev_detail_label);
		curr_main_label = NULL;
		break;
	case DSRALGenerateUpdate:
		/*
		 * Archive list Generation: progress update
		 */
		write_debug(CUI_DEBUG_L1, "DSRALGenerateUpdate:");
		write_debug(CUI_DEBUG_NOHD, LEVEL2, "contents file: %s",
			media_data->Data.GenerateUpdate.ContentsFile);
		write_debug(CUI_DEBUG_NOHD, LEVEL2, "file system: %s",
			media_data->Data.GenerateUpdate.FileSystem);
		write_debug(CUI_DEBUG_NOHD, LEVEL2, "percent complete: %d",
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
			pfcProgressBarUpdate(display_data, FALSE,
				PROGRESSBAR_DETAIL_LABEL, buf,
				NULL);
			_clear_label(&prev_detail_label);
			prev_detail_label = buf;
		} else {
			free(buf);
		}

		/* update the scale */
		pfcProgressBarUpdate(display_data, FALSE,
			PROGRESSBAR_PERCENT,
			UI_ScalePercent(
			media_data->Data.GenerateUpdate.PercentComplete,
			display_data->scale_info[PROGBAR_ALGEN_INDEX].start,
			display_data->scale_info[PROGBAR_ALGEN_INDEX].factor),
			NULL);
		break;
	}

	return (SUCCESS);
}

static void
_clear_label(char **prev_label)
{
	if (prev_label && *prev_label)
		free(*prev_label);
	*prev_label = NULL;
}
