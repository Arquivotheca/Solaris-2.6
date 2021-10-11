/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.
 */

#pragma ident	"@(#)adm_auth_update.c	1.11	92/02/28 SMI"

/*
 * FILE:  adm_auth_update.c
 *
 *	Admin Framework high level security library routines for updating
 *	Admin security information access control lists.  Routines include:
 *		adm_auth_addacl  - Add new ACL entries to an ACL
 *		adm_auth_modacl  - Modify ACL entries in an ACL
 *		adm_auth_delacl	 - Delete ACL entries from an ACL
 *		adm_auth_clracl	 - Clear users and groupts ACL entries
 *		adm_auth_rstacl	 - Set/reset flag in an ACL
 */

#include <string.h>
#include <sys/types.h>
#include "adm_auth.h"
#include "adm_auth_impl.h"
#include "adm_err_msgs.h"

/*
 * ---------------------------------------------------------------------
 *  adm_auth_addacl - Add new ACL entries to the ACL in the security info
 *	of a security handle.
 *	Converts the character format ACL input argument into an internal
 *	ACL structure, validates that ACL's entries, and merges the new
 *	ACL entries into the existing ACL structure.
 *	Returns 0 if ACL is converted and merged successfully; otherwise,
 *	returns an error code and the previous ACL in the security info
 *	remains intact.
 * ---------------------------------------------------------------------
 */

int
adm_auth_addacl(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	char  *aclstr)		/* Buffer containing char format ACL */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth entry structure */
	Adm_auth *ap;		/* Local pointer to security info struct */
	Adm_acl *o_aclp;	/* Local pointer to old ACL structure */
	Adm_acl *a_aclp;	/* Local pointer to add ACL structure */
	Adm_acl *n_aclp;	/* Local pointer to new ACL structure */
	struct Adm_acl_entry *aep;	/* Local ptr to ACL entry */
	struct Adm_acl_entry *sep;	/* Local ptr to ACL entry */
	char *emsgp;		/* Local pointer to error message */
	u_int tot_entries;	/* Total old and new ACL entries */
	u_int size;		/* Size of char format ACL string */
	u_int i;		/* Temporary loop counter */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Check for valid character format ACL arguments */
	if (aclstr == (char *)NULL) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADARGUMENT);
		return (stat);
	}
	size = (u_int)strlen(aclstr);

	/* Set some local pointers */
	ap = &auth_ep->auth_info;
	o_aclp = &ap->acl;

	/* Allocate a temporary ACL structure for the add ACL entries */
	if ((a_aclp = (struct Adm_acl *)malloc((size_t)ADM_ACL_MAXSIZE))
	    == (struct Adm_acl *)NULL) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_AUTHNOMEMORY);
		return (stat);
	}
	(void) memset((char *)a_aclp, (int)0, (int)ADM_ACL_MAXSIZE);

	/* Convert the char format ACL to an internal ACL structure */
	if ((stat = adm_auth_str2acl((u_int)0, a_aclp, aclstr, size, &emsgp))
	    != 0) {
		adm_auth_seterr(auth_ep, stat, emsgp);
		(void) free(a_aclp);
		return (stat);
	}

	/* Validate that the ACL structure we converted is valid */
	if ((stat = adm_auth_valacl(a_aclp, (u_int)0, &emsgp))
	    != 0) {
		adm_auth_seterr(auth_ep, stat, emsgp);
		(void) free(a_aclp);
		return (stat);
	}

	/* Allocate a new ACL structure and copy old ACL into it */
	if ((n_aclp = (struct Adm_acl *)malloc((size_t)ADM_ACL_MAXSIZE))
	    == (struct Adm_acl *)NULL) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_AUTHNOMEMORY);
		(void) free(a_aclp);
		return (stat);
	}
	(void) memcpy(n_aclp, o_aclp, (size_t)ADM_ACL_MAXSIZE);

	/*
	 * Merge the add ACL entries with the old ACL into the new ACL.
	 * We do this by adding the ACL entries to the end of the new ACL
	 * structure, then re-validating the resultant new ACL.  If valid,
	 * we win.  If not, must have been some sort of duplicate or
	 * missing special entry, so just throw away the new ACL structure.
	 */

	if ((tot_entries = a_aclp->number_entries + n_aclp->number_entries)
	    > ADM_ACL_MAXENTRIES) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_ACLTOOBIG);
		(void) free(a_aclp);
		(void) free(n_aclp);
		return (stat);
	}
	aep = &a_aclp->entry[0];
	sep = &n_aclp->entry[n_aclp->number_entries];
	for (i = 0; i < a_aclp->number_entries; i++) {
		sep->type = aep->type;
		sep->id = aep->id;
		sep->permissions = aep->permissions;
		aep++;
		sep++;
	}						/* End of for */
	n_aclp->number_entries = tot_entries;
	if ((stat = adm_auth_valacl(n_aclp, (u_int)ADM_AUTH_OPT_SETACL, &emsgp))
	    != 0) {
		adm_auth_seterr(auth_ep, stat, emsgp);
		(void) free(a_aclp);
		(void) free(n_aclp);
		return (stat);
	}

	/* Resultant ACL is good. Copy into the security info ACL */
	(void) memcpy(o_aclp, n_aclp, (size_t)ADM_ACL_MAXSIZE);
	(void) free(a_aclp);
	(void) free(n_aclp);

	/* Return success */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_modacl - Modify entries in the ACL in the security info
 *	of a security handle.
 *	Converts the character format ACL input argument into an internal
 *	ACL structure, validates that ACL's entries, and moves the new
 *	ACL entries into the existing ACL structure replacing the
 *	matching entry.
 *	Returns 0 if ACL is converted and merged successfully; otherwise,
 *	returns an error code and the previous ACL in the security info
 *	remains intact.
 * ---------------------------------------------------------------------
 */

