/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
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

#ifndef lint
#ident	"@(#)xv_tty.c 1.11 94/01/04"
#endif

#include "defs.h"
#include "ui.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <sys/filio.h>
#include <sys/ioctl.h>
#include <xview/notify.h>
#include <xview/termsw.h>
#include <xview/ttysw.h>
#include "Cmd_ui.h"
#include "Base_ui.h"

extern	Cmd_CmdWin_objects *Cmd_CmdWin;
extern	Base_BaseWin_objects *Base_BaseWin;

pid_t	parent;

static Exit_func tty_user_func;
static caddr_t	tty_user_arg;

static pid_t	tty_pid;
static int	proc_input;
static int	proc_output;
static int	tty_input;

static int	catch_all_children = TRUE;

static void tty_control(Cmd_CmdWin_objects *);
static Notify_value get_user_input(Notify_client, int);
static Notify_value get_process_output(Notify_client, int);
static void tty_write(Xv_opaque tty, char *buf);
static int tty_busy(void);
static void tty_free(void);
static void tty_done_proc(Frame frame);
static Notify_value tty_catch_all_children(
			Notify_client, int, Notify_signal_mode);
#ifdef _SYS_RUSAGE_H
static Notify_value tty_exit_func(Notify_client, int, int *, struct rusage *);
static Notify_value tty_exit_cmd(Notify_client, int, int *, struct rusage *);
#else
static Notify_value tty_exit_func(Notify_client, int, int *, void *);
static Notify_value tty_exit_cmd(Notify_client, int, int *, void *);
#endif

/*
 * tty_is_acitve -- makes status of tty
 * window available to other parts of the
 * program.
 */
int
tty_is_active(void)
{
	return (tty_pid != 0);
}

/*
 * tty_exec_func -- Execute a function as a separate
 * process in the tty window.  A second (clean-up)
 * function can be specified to be called when the
 * process exits.
 *
 * The function to execute is declared as
 *
 *	void
 *	func(caddr_t)
 *
 * The cleanup function is declared as
 *
 *	void
 *	func(caddr_t, int)
 *
 * where the 2nd argument is the exit status of
 * the process.
 *
 * tty_exec_func returns SUCCESS if it was able to
 * start the child process, ERR_* otherwise.
 */
int
tty_exec_func(
	void	(*exec_func)(caddr_t),
	caddr_t	exec_arg,
	Exit_func exit_func,
	caddr_t	exit_arg)
{
	Cmd_CmdWin_objects *ip = Cmd_CmdWin;
	pid_t	pid;
	int	ifds[2];
	int	ofds[2];

	if (exit_func) {
		tty_user_func = exit_func;
		tty_user_arg = exit_arg;
	}

	if (pipe(ifds) < 0)
		msg(ip->CmdWin, 1, gettext(
		    "WARNING:  cannot create input pipe:  %s\n"),
			strerror(errno));
	if (pipe(ofds) < 0)
		msg(ip->CmdWin, 1, gettext(
		    "WARNING:  cannot create output pipe:  %s\n"),
			strerror(errno));

	parent = getpid();
	catch_all_children = FALSE;

	pid = fork();
	switch (pid) {
	case -1:	/* error */
		(void) close(ifds[0]);
		(void) close(ifds[1]);
		(void) close(ofds[0]);
		(void) close(ofds[1]);
		catch_all_children = TRUE;
		return (ERR_FORKFAIL);
	case 0:		/* child */
		(void) dup2(ofds[0], 0);
		(void) dup2(ifds[1], 1);
		(void) dup2(ifds[1], 2);
		(void) close(ifds[0]);
		(void) close(ofds[1]);
		break;
	default:	/* parent */
		(void) tty_busy();
		/*
		 * Setting TTY_PID does not cause the normal Xview
		 * wait3 handler to be installed, so we have to set
		 * (not interpose) our wait3 function.
		 */
		xv_set(ip->Term, TTY_PID, pid, NULL);
		notify_set_wait3_func(ip->Term, tty_exit_func, pid);
		notify_set_signal_func(ip->Term,
			tty_catch_all_children, SIGUSR2, NOTIFY_ASYNC);

		tty_pid = pid;

		proc_output = ifds[0];
		(void) close(ifds[1]);

		proc_input = ofds[1];
		(void) close(ofds[0]);

		tty_input = (int)xv_get(ip->Term, TTY_TTY_FD);

		notify_set_input_func(ip->Term, get_user_input, tty_input);
		notify_set_input_func(ip->Term,
			get_process_output, proc_output);

		return (SUCCESS);	/* back to notifier */
	}

	/*
	 * Now in child
	 *	Turn off Xview notifier
	 *	Enable signal handling
	 */
	notify_stop();

	setbuf(stdin, (char *)0);
	setbuf(stdout, (char *)0);
	setbuf(stderr, (char *)0);

	tty_control(ip);

	exec_func(exec_arg);
	exit(0);
	/*NOTREACHED*/
}


