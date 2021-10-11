/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

#ifndef lint
#ident	"@(#)defs.h 1.12 95/10/20"
#endif

#ifndef SWM_DEFS_H
#define	SWM_DEFS_H

#include <locale.h>
#include <sys/types.h>
#include <stdio.h>
#include <spmisoft_api.h>

/*
 * Program functions:
 *	install software
 *	remove software
 *	define clusters
 */
typedef enum swm_mode {
	MODE_UNSPEC,
	MODE_INSTALL,
	MODE_REMOVE,
	MODE_UPGRADE,
	MODE_DEFINE
} SWM_mode;

typedef	enum select_mode {
	MOD_SELECT,
	MOD_DESELECT,
	MOD_TOGGLE
} Select_mode;

typedef enum swm_view {
	VIEW_UNSPEC,
	VIEW_NATIVE,
	VIEW_SERVICES
} SWM_view;

typedef enum cf_mode {
	CONFIG_SAVE,
	CONFIG_SAVEAS,
	CONFIG_LOAD
} CF_mode;

extern Space	**meter;		/* global space meter */

#define	THEO_MP		0		/* index of theoretical mount point */
#define	REAL_MP		1		/* get index of actual mount point */

/*
 * File system indecies -- software info requires
 * 2D index since package may have both relocatable
 * and non-relocatable components.  Next index is
 * used as pointer to mount point.
 */
#define	ABS		0
#define	REL		1

#define	FS_MNTP		-1		/* f/s is a mount point */

#define	FS_ROOT		0		/* f/s part of root */
#define	FS_USR		1		/* f/s part of /usr */
#define	FS_OPT		2		/* f/s part of /opt */
#define	FS_VAR		3		/* f/s part of /var */
#define	FS_EXPORT	4		/* f/s part of /export */
#define	FS_USROWN	5		/* f/s part of /xxx/openwin */
#define	FS_ETC		6		/* not really used */

#define	FS_MAX		6

#define	SWM_LOCALE_DIR		"/usr/snadm/classes/locale"
#define	SWM_DIR_FLAG		".created_by_swm"
#define	SWM_UPGRADE_SCRIPT	"/var/sadm/system/admin/upgrade_script"
#define	SWM_UPGRADE_SCRIPT_OLD	"/var/sadm/install_data/upgrade_script"

extern char	*progname;		/* our program name */
extern char	thishost[];		/* our host name */

extern int	browse_mode;		/* browse (read-only) mode */
extern int	swm_eject_on_exit;	/* eject mounted CD before exiting */
extern int	verbose;		/* print all [non-fatal] error msgs */

extern char	*device_name;		/* initial load device */

#ifdef lint
#define	gettext(x)	(x)
#endif

#ifdef __STDC__
/*
 * subr.c
 */
extern int	path_is_writable(char *);
extern int	path_is_block_device(char *);
extern int	path_is_local(char *);
extern int	path_create(char *, char *);
extern int	path_remove(char *, char *);
extern void	set_mode(SWM_mode);
extern SWM_mode	get_mode(void);
extern void	set_view(SWM_view);
extern SWM_view	get_view(void);
extern void	set_installed_media(Module *media);
extern Module	*get_installed_media(void);
extern char	*get_full_name(Module *);
extern char	*get_short_name(Module *);
extern int	in_category(Module *, Module *);
extern void	set_source_media(Module *);
extern Module	*get_source_media(void);
extern Module	*get_parent_product(Module *);
extern File	*get_module_icon(Module *);
/*
 * space.c
 */
extern u_long	calc_total_space(Module *);
extern Space	**calc_module_space(Module *);
extern u_long	get_fs_space(Space **, char *);
/*
 * select.c
 */
extern void	mark_selection(Module *, Select_mode);
extern void	reset_selections(int, int, int);
extern int	count_selections(Module *);
/*
 * pkgexec.c
 */
extern pid_t	pkg_exec(Module *);
/*
 * file.c
 */
extern int	runfile(File *);
/*
 * mount.c
 */
extern int	rmount_fs(char *);
extern int	rumount_fs(char *);
#endif

#endif	/* !SWM_DEFS_H */
