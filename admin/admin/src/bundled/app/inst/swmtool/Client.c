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
#ident	"@(#)Client.c 1.8 94/05/26"
#endif

#include "defs.h"
#include "ui.h"
#include "Client_ui.h"

static	int SetClientOs(
	Panel_item, char *, Xv_opaque, Panel_list_op, Event *, int);
static	int SetClientArch(
	Panel_item, char *, Xv_opaque, Panel_list_op, Event *, int);
static	void set_required(Module *);

/*ARGSUSED1*/
void
InitClientInfo(Xv_opaque instance, SWM_mode mode)
{
	Client_ClientWin_objects *ip = (Client_ClientWin_objects *)instance;
	Module	*media, *prod;
	char	name[256];
	int	nrows;

	if (get_view() == VIEW_SERVICES)
		return;
	/*
	 * Recreate OS list
	 */
	nrows = (int)xv_get(ip->ClientOs, PANEL_LIST_NROWS);
	xv_set(ip->ClientOs,
	    PANEL_LIST_DELETE_ROWS,		0,	nrows,
	    PANEL_CLIENT_DATA,			0,
	    PANEL_LIST_INSERT_DUPLICATE,	FALSE,
	    PANEL_NOTIFY_PROC,			SetClientOs,
	    NULL);
	nrows = 0;

	media = get_source_media();
	if (media != (Module *)0) {
		for (prod = media->sub; prod; prod = prod->next) {
			/*
			 * XXX We really only want to show
			 * products for which the user can
			 * install client support (i.e., OS
			 * releases).
			 */
			if (prod->type == NULLPRODUCT)
				continue;

			(void) sprintf(name, "%s %s.%s",
			    prod->info.prod->p_name,
			    prod->info.prod->p_version,
			    prod->info.prod->p_rev);

			xv_set(ip->ClientOs,
			    PANEL_LIST_INSERT,		nrows,
			    PANEL_LIST_STRING,		nrows,	name,
			    PANEL_LIST_SELECT,		nrows,	FALSE,
			    PANEL_LIST_CLIENT_DATA,	nrows,	prod,
			    NULL);
			nrows++;
		}
	}
	(void) SetClientOs((Panel_item)ip->ClientOs,
		(char *)0,
		(Xv_opaque)0,
		PANEL_LIST_OP_SELECT,
		(Event *)0,
		0);
	xv_set(ip->ClientOs,
		PANEL_INACTIVE,		nrows == 0 ? TRUE : FALSE,
		NULL);
	xv_set(ip->ClientApply,
		PANEL_INACTIVE,		nrows == 0 ? TRUE : FALSE,
		NULL);
	xv_set(ip->ClientReset,
		PANEL_INACTIVE,		nrows == 0 ? TRUE : FALSE,
		NULL);
	SetClientInfo((Panel_item)ip->ClientApply);
}

