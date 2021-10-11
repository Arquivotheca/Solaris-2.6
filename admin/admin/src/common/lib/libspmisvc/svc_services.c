#ifndef lint
#pragma ident "@(#)svc_services.c 1.6 96/06/25 SMI"
#endif
/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 * All Rights Reserved.
 */
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "spmisvc_lib.h"
#include "spmicommon_api.h"
#include "spmisoft_lib.h"
#include "spmistore_api.h"

/* Public Function Prototypes */
SW_service_list  *list_available_services(char *, SW_error_info **);
SW_service_list  *list_installed_services(SW_error_info **);
StringList	*list_avail_svc_platforms(char *,SW_service *,
    SW_error_info **);
StringList	*list_installed_svc_platforms(SW_service *, SW_error_info **);
int	 	validate_service_modification(SW_service_modspec *,
				SW_error_info **);
int  	 	execute_service_modification(SW_service_modspec *,
				SW_error_info **);
SW_createroot_info * get_createroot_info(SW_service *, SW_error_info **);

/* Private Function Prototypes */
static int	setup_service_modification(SW_service_modspec *,
    SW_error_info **);
static int	read_service_file(SW_service_list *, char *, char *,
    SW_error_info **);
static SW_error_info *alloc_sw_error(SW_return_code);
static Module	*findmeta(Module *, SW_metacluster *);
static int	 do_add_service(SW_service_mod *, SW_error_info **);
static int	is_pkg(char *);

extern SW_diffrev *g_sw_diffrev;

/*
 * list_available_services
 * Parameters:
 *	media	-
 *	err_block - 
 * Return:
 * Status:
 *	public
 */
SW_service_list *
list_available_services(char *media, SW_error_info **err_info)
{
	Module	*mediamod;
	Arch	*ap;
	Product	*prodinfo;
	int	count;
	SW_service_list	*svlist;
	SW_service	*svc;
	SW_locale	*loc;
	SW_metacluster	*meta;
	char	*archstr, *karch;
	Module  *modp;
	char	*cp;

	set_rootdir("/");
	/*  see if media has already been loaded */
	mediamod = find_media(media, NULL);
	if (mediamod == NULL) {
		mediamod = add_media(media);
		if (mediamod == NULL || load_media(mediamod, TRUE) != SUCCESS) {
			*err_info = alloc_sw_error(SW_MEDIA_FAILURE);
			return (NULL);
		}
	}

	/*  media is now loaded */

	prodinfo = mediamod->sub->info.prod;
	svlist = (SW_service_list *)xcalloc((size_t) sizeof (SW_service_list));
	count = 0;
	isa_handled_clear();
	for (ap = prodinfo->p_arches; ap; ap = ap->a_next) {
		archstr = xstrdup(ap->a_arch);
		karch = strchr(archstr, '.');
		*karch++ = '\0';
		svc = (SW_service *)xcalloc((size_t) sizeof (SW_service));
		svc->sw_svc_os = xstrdup(prodinfo->p_name);
		svc->sw_svc_version = xstrdup(prodinfo->p_version);
		svc->sw_svc_isa = xstrdup(archstr);
		svc->sw_svc_plat = xstrdup(karch);
		free(archstr);
		link_to((Item **)&svlist->sw_svl_services, (Item *)svc);
		count++;
		if (isa_handled(svc->sw_svc_isa))
			continue;
		/*  Generate list of locales */
		for (modp = prodinfo->p_locale; modp; modp = modp->next) {
			loc = (SW_locale *)xcalloc((size_t) sizeof (SW_locale));
			loc->sw_loc_name =
			    xstrdup(modp->info.locale->l_locale);
			if ((cp = get_C_lang_from_locale( loc->sw_loc_name))
			    != NULL)
				loc->sw_loc_nametext = xstrdup(cp);
			else
				loc->sw_loc_nametext = NULL;
			loc->sw_loc_os = xstrdup(prodinfo->p_name);
			loc->sw_loc_ver= xstrdup(prodinfo->p_version);
			loc->sw_loc_isa = xstrdup(svc->sw_svc_isa);
			link_to((Item **)&svlist->sw_svl_locales, (Item *)loc);
		}

		/*  Generate list of metaclusters */
		if (mediamod->sub)
			modp = mediamod->sub->sub;
		for (  ; modp; modp = modp->next) {
			if (modp->type != METACLUSTER)
				break;
			meta = (SW_metacluster *)xcalloc((size_t) sizeof
			    (SW_metacluster));
			meta->sw_meta_name = xstrdup(modp->info.mod->m_pkgid);
			if (modp->info.mod->m_name)
				meta->sw_meta_desc =
				    xstrdup(modp->info.mod->m_name);
			meta->sw_meta_os = xstrdup(prodinfo->p_name);
			meta->sw_meta_ver = xstrdup(prodinfo->p_version);
			meta->sw_meta_isa =
			    xstrdup(svc->sw_svc_isa);
			link_to((Item **)&svlist->sw_svl_metaclusters,
			    (Item *)meta);
		}
	}
	svlist->sw_svl_num_services = count;

	return (svlist);
}

