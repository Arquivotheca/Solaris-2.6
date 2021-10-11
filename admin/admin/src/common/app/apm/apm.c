
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 **************************************************************************
 *
 *	This file contains the command line interface for invoking
 *	administrative framework methods.  The syntax for invoking a
 *	method is:
 *
 *	    {apm | tpm}  -c class [version]  -m method  [-h host]  [-d domain]
 *			 [-t AuthType]  [-f AuthFlavor]  [-g ClientGroup]
 *			 [-n | -p]  [-x AckSecs AckUsecs]  [-y RepSecs RepUsecs]
 *			 [-k PingCnt]  [-w PingSecs PingUsecs]
 *			 [-i DelaySecs DelayUsecs]  [-r ServerProg ServerVers]
 *			 [-u [file]]  [-D DebugCategories]  [-l]
 *			 [-a name[=value] name[=value] ...]
 *
 *	The caller specifies the name of the class and method to invoke
 *	(with optional class version number), along with the following
 *	optional specifications:
 *
 *	    -h host
 *	    -host host
 *
 *		Name of host on which to invoke the method.  Default is the
 *		local host.
 *
 *	    -d domain
 *	    -domain domain
 *
 *		Domain in which to invoke the method.
 *
 *	    -g ClientGroup
 *	    -client_group ClientGroup
 *
 *		Set the group name under which the client wishes to
 *		have the method exec'd.
 *
 *	    -t AuthType
 *	    -auth_type  AuthType
 *
 *		Type of authentication to use for method request.
 *
 *	    -f AuthFlavor
 *	    -auth_flavor  AuthFlavor
 *
 *		Flavor of authentication to use for method request.
 *
 *	    -n
 *	    -no_nego
 *
 *		Disable authentication negotiation.
 *
 *	    -p
 *	    -permit_nego
 *
 *		Permit authentication negotiation.
 *
 *	    -k PingCnt
 *	    -ping_cnt PingCnt
 *
 *		Number of times to try a ping request before assuming
 *		that the class agent has crashed.  Default is
 *		ADM_PING_CNT tries.
 *
 *	    -w PingSecs PingUsecs
 *	    -ping_timeout PingSecs PingUsecs
 *
 *		Number of seconds and microseconds to wait for an
 *		acknowledgement to a ping request.  Default is
 *		ADM_PING_TSECS seconds and ADM_PING_TUSECS microseconds.
 *
 *	    -i DelaySecs DelayUsecs
 *	    -ping_delay DelaySecs DelayUsecs
 *
 *		Number of seconds and microseconds to delay before
 *		attempting the first ping request.  Default is
 *		ADM_DELAY_TSECS seconds and ADM_DELAY_TUSECS microseconds.
 *
 *	    -x AckSecs AckUsecs
 *	    -ack_timeout AckSecs AckUsecs
 *
 *		Number of seconds and microseconds to wait for the method
 *		server to acknowledge the method request.  Default is
 *		ADM_ACK_TSECS seconds and ADM_ACK_TUSECS microseconds.
 *
 *	    -y RepSecs RepUsecs
 *	    -rep_timeout RepSecs RepUsecs
 *
 *		Number of seconds and microseconds to wait for the method
 *		to return the results of the invocation.  Default is
 *		ADM_REP_TSECS and ADM_REP_TUSECS.
 *
 *	    -r ServerProg ServerVers
 *	    -agent ServerProg ServerVers
 *
 *		RPC program and version number of method server.  Default
 *		is the standard administrative class agent.
 *
 *	    -l
 *	    -local_dispatch
 *
 *		Invoke the requested method locally without using RPC
 *		or a class agent.  This mode is useful for debugging
 *		methods and for invoking methods under unusual circumstances
 *		when a class agent is not available (such as at boot-time).
 *
 *	    -D DebugCategories
 *	    -DEBUG DebugCategories
 *
 *		Debug categories to activate during this method invocation.
 *		AMCL debug messages from within the specified categories
 *		will be written to STDOUT during the method invocation.
 *		If the method is being invoked in "local dispatch" mode,
 *		class agent and object manager debug messages from within
 *		the specified categories will also be displayed.
 *
 *	The caller may also specify the data to be used as input to the
 *	method using the following options:
 *
 *	    -u [file]
 *	    -unfmt [file]
 *
 *		Send unformatted input to the method.  If a file is specified,
 *		the contents of the file are used as the method's unformatted
 *		input.  If no file is specified, the unformatted input is
 *		taken from the standard input until EOF is reached.
 *
 *	    -a name[=value] name[=value] ...
 *	    -arguments name[=value] name[=value] ...
 *
 *		Use the specified "name=value" pairs as the formatted
 *		input arguments to the method.  All arguments will be
 *		sent a string types.  If no "=" appears after the name,
 *		the argument is given a NULL value.
 *
 *	NOTE: In order to create the debugging version of this command,
 *	      tpm, this source should be compiled with the symbol "TPM"
 *	      defined and should be linked to the fake version of the
 *	      SunNet Manager library.
 *
 **************************************************************************
 */

