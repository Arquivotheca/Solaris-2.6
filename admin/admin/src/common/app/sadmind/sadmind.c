/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.
 */

#pragma ident	"@(#)sadmind.c	1.28	95/06/01 SMI"

/*
 * FILE:  admind.c
 *
 *	Admin Framework class agent main procedure for starting up the SNM
 *	based class agent program.  Also includes the SNM agent reap and
 *	shutdown procedures.  The amsl program can be called with the
 *	following command line arguments:
 *
 *		-c <categories>	  => Specify log file filter categories
 *		-l <logfile>	  => Specify log file pathname
 *              -i <idletime>     => Specify idle time before termination (sec)
 *	  	-n		  => Do NOT cache in Object Manager
 *		-O <pathname>	  => Open Windows pathname
 *		-r <prog> <vers>  => Specify RPC program and version numbers
 *		-S <security>	  => Specify security level
 *		-t <timeout>	  => Specify RPC request timeout in seconds
 *		-v		  => Write request msg's to system console
 *
 *	Unpublished (hidden) command line options:
 *
 *		-D <categories>	  => Specify debug message categories
 *
 *	Exits with error code 1 if command line options are invalid or
 *	if unable to initialize class agent program.
 *
 *	NOTE:  The class agent registers with the RPC system at a well-known
 *		RPC number currently defined in the adm_amsl.h header file,
 *		unless overridden with the "-r" option.
 *
 */
#include <thread.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <netmgt/netmgt.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"

/* Declare static functions */
static void amsl_reap(int, pid_t, int *, caddr_t);
static void amsl_shutdown(int);

/*
 * -------------------------------------------------------------------
 *  main - class agent main procedure
 *	Command line (optional) options include:
 *		-c <logfile_categories>
 *              -i <idletime>     => Specify idle time before termination (sec)
 *		-l <logfile_pathname>
 *		-n
 *		-O <OW_pathname>
 *		-r <Server_RPC_Program_Number> [<Server_RPC_version_number>]
 *		-S <Security-level>
 *		-t <RPC_timeout_in_seconds>
 *		-v
 *
 *	Undocumented options:
 *		-D <categories>
 *
 *	Returns status code 1 if invalid options or unable to initialize.
 * -------------------------------------------------------------------
 */