/*
 * list_avail_svc_platforms
 * Parameters:
 *	media	-
 *	err_block - 
 * Return:
 * Status:
 *	public
 */
StringList *
list_avail_svc_platforms(char *media, SW_service *svc, SW_error_info **err_info)
{
	Module	*mediamod;
	Arch	*ap;
	Product	*prodinfo;
	char	fullarch[256];
	StringList	*sl, *nsl;
	StringList	*head_sl = NULL;

	set_rootdir("/");

	/*  see if media has already been loaded */
	mediamod = find_media(media, NULL);
	if (mediamod == NULL) {
		mediamod = add_media(media);
		if (mediamod == NULL || load_media(mediamod, TRUE) != SUCCESS) {
			*err_info = alloc_sw_error(SW_MEDIA_FAILURE);
			return (NULL);
		}
	}

	/*  media is now loaded */

	prodinfo = mediamod->sub->info.prod;
	if (!streq(svc->sw_svc_os, prodinfo->p_name) ||
	    !streq(svc->sw_svc_version, prodinfo->p_version)) {
		return (NULL);
	}
	(void) strcpy(fullarch, svc->sw_svc_isa);
	(void) strcat(fullarch, ".");
	(void) strcat(fullarch, svc->sw_svc_plat);
	for (ap = prodinfo->p_arches; ap; ap = ap->a_next) {
		if (!streq(fullarch, ap->a_arch))
			continue;
		if (ap->a_platforms) {
			for (sl = ap->a_platforms; sl; sl = sl->next) {
				nsl = (StringList *)xcalloc((size_t)
				    sizeof(StringList));
				nsl->string_ptr = xstrdup(sl->string_ptr);
				link_to((Item **)&head_sl, (Item *)nsl);
			}
		} else {
			nsl = (StringList *)xcalloc((size_t)
			    sizeof(StringList));
			nsl->string_ptr = xstrdup(ap->a_arch);
			link_to((Item **)&head_sl, (Item *)nsl);
		}
		break;
	}
	return (head_sl);
}

/*
 * list_installed_services
 * Parameters:
 *	err_block - 
 * Return:
 * Status:
 *	public
 */
SW_service_list *
list_installed_services(SW_error_info **err_info)
{
	DIR		*dirp;
	struct dirent	*dp;
	SW_service_list	*svlist;
	char		*servdir;
	int		ret;

	svlist = (SW_service_list *)xcalloc((size_t) sizeof (SW_service_list));

	servdir = services_read_path("/");
	if ((dirp = opendir(servdir)) == (DIR *)0) {
		svlist->sw_svl_num_services = 0;
		return (svlist);
	}

	while ((dp = readdir(dirp)) != (struct dirent *)0) {
		if (streq(dp->d_name, ".") || streq(dp->d_name, ".."))
			continue;
		ret = read_service_file(svlist, servdir, dp->d_name, err_info);
		if (ret == -2)
			return (NULL);
	}
	(void) closedir(dirp);
	return (svlist);
}

/*
 * list_installed_svc_platforms
 * Parameters:
 *	media	-
 *	err_block - 
 * Return:
 * Status:
 *	public
 */
