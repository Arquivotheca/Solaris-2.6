/*
 * Copyright (c) 1995-1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */
#ifndef lint
#pragma ident "@(#)spmisoft_api.h 1.13 96/10/03 SMI"
#endif

#ifndef _SPMISOFT_API_H
#define	_SPMISOFT_API_H

#include "spmicommon_api.h"
#include "soft_hash.h"

/*
 * for all modules, this is their type
 */
typedef enum modtype
{
	PACKAGE = 0,
	MODULE = 1,
	PRODUCT = 2,
	MEDIA = 3,
	CLUSTER = 4,
	METACLUSTER = 5,
	NULLPRODUCT = 6,
	CATEGORY = 7,
	LOCALE = 8,
	UNBUNDLED_4X = 9,
	ARCH = 10
} ModType;

/*
 * for all modules, this is their state
 */
typedef enum modstate
{
	NOTDUPLICATE = 0,
	DUPLICATE = 1,
	NULLPKG = 2,
	SPOOLED_DUP = 3,
	SPOOLED_NOTDUP = 4
} ModState;

/*
 * for package modules: this is their status
 */
typedef enum modstatus
{
	UNSELECTED = 0,
	SELECTED = 1,
	PARTIALLY_SELECTED = 2,
	LOADED = 3,
	REQUIRED = 4,
	INSTALL_SUCCESS = 5,
	INSTALL_FAILED = 6
} ModStatus;

typedef enum actions
{
	NO_ACTION_DEFINED = 0,
	TO_BE_PRESERVED = 1,
	TO_BE_REPLACED = 2,
	TO_BE_REMOVED = 3,
	TO_BE_PKGADDED = 4,
	TO_BE_SPOOLED = 5,
	EXISTING_NO_ACTION = 6,
	ADDED_BY_SHARED_ENV = 7,
	CANNOT_BE_ADDED_TO_ENV = 8
} Action;

typedef enum environ_action
{
	NO_ENV_ACTION_DEFINED = 0,
	ENV_TO_BE_UPGRADED = 1,
	ADD_SVC_TO_ENV = 2
} Environ_Action;

/* Default file systems */

typedef enum filesys
{
	ROOT_FS = 0,
	USR_FS = 1,
	USR_OWN_FS = 2,
	OPT_FS = 3,
	SWAP_FS = 4,
	VAR_FS = 5,
	EXP_EXEC_FS = 6,
	EXP_SWAP_FS = 7,
	EXP_ROOT_FS = 8,
	EXP_HOME_FS = 9,
	EXPORT_FS = 10
} FileSys;

typedef enum arch_match_type
{
	NO_ARCH_MATCH = 0,
	ARCH_MATCH = 1,
	ARCH_MORE_SPECIFIC = 2,
	ARCH_LESS_SPECIFIC = 3,
	PKGID_NOT_PRESENT = 4,
	ARCH_NOT_SUPPORTED = 5
} Arch_match_type;

#define	N_LOCAL_FS 11

extern char *def_mnt_pnt[];

typedef struct sw_config
{
	struct sw_config *next;
	char		*sw_cfg_name;
	StringList	*sw_cfg_members;
	int		 sw_cfg_auto;
} SW_config;

typedef struct platform
{
	struct platform	*next;
	char		*plat_name;
	char		*plat_uname_id;
	char		*plat_machine;
	char		*plat_group;
	SW_config	*plat_config;
	SW_config	*plat_all_config;
	char		*plat_isa;
} Platform;

typedef struct plat_group
{
	struct plat_group *next;
	char		*pltgrp_name;
	Platform	*pltgrp_members;
	SW_config	*pltgrp_config;
	SW_config	*pltgrp_all_config;
	char		*pltgrp_isa;
	int		 pltgrp_export;
} PlatGroup;

typedef struct hw_config
{
	struct hw_config *next;
	char		*hw_node;
	char		*hw_testprog;
	char		*hw_testarg;
	StringList	*hw_support_pkgs;
} HW_config;

typedef struct arch
{
	char		*a_arch;	/* a unique architecture...    */
	int		 a_selected;	/* load support for this arch? */
	int		 a_loaded;	/* is support for this arch loaded? */
	StringList	*a_platforms;	/* platforms in this group	*/
	struct arch	*a_next;
}Arch;

/* Locale structures referenced by locale Modules associated with products */

typedef struct locale
{
	char		*l_locale;	/* the locale string		*/
	char		*l_language;	/* the language of the locale   */
	int		 l_selected;	/* selected for installation ?  */
}Locale;


typedef struct l10n
{
	struct modinfo *l10n_package;	/* ptr to package that localizes us */
	struct l10n    *l10n_next;
}L10N;

typedef struct pkgs_localized
{
	struct pkgs_localized	*next;
	struct modinfo		*pkg_lclzd;
} PkgsLocalized;

typedef struct depend
{
	char		*d_pkgid;	/* what package?	  */
	char		*d_pkgidb;	/* dependant package	  */
	char 		*d_version;	/* which package version? */
	char 		*d_arch;	/* which architecture?    */
	struct depend 	*d_next;
	struct depend 	*d_prev;
}Depend;

