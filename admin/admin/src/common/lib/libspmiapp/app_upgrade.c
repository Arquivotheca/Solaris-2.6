#ifndef lint
#pragma ident "@(#)app_upgrade.c 1.19 96/10/03 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	app_upgrade.c
 * Group:	libspmiapp
 * Description:
 *	OS upgrade app level code.
 *	The general idea of this module is to get the upgradeable slices
 *	from the library and throw that information into another higher-level
 *	data structure (an array of UpgOs_t's) that the interactive apps can
 *	use to help control user selection of the upgradeable slice.
 *	These are all the Slice*() routines.
 *
 *	This module also contains some abstracted code for handling mount and
 *	swap on the upgradeable slice.
 *	These are all the App*() routines.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include "spmiapp_api.h"
#include "spmiapp_lib.h"
#include "spmisvc_api.h"
#include "app_utils.h"

static int SliceSetSelectedFromBootObj(UpgOs_t *slices);
static TChildAction AppForkSwapChild(FSspace ***fs_space,
	UpgOs_t *slices, UpgOs_t *slice, unsigned int *state,
	void (*confirm_exit_func)(void),
	void (*parent_reinit_func)(void *),
	void *parent_reinit_data);
static TChildAction AppParentProcessSliceError(
	UpgOs_t *slices, UpgOs_t *slice, unsigned int state, int err,
	void (*confirm_exit_func)(void));
static UI_MsgResponseType AppUpgradeForceInitialMsg();

/*
 * Function: SliceGetUpgradeable
 * Description: Return an array of upgradeable slices information. Initially
 *		each slice is marked as not failed. The array is terminated
 *		by a slice entry with a NULL slice field.
 * Scope:	public
 * Parameters:	slices		ptr to address of upgradeable slices array to
 *				be filled in
 * Return:	[void]
 */
void
SliceGetUpgradeable(UpgOs_t **slices)
{
	StringList *	sptr;
	StringList *	vptr;
	StringList *	upgrade_slices = NULL;
	StringList *	upgrade_releases = NULL;
	int		count;
	UpgOs_t *	slice_array;
	char 		buf[24];

	write_debug(APP_DEBUG_L1, "Entering SliceGetUpgradeable");

	/* we already have the data - don't do it again */
	if (*slices)
		return;

	/* find the upgradeable slices */
	SliceFindUpgradeable(&upgrade_slices, &upgrade_releases);

	/*
	 * In debug mode, print out the list of upgradeable slices
	 * and their corresponding Solaris Versions
	 */
	if (get_trace_level() > 0) {
		/* print out the slices returned from the library */
		write_debug(APP_DEBUG_L1, "Upgradeable Slices:");
		vptr = upgrade_releases;
		WALK_LIST(sptr, upgrade_slices) {
			write_debug(APP_DEBUG_L1_NOHD,
				"%s (%s)",
				sptr->string_ptr,
				vptr->string_ptr ? vptr->string_ptr : "");
			vptr = vptr->next;
		}
	}

	/*
	 * Use the slices and releases to fill in the app level data
	 * structure we want to use to track this info.
	 * First - count how many we have and malloc an array of them.
	 * Then fill in the array.
	 */
	count = 0;
	WALK_LIST(sptr, upgrade_slices) {
		count++;
	}

	*slices = (UpgOs_t *) xcalloc(sizeof (UpgOs_t) * (count + 1));
	slice_array = *slices;
	if (!slice_array)
		return;

	count = 0;
	vptr = upgrade_releases;
	WALK_LIST(sptr, upgrade_slices) {
		/*
		 * if in simulation, the release string is empty -
		 * fill in dummy data for the release information since
		 * we don't get release strings in simulation.
		 */
		if (GetSimulation(SIM_EXECUTE)) {
			if (!vptr->string_ptr || !(*vptr->string_ptr)) {
				/* make a release name up */
				switch (count%4) {
				case 0:
					(void) sprintf(buf, "%s %s",
						OS_SOLARIS_PREFIX,
						"2.2");
					break;
				case 1:
					(void) sprintf(buf, "%s %s",
						OS_SOLARIS_PREFIX,
						"2.5.1");
					break;
				case 2:
					(void) sprintf(buf, "%s %s",
						OS_SOLARIS_PREFIX,
						"2.6");
					break;
				case 3:
					(void) sprintf(buf, "%s %s",
						OS_SOLARIS_PREFIX,
						"2.6.10");
					break;
				}
			}
		} else {
			(void) sprintf(buf, "%s %s",
				OS_SOLARIS_PREFIX,
				vptr->string_ptr);
		}
		slice_array[count].release = xstrdup(buf);
		slice_array[count].slice = xstrdup(sptr->string_ptr);
		slice_array[count].failed = 0;
		slice_array[count].selected = 0;
		vptr = vptr->next;
		count++;
	}
	/* the last one in the list is marked by having a null slice entry */
	slice_array[count].slice = NULL;

	/* free the string lists from the library */
	StringListFree(upgrade_slices);
	StringListFree(upgrade_releases);
}

/*
 * Function: SliceFreeUpgradeable
 * Description:
 *	Free the slice structure and any internal data.
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 * Return:	[void]
 * Globals:	none
 * Notes:
 */
void
SliceFreeUpgradeable(UpgOs_t *slices)
{
	int i;

	if (!slices)
		return;

	for (i = 0; SliceGetTotalNumUpgradeable(slices); i++) {
		if (slices[i].slice)
			free(slices[i].slice);
		if (slices[i].release)
			free(slices[i].release);
	}
	free(slices);
}

/*
 * Function: SliceGetTotalNumUpgradeable
 * Description:
 *	Return the total num of slices that were originally deemed
 *	to be potentially upgradeable. (i.e. this is the total
 *	number of slices in the slice array).
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 * Return:	[int]
 * Globals:	none
 * Notes:
 */
int
SliceGetTotalNumUpgradeable(UpgOs_t *slices)
{
	int i;

	if (!slices)
		return (0);

	for (i = 0; slices[i].slice; i++)
		;

	return (i);
}

/*
 * Function: SliceGetNumUpgradeable
 * Description:
 *	Return the number of slices in the slice array that are still
 *	deemed to be potentially upgradeable. (i.e. the number of slices
 *	in the slice array that haven't already failed an ugprade attempt.)
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 * Return:	[int]
 * Globals:	none
 * Notes:
 */
int
SliceGetNumUpgradeable(UpgOs_t *slices)
{
	int i;
	int count;

	if (!slices)
		return (0);

	for (i = 0, count = 0; slices[i].slice; i++) {
		if (!slices[i].failed)
			count++;
	}

	write_debug(APP_DEBUG_L1, "Num Upgradeable Slices: %d", count);
	return (count);
}

/*
 * Function: SliceIsSystemUpgradeable
 * Description:
 *	Return if the system appears to be upgradeable. It appears to
 *	be upgradeable as long there is at least one slice in
 *	the array that hasn't failed yet.
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 * Return:	[int]
 *	TRUE: >0 potentially upgradeable slices still exist
 *	FALSE: 0 potentially upgradeable slices
 * Globals:	none
 * Notes:
 */
int
SliceIsSystemUpgradeable(UpgOs_t *slices)
{
	write_debug(APP_DEBUG_L1,
		"SliceIsSystemUpgradeable = %d\n",
		SliceGetNumUpgradeable(slices) > 0 ? 1 : 0);
	if (SliceGetNumUpgradeable(slices) > 0)
		return (TRUE);
	else
		return (FALSE);
}

/*
 * Function: SliceSelectOne
 * Description:
 *	Pick a slice as selected.
 *	If one is already selected, then that one remains selected.
 *	Next try to pick the existing boot device slice, unless it
 *	is already marked as failed.
 *	Lastly, if we still haven't selected one, just set as selected
 *	the first non-selected, non-failed slice.
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 * Return:	[void]
 * Globals:	none
 * Notes:
 */
void
SliceSelectOne(UpgOs_t *slices)
{
	int i;
	UpgOs_t *slice;

	/*
	 * if there's one selected, leave it selected and we're done,
	 */
	slice = SliceGetSelected(slices, NULL);
	if (slice)
		return;

	/*
	 * try selecting the existing boot device
	 */
	if (SliceSetSelectedFromBootObj(slices))
		return;

	/*
	 * otherwise, select the first valid one in the list.
	 */
	for (i = 0; i < SliceGetTotalNumUpgradeable(slices); i++) {
		if (!slices[i].selected && !slices[i].failed) {
			slices[i].selected = 1;
			break;
		}
	}
	SlicePrintDebugInfo(slices);
}

/*
 * Function: SliceSetSelectedFromBootObj
 * Description:
 *	Try to select an upgradeable slice that matches the existing
 *	boot object.
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 * Return:
 *	TRUE: the boot object slice was set to selected
 *	FALSE: the boot object slice was not set to selected
 * Globals:	none
 * Notes:
 */
static int
SliceSetSelectedFromBootObj(UpgOs_t *slices)
{
	char diskname[32];
	char device_type;
	int device;
	char slice_name[40];
	int i;
	int found;

	write_debug(APP_DEBUG_L1, "Entering SliceSetSelectedFromBootObj");

	/* find the boot current boot object, if there is one */
	if (BootobjGetAttribute(CFG_EXIST,
		BOOTOBJ_DISK, diskname,
		BOOTOBJ_DEVICE_TYPE, &device_type,
		BOOTOBJ_DEVICE, &device,
		NULL) != D_OK) {
		return (FALSE);
	}

	/*
	 * there is a boot object - try and match it to an upgradeable
	 * slice now.
	 */

	found = FALSE;
	(void) sprintf(slice_name, "%s%c%d", diskname, device_type, device);

	if (device_type == 's') {
		/* try and match the boot slice to an upgradeable slice */
		for (i = 0; i < SliceGetTotalNumUpgradeable(slices); i++) {
			if (streq(slice_name, slices[i].slice)) {
				found = TRUE;
				break;
			}
		}
	} else {
		/*
		 * We have to map the fdisk partition to the slice.
		 * Since there's only one Solaris partition per
		 * fdisk, we can can just match the disk portion
		 * of the names.
		 */
		for (i = 0; i < SliceGetTotalNumUpgradeable(slices); i++) {
			if (strneq(slice_name, slices[i].slice,
				strlen(diskname))) {
				found = TRUE;
				break;
			}
		}
	}

	if (found) {
		if (slices[i].failed) {
			found = FALSE;
		} else {
			write_debug(APP_DEBUG_L1, "selecting boot obj slice %s",
				slices[i].slice);
			slices[i].selected = 1;
		}
	}
	return (found);
}

/*
 * Function: SliceGetSelected
 * Description:
 *	Return a ptr to the entry in the slice array that contains
 *	the currently selected slice.
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 *	slice_index: ptr to int for returning slice_index if requested.
 *		- no slice_index is returned if ptr is NULL
 * Return:	[UpgOs_t *]
 * Globals:	none
 * Notes:
 */
UpgOs_t *
SliceGetSelected(UpgOs_t *slices, int *slice_index)
{
	int i;

	write_debug(APP_DEBUG_L1, "Entering SliceGetSelected");

	for (i = 0; i < SliceGetTotalNumUpgradeable(slices); i++) {
		if (slices[i].selected) {
			if (slice_index)
				*slice_index = i;
			return (&slices[i]);
		}
	}
	if (slice_index)
		*slice_index = -1;
	return (NULL);
}

/*
 * Function: SliceSetFailed
 * Description:
 *	Set the currently selected slice to failed (and unselect the slice).
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 * Return:	[void]
 * Globals:	none
 * Notes:
 */
void
SliceSetFailed(UpgOs_t *slices)
{
	int i;

	write_debug(APP_DEBUG_L1, "Entering SliceSetFailed");

	for (i = 0; i < SliceGetTotalNumUpgradeable(slices); i++) {
		if (slices[i].selected) {
			write_debug(APP_DEBUG_L1,
				"slice failure: %s %s",
				slices[i].release,
				slices[i].slice);

			slices[i].failed = 1;
			slices[i].selected = 0;
			break;
		}
	}
}

/*
 * Function: SliceSetUnselected
 * Description:
 *	Unselect the currently selected slice.
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 * Return:	[void]
 * Globals:	none
 * Notes:
 */
void
SliceSetUnselected(UpgOs_t *slices)
{
	int i;

	write_debug(APP_DEBUG_L1, "Entering SliceSetUnselected");

	for (i = 0; i < SliceGetTotalNumUpgradeable(slices); i++) {
		if (slices[i].selected) {
			slices[i].selected = 0;
			break;
		}
	}
}

/*
 * Function: SlicePrintDebugInfo
 * Description:
 *	Print useful debug information for each entry in the slice array.
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 * Return:	[void]
 * Globals:	none
 * Notes:
 */
void
SlicePrintDebugInfo(UpgOs_t *slices)
{
	int i;

	if (get_trace_level() <= 0)
		return;

	if (!slices)
		return;

	write_debug(APP_DEBUG_L1, "Complete slice debug info");
	for (i = 0; i < SliceGetTotalNumUpgradeable(slices); i++) {
		write_debug(APP_DEBUG_L1_NOHD,
			"[%d]: %s (%s)\n\t\tselected: %d\n\t\tfailed: %d",
			i, slices[i].release, slices[i].slice,
			slices[i].selected, slices[i].failed);
	}
}


/*
 * Function: AppUpgradeInitSw
 * Description:
 *	Initialize upgrade (on-disk) software hierarchy.
 * Scope:	public
 * Parameters:
 *	state:  ptr to application state mask - [RW]
 * Return:	[int]
 *	SUCCESS or FAILURE
 * Globals:	none
 * Notes:
 */
int
AppUpgradeInitSw(unsigned int *state)
{
	Module *media;
	Module *prod;
	Module *prodmeta;

	write_debug(APP_DEBUG_L1, "Entering AppUpgradeInitSw");

	/*
	 * Check to see if the system being upgraded is pre-KBI
	 * and if so create the necessary directories.
	 */
	if (SetupPreKBI()) {
		return (FAILURE);
	}

	set_action_code_mode(PRESERVE_IDENTICAL_PACKAGES);

	/*
	 * get product pointer for new OS version
	 */

	if ((prod = get_current_product()) != (Module *) NULL) {
		write_debug(APP_DEBUG_L1, "got current product");

		/*
		 * clear out any pre-existing initial install state...
		 */
		clear_instdir_svc_svr(prod);

		/*
		 * 5% free fs space to be consistent with pfupgrade
		 */
		sw_lib_init(PTYPE_UNKNOWN);
		set_percent_free_space(5);

		/*
		 * get existing OS version from hard disk...
		 */
		if ((media = load_installed("/", FALSE)) == (Module *) NULL) {
			return (FAILURE);
		}

		/*
		 * map the upgrade package requirements onto the new
		 * product's `view' of the software hierarchy
		 */
		if (load_view(prod, media) == SUCCESS) {
			/*
			 * find the metacluster which will be installed and
			 * put this into the structure which tracks what is
			 * to be displayed.
			 */
			if (prod->sub)
				prodmeta = prod->sub;   /* head metacluster */

			/*
			 * media->sub->sub is base metacluster on system
			 * which will be upgraded, need to find it's
			 * corresponding metacluster in the product chain
			 */
			for (; prodmeta != (Module *) NULL;
				prodmeta = prodmeta->next)
				if (streq(prodmeta->info.mod->m_pkgid,
					media->sub->sub->info.mod->m_pkgid))
					break;

			if (prodmeta != (Module *) NULL) {
				write_debug(APP_DEBUG_L1,
					"setting default & current meta");
				set_current(prodmeta);
				set_default(prodmeta);

			}
		}
		/* falls off end, doesn't return a useful value */
		(void) load_clients();

		if (upgrade_all_envs() != SUCCESS)
			return (FAILURE);
		load_view(prod, media);
		*state |= AppState_UPGRADESW;

		return (SUCCESS);
	}
	return (FAILURE);
}

/*
 * Function: AppUpgradeResetToInitial
 *	Function to reset the software library and unmount the disk
 *	after leaving the upgrade path.
 * Description:
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 *	state:  ptr to application state mask - [RW]
 * Return:	[int]
 *	SUCCESS or FAILURE
 * Globals:	none
 * Notes:
 */
int
AppUpgradeResetToInitial(unsigned int *state)
{
	Module *prod, *media;

	write_debug(APP_DEBUG_L1, "Entering AppUpgradeResetToInitial");

	if (*state & AppState_UPGRADE) {
		if (AppUnmountAll() == FAILURE) {
			return (FAILURE);
		}
		if (*state & AppState_UPGRADESW) {
			prod = get_current_product();
			media = get_media_head();
			set_instdir_svc_svr(prod);
			load_view(prod, media);
			initNativeArch();
			*state &= ~AppState_UPGRADESW;
		}
	}

	/* Deselect all disks upon entry to the initial path */
	DiskSelectAll(FALSE);

	/* reset application state to clean slate */
	*state = 0;

	return (SUCCESS);
}

/*
 * Function: AppParentStartUpgrade
 * Description:
 *	This function initializes the input slice as the upgrade
 *	slice. This involves mounting stuff in this slices vfstab,
 *	creating the swap file and
 *	initializing the software library (getting the current file
 *	system layout and initializing the product module structures).
 *
 *	Due to the fact that swap must be mounted in order to run most
 *	of the app (the sw lib analyzing stuff in particular), but that swap
 *	can't be mounted when we do the newfs in DSR, we have to do some
 *	special stuff to work around this.
 *
 *	- fork off a child process
 *	- mount_and_add_swap in the parent process so that it is available for
 *	use by both the parent and the child
 *	- do the actual software analyzing and run most of the parade actually
 *	in the child process
 *	- return to the parent process to actually perform the SystemUpdate()
 *	(where the backup/restore/newfs/upgrade take place)
 *	and unmount swap at that point.
 *
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 *	state:  ptr to application state mask - [RW]
 * Return:	[int]
 * Globals:	none
 * Notes:
 */
TChildAction
AppParentStartUpgrade(
	FSspace ***fs_space,
	UpgOs_t *slices,
	unsigned int *state,
	void (*confirm_exit_func)(void),
	void (*parent_reinit_func)(void *),
	void *parent_reinit_data)
{
	int err;
	TUpgradeResumeState resume_state;
	UpgOs_t *slice;
	char *str;
	TChildAction status;

	write_debug(APP_DEBUG_L1, "Entering AppParentStartUpgrade");
	SlicePrintDebugInfo(slices);

	/* get the slice we're trying to upgrade */
	slice = SliceGetSelected(slices, NULL);
	if (!slice)
		return (ChildUpgExitFailure);

	/* unmount any old disks, if necessary */
	err = AppUnmountAll();
	if (err != SUCCESS) {
		return (AppParentProcessSliceError(slices, slice, *state,
			err, confirm_exit_func));
	}

	err = mount_and_add_swap(slice->slice);
	if (err != SUCCESS) {
		return (AppParentProcessSliceError(slices, slice, *state,
			err, confirm_exit_func));
	}

	/*
	 * Figure out if there is an upgrade to resume.
	 */
	resume_state = UpgradeResume();

	if (resume_state != UpgradeResumeNone) {
		write_debug(APP_DEBUG_L1, "Upgrade is resumable (%d)",
			resume_state);
		/*
		 * There was a partial upgrade that can be resumed.
		 * Find out if the user wants to resume or not.
		 */
		str = (char *) xmalloc(
			strlen(UPG_RECOVER_QUERY) +
			strlen(slice->slice) +
			strlen(slice->release) +
			1);
		(void) sprintf(str, UPG_RECOVER_QUERY,
			slice->slice, slice->release);
		if (UI_DisplayBasicMsg(UI_MSGTYPE_INFORMATION, NULL, str) ==
			UI_MSGRESPONSE_OK) {
			/* resume the upgrade */
			free(str);
			if (resume_state == UpgradeResumeRestore)
				return (ChildUpgRecoverRestore);
			else if (resume_state == UpgradeResumeScript)
				return (ChildUpgRecoverUpgScript);
		} else {
			/* don't resume the upgrade */
			free(str);
			return (ChildUpgRecoverNo);
		}
	}

	/*
	 * We are proceeding with a normal upgrade.
	 * At this point, we need to pass off processing into the
	 * child process...
	 * The parent waits in here until the child is done and then
	 * goes back to the upgrade.
	 * The child cruises on from here directly back into the parade.
	 */
	status = AppForkSwapChild(fs_space, slices, slice, state,
		confirm_exit_func,
		parent_reinit_func,
		parent_reinit_data);

	return (status);
}

/*
 * Function:	AppForkSwapChild
 * Description:
 *	Function that actually forks off the child process for the
 *	upgrade process and loads the requested upgradeable slice's
 *	software info.
 *	The parent process waits in here until the child exits.
 * Scope:	PUBLIC
 * Parameters:
 *	UpgOs_t *slices:
 *		the upgradeable slices array.
 *	UpgOs_t *slice:
 *		the slice we're actually trying to upgrade.
 *	unsigned int *state:
 *		ptr to application state
 *	void (*confirm_exit_func)(void):
 *		function to call to confirm exit requests - may be NULL
 *	void (*parent_reinit_func)(void *):
 *		function to call when parent resumes - may be NULL
 *	void *parent_reinit_data):
 *		data to call function with when parent resumes
 * Return:
 *	TChildAction: one of the child return codes.
 *	The parent and the child process both then map this back to an
 *	appropriate parade action.
 * Globals:
 * Notes:
 */
static TChildAction
AppForkSwapChild(FSspace ***fs_space,
	UpgOs_t *slices, UpgOs_t *slice, unsigned int *state,
	void (*confirm_exit_func)(void),
	void (*parent_reinit_func)(void *),
	void *parent_reinit_data)
{
	int err;
	pid_t child_pid;
	int child_status;
	int exit_code;

#ifndef _NO_FORK
/*
 * No guarantees on how far the use of _NO_FORK will get you.
 * I know it gets me far enough that I can bring up the DSR FS Summary
 * and DSR Auto-layout Constraints screen in the parent process instead
 * (so I can try and use the debugger's memory checking -
 * which doesn't work on attached child processes).
 */
	child_pid = fork();
	if (child_pid == (pid_t) -1) {
		/* fork error - a message here maybe... */
		return (ChildUpgExitFailure);
	} else if (child_pid == 0) {
#endif
		/*
		 * this is the child process
		 */
		write_debug(APP_DEBUG_L1, "Entering child process");
		*state |= AppState_UPGRADE_CHILD;
		*state &= ~AppState_UPGRADE_PARENT;

		err = AppUpgradeInitSw(state);
		write_debug(APP_DEBUG_L1, "AppUpgradeInitSw returned %d", err);
		if (err != SUCCESS) {
			return (AppParentProcessSliceError(slices, slice,
				*state, ERR_LOAD_INSTALLED, confirm_exit_func));
		}

		/*
		 * Get the current file system layout for
		 * the disk the user wants to upgrade.
		 */
		if (*fs_space) {
			swi_free_space_tab(*fs_space);
			*fs_space = NULL;
		}
		write_debug(APP_DEBUG_L1, "calling load_current_fs_layout");
		*fs_space = load_current_fs_layout();
		if (!*fs_space) {
			return (AppParentProcessSliceError(slices, slice,
				*state, ERR_LOAD_INSTALLED, confirm_exit_func));
		}

		/*
		 * child should continue processing in the normal
		 * parade from here
		 */
		return (ChildUpgContinue);

#ifndef _NO_FORK
	} else {
		/*
		 * This is the parent process
		 * Wait for the child process to exit.
		 */
		write_debug(APP_DEBUG_L1, "Parent Process waiting...");
		*state |= AppState_UPGRADE_PARENT;
		*state &= ~AppState_UPGRADE_CHILD;
		if (waitpid(child_pid, &child_status, 0) < 0) {
			return (ChildUpgExitFailure);
		}
		write_debug(APP_DEBUG_L1, "Parent process proceeding...");

		/*
		 * Reinitialize anything in the parent that exitting
		 * the child may have hosed.
		 * In the GUI, this means reinitializing the entire
		 * X environment...
		 */
		if (parent_reinit_func) {
			(*parent_reinit_func)(parent_reinit_data);
		}

		write_debug(APP_DEBUG_L1, "Child Exitted:");
		write_debug(APP_DEBUG_L1_NOHD,
			"child_status = %d", child_status);
		write_debug(APP_DEBUG_L1_NOHD,
			"WIFEXITED = %d", WIFEXITED(child_status));
		write_debug(APP_DEBUG_L1_NOHD,
			"WEXITSTATUS = %d", WEXITSTATUS(child_status));
		write_debug(APP_DEBUG_L1_NOHD,
			"WIFSIGNALED = %d", WIFSIGNALED(child_status));
		write_debug(APP_DEBUG_L1_NOHD,
			"WIFSTOPPED = %d", WIFSTOPPED(child_status));

		if (WIFEXITED(child_status)) {
			exit_code = (int)((char)(WEXITSTATUS(child_status)));
			if (exit_code < 0)
				return (ChildUpgExitFailure);
		} else if (WIFSIGNALED(child_status)) {
			return (ChildUpgExitFailure);
		} else if (WIFSTOPPED(child_status)) {
			return (ChildUpgExitFailure);
		}

		write_debug(APP_DEBUG_L1, "Child process exitted with %d",
			exit_code);

		return ((TChildAction)exit_code);
	}
#endif

	/* NOTREACHED */
}

/*
 * Function:	AppParentProcessSliceError
 * Description:
 *	Present a dialog telling the user that there has been an error
 *	trying to upgrade the currently selected upgradeable slice.
 *	If there are no more upgradeable slices, force them to either
 *	exit or to choose to go down the initial install path.
 * Scope:	PUBLIC
 * Parameters:
 *	UpgOs_t *slices:
 *		the upgradeable slices array.
 *	UpgOs_t *slice:
 *		the slice we're actually trying to upgrade.
 *	unsigned int *state:
 *		ptr to application state
 *	int err:
 *		the error that caused the failure
 *	void (*confirm_exit_func)(void):
 *		function to call to confirm exit requests - may be NULL
 * Return:
 * Globals:
 * Notes:
 */
static TChildAction
AppParentProcessSliceError(UpgOs_t *slices, UpgOs_t *slice, unsigned int state,
	int err, void (*confirm_exit_func)(void))
{
	char *str;
	UI_MsgStruct *msg_info;
	int done;

	SliceSetFailed(slices);

	/*
	 * present the slice failure to the user
	 */
	str = AppGetUpgradeErrorStr(err,
		slice->release,
		slice->slice);

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->msg_type = UI_MSGTYPE_WARNING;
	msg_info->msg = str;
	msg_info->btns[UI_MSGBUTTON_OK].button_text = UI_BUTTON_OK_STR;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;

	/* invoke the message */
	(void) UI_MsgFunction(msg_info);

	/* cleanup */
	UI_MsgStructFree(msg_info);
	free(str);

	/*
	 *
	 */
	if (SliceIsSystemUpgradeable(slices)) {
		/*
		 * if there are more slices left to try, then
		 * return a slice failure, so the parent knows to keep
		 * trying...
		 */
		if (state & AppState_UPGRADE_CHILD)
			exit (ChildUpgSliceFailure);
		else
			return (ChildUpgSliceFailure);
	} else {
		/*
		 * if there are NO more slices left to try, then
		 * tell the user this and return either initial path
		 * or exit...
		 *
		 * The loop here is so that if the user chooses cancel
		 * from the exit confirmation dialog,
		 * that then the initial/exit screen is popped back up.
		 */
		done = FALSE;
		while (!done) {
			if (AppUpgradeForceInitialMsg() == UI_MSGRESPONSE_OK) {
				if (state & AppState_UPGRADE_CHILD)
					exit (ChildUpgInitial);
				else
					return (ChildUpgInitial);
			} else {
				(*confirm_exit_func)();
			}
		}
	}

	/* NOTREACHED */
}

/*
 * Function:	AppParentContinueUpgrade
 * Description:
 *	Figure out how the parent upgrade process is supposed to resume
 *	processing based on the exit code from the child.
 * Scope:	PUBLIC
 * Parameters:
 *	TChildAction child_action:
 *		the child process exit code
 *	unsigned int *state:
 *		ptr to application state mask
 *	void (*exit_func)(int exit_code, void *exit_data):
 *		function for parent to use to exit the application
 * Return:
 *	parAction_t
 *		the next parade action for the parent
 * Globals:
 * Notes:
 */
parAction_t
AppParentContinueUpgrade(
	TChildAction child_action,
	unsigned int *state,
	void (*exit_func)(int exit_code, void *exit_data))
{
	int exit_data;

	switch (child_action) {
	case ChildUpgRecoverNo:
		/*
		 * go back to the Upgrade/Initial screen from the parOs
		 * processing
		 */
		return (parAGoback);
	case ChildUpgRecoverRestore:
		*state |= AppState_UPGRADE_RECOVER;
		*state |= AppState_UPGRADE_RECOVER_RESTORE;
		return (parAContinue);
	case ChildUpgRecoverUpgScript:
		*state |= AppState_UPGRADE_RECOVER;
		*state |= AppState_UPGRADE_RECOVER_UPGSCRIPT;
		return (parAContinue);
	case ChildUpgNormal:
		*state &= ~AppState_UPGRADE_DSR;
		return (parAContinue);
	case ChildUpgDsr:
		*state |= AppState_UPGRADE_DSR;
		return (parAContinue);
	case ChildUpgGoback:
		/*
		 * A Comeback here in the parent really means
		 * that the child process is requesting a
		 * goback across the parent/child boundary.
		 * This really means not a goback from parOs,
		 * but a goback to parOs (or, from the parent's
		 * perspective, a comeback to this screen
		 * again...)
		 */
		return (parAComeback);
	case ChildUpgChange:
		return (parAChange);
	case ChildUpgInitial:
		return (parAInitial);
	case ChildUpgExitOkReboot:
		if (exit_func)
			(*exit_func)(EXIT_INSTALL_SUCCESS_REBOOT,
				(void *) 1);
		else
			exit(EXIT_INSTALL_SUCCESS_REBOOT);

		/* NOTREACHED */
		break;
	case ChildUpgExitOkNoReboot:
		if (exit_func)
			(*exit_func)(EXIT_INSTALL_SUCCESS_NOREBOOT,
				(void *) 1);
		else
			exit(EXIT_INSTALL_SUCCESS_NOREBOOT);

		/* NOTREACHED */
		break;
	case ChildUpgExitSignal:
	case ChildUpgExitFailure:
	case ChildUpgUserExit:
	default:
		if (exit_func) {
			/*
			 * send data to the exit function so it knows if
			 * it is exitting due to user request or a signal
			 */
			switch (child_action) {
			case ChildUpgExitSignal:
				exit_data = 0;
				break;
			case ChildUpgExitFailure:
			default:
				exit_data = 1;
				break;
			case ChildUpgUserExit:
				exit_data = 2;
				break;
			}
			(*exit_func)(EXIT_INSTALL_FAILURE, (void *) exit_data);
		} else
			exit(EXIT_INSTALL_FAILURE);

		/* NOTREACHED */
		break;
	}

	/* NOTREACHED */
}

/*
 * Function: AppDoMountsAndSwap
 * Description:
 * Scope:	public
 * Parameters:
 *	slices: upgradeable slices array
 * Return:	[int]
 * Globals:	none
 *	0: success
 *	FAILURE: generic failure
 *	any other: failure code from mount_and_add_swap()
 * Notes:
 */
int
AppDoMountsAndSwap(UpgOs_t *slices)
{
	int err;
	UpgOs_t *slice;

	write_debug(APP_DEBUG_L1, "Entering AppDoMountsAndSwap");

	slice = SliceGetSelected(slices, NULL);
	if (!slice)
		return (FAILURE);
	err = mount_and_add_swap(slice->slice);
	write_debug(APP_DEBUG_L1, "mount_and_add_swap(%s) returned = %d",
		slice->slice, err);
	return (err);
}

/*
 * Function: AppUnmountAll
 * Description:
 *	unmount all disks and swap.
 * Scope:	PRIVATE
 * Parameters: none
 * Return:	[int]
 *	SUCCESS
 *	!SUCCESS: soft/svc lib error code
 * Globals:	none
 * Notes:
 */
int
AppUnmountAll(void)
{
	int err;

	write_debug(APP_DEBUG_L1, "Entering AppUnmountAll");

	err = umount_all();
	if (err != SUCCESS)
		return (err);

	err = unswap_all();
	if (err != SUCCESS)
		return (err);

	return (SUCCESS);
}

/*
 * Function: AppGetUpgradeProgressStr
 * Description:
 *	Get localized, descriptive strings that describe the
 *	main stage of upgrade processing we are in and the
 *	secondary information provided for that main stage.
 *	The secindary string is not localized since it is assumed
 *	that it is something like a pkg name or a filename.
 *
 * Scope:	PUBLIC
 * Parameters:
 *	val_progress:
 *		progress structure as provided by the software
 *		library space checking code.
 *	main_label:
 *		address to stick the localized main string into.
 *	detail_label:
 *		address to stick the secondary string into.
 *
 * Return:	none
 * Globals:	none
 * Notes:
 */
void
AppGetUpgradeProgressStr(ValProgress *val_progress,
	char **main_label, char **detail_label)
{
	char *main_str;
	char buf[PATH_MAX];

	/* get the main stage str */
	switch (val_progress->valp_stage) {
	case VAL_UNKNOWN:
	default:
		main_str = NULL;
		break;
	case VAL_ANALYZE_BEGIN:
		main_str = LABEL_SW_ANALYZE;
		break;
	case VAL_ANALYZE_END:
		/*
		 * the apps fill in the end string based on
		 * success vs. failure
		 */
		main_str = NULL;
		break;
	case VAL_UPG_BEGIN:
		main_str = LABEL_UPG_PROGRESS;
		break;
	case VAL_UPG_END:
		/*
		 * the apps fill in the end string based on
		 * success vs. failure
		 */
		main_str = NULL;
		break;
	case VAL_FIND_MODIFIED:
		main_str = LABEL_UPG_VAL_FIND_MODIFIED;
		break;
	case VAL_CURPKG_SPACE:
		main_str = LABEL_UPG_VAL_CURPKG_SPACE;
		break;
	case VAL_CURPATCH_SPACE:
		main_str = LABEL_UPG_VAL_CURPATCH_SPACE;
		break;
	case VAL_SPOOLPKG_SPACE:
		main_str = LABEL_UPG_VAL_SPOOLPKG_SPACE;
		break;
	case VAL_CONTENTS_SPACE:
		main_str = LABEL_UPG_VAL_CONTENTS_SPACE;
		break;
	case VAL_NEWPKG_SPACE:
		main_str = LABEL_UPG_VAL_NEWPKG_SPACE;
		break;
	case VAL_EXEC_PKGADD:
		main_str = LABEL_UPG_VAL_EXEC_PKGADD;
		break;
	case VAL_EXEC_PKGRM:
		main_str = LABEL_UPG_VAL_EXEC_PKGRM;
		break;
	case VAL_EXEC_REMOVEF:
		main_str = LABEL_UPG_VAL_EXEC_REMOVEF;
		break;
	case VAL_EXEC_SPOOL:
		main_str = LABEL_UPG_VAL_EXEC_SPOOL;
		break;
	case VAL_EXEC_RMTEMPLATE:
		main_str = LABEL_UPG_VAL_EXEC_RMTEMPLATE;
		break;
	case VAL_EXEC_RMDIR:
		main_str = LABEL_UPG_VAL_EXEC_RMDIR;
		break;
	case VAL_EXEC_RMSVC:
		main_str = LABEL_UPG_VAL_EXEC_RMSVC;
		break;
	case VAL_EXEC_RMPATCH:
		main_str = LABEL_UPG_VAL_EXEC_RMPATCH;
		break;
	case VAL_EXEC_RMTEMPLATEDIR:
		main_str = LABEL_UPG_VAL_EXEC_RMTEMPLATEDIR;
		break;
	}

	/* no main stage ==> no detail stage either */
	if (!main_str) {
		*main_label = NULL;
		*detail_label = NULL;
		return;
	}

	/*
	 * If there is a secondary data string, then
	 * the main string looks like:
	 *	"main str:"
	 * If there is no secondary data string, then
	 * the main string looks like:
	 *	"main str..."
	 */
	if (val_progress->valp_detail) {
		(void) sprintf(buf, "%s:", main_str);
		*main_label = xstrdup(buf);
		*detail_label = xstrdup(val_progress->valp_detail);
		UI_ProgressBarTrimDetailLabel(
			*main_label,
			*detail_label,
			APP_UI_UPG_PROGRESS_STR_LEN);
	} else {
		(void) sprintf(buf, "%s...", main_str);
		*main_label = xstrdup(buf);
		*detail_label = NULL;
	}

	write_debug(APP_DEBUG_L1, "Upgrade Progress:");
	write_debug(APP_DEBUG_NOHD, LEVEL2, "%s %s",
		*main_label,
		*detail_label ? *detail_label : "");
	write_debug(APP_DEBUG_NOHD, LEVEL2, "percent complete: %d",
		val_progress->valp_percent_done);
}

/*
 * Function:	AppGetUpgradeErrorStr
 * Description:
 *	Get an error string to present to the user about this
 *	upgradeable slice upgrade error.
 * Scope:	PUBLIC
 * Parameters:
 *	int error_code:
 *		the error that occured
 *	char *release:
 *		the release name (e.g. Solaris 2.6) for this currently
 *		selected upgradeable slice.
 *	char *slice:
 *		the slice name (e.g. c0t0d0s0) for this currently
 *		selected upgradeable slice.
 * Return:
 *	the error string - which is dynamically allocated.
 * Globals:
 * Notes:
 */
char *
AppGetUpgradeErrorStr(int error_code, char *release, char *slice)
{
	char *str;
	char *msg;

	if (!release || !slice)
		return (NULL);

	switch (error_code) {
	case ERR_OPEN_VFSTAB:
	case ERR_OPENING_VFSTAB:
		msg = APP_MSG_VFSTAB_OPEN_FAILED;
		break;
	case ERR_ADD_SWAP:
		msg = APP_MSG_ADD_SWAP_FAILED;
		break;
	case ERR_MOUNT_FAIL:
		msg = APP_MSG_MOUNT_FAILED;
		break;
	case ERR_MUST_MANUAL_FSCK:
	case ERR_FSCK_FAILURE:
		msg = APP_MSG_FSCK_FAILED;
		break;
	case ERR_DELETE_SWAP:
		msg = APP_MSG_DELETE_SWAP_FAILED;
		break;
	case ERR_UMOUNT_FAIL:
		msg = APP_MSG_UMOUNT_FAILED;
		break;
	case ERR_LOAD_INSTALLED:
	default:
		msg = APP_MSG_LOAD_INSTALLED;
		break;
	}

	str = (char *) xmalloc(
		strlen(APP_ER_SLICE_CANT_UPGRADE) +
		strlen(msg) +
		strlen(release) +
		strlen(slice) +
		1);
	(void) sprintf(str, APP_ER_SLICE_CANT_UPGRADE,
		release,
		slice,
		msg);

	return (str);
}

/*
 * Function:	AppUpgradeForceInitialMsg
 * Description:
 *	Put up the message to tell the user they must either choose to
 *	do an initial install or exit the app and record nd return their
 *	response..
 * Scope:	PUBLIC
 * Parameters: none
 * Return:
 *	UI_MsgResponseType:
 *		UI_MSGRESPONSE_OK: initial
 *		UI_MSGRESPONSE_CANCEL: exit
 * Globals:
 * Notes:
 */
static UI_MsgResponseType
AppUpgradeForceInitialMsg(void)
{

	UI_MsgResponseType response;
	UI_MsgStruct *msg_info;

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->msg_type = UI_MSGTYPE_WARNING;
	msg_info->msg = APP_ER_FORCE_INSTALL;
	msg_info->btns[UI_MSGBUTTON_OK].button_text = LABEL_INITIAL_BUTTON;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = UI_BUTTON_EXIT_STR;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;

	/* invoke the message */
	response = UI_MsgFunction(msg_info);

	/* cleanup */
	UI_MsgStructFree(msg_info);

	return (response);
}

/*
 * Function:	AppUpgradeGetProgressBarInfo
 * Description:
 *	Get information about where in the progress bar a certain
 *	phase should start displaying and how much of the progress bar it
 *	should use.
 * Scope:	PUBLIC
 * Parameters:
 *	int index:
 *		index which indicates which phase we're in
 *	int state:
 *		application state mask
 *	int *start:
 *		ptr in which to return where in the progress bar to start
 *	float *factor:
 *		ptr in which to return much of the progress bar to use
 * Return:
 * Globals:
 * Notes:
 *	In the case of figuring out where to start the ugprade script
 *	progress display in recovery mode:
 *	Preferably we would restart the
 *	display at 50 with factor .50 if it is recovering in DSR mode, and at
 *	0/1.0 in regular upgrade mode.  However, upon a restart, we no longer
 *	have information about which type of ugprade it was, so we always start
 *	at 0/1.0.  The service library would have to figure out which mode of
 *	ugprade we are starting fom in order for us to change this.
 */
void
AppUpgradeGetProgressBarInfo(int index, int state, int *start, float *factor)
{
	/* defaults */
	*start = 0;
	*factor = 1.0;

	switch (index) {
	case PROGBAR_ALBACKUP_INDEX:
		*start = 0;
		*factor = .25;
		break;
	case PROGBAR_ALRESTORE_INDEX:
		if (state & AppState_UPGRADE_DSR) {
			*start = 25;
			*factor = .25;
		} else if (state & AppState_UPGRADE_RECOVER_RESTORE) {
			*start = 25;
			*factor = .25;
		}
		break;
	case PROGBAR_UPGRADE_INDEX:
		if (state & AppState_UPGRADE_DSR) {
			*start = 50;
			*factor = .50;
		} else if (state & AppState_UPGRADE_RECOVER_RESTORE) {
			*start = 50;
			*factor = .50;
		} else if (state & AppState_UPGRADE_RECOVER_UPGSCRIPT) {
			*start = 0;
			*factor = 1.0;
		}
		break;
	}
}
