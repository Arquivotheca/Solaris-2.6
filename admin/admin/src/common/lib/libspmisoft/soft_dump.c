#ifndef lint
#pragma ident "@(#)soft_dump.c 1.6 96/06/25 SMI"
#endif
/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.  All rights reserved.
 */
#include "spmisoft_lib.h"
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>

/* Externals */

extern struct locmap *global_locmap;

/* Local Statics and Constants */

static FILE	*dfp = (FILE *)NULL;
static Module *	s_newproductmod;
static Module *	s_newmedia;
static char *indent[] = {
	"",
	"    ",
	"	",
	"	    ",
	"		",
	"		    ",
	"			",
	"			    ",
	"				",
	"				   ",
	"					",
	"					    ",
	"						"
};

/* Local Globals */

int	dump_level = 0;

/* Public Function Prototypes */

int		swi_dumptree(char *);

/* Library Function Prototypes */

static int	open_dump(char *);
static void	close_dump(void);
static void	dumpmod(Module *);
static void	dump_fields(Module *);
static void	dump_modinfo(Modinfo *);
static void	dump_modinfo_fields(Modinfo *);
static void	dump_prodinfo(Product *);
static void	dump_locale(Locale *);
static void	dump_mediainfo(Media *);
static void	dump_depends(Depend *dp);
static void	dump_list(List *list);

/* Local Function Prototypes */

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * dumptree()
 *
 * Parameters:
 *	filename	- name of file into which the data should be
 *			  dumped
 * Return:
 * Status:
 *	public
 */
int
swi_dumptree(char * filename)
{
	int status;
	Module *mod;
	Module *savedview;

	s_newmedia = NULL;
	s_newproductmod = NULL;

	if ((status = open_dump(filename)) != 0)
		return (status);

	/* find the new product media */
	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type != INSTALLED_SVC &&
		    mod->info.media->med_type != INSTALLED &&
		    mod->sub->type == PRODUCT &&
		    strcmp(mod->sub->info.prod->p_name, "Solaris") == 0)
			s_newmedia = mod;
	}
	if (s_newmedia)
		s_newproductmod = s_newmedia->sub;

	/* save the media at the time dump was called */
	if (s_newproductmod)
		savedview = get_current_view(s_newproductmod);

	/* dump the default view of the new media */
	if (s_newproductmod) {
		load_default_view(s_newproductmod);
		(void) fprintf(dfp, "Dumping default view of new media.\n");
		dumpmod(s_newmedia);
	}

	/* for each installed media, dump it and its view of the new media */
	mod = get_media_head();
	while (mod != NULL) {
		if (mod->info.media->med_type == INSTALLED_SVC ||
		    mod->info.media->med_type == INSTALLED) {
			(void) fprintf(dfp, "dumping local environment:\n");
			dumpmod(mod);
			if (s_newproductmod &&
			    has_view(s_newproductmod, mod) == SUCCESS) {
				(void) fprintf(dfp,
				    "dump VIEW from media at %s:\n",
				    mod->info.media->med_dir);
				(void) load_view(s_newproductmod, mod);
				dumpmod(s_newmedia);
			}
		}
		mod = mod->next;
	}
	/*
	 * restore view that was loaded when this function was
	 * started.
	 */
	if (s_newproductmod) {
		if (savedview != NULL)
			(void) load_view(s_newproductmod, savedview);
		else
			load_default_view(s_newproductmod);
	}
	close_dump();
	return (0);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * open_dump()
 *	Open the dumpfile 'filename' (for write) into which tree dump data
 *	will later be placed
 * Parameters:
 *	filename	-
 * Return:
 *	0	- open successful
 *	1	- open failed
 * Status:
 *	public
 */
static int
open_dump(char * filename)
{
	dfp = fopen(filename, "w");
	if (dfp == NULL) {
		(void) printf("Couldn't open dump file\n");
		return (1);
	}
	return (0);
}

/*
 * close_dump()
 *	CLose the dump file pointer
 * Parameters:
 *	none
 * Return:
 *	none
 * Status:
 *	public
 */