/*
 * the modinfo structure contains per cluster or per package information
 */

typedef struct contents_breakdown {
	ulong	contents_packaged;
	ulong	contents_nonpkg;
	ulong	contents_devfs;
	ulong	contents_savedfiles;
	ulong	contents_pkg_ovhd;
	ulong	contents_patch_ovhd;
	ulong	contents_inodes_used;
} ContentsBrkdn;

typedef struct contents_record {
	struct contents_record	*next;
	int	ctsrec_idx;
	ContentsBrkdn	ctsrec_brkdn;
} ContentsRecord;

typedef struct modinfo
{
	int		  m_order;	/* Package installation order...     */
	ModStatus	  m_status;	/* {SELECTED | UNSELECTED | PARTIAL} */
	ModState	  m_shared;	/* {SHARED | NOTSHARED}              */
	Action		  m_action;
	int		  m_flags;	/* see flags below		     */
	int		  m_refcnt;	/* number of times selected          */
	int		  m_sunw_ptype;	/* { PTYPE_* (for interface file) }  */
	char		 *m_pkgid;	/* short name of package/cluster     */
	char		 *m_pkginst;	/* short name of package/cluster     */
	char		 *m_pkg_dir;	/* package dir, for interface file   */
	char		 *m_name;	/* full name of package/cluster      */
	char		 *m_vendor;	/* Vendor for package		     */
	char		 *m_version;	/* Version of package		     */
	char		 *m_prodname;	/* Product name			     */
	char		 *m_prodvers;	/* Product version		     */
	char		 *m_arch;	/* Architecture supported by package */
	char		 *m_expand_arch;/* Architecture supported by package */
	char		 *m_desc;	/* description of package	     */
	char		 *m_category;	/* category of package		     */
	char		 *m_instdate;	/* installation date of package	     */
	char		 *m_patchid;	/* patch id			     */
	char		 *m_locale;	/* locale this package localizes for */
	char		 *m_l10n_pkglist;/* list of packages localized by this
					    module; pkgid list separated by ','s */
	L10N		 *m_l10n;	/* pointer to list of available L10Ns*/
					/* for  this package (linked list)   */
	PkgsLocalized	 *m_pkgs_lclzd; /* packages localized by this pkg    */	
	Node		 *m_instances;	/* list of package instances,	     */
	Node		 *m_next_patch;	/* patch specific instances	     */
	struct modinfo	 *m_patchof;	/* package patched by this package   */
	Depend		 *m_pdepends;	/* pre-requisite dependencies	     */
	Depend		 *m_idepends;   /* incompatible packages	     */
	Depend		 *m_rdepends;   /* reverse dependencies		     */
	struct file	**m_text;
	struct file	**m_demo;
	struct file	 *m_install;
	struct file	 *m_icon;
	char		 *m_basedir;	/* package specific default base dir */
	char		 *m_instdir;	/* current installation base directory*/
	struct pkg_hist	 *m_pkg_hist;
	daddr_t		  m_spooled_size;
	ulong		  m_pkgovhd_size;
	daddr_t		  m_deflt_fs[N_LOCAL_FS]; /* fs size for default fs */
	struct filediff	 *m_filediff;	/* list of modified files in this pkg */
	struct patch_num *m_newarch_patches;	/* new architecture patches */
	StringList	 *m_loc_strlist;	/* list of locales	*/
	ContentsRecord	 *m_fs_usage;	/* usage of current file systems  */
} Modinfo;

/* Modinfo flags */

#define	PART_OF_CLUSTER			0x0001
#define	INSTANCE_ALREADY_PRESENT	0x0002
#define	DO_PKGRM			0x0004
#define	IN_ORDER_FCN			0x0008
#define	CONTENTS_GOING_AWAY		0x0010	/*
						 * Set if pkghistory files
						 * has an remove from
						 * cluster entry, or if the
						 * used explicitly
						 * de-selected this package.
						 */
#define IS_UNBUNDLED_PKG		0x0020
#define	UI_SHOW_CHLD			0x0100	/* character install specific flag	*/
#define	UI_SUBMOD_L1			0x0200	/* character install specific flag	*/
#define	UI_SUBMOD_L2			0x0400	/* character install specific flag	*/
#define	UI_SUBMOD_L3			0x0800	/* character install specific flag	*/

