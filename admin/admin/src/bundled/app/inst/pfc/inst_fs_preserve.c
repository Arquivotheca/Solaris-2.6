#ifndef lint
#pragma ident "@(#)inst_fs_preserve.c 1.61 96/06/21 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_fs_preserve.c
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
#include <sys/fs/ufs_fs.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"
#include "disk_fs_util.h"

#include "v_types.h"
#include "v_check.h"
#include "v_disk.h"
#include "v_lfs.h"
#include "v_sw.h"
#include "v_misc.h"

typedef struct {
	char *mntpt;
	int size;
	int disk;
	int slice;
	int preserve;
} _Pres_Entry_t;

static void _save_preserved(_Pres_Entry_t *, int);
static int _select_preserve(_Pres_Entry_t *, int *);
static int _preserve_query(void);
static int _sufficient_usable_space(int);
static _Pres_Entry_t *_load_preserve_table(int *);

/*
 * look for any file systems on disks to be used
 *
 * interested in three different `versions' of the disk information:
 *	orig: original state of disk, no matter what happens, this is constant
 *	comm: committed state of disk, last `saved' state (so saved edits)
 *	cur:  current state, should be the same as committed.
 *
 * when examining the disk to see what is preservable, look at `orig'.
 * when examining file systems to see which are preserved, look at `current'
 * when examining the file system *selected* (e.g., pending; to be preserved)
 *	need to look at `current' to see if `orig' state would conflict with
 *	`current'
 */
parAction_t
do_preserve_fs()
{
	_Pres_Entry_t *table = (_Pres_Entry_t *) NULL;
	int step;
	int done;
	int ret;
	parAction_t retcode;
	int n_pfs;
	int ndisks = v_get_n_disks();

	step = 1;
	done = 0;

	while (!done) {

		switch (step) {

		case 1:
			table = _load_preserve_table(&n_pfs);

			step = 2;
			break;

		case 2:

			/*
			 * any preserve candidates?
			 */
			if (n_pfs == 0) {
				retcode = parAContinue;
				step = 5;
			} else if ((ret = _preserve_query()) == 2) {
				retcode = parAContinue;
				step = 5;
			} else if (ret == 0) {
				if (confirm_exit(stdscr) == 1) {
					retcode = parAExit;
					done = 1;	/* done, exit */
				} else
					continue;
			} else if (ret == -1) {
				retcode = parAGoback;
				step = 5;
			} else
				step = 3;

			break;

		case 3:
			if (_select_preserve(table, &n_pfs) == 1) {
				_save_preserved(table, n_pfs);
				/* done with preserve */
				retcode = parAContinue;
				step = 4;
			} else {
				step = 2;
			}
			break;

		case 4:
			if ((ret = _sufficient_usable_space(ndisks)) == 0)
				step = 3;	/* back up */
			else if (ret == 1) {
				retcode = parAContinue;	/* continue */
				step = 5;	/* cancel/exit */
			} else if (ret == 2) {
				retcode = parAContinue;	/* skip auto-config */
				step = 5;	/* cancel/exit */
			}
			break;

		case 5:

			/*
			 * unconfig any unedited/unpreserved disks disks,
			 * this wipes out any intermediate state introduced
			 * into the `current' registers of the disk library
			 * by the preserve logic.
			 */
			_restore_all_selected_disks_commit(ndisks);

			/* cleanup memory */
			if (table != (_Pres_Entry_t *) NULL) {
				free((void *) table);
			}
			done = 1;

			break;

		}

	}
	return (retcode);

}

int
has_preservable_fs()
{
	int i;
	int j;
	int size, locked;

	for (i = 0; i < v_get_n_disks(); i++) {

		if (v_get_disk_usable(i) == 1 &&
		    v_get_disk_slices_intact(i) == 1) {

			(void) v_set_current_disk(i);

			for (j = 0; j < N_Slices; j++) {

				(void) v_get_orig_mount_pt(j);
				size = v_get_orig_size(j);
				locked = v_get_lock_state(j);

				/*
				 * size & mount point -> at least one
				 * preserve candidate... return true
				 */
				if (size > 0 && !locked) {

					return (1);

					/* NOTREACHED */
				}
			}
		}
	}

	return (0);
}

