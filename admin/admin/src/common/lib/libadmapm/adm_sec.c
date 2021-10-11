
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _adm_sec_c
#define _adm_sec_c

/*
 * FILE:  adm_sec.h
 *
 *	Admin Framework general purpose security information functions
 *	exported to methods.
 */

#pragma	ident	"@(#)adm_sec.c	1.5	92/02/28 SMI"

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

/* Declare internal function prototypes */
static int cvt_str2cpn(Adm_cpn *cpnp, char *buff, u_int type);

/*
 * -----------------------------------------------------------------------
 * ADM_AUTH_CLIENT (u_int *type, u_int *flavor, u_int *uid, Adm_cpn **cpn_pp)
 *
 *	Reads the ADM_CLIENT_ID environment variable to obtain the client
 *	authentication identity and the authentication type and flavor used.
 *	Returns the authentication type, authentication flavor, local domain
 *	uid identity, and client's common principal name identity.
 *	Returns 0 (Client identity returned)	>0 (Error number)
 * -----------------------------------------------------------------------
 */

int
adm_auth_client(
	u_int *type,		/* Authentication type */
	u_int *flavor,		/* Authentication flavor */
	uid_t *uid,		/* Pointer to return local uid identity */
	Adm_cpn **cpn_pp)	/* Pointer to pointer to cpn structure */
{
	Adm_cpn *cpnp;		/* Pointer to admin cpn structure */
	char  *sp;		/* String pointer */
	char  *tp;		/* String pointer */
	long  lval;		/* Long integer conversion value */
	u_int ctype;		/* Cpn type */
	int   stat;		/* Return status code */

	/* Make sure initialized */
	stat = adm_init();
	if (stat != ADM_SUCCESS)
		return (stat);

	/* Check the client identity environment variable */
	if (adm_client_id == (char *)NULL)
		return (ADM_ERR_NOVALUE);

	return (adm_auth_client2(adm_client_id, type, flavor, uid, cpn_pp));
} 

/*
 * -----------------------------------------------------------------------
 * ADM_CPN_CPN2STR (Adm_cpn *cpn_p, char *buffer, u_int buffsize)
 *
 *	Converts an Adm_cpn structure into a cpn string format.
 *	Moves the cpn string into the caller's buffer up to buffsize bytes
 *	uid identity, and client's common principal name identity.
 *	Returns 0 (Conversion successful)	>0 (Error number)
 * -----------------------------------------------------------------------
 */

int
adm_cpn_cpn2str(
	Adm_cpn *cpnp,		/* Pointer to cpn structure */
	char    *buff,		/* Buffer for cpn string */
	u_int	size)		/* Size of buffer for cpn string */
{
	char	tbuff[ADM_AUTH_CPN_NAMESIZE+1];	/* Temp buffer */
	uid_t	no_uid;		/* Missing uid check value */
	u_int	len;		/* Temp length */

	/* Make sure we have valid arguments */
	if (cpnp == (Adm_cpn *)NULL) 
		return (ADM_ERR_BADCPN);

	/* Check type of cpn */
	switch (cpnp->type) {
	case ADM_CPN_USER:
		no_uid = (uid_t)UID_NOBODY;
		break;
	case ADM_CPN_GROUP:
		no_uid = (uid_t)GID_NOBODY;
		break;
	case ADM_CPN_OTHER:
		no_uid = (uid_t)UID_NOBODY;
		break;
	default:
		return (ADM_ERR_BADCPN);
	}

	/* Format the string into a temporary buffer */
	*tbuff = '\0';
	if (cpnp->id != no_uid)
		sprintf(tbuff, "(%ld)", (long)cpnp->id);
	if (cpnp->name != (char *)NULL)
		strcat(tbuff, cpnp->name);
	if (cpnp->role != (char *)NULL) {
		strcat(tbuff, ".");
		strcat(tbuff, cpnp->role);
	}
	if (cpnp->domain != (char *)NULL) {
		strcat(tbuff, "@");
		strcat(tbuff, cpnp->domain);
	}

	/* Move the cpn string into the caller's buffer */
	len = (u_int)strlen(tbuff);
	len++;					/* Account for \0 */
	if (len > size)
		len = size;
	if ((buff != (char *)NULL) && (len > 0)) {
		len--;				/* Move all but \0 */
		strncpy(buff, tbuff, len);
		buff[len] = '\0';		/* Now add the \0 */
	}

	/* Return with status */
	return (ADM_SUCCESS);
}

