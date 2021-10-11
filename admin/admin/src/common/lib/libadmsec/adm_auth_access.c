/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.
 */

#pragma ident	"@(#)adm_auth_access.c	1.14	92/02/21 SMI"

/*
 * FILE:  adm_auth_access.c
 *
 *	Admin Framework high level security library routines for getting
 *	and storing Admin security information.  Routines include:
 *		adm_auth_newh	 - Allocate security info handle
 *		adm_auth_freeh	 - Release security info handle
 *		adm_auth_getinfo - Retrieve security info for a method
 *		adm_auth_putinfo - Store security info for a method
 *		adm_auth_delinfo - Delete security info for a method
 *		adm_auth_getacl	 - Return ACL in character format
 *		adm_auth_setacl	 - Set character format ACL in handle
 *		adm_auth_getauth - Return authentication type
 *		adm_auth_setauth - Set authentication type in handle
 *		adm_auth_getsid	 - Return set identity username & groupname
 *		adm_auth_setsid	 - Set username & groupname set identity
 *		adm_auth_getname - Return name of security entry (method)
 */

#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include "adm_auth.h"
#include "adm_auth_impl.h"
#include "adm_err_msgs.h"

/*
 * ---------------------------------------------------------------------
 *  adm_auth_newh - Create a new Admin security information entry structure.
 *	Allocates a new Admin_auth structure, initializes it to null, and
 *	allocates a handle to contain its pointer (as well as error info).
 *	Returns 0 if structure is allocated; otherwise, returns an error
 *	 code.
 * ---------------------------------------------------------------------
 */

