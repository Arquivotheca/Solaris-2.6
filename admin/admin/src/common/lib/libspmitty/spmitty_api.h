#ifndef lint
#pragma ident "@(#)spmitty_api.h 1.9 96/07/29 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	spmitty_api.h
 * Group:	libspmitty
 * Description:
 */

#ifndef _SPMITTY_API_H
#define	_SPMITTY_API_H

#include <curses.h>
#include <term.h>
#include <sys/ttychars.h>

#include "spmiapp_api.h"

/*
 * tty miscellaneous stuff...
 */

/* globals */
extern int 	HeaderLines;
extern int 	FooterLines;
extern int	curses_on;

extern	char *Sel;
extern  char *Unsel;
extern  char *Clear;


#if !defined(NOMACROS) && !defined(beep)
#define	beep()	(void) printf("\007");
#endif

#define	MINLINES	24
#define	MINCOLS		80

/* a single string of 80 `-' characters */
#define	DASHES_STR \
"--------------------------------------------------------------------------------"

/* a single string of 80 `=' characters */
#define	EQUALS_STR \
"================================================================================"

/* a single string of 80 ` ' characters */
#define	SPACES_STR \
"                                                                                "

/*
 * tty color
 */

typedef enum hilite {
	TITLE,
	FOOTER,
	BODY,
	CURSOR,
	CURSOR_INV,
	NORMAL
} HiLite_t;

/*
 * tty help
 */
typedef enum {
	HELP_NONE = -1,
	HELP_TOPIC = 'C',
	HELP_HOWTO = 'P',
	HELP_REFER = 'R'
} help_t;

typedef struct {
	WINDOW 	*win;
	help_t	type;
	char	*title;
} HelpEntry;

/*
 * tty messaging
 * The app has to tell the messaging code:
 *	- what the parent curses window is and
 *	- which fkeys to use for the dialog chioces.
 *	- The fkeys used MUST correspond to keys
 *	  F2, F3, F4, F5, F6, in that order, where
 *	  dialog_fkeys is indexed by the values of the enum type UI_MsgButton.
 */
typedef struct {
	WINDOW *parent;
	int dialog_fkeys[UI_MSGBUTTON_MAX];
} tty_MsgAdditionalInfo;

/*
 * tty key strokes
 */

/*
 * Various keystroke definitions for user input.
 * KEY_* defined in curses.h
 */
#define	L_ARROW	KEY_LEFT
#define	R_ARROW	KEY_RIGHT
#define	U_ARROW	KEY_UP
#define	D_ARROW	KEY_DOWN

#define	CTRL_F	CTRL('f')
#define	CTRL_B	CTRL('b')
#define	CTRL_U	CTRL('u')
#define	CTRL_D	CTRL('d')
#define	CTRL_N	CTRL('n')
#define	CTRL_P	CTRL('p')
#define	CTRL_Z	CTRL('z')
#define	CTRL_C	CTRL('c')
#define	CTRL_A	CTRL('a')
#define	CTRL_R	CTRL('r')
#define	CTRL_H	CTRL('h')
#define	CTRL_G	CTRL('g')
#define	CTRL_T	CTRL('t')
#define	CTRL_M	CTRL('m')
#define	CTRL_I	CTRL('i')
#define	CTRL_E	CTRL('e')

#define	RETURN	0x0a	/* ASCII RETURN */
#define	TAB	0x09	/* ASCII TAB */
#define	DONE	0x1b	/* ASCII ESCAPE */
#define	ESCAPE	0x1b	/* ASCII ESCAPE */
#define	SPACE	0x20	/* ASCII SPACE */
#define	CONTINUECHAR	KEY_F(2)

struct f_key {
	char *f_keycap;		/* terminfo DB: e.g. "kf2" */
	char *f_special;	/* e.g. "F2" */
	char *f_fallback;	/* e.g. "Esc-2" */
	char *f_func;		/* the button label (e.g. Continue) */
	char *f_label;		/* str that's actually displayed */
};
typedef struct f_key Fkey;

/*
 * These should be defined in desired
 * presentation order
 */
#define	F_OKEYDOKEY	0x1UL
#define	F_CONTINUE	0x2UL

/* all values in between here reserved for use by the apps */

