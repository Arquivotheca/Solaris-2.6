/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.
 */

#pragma ident	"@(#)adm_auth_impl.c 1.61	92/10/30 SMI"

/*
 * FILE:  adm_auth_impl.c
 *
 *	Admin Framework low level security library routines
 */

/*
 *	Changes
 *
 *	JParker, Sept 9, 1991 	Readers no longer lock
 *	JParker, Nov,    1991 	getnext and putnext.  Reorg read/write_auth
 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
/*
#include "types.h"
 */
#include <rpc/types.h>
#include <rpc/auth.h>
/*
#include "xdr.h"
*/
#include <rpc/xdr.h>
#include <stdarg.h>

#include "adm_auth.h"
#include "adm_auth_impl.h"
#include "adm_om_impl.h"
#include "adm_sec_impl.h"
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_err_msgs.h"
#include "adm_lock_impl.h"

#define	STREQ(a, b)	(strcmp((a), (b)) == 0)
#define	P(s)		(((s) == (char *)NULL) ? "(null)" : (s))
#define	ERRSTR		(P(strerror(errno)))

#define	ADM_SEC_ACLFILE	".acl"
#define ADM_AUTH_MUTEX	".acllock"

#define	ADM_AUTH_COOKIE	19510328	/* Not in /etc/magic */	
#define	AUTH_BAD_READ	-1
#define	AUTH_BAD_COOKIE	-2
#define	AUTH_BAD_VER	-3


/*
 *	xdr routines
 *
 *	The following are xdr routines for writing and reading
 *	the datatypes defined in adm_auth_impl.h.
 */

bool_t
xdr_acl_version(XDR *xdr_p, Adm_acl_version *version_p)
{
	char *ptr = &version_p->comment[0];

	return (xdr_long(xdr_p, &version_p->cookie) &&
	    xdr_long(xdr_p, &version_p->version) &&
	    xdr_array(xdr_p, &ptr, &version_p->str_length,
		ADM_AUTH_MAXHDR, sizeof (char), (xdrproc_t) xdr_char));
}

bool_t
xdr_acl_entry(XDR *xdr_p, Adm_acl_entry *entry_p)
{
/*
 *	ADM_DBG("S",("AthImp: Enter xdr_acl_entry"));
 */
	return (xdr_u_short(xdr_p, &entry_p->type) &&
		xdr_u_short(xdr_p, &entry_p->permissions) &&
		xdr_long(xdr_p, &entry_p->id));
}

bool_t
xdr_acl(XDR *xdr_p, Adm_acl *acl_p)
{
	Adm_acl_entry *entry_p = &acl_p->entry[0];

/*
 *	ADM_DBG("S",("AthImp: Enter xdr_acl"));
 */

	return (xdr_u_short(xdr_p, &acl_p->flags) &&
		xdr_u_short(xdr_p, &acl_p->groups_offset) &&
		xdr_array(xdr_p, (char **)&entry_p,
		    &acl_p->number_entries,
		    ADM_ACL_MAXENTRIES, sizeof (Adm_acl_entry),
		    (xdrproc_t) xdr_acl_entry));
}

bool_t
xdr_auth_info(XDR *xdr_p, Adm_auth *auth_p)
{
	u_int *u_int_p = &auth_p->auth_flavor[0];

/*
 *	ADM_DBG("S",("AthImp: Enter xdr_auth_info"));
 */

	return (xdr_u_int(xdr_p, &auth_p->version) &&
		xdr_u_int(xdr_p, &auth_p->auth_type) &&
		xdr_u_int(xdr_p, &auth_p->number_flavors) &&
		xdr_array(xdr_p, (char **)&u_int_p,
		    &auth_p->number_flavors,
		    ADM_AUTH_MAXFLAVORS, sizeof (u_int),
		    (xdrproc_t) xdr_u_int) &&
		xdr_u_int(xdr_p, &auth_p->set_flag) &&
		xdr_long(xdr_p, &auth_p->set_uid) &&
		xdr_long(xdr_p, &auth_p->set_gid) &&
		xdr_acl(xdr_p, &auth_p->acl));
}

bool_t
xdr_auth_entry(XDR *xdr_p, Adm_auth_entry *entry_p)
{
	char *name_p = &(entry_p->name[0]);

/*
 *	ADM_DBG("S",("AthImp: Enter xdr_auth_entry"));
 */

	return (
	    xdr_u_int(xdr_p, &entry_p->name_length) &&
	    xdr_u_int(xdr_p, &entry_p->auth_length) &&
	    xdr_auth_info(xdr_p, &entry_p->auth_info) &&
	    xdr_string(xdr_p, &name_p, (u_int) ADM_AUTH_NAMESIZE));
}


/*
 *	Read Authorization Entry
 *
 *	Data is stored in compact XDR form, expanded to fixed size
 */

int
read_auth_entry(XDR *xdr_p, Adm_auth_entry *entry_p)
{
	entry_p->errcode = ADM_AUTH_OK;
	entry_p->errmsgp = (char *)NULL;

	if (!xdr_auth_entry(xdr_p, entry_p))	{
		entry_p->name_length = 0;
		entry_p->auth_length = 0;
		(void) adm_auth_fmterr(entry_p, ADM_ERR_READFAILED, errno);
		return (1);
	}
	return (0);
}


/*
 *	Write Authorization Entry
 *
 *	Write compact XDR version of record
 */

