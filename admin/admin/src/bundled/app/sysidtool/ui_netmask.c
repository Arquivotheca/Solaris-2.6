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

#pragma	ident	"@(#)ui_netmask.c 1.5 95/10/06"

/*
 *	File:		netmask.c
 *
 *	Description:	This file contains the routines needed to prompt
 *			the user as to whether or not the system being
 *			installed is standalone or connected to a network,
 *			and if networked, subnet information.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "sysid_ui.h"
#include "sysid_msgs.h"

static Validate_proc	ui_valid_ip_netmask;

/*
 * ui_valid_ip_netmask:
 *
 *	Validation routine for checking the validity of an IP netmask.
 */

static int
ui_valid_ip_netmask(Field_desc *f)
{
	char	*input = (char *)f->value;
	char	*netmask;
	char	*mask_buf;
	char	*cp;

	/*
	 * Get the field value and remove leading and trailing spaces.
	 */

	mask_buf = xstrdup(input);
	for (netmask = mask_buf; *netmask != NULL; netmask++) {
		if (!isspace((int) *netmask)) {
			break;
		}
	}
	for (cp = (char *) (netmask + strlen(netmask)); cp > netmask; cp--) {
		if (!isspace((int) *(char *)(cp - 1))) {
			break;
		}
	}
	*cp = NULL;

	/*
	 * Validate the netmask.
	 */

	if (!valid_ip_netmask(netmask)) {
		free(mask_buf);
		return (SYSID_ERR_NETMASK_FMT);
	}

	free(mask_buf);
	return (SYSID_SUCCESS);
}


/*
 * ui_get_subnetted:
 *
 *	This routine is the client interface routine used for
 *	asking the user whether or not the network the system
 *	is being installed into is a subnetwork.
 *
 *	Input:  Address of integer holding the default menu item
 *		number.  This integer is updated to the user's
 *		actual selection.
 *
 *	Returns: 1 if it is a subnet, otherwise 0.
 */

static	Field_desc	subnets[] = {
	{FIELD_CONFIRM, (void *)ATTR_SUBNETTED, NULL, NULL, NULL,
		-1, -1, -1, -1, FF_LAB_ALIGN | FF_LAB_LJUST, NULL }
};

void
ui_get_subnetted(MSG *mp, int reply_to)
{
	static Field_help help;
	int	isit_subnet;

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)&isit_subnet, sizeof (int));
	(void) msg_delete(mp);

	subnets[0].help = dl_get_attr_help(ATTR_SUBNETTED, &help);
	subnets[0].label = dl_get_attr_prompt(ATTR_SUBNETTED);
	subnets[0].value = (void *)isit_subnet;

	dl_do_form(
		dl_get_attr_title(ATTR_SUBNETTED),
		dl_get_attr_text(ATTR_SUBNETTED),
		subnets, 1, reply_to);
}


/*
 * ui_get_netmask:
 *
 *	This routine is the client interface routine used for
 *	retrieving the subnet mask from the user.
 *
 *	Input:  pointer to character buffer in which to place
 *		the subnet mask.
 */

static	Field_desc netmasks[] = {
	{FIELD_TEXT, (void *)ATTR_NETMASK, NULL, NULL, NULL,
		15, MAX_NETMASK, -1, -1,
		FF_LAB_ALIGN | FF_LAB_RJUST | FF_KEYFOCUS,
		ui_valid_ip_netmask}
};

void
ui_get_netmask(MSG *mp, int reply_to)
{
	static Field_help help;
	static char netmask[MAX_NETMASK+1];

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)netmask, sizeof (netmask));
	(void) msg_delete(mp);

	netmasks[0].help = dl_get_attr_help(ATTR_NETMASK, &help);
	netmasks[0].label = dl_get_attr_prompt(ATTR_NETMASK);
	netmasks[0].value = netmask;

	dl_do_form(
		dl_get_attr_title(ATTR_NETMASK),
		dl_get_attr_text(ATTR_NETMASK),
		netmasks, 1, reply_to);
}


/*
 * ui_get_isit_standalone:
 *
 *	This routine is the client interface routine used for
 *	asking the user whether or not the system is being setup
 *	standalone or connected to a network.
 *
 *	Input:  Address of integer containing the default item to
 *		select in this menu.  This integer will be updated
 *		to the actual item selected by the user.
 *
 *	Returns: 1 if it is standalone, otherwise 0.
 */

static	Field_desc	question[] = {
	{FIELD_CONFIRM, (void *)ATTR_NETWORKED, NULL, NULL, NULL,
		-1, -1, -1, -1, FF_LAB_ALIGN | FF_LAB_LJUST, NULL }
};

void
ui_get_networked(MSG *mp, int reply_to)
{
	static Field_help help;
	int	isit_networked;

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)&isit_networked, sizeof (int));
	(void) msg_delete(mp);

	question[0].help = dl_get_attr_help(ATTR_NETWORKED, &help);
	question[0].label = dl_get_attr_prompt(ATTR_NETWORKED);
	question[0].value = (void *)isit_networked;

	dl_do_form(
		dl_get_attr_title(ATTR_NETWORKED),
		dl_get_attr_text(ATTR_NETWORKED),
		question, 1, reply_to);
}