int
adm_auth_modacl(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	char *aclstr)		/* Buffer containing char format ACL */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth entry structure */
	Adm_auth *ap;		/* Local pointer to security info struct */
	Adm_acl *o_aclp;	/* Local pointer to old ACL structure */
	Adm_acl *a_aclp;	/* Local pointer to mod ACL structure */
	Adm_acl *n_aclp;	/* Local pointer to new ACL structure */
	struct Adm_acl_entry *aep;	/* Local ptr to ACL entry */
	struct Adm_acl_entry *sep;	/* Local ptr to ACL entry */
	char  tbuff[ADM_AUTH_ACLENTRY_SIZE]; /* Buff for ACl entry error */
	char *emsgp;		/* Local pointer to error message */
	u_int tot_entries;	/* Total old and new ACL entries */
	u_int size;		/* Size of char format ACL string */
	u_int i;		/* Temporary loop counter */
	u_int j;		/* Temporary loop counter */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Check for valid character format ACL arguments */
	if (aclstr == (char *)NULL) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADARGUMENT);
		return (stat);
	}
	size = (u_int)strlen(aclstr);

	/* Set some local pointers */
	ap = &auth_ep->auth_info;
	o_aclp = &ap->acl;

	/* Allocate a temporary ACL structure for the add ACL entries */
	if ((a_aclp = (struct Adm_acl *)malloc((size_t)ADM_ACL_MAXSIZE))
	    == (struct Adm_acl *)NULL) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_AUTHNOMEMORY);
		return (stat);
	}
	(void) memset((char *)a_aclp, (int)0, (int)ADM_ACL_MAXSIZE);

	/* Convert the char format ACL to an internal ACL structure */
	if ((stat = adm_auth_str2acl((u_int)0, a_aclp, aclstr, size, &emsgp))
	    != 0) {
		adm_auth_seterr(auth_ep, stat, emsgp);
		(void) free(a_aclp);
		return (stat);
	}

	/* Validate that the ACL structure we converted is valid */
	if ((stat = adm_auth_valacl(a_aclp, (u_int)0, &emsgp))
	    != 0) {
		adm_auth_seterr(auth_ep, stat, emsgp);
		(void) free(a_aclp);
		return (stat);
	}

	/* Allocate a new ACL structure and copy old ACL into it */
	if ((n_aclp = (struct Adm_acl *)malloc((size_t)ADM_ACL_MAXSIZE))
	    == (struct Adm_acl *)NULL) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_AUTHNOMEMORY);
		(void) free(a_aclp);
		return (stat);
	}
	(void) memcpy(n_aclp, o_aclp, (size_t)ADM_ACL_MAXSIZE);

	/*
	 * Merge the change ACL entries with the old ACL entries in the new
	 * ACL structure.  We do this by looking for each change entry in
	 * the new ACL.  If found, simply replace with the new entry.
	 * If not found, return an error and throw away the new ACL.
	 */

	aep = &a_aclp->entry[0];
	for (i = 0; i < a_aclp->number_entries; i++) {

		/* Find matching entry based on type */
		switch (aep->type) {
		case ADM_ACL_OWNER:
			j = ADM_ACL_OWNER_OFFSET;
			break;
		case ADM_ACL_GROUP:
			j = ADM_ACL_GROUP_OFFSET;
			break;
		case ADM_ACL_OTHER:
			j = ADM_ACL_OTHER_OFFSET;
			break;
		case ADM_ACL_NOBODY:
			j = ADM_ACL_NOBODY_OFFSET;
			break;
		case ADM_ACL_MASK:
			j = ADM_ACL_MASK_OFFSET;
			break;
		case ADM_ACL_USERS:
			j = ADM_ACL_USERS_OFFSET;
			sep = &n_aclp->entry[j];
			for (; j < n_aclp->groups_offset; j++, sep++) {
				if (aep->type == sep->type)
					if (aep->id == sep->id)
						break;
			}				/* End for loop */
			if (j == n_aclp->groups_offset)
				j = n_aclp->number_entries;
			break;
		case ADM_ACL_GROUPS:
			j = n_aclp->groups_offset;
			sep = &n_aclp->entry[j];
			for (; j < n_aclp->number_entries; j++, sep++) {
				if (aep->type == sep->type)
					if (aep->id == sep->id)
						break;
			}				/* End for loop */
			break;
		default:
			j = n_aclp->number_entries;
		}					/* End of switch */

		/* If found entry, reset identifier and permissions */
		if (j < n_aclp->number_entries) {
			sep = &n_aclp->entry[j];
			sep->id = aep->id;
			sep->permissions = aep->permissions;
		}

		/* If entry not found, set missing entry error */
		else {
			size = (u_int)ADM_AUTH_ACLENTRY_SIZE;
			(void) adm_auth_ace2str((u_int)0, aep, tbuff, &size);
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_ACLMISSENTRY,
			    tbuff);
			(void) free(a_aclp);
			(void) free(n_aclp);
			return (stat);
		}
		aep++;
	}						/* End outer for */

	/* Resultant ACL is good. Copy into the security info ACL */
	(void) memcpy(o_aclp, n_aclp, (size_t)ADM_ACL_MAXSIZE);
	(void) free(a_aclp);
	(void) free(n_aclp);

	/* Return success */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_delacl - delete ACL entries from the ACL in the security info
 *	of a security handle.
 *	Converts the character format ACL input argument into an internal
 *	ACL structure, validates that ACL's entries, and merges the new
 *	ACL entries into the existing ACL structure, deleting matching
 *	ACL entries.
 *	Returns 0 if ACL is converted and merged successfully; otherwise,
 *	returns an error code.  If error is not fatal, continues to
 *	update the existing ACL.
 * ---------------------------------------------------------------------
 */

