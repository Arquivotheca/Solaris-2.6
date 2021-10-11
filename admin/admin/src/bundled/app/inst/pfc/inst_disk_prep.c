#ifndef lint
#pragma ident "@(#)inst_disk_prep.c 1.52 96/06/21 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_disk_prep.c
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
#include "v_disk.h"
#include "inst_msgs.h"

int edit_disk_parts(WINDOW *, int, int);

#define	FLDS_PER_PART	5

static const int part_width = 10;
static const int type_width = 17;
static const int size_width = 15;
static const int cyl_width = 16;
static const int max_prompt_width = 24;
/*
 * only need this when showing cylinder start/end
 *
 * static const int cyl_width = 11;
 *
 */

typedef struct {
	NRowCol loc;
	FieldType type;
	char *value;
	char *prompt;
	int len;
} _Row_item_t;

typedef struct {

	/*
	 * a row in the partition editor is a vector of 4 items:
	 *	field for partition number
	 *	field for partition type
	 *	field for size
	 *	field for start cylinder
	 *	field for end cylinder
	 */
	_Row_item_t	f[FLDS_PER_PART];

}	_FDiskPart_t;

static int _create_part(WINDOW *, int);
static int _delete_part(WINDOW *, int);
static int _valid_size(WINDOW *, int, int);
static int _fdisk_is_legal(WINDOW *, int);

static int _check_state(WINDOW *, int);
static int _do_fdisk_label(WINDOW *, int);
static int _do_solaris_part(WINDOW *, int);
static int _create_spart(WINDOW *, int);
static int _create_max_spart(WINDOW *, int);
static int _create_free_spart(WINDOW *, int);
static int _edit_and_create_spart(WINDOW *, int);
static void _change_part_type(WINDOW *, int, int);
static void _show_parts(WINDOW *, _FDiskPart_t *, int);
static _FDiskPart_t *_load_parts(_FDiskPart_t *);

/*
 * Entry point into disk preparation for ttinstall.
 *
 * Most of the processing in this module is directed at X86 installs
 * since they must cope with the FDISK labeling and physical disk
 * `partitioning' before getting into the logical file system disk
 * partitioning (`slicing').
 *
 * The automatic disk `configuration' done by the disk library handles
 * disks with no physical geometry (p-geom).  This apparently goes out
 * any queries the drive for the necessary information (tracks, heads, ...).
 *
 * So, disk preparation consists of the following logical steps:
 *
 *	1.	check disk state
 *			(_check_state())
 *	2.	check/do FDISK label
 *			(_do_fdisk_label())
 *	3.	check/do Solaris partition
 *			(_do_solaris_part())
 *	4.	validate FDISK
 *
 */

int
prepare_disk(WINDOW * parent, int disk)
{
	int ret;
	WINDOW *win;
	int done = 0;

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);

	while (!done) {

		/*
		 * disk is usable?
		 */
		if (_check_state(win, disk) == 0) {

			ret = 0;	/* disk not usable */
			done = 1;
			continue;
		}
		/*
		 * disk must be selected for further processing... so,
		 * select it and make it the current disk
		 */
		(void) v_set_disk_selected(disk, 1);
		(void) v_set_current_disk(disk);

		/*
		 * disk needs & has FDISK label?
		 */
		if ((ret = _do_fdisk_label(win, disk)) == 0) {

			done = 1;
			continue;	/* cancelled */

		} else if (ret == 1) {	/* no FDISK required */

			parent = (WINDOW *) NULL;	/* refresh opt. */

			done = 1;
			continue;

		}
		/*
		 * disk needs & has Solaris Partition?
		 */
		if (_do_solaris_part(win, disk) == 0) {

			ret = 0;	/* disk not usable */

			done = 1;
			continue;
		}
		/*
		 * disk & FDISK configured ok? The too small solaris
		 * partition problem is caught elsewhere, don't want to put
		 * it up twice.
		 */
		if (v_fdisk_validate(disk) != V_OK &&
		    v_get_v_errno() != V_TOOSMALL) {
			(void) simple_notice(win, F_OKEYDOKEY,
			    DISK_PREP_CONFIG_ERR_TITLE, v_fdisk_get_err_buf());

			(void) v_set_disk_selected(disk, 0);

			ret = 0;	/* disk not usable */

			done = 1;
			continue;

		} else {

			ret = 1;	/* disk ok to use */

			done = 1;
			continue;
		}

	}

	(void) delwin(win);

	if (parent != (WINDOW *) NULL) {
		(void) clearok(curscr, TRUE);
		(void) touchwin(parent);
		(void) wnoutrefresh(parent);
		(void) clearok(curscr, FALSE);
	}
	return (ret);

}

/*
 * is the disk OK and usable ?
 */