/*
 * -----------------------------------------------------------------------
 * ADM_CPN_FREE (Adm_cpn *cpn_p)
 *
 *	Frees an Adm_cpn structure.
 *	Returns 0 (Free successful)	>0 (Error number)
 * -----------------------------------------------------------------------
 */

int
adm_cpn_free(
	Adm_cpn *cpnp)		/* Pointer to cpn structure */
{
	int	stat;		/* Return status code */

	/* Make sure we got a valid argument */
	if (cpnp == (Adm_cpn *)NULL)
		return (ADM_SUCCESS);

	/* Validate cpn structure and free it up */
	stat = ADM_SUCCESS;
	switch (cpnp->type) {
	case ADM_CPN_NONE:
		free(cpnp);
		break;
	case ADM_CPN_USER:
	case ADM_CPN_GROUP:
	case ADM_CPN_OTHER:
		if (cpnp->name != (char *)NULL) {
			free(cpnp->name);
			cpnp->name = (char *)NULL;
		}
		if (cpnp->role != (char *)NULL) {
			free(cpnp->role);
			cpnp->role = (char *)NULL;
		}
		if (cpnp->domain != (char *)NULL) {
			free(cpnp->domain);
			cpnp->domain = (char *)NULL;
		}
		free(cpnp);
		break;
	default:
		stat = ADM_ERR_BADCPN;
		break;
	}						/* End of switch */

	/* Return status */
	return (stat);
}

/*
 * -----------------------------------------------------------------------
 * CVT_STR2CPN (Adm_cpn *cpnp, char *buff, u_int type)
 *
 *	Converts a cpn string format into an Adm_cpn structure.
 *	Returns 0 (Conversion successful)	>0 (Error number)
 * -----------------------------------------------------------------------
 */

int
cvt_str2cpn(
	Adm_cpn *cpnp,		/* Pointer to cpn structure */
	char    *buff,		/* Buffer containing cpn string */
	u_int	type)		/* Type of cpn */
{
	char	tbuff[ADM_AUTH_CPN_NAMESIZE+1];	/* Temp buffer */
	uid_t	no_uid;		/* Missing id value */
	char	*np;		/* Pointer to name string */
	char	*rp;		/* Pointer to role string */
	char	*dp;		/* Pointer to domain string */
	char	*tp;		/* Pointer to string */
	char	*xp;		/* Pointer to string */
	long	lval;		/* Integer conversion buffer */

	/* Set up null id value */
	switch (type) {
	case ADM_CPN_USER:
		no_uid = (uid_t)UID_NOBODY;
		break;
	case ADM_CPN_GROUP:
		no_uid = (uid_t)GID_NOBODY;
		break;
	case ADM_CPN_OTHER:
		no_uid = (uid_t)UID_NOBODY;
		break;
	default:
		return (ADM_ERR_BADCPN);
	}

	/* Copy string into temp buffer and find separators */
	strncpy(tbuff, buff, (u_int)ADM_AUTH_CPN_NAMESIZE);
	tbuff[ADM_AUTH_CPN_NAMESIZE] = '\0';
	if ((dp = strchr(tbuff, '@')) != (char *)NULL)
		*dp++ = '\0';
	if ((rp = strchr(tbuff, '.')) != (char *)NULL)
		*rp++ = '\0';

	/* Convert id portion of string */
	tp = tbuff;
	if (*tp == '(') {
		tp++;
		lval = strtol(tp, &xp, (u_int)10);
		if (tp == xp)
			return (ADM_ERR_BADCPN);
		cpnp->id = (uid_t)lval;
		tp = xp;
		if (*tp++ != ')')
			return (ADM_ERR_BADCPN);
	} else {
		cpnp->id = no_uid;
		if (*tp == '\0')
			return (ADM_ERR_BADCPN);
	}
 
	/* Look for a name string */
	if (*tp != '\0')
		np = tp;
	else
		np = (char *)NULL;

	/* Move string parts into cpn */
	if (np != (char *)NULL)
		if ((cpnp->name = strdup(np)) == (char *)NULL)
			return (ADM_ERR_NOMEM);
	if (rp != (char *)NULL)
		if ((cpnp->role = strdup(rp)) == (char *)NULL)
			return (ADM_ERR_NOMEM);
	if (dp != (char *)NULL)
		if ((cpnp->domain = strdup(dp)) == (char *)NULL)
			return (ADM_ERR_NOMEM);

	/* Finally, set the type of the cpn */
	cpnp->type = type;

	/* Return success */
	return (ADM_SUCCESS);
}

#endif /* !_adm_sec_c */

