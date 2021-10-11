#ifndef lint
#pragma ident "@(#)rfs_util.c 1.29 96/06/21 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	rfs_util.c
 * Group:	ttinstall
 * Description:
 */

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/bitmap.h>
#include <sys/fs/ufs_fs.h>

#include <sys/utsname.h>
#include <netdb.h>
#include <netdir.h>
#include <libintl.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"
#include "rfs_util.h"

#include "v_types.h"
#include "v_check.h"
#include "v_rfs.h"
#include "v_sw.h"

static char *_do_select_export(WINDOW *, char *);
static void _test_mount(WINDOW * w, int idx, RFS_t);
static char *_show_exports(WINDOW *, char *, char *);
static int _check_rfs_spec(WINDOW *, int *, RFS_t);

#define	REMOTE_EXPORTS_ERR1_TITLE	gettext(\
	"Server Name and IP Address?")

#define	REMOTE_EXPORTS_ERR1	gettext(\
	"You must specify both the server name and its IP address before "\
	"the exported file systems can be determined.")

#define	REMOTE_EXPORTS_STATUS_MSG1	gettext(\
	" Looking up exported file systems... please wait...")

#define	REMOTE_EXPORTS_ERR2_TITLE	gettext(\
	"No exports")

#define	REMOTE_EXPORTS_ERR2	gettext(\
	"The server you've selected is not currently exporting any "\
	"mountable file systems.")

static char *
_show_exports(WINDOW * parent, char *ipaddr, char *server)
{

	if ((server == (char *) NULL || server[0] == '\0') ||
	    (ipaddr == (char *) NULL || ipaddr[0] == '\0')) {

		(void) simple_notice(parent, F_OKEYDOKEY,
			REMOTE_EXPORTS_ERR1_TITLE,
			REMOTE_EXPORTS_ERR1);

		return ((char *) NULL);
	}
	wstatus_msg(parent, REMOTE_EXPORTS_STATUS_MSG1);
	v_clear_export_fs();

	if (v_init_server_exports(ipaddr) == V_FAILURE) {

		(void) simple_notice(parent, F_OKEYDOKEY,
			REMOTE_EXPORTS_ERR2_TITLE,
			REMOTE_EXPORTS_ERR2);

		wclear_status_msg(parent);
		return ((char *) NULL);

	} else {

		wclear_status_msg(parent);
		return (_do_select_export(parent, server));
	}

}


/*ARGSUSED1*/
static char *
_do_select_export(WINDOW * parent, char *srvr)
{
	WINDOW *win = (WINDOW *) NULL;
	int i;
	int nexports;
	int ch;

	int row;		/* first row of menu */
	char **opts;
	unsigned long fkeys;

	int selected;
	HelpEntry _help;

	/* load up array of choices */
	nexports = v_get_n_exports();
	opts = (char **) xcalloc(nexports * sizeof (char *));

	for (i = 0; i < nexports; i++) {
		opts[i] = (char *) v_get_export_fs_name(i);
	}

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);

	_help.win = win;
	_help.type = HELP_REFER;
	_help.title = "Server's Exportable File Systems Screen";

	(void) werase(win);
	(void) wclear(win);

	wheader(win, REMOTE_EXPORTS_TITLE);
	row = HeaderLines;
	row = wword_wrap(win, row, INDENT0, COLS - (2 * INDENT0),
	    REMOTE_EXPORTS_ONSCREEN_HELP);
	++row;

	fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;
	wfooter(win, fkeys);
	selected = -1;

	/* display options */
	ch = wmenu(win, row, INDENT1, LINES - row - FooterLines - 1,
	    COLS - INDENT1,
	    show_help, (void *) &_help,
	    (Callback_proc *) NULL, (void *) NULL,
	    (Callback_proc *) NULL, (void *) NULL,
	    NULL, opts, nexports, (void *) &selected,
	    M_RADIO,
	    fkeys);

	if (win != (WINDOW *) NULL) {
		(void) delwin(win);
		(void) touchwin(parent);
		(void) wnoutrefresh(parent);
	}
	if (opts)
		free((void *) opts);

	if (is_continue(ch) && selected != -1 && selected < nexports)
		return (v_get_export_fs_name(selected));
	else
		return ((char *) NULL);
}

