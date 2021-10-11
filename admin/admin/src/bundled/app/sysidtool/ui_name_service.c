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

#pragma	ident	"@(#)ui_name_service.c 1.5 95/10/06"

/*
 *	File:		name_service.c
 *
 *	Description:	This file contains the routines needed to prompt
 *			the user naming service information.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "sysid_ui.h"
#include "sysid_msgs.h"


static Validate_proc	ui_valid_domainname;

/*
 * ui_valid_domainname:
 *
 *	Validation routine for checking the validity of a domainname.
 *	We only check for a non-null domainname.
 */

static int
ui_valid_domainname(Field_desc *f)
{
	char	*input = (char *)f->value;
	char	*domain;
	char	*dom_buf;
	char	*cp;
	int	l;

	/*
	 * Get the field value and remove leading and trailing spaces.
	 */

	dom_buf = xstrdup(input);
	for (domain = dom_buf; *domain != NULL; domain++) {
		if (!isspace((int) *domain)) {
			break;
		}
	}
	for (cp = (char *) (domain + strlen(domain)); cp > domain; cp--) {
		if (!isspace((int) *(char *)(cp - 1))) {
			break;
		}
	}
	*cp = NULL;

	/*
	 * Validate the domainname.
	 */

	l = strlen(domain);
	if ((l < 1) || (l >= MAX_DOMAINNAME)) {
		free(dom_buf);
		return (SYSID_ERR_DOMAIN_LEN);
	}

	free(dom_buf);
	return (SYSID_SUCCESS);
}


/*
 * ui_get_name_service:
 *
 *	This routine is the client interface routine used for
 *	retrieving the desired naming service to be used.
 *	menu of possible naming services is presented.
 *
 *	Input:  pointer to character buffer in which to place
 *		the internal token indicating the selected
 *		naming service.
 *
 *		pointer to an integer holding the default menu
 *		item number.  This integer is updated to the
 *		user's actual selection.
 */

static	Field_desc	name_service[] = {
	{ FIELD_EXCLUSIVE_CHOICE, (void *)ATTR_NAME_SERVICE, NULL, NULL, NULL,
		-1, -1, -1, -1, FF_LAB_ALIGN | FF_LAB_LJUST | FF_VALREQ,
		ui_valid_choice }
};

void
ui_get_name_service(MSG *mp, int reply_to)
{
	static Menu menu;
	static Field_help help;
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

	name_service[0].help = dl_get_attr_help(ATTR_NAME_SERVICE, &help);
	name_service[0].label = dl_get_attr_name(ATTR_NAME_SERVICE);
	name_service[0].value = (void *)&menu;

	dl_do_form(
		dl_get_attr_title(ATTR_NAME_SERVICE),
		dl_get_attr_text(ATTR_NAME_SERVICE),
		name_service, 1, reply_to);
}


/*
 * ui_get_domain:
 *
 *	This routine is the client interface routine used for
 *	retrieving the name of the domain the installing system
 *	join.  The user is prompted to enter the name of the domain.
 *
 *	Input:  pointer to character buffer in which to place
 *		the entered domain name.
 */

static	Field_desc	domains[] = {
	{ FIELD_TEXT, (void *)ATTR_DOMAIN, NULL, NULL, NULL,
		32, MAX_DOMAINNAME, -1, -1,
		FF_LAB_ALIGN | FF_LAB_RJUST | FF_KEYFOCUS,
		ui_valid_domainname },
};

void
ui_get_domain(MSG *mp, int reply_to)
{
	static char	domain[MAX_DOMAINNAME+1];
	static Field_help help;

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)domain, sizeof (domain));
	(void) msg_delete(mp);

	domains[0].help = dl_get_attr_help(ATTR_DOMAIN, &help);
	domains[0].label = dl_get_attr_name(ATTR_DOMAIN);
	domains[0].value = domain;

	dl_do_form(
		dl_get_attr_title(ATTR_DOMAIN),
		dl_get_attr_text(ATTR_DOMAIN),
		domains, 1, reply_to);
}


/*
 * ui_get_broadcast:
 *
 *	This routine is the client interface routine used
 *	to determine if the naming server should be located
 *	using a broadcast, or should be explicitly named by
 *	the client.
 *
 *	Input:  pointer to character buffer in which to place
 *		the internal token indicating the selection.
 *
 *		pointer to an integer holding the default menu
 *		item number.  This integer is updated to the
 *		user's actual selection.
 */

static	Field_desc	location[] = {
	{ FIELD_EXCLUSIVE_CHOICE, (void *)ATTR_BROADCAST, NULL, NULL, NULL,
		-1, -1, -1, -1, FF_LAB_ALIGN | FF_LAB_LJUST | FF_VALREQ,
		ui_valid_choice }
};