int
write_auth_entry(XDR *xdr_p, Adm_auth_entry *entry_p)
{
	entry_p->errcode = (u_int)0;
	entry_p->errmsgp = (char *)NULL;

	return (xdr_auth_entry(xdr_p, entry_p));
}


/*
 *	check_version
 *
 *	Reads a special record from the head of the file
 *	Currently, the record contains an integer and a blank string
 *	Checks the version against expectations.
 *
 *	Returns		0 	version is as expected
 *			-1	If could not read the version
 *			version	If read version different from expectations
 */

int
check_version(XDR *xdrs, int current_version, int *status)
{
	Adm_acl_version version_header;

	if (!xdr_acl_version(xdrs, &version_header))	{
		ADM_DBG("S",("AthImp: Couldn't read version header from acl file"));
		*status = AUTH_BAD_READ;
		return (-1);	/* Caller will handle error message */
	}
	if (version_header.cookie != (long) ADM_AUTH_COOKIE)	{
		ADM_DBG("S",("AthImp: rong magic number in acl file"));
		*status = AUTH_BAD_COOKIE;
		return (-1);	/* Caller will handle error message */
	}
	if (version_header.version == current_version)	{
		status = ADM_AUTH_OK;
		return (0);
	} else {
		*status = AUTH_BAD_VER;
		return (version_header.version);
	}
}


/*
 *	Rights (permissions, mask)
 *
 *	Are these permissions strong enough to meet the user's needs?
 *	Note that rights is not commutative:
 *		rights(7,3) == 1
 *		rights(3,7) == 0
 */

int
rights(u_short permissions, u_short mask)
{
	return (mask == (permissions & mask));
}


/*
 *	adm_auth_chknobody (auth_entry, request)
 *
 *	Unix has no uid for "nobody".  This interface provides
 *	an alternative entrypoint to adm_auth_chkacl for this case.
 *
 *	Return: 0 (allow) 1 (prohibit)
 */

extern int
adm_auth_chknobody(Adm_auth *auth_p, u_short request)
{
	return (!rights(auth_p->acl.entry[ADM_ACL_NOBODY_OFFSET].permissions,
	    request));
}


/*
 *	adm_auth_chkother (auth_entry, request)
 *
 *	This interface provides an alternative entrypoint to adm_auth_chkacl
 *	for authenticated clients which have NO local domain uid and gid
 *	identities.  We simply check the "other" ACL entry by itself after
 *	making sure mask entry allows access.
 *
 *	Return: 0 (allow) 1 (prohibit)
 */

extern int
adm_auth_chkother(Adm_auth *auth_p, u_short request)
{
	if (! (rights(auth_p->acl.entry[ADM_ACL_MASK_OFFSET].permissions,
	    request)))
		return(ADM_AUTH_DENIED);
	if (! (rights(auth_p->acl.entry[ADM_ACL_OTHER_OFFSET].permissions,
	    request)))
		return (ADM_AUTH_DENIED);
	return (0);
}


/*
 *	adm_auth_chkacl
 *
 *	Given an authorization entry, access more, and a user, check to see
 * 	if the user has the rights requested
 *
 *	We use the POSIX 1003.6 D9 algorithm to check acls, with minor change
 *
 *	Returns 0 (yes) 1 (no) -1 (error)
 */

int
adm_auth_chkacl(Adm_auth *auth_p, u_short request, uid_t useruid,
	gid_t usergid, int grouplen, gid_t  groups[])
{
	Adm_acl *acl_p;
	int i, j, match;
	u_short your_rights;
	uid_t id;
	Adm_acl_entry *m_p;		/* Method pointer */

	assert(auth_p);
	acl_p = &auth_p->acl;

			/* Acl's have flags to turn off checking */
	ADM_DBG("S",("AthImp: Checking Acl: user = %d, group = %d", useruid, usergid));

	if (acl_p->flags == ADM_ACL_OFF)	{
		return (ADM_AUTH_OK);
	}

			/* Is the user the owner? */
	m_p = &acl_p->entry[ADM_ACL_OWNER_OFFSET];
	if (m_p->id == useruid)	{
		ADM_DBG("S",("AthImp: User is owner"));
		return (!rights(m_p->permissions, request));
	}

			/* Is the right allowed by the mask object?	*/
			/* If not, quit now.  				*/
			/* A strict reading of the POSIX standard shows	*/
			/* that "other" need not meet this condition	*/
			/* We assume this is a bug in the standard XXX	*/

	m_p = &acl_p->entry[ADM_ACL_MASK_OFFSET];
	if (!rights(m_p->permissions, request))	{
		ADM_DBG("S",("AthImp: Mask prohibits query: return"));
		return (ADM_AUTH_DENIED);
	}

			/* Is the user a named user? */
	for (m_p = &acl_p->entry[ADM_ACL_USERS_OFFSET];
		m_p < &acl_p->entry[acl_p->groups_offset]; m_p++) {
		if (m_p->id == useruid)	{
			ADM_DBG("S",("AthImp: User is named"));
			return (!rights(m_p->permissions, request));
		}
	}

			/* Is this user one of the owning group? */
	m_p = &acl_p->entry[ADM_ACL_GROUP_OFFSET];
	id = m_p->id;
	if (m_p->id == usergid)	{
		ADM_DBG("S",("AthImp: User belongs to owning group"));
		return (!rights(m_p->permissions, request));
	}
	for (j=0; j < (int) grouplen; j++)	{
		if (id == groups[j])	{
			ADM_DBG("S",("AthImp: User belongs to owning group-in list"));
			return (!rights(m_p->permissions, request));
		}
	}

			/* Is one of the user's groups listed?	*/
	i = ADM_ACL_GROUP_OFFSET;
	your_rights= (u_short)0;	/* we need to total all rights */
	match = 0;

	ADM_DBG("S",("AthImp: We'll try to match groups"));
	for (i = acl_p->groups_offset; i < (int) acl_p->number_entries; i++) {
			/* Does the user belong to this group? */
		if (acl_p->entry[i].id == usergid)	{
			ADM_DBG("S",("AthImp: Found a match in user's main group"));
			your_rights = your_rights | acl_p->entry[i].permissions;
			match = 1;
		}
		for (j=0; j < (int) grouplen; j++)	{
			if (acl_p->entry[i].id == groups[j])	{
				ADM_DBG("S",("AthImp: Found a match in user's group"));
				your_rights =
				    your_rights | acl_p->entry[i].permissions;
				match = 1;
				break;		/* No more to be gained here */
			}
		}
	}
	if (match)	{
		return (!rights(your_rights, request));
	}

			/* User's only hope is for a significant Other */
	ADM_DBG("S",("AthImp: Only hope is significant other!"));
	return (!rights(acl_p->entry[ADM_ACL_OTHER_OFFSET].permissions,
	    request));
}

