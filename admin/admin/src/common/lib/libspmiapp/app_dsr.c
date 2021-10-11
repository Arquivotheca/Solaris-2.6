#ifndef lint
#pragma ident "@(#)app_dsr.c 1.40 96/10/11 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	app_dsr.c
 * Group:	libspmiapp
 * Description:
 *	Space Adapting Upgrade app level code.
 */
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>

#include "spmiapp_api.h"
#include "app_utils.h"

#define	DSR_SL_HAS_SPACE_ENTRY(slentry) \
	((slentry) && ((slentry)->Space))

static int _DsrSLDefaultAutoLayout(TList slhandle, FSspace **fs_space);
static int _DsrSLAutoLayout(TList slhandle);
static int _DsrSLAutoLayoutDiskSetup(TList slhandle);
static int _DsrSLSetInstanceNumber(TList slhandle, TSLEntry *curr_entry,
	int num_swap_in_vfstab);
static int _DsrSLCopy(TList *slhandle_copy, TList slhandle);
static int _DsrSLMountPointInVFSTab(TList slhandle, char *mount_point);
static void _DsrSLUIPrintEntry(TSLEntry *slentry);

/*
 * Function: DsrFSAnalyzeSystem
 * Description:
 *	Analyze the current system based on the currently installed
 *	system and the software that has been requested to be installed.
 *	Prior to this mount_and_add_swap and load_current_fs_layout
 *	must have been called.
 *
 * Scope:	PUBLIC
 * Parameters:
 *	num_failed	[*RW] (int*):
 *		ptr in which to return the number of failed file systems.
 *		0 if analyze was successful.
 * Return:
 *	SP_ERR_NOT_ENOUGH_SPACE: analyze failed due to lack of space.
 *	SUCCESS: analyze was successful.
 * Globals:
 * Notes:
 */
int
DsrFSAnalyzeSystem(FSspace **fs_space, int *num_failed,
	TCallback callback_func, void *user_data)
{
	int err;

	/*
	 * Do the actual file system check and
	 * return how many failed.
	 */
	set_action_code_mode(PRESERVE_IDENTICAL_PACKAGES);
	err = verify_fs_layout(fs_space, callback_func, user_data);
	if (err == SP_ERR_NOT_ENOUGH_SPACE) {
		/* failure due to not enough space - how many failed? */
		if (num_failed)
			*num_failed = DsrFSGetNumFailed(fs_space);
		set_action_code_mode(REPLACE_IDENTICAL_PACKAGES);
		return (err);
	} else {
		/* successful */
		if (num_failed)
			*num_failed = 0;
		return (err);
	}
}

/*
 * Function:	DsrFSGetNumFailed
 * Description:
 *	Find out how many file systems there are in the current
 *	sw library space table have failed the verify analyze call
 *	due to lack of space.
 * Scope:	PUBLIC
 * Parameters:
 *	fs_space:	[RO] (FSspace **)
 *		SW library space table.
 * Return:
 *	int	- the number of failed file systems in the space table
 * Globals:	none
 * Notes:
 */
int
DsrFSGetNumFailed(FSspace **fs_space)
{
	int i;
	int num_failed = 0;

	for (i = 0; fs_space && fs_space[i]; i++) {
		if (fs_space[i]->fsp_flags & FS_INSUFFICIENT_SPACE) {
			write_debug(APP_DEBUG_L1, "failed file system: %s",
				fs_space[i]->fsp_mntpnt);
			num_failed++;
		}
	}

	write_debug(APP_DEBUG_L1, "number of failed file systems: %d",
		num_failed);
	return (num_failed);
}

/*
 * Function:	DsrSLAutoLayout
 * Description:
 *	Perform autolayout.
 *
 *	General disk library assumptions:
 *	- instance numbers have been assigned (they get assigned during
 *		DsrSLCreate()
 *	- after instance numbers are assigned, the current disk
 *		configuration is committed (the interactive apps MUST
 *		have the orignal data + the instance numbers available
 *		to them after auto-layout is done.)
 *	- To perform autolayout:
 *		- null out current disk configuration
 *		- create resource list
 *		- add any slice object data to the current disk list
 *			e.g. non-vfstab, fixed slices
 *		- run autolayout
 *		- results of autolayout are in current disk
 *			configuration upon successful autolayout run
 *
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 *	fs_space:	[RO] (FSspace **)
 *		SW library space table.
 *	default_layout:	[RO] (int)
 *		0:	perform autolayout based on the values provided in
 *			the slice list.
 *		!0:	perform autolayout based on a set of default
 *			assumptions (i.e. do an automatic auto-layout).
 *
 * Return:
 *	SUCCESS:
 *	FAILURE:
 * Globals: works on global disk list
 * Notes:
 */
int
DsrSLAutoLayout(TList slhandle, FSspace **fs_space, int default_layout)
{
	if (default_layout) {
		return (_DsrSLDefaultAutoLayout(slhandle, fs_space));
	} else {
		return (_DsrSLAutoLayout(slhandle));
	}
}

/*
 * Function:	_DsrSLDefaultAutoLayout
 * Description:
 *	Perform an autolayout by applying a set of default rules.
 *	The default rules are:
 *		- all non-vfstab slices are fixed
 *			with final size == original size.
 *		- all failed vfstab slices are changeable with
 *			final size == required size.
 *		- all vfstab slices co-resident with a failed slice
 *			are changeable with
 *			final size == required size.
 *		- all other vfstab slices are fixed.
 *
 * Scope:	PRIVATE
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 *	fs_space:	[RO] (FSspace **)
 *		SW library space table.
 * Return:
 *	SUCCESS
 *	FAILURE
 * Globals:
 * Notes:
 */
static int
_DsrSLDefaultAutoLayout(TList slhandle, FSspace **fs_space)
{
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;

	/*
	 * set up the slice list with our default set of constraints
	 * from which we will try to autolayout automatically for the
	 * user.
	 */

	if (DsrSLSetDefaults(slhandle)) {
		return (FAILURE);
	}

	LL_WALK(slhandle, slcurrent, slentry, err) {

		/*
		 * Ok, for every slice contained in the vfstab
		 */

		if (slentry->InVFSTab) {

			/*
			 * If the slice is coresident with the failed slice
			 */

			if (DsrIsDeviceCoResident(fs_space,
			    slentry->SliceName)) {

				slentry->State = SLChangeable;
				DsrSLEntryGetAttr(slentry,
				    DsrSLAttrReqdSize,
				    &slentry->Size,
				    NULL);

				/*
				 * All swap slices other than
				 * instance zero are set to movable
				 */

				if (slentry->FSType == SLSwap) {
					if (slentry->MountPointInstance != 0) {
						slentry->State = SLMoveable;
					} else {
						slentry->Size = 0;
					}
				}
			}
		}
	}

	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}

	/* strictly for debugging */
	if (getenv("SYS_DSR_AUTO_AUTOLAYOUT_FAIL")) {
		(void) _DsrSLAutoLayout(slhandle);
		return (FAILURE);
	}

	/*
	 * If tracing is enabled
	 */

	if (get_trace_level() > 2) {
		write_status(LOGSCR, LEVEL0,
			"Slice List after co-resident filtering:");
		SLPrint(slhandle, 0);
	}

	/*
	 * do the autolayout
	 */
	return (_DsrSLAutoLayout(slhandle));
}

/*
 * Function: _DsrSLAutoLayout
 * Description:
 *	Perform an autolayout based on whatever is in the slice list
 *	at this point.
 * Scope:	PRIVATE
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 * Return:
 *	SUCCESS
 *	FAILURE
 * Globals:
 * Notes:
 */
static int
_DsrSLAutoLayout(TList slhandle)
{
	int status;
	Disk_t *dp;

	/* strictly for debugging */
	if (getenv("SYS_DSR_AUTOLAYOUT_OK")) {
		return (SUCCESS);
	}

	/*
	 * set up resource list for autolayout
	 * based on the current slice list settings
	 */
	if (_DsrSLAutoLayoutDiskSetup(slhandle)) {
		return (FAILURE);
	}

	/*
	 * Call the auto-layout function
	 */

	status =  SdiskobjAutolayout();

	if (status != SUCCESS) {
		return (FAILURE);
	}

	if (get_trace_level() > 2) {
		write_status(LOGSCR, LEVEL0, "Disk List after Autolayout:");
		WALK_DISK_LIST(dp) {
			print_disk(dp, NULL);
		}
	}

	/*
	 * Write out the disk list to file
	 */

	WriteDiskList();
	return (SUCCESS);
}

/*
 * Function:	_DsrSLAutoLayoutDiskSetup
 * Description:
 *	Set up resource list and disk list/slice objects for autolayout
 *	based on the current slice list settings.
 * Scope:	PRIVATE
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 * Return:
 * 	SUCCESS:
 *	FAILURE:
 * Globals:
 * Notes:
 */
