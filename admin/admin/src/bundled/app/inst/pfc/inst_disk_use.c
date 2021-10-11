#ifndef lint
#pragma ident "@(#)inst_disk_use.c 1.72 96/09/20 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_disk_use.c
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

#define SELECT_ANSWER	3

typedef struct {
	HelpEntry help;
	NRowCol loc;
	FieldType type;
	char *label;
	char *prompt;
} _Item_Field_t;

/*
 * a row in the disk table consists of 3 items:
 *	field for select toggle
 *	field for disk name/size
 *	field for prompts
 */
typedef struct {

	_Item_Field_t fld[4];

} _Disk_Row_t;

typedef struct {
	WINDOW *w;
	int use_total;		/* total space in selected disks */
	int req_total;		/* required space */
	int r;			/* row to display Use total on */
	int c;			/* col to display Use total on */
} _total_t;

int _set_edit_choice(WINDOW *);
static int _sufficient_avail_space(WINDOW *, int);
static int _boot_disk_selected();
static void _edit_fdisk_choice(int, _total_t *, _Disk_Row_t *);
static int _edit_boot_device_choice(_Disk_Row_t *);

static int _do_disk_menu(int);
static int _unselect_disk_ok(int);
int changeBootQuery(char *);
int ndisks = -1;
static int first = TRUE;

/*
 * Entry point into disk configuration for ttinstall.
 *
 * Disk configuration consists of the following logical steps:
 *	1.	select the disk(s) to use during install
 *			(_do_disk_menu())
 *	2.	do preparation on selected disks (format/fdisK...)
 *			(prepare_disks())
 */
parAction_t
do_choose_disks()
{
	int step;
	int ret;
	parAction_t retcode;
	int done;

	/*
	 * At this point we know some disks exist because this was checked
	 * for during initial startup (see pfCheckDisks).
	 */
	ndisks = v_get_n_disks();

	v_set_n_lfs();
	v_update_lfs_space();

	step = 1;
	done = 0;

	while (!done) {

		switch (step) {

		case 1:	/* choose disks */

			_clear_all_selected_disks(ndisks);

			if ((ret = _do_disk_menu(ndisks)) == parAExit) {

				if (confirm_exit(stdscr) == 1) {
					retcode = parAExit;	/* exit */
					done = 1;
				} else
					continue;

			} else if (ret == parAGoback) {
				wstatus_msg(stdscr, PLEASE_WAIT_STR);
				retcode = parAGoback;
				done = 1;
			} else if (ret == parAContinue) {
				step = 2;	/* next step: nuf space? */
			}

			break;

		case 2:	/* enough space selected */

			if ((ret = _sufficient_avail_space(stdscr, ndisks))
				== -1)
				step = 1;	/* start over */
			else if (ret == 1) {
				retcode = parAContinue;
				done = 1;
			}
			break;

		}

	}

	return (retcode);

}

/*
 * following stuff puts up a menu of disks in use, and allows user to select
 * one to customize.
 */

static void
display_totals(_total_t * tot)
{
	char buf[128];

	/* i18n: 35 characters max */
	(void) sprintf(buf, "%s: %6d MB", gettext("Total Selected"),
	    tot->use_total);
	(void) mvwprintw(tot->w, tot->r, tot->c - strlen(buf), buf);

	/* i18n: 35 characters max */
	(void) sprintf(buf, "%s: %6d MB", gettext("Suggested Minimum"),
	    tot->req_total);
	(void) mvwprintw(tot->w, tot->r + 1, tot->c - strlen(buf), buf);

}

/*
 * free any existing disktable labels which have been strdup'ed
 */
static void
_free_disktable_labels(int ndisks, _Disk_Row_t * disktable)
{
	register int i;

	if (disktable != (_Disk_Row_t *) NULL) {

		for (i = 0; i < ndisks; i++) {

			if (disktable[i].fld[1].label != (char *) NULL)
				free((void *) disktable[i].fld[1].label);

		}

	}
}

