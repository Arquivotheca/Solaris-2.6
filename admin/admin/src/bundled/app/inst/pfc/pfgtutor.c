#ifndef lint
#pragma ident "@(#)pfgtutor.c 1.31 96/09/03 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgtutor.c
 * Group:	ttinstall
 * Description:
 */

#include <stdlib.h>
#include <unistd.h>

#include "pf.h"
#include "pfgprocess.h"

/* should the intro screen be presented? */
static int do_parIntro;

/* private functions */
static void history_push(parWin_t win);
static parWin_t history_pop(void);
static void history_clear(void);
static void history_print(void);
static int history_cnt_win(parWin_t win);
static void history_rm_prev(parWin_t win);

/* walk thru parade of screens */
void
pfgParade(parWin_t win)
{
	int err;
	static int reboot;
	parAction_t action = parANone; /* action set by the toplevel screens */

	/*
	 * Find out up front if we are supposed to be handling the
	 * intro screen here or if sysidtool already did it.
	 *
	 * NOTE: maybe we shouldn't actually remove this file unless
	 * they exit successfully (so that if they exit and restart,
	 * then they'll still get this window and the parade will be
	 * consistent until they've actually changed their system?)
	 */
	if (unlink(PARADE_INTRO_FILE) == 0) {
		do_parIntro = 1;
	} else {
		do_parIntro = 0;
	}

	/*
	 * loop processing parade windows and
	 * actions
	 */
	while (action != parAExit) {

	/*
	 * handle all go back requests here so individual windows
	 * don't have to.
	 */
	if (action == parAGoback) {
		/* pop off the currently displayed screen */
		(void) history_pop();

		/*
		 * pop off and get the type of the 2nd to last screen
		 * that we're supposed to go back to.
		 */
		win = history_pop();
		if (win == parNoWin) {
			pfcCleanExit(EXIT_INSTALL_FAILURE, (void *) 1);
		}

		/*
		 * Check and see if we are in upgrade's child process.
		 * If we are and we are trying to back up into
		 * what should be the parent's domain, then exit
		 * the child and let the parent pick up from here.
		 */
		if ((pfgState & AppState_UPGRADE_CHILD) &&
			(win == parOs || win == parUpgrade)) {
			pfcChildShutdown(ChildUpgGoback);
		}
	}

	/* push the current window on the stack now */
	history_push(win);
	if (get_trace_level() > 5)
		history_print();

	pfgSetAction(parANone); /* reset the action to take to None */

	/* process the current window now */
	switch (win) {
	case parIntro:
		write_debug(CUI_DEBUG_L1, "parade: parIntro");

		/*
		 * If the intro screen was already displayed at the
		 * beginning of sysidtool, don't do it here and
		 * don't let the user go back to it either.
		 */
		if (do_parIntro) {
			action = pfgProcessIntro(win);
		} else {
			/*
			 * Remove any trace of this from the window stack
			 * and proceed to the next window.
			 */
			action = parAContinue;
			(void) history_pop();
		}

		switch (action) {
		case parAContinue:
			if (SliceIsSystemUpgradeable(UpgradeSlices)) {
				/* continue to the upgrade */
				win = parUpgrade;
			} else {
				/*
				 * system is not upgradable, continue to
				 * intro initial screen
				 */
				win = parIntroInitial;
			}
			break;
		}
		break;
	case parIntroInitial:
		write_debug(CUI_DEBUG_L1, "parade: parIntroInitial");
		action = pfgProcessIntro(win);

		switch (action) {
		case parAContinue:
			win = parAllocateSvcQuery;
			break;
		}
		break;
	case parUpgrade:
		write_debug(CUI_DEBUG_L1, "parade: parUpgrade");

		/*
		 * create upgrade screen and process actions returned.
		 * for upgrade there are two possible actions. parInitial
		 * for initial install, and parAUpgrade to perform an upgrade
		 */
		action = pfgProcessUpgrade(); /* create upgrade screen */

		switch (action) {
		case parAUpgrade:
			/*
			 * Make sure any errors/warnings, etc are going
			 * to the proper log file.
			 */
			ErrWarnLogFileName = DFLT_INSTALL_LOG_FILE;
			(void) write_error_register_log(ErrWarnLogFileName);
			(void) write_warning_register_log(ErrWarnLogFileName);

			/* make sure a slice is selected */
			SliceSelectOne(UpgradeSlices);

			win = parOs;
			break;
		case parAInitial:
			/*
			 * Make sure any errors/warnings, etc are going
			 * to the proper log file.
			 */
			ErrWarnLogFileName = DFLT_UPGRADE_LOG_FILE;
			(void) write_error_register_log(ErrWarnLogFileName);
			(void) write_warning_register_log(ErrWarnLogFileName);

			/*
			 * reset sw lib view after coming back from
			 * upgrade path and make sure disk has been unmounted
			 */
			if (pfgState & AppState_UPGRADE) {
				if (AppUpgradeResetToInitial(&pfgState)
					== FAILURE) {
					pfgUnmountOrSwapError();
					/* we've just exitted */
				}

			}
			win = parIntroInitial;
			break;
		}
		break;
	case parAllocateSvcQuery:
		write_debug(CUI_DEBUG_L1, "parade: parAllocateSvcQuery");
		action = pfgProcessAllocateSvcQuery();

		switch (action) {
		case parAContinue:
			switch (get_machinetype()) {
			case MT_SERVER:
				win = parClients;
				break;
			case MT_STANDALONE:
			default:
				if (get_all_locales()) {
					win = parLocales;
				} else {
					win = parSw;
				}
				break;
			}
			break;
		}
		break;
	case parOs:
		/*
		 * parOs processing is interesting...
		 * It is responsible for handling starting the upgrade
		 * on the correct slice.  If there is only one possible
		 * upgradeable slice, then the actual "Select Version
		 * to Upgrade" screen is not presented.  If there are more
		 * than one, then parOs is responsible for querying the
		 * user for the slice to upgrade.
		 * In either case, we treat parOs as a discrete step in
		 * the parade and let the pfgProcessOs() processing handle
		 * whether or not a window actually gets created..
		 * that gets presented in th
		 * Among other things this brought the logic for moving
		 * to parLocales or parSwQuery into just one spot here
		 * rather than having it spread out elsewhere in the
		 * parade depending on whether parOs got presented or
		 * not.
		 */
		write_debug(CUI_DEBUG_L1, "parade: parOs");
		action = pfgProcessOs();

		switch (action) {
		case parAContinue:
			if (pfgState & AppState_UPGRADE_CHILD) {
				/* the child process is running here */
				if (get_all_locales()) {
					win = parLocales;
				} else {
					win = parDsrAnalyze;
				}
			} else {
				/* the parent process is running here */
				win = parUpgradeProgress;
			}
			break;
		case parAChange:
			/*
			 * This is the parent processing here.
			 * The child should never get here, since it
			 * should exit with a change exit code before it
			 * gets here.
			 * If the parent needed to be reinitialized, it
			 * will have already been done down in the parOs
			 * processing.
			 */
			history_clear();
			history_push(parIntro);
			if (pfgState & AppState_UPGRADE) {
				history_push(parUpgrade);
				win = parOs;
			}
			break;
		case parAInitial:
			/*
			 * The child should never get here, since it
			 * should exit with an initial exit code before it
			 * gets here.
			 */

			/*
			 * Fix the window history so that just the appropriate
			 * intro screens are there in the parent.
			 */
			history_clear();
			if (do_parIntro)
				history_push(parIntro);

			win = parIntroInitial;
			break;
		case parAComeback:
			/*
			 * The child should never get here, since it
			 * should exit with a goback exit code before it
			 * gets here.
			 *
			 * Normal go back for this screen is handled as
			 * totally normal go back.
			 *
			 * A Comeback here in the parent really means
			 * that the child process is requesting a
			 * goback across the parent/child boundary.
			 * This really means not a goback from parOs,
			 * but a goback to parOs (or, from the parent's
			 * perspective, a comeback to this screen
			 * again...)
			 */
			if (SliceGetTotalNumUpgradeable(UpgradeSlices) > 1) {
				/*
				 * Dummy up the window stack so that
				 * the Comeback processing brings us back to
				 * the last window the parent displayed.
				 * i.e. goback pops 2 windows off the
				 * stack and pushed the 2nd one back on -
				 * push on a dummy window that can get
				 * popped off first in this case (i.e. it
				 * essentially simulates that it's the
				 * parLocales or parSwQuery window getting
				 * popped off).
				 */
				history_push(parNoWin);
				action = parAGoback;
			} else {
				action = parAGoback;
			}
			break;
		}
		break;
	case parClientParams:
		write_debug(CUI_DEBUG_L1, "parade: parClientParams");
		action = pfgProcessClientParams();

		switch (action) {
		case parAContinue:
			if (get_all_locales()) {
				win = parLocales;
			} else {
				win = parSw;
			}
			break;
		default:
			break;
		}
		break;
	case parClients:
		write_debug(CUI_DEBUG_L1, "parade: parClients");
		action = pfgProcessClients();
		switch (action) {
		case parAContinue:
			win = parClientParams;
			break;
		}
		break;
	case parSw:
		write_debug(CUI_DEBUG_L1, "parade: parSw");
		action = pfgProcessSw();
		switch (action) {
		case parAContinue:
			win = parUsedisks;
			break;
		}
		break;
	case parLocales:
		write_debug(CUI_DEBUG_L1, "parade: parLocales");
		action = pfgProcessLocales();

		switch (action) {
		case parAContinue:
			/* test if we are doing an upgrade */
			if (pfgState & AppState_UPGRADE) {
				win = parDsrAnalyze;
			} else {
				win = parSw;
			}
			break;
		}
		break;
	case parUsedisks:
		write_debug(CUI_DEBUG_L1, "parade: parUsedisks");
		action = pfgProcessUseDisks();

		switch (action) {
		case parAContinue:
			if (any_preservable_filesystems()) {
				win = parPrequery;
			} else {
				win = parAutoQuery;
			}
			break;
		}
		break;
	case parPrequery:
		write_debug(CUI_DEBUG_L1, "parade: parPrequery");
		action = pfgProcessPreQuery();

		switch (action) {
		case parAContinue:
			win = parAutoQuery;
			break;
		}
		break;
	case parAutoQuery:
		write_debug(CUI_DEBUG_L1, "parade: parAutoQuery");
		action = pfgProcessAutoQuery();

		switch (action) {
		case parAContinue:
			win = parFilesys;
			break;
		}
		break;
	case parFilesys:
		write_debug(CUI_DEBUG_L1, "parade: parFilesys");
		action = pfgProcessFilesys();

		switch (action) {
		case parAContinue:
			win = parRemquery;
			break;
		}
		break;
	case parRemquery:
		write_debug(CUI_DEBUG_L1, "parade: parRemquery");
		action = pfgProcessRemquery();

		switch (action) {
		case parAContinue:
			win = parSummary;
			break;
		}
		break;
	case parSummary:
		write_debug(CUI_DEBUG_L1, "parade: parSummary");
		action = pfgProcessSummary();

		switch (action) {
		case parAContinue:
			if (pfgState & AppState_UPGRADE) {
				/*
				 * At this point, we are in the child process
				 * and we want to proceed forward to the actual
				 * upgrade now (i.e. the SystemUpdate()).
				 * SystemUpdate is in the parent's realm,
				 * so exit the child with the appropriate
				 * exit code and let the parent pick up
				 * from here (in the return from parOs).
				 */
				if (pfgState & AppState_UPGRADE_DSR)
					/*
					 * DSR upgrade path has already
					 * generated the upgrade script
					 * prior to doing a
					 * DSRALGenerate in the child
					 */
					pfcChildShutdown(ChildUpgDsr);
				else {
					/*
					 * regular upgrade path has to
					 * generate the upgrade script
					 * in the child
					 */
					err = gen_upgrade_script();
					if (err != SUCCESS) {
						pfcChildShutdown(
							ChildUpgExitFailure);
					}
					pfcChildShutdown(ChildUpgNormal);
				}
			} else {
				/* initial path */
				win = parReboot;
			}
			break;
		case parAChange:
			if (pfgState & AppState_UPGRADE) {
				/*
				 * At this point, we are in the child process
				 * and we are jumping back to the beginning
				 * of the upgrade parade (i.e. we are
				 * exitting the child and dropping back
				 * into the parent's process.)
				 * So, exit the child and let the parent
				 * pick up from here
				 * (in the return from parOs).
				 */
				pfcChildShutdown(ChildUpgChange);
				break;
			} else {
				/* initial path */
				history_clear();
				if (do_parIntro)
					history_push(parIntro);
				history_push(parIntroInitial);
				win = parAllocateSvcQuery;
			}
			break;
		}
		break;
	case parUpgradeProgress:
		/* this is only encountered in the upgrade parent process */

		write_debug(CUI_DEBUG_L1, "parade: parUpgradeProgress");
		action = pfgProcessUpgradeProgress();

		switch (action) {
		case parAContinue:
			/* upgrade successful - exit application */
			pfcCleanExit(EXIT_INSTALL_SUCCESS_NOREBOOT,
				(void *) 0);

			/* NOTREACHED */
			break;
		case parAUpgradeFail:
			/* upgrade failed - this is fatal */
			pfcCleanExit(EXIT_INSTALL_FAILURE, (void *) 0);
			break;
		}
		break;
	case parReboot:
		write_debug(CUI_DEBUG_L1, "parade: parReboot");
		action = pfgProcessReboot();

		switch (action) {
		case parAReboot:
			reboot = TRUE;
			break;
		case parANone:
		default:
			reboot = FALSE;
			break;
		}
		win = parProgress;
		break;
	case parProgress:
		write_debug(CUI_DEBUG_L1, "parade: parProgress");
		action = pfgProcessProgress();

		switch (action) {
		case parAContinue:
			if (reboot) {
				if (GetSimulation(SIM_EXECUTE))
					write_debug(CUI_DEBUG_L1,
						"reboot would occur");
				else
					pfcCleanExit(
						EXIT_INSTALL_SUCCESS_REBOOT,
						(void *) 1);
			}
			/* exit to sh */
			pfcCleanExit(EXIT_INSTALL_SUCCESS_NOREBOOT,
				(void *) 1);
			break;
		case parAExit:
			pfcCleanExit(EXIT_INSTALL_FAILURE,
				(void *) 1);
			break;
		}
		break;
	case parSwQuery: /* upgrade only */
		write_debug(CUI_DEBUG_L1, "parade: parSwQuery");
		action = pfgProcessSwQuery();

		switch (action) {
		case parAContinue:
			/* DSR not required */
			pfgState &= ~AppState_UPGRADE_DSR;
			win = parSummary;
			break;
		case parADsrSpaceReq:
			/* DSR required */
			pfgState |= AppState_UPGRADE_DSR;
			win = parDsrSpaceReq;

			break;
		}
		break;
	case parDsrAnalyze:
		/*
		 * This comes up prior to the sw customization screen.
		 * We analyze the system the 1st time with a progress bar
		 * based on the current system setup and then re-analyze
		 * the system after they have customized software to do
		 * a final space check.
		 */
		write_debug(CUI_DEBUG_L1, "parade: parDsrAnalyze");
		action = pfgProcessDsrAnalyze();

		win = parSwQuery;

		/* analyze is not a screen you can go back to */
		(void) history_pop();

		break;
	case parDsrFSRedist:
		write_debug(CUI_DEBUG_L1, "parade: parDsrFSRedist");
		history_rm_prev(parDsrFSRedist);

		action = pfgProcessDsrFSRedist();

		switch (action) {
		case parAContinue:
			win = parDsrFSSummary;
			break;
		}
		break;
	case parDsrFSSummary:
		write_debug(CUI_DEBUG_L1, "parade: parDsrFSSummary");
		history_rm_prev(parDsrFSSummary);

		action = pfgProcessDsrFSSummary();

		/*
		 * The only time we want FS Summary to stay on the stack
		 * is if they are proceeding forward from here...
		 */
		switch (action) {
		case parAContinue:
			win = parDsrALGenerateProgress;
			break;
		case parADsrFSRedist:
			win = parDsrFSRedist;
			break;
		}
		break;
	case parDsrALGenerateProgress:
		write_debug(CUI_DEBUG_L1, "parade: parDsrALGenerateProgress");
		action = pfgProcessDsrALGenerateProgress();

		switch (action) {
		case parAContinue:
			win = parDsrMedia;
			/* media progress is not a screen you can go back to */
			(void) history_pop();
			break;
		}
		break;
	case parDsrMedia:
		write_debug(CUI_DEBUG_L1, "parade: parDsrMedia");
		action = pfgProcessDsrMedia();

		switch (action) {
		case parAContinue:
			win = parSummary;
			break;
		}
		break;
	case parDsrSpaceReq:
		write_debug(CUI_DEBUG_L1, "parade: parDsrSpaceReq");
		action = pfgProcessDsrSpaceReq();

		switch (action) {
		case parADsrFSSumm:
			win = parDsrFSSummary;
			break;
		case parADsrFSRedist:
			win = parDsrFSRedist;
			break;
		}
		break;
	} /* end window switch */

	/*
	 * handle exit requests from any screen
	 */
	if (action == parAExit) {
		action = pfgConfirmExit();
		if (action == parAExit) {
			pfcCleanExit(EXIT_INSTALL_FAILURE,
				(void *) 1);
		}
	}

	} /* end while */
}