int
adm_auth_delacl(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	char *aclstr)		/* Buffer containing char format ACL */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth entry structure */
	Adm_auth *ap;		/* Local pointer to auth info structure */
	Adm_acl *aclp;		/* Local pointer to ACL structure */
	Adm_acl *a_aclp;	/* Local pointer to add ACL structure */
	struct Adm_acl_entry *aep; /* Local ptr to aux ACL entry */
	struct Adm_acl_entry *sep; /* Local ptr to set ACL entry */
	struct Adm_acl_entry *uep; /* Local ptr to first users entry */
	struct Adm_acl_entry *gep; /* Local ptr to first groups entry */
	char  tbuff[ADM_AUTH_ACLENTRY_SIZE]; /* Buff for ACl entry error */
	u_int  size;		/* Size of char format ACL string */
	u_int  fu;		/* ACL array pos of first users entry */
	u_int  lu;		/* ACL array pos of last users entry */
	u_int  fg;		/* ACL array pos of first groups entry */
	u_int  lg;		/* ACL array pos of last groups entry */
	char *emsgp;		/* Local pointer to error message */
	u_int i;		/* Temporary loop counter */
	u_int j;		/* Temporary loop counter */
	u_int count;		/* Count of deleted entries */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Check for valid character format ACL arguments */
	if (aclstr == (char *)NULL) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADARGUMENT);
		return (stat);
	}
	size = (u_int)strlen(aclstr);

	/* Set some local pointers */
	ap = &auth_ep->auth_info;
	aclp = &ap->acl;

	/* Allocate a temporary ACL structure for the new ACL */
	if ((a_aclp = (struct Adm_acl *)malloc((size_t)ADM_ACL_MAXSIZE))
	    == (struct Adm_acl *)NULL) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_AUTHNOMEMORY);
		return (stat);
	}
	(void) memset((char *)a_aclp, (int)0, (int)ADM_ACL_MAXSIZE);

	/* Convert the char format ACL to an internal ACL structure */
	if ((stat = adm_auth_str2acl((u_int)0, a_aclp, aclstr, size, &emsgp))
	    != 0) {
		adm_auth_seterr(auth_ep, stat, emsgp);
		(void) free(a_aclp);
		return (stat);
	}

	/* Validate that the ACL structure we converted is valid */
	if ((stat = adm_auth_valacl(a_aclp, (u_int)ADM_AUTH_OPT_DELACL,
	    &emsgp)) != 0) {
		adm_auth_seterr(auth_ep, stat, emsgp);
		(void) free(a_aclp);
		return (stat);
	}

	/*
	 * Merge the new ACL entries with the existing ACL.
	 * We do this by matching the new ACL entries to the existing ACL
	 * entries.  If a match, mark the existing entry as an duplicate
	 * type.  When done, re-sort the existing ACL to move the deleted
	 * entries to the end and reset the number of entries.
	 * Note that we do NOT let you delete one of the special ACL
	 * entries (OWNER, GROUP, other, nobody, and mask).
	 */

	stat = 0;
	count = 0;
	fu = ADM_ACL_USERS_OFFSET;	/* Set first & last user entry */
	lu = aclp->groups_offset;
	if (lu == 0)
		lu = aclp->number_entries;
	uep = &aclp->entry[fu];
	fg = aclp->groups_offset;	/* Set first & last group entry */
	lg = aclp->number_entries;
	if (fg == 0)
		fg = aclp->number_entries;
	gep = &aclp->entry[fg];
	aep = &a_aclp->entry[0];	/* Set first delete entry */
	for (i = 0; i < a_aclp->number_entries; i++) {
		switch (aep->type) {
		case ADM_ACL_USERS:
			sep = uep;
			for (j = fu; j < lu; j++) {
				if ((aep->type == sep->type) && (aep->id
				    == sep->id)) {
					sep->type += ADM_ACL_DEAD;
					count++;
					break;
				}
				sep++;
			}				/* End inner for */
			if (j >= lu) {
				stat = ADM_ERR_ACLMISSENTRY;
			}
			break;
		case ADM_ACL_GROUPS:
			sep = gep;
			for (j = fg; j < lg; j++) {
				if ((aep->type == sep->type) && (aep->id
				    == sep->id)) {
					sep->type += ADM_ACL_DEAD;
					count++;
					break;
				}
				sep++;
			}				/* End inner for */
			if (j >= lg) {
				stat = ADM_ERR_ACLMISSENTRY;
			}
			break;
		case ADM_ACL_OWNER:
		case ADM_ACL_GROUP:
		case ADM_ACL_OTHER:
		case ADM_ACL_NOBODY:
		case ADM_ACL_MASK:
		default:
			size = (u_int)ADM_AUTH_ACLENTRY_SIZE;
			(void) adm_auth_ace2str((u_int)0, aep, tbuff, &size);
			stat = adm_auth_fmterr(auth_ep, ADM_ERR_ACLCANTDELETE,
			    tbuff);
			break;
		}					/* End of switch */
		aep++;
	}						/* End outer for */

	/* Ignore missing entry errors */
	if (stat == ADM_ERR_ACLMISSENTRY)
		stat = 0;

	/* If actually deleted any entries, resort the ACL entries */
	if (count > 0)
		adm_auth_sortacl(aclp);
	aclp->number_entries -= count;

	/* Return status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_clracl - clear ACL entries from the ACL in the security info
 *	of a security handle.
 *	Deletes all user and group ACL entries from the ACL structure,
 *	leaving the "required" five entries: OWNER, GROUP, other, nobody,
 *	and mask.
 *	Returns 0 if ACL is cleared successfully; otherwise,
 *	returns an error code.  If error is not fatal, continues to
 *	update the existing ACL.
 * ---------------------------------------------------------------------
 */

