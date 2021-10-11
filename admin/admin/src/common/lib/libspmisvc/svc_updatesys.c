/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */

#ifndef lint
#pragma ident "@(#)svc_updatesys.c 1.26 96/10/10 SMI"
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mntent.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/vfstab.h>
#include "spmisvc_lib.h"
#include "spmicommon_api.h"
#include "spmistore_lib.h"
#include "svc_strings.h"

/* public prototypes */

TSUError	SystemUpdate(TSUData *SUData);
char *		SUGetErrorText(TSUError);

/* private prototypes */

static TSUError	SUInstall(TSUInitialData *Data);
static TSUError	SUUpgrade(OpType Operation, TSUUpgradeData *Data);
static TSUError	SUUpgradeAdaptive(TSUUpgradeAdaptiveData *Data);
static TSUError	SUUpgradeRecover(TSUUpgradeRecoveryData *Data);

/* external prototypes */

/* ---------------------- public functions ----------------------- */

/*
 * *********************************************************************
 * FUNCTION NAME: SystemUpdate
 *
 * DESCRIPTION:
 *  This function is responsible for actually manipulating the system
 *  resources achieve the desired configuration.  This function has four
 *  distinct modes, Initial Install, Upgrade, Adaptive Upgrade and
 *  Upgrade recovery.  For each of these modes the calling application
 *  provides callbacks to keep the calling application informed of the
 *  progress of the processing.
 *
 * RETURN:
 *  TYPE			DESCRIPTION
 *  TSUError			The value of SUSuccess is returned upon
 *				successful completion.  Upon failure the
 *				appropriate error code will be returned that
 *				describes the encountered failure.
 *
 * PARAMETERS:
 *  TYPE			DESCRIPTION
 *  TSUData *			A pointer to the SystemUpdate data structure.
 *				The Operation field of this structure dictates
 *				how this function will behave.  Based on the
 *				value for the Operation, the appropriate union
 *				must be populated.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TSUError
SystemUpdate(TSUData *SUData)
{
	TSUError	SUError;

	switch (SUData->Operation) {
	    case SI_INITIAL_INSTALL:
		/*
		 * Print message indicating system preparation beginning
		 */
		write_status(SCR, LEVEL0, MSG0_SU_INITIAL_INSTALL);

		/*
		 * Process an initial install
		 */
		if ((SUError = SUInstall(&SUData->Info.Initial)) != SUSuccess)
			return (SUError);

		/*
		 * Print message indicating system modification is complete
		 */
		write_status(SCR, LEVEL0, MSG0_SU_INITIAL_INSTALL_COMPLETE);
		break;

	    case SI_UPGRADE:	/* normal (non-adaptive) upgrade */
		/*
		 * Print message indicating system preparation beginning
		 */
		write_status(SCR, LEVEL0, MSG0_SU_UPGRADE);

		/*
		 * Process a standard upgrade
		 */
		if ((SUError = SUUpgrade(SI_UPGRADE,
		    &SUData->Info.Upgrade)) != SUSuccess) {
			return (SUError);
		}

		/*
		 * Print message indicating system modification is complete
		 */
		write_status(SCR, LEVEL0, MSG0_SU_UPGRADE_COMPLETE);
		break;

	    case SI_ADAPTIVE:	/* adaptive upgrade */
		/*
		 * Print message indicating system preparation beginning
		 */
		write_status(SCR, LEVEL0, MSG0_SU_UPGRADE);

		/*
		 * Process an adaptive upgrade
		 */
		if ((SUError =
		    SUUpgradeAdaptive(&SUData->Info.AdaptiveUpgrade)) !=
		    SUSuccess) {
			return (SUError);
		}

		/*
		 * Print message indicating system modification is complete
		 */
		write_status(SCR, LEVEL0, MSG0_SU_UPGRADE_COMPLETE);
		break;

	    case SI_RECOVERY:	/* recovering from a previous */
		/*
		 * Print message indicating system preparation beginning
		 */
		write_status(SCR, LEVEL0, MSG0_SU_UPGRADE);

		/*
		 * Process an upgrade recovery
		 */
		if ((SUError =
		    SUUpgradeRecover(&SUData->Info.UpgradeRecovery)) !=
		    SUSuccess) {
			return (SUError);
		}

		/*
		 * Print message indicating system modification is complete
		 */
		write_status(SCR, LEVEL0, MSG0_SU_UPGRADE_COMPLETE);
		break;

	    default:	/* Don't know what it is, punt */
		write_notice(ERRMSG, SUGetErrorText(SUInvalidOperation));
		return (SUInvalidOperation);
	}

	/* sync out the disks (precautionary) */
	sync();

	return (SUSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: SUGetErrorText
 *
 * DESCRIPTION:
 *  This function converts the given error code into an
 *  internationalized human readable string.
 *
 * RETURN:
 *  TYPE			DESCRIPTION
 *  char *			The internationalized error string
 *				corresponding to the provided error
 *				code.
 *
 * PARAMETERS:
 *  TYPE			DESCRIPTION
 *  TSUError			The error code to convert.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

char *
SUGetErrorText(TSUError SUError)
{
	switch (SUError) {
	case SUSuccess:
		return (MSG0_SU_SUCCESS);
	case SUInvalidOperation:
		return (MSG0_SU_INVALID_OPERATION);
	case SUResetStateError:
		return (MSG0_SU_STATE_RESET_FAILED);
	case SUCreateMountListError:
		return (MSG0_SU_MNTPNT_LIST_FAILED);
	case SUSetupDisksError:
		return (MSG0_SU_SETUP_DISKS_FAILED);
	case SUMountFilesysError:
		return (MSG0_SU_MOUNT_FILESYS_FAILED);
	case SUSetupSoftwareError:
		return (MSG0_SU_PKG_INSTALL_TOTALFAIL);
	case SUSetupVFSTabError:
		return (MSG0_SU_VFSTAB_CREATE_FAILED);
	case SUSetupVFSTabUnselectedError:
		return (MSG0_SU_VFSTAB_UNSELECTED_FAILED);
	case SUSetupHostsError:
		return (MSG0_SU_HOST_CREATE_FAILED);
	case SUSetupHostIDError:
		return (MSG0_SU_SERIAL_VALIDATE_FAILED);
	case SUSetupDevicesError:
		return (MSG0_SU_SYS_DEVICES_FAILED);
	case SUSetupBootBlockError:
		return (MSG0_SU_BOOT_BLOCK_FAILED);
	case SUSetupBootPromError:
		return (MSG0_SU_PROM_UPDATE_FAILED);
	case SUUpgradeScriptError:
		return (MSG0_SU_UPGRADE_SCRIPT_FAILED);
	case SUDiskListError:
		return (MSG0_SU_DISKLIST_READ_FAILED);
	case SUDSRALCreateError:
		return (MSG0_SU_DSRAL_CREATE_FAILED);
	case SUDSRALArchiveBackupError:
		return (MSG0_SU_DSRAL_ARCHIVE_BACKUP_FAILED);
	case SUDSRALArchiveRestoreError:
		return (MSG0_SU_DSRAL_ARCHIVE_RESTORE_FAILED);
	case SUDSRALDestroyError:
		return (MSG0_SU_DSRAL_DESTROY_FAILED);
	case SUUnmountError:
		return (MSG0_SU_UNMOUNT_FAILED);
	case SUFileCopyError:
		return (MSG0_SU_FILE_COPY_FAILED);
	case SUFatalError:
		return (MSG0_SU_FATAL_ERROR);
	default:
		return (MSG0_SU_UNKNOWN_ERROR_CODE);
	}
}

/* ---------------------- private functions ----------------------- */

/*
 * *********************************************************************
 * FUNCTION NAME: SUInstall
 *
 * DESCRIPTION:
 *
 * RETURN:
 *  TYPE			DESCRIPTION
 *  TSUError			The value of SUSuccess is returned upon
 *				successful completion.  Upon failure the
 *				appropriate error code will be returned that
 *				describes the encountered failure.
 *
 * PARAMETERS:
 *  TYPE			DESCRIPTION
 *  TSUInitialData *		A pointer to the Initial Install data
 *				structure.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TSUError
SUInstall(TSUInitialData *Data)
{
	Vfsent		*vlist = NULL;
	MachineType	mt = get_machinetype();
	Disk_t *	dlist = first_disk();
	TransList	*trans;

	/*
	 * cleanup from possible previous install attempts for
	 * all install processes which could be restarted
	 */

	if (INDIRECT_INSTALL) {
		if (reset_system_state() < 0) {
			write_notice(ERRMSG, SUGetErrorText(SUResetStateError));
			return (SUResetStateError);
		}
	}

	/*
	 * create a list of local and remote mount points
	 */

	if (_create_mount_list(dlist,
	    Data->cfs,
	    &vlist) == ERROR) {
		write_notice(ERRMSG, SUGetErrorText(SUCreateMountListError));
		return (SUCreateMountListError);
	}

	/*
	 * update the F-disk and VTOC on all selected disks
	 * according to the disk list configuration; start
	 * swapping to defined disk swap slices
	 */

	if (_setup_disks(dlist, vlist) == ERROR) {
		write_notice(ERRMSG, SUGetErrorText(SUSetupDisksError));
		return (SUSetupDisksError);
	}

	/*
	 * create mount points on the target system according
	 * to the mount list; note that the target file systems
	 * may be offset of a base directory if the installation
	 * is indirect.
	 */

	if (_mount_filesys_all(SI_INITIAL_INSTALL, vlist) != NOERR) {
		write_notice(ERRMSG, SUGetErrorText(SUMountFilesysError));
		return (SUMountFilesysError);
	}

	/*
	 * lock critical applications in memory for performance
	 */

	(void) _lock_prog("/usr/sbin/pkgadd");
	(void) _lock_prog("/usr/sadm/install/bin/pkginstall");
	(void) _lock_prog("/usr/bin/cpio");

	/*
	 * create the packaging admin file to be used for all
	 * package adds (software related task). Also create
	 * the transfer file list, used to move necessary files
	 * from /tmp/root to /a
	 */

	if (_setup_software(Data->prod,
	    &trans,
	    Data->SoftUpdateCallback,
	    Data->ApplicationData) == ERROR) {
		write_notice(ERRMSG, SUGetErrorText(SUSetupSoftwareError));
		return (SUSetupSoftwareError);
	}

	write_status(LOGSCR, LEVEL0, MSG0_SU_FILES_CUSTOMIZE);

	/*
	 * write out the 'etc/vfstab' file to the appropriate
	 * location.  This is a writeable system file (affects
	 * location wrt indirect installs)
	 */

	if (_setup_vfstab(SI_INITIAL_INSTALL, &vlist) == ERROR) {
		write_notice(ERRMSG, SUGetErrorText(SUSetupVFSTabError));
		return (SUSetupVFSTabError);
	}

	/*
	 * write out the vfstab.unselected file to the appropriate
	 * location (if unselected disks with file systems exist)
	 */

	if (_setup_vfstab_unselect() == ERROR) {
		write_notice(ERRMSG,
		    SUGetErrorText(SUSetupVFSTabUnselectedError));
		return (SUSetupVFSTabUnselectedError);
	}

	/*
	 * set up the etc/hosts file. This is a writeable system
	 * file (affects location wrt indirect installs)
	 */

	if (_setup_etc_hosts(Data->cfs) == ERROR) {
		write_notice(ERRMSG, SUGetErrorText(SUSetupHostsError));
		return (SUSetupHostsError);
	}

	/*
	 * initialize serial number if there isn't one supported on the
	 * target architecture
	 */

	if (_setup_hostid() == ERROR) {
		write_notice(ERRMSG, SUGetErrorText(SUSetupHostIDError));
		return (SUSetupHostIDError);
	}

	/*
	 * Copy information from tmp/root to real root, using the
	 * transfer file list. From this point on, all modifications
	 * to the system will have to write directly to the /a/<*>
	 * directories. (if applicable)
	 */

	if (_setup_tmp_root(&trans) == ERROR) {
		write_notice(ERRMSG, SUGetErrorText(SUFileCopyError));
		return (SUFileCopyError);
	}

	/*
	 * setup /dev, /devices, and /reconfigure
	 */

	if (_setup_devices() == ERROR) {
		write_notice(ERRMSG, SUGetErrorText(SUSetupDevicesError));
		return (SUSetupDevicesError);
	}

	/*
	 * set up boot block
	 */

	if (_setup_bootblock() != NOERR) {
		write_notice(ERRMSG, SUGetErrorText(SUSetupBootBlockError));
		return (SUSetupBootBlockError);
	}

	/*
	 * update the booting PROM if necessary; if the update
	 * fails, print a warning and update the boot object
	 * updateability fields for all states
	 */

	if (SystemConfigProm() != NOERR) {
		write_notice(WARNMSG, SUGetErrorText(SUSetupBootPromError));
		(void) BootobjSetAttributePriv(CFG_CURRENT,
			BOOTOBJ_PROM_UPDATEABLE,  FALSE,
			NULL);
		(void) BootobjSetAttributePriv(CFG_COMMIT,
			BOOTOBJ_PROM_UPDATEABLE,  FALSE,
			NULL);
		(void) BootobjSetAttributePriv(CFG_EXIST,
			BOOTOBJ_PROM_UPDATEABLE,  FALSE,
			NULL);
	}

	/*
	 * 3rd party driver installation
	 *
	 * NOTE:	this needs to take 'bdir' as an argument
	 *		if it intends to write directly into the
	 *		installed image.
	 */

	if (access("/tmp/diskette_rc.d/inst9.sh", X_OK) == 0) {
		write_status(LOGSCR, LEVEL0, MSG0_SU_DRIVER_INSTALL);
		(void) system("/sbin/sh /tmp/diskette_rc.d/inst9.sh");
	}

	/*
	 * copy status file to /var/sadm or /tmp
	 */

	(void) _setup_install_log();

	/*
	 * wait for newfs's and fsck's to complete
	 */

	if (!GetSimulation(SIM_EXECUTE)) {
		while (ProcWalk(ProcIsRunning, "newfs") == 1 ||
				ProcWalk(ProcIsRunning, "fsck") == 1)
			(void) sleep(5);
	}

	/*
	 * on non-AutoClient systems, finish mounting all file systems
	 * which were not previously mounted during install; this
	 * leaves the system correctly configured for a finish script
	 */

	if (mt != MT_CCLIENT) {
		if (GetSimulation(SIM_EXECUTE) || get_trace_level() > 1)
			write_status(SCR, LEVEL0, MSG0_SU_MOUNTING_TARGET);

		if (_mount_remaining(vlist) != NOERR) {
			write_notice(ERRMSG,
			    SUGetErrorText(SUMountFilesysError));
			return (SUMountFilesysError);
		}
	}

	/*
	 * cleanup
	 */

	_free_mount_list(&vlist);
	return (SUSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: SUUpgrade
 *
 * DESCRIPTION:
 *
 * RETURN:
 *  TYPE			DESCRIPTION
 *  TSUError			The value of SUSuccess is returned upon
 *				successful completion.  Upon failure the
 *				appropriate error code will be returned that
 *				describes the encountered failure.
 *
 * PARAMETERS:
 *  TYPE			DESCRIPTION
 *  TSUUpgradeData *		A pointer to the Upgrade data structure.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TSUError
SUUpgrade(OpType Operation, TSUUpgradeData *Data)
{

	/*
	 * If we are not running in simulation mode
	 */

	if (!GetSimulation(SIM_EXECUTE)) {

		/*
		 * lock critical applications in memory for performance
		 */

		(void) _lock_prog("/usr/sbin/pkgadd");
		(void) _lock_prog("/usr/sadm/install/bin/pkginstall");
		(void) _lock_prog("/usr/bin/cpio");

		/*
		 * If we are not running in simulation mode then go execute
		 * the upgrade script.
		 */

		if (execute_upgrade(Operation,
		    Data->ScriptCallback,
		    Data->ScriptData)) {
			write_notice(ERRMSG,
			    SUGetErrorText(SUUpgradeScriptError));
			return (SUUpgradeScriptError);
		}

		/*
		 * If we are not running in simulation mode copy log
		 * file to /var/sadm
		 */

		(void) _setup_install_log();
	}

	return (SUSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: SUUpgradeAdaptive
 *
 * DESCRIPTION:
 *
 * RETURN:
 *  TYPE			DESCRIPTION
 *  TSUError			The value of SUSuccess is returned upon
 *				successful completion.  Upon failure the
 *				appropriate error code will be returned that
 *				describes the encountered failure.
 *
 * PARAMETERS:
 *  TYPE			DESCRIPTION
 *  TSUUpgradeAdaptiveData *	A pointer to the Adaptive Upgrade data
 *				structure.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TSUError
SUUpgradeAdaptive(TSUUpgradeAdaptiveData *Data)
{
	TSUUpgradeData	UpgradeData;

	Vfsent		*vlist = NULL;
	Disk_t *	dlist = first_disk();
	Disk_t *	dp;
	char		buf[MAXPATHLEN];

	TDSRArchiveList ArchiveList;
	TSUError	SUError;

	TDSRALError	DSRALError;

	/*
	 * If we are not running in simulation mode then we need to
	 * create an instance of the DSR archive list.
	 */

	if (!GetSimulation(SIM_EXECUTE)) {

		if ((DSRALError = DSRALCreate(&ArchiveList))) {
			write_notice(ERRMSG,
			    DSRALGetErrorText(DSRALError));
			write_notice(ERRMSG,
			    SUGetErrorText(SUDSRALCreateError));
			return (SUDSRALCreateError);
		}

		/*
		 * Backup the archive to the specified media
		 */

		if ((DSRALError = DSRALArchive(ArchiveList,
		    DSRALBackup,
		    Data->ArchiveCallback,
		    Data->ArchiveData))) {
			write_notice(ERRMSG,
			    DSRALGetErrorText(DSRALError));
			write_notice(ERRMSG,
			    SUGetErrorText(SUDSRALArchiveBackupError));
			return (SUDSRALArchiveBackupError);
		}
	}

	/*
	 * Read the Disk list from the backup file generated by the
	 * child process.
	 */

	if (ReadDiskList(&dlist)) {
		write_notice(ERRMSG, SUGetErrorText(SUDiskListError));
		return (SUDiskListError);
	}

	/*
	 * If tracing is enabled
	 */

	if (get_trace_level() > 2) {
		write_status(SCR, LEVEL0,
		    "Disk list read from child process");
		WALK_LIST(dp, dlist) {
			print_disk(dp, NULL);
		}
	}

	/*
	 * create a list of local and remote mount points
	 */

	if (_create_mount_list(dlist, NULL, &vlist) == ERROR) {
		write_notice(ERRMSG, SUGetErrorText(SUCreateMountListError));
		return (SUCreateMountListError);
	}

	/*
	 * If tracing is enabled then dump the mount list
	 */

	if (get_trace_level() > 2) {
		write_status(SCR, LEVEL0, "New entries for the vfstab");
		_mount_list_print(&vlist);
	}

	write_status(LOGSCR, LEVEL0, MSG0_SU_FILES_CUSTOMIZE);

	/*
	 * write out the 'etc/vfstab' file to the appropriate location
	 * This is a writeable system file (affects location wrt
	 * indirect installs)
	 */

	if (_setup_vfstab(SI_ADAPTIVE, &vlist) == ERROR) {
		write_notice(ERRMSG, SUGetErrorText(SUSetupVFSTabError));
		return (SUSetupVFSTabError);
	}

	/*
	 * If tracing is enabled then dump the mount list
	 */

	if (get_trace_level() > 2) {
		write_status(LOGSCR, LEVEL0, "The merged vfstab:");
		CatFile("/tmp/vfstab",
		    LOGSCR, STATMSG, LEVEL1);
	}

	/*
	 * If we are not running in simulation mode
	 */

	if (!GetSimulation(SIM_EXECUTE)) {

		/*
		 * Ok, the required files have been archived to
		 * the media so unmount the file systems in
		 * preperation for laying down the new file
		 * system layout.
		 */

		if (umount_and_delete_swap()) {
			write_notice(ERRMSG,
			    SUGetErrorText(SUUnmountError));
			return (SUUnmountError);
		}

		/*
		 * update the F-disk and VTOC on all selected disks
		 * according to the disk list configuration; start
		 * swapping to defined disk swap slices
		 */

		if (_setup_disks(dlist, vlist) == ERROR) {
			write_notice(ERRMSG,
			    SUGetErrorText(SUSetupDisksError));
			return (SUSetupDisksError);
		}

		/*
		 * wait for newfs's and fsck's to complete
		 */

		if (!GetSimulation(SIM_EXECUTE)) {
			while (ProcWalk(ProcIsRunning, "newfs") == 1 ||
			    ProcWalk(ProcIsRunning, "fsck") == 1)
				(void) sleep(5);
		}

		/*
		 * Sort the vfstab list prior to mounting it to
		 * insure that parent gets mounted prior to a
		 * dependent child
		 */

		_mount_list_sort(&vlist);

		/*
		 * Mount all of the slices in the new file system.
		 */

		if (_mount_filesys_all(SI_ADAPTIVE, vlist) !=
		    NOERR) {
			write_notice(ERRMSG,
			    SUGetErrorText(SUMountFilesysError));
			return (SUMountFilesysError);
		}

		/*
		 * If we are in simulation mode or tracing is enabled
		 */

		if (GetSimulation(SIM_EXECUTE) || get_trace_level() > 1)
			write_status(SCR, LEVEL0, MSG0_SU_MOUNTING_TARGET);

		/*
		 * Mount all of the file systems that may have been
		 * newfs'd in the back ground.
		 */

		if (_mount_remaining(vlist) != NOERR) {
			write_notice(ERRMSG,
			    SUGetErrorText(SUMountFilesysError));
			return (SUMountFilesysError);
		}

		/*
		 * Check to see if the destination directory exists
		 * and if not create it.
		 */

		(void) sprintf(buf, "%s/etc", get_rootdir());

		if (access(buf, X_OK) != 0) {
			if (_create_dir(buf) != NOERR) {
				write_notice(ERRMSG,
				    SUGetErrorText(SUCreateDirectoryError));
				return (SUCreateDirectoryError);
			}
		}

		/*
		 * Copy the merged vfstab from the temporary location
		 * into the real location
		 */

		(void) sprintf(buf, "%s%s", get_rootdir(), VFSTAB);

		if (_copy_file(buf, "/tmp/root/etc/vfstab") == ERROR) {
			write_notice(ERRMSG,
			    SUGetErrorText(SUFileCopyError));
			return (SUFileCopyError);
		}

		/*
		 * Restore the archive from the media.
		 */

		if ((DSRALError = DSRALArchive(ArchiveList,
		    DSRALRestore,
		    Data->ArchiveCallback,
		    Data->ArchiveData))) {
			write_notice(ERRMSG,
			    DSRALGetErrorText(DSRALError));
			write_notice(ERRMSG,
			    SUGetErrorText(SUDSRALArchiveRestoreError));
			return (SUDSRALArchiveRestoreError);
		}

		/*
		 * Destroy the ArchiveList Object.
		 */

		if ((DSRALError = DSRALDestroy(&ArchiveList)))	{
			write_notice(ERRMSG,
			    DSRALGetErrorText(DSRALError));
			write_notice(ERRMSG,
			    SUGetErrorText(SUDSRALDestroyError));
			return (SUDSRALDestroyError);
		}

		/*
		 * cleanup
		 */

		_free_mount_list(&vlist);

		/*
		 * Ok, now this is interesting.  The normal upgrade code
		 * expects that the directory structure for the upgrade
		 * will be in place prior to beginning the upgrade.
		 * However, since the backup logic is optimized to only
		 * archive off those files that have been modified since
		 * their original installation or other user modified
		 * files, we get caught with the Post KBI directories
		 * that upgrade depends on may not exist after the
		 * restore.  So, we go ahead and make them here just in case.
		 * Note: I do not care about return codes since if the
		 *	directories already exist thats fine.
		 */

		MakePostKBIDirectories();

	}

	/*
	 * Upgrade the system
	 */

	UpgradeData.ScriptCallback = Data->ScriptCallback;
	UpgradeData.ScriptData = Data->ScriptData;
	if ((SUError = SUUpgrade(SI_ADAPTIVE, &UpgradeData)) != SUSuccess) {
		return (SUError);
	}

	return (SUSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: SUUpgradeRecover
 *
 * DESCRIPTION:
 *
 * RETURN:
 *  TYPE			DESCRIPTION
 *  TSUError			The value of SUSuccess is returned upon
 *				successful completion.  Upon failure the
 *				appropriate error code will be returned that
 *				describes the encountered failure.
 *
 * PARAMETERS:
 *  TYPE			DESCRIPTION
 *  TSUUpgradeRecoveryData *	A pointer to the upgrade recovery data
 *				structure.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TSUError
SUUpgradeRecover(TSUUpgradeRecoveryData *Data)
{
	TSUUpgradeData	UpgradeData;
	OpType		Operation = SI_RECOVERY;

	TDSRArchiveList ArchiveList;
	TDSRALError	ArchiveError;
	TDSRALMedia	Media;
	char		MediaString[PATH_MAX];
	TSUError	SUError;

	/*
	 * Check to see if we can recover from an interrupted
	 * adaptive upgrade
	 */

	if ((ArchiveError = DSRALCanRecover(&Media, MediaString))) {
		switch (ArchiveError) {

		/*
		 * If we can recover from a interrupted restore
		 */

		case DSRALRecovery:
			break;

		/*
		 * If we hit this path we have a problem
		 */

		default:
			write_notice(ERRMSG,
			    DSRALGetErrorText(ArchiveError));
			write_notice(ERRMSG,
			    SUGetErrorText(SUFatalError));
			return (SUFatalError);
		}
	}

	/*
	 * If we are not running in simulation mode
	 */

	if (!GetSimulation(SIM_EXECUTE)) {

		/*
		 * If we are recovering from a failed restore
		 */

		if (ArchiveError == DSRALRecovery) {

			/*
			 * Create an instance of the DSR Archive
			 * List object for use
			 */

			if ((ArchiveError = DSRALCreate(&ArchiveList))) {
				write_notice(ERRMSG,
				    DSRALGetErrorText(ArchiveError));
				write_notice(ERRMSG,
				    SUGetErrorText(SUDSRALCreateError));
				return (SUDSRALCreateError);
			}

			/*
			 * Restore the archive from the media.
			 */

			if ((ArchiveError = DSRALArchive(ArchiveList,
			    DSRALRestore,
			    Data->ArchiveCallback,
			    Data->ArchiveData))) {
				write_notice(ERRMSG,
				    DSRALGetErrorText(ArchiveError));
				write_notice(ERRMSG,
				    SUGetErrorText(SUDSRALArchiveRestoreError));
				return (SUDSRALArchiveRestoreError);
			}

			/*
			 * Destroy the ArchiveList Object.
			 */

			if ((ArchiveError = DSRALDestroy(&ArchiveList))) {
				write_notice(ERRMSG,
				    DSRALGetErrorText(ArchiveError));
				write_notice(ERRMSG,
				    SUGetErrorText(SUDSRALDestroyError));
				return (SUDSRALDestroyError);
			}
			Operation = SI_UPGRADE;
		}
	}

	/*
	 * Upgrade the system
	 */

	UpgradeData.ScriptCallback = Data->ScriptCallback;
	UpgradeData.ScriptData = Data->ScriptData;
	if ((SUError = SUUpgrade(Operation, &UpgradeData)) != SUSuccess) {
		return (SUError);
	}

	return (SUSuccess);
}