static int
_DsrSLAutoLayoutDiskSetup(TList slhandle)
{
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;
	ResobjHandle	reshandle;
	SliceKey *slice_key;
	int SliceNumber;
	Disk_t *dp;

	/*
	 * always re-initialize the resource object list
	 * so we can populate it anew each time.
	 */
	(void) ResobjReinitList();

	/*
	 * Loop through all of the default resources in the
	 * list and set them as dependent resources.
	 */

	WALK_RESOURCE_LIST(reshandle, RESTYPE_UNDEFINED) {
		/*
		 * This will fail on the default root slice
		 * and on swap, which is ok...
		 */
		ResobjSetAttribute(reshandle,
		    RESOBJ_STATUS,
		    RESSTAT_DEPENDENT,
		    NULL);
	}

	/*
	 * Start out with a empty disk list and populate below
	 * as necessary.
	 */
	DiskNullAll();

	/*
	 * Walk the slice list and set all the appropriate
	 * resources in the resource list.
	 */
	LL_WALK(slhandle, slcurrent, slentry, err) {

		/*
		 * Handle the non-vfstab entries here.
		 *
		 * Available slices don't need anything more done -
		 * by not putting them in the resource list or setting
		 * up a slice for them, they will just get used as
		 * available space.
		 *
		 * Fixed slices must be marked via the slice interface
		 * to be ignored.
		 */
		if (!slentry->InVFSTab) {
			switch (slentry->State) {
			case SLAvailable:
				/* do nothing */
				break;
			case SLFixed:
				/*
				 * Set the Fixed slices in the DiskObject
				 * list so that they will be seen
				 * as used space by autolayout.
				 */

				SliceNumber = DsrGetSliceNumFromDeviceName(
					slentry->SliceName);
				if (SliceNumber == -1) {
					return (FAILURE);
				}

				dp = find_disk(slentry->SliceName);

				/*
				 * Mark the disk as selected
				 */

				select_disk(dp, NULL);

				if (SliceobjSetAttribute(dp,
				    SliceNumber,
				    SLICEOBJ_START,
				    Sliceobj_Start(
					CFG_EXIST, dp, SliceNumber),
				    SLICEOBJ_SIZE,
				    Sliceobj_Size(
					CFG_EXIST, dp, SliceNumber),
				    SLICEOBJ_USE,
				    Sliceobj_Use(
					CFG_EXIST, dp, SliceNumber),
				    SLICEOBJ_INSTANCE,
				    Sliceobj_Instance(
					CFG_COMMIT, dp, SliceNumber),
				    SLICEOBJ_IGNORED, TRUE,
				    NULL)) {
					write_debug(APP_DEBUG_L1,
						"SliceobjSetAttribute failed");
					return (FAILURE);
				}
				break;
			default:
				/* should never happen! */
				return (FAILURE);
			}

		/*
		 * OK, handle the vfstab entries here.
		 */

		} else {

			/*
			 * Collapsed entries are just available space, so ignore
			 * it just like we do non-vfstab available entries.
			 */
			if (slentry->State == SLCollapse) {
				continue;
			}

			/*
			 * Find the resource object associated
			 * with this slice entry.  The ones
			 * we find here are any of the resource
			 * objects in the default resource list.
			 * (e.g. /usr, /var, /opt ...)
			 */
			reshandle = ResobjFind(
				slentry->MountPoint,
				slentry->MountPointInstance);

			/*
			 * Make a resource entry for this
			 * slice entry if we didn't find one
			 * in the list already.
			 */
			if (!reshandle) {
				reshandle = ResobjCreate(
					slentry->FSType == SLSwap ?
						RESTYPE_SWAP :
						RESTYPE_DIRECTORY,
					slentry->MountPoint,
					slentry->MountPointInstance,
					NULL);
			}

			/*
			 * We should always have a resource object
			 * by now.
			 * If not, we're screwed.
			 */
			if (!reshandle)
				return (FAILURE);

			/*
			 * get a pointer to this resource
			 * object's original disk configuration
			 * (which is stored in the committed
			 * state at this point).
			 * SliceobjFindUse should never return
			 * NULL at this point.
			 */
			slice_key = SliceobjFindUse(CFG_COMMIT, NULL,
				slentry->MountPoint,
				slentry->MountPointInstance,
				1);

			/* punt... */
			if (!slice_key)
				return (FAILURE);

			/*
			 * Set up the resource object
			 * attributes depending on the
			 * state of the slice entry.
			 */
			switch (slentry->State) {
			case SLFixed:
				if (ResobjSetAttribute(reshandle,
				    RESOBJ_STATUS, RESSTAT_INDEPENDENT,
				    NULL) != D_OK)
					return (FAILURE);

				/*
				 * Set the fixed file systems from the
				 * vfstab into the disk object list as fixed.
				 */

				SliceNumber = DsrGetSliceNumFromDeviceName(
					slentry->SliceName);
				if (SliceNumber == -1) {
					return (FAILURE);
				}

				/*
				 * Get the handle to the disk
				 */

				dp = find_disk(slentry->SliceName);

				/*
				 * Mark the disk as selected
				 */

				select_disk(dp, NULL);

				if (slentry->FSType == SLSwap) {
					if (SliceobjSetAttribute(dp,
					    SliceNumber,
					    SLICEOBJ_START,
					    Sliceobj_Start(
						CFG_EXIST, dp, SliceNumber),
					    SLICEOBJ_SIZE,
					    Sliceobj_Size(
						CFG_EXIST, dp, SliceNumber),
					    SLICEOBJ_USE,
					    Sliceobj_Use(
						CFG_COMMIT, dp, SliceNumber),
					    SLICEOBJ_INSTANCE,
					    Sliceobj_Instance(
						CFG_COMMIT, dp, SliceNumber),
					    SLICEOBJ_STUCK, TRUE,
					    SLICEOBJ_EXPLICIT, TRUE,
					    NULL)) {
						return (FAILURE);
					}
				} else {
					if (SliceobjSetAttribute(dp,
					    SliceNumber,
					    SLICEOBJ_START,
					    Sliceobj_Start(
						CFG_EXIST, dp, SliceNumber),
					    SLICEOBJ_SIZE,
					    Sliceobj_Size(
						CFG_EXIST, dp, SliceNumber),
					    SLICEOBJ_USE,
					    Sliceobj_Use(
						CFG_COMMIT, dp, SliceNumber),
					    SLICEOBJ_INSTANCE,
					    Sliceobj_Instance(
						CFG_COMMIT, dp, SliceNumber),
					    SLICEOBJ_PRESERVED, TRUE,
					    NULL)) {
						return (FAILURE);
					}

					if (streq(slentry->MountPoint, ROOT)) {

						/*
						 * Set the explicit boot disk so
						 * that the root partition will
						 * not be moved to another disk.
						 */

						if (BootobjSetAttribute(
						    CFG_CURRENT,
						    BOOTOBJ_DISK,
						    disk_name(slice_key->dp),
						    BOOTOBJ_DISK_EXPLICIT, TRUE,
						    NULL) != D_OK) {
							return (FAILURE);
						}

						/*
						 * For a sparc architecture then
						 * go ahead and set the device
						 * explicitely as well.
						 */

						if (IsIsa("sparc")) {
							if (BootobjSetAttribute(
							    CFG_CURRENT,
							    BOOTOBJ_DEVICE,
							    slice_key->slice,
							    BOOTOBJ_DEVICE_EXPLICIT,
							    TRUE,
							    NULL) != D_OK) {
								return
								    (FAILURE);
							}
						}
					}
				}
				break;
			case SLMoveable:
				if (ResobjSetAttribute(reshandle,
				    RESOBJ_STATUS, RESSTAT_INDEPENDENT,
				    RESOBJ_DEV_PREFDISK,
					disk_name(slice_key->dp),
				    RESOBJ_DEV_PREFDEVICE,
					slice_key->slice,
				    RESOBJ_DEV_EXPLSIZE,
					Sliceobj_Size(CFG_COMMIT,
						slice_key->dp,
						slice_key->slice),
				    NULL) != D_OK)
					return (FAILURE);
				break;
			case SLChangeable:
				/*
				 * Changed normally implies a
				 * fully moveable slice, but /
				 * has a fixed disk/slice since
				 * DSR is not handling moving the
				 * boot object.
				 */
				if (streq(slentry->MountPoint, ROOT)) {

					/*
					 * Set the explicit boot disk
					 * so that the root partition
					 * will not be moved to
					 * another disk.
					 */

					if (BootobjSetAttribute(
					    CFG_CURRENT,
					    BOOTOBJ_DISK,
					    disk_name(slice_key->dp),
					    BOOTOBJ_DISK_EXPLICIT, TRUE,
					    NULL) != D_OK) {
						return (FAILURE);
					}

					/*
					 * For a sparc architecture then
					 * go ahead and set the device
					 * explicitly as well.
					 */

					if (IsIsa("sparc")) {
						if (BootobjSetAttribute(
						    CFG_CURRENT,
						    BOOTOBJ_DEVICE,
						    slice_key->slice,
						    BOOTOBJ_DEVICE_EXPLICIT,
						    TRUE,
						    NULL) != D_OK) {
							return (FAILURE);
						}
					}

					if (ResobjSetAttribute(
					    reshandle,
					    RESOBJ_DEV_EXPLDISK,
					    disk_name(slice_key->dp),
					    RESOBJ_DEV_EXPLDEVICE,
					    slice_key->slice,
					    NULL) != D_OK)
						return (FAILURE);
				} else if (slentry->FSType == SLSwap &&
					slentry->MountPointInstance != 0) {
					if (ResobjSetAttribute(
						reshandle,
						RESOBJ_DEV_EXPLSIZE,
						kb_to_sectors(
							slentry->Size),
						NULL) != D_OK) {
							return (FAILURE);
					}
				} else {
					if (ResobjSetAttribute(
					    reshandle,
					    RESOBJ_DEV_PREFDISK,
					    disk_name(slice_key->dp),
					    RESOBJ_DEV_PREFDEVICE,
					    slice_key->slice,
					    RESOBJ_STATUS,
					    RESSTAT_INDEPENDENT,
					    NULL) != D_OK)
						return (FAILURE);
				}

				if (ResobjSetAttribute(reshandle,
				    RESOBJ_DEV_EXPLMINIMUM,
				    kb_to_sectors(slentry->Size),
				    NULL) != D_OK) {
					return (FAILURE);
				}
				break;
			default:
				/* should never happen! */
				return (FAILURE);
			}
		}
	}

	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}

	/*
	 * If tracing is enabled
	 */

	if (get_trace_level() > 2) {

		/*
		 * Dump the created resource list
		 */

		write_status(LOGSCR, LEVEL0, "Resource List after creation:");
		ResobjPrintList();

		/*
		 * Dump the created disk list
		 */

		write_status(LOGSCR, LEVEL0, "Disk List after creation:");
		WALK_DISK_LIST(dp) {
			print_disk(dp, NULL);
		}
	}

	return (SUCCESS);
}

/*
 * Function:	DsrSLStateStr
 * Description:
 *	Get a string representation of the given slice list state.
 * Scope:	PUBLIC
 * Parameters:
 *	state:	[RO]: (TSLState)
 * Return:
 *	string representation of state - points to static string space
 * Globals:	none
 * Notes:
 */
char *
DsrSLStateStr(TSLState state)
{
	char *str;

	switch (state) {
	case SLChangeable:
		str = LABEL_DSR_FSREDIST_CHANGE;
		break;
	case SLFixed:
		str = LABEL_DSR_FSREDIST_FIXED;
		break;
	case SLMoveable:
		str = LABEL_DSR_FSREDIST_MOVE;
		break;
	case SLAvailable:
		str = LABEL_DSR_FSREDIST_AVAILABLE;
		break;
	case SLCollapse:
		str = LABEL_DSR_FSREDIST_COLLAPSED;
		break;
	default:
		str = NULL;
		break;
	}

	return (str);
}

/*
 * Function:	DsrALMediaTypeStr
 * Description:
 *	Get a string representation of the given media type.
 * Scope:	PUBLIC
 * Parameters:
 *	TDSRALMedia media_type
 * Return:
 *	string representation of media type - points to static string space
 * Globals:
 * Notes:
 */
char *
DsrALMediaTypeStr(TDSRALMedia media_type)
{
	char *str;

	switch (media_type) {
	case  DSRALFloppy:
		str = LABEL_DSR_MEDIA_OPT_LFLOPPY;
		break;
	case DSRALTape:
		str = LABEL_DSR_MEDIA_OPT_LTAPE;
		break;
	case DSRALDisk:
		str = LABEL_DSR_MEDIA_OPT_LDISK;
		break;
	case DSRALNFS:
		str = LABEL_DSR_MEDIA_OPT_NFS;
		break;
	case DSRALRsh:
		str = LABEL_DSR_MEDIA_OPT_RSH;
		break;
	default:
		str = NULL;
		break;
	}

	return (str);
}

/*
 * Function:	DsrSLFilterTypeStr
 * Description:
 *	Get a string representation of the given filter type.
 * Scope:	PUBLIC
 * Parameters:
 *	TSLFilter filter_type
 * Return:
 *	string representation of filter type - points to static string space
 * Globals:
 * Notes:
 */
char *
DsrSLFilterTypeStr(TSLFilter filter_type)
{
	char *str;

	switch (filter_type) {
	case SLFilterAll:
		str = LABEL_DSR_FSREDIST_FILTER_ALL;
		break;
	case SLFilterFailed:
		str = LABEL_DSR_FSREDIST_FILTER_FAILED;
		break;
	case SLFilterVfstabSlices:
		str = LABEL_DSR_FSREDIST_FILTER_VFSTAB;
		break;
	case SLFilterNonVfstabSlices:
		str = LABEL_DSR_FSREDIST_FILTER_NONVFSTAB;
		break;
	case SLFilterSliceNameSearch:
		str = LABEL_DSR_FSREDIST_FILTER_SLICE;
		break;
	case SLFilterMountPntNameSearch:
		str = LABEL_DSR_FSREDIST_FILTER_MNTPNT;
		break;
	default:
		str = NULL;
		break;
	}

	return (str);
}

/*
 * Function:	DsrALMediaTypeDeviceStr
 * Description:
 *	Get a string representation of the label we will use when
 *	prompting for the various media types.
 * Scope:	PUBLIC
 * Parameters:
 *	TDSRALMedia media_type
 * Return:
 *	string representation of media device label - points to
 *	static string space
 * Globals:
 * Notes:
 */