StringList *
list_installed_svc_platforms(SW_service *svc, SW_error_info **err_info)
{
	char	 service_name[256];
	char	 softinfo_file[MAXPATHLEN];
	FILE	*fp;
	char	 buf[BUFSIZ + 1];
	char	 key[BUFSIZ];
	int	 len;
	char	*cp, *isa, *servdir;
	char	 platarch[ARCH_LENGTH];
	int	 platgrp_found = 0;
	int	 platlist_inprogress = 0;
	StringList	 *sl;
	StringList	*head_sl = NULL;

	if (svc->sw_svc_os == NULL || svc->sw_svc_version == NULL ||
	    svc->sw_svc_isa == NULL || svc->sw_svc_plat == NULL) {
		*err_info = alloc_sw_error(SW_INVALID_SVC);
		return (NULL);
	}

	servdir = services_read_path("/");

	(void) sprintf(service_name, "%s_%s", svc->sw_svc_os, svc->sw_svc_version);
	(void) sprintf(platarch, "%s.%s", svc->sw_svc_isa, svc->sw_svc_plat);
	(void) sprintf(softinfo_file, "%s/%s", servdir, service_name);

	if ((fp = fopen(softinfo_file, "r")) == (FILE *)NULL) {
		*err_info = alloc_sw_error(SW_INVALID_SVC);
		return (NULL);
	}

	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf) - 1] = '\0';
		if (buf[0] == '#' || buf[0] == '\n')
			continue;

		if ((cp = strchr(buf, '=')) == NULL)
			continue;
		len = cp - buf;
		(void) strncpy(key, buf, len);
		key[len] = '\0';
		cp++;	/* cp now points to string after '=' */

		if (streq(key, "SPOOLED_ROOT") || streq(key, "KVM_PATH")) {
			if (platgrp_found)
				continue;
			isa = cp;
			cp = strchr(cp, ':');
			if (!cp) {
				/* file format error, skip file */
					(void) fclose(fp);
					*err_info =
					    alloc_sw_error(SW_INVALID_SOFTINFO);
					return (NULL);
				}
			*cp = '\0';
			if (streq(isa, platarch))
				platgrp_found = 1;
			continue;
		}

		if (streq(key, "PLATFORM_GROUP")) {
			if (platlist_inprogress)
				break;  /* prior plat list finished */
			isa = cp;
			cp = strchr(isa, ':');
			if (!cp) {
				cp = strchr(isa, '.');
				if (!cp) {
					(void) fclose(fp);
					*err_info =
					    alloc_sw_error(SW_INVALID_SOFTINFO);
					return (NULL);
				}
			}
			*cp++ = '\0';
			if (streq(isa, svc->sw_svc_isa) &&
			    streq(cp, svc->sw_svc_plat))
				platlist_inprogress = 1;
			continue;
		}

		if (streq(key, "PLATFORM_MEMBER")) {
			if (!platlist_inprogress)
				continue;
			sl = (StringList *)xcalloc((size_t)
			    sizeof(StringList));
			sl->string_ptr = xstrdup(cp);
			link_to((Item **)&head_sl, (Item *)sl);
			continue;
		}
	}
	if (!head_sl) {
		if (platgrp_found) {
			sl = (StringList *)xcalloc((size_t)
			    sizeof(StringList));
			sl->string_ptr = xstrdup(svc->sw_svc_plat);
			link_to((Item **)&head_sl, (Item *)sl);
		} else {
			*err_info = alloc_sw_error(SW_INVALID_SVC);
			return (NULL);
		}
	}
	(void) fclose(fp);
	return (head_sl);
}

/*
 * Function:	validate_service_modification
 * Description:
 * Scope:	public
 * Parameters:	modsp	- [RO, *RW]
 *		errinfo	- [RO, **RW]
 * Return:	 0	-
 *		-1	-
 */
int
validate_service_modification(SW_service_modspec *modsp,
	SW_error_info **err_info)
{
	int status;
	FSspace	**sp;

	set_rootdir("/");
	if ((status = setup_service_modification(modsp, err_info)) != 0)
		return (status);

	(void) DiskobjInitList(NULL);

	/*
	 * Set the add_service flag so teh space calculations get done
	 * correctly. This flag is checked in sp_read_pkg_map().
	 */
	set_add_service_mode(1);
	if ((sp = load_current_fs_layout()) == NULL) {
		*err_info = alloc_sw_error(SW_EXEC_FAILURE);
		return (-1); 
	}
	status = verify_fs_layout(sp, NULL, NULL);
	set_add_service_mode(0);

	if (status != SUCCESS) {
		if (status == SP_ERR_NOT_ENOUGH_SPACE) {
			*err_info = alloc_sw_error(SW_INSUFFICIENT_SPACE);
			(*err_info)->sw_space_results =
			    gen_final_space_report(sp);
		} else
			*err_info = alloc_sw_error(SW_EXEC_FAILURE);
		free_space_tab(sp);
		return (-1); 
	}

	free_space_tab(sp);
	return (0);
}

