#ifndef lint
#pragma ident "@(#)upg_summary.c 1.79 96/09/12 SMI"
#endif

/*
 * Copyright (c) 1991-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	upg_summary.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/bitmap.h>
#include <locale.h>
#include <libintl.h>
#include <string.h>

#include "pf.h"
#include "tty_pfc.h"
#include "pfgprocess.h"
#include "inst_msgs.h"
#include "summary_util.h"
#include "v_types.h"
#include "v_disk.h"
#include "v_misc.h"
#include "v_sw.h"
#include "v_upgrade.h"
#include "inst_progressbar.h"

static int num_slices = 0;

static parAction_t _get_upgrade_disk(void);

/*
 * Upgrade or Initial Install?
 *
 * returns:
 *	1:	upgrade
 *	2:	install
 *	0:	exit
 * 	-1:	go back
 */
parAction_t
do_upgrade_or_install()
{
	int ch;
	unsigned long fkeys;

	if (!SliceIsSystemUpgradeable(UpgradeSlices)) {
		return (parAInitial);
	}
	(void) wclear(stdscr);
	(void) werase(stdscr);

	wheader(stdscr, TITLE_UPGRADE);

	(void) wword_wrap(stdscr, HeaderLines, INDENT0, COLS - (2 * INDENT0),
	    MSG_UPGRADE);

	if (parade_prev_win()) {
		fkeys = F_INSTALL | F_UPGRADE | F_GOBACK | F_EXIT | F_HELP;
	} else {
		fkeys = F_INSTALL | F_UPGRADE | F_EXIT | F_HELP;
	}

	wfooter(stdscr, fkeys);
	wcursor_hide(stdscr);

	for (;;) {

		flush_input();
		ch = wzgetch(stdscr, fkeys);

		if (is_install(ch)) {
			return (parAInitial);
		} else if (is_upgrade(ch)) {
			pfgState |= AppState_UPGRADE;
			wstatus_msg(stdscr, PLEASE_WAIT_STR);
			return (parAUpgrade);
		} else if (is_exit(ch)) {
			if (confirm_exit(stdscr))
				return (parAExit);
		} else if ((fkeys & F_GOBACK) && is_goback(ch)) {
			return (parAGoback);
		} else if (is_help(ch)) {
			do_help_index(stdscr, HELP_TOPIC, "Upgrading Option");
		} else if (is_escape(ch)) {
			continue;
		} else
			beep();
	}

	/* NOTREACHED */
}

/*
 * Select Solaris OS to Upgrade
 *
 * when there are several disks that are potential targets for an upgrade,
 * need to ask user to select one.
 *
 * this function puts up a dialog asking the user to select a disk from the
 * list of possible targets.
 *
 */

parAction_t
do_os(void)
{
	parAction_t action;
	int done = 0;
	char buf[128];
	UpgOs_t *new_slice;
	TChildAction status;

	/* make the first slice selected if one isn't already selected */
	SliceSelectOne(UpgradeSlices);

	/*
	 * continue looping until a disk is selected
	 * that is upgradeable or we run out of disks
	 */
	while (!done) {
		action = _get_upgrade_disk();
		if (action == parAExit || action == parAGoback) {
			return (action);
		}

		new_slice = SliceGetSelected(UpgradeSlices, NULL);

		/*
		 * initialize sw lib, etc. with the new slice to
		 * upgrade.
		 */
		/* checking release status message */
		(void) sprintf(buf,
			UPG_LOAD_INSTALLED_STATUS_MSG, new_slice->slice);
		wstatus_msg(stdscr, buf);

		status = AppParentStartUpgrade(
			&FsSpaceInfo,
			UpgradeSlices,
			&pfgState,
			pfcExit,
			NULL,
			(void *) NULL);

		if (pfgState & AppState_UPGRADE_CHILD) {
			pfgSetAction(parAContinue);
			done = 1;
		} else {
			/* we're in the parent */

			if (status != ChildUpgSliceFailure) {
				/*
				 * Anything but a ChildUpgSliceFailure means
				 * we're either ok and should continue or
				 * we're hosed and can't try any more slices
				 * and should just exit.
				 */
				action = AppParentContinueUpgrade(
					status, &pfgState, pfcCleanExit);

				/*
				 * If we haven't exitted yet, then
				 * the selected slice to upgrade is OK
				 * and we can proceed with the upgrade.
				 */
				pfgSetAction(action);
				done = 1;
			}

			/*
			 * A ChildUpgSliceFailure means this slice failed and
			 * there are more possible slices to try and that
			 * we should try another.
			 * Set the currently selected slice insensitive
			 * and select another one for them.
			 */

			/* pick another slice */
			SliceSelectOne(UpgradeSlices);
		}
	}
	return (action);
}