char *
DsrALMediaTypeDeviceStr(TDSRALMedia media_type)
{
	char *str;

	switch (media_type) {
	case DSRALFloppy:
	case DSRALTape:
	case DSRALDisk:
	case DSRALNFS:
	case DSRALRsh:
		str = LABEL_DSR_MEDIA_PATH;
		break;
	default:
		str = NULL;
		break;
	}
	return (str);
}

/*
 * Function:	DsrALMediaTypeEgStr
 * Description:
 *	Get a string representation of the example media path that
 *	corresponds to this media type.
 * Scope:	PUBLIC
 * Parameters:
 *	TDSRALMedia media_type
 * Return:
 *	string representation of example media path -
 *	points to static string space
 * Globals:
 * Notes:
 */
char *
DsrALMediaTypeEgStr(TDSRALMedia media_type)
{
	char *str;

	switch (media_type) {
	case  DSRALFloppy:
		str = LABEL_DSR_MEDIA_DEV_LFLOPPY;
		break;
	case DSRALTape:
		str = LABEL_DSR_MEDIA_DEV_LTAPE;
		break;
	case DSRALDisk:
		str = LABEL_DSR_MEDIA_DEV_LDISK;
		break;
	case DSRALNFS:
		str = LABEL_DSR_MEDIA_DEV_NFS;
		break;
	case DSRALRsh:
		str = LABEL_DSR_MEDIA_DEV_RSH;
		break;
	default:
		str = NULL;
		break;
	}
	return (str);
}

/*
 * Function:	DsrSLPrint
 * Description:
 *	Debugging routine used to print out a slice list.
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RO] (TList)
 *		slice list (linked list) handle
 *	file:		[RO] (char *)
 *		the file this print routine was called from.
 *	line_num:		[RO] (int)
 *		the line number this print routine was called from.
 * Return: none
 * Globals:
 * Notes:
 */
void
DsrSLPrint(TList slhandle, char *file, int line_num)
{
	DsrSLListExtraData *LLextra;

	write_debug(APP_DEBUG_L1,
		"Printing Dsr Slice list from \"%s\", line %d",
		file, line_num);

	/* print the UI data stored at the list level */
	(void) LLGetSuppliedListData(slhandle, NULL, (TLLData *)&LLextra);

	/*
	 * If there is list level data
	 */

	if (LLextra) {
		write_debug(APP_DEBUG_L1_NOHD, "List level user data:");
		write_debug(APP_DEBUG_NOHD, LEVEL2, "filter_type = %d",
			LLextra->filter_type);
		write_debug(APP_DEBUG_NOHD, LEVEL2, "filter_pattern = %s",
			LLextra->filter_pattern ? LLextra->filter_pattern : "");
		write_debug(APP_DEBUG_NOHD, LEVEL2, "history.filter_type = %d",
			LLextra->history.filter_type);
		write_debug(APP_DEBUG_NOHD, LEVEL2,
			"history.filter_pattern = %s",
			LLextra->history.filter_pattern ?
			LLextra->history.filter_pattern : "");
		write_debug(APP_DEBUG_NOHD, LEVEL2, "media type = %s",
			DsrALMediaTypeStr(LLextra->history.media_type));
		write_debug(APP_DEBUG_NOHD, LEVEL2, "media_device = %s",
			LLextra->history.media_device ?
			LLextra->history.media_device : "");
	}

	/* print the list entries & their UI extra data */
	(void) SLPrint(slhandle, _DsrSLUIPrintEntry);
}

/*
 * Function:	_DsrSLUIPrintEntry
 * Description:
 *	debug routine used to print out the UI extra application data
 *	stored with each slice list entry
 * Scope:	PRIVATE
 * Parameters:
 *	TSLEntry *slentry
 * Return:	none
 * Globals:
 * Notes:
 */
static void
_DsrSLUIPrintEntry(TSLEntry *slentry)
{
	DsrSLEntryExtraData *SLEntryextra;
	SliceKey *slice_key;

	write_debug(APP_DEBUG_NOHD, LEVEL1, "UI extra data:");

	write_debug(APP_DEBUG_NOHD, LEVEL2,
		"size = %lu KB (%lu MB)",
		slentry->Size, kb_to_mb(slentry->Size));

	/* slice list entry user data */
	slice_key = SliceobjFindUse(CFG_COMMIT, NULL,
		slentry->MountPoint,
		slentry->MountPointInstance,
		1);
	if (slice_key) {
		write_debug(APP_DEBUG_NOHD, LEVEL2, "Existing Disk:");
		write_debug(APP_DEBUG_NOHD, LEVEL3, "Disk name = %s",
			disk_name(slice_key->dp) ?
				disk_name(slice_key->dp) : "");
		write_debug(APP_DEBUG_NOHD, LEVEL3,
			"Disk slice_num = %d",
			slice_key->slice);
	} else {
		write_debug(APP_DEBUG_NOHD, LEVEL2,
			"Existing Disk: none");
	}

	slice_key = SliceobjFindUse(CFG_CURRENT, NULL,
		slentry->MountPoint,
		slentry->MountPointInstance,
		1);
	if (slice_key) {
		write_debug(APP_DEBUG_NOHD, LEVEL2, "Current Disk:");
		write_debug(APP_DEBUG_NOHD, LEVEL3, "Disk name = %s",
			disk_name(slice_key->dp) ?
				disk_name(slice_key->dp) : "");
		write_debug(APP_DEBUG_NOHD, LEVEL3,
			"Disk slice_num = %d",
			slice_key->slice);
	} else {
		write_debug(APP_DEBUG_NOHD, LEVEL2,
			"Current Disk: none");
	}

	write_debug(APP_DEBUG_NOHD, LEVEL2, "FSspace ptr = 0x%x",
		slentry->Space);
	write_debug(APP_DEBUG_NOHD, LEVEL2, "FSspace mount point = %s",
		slentry->Space ?
		slentry->Space->fsp_mntpnt : "");

	SLEntryextra = (DsrSLEntryExtraData *)slentry->Extra;
	if (!SLEntryextra)
		return;

	write_debug(APP_DEBUG_NOHD, LEVEL2, "User final_size = %s",
		SLEntryextra->history.final_size ?
		SLEntryextra->history.final_size : "");

	write_debug(APP_DEBUG_NOHD, LEVEL2, "in_filter = %d",
		SLEntryextra->in_filter);

	write_debug(APP_DEBUG_NOHD, LEVEL2, "UI 2nd level extra ptr = 0x%x",
		SLEntryextra->extra);
}

/*
 * Function:	DsrSLCreate
 * Description:
 *	Create a slice list based on the current system configuration.
 *	Upon creation, the slice list will have instance numbers
 *	assigned (into the committed state of the disks), the slice list
 *	will be sorted by ascending slice name, and the slice list will be
 *	initialized with all the default values (see DsrSLSetDefaults()).
 *
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RO] (TList)
 *		slice list (linked list) handle
 *	list_data:	[RO] (TLLData)
 *		list level data to attach to the slice list when it's
 *		created.
 *	fs_space:	[RO] (FSspace **)
 *		SW library space table.
 * Return:
 *	SUCCESS:
 *	FAILURE:
 *
 * Globals:
 * Notes:
 */
int
DsrSLCreate(TList *slice_list, TLLData list_data, FSspace **fs_space)
{
	TList slhandle;
	TSLEntry *slentry;
	Disk_t *dp;
	int i;
	char slicebuf[PATH_MAX];
	TSLState SliceState;

	if (!slice_list)
		return (FAILURE);

	(void) LLCreateList(slice_list, (TLLData) list_data);
	slhandle = *slice_list;
	if (!slhandle)
		return (FAILURE);

	/*
	 * restore the current disk state from the original in case
	 * we've nulled out current previously (may happen upon goback
	 * in the interactive apps if we've already run autolayout
	 * and have come back to here again).
	 */
	DiskSelectAll(TRUE);
	DiskRestoreAll(CFG_EXIST);

	/*
	 * make an entry in the slice list for each entry in the
	 * space array.
	 * Note that swap does NOT get added here.
	 * It gets added when we walk the disk list later on.
	 * These are all in the vfstab.
	 *
	 * Note that state and size get set here since some
	 * consistency checking is done down inside of the SLAdd() call.
	 * However, DsrSLSetDefaults() is called at the end of this routine
	 * to set up the actual state/size defaults we will leave this
	 * routine with.  Setting these in two places is not good and
	 * should be coalesced or better written so it's not duplicated
	 * (and even potentially different logic)...
	 */
	for (i = 0; fs_space && fs_space[i]; i++) {

		/*
		 * If tracing is enabled
		 */

		if (get_trace_level() > 2) {
			write_status(LOGSCR, LEVEL0,
			    "Create - Adding SW FS space entry: %s",
			    fs_space[i]->fsp_mntpnt);
		}

		/*
		 * If the Ignore bit has been set in the space structure then
		 * the file system is being collapsed into the
		 * parent file system
		 */
		if (fs_space[i]->fsp_flags & FS_IGNORE_ENTRY) {
			SliceState = SLCollapse;
		}
		/*
		 * If the insufficient space bit is set in the space structure
		 * then the slice is set to changeable.
		 */
		else if (fs_space[i]->fsp_flags & FS_INSUFFICIENT_SPACE) {
			SliceState = SLChangeable;
		}
		/*
		 * Otherwise, the state is set to Fixed.
		 */
		else {
			SliceState = SLFixed;
		}
		SLAdd(slhandle,
		    basename(fs_space[i]->fsp_fsi->fsi_device),
		    fs_space[i]->fsp_mntpnt,
		    SL_GENERATE_INSTANCE,
		    TRUE,
		    SLUfs,
		    SliceState,
		    (ulong) fs_space[i]->fsp_reqd_slice_size,
		    fs_space[i],
		    (void *) NULL,
		    &slentry);
	}

	/*
	 * Now, for each entry in the disk list that hasn't already
	 * been added to the slice list, add it now.
	 * These are not in the vfstab.
	 */
	WALK_DISK_LIST(dp) {
		/*
		 * I want these all selected, so that
		 * all the SliceobjFindUse calls will work on all disks.
		 * autolayout also requires any disks that we're using
		 * to be selected,
		 */
		WALK_SLICES_STD(i) {
			/* don't put "overlap" in the list */
			if (streq(Sliceobj_Use(CFG_CURRENT, dp, i), OVERLAP)) {
				continue;
			}

			/* slice has to have size to matter to me */
			if (!Sliceobj_Size(CFG_CURRENT, dp, i)) {
				continue;
			}

			/* get slice name */
			(void) strcpy(slicebuf,
				make_slice_name(disk_name(dp), i));

			/* we skip this slice if already in the slice list */
			if (SLNameInList(slhandle, slicebuf) == SLSuccess)
				continue;

			/*
			 * If the slice to be added is not a swap
			 */

			if (!streq(Sliceobj_Use(CFG_CURRENT, dp, i), SWAP)) {
				SLAdd(slhandle,
					slicebuf,
					Sliceobj_Use(CFG_CURRENT, dp, i),
					SL_GENERATE_INSTANCE,
					FALSE,
					SLUfs,
					SLFixed,
					(ulong) 0,
					(FSspace *) NULL,
					(void *) NULL,
					&slentry);
			} else {
				/*
				 * handle the add of swap specially.
				 * 1st, let the add routine figure out
				 * if this swap slice is in the vfstab.
				 * SLAdd doesn't fill in a mount point
				 * name for swap, so fill it in
				 * regardless.
				 * And if it's not in the vfstab, mark
				 * it as file type swap (needed for the
				 * instance number setting logic).
				 */
				SLAdd(slhandle,
					slicebuf,
					NULL,
					SL_GENERATE_INSTANCE,
					FALSE,
					SLUnknown,
					SLFixed,
					(ulong) 0,
					(FSspace *) NULL,
					(void *) NULL,
					&slentry);
				if (strlen(slentry->MountPoint) == 0) {
					(void) strcpy(slentry->MountPoint,
						Sliceobj_Use(CFG_CURRENT,
							dp, i));
				}
				if (!slentry->InVFSTab) {
					slentry->FSType = SLSwap;
				}
			}
		}
	}

	/*
	 * Sort the slice list by slice name before we leave
	 */
	SLSort(slhandle, SLSliceNameAscending);

	/*
	 * Now that we have a complete list - go fill in the
	 * instance numbers...
	 */
	(void) DsrSLSetInstanceNumbers(slhandle);

	if (get_trace_level() > 2) {
		write_status(LOGSCR, LEVEL0,
			"Instance Numbers from disk list:");
		WALK_DISK_LIST(dp) {
			WALK_SLICES_STD(i) {
				write_status(LOGSCR, LEVEL1,
				    "%s (%s): instance_number = %d",
				    Sliceobj_Use(CFG_CURRENT, dp, i),
				    make_slice_name(disk_name(dp), i),
				    Sliceobj_Instance(CFG_CURRENT, dp, i));
			}
		}
	}

	/*
	 * Now that the instance numbers have all been set,
	 * commit the disk state.  So, in CFG_COMMIT at this point we
	 * have the original disk configuration + the assigned instance
	 * numbers.
	 */
	DiskCommitAll();

	/*
	 * Go and set the default attributes.  (e.g. If the space
	 * structure says that the file system has insufficient space
	 * then set the state to changeable)
	 */

	if (DsrSLSetDefaults(slhandle)) {
		return (FAILURE);
	}

	if (get_trace_level() > 2) {
		write_status(LOGSCR, LEVEL0, "Slice List after creation:");
		SLPrint(slhandle, 0);
	}

	return (SUCCESS);
}

