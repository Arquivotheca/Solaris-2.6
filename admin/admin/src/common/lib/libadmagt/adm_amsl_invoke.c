/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)adm_amsl_invoke.c	1.10	92/02/28 SMI"

/*
 * FILE:  adm_amsl_invoke.c
 *
 *	Admin Framework class agent routine to invoke a method runfile.
 */

#include <stdio.h>
#include <string.h>
#include <stropts.h>
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netmgt/netmgt.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"

/* Typedef defined for "pointer to function returning void" */
typedef void (*PFV)();

/* Declare static functions */
static void invk_get_output(struct amsl_req *, struct pollfd *);
static void invk_get_error(struct amsl_req *, struct pollfd *);
static void invk_get_args(struct amsl_req *, struct pollfd *);
static void invk_put_error(struct pollfd *, struct Adm_error *);
static void invk_put_input(struct amsl_req *, struct pollfd *);
static void invk_read_pipe(struct amsl_req *, struct bufctl *, struct pollfd *);
static void invk_sig_exit();

/*
 * -----------------------------------------------------------------------
 * amsl_invoke - Routine to execute the invocation of a class method.
 *	Accepts pointer to the amsl request structure.
 *	This routine sets up and monitors pipes for STDIN, STDOUT, STDERR, 
 *	and STDFMT, builds an argv list and envp list for the method
 *	runfile, forks and execv's the method runfile, returns output
 *	arguments in the output argument list anchored in the request
 *	structure and returns error messages in the error structure.
 *	If a system error prevents full execution, an error status code is
 *	returned and the associated error message is in the error structure.
 *	Returns the exit status code of the method runfile in the request
 *	structure.
 * ----------------------------------------------------------------------
 */

int
amsl_invoke(
	struct amsl_req *reqp)	/* Pointer to amsl_req structure */
{
	struct pollfd Pfd[AMSL_NUM_PIPES * 2];	/* Poll structs for pipes */
	struct {		/* Arrays for processing pipe data */
		int pfdindx[AMSL_NUM_PIPES];	/* Pfd index for pipe */
		PFV pfdfunc[AMSL_NUM_PIPES];	/* Function to process pipe */
	} pollpipe;
	PFV    fx;		/* Pointer to function returning int */
	struct amsl_auth *ap;	/* Local pointer to amsl auth structure */
	int    ix;		/* Index into Pfd array for a pipe */
	int    brksw;		/* Loop break switch */
	int    stat;		/* Completion status code */
	int    tstat;		/* Temporary error status code */
	int    xstat;		/* Exit status code */
	pid_t  pid;		/* Forked child process ID */
	pid_t  reap_pid;	/* Reaped child process ID */
	pid_t  cpid;
	pid_t  ppid;
	pid_t  gpid;
	int    waittime;	/* Wait time for poll function */
	int    nfds;		/* Number of fd's with events from poll */
	int    status;		/* Child process status */
	char **argv;		/* Pointer to argv list */
	char **env;		/* Pointer to env list */
	int    i;		/* Temporary int variable */

	/* Initialize structure for processing pipe data after fork() */
	pollpipe.pfdindx[0] = AMSL_STDIN_WRITE;
	pollpipe.pfdfunc[0] = invk_put_input;
	pollpipe.pfdindx[1] = AMSL_STDOUT_READ;
	pollpipe.pfdfunc[1] = invk_get_output;
	pollpipe.pfdindx[2] = AMSL_STDERR_READ;
	pollpipe.pfdfunc[2] = invk_get_error;
	pollpipe.pfdindx[3] = AMSL_STDFMT_READ;
	pollpipe.pfdfunc[3] = invk_get_args;

	/* Initialize formatted output argument row switch */
	reqp->flags |= AMSL_REQ_NEWROW;		/* Force new row to start */

	/* Set up pipes for STDIN, STDOUT, STDERR, and STDARG */
	if ((stat = init_pipes(reqp, Pfd)) != 0)
		return (stat);

