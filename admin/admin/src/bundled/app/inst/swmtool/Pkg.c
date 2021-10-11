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
#ident	"@(#)Pkg.c 1.9 94/07/21"
#endif

#include "defs.h"
#include "ui.h"
#include <sys/param.h>
#include <xview/textsw.h>
#include <group.h>
#include "Pkg_ui.h"

extern 	Pkg_PkgWin_objects *Pkg_PkgWin;
extern 	Pkg_ProdWin_objects *Pkg_ProdWin;

static	void set_basedir(Module *, char *, char *);
static	char *get_product_name(Module *);

void
GetPackageInfo(Module *mod)
{
	Pkg_PkgWin_objects *ip = Pkg_PkgWin;
	Modinfo	*modinfo;
	int	i;
	u_long	sizes[FS_MAX];
	char	strings[FS_MAX][20];
	char	instbuf[1024];
	char	*date;
	SWM_mode mode = get_mode();
	static	char *date_unknown;
	Space	**sp;

	if (date_unknown == (char *)0)
		date_unknown = xstrdup(gettext("date unknown"));

	if ((Module *)xv_get(ip->PkgCtrl, PANEL_CLIENT_DATA) == mod)
		return;

	xv_set(ip->PkgCtrl, PANEL_CLIENT_DATA, mod, NULL);

	if (mod == 0) {
		SET_VALUE(ip->PkgName, "");
		SET_VALUE(ip->PkgProd, "");
		SET_VALUE(ip->PkgAbbrev, "");
		SET_VALUE(ip->PkgVend, "");
		SET_VALUE(ip->PkgVers, "");
		SET_VALUE(ip->PkgDesc, "");
		SET_VALUE(ip->PkgArch, "");
		SET_VALUE(ip->PkgStatus, "");
		SET_VALUE(ip->PkgRoot, "");
		SET_VALUE(ip->PkgUsr, "");
		SET_VALUE(ip->PkgOpt, "");
		SET_VALUE(ip->PkgVar, "");
		SET_VALUE(ip->PkgExport, "");
		SET_VALUE(ip->PkgOpenwin, "");
		xv_set(ip->PkgBase,
			PANEL_VALUE,	"",
			PANEL_INACTIVE,	TRUE,
			NULL);
		xv_set(ip->PkgApply, PANEL_INACTIVE, TRUE, NULL);
		xv_set(ip->PkgReset, PANEL_INACTIVE, TRUE, NULL);
		return;
	}

	if (mod->type == PACKAGE ||
	    mod->type == MODULE ||
	    mod->type == CLUSTER ||
	    mod->type == METACLUSTER ||
	    mod->type == UNBUNDLED_4X) {
		char	vbuf[1024];
		char	abuf[1024];
		char	*description;
		char	*vendor;
		char	*name;
		char	*arch;

		name = get_product_name(mod);
		SET_VALUE(ip->PkgProd, name);
		SET_VALUE(ip->PkgAbbrev, mod->info.mod->m_pkgid);
		/*
		 * The first instance can be a NULLPKG so we
		 * have to be careful about what we dereference.
		 * We may have to look past the first NULLPKG to
		 * find several of the strings.  The arch and
		 * version strings are compound strings built
		 * from the union of all the non-NULLPKG values.
		 */
		name = (char *)0;
		description = (char *)0;
		vendor = (char *)0;
		vbuf[0] = '\0';
		abuf[0] = '\0';
		for (modinfo = mod->info.mod;
		    modinfo != (Modinfo *)0;
		    modinfo = next_inst(modinfo)) {
			if (modinfo->m_shared == NULLPKG)
				continue;
			if (name == (char *)0)
				name = modinfo->m_name;
			if (description == (char *)0)
				description = modinfo->m_desc;
			if (vendor == (char *)0)
				vendor = modinfo->m_vendor;
			if (modinfo->m_version != (char *)0) {
				(void) sprintf(&vbuf[strlen(vbuf)],
				    "%s%.*s",
					vbuf[0] ? ", " : "",
					sizeof (vbuf) - (strlen(vbuf) + 2),
					modinfo->m_version);
			}
			if (modinfo->m_expand_arch != (char *)0)
				arch = modinfo->m_expand_arch;
			else
				arch = modinfo->m_arch;
			if (arch != (char *)0) {
				(void) sprintf(&abuf[strlen(abuf)],
				    "%s%.*s",
					abuf[0] ? ", " : "",
					sizeof (abuf) - (strlen(abuf) + 2),
					arch);
			}
		}
		vbuf[sizeof (vbuf) - 1] = '\0';
		abuf[sizeof (abuf) - 1] = '\0';

		SET_VALUE(ip->PkgName, name);
		SET_VALUE(ip->PkgDesc, description);
		SET_VALUE(ip->PkgVend, vendor);
		SET_VALUE(ip->PkgVers, vbuf);
		SET_VALUE(ip->PkgArch, abuf);
	} else if (mod->type == PRODUCT) {
		modinfo = (Modinfo *)0;

		SET_VALUE(ip->PkgName, get_full_name(mod));
		SET_VALUE(ip->PkgProd, get_short_name(mod));
		SET_VALUE(ip->PkgAbbrev, "");
		SET_VALUE(ip->PkgVend, "");
		SET_VALUE(ip->PkgVers, mod->info.prod->p_version);
		SET_VALUE(ip->PkgDesc, "");
		SET_VALUE(ip->PkgArch, "");
	}
	ResetPackageInfo((Xv_opaque)ip);

	if (mode == MODE_INSTALL) {
		ModStatus	status;

		if (mod->type == PRODUCT || mod->type == NULLPRODUCT)
			status = mod->info.prod->p_status;
		else
			status = mod->info.mod->m_status;

		if (status == SELECTED)
			(void) sprintf(instbuf, "%.*s",
			    sizeof (instbuf),
			    gettext("Selected for installation"));
		else
			(void) sprintf(instbuf, "%.*s",
			    sizeof (instbuf),
			    gettext("Available for installation"));
	} else if (modinfo) {
		date = modinfo->m_instdate && modinfo->m_instdate[0] ?
			modinfo->m_instdate : date_unknown;

		if (modinfo->m_status == SELECTED)
			(void) sprintf(instbuf, "%.*s",
			    sizeof (instbuf),
			    gettext("Selected for removal"));
		else if (modinfo->m_status == PARTIAL)
			(void) sprintf(instbuf, "%.*s (%s)",
			    sizeof (instbuf) - strlen(date) - 4,
			    gettext("Partially Installed"), date);
		else
			(void) sprintf(instbuf, "%.*s (%s)",
			    sizeof (instbuf) - strlen(date) - 4,
			    gettext("Fully Installed"), date);
	} else
		instbuf[0] = '\0';

	xv_set(ip->PkgStatus, PANEL_VALUE, instbuf, NULL);

	if (mode == MODE_INSTALL && mod->type != UNBUNDLED_4X) {
		xv_set(ip->PkgBase,
			PANEL_READ_ONLY,	FALSE,
			PANEL_VALUE_UNDERLINED,	TRUE,
			NULL);
		xv_set(ip->PkgApply, PANEL_INACTIVE, FALSE, NULL);
		xv_set(ip->PkgReset, PANEL_INACTIVE, FALSE, NULL);
	} else {
		xv_set(ip->PkgBase,
			PANEL_READ_ONLY,	TRUE,
			PANEL_VALUE_UNDERLINED,	FALSE,
			NULL);
		xv_set(ip->PkgApply, PANEL_INACTIVE, TRUE, NULL);
		xv_set(ip->PkgReset, PANEL_INACTIVE, TRUE, NULL);
	}

	sp = calc_module_space(mod);

	/*
	 * XXX Hack
	 */
	sizes[FS_ROOT] = get_fs_space(sp, "/");
	sizes[FS_USR] = get_fs_space(sp, "/usr");
	sizes[FS_OPT] = get_fs_space(sp, "/opt");
	sizes[FS_VAR] = get_fs_space(sp, "/var");
	sizes[FS_EXPORT] = get_fs_space(sp, "/export");
	sizes[FS_USROWN] = get_fs_space(sp, "/usr/openwin");

	for (i = 0; i < FS_MAX; i++)
		(void) sprintf(strings[i], "%6.2f", (float)sizes[i] / 1024);

	xv_set(ip->PkgRoot, PANEL_VALUE, strings[FS_ROOT], NULL);
	xv_set(ip->PkgUsr, PANEL_VALUE, strings[FS_USR], NULL);
	xv_set(ip->PkgOpt, PANEL_VALUE, strings[FS_OPT], NULL);
	xv_set(ip->PkgVar, PANEL_VALUE, strings[FS_VAR], NULL);
	xv_set(ip->PkgExport, PANEL_VALUE, strings[FS_EXPORT], NULL);
	xv_set(ip->PkgOpenwin, PANEL_VALUE, strings[FS_USROWN], NULL);
}