/*
 * Function:	DsrSLUICreate
 * Description:
 *	Destroy the old slice list if there is one and create a new
 *	one.  It creates the slice list with all the default values
 *	and initializes the UI data as well.
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 *	archive_list	[RW] (TDSRArchiveList)
 *		ptr to archive list handle
 *	fs_space:	[RO] (FSspace **)
 *		SW library space table.
 * Return:
 *	SUCCESS:
 *	FAILURE:
 * Globals:
 * Notes:
 */
int
DsrSLUICreate(TDSRArchiveList *archive_list,
	TList *slice_list, FSspace **fs_space)
{
	DsrSLListExtraData *LLextra;

	write_debug(APP_DEBUG_L1, "Creating UI slice list");

	if (!slice_list)
		return (FAILURE);

	/*
	 * destroy the old list, if there is one
	 */
	if (*slice_list) {
		if (LLClearList(*slice_list, DsrSLUIEntryDestroy)) {
			return (FAILURE);
		}

		/* destroy the slice list handle and it's list level data */
		if (LLDestroyList(slice_list, (TLLData *)&LLextra)) {
			return (FAILURE);
		}
		if (LLextra) {
			if (LLextra->filter_pattern)
				free(LLextra->filter_pattern);

			if (LLextra->history.filter_pattern)
				free(LLextra->history.filter_pattern);
			if (LLextra->history.media_device)
				free(LLextra->history.media_device);

			/*
			 * free any additional extra app data in here...
			 * (there is none currently)
			 */
		}

	}

	/* destroy the archive list too */
	if (archive_list && *archive_list) {
		if (DSRALDestroy(*archive_list)) {
			return (FAILURE);
		}
	}

	/*
	 * make the new slice list
	 * The standard defaults get set in the DsrSLCreate call as well.
	 */
	LLextra = (DsrSLListExtraData *) xcalloc(sizeof (DsrSLListExtraData));
	if (DsrSLCreate(slice_list, (TLLData)LLextra, fs_space)) {
		return (FAILURE);
	}

	/*
	 * Initialize the slice list entry data fields in the
	 * extra pointer that UI's need.
	 */
	if (DsrSLUIInitExtras(*slice_list)) {
		return (FAILURE);
	}

	/*
	 * Initially filter the list so everything is in it.
	 */
	if (DsrSLFilter(*slice_list, NULL)) {
		return (FAILURE);
	}

	return (SUCCESS);
}

/*
 * Function:	DsrSLUIEntryDestroy
 * Description:
 *	This is the destroy routine called from the LLClearList()
 *	function.
 *	It passes us a slice entry and we want to
 *	destroy the UI extra data that was associated with this slice.
 * Scope:	PUBLIC
 * Parameters:
 *	TLLData data:
 *		really a slice entry
 * Return:
 *	(TLLError)
 * Globals:
 * Notes:
 */
TLLError
DsrSLUIEntryDestroy(TLLData data)
{
	TSLEntry *slentry = (TSLEntry *)data;
	DsrSLEntryExtraData *SLEntryextra;

	/* free the extra data */
	SLEntryextra = (DsrSLEntryExtraData *)slentry->Extra;
	if (SLEntryextra) {
		if (SLEntryextra->history.final_size)
			free(SLEntryextra->history.final_size);
		free(SLEntryextra);
	}

	/* free the slice list data */
	(void) SLClearCallback(data);

	return (LLSuccess);
}

/*
 * Function:	DsrSLUIInitExtras
 * Description:
 *	This is called after the slice list has been generated to
 *	initialize the extra data pointers for the UI apps.
 *	It initializes the slice list level data (e.g. swap info)
 *	as well as initializes each slice list entry's extra data
 *	pointer.
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 * Return:
 *	SUCCESS:
 *	FAILURE:
 * Globals:
 * Notes:
 */
int
DsrSLUIInitExtras(TList slhandle)
{
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;
	DsrSLEntryExtraData *SLEntryextra;
	DsrSLListExtraData *LLextra;

	/* set up list level data for the list itself */
	if (LLGetSuppliedListData(slhandle, NULL, (TLLData *)&LLextra)) {
		return (FAILURE);
	}
	LLextra->history.filter_type = SLFilterAll;
	LLextra->history.filter_pattern = NULL;
	LLextra->history.media_type = DSRALNFS;
	LLextra->history.media_device = NULL;
	LLextra->swap.reqd =
		sectors_to_kb_trunc(ResobjGetSwap(RESSIZE_MINIMUM));
	LLextra->swap.num_in_vfstab = 0;

	/* set up extra data for each slice */
	LL_WALK(slhandle, slcurrent, slentry, err) {
		if (slentry->InVFSTab && slentry->FSType == SLSwap) {
			LLextra->swap.num_in_vfstab++;
		}

		SLEntryextra = (DsrSLEntryExtraData *)
			xcalloc(sizeof (DsrSLEntryExtraData));
		SLEntryextra->in_filter = TRUE;
		SLEntryextra->extra = (void *) NULL;
		slentry->Extra = (void *) SLEntryextra;

		/*
		 * set the history fields, etc...
		 */
		DsrSLUIEntrySetDefaults(slentry);
	}
	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}
	return (SUCCESS);
}

/*
 * Function:	DsrSLUIResetDefaults
 * Description:
 *	Reset the slice data to a standard defaults for the
 *	interactives.
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 *	fs_space:	[RO] (FSspace **)
 *		SW library space table.
 *	lose_collapse:	[RO] (int)
 *		flag indicating if the UI reset should also blow away
 *		any file system collapse requests the user has made.
 * Return:
 *	SUCCESS:
 *	FAILURE:
 * Globals:
 * Notes:
 */
int
DsrSLUIResetDefaults(TList slhandle, FSspace **fs_space, int lose_collapse)
{
	int i;

	/*
	 * lose the collapse edits
	 */
	if (lose_collapse) {
		for (i = 0; fs_space[i]; i++) {
			fs_space[i]->fsp_flags &= ~FS_IGNORE_ENTRY;
		}
	}

	/*
	 * redo the verify_fs_layout in case we just changed the
	 * ignore entries at all.
	 *
	 * I'm not concerned with the return value of the analyze call
	 * here.  Since we're here, we already know that it fails...
	 */
	(void) DsrFSAnalyzeSystem(fs_space, NULL, NULL, NULL);

	/* setup the default states for all the entries */
	if (DsrSLSetDefaults(slhandle)) {
		return (FAILURE);
	}

	if (DsrSLUISetDefaults(slhandle)) {
		return (FAILURE);
	}

	if (get_trace_level() > 2) {
		DsrSLPrint(slhandle, __FILE__, __LINE__);
	}
	return (SUCCESS);
}

/*
 * Function:	DsrSLSetDefaults
 * Description:
 *	Run the slice list and set all the slice list entries to their
 *	default values.
 * Scope:	PRIVATE
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 * Scope:	PUBLIC
 * Parameters:
 * Return:
 *	SUCCESS:
 *	FAILURE:
 * Globals:
 * Notes:
 */
int
DsrSLSetDefaults(TList slhandle)
{
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;

	LL_WALK(slhandle, slcurrent, slentry, err) {
		if (DsrSLEntrySetDefaults(slentry)) {
			return (FAILURE);
		}
	}

	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}

	return (SUCCESS);
}

/*
 * Function:	DsrSLEntrySetDefaults
 * Description:
 *	Set the default values for this slice list entry.
 *	The default values are:
 *		- all non-vfstab slices are fixed
 *			with final size == original size.
 *		- all failed vfstab slices are changeable with
 *			final size == required size.
 *		- all other vfstab slices are fixed.
 *			with final size == original size.
 *
 * Scope:	PUBLIC
 * Parameters:
 *	slentry:	[RW] (TSLEntry *)
 *		The slice entry to set the defaults in.
 * Return:
 *	SUCCESS:
 *	FAILURE:
 * Globals:	none
 * Notes:
 */
int
DsrSLEntrySetDefaults(TSLEntry *slentry)
{
	/*
	 * Set the allowed states bit mask.
	 * This may change in the case where a slice has been
	 * marked as collapsed/uncollapsed since the mask was
	 * previously determined.
	 */
	SLSetAllowedStates(slentry);

	/*
	 * set the default state and the default size
	 */
	/* if it's in the vfstab */
	if (slentry->InVFSTab) {
		if (SL_SLICE_IS_COLLAPSED(slentry)) {
			slentry->State = SLCollapse;
			slentry->Size = 0;
		} else if (SL_SLICE_HAS_INSUFF_SPACE(slentry)) {
			/* slice failed due to lack of space */
			slentry->State = SLChangeable;
			DsrSLEntryGetAttr(slentry,
				DsrSLAttrReqdSize,
				&slentry->Size,
				NULL);
		} else {
			/* most non-failed slice default to fixed */
			slentry->State = SLFixed;
			DsrSLEntryGetAttr(slentry,
				DsrSLAttrExistingSize,
				&slentry->Size,
				NULL);
		}
	} else {
		/* all non-vfstab entries default to fixed */
		slentry->State = SLFixed;
		DsrSLEntryGetAttr(slentry,
			DsrSLAttrExistingSize,
			&slentry->Size,
			NULL);
	}
	return (SUCCESS);
}

/*
 * Function:	DsrSLUISetDefaults
 * Description:
 *	Set the UI specific default data for the slice list.
 *	Basically this means setting the history data in the extra data
 *	pointers so that the UI's are seeded correctly.
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 * Return:
 *	SUCCESS:
 *	FAILURE:
 * Globals:
 * Notes:
 */