	/* Set up buffers for STDIN, STDOUT, and STDERR */
	if ((stat = init_buffs(reqp, Pfd)) != 0)
		return (stat);

	/*
	 * At this point, we are ready to fork a child process and exec
	 * the class method's runfile (whose pathname was set by the
	 * verify procedure).  We build the argv list and Admin system
	 * environment variables in the child process.  We monitor the
	 * pipes in the parent process, handling input via STDIN, output
	 * via STDOUT, and errors via STDERR.
	 * If we cannot fork the child process, shutdown the request and
	 * return an error (which causes an SNM error return RPC).
	 */

	/* Replace inherited signal handler for SIGCHLD with default */
	(void) signal(SIGCHLD, SIG_DFL);

	pid = fork();
	switch ((long)pid) {

	/*
	 * ***** ERROR FORKING *****
	 *
	 * Cleanup pipes and buffers; then return a FATAL error.
	 */

	case -1:
		(void) free_pipes(Pfd);
		(void) free_buffs(reqp);
		return (amsl_err(reqp, ADM_ERR_REQNOCHILD, errno));
		/*NOTREACHED*/
		break;

	/*
	 * ***** CHILD PROCESS *****
	 *
	 * Hook child's ends of the pipes to STDIN, STDOUT, and STDERR;
	 * and make STDFMT file descriptor available to parameter handling
	 * routines within the method runfile (via environment variable).
	 * Build the argv argument list from the input arguments and build
	 * the ADM_ environment variables.
	 *
	 * NOTE: Since we are in the child process, we can NOT simply put
	 *	 the error in an error struct, but must pass it back to
	 *	 the AMSL in the parent process.  We do this by writing
	 *	 the error to the STDFMT pipe, then exiting the child
	 *	 process with the ADM_FAILURE status code.
	 */

	case 0:

		/* Put method process in its own session group */
		(void) setsid();

		/* Set signal handler for SIGTERM and SIGQUIT */
		(void) signal(SIGTERM, invk_sig_exit);
		(void) signal(SIGQUIT, invk_sig_exit);

		ppid = getppid();
		cpid = getpid();
		gpid = getpgrp();
		ADM_DBG("i", ("Invoke: Method prologue: cpid=%ld, ppid=%ld, gpid=%ld",
		    (long)cpid, (long)ppid, (long)gpid));
		/*
		 * Blow away log info to prevent logging.  This assumes
		 * we have forked and are running in the method runfile
		 * process.  We do NOT want the class agent code in this
		 * process to generate logging messages.  Note that we
		 * do not blow away stdcats; we need to set an environment
		 * variable to this value.
		 */

		reqp->diag_reqcats = (char *)NULL;
		reqp->diag_errcats = (char *)NULL;
		reqp->diag_dbgcats = (char *)NULL;
		reqp->diag_infocats = (char *)NULL;

		/*
		 * Set child's end of pipes.  Turn off debugging now so
		 * debug messages don't show up on the stdout pipe.  If
		 * errors, log the message (can't send it back to the
		 * dispatch process since pipe may be broken) and exit.
		 */

		adm_debug_cats = ADM_DBG_NOCATS;
		if ((stat = set_pipes(reqp, Pfd, AMSL_CHILD)) != 0) {
			/* Cannot write this error to a pipe! */
			(void) amsl_err_syslog1(reqp->errp->message);
			exit(ADM_FAILURE);
		}

		/* Build runfile argv list; exit if errors */
		if ((stat = build_argv(reqp, reqp->inp, &argv)) != 0) {
			invk_put_error(&Pfd[AMSL_STDFMT_WRITE], reqp->errp);
			exit(ADM_FAILURE);
		}

		/* Build ADM environment variables; exit if errors */
		if ((stat = build_env(reqp, reqp->inp,
		    Pfd[AMSL_STDFMT_WRITE].fd, &env)) != 0) {
			invk_put_error(&Pfd[AMSL_STDFMT_WRITE], reqp->errp);
			exit(ADM_FAILURE);
		}

		/* Set the default umask for method process */
		(void) umask((mode_t)ADM_DEFAULT_UMASK);

		/* Change directories to the method's directory */
		if ((chdir(reqp->method_pathname)) == -1) {
			stat = amsl_err(reqp, ADM_ERR_CHLDCHDIR, errno,
			    reqp->method_pathname);
			invk_put_error(&Pfd[AMSL_STDFMT_WRITE], reqp->errp);
			exit(ADM_FAILURE);
		}

		/* If not running with security checking turned off */
		ap = reqp->authp;
		if (! (ap->auth_flag & AMSL_AUTH_OFF)) {

		 	/* change the real and effective group identity */
			if ((setgid(ap->auth_sid_gid)) == -1) {
				stat = amsl_err(reqp, ADM_ERR_CHLDSETGID,
				    errno, (long)(ap->auth_sid_gid));
				invk_put_error(&Pfd[AMSL_STDFMT_WRITE],
				    reqp->errp);
				exit(ADM_FAILURE);
			}

			/* Change the real and effective user identity */
			if ((setuid(ap->auth_sid_uid)) == -1) {
				stat = amsl_err(reqp, ADM_ERR_CHLDSETUID,
				    errno, (long)(ap->auth_sid_uid));
				invk_put_error(&Pfd[AMSL_STDFMT_WRITE],
				    reqp->errp);
				exit(ADM_FAILURE);
			}
		}

		/* Exec the runfile for the method with the argv list */
		(void) execv(reqp->method_filename, argv);

		/* If execv returns, error.  Send back error message */
		stat = amsl_err(reqp, ADM_ERR_CHLDBADEXECV, errno);
		invk_put_error(&Pfd[AMSL_STDFMT_WRITE], reqp->errp);
		exit(ADM_FAILURE);
		break;

	/*
	 * ***** PARENT PROCESS *****
	 *
	 * Set parent's ends of the pipes for writing unformatted input
	 * data to STDIN, and for receiving data from STDOUT, STDERR, and
	 * formatted arguments/errors from STDFMT.  Go into processing
	 * loop polling for events on these four pipes.  When all four
	 * pipes are closed, we drop out of the loop and reap the child
	 * process via the wait4 routine.  Note that any errors encountered
	 * while processing data from the pipes is NOT a fatal error, but
	 * results in a warning error being put on the local error stack.
	 * The child process exit code is reaped and returned in a
	 * warning error message to the client if non-zero.
	 * If we get a framework error waiting to reap the child process
	 * or building the return parameters, this is a fatal error and
	 * no result arguments are returned.  A fatal error message is
	 * returned to the client.
	 *
	 * NOTE:  We try to keep up and running even in the face of errors
	 *	  on the pipes.  An error results in that pipe being closed
	 *	  down, which might in turn cause the method runfile to
	 *	  write an error and exit with a non-zero status code.
	 */

	default:
		if (amsl_ctlp->flags & AMSL_DEBUG_CHILD)
			adm_fw_debug("Invoke: Class=%s  Method=%s  Pid=%ld",
			    reqp->class_name, reqp->method_name, (long)pid);
		else {
			ADM_DBG("ix", ("Invoke: Class=%s  Method=%s  Pid=%ld",
			    reqp->class_name, reqp->method_name, (long)pid));
		}

		/* Save method pid in request struct for forced shutdown */
		reqp->method_pid = pid;

		/* Initialize pipes for parent process */
		(void) set_pipes(reqp, Pfd, AMSL_PARENT);

		/*
		 * Loop until following conditions occur:
		 *	All pipes are closed (by EOF or errors)
		 *
		 * Note that we set the wait time for the poll() function
		 * to -1, effectively making this code event driven.  When
		 * any of the expected events occur, we should be notified
		 * and the poll() will return.  We then loop through the
		 * Pfd[] array looking for the event(s) and process them.
		 *
		 * If the poll function fails, we close all pipes and
		 * return an error containing the poll error code.
		 */

		waittime = -1;
		while (check_pipes(Pfd) > 0) {
			stat = 0;
			errno = 0;

			/* Wait for an event on one or more pipes */
			nfds = poll(Pfd, (AMSL_NUM_PIPES * 2), waittime);
			if (nfds < 0) {
				if ((errno == EAGAIN) || (errno == EINTR))
					continue;
				ADM_DBG("i", ("Invoke: error %d from poll",
				    errno));
				stat = amsl_err(reqp, ADM_ERR_REQPOLLCALL,
				    errno);
				for (i = 0; i < AMSL_NUM_PIPES; i++) {
					ix = pollpipe.pfdindx[i];
					(void) close_pipe(&Pfd[ix]);
				}
				break;
			}
			if (nfds == 0)
				continue;

		ADM_DBG("i", ("Invoke: poll IN=%d, OUT=%d, ERR=%d, FMT=%d",
			    Pfd[AMSL_STDIN_WRITE].revents,
			    Pfd[AMSL_STDOUT_READ].revents,
			    Pfd[AMSL_STDERR_READ].revents,
			    Pfd[AMSL_STDFMT_READ].revents));

			/* Process any input or output data on pipes */
			for (i = 0; i < AMSL_NUM_PIPES; i++) {
				nfds--;
				ix = pollpipe.pfdindx[i];
				fx = pollpipe.pfdfunc[i];
				if (Pfd[ix].revents & (POLLIN|POLLOUT))
					(*fx)(reqp, &Pfd[ix]);
				else if (Pfd[ix].revents & POLLHUP)
					(void) close_pipe(&Pfd[ix]);
				else if (Pfd[ix].revents & (POLLNVAL|POLLERR)) {
					(void) amsl_err(reqp, ADM_ERR_REQPOLLPIPE, ix);
					(void) close_pipe(&Pfd[ix]);
				}
			}		/* End of for loop */
		}			/* End of infinite while loop */

		ADM_DBG("i", ("Invoke: All pipes closed"));

		/*
		 * Wait for child process to terminate.  We may have
		 * gotten an error from polling loop, which will be
		 * reflected in the "stat" variable.
		 */

		brksw = 1;
		while (brksw) {
			errno = 0;
			reap_pid = waitpid(pid, &status, (int)WUNTRACED);
			if ((long)reap_pid != -1) {
				if (WIFSTOPPED(status)) {
					tstat = WSTOPSIG(status);
		ADM_DBG("i", ("Invoke: method processs stopped, code %d", tstat));
					(void) amsl_err(reqp, ADM_ERR_REQCHLDSTOPPED,
					    tstat);
					reqp->errp->cleanup = ADM_FAILDIRTY;
					stat = ADM_FAILURE;
					xstat = ADM_FAILURE;
					brksw = 0;
				} else if (WIFSIGNALED(status)) {
					tstat = WTERMSIG(status);
		ADM_DBG("i", ("Invoke: method process stopped, signal %d", tstat));
					(void) amsl_err(reqp, ADM_ERR_REQCHLDSIGNALED,
					    tstat);
					reqp->errp->cleanup = ADM_FAILDIRTY;
					stat = ADM_FAILURE;
					xstat = ADM_FAILURE;
					brksw = 0;
				} else {
					xstat = WEXITSTATUS(status);
		ADM_DBG("i", ("Invoke: method process exited, status %d", xstat));
					brksw = 0;
					break;		/* Leave while */
				}
			} else if (errno != EINTR) {
				(void) amsl_err(reqp, ADM_ERR_REQBADWAIT, errno);
				stat = ADM_FAILURE;
				xstat = ADM_FAILURE;
				brksw = 0;
			}
		}					/* End of while */


		/*
		 * At this point, the child process has terminated.
		 * We detected such via all pipes having been closed.
		 * Note that we may have accumulated an error along
		 * the way, including a non-fatal framework error.
		 *
		 * We have the following status info:
		 *	stat		    => Completion status code
		 *	xstat		    => Exit status code
		 *	reqp->errp->code    => Error code
		 *	reqp->errp->cleanup => Cleanup status
		 *
		 * Clear the method pid in the request structure and
		 * set the exit status code in the request structure.
		 */

		reqp->method_pid = (uid_t)0;
		reqp->exit_status = xstat;

		/* Cleanup pipes (leave buffers for caller to clean up) */
		(void) free_pipes(Pfd);

		/* Return completion status code */
		return (stat);
		/*NOTREACHED*/
		break;				/* End of parent code */
	}					/* End of switch on fork */

