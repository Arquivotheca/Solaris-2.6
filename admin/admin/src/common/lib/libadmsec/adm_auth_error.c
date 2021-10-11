/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.
 */

#pragma ident	"@(#)adm_auth_error.c	1.7	92/01/30 SMI"

/*
 * FILE:  adm_auth_error.c
 *
 *	Admin Framework security library routines for creating and
 *	retrieving Admin security information errors.  Routines include:
 *		adm_auth_geterr  - Retrieve most recent error message
 *		adm_auth_fmterr	 - Format an error message
 *		adm_auth_seterr  - Set an error message
 */

#include <locale.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include "adm_auth.h"
#include "adm_auth_impl.h"
#include "adm_err_impl.h"
#include "adm_err_msgs.h"


/*
 * ---------------------------------------------------------------------
 *  adm_auth_geterr - Retrieve the most recent error message.
 *	Returns 0 if there has been no error (along with a null string);
 *	otherwise, returns the error code and message
 * ---------------------------------------------------------------------
 */

int
adm_auth_geterr(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	char **emsgpp)		/* Address to contain ptr to error msg */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth info entry structure */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Do we have an error code and message? */
	if (auth_ep->errcode == 0) {
		*emsgpp = (char *)NULL;
		return (0);
	}

	*emsgpp = auth_ep->errmsgp;
	return (auth_ep->errcode);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_fmterr - Format an error message and store error in the
 *	security information handle.
 *	Returns the error code of the error being formatted.
 * ---------------------------------------------------------------------
 */

int
adm_auth_fmterr(
	Adm_auth_entry *auth_ep, /* Ptr to auth info entry structure */
	u_int  errstat,		/* Error status code */
	...)			/* Substitutible arguments */
{
	va_list vp;		/* Varargs argument list pointer */
	char  errbuff[ADM_AUTH_ERRMSG_SIZE*2];	/* Error msg buffer */
	char *msgfmt;		/* Pointer to error msg template */
	int stat;		/* Local error status code */

	stat = (int)errstat;

	/* Check for a valid Adm_auth_entry pointer */
	if (auth_ep == (Adm_auth_entry *)NULL)
		return (ADM_ERR_NULLHANDLE);

	/* Get the error message template */
	if ((msgfmt = adm_err_msg(errstat)) != (char *)NULL) {
		va_start(vp, errstat);
		vsprintf(errbuff, msgfmt, vp);
		errbuff[ADM_AUTH_ERRMSG_SIZE] = '\0';
		va_end(vp);
	} else
		sprintf(errbuff, "ERR%d", errstat);

	/* Free any previous message from handle */
	if (auth_ep->errmsgp != (char *)NULL)
		(void) free(auth_ep->errmsgp);

	/* Copy and store the error message in the security info handle */
	if ((auth_ep->errmsgp = strdup(errbuff)) == (char *)NULL)
		stat = ADM_ERR_AUTHNOMEMORY;

	/* Store the error code in the security info handle */
	auth_ep->errcode = stat;

	/* Return the error status code */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_seterr - Store error in the security information handle.
 *	Returns the error code of the error being formatted.
 * ---------------------------------------------------------------------
 */

int
adm_auth_seterr(
	Adm_auth_entry *auth_ep, /* Ptr to auth info entry structure */
	u_int  errstat,		/* Error status code */
	char  *emsgp)		/* Pointer to error message */
{
	int    stat;		/* Local status code */

	stat = (int)errstat;

	/* Check for a valid Adm_auth_entry pointer */
	if (auth_ep == (Adm_auth_entry *)NULL)
		return (ADM_ERR_NULLHANDLE);

	/* Free any previous message from handle */
	if (auth_ep->errmsgp != (char *)NULL)
		(void) free(auth_ep->errmsgp);

	/* If the error message is NULL, simply store a null pointer */
	if (emsgp == (char *)NULL)
		auth_ep->errmsgp = (char *)NULL;
	else if ((auth_ep->errmsgp = strdup(emsgp)) == (char *)NULL)
		stat = ADM_ERR_AUTHNOMEMORY;

	/* Store and return the original error status code */
	auth_ep->errcode = stat;
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_err2str - Format an error message and return the address of
 *	the allocated memory containing the message.
 *	Returns the error code of the error being formatted.
 * ---------------------------------------------------------------------
 */

int
adm_auth_err2str(
	char **emsgpp,		/* Ptr to contain address of error msg */
	u_int  errstat,		/* Error status code */
	...)			/* Substitutible arguments */
{
	va_list vp;		/* Varargs argument list pointer */
	char  errbuff[ADM_AUTH_ERRMSG_SIZE*2];	/* Error msg buffer */
	char *msgfmt;		/* Pointer to error msg template */
	u_int stat;		/* Local error status code */

	stat = (int)errstat;

	/* Check for a valid argument to return message pointer */
	if (emsgpp == (char **)NULL)
		return (stat);

	/* Get the error message template */
	if ((msgfmt = adm_err_msg(errstat)) != (char *)NULL) {
		va_start(vp, errstat);
		vsprintf(errbuff, msgfmt, vp);
		errbuff[ADM_AUTH_ERRMSG_SIZE] = '\0';
		va_end(vp);
	} else
		sprintf(errbuff, "ERR%d", errstat);

	/* Copy and store the error message */
	if ((*emsgpp = strdup(errbuff)) == (char *)NULL)
		stat = ADM_ERR_AUTHNOMEMORY;

	/* Return the original error status code */
	return (stat);
}