#ifndef _apm_c
#define _apm_c

#pragma	ident	"@(#)apm.c	1.18	92/02/28 SMI"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <locale.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include "netmgt/netmgt.h"
#include "apm_impl.h"
#include "apm_msgs.h"
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_auth_impl.h"

#define BUF_EXT	1024	/* Length by which to extend unformatted input buffer */
			/* whenever more space is needed in it. */

/*
 *-------------------------------------------------------------------------
 *
 * PV( s ):
 *
 *	Return a printable value for string s.  If s is not NULL, a pointer
 *	to s is returned.  If s is NULL, a pointer to the string "<none>"
 *	is returned.
 *
 *-------------------------------------------------------------------------
 */

char *
pv(char *s)
{
	if (s != NULL) {
		return(s);
	} else {
		return(APM_MSGS(ASH_NONE));
	}
}

/*
 *-------------------------------------------------------------------------
 *
 * ASH_ERRMSG( code ):
 *
 *	Return the (localized) error message associated with apm
 *	error "code".  If the code does not exist, a (localized)
 *	string equivalent to "Unkown Error" is return.
 *
 *-------------------------------------------------------------------------
 */

static
char *
ash_errmsg(int code)
{
	if ((code <= APM_MSGS_BASE_ERRORS) || (code > APM_MSGS_END_ERRORS)) {
		return(APM_MSGS(ASH_UNKNOWN_ERR));
	} else {
		return(APM_MSGS(code));
	}
}

/*
 *-------------------------------------------------------------------------
 *
 * GET_UNFMT_INPUT( in_handlep, filename ):
 *
 *	Determine the unformatted input to be used in the method
 *	invocation and stored it in the specified administrative data
 *	handle.  If filename is NULL, the input is taken from the standard
 *	input until EOF is reached.  If filename is not NULL, the input
 *	is taken from the specified file.
 *
 *-------------------------------------------------------------------------
 */

void
get_unfmt_input(
	Adm_data *in_handlep,
	char *filename
	)
{
	int infile;		/* Input file descriptor */
	caddr_t data;		/* Input data */
	u_int size;		/* Size of input data */
	u_int buf_size;		/* No. of free bytes remaining in buffer */
	int ibytes;		/* No. of bytes read by read (2v) */
	boolean_t end_of_input;	/* End of unformatted input? */
	int stat;

	/*
	 * Open input file.
	 */

	if (filename != NULL) {				/* Filename specified */
		infile = open(filename, O_RDONLY);
		if (infile == -1) {
			fprintf(stderr, ash_errmsg(ASH_OPEN), errno,
				strerror(errno));
			exit(ADM_FAILURE);
		}
	} else {					/* Default to STDIN */
		infile = 0;
	}

	/*
	 * Read input until EOF.
	 */

	data = malloc(BUF_EXT);				/* Allocate Buffer */
	if (data == NULL) {
		fprintf(stderr, ash_errmsg(ASH_MALLOC), errno,
			strerror(errno));
		exit(ADM_FAILURE);
	}
	buf_size = BUF_EXT;
	size = 0;

	end_of_input = B_FALSE;
	while (!end_of_input) {

		if (buf_size == (u_int) 0) {		/* Extend Buffer */
			data = realloc(data, (size_t)(size + BUF_EXT));
			if (data == NULL) {
				fprintf(stderr, ash_errmsg(ASH_REALLOC), errno,
					strerror(errno));
				exit(ADM_FAILURE);
			}
			buf_size = BUF_EXT;
		}

							/* Get input */
		ibytes = read(infile, (char *)(data + size), (unsigned) buf_size);
		switch (ibytes) {

		    case -1:

			fprintf(stderr, ash_errmsg(ASH_READ), errno,
				strerror(errno));
			exit(ADM_FAILURE);

		    case 0:

			end_of_input = B_TRUE;
			break;

		    default:

			size += ibytes;
			buf_size -= ibytes;
			break;
		}
	}

	if (filename != NULL) {
		close(infile);
	}
	stat = adm_args_putu(in_handlep, (caddr_t) data, (u_int) size);
	if (stat != ADM_SUCCESS) {
		fprintf(stderr, ash_errmsg(ASH_SET_UNFMT), stat,
			adm_err_msg(stat));
		exit(ADM_FAILURE);
	}
	free(data);
}

