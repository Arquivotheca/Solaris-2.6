/*LINTLIBRARY*/
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

#pragma	ident	"@(#)tty_intro.c 1.5 96/06/17"

#include <unistd.h>
#include <libintl.h>

#include "tty_utils.h"

#include "sysid_ui.h"
#include "tty_msgs.h"
#include "tty_help.h"

void
wintro(Callback_proc *help_proc, void *help_data)
{
	u_long	keys;
	int	ch;

	if (unlink(PARADE_INTRO_FILE) == 0) {

		(void) start_curses();
		wclear(stdscr);

		wheader(stdscr, PARADE_INTRO_TITLE);

		(void) wword_wrap(stdscr, 2, 2, 75, PARADE_INTRO_TEXT);

		keys = F_CONTINUE;
		if (help_proc != (Callback_proc *)0)
			keys |= F_HELP;

		wfooter(stdscr, keys);
		wcursor_hide(stdscr);

		for (;;) {
			ch = wzgetch(stdscr, keys);

			if (is_continue(ch))
				break;

			if (is_help(ch) && help_proc != (Callback_proc *)0)
				(*help_proc)(help_data, (void *)0);
			else
				beep();
		}
	}
}