parAction_t
_get_upgrade_disk(void)
{
	char **opts;
	char buf[128];
	unsigned long fkeys;
	int i;
	int sindex;
	int opt_index;
	int ch;
	int row;
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = "Select Version to Upgrade Screen";

	num_slices = SliceGetNumUpgradeable(UpgradeSlices);

	opts = (char **) xcalloc(num_slices * sizeof (char *));

	for (i = 0, sindex = 0; UpgradeSlices[sindex].slice; sindex++) {
		/* skip slices that have already been tried and have failed */
		if (UpgradeSlices[sindex].failed)
			continue;

		(void) sprintf(buf, "%-20s %s",
			UpgradeSlices[sindex].release,
			UpgradeSlices[sindex].slice);

		opts[i++] = (char *) xstrdup(buf);
	}

	(void) werase(stdscr);
	(void) wclear(stdscr);
	wheader(stdscr, TITLE_OS_MULTIPLE);

	row = HeaderLines;
	row = wword_wrap(stdscr, row, INDENT0, COLS - (2 * INDENT0), MSG_OS);
	row += 2;

	fkeys = F_CONTINUE | F_GOBACK | F_EXIT | F_HELP;
	wfooter(stdscr, fkeys);

	/* find the opts index of the currently selected slice */
	SlicePrintDebugInfo(UpgradeSlices);
	SliceGetSelected(UpgradeSlices, &sindex);
	for (i = 0, opt_index = 0; i < sindex; i++) {
		/* if it hasn't failed, then it's in opt */
		if (!UpgradeSlices[i].failed) {
			opt_index++;
		}
	}
	write_debug(CUI_DEBUG_L1, "initially selected slice (%d)", opt_index);

	/* set up a menu label */
	(void) sprintf(buf, "%-25s %s", OS_VERSION_LABEL, LABEL_SLICE);

	/* display the upgradeable slice menu */
	flush_input();
	ch = wmenu(stdscr, row, INDENT1, LINES - HeaderLines - FooterLines,
	    COLS - INDENT1,
	    show_help, (void *) &_help,
	    (Callback_proc *) NULL, (void *) NULL,
	    (Callback_proc *) NULL, (void *) NULL,
	    buf, opts, num_slices, (void *) &opt_index,
	    M_RADIO | M_CHOICE_REQUIRED | M_RADIO_ALWAYS_ONE,
	    fkeys);
	write_debug(CUI_DEBUG_L1, "user selected %s (%d)",
		opts[opt_index], opt_index);

	/* free opts */
	for (i = 0; i < num_slices; i++) {
		if (opts[i] != (char *) NULL)
			free((void *) opts[i]);

	}
	if (opts)
		free((void *) opts);

	/* set the selected slice from the current CUI setting */
	SliceSetUnselected(UpgradeSlices);
	for (i = 0, sindex = 0; UpgradeSlices[sindex].slice; sindex++) {
		if (UpgradeSlices[sindex].failed) {
			continue;
		} else {
			if (i == opt_index) {
				write_debug(CUI_DEBUG_L1,
					"selected %s",
					UpgradeSlices[sindex].slice);
				UpgradeSlices[sindex].selected = 1;
				break;
			} else {
				i++;
			}
		}
	}

	if (is_continue(ch)) {
		return (parAContinue);
	} else if (is_goback(ch)) {
		return (parAGoback);
	} else if (is_exit(ch)) {
		return (parAExit);
	}

	/* NOTREACHED */
}

