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
#ident	"@(#)xv_host.c 1.12 94/10/13"
#endif

#include "defs.h"
#include "ui.h"
#include "host.h"
#include <xview/font.h>
#include "Props_ui.h"

/*
 * i18n:  This is the format string for the host scrolling
 * list.  Change the last foramt spec to match translations
 * of "SELECTED" and "UNSELECTED".
 */
#define	HOSTFMT		gettext("%-12.12s  %-12.12s  %-12.12s")

extern	Props_PropsWin_objects *Props_PropsWin;

static	char *unknown_arch;
static 	Xv_font fixed_font;

static Xv_opaque HostImage(Hostlist *);
static void GetHostStatus(Props_PropsWin_objects *, Hostlist *);
static void SetPwdStatus(Props_PropsWin_objects *);

/*
 * One-time list initialization routine
 */
void
InitHosts(caddr_t instance)
{
	/*LINTED [alignment ok]*/
	Props_PropsWin_objects *ip = (Props_PropsWin_objects *)instance;

	fixed_font = (Xv_font)xv_find(ip->PropsWin, FONT,
		FONT_FAMILY,	FONT_FAMILY_DEFAULT_FIXEDWIDTH,
		FONT_STYLE,	FONT_STYLE_DEFAULT,
		NULL);

	unknown_arch = xstrdup(gettext("unknown"));

	xv_set(ip->HostPwd, PANEL_MASK_CHAR, ' ', NULL);

	GetHosts();
}

/*
 * Called to initialize the scrolling list
 * from the linked list of hosts.
 */
void
#ifdef __STDC__
GetHosts(void)
#else
GetHosts()
#endif
{
	Props_PropsWin_objects *ip = Props_PropsWin;
	Xv_opaque list = ip->HostList;
	Hostlist	*hlp, *new;
	int	rows = (int)xv_get(list, PANEL_LIST_NROWS);
	char	hoststr[1024];

	if (rows)
		xv_set(list, PANEL_LIST_DELETE_ROWS,
			0, (int)xv_get(list, PANEL_LIST_NROWS), NULL);

	for (hlp = hostlist.h_prev; hlp != &hostlist; hlp = hlp->h_prev) {
		new = host_alloc(hlp->h_name);
		new->h_passwd = xstrdup(hlp->h_passwd);
		new->h_rootdir = xstrdup(hlp->h_rootdir);
		new->h_status = hlp->h_status;
		new->h_arch = hlp->h_arch;
		new->h_type = hlp->h_type;
		(void) sprintf(hoststr, HOSTFMT,
		    new->h_name,
		    new->h_arch ? new->h_arch : unknown_arch,
		    new->h_status & HOST_SELECTED ?
			gettext("SELECTED") : gettext("UNSELECTED"));
		xv_set(list,
		    PANEL_LIST_INSERT,		0,
		    PANEL_LIST_FONT,		0,	fixed_font,
		    PANEL_LIST_STRING,		0,	hoststr,
		    PANEL_LIST_GLYPH,		0,	HostImage(new),
		    PANEL_LIST_CLIENT_DATA,	0,	new,
		    PANEL_LIST_SELECT,		0,
			new->h_status & HOST_SELECTED ? TRUE : FALSE,
		    NULL);
	}

	SetPwdStatus(ip);
}

/*
 * Called to re-initialized the linked list of
 * hosts from the contents of the scrolling list.
 */
void
#ifdef __STDC__
SetHosts(void)
#else
SetHosts()
#endif
{
	Props_PropsWin_objects *ip = Props_PropsWin;
	Xv_opaque list = ip->HostList;
	Hostlist *hlp, *new;
	Config	config;
	register int i;

	host_clear(&hostlist);

	for (i = 0; i < (int)xv_get(list, PANEL_LIST_NROWS); i++) {
		hlp = (Hostlist *)xv_get(list, PANEL_LIST_CLIENT_DATA, i);
		new = host_insert(&hostlist, hlp->h_name);
		new->h_passwd = xstrdup(hlp->h_passwd);
		new->h_rootdir = xstrdup(hlp->h_rootdir);
		new->h_status = hlp->h_status;
		new->h_arch = hlp->h_arch;
		new->h_type = hlp->h_type;
	}
	config_get(&config);
	config.hosts = host_string(&hostlist);
	config_set(&config);

	SetPwdStatus(ip);
}

