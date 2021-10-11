#ifndef lint
#pragma ident "@(#)inst_disk_edit.c 1.79 96/09/12 SMI"
#endif

/*
 * Copyright (c) 1991-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_disk_edit.c
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
#include <sys/fs/ufs_fs.h>

#include "pf.h"
#include "tty_pfc.h"
#include "v_types.h"
#include "v_check.h"
#include "v_disk.h"
#include "v_lfs.h"
#include "v_sw.h"
#include "inst_msgs.h"

static void _set_editor_props(WINDOW *, int);
static void _customize_disk_menu(WINDOW *, int, ChoiceItem *);
static int _update_field(WINDOW *, int, int, char *);
static int _can_preserve(int);

#ifdef ALT_BOOTDRIVE
static void _alternate_bootdrive(WINDOW * win);
#endif

/*
 * this remembers the currently selected display units for the disk editor.
 */
static V_Units_t _cur_units = V_MBYTES;

void
do_disk_edit(void)
{
	(void) customize_disks(stdscr, v_get_n_disks());
}

/*
 * internal helper function!
 *
 * format a string like this from the input args:
 *  "c1t0d0 (638 MB)"
 *
 * returns pointer to a static buffer which is overwritten on each call.
 *
 */
static char *
_fmt_label(char *diskname, int raw_cap, char *units)
{
	static char buf[256];	/* scratch pointer for sprintf() */

	/*
	 * format a string like this from the input args: "c1t0d0 (638 MB) "
	 */

	/* raw disk's size */
	(void) sprintf(buf, "%-8.8s (%d %s)", diskname, raw_cap, units);

	return (buf);
}

int
customize_disks(WINDOW * parent, int ndisks)
{

	int i;			/* first row of menu */
	int actual;
	char buf[128];		/* scratch pointer for sprintf() */
	char *ptr;		/* scratch pointer for gettext() */
	ChoiceItem *opts;
	static HelpEntry _help;

	_help.win = parent;
	_help.type = HELP_REFER;
	_help.title = "Select Disk to Customize Screen";

	opts = (ChoiceItem *) xcalloc(ndisks * sizeof (ChoiceItem));

	v_set_disp_units(V_MBYTES);
	/*
	 * load menu of editable disks, these are ones that the user has
	 * selected to 'use'
	 */
	for (i = 0, actual = 0; i < ndisks; i++) {

		if (v_get_disk_usable(i) == 1) {

			if (((ptr = v_get_disk_mountpts(i)) == (char *) NULL) &&
			    *ptr == '\0')
				ptr = gettext("none");

			(void) sprintf(buf, "%-22.22s  %-*.*s%s",
			    _fmt_label(v_get_disk_name(i), v_get_disk_size(i),
				v_get_disp_units_str()),
			    (int) strlen(ptr) >= 42 ? 37 : 42,
			    (int) strlen(ptr) >= 42 ? 37 : 42, ptr,
			    (int) strlen(ptr) >= 42 ? " ...>" : "");

			opts[actual].label = (char *) xstrdup(buf);
			opts[actual].help.win = _help.win;
			opts[actual].help.type = _help.type;
			opts[actual].help.title = _help.title;
			opts[actual].sel = -1;
			opts[actual].data = (void *) i;	/* real disk index */
			opts[actual].loc.c = INDENT1;
			actual++;

			/*
			 * XXX - look up which file system is `current' on
			 * current config display.  ie which one has focus.
			 * use this as the default current disk here
			 */
		}
	}

	ndisks = actual;

	if (ndisks == 0) {

		(void) simple_notice(parent, F_OKEYDOKEY,
		    CUSTOMIZE_DISK_FILESYS_TITLE,
		    CUSTOMIZE_DISK_FILESYS_NODISKS);

	} else if (ndisks == 1) {

		edit_disk(parent, (int) opts[0].data, 0);

	} else {

		_customize_disk_menu(parent, ndisks, opts);

	}

	/* cleanup memory */
	if (opts) {
		for (i = 0; i < ndisks; i++) {
			if (opts[i].label != (char *) NULL)
				free((void *) opts[i].label);
		}

		free((void *) opts);
	}
	return (0);

}

static void
_customize_disk_menu(WINDOW * parent, int ndisks, ChoiceItem * opts)
{
	WINDOW *win;
	char *ptr;		/* scratch pointer for gettext() */
	char buf[128];		/* scratch pointer for sprintf() */
	int top_row;		/* first row of menu */
	int last_row;		/* first row of menu */
	int cur;		/* remember last selection */

	int top;		/* index of first item displayed */
	int dirty = 1;
	int disks_per_page;
	int ch;
	unsigned long fkeys;

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);
	(void) werase(win);
	(void) wclear(win);

	wheader(win, CUSTOMIZE_DISK_FILESYS_TITLE);

	top_row = wword_wrap(win, HeaderLines, INDENT0, COLS - (2 * INDENT0),
	    CUSTOMIZE_DISK_FILESYS_ONSCREEN);
	top_row++;

	(void) sprintf(buf, "%-.10s (%s)",
	/* i18n: 10 characters maximum */
	    gettext("Disk"),
	/* i18n: 9 characters maximum */
	    gettext("Size"));

	(void) mvwprintw(win, top_row++, INDENT1, "%-22.22s  %-45.45s",
	    buf,
	/* i18n: 45 characters maximum */
	    gettext("Configured File Systems"));

	(void) mvwprintw(win, top_row++, INDENT1, "%-.*s",
	    10 + 9 + 45 + 2, EQUALS_STR);

	/*
	 * set up for menu
	 */
	cur = 0;
	top = 0;
	disks_per_page = LINES - top_row - FooterLines - 1;
	last_row = top_row + disks_per_page - 1;

	fkeys = F_OKEYDOKEY | F_CUSTOMIZE | F_HELP;

	/* process events */
	for (;;) {

		if (dirty) {

			(void) show_choices(win, ndisks, disks_per_page,
			    top_row, INDENT1, opts, top);

			scroll_prompts(win, top_row, 1, top, ndisks,
			    disks_per_page);

			wfooter(win, fkeys);

			dirty = 0;
		}
		/* highlight current */
		wfocus_on(win, opts[cur].loc.r, opts[cur].loc.c,
		    opts[cur].label);

		ch = wzgetch(win, fkeys);

		/* unhighlight */
		wfocus_off(win, opts[cur].loc.r, opts[cur].loc.c,
		    opts[cur].label);

		wnoutrefresh(win);

		if (is_ok(ch) != 0) {

			break;

		} else if (is_escape(ch) != 0) {

			continue;

		} else if (is_customize(ch) != 0) {

			/*
			 * get real disk index
			 */
			int index = (int) opts[cur].data;

			edit_disk(win, index, 0);

			/*
			 * update this disk's file systems
			 */
			if (((ptr = v_get_disk_mountpts(index)) ==
				(char *) NULL) && *ptr == '\0')
				ptr = gettext("none");

			(void) sprintf(buf, "%-22.22s  %-*.*s%s",
			    _fmt_label(v_get_disk_name(index),
				v_get_disk_size(index),
				v_get_disp_units_str()),
			    (int) strlen(ptr) >= 42 ? 37 : 42,
			    (int) strlen(ptr) >= 42 ? 37 : 42, ptr,
			    (int) strlen(ptr) >= 42 ? " ...>" : "");

			opts[cur].label = (char *)
			    xrealloc((void *) opts[cur].label, strlen(buf));

			(void) strcpy(opts[cur].label, buf);

			dirty = 1;

		} else if (is_help(ch) != 0) {

			do_help_index(win, opts[cur].help.type,
			    opts[cur].help.title);

		} else if (ch == U_ARROW || ch == D_ARROW ||
		    ch == CTRL_N || ch == CTRL_P) {

			/* move */
			if (ch == U_ARROW || ch == CTRL_P) {

				if (opts[cur].loc.r == top_row) {

					if (top) {	/* scroll down */
						cur = --top;
						dirty = 1;
					} else
						beep();	/* very top */

				} else
					cur--;

			} else if (ch == D_ARROW || ch == CTRL_N) {

				if (opts[cur].loc.r == last_row) {

					if ((cur + 1) < ndisks) {

						/* scroll up */
						top++;
						cur++;
						dirty = 1;

					} else
						beep();	/* bottom */

				} else {

					if ((cur + 1) < ndisks)
						cur++;
					else
						beep();	/* last, no wrap */
				}

			}
		} else
			beep();

	}

	(void) delwin(win);
	(void) clearok(curscr, TRUE);
	(void) touchwin(parent);
	(void) wnoutrefresh(parent);
	(void) clearok(curscr, FALSE);
}