parAction_t
upgrade_sw_edit()
{
	int ch;
	unsigned long fkeys;
	int dirty;
	int err;

	dirty = 1;
	for (;;) {
		if (dirty) {
			(void) werase(stdscr);
			(void) wclear(stdscr);

			wheader(stdscr, TITLE_UPG_CUSTOM_SWQUERY);
			(void) wword_wrap(stdscr, HeaderLines, INDENT0,
			    COLS - (2 * INDENT0),
			    MSG_UPG_CUSTOM_SWQUERY);

			fkeys = F_CONTINUE | F_GOBACK | F_CUSTOMIZE | F_EXIT;
			dirty = 0;

		}
		wfooter(stdscr, fkeys);
		wcursor_hide(stdscr);
		ch = wzgetch(stdscr, fkeys);

		if (is_continue(ch) != 0) {

			break;

		} else if (is_goback(ch) != 0) {

			break;

		} else if (is_customize(ch) != 0) {

			(void) do_sw_edit();
			dirty = 1;

		} else if (is_exit(ch) != 0) {

			break;

		} else if (is_escape(ch) != 0) {

			continue;

		} else
			beep();
	}

	if (is_continue(ch) != 0) {
		/*
		 * At this point, we have to find out if there really is
		 * enough space on the system to hold all the currently selected
		 * software.
		 * A call to verify_fs_layout along with the parDsrAnalyze
		 * progress bar has already been made, so the assumption here is
		 * that this is a fast call, so there is no progress bar here...
		 */
		err = DsrFSAnalyzeSystem(FsSpaceInfo, NULL, NULL, NULL);
		if (err == SP_ERR_NOT_ENOUGH_SPACE) {
			/*
			 * There are failed file systems.
			 * Now that we will be entering DSR,
			 * create the slice list.
			 */
			if (DsrSLUICreate(&DsrALHandle, &DsrSLHandle,
				FsSpaceInfo)) {
				simple_notice(stdscr, F_OKEYDOKEY, TITLE_ERROR,
					"Internal DSR error - can't create slice list");
				pfcCleanExit(EXIT_INSTALL_FAILURE,
					(void *) NULL);
			}
			if (get_trace_level() > 2) {
				DsrSLPrint(DsrSLHandle, DEBUG_LOC);
			}

			return (parADsrSpaceReq);
		} else {
			/* there are no failed file systems */
			return (parAContinue);
		}
	} else if (is_exit(ch) != 0) {		/* exit */
		return (parAExit);
	} else /* if (is_goback(ch) != 0) */ {	/* go back */
		return (parAGoback);
	}
}