typedef struct product
{
	char		*p_name;	/* product name 	*/
	char		*p_version;	/* product version 	*/
	char		*p_rev;		/* product revision 	*/
	ModStatus	 p_status;
	char		*p_id;
	char		*p_pkgdir;
	char		*p_instdir;
	Arch		*p_arches;	/* arch linked list - used to be global all_arches */
	SW_config	*p_swcfg;	/* software configurations	*/
	PlatGroup	*p_platgrp;	/* platform groups		*/
	HW_config	*p_hwcfg;	/* hardware configurations	*/
	List		*p_sw_4x;
	List		*p_packages;	/* used to be global packages */
	List		*p_clusters;	/* used to be global clusters */
	struct module   *p_locale;
	Node		*p_orphan_patch;
	char		*p_rootdir;
	struct module   *p_cur_meta;
	struct module   *p_cur_cluster;
	struct module   *p_cur_pkg;
	struct module   *p_cur_cat;
	struct module   *p_deflt_meta;
	struct module   *p_deflt_cluster;
	struct module   *p_deflt_pkg;
	struct module   *p_deflt_cat;
	struct module   *p_view_from;
	List		*p_view_4x;
	List		*p_view_pkgs;
	List		*p_view_cluster;
	List		*p_view_locale;
	List		*p_view_arches;
	struct product  *p_current_view;
	struct product  *p_next_view;
	struct module  	*p_categories;	/* list of software categories */
	struct patch	*p_patches;
	StringList	*p_modfile_list;
} Product;

/*
 * XXX values used as indices
 * for media type strings
 */
typedef enum media_type {
	ANYTYPE = 0,
	CDROM = 1,
	MOUNTED_FS = 2,
	TAPE = 3,
	FLOPPY = 4,
	REMOVABLE = 5,
	INSTALLED = 6,
	INSTALLED_SVC = 7,
	ENDMTYPE = 8
} MediaType;

typedef enum media_status {
	UNSET = 0,
	MOUNT_CREATE = 1,
	MOUNT_NO_CREATE = 2
} MediaStatus;

typedef enum client_type
{
	DISKLESS = 0,
	CACHEONLY = 1
} ClientType;


/*
 * Media structure -- this can correspond to a directory
 *	on a mounted file system, a CD platter, a floppy
 *	disk, a tape cartridge, etc.  We allocate one of
 *	these structures every time the user looks at a
 *	new piece of media or directory.
 */
typedef struct media {
	MediaType	 med_type;	/* type, status */
	int		 med_status;
	int		 med_machine;	/* machine type */
	char		*med_device;	/* device name, e.g., "/dev/sr0c" */
	char		*med_dir;	/* directory or mount point */
	char		*med_volume;	/* volume name for removable media */
	struct module   *med_cur_prod;
	struct module   *med_cur_cat;
	struct module   *med_deflt_prod;
	struct module   *med_deflt_cat;
	int		 med_flags;
	struct module   *med_upg_from;
	struct module   *med_upg_to;
	struct module   *med_cat;
  /*	struct locmap	*med_locmap;	** This is now a global */
	StringList	*med_hostname;
} Media;

/*
 * The category structure is used to maintain the list
 * of categories represented by the software on a piece
 * of media.
 */
typedef struct category {
	char		*cat_name;	/* name of category */
} Category;

typedef struct module
{
	ModType		type;	/* {PACKAGE, CLUSTER, PRODUCT, MEDIA}	*/
	union {
		Modinfo *mod;	/* actual module information		*/
		Product *prod;	/* actual product information		*/
		Media   *media;	/* actual media information		*/
		Locale  *locale;/* actual locale information		*/
		Category *cat;  /* actual locale information		*/
	} info;
	struct module	*next;	/* pointer to next module at this level	*/
	struct module	*prev;	/* pointer to the previous module at this lvl */
	struct module	*sub;	/* pointer to node's first sub-module	*/
	struct module	*head;	/* pointer to head of the this module lvl */
	struct module	*parent; /* pointer to parent module		*/
} Module;

typedef struct view
{
	ModType		 v_type;
	union {
		Modinfo *v_mod;	  /* actual module information		*/
		Locale  *v_locale;/* actual locale information		*/
		Arch 	*v_arch;  /* actual arch information		*/
	} v_info;
	ModStatus 	*v_status_ptr;
	ModStatus	 v_status;
	ModState	 v_shared;
	Action		 v_action;
	int		 v_refcnt;
	char 		*v_instdir;
	int		 v_flags;
	ContentsRecord	*v_fs_usage;
	struct view 	*v_instances;
} View;

typedef enum file_type {
	UNKNOWN = 0,		/* don't known anything about the file */
	TEXTFILE = 1,		/* generic text file, no specific type yet */
	ASCII = 2,		/* ascii text file */
	POSTSCRIPT = 3,		/* postscript text file */
	RUNFILE = 4,		/* generic executable, no specifics yet */
	EXECUTABLE = 5,		/* executable [shell script] */
	ROLLING = 6,		/* "rolling" demo (unimplemented) */
	ICONFILE = 7,		/* generic icon file, unknown specific format */
	X11BITMAP = 8,		/* X11 bitmap icon */
	PIXRECT = 9		/* pixrect bitmap icon */
} FileType;

typedef struct file {
	char		*f_path;	/* path relative to product dir */
	char		*f_name;	/* external name (not used) */
	FileType	 f_type;	/* ascii, postscript, exec, pixrect, */
	char		*f_args;	/* command-line args (for scripts) */
	void		*f_data;	/* file data (icon image, etc.) */
} File;

struct ptype
{
	char		name[25];
	int		namelen;
	char		flag;
};