	/*NOTREACHED*/
}

/*
 * -----------------------------------------------------------------------
 * invk_put_input - Routine to pass unformatted input to method runfile
 *	by writing it on STDIN pipe.
 *	Accepts pointer to an amsl_req structure and the pollfd structure
 *	for the write end of the STDIN pipe to the method runfile.
 *	If a system error occurs, the associated error message is put on
 *	top of the error stack pointed to by the amsl_req structure and the
 *	STDIN pipe is closed.  Method runfile processing continues.
 *
 *	!!! WARNING !!!
 *
 *	This routine should NEVER block writing to the pipe.  To this end,
 *	make sure AMSL_STDIN_MAXWRITE never exceeds the system buffer
 *	size.
 * ----------------------------------------------------------------------
 */

static void
invk_put_input(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct pollfd *pfdp)	/* Pointer to pollfd structure for pipe */
{
	struct bufctl *bp;	/* Local pointer to current input buffer */
	int   nbytes;		/* Number of bytes written */
	int   nwrite;		/* Number of bytes to write this time */

	bp = reqp->inbuff;

	/* If pipe has been closed, just return */
	if (pfdp->fd == -1)
		return;

	/* Determine how many bytes to write this time */
	if ((nwrite = bp->left) <= 0) {
		(void) close_pipe(pfdp);
		return;
	}
	if (nwrite > AMSL_STDIN_MAXWRITE)
		nwrite = AMSL_STDIN_MAXWRITE;

	ADM_DBG("i", ("Invoke: Writing %d bytes to method's stdin...", nwrite));

	/* Write to pipe.  If fatal error, close pipe & mark buffer done */
	if ((nbytes = write(pfdp->fd, bp->currp, nwrite)) < 0)
		if (errno == EAGAIN)
			nbytes = 0;
		else {
			(void) amsl_err(reqp, ADM_ERR_REQWRITESTDIN, errno);
			(void) close_pipe(pfdp);
			nbytes = bp -> left;
		}

	/* Adjust buffer counts and pointers */
	bp->currp += nbytes;
	bp->left  -= nbytes;

	/* Return */
	return;
}