void
AddHost(where)
	int	where;
{
	Props_PropsWin_objects *ip = Props_PropsWin;
	register int i;
	char	*name = (char *)xv_get(ip->HostName, PANEL_VALUE);
	char	*passwd = (char *)xv_get(ip->HostPwd, PANEL_VALUE);
	int	row = (int)xv_get(ip->HostList, PANEL_LIST_FIRST_SELECTED);
	char	hoststr[1024];
	Hostlist *hlp;

	switch (where) {
	case 0:	/* before */
		if (--row < 0)
			row = 0;
		break;
	case 1:	/* after */
		if (row++ < 0)
			row = (int)xv_get(ip->HostList, PANEL_LIST_NROWS);
		break;
	case 2:	/* top */
		row = 0;
		break;
	default:
	case 3: /* bottom */
		row = (int)xv_get(ip->HostList, PANEL_LIST_NROWS);
		break;
	}
	/*
	 * Add a host with the given name to the
	 * list AFTER the selected position (or
	 * bottom, if none selected).
	 */

	if (name == (char *)0 || name[0] == '\0') {
		asktoproceed(Hostscreen, gettext(
		    "First enter a host name, then press \"Add\"."));
		return;
	}

	for (i = 0; i < (int)xv_get(ip->HostList, PANEL_LIST_NROWS); i++) {
		if (strcmp((char *)
		    xv_get(ip->HostList, PANEL_LIST_STRING, i), name) == 0) {
			asktoproceed(Hostscreen, gettext(
			    "Host `%s' is already in the list.\n\
You are only allowed to move or delete it."),
				name);
			return;
		}
	}

	hlp = host_alloc(name);
	if (passwd != (char *)0 && passwd[0] != '\0')
		hlp->h_passwd = xstrdup(passwd);
	else
		hlp->h_passwd = (char *)0;

	GetHostStatus(ip, hlp);

	(void) sprintf(hoststr, HOSTFMT,
		name,
		hlp->h_arch ? hlp->h_arch : unknown_arch,
		hlp->h_status & HOST_SELECTED ?
			gettext("SELECTED") : gettext("UNSELECTED"));

	xv_set(ip->HostList,
	    PANEL_LIST_INSERT,		row,
	    PANEL_LIST_FONT,		row,	fixed_font,
	    PANEL_LIST_STRING,		row,	hoststr,
	    PANEL_LIST_GLYPH,		row,	HostImage(hlp),
	    PANEL_LIST_CLIENT_DATA,	row,	hlp,
	    PANEL_LIST_SELECT,		row,	TRUE,
	    NULL);

	SetPwdStatus(ip);
}

void
#ifdef __STDC__
DeleteHost(void)
#else
DeleteHost()
#endif
{
	Props_PropsWin_objects *ip = Props_PropsWin;
	int	row = (int)xv_get(ip->HostList, PANEL_LIST_FIRST_SELECTED);
	int	nrows = (int)xv_get(ip->HostList, PANEL_LIST_NROWS);
	Hostlist *hlp;

	if (row == -1) {
		asktoproceed(Hostscreen, gettext(
			"You have not selected a host."));
		return;
	}

	hlp = (Hostlist *)xv_get(ip->HostList, PANEL_LIST_CLIENT_DATA, row);
	if (strcmp(hlp->h_name, thishost) == 0) {
		asktoproceed(Hostscreen, gettext(
		    "Sorry, you cannot remove the\nlocal host from the list."));
		return;
	}
	host_remove(hlp);

	xv_set(ip->HostList, PANEL_LIST_DELETE, row, NULL);

	/*
	 * Just deleted a row.  If we deleted the
	 * last row and there are rows left, select
	 * the new last row.  If we deleted something
	 * other than the last row, select the new
	 * occupant of the deleted position.
	 */
	if (--nrows > 0) {
		if (row >= nrows)
			xv_set(ip->HostList,
				PANEL_LIST_SELECT, nrows - 1, TRUE, NULL);
		else
			xv_set(ip->HostList,
				PANEL_LIST_SELECT, row, TRUE, NULL);
	}
}