int
main(int argc, char **argv)
{
	char  msgbuf[ADM_MAXERRMSG*2];	/* Message buffer for errors */
	char *msgfmt;		/* Message format for errors */
	register c;		/* argv option letter */
	int   errflag;		/* argv error flag */
	char *optname;		/* argv option name */
	char *optarg;		/* argv option argument */
	u_long rpcnum;		/* Class agent RPC program number */
	u_long rpcver;		/* Class agent RPC version number */
	u_int sec_level;	/* Security level */
	char *debug_cats;	/* Pointer to debug catagories */
	int   flags;		/* options flag values */
        int idletime;         /* idle time before termination (sec) */
	time_t timeout;		/* RPC timeout */
	struct timeval timeout_s; /* RPC timeout structure for SNM */
	char *logfile;		/* Local logfile pathname */
	char *logcats;		/* Local logfile categories */
	char *owpath;		/* OW pathname argument */
	int   nmcode;		/* Netmgt status code */
	char *nmmsgp;		/* Netmgt message pointer */
	long  tlong;		/* Temporary long integer */
	char *tp;		/* Temporary string pointer */

	/* Initialize for internationalized error messages */
	adm_msgs_init();

	/* Initialize default command line options */
	flags = 0;
	rpcnum = (u_long) ADM_CLASS_AGENT_PROG;
	rpcver = (u_long) ADM_CLASS_AGENT_VERS;
	sec_level = (u_int) ADM_DEFAULT_SECURITY_LEVEL;
	debug_cats = ADM_DBG_NOCATS;
	timeout = ADM_AMSL_TIMEOUT;
        idletime = ADM_AMSL_IDLETIME;
	logfile = (char *)NULL;
	logcats = (char *)NULL;
	owpath = (char *) NULL;
	/* Process command line options	*/
	argc--;
	argv++;
	errflag = 0;
	while ((errflag == 0) && (*argv != (char *)NULL)) {
		optname = *argv;
		if (((*argv)[0] == '-') && ((*argv)[2] == '\0')) {
			c = (*argv)[1];
			argv++;
			optarg = *argv;
			switch (c) {
			case 'c':		/* Log file categories */
				if ((optarg != (char *)NULL) &&
				    (*optarg != '-')) {
					logcats = optarg;
					argv++;
				}
				break;
			case 'D':		/* Debug categories */
				if ((optarg != (char *)NULL) &&
				    (*optarg != '-')) {
					debug_cats = optarg;
					argv++;
				} else
					debug_cats = ADM_DBG_ALLCATS;
				flags |= AMSL_DEBUG_ON;
				break;
			case 'h':		/* Help option */
					msgfmt = adm_err_msg(ADM_ERR_AMSL_USAGE);
					(void) sprintf (msgbuf, msgfmt,
					    ADM_CLASS_AGENT_NAME);
					(void) printf(msgbuf);
					exit (1);
				break;
                        case 'i':               /* Idle time limit */
                                if ((optarg != (char *)NULL) &&
                                    (*optarg != '-')) {
                                        idletime = (int)atoi(optarg);
                                        argv++;
                                }
                                if (idletime > ADM_AMSL_MAXIDLETIME)
                                        idletime = 0;
                                if (idletime < 0)
                                        errflag++;
                                break;
			case 'l':		/* Log file pathname */
				flags |= AMSL_DIAGLOG_ON;
				if ((optarg != (char *)NULL) &&
				    (*optarg != '-')) {
					logfile = optarg;
					argv++;
				} else
					logfile = ADM_LOG_FILENAME;
				break;
			case 'n':		/* Turn off OM caching */
				flags |= AMSL_CACHE_OFF;
				break;
			case 'O':		/* Open Windows pathname */
				if ((optarg != (char *)NULL) &&
				    (*optarg != '-')) {
					owpath = optarg;
					argv++;
				} else
					errflag++;
				break;
			case 'r':		/* RPC program number */
				if ((optarg != (char *)NULL) &&
				    (*optarg != '-')) {
					rpcnum = (u_long)atol(optarg);
					argv++;
				}
				if (rpcnum == 0)
					errflag++;
				optarg = *argv;
				if ((optarg != (char *)NULL) &&
				    (*optarg != '-')) {
					rpcver = (u_long)atol(optarg);
					argv++;
				} else
					rpcver = 1;
				if (rpcver == 0)
					errflag++;
				break;
			case 'S':		/* Security level */
				tlong = -1;
				if ((optarg != (char *)NULL) &&
				    (*optarg != '-')) {
					tlong = strtol(optarg, &tp, 10);
					if (tp == optarg)
						errflag++;
					argv++;
				}
				if ((tlong >= 0) && 
				    (tlong <= ADM_MAXIMUM_SECURITY_LEVEL))
					sec_level = (u_int)tlong;
				else
					errflag++;
				break;
			case 't':		/* RPC timeout value */
				timeout = (time_t)0;
				if ((optarg != (char *)NULL) &&
				    (*optarg != '-')) {
					timeout = (time_t)atol(optarg);
					argv++;
				}
				if (timeout <= 0)
					errflag++;
				break;
			case 'v':		/* Request msgs to syslog */
				flags |= AMSL_CONSLOG_ON;
				break;
			default:			/* Bad option */
				errflag++;
				break;
			}				/* End of switch */
		} else {
			errflag++;			/* Bad option */
			argv++;
		}
	}						/* End of while */

	if (errflag != 0) {
		msgfmt = adm_err_msg(ADM_ERR_AMSL_BADCMD);
		(void) sprintf (msgbuf, "%s: %s \"%s\"\n", ADM_CLASS_AGENT_NAME,
		    msgfmt, optname);
		(void) printf(msgbuf);
		(void) amsl_err_syslog1(msgbuf);
		exit(1);
	}

	/* XXX - PATCH:  TURN OFF CACHING FOR THIS RELEASE */
/*
 *	flags |= AMSL_CACHE_OFF;
 */
	/* XXX - END OF PATCH */

	/* Set debugging categories and make sure daemon does not fork */
	if (debug_cats != ADM_DBG_NOCATS) {
		adm_debug_cats = strdup(debug_cats);
		netmgt_set_debug((u_int)1);
	} else
		adm_debug_cats = ADM_DBG_NOCATS;

	/* Initialize amsl internal structures, options, flags, etc. */
	if (init_amsl(timeout, sec_level, logfile, logcats, owpath, flags)
	    != 0) {
		(void) free_amsl(&amsl_ctlp);
		exit(1);
	}


	/* OK to start up as SNM server; amsl_ctlp now set */
	timeout_s.tv_sec = timeout;
	timeout_s.tv_usec = 0;
	if (!netmgt_init_rpc_agent(ADM_CLASS_AGENT_NAME,
	    (u_int)  ADM_CLASS_AGENT_SNUM,
	    rpcnum,
	    rpcver,
	    (u_long) IPPROTO_UDP,
	    timeout_s,
            idletime,
	    amsl_ctlp->startup_flags,
	    amsl_verify,
	    amsl_dispatch,
	    amsl_reap,
	    amsl_shutdown)) {
		amsl_err_netmgt(&nmcode, &nmmsgp);
		msgfmt = adm_err_msg(ADM_ERR_AMSL_INIT);
		(void) printf("%s: ", ADM_CLASS_AGENT_NAME);
		(void) printf(msgfmt, nmcode);
		(void) printf("\n");
		(void) printf("%s: %s\n", ADM_CLASS_AGENT_NAME, nmmsgp);
		(void) amsl_err_syslog(ADM_ERR_AMSL_INIT,
		    nmcode);
		(void) amsl_err_syslog1(nmmsgp);
		ADM_DBG("e", ("AMSL:   Error %d from netmgt_init_rpc_request",
		    nmcode));
		(void) free_amsl(&amsl_ctlp);
		exit(1);
	}


	/* If logging, write start up message to log file */
	(void) amsl_log3(ADM_ERR_AMSL_STARTUP, (long)getpid());

	/* OK, let 'er rip! */
	netmgt_start_agent();

	/* abnormal return */
	(void) amsl_err_syslog(ADM_ERR_AMSL_ABNORMAL);
	ADM_DBG("e", ("AMSL:   Abnormal return from netmgt_start_agent"));
	return (1);
}

