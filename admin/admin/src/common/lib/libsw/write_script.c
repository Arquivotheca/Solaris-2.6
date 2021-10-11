#ifndef lint
#pragma ident   "@(#)write_script.c 1.89 95/10/13 SMI"
#endif
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#include "sw_lib.h"

#include <sys/mntent.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>

#include "templates.h"

enum instance_type { UNIQUE, OVERWRITE };

struct admin_file {
	struct admin_file *next;
	char		  admin_name[4];
	enum instance_type inst_type;
	char		  basedir[2];  /* must be at end of struct */
};

struct softinfo_merge_entry {
	struct softinfo_merge_entry	*next;
	Modinfo				*new_mi;
	Modinfo				*cur_mi;
};

/* Local Statics and Constants */

#define	streq(s1, s2) (strcmp((s1), (s2)) == 0)

static struct admin_file *adminfile_head = NULL;
static int	admin_seq = 0;
static FILE	*fp;
static int	start_perm_printed = 0;
static char	ascii_number[30];
static Module	*s_newproductmod;
static Module	*s_localmedia;
static Product	*s_newproduct;
static int	g_seq = 1;
static struct softinfo_merge_entry *softinfo_merge_chain = NULL;
static struct softinfo_merge_entry *smep_cur = NULL;

/* Externals */

extern int	g_debug;
extern int	g_is_swm;
extern int	g_online_upgrade;
extern char 	*g_swmscriptpath;
static char	newver[MAXPATHLEN];
static int 	copyright_printed = 0;

/* Public Function Prototypes */

int		write_script(Module *);
void		scriptwrite(FILE *, char **, ...);
void		set_umount_script_fcn(int (*)(FILE *, int), void (*)(FILE *));

/* Library Function Prototypes */
int 		is_KBI_service(Product *);

/* Local Function Prototypes */

static void	gen_inetboot_files(Module *);
static void	gen_softinfo(Product *);
static int	gen_rm_svc(Module *);
static int	gen_mv_svc(Module *, Module *);
static int	archinlist(char *, Arch *);
static void	gen_add_svc(Module *);
static int	save_and_rm(Node *, caddr_t);
static void	_save_and_rm(Modinfo *, Module *);
static void	remove_patches(Module *);
static int	walk_pkgrm_f(Node *, caddr_t);
static void	_walk_pkgrm_f(Modinfo *, Module *);
static int	walk_root_kvm_softinfo(Node *, caddr_t);
static void	_walk_root_kvm_softinfo(Modinfo *, Product *);
static void	_gen_root_kvm_softinfo(Modinfo *, Product *);
static int	gen_softinfo_merge_chain(Node *, caddr_t);
static void	_gen_softinfo_merge_chain(Modinfo *, Product *);
static char *	getrealbase(Modinfo *);
static int	pkgadd_or_spool(Node *, caddr_t);
static void	_pkgadd_or_spool(Modinfo *, Module *);
static char *	newadmin(char *, enum instance_type);
static void	modify_existing_svc(Module *);
static void	add_new_isa_svc(Module *);
static void	gen_share_commands(Module *);
static int	restore_perm(Node *, caddr_t);
static void	_restore_perm(Modinfo *, Module *);
static char *	cvtperm(mode_t);
static char *	cvtuid(uid_t);
static char *	cvtgid(gid_t);
int (*script_fcn)(FILE *fp, int do_root) = NULL;
void (*install_boot_fcn)(FILE *fp) = NULL;
static Product *get_product(void);

static char *	inst_release_path(Product *prod);
static char *	softinfo_services_path(void);
static char *	cluster_path(Product *);
static char *	clustertoc_path(Product *);
static void	gen_softinfo_locales(FILE *, Product *, Product *);
static void	merge_softinfo_locales(FILE *, Product *);
static char *	upgrade_cleanup_path(void);
static char *	upgrade_failedpkgs_path(void);
static char *	upgrade_restart_path(void);
static void	upgrade_clients_inetboot(Module *, Product *);
static void	remove_entire_svc(Module *);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * scriptwrite()
 *	Write out specified script fragment to output file, replacing
 *	specified tokens by values.
 *
 *	expected arguments:
 *		FILE	*fp;			file pointer
 *		char	**cmds;			array of shell commands
 *		{char	*token, *value}*	up to 10 token-value pairs
 *		char	*(0)			to mark end of list
 * Parameters:
 *	fp	 -
 *	cmdarray -
 * Return:
 *	none
 * Status:
 *	public
 */
void
scriptwrite(FILE *fp, char **cmdarray, ...)
{
	va_list ap;
	char	*token[10], *value[10];
	char	thistoken[20];
	int	count = 0;
	int	i, j;
	char	c, *cp, *dst;

	va_start(ap, cmdarray);
	while ((token[count] = va_arg(ap, char *)) != (char *)0) {
		value[count++] = va_arg(ap, char *);
	}
	va_end(ap);

	for (i = 0; *(cp = cmdarray[i]) != '\0'; i++) {
		while ((c = *cp++) != '\0') {
			switch (c) {

			case '@':
				dst = thistoken;
				while ((*dst++ = *cp++) != '@')
					;
				*--dst = '\0';
				if (strcmp(thistoken, "SEQ") == 0) {
					fprintf(fp, "%d", g_seq);
					break;
				}
				for (j = 0; j < count; j++) {
					if (strcmp(thistoken,
					    token[j]) == 0) {
						dst = value[j];
						while (*dst)
							fputc(*dst++, fp);
						break;
					}
				}
				if (j == count) {
					printf(dgettext("SUNW_INSTALL_SWLIB",
							"Bad Token: %s\n"),
					    thistoken);
				}
				break;

			default:
				fputc(c, fp);
				break;
			}
		}
		fputc('\n', fp);
	}
	g_seq++;
}
/*
 * write_script()
 * Parameters:
 *	prodmod	-
 * Return:
 * Status:
 *	public
 */