/*
 * needed to keep state about whether the the disk has been re-loaded with
 * the xisting VTOC label
 */
static int _using_existing;

static int
_set_using_existing(int val)
{
	_using_existing = val;

	return (val);
}

static int
_get_using_existing(void)
{
	return (_using_existing);
}


typedef struct {
	HelpEntry help;
	NRowCol loc;
	FieldType type;
	char *value;
	int len;
	int maxlen;
	void *data;
} _Row_item_t;

typedef struct {

	/*
	 * a row in the disk editor is a vector of 5 items:
	 *	field for slice number
	 *	field for mount point
	 *	field for size
	 *	field for preserve
	 *	field for start cylinder
	 */
	_Row_item_t	f[5];

}	_DEditor_row_t;

static _DEditor_row_t *table = (_DEditor_row_t *) NULL;

#define		FLDS_PER_SLICE	5

/*
 * fields which display information about `currently focused' file system.
 */
static EditField filesys;
static EditField suggest;
static EditField require;

static void
_show_slices(WINDOW * w)
{
	int i;
	int r;
	int c;
	int disk;
	int used;
	int showcyls;
	int preserve;
	/*
	int free;
	int cyl;
	*/

	char sizebuf[16];
	char buf[128];
	char *mount;
	char *units;

	const int slice_width = 5;
	const int cyl_width = 10;
	const int preserve_width = 10;
	const int mount_width = 22;
	const int first_row = 6;

	write_debug(CUI_DEBUG_L1, "entering _show_slices()");
	(void) getsyx(r, c);

	v_set_disp_units(_cur_units);
	units = v_get_disp_units_str();
	disk = v_get_current_disk();
	showcyls = v_get_default_showcyls();

	if (v_has_preserved_slice(disk) || _get_using_existing())
		preserve = 1;
	else
		preserve = 0;

	/*
	 * show editor's `active' fields & prompts
	 */
	(void) sprintf(buf,
	    "%*.*s: %*s %*.*s: %4.4s %-4.4s %*.*s: %4.4s %-4.4s",
	/* i18n: 7 characters max */
	    7, 7, gettext("Entry"), 24, " ",
	/* i18n: 13 characters max */
	    13, 13, gettext("Recommended"), " ", units,
	/* i18n: 9 characters max */
	    9, 9, gettext("Minimum"), " ", units);

	(void) wmove(w, HeaderLines, 0);
	(void) wclrtoeol(w);
	(void) wmove(w, HeaderLines + 1, 0);
	(void) wclrtoeol(w);
	(void) wmove(w, HeaderLines + 2, 0);
	(void) wclrtoeol(w);
	(void) mvwprintw(w, HeaderLines + 1, 0, "%-*s", COLS, buf);

	/*
	 * setup scratch field for data entry of actual fields
	 */
	filesys.r = HeaderLines + 1;
	filesys.c = 9;		/* max width of 'entry' is 7 */
	filesys.len = 22;
	filesys.maxlen = MAXMNTLEN;
	filesys.type = LSTRING;
	filesys.value = (char *) NULL;

	/*
	 * setup fields for displaying Suggested and minimum sizes
	 */
	suggest.r = HeaderLines + 1;
	suggest.c = filesys.c + filesys.len + 2 + 13 + 2;
	/* 1 for colon, 1 for space, 13 for 'Recommended', */
	/* 2 more for  colon/space */

	require.r = HeaderLines + 1;
	/* i18n: 9 characters max */
	require.c = suggest.c + 4 + 7 + 9 + 1;
	/* 4 for suggest data, 6 for units str/spaces, */
	/* 9 for 'minimum' */
	/* 1 for colon, 1 for space */

	/*
	 * show editor's table of fields & column titles
	 */
	(void) mvwprintw(w, 4, 0, "%.*s", COLS, EQUALS_STR);

	(void) sprintf(sizebuf, "%5.5s (%-.5s)",
	/* i18n: 5 chars max */
	    gettext("Size"), units);

	if (showcyls && preserve) {
		(void) sprintf(buf,
		    "%*.*s  %-*.*s  %13s %*.*s  %*.*s %*.*s",
		/* i18n: 5 chars max */
		    slice_width, slice_width, gettext("Slice"),
		/* i18n: 25 chars max */
		    mount_width, mount_width, gettext("Mount Point"),
		    sizebuf,
		/* i18n: 10 chars max */
		    preserve_width, preserve_width,
		    gettext("Preserve"),
		/* i18n: 10 chars max */
		    cyl_width, cyl_width, gettext("Start Cyl"),
		/* i18n: 10 chars max */
		    cyl_width, cyl_width, gettext("End Cyl"));
	} else if (showcyls) {
		(void) sprintf(buf,
		    "%*.*s  %-*.*s  %13s %*.*s  %*.*s",
		/* i18n: 5 chars max */
		    slice_width, slice_width, gettext("Slice"),
		/* i18n: 25 chars max */
		    mount_width, mount_width, gettext("Mount Point"),
		    sizebuf,
		/* i18n: 10 chars max */
		    cyl_width, cyl_width, gettext("Start Cyl"),
		/* i18n: 10 chars max */
		    cyl_width, cyl_width, gettext("End Cyl"));
	} else if (preserve) {
		(void) sprintf(buf,
		    "%*.*s  %-*.*s  %13s %*.*s",
		/* i18n: 5 chars max */
		    slice_width, slice_width, gettext("Slice"),
		/* i18n: 25 chars max */
		    mount_width, mount_width, gettext("Mount Point"),
		    sizebuf,
		/* i18n: 10 chars max */
		    preserve_width, preserve_width,
		    gettext("Preserve"));
	} else {
		(void) sprintf(buf,
		    "%*.*s  %-*.*s  %13s",
		/* i18n: 5 chars max */
		    slice_width, slice_width, gettext("Slice"),
		/* i18n: 25 chars max */
		    mount_width, mount_width, gettext("Mount Point"),
		    sizebuf);
	}

	(void) mvwprintw(w, 5, 2, buf);

	/* show values for each slice  */
	for (i = 0, used = 0; i < N_Slices; i++) {
		write_debug(CUI_DEBUG_L1, "show values for slice #%d", i);

		mount = v_get_cur_mount_pt(i);

		/*
		 * sum up space allocated to file systems, do addition in
		 * terms of cylinders to avoid rounding errors.
		 */
		if ((strcmp(mount, "alts")) != 0 &&
		    (strcmp(mount, "overlap")) != 0) {

			used += v_get_cur_size(i);
		}

		if (showcyls && preserve)
			(void) mvwprintw(w, i + first_row, 4,
			    "%2d   %-*.*s%s       %7d      %s   %8d      %6d",
			    i,
			    mount_width, mount_width, mount,
			    ((int) strlen(mount) > mount_width) ? ">" : " ",
			    (int) v_get_cur_size(i),
			    ((_can_preserve(i) == 1)
				? (v_get_cur_preserved(i) == 1 ? Sel : Unsel) :
				    Clear),
			    (int) v_get_cur_start_cyl(i),
			    (int) v_get_cur_end_cyl(i));
		else if (showcyls)
			(void) mvwprintw(w, i + first_row, 4,
			    "%2d   %-*.*s%s       %7d     %6d      %6d",
			    i,
			    mount_width, mount_width, mount,
			    ((int) strlen(mount) > mount_width) ? ">" : " ",
			    (int) v_get_cur_size(i),
			    (int) v_get_cur_start_cyl(i),
			    (int) v_get_cur_end_cyl(i));
		else if (preserve)
			(void) mvwprintw(w, i + first_row, 4,
			    "%2d   %-*.*s%s       %7d      %s",
			    i,
			    mount_width, mount_width, mount,
			    ((int) strlen(mount) > mount_width) ? ">" : " ",
			    (int) v_get_cur_size(i),
			((_can_preserve(i) == 1)
			    ? (v_get_cur_preserved(i) == 1 ? Sel : Unsel) :
			    Clear));
		else
			(void) mvwprintw(w, i + first_row, 4,
			    "%2d   %-*.*s%s       %7d",
			    i,
			    mount_width, mount_width, mount,
			    ((int) strlen(mount) > mount_width) ? ">" : " ",
			    (int) v_get_cur_size(i));
	}

	i += (first_row);
	(void) mvwprintw(w, i++, 0, "%.*s", COLS, EQUALS_STR);

	/*
	 * free field: get in MB, then in Cyls. If MB == 0 && Cyls != 0, we
	 * have a rounding problem... show as "< 1"
	 *
	 * just calculate free by subtracting `used' from usable capacity.
	 * makes things look a little better to eliminate one source of
	 * rounding error
	 *
	free = v_get_space_avail(disk);

	v_set_disp_units(V_CYLINDERS);
	cyl = v_get_space_avail(disk);
	v_set_disp_units(_cur_units);

	if (free == 0 && (cyl != 0))
		(void) strcpy(buf, "< 1");
	else
		(void) sprintf(buf, "%d", free);
	 */

	if (disk_fdisk_req(first_disk())) {
		/*
		 * Raw size of the Solaris partition: including the boot/altsect
		 * slices
		 */
		(void) mvwprintw(w, i++, 2, "%31s:     %7d %s",
			/* i18n: 30 chars max */
			gettext("Solaris Partition Size"),
			v_get_sdisk_size(disk), units);

		/*
		 * Solaris partition size lost to the boot/altsect slices
		 */
		(void) mvwprintw(w, i++, 2, "%31s:     %7d %s",
			/* i18n: 30 chars max */
			gettext("OS Overhead"), (int) (v_get_sdisk_size(disk) -
			v_get_sdisk_capacity(disk)), units);
		++i;

		/*
		 * Usable Solaris partition size: largest fs possible
		 */
		(void) mvwprintw(w, i++, 2, "%31s:     %7d %s",
		/* i18n: 30 chars max */
		gettext("Usable Capacity"), v_get_sdisk_capacity(disk), units);
	} else {
		/*
		 * capacity of the drive: the largest filesystem possible on it
		 */
		(void) mvwprintw(w, i++, 2, "%31s:     %7d %s",
		/* i18n: 30 chars max */
		gettext("Capacity"), v_get_sdisk_capacity(disk), units);
	}

	/*
	 * used space, total of space allocated to slices, sizes were
	 * truncated
	 */
	(void) mvwprintw(w, i++, 2, "%31s:     %7d %s",
	/* i18n: 30 chars max */
	    gettext("Allocated"), used, units);

	/*
	 * if not working in cylinders, this is a potential apparent
	 * loss of space due to unit conversion and rounding.
	 *
	 * if necessary, display the apparent loss
	 */
	if (_cur_units != V_CYLINDERS) {

		/*
		 * if (used + free != capacity) we've lost something
		 * to rounding...
		 */
		if ((used + v_get_space_avail(disk) !=
			v_get_sdisk_capacity(disk))) {
				(void) mvwprintw(w, i++, 2, "%31s:     %7d %s",
				/* i18n: 30 chars max */
				gettext("Rounding Error"),
				v_get_sdisk_capacity(disk) -
				v_get_space_avail(disk) - used, units);
		}

	}
	(void) mvwprintw(w, i++, 2, "%31s:     %7d %s",
	/* i18n: 30 chars max */
	    gettext("Free"), v_get_space_avail(disk), units);

	/* clear next line - may have had "Free" text shown */
	(void) wmove(w, i, 0);
	(void) wclrtoeol(w);

	(void) wnoutrefresh(w);
	(void) setsyx(r, c);
	(void) doupdate();

	write_debug(CUI_DEBUG_L1, "leaving _show_slices()");
}