/*
 *	Adm_auth_chkauth
 *
 *	Is the authorization the user requested correct for the method?
 *
 *	returns 0 (yes) 1 (no) -1 (error)
 *	If we don't return 0, we return a list of flavors that we'll take
 *
 *	We accept a pointer to the method's security entry structure,
 *	the system-wide authentication type and flavor, and the client
 *	request flavor.
 *
 *	We determine the valid authentication type and flavor(s) to be
 *	used as follows:
 *
 *	Given:  Method's type and flavor; T(m) and F(m)
 *              System's type and flavor: T(s) and F(s)
 *
 *	then:	if T(s) < T(m), T = T(m) and F = F(m)
 *		if T(s) > T(m), T = T(s) and F = F(s)
 *		if T(s) = T(m), T = T(s) and if F(s) specified, F = F(S)
 *		                             if F(s) unspecified, F = F(m)
 *		if F is unspecified, F = default flavor for T
 *
 *	Once we have the type and flavor, we check the request flavor
 *	against them.  Must have sufficient strength and may need to be
 *	the right flavor.  If using F(m), there may be multiple flavors 
 *	returned.
 */

int
adm_auth_chkauth(Adm_auth *auth_p, 	/* Method's authorization info */

	u_int sys_auth_type,		/* System-wide auth type */
	u_int sys_auth_flavor,		/* System-wide auth flavors */

	u_int user_auth_flavor, 	/* Clients propsed auth flavor */
	u_int *auth_type_p,		/* Clients proposed auth strength */
	u_int *user_flavor_len_p, 	/* Limit on flavors to output */

	u_int auth_flavor[])		/* Return: Flavors method accepts */
{
	int i, pos;
	u_int len, strength;
	u_int temp[ADM_AUTH_MAXFLAVORS];
	u_int flavor_sys;
	u_int flavor_type;
	u_int *flavor_array_p;			/* Pointer to valid flavors */
	u_int flavor_count;			/* How many valid flavors? */
	int status;

/*
 *	ADM_DBG("S",("AthImp: Check auth: sys_type %d, user_flavor %d", 
 *	    sys_auth_type, user_auth_flavor));
 */

		/*
		 * Which is more stringent: method or system?
		 * System wins if it has a higher type, or if system, method 
		 * have same type and system defines flavors 
		 */
	if ((sys_auth_type != ADM_AUTH_UNSPECIFIED) &&
	    ((sys_auth_type > auth_p->auth_type) ||
		((sys_auth_type == auth_p->auth_type) &&
		 (sys_auth_flavor != ADM_AUTH_UNSPECIFIED))))	{
			flavor_type = sys_auth_type;
			flavor_sys = sys_auth_flavor;
			flavor_array_p = &flavor_sys;
			flavor_count = 1;
 			ADM_DBG("S",("AthImp: Check auth: use system"));
	} else {
		flavor_type = auth_p->auth_type;
		flavor_array_p = &(auth_p->auth_flavor[0]);
		flavor_count = auth_p->number_flavors;
 		ADM_DBG("S",("AthImp: Check auth: use method"));
	}
			
	/* Map user flavor to corresponding strength */
	status = adm_auth_chkflavor(user_auth_flavor, auth_type_p);
	if (status == ADM_AUTH_OK)	{
		if (flavor_type > *auth_type_p)	{
			ADM_DBG("S",("AthImp: Check auth fails: too weak"));
			status = ADM_AUTH_DENIED;
		} else {
			if ((flavor_count < 1) ||
			    (*flavor_array_p == ADM_AUTH_NONE) ||
			    (*flavor_array_p == ADM_AUTH_UNSPECIFIED)) {
					/* Then any will do */
/*
 *				ADM_DBG("S",("AthImp: Chkauth finds no authorization"));
 */
				status = ADM_AUTH_OK;
			} else {		/* Walk the method's array */
				status = ADM_AUTH_WRONG_FLAVOR;
				for (i = 0; i < flavor_count; i++) {
					if (*flavor_array_p ==
					    user_auth_flavor) {
						ADM_DBG("S",("AthImp: Match on auth"));
						status = ADM_AUTH_OK;
						break;
					}
					flavor_array_p++;
				}
				if (status == ADM_AUTH_WRONG_FLAVOR)	{
					/* User didn't have right flavor */
					ADM_DBG("S",
					    ("AthImp: User didn't have right flavor"));
				}
			}
		}
	} /* else there was an error */

		/* If the client fails, we return the flavors we like */
	if (status != ADM_AUTH_OK)	{

		*auth_type_p = flavor_type; /* Set the strength */
				/*
				 * If the method has no flavor, return all
				 * at this strength or stronger
				 */
		if ((flavor_count < 1) ||
		    (*flavor_array_p == ADM_AUTH_NONE) ||
		    (*flavor_array_p == ADM_AUTH_UNSPECIFIED))	{
 			ADM_DBG("S",("AthImp: Check auth:method w/no flavors"));
			pos = 0;
			for (strength = flavor_type;
			    strength <= ADM_AUTH_STRONG; strength++)	{
				if (pos == *user_flavor_len_p)	{
					break;
				}

				len = ((*user_flavor_len_p < ADM_AUTH_MAXFLAVORS) ?
				    *user_flavor_len_p : ADM_AUTH_MAXFLAVORS);

				adm_auth_chktype(strength, &len, temp);
				for (i = 0; i < len; i++)	{
					auth_flavor[pos] = temp[i];
					pos++;
					if (pos == *user_flavor_len_p) {
						break;
					}
				}
			}

			*user_flavor_len_p = pos; /* return length of array */
 			ADM_DBG("S",("AthImp: return %d flavors", pos));

		} else {	/* The method defined some flavor(s). */
				/* Take the min length */
 			ADM_DBG("S",("AthImp: chkauth:method has flavors"));
			len = ((flavor_count < *user_flavor_len_p) ?
			    flavor_count : *user_flavor_len_p);
			*user_flavor_len_p = len;
 			ADM_DBG("S",("AthImp: return %d flavors", len));
			for (i = 0; i < len; i++, flavor_array_p++) {
				auth_flavor[i] = *flavor_array_p;
			}
		}
	}
	return (status);
}