void
SetPackageInfo(Xv_opaque instance)
{
	Pkg_PkgWin_objects *ip = (Pkg_PkgWin_objects *)instance;
	Module	*mod = (Module *)xv_get(ip->PkgCtrl, PANEL_CLIENT_DATA);
	char	*dir = (char *)xv_get(ip->PkgBase, PANEL_VALUE);

	if (mod == (Module *)0)
		return;

	set_basedir(mod, dir, (char *)0);
}

void
ResetPackageInfo(Xv_opaque instance)
{
	Pkg_PkgWin_objects *ip = (Pkg_PkgWin_objects *)instance;
	Module	*mod = (Module *)xv_get(ip->PkgCtrl, PANEL_CLIENT_DATA);

	if (mod == (Module *)0)
		return;

	if (mod->type == PRODUCT || mod->type == NULLPRODUCT)
		SET_VALUE(ip->PkgBase, mod->info.prod->p_instdir ?
			mod->info.prod->p_instdir : mod->info.prod->p_rootdir);
	else
		SET_VALUE(ip->PkgBase, mod->info.mod->m_instdir ?
			mod->info.mod->m_instdir : mod->info.mod->m_basedir);

	if (mod->type == UNBUNDLED_4X)
		xv_set(ip->PkgBase, PANEL_INACTIVE, TRUE, NULL);
	else
		xv_set(ip->PkgBase, PANEL_INACTIVE, FALSE, NULL);
}