struct pkg_hist {
	struct pkg_hist	*hist_next;
	char		*replaced_by;
	char		*deleted_files;
	char		*cluster_rm_list;
	char		*ignore_list;
	int		 to_be_removed;
	int		 needs_pkgrm;
	int		 ref_count;
};

struct patch_num {
	struct	patch_num	*next;
	char			*patch_num_id;
	char			*patch_num_rev_string;
	unsigned int		patch_num_rev;
};

#define	DIFF_MISSING		0x00000001
#define	DIFF_MISSING_LINK	0x00000002
#define	DIFF_TYPE		0x00000004
#define	DIFF_SLINK_TARGET	0x00000008
#define	DIFF_HLINK_TARGET	0x00000010
#define	DIFF_CONTENTS		0x00000020
#define	DIFF_MAJORMINOR		0x00000040
#define	DIFF_PERM 		0x00000080
#define	DIFF_UID		0x00000100
#define	DIFF_GID		0x00000200
#define	DIFF_MASK		0x000003ff

#define DIFF_EDITABLE		0x00000400

struct pkg_info {
	struct pkg_info *next;
	char *name;
	char *arch;
};

struct filediff {
	struct	filediff *diff_next;
	struct	pkg_info *pkg_info_ptr;
	Modinfo *owning_pkg;
	Modinfo *replacing_pkg;
	int	diff_flags;
	char 	*linkptr;
	char	*link_found;
	dev_t	majmin;
	mode_t	act_mode;
	uid_t	act_uid;
	gid_t	act_gid;
	char	exp_type;	/* expected type */
	char	actual_type;
	char	pkgclass[14];
	char	component_path[2];   /* must be at end of struct */
};

typedef struct fsinfo {
	char	*fsi_device;	/* device name 				*/
	ulong	f_frsize;	/* fundamental filesystem block size 	*/
	ulong	f_blocks;	/* total # of blocks (f_frsize) on fs 	*/
	ulong	f_bfree;	/* total # of free blocks 		*/
	ulong	f_bavail;	/* blocks avail to non superuser	*/
	ulong	f_files;	/* total # of file nodes (inodes) 	*/
	ulong	f_ffree;	/* total # of free file nodes 		*/
	int	su_only;	/* Percent of su only space		*/
	dev_t	f_st_dev;	/* ID of device containing 		*/
				/* a directory entry for this file	*/
} Fsinfo;

typedef struct fsspace {
	char	*fsp_mntpnt;		/* mount point	*/
	int	fsp_flags;		/* flags 	*/

	int	fsp_reqd_free_percent;	/*
					 * application-provided required
					 * free-space percentage.
					 */
	int	fsp_reqd_free_kb;	/*
					 *  amount of extra free space 
					 *  required.
					 */
	ulong	fsp_proposed_slice_size;/* proposed slice size in KB */
	/*
	 *  At all times, fsp_reqd_slice_size is the sum of
	 *	fsp_reqd_contents_space,
	 *	fsp_su_only,
	 *	fsp_reqd_free,
	 *	fsp_ufs_ovhd,
	 *	fsp_err_extra
	 *
	 */
	ulong	fsp_reqd_slice_size;
	ulong	fsp_reqd_contents_space;	/* in 1KB blocks */
	ulong	fsp_su_only;
	ContentsBrkdn	fsp_cts;
	ulong	fsp_reqd_free;
	ulong	fsp_ufs_ovhd;
	ulong	fsp_err_extra;
	StringList	*fsp_pkg_databases;  /* contents files for this fs */
	ulong	fsp_cur_slice_size;	/* current slize size in KB */
	Fsinfo	*fsp_fsi;
	void	*fsp_internal;	/* internal data; not used by application */
} FSspace;

typedef enum fsp_field {
	FSP_CONTENTS_PKGD,
	FSP_CONTENTS_NONPKG,
	FSP_CONTENTS_DEVFS,
	FSP_CONTENTS_SAVEDFILES,
	FSP_CONTENTS_PKG_OVHD,
	FSP_CONTENTS_PATCH_OVHD,
	FSP_CONTENTS_SU_ONLY,
	FSP_CONTENTS_REQD_FREE,
	FSP_CONTENTS_UFS_OVHD,
	FSP_CONTENTS_ERR_EXTRA
} FSPfield;

/*  Flags for fsp_flags in FSspace structure */

/*  flags set by caller */
#define FS_IGNORE_ENTRY				0x0001
#define FS_USE_PROPOSED_SIZE			0x0002
#define FS_USE_REQD_FREE_PERCENT		0x0004
#define FS_CALLER_FLAGS_MASK			0x0007

/*  flags set by library */
#define	FS_INSUFFICIENT_SPACE			0x0100
#define	FS_HAS_PACKAGED_DATA			0x0200
#define FS_LIBRARY_FLAGS_MASK			0x0300

typedef enum action_code_mode {
	REPLACE_IDENTICAL_PACKAGES,
	PRESERVE_IDENTICAL_PACKAGES
} ActionCodeMode;

