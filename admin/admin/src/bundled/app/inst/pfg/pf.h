#ifndef lint
#pragma ident "@(#)pf.h 1.39 96/07/11 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pf.h
 * Group:	installtool
 * Description:
 */

#ifndef	_PF_H_
#define	_PF_H_

/*
 * pf.h
 *	Profile library & application header file
 *
 *	Contents (use "find" of select, copy /paste):
 *		CONSTANTS
 *			pf->mask
 *			pfValue_t
 *		INCLUDES
 *		TYPES
 *			pfFs_t
 *			pfSw_t
 *			pfName_t
 *			pfMask_t
 *			pf_t
 *			pfDiskArray_t
 *
 *		GLOBALS
 *			pfgTopLevel
 *			pfgAppContext
 *			pfProfile
 *			pfDiskHead
 *			pfDiskArray
 *			debug
			upgradeEnabled
 *			environ
 *			_disk_debug
 *			_ibe_debug
 *			_sw_debug
 *
 *		FUNCTIONS(by file)
 *			pfvalidate.c
 *			pfcalc.c
 *			pfdata.c
 * 			pfparse.c
 *			pfio.c
 *			pfutil.c
*/
/* ==================================================CONSTANTS======== */


/* ==================================================INCLUDES======== */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/param.h>
#include <fcntl.h>
#include <locale.h>
#include <libintl.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/mntent.h>
#include <Xm/Xm.h>

#include "spmicommon_api.h"
#include "spmisoft_api.h"
#include "spmistore_api.h"
#include "spmisvc_api.h"
#include "spmiapp_api.h"
#include "spmixm_api.h"

#include "pferror.h"

/* ========================================================TYPES=== */

/* defines for signifying if a package was to be added or removed */
#define	PF_ADD 1
#define	PF_REMOVE 0
/* localfs node  */
typedef struct FS {
	char		*name;
	char		*dev;
	char		*size;
	char		*mntopts;
	struct FS	*next;
	int		preserve;   /* new */
	unsigned int	change;	    /* new, used when changing partitioning */
} pfFs_t;


/* software package or cluster node */
typedef struct pf_sw_def {
	char *name;
	int delta;
	Module *mod;
	struct pf_sw_def *next;
} pfSw_t;

/* simple char * node */
typedef struct name_list {
	char *name;
	struct name_list *next;
} pfName_t;

/* ======================================================== GLOBALS */
extern Profile *pfProfile;

/*
 * the last option chosen on the client services
 * screen
 * 0 = No Selection (initial setting)
 * 1 = Both Root and Swap entries shown
 * 2 = Root entry shown
 * 3 = Swap entry shown
 */
extern int LastServiceChoice;

/* if non-zero, copious printf */
extern unsigned int debug;
extern int DebugDest;

/* if not 1 then disable upgrade,server */
extern unsigned int upgradeEnabled;

extern char **environ;		/* for exec's (in use?) */

/* ========================================================FUNCTIONS= */

/* pfgutil.c */
extern void unregister_as_dropsite(Widget, XtPointer, XtPointer);

/* pferror.c */
extern void pfDiskErr(char *, int);

/* v_pfg_disks.c */
extern pfErCode pfLoadDisks(int *numDisks);
extern void pfgNullDisks(void);
extern int IsDiskModified(char *diskName);
extern int pfgIsBootSelected(void);
extern void pfgResetDisks(void);
extern void pfgCommitDisks(void);
extern int pfgIsBootDrive(Disk_t *disk);
extern void saveDiskConfig(Disk_t *diskPtr);
extern void restoreDiskConfig(Disk_t *diskPtr);
extern void pfgLoadExistingDisk(Disk_t *diskPtr);
extern int pfgInitializeDisks(void);

/* v_pfg_fdisk.c */
extern void pfPreserveFdisk(void);
extern int useEntireDisk(Disk_t *diskPtr);
extern int useLargestPart(Disk_t *diskPtr);
extern void getLargestPart(Disk_t *diskPtr, int *maxSize, int *part);