static void
show_disktable(WINDOW * w, int max, int npp, int row,
    _Disk_Row_t * disktable, int first)
{
	register int i;		/* counts number displayed */
	register int j;		/* scratch index counter */
	register int r;		/* row counter */

	for (i = 0, r = row, j = first;
	    (i < npp) && (j < max);
	    i++, r++, j++) {

		(void) mvwprintw(w, r, INDENT1, "%s %s",
		    disktable[j].fld[0].label,
		    disktable[j].fld[1].label);

		disktable[j].fld[0].loc.r =
		    disktable[j].fld[1].loc.r =
		    disktable[j].fld[2].loc.r = (int) r;
	}

	/*
	 * clear remaining rows, i counts lines displayed, r counts row
	 * lines are displayed on
	 */
	for (; i < npp; i++, r++) {
		(void) mvwprintw(w, r, 0, "%*s", COLS, " ");
	}

}

/*
 * internal helper function!
 *
 * format a string like this from the input args: "c1t0d0 (638 MB)
 * 0 MB  (F4 to edit fdisk)"
 *
 * returns pointer to a static buffer which is overwritten on each call.
 *
 */
static char *
_fmt_label(char *diskname, int raw_cap, int solaris_cap, char *units, int boot)
{
	static char buf[256];	/* scratch pointer for sprintf() */
	char buf1[32];		/* scratch pointer for sprintf() */

	/*
	 * format a string like this from the input args: "c1t0d0 (638 MB)
	 * 0 MB  (F4 to edit fdisk)"
	 */

	/* raw disk's size */
	(void) sprintf(buf1, "%-8.8s (%d %s) %s", diskname, raw_cap, units,
	    boot ? "boot disk" : "");

	/* solaris partition size */
	(void) sprintf(buf, "%-30.30s   %5d %s", buf1, solaris_cap, units);

	return (buf);
}

/*
 * updates/changes current disk label information for disk 'cur'
 */
static void
_reset_label(_Disk_Row_t * disktable, int cur)
{
	char *cp;

	if (disktable[cur].fld[1].label != (char *) NULL)
		free(disktable[cur].fld[1].label);

	cp = _fmt_label(v_get_disk_name(cur), v_get_disk_capacity(cur),
	    v_get_sdisk_capacity(cur), v_get_disp_units_str(),
	    v_is_bootdrive(cur));

	disktable[cur].fld[1].label = xstrdup(cp);

}