int
write_script(Module * prodmod)
{
	char	scriptpath[MAXPATHLEN];
	long	timestamp;
	char	timestr[20];
	char	*root_path;
	Module	*mod, *cmod;
	char	service[MAXPATHLEN];
	char	clientarch[30], *karch;

	s_newproductmod = prodmod;
	s_localmedia = get_localmedia();
	s_newproduct = prodmod->info.prod;

	timestamp = time((long *)0);
	sprintf(timestr, "%ld", timestamp);

	root_path = get_rootdir();

	if (g_is_swm)
		(void) strcpy(scriptpath, g_swmscriptpath);
	else if (s_localmedia->info.media->med_flags & BASIS_OF_UPGRADE) {
		if (*root_path == '\0') {
			(void) strcpy(scriptpath,
			    upgrade_script_path(s_newproduct));
		} else {
			(void) strcpy(scriptpath, root_path);
			(void) strcat(scriptpath,
			    upgrade_script_path(s_newproduct));
		}
	} else {
		if (*root_path == '\0') {
			(void) strcpy(scriptpath,
			    upgrade_script_path(s_localmedia->sub->info.prod));
		} else {
			(void) strcpy(scriptpath, root_path);
			(void) strcat(scriptpath,
			    upgrade_script_path(s_localmedia->sub->info.prod));
		}
	}

	if ((fp = fopen(scriptpath, "w")) == (FILE *)NULL) {
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
				"Cannot create upgrade script.\n"));
		return (-1);
	}
	chmod(scriptpath, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

 	scriptwrite(fp, script_start, "TIMESTAMP", timestr,
	    "RESTART_PATH", upgrade_restart_path(),
	    "CLEANUP_PATH", upgrade_cleanup_path(),
	    "UPGRADE_FAILED_PKGS", upgrade_failedpkgs_path(),
	    (char *)0);

	if (g_is_swm)
		scriptwrite(fp, init_swm_coalesce, (char *)0);
	else {
		scriptwrite(fp, init_coalesce, (char *)0);
		if (script_fcn)
			(*script_fcn)(fp, FALSE);
	}

	/*
	 * Generate commands to upgrade var/sadm directory
	 * structure. This is only needed during an upgrade because it
	 * is assumed that if we are adding services the new directory
	 * structure is already present( >= Solaris 2.5), or not
	 * needed (< Solaris 2.5).
	 */

	if (s_localmedia->info.media->med_flags & BASIS_OF_UPGRADE) {
		/* Make sure new directories are present */
		scriptwrite(fp, mk_varsadm_dirs, "CLIENTROOT", "/",
		    (char *)0);

		/* Move file in old directory to new locations */
		scriptwrite(fp, mv_varsadm_files, "CLIENTROOT", "/",
		    (char *)0);
	}

	if (is_server())
		scriptwrite(fp, init_inetboot_dir, (char *)0);

	/* for each client, set up the new var/sadm directory structure, */

	/* find all clients */
	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type == INSTALLED &&
		    !streq(mod->info.media->med_dir, "/") &&
		    mod->info.media->med_flags & BASIS_OF_UPGRADE) {
			/* Make new var/sadm directory tree and */
			/* move files from the old directories to */
			/* the new locations */
			scriptwrite(fp, mk_varsadm_dirs,
			    "CLIENTROOT", mod->sub->info.prod->p_rootdir,
			    (char *)0);
			scriptwrite(fp, mv_varsadm_files,
			    "CLIENTROOT", mod->sub->info.prod->p_rootdir,
			    (char *)0);
		}
	}

	/*
	 * If a server, modify the service links, remove old services,
	 * update the softinfo files.
	 */

	for (mod = get_media_head(); mod != NULL; mod = mod->next)  {
		if (mod->info.media->med_type != INSTALLED_SVC)
			continue;
		/*
		 *  Determine whether the entire service, or any part
		 *  the service needs to be removed.  If the entire
		 *  service needs to be removed, continue, since there
		 *  no reason to look at it further.
		 */
		if (gen_rm_svc(mod) != 0)
			continue;	/* service was entirely removed */

		/*
		 *  Determine whether the entire service, or any part
		 *  the service needs to be moved.  If the entire
		 *  service needs to be moved, continue, since there is
		 *  no reason to look at it further.
		 */
		if (gen_mv_svc(mod, s_newproductmod) != 0)
			continue;	/* service was entirely moved */

		/*
		 *  If the service is entirely new,  add it and continue.
		 */
		if ((mod->info.media->med_flags & NEW_SERVICE) &&
		    !(mod->info.media->med_flags & BUILT_FROM_UPGRADE) &&
		    has_view(s_newproductmod, mod) == SUCCESS) {
			(void) load_view(s_newproductmod, mod);
			gen_add_svc(mod);
			continue;
		}

		if ((mod->info.media->med_flags & NEW_SERVICE) &&
		    (mod->info.media->med_flags & BUILT_FROM_UPGRADE))
			continue;

		if (has_view(s_newproductmod, mod) == SUCCESS) {
			(void) load_view(s_newproductmod, mod);
			modify_existing_svc(mod);
		}
	}

	/* generate admin scripts */
	scriptwrite(fp, build_admin_file, "NAME", "dflt",
	    "INSTANCE", "overwrite", "BASEDIR", "default", (char *)0);
	scriptwrite(fp, build_admin_file, "NAME", "root",
	    "INSTANCE", "overwrite", "BASEDIR", "/", (char *)0);
	scriptwrite(fp, build_admin_file, "NAME", "usr",
	    "INSTANCE", "overwrite", "BASEDIR", "/usr", (char *)0);
	scriptwrite(fp, build_admin_file, "NAME", "opt",
	    "INSTANCE", "overwrite", "BASEDIR", "/opt", (char *)0);
	scriptwrite(fp, build_admin_file, "NAME", "un.root",
	    "INSTANCE", "unique", "BASEDIR", "/", (char *)0);
	scriptwrite(fp, build_admin_file, "NAME", "un.usr",
	    "INSTANCE", "unique", "BASEDIR", "/usr", (char *)0);
	scriptwrite(fp, build_admin_file, "NAME", "un.opt",
	    "INSTANCE", "unique", "BASEDIR", "/opt", (char *)0);

	strcpy(newver, s_newproduct->p_version);

	/*
	 * Generate all commands to save old files and remove
	 * defunct packages.
	 */

	scriptwrite(fp, print_rmpkg_msg, (char *)0);
	for (mod = get_media_head(); mod != NULL; mod = mod->next)  {
		if (mod->info.media->med_type == INSTALLED &&
		    mod->info.media->med_flags & BASIS_OF_UPGRADE) {
			(void) load_view(s_newproductmod, mod);
			(void) walklist(mod->sub->info.prod->p_packages,
				save_and_rm, (caddr_t)mod);
			remove_patches(mod);
		} else if (mod->info.media->med_type == INSTALLED_SVC) {
			if ((mod->info.media->med_flags & BUILT_FROM_UPGRADE) &&
			    !(mod->info.media->med_flags & BASIS_OF_UPGRADE))
				continue;
			if ((mod->info.media->med_flags & SVC_TO_BE_REMOVED) &&
			    !(mod->info.media->med_flags & BASIS_OF_UPGRADE))
				continue;
			if (mod->info.media->med_flags & NEW_SERVICE)
				continue;
			if (mod->info.media->med_flags & BASIS_OF_UPGRADE &&
			    mod->info.media->med_upg_to != NULL &&
			    has_view(s_newproductmod,
			    mod->info.media->med_upg_to) == SUCCESS) {
				(void) load_view(s_newproductmod,
				    mod->info.media->med_upg_to);
			} else
				(void) load_view(s_newproductmod, mod);
			(void) walklist(mod->sub->info.prod->p_packages,
			    save_and_rm, (caddr_t)mod);
			if (mod->info.media->med_flags & SVC_TO_BE_REMOVED)
				scriptwrite(fp, rm_template_dir,
				    "SVC", mod->info.media->med_volume,
				    (char *)0);
			if (!(mod->info.media->med_flags & SPLIT_FROM_SERVER))
				remove_patches(mod);
		}
	}

	/*
	 *  Generate all pkgadd commands for new packages.
	 */
	for (mod = get_media_head(); mod != NULL; mod = mod->next)  {
		if (mod->info.media->med_type == INSTALLED &&
		    has_view(s_newproductmod, mod) == SUCCESS) {
			(void) load_view(s_newproductmod, mod);
			(void) walklist(s_newproductmod->info.prod->p_packages,
			    pkgadd_or_spool, (caddr_t)mod);
		} else if (mod->info.media->med_type == INSTALLED_SVC) {
			if ((mod->info.media->med_flags & BUILT_FROM_UPGRADE) &&
			    !(mod->info.media->med_flags & BASIS_OF_UPGRADE))
				continue;
			if ((mod->info.media->med_flags & SVC_TO_BE_REMOVED) &&
			    !(mod->info.media->med_flags & BASIS_OF_UPGRADE))
				continue;
			if (mod->info.media->med_flags & BASIS_OF_UPGRADE &&
			    has_view(s_newproductmod,
			    mod->info.media->med_upg_to) == SUCCESS) {
				(void) load_view(s_newproductmod,
				    mod->info.media->med_upg_to);
				(void) walklist(
				    s_newproductmod->info.prod->p_packages,
				    pkgadd_or_spool, (caddr_t)mod);
			} else if (has_view(s_newproductmod, mod) == SUCCESS) {
				(void) load_view(s_newproductmod, mod);
				(void) walklist(
				    s_newproductmod->info.prod->p_packages,
				    pkgadd_or_spool, (caddr_t)mod);
			}
		}
	}

	/*
	 * Generate all commands to restore permissions
	 */
	for (mod = get_media_head(); mod != NULL; mod = mod->next)  {
		if (mod->info.media->med_type == INSTALLED) {
			(void) walklist(mod->sub->info.prod->p_packages,
				restore_perm, (caddr_t)mod);
		} else if (mod->info.media->med_type == INSTALLED_SVC) {
			if ((mod->info.media->med_flags & BUILT_FROM_UPGRADE) &&
			    !(mod->info.media->med_flags & BASIS_OF_UPGRADE))
				continue;
			if ((mod->info.media->med_flags & SVC_TO_BE_REMOVED) &&
			    !(mod->info.media->med_flags & BASIS_OF_UPGRADE))
				continue;
			if (!(mod->info.media->med_flags & NEW_SERVICE)) {
				(void) walklist(mod->sub->info.prod->p_packages,
				    restore_perm, (caddr_t)mod);
			}
		}
	}

	if (start_perm_printed)
		scriptwrite(fp, end_perm_restores, (char *)0);
	/*
	 * Generate all commands to remove replaced packages
	 * from package database.  Also, generate commands
	 * to remove patch directories for patch packages that
	 * were overwritten by pkgadd.
	 */
	for (mod = get_media_head(); mod != NULL; mod = mod->next)  {
		if (mod->info.media->med_type == INSTALLED) {
			(void) walklist(mod->sub->info.prod->p_packages,
			    walk_pkgrm_f, (caddr_t)mod);
		} else if (mod->info.media->med_type == INSTALLED_SVC) {
			if ((mod->info.media->med_flags & BUILT_FROM_UPGRADE) &&
			    !(mod->info.media->med_flags & BASIS_OF_UPGRADE))
				continue;
			if ((mod->info.media->med_flags & SVC_TO_BE_REMOVED) &&
			    !(mod->info.media->med_flags & BASIS_OF_UPGRADE))
				continue;
			if (!(mod->info.media->med_flags & NEW_SERVICE)) {
				(void) walklist(mod->sub->info.prod->p_packages,
				    walk_pkgrm_f, (caddr_t)mod);
			}
		}
	}

	/*
	 * generate commands to copy CLUSTER and .clustertoc files to
	 * local and service directories.
	 */
	/*
	 * Here's some explanation of the mondo boolean expression
	 * that follows:  environments that need to have their
	 * .clustertoc, CLUSTER, and INST_RELEASE files updated are:
	 * (1) installed environments that have been upgraded,
	 * (2) services that are entirely new or have been built by
	 *	by an upgrade, but with the following exceptions:
	 *	   -  the environment should not be split from the
	 *	      server (the server's update will catch it)
	 *	   -  if the service is built from an upgrade of
	 *	      an existing service, and the service from
	 *	      which the upgrade is being done is split from
	 *	      the server
	 * In addition, any environment getting CLUSTER, .clustertoc,
	 * and INST_RELEASE files must have a view of the new media.
	 */
	for (mod = get_media_head(); mod != NULL; mod = mod->next)  {
		if ((mod->info.media->med_type == INSTALLED &&
		    mod->info.media->med_flags & BASIS_OF_UPGRADE) ||
		    (mod->info.media->med_type == INSTALLED_SVC &&
		    (mod->info.media->med_flags & BUILT_FROM_UPGRADE ||
			mod->info.media->med_flags & NEW_SERVICE) &&
		    (!(mod->info.media->med_flags & SPLIT_FROM_SERVER)) &&
		    !(mod->info.media->med_flags & BUILT_FROM_UPGRADE &&
			mod->info.media->med_upg_from->info.media->med_flags &
			SPLIT_FROM_SERVER)) &&
		    has_view(s_newproductmod, mod) == SUCCESS) {
			(void) load_view(s_newproductmod, mod);
			/*
			 * first, look for a selected metacluster.
			 * If none, look for a required metacluster.
			 */
			for (cmod = s_newproductmod->sub; cmod != NULL;
			    cmod = cmod->next) {
				if (cmod->type == METACLUSTER &&
				    cmod->info.mod->m_status == SELECTED)
					break;
			}
			if (cmod == NULL) {
				for (cmod = s_newproductmod->sub;
				    cmod != NULL;
				    cmod = cmod->next)
					if (cmod->type == METACLUSTER &&
					    cmod->info.mod->m_status ==
					    REQUIRED)
						break;
			}
			if (cmod) {
				scriptwrite(fp, write_CLUSTER,
				    "CLUSTER_PATH", cluster_path(s_newproduct),
				    "ROOT", mod->info.media->med_dir,
				    "CLUSTER", cmod->info.mod->m_pkgid,
				    (char *)0);
			}
			scriptwrite(fp, write_clustertoc,
			    "CLUSTERTOC_PATH", clustertoc_path(s_newproduct),
			    "ROOT", mod->info.media->med_dir,
			    "TOC", get_clustertoc_path(s_newproductmod),
			    (char *)0);
			scriptwrite(fp, echo_INST_RELEASE,
			    "INST_REL_PATH", inst_release_path(s_newproduct),
			    "OS", s_newproduct->p_name,
			    "VERSION", s_newproduct->p_version,
			    "ROOT", mod->info.media->med_dir,
			    "REVISION", s_newproduct->p_rev, (char *)0);
			upg_write_platform_file(fp, mod->info.media->med_dir,
			    s_newproduct, mod->sub->info.prod);
		}
	}

	for (mod = get_media_head(); mod != NULL; mod = mod->next)  {
		if (mod->info.media->med_type == INSTALLED_SVC) {
			if ((mod->info.media->med_flags & BASIS_OF_UPGRADE) &&
			    !(mod->info.media->med_flags &
			    BUILT_FROM_UPGRADE) &&
			    has_view(s_newproductmod,
			    mod->info.media->med_upg_to) == SUCCESS) {
				(void) load_view(s_newproductmod,
				    mod->info.media->med_upg_to);
				gen_inetboot_files(mod);
			} else if ((mod->info.media->med_flags & NEW_SERVICE) &&
			    !(mod->info.media->med_flags &
				    BUILT_FROM_UPGRADE) &&
			    has_view(s_newproductmod, mod) == SUCCESS) {
				(void) load_view(s_newproductmod, mod);
				gen_inetboot_files(mod);
			} else if ((mod->info.media->med_flags & NEW_SERVICE) &&
			    (mod->info.media->med_flags &
				    BUILT_FROM_UPGRADE)) {
				continue;
			} else if (has_view(s_newproductmod, mod) == SUCCESS) {
				(void) load_view(s_newproductmod, mod);
				gen_inetboot_files(mod);
			}
		}
	}

	/* for each client, set up vfstab and inetboot entry */
	/* only upgrade clients with BASIS_OF_UPGRADE flag set */
	if (!g_is_swm && is_server()) {
		sprintf(service, "%s_%s",
		    s_newproduct->p_name, s_newproduct->p_version);
		/* find all clients */
		for (mod = get_media_head(); mod != NULL; mod = mod->next) {
			if (mod->info.media->med_type == INSTALLED &&
			    !streq(mod->info.media->med_dir, "/") &&
			    mod->info.media->med_flags & BASIS_OF_UPGRADE) {
				strcpy(clientarch,
				    mod->sub->info.prod->p_arches->a_arch);
				karch = (char *)strchr(clientarch, '.');
				*karch++ = '\0';
				if (!streq(mod->info.media->med_volume,
				    service)) {
					/*
					* The vfstab needs the kvm entries
					* removed if upgrade is from a
					* pre-KBI to a post-KBI system,
					* since they are no longer used.
					*/
					if (is_KBI_service(s_newproduct) &&
					    ! is_KBI_service(mod->sub->info.prod))
						scriptwrite(fp,
						    sed_vfstab_rm_kvm,
						    "CLIENTROOT",
						    mod->sub->info.prod->p_rootdir,
						    (char *) 0);
					scriptwrite(fp, sed_vfstab,
					    "CLIENTROOT",
					    mod->sub->info.prod->p_rootdir,
					    "OLD", mod->info.media->med_volume,
					    "NEW", service,
					    "ARCH", clientarch,
					    "KARCH", karch, (char *)
					    0);
					
					/*
					 * update tftpboot entries for each
					 * client
					 */
					upgrade_clients_inetboot(mod,
					    s_newproduct);
				}

				scriptwrite(fp, touch_client_reconfigure,
				    "CLIENTROOT",
				    mod->sub->info.prod->p_rootdir,
				    (char *)0);
			}
		}
	}

	if (g_is_swm) {
		for (mod = get_media_head(); mod != NULL; mod = mod->next)  {
			if (mod->info.media->med_type == INSTALLED_SVC &&
	    		    !svc_unchanged(mod->info.media) ||
			    has_view(s_newproductmod, mod) == SUCCESS) {
					(void) load_view(s_newproductmod, mod);
					gen_share_commands(mod);
			}
		}
	}

	if (s_localmedia->info.media->med_flags & BASIS_OF_UPGRADE) {
		if (!is_server()) {
			scriptwrite(fp, echo_softinfo,
			    "SERVICE_PATH",
			    softinfo_services_path(),
			    "OS", s_newproduct->p_name,
			    "VERSION", s_newproduct->p_version,
			    "REVISION", s_newproduct->p_rev, (char *)0);
		}

		if (is_dataless_machine()) {
			sprintf(service, "%s_%s",
			    s_newproduct->p_name, s_newproduct->p_version);
			mod = s_localmedia;
			strcpy(clientarch,
			    mod->sub->info.prod->p_arches->a_arch);
			karch = (char *)strchr(clientarch, '.');
			*karch++ = '\0';
			if (!streq(mod->info.media->med_volume,
			    service)) {
				scriptwrite(fp, sed_dataless_vfstab,
				    "OLD", mod->info.media->med_volume,
				    "NEW", service,
				    "ARCH", clientarch,
				    "KARCH", karch, (char *) 0);
				/*
				 * KBI serivces do not support the kvm
				 * entries previously used, so remove
				 * them.
				 */
				if (is_KBI_service(s_newproduct))
					scriptwrite(fp, sed_vfstab_rm_kvm,
					    "CLIENTROOT", "/", (char *) 0);
			}
		}

		if (install_boot_fcn)
			(*install_boot_fcn)(fp);
		scriptwrite(fp, touch_reconfig, (char *)0);
	}

	scriptwrite(fp, rm_tmp, (char *)0);
	scriptwrite(fp, remove_restart_files, (char *)0);

	scriptwrite(fp, print_cleanup_msg,
	    "UPGRADE_LOG", upgrade_log_path(s_newproduct),
	    "UPGRADE_CLEANUP", upgrade_cleanup_path(),
	    (char *)0);

	scriptwrite(fp, exit_ok, (char *)0);

	(void) fclose(fp);
	return (0);
}