typedef enum valstage {
	VAL_UNKNOWN,
	VAL_ANALYZE_BEGIN,
	VAL_FIND_MODIFIED,
	VAL_CURPKG_SPACE,
	VAL_CURPATCH_SPACE,
	VAL_SPOOLPKG_SPACE,
	VAL_CONTENTS_SPACE,
	VAL_NEWPKG_SPACE,
	VAL_ANALYZE_END,
	VAL_UPG_BEGIN,
	VAL_EXEC_PKGADD,
	VAL_EXEC_PKGRM,
	VAL_EXEC_REMOVEF,
	VAL_EXEC_SPOOL,
	VAL_EXEC_RMTEMPLATE,
	VAL_EXEC_RMDIR,
	VAL_EXEC_RMSVC,
	VAL_EXEC_RMPATCH,
	VAL_EXEC_RMTEMPLATEDIR,
	VAL_UPG_END
} ValStage;

typedef struct valprog {
	int		valp_percent_done;
	ValStage	valp_stage;
	char		*valp_detail;
} ValProgress;

typedef struct pkg_flags {
	int		silent;
	int		checksum;
	int		notinteractive;
	int		accelerated;
	char		*spool;
	char		*admin_file;
	char		*basedir;
} PkgFlags;

/*
 * The structure which contains admin file values
 */
typedef struct admin_files {
	char	*mail;
	char	*instance;
	char	*partial;
	char	*runlevel;
	char	*idepend;
	char	*rdepend;
	char	*space;
	char	*setuid;
	char	*action;
	char	*conflict;
	char	*basedir;
} Admin_file;

struct patchpkg {
	struct patchpkg	*next;
	Modinfo *pkgmod;
};

struct patch {
	struct patch *next;
	char *patchid;
	int  removed;
	struct patchpkg *patchpkgs;
};

typedef struct locmap
{
	struct locmap	*next;
	char		*locmap_partial;
	StringList	*locmap_base;
	char		*locmap_description;
} LocMap;


/* media flags */
#define	NEW_SERVICE		0x1	/* service is new	*/
#define	SVC_TO_BE_REMOVED	0x2	/* service is going to be removed */
#define	SVC_TO_BE_MODIFIED	0x4	/* service is going to be modified */
#define	SVC_UNCHANGED		0x0
#define SVC_MOD_MASK		0x7

#define	SPLIT_FROM_SERVER	0x8	/* service shares /var/sadm with
					 * local environment.
					 */

#define	BUILT_FROM_UPGRADE	0x10	/* env is built from an upgrade */
#define	BASIS_OF_UPGRADE	0x20	/* env is basis of an upgrade */

#define	MODIFIED_FILES_FOUND	0x40	/* the find_modified has been done
					 * for this environment.
					 */

#define svc_unchanged(mi)	\
	(((mi)->med_flags & SVC_MOD_MASK) == SVC_UNCHANGED)

#define	set_svc_modstate(mi, state)        \
	((mi)->med_flags = ((mi)->med_flags & ~SVC_MOD_MASK) | (state))

/* upgrade mode flags */
#define	SELECT_CLIENTS		0x0001
#define	UPDATE_ENVIRONMENTS	0x0002
#define	ADD_SERVICES		0x0004
#define	MODE_MASK		0x000f
#define	LOCAL_UPGRADE		0x0010


#if	!defined(TRUE) || ((TRUE) != 1)
#define	TRUE    (1)
#endif
#if	!defined(FALSE) || ((FALSE) != 0)
#define	FALSE   (0)
#endif

/*
 * for package modules: sunw_ptype is one of these:
 */
#define		PTYPE_ROOT	'R'
#define		PTYPE_USR	'U'
#define		PTYPE_KVM	'K'
#define		PTYPE_APP	'A'
#define		PTYPE_OW	'O'
#define		PTYPE_UNKNOWN	'N'

#define		ARCH_SEPARATOR	'.'	/* separator: sparc.sun4c	*/
#define		ARCH_DELIMITER	','	/* arch token delimiter		*/

#define	ENDUSER_METACLUSTER	"SUNWCuser"
#define	ALL_METACLUSTER		"SUNWCall"
#define	REQD_METACLUSTER	"SUNWCreq"

#define		MBYTE		1048576.0	/* 1M bytes 2^21    */
#define		KBYTE		1024.0		/* 1K bytes (2*512) */

#define	PACKAGE_TOC_NAME 	".packagetoc"
#define	CLUSTER_TOC_NAME 	".clustertoc"
#define	ORDER_FILE_NAME  	".order"

#define	NODENAME_LENGTH	50

#define	MAXCATNAMELEN	256

#define	V_NOT_UPGRADEABLE	-2
#define	V_LESS_THEN		-1
#define	V_EQUAL_TO		0
#define	V_GREATER_THEN		1

/* Space code defines */

#define	SP_CNT_DEVS		0x004

#define	SP_UPG			0x001

