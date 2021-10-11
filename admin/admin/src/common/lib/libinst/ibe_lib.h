#ifndef lint
#pragma ident "@(#)ibe_lib.h 1.39 95/03/21"
#endif
/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary
 * trade secret, and it is available only under strict license
 * provisions.  This copyright notice is placed here only to protect
 * Sun in the event the source is deemed a published work.  Dissassembly,
 * decompilation, or other means of reducing the object code to human
 * readable form is prohibited by the license agreement under which
 * this code is provided to the user or company in possession of this
 * copy.
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 * NASA FAR Supplement.
 */
#ifndef _IBE_LIB_H
#define	_IBE_LIB_H

#include "ibe_api.h"
#include "ibe_strings.h"
#include <sys/vfstab.h>
#include <sys/mntent.h>

/* Mount list structure */

typedef struct vfs_entry {
	struct vfstab		*entry;
	struct vfs_entry 	*next;
} Vfsent;

typedef Remote_FS	Dfs;

/*
 *  The structure for the transfer file elements
 */
typedef struct trans_file_element {
	char	*file;		/* name of a transition file		*/
	char	*package;	/* name of package containing file	*/
	int	found;		/* a status flag			*/
	mode_t	mode;		/* File mode 				*/
	uid_t	uid;		/* User ID of the file's owner		*/
	gid_t	gid;		/* Group ID of the file's group		*/
} TransList;

/*
 * Constants
 */
#define	TMPLOGFILE		"/tmp/install_log"
#define	TMPVFSTAB		"/tmp/vfstab"
#define	TMPVFSTABUNSELECT	"/tmp/vfstab.unselected"
#define	IDKEY			"/kernel/misc/sysinit"
#define	IDSAVE			"/.atconfig"
#define	TRANS_LIST		"/tmp/.transfer_list"
#define	SYS_ADMIN_DIRECTORY	"/var/sadm/system/admin"
#define	SYS_SERVICES_DIRECTORY	"/var/sadm/system/admin/services"
#define	SYS_DATA_DIRECTORY	"/var/sadm/system/data"
#define	SYS_LOGS_DIRECTORY	"/var/sadm/system/logs"
#define	SYS_LOGS_RELATIVE	"../system/logs"
#define	OLD_DATA_DIRECTORY	"/var/sadm/install_data"


#define	DIRECT_INSTALL	 \
	(*get_rootdir() == '\0' || strcmp(get_rootdir(), "/") == 0 ? 1 : 0)
#define	INDIRECT_INSTALL \
	(*get_rootdir() != '\0' && strcmp(get_rootdir(), "/") != 0 ? 1 : 0)

/* ibe_fileio.c */

extern int 		_create_inst_release(Product *);
extern int		_open_product_file(Product *);

/* ibe_mount.c */

extern void 		_swap_add(Disk_t *);
extern int		_create_mount_list(Disk_t *, Dfs *, Vfsent **);
extern int 		_mount_filesys_all(Vfsent *);
extern int		_mount_filesys_specific(char *, struct vfstab *);
extern int		_mount_synchronous_fs(struct vfstab *, Vfsent *);
extern int		_merge_mount_entry(struct vfstab *, Vfsent **);
extern void		_free_mount_list(Vfsent **);
extern void		_vfstab_free_entry(struct vfstab *);

/* ibe_util.c */

extern int 		_create_dir(char *);
extern int 		_arch_cmp(char *, char *, char *);
extern int 		_lock_prog(char *);
extern int		_copy_file(char *, char *);
extern int		_build_admin(Admin_file *);

/* install_setup.c */

extern int		_setup_vfstab(Vfsent *);
extern int		_setup_vfstab_unselect(void);
extern int		_setup_bootblock(void);
extern int 		_setup_etc_hosts(Dfs *);
extern int		_setup_devices(void);
extern int		_setup_inetdconf(void);
extern int		_setup_hostid(void);
extern int		_setup_tmp_root(TransList **);
extern int		_setup_software(Module *, TransList **);
extern int		_setup_install_log(void);
extern int		_setup_disks(Disk_t *, Vfsent *);

/* ibe_setup.c */

/* ibe_print.c */

extern void		_print_results(Module *);

/* install_prod.c */

extern int 		_install_prod(Module *, PkgFlags *,
					Admin_file *, TransList **);

/* setser.c */

extern long	setser(char *);

#endif /* _IBE_LIB_H */
