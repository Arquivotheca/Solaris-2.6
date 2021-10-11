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
#ident	"@(#)Base.c 1.15 94/09/15"
#endif

#include "defs.h"
#include "ui.h"
#include <group.h>
#include "Base_ui.h"

static void ButtonSize(Base_BaseWin_objects *);

void
BaseResize(Xv_opaque instance)
{
	Base_BaseWin_objects *ip = (Base_BaseWin_objects *)instance;
	int	y, delta;
	int	x1, x2;
	int	needs_resize = 0;

	/*
	 * Lay out bottom row of buttons
	 * so each button lines up with
	 * the one above it.
	 */
	ButtonSize(ip);

	/*
	 * Line up mode setting centered on
	 * top row of buttons and tight to
	 * right side of screen.
	 */
	y = (int)xv_get(ip->BaseProps, XV_Y);
	delta =	(int)xv_get(ip->BaseMode, XV_HEIGHT) -
		(int)xv_get(ip->BaseProps, XV_HEIGHT);
	y -= (delta / 2);

	xv_set(ip->BaseModeGrp,
	    GROUP_ANCHOR_OBJ,		ip->BaseCtrl,
	    GROUP_ANCHOR_POINT,		GROUP_NORTHEAST,
	    GROUP_REFERENCE_POINT,	GROUP_NORTHEAST,
	    GROUP_HORIZONTAL_OFFSET,	-10,
	    GROUP_VERTICAL_OFFSET,	y,
	    NULL);

	x1 = (int)xv_get(ip->BaseModeGrp, XV_X);
	x2 = (int)xv_get(ip->BaseProps, XV_X) +
	    (int)xv_get(ip->BaseProps, XV_WIDTH) + 10;	/* min gap */

	if (x2 > x1) {
		xv_set(ip->BaseModeGrp,
		    GROUP_ANCHOR_OBJ,		ip->BaseProps,
		    GROUP_ANCHOR_POINT,		GROUP_EAST,
		    GROUP_REFERENCE_POINT,	GROUP_WEST,
		    GROUP_HORIZONTAL_OFFSET,	10,
		    GROUP_VERTICAL_OFFSET,	0,
		    NULL);
		needs_resize = 1;
	}

	/*
	 * Line up category centered on
	 * bottom row of buttons tight to
	 * right side of screen.
	 */
	y = (int)xv_get(ip->BaseDoit, XV_Y);
	delta =	(int)xv_get(ip->BaseCat, XV_HEIGHT) -
		(int)xv_get(ip->BaseDoit, XV_HEIGHT);
	y -= (delta / 2);

	xv_set(ip->BaseCatGrp,
	    GROUP_ANCHOR_OBJ,		ip->BaseCtrl,
	    GROUP_ANCHOR_POINT,		GROUP_NORTHEAST,
	    GROUP_REFERENCE_POINT,	GROUP_NORTHEAST,
	    GROUP_HORIZONTAL_OFFSET,	-10,
	    GROUP_VERTICAL_OFFSET,	y,
	    NULL);

	x1 = (int)xv_get(ip->BaseCatGrp, XV_X);
	x2 = (int)xv_get(ip->BaseDoit, XV_X) +
	    (int)xv_get(ip->BaseDoit, XV_WIDTH) + 10;	/* min gap */

	if (x2 > x1) {
		xv_set(ip->BaseCatGrp,
		    GROUP_ANCHOR_OBJ,		ip->BaseDoit,
		    GROUP_ANCHOR_POINT,		GROUP_EAST,
		    GROUP_REFERENCE_POINT,	GROUP_WEST,
		    GROUP_HORIZONTAL_OFFSET,	10,
		    GROUP_VERTICAL_OFFSET,	0,
		    NULL);
		needs_resize = 1;
	}

	if (needs_resize) {
		window_fit(ip->BaseCtrl);
		window_fit(ip->BaseWin);	/* XXX possible resize loop */
		xv_set(ip->BaseCtrl, XV_WIDTH, WIN_EXTEND_TO_EDGE, NULL);
	} else
		panel_paint(ip->BaseCtrl, PANEL_NO_CLEAR);
}

static void
ButtonSize(Base_BaseWin_objects *ip)
{
	static	int buttons_already_sized;
	int	file_len, view_len, edit_len, props_len;
	int	but_len;
	int	label_len;

	if (buttons_already_sized)
		return;

	file_len = (int)xv_get(ip->BaseFile, XV_WIDTH);
	view_len = (int)xv_get(ip->BaseView, XV_WIDTH);
	edit_len = (int)xv_get(ip->BaseEdit, XV_WIDTH);
	props_len = (int)xv_get(ip->BaseProps, XV_WIDTH);

	/*
	 * Determine how big the buttons are going
	 * to be.  This is the maximum width of the
	 * five basic buttons.
	 */
	but_len = 0;
	if (file_len > but_len)
		but_len = file_len;
	if (view_len > but_len)
		but_len = view_len;
	if (edit_len > but_len)
		but_len = edit_len;
	if (props_len > but_len)
		but_len = props_len;

	if (but_len > file_len) {
		label_len = (int)xv_get(ip->BaseFile, PANEL_LABEL_WIDTH);
		label_len += (but_len - file_len);
		xv_set(ip->BaseFile, PANEL_LABEL_WIDTH, label_len, NULL);
	}
	if (but_len > view_len) {
		label_len = (int)xv_get(ip->BaseView, PANEL_LABEL_WIDTH);
		label_len += (but_len - view_len);
		xv_set(ip->BaseView, PANEL_LABEL_WIDTH, label_len, NULL);
	}
	if (but_len > edit_len) {
		label_len = (int)xv_get(ip->BaseEdit, PANEL_LABEL_WIDTH);
		label_len += (but_len - edit_len);
		xv_set(ip->BaseEdit, PANEL_LABEL_WIDTH, label_len, NULL);
	}
	if (but_len > props_len) {
		label_len = (int)xv_get(ip->BaseProps, PANEL_LABEL_WIDTH);
		label_len += (but_len - props_len);
		xv_set(ip->BaseProps, PANEL_LABEL_WIDTH, label_len, NULL);
	}
	xv_set(ip->BaseTop, GROUP_LAYOUT, TRUE, NULL);
	xv_set(ip->BaseBottom, GROUP_LAYOUT, TRUE, NULL);

	buttons_already_sized = 1;
}