#define	SP_FAILURE			-1
#define	SP_ERR_STAT			-2
#define	SP_ERR_PATH_INVAL		-3
#define	SP_ERR_CHROOT			-4
#define	SP_ERR_POPEN			-5
#define	SP_ERR_OPEN			-6
#define	SP_ERR_PARAM_INVAL		-7
#define	SP_ERR_STAB_CREATE		-8
#define	SP_ERR_CORRUPT_CONTENTS		-9
#define	SP_ERR_CORRUPT_PKGMAP		-10
#define	SP_ERR_CORRUPT_SPACEFILE	-11
#define	SP_ERR_MALLOC			-12
#define	SP_ERR_GETMNTENT		-13
#define	SP_ERR_STATVFS			-14
#define	SP_ERR_NOSLICES			-15

#define	SP_ERR_NOT_ENOUGH_SPACE		-101

#define	SP_WARN_UNEXPECTED_LINE_FORMAT		0x001
#define	SP_WARN_PARTIAL_INSTALL_FOUND		0x002
#define	SP_WARN_FINDNODE_FAILED			0x004
#define	SP_WARN_FIND_OWNING_INST_FAILED		0x008

/******************************************************************/
/*
 *  Data structures used for listing and modifying services.
 *
 */
/******************************************************************/

typedef enum sw_svc_action
{
	SW_ADD_SERVICE = 1,
	SW_REMOVE_SERVICE = 2,
	SW_UPGRADE_SERVICE = 3
} SW_svc_action;

/*
 *  The following data structure identifies a locale.
 */
typedef struct sw_locale
{
	struct sw_locale *next;
	char	*sw_loc_name;		/* locale name			*/
	char	*sw_loc_nametext;	/* English name oflocale	*/
	char	*sw_loc_os;		/* product name including locale */
	char	*sw_loc_ver;		/* prod version including locale */
	char	*sw_loc_isa;		/* isa for which locale is valid */
} SW_locale;

/*
 *  The following data structure identifies a metacluster.
 */
typedef struct sw_metacluster
{
	struct sw_metacluster *next;
	char	*sw_meta_name;		/* metacluster name		*/
	char	*sw_meta_desc;		/* cluster description, in English */
	char	*sw_meta_os;		/* product name including mcluster */
	char	*sw_meta_ver;		/* prod version including mcluster */
	char	*sw_meta_isa;		/* isa for which mcluster is valid */
} SW_metacluster;

/*
 *  The following two data structures identify a service.
 */
typedef struct sw_sizeinfo
{
	daddr_t	sw_size_clientroot;	/* client root size		*/
	daddr_t	sw_size_tmpl_all;	/* arch-neutral root templates  */
	daddr_t	sw_size_tmpl_isa;	/* isa-specific root templates  */
	daddr_t sw_size_tmpl_plat;	/* platform-dependent templates */
	daddr_t sw_size_usr_all;	/* arch-neutral /usr packages	*/
	daddr_t sw_size_usr_isa;	/* isa-specific /usr packages	*/
	daddr_t sw_size_usr_plat;	/* plat-dependent /usr packages	*/
} SW_svcsize;

typedef struct sw_service
{
	struct sw_service *next;
	char	 	*sw_svc_os;	/* OS, typically "Solaris" 	*/
	char 		*sw_svc_version; /* OS version, such as "2.5"	*/
	char 		*sw_svc_isa;	/* instruction set arch, i.e."sparc */
	char 		*sw_svc_plat;	/* platform name or group	*/
	SW_svcsize	*sw_svc_size;	/* size info structure 		*/
} SW_service;

/*
 *  The following data structure is returned by the list_available_services
 *  and list_installed_services functions.  The struct identifies the
 *  the number of services and contains a pointer to a linked list of
 *  of SW_service structs, each of which identifies a service.
 */
typedef struct sw_service_list
{
	int		 sw_svl_num_services;	/* number of services	*/
	SW_service	*sw_svl_services;	/* linked list of services */
	SW_locale	*sw_svl_locales;	/* linked list of locales */
	SW_metacluster	*sw_svl_metaclusters;	/* linked list of mclusters */
} SW_service_list;

/*
 *  These data structures are the input to the validate_service_modification
 *  and execute_service_modification functions.  They indicates the actions
 *  to be performed on a service.  A linked list of SW_service_mod structures
 *  defines a collection of actions to be performed on the system.  If the
 *  action is SW_ADD_SERVICE, the sv_svmod_media and sv_svmod_newservice
 *  fields will be need to be supplied.  If the action is SW_REMOVE_SERVICE,
 *  the sv_svmod_oldservice will indicate the service to be removed.
 *  If the action is SW_UPGRADE_SERVICE, the sv_svmod_oldservice will
 *  identify the service to be upgraded, the sv_svmod_newservice will
 *  identify the service being upgrade TO, and the sv_svmod_media will
 *  provide the new service.
 */

typedef struct sw_service_mod
{
	struct sw_service_mod	 *next;
	SW_svc_action	 sw_svmod_action;	/* action to be performed */
	char		*sw_svmod_media;	/* installation media	  */
	SW_service	*sw_svmod_newservice;	/* service to be added    */
	SW_service	*sw_svmod_oldservice; 	/* service to be removed  */
	SW_locale	*sw_svmod_locales;	/* locales to be added	  */
	SW_metacluster	*sw_svmod_metaclusters;	/* metaclusters to be added */
} SW_service_mod;