/*
 *	adm_auth_chksid
 *
 *	We assume that the user has the right to access the procedure
 *	Check the method's preferences for the uid and gid to run under
 *
 */
int
adm_auth_chksid(Adm_auth *auth_p, uid_t *useruid_p, gid_t *usergid_p)
{
	u_int flag;
	int   status = ADM_AUTH_OK;

	flag = auth_p->set_flag;

	if (flag == 0)	{
		*useruid_p  = auth_p->set_uid;
		*usergid_p = auth_p->set_gid;
		status = 1;
		ADM_DBG("S",("AthImp: Check SID: use method's id"));
		return (status);
	}
	if (ADM_SID_CLIENT_UID & flag)	{
		/* Use the user's uid */
		status = ADM_AUTH_OK;
	}
	if (ADM_SID_CLIENT_GID & flag)	{
		/* Use the user's gid */
		status = ADM_AUTH_OK;
	}
	if (ADM_SID_AGENT_UID & flag)	{
		*useruid_p = getuid();
		status = 1;
	}
	if (ADM_SID_AGENT_GID & flag)	{
		*usergid_p = getgid();
		status = 1;
	}

/*
 *	ADM_DBG("S",("AthImp: Check SID: returns %d", status));
 */
	return (status);
}


/*	mutex_lock
 *
 *	Obtain a lock for reading or writing
 *	Since the the .acl file is renamed during writing, we
 *	use a dummy file to control locking.  
 *	If the file is missing, we create it.
 *
 *	Return 0 (yes)		1 (no)
 */
static int
mutex_lock(char *path, char *class, FILE **fp_p, int lock_type, int wait_time,
    Adm_auth_entry *entry_p)
{
	int status = ADM_AUTH_OK;
	int err;		/* dummy int */
	char mutex_path[MAXPATHLEN];

	errno = 0;

	strcpy(mutex_path, path);
	adm_build_path(mutex_path, ADM_AUTH_MUTEX);
	status = access(mutex_path, F_OK);

	if (status != ADM_AUTH_OK)	{
		if (errno != ENOENT)	{
			status = adm_auth_fmterr(entry_p, ADM_ERR_FINDMUTEX, 
			    P(class), ERRSTR);
		} else {			/* Doesn't exist: create it */
			if ((*fp_p = fopen(mutex_path, "w")) == (FILE *)NULL) {
				ADM_DBG("S",("AthImp: Could not create mutex file"));
				status = adm_auth_fmterr(entry_p, 
				    ADM_ERR_MAKEMUTEX, P(class), ERRSTR);
			} else {
				status = ADM_AUTH_OK;
				err = fclose(*fp_p);
			}	
		}
	}

	if (status == ADM_AUTH_OK)	{
		if (lock_type == ADM_LOCK_SH)	{
			*fp_p = fopen(mutex_path, "r");
		} else {
			*fp_p = fopen(mutex_path, "w");
		}
		if (*fp_p == (FILE *)NULL)	{
			ADM_DBG("S",("AthImp: Could not open mutex %s", P(mutex_path)));
			status = adm_auth_fmterr(entry_p, 
			    ADM_ERR_OPENMUTEX, P(class), ERRSTR);
		}
	}
				
			/* Should have found and opened file by now */	
	if (status == ADM_AUTH_OK)	{
		status = adm_lock(fileno(*fp_p), lock_type, wait_time);
		if (status != ADM_AUTH_OK)	{
			ADM_DBG("S",("AthImp: Couldn't lock mutex"));
			status = adm_auth_fmterr(entry_p, ADM_ERR_LOCKMUTEX, 
			    lock_type, class, ((err < (int) 0) ? "-1": ERRSTR));
			err = fclose(*fp_p); /* Catch and drop error */
		}
	}

	return (status);
}