int
DsrSLUISetDefaults(TList slhandle)
{

	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;

	LL_WALK(slhandle, slcurrent, slentry, err) {
		DsrSLUIEntrySetDefaults(slentry);

	}

	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}
	return (SUCCESS);
}

/*
 * Function:	DsrSLUIEntrySetDefaults
 * Description:
 *	Set the UI specific default data for a given slice list entry.
 *	Basically this means setting the history data in the extra data
 *	pointers so that the UI's are seeded correctly.
 * Scope:	PUBLIC
 * Parameters:
 *	TSLEntry *slentry: slice list entry
 * Return:	none
 * Globals:
 * Notes:
 */
void
DsrSLUIEntrySetDefaults(TSLEntry *slentry)
{
	DsrSLEntryExtraData *SLEntryextra;
	char buf[UI_FS_SIZE_DISPLAY_LENGTH + 1];

	SLEntryextra = (DsrSLEntryExtraData *)slentry->Extra;

	if (!SLEntryextra)
		return;

	if (SLEntryextra->history.final_size)
		free(SLEntryextra->history.final_size);

	/* if it's in the vfstab */
	if (slentry->InVFSTab) {
		if (SL_SLICE_IS_COLLAPSED(slentry)) {
			(void) sprintf(buf, "%*lu",
				UI_FS_SIZE_DISPLAY_LENGTH, (ulong) 0);
			SLEntryextra->history.final_size = xstrdup(buf);
		} else if (SL_SLICE_HAS_INSUFF_SPACE(slentry)) {
			DsrSLEntryGetAttr(slentry,
				DsrSLAttrReqdSizeStr,
				&SLEntryextra->history.final_size,
				NULL);
		} else {
			DsrSLEntryGetAttr(slentry,
				DsrSLAttrExistingSizeStr,
				&SLEntryextra->history.final_size,
				NULL);
		}
	} else {
		DsrSLEntryGetAttr(slentry,
			DsrSLAttrExistingSizeStr,
			&SLEntryextra->history.final_size,
			NULL);
	}
}

/*
 * Function:	DsrIsDeviceCoResident
 * Description:
 *	Determine if this slice is co-resident with a failed slice.
 * Scope:	PUBLIC
 * Parameters:
 *	fs_space:	[RO] (FSspace **)
 *		SW library space table.
 *	device:		[RO] (char *)
 *		slice string we want to check co-residence for.
 *		device may look like c0t0d0s0 or c0d0s0
 *		or like /dev/dsk/c0t0d0s0...
 * Return:
 *	TRUE:	device is co-resident with a failed slice
 *	FALSE:	device is not co-resident with a failed slice
 * Globals:	none
 * Notes:
 */
int
DsrIsDeviceCoResident(FSspace **fs_space, char *device)
{
	int i;
	int cmp_len;
	char *tmp_device;
	char *tmp_disk;
	char *ptr;

	/*
	 * Find length up to slice number for something that looks like:
	 * /dev/dsk/c0t0d0s0 or c0t0d0s0
	 */
	tmp_device = xstrdup(basename(device));
	tmp_disk = xstrdup(tmp_device);
	ptr = strrchr(tmp_disk, 's');
	*ptr = '\0';
	cmp_len = strlen(tmp_disk);
	free(tmp_disk);

	for (i = 0; fs_space[i]; i++) {
		if (fs_space[i]->fsp_flags & FS_INSUFFICIENT_SPACE) {
			if (strneq(basename(fs_space[i]->fsp_fsi->fsi_device),
				tmp_device, cmp_len)) {
				free(tmp_device);
				return (TRUE);
			}
		}
	}

	free(tmp_device);
	return (FALSE);
}

/*
 * Function:	DsrGetSliceNumFromDeviceName
 * Description:
 *	 Get the slice number from a device (slice) name.
 * Scope:	PUBLIC
 * Parameters:
 *	device:	[RO] (char *)
 *		slice/device specifier to get slice number from.
 *		device may look like c0t0d0s0 or c0d0s0
 *		or /dev/dsk/c0t0d0s0...
 * Return:
 *	-1: Error - could not get slice num from device name
 *	>=0: slice number
 * Globals:
 * Notes:
 */
int
DsrGetSliceNumFromDeviceName(char *device)
{
	int scan_ret;
	int slice_num;

	/*
	 * get just the c0t0d0s0 part of the device name and
	 * scan everything after the 's' into the slice number
	 */
	scan_ret = sscanf(strchr(basename(device), 's'),
				"s%d", &slice_num);
	if (scan_ret != 1) {
		write_debug(APP_DEBUG_L1,
			"DsrGetSliceNumFromDeviceName() failed");
		return (-1);
	}

	return (slice_num);
}

/*
 * Function:	DsrSLEntryGetAttr
 * Description:
 *	Routine to retrieve the requested data from a slice entry.
 * Scope:	PUBLIC
 * Parameters:
 *	slentry	[RO]: (TSLEntry *)
 *		the slice list entry we want to get data about.
 *	...	attribute value pairs for requesting data.
 *		attributes are of type DsrSLEntryAttr.
 * Return:
 *	Function is a varargs function that returns the requested
 *	values in the supplied argument pointers.
 * Globals:	uses the global disk list.
 * Notes:
 *	All *Str size values returned in MB (mainly intended for use
 *	by the UI's).
 *
 *	Rounding notes:
 *	All values that are dipslayed are rounded so that the
 *	required sizes are rouned up and the current sizes are rouned down.
 *	This way, since the sizes are also converted to MB, any sizes that
 *	differ but are within a MB of each other, will display as actually
 *	being different by rounding one up and one down to the nearest MB.
 *	Otherwise we could end telling them that the required size is 24 MB
 *	and that the existing size is 24 MB and that the file system failed
 *	the space check - could be a bit confusing...
 */
void
DsrSLEntryGetAttr(TSLEntry *slentry, ...)
{
	DsrSLEntryAttr attr;
	char **vcp;
	ulong *vulp;
	char buf[PATH_MAX];
	ulong size;
	ulong size2;
	va_list ap;
	char *str;
	SliceKey *slice_key;

	if (!slentry)
		return;

	set_units(D_KBYTE);
	va_start(ap, slentry);

	while ((attr = va_arg(ap, DsrSLEntryAttr)) != NULL)  {

	switch (attr) {
	case DsrSLAttrReqdSize:
		vulp = va_arg(ap, ulong*);

		/* req'd size only makes sense for vfstab slices */
		if (slentry->InVFSTab) {
			if (slentry->FSType == SLSwap) {
				DsrSLEntryGetAttr(slentry,
					DsrSLAttrExistingSize, &size,
					NULL);
				*vulp = size;
			} else {
				*vulp = slentry->Space->fsp_reqd_slice_size;
			}
		} else {
			*vulp = 0;
		}
		break;
	case DsrSLAttrReqdSizeStr:
		vcp = va_arg(ap, char **);

		DsrSLEntryGetAttr(slentry,
			DsrSLAttrReqdSize, &size,
			NULL);

		/* round up the req'd size */
		if (slentry->InVFSTab) {
			(void) sprintf(buf, "%*lu",
				UI_FS_SIZE_DISPLAY_LENGTH,
				kb_to_mb(size));
		} else {
			(void) sprintf(buf, "%s", LABEL_DSR_FSREDIST_NA);
		}
		*vcp = xstrdup(buf);
		break;
	case DsrSLAttrExistingSize:
		vulp = va_arg(ap, ulong*);

		if (slentry->InVFSTab && slentry->FSType != SLSwap) {
			*vulp = slentry->Space->fsp_cur_slice_size;
		} else {
			slice_key = SliceobjFindUse(CFG_COMMIT, NULL,
				slentry->MountPoint,
				slentry->MountPointInstance,
				1);
			if (!slice_key)
				*vulp = (ulong) 0;
			else
				*vulp = (ulong) blocks2size(slice_key->dp,
					Sliceobj_Size(CFG_COMMIT,
						slice_key->dp,
						slice_key->slice),
					ROUNDDOWN);
		}
		break;
	case DsrSLAttrExistingSizeStr:
		vcp = va_arg(ap, char **);

		DsrSLEntryGetAttr(slentry,
			DsrSLAttrExistingSize, &size,
			NULL);

		/* round down the existing size */
		(void) sprintf(buf, "%*lu",
			UI_FS_SIZE_DISPLAY_LENGTH,
			kb_to_mb_trunc(size));
		*vcp = xstrdup(buf);
		break;
	case DsrSLAttrCurrentSize:
		vulp = va_arg(ap, ulong*);

		slice_key = SliceobjFindUse(CFG_CURRENT, NULL,
			slentry->MountPoint,
			slentry->MountPointInstance,
			1);
		if (!slice_key)
			*vulp = (ulong) 0;
		else
			*vulp = (ulong) blocks2size(slice_key->dp,
				Sliceobj_Size(CFG_CURRENT,
					slice_key->dp, slice_key->slice),
				ROUNDDOWN);
		break;
	case DsrSLAttrCurrentSizeStr:
		vcp = va_arg(ap, char **);

		DsrSLEntryGetAttr(slentry,
			DsrSLAttrCurrentSize, &size,
			NULL);

		/* round display of current size down */
		(void) sprintf(buf, "%*lu",
			UI_FS_SIZE_DISPLAY_LENGTH,
			kb_to_mb_trunc(size));
		*vcp = xstrdup(buf);
		break;
	case DsrSLAttrFreeSpace:
		vulp = va_arg(ap, ulong*);
		if (slentry->InVFSTab && slentry->FSType != SLSwap) {
			*vulp = slentry->Space->fsp_fsi->f_bavail;
		} else {
			/* same as existing size */
			DsrSLEntryGetAttr(slentry,
				DsrSLAttrExistingSize, vulp,
				NULL);
		}
		break;
	case DsrSLAttrFreeSpaceStr:
		vcp = va_arg(ap, char **);

		if (slentry->InVFSTab && slentry->FSType == SLSwap) {
			/*
			 * free space on a vfstab swap slice doesn't really
			 * make sense...
			 */
			*vcp = xstrdup(LABEL_DSR_FSREDIST_NA);
		} else {
			DsrSLEntryGetAttr(slentry,
				DsrSLAttrFreeSpace, &size,
				NULL);

			(void) sprintf(buf, "%*lu",
				UI_FS_SIZE_DISPLAY_LENGTH,
				kb_to_mb_trunc(size));
			*vcp = xstrdup(buf);
		}
		break;
	case DsrSLAttrSpaceReqd:
		vulp = va_arg(ap, ulong*);
		if (slentry->InVFSTab) {
			/*
			 * swap doesn't have size checking on it and
			 * isn't in the space structure. So Space
			 * Reqd isn't terribly relevant for swap.
			 */
			if (slentry->FSType == SLSwap) {
				*vulp = (ulong) 0;
			} else {
				DsrSLEntryGetAttr(slentry,
					DsrSLAttrReqdSize, &size,
					NULL);
				DsrSLEntryGetAttr(slentry,
					DsrSLAttrExistingSize, &size2,
					NULL);
				if (size > size2) {
					/* req'd > current */
					*vulp = size - size2;
				} else {
					/* there's enough space, so it's 0 */
					*vulp = (ulong) 0;
				}
			}
		} else {
			/* not relevant for non-vfstab entries */
			*vulp = (ulong) 0;
		}
		break;
	case DsrSLAttrSpaceReqdStr:
		vcp = va_arg(ap, char **);

		DsrSLEntryGetAttr(slentry,
			DsrSLAttrSpaceReqd, &size,
			NULL);

		if (slentry->InVFSTab) {
			if (slentry->FSType == SLSwap) {
				*vcp = xstrdup(LABEL_DSR_FSREDIST_NA);
			} else {
				(void) sprintf(buf, "%*lu",
					UI_FS_SIZE_DISPLAY_LENGTH,
					kb_to_mb(size));
				*vcp = xstrdup(buf);
			}
		} else {
			*vcp = xstrdup(LABEL_DSR_FSREDIST_NA);
		}
		break;
	case DsrSLAttrMountPointStr:
		vcp = va_arg(ap, char **);

		(void) sprintf(buf, "%s",
			strlen(slentry->MountPoint) ?
				slentry->MountPoint :
				APP_FS_UNNAMED);

		*vcp = xstrdup(buf);
		break;
	case DsrSLAttrTaggedMountPointStr:
		vcp = va_arg(ap, char **);

		DsrSLEntryGetAttr(slentry,
			DsrSLAttrMountPointStr, &str,
			NULL);

		if (slentry->InVFSTab && slentry->FSType == SLSwap) {
			(void) sprintf(buf, "%*s%-*.*s",
				strlen(LABEL_DSR_FSREDIST_LEGENDTAG_FAILED),
				" ",
				UI_FS_DISPLAY_LENGTH,
				UI_FS_DISPLAY_LENGTH,
				str);
		} else if (slentry->InVFSTab &&
			SL_SLICE_HAS_INSUFF_SPACE(slentry)) {
			(void) sprintf(buf, "%s%-*.*s",
				LABEL_DSR_FSREDIST_LEGENDTAG_FAILED,
				UI_FS_DISPLAY_LENGTH,
				UI_FS_DISPLAY_LENGTH,
				str);
		} else {
			(void) sprintf(buf, "%*s%-*.*s",
				strlen(LABEL_DSR_FSREDIST_LEGENDTAG_FAILED),
				" ",
				UI_FS_DISPLAY_LENGTH,
				UI_FS_DISPLAY_LENGTH,
				str);
		}

		if (str)
			free(str);
		*vcp = xstrdup(buf);
		break;
	case DsrSLAttrExistingSlice:
	case DsrSLAttrCurrentSlice:
		slice_key = SliceobjFindUse(
			attr == DsrSLAttrExistingSlice ?
				CFG_COMMIT : CFG_CURRENT,
			NULL,
			slentry->MountPoint,
			slentry->MountPointInstance,
			1);
		if (!slice_key)
			*vcp = NULL;
		else
			*vcp = xstrdup(make_slice_name(
				disk_name(slice_key->dp), slice_key->slice));
		break;
	default:
		break;
	} /* end switch */

	} /* end while */
	va_end(ap);
}