/* v_pfg_lfs.c */
extern int any_preservable_filesystems(void);
extern void pfgNullUnpres(void);
extern void pfgResetNames(void);
extern void pfgResetDefaults(void);
extern void pfgSetManualDefaultMounts(void);
extern void pfgBuildLayoutArray(void);
extern void pfgCompareLayout(void);
extern void saveDefaultMountList(MachineType type);

/* v_pfg_rfs.c */
extern char *getDefaultRfs(char *local);
extern pfErCode pfValidRem(Remote_FS *);	/* return pfOK or error value */
extern int pfIsValidRemoteOptions(char *);	/* returns 0 if preserve used */

/* 0 means not swap nor begins with "/" */
extern int pfIsValidMountPoint(char *);

extern int pfIsValidIPAddr(char *);	/* 0 = not a valid ip address */
extern pfErCode pfValidMountPoint(char *name);	/* does name validation */

/*
 * following functions add a new element (second arg) to the end of the
 * 	list (first arg == head)
 */
extern pfErCode pfAppendName(pfName_t ** head, pfName_t * newbee);
extern pfErCode pfAppendRem(Remote_FS ** head, Remote_FS * newbee);

extern pfName_t *pfNewName(char *name);
extern void pfSetRemoteFS(Remote_FS *remotes);
extern Remote_FS *pfGetRemoteFS(void);
extern Remote_FS *pfNewRem(
	TestMount c_test_mounted, char *c_mnt_pt, char *c_hostname,
	char *c_ip_addr, char *c_export_path, char *c_mount_opts);
extern Remote_FS *pfDupRem(Remote_FS *rem);
extern Remote_FS *pfDupRemList(Remote_FS *rem);
extern void pfFreeRem(Remote_FS *rem);
extern void pfFreeNameList(pfName_t *name);
extern void pfFreeRemList(Remote_FS *rem);

/* v_pfg_sw.c */
extern pfErCode pfLoadCD(void);
extern void pfNullPackClusterLists(void);
extern pfSw_t * pfGetClusterList(void);
extern pfSw_t * pfGetPackageList(void);
extern pfSw_t ** pfGetClusterListPtr(void);
extern pfSw_t ** pfGetPackageListPtr(void);
extern int pfSW_add(pfSw_t ** head, pfSw_t ** curr, Module * module);
extern int getModuleSize(Module * module, ModStatus status);
extern char * pkgid_from_pkgdir(char *path);
extern int get_total_kb_to_install(void);
extern int get_size_in_kbytes(char *pkgid);
extern void resetPackClustSelects(void);
extern pfErCode pfInitializeSw(void);
extern Module * pfGetCurrentMeta(void);
extern void pfSetMetaCluster(Module * module);
extern int setDefaultLocale(char *loc);
extern void initNativeArch(void);
extern void setSystemType(MachineType type);
extern int getNumClients(void);
extern int getSwapPerClient(void);
extern int getRootPerClient(void);
extern void setNumClients(int numClients);
extern void setRootPerClient(int root);
extern void setSwapPerClient(int swap);
extern char * pfPackagename(char *pkgid);
extern char * pfClustername(char *pkgid);

/* v_pfg_upgrade.c */
extern int pfgIsUpgradeable(void);
extern int pfInitUpgradeSw(char *dir);
extern int pfgMultipleOs(void);
extern void pfgSetSelectedDisk(struct disk * currentDisk);
extern Disk_t * pfgGetSelectedDisk(void);
extern void pfgRemoveSelectedDisk(void);
extern struct disk * pfgFirstUpgradeDisk(void);
extern void pfgRemoveUpgradeableDisk(char *name);
extern void SetDiskMounted(int status);
extern int pfgUnmountDisk(void);
extern int pfgResetInitial(void);
extern int pfgInitializeUpgrade(Disk_t *disk);
extern int pfgDoMountsAndSwap(Disk_t *disk);
extern int pfgGetNumberUpgradeDisk(void);

#endif	/* _PF_H_ */