/*	mutex_unlock
 *
 *	Release a lock on mutex file
 */
static int
mutex_unlock(char *class, FILE *fp, Adm_auth_entry *entry_p)
{
	int status = ADM_AUTH_OK;
	int err;		/* dummy int */

	if (fp != (FILE *)NULL)	{
		if ((err = adm_unlock(fileno(fp))) != ADM_AUTH_OK)    {
			ADM_DBG("S",("AthImp: Could not unlock file"));
			status = adm_auth_fmterr(entry_p, ADM_ERR_UNLOCKMUTEX, 
			    P(class), ((err < 0) ? "-1" : ERRSTR));
			err = fclose(fp);
		} else {
			if ((status = fclose(fp)) != 0)	{
				ADM_DBG("S",("AthImp: Could not close acl file"));
				status = adm_auth_fmterr(entry_p, 
				    ADM_ERR_CLOSEMUTEX, P(class), ERRSTR);
			}
		}
	}	/* else ignore empty file pointer.  This eases cleanup */

	return (status);
}


/*
 *	read_only_open
 *
 *	Opens a file for reading.  returns an XDR stream as parameter
 *	Returns a status code (and an error string, if needed)
 *	Checks version.
 */
int
read_only_open(char *class, char *c_version, char *method, char *path, 
	XDR *xdr_p, FILE **fp_p, Adm_auth_entry *entry_p)
{
	int status = ADM_AUTH_OK;
	int err    = 0;
	int ver;		/* Version of acl file */

	*fp_p = (FILE *)NULL;
	errno = 0;

	if (adm_find_class(class, c_version, path, MAXPATHLEN)) {
		status = adm_auth_fmterr(entry_p, ADM_ERR_NOAUTHCLASS, P(class),
		    P(c_version));
		ADM_DBG("S",("AthImp: Could not find file %s", P(path)));
		return (status);
	}

	adm_build_path(path, ADM_SEC_ACLFILE);
	status = access(path, F_OK);
	if (status != ADM_AUTH_OK)	{
		status = adm_auth_fmterr(entry_p, ADM_ERR_OPENACL, class);
		if (errno == ENOENT)	{
			ADM_DBG("S",("AthImp: Could not find file %s",
			    P(path)));
		} else {
			ADM_DBG("S",("AthImp: Read: %s", ERRSTR));
		}
	} else {
		if ((*fp_p = fopen(path, "r")) == (FILE *)NULL)  {
			status = adm_auth_fmterr(entry_p, ADM_ERR_OPENACL,
			    class);
			ADM_DBG("S",("AthImp: Could not open file %s",
			    P(path)));
		} /* else file is open */
	}

	if ((status == ADM_AUTH_OK) && (feof(*fp_p))) {	/* File is empty */
		status = ADM_ERR_NOAUTHMETHOD;
	}

	if (status == ADM_AUTH_OK)	{ 	/* Open and locked */
		xdrstdio_create(xdr_p, *fp_p, XDR_DECODE);
		ver = check_version(xdr_p, ADM_AUTH_VERSION, &err);
		if (ver != 0)	{
			if (ver < 0)	{
				status = adm_auth_fmterr(entry_p, 
				    ADM_ERR_AUTHREAD, ERRSTR, P(class), 
				    P(c_version), P(method));
				if (err == AUTH_BAD_READ)	{
					ADM_DBG("S",
					    ("AthImp: Couldn't read acl ver"));
				}
				if (err == AUTH_BAD_COOKIE)	{
					ADM_DBG("S",
					    ("AthImp: Wrong cookie in acl"));
				}
			} else {
				status = adm_auth_fmterr(entry_p, 
				    ADM_ERR_WRONGVER, ver);
				ADM_DBG("S",
				    ("AthImp: Wrong ACL version in read"));
			}
		}
		if (status != ADM_AUTH_OK) {
			xdr_destroy(xdr_p);	/* Free private storage */
		}
	}

	if ((status != ADM_AUTH_OK) && (*fp_p != (FILE *)NULL))	{ /* Clean up */
		err = fclose(*fp_p);	/* Ignore err: status more important */
	}
		
	return (status);
}

/*
 *	adm_auth_read
 *
 *	This relies on two procedures for low-level work
 *		check_version for versioning
 *		read_auth_entry to read in the bits
 *	Improvements made to these should be invisible
 */

int
adm_auth_read(char *class, char *c_version, char *method,
	Adm_auth_entry *entry_p)
{
	int status = ADM_AUTH_OK;
	FILE *fp = (FILE *)NULL;	/* Acl file */
	XDR xdrs;			/* xdr stream */
	char path[MAXPATHLEN];


	errno = ADM_AUTH_OK;
	if ((status = read_only_open(class, c_version, method, path,
	    &xdrs, &fp, entry_p)) == ADM_AUTH_OK)	{

		/* xdr stream is hooked up, file is open */
		status = ADM_ERR_NOAUTHMETHOD;	/* Prepare to fail */
	
		while (!feof(fp)) {
			if (read_auth_entry(&xdrs, entry_p) != ADM_AUTH_OK) {
				status = ADM_ERR_READFAILED;
				break;
			} else {
				if (STREQ(entry_p->name, method)) {
					status = ADM_AUTH_OK;
					break;
				}
			}
		}

		if (status != ADM_AUTH_OK)	{
			/* Format an eror statement */
			if (feof(fp) || (status == ADM_ERR_NOAUTHMETHOD)) {
				status = adm_auth_fmterr(entry_p,
				    ADM_ERR_NOAUTHMETHOD, P(method), P(path));
			} else {
/* XXX At least in the case of ADM_ERR_AUTHREAD, a better message is there. */
/* XXX What do consumers of this routine expect? */
				status = adm_auth_fmterr(entry_p,
				    ADM_ERR_READFAILED, errno);
			}
		}
	
					/* Close down */
		if (fp != (FILE *)NULL)	{
			(void) fclose(fp);
		}
		xdr_destroy(&xdrs);		/* Free private storage */
	}
	return (status);
}