/*
 * initializing these structs got harder since the number of partitions per
 * disk is now a variable.
 */
static void
init_disk_table()
{
	int i;
	int row = 6;
	char buf[128];

	write_debug(CUI_DEBUG_L1, "entering init_disk_table()");

	if (table == (_DEditor_row_t *) NULL)
		table = (_DEditor_row_t *)
		    xcalloc(N_Slices * sizeof (_DEditor_row_t));

	for (i = 0; i < N_Slices; row++, i++) {
		write_debug(CUI_DEBUG_L1, "allocating table slice #d", i);

		/* slice field */
		table[i].f[0].help.type = HELP_NONE;
		table[i].f[0].help.title = "";
		table[i].f[0].loc.r = row;
		table[i].f[0].loc.c = 3;
		table[i].f[0].type = INSENSITIVE;

		(void) sprintf(buf, "%2d", i);
		table[i].f[0].value = (char *) xstrdup(buf);
		table[i].f[0].len = 2;
		table[i].f[0].maxlen = 2;

		/* mount point field */
		table[i].f[1].help = HelpGetTopLevelEntry();
		table[i].f[1].loc.r = row;
		table[i].f[1].loc.c = 9;
		table[i].f[1].type = LSTRING;

		table[i].f[1].value = (char *) NULL;
		table[i].f[1].len = 25;
		table[i].f[1].maxlen = MAXMNTLEN;

		/* Size field */
		table[i].f[2].help = HelpGetTopLevelEntry();
		table[i].f[2].loc.r = row;
		table[i].f[2].loc.c = 38;
		table[i].f[2].type = NUMERIC;

		table[i].f[2].value = (char *) NULL;
		table[i].f[2].len = 8;
		table[i].f[2].maxlen = 8;

		/* preserve field */
		table[i].f[3].help = HelpGetTopLevelEntry();
		table[i].f[3].loc.r = row;
		table[i].f[3].loc.c = 52;
		table[i].f[3].type = LSTRING;

		table[i].f[3].value = Unsel;
		table[i].f[3].len = 3;
		table[i].f[3].maxlen = 3;

		/* start cyl field */
		table[i].f[4].help = HelpGetTopLevelEntry();
		table[i].f[4].loc.r = row;
		table[i].f[4].loc.c = 58;
		table[i].f[4].type = NUMERIC;

		table[i].f[4].value = (char *) NULL;
		table[i].f[4].len = 8;
		table[i].f[4].maxlen = 8;

	}

	write_debug(CUI_DEBUG_L1, "leaving init_disk_table()");
}

