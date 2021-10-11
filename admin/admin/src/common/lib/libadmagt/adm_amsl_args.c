/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)adm_amsl_args.c	1.22	94/11/16 SMI"

/*
 * FILE:  args.c
 *
 *	Admin Framework routines to build argv list and environment
 *	variables for invoking a class method executable runfile.
 */

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <malloc.h>
#include <stdio.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"

/* Declare static functions */
static int build_var(struct amsl_req *, char *, char *);

/* Declare external functions not defined in header files */
extern int putenv();

/*
 * -------------------------------------------------------------------
 * build_argv - Routine to build executable runfile argv list.
 *	Accepts the pathname of the method executable runfile, the address
 *	of an Adm_data structure pointing to a linked list of input
 *	arguments, and the address of variable to contain the argv list
 *	structure address.
 *	Builds an argv structure to be used as input to an exec'd method.
 *	The argv structure will contain the pathname in element 0 and a
 *	"value" string for each (named) argument and value in the linked
 *	list.  The "value" part of the pairs will be placed in the argv
 *	structure in the same order that they appear in the linked list.
 *	This function returns SUCCESS upon normal completion.  If an error
 *	occurs building the argv structure, the function will return an error
 *	status and add the error message to the top of the error stack.
 * -------------------------------------------------------------------
 */

#define	PROG_NAME	"build_argv"

int
build_argv(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct Adm_data *argh,	/* Ptr to adm data handle for input args */
	char ***argv_p)		/* Ptr to address to return argv struct */
{
	int arg_no;		/* Argument number in argv */
	int count;		/* Number of arguments in arg-list "argp" */
	int tstat;		/* Local error status code */
	Adm_arg *ap;		/* Pointer to an admin argument */
	char **argv;		/* argv structure for method arguments */
	int arg_size;		/* Number of bytes needed to store value */

	count = 0;
	/* Sequentially read through input arguments to count how many */
	if ((adm_args_reset(argh)) == ADM_SUCCESS) {
		while ((tstat = adm_args_nexta(argh, ADM_STRING, &ap)) ==
		    ADM_SUCCESS)
			count++;
		if ((tstat != ADM_ERR_ENDOFROW) && (tstat != ADM_ERR_NOFRMT))
			return (amsl_err(reqp, ADM_ERR_BADINPUTARG, tstat));
	}

	/* Create an argv structure with method pathname in first entry */
	argv = (char **)calloc((unsigned)(count+2), sizeof (char *));
	if (argv == NULL)
		return (amsl_err(reqp, ADM_ERR_NOMEMORY, PROG_NAME, 1));
	argv[0] = reqp->method_filename;

	/* Reset input argument handle to start of argument list */
	adm_args_reset(argh);

	/* Read each input argument and add its value to argv list */
	for (arg_no = 1; arg_no <= count; arg_no++) {
		if ((tstat = adm_args_nexta(argh, ADM_STRING, &ap)) ==
		    ADM_SUCCESS) {
			arg_size = ap->length + 1;
			if ((argv[arg_no] = malloc((size_t)(arg_size + 1))) ==
			    NULL)
				return (amsl_err(reqp, ADM_ERR_NOMEMORY,
				    PROG_NAME, 2));
			if (ap->valuep != NULL)
				(void) strcpy(argv[arg_no], ap->valuep);
		}
		else
			return (amsl_err(reqp, ADM_ERR_BADINPUTARG, tstat));
	}
	argv[count + 1] = NULL;	/* argv is always NULL-terminated */


	/* Return the argv structure and pointer to the input_args string */

	*argv_p = argv;
	return (0);
}

#undef PROG_NAME