/*
 * set_umount_script_fcn()
 * Parameters:
 *	scriptfcn	-
 * Return:
 *	none
 * Status:
 *	public
 */
void
set_umount_script_fcn(int (*scriptfcn)(FILE *fp, int do_root),
    void (*installbootfcn)(FILE *fp))
{
	script_fcn = scriptfcn;
	install_boot_fcn = installbootfcn;
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * is_KBI_service()
 *
 * Parameters:
 *	prod - 	a pointer to the product module containing service in
 *		question.
 * Return:
 *	1 - if the product is a post KBI product
 *	0 - if the procudt is a pre KBI product.
 * Status:
 *	semi-private
 */
int
is_KBI_service(Product *prod)
{
	/*
	 * This is a quick and dirty way of determining a post-KBI
	 * service. If the version of the OS is 2.5 or greater then it
	 * is a post-KBI service.
	 * THIS WILL HAVE TO CHANGE IN THE FUTURE IF A BETTER WAY IS
	 * FOUND.
	 */

	char	version_string[50];

	if (prod->p_name == NULL || prod->p_version == NULL)
		return (0);

	sprintf(version_string, "%s_%s", prod->p_name,
	    prod->p_version);

	if (prod_vcmp(version_string, "Solaris_2.5") >= 0)
		return (1);	/* A version, 2.5 or higher */
	else
		return (0);	/* Less than 2.5 */
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * gen_inetboot_files()
 * Parameters:
 *	mod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
gen_inetboot_files(Module * mod)
{
	Arch *ap;
	char arch_buf[30], *karch;
	char bootd[10];

	for (ap = s_newproduct->p_arches; ap != NULL; ap = ap->a_next) {
		if (!(ap->a_selected))
			continue;
		strcpy(arch_buf, ap->a_arch);
		karch = (char *)strchr(arch_buf, '.');
		*karch++ = '\0';
		if (strcmp(arch_buf, "i386") == 0)
			strcpy(bootd, "rplboot");
		else
			strcpy(bootd, "tftpboot");
		
		if ((mod->info.media->med_flags & SPLIT_FROM_SERVER) &&
		    strcmp(get_default_inst(), arch_buf) == 0)
			scriptwrite(fp, cp_shared_inetboot,
			    "KARCH", karch,
			    "SVCPROD", s_newproduct->p_name,
			    "SVCVER", s_newproduct->p_version,
			    "BOOTDIR", bootd, (char *)0);
		else
			scriptwrite(fp, cp_svc_inetboot,
			    "KARCH", karch,
			    "ARCH", arch_buf,
			    "SVCPROD", s_newproduct->p_name,
			    "SVCVER", s_newproduct->p_version,
			    "BOOTDIR", bootd, (char *)0);
		scriptwrite(fp, cp_inetboot, "KARCH", karch,
		    "SVCPROD", s_newproduct->p_name,
		    "SVCVER", s_newproduct->p_version,
		    "BOOTDIR", bootd, (char *)0);
	}
}

/*
 *
 * Parameters:
 *	prod
 * Return:
 *	none
 * Status:
 *	private
 */
static void
gen_softinfo(Product * prod)
{
	Arch *ap;
	char temparch[ARCH_LENGTH];
	struct softinfo_merge_entry *smep, *holdsmep;

	scriptwrite(fp, start_softinfo,
	    "SERVICE_PATH", softinfo_services_path(),
	    "OS", s_newproduct->p_name,
	    "VER", s_newproduct->p_version,
	    "REVISION", s_newproduct->p_rev, (char *) 0);

	isa_handled_clear();
	for (ap = prod->p_arches; ap != NULL; ap = ap->a_next) {
		if (ap->a_selected) {
			extract_isa(ap->a_arch, temparch);
			if (!isa_handled(temparch))
				scriptwrite(fp, usr_softinfo,
				    "OS", s_newproduct->p_name,
				    "VER", s_newproduct->p_version,
				    "ARCH", temparch, (char *) 0);
		}
	}

	for (ap = s_newproduct->p_arches; ap != NULL; ap = ap->a_next) {
		if (ap->a_selected) {
			extract_isa(ap->a_arch, temparch);
			if (!isa_handled(temparch))
				scriptwrite(fp, usr_softinfo,
				    "OS", s_newproduct->p_name,
				    "VER", s_newproduct->p_version,
				    "ARCH", temparch, (char *) 0);
		}
	}

	(void) walklist(prod->p_packages, gen_softinfo_merge_chain,
	    (caddr_t)s_newproduct);
	smep_cur = softinfo_merge_chain;

	(void) walklist(s_newproduct->p_packages, walk_root_kvm_softinfo,
	    (caddr_t)s_newproduct);

	/* Now print out the remainder of the softinfo_merge chain */
	while (smep_cur != NULL) {
		_gen_root_kvm_softinfo(smep_cur->cur_mi, s_newproduct);
		smep_cur = smep_cur->next;
	}

	/* free the softinfo merge chain */
	smep = softinfo_merge_chain;
	while (smep) {
		holdsmep = smep->next;
		free(smep);
		smep = holdsmep;
	}

	softinfo_merge_chain = NULL;

	upg_write_plat_softinfo(fp, s_newproduct, prod);

	gen_softinfo_locales(fp, s_newproduct, prod);

	scriptwrite(fp, end_softinfo, (char *)0);
	return;
}

/*
 * returns non-zero if service removed entirely.  Returns zero if
 * none of the service is removed, or if only part of it is removed.
 */
static int
gen_rm_svc(Module *mod)
{
#ifdef SERVICE_REMOVAL_SUPPORTED
	Arch	*ap, *ap2;
	Module	*mod_upg_to;
	char	isa[ARCH_LENGTH];
#endif

	if ((mod->info.media->med_flags & SVC_TO_BE_REMOVED) &&
	    !(mod->info.media->med_flags & BASIS_OF_UPGRADE)) {
		remove_entire_svc(mod);
		return (1);
	}
#ifdef SERVICE_REMOVAL_SUPPORTED
	mod_upg_to = mod->info.media->med_upg_to;

	if (mod_upg_to == NULL)
		mod_upg_to = mod;
	isa_handled_clear();
	for (ap = mod->sub->info.prod->p_arches; ap; ap = ap->a_next) {
		if (!ap->a_loaded)
			continue;
		extract_isa(ap->a_arch, isa);
		/*
		 *  If this ISA is not selected in med_upg_to media,
		 *  remove the ISA's support entirely from this service.
		 */
		if (!isa_is_selected(mod_upg_to->sub->info.prod, isa)) {
			if (!isa_handled(isa))
				remove_isa_support(mod, isa);
			continue;
		}

		/*
		 *  The ISA is still supported, but is the platform group
		 *  (karch) still supported?  If not, remove it.
		 */

		for (ap2 = mod_upg_to->sub->info.prod->p_arches; ap2;
			    ap2 = ap2->a_next) {
			if (streq(ap->a_arch, ap2->a_arch) &&
			    !ap2->a_selected) {
				remove_platform_support(mod, ap->a_arch);
				break;
			}
		}
	}
#endif
	return (0);
}

static void
remove_entire_svc(Module *mod)
{
	char name[MAXPATHLEN];
	char *cp;

	scriptwrite(fp, rm_service, "PRODVER", mod->info.media->med_volume,
	    (char *)0);
	strcpy(name, mod->info.media->med_volume);
	cp = (char *)strchr(name, '_');
	if (cp == NULL)
		return;
	*cp++ = '\0';
	scriptwrite(fp, rm_inetboot, "SVCPROD", name, "SVCVER", cp, (char *)0);
}

/*
 * gen_mv_svc() - Generate the script commands necessary to move
 *	service.
 *
 * Parameters:
 *	mod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static int
gen_mv_svc(Module *mod, Module *prodmod)
{
	Arch	*ap, *newarchlist;
	Module	*mod_upg_to;
	char	isa[ARCH_LENGTH];
	char *newvolptr, *cp;
	char name[MAXPATHLEN];

	mod_upg_to = mod->info.media->med_upg_to;
	if (!(mod->info.media->med_flags & BASIS_OF_UPGRADE) ||
	    svc_unchanged(mod->info.media) ||
	    !mod_upg_to || mod_upg_to == mod)
		return (0);	/* nothing to move */

	if (has_view(prodmod, mod_upg_to) == SUCCESS)
		(void) load_view(prodmod, mod_upg_to);
	else
		return (0);	/* SHOULDN'T HAPPEN */

	newvolptr = mod->info.media->med_upg_to->info.media->med_volume;
	newarchlist = mod->info.media->med_upg_to->sub->info.prod->p_arches;

	/* move ISA-neutral parts of service */
	scriptwrite(fp, mv_whole_svc, "OLD", mod->info.media->med_volume,
	    "NEW", newvolptr, (char *)0);
	/*
	 * remove the old service's entries from the /etc/dfs/dfstab
	 * and add the new ones.
	 */
	scriptwrite(fp, rm_svc_dfstab, "NAME", mod->info.media->med_volume,
		(char *) 0);

	isa_handled_clear();
	for (ap = mod->sub->info.prod->p_arches; ap != NULL; ap = ap->a_next) {
		if (fullarch_is_selected(
		    mod->info.media->med_upg_to->sub->info.prod, ap->a_arch)) {
			extract_isa(ap->a_arch, isa);
			if (!isa_handled(isa)) {
				scriptwrite(fp, mv_isa_svc,
				    "OLD", mod->info.media->med_volume,
				    "NEW", newvolptr,
				    "PARCH", isa, (char *)0);
				scriptwrite(fp, add_usr_svc_dfstab,
				    "NAME", newvolptr, "ISA", isa, (char *) 0);
				/*
				 * This piece of code will update any
				 * Solaris_2.x/usr entries still left in the
				 * dfstab. Things like usr/openwin.
				 */
				scriptwrite(fp, sed_dfstab_usr,
				    "NEW", newvolptr, "ISA", isa,
				    "OLD", mod->info.media->med_volume,
				    (char *) 0);
			}
			/*
			 * If this is an upgrade from a pre-KIB to post-KBI
			 * service we need to remove the oldkvm service
			 * from the media. If this is an upgrade to a
			 * pre-KBI service the KVM service needs to be moved.
			 */
			if (is_KBI_service(s_newproduct) &&
			    (! is_KBI_service(mod->sub->info.prod)))
				scriptwrite(fp, rm_kvm_svc,
				    "OLD", mod->info.media->med_volume,
				    "ARCH", ap->a_arch, (char *)0);
			else if (! is_KBI_service(s_newproduct))
				scriptwrite(fp, mv_kvm_svc,
				    "OLD", mod->info.media->med_volume,
				    "NEW", newvolptr,
				    "ARCH", ap->a_arch, (char *)0);
		}
	}

	scriptwrite(fp, move_files_in_contents,
	    "OLD", mod->info.media->med_volume, "NEW", newvolptr,
	    "ROOT", (mod->info.media->med_flags & SPLIT_FROM_SERVER) ? "" :
	    mod_upg_to->sub->info.prod->p_rootdir,  (char *) 0);

	/*
	 * find all arches that are new in the new service, set up
	 * links to them, if this is not a KBI service. The KVM links
	 * are not used for post-KBI systems.
	 */
	if (!is_KBI_service(s_newproduct)) {
		for (ap = newarchlist; ap != NULL; ap = ap->a_next)
			if (ap->a_selected && !archinlist(ap->a_arch,
			    mod->sub->info.prod->p_arches))
				scriptwrite(fp, add_kvm_svc, "NEW",
				    newvolptr, "ARCH", ap->a_arch,
				    (char *)0);
	}

	/* remove the old service's inetboot */
	strcpy(name, mod->info.media->med_volume);
	cp = (char *)strchr(name, '_');
	*cp++ = '\0';
	scriptwrite(fp, rm_inetboot, "SVCPROD", name, "SVCVER", cp, (char *)0);

	/* remove the old softinfo file and generate the new one */
	scriptwrite(fp, rm_softinfo, "OLD", mod->info.media->med_volume,
	    "SERVICE_PATH", softinfo_services_path(),
	    (char *)0);
	gen_softinfo(mod->sub->info.prod);

	/* If this is a KBI service there is no need to add the kvm */
	/* entries in the dfstab, since they are no longer used. */
	if (!is_KBI_service(s_newproduct))
		for (ap = newarchlist; ap != NULL; ap = ap->a_next)
			if (ap->a_selected)
				scriptwrite(fp, add_kvm_svc_dfstab,
				    "NAME", newvolptr,
				    "ARCH", ap->a_arch, (char *)0);
	return (1);
}

/*
 *
 * Parameters:
 *	mod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
modify_existing_svc(Module *mod)
{
	Arch *ap, *newarchlist;
	char *newvolptr, *cp;
	char name[MAXPATHLEN];
	char isa[ARCH_LENGTH];

	newvolptr = mod->info.media->med_volume;
	newarchlist = s_newproduct->p_arches;

	add_new_isa_svc(mod);
	/*
	 * find all arches that are new in the new service, set up
	 * links to them, only if this is not a KBI service. For
	 * post-KBI systems these links are meaningless.
	 */
	if (!is_KBI_service(s_newproduct))
		for (ap = newarchlist; ap != NULL; ap = ap->a_next)
			if (ap->a_selected && !archinlist(ap->a_arch,
			    mod->sub->info.prod->p_arches)) {
				if (mod->info.media->med_flags &
				    SPLIT_FROM_SERVER &&
				    strcmp(get_default_arch(), ap->a_arch) == 0)
					scriptwrite(fp, link_kvm_svc,
					    "NEW", newvolptr,
					    "ARCH", ap->a_arch, (char *)0);
				else
					scriptwrite(fp, add_kvm_svc,
					    "NEW", newvolptr,
					    "ARCH", ap->a_arch, (char *)0);

			}
	/* remove the service's inetboot file and links */
	strcpy(name, mod->info.media->med_volume);
	cp = (char *)strchr(name, '_');
	*cp++ = '\0';

	/* remove the old softinfo file and generate the new one */
	scriptwrite(fp, rm_softinfo, "OLD", mod->info.media->med_volume,
	    "SERVICE_PATH", softinfo_services_path(),
	    (char *)0);
	gen_softinfo(mod->sub->info.prod);

	/*
	 * remove the old service's entries from the /etc/dfs/dfstab
	 * and add the new ones.
	 */
	scriptwrite(fp, rm_svc_dfstab, "NAME", mod->info.media->med_volume,
		(char *) 0);
	isa_handled_clear();
	for (ap = mod->sub->info.prod->p_arches; ap != NULL; ap = ap->a_next) {
		if (ap->a_selected) {
			extract_isa(ap->a_arch, isa);
			if (!isa_handled(isa)) {
				scriptwrite(fp, add_usr_svc_dfstab,
				    "NAME", newvolptr,
				    "ISA", isa, (char *) 0);
			}
		}
	}
	for (ap = newarchlist; ap != NULL; ap = ap->a_next) {
		if (ap->a_selected) {
			extract_isa(ap->a_arch, isa);
			if (!isa_handled(isa)) {
				scriptwrite(fp, add_usr_svc_dfstab,
				    "NAME", newvolptr,
				    "ISA", isa, (char *) 0);
			}
		}
	}
	/*
	 * If the service being added is pre-KBI then add the dfstab
	 * entries to the server. Else do nothing since the post-KBI
	 * service do not use the kvm links.
	 */
	if (!is_KBI_service(s_newproduct)) {
		isa_handled_clear();
		for (ap = mod->sub->info.prod->p_arches; ap != NULL;
		    ap = ap->a_next) {
			if (ap->a_selected && !isa_handled(ap->a_arch))
				scriptwrite(fp, add_kvm_svc_dfstab,
				    "NAME", newvolptr,
				    "ARCH", ap->a_arch, (char *)0);
		}
		for (ap = newarchlist; ap != NULL; ap = ap->a_next) {
			if (ap->a_selected && !isa_handled(ap->a_arch))
				scriptwrite(fp, add_kvm_svc_dfstab,
				    "NAME", newvolptr,
				    "ARCH", ap->a_arch, (char *)0);
		}
	}
}

