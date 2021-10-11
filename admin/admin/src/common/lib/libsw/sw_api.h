/*
 * Copyright (c) 1995-1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */
#ifndef lint
#pragma ident "@(#)sw_api.h 1.47 96/02/08"
#endif

#ifndef _SW_API_H
#define	_SW_API_H

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <libintl.h>

#include "swmgmt_api.h"
#include "hash.h"

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
	PARTIAL = 2,
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

typedef struct depend
{
	char		*d_pkgid;	/* what package?	  */
	char		*d_pkgidb;	/* dependant package	  */
	char 		*d_version;	/* which package version? */
	char 		*d_arch;	/* which architecture?    */
	struct depend 	*d_next;
	struct depend 	*d_prev;
}Depend;

#ifdef	MMAP
#include <sys/mman.h>
typedef struct memory_file {
	int	m_fd;		/* file descriptor */
	size_t	m_size;		/* size of file in bytes */
	caddr_t	m_base;		/* base [mapped] address */
	caddr_t	m_ptr;		/* currently addressed offset in file */
} MFILE;
#else
typedef FILE    MFILE;
#endif

/*
 * the modinfo structure contains per cluster or per package information
 */

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
	daddr_t		  m_deflt_fs[N_LOCAL_FS]; /* fs size for default fs */
	struct filediff	 *m_filediff;	/* list of modified files in this pkg */
	struct patch_num *m_newarch_patches;	/* new architecture patches */
} Modinfo;

/* Modinfo flags */

#define	PART_OF_CLUSTER			0x0001
#define	INSTANCE_ALREADY_PRESENT	0x0002
#define	DO_PKGRM			0x0004
#define	CONTENTS_GOING_AWAY		0x0010	/*
						 * Set if pkghistory files
						 * has an remove from
						 * cluster entry, or if the
						 * used explicitly
						 * de-selected this package.
						 */
#define	IN_ORDER_FCN			0x0008
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

typedef enum machine_type {
	MT_UNDEFINED = -1,
	MT_STANDALONE = 0,
	MT_SERVER = 1,
	MT_DATALESS = 2,
	MT_DISKLESS = 3,
	MT_SERVICE = 4,
	MT_CCLIENT = 5
} MachineType;

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
	struct locmap	*med_locmap;
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

#define	DIFF_INMAP		0x00000400

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
typedef enum test_mount {
	NOT_TESTED = 0,
	TEST_FAILURE = 1,
	TEST_SUCCESS = 2
}TestMount;

typedef struct remote_fs {
	TestMount	 c_test_mounted;
	char		*c_mnt_pt;
	char		*c_hostname;
	char		*c_ip_addr;
	char		*c_export_path;
	char		*c_mount_opts;
	struct remote_fs *c_next;
}Remote_FS;

typedef struct fsinfo {
	char	*device;	/* device name 				*/
	ulong	f_frsize;	/* fundamental filesystem block size 	*/
	ulong	f_blocks;	/* total # of blocks (f_frsize) on fs 	*/
	ulong	f_bfree;	/* total # of free blocks 		*/
	ulong	f_bavail;	/* blocks avail to non superuser	*/
	ulong	f_files;	/* total # of file nodes (inodes) 	*/
	ulong	f_ffree;	/* total # of free file nodes 		*/
	int	su_only;	/* Percent of su only space		*/
	dev_t	st_dev;		/* ID of device containing 		*/
				/* a directory entry for this file	*/
} Fsinfo;

typedef struct space {
	char	*mountp;	/* mount point 				*/
	daddr_t	bused;		/* blocks (f_frsize) used 		*/
	daddr_t	fused;		/* inodes used				*/
	ulong	bavail;		/* Free space available in partition	*/
	int	touched;	/* Upg has space in this fs		*/
	Fsinfo	*fsi;
} Space;

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


/* upgrade mode flags */
#define	SELECT_CLIENTS		0x0001
#define	UPDATE_ENVIRONMENTS	0x0002
#define	ADD_SERVICES		0x0004
#define	MODE_MASK		0x000f
#define	LOCAL_UPGRADE		0x0010

/*
 * calc_cluster_space() constraint flags used to limit which
 * pacakges are included in the calculation
 */
#define	CSPACE_NONE	0x00	/* no constraints - all instances and locales */
#define	CSPACE_ARCH	0x01	/* exclude inapplicable archs */
#define	CSPACE_LOCALE	0x02	/* exclude inapplicable locales */
#define	CSPACE_ALL	(CSPACE_ARCH | CSPACE_LOCALE)

#if	!defined(TRUE) || ((TRUE) != 1)
#define	TRUE    (1)
#endif
#if	!defined(FALSE) || ((FALSE) != 0)
#define	FALSE   (0)
#endif

#define	SUCCESS		0
#define	FAILURE		1
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
/*
 * NOTE: the following two values are pulled out of the air.  We should
 * find the real maximum values, but I'm pretty sure that 256 is adequate.
 */
#define	ARCH_LENGTH	256
#define	PLATFORM_LENGTH	256