/*
 *	adm_auth_getnext
 *
 *	Provides interface to read all acls.
 *	Opens acl file, keeping state hidden from caller.
 *
 *	Does read_only_open really need to see method?
 */

int
adm_auth_getnext(char *class, char *c_version, u_int opt, 
    Adm_auth_entry *entry_p)
{
	int status = ADM_AUTH_OK;
	static FILE *fp;		/* Acl file */
	static XDR xdrs;		/* xdr stream */
	char path[MAXPATHLEN];
	char *method = "All methods";
	errno = 0;

	if (opt == ADM_AUTH_FIRSTACL)	{

		/* This is a request to open new file */
		if ((status = read_only_open(class, c_version, method, path,
		    &xdrs, &fp, entry_p)) != ADM_AUTH_OK)	{
			return (status);
		}

		/* If open suceeded, we'll continue below */
	}

	/* xdr stream is hooked up, file is open, version is right */
	if ((status = read_auth_entry(&xdrs, entry_p)) != ADM_AUTH_OK) {
		if (feof(fp)) {			/* Close down */
			(void) fclose(fp);
			xdr_destroy(&xdrs);	/* Free private storage */
			return (ADM_ERR_NOACLINFO);
		} else {
			status = ADM_ERR_READFAILED;
		}
	}

	return (status);
}
	

/*
 *      prepare_acl_for_write
 *
 *      Prepare an acl file for writing.  Locks file, opens tempfile
 *	for output
 *
 */
  
int
prepare_acl_for_write(char *class, char *c_version, char *path, 
	char *temp_path, FILE **np_p, FILE **mp_p, XDR *out_xdrs_p, 
	Adm_auth_entry *entry_p)
{
        int status = ADM_AUTH_OK;
        char *temp = ".temp";
 
        errno = 0;
 
        if (adm_find_class(class, c_version, path, MAXPATHLEN)) {
                status = adm_auth_fmterr(entry_p, ADM_ERR_NOAUTHCLASS, P(class),                    P(c_version));
                return (status);
        }
 
        if ((status = mutex_lock(path, class, mp_p, ADM_LOCK_EX,
            ADM_LOCK_MAXWAIT, entry_p)) != ADM_AUTH_OK)   {
                return (status);
        }
 
        strcpy(temp_path, path);
        adm_build_path(temp_path, temp);
 
        if ((*np_p = fopen(temp_path, "w")) == (FILE *)NULL)       {
                status = adm_auth_fmterr(entry_p, ADM_ERR_OPENACL, class);
                return (status);
        }
        xdrstdio_create(out_xdrs_p, *np_p, XDR_ENCODE);

        adm_build_path(path, ADM_SEC_ACLFILE);
        if (access(path, F_OK)) {       /* Acl file doesn't exist */
                status = ADM_ERR_NOACLINFO;
        }
        
	return (status);
}

/*
 *	adm_auth_write
 *
 *	Write an acl to a file.  If previous version exists, remove it
 */