void
#ifdef __STDC__
ChangeHost(void)
#else
ChangeHost()
#endif
{
	Props_PropsWin_objects *ip = Props_PropsWin;
	register int i;
	char	*name = (char *)xv_get(ip->HostName, PANEL_VALUE);
	char	*passwd = (char *)xv_get(ip->HostPwd, PANEL_VALUE);
	int	row = (int)xv_get(ip->HostList, PANEL_LIST_FIRST_SELECTED);
	char	*entry;
	char	hoststr[1024];
	Hostlist *hlp;

	if (row == -1) {
		asktoproceed(Hostscreen, gettext(
			"To change an entry, select it from\n"
			"the list, enter its new name or\n"
			"password, then use the Edit\n"
			"function \"Modify Entry\"."));
		return;
	}

	hlp = (Hostlist *)xv_get(ip->HostList, PANEL_LIST_CLIENT_DATA, row);

	if (strcmp(hlp->h_name, thishost) == 0) {
		asktoproceed(Hostscreen, gettext(
		    "Sorry, you cannot change the\nentry for the local host."));
		return;
	}

	if (name == (char *)0 || name[0] == '\0' &&
	    passwd == (char *)0 && passwd[0] == '\0') {
		asktoproceed(Hostscreen, gettext(
		    "First enter a host name or\n"
		    "password, then use the Edit\n"
		    "function \"Modify Entry\"."));
		return;
	}

	for (i = 0; i < (int)xv_get(ip->HostList, PANEL_LIST_NROWS); i++) {
		entry = (char *)xv_get(ip->HostList, PANEL_LIST_STRING, i);
		if (strcmp(entry, name) == 0 && i != row) {
			asktoproceed(Hostscreen, gettext(
			    "Host `%s' is already in the list.\n"
			    "You are only allowed to delete it."),
				name);
			return;
		}
	}

	if (strcmp(hlp->h_name, name) == 0)
		free(hlp->h_passwd);
	else {
		host_remove(hlp);
		hlp = host_alloc(name);
	}
	if (passwd != (char *)0 && passwd[0] != '\0')
		hlp->h_passwd = xstrdup(passwd);
	else
		hlp->h_passwd = (char *)0;

	GetHostStatus(ip, hlp);

	(void) sprintf(hoststr, HOSTFMT,
		name,
		hlp->h_arch ? hlp->h_arch : unknown_arch,
		hlp->h_status & HOST_SELECTED ?
			gettext("SELECTED") : gettext("UNSELECTED"));

	xv_set(ip->HostList,
	    PANEL_LIST_FONT,		row,	fixed_font,
	    PANEL_LIST_STRING,		row,	hoststr,
	    PANEL_LIST_GLYPH,		row,	HostImage(hlp),
	    PANEL_LIST_CLIENT_DATA,	row,	hlp,
	    NULL);

	SetPwdStatus(ip);
}

static Xv_opaque passwd_image;
static u_short passwd_bits[] = {
#include "icons/Interdit.icon"
};

static Xv_opaque up_image;
static u_short up_bits[] = {
#include "icons/Up.icon"
};

static Xv_opaque down_image;
static u_short down_bits[] = {
#include "icons/Down.icon"
};

static Xv_opaque unknown_image;
static u_short unknown_bits[] = {
#include "icons/Question.icon"
};

/*
 * Create/return host status glyph to
 * include in host list.
 */
static Xv_opaque
HostImage(host)
	Hostlist *host;
{
	Xv_opaque	*imagep;
	u_short		*bits;

	if ((host->h_status & HOST_PWDREQ) &&
	    (host->h_status & HOST_PWDOK) == 0) {
		imagep = &passwd_image;
		bits = passwd_bits;
	} else if (host->h_type == unknown || host->h_type == error) {
		imagep = &unknown_image;
		bits = unknown_bits;
	} else if ((host->h_status & HOST_UP) == 0) {
		imagep = &down_image;
		bits = down_bits;
	} else {
		imagep = &up_image;
		bits = up_bits;
	}

	if (*imagep == (Xv_opaque)0) {
		*imagep = xv_create(XV_NULL, SERVER_IMAGE,
		    SERVER_IMAGE_DEPTH, 1,
		    SERVER_IMAGE_BITS, bits,
		    XV_WIDTH, 16,
		    XV_HEIGHT, 16,
		    NULL);
	}

	return (*imagep);
}

