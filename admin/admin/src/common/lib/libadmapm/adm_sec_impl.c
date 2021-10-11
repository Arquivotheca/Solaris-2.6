
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _adm_sec_impl_c
#define _adm_sec_impl_c

/*
 * FILE:  adm_sec_impl.h
 *
 *	Admin Framework general purpose security information functions
 *	used by AMCL and class agent.
 */

#pragma	ident	"@(#)adm_sec_impl.c	1.4	92/02/28 SMI"

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
 * CVT_STR2CPN (Adm_cpn *cpnp, char *buff, u_int type)
 *
 *	Converts a cpn string format into an Adm_cpn structure.
 *	Returns 0 (Conversion successful)	>0 (Error number)
 * -----------------------------------------------------------------------
 */

static
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

/*
 * -----------------------------------------------------------------------
 * ADM_AUTH_CHKFLAVOR (u_int flavor, u_int *type)
 *
 *	Check for a valid authentication flavor and return type.
 *	Accepts an authentication flavor value and sets the corresponding
 *	authentication type.
 *	Returns 0 (Flavor OK)	-1 (Flavor bogus)
 * -----------------------------------------------------------------------
 */

int
adm_auth_chkflavor(
	u_int flavor,		/* Authentication flavor */
	u_int *type)		/* Pointer to authentication type */
{
	int status = 0;

	switch (flavor)	{
		case AUTH_NONE: 
			*type = ADM_AUTH_NONE;
			break;
		case AUTH_UNIX:
			*type = ADM_AUTH_WEAK;	
			break;
		case AUTH_DES:
			*type = ADM_AUTH_STRONG;
			break;
		default:
			*type = ADM_AUTH_NONE;
			status = -1;
	}
	return (status);
} 

/*
 * -----------------------------------------------------------------------
 * ADM_AUTH_CHKTYPE (u_int type, u_int *number_flavors, u_int flavor[])
 *
 *	Check for a valid authentication type and return set of supported
 *	authentication flavors for that type.
 *	Accepts an authentication type value and sets the array of
 *	authentication flavors supported for that type.
 *	Returns 0 (type OK)	-1 (type bogus)
 * -----------------------------------------------------------------------
 */

int
adm_auth_chktype(
	u_int auth_type,	/* Authentication type */
	u_int *num_flavor,	/* Pointer to number of flavors in array */
	u_int flavor[])		/* Authentication flavor array */
{
	int status = 0;

	switch (auth_type)	{
		case ADM_AUTH_NONE: 
			if (*num_flavor >= 1)	{
				*num_flavor = 1;
				flavor[0] = AUTH_NONE;
			}
			break;
		case ADM_AUTH_WEAK:
			if (*num_flavor >= 1)	{
				*num_flavor = 1;
				flavor[0] = AUTH_UNIX;
			}
			break;
		case ADM_AUTH_STRONG:
			if (*num_flavor >= 1)	{
				*num_flavor = 1;
				flavor[0] = AUTH_DES;
			}
			break;
		default:
			if (*num_flavor >= 1)	{
				*num_flavor = 1;
				flavor[0] = AUTH_NONE;
			}
			status = -1;
	}
	return (status);
} 

/*
 * -----------------------------------------------------------------------
 * ADM_AUTH_CLIENT2 (char *env_string, u_int *type, u_int *flavor,
 *		     u_int *uid, Adm_cpn **cpn_pp)
 *
 *	Work routine for adm_auth_client().  Parses the string env_string
 *	(which should be the value of the ADM_CLIENT_ID environment
 *	variable) to obtain the client authentication identity and the
 *	authentication type and flavor used.  Returns the authentication
 *	type, authentication flavor, local domain uid identity, and
 *	client's common principal name identity.  Returns 0 (Client
 *	identity returned)	>0 (Error number)
 * -----------------------------------------------------------------------
 */

int
adm_auth_client2(
	char *env_string,	/* String to parse */
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

	/* Assume the best */
	stat = ADM_SUCCESS;

	/* Decode the fields in the client identity environment variable */
	sp = env_string;
	lval = strtol(sp, &tp, (u_int)10);		/* Auth type */
	if (tp == sp)
		return (ADM_ERR_BADCLIENTID);
	*type = (u_int)lval;
	sp = tp;
	if (*sp++ != ':')
		return (ADM_ERR_BADCLIENTID);
	lval = strtol(sp, &tp, (u_int)10);		/* Auth flavor */
	if (tp == sp)
		return (ADM_ERR_BADCLIENTID);
	*flavor = (u_int)lval;
	sp = tp;
	if (*sp++ != ':')
		return (ADM_ERR_BADCLIENTID);
	lval = strtol(sp, &tp, (u_int)10);		/* Local uid */
	if (tp == sp)
		return (ADM_ERR_BADCLIENTID);
	*uid = (uid_t)lval;
	sp = tp;
	if (*sp++ != ':')
		return (ADM_ERR_BADCLIENTID);
	lval = strtol(sp, &tp, (u_int)10);		/* Cpn type */
	if (tp == sp)
		return (ADM_ERR_BADCLIENTID);
	ctype = (u_int)lval;
	sp = tp;
	if (*sp++ != ':')
		return (ADM_ERR_BADCLIENTID);

	/* Allocate an Adm_cpn structure and set its values */
	if ((cpnp = malloc(sizeof (struct Adm_cpn))) == (Adm_cpn *)NULL)
		return (ADM_ERR_NOMEM);
	cpnp->type = (u_int)ADM_CPN_NONE;
	cpnp->id = (uid_t)UID_NOBODY;
	cpnp->name = (char *)NULL;
	cpnp->role = (char *)NULL;
	cpnp->domain = (char *)NULL;
	if ((stat = cvt_str2cpn(cpnp, sp, ctype)) != ADM_SUCCESS) {
		(void) adm_cpn_free(cpnp);
		return (stat);
	}
	*cpn_pp = cpnp;

	/* Return success */
	return (stat);
} 