/*
 * -----------------------------------------------------------------------
 * build_env - Build class agent environment variables for access from
 *	method runfile process environment.
 *	Accepts a pointer to the amsl request structure containing the
 *	class name, method name, host name, domain name, and requestID
 *	values.  Builds the following environment variables...
 *
 *		ADM_FLAGS	=> framework flags (agent to AMCL)
 *		ADM_CLASS_NAME	=> class name
 *		ADM_METHOD_NAME	=> method name
 *		ADM_DOMAIN_NAME	=> method domain name
 *		ADM_CLIENT_ID	=> client identity: flavor, uid, cpn
 *		ADM_CLIENT_HOST	=> client host name
 *		ADM_CLIENT_DOMAIN => client domain name
 *		ADM_REQUESTID	=> request identifier from client
 *		ADM_INPUTARGS	=> formatted input arguments
 *		ADM_STDFMT	=> formatted return argument and error
 *				   pipe file descripter in child process
 *		ADM_STDCATS	=> standard logging category names
 *		ADM_TIMEOUT_PARMS => client's maximum timeout for method
 *		ADM_LPATH	=> Class localization pathname
 *		ADM_TEXT_DOMAINS=> Class localization text domain names
 *
 *	Resets the following environment variables...
 *
 *		LANG		=> Localization language
 *		PATH		=> ${PATH}:<OW_path>/bin
 *		LD_LIBRARY_PATH	=> Shared library search path for method
 *		HELPPATH	=> Spot help search path for method
 *		OPENWINHOME	=> OW path (reset if changed)
 *
 *	Places variables directly into environmnet via putenv calls.
 */

#define	PROG_NAME	"build_env"

