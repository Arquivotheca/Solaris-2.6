/*
 * Copyright (c) 1991,1992,1993 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
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

#pragma	ident	"@(#)ui_net.c 1.4 93/12/06"

/*
 *	File:		ui_net.c
 *
 *	Description:	This file contains the routines needed to prompt
 *			the user for the primary network interface
 */

#include <stdlib.h>
#include <string.h>
#include "sysid_ui.h"
#include "sysid_msgs.h"


static	Field_desc	primary_net[] = {
	{ FIELD_EXCLUSIVE_CHOICE, (void *)ATTR_PRIMARY_NET, NULL, NULL, NULL,
		-1, -1, -1, -1,
		FF_LAB_ALIGN | FF_LAB_LJUST | FF_VALREQ | FF_FORCE_SCROLLABLE,
		ui_valid_choice }
};

void
ui_get_primary_net_if(MSG *mp, int reply_to)
{
	static Field_help help;
	static Menu menu;
	char	argbuf[MAX_FIELDVALLEN];
	int	i, n, pick;

	if (menu.nitems != 0)
		free_menu(&menu);

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)&n, sizeof (n));

	menu.labels = (char **)xmalloc(n * sizeof (char *));
	menu.values = (void *)0;

	for (i = 0; i < n; i++) {
		(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)argbuf, sizeof (argbuf));
		menu.labels[i] = xstrdup(argbuf);
	}
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)&pick, sizeof (pick));
	(void) msg_delete(mp);

	menu.nitems = n;
	menu.selected = pick;

	primary_net[0].help = dl_get_attr_help(ATTR_PRIMARY_NET, &help);
	primary_net[0].label = dl_get_attr_name(ATTR_PRIMARY_NET);
	primary_net[0].value = (void *)&menu;

	dl_do_form(
		dl_get_attr_title(ATTR_PRIMARY_NET),
		dl_get_attr_text(ATTR_PRIMARY_NET),
		primary_net, 1, reply_to);
}