Adm_auth_handle_t *
adm_auth_newh(void)
{
	Adm_auth_handle_t *auth_hp; /* Ptr to Admin security info handle */
	Adm_auth_entry *auth_ep;    /* Ptr to Admin security entry struct */
	Adm_auth *authp;	/* Pointer to Admin security info struct */

	/* Allocate and initialize an Adm_auth_entry structure */
	if ((auth_ep =
	    (struct Adm_auth_entry *)malloc(sizeof (struct Adm_auth_entry)))
	    != (struct Adm_auth_entry *)NULL) {
		auth_hp = (Adm_auth_handle_t *)auth_ep;
		(void) adm_auth_clearh(auth_hp);
	} else
		return ((Adm_auth_handle_t *)NULL);

	/* Return */
	return (auth_hp);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_freeh - Free an Admin security information entry structure.
 *	Frees all structures and memory associated with an Admin security
 *	information handle, then frees the handle structure itself.
 *	There is no return status from this function.
 * ---------------------------------------------------------------------
 */

void
adm_auth_freeh(Adm_auth_handle_t *auth_hp) /* Ptr to security info handle */
{
	Adm_auth_entry *auth_ep;	/* Ptr to auth info entry struct */

	/* Cast opaque security info handle pointer to auth entry pointer */
	auth_ep = (Adm_auth_entry *)auth_hp;

	/* Release the memory associated with an Adm_auth_h handle */
	if (auth_ep != (Adm_auth_entry *)NULL) {
		if (auth_ep->errmsgp != (char *)NULL)
			(void) free(auth_ep->errmsgp);
		(void) free(auth_ep);
	}

	/* Close the passwd and group files, in case they were opened */
	(void) endpwent();
	(void) endgrent();

	/* Return without status */
	return;
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_clearh - Clear an Admin security information entry structure.
 *	Frees all structures and memory associated with an Admin security
 *	information handle, then frees the handle structure itself.
 *	There is no return status from this function.
 * ---------------------------------------------------------------------
 */

void
adm_auth_clearh(Adm_auth_handle_t *auth_hp) /* Ptr to security info handle */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth info entry structure */
	Adm_auth *authp;	/* Local pointer to auth info structure */

	/* Cast opaque security info handle pointer to auth entry pointer */
	auth_ep = (Adm_auth_entry *)auth_hp;

	/* Clear the security info entry buffer. */
	if (auth_ep != (Adm_auth_entry *)NULL) {
		auth_ep->errcode = 0;
		if (auth_ep->errmsgp != (char *)NULL)
			(void) free(auth_ep->errmsgp);
		auth_ep->errmsgp = (char *)NULL;
		auth_ep->name_length = ADM_AUTH_NAMESIZE;
		auth_ep->auth_length = ADM_AUTH_MAXSIZE;
		authp = &auth_ep->auth_info;
		(void) memset((char *)authp, (int)0, (int)ADM_AUTH_MAXSIZE);
		authp->version = (u_int)ADM_AUTH_VERSION;
		authp->auth_type = ADM_AUTH_NONE;
		authp->number_flavors = 1;
		authp->auth_flavor[0] = AUTH_NONE;
	}

	/* Return */
	return;
}


/*
 * ---------------------------------------------------------------------
 *  adm_auth_getname - Retrieve security entry name.
 *	Returns a pointer to the entry name if successful; otherwise,
 *	returns a null pointer.
 * ---------------------------------------------------------------------
 */

char *
adm_auth_getname(
	Adm_auth_handle_t *auth_hp) /* Ptr to Admin security info handle */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth entry structure */
	char *entry_name;	 /* Security entry name */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return ((char *)NULL);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Return entry name; checking for special entries */
	entry_name = auth_ep->name;
	if (! (strcmp(entry_name, ADM_AUTH_CLASS_INFO)))
		entry_name = ADM_AUTH_CLASS_NAME;
	else if (! (strcmp(entry_name, ADM_AUTH_DEFAULT_INFO)))
		entry_name = ADM_AUTH_DEFAULT_NAME;

	/* Return entry name */
	return (entry_name);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_getinfo - Retrieve security info for an entry in a class.
 *	Retrieves a method's internal ACL, set identity, and authentication
 *	type security information into the specified Admin auth structure.
 *	Uses the low level security library routines to get the info.
 *	Returns 0 if info is retrieved; otherwise, returns an error code.
 * ---------------------------------------------------------------------
 */

int
adm_auth_getinfo(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	char *class_name,	/* Name of class */
	char *class_vers,	/* Version of class */
	char *method_name,	/* Name of method */
	u_int options)		/* Retrieval options flag */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth entry structure */
	char *method;		/* Local method name */
	char *vers;		/* Local class version */
	int   tflag;		/* Temporary flag */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Initialize our local variables */
	method = method_name;
	vers = class_vers;
	if (class_vers == (char *)NULL)
		vers = "";

	/* Check the options for special cases */
	tflag = -1;
	if (options != 0) {
		if (options == ADM_AUTH_DEFAULT)	/* default entry */
			method = ADM_AUTH_DEFAULT_INFO;
		else if (options == ADM_AUTH_CLASS)	/* class entry */
			method = ADM_AUTH_CLASS_INFO;
		else if (options == ADM_AUTH_FIRST) {	/* first entry */
			tflag = ADM_AUTH_FIRSTACL;
			method = ADM_AUTH_FIRST_NAME;
		} else if (options == ADM_AUTH_NEXT) {	/* next entry */
			tflag = 0;
			method = ADM_AUTH_NEXT_NAME;
		} else {
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADOPTION);
			return (stat);
		}
	}

	/* Clear security info entry buffer */
	(void) adm_auth_clearh((Adm_auth_handle_t *)auth_ep);

	/* Retrieve the auth info for the given class entry */
	if (tflag < 0)
		stat = adm_auth_read(class_name, class_vers, method,
		    auth_ep);
	else
		stat = adm_auth_getnext(class_name, class_vers,
		    (u_int)tflag, auth_ep);
	if (stat != 0) {
		if (auth_ep->errmsgp == (char *)NULL)
			(void) adm_auth_fmterr(auth_ep, ADM_ERR_AUTHREAD,
			    stat, class_name, vers, method);
		return (stat);
	}

	/* Return success */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_putinfo - Store security info for a method in a class.
 *	Writes the ACL structure, authentication type, and set identity
 *	info back to the class security data store.
 *	Uses the low level security library routines to store the ACL.
 *	Returns 0 if ACL is stored; otherwise, returns an error code.
 * ---------------------------------------------------------------------
 */