/*
 * ---------------------------------------------------------------------
 *  amsl_reap - SIGCHLD signal handler
 *	called by agent library SIGCHLD handler after handler called
 *	wait(3) to reap the child process
 *	no return value
 *
 *	NOTE:  We are reaping the class agent dispatch process, NOT the
 *		class method executable runfile child process.
 * ---------------------------------------------------------------------
 */

/*ARGSUSED*/

static void
amsl_reap(
	int sig,		/* signal number caught */
	pid_t child_pid,	/* child's pid */
	int *status,		/* child return status */
	caddr_t rusage)		/* child resource usage */
{
	struct amsl_req *rp;	/* Local pointer to request structure */
	u_int reqtag;		/* Local request sequence tag */

	if (sig == (int)SIGCLD) {
		ADM_DBG("i", ("Reap:   dispatch process reaped; pid %ld",
		    (long)child_pid));
	} else {
		ADM_DBG("d", ("Reap:   Dispatch process %ld terminated, signal %d",
		    (long)child_pid, sig));
	}

	/* Get the request sequence tag for child dispatch process */
	reqtag = _netmgt_get_child_sequence();

	/* Find, unlink and free request structure */
	rp = find_req(reqtag);
	unlink_req(rp);
	free_req(rp);

	/* Return without status */
	return;
}

/* -----------------------------------------------------------------
 *  amsl_shutdown - agent shutdown procedure
 *	Release control and request structure memory and clear global
 *	pointers.
 *	No return
 * -----------------------------------------------------------------
 */
