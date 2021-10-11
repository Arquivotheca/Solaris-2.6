#ifndef TTY_UTILS_H
#define	TTY_UTILS_H

/*
 * Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

#pragma	ident	"@(#)tty_utils.h 1.12 96/06/13"

#include <stdarg.h>
#include <curses.h>
#include <term.h>
#include <sys/ttychars.h>
#include "tty_color.h"

#ifdef __cplusplus
extern	"C"	{
#endif

/*
 * various keystroke definitions for user input
 */
#ifndef	KEY_LEFT	/* from 5include/curses.h */
#define	KEY_DOWN	0402		/* Sent by terminal down arrow key */
#define	KEY_UP		0403		/* Sent by terminal up arrow key */
#define	KEY_LEFT	0404		/* Sent by terminal left arrow key */
#define	KEY_RIGHT	0405		/* Sent by terminal right arrow key */
#define	KEY_HOME	0406		/* Sent by home key. */
#define	KEY_BACKSPACE	0407		/* Sent by backspace key */
#define	KEY_END		0550		/* end key */
#endif

#define	L_ARROW		KEY_LEFT	/* from curses.h */
#define	R_ARROW		KEY_RIGHT	/* from curses.h */
#define	U_ARROW		KEY_UP		/* from curses.h */
#define	D_ARROW		KEY_DOWN	/* from curses.h */

#define	CTRL_F		CTRL('f')
#define	CTRL_B		CTRL('b')
#define	CTRL_U		CTRL('u')
#define	CTRL_D		CTRL('d')
#define	CTRL_N		CTRL('n')
#define	CTRL_P		CTRL('p')
#define	CTRL_Z		CTRL('z')
#define	CTRL_C 		CTRL('c')
#define	CTRL_A 		CTRL('a')
#define	CTRL_E 		CTRL('e')
#define	CTRL_R 		CTRL('r')
#define	CTRL_G		CTRL('g')
#define	CTRL_H		CTRL('h')

#define	RETURN		0x0a		/* ASCII RETURN */
#define	TAB		0x09		/* ASCII TAB */
#define	DONE		0x1b		/* ASCII ESCAPE */
#define	ESCAPE		0x1b		/* ASCII ESCAPE */
#define	SPACE		0x20		/* ASCII SPACE */
#define	CONTINUE	KEY_F(2)	/* Function key F2 */

#if !defined(NOMACROS) && !defined(beep)
#define	beep()		(void) printf("\007");
#endif

#if !defined(MIN)
#define	MIN(a, b)	((int) (a) > (int) (b) ? (b) : (a))
#endif

#if !defined(MAX)
#define	MAX(a, b)	((int) (a) > (int) (b) ? (a) : (b))
#endif

#define	MINLINES	24
#define	MINCOLS		80

/*
 * XXX These should be defined in desired
 * presentation order.  If you change the
 * order, you must also change the function
 * key table in tty_utils.c.  The bits used
 * must also be contiguous.
 */
#define	F_OKEYDOKEY	0x1UL
#define	F_SPARE1	0x2UL
#define	F_SPARE2	0x4UL
#define	F_CONTINUE	0x8UL
#define	F_BEGIN		0x10UL
#define	F_EXITINSTALL	0x20UL
#define	F_UPGRADE	0x40UL
#define	F_GOBACK	0x80UL
#define	F_TESTMOUNT	0x100UL
#define	F_DELETE	0x200UL
#define	F_PRESERVE	0x400UL
#define	F_SPARE3	0x800UL
#define	F_SHOWEXPORTS	0x1000UL
#define	F_CHANGE	0x2000UL
#define	F_CHANGETYPE	0x4000UL
#define	F_DOREMOTES	0x8000UL
#define	F_CUSTOMIZE	0x10000UL
#define	F_EDIT		0x20000UL
#define	F_CREATE	0x40000UL
#define	F_MORE		0x80000UL
#define	F_INSTALL	0x100000UL
#define	F_ADDNEW	0x200000UL
#define	F_SPARE4	0X400000UL
#define	F_CANCEL	0x800000UL
#define	F_EXIT		0x1000000UL
#define	F_HELP		0x2000000UL
#define	F_GOTO		0x4000000UL
#define	F_MAININDEX	0x8000000UL
#define	F_TOPICS	0x10000000UL
#define	F_REFER		0x20000000UL
#define	F_HOWTO		0x40000000UL
#define	F_EXITHELP	0x80000000UL