#define	RFS_NO_MOUNT_POINT_ERR  gettext(\
	"You must specify a local mount point...")

#define	RFS_NO_SERVER_ERR	gettext(\
	"You must specify a file server name for the %s file system...")

#define	RFS_NO_IPADDR_ERR	gettext(\
	"You must provide an IP address for server %s...")

#define	RFS_NO_REMOTE_PATH_ERR	gettext(\
	"You must specify a remote file system to mount from server %s...")

static int
_check_rfs_spec(WINDOW * w, int *cur, RFS_t rfs)
{
	int ret;
	char buf[BUFSIZ];

	ret = 1;

	if (rfs.mnt_pt[0] == '\0') {	/* prompt for mount point */

		(void) simple_notice(w, F_OKEYDOKEY, TITLE_WARNING,
		    RFS_NO_MOUNT_POINT_ERR);

		ret = 0;
		*cur = 0;

	} else if (rfs.server[0] == '\0') {	/* prompt for server */

		(void) sprintf(buf, RFS_NO_SERVER_ERR, rfs.mnt_pt);
		(void) simple_notice(w, F_OKEYDOKEY, TITLE_WARNING, buf);

		ret = 0;
		*cur = 1;

	} else if (rfs.ip_addr[0] == '\0') {	/* prompt for ipaddr */

		(void) sprintf(buf, RFS_NO_IPADDR_ERR, rfs.server);
		(void) simple_notice(w, F_OKEYDOKEY, TITLE_WARNING, buf);

		ret = 0;
		*cur = 2;

	} else if (rfs.server_path[0] == '\0') {	/* prompt for rfs */

		(void) sprintf(buf, RFS_NO_REMOTE_PATH_ERR, rfs.server);
		(void) simple_notice(w, F_OKEYDOKEY, TITLE_WARNING, buf);

		ret = 0;
		*cur = 3;

	}
	return (ret);
}

