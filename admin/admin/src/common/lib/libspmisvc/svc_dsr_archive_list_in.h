/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */

#ifndef	lint
#pragma	ident "@(#)svc_dsr_archive_list_in.h 1.18 96/09/10 SMI"
#endif

#ifndef	__SVC_DSR_ARCHIVE_LIST_IN_H
#define	__SVC_DSR_ARCHIVE_LIST_IN_H

#include<limits.h>
#include<sys/types.h>
#include<sys/statvfs.h>
#include<pkgstrct.h>

#include "spmicommon_api.h"
#include "spmisvc_api.h"

#define	DSRAL_INITIALIZED		0xDEADBEEF
#define	DSRAL_MEDIA_REPLACEMENT_TOKEN	"REPLACE_MEDIA"
#define	DSRAL_MEDIA_REPLACEMENT_STRING	"REPLACE_MEDIA %d"
#define	DSRAL_ARCHIVE_LIST_PATH		"/tmp/dsr_archive.list"
#define	DSRAL_CONTROL_PATH		"/tmp/dsr_control.file"
#define	DSRAL_RECOVERY_BACKUP_PATH	"/tmp/dsr_recovery.file"
#define	DSRAL_RECOVERY_RESTORE_PATH	"/var/sadm/system/admin/dsr_recovery.file"
#define	DSRAL_UPGRADE_SCRIPT_PATH	"/a/var/sadm/system/admin/upgrade_script"
#define	DSRAL_UPGRADE_SCRIPT_TMP_PATH	"/tmp/upgrade_script"
#define	DSRAL_USR_PACKAGES_EXIST_PATH	"/a/var/sadm/pkg/SUNWcsu"
#define	DSRAL_DIRECTORY_MOUNT_POINT	"/tmp/dsr"
#define	DSRAL_ARCHIVE_FILE		"dsr_archive.cpio"
#define	DSRAL_GENERATE_STATE		"GENERATE_COMPLETE"
#define	DSRAL_RECOVERY_FILE_SIZE 	512
#define	MEDIA_CONTINUE			0x0D
#define	CANCEL_ARCHIVE			0x03

typedef struct {
	Module	*Mod;
	char	*RootDir;
} TDSRALServiceEntry;

typedef struct {
	char 	Path[PATH_MAX];
} TDSRALDirectoryEntry;

/*
 * *********************************************************************
 * This structure is used to store data specific to the Disk Space
 * Reallocation object.
 * *********************************************************************
 */

typedef struct {
	unsigned int		Initialized;
	unsigned long long	BytesTransfered;
	unsigned long long	BytesToTransfer;
	unsigned long long	TotalFSBytes;
	unsigned long long	FSBytesProcessed;
	FILE			*OutFILE;
	TDSRALMedia		Media;
	TBoolean		IsDevice;
	char			MediaString[PATH_MAX];
	TCallback		*UserCallback;
	void 			*UserData;
	int			ReplacementErrorCount;
	TPCHandle		PCHandle;
} TDSRALData;

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
DSRALValidateHandle(TDSRArchiveList Handle);

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
    char *MediaString);

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
DSRALUnMount(void);

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
    TBoolean *IsDevice);

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
    char *CommandString);

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
 *  TDSRArchiveList        This is the handle to the DSR
 *                         object returned by the call to
 *                         DSRALCreate().
 *  char                   This is the character to be sent to
 *                         shell.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TDSRALError
DSRALSendCommand(TDSRALData *ArchiveData, char Command);

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
DSRALClearServiceList(TLLData Data);

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
DSRALClearDirectoryList(TLLData Data);

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
    TLLData Sorted);

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
    TLLData Sorted);

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
    char *Path);

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
    int Reset);

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
    char *CurrentPath);

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
DSRALShellCleanup(TDSRALData *ArchiveData);

/*
 * *********************************************************************
 * FUNCTION NAME: DSRALProcessFile
 *
 * DESCRIPTION:
 *  This function handles the reading and writing of the process
 *  backing store file which is used to hold state information across
 *  a power failure.
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
DSRALProcessFile(char *FilePath, TDSRALData *ArchiveData, TBoolean ReadFile);

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
DSRALComputeArchiveSize(char *FilePath, unsigned long long *ArchiveSize);

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
    char *Path);

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
DSRALRemoveArchiveFiles(TDSRALData *ArchiveData);

#endif