#define	SW_SVMOD_UNCONDITIONAL	0x01;

typedef struct sw_service_modspec
{
	int		sw_svmodspec_flags;	/* flags		  */
	SW_service_mod *sw_svmodspec_mods;	/* linked list of mods    */
} SW_service_modspec;

/*
 *  The following data structure reads a service definition from a
 *  softinfo file and reports what needs to be done in order to
 *  set up a diskless client root that will run that service.
 *
 *  The things that need to be done to set up a diskless client root
 *  are:
 *	1.  Pkgadd a series of root packages into the client's
 *	    file system.
 *	2.  Set up several remote mounts in the client's /etc/vfstab
 *	    file.  (/usr at a minimum, possibly /usr/kvm and /usr/share).
 */
typedef struct sw_pkgadd_def
{
	struct sw_pkgadd_def	*next;
	char	*sw_pkg_dir;		/* directory containing packages */
	char	*sw_pkg_name;		/* package name			*/
	daddr_t  sw_pkg_size;		/* package size in KB		*/
} SW_pkgadd_def;

typedef struct sw_remmnt
{
	struct sw_remmnt	*next;
	char	*sw_remmnt_mntpnt;	/* mount point, such as /usr	*/
	char	*sw_remmnt_mntdir;	/* remote directory to be mounted */
} SW_remmnt;

typedef struct sw_createroot_info
{
	daddr_t		 sw_root_size;		/* sum of all package sizes */
	SW_pkgadd_def	*sw_root_packages;	/* packages to be pkgadd'ed */
	SW_remmnt	*sw_root_remmnt;	/* remote mounts	*/
} SW_createroot_info;

/******************************************************************/
/*
 *  Data structures used for error reporting.
 *
 */
/******************************************************************/

typedef enum sw_return_code
{
	SW_OK = 0,
	SW_INSUFFICIENT_SPACE,   /* indicates a predicted space failure */
	SW_OUT_OF_SPACE,   /* actually ran out of space during operation*/
	SW_DEPENDENCY_FAILURE,
	SW_MEDIA_FAILURE,
	SW_EXEC_FAILURE,
	SW_INVALID_SVC,
	SW_INVALID_OP,
	SW_INVALID_SOFTINFO,
	SW_INCONSISTENT_REV
} SW_return_code;

/*
 *  The following data structure is returned in the SW_error_info.info
 *  union if the error from validate_service_modifications or
 *  execute_service_modifications is SW_INSUFFICIENT_SPACE.  It is
 *  a linked list of structures, each of which show the space available
 *  and space needed in a file system.
 */
typedef struct sw_space_results
{
	struct sw_space_results	*next;
	char		*sw_mountpnt;	/* file system mount point	*/
	char		*sw_devname;	/* device containing file system */
	daddr_t		sw_cursiz;	/* current size in KB		*/
	daddr_t		sw_newsiz;	/* required size in KB		*/
	int		sw_toofew_inodes; /* insufficient inode flag	*/
} SW_space_results;

/*
 *  The following data structure is returned in the SW_error_info.info
 *  union if the error from validate_service_modifications or
 *  execute_service_modifications is SW_INSUFFICIENT_SPACE.  It is
 *  a linked list of structures, each of which show the space available
 *  and space needed in a file system.
 */
typedef struct sw_out_of_space
{
	char		*sw_out_mntpnt;	/* file system out of space	*/
	char		*sw_out_dev;	/* device containing file system */
	SW_service_list *sw_out_svclist; /* updated service list 	*/
} SW_out_of_space;

/*
 * The following data structure reports the package that is part of
 * the service being added, but is already installed and has a different
 * revision than the one in the service being added.
 */
typedef struct sw_diffrev
{
	char	*sw_diffrev_pkg;	/* package ID */
	char	*sw_diffrev_arch;	/* package architecture */
	char	*sw_diffrev_curver;	/* currently-installed version */
	char	*sw_diffrev_newver;	/* version of pkg in service */
} SW_diffrev;

/*
 *  The following data structure is returned from validate_service_modifications
 *  or execute_service_modifications in case of an error.  The error code
 *  is supplied in sw_error_code.  The contents of the SW_error_info.info
 *  union will depend on the value of the error code.
 */
typedef struct sw_error_info
{
	SW_return_code	sw_error_code;	/* error code			*/
	union {
		SW_space_results *swspace_results;  /* if INSUFFICIENT_SPACE */
		SW_out_of_space  *swout_of_space;   /* if OUT_OF_SPACE   */
		int		 swexec_errcode;    /* if EXEC_FAILURE	*/
		SW_diffrev	 *swdiff_rev;	    /* if INCONSISTENT REV */
		SW_service	 *swinvalid_svc;    /* if INVALID_SVC	*/
		SW_service	 *swinvalid_soft;   /* if INVALID_SOFTINFO */
	} sw_specific_errinfo_u;
} SW_error_info;