int
get_rfs_spec(WINDOW * w, char *title, char *text, RFS_t * rfs, int *index,
    int fkeys, HelpEntry help)
{
	char *ipaddr;
	char fmt_str[25];
	char *cp;

	int dirty = 1;
	int nflds = N_RFS_FIELDS;
	int ch;
	int cur;
	int row;
	int i;
	int j;

	static _RFS_row_t *form = (_RFS_row_t *) NULL;

	if (form == (_RFS_row_t *) NULL)
		form = (_RFS_row_t *) xmalloc(N_RFS_FIELDS *
		    sizeof (_RFS_row_t));

	/* first field is local mount point */
	j = 0;
	/* i18n: 24 chars max */
	form[j].f[0].label = gettext("Local mount point");
	form[j].f[0].loc.c = 0;
	form[j].f[0].len = 24;
	form[j].f[0].maxlen = 24;
	form[j].f[0].type = INSENSITIVE;

	form[j].f[1].label = rfs->mnt_pt;
	form[j].f[1].loc.c = 26;
	form[j].f[1].len = 50;
	form[j].f[1].maxlen = MAXMNTLEN;
	form[j].f[1].type = LSTRING;

	/* second field is server's name */
	++j;
	/* i18n: 24 chars max */
	form[j].f[0].label = gettext("Server's host name");
	form[j].f[0].loc.c = 0;
	form[j].f[0].len = 24;
	form[j].f[0].maxlen = 24;
	form[j].f[0].type = INSENSITIVE;

	form[j].f[1].label = rfs->server;
	form[j].f[1].loc.c = 26;
	form[j].f[1].len = 50;
	form[j].f[1].maxlen = 257;
	form[j].f[1].type = LSTRING;

	/* third field is server's ip addr */
	++j;
	/* i18n: 24 chars max */
	form[j].f[0].label = gettext("Server's IP address");
	form[j].f[0].loc.c = 0;
	form[j].f[0].len = 24;
	form[j].f[0].maxlen = 24;
	form[j].f[0].type = INSENSITIVE;

	form[j].f[1].label = rfs->ip_addr;
	form[j].f[1].loc.c = 26;
	form[j].f[1].len = 16;
	form[j].f[1].maxlen = 16;
	form[j].f[1].type = LSTRING;

	/* fourth field is file system exported from server */
	++j;
	/* i18n: 24 chars max */
	form[j].f[0].label = gettext("File system path");
	form[j].f[0].loc.c = 0;
	form[j].f[0].len = 24;
	form[j].f[0].maxlen = 24;
	form[j].f[0].type = INSENSITIVE;

	form[j].f[1].label = rfs->server_path;
	form[j].f[1].loc.c = 26;
	form[j].f[1].len = 50;
	form[j].f[1].maxlen = MAXMNTLEN;
	form[j].f[1].type = LSTRING;

	if (*index == -1) {

		/*
		 * create a new remote file system, this new rfs is deleted
		 * if the add is cancelled.
		 */
		*index = v_get_n_rfs();
		(void) v_new_rfs(form[1].f[1].label, form[2].f[1].label,
		    form[3].f[1].label, form[0].f[1].label);

		(void) v_set_rfs_test_status(*index, V_NOT_TESTED);
	}
	(void) werase(w);
	(void) wclear(w);

	wheader(w, title);

	row = HeaderLines;
	row = wword_wrap(w, row, INDENT0, COLS - (2 * INDENT0), text);

	(void) mvwprintw(w, row, INDENT0, "%.*s", COLS - (2 * INDENT0),
		EQUALS_STR);
	row += 2;

	/* set rows on each field */
	for (i = 0; i < N_RFS_FIELDS; i++) {
		form[i].f[0].loc.r = row;
		form[i].f[1].loc.r = row;
		++row;
	}

	cur = 0;

	for (;;) {

		if (dirty == 1) {
			for (j = 0; j < nflds; j++) {

				(void) mvwprintw(w, form[j].f[0].loc.r,
				    form[j].f[0].loc.c, "%*.*s:",
				    form[j].f[0].maxlen, form[j].f[0].maxlen,
				    form[j].f[0].label);

				(void) sprintf(fmt_str, "%%-%d.%ds%%s",
				    form[j].f[1].len - 1, form[j].f[1].len - 1);

				(void) mvwprintw(w, form[j].f[1].loc.r,
				    form[j].f[1].loc.c, fmt_str,
				    form[j].f[1].label,
				    (((int) strlen(form[j].f[1].label) >
					    form[j].f[1].len) ? ">" : ""));
			}

			wnoutrefresh(w);
			wfooter(w, fkeys);
			dirty = 0;
		}
		ch = wget_field(w, form[cur].f[1].type,
		    form[cur].f[1].loc.r, form[cur].f[1].loc.c,
		    form[cur].f[1].len, form[cur].f[1].maxlen,
		    form[cur].f[1].label, fkeys);

		(void) mvwprintw(w, form[cur].f[1].loc.r,
		    form[cur].f[1].loc.c, "%-*.*s%s",
		    form[cur].f[1].len - 1, form[cur].f[1].len - 1,
		    form[cur].f[1].label,
		    (int) strlen(form[cur].f[1].label) >
		    (form[cur].f[1].len - 1) ? ">" : " ");

		(void) wnoutrefresh(w);

		if (fwd_cmd(ch) != 0 || bkw_cmd(ch) != 0 || ch == RETURN) {

			wclear_status_msg(w);

			/* move forward or backward */
			if ((cur % N_RFS_FIELDS) == 1) {

				/* server name field */
				if (check_hostname(w,
					form[cur].f[1].label) == 0)
					continue;

				/* lookup IP Address */
				if ((form[cur].f[1].label != (char *) NULL) &&
				    (form[cur].f[1].label[0]) != '\0' &&
				    (form[cur + 1].f[1].label !=
					(char *) NULL)) {
					ipaddr = v_ipaddr_from_hostname(
					    form[cur].f[1].label);

					if (ipaddr == (char *) NULL ||
					    *ipaddr == '\0') {
						wstatus_msg(w,
						    REMOTE_NEED_IPADDR);
						ipaddr = "";
					}
					(void) strcpy(form[cur + 1].f[1].label,
					    ipaddr);
				}
				dirty = 1;

			}
			if ((cur % N_RFS_FIELDS) == 2) {

				/* server ipaddr field */
				if (check_ipaddr(w,
					form[cur].f[1].label) == 0)
					continue;
			}

			if (((cur % N_RFS_FIELDS) == 0) ||
			    ((cur % N_RFS_FIELDS) == 3)) {

				/* path name field */
				if (check_mountpt(w,
					form[cur].f[1].label) == 0)
					continue;
			}

			if (fwd_cmd(ch) != 0 || ch == RETURN) {
				++cur;

				if (cur == nflds) {

					cur = 0;

				}
			} else {	/* backwards */

				cur = (cur + nflds) % (nflds + 1);

				if (cur == nflds)
					cur = nflds - 1;

			}

		} else if (is_continue(ch) != 0) {

			if (_check_rfs_spec(w, &cur, *rfs) == 1)
				break;

			dirty = 1;

		} else if (is_goback(ch) != 0 && fkeys & F_GOBACK) {

			break;

		} else if (is_cancel(ch) != 0 && fkeys & F_CANCEL) {

			break;

		} else if (is_exit(ch) != 0 && fkeys & F_EXIT) {

			break;

		} else if (is_showexports(ch) != 0) {

			/*
			 * show file systems exported by server@ipaddr
			 */
			if (((cp = _show_exports(w, form[2].f[1].label,
			    form[1].f[1].label)) != (char *) NULL) &&
			    (*cp != '\0')) {
				(void) strcpy(form[3].f[1].label, cp);

				dirty = 1;
				continue;
			}
		} else if (is_testmount(ch) != 0 && fkeys & F_TESTMOUNT) {

			if (_check_rfs_spec(w, &cur, *rfs) == 1) {

				_test_mount(w, *index, *rfs);

			}
		} else if (is_help(ch) != 0) {

			do_help_index(w, help.type, help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else
			beep();
	}

	if (is_continue(ch) != 0) {

		(void) v_set_rfs_mnt_pt(*index, rfs->mnt_pt);
		(void) v_set_rfs_server(*index, rfs->server);
		(void) v_set_rfs_ip_addr(*index, rfs->ip_addr);
		(void) v_set_rfs_server_path(*index, rfs->server_path);
		return (1);

	} else if ((is_cancel(ch) != 0 && fkeys & F_CANCEL) ||
	    (is_exit(ch) != 0 && fkeys & F_EXIT)) {

		werase(w);
		wclear(w);
		wnoutrefresh(w);
		return (0);

	} /* else if (is_goback(ch) != 0 && fkeys & F_GOBACK) */ {

		werase(w);
		wclear(w);
		return (-1);

	}

}

static void
_test_mount(WINDOW * w, int idx, RFS_t rfs)
{
	char buf[BUFSIZ];
	char buf1[BUFSIZ];
	int newidx;

	newidx = v_get_n_rfs();

	(void) v_new_rfs(rfs.server, rfs.ip_addr, rfs.server_path, rfs.mnt_pt);

	wstatus_msg(w, gettext(" Test mounting %s... "),
	    v_get_rfs_mnt_pt(newidx));

	if (v_test_rfs_mount(newidx) != V_OK) {

		wclear_status_msg(w);

		(void) strcpy(buf,
		    gettext("Test mount of %s from %s failed."));
		(void) sprintf(buf1, buf, v_get_rfs_server_path(newidx),
		    v_get_rfs_server(newidx));

		(void) simple_notice(w, F_OKEYDOKEY,
			REMOTE_TEST_MOUNT_FAILED_TITLE,
			buf1);

		(void) v_set_rfs_test_status(idx, V_TEST_FAILURE);

	} else {

		(void) strcpy(buf,
		    gettext("Test mount of %s from %s was successful."));

		(void) sprintf(buf1, buf, v_get_rfs_server_path(newidx),
		    v_get_rfs_server(newidx));

		(void) simple_notice(w, F_OKEYDOKEY,
			gettext("Test Mount Successful"),
			buf1);

		(void) v_set_rfs_test_status(idx, V_TEST_SUCCESS);
		wclear_status_msg(w);
	}

	(void) v_delete_rfs(newidx);
}