/*
 * keep an editable copy of the disk partitioning, these are just buffers
 * for I/O and passing values back and forth between UI and disk library.
 *
 */
typedef struct {
	char start_cyl[25];
	char size[25];
	char mount_pt[MAXMNTLEN];
	char preserve[4];
} _DEditor_row_buf_t;

static _DEditor_row_buf_t *e_part = (_DEditor_row_buf_t *) NULL;

/*
 * loads editor table with current disk's values
 */
static void
_set_field_values(void)
{
	int i;
	int showcyls = v_get_default_showcyls();
	int preserve;


	if (v_has_preserved_slice(v_get_current_disk()) ||
	    _get_using_existing())
		preserve = 1;
	else
		preserve = 0;

	/*
	 * if first time... set up table and allocate space for storing
	 * field values
	 */
	if (e_part == (_DEditor_row_buf_t *) NULL) {
		e_part = (_DEditor_row_buf_t *) xcalloc(N_Slices *
		    sizeof (_DEditor_row_buf_t));
		init_disk_table();
	}
	for (i = 0; i < N_Slices; i++) {
		write_debug(CUI_DEBUG_L1, "set_field_values for slice %d", i);

		/* copy values for editing */
		(void) sprintf(e_part[i].size, "%-ld",
			(long) v_get_cur_size(i));
		(void) strcpy(e_part[i].mount_pt, v_get_cur_mount_pt(i));
		(void) strcpy(e_part[i].preserve,
		    (_can_preserve(i) ?
			(v_get_cur_preserved(i) == 1 ? Sel : Unsel) : Clear));
		(void) sprintf(e_part[i].start_cyl, "%-ld",
		    (long) v_get_cur_start_cyl(i));

		/* point the editable fields at the editable copies... */
		table[i].f[1].value = (char *) e_part[i].mount_pt;
		table[i].f[2].value = (char *) e_part[i].size;
		table[i].f[3].value = (char *) e_part[i].preserve;
		table[i].f[4].value = (char *) e_part[i].start_cyl;

		/* set proper column for the start cyl field */
		if ((showcyls == 1) && (preserve == 1)) {
			table[i].f[3].loc.c = 52;
			table[i].f[4].loc.c = 58;
		} else if (showcyls == 1)
			table[i].f[4].loc.c = 49;
	}

}

