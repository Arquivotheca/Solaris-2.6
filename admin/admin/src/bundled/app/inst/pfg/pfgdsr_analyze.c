#ifndef lint
#pragma ident "@(#)pfgdsr_analyze.c 1.9 96/08/16 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgdsr_analyze.c
 * Group:	installtool
 * Description:
 */
#include "pfg.h"
#include "pfgprogressbar.h"

static ValStage prev_stage = VAL_UNKNOWN;
static char *prev_detail = NULL;
static int prev_percent = -1;

Widget
pfgCreateDsrAnalyze(void)
{
	char buf[PATH_MAX];
	UIProgressBarInitData	init_data;
	pfgProgressBarDisplayData *display_data;
	Widget dsr_analyze_dialog;

	init_data.title = TITLE_SW_ANALYZE;
	init_data.main_msg = MSG_SW_ANALYZE;
	init_data.main_label = NULL;
	init_data.detail_label = NULL;
	init_data.percent = 0;

	prev_stage = VAL_UNKNOWN;
	if (prev_detail)
		free(prev_detail);
	prev_detail = NULL;
	prev_percent = 0;

	/* create the progress bar display */
	dsr_analyze_dialog = pfgProgressBarCreate(&init_data, &display_data, 1);
	display_data->scale_info[PROGBAR_SW_ANALYZE_INDEX].start = 0;
	display_data->scale_info[PROGBAR_SW_ANALYZE_INDEX].factor = 1;

	/*
	 * force an initial update.
	 */
	xm_ForceDisplayUpdate(pfgTopLevel, display_data->dialog);

	/*
	 * Make the analyze call.
	 * No conclusions are made at this point regarding space
	 * availability and whether or not DSR is required.
	 * This is just analyzing the current system.
	 * This will cache data so that subsequent analyzes will be fast
	 * and won't require a progress bar.
	 * The subsequent analyzes are the ones that determine if DSR is
	 * still required (after sw selections are made or file systems
	 * are collapsed.)
	 */
	(void) DsrFSAnalyzeSystem(FsSpaceInfo, NULL,
		pfg_upgrade_progress_cb, (void *)display_data);

	/* make sure we show 100% completion when the analyze is done */
	XmScaleSetValue(display_data->scale, 100);
	(void) sprintf(buf, "%s", LABEL_SW_ANALYZE_COMPLETE);
	xm_SetWidgetString(display_data->main_label, buf);
	xm_SetWidgetString(display_data->detail_label, NULL);

	pfgProgressBarUpdate(display_data, TRUE);
	pfgProgressBarCleanup(display_data);

	pfgSetAction(parAContinue);
	return (dsr_analyze_dialog);
}

/* ARGSUSED */
int
pfg_upgrade_progress_cb(void *mydata, void *progress_data)
{
	pfgProgressBarDisplayData *display_data =
		(pfgProgressBarDisplayData *)mydata;
	ValProgress *cb_data = (ValProgress *)progress_data;
	char *main_label;
	char *detail_label;

	write_debug(GUI_DEBUG_L1, "Entering pfg_upgrade_progress_cb");

	if (!display_data)
		return (FAILURE);

	xm_ForceDisplayUpdate(pfgTopLevel, display_data->dialog);

	if (!cb_data)
		return (FAILURE);

	/* update the msg */
	AppGetUpgradeProgressStr(cb_data, &main_label, &detail_label);

	if (main_label) {
		if (prev_stage != cb_data->valp_stage) {
			xm_SetWidgetString(display_data->main_label,
				main_label);
		}
		free(main_label);
	} else if (cb_data->valp_stage != VAL_UNKNOWN) {
		/* leave it alone (i.e. the last stage) if it's unknown */
		xm_SetWidgetString(display_data->main_label, NULL);
	}

	if (detail_label) {
		if (!prev_detail ||
			!streq(prev_detail, detail_label)) {
			xm_SetWidgetString(display_data->detail_label,
				detail_label);
		}
		if (prev_detail)
			free(prev_detail);
		free(detail_label);
	} else {
		xm_SetWidgetString(display_data->detail_label, NULL);
	}

	/* update the scale */
	if (prev_percent != cb_data->valp_percent_done) {
		XmScaleSetValue(display_data->scale,
			UI_ScalePercent(
			cb_data->valp_percent_done,
			display_data->scale_info[PROGBAR_SW_ANALYZE_INDEX].start,
			display_data->scale_info[PROGBAR_SW_ANALYZE_INDEX].factor));
		write_debug(GUI_DEBUG_L1,
			"scale value = %d (start = %d, factor = %ld)",
			UI_ScalePercent(cb_data->valp_percent_done,
			display_data->scale_info[PROGBAR_SW_ANALYZE_INDEX].start,
			display_data->scale_info[PROGBAR_SW_ANALYZE_INDEX].factor),
			display_data->scale_info[PROGBAR_SW_ANALYZE_INDEX].start,
			display_data->scale_info[PROGBAR_SW_ANALYZE_INDEX].factor);
	}

	prev_stage = cb_data->valp_stage;
	prev_detail = xstrdup(cb_data->valp_detail);
	prev_percent = cb_data->valp_percent_done;

	xm_ForceDisplayUpdate(pfgTopLevel, display_data->dialog);
	return (SUCCESS);
}
