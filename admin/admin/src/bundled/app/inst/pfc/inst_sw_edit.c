#ifndef lint
#pragma ident "@(#)inst_sw_edit.c 1.66 96/07/29 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_sw_edit.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libintl.h>
#include <stdarg.h>
#include <unistd.h>

#include "pf.h"
#include "inst_msgs.h"

#include "v_check.h"
#include "v_types.h"
#include "v_disk.h"
#include "v_sw.h"

typedef struct {
	HelpEntry help;
	NRowCol loc;
	FieldType type;
	char *label;
	char *prompt;
	parIntFunc sel_proc;
} _SWTable_Item_t;

/*
 * a row in the software menu consists of 3 items:
 *	field for expand/contract thingy
 *	field for selection thingy
 *	field for SW mod title/size
 */
typedef struct {

	_SWTable_Item_t fld[3];

} _SWTable_Row_t;

/* local function definitions */
static void _sw_menu();
static void _show_props(int);
static void _show_tuples(WINDOW *, int, int, int, _SWTable_Row_t *, int);

#define	CUSTOMIZE_SW_STR	gettext("Customize Software")

parAction_t
do_sw_edit(void)
{
	for (;;) {

		/*
		 * XXX typeahead gets around some refresh problems when
		 * scrolling quickly...
		 */
		typeahead(0);
		_sw_menu();
		typeahead(-1);

		if (v_check_sw_depends() == CONFIG_WARNING) {

			if (show_sw_depends() == 0)
				break;	/* really exit */

			(void) werase(stdscr);
			(void) wclear(stdscr);

		} else
			break;
	}

	wstatus_msg(stdscr, PLEASE_WAIT_STR);
	wcursor_hide(stdscr);

	/* ?? */
	return (parANone);

}

/*
 * the next three functions are the `callback' routines invoked when a
 * RETURN is pressed on any active item in a column... the return value sort
 * of controls when a complete reload of the sw list is done.
 *
 */

int
toggle_sw(int i, _SWTable_Item_t ** item)
{
	int status = v_get_module_status(i);

	if (status == REQUIRED) {
		beep();
		return (0);
	} else {
		/* toggle module */
		(void) v_set_module_status(i);

		status = v_get_module_status(i);

		if (status == REQUIRED) {
			((_SWTable_Item_t *) item)->label = "[!]";
			((_SWTable_Item_t *) item)->prompt =
			    SW_MENU_REQUIRED_PROMPT;
		} else if (status == SELECTED) {
			((_SWTable_Item_t *) item)->label = "[X]";
			((_SWTable_Item_t *) item)->prompt =
			    SW_MENU_SELECTED_PROMPT;
		} else if (status == PARTIALLY_SELECTED) {
			((_SWTable_Item_t *) item)->label = "[/]";
			((_SWTable_Item_t *) item)->prompt =
			    SW_MENU_PARTIAL_PROMPT;
		} else {
			((_SWTable_Item_t *) item)->label = "[ ]";
			((_SWTable_Item_t *) item)->prompt =
			    SW_MENU_UNSELECTED_PROMPT;
		}

		return (1);
	}
}

/*ARGSUSED1*/
int
do_sw_info(int i, _SWTable_Item_t ** item)
{
	_show_props(i);

	return (0);
}

/*ARGSUSED1*/
int
expand_contract(int i, _SWTable_Item_t ** item)
{
	if (v_get_submods_are_shown(i) == 1) {
		(void) v_contract_module(i);
	} else {
		(void) v_expand_module(i);
	}

	return (1);
}

/*
 * need to free module name strings which were strdup()ed in _load_tuples().
 * Detach this from the actualy process of loading the table since the
 * number of lines in the table varies with each call.
 */
static void
_free_sw_labels(int nmods, _SWTable_Row_t * table)
{
	register int i;

	/*
	 * free any existing sw module labels which have been strdup'ed
	 */
	if (table != (_SWTable_Row_t *) NULL) {
		for (i = 0; i < nmods; i++) {
			if (table[i].fld[2].label != (char *) NULL)
				free((void *) table[i].fld[2].label);
		}
	}
}