void
edit_disk(WINDOW * parent, int disk, int startat)
{
	WINDOW *win;

	unsigned long fkeys;
	char buf[228];
	void *chkpt;

	int slice;
	int fld;

	int ch;
	int err;
	int really_dirty = 1;
	int dirty = 1;
	int showcyls;
	int preserve;
	HelpEntry _help;

	char	bootdeviceName[32];
	char	part_char;
	int	dev_index;

	char	bootdev_buf[80];


	write_debug(CUI_DEBUG_L1, "Edit Disk");
	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);

	_help.win = win;
	_help.type = HELP_REFER;
	_help.title = "Customize Disks Screen";

	(void) v_set_current_disk(disk);
	(void) _set_using_existing(FALSE);

	/*
	 * this is kind of gross... want to checkpoint the current state of
	 * the disk so that `Cancel'ing edits doesn't require going down to
	 * the disk library and doing a `restore last committed' state.
	 *
	 * this is because the auto-partitioning never gets committed (since
	 * that messes up the preserve logic), but we don't want to loose
	 * the autopartitioning on a cancel...
	 */
	chkpt = v_checkpoint_disk(disk);

	fld = 1;		/* first field */
	slice = startat;	/* first slice */

	for (;;) {

		if (really_dirty == 1) {

			(void) werase(win);
			(void) wclear(win);

			write_debug(CUI_DEBUG_L1, "getting boot device info");
			(void) BootobjGetAttribute(CFG_CURRENT,
				BOOTOBJ_DISK, bootdeviceName,
				BOOTOBJ_DEVICE_TYPE, &part_char,
				BOOTOBJ_DEVICE, &dev_index,
				NULL);

			if (!streq(bootdeviceName, "")) {
				if (IsIsa("sparc") && dev_index != -1) {
					(void) sprintf(bootdev_buf,
						"%s: %-s%c%d",
						gettext("Boot Device"),
						bootdeviceName,
						part_char, dev_index);
				} else {
					(void) sprintf(bootdev_buf,
						"%s: %-s",
						gettext("Boot Disk"),
						bootdeviceName);
				}
			}
			write_debug(CUI_DEBUG_L1,
				"bootdev_buf = %s", bootdev_buf);

			/* show Editor screen title */
			(void) sprintf(buf, "%s %-s",
				DISK_EDIT_TITLE,
				v_get_disk_name(disk));
			write_debug(CUI_DEBUG_L1,
				"screen title = %s", buf);
			wheader(win, buf);

			/*
			 * Show boot device tag, if there is one.
			 * Print right below the title and lined up
			 * on it's left edge with the title.
			 */
			(void) mvwprintw(win, 1, 2, bootdev_buf);

			wnoutrefresh(win);

			fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP | F_OPTIONS;
			wfooter(win, fkeys);

			really_dirty = 0;
			dirty = 1;

			write_debug(CUI_DEBUG_L1,
				"leaving \"really dirty\" section");
		}
		if (dirty == 1) {

			write_debug(CUI_DEBUG_L1, "in \"dirty\" section");
			showcyls = v_get_default_showcyls();
			if (v_has_preserved_slice(disk) ||
			    _get_using_existing())
				preserve = 1;
			else
				preserve = 0;

			(void) _set_field_values();
			(void) _show_slices(win);

			dirty = 0;
			write_debug(CUI_DEBUG_L1, "leaving \"dirty\" section");
		}
		/*
		 * if current row has a default mount pnt, display any
		 * sizing information we have.
		 */
		write_debug(CUI_DEBUG_L1, "printing mount point info: %s",
			table[slice].f[1].value ?
			table[slice].f[1].value : "none");
		if ((table[slice].f[1].value != (char *) NULL) &&
		    (v_is_default_mount_point(table[slice].f[1].value))) {

			int m;

			/*
			 * show mount point size information above slice
			 * table set  data entry field with current row's
			 * parameters
			 */
			filesys.type = table[slice].f[1].type;
			filesys.maxlen = table[slice].f[1].maxlen;
			filesys.value = table[slice].f[1].value;

			(void) mvwprintw(win, filesys.r, filesys.c, "%-*.*s",
			    filesys.len, filesys.len, filesys.value);

			m = v_get_mntpt_size_hint(table[slice].f[1].value, 0);

			(void) mvwprintw(win, suggest.r, suggest.c, "%5d", m);

			m = v_get_mntpt_size_hint(table[slice].f[1].value, 1);

			(void) mvwprintw(win, require.r, require.c, "%5d", m);
			/*
			 * this provides the minimum size for ths
			 * filesystem` without rollup.
			 * v_get_mntpt_req_size(table[slice].f[1].value,
			 * &n);
			 */

		} else {

			/* clear out last mount point's info */
			(void) mvwprintw(win, filesys.r, filesys.c, "%-*.*s",
			    filesys.len, filesys.len, " ");

			(void) mvwprintw(win, suggest.r, suggest.c,
			    "%5.5s", " ");

			(void) mvwprintw(win, require.r, require.c,
			    "%5.5s", " ");

		}

		write_debug(CUI_DEBUG_L1,
			"positioning cursor and getting field input");

		/*
		 * position cursor and get field/input
		 */
		if (fld == 3) {

			wfocus_on(win, table[slice].f[fld].loc.r,
			    table[slice].f[fld].loc.c,
			    table[slice].f[fld].value);

			ch = wzgetch(win, fkeys);

			wfocus_off(win, table[slice].f[fld].loc.r,
			    table[slice].f[fld].loc.c,
			    table[slice].f[fld].value);

			if (ch == RETURN) {

				/*
				 * toggle preserved status
				 */
				err = V_NOERR;
				if (v_get_cur_preserved(slice) != 0) {
					(void) v_set_preserved(slice, 0);
				} else if (try_preserve(win, disk, slice,
					table[slice].f[1].value) == 1) {
					(void) v_set_preserved(slice, 1);
				} else
					err = V_CANTPRES;

				if (err == V_NOERR) {
					(void) _set_field_values();
					(void) _show_slices(win);
				}

				continue;

			}
		} else {

			ch = wget_field(win, table[slice].f[fld].type,
			    table[slice].f[fld].loc.r,
			    table[slice].f[fld].loc.c,
			    table[slice].f[fld].len,
			    table[slice].f[fld].maxlen,
			    table[slice].f[fld].value, fkeys);

			err = _update_field(win, slice, fld,
			    table[slice].f[fld].value);

			(void) _set_field_values();
			(void) _show_slices(win);

			if (err != V_NOERR)
				continue;

		}

		/*
		 * interpret input
		 */
		if (is_ok(ch) != 0) {

			if (v_sdisk_validate(disk) != V_OK &&
			    v_get_v_errno() != V_OK) {
				(void) simple_notice(win, F_OKEYDOKEY,
				    TITLE_ERROR, v_sdisk_get_err_buf());

				_show_slices(win);
				continue;

			} else {
				(void) v_commit_disk(disk, V_IGNORE_ERRORS);
				chkpt = v_free_checkpoint(chkpt);
			}

			break;

#ifdef notdef
		} else if (is_goback(ch) != 0) {

			inst_space_meter(win);
			_show_slices(win);
			continue;
#endif

		} else if (is_cancel(ch) != 0) {

			chkpt = v_restore_checkpoint(disk, chkpt);
			break;

		} else if (is_options(ch) != 0) {

			_set_editor_props(win, v_get_current_disk());
			showcyls = v_get_default_showcyls();
			really_dirty = 1;

			/*
			 * display may have changed, so skip any newly,
			 * `insensitive' fields
			 */
			if (fld == 4 && showcyls == 0) {
				--fld;	/* start cyl */
			}
			if (fld == 3 && preserve == 0) {
				--fld;	/* preserve */

			}
		} else if (is_help(ch) != 0) {

			do_help_index(win, _help.type, _help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else if (ch == D_ARROW || ch == CTRL_N || ch == CTRL_D) {

			/* move to next row */
			if (slice < (N_Slices - 1))
				++slice;
			else
				slice = 0;

			if (fld == 3)	/* preserve field ? */
				if (preserve == 0 ||
				    _can_preserve(slice) == 0) {
					--fld;	/* skip preserve */
				}
		} else if (ch == U_ARROW || ch == CTRL_P ||
		    ch == CTRL_U) {

			/* move to previous row */
			if (slice > 0)
				--slice;
			else
				slice = N_Slices - 1;

			if (fld == 3)	/* preserve field ? */
				if (preserve == 0 ||
				    _can_preserve(slice) == 0) {
					--fld;	/* skip preserve */
				}
		} else if (ch == L_ARROW || ch == CTRL_B) {

			/* move to previous field */
			if (fld > 1)
				--fld;
			else
				fld = FLDS_PER_SLICE - 1;

			/*
			 * skip `insensitive' fields
			 */
			if (showcyls == 0 && fld == 4) {
				--fld;	/* skip start cyl */
			}
			if (fld == 3)	/* preserve field ? */
				if (preserve == 0 ||
				    _can_preserve(slice) == 0) {
					--fld;	/* skip preserve */
				}
		} else if (ch == R_ARROW || ch == CTRL_F) {

			/* move to next field */
			if (fld < (FLDS_PER_SLICE - 1))
				++fld;
			else
				fld = 1;

			/*
			 * skip `insensitive' fields
			 */
			if (fld == 3)	/* preserve field ? */
				if (preserve == 0 ||
				    _can_preserve(slice) == 0) {
					++fld;	/* skip preserve */
				}
			if (showcyls == 0 && fld == 4) {

				fld = 1;	/* skip start cyl */

			}
		} else if (ch == TAB || ch == RETURN) {

			/* move to next field */
			++fld;

			if (fld == 3)	/* preserve field ? */
				if (preserve == 0 ||
				    _can_preserve(slice) == 0) {
					++fld;	/* skip preserve */
				}
			if (showcyls == 0 && fld == 4) {
				++fld;	/* skip start cyl */
			}
			if (fld == FLDS_PER_SLICE) {
				fld = 1;
				slice++;
			}
			if (slice == N_Slices)
				slice = 0;


		} else
			beep();
	}

	(void) delwin(win);
	(void) clearok(curscr, TRUE);
	(void) touchwin(parent);
	(void) wnoutrefresh(parent);
	(void) clearok(curscr, FALSE);
}

/*
 * Disk error messages to be used when editing the partitioning, soe errors
 * are specific to a field, some are generic and must be different depending
 * on which field generated it
 */
static char *
edit_slice_err_msg(int fld, char *val, int err)
{
	char buf[BUFSIZ];
	static char buf1[BUFSIZ];
	char	bootdiskName[32];
	char	part_char;
	int	dev_index;

	buf[0] = buf1[0] = '\0';

	switch (err) {

	case V_NODISK:
		(void) strcpy(buf1, gettext("There is no current disk."));
		break;

	case V_BADARG:

		switch (fld) {

		case 1:	/* mount point */
			(void) strcpy(buf, DISK_EDIT_INVALID_MOUNTPT);
			(void) sprintf(buf1, buf, val);
			break;

		case 2:	/* slice size */
			(void) strcpy(buf, DISK_EDIT_INVALID_SIZE);
			(void) sprintf(buf1, buf, val);
			break;

		case 3:
			break;

		case 4:
			(void) strcpy(buf, DISK_EDIT_INVALID_START_CYL);
			(void) sprintf(buf1, buf, val);
			break;

		default:
			break;

		}
		break;

	case V_NOSPACE:
		(void) strcpy(buf, DISK_EDIT_NO_SPACE_LEFT);
		(void) sprintf(buf1, buf, val, v_get_disp_units_str());
		break;

	case V_DUPMNT:
		(void) strcpy(buf, DISK_EDIT_DUP_MOUNTPT);
		(void) sprintf(buf1, buf, val);
		break;

	case V_CHANGED:
		(void) strcpy(buf1, DISK_EDIT_CHANGED);
		break;

	case V_CANTPRES:
		(void) strcpy(buf1, DISK_EDIT_CANT_PRESERVE);
		break;

	case V_PRESERVED:
		(void) strcpy(buf, DISK_EDIT_CANT_CHANGE);
		(void) sprintf(buf1, buf, val, v_get_disp_units_str());
		break;

	case V_BADDISK:
		(void) strcpy(buf1, gettext("Unknown disk error..."));
		break;

	case V_PATH_TOO_LONG:
		(void) strcpy(buf, DISK_EDIT_LONG_MOUNTPT);
		(void) sprintf(buf1, buf, val, (MAXMNTLEN - 1));
		break;

	case V_BOOTFIXED:
		(void) BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, bootdiskName,
			BOOTOBJ_DEVICE_TYPE, &part_char,
			BOOTOBJ_DEVICE, &dev_index,
			NULL);
		if (dev_index != -1 && IsIsa("sparc"))
			(void) strcpy(buf, "You cannot put the \"%s\" file system on a slice which is not the boot device.\n\n  This file system must go on the boot device (%s%c%d).");
		else
			(void) strcpy(buf, "You cannot put the \"%s\" file system on a slice which is not the boot device.\n\n  This file system must go on the boot device (%ss0).");


		if (!streq(bootdiskName, "") &&
			dev_index != -1 && IsIsa("sparc")) {
			(void) sprintf(buf1, buf, val, bootdiskName, part_char,
				dev_index);
		} else {
			(void) sprintf(buf1, buf, val, bootdiskName);
		}
		break;

	case V_ILLEGAL:
		(void) sprintf(buf1, "THIS IS A TEMORARY ERROR MESSAGE AWAITING FURTHER DEVELOPMENT:  S-Disk is in an illegal configuration");
		break;

	case V_ALTSLICE:
		(void) sprintf(buf1, "THIS IS A TEMORARY ERROR MESSAGE AWAITING FURTHER DEVELOPMENT:  cannot modify the alternate sector slice");
		break;

	case V_NOTSELECT:
		(void) sprintf(buf1, "THIS IS A TEMORARY ERROR MESSAGE AWAITING FURTHER DEVELOPMENT:  action requested not done; disk state not selected");
		break;

	case V_LOCKED:
		(void) sprintf(buf1, "THIS IS A TEMORARY ERROR MESSAGE AWAITING FURTHER DEVELOPMENT:  slice preserve state locked and can't be changed");
		break;

	case V_GEOMCHNG:
		(void) sprintf(buf1, "THIS IS A TEMORARY ERROR MESSAGE AWAITING FURTHER DEVELOPMENT:  disk geometry changed");
		break;

	case V_NOGEOM:
		(void) sprintf(buf1, "THIS IS A TEMORARY ERROR MESSAGE AWAITING FURTHER DEVELOPMENT:  no disk geometry defined");
		break;

	case V_NOFIT:
		(void) sprintf(buf1, "THIS IS A TEMORARY ERROR MESSAGE AWAITING FURTHER DEVELOPMENT:  slice/partition doesn't fit in disk segment");
		break;

	case V_NOSOLARIS:
		(void) sprintf(buf1, "THIS IS A TEMORARY ERROR MESSAGE AWAITING FURTHER DEVELOPMENT:  no Solaris partition configured on the disk");
		break;

	case V_BADORDER:
		(void) sprintf(buf1, "THIS IS A TEMORARY ERROR MESSAGE AWAITING FURTHER DEVELOPMENT:  slices/partitions not in a legal ordering");
		break;

	default:
		break;
	}

	return (buf1);
}