int
execute_service_modification(SW_service_modspec *modsp,
	SW_error_info **err_info)
{
	int	status, realstat;
	char	buf[MAXPATHLEN];
	char	*script, *log;
	FSspace	**sp;

	set_rootdir("/");
	if ((status = setup_service_modification(modsp, err_info)) != 0)
		return (status);

	(void) DiskobjInitList(NULL);

	/*
	 * Set the add_service flag so teh space calculations get done
	 * correctly. This flag is checked in sp_read_pkg_map().
	 */
	set_add_service_mode(1);
	if ((sp = load_current_fs_layout()) == NULL) {
		*err_info = alloc_sw_error(SW_EXEC_FAILURE);
		return (-1); 
	}
	status = verify_fs_layout(sp, NULL, NULL);
	set_add_service_mode(0);

	if (status != SUCCESS) {
		if (status == SP_ERR_NOT_ENOUGH_SPACE) {
			*err_info = alloc_sw_error(SW_INSUFFICIENT_SPACE);
			(*err_info)->sw_space_results =
			    gen_final_space_report(sp);
		} else
			*err_info = alloc_sw_error(SW_EXEC_FAILURE);
		free_space_tab(sp);
		return (-1); 
	}
	free_space_tab(sp);
	script = upgrade_script_path(get_localmedia()->sub->info.prod);
	log = upgrade_log_path(get_localmedia()->sub->info.prod);
	generate_swm_script(script);
	(void) sprintf(buf, "sh %s / > %s 2>&1\n", script, log);
	status = system(buf);
	(void) chmod(log, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	realstat = WEXITSTATUS(status);
	if (realstat != 0) {
		*err_info = (SW_error_info *)xcalloc((size_t) sizeof
		    (SW_error_info));
		(*err_info)->sw_error_code = SW_EXEC_FAILURE;
		(*err_info)->sw_exec_errcode = realstat;
		return (-1);
	}
	return (0);
}

char usrmntpnt[] = "/usr";
char kvmmntpnt[] = "/usr/kvm";
char sharemntpnt[] = "/usr/share";

#define	ALL_ALL	"all.all"

SW_createroot_info *
get_createroot_info(SW_service *svc, SW_error_info **err_info)
{
	char	 service_name[256];
	char	 softinfo_file[MAXPATHLEN];
	FILE	*fp;
	char	 buf[BUFSIZ + 1];
	char	 key[BUFSIZ];
	int	 len;
	char	*cp, *cp2, *servdir;
	char	 instarch[ARCH_LENGTH];
	char	 platarch[ARCH_LENGTH];
	SW_createroot_info	*cri;
	SW_pkgadd_def 		*pkgp = NULL;
	SW_remmnt		*remmntp = NULL;
	int	is_4X = FALSE;		/* processing 4.x softinfo ? */

	if (svc->sw_svc_os == NULL || svc->sw_svc_version == NULL ||
	    svc->sw_svc_isa == NULL || svc->sw_svc_plat == NULL) {
		*err_info = alloc_sw_error(SW_INVALID_SVC);
		return (NULL);
	}

	servdir = services_read_path("/");

	(void) sprintf(service_name, "%s_%s", svc->sw_svc_os, svc->sw_svc_version);
	(void) sprintf(instarch, "%s.all:", svc->sw_svc_isa);
	(void) sprintf(platarch, "%s.%s:", svc->sw_svc_isa, svc->sw_svc_plat);
	(void) sprintf(softinfo_file, "%s/%s", servdir, service_name);

	if ((fp = fopen(softinfo_file, "r")) == (FILE *)NULL) {
		*err_info = alloc_sw_error(SW_INVALID_SVC);
		return (NULL);
	}
	cri = (SW_createroot_info *)xcalloc((size_t) sizeof
	    (SW_createroot_info));

	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf) - 1] = '\0';
		if (buf[0] == '#' || buf[0] == '\n')
			continue;

		if ((cp = strchr(buf, '=')) == NULL)
			continue;
		len = cp - buf;
		(void) strncpy(key, buf, len);
		key[len] = '\0';
		cp++;	/* cp now points to string after '=' */

		if (streq(key, "USR_PATH")) {
			if (!strneq(cp, instarch, strlen(instarch)))
				continue;
			for (remmntp = cri->sw_root_remmnt; remmntp;
			    remmntp = remmntp->next) {
				if (streq(remmntp->sw_remmnt_mntpnt,
				    usrmntpnt))
					break;
			}

			/*
			 *  if remmntp is non-null,  a remote mount for /usr
			 *  has already been defined.  Just continue.
			 */
			if (remmntp)
				continue;

			cp = strchr(buf,':');
			if (!cp) {
				free_createroot_info(cri);
				*err_info =
				    alloc_sw_error(SW_INVALID_SOFTINFO);
				return (NULL);
			}
			remmntp = (SW_remmnt *)xcalloc((size_t) sizeof
				(SW_createroot_info));
			remmntp->sw_remmnt_mntpnt = xstrdup(usrmntpnt);
			remmntp->sw_remmnt_mntdir = xstrdup(cp + 1);

			/*
			 *  HACK here:  we should really sort the
			 *  mount points on the mount point queue, so
			 *  that any arbitrary mounts would be done in the
			 *  order.  However, since we know that the only
			 *  mounts that will appear will be /usr and
			 *  some directories under /usr (kvm, share, maybe
			 *  openwin or dt), just make sure that /usr is
			 *  first by inserting it at the beginning of the
			 *  list.
			 */

			if (cri->sw_root_remmnt)
				remmntp->next = cri->sw_root_remmnt->next;

			cri->sw_root_remmnt = remmntp;
			remmntp = NULL;
		} else if (streq(key, "KVM_PATH")) {
			if (!strneq(cp, platarch, strlen(platarch)))
				continue;
			for (remmntp = cri->sw_root_remmnt; remmntp;
			    remmntp = remmntp->next) {
				if (streq(remmntp->sw_remmnt_mntpnt,
				    kvmmntpnt))
					break;
			}

			/*
			 *  if remmntp is non-null,  a remote mount for /kvm
			 *  has already been defined.  Just continue.
			 */
			if (remmntp)
				continue;

			cp = strchr(buf,':');
			if (!cp) {
				free_createroot_info(cri);
				*err_info =
				    alloc_sw_error(SW_INVALID_SOFTINFO);
				return (NULL);
			}
			remmntp = (SW_remmnt *)xcalloc((size_t) sizeof
				(SW_createroot_info));
			remmntp->sw_remmnt_mntpnt = xstrdup(kvmmntpnt);
			remmntp->sw_remmnt_mntdir = xstrdup(cp + 1);

			/* link remote mount to end of list */
			link_to((Item **)&cri->sw_root_remmnt,
			    (Item *)remmntp);
			remmntp = NULL;
		} else if (streq(key, "SHARE_PATH")) {
			/*
			 * Get the string after the '.'. This string is in
			 * the form of <ISA|all>.all:<path>
			 */
			if ((cp = strchr(buf,'.')) == NULL)
				continue;
			cp++;	/* advance cp to string after '.' */
			
			/*
			 * Make sure we have the something with an all as
			 * the second part of the ISA.PLAT string.
			 */
			if (!strneq(cp, "all", strlen("all")))
				continue;
			for (remmntp = cri->sw_root_remmnt; remmntp;
			    remmntp = remmntp->next) {
				if (streq(remmntp->sw_remmnt_mntpnt,
				    sharemntpnt))
					break;
			}

			/*
			 *  if remmntp is non-null,  a remote mount for
			 *  /usr/share has already been defined.  Just
			 *  continue.
			 */
			if (remmntp)
				continue;

			cp = strchr(buf,':');
			if (!cp) {
				free_createroot_info(cri);
				*err_info =
				    alloc_sw_error(SW_INVALID_SOFTINFO);
				return (NULL);
			}
			remmntp = (SW_remmnt *)xcalloc((size_t) sizeof
				(SW_createroot_info));
			remmntp->sw_remmnt_mntpnt = xstrdup(sharemntpnt);
			remmntp->sw_remmnt_mntdir = xstrdup(cp + 1);

			/* link remote mount to end of list */
			link_to((Item **)&cri->sw_root_remmnt,
			    (Item *)remmntp);
			remmntp = NULL;
		} else if (streq(key, "SPOOLED_ROOT")) {
			/*
			 *  If pkgp is non-NULL, the program was expecting a
			 *  a "ROOT" keyword.  The file is munged, so report
			 *  an error and quit.
			 */
			if (pkgp) {
				free(pkgp->sw_pkg_dir);
				free(pkgp);
				free_createroot_info(cri);
				*err_info =
				    alloc_sw_error(SW_INVALID_SOFTINFO);
				return (NULL);
			}

			if (!strneq(cp, instarch, strlen(instarch)) &&
			    !strneq(cp, platarch, strlen(platarch)) &&
			    !strneq(cp, ALL_ALL, strlen(ALL_ALL)))
				continue;
			cp = strchr(buf, ':');
			if (!cp) {
				free_createroot_info(cri);
				*err_info =
				    alloc_sw_error(SW_INVALID_SOFTINFO);
				return (NULL);
			}
			pkgp = (SW_pkgadd_def *)xcalloc((size_t) sizeof
				(SW_pkgadd_def));
			pkgp->sw_pkg_dir = xstrdup(cp + 1);
			/*
			 * At this point we need to know if this is a
			 * Solaris 2.X package or a SunOS 4.x cpio
			 * archive. If it is a 4.x cpio archive we will get
			 * the size from df and set the 4.x flag.
			 */
			if (! is_pkg(pkgp->sw_pkg_dir)) {
				FILE	*pp;
				char	command[MAXPATHLEN + 20];

				is_4X = TRUE;
				
				/*
				 * Determine how much space is neede for
				 * this archive, using df
				 */
				(void) sprintf(command,
				    "/usr/bin/du -sk %s", pkgp->sw_pkg_dir);
				if ((pp = popen(command, "r")) == NULL) {
					pkgp->sw_pkg_size = 0;
				} else {
					while (!feof(pp)) {
						if (fgets(buf, BUFSIZ, pp)
						    != NULL) {
							buf[strlen(buf)-1] =
							    '\0';
							(void) sscanf(buf,
							    "%ld %*s",
		 					    &pkgp->sw_pkg_size);
						}
					}
					(void) pclose(pp);
				}
				cri->sw_root_size += pkgp->sw_pkg_size;
				
				/* link package entry to end of list */
				link_to((Item **)&cri->sw_root_packages,
				    (Item *)pkgp);
				pkgp = NULL;
			}
		} else if (streq(key, "ROOT")) {
			if (is_4X && strneq(cp, ALL_ALL, strlen(ALL_ALL)))
				continue;
			
			if (!strneq(cp, instarch, strlen(instarch)) &&
			    !strneq(cp, platarch, strlen(platarch)))
				continue;
			/*
			 *  If pkgp is NULL, this line is not following a
			 *  SPOOLED_ROOT line like it's supposed to.
			 */
			if (!pkgp) {
				free_createroot_info(cri);
				*err_info =
				    alloc_sw_error(SW_INVALID_SOFTINFO);
				return (NULL);
			}
			cp = strchr(buf, ':');
			if ((cp = strchr(buf, ':')) == NULL ||
			    (cp2 = strchr(cp, ',')) == NULL) {
				free(pkgp->sw_pkg_dir);
				free(pkgp);
				free_createroot_info(cri);
				*err_info =
				    alloc_sw_error(SW_INVALID_SOFTINFO);
				return (NULL);
			}

			*cp2++ = '\0';

			/* cp + 1 now points to the package abbreviation */
			/* cp2 now points to the size */

			pkgp->sw_pkg_name = xstrdup(cp + 1);

			cp = strchr(cp2, ',');
			if (!cp) {
				free(pkgp->sw_pkg_dir);
				free(pkgp->sw_pkg_name);
				free(pkgp);
				free_createroot_info(cri);
				*err_info =
				    alloc_sw_error(SW_INVALID_SOFTINFO);
				return (NULL);
			}
			/* cp now points to the commas after the size */
			*cp = '\0';
			pkgp->sw_pkg_size = atol(cp2);
			cri->sw_root_size += pkgp->sw_pkg_size;
			/* link package entry to end of list */
			link_to((Item **)&cri->sw_root_packages,
			    (Item *)pkgp);
			pkgp = NULL;
		}
	}
	(void) fclose(fp);
	return (cri);
}