#define	MAXCATNAMELEN	256
#define	IP_ADDR		35
#define	NO_EXTRA_SPACE	-1

/* Error return codes */

#define	ERR_NOMEDIA	1
#define	ERR_NODIR	2
#define	ERR_INVALIDTYPE	3
#define	ERR_UMOUNTED	4
#define	ERR_NOPROD	5
#define	ERR_MOUNTED	6
#define	ERR_INVALID	7
#define	ERR_NOPRODUCT	8
#define	ERR_NOLOAD	9
#define	ERR_NOCLSTR	10
#define	ERR_LOADFAIL	11
#define	ERR_UNDEF	12
#define	ERR_NOMATCH	13
#define	ERR_NOFILE	14
#define	ERR_BADENTRY	15
#define	ERR_NOPKG	16
#define	ERR_BADPKG	17
#define	ERR_UNMOUNT	18
#define	ERR_NODEVICE	19
#define	ERR_PREVLOAD	20
#define	ERR_BADARCH	21
#define	ERR_INVSERVER	22
#define	ERR_NOMOUNT	23
#define	ERR_FSTYPE	24
#define	ERR_SHARE	25
#define	ERR_LOCKFILE	26
#define	ERR_VOLUME	27
#define	ERR_MOUNTPT	28
#define	ERR_SAVE	29
#define	ERR_PIPECREATE	30
#define	ERR_ULIMIT	31
#define	ERR_FORKFAIL	32
#define	ERR_MNTTAB	33
#define	ERR_HOSTDOWN	34
#define	ERR_NOPORT	35
#define	ERR_NOSTREAM	36
#define	ERR_NOPASSWD	37
#define	ERR_INVPASSWD	38
#define	ERR_BADCOMM	39
#define	ERR_HOSTINFO	40
#define	ERR_NOACCESS	41
#define	ERR_DIFFREV	42
#define	ERR_INVARCH	43
#define	ERR_BADLOCALE	44
#define	ERR_NULLPKG	45
#define	ERR_OPENING_VFSTAB 	46
#define	ERR_ADD_SWAP		47
#define	ERR_MOUNT_FAIL		48
#define	ERR_MUST_MANUAL_FSCK	49
#define	ERR_FSCK_FAILURE	50
#define	ERR_OPEN_VFSTAB		51
#define	ERR_DELETE_SWAP		52
#define	ERR_UMOUNT_FAIL		53
#define	ERR_SVC_ALREADY_EXISTS	54
#define	ERR_NONNATIVE_MEDIA	55
#define	ERR_NOTHING_TO_UPGRADE	56

/* FATAL ERROR CODES */
#define	ERR_MALLOC_FAIL -50
#define	ERR_IBE		-51
#define	ERR_STR_TOO_LONG	-101

#define	V_NOT_UPGRADEABLE	-2
#define	V_LESS_THEN		-1
#define	V_EQUAL_TO		0
#define	V_GREATER_THEN		1

/* Space code defines */

#define	SP_DIRECTORY		0x001
#define	SP_MOUNTP		0x002
#define	SP_CNT_DEVS		0x004

#define	SP_UPG			0x001
#define	SP_UPG_INSTALLED_CHK	0x002
#define	SP_UPG_SPACE_CHK	0x004
#define	SP_UPG_EXTRA		0x008

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

/*---------------------------------------------------------*/
/*                                                         */
/*---------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

extern void 	(*fatal_err_func)();

/* admin.c */

extern char	*admin_file(char *);
extern int	admin_write(char *, Admin_file *);

/* arch.c */

extern Arch *	get_all_arches(Module *);
extern int	package_selected(Node *, char *);
extern char *	get_default_arch(void);
extern char *	get_default_impl(void);
extern char *	get_default_inst(void);
extern char *	get_default_machine(void);
extern char *	get_default_platform(void);
extern char *	get_actual_platform(void);
extern int	select_arch(Module *, char *);
extern int	deselect_arch(Module *, char *);
extern void	mark_arch(Module *);
extern int	valid_arch(Module *, char *);

/* client.c */

extern char *	name2ipaddr(char *);
extern int	test_mount(Remote_FS *, int);
extern TestMount	get_rfs_test_status(Remote_FS *);
extern int	set_rfs_test_status(Remote_FS *, TestMount);

/* depend.c */

extern int	check_sw_depends(void);
extern Depend * get_depend_pkgs(void);

/* install.c */

extern Module *	load_installed(char *, int);
extern Modinfo *next_patch(Modinfo *);
extern Modinfo *next_inst(Modinfo *);

/* locale.c */

extern Module *	get_all_locales(void);
extern void	update_l10n_package_status(Module *);
extern int	select_locale(Module *, char *);
extern int	deselect_locale(Module *, char *);
extern void	mark_locales(Module *, ModStatus);
extern int	valid_locale(Module *, char *);

/* media.c */

