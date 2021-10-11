#ifndef lint
#pragma ident "@(#)inst_bootdisk.c 1.61 96/03/07 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_bootdisk.c
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

int select_boot_device(WINDOW *, char *);
static int	set_boot_device(WINDOW *, int);
int UpdatePromQuery(void);

static int	device_index = -1; /* initialize to ANY choice */
typedef struct disk_device DiskDevice;
struct disk_device {
	char	*disk;
	char	**devices;
};

typedef struct {
	HelpEntry help;
	NRowCol loc;
	FieldType type;
	Disk_t	*disk_ptr;
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

#define	PFC_BOOTDISK_CHANGE_TEXT gettext(\
	"On this screen you can select the disk for installing the root (/) " \
	"file system of the Solaris software. %s")
#define	PFC_BOOTDISK_NOPREF	gettext(\
	"If you choose No Preference, the Solaris installation program " \
	"will choose a %s for you from the disks listed.")
#define	PFC_ORIGINAL_BOOT_DISK	gettext(\
	"Original Boot Disk : %s")
#define	PFC_ORIGINAL_BOOT_DEVICE	gettext(\
	"Original Boot Device : %s%c%d")

#define	PFC_ANY_CHOICE	gettext(\
	"No Preference")
#define	PFC_SELECT_BOOT_DEVICE gettext(\
	"On this screen you can select the specific %s for the root (/) " \
	"file system. %s")
#define	PFC_SELECT_BOOT_DEVICE_ANY	gettext(\
"If you choose Any of the Above, the Solaris installation program will choose " \
"a %s for you.")

#define	UPDATE_STR gettext(\
	"Update PROM")
#define	NO_UPDATE_STR gettext(\
	"Don't Update PROM")
#define	PROM_MSG	gettext( \
	"Do you want to update the system's hardware (EEPROM) to always boot " \
	"from %s?")
#define	PROM_SELECTED_DISK	gettext(\
	"the boot device selected by the Solaris installation program")
#define	PROM_MSG1	gettext( \
	"Do you want the system's hardware (EEPROM) to be changed to always boot " \
	"from the selected boot disk (%s)?")

static int _do_bootdisk_menu(int);

static int	numDisks;

parAction_t
do_choose_bootdisk()
{

	int		ret;
	int		done;
	parAction_t	retcode;

	int		count;
	Disk_t		*dp;


	count = 0;
	WALK_DISK_LIST(dp) {
		if (disk_okay(dp) && disk_selected(dp)) {
			count++;
			numDisks = count;
		}
	}

	if (numDisks > 1)
		numDisks = numDisks + 1;

	done = 0;

	while (!done) {

		if ((ret = _do_bootdisk_menu(numDisks)) == parAExit) {

			if (confirm_exit(stdscr) == 1) {
				retcode = parAExit;	/* exit */
				done = 1;
			} else {
				continue;
			}

		} else if (ret == parAGoback) {
			wstatus_msg(stdscr, PLEASE_WAIT_STR);
			retcode = parAGoback;
			done = 1;
		} else if (ret == parAContinue) {
			retcode = parAContinue;
			done = 1;
		}

	}

	return (retcode);

}

/*
 * free any existing disktable labels which have been strdup'ed
 */
static void
_free_disktable_labels(int ndisks, _Disk_Row_t * dtable)
{
	register int i;

	if (dtable != (_Disk_Row_t *) NULL) {

		for (i = 0; i < ndisks; i++) {

			if (dtable[i].fld[1].label != (char *) NULL)
				free((void *) dtable[i].fld[1].label);

		}

	}
}

