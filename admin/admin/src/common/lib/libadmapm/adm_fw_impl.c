
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains the general functions used internally within
 *	the administative framework.  It contains:
 *
 *		adm_check_len()	Verify that the length of a string does
 *				not exceed a given threshold.
 *
 *		adm_exit()	Exit handling routine.  Used to clean
 *				up when a framework client is terminating.
 *
 *		adm_fw_debug()	Display a framework debugging message.
 *
 *		adm_meth_init()	Initialize default pathname to the method's
 *				message files.
 *
 *		adm_msgs_init()	Initialize the message localization file
 *				pathnames for the framework message files.
 *
 *		adm_env_init()	Initialize the above global variables from
 *				their values in the environment (as placed
 *				there by the AMSL).
 *
 *		adm_strtok()	Thread-safe version of strtok.
 *
 *		adm_make_tok()	Find the next token in a string.
 *
 *	NOTE: There is no checking whether specified class names, method names,
 *	      host names, domain names, or request IDs are valid.  For names,
 *	      the names should have no spaces or be the same as ADM_BLANK.
 *
 *******************************************************************************
 */

#ifndef _adm_fw_impl_c
#define _adm_fw_impl_c

#pragma	ident	"@(#)adm_fw_impl.c	1.22	92/02/28 SMI"

#include <unistd.h>
#include <stddef.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

/*
 *--------------------------------------------------------------------
 *
 *  ADM_CHECK_LEN( str, len, err_cond, flagp, condp ):
 *
 *	Verify that the specified string does not exceed len bytes.
 *	If it does then set *flagp to B_TRUE and set *condp to be the
 *	error condition indicated by err_cond.
 *
 *	NOTE: If str is a NULL pointer, this routine takes no actions.
 *
 *--------------------------------------------------------------------
 */