/*
 * archinlist()
 * Parameters:
 *	arch	 -
 *	archlist -
 * Return:
 *
 * Status:
 *	private
 */
static int
archinlist(char *arch, Arch *archlist)
{
	Arch *ap;

	for (ap = archlist; ap != NULL; ap = ap->a_next)
		if (strcmp(arch, ap->a_arch) == 0)
			return (1);
	return (0);
}

/*
 * gen_add_svc()
 * Parameters:
 *	mod
 * Return:
 * Status:
 *	private
 */
static void
gen_add_svc(Module *mod)
{
	Arch *ap, *newarchlist;
	char *newvolptr;
	char isa[ARCH_LENGTH];

	newvolptr = mod->info.media->med_volume;
	newarchlist = s_newproduct->p_arches;
	if (mod->info.media->med_flags & SPLIT_FROM_SERVER)
		scriptwrite(fp, link_varsadm_usr,
		    "SVC", mod->info.media->med_volume, (char *)0);

	else {
		scriptwrite(fp, add_varsadm_usr,
		    "SVC", mod->info.media->med_volume,
		    "POST_KBI",
		    is_KBI_service(s_newproduct) ? "postKBI" : "preKBI",
		    (char *)0);
	}

	add_new_isa_svc(mod);

	/*
	 * find all arches that are new in the new service, set up
	 * links to them, only if this is not a KBI service. In a
	 * post-KBI service the kvm entries are not used.
	 */
	if (!is_KBI_service(s_newproduct))
		for (ap = newarchlist; ap != NULL; ap = ap->a_next) {
			if (ap->a_selected) {
				if (mod->info.media->med_flags &
				    SPLIT_FROM_SERVER &&
				    strcmp(get_default_arch(),
					ap->a_arch) == 0)
					scriptwrite(fp, link_kvm_svc,
					    "NEW", newvolptr, "ARCH",
					    ap->a_arch, (char *)0);
				else {
					scriptwrite(fp, add_kvm_svc,
					    "NEW", newvolptr,
					    "ARCH", ap->a_arch, (char *)0);
				}
			}
		}
	gen_softinfo(mod->sub->info.prod);
	/*
	 * add the service's entries to /etc/dfs/dfstab
	 */
	isa_handled_clear();
	for (ap = newarchlist; ap != NULL; ap = ap->a_next)
		if (ap->a_selected) {
			extract_isa(ap->a_arch, isa);
			if (!isa_handled(isa))
				scriptwrite(fp, add_usr_svc_dfstab,
				    "NAME", newvolptr, "ISA", isa, (char *) 0);
		}
	/*
	 * find all arches that are new in the new service, set up
	 * dfstab entries for them, only if this is not a KBI
	 * service. Again, in the post-KBI services the kvm links are
	 * not use therefore the dfstab entries are not needed.
	 */
	if (!is_KBI_service(s_newproduct))
		for (ap = newarchlist; ap != NULL; ap = ap->a_next)
			if (ap->a_selected) {
				scriptwrite(fp, add_kvm_svc_dfstab,
				    "NAME", newvolptr,
				    "ARCH", ap->a_arch, (char *)0);
			}
}

