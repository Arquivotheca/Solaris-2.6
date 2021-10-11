#ifndef lint
#pragma ident "@(#)inst_fs_auto.c 1.28 96/09/17 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_fs_auto.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <libintl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/bitmap.h>

#include "pf.h"
#include "tty_pfc.h"
#include "v_types.h"
#include "v_check.h"
#include "v_disk.h"
#include "v_lfs.h"
#include "v_sw.h"
#include "v_misc.h"

#include "inst_msgs.h"
#include "disk_fs_util.h"

int do_autolayout(WINDOW *);
static int _auto_or_manual_disk_config();
static void _clear_sel_disks_save_preserves(int);
static int _redo_auto_disk_config();
static int _check_bootdrive_location(void);
static int _choose_autolayout_filesys(void);
static int _reset_fs_status;

/*
 * returns:
 *	-1:	goback
 *	 0:	exit
 *	 1:	continue
 *	 2:	skip to disk menu
 *
 */
parAction_t
do_fs_autoconfig()
{

	int step;
	int prev;
	int ret;
	parAction_t retcode;
	int done;
	int ndisks = v_get_n_disks();

	step = 1;

	done = 0;

	_reset_fs_status = 1;
	while (!done) {

		switch (step) {

		case 1:

			/* auto-layout or manual? */
			if ((ret = _auto_or_manual_disk_config()) == 1) {
				_clear_sel_disks_save_preserves(ndisks);
				step = 3;	/* auto config */
				prev = 1;
			} else if (ret == -1) {
				retcode = parAGoback;
				done = 1;	/* goback to preserve */
			} else if (ret == 0) {
				if (confirm_exit(stdscr) == 1) {
					retcode = parAExit;
					done = 1;	/* done, exit */
				} else
					continue;

			} else if (ret == 2) {

				v_set_has_auto_partitioning(FALSE);
				/* skip-auto, jump to disk edit menu */
				retcode = parAContinue;
				done = 1;
			}
			break;

		case 3:	/* auto-layout, choose which file systems */
			if ((ret = _choose_autolayout_filesys()) == 1)
				step = 4;	/* continue */
			else if (ret == -1)
				step = prev;	/* cancel/go back */
			break;

		case 4:

			/* try auto-layout of default file systems */
			_reset_fs_status = 1;
			if (do_autolayout(stdscr) == 0) {
				/* there was an autolayout problem */

				if (yes_no_notice(stdscr, F_CONTINUE | F_CANCEL,
					F_CONTINUE, F_CANCEL,
					AL_FAILED_TITLE,
					USE_DISK_INSUFFICIENT_SPACE)
					== F_CONTINUE) {

					v_set_has_auto_partitioning(TRUE);
					done = 1;
					retcode = parAContinue;

				} else {
					/*
					 * go back to Auto-Layout File
					 * System
					 */
					_reset_fs_status = 0;
					step = 3;
				}

			} else {
				v_set_has_auto_partitioning(TRUE);
				step = 5;
			}

			_commit_all_selected_disks(ndisks);

			break;

		case 5:

			/* check to see where boot drive ended up */
			if (_check_bootdrive_location() == 1) {
				done = 1;
				retcode = parAContinue;
			} else
				step = prev;	/* go back */

			break;

		}

	}

	_restore_all_selected_disks_commit(ndisks);

	return (retcode);
}