static _Pres_Entry_t *
_load_preserve_table(int *n_pfs)
{
	_Pres_Entry_t *table = (_Pres_Entry_t *) NULL;

	int last = 32;
	int cnt;

	int i;
	int j;
	char *mount;
	int size, locked;
	V_Units_t units = v_get_disp_units();

	v_set_disp_units(V_MBYTES);

	/* create an array of `last' preserve entries */
	table = (_Pres_Entry_t *) xcalloc(last *
	    sizeof (_Pres_Entry_t));

	/* for each disk */
	cnt = 0;
	for (i = 0; i < v_get_n_disks(); i++) {

		if (v_get_disk_usable(i) == 1 &&
		    v_get_disk_slices_intact(i) == 1) {

			(void) v_set_current_disk(i);

			/*
			 * need to reset disk to last commited state before
			 * trying to do any preserve stuff since the view
			 * and disk libraries enforce the rule that nothing
			 * can have changed if the slice is going to be
			 * preserved.
			 *
			 * v_restore_disk_commit() restores the disk to its
			 * last saved state.
			 */
			v_restore_disk_commit(i);

			/* for each possible slice */
			for (j = 0; j < N_Slices; j++) {

				/*
				 * want to see&use original configuration
				 * when determining preserved slices
				 */
				mount = v_get_orig_mount_pt(j);
				locked = v_get_lock_state(j);
				size = v_get_orig_size(j);

				/*
				 * if there's a size and the slice is not locked
				 * this slice is a preserve candidate
				 * so add to array.
				 *
				 * preserve overlaps and alts by default,
				 * these slices are always locked
				 */

				if (size > 0 && !locked) {

					table[cnt].mntpt = mount;
					table[cnt].size = size;
					table[cnt].disk = i;
					table[cnt].slice = j;

					/*
					 * If this is an `overlap' slice,
					 * automatically preserve it.
					 *
					 * is commited state of slice already
					 * preserved?  if so, get the
					 * committed mount point, since it
					 * may have already been changed
					 * during a prior `pass' through the
					 * preserve screen
					 *
					 */
					if (v_get_comm_preserved(j) == 1) {
						table[cnt].mntpt =
						    v_get_comm_mount_pt(j);
						table[cnt].preserve = 1;
					} else if (strcmp(mount, Overlap) == 0)
						table[cnt].preserve = 1;
					else
						table[cnt].preserve = 0;

					if (++cnt == last) {

						/* grow array */
						last += 32;

						table = (_Pres_Entry_t *)
						    xrealloc((void *) table,
						    (last *
						    sizeof (_Pres_Entry_t)));
					}
				}
			}
		}
		v_set_disp_units(units);

	}

	*n_pfs = cnt;
	return (table);
}

/*
 * go through all slices in table of slices:
 *
 * For each disk:
 *
 * 1.	For any slice that is preserved
 *		1.  mark it as preserved
 *		2.  restore it's size & start cylinder to original
 *		3.  increment preserved counter
 *
 * 2.   Commit the disk.
 *		saves the current label including any
 *		preserved & modified mount point names.
 */
static void
_save_preserved(_Pres_Entry_t * table, int cnt)
{
	int curdisk;
	int i;

	i = 0;
	while (i < cnt) {

		curdisk = table[i].disk;

		(void) v_set_current_disk(curdisk);

		/* set preserved slices in 'current' */
		while (i < cnt && (curdisk == table[i].disk)) {

			if (table[i].preserve == 1) {

				/*
				 * copy `orig' info to `current'... except
				 * for mount point, since that may have been
				 * changed.
				 */
				(void) v_set_mount_pt(table[i].slice,
				    table[i].mntpt);

				(void) v_set_start_cyl(table[i].slice,
				    v_get_orig_start_cyl(table[i].slice), TRUE);

				(void) v_restore_orig_size(table[i].slice);
				(void) v_set_preserved(table[i].slice, 1);

			} else if (v_get_cur_preserved(table[i].slice) != 0) {

				/*
				 * was preserved, isn't any longer, clear
				 * slice info
				 */
				(void) v_set_preserved(table[i].slice, 0);
				(void) v_set_mount_pt(table[i].slice, "");
				(void) v_set_start_cyl(table[i].slice, 0, TRUE);
				(void) v_set_size(table[i].slice, 0, TRUE);
			}
			++i;
		}

		(void) v_commit_disk(curdisk, V_IGNORE_ERRORS);
	}
}