static void
close_dump(void)
{
	if (dfp != (FILE *)NULL) {
		(void) fclose(dfp);
		dfp = (FILE *)NULL;
	}
	return;
}

/*
 * dumpmod()
 * Parameters:
 *	mod	- pointer to module to be dumped
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
static void
dumpmod(Module * mod)
{
	Module *child;

	dump_fields(mod);
	dump_level++;
	child = mod->sub;
	while (child) {
		dumpmod(child);
		child = child->next;
	}
	dump_level--;
}

/*
 * dump_fields()
 * Parameters:
 *	mod	-
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
static void
dump_fields(Module * mod)
{
	(void) fprintf(dfp, "%sMODULE: type = ", indent[dump_level]);
	switch (mod->type)
	{
	case (PACKAGE):		(void) fprintf(dfp, "PACKAGE\n"); break;
	case (MODULE): 		(void) fprintf(dfp, "MODULE\n"); break;
	case (PRODUCT): 	(void) fprintf(dfp, "PRODUCT\n"); break;
	case (MEDIA): 		(void) fprintf(dfp, "MEDIA\n"); break;
	case (CLUSTER): 	(void) fprintf(dfp, "CLUSTER\n"); break;
	case (METACLUSTER):	(void) fprintf(dfp, "METACLUSTER\n"); break;
	case (NULLPRODUCT):	(void) fprintf(dfp, "NULLPRODUCT\n"); break;
	case (CATEGORY):	(void) fprintf(dfp, "CATEGORY\n"); break;
	case (LOCALE): 		(void) fprintf(dfp, "LOCALE\n"); break;
	case (UNBUNDLED_4X): 	(void) fprintf(dfp, "UNBUNDLED_4X\n"); break;
	}
#ifdef PRINTMODULE
	(void) fprintf(dfp, "%snext = %x\n", indent[dump_level], mod->next);
	(void) fprintf(dfp, "%sprev = %x\n", indent[dump_level], mod->prev);
	(void) fprintf(dfp, "%ssub = %x\n", indent[dump_level], mod->sub);
	(void) fprintf(dfp, "%shead = %x\n", indent[dump_level], mod->head);
	(void) fprintf(dfp, "%sparent = %x\n", indent[dump_level], mod->parent);
	(void) fprintf(dfp, "%sMODINFO:\n", indent[dump_level]);
#endif
	switch (mod->type) {
		case (PACKAGE):
			dump_modinfo(mod->info.mod);
			break;
		case (MODULE):
			dump_modinfo(mod->info.mod);
			break;
		case (PRODUCT):
			dump_prodinfo(mod->info.prod);
			break;
		case (MEDIA):
			dump_mediainfo(mod->info.media);
			break;
		case (CLUSTER):
			dump_modinfo(mod->info.mod);
			break;
		case (METACLUSTER):
			dump_modinfo(mod->info.mod);
			break;
		case (NULLPRODUCT):
			dump_prodinfo(mod->info.prod);
			break;
		case (CATEGORY):
			(void) fprintf(dfp,
				"Don't know how to dump CATEGORY.\n");
			break;
		case (LOCALE):
			dump_locale(mod->info.locale);
			break;
		case (UNBUNDLED_4X):
			(void) fprintf(dfp,
				"Don't know how to dump UNBUNDLED_4X.\n");
			break;
	}
}

/*
 * dump_modinfo()
 * Parameters:
 *	mi	-
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
static void
dump_modinfo(Modinfo * mi)
{
	Node	*node;
	Modinfo	*mip;	/* The patch modinfo */

	dump_modinfo_fields(mi);
	if (mi->m_next_patch != NULL) {
		fprintf(dfp, "%sPATCH INSTANCES:\n", indent[dump_level]);
		dump_level++;
		mip = mi;
		while ((node = mip->m_next_patch) != NULL) {
			mip = (Modinfo *)(node->data);
			dump_modinfo_fields(mip);
		}
		dump_level--;
	}
	if (mi->m_instances != NULL) {
		fprintf(dfp, "%sINSTANCES:\n", indent[dump_level]);
		dump_level++;
		while ((node = mi->m_instances) != NULL) {
			mi = (Modinfo *)(node->data);
			dump_modinfo_fields(mi);
			if (mi->m_next_patch != NULL) {
				fprintf(dfp, "%sPATCH INSTANCES:\n",
				    indent[dump_level]);
				dump_level++;
				mip = mi;
				while ((node = mip->m_next_patch) != NULL) {
					mip = (Modinfo *)(node->data);
					dump_modinfo_fields(mip);
				}
				dump_level--;
			}
		}
		dump_level--;
	}
	return;
}