/*
 * -----------------------------------------------------------------------
 * invk_get_output - Routine to retrieve unformatted output from the method
 *	runfile by reading the STDOUT pipe.
 *	Accepts pointer to an amsl_req structure and the pollfd structure
 *	for the read end of the STDOUT pipe to the method runfile.
 *	If a system error occurs, the associated error message is put on
 *	top of the error stack pointed to by the amsl_req structure and the
 *	STDOUT pipe is closed.  Method runfile processing continues.
 * ----------------------------------------------------------------------
 */

static void
invk_get_output(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct pollfd *pfdp)	/* Pointer to pollfd structure for pipe */
{
	/* If pipe closed, shouldn't be here */
	if (pfdp->fd == -1)
		return;

	ADM_DBG("i", ("Invoke: reading from stdout..."));

	/* Get data off the pipe */
	invk_read_pipe(reqp, reqp->outbuff, pfdp);

	/* Return without status */
	return;
}

/*
 * -----------------------------------------------------------------------
 * invk_get_error - Routine to retrieve unformatted errors from the method
 *	runfile by reading the STDERR pipe.
 *	Accepts pointer to an amsl_req structure and the pollfd structure
 *	for the read end of the STDERR pipe to the method runfile.
 *	If a system error occurs, the associated error message is put on
 *	top of the error stack pointed to by the amsl_req structure and the
 *	STDERR pipe is closed.  Method runfile processing continues.
 * ----------------------------------------------------------------------
 */