/************************************************************************/
/*	Local support functions						*/
/************************************************************************/

static int
setup_service_modification(SW_service_modspec *modsp, SW_error_info **err_info)
{
	SW_service_mod		*msp;
	Module			*mediamod;
	int			 stat;

	for (msp = modsp->sw_svmodspec_mods; msp != NULL; msp = msp->next) {

		/*  see if media has already been loaded */
		mediamod = find_media(msp->sw_svmod_media, NULL);
		if (mediamod == NULL) {
			mediamod = add_media(msp->sw_svmod_media);
			if (mediamod == NULL ||
			    load_media(mediamod, TRUE) != SUCCESS) {
				*err_info = alloc_sw_error(SW_MEDIA_FAILURE);
				return (-1);
			}
		}
	}

	/* Now that we have a media pointer make sure its product is good. */
	if (mediamod->sub == (Module *)NULL ||
	    mediamod->sub->type == NULLPRODUCT) {
		*err_info = alloc_sw_error(SW_MEDIA_FAILURE);
		return (-1);
	}

	/*  see if installed media has already been loaded */
	mediamod = find_media("/", NULL);
	if (mediamod == NULL) {
		mediamod = load_installed("/", FALSE);
		if (mediamod == NULL) {
			*err_info = alloc_sw_error(SW_MEDIA_FAILURE);
			return (-1);
		}
	}

	for (msp = modsp->sw_svmodspec_mods; msp != NULL; msp = msp->next) {
		switch (msp->sw_svmod_action) {
		  case SW_ADD_SERVICE:
			if ((stat = do_add_service(msp, err_info)) != 0)
				return (stat);
			break;

		  default:
			*err_info = alloc_sw_error(SW_INVALID_OP);
			return (-1);
		}
	}
	return (0);
}