/*
 * Function:	DsrSLListGetAttr
 * Description:
 *	Routine to retrieve the requested data from a slice list.
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		the slice list we want to get data about.
 *	...	attribute value pairs for requesting data.
 *		attributes are of type DsrSLListAttr.
 * Return:
 *	Function is a varargs function that returns the requested
 *	values in the supplied argument pointers.
 * Globals:
 * Notes:
 */
void
DsrSLListGetAttr(TList slhandle, ...)
{
	char **vcp;
	DsrSLListAttr attr;
	TDSRALMedia *vmedia_type;
	TDSRALMedia media_type;
	DsrSLListExtraData *LLextra;
	va_list ap;
	char buf[PATH_MAX];
	char *str;
	char *media_type_str;
	char *media_device_str;

	(void) LLGetSuppliedListData(slhandle, NULL, (TLLData *)&LLextra);
	if (!LLextra)
		return;

	va_start(ap, slhandle);

	while ((attr = va_arg(ap, DsrSLListAttr)) != NULL)  {

	switch (attr) {
	case DsrSLAttrMediaType:
		vmedia_type = va_arg(ap, TDSRALMedia*);
		*vmedia_type = LLextra->history.media_type;
		break;
	case DsrSLAttrMediaTypeStr:
		vcp = va_arg(ap, char **);

		DsrSLListGetAttr(slhandle,
			DsrSLAttrMediaType, &media_type,
			NULL);
		str = DsrALMediaTypeStr(media_type);
		*vcp = xstrdup(str);
		break;
	case DsrSLAttrMediaTypeDeviceStr:
		vcp = va_arg(ap, char **);
		(void) sprintf(buf, "%s %s",
			DsrALMediaTypeDeviceStr(LLextra->history.media_type),
			DsrALMediaTypeEgStr(LLextra->history.media_type));
		*vcp = xstrdup(buf);
		break;
	case DsrSLAttrMediaTypeEgStr:
		vcp = va_arg(ap, char **);
		str = DsrALMediaTypeEgStr(media_type);
		*vcp = xstrdup(str);
		break;
	case DsrSLAttrMediaDeviceStr:
		vcp = va_arg(ap, char **);
		if (LLextra->history.media_device)
			*vcp = xstrdup(LLextra->history.media_device);
		else
			*vcp = NULL;
		break;
	case DsrSLAttrMediaToggleStr:
		vcp = va_arg(ap, char **);

		DsrSLListGetAttr(slhandle, NULL,
			DsrSLAttrMediaTypeStr, &media_type_str,
			DsrSLAttrMediaTypeEgStr, &media_device_str,
			NULL);
		(void) sprintf(buf, "%*s: %s",
			25, media_type_str,
			media_device_str);
		free(media_type_str);
		free(media_device_str);
		*vcp = xstrdup(buf);
		break;
	default:
		break;
	} /* end switch */

	} /* end while */
	va_end(ap);
}

/*
 * Function:	DsrSLFilter
 * Description:
 *	Apply the filter that is stored in the slice list level data
 *	to the slice list.
 *	The in_filter value of each slices extra data pointer is set
 *	according to whether it matched this filter or not.
 *
 *	The UI extra data pointer fields must be filled in before this
 *	function can be called.
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 *	match_cnt:	[*RW] (int *)
 *		pointer in which to return the number of slices in the
 *		slice list that match this filter request.
 * Return:
 *	SUCCESS
 *	FAILURE --> RE expression match error
 * Globals:
 * Notes:
 */
int
DsrSLFilter(TList slhandle, int *match_cnt)
{

	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;
	DsrSLEntryExtraData *SLEntryextra;
	DsrSLListExtraData *LLextra;
	int num_matched;
	TSLFilter filter_type;
	char *pattern;

	/* get list level user data */
	(void) LLGetSuppliedListData(slhandle, NULL, (TLLData *)&LLextra);
	if (!LLextra)
		return (FAILURE);

	filter_type = LLextra->history.filter_type;
	pattern = LLextra->history.filter_pattern;

	write_debug(APP_DEBUG_L1, "Slices in filter %d (%s):",
		filter_type, pattern ? pattern : "");

	num_matched = 0;
	LL_WALK(slhandle, slcurrent, slentry, err) {
		SLEntryextra = (DsrSLEntryExtraData *)slentry->Extra;
		if (!SLEntryextra)
			return (FAILURE);

		switch (filter_type) {
		default:
		case SLFilterAll:
			SLEntryextra->in_filter = TRUE;
			break;
		case SLFilterFailed:
			if (SL_SLICE_HAS_INSUFF_SPACE(slentry))
				SLEntryextra->in_filter = TRUE;
			else
				SLEntryextra->in_filter = FALSE;
			break;
		case SLFilterVfstabSlices:
			if (slentry->InVFSTab)
				SLEntryextra->in_filter = TRUE;
			else
				SLEntryextra->in_filter = FALSE;
			break;
		case SLFilterNonVfstabSlices:
			if (!slentry->InVFSTab)
				SLEntryextra->in_filter = TRUE;
			else
				SLEntryextra->in_filter = FALSE;
			break;
		case SLFilterSliceNameSearch:
			if (re_match(slentry->SliceName, pattern, 1)
				== REMatch) {
				SLEntryextra->in_filter = TRUE;
			} else {
				SLEntryextra->in_filter = FALSE;
			}
			break;
		case SLFilterMountPntNameSearch:
			if (re_match(slentry->MountPoint, pattern, 1)
				== REMatch) {
				SLEntryextra->in_filter = TRUE;
			} else {
				SLEntryextra->in_filter = FALSE;
			}
			break;
		}
		if (SLEntryextra->in_filter)
			num_matched++;

		if (SLEntryextra->in_filter) {
			write_debug(APP_DEBUG_L1_NOHD,
				"Slice %s (%s): filter = %d",
				slentry->SliceName,
				slentry->MountPoint,
				SLEntryextra->in_filter);
		}
	}

	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}

	/* return the number of matched entries */
	if (match_cnt)
		*match_cnt = num_matched;

	/* if any matched, update the last filter info */
	if (num_matched) {
		LLextra->filter_type = filter_type;
		if (LLextra->filter_pattern)
			free(LLextra->filter_pattern);
		LLextra->filter_pattern = xstrdup(pattern);
	}

	/*
	 * Print the list in debug mode
	 */
	if (get_trace_level() > 2)
		DsrSLPrint(slhandle, DEBUG_LOC);

	return (SUCCESS);
}

/*
 * Function:	DsrALMediaErrorStr
 * Description:
 *	Get the error string for this media error.
 * Scope:	PUBLIC
 * Parameters:
 *	TDSRALMedia Media
 *		the media type
 *	char *MediaPath
 *		the media path supplied by the user
 *	TDSRALError err
 *		the media error encountered
 * Return:
 *	media error string - dynamically allocated - user should free
 *	when done with it
 * Globals:
 * Notes:
 */
char *
DsrALMediaErrorStr(TDSRALMedia Media, char *MediaPath, TDSRALError err)
{
	char buf[PATH_MAX];

	write_debug(APP_DEBUG_L1, "Media error = %d", err);

	(void) sprintf(buf, APP_ER_DSR_MEDIA_SUMM,
		DSRALGetErrorText(err),
		DsrALMediaTypeStr(Media),
		MediaPath);

	return (xstrdup(buf));
}

/*
 * Function:	DsrALMediaErrorIsFatal
 * Description:
 *	Figure out if this is a media error caused by user input which
 *	is fixable, or if this is fatal.
 * Scope:	PUBLIC
 * Parameters:
 *	TDSRALError err
 *		the media error encountered
 * Return:
 *	TRUE
 *	FALSE
 * Globals:
 * Notes:
 */
int
DsrALMediaErrorIsFatal(TDSRALError err)
{
	switch (err) {
	case DSRALCallbackFailure:
	case DSRALMemoryAllocationFailure:
	case DSRALInvalidHandle:
	case DSRALUpgradeCheckFailure:
	case DSRALInvalidMedia:
	case DSRALChildProcessFailure:
	case DSRALListManagementError:
	case DSRALSystemCallFailure:
	case DSRALItemNotFound:
	case DSRALInvalidFileType:
		return (TRUE);
	default:
		return (FALSE);
	}

	/* NOTREACHED */
}

/*
 * Function:	DsrSLGetSwapInfo
 * Description:
 *	get the amount of total swap specified in the slice list.
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 *	total_swap:	[RW] (ulong *)
 *		ptr to total swap value.
 *		total swap is the total cumulative swap (the sum of the
 *		size field field of all swap entries in the slice list)
 * Return:
 *	SUCCESS:
 *	FAILURE:
 * Globals:
 * Notes:
 *	return all units in MB
 */
