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

#pragma	ident	"@(#)tty_subr.c 1.13 93/12/06"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <stropts.h>
#include <sys/conf.h>
#include <netdb.h>
#include <fcntl.h>
#include "tty_defs.h"
#include "tty_help.h"
#include "tty_msgs.h"

#ifdef DEV
/*
 * Development-specific stuff
 */
#include <assert.h>

/*ARGSUSED*/
static void
onintr(int sig)
{
	end_curses(1, 1);
	exit(1);
}
#endif

static int	ok_to_read;

static int	read_fd;
static int	write_fd;

/*
 * void start_stop(int sig)
 *
 * This routine is used to implement the persistent Motif server.
 * We get a SIGUSR1 when we're supposed to start up (add input
 * descriptor) and SIGPOLL when the file descriptor is closed by
 * sysidtool (remove input descriptor).
 *
 * Unfortunately, the signals don't always arrive in the order we
 * expect.  In particular, SIGUSR1 can be delivered before SIGPOLL
 * if a prompt_close/prompt_open pair is used within the same process.
 * To guard against this, we must block both SIGUSR1 and SIGPOLL on
 * entry to this routine and flag SIGUSR1 as "pending" if we haven't
 * gotten SIGPOLL yet (i.e., the input descriptor is active).
 *
 * If this situation occurs, we can essentially consume the [pending]
 * SIGUSR1 signal at the time we get SIGPOLL, since we know sysidtool
 * is waiting for us (we don't need to go through the remove/add cycle).
 */
static void
start_stop(int sig)
{
	static int	start_pending;
	struct strrecvfd recvfd;
	int	fd;

	if (sig == SIGUSR1) {	/* start */
		if (ok_to_read)
			start_pending = 1;
		else {
			ok_to_read = 1;
			start_pending = 0;
		}
	} else {		/* stop */
		if (!start_pending) {
			ok_to_read = 0;
			(void) ioctl(write_fd, I_FLUSH, FLUSHRW);
		} else {
			start_pending = 0;
			ok_to_read = 1;
		}
		(void) close(0);
	}
	if (ok_to_read && !start_pending) {
		if (ioctl(read_fd, I_RECVFD, &recvfd) >= 0)
			fd = dup2(recvfd.fd, 0);
#ifdef DEV
		else
			assert (errno == 0);

		assert(fd == 0);
#endif
		flush_input();
	}
}

/*ARGSUSED*/
Sysid_err
do_init(int *argcp, char **argv, int read_from, int reply_to)
{
	struct sigaction sa;
	MSG	*mp;

	(void) close(0);

	read_fd = read_from;
	write_fd = reply_to;

	sa.sa_handler = start_stop;
	sa.sa_flags = 0;
	(void) sigemptyset(&sa.sa_mask);
	(void) sigaddset(&sa.sa_mask, SIGPOLL);
	(void) sigaddset(&sa.sa_mask, SIGUSR1);

	(void) sigaction(SIGUSR1, &sa, (struct sigaction *)0);
	(void) sigaction(SIGPOLL, &sa, (struct sigaction *)0);

	(void) sigrelse(SIGUSR1);	/* just in case */
	(void) sigrelse(SIGPOLL);	/* just in case */

#ifdef DEV
	(void) signal(SIGINT, onintr);
#endif
	/*
	 * NB:  We don't start curses in this routine
	 * since the program may not have to prompt the
	 * user for any information and it would look
	 * strange if the screen blanked out for no
	 * apparent reason.
	 */
	for (;;) {
		(void) sighold(SIGUSR1);
		if (!ok_to_read) {
			(void) sigpause(SIGUSR1);
		} else {
			(void) sigrelse(SIGUSR1);
			mp = msg_receive(read_from);
			if (mp != (MSG *)0)
				run_display(mp, reply_to);
		}
	}
	/*NOTREACHED*/
#ifdef lint
	return (SYSID_SUCCESS);
#endif
}

/*
 * UI entry point for generic forms processing.
 */
void
do_form(char *title, char *text, Field_desc *fields, int nfields, int reply_to)
{
	Field_help	help;

	if (is_install_environment())
		form_intro(INTRO_TITLE, INTRO_TEXT,
			get_attr_help(ATTR_NONE, &help), INTRO_ONE_TIME_ONLY);

	(void) form_common(title, text, fields, nfields, _get_err_string);

	if (reply_to != -1)
		reply_fields(fields, nfields, reply_to);
}

/*
 * UI entry point for generic confirmation processing.
 */