/*
 * Called after each fork() to establish the
 * tty window as the new process' controlling
 * terminal.
 */
static void
tty_control(Cmd_CmdWin_objects *ip)
{
	int	fd, pty;
	char	*ptyname;
	struct sigaction vec, ovec;

	vec.sa_handler = SIG_DFL;
	(void) sigemptyset(&vec.sa_mask);
	vec.sa_flags = 0;
	(void) sigaction(SIGWINCH, &vec, (struct sigaction *) 0);

	/*
	 * Become controlling process for the tty
	 * window:  setsid(), and reopen tty.
	 */
	(void) setsid();

	vec.sa_handler = SIG_IGN;
	(void) sigemptyset(&vec.sa_mask);
	vec.sa_flags = 0;
	(void) sigaction(SIGTTOU, &vec, &ovec);

	pty = (int)xv_get(ip->Term, TTY_PTY_FD);
	ptyname = ptsname(pty);
	if (ptyname) {
		fd = open(ptsname(pty), O_RDWR);
		if (fd < 0)
			return;
	}
	(void) close(pty);
	(void) sigaction(SIGTTOU, &ovec, (struct sigaction *) 0);

	/*
	 * restore various signals to their defaults
	 */
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);
}

/*
 * Get input from the user and send it to
 * the process running in the tty window.
 */
static Notify_value
get_user_input(client, fd)
	Notify_client client;
	int	fd;
{
	char	buf[BUFSIZ+1];
	int	ready, n;

	if (ioctl(fd, FIONREAD, &ready) < 0 || tty_input == -1)
		(void) notify_set_input_func(client, NOTIFY_FUNC_NULL, fd);
	else {
		while (ready > 0) {
			n = read(fd, (void *)buf, sizeof (buf) - 1);
			if (n > 0) {
				(void) write(proc_input, (void *)buf, n);
			} else if (n < 0)
				break;
			ready -= n;
		}
	}
	return (NOTIFY_DONE);
}

/*
 * Get process output and throw it up
 * on the tty window.
 */
static Notify_value
get_process_output(client, fd)
	Notify_client client;
	int	fd;
{
	char	buf[BUFSIZ+1];
	int	ready, n;

	if (ioctl(fd, FIONREAD, &ready) < 0)
		(void) notify_set_input_func(client, NOTIFY_FUNC_NULL, fd);
	else {
		while (ready > 0) {
			n = read(fd, (void *)buf, sizeof (buf) - 1);
			if (n > 0) {
				buf[n] = '\0';
				tty_write(client, buf);
			} else if (n < 0)
				break;
			ready -= n;
		}
	}
	return (NOTIFY_DONE);
}

/*ARGSUSED*/
static Notify_value
tty_exit_func(client, childpid, statusp, rusage)
	Notify_client client;
	int	childpid;
	int	*statusp;
#ifdef _SYS_RUSAGE_H
	struct rusage *rusage;
#else
	void	*rusage;
#endif
{
	if (childpid != tty_pid)
		return (NOTIFY_DONE);
	/*
	 * The process may exit before any event
	 * is generated for its output so we call
	 * the read routine before blowing away
	 * the file descriptor and the callback.
	 */
	(void) get_process_output(client, proc_output);
	(void) notify_set_input_func(client, NOTIFY_FUNC_NULL, proc_output);
	(void) close(proc_output);
	(void) close(proc_input);
	(void) notify_set_input_func(client, NOTIFY_FUNC_NULL, tty_input);

	if (WEXITSTATUS(*statusp) != 0) {
		asktoproceed(Cmd_CmdWin->CmdWin,
		    gettext("pid %d exits status %d\n"),
			childpid, WEXITSTATUS(*statusp));
	}

	if (tty_user_func) {
		(*tty_user_func)(tty_user_arg, WEXITSTATUS(*statusp));
		tty_user_func = 0;
	}
	tty_pid = 0;
	tty_free();

	return (NOTIFY_DONE);
}

static void
tty_write(Xv_opaque tty, char *buf)
{
	/*
	 * XXX  Xview bug workaround (1083304)
	 *
	 * Hack to work around bug in ttysw_output
	 * A single newline doesn't get expanded to
	 * cr-lf, so expand it ourselves.
	 */
	if (strcmp(buf, "\n") == 0)
		(void) strcpy(buf, " \n");
	/*
	 * Send to tty window
	 */
	ttysw_output((Tty)tty, buf, strlen(buf));
}