static void
GetHostStatus(Props_PropsWin_objects *ip, Hostlist *host)
{
	char	footer[1024];

	/*
	 * i18n:  footer message for Remote Hosts screen
	 */
	(void) sprintf(footer, gettext("Checking access on `%s'.\n"),
		host->h_name);
	xv_set(ip->PropsWin, FRAME_LEFT_FOOTER, footer, NULL);

	switch (check_host_info(host)) {
	case ERR_NOPASSWD:
		asktoproceed(Hostscreen, gettext(
		    "In order to install or remove software\n"
		    "on `%s', you must know its root\n"
		    "password.  If you know this password,\n"
		    "enter it in the \"Root Password\" field,\n"
		    "then use the Edit function \"Modify Entry\"."),
			host->h_name);
		break;
	case ERR_INVPASSWD:
		asktoproceed(Hostscreen, gettext(
		    "In order to install or remove software\n"
		    "on `%s', you must know its root\n"
		    "password.  The current value for this\n"
		    "password is incorrect.  If you know the\n"
		    "correct password, enter it in the \"Root\n"
		    "Password\" field, then use the Edit\n"
		    "function \"Modify Entry\"."),
			host->h_name);
		break;
	case SUCCESS:
	default:
		break;
	}

	xv_set(ip->PropsWin, FRAME_LEFT_FOOTER, "", NULL);
}

static void
SetPwdStatus(Props_PropsWin_objects *ip)
{
	char	*passwd = (char *)xv_get(ip->HostPwd, PANEL_VALUE);
	int	isinactive = (int)xv_get(ip->HostPwdClear, PANEL_INACTIVE);

	if (passwd != (char *)0 && passwd[0] != '\0') {
		if (isinactive)
			xv_set(ip->HostPwdClear, PANEL_INACTIVE, FALSE, NULL);
	} else {
		if (isinactive == 0)
			xv_set(ip->HostPwdClear, PANEL_INACTIVE, TRUE, NULL);
	}
}

void
ToggleHost(Xv_opaque instance)
{
	Props_PropsWin_objects *ip = (Props_PropsWin_objects *)instance;
	char	hoststr[1024];
	int	row = (int)xv_get(ip->HostList, PANEL_LIST_FIRST_SELECTED);
	Hostlist *hlp =
		(Hostlist *)xv_get(ip->HostList, PANEL_LIST_CLIENT_DATA, row);

	switch (host_select(hlp)) {
	case ERR_NOPASSWD:
	case ERR_INVPASSWD:
		asktoproceed(Hostscreen, gettext(
		    "Host `%s' cannot be selected at\n"
		    "this time.  You must configure an entry\n"
		    "either in its /etc/hosts.equiv or /.rhosts\n"
		    "files allowing root access from `%s',\n"
		    "or enter its root password using the \"Modify\n"
		    "Entry\" function.  After executing either\n"
		    "step, select `%s' again."),
			hlp->h_name, thishost, hlp->h_name);
		break;
	case ERR_FSTYPE:
	default:
		asktoproceed(Hostscreen, gettext(
		    "Host `%s' cannot be selected at\n"
		    "this time because its file system configuration\n"
		    "cannot be determined.  The most common reason\n"
		    "for this failure is the remote host is running\n"
		    "SunOS 4.x.  You may only select hosts running\n"
		    "Solaris 2.0-based OS releases.\n"),
			hlp->h_name);
		break;
	case ERR_HOSTDOWN:
		asktoproceed(Hostscreen, gettext(
		    "Host `%s' cannot be selected at\n"
		    "this time because it appears to be down.\n"
		    "Try selecting it again at a later time.\n"),
			hlp->h_name);
		break;
	case SUCCESS:
		break;
	}

	(void) sprintf(hoststr, HOSTFMT,
		hlp->h_name,
		hlp->h_arch ? hlp->h_arch : unknown_arch,
		hlp->h_status & HOST_SELECTED ?
			gettext("SELECTED") : gettext("UNSELECTED"));

	xv_set(ip->HostList,
	    PANEL_LIST_FONT,		row,	fixed_font,
	    PANEL_LIST_STRING,		row,	hoststr,
	    NULL);
}

void
SetSelectedHost(Xv_opaque instance,
	Panel_item	item,
	int		row,
	Xv_opaque	client_data)
{
	Props_PropsWin_objects *ip = (Props_PropsWin_objects *)instance;
	Hostlist *hlp = (Hostlist *)client_data;

	xv_set(item, PANEL_LIST_SELECT, row, TRUE, NULL);
	xv_set(ip->HostName, PANEL_VALUE, hlp->h_name, NULL);
	xv_set(ip->HostPwd, PANEL_VALUE, hlp->h_passwd, NULL);
	SetPwdStatus(ip);
}