#define	F_HALT		0x400000UL
#define	F_CANCEL	0x800000UL
#define	F_EXIT		0x1000000UL
#define	F_HELP		0x2000000UL
#define	F_GOTO		0x4000000UL
#define	F_MAININDEX	0x8000000UL
#define	F_TOPICS	0x10000000UL
#define	F_REFER		0x20000000UL
#define	F_HOWTO		0x40000000UL
#define	F_EXITHELP	0x80000000UL

#define	F_MAXKEY	F_EXITHELP

#define	is_escape(c)		((c) == ESCAPE)

#define	is_ok(c)		((c) == KEY_F(2))
#define	is_continue(c)		((c) == KEY_F(2))

#define	is_halt(c)		((c) == KEY_F(5))
#define	is_cancel(c)		((c) == KEY_F(5))
#define	is_exit(c)		((c) == KEY_F(5))

#define	is_exithelp(c)		((c) == KEY_F(5))
#define	is_helpindex(c)		((c) == KEY_F(3))
#define	is_help(c)		((c) == KEY_F(1) || (c) == KEY_F(6) || \
				 (c) == '?')

#define	is_fkey(c) \
	(c == KEY_F(1) || c == KEY_F(2) || c == KEY_F(3) || \
	c == KEY_F(4) || c == KEY_F(5) || c == KEY_F(6) || \
	c == KEY_F(7) || c == KEY_F(8) || c == KEY_F(9) || \
	c == KEY_F(10))

#define	is_fkey_num(c, num)	(c == KEY_F((num)))

/*
 * macros to interpret field navigation commands
 */
#define	fwd_cmd(c)	((c) == R_ARROW || (c) == D_ARROW || \
			 (c) == CTRL_F || (c) == CTRL_N  || \
			 (c) == TAB)

#define	bkw_cmd(c)	((c) == L_ARROW || (c) == U_ARROW || \
			 (c) == CTRL_B || (c) == CTRL_P || \
			 (c) == KEY_BTAB)

#define	pgup_cmd(c)	((c) == CTRL_U || (c) == KEY_PPAGE))
#define	pgdn_cmd(c)	((c) == CTRL_D || (c) == KEY_NPAGE))

#define	sel_cmd(c)	((c) == RETURN)
#define	alt_sel_cmd(c)  ((c) == RETURN || (c) == 'x' || (c) == SPACE)

#define	nav_cmd(c)	(fwd_cmd(c) || bkw_cmd(c) || sel_cmd(c) || is_fkey(c))

#define	esc_cmd(c)	((c) == ESCAPE)

/*
 * Flags used by wmenu
 */
#define	M_READ_ONLY		0x01
#define	M_RADIO			0x02
#define	M_RADIO_ALWAYS_ONE	0x04
#define	M_CHOICE_REQUIRED	0x08
#define	M_RETURN_TO_CONTINUE	0x10

/* struct used for scrolling list */
typedef struct {
	char *str;
	int row;
} ttyScrollingListTable;


#define	INDENT0		2
#define		INDENT1		8
#define		INDENT2		16
#define		INDENT3		24

typedef int    Callback_proc(void *, void *);
typedef int    Callback_ExitProc(WINDOW *);

typedef enum fieldtype {
	RSTRING = 0,
	LSTRING = 1,
	NUMERIC = 2,
	DISPLAY = 3,
	INSENSITIVE = 4
} FieldType;

typedef struct {
	short	r;
	short	c;
} NRowCol;

typedef struct {
	HelpEntry	help;
	NRowCol	loc;
	short	sel;
	char	*label;
	void	*data;
} ChoiceItem;

/*
 * structures for formatting labels (e.g. above a scrolling list)
 */
typedef char **ttyLabelRowData;

typedef struct {
	char *heading;
	int heading_rows;
	u_int max_width;
} ttyLabelColData;

/*
 * function types used to let apps register some functions
 */
typedef void (*Fkeys_init_func) (int force_alternates);
typedef int (*Fkey_check_func) (u_long keys, int ch);


/*
 * for field editing in various curses screens:
 */
typedef struct {
	int r;		/* row */
	int c;		/* col */
	int len;	/* viewable field width */
	int maxlen;	/* maximum string length */
	int type;	/* { RSTRING | LSTRING } */
	char *value;	/* opaque ptr to current val */
} EditField;


/*
 * tty external functions
 */