int
tty_exec_cmd(
	char	*cmd,
	Exit_func exit_func,
	caddr_t	exit_arg)
{
	Cmd_CmdWin_objects *ip = Cmd_CmdWin;
	Xv_termsw *term = (Xv_termsw *)ip->Term;
	char	*argv[100], *start, *end, **av;
	char	argbuf[BUFSIZ];
	pid_t	pid;
	int	s;

	tty_user_func = exit_func;
	tty_user_arg = exit_arg;

	(void) tty_busy();

	start = cmd;
	av = argv;
	while (*start) {
		while (*start && isspace(*start))
			start++;
		end = start;
		if (*end == '"') {
			end++;
			while (*end && *end != '"')
				end++;
			if (*end == '"')
				end++;
		} else
			while (*end && !isspace(*end))
				end++;
		(void) strncpy(argbuf, start, end - start);
		argbuf[end - start] = '\0';
		*av++ = xstrdup(argbuf);
		start = end;
	}
	*av = 0;
	s = xv_set(ip->Term, TTY_ARGV, argv, NULL);
	pid = (pid_t)xv_get(ip->Term, TTY_PID);

	/*
	 * XXX  There is a race condition in Xview -- if the process
	 * exits before notify_interpose_func can do a kill(0) on
	 * it, the notifier routine is never registered and thus
	 * is never called.  This essentially causes us to hang.
	 * We defend against this by reaping the process and calling
	 * the notifier routine ourselves.
	 */
	if (notify_interpose_wait3_func(
	    term->private_tty, tty_exit_cmd, pid) == NOTIFY_SRCH) {
		s = 0;
		(void) waitpid(pid, &s, WNOHANG);
		/*
		 * XXX reset the tty's idea of current
		 * pid, essentially telling it the process
		 * has exited.  This normally occurs in
		 * the tty's notify_wait3 function.
		 */
		xv_set(ip->Term, TTY_ARGV, TTY_ARGV_DO_NOT_FORK, NULL);
		(void) tty_exit_cmd(term->private_tty, (pid_t)-1, &s, 0);
	} else
		tty_pid = pid;
	return (SUCCESS);
}

/*ARGSUSED*/
static Notify_value
tty_exit_cmd(client, childpid, statusp, rusage)
	Notify_client client;
	int	childpid;	/* last child to exit */
	int	*statusp;	/* status of last child to exit */
#ifdef _SYS_RUSAGE_H
	struct rusage *rusage;	/* not used */
#else
	void	*rusage;	/* not used */
#endif
{
	if (childpid > 0)
		notify_next_wait3_func(client, childpid, statusp, rusage);

	if (childpid == tty_pid) {
		if (WEXITSTATUS(*statusp) != 0) {
			asktoproceed(Cmd_CmdWin->CmdWin, gettext(
			    "pid %d exits status %d\n"),
				childpid, WEXITSTATUS(*statusp));
		}
		if (tty_user_func) {
			(*tty_user_func)(tty_user_arg, WEXITSTATUS(*statusp));
			tty_user_func = 0;
		}
		tty_pid = 0;
		tty_free();
	}

	return (NOTIFY_DONE);
}

static int
tty_busy(void)
{
	if (tty_pid) {
		asktoproceed(Cmd_CmdWin->CmdWin, gettext(
			"You must wait for the currently-\n\
running program to exit before\nyou can execute another."));
		return (ERR_NOACCESS);
	}

	xv_set(Cmd_CmdWin->CmdWin,
		FRAME_DONE_PROC,	tty_done_proc,
		NULL);

	xv_set(Cmd_CmdWin->CmdWin, XV_SHOW, TRUE, NULL);
	xv_set(Cmd_CmdWin->CmdWin, FRAME_CMD_PUSHPIN_IN, TRUE, NULL);
	xv_set(Base_BaseWin->BaseWin, FRAME_BUSY, TRUE, NULL);

	return (SUCCESS);
}

static void
tty_free(void)
{
	struct sigaction vec;

	xv_set(Base_BaseWin->BaseWin, FRAME_BUSY, FALSE, NULL);

	vec.sa_handler = SIG_IGN;
	vec.sa_flags = 0;
	(void) sigemptyset(&vec.sa_mask);
	(void) sigaction(SIGWINCH, &vec, (struct sigaction *) 0);
}

static void
tty_done_proc(Frame frame)
{
	if (xv_get(Base_BaseWin->BaseWin, FRAME_BUSY) == FALSE)
		xv_set(frame, XV_SHOW, FALSE, NULL);
	else
		asktoproceed(frame, gettext(
		    "You cannot dismiss the command I/O window\n\
while you are installing or removing software."));
}

/*ARGSUSED*/
static Notify_value
tty_catch_all_children(client, sig, when)
	Notify_client		client;
	int			sig;
	Notify_signal_mode	when;
{
	catch_all_children = TRUE;
	(void) kill(tty_pid, SIGUSR2);	/* I'm ready to reap you */
	return (NOTIFY_DONE);
}

pid_t
wait(int *status)
{
	return (waitpid(-1, status, WUNTRACED));
}

pid_t
waitpid(pid_t pid, int *status, int flags)
{
	extern pid_t _waitpid(pid_t, int *, int);

	if (pid != -1 || catch_all_children == TRUE)
		return (_waitpid(pid, status, flags));
	else
		return (_waitpid(0, status, flags));
}