static int
do_add_service(SW_service_mod *msp, SW_error_info **err_info)
{
	SW_locale		*lp;
	SW_metacluster		*meta;
	Module			*mediamod, *prodmod, *metamod;
	Arch			*ap;
	char			 fullarch[256];
	int			 stat;

	/*
	 * calling routine has already guaranteed that this media
	 * is loaded.
	 */
	mediamod = find_media(msp->sw_svmod_media, NULL);

	/* find the product matching the name and version */
	for (prodmod = mediamod->sub; prodmod != NULL;
	    prodmod = prodmod->next) {
		if (streq(prodmod->info.prod->p_name,
		    msp->sw_svmod_newservice->sw_svc_os) &&
		    streq(prodmod->info.prod->p_version,
		    msp->sw_svmod_newservice->sw_svc_version))
			break;
	}
	if (prodmod == NULL) {
		*err_info = alloc_sw_error(SW_INVALID_SVC);
		return (-1);
	}


	/* verify that arch is supported */
	(void) sprintf(fullarch, "%s.%s", msp->sw_svmod_newservice->sw_svc_isa,
	    msp->sw_svmod_newservice->sw_svc_plat);
	for (ap = prodmod->info.prod->p_arches; ap != NULL; ap =ap->a_next)
		if (streq(fullarch, ap->a_arch))
			break;
	if (ap == NULL) {
		*err_info = alloc_sw_error(SW_INVALID_SVC);
		return (-1);
	}

	/* verify that metacluster is supported */
	if (msp->sw_svmod_metaclusters) {
		for (meta = msp->sw_svmod_metaclusters; meta;
		    meta = meta->next) {
			metamod = findmeta(prodmod, meta);
			if (metamod == NULL) {
				*err_info = alloc_sw_error(SW_INVALID_SVC);
				return (-1);
			}
			stat = add_service(prodmod, fullarch, metamod);
			if (stat == ERR_DIFFREV) {
				*err_info =
				    alloc_sw_error(SW_INCONSISTENT_REV);
				(*err_info)->sw_diff_rev = g_sw_diffrev;
				g_sw_diffrev = NULL;
				return (-1);
			}
		}
	} else {
		stat = add_service(prodmod, fullarch, prodmod->sub);
		if (stat == ERR_DIFFREV) {
			*err_info = alloc_sw_error(SW_INCONSISTENT_REV);
			(*err_info)->sw_diff_rev = g_sw_diffrev;
			g_sw_diffrev = NULL;
			return (-1);
		}
	}
	/* add locales */
	for (lp = msp->sw_svmod_locales; lp; lp = lp->next) {
		stat = select_locale(prodmod, lp->sw_loc_name);
	}
	return (0);
}