/*
 * dump_modinfo_fields()
 * Parameters:
 *	mi	-
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
static void
dump_modinfo_fields(Modinfo * mi)
{
	struct filediff *filep;
	char deleted_file_buf[512];
	StringList *str;

	(void) fprintf(dfp, "%sstatus=", indent[dump_level]);
	switch (mi->m_status)
	{
	case (UNSELECTED):	(void) fprintf(dfp, "UNSELECTED"); break;
	case (SELECTED):	(void) fprintf(dfp, "SELECTED"); break;
	case (PARTIALLY_SELECTED):
				(void) fprintf(dfp, "PARTIALLY_SELECTED");
				break;
	case (LOADED):		(void) fprintf(dfp, "LOADED"); break;
	case (REQUIRED):	(void) fprintf(dfp, "REQUIRED"); break;
	case (INSTALL_SUCCESS):	(void) fprintf(dfp, "INSTALL_SUCCESS"); break;
	case (INSTALL_FAILED):	(void) fprintf(dfp, "INSTALL_FAILED"); break;
	}

	(void) fprintf(dfp, " share=");
	switch (mi->m_shared)
	{
	case (NOTDUPLICATE):	(void) fprintf(dfp, "NOTDUPLICATE"); break;
	case (DUPLICATE):	(void) fprintf(dfp, "DUPLICATE"); break;
	case (NULLPKG):		(void) fprintf(dfp, "NULLPKG"); break;
	case (SPOOLED_DUP):	(void) fprintf(dfp, "SPOOLED_DUP"); break;
	case (SPOOLED_NOTDUP):	(void) fprintf(dfp, "SPOOLED_NOTDUP"); break;
	}

	(void) fprintf(dfp, " m_action=");
	switch (mi->m_action)
	{
	case (NO_ACTION_DEFINED):
			(void) fprintf(dfp, "NO_ACTION_DEFINED\n"); break;
	case (TO_BE_PRESERVED):
			(void) fprintf(dfp, "TO_BE_PRESERVED\n"); break;
	case (TO_BE_REPLACED):
			(void) fprintf(dfp, "TO_BE_REPLACED\n"); break;
	case (TO_BE_REMOVED):
			(void) fprintf(dfp, "TO_BE_REMOVED\n"); break;
	case (TO_BE_PKGADDED):
			(void) fprintf(dfp, "TO_BE_PKGADDED\n"); break;
	case (TO_BE_SPOOLED):
			(void) fprintf(dfp, "TO_BE_SPOOLED\n"); break;
	case (EXISTING_NO_ACTION):
			(void) fprintf(dfp, "EXISTING_NO_ACTION\n"); break;
	case (ADDED_BY_SHARED_ENV):
			(void) fprintf(dfp, "ADDED_BY_SHARED_ENV\n"); break;
	case (CANNOT_BE_ADDED_TO_ENV):
			(void) fprintf(dfp, "CANNOT_BE_ADDED_TO_ENV\n"); break;
	}

	(void) fprintf(dfp, "%sm_sunw_ptype = ", indent[dump_level]);
	switch (mi->m_sunw_ptype)
	{
	case (0): 		(void) fprintf(dfp, "\n"); break;
	case (PTYPE_ROOT):	(void) fprintf(dfp, "PTYPE_ROOT\n"); break;
	case (PTYPE_USR):	(void) fprintf(dfp, "PTYPE_USR\n"); break;
	case (PTYPE_KVM):	(void) fprintf(dfp, "PTYPE_KVM\n"); break;
	case (PTYPE_OW):	(void) fprintf(dfp, "PTYPE_OW\n"); break;
	case (PTYPE_UNKNOWN):	(void) fprintf(dfp, "PTYPE_UNKNOWN\n"); break;
	default: 		(void) fprintf(dfp, "illegal value\n"); break;
	}
	if (mi->m_pkgid)
		(void) fprintf(dfp,
			"%sm_pkgid = %s\n", indent[dump_level], mi->m_pkgid);
	if (mi->m_pkginst)
		(void) fprintf(dfp,
		    "%sm_pkginst = %s\n", indent[dump_level], mi->m_pkginst);
	if (mi->m_pkg_dir)
		(void) fprintf(dfp, "%sm_pkg_dir = %s\n", indent[dump_level],
		    mi->m_pkg_dir);
	if (mi->m_basedir)
		(void) fprintf(dfp,
		    "%sm_basedir = %s\n", indent[dump_level], mi->m_basedir);
	if (mi->m_instdir)
		(void) fprintf(dfp,
		    "%sm_instdir = %s\n", indent[dump_level], mi->m_instdir);
	if (mi->m_arch)
		(void) fprintf(dfp,
		    "%sm_arch = %s\n", indent[dump_level], mi->m_arch);
	if (mi->m_expand_arch)
		(void) fprintf(dfp,
		    "%sm_expand_arch = %s\n", indent[dump_level],
		    mi->m_expand_arch);
	if (mi->m_category)
		(void) fprintf(dfp,
		    "%sm_category = %s\n", indent[dump_level], mi->m_category);
	if (mi->m_version)
		(void) fprintf(dfp,
		    "%sm_version = %s\n", indent[dump_level], mi->m_version);
	if (mi->m_patchid)
		(void) fprintf(dfp, "%sm_patchid = %s\n",
		    indent[dump_level], mi->m_patchid);
	if (mi->m_flags) {
		(void) fprintf(dfp, "%sm_flags = ", indent[dump_level]);
		if (mi->m_flags & PART_OF_CLUSTER)
			(void) fprintf(dfp, " PART_OF_CLUSTER");
		if (mi->m_flags & INSTANCE_ALREADY_PRESENT)
			(void) fprintf(dfp, " INSTANCE_ALREADY_PRESENT");
		if (mi->m_flags & DO_PKGRM)
			(void) fprintf(dfp, " DO_PKGRM");
		(void) fprintf(dfp, "\n");
	}
	if (mi->m_refcnt != 0)
		(void) fprintf(dfp,
			"%sm_refcnt = %d\n", indent[dump_level], mi->m_refcnt);
	if (mi->m_locale)
		(void) fprintf(dfp,
			"%sm_locale = %s\n", indent[dump_level], mi->m_locale);
	if (mi->m_loc_strlist) {
		(void) fprintf(dfp, "%sm_loc_strlist =", indent[dump_level]);
		for (str = mi->m_loc_strlist; str; str = str->next)
			(void) fprintf(dfp, " %s", str->string_ptr);
		(void) fprintf(dfp, "\n");
	}
	if (mi->m_l10n_pkglist)
		(void) fprintf(dfp,
			"%sm_l10n_pkglist = %s\n", indent[dump_level],
			mi->m_l10n_pkglist);
	if (mi->m_l10n) {
		L10N *l10np;

		for (l10np = mi->m_l10n; l10np != NULL;
						l10np = l10np->l10n_next)
			(void) fprintf(dfp, "%sm_l10n = %s, %s, %s\n",
				indent[dump_level],
				l10np->l10n_package->m_pkgid,
				l10np->l10n_package->m_arch,
				l10np->l10n_package->m_version);
	}
	if (mi->m_pdepends) {
		(void) fprintf(dfp, "%sm_pdepends:\n", indent[dump_level]);
		dump_depends(mi->m_pdepends);
	}
	if (mi->m_idepends) {
		(void) fprintf(dfp, "%sm_idepends:\n", indent[dump_level]);
		dump_depends(mi->m_idepends);
	}
	if (mi->m_rdepends) {
		(void) fprintf(dfp, "%sm_rdepends:\n", indent[dump_level]);
		dump_depends(mi->m_rdepends);
	}
	if (mi->m_pkg_hist) {
		(void) fprintf(dfp, "%sm_pkg_hist:\n", indent[dump_level]);
		dump_level++;
		if (mi->m_pkg_hist->replaced_by)
			(void) fprintf(dfp, "%sreplaced_by = %s\n",
					indent[dump_level],
					mi->m_pkg_hist->replaced_by);
		if (mi->m_pkg_hist->deleted_files) {
			(void) strncpy(deleted_file_buf,
			    mi->m_pkg_hist->deleted_files, 511);
			deleted_file_buf[511] = '\0';
			(void) fprintf(dfp, "%sdeleted_files = %s\n",
					indent[dump_level],
					deleted_file_buf);
		}
		if (mi->m_pkg_hist->cluster_rm_list)
			(void) fprintf(dfp, "%scluster_rm_list = %s\n",
					indent[dump_level],
					mi->m_pkg_hist->cluster_rm_list);
		(void) fprintf(dfp, "%sto_be_removed = %d\n",
					indent[dump_level],
					mi->m_pkg_hist->to_be_removed);
		(void) fprintf(dfp, "%sneeds_pkgrm = %d\n",
					indent[dump_level],
					mi->m_pkg_hist->needs_pkgrm);
		dump_level--;
	}
	if (mi->m_deflt_fs[ROOT_FS] != 0)
		(void) fprintf(dfp, "%ssize(root) = %ld\n", indent[dump_level],
		    mi->m_deflt_fs[ROOT_FS]);
	if (mi->m_deflt_fs[USR_FS] != 0)
		(void) fprintf(dfp, "%ssize(usr) = %ld\n", indent[dump_level],
		    mi->m_deflt_fs[USR_FS]);
	if (mi->m_deflt_fs[USR_OWN_FS] != 0)
		(void) fprintf(dfp, "%ssize(ow) = %ld\n", indent[dump_level],
		    mi->m_deflt_fs[USR_OWN_FS]);
	if (mi->m_deflt_fs[OPT_FS] != 0)
		(void) fprintf(dfp, "%ssize(opt) = %ld\n", indent[dump_level],
		    mi->m_deflt_fs[OPT_FS]);
	if (mi->m_deflt_fs[SWAP_FS] != 0)
		(void) fprintf(dfp, "%ssize(swap) = %ld\n", indent[dump_level],
		    mi->m_deflt_fs[SWAP_FS]);
	if (mi->m_deflt_fs[VAR_FS] != 0)
		(void) fprintf(dfp, "%ssize(var) = %ld\n", indent[dump_level],
		    mi->m_deflt_fs[VAR_FS]);
	if (mi->m_deflt_fs[EXP_EXEC_FS] != 0)
		(void) fprintf(dfp, "%ssize(/export/exec) = %ld\n",
		    indent[dump_level], mi->m_deflt_fs[EXP_EXEC_FS]);
	if (mi->m_deflt_fs[EXP_SWAP_FS] != 0)
		(void) fprintf(dfp, "%ssize(/export/swap) = %ld\n",
		    indent[dump_level], mi->m_deflt_fs[EXP_SWAP_FS]);
	if (mi->m_deflt_fs[EXP_ROOT_FS] != 0)
		(void) fprintf(dfp, "%ssize(/export/root) = %ld\n",
		    indent[dump_level], mi->m_deflt_fs[EXP_ROOT_FS]);
	if (mi->m_deflt_fs[EXP_HOME_FS] != 0)
		(void) fprintf(dfp, "%ssize(/export/home) = %ld\n",
		    indent[dump_level], mi->m_deflt_fs[EXP_HOME_FS]);
	if (mi->m_deflt_fs[EXPORT_FS] != 0)
		(void) fprintf(dfp, "%ssize(/export) = %ld\n",
		    indent[dump_level], mi->m_deflt_fs[EXPORT_FS]);
	(void) fprintf(dfp, "%ssize(spooled) = %ld\n", indent[dump_level],
		    mi->m_spooled_size);

	if (mi->m_pkgovhd_size != 0)
		(void) fprintf(dfp, "%ssize(pkg ovhd) = %ld\n",
		    indent[dump_level], mi->m_pkgovhd_size);

	if (mi->m_filediff) {
		(void) fprintf(dfp, "%sModified component list: \n",
		    indent[dump_level]); filep = mi->m_filediff;
		while (filep) {
			(void) fprintf(dfp, "%s%s\n", indent[dump_level],
			    filep->component_path);
			(void) fprintf(dfp, "%sclass = %s, flags = %x\n",
			    indent[dump_level],
			    filep->pkgclass, filep->diff_flags);
			if (filep->linkptr)
				(void) fprintf(dfp, "%slink = %s\n",
				    indent[dump_level], filep->linkptr);

			filep = filep->diff_next;
		}
	}
	if (mi->m_newarch_patches) {
		struct patch_num	*pnum;

		(void) fprintf(dfp, "%sNew Architecture Patches:\n",
		    indent[dump_level]);
		for (pnum = mi->m_newarch_patches; pnum != NULL;
		    pnum = pnum->next)
			(void) fprintf(dfp, "%s   %s-%s (numeric rev = %d)\n",
			    indent[dump_level], pnum->patch_num_id,
			    pnum->patch_num_rev_string,
			    pnum->patch_num_rev);
	}
	(void) fprintf(dfp, "\n");
	return;
}

/*
 * dump_prodinfo()
 *
 * Parameters:
 *	prod	- pointer to product info structure to be dumped
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
static void
dump_prodinfo(Product * prod)
{
	Arch  	*ap;
	Module  *mod;
	Node	*node;
	Modinfo	*mi;
	struct patch *p;
	struct patchpkg *ppkg;
	PlatGroup	*pltgrp;
	Platform	*plat;

	if (prod->p_name)
		(void) fprintf(dfp,
				"%sp_name = %s\n",
				indent[dump_level], prod->p_name);
	if (prod->p_version)
		(void) fprintf(dfp,
				"%sp_version = %s\n",
				indent[dump_level], prod->p_version);
	if (prod->p_rev)
		(void) fprintf(dfp,
				"%sp_rev = %s\n",
				indent[dump_level], prod->p_rev);
	if (prod->p_pkgdir)
		(void) fprintf(dfp,
				"%sp_pkgdir = %s\n",
				indent[dump_level], prod->p_pkgdir);
	if (prod->p_rootdir)
		(void) fprintf(dfp, "%sp_rootdir = %s\n",
				indent[dump_level], prod->p_rootdir);

	for (ap = prod->p_arches; ap != NULL; ap = ap->a_next) {
		(void) fprintf(dfp, "%sarch = %s, selected = %s, loaded = %s\n",
				indent[dump_level],
				ap->a_arch, (ap->a_selected ? "YES" : "NO"),
				(ap->a_loaded ? "YES" : "NO"));
	}

	for (pltgrp = prod->p_platgrp; pltgrp != NULL; pltgrp = pltgrp->next) {
		(void) fprintf(dfp, "%splatgrp = %s, isa = %s, exported = %s\n",
				indent[dump_level],
				pltgrp->pltgrp_name, pltgrp->pltgrp_isa,
				(pltgrp->pltgrp_export ? "YES" : "NO"));
		dump_level++;
		for (plat = pltgrp->pltgrp_members; plat != NULL;
				    plat = plat->next)
			(void) fprintf(dfp, "%splatform = %s\n",
					indent[dump_level], plat->plat_name);
		dump_level--;
	}

	for (mod = prod->p_locale; mod != NULL; mod = mod->next)
		dumpmod(mod);

	if (prod->p_orphan_patch != NULL) {
		(void) fprintf(dfp, "%sORPHAN PATCHES:\n", indent[dump_level]);
		dump_level++;
		node = prod->p_orphan_patch;
		while (node != NULL) {
			mi = (Modinfo *)(node->data);
			dump_modinfo_fields(mi);
			node = mi->m_next_patch;
		}
		dump_level--;
	}

	if (prod->p_patches) {
		(void) fprintf(dfp, "%sPATCHES:\n", indent[dump_level]);
		dump_level++;
		for (p = prod->p_patches; p != NULL; p = p->next) {
			(void) fprintf(dfp, "%sPatchid = %s, removed = %s\n",
				indent[dump_level],
				p->patchid, (p->removed ? "YES" : "NO"));
			for (ppkg = p->patchpkgs; ppkg != NULL;
						  ppkg = ppkg->next) {
				if (ppkg->pkgmod->m_pkginst != NULL)
					(void) fprintf(dfp,
					    "%sPatchPkgInstance = %s\n",
					    indent[dump_level],
					    ppkg->pkgmod->m_pkginst);
				else {
					if (ppkg->pkgmod->m_shared ==
					    SPOOLED_NOTDUP &&
					    ppkg->pkgmod->m_pkgid != NULL)
						(void) fprintf(dfp,
						    "%sPatchPkgInstance = "
						    "SPOOLED %s\n",
						    indent[dump_level],
						    ppkg->pkgmod->m_pkgid);
					else
						(void) fprintf(dfp,
						    "%sPatchPkgInstance = NONE\n",
						    indent[dump_level]);
				}
			}
		}
		dump_level--;
	}

	if (prod->p_clusters) {
		(void) fprintf(dfp, "%sp_clusters:\n", indent[dump_level]);
		dump_list(prod->p_clusters);
	}

	if (prod->p_packages) {
		(void) fprintf(dfp, "%sp_packages:\n", indent[dump_level]);
		dump_list(prod->p_packages);
	}

	(void) fprintf(dfp, "\n");
	return;
}

/*
 * dump_locale()
 *
 * Parameters:
 *	locale	- pointer to locale structure to be dumped
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
static void
dump_locale(Locale * locale)
{

	if (locale->l_locale)
		(void) fprintf(dfp,
				"%sl_locale = %s\n",
				indent[dump_level], locale->l_locale);
	if (locale->l_language)
		(void) fprintf(dfp, "%sl_language = %s\n",
				indent[dump_level], locale->l_language);
	(void) fprintf(dfp, "%sselected = %s\n",
			indent[dump_level],
			(locale->l_selected ? "YES" : "NO"));
	return;
}

/*
 * dump_mediainfo()
 * Parameters:
 *	med	- pointer to media structure to be dumped
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
static void
dump_mediainfo(Media * med)
{
	LocMap	*lmap;
	StringList *str;

	(void) fprintf(dfp, "%smed_type = ", indent[dump_level]);

	switch (med->med_type)
	{
	case (ANYTYPE):		(void) fprintf(dfp, "ANYTYPE\n"); break;
	case (CDROM):		(void) fprintf(dfp, "CDROM\n"); break;
	case (MOUNTED_FS):	(void) fprintf(dfp, "MOUNTED_FS\n"); break;
	case (TAPE):		(void) fprintf(dfp, "TAPE\n"); break;
	case (FLOPPY):		(void) fprintf(dfp, "FLOPPY\n"); break;
	case (REMOVABLE):	(void) fprintf(dfp, "REMOVABLE\n"); break;
	case (INSTALLED):	(void) fprintf(dfp, "INSTALLED\n"); break;
	case (INSTALLED_SVC):	(void) fprintf(dfp, "INSTALLED_SVC\n"); break;
	case (ENDMTYPE):	(void) fprintf(dfp, "ENDMTYPE\n"); break;
	}

	(void) fprintf(dfp, "%smed_machine = ", indent[dump_level]);
	switch (med->med_machine)
	{
	case (MT_STANDALONE):	(void) fprintf(dfp, "MT_STANDALONE\n"); break;
	case (MT_SERVER):	(void) fprintf(dfp, "MT_SERVER\n"); break;
	case (MT_DATALESS):	(void) fprintf(dfp, "MT_DATALESS\n"); break;
	case (MT_SERVICE):	(void) fprintf(dfp, "MT_SERVICE\n"); break;
	}

	if (med->med_device)
		(void) fprintf(dfp, "%smed_device = %s\n",
				indent[dump_level], med->med_device);
	if (med->med_dir)
		(void) fprintf(dfp, "%smed_dir = %s\n",
				indent[dump_level], med->med_dir);
	if (med->med_volume)
		(void) fprintf(dfp, "%smed_volume = %s\n",
				indent[dump_level], med->med_volume);

	if (med->med_flags) {
		(void) fprintf(dfp, "%smed_flags = ", indent[dump_level]);
		if (med->med_flags & NEW_SERVICE)
			(void) fprintf(dfp, " NEW_SERVICE");
		if (med->med_flags & BUILT_FROM_UPGRADE)
			(void) fprintf(dfp, " BUILT_FROM_UPGRADE");
		if (med->med_flags & BASIS_OF_UPGRADE)
			(void) fprintf(dfp, " BASIS_OF_UPGRADE");
		if (med->med_flags & SVC_TO_BE_REMOVED)
			(void) fprintf(dfp, " SVC_TO_BE_REMOVED");
		if (med->med_flags & SPLIT_FROM_SERVER)
			(void) fprintf(dfp, " SPLIT_FROM_SERVER");
		if (med->med_flags & MODIFIED_FILES_FOUND)
			(void) fprintf(dfp, " MODIFIED_FILES_FOUND");
		(void) fprintf(dfp, "\n");
	}

	if (global_locmap) {
		(void) fprintf(dfp, "%sLOCMAPs\n", indent[dump_level]);
		dump_level++;
		for (lmap = global_locmap; lmap; lmap = lmap->next) {
			(void) fprintf(dfp, "\n%slocmap_partial = %s\n",
			    indent[dump_level], lmap->locmap_partial);
			if (lmap->locmap_description)
				(void) fprintf(dfp,
				    "%slocmap_description = %s\n",
				    indent[dump_level],
				    lmap->locmap_description);
			if (lmap->locmap_base) {
				(void) fprintf(dfp, "%slocmap_base = ",
				    indent[dump_level]);
				for (str = lmap->locmap_base; str;
				    str = str->next)
					(void) fprintf(dfp, " %s",
					    str->string_ptr);
				(void) fprintf(dfp, "\n");
			}
		}
		dump_level--;
	}

	if (med->med_hostname) {
		StringList	*host;
		host = med->med_hostname;
		(void) fprintf(dfp, "%smed_hostname = ", indent[dump_level]);
		while (host) {
			(void) fprintf(dfp, "%s, ", host->string_ptr);
			host = host->next;
		}
		(void) fprintf(dfp, "\n");
	}

	(void) fprintf(dfp, "\n");
	return;
}

static void
dump_depends(Depend *dp)
{
	dump_level++;
	while (dp) {
		(void) fprintf(dfp, "%sDEPENDENCY:\n", indent[dump_level]);
		if (dp->d_pkgid)
			(void) fprintf(dfp, "%sd_pkgid = %s\n",
			    indent[dump_level], dp->d_pkgid);
		if (dp->d_pkgidb)
			(void) fprintf(dfp, "%sd_pkgidb = %s\n",
			    indent[dump_level], dp->d_pkgidb);
		if (dp->d_version)
			(void) fprintf(dfp, "%sd_version = %s\n",
			    indent[dump_level], dp->d_version);
		if (dp->d_arch)
			(void) fprintf(dfp, "%sd_arch = %s\n",
			    indent[dump_level], dp->d_arch);
		dp = dp->d_next;
	}
	dump_level--;
}

static void
dump_list(List *list)
{
	Node	*head, *p;
	int	count = 0;

	dump_level++;
	head = list->list;

	for (p = head->next; p != head; p = p->next) {
		if (count == 0)
			(void) fprintf(dfp, "%s", indent[dump_level]);
		(void) fprintf(dfp, " %s", p->key ? p->key : "()");
		if (++count == 5) {
			count = 0;
			(void) fprintf(dfp, "\n");
		}
	}
	if (count != 0)
		(void) fprintf(dfp, "\n");

	dump_level--;
}