/*ARGSUSED*/
static int
SetClientOs(Panel_item	item,
	char		*string,
	Xv_opaque	client_data,
	Panel_list_op	op,
	Event		*event,
	int		row)
{
	Client_ClientWin_objects *ip =
	    (Client_ClientWin_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	Module	*prod = (Module *)client_data;
	Arch	*arch;
	int	nrows;
	char	prodname[MAXPATHLEN];

	/*
	 * Recreate architecture list
	 */
	nrows = (int)xv_get(ip->ClientArch, PANEL_LIST_NROWS);
	xv_set(ip->ClientArch,
	    PANEL_LIST_DELETE_ROWS,	0,	nrows,
	    NULL);
	nrows = 0;

	if (prod != (Module *)0 && op == PANEL_LIST_OP_SELECT) {
		Module	*inst;
		/*
		 * Find out if the product for which we are
		 * installing client support already has some
		 * support installed on the system.  If so,
		 * select all currently-installed architectures.
		 *
		 * XXX Since there's no provision for having
		 * multiple revisions of a product on the same
		 * server, we ignore revisions.
		 */
		(void) sprintf(prodname, "%s_%s",
			prod->info.prod->p_name,
			prod->info.prod->p_version);

		for (inst = get_media_head();
		    inst != (Module *)0;
		    inst = inst->next) {
			if (inst->info.media->med_type != INSTALLED_SVC)
				continue;
			if (strcmp(
			    prodname, inst->info.media->med_volume) == 0) {
				for (arch = get_all_arches(inst->sub);
				    arch != (Arch *)0; arch = arch->a_next)
					(void) select_arch(prod, arch->a_arch);
				break;
			}
		}
		/*
		 * Select the native architecture,
		 * regardless of whether or not it's
		 * already installed (this is enforced
		 * by the software library).
		 */
		(void) select_arch(prod, get_default_arch());

		for (arch = get_all_arches(prod);
		    arch != (Arch *)0; arch = arch->a_next) {
			xv_set(ip->ClientArch,
			    PANEL_LIST_INSERT,		nrows,
			    PANEL_LIST_STRING,		nrows, arch->a_arch,
			    PANEL_LIST_SELECT,		nrows, arch->a_selected,
			    PANEL_LIST_CLIENT_DATA,	nrows, (Xv_opaque)arch,
			    NULL);
			nrows++;
		}
	}
	xv_set(ip->ClientArch,
		PANEL_NOTIFY_PROC,	SetClientArch,
		PANEL_INACTIVE,		nrows > 0 ? FALSE : TRUE,
		NULL);
	/*
	 * Set footer string
	 */
	xv_set(ip->ClientWin,
	    FRAME_LEFT_FOOTER,	dgettext("SUNW_INSTALL_SWM",
		"Select architectural support to install..."),
	    NULL);

	return (XV_OK);
}

/*ARGSUSED*/
static int
SetClientArch(Panel_item item,
	char		*string,
	Xv_opaque	client_data,
	Panel_list_op	op,
	Event		*event,
	int		row)
{
	Client_ClientWin_objects *ip =
	    (Client_ClientWin_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	Arch	*arch = (Arch *)client_data;

	if (op == PANEL_LIST_OP_DESELECT) {
		/*
		 * Policy enforced by underlying service library
		 * code:  the architecture of the machine must
		 * always be installed (i.e., you must always
		 * install it as part of OS support and you can
		 * never remove it).
		 *
		 * XXX Allow user to de-select installed arches?
		 */
		if (arch &&
		    (strcmp(arch->a_arch, get_default_arch()) == 0 ||
		    strcmp(arch->a_arch, get_default_impl()) == 0 ||
		    strcmp(arch->a_arch, get_default_inst()) == 0)) {
			xv_set(item,
			    PANEL_LIST_SELECT,	row,	TRUE,
			    NULL);
			NoticeNativeArch(ip->ClientWin);
			return (XV_ERROR);
		}
	}
	return (XV_OK);
}

/*
 * The "Apply" function for the client support
 * selection window.
 */
void
SetClientInfo(Panel_item item)
{
	Client_ClientWin_objects *ip =
	    (Client_ClientWin_objects *)xv_get(item, XV_KEY_DATA, INSTANCE);
	Module	*prod = (Module *)0;
	Arch	*arch;
	int	nrows;
	int	i;

	nrows = (int)xv_get(ip->ClientOs, PANEL_LIST_FIRST_SELECTED);
	if (nrows != -1)
		prod = (Module *)
			xv_get(ip->ClientOs, PANEL_LIST_CLIENT_DATA, nrows);
	xv_set(ip->ClientOs, PANEL_CLIENT_DATA, prod, NULL);

	nrows = (int)xv_get(ip->ClientArch, PANEL_LIST_NROWS);
	for (i = 0; i < nrows; i++) {
		arch = (Arch *)
			xv_get(ip->ClientArch, PANEL_LIST_CLIENT_DATA, i);
		if (arch == (Arch *)0)
			continue;
		if (xv_get(ip->ClientArch, PANEL_LIST_SELECTED, i)) {
			Module	*mod=NULL;
			for (mod = prod->sub; mod; mod = mod->next) {
				if (mod->type == METACLUSTER &&
				    !strcmp(mod->info.mod->m_pkgid,
				    ENDUSER_METACLUSTER))
					break;
			}
			/* If we did not find the end user metacluster
			 * default to the required metacluster.
			 * By definition the required metacluster must exist.
			 */
			if (mod == NULL)
				for (mod = prod->sub; mod; mod = mod->next) {
					if (mod->type == METACLUSTER &&
				    	    !strcmp(mod->info.mod->m_pkgid,
				    	    REQD_METACLUSTER))
					break;
				}
			add_service(prod, arch->a_arch, mod);
		} else {
			arch->a_selected = 0;
			mark_arch(prod);
		}
	}
	if (prod) {
		set_installed_media(get_current_view(prod));
		set_required(prod);
	} else
		set_installed_media((Module *)0);
	BrowseModules(MODE_INSTALL, VIEW_SERVICES);
}

/*
 * The "Reset" function for the client support
 * selection window.
 */
void
ResetClientInfo(Panel_item item)
{
	Client_ClientWin_objects *ip =
	    (Client_ClientWin_objects *)xv_get(item, XV_KEY_DATA, INSTANCE);
	Module	*prod;
	char	*name;
	int	nrows;
	int	i;

	name = (char *)0;
	prod = (Module *)xv_get(ip->ClientOs, PANEL_CLIENT_DATA);

	nrows = (int)xv_get(ip->ClientOs, PANEL_LIST_NROWS);
	for (i = 0; i < nrows; i++) {
		if (prod == (Module *)
		    xv_get(ip->ClientOs, PANEL_LIST_CLIENT_DATA, i)) {
			xv_set(ip->ClientOs,
			    PANEL_LIST_SELECT,	i, TRUE,
			    NULL);
			name = (char *)
			    xv_get(ip->ClientOs, PANEL_LIST_STRING, i);
		} else
			xv_set(ip->ClientOs,
			    PANEL_LIST_SELECT,	i, FALSE,
			    NULL);
	}
	(void) SetClientOs((Panel_item)ip->ClientOs,
	    name,
	    (Xv_opaque)prod,
	    PANEL_LIST_OP_SELECT,
	    (Event *)0,
	    nrows);
}

/*
 * set_required() - Mark required status
 *	The software library does not provide functionality
 *	to propagate required status up, only down.  This
 *	results in inconsistencies in the user interface;
 *	namely, that a cluster may not be marked as required
 *	when all its component parts are.  We run into this
 *	when installing client support, where all currently-
 *	installed components are required as part of the
 *	service.
 */
void
set_required(Module *mod)
{
	register int    n, r;
	register Module *mp;

	if (mod->type == MEDIA || mod->sub == (Module *)0)
		return;

	for (n = 0, r = 0, mp = mod->sub; mp; mp = mp->next) {
		if (mp->sub != (Module *)0)
			set_required(mp);
		n++;
		if (mp->info.mod->m_status == REQUIRED)
			r++;
	}

	if (n != 0 && n == r) {
		if (mod->type == PRODUCT || mod->type == NULLPRODUCT)
			mod->info.prod->p_status = REQUIRED;
		else
			mod->info.mod->m_status = REQUIRED;
	}
}