/*
 * These can be defined in any order you want.
 * They are matched up to the appropriate entries
 * in the function key descriptor table by the
 * initialization code in wfooter().
 */
#define	DESC_F_OKEYDOKEY	dgettext("SUNW_INSTALL_SYSID", "OK")
#define	DESC_F_SPARE1 		dgettext("SUNW_INSTALL_SYSID", "Spare-1")
#define	DESC_F_SPARE2 		dgettext("SUNW_INSTALL_SYSID", "Spare-2")
#define	DESC_F_CONTINUE		dgettext("SUNW_INSTALL_SYSID", "Continue")
#define	DESC_F_BEGIN		dgettext("SUNW_INSTALL_SYSID", "Begin Installation")
#define	DESC_F_EXITINSTALL	dgettext("SUNW_INSTALL_SYSID", "Exit Installation")
#define	DESC_F_UPGRADE		dgettext("SUNW_INSTALL_SYSID", "Upgrade System")
#define	DESC_F_GOBACK		dgettext("SUNW_INSTALL_SYSID", "Go Back")
#define	DESC_F_TESTMOUNT	dgettext("SUNW_INSTALL_SYSID", "Test Mount")
#define	DESC_F_ADDNEW		dgettext("SUNW_INSTALL_SYSID", "New")
#define	DESC_F_PRESERVE		dgettext("SUNW_INSTALL_SYSID", "Preserve")
#define	DESC_F_SPARE3		dgettext("SUNW_INSTALL_SYSID", "Spare-3")
#define	DESC_F_SHOWEXPORTS	dgettext("SUNW_INSTALL_SYSID", "Show Exports")
#define	DESC_F_CHANGE		dgettext("SUNW_INSTALL_SYSID", "Change")
#define	DESC_F_CHANGETYPE	dgettext("SUNW_INSTALL_SYSID", "Change Type")
#define	DESC_F_DOREMOTES	dgettext("SUNW_INSTALL_SYSID", "Remote Mounts")
#define	DESC_F_CUSTOMIZE	dgettext("SUNW_INSTALL_SYSID", "Customize")
#define	DESC_F_EDIT		dgettext("SUNW_INSTALL_SYSID", "Edit")
#define	DESC_F_CREATE		dgettext("SUNW_INSTALL_SYSID", "Create")
#define	DESC_F_MORE		dgettext("SUNW_INSTALL_SYSID", "More")
#define	DESC_F_INSTALL		dgettext("SUNW_INSTALL_SYSID", "New Install")
#define	DESC_F_SPARE4		dgettext("SUNW_INSTALL_SYSID", "Spare")
#define	DESC_F_CANCEL		dgettext("SUNW_INSTALL_SYSID", "Cancel")
#define	DESC_F_EXIT		dgettext("SUNW_INSTALL_SYSID", "Exit")
#define	DESC_F_DELETE		dgettext("SUNW_INSTALL_SYSID", "Delete")
#define	DESC_F_HELP		dgettext("SUNW_INSTALL_SYSID", "Help")
#define	DESC_F_GOTO		dgettext("SUNW_INSTALL_SYSID", "Goto")
#define	DESC_F_MAININDEX	dgettext("SUNW_INSTALL_SYSID", "Main Index")
#define	DESC_F_TOPICS		dgettext("SUNW_INSTALL_SYSID", "Topics")
#define	DESC_F_REFER		dgettext("SUNW_INSTALL_SYSID", "Reference")
#define	DESC_F_HOWTO		dgettext("SUNW_INSTALL_SYSID", "How To")
#define	DESC_F_EXITHELP		dgettext("SUNW_INSTALL_SYSID", "Exit Help")

#define	is_escape(c)		((c) == ESCAPE)

#define	is_ok(c)		((c) == KEY_F(2))
#define	is_index(c)		((c) == KEY_F(2))
#define	is_continue(c)		((c) == KEY_F(2))
#define	is_begin(c)		((c) == KEY_F(2))
#define	is_exitinstall(c)	((c) == KEY_F(2))
#define	is_upgrade(c)		((c) == KEY_F(2))

#define	is_goback(c)		((c) == KEY_F(3))
#define	is_testmount(c)		((c) == KEY_F(3))
#define	is_delete(c)		((c) == KEY_F(3))