int
DsrSLGetSwapInfo(TList slhandle, ulong *total_swap)
{
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;
	DsrSLEntryExtraData *SLEntryextra;
	ulong size;

	if (!slhandle || !total_swap)
		return (FAILURE);

	*total_swap = 0;
	LL_WALK(slhandle, slcurrent, slentry, err) {
		SLEntryextra = slentry->Extra;

		if (slentry->FSType == SLSwap && slentry->InVFSTab) {
			if (SLEntryextra &&
				SLEntryextra->history.final_size) {
				if (DsrSLValidFinalSize(
					SLEntryextra->history.final_size,
					&size))
					*total_swap += size;
			}
		}
	}
	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}
	return (SUCCESS);
}

/*
 * Function:	DsrSLSetInstanceNumbers
 * Description:
 *	Set the instance numbers for the slice list entries.
 *	The slice list must be fully populated at this point.
 *	Instance number setting:
 *		- anything in the vfstab besides swap is instance number 0
 *		(since file system mount points are unique)
 *		- anything not in the vfstab are assigned starting at
 *		0 if there is no matching mount point name in the
 *		vfstab, and at 1 if there is a matching mount point name
 *		in the vfstab.
 *		(they may not be unique and so are just assigned
 *		incremental numbers as they are encountered)
 *		- The n swap entries in the vfstab are numbered from 0...(n-1),
 *		and swap entries not in the vfstab are numbered n...m.
 *
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 * Return:
 *	SUCCESS:
 *	FAILURE:
 * Globals:
 * Notes:
 */
int
DsrSLSetInstanceNumbers(TList slhandle)
{
	TSLEntry *slentry;
	TLink slcurrent;
	TLLError err;
	TList slhandle_copy;
	int num_swap_in_vfstab = 0;
	int slice_num;
	Disk_t *dp;

	/*
	 * Copy slice list for nested slice list walking
	 */
	if (_DsrSLCopy(&slhandle_copy, slhandle)) {
		return (FAILURE);
	}

	/*
	 * 1st find out how many vfstab swaps there are
	 */
	write_debug(APP_DEBUG_L1, "Instance Numbers for slice list:");
	num_swap_in_vfstab = 0;
	LL_WALK(slhandle, slcurrent, slentry, err) {
		if (slentry->InVFSTab && slentry->FSType == SLSwap)
			num_swap_in_vfstab++;
	}

	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}

	/*
	 * Now, walk the slice list and set the instance numbers for
	 * each entry
	 */
	LL_WALK(slhandle, slcurrent, slentry, err) {
		if (slentry->InVFSTab) {
			if (slentry->FSType == SLSwap) {
				if (_DsrSLSetInstanceNumber(slhandle_copy,
				    slentry, num_swap_in_vfstab)) {
					return (FAILURE);
				}
			} else {
				slentry->MountPointInstance = 0;
			}
		} else {
			if (_DsrSLSetInstanceNumber(slhandle_copy,
			    slentry, num_swap_in_vfstab)) {
				return (FAILURE);
			}
		}

		/*
		 * Set the instance number in the CFG_CURRENT
		 * disk configuration.
		 */
		dp = find_disk(slentry->SliceName);
		slice_num = DsrGetSliceNumFromDeviceName(slentry->SliceName);

		if (SliceobjSetAttribute(dp, slice_num,
			SLICEOBJ_USE, slentry->MountPoint,
			SLICEOBJ_INSTANCE, slentry->MountPointInstance,
			NULL) != D_OK) {
			write_debug(APP_DEBUG_L1,
				"Setting Instance number failed!");
			write_debug(APP_DEBUG_L1_NOHD,
				"Mount: %s, Instance: %d, Slice: %s",
				slentry->MountPoint,
				slentry->MountPointInstance,
				slentry->SliceName);
		}
	}

	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}

	/* free the copied list */
	if (LLClearList(slhandle_copy, DsrSLUIEntryDestroy)) {
		return (FAILURE);
	}
	if (LLDestroyList(&slhandle_copy, NULL)) {
		return (FAILURE);
	}
	return (SUCCESS);
}

/*
 * Function:	_DsrSLSetInstanceNumber
 * Description:
 *	Internal guts of instance setting code.
 *	Set DsrSLSetInstanceNumbers() for details.
 * Scope:	PRIVATE
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 *	TSLEntry *curr_entry:
 *		the current slice list entry we are trying to assign
 *	int num_swap_in_vfstab:
 *		the number of swaps that are in the vfstab
 * Return:
 *	SUCCESS:
 *	FAILURE:
 * Globals:
 * Notes:
 */
static int
_DsrSLSetInstanceNumber(
	TList slhandle,
	TSLEntry *curr_entry,
	int num_swap_in_vfstab)
{
	TSLEntry *slentry;
	TLink slcurrent;
	TLLError err;
	int instance;

	/*
	 * Decide where to start instance number counter.
	 *
	 * The n swap entries in the vfstab are numbered from 0...(n-1), and
	 * swap entries not in the vfstab are numbered n...m.
	 *
	 * If it's not swap and it's in the vfstab, start at 1.
	 * (except for swap, anything in the vfstab only occurs once and
	 * they are always instance # 0's).
	 * If it's not swap and it's not in the vfstab, start at 0
	 * (if there is a similarly named mount point in the vfstab it
	 * will be instance 0.
	 */
	if (curr_entry->FSType == SLSwap) {
		if (curr_entry->InVFSTab)
			instance = 0;
		else
			instance = num_swap_in_vfstab;
	} else {
		if (_DsrSLMountPointInVFSTab(
			slhandle, curr_entry->MountPoint) &&
			!curr_entry->InVFSTab)
			instance = 1;
		else
			instance = 0;
	}

	/*
	 * loop through copy of slice list and get the instance
	 * count for this mount point.
	 */
	LL_WALK(slhandle, slcurrent, slentry, err) {
		if (!streq(curr_entry->MountPoint, slentry->MountPoint))
			continue;

		/*
		 * If I've landed on the one in the copy list
		 * that is the same as the current one
		 * (i.e. the same mount point name AND the
		 * the same slice), then
		 * assign the instance number.
		 *
		 * Else if we find a matching mount point name
		 * in the copy list that is not the same entry
		 * as the current one, then
		 * bump up the counter and keep going.
		 */
		if (streq(curr_entry->SliceName, slentry->SliceName)) {
			/* set the instance number in the slice list */
			curr_entry->MountPointInstance = instance;

			return (SUCCESS);
		} else {

			if (slentry->InVFSTab == curr_entry->InVFSTab)
				instance++;
		}
	}

	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}
	return (SUCCESS);
}

/*
 * Function:	_DsrSLCopy
 * Description:
 *	Internal routine used to copy just enough of the slice list that
 *	we can use the copy to loop thru the slice list in a nested
 *	manner when assigning instance numbers.
 * Scope:	PRIVATE
 * Parameters:
 *	TList *slhandle_copy:
 *		ptr to the slice list handle that will hold the copied
 *		slice list.
 *	TList slhandle:
 *		the actual slice list to copy
 * Return:
 *	SUCCESS:
 *	FAILURE:
 * Globals:
 * Notes:
 */
static int
_DsrSLCopy(TList *slhandle_copy, TList slhandle)
{
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;

	if (!slhandle_copy)
		return (FAILURE);

	if (LLCreateList(slhandle_copy, NULL)) {
		return (FAILURE);
	}

	LL_WALK(slhandle, slcurrent, slentry, err) {
		if (SLAdd(*slhandle_copy,
		    slentry->SliceName,
		    slentry->MountPoint,
		    slentry->MountPointInstance,
		    slentry->InVFSTab,
		    slentry->FSType,
		    slentry->State,
		    slentry->Size,
		    slentry->Space,
		    (void *) NULL,
		    (TSLEntry **) NULL)) {
			return (FAILURE);
		}
	}

	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}
	return (SUCCESS);
}

/*
 * Function:	_DsrSLMountPointInVFSTab
 * Description:
 *	Find out if this mount point name exists in the vfstab.
 * Scope:	PRIVATE
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 *	mount_point:	[RO] (char *)
 *		the mount point name
 * Return:
 *	TRUE
 *	FALSE
 * Globals:
 * Notes:
 */
static int
_DsrSLMountPointInVFSTab(TList slhandle, char *mount_point)
{
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;

	LL_WALK(slhandle, slcurrent, slentry, err) {
		if (streq(slentry->MountPoint, mount_point) &&
			slentry->InVFSTab)
				return (TRUE);
	}

	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FALSE);
	}

	return (FALSE);
}

/*
 * Function: DsrSLGetNumCollapseable
 * Description:
 *	Find out how many slices in the slice list should be displayed
 *	in the collapse file systems screen.
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 * Return: int: number of collapseable slice list entries
 * Globals: none
 * Notes:
 */
int
DsrSLGetNumCollapseable(TList slhandle)
{
	int num_fs;

	TSLEntry *slentry;
	TLink slcurrent;
	TLLError err;

	num_fs = 0;
	LL_WALK(slhandle, slcurrent, slentry, err) {
		if (SL_SLICE_IS_COLLAPSEABLE(slentry)) {
			num_fs++;
		}
	}

	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (0);
	}

	return (num_fs);

}

/*
 * Function:	DsrSLGetParentFS
 * Description:
 *	Finds the parent file system for a given mount point in
 *	the current slice list.	 The only file systems considered for
 *	'parenthood' are those inthe vfstab that are not currently set
 *	as ignored in the sw lib space structure for this slice.
 *
 * Scope:	PUBLIC
 * Parameters:
 *	FSspace **fs_space:
 *		the current FSspace structure - used to see if entrie
 *		are ignored or not (which to use means are they
 *		collapsed or not).
 *	char *mount_point:
 *		the mount point whose parent file system we want to find.
 * Return:
 *	parent mount point
 *	The mount point returned is dynamically allocated.
 * Globals: none
 * Notes:
 *	This is a recursive function
 */
char *
DsrSLGetParentFS(FSspace **fs_space, char *mount_point)
{
	char *ptr;
	int i;
	int found_parent = FALSE;
	char *local_mount_point;
	char *parent;

	if (!mount_point)
		return (NULL);

	local_mount_point = xstrdup(mount_point);

	/* "/" is it's own parent */
	if (streq(local_mount_point, ROOT)) {
		return (local_mount_point);
	}

	/* find the last '/' in the string */
	ptr = strrchr(local_mount_point, '/');
	if (!ptr) {
		free(local_mount_point);
		return (NULL);
	}
	*ptr = '\0';

	if (streq(local_mount_point, "")) {
		return (xstrdup(ROOT));
	}

	/*
	 * local_mount_point now has the "parent" string -
	 * see if this is a file system entry in the vfstab
	 */
	for (i = 0; fs_space[i]; i++) {
		if (streq(fs_space[i]->fsp_mntpnt, local_mount_point)) {
			if (!(fs_space[i]->fsp_flags & FS_IGNORE_ENTRY)) {
				found_parent = TRUE;
			}
			break;
		}
	}

	if (!found_parent) {
		parent = DsrSLGetParentFS(fs_space, local_mount_point);
		free(local_mount_point);
		return (parent);
	} else
		return (local_mount_point);
}


/*
 * Function:	DsrSLGetEntry
 * Description:
 *	Given a mount point name and an instance number,
 *	find the  corresponding slice list entry
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 *	mount_point:	[RO] (char *)
 *		the mount point name
 *	instance_number:	[RO] (int)
 *		the instance number
 * Return:
 *	(TSLEntry *): the slice list entry ptr
 *	NULL if not found
 * Globals:
 * Notes:
 */