/*
 * save_and_rm()
 * Parameters:
 *	node	-
 * Return:
 * Status:
 *	private
 */
static int
save_and_rm(Node *node, caddr_t val)
{
	Modinfo *mi;
	Module 	*mod;

	mi = (Modinfo *)(node->data);
	/*LINTED [alignment ok]*/
	mod = (Module *)val;
	_save_and_rm(mi, mod);
	while ((mi = next_inst(mi)) != NULL)
		_save_and_rm(mi, mod);
	return (0);
}

/*
 * _save_and_rm()
 * Parameters:
 *	mi	-
 *	mod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_save_and_rm(Modinfo *mi, Module *mod)
{
	struct filediff *diffp;
	char	*cp;
	char	*p;
	char	pathhold[MAXPATHLEN];
	Node	*node;
	Modinfo *tmpmi;

	char	*diff_missing = "DIFF_MISSING";
	char	*diff_type = "DIFF_TYPE";
	char	*diff_slink_target = "DIFF_SLINK_TARGET";
	char	*diff_hlink_target = "DIFF_HLINK_TARGET";
	char	*err;
	char	arg3[MAXPATHLEN], arg4[MAXPATHLEN];

	diffp = mi->m_filediff;
	while (diffp != NULL) {
		/*
		 * This huge condition is determining if we should process
		 * the files on the filediff list. This condition is:
		 * if the replacing package is TO_BE_PKGADDED or
		 * if the action is not TO_BE_PRESERVED and
		 * if the contents of the package are not going away
		 * then process the filediff list.
		 */
		if ((diffp->replacing_pkg != NULL &&
		    (diffp->replacing_pkg->m_status == SELECTED ||
			diffp->replacing_pkg->m_status == REQUIRED) &&
		    diffp->replacing_pkg->m_action == TO_BE_PKGADDED) ||
		/* 2nd condition */
		    (mi->m_action != TO_BE_PRESERVED &&
			!(mi->m_flags & CONTENTS_GOING_AWAY))) {
			err = NULL;
			(void) strcpy(arg3, "0");
			(void) strcpy(arg4, "0");

			if (diffp->diff_flags & DIFF_MISSING)
				err = diff_missing;
			else if (diffp->diff_flags & DIFF_TYPE) {
				err = diff_type;
				(void) sprintf(arg3, "%c", diffp->actual_type);
				(void) sprintf(arg4, "%c", diffp->exp_type);

			} else if (diffp->diff_flags & DIFF_SLINK_TARGET) {
				err = diff_slink_target;
				if (diffp->linkptr != NULL) {
					(void) sprintf(arg3, "%s",
					    diffp->linkptr);
				}
				if (diffp->link_found != NULL) {
					sprintf(arg4, "%s", diffp->link_found);
				}
			} else if (diffp->diff_flags & DIFF_HLINK_TARGET) {
				err = diff_hlink_target;
				if (diffp->linkptr != NULL) {
					sprintf(arg3, "%s", diffp->linkptr);
				}
			}

			if (err != NULL) {
				sprintf(pathhold, "%s/%s",
				    mod->sub->info.prod->p_rootdir,
				    diffp->component_path);
				canoninplace(pathhold);
				scriptwrite(fp, log_file_diff, "ERR", err,
				    "FILE", pathhold,
				    "ARG3", arg3, "ARG4", arg4,
				    (char *)0);
			}

			if ((diffp->diff_flags & DIFF_CONTENTS) ||
			    (err == diff_type && diffp->actual_type == 'd'))
				scriptwrite(fp, rename_file, "DIR",
				    mod->sub->info.prod->p_rootdir,
				    "FILE", diffp->component_path,
				    "VER", mod->info.media->med_volume +
				    strlen(s_newproduct->p_name) + 1,
				    (char *)0);

		}
		diffp = diffp->diff_next;

	}

	if (mi->m_pkg_hist != NULL && mi->m_shared == NOTDUPLICATE) {
		cp = mi->m_pkg_hist->deleted_files;
		if (cp != NULL) {
			scriptwrite(fp, start_rmlist, (char *)0);
			while ((p = split_name(&cp)) != NULL) {
				sprintf(pathhold, "/%s/%s", getrealbase(mi),
				    p);
				/* convert path to canonical form */
				canoninplace(pathhold);
				scriptwrite(fp, addto_rmlist,
				    "FILE", pathhold, (char *)0);
			}
			scriptwrite(fp, end_rmlist, (char *)0);
			scriptwrite(fp, do_removef,
			    "ROOT", mod->sub->info.prod->p_rootdir,
			    "PKG", mi->m_pkginst, (char *)0);
		}
	}
	if (((mi->m_pkg_hist && mi->m_pkg_hist->needs_pkgrm) ||
	    mi->m_flags & DO_PKGRM) &&
	    mi->m_shared == NOTDUPLICATE &&
	    (mi->m_action == TO_BE_REMOVED ||
	    mi->m_action == TO_BE_REPLACED)) {
		tmpmi = mi;
		while ((node = tmpmi->m_next_patch) != NULL) {
			tmpmi = (Modinfo *)(node->data);
			scriptwrite(fp, do_pkgrm,
			    "ROOT", mod->sub->info.prod->p_rootdir,
			    "PKG", tmpmi->m_pkginst, (char *)0);
		}
		scriptwrite(fp, do_pkgrm,
		    "ROOT", mod->sub->info.prod->p_rootdir,
		    "PKG", mi->m_pkginst, (char *)0);
	}
	/* remove any patch packages */
	if (mi->m_shared == SPOOLED_NOTDUP) {
		if (mi->m_action == TO_BE_REMOVED ||
		    mi->m_action == TO_BE_REPLACED) {
			if (mi->m_instdir != NULL) {
				scriptwrite(fp, remove_template,
				    "DIR", mi->m_instdir, (char *)0);
			}
		} else {
			if (mi->m_action == TO_BE_PRESERVED &&
			    mod->info.media->med_flags & BASIS_OF_UPGRADE &&
			    !(mod->info.media->med_flags & BUILT_FROM_UPGRADE)){
				if (mi->m_instdir != NULL &&
				    mod->info.media->med_upg_to != NULL) {
					scriptwrite(fp, move_template,
					    "OLDDIR", mi->m_instdir,
					    "NEWSVC", mod->info.media->
					     med_upg_to->info.media->med_volume,
					    "PKG", mi->m_pkgid,
					    "VER", mi->m_version,
					    "ARCH", mi->m_arch,
					    (char *)0);
				}
		}
#ifdef SPOOLED_PATCHES_SUPPORTED
			while ((node = mi->m_next_patch) != NULL) {
				mi = (Modinfo *)(node->data);
				scriptwrite(fp, do_rm_fr,
				    "DIR", mi->m_instdir, (char *)0);
			}
#endif
		}
	}
	if (((!mi->m_pkg_hist || !mi->m_pkg_hist->needs_pkgrm) &&
	    !(mi->m_flags & DO_PKGRM)) &&
	    mi->m_shared == NOTDUPLICATE &&
	    (mi->m_action == TO_BE_REMOVED || mi->m_action == TO_BE_REPLACED)) {
		tmpmi = mi;
		while ((node = tmpmi->m_next_patch) != NULL) {
			tmpmi = (Modinfo *)(node->data);
			scriptwrite(fp, do_pkgrm,
			    "ROOT", mod->sub->info.prod->p_rootdir,
			    "PKG", tmpmi->m_pkginst, (char *)0);
		}
	}
}