static void
amsl_shutdown(
	int sig)		/* signal caught */
{
	struct amsl_req *rp;	/* Local pointer to a request struct */
	pid_t pid;		/* Local process id variable */
	time_t starttime;	/* Time shutdown wait period started */
	time_t currtime;	/* Current time */
	int   killsignal;	/* Signal to kill method processes */
	u_int waittime;		/* Total wait time before self destruct */
	u_int waitincr;		/* Time increment to check shutdown */
	u_int count;		/* Number of active method processes */

	/* if logging, indicate that the agent is shutting down */
	pid = getpid();
	(void) amsl_log3(ADM_ERR_AMSL_SHUTDOWN, (long)pid, sig);
	ADM_DBG("d", ("Shtdwn: Class agent %ld shutting down, signal %d",
	    (long)pid, sig));

	/*
	 * This shutdown routine is invoked when the following signals
	 * are caught by the SNM Agent Services library dispatcher:
	 *
	 *	SIGINT	=> ctrl-C in debug mode; graceful shutdown
	 *	SIGTERM	=> "kill" agent; graceful shutdown
	 *	SIGQUIT	=> force kill agent; forced shutdown
	 *
	 * The following actions are taken for agent shutdown:
	 *
	 *	If SIGINT or SIGTERM,
	 *		set graceful shutdown wait time
	 *		set to kill method dispatch processes with SIGTERM
	 *	If SIGQUIT,
	 *		set forced shutdown wait time
	 *		set to kill method dispatch processes with SIGQUIT
	 *	Loop through request structures and kill dispatch processes
	 *	Sleep for the wait time or until all methods cleanup
	 *	If forced shutdown,
	 *		SIGKILL any remaining method dispatch processes
	 *	Terminate the class agent (just return this routine)
	 *
	 * NOTE	Each method has a dispatch process which in turn forks
	 *	the method runfile process.  We are killing the dispatch
	 *	process, which in turn catches our signals and kills
	 *	its child runfile process.  (See the amsl_dispatch routine)
	 */

	if (sig == SIGQUIT) {
		killsignal = SIGQUIT;
		waittime = AMSL_SHUTDOWN_FORCE_TIME;
	} else {
		killsignal = SIGTERM;
		waittime = AMSL_SHUTDOWN_GRACE_TIME;
	}
	waitincr = AMSL_SHUTDOWN_CHECK_TIME;
	if (waitincr > waittime)
		waitincr = waittime;

	/* Loop through requests and signal any live dispatch process */
	starttime = time((time_t)0);
	count = 0;
	rp = amsl_ctlp->firstreq;
	while (rp != (struct amsl_req *)NULL) {
		if ((pid = _netmgt_get_child_pid(rp->request_tag))
		    > (pid_t)0) {
			count++;
			(void) sigsend((idtype_t)P_PID, pid, killsignal);
		}
		rp = rp->nextreq;
	}						/* End of while */
	if (count != 0)
		ADM_DBG("d", ("Shtdwn: terminated %d dispatched methods",
		    count));
	
	/* While any dispatch process still active, wait til time expires */
	while ((count != 0) && (waittime != 0)) {
		count = 0;
		rp = amsl_ctlp->firstreq;
		while (rp != (struct amsl_req *)NULL) {
			if ((pid = _netmgt_get_child_pid(rp->request_tag))
			    > (pid_t)0)
				count++;
			rp = rp->nextreq;
		}
		if (count == 0)
			break;
		(void) sleep (waitincr);
		currtime = time((time_t)0);
		if ((currtime - starttime) >= waittime)
			waittime = 0;
	}						/* End of while */

	/* Grace period is over.  If forced, kill active dispatch procs */
	if ((sig == SIGQUIT) && (count != 0)) {
		count = 0;
		rp = amsl_ctlp->firstreq;
		while (rp != (struct amsl_req *)NULL) {
			if ((pid = _netmgt_get_child_pid(rp->request_tag))
			    > (pid_t)0) {
				count++;
				(void) sigsend((idtype_t)P_PID, pid, SIGKILL);
			}
			rp = rp->nextreq;
		}					/* End of while */
		if (count != 0)
			ADM_DBG("d", ("Shtdwn: killed %d dispatched methods",
			    count));
	}

	/* Release any remaining request structures in linked list */
	while ((rp = amsl_ctlp->firstreq) != (struct amsl_req *)NULL) {
		amsl_ctlp->firstreq = rp->nextreq;
		free_req(rp);
	}

	/* Release control structure and clear global pointer */
	ADM_DBG("d", ("Shtdwn: class agent going down..."));
	free_amsl(&amsl_ctlp);

	/* shutdown agent (no return) */
	netmgt_shutdown_agent(sig);

	/*NOTREACHED*/
	return;
}
