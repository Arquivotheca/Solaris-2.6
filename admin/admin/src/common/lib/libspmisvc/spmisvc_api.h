#ifndef lint
#pragma ident "@(#)spmisvc_api.h 1.55 96/08/27 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	spmisvc_api.h
 * Group:	libspmisvc
 * Description:
 */

#ifndef _SPMISVC_API_H
#define	_SPMISVC_API_H

#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <nss_dbdefs.h>
#include "spmistore_api.h"
#include "spmisoft_api.h"
#include "spmicommon_api.h"

/* compatibility type definitions */

typedef Remote_FS	Dfs;

/* constants */

/* local file systems */
#define	USR		"/usr"
#define	USROWN		"/usr/openwin"
#define	USRPLATFORM	"/usr/platform"
#define	OPT		"/opt"
#define	VAR		"/var"
#define	EXPORT		"/export"
#define	EXPORTROOT	"/export/root"
#define	EXPORTEXEC	"/export/exec"
#define	EXPORTSWAP	"/export/swap"
#define	EXPORTHOME	"/export/home"
#define	HOME		"/home"
#define	CACHESWAP	"/.cache/swap"

/* default size status field values */
#define	DFLT_IGNORE	0
#define	DFLT_SELECT	1
#define	DFLT_DONTCARE	2

/* swap size summarization constraint flags */
#define	SWAPALLOC_RESONLY	0x01
#define	SWAPALLOC_RESEXCLUDE	0x02
#define	SWAPALLOC_ALL		(SWAPALLOC_RESONLY | SWAPALLOC_RESEXCLUDE)

/* default size roll-up specifiers */
#define	DONTROLLUP	0
#define	ROLLUP		1

typedef enum {
	ADOPT_UNDEFINED = 0,	/* undefined placeholder */
	ADOPT_ALL,
	ADOPT_NONE
} Adopt_t;

/* return statuses */
#define	NOERR		0
#define	ERROR		1

/* input to set_percent_free_space */
#define	NO_EXTRA_SPACE	-1
/* data structures */

/* -------------------------------------------------------------- */
/*		system resource data elements			  */
/* -------------------------------------------------------------- */

/*
 * sizing specifier - default versus minimum
 */
typedef enum {
	RESSIZE_UNDEFINED = 0,	/* undefined placeholder */
	RESSIZE_DEFAULT,	/* default sizing  with overhead */
	RESSIZE_MINIMUM,	/* minimum size with overhead */
	RESSIZE_NONE		/* no overhead addition */
} ResSize_t;

/*
 * Resource type specifier identifying exact type of a given resource
 */
typedef enum {
	RESTYPE_UNDEFINED = 0,		/* undefined placeholder 	*/
	RESTYPE_DIRECTORY,		/* file system directory 	*/
	RESTYPE_SWAP,			/* swap device or swap file 	*/
	RESTYPE_UNNAMED			/* anonymous resource 		*/
} ResType_t;

/*
 * Resource storage requirements status indicating whether or not
 * the resource is supported, and if independent storage is required
 * during configuration
 */
typedef enum {
	RESSTAT_DEPENDENT   = 0,	/* old DFLT_UNSELECT		*/
	RESSTAT_INDEPENDENT = 1,	/* old DFLT_SELECT 		*/
	RESSTAT_OPTIONAL    = 2,	/* old DFLT_DONTCARE 		*/
	RESSTAT_IGNORED,		/* new parameter 		*/
	RESSTAT_UNDEFINED		/* undefined placeholder 	*/
} ResStat_t;

/*
 * Resource modification permission indicating if the status of the
 * resource can be modified, and by whom
 */
typedef enum {
	RESMOD_SYS	 = 0,	/* "not allowed" - system modify only 	*/
	RESMOD_ANY	 = 1,	/* "allowed" - anyone can modify 	*/
	RESMOD_UNDEFINED	/* undefined placeholder 		*/
} ResMod_t;

/*
 * Resource creation origin specifier indicating who or when the
 * resource object was created.
 */
typedef enum {
	RESORIGIN_UNDEFINED = 0,	/* undefined placeholder	*/
	RESORIGIN_DEFAULT,		/* created as a default		*/
	RESORIGIN_APPEXT		/* created by an application	*/
					/* call				*/
} ResOrigin_t;

/*
 * Resource data storage class
 */