static int
_preserve_query()
{
	int ch;
	unsigned long fkeys;
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = "Preserving Data";

	(void) werase(stdscr);
	(void) wclear(stdscr);

	wheader(stdscr, TITLE_PREQUERY);

	(void) wword_wrap(stdscr, HeaderLines, INDENT0, COLS - (2 * INDENT0),
	    MSG_PREQUERY);

	fkeys = F_PRESERVE | F_GOBACK | F_CONTINUE | F_EXIT | F_HELP;

	for (;;) {

		wfooter(stdscr, fkeys);
		wcursor_hide(stdscr);
		ch = wzgetch(stdscr, fkeys);

		if (is_continue(ch) != 0) {

			break;

		} else if (is_goback(ch) != 0) {

			break;

		} else if (is_exit(ch) != 0) {

			break;

		} else if (is_preserve(ch) != 0) {

			break;

		} else if (is_help(ch) != 0) {

			do_help_index(stdscr, _help.type, _help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else
			beep();
	}

	if (is_continue(ch) != 0)
		return (2);
	else if (is_preserve(ch) != 0)
		return (1);
	else if (is_exit(ch) != 0)
		return (0);	/* exit */
	else /* if (is_goback(ch) != 0) */
		return (-1);	/* go back to use disks */

}

typedef struct {
	HelpEntry help;
	NRowCol loc;
	FieldType type;
	char *label;
	char *prompt;
	int preserve;
} _Item_Field_t;

/*
 * a row in the preserve table consists of 4 items:
 *	field for preserve toggle
 *	field for mount point
 *	field for disk/slice
 *	field for size
 */
typedef struct {

	_Item_Field_t fld[4];

} _Preserve_Row_t;

/*
 * free any existing fstable labels which have been strdup'ed
 */
static void
_free_fstable_labels(int nslices, _Preserve_Row_t * fstable)
{
	register int i;

	if (fstable != (_Preserve_Row_t *) NULL) {

		for (i = 0; i < nslices; i++) {

			if (fstable[i].fld[1].label != (char *) NULL)
				free((void *) fstable[i].fld[1].label);
			if (fstable[i].fld[2].label != (char *) NULL)
				free((void *) fstable[i].fld[2].label);
			if (fstable[i].fld[3].label != (char *) NULL)
				free((void *) fstable[i].fld[3].label);

		}

	}
}

static void
show_fstable(WINDOW * w, int max, int npp, int row,
    _Preserve_Row_t * fstable, int first)
{
	int i;		/* counts modules displayed	*/
	int j;		/* index of cur software mod	*/
	int r;		/* counts row positions		*/

	for (i = 0, r = row, j = first;
	    (i < npp) && (j < max);
	    i++, r++, j++) {

		(void) mvwprintw(w, r, INDENT1, "%s %s  %s    %s",
		    fstable[j].fld[0].label,
		    fstable[j].fld[1].label, fstable[j].fld[2].label,
		    fstable[j].fld[3].label);

		fstable[j].fld[0].loc.r =
		    fstable[j].fld[1].loc.r =
		    fstable[j].fld[2].loc.r =
		    fstable[j].fld[3].loc.r = (int) r;

		fstable[j].fld[0].loc.c = INDENT1;
		fstable[j].fld[1].loc.c = INDENT1 + 4;
		fstable[j].fld[2].loc.c = 55;
		fstable[j].fld[3].loc.c = 69;

	}

	/*
	 * clear remaining rows, i counts lines displayed, r counts row
	 * lines are displayed on
	 */
	for (; i < npp; i++, r++) {
		(void) mvwprintw(w, r, 0, "%*s", COLS, " ");
	}

}

static int
_select_preserve(_Pres_Entry_t * table, int *nslices)
{
	int i;			/* scratch counter */
	int top_row;		/* first row of menu */
	int last_row;		/* first row of menu */
	int tuple;		/* which line */
	int field;		/* which field */
	int r, c;		/* cursor location */
	int ret;

	int top;		/* index of first item displayed */
	int dirty = 1;
	int fs_per_page;
	int ch;

	char buf[MAXPATHLEN];	/* scratch pointer for sprintf() */
	char buf1[BUFSIZ];	/* scratch pointer for sprintf() */
	char buf2[BUFSIZ];	/* scratch pointer for sprintf() */
	EditField f;
	unsigned long fkeys;
	_Preserve_Row_t *fstable;

	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = "Preserve Data Screen";

	/*
	 * this array parallels the `table' array. table keeps internal
	 * state, fstable keeps display state.
	 */
	fstable = (_Preserve_Row_t *) xcalloc(*nslices *
	    sizeof (_Preserve_Row_t));

	for (i = 0; i < *nslices; i++) {

		/*
		 * first field is preserve toggle widgey
		 */
		fstable[i].fld[0].help = _help;
		fstable[i].fld[0].type = LSTRING;
		fstable[i].fld[0].preserve = table[i].preserve;

		if (table[i].preserve == 0) {
			fstable[i].fld[0].label = Unsel;	/* "[ ]" */
			fstable[i].fld[0].prompt =
			    gettext(" Press Return to preserve this slice ");
		} else {
			fstable[i].fld[0].label = Sel;	/* "[X]" */
			fstable[i].fld[0].prompt =
			    gettext(" Press Return to unpreserve this slice");
		}

		/*
		 * second field is mount point
		 */
		fstable[i].fld[1].help = _help;

		(void) sprintf(buf, "%-*.*s%s",
		    35,
		    ((int) strlen(table[i].mntpt) > 35 ? 33 : 35),
		    table[i].mntpt,
		    ((int) strlen(table[i].mntpt) > 35 ? " ...>" : ""));

		fstable[i].fld[1].label = xstrdup(buf);
		fstable[i].fld[1].prompt = "";
		fstable[i].fld[1].type = LSTRING;

		/*
		 * third field is disk/slice
		 */
		fstable[i].fld[2].help = _help;

		(void) sprintf(buf, "%8.8ss%d", v_get_disk_name(table[i].disk),
		    table[i].slice);

		fstable[i].fld[2].label = xstrdup(buf);
		fstable[i].fld[2].prompt = "";
		fstable[i].fld[2].type = INSENSITIVE;

		/*
		 * fourth field is size
		 */
		fstable[i].fld[3].help = _help;

		(void) sprintf(buf, "%8d %s", table[i].size, gettext("MB"));

		fstable[i].fld[3].label = xstrdup(buf);
		fstable[i].fld[3].prompt = "";
		fstable[i].fld[3].type = INSENSITIVE;

	}

	(void) werase(stdscr);
	(void) wclear(stdscr);
	wheader(stdscr, TITLE_PRESERVE);

	top_row = HeaderLines;

	top_row = wword_wrap(stdscr, top_row, INDENT0, COLS - (2 * INDENT0),
	    MSG_PRESERVE);

	/* print headings */
	++top_row;
	(void) mvwprintw(stdscr, top_row, INDENT1 + 4, "%-35s %-15.15s %10.10s",
	/* i18n: 35 chars max */
	    gettext("Mount Point"),
	/* i18n: 15 chars max */
	    gettext("Disk/Slice"),
	/* i18n: 10 chars max */
	    gettext("Size"));

	++top_row;
	(void) mvwprintw(stdscr, top_row, INDENT1, "%-.*s",
	    35 + 15 + 10 + 3 + 4, EQUALS_STR);

	/* setup menu variables */
	++top_row;
	tuple = 0;
	field = 0;
	top = 0;
	fs_per_page = LINES - top_row - FooterLines - 1;
	last_row = top_row + fs_per_page - 1;

	fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;

	for (;;) {

		if (dirty) {

			(void) show_fstable(stdscr, *nslices, fs_per_page,
			    top_row, fstable, top);

			scroll_prompts(stdscr, top_row, 1, top, *nslices,
			    fs_per_page);

			wfooter(stdscr, fkeys);
			dirty = 0;
		}
		/* highlight current */
		wfocus_on(stdscr, fstable[tuple].fld[field].loc.r,
		    fstable[tuple].fld[field].loc.c,
		    fstable[tuple].fld[field].label);

		(void) getsyx(r, c);
		(void) wnoutrefresh(stdscr);
		(void) setsyx(r, c);
		(void) doupdate();

		if (field == 0)
			ch = wzgetch(stdscr, fkeys);

		else if (field == 1) {

			/*
			 * mount point is editable... cope with it being
			 * modified
			 */
			f.type = LSTRING;
			f.r = fstable[tuple].fld[1].loc.r;
			f.c = fstable[tuple].fld[1].loc.c;
			f.len = 35;
			f.maxlen = MAXMNTLEN;

			(void) strcpy(buf, table[tuple].mntpt);
			f.value = buf;

			ch = wget_field(stdscr, f.type, f.r, f.c, f.len,
			    f.maxlen, f.value, fkeys);

			if (strcmp(buf, table[tuple].mntpt) != 0) {

				(void) v_set_current_disk(table[tuple].disk);

				if (v_set_mount_pt(table[tuple].slice, buf) !=
				    V_NOERR && v_get_v_errno() == V_BADARG) {

					/* illegal mount point */
					(void) strcpy(buf1,
						DISK_EDIT_INVALID_MOUNTPT);
					(void) sprintf(buf2, buf1, buf);

					(void) simple_notice(stdscr,
						F_OKEYDOKEY,
						TITLE_ERROR, buf2);

				} else {

					table[tuple].mntpt =
					    v_get_cur_mount_pt(
						table[tuple].slice);

					/*
					 * save changed mount point into the
					 * status table and the display table
					 */

					(void) sprintf(
					    fstable[tuple].fld[1].label,
					    "%-*.*s%s", 35,
					    ((int) strlen(table[tuple].mntpt)
						> 35 ? 33 : 35),
					    table[tuple].mntpt,
					    ((int) strlen(table[tuple].mntpt)
						> 35 ? " ...>" : ""));

				}
				dirty = 1;
			}
		}
		if ((field == 0) &&
		    ((sel_cmd(ch) != 0) || (alt_sel_cmd(ch) != 0))) {

			if (table[tuple].preserve == 0) {

				if ((ret = try_preserve(stdscr,
					table[tuple].disk, table[tuple].slice,
					table[tuple].mntpt)) == 1) {

					dirty = 1;
					table[tuple].preserve = 1;
					fstable[tuple].fld[0].label = Sel;

				} else {
					/* unhighlight */
					(void) mvwprintw(stdscr,
					    fstable[tuple].fld[field].loc.r,
					    fstable[tuple].fld[field].loc.c,
					    fstable[tuple].fld[field].label);

					if (ret == 0)
						field = 1;  /* skip to name */
				}

			} else if (table[tuple].preserve == 1) {

				table[tuple].preserve = 0;
				fstable[tuple].fld[0].label = Unsel;

				dirty = 1;

			}
		} else if (is_ok(ch) != 0) {

			break;

		} else if (is_cancel(ch) != 0) {

			break;

		} else if (is_help(ch) != 0) {

			do_help_index(stdscr,
			    fstable[tuple].fld[field].help.type,
			    fstable[tuple].fld[field].help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else if (ch == U_ARROW || ch == D_ARROW ||
			ch == R_ARROW || ch == L_ARROW ||
			ch == CTRL_F || ch == CTRL_D ||
			ch == CTRL_N || ch == CTRL_P ||
			ch == CTRL_B || ch == CTRL_U) {

			dirty = 0;

			/* unhighlight */
			wfocus_off(stdscr, fstable[tuple].fld[field].loc.r,
			    fstable[tuple].fld[field].loc.c,
			    fstable[tuple].fld[field].label);

			/* move */
			if (ch == CTRL_D) {

				/* page down */
				if ((tuple + fs_per_page) < *nslices) {

					/* advance a page */
					top += fs_per_page;
					tuple += fs_per_page;
					dirty = 1;

				} else if (tuple < (*nslices - 1)) {

					/* advance to last file system */
					tuple = *nslices - 1;
					top = tuple - 2;
					dirty = 1;

				} else
					beep();	/* at end */

			} else if (ch == CTRL_U) {

				/* page up */
				if ((tuple - fs_per_page) >= 0) {

					/* reverse a page */
					top = (top > fs_per_page ?
					    top - fs_per_page : 0);
					tuple -= fs_per_page;
					dirty = 1;

				} else if (tuple > 0) {

					/* back to first file system */
					top = 0;
					tuple = 0;
					dirty = 1;

				} else
					beep();	/* at top */

			} else if (ch == R_ARROW || ch == L_ARROW ||
			    ch == CTRL_F || ch == CTRL_B) {

				if (field == 1)
					field = 0;
				else if (field == 0)
					field = 1;

			} else if (ch == U_ARROW || ch == CTRL_P) {

				if (fstable[tuple].fld[field].loc.r ==
				    top_row) {

					if (top) {	/* scroll down */
						tuple = --top;
						dirty = 1;
					} else
						beep();	/* very top */

				} else {
					tuple--;
				}

			} else if (ch == D_ARROW || ch == CTRL_N) {

				if (fstable[tuple].fld[field].loc.r ==
				    last_row) {

					if ((tuple + 1) < *nslices) {

						/* scroll up */
						top++;
						tuple++;
						dirty = 1;

					} else
						beep();	/* bottom */

				} else {

					if ((tuple + 1) < *nslices) {
						tuple++;
					} else
						beep();	/* last, no wrap */
				}

			}
		} else
			beep();

	}

	/* cleanup memory */
	if (fstable) {
		_free_fstable_labels(*nslices, fstable);
		free((void *) fstable);
	}
	if (is_ok(ch) != 0)	/* Continue */
		return (1);
	else
		return (0);

}

/*
 * try_preserve()
 *	user wants to preserve slice/mntpnt.
 *	is the mount point (fs name) preserveable?
 *	is the slice itself preserveable - does it conflict with
 *		any edits?
 */
int
try_preserve(WINDOW * win, int disk, int slice, char *mntpt)
{
	char buf[BUFSIZ];
	char buf1[128];
	int err;
	int j;
	int i;
	int tmpdisk;

	if ((err = v_get_preserve_ok(disk, slice, mntpt)) == V_OK)
		return (1);

	switch (err) {

	case V_ALIGNED:

		(void) sprintf(buf, PRESERVE_ERR_MISALIGNED, mntpt);

		(void) simple_notice(win, F_OKEYDOKEY, TITLE_WARNING, buf);

		err = 2;		/* can't preserve */

		break;

	case V_CANTPRES:

		(void) sprintf(buf, PRESERVE_ERR_CANT_PRESERVE, mntpt);

		(void) simple_notice(win, F_OKEYDOKEY, TITLE_WARNING, buf);

		err = 0;

		break;

	case V_SHOULDNTPRES:

		(void) sprintf(buf, PRESERVE_ERR_SHOULDNT_PRESERVE, mntpt);

		if (yes_no_notice(win, (F_OKEYDOKEY | F_CANCEL),
			F_OKEYDOKEY, F_CANCEL, TITLE_WARNING, buf) ==
			F_CANCEL)
			err = 2;	/* don't preserve */
		else
			err = 1;	/* preserve */

		break;

	case V_CHANGED:

		(void) sprintf(buf, PRESERVE_ERR_CONFLICTS_1, mntpt);

		(void) simple_notice(win, F_OKEYDOKEY, TITLE_WARNING, buf);

		err = 0;

		break;

	case V_CONFLICT:
		/*
		 * popup a notice showing all conflicting mount
		 * points/slices. if user chooses to continue and really
		 * preserve this slice, clear out slice information on each
		 * slice in conflict.
		 */
		tmpdisk = v_get_current_disk();
		(void) v_set_current_disk(disk);

		(void) sprintf(buf, PRESERVE_ERR_CONFLICTS_2, mntpt);

		for (i = 0; i < v_get_n_conflicts(); i++) {
			(void) sprintf(buf1, "\t%s %2d: %-.*s\n",
			    gettext("slice"),
			    v_get_conflicting_slice(i),
			    COLS - (2 * INDENT1),
			    v_get_comm_mount_pt(v_get_conflicting_slice(i)));

			(void) strcat(buf, buf1);
		}

		if (yes_no_notice(win, (F_OKEYDOKEY | F_CANCEL),
			F_CONTINUE, F_CANCEL, TITLE_WARNING, buf) ==
			F_CANCEL)
			err = 2;	/* don't preserve */
		else {

			/*
			 * reset all conflicting slices
			 */
			for (i = 0; i < v_get_n_conflicts(); i++) {

				j = v_get_conflicting_slice(i);
				(void) v_set_size(j, 0, TRUE);
				(void) v_set_start_cyl(j, 0, TRUE);
				(void) v_set_mount_pt(j, "");
				(void) v_set_preserved(j, 0);

			}

			err = 1;	/* preserve */
		}

		(void) v_set_current_disk(tmpdisk);

		break;

	    default:
		(void) sprintf(buf, PRESERVE_ERR_CANT_PRESERVE_UNKNOWN, mntpt);

		(void) simple_notice(win, F_OKEYDOKEY, TITLE_WARNING, buf);

		err = 2;
		break;

	}

	return (err);		/* don't preserve */
}

/*
 * returns
 *	0: return to preserve screen
 *	1: continue
 */
int
_sufficient_usable_space(int ndisks)
{
	int i;
	int j;
	int used_disks;
	int avail;
	int usable;
	int needed;

	int fkeys;
	int ret;
	int continue_ret;
	int cancel_ret;

	char buf[BUFSIZ];

	needed = _get_reqd_space();
	avail = _get_avail_space(ndisks);
	used_disks = _get_used_disks(ndisks);

	/*
	 * for each disk available for use, add it's overall size to the
	 * usable total.  Then for each slice on the disk, subtract its
	 * size. This may get bizzarre with overlapped slices or with
	 * unnamed slices...
	 */
	for (i = 0, usable = 0; i < ndisks; i++) {

		if (v_get_disk_usable(i) == 1) {

			usable += v_get_sdisk_capacity(i);

			/* subtract any `preserved' space */
			if (v_get_disk_status(i) == V_DISK_EDITED) {

				(void) v_set_current_disk(i);

				for (j = 0; j < N_Partitions; j++) {
					if (strcmp(v_get_cur_mount_pt(j),
						"overlap") != 0)
						usable -= v_get_cur_size(j);
				}
			}
		}
	}

	/*
	 * have space required for this install (needed)
	 * have total raw space available (avail)
	 * have total usable space (usable)
	 * have # of disks (ndisks)
	 * have # of disks used (used_disks)
	 *
	 * need to determine if usable space on disks, after preserving
	 * any file systems, is sufficient for the installation.
	 * if not, need to figure out what can be done.
	 *
	 */

	ret = 1;

	/* however, circumstance may prevent taking that step */
	if (usable < needed) {

		/* need more space, where can it come from? */

		if (used_disks == 0) {

			/* no disks selected, must choose one */
			(void) sprintf(buf, USE_DISK_INSUFFICIENT_SPACE_4);
			fkeys = F_OKEYDOKEY | F_CANCEL;
			continue_ret = 2;
			cancel_ret = 0;

		} else if ((used_disks < ndisks) && (usable < avail)) {

			/*
			 * not all disks used, choose more disks or free up
			 * preserved space
			 */
			(void) sprintf(buf, USE_DISK_INSUFFICIENT_SPACE_5);

			fkeys = F_OKEYDOKEY | F_CANCEL;
			continue_ret = 2;
			cancel_ret = 0;

		} else if ((used_disks == ndisks) && (usable < avail)) {

			/*
			 * all disks in use and not enough space, can
			 * continue and do manual configuration of disks,
			 * force the next step to be custom config
			 */
			(void) sprintf(buf, USE_DISK_INSUFFICIENT_SPACE_6);
			fkeys = F_OKEYDOKEY | F_CANCEL;
			continue_ret = 2;
			cancel_ret = 0;

		} else if ((used_disks == ndisks) && (usable == avail)) {

			/*
			 * all disks in use and all space is used, must
			 * reduce the space demands by deselecting sw if
			 * server, can reduce clients or architectures.
			 */
			(void) strcpy(buf, USE_DISK_INSUFFICIENT_SPACE_7);

			if (v_get_system_type() == V_SERVER &&
			    (v_get_n_diskless_clients() > 0 ||
				v_get_n_cache_clients() > 0)) {

				(void) strcat(buf,
				    USE_DISK_INSUFFICIENT_SPACE_7A);
			}

			fkeys = F_CONTINUE;
			continue_ret = 2;
			cancel_ret = 2;

		}
		if (yes_no_notice(stdscr, fkeys, F_CONTINUE, F_GOBACK,
			USE_DISK_INSUFFICIENT_SPACE_TITLE, buf) == F_CONTINUE)
			ret = continue_ret;
		else
			ret = cancel_ret;

	}

	return (ret);

}