int
adm_auth_putinfo(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	char *class_name,	/* Name of class */
	char *class_vers,	/* Version of class */
	char *method_name,	/* Name of method */
	u_int	options)	/* Storage options flag */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth entry structure */
	Adm_auth *ap;		/* Local auth info structure pointer */
	Adm_acl *aclp;		/* Local ACL structure pointer */
	char *method;		/* Local method name */
	char *vers;		/* Local class version */
	u_int nument;		/* Local number of ACL entries */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Initialize our local variables */
	method = method_name;
	vers = class_vers;
	if (class_vers == (char *)NULL)
		vers = "";

	/* Check the options for special cases */
	if (options != 0) {
		if (options == ADM_AUTH_DEFAULT)	/* class default */
			method = ADM_AUTH_DEFAULT_INFO;
		else if (options == ADM_AUTH_CLASS)	/* class itself */
			method = ADM_AUTH_CLASS_INFO;
		else {
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADOPTION);
			return (stat);
		}
	}

	/* Validate security info. Return if errors. */
	if ((stat = adm_auth_verinfo((Adm_auth_handle_t *)auth_ep)) != 0) {
		return (stat);
	}

	/*  Store the security info for the given class method */
	if ((stat = adm_auth_write(class_name, class_vers, method, auth_ep))
	    != 0) {
		if (auth_ep->errmsgp == (char *)NULL)
			(void) adm_auth_fmterr(auth_ep, ADM_ERR_AUTHWRITE,
			    stat, class_name, vers, method);
		return (stat);
	}

	/* Return success */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_delinfo - Delete security info for a method in a class.
 *	Uses the low level security library routines to remove the info.
 *	Returns 0 if info is removed; otherwise, returns an error code.
 * ---------------------------------------------------------------------
 */

int
adm_auth_delinfo(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	char *class_name,	/* Name of class */
	char *class_vers,	/* Version of class */
	char *method_name,	/* Name of method */
	u_int options)		/* Retrieval options flag */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth entry structure */
	char *method;		/* Local method name */
	char *vers;		/* Local class version */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Initialize our local variables */
	method = method_name;
	vers = class_vers;
	if (class_vers == (char *)NULL)
		vers = "";

	/* Check the options for special cases */
	if (options != 0) {
		if (options == ADM_AUTH_DEFAULT)	/* class default */
			method = ADM_AUTH_DEFAULT_INFO;
		else if (options == ADM_AUTH_CLASS)	/* class itself */
			method = ADM_AUTH_CLASS_INFO;
		else {
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADOPTION);
			return (stat);
		}
	}

	/* Clear security info entry buffer */
	(void) adm_auth_clearh((Adm_auth_handle_t *)auth_ep);

	/* Delete the auth info for the given class method */
	if ((stat = adm_auth_delete(class_name, class_vers, method,
	    auth_ep)) != 0) {
		if (auth_ep->errmsgp == (char *)NULL)
			(void) adm_auth_fmterr(auth_ep, ADM_ERR_AUTHREAD,
			    stat, class_name, vers, method);
		return (stat);
	}

	/* Return success */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_getacl - Retrieve ACL from the security info of a security
 *	handle.
 *	Checks the existence of an ACL structure in the security handle,
 *	and converts that ACL structure into a character format ACL.
 *	Returns 0 if ACL exists and is converted successfully; otherwise,
 *	returns an error code.
 * ---------------------------------------------------------------------
 */

int
adm_auth_getacl(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	char  *aclstr,		/* Buffer to contain char format ACL */
	u_int *size,		/* Length of char ACL buffer on input */
				/* Length of ACL string on output */
	u_int *numentries,	/* Number of ACL entries on output */
	u_int *foptions,	/* ACL flag options on output */
	u_int  options)		/* Retrieval options flag */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth info entry structure */
	Adm_auth *ap;		/* Local pointer to auth info structure */
	Adm_acl *aclp;		/* Local pointer to ACL structure */
	char *emsgp;		/* Local error message pointer */
	u_int mask;		/* Local options mask */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Check for valid character format ACL arguments */
	if ((aclstr == (char *)NULL) || (size == (u_int *)NULL)) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADARGUMENT);
		return (stat);
	}

	/* Check the options for special cases */
	if (options != 0) {
		mask = (ADM_AUTH_FULLIDS|ADM_AUTH_LONGTYPE|ADM_AUTH_NOIDS);
		if ((options | mask) != mask) {
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADOPTION);
			return (stat);
		}
	}

	/* Set some local pointers */
	ap = &auth_ep->auth_info;
	aclp = &ap->acl;

	/* Check that a non-empty ACL exists */
	if (aclp->number_entries == 0) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_NOACLINFO);
		return (stat);
	}

	/* Convert the ACL to string format */
	if ((stat = adm_auth_acl2str(options, aclp, aclstr, size, &emsgp))
	    != 0)
		adm_auth_seterr(auth_ep, stat, emsgp);

	/* Return number of entries and options */
	if ((stat == 0) || (stat == ADM_ERR_ACLTOOBIG)) {
		*numentries = aclp->number_entries;
		*foptions = 0;
		if (aclp->flags & ADM_ACL_OFF)
			*foptions = ADM_AUTH_ACLOFF;
	}

	/* Return with status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_setacl - Set (replace) ACL in the security info of a security
 *	handle.
 *	Converts the character format ACL input argument into an internal
 *	ACL structure, validates the ACL, and associates the new ACL
 *	structure with the specified security handle.
 *	Any previous ACL structure is overlayed in the security info.
 *	Returns 0 if ACL is converted successfully; otherwise,
 *	returns an error code and the previous ACL in the security info
 *	remains intact.
 * ---------------------------------------------------------------------
 */