/* push pop stack of parWin_t */

static parWin_t par_stack[parWin_t_count];
static int par_stack_last = 0;

static void
history_push(parWin_t win)
{
	if (++par_stack_last >= parWin_t_count) {
		(void) printf("pfgParade: internal error in history_push\n");
		pfcCleanExit(EXIT_INSTALL_FAILURE,
			(void *) 1);
	}
	par_stack[par_stack_last] = win;
}

static parWin_t
history_pop(void)
{
	if (--par_stack_last < 0) {
		par_stack_last = 0;
		write_debug(CUI_DEBUG_L1, "stack is empty can't go back");
		return (parNoWin);
	}
	return (par_stack[par_stack_last+1]);
}


static void
history_clear(void)
{
	par_stack_last = 0;
}

int
parade_prev_win(void)
{
	parWin_t curr_win;
	parWin_t prev_win;

	curr_win = history_pop();
	prev_win = history_pop();

	if (prev_win == parNoWin) {
		history_push(curr_win);
		return (FALSE);
	} else {
		/* put the windows back on the stack */
		history_push(prev_win);
		history_push(curr_win);
		return (TRUE);
	}

	/* NOTREACHED */
}

static int
history_cnt_win(parWin_t win)
{
	int i;
	int cnt;

	for (i = 0, cnt = 0; i <= par_stack_last; i++) {
		if (par_stack[i] == win)
			cnt++;
	}

	return (cnt);
}

static void
history_print(void)
{
	int i;

	write_debug(CUI_DEBUG_L1, "Window stack:");
	for (i = 0; i <= par_stack_last; i++) {
		write_debug(CUI_DEBUG_L1_NOHD,
			"Window stack[%d]: %d", i, par_stack[i]);
	}
}

/*
 * remove the previous version of this window from the stack,
 * if more than one of them exits
 */
static void
history_rm_prev(parWin_t win)
{
	int i;
	int win_num;

	if (history_cnt_win(win) <= 1)
		return;

	/* find the first occurrence of 'win' on the stack */
	for (i = 0, win_num = -1; i <= par_stack_last && win_num == -1; i++) {
		if (par_stack[i] == win)
			win_num = i;
	}

	/* there were no such windows on the stack */
	if (win_num == -1)
		return;

	/* remove win from the stack by sliding everything else down over it */
	for (i = win_num; i < par_stack_last; i++) {
		par_stack[i] = par_stack[i + 1];
	}

	par_stack_last--;
}