#define sw_space_results	sw_specific_errinfo_u.swspace_results
#define sw_out_of_space		sw_specific_errinfo_u.swout_of_space
#define sw_exec_errcode		sw_specific_errinfo_u.swexec_errcode
#define sw_diff_rev		sw_specific_errinfo_u.swdiff_rev
#define sw_invalid_svc		sw_specific_errinfo_u.swinvalid_svc
#define sw_invalid_soft		sw_specific_errinfo_u.swinvalid_soft

/*---------------------------------------------------------*/
/*                                                         */
/*---------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

void 	(*fatal_err_func)();

/* admin.c */

char	*getset_admin_file(char *);
int	admin_write(char *, Admin_file *);

/* arch.c */

char	*get_default_arch(void);
char	*get_default_impl(void);
Arch	*get_all_arches(Module *);
int	package_selected(Node *, char *);
int	select_arch(Module *, char *);
int	deselect_arch(Module *, char *);
void	mark_arch(Module *);
int	valid_arch(Module *, char *);

/* depend.c */

int	check_sw_depends(void);
Depend * get_depend_pkgs(void);

/* soft_do_upgrade.c */
void	set_pkg_hist_file(char *);
void	set_onlineupgrade_mode(void);
int	upgrade_all_envs(void);
int	nonnative_upgrade(StringList *);

/* soft_find_modified.c */
int	file_was_modified(Module *, char *);
int	file_will_be_upgraded(char *, char *, int);
void	end_upgraded_file_scan(void);

/* install.c */

Module *	load_installed(char *, int);
Modinfo *next_patch(Modinfo *);
Modinfo *next_inst(Modinfo *);

/* locale.c */

Module *	get_all_locales(void);
int	select_locale(Module *, char *);
int	deselect_locale(Module *, char *);
int	valid_locale(Module *, char *);

/* soft_media.c */

Module *	add_media(char *);
Module *	add_specific_media(char *, char *);
int		load_media(Module *, int);
int		unload_media(Module *);
void		set_eject_on_exit(int);
Module *	get_media_head(void);
Module *	find_media(char *, char *);

/* soft_update_action.c */
Module *	get_localmedia(void);

/* module.c */

int 	set_current(Module *);
int	set_default(Module *);
Module *	get_current_media(void);
Module *	get_current_service(void);
Module *	get_current_product(void);
Module *	get_current_category(ModType);
Module *	get_current_metacluster(void);
Module *	get_local_metacluster(void);
Module *	get_current_cluster(void);
Module *	get_current_package(void);
Module *	get_default_media(void);
Module *	get_default_service(void);
Module *	get_default_product(void);
Module *	get_default_category(ModType);
Module *	get_default_metacluster(void);
Module *	get_default_cluster(void);
Module *	get_default_package(void);
Module *	get_next(Module *);
Module *	get_sub(Module *);
Module *	get_prev(Module *);
Module *	get_head(Module *);
int		mark_required(Module *);
int		mark_module(Module *, ModStatus);
int		mod_status(Module *);
int		toggle_module(Module *);
char *		get_current_locale(void);
void		set_current_locale(char *);
char *		get_default_locale(void);
int		toggle_product(Module *, ModStatus);
int		partial_status(Module *);

/* prod.c */

char *	get_clustertoc_path(Module *);
void	media_category(Module *);

/* util.c */

void	sw_lib_init(int);
int	set_instdir_svc_svr(Module *);
void	clear_instdir_svc_svr(Module *);
char *	gen_bootblk_path(char *);
char *	gen_pboot_path(char *);
char *	gen_openfirmware_path(char *);
int	is_upgrade(void);

/* dump.c */

int	dumptree(char *);

/* update_actions.c */

int	load_clients(void);
void	update_action(Module *);
void	upg_select_locale(Module *, char *);
void	upg_deselect_locale(Module *, char *);

/* platform.c */
int	write_platform_file(char *, Module *);

/* v_version.c */

int	prod_vcmp(char *, char *);
int	pkg_vcmp(char *, char *);
int	is_patch(Modinfo *);
int	is_patch_of(Modinfo *, Modinfo *);

/* sp_load.c */

void	set_add_service_mode(int);

/* sp_util.c */
int	valid_mountp(char *);
void	fsp_add_to_field(FSspace *, FSPfield, long);
void	fsp_set_field(FSspace *, FSPfield, long);

/* sp_space.c */

FSspace **calc_cluster_space(Module *, ModStatus);
ulong	calc_tot_space(Product *);
void	swi_free_space_tab(FSspace **);
void	free_fsspace(FSspace *);
long	tot_pkg_space(Modinfo *);
int	calc_sw_fs_usage(FSspace **, int (*)(void *, void *), void *);
FSspace **get_current_fs_layout();
FSspace **load_current_fs_layout();
FSspace **gen_dflt_fs_spaceinfo(void);

/* soft_update_actions.c */
int	set_action_code_mode(ActionCodeMode);

/* soft_view.c */
int	load_view(Module *, Module *);
int	load_local_view(Module *);

#ifdef __cplusplus
}
#endif

#endif	/* _SPMISOFT_API_H */