int
adm_auth_setacl(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	char  *aclstr,		/* Buffer containing char format ACL */
	u_int size,		/* Length of string ACL buffer on input */
	u_int *numentries,	/* Number of ACL entries on output */
	u_int  options)		/* Options flag */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth info entry structure */
	Adm_auth *ap;		/* Local pointer to auth info structure */
	Adm_acl *aclp;		/* Local pointer to ACL structure */
	char *emsgp;		/* Local pointer to error message */
	u_int flag;		/* Local ACL flag value */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Check for valid character format ACL arguments */
	if ((aclstr == (char *)NULL) || (size == 0)) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADARGUMENT);
		return (stat);
	}

	/* Check the options for special cases */
	flag = (int)0;
	if (options != 0) {
		if (options == ADM_AUTH_ACLOFF) { /* ACL checking off */
			flag = (int)ADM_ACL_OFF;
		} else {
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADOPTION);
			return (stat);
		}
	}

	/* Set some local pointers */
	ap = &auth_ep->auth_info;

	/* Allocate memory for local maximum sized ACL */
	if ((aclp = (struct Adm_acl *)malloc((size_t)ADM_ACL_MAXSIZE))
	    == (struct Adm_acl *)NULL){
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_AUTHNOMEMORY);
		return (stat);
	}

	/* Convert the char format ACL to an internal ACL structure */
	if ((stat = adm_auth_str2acl((u_int)0, aclp, aclstr, size, &emsgp))
	    != 0) {
		adm_auth_seterr(auth_ep, stat, emsgp);
		(void) free(aclp);
		return (stat);
	}
	aclp->flags = flag;

	/* Validate that the ACL structure we built is valid */
	if ((stat = adm_auth_valacl(aclp, (u_int)ADM_AUTH_OPT_SETACL, &emsgp))
	    != 0) {
		adm_auth_seterr(auth_ep, stat, emsgp);
		(void) free(aclp);
		return (stat);
	}

	/* OK, got a good ACL.  Copy over previous ACL */
	(void) memcpy(&ap->acl, aclp, (size_t)ADM_ACL_MAXSIZE);

	/* Return number of entries */
	*numentries = aclp->number_entries;

	/* Return with status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_getauth - Retrieve authentication type and flavor from the
 *	security info of a security handle.
 *	Converts internal authentication info to string names.
 *	Returns 0 if type is converted successfully; otherwise,
 *	returns an error code.
 * ---------------------------------------------------------------------
 */

