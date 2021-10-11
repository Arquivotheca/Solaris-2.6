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
#ident	"@(#)Props.c 1.4 93/04/09"
#endif

#include "defs.h"
#include "ui.h"
#include <sys/param.h>
#include <xview/textsw.h>
#include <group.h>
#include "Props_ui.h"

extern Props_PropsWin_objects *Props_PropsWin;

#define	SET_VALUE(f, v) xv_set((f), PANEL_VALUE, (v) ? (v) : "", NULL)

static void	MediaProps(Props_PropsWin_objects *ip);
static void	AdminProps(Props_PropsWin_objects *ip);
static void	DisplayProps(Props_PropsWin_objects *ip);
static void	CategoryProps(Props_PropsWin_objects *ip);
static void	HostProps(Props_PropsWin_objects *ip);

static void	FitPropsPanel(Props_PropsWin_objects *ip, Xv_opaque panel);

/*ARGSUSED*/
void
ShowProperties(int which, caddr_t data)
{
	switch (which) {
	case PROPS_MEDIA:
		MediaProps(Props_PropsWin);
		break;
	case PROPS_ADMIN:
		AdminProps(Props_PropsWin);
		break;
	case PROPS_BROWSER:
		DisplayProps(Props_PropsWin);
		break;
	case PROPS_CATEGORY:
		CategoryProps(Props_PropsWin);
		break;
	case PROPS_HOSTS:
		HostProps(Props_PropsWin);
		break;
	case PROPS_LAST:
	default:
		xv_set(Props_PropsWin->PropsWin, XV_SHOW, TRUE, NULL);
		break;
	}
}

static void
MediaProps(Props_PropsWin_objects *ip)
{
	xv_set(ip->PropsSet, PANEL_VALUE, PROPS_MEDIA, NULL);
	xv_set(ip->PropsWin, XV_SHOW, TRUE, NULL);

	xv_set(ip->LoadCtrl, XV_SHOW, TRUE, NULL);
	xv_set(ip->ConfigCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->DispCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->CatCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->HostCtrl, XV_SHOW, FALSE, NULL);

	FitPropsPanel(ip, ip->LoadCtrl);
}

static void
AdminProps(Props_PropsWin_objects *ip)
{
	xv_set(ip->PropsSet, PANEL_VALUE, PROPS_ADMIN, NULL);
	xv_set(ip->PropsWin, XV_SHOW, TRUE, NULL);

	xv_set(ip->ConfigCtrl, XV_SHOW, TRUE, NULL);
	xv_set(ip->LoadCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->CatCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->DispCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->HostCtrl, XV_SHOW, FALSE, NULL);

	FitPropsPanel(ip, ip->ConfigCtrl);
}

static void
DisplayProps(Props_PropsWin_objects *ip)
{
	xv_set(ip->PropsSet, PANEL_VALUE, PROPS_BROWSER, NULL);
	xv_set(ip->PropsWin, XV_SHOW, TRUE, NULL);

	xv_set(ip->DispCtrl, XV_SHOW, TRUE, NULL);
	xv_set(ip->LoadCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->ConfigCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->CatCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->HostCtrl, XV_SHOW, FALSE, NULL);

	FitPropsPanel(ip, ip->DispCtrl);
}

static void
CategoryProps(Props_PropsWin_objects *ip)
{
	xv_set(ip->PropsSet, PANEL_VALUE, PROPS_CATEGORY, NULL);
	xv_set(ip->PropsWin, XV_SHOW, TRUE, NULL);

	xv_set(ip->CatCtrl, XV_SHOW, TRUE, NULL);
	xv_set(ip->DispCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->LoadCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->ConfigCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->HostCtrl, XV_SHOW, FALSE, NULL);

	FitPropsPanel(ip, ip->CatCtrl);
}

static void
HostProps(Props_PropsWin_objects *ip)
{
	xv_set(ip->PropsSet, PANEL_VALUE, PROPS_HOSTS, NULL);
	xv_set(ip->PropsWin, XV_SHOW, TRUE, NULL);

	xv_set(ip->CatCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->DispCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->LoadCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->ConfigCtrl, XV_SHOW, FALSE, NULL);
	xv_set(ip->HostCtrl, XV_SHOW, TRUE, NULL);

	FitPropsPanel(ip, ip->HostCtrl);
}

static void
FitPropsPanel(Props_PropsWin_objects *ip, Xv_opaque panel)
{
	window_fit(panel);
	window_fit(ip->PropsCtrl);
	if (xv_get(ip->PropsCtrl, XV_WIDTH) < xv_get(panel, XV_WIDTH) + 2)
		xv_set(ip->PropsCtrl, XV_WIDTH, xv_get(panel, XV_WIDTH) + 2, 0);
	else
		xv_set(panel, XV_WIDTH, xv_get(ip->PropsCtrl, XV_WIDTH) - 2, 0);
	window_fit(ip->PropsWin);
}

/*
 * Called when the user applies their display changes;
 * we save each value for use during "reset" and re-display
 * the software using the new parameters.
 */
void
SetDisplayModes(void)
{
	Props_PropsWin_objects *ip = Props_PropsWin;
	Config	config;

	config_get(&config);

	config.gui_mode.value = (int)xv_get(ip->DispIconic, PANEL_VALUE);
	config.namelen.value = (int)xv_get(ip->DispName, PANEL_VALUE);
	config.ncols.value = (int)xv_get(ip->DispCols, PANEL_VALUE);

	config_set(&config);

	UpdateModules();
}

/*
 * Called when user presses "reset"; reset values from last "apply".
 */
void
ResetDisplayModes(void)
{
	Props_PropsWin_objects *ip = Props_PropsWin;
	Config	config;

	config_get(&config);

	xv_set(ip->DispIconic, PANEL_VALUE, config.gui_mode.value, NULL);
	xv_set(ip->DispName, PANEL_VALUE, config.namelen.value, NULL);
	xv_set(ip->DispCols, PANEL_VALUE, config.ncols.value, NULL);
}
