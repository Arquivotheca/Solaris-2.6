#ifndef TTY_DEFS_H
#define	TTY_DEFS_H

/*
 * Copyright (c) 1993-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)tty_defs.h 1.5 96/02/05"

#include <stdarg.h>
#include <curses.h>
#include <term.h>
#include <sys/ttychars.h>
#include "sysid_ui.h"
#include "tty_utils.h"
#include "tty_form.h"

#define	HELPDIR		"sysidtty.help"

/*
 * The following group of routines implement the
 * external interface for the tty dynamically-
 * loadable user interface module.
 */
extern Sysid_err	do_init(int *, char **, int, int);
extern void		do_cleanup(char *, int *);
extern Form_proc	do_form;
extern Form_proc	do_confirm;
extern void		do_error(char *, int);
extern void		do_message(MSG *, int);
extern void		do_dismiss(MSG *, int);
extern void		do_get_terminal(MSG *, int);
extern void		do_get_password(MSG *, int);
extern TZ_proc		do_get_timezone;
extern LL_proc		do_get_locale;

extern char		*get_err_string(int, int, va_list);
extern char		*get_attr_title(Sysid_attr);
extern char		*get_attr_text(Sysid_attr);
extern char		*get_attr_prompt(Sysid_attr);
extern char		*get_attr_name(Sysid_attr);
extern Field_help	*get_attr_help(Sysid_attr, Field_help *);

extern void	wget_keys(WINDOW *, char *, int, char *, int);
extern char	*promptstr(char *);

extern char	*_get_err_string(int, int, ...);

#endif	/* !TTY_DEFS_H */