void
do_confirm(
	char		*title,
	char		*text,
	Field_desc	*fields,
	int		nfields,
	int		reply_to)
{
	static Field_desc confirm[] = {
	    { FIELD_NONE, (void *)ATTR_CONFIRM, NULL, NULL, NULL,
		-1, -1, -1, -1, FF_CHANGE, NULL }
	};
	Field		confirm_f;
	Field_desc	*f;
	Field_help	help;
	Form		*form;
	int		confirmed;
	int		ch;
	int		i;

	if (is_install_environment())
		form_intro(INTRO_TITLE, INTRO_TEXT,
			get_attr_help(ATTR_NONE, &help), INTRO_ONE_TIME_ONLY);

	confirm[0].help = get_attr_help(ATTR_CONFIRM, &help);

	form = form_create(title, text, fields, nfields);

	confirm_f.f_desc = confirm;
	confirm_f.f_row = confirm_f.f_col = -1;
	confirm_f.f_label_width = 0;
	confirm_f.f_keys = F_HELP | F_CONTINUE | F_CHANGE;

	confirm_f.f_next = form->fields;
	confirm_f.f_prev = form->fields->f_prev;

	form->fields->f_prev->f_next = &confirm_f;
	form->fields->f_prev = &confirm_f;

	ch = form_exec(form, _get_err_string);

	if (is_continue(ch))
		confirmed = TRUE;
	else
		confirmed = FALSE;

	if (reply_to != -1)
		reply_integer(ATTR_CONFIRM, confirmed, reply_to);

	/*
	 * "fields" and associated memory (label,
	 * value) were allocated in ui_get_confirm()
	 * and must be freed here.
	 */
	f = fields;
	for (i = 0; i < nfields; i++) {
		if (f->label)
			free(f->label);
		if (f->value)
			free(f->value);
		f++;
	}
	free(fields);
}

/*
 * Clean up the UI process
 */
/*ARGSUSED*/
void
do_cleanup(char *text, int *do_exit)
{
	int	show_message = curses_on;

	end_curses(1, 1);

	if (show_message && text != (char *)0 && text[0] != '\0') {
		/*
		 * We'll be nice and tack on a newline if
		 * there isn't one already...
		 */
		(void) fprintf(stderr, "%s%s", text,
			text[strlen(text)-1] != '\n' ? "\n" : "");
		(void) fflush(stderr);
	}
}

void
do_message(MSG *mp, int reply_to)
{
	char		buf[1024];
	Prompt_t	prompt_id;

	/*
	 * The incoming Prompt_t is always ignored.
	 */
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&prompt_id, sizeof (prompt_id));
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)buf, sizeof (buf));
	msg_delete(mp);

	/*
	 * Messages are only printed if curses is on.
	 */
	if (curses_on) {
		wstatus_msg(stdscr, "%s", buf);
		wcursor_hide(stdscr);
	}

	prompt_id = (Prompt_t)1;

	reply_integer(ATTR_PROMPT, (int)prompt_id, reply_to);
}

void
do_dismiss(MSG *mp, int reply_to)
{
	/* ignore incoming Prompt_t handle */
	msg_delete(mp);

	if (curses_on == TRUE) {
		/*
		 * XXX [shumway]
		 *
		 * NB:  The functionality of "clear status message
		 * and continue with present form" is not used at
		 * this time and thus has not been fully implemented.
		 * (This program always advances to the next form.)
		 * After wclear_status_msg, the cursor's location
		 * is probably where it was left by wcursor_hide
		 * and it probably has default attributes (i.e.,
		 * highlighting and/or color).  If you intend to
		 * use the "clear and continue with present form"
		 * functionality, you will now have to ensure the
		 * cursor is positioned and rendered appropriately
		 * (i.e., complete the implementation yourself).
		 */
		wclear_status_msg(stdscr);
		wcursor_hide(stdscr);		/* XXX good for now */
		(void) wrefresh(stdscr);
	}

	reply_void(reply_to);
}

void
do_error(char *errstr, int reply_to)
{
	if (curses_on == TRUE) {
		Field_help	help;
		size_t	len = strlen(errstr);
		char	*newstr;
		char	*cp;

		newstr = (char *)xmalloc(len + strlen(DISMISS_TO_ADVANCE) + 3);

		(void) strcpy(newstr, errstr);

		/*
		 * Strip all trailing newlines
		 */
		cp = &newstr[len];	/* *cp == terminating '\0' */
		while (--cp > newstr && *cp == '\n')
			*cp = '\0';

		(void) strcat(newstr, "\n\n");
		(void) strcat(newstr, DISMISS_TO_ADVANCE);

		form_error(ERROR_TITLE, newstr,
					get_attr_help(ATTR_NONE, &help));
		free(newstr);
	} else
		werror(stdscr, 0, 0, 80, errstr);

	reply_void(reply_to);
}
