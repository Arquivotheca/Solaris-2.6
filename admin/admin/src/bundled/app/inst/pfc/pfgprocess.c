#ifndef lint
#pragma ident "@(#)pfgprocess.c 1.31 96/08/07 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgprocess.c
 * Group:	ttinstall
 * Description:
 */

#include <stdlib.h>
#include <libintl.h>
#include <unistd.h>

#include "pf.h"
#include "inst_msgs.h"
#include "summary_util.h"
#include "inst_parade.h"
#include "v_check.h"
#include "v_types.h"
#include "v_misc.h"
#include "v_upgrade.h"

/* static globals */
static parAction_t UIaction = parANone;

parAction_t
pfgProcessIntro(parWin_t win)
{
	return (do_install_intro(win));
}

parAction_t
pfgProcessUpgrade()
{
	return (do_upgrade_or_install());
}

parAction_t
pfgProcessAllocateSvcQuery()
{
	return (do_systype());
}

parAction_t
pfgProcessLocales()
{
	return (do_alt_lang());
}

parAction_t
pfgProcessSw()
{
	return (do_sw());
}

parAction_t
pfgProcessUseDisks()
{
	return (do_choose_disks());
}

parAction_t
pfgProcessPreQuery()
{
	return (do_preserve_fs());
}

parAction_t
pfgProcessAutoQuery()
{
	return (do_fs_autoconfig());
}

parAction_t
pfgProcessFilesys()
{
	parAction_t action = parANone;
	int done = 0;

	while (!done) {
		action = do_show_filesystems();

		/*
		 * if the action is either goback or exit, then we're done,
		 * else put of the warning window if necessary.
		 */
		if (action != parAContinue) {
			done = 1;
		} else if (v_check_part() == CONFIG_WARNING) {
			if ((action = do_fs_space_warning()) == parAContinue) {
				done = 1;
			}
		} else {
			done = 1;
		}
	}
	return (action);
}


parAction_t
pfgProcessRemquery()
{
	return (do_rfs());
}


parAction_t
pfgProcessSummary()
{
	if (!(pfgState & AppState_UPGRADE)) {
		return (do_initial_summary());
	} else {
		return (do_upgrade_summary());
	}
}


parAction_t
pfgProcessOs()
{
	parAction_t action;
	TChildAction status;

	/*
	 * Upon entry to this routine we are always in the parent
	 * upgrade process.
	 * Down inside the AppParentStartUpgrade() routine, the
	 * parent/child split is made.
	 */

	/*
	 * If no parOs window is actually displayed.
	 */
	if (SliceGetTotalNumUpgradeable(UpgradeSlices) == 1) {
		/*
		 * initialize sw lib, etc. with the new slice to
		 * upgrade.
		 */
		status = AppParentStartUpgrade(
			&FsSpaceInfo,
			UpgradeSlices,
			&pfgState,
			pfcExit,
			NULL,
			(void *) NULL);

		if (pfgState & AppState_UPGRADE_CHILD) {
			/*
			 * the child continues normal forward parade
			 * processing here.
			 */
			pfgSetAction(parAContinue);
			return (parAContinue);
		} else {
			/* we're in the parent */

			/*
			 * Anything but a ChildUpgSliceFailure means
			 * we're either ok and should continue or
			 * we're hosed and can't try any more slices
			 * and should just exit.
			 * ChildUpgSliceFailure should never happen here since
			 * there was only one upgradeable slice to begin
			 * with.
			 */
			action = AppParentContinueUpgrade(
				status, &pfgState, pfcCleanExit);

			/*
			 * If we haven't exitted yet, then
			 * the selected slice to upgrade is OK
			 * and we can proceed with the upgrade.
			 */
			pfgSetAction(action);
			return (action);
		}
	} else {
		action = do_os();

		if (pfgState & AppState_UPGRADE_CHILD) {
			pfgSetAction(parAContinue);
			return (parAContinue);
		}
		return (action);
	}

	/* NOTREACHED */
}

parAction_t
pfgProcessSwQuery()
{
	return (upgrade_sw_edit());
}

parAction_t
pfgProcessClientParams()
{
	return (do_server_params());
}

parAction_t
pfgProcessClients()
{
	return (do_client_arches());
}

/*
 * process reboot screen.  This function is different internal from
 * the other process functions because the reboot screen is a query
 * screen that handles its own event processing and returns true/false.
 */

parAction_t
pfgProcessReboot()
{
	int reboot;

	reboot = _confirm_reboot();

	switch (reboot) {
		case 1:
			return (parAReboot);
		case 0:
			return (parANone);
		case -1:
			return (parAGoback);
	}

	/*NOTREACHED*/
}

parAction_t
pfgProcessProgress()
{
	return (v_do_install());
}

parAction_t
pfgProcessUpgradeProgress(void)
{
	return (do_upgrade_progress());
}

parAction_t
pfgProcessDsrALGenerateProgress(void)
{
	return (do_dsr_al_progress());
}

parAction_t
pfgProcessDsrAnalyze(void)
{
	return (do_dsr_analyze());
}

parAction_t
pfgProcessDsrFSRedist(void)
{
	return (do_dsr_fsredist());
}

parAction_t
pfgProcessDsrFSSummary(void)
{
	return (do_dsr_fssummary());
}

parAction_t
pfgProcessDsrMedia(void)
{
	return (do_dsr_media());
}

parAction_t
pfgProcessDsrSpaceReq(void)
{
	return (do_dsr_space_req(TRUE));
}


/*
 * function used to set the action to be taken
 */
void
pfgSetAction(parAction_t action)
{
	UIaction = action;
}

/*
 * function to get action specified by the screen whose events
 * were just processed
 */
parAction_t
pfgGetAction(void)
{
	return (UIaction);
}