int
adm_auth_write(char *class, char *c_version, char *method,
	Adm_auth_entry *entry_p)
{
	Adm_auth_entry *old_entry_p;
	Adm_auth_entry old_entry;
	char path[MAXPATHLEN];
	char temp_path[MAXPATHLEN];
	int num_wtn;		/* Number of records written */
	int status = ADM_AUTH_OK;
	int newfile = 0;	/* Is this a new acl file? */
	int found = 0;		/* Has this been seen before? */
	FILE *op = (FILE *)NULL, 	/* Old acl file */
	     *np = (FILE *)NULL, 	/* New acl file */
	     *mp = (FILE *)NULL; 	/* Mutex file */
	XDR in_xdrs, out_xdrs;		/* Input and output streams */
	Adm_acl_version version_header;

	errno = 0;

	status = prepare_acl_for_write(class, c_version, path, temp_path, 
	    &np, &mp, &out_xdrs, entry_p);

	if (status == ADM_ERR_NOACLINFO)	{

		/* No ACL file existed */
		/* Create version entry and fall through to "append" */
		/* the ACL entry to be written */

		newfile = 1;
					/* Temporary file is open and locked */
		version_header.cookie = ADM_AUTH_COOKIE;
		version_header.version = ADM_AUTH_VERSION;
		version_header.str_length = 0;
		version_header.comment[0] = '\0';
	
		if (!xdr_acl_version(&out_xdrs, &version_header)) {
			status = adm_auth_fmterr(entry_p, ADM_ERR_AUTHWRITE,
			    ERRSTR, P(class), P(c_version), P(method));
		} else {
				status = ADM_AUTH_OK;
		}
	} else {

		/* If file exists, get the version and copy to new file */
		/* This while loop is really an if-statment with breaks */
		/* Note: We only copy the version entry in this loop */

		while (status == ADM_AUTH_OK) {

			/* Old ACL file exists: open for read */
			if ((op = fopen(path, "r+")) == (FILE *)NULL) {
				status = adm_auth_fmterr(entry_p, 
				    ADM_ERR_OPENACL, class);
				break;
			}
			xdrstdio_create(&in_xdrs, op, XDR_DECODE);

			/* Get version and check it */
			if (!xdr_acl_version(&in_xdrs, &version_header)) {
				status = adm_auth_fmterr(entry_p, 
				    ADM_ERR_AUTHREAD, ERRSTR, P(class), 
				    P(c_version), P(method));
				ADM_DBG("S",("AthImp: Read acl %s", ERRSTR));
				break;
			}
	
			if (version_header.version != ADM_AUTH_VERSION) {
				status = adm_auth_fmterr(entry_p, 
				    ADM_ERR_WRONGVER, version_header.version);
				ADM_DBG("S",("AthImp: Wrong ACL ver in write"));
				break;
			}
	
			/* Write version to new .acl file */
			if (!xdr_acl_version(&out_xdrs, &version_header)) {
				status = adm_auth_fmterr(entry_p, 
				    ADM_ERR_AUTHWRITE, ERRSTR, P(class), 
				    P(c_version), P(method));
				break;
			}
			break;
		}
	}

	/* If status = OK, new acl file exists, with proper version at front */
	/* Prepare to modify existing acl entry or append new acl for method */
	strcpy(entry_p->name, method);
	entry_p->name_length = strlen(method);

	/* If not a new file, we must copy all existing acl entries from */
	/* old file to new.  While doing the copy, look out for the acl */
	/* entry we are writing.  If it exists, replace the old entry */
	/* with this new entry.  If it does not exist, we must append */
	/* the entry to be written to the end of the new acl file. */
	/* Append logic follows this loop */

	if ((status == ADM_AUTH_OK) && (!newfile)) {

		old_entry_p = &old_entry;

		/* Copy old file to new, alter method acl if seen */
		while (read_auth_entry(&in_xdrs, old_entry_p) == ADM_AUTH_OK) {
			if (!STREQ(old_entry_p->name, method))	{
				num_wtn = write_auth_entry(&out_xdrs, 
				    old_entry_p);
			} else {
				if (found != 1)	{
					num_wtn = write_auth_entry(&out_xdrs, 
					    entry_p);
					found = 1;
				}
			}
	
			if (num_wtn != 1)	{
				status = adm_auth_fmterr(entry_p, 
				    ADM_ERR_AUTHWRITE, ERRSTR, P(class), 
				    P(c_version), P(method));
				break;
			}
		}	/* done copying file: done or broke */
	}

	/* If a new acl file or acl entry to be written did not exist */
	/* in the old acl file, we must append acl entry to end of new file */

	if ((status == ADM_AUTH_OK) && (found == 0)) {	

		/* New entry or new file */
		if (1 != write_auth_entry(&out_xdrs, entry_p)) {
			status = adm_auth_fmterr(entry_p, ADM_ERR_AUTHWRITE, 
			    ERRSTR, P(class), P(c_version), P(method));
		}
	}

	/* Always clean up: sometime commit changes */
	if (np != (FILE *)NULL)	{
		(void) fclose(np);
		xdr_destroy(&out_xdrs);
	}
	if (op != (FILE *)NULL)	{
		(void) fclose(op);
		xdr_destroy(&in_xdrs);
	}

	if (status == ADM_AUTH_OK)	{
		if (rename(temp_path, path) != 0)	{
			status = adm_auth_fmterr(entry_p, ADM_ERR_RENAME,
			    P(path));
		}
	}
	(void) mutex_unlock(class, mp, entry_p);
	return (status);
}

/*
 *	adm_auth_putnext
 *
 *	Append an acl to a file. 
 */

int
adm_auth_putnext(char *class, char *c_version, u_int opt, 
    Adm_auth_entry *entry_p)
{
	char path[MAXPATHLEN];
	char temp_path[MAXPATHLEN];
	int status;
	static FILE *np, 	/* acl file */
	            *mp; 	/* Mutex file */
	static XDR out_xdrs;	/* output stream */
	Adm_acl_version version_header;

	status = ADM_AUTH_OK;
	if (opt == ADM_AUTH_FIRSTACL)	{
		mp = (FILE *)NULL; 	/* Mutex file */
		np = (FILE *)NULL; 	/* Output file */

		/* This is a new file: open the file */
		status = prepare_acl_for_write(class, c_version, path, 
		    temp_path, &np, &mp, &out_xdrs, entry_p);

		if ((status != ADM_AUTH_OK) && (status != ADM_ERR_NOACLINFO)) {
			return (status);
		}
					/* Temporary file is open and locked */
		version_header.cookie = ADM_AUTH_COOKIE;
		version_header.version = ADM_AUTH_VERSION;
		version_header.str_length = 0;
		version_header.comment[0] = '\0';
	
		if (!xdr_acl_version(&out_xdrs, &version_header)
		    || !write_auth_entry(&out_xdrs, entry_p)) {
			status = adm_auth_fmterr(entry_p, ADM_ERR_AUTHWRITE,
			    ERRSTR, P(class), P(c_version), "version header");
		}
	}

	/* file is open, xdr stream is hooked up, version has be written */
	/* Append new entry to the file */
	if ((status == ADM_AUTH_OK) && (opt != ADM_AUTH_FIRSTACL) 
	    && (opt != ADM_AUTH_LASTACL))	{
		if (1 != write_auth_entry(&out_xdrs, entry_p)) {
			status = adm_auth_fmterr(entry_p, ADM_ERR_AUTHWRITE, 
			    ERRSTR, P(class), P(c_version), "put next");
		/* XXX This error assumes method namehere. New error message? */
		}
	}

	if ((opt == ADM_AUTH_LASTACL) || 
	    ((status != ADM_AUTH_OK) && (status != ADM_ERR_NOACLINFO))) {

		/* Shut down and quit */
		/* XXX There may be other conditions that don't force quit */
		
		if (np != (FILE *)NULL)	{
			(void) fclose(np);
			xdr_destroy(&out_xdrs);
		}
		(void) mutex_unlock(class, mp, entry_p);
	}

	return (status);
}