parAction_t
do_upgrade_summary()
{
	int r, c;
	int ch;
	int top_row;
	int last_row;
	int lines_per_page;
	int nlines;
	int cur;
	int top;
	int dirty;
	int really_dirty = 1;
	unsigned long fkeys;
	_Summary_Row_t *table = (_Summary_Row_t *) NULL;

	/* flush any premature user input */
	flush_input();

	cur = top = 0;
	fkeys = F_UPGRADE | F_CHANGE | F_EXIT | F_HELP;

	for (;;) {

		if (really_dirty) {

			(void) werase(stdscr);
			(void) wclear(stdscr);

			wheader(stdscr, gettext("Profile"));

			/* show blurb about this screen */
			top_row = HeaderLines;
			top_row = wword_wrap(stdscr, top_row, INDENT0,
			    COLS - (2 * INDENT0), UPG_SUMMARY_ONSCREEN_HELP);
			top_row++;

			(void) mvwprintw(stdscr, top_row, 2, "%.*s",
			    COLS - 2 - 2, EQUALS_STR);
			top_row += 2;

			wfooter(stdscr, fkeys);

			nlines = 0;
			table = load_install_summary(&nlines);

			/* calculate free lines for display of config */
			lines_per_page = LINES - FooterLines - top_row - 1;
			last_row = top_row + lines_per_page - 1;

			dirty = 1;
			really_dirty = 0;
		}
		if (dirty) {

			show_summary_table(stdscr, nlines, lines_per_page,
			    top_row, table, top);
			dirty = 0;

			scroll_prompts(stdscr, top_row, 1, top, nlines,
			    lines_per_page);

		}
		/* set footer */
		if (table[cur].fld[0].prompt != (char *) 0 &&
		    table[cur].fld[0].prompt[0] != '\0') {
			wstatus_msg(stdscr, table[cur].fld[0].prompt);
		} else {
			wclear_status_msg(stdscr);
		}

		if (nlines > lines_per_page)
			(void) wmove(stdscr, table[cur].fld[1].loc.r,
			    table[cur].fld[1].loc.c - 1);
		else
			wcursor_hide(stdscr);

		(void) getsyx(r, c);
		(void) wnoutrefresh(stdscr);
		(void) setsyx(r, c);
		(void) doupdate();

		ch = wzgetch(stdscr, fkeys);

#ifdef notdef
		if (sel_cmd(ch) != 0) {

			/* category header? */
			if (table[cur].fld[0].sel_proc != (int (*) ()) NULL) {

				/*
				 * call modify proc and then reload table
				 */
				(void) table[cur].fld[0].sel_proc();

				free_summary_table(table, nlines);
				really_dirty = 1;

			} else
				beep();
		} else
#endif
		if (is_exit(ch) != 0) {

			if (confirm_exit(stdscr) == 1)
				break;

		} else if (is_change(ch) != 0) {

			break;

		} else if (is_help(ch) != 0) {

			do_help_index(stdscr, HELP_TOPIC, "Upgrading Option");

		} else if (is_escape(ch) != 0) {

			continue;

		} else if (is_upgrade(ch)) {

			break;

		} else if (ch == U_ARROW || ch == D_ARROW ||
			    ch == CTRL_F || ch == CTRL_D ||
		    ch == CTRL_B || ch == CTRL_U) {

			dirty = 0;

			/* move */
			if (ch == CTRL_D) {

				/* page down */
				if ((cur + lines_per_page) < nlines) {

					/* advance a page */
					top += lines_per_page;
					cur += lines_per_page;
					dirty = 1;

				} else if (cur < (nlines - 1)) {

					/* advance to lat line */
					cur = nlines - 1;
					top = cur - 2;
					dirty = 1;

				} else
					beep();	/* at end */

			} else if (ch == CTRL_U) {

				/* page up */
				if ((cur - lines_per_page) >= 0) {

					/* reverse a page */
					top = (top > lines_per_page ?
					    top - lines_per_page : 0);
					cur -= lines_per_page;
					dirty = 1;

				} else if (cur > 0) {

					/* back to first line */
					top = 0;
					cur = 0;
					dirty = 1;

				} else
					beep();	/* at top */

			} else if (ch == U_ARROW || ch == CTRL_B ||
				ch == CTRL_P) {

				if (table[cur].fld[0].loc.r == top_row) {

					if (top) {	/* scroll down */
						cur = --top;
						dirty = 1;
					} else
						beep();	/* very top */

				} else {
					cur--;
				}

			} else if (ch == D_ARROW || ch == CTRL_F ||
				ch == CTRL_N) {

				if (table[cur].fld[0].loc.r == last_row) {

					if ((cur + 1) < nlines) {

						/* scroll up */
						top++;
						cur++;
						dirty = 1;

					} else
						beep();	/* bottom */

				} else {

					if ((cur + 1) < nlines) {
						cur++;
					} else
						beep();	/* last, no wrap */
				}
			}
		} else
			beep();
	}

	if (is_change(ch) != 0) {
		return (parAChange);
	} else if (is_upgrade(ch)) {
		return (parAContinue);
	} else /* if (is_exit(ch) != 0) */ {
		return (parAExit);
	}
}

