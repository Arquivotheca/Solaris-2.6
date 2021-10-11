/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)ui_layout.c	1.19	94/02/17 SMI"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/procset.h>
#include <X11/Intrinsic.h>

#include "launcher.h"
#include "util.h"

/* file descriptor for /dev/null, useful for dup2() before exec */
extern int	devnull;

/* launcher pid for sigsend() */
extern pid_t	launcher_pid;

/* apptable.c */
extern Widget reset_appTable_launcherInfo(pid_t);
extern void set_appTable_launcherInfo(Widget, pid_t);

/* one second timeout for timer proc interval */
extern unsigned long	one_sec;

/* just to satisfy UxXt.c */
Widget		UxTopLevel;

/* signal state */
static Boolean		sigchld_initialized = False;
static pid_t		sigusr1_pid = -1;
static pid_t		sigchld_pid = -1;
static int		sigchld_status = -1;

static char		error_app_name[PATH_MAX];

/* Number of apps that are "launching" but not yet mapped to screen */
static int		apps_launching = 0;

void
set_toggle_status(Widget w, Boolean flg)
{
	XtSetSensitive(w, flg);
	XmToggleButtonSetState(w, flg == True ? False : True, NULL);
}

/* ARGSUSED */
void
app_mapped_prop_notify(XEvent *ev)
{

	Atom		actual_type;
	int		actual_format;
	unsigned long	nitems;
	unsigned long	bytes_after;
	unsigned char	* prop;
	Widget		w; /* this is toggle button widget */


	if (XGetWindowProperty(ev->xproperty.display,
			       ev->xproperty.window,
			       ev->xproperty.atom,
			       0L, 2L, False, XA_INTEGER,
			       &actual_type, &actual_format, &nitems,
			       &bytes_after, &prop) == Success) {

		if (*(int *)prop == getpid()) {

			/* Test if there are in fact 2 pids on property 
			 * AND
 			 * Use pid to search appTable for entry 
		         * associated with pid. If found, return
			 * toggle widget and make toggle sensitive.
			 */
			if ((nitems == 2) && 
                           (w = reset_appTable_launcherInfo(*(((int *)prop)+1)))) {
				set_toggle_status(w, True);
			}

		}
	}
}

void
exec_fail_sigusr1_handler(int sig, siginfo_t *info, ucontext_t *context)
{
	/*
	 * Save info and get out as quickly as possible; we have to
	 * let the work proc deal with it as we can't hang out for
	 * long in a signal handler when working with X.
	 */

	/*
	 * This should be a user generated SIGUSR1 when a forked
	 * process failed to exec.  Save the pid for later use
	 * in the work proc, where it determines whether an exit
	 * was normal or the result of a failed exec.
	 */
#ifdef DEBUG
if (info)
    fprintf(stderr, "in exec_fail_sigusr1_handler pid = %d\n",info->si_pid );
else
    fprintf(stderr, "in exec_fail_sigusr1_handler no info\n");
#endif


	if (info != NULL) {

		/*
		 * If user-process generated signal, save pid;
		 * should always be true, but check to make sure.
		 */

		if (info->si_code <= 0) {
			sigusr1_pid = info->si_pid;
		} else {
			sigusr1_pid = -1;
		}

		sigchld_initialized = True;
	}
}


void
exec_fail_sigchld_handler(int sig, siginfo_t *info, ucontext_t *context)
{
	int wstat;
	/*
	 * Save info and get out as quickly as possible; we have to
	 * let the work proc deal with it as we can't hang out for
	 * long in a signal handler when working with X.
	 */

	/*
	 * This should be a kernel generated SIGCHLD when one of
	 * the forked child processes exits.  It may be due to
	 * either a normal child termination, or a process that
	 * failed to exec doing an explicit _exit().  Save the
	 * siginfo structure (which contains the child pid) for
	 * later testing in the work proc; if in the work proc
	 * the saved siginfo pid matches the pid that was saved
	 * in the sigusr handler, we know it was a failed exec
	 * and we'll display an error dialog.
	 */
#ifdef DEBUG
if (info)
    fprintf(stderr,"in exec_fail_sigchld_handler pid = %d\n",info->si_pid );
else
    fprintf(stderr,"in exec_fail_sigchld_handler no info\n");
#endif


	if (info != NULL) {

		/*
		 * If kernel generated signal, save pid; this
		 * should always be true, but check to make sure.
		 */

		if (info->si_code > 0) {
			sigchld_pid = info->si_pid;
		} else {
			sigchld_pid = -1;
		}

		sigchld_status = info->si_status;

		sigchld_initialized = True;
	}
}