/*
 * Can return one of the following values:
 *
 *   0	- ok, the list of services was appended to svlist
 *  -1	- the file read was not recognized as a softinfo file
 *  -2	- the file read was recognized as a softinfo file, but contained
 *	  a syntax error.  The syntax error is detailed in the err_info
 *	  struct.
 */
static int
read_service_file(SW_service_list *svlist, char *dir, char *file,
    SW_error_info **err_info)
{
	FILE	*fp;
	char	buf[BUFSIZ + 1];
	char	path[MAXPATHLEN];
	char	fullarch[ARCH_LENGTH];
	char	svc_name[MAXPATHLEN];
	char	key[BUFSIZ];
	char	*cp, *isa, *lp, *locp;
	int	len, first, format;
	char	*os = NULL;
	char	*version = NULL;
	int	is_a_softinfo_file = 0;
	SW_service	*svc;
	SW_service_list	*tmpsvclist;
	SW_locale	*loc;

	(void) sprintf(path, "%s/%s", dir, file);
	if ((fp = fopen(path, "r")) == (FILE *)NULL)
		return (-1);

	tmpsvclist = xcalloc((size_t) sizeof (SW_service_list));
	first = 1;
	isa_handled_clear();
	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf) - 1] = '\0';
		if (buf[0] == '#' || buf[0] == '\n')
			continue;

		if ((cp = strchr(buf, '=')) == NULL)
			continue;
		len = cp - buf;
		(void) strncpy(key, buf, len);
		key[len] = '\0';
		cp++;	/* cp now points to string after '=' */

		if (first) {
			first = 0;
			if (streq(key, "FORMAT_VERSION")) {
				(void) sscanf(cp, "%d", &format);
				continue;
			} else
				format = 1;
		}

		if (streq(key, "OS")) {
			if (os != NULL || version != NULL) {
				/* file format error, skip file */
				(void) fclose(fp);
				if (tmpsvclist)
					free_service_list(tmpsvclist);
				if (is_a_softinfo_file) {
					*err_info =
					    alloc_sw_error(SW_INVALID_SOFTINFO);
					return (-2);
				}
				else return (-1);
			}
			os = xstrdup(cp);
			continue;
		}

		if (streq(key, "VERSION")) {
			if (version != NULL || os == NULL) {
				/* file format error, skip file */
				(void) fclose(fp);
				if (tmpsvclist)
					free_service_list(tmpsvclist);
				if (is_a_softinfo_file) {
					*err_info =
					    alloc_sw_error(SW_INVALID_SOFTINFO);
					return (-2);
				}
				else return (-1);
			}
			version = xstrdup(cp);
			(void) sprintf(svc_name, "%s_%s", os, version);
			if (!streq(svc_name, file)) {
				/* file format error, skip file */
				(void) fclose(fp);
				if (tmpsvclist)
					free_service_list(tmpsvclist);
				return (-1);
			}
			is_a_softinfo_file = 1;
			continue;
		}

		/*
		 *  If we haven't found the leading OS and VERSION
		 *  keywords and we find some other keyword, quit.
		 *  This isn't a valid softinfo file.
		 */
		if (!is_a_softinfo_file) {
			(void) fclose(fp);
			if (tmpsvclist)
				free_service_list(tmpsvclist);
			if (is_a_softinfo_file) {
				*err_info = alloc_sw_error(SW_INVALID_SOFTINFO);
				return (-2);
			}
			else return (-1);
		}

		if (streq(key, "SPOOLED_ROOT") || streq(key, "KVM_PATH") ||
		    streq(key, "PLATFORM_GROUP")) {
			isa = cp;
			if (streq(key, "PLATFORM_GROUP")) {
				cp = strchr(cp, ':');
				if (cp)
					*cp = '.';
			} else {
				cp = strchr(cp, ':');
				if (!cp) {
					/* file format error, skip file */
					if (tmpsvclist)
						free_service_list(tmpsvclist);
					(void) fclose(fp);
					if (is_a_softinfo_file) {
						*err_info =
						    alloc_sw_error(
						    SW_INVALID_SOFTINFO);
						return (-2);
					}
					else return (-1);
				}
				*cp = '\0';
			}
			(void) strcpy(fullarch, isa);  /* save the full arch string */
			/* now split out the isa from the karch */
			cp = strchr(isa, '.');
			if (cp == NULL)
				continue;
			*cp++ = '\0';
			/*  isa now points to inst arch.  cp points to karch */
			if (streq(cp, "all"))
				continue;
			if (isa_handled(fullarch))
				continue;
			svc = (SW_service *)xcalloc((size_t)
			    sizeof (SW_service));
			svc->sw_svc_os = xstrdup(os);
			svc->sw_svc_version = xstrdup(version);
			svc->sw_svc_isa = xstrdup(isa);
			svc->sw_svc_plat = xstrdup(cp);
			link_to((Item **)&tmpsvclist->sw_svl_services,
			    (Item *)svc);
			tmpsvclist->sw_svl_num_services++;
			continue;
		}

		if (streq(key, "LOCALE")) {
			isa = cp;
			lp = strchr(cp, ':');
			if (!lp)
				continue;  /* just ignore the line */
			*lp++ = '\0';
			(void) strcpy(fullarch, isa);  /* save the full arch string */
			/* now split out the isa from the karch */
			cp = strchr(isa, '.');
			if (cp == NULL)
				continue;
			*cp = '\0';
			/*
			 *  isa now points to inst arch.
			 *  lp points to the locale.
			 */
			loc = (SW_locale *)xcalloc((size_t)
			    sizeof (SW_locale));
			loc->sw_loc_name= xstrdup(lp);
			if ((locp = get_locale_description("/",
			    loc->sw_loc_name)) != NULL)
				loc->sw_loc_nametext = xstrdup(locp);
			else
				loc->sw_loc_nametext = NULL;
			loc->sw_loc_os = xstrdup(os);
			loc->sw_loc_ver = xstrdup(version);
			loc->sw_loc_isa = xstrdup(isa);
			link_to((Item **)&tmpsvclist->sw_svl_locales,
			    (Item *)loc);
			continue;
		}
	}
	link_to((Item **)&svlist->sw_svl_services,
	    (Item *)tmpsvclist->sw_svl_services);
	tmpsvclist->sw_svl_services = NULL;
	link_to((Item **)&svlist->sw_svl_locales,
	    (Item *)tmpsvclist->sw_svl_locales);
	tmpsvclist->sw_svl_locales = NULL;
	svlist->sw_svl_num_services += tmpsvclist->sw_svl_num_services;
	free_service_list(tmpsvclist);
	(void) fclose(fp);
	return (0);
}

static SW_error_info *
alloc_sw_error(SW_return_code code)
{
	SW_error_info	*eip;

	eip = (SW_error_info *)xcalloc((size_t) sizeof (SW_error_info));
	eip->sw_error_code = code;
	return (eip);
}

static Module *
findmeta(Module *prodmod, SW_metacluster *meta)
{
	Module *mod;

	for (mod = prodmod->sub; mod; mod = mod->next)
		if (streq(meta->sw_meta_name, mod->info.mod->m_pkgid))
			return (mod);
	return (NULL);
}

/*
 * See whether a pkginfo command issued against the spooled root
 * directory succeeds.  It if does, the directory is in package
 * format.
 *
 * return TRUE if it's in package format
 * else, return FALSE
 */
 
static int
is_pkg(char *spooled_root)
{
        char            pkginfo_cmd[MAXPATHLEN];
	 
        (void) sprintf(pkginfo_cmd,
		"/usr/bin/pkginfo  -d %s >> /dev/null 2>&1",
                spooled_root);
        if (system(pkginfo_cmd) == 0)
                return (TRUE);
        else
                return (FALSE);
}