int
build_env(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct Adm_data *argh,	/* Ptr to adm data handle for input args */
	int fmt_fd,		/* File descriptor for STDFMT pipe */
	char ***env_p)		/* Address of pointer to envp structure */
{
	struct amsl_auth *aup;	/* Pointer to request auth info */
	Adm_arg *ap;		/* Pointer to an admin argument */
	char *bp;		/* Temporary string buffer pointer */
	u_int  blen;		/* Temporary string buffer size */
	u_int  size; 		/* Temporary string buffer size */
	int  tstat;		/* Temporary error status code */
	char tbuff[MAXPATHLEN*2]; /* Temp buffer for conversions */
	char cpnbuff[ADM_AUTH_CPN_NAMESIZE + 1]; /* Cpn conversion buffer */
	char ridbuf[ADM_MAXRIDLEN+1];  /* Temp buffer for request ID */
	long   aflags;		/* Agent framework to AMCL flags */
	char  *owhp;		/* Pointer to OW home pathname */
	char  *tp;		/* Temp string pointer */
	u_int  tlen;		/* Size of string in ridbuf */
	char env_buff[AMSL_INPUTARGS_BUFFSIZE];	/* Input args buffer */

	extern char **environ;	/* Current environment global variable */

	/* Get client identity and authentication information */
	/* Note: Done at this time due to an alpha4 dlopen() bug */
	aup = reqp->authp;
	cpnbuff[0] = '\0';
	adm_auth_cpn2str((u_int)ADM_AUTH_FULLIDS, &aup->auth_cpn, cpnbuff,
	    (u_int)ADM_AUTH_CPN_NAMESIZE);

	/*
	 * Reset existing environment variables for the method process
	 *	LANG		=> Set to client's language preferrence
	 *	PATH		=> Set to ${PATH}:<OW_pathname>/bin
	 *	LD_LIBRARY_PATH	=> Set to <OW_pathname>/lib and class
	 *			   hierarchy lib and class directories
	 *	HELPPATH	=> Set to <OW_pathname> help and locale
	 *			   directories, and class hierarchy locale
	 *			   directory
	 *	OPENWINHOME	=> Set to OW pathname for method process
	 *
	 * Note: OW_pathname is set from the following sources (in order):
	 *	-O <pathname> CLI argument, if it was specified and exists
	 *	OPENWINHOME environment variable, if it exists
	 *	/etc/OPENWINHOME installed pathname, if it exists
	 *	ADM_OW_PATHNAME from adm_amsl header file (all else fails).
	 */

	/* Get open windows home pathname. */
	owhp = amsl_ctlp->ow_cli_pathname;
	if (owhp == (char *)NULL)
		owhp = getenv("OPENWINHOME");
	if (owhp == (char *)NULL)
		owhp = amsl_ctlp->ow_etc_pathname;
	if (owhp == (char *)NULL)
		owhp = ADM_OW_PATHNAME;

	/* Reset localization language variable; ignore errors */
	if (reqp->client_lang_parm != (char *)NULL)
		(void) build_var(reqp, "LANG", reqp->client_lang_parm);

	/* Reset OPENWINHOME variable for method process */
	if ((tstat = build_var(reqp, "OPENWINHOME", owhp)) != 0)
		return (tstat);

	/* Add path to OW binaries to PATH variable */
	if (owhp != (char *)NULL) {
		if ((tp = getenv("PATH")) != (char *)NULL) {
			(void) strncpy(tbuff, tp, MAXPATHLEN);
			tbuff[MAXPATHLEN] = '\0';
			(void) strcat(tbuff, ":");
		} else
			tbuff[0] = '\0';
		(void) strcat(tbuff, owhp);
		(void) strcat(tbuff, "/bin");
		if ((tstat = build_var(reqp, "PATH", tbuff)) != 0)
			return (tstat);
	}

	/*
	 * Build shared library search paths for the method process.
	 * The first part of the path is .../classes/lib (derived from
	 * ADMINPATH).  The second is the same as the first except
	 * that we remove the 'classes' subdir.  This is needed since
	 * we have moved the location of the libraries.  They are no
	 * longer in /usr/snadm/classes/lib but are not just in
	 * /usr/snadm/lib.  Other paths are added after this.
	 */
	tp = tbuff;
	if ((adm_find_object_class(tp, (u_int)MAXPATHLEN)) == 0) {
		char *c = strstr(tp, "/classes");
		int len = c - tp;
		(void) strcat(tp, "/lib");
		(void) strcat(tp, ":");
		(void) strncat(tp, tp, len);
		(void) strcat(tp, "/lib");
	} else
		*tp = '\0';
	if (reqp->method_pathname != (char *)NULL) {
		if (*tp != '\0')
			(void) strcat(tp, ":");
		(void) strcat(tp, reqp->method_pathname);
	}
	if (owhp != (char *)NULL) {
		if (*tp != '\0')
			(void) strcat(tp, ":");
		(void) strcat(tp, owhp);
		(void) strcat(tp, "/lib");
	}
	if ((tstat = build_var(reqp, "LD_LIBRARY_PATH", tbuff)) != 0)
		return (tstat);
	
	/* Build spot help paths for the method process */
	tp = tbuff;
	*tp = '\0';
	if (owhp != (char *)NULL) {
		(void) strcat(tp, owhp);
		(void) strcat(tp, "/lib/help:");
		(void) strcat(tp, owhp);
		(void) strcat(tp, "/lib/locale");
	}
	if (amsl_ctlp->class_locale != (char *)NULL) {
		if (*tp != '\0')
			(void) strcat(tp, ":");
		(void) strcat(tp, amsl_ctlp->class_locale);
	}
	if ((tstat = build_var(reqp, "HELPPATH", tbuff)) != 0)
		return (tstat);

	/*
	 * Set special Admin environment variables
	 */

	/* Create framework flags environment variable */
	aflags = (long)0;
	if (reqp->request_flags & AMSL_LOCAL_REQUEST)
		aflags = (long)ADM_LOCAL_REQUEST_FLAG;
	(void) sprintf(tbuff, "%ld", aflags);
	if ((tstat = build_var(reqp, ADM_FLAGS_NAME, tbuff)) != 0)
		return (tstat);

	/* Create class name environment variable */
	if ((tstat = build_var(reqp, ADM_CLASS_NAME, reqp->class_name)) != 0)
		return (tstat);

	/* Create class version environment variable */
	if ((tstat = build_var(reqp, ADM_CLASS_VERS_NAME,
	    reqp->class_version)) != 0)
		return (tstat);

	/* Create method name environment variable */
	if ((tstat = build_var(reqp, ADM_METHOD_NAME, reqp->method_name)) != 0)
		return (tstat);

	/* Create agent domain name environment variable */
	if ((tstat = build_var(reqp, ADM_DOMAIN_NAME, reqp->agent_domain)) != 0)
		return (tstat);

	/* Create client identity environment variable */
	aup = reqp->authp;
	(void) sprintf(tbuff, "%d:%d:%ld:%d:%s", aup->auth_type, aup->auth_flavor,
	    (long)aup->auth_uid, aup->auth_cpn.context, cpnbuff);
	if ((tstat = build_var(reqp, ADM_CLIENT_ID_NAME, tbuff)) != 0)
		return (tstat);

	/* Create client host name environment variable */
	if ((tstat = build_var(reqp, ADM_CLIENT_HOST_NAME,
	    reqp->client_host)) != 0)
		return (tstat);

	/* Create client domain name environment variable */
	if ((tstat = build_var(reqp, ADM_CLIENT_DOMAIN_NAME,
	    reqp->client_domain)) != 0)
		return (tstat);

	/* Create requestID environment variable */
	tlen = 0;
	adm_reqID_rid2str(*reqp->reqIDp, ridbuf, ADM_MAXRIDLEN + 1, &tlen);
	ridbuf[tlen] = '\0';
	if ((tstat = build_var(reqp, ADM_REQUESTID_NAME, ridbuf)) != 0)
		return (tstat);

	/* Create STDFMT file desecripter environment variable */
	(void) sprintf(tbuff, "%d", fmt_fd);
	if ((tstat = build_var(reqp, ADM_STDFMT_NAME, tbuff)) != 0)
		return (tstat);

	/* Create diagnostic standard categories environment variable */
	if ((tstat = build_var(reqp, ADM_STDCATS_NAME, reqp->diag_stdcats))
	    != 0)
		return (tstat);

	/* Optionally create client method maximum timeout env variable */
	if (reqp->client_timeout_parm != (char *)NULL)
		if ((tstat = build_var(reqp, ADM_TIMEOUT_PARMS_NAME,
	    	 reqp->client_timeout_parm)) != 0)
			return (tstat);

	/* Create class L10n pathname environment variable */
	if ((tstat = build_var(reqp, ADM_LPATH_NAME, amsl_ctlp->class_locale))
	    != 0)
		return (tstat);

	/* Create class L10n text domain names environment variable */
	if ((tstat = build_var(reqp, ADM_TEXT_DOMAINS_NAME,
	     reqp->method_text_domains)) != 0)
		return (tstat);

	/* Build buffer of input arguments converted to string format */
	env_buff[0] = '\0';			/* In case no input args */
	if ((adm_args_reset(argh)) == ADM_SUCCESS) {
		bp = env_buff;
		blen = AMSL_INPUTARGS_BUFFSIZE;
		while ((tstat = adm_args_nexta(argh, ADM_STRING, &ap)) ==
		    ADM_SUCCESS) {
			if ((tstat = adm_args_a2str(ap->name, ap->type,
			    ap->length, ap->valuep, bp, blen, &size)) !=
			    ADM_SUCCESS)
				return (amsl_err(reqp, ADM_ERR_BADINPUTARG,
				    tstat));
			bp += size;
			blen -= size;
		}
		if ((tstat != ADM_ERR_ENDOFROW) && (tstat != ADM_ERR_NOFRMT))
			return (amsl_err(reqp, ADM_ERR_BADINPUTARG, tstat));
	}

	/* Now create the input arguments environment variable */
	if ((tstat = build_var(reqp, ADM_INPUTARGS_NAME, env_buff)) != 0)
		return (tstat);

	/* Return success */
	*env_p = environ;
	return (0);
}