/*
 *-------------------------------------------------------------------------
 *
 * PARSE_ARGS( argc, argv, arg_index, in_handlep ):
 *
 *	Parse any formatted input arguments for the method.  arg_index
 *	should be the index (in argv) of the first "name=value" pair
 *	specified on the command line.  For each "name=value" pair,
 *	a corresponding string argument is added to the specified
 *	administrative data handle.
 *
 *	NOTE: This routine alters the argv structure.  Specifically,
 *	      the routine overwrites the '=' character in each "name=value"
 *	      pair with a NULL.
 *
 *-------------------------------------------------------------------------
 */

void
parse_args(
	int argc,
	char *argv[],
	int arg_index,
	Adm_data *in_handlep
	)
{
	char *name;		/* Name of argument */
	u_int length;		/* Length of argument value */
	caddr_t valuep;		/* Value of argument */
	int next_arg;		/* Index of next argument to process */
	char *eqpos;		/* Position of '=' in name=value pair */
	int stat;

	next_arg = arg_index;
	while (next_arg < argc) {

	    /*
	     * Get argument name and value.
	     */

	    name = argv[next_arg++];
	    if ((eqpos = strchr(name, (int) '=')) != NULL) {
	    	*eqpos = NULL;
	        valuep = (caddr_t) (eqpos + 1);
	        length = strlen(valuep);
	    } else {
		valuep = NULL;
		length = 0;
	    }

	    /*
	     * Add argument to administrative data handle.
	     */

	    stat = adm_args_puta(in_handlep, name, ADM_STRING, length, valuep);
	    if (stat != ADM_SUCCESS) {
		fprintf(stderr, ash_errmsg(ASH_SET_FAIL), stat,
			adm_err_msg(stat));
		exit(ADM_FAILURE);
	    }
	}
}

/*
 *-------------------------------------------------------------------------
 *
 * PARSE_OPTS( argc, argv, in_handlep, arg_index, flagsp, class, class_vers,
 *	       method, host, domain, auth_type, auth_flavor, ack_timeout,
 *	       rep_timeout, ping_cnt, ping_timeout, ping_delay, agent_prog,
 *	       agent_vers, client_group, debugp, allow_negop ):
 *
 *	Parse the method request options from the command line.  The
 *	valid options (everything excluding the method request arguments)
 *	are outlined at the top of this file.  The parsed values are placed
 *	in the following locations:
 *
 *		*flagsp		Framework control flags.
 *
 *		*class		Class of method to invoke.
 *
 *		*class_vers	Class version number.
 *
 *		*method		Name of method to invoke.
 *
 *		*host		Host on which to invoke method.
 *
 *		*domain		Domain in which method should process request.
 *
 *		*auth_type	Request authentication type.
 *
 *		*auth_flavor	Request authentication flavor.
 *
 *		*ack_timeout	timeval structure representing timeout to
 *				wait for acknowledgement from class agent
 *				of initial method request.
 *
 *		*rep_timeout	timeval structure representing timeout to
 *				wait for results of method invocation.
 *
 *		*ping_cnt	Number of times to try pinging a class
 *				agent before assuming it has crashed.
 *
 *		*ping_timeout	timeval structure representing timeout to
 *				wait for acknowledgement from a ping request.
 *
 *		*ping_delay	timeval structure representing delay before
 *				making first attempt.
 *
 *		*agent_prog	RPC program number of class agent serving method.
 *
 *		*agent_vers	RPC version number of class agent serving method.
 *
 *		*client_group	Client's preferred group.
 *
 *		*debugp		Active debugging categories.
 *
 *		*allow_negop	Set to B_TRUE is authentication negotiation
 *				is allowed, B_FALSE otherwise.
 *
 *	These value are taken from the command line specifications.  If
 *	a value is not specified for one of the options, an appropriate
 *	default is chosen.
 *
 *	In addition, this routine places an unformatted input specification
 *	(option "-u") into the specified administrative data handle.
 *
 *	The index of the first method argument ("name=value" pair) is returned
 *	in *arg_index.  If there are no method arguments specified on the
 *	command line, *arg_index is set to argc.
 *
 *-------------------------------------------------------------------------
 */

