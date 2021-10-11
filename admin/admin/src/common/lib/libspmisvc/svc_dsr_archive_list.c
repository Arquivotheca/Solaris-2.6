/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */

#ifndef lint
#pragma ident "@(#)svc_dsr_archive_list.c 1.35 96/09/10 SMI"
#endif

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ftw.h>
#include<dirent.h>
#include<unistd.h>
#include<libintl.h>
#include<fcntl.h>
#include<errno.h>
#include<libgen.h>

#include<sys/types.h>
#include<sys/stat.h>
#include<sys/statvfs.h>
#include<sys/utsname.h>

#include "svc_dsr_archive_list_in.h"
#include "spmisoft_lib.h"
#include "svc_strings.h"

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALValidateHandle
 *
 * DESCRIPTION:
 *  This function validates the opaque handle passed in by the calling
 *  application.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, DSRALSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRArchiveList        The archive handle passed in by the
 *                         calling application.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALValidateHandle(TDSRArchiveList Handle)
{
	TDSRALData	*ArchiveData;

	/*
	 * Check to see if the handle is NULL before testing
	 */

	if (Handle == NULL) {
		return (DSRALInvalidHandle);
	}

	/*
	 * Typecast the handle to the internal representation
	 */

	ArchiveData = (TDSRALData *) Handle;

	/*
	 * Check to see if the object has been initialized
	 */

	if (ArchiveData->Initialized != DSRAL_INITIALIZED) {
		return (DSRALInvalidHandle);
	}
	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALCreate
 *
 * DESCRIPTION:
 *  This function creates an instance of the DSR object and associates
 *  a handle with the object.  This function must be called prior to
 *  using any of the DSR API calls.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, DSRALSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRArchiveList *      A pointer to a TDSRArchiveList type.
 *                         Upon successful completion, the
 *                         pointer will point to an
 *                         initialized DSR object handle.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TDSRALError
DSRALCreate(TDSRArchiveList * Handle)
{
	TDSRALData	*ArchiveData;

	/*
	 * Initialize the backup list package
	 */

	ArchiveData = (TDSRALData *) malloc(sizeof (TDSRALData));
	if (ArchiveData == NULL) {
		*Handle = NULL;
		return (DSRALMemoryAllocationFailure);
	}

	/*
	 * Set the handle to point at the internal data structure and set the
	 * initialization flag to true.
	 */

	ArchiveData->Initialized = DSRAL_INITIALIZED;
	ArchiveData->BytesToTransfer = 0;
	ArchiveData->BytesTransfered = 0;
	ArchiveData->Media = DSRALNoMedia;
	ArchiveData->MediaString[0] = '\0';

	*Handle = ArchiveData;

	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALCanRecover
 *
 * DESCRIPTION:
 *  This function determines if the previous run of the DSRAL object
 *  was interrupted during the restore phase.  If the processing was
 *  interrupted, then the data that was used to generate the archive
 *  is returned.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  If the previous run was
 *                         interrupted then DSRALRecovery is returned.
 *                         Otherwise, DSRALSuccess is returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRALMedia *          The media type used on the interrupted
 *                         upgrade.
 *  char *                 The MediaString used on the interrupted
 *                         upgrade.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TDSRALError
DSRALCanRecover(TDSRALMedia* Media, char *MediaString)
{
	char		TmpBuffer1[PATH_MAX];
	TDSRALData	ArchiveData;

	/*
	 * Now, lets go check to see if this is a recovery from a
	 * previous run.
	 */

	(void) strcpy(TmpBuffer1,
	    get_rootdir());
	(void) strcat(TmpBuffer1, DSRAL_RECOVERY_RESTORE_PATH);
	canoninplace(TmpBuffer1);

	if ((DSRALProcessFile(TmpBuffer1,
	    &ArchiveData,
	    True))) {

		/*
		 * Ok, we could not find a valid recovery file so
		 * we're calling it a new run.
		 */

		*Media = DSRALNoMedia;
		MediaString[0] = '\0';
		return (DSRALSuccess);
	}

	/*
	 * We found a valid recovery file so we need to write back
	 * out the control file for use by DSRALArchive().
	 */

	if ((DSRALProcessFile(DSRAL_CONTROL_PATH,
	    &ArchiveData,
	    False))) {
		return (DSRALProcessFileFailure);
	}

	/*
	 * Now, we need to write the recovery file back into the /tmp
	 * location so that the DSRALArchive() function when called
	 * with the DSRALRestore state will operate corectly.
	 */

	if ((DSRALProcessFile(DSRAL_RECOVERY_BACKUP_PATH,
	    &ArchiveData,
	    False))) {
		return (DSRALProcessFileFailure);
	}

	*Media = ArchiveData.Media;
	(void) strcpy(MediaString, ArchiveData.MediaString);

	return (DSRALRecovery);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALDestroy
 *
 * DESCRIPTION:
 *  This function destroys a DSR object.  It cleans up all internally
 *  allocated data structures and invalidates the DSR object handle.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, DSRALSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRArchiveList *      A pointer to a TDSRArchiveList type.
 *                         Upon successful completion, the
 *                         pointer will be set to NULL.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TDSRALError
DSRALDestroy(TDSRArchiveList * Handle)
{
	TDSRALError	ArchiveError;
	TDSRALData	*ArchiveData;

	if ((ArchiveError = DSRALValidateHandle(*Handle))) {
		return (ArchiveError);
	}
	ArchiveData = (TDSRALData *) * Handle;

	free(ArchiveData);

	*Handle = NULL;

	/*
	 * Call the library clean-up function
	 */

	end_upgraded_file_scan();

	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALMount
 *
 * DESCRIPTION:
 *  This function takes in the archive media string and parses it to
 *  construct the mount command necessary for the media.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, DSRALSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRALMedia            Which media is being used.
 *  char *                 The archive command string.  The
 *                         format for each media type is:
 *                           DISK:         /dev/dsk/c0t3d0s0
 *                           REMOTE NFS:   whistler:/home/vos
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALMount(TDSRALMedia Media,
    char *MediaString)
{
	char		User[PATH_MAX];
	char		Machine[PATH_MAX];
	char		Path[PATH_MAX];
	char		Command[PATH_MAX];
	struct stat	StatBuf;

	TDSRALError	ArchiveError;

	/*
	 * Parse the Media String
	 */

	if ((ArchiveError = DSRALParseMediaString(Media,
	    MediaString,
	    User,
	    Machine,
	    Path))) {
		return (ArchiveError);
	}

	/*
	 * Remove any entry in /tmp with the name dsr.	I consider this a
	 * legal thing to do since we are running under the boot image and it
	 * is the /tmp directory.
	 */

	(void) DSRALUnMount();

	/*
	 * blindly unlink the directory.  Again I don't care if this
	 * fails or not.  I am just doing clean-up here.
	 */

	(void) unlink(DSRAL_DIRECTORY_MOUNT_POINT);

	/*
	 * Create the temporary directory to mount the file system
	 */

	if (mkdir(DSRAL_DIRECTORY_MOUNT_POINT,
	    S_IRWXU | S_IRWXG | S_IRWXO)) {
		return (DSRALSystemCallFailure);
	}
	switch (Media) {
	case DSRALDisk:
		canoninplace(Path);
		if (stat(Path, &StatBuf) < 0) {
			return (DSRALUnableToStatPath);
		}
		switch (StatBuf.st_mode & S_IFMT) {
		case S_IFBLK:

			/*
			 * Ok, the string given points to a device so build
			 * up the command to mount the device on /tmp/dsr
			 */

			(void) sprintf(Command,
			    "mount -F ufs %s %s\n",
			    Path,
			    DSRAL_DIRECTORY_MOUNT_POINT);
			break;
		case S_IFDIR:

			/*
			 * The MediaString points to an already mounted
			 * directory, so there is nothing to do.
			 */

			return (DSRALSuccess);
		case S_IFCHR:
		case S_IFREG:
		case S_IFIFO:
		case S_IFLNK:
		case S_IFSOCK:
		case S_IFDOOR:
		default:
			return (DSRALInvalidDiskPath);
		}
		break;
	case DSRALNFS:

		/*
		 * Now run the compiled path through the canonizing function
		 * to remove any redundant characters.
		 */

		canoninplace(Path);

		(void) sprintf(Command,
		    "mount -F nfs %s:%s %s\n",
		    Machine,
		    Path,
		    DSRAL_DIRECTORY_MOUNT_POINT);
		break;
	default:
		return (DSRALInvalidMedia);
	}

	if (system(Command)) {
		return (DSRALSystemCallFailure);
	}
	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALUnMount
 *
 * DESCRIPTION:
 *  This function umounts the dsr temporary diresctory.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, DSRALSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  (void)
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALUnMount(void)
{
	char		Command[PATH_MAX];

	/*
	 * Unmount the directory
	 */

	(void) sprintf(Command,
	    "umount %s 2>/dev/null\n",
	    DSRAL_DIRECTORY_MOUNT_POINT);
	if (system(Command)) {
		return (DSRALSystemCallFailure);
	}

	/*
	 * Unlink the temporary directory entry
	 */

	if (unlink(DSRAL_DIRECTORY_MOUNT_POINT)) {
		return (DSRALSystemCallFailure);
	}
	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALValidateMedia
 *
 * DESCRIPTION:
 *  This function takes in the media string and performs a validation.
 *  Upon successful validation, DSRALSuccess is returned.  Upon failure
 *  the error code specific to the error is returned.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, DSRALSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TList                  The SliceList containing all of the slices
 *                          on the system.  This is used to validate
 *                          that the supplied media is not a local slice
 *                          that will be destroyed as a part of upgrade.
 *  TDSRALMedia            Which media is being used.
 *  char *                 The archive command string.  The
 *                         format for each media type is:
 *                          LOCAL FLOPPY: /dev/rdiskette0
 *                          LOCAL TAPE:   /dev/rmt0
 *                          LOCAL DISK:   /export/tmp or /dev/dsk/c0t3d0s0
 *                          REMOTE NFS:   whistler:/home/vos
 *                          REMOTE RSH:   vos@whistler:/home/vos
 *  TBoolean *             If a non NULL pointer is provided then if
 *                         the media is a device (either char or blk)
 *                         then the flag is set to true.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALValidateMedia(TList SliceList,
    TDSRALMedia Media,
    char *MediaString,
    TBoolean *IsDevice)
{
	char		User[PATH_MAX];
	char		Machine[PATH_MAX];
	char		Path[PATH_MAX];
	char		TmpBuffer[PATH_MAX];
	TDSRALError	ArchiveError;

	struct stat	StatBuf;
	TLLError	LLError;
	TLink		CurrentLink;
	TSLEntry	*SLEntry;
	TBoolean	DiskIsFixed;
	char		*CheckPath;
	int		MediaHandle;

	/*
	 * Parse the Media String
	 */

	if ((ArchiveError = DSRALParseMediaString(Media,
	    MediaString,
	    User,
	    Machine,
	    Path))) {
		return (ArchiveError);
	}

	/*
	 * Figure out what the media type is to determine the
	 * media specific validation to be performed
	 */

	switch (Media) {
	case DSRALFloppy:
	case DSRALTape:
		canoninplace(Path);
		if (stat(Path, &StatBuf) < 0) {
			return (DSRALUnableToStatPath);
		}
		switch (StatBuf.st_mode & S_IFMT) {
		case S_IFCHR:

			/*
			 * Ok, the string given points to a valid device.
			 */

			if (IsDevice)
				*IsDevice = True;

			/*
			 * Now that we have a valid raw device lets try
			 * to write to it to make sure that it is mounted
			 * and not write protected
			 */

			if (!(MediaHandle = open(Path, O_RDWR | O_DSYNC))) {
				return (DSRALUnableToWriteMedia);
			}

			/*
			 * Write out a test message to the media to make sure that
			 * it is not write protected.  Note that the 0x200 size
			 * is required for the blocking size on a floppy.  For
			 * a tape the size doesen't matter since it is
			 * a streamed device.
			 */

			if (write(MediaHandle, MediaString, 0x200) == -1) {
				return (DSRALUnableToWriteMedia);
			}

			/*
			 * Ok, it worked so close the media and return success
			 */

			(void) close(MediaHandle);

			return (DSRALSuccess);
		case S_IFBLK:
		case S_IFREG:
		case S_IFIFO:
		case S_IFLNK:
		case S_IFSOCK:
		case S_IFDOOR:
		case S_IFDIR:
		default:
			return (DSRALNotCharDevice);
		}
	case DSRALDisk:
		canoninplace(Path);
		if (stat(Path, &StatBuf) < 0) {
			return (DSRALUnableToStatPath);
		}
		switch (StatBuf.st_mode & S_IFMT) {
		case S_IFBLK:

			/*
			 * The first check is to see if the slice specified is
			 * marked as fixed in the slice list.  If not then the
			 * contents of the slice will be destroyed as a part of
			 * the upgrade so the archive cannot be stored on the
			 * slice.
			 */

			LL_WALK(SliceList, CurrentLink, SLEntry, LLError) {
				if (streq(SLEntry->SliceName, basename(Path)) &&
				    SLEntry->State != SLFixed) {
					return (DSRALDiskNotFixed);
				}
			}

			/*
			 * Check the return code from walking the slice list.
			 * If it is not an expected error result then return
			 * an error
			 */

			if (LLError != LLEndOfList &&
			    LLError != LLListEmpty &&
			    LLError != LLSuccess) {
				return (DSRALListManagementError);
			}

			/*
			 * Ok, the string given points to a valid device, now
			 * lets make sure that we can mount it.
			 */

			if ((ArchiveError = DSRALMount(Media,
			    MediaString))) {
				return (DSRALUnableToMount);
			}

			/*
			 * Now, unmount the service.
			 */

			if ((ArchiveError = DSRALUnMount())) {
				return (ArchiveError);
			}

			if (IsDevice)
				*IsDevice = True;
			return (DSRALSuccess);
		case S_IFDIR:

			/*
			 * The first check for a path is to see if the path is
			 * contained on a file system that is not marked as
			 * fixed.  Since the user can provide a path that is
			 * contained within a file system we have to work back
			 * from the complete given path one directory at a
			 * time comparing against the slice list.
			 */

			DiskIsFixed = False;

			/*
			 * If there is a root directory offset appended to the
			 * path
			 */

			if (strncmp(get_rootdir(),
			    Path,
			    strlen(get_rootdir())) == 0) {
				CheckPath = &Path[strlen(get_rootdir())];
			}

			/*
			 * Otherwsie, we are running live so there is nothing to
			 * strip off
			 */

			else {
				CheckPath = Path;
			}

			while (!DiskIsFixed) {
				LL_WALK(SliceList,
				    CurrentLink,
				    SLEntry,
				    LLError) {
					/* check by mount point name */
					if (SLEntry->InVFSTab &&
					    streq(SLEntry->MountPoint,
						CheckPath)) {
						if (SLEntry->State == SLFixed) {
							DiskIsFixed = True;
							break;
						} else {
							return (DSRALDiskNotFixed);
						}
					}
				}

				/*
				 * Check to see if the disk was found and is
				 * set to fixed.  If so then we are done.
				 */

				if (DiskIsFixed) {
					break;
				}

				/*
				 * Otherwise check the error return code
				 */

				else {
					/*
					 * Check the return code from walking
					 * the slice list.  If it is not an
					 * expected error result then return
					 * an error.
					 */

					if (LLError != LLEndOfList &&
					    LLError != LLListEmpty &&
					    LLError != LLSuccess) {
						return (DSRALListManagementError);
					}
				}
				CheckPath = dirname(CheckPath);
				if (*CheckPath == '.') {
					break;
				}
			}

			/*
			 * Check to see if we have permission to read and
			 * write to the directory.
			 */

			if (!(StatBuf.st_mode & S_IRUSR) ||
			    !(StatBuf.st_mode & S_IWUSR)) {
				return (DSRALInvalidPermissions);
			}
			if (IsDevice)
				*IsDevice = False;
			return (DSRALSuccess);
		case S_IFCHR:
		case S_IFREG:
		case S_IFIFO:
		case S_IFLNK:
		case S_IFSOCK:
		case S_IFDOOR:
		default:
			return (DSRALInvalidDiskPath);
		}
	case DSRALNFS:

		/*
		 * Ok, now try and mount the specified service
		 */

		if (DSRALMount(Media,
		    MediaString)) {
			return (DSRALUnableToMount);
		}

		/*
		 * Check to see if we have permission to read and write to
		 * the directory.  I have to re-stat the directory to get
		 * permissions of the remote directory.
		 */

		if (stat(DSRAL_DIRECTORY_MOUNT_POINT, &StatBuf) < 0) {
			return (DSRALUnableToStatPath);
		}

		/*
		 * Now, unmount the service.
		 */

		if ((ArchiveError = DSRALUnMount())) {
			return (ArchiveError);
		}

		/*
		 * Check to see if root would have access to write
		 * to the file system.  Remember, root accesses a
		 * NFS mount with "other" permissions.
		 */

		if (!(StatBuf.st_mode & S_IROTH) ||
		    !(StatBuf.st_mode & S_IWOTH)) {
			return (DSRALInvalidPermissions);
		}

		/*
		 * Looks to be a valid NFS mount.
		 */

		if (IsDevice)
			*IsDevice = False;
		return (DSRALSuccess);
	case DSRALRsh:

		if (IsDevice)
			*IsDevice = False;

		/*
		 * Check to see if the specified directory is
		 * writable.
		 */

		/*
		 * If the user name was given
		 */

		if (strlen(User)) {
			(void) sprintf(TmpBuffer,
			    "ls /tmp | rsh -l %s %s \"cat > %s/.tmp\"\n",
			    User,
			    Machine,
			    Path);
		}

		/*
		 * Otherwise a user name was not given
		 */

		else {
			(void) sprintf(TmpBuffer,
			    "ls /tmp | rsh %s \"cat > %s/.tmp\"\n",
			    Machine,
			    Path);
		}

		/*
		 * Now issue the command to see if the user really has
		 * access to the machine via rsh.
		 */

		if (system(TmpBuffer)) {
			return (DSRALCannotRsh);
		}

		/*
		 * Now check to see if the specified directory is readable
		 */

		/*
		 * If the user name was given
		 */

		if (strlen(User)) {
			(void) sprintf(TmpBuffer,
			    "rsh -l %s %s \"ls %s\" 1>/dev/null 2>/dev/null\n",
			    User,
			    Machine,
			    Path);
		}

		/*
		 * Otherwise a user name was not given
		 */

		else {
			(void) sprintf(TmpBuffer,
			    "rsh %s \"ls %s\" 1>/dev/null 2>/dev/null\n",
			    Machine,
			    Path);
		}

		/*
		 * Now issue the command to see if the user really has
		 * access to the machine via rsh.
		 */

		if (system(TmpBuffer)) {
			return (DSRALCannotRsh);
		}
		return (DSRALSuccess);
	default:
		return (DSRALInvalidMedia);
	}
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALCheckMediaSpace
 *
 * DESCRIPTION:
 *  This function takes in the media to be used for the archive and if
 *  possible determines if it has sufficient space to complete the
 *  generation of the archive.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, DSRALSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRArchiveList        This is the handle to the DSR
 *                         object returned by the call to
 *                         DSRALCreate().
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TDSRALError
DSRALCheckMediaSpace(TDSRArchiveList Handle)
{
	struct statvfs	StatVFSBuf;
	TDSRALData	*ArchiveData;
	TDSRALError	ArchiveError;

	/*
	 * Check to see if the supplied handle is valid
	 */

	if ((ArchiveError = DSRALValidateHandle(Handle))) {
		return (ArchiveError);
	}
	ArchiveData = (TDSRALData *) Handle;

	/*
	 * If the destination media is either NFS or Disk then mount the
	 * media.
	 */

	if (ArchiveData->Media == DSRALNFS ||
	    (ArchiveData->Media == DSRALDisk &&
		ArchiveData->IsDevice == True)) {
		if ((ArchiveError = DSRALMount(ArchiveData->Media,
		    ArchiveData->MediaString))) {
			return (ArchiveError);
		}
	}

	/*
	 * If the media can be checked for sufficient size,
	 * note that this relies on the fact that the media has
	 * already been mounted above.
	 */

	if (ArchiveData->Media == DSRALNFS ||
	    ArchiveData->Media == DSRALDisk) {

		/*
		 * Get the stats on the mounted file system.
		 */

		if (ArchiveData->Media == DSRALDisk &&
		    ArchiveData->IsDevice == False) {
			if (statvfs(ArchiveData->MediaString, &StatVFSBuf)) {
				return (DSRALSystemCallFailure);
			}
		} else {
			if (statvfs(DSRAL_DIRECTORY_MOUNT_POINT, &StatVFSBuf)) {
				return (DSRALSystemCallFailure);
			}
		}

		/*
		 * If the number of bytes free on the file system is less
		 * than the number of bytes to archive then we have an error.
		 */

		if ((StatVFSBuf.f_bfree * StatVFSBuf.f_bsize) <
		    ArchiveData->BytesToTransfer) {
			return (DSRALInsufficientMediaSpace);
		}
	}
	if (ArchiveData->Media == DSRALNFS ||
	    (ArchiveData->Media == DSRALDisk &&
		ArchiveData->IsDevice == True)) {
		if ((ArchiveError = DSRALUnMount())) {
			return (ArchiveError);
		}
	}
	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALBuildCommand
 *
 * DESCRIPTION:
 *  This function takes in the archive media string and parses it to
 *  construct the bourne shell command that is required to carry out the
 *  backup/restore of the archive list to/from the media.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, DSRALSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRALOperation        Whether a backup or restore command
 *                         should be generated.
 *  TDSRALMedia            Which media is being used.
 *  char *                 The archive command string.  The
 *                         format for each media type is:
 *                           LOCAL FLOPPY: /dev/rdiskette0
 *                           LOCAL TAPE:   /dev/rmt0
 *                           LOCAL DISK:   /export/tmp or /dev/dsk/c0t3d0s0
 *                           REMOTE NFS:   whistler:/home/vos
 *                           REMOTE RSH:   vos@whistler:/home/vos
 *  char *                 The command string required to carry
 *                         out the specified archive command
 *                         to the specified media.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALBuildCommand(TDSRALOperation Operation,
    TDSRALMedia Media,
    char *MediaString,
    char *CommandString)
{
	char		User[PATH_MAX];
	char		Machine[PATH_MAX];
	char		Path[PATH_MAX];
	char		TmpBuffer[PATH_MAX];
	struct stat	StatBuf;
	TDSRALError	ArchiveError;
	struct utsname  SysInfo;

	/*
	 * Parse the Media String
	 */

	if ((ArchiveError = DSRALParseMediaString(Media,
	    MediaString,
	    User,
	    Machine,
	    Path))) {
		return (ArchiveError);
	}

	/*
	 * Initialize all of the strings to NULL
	 */

	CommandString[0] = NULL;

	/*
	 * Get the system's info structure so that the name of the
	 * machine can be used to brand the archive to keep from having
	 * name collisions
	 */

	if (uname(&SysInfo) == -1) {
		return (DSRALSystemCallFailure);
	}

	/*
	 * --------------------------->READ THIS <------------------------------
	 * In the following command strings you will notice that I redirect
	 * STDOUT to STDERR in several cases.  I do this because cpio uses
	 * STDOUT for output when reading/writing to a local device (e.g.
	 * tape or floppy) but uses STDERR when embedded in a piped command
	 * where STDOUT is being used to redirect into another program
	 * (e.g. cpio -mocv | compress).  To make the DSRALArchive() function
	 * simpler, I have redirected STDOUT to STDERR in the cases where
	 * STDOUT is being used.  This allows me to only have to read from
	 * STDERR instead of having to multiplex STDOUT and STDERR.
	 * --------------------------->READ THIS <------------------------------
	 */

	switch (Media) {
	case DSRALFloppy:
	case DSRALTape:
		canoninplace(Path);
		switch (Operation) {
		case DSRALBackup:
			(void) sprintf(CommandString,
			    "cat %s | cpio -M \"%s\" -mocvO %s 1>&2\n",
			    DSRAL_ARCHIVE_LIST_PATH,
			    DSRAL_MEDIA_REPLACEMENT_STRING,
			    Path);
			break;
		case DSRALRestore:
			(void) sprintf(CommandString,
			    "cpio -M \"%s\" -dumicvI %s 1>&2\n",
			    DSRAL_MEDIA_REPLACEMENT_STRING,
			    Path);
			break;
		}
		break;
	case DSRALDisk:
		canoninplace(Path);
		if (stat(Path, &StatBuf) < 0) {
			return (DSRALUnableToStatPath);
		}
		switch (StatBuf.st_mode & S_IFMT) {

		/*
		 * If the path is to a block device then it will be
		 * mounted on /tmp/dsr so set up the path to point
		 * over there.
		 */

		case S_IFBLK:
			(void) sprintf(TmpBuffer,
			    "%s/%s.%s.Z",
			    DSRAL_DIRECTORY_MOUNT_POINT,
			    SysInfo.nodename,
			    DSRAL_ARCHIVE_FILE);
			break;

		/*
		 * If the path points to a directory, then
		 * load the Media string into the path.
		 */

		case S_IFDIR:
			(void) sprintf(TmpBuffer,
			    "%s/%s.%s.Z",
			    Path,
			    SysInfo.nodename,
			    DSRAL_ARCHIVE_FILE);
			break;
		case S_IFCHR:
		case S_IFREG:
		case S_IFIFO:
		case S_IFLNK:
		case S_IFSOCK:
		case S_IFDOOR:
		default:
			return (DSRALInvalidDiskPath);
		}

		canoninplace(TmpBuffer);
		switch (Operation) {
		case DSRALBackup:
			(void) sprintf(CommandString,
			    "cat %s | cpio -mocv | compress > %s\n",
			    DSRAL_ARCHIVE_LIST_PATH,
			    TmpBuffer);
			break;
		case DSRALRestore:
			(void) sprintf(CommandString,
			    "zcat %s | cpio -dumicv 1>&2\n",
			    TmpBuffer);
			break;
		}
		break;
	case DSRALNFS:

		/*
		 * Since we are using compress and compress is VERY
		 * unforgiving about a compressed file not having a .Z
		 * extension, I cat on a .Z extension just to be safe.
		 */

		(void) sprintf(TmpBuffer,
		    "%s/%s.%s.Z",
		    DSRAL_DIRECTORY_MOUNT_POINT,
		    SysInfo.nodename,
		    DSRAL_ARCHIVE_FILE);

		switch (Operation) {
		case DSRALBackup:
			(void) sprintf(CommandString,
			    "cat %s | cpio -mocv | compress > %s\n",
			    DSRAL_ARCHIVE_LIST_PATH,
			    TmpBuffer);
			break;
		case DSRALRestore:
			(void) sprintf(CommandString,
			    "zcat %s | cpio -dumicv 1>&2\n",
			    TmpBuffer);
			break;
		}

		break;
	case DSRALRsh:

		/*
		 * Since we are using compress and compress is VERY
		 * unforgiving about a compressed file not having a .Z
		 * extension, I cat on a .Z extension just to be safe.
		 */

		(void) sprintf(TmpBuffer,
		    "%s/%s.%s.Z",
		    Path,
		    SysInfo.nodename,
		    DSRAL_ARCHIVE_FILE);


		/*
		 * Now run the compiled path through the canonizing function
		 * to remove any redundant characters.
		 */

		canoninplace(TmpBuffer);

		/*
		 * If a user name was specified
		 */

		if (strlen(User)) {
			switch (Operation) {
			case DSRALBackup:
				(void) sprintf(CommandString,
				    "cat %s | cpio -mocv | compress | "
				    "rsh -l %s %s \"cat > %s\"\n",
				    DSRAL_ARCHIVE_LIST_PATH,
				    User,
				    Machine,
				    TmpBuffer);
				break;
			case DSRALRestore:
				(void) sprintf(CommandString,
				    "rsh -l %s %s \"zcat %s\" | cpio -dumicv 1>&2\n",
				    User,
				    Machine,
				    TmpBuffer);
				break;
			}
		}

		/*
		 * Otherwise a user name was not given
		 */

		else {
			switch (Operation) {
			case DSRALBackup:
				(void) sprintf(CommandString,
				    "cat %s | cpio -mocv | compress | "
				    "rsh %s \"cat > %s\"\n",
				    DSRAL_ARCHIVE_LIST_PATH,
				    Machine,
				    Path);
				break;
			case DSRALRestore:
				(void) sprintf(CommandString,
				    "rsh %s \"zcat %s\" | cpio -dumicv 1>&2\n",
				    Machine,
				    Path);
				break;
			}
		}
		break;
	default:
		return (DSRALInvalidMedia);
	}

	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALArchive
 *
 * DESCRIPTION:
 *  This function either back's up or restores the archive list
 *  generated by DSRALGenerate();
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, DSRALSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRArchiveList        This is the handle to the DSR
 *                         object returned by the call to
 *                         DSRALCreate().
 *  TDSRALOperation        The operation to be performed
 *                         on the archive list.
 *  TCallback *            A pointer to the callback function
 *                         to be called with updates.
 *  void *                 A pointer to any data that the
 *                         callback wants as the first argument
 *                         when invoked.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TDSRALError
DSRALArchive(TDSRArchiveList Handle,
    TDSRALOperation Operation,
    TCallback *UserCallback,
    void *UserData)
{
	TDSRALStateData StateData;

	TPCFILE		ChildFILE;

	char		TmpBuffer1[PATH_MAX];
	char		TmpBuffer2[PATH_MAX];
	char		FileEntry[PATH_MAX];
	char		LastFile[PATH_MAX];

	TDSRALData	*ArchiveData;
	TDSRALError	ArchiveError;
	TDSRALError	ArchiveError2;

	TBoolean	Done;

	unsigned long	MediaCounter = 1;

	int		TmpInt;

	static unsigned long	LastPercentComplete;

	/*
	 * Check to see if the supplied handle is valid
	 */

	if ((ArchiveError = DSRALValidateHandle(Handle))) {
		return (ArchiveError);
	}
	ArchiveData = (TDSRALData *) Handle;

	/*
	 * Go read the Control file generated by DSRALGenerate() to
	 * retrieve the necessary state information.  The only
	 * reason for this file is to allow the DSRALGenerate()
	 * function to be called in one process while the
	 * DSRALArchive() function is called in another process.
	 */

	if ((DSRALProcessFile(DSRAL_CONTROL_PATH,
	    ArchiveData,
	    True))) {
		return (DSRALProcessFileFailure);
	}

	/*
	 * If this is a backup of the archive, generate the recovery
	 * file so that it can be included in the backup.
	 */

	if (Operation == DSRALBackup) {

		/*
		 * Generate the control file into the /tmp location.  I have
		 * to write it here because the final /a/var/sadm/install
		 * location may get blown away by the disk re-layout.
		 */

		if ((DSRALProcessFile(DSRAL_RECOVERY_BACKUP_PATH,
		    ArchiveData,
		    False))) {
			return (DSRALProcessFileFailure);
		}

	/*
	 * Otherwise this is a restore, so we need to move the recovery
	 * file from the /tmp location to the perminant disk
	 * location so that if we have a system failure we can recover.
	 */

	} else {

		(void) strcpy(TmpBuffer1,
		    get_rootdir());
		(void) strcat(TmpBuffer1, DSRAL_RECOVERY_RESTORE_PATH);
		canoninplace(TmpBuffer1);

		if ((DSRALProcessFile(TmpBuffer1,
		    ArchiveData,
		    False))) {
			return (DSRALProcessFileFailure);
		}
	}

	/*
	 * Call the user's callback with the appropriate begin state
	 */

	if (UserCallback) {
		if (Operation == DSRALBackup) {
			StateData.State = DSRALBackupBegin;
			StateData.Data.BackupBegin.Media = ArchiveData->Media;
			(void) strcpy(StateData.Data.BackupBegin.MediaString,
			    ArchiveData->MediaString);
		} else {
			StateData.State = DSRALRestoreBegin;
			StateData.Data.RestoreBegin.Media = ArchiveData->Media;
			(void) strcpy(StateData.Data.RestoreBegin.MediaString,
			    ArchiveData->MediaString);
		}

		if (UserCallback(UserData, &StateData)) {
			return (DSRALCallbackFailure);
		}

		/*
		 * If the operation is set to restore and the media
		 * is set to either Tape or diskette then we need to
		 * prompt the user to load the media
		 */

		if (Operation == DSRALRestore &&
			(ArchiveData->Media == DSRALTape ||
			ArchiveData->Media == DSRALFloppy)) {
			StateData.State = DSRALNewMedia;
			StateData.Data.NewMedia.Operation = Operation;
			StateData.Data.NewMedia.MediaNumber = MediaCounter;
			StateData.Data.NewMedia.Media = ArchiveData->Media;
			(void) strcpy(StateData.Data.NewMedia.MediaString,
			    ArchiveData->MediaString);
			if (UserCallback(UserData, &StateData)) {
				return (DSRALCallbackFailure);
			}
		}
	}

	/*
	 * If the destination media is either NFS or Disk then mount the
	 * media.
	 */

	if (ArchiveData->Media == DSRALNFS ||
	    (ArchiveData->Media == DSRALDisk &&
		ArchiveData->IsDevice == True)) {
		if ((ArchiveError = DSRALMount(ArchiveData->Media,
		    ArchiveData->MediaString))) {
			return (ArchiveError);
		}
	}

	/*
	 * Build the command to issue in the shell to perform
	 * the backup/restore.
	 */

	if ((ArchiveError = DSRALBuildCommand(Operation,
	    ArchiveData->Media,
	    ArchiveData->MediaString,
	    TmpBuffer1))) {
		return (ArchiveError);
	}

	/*
	 * Create a process control block for the shell process.
	 */

	if (PCCreate(&ArchiveData->PCHandle,
	    "sh",
	    "sh",
	    "-e",
	    NULL)) {
		return (DSRALChildProcessFailure);
	}

	/*
	 * Start the shell process.
	 */

	if (PCStart(ArchiveData->PCHandle)) {
		return (DSRALChildProcessFailure);
	}

	/*
	 * Get the child processes FILE handles so that we can talk to it.
	 */

	if (PCGetFILE(ArchiveData->PCHandle,
	    &ChildFILE)) {

		/*
		 * Clean-up the shell process.
		 */

		if (ArchiveError = DSRALShellCleanup(ArchiveData))
			return (ArchiveError);

		return (DSRALChildProcessFailure);
	}

	/*
	 * Write out the command string to the shell.
	 */

	(void) fputs(TmpBuffer1, ChildFILE.StdIn);
	(void) fflush(ChildFILE.StdIn);
	(void) fclose(ChildFILE.StdIn);

	/*
	 * Initialize necessary variables prior to entering the main
	 * processing loop.
	 */

	ArchiveData->BytesTransfered = 0;
	LastFile[0] = '\0';
	ArchiveData->ReplacementErrorCount = 0;

	/*
	 * While there is data on STDERR.  NOTE: I have built the shell command
	 * actually generates the archive to redirect STDOUT to STDERR so that
	 * I only have to listen on the one stream.  So, If anyone ever modifies
	 * the shell command and removes the redirect, this code is broken since
	 * cpio uses stdout when writing to a local device (ie. tape or floppy)
	 * but uses stderr when piping the output to another program
	 * (ie. cpio -mocv | compress).
	 */

	Done = False;
	while (!Done) {

		/*
		 * Get a line of text from STDERR.
		 */

		if (!fgets(TmpBuffer1,
		    sizeof (TmpBuffer1),
		    ChildFILE.StdErr)) {

			/*
			 * If fgets fails because the
			 * pipe has closed then set done to True
			 * and we're done.
			 */

			if (feof(ChildFILE.StdErr)) {
				Done = True;
				break;
			}
		}

		/*
		 * Otherwise, we got a line of text
		 * so let's go parse it.
		 */

		else {

			/*
			 * Check to see if the line
			 * contains the media
			 * replacement token.
			 */

			if (strstr(TmpBuffer1,
			    DSRAL_MEDIA_REPLACEMENT_TOKEN)) {

				/*
				 * Scan the media number out of the
				 * replacement string generated by cpio.
				 * If the media number is the same as
				 * the previous one then this means that
				 * we had some kind of problem with the
				 * current media (e.g. not loaded,
				 * not correctly formatted...)
				 */

				(void) sscanf(TmpBuffer1,
				    "%s %d",
				    TmpBuffer2,
				    &TmpInt);

				if (TmpInt == MediaCounter) {
					ArchiveData->ReplacementErrorCount++;
				} else {

					MediaCounter = TmpInt;

					/*
					 * If the media is a
					 * floppy, then issue
					 * an eject
					 */

					if (ArchiveData->Media == DSRALFloppy &&
					    IsIsa("sparc")) {
						(void) sprintf(TmpBuffer1,
						    "eject %s\n",
						    ArchiveData->MediaString);
						if (system(TmpBuffer1)) {

							/*
							 * Clean-up the shell process.
							 */

							if (ArchiveError =
							    DSRALShellCleanup(ArchiveData)) {
								return (ArchiveError);
							}
							return (DSRALSystemCallFailure);
						}
					}
				}

				/*
				 * Call the user's
				 * callback
				 */

				StateData.State = DSRALNewMedia;
				StateData.Data.NewMedia.Operation = Operation;
				StateData.Data.NewMedia.MediaNumber = MediaCounter;
				StateData.Data.NewMedia.Media = ArchiveData->Media;
				(void) strcpy(StateData.Data.NewMedia.MediaString,
				    ArchiveData->MediaString);
				if (UserCallback(UserData, &StateData)) {

					/*
					 * Clean-up the shell process.
					 */

					if (ArchiveError =
					    DSRALShellCleanup(ArchiveData)) {
						return (ArchiveError);
					}
					return (DSRALCallbackFailure);
				}

				if (ArchiveError = DSRALSendCommand(Handle,
				    MEDIA_CONTINUE)) {

					/*
					 * Clean-up the shell process.
					 */

					if (ArchiveError2 =
					    DSRALShellCleanup(ArchiveData)) {
						return (ArchiveError2);
					}
					return (ArchiveError);
				}

				/*
				 * Now, call the users callback with the last
				 * file archived so that we force a re-paint
				 * of the percent complete screen.
				 */

				if (Operation == DSRALBackup) {
					StateData.State = DSRALBackupUpdate;
				} else {
					StateData.State = DSRALRestoreUpdate;
				}

				/*
				 * Compute the percent complete
				 */

				StateData.Data.FileUpdate.PercentComplete =
					100 * ArchiveData->BytesTransfered /
					ArchiveData->BytesToTransfer;

				/*
				 * Make sure that we don't get bit by a rounding
				 * error
				 */

				if (StateData.Data.FileUpdate.PercentComplete >
				    100) {
					StateData.Data.FileUpdate.PercentComplete =
						100;
				}

				(void) strcpy(
					StateData.Data.FileUpdate.FileName,
					    LastFile);
				StateData.Data.FileUpdate.BytesToTransfer =
					ArchiveData->BytesToTransfer;
				StateData.Data.FileUpdate.BytesTransfered =
					ArchiveData->BytesTransfered;

				/*
				 * Call the user's callback
				 */

				if (UserCallback(UserData, &StateData)) {

					/*
					 * Clean-up the shell process.
					 */

					if (ArchiveError =
					    DSRALShellCleanup(ArchiveData)) {
						return (ArchiveError);
					}
					return (DSRALCallbackFailure);
				}
			}

			/*
			 * If the first character is
			 * a / or a ./ then this is a
			 * file that has been
			 * archived.
			 */

			else if (strncmp(TmpBuffer1, "/", 1) == 0 ||
			    strncmp(TmpBuffer1, "./", 2) == 0) {

				/*
				 * Scan the file name out of the buffer
				 */

				(void) sscanf(TmpBuffer1, "%s", FileEntry);

				/*
				 * If the file name is the same as the last
				 * file processed then we skip it.  This
				 * happens because cpio kicks out
				 * informational text along with the file
				 * being restored.  (e.g. In the case of a
				 * hard link to another file cpio issues
				 * the following message "Created link to ...")
				 */

				if (strcmp(FileEntry, LastFile) != 0) {
					(void) strcpy(LastFile, FileEntry);
					if ((ArchiveError =
					    DSRALComputeArchiveSize(FileEntry,
						&ArchiveData->BytesTransfered))) {

						/*
						 * Clean-up the shell process.
						 */

						if (ArchiveError2 =
						    DSRALShellCleanup(ArchiveData)) {
							return (ArchiveError2);
						}
						return (ArchiveError);
					}
				}

				/*
				 * Set the callback data's state
				 */

				if (Operation == DSRALBackup) {
					StateData.State = DSRALBackupUpdate;
				} else {
					StateData.State = DSRALRestoreUpdate;
				}

				/*
				 * Compute the percent complete
				 */

				StateData.Data.FileUpdate.PercentComplete =
					100 * ArchiveData->BytesTransfered /
					ArchiveData->BytesToTransfer;

				/*
				 * If the percent complete is zero then we
				 * need to reset the last percent complete
				 * since this is the beginning of a new run
				 */

				if (StateData.Data.FileUpdate.PercentComplete == 0) {
					LastPercentComplete = 0;
				}

				/*
				 * Make sure that we don't get bit by a rounding
				 * error
				 */

				if (StateData.Data.FileUpdate.PercentComplete >
				    100) {
					StateData.Data.FileUpdate.PercentComplete =
						100;
				}

				if (StateData.Data.FileUpdate.PercentComplete -
				    LastPercentComplete > 1) {
					LastPercentComplete =
						StateData.Data.FileUpdate.PercentComplete;
					/*
					 * Copy the state specific information into the
					 * callback structure
					 */

					(void) strcpy(
						StateData.Data.FileUpdate.FileName,
						    TmpBuffer1);
					StateData.Data.FileUpdate.BytesToTransfer =
						ArchiveData->BytesToTransfer;
					StateData.Data.FileUpdate.BytesTransfered =
						ArchiveData->BytesTransfered;

					/*
					 * Call the user's callback
					 */

					if (UserCallback(UserData, &StateData)) {

						/*
						 * Clean-up the shell process.
						 */

						if (ArchiveError =
						    DSRALShellCleanup(ArchiveData)) {
							return (ArchiveError);
						}
						return (DSRALCallbackFailure);
					}
				}
			}
		}
	}

	/*
	 * Clean-up the shell process.
	 */

	if (ArchiveError = DSRALShellCleanup(ArchiveData)) {
		return (ArchiveError);
	}

	/*
	 * Check to see if we just finished a restore.
	 */

	if (Operation == DSRALRestore) {
		if (ArchiveError = DSRALRemoveArchiveFiles(ArchiveData)) {
			return (ArchiveError);
		}
	}

	/*
	 * If the backup is to floppy, then issue an eject command to get the
	 * floppy out of the drive.
	 */

	if (ArchiveData->Media == DSRALFloppy &&
	    IsIsa("sparc")) {
		(void) sprintf(TmpBuffer1,
		    "eject %s\n",
		    ArchiveData->MediaString);
		if (system(TmpBuffer1)) {
			return (DSRALSystemCallFailure);
		}
	} else if (ArchiveData->Media == DSRALNFS ||
	    (ArchiveData->Media == DSRALDisk &&
		ArchiveData->IsDevice == True)) {
		if ((ArchiveError = DSRALUnMount())) {
			return (ArchiveError);
		}
	}

	/*
	 * Call the user's callback with the appropriate end state
	 */

	if (UserCallback) {
		if (Operation == DSRALBackup) {
			StateData.State = DSRALBackupEnd;
		} else {
			StateData.State = DSRALRestoreEnd;
		}

		if (UserCallback(UserData, &StateData)) {
			return (DSRALCallbackFailure);
		}
	}

	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALSendCommand
 *
 * DESCRIPTION:
 *  This function allows the calling application to send a keyboard
 *  character to the shell created by DSRALArchive().
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, DSRALSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRALData *           This is a pointer to the Archive Data
 *                         structure.
 *  char                   This is the character to be sent to
 *                         shell.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALSendCommand(TDSRALData *ArchiveData, char Command)
{
	int		BytesToWrite;

	TPCFD		ChildFD;

	char		Buffer[2];

	if (PCGetFD(ArchiveData->PCHandle,
	    &ChildFD)) {
		return (DSRALChildProcessFailure);
	}
	Buffer[0] = Command;
	Buffer[1] = 0x00;

	BytesToWrite = strlen(Buffer);
	if (write(ChildFD.PTYMaster,
	    Buffer,
	    BytesToWrite) != BytesToWrite) {
		return (DSRALSystemCallFailure);
	}
	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALSetMedia
 *
 * DESCRIPTION:
 *  This function allows the calling aplication to set the media that
 *  will be used to backup and restore the archive.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, DSRALSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRArchiveList        This is the handle to the DSR
 *                         object returned by the call to
 *                         DSRALCreate().
 *  TDSRALMedia            The enumerated value for the type
 *                         of media to be used for the archive.
 *  char *                 The string that defines the
 *                         specified media.  The following
 *                         is the format for each allowed
 *                         media type.
 *                           FLOPPY: Path to floppy device
 *                            (e.g. /dev/rdiskette0)
 *                           TAPE: Path to tape device
 *                            (e.g. /dev/rmt/0)
 *                           DISK: Either a path to mounted directory
 *                                 or a path to a block device.
 *                            (e.g. /export/tmp or /dev/dsk/c0t3d0s0)
 *                           NFS: Path to NFS directory
 *                            (e.g. whistler:/export/home/vos)
 *                           RSH: Path to remote directory
 *                            (e.g. vos@whistler:/export/home/vos)
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TDSRALError
DSRALSetMedia(TDSRArchiveList Handle,
    TList SliceList,
    TDSRALMedia Media,
    char *MediaString)
{
	TDSRALData	*ArchiveData;
	TDSRALError	ArchiveError;
	char		TmpBuffer1[PATH_MAX];
	TBoolean	IsDevice;

	/*
	 * Check to see if the supplied handle is valid
	 */

	if ((ArchiveError = DSRALValidateHandle(Handle))) {
		return (ArchiveError);
	}
	ArchiveData = (TDSRALData *) Handle;

	(void) strcpy(TmpBuffer1, MediaString);
	canoninplace(TmpBuffer1);
	if ((ArchiveError = DSRALValidateMedia(SliceList,
	    Media,
	    TmpBuffer1,
	    &IsDevice))) {

		/*
		 * If the media is set to DSRALDisk then lets
		 * prepend the get_rootdir() to the beginning
		 * of the path and try again.  (The user may
		 * have given us a path not relative to the
		 * boot environment's root.
		 */

		switch (Media) {
		case DSRALDisk:
			(void) strcpy(TmpBuffer1, get_rootdir());
			(void) strcat(TmpBuffer1, MediaString);
			canoninplace(TmpBuffer1);

			if ((ArchiveError = DSRALValidateMedia(SliceList,
			    Media,
			    TmpBuffer1,
			    &IsDevice))) {
				return (ArchiveError);
			}
			break;
		default:
			return (ArchiveError);
		}
	}

	/*
	 * Read in the control file contents, if an error is
	 * encountered it is ignored since it means that the file
	 * does not exist.
	 */

	(void) DSRALProcessFile(DSRAL_CONTROL_PATH,
	    ArchiveData,
	    True);

	(void) strcpy(ArchiveData->MediaString, TmpBuffer1);
	ArchiveData->Media = Media;
	ArchiveData->IsDevice = IsDevice;

	/*
	 * Ok, write out the modifications back to the
	 * control file.
	 */

	if ((DSRALProcessFile(DSRAL_CONTROL_PATH,
	    ArchiveData,
	    False))) {
		return (DSRALProcessFileFailure);
	}

	return (DSRALSuccess);
}


/*
 * *********************************************************************
 * FUNCTION NAME: DSRALGenerate
 *
 * DESCRIPTION:
 *  This function generates the list of files to be archived given the
 *  list of file systems to search.  The results of the search are
 *  written to the file specified by the calling application.  The format
 *  of the output is a line oriented output where each line represents
 *  the path to the file system entry to be archived.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, DSRALSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRArchiveList        This is the handle to the DSR
 *                         object returned by the call to
 *                         DSRALCreate().
 *  TCallback *            A pointer to the callback function
 *                         to be called with updates.
 *  void *                 A pointer to any data that the
 *                         callback wants as the first argument
 *                         when invoked.
 *  unsigned long long *   This is the number of bytes in the final
 *                         archive.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TDSRALError
DSRALGenerate(TDSRArchiveList Handle,
    TList SliceList,
    TCallback *UserCallback,
    void *UserData,
    unsigned long long *BytesToTransfer)
{
	TDSRALStateData StateData;

	TDSRALData		*ArchiveData;
	TDSRALData		TmpArchiveData;
	TDSRALError		ArchiveError;

	Module			*Mod;

	char			TmpBuffer1[PATH_MAX];
	char			*LongString;
	char			*ShortString;

	int			Length1;
	int			Length2;

	TDSRALServiceEntry	*ServiceEntry;
	char			ContentsFile[PATH_MAX];
	char			CompletePath[PATH_MAX];

	TList			ServiceList2;
	TList			ServiceList1;
	TLink			CurrentLink;
	TLLError		LLError;

	TSLEntry		*SLEntry;

	TBoolean		UsedMountPoint;
	TBoolean		Complete;
	TBoolean		Done;

	struct statvfs		StatVFSBuf;

	/*
	 * Check to see if the supplied handle is valid
	 */

	if ((ArchiveError = DSRALValidateHandle(Handle))) {
		return (ArchiveError);
	}
	ArchiveData = (TDSRALData *) Handle;

	/*
	 * Initialize any of the internal data used within this function.
	 */

	ArchiveData->BytesToTransfer = 0;
	ArchiveData->TotalFSBytes = 0;
	ArchiveData->FSBytesProcessed = 0;
	ArchiveData->UserCallback = UserCallback;
	ArchiveData->UserData = UserData;

	/*
	 * Call the user's callback with the GenerateBegin state
	 */

	if (ArchiveData->UserCallback) {
		StateData.State = DSRALGenerateBegin;
		if (ArchiveData->UserCallback(ArchiveData->UserData,
		    &StateData)) {
			return (DSRALCallbackFailure);
		}
	}

	/*
	 * Open the file that I write each directory entry to be archived into
	 */

	if (!(ArchiveData->OutFILE = fopen(DSRAL_ARCHIVE_LIST_PATH, "w"))) {
		return (DSRALSystemCallFailure);
	}

	/*
	 * Create a list to hold the installed products to be searched
	 */

	if ((LLCreateList(&ServiceList1, NULL)) != LLSuccess) {
		return (DSRALListManagementError);
	}

	/*
	 * Create another list to hold the installed products to be searched.
	 * This one will be used by the directory processing calls to ensure
	 * that a search does not cross contents file boundaries.
	 */

	if ((LLCreateList(&ServiceList2, NULL)) != LLSuccess) {
		return (DSRALListManagementError);
	}

	/*
	 * Sort the list of file systems alphabetically by mount point
	 */

	if (SLSort(SliceList, SLMountPointDescending)) {
		return (DSRALListManagementError);
	}

	/*
	 * Loop through all of the slices in the slice list that will be
	 * archived and set the Searched field to False.
	 */

	if ((LLError = LLGetLinkData(SliceList,
	    LLHead,
	    &CurrentLink,
	    (void **) &SLEntry))) {
		return (DSRALListManagementError);
	}

	Done = False;
	while (!Done) {

		/*
		 * Check to see if the slice is in the /etc/vfstab, that
		 * the slice has been selected to be archived and
		 * it is a ufs type file system.
		 */

		if (SLEntry->InVFSTab	== True &&
		    SLEntry->State	!= SLFixed &&
		    SLEntry->FSType	== SLUfs) {

			/*
			 * Get the stats on the file syetm so that the total
			 * number of bytes across all file systems to be
			 * searched.
			 */

			(void) strcpy(TmpBuffer1, get_rootdir());
			(void) strcat(TmpBuffer1, SLEntry->MountPoint);
			canoninplace(TmpBuffer1);

			if (statvfs(TmpBuffer1, &StatVFSBuf)) {
				return (DSRALSystemCallFailure);
			}

			/*
			 * Determine how many bytes on the file system are being
			 * used.
			 */

			ArchiveData->TotalFSBytes +=
				(StatVFSBuf.f_blocks - StatVFSBuf.f_bfree) *
				StatVFSBuf.f_frsize;

			/*
			 * Ok, Set the searched field to false.
			 */

			SLEntry->Searched = False;
		}
		LLError = LLGetLinkData(SliceList,
		    LLNext,
		    &CurrentLink,
		    (void **) &SLEntry);
		switch (LLError) {
		case LLSuccess:
			break;
		case LLEndOfList:
			Done = True;
			break;
		default:
			return (DSRALListManagementError);
		}
	}

	/*
	 * Loop through all of the services within the product
	 */

	for (Mod = get_media_head(); Mod != NULL; Mod = Mod->next) {

		/*
		 * If the product has not been tagged to be upgraded.
		 * Then we just skip it here.
		 */

		if (!(Mod->info.media->med_flags & BASIS_OF_UPGRADE)) {
			continue;
		}

		/*
		 * If this is the O/S service that is the same version
		 * as the installed O/s of the server then we don't want
		 * to process it's contents file since it is shared
		 * with the installed O/S.
		 */

		if (Mod->info.media->med_type == INSTALLED_SVC &&
		    Mod->info.media->med_flags & SPLIT_FROM_SERVER) {
			continue;
		}

		/*
		 * If the current product is either the Installed system or a
		 * installed service
		 */

		if (Mod->info.media->med_type == INSTALLED ||
		    Mod->info.media->med_type == INSTALLED_SVC) {

			/*
			 * Load up the view of the current service so that it
			 * can be accessed.
			 */

			(void) load_view(Mod->sub, Mod);

			/*
			 * Ok, the next step is to build a list of the
			 * services that are installed on the system that are
			 * of interest to the archive logic.  We have to
			 * build this list and cannot use the current link
			 * list that we are looping through, is the list of
			 * services must be backwards alphabetically sorted
			 * my root directory start points.
			 */

			/*
			 * Malloc out space for a new Module entry.
			 */

			if (!(ServiceEntry = (TDSRALServiceEntry *)
			    malloc(sizeof (TDSRALServiceEntry)))) {
				return (DSRALMemoryAllocationFailure);
			}

			/*
			 * Malloc out space for the Root Directory of the
			 * module
			 */

			if (!(ServiceEntry->RootDir = (char *) malloc(
				strlen(Mod->sub->info.prod->p_rootdir)))) {
				return (DSRALMemoryAllocationFailure);
			}
			(void) strcpy(ServiceEntry->RootDir,
			    Mod->sub->info.prod->p_rootdir);
			ServiceEntry->Mod = Mod;

			/*
			 * Create a new link for the entry
			 */

			if ((LLCreateLink(&CurrentLink, ServiceEntry))) {
				return (DSRALListManagementError);
			}

			/*
			 * Add the new link to the list.
			 */

			if ((LLAddLink(ServiceList1,
			    CurrentLink,
			    LLTail))) {
				return (DSRALListManagementError);
			}

			/*
			 * Malloc out space for a new Module entry.
			 */

			if (!(ServiceEntry = (TDSRALServiceEntry *)
			    malloc(sizeof (TDSRALServiceEntry)))) {
				return (DSRALMemoryAllocationFailure);
			}

			/*
			 * Malloc out space for the Root Directory of the
			 * module
			 */

			if (!(ServiceEntry->RootDir = (char *) malloc(
				strlen(Mod->sub->info.prod->p_rootdir)))) {
				return (DSRALMemoryAllocationFailure);
			}
			(void) strcpy(ServiceEntry->RootDir,
			    Mod->sub->info.prod->p_rootdir);
			ServiceEntry->Mod = Mod;

			/*
			 * Create a new link for the entry
			 */

			if ((LLCreateLink(&CurrentLink, ServiceEntry))) {
				return (DSRALListManagementError);
			}

			/*
			 * Add the new link to the list.
			 */

			if ((LLAddLink(ServiceList2,
			    CurrentLink,
			    LLTail))) {
				return (DSRALListManagementError);
			}
		}
	}

	/*
	 * Now that we have all of the installed services that will be
	 * searched in the list, it's time to reverse alphabetically sort the
	 * list.  This is done so that the search of the contents files will
	 * be from child to parent.
	 */

	if (LLSortList(ServiceList1, DSRALSortServiceList, NULL)) {
		return (DSRALListManagementError);
	}

	/*
	 * Now sort the directory parser's version of the list.
	 */

	if (LLSortList(ServiceList2, DSRALSortServiceList, NULL)) {
		return (DSRALListManagementError);
	}

	/*
	 * Write out the name of the recovery file and the upgrade script
	 * into the archive list.  This is done to allow recovery from a
	 * system failure that occurs after the successful archival
	 * of the files contained in the archive list.  The reason for
	 * making these files the first two, is to allow the files to
	 * be easily extracted from the archive by the user if required.
	 */

	(void) fprintf(ArchiveData->OutFILE, "%s\n", DSRAL_RECOVERY_BACKUP_PATH);
	(void) fprintf(ArchiveData->OutFILE, "%s\n", DSRAL_UPGRADE_SCRIPT_PATH);
	(void) fprintf(ArchiveData->OutFILE, "%s\n", INST_RELEASE_read_path(""));
	(void) fprintf(ArchiveData->OutFILE, "%s\n", CLUSTER_read_path(""));
	(void) fprintf(ArchiveData->OutFILE, "%s\n", clustertoc_read_path(""));
	(void) fprintf(ArchiveData->OutFILE, "%s\n", DSRAL_USR_PACKAGES_EXIST_PATH);

	/*
	 * Now, lets loop through each entry in the link list.
	 */

	if ((LLGetLinkData(ServiceList1,
	    LLHead,
	    &CurrentLink,
	    (void **) &ServiceEntry))) {
		return (DSRALListManagementError);
	}
	Complete = False;
	while (!Complete) {

		/*
		 * Load up the view of the current service so that it can be
		 * accessed.
		 */

		(void) load_view(ServiceEntry->Mod->sub, ServiceEntry->Mod);

		/*
		 * Build the path to the current service's contents file
		 */

		(void) sprintf(ContentsFile,
		    "%s/var/sadm/install/contents",
		    ServiceEntry->RootDir);

		canoninplace(ContentsFile);

		/*
		 * Now, lets loop through each entry in the slice list.
		 */

		if ((LLError = LLGetLinkData(SliceList,
		    LLHead,
		    &CurrentLink,
		    (void **) &SLEntry))) {
			return (DSRALListManagementError);
		}
		Done = False;
		while (!Done) {

			/*
			 * Now, we need to figure out if the root start point
			 * is contained within the current file system or
			 * vice versa.
			 */

			Length1 = strlen(SLEntry->MountPoint);
			Length2 = strlen(ServiceEntry->RootDir);


			if (Length1 >= Length2) {
				ShortString = ServiceEntry->RootDir;
				LongString = SLEntry->MountPoint;
				UsedMountPoint = True;
				Length1 = Length2;
			} else {
				ShortString = SLEntry->MountPoint;
				LongString = ServiceEntry->RootDir;
				UsedMountPoint = False;
			}

			/*
			 * Check to see if the slice is in the /etc/vfstab,
			 * that the slice has been selected to be archived,
			 * that the file system has not already been searched
			 * and that it is a ufs file system.  If all of these
			 * items are true then we need to search it.
			 */

			if (SLEntry->InVFSTab	== True &&
			    SLEntry->State 	!= SLFixed &&
			    SLEntry->Searched	== False &&
			    SLEntry->FSType	== SLUfs) {

				if (strncmp(ShortString,
				    LongString,
				    Length1) == 0) {

					/*
					 * Ok, the shorter string is
					 * contained within the longer string
					 * so pass in the longer string as
					 * the starting point for the file
					 * system search.
					 */

					(void) strcpy(CompletePath,
					    get_rootdir());
					(void) strcat(CompletePath, LongString);
					canoninplace(CompletePath);

					if ((ArchiveError = DSRALProcessPath(
						ArchiveData,
						    NULL,
						    ServiceList2,
						    ContentsFile,
						    SLEntry->MountPoint,
						    CompletePath))) {
						return (ArchiveError);
					}

					/*
					 * If the MountPoint was used to seed the
					 * search rather than the root directory
					 * for the controlling contents file then
					 * set the searched flag to true on the
					 * file system so that we don't search it
					 * again.
					 */

					if (UsedMountPoint == True) {

						/*
						 * Set the Searched field to true so
						 * that we don't re-search the file
						 * system
						 */

						SLEntry->Searched = True;

					}

					/*
					 * If the root directory for the
					 * controlling contents file was
					 * used then we don't want to set the
					 * searched flag to true since the
					 * portion of the file system that is
					 * above the search start point
					 * belongs to the parent contents file.
					 * Also, since the file system's are
					 * reverse alphabetically sorted we
					 * know that we are done searching
					 * file systems for this contents file.
					 */

					else {
						Done = True;
					}
				}
			}
			LLError = LLGetLinkData(SliceList,
			    LLNext,
			    &CurrentLink,
			    (void **) &SLEntry);
			switch (LLError) {
			case LLSuccess:
				break;
			case LLEndOfList:
				Done = True;
				break;
			default:
				return (DSRALListManagementError);
			}
		}

		/*
		 * Get the next module entry from the installed list
		 */

		LLError = LLGetLinkData(ServiceList1,
		    LLNext,
		    &CurrentLink,
		    (void **) &ServiceEntry);

		switch (LLError) {
		case LLSuccess:
			break;
		case LLEndOfList:
		case LLListEmpty:
			Complete = True;
			break;
		default:
			return (DSRALListManagementError);
		}

	}

	/*
	 * This is a temporary solution.  The way that I update the progress
	 * meter results in never quite getting to 100%.  So I go ahead and
	 * call the callback after everything is complete to give it the last
	 * update.
	 */

	if (ArchiveData->UserCallback) {
		StateData.State = DSRALGenerateUpdate;
		StateData.Data.GenerateUpdate.PercentComplete = 100;
		(void) strcpy(StateData.Data.GenerateUpdate.ContentsFile,
		    ContentsFile);
		(void) strcpy(StateData.Data.GenerateUpdate.FileSystem, "");
		if (ArchiveData->UserCallback(ArchiveData->UserData,
		    &StateData)) {
			return (DSRALCallbackFailure);
		}
	}

	/*
	 * Call the callback with the GenerateEnd state
	 */

	if (ArchiveData->UserCallback) {
		StateData.State = DSRALGenerateEnd;
		if (ArchiveData->UserCallback(ArchiveData->UserData,
		    &StateData)) {
			return (DSRALCallbackFailure);
		}
	}

	/*
	 * We're done so clean up the temporary Service list
	 */

	if ((LLClearList(ServiceList1, DSRALClearServiceList))) {
		return (DSRALListManagementError);
	}
	if ((LLDestroyList(&ServiceList1, NULL)) != LLSuccess) {
		return (DSRALListManagementError);
	}
	if ((LLClearList(ServiceList2, DSRALClearServiceList))) {
		return (DSRALListManagementError);
	}
	if ((LLDestroyList(&ServiceList2, NULL)) != LLSuccess) {
		return (DSRALListManagementError);
	}

	/*
	 * Close the output file.
	 */

	(void) fclose(ArchiveData->OutFILE);

	/*
	 * Finally, Add the file size of the recovery file and the
	 * upgrade script to the archive size.  NOTE: I cannot
	 * actually stat the Recovery script at this point but
	 * I need to account for it's size, so add in the worst
	 * case sizing for the file.
	 */

	ArchiveData->BytesToTransfer += DSRAL_RECOVERY_FILE_SIZE;

	/*
	 * If we are running in simulation mode then the upgrade script
	 * is generated into the /tmp directory so we have to go there
	 * to get it's size
	 */

	if (GetSimulation(SIM_EXECUTE)) {
		if ((ArchiveError = DSRALComputeArchiveSize(
			DSRAL_UPGRADE_SCRIPT_TMP_PATH,
			    &ArchiveData->BytesToTransfer))) {
			return (ArchiveError);
		}
	} else {
		if ((ArchiveError = DSRALComputeArchiveSize(
			DSRAL_UPGRADE_SCRIPT_PATH,
			    &ArchiveData->BytesToTransfer))) {
			return (ArchiveError);
		}
	}

	/*
	 * Try and read in the control file.  This file will only exist
	 * if the user has already called the DSRALSetMedia() call.  If
	 * the file exists then we need to copy the Media, MediaString,
	 * IsDevice fields.
	 */

	if ((DSRALProcessFile(DSRAL_CONTROL_PATH,
	    &TmpArchiveData,
	    True)) == DSRALSuccess) {
		ArchiveData->Media = TmpArchiveData.Media;
		(void) strcpy(ArchiveData->MediaString,
		    TmpArchiveData.MediaString);
		ArchiveData->IsDevice = TmpArchiveData.IsDevice;
	}

	/*
	 * Ok, now that the list of files to archive have been generated,
	 * open the control file and write out the information.
	 * This file is used to allow the DSRALGenerate() function to
	 * be called in one process and the DSRALArchive() function to
	 * be called from another process.  To allow this, the
	 * information that needs to be passed from DSRALGenerate()
	 * to DSRALArchive() is stored in the file.
	 */

	if ((DSRALProcessFile(DSRAL_CONTROL_PATH,
	    ArchiveData,
	    False))) {
		return (DSRALProcessFileFailure);
	}

	*BytesToTransfer = ArchiveData->BytesToTransfer;
	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALGetErrorText
 *
 * DESCRIPTION:
 *  Convert the provided enumerated error code into its internationalized
 *  error text.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  char *                 The internationalized error text corresponding
 *                         to the provided enumerated error code.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            The enumerated error code to be converted.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

char *
DSRALGetErrorText(TDSRALError DSRALError)
{
	switch (DSRALError) {
	case DSRALSuccess:
		return (MSG0_DSRAL_SUCCESS);
	case DSRALRecovery:
		return (MSG0_DSRAL_RECOVERY);
	case DSRALCallbackFailure:
		return (MSG0_DSRAL_CALLBACK_FAILURE);
	case DSRALProcessFileFailure:
		return (MSG0_DSRAL_PROCESS_FILE_FAILURE);
	case DSRALMemoryAllocationFailure:
		return (MSG0_DSRAL_MEMORY_ALLOCATION_FAILURE);
	case DSRALInvalidHandle:
		return (MSG0_DSRAL_INVALID_HANDLE);
	case DSRALUpgradeCheckFailure:
		return (MSG0_DSRAL_UPGRADE_CHECK_FAILURE);
	case DSRALInvalidMedia:
		return (MSG0_DSRAL_INVALID_MEDIA);
	case DSRALNotCharDevice:
		return (MSG0_DSRAL_NOT_CHAR_DEVICE);
	case DSRALUnableToWriteMedia:
		return (MSG0_DSRAL_UNABLE_TO_WRITE_MEDIA);
	case DSRALUnableToStatPath:
		return (MSG0_DSRAL_UNABLE_TO_STAT_PATH);
	case DSRALCannotRsh:
		return (MSG0_DSRAL_CANNOT_RSH);
	case DSRALUnableToOpenDirectory:
		return (MSG0_DSRAL_UNABLE_TO_OPEN_DIRECTORY);
	case DSRALInvalidPermissions:
		return (MSG0_DSRAL_INVALID_PERMISSIONS);
	case DSRALInvalidDiskPath:
		return (MSG0_DSRAL_INVALID_DISK_PATH);
	case DSRALDiskNotFixed:
		return (MSG0_DSRAL_DISK_NOT_FIXED);
	case DSRALUnableToMount:
		return (MSG0_DSRAL_UNABLE_TO_MOUNT);
	case DSRALNoMachineName:
		return (MSG0_DSRAL_NO_MACHINE_NAME);
	case DSRALItemNotFound:
		return (MSG0_DSRAL_ITEM_NOT_FOUND);
	case DSRALChildProcessFailure:
		return (MSG0_DSRAL_CHILD_PROCESS_FAILURE);
	case DSRALListManagementError:
		return (MSG0_DSRAL_LIST_MANAGEMENT_ERROR);
	case DSRALInsufficientMediaSpace:
		return (MSG0_DSRAL_INSUFFICIENT_MEDIA_SPACE);
	case DSRALSystemCallFailure:
		return (MSG0_DSRAL_SYSTEM_CALL_FAILURE);
	case DSRALInvalidFileType:
		return (MSG0_DSRAL_INVALID_FILE_TYPE);
	default:
		return (MSG0_DSRAL_INVALID_ERROR_CODE);
	}
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALClearServiceList
 *
 * DESCRIPTION:
 *  This function is the callback for LLClearList to clean up the data
 *  pointer of the link before deleting the link.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  int                      0 : Success
 *                          -1 : Failure
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDLLData               A pointer to the data to be free'd
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TLLError
DSRALClearServiceList(TLLData Data)
{
	TDSRALServiceEntry *ServiceEntry;

	ServiceEntry = (TDSRALServiceEntry *) Data;

	free(ServiceEntry->RootDir);
	free(ServiceEntry);

	return (LLSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALClearDirectoryList
 *
 * DESCRIPTION:
 *  This function is the callback for LLClearList to clean up the data
 *  pointer of the link before deleting the link.
 *
 * RETURN:
 *  TYPE		   DESCRIPTION
 *  LLError                This function will always return LLSuccess.
 *                         the return value is only supported because
 *                         LLClearList() requires a return value.
 *
 * PARAMETERS:
 *  TYPE		   DESCRIPTION
 *  TDLLData		   A pointer to the data to be free'd
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TLLError
DSRALClearDirectoryList(TLLData Data)
{
	TDSRALDirectoryEntry *DirectoryEntry;

	DirectoryEntry = (TDSRALDirectoryEntry *) Data;

	free(DirectoryEntry->Path);
	free(DirectoryEntry);

	return (LLSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALSortServiceList
 *
 * DESCRIPTION:
 *  This is the callback for the LLSortList function.  This function compares
 *  the contents of the two root directory paths and determines which is
 *  alphabetically larger.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TLLCompare             LLCompareLess    : The Insert string is before
 *                                            the current sorted entry.
 *                         LLCompareEqual   : The entries are te same
 *                         LLCompareGreater : The Insert string is after
 *                                            the current sorted entry.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  void *                 User supplied data pointer.  Given
 *                         to the LLSortList function when called.
 *  TDLLData               A pointer to the data associated with the
 *                         link to be inserted.
 *  TDLLData               A pointer to the data associated with the
 *                         current link in the sorted list.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TLLCompare
DSRALSortServiceList(void *UserPtr,
    TLLData Insert,
    TLLData Sorted)
{

	TDSRALServiceEntry *InsertEntry;
	TDSRALServiceEntry *SortedEntry;

	/*
	 * Keep lint from complaining
	 */

	UserPtr = UserPtr;

	InsertEntry = (TDSRALServiceEntry *) Insert;
	SortedEntry = (TDSRALServiceEntry *) Sorted;

	if (strcmp(InsertEntry->RootDir, SortedEntry->RootDir) >= 0) {
		return (LLCompareLess);
	}
	return (LLCompareGreater);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALSortDirectoryList
 *
 * DESCRIPTION:
 *  This is the callback for the LLSortList function.  This function compares
 *  the contents of the two paths and determines which is alphabetically
 *  smaller.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TLLCompare             LLCompareLess    : The Insert string is before
 *                                            the current sorted entry.
 *                         LLCompareEqual   : The entries are te same
 *                         LLCompareGreater : The Insert string is after
 *                                            the current sorted entry.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  void *                 Not used within the callback.
 *  TDLLData               A pointer to the data associated with the
 *                         link to be inserted.
 *  TDLLData               A pointer to the data associated with the
 *                         current link in the sorted list.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TLLCompare
DSRALSortDirectoryList(void *UserPtr,
    TLLData Insert,
    TLLData Sorted)
{
	TDSRALDirectoryEntry *InsertEntry;
	TDSRALDirectoryEntry *SortedEntry;


	/*
	 * Keep lint from complaining
	 */

	UserPtr = UserPtr;

	InsertEntry = (TDSRALDirectoryEntry *) Insert;
	SortedEntry = (TDSRALDirectoryEntry *) Sorted;

	/*
	 * If the current Directory Entry is less than the current sorted
	 * entry
	 */

	if (strcmp(InsertEntry->Path, SortedEntry->Path) <= 0) {
		return (LLCompareLess);
	}
	return (LLCompareGreater);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALInServiceList
 *
 * DESCRIPTION:
 *  This function determines if the given path is one of the starting
 *  root paths for a service in the service list.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, SLSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TList                  This is the ServiceList which contains
 *                         the installed services root start
 *                         point.
 *  char *                 The path to be found in the service list.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALInServiceList(TList ServiceList,
    char *Path)
{
	TDSRALServiceEntry *ServiceEntry;
	TLLError	LLError;
	TLink		CurrentLink;
	TBoolean	Done;

	if ((LLError = LLGetLinkData(ServiceList,
	    LLHead,
	    &CurrentLink,
	    (void **) &ServiceEntry))) {
		switch (LLError) {
		case LLListEmpty:
			return (DSRALItemNotFound);
		default:
			return (DSRALListManagementError);
		}
	}

	/*
	 * Loop through all of the slices to find a match
	 */

	Done = False;
	while (!Done) {
		if (strcmp(Path, ServiceEntry->RootDir) == 0) {
			return (DSRALSuccess);
		}
		if ((LLError = LLGetLinkData(ServiceList,
		    LLNext,
		    &CurrentLink,
		    (void **) &ServiceEntry))) {
			switch (LLError) {
			case LLEndOfList:
				return (DSRALItemNotFound);
			default:
				return (DSRALListManagementError);
			}
		}
	}
	return (DSRALItemNotFound);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALDirectoryEntryParser
 *
 * DESCRIPTION:
 *  This function determines the type of the given file system entry
 *  and then calls will_file_be_upgraded() to find out if the file should
 *  be archived.  If the file will not be replaced during the upgrade
 *  then the file path is written to the archive list output file and
 *  the number of bytes in the archive are incremented.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, SLSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRALData *           This is a pointer to the Archive Data
 *                         structure.
 *  char *                 The path to the file being processed
 *  struct stat *          The stat structure returned by lstat() for
 *                         file being processed.
 *  int                    The type of file beign processed.  The values
 *                         are those from ftw.h.  They are:
 *                           FTW_F          The object is a file.
 *                           FTW_D          The object is a directory.
 *                           FTW_DNR        The object is a directory
 *                                          that cannot be read.  Descendants
 *                                          of the directory will not be
 *                                          processed.
 *                           FTW_NS         stat failed on the object
 *                                          because of lack of appropriate
 *                                          permission or the object is a
 *                                          symbolic link that points to
 *                                          a non-existent file.  The stat
 *                                          buffer passed to fn is
 *                                          undefined.
 *  char *                 The path for the contents file that controls
 *                         the entry being processed.
 *  int                    The reset flag.  This flag is set to one when
 *                         a new file system search is being started.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALDirectoryEntryParser(TDSRALData *ArchiveData,
    char *Path,
    struct stat *StatBuf,
    int EntryClass,
    char *ContentsFile,
    int Reset)
{

	int Status;
	char LocalPath[PATH_MAX];

	/*
	 * Depending on the type of the entry that the path points to.
	 */

	switch (EntryClass) {

	/*
	 * If the entry is either a file or directory entry then
	 * continue the processing.
	 */

	case FTW_F:
	case FTW_D:
		break;

	/*
	 * The remaining types are errors so trap them and return
	 * the appropriate error.
	 */

	case FTW_DNR:
		return (DSRALUnableToOpenDirectory);
	case FTW_NS:
		return (DSRALUnableToStatPath);
	default:
		return (DSRALInvalidFileType);
	}

	/*
	 * Now, call the function that determines if the given file will be
	 * upgraded.  Note, that the if this is the first call to this
	 * function for this file system, then the reset flag is set to
	 * non-zero to force the search logic to reinitialize.	Also,
	 * The path that is passed into this function has prepended the
	 * results of a get_rootdir() call.  However, the entry in the
	 * contents file does not, so we have to remove it prior to calling
	 * the following function.
	 */

	(void) strcpy(LocalPath, &Path[strlen(get_rootdir())]);

	/*
	 * Check to see if there is anything left in the LocalPath string.
	 * In the case of the root directory with a /a prepended the
	 * trailing / is lost.  So when I compute the offset to get
	 * the root relative string I end up with NULL.  If this is the
	 * case, then I just put the "/" back.
	 */

	if (strlen(LocalPath) == 0) {
		(void) strcpy(LocalPath, "/");
	}
	Status = file_will_be_upgraded(LocalPath, ContentsFile, Reset);

	/*
	 * If the return is zero, then the file will not be replaced as a part
	 * of the upgrade so we need to enter it into the archive.
	 */

	if (Status == 0) {

		/*
		 * Ok, this is interesting.  Since the /etc/vfstab is being
		 * modified as a part of the upgrade we don't want to
		 * archive it.  It will be copied into place after the
		 * backup is complete.  By ommiting it from here
		 * I can guarentee that the right vfstab is in place
		 * on the disk.
		 */

		if (!streq("/a/etc/vfstab", Path)) {
			(void) fprintf(ArchiveData->OutFILE,
			    "%s\n",
			    Path);
		}

		switch (StatBuf->st_mode & S_IFMT) {

		/*
		 * All of the following contribute to the archive size.
		 */

		case S_IFDIR:
		case S_IFREG:
		case S_IFIFO:
		case S_IFLNK:
		case S_IFSOCK:
		case S_IFDOOR:
			ArchiveData->BytesToTransfer += StatBuf->st_size;
			break;

		/*
		 * All of the following are valid file types but
		 * we don't count their sizes into the archive
		 */

		case S_IFBLK:
		case S_IFCHR:
			break;

		/*
		 * If I can't figure the type then we have a problem
		 */

		default:
			return (DSRALInvalidFileType);
		}
	}

	/*
	 * If the return status is less than zero, then the function had a
	 * fatal failure.
	 */

	else if (Status < 0) {
		return (DSRALUpgradeCheckFailure);
	}

	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALProcessPath
 *
 * DESCRIPTION:
 *  This function recursively searches the given current path for
 *  each file system entry.  As each entry is found, its type is determined
 *  and the DirectoryEntryParser() function is called to parse the
 *  entry and record it to the output file if necessary.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, SLSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRALData *           A pointer to the DSR ArchiveList's internal
 *                         data structure.
 *  void *                 This contains the parent's stat() buffer.
 *                         When calling this function for the first time
 *                         this field should be set to NULL.
 *  TList                  This is the list of installed services on the
 *                         platform that are to be upgraded.
 *  char *                 The name of the contents file being searched.
 *  char *                 The mount point for the file systems being
 *                         searched.
 *  char *                 The directory path to start searching the
 *                         file system.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALProcessPath(TDSRALData *ArchiveData,
    void *RecursiveCall,
    TList ServiceList,
    char *ContentsFile,
    char *FSMountPoint,
    char *CurrentPath)
{
	TList			DirectoryList;
	TLink			DirectoryLink;
	TDSRALDirectoryEntry	*DirectoryEntry;
	TLLError		LLError;
	TBoolean		Done;
	static unsigned long	LastPercentComplete;

	struct stat		CurrentStatBuf;
	struct stat		ParentStatBuf;
	struct dirent		*dirp;
	DIR			*dp;
	char			*ptr;

	int			Reset;

	TDSRALError		ArchiveError;
	TDSRALStateData		StateData;

	/*
	 * If this is being invoked recursively then the RecursiveCall field
	 * will be non-NULL.
	 */

	if (!RecursiveCall) {
		if (lstat(CurrentPath, &ParentStatBuf) < 0) {
			return (DSRALUnableToStatPath);
		}
		Reset = 1;
	} else {
		(void) memcpy(&ParentStatBuf,
		    RecursiveCall,
		    sizeof (ParentStatBuf));
		Reset = 0;
	}

	/*
	 * Get the file stats for the current path, if an error is
	 * encountered then call the user's Callback with FTW_NS set.
	 */

	if (lstat(CurrentPath, &CurrentStatBuf) < 0) {
		return (DSRALDirectoryEntryParser(ArchiveData,
		    CurrentPath,
		    &CurrentStatBuf,
		    FTW_NS,
		    ContentsFile,
		    Reset));
	}

	if (ArchiveData->UserCallback) {

		/*
		 * Call the user's callback with the current status.
		 */

		switch (CurrentStatBuf.st_mode & S_IFMT) {
		case S_IFDIR:
		case S_IFREG:
		case S_IFIFO:
		case S_IFLNK:
		case S_IFSOCK:
		case S_IFDOOR:
			ArchiveData->FSBytesProcessed += CurrentStatBuf.st_size;
			break;
		case S_IFBLK:
		case S_IFCHR:
		default:
			break;
		}

		StateData.State = DSRALGenerateUpdate;
		StateData.Data.GenerateUpdate.PercentComplete =
			100 * ArchiveData->FSBytesProcessed /
			ArchiveData->TotalFSBytes;

		/*
		 * If the percent complete is zero then we need to reset
		 * the last percent complete since this is the beginning
		 * of a new run
		 */

		if (StateData.Data.GenerateUpdate.PercentComplete == 0) {
			LastPercentComplete = 0;
		}

		/*
		 * If the percent complete is greater than 100 percent
		 * then set it to 100
		 */

		if (StateData.Data.GenerateUpdate.PercentComplete > 100) {
			StateData.Data.GenerateUpdate.PercentComplete = 100;
		}

		/*
		 * Ok, if the difference between the last percent complete
		 * and the newly calculated percent is greater than 1% then
		 * call the update callback
		 */

		if (StateData.Data.GenerateUpdate.PercentComplete -
		    LastPercentComplete > 1) {
			LastPercentComplete =
				StateData.Data.GenerateUpdate.PercentComplete;

			(void) strcpy(StateData.Data.GenerateUpdate.ContentsFile,
			    ContentsFile);
			(void) strcpy(StateData.Data.GenerateUpdate.FileSystem,
			    FSMountPoint);
			if (ArchiveData->UserCallback(ArchiveData->UserData,
			    &StateData)) {
				return (DSRALCallbackFailure);
			}
		}

	}

	/*
	 * Check to see if the current path points to a directory.  If it
	 * DOES NOT then it is a file, so call the user's callback.
	 */

	if (S_ISDIR(CurrentStatBuf.st_mode) == 0) {
		return (DSRALDirectoryEntryParser(ArchiveData,
		    CurrentPath,
		    &CurrentStatBuf,
		    FTW_F,
		    ContentsFile,
		    Reset));
	}

	/*
	 * Ok, It's a directory.  Call the user's Callback for the current
	 * directory before processing the files within the directory.
	 */

	if ((ArchiveError = DSRALDirectoryEntryParser(ArchiveData,
	    CurrentPath,
	    &CurrentStatBuf,
	    FTW_D,
	    ContentsFile,
	    Reset))) {
		return (ArchiveError);
	}

	/*
	 * Check to see if we are transitioning onto a new file system
	 */

	if (ParentStatBuf.st_dev != CurrentStatBuf.st_dev) {
		return (DSRALSuccess);
	}

	/*
	 * If this is a recursive call to this routine then we need to make
	 * sure that we are not about to step across into another contents
	 * file.  So search the root start list and see if the current
	 * directory matches any entry in the list.
	 */

	if (RecursiveCall) {

		/*
		 * Since the service list does not contain the possible /a
		 * preceeding the path, I must remove it from the CurrentPath.
		 */

		ArchiveError = DSRALInServiceList(ServiceList,
		    &CurrentPath[strlen(get_rootdir())]);
		switch (ArchiveError) {
		case DSRALSuccess:
			return (DSRALSuccess);
		case DSRALItemNotFound:
			break;
		default:
			return (ArchiveError);
		}
	}

	/*
	 * Try to open the current directory.  If it fails then call the
	 * user's Callback and send the FTW_DNR flag.
	 */

	if ((dp = opendir(CurrentPath)) == NULL) {
		return (DSRALDirectoryEntryParser(ArchiveData,
		    CurrentPath,
		    &CurrentStatBuf,
		    FTW_DNR,
		    ContentsFile,
		    Reset));
	}

	/*
	 * Set up the local pointer to point to the end of the current path.
	 * If the path only contains the root '/' then skip adding another
	 * slash.  Otherwise a slash at the end of the path in preperation
	 * for appending more text.
	 */

	ptr = CurrentPath + strlen(CurrentPath);

	if (strlen(CurrentPath) != 1 || CurrentPath[0] != '/') {
		*ptr++ = '/';
		*ptr = NULL;
	}

	/*
	 * Create a list to hold the contents of the directory
	 */

	if ((LLCreateList(&DirectoryList, NULL)) != LLSuccess) {
		return (DSRALListManagementError);
	}

	/*
	 * Read the info on the current directory.
	 */

	while ((dirp = readdir(dp)) != NULL) {

		/*
		 * Check for the current and parent directories and toss them
		 * away
		 */

		if (strcmp(dirp->d_name, ".") == 0 ||
		    strcmp(dirp->d_name, "..") == 0) {
			continue;
		}

		/*
		 * Malloc out space for the Directory Entry.
		 */

		if (!(DirectoryEntry = (TDSRALDirectoryEntry *)
		    malloc(sizeof (TDSRALDirectoryEntry)))) {
			return (DSRALMemoryAllocationFailure);
		}

		/*
		 * Copy the next name in the directory onto the path.
		 */

		(void) strcpy(ptr, dirp->d_name);
		(void) strcpy(DirectoryEntry->Path, CurrentPath);

		/*
		 * Create a new link for the entry
		 */

		if ((LLCreateLink(&DirectoryLink, DirectoryEntry))) {
			return (DSRALListManagementError);
		}

		/*
		 * Add the new link to the list.
		 */

		if ((LLAddLink(DirectoryList,
		    DirectoryLink,
		    LLTail))) {
			return (DSRALListManagementError);
		}
	}

	/*
	 * Null out the current path at the current directory level.
	 */

	*(--ptr) = NULL;

	/*
	 * Close the current directory.
	 */

	if (closedir(dp) < 0) {
		return (DSRALSystemCallFailure);
	}

	/*
	 * Now, alphabetically sort the list of directory entries.
	 */

	if (LLSortList(DirectoryList,
	    DSRALSortDirectoryList,
	    NULL)) {
		return (DSRALListManagementError);
	}

	/*
	 * Now that the directory entry list is alphabeticaly
	 * sorted loop through and process each entry
	 */

	if ((LLError = LLGetLinkData(DirectoryList,
	    LLHead,
	    &DirectoryLink,
	    (void **) &DirectoryEntry))) {
		switch (LLError) {
		case LLListEmpty:
			return (DSRALSuccess);
		default:
			return (DSRALListManagementError);
		}
	}

	Done = False;
	while (!Done) {

		/*
		 * Call this same routine recursively to process this path
		 * name
		 */

		if ((ArchiveError = DSRALProcessPath(ArchiveData,
		    &ParentStatBuf,
		    ServiceList,
		    ContentsFile,
		    FSMountPoint,
		    DirectoryEntry->Path))) {
			return (ArchiveError);
		}
		if ((LLError = LLGetLinkData(DirectoryList,
		    LLNext,
		    &DirectoryLink,
		    (void **) &DirectoryEntry))) {
			switch (LLError) {
			case LLEndOfList:
				Done = True;
				break;
			default:
				return (DSRALListManagementError);
			}
		}
	}

	/*
	 * Clean up the temporary directory list
	 */

	if ((LLClearList(DirectoryList, DSRALClearDirectoryList))) {
		return (DSRALListManagementError);
	}
	if ((LLDestroyList(&DirectoryList, NULL)) != LLSuccess) {
		return (DSRALListManagementError);
	}

	/*
	 * Return Success.
	 */

	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALShellCleanup
 *
 * DESCRIPTION:
 *  This is the cleanup function for the DSRALArchive function.  It
 *  handles sending the cancel command to the shell and then waiting for
 *  the shell to exit.  It check the return code and returns the
 *  appropriate status code.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, SLSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRALData *           This is a pointer to the Archive Data
 *                         structure.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALShellCleanup(TDSRALData *ArchiveData)
{
	int		ExitStatus;
	int		ExitSignal;

	TDSRALError		ArchiveError;

	if (ArchiveError = DSRALSendCommand(ArchiveData,
	    CANCEL_ARCHIVE)) {
		return (ArchiveError);
	}

	if (PCWait(ArchiveData->PCHandle,
	    &ExitStatus,
	    &ExitSignal)) {
		return (DSRALChildProcessFailure);
	}

	if (PCDestroy(&(ArchiveData->PCHandle))) {
		return (DSRALChildProcessFailure);
	}

	if ((ExitStatus - ArchiveData->ReplacementErrorCount) > 0 ||
	    ExitSignal > 0) {
		return (DSRALChildProcessFailure);
	}

	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALProcessFile
 *
 * DESCRIPTION:
 *  This function handles the reading and writing of the control and
 *  recovery files.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, SLSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  char *                 This is the path to the control file.
 *  TDSRALData *           This is a pointer to the Archive Data
 *                         structure.
 *  TBoolean               This flag is set to true to read the recovery
 *                         file and false to write it.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALProcessFile(char *FilePath, TDSRALData *ArchiveData, TBoolean ReadFile)
{
	char State[PATH_MAX];
	FILE *ProcessFILE;
	char Buffer[PATH_MAX];
	char *CharPtr;
	int i = 0;
	int TmpInt;

	if (ReadFile) {

		if (!(ProcessFILE = fopen(FilePath, "r"))) {
			return (DSRALProcessFileFailure);
		}

		/*
		 * While there are lines to read in the file
		 */

		while (!feof(ProcessFILE)) {

			/*
			 * If fgets() returned data
			 */

			if (fgets(Buffer, sizeof (Buffer), ProcessFILE)) {
				switch (i) {
				case 0:
					(void) sscanf(Buffer,
					    "MEDIA = %d\n",
					    &ArchiveData->Media);
					break;
				case 1:
					(void) sscanf(Buffer,
					    "MEDIA_STRING = %s\n",
					    ArchiveData->MediaString);
					break;
				case 2:
					(void) sscanf(Buffer,
					    "IS_DEVICE = %d\n",
					    &TmpInt);
					ArchiveData->IsDevice = (char)TmpInt;
					break;
				case 3:
					(void) sscanf(Buffer,
					    "BYTES_TO_TRANSFER = %llu\n",
					    &ArchiveData->BytesToTransfer);
					break;
				case 4:
					(void) sscanf(Buffer,
					    "STATE = %s\n",
					    State);
					break;
				default:
					break;
				}
				i++;
			}
		}

		(void) fclose(ProcessFILE);

		/*
		 * if the state contained in the file is not equal to the
		 * define DSRAL_GENERATE_STATE then the file is either
		 * incomplete or corrupt, so punt.
		 */

		if (strcmp(State, DSRAL_GENERATE_STATE) != 0) {
			return (DSRALProcessFileFailure);
		}

	} else {

		/*
		 * First check to see if the directory that the calling
		 * application wants to write to exists.
		 */

		(void) strcpy(Buffer, FilePath);
		CharPtr = dirname(Buffer);
		if (access(CharPtr, X_OK) != 0) {

			/*
			 * If not, then create the directory
			 */

			if (_create_dir(CharPtr) != NOERR)
				return (DSRALProcessFileFailure);
		}

		if (!(ProcessFILE = fopen(FilePath, "w"))) {
			return (DSRALProcessFileFailure);
		}

		(void) fprintf(ProcessFILE,
		    "MEDIA = %d\n",
		    ArchiveData->Media);
		(void) fprintf(ProcessFILE,
		    "MEDIA_STRING = %s\n",
		    ArchiveData->MediaString);
		(void) fprintf(ProcessFILE,
		    "IS_DEVICE = %d\n",
		    ArchiveData->IsDevice);
		(void) fprintf(ProcessFILE,
		    "BYTES_TO_TRANSFER = %llu\n",
		    ArchiveData->BytesToTransfer);
		(void) fprintf(ProcessFILE,
		    "STATE = %s\n",
		    DSRAL_GENERATE_STATE);

		(void) fclose(ProcessFILE);
	}

	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALComputeArchiveSize
 *
 * DESCRIPTION:
 *  This function takes in the path to a file and determines if it's
 *  size should be accounted for in the archive.  If so then the file
 *  size returned by stat() is added to the current archive size.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, SLSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  char *                 This is the path to the file.
 *  unsigned long long *   A pointer to the archive size variable.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALComputeArchiveSize(char *FilePath, unsigned long long *ArchiveSize)
{
	struct stat StatBuf;

	if (lstat(FilePath, &StatBuf) < 0) {
		return (DSRALUnableToStatPath);
	}

	switch (StatBuf.st_mode & S_IFMT) {
	case S_IFDIR:
	case S_IFREG:
	case S_IFIFO:
	case S_IFLNK:
	case S_IFSOCK:
	case S_IFDOOR:
		*ArchiveSize += StatBuf.st_size;
		break;
	case S_IFBLK:
	case S_IFCHR:
	default:
		break;
	}
	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALParseMedia
 *
 * DESCRIPTION:
 *  This function takes in the Media and MediaString and parses it
 *  into it's components.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, SLSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRALMedia            The enumerated value for the type
 *                         of media to be used for the archive.
 *  char *                 The string that defines the
 *                         specified media.  The following
 *                         is the format for each allowed
 *                         media type.
 *                           FLOPPY: Path to floppy device
 *                            (e.g. /dev/rdiskette0)
 *                           TAPE: Path to tape device
 *                            (e.g. /dev/rmt/0)
 *                           DISK: Either a path to mounted directory
 *                                 or a path to a block device.
 *                            (e.g. /export/tmp or /dev/dsk/c0t3d0s0)
 *                           NFS: Path to NFS directory
 *                            (e.g. whistler:/export/home/vos)
 *                           RSH: Path to remote directory
 *                            (e.g. vos@whistler:/export/home/vos)
 *  char *                 The User name if appropriate.
 *  char *                 The Machine name if appropriate.
 *  char *                 The Path to the media.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALParseMediaString(TDSRALMedia Media,
    char *MediaString,
    char *User,
    char *Machine,
    char *Path)
{
	char		*Offset;

	/*
	 * Initialize all of the strings to NULL
	 */

	User[0] =	'\0';
	Machine[0] =	'\0';
	Path[0] =	'\0';

	switch (Media) {
	case DSRALFloppy:
	case DSRALTape:
	case DSRALDisk:
		(void) strcpy(Path, MediaString);
		break;
	case DSRALNFS:

		/*
		 * Search for the ":" in the media string to find the break
		 * betweent the machine name and the path.
		 */

		(void) strcpy(Machine, MediaString);
		if (!(Offset = strstr(Machine, ":"))) {

			/*
			 * A Machine name must be specified so we're out of
			 * here
			 */

			return (DSRALNoMachineName);
		}
		*Offset = '\0';
		Offset++;

		/*
		 * Copy in the name of the file that they want us to back up
		 * to
		 */

		(void) strcpy(Path, Offset);

		/*
		 * Now run the compiled path through the canonizing function
		 * to remove any redundant characters.
		 */

		break;
	case DSRALRsh:

		/*
		 * See if there is an @ in the archive string.	If so then
		 * there is a user name provided for the remote machine.
		 */

		Offset = MediaString;
		if ((strstr(MediaString, "@"))) {

			/*
			 * The backup device provided a specific user name
			 */

			(void) strcpy(User, MediaString);
			Offset = strstr(User, "@");
			*Offset = '\0';

			Offset++;
		}

		/*
		 * Search for the ":" in the media string to find the break
		 * betweent the machine name and the path.
		 */

		(void) strcpy(Machine, Offset);
		if (!(Offset = strstr(Machine, ":"))) {

			/*
			 * A Machine name must be specified so we're out of
			 * here
			 */
			return (DSRALNoMachineName);
		}
		*Offset = '\0';
		Offset++;

		(void) strcpy(Path, Offset);
		break;
	default:
		return (DSRALInvalidMedia);
	}
	return (DSRALSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALRemoveArchiveFiles
 *
 * DESCRIPTION:
 *  This function handles cleaning up (removing) any files generated
 *  as a part of the archive process.
 *
 * RETURN:
 *  TYPE                   DESCRIPTION
 *  TDSRALError            This is the enumerated error
 *                         code defined in the public
 *                         header.  Upon success, SLSuccess
 *                         will be returned.  Upon error,
 *                         the appropriate error code will
 *                         returned.
 *
 * PARAMETERS:
 *  TYPE                   DESCRIPTION
 *  TDSRALData *           This is a pointer to the Archive Data
 *                         structure.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALRemoveArchiveFiles(TDSRALData *ArchiveData)
{
	char		User[PATH_MAX];
	char		Machine[PATH_MAX];
	char		Path[PATH_MAX];
	char		TmpBuffer[PATH_MAX];
	char		TmpBuffer1[PATH_MAX];
	struct utsname  SysInfo;

	TDSRALError	ArchiveError;

	/*
	 * Parse the Media String
	 */

	if ((ArchiveError = DSRALParseMediaString(ArchiveData->Media,
	    ArchiveData->MediaString,
	    User,
	    Machine,
	    Path))) {
		return (ArchiveError);
	}

	/*
	 * Get the system's info structure so that the name of the
	 * machine can be used to brand the archive to keep from having
	 * name collisions
	 */

	if (uname(&SysInfo) == -1) {
		return (DSRALSystemCallFailure);
	}

	switch (ArchiveData->Media) {

	/*
	 * If the media is either tape of floppy then there is not
	 * a file to remove.
	 */

	case DSRALFloppy:
	case DSRALTape:
		TmpBuffer[0] = '\0';
		break;

	/*
	 * For the remaining media types the archive file can be removed.
	 */

	case DSRALDisk:
		if (ArchiveData->IsDevice) {
			(void) sprintf(TmpBuffer,
			    "\\rm -f %s/%s.%s.Z",
			    DSRAL_DIRECTORY_MOUNT_POINT,
			    SysInfo.nodename,
			    DSRAL_ARCHIVE_FILE);
		} else {
			(void) sprintf(TmpBuffer,
			    "\\rm -f %s/%s.%s.Z",
			    Path,
			    SysInfo.nodename,
			    DSRAL_ARCHIVE_FILE);
		}
		break;

	case DSRALNFS:
		(void) sprintf(TmpBuffer,
		    "\\rm -f %s/%s.%s.Z",
		    DSRAL_DIRECTORY_MOUNT_POINT,
		    SysInfo.nodename,
		    DSRAL_ARCHIVE_FILE);
		break;

	case DSRALRsh:

		(void) sprintf(TmpBuffer1,
		    "%s/%s.%s.Z",
		    Path,
		    SysInfo.nodename,
		    DSRAL_ARCHIVE_FILE);


		/*
		 * Now run the compiled path through the canonizing
		 * function to remove any redundant characters.
		 */

		canoninplace(TmpBuffer1);

		/*
		 * If a user name was specified
		 */

		if (strlen(User)) {
			(void) sprintf(TmpBuffer,
			    "rsh -l %s %s \"\\rm -f %s\" 1>/dev/null 2>/dev/null\n",
			    User,
			    Machine,
			    TmpBuffer1);
		}

		/*
		 * Otherwise a user name was not given
		 */

		else {
			(void) sprintf(TmpBuffer,
			    "rsh %s \"\\rm -f %s\" 1>/dev/null 2>/dev/null\n",
			    Machine,
			    TmpBuffer1);
		}
		break;
	default:
		return (DSRALInvalidMedia);
	}

	/*
	 * If there is a command to issue to remove the archive.
	 */

	if (strlen(TmpBuffer)) {

		/*
		 * Issue the command to remove the archive file from the
		 * media.
		 */

		if (system(TmpBuffer)) {
			return (DSRALSystemCallFailure);
		}
	}

	/*
	 * Remove the archive file.  I don't care if this errors because if
	 * the file does not exist then I'm just as happy.
	 */

	(void) unlink(DSRAL_ARCHIVE_LIST_PATH);

	/*
	 * Remove the recovery file and the control file
	 * since all of the DSR work is complete.
	 */

	(void) strcpy(TmpBuffer, get_rootdir());
	(void) strcat(TmpBuffer, DSRAL_RECOVERY_RESTORE_PATH);
	canoninplace(TmpBuffer);

	if (unlink(TmpBuffer)) {
		return (DSRALSystemCallFailure);
	}

	if (unlink(DSRAL_CONTROL_PATH)) {
		return (DSRALSystemCallFailure);
	}

	if (unlink(DSRAL_RECOVERY_BACKUP_PATH)) {
		return (DSRALSystemCallFailure);
	}
	return (DSRALSuccess);
}