void
adm_check_len(
	char *str,
	u_int len,
	int err_cond,
	boolean_t *flagp,
	int *condp
	)
{
	if (str != NULL) {
		if (strlen(str) > (size_t) len) {
			if (!*flagp) {
				*condp = err_cond;
			}
			*flagp = B_TRUE;
		}
	}
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_FW_DEBUG( message, arg1, arg2, ... ):
 *
 *	Print the specified debugging message (with substitution
 *	arguments) to STDOUT.
 *
 *--------------------------------------------------------------------
 */

void
adm_fw_debug(
	char *message,
	...)
{
	va_list vp;		/* Varargs argument list pointer */
	time_t  dtime;		/* Time argument */
	struct  tm *tm_p;	/* Pointer to time buffer */

	va_start(vp, messages);
	dtime = time((time_t *)NULL);
	tm_p = localtime(&dtime);
	printf("[%ld; %.2d:%.2d:%.2d] ", (long)getpid(), tm_p->tm_hour,
	    tm_p->tm_min, tm_p->tm_sec);
	vprintf(message, vp);
	printf("\n");
	va_end(vp);

	return;
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_EXIT():
 *
 *	Framework exit handling routine.  This routine de-allocates
 *	the rendezvous RPC number being used by the AMCL.
 *
 *	Upon normal completion, this routine returns ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

void
adm_exit()
{
	if (is_rpc_allocated) {
		adm_amcl_cleanup(t_udp_sock, t_tcp_sock,
			      t_rendez_prog, (u_long) RENDEZ_VERS);
	}

	return;
}

/*
 *--------------------------------------------------------------------
 *
 *  ADM_ENV_INIT():
 *
 *	Initialize the general global variables maintained by the framework.
 *	The routine sets the process's host variable (adm_host), default
 *	timeout parameters, and framework control flags (adm_flags).  If the
 *	process is a method request, the routine also sets the process's
 *	request ID (adm_reqID), class (adm_class), method (adm_method),
 *	domain (adm_domain), standard formatted output FILE pointer
 *	(adm_stdfmt), and standard category list (adm_stdcats).
 *
 *	Upon normal completion, this routine returns ADM_SUCCESS.
 *
 *--------------------------------------------------------------------
 */

int
adm_env_init()
{
	u_int type;	/* Authentication type */
	u_int flavor;	/* Authentication flavor */
	uid_t uid;	/* Pointer to return local uid identity */
	Adm_cpn *cpn_p;	/* Pointer to pointer to cpn structure */
	int stdfmt_fd;	/* File descriptor for standard formatted output */
	int matched;
	int stat;
	char *tmpp;
	char *digp;

	if (sysinfo(SI_HOSTNAME, adm_host, (long)(MAXHOSTNAMELEN + 1)) == -1) {
		adm_host[0] = NULL;
		return(ADM_ERR_GETHOST);
	}
	adm_host[MAXHOSTNAMELEN] = NULL;

	adm_flags = ADM_DEFAULT_FLAGS;
	tmpp = getenv(ADM_FLAGS_NAME);
	if (tmpp != NULL) {
		adm_flags = strtol(tmpp, &digp, 10);
		if ((tmpp == digp) || (*digp != NULL)) {
			return(ADM_ERR_BADFLAGS);
		}
	}

	tmpp = getenv(ADM_REQUESTID_NAME);
	if (tmpp != NULL) {
		stat = adm_reqID_str2rid(tmpp, &adm_reqID, NULL);
		if (stat != ADM_SUCCESS) {
			return(ADM_ERR_BADREQID);
		}
	} else {
		adm_reqID_blank(&adm_reqID);
	}

	adm_class = getenv(ADM_CLASS_NAME);

	adm_method = getenv(ADM_METHOD_NAME);

	adm_client_id = getenv(ADM_CLIENT_ID_NAME);
	if (adm_client_id != NULL) {
	    if (adm_auth_client2(adm_client_id, &type, &flavor, &uid,
				 &cpn_p) == ADM_SUCCESS) {
		adm_auth_init_type = ADM_AUTH_UNSPECIFIED;
		adm_auth_init_flavor = flavor;
	    }
	}
	
	adm_domain = getenv(ADM_DOMAIN_NAME);

	/*
	 * Set the default timeout parameters, including maximum
	 * time-to-live for a request, timeout for a ping acknowledgement,
	 * number of times to ping an agent before assuming it has
	 * crashed, and the delay at the start of a request before 
	 * beginning pinging activities.  If this is an executing
	 * methods, these values are taken from the environment,
	 * otherwise standard defaults are used.
	 */

	adm_rep_timeout  = ADM_REP_TSECS;
	adm_ping_timeout = ADM_PING_TSECS;
	adm_ping_cnt     = ADM_PING_CNT;
	adm_ping_delay   = ADM_DELAY_TSECS;
	tmpp = getenv(ADM_TIMEOUT_PARMS_NAME);
	if (tmpp != NULL) {
		matched = sscanf(tmpp, ADM_TIMEOUTS_FMT, &adm_rep_timeout,
				&adm_ping_timeout, &adm_ping_cnt,
				&adm_ping_delay);
		if (matched != 4) {
			return(ADM_ERR_BADTOPARMS);
		}
	}

	/*
	 * Open the standard formatted output pipe (if defined), set
	 * it to be closed-on-exec, and set the pipe to be flushed at
	 * the end of each line.
	 */

	tmpp = getenv(ADM_STDFMT_NAME);
	if (tmpp != NULL) {
		digp = tmpp;
		do {			/* Verify numeric specification */
			if (!isdigit((int)(*digp))) {
				return(ADM_ERR_BADSTDFMT);
			}
		} while ((*++digp) != NULL);
		stdfmt_fd = atoi(tmpp);
		stat = fcntl(stdfmt_fd, F_SETFD, 1);
		if (stat == -1) {
			return(ADM_ERR_FCNTLFAIL);
		}
		adm_stdfmt = fdopen(stdfmt_fd, ADM_FMTMODE);
		if (adm_stdfmt == NULL) {
			return(ADM_ERR_BADSTDFMT);
		}
		setvbuf(adm_stdfmt, NULL, _IOLBF, (size_t) 0);
	} else {
		adm_stdfmt = NULL;
	}
	
	adm_stdcats = getenv(ADM_STDCATS_NAME);

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_METH_INIT():
 *
 *	Bind the pathname at which the method's internationalization
 *	text domains can be found.
 *
 *--------------------------------------------------------------------
 */

int
adm_meth_init()
{
	char *doms;	/* Working copy of list of method's text domains */
	char *cur_dom;
	char *next_dom;
	
	/*
	 * Determine the names of the i18n text domains used by the
	 * method and the path to the locale directory where they are
	 * located.
	 */

	adm_text_domains = getenv(ADM_TEXT_DOMAINS_NAME);
	adm_lpath = getenv(ADM_LPATH_NAME);
	if ((adm_lpath == NULL) || (adm_text_domains == NULL)) {
		return(ADM_SUCCESS);
	}

	/*
	 * Bind all of the method's i18n text domains to the
	 * appropriate locale directory.
	 */

	doms = strdup(adm_text_domains);
	if (doms == NULL) {
		return(ADM_ERR_NOMEM);
	}

	next_dom = doms;
	while ((cur_dom = adm_strtok(&next_dom, " \t")) != NULL) {
		bindtextdomain(cur_dom, adm_lpath);
	}

	free(doms);

	return(ADM_SUCCESS);
}

/*
 *--------------------------------------------------------------------
 *
 * ADM_MSGS_INIT():
 *
 *	Initialize the pathnames needed for the localization tools
 *	to find the framework and object manager message files.
 *
 *	NOTE: This routine is called by the class agent to initialize
 *	      localization of it's message files (as well as by the
 *	      adm_init() routine).  As a result, the call to setlocale()
 *	      should not be removed.
 *
 *--------------------------------------------------------------------
 */

int
adm_msgs_init()
{
	if (adm_msgs_inited) {
		return(ADM_SUCCESS);
	}

	(void) setlocale(LC_ALL, "");
	bindtextdomain(ADM_TEXT_DOMAIN, ADM_MSG_PATH);
	bindtextdomain(NETMGT_TEXT_DOMAIN, ADM_MSG_PATH);

	adm_msgs_inited = B_TRUE;
	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_MAKE_TOK( s1 , s2, sep_char, seps, max_length ):
 *
 *	Find the next token in a string and make a copy of it.  The string
 *	*s1 is searched until a separator from seps is found.  The preceding
 *	token is copied into the string pointed to by s2 and the separator
 *	found is copied into the string pointed to by sep_char.  In addition,
 *	*s1 is advanced to point to the character immediate following the
 *	found separator.
 *
 *	Upon normal completion, this routine returns ADM_SUCCESS.  If the
 *	token found is longer than max_length, this routine returns
 *	ADM_ERR_TOOLONG and *s1 will remain unchanged.  If no separator
 *	was found, this routine returns ADM_ERR_NOSEP and *s1 will remain
 *	unchanged. 
 *
 *---------------------------------------------------------------------
 */
int
adm_make_tok(
	char **s1,
	char *s2,
	char *sep_char,
	char *seps,
	u_int max_length
	)
{
	char *sep_ptr;		/* Pointer to first token separator in string */
	u_int tok_len;		/* Length of first token in string */

	/* Find the first token separator, first token, and token length */

	sep_ptr = strpbrk(*s1, seps);
	if (sep_ptr == NULL) {
		return(ADM_ERR_NOSEP);
	}

	tok_len = (u_int) (sep_ptr - *s1);
	if (tok_len > max_length) {
		return(ADM_ERR_TOOLONG);
	}

	/* Return the token and its separator */

	strncpy(s2, *s1, (size_t) tok_len);	/* Copy the token */
	s2[tok_len] = NULL;

	strncpy(sep_char, sep_ptr, (size_t) 1);	/* Copy the separator */
	sep_char[1] = NULL;

	/* Advance *s1 to the character following the separator */

	*s1 = (sep_ptr + 1);

	return(ADM_SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * ADM_STRTOK( spp, seps):
 *
 *	Return a pointer to the start of the next token in the string
 *	pointed to by *spp.  seps should be set to a list of valid token
 *	separator characters.   This routine replaces the first separator
 *	following the token with a null character, and resets *spp
 *	to point to the next character after the token terminator.  If
 *	there are no tokens in the string, this routine returns a NULL
 *	pointer.
 *
 *----------------------------------------------------------------------
 */

char *
adm_strtok(
	char **spp,
	char *seps
	)
{
	char *sp;		/* Start of token pointer */
	char *tp;		/* End of token pointer */
	char *np;		/* Start of next token */

	/* Skip over initial separators */
	sp = (char *)(*spp + strspn(*spp, seps));
	if (*sp == NULL) {
		return(NULL);
	}

	/* Scan string for next separator character */
	tp = (char *)(sp + strcspn(sp, seps));

	/* Determine start of next token */
	np = (char *)(tp + strspn(tp, seps));

	/* Return the first token from the string */
	*tp = NULL;
	*spp = np;
	return(sp);
}

#endif /* !_adm_fw_impl_c */