static int
_update_field(WINDOW * win, int slice, int fld, char *val)
{
	int err;
	char *str;
	int n;
	int cursize;
	int avail;

	err = V_NOERR;

	switch (fld % 5) {
	    case 1:		/* mount point */

		/* skip if nothing's changed */
		if (strcmp(val, v_get_cur_mount_pt(slice)) == 0)
			break;

		err = v_set_mount_pt(slice, val);

#ifdef ALT_BOOTDRIVE
		/* warn them about setting an alternate boot drive */
		if (strcmp(val, "/") == 0) {
			_alternate_bootdrive(win);
		}
#endif

		break;

	    case 2:		/* size */

		n = atoi(val);
		cursize = v_get_cur_size(slice);

		/*
		 * handle a couple of easy cases...
		 *	1. skip further tests if nothing's changed
		 *	2. skip further tests of new size is 0.
		 */
		if (n == cursize)
			break;

		if (n == 0) {
			err = v_set_size(slice, n, v_get_default_overlap());
			break;
		}

		/*
		 * size is changing, need to deal with rounding.
		 *
		 * get `free space' in terms of current units.
		 * if new value == `free space'
		 *	set new value in terms of absolute cylinders
		 * else if new value == current size + `free space'
		 *	set new value in terms of absolute cylinders
		 * else
		 * 	set new value in terms of absolute cylinders
		 */
		avail = v_get_space_avail(v_get_current_disk());

		if (n == avail) {

			/*
			 * set size to free space in terms of cylinders.
			 * restore units
			 */
			v_set_disp_units(V_CYLINDERS);
			avail = v_get_space_avail(v_get_current_disk());
			err = v_set_size(slice, avail,
			    v_get_default_overlap());
			v_set_disp_units(_cur_units);

		} else if ((cursize + n) == avail) {

			v_set_disp_units(V_CYLINDERS);
			avail = v_get_space_avail(v_get_current_disk());
			cursize = v_get_cur_size(slice);

			err = v_set_size(slice, avail + cursize,
			    v_get_default_overlap());

			v_set_disp_units(_cur_units);

		} else {

			err = v_set_size(slice, n, v_get_default_overlap());

		}

		break;

	    case 3:		/* preserve */

		break;

	    case 4:		/* start cylinder */

		n = atoi(val);

		if (n == v_get_cur_start_cyl(slice))
			break;

		err = v_set_start_cyl(slice, n, v_get_default_overlap());
		break;

	    default:
		break;

	}

	if (err != V_NOERR) {

		/* locked partitions are OK, so ignore V_LOCKED */
		if (v_get_v_errno() != V_LOCKED) {

			str = edit_slice_err_msg(fld % 5, val,
			    v_get_v_errno());

			(void) simple_notice(win, F_OKEYDOKEY,
			    gettext("Disk Editing Error"), str);

			return (err);
		} else
			err = V_NOERR;
	}
	return (err);

}