static int
_check_state(WINDOW * w, int i)
{
	char buf[BUFSIZ];

	if (v_get_disk_status(i) != V_DISK_NOTOKAY)
		return (1);

	(void) sprintf(buf, DISK_PREP_DISK_HOSED, v_get_disk_name(i),
	    v_get_disk_status_str(i));

	(void) simple_notice(w, F_OKEYDOKEY,
		DISK_PREP_DISK_NOTUSABLE_ERR_TITLE, buf);

	return (0);
}

/*
 * does the disk need & have an FDISK label?
 */
static int
_do_fdisk_label(WINDOW * w, int i)
{
	if (v_fdisk_flabel_req(i) == 0)
		return (1);

	if (v_fdisk_flabel_exist(i) == 1)
		return (2);

	if (yes_no_notice(w, (F_OKEYDOKEY | F_CANCEL),
		F_OKEYDOKEY, F_CANCEL, DISK_PREP_NO_FDISK_LABEL_TITLE,
		DISK_PREP_NO_FDISK_LABEL) == F_OKEYDOKEY) {

		(void) v_fdisk_set_default_flabel(i);
		return (2);
	}
	return (0);
}

/*
 * does the disk have & need a Solaris Partition? If not, allow the user to
 * creat or define one.
 */
static int
_do_solaris_part(WINDOW * w, int disk)
{

	if (v_fdisk_flabel_has_spart(disk) != 0)
		return (1);

	if (yes_no_notice(w, (F_OKEYDOKEY | F_CANCEL),
		F_OKEYDOKEY, F_CANCEL, DISK_PREP_NO_SOLARIS_PART_TITLE,
		DISK_PREP_NO_SOLARIS_PART) == F_OKEYDOKEY) {

		/*
		 * is a default partition possible?
		 */
		if (v_fdisk_get_max_partsize_free(disk) != -1)
			return (_create_spart(w, disk));

		/*
		 * yikes!  no!
		 */
		if (yes_no_notice(w, (F_OKEYDOKEY | F_CANCEL),
			F_OKEYDOKEY, F_CANCEL,
			DISK_PREP_NO_FREE_FDISK_PART_TITLE,
			DISK_PREP_NO_FREE_FDISK_PART) == F_OKEYDOKEY) {
			return (_edit_and_create_spart(w, disk));
		} else
			return (0);	/* cancelled */
	}
	return (0);
}

/*
 * create a Solaris partition.
 *
 * default choices: entire disk, largest free chunk or customize.
 *
 * if no free partition, force user to customize or cancel
 */