static void
show_disklist(WINDOW * w, int max, int npp, int row,
    _Disk_Row_t * dtable, int first)
{
	register int i;		/* counts number displayed */
	register int j;		/* scratch index counter */
	register int r;		/* row counter */

	for (i = 0, r = row, j = first;
	    (i < npp) && (j < max);
	    i++, r++, j++) {

		(void) mvwprintw(w, r, INDENT1, "%s %s",
		    dtable[j].fld[0].label,
		    dtable[j].fld[1].label);

		dtable[j].fld[0].loc.r =
		    dtable[j].fld[1].loc.r =
		    dtable[j].fld[2].loc.r = (int) r;
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
_newfmt_label(char *diskname)
{
	static char buf1[32];		/* scratch pointer for sprintf() */

	(void) sprintf(buf1, "%-8.8s", diskname);

	return (buf1);
}

static char *
_fmt_any_label(char *anylabel)
{
	static char buf2[32];		/* scratch pointer for sprintf() */

	(void) sprintf(buf2, "%-30.30s", anylabel);

	return (buf2);
}


static int
_do_bootdisk_menu(int ndisks)
{
	char	diskname[32];
	int i;			/* scratch counter */
	int top_row;		/* first row of menu */
	int last_row;		/* last row of menu */
	int cur;		/* which disk */
	int	last_selected;
	int field;		/* which field */
	int r, c;		/* cursor location */

	int top;		/* index of first item displayed */
	int dirty = 1;
	int disks_per_page;
	int ch;
	char *cp;
	char *any;
	Disk_t	*dp;
	static char	part_char;
	char	tmp_buf[150];
	char	buf[300];
	unsigned long fkeys;
	_Disk_Row_t *dtable;
	DiskDevice	*dd;
	int	dcount;
	Disk_t	*bootdisk;
	char	bootname[32];
	static int	boot_index;
	static char	boot_char;
	int	update, ret;
	char	orig_buf[200];
	int	index;

	dd = (DiskDevice *) xcalloc(ndisks * sizeof (DiskDevice));

	/*
	 * load up table of disks
	 */
	dtable = (_Disk_Row_t *) xcalloc(ndisks * sizeof (_Disk_Row_t));

	i = 0;
	WALK_DISK_LIST(dp) {
		if (disk_selected(dp)) {

			v_set_disp_units(V_MBYTES);

			dtable[i].fld[0].help.win = stdscr;
			dtable[i].fld[0].help.type = HELP_REFER;
			dtable[i].fld[0].help.title =
			    "Select Boot Disk";
			dtable[i].fld[0].type = LSTRING;
			dtable[i].fld[0].loc.c = INDENT1;

			(void) DiskobjFindBoot(CFG_CURRENT, &bootdisk);
			if (bootdisk != NULL && (bootdisk == dp) &&
				BootobjIsExplicit(CFG_CURRENT,
					BOOTOBJ_DISK_EXPLICIT)) {
				dtable[i].fld[0].label = Sel;	/* "[x]" */
				last_selected = i;
			} else if ((bootdisk == NULL) && i == 0 &&
				BootobjIsExplicit(CFG_CURRENT,
					BOOTOBJ_DISK_EXPLICIT)) {
				dtable[i].fld[0].label = Sel;	/* "[x]" */
				last_selected = i;
			} else if (ndisks == 1 && i == 0) {
				dtable[i].fld[0].label = Sel;	/* "[x]" */
				last_selected = i;
			} else {
				dtable[i].fld[0].label = Unsel;	/* "[ ]" */
			}
			dtable[i].fld[0].prompt =
			    USE_DISK_NOT_SELECTED_PROMPT;

			cp = (char *) xmalloc(strlen(disk_name(dp) + 1));
			cp[0] = '\0';
			cp = _newfmt_label(disk_name(dp));

			dtable[i].fld[1].label = xstrdup(cp);
			dtable[i].fld[1].disk_ptr = dp;
			if (dtable[i].fld[0].label == Sel)
					dd->disk = disk_name(dp);
			dtable[i].fld[1].prompt = "";
			dtable[i].fld[1].type = LSTRING;
			dtable[i].fld[1].loc.c = INDENT1 + 4;
			dtable[i].fld[1].help.win = stdscr;
			dtable[i].fld[1].help.type = HELP_NONE;
			dtable[i].fld[1].help.title = "";

			dtable[i].fld[2].help.win = stdscr;
			dtable[i].fld[2].help.type = HELP_NONE;
			dtable[i].fld[2].help.title = "";
			dtable[i].fld[2].label = "";
			dtable[i].fld[2].prompt = "";
			dtable[i].fld[2].type = INSENSITIVE;
			dtable[i].fld[2].loc.c = INDENT1 + 4 +
			    strlen(dtable[i].fld[1].label) + 2;
			i++;
		}
	}

	if (ndisks > 1) {
			dtable[i].fld[0].help.win = stdscr;
			dtable[i].fld[0].help.type = HELP_TOPIC;
			dtable[i].fld[0].help.title =
			    "Device Naming Conventions";
			dtable[i].fld[0].type = LSTRING;
			dtable[i].fld[0].loc.c = INDENT1;

			if (BootobjIsExplicit(CFG_CURRENT,
						BOOTOBJ_DISK_EXPLICIT)) {
				dtable[i].fld[0].label = Unsel;	/* "[ ]" */
			} else {
				dtable[i].fld[0].label = Sel;	/* "[x]" */
				last_selected = i;
			}

			dtable[i].fld[0].prompt =
			    USE_DISK_NOT_SELECTED_PROMPT;


			any = (char *) xmalloc((strlen(APP_NOPREF_CHOICE) + 1));
			any[0] = '\0';
			any = _fmt_any_label(APP_NOPREF_CHOICE);

			dtable[i].fld[1].label = xstrdup(any);
			dtable[i].fld[1].disk_ptr = NULL;
			dtable[i].fld[1].prompt = "";
			dtable[i].fld[1].type = LSTRING;
			dtable[i].fld[1].loc.c = INDENT1 + 4;
			dtable[i].fld[1].help.win = stdscr;
			dtable[i].fld[1].help.type = HELP_NONE;
			dtable[i].fld[1].help.title = "";

			dtable[i].fld[2].help.win = stdscr;
			dtable[i].fld[2].help.type = HELP_NONE;
			dtable[i].fld[2].help.title = "";
			dtable[i].fld[2].label = "";
			dtable[i].fld[2].prompt = "";
			dtable[i].fld[2].type = INSENSITIVE;
			dtable[i].fld[2].loc.c = INDENT1 + 4 +
			    strlen(dtable[i].fld[1].label) + 2;
			i++;
	}

	(void) BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, diskname,
			BOOTOBJ_DEVICE_TYPE, &part_char,
			BOOTOBJ_DEVICE, &index,
			NULL);

	if (part_char == 's') {
		if (ndisks > 1)	{
			(void) sprintf(tmp_buf, PFC_BOOTDISK_NOPREF, APP_SLICE);
			(void) sprintf(buf, PFC_BOOTDISK_CHANGE_TEXT, tmp_buf);
		} else {
			(void) sprintf(buf, PFC_BOOTDISK_CHANGE_TEXT, "");
		}

	} else {
		if (ndisks > 1 && IsIsa("ppc")) {
			(void) sprintf(tmp_buf, PFC_BOOTDISK_NOPREF, APP_DISK);
			(void) sprintf(buf, PFC_BOOTDISK_CHANGE_TEXT, tmp_buf);
		} else if (ndisks > 1 && IsIsa("i386")) {
			(void) sprintf(tmp_buf, PFC_BOOTDISK_NOPREF, APP_DISK);
			(void) sprintf(buf, PFC_BOOTDISK_CHANGE_TEXT, tmp_buf);
		} else if (ndisks == 1) {
			(void) sprintf(buf, PFC_BOOTDISK_CHANGE_TEXT, "");
		}
	}

	(void) BootobjGetAttribute(CFG_EXIST,
			BOOTOBJ_DISK, bootname,
			BOOTOBJ_DEVICE_TYPE, &part_char,
			BOOTOBJ_DEVICE, &boot_index,
			NULL);

	if (bootname != NULL && !IsIsa("ppc")) {
		if (index != -1) {
			(void) sprintf(orig_buf, PFC_ORIGINAL_BOOT_DEVICE, bootname,
						boot_char, boot_index);
		} else {
			(void) sprintf(orig_buf, PFC_ORIGINAL_BOOT_DISK, bootname);
		}
	}

	(void) werase(stdscr);
	(void) wclear(stdscr);
	wheader(stdscr, TITLE_SELECT_BOOT_DISK);

	top_row = HeaderLines;

	top_row = wword_wrap(stdscr, top_row, INDENT0, COLS - (2 * INDENT0),
	    buf);


	if (diskname != NULL) {
		top_row++;
		top_row = wword_wrap(stdscr, top_row, INDENT0, COLS - (2 * INDENT0),
		    orig_buf);
	}


	/* print headings */
	top_row++;
	(void) mvwprintw(stdscr, top_row, INDENT1 + 4, "%-23.23s",
	    gettext("Disk"));

	++top_row;
	(void) mvwprintw(stdscr, top_row, INDENT1, "%-.*s", 23 + 3 + 4,
	    EQUALS_STR);

	/* setup menu variables */
	++top_row;
	cur = 0;
	field = 0;
	top = 0;
	disks_per_page = LINES - top_row - FooterLines - 3;
	last_row = top_row + disks_per_page - 1;


	if (!IsIsa("sparc"))
		fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;
	else
		fkeys = F_OKEYDOKEY | F_CANCEL | F_EDIT | F_HELP;

	wfooter(stdscr, fkeys);

	for (;;) {

		if (dirty) {

			if (ndisks > 1) {
				(void) show_disklist(stdscr, ndisks, disks_per_page,
				    top_row, dtable, top);
				scroll_prompts(stdscr, top_row, 1, top, ndisks,
				    disks_per_page);
			} else {
				(void) show_disklist(stdscr, ndisks, disks_per_page,
				    top_row, dtable, top);
				scroll_prompts(stdscr, top_row, 1, top, ndisks,
				    disks_per_page);
			}

			dirty = 0;
		}
		/*
		 * drop a hint to the user as to how to get to the
		 * the boot device selection screen
		 *
		 * also, make sure disk is OK before doing this....
		 */
/*		if (v_get_disk_status(cur) != V_DISK_NOTOKAY &&
 *		    v_get_disk_selected(cur) != 0) {
 */
/*		if (v_get_disk_status(cur) != V_DISK_NOTOKAY ) { */
		if (dtable[cur].fld[1].disk_ptr != NULL && IsIsa("sparc")) {
			(void) mvwprintw(stdscr, dtable[cur].fld[2].loc.r,
			    dtable[cur].fld[2].loc.c,
			    gettext("(F4 to select boot device)"));

		} else {

			(void) wmove(stdscr, dtable[cur].fld[2].loc.r,
			    dtable[cur].fld[2].loc.c);
			(void) wclrtoeol(stdscr);

		}

		/* highlight current */
		wfocus_on(stdscr, dtable[cur].fld[field].loc.r,
		    dtable[cur].fld[field].loc.c,
		    dtable[cur].fld[field].label);


/*		if (dtable[cur].fld[1].disk_ptr != NULL) {
 *			dd->disk = disk_name(dtable[cur].fld[1].disk_ptr);
 *			write_debug(CUI_DEBUG_L1,
 *				"The selected disk is %s\n", dd->disk);
 *		} else {
 *			dd->disk = NULL;
 *			write_debug(CUI_DEBUG_L1,
 *				"The selected disk is ANY\n");
 *		}
 */

		(void) getsyx(r, c);
		(void) wnoutrefresh(stdscr);
		(void) setsyx(r, c);
		(void) doupdate();

		ch = wzgetch(stdscr, fkeys);

		if ((field == 0) &&
		    ((sel_cmd(ch) != 0) || (alt_sel_cmd(ch) != 0))) {

			if (dtable[cur].fld[0].label == Sel) {
				dtable[cur].fld[0].label = Unsel;
			} else {
				if (last_selected != cur) {
	 				dtable[cur].fld[0].label = Sel;
					if (dtable[cur].fld[1].disk_ptr != NULL) {
						dd->disk =
						disk_name(dtable[cur].fld[1].disk_ptr);
						write_debug(CUI_DEBUG_L1,
							"The selected disk is %s\n",
								dd->disk);
					} else {
						dd->disk = NULL;
						write_debug(CUI_DEBUG_L1,
							"The selected disk is ANY\n");
					}

	 				dtable[last_selected].fld[0].label = Unsel;
					last_selected = cur;
				} else if (last_selected == cur) {
					dtable[cur].fld[0].label = Sel;
					if (dtable[cur].fld[1].disk_ptr != NULL) {
						dd->disk =
						disk_name(dtable[cur].fld[1].disk_ptr);
						write_debug(CUI_DEBUG_L1,
							"The selected disk is %s\n",
								dd->disk);
					} else {
						dd->disk = NULL;
						write_debug(CUI_DEBUG_L1,
							"The selected disk is ANY\n");
					}
					last_selected = cur;

				}
			}

			dirty = 1;

			/*
			 * make sure unsuable disks are not selectable...
			 * this condition is caught by prepare_disk()...
			 * this may change, so comment it out...
			 *
			 * if (v_get_disk_status(cur) == V_DISK_NOTOKAY) {
			 * beep(); continue; }
			 */

		} else if (is_ok(ch) != 0 || is_continue(ch) != 0) {
			if (dd->disk != NULL) {
				write_debug(CUI_DEBUG_L1, "is_ok boot disk is %s",
								dd->disk);
			} else {
				write_debug(CUI_DEBUG_L1, "selected no preference");
			}

			write_debug(CUI_DEBUG_L1, "selected device index is %d",
						device_index);

			BootobjSetAttribute(CFG_CURRENT,
				BOOTOBJ_DISK, dd->disk,
				BOOTOBJ_DISK_EXPLICIT, dd->disk == NULL ? 0 : 1,
				BOOTOBJ_DEVICE, device_index,
				BOOTOBJ_DEVICE_EXPLICIT, device_index == -1 ? 0 : 1,
				NULL);
			(void) BootobjCommit();
			(void) BootobjGetAttribute(CFG_CURRENT,
					BOOTOBJ_PROM_UPDATEABLE, &update,
					NULL);
			if (update == 1) {
				ret = UpdatePromQuery();
				if (ret == TRUE) {
					(void) BootobjSetAttribute(CFG_CURRENT,
						BOOTOBJ_PROM_UPDATE, 1,
						NULL);
				} else if (ret == FALSE) {
					(void) BootobjSetAttribute(CFG_CURRENT,
						BOOTOBJ_PROM_UPDATE, 0,
						NULL);
				}
			}
			break;

		} else if (is_cancel(ch) != 0) {
			(void) BootobjRestore(CFG_COMMIT);
			break;

		} else if (is_edit(ch) != 0) {
			/* (fkeys & F_EDIT) && */
			if (dd->disk != NULL && IsIsa("sparc")) {
				device_index = select_boot_device(stdscr, dd->disk);
				dirty = 1;
			} else {
				beep();
			}

		} else if (is_help(ch) != 0) {

			do_help_index(dtable[cur].fld[field].help.win,
			    dtable[cur].fld[field].help.type,
			    dtable[cur].fld[field].help.title);

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
			wfocus_off(stdscr, dtable[cur].fld[field].loc.r,
			    dtable[cur].fld[field].loc.c,
			    dtable[cur].fld[field].label);

/*			last = cur;  */

			/* unhint, bleah! */
			(void) wmove(stdscr, dtable[cur].fld[2].loc.r,
			    dtable[cur].fld[2].loc.c);
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

				if (dtable[cur].fld[field].loc.r ==
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

				if (dtable[cur].fld[field].loc.r ==
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
	if (dtable) {
		_free_disktable_labels(ndisks, dtable);
		free((void *) dtable);
	}

	if (is_ok(ch) != 0 || is_continue(ch) != 0)
		return (parAContinue);	/* continue */
	else if (is_goback(ch) != 0 || is_cancel(ch) !=0)
		return (parAGoback);	/* go back */
	else			/* if is_exit(ch) != 0) */
		return (parAExit);	/* exit */

}

int
select_boot_device(WINDOW *parent, char *selected_disk)
{
	char		*opts[9];
	char		buf[128];
	
	int		(*actions[9])(WINDOW *, int);
	int		nopts;
	int		i, j;
	int		ch;
	int		row;
	int		selected_device = 0;
	static char		part_char;
	char		diskname[32];
	int		index;
	int		part_count;

	u_long		selected;
	u_int		fkeys;
	HelpEntry	_help;
	WINDOW		*w;
	char		*label_buf;
	char		*tmp_buf;
	char		*buf1;
	char		bootname[32];
	static char		boot_char;
	static int		boot_index;


	w = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(w, BODY);

	_help.win = w;
	_help.type = HELP_REFER;
	_help.title = "Select Root Location";

	if (IsIsa("i386")) {
		part_count = FD_NUMPART;
	} else {
		part_count = NUMPARTS;
	}

	BootobjGetAttribute(CFG_CURRENT,
		BOOTOBJ_DEVICE_TYPE, &part_char,
		NULL);

	BootobjSetAttribute(CFG_CURRENT,
		BOOTOBJ_DISK, selected_disk,
		BOOTOBJ_DISK_EXPLICIT, 1,
		NULL);


	nopts = 0;

	write_debug(CUI_DEBUG_L1, "part count is %d", part_count);
	for (j = 0; j < part_count; j++) {
		(void) sprintf(buf, "%s%c%d", selected_disk, part_char, j);
		write_debug(CUI_DEBUG_L1, "nopts is %d", nopts);
		opts[nopts] = xstrdup(buf);
		actions[nopts] = set_boot_device;
		nopts++;
	}
	/*
	 * if we're on sparc, then show the Any of the Above
	 * choice for the slices... this is not shown for x86 or ppc
	 * since we never get to this screen
	 */
	if (IsIsa("sparc")) {
		(void) sprintf(buf, "%s", gettext("Any of the Above"));
		opts[nopts] = xstrdup(buf);
		actions[nopts] = set_boot_device;
		nopts++;
	}

	selected = 0;

	(void) werase(w);
	(void) wclear(w);

	label_buf = (char *) xmalloc(strlen(PFC_SELECT_BOOT_DEVICE) +
					strlen(PFC_SELECT_BOOT_DEVICE_ANY) +
					(2 * strlen(APP_SLICE)) +
					(2 * strlen(APP_PARTITION)));
	tmp_buf = (char *) xmalloc(strlen(PFC_SELECT_BOOT_DEVICE_ANY) +
					(2 * strlen(APP_SLICE)) +
					(2 * strlen(APP_PARTITION)));

	BootobjGetAttribute(CFG_CURRENT,
		BOOTOBJ_DEVICE_TYPE, &part_char,
		BOOTOBJ_DISK, diskname,
		BOOTOBJ_DEVICE, &index,
		NULL);
	if (part_char == 's') {
		(void) sprintf(tmp_buf, PFC_SELECT_BOOT_DEVICE_ANY, APP_SLICE);
		(void) sprintf(label_buf, PFC_SELECT_BOOT_DEVICE,
					APP_SLICE, tmp_buf);
	} else {
		(void) sprintf(label_buf, PFC_SELECT_BOOT_DEVICE,
					APP_PARTITION, "");
	}

	wheader(w, TITLE_SELECT_BOOT_DEVICE);
	row = HeaderLines;
	row = wword_wrap(w, row, INDENT0, COLS - (2 * INDENT0),
		label_buf);
	++row;

	(void) BootobjGetAttribute(CFG_EXIST,
			BOOTOBJ_DISK, bootname,
			BOOTOBJ_DEVICE_TYPE, &boot_char,
			BOOTOBJ_DEVICE, &boot_index,
			NULL);


	if (diskname != NULL && !IsIsa("ppc")) {
		if (index != -1) {
			buf1 = (char *) xmalloc(strlen(PFC_ORIGINAL_BOOT_DEVICE) + 32);
			(void) sprintf(buf1, PFC_ORIGINAL_BOOT_DEVICE, bootname,
						boot_char, boot_index);
		} else {
			buf1 = (char *) xmalloc(strlen(PFC_ORIGINAL_BOOT_DISK) + 32);
			(void) sprintf(buf1, PFC_ORIGINAL_BOOT_DISK, bootname);
		}
		row = wword_wrap(w, row, INDENT0, COLS - (2 * INDENT0), buf1);
	}
	row++;


	if (IsIsa("i386")) {
		selected = 0;
	} else {
		if (BootobjIsExplicit(CFG_CURRENT, BOOTOBJ_DEVICE_EXPLICIT))
			selected = index;
		else
			selected = nopts - 1;
	}

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

	(void) delwin(w);

	if (parent != (WINDOW *) NULL) {
		(void) clearok(curscr, TRUE);
		(void) touchwin(parent);
		(void) wnoutrefresh(parent);
		(void) clearok(curscr, FALSE);
	}


	if (is_ok(ch) != 0 || is_continue(ch) != 0) {
		(void) BootobjCommit();
		/* call appropriate action */
 		if (selected < nopts)
			return (actions[selected](w, selected));
		else
			return (0);

	} else /* if (is_cancel(ch) != 0 || is_exit(ch) != 0) */ {
		(void) BootobjRestore(CFG_COMMIT);
		/* return the default selection */
		return (0);
	}


}

/* ARGUSED1 */
static int
set_boot_device(WINDOW *win, int index)
{
	write_debug(CUI_DEBUG_L1,
		"The selected device index is %d\n", index);
	if (IsIsa("i386")) {
		if (index ==  FD_NUMPART) {
			write_debug(CUI_DEBUG_L1, "setting device to -1 for ANY choice");
			index = -1;
		}
	} else {
		if (index == NUMPARTS) {
			write_debug(CUI_DEBUG_L1, "setting device to -1 for ANY choice");
			index = -1;
		}
	}
	write_debug(CUI_DEBUG_L1, "setting device to %d", index);
	BootobjSetAttribute(CFG_CURRENT,
		BOOTOBJ_DEVICE, index,
		BOOTOBJ_DEVICE_EXPLICIT, index == -1 ? 0 : 1,
		NULL);
	(void) BootobjCommit();
	return(index);
}

int
UpdatePromQuery(void)
{
	UI_MsgStruct    *msg_info;
	int             answer;
	char		*buf;
	char		part_char;
	int		index;
	char		disk_name[32];
	char		tmp_buf[40];
	int		disk_explicit, device_explicit;


	write_debug(CUI_DEBUG_L1, "entering UpdatePromQuery");
	buf = (char *) xmalloc(strlen(PROM_MSG) + 60);

	(void) BootobjGetAttribute(CFG_COMMIT,
			BOOTOBJ_DISK, disk_name,
			BOOTOBJ_DEVICE_TYPE, &part_char,
			BOOTOBJ_DEVICE, &index,
			BOOTOBJ_DISK_EXPLICIT, &disk_explicit,
			BOOTOBJ_DEVICE_EXPLICIT, &device_explicit,
			NULL);

	if (index != -1)
		(void) sprintf(tmp_buf, "%s%c%d", disk_name, part_char, index);
	else
		(void) sprintf(tmp_buf, "%s", disk_name);

	if (disk_explicit != 0 || device_explicit != 0) {
		(void) sprintf(buf, PROM_MSG, tmp_buf);
	} else {
		(void) sprintf(buf, PROM_MSG, PROM_SELECTED_DISK);
	}
/*	if (index != -1)
 *		(void) sprintf(buf, PROM_MSG, disk_name, part_char, index);
 *	else
 *		(void) sprintf(buf, PROM_MSG1, disk_name);
 */
	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->msg_type = UI_MSGTYPE_WARNING;
	msg_info->title = UPDATE_PROM_QUERY;
	msg_info->msg = buf;
	msg_info->btns[UI_MSGBUTTON_OK].button_text = UPDATE_STR;
	msg_info->btns[UI_MSGBUTTON_OTHER1].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_OTHER2].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = NO_UPDATE_STR;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;
	msg_info->default_button = UI_MSGBUTTON_OK;

	/* invoke the message */
	UI_MsgFunction(msg_info);

	/* cleanup */
	UI_MsgStructFree(msg_info);

	switch (UI_MsgResponseGet()) {
	case UI_MSGRESPONSE_CANCEL:
		answer = FALSE;
		break;
	case UI_MSGRESPONSE_OK:
		default:
		answer = TRUE;
		break;
	}
	return (answer);

}