void
GetProductInfo(Module *mod, int display)
{
	Pkg_ProdWin_objects *ip = Pkg_ProdWin;
	Module	*prod, *loc;
	Module	*m;
	char	*prodname;
	char	buf[256];
	int	nrows;

	prod = (Module *)0;
	prodname = (char *)0;

	if (mod != (Module *)0) {
		prodname = get_product_name(mod);

		m = mod;
		do {
			if (m->type == PRODUCT || m->type == NULLPRODUCT)
				prod = m;
			m = m->parent;
		} while (prod == (Module *)0 &&
				m != (Module *)0 && m->type != MEDIA);
	}

	if (prod != (Module *)0) {
		set_current(prod);
		/*
		 * Locale list
		 */
		nrows = (int)xv_get(ip->ProdLocList, PANEL_LIST_NROWS);
		if (nrows > 0)
			xv_set(ip->ProdLocList,
			    PANEL_LIST_DELETE_ROWS,	0,	nrows,
			    NULL);

		for (loc = get_all_locales(), nrows = 0; loc;
		    loc = get_next(loc), nrows++) {
			(void) sprintf(buf, "%s (%s)",
				loc->info.locale->l_language,
				loc->info.locale->l_locale);
			xv_set(ip->ProdLocList,
			    PANEL_LIST_INSERT,		nrows,
			    PANEL_LIST_STRING,		nrows,	buf,
			    PANEL_LIST_SELECT,		nrows,
				loc->info.locale->l_selected,
			    PANEL_LIST_CLIENT_DATA,	nrows,
				(Xv_opaque)loc->info.locale,
			    NULL);
		}
		xv_set(ip->ProdName,
			PANEL_VALUE,		prodname,
			PANEL_INACTIVE,		FALSE,
			NULL);
		xv_set(ip->ProdLocList,
			PANEL_INACTIVE,		nrows ? FALSE : TRUE,
			PANEL_CLIENT_DATA,	prod,
			NULL);
		xv_set(ip->ProdApply,
			PANEL_INACTIVE,	nrows ? FALSE : TRUE,
			NULL);
		xv_set(ip->ProdReset,
			PANEL_INACTIVE,	nrows ? FALSE : TRUE,
			NULL);
	} else if (prod == (Module *)0) {
		nrows = (int)xv_get(ip->ProdLocList, PANEL_LIST_NROWS);
		if (nrows > 0)
			xv_set(ip->ProdLocList,
			    PANEL_LIST_DELETE_ROWS,	0,	nrows,
			    NULL);
		xv_set(ip->ProdName,
			PANEL_VALUE,		"",
			PANEL_INACTIVE,		TRUE,
			NULL);
		xv_set(ip->ProdLocList,
			PANEL_INACTIVE,		TRUE,
			PANEL_CLIENT_DATA,	0,
			NULL);
		xv_set(ip->ProdApply, PANEL_INACTIVE, TRUE, NULL);
		xv_set(ip->ProdReset, PANEL_INACTIVE, TRUE, NULL);
	}
	/*
	 * Need this for reset, since another module
	 * may become the current one between the time
	 * we populate the window and the time the
	 * user hits reset.
	 */
	xv_set(ip->ProdCtrl, PANEL_CLIENT_DATA, (Xv_opaque)mod, NULL);
	if (display)
		xv_set(ip->ProdWin, XV_SHOW, TRUE, NULL);
}