/*
 *	adm_auth_delete
 *
 *	Remove an acl from a file
 */

int
adm_auth_delete(char *class, char *c_version, char *method,
	Adm_auth_entry *entry_p)
{
	Adm_auth_entry *old_entry_p;
	Adm_auth_entry old_entry;
	char path[MAXPATHLEN];
	char temp_path[MAXPATHLEN];
	int num_wtn;	/* Number of records written */
	int status = ADM_AUTH_OK;
	char *temp = ".temp";
	int found = 0;		/* Has this been seen before? */
	FILE *op = (FILE *)NULL, 	/* Old acl file */
	     *np = (FILE *)NULL, 	/* New acl file */
	     *mp = (FILE *)NULL; 	/* Mutex file */
	XDR in_xdrs, out_xdrs;		/* Input and output streams */
	Adm_acl_version version_header;

	errno = 0;

	if (adm_find_class(class, c_version, path, MAXPATHLEN)) {
		status = adm_auth_fmterr(entry_p, ADM_ERR_NOAUTHCLASS, P(class),
		    P(c_version));
		return (status);
	}

	if ((status = mutex_lock(path, class, &mp, ADM_LOCK_EX, 
	    ADM_LOCK_MAXWAIT, entry_p)) != ADM_AUTH_OK)	{
		return (status);
	}
			/*
			 * The following while statment will only be
			 * executed once.  While/break is used rather
			 * than a go-to
			 */
	while (status == ADM_AUTH_OK)	{
		strcpy(entry_p->name, method);
		entry_p->name_length = strlen(method);
		strcpy(temp_path, path);
		adm_build_path(path, ADM_SEC_ACLFILE);
		adm_build_path(temp_path, temp);

		if (access(path, F_OK))	{	 /* New acl file */
			status = adm_auth_fmterr(entry_p, ADM_ERR_NOAUTHMETHOD,
			    P(method), P(path));
			break;
		}				/* End of new Acl case */

		if ((op = fopen(path, "r+")) == (FILE *)NULL) {
			status = adm_auth_fmterr(entry_p, ADM_ERR_OPENACL, 
			    class);
			break;
		}
		xdrstdio_create(&in_xdrs, op, XDR_DECODE);

		if ((np = fopen(temp_path, "w")) == (FILE *)NULL)	{
			status = adm_auth_fmterr(entry_p, ADM_ERR_OPENACL, 
			    class);
			break;
		}
		xdrstdio_create(&out_xdrs, np, XDR_ENCODE);
	
				/* Both files are open and locked */
		if (!xdr_acl_version(&in_xdrs, &version_header)) {
			status = adm_auth_fmterr(entry_p, ADM_ERR_AUTHREAD, 
			    ERRSTR, P(class), P(c_version), P(method));
			ADM_DBG("S",("AthImp: Read acl%s", ERRSTR));
			break;
		}
	
				/* Version was read: check it */
		if (version_header.version != ADM_AUTH_VERSION) {
			status = adm_auth_fmterr(entry_p, ADM_ERR_WRONGVER,
			    version_header.version);
			ADM_DBG("S",("AthImp: Wrong ACL ver in write"));
			break;
		}
	
		if (!xdr_acl_version(&out_xdrs, &version_header)) {
			status = adm_auth_fmterr(entry_p, ADM_ERR_AUTHWRITE, 
			    ERRSTR, P(class), P(c_version), P(method));
			break;
		}
	
		old_entry_p = &old_entry;
	
		while (read_auth_entry(&in_xdrs, old_entry_p) == ADM_AUTH_OK) {
			if (!STREQ(old_entry_p->name, method))	{
				num_wtn = write_auth_entry(&out_xdrs, 
				    old_entry_p);
			} else {
				found = 1;
				num_wtn = 1;
				/* XXX A bit obscure.  Needed for test below */
			}
	
			if (num_wtn != 1)	{
				status = adm_auth_fmterr(entry_p, 
				    ADM_ERR_AUTHWRITE, ERRSTR, P(class), 
				    P(c_version), P(method));
				break;
			}
		}	/* done copying file: done or broke */
	
		if (found == 0)	{
			status = adm_auth_fmterr(entry_p, ADM_ERR_NOAUTHMETHOD,
			    P(method), P(path));
		}
		break;
	}	

	if (np != (FILE *)NULL)	{
		(void) fclose(np);
		xdr_destroy(&out_xdrs);
	}
	if (op != (FILE *)NULL)	{
		(void) fclose(op);
		xdr_destroy(&in_xdrs);
	}
	if (status == ADM_AUTH_OK) {
		if (rename(temp_path, path) != 0)	{
			status = adm_auth_fmterr(entry_p, ADM_ERR_RENAME,
			    P(path));
		}
	}
	(void) mutex_unlock(class, mp, entry_p);
	return (status);
}