int
adm_auth_getauth(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	char *authtype_name,	/* Buffer to contain auth type */
	u_int authtype_length,	/* Length of auth type buffer */
	char *authflavor_name,	/* Buffer to contain auth flavor */
	u_int authflavor_length) /* Length of auth flavor buffer */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth info entry structure */
	Adm_auth *ap;		/* Local pointer to auth info structure */
	u_int t_flavor;		/* Local auth flavor */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Check for valid auth type buffer and length */
	if ((authtype_name == (char *)NULL) || (authtype_length == 0)) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADARGUMENT);
		return (stat);
	}

	/* Check for valid auth flavor buffer and length */
	if ((authflavor_name == (char *)NULL) || (authflavor_length == 0)) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADARGUMENT);
		return (stat);
	}

	/* Set some local pointers */
	ap = &auth_ep->auth_info;

	/* Convert and validate authentication type */
	if ((stat = adm_auth_type2str(authtype_name, authtype_length,
	    ap->auth_type)) != 0) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADAUTHTYPE,
		    ap->auth_type);
		return (stat);
	}

	/* Convert and validate authentication flavor */
	if (ap->number_flavors == 0)
		t_flavor = ADM_AUTH_UNSPECIFIED;
	else
		t_flavor = ap->auth_flavor[0];
	if (t_flavor != ADM_AUTH_UNSPECIFIED) {
		if ((stat = adm_auth_flavor2str(authflavor_name,
		    authflavor_length, t_flavor)) != 0) {
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADAUTHFLAVOR,
			    t_flavor);
			return (stat);
		}
	} else
		authflavor_name = "";

	/* Return success */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_setauth - Set authentication type and flavor in the security
 *	info of a security handle.
 *	Converts external authentication type and flavor strings to
 *	internal type and flavor.  Checks that flavor matches type.
 *	Flavor is optional.
 *	Returns 0 if type is converted successfully; otherwise,
 *	returns an error code.
 * ---------------------------------------------------------------------
 */

int
adm_auth_setauth(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	char *authtype_name,	/* Authentication type name */
	char *authflavor_name)	/* Authentication flavor name */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth info entry structure */
	Adm_auth *ap;		/* Local ptr to security info structure */
	u_int t_type;		/* Local authentication type */
	u_int t_flavor;		/* Local authenticaiton flavor */
	char *emsgp;		/* Pointer to error message */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Check for valid authentication type argument */
	if (authtype_name == (char *)NULL) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADARGUMENT);
		return (stat);
	}

	/* Set some local pointers */
	ap = &auth_ep->auth_info;

	/* Convert and validate authentication type */
	if ((stat = adm_auth_str2type(authtype_name, &t_type)) != 0) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADCHARAUTHTYPE,
		    authtype_name);
		return (stat);
	}

	/* Convert and validate authentication flavor */
	stat = 0;
	if (authflavor_name == (char *)NULL)
		t_flavor = ADM_AUTH_UNSPECIFIED;
	else if (! (strcmp(authflavor_name, "")))
		t_flavor = ADM_AUTH_UNSPECIFIED;
	else if ((stat = adm_auth_str2flavor(authflavor_name, &t_flavor))
	    != 0) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADCHARAUTHFLAV,
		    authflavor_name);
		return (stat);
	}

	/* If flavor other than NONE, make sure consistent with type */
	if ((stat = adm_auth_valauth(t_type, t_flavor, &emsgp)) != 0) {
		stat = adm_auth_seterr(auth_ep, stat, emsgp);
		return (stat);
	}

	/*  Looks good.  Store type and flavor in our auth structure */
	ap->auth_type = t_type;
	ap->auth_flavor[0] = t_flavor;
	ap->number_flavors = 1;

	/* Return with status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_getsid - Retrieve set identitities from the security info
 *	of a security handle.
 *	Converts uid and gid into username and groupname, respectively.
 *	Returns 0 if id's are converted successfully; otherwise,
 *	returns an error code.
 *
 *	!!!WARNING!!!
 *
 *	This routine assumes the character buffers are large enough
 *	to contain a username and groupname, respectively.
 * ---------------------------------------------------------------------
 */