TSLEntry *
DsrSLGetEntry(TList slhandle, char *mntpnt, int instance_number)
{
	TSLEntry *slentry;
	TLink slcurrent;
	TLLError err;

	LL_WALK(slhandle, slcurrent, slentry, err) {
		if (streq(slentry->MountPoint, mntpnt) &&
			slentry->MountPointInstance == instance_number)
				return (slentry);
	}

	/*
	 * There is no reason to check the return code from walking
	 * the slice list, since no matter whether we error'd or could
	 * not find the info, both are errors.
	 */

	return ((TSLEntry *) NULL);
}

/*
 * Function:	DsrSLGetSpaceSummary
 * Description:
 *	Run through the slice list and determine the values
 *	the UI's want to report on for total space needed and total
 *	additional space allocated.
 *
 *	Additional space required is total of all:
 *		- (final minimum size - existing_size)
 *		  for all vfstab entries which are changeable with
 *		  (final minimum size > existing_size)
 *
 *	Additional space allocated is total of all:
 *		- (existing_size - final minimum size)
 *		  for all vfstab entries which are changeable with
 *		  (final minimum size < existing_size)
 *		- existing size for all non-vfstab entries marked available
 *		- existing size for all collapsed entries
 *
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 *	ulong *add_space_req
 *		ptr to ulong in which to return the total space needed.
 *	ulong *add_space_alloced
 *		ptr to ulong in which to return the total additional
 *		space allocated.
 * Return:
 *	0 : ok
 *	!0: error
 * Globals: none
 * Notes:
 */
int
DsrSLGetSpaceSummary(TList slhandle,
	ulong *add_space_req,
	ulong *add_space_alloced)
{
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;
	DsrSLEntryExtraData *SLEntryextra;
	ulong existing_size;

	*add_space_req = (ulong) 0;
	*add_space_alloced = (ulong) 0;
	LL_WALK(slhandle, slcurrent, slentry, err) {
		SLEntryextra = (DsrSLEntryExtraData *)slentry->Extra;
		if (slentry->InVFSTab &&
			slentry->State == SLChangeable) {
			/*
			 * failed file systems are marked changeable,
			 * so they fall into here...
			 */
			if (DsrSLValidFinalSize(
				SLEntryextra->history.final_size,
				&slentry->Size)) {

				/* final size string is a valid number */

				DsrSLEntryGetAttr(slentry,
					DsrSLAttrExistingSize, &existing_size,
					NULL);
				if (slentry->Size > existing_size) {
					*add_space_req +=
						(slentry->Size - existing_size);
				} else {
					*add_space_alloced +=
						(existing_size - slentry->Size);
				}
			}
		} else if (!slentry->InVFSTab &&
			slentry->State == SLAvailable) {
			DsrSLEntryGetAttr(slentry,
				DsrSLAttrExistingSize, &existing_size,
				NULL);
			*add_space_alloced += existing_size;

		} if (slentry->InVFSTab &&
			slentry->State == SLCollapse) {
			DsrSLEntryGetAttr(slentry,
				DsrSLAttrExistingSize, &existing_size,
				NULL);
			*add_space_alloced += existing_size;
		}
	}
	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}
	return (SUCCESS);
}

/*
 * Function:	DsrSLValidFinalSize
 * Description:
 *	Check that the final_size_str passed in is a valid integer.
 *	If it is, then convert it to kb and return it in the final_size
 *	param.
 * Scope:	PUBLIC
 * Parameters:
 *	char *final_size_str:
 *		string the user typed in for a final size field (in MB).
 *		that needs validating.
 *	ulong *final_size:
 *		final_size_str is converted from MB to KB and returned
 *		in here.
 * Return:
 *	TRUE
 *	FALSE
 * Globals:
 * Notes:
 *	- final_size_str is in MB.
 *	- final_size returned is in KB
 */
int
DsrSLValidFinalSize(char *final_size_str, ulong *final_size)
{
	int scan_ret;
	ulong size;

	scan_ret = sscanf(final_size_str, "%lu", &size);
	if (scan_ret != 1) {
		/* scan conversion failed */
		if (final_size)
			*final_size = (ulong) 0;
		return (FALSE);
	}
	if (final_size)
		*final_size = mb_to_kb(size);

	return (TRUE);
}

/*
 * Function:	DsrHowSliceChanged
 * Description:
 *	Figure out how the slice changed from it's original state, to
 *	the state after an autolayout has succeeded during DSR.
 *	We want to know things like if its size changed, or it's
 *	location changed, etc.
 * Scope:	PUBLIC
 * Parameters:
 *	Disk_t *new_dp:
 *		disk the slice entry is now on
 *	int new_slice:
 *		slice number the slice entry is now on
 *	Disk_t **orig_dp:
 *		ptr in which to return the original disk the slice was on
 *	int *orig_slice:
 *		ptr in which to return the original slice number the slice
 *		was on
 * Return:
 *	ulong:
 *		bitmask value indicating how the slice has changed
 *		consisting of the following bitmask values:
 *			SliceChange_Nothing_mask
 *			SliceChange_Size_mask
 *			SliceChange_Slice_mask
 *			SliceChange_Unused_mask
 *			SliceChange_Collapsed_mask
 *			SliceChange_Deleted_mask
 *			SliceChange_Created_mask
 * Globals:
 * Notes:
 */
ulong
DsrHowSliceChanged(Disk_t *new_dp, int new_slice,
	Disk_t **orig_dp, int *orig_slice)
{
	ulong mask = 0;
	SliceKey *slice_key;
	char orig_slice_name[16];
	char new_slice_name[16];
	ulong new_size;
	ulong orig_size;
	Units_t units;

	if (!orig_dp || !orig_slice)
		return (mask);

	/* get the original disk information */
	slice_key = SliceobjFindUse(CFG_COMMIT, NULL,
		Sliceobj_Use(CFG_CURRENT, new_dp, new_slice),
		Sliceobj_Instance(CFG_CURRENT, new_dp, new_slice),
		1);
	if (!slice_key) {
		/*
		 * no such entry in original list - so this was newly
		 * created
		 */
		mask = SliceChange_Created_mask;
		return (mask);
	}

	/*
	 * there was such an entry in the original list,
	 * so do some comparisons on them.
	 */
	*orig_dp = slice_key->dp;
	*orig_slice = slice_key->slice;

	(void) strcpy(orig_slice_name,
		make_slice_name(disk_name(*orig_dp), *orig_slice));
	(void) strcpy(new_slice_name,
		make_slice_name(disk_name(new_dp), new_slice));

	if (!streq(orig_slice_name, new_slice_name))
		mask |= SliceChange_Slice_mask;

	units = get_units();
	set_units(D_KBYTE);
	orig_size = blocks2size(*orig_dp,
		Sliceobj_Size(CFG_COMMIT, *orig_dp, *orig_slice), ROUNDDOWN);
	new_size = blocks2size(new_dp,
		Sliceobj_Size(CFG_CURRENT, new_dp, new_slice), ROUNDDOWN);
	if (orig_size != new_size)
		mask |= SliceChange_Size_mask;
	set_units(units);

	/*
	 * if no other changes detected so far,
	 * then mark as nothing changed.
	 */
	if (!mask)
		mask = SliceChange_Nothing_mask;

	return (mask);
}

/*
 * Function:	DsrHowSliceChangedStr
 * Description:
 *	Take a bitmask value indicating how a slice entry has changed
 *	via DSR and return a string describing the change for use in the
 *	interactive apps File System Modifications Summary screen
 * Scope:	PUBLIC
 * Parameters:
 *	bitmask:	[RO] (ulong)
 *		bitmask value indicating how a slice entry has changed
 * Return:
 *	string describing the change
 *	string is static storage
 * Globals:
 * Notes:
 */
char *
DsrHowSliceChangedStr(ulong mask)
{
	char *str = NULL;

	if (mask & SliceChange_Nothing_mask) {
		str = LABEL_DSR_FSSUMM_NOCHANGE;
	} else if (mask & SliceChange_Unused_mask) {
		str = LABEL_DSR_FSSUMM_UNUSED;
	} else if (mask & SliceChange_Collapsed_mask) {
		str = LABEL_DSR_FSSUMM_COLLAPSED;
	} else if (mask & SliceChange_Deleted_mask) {
		str = LABEL_DSR_FSSUMM_DELETED;
	} else if (mask & SliceChange_Created_mask) {
		str = LABEL_DSR_FSSUMM_CREATED;
	} else if ((mask & SliceChange_Size_mask) ||
		(mask & SliceChange_Slice_mask)) {
		str = LABEL_DSR_FSSUMM_CHANGED;
	}

	return (str);
}

/*
 * Function:	DsrSLGetSlice
 * Description:
 *	Look for a slice name in the slice list that
 *	matches the one given and return the
 *	matching slice list entry.
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 * Return:
 * Globals:
 * Notes:
 */
TSLEntry *
DsrSLGetSlice(TList slhandle, char *slicename)
{
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;

	if (!slicename)
		return (NULL);

	LL_WALK(slhandle, slcurrent, slentry, err) {
		/* check by slice name */
		if (streq(slentry->SliceName, basename(slicename)))
			return (slentry);
	}

	/*
	 * There is no reason to check the return code from walking
	 * the slice list, since no matter whether we error'd or could
	 * not find the info, both are errors.
	 */

	return (NULL);
}

/*
 * Function: DsrSLResetSpaceIgnoreEntries
 * Description:
 *	get the sw lib space structure FS_IGNORE_ENTRY settings
 *	for each entry in the slice list back in sync with the
 *	state settings in the slice list.
 *	(i.e. any SLCollapse slice entry should have FS_IGNORE_ENTRY set
 *	and any non-SLCollapse slice entry should not have
 *	FS_IGNORE_ENTRY set)
 *
 *	This is useful for the interactives, which munge with the
 *	FS_IGNORE_ENTRY settings while the user is toggling around
 *	in them. If they do that and then cancel the operation, we set
 *	them back to match the slice list state settings.
 * Scope:	PUBLIC
 * Parameters:
 *	slhandle:	[RW] (TList)
 *		slice list (linked list) handle
 * Return:
 *	SUCCESS
 *	FAILURE
 * Globals: none
 * Notes:
 */
int
DsrSLResetSpaceIgnoreEntries(TList slhandle)
{
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;

	LL_WALK(slhandle, slcurrent, slentry, err) {
		if (!SL_SLICE_IS_COLLAPSEABLE(slentry))
			continue;

		if (slentry->State == SLCollapse)
			slentry->Space->fsp_flags |= FS_IGNORE_ENTRY;
		else
			slentry->Space->fsp_flags &= ~FS_IGNORE_ENTRY;
	}

	/*
	 * Check the return code from walking the slice list.  If it is not
	 * an expected error result then return an error
	 */

	if (err != LLEndOfList && err != LLListEmpty && err != LLSuccess) {
		return (FAILURE);
	}
	return (SUCCESS);
}

/*
 * Function:	DsrSLUIRenameUnnamedSlices
 * Description:
 *	Convert any "unnamed" slices to a the unnamed string for
 *	presentation in the UI's.
 *	i.e. mount point names in the slice list that are empty
 *	are dislayed as "(unnamed)" so they are more obviously
 *	tagged...
 *	If the name is not null or empty, then leave it alone.
 *	Otherwise, rename it.
 * Scope:	PUBLIC
 * Parameters:
 *	ptr to the string we want to place the final name in
 *	(if the string is renamed, then the storage is dynamic)
 * Return:	none
 * Globals:
 * Notes:
 */
void
DsrSLUIRenameUnnamedSlices(char **str)
{
	if (!str)
		return;

	if (*str && strlen(*str))
		return;

	*str = xstrdup(APP_FS_UNNAMED);
}