typedef enum {
	RESCLASS_UNDEFINED = 0,	/* undefined placeholder 		*/
	RESCLASS_DYNAMIC,	/* content size is expected to 		*/
				/* continuously change 			*/
	RESCLASS_REPOSITORY,	/* content size is block dynamic 	*/
	RESCLASS_STATIC		/* content size not expected to change 	*/
				/* significantly 			*/
} ResClass_t;

/*
 * Resource attributes used to identify specific attributes of a
 * given resource in get/set calls
 */
typedef enum {
	RESOBJ_UNDEFINED = 0,	/* RESTRICTED */
	RESOBJ_NAME,		/* RESTRICTED: read-only/write-on-create */
	RESOBJ_INSTANCE,	/* RESTRICTED: read-only/write-on-create */
	RESOBJ_ORIGIN,		/* RESTRICTED: read-only */
	RESOBJ_NEXT,		/* RESTRICTED */
	RESOBJ_STATUS,
	RESOBJ_STATUS_STANDALONE,  /* RESTRICTED */
	RESOBJ_STATUS_SERVER,	   /* RESTRICTED */
	RESOBJ_STATUS_AUTOCLIENT,  /* RESTRICTED */
	RESOBJ_MODIFY,		   /* RESTRICTED: read-only (sys dependent) */
	RESOBJ_MODIFY_STANDALONE,  /* RESTRICTED */
	RESOBJ_MODIFY_SERVER,	   /* RESTRICTED */
	RESOBJ_MODIFY_AUTOCLIENT,  /* RESTRICTED */
	RESOBJ_CONTENT_CLASS,
	RESOBJ_CONTENT_SOFTWARE,   /* RESTRICTED: read-only */
	RESOBJ_CONTENT_EXTRA,
	RESOBJ_CONTENT_SERVICES,
	RESOBJ_DEV_DFLTDEVICE,	   /* RESTRICTED: read-only */
	RESOBJ_DEV_PREFDISK,
	RESOBJ_DEV_PREFDEVICE,
	RESOBJ_DEV_EXPLDISK,
	RESOBJ_DEV_EXPLDEVICE,
	RESOBJ_DEV_EXPLSTART,
	RESOBJ_DEV_EXPLSIZE,
	RESOBJ_DEV_EXPLMINIMUM,
	RESOBJ_FS_ACTION,
	RESOBJ_FS_MINFREE,
	RESOBJ_FS_PERCENTFREE,
	RESOBJ_FS_MOUNTOPTS
} ResobjAttr_t;

/*
 * File system action specifiers indicating what action should
 * be taken when processing the back-end code for directory
 * resource objects which have their own storage allocated
 */
typedef enum {
	FSACT_UNDEFINED = 0,	/* undefined placeholder */
	FSACT_CREATE,		/* newfs file system */
	FSACT_CHECK		/* fsck file system */
} FsAction_t;

/*
 * Global resource identifier list used in get/set routines for
 * global resources
 */
typedef enum {
	GLOBALOBJ_UNDEFINED	= 0,	/* undefined placeholder */
	GLOBALOBJ_SWAP		= 1,	/* explicit swap specifier */
	GLOBALOBJ_LAST		= 2	/* MUST BE THE LAST ENTRY */
} GlobalAttr_t;

/*
 * Resource object handle type used by applications when manipulating
 * any resource objects with defined methods
 */
typedef void *	    ResobjHandle;

/*
 * (OBSOLETE) Old default mount entry data structure used to get/set
 * status and size attributes of default resource objects. Required
 * for backward compatibility
 */
typedef struct {
	char	name[MAXNAMELEN];	/* resource name */
	int	status;			/* selection status */
	int	allowed;		/* status modification flag */
} Defmnt_t;

/*
 * Space structure used to retrieve allocated and required software size
 * information for size checking routine ResobjIsComplete()
 */
typedef struct {
	char 	name[MAXNAMELEN];	/* resource name */
	int	instance;		/* resource instance */
	ulong	required;		/* sectors required by resource */
	ulong	allocated;		/* sectors configured for resource */
} Space;

/*
 * Resource status list element used for retrieving and setting the status
 * of all resources in one shot
 */
typedef struct resstatentry {
	ResobjHandle		resource;
	ResStat_t		status;
	struct resstatentry *	next;
} ResStatEntry;

/*
 * Resource object convenience macros
 */