int
adm_auth_getsid(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	u_int *sidflag,		/* Set identity options on output */
	char  *username,	/* Buffer to contain user name */
	u_int  usersize,	/* User buffer length */
	char  *groupname,	/* Buffer to contain group name */
	u_int  groupsize,	/* Group buffer length */
	u_int  options)		/* Options */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth info entry structure */
	Adm_auth *ap;		/* Local ptr to security info structure */
	struct passwd pwbuff;	/* Password entry buffer */
	struct group grbuff;	/* Group entry buffer */
	char  tbuff[32];	/* Temporary conversion buffer */
	int   temp;		/* Temporary integer */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Check for valid return arguments */
	if ((username == (char *)NULL) || (groupname == (char *)NULL)) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADARGUMENT);
		return (stat);
	}

	/* Check the options for special cases */
	if (options != 0) {
		if (options != ADM_AUTH_NOIDS) {	/* no ids */
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADOPTION);
			return (stat);
		}
	}

	/* Set some local pointers */
	ap = &auth_ep->auth_info;

	/* Check for a valid uid and convert it to a username or uid */
	stat = 0;
	if (! (ap->set_flag & (ADM_SID_CLIENT_UID|ADM_SID_AGENT_UID))) {
		if (adm_auth_uid2str(username, usersize, ap->set_uid) != 0)
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADUID,
			    (long)ap->set_uid);
	}

	/* Check for a valid gid and convert it to a username */
	temp = 0;
	if (! (ap->set_flag & (ADM_SID_CLIENT_GID|ADM_SID_AGENT_GID))) {
		if ((adm_auth_gid2str(groupname, groupsize, ap->set_gid))
		    != 0)
			temp = adm_auth_fmterr(auth_ep, ADM_ERR_BADGID,
			    (long)ap->set_gid);
	}

	/* Set the set identity options returned */
	*sidflag = 0;
	if (ap->set_flag & (ADM_SID_CLIENT_UID|ADM_SID_CLIENT_GID))
		*sidflag = ADM_AUTH_CLIENT;
	if (ap->set_flag & (ADM_SID_AGENT_UID|ADM_SID_AGENT_GID))
		*sidflag = ADM_AUTH_AGENT;

	/* Set return status from either uid or gid conversion results */
	if (stat == 0)
		stat = temp;

	/* Return with status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_setsid - Set the set identitities in the security info
 *	of a security handle.
 *	Converts username and groupname into uid and gid, respectively.
 *	Returns 0 if id's are converted and validated successfully;
 *	otherwise, returns an error code.
 * ---------------------------------------------------------------------
 */

int
adm_auth_setsid(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	char  *username,	/* Buffer to contain user name */
	char  *groupname,	/* Buffer to contain group name */
	u_int  options)		/* Options */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth info entry structure */
	Adm_auth *ap;		/* Local ptr to security info structure */
	struct passwd pwbuff;	/* Password entry buffer */
	struct group grbuff;	/* Group entry buffer */
	uid_t  t_uid;		/* Local uid */
	gid_t  t_gid;		/* Local gid */
	char  *emsgp;		/* Local error message pointer */
	char  *tp;		/* Temporary string pinter */
	u_int  cvtopt;		/* Conversion options */
	int    stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Check the options for special cases */
	cvtopt = 0;
	if (options != 0) {
		if (options & ADM_AUTH_CLIENT)		/* use client id */
			cvtopt = ADM_SID_CLIENT_UID + ADM_SID_CLIENT_GID;
		else if (options & ADM_AUTH_AGENT)	/* use agent id */
			cvtopt = ADM_SID_AGENT_UID + ADM_SID_AGENT_GID;
		else {
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADOPTION);
			return (stat);
		}
	}

	/* Set some local pointers */
	ap = &auth_ep->auth_info;

	/* Check for a valid username and convert it to a uid */
	t_uid = (uid_t)0;
	if (! (cvtopt & (ADM_SID_CLIENT_UID|ADM_SID_AGENT_UID))) {
		if (username == (char *)NULL) {
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADUSERNAME,
			    "(nil)");
			return (stat);
		}
		if (adm_auth_str2uid(username, &t_uid) != 0) {
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADUSERNAME,
			    username);
			return (stat);
		}
	}

	/* Check for a valid groupname and convert it to a uid */
	t_gid = (gid_t)0;
	if (! (cvtopt & (ADM_SID_CLIENT_GID|ADM_SID_AGENT_GID))) {
		if (groupname == (char *)NULL) {
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADGROUPNAME,
			    "(nil)");
			return (stat);
		}
		if (adm_auth_str2gid(groupname, &t_gid) != 0) {
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADGROUPNAME,
			    groupname);
			return (stat);
		}
	}

	/* Validate uid and gid, make sure uid is in gid group */
	if ((stat = adm_auth_valsid(cvtopt, t_uid, t_gid, &emsgp)) == 0) {
		ap->set_flag = cvtopt;
		ap->set_uid = t_uid;
		ap->set_gid = t_gid;
	} else
		adm_auth_seterr(auth_ep, stat, emsgp);

	/* Return with status */
	return (stat);
}