parAction_t
do_upgrade_progress(void)
{
	TSUError ret;
	UIProgressBarInitData init_data;
	pfcProgressBarDisplayData *disp_data;
	DsrSLListExtraData *LLextra;
	TSUData su_data;
	parAction_t action;

	/*
	 * Initialize the progress bar display
	 */
	init_data.title = TITLE_UPG_PROGRESS;
	init_data.main_msg = MSG_UPG_PROGRESS;
	init_data.main_label = LABEL_UPG_PROGRESS;
	init_data.detail_label = NULL;
	init_data.percent = 0;

	if (pfgState & AppState_UPGRADE_DSR) {
		(void) pfcProgressBarCreate(
			&init_data, &disp_data,
			stdscr, PROGBAR_PROGRESS_CNT);

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
		(void) pfcProgressBarCreate(
			&init_data, &disp_data,
			stdscr, 2);

		AppUpgradeGetProgressBarInfo(PROGBAR_ALRESTORE_INDEX,
			pfgState,
			&disp_data->scale_info[PROGBAR_ALRESTORE_INDEX].start,
			&disp_data->scale_info[PROGBAR_ALRESTORE_INDEX].factor);

		AppUpgradeGetProgressBarInfo(PROGBAR_UPGRADE_INDEX,
			pfgState,
			&disp_data->scale_info[PROGBAR_UPGRADE_INDEX].start,
			&disp_data->scale_info[PROGBAR_UPGRADE_INDEX].factor);
	} else {
		(void) pfcProgressBarCreate(
			&init_data, &disp_data,
			stdscr, 1);

		AppUpgradeGetProgressBarInfo(PROGBAR_UPGRADE_INDEX,
			pfgState,
			&disp_data->scale_info[PROGBAR_UPGRADE_INDEX].start,
			&disp_data->scale_info[PROGBAR_UPGRADE_INDEX].factor);
	}
	pfcProgressBarUpdate(disp_data, FALSE,
		NULL);

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
			pfc_upgrade_progress_cb;
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
			pfc_upgrade_progress_cb;
		su_data.Info.UpgradeRecovery.ScriptData =
			(void *) disp_data;
	} else {
		su_data.Operation = SI_UPGRADE;
		su_data.Info.Upgrade.ScriptCallback =
			pfc_upgrade_progress_cb;
		su_data.Info.Upgrade.ScriptData =
			(void *) disp_data;
	}

	/*
	 * call the backend
	 */
	ret = SystemUpdate(&su_data);
	if (ret == SUSuccess) {
		/* upgrade succeeded */
		pfcProgressBarUpdate(disp_data, FALSE,
			PROGRESSBAR_MAIN_LABEL,
			LABEL_UPGRADE_PROGRESS_COMPLETE,
			PROGRESSBAR_DETAIL_LABEL, NULL,
			NULL);

		pfcProgressBarUpdate(disp_data, TRUE,
			NULL);
		pfcProgressBarCleanup(disp_data);

		action = parAContinue;
	} else {
		/*
		 * upgrade failed
		 */
		simple_notice(stdscr, F_OKEYDOKEY,
			TITLE_ERROR,
			SUGetErrorText(ret));

		action = parAUpgradeFail;
	}


	/* flush any user input before we leave here */
	flush_input();

	/*
	 * turn off curses and make it so that the write status message
	 * goes to the scr instead of an output file.
	 * Print out to the screen that the upgrade was successful.
	 * This must be done here since it gets masked into a throw-away
	 * debug log when printed from inside SystemUpdate() since
	 * curses is still on at that point;
	 */
	if (action == parAContinue) {
		end_curses(TRUE, FALSE);
		(void) write_status_register_log(NULL);
		write_status(SCR, LEVEL0, CUI_MSG_SU_SYSTEM_UPGRADE_COMPLETE);
		(void) write_status_register_log(StatusScrFileName);
	} else if (action == parAUpgradeFail) {
		end_curses(TRUE, FALSE);
		(void) write_status_register_log(NULL);
		write_status(SCR, LEVEL0, SUGetErrorText(ret));
		(void) write_status_register_log(StatusScrFileName);
	}

	return (action);
}