#define	WALK_RESOURCE_LIST(r, t)  for ((r) = ResobjFirst((t)); \
					(r) != NULL; \
					(r) = ResobjNext((r), (t)))
#define	WALK_DIRECTORY_LIST(r)    for ((r) = ResobjFirst(RESTYPE_DIRECTORY); \
					(r) != NULL; \
					(r) = ResobjNext((r), \
						RESTYPE_DIRECTORY))
#define	WALK_SWAP_LIST(r)	  for ((r) = ResobjFirst(RESTYPE_SWAP); \
					(r) != NULL; \
					(r) = ResobjNext((r), RESTYPE_SWAP))

/*
 * *********************************************************************
 * The following section defines the types associated with the
 * SliceList object.
 * *********************************************************************
 */

#define	SL_GENERATE_INSTANCE -999

#define	SL_SLICE_HAS_INSUFF_SPACE(slentry) \
	((slentry) && (slentry->InVFSTab) && \
	    (slentry)->Space && \
	    (slentry)->Space->fsp_flags & FS_INSUFFICIENT_SPACE)

#define	SL_SLICE_IS_COLLAPSED(slentry) \
	((slentry) && (slentry->InVFSTab) && \
	    (slentry)->Space && \
	    (slentry)->Space->fsp_flags & FS_IGNORE_ENTRY)

/*
 * *********************************************************************
 * Only vfstab entries are available for collapsing.
 * You also cannot collapse ROOT or SWAP even when they
 * are in the vfstab.
 * *********************************************************************
 */

#define	SL_SLICE_IS_COLLAPSEABLE(slentry) \
	((slentry) && (slentry->InVFSTab) && \
	!streq(slentry->MountPoint, SWAP) && \
	!streq(slentry->MountPoint, ROOT))

/*
 * *********************************************************************
 * Define the enumerated types for the states of the slice.
 * *********************************************************************
 */

typedef enum {
	SLFixed = 0x0001,
	SLMoveable = 0x0002,
	SLChangeable = 0x0004,
	SLAvailable = 0x0008,
	SLCollapse = 0x0010
} TSLState;

/*
 * *********************************************************************
 * Define the enumerated types for the File System types.
 * *********************************************************************
 */

typedef enum {
	SLUnknown,
	SLUfs,
	SLSwap
} TSLFSType;

/*
 * *********************************************************************
 * This structure is used to store the name of a slice to be searched
 * along with it's associated device id.
 * *********************************************************************
 */

typedef struct {
	char		MountPoint[PATH_MAX];
	int		MountPointInstance;
	char		SliceName[PATH_MAX];
	TBoolean	InVFSTab;
	TSLFSType	FSType;
	TSLState	State;
	unsigned long	AllowedStates;
	TBoolean	Searched;
	unsigned long	Size;
	FSspace		*Space;
	void		*Extra;
} TSLEntry;

/*
 * *********************************************************************
 * Define the error codes returned by the slice list object.
 * *********************************************************************
 */

typedef enum {
	SLSuccess = 0,
	SLListManagementError,
	SLSystemCallFailure,
	SLMemoryAllocationError,
	SLSliceNotFound,
	SLStateNotAllowed,
	SLFailure
} TSLError;

/*
 * *********************************************************************
 * Define the types of items that the slice list can be sorted by
 * *********************************************************************
 */

typedef enum {
	SLSliceNameAscending,
	SLSliceNameDescending,
	SLMountPointAscending,
	SLMountPointDescending
} TSLSortBy;

/*
 * *********************************************************************
 * The following section defines the types associated with the
 * DSRArchiveList object.
 * *********************************************************************
 */

/*
 * *********************************************************************
 * Define the opaque Disk Space Reallocation handle
 * *********************************************************************
 */

typedef void *TDSRArchiveList;

/*
 * *********************************************************************
 * Define the types of for the supported media.
 * *********************************************************************
 */

typedef enum {
	DSRALFloppy,
	DSRALTape,
	DSRALDisk,
	DSRALNFS,
	DSRALRsh,
	DSRALNoMedia
} TDSRALMedia;

/*
 * *********************************************************************
 * Define a type for the options supported on the archive
 * *********************************************************************
 */

typedef enum {
	DSRALBackup,
	DSRALRestore
} TDSRALOperation;

/*
 * *********************************************************************
 * Define the states for the callback to archive backup/restore.
 * *********************************************************************
 */

