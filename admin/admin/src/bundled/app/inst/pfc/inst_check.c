#ifndef lint
#pragma ident "@(#)inst_check.c 1.47 96/06/21 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_check.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <limits.h>
#include <locale.h>
#include <libintl.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"
#include "v_types.h"
#include "v_check.h"
#include "v_sw.h"
#include "v_disk.h"

static char *_get_disk_err_buf(void);

int
show_sw_depends()
{
	WINDOW *win;

	int i;
	int row;
	int ch;
	unsigned long fkeys;
	HelpEntry _help;

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);
	(void) werase(win);
	(void) wclear(win);

	_help.win = win;
	_help.type = HELP_HOWTO;
	_help.title = "Resolve Software Dependencies";

	wheader(win, TITLE_WARNING);

	row = HeaderLines;
	row = wword_wrap(win, row, INDENT0, COLS - (2 * INDENT0),
	    SW_DEPEND_ONSCREEN_HELP);
	row += 2;

	(void) mvwprintw(win, row, 4, "%-36.36s  %-36.36s",
	/* i18n: 36 chars max */
	    gettext("Selected package"),
	/* i18n: 36 chars max */
	    gettext("Depends on package"));

	++row;
	(void) mvwprintw(win, row, 4, "%.*s", COLS - (2 * INDENT0), EQUALS_STR);
	++row;

	for (i = 0; i < v_get_n_depends() && row < LINES - 5; i++, row++) {

		(void) mvwprintw(win, row, 4, "%-36.36s  %-36.36s",
		    v_get_depends_pkgname(i), v_get_dependson_pkgname(i));
	}

	fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;
	beep();

	for (;;) {

		wfooter(win, fkeys);
		wcursor_hide(win);
		ch = wzgetch(win, fkeys);

		if (is_ok(ch) != 0) {

			break;

		} else if (is_cancel(ch) != 0) {

			break;

		} else if (is_help(ch) != 0) {

			do_help_index(win, _help.type, _help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else
			beep();

	}

	(void) delwin(win);

	if (is_ok(ch) != 0)
		return (0);	/* really done  */
	else {
		return (1);	/* continue editing */
	}

}


int
show_small_part()
{
	WINDOW *win;

	int i;
	int row;
	int ch;
	unsigned long fkeys;

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);
	(void) werase(win);
	(void) wclear(win);

	wheader(win, TITLE_WARNING);

	row = HeaderLines;
	row = wword_wrap(win, row, INDENT0, COLS - (2 * INDENT0),
	    SMALL_PARTITION_ONSCREEN_HELP);
	row++;

	(void) mvwprintw(win, row, 4, "%-35.34s%-14.14s  %-15.14s",
	/* i18n: 30 chars max */
	    gettext("Undersized File Systems"),
	/* i18n: 12 chars max */
	    gettext("Configured"),
	/* i18n: 13 chars max */
	    gettext("Required"));

	++row;
	(void) mvwprintw(win, row, 4, "%-.67s", DASHES_STR);
	++row;

	v_set_disp_units(V_MBYTES);
	for (i = 0;
	    i < v_get_n_small_filesys() && row < LINES - 5;
	    i++, row++) {

		(void) mvwprintw(win, row, 4, "%-35.32s%10.10s %s %10.10s %s",
		    v_get_small_filesys(i),
		    v_get_small_filesys_avail(i), v_get_disp_units_str(),
		    v_get_small_filesys_reqd(i), v_get_disp_units_str());
	}

	fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;
	beep();

	for (;;) {

		wfooter(win, fkeys);
		wcursor_hide(win);
		ch = wzgetch(win, fkeys);

		if (is_ok(ch) != 0) {

			break;

		} else if (is_cancel(ch) != 0) {

			break;

		} else if (is_escape(ch) != 0) {

			continue;

		} else if (is_help(ch) != 0) {

			do_help_index(win, HELP_NONE, (char *) NULL);

		} else
			beep();

	}

	(void) delwin(win);
	(void) clearok(curscr, TRUE);
	(void) touchwin(stdscr);
	(void) wnoutrefresh(stdscr);
	(void) clearok(curscr, FALSE);

	if (is_cancel(ch) != 0)
		return (0);	/* cancel install */
	else
		return (1);	/* continue with install */

}

int
check_hostname(WINDOW * w, char *name)
{
	char buf[BUFSIZ];


	if (*name == NULL)
		return (1);

	if (v_valid_hostname(name) != 1) {

		(void) sprintf(buf, BAD_HOSTNAME, 2, MAXHOSTNAMELEN);

		(void) simple_notice(w, F_OKEYDOKEY, BAD_HOSTNAME_TITLE, buf);

		return (0);
	}
	return (1);
}

int
check_mountpt(WINDOW * w, char *mountpt)
{

	if (*mountpt == NULL)
		return (1);

	if (v_valid_filesys_name(mountpt) != 1) {
		(void) simple_notice(w, F_OKEYDOKEY, BAD_MOUNTPT_NAME_TITLE,
		    BAD_MOUNTPT_NAME);

		return (0);
	}
	return (1);
}

int
check_ipaddr(WINDOW * w, char *ipaddr)
{

	if (*ipaddr == NULL)
		return (1);

	if (v_valid_host_ip_addr(ipaddr) != 1) {
		(void) simple_notice(w, F_OKEYDOKEY, BAD_IPADDR_TITLE,
		    BAD_IPADDR);

		return (0);
	}
	return (1);
}

int
show_disk_warning()
{

	(void) simple_notice(stdscr, F_OKEYDOKEY, DISK_CONFIG_ERROR_TITLE,
	    _get_disk_err_buf());

	return (0);
}

/*
 * _get_disk_err_buf()
 *
 * function: this is only called in response to a chk_disk() call which
 * 	produced an error code.  We want to create a
 * 	reasonably detailed error message containing the disk library's
 * 	abbreviated message.  Also want to include some ideas for how
 * 	to resolve the problem.
 * returns:  pointer to a static buffer containing message
 */
static char *
_get_disk_err_buf(void)
{

	static char buf[BUFSIZ];

	switch (v_get_v_errno()) {
	case V_OFF:
		(void) sprintf(buf, "%s", SDISK_ERR_OFFEND);
		break;

	case V_ZERO:
		(void) sprintf(buf, "%s", SDISK_ERR_ZERO);
		break;

	case V_OVER:
		(void) sprintf(buf, "%s", SDISK_ERR_OVERLAP);
		break;

	case V_DUPMNT:
		(void) sprintf(buf, "%s", SDISK_ERR_DUPMNT);
		break;

	case V_BOOTCONFIG:
		(void) sprintf(buf, "%s", DISK_ERR_BOOTCONFIG);
		break;

	case V_NODISK_SELECTED:
		(void) sprintf(buf, "%s", DISK_ERR_NODISK_SELECTED);
		break;

	case V_NO_ROOTFS:
		(void) sprintf(buf, "%s", DISK_ERR_NOROOT_FS);
		break;

	case V_ILLEGAL:
	case V_ALTSLICE:
	case V_BOOTFIXED:
	case V_NOTSELECT:
	case V_LOCKED:
	case V_GEOMCHNG:
	case V_NOGEOM:
	case V_NOFIT:
	case V_NOSOLARIS:
	case V_BADORDER:
	case V_SMALLSWAP:
	default:
		(void) sprintf(buf, "%s", v_sdisk_get_err_buf());
		break;

	}

	return (buf);
}