/*
 * this function depends on the sequence of enums in v_disk.h....
 */
static void
_set_editor_props(WINDOW * parent, int disk)
{
	WINDOW *win;

	int i;
	int ch;
	int nopts;
	int dirty;
	int row;
	int cur;
	int units_idx;
#ifdef notdef
	int overlap_idx;
#endif
	int showcyls_idx;
	int existing_idx;
	HelpEntry _help;

	unsigned long fkeys;
	ChoiceItem opts[15];

	/* temporary storage of properties until applied */
	V_Units_t tmp_units, orig_units;
	int tmp_showcyls, orig_showcyls;
	int tmp_overlap, orig_overlap;
	int tmp_existing;

	row = HeaderLines;

	(void) v_set_current_disk(disk);

	orig_units = tmp_units = v_get_disp_units();
	orig_showcyls = tmp_showcyls = v_get_default_showcyls();
	orig_overlap = tmp_overlap = v_get_default_overlap();
	tmp_existing = 0;

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);
	(void) werase(win);
	(void) wclear(win);

	_help.win = win;
	_help.type = HELP_REFER;
	_help.title = "Disk Editor Properties Screen";

	/* show title */
	wheader(win, gettext("Disk Editing Options"));

	i = 0;
	opts[i].help = _help;
	opts[i].sel = -1;
	opts[i].loc.c = INDENT1;
	opts[i].loc.r = row;
	opts[i].label = gettext("Show size in:");

	++i;
	++row;
	units_idx = i;		/* index of first units choice */
	opts[i].help = _help;
	opts[i].sel = (tmp_units == V_MBYTES);
	opts[i].loc.c = INDENT2;
	opts[i].loc.r = row;
	opts[i].label = (char *) gettext("MBytes");

	++i;
	++row;
	opts[i].help = _help;
	opts[i].sel = (tmp_units == V_CYLINDERS);
	opts[i].loc.c = INDENT2;
	opts[i].loc.r = row;
	opts[i].label = (char *) gettext("Cylinders");

	++i;
	row += 2;
	opts[i].help = HelpGetTopLevelEntry();
	opts[i].sel = -1;
	opts[i].loc.c = INDENT1;
	opts[i].loc.r = row;
	opts[i].label = gettext("Other Options:");

#ifdef notdef
	/* overlapping cylinders toggle item */
	++i;
	++row;

	overlap_idx = i;	/* index of overlapped slices toggle */
	opts[i].help = _help;
	opts[i].sel = (tmp_overlap == 1);
	opts[i].loc.c = INDENT2;
	opts[i].loc.r = row;
	opts[i].label = gettext("Enable overlap slice editing");
