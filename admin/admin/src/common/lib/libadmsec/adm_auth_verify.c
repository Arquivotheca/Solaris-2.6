/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.
 */

#pragma ident	"@(#)adm_auth_verify.c	1.9	92/03/03 SMI"

/*
 * FILE:  adm_auth_access.c
 *
 *	Admin Framework high level security library routines for verifying
 *	Admin security information.  Routines include:
 *		adm_auth_verinfo - Validate security info in security handle
 *		adm_auth_valacl	 - Validate ACL in internal structure format
 *		adm_auth_sortacl - Sort ACL entries
 *		adm_auth_valauth - Validate authentication type in handle
 *		adm_auth_valsid  - Validate set identities in handle
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
 *  adm_auth_verinfo - Verify security info in a security handle.
 *	Validate's the internal ACL, set identity, and authentication
 *	type security information in the security handle.
 *	Returns 0 if info is validated; otherwise, returns an error code.
 * ---------------------------------------------------------------------
 */

int
adm_auth_verinfo(Adm_auth_handle_t *auth_hp) /* Ptr to security info handle */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth info entry structure */
	Adm_auth *ap;		/* Local pointer to security info struct */
	Adm_acl *aclp;		/* Local pointer to ACL structure */
	char *emsgp;		/* Local pointer to error message */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Set some local pointers */
	ap = &auth_ep->auth_info;
	aclp = &ap->acl;

	/* Validate the ACL. Return if errors. */
	if ((stat = adm_auth_valacl(aclp, (u_int)ADM_AUTH_OPT_SETACL, &emsgp))
	    != 0) {
		adm_auth_seterr(auth_ep, stat, emsgp);
		return (stat);
	}

	/* Validate authentication type and flavors. Return if errors. */
	if ((stat = adm_auth_valauth(ap->auth_type, ap->auth_flavor[0],
	    &emsgp))
	    != 0) {
		adm_auth_fmterr(auth_ep, stat, emsgp);
		return (stat);
	}

	/* Validate set identity uid and gid.  Return if errors. */
	if ((stat = adm_auth_valsid(ap->set_flag, ap->set_uid, ap->set_gid,
	    &emsgp)) != 0) {
		adm_auth_fmterr(auth_ep, stat, emsgp);
		return (stat);
	}

	/* If authentication type is NONE and set identity CLIENT, error */
	if ((ap->auth_type == ADM_AUTH_NONE) &&
	    (ap->set_flag & (ADM_SID_CLIENT_UID|ADM_SID_CLIENT_GID))) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADTYPEANDSID);
	}

	/* Return success */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_valacl - Validate an ACL structure.
 *	Checks that the ACL has a valid flag field, has valid entries for
 *	the first five "standard" entries, has valid specific user entries
 *	(and the uid's exist on the system or in the domain), and has
 *	valid groups entries (and the gid's exist on the system or in the
 *	domain).  This routine also sorts the ACL entries and computes
 *	the maximum permissions of the groups entries (the "class" entry).
 *	Returns 0 if ACL is valid; otherwise, returns an error code and
 *	error message.
 * ---------------------------------------------------------------------
 */

int
adm_auth_valacl(
	Adm_acl *aclp,		/* Pointer to Admin ACL structure */
	u_int  option,		/* Validation options */
	char **emsgpp)		/* Address to contain ptr to error msg */
{
	struct Adm_acl_entry *tep;	/* Temp ptr to ACL entry struct */
	struct Adm_acl_entry *lep;	/* Temp ptr to ACL entry struct */
	struct passwd *pwbuffp;	/* Pointer to passwd file entry */
	struct group *grbuffp;	/* Pointer to group file entry */
	char   ebuff[ADM_AUTH_ACLENTRY_SIZE]; /* Buff for ACL entry conv */
	char  *tp;		/* ACL type name pointer */
	ushort mask_flag;	/* Flag indicating mask entry present */
	u_int  del_flag;	/* Flag indicating delete verify request */
	u_int  count;		/* Count of duplicate ACL entries */
	u_int  size;		/* Size of ACL entry conversion buffer */
	u_int  i;		/* Temporary for loop counter */
	int    stat;		/* Local status code */

	/* Check for a valid Adm_acl pointer */
	if (aclp == (struct Adm_acl *)NULL) {
		stat = adm_auth_err2str(emsgpp, ADM_ERR_NOACLINFO);
		return (stat);
	}

	/* Check for a valid ACL flag value */
	if (aclp->flags & (~ADM_ACL_OFF)) {
		stat = adm_auth_err2str(emsgpp, ADM_ERR_BADACLFLAG,
		    aclp->flags);
		return (stat);
	}

	/* If request from delete, won't lookup uid's and gid's */
	del_flag = 0;
	if (option & ADM_AUTH_OPT_DELACL)
		del_flag = 1;

	/* Check for non-zero number of entries and sort the entries */
	if ((aclp->number_entries == 0) ||
	    (aclp->number_entries > ADM_ACL_MAXENTRIES)) {
		stat = adm_auth_err2str(emsgpp, ADM_ERR_BADACLNUMENTRY,
		    aclp->number_entries);
		return (stat);
	}
	adm_auth_sortacl(aclp);

	/*
	 * Walk down the ACL entries.  Validate each entry according to
	 * its type, checking for valid types and permissions along the
	 * way.  For each user type, make sure the uid exists.  For each
	 * group type, make sure the gid exists.  While we're at it, lets
	 * check for duplicate entries:  if the duplicates have the
	 * same permissions, simply throw one of them away (reset its
	 * type to a high value, we'll resort the entries later); if
	 * the duplicates have different permissions, report an error.
	 * Count the number of valid duplicates, as this will trigger
	 * the resort and reduction of the number of entries.
	 */

	stat = 0;
	count = 0;
	mask_flag = 0;
	for (i = 0; i < aclp->number_entries; i++) {
		tep = &aclp->entry[i];
		size = (u_int)ADM_AUTH_ACLENTRY_SIZE;
		switch (tep->type) {

		/* Check entry type and identifier */
		case ADM_ACL_OWNER:			/* User entry */
		case ADM_ACL_USERS:
			if (del_flag)
				break;
			pwbuffp = getpwuid((uid_t)tep->id);
			if (pwbuffp == (struct passwd *)NULL) {
				(void) adm_auth_ace2str((u_int)0, tep, ebuff,
				    &size);
				stat = adm_auth_err2str(emsgpp,
				    ADM_ERR_BADACLUID, ebuff);
				return (stat);
			}
			break;
		case ADM_ACL_GROUP:			/* Group entry */
		case ADM_ACL_GROUPS:
			if (del_flag)
				break;
			grbuffp = getgrent();		/* Reset groups */
			grbuffp = getgrgid((gid_t)tep->id);
			if (grbuffp == (struct group *)NULL) {
				(void) adm_auth_ace2str((u_int)0, tep, ebuff,
				    &size);
				stat = adm_auth_err2str(emsgpp,
				    ADM_ERR_BADACLGID, ebuff);
				return (stat);
			}
			break;
		case ADM_ACL_MASK:			/* Mask entry */
			mask_flag++;			/* Fall through */
		case ADM_ACL_OTHER:			/* Special entry */
		case ADM_ACL_NOBODY:
			if ((u_long)tep->id != 0) {
				(void) adm_auth_ace2str((u_int)0, tep, ebuff,
				    &size);
				stat = adm_auth_err2str(emsgpp,
				    ADM_ERR_BADACLNOID, ebuff);
				return (stat);
			}
			break;
		default:				/* Invalid type */
			(void) adm_auth_ace2str((u_int)0, tep, ebuff, &size);
			stat = adm_auth_err2str(emsgpp, ADM_ERR_BADACLTYPE,
				ebuff);
			return (stat);
			break;
		}					/* End of switch */

		/* Type and id valid.  Now check entry permissions */
		if (tep->permissions & (~ADM_ACL_RWX)) {
			(void) adm_auth_ace2str((u_int)0, tep, ebuff, &size);
			stat = adm_auth_err2str(emsgpp, ADM_ERR_BADACLPERM,
				ebuff);
			return (stat);
		}

		/* Entry valid.  Check for a duplicate entry */
		if (i > 0) {
			lep = &aclp->entry[i - 1];
			if ((lep->type == tep->type) && (lep->id == tep->id))
				if (lep->permissions == tep->permissions) {
					lep->type += ADM_ACL_DEAD;
					count++;
				} else {
					(void) adm_auth_ace2str((u_int)0, tep,
					    ebuff, &size);
					stat = adm_auth_err2str(emsgpp,
					    ADM_ERR_ACLDUPENTRY, ebuff);
					return (stat);
				}
		}
	}						/* End of for */

	/* ACL is valid.  If a set ACL and no mask entry, create mask */
	if ((option & ADM_AUTH_OPT_SETACL) && (mask_flag == 0)) {
		if (aclp->number_entries < ADM_ACL_MAXENTRIES) {
			tep = &aclp->entry[aclp->number_entries];
			tep->type = ADM_ACL_MASK;
			tep->id = (uid_t)0;
			tep->permissions = ADM_ACL_RWX;
			aclp->number_entries += 1;
		} else
			mask_flag++;
	} else
		mask_flag++;

	/* If valid duplicates or added mask entry, resort the ACL */
	if ((count > 0) || (mask_flag == 0)) {
		(void) adm_auth_sortacl(aclp);
		aclp->number_entries -= count;
	}

	/* If this is a "set" ACL, check for valid special entries */
	tp = (char *)NULL;
	if (option & ADM_AUTH_OPT_SETACL) {
		if (aclp->entry[ADM_ACL_OWNER_OFFSET].type != ADM_ACL_OWNER) {
			stat = adm_auth_err2str(emsgpp, ADM_ERR_ACLMISSTYPE,
			    ADM_ACL_OWNER_NAME);
			return (stat);
		}
		if (aclp->entry[ADM_ACL_GROUP_OFFSET].type != ADM_ACL_GROUP) {
			stat = adm_auth_err2str(emsgpp, ADM_ERR_ACLMISSTYPE,
			    ADM_ACL_GROUP_NAME);
			return (stat);
		}
		if (aclp->entry[ADM_ACL_OTHER_OFFSET].type != ADM_ACL_OTHER) {
			stat = adm_auth_err2str(emsgpp, ADM_ERR_ACLMISSTYPE,
			    ADM_ACL_OTHER_NAME);
			return (stat);
		}
		if (aclp->entry[ADM_ACL_NOBODY_OFFSET].type != ADM_ACL_NOBODY) {
			stat = adm_auth_err2str(emsgpp, ADM_ERR_ACLMISSTYPE,
			    ADM_ACL_NOBODY_NAME);
			return (stat);
		}
		if (aclp->entry[ADM_ACL_MASK_OFFSET].type != ADM_ACL_MASK) {
			stat = adm_auth_err2str(emsgpp, ADM_ERR_ACLMISSTYPE,
			    ADM_ACL_MASK_NAME);
			return (stat);
		}
	}

	/* Return valid ACL status */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_sortacl - Sort the ACL entries in the ACL structure.
 *	Sorts the ACL entries first by type, then by identifier.
 *	Returns zero if no errors; otherwise, returns error status -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_sortacl(Adm_acl *aclp)	/* Pointer to ACL structure */
{
	Adm_acl_entry *iep;	/* Temporary ACL entry pointer */
	Adm_acl_entry *jep;	/* Temporary ACL entry pointer */
	Adm_acl_entry *kep;	/* Temporary ACL entry pointer */
	Adm_acl_entry *lep;	/* Temporary ACL entry pointer */
	Adm_acl_entry *tep;	/* Temporary ACL entry pointer */
	Adm_acl_entry  tentry;	/* Temporary ACL entry structure */
	u_int  i;		/* Loop counter */

	/* Check for a valid Adm_acl structure pointer */
	if (aclp == (struct Adm_acl *)NULL)
		return (-1);

	/* If less than two entries, don't bother sorting */
	if (aclp->number_entries < 2)
		return (0);

	/* In many cases, we may already be in sort order. Do quick check */
	lep = &aclp->entry[aclp->number_entries];
	iep = &aclp->entry[0];
	jep = iep++;
	for (; jep < lep; iep++) {
		if ((iep->type > jep->type) ||
		    ((iep->type == jep->type) && (iep->id > jep->id)))
			break;
		jep++;
	}						/* End of for */
	if (jep != lep) {

		/* Use simple bubble sort; don't expect too many entries */
		tep = &tentry;
		for (iep = &aclp->entry[0]; iep < (lep - 1); iep++) {
			kep = iep;
			for (jep = (iep + 1); jep < lep; jep++) {
				if ((kep->type > jep->type) ||
				    ((kep->type == jep->type) &&
				    (kep->id > jep->id)))
					kep = jep;
			}				/* End inner for */
			if (kep != iep) {		/* Switch entries */
				tep->type = iep->type;
				tep->id = iep->id;
				tep->permissions = iep->permissions;
				iep->type = kep->type;
				iep->id = kep->id;
				iep->permissions = kep->permissions;
				kep->type = tep->type;
				kep->id = tep->id;
				kep->permissions = tep->permissions;
			}
		}					/* End outer for */
	}

	/*
	 * Reset array position of first groups entry, if any exist.
	 * If none exist, set offset to number of entries.
	 */

	iep = &aclp->entry[0];
	for (i = 0; i < aclp->number_entries; i++, iep++) {
		if (iep->type == ADM_ACL_GROUPS)
			break;
	}
	aclp->groups_offset = i;

	/* Return success */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_valauth - Validate authentication type and flavor.
 *	Checks type for one of our valid types, and flavor, if it exists,
 *	for one of SecureRPC valid flavors.
 *	Returns 0 if type and flavor are valid; otherwise, returns -1;
 * ---------------------------------------------------------------------
 */

int
adm_auth_valauth(
	u_int  authtype,	/* Authentication type */
	u_int  authflavor,	/* Authentication flavor */
	char **emsgpp)		/* Address to store ptr to error message */
{
	u_int t_type;		/* Local auth type */
	u_int t_num;		/* Local number flavors */
	u_int t_flavors[ADM_AUTH_MAXFLAVORS]; /* Local flavor array */
	int   stat;		/* Local status code */

	/* Validate authentication type */
	stat = 0;
	t_num = ADM_AUTH_MAXFLAVORS;
	if ((adm_auth_chktype(authtype, &t_num, t_flavors)) !=
	    ADM_AUTH_OK) {
		stat = adm_auth_err2str(emsgpp, ADM_ERR_BADAUTHTYPE,
		    authtype);
		return (stat);
	}

	/* Validate authentication flavor */
	if (authflavor == ADM_AUTH_UNSPECIFIED)
		return (0);

	if ((adm_auth_chkflavor(authflavor, &t_type)) != ADM_AUTH_OK) {
		stat = adm_auth_err2str(emsgpp, ADM_ERR_BADAUTHFLAVOR,
		    authflavor);
		return (stat);
	}

	/* Check for valid combination of type and flavor */
	if (authtype != t_type) {
		stat = adm_auth_err2str(emsgpp, ADM_ERR_BADAUTHFLAVOR,
		    authflavor);
		return (stat);
	}

	/* Return with status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_valsid - Validate set identitities.
 *	Checks that the set identity flag is valid, and if specific
 *	user and/or group identities are specified, they are valid on the
 *	local system or NIS domain.
 *	Returns 0 if id's are valid; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_valsid(
	u_int  setflag,		/* Set identity flag */
	uid_t  setuid,		/* Set identity user id */
	gid_t  setgid,		/* Set identity group id */
	char **emsgpp)		/* Address to contain ptr to error msg */
{
	struct passwd *pwbuff;	/* Password entry buffer */
	struct group *grbuff;	/* Group entry buffer */
	int   stat;		/* Local status code */

	/* Validate set identity flag */
	stat = 0;
	if (setflag != 0) {
		if (setflag == (ADM_SID_CLIENT_UID|ADM_SID_CLIENT_GID))
			stat = 0;
		else if (setflag == (ADM_SID_AGENT_UID|ADM_SID_AGENT_GID))
			stat = 0;
		else {
			stat = adm_auth_err2str(emsgpp, ADM_ERR_BADSIDFLAG,
			    setflag);
			return (stat);
		}
	}

	/* Validate user uid */
	if (! (setflag & (ADM_SID_CLIENT_UID|ADM_SID_AGENT_UID))) {
		pwbuff = getpwuid(setuid);
		if (pwbuff == (struct passwd *)NULL) {
			stat = adm_auth_err2str(emsgpp, ADM_ERR_BADSIDUID,
			    setuid);
			return (stat);
		}
	}

	/* Validate group gid */
	if (! (setflag & (ADM_SID_CLIENT_GID|ADM_SID_AGENT_GID))) {
		grbuff = getgrent();		/* Reset groups */
		grbuff = getgrgid(setgid);
		if (grbuff == (struct group *)NULL) {
			stat = adm_auth_err2str(emsgpp, ADM_ERR_BADSIDGID,
			    setgid);
			return (stat);
		}
	}

	/* Return with status */
	return (stat);
}