static int
_auto_or_manual_disk_config()
{
	unsigned long fkeys;
	int ch;
	int top_row;		/* first row of menu */
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = "Auto-layout";

	(void) werase(stdscr);
	(void) wclear(stdscr);

	wheader(stdscr, TITLE_AUTOLAYOUTQRY);

	top_row = HeaderLines;

	top_row = wword_wrap(stdscr, top_row, INDENT0, COLS - (2 * INDENT0),
	    MSG_AUTOLAYOUTQRY);

	fkeys = F_AUTO | F_GOBACK | F_MANUAL | F_EXIT | F_HELP;

	wfooter(stdscr, fkeys);
	wcursor_hide(stdscr);

	for (;;) {

		ch = wzgetch(stdscr, fkeys);

		if (is_auto(ch) != 0) {

			break;

		} else if (is_manual(ch) != 0) {

			break;

		} else if (is_goback(ch) != 0) {

			break;

		} else if (is_exit(ch) != 0) {

			break;

		} else if (is_help(ch) != 0) {

			do_help_index(stdscr, _help.type, _help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else
			beep();

	}

	if (is_auto(ch) != 0)
		return (1);	/* auto-config */
	else if (is_manual(ch) != 0)
		return (2);	/* manual */
	else if (is_exit(ch) != 0)
		return (0);	/* exit */
	else /* if (is_goback(ch) != 0) */
		return (-1);	/* go back to use disks */

}

static int
_redo_auto_disk_config()
{
	unsigned long fkeys;
	int ch;
	int top_row;		/* first row of menu */
	HelpEntry _help;

	int	ret;
	char    diskname[32];
	char    part_char;
	int	dev_index;
	Disk_t  *dp;
	int	slice;
	char    root_slicename[32];
	char    root_diskname[32];
	char    buf[32];
	char    diskbuf[32];


	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = "Auto-layout";

	(void) werase(stdscr);
	(void) wclear(stdscr);

	wheader(stdscr, TITLE_REDO_AUTOLAYOUT);

	top_row = HeaderLines;

	top_row = wword_wrap(stdscr, top_row, INDENT0, COLS - (2 * INDENT0),
	    MSG_REDO_AUTOLAYOUT);

	fkeys = F_CONTINUE | F_GOBACK | F_REDOAUTO | F_EXIT | F_HELP;

	wfooter(stdscr, fkeys);
	wcursor_hide(stdscr);

	for (;;) {

		ch = wzgetch(stdscr, fkeys);

		if (is_redoauto(ch) != 0) {

			break;

		} else if (is_continue(ch) != 0) {
			WALK_DISK_LIST(dp) {
				if (!disk_selected(dp))
					continue;
				WALK_SLICES(slice) {
					if (strcmp(slice_mntpnt(dp, slice),
							ROOT) == 0) {
						(void) sprintf(root_slicename,
							"%ss%d",
							disk_name(dp),
							slice);
						(void) sprintf(root_diskname,
							"%s",
							disk_name(dp));
					}
				}
			}

			/*
			 * get the name of the committed boot device
			 */
			(void) BootobjGetAttribute(CFG_COMMIT,
				BOOTOBJ_DISK, diskname,
				BOOTOBJ_DEVICE, &dev_index,
				BOOTOBJ_DEVICE_TYPE, &part_char,
				NULL);

			if ((strcmp(diskname, "") != 0) && dev_index != -1) {
				(void) sprintf(buf, "%s%c%d", diskname,
						part_char, dev_index);
			} else if ((strcmp(diskname, "") != 0) && dev_index == -1) {
				(void) sprintf(diskbuf, "%s", diskname);
			}
			if ((strcmp(diskname, root_diskname) != 0) ||
				((strcmp(diskname, root_diskname) == 0) &&
				(strcmp(root_slicename, buf) != 0))) {
				ret = BootobjDiffersQuery(root_slicename);
				if (ret == FALSE) {
					/*
					 * the user selected cancel, go back to the
					 * auto-layout query screen
					 */
				/*	return(-1); */
					(void) do_fs_autoconfig();
				} else {
					return (1);	/* auto-config */
				}
			} else {


				break;
			}

		} else if (is_goback(ch) != 0) {

			break;

		} else if (is_exit(ch) != 0) {

			break;

		} else if (is_help(ch) != 0) {

			do_help_index(stdscr, _help.type, _help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else
			beep();

	}

	if (is_redoauto(ch) != 0)
		return (1);	/* auto-config */
	else if (is_continue(ch) != 0)
		return (2);	/* manual */
	else if (is_exit(ch) != 0)
		return (0);	/* exit */
	else /* if (is_goback(ch) != 0) */
		return (-1);	/* go back to use disks */

}

static void
_clear_sel_disks_save_preserves(int n)
{
	int i;
	int j;

	/*
	 * restore last commited state on each disk. clear out any
	 * non-preserved file systems re-commit the disks
	 */
	for (i = 0; i < n; i++) {

		if (v_get_disk_selected(i) != 0) {

			v_restore_disk_commit(i);
			(void) v_set_current_disk(i);

			for (j = 0; j < N_Slices; j++) {

				if (v_get_cur_preserved(j) == 0) {

					/*
					 * not preserved, clear slice info
					 */
					(void) v_set_preserved(j, 0);
					(void) v_set_mount_pt(j, "");
					(void) v_set_start_cyl(j, 0, TRUE);
					(void) v_set_size(j, 0, TRUE);

					if (j == 2) {
						/*
						 * restore all of slice 2's
						 * state by setting the
						 * mntpt to overlap.  This
						 * is a key word recognized
						 * and handled by the disk
						 * lib.
						 *
						 * slice 2 will be reset to be
						 * a slice covering the
						 * whole disk and marked as
						 * `offlimits' to the disk
						 * lib.
						 */
						(void) v_set_mount_pt(j,
						    Overlap);
					}
				}
			}

			(void) v_commit_disk(i, V_IGNORE_ERRORS);

		}
	}
}

static int
_check_bootdrive_location(void)
{
	char *cp;
	char *dflt;
	char *configed;
	int ret;
	char buf[BUFSIZ];
	char buf1[BUFSIZ];

	ret = 1;
	dflt = v_get_default_bootdrive_name();
	configed = v_get_disk_from_lfs_name("/");

	/*
	 * is the configured bootdrive (the one with / on it) the same as
	 * the default bootdrive?
	 */
	if (dflt && dflt[0] != '\0' && configed && configed[0] != '\0' &&
	    strcmp(configed, gettext("None")) != 0) {

		/*
		 * configed has slice information: 'c0t0d0s0` need to get
		 * rid of it...
		 */
		if (cp = strrchr(configed, 's'))
			*cp = '\0';

		if (strcmp(configed, dflt) != 0) {

			(void) strcpy(buf1, AUTO_ALT_BOOTDRIVE_WARNING);
			(void) sprintf(buf, buf1, dflt, configed);

			if (yes_no_notice(stdscr, F_OKEYDOKEY | F_CANCEL,
				F_OKEYDOKEY, F_CANCEL, TITLE_WARNING,
				buf) != F_OKEYDOKEY) {

				ret = 0;
			}
		}
	}
	return (ret);
}

/*
 * internal driver for the auto-disk configuration algorithm
 */
int
do_autolayout(WINDOW *win)
{
	wstatus_msg(win, PLEASE_WAIT_STR);

	switch (v_auto_config_disks()) {

		case -2:	/* no disks!?!? */
		return (0);

		/* NOTREACHED */
		break;

	case -1:		/* not enough space */
		return (0);

		/* NOTREACHED */
		break;

	case 0:
		return (1);

		/* NOTREACHED */
		break;

	}
	wclear_status_msg(win);

	return (0);
}

/*ARGSUSED0*/
static int
_deselect_cb(void *data, void *item)
{
	char *name;
	char  buf[80];
	double	minswap = 0.0;

/* i18n: 65 characters max */
#define	AUTO_FS_REQUIRED_MOUNTPNT_ERR1 	gettext(\
	" %s is required and may not be deselected... ")
/* i18n: 65 characters max */
#define	AUTO_FS_REQUIRED_MOUNTPNT_ERR2 	gettext(\
	" %s is required and may not be deselected... ")

	name = v_get_default_fs_name((int) item);
	v_get_mntpt_req_size(name, &minswap);

	if (strcmp(name, "/") == 0) {

		(void) strcpy(buf, AUTO_FS_REQUIRED_MOUNTPNT_ERR1);
		wstatus_msg(stdscr, buf,
		    (char *) v_get_default_fs_name((int) item));

		beep();
		(void) peekch();
		wclear_status_msg(stdscr);
		return (0);
	} else if ((strcmp(name, "swap") == 0) && (minswap > 0)) {

		(void) strcpy(buf, AUTO_FS_REQUIRED_MOUNTPNT_ERR2);
		wstatus_msg(stdscr, buf,
		    (char *) v_get_default_fs_name((int) item));

		beep();
		(void) peekch();
		wclear_status_msg(stdscr);
		return (0);
	} else
		return (1);

}

static int
_choose_autolayout_filesys(void)
{
	char **opts;

	int ch;
	static int nfs;
	int i;
	int row;

	u_long selected = 0L;
	u_int fkeys;
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = "Auto-layout";

	/*
	 * Reset to the original mount list if we are coming into this
	 * screen forward thru the nornal parade route.
	 * If we have cancelled out of an autolayout error message,
	 * then we want to come back here with the same settings the
	 * screen when we left last time.  In this case, presumably the
	 * routine has already been called at least once with
	 * _reset_fs_status == 1, so that the proper default fs data has
	 * been initialized.
	 */
	if (_reset_fs_status) {
		v_restore_default_fs_table();
		nfs = v_get_n_default_fs();
	}
	opts = (char **) xcalloc(nfs * sizeof (char *));

	/*
	 * load array of choices, set selected status on any currently
	 * selected file systems
	 */
	for (i = 0; i < nfs; i++) {
		opts[i] = (char *) v_get_default_fs_name(i);

		if (v_get_default_fs_status(i) == B_TRUE) {
			BT_SET(&selected, i);
		}
	}

	(void) werase(stdscr);
	(void) wclear(stdscr);

	wheader(stdscr, TITLE_AUTOLAYOUT);

	row = HeaderLines;
	row = wword_wrap(stdscr, row, INDENT0, COLS - (2 * INDENT0),
	    MSG_AUTOLAYOUT);
	row++;

	(void) mvwprintw(stdscr, row++, INDENT2 - 4,
	/* i18n: 40 characters max */
	    gettext("File Systems for Auto-layout"));
	(void) mvwprintw(stdscr, row++, INDENT2 - 4, "%-.40s", EQUALS_STR);

	fkeys = (F_CONTINUE | F_CANCEL | F_HELP);

	wfooter(stdscr, fkeys);

	ch = wmenu(stdscr, row, INDENT1, LINES - HeaderLines - FooterLines,
	    COLS - INDENT1 - 2,
	    show_help, (void *) &_help,
	    (Callback_proc *) NULL, (void *) NULL,
	    _deselect_cb, (void *) NULL,
	    NULL, opts, nfs, &selected,
	    0,
	    fkeys);

	if (is_continue(ch) != 0) {

		/* set status on any selected file systems */
		for (i = 0; i < nfs; i++) {
			if (BT_TEST(&selected, i))
				(void) v_set_default_fs_status(i, 1);
			else
				(void) v_set_default_fs_status(i, 0);
		}

		(void) v_set_fs_defaults();
	}
	if (opts)
		free((void *) opts);

	if (is_continue(ch) != 0)
		return (1);
	else /* if (is_cancel(ch) != 0) */
		return (-1);

}
