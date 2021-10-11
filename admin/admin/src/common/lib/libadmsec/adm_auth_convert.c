/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.
 */

#pragma ident	"@(#)adm_auth_convert.c	1.14	92/03/03 SMI"

/*
 * FILE:  adm_auth_convert.c
 *
 *	Admin Framework security library routines for converting
 *	Admin security information.  Routines include:
 *		adm_auth_acl2str    - Convert ACL structure to char format
 *		adm_auth_str2acl    - Convert char format ACL to ACL struct
 *		adm_auth_ace2str    - Convert ACL entry to char format
 *		adm_auth_str2ace    - Convert char format to ACL entry
 *		adm_auth_uid2str    - Convert uid to username
 *		adm_auth_str2uid    - Convert username to uid
 *		adm_auth_gid2str    - Convert gid to groupname
 *		adm_auth_str2gid    - Convert groupname to gid
 *		adm_auth_loc2cpn    - Convert local identity to cpn
 *		adm_auth_net2cpn    - Convert netname identity to cpn
 *		adm_auth_cpn2str    - Convert cpn to string format
 *		adm_auth_clear_cpn  - Clear a cpn to null values
 */

#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include "adm_auth.h"
#include "adm_auth_impl.h"
#include "adm_err_msgs.h"

/* Typedef defined for "pointer to function returning int" */
typedef int (*PFI)();

/* Internal function prototype definitions */
static char *cvt_strtok(char **);
static int   cvt_str2null(char *, id_t *);
static int   cvt_chk_domain(char *);
static int   cvt_chk_system(char *);
static void  cvt_build_cpn(Adm_auth_cpn *, u_int, uid_t, char *, u_int,
		 char *, u_int, char *);
static u_int cvt_hash_name(char *, u_int);

/*
 * ---------------------------------------------------------------------
 *  adm_auth_acl2str - Convert ACL entries to char format ACL string.
 *	Builds a multiple entry character string from the entries in the
 *	ACL structure, omitting the class entry.
 *	Returns 0 if ACL is converted; otherwise, returns an error code.
 * ---------------------------------------------------------------------
 */