typedef enum {
	DSRALBackupBegin,
	DSRALBackupUpdate,
	DSRALBackupEnd,
	DSRALRestoreBegin,
	DSRALRestoreUpdate,
	DSRALRestoreEnd,
	DSRALNewMedia,
	DSRALGenerateBegin,
	DSRALGenerateUpdate,
	DSRALGenerateEnd
} TDSRALState;

/*
 * *********************************************************************
 * Define the union for returning information for each of the above
 * states
 * *********************************************************************
 */

typedef struct {

	/*
	 * This state is used to determine which element of the union
	 * to read.
	 */

	TDSRALState State;
	union {

		/*
		 * If we are beginning a backup
		 */

		struct {
			TDSRALMedia		Media;
			char 			MediaString[PATH_MAX];
		} BackupBegin;

		/*
		 * If we are beginnign a restore
		 */

		struct {
			TDSRALMedia		Media;
			char 			MediaString[PATH_MAX];
		} RestoreBegin;

		/*
		 * If it is a file update during either the backup or
		 * restore phase
		 */

		struct {
			unsigned long		PercentComplete;
			char			FileName[PATH_MAX];
			unsigned long long	BytesToTransfer;
			unsigned long long	BytesTransfered;
		} FileUpdate;

		/*
		 * For a media replacement
		 */

		struct {
			TDSRALOperation		Operation;
			TDSRALMedia		Media;
			char 			MediaString[PATH_MAX];
			unsigned long		MediaNumber;
		} NewMedia;

		/*
		 * If there is an error with the media.
		 */

		struct {
			char			ErrorString[PATH_MAX];
		} MediaError;

		/*
		 * If it is an update during generation of the archive
		 * list.
		 */

		struct {
			unsigned long		PercentComplete;
			char			ContentsFile[PATH_MAX];
			char			FileSystem[PATH_MAX];
		} GenerateUpdate;
	} Data;
} TDSRALStateData;

/*
 * *********************************************************************
 * Define the types of errors returned by the Disk Space Reallocation
 * package.
 * *********************************************************************
 */

typedef enum {
	/*
	 * Successful completion of operation
	 */

	DSRALSuccess,

	/*
	 * System can be recovered from a failure.  This means that
	 * the backup phase run to completion
	 */

	DSRALRecovery,

	/*
	 * The user provided callback returned non-zero
	 */

	DSRALCallbackFailure,

	/*
	 * The reading or writing of the DSR backing store file
	 * failed
	 */

	DSRALProcessFileFailure,

	/*
	 * The called function was unable to allocate memory
	 */

	DSRALMemoryAllocationFailure,

	/*
	 * The TDSRArchiveList handle provided is invalid
	 */

	DSRALInvalidHandle,

	/*
	 * The file_will_be_upgraded() function returned non-zero
	 * during the file upgrade determination logic
	 */

	DSRALUpgradeCheckFailure,

	/*
	 * Means that the TDSRALMedia type provided to the call is
	 * not a valid enumerated type
	 */

	DSRALInvalidMedia,

	/*
	 * Means that the Media provided is not the required character
	 * (raw) device
	 */

	DSRALNotCharDevice,

	/*
	 * If unable to write out to either the tape or
	 * floppy
	 */

	DSRALUnableToWriteMedia,

	/*
	 * Unable to stat a required path
	 */

	DSRALUnableToStatPath,

	/*
	 * Cannot open a remote shell on the specified machine
	 */

	DSRALCannotRsh,

	/*
	 * Unable to open a required directory
	 */

	DSRALUnableToOpenDirectory,

	/*
	 * The permissions on the provided directory do not allow
	 * read/write access
	 */

	DSRALInvalidPermissions,

	/*
	 * The provided disk path is neither a directory or a
	 * character device
	 */

	DSRALInvalidDiskPath,

	/*
	 * The media path provided points to a file system that
	 * will be destroyed as a part of the upgrade.
	 */

	DSRALDiskNotFixed,
	/*
	 * Unable to mount the target directory
	 */

	DSRALUnableToMount,

	/*
	 * The machine name for either a Rsh or NFS media was not
	 * provided
	 */

	DSRALNoMachineName,

	/*
	 * Internal error, not returned by any public function.
	 */

	DSRALItemNotFound,

	/*
	 * The child shell process had a fatal failure
	 */

	DSRALChildProcessFailure,

	/*
	 * The internal list management function had a fatal failure
	 */

	DSRALListManagementError,

	/*
	 * There is insufficient space on the target media
	 */

	DSRALInsufficientMediaSpace,

	/*
	 * An internal system call had a fatal error
	 */

	DSRALSystemCallFailure,

	/*
	 * The type of the file system entry is unknown
	 */

	DSRALInvalidFileType

} TDSRALError;