static _SWTable_Row_t *
_load_tuples(int nmods, _SWTable_Row_t * table)
{
	int i;
	int indent;
	int max_name_fld_width;
	int name_fld_width;
	int namestr_len;
	int spacestr_len;
	int status;

	char *namestr;
	char *spacestr;

	/* 68 character width for names... set up leading and space buffers */
	char *dots =
".......................................................................";
	char *spcs =
"                                                                       ";
	char *leading;
	char buf[128];

	max_name_fld_width = strlen(dots) - 1;

	table = (_SWTable_Row_t *) xrealloc((void *) table,
	    (nmods * sizeof (_SWTable_Row_t)));

	for (i = 0; i < nmods; i++) {

		/* reset name field width before each module */
		name_fld_width = max_name_fld_width;

		(void) v_set_current_module(i);

		/* first field of tuple is the expand contract thingy. */
		table[i].fld[0].help.win = stdscr;
		table[i].fld[0].help.type = HELP_REFER;
		table[i].fld[0].help.title = "Customize Software Screen";

		if (v_get_module_has_submods(i) == 1) {

			if (v_get_submods_are_shown(i) == 1) {
				table[i].fld[0].prompt =
					SW_MENU_CONTRACT_PROMPT;
				table[i].fld[0].label = "V";	/* expanded */
			} else {
				table[i].fld[0].prompt = SW_MENU_EXPAND_PROMPT;
				table[i].fld[0].label = ">";	/* can expand */
			}

			table[i].fld[0].type = LSTRING;
			table[i].fld[0].sel_proc = expand_contract;

		} else {

			table[i].fld[0].prompt = "";
			table[i].fld[0].type = INSENSITIVE;
			table[i].fld[0].label = " ";

		}

		/* second field of tuple is the status thingy. */
		table[i].fld[1].help.win = stdscr;
		table[i].fld[1].help.type = HELP_REFER;
		table[i].fld[1].help.title = "Customize Software Screen";
		table[i].fld[1].type = LSTRING;
		table[i].fld[1].sel_proc = toggle_sw;

		status = v_get_module_status(i);

		if (status == REQUIRED) {
			table[i].fld[1].label = "[!]";
			table[i].fld[1].prompt = SW_MENU_REQUIRED_PROMPT;
		} else if (status == SELECTED) {
			table[i].fld[1].label = "[X]";
			table[i].fld[1].prompt = SW_MENU_SELECTED_PROMPT;
		} else if (status == PARTIALLY_SELECTED) {
			table[i].fld[1].label = "[/]";
			table[i].fld[1].prompt = SW_MENU_PARTIAL_PROMPT;
		} else {
			table[i].fld[1].label = "[ ]";
			table[i].fld[1].prompt = SW_MENU_UNSELECTED_PROMPT;
		}

		/* third field is the module name & size info... */
		table[i].fld[2].help.win = stdscr;
		table[i].fld[2].help.type = HELP_REFER;
		table[i].fld[2].help.title = "Customize Software Screen";
		table[i].fld[2].type = LSTRING;
		table[i].fld[2].prompt = SW_MENU_MODINFO_PROMPT;
		table[i].fld[2].sel_proc = do_sw_info;

		/*
		 * is this a `submodule' ?  if so, what level? indent module
		 * name 4 spaces per level
		 */
		indent = 4 * v_get_module_level(i);
		name_fld_width -= indent;

		namestr = v_get_module_name(i);

		if ((namestr_len = strlen(namestr)) >= name_fld_width)
			namestr_len = name_fld_width;

		spacestr = v_get_module_size(i);
		spacestr_len = strlen(spacestr);

		if (v_get_submods_are_shown(i) == 1)
			leading = spcs;
		else
			leading = dots;

		(void) sprintf(buf, "%-.*s%.*s%.*s%s",
		    indent, "                ",	/* indention blanks  */
		    namestr_len >= name_fld_width
		    ? (name_fld_width - spacestr_len) : namestr_len,
		    namestr,
		    (namestr_len >= name_fld_width)
		    ? name_fld_width
		    : name_fld_width - namestr_len - spacestr_len,
		    namestr_len >= name_fld_width ? ">" : leading,
		    spacestr);

		table[i].fld[2].label = xstrdup(buf);
	}

	return (table);
}