int
adm_auth_acl2str(
	u_int  option,		/* Conversion options */
	Adm_acl *aclp,		/* Pointer to ACL structure */
	char  *aclbuff,		/* Pointer to buff to contain ACL string */
	u_int *aclsize,		/* Pointer to length of ACL buffer */
	char **emsgpp)		/* Ptr to contain address of error msg */
{
	Adm_acl_entry *tep;	/* Temp ptr to ACL entry struct */
	char  tbuff[ADM_AUTH_ACLENTRY_SIZE]; /* Buffer to convert entry */
	char *sp;		/* Pointer to ACL string buffer curr pos */
	u_int slen;		/* Length of ACL string total length */
	u_int elen;		/* Length of ACL entry in char format */
	u_int  i;		/* Temporary for loop counter */
	int   stat;		/* Local status code */

	/* Check for a valid ACL structure */
	if (aclp == (struct Adm_acl *)NULL) {
		stat = adm_auth_err2str(emsgpp, ADM_ERR_NOACLINFO);
		return (stat);
	}

	/* Check for valid output arguments */
	if ((aclbuff == (char *)NULL) || (aclsize == (u_int *)NULL)) {
		stat = adm_auth_err2str(emsgpp, ADM_ERR_BADARGUMENT);
		return (stat);
	}

	/* set up for converting ACL entries */
	stat = 0;
	slen = 0;
	sp = aclbuff;
	*sp = '\0';

	/* Walk through ACL entries, converting each entry as we go */
	for (i = 0; i < aclp->number_entries; i++) {

		/* Convert next ACL entry to a string in local buffer */
		tep = &aclp->entry[i];
		elen = (u_int)ADM_AUTH_ACLENTRY_SIZE;
		if ((stat = adm_auth_ace2str(option, tep, tbuff, &elen)) != 0)
			stat = adm_auth_err2str(emsgpp, ADM_ERR_BADACLTYPE,
				tbuff);

		/* If not first entry, add ACL entry separator */
		if ((i > 0) && (slen < *aclsize)) {
			*sp++ = ' ';
			slen++;
		}

		/* If still room in output buffer, append entry string */
		if ((slen + elen) >= *aclsize) {
			*sp = '\0';
			stat = adm_auth_err2str(emsgpp, ADM_ERR_ACLTOOBIG);
			break;
		}
		strcpy(sp, tbuff);
		sp += elen;
		slen += elen;
	}					/* End of for loop */

	/* Return length and status */
	*aclsize = slen;
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_str2acl - Convert char format ACL string to ACL entries.
 *	Builds entries in the ACL structure from the ACL string entries.
 *	Does NOT check for duplicate entries or guarantee the special
 *	entries were provided.  Use adm_auth_valacl for this.
 *	Returns 0 if ACL is converted; otherwise, returns an error code.
 * ---------------------------------------------------------------------
 */

int
adm_auth_str2acl(
	u_int  option,		/* Conversion options */
	Adm_acl *aclp,		/* Pointer to ACL structure */
	char  *aclbuff,		/* Pointer to buff to contain ACL string */
	u_int aclsize,		/* Pointer to length of ACL buffer */
	char **emsgpp)		/* Ptr to contain address of error msg */
{
	struct Adm_acl_entry *tep;	/* Temp ptr to ACL entry struct */
	char *sp;		/* Pointer to next position in string */
	char *tp;		/* Pointer to fields in entry string */
	char *ep;		/* Pointer to entry string */
	u_int count;		/* Temporary counter */
	int   stat;		/* Local status code */

	/* Check for a valid ACL structure */
	if (aclp == (struct Adm_acl *)NULL) {
		stat = adm_auth_err2str(emsgpp, ADM_ERR_NOACLINFO);
		return (stat);
	}

	/* Check for valid ACL string arguments */
	if (aclbuff == (char *)NULL) {
		stat = adm_auth_err2str(emsgpp, ADM_ERR_BADARGUMENT);
		return (stat);
	}

	/* Make sure we have at least one token in the ACL string */
	if (aclsize == 0) {
		stat = adm_auth_err2str(emsgpp, ADM_ERR_NOACLINFO);
		return (stat);
	}
	sp = aclbuff;
	if ((tp = cvt_strtok(&sp)) == (char *)NULL) {
		stat = adm_auth_err2str(emsgpp, ADM_ERR_BADARGUMENT);
		return (stat);
	}

	/* Walk through ACL entries, converting each entry as we go */
	stat = 0;
	count = 0;
	while (stat == 0) {

		/* Check if we have too many entries */
		if (count == (u_int)ADM_ACL_MAXENTRIES) {
			stat = adm_auth_err2str(emsgpp, ADM_ERR_ACLTOOBIG);
			continue;
		}

		/* convert next entry to an ACL entry structure */
		ep = tp;
		tep = &aclp->entry[count];
		if ((stat = adm_auth_str2ace(tep, tp)) != 0) {
			stat = adm_auth_err2str(emsgpp, stat, ep);
			continue;
		}

		/* Converted a valid ACL entry!  Increment to next entry */
		count++;
		if ((tp = cvt_strtok(&sp)) == (char *)NULL)
			break;

	}					/* End of while loop */

	/* Complete ACL header */
	aclp->flags = 0;
	aclp->number_entries = count;
	aclp->groups_offset = 0;		/* Can't set offset yet */

	/* Return status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_ace2str - Convert ACL entry to char format ACL entry string.
 *	Builds a character string from the ACL entry input argument.
 *	Always returns some kind of string, even if errors, and returns
 *	the actual length of the output string.
 *	Returns 0 if ACL entry is converted without errors; otherwise,
 *	returns an error code (no error message is formatted).
 * ---------------------------------------------------------------------
 */

int
adm_auth_ace2str(
	u_int  option,		/* Options */
	Adm_acl_entry *aep,	/* Pointer to ACL structure */
	char  *ebuffp,		/* Pointer to buff to contain ACL string */
	u_int *size)		/* Ptr to buff size (in), real size (out) */
{
	char  tbuff[ADM_AUTH_ACLENTRY_SIZE]; /* Buffer to convert entry */
	char *bp;		/* Buffer pointer */
	char *tp;		/* Pointer to string */
	u_int len;		/* Length of converted string */
	int   stat;		/* Local status code */

	/* Check for a valid arguments */
	if ((ebuffp == (char *)NULL) || (size == (u_int *)NULL))
		return (ADM_ERR_BADARGUMENT);
	if (*size == 0)
		return (ADM_ERR_ACLTOOBIG);
	if (aep == (Adm_acl_entry *)NULL) {
		*ebuffp = '\0';
		return (ADM_ERR_NOACLINFO);
	}

	/* If output buffer is big enough, build in it directly */
	if (*size == ADM_AUTH_ACLENTRY_SIZE)
		bp = ebuffp;
	else
		bp = tbuff;

	/* Build ACL entry type */
	tp = bp;
	switch (aep->type) {
	case ADM_ACL_OWNER:
		if (option & ADM_AUTH_LONGTYPE) {
			strcpy(tp, ADM_ACL_OWNER_NAME);
			tp += (u_int)strlen(ADM_ACL_OWNER_NAME);
		} else
			*tp++ = *ADM_ACL_OWNER_NAME;
		*tp++ = ADM_ACL_TYPE_SEP;
		(void) adm_auth_uid2str(tp, (u_int)ADM_AUTH_NAMESIZE,
		    (uid_t)aep->id);
		tp += strlen(tp);
		break;
	case ADM_ACL_GROUP:
		if (option & ADM_AUTH_LONGTYPE) {
			strcpy(tp, ADM_ACL_GROUP_NAME);
			tp += (u_int)strlen(ADM_ACL_GROUP_NAME);
		} else
			*tp++ = *ADM_ACL_GROUP_NAME;
		*tp++ = ADM_ACL_TYPE_SEP;
		(void) adm_auth_gid2str(tp, (u_int)ADM_AUTH_NAMESIZE,
		    (gid_t)aep->id);
		tp += strlen(tp);
		break;
	case ADM_ACL_OTHER:
		if (option & ADM_AUTH_LONGTYPE) {
			strcpy(tp, ADM_ACL_OTHER_NAME);
			tp += (u_int)strlen(ADM_ACL_OTHER_NAME);
		} else
			*tp++ = *ADM_ACL_OTHER_NAME;
		*tp++ = ADM_ACL_TYPE_SEP;
		break;
	case ADM_ACL_NOBODY:
		if (option & ADM_AUTH_LONGTYPE) {
			strcpy(tp, ADM_ACL_NOBODY_NAME);
			tp += (u_int)strlen(ADM_ACL_NOBODY_NAME);
		} else
			*tp++ = *ADM_ACL_NOBODY_NAME;
		*tp++ = ADM_ACL_TYPE_SEP;
		break;
	case ADM_ACL_MASK:
		if (option & ADM_AUTH_LONGTYPE) {
			strcpy(tp, ADM_ACL_MASK_NAME);
			tp += (u_int)strlen(ADM_ACL_MASK_NAME);
		} else
			*tp++ = *ADM_ACL_MASK_NAME;
		*tp++ = ADM_ACL_TYPE_SEP;
		break;
	case ADM_ACL_USERS:
		if (option & ADM_AUTH_LONGTYPE) {
			strcpy(tp, ADM_ACL_USERS_NAME);
			tp += (u_int)strlen(ADM_ACL_USERS_NAME);
		} else
			*tp++ = *ADM_ACL_USERS_NAME;
		*tp++ = ADM_ACL_TYPE_SEP;
		(void) adm_auth_uid2str(tp, (u_int)ADM_AUTH_NAMESIZE,
		    (uid_t)aep->id);
		tp += strlen(tp);
		break;
	case ADM_ACL_GROUPS:
		if (option & ADM_AUTH_LONGTYPE) {
			strcpy(tp, ADM_ACL_GROUPS_NAME);
			tp += (u_int)strlen(ADM_ACL_GROUPS_NAME);
		} else
			*tp++ = *ADM_ACL_GROUPS_NAME;
		*tp++ = ADM_ACL_TYPE_SEP;
		(void) adm_auth_gid2str(tp, (u_int)ADM_AUTH_NAMESIZE,
		    (gid_t)aep->id);
		tp += strlen(tp);
		break;
	default:
		*tp++ = '?';
		*tp++ = ADM_ACL_TYPE_SEP;
		sprintf(tp, "%ld", (long)aep->id);
		tp += strlen(tp);
		break;
	}

	/* Build ACL entry permissions */
	*tp++ = ADM_ACL_PERM_SEP;
	if (aep->permissions & ADM_ACL_READ)
		*tp++ = 'r';
	if (aep->permissions & ADM_ACL_WRITE)
		*tp++ = 'w';
	if (aep->permissions & ADM_ACL_EXECUTE)
		*tp++ = 'x';

	/* Build string terminator */
	*tp = '\0';

	/* If not building directly in output buffer, copy the string */
	if (bp == tbuff)
		strncpy(ebuffp, bp, *size);

	/* Return actual length and too big error if string too big */
	len = tp - bp;
	if (len >= *size) {
		stat = ADM_ERR_ACLTOOBIG;
		ebuffp[*size - 1] = '\0';
	}
	*size = len;

	/* Return status */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_str2ace - Convert char format ACL entry string to ACL entry
 *	structure.
 *	Builds an ACL entry structure from the string input argument.
 *	Returns 0 if ACL string is converted without errors; otherwise,
 *	returns an error code (no error message is formatted).
 * ---------------------------------------------------------------------
 */

int
adm_auth_str2ace(
	Adm_acl_entry *aep,	/* Pointer to ACL structure */
	char  *ebuffp)		/* Pointer to buff to contain ACL string */
{
	PFI   crp;		/* Pointer to function to convert id */
	char *owner_name;	/* Owner entry type name */
	char *group_name;	/* Owner group entry type name */
	char *other_name;	/* Other entry type name */
	char *nobody_name;	/* Nobody entry type name */
	char *mask_name;	/* Mask entry type name */
	char *users_name;	/* User entry type name */
	char *groups_name;	/* Group entry type name */
	u_int owner_len;	/* Owner entry type name length */
	u_int group_len;	/* Owner group entry type name length */
	u_int other_len;	/* Other entry type name length */
	u_int nobody_len;	/* Nobody entry type name length */
	u_int mask_len;		/* Mask entry type name length */
	u_int users_len;	/* User entry type name length */
	u_int groups_len;	/* Group entry type name length */
	char *tp;		/* Pointer to ACL type string */
	char *xp;		/* Pointer to ACL permissions separator */
	int   stat;		/* Local error status code */

	/* Set up for scanning ACL entry type strings */
	owner_name = ADM_ACL_OWNER_NAME;
	group_name = ADM_ACL_GROUP_NAME;
	other_name = ADM_ACL_OTHER_NAME;
	nobody_name = ADM_ACL_NOBODY_NAME;
	mask_name = ADM_ACL_MASK_NAME;
	users_name = ADM_ACL_USERS_NAME;
	groups_name = ADM_ACL_GROUPS_NAME;
	owner_len = strlen(owner_name);
	group_len = strlen(group_name);
	other_len = strlen(other_name);
	nobody_len = strlen(nobody_name);
	mask_len = strlen(mask_name);
	users_len = strlen(users_name);
	groups_len = strlen(groups_name);

	/* Check for valid ACL type string and setup for id conversion */
	stat = 0;
	tp = ebuffp;
	if (*tp == *owner_name) {	/* Owner user entry */
		if (*(tp + 1) != ADM_ACL_TYPE_SEP)
			if (! (strncmp(tp, owner_name, owner_len)))
				tp += owner_len;
			else
				stat = ADM_ERR_BADCHARACLTYPE;
		else
			tp++;
		aep->type = (ushort)ADM_ACL_OWNER;
		crp = adm_auth_str2uid;
	} else if (*tp == *group_name) { /* Owner group entry */
		if (*(tp + 1) != ADM_ACL_TYPE_SEP)
			if (! (strncmp(tp, group_name, group_len)))
				tp += group_len;
			else
				stat = ADM_ERR_BADCHARACLTYPE;
		else
			tp++;
		aep->type = (ushort)ADM_ACL_GROUP;
		crp = adm_auth_str2gid;
	} else if (*tp == *other_name) { /* Other users entry */
		if (*(tp + 1) != ADM_ACL_TYPE_SEP)
			if (! (strncmp(tp, other_name, other_len)))
				tp += other_len;
			else
				stat = ADM_ERR_BADCHARACLTYPE;
		else
			tp++;
		aep->type = (ushort)ADM_ACL_OTHER;
		crp = cvt_str2null;
	} else if (*tp == *nobody_name) { /* Nobody entry */
		if (*(tp + 1) != ADM_ACL_TYPE_SEP)
			if (! (strncmp(tp, nobody_name, nobody_len)))
				tp += nobody_len;
			else
				stat = ADM_ERR_BADCHARACLTYPE;
		else
			tp++;
		aep->type = (ushort)ADM_ACL_NOBODY;
		crp = cvt_str2null;
	} else if (*tp == *mask_name) { /* Mask entry */
		if (*(tp + 1) != ADM_ACL_TYPE_SEP)
			if (! (strncmp(tp, mask_name, mask_len)))
				tp += mask_len;
			else
				stat = ADM_ERR_BADCHARACLTYPE;
		else
			tp++;
		aep->type = (ushort)ADM_ACL_MASK;
		crp = cvt_str2null;
	} else if (*tp == *users_name) { /* User entry */
		if (*(tp + 1) != ADM_ACL_TYPE_SEP)
			if (! (strncmp(tp, users_name, users_len)))
				tp += users_len;
			else
				stat = ADM_ERR_BADCHARACLTYPE;
		else
			tp++;
		aep->type = (ushort)ADM_ACL_USERS;
		crp = adm_auth_str2uid;
	} else if (*tp == *groups_name) { /* Group entry */
		if (*(tp + 1) != ADM_ACL_TYPE_SEP)
			if (! (strncmp(tp, groups_name, groups_len)))
				tp += groups_len;
			else
				stat = ADM_ERR_BADCHARACLTYPE;
		else
			tp++;
		aep->type = (ushort)ADM_ACL_GROUPS;
		crp = adm_auth_str2gid;
	} else {			/* Invalid type */
		stat = ADM_ERR_BADCHARACLTYPE;
	}				/* End of types */

	/* If invalid type, return bad type error */
	if (stat != 0)
		return (stat);

	/* Check for separator */
	if (*tp != ADM_ACL_TYPE_SEP)
		return (ADM_ERR_BADCHARACLENTRY);
	tp++;

	/* Call routine to convert identifier */
	if ((xp = strchr(tp, ADM_ACL_PERM_SEP)) != (char *)NULL)
		*xp = '\0';
	stat = (*crp)(tp, &aep->id);
	if (xp != (char *)NULL)
		*xp = ADM_ACL_PERM_SEP;
	if (stat != 0)
		return (ADM_ERR_BADCHARACLNAME);

	/* Position past identifier and check for optional perms */
	aep->permissions = 0;
	if (xp != (char *)NULL) {
		tp = xp + 1;
		while (*tp != '\0') {
			switch (*tp++) {
			case 'r':		/* Read permission */
				if (aep->permissions & ADM_ACL_READ)
					stat = ADM_ERR_BADCHARACLPERM;
				aep->permissions |= ADM_ACL_READ;
				break;
			case 'w':		/* Write permission */
				if (aep->permissions & ADM_ACL_WRITE)
					stat = ADM_ERR_BADCHARACLPERM;
				aep->permissions |= ADM_ACL_WRITE;
				break;
			case 'x':		/* Execute permission */
				if (aep->permissions & ADM_ACL_EXECUTE)
					stat = ADM_ERR_BADCHARACLPERM;
				aep->permissions |= ADM_ACL_EXECUTE;
				break;
			case '-':		/* Dash means ignore */
				break;
			default:		/* Bad letter */
				stat = ADM_ERR_BADCHARACLPERM;
			}				/* End of switch */
		}					/* End of while */
	}

	/* Return with status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_loc2cpn - Create a common principal name from local identity.
 *	Accepts a local user identifier uid and returns a cpn for it.
 * 	Omits local user name, assumes NO role name and omits local
 *	domain name.  Signature of zero indicates local cpn.
 *	Note that we will store the host name in the cpn for root.
 *	Returns 0 if cpn can be created; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_loc2cpn(
	uid_t  local_uid,	/* Local user uid */
	Adm_auth_cpn *cpnp)	/* Pointer to cpn structure */
{
	char  tbuff[MAXHOSTNAMELEN + 1]; /* Buffer for local host name */
	char *hp;		/* Pointer to host name for root */
	uid_t uid;		/* Local user uid */
	u_int hlen;		/* Length of host name for root */

	/* Check for a cpn structure */
	if (cpnp == (Adm_auth_cpn *)NULL)
		return (-1);

	/* Initialize the user cpn structure */
	adm_auth_clear_cpn(cpnp);

	/* Check for root identity and get local system name */
	if (local_uid == (uid_t)0) {
		uid = (uid_t)ADM_CPN_ROOT_ID;
		if ((sysinfo((int)SI_HOSTNAME, tbuff, (long)MAXHOSTNAMELEN))
		    <= 0)
			return (-1);
		hp = tbuff;
		hlen = strlen(tbuff);
	} else {
		uid = local_uid;
		hp = (char *)NULL;
		hlen = 0;
	}

	/* Build the cpn for this local user */
	cvt_build_cpn(cpnp, (u_int)ADM_CPN_USER, uid, (char *)NULL,
	    (u_int)0, hp, hlen, (char *)NULL);

	/* Return success */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_net2cpn - Create a common principal name from client netname.
 *	Accepts an authentication flavor, authentication netname, and a
 *	pointer to a cpn structure.
 *	Decodes the netname into a cpn algorithmically.  Local domain
 *	users without roles and local system roots are stored simply
 *	as uid's with signatures of zero.
 *	Returns 0 if cpn can be created; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_net2cpn(
	u_int  auth_flavor,	/* Authentication flavor */
	char  *auth_netname,	/* Pointer to authentication netname */
	Adm_auth_cpn *cpnp)	/* Pointer to cpn structure */
{
	char  tbuff[ADM_AUTH_NAMESIZE+1]; /* Temp buffer for conversion */
	uid_t uid;		/* User uid */
	char *tp;		/* Pointer to netname string */
	char *np;		/* Pointer to user name */
	char *hp;		/* Pointer to host or role name */
	char *dp;		/* Pointer to domain name */
	u_int nlen;		/* Length of user name */
	u_int hlen;		/* Length of host or role name */
	long  tlong;		/* Long integer for conversions */

	/* Check for a cpn structure */
	if (cpnp == (Adm_auth_cpn *)NULL)
		return (-1);

	/* Check for a netname */
	if (auth_netname == (char *)NULL)
		return (-1);

	/* Initialize the user cpn structure */
	adm_auth_clear_cpn(cpnp);

	/*
	 * Decode netname based upon authentication flavor.
	 *
	 * For each netname, set the following local variables:
	 *	uid  -> User uid
	 *	np   -> Pointer to user name
	 *	nlen -> Length of user name
	 *	hp   -> Pointer to host or role name
	 *	hlen -> Length of host name
	 *	dp   -> Pointer to domain name (null terminated)
	 */

	tp = auth_netname;
	switch (auth_flavor) {

	/* NONE flavor means no credentials were passed.  Set for nobody */
	case AUTH_NONE:				/* None flavor */
		uid = (uid_t)UID_NOBODY;
		np = (char *)NULL;
		nlen = 0;
		hp = (char *)NULL;
		hlen = 0;
		dp = (char *)NULL;
		break;

	/* UNIX flavor netname: users => <uid>.<host>@<domain> */
	/*			roots => 0.<host>@<domain> */
	case AUTH_UNIX:				/* Unix flavor */
		tlong = strtol(tp, &hp, (int)10);
		if (tp == hp)
			return (-1);
		uid = (uid_t)tlong;
		if (*hp++ != '.')
			return (-1);
		if ((dp = strchr(hp, '@')) == (char *)NULL)
			return (-1);
		hlen = dp - hp;
		dp++;
		if (uid == (uid_t)0) {
			uid = (uid_t)ADM_CPN_ROOT_ID;
			np = ADM_CPN_ROOT_NAME;
			nlen = strlen(np);
		} else {
			np = (char *)NULL;	/* No names for users */
			nlen = 0;
			hp = (char *)NULL;	/* No roles for users */
			hlen = 0;
		}
		if (cvt_chk_domain(dp) == 0) {
			np = (char *)NULL;
			nlen = 0;
		}
		break;

	/* DES flavor netname:  users => unix.<uid>@<domain>  */
	/*			roots => unix.<host>@<domain> */
	case AUTH_DES:
		if (strncmp(tp, "unix.", 5))
			return (-1);
		tp += 5;
		tlong = strtol(tp, &dp, (int)10);
		if (tp != dp) {			/* Non-root user */
			uid = (uid_t)tlong;
			np = (char *)NULL;	/* No names for users */
			nlen = 0;
			hp = (char *)NULL;	/* No roles for users */
			hlen = 0;
			if (*dp != '@')
				return (-1);
			dp++;
		} else {			/* Root user */
			uid = (uid_t)ADM_CPN_ROOT_ID;
			np = ADM_CPN_ROOT_NAME;
			nlen = strlen(np);
			if ((dp = strchr(tp, '@')) == (char *)NULL)
				return (-1);
			hp = tp;
			hlen = dp - tp;
			dp++;
		}
		if (cvt_chk_domain(dp) == 0) {
			np = (char *)NULL;
			nlen = 0;
		}
		break;

	/* Kerberos flavor netname:  users => <name>.<role>@<domain> */
	/*			     roots => root.<host>@<domain> */
	case AUTH_KERB:
		return (-1);
		break;

	/* Unknown authentication flavor is an error */
	default:				/* Unknown flavor */
		return (-1);
		break;
	}					/* End of switch */

	/* Build the cpn for this user */
	cvt_build_cpn(cpnp, (u_int)ADM_CPN_USER, uid, np, nlen, hp, hlen,
	    dp);

	/* Return success */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_cpn2str - Create a string format common principal name from
 *	a cpn structure.
 *	Accepts a pointer to a cpn structure, pointer to a string buffer,
 *	and the maximum string buffer size.
 *	Returns 0 if cpn string can be created; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_cpn2str(
	u_int option,		/* Conversion options */
	Adm_auth_cpn *cpnp,	/* Pointer to cpn structure */
	char *cpn_buff,		/* Pointer to cpn string buffer */
	u_int size)		/* Maximum size of cpn string buffer */
{
	char  tbuff[ADM_AUTH_NAMESIZE+1]; /* Temp buffer for conversion */
	char  dbuff[DOM_NM_LN + 1];	  /* Temp buffer for local domain */
	uid_t uid;		/* User uid */
	char *tp;		/* Pointer to cpn string buffer */
	char *np;		/* Pointer to user name */
	char *hp;		/* Pointer to host name */
	char *dp;		/* Pointer to domain name */
	u_int slen;		/* Remaining length of string buffer */
	u_int tlen;		/* Temp integer length */

	/* Make sure we got a valid buffer pointer */
	if (cpn_buff == (char *)NULL)
		return (-1);
	tp = cpn_buff;
	slen = size;

	/* Make sure this is a valid user cpn structure */
	if (cpnp->context != ADM_CPN_USER)
		return (-1);

	/* Convert user uid and move to cpn string buffer */
	if (cpnp->ugid != (uid_t)UID_NOBODY) {
		sprintf(tbuff, "(%ld)", (long)cpnp->ugid);
		tlen = strlen(tbuff);
		if (tlen >= slen)
			return (-1);
		strcpy(tp, tbuff);
		tp += tlen;
		slen -= tlen;
	}

	/* Check for special cases: Nobody uid and Local uid */
	if ((cpnp->signature & (ulong)ADM_CPN_NAME_MASK) == 0) {
		if (cpnp->ugid == (uid_t)UID_NOBODY)
			np = ADM_CPN_NOBODY_NAME;
		else if (cpnp->ugid == (uid_t)ADM_CPN_ROOT_ID)
			np = ADM_CPN_ROOT_NAME;
		else if ((cpnp->signature & (ulong)ADM_CPN_DOMAIN_MASK)
		     == 0)
			if ((adm_auth_uid2str(tbuff,
			    (u_int)ADM_AUTH_NAMESIZE, cpnp->ugid)) == 0) 
				np = tbuff;
			else
				np = "";
		else
			np = "";
	} else
		np = cpnp->name;

	/* Get pointer to host or role name, if one exists */
	if ((cpnp->signature & (ulong)ADM_CPN_ROLE_MASK) != 0)
		hp = &cpnp->name[cpnp->role_off];
	else if ((cpnp->ugid == (uid_t)ADM_CPN_ROOT_ID) &&
		 (cpnp->signature == (ulong)ADM_CPN_LOCAL_USER)) {
			(void) sysinfo((int)SI_HOSTNAME, tbuff,
			    (long)ADM_AUTH_NAMESIZE);
			tbuff[ADM_AUTH_NAMESIZE] = '\0';
			hp = tbuff;
		} else
			hp = (char *)NULL;

	/* Get pointer to domain name */
	if ((cpnp->signature & (ulong)ADM_CPN_DOMAIN_MASK) != 0)
		dp = &cpnp->name[cpnp->domain_off];
	else if (option & ADM_AUTH_FULLIDS) {
			(void) sysinfo((int)SI_SRPC_DOMAIN, dbuff,
			    (long)(DOM_NM_LN+1));
			dbuff[DOM_NM_LN] = '\0';
			dp = dbuff;
		} else
			dp = (char *)NULL;

	/* Copy user name into string buffer */
	tlen = strlen(np);
	if (tlen >= slen)
		return (-1);
	strcpy(tp, np);
	tp += tlen;
	slen -= tlen;

	/* Copy role or host name into string buffer */
	if (hp != (char *)NULL) {
		tlen = strlen(hp);
		if ((tlen + 1) >= slen)
			return (-1);
		*tp++ = '.';
		strcpy(tp, hp);
		tp += tlen;
		slen -= tlen;
	}

	/* Copy domain name into string buffer */
	if (dp != (char *)NULL) {
		tlen = strlen(dp);
		if ((tlen + 1) >= slen)
			return (-1);
		*tp++ = '@';
		strcpy(tp, dp);
	}

	/* Return success */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_uid2str - Convert a uid to a username or numeric string.
 *	Look up the uid in the local system (or domain)	passwd file
 *	(or map) and return user name.  If no name, convert uid to
 *	string instead.
 *	Returns 0 if uid can be converted; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_uid2str(
	char  *username,	/* Buffer to contain user name */
	u_int  size,		/* Buffer maximum size */
	uid_t  useruid)		/* User uid */
{
	struct passwd *pwbuffp;	/* Password entry buffer */
	char  tbuff[16];	/* Temporary conversion buffer */
	char *sp;		/* Pointer to a string buffer */
	int   stat;		/* Local status code */

	/* Make sure output arguments are good */
	if ((username == (char *)NULL) || (size == 0))
		return (-1);

	/* Get user name, look it up to get string */
	stat = 0;
	pwbuffp = getpwuid(useruid);
	if (pwbuffp == (struct passwd *)NULL) {
		sprintf(tbuff, "%ld", (u_long)useruid);
		sp = tbuff;
		stat = -1;
	} else
		sp = pwbuffp->pw_name;

	/* Copy string user identifier into output buffer */
	strncpy(username, sp, size);
	username[size - 1] = '\0';

	/* Return with status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_gid2str - Convert a gid to a groupname or numeric string.
 *	Look up the gid in the local system (or domain)	group file
 *	(or map) and return group name.  If not found, convert gid to
 *	a string instead.
 *	Returns 0 if uid can be converted; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_gid2str(
	char  *groupname,	/* Buffer to contain group name */
	u_int  size,		/* Buffer maximum size */
	gid_t  groupgid)	/* Group gid */
{
	struct group *grbuffp;	/* Group datastore entry buffer */
	char  tbuff[16];	/* Temporary conversion buffer */
	char *sp;		/* Pointer to a string buffer */
	int   stat;		/* Local status code */

	/* Make sure output arguments are good */
	if ((groupname == (char *)NULL) || (size == 0))
		return (-1);

	/* If returning group name, look it up to get string */
	stat = 0;
	grbuffp = getgrent();			/* Reset groups */
	grbuffp = getgrgid(groupgid);
	if (grbuffp == (struct group *)NULL) {
		sprintf(tbuff, "%ld", (u_long)groupgid);
		sp = tbuff;
		stat = -1;
	} else
		sp = grbuffp->gr_name;

	/* Copy string group identifier into output buffer */
	strncpy(groupname, sp, size);
	groupname[size - 1] = '\0';

	/* Return with status */
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_str2uid - Convert a username or numeric string to a user uid.
 *	If the string is a numeric character string, convert it to numeric;
 *	otherwise, look up the username in the local system (or domain)
 *	passwd file (or map) and return user uid.
 *	Returns 0 if username can be converted; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_str2uid(
	char  *username,	/* User name */
	uid_t *useruid)		/* Buffer to contain user uid */
{
	struct passwd *pwbuffp;	/* Password entry buffer */
	char *sp;		/* Pointer to a string buffer */
	uid_t t_uid;		/* Local uid */
	int   stat;		/* Local status code */

	/* Set up default return values */
	stat = 0;
	t_uid = (uid_t)0;

	/*
	 * If username is a null string, return default uid and an error.
	 * If username is a numeric string, convert and return it.
	 * If username is not a numeric string, look it up in passwd
	 * database.
	 */

	if (! (strcmp(username, ""))) {
		stat = -1;
	} else {
		t_uid = (uid_t)strtol(username, &sp, 10);
		if ((sp == username) || (*sp != '\0')) {
			pwbuffp = getpwnam(username);
			if (pwbuffp == (struct passwd *)NULL)
				stat = -1;
			else
				t_uid = pwbuffp->pw_uid;
		}
	}

	/* Return uid and status */
	*useruid = t_uid;
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_str2gid - Convert a groupname or numeric string to a gid.
 *	If groupname is a numeric character string, convert it to numeric;
 *	otherwise, look up the groupname in the local system (or domain)
 *	group file (or map) and return the group gid.
 *	Returns 0 if uid can be converted; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

int
adm_auth_str2gid(
	char  *groupname,	/* Group name */
	gid_t *groupgid)	/* Buffer to contain group gid */
{
	struct group *grbuffp;	/* Group datastore entry buffer */
	char *sp;		/* Pointer to a string buffer */
	gid_t t_gid;		/* Local gid */
	int   stat;		/* Local status code */

	/* Set up default return values */
	stat = 0;
	t_gid = (gid_t)0;

	/*
	 * If groupname is a null string, return default uid and an error.
	 * If groupame is a numeric string, convert and return it.
	 * If groupname is not a numeric string, look it up in group
	 * database.
	 */

	if (! (strcmp(groupname, ""))) {
		stat = -1;
	} else {
		t_gid = (gid_t)strtol(groupname, &sp, 10);
		if ((sp == groupname) || (*sp != '\0')) {
			grbuffp = getgrent();		/* Reset groups */
			grbuffp = getgrnam(groupname);
			if (grbuffp == (struct group *)NULL)
				stat = -1;
			else
				t_gid = grbuffp->gr_gid;
		}
	}

	/* Return gid and status */
	*groupgid = t_gid;
	return (stat);
}

/*
 * ---------------------------------------------------------------------
 *  adm_auth_clear_cpn - Clear a cpn structure to null values;
 *	No return code.
 * ---------------------------------------------------------------------
 */

void
adm_auth_clear_cpn(
	Adm_auth_cpn *cpnp)	/* Pointer to cpn structure */
{
	/* Clear cpn fields */
	cpnp->signature = (ulong)0;
	cpnp->ugid = UID_NOBODY;
	cpnp->name_len = (ushort)0;
	cpnp->role_off = (ushort)0;
	cpnp->domain_off = (ushort)0;
	cpnp->context = (ushort)0;
	memset((char *)cpnp->name, (int)0, (int)ADM_AUTH_CPN_NAMESIZE);
 
	/* Return */
	return;
}

/*
 * ---------------------------------------------------------------------
 * cvt_build_cpn - Build a cpn structure from user information.
 *	Accepts a pointer to a cpn structure, a cpn type, a uid or gid,
 *	a user or group name and length, a role or host name and length,
 *	and a domain name.
 *	No return code.
 * ---------------------------------------------------------------------
 */

void
cvt_build_cpn(
	Adm_auth_cpn *cpnp,	/* Pointer to cpn structure */
	u_int type,		/* Type of cpn: user, group, other */
	uid_t uid,		/* User uid or unix group gid */
	char *np,		/* Pointer to user or group name */
	u_int nlen,		/* Length of name (not null terminated) */
	char *hp,		/* Pointer to host or role name */
	u_int hlen,		/* Length of host name */
	char *dp)		/* Pointer to domain name */
{
	ulong tsign;		/* Temporary signature */
	u_int thash;		/* Temporary hash value for a name */
	u_int dlen;		/* Domain name length */
	char *tp;		/* String pointer */

	/*
	 * Build the cpn for this user or group given the following:
	 *	uid  -> User uid or gid
	 *	np   -> Pointer to user or group name (may be null)
	 *	nlen -> Length of user name
	 *	hp   -> Pointer to host name (may be null)
	 *	hlen -> Length of host name
	 *	dp   -> Pointer to domain name (null terminated)
	 *
	 * If local domain, omit domain name string.
	 * If root user and local domain, omit user name.
	 */

	/* Handle storage optimizations */
	if (cvt_chk_domain(dp) == 0) {
		dp = (char *)NULL;
		if (uid == 0) {
			np = (char *)NULL;
			nlen = 0;
		}
	}

	/* Set up initial values */
	cpnp->context = type;
	cpnp->ugid = uid;
	tp = cpnp->name;
	tsign = (ulong)0;
	*tp = '\0';

	/* Process user name */
	if (np != (char *)NULL) {
		thash = (cvt_hash_name(np, nlen) % 251) + 1;
		strncpy(tp, np, nlen);
	} else {
		thash = 0;
		nlen = 0;
	}
	tp += nlen;
	*tp = '\0';
	tsign = (tsign << 8) + thash;

	/* Process host or role name */
	if (hp != (char *)NULL) {
		thash = (cvt_hash_name(hp, hlen) % 251) + 1;
		tp++;
		strncpy(tp, hp, hlen);
	} else {
		thash = 0;
		hlen = 0;
	}
	cpnp->role_off = (ushort)(tp - cpnp->name);
	tp += hlen;
	*tp = '\0';
	tsign = (tsign << 8) + thash;

	/* Process domain name */
	if (dp != (char *)NULL) {
		thash = (cvt_hash_name(dp, (u_int)strlen(dp)) % 251) + 1;
		tp++;
		strcpy(tp, dp);
		dlen = strlen(tp);
	} else {
		thash = 0;
		dlen = 0;
	}
	cpnp->domain_off = (ushort)(tp - cpnp->name);
	tp += dlen;
	*tp = '\0';
	tsign = (tsign << 8) + thash;

	/* Set total name length and name signature */
	cpnp->name_len = (ushort)(tp - cpnp->name);
	cpnp->signature = tsign;

	/* Return */
	return;
}
 
/*
 * -------------------------------------------------------------------
 *  cvt_chk_domain - Check given domain name with local domain name.
 *	Returns 0 if specified domain name is not null and matches the
 *	local domain name; otherwise, returns -1.
 *	NOTE: Null strings are valid domain names for standalone systems!
 * -------------------------------------------------------------------
 */

int
cvt_chk_domain(
	char  *dname)		/* Name of domain to be checked */
{
	char   lname[DOM_NM_LN+1]; /* Buffer for local domain name */
 
	/* If the domain name to be checked is null, return error */
	if (dname == (char *)NULL)
		return (-1);
 
	/* Get the local system domain name */
	if (sysinfo((int)SI_SRPC_DOMAIN, lname, (long)(DOM_NM_LN+1)) <= 0)
		return (-1);

	/* Compare domain names.  If no match, return error */
	if (strcmp(dname, lname))
		return (-1);

	/* Match.  Return success */
	return (0);
}
 
/*
 * -------------------------------------------------------------------
 *  cvt_chk_system - Check given system name with local system name.
 *	Returns 0 if specified system name is not null and matches the
 *	local system name; otherwise, returns -1.
 *	NOTE: Null strings are NOT valid host names!
 * -------------------------------------------------------------------
 */

int
cvt_chk_system(
	char  *sname)		/* Name of system to be checked */
{
	char   lname[MAXHOSTNAMELEN+1]; /* Buffer for local system name */
 
	/* If the system name to be checked is null, return error */
	if (sname == (char *)NULL)
		return (-1);
	if (! strcmp(sname, ""))
		return (-1);
 
	/* Get the local system name */
	if (sysinfo((int)SI_HOSTNAME, lname, (long)(MAXHOSTNAMELEN+1)) <= 0)
		return (-1);

	/* Compare system names.  If no match, return error */
	if (strcmp(sname, lname))
		return (-1);

	/* Match.  Return success */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  cvt_hash_name - Generate a hash value for a string name.
 *	Accepts the name to be hashed.
 *	Returns an unsigned integer hash value.
 * ---------------------------------------------------------------------
 */

u_int
cvt_hash_name(
	char  *name,		/* Name to be hashed */
	u_int  len)		/* Length of name to be hashed */
{
	char  *cp;		/* Pointer to a character */
	u_int  hval;		/* Hash value */

	/* Generate the hash value from the string name */
	hval = 0;
	for (cp = name; len > 0; cp++, len--)
		hval = ((hval << 5) + hval) ^ *cp;

	/* Return hash value */
	return (hval);
}

/*
 * ---------------------------------------------------------------------
 *  cvt_str2null - Return a zero identifier.
 *	Returns a zero identifier for special ACL entry id fields.
 *	Returns 0 if null identifier; otherwise, returns -1.
 * ---------------------------------------------------------------------
 */

/* ARGSUSED */

int
cvt_str2null(
	char  *username,	/* User name */
	uid_t *useruid)		/* Buffer to contain user uid */
{

	/* Return null identifier */
	*useruid = (uid_t)0;

	/* Check for a null identifier.  Error if not */
	if ((*username != ADM_ACL_PERM_SEP) && (*username != '\0'))
		return (-1);

	/* Return success */
	return (0);
}

/*
 * ---------------------------------------------------------------------
 *  cvt_strtok - Return pointer to next token in ACL string buffer.
 *	Returns pointer to start of token as results of function call,
 *	replaces token terminator with a null character, and resets the
 *	argument string pointer to the next character after the token
 *	terminator.
 *	Returns a null pointer when no further tokens in the string buffer.
 * ---------------------------------------------------------------------
 */


char *
cvt_strtok(char **spp)		/* Address of ptr to remainder of string */
{
	char *sp;		/* Start of token pointer */
	char *tp;		/* Start of token pointer */

	/* If already at end of buffer, return null pointer */
	sp = *spp;
	if ((sp == (char *)NULL) || (*sp == '\0'))
		return ((char *)NULL);

	/* Scan string for next non-separator character */
	while (*sp == ADM_ACL_ENTRY_SEP)
		sp++;

	/* String pointer is now at end of string or start of next token */
	if (*sp == '\0') {
		*spp = sp;
		return ((char *)NULL);
	}
	tp = sp;				/* Save start of token */

	/* Now scan to end of new token */
	while ((*sp != '\0') && (*sp != ADM_ACL_ENTRY_SEP))
		sp++;

	/* If end of token is not end of string, set terminator */
	if (*sp != '\0')
		*sp++ = '\0';

	/* Return start of token pointer */
	*spp = sp;
	return (tp);
}