/*
 * *********************************************************************
 * The following section defines the types associated with the
 * SystemUpdate function.
 * *********************************************************************
 */

typedef enum optype {
	SI_UNDEFINED		= 0,
	SI_INITIAL_INSTALL	= 1,
	SI_UPGRADE		= 2,
	SI_ADAPTIVE		= 3,
	SI_RECOVERY		= 4
} OpType;

/*
 * *********************************************************************
 * Define the callback used by the function to report status.
 * *********************************************************************
 */

typedef enum {
	SoftUpdateBegin,
	SoftUpdatePkgAddBegin,
	SoftUpdatePkgAddEnd,
	SoftUpdateInteractivePkgAdd,
	SoftUpdateEnd
} TSoftUpdateState;

/*
 * *********************************************************************
 * The following structure is used to pass progress update information
 * to the calling application via the registered callback.  This
 * structure is used only for the initial install path through
 * SystemUpdate()
 * *********************************************************************
 */

typedef struct {

	/*
	 * This state is used to determine which element of the union
	 * to read.
	 */

	TSoftUpdateState State;
	union {

		/*
		 * If we are beginning the PkgAdd
		 */

		struct {
			char 	PkgDir[PATH_MAX];
		} PkgAddBegin;

		/*
		 * If we are beginning the PkgAdd
		 */

		struct {
			char 	PkgDir[PATH_MAX];
		} PkgAddEnd;

	} Data;
} TSoftUpdateStateData;

/*
 * *********************************************************************
 * Define the structure for passing the data into the function.
 * *********************************************************************
 */

/*
 * If calling SystemUpdate() to perform the processing for
 * an initial install
 */

typedef struct {
	Module		*prod;
	Dfs		*cfs;
	TCallback	*SoftUpdateCallback;
	void 		*ApplicationData;
} TSUInitialData;


/*
 * If calling SystemUpdate() to perform the processing for
 * an upgrade (non adaptive)
 */

typedef struct {
	TCallback	*ScriptCallback;
	void		*ScriptData;
} TSUUpgradeData;


/*
 * If calling SystemUpdate() to perform the processing for
 * an adaptive upgrade
 */

typedef struct {
	TCallback	*ArchiveCallback;
	void		*ArchiveData;
	TCallback	*ScriptCallback;
	void		*ScriptData;
} TSUUpgradeAdaptiveData;


/*
 * If calling SystemUpdate() to perform the processing for
 * the recovery of an interrupted upgrade
 */

typedef struct {
	TCallback	*ArchiveCallback;
	void		*ArchiveData;
	TCallback	*ScriptCallback;
	void		*ScriptData;
} TSUUpgradeRecoveryData;

/*
 * Define the structure used by the calling application to
 * pass in the operation specific data to SystemUpdate()
 */

typedef struct {

	/*
	 * The operation is used to determine which element of the union
	 * should be used to access the data.
	 */

	OpType Operation;
	union {
		TSUInitialData		Initial;
		TSUUpgradeData		Upgrade;
		TSUUpgradeAdaptiveData 	AdaptiveUpgrade;
		TSUUpgradeRecoveryData	UpgradeRecovery;
	} Info;
} TSUData;

/*
 * *********************************************************************
 * Define the enumerated types for the errors returned by SystemUpdate.
 * *********************************************************************
 */

