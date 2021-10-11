#ifndef lint
#pragma ident "@(#)tty_strings.h 1.1 96/03/07 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	tty_strings.h
 * Group:	libspmitty
 * Description:
 */

#ifndef	_TTY_STRINGS_H
#define	_TTY_STRINGS_H

#ifndef TEXT_DOMAIN
#define	TEXT_DOMAIN	"SUNW_INSTALL_LIBTTY"
#endif

#ifndef ILIBSTR
#define	ILIBSTR(x)	dgettext(TEXT_DOMAIN, x)
#endif

/*
 * function key strings
 */

/*
 * These can be define	d in any order you want.
 * They are matched up to the appropriate entries
 * in the function key descriptor table by the
 * initialization code in wfooter().
 */
#define	DESC_F_OKEYDOKEY	ILIBSTR("OK")
#define	DESC_F_CONTINUE	ILIBSTR("Continue")
#define	DESC_F_HALT	ILIBSTR("Halt")
#define	DESC_F_CANCEL	ILIBSTR("Cancel")
#define	DESC_F_EXIT	ILIBSTR("Exit")
#define	DESC_F_HELP	ILIBSTR("Help")
#define	DESC_F_GOTO	ILIBSTR("Goto")
#define	DESC_F_MAININDEX	ILIBSTR("Main Index")
#define	DESC_F_TOPICS	ILIBSTR("Topics")
#define	DESC_F_REFER	ILIBSTR("Reference")
#define	DESC_F_HOWTO	ILIBSTR("How To")
#define	DESC_F_EXITHELP	ILIBSTR("Exit Help")

/*
 * tty_help.c
 */
#define	TTY_HELP_NOHELP_TITLE	ILIBSTR("No Help Available")
#define	TTY_HELP_NOHELP_NOTICE	ILIBSTR(\
	"The load of the help indexes failed.  No Help is available.")

#define	TTY_HELP_NOHELPTOPIC_TITLE	ILIBSTR("Help Topic Not Available")
#define	TTY_HELP_NOHELPTOPIC_TEXT ILIBSTR(\
	"The load of the help text for the topic \"%s\" failed.")

#define	TTY_HELP_CATEGORY	ILIBSTR(\
	"To make a selection, use the arrow keys to highlight " \
	"the option and press Return to mark it [X].\n\n" \
	"To go to your selection, choose F2.")

#define	TTY_HELP_MAIN_INDEX	ILIBSTR("Help Main Index")
#define	TTY_HELP_SUBJECTS	ILIBSTR("Help Subjects")

#define	TTY_HELP_SYNTAX_ERR	ILIBSTR(\
	"syntax error:\n\tfile=%s\n\tline number=%d,\n\t=%s=\n")

#define	TTY_HELP_OPEN_FILE_ERR	ILIBSTR("Can't open %s\n")
#define	TTY_HELP_EOF_ERR	ILIBSTR("unexpected end-of-file")
#define	TTY_HELP_LEADING_TAB_ERR	ILIBSTR("unexpected leading tab")
#define	TTY_HELP_BLANK_LINE_ERR	ILIBSTR("blank line")
#define	TTY_HELP_EXCESS_TOKENS_ERR	ILIBSTR("too many tokens")

/*
 * tty_intro.c
 */
#define	PARADE_INTRO_TITLE	ILIBSTR("The Solaris Installation Program")
#define	PARADE_INTRO_TEXT	ILIBSTR(\
	"You are now interacting with the Solaris installation program.  " \
	"The program is divided into a series of short sections.  At the " \
	"end of each section, you will see a summary of the choices you've " \
	"made, and be given the opportunity to make changes.\n\n" \
	"As you work with the program, you will complete one or more of " \
	"the following tasks:\n\n" \
	"  1 - Identify peripheral devices\n" \
	"  2 - Identify your system\n" \
	"  3 - Install Solaris software\n\n" \
	"About navigation...\n\n" \
	"  - The mouse cannot be used\n\n" \
	"  - If your keyboard does not have function keys, or they do not " \
	"respond,\n    press ESC; the legend at the bottom of the screen "\
	"will change to \n    show the ESC keys to use for navigation.")

/*
 * tty_util.c
 */
#define	TTY_ROWCOL_SIZE_ERR	ILIBSTR(\
	"The screen must have at least %d rows and %d columns.\n\r")

#define	TTY_ROWCOL_SIZE_INFO_ERR	ILIBSTR(\
	"Your current screen only has %d rows and %d columns.\n\r")

#define	TTY_ERROR_RETURN_TO_CONT	ILIBSTR(\
	"Press Return to continue")

#endif /* _TTY_STRINGS_H */