static void
invk_get_error(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct pollfd *pfdp)	/* Pointer to pollfd structure for pipe */
{
	/* If pipe closed, shouldn't be here */
	if (pfdp->fd == -1)
		return;

	ADM_DBG("i", ("Invoke: reading from stderr..."));

	/* Get data off the pipe */
	invk_read_pipe(reqp, reqp->errbuff, pfdp);

	/* Return without status */
	return;
}

/*
 * -----------------------------------------------------------------------
 * invk_get_args - Routine to retrieve formatted output arguments from the
 *	method runfile by reading the STDARG pipe.
 *	Accepts pointer to an amsl_req structure and the pollfd structure
 *	for the read end of the STDARG pipe to the method runfile.
 *	If a system error occurs, the associated error message is at the
 *	top of the error stack pointed to by the amsl_req structure and the
 *	STDARG pipe is closed.  Method runfile processing continues.
 * ----------------------------------------------------------------------
 */

static void
invk_get_args(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct pollfd *pfdp)	/* Pointer to pollfd structure for pipe */
{
	Adm_data *op;		/* Local pointer to Admin output handle */
	struct bufctl *bp;	/* Local pointer to data buffer control */
	char *sp;		/* Pointer to current string in buffer */
	char *xp;		/* Temporary string pointer */
	char *tp;		/* Pointer to current character in string */
	char  namebuf[ADM_MAX_NAMESIZE + 1];	/* Local arg name buffer */
	int   len;		/* Formatted string length */
	u_int type;		/* Arg type */
	u_int length;		/* Arg length */
	int   code;		/* Error code */
	time_t msgtime;		/* Diag time value */
	char *catsp;		/* Pointer to diag categories */
	char *valuep;		/* Pointer to arg value */
	char  cp;		/* First character in formatted string */

	/* If pipe closed, shouldn't be here */
	if (pfdp->fd == -1)
		return;

	/* Set local control structure pointers */
	bp = reqp->fmtbuff;
	op = reqp->outp;

	ADM_DBG("i", ("Invoke: reading from stdfmt..."));

	/* Get data off the pipe */
	invk_read_pipe(reqp, bp, pfdp);
	if (bp->left == bp->size)
		return;

	/*
	 * Got at least one formatted string and maybe several.
	 * Loop through buffer processing formatted strings.  Each string
	 * can be one of three types, determined by the first character in
	 * the string (after the length prefix), as follows:
	 *
	 * *ADM_ARGMARKER   (+) => formatted output argument
	 * *ADM_ERRMARKER   (-) => formatted error message
	 * *ADM_ROWMARKER   (@) => end of row marker
	 * *ADM_DIAGMARKER  (#) => formatted diagnostic message
	 * *ADM_CLEANMARKER (=) => formatted method cleanup indicator
	 *
	 * Each type of string is handled differently...
	 *
	 * output arg => parse into Adm_arg structure and append to
	 *		 list of output arguments.  If new row required,
	 *		 call routine to create next row structure.
	 * error msg  => simply put into the formatted error structure
	 * row marker => end of current row of arguments, reset new row
	 *		 indicator
	 * cleanup    => move cleanup code into formatted error structure
	 * diag msg   => write message to diagnostic log file
	 *
	 * Note that each string begins with its length in ASCII format.
	 */

	*(bp->currp) = '\0';			/* Make string safe */
	sp = bp->startp;			/* Always begin at start */
	while (sp < bp->currp) {
		tp = sp;
		len = 0;
		while ((isdigit(*tp)) && (tp < bp->currp)) {
			len = len*10 + (*tp - '0');
			tp++;
		}

		/* Check for invalid length or partial arg in buffer */
		if (len == 0) {
			(void) amsl_err(reqp, ADM_ERR_BADOUTPUTLEN, len);
			(void) close_pipe(pfdp);
			break;
		}
		if (len > (bp->currp - tp))
			break;

		/* Process formatted argument depending on type */
		xp = tp;
		cp = *tp;
		if (cp == *ADM_ARGMARKER) {		/* Output arg */
			if (reqp->flags & AMSL_REQ_NEWROW)
				adm_args_insr(op);
			reqp->flags &= ~(AMSL_REQ_NEWROW);
			if ((adm_args_str2a(&xp, namebuf, &type, &length,
			    &valuep)) == 0) {
				xp--;
				*xp = '\0';
				ADM_DBG("i", ("Invoke: Method output: %s(%d)=%s",
				    namebuf, length,
				    (valuep != NULL?valuep:"(nil)")));
				*xp = '\n';
				xp++;
				adm_args_puta(op, namebuf, type, length,
				    valuep);
			} else {
				(void) amsl_err(reqp, ADM_ERR_BADOUTPUTARG, tp);
			}
			goto label1;

		} else if (cp == *ADM_ERRMARKER) {	/* Error */
			if ((adm_err_str2err(xp, len, &code, &type,
			    &valuep)) == 0) {
				ADM_DBG("i", ("Invoke: Method error: %s", tp));
				adm_err_fmt(reqp->errp, code, type,
				    ADM_FAILCLEAN, valuep);
				(void) amsl_log1(reqp->diag_errcats, valuep);
				if (valuep != (char *)NULL)
					(void) free(valuep);
			} else {
				(void) amsl_err(reqp, ADM_ERR_BADOUTPUTARG, tp);
			}
			goto label1;

		} else if (cp == *ADM_DIAGMARKER) {	/* Diagnostic */
			if ((adm_diag_str2msg(xp, len, &msgtime, &catsp,
			    &valuep, &length, &length)) == 0) {
				ADM_DBG("i", ("Invoke: Method diagnostic: %s", tp));
				adm_diag_fmt(&code, ADM_LOG_STDHDR, msgtime,
				    catsp, valuep);
				if (catsp != (char *)NULL)
					(void) free(catsp);
				if (valuep != (char *)NULL)
					(void) free(valuep);
			} else {
				(void) amsl_err(reqp, ADM_ERR_BADOUTPUTARG, tp);
			}
			goto label1;

		} else if (cp == *ADM_ROWMARKER) {	/* End of row */
			ADM_DBG("i", ("Invoke: Method output: (End of row)"));
			if (reqp->flags & AMSL_REQ_NEWROW)
				adm_args_insr(op);
			reqp->flags |= AMSL_REQ_NEWROW;
			goto label1;

		} else if (cp == *ADM_CLEANMARKER) {	/* Cleanup */
			if ((adm_err_str2cln(xp, len, &type)) == 0) {
				ADM_DBG("i", ("Invoke: Method cleanup: %d", type));
				reqp->errp->cleanup = type;
			} else {
				(void) amsl_err(reqp, ADM_ERR_BADOUTPUTARG, tp);
			}
			goto label1;

		} else {				/* Bad string */
			(void) amsl_err(reqp, ADM_ERR_BADOUTPUTARG, tp);
			ADM_DBG("i", ("Invoke: Bad formatted string: %.32s",
			    tp));
			goto label1;
		}
label1:
		sp = tp + len;
	}						/* End of while */

	/* Check if any residual data left in input buffer */
	if ((len = bp->currp - sp) > 0) {
		if (sp != bp->startp) {
			(void) memcpy(bp->startp, sp, (size_t)len);
			bp->currp = bp->startp + len;
			bp->left = bp->size - len;
		}
	} else {
		bp->currp = bp->startp;
		bp->left = bp->size;
	}
	*(bp->currp) = '\0';				/* Make string safe */

	/* Return without status */
	return;
}