void
ui_get_broadcast(MSG *mp, int reply_to)
{
	static Field_help help;
	static Menu menu;

	if (menu.labels == (char **)0) {
		menu.labels = (char **)xmalloc(2 * sizeof (char *));
		menu.values = (void *)0;

		menu.labels[0] = BROADCAST;
		menu.labels[1] = SPECNAME;

		menu.nitems = 2;

		location[0].help = dl_get_attr_help(ATTR_BROADCAST, &help);
		location[0].label = dl_get_attr_name(ATTR_BROADCAST);
		location[0].value = (void *)&menu;
	}
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)&menu.selected, sizeof (int));
	(void) msg_delete(mp);

	dl_do_form(
		dl_get_attr_title(ATTR_BROADCAST),
		dl_get_attr_text(ATTR_BROADCAST),
		location, 1, reply_to);
}

#ifdef notdef
/*
 * ui_get_save_NIS:
 *
 *	This routine is the client interface routine used for
 *	asking the user whether or not the system parameters
 *	should be saved under NIS.
 *
 *	Input:  Address of an integer holding the default menu
 *		item number.  This integer will be updated to the
 *		user's actual selection.
 *
 *	Returns: 1 if it should be saved, otherwise 0.
 */


static	Field_desc	save_params[] = {
	{ FIELD_CONFIRM, (void *)ATTR_SAVE_TO_NIS, NULL, NULL, NULL,
		-1, -1, -1, -1, FF_LAB_LJUST, NULL },
};

void
ui_get_save_NIS(MSG *mp, int reply_to)
{
	static	char label[256];
	int	save_NIS;

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)&save_NIS, sizeof (int));
	(void) msg_delete(mp);

	/*
	 * The prompt string in the message catalog is
	 * terminated by a colon.  We want it terminated
	 * by a question mark, to match the other, similar
	 * questions.  This hack keeps the localization
	 * folks from having to retranslate the string
	 * just to change the punctuation.
	 *
	 * Remove the following block of code and substitute
	 * the following line:
	 *
	 *	save_params[0].label = SAVE_TO_NIS;
	 *
	 * when this is changed.
	 */
	if (label[0] == '\0') {
		char	*cp;
		(void) strncpy(label, SAVE_TO_NIS, sizeof (label));
		label[sizeof (label) - 1] = '\0';
		cp = strrchr(label, ':');
		if (cp)
			*cp = '?';
		save_params[0].label = label;
	}

	save_params[0].value = (void *)save_NIS;

	dl_do_form(SAVE_PARAMS_TO_NIS, save_params, 1, reply_to);
}
#endif


/*
 * get_nisservers:
 *
 *	This routine is the client interface routine
 *	used for retrieving the hostnames and IP addresses of the NIS servers.
 *
 *	Input:	pointers to character buffers in which to place the
 *		values for the servername and server address
 */

static Field_desc	serverinfo[] = {
	{ FIELD_TEXT, (void *)ATTR_NISSERVERNAME, NULL, NULL, NULL,
		32, MAX_HOSTNAME, -1, -1,
		FF_LAB_ALIGN | FF_LAB_RJUST | FF_KEYFOCUS,
		ui_valid_hostname },
	{ FIELD_TEXT, (void *)ATTR_NISSERVERADDR, NULL, NULL, NULL,
		16, MAX_IPADDR, -1, -1, FF_LAB_ALIGN | FF_LAB_RJUST,
		ui_valid_host_ip_addr },
};

void
ui_get_nisservers(MSG *mp, int reply_to)
{
	static Field_help help, *phelp;
	static char	server_name[MAX_HOSTNAME+1];
	static char	server_addr[MAX_IPADDR+1];

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&server_name, sizeof (server_name));
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&server_addr, sizeof (server_addr));
	(void) msg_delete(mp);

	phelp = dl_get_attr_help(ATTR_NISSERVERNAME, &help);

	serverinfo[0].help = phelp;
	serverinfo[0].label = dl_get_attr_name(ATTR_NISSERVERNAME);
	serverinfo[0].value = server_name;

	serverinfo[1].help = phelp;
	serverinfo[1].label = dl_get_attr_name(ATTR_NISSERVERADDR);
	serverinfo[1].value = server_addr;

	dl_do_form(
		dl_get_attr_title(ATTR_NISSERVERNAME),
		dl_get_attr_text(ATTR_NISSERVERNAME),
		serverinfo, 2, reply_to);
}

static	Field_desc	retry[] = {
	{ FIELD_CONFIRM, (void *)ATTR_BAD_NIS, NULL, NULL, NULL,
		-1, -1, -1, -1, FF_LAB_LJUST, NULL },
};

/*
 * ui_get_bad_nis:
 */
void
ui_get_bad_nis(MSG *mp, int reply_to)
{
	static Field_help help;
	char	errmsg[BUFSIZ];

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)errmsg, sizeof (errmsg));
	(void) msg_delete(mp);

	retry[0].help = dl_get_attr_help(ATTR_BAD_NIS, &help);
	retry[0].label = dl_get_attr_prompt(ATTR_BAD_NIS);
	retry[0].value = (void *)0;

	dl_do_form(dl_get_attr_title(ATTR_BAD_NIS), errmsg, retry, 1, reply_to);
}