static int
_create_spart(WINDOW * w, int disk)
{
	char *opts[4];
	int  (*actions[4])(WINDOW *, int);
	int nopts;

	int i;
	int ch;
	int row;
	u_long selected;
	u_int fkeys;
	HelpEntry _help;

	char buf[128];

	_help.win = w;
	_help.type = HELP_TOPIC;
	_help.title = "Solaris fdisk Partitions";

	v_set_disp_units(V_MBYTES);

	nopts = 0;

	(void) sprintf(buf, "%s (%d %s)", CHOOSE_SOLARIS_PART_TYPE_CHOICE1,
	    v_fdisk_get_max_partsize(disk), v_get_disp_units_str());

	opts[nopts] = xstrdup(buf);
	actions[nopts] = _create_max_spart;

	nopts++;

	if (v_fdisk_get_max_partsize_free(disk) > 0) {
		(void) sprintf(buf, "%s (%d %s)",
		CHOOSE_SOLARIS_PART_TYPE_CHOICE2,
		v_fdisk_get_max_partsize_free(disk), v_get_disp_units_str());

		opts[nopts] = xstrdup(buf);
		actions[nopts] = _create_free_spart;
		nopts++;
	}

	opts[nopts] = xstrdup(CHOOSE_SOLARIS_PART_TYPE_CHOICE3);
	actions[nopts] = _edit_and_create_spart;
	nopts++;

	selected = 0;		/* solaris is entire disk */

	(void) werase(w);
	(void) wclear(w);

	wheader(w, TITLE_CREATESOLARIS);

	row = HeaderLines;
	row = wword_wrap(w, row, INDENT0, COLS - (2 * INDENT0),
	    MSG_CREATESOLARIS);
	++row;

	fkeys = (F_OKEYDOKEY | F_CANCEL | F_HELP);

	wfooter(w, fkeys);

	ch = wmenu(w, row, INDENT1, LINES - HeaderLines - FooterLines,
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

	if (is_ok(ch) != 0 || is_continue(ch) != 0) {

		/* call appropriate action */
		if (selected < nopts)
			return (actions[selected](w, disk));
		else
			return (0);

	} else /* if (is_cancel(ch) != 0 || is_exit(ch) != 0) */
		return (0);

}

/*
 * user wants entire disk top be used by solaris.
 */
/*ARGSUSED0*/
static int
_create_max_spart(WINDOW *w, int disk)
{
	if (v_fdisk_set_solaris_max_partsize(disk) != 0)
		return (1);
	else
		return (0);
}

/*
 * user wants largest free chunk to be used for solaris
 */
/*ARGSUSED0*/
static int
_create_free_spart(WINDOW *w, int disk)
{
	if (v_fdisk_set_solaris_free_partsize(disk) != 0)
		return (1);
	else
		return (0);
}

/*
 * user wants to customize FDISK label.
 */
static int
_edit_and_create_spart(WINDOW * w, int disk)
{
	return (edit_disk_parts(w, disk, 0));
}

static void
_show_parts(WINDOW * win, _FDiskPart_t * table, int top_row)
{
	int i;
	int j;
	int row;
	int used;
	int disk = v_get_current_disk();
	char *units = v_get_disp_units_str();

	used = 0;

	/* show table */
	for (i = 0, row = top_row; i < N_Partitions; i++, row++) {

		if (v_fdisk_get_part_type(i) != V_UNUSED)
			used += v_fdisk_get_part_size(i);

		for (j = 0; j < FLDS_PER_PART; j++) {

			if (table[i].f[j].type == RSTRING)
				(void) mvwprintw(win, row, table[i].f[j].loc.c,
				    "%*s", table[i].f[j].len,
				    table[i].f[j].value);
			else if (table[i].f[j].type == LSTRING)
				(void) mvwprintw(win, row, table[i].f[j].loc.c,
				    "%-*s", table[i].f[j].len,
				    table[i].f[j].value);

			table[i].f[j].loc.r = row;

		}
	}

	/*
	 * print 'totals'... used, available and capacity
	 */
	++row;
	v_set_disp_units(V_MBYTES);

	/*
	 * total raw drive size
	 */
	(void) mvwprintw(win, row++, 2, "%40s:   %7d %s",
	/* i18n: 35 chars max */
	    gettext("Capacity"), v_get_disk_capacity(disk), units);

	/*
	 * total space allocated to partitions, sum of partition sizes
	 * after truncation
	 */
	(void) mvwprintw(win, row++, 2, "%40s:   %7d %s",
	/* i18n: 35 chars max */
	    gettext("Allocated"), used, units);

	/*
	 * there is a potential apparent loss of space due to unit
	 * conversion and rounding/truncation.
	 *
	 * if necessary, display the apparent loss
	 *
	 * if (used + free != capacity) we've lost something to
	 * rounding...
	 */
	if ((used + v_fdisk_get_space_avail(disk)) !=
				v_get_disk_capacity(disk)) {
		(void) mvwprintw(win, row++, 2, "%40s:   %7d %s",
		/* i18n: 35 chars max */
		gettext("Rounding Error"),
			v_get_disk_capacity(disk) -
			v_fdisk_get_space_avail(disk) - used,
			units);
	}

	/*
	 * remaining space on raw drive, truncated.
	 */
	(void) mvwprintw(win, row++, 2, "%40s:   %7d %s",
	/* i18n: 35 chars max */
	    gettext("Free"), v_fdisk_get_space_avail(disk), units);
}

static _FDiskPart_t *
_load_parts(_FDiskPart_t * table)
{
	int i;
	int j;

	char buf[128];

	/*
	 * get memory for partition table
	 */
	if (table == (_FDiskPart_t *) NULL) {
		table = (_FDiskPart_t *) xcalloc(N_Partitions *
		    sizeof (_FDiskPart_t));
		(void) memset(table, '\0',
			(N_Partitions * sizeof (_FDiskPart_t)));
	} else {
		for (i = 0; i < N_Partitions; i++)
			for (j = 0; j < FLDS_PER_PART; j++)
				if (table[i].f[j].value != (char *) NULL) {
					free((void *) table[i].f[j].value);
					table[i].f[j].value = (char *) NULL;
				}
	}

	/*
	 * load partition `table' to pass back to event loop
	 */
	v_set_disp_units(V_MBYTES);
	for (i = 0; i < N_Partitions; i++) {

		/* part # field */
		/* table[i].f[0].loc.c = INDENT1 + 4; */
		table[i].f[0].loc.c = INDENT1;
		table[i].f[0].type = LSTRING;

		(void) sprintf(buf, "%*d", (int) (part_width / 2),
		    FD_PART_NUM(i));
		table[i].f[0].value = (char *) xstrdup(buf);
		table[i].f[0].len = part_width;
		table[i].f[0].prompt = (char *) NULL;

		/* part type */
		table[i].f[1].loc.c = table[i].f[0].loc.c + part_width + 6;
		table[i].f[1].type = LSTRING;

		(void) sprintf(buf, "%s", v_fdisk_get_part_type_str(i));

		table[i].f[1].value = (char *) xstrdup(buf);
		table[i].f[1].len = type_width;

		if (v_fdisk_get_part_type(i) != V_UNUSED)
			table[i].f[1].prompt = (char *) xstrdup(
			gettext(" Press Return to change fdisk partition type "));
		else if (v_fdisk_get_part_maxsize(i) > 0)
			table[i].f[1].prompt = (char *) xstrdup(
			gettext(" Press Return to create a new fdisk partition "));
		else
			table[i].f[1].prompt = (char *) NULL;

		/* part size in MB */
		table[i].f[2].loc.c = table[i].f[1].loc.c + type_width + 2;
		table[i].f[2].type = RSTRING;

		(void) sprintf(buf, "%*d", size_width,
		    v_fdisk_get_part_size(i));
		table[i].f[2].value = (char *) xstrdup(buf);
		table[i].f[2].len = size_width;

		if (v_fdisk_get_part_maxsize(i) > 0) {
			(void) sprintf(buf, " %s ", gettext("Enter new size"));
			table[i].f[2].prompt = (char *) xstrdup(buf);
		} else
			table[i].f[2].prompt = (char *) NULL;

		/* part start cyl */

		table[i].f[3].loc.c = table[i].f[2].loc.c + size_width + 2;
		table[i].f[3].type = RSTRING;

		(void) sprintf(buf, "%*d", cyl_width,
		    v_fdisk_get_part_startsect(i));
		table[i].f[3].value = (char *) xstrdup(buf);
		table[i].f[3].len = cyl_width;
		table[i].f[3].prompt = (char *) NULL;

#ifdef notdef

		/* part end cyl */
		table[i].f[4].loc.c = table[i].f[3].loc.c + cyl_width + 2;
		table[i].f[4].type = RSTRING;

		(void) sprintf(buf, "%*d", cyl_width,
		    v_fdisk_get_part_endcyl(i));
		table[i].f[4].value = (char *) xstrdup(buf);
		table[i].f[4].len = cyl_width;
		table[i].f[4].prompt = (char *) NULL;

		/* max part size prompt in MB */
		table[i].f[3].loc.c = table[i].f[2].loc.c + size_width + 1;
		table[i].f[3].type = RSTRING;
		table[i].f[3].len = max_prompt_width;
		table[i].f[3].prompt = (char *) NULL;

		if (v_fdisk_get_part_maxsize(i) > 0) {
			(void) sprintf(buf, "(%s: %d %s)", gettext("maximum"),
			    v_fdisk_get_part_maxsize(i),
			    v_get_disp_units_str());
			table[i].f[3].value = (char *) xstrdup(buf);
		} else {
			(void) sprintf(buf, "%*s", max_prompt_width, " ");
			table[i].f[3].value = (char *) xstrdup(buf);
		}

#else

		/* max part size prompt clear buffer */
		table[i].f[4].loc.c = table[i].f[3].loc.c + cyl_width;
		table[i].f[4].type = RSTRING;
		table[i].f[4].len = max_prompt_width;
		table[i].f[4].prompt = (char *) NULL;

		(void) sprintf(buf, "%*s", max_prompt_width, " ");
		table[i].f[4].value = (char *) xstrdup(buf);
#endif

	}

	return (table);
}

/*
 * edit paritions on thie FDISK label.
 *
 * returns 1 if FDISK is ok for use by Solaris.
 * returns 0 if disk is not usable.
 */
int
edit_disk_parts(WINDOW * parent, int disk, int startat)
{
	WINDOW *win;

	int part;		/* which partition */
	int fld;		/* which field */
	int ch;
	int ret;
	int row;
	int locked;
	int really_dirty;
	int dirty;
	char buf[128];		/* scratch buffer for size edits */
	int r, c;		/* cursor location */
	HelpEntry _help;
	unsigned long fkeys;
	_FDiskPart_t *table = (_FDiskPart_t *) NULL;

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);

	_help.win = win;
	_help.type = HELP_TOPIC;
	_help.title = "Solaris fdisk Partitions";

	(void) v_set_current_disk(disk);

	part = startat;		/* first slice */
	fkeys = F_OKEYDOKEY | F_DELETE | F_CREATE | F_CANCEL | F_HELP;

	(void) werase(win);
	(void) wclear(win);

	/* show Editor screen title */
	(void) sprintf(buf, "%s: %-s",
	    gettext("Customize fdisk Partitions for Disk"),
	    v_get_disk_name(disk));

	wheader(win, buf);

	row = HeaderLines;
	row = wword_wrap(win, row, INDENT0, COLS - (2 * INDENT0),
	    DISK_PREP_PART_EDITOR_ONSCREEN);
	row++;

	/* show column headings */
	/* (void) sprintf(buf, "%-*.*s  %-*.*s  %*.*s  %*.*s  %*.*s", */

	(void) sprintf(buf, "%-*.*s  %-*.*s  %*.*s  %*.*s",
	/* i18n: 11 chars max */
	    part_width, part_width, gettext("Partition"),
	/* i18n: 20 chars max */
	    type_width, type_width, gettext("Type"),
	/* i18n: 10 chars max */
	    size_width, size_width, gettext("Size"),
	/* i18n: 16 chars max */
	    cyl_width, cyl_width, gettext("Start Cylinder"));

	(void) mvwprintw(win, row++, INDENT1 + 4, buf);

	(void) sprintf(buf, "%.*s", part_width + type_width +
			size_width + cyl_width + 7, EQUALS_STR);

	(void) mvwprintw(win, row++, INDENT1 + 4, buf);
	(void) mvwprintw(win, row+4, INDENT1 + 4, buf);

	part = 0;
	fld = 1;
	really_dirty = 1;
	locked = 0;

	for (;;) {

		if (really_dirty == 1) {

			/*
			 * _load_parts() malloc's memory which must be freed
			 * on exit.  However, during the editing process,
			 * memory is re-used
			 */
			table = _load_parts(table);
			wfooter(win, fkeys);

			really_dirty = 0;
			dirty = 1;
		}

		if (dirty == 1) {

			_show_parts(win, table, row);
			dirty = 0;

		}

		/* hilight current */
		wfocus_on(win, table[part].f[fld].loc.r,
		    table[part].f[fld].loc.c, table[part].f[fld].value);

		(void) getsyx(r, c);
		(void) wnoutrefresh(win);
		(void) setsyx(r, c);
		(void) doupdate();

#ifdef notdef
		/* set footer */
		if (table[part].f[fld].prompt != (char *) NULL &&
		    table[part].f[fld].prompt[0] != '\0') {
			wstatus_msg(win, table[part].f[fld].prompt);
		} else {
			wclear_status_msg(win);
		}
#endif

		ch = wzgetch(win, fkeys);

		/* unhilight current */
		wfocus_off(win, table[part].f[fld].loc.r,
		    table[part].f[fld].loc.c, table[part].f[fld].value);

		if (is_continue(ch) != 0 && (locked == 0)) {

			if ((ret = _fdisk_is_legal(win, disk)) == 1) {
				(void) v_commit_disk(disk, V_IGNORE_ERRORS);
				break;
			} else if (ret == 0) {
				ret = 0;	/* disk *not* OK */
				break;
			} else
				dirty = 1;

		} else if (is_cancel(ch) != 0) {

			v_restore_disk_commit(disk);

			if ((ret = _fdisk_is_legal(win, disk)) == 1) {
				break;
			} else if (ret == 0) {
				break;
			} else
				dirty = 1;

			break;

		} else if (is_create(ch) != 0 || ch == RETURN) {

			(void) _create_part(win, part);
			really_dirty = 1;

		} else if (is_delete(ch) != 0) {

			(void) _delete_part(win, part);
			really_dirty = 1;

		} else if (is_help(ch) != 0) {

			do_help_index(win, _help.type, _help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else if ((ch == D_ARROW || ch == CTRL_F || ch == CTRL_D ||
			ch == CTRL_N || ch == TAB) && (locked == 0)) {

			/* move to next row */
			if (part < (N_Partitions - 1))
				++part;
			else
				part = 0;

			if (fld == 2 && v_fdisk_get_part_type(part) ==
			    V_UNUSED)
				fld = 1;

		} else if ((ch == U_ARROW || ch == CTRL_B || ch == CTRL_U ||
		    ch == CTRL_P) && (locked == 0)) {

			/* move to previous row */
			if (part > 0)
				--part;
			else
				part = N_Partitions - 1;

			if (fld == 2 && v_fdisk_get_part_type(part) ==
			    V_UNUSED)
				fld = 1;

#ifdef notdef
	/*
	 * size is not directly editable
	 */
		} else if (ch == L_ARROW || ch == CTRL_P) {

			/* move to other field */
			if (fld == 1 && v_fdisk_get_part_type(part) != V_UNUSED)
				fld = 2;
			else
				fld = 1;

		} else if (ch == R_ARROW || ch == CTRL_N) {

			/* move to other field */
			if (fld == 1 && v_fdisk_get_part_type(part) != V_UNUSED)
				fld = 2;
			else
				fld = 1;
#endif

		} else
			beep();
	}

	/*
	 * clean up memory
	 */
	if (table != (_FDiskPart_t *) NULL) {

		for (part = 0; part < N_Partitions; part++)
			for (fld = 0; fld < FLDS_PER_PART; fld++) {

				if (table[part].f[fld].value != (char *) NULL)
					free((void *)
					    table[part].f[fld].value);
			}

		free((void *) table);
		table = (_FDiskPart_t *) NULL;
	}
	(void) delwin(win);
	(void) clearok(curscr, TRUE);
	(void) touchwin(parent);
	(void) wnoutrefresh(parent);
	(void) clearok(curscr, FALSE);

	v_set_disp_units(V_MBYTES);

	return (ret);
}

/*
 * disk & FDISK configured ok?
 *
 * error handling is dependent on `severity' of error.  Some errors
 * cannot be ignored and the user may not exit the editor.
 *
 * other errors are tolerable, but make the disk unusable.
 *
 * returns:
 * 	0 	- disk not OK, don't use
 *	1	- disk OK, use
 *	2 	- continue editing
 */
static int
_fdisk_is_legal(WINDOW * win, int disk)
{
	int ret;
	char buf[BUFSIZ];

	if (v_fdisk_validate(disk) != V_OK) {

		switch (v_get_v_errno()) {
		case V_OVER:
		case V_BADORDER:
		case V_OUTOFREACH:

			(void) sprintf(buf, "%s\n\n%s", v_fdisk_get_err_buf(),
			    FDISK_DISMISS_TO_EDIT);

			/* terminal errors, can't leave! */
			(void) simple_notice(win, F_OKEYDOKEY,
			    DISK_PREP_CONFIG_ERR_TITLE, buf);

			ret = 2;	/* continue edits */
			break;

		case V_NOSOLARIS:
		case V_TOOSMALL:
		default:

			/*
			 * problems, can exit, but disk will not be usable.
			 */
			(void) sprintf(buf, "%s\n\n%s", v_fdisk_get_err_buf(),
			    FDISK_CONTINUE_TO_EDIT);

			if (yes_no_notice(win, (F_OKEYDOKEY | F_CANCEL),
				F_CONTINUE, F_CANCEL,
				DISK_PREP_CONFIG_ERR_TITLE,
				buf) == F_CANCEL) {

				ret = 2;	/* continue edits */

			} else
				ret = 1;	/* selected, not OK. */

			break;

		}

	} else {
		ret = 1;	/* disk ok to use */
	}

	return (ret);

}
/*
 * verifies that partition size entered is valid.  issues warning if not.
 */
static int
_valid_size(WINDOW * w, int tmpsize, int maxsize)
{
	char buf[BUFSIZ];

	if (tmpsize <= 0) {

		(void) simple_notice(w, F_OKEYDOKEY,
			TITLE_DISK_PREP_PART_SIZE_TOO_SMALL,
			DISK_PREP_PART_SIZE_TOO_SMALL);

		return (0);
	} else if (tmpsize <= maxsize) {
		return (1);
	} else {
		(void) sprintf(buf, DISK_PREP_PART_SIZE_TOO_BIG,
		    v_cyls_to_mb(v_get_current_disk(), maxsize), maxsize);

		(void) simple_notice(w, F_OKEYDOKEY, TITLE_WARNING, buf);

		return (0);
	}
}

/*
 * delete FDISK partition `part'
 */
static int
_delete_part(WINDOW * w, int part)
{
	char buf[BUFSIZ];
	Disk_t *diskPtr = v_int_get_current_disk_ptr();

	if (v_fdisk_get_part_size(part) > 0) {

		(void) sprintf(buf, DISK_PREP_DELETE_PART, FD_PART_NUM(part),
		    v_fdisk_get_part_size(part));

		if (yes_no_notice(w, F_OKEYDOKEY | F_CANCEL, F_OKEYDOKEY,
			F_CANCEL,
			DISK_PREP_DELETE_PART_TITLE, buf) == F_OKEYDOKEY) {

			(void) set_part_preserve(diskPtr, FD_PART_NUM(part),
				PRES_NO);
			(void) v_fdisk_set_part_type(part, V_UNUSED);
			(void) v_fdisk_set_part_size(part, 0);

			return (1);
		}
	} else if (v_fdisk_get_part_type(part) != V_UNUSED) {
		/*
		 * you should be able to delete a partion of size zero,
		 * if it is unused. This is just in case the fdisk table
		 * is out of whack somehow.
		 */
		(void) set_part_preserve(diskPtr, FD_PART_NUM(part),
			PRES_NO);
		(void) v_fdisk_set_part_type(part, V_UNUSED);
		(void) v_fdisk_set_part_size(part, 0);

		return (1);
	}
	return (0);
}

static EditField f[3] = {
	{0, 48, 10, 10, RSTRING, (char *) 0},
	{0, 48, 10, 10, RSTRING, (char *) 0},
	{0, 48, 10, 10, RSTRING, (char *) 0},
};

/*
 * create a new FDISK partition 'part'
 */
static int
_create_part(WINDOW *parent, int part)
{
	WINDOW *win;
	char size_mb[10];
	char size_cyl[10];
	int maxsize;
	int tmpsize;
	int orig_cyls;
	int orig_mb;
	HelpEntry _help;

	int ch;
	int j;
	int fld = 0;
	int nflds;
	int row;
	int disk = v_get_current_disk();

	V_DiskPart_t orig;
	unsigned long fkeys;

	/*
	 * figure out if we can even create a partition...
	 */
	orig = v_fdisk_get_part_type(part);

	if (orig != V_UNUSED) {
		/* partition is in use, tell user.	 */
		(void) simple_notice(parent, F_OKEYDOKEY,
		    DISK_PREP_CREATE_PART_ERR_TITLE1,
		    DISK_PREP_CREATE_PART_ERR1);

		return (0);
	}

	v_set_disp_units(V_CYLINDERS);
	orig_cyls = maxsize = v_fdisk_get_part_maxsize(part);

	if (maxsize <= 0) {

		/* no space, tell user.	 */
		(void) simple_notice(parent, F_OKEYDOKEY,
		    DISK_PREP_CREATE_PART_ERR_TITLE,
		    DISK_PREP_CREATE_PART_ERR);

		return (0);
	}

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);

	_help.win = win;
	_help.type = HELP_TOPIC;
	_help.title = "Solaris fdisk Partitions";

	werase(win);
	wclear(win);
	wheader(win, DISK_PREP_NEWPART_TITLE);

	/*
	 * there's space, so set default values
	 */
	if (orig == V_UNUSED && v_fdisk_flabel_has_spart(disk) == 0)
		(void) v_fdisk_set_part_type(part, V_SUNIXOS);
	else if (orig == V_UNUSED && v_fdisk_flabel_has_spart(disk) != 0)
		(void) v_fdisk_set_part_type(part, V_DOSPRIMARY);

	orig_mb = v_cyls_to_mb(disk, maxsize);

	(void) sprintf(size_mb, "%d", orig_mb);
	(void) sprintf(size_cyl, "%d", orig_cyls);

	f[0].value = (char *) v_fdisk_get_part_type_str(part);
	f[1].value = (char *) size_mb;
	f[2].value = (char *) size_cyl;

	f[0].c = f[1].c = f[2].c = 38;

	/*
	 * draw title, field prompts, etc
	 */
	row = HeaderLines;
	row = wword_wrap(win, row, INDENT0, COLS - (2 * INDENT0),
	    DISK_PREP_NEWPART_ONSCREEN_HELP);
	row += 2;

	/* display fields */
	f[0].r = row;
	(void) mvwprintw(win, row++, 0, "%35.35s:",
	/* i18n: 35 characters max */
	    gettext("Partition type"));
	f[1].r = row;
	(void) mvwprintw(win, row++, 0, "%35.35s:",
	/* i18n: 35 characters max */
	    gettext("Partition size (MB)"));
	f[2].r = row;
	(void) mvwprintw(win, row++, 0, "%35.35s:",
	/* i18n: 35 characters max */
	    gettext("Partition size (Cyl)"));

	fld = 1;
	nflds = 3;
	fkeys = F_OKEYDOKEY| F_CHANGETYPE | F_CANCEL | F_HELP;

	for (;;) {

		wfooter(win, fkeys);

		/* (re)paint both fields */
		for (j = 0; j < nflds; j++) {

			(void) mvwprintw(win, f[j].r, f[j].c, "%*.*s", f[j].len,
			    f[j].len, f[j].value);
		}

		ch = wget_field(win, f[fld].type, f[fld].r, f[fld].c,
		    f[fld].len, f[fld].maxlen, f[fld].value, fkeys);

		if (is_cancel(ch) != 0) {
			break;
		}

		/* validate input... */
		if (verify_field_input(&f[fld], NUMERIC, 0, 0) ==
		    FAILURE) {

			/* hack to make fields with `bad' input 0 */
			(void) strcpy(f[fld].value, "0");
		}

		if (fld == 1) {

			/*
			 * XXX
			 * gotta get around rounding problems.  if the
			 * size in MB hasn't changed, don't convert the
			 * the MB to Cyls.
			 *
			 * MB field, set Cyl field value
			 */
			if (atoi(size_mb) == orig_mb)
				tmpsize = orig_cyls;
			else
				tmpsize = v_mb_to_cyls(disk, atoi(size_mb));

			if (_valid_size(win, tmpsize, maxsize) == 1) {

				/* update size in cylinders */
				(void) sprintf(size_cyl, "%d", tmpsize);

			} else {

				/* restore size in mb */
				(void) sprintf(size_mb, "%d",
				    v_cyls_to_mb(disk, maxsize));
				continue;
			}

		} else if (fld == 2) {

			/* Cyl field, set MB field value */
			tmpsize = atoi(size_cyl);

			if (_valid_size(win, tmpsize, maxsize) == 1) {

				/* update size in mb */
				(void) sprintf(size_mb, "%d",
				    v_cyls_to_mb(disk, tmpsize));

			} else {

				/* restore size in cyl */
				(void) sprintf(size_cyl, "%d", maxsize);
				continue;
			}
		}

		if (is_ok(ch) != 0) {

			break;

		} else if (is_changetype(ch) != 0) {

			_change_part_type(win, disk, part);
			f[0].value = (char *) v_fdisk_get_part_type_str(part);

		} else if (is_help(ch) != 0) {

			do_help_index(_help.win, _help.type, _help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else if (ch == U_ARROW || ch == D_ARROW || ch == TAB ||
		    ch == RETURN) {

			if (ch == U_ARROW || ch == TAB ||
			    ch == RETURN) {

				if (fld == 1)
					fld = 2;
				else if (fld == 2)
					fld = 1;

			} else if (ch == D_ARROW) {	/* back */

				if (fld == 2)
					fld = 1;
				else if (fld == 1)
					fld = 2;
			}
		} else
			beep();
	}

	(void) delwin(win);
	(void) clearok(curscr, TRUE);
	(void) touchwin(parent);
	(void) wnoutrefresh(parent);
	(void) clearok(curscr, FALSE);

	if (is_ok(ch) != 0) {

		(void) v_fdisk_set_part_size(part, atoi(size_cyl));

		/* force Solaris partition to be active */
		if (v_fdisk_get_part_type(part) == V_SUNIXOS)
			(void) v_fdisk_set_active_part(part);

		v_set_disp_units(V_MBYTES);

		return (1);

	} else /* if (is_cancel(ch) != 0) */ {

		(void) v_fdisk_set_part_type(part, orig);

		v_set_disp_units(V_MBYTES);
		return (0);

	}
}

static void
_change_part_type(WINDOW * parent, int disk, int part)
{
	WINDOW *win;
	char **opts;
	int   *indexes;

	int done;
	int ch;
	int ntypes;
	int i;
	int row;
	int options;

	u_long selected = 999L;
	u_long fkeys;
	HelpEntry _help;

	ntypes = v_fdisk_get_n_part_types();
	opts = (char **) xcalloc(ntypes * sizeof (char *));
	indexes = (int *) xcalloc(ntypes * sizeof (int));

	/*
	 * load array of choices, set selected status on current type
	 */
	for (i = 0, options = 0; i < ntypes; i++) {

		/*
		 * Do not include DOSEXT, "<unused>" or "Other" in the
		 * list of choices presented to the user for creating
		 * a parition - only "Solaris" and "DOS" are presented
		 * as valid choices for creation.
		 */
		if (v_fdisk_get_type_by_index(i) == V_UNUSED ||
		    v_fdisk_get_type_by_index(i) == V_DOSEXT ||
		    v_fdisk_get_type_by_index(i) == V_OTHER)
			continue;

		opts[options] = (char *) v_fdisk_get_type_str_by_index(i);
		indexes[options] = i;	/* real type index */

		if (v_fdisk_get_part_type(part) ==
		    v_fdisk_get_type_by_index(i)) {
			selected = options;
		}
		++options;
	}

	/*
	 * if current type is unused (selected == -1)
	 * and there's no SOlaris parititon, make that the default,
	 * otherwise, make DOSHUGE the default
	 */
	if (selected == 999L && v_fdisk_flabel_has_spart(disk) == 0)
		selected = 0L;
	else if (selected == 999L && v_fdisk_flabel_has_spart(disk) != 0)
		selected = 1L;

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);

	_help.win = win;
	_help.type = HELP_TOPIC;
	_help.title = "Solaris fdisk Partitions";

	(void) werase(win);
	(void) wclear(win);
	wheader(win, CHANGE_PART_TYPE_TITLE);

	row = HeaderLines;
	row = wword_wrap(win, row, INDENT0, COLS - (2 * INDENT0),
	    CHANGE_PART_TYPE_ONSCREEN_HELP);
	row++;

	fkeys = F_OKEYDOKEY | F_CANCEL;

	wfooter(win, fkeys);

	done = 0;
	while (!done) {
		ch = wmenu(win, row, INDENT1, LINES - HeaderLines - FooterLines,
		    COLS - INDENT1 - 2,
		    show_help, (void *) &_help,
		    (Callback_proc *) NULL, (void *) NULL,
		    (Callback_proc *) NULL, (void *) NULL,
		    NULL, opts, options, &selected,
		    M_RADIO | M_RADIO_ALWAYS_ONE | M_CHOICE_REQUIRED,
		    fkeys);

		if (is_ok(ch) != 0 || is_continue(ch) != 0) {

			if ((v_fdisk_get_type_by_index(indexes[selected]) ==
			    V_SUNIXOS) &&
			    v_fdisk_flabel_has_spart(disk) != 0) {

#define	CHANGE_PART_TYPE_WARNING   gettext(\
	"A Solaris fdisk partition already exists. Only one Solaris "\
	"partition can exist on a disk.\n\n")

				(void) simple_notice(win, F_OKEYDOKEY,
				    TITLE_WARNING,
				    CHANGE_PART_TYPE_WARNING);

			} else {

				/* set new partition type */
				(void) v_fdisk_set_part_type(part,
				    v_fdisk_get_type_by_index(selected));

				done = 1;

			}

		} else if (is_cancel(ch) != 0)
			break;

	}

	if (opts)
		free((void *) opts);

	(void) delwin(win);
	(void) clearok(curscr, TRUE);
	(void) touchwin(parent);
	(void) wnoutrefresh(parent);
	(void) clearok(curscr, FALSE);

}