/*
 * -----------------------------------------------------------------------
 * invk_read_pipe - Routine to retrieve data from the read end of a pipe.
 *	Accepts pointer to the amsl request structure, a pointer to a
 *	buffer control structure, and the pollfd structure for the read end
 *	of the pipe to the method runfile.
 *	If a system error occurs, the associated error message is put on
 *	top of the error stack pointed to by the amsl_req structure and the
 *	STDERR pipe is closed.  Method runfile processing continues.
 *
 *	!!! WARNING !!!
 *
 *	This routine should NEVER block reading from the pipe.  To this end,
 *	the pipe end is fnctl'ed with the FNDELAY flag (no delay).
 * ----------------------------------------------------------------------
 */

static void
invk_read_pipe (
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct bufctl *bp,	/* Pointer to buffer control structure */
	struct pollfd *pfdp)	/* Pointer to pollfd structure for pipe */
{
	int   nbytes;		/* Number of bytes to be read */
	int   nread;		/* Number of bytes actually read */

	/* Loop until all bytes read off pipe */
	nread = 1;
	while (nread > 0) {
		if (bp->left <= 1)
			if (grow_buff(bp) != 0) {
				(void) amsl_err(reqp, ADM_ERR_REQNOBUFFER, -1);
				(void) close_pipe(pfdp);
				break;
			}
		nbytes = bp->left - 1;
		nread = read(pfdp->fd, bp->currp, nbytes);
		ADM_DBG("i", ("Invoke: Read %d bytes from pipe", nread));
		if (nread > 0) {
			*(bp->currp + nread) = '\0';
			bp->currp += nread;
			bp->left  -= nread;
			if (nread < nbytes)
				break;

		} else if (nread == 0) {	/* End of file on pipe */
			(void) close_pipe(pfdp);
			break;

		} else if ((errno != EINTR) && (errno != EAGAIN)) {
			(void) amsl_err(reqp, ADM_ERR_PIPEREADERR, errno, 2);
			(void) close_pipe(pfdp);
			break;

		} else				/* Interrupted read */
			break;
	}					/* End of while loop */

