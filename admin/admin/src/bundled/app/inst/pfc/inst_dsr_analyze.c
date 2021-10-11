#ifndef lint
#pragma ident "@(#)inst_dsr_analyze.c 1.8 96/08/16 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_dsr_analyze.c
 * Group:	ttinstall
 * Description:
 */

#include <stdlib.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"
#include "inst_progressbar.h"

#define	DSR_BACKUP_PROGRESS_FACTOR	.25
#define	DSR_RESTORE_PROGRESS_FACTOR	.25
#define	UPGRADE_PROGRESS_FACTOR \
	((pfgState & AppState_UPGRADE_DSR) ? .50 : 1.0)

static ValStage prev_stage = VAL_UNKNOWN;
static char *prev_detail = NULL;
static int prev_percent = -1;

parAction_t
do_dsr_analyze(void)
{
	char buf1[PATH_MAX];
	UIProgressBarInitData init_data;
	pfcProgressBarDisplayData *display_data;

	init_data.title = TITLE_SW_ANALYZE;
	init_data.main_msg = MSG_SW_ANALYZE;
	init_data.main_label = LABEL_SW_ANALYZE;
	init_data.detail_label = NULL;
	init_data.percent = 0;

	prev_stage = VAL_UNKNOWN;
	if (prev_detail)
		free(prev_detail);
	prev_detail = NULL;
	prev_percent = 0;

	/* create the progress bar display */
	pfcProgressBarCreate(&init_data, &display_data, NULL, 1);
	display_data->scale_info[PROGBAR_SW_ANALYZE_INDEX].start = 0;
	display_data->scale_info[PROGBAR_SW_ANALYZE_INDEX].factor = 1;
	wstatus_msg(stdscr, PLEASE_WAIT_STR);
	(void) wnoutrefresh(stdscr);

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
		pfc_upgrade_progress_cb, (void *)display_data);

	/* make sure we show 100% completion when the analyze is done */
	(void) sprintf(buf1, "%s", LABEL_SW_ANALYZE_COMPLETE);
	pfcProgressBarUpdate(display_data, TRUE,
		PROGRESSBAR_PERCENT, 100,
		PROGRESSBAR_MAIN_LABEL, buf1,
		PROGRESSBAR_DETAIL_LABEL, NULL,
		NULL);
	pfcProgressBarCleanup(display_data);

	/* flush any user input before we leave here */
	flush_input();

	return (parAContinue);
}

/* ARGSUSED */
int
pfc_upgrade_progress_cb(void *mydata, void *progress_data)
{
	pfcProgressBarDisplayData *display_data =
		(pfcProgressBarDisplayData *)mydata;
	ValProgress *cb_data = (ValProgress *)progress_data;
	char *main_label;
	char *detail_label;

	write_debug(CUI_DEBUG_L1, "Entering pfc_upgrade_progress_cb");

	if (!display_data)
		return (FAILURE);

	if (!cb_data)
		return (FAILURE);

	/* update the msg */
	AppGetUpgradeProgressStr(cb_data, &main_label, &detail_label);

	if (main_label) {
		if (prev_stage != cb_data->valp_stage) {
			pfcProgressBarUpdate(display_data, FALSE,
				PROGRESSBAR_MAIN_LABEL, main_label,
				PROGRESSBAR_DETAIL_LABEL, NULL,
				NULL);
		}
		free(main_label);
	} else if (cb_data->valp_stage != VAL_UNKNOWN) {
		/* leave it alone (i.e. the last stage) if it's unknown */
		pfcProgressBarUpdate(display_data, FALSE,
			PROGRESSBAR_MAIN_LABEL, NULL,
			PROGRESSBAR_DETAIL_LABEL, NULL,
			NULL);
	}

	if (detail_label) {
		if (!prev_detail ||
			!streq(prev_detail, detail_label)) {
			pfcProgressBarUpdate(display_data, FALSE,
				PROGRESSBAR_DETAIL_LABEL, detail_label,
				NULL);
		}
		if (prev_detail)
			free(prev_detail);
		free(detail_label);
	} else {
		pfcProgressBarUpdate(display_data, FALSE,
			PROGRESSBAR_DETAIL_LABEL, NULL,
			NULL);
	}

	/* update the scale */
	if (prev_percent != cb_data->valp_percent_done) {
		pfcProgressBarUpdate(display_data, FALSE,
			PROGRESSBAR_PERCENT,
				UI_ScalePercent(
				cb_data->valp_percent_done,
				display_data->scale_info[PROGBAR_SW_ANALYZE_INDEX].start,
				display_data->scale_info[PROGBAR_SW_ANALYZE_INDEX].factor),
			NULL);
	}

	prev_stage = cb_data->valp_stage;
	prev_detail = xstrdup(cb_data->valp_detail);
	prev_percent = cb_data->valp_percent_done;

	return (SUCCESS);
}