void
Apply(Xv_opaque instance)
{
	Base_BaseWin_objects *ip = (Base_BaseWin_objects *)instance;
	char	*footer;
	int	status;

	xv_set(ip->BaseWin, FRAME_BUSY, TRUE, NULL);

	if (get_mode() == MODE_INSTALL) {
		if (get_view() == VIEW_NATIVE) {
			footer = gettext("Installing selected software...");
			xv_set(ip->BaseWin, FRAME_LEFT_FOOTER, footer, NULL);
			status = tty_exec_func(
			    (void(*)(caddr_t))pkg_exec,
			    (caddr_t)get_source_media(),
			    Reset, (caddr_t)instance);
		} else {
			char	cmd[BUFSIZ];

			footer =
			    gettext("Installing selected client support...");
			xv_set(ip->BaseWin, FRAME_LEFT_FOOTER, footer, NULL);

			generate_swm_script(SWM_UPGRADE_SCRIPT);
			(void) sprintf(cmd, "sh %s /", SWM_UPGRADE_SCRIPT);
			status = tty_exec_cmd(cmd, Reset, (caddr_t)instance);

			if (status != SUCCESS) {
				/* try with the old upgrade script path. */
				generate_swm_script(SWM_UPGRADE_SCRIPT_OLD);
				(void) sprintf(cmd, "sh %s /", 
					SWM_UPGRADE_SCRIPT);
				status = tty_exec_cmd(cmd, Reset, 
					(caddr_t)instance);
			}

		}
	} else {
		footer = gettext("Removing selected software...");
		xv_set(ip->BaseWin, FRAME_LEFT_FOOTER, footer, NULL);
		status = tty_exec_func(
		    (void(*)(caddr_t))pkg_exec, (caddr_t)get_installed_media(),
		    Reset, (caddr_t)instance);
	}
	if (status != SUCCESS) {
		xv_set(ip->BaseWin, FRAME_BUSY, FALSE, NULL);
		Reset((caddr_t)instance, -1);
	}
}

/*ARGSUSED*/
void
Reset(caddr_t arg, int status)
{
	/*LINTED [alignment ok]*/
	Base_BaseWin_objects *ip = (Base_BaseWin_objects *)arg;
	Module	*media, *next;
	char	installed_media_dir[MAXPATHLEN];

	xv_set(ip->BaseWin,
	    FRAME_LEFT_FOOTER,	gettext(
		"Re-initializing list of installed software..."),
	    NULL);

	/*
	 * Save name of installed media directory 
	 * so we can re-establish installed media
	 * pointer after unload_media/load_installed
	 */
	media = get_installed_media();
	if (media != (Module *)0)
		(void) realpath(
			media->info.media->med_dir, installed_media_dir);
	else
		installed_media_dir[0] = '\0';

#ifdef lint
	next = (Module *)0;
#endif
	/*
	 * Tear down and free all installed
	 * media structures
	 */
	for (media = get_media_head(); media != (Module *)0; media = next) {
		next = media->next;
		if (media->type != MEDIA)
			continue;	/* sanity check */
		if (media->info.media->med_type == INSTALLED ||
		    media->info.media->med_type == INSTALLED_SVC)
			if (unload_media(media) != SUCCESS)
				die(gettext(
		    "PANIC:  cannot free list of installed software!\n"));
	}
	/*
	 * Re-load installed media(s) -- this
	 * creates any needed service media
	 * structures as well.
	 */
	media = load_installed("/", FALSE);
	if (media == (Module *)0)
		die(gettext(
		    "PANIC:  cannot reload list of installed software!\n"));

	/*
	 * Restore installed media pointer
	 */
	if (installed_media_dir[0] != '\0')
		set_installed_media(find_media(installed_media_dir, (char *)0));

	/*
	 * Reset selection status
	 * and display
	 */
	reset_selections(1, 0, 0);	/* selection status only */
	BrowseModules(MODE_UNSPEC, VIEW_UNSPEC);
}

void
BaseModeSet(SWM_mode mode, int active)
{
	extern Base_BaseWin_objects *Base_BaseWin;

	if (mode == MODE_REMOVE)
		xv_set(Base_BaseWin->BaseMode,
			PANEL_INACTIVE,		active ? FALSE : TRUE,
			PANEL_VALUE,		1,
			NULL);
	else
		xv_set(Base_BaseWin->BaseMode,
			PANEL_INACTIVE,		active ? FALSE : TRUE,
			PANEL_VALUE,		0,
			NULL);
}
