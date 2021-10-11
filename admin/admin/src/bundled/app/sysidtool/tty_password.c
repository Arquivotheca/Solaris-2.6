/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */


/*
 * Copyright (c) 1991,1992,1993 Sun Microsystems, Inc.  All Rights Reserved.
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

#pragma	ident	"@(#)tty_password.c 1.6 96/06/13"

/*
 *	File:		password.c
 *
 *	Description:	This file contains the routines needed to prompt
 *			the user for the root password.  It returns both
 *			the clear-text password, and the encrypted password.
 */

#include <time.h>
#include <crypt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include "tty_defs.h"
#include "tty_msgs.h"
#include "tty_utils.h"

#ifdef USE_XPG4_WCS
/*
 * If strwidth() is not adopted in CDE/s495, convert x to wide char
 * string and use wcswidth(x).
 */
#define MBWIDTH(x)		strwidth(x)
#else
#define MBWIDTH(x)		eucscol(x)
#endif

static	int	get_pw(char *);

static	int		dialog_row;
static	int		prompt_row;
static	int		prompt_col;




/*
 * do_get_password:
 *
 *	This routine is the client interface routine used for
 *	retrieving the root password from the user.
 *
 *	Display a prompt that asks for a password.  Re-prompt to verify
 *	the password.  If the passwords do not match, start all over
 *	again.
 *
 *	Output: If successful, this routine returns a status of 0, copies
 *		the password to the string pointed to by passwd, and copies
 *		the encrypted password to the string pointed to by
 *		e_passwd.  If an error occurs, this routine returns -1.
 */

void
do_get_password(MSG *mp, int reply_to)
{
	boolean_t done = B_FALSE;
	boolean_t needs_instructions = B_TRUE;
	char	pw[MAX_PASSWORD+2];
	char	pw2[MAX_PASSWORD+2];
	char	*e_pw;
	int	n;

	msg_delete(mp);

	/* Make sure libcurses has been initialized */
	(void) start_curses();

	/*
	 * Prompt for the root password, and re-prompt to verify that
	 * the password is correct.  Repeat the sequence until the
	 * password is correctly entered and verified.
	 */

	while (!done) {

		/* Do we need to display instructions? */
		if (needs_instructions) {
			wclear(stdscr);
			wrefresh(stdscr);
			prompt_row = wword_wrap(stdscr, 0, 0, MINCOLS,
						PASSWORD_INSTRUCTION);
			dialog_row = wword_wrap(stdscr, prompt_row, 0, MINCOLS, 
						PASSWORD_PROMPT);
			dialog_row +=2;
			(void) wword_wrap(stdscr, dialog_row, 0, MINCOLS,
						PRESS_RETURN_TO_CONTINUE);
			needs_instructions = B_FALSE;
		}

		/* Get the password - first try */
		prompt_col = MBWIDTH(PASSWORD_PROMPT);
		(void) wmove(stdscr, prompt_row, prompt_col);
		(void) wrefresh(stdscr);
		if (get_pw(pw) == -1) {
			werror(stdscr, 0, 0, MINCOLS, PW_TOO_LONG);
			needs_instructions = B_TRUE;
			continue;
		}

		/* Get the password - second try */
		(void) wmove(stdscr, dialog_row, 0);
		wclrtobot(stdscr);
		(void) wword_wrap(stdscr, dialog_row, 0, MINCOLS, PW_REENTER);
		(void) wword_wrap(stdscr, dialog_row + 3, 0, MINCOLS,
						PRESS_RETURN_TO_CONTINUE);
		(void) wmove(stdscr, prompt_row, prompt_col);
		wrefresh(stdscr);
		if ((get_pw(pw2) == -1) || (strcmp(pw, pw2) != 0)) {
			werror(stdscr, 0, 0, MINCOLS, PW_MISMATCH);
			needs_instructions = B_TRUE;
			continue;
		}

		done = B_TRUE;
	}
	wclear(stdscr);
	wrefresh(stdscr);

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_add_arg(mp, ATTR_PASSWORD, VAL_STRING,
		(void *)pw, strlen(pw) + 1);

	/* Encrypt the password */
	if (*pw != NULL) {
		e_pw = encrypt_pw(pw);
		(void) msg_add_arg(mp, ATTR_EPASSWORD, VAL_STRING,
			(void *)e_pw, strlen(e_pw) + 1);
	}

	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}



/*
 * get_pw
 *
 *	Read in the root password without echoing it.  Return the
 *	password at the location pointed to by passwd.  If successful,
 *	this routine returns 0.  If the password is too long, return -1.
 *
 *	passwd should point to a buffer at least MAX_PASSWORD+1 bytes long.
 */

static int
get_pw(char *passwd)
{
	int	  i;
	char	  *cp;
	char	  c;

	i = 0;
	cp = passwd;

	/*
	 * Read characters up to a newline.
	 */

	for (;;) {

		c = (char)getch();

		/* Newline indicates end of password */
		if ((c == '\n') || (c == '\r')) {
			break;
		}

		/* Delete or backspace -- remove a character */
		if ((c == '\b') || (c == '\177')) {
			if (i > 0) {
				i--;
				cp--;
			}
			continue;
		}

		/* Control-U -- delete entire line */
		if (c == 0x15) {
			i = 0;
			cp = passwd;
			continue;
		}

		/* Password too long? */
		if (i == MAX_PASSWORD) {
			return (-1);
		}

		/* Add the character to the password buffer */
		*cp++ = c;
		i++;
	}

	*cp = NULL;
	return (0);
}