/*
 * _save_and_rm()
 * Parameters:
 *	mi	-
 *	mod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
remove_patches(Module *mod)
{
	struct patch	*p;

	for (p = mod->sub->info.prod->p_patches; p != NULL; p = p->next)
		if (p->removed)
			scriptwrite(fp, remove_patch,
			    "ROOT", mod->sub->info.prod->p_rootdir,
			    "PATCHID", p->patchid, (char *)0);
}

/*
 * restore_perm()
 * Parameters:
 *	np	-
 *	data	-
 * Return:
 * Status:
 *	private
 */
static int
restore_perm(Node *np, caddr_t data)
{
	Modinfo *mi;
	Module 	*mod;

	mi = (Modinfo *)(np->data);
	/*LINTED [alignment ok]*/
	mod = (Module *)data;
	_restore_perm(mi, mod);
	while ((mi = next_inst(mi)) != NULL)
		_restore_perm(mi, mod);
	return (0);
}

/*
 * _restore_perm()
 * Parameters:
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_restore_perm(Modinfo * mi, Module * mod)
{
	struct filediff *diffp;

	if (mi->m_action == TO_BE_REPLACED) {
		for (diffp = mi->m_filediff; diffp != NULL;
		    diffp = diffp->diff_next) {
			if (!(diffp->diff_flags &
			    (DIFF_PERM | DIFF_UID | DIFF_GID)))
				continue;
			if (!start_perm_printed) {
				scriptwrite(fp, start_perm_restores, (char *)0);
				start_perm_printed = 1;
			}
			if (diffp->diff_flags & DIFF_PERM)
				scriptwrite(fp, chmod_file, "DIR",
				    mod->sub->info.prod->p_rootdir,
				    "FILE", diffp->component_path,
				    "MODE", cvtperm(diffp->act_mode),
				    (char *)0);
			if (diffp->diff_flags & DIFF_UID)
				scriptwrite(fp, chown_file, "DIR",
				    mod->sub->info.prod->p_rootdir,
				    "FILE", diffp->component_path,
				    "OWNER", cvtuid(diffp->act_uid), (char *)0);
			if (diffp->diff_flags & DIFF_GID)
				scriptwrite(fp, chgrp_file, "DIR",
				    mod->sub->info.prod->p_rootdir,
				    "FILE", diffp->component_path,
				    "GROUP", cvtgid(diffp->act_gid), (char *)0);
		}
	}
}

/*
 * walk_pkgrm_f()
 * Parameters:
 *	node	-
 *	val	-
 * Return:
 * Status:
 *	private
 */
static int
walk_pkgrm_f(Node *node, caddr_t val)
{
	Modinfo *mi;
	Module 	*mod;

	mi = (Modinfo *)(node->data);
	/*LINTED [alignment ok]*/
	mod = (Module *)val;
	_walk_pkgrm_f(mi, mod);
	while ((mi = next_inst(mi)) != NULL)
		_walk_pkgrm_f(mi, mod);
	return (0);
}

/*
 * _walk_pkgrm_f()
 * Parameters:
 *	mi	-
 *	mod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_walk_pkgrm_f(Modinfo *mi, Module *mod)
{
	if (mi->m_pkg_hist && !mi->m_pkg_hist->needs_pkgrm &&
	    !(mi->m_flags & DO_PKGRM) &&
	    mi->m_shared == NOTDUPLICATE &&
	    mi->m_action == TO_BE_REMOVED) {
		scriptwrite(fp, do_pkgrm_f,
		    "ROOT", mod->sub->info.prod->p_rootdir,
		    "PKG", mi->m_pkginst, (char *)0);
	}
}

/*
 * walk_root_kvm_softinfo()
 * Parameters:
 *	np	-
 *	data	-
 * Return:
 *	0
 * Status:
 *	private
 */
