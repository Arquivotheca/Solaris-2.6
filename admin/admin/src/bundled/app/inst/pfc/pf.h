#ifndef lint
#pragma ident "@(#)pf.h 1.25 96/09/03 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pf.h
 * Group:	ttinstall
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
 *
 *		FUNCTIONS (by file)
 *			pfvalidate.c
 *			pfcalc.c
 *			pfdata.c
 * 			pfparse.c
 *			pfio.c
 *			pfutil.c
*/

/* ==================================================INCLUDES======== */

#include "spmitty_api.h"
#include "spmicommon_api.h"
#include "spmistore_api.h"
#include "spmisoft_api.h"
#include "spmisvc_api.h"
#include "spmiapp_api.h"
#include "inst_parade.h"

/* ==================================================CONSTANTS======== */
#define	TTINSTALL_NAME	"TTINSTALL"
#define	CUI_DEBUG	LOGSCR, debug, TTINSTALL_NAME, DEBUG_LOC
#define	CUI_DEBUG_NOHD	LOGSCR, debug, NULL, DEBUG_LOC
#define	CUI_DEBUG_L1	CUI_DEBUG, LEVEL1
#define	CUI_DEBUG_L1_NOHD	CUI_DEBUG_NOHD, LEVEL1

#ifdef INSTALL_GUI
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

/*
 * remotefs node (see ../../include/sw_lib.h) (structure is use by ibe)
 */
typedef struct remote_fs pfRem_t;


/* simple char * node */
typedef struct name_list {
	char *name;
	struct name_list *next;
} pfName_t;


/* ======================================================== GLOBALS */

/* ========================================================FUNCTIONS= */

/* pferror.c */
extern void pfDiskErr(char *, int);

/* inst_fs_preserve.c */
extern int has_preservable_fs();

/* v_pfg_sw.c */
extern pfSw_t *pfGetPackageList();
extern pfSw_t *pfGetClusterList();
extern void clearPackClustSelects();
extern int pfSW_add(pfSw_t **, pfSw_t **, Module *);
extern int getModuleSize(Module * module, ModStatus status);
extern char *pkgid_from_pkgdir(char *path);
extern int get_total_kb_to_install(void);
extern int get_size_in_kbytes(char *pkgid);
extern void resetPackClustSelects();
extern pfErCode pfInitializeSw();
extern Module *pfGetCurrentMeta();
extern void pfSetMetaCluster(Module * module);
extern int setDefaultLocale(char *loc);
extern void initNativeArch(void);
extern void setSystemType(MachineType type);
extern int getNumClients();
extern int getSwapPerClient();
extern void setNumClients(int numClients);
extern void setSwapPerClient(int swap);
extern char *pfPackagename(char *pkgid);
extern char *pfClustername(char *pkgid);

/* v_pfg_rfs.c */

extern char *getDefaultRfs(char *local);
extern pfErCode pfValidRem(pfRem_t *);	/* return pfOK or error value */
extern int pfIsValidRemoteOptions(char *);	/* returns 0 if preserve used */
extern int pfIsValidMountPoint(char *); /* 0 == not swap nor begins with "/" */
extern int pfIsValidIPAddr(char *);	/* 0 = not a valid ip address */
extern pfErCode pfValidMountPoint(char *name);	/* does name validation */

/*
 * following functions add a new element (second arg) to the end of the
 * 	list (first arg == head)
 */
extern pfErCode pfAppendName(pfName_t ** head, pfName_t * newbee);
extern pfErCode pfAppendRem(pfRem_t ** head, pfRem_t * newbee);

extern pfName_t *pfNewName(char *name);
extern void pfSetRemoteFS(pfRem_t *remotes);
extern pfRem_t *pfGetRemoteFS();
extern pfRem_t *pfNewRem(TestMount c_test_mounted, char *c_mnt_pt,
	char *c_hostname, char *c_ip_addr, char *c_export_path,
	char *c_mount_opts);
extern pfRem_t *pfDupRem(pfRem_t *rem);
extern pfRem_t *pfDupRemList(pfRem_t *rem);
extern void pfFreeRem(pfRem_t *rem);
extern void pfFreeNameList(pfName_t *name);
extern void pfFreeRemList(pfRem_t *rem);

/* v_pfg_disks.c */
extern int IsDiskModified(char *diskName);
extern void pfgNullDisks();
extern int pfgIsBootSelected();
extern void pfgResetDisks();
extern void pfgCommitDisks();
extern int pfgIsBootDrive(Disk_t *disk);
extern void saveDiskConfig(Disk_t *diskPtr);
extern void restoreDiskConfig(Disk_t *diskPtr);
extern void pfgLoadExistingDisk(Disk_t *diskPtr);
extern int pfgInitializeDisks();

/* v_pfg_fdisk.c */
extern int useEntireDisk(Disk_t *diskPtr);
extern int useLargestPart(Disk_t *diskPtr);
extern void getLargestPart(Disk_t *diskPtr, int *maxSize, int *part);

#endif	INSTALL_GUI

/* ======================================================== GLOBALS */
extern Profile *pfProfile;
extern unsigned int pfgState;	/* state definitions in spmiapp_api.h */
extern UpgOs_t *UpgradeSlices;
extern FSspace **FsSpaceInfo;
extern tty_MsgAdditionalInfo ttyInfo;
extern int DebugDest;
extern int child_signal_exit;
extern char *StatusScrFileName;
extern char *ErrWarnLogFileName;

/* DSR related stuff */
extern TList DsrSLHandle;
extern TDSRArchiveList DsrALHandle;

/* ========================================================FUNCTIONS= */

/* inst_dsr_al.c */
extern int dsr_al_progress_cb(void *client_data, void *call_data);

/* inst_dsr_analyze.c */
extern int pfc_upgrade_progress_cb(void *mydata, void *progress_data);

/* pfgprocess.c */
extern void pfgSetAction(parAction_t action);
extern parAction_t pfgGetAction(void);
extern void pfcChildShutdown(TChildAction exit_code);
extern void pfcCleanExit(int exit_code, void *exit_data);

/* inst_check.c */
extern int show_small_part(void);
extern int show_disk_warning();
extern int show_sw_depends();

/* v_lfs.c */
extern void v_restore_current_default_fs();
extern void v_save_current_default_fs();
extern int v_get_default_fs_req_size(int);
extern int v_get_default_fs_sug_size(int);

/* v_pfg_disks.c */
extern void pfCheckDisks(void);

/* v_pfg_lfs.c */
extern int any_preservable_filesystems(void);
extern void pfgNullUnpres();
extern void pfgResetNames();
extern void pfgResetDefaults();
extern void pfgSetManualDefaultMounts();
extern void pfgBuildLayoutArray();
extern void pfgCompareLayout();

#endif	/* _PF_H_ */