typedef enum {
	/*
	 * Successful completion
	 */

	SUSuccess,

	/*
	 * Invalid operation provided to SystemUpdate
	 */

	SUInvalidOperation,

	/*
	 * Unable to reset the systems state
	 */

	SUResetStateError,

	/*
	 * Unable to create the list of mount points for the
	 * desired file system layout
	 */

	SUCreateMountListError,

	/*
	 * Unable to commit the desired disk configuration
	 */

	SUSetupDisksError,

	/*
	 * Unable to mount the requested file systems
	 */

	SUMountFilesysError,

	/*
	 * Unable to create the desired target directory
	 */

	SUCreateDirectoryError,

	/*
	 * Unable to install the requested packages on the system
	 */

	SUSetupSoftwareError,

	/*
	 * Unable to setup the /etc/vfstab
	 */

	SUSetupVFSTabError,

	/*
	 * Unable to setup the vfstab.unselected file
	 */

	SUSetupVFSTabUnselectedError,

	/*
	 * Unable to setup the /etc/hosts file
	 */

	SUSetupHostsError,

	/*
	 * Unable to setup the Host ID (Serial number)
	 */

	SUSetupHostIDError,

	/*
	 * Unable to setup the systems devices
	 */

	SUSetupDevicesError,

	/*
	 * Unable to write the boot block to the disk
	 */

	SUSetupBootBlockError,

	/*
	 * Unable to update the boot prom
	 */

	SUSetupBootPromError,

	/*
	 * The Upgrade script failed to complete successfully
	 */

	SUUpgradeScriptError,

	/*
	 * Unable to read the disklist from file
	 */

	SUDiskListError,

	/*
	 * Unable to create an instance of the DSRAL object
	 */

	SUDSRALCreateError,

	/*
	 * Unable to generate the archive
	 */

	SUDSRALArchiveBackupError,

	/*
	 * Unable to restore the archive
	 */

	SUDSRALArchiveRestoreError,

	/*
	 * Unable to destroy the instance of the DSRAL object used
	 * to manipulate the archive
	 */

	SUDSRALDestroyError,

	/*
	 * Unable to unmount the mounted file systems or unable to
	 * delete the swap.
	 */

	SUUnmountError,

	/*
	 * Unable to copy a temporary file to its permanent destination
	 */

	SUFileCopyError,

	/*
	 * A non specific internal error has occured that cannot be
	 * recovered from.
	 */

	SUFatalError
} TSUError;

/*
 * *********************************************************************
 * Define the enumerated types for the states returned by UpgradeResume.
 * *********************************************************************
 */

typedef enum {
	UpgradeResumeNone,
	UpgradeResumeRestore,
	UpgradeResumeScript
} TUpgradeResumeState;

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/* svc_dfltrsrc.c */
int		ResobjSetAttribute(ResobjHandle, ...);
int		ResobjGetAttribute(ResobjHandle, ...);
ResobjHandle	ResobjCreate(ResType_t, char *, int, ...);
int		ResobjDestroy(ResobjHandle);
ResobjHandle	ResobjFind(char *, int);
ResobjHandle	ResobjFirst(ResType_t);
int		ResobjInitList(void);
int		ResobjReinitList(void);
ResobjHandle	ResobjNext(ResobjHandle, ResType_t);
Space *		ResobjIsComplete(ResSize_t);
int 		ResobjUpdateContent(void);
void		ResobjPrintList(void);
void		ResobjPrint(ResobjHandle, char *, int);
int		ResobjIsExisting(ResobjHandle);
int		ResobjGetStatusList(ResStatEntry **);
int		ResobjSetStatusList(ResStatEntry *);
void		ResobjFreeStatusList(ResStatEntry *);
void		ResobjPrintStatusList(ResStatEntry *);

/* svc_dfltmnt.c (OBSOLETE) */
Defmnt_t **	free_dfltmnt_list(Defmnt_t **);
int 		get_dfltmnt_ent(Defmnt_t *, char *);
Defmnt_t **	get_dfltmnt_list(Defmnt_t **);
void		print_dfltmnt_list(const char *, Defmnt_t **);
int		set_dfltmnt_ent(Defmnt_t *, char *);
int		set_dfltmnt_list(Defmnt_t **);

/* svc_fdiskconfig.c */
int		FdiskobjConfig(Layout_t, Disk_t *, char *);

/* svc_global.c */
int		GlobalSetAttribute(GlobalAttr_t, void *);
int		GlobalGetAttribute(GlobalAttr_t, void *);

/* svc_resource.c */
int		get_default_fs_size(char *, Disk_t *, int);
int		get_minimum_fs_size(char *, Disk_t *, int);
int		set_client_space(int, int, int);
int		ResobjGetContent(ResobjHandle, Adopt_t, ResSize_t);
int		ResobjGetStorage(ResobjHandle, Adopt_t, ResSize_t);
int		ResobjGetSwap(ResSize_t);
int		SliceobjSumSwap(Disk_t *, u_char);

/* svc_sdiskconfig.c */
int		SdiskobjConfig(Layout_t, Disk_t *, char *);
int		SdiskobjAutolayout(void);