void
parse_opts(
	int argc,
	char *argv[],
	Adm_data *in_handlep,
	int *arg_index,
	u_long *flagsp,
	char **class,
	char **class_vers,
	char **method,
	char **host,
	char **domain,
	u_int *auth_type,
	u_int *auth_flavor,
	struct timeval *ack_timeout,
	struct timeval *rep_timeout,
	u_int *ping_cnt,
	struct timeval *ping_timeout,
	struct timeval *ping_delay,
	u_long *agent_prog,
	u_long *agent_vers,
	char **client_group,
	char **debugp,
	boolean_t *allow_negop
	)
{
	char *cmd;		/* Ptr. to command option being processed */
	long secs;		/* Timeout seconds */
	long usecs;		/* Timeouts micro-seconds */
	int next_arg;		/* Index of next argument to process */
	char *unfmt_file;	/* File containing unfmt input. NULL=STDIN */
	u_int new_type;		/* New authentication type */
	u_int new_flavor;	/* New authentication flavor */
	char *endno;
	int stat;

	/*
	 * Initialize request option values.
	 */

	*flagsp	      = adm_flags;
	*class 	      = NULL;
	*class_vers   = ADM_DEFAULT_VERS;
	*method       = NULL;
	*host 	      = ADM_LOCALHOST;
	*domain       = NULL;
	*client_group = NULL;
	*auth_type    = adm_auth_init_type;
	*auth_flavor  = adm_auth_init_flavor;
	*ping_cnt     = adm_ping_cnt;
	ack_timeout->tv_sec   = ADM_ACK_TSECS;
	ack_timeout->tv_usec  = ADM_ACK_TUSECS;
	rep_timeout->tv_sec   = adm_rep_timeout;
	rep_timeout->tv_usec  = 0L;
	ping_timeout->tv_sec  = adm_ping_timeout;
	ping_timeout->tv_usec = 0L;
	ping_delay->tv_sec    = adm_ping_delay;
	ping_delay->tv_usec   = 0L;
	*agent_prog  = ADM_CLASS_AGENT_PROG;
	*agent_vers  = ADM_CLASS_AGENT_VERS;
	*debugp      = ADM_DBG_NOCATS;
	*allow_negop = ADM_DEFAULT_NEGO;

	/*
	 * Parse the control options.
	 */

	if (argc == 1) {
		printf("%s\n", APM_MSGS(ASH_USAGE1));
		printf("%s\n", APM_MSGS(ASH_USAGE2));
		printf("%s\n", APM_MSGS(ASH_USAGE3));
		printf("%s\n", APM_MSGS(ASH_USAGE4));
		printf("%s\n", APM_MSGS(ASH_USAGE5));
		printf("%s\n", APM_MSGS(ASH_USAGE6));
		printf("\n");
		printf("%s\n", APM_MSGS(ASH_MNEMONIC));
		printf("\n");
		exit(ADM_FAILURE);
	}

	next_arg = 1;
	while (next_arg < argc) {
	    cmd = argv[next_arg++];

	    if ((strcmp(cmd, ASH_CLASS_O) == 0) ||		/* Class */
		(strcmp(cmd, ASH_CLASS_L) == 0))   {		/* and version */

		if (next_arg < argc) {
			*class = argv[next_arg++];
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_CLASS));
			exit(ADM_FAILURE);
		}
		if ((next_arg < argc) && (*argv[next_arg] != '-')) {
			if (strchr(argv[next_arg], (int)'=') != NULL) {
			    fprintf(stderr, ash_errmsg(ASH_MISSING_ARGSPEC),
				    ASH_ARGUMENTS_O, ASH_ARGUMENTS_L,
				    argv[next_arg]);
			    exit(ADM_FAILURE);
			}
			*class_vers = argv[next_arg++];
		}

	    } else if ((strcmp(cmd, ASH_METHOD_O) == 0) ||	/* Method */
		       (strcmp(cmd, ASH_METHOD_L) == 0))  {

		if (next_arg < argc) {
			*method = argv[next_arg++];
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_METHOD));
			exit(ADM_FAILURE);
		}

	    } else if ((strcmp(cmd, ASH_HOST_O) == 0) ||	/* Host */
		       (strcmp(cmd, ASH_HOST_L) == 0))  {

		if (next_arg < argc) {
			*host = argv[next_arg++];
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_HOST));
			exit(ADM_FAILURE);
		}

	    } else if ((strcmp(cmd, ASH_DOMAIN_O) == 0) ||	/* Domain */
		       (strcmp(cmd, ASH_DOMAIN_L) == 0))  {

		if (next_arg < argc) {
			*domain = argv[next_arg++];
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_DOMAIN));
			exit(ADM_FAILURE);
		}

	    } else if ((strcmp(cmd, ASH_AUTHTYPE_O) == 0) ||	/* Auth Type */
		       (strcmp(cmd, ASH_AUTHTYPE_L) == 0))  {

		if (next_arg < argc) {
			stat = adm_auth_str2type(argv[next_arg++], &new_type);
			if (stat != ADM_AUTH_OK) {
				fprintf(stderr, ash_errmsg(ASH_BAD_AUTHTYPE));
				exit(ADM_FAILURE);
			}
			*auth_type = new_type;
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_AUTHTYPE));
			exit(ADM_FAILURE);
		}

	    } else if ((strcmp(cmd, ASH_AUTHFLAVOR_O) == 0) ||	/* Auth Flavor */
		       (strcmp(cmd, ASH_AUTHFLAVOR_L) == 0))  {

		if (next_arg < argc) {
			stat = adm_auth_str2flavor(argv[next_arg++], &new_flavor);
			if (stat != ADM_AUTH_OK) {
				fprintf(stderr, ash_errmsg(ASH_BAD_AUTHFLAVOR));
				exit(ADM_FAILURE);
			}
			*auth_flavor = new_flavor;
			*auth_type = ADM_AUTH_UNSPECIFIED;
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_AUTHFLAVOR));
			exit(ADM_FAILURE);
		}

	    } else if ((strcmp(cmd, ASH_NONEGO_O) == 0) ||	/* No nego */
		       (strcmp(cmd, ASH_NONEGO_L) == 0))  {

		*allow_negop = B_FALSE;

	    } else if ((strcmp(cmd, ASH_PERMITNEGO_O) == 0) ||	/* Permit nego */
		       (strcmp(cmd, ASH_PERMITNEGO_L) == 0))  {

		*allow_negop = B_TRUE;

	    } else if ((strcmp(cmd, ASH_ARGUMENTS_O) == 0) ||	/* Arguments */
		       (strcmp(cmd, ASH_ARGUMENTS_L) == 0))  {

		break;

	    } else if ((strcmp(cmd, ASH_ACK_TIMEOUT_O) == 0) ||	/* Ack Timeout */
		       (strcmp(cmd, ASH_ACK_TIMEOUT_L) == 0))  {
	
		if (next_arg < (argc - 1)) {
			secs = strtol(argv[next_arg], &endno, 10);
			if (endno == argv[next_arg++]) {
				fprintf(stderr, ash_errmsg(ASH_BAD_ACKTIME));
				exit(ADM_FAILURE);
			}
			usecs = strtol(argv[next_arg], &endno, 10);
			if (endno == argv[next_arg++]) {
				fprintf(stderr, ash_errmsg(ASH_BAD_ACKTIME));
				exit(ADM_FAILURE);
			}
			ack_timeout->tv_sec = secs;
			ack_timeout->tv_usec = usecs;
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_ACKTIME));
			exit(ADM_FAILURE);
		}

	    } else if ((strcmp(cmd, ASH_REP_TIMEOUT_O) == 0) ||	/* Rep Timeout */
		       (strcmp(cmd, ASH_REP_TIMEOUT_L) == 0))  {

		if (next_arg < (argc - 1)) {
			secs = strtol(argv[next_arg], &endno, 10);
			if (endno == argv[next_arg++]) {
				fprintf(stderr, ash_errmsg(ASH_BAD_REPTIME));
				exit(ADM_FAILURE);
			}
			usecs = strtol(argv[next_arg], &endno, 10);
			if (endno == argv[next_arg++]) {
				fprintf(stderr, ash_errmsg(ASH_BAD_REPTIME));
				exit(ADM_FAILURE);
			}
			rep_timeout->tv_sec = secs;
			rep_timeout->tv_usec = usecs;
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_REPTIME));
			exit(ADM_FAILURE);
		}

	    } else if ((strcmp(cmd, ASH_PING_CNT_O) == 0) ||	/* Ping Tries */
		       (strcmp(cmd, ASH_PING_CNT_L) == 0))  {

		if (next_arg < argc) {
			*ping_cnt = strtol(argv[next_arg], &endno, 10);
			if (endno == argv[next_arg++]) {
				fprintf(stderr, ash_errmsg(ASH_BAD_PING_CNT));
				exit(ADM_FAILURE);
			}
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_PING_CNT));
			exit(ADM_FAILURE);
		}

	    } else if ((strcmp(cmd, ASH_PING_TIMEOUT_O) == 0) || /* Ping T.O. */
		       (strcmp(cmd, ASH_PING_TIMEOUT_L) == 0))  {

		if (next_arg < (argc - 1)) {
			secs = strtol(argv[next_arg], &endno, 10);
			if (endno == argv[next_arg++]) {
				fprintf(stderr, ash_errmsg(ASH_BAD_PINGTIME));
				exit(ADM_FAILURE);
			}
			usecs = strtol(argv[next_arg], &endno, 10);
			if (endno == argv[next_arg++]) {
				fprintf(stderr, ash_errmsg(ASH_BAD_PINGTIME));
				exit(ADM_FAILURE);
			}
			ping_timeout->tv_sec = secs;
			ping_timeout->tv_usec = usecs;
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_PINGTIME));
			exit(ADM_FAILURE);
		}

	    } else if ((strcmp(cmd, ASH_PING_DELAY_O) == 0) ||  /* Ping Delay */
		       (strcmp(cmd, ASH_PING_DELAY_L) == 0))  {

		if (next_arg < (argc - 1)) {
			secs = strtol(argv[next_arg], &endno, 10);
			if (endno == argv[next_arg++]) {
				fprintf(stderr, ash_errmsg(ASH_BAD_DELAYTIME));
				exit(ADM_FAILURE);
			}
			usecs = strtol(argv[next_arg], &endno, 10);
			if (endno == argv[next_arg++]) {
				fprintf(stderr, ash_errmsg(ASH_BAD_DELAYTIME));
				exit(ADM_FAILURE);
			}
			ping_delay->tv_sec = secs;
			ping_delay->tv_usec = usecs;
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_DELAYTIME));
			exit(ADM_FAILURE);
		}

	    } else if ((strcmp(cmd, ASH_AGENT_O) == 0) ||	/* Class agent */
		       (strcmp(cmd, ASH_AGENT_L) == 0))  {

		if (next_arg < (argc - 1)) {
			*agent_prog = strtol(argv[next_arg], &endno, 10);
			if (endno == argv[next_arg++]) {
				fprintf(stderr, ash_errmsg(ASH_BAD_AGENT));
				exit(ADM_FAILURE);
			}
			*agent_vers = strtol(argv[next_arg], &endno, 10);
			if (endno == argv[next_arg++]) {
				fprintf(stderr, ash_errmsg(ASH_BAD_AGENT));
				exit(ADM_FAILURE);
			}
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_AGENT));
			exit(ADM_FAILURE);
		}

	    } else if ((strcmp(cmd, ASH_UNFMT_O) == 0) ||	/* Unfmt input */
		       (strcmp(cmd, ASH_UNFMT_L) == 0))  {

		if (next_arg < argc) {
			unfmt_file = (*argv[next_arg] == '-' ? NULL :
							       argv[next_arg++]);
		} else {
			unfmt_file = NULL;
		}
		get_unfmt_input(in_handlep, unfmt_file);

	    } else if ((strcmp(cmd, ASH_CLIENTGROUP_O) == 0) ||	/* Clnt Group */
		       (strcmp(cmd, ASH_CLIENTGROUP_L) == 0))  {

		if (next_arg < argc) {
			*client_group = argv[next_arg++];
		} else {
			fprintf(stderr, ash_errmsg(ASH_MISSING_CLIENTGROUP));
			exit(ADM_FAILURE);
		}

	    } else if ((strcmp(cmd, ASH_LOCAL_O) == 0) ||   /* Local Dispatch? */
		       (strcmp(cmd, ASH_DEBUG_L) == 0))  {

		*flagsp |= ADM_LOCAL_REQUEST_FLAG;

	    } else if ((strcmp(cmd, ASH_DEBUG_O) == 0) ||	/* Debug Level */
		       (strcmp(cmd, ASH_DEBUG_L) == 0))  {

		if ((next_arg < argc) && (*argv[next_arg] != '-')) {
			*debugp = argv[next_arg++];
		} else {
			*debugp = ADM_DBG_ALLCATS;
		}

	    } else if ((strcmp(cmd, ASH_ARGUMENTS_O) == 0) ||	/* Arguments */
		       (strcmp(cmd, ASH_ARGUMENTS_L) == 0))  {

		break;

	    } else {

		printf("%s\n", APM_MSGS(ASH_USAGE1));
		printf("%s\n", APM_MSGS(ASH_USAGE2));
		printf("%s\n", APM_MSGS(ASH_USAGE3));
		printf("%s\n", APM_MSGS(ASH_USAGE4));
		printf("%s\n", APM_MSGS(ASH_USAGE5));
		printf("%s\n", APM_MSGS(ASH_USAGE6));
		printf("\n");
		printf("%s\n", APM_MSGS(ASH_MNEMONIC));
		printf("\n");
		exit(ADM_FAILURE);

	    }
	}

	/*
	 * Verify option values.
	 */

	if (*class == NULL) {			/* Must specify a class */
		fprintf(stderr, ash_errmsg(ASH_MISSING_CLASS));
		exit(ADM_FAILURE);
	}
	if (*method == NULL) {			/* Must specify a method */
		fprintf(stderr, ash_errmsg(ASH_MISSING_METHOD));
		exit(ADM_FAILURE);
	}

	/*
	 * Return index (in argv) of first argument to method.
	 */

	*arg_index = next_arg;
}