static int
walk_root_kvm_softinfo(Node * np, caddr_t data)
{
	Modinfo *mi;
	Product	*prod;

	mi = (Modinfo *)(np->data);
	/*LINTED [alignment ok]*/
	prod = (Product *)data;
	_walk_root_kvm_softinfo(mi, prod);
	while ((mi = next_inst(mi)) != NULL)
		_walk_root_kvm_softinfo(mi, prod);
	return (0);
}

static void
_walk_root_kvm_softinfo(Modinfo * mi, Product *prod)
{
	struct softinfo_merge_entry *matchsmep, *smep;

	if (mi->m_shared == NULLPKG ||
	    (mi->m_sunw_ptype != PTYPE_KVM && mi->m_sunw_ptype != PTYPE_ROOT))
		return;

	/*
	 *  See if this package is on the already-printed part of the
	 *  softinfo_merge_chain
	 */
	matchsmep = softinfo_merge_chain;
	for (matchsmep = softinfo_merge_chain; matchsmep != smep_cur;
	    matchsmep = matchsmep->next) {
		if (matchsmep->new_mi == mi)
			return; /* already handled */
	}
	for (matchsmep = smep_cur; matchsmep != NULL;
	    matchsmep = matchsmep->next) {
		if (matchsmep->new_mi == mi) {
			smep = smep_cur;
			while (smep != matchsmep) {
				_gen_root_kvm_softinfo(smep->cur_mi, prod);
				smep = smep->next;
			}
			_gen_root_kvm_softinfo(smep->cur_mi, prod);
			smep_cur = smep->next;
			return;
		}
	}
	
	if (mi->m_status == UNSELECTED ||
	    (mi->m_sunw_ptype == PTYPE_ROOT &&
		mi->m_shared != SPOOLED_NOTDUP &&
		mi->m_shared != SPOOLED_DUP &&
		mi->m_action != TO_BE_SPOOLED))
		return;
	
	_gen_root_kvm_softinfo(mi, prod);
}

/*
 * _gen_root_kvm_softinfo()
 * Parameters:
 *	mi	-
 *	prod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_gen_root_kvm_softinfo(Modinfo *mi, Product *prod)
{
	char rootsize[10];
	char fullarch[20];
	char mapname[MAXPATHLEN];

	/*
	 * For post-KBI systems there is no reason to add the KVM_ROOT
	 * information into the softinfo file. This is due to the fact
	 * that post-KBI systems handle KBI packages just like and
	 * other USR package.
	 */
	if (mi->m_sunw_ptype == PTYPE_KVM && ! is_KBI_service(s_newproduct)) {
		scriptwrite(fp, kvm_softinfo, "ARCH", mi->m_arch,
		    "OS", prod->p_name, "VER", prod->p_version, (char *) 0);
	} else if (mi->m_sunw_ptype == PTYPE_ROOT) {
		if (mi->m_deflt_fs[ROOT_FS] + mi->m_deflt_fs[VAR_FS] == 0) {
			sprintf(mapname, "%s%s/%s/pkgmap", get_rootdir(),
			    mi->m_instdir, mi->m_pkg_dir);
			calc_pkg_space(mapname, mi);
		}
		sprintf(rootsize, "%ld",
		    mi->m_deflt_fs[ROOT_FS] + mi->m_deflt_fs[VAR_FS]);
		if (strrchr(mi->m_arch, '.') == 0)
			sprintf(fullarch, "%s.all", mi->m_arch);
		else
			strcpy(fullarch, mi->m_arch);
		if (mi->m_instdir)
			scriptwrite(fp, root_softinfo,
			    "ARCH", fullarch,
			    "PATH", mi->m_instdir,
			    "PKG", mi->m_pkgid,
			    "SIZE", rootsize,
			    "VERSION", mi->m_version, (char *)0);
	}
}

/*
 * gen_softinfo_merge_chain()
 * Parameters:
 *	np	-
 *	data	-
 * Return:
 *	0
 * Status:
 *	private
 */
static int
gen_softinfo_merge_chain(Node * np, caddr_t data)
{
	Modinfo *mi, *j;
	Product	*prod;

	mi = (Modinfo *)(np->data);
	/*LINTED [alignment ok]*/
	prod = (Product *)data;

	/* Check the header and it patches to see if they need to be */
	/* added. */
	if (mi->m_status != UNSELECTED) {
		_gen_softinfo_merge_chain(mi, prod);
		for (j = next_patch(mi); j != NULL; j = next_patch(j))
			_gen_softinfo_merge_chain(j, prod);
	}

	/*
	 * Step throuch the instances and the patches to see if they
	 * need to be added to the chain.
	 * The check of unselected is because the patches are not
	 * selected, but the rule is that if a package is selected that
	 * implicitly its patches are selected.
	 */
	while ((mi = next_inst(mi)) != NULL)
		if (mi->m_status != UNSELECTED)
			for (j = mi; j != NULL; j = next_patch(j))
				_gen_softinfo_merge_chain(j, prod);

	return (0);
}

/*
 * _gen_softinfo_merge_chain()
 * Parameters:
 *	mi	-
 *	prod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_gen_softinfo_merge_chain(Modinfo *mi, Product *prod)
{
	Arch_match_type		match;
	Modinfo			*mnew;
	struct softinfo_merge_entry	*sme, **smepp;

	/* If the package is null or not to be preserved then do */
	/* nothing */
	if (mi->m_shared == NULLPKG || mi->m_action == TO_BE_REMOVED ||
	    (mi->m_status == LOADED && mi->m_action != TO_BE_PRESERVED))
		return;
	if (mi->m_sunw_ptype == PTYPE_ROOT &&
	    mi->m_shared != SPOOLED_NOTDUP &&
	    mi->m_shared != SPOOLED_DUP &&
	    mi->m_action != TO_BE_SPOOLED)
		return;
	if (mi->m_sunw_ptype == PTYPE_KVM || mi->m_sunw_ptype == PTYPE_ROOT) {
		mnew = find_new_package(prod, mi->m_pkgid, mi->m_arch, &match);
		if (mnew == NULL &&
		    (mi->m_action == TO_BE_REPLACED ||
		    mi->m_action == TO_BE_REMOVED))
			return;
		sme = (struct softinfo_merge_entry *) xmalloc((size_t)
		    sizeof (struct softinfo_merge_entry));
		/*
		 *  Note that mnew might be NULL.  That's OK.  The
		 *  code that reads this list will expect that.
		 */
		sme->new_mi = mnew;
		sme->cur_mi = mi;
		for (smepp = &softinfo_merge_chain;
		    *smepp != NULL; smepp = &((*smepp)->next)) ;
		*smepp = sme;
		sme->next = NULL;
	}
}

/*
 * getrealbase()
 * Parameters:
 *	mi	-
 * Return:
 * Status:
 *	private
 */
static char *
getrealbase(Modinfo * mi)
{
	if (mi->m_instdir != NULL)
		return (mi->m_instdir);
	else
		return (mi->m_basedir);
}

/*
 * pkgadd_or_spool()
 * Parameters:
 *	np	-
 *	data	-
 * Return:
 *	0
 * Status:
 *	private
 */
static int
pkgadd_or_spool(Node * np, caddr_t data)
{
	Modinfo *mi;
	Module 	*mod;

	mi = (Modinfo *)(np->data);
	/*LINTED [alignment ok]*/
	mod = (Module *)data;
	_pkgadd_or_spool(mi, mod);
	while ((mi = next_inst(mi)) != NULL)
		_pkgadd_or_spool(mi, mod);
	return (0);
}

/*
 * _pkgadd_or_spool()
 * Parameters:
 *	mi	-
 *	mod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_pkgadd_or_spool(Modinfo * mi, Module * mod)
{
	char *adminfile;
	char pkginst[16];
	char *realbase;

	if ((mi->m_status == SELECTED || mi->m_status == REQUIRED) &&
	    (mi->m_action == TO_BE_SPOOLED || mi->m_action == TO_BE_PKGADDED)) {
		strcpy(pkginst, mi->m_pkg_dir);
		realbase = getrealbase(mi);
		if (mi->m_action == TO_BE_PKGADDED) {
			adminfile = NULL;
			if (mi->m_flags & INSTANCE_ALREADY_PRESENT) {
				if (strcmp(realbase, "/") == 0)
					adminfile = "root";
				else if (strcmp(realbase, "/usr") == 0)
					adminfile = "usr";
				else if (strcmp(realbase, "/opt") == 0)
					adminfile = "opt";
				else
					adminfile = newadmin(realbase,
					    OVERWRITE);
			} else {
				if (strcmp(realbase, "/") == 0)
					adminfile = "un.root";
				else if (strcmp(realbase, "/usr") == 0)
					adminfile = "un.usr";
				else if (strcmp(realbase, "/opt") == 0)
					adminfile = "un.opt";
				else
					adminfile = newadmin(realbase,
					    UNIQUE);
			}
			if (!copyright_printed) {
				scriptwrite(fp, print_copyright,
				    "SPOOL", s_newproduct->p_pkgdir,
				    "PKG", pkginst, (char *)0);
				copyright_printed = 1;
			}
			scriptwrite(fp, do_pkgadd,
			    "ROOT", mod->sub->info.prod->p_rootdir,
			    "SPOOL", s_newproduct->p_pkgdir,
			    "ADMIN", adminfile,
			    "PKG", pkginst, (char *)0);
		} else if (mi->m_action == TO_BE_SPOOLED) {
			scriptwrite(fp, spool_pkg, "SPOOLDIR",
			mi->m_instdir, "PKG", pkginst, "MEDIA",
			s_newproduct->p_pkgdir, (char *)0);

		}
	}
}

/*
 * newadmin()
 * Parameters:
 *	basedir	 -
 *	instance -
 * Return:
 * Status:
 *	private
 */