/*
 * ---------------------------------------------------------------------
 * ADM_AUTH_TYPE2STR (char *typename, u_int namesize, u_int authtype)
 *
 *	Convert an authentication type to its name.
 *	Returns 0 if type can be converted; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_type2str(
	char  *authtype_name,	/* Authentication type name */
	u_int authtype_length,	/* Maximum size of name buffer */
	u_int auth_type)	/* Authentication type field */
{
	char  tbuff[16];	/* Conversion buffer */
	int   stat;		/* Local status code */
 
	/* Make sure output arguments are good */
	if ((authtype_name == (char *)NULL) || (authtype_length == 0))
		return (-1);

	/* Convert and validate authentication type */
	stat = 0;
	switch (auth_type) {
	case ADM_AUTH_NONE:
		strncpy(authtype_name, ADM_AUTH_NONE_NAME, authtype_length);		break;
	case ADM_AUTH_WEAK:
		strncpy(authtype_name, ADM_AUTH_WEAK_NAME, authtype_length);		break;
	case ADM_AUTH_STRONG:
		strncpy(authtype_name, ADM_AUTH_STRONG_NAME, authtype_length);
		break;
	default:
		sprintf(tbuff, "%d", auth_type);
		strncpy(authtype_name, tbuff, authtype_length);
		stat = -1;
		break;
	}
	authtype_name[authtype_length - 1] = '\0';
 
	/* Return with status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 * ADM_AUTH_FLAVOR2STR (char *flavorname, u_int namesize, u_int flavor)
 *
 *	Convert an authentication flavor to its name.
 *	Returns 0 if flavor can be converted; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_flavor2str(
	char  *authflavor_name,	/* Authentication flavor name */
	u_int authflavor_length, /* Maximum size of name buffer */
	u_int auth_flavor)	/* Authentication flavor field */
{
	char  tbuff[16];	/* Conversion buffer */
	int   stat;		/* Local status code */
 
	/* Make sure output arguments are good */
	if ((authflavor_name == (char *)NULL) || (authflavor_length == 0))
		return (-1);

	/* Convert and validate authentication flavor */
	stat = 0;
	switch (auth_flavor) {
	case AUTH_NONE:
		strncpy(authflavor_name, ADM_AUTH_NONE_NAME, authflavor_length);		break;
	case AUTH_UNIX:
		strncpy(authflavor_name, ADM_AUTH_UNIX_NAME, authflavor_length);		break;
	case AUTH_DES:
		strncpy(authflavor_name, ADM_AUTH_DES_NAME, authflavor_length);
		break;
	default:
		sprintf(tbuff, "%d", auth_flavor);
		strncpy(authflavor_name, tbuff, authflavor_length);
		stat = -1;
		break;
	}
	authflavor_name[authflavor_length - 1] = '\0';
 
	/* Return with status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 * ADM_AUTH_STR2TYPE (char *typename, u_int *type)
 *
 *	Convert an authentication type name to internal type.
 *	Returns 0 if name can be converted; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_str2type(
	char  *authtype_name,	/* Authentication type name */
	u_int *auth_type)	/* Pointer to auth type field */
{
	int   stat;		/* Local status code */
 
	/* Convert and validate authentication type */
	stat = 0;
	if (authtype_name == (char *)NULL)
		return (-1);
	if (! (strcmp(authtype_name, "")))
		return (-1);
	if (! (strcmp(authtype_name, ADM_AUTH_NONE_NAME)))
		*auth_type = ADM_AUTH_NONE;
	else if (! (strcmp(authtype_name, ADM_AUTH_WEAK_NAME)))
		*auth_type = ADM_AUTH_WEAK;
	else if (! (strcmp(authtype_name, ADM_AUTH_STRONG_NAME)))
		*auth_type = ADM_AUTH_STRONG;
	else
		stat = -1;

	/* Return status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 * ADM_AUTH_STR2FLAVOR ( char *flavorname, u_int *flavor)
 *
 *	Convert an authentication flavor name to internal flavor.
 *	Returns 0 if name can be converted; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_str2flavor(
	char  *authflavor_name,	/* Authentication type name */
	u_int *auth_flavor)	/* Pointer to auth type field */
{
	int   stat;		/* Local status code */
 
	/* Convert and validate authentication flavor */
	stat = 0;
	if (authflavor_name == (char *)NULL)
		return (-1);
	if (! (strcmp(authflavor_name, "")))
		return (-1);
	if (! (strcmp(authflavor_name, ADM_AUTH_NONE_NAME)))
		*auth_flavor = AUTH_NONE;
	else if (! (strcmp(authflavor_name, ADM_AUTH_UNIX_NAME)))
		*auth_flavor = AUTH_UNIX;
	else if (! (strcmp(authflavor_name, ADM_AUTH_DES_NAME)))
		*auth_flavor = AUTH_DES;
	else
		stat = -1;

	/* Return with status */
	return (stat);
}

#endif /* !_adm_sec_impl_c */