extern Module *	add_media(char *);
extern Module *	add_specific_media(char *, char *);
extern int	load_media(Module *, int);
extern int	mount_media(Module *, char *, MediaType);
extern int	unload_media(Module *);
extern void	set_eject_on_exit(int);
extern Module *	get_media_head(void);
extern Module *	find_media(char *, char *);

/* module.c */

extern int 	set_current(Module *);
extern int	set_default(Module *);
extern Module *	get_current_media(void);
extern Module *	get_current_service(void);
extern Module *	get_current_product(void);
extern Module *	get_current_category(ModType);
extern Module *	get_current_metacluster(void);
extern Module *	get_local_metacluster(void);
extern Module *	get_current_cluster(void);
extern Module *	get_current_package(void);
extern Module *	get_default_media(void);
extern Module *	get_default_service(void);
extern Module *	get_default_product(void);
extern Module *	get_default_category(ModType);
extern Module *	get_default_metacluster(void);
extern Module *	get_default_cluster(void);
extern Module *	get_default_package(void);
extern Module *	get_next(Module *);
extern Module *	get_sub(Module *);
extern Module *	get_prev(Module *);
extern Module *	get_head(Module *);
extern int	mark_required(Module *);
extern int	mark_module(Module *, ModStatus);
extern int	mod_status(Module *);
extern int	toggle_module(Module *);
extern MachineType get_machinetype(void);
extern void	set_machinetype(MachineType);
extern char *	get_current_locale(void);
extern void	set_rootdir(char *);
extern void	set_current_locale(char *);
extern char *	get_default_locale(void);
extern char *	get_rootdir(void);
extern int	toggle_product(Module *, ModStatus);
extern int	mark_module_action(Module *, Action);
extern int	partial_status(Module *);

/* mount.c */

extern int	mount_fs(char *, char *, char *);
extern int	umount_fs(char *);
extern int	share_fs(char *);
extern int	unshare_fs(char *);

/* pkgexec.c */

extern int	add_pkg(char *, PkgFlags *, char *);
extern int	remove_pkg(char *, PkgFlags *);


/* prod.c */

extern char *	get_clustertoc_path(Module *);
extern int	path_is_readable(char *);
extern void	media_category(Module *);

/* util.c */

extern void	sw_lib_init(void(*)(int), int, int);
extern char *	get_err_str(int);
extern void	error_and_exit(int);
extern void *	xcalloc(size_t size);
extern void *	xmalloc(size_t);
extern void *	xrealloc(void *, size_t);
extern char *	xstrdup(char *);
extern void	deselect_usr_pkgs(Module *);
extern int	set_instdir_svc_svr(Module *);
extern void	clear_instdir_svc_svr(Module *);
extern void	set_action_for_machine_type(Module *);
extern Space **	sort_space_fs(Space **, char **);
extern int	percent_free_space(void);
extern int	set_sw_debug(int);
extern char *	gen_bootblk_path(char *);
extern char *	gen_pboot_path(char *);
extern char *	gen_openfirmware_path(char *);

/* dump.c */

extern int	dumptree(char *);

/* update_actions.c */

extern int	load_clients(void);
extern void	update_action(Module *);
extern void	upg_select_locale(Module *, char *);
extern void	upg_deselect_locale(Module *, char *);

/* do_upgrade.c */

extern void	set_debug(char *);
extern void	set_skip_mod_search(void);
extern void	set_pkg_hist_file(char *);
extern void	set_onlineupgrade_mode(void);
extern int	upgrade_all_envs(void);
extern int	local_upgrade(void);
extern int	do_upgrade(void);
extern int	do_product_upgrade(Module *);
extern int	do_find_modified(void);
extern int	do_final_space_check(void);
extern void	do_write_upgrade_script(void);
extern int	nonnative_upgrade(StringList *);

/* platform.c */
extern int	write_platform_file(char *, Module *);

/* mountall.c */

extern int	mount_and_add_swap(char *);
extern int	umount_and_delete_swap(void);
extern int	umount_all(void);
extern int	unswap_all(void);

/* upg_recover.c */

extern int	partial_upgrade(void);
extern int	resume_upgrade(void);

/* v_version.c */

extern int	prod_vcmp(char *, char *);
extern int	pkg_vcmp(char *, char *);
extern int	is_patch(Modinfo *);
extern int	is_patch_of(Modinfo *, Modinfo *);

/* sp_calc.c */

extern u_int	min_req_space(u_int);

/* sp_util.c */
extern int	valid_mountp(char *);

/* sp_space.c */

extern void	free_space_tab(Space **);
extern Space	**space_meter(char **);
extern Space	**swm_space_meter(char **);
extern Space	**upg_space_meter(void);
extern Space 	**calc_cluster_space(Module *, ModStatus, int);
extern Space 	**calc_tot_space(Product *);
extern long	tot_pkg_space(Modinfo *);

/* sp_print_results.c */

extern void	print_final_results(char *);
extern SW_space_results	*gen_final_space_report();

/* sp_free_results.c */

extern void	free_final_space_report(SW_space_results *);

#ifdef __cplusplus
}
#endif

#endif	/* _SW_API_H */