void
SetProductInfo(Xv_opaque instance)
{
	Pkg_ProdWin_objects *ip = (Pkg_ProdWin_objects *)instance;
	Module	*prod = (Module *)xv_get(ip->ProdLocList, PANEL_CLIENT_DATA);
	Locale	*loc;
	int	nrows;
	int	i;

	nrows = (int)xv_get(ip->ProdLocList, PANEL_LIST_NROWS);
	for (i = 0; i < nrows; i++) {
		loc = (Locale *)
			xv_get(ip->ProdLocList, PANEL_LIST_CLIENT_DATA, i);
		loc->l_selected =
		    (int)xv_get(ip->ProdLocList, PANEL_LIST_SELECTED, i);
	}
	if (prod) {
		Node	*pkg = prod->info.prod->p_packages->list;
		/*
		 * Select all localization packages not
		 * explicitly attached to any package
		 * (i.e., with empty SUNW_PKGLIST= macros).
		 */
		update_l10n_package_status(prod);
		/*
		 * So the space code counts selected locale
		 * packages, set the action field on all
		 * such packages.  Selection status will
		 * be set either above or when a localized
		 * package is selected (via mark_module and
		 * mark_locale).
		 */
		if (get_view() == VIEW_NATIVE) {
			for (pkg = pkg->next;
		   	    pkg != prod->info.prod->p_packages->list;
			    pkg = pkg->next) {
				Modinfo	*info = (Modinfo *)pkg->data;

				if (info->m_locale != (char *)0)
					info->m_action = TO_BE_PKGADDED;
			}
		}
	}
	UpdateModules();
}


/*
 * Set module base directory.  Keep track of the
 * root module's existing base setting and only
 * reset those descendents that have the same or
 * an unspecified installation directory.
 */
static void
set_basedir(Module *mod, char *new, char *old)
{
	Module	*sub;

	if (mod->type == PRODUCT || mod->type == NULLPRODUCT) {
		if (old == (char *)0)
			old = mod->info.prod->p_instdir;
		clear_product_basedir(mod);
		set_product_basedir(mod, new);
	} else {
		if (old == (char *)0)
			old = mod->info.mod->m_instdir;
		if (old)
			clear_pkg_dir(mod, old);
		set_pkg_dir(mod, new);
	}
	for (sub = mod->sub; sub; sub = sub->next)
		set_basedir(sub, new, old);
}

static char *
get_product_name(Module *mod)
{
	static char buf[BUFSIZ];
	char	*cp;

	if (mod->type == PACKAGE ||
	    mod->type == MODULE ||
	    mod->type == CLUSTER ||
	    mod->type == METACLUSTER ||
	    mod->type == UNBUNDLED_4X) {
		if (mod->info.mod->m_prodname &&
		    mod->info.mod->m_prodname[0]) {
			(void) sprintf(buf, "%s %s",
			    mod->info.mod->m_prodname,
			    mod->info.mod->m_prodvers ?
				mod->info.mod->m_prodvers : "");
			return (buf);
		}
		mod = get_parent_product(mod);
	}
	if (mod != (Module *)0) {
		if (mod->type == PRODUCT) {
			if (mod->info.prod->p_name &&
			    mod->info.prod->p_name[0]) {
				(void) sprintf(buf, "%s %s",
				    mod->info.prod->p_name,
				    mod->info.prod->p_version ?
					mod->info.prod->p_version : "");
				return (buf);
			}
		} else if (mod->type == NULLPRODUCT) {
			if (mod->info.prod->p_name &&
			    mod->info.prod->p_name[0]) {
				(void) sprintf(buf, "%s %s",
				    mod->info.prod->p_name,
				    mod->info.prod->p_version ?
					mod->info.prod->p_version : "");
				return (buf);
			}
			mod = mod->parent;
			if (mod && mod->type == MEDIA &&
			    mod->info.media->med_volume) {
				(void) strcpy(buf, mod->info.media->med_volume);
				for (cp = buf; *cp; cp++)
					if (*cp == '_')
						*cp = ' ';
				return (buf);
			}
		}
	}
	return (gettext("product unknown"));
}