	/* Return without status */
	return;
}

/*
 * -----------------------------------------------------------------------
 * invk_put_error - Routine to pass framework error from the child process
 *	back to parent process by writing a formatted error on STDFMT pipe.
 *	Accepts pointer to the pollfd structure for the write end
 *	of the STDFMT pipe and a pointer to an Adm_error structure.
 *	This routine uses adm_err_err2str to create the formatted error
 *	message, then writes the formatted error to the STDFMT pipe.
 * ----------------------------------------------------------------------
 */

static void
invk_put_error(
	struct pollfd *pfdp,	/* Pointer to pollfd structure for pipe */
	struct Adm_error *ep)	/* Pointer to Adm error structure */
{
	FILE  *fp;		/* Local file pointer */

	/* If pipe has been closed, just return */
	if (pfdp->fd == -1)
		return;

	/* Open a user level file pointer for the stdfmt pipe */
	if ((fp = fdopen(pfdp->fd, "w")) == (FILE *)NULL)
		return;

	/* Format the error; forget it if we get an error */
	adm_err_set2(fp, (char *)NULL, ep->code, ADM_ERR_SYSTEM, ep->message);

	/* Close the stdfmt pipe and its file pointer */
	(void) fclose(fp);
	pfdp->fd = -1;

	/* Return */
	return;
}

/*
 * -----------------------------------------------------------------------
 * invk_sig_exit - Signal handling routine for method process.
 *	If running method process class agent prologue code and catch
 *	a SIGTERM or SIGQUIT, must be agent shutdown sequence.  Since
 *	no method code is running yet, just exit this process.
 * ----------------------------------------------------------------------
 */

static void
invk_sig_exit()
{

	/* Must be in method process agent prologue code; just exit */
	exit(ADM_FAILURE);
}