/* svc_sdiskfreespace.c */
void		SdiskobjAllocateUnused(Disk_t *);

/* svc_updatesys.c */
TSUError 	SystemUpdate(TSUData *SUData);
char *		SUGetErrorText(TSUError);

/* svc_upgradeable.c */
void		SliceFindUpgradeable(StringList **, StringList **);

/* svc_do_upgrade.c */
int		gen_upgrade_script(void);
void		log_spacechk_failure(int);
void		MakePostKBIDirectories(void);
int		SetupPreKBI(void);
int		execute_upgrade(OpType, int (*)(void *, void *), void *);
void		rm_link_mv_file(char *, char *);

/* svc_mountall.c */
int		mount_and_add_swap(char *);
int		mount_and_add_swap_from_vfstab(char *);
int		umount_and_delete_swap(void);
int		umount_all(void);
int		unswap_all(void);
void		set_profile_upgrade(void);
char *   	get_failed_mntdev(void);

/* svc_sp_print_results.c */
void	print_final_results(FSspace **, char *);
SW_space_results	*gen_final_space_report(FSspace **);

/* svc_fs_space.c */
int	calc_partition_size(FSspace **, int (*)(void *, void *), void *);
int	verify_fs_layout(FSspace **, int (*)(void *, void *), void *);
int	percent_free_space(void);
void	set_percent_free_space(int);
FSspace ** space_meter(char **);

/* svc_service_free.c */
void		free_service_list(SW_service_list *);
void		free_error_info(SW_error_info *);
void		free_createroot_info(SW_createroot_info *);

/* svc_services.c */
SW_service_list	*list_available_services(char *, SW_error_info **);
SW_service_list	*list_installed_services(SW_error_info **);
StringList	*list_avail_svc_platforms(char *, SW_service *,
    SW_error_info **);
StringList	*list_installed_svc_platforms(SW_service *, SW_error_info **);
int		validate_service_modification(SW_service_modspec *,
    SW_error_info **);
int		execute_service_modification(SW_service_modspec *,
    SW_error_info **);
SW_createroot_info * get_createroot_info(SW_service *, SW_error_info **);

/* svc_sp_free_results.c */
void		free_final_space_report(SW_space_results *);

/* svc_setaction.c */
void		set_action_for_machine_type(Module *);

/* svc_upg_recover.c */
TUpgradeResumeState 	UpgradeResume(void);

/* svc_slice_list.c */

void
SLSetAllowedStates(TSLEntry *Entry);

TSLError
SLAdd(TList SliceList,
    char *SliceName,
    char *MountPoint,
    int MountPointInstance,
    TBoolean InVFSTab,
    TSLFSType FSType,
    TSLState State,
    unsigned long Size,
    FSspace *Space,
    void *Extra,
    TSLEntry **SLEntry);

TSLError
SLNameInList(TList SliceList, char *SliceName);

TSLError
SLRemove(TList SliceList, char *SliceName);

TLLError
SLClearCallback(TLLData Data);

char *
SLStateToString(TSLState State);

char *
SLFSTypeToString(TSLFSType FSType);

TSLError
SLPrint(TList SliceList,
	void (*SliceEntryPrintFunction) (TSLEntry *SLEntry));

TSLError
SLSort(TList SliceList, TSLSortBy SortBy);

/* svc_dsr_archive_list.c */

TDSRALError
DSRALCanRecover(TDSRALMedia* Media, char *MediaString);

TDSRALError
DSRALCreate(TDSRArchiveList *Handle);

TDSRALError
DSRALDestroy(TDSRArchiveList *Handle);

TDSRALError
DSRALArchive(TDSRArchiveList Handle,
    TDSRALOperation Operation,
    TCallback *UserCallback,
    void *UserData);

TDSRALError
DSRALSetMedia(TDSRArchiveList Handle,
    TList SliceList,
    TDSRALMedia Media,
    char *MediaString);

TDSRALError
DSRALCheckMediaSpace(TDSRArchiveList Handle);

TDSRALError
DSRALGenerate(TDSRArchiveList Handle,
    TList SliceList,
    TCallback *UserCallback,
    void *UserData,
    unsigned long long *BytesToTransfer);

char *
DSRALGetErrorText(TDSRALError DSRALError);

#ifdef __cplusplus
}
#endif

#endif	/* _SPMISVC_API_H */