#define	is_preserve(c)	 	((c) == KEY_F(4))
#define	is_showexports(c) 	((c) == KEY_F(4))
#define	is_change(c)		((c) == KEY_F(4))
#define	is_changetype(c)	((c) == KEY_F(4))
#define	is_doremotes(c)		((c) == KEY_F(4))
#define	is_customize(c)		((c) == KEY_F(4))
#define	is_edit(c)		((c) == KEY_F(4))
#define	is_create(c)		((c) == KEY_F(4))
#define	is_more(c)		((c) == KEY_F(4))
#define	is_install(c)		((c) == KEY_F(4))

#define	is_addnew(c)		((c) == KEY_F(5))
#define	is_spare4(c)		((c) == KEY_F(5))
#define	is_cancel(c)		((c) == KEY_F(5))
#define	is_exit(c)		((c) == KEY_F(5))
#define	is_exithelp(c)		((c) == KEY_F(5))

#define	is_help(c)		((c) == KEY_F(1) || (c) == KEY_F(6) || \
					(c) == '?')


/*
 * macros to interpret field navigation commands
 */
#define	fwd_cmd(c)	((c) == R_ARROW || (c) == D_ARROW || \
			    (c) == CTRL_F || (c) == CTRL_N || \
			    (c) == TAB)

#define	bkw_cmd(c)	((c) == L_ARROW || (c) == U_ARROW || \
			    (c) == CTRL_B  || (c) == CTRL_P || \
			    (c) == KEY_BTAB)

#define	pgup_cmd	((c) == CTRL_U || (c) == KEY_PPAGE))
#define	pgdn_cmd	((c) == CTRL_D || (c) == KEY_NPAGE))

#define	sel_cmd(c)	((c) == RETURN)

#define	alt_sel_cmd(c)	((c) == RETURN || (c) == 'x' || (c) == SPACE)

#define	esc_cmd(c)	((c) == ESCAPE)

#define	nav_cmd(c)	(fwd_cmd(c) || bkw_cmd(c) || \
			    sel_cmd(c) || is_help(c) || is_continue(c) || \
			    is_cancel(c) || is_change(c) || is_exit(c))

#ifndef MIN
#define	MIN(a, b)	((int) (a) > (int) (b) ? (b) : (a))
#endif
#ifndef MAX
#define	MAX(a, b)	((int) (a) > (int) (b) ? (a) : (b))
#endif

/*
 * Flags used by wmenu
 */
#define	M_READ_ONLY		0x01
#define	M_RADIO			0x02
#define	M_RADIO_ALWAYS_ONE	0x04
#define	M_CHOICE_REQUIRED	0x08
#define	M_RETURN_TO_CONTINUE	0x10

/*
 * Flags used by wget_keys
 */
#define	GK_CASESENS		0x01
#define	GK_VALIDATE		0x02
#define	GK_RETURNOK		0x04
#define	GK_DONEOK		0x08

#define	INDENT0			2
#define	INDENT1			8
#define	INDENT2			16
#define	INDENT3			24

typedef	int	Callback_proc(void *, void *);

extern int	curses_on;

extern int	HeaderLines;
extern int	FooterLines;

extern int	start_curses(void);
extern void	end_curses(int, int);

extern int	wword_wrap(WINDOW *, const int, const int, const int, const char *);

extern int	wzgetch(WINDOW *, u_long);
extern void	flush_input(void);
extern void	wheader(WINDOW *, const char *);
extern void	wfooter(WINDOW *, u_long);
extern void	wcursor_hide(WINDOW *);
extern int	wmenu(WINDOW *, int, int, int, int,
				Callback_proc *, void *,
				Callback_proc *, void *,
				Callback_proc *, void *,
				char *, char **, size_t, void *, int, u_long);
extern void	wintro(Callback_proc *, void *);
extern void	scroll_prompts(WINDOW *, int, int, int, int, int);

/*
 * These routines are not yet part of the common
 * ttinstall/sysidtool/kdconf interface.
 */
/*PRINTFLIKE5*/
extern int	mvwgets(WINDOW *, int, int, int,
				Callback_proc *, void *,
				char *, int, int, u_long);
/*PRINTFLIKE5*/
extern void	werror(WINDOW *, int, int, int, char *, ...);
extern void	wclear_status_msg(WINDOW *);
/*PRINTFLIKE2*/
extern void	wstatus_msg(WINDOW *, char *, ...);

#ifdef __cplusplus
}
#endif

#endif	/* !TTY_UTILS_H */