#undef PROG_NAME

/*
 * ----------------------------------------------------------------------
 * build_var - Build specified environment variable and add to environment.
 *	Accepts the name and string value for an environment variable.
 *	Allocates memory for and builds the "name=value" string, then adds
 *	the variable to the environment via a putenv call.
 *	Returns an error status if unable to allocate memory, placing an
 *	error message on the top of the error stack.
 * ----------------------------------------------------------------------
 */

#define	PROG_NAME	"build_var"

static int
build_var(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	char *name,		/* Name of environment variable */
	char *valuep)		/* String value of environment variable */
{
	size_t size;		/* Temporary size variable */
	char *cp;		/* Temporary char string pointer */

	/* Create environment variable */
	size = strlen(name) + 2;
	if (valuep != NULL)
		size += strlen(valuep);
	if ((cp = malloc(size)) == NULL)
		return (amsl_err(reqp, ADM_ERR_NOMEMORY, PROG_NAME, 3));
	(void) strcpy(cp, name);
	(void) strcat(cp, "=");
	if (valuep != NULL)
		(void) strcat(cp, valuep);
	if (putenv(cp) != 0)
		return (amsl_err(reqp, ADM_ERR_BADPUTENV, name));

	/* Return success */
	return (0);
}

#undef PROG_NAME