/*ARGSUSED*/
void
exec_fail_timer_proc(XtPointer client_data)
{
	int		wstat;
	pid_t		pid;
	Widget 		w = NULL;
	char		msg[512];
	const char	*exec_eacces_msg =
		catgets(catd, 1, 30, "Sorry, inappropriate permission to run \"%s\"");
	const char	*exec_enoent_msg =
		catgets(catd, 1, 31, "Sorry, the specified program \"%s\" does not exist");
	const char	*exec_enomem_msg =
		catgets(catd, 1, 32, "Sorry, insufficient memory available to run \"%s\"");
	const char	*exec_failed_msg =
		catgets(catd, 1, 33, "Sorry, insufficient system resources available to run \"%s\" (exec failed, errno %d)");

	const char	*abnormal_term_msg =
		catgets(catd, 1, 92, "Process %d terminated abnormally.\nSignal %d was received.");

	if (! sigchld_initialized) {

		/* Do nothing, re-register timeout */
		
		(void) XtAppAddTimeOut(GappContext, one_sec,
				       exec_fail_timer_proc, NULL);
		return;
	}

	if (sigusr1_pid == sigchld_pid) {

 		waitpid(sigchld_pid, &wstat, WNOHANG);

		switch (sigchld_status) {
		case EACCES:
			sprintf(msg, exec_eacces_msg, error_app_name);
			break;
		case ENOENT:
			sprintf(msg, exec_enoent_msg, error_app_name);
			break;
		case ENOMEM:
			sprintf(msg, exec_enomem_msg, error_app_name);
			break;
		default:
			sprintf(msg, exec_failed_msg, error_app_name, sigchld_status);
			break;
		}

		if (w = reset_appTable_launcherInfo(sigchld_pid)) {
			set_toggle_status(w, True);
		}

		/* reset sigusr pid, we've handled this failure */
		sigusr1_pid = -1;

		display_error(NULL, (char *)msg);
		sigchld_initialized = False;
	}
	else {
 		while ((pid = waitpid(0, &wstat, WNOHANG)) > 0) {
			if (w = reset_appTable_launcherInfo(pid)) {
				set_toggle_status(w, True);
			}
			if (WIFSIGNALED(wstat)) {
			    sprintf(msg, abnormal_term_msg,pid,WTERMSIG(wstat));
			    display_error(NULL, (char *)msg);
			}
		}
	}

	(void) XtAppAddTimeOut(GappContext, one_sec,
			       exec_fail_timer_proc, NULL);
}


char **
build_args(char * path, const char * argstring)
{
	int nargs = 0;
	char ** args = NULL;
	char *np, * bp;
	char * newargs;
	int i;

	if (argstring == NULL)
		return(NULL);

	if (argstring[0] != '\0') {
		bp = newargs = strdup((char *)argstring);
		do {
			nargs++;
			bp = strchr(bp, ' ');
			bp = bp ? bp+1 : bp;
		} while (bp);
	}

	nargs++; /* to accommodate arg[0] */

	args = (char **) malloc((nargs + 1) * sizeof(char *));
	if (args == NULL)
		fatal(catgets(catd, 1, 34, "Unable to allocate memory for exec'ing application"));

	args[0] = path;
	bp = newargs;
	for (i = 1; i < nargs; i++) {
		np = strchr(bp, ' ');
		if (np)	*np = 0;
		args[i] = bp;
		bp = np + 1;
	}
	args[nargs] = NULL;
	return(args);
}

/*
 * exec_callback -- fork and exec the program specified by the
 * AppInfo structure passed in via callback client_data.
 */

/*ARGSUSED2*/
void
exec_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

	pid_t		cp;
	char		*p;
	int		exit_code;
	char		env[128];
	AppInfo		*app_data = (AppInfo *)client_data;
	static char	msg[512];
	char ** args;
	const char	*fork_failed_msg =
		catgets(catd, 1, 35, "Sorry, insufficient system resources available to run \"%s\" (fork failed)");


	if (app_data && app_data->a_appPath) {

		/* Store the app name for possible error display. */

		if ((p = strrchr(app_data->a_appPath, '/')) != NULL) {
			strcpy(error_app_name, p + 1);
		} else {
			strcpy(error_app_name, app_data->a_appPath);
		}

		args = build_args(app_data->a_appPath, app_data->a_appArgs);

		if ((cp = fork()) == 0) {

			/* child */

			/* Make sure DISPLAY is in env for app */
		
			(void) sprintf(env, "DISPLAY=%s",
				       DisplayString(XtDisplayOfObject(w)));
			(void) putenv(env);

			(void) dup2(devnull, 1);
			(void) dup2(devnull, 2);

			execvp(app_data->a_appPath, args);

			/*
			 * If we get here, the exec returned -- this is
			 * a bad thing, successful exec doesn't return.
			 * send SIGUSR1, handler will register failure,
			 * then _exit() to send SIGCHLD to register exit.
			 * An Xt work proc will compare the registered
			 * failure with the exit pid and put up an error
			 * dialog.
			 */

			exit_code = errno;

			(void) sigsend(P_PID, (id_t)launcher_pid, SIGUSR1);

			_exit(exit_code);

		} else if (cp != -1) {

			/* parent */
#ifdef DEBUG	
			fprintf(stderr, "Process %d launched\n", cp);
#endif

			/* 
			 * After app starts, set the app's icon to
			 * insensitive, store pid in appTable and
	                 * turn on timeout timer.
			 */

			set_toggle_status(w, False);
			set_appTable_launcherInfo(w, cp);

		} else {

			/* fork failed */

			/*
			 * Yes, this switch currently falls through to
			 * the default case, I don't have specific messages
			 * for each errno.  If at some time in the future
			 * separate messages are desired this code is ready.
			 */

			switch (errno) {
			case EAGAIN:
			case ENOMEM:
			default:
				sprintf(msg, fork_failed_msg, app_data->a_appPath);
				break;
			}

			display_error(NULL, (char *)msg);
		}
	}
}

/* display immediately 
	XFlush(XtDisplayOfObject(confirm_dialog));
*/


