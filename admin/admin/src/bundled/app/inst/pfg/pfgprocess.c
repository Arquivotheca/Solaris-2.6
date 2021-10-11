#ifndef lint
#pragma ident "@(#)pfgprocess.c 1.17 96/10/07 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgprocess.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"

parAction_t
pfgProcessIntro(parWin_t win)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateIntro(win);

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

parAction_t
pfgProcessUpgrade(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateUpgrade();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

parAction_t
pfgProcessUpgradeProgress(void)
{
	parAction_t action;

	/*
	 * The 'event loop' per se is handled inside of the progress
	 * call here (i.e. it's handled via forced updates, etc, during
	 * the backend processing.
	 * So, the X processing is complete when we exit this call.
	 */
	pfgCreateUpgradeProgress();

	/*
	 * parade code will just exit with appropriate exit code based
	 * on action.
	 */
	action = pfgGetAction();

	return (action);
}

parAction_t
pfgProcessAllocateSvcQuery(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateAllocateSvcQuery();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

parAction_t
pfgProcessLocales(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateLocales();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

parAction_t
pfgProcessSw(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateSw();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

parAction_t
pfgProcessUseDisks(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateUseDisks();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

parAction_t
pfgProcessPreQuery(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreatePreQuery();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

parAction_t
pfgProcessAutoQuery(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateAutoQuery();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

parAction_t
pfgProcessFilesys(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateFilesys();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

parAction_t
pfgProcessRemquery(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateRemquery();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}


parAction_t
pfgProcessSummary(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateSummary();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	if (pfgState & AppState_UPGRADE_CHILD) {
		pfgSetCurrentScreen(NULL);
	}

	return (action);
}


parAction_t
pfgProcessOs(void)
{
	parAction_t action;
	TChildAction status;
	Widget ww;

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
			pfgExit,
			pfgParentReinit,
			(void *) &pfgParentReinitData);

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
			 * and should just exit or go into an
			 * initial install.
			 * ChildUpgSliceFailure should never happen here since
			 * there was only one upgradeable slice to begin
			 * with.
			 */
			action = AppParentContinueUpgrade(
				status, &pfgState, pfgCleanExit);

			/*
			 * If we haven't exitted yet, then
			 * the selected slice to upgrade is OK
			 * and we can proceed with the upgrade,
			 * or we are proceeding with an initial install.
			 *
			 * If we are resuming an upgrade, then there
			 * was never a child process in which case the
			 * parent still needs to handle destroying the
			 * previous window.
			 * This is true if we forced the user down
			 * the initial install path as well.
			 * Otherwise, no matter what, the child has already
			 * killed the window the parent has displayed most
			 * previously, so make sure the parent doesn't try
			 * and destroy it again by setting the current
			 * screen to NULL.
			 */
			if (action == parAInitial) {
				pfgSetCurrentScreen(NULL);
			} else if (action != parAGoback) {
				if (pfgState & AppState_UPGRADE_RECOVER) {
					pfgSetCurrentScreen(NULL);
				} else {
					pfgSetCurrentScreenWidget(NULL);
				}
			}
			pfgSetAction(action);
			return (action);
		}
	} else {
		ww = pfgCreateOs();
		pfgSetCurrentScreen(ww);

		action = pfgEventLoop();

		if (pfgState & AppState_UPGRADE_CHILD) {
			pfgSetAction(parAContinue);
			return (parAContinue);
		} else if (action == parAInitial) {
			pfgSetCurrentScreen(NULL);
		} else if (action != parAGoback) {
			pfgSetCurrentScreenWidget(NULL);
			if (pfgState & AppState_UPGRADE_RECOVER) {
				pfgSetCurrentScreen(NULL);
			} else {
				pfgSetCurrentScreenWidget(NULL);
			}
		}
		return (action);
	}
}


parAction_t
pfgProcessSwQuery(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateSwQuery();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}


/* *** this will go back in 2.6 *** */
/*
parAction_t
pfgProcessServiceSelection(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateClientSelector();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);

}
*/

parAction_t
pfgProcessClientSetup(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateClientSetup();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);

}

parAction_t
pfgProcessClients(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateClients();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

/*
 * process reboot screen.  This function is different internal from
 * the other process functions because the reboot screen is a query
 * screen that handles its own event processing and returns true/false.
 */

parAction_t
pfgProcessReboot(void)
{
	int reboot;

	/*
	 * query screen destroies itself so pass
	 * null to current screen.
	 */
	pfgSetCurrentScreen(NULL);

	reboot = pfgQuery(pfgTopLevel, pfQREBOOT);

	if (reboot) {
		return (parAReboot);
	} else {
		return (parANone);
	}
}

parAction_t
pfgProcessProgress(void)
{
	parAction_t action;

	/*
	 * No need to handle return widget and setting current screen
	 * here since the application immediately exits after the
	 * initial path progress bar (regradless of the value returned
	 * from the update call.
	 */
	pfgCreateProgress();

	action = pfgSystemUpdateInitial();

	return (action);
}

/*
 * used to display unmount or delete swap error from pfgParade
 */
void
pfgUnmountOrSwapError(void)
{
	pfgExitError(pfgTopLevel, pfErUNMOUNT);
}

parAction_t
pfgConfirmExit(void)
{
	return (parAContinue);
}

/*
 * stub function, only needed in CUI
 */
parAction_t
pfgProcessClientParams(void)
{
	return (parAContinue);
}

parAction_t
pfgProcessDsrAnalyze(void)
{
	parAction_t action;
	Widget ww;

	/* destroy the previous screen */
	pfgSetCurrentScreen(NULL);

	ww = pfgCreateDsrAnalyze();

	pfgSetCurrentScreen(ww);

/* 	action = pfgEventLoop(); */
	action = pfgGetAction();

	return (action);
}

parAction_t
pfgProcessDsrFSRedist(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateDsrFSRedist();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

parAction_t
pfgProcessDsrFSSummary(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateDsrFSSummary();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

parAction_t
pfgProcessDsrMedia(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateDsrMedia();

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}

parAction_t
pfgProcessDsrALGenerateProgress(void)
{
	parAction_t action;
	Widget ww;

	/* destroy the previous screen */
	pfgSetCurrentScreen(NULL);
	ww = pfgCreateDsrALGenerateProgress();

	pfgSetCurrentScreen(ww);

/* 	action = pfgEventLoop(); */
	action = pfgGetAction();

	return (action);
}
parAction_t
pfgProcessDsrSpaceReq(void)
{
	parAction_t action;
	Widget ww;

	ww = pfgCreateDsrSpaceReq(TRUE);

	pfgSetCurrentScreen(ww);

	action = pfgEventLoop();

	return (action);
}