/*
 *-------------------------------------------------------------------------
 *
 * PRINT_ERROR( exit_stat, errorp ):
 *
 *	Print the specified exit status and error structure to STDOUT.
 *	The formatted error portion of the structure will be output as:
 *
 *		Completion Status: <status> [(exit status n)]
 *
 *		    <cleanliness> <type> failure <code>
 *		    <message>
 *
 *	where <status> is either "Success" or "Failure", and the exit
 *	status is printed only if it is not ADM_SUCCESS or ADM_FAILURE.
 *	In addition, a more informative error message will be printed
 *	if there is one in the Adm_error structure (*errorp).  This
 *	message will include the cleanliness indication ("Clean", "Dirty",
 *	or "Unknown"), source of the error ("System", "Method", or
 *	"Unknown"), error code, and optional error message.
 *
 *	If the error structure contains any unformatted error text, this
 *	will be output in the form:
 *
 *		Unformatted Error Text From Method:
 *
 *		<body of unformatted error text>
 *
 *-------------------------------------------------------------------------
 */

void
print_error(
	int exit_stat,
	Adm_error *errorp
	)
{
	/*
	 * Output formatted error or success indication.
	 */

	printf("%s ", APM_MSGS(ASH_COMPSTAT));

	if (exit_stat == ADM_SUCCESS) {
		printf("%s", APM_MSGS(ASH_SUCCESS));
	} else {
		printf("%s ", APM_MSGS(ASH_FAILURE));
		if (exit_stat != ADM_FAILURE) {
			printf(APM_MSGS(ASH_EXITSTAT),
			       exit_stat);
		}
	}
	printf("\n\n");

	if (errorp == NULL) {
		printf("%s\n\n", APM_MSGS(ASH_MISSERR));
		return;
	}

	if (adm_err_isentry(errorp)) {
	        printf("%s", APM_MSGS(ASH_COMPINDENT));
		printf(APM_MSGS(ASH_COMPFMT),
			APM_MSGS(
			    (errorp->cleanup == ADM_FAILCLEAN ? ASH_CLEAN :
			    (errorp->cleanup == ADM_FAILDIRTY ? ASH_DIRTY :
							        ASH_UNKNOWN))),
			APM_MSGS(
			    (errorp->type == ADM_ERR_SYSTEM ? ASH_ESYSTEM :
			    (errorp->type == ADM_ERR_CLASS  ? ASH_ECLASS :
							      ASH_UNKNOWN))),
			errorp->code);
		printf("\n");
		if (errorp->message != NULL) {
			printf("%s%s\n", APM_MSGS(ASH_COMPINDENT),
			    errorp->message);
		}
		printf("\n");
	}

	/*
	 * Output unformatted error text (if any).
	 */

	if (errorp->unfmt_txt != NULL) {
		printf("%s\n\n", APM_MSGS(ASH_UNFERRORS));
		fwrite(errorp->unfmt_txt, (size_t) errorp->unfmt_len,
		       (size_t) 1, stdout);
		printf("\n\n");
	}
}