#endif

	/* cylinder boundaries toggle row */
	++i;
	++row;

	showcyls_idx = i;	/* index of start/end cylinders toggle */
	opts[i].help = _help;
	opts[i].sel = (tmp_showcyls == 1);
	opts[i].loc.c = INDENT2;
	opts[i].loc.r = row;
	opts[i].label = gettext("Show cylinder boundaries");

	/* set to existing toggle row */
	++i;
	++row;

	existing_idx = i;	/* index of existing slicing toggle */
	opts[i].help = _help;
	opts[i].sel = (tmp_existing == 1);
	opts[i].loc.c = INDENT2;
	opts[i].loc.r = row;
	opts[i].label = gettext("Load existing slices from VTOC label");

	nopts = i + 1;

	cur = 1;
	dirty = 1;

	fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;
	wfooter(win, fkeys);

	/* cope with input... */
	for (;;) {

		if (dirty) {
			for (i = 0; i < nopts; i++) {
				(void) wmove(win, opts[i].loc.r,
				    opts[i].loc.c - 4);
				(void) wclrtoeol(win);

				(void) mvwprintw(win, opts[i].loc.r,
				    opts[i].loc.c - 4, "%s %s",
				    (opts[i].sel == 1) ? Sel :
				    (opts[i].sel == 0) ? Unsel : Clear,
				    opts[i].label);

			}
			dirty = 1;
		}

		/* hilight current */
		wfocus_on(win, opts[cur].loc.r, opts[cur].loc.c - 4,
		    opts[cur].sel ? Sel : Unsel);

		ch = wzgetch(win, fkeys);

		if (is_help(ch) != 0) {

			do_help_index(win, _help.type, _help.title);

		} else if (is_ok(ch) != 0) {

			if (tmp_existing != 0) {

				/*
				 * warn the user that any edits will be lost
				 */
				if (yes_no_notice(win, F_OKEYDOKEY | F_CANCEL,
					F_OKEYDOKEY, F_CANCEL,
					TITLE_WARNING,
					DISK_EDIT_LOAD_ORIG_WARNING) ==
					F_OKEYDOKEY) {

					/* go ahead and apply */
					break;
				} else {
					/*
					 * turn off use existing choice &
					 * redisplay props
					 */
					opts[existing_idx].sel =
					    tmp_existing = 0;
				}
			} else {

				break;
			}

		} else if (is_cancel(ch) != 0) {

			break;

		} else if (is_escape(ch) != 0) {

			continue;

		} else if (sel_cmd(ch) != 0 || alt_sel_cmd(ch) != 0) {

			if (cur == 1 || cur == 2) {

				opts[units_idx + (int) tmp_units].sel = 0;

				tmp_units = (V_Units_t) cur - units_idx;

				opts[units_idx + (int) tmp_units].sel = 1;

#ifdef notdef
			} else if (cur == overlap_idx) {
				opts[cur].sel = tmp_overlap =
				    (tmp_overlap ? 0 : 1);
#endif
			} else if (cur == showcyls_idx) {
				opts[cur].sel = tmp_showcyls =
				    (tmp_showcyls ? 0 : 1);

			} else if (cur == existing_idx) {
				opts[cur].sel = tmp_existing =
				    (tmp_existing ? 0 : 1);

			}
			dirty = 1;

		} else if (fwd_cmd(ch) || bkw_cmd(ch)) {

			/* unhighlight current position */
			wfocus_off(win, opts[cur].loc.r, opts[cur].loc.c - 4,
			    opts[cur].sel ? Sel : Unsel);

			if (bkw_cmd(ch) != 0) {

				if (cur == units_idx) {
					beep();
				} else
					cur--;

				/* skip titles */
				if (opts[cur].sel == -1)
					cur--;

			} else if (fwd_cmd(ch) != 0) {

				if (cur == nopts - 1) {
					beep();
				} else
					cur++;

				/* skip titles */
				if (opts[cur].sel == -1)
					cur++;
			}
		} else
			beep();

	}

	/*
	 * update only if something has really been changed
	 */
	if (is_ok(ch) != 0 &&
	    (orig_units != tmp_units ||
		orig_showcyls != tmp_showcyls ||
		orig_overlap != tmp_overlap ||
		tmp_existing != 0)) {

		v_set_disp_units(tmp_units);

		if (tmp_existing != 0) {

			/*
			 * recover original slicing... if possible
			 */
			if (v_restore_orig_slices(disk) == V_FAILURE) {
				(void) simple_notice(win, F_OKEYDOKEY,
				    DISK_EDIT_ORIG_SLICES_TITLE,
				    DISK_EDIT_ORIG_SLICES_ERR);
			} else {
				(void) _set_using_existing(TRUE);
			}
		}

		/*
		 * these calls have no side effects, they just set the state
		 * values for the view libraries' concept of what the
		 * default properties are.
		 *
		 * Other functions actually cause the editor state changes.
		 */
		v_set_default_showcyls(tmp_showcyls);
		v_set_default_overlap(tmp_overlap);

		_cur_units = tmp_units;
		v_set_disp_units(_cur_units);
	}
	(void) delwin(win);
	(void) clearok(curscr, TRUE);
	(void) touchwin(parent);
	(void) wnoutrefresh(parent);
	(void) clearok(curscr, FALSE);

}


/*
 * need to determine if the slice can or cannot be preserved:
 *
 * if a slice's size and start cylinder are the same as their original values,
 * the slice is preserveable
 */
static int
_can_preserve(int slice)
{
	/*
	 * if a slice's size and start cylinder are the same as their
	 * original values, the slice is preserveable
	 */
	if ((v_get_orig_size(slice) > 0) &&
	    (v_get_orig_size(slice) == v_get_cur_size(slice)) &&
	    (v_get_orig_start_cyl(slice) == v_get_cur_start_cyl(slice)) &&
	    (strcmp(v_get_cur_mount_pt(slice), Overlap) != 0) &&
	    (strcmp(v_get_cur_mount_pt(slice), "swap") != 0))
		return (1);
	else
		return (0);
}

#ifdef ALT_BOOTDRIVE
static void
_alternate_bootdrive(WINDOW * win)
{
	char *dflt;
	char *configed;
	char buf[BUFSIZ];
	char buf1[BUFSIZ];

	dflt = v_get_default_bootdrive_name();
	configed = v_get_disk_name(v_get_current_disk());

	/*
	 * is the configured bootdrive (the one with / on it) the same as
	 * the default bootdrive?
	 */
	if (dflt && dflt[0] != '\0' && configed && configed[0] != '\0') {

		if (strcmp(configed, dflt) != 0) {

			(void) strcpy(buf1, ALT_BOOTDRIVE_WARNING);
			(void) sprintf(buf, buf1, dflt, configed);

			(void) simple_notice(win, F_CONTINUE,
				TITLE_WARNING, buf);
		}
	}
}
#endif
