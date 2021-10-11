#ifndef lint
#pragma ident "@(#)swmgmt_api.h 1.6 95/02/24 SMI"
#endif
/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _SWMGMT_API_H
#define _SWMGMT_API_H

/* Don't remove the next line. */
/* DOC EXTRACT START */
#include <sys/types.h>

/*
 *  The following data structure is a linked list of strings.  It is
 *  used in numerous places.
 */
typedef struct string_list
{
	struct string_list	*next;
	char			*string_ptr;
} StringList;

typedef struct item {
	struct item *next;
} Item;

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
/* DOC EXTRACT END */
/* Don't delete the previous line */

/************************************************/
/*		FUNCTION PROTOTYPES 		*/
/************************************************/

#ifdef __cplusplus
extern "C" {
#endif

extern SW_service_list  *list_available_services(char *, SW_error_info **);

extern SW_service_list  *list_installed_services(SW_error_info **);

extern StringList	*list_avail_svc_platforms(char *, SW_service *,
				SW_error_info **);

extern StringList	*list_installed_svc_platforms(SW_service *,
				SW_error_info **);

extern int	 	validate_service_modification(SW_service_modspec *,
				SW_error_info **);

extern int  	 	execute_service_modification(SW_service_modspec *,
				SW_error_info **);

extern SW_createroot_info	*get_createroot_info(SW_service *,
					SW_error_info **);

extern void		free_service_list(SW_service_list *);

extern void		free_StringList(StringList *);

extern void		free_error_info(SW_error_info *);

extern void		free_createroot_info(SW_createroot_info *);

extern void		free_diff_rev(SW_diffrev *);

#ifdef __cplusplus
}
#endif

#endif /* _SWMGMT_API_H */