#ifdef __cplusplus
extern	"C" {
#endif

/*
 * tty color
 */
extern void wcolor_set_bkgd(WINDOW *, HiLite_t);
extern void wcolor_on(WINDOW *, HiLite_t);
extern void wcolor_off(WINDOW *, HiLite_t);
extern void wfocus_on(WINDOW *, int, int, char *);
extern void wfocus_off(WINDOW *, int, int, char *);

/*
 * tty help
 */
extern void HelpInitialize(char *help_dir);
extern HelpEntry HelpGetTopLevelEntry(void);
extern int  show_help(void *, void *);
extern void do_help_index(WINDOW *, help_t, char *);

/*
 * tty_list
 */
extern int
show_scrolling_list(
	WINDOW *win,
	int start_row,
	int start_col,
	int height,
	int width,
	char **label_rows,
	int num_label_rows,
	ttyScrollingListTable *entries,
	int num_entries,
	HelpEntry _help,
	Callback_ExitProc exit_proc,
	u_long  fkeys);

/*
 * tty_menu
 */
extern void	fkey_wmenu_check_register(Fkey_check_func func);
extern int	wmenu(WINDOW    *w,
	int	starty,
	int	startx,
	int	height,
	int	width,
	Callback_proc	*help_proc,
	void	*help_data,
	Callback_proc	*select_proc,
	void	*select_data,
	Callback_proc	*deselect_proc,
	void	*deselect_data,
	char	*label,
	char	**items,
	size_t	nitems,
	void	*selected,
	int	flags,
	u_long	keys);

/*
 * tty_msg
 */
extern void tty_MsgFunction(UI_MsgStruct *msg_info);

/*
 * tty_init
 */
extern int	fkey_index(u_long bits);
extern void	fkey_notice_check_register(Fkey_check_func func);
extern void	fkey_mvwgets_check_register(Fkey_check_func func);
extern void	wfooter_fkeys_init(Fkey *fkeys, int num_fkeys,
		Fkeys_init_func fkeys_init_func);
extern void	wfooter_fkeys_func_init(Fkey *f_keys, int full_init);

/*
 * tty_wins
 */
extern void wfooter_func_set(int fkey, char *str);
extern char *wfooter_func_get(int fkey);
extern int	show_choices(WINDOW *, int, int, int, int, ChoiceItem *,
			int);
extern void	scroll_prompts(WINDOW *w,
			int top, int col, int scr, int max, int npp);
extern int	wget_field(WINDOW *w, int, int, int, int, int, char *,
		u_long);
extern void	wheader(WINDOW *w, const char *title);
extern void	wfooter(WINDOW *w, u_long which_keys);
extern void	wcursor_hide(WINDOW *w);
extern int simple_notice(WINDOW *, int, char *, char *);
extern int yes_no_notice(WINDOW *, int, int, int, char *, char *);
extern void wclear_status_msg(WINDOW *w);
extern void wstatus_msg(WINDOW *w, char *format, ...);
extern int wword_wrap(WINDOW *, const int, const int, const int, const char *);
extern int verify_field_input(EditField * f, FieldType type, int low, int hi);
extern int mvwgets(WINDOW *w,
	int		starty,
	int		startx,
	int		ncols,
	Callback_proc	*help_proc,
	void		*help_data,
	char		*buf,
	int		len,
	int		type_to_wipe,
	u_long		keys);
extern void werror(WINDOW *w, int row, int col, int width, char *fmt);

/*
 * tty_utils
 */
extern int	init_curses(void);
extern int	start_curses(void);
extern void	end_curses(int do_clear, int top);
extern int	wzgetch(WINDOW *w, u_long fkeys);
extern void	flush_input(void);
extern void set_clearscr(int);
extern int get_clearscr(void);
extern int peekch(void);
extern void tty_cleanup(void);
extern int count_lines(char *, int);
extern int tty_CheckInput(ulong which_fkeys, int ch);
extern void tty_GetRowColData(
	ttyLabelRowData *row_data,
	int num_rows,
	ttyLabelColData *col_data,
	int num_cols,
	int space_len,
	char ***label_rows,
	int *num_label_rows,
	char ***entries,
	int *entry_row_len);

extern int	show_menu(WINDOW *, int, int, int, int, char **, char *,
		int);

#ifdef __cplusplus
}
#endif

#endif	/* _SPMITTY_API_H */
