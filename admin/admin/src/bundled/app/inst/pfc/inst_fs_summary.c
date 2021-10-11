#ifndef lint
#pragma ident "@(#)inst_fs_summary.c 1.28 96/07/11 SMI"
#endif

/*
 * Copyright (c) 1993-1996 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 */

/*
 * Module:	inst_fs_summary.c
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
#include "v_lfs.h"
#include "v_sw.h"

#include "inst_msgs.h"

static int _jump_into_editor(WINDOW *, ChoiceItem *);
static void _free_opts(ChoiceItem *, int);
static ChoiceItem *_load_opts(ChoiceItem *, int *);
static struct _ChoiceItemShowFSDataStruct {
	char *disk_name;
	int slice_num;
};

#define	NO_FILESYS_STR	gettext(\
	"<no file systems configured>")

parAction_t
do_show_filesystems()
{
	int top_row;		/* first row of menu */
	int last_row;		/* last row of menu */
	int cur;		/* remember last selection */

	static int ch;
	int top;		/* index of first item displayed */
	int dirty;
	int really_dirty;
	int fs_per_page;
	int nlfs = 0;
	int ok_to_jump;
	int ndisks = v_get_n_disks();
	int	ret = FALSE;
	char	diskname[32];
	char	part_char;
	int	dev_index;
	Disk_t	*dp;
	int	slice;
	char	root_slicename[32];
	char	root_diskname[32];
	char	buf[32];
	char	diskbuf[32];
	int	status;

	unsigned long fkeys;

	ChoiceItem *opts = (ChoiceItem *) NULL;

	(void) werase(stdscr);
	(void) wclear(stdscr);
	wheader(stdscr, TITLE_FILESYS);
	top_row = HeaderLines;

	top_row = wword_wrap(stdscr, top_row, INDENT0, COLS - (2 * INDENT0),
	    MSG_FILESYS);
	top_row++;

	(void) mvwprintw(stdscr, top_row++, INDENT1, "%-32.32s  %-15.15s  %10s",
	/* i18n: 35 characters maximum */
	    gettext("File system/Mount point"),
	/* i18n: 15 characters maximum */
	    gettext("Disk/Slice"),
	/* i18n: 10 characters maximum */
	    gettext("Size"));

	(void) mvwprintw(stdscr, top_row++, INDENT1, "%-.*s",
	    32 + 15 + 10 + 4, EQUALS_STR);

	/* init menu state */
	cur = 0;
	top = 0;
	fs_per_page = LINES - top_row - FooterLines - 1;
	last_row = top_row + fs_per_page - 1;
	dirty = really_dirty = 1;

	fkeys = F_CONTINUE | F_GOBACK | F_EXIT | F_CUSTOMIZE | F_HELP;

	/* process events */
	for (;;) {

		if (really_dirty == 1) {

			if (opts != (ChoiceItem *) NULL) {
				_free_opts(opts, nlfs);
				opts = (ChoiceItem *) NULL;
			}
			v_set_n_lfs();
			nlfs = v_get_n_lfs();

			/*
			 * use a function to load opt array.  may need to do
			 * this several times, so made it a function
			 */
			opts = _load_opts(opts, &nlfs);

			if (nlfs == 0) {	/* yikes! */
				opts = (ChoiceItem *) xcalloc(1 *
				    sizeof (ChoiceItem));
				nlfs = 1;
				ok_to_jump = 0;

				opts[0].loc.r = top_row;
				opts[0].loc.c = INDENT1;
				opts[0].help.type = HELP_TOPIC;
				opts[0].help.title =
				    "Laying Out File Systems on Disks";
				opts[0].label = xstrdup(NO_FILESYS_STR);
				opts[0].sel = -1;	/* XXX HACK! */
				opts[0].data = (void *) NULL;

			} else
				ok_to_jump = 1;

			cur = 0;
			really_dirty = 0;
			dirty = 1;

		}
		if (dirty == 1) {

			(void) show_choices(stdscr, nlfs, fs_per_page,
			    top_row, INDENT1, opts, top);

			scroll_prompts(stdscr, top_row, 1, top, nlfs,
			    fs_per_page);

			wfooter(stdscr, fkeys);
			dirty = 0;

		}
		/* highlight current */
		wfocus_on(stdscr, opts[cur].loc.r, opts[cur].loc.c,
		    opts[cur].label);

		ch = wzgetch(stdscr, fkeys);

		/* unhighlight */
		wfocus_off(stdscr, opts[cur].loc.r, opts[cur].loc.c,
		    opts[cur].label);

		wnoutrefresh(stdscr);

		if (is_continue(ch) != 0) {
			write_debug(CUI_DEBUG_L1, "key is continue");

			WALK_DISK_LIST(dp) {
				if (disk_selected(dp)) {
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
			}

			WALK_DISK_LIST(dp) {
				if (disk_selected(dp)) {
					WALK_SLICES(slice) {
						if (strcmp(slice_mntpnt(dp, slice),
							ROOT) == 0) {
							status = 
								SdiskobjRootSetBoot(dp,
										slice);
						if (status != D_OK) {
						(void) simple_notice(stdscr,
						F_OKEYDOKEY,
						gettext("Disk Editing Error"),
							DISK_ERR_ROOTBOOT);
						}
						}
					}
				}
			}

			/*
			 * get the name of the committed boot device
			 */
			(void) BootobjGetAttribute(CFG_CURRENT,
				BOOTOBJ_DISK, diskname,
				BOOTOBJ_DEVICE, &dev_index,
				BOOTOBJ_DEVICE_TYPE, &part_char,
				NULL);

			if ((strcmp(diskname, "") != 0) && dev_index != -1 &&
								IsIsa("sparc")) {
				(void) sprintf(buf, "%s%c%d", diskname,
					part_char, dev_index);
			} else if ((strcmp(diskname, "") != 0) && dev_index != -1 &&
								IsIsa("ppc")) {
				(void) sprintf(buf, "%ss0", diskname);
			} else if ((strcmp(diskname, "") != 0) && dev_index != -1 &&
								IsIsa("i386")) {
				(void) sprintf(diskbuf, "%s", diskname);
			} else if ((strcmp(diskname, "") != 0) && dev_index == -1) {
				(void) sprintf(diskbuf, "%s", diskname);
			}

			if (strcmp(diskname, "") != 0) {
				if ((strcmp(diskname, root_diskname) != 0) ||
					(!IsIsa("i386") &&
					(strcmp(diskname, root_diskname) == 0) &&
					(strcmp(root_slicename, buf) != 0))) {
					ret = BootobjDiffersQuery(root_slicename);
					if (ret == TRUE) {
						ret = FALSE;
						(void) do_show_filesystems();
					} else if (ret == FALSE) {
						break;
					}
				}
			}
			break;

		} else if (is_goback(ch) != 0) {

			break;

		} else if (is_customize(ch) != 0) {

			(void) customize_disks(stdscr, ndisks);
			really_dirty = 1;	/* reload & redraw options */

		} else if (is_exit(ch) != 0) {

			if (confirm_exit(stdscr) == 1)
				break;	/* cancel/exit */

		} else if (is_help(ch) != 0) {

			do_help_index(stdscr, opts[cur].help.type,
			    opts[cur].help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else if ((ch == '/' || isdigit(ch) != 0) && ok_to_jump) {

			/*
			 * they tried entering something which looked like a
			 * file system path or size.  Jump them into the
			 * disk editor with this file system... pass the
			 * real index so that jump_into_editor() can go find
			 * stuff out about the `current' file system
			 */
			if (_jump_into_editor(stdscr,
				&opts[cur]))
				really_dirty = 1;	/* reload & redraw */

		} else if (ch == U_ARROW || ch == D_ARROW || ch == CTRL_U ||
			ch == CTRL_P || ch == CTRL_F || ch == CTRL_B ||
			ch == CTRL_N || ch == CTRL_P) {

			/* move */
			if (ch == CTRL_D) {

				/* page down */
				if ((cur + fs_per_page) < nlfs) {

					/* advance a page */
					top += fs_per_page;
					cur += fs_per_page;
					dirty = 1;

				} else if (cur < (nlfs - 1)) {

					/* advance to last file system */
					cur = nlfs - 1;
					top = cur - 2;
					dirty = 1;

				} else
				beep(); /* at end */

			} else if (ch == CTRL_U) {

				/* page up */
				if ((cur - fs_per_page) >= 0) {

					/* reverse a page */
					top = (top > fs_per_page ?
					top - fs_per_page : 0);
					cur -= fs_per_page;
					dirty = 1;

				} else if (cur > 0) {

					/* back to first file system */
					top = 0;
					cur = 0;
					dirty = 1;

				} else
					beep(); /* at top */

			} else if (ch == U_ARROW || ch == CTRL_P ||
				ch == CTRL_B) {

				if (opts[cur].loc.r == top_row) {

					if (top) {	/* scroll down */
						cur = --top;
						dirty = 1;
					} else
						beep();	/* very top */

				} else
					cur--;

			} else if (ch == D_ARROW || ch == CTRL_N ||
			    ch == CTRL_F) {

				if (opts[cur].loc.r == last_row) {

					if ((cur + 1) < nlfs) {

						/* scroll up */
						top++;
						cur++;
						dirty = 1;

					} else
						beep();	/* bottom */

				} else {

					if ((cur + 1) < nlfs)
						cur++;
					else
						beep();	/* last, no wrap */
				}

			}
		} else
			beep();

	}

	/* cleanup memory */
	_free_opts(opts, nlfs);

	if (is_exit(ch) != 0)
		return (parAExit);	/* cancel */
	else if (is_goback(ch) != 0)
		return (parAGoback);	/* goback */
	else			/* if (is_continue(ch) != 0) */
		return (parAContinue);	/* done & continue */
}

static int
_jump_into_editor(WINDOW * parent, ChoiceItem *opt)
{
	int retcode;
	struct _ChoiceItemShowFSDataStruct *data;

	retcode = 0;

	/*
	 * pass NULL window into notice routine.
	 *
	 * notice wants to refresh on the way out. only do the refresh if not
	 * going into the editor,  saves some extra repainting.
	 *
	 */
	if (yes_no_notice(parent, (F_CONTINUE | F_CANCEL),
		F_CONTINUE, F_CANCEL, FILESYS_EDIT_FILE_SYSTEM_TITLE,
		FILESYS_EDIT_FILE_SYSTEM) == F_CONTINUE) {

		/*
		 * figure out which disk we need to stuff into the editor
		 * and which slice should start at...
		 *
		 * get current mount point, from that get the disk/slice it is
		 * on, from that compute the disk index & slice index
		 */
		if (opt && opt->data) {
			data = (struct _ChoiceItemShowFSDataStruct *) opt->data;
			edit_disk(parent,
				v_get_disk_index_from_name(data->disk_name),
				data->slice_num);
			retcode = 1;
		}
	}
	touchwin(parent);
	wnoutrefresh(parent);

	return (retcode);
}


static void
_free_opts(ChoiceItem * opts, int n)
{
	register int i;
	struct _ChoiceItemShowFSDataStruct *data;

	if (opts != (ChoiceItem *) NULL) {
		for (i = 0; i < n; i++) {
			if (opts[i].label != (char *) NULL)
				free((void *) opts[i].label);
			data = (struct _ChoiceItemShowFSDataStruct *)
				opts[i].data;
			if (data && data->disk_name != (char *) NULL)
				free((void *) data->disk_name);
			if (opts[i].data != (char *) NULL)
				free((void *) opts[i].data);
		}
	}
	free((void *) opts);
}

static ChoiceItem *
_load_opts(ChoiceItem * opts, int *n)
{
	char buf[128];		/* scratch pointer for sprintf() */
	int actual;

	struct disk *d;
	int slice_num;
	char buf2[32];
	struct _ChoiceItemShowFSDataStruct *data;

	opts = (ChoiceItem *) NULL;

	set_units(D_MBYTE);
	actual = 0;
	WALK_DISK_LIST(d) {
		if (!disk_selected(d)) {
			continue;
		}
		WALK_SLICES_STD(slice_num) {
			if ((strcmp(slice_mntpnt(d, slice_num), "alts") == 0) ||
				!slice_size(d, slice_num))
				continue;

			(void) sprintf(buf2, "%ss%1d",
				disk_name(d), slice_num);
			(void) sprintf(buf,
				"%-32.32s  %-15.15s  %7d %s",
				slice_mntpnt(d, slice_num),
				buf2,
				blocks2size(d, slice_size(d, slice_num),
					ROUNDDOWN),
				gettext("MB"));

			opts = (ChoiceItem *) xrealloc(opts,
				(actual + 1) * sizeof (ChoiceItem));
			opts[actual].label = (char *) xstrdup(buf);
			opts[actual].help.win = (WINDOW *) NULL;
			opts[actual].help.type = HELP_TOPIC;
			opts[actual].help.title =
				"Laying Out File Systems on Disks";
			opts[actual].sel = -1;	/* XXX HACK! */
			opts[actual].loc.c = INDENT1;
			opts[actual].loc.r = 0;

			data = (struct _ChoiceItemShowFSDataStruct *) malloc(
				sizeof (struct _ChoiceItemShowFSDataStruct));
			data->disk_name = strdup(disk_name(d));
			data->slice_num = slice_num;
			opts[actual].data = (void *) data;

			actual++;
		}
	}

	(*n) = actual;		/* return actual # of file systems */

	return (opts);		/* return new, possibly changed array */

}