static void
_sw_menu()
{
	int r, c;
	int curmeta;
	int nmods;
	int dirty;
	int ch;
	int top_row;
	int last_row;
	int mods_per_page;
	int top;
	int tuple;
	int field;
	int first = 1;
	unsigned long fkeys;

	_SWTable_Row_t *table = (_SWTable_Row_t *) NULL;
	char buf[128];
	char *curmeta_str;

	curmeta = v_get_current_metaclst();
	curmeta_str = v_get_metaclst_name(curmeta);

	/*
	 * set up for first page of modules.
	 */
	top_row = HeaderLines;
	tuple = top = 0;
	field = 0;
	mods_per_page = LINES - HeaderLines - FooterLines - 2;
	last_row = top_row + mods_per_page;

	(void) werase(stdscr);
	(void) wclear(stdscr);

	/* show title */
	(void) sprintf(buf, "%s: %-s", CUSTOMIZE_SW_STR,
	    curmeta_str);
	wheader(stdscr, buf);

#ifdef notdef
	/* this is very slow now. */
	(void) sprintf(buf, "%s %s", gettext(" Approx. space:"),
	    v_get_metaclst_size(curmeta));

	wcolor_on(stdscr, TITLE);
	(void) mvwprintw(stdscr, 0, COLS - strlen(buf) - 2, "%-s -", buf);
	wcolor_off(stdscr, TITLE);
#endif

	nmods = v_get_n_modules();
	table = _load_tuples(nmods, table);
	dirty = 1;

	/* show input options... */
	fkeys = F_OKEYDOKEY | F_HELP;

	for (;;) {

		if (dirty) {

			_show_tuples(stdscr, nmods, mods_per_page, top_row,
			    table, top);
			wfooter(stdscr, fkeys);

			dirty = 0;
		}
		/* set footer */
		if (table[tuple].fld[field].prompt != (char *) NULL &&
		    table[tuple].fld[field].prompt[0] != '\0') {
			wstatus_msg(stdscr, table[tuple].fld[field].prompt);
		} else {
			wclear_status_msg(stdscr);
		}

		if (first) {
			wstatus_msg(stdscr, SW_MENU_NAVIGATION_HINT);
			first = 0;
		}
		wfocus_on(stdscr, table[tuple].fld[field].loc.r,
		    table[tuple].fld[field].loc.c,
		    table[tuple].fld[field].label);

		/*
		 * use doupdate() and wnoutrefresh() to fix refresh
		 * problems... cursor gets lost and turds get left on screen
		 * when scrolling or paging.
		 */
		(void) getsyx(r, c);
		(void) wnoutrefresh(stdscr);
		(void) setsyx(r, c);
		(void) doupdate();

		ch = wzgetch(stdscr, fkeys);

		/* unhighlight */
		wfocus_off(stdscr, table[tuple].fld[field].loc.r,
		    table[tuple].fld[field].loc.c,
		    table[tuple].fld[field].label);

		wnoutrefresh(stdscr);

		if (ch == U_ARROW || ch == D_ARROW ||
		    ch == R_ARROW || ch == L_ARROW ||
		    ch == CTRL_N || ch == CTRL_P ||
		    ch == CTRL_F || ch == CTRL_D ||
		    ch == CTRL_B || ch == CTRL_U) {

			dirty = 0;

			/* move */
			if (ch == CTRL_D) {

				/* page down */
				if ((tuple + mods_per_page) < nmods) {

					/* advance a page */
					top += mods_per_page;
					tuple += mods_per_page;
					dirty = 1;

				} else if (tuple < nmods - 1) {

					/* advance to last line */
					tuple = nmods - 1;
					top = tuple - 2;
					dirty = 1;

				} else
					beep();	/* at end */

			} else if (ch == CTRL_U) {

				/* page up */
				if ((tuple - mods_per_page) >= 0) {

					/* reverse a page */
					top = (top > mods_per_page ?
					    top - mods_per_page : 0);
					tuple -= mods_per_page;
					dirty = 1;

				} else if (tuple > 0) {

					/* back to first line */
					top = 0;
					tuple = 0;
					dirty = 1;

				} else
					beep();	/* at top */

			} else if (ch == R_ARROW || ch == CTRL_F) {

				if (field == 2)
					field = 0;
				else
					field++;

			} else if (ch == L_ARROW || ch == CTRL_B) {

				if (field == 0)
					field = 2;
				else
					field--;

			} else if (ch == U_ARROW || ch == CTRL_P) {

				if (table[tuple].fld[field].loc.r == top_row) {

					if (top) {	/* scroll down */
						tuple = --top;
						dirty = 1;
					} else
						beep();	/* very top */

				} else {
					tuple--;
				}

			} else if (ch == D_ARROW || ch == CTRL_N) {

				if (table[tuple].fld[field].loc.r ==
				    (last_row - 1)) {

					if ((tuple + 1) < nmods) {

						/* scroll up */
						top++;
						tuple++;
						dirty = 1;

					} else
						beep();	/* bottom */

				} else {

					if ((tuple + 1) < nmods) {
						tuple++;
					} else
						beep();	/* last, no wrap */
				}

			}
		} else if ((sel_cmd(ch) != 0) || (alt_sel_cmd(ch) != 0)) {

			/* selection */
			if (field == 0) {

				/* expand/contract? */
				if (table[tuple].fld[field].type !=
				    INSENSITIVE) {

					/*
					 * selection proc expands or
					 * contracts the cluster, need to
					 * reset module count afterwards
					 */
					dirty =
					    table[tuple].fld[field].sel_proc(
					    tuple, &table[tuple].fld[field]);

					_free_sw_labels(nmods, table);
					nmods = v_get_n_modules();
					table = _load_tuples(nmods, table);

				} else
					beep();	/* can't expand/contract */

			} else if (field == 1) {

				/* toggle module */
				dirty = table[tuple].fld[field].sel_proc(tuple,
				    &table[tuple].fld[field]);

				/*
				 * Need to reload entire table to pick up
				 * all possible chagnes
				 */
				if (dirty) {
					_free_sw_labels(nmods, table);
					table = _load_tuples(nmods, table);

					/* update title */
					(void) sprintf(buf, "%s: %-s",
					    CUSTOMIZE_SW_STR,
					    curmeta_str);
					wheader(stdscr, buf);

#ifdef notdef
					/* this is very slow now. */
					(void) sprintf(buf, "%s %s",
					    gettext(" Approx. space:"),
					    v_get_metaclst_size(curmeta));

					wcolor_on(stdscr, TITLE);
					(void) mvwprintw(stdscr, 0,
						COLS - strlen(buf) - 2, "%-s -",
						buf);
					wcolor_off(stdscr, TITLE);
#endif
				}
			} else if (field == 2) {

				/* show module info */
				dirty = table[tuple].fld[field].sel_proc(tuple,
				    &table[tuple].fld[field]);

			}
		} else if (is_ok(ch) != 0) {

			break;

		} else if (is_help(ch) != 0) {

			do_help_index(table[tuple].fld[field].help.win,
			    table[tuple].fld[field].help.type,
			    table[tuple].fld[field].help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else
			beep();
	}

	if (table) {
		_free_sw_labels(nmods, table);
		free((void *) table);
	}
}

static void
_show_tuples(WINDOW * w, int max, int npp, int row, _SWTable_Row_t * table,
    int first)
{
	int i;			/* counts modules displayed		*/
	int j;			/* index of current software mod	*/
	int r;			/* counts row positions			*/

	for (i = 0, r = row, j = first;
	    (i < npp) && (j < max);
	    i++, r++, j++) {

		(void) mvwprintw(w, r, 0, "  %s %s %s", table[j].fld[0].label,
		    table[j].fld[1].label, table[j].fld[2].label);

		table[j].fld[0].loc.r =
		    table[j].fld[1].loc.r =
		    table[j].fld[2].loc.r = (int) r;

		table[j].fld[0].loc.c = 2;
		table[j].fld[1].loc.c = 4;
		table[j].fld[2].loc.c = 8;

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
 * this is for a 2 column display of space usage in the property displays...
 */
static NRowCol fs_size[] = {
	{0, 28}, {0, 49},
	{1, 28}, {1, 49},
	{2, 28}, {2, 49}
};

static void
_show_props(int mod)
{
	WINDOW *win;

	char *cp, *cp1;
	char *descript;
	int ch;
	int sizef = 0, row = 3;
	char sz_fmt[] = "%9.9s in %s";

	Sizes_t *s;

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);

	(void) werase(win);
	(void) wclear(win);

	(void) v_set_current_module(mod);

	/* show title */
	cp = v_get_module_name(mod);
	wheader(win, cp);

	row = HeaderLines;
	cp = v_get_module_id(mod);
	(void) mvwprintw(win, row++, 0, "%25s:  %s", gettext("Abbreviation"),
	    cp ? cp : "");

	cp = v_get_module_vendor(mod);
	(void) mvwprintw(win, row++, 0, "%25s:  %s", gettext("Vendor"),
	    cp ? cp : "");

	cp = v_get_module_version(mod);
	(void) mvwprintw(win, row++, 0, "%25s:  %s", gettext("Version"),
	    cp ? cp : "");

	(void) mvwprintw(win, row, 0, "%25s:", gettext("Description"));

	/* split description into lines of < 50 characters */
	descript = xstrdup(v_get_module_description(mod));
	for (cp = descript; cp && (int) strlen(cp) > 48; row++) {

		/* find a space, break line there */
		for (cp1 = cp + 48; cp1 && (*cp1 != ' '); cp1--);

		*cp1 = '\0';
		(void) mvwprintw(win, row, 28, "%-48s", cp);
		*cp1 = ' ';
		cp = ++cp1;
	}
	if (cp)
		(void) mvwprintw(win, row++, 28, "%-50s", cp);

	if (descript)
		free(descript);

	if (v_get_module_type(mod) == V_PACKAGE) {
		(void) mvwprintw(win, ++row, 0, "%25s:  %s",
		    gettext("Default location"), v_get_module_basedir(mod));
	}
	++row;
	(void) mvwprintw(win, ++row, 0, "%25s:", gettext("Estimated size"));

	if (s = v_get_module_fsspace_used(mod)) {

		if (strcmp(s->sz[ROOT_FS], "  0.00 MB") != 0) {

			(void) mvwprintw(win, row + fs_size[sizef].r,
			    fs_size[sizef].c, sz_fmt, s->sz[ROOT_FS], "/");
			++sizef;

		}
		if (strcmp(s->sz[USR_FS], "  0.00 MB") != 0) {

			(void) mvwprintw(win, row + fs_size[sizef].r,
			    fs_size[sizef].c, sz_fmt, s->sz[USR_FS], "/usr");
			++sizef;

		}
		if (strcmp(s->sz[OPT_FS], "  0.00 MB") != 0) {

			(void) mvwprintw(win, row + fs_size[sizef].r,
			    fs_size[sizef].c, sz_fmt, s->sz[OPT_FS], "/opt");
			++sizef;

		}
		if (strcmp(s->sz[VAR_FS], "  0.00 MB") != 0) {

			(void) mvwprintw(win, row + fs_size[sizef].r,
			    fs_size[sizef].c, sz_fmt, s->sz[VAR_FS], "/var");
			++sizef;

		}
		if (strcmp(s->sz[EXPORT_FS], "  0.00 MB") != 0) {

			(void) mvwprintw(win, row + fs_size[sizef].r,
			    fs_size[sizef].c, sz_fmt, s->sz[EXPORT_FS],
			    "/export");
			++sizef;

		}
		if (strcmp(s->sz[USR_OWN_FS], "  0.00 MB") != 0) {

			(void) mvwprintw(win, row + fs_size[sizef].r,
			    fs_size[sizef].c, sz_fmt, s->sz[USR_OWN_FS],
			    "/usr/openwin");
			++sizef;

		}
		row += (int) ((sizef + 1) / 2);

		(void) mvwprintw(win, ++row, 0, "%25s:  %s",
		    gettext("Architectures supported"),
		    v_get_module_arches(mod));

	}
	wfooter(win, F_OKEYDOKEY);
	wcursor_hide(win);

	for (;;) {

		ch = wzgetch(win, F_OKEYDOKEY);

		if (is_ok(ch) != 0 || sel_cmd(ch) != 0) {

			break;

		} else if (is_escape(ch) != 0) {

			continue;

		} else
			beep();
	}

	(void) delwin(win);
	(void) clearok(curscr, TRUE);
	(void) touchwin(stdscr);
	(void) wnoutrefresh(stdscr);
	(void) clearok(curscr, FALSE);
}
