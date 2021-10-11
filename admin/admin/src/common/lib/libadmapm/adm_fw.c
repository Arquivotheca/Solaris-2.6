
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the general exported functions from the
 *	administative framework.  It contains:
 *
 *		adm_init()
 *		    Initialize the current process or requested method.
 *
 *		adm_msg_path()
 *
 *		    Return the default pathname where a method's message
 *		    localization files are located.
 *
 *		adm_set_local_dispatch_info()
 *
 *		    Specify information to be used in local dispatch mode.
 *		    Includes only the "print method PID" flag.
 *
 *		adm_get_local_dispatch_info()
 *
 *		    Retrieve information used in local dispatch mode.
 *		    Includes only the "print method PID" flag.
 *
 *******************************************************************************
 */

#ifndef _adm_fw_c
#define _adm_fw_c

#pragma	ident	"@(#)adm_fw.c	1.8	92/02/26 SMI"

#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

/*
 *--------------------------------------------------------------------
 *
 *  ADM_INIT():
 *
 *	Initialize the current process (application client or method).
 *
 *	Upon normal completion, this routine returns ADM_SUCCESS.
 *
 *	Note: This routine is not thread safe.  If multiple threads
 *	      concurrently attempt to initialize the process, unpredictable
 *	      results will occur.  A mutex lock is needed to gaurd
 *	      against concurrent access.
 *
 *--------------------------------------------------------------------
 */

int
adm_init()
{
	int stat;
	pid_t tpid;

	/*
	 * If this process has been forked from another one, then
	 * deallocate this process' copy of any rendezvous RPC allocated
	 * by the parent.
	 */

	tpid = getpid();
	if (is_rpc_allocated && (tpid != adm_pid)) {
		adm_amcl_cleanup(t_udp_sock, t_tcp_sock,
			      t_rendez_prog, (u_long) RENDEZ_VERS);
	}

	/*
	 * Initialize this process as a framework client.
	 */

	adm_pid = tpid;

	if (adm_inited) {
		return(ADM_SUCCESS);
	}

	if (atexit(adm_exit) != 0) {	/* Set up cleanup routine */
		return(ADM_ERR_EXITHAND);
	}

	stat = adm_msgs_init();		/* Init framework/object manager msgs */
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	stat = adm_env_init();		/* Init global framework variables */
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	stat = adm_args_init();		/* Initialize admin argument handler */
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	stat = adm_reqID_init();	/* Initialize request ID handler */
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	stat = adm_meth_init();		/* Init method message file pathname */
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	adm_inited = B_TRUE;

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_MSG_PATH( lpath ):
 *
 *	Return the full pathname of the default locale directory
 *	where a method can find its message localization files.
 *	A pointer to the pathname is returned in *lpath.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

int
adm_msg_path(char **lpath)
{
	int stat;

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		return(stat);
	}
	*lpath = adm_lpath;

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_SET_LOCAL_DISPATCH_INFO( amsl_debug, amsl_pid ):
 *
 *	This routine is used to specify information for controlling
 *	the AMSL when methods are dispatched locally.  amsl_pid should
 *	indicate whether or not the AMSL should print out information
 *	about an invoked method's PID:
 *
 *		TRUE  => AMSL should print PID of each invoked method.
 *
 *		FALSE => AMSL should not print a method's PID.
 *
 *	This routine always returns ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

int
adm_set_local_dispatch_info(boolean_t amsl_pid)
{
	adm_amsl_pid = amsl_pid;

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_GET_LOCAL_DISPATCH_INFO( amsl_debugp, amsl_pidp ):
 *
 *	This routine returns information about the behavior of the
 *	AMSL during locally dispatched method invocations.  It
 *	returns:
 *
 *	    *amsl_pidp		Boolean flag indicating whether or not
 *				the AMSL will print out method PID
 *				information:
 *
 *				    B_TRUE  => AMSL should print the PID
 *					       under which a method is running.
 *
 *				    B_FALSE => AMSL should not print a
 *					       method's PID.
 *
 *	The return value of this routine is always ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

int
adm_get_local_dispatch_info(boolean_t *amsl_pidp)
{
	if (amsl_pidp != NULL) {
		*amsl_pidp = adm_amsl_pid;
	}

	return(ADM_SUCCESS);
}

#endif /* !_adm_fw_c */