int
adm_auth_clracl(
	Adm_auth_handle_t *auth_hp) /* Ptr to Admin security info handle */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth entry structure */
	Adm_auth *ap;		/* Local pointer to auth info structure */
	Adm_acl *aclp;		/* Local pointer to ACL structure */
	Adm_acl *a_aclp;	/* Local pointer to add ACL structure */
	struct Adm_acl_entry *aep; /* Local ptr to aux ACL entry */
	char *tp;		/* Temporary string pointer */
	int   len;		/* Temporary string length */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Set some local pointers */
	ap = &auth_ep->auth_info;
	aclp = &ap->acl;

	/* Check that the ACL has enough entries */
	if (aclp->number_entries < ADM_ACL_USERS_OFFSET) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_BADACL);
		return (stat);
	}

	/* Check that the ACL has valid required entries */
	tp = (char *)NULL;
	if (aclp->entry[ADM_ACL_OWNER_OFFSET].type != ADM_ACL_OWNER)
		tp = ADM_ACL_OWNER_NAME;
	if (aclp->entry[ADM_ACL_GROUP_OFFSET].type != ADM_ACL_GROUP)
		tp = ADM_ACL_GROUP_NAME;
	if (aclp->entry[ADM_ACL_OTHER_OFFSET].type != ADM_ACL_OTHER)
		tp = ADM_ACL_OTHER_NAME;
	if (aclp->entry[ADM_ACL_NOBODY_OFFSET].type != ADM_ACL_NOBODY)
		tp = ADM_ACL_NOBODY_NAME;
	if (aclp->entry[ADM_ACL_MASK_OFFSET].type != ADM_ACL_MASK)
		tp = ADM_ACL_MASK_NAME;
	if (tp != (char *)NULL) {
		stat = adm_auth_fmterr(auth_ep, ADM_ERR_ACLMISSTYPE, tp);
		return (stat);
	}

	/* ACL is good.  Blow away the user and group entries */
	aclp->number_entries = ADM_ACL_USERS_OFFSET;
	aclp->groups_offset = ADM_ACL_USERS_OFFSET;
	len = (ADM_ACL_MAXENTRIES - ADM_ACL_USERS_OFFSET) *
	    (int)sizeof (struct Adm_acl_entry);
	tp = (char *)aclp + (((int)sizeof (struct Adm_acl)) - len);
	(void) memset(tp, (int)0, len);

	/* Return */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_rstacl - reset ACL on/off flag for the ACL in the security
 *	info of a security handle.
 *	Depending upon option argument, sets or resets the ACL OFF flag.
 *	Returns 0 if ACL is reset successfully; otherwise,
 *	returns an error code.  If error is not fatal, continues to
 *	update the existing ACL.
 * ---------------------------------------------------------------------
 */

int
adm_auth_rstacl(
	Adm_auth_handle_t *auth_hp, /* Ptr to Admin security info handle */
	u_int  options)		/* ACL flag reset option) */
{
	Adm_auth_entry *auth_ep; /* Ptr to auth entry structure */
	Adm_auth *ap;		/* Local pointer to auth info structure */
	Adm_acl *aclp;		/* Local pointer to ACL structure */
	u_int flag;		/* Local ACL flag value */
	int   stat;		/* Local status code */

	/* Check for a valid Adm_auth_handle_t handle */
	if (auth_hp == (Adm_auth_handle_t *)NULL)
		return (ADM_ERR_NULLHANDLE);
	else
		auth_ep = (Adm_auth_entry *)auth_hp;

	/* Set some local pointers */
	ap = &auth_ep->auth_info;
	aclp = &ap->acl;

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


	/* Reset the ACL flag from the options */
	aclp->flags = flag;

	/* Return success */
	return (0);
}