static char *
newadmin(char * basedir, enum instance_type instance)
{
	struct admin_file *afp;

	for (afp = adminfile_head; afp != NULL; afp = afp->next) {
		if (strcmp(basedir, afp->basedir) == 0 &&
		    afp->inst_type == instance)
			return (afp->admin_name);
	}
	afp = (struct admin_file *) xmalloc(sizeof (struct admin_file) +
					strlen(basedir));
	(void) strcpy(afp->basedir, basedir);
	(void) sprintf(afp->admin_name, "%d", admin_seq++);
	afp->inst_type = instance;
	afp->next = adminfile_head;
	adminfile_head = afp;
	scriptwrite(fp, build_admin_file,
			"NAME", afp->admin_name,
			"INSTANCE",
			(instance == UNIQUE ? "unique" : "overwrite"),
			"BASEDIR",
			basedir,
			(char *)0);
	return (afp->admin_name);
}

/*
 * gen_share_commands()
 * Parameters:
 *	mod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
gen_share_commands(Module *mod)
{
	Arch *ap, *newarchlist;
	char *newvolptr;
	char	isa[ARCH_LENGTH];

	newvolptr = mod->info.media->med_volume;
	newarchlist = s_newproduct->p_arches;
	/*
	 * share the service
	 */
	isa_handled_clear();
	for (ap = newarchlist; ap != NULL; ap = ap->a_next)
		if (ap->a_selected) {
			extract_isa(ap->a_arch, isa);
			if (!isa_handled(isa))	
				scriptwrite(fp, share_usr_svc_dfstab,
				    "NAME", newvolptr, "PARCH", isa,
				    (char *) 0);
		}
	/*
	 * find all arches that are new in the new service
	 * and share them, only if this is not a KBI
	 * service. In post-KBI services there is no need since the
	 * KVM entries are not used.
	 */
	if (!is_KBI_service(s_newproduct))
		for (ap = newarchlist; ap != NULL; ap = ap->a_next)
			if (ap->a_selected) {
				scriptwrite(fp, share_kvm_svc_dfstab,
				    "NAME", newvolptr,
				    "ARCH", ap->a_arch, (char *)0);
			}

}

/*
 * cvtperm()
 * Parameters:
 *	mode
 * Return:
 * Status:
 *	private
 */
static char *
cvtperm(mode_t mode)
{
	sprintf(ascii_number, "%lo", mode);
	return (ascii_number);
}

/*
 * cvtuid()
 * Parameters:
 *	uid	-
 * Return:
 * Status:
 *	private
 */
static char *
cvtuid(uid_t uid)
{
	sprintf(ascii_number, "%ld", uid);
	return (ascii_number);
}

/*
 * cvtgid()
 * Parameters:
 *	gid	-
 * Return:
 * Status:
 *	private
 */
static char *
cvtgid(gid_t gid)
{
	sprintf(ascii_number, "%ld", gid);
	return (ascii_number);
}

static void
add_new_isa_svc(Module *mod)
{
	Arch *ap, *newarchlist;
	char isa[ARCH_LENGTH];

	newarchlist = s_newproduct->p_arches;

	/*
	 * find all ISA's that are new in the new service, set up
	 * links to them.
	 */
	isa_handled_clear();
	for (ap = newarchlist; ap != NULL; ap = ap->a_next) {
		if (ap->a_selected) {
			extract_isa(ap->a_arch, isa);
			if (!isa_is_loaded(mod->sub->info.prod, isa) &&
			    !isa_handled(isa)) {
				if (mod->info.media->med_flags &
				    SPLIT_FROM_SERVER &&
				    strcmp(get_default_inst(), isa) == 0)
					scriptwrite(fp, link_usr_svc,
					    "SVC", mod->info.media->med_volume,
					    "ISA", isa, (char *)0);
				else
					scriptwrite(fp, add_usr_svc,
					    "SVC", mod->info.media->med_volume,
					    "ISA", isa, (char *)0);
			}
		}
	}
}

/*
 * get_product()
 *
 */
static Product *
get_product(void)
{
	Product		*theProd;

 	if (s_localmedia->info.media->med_flags & BASIS_OF_UPGRADE)
 		theProd = s_newproduct;
 	else
 		theProd = s_localmedia->sub->info.prod;
	return (theProd);
}

/*
 * *_path()
 *	These functions will return the path to a specific file based on
 *	the product passed in as the argument. The path generation is based
 *	on the pre or port_KBI status of the product. Pre-KBI uses the old
 *	var/sadm directory tree and the post-KBI systems use the new
 *	var/sadm structure.
 *
 * Parameters:
 *	prod	- the product to be used in the comparson
 * Return:
 *	a file path to the given file
 * Status:
 *	private
 */

char *
upgrade_script_path(Product *prod)
{
	if (is_KBI_service(prod))
		return ("/var/sadm/system/admin/upgrade_script");
	else
		return ("/var/sadm/install_data/upgrade_script");
}
char *
upgrade_log_path(Product *prod)
{
	if (is_KBI_service(prod))
		return ("/var/sadm/system/logs/upgrade_log");
	else
		return ("/var/sadm/install_data/upgrade_log");
}
static char *
upgrade_restart_path(void)
{
	if (is_KBI_service(get_product()))
		return ("/var/sadm/system/admin/upgrade_restart");
	else
		return ("/var/sadm/install_data/upgrade_restart");
}
static char *
upgrade_cleanup_path(void)
{
	if (is_KBI_service(get_product()))
		return ("/var/sadm/system/data/upgrade_cleanup");
	else
		return ("/var/sadm/install_data/upgrade_cleanup");
}
static char *
upgrade_failedpkgs_path(void)
{
	if (is_KBI_service(get_product()))
		return ("/var/sadm/system/data/upgrade_failed_pkgadds");
	else
		return ("/var/sadm/install_data/upgrade_failed_pkgadds");
}
static char *
inst_release_path(Product *prod)
{
	if (is_KBI_service(prod))
		return ("/var/sadm/system/admin/INST_RELEASE");
	else
		return ("/var/sadm/softinfo/INST_RELEASE");
}
static char *
softinfo_services_path(void)
{
	if (is_KBI_service(get_product()))
		return ("/var/sadm/system/admin/services/");
	else
		return ("/var/sadm/softinfo/");
}
static char *
cluster_path(Product *prod)
{
	if (is_KBI_service(prod))
		return ("/var/sadm/system/admin/CLUSTER");
	else
		return ("/var/sadm/install_data/CLUSTER");
}
static char *
clustertoc_path(Product *prod)
{
	if (is_KBI_service(prod))
		return ("/var/sadm/system/admin/.clustertoc");
	else
		return ("/var/sadm/install_data/.clustertoc");
}
static void
gen_softinfo_locales(FILE *fp, Product *prod1, Product *prod2)
{
	isa_handled_clear();
	merge_softinfo_locales(fp, prod1);
	merge_softinfo_locales(fp, prod2);
}

static void
merge_softinfo_locales(FILE *fp, Product *prod)
{
	char	buf[MAXPATHLEN];
	Module	*modp, *subp;

	for (modp = prod->p_locale; modp; modp = modp->next) {
		if (!modp->info.locale->l_selected)
			continue;
		for (subp = modp->sub; subp; subp = subp->next) {
			if (subp->info.mod->m_arch) {
				sprintf(buf, "%s:%s", subp->info.mod->m_arch,
				    modp->info.locale->l_locale);
				if (!isa_handled(buf))
					scriptwrite(fp, locale_softinfo,
					    "ARCH", subp->info.mod->m_arch,
					    "LOC", modp->info.locale->l_locale,
					    (char *)0);
			}
		}
	}
}

/*
 * upgrade_clients_inetboot()
 * Parameters:
 *	mod	- the media of the client
 * Return:
 *	none
 * Status:
 *	private
 */
static void
upgrade_clients_inetboot(Module * mod, Product *newproduct)
{
	Arch *ap;
	char arch_buf[30], *karch;
	char bootd[10];
	StringList	*host;
	char		*ip_addr;
	char		searchFile[MAXPATHLEN];
	ulong		addr;

	for (ap = mod->sub->info.prod->p_arches; ap != NULL;
						 ap = ap->a_next) {
		if (!(ap->a_selected))
			continue;
		strcpy(arch_buf, ap->a_arch);
	}
	
	karch = (char *)strchr(arch_buf, '.');
	*karch++ = '\0';
	if (karch == NULL)
		return;
	
	if (strcmp(arch_buf, "i386") == 0)
		strcpy(bootd, "rplboot");
	else
		strcpy(bootd, "tftpboot");
		
	for (host = mod->info.media->med_hostname; host != NULL;
						   host = host->next ) {
		if ((ip_addr = name2ipaddr(host->string_ptr)) == NULL)
			continue;
			
		if (strcmp(arch_buf, "i386") == 0) {
			sprintf(searchFile, "%s.inetboot", ip_addr);
		} else {
			addr = inet_addr(ip_addr);
			sprintf(searchFile, "%08lX", htonl(addr));
		}
		scriptwrite(fp, upgrade_client_inetboot,
		    "KARCH", karch,
		    "SVCPROD", newproduct->p_name,
		    "SVCVER", newproduct->p_version,
		    "SEARCHFILE", searchFile,
		    "BOOTDIR", bootd, (char *)0);
	}
}