/*
 *-------------------------------------------------------------------------
 *
 * PRINT_RESULTS( out_handlep ):
 *
 *	Print the results of a method invocation.  For each row of formatted
 *	data in the specified handle, an entry in the form
 *
 *	    Row i
 *		name=value
 *		name=value
 *		    :
 *		    :
 *
 *	is printed showing the argument names and values in that row.
 *	In addition, the unformatted data in the handle is also printed.
 *
 *-------------------------------------------------------------------------
 */

void
print_results(
	Adm_data *out_handlep
	)
{
	int row_num;		/* Row number */
	Adm_arg *argp;		/* Result argument */
	caddr_t unfp;		/* Ptr. to unformatted results */
	u_int unflen;		/* Length of unformatted results */
	boolean_t done;		/* Finished printing result table? */
	int stat;

	if (out_handlep == NULL) {
		return;
	}

	if ((adm_args_htype(out_handlep) & ADM_INVALID) == ADM_INVALID) {
		printf("\n%s\n", APM_MSGS(ASH_I_RESULTS));
	}
	
	/*
	 * Print formatted data table from specified handle.
	 */

	if ((adm_args_htype(out_handlep) & ADM_FORMATTED) == ADM_FORMATTED) {

	    adm_args_reset(out_handlep);
	    printf(APM_MSGS(ASH_F_RESULTS), "");
	    printf("\n\n");
	    row_num = 1;
	    printf(APM_MSGS(ASH_ROW), row_num);
	    printf("\n");

	    done = B_FALSE;
	    while (!done) {

		stat = adm_args_nexta(out_handlep, ADM_ANYTYPE, &argp);
		switch(stat) {

		    case ADM_SUCCESS:

			printf(APM_MSGS(ASH_ARGFMT),
				argp->name, pv(argp->valuep));
			printf("\n");
			break;

		    case ADM_ERR_ENDOFROW:

			stat = adm_args_nextr(out_handlep);
			if (stat == ADM_SUCCESS) {
			    printf("\n");
	    		    printf(APM_MSGS(ASH_ROW), ++row_num);
			    printf("\n");
			} else if (stat == ADM_ERR_ENDOFTABLE) {
			    done = B_TRUE;
			} else {
			    fprintf(stderr, ash_errmsg(ASH_ROWFAIL), stat,
					adm_err_msg(stat));
			    exit(ADM_FAILURE);
			}
			break;

		    default:

			fprintf(stderr, ash_errmsg(ASH_ARGFAIL), stat,
				adm_err_msg(stat));
			exit(ADM_FAILURE);
		}
	    }
	} else {

	    printf(APM_MSGS(ASH_F_RESULTS), APM_MSGS(ASH_NONE));
	    printf("\n");

	}

	/*
	 * Print any unformatted data contained in the specified handle.
	 */

	if ((adm_args_htype(out_handlep) & ADM_UNFORMATTED) == ADM_UNFORMATTED) {

		printf("\n");
		printf(APM_MSGS(ASH_U_RESULTS), "");
		printf("\n\n");
		adm_args_getu(out_handlep, &unfp, &unflen);
		fwrite(unfp, (size_t) unflen, (size_t) 1, stdout);
		printf("\n");

	} else {

		printf("\n");
		printf(APM_MSGS(ASH_U_RESULTS), APM_MSGS(ASH_NONE));
		printf("\n");
	}
}