static int
_do_disk_menu(int ndisks)
{
	int i;			/* scratch counter */
	int top_row;		/* first row of menu */
	int last_row;		/* last row of menu */
	int cur;		/* which disk */
	int field;		/* which field */
	int r, c;		/* cursor location */
	int	choice;
	Disk_t	*bootDisk;
	char	*name;

	int top;		/* index of first item displayed */
	int dirty = 1;
	int disks_per_page;
	int ch;
	char *cp;
	int	ret;
	int	ret_code;

	unsigned long fkeys;
	Disk_t	*boot;
	Disk_t	*dp;
	int	count;

	_total_t tot;		/* struct for total display stuff */

	_Disk_Row_t *disktable;

	tot.w = stdscr;
	tot.req_total = _get_reqd_space();
	tot.use_total = 0;

	(void) DiskobjFindBoot(CFG_CURRENT, &boot);

	 /* attempt auto-selecting disks */
	 if (first) {
		 if (boot != NULL) {
			 WALK_DISK_LIST(dp) {
				 if (dp == boot)
					 DiskAutoSelect(dp);
			 }
		 }
		first = FALSE;
	 }


	/*
	 * load up table of disks
	 */
	disktable = (_Disk_Row_t *) xcalloc(ndisks * sizeof (_Disk_Row_t));

	for (i = 0; i < ndisks; i++) {

		v_set_disp_units(V_MBYTES);

		/*
		 * first field is select toggle widgey
		 */
		disktable[i].fld[0].help.win = stdscr;
		disktable[i].fld[0].help.type = HELP_TOPIC;
		disktable[i].fld[0].help.title =
		    "Device Naming Conventions";
		disktable[i].fld[0].type = LSTRING;
		disktable[i].fld[0].loc.c = INDENT1;

		if (v_get_disk_selected(i) != 0) {
			tot.use_total += v_get_sdisk_capacity(i);
			disktable[i].fld[0].label = Sel;	/* "[X]" */
			disktable[i].fld[0].prompt = USE_DISK_SELECTED_PROMPT;

		} else if (v_get_disk_status(i) == V_DISK_NOTOKAY) {
			disktable[i].fld[0].label = "[-]";
			disktable[i].fld[0].prompt = USE_DISK_BADDISK_PROMPT;
		} else {
			disktable[i].fld[0].label = Unsel;	/* "[ ]" */
			disktable[i].fld[0].prompt =
			    USE_DISK_NOT_SELECTED_PROMPT;
		}

		/*
		 * second field is disk size/name
		 */
		/* format up the disply string and save it */
		cp = _fmt_label(v_get_disk_name(i), v_get_disk_capacity(i),
		    v_get_sdisk_capacity(i), v_get_disp_units_str(),
		    v_is_bootdrive(i));

		disktable[i].fld[1].label = xstrdup(cp);
		disktable[i].fld[1].prompt = "";
		disktable[i].fld[1].type = LSTRING;
		disktable[i].fld[1].loc.c = INDENT1 + 4;
		disktable[i].fld[1].help.win = stdscr;
		disktable[i].fld[1].help.type = HELP_NONE;
		disktable[i].fld[1].help.title = "";

		/*
		 * third field is extra, where the `F4 to edit fdisk' prompt
		 * goes.
		 */
		disktable[i].fld[2].help.win = stdscr;
		disktable[i].fld[2].help.type = HELP_NONE;
		disktable[i].fld[2].help.title = "";
		disktable[i].fld[2].label = "";
		disktable[i].fld[2].prompt = "";
		disktable[i].fld[2].type = INSENSITIVE;
		disktable[i].fld[2].loc.c = INDENT1 + 4 +
		    strlen(disktable[i].fld[1].label) + 2;

	}

	(void) werase(stdscr);
	(void) wclear(stdscr);
	wheader(stdscr, TITLE_USEDISKS);

	top_row = HeaderLines;

	top_row = wword_wrap(stdscr, top_row, INDENT0, COLS - (2 * INDENT0),
	    USE_DISK_CHOOSE_DISK_ONSCREEN_HELP);

	/* print headings */
	top_row++;
	(void) mvwprintw(stdscr, top_row, INDENT1 + 4, "%-23.23s   %15.15s",
	    gettext("Disk Device (Size)"),
	    gettext("Available Space"));

	++top_row;
	(void) mvwprintw(stdscr, top_row, INDENT1, "%-.*s", 23 + 15 + 3 + 4,
	    EQUALS_STR);

	/* setup menu variables */
	++top_row;
	cur = 0;
	field = 0;
	top = 0;
	disks_per_page = LINES - top_row - FooterLines - 3;
	last_row = top_row + disks_per_page - 1;

	/* setup current use/required totals */
	if (ndisks < disks_per_page) {
		tot.r = top_row + ndisks + 1;
	} else {
		tot.r = last_row + 2;
	}
	tot.c = INDENT1 + 45;

	display_totals(&tot);

	fkeys = F_CONTINUE | F_GOBACK | F_EDIT | F_EXIT | F_HELP;

	wfooter(stdscr, fkeys);

	for (;;) {

		if (dirty) {

			(void) show_disktable(stdscr, ndisks, disks_per_page,
			    top_row, disktable, top);

			display_totals(&tot);

			scroll_prompts(stdscr, top_row, 1, top, ndisks,
			    disks_per_page);

			dirty = 0;
		}
		/*
		 * if disk is selected, and this is an x86 box and there is
		 * an existing fdisk label, drop a hint to the user as to
		 * how to get to the fdisk editor.
		 *
		 * also, make sure disk is OK before doing this....
		 */
		if (v_get_disk_status(cur) != V_DISK_NOTOKAY &&
		    v_get_disk_selected(cur) != 0 &&
		    v_fdisk_flabel_req(cur) != 0 &&
		    v_fdisk_flabel_exist(cur) != 0) {

			(void) mvwprintw(stdscr, disktable[cur].fld[2].loc.r,
			    disktable[cur].fld[2].loc.c,
			    gettext("(F4 to edit)"));
		} else {

/*			(void) wmove(stdscr, disktable[cur].fld[2].loc.r,
 *			    disktable[cur].fld[2].loc.c);
 *			(void) wclrtoeol(stdscr);
 */
			if (v_get_disk_status(cur) != V_DISK_NOTOKAY &&
						v_get_disk_selected(cur) != 0) {
				(void) mvwprintw(stdscr, disktable[cur].fld[2].loc.r,
					disktable[cur].fld[2].loc.c,
					gettext("(F4 to edit)"));
			}
		}

		/* highlight current */
		wfocus_on(stdscr, disktable[cur].fld[field].loc.r,
		    disktable[cur].fld[field].loc.c,
		    disktable[cur].fld[field].label);

		(void) getsyx(r, c);
		(void) wnoutrefresh(stdscr);
		(void) setsyx(r, c);
		(void) doupdate();

		ch = wzgetch(stdscr, fkeys);

		if ((field == 0) &&
		    ((sel_cmd(ch) != 0) || (alt_sel_cmd(ch) != 0))) {

			/*
			 * make sure unsuable disks are not selectable...
			 * this condition is caught by prepare_disk()...
			 * this may change, so comment it out...
			 *
			 * if (v_get_disk_status(cur) == V_DISK_NOTOKAY) {
			 * beep(); continue; }
			 */

			if (v_get_disk_selected(cur) == 0) {

				/*
				 * add a new disk to the `use' list
				 */
				if (prepare_disk(stdscr, cur) != 0) {

					tot.use_total +=
					    v_get_sdisk_capacity(cur);

					/*
					 * update disk's usable size in the
					 * display table, update usable
					 * space total
					 */
					_reset_label(disktable, cur);

					disktable[cur].fld[0].label = Sel;

					dirty = 1;
				} else
					(void) v_set_disk_selected(cur, 0);

			} else if (v_get_disk_selected(cur) != 0) {

				/* remove a disk from the 'use' list */
				if (_unselect_disk_ok(cur) != 0) {
					tot.use_total -=
					    v_get_sdisk_capacity(cur);

					v_restore_disk_orig(cur);
					(void) v_unconfig_disk(cur);
					(void) v_set_disk_selected(cur, 0);

					/*
					 * update disk's usable size in the
					 * display table, update usable
					 * space total
					 */
					_reset_label(disktable, cur);
					disktable[cur].fld[0].label = Unsel;
					dirty = 1;

				}
			}
		} else if (is_ok(ch) != 0 || is_continue(ch) != 0) {
			(void) BootobjCommit();
			DiskobjFindBoot(CFG_CURRENT, &bootDisk);
			if (bootDisk != NULL)
				name = disk_name(bootDisk);
			else
				name = "";
			count = 0;
			WALK_DISK_LIST(dp) {
				if (disk_selected(dp))
					count ++;
			}
			if (pfgIsBootSelected() == FALSE && count != 0) {
				write_debug(CUI_DEBUG_L1,
						"The boot disk is not selected");
				ret = changeBootQuery(name);
				if (ret == TRUE) {
					BootobjSetAttribute(CFG_CURRENT,
						BOOTOBJ_DISK, NULL,
						BOOTOBJ_DISK_EXPLICIT, 0,
						BOOTOBJ_DEVICE, -1,
						BOOTOBJ_DEVICE_EXPLICIT, 0,
						NULL);
					break;
				} else if (ret == FALSE) {
					return;
				} else if (ret == SELECT_ANSWER) {
					ret_code = _edit_boot_device_choice(disktable);
					if (ret_code == parAContinue)
						return (parAContinue);
					else
						return;

				}
			} else {

				break;
			}

		} else if (is_goback(ch) != 0) {

			break;

		} else if (is_edit(ch) != 0) {
			count = 0;
			WALK_DISK_LIST(dp) {
				if (disk_selected(dp))
					count ++;
			}
			if (count == 0) {
				beep();
				return;
			}

			if (v_fdisk_flabel_req(cur) != 0) {
				choice = _set_edit_choice(stdscr);
				if (choice == 0) {
					/* user selected edit fdisk paritions */
					write_debug(CUI_DEBUG_L1,
						"User selected Edit Fdisk Partitions.");
					(void) _edit_fdisk_choice(cur, &tot, disktable);
				} else if (choice == 1) {
					/* user selected define root location */
					write_debug(CUI_DEBUG_L1,
						"User selected Set Root Location.");
					ret_code = _edit_boot_device_choice(disktable);
					if (ret_code == parAContinue)
						return (parAContinue);	/* continue */
					else
						return;
				}
			} else {
				ret_code = _edit_boot_device_choice(disktable);
					if (ret_code == parAContinue)
						return (parAContinue);	/* continue */
					else
						return;
			}

		} else if (is_help(ch) != 0) {

			do_help_index(disktable[cur].fld[field].help.win,
			    disktable[cur].fld[field].help.type,
			    disktable[cur].fld[field].help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else if (is_exit(ch) != 0) {

			break;	/* exit */

		} else if (ch == U_ARROW || ch == D_ARROW ||
			    ch == CTRL_N || ch == CTRL_P ||
			    ch == CTRL_F || ch == CTRL_D ||
			    ch == CTRL_B || ch == CTRL_U) {

			dirty = 0;

			/* unhighlight */
			wfocus_off(stdscr, disktable[cur].fld[field].loc.r,
			    disktable[cur].fld[field].loc.c,
			    disktable[cur].fld[field].label);

			/* unhint, bleah! */
			(void) wmove(stdscr, disktable[cur].fld[2].loc.r,
			    disktable[cur].fld[2].loc.c);
			(void) wclrtoeol(stdscr);

			/* move */
			if (ch == CTRL_D) {

				/* page down */
				if ((cur + disks_per_page) < ndisks) {

					/* advance a page */
					top += disks_per_page;
					cur += disks_per_page;
					dirty = 1;

				} else if (cur < ndisks - 1) {

					/* advance to last disk */
					cur = ndisks - 1;
					top = cur - 2;
					dirty = 1;

				} else
					beep();	/* at end */

			} else if (ch == CTRL_U) {

				/* page up */
				if ((cur - disks_per_page) >= 0) {

					/* reverse a page */
					top = (top > disks_per_page ?
					    top - disks_per_page : 0);
					cur -= disks_per_page;
					dirty = 1;

				} else if (cur > 0) {

					/* back to first disk */
					top = 0;
					cur = 0;
					dirty = 1;

				} else
					beep();	/* at top */

			} else if (ch == U_ARROW || ch == CTRL_P ||
			    ch == CTRL_B) {

				if (disktable[cur].fld[field].loc.r ==
				    top_row) {

					if (top) {	/* scroll down */
						cur = --top;
						dirty = 1;
					} else
						beep();	/* very top */

				} else {
					cur--;
				}

			} else if (ch == D_ARROW || ch == CTRL_N ||
			    ch == CTRL_F) {

				if (disktable[cur].fld[field].loc.r ==
				    last_row) {

					if ((cur + 1) < ndisks) {

						/* scroll up */
						top++;
						cur++;
						dirty = 1;

					} else
						beep();	/* bottom */

				} else {

					if ((cur + 1) < ndisks) {
						cur++;
					} else
						beep();	/* last, no wrap */
				}

			}
		} else
			beep();

	}

	/* cleanup memory */
	if (disktable) {
		_free_disktable_labels(ndisks, disktable);
		free((void *) disktable);
	}

	if (is_continue(ch) != 0)
		return (parAContinue);	/* continue */
	else if (is_goback(ch) != 0)
		return (parAGoback);	/* go back to sw */
	else			/* if is_exit(ch) != 0) */
		return (parAExit);	/* exit */

}

/*
 * returns
 *	-1: start disks over
 *	1: continue
 */
static int
_sufficient_avail_space(WINDOW *win, int ndisks)
{
	int used_disks;
	int avail;
	int needed;

	int fkeys;
	int ret;
	int continue_ret;
	int cancel_ret;
	int continue_key;
	int cancel_key;

	char buf[BUFSIZ];

	needed = _get_reqd_space();
	avail = _get_avail_space(ndisks);
	used_disks = _get_used_disks(ndisks);

	/*
	 * have space required for this install (needed) have total raw
	 * space available (avail) have # of disks (ndisks) have # of disks
	 * used (used_disks)
	 *
	 * need to determine if there is sufficient space available to continue
	 * with the installation.  If not, need to determine what, if any,
	 * the possible remedies are.
	 *
	 */

	/* default is to continue on and check for preserved file systems... */
	ret = 1;

	/* however, circumstance may prevent taking that step */

	if (avail < needed) {

		/* need more space, where can it come from? */
		if (used_disks == 0 && ndisks) {

			/* no disk selected, must choose one. */

			(void) sprintf(buf, USE_DISK_INSUFFICIENT_SPACE_1);

			continue_key = F_OKEYDOKEY;
			cancel_key = F_OKEYDOKEY;
			fkeys = (F_OKEYDOKEY);
			continue_ret = -1;
			cancel_ret = -1;

		} else if (used_disks == ndisks) {

			/*
			 * all possible disks selected, need to reduce the
			 * space demands by deselecting sw or if server, can
			 * reduce clients or architectures.
			 */

			(void) strcpy(buf, USE_DISK_INSUFFICIENT_SPACE_2);

			if (v_get_system_type() == V_SERVER &&
			    (v_get_n_diskless_clients() > 0 ||
				v_get_n_cache_clients() > 0)) {

				(void) strcat(buf,
				    USE_DISK_INSUFFICIENT_SPACE_2A);
			}

			continue_key = F_OKEYDOKEY;
			cancel_key = F_OKEYDOKEY;
			fkeys = (F_OKEYDOKEY);
			continue_ret = -1;
			cancel_ret = -1;

		} else if (used_disks < ndisks) {

			/*
			 * not all possible disks selected, maybe add
			 * another one?
			 */

			(void) sprintf(buf, USE_DISK_INSUFFICIENT_SPACE_3);

			continue_key = F_OKEYDOKEY;
			cancel_key = F_OKEYDOKEY;
			fkeys = (F_OKEYDOKEY);
			continue_ret = -1;
			cancel_ret = -1;

		}

		if (yes_no_notice(win, fkeys, continue_key, cancel_key,
			TITLE_ERROR, buf) == continue_key) {
			ret = continue_ret;
		}


	}
	return (ret);

}

/*
 * see if they'd lose any edits by unselecting this disk. if so, give the
 * user an opportunity to cancel the unselect
 */
static int
_unselect_disk_ok(int disk)
{
	char buf[BUFSIZ];
	char buf1[BUFSIZ];

	if (v_get_disk_status(disk) == V_DISK_EDITED) {

		(void) strcpy(buf, USE_DISK_UNSELECT_DISK_WARNING);
		(void) sprintf(buf1, buf, v_get_disk_name(disk));

		if (yes_no_notice(stdscr, F_OKEYDOKEY | F_CANCEL, F_OKEYDOKEY,
			F_CANCEL, USE_DISK_UNSELECT_DISK_TITLE, buf1) ==
			F_OKEYDOKEY)
			return (1);	/* OK to unselect */
		else
			return (0);	/* don't unselect */
	}
	return (1);

}

static int
_boot_disk_selected()
{
	char buf[BUFSIZ];
	char buf1[BUFSIZ];

	if (v_boot_disk_selected() == 0) {

		(void) strcpy(buf1, USE_DISK_BOOTDRIVE_UNSELECTED);
		(void) sprintf(buf, buf1, v_get_default_bootdrive_name());

		if (yes_no_notice((WINDOW *) NULL, F_OKEYDOKEY | F_CANCEL,
			F_OKEYDOKEY, F_CANCEL, USE_DISK_BOOTDRIVE_UNSELECTED_TITLE,
			buf) == F_OKEYDOKEY)
			return (1);	/* OK to continue */
		else
			return (0);	/* don't continue */

	} else
		return (1);	/* OK, continue */

}

int
_set_edit_choice(WINDOW *parent)
{
	char		*opts[4];
	int		nopts;
	int		i;
	int		ch;
	int		row;
	u_long		selected;
	u_int		fkeys;
	HelpEntry	_help;
	WINDOW		*win;

	win = newwin(LINES, COLS, 0, 0);
 	_help.win = win;
	_help.type = HELP_REFER;
	_help.title = "Disk Editor Properties Screen";

	nopts = 0;
 	i = 0;
	opts[i] = xstrdup((char *) gettext("Fdisk partitions"));
	i++;
	nopts++;

	opts[i] = xstrdup((char *) gettext("Root filesystem location"));
	i++;
	nopts++;

	selected = 0;

	(void) werase(win);
	(void) wclear(win);

	wheader(win, gettext("Disk Edition Options"));

	row = HeaderLines;
	row = wword_wrap(win, row, INDENT0, COLS - (2 * INDENT0),
		gettext("What do you want to edit?"));
	++row;

	fkeys = (F_OKEYDOKEY | F_CANCEL);

	wfooter(win, fkeys);

	ch = wmenu(win, row, INDENT1, LINES - HeaderLines - FooterLines,
		COLS - INDENT1 - 2,
		show_help, (void *) &_help,
		(Callback_proc *) NULL, (void *) NULL,
		(Callback_proc *) NULL, (void *) NULL,
		NULL, opts, nopts, &selected,
		M_RADIO | M_RADIO_ALWAYS_ONE | M_CHOICE_REQUIRED,
		fkeys);


	for (i = 0; i < nopts; i++) {
		if (opts[i] != (char *) NULL)
			free((void *) opts[i]);
	}

	(void) delwin(win);

	if (parent != (WINDOW *) NULL) {
		(void) clearok(curscr, TRUE);
		(void) touchwin(parent);
		(void) wnoutrefresh(parent);
		(void) clearok(curscr, FALSE);
	}

	if (is_ok(ch) != 0 || is_continue(ch) != 0) {
		/* call appropriate action */
		if (selected < nopts)
			return (selected);
		else
			return (0);

	} else /* if (is_cancel(ch) != 0 || is_exit(ch) != 0) */ {
		/* return the default selection */
		return;
	}
}

static void
_edit_fdisk_choice(int current, _total_t *totals, _Disk_Row_t *dtable)
{
	/*
	 * make sure unsuable disks are not selectable
	 */
	if (v_get_disk_status(current) == V_DISK_NOTOKAY) {
		beep();
		/* continue; */
	}
	if (v_get_disk_selected(current) == 0) {
		(void) simple_notice(stdscr, F_OKEYDOKEY,
		    gettext("Disk Unselected"),
		    gettext("You must select the disk before you can edit its fdisk partitions."));
		/* continue; */
	}

	/*
	 * edit an existing fdisk label: handle making a
	 * used disk unusable, and an unselected disk
	 * usable.
	 *
	 * subtract out current size before doing anything,
	 * since it may be changed as part of the editing
	 * process.
	 */

	totals->use_total -= v_get_sdisk_capacity(current);
	if (edit_disk_parts(stdscr, current, 0) == 1) {

		(void) v_set_disk_selected(current, 1);
		_reset_label(dtable, current);
		dtable[current].fld[0].label = Sel;

		/*
		 * add disk's current usable size to total.
		 */
		totals->use_total += v_get_sdisk_capacity(current);

	} else {

		/*
		 * disk is unusable, it has no Solaris
		 * partition.
		 */
		if (v_get_disk_selected(current) != 0) {
			/*
			 * a previously used disk is now
			 * unusable...
			 *
			 * update its label
			 */
			(void) v_set_disk_selected(current, 0);
			_reset_label(dtable, current);
			dtable[current].fld[0].label = Unsel;
		}
	}

}

static int
_edit_boot_device_choice(_Disk_Row_t *dtable)
{
	int	ret;

	ret = do_choose_bootdisk();
	return(ret);
}

int
changeBootQuery(char *boot)
{
	UI_MsgStruct	*msg_info;
	int		answer;
	char		*msg_buf;

	msg_buf = (char *) xmalloc(strlen(NO_BOOT_WARNING) + strlen(boot) + 1);

	(void) sprintf(msg_buf, NO_BOOT_WARNING, boot);


	write_debug(CUI_DEBUG_L1, "entering changeBootQuery");

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->msg_type = UI_MSGTYPE_WARNING;
	msg_info->title = USE_DISK_BOOTDRIVE_UNSELECTED_TITLE;
	msg_info->msg = msg_buf;
	msg_info->btns[UI_MSGBUTTON_OK].button_text = UI_BUTTON_OK_STR;
	msg_info->btns[UI_MSGBUTTON_OTHER1].button_text = SELECT_STR;
	msg_info->btns[UI_MSGBUTTON_OTHER2].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = UI_BUTTON_CANCEL_STR;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;
	msg_info->default_button = UI_MSGBUTTON_CANCEL;

	/* invoke the message */
	UI_MsgFunction(msg_info);

	/* cleanup */
	UI_MsgStructFree(msg_info);

	switch (UI_MsgResponseGet()) {
	case UI_MSGRESPONSE_OK:
		answer = TRUE;
		break;
	case UI_MSGRESPONSE_OTHER1:
		answer = SELECT_ANSWER;
		break;
	case UI_MSGRESPONSE_CANCEL:
		default:
		answer = FALSE;
		break;
	}
	return (answer);

}