/*
 *-------------------------------------------------------------------------
 *
 * MAIN( argc, argv ):
 *
 *	Main command control body.
 *
 *	    1. Parse command line options.
 *	    2. Parse input arguments for method.
 *	    3. Invoke requested method.
 *	    4. Display completion status and errors from method invocation.
 *	    5. Display results of method invocation.
 *
 *-------------------------------------------------------------------------
 */

main(argc, argv)
    int argc;
    char *argv[];
{
	u_long flags;			/* Framework control flags */
	char *class;			/* Class of method to invoke */
	char *class_vers;		/* Class version number */
	char *method;			/* Method to invoke */
	char *host;			/* Host on which to invoke method */
	char *domain;			/* Domain in which to exec method */
	char *client_group;		/* Client's preferred group */
	u_int auth_type;		/* Authentication type */
	u_int auth_flavor;		/* Authentication flavor */
	u_int ping_cnt;			/* # of ping tries per attempt */
	struct timeval ack_timeout;	/* Timeout for initial method request */
	struct timeval rep_timeout;	/* Timeout for results from method */
	struct timeval ping_timeout;	/* Timeout for ping acknowledgements */
	struct timeval ping_delay;	/* Delay before first ping attempt */
	u_long agent_prog;		/* RPC program # of method server */
	u_long agent_vers;		/* RPC version # of method server */
	boolean_t allow_nego;		/* Allow auth negotiation? */
	Adm_data *in_handlep;		/* Input arguments to method */
	Adm_data *out_handlep;		/* Results of method */
	Adm_error *errorp;		/* Error info from method */
	int arg_index;			/* Index in argv of first method arg */
	int stat;

	(void) setlocale(LC_ALL, "");
	bindtextdomain(ASH_TEXT_DOMAIN, ADM_MSG_PATH);

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		fprintf(stderr, ash_errmsg(ASH_INITFAIL));
		exit(ADM_FAILURE);
	}

	/*
	 * Parse options.
	 */

	in_handlep = adm_args_newh();
	if (in_handlep == NULL) {
		fprintf(stderr, ash_errmsg(ASH_NOHANDLE), stat, adm_err_msg(stat));
		exit(ADM_FAILURE);
	}
	parse_opts(argc, argv, in_handlep, &arg_index, &flags, &class,
		   &class_vers, &method, &host, &domain, &auth_type,
		   &auth_flavor, &ack_timeout, &rep_timeout, &ping_cnt,
		   &ping_timeout, &ping_delay, &agent_prog, &agent_vers,
		   &client_group, &adm_debug_cats, &allow_nego);

	/*
	 * Parse input arguments for method.
	 */

	parse_args(argc, argv, arg_index, in_handlep);

	/*
	 * Execute Method.
	 */

	adm_set_local_dispatch_info(B_TRUE);

	stat = adm_perf_method(method, in_handlep, &out_handlep, &errorp,
		    ADM_CLASS,           class,
		    ADM_CLASS_VERS,      class_vers,
		    ADM_HOST,            host,
		    ADM_LOCAL_DISPATCH,  ((flags & ADM_LOCAL_REQUEST_FLAG) != 0),
		    ADM_DOMAIN,          domain,
		    ADM_CLIENT_GROUP,    client_group,
		    ADM_AUTH_FLAVOR,     auth_flavor,
		    ADM_AUTH_TYPE,       auth_type,
		    ADM_ALLOW_AUTH_NEGO, allow_nego,
		    ADM_PINGS,	 	 ping_cnt,
		    ADM_ACK_TIMEOUT,  ack_timeout.tv_sec,  ack_timeout.tv_usec,
		    ADM_REP_TIMEOUT,  rep_timeout.tv_sec,  rep_timeout.tv_usec,
		    ADM_PING_TIMEOUT, ping_timeout.tv_sec, ping_timeout.tv_usec,
		    ADM_PING_DELAY,   ping_delay.tv_sec,   ping_delay.tv_usec,
		    ADM_AGENT,        agent_prog, agent_vers,
		    ADM_ENDOPTS);

	/*
	 * Print method results.
	 */

	printf("\n");

	print_error(stat, errorp);

	print_results(out_handlep);

	exit(stat);
}

#endif /* !_apm_c */

