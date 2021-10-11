/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)adm_amsl_auth.c	1.25	92/09/02 SMI"

/*
 * FILE:  auth.c
 *
 *	Admin Framework class agent routines for handling authentication
 *	and authorization in the class agent.
 */

#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <netmgt/netmgt.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"
#include "adm_auth_impl.h"

/* Static internal function prototype definitions */
static int auth_chk_system(char *);
static int auth_get_group(struct amsl_req *);
static int auth_get_gidlist(uid_t, u_int *, gid_t []);

/*
 * -------------------------------------------------------------------
 *  set_local_auth - Initialize authentication data from local process.
 *	Accepts a pointer to the request structure.
 *	Stores the local user and group identity in the request
 *	authorization structure and sets the action flag to local
 *	authentication.
 *	Returns 0 if authentication data retrieved; otherwise, returns
 *	an error status code and puts an error in formatted error struct.
 * -------------------------------------------------------------------
 */

int
set_local_auth(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	struct amsl_auth *ap;	/* Local pointer to amsl auth structure */
	int   tnum;		/* Temp integer variable */
	int   stat;		/* Local error status code */

	/* Make sure we have an authentication structure */
	if (reqp->authp == (struct amsl_auth *)NULL) {
		stat = amsl_err(reqp, ADM_ERR_AMSL_AUTHINFO,
		    amsl_ctlp->server_hostname, reqp->class_name,
		    (reqp->class_version == NULL ? "" : reqp->class_version),
		    reqp->method_name);
		return (stat);
	}
	ap = reqp->authp;

	stat = 0;
	ap->auth_flag = (u_int)AMSL_AUTH_LOCAL_IDS;
	ap->auth_type = (u_int)ADM_AUTH_UNSPECIFIED;
	ap->auth_flavor = (u_int)ADM_AUTH_UNSPECIFIED;

	/* Get local process effective user and group identity */
	ap->auth_uid = geteuid();
	ap->auth_gid = getegid();

	/* Get list of groups for local process user identity */
	ap->auth_gid_entries = (u_int)0;
	if ((tnum = getgroups((int)0, ap->auth_gid_list)) > 0) {
		if (tnum > (int)NGRPS)
			tnum = (int)NGRPS;
		ap->auth_gid_entries = tnum;
		(void) getgroups(ap->auth_gid_entries, ap->auth_gid_list);
	}

	/* Build a common principal name from the local user identity */
	(void) adm_auth_loc2cpn(ap->auth_uid, &ap->auth_cpn);

	ADM_DBG("s", ("Auth:   flavor=local, uid=%ld, gid=%ld, netname=local",
	    (long)ap->auth_uid, (long)ap->auth_gid));

	/* Return success */
	return (0);
}

/*
 * -------------------------------------------------------------------
 *  get_auth - Retrieve authentication data from SNM agent services.
 *	Stores the info in the request authorization structure and
 *	sets the action flag indicating info retrieved.
 *	Returns 0 if authentication data retrieved; otherwise, returns
 *	an error status code and puts an error in formatted error struct.
 * -------------------------------------------------------------------
 */

int
get_auth(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	struct amsl_auth *ap;	/* Local pointer to amsl auth structure */
	Adm_auth_cpn *cpnp;	/* Local pointer to client cpn */
	char   netname[MAXNETNAMELEN+1]; /* Client host name in cred */
	char   tbuff[MAXNETNAMELEN+1];	/* Temporary buffer */
	char  *hp;		/* Local pointer to host name */
	char  *dp;		/* Local pointer to domain name */
	int    stat;		/* Local error status code */
	int    temp;		/* Local temporary integer */
	int    i;		/* Iteration variable */

	/* Make sure we have an authentication structure */
	if (reqp->authp == (struct amsl_auth *)NULL) {
		stat = amsl_err(reqp, ADM_ERR_AMSL_AUTHINFO,
		    amsl_ctlp->server_hostname, reqp->class_name,
		    (reqp->class_version == NULL ? "" : reqp->class_version),
		    reqp->method_name);
		return (stat);
	}
	ap = reqp->authp;

	stat = 0;
	ap->auth_flavor = (u_int)0;
	ap->auth_gid_entries = (u_int)NGRPS;
	netname[0] = '\0';
	ap->auth_flavor = _netmgt_get_auth_flavor(&ap->auth_uid,
	    &ap->auth_gid, &ap->auth_gid_entries, ap->auth_gid_list,
	    (u_int)MAXNETNAMELEN, netname);

	ADM_DBG("s", ("Auth:   flavor=%d, uid=%ld, gid=%ld, netname=%s",
	    ap->auth_flavor, (long)ap->auth_uid, (long)ap->auth_gid,
	    netname));

	/*
	 * Got authentication information from RPC credentials.
	 * Make following additional checks:
	 *
	 *	DES:	Generate a common principal name from netname.
	 *
	 *		If uid = 0, we have a root client identity.
	 *		If host name and domain name are the same as our
	 *		server system, set authenticated uid = 0 (root).
	 *		If not same host and domain, set authenticated
	 *		uid to UID_NOBODY.
	 *
	 *	UNIX:	Create a UNIX flavor netname from the client
	 *		credential uid and host name, and from the
	 *		client domain name.  Use this netname to generate
	 *		a common principal name.
	 *
	 *		If uid = 0, we have a root client identity.
	 *		If host name and domain name are the same as our
	 *		server system, set authenticated uid = 0 (root).
	 *		If not same host and domain, set authenticated
	 *		uid to UID_NOBODY.
	 *
	 *	NONE:	No client identity, so mark common principal name
	 *		as UID_NOBODY.
	 *
	 * For any client, if local uid is UID_NOBODY, mark the auth
	 * info as having no local identity for later authorization
	 * checking.
	 */

	cpnp = &ap->auth_cpn;
	adm_auth_clear_cpn(cpnp);
	cpnp->context = ADM_CPN_USER;
	if (! (ap->auth_flag & AMSL_AUTH_OFF))
		switch (ap->auth_flavor) {

		case AUTH_DES:			/* DES authentication */
			(void) adm_auth_net2cpn(ap->auth_flavor, netname,
			    cpnp);
			if (cpnp->ugid == 0) {
				ap->auth_flag |= AMSL_AUTH_ROOT_ID;
				if ((cpnp->signature | ADM_CPN_HOST_MASK)
				    == ADM_CPN_HOST_MASK) {
					hp = &cpnp->name[cpnp->role_off];
					if (auth_chk_system(hp) != 0) {
						ap->auth_uid = UID_NOBODY;
				ADM_DBG("s", ("Auth:   Non-local root identity demoted"));
					}
				} else
					ap->auth_uid = UID_NOBODY;
			}
			break;

		case AUTH_UNIX:			/* UNIX authentication */
			if (reqp->client_domain == (char *)NULL)
				dp = "??";	/* Unknown domain! */
			else
				dp = reqp->client_domain;
			(void) sprintf(tbuff, "%ld.%s@%s", (long)ap->auth_uid,
			    netname, dp);
			(void) adm_auth_net2cpn(ap->auth_flavor, tbuff, 
			    cpnp);
			if (cpnp->ugid == 0) {
				ap->auth_flag |= AMSL_AUTH_ROOT_ID;
				if ((cpnp->signature | ADM_CPN_HOST_MASK)
				    == ADM_CPN_HOST_MASK) {
					hp = &cpnp->name[cpnp->role_off];
					if (auth_chk_system(hp) != 0) {
						ap->auth_uid = UID_NOBODY;
				ADM_DBG("s", ("Auth:   Non-local root identity demoted"));
					}
				} else
					ap->auth_uid = UID_NOBODY;
			}

			/* Add checks for accepting client credentials
			 * for weak authentication:
			 *	if uid does not exist on server system,
			 *		demote uid and gid to nobody
			 *	if uid exists but gid does not exist,
			 *		demote gid to nobody
			 */

			if (ap->auth_uid != UID_NOBODY) {
				temp = auth_get_gidlist(ap->auth_uid,
				    &ap->auth_gid_entries,
				    ap->auth_gid_list);
				if (temp == -1) {
					ADM_DBG("s", ("Auth:   Uid %ld does not exist on this system",
					    ap->auth_uid));
					ap->auth_uid = UID_NOBODY;
					ap->auth_gid = GID_NOBODY;
				}
			}
			if (ap->auth_uid != UID_NOBODY) {
				for (i=0; i < ap->auth_gid_entries; i++)
					if (ap->auth_gid == ap->auth_gid_list[i])
						break;
				if (i >= ap->auth_gid_entries) {
					ADM_DBG("s", ("Auth:   Uid %ld not a member of group %ld on this system",
					    ap->auth_uid, ap->auth_gid));
					ap->auth_gid = GID_NOBODY;
				}
			}
			break;

		case AUTH_NONE:
			ap->auth_uid = UID_NOBODY;
			ap->auth_gid = GID_NOBODY;

		default:			/* Other authentication  */
			ap->auth_uid = UID_NOBODY;
			ap->auth_gid = GID_NOBODY;
			break;
		}				/* End of switch */

	/* If we have no valid local uid, mark it for authorization */
	if (ap->auth_uid == UID_NOBODY)
		ap->auth_fail = AMSL_AUTH_FAIL_NOUID;

	/* Set flag indicating we got the client identity */
	ap->auth_flag |= AMSL_AUTH_GOT_IDS;

	/* Return status */
	return (stat);
}

/*
 * -------------------------------------------------------------------
 *  check_auth - Check authorization to execute the method.
 *	Retrieves authorization information from Object Manager and
 *	checks the following:
 *		A strong enough authentication type was used.
 *		Client identity is authorized to execute the method.
 *		The appropriate set identity is retrieved for the method.
 *	Returns 0 if authorization checks passed; returns -1 if checks
 *	failed; otherwise, returns an error status code and puts an
 *	error in formatted error structure.
 * -------------------------------------------------------------------
 */

int
check_auth(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	struct amsl_auth *ap;	/* Local pointer to amsl auth structure */
	struct Adm_auth_entry auth_info_entry;	/* Local auth entry buff */
	Adm_auth_entry *auth_ep; /* Pointer to auth entry buffer */
	Adm_auth *auth_ip;	/* Pointer to auth info structure */
	Adm_acl *acl_ip;	/* Pointer to ACL structure */
	char *classdflt;	/* Local pointer to class default name */
	char *classvers;	/* Local pointer to class version string */
	char   tbuff[ADM_AUTH_NAMESIZE+1]; /* Buffer for cpn string */
	u_short check_perms;	/* Local check permissions */
	int   stat;		/* Local error status code */

	/* Do some local set up */
	if (reqp->class_version == (char *)NULL)
		classvers = "";
	else
		classvers = reqp->class_version;

	/* Make sure we have an authentication structure */
	if (reqp->authp == (struct amsl_auth *)NULL) {
		stat = amsl_err(reqp, ADM_ERR_AMSL_AUTHINFO,
		    amsl_ctlp->server_hostname, reqp->class_name,
		    classvers, reqp->method_name);
		return (stat);
	}
	ap = reqp->authp;

	/* If security checking turned off, get agent identity and return */
	if (ap->auth_flag & AMSL_AUTH_OFF) {
		ap->auth_sid_uid = geteuid();
		ap->auth_sid_gid = getegid();
		ADM_DBG("s", ("Auth:   Security checking off for agent"));
		return (0);
	}

	/* Retrieve the security info for this method from the OM */
	auth_ep = &auth_info_entry;
	auth_ep->errcode = 0;
	auth_ep->errmsgp = (char *)NULL;
	stat = adm_auth_read(reqp->class_name, reqp->class_version,
	    reqp->method_name, auth_ep);
	if (stat == ADM_ERR_NOAUTHMETHOD) {
		classdflt = ADM_AUTH_DEFAULT_INFO;
		stat = adm_auth_read(reqp->class_name, reqp->class_version,
		    classdflt, auth_ep);
	}
	if (stat != 0) {
		(void) adm_err_fmt(reqp->errp, stat, ADM_ERR_SYSTEM,
		    ADM_FAILCLEAN, auth_ep->errmsgp);
		(void) free(auth_ep->errmsgp);
		auth_ep->errcode = 0;
		auth_ep->errmsgp = (char *)NULL;
		return (stat);
	}
	auth_ip = &auth_ep->auth_info;
	acl_ip = &auth_ip->acl;

	/*
	 * Check the client request authentication flavor for strength
	 * and for a match if method requires specific auth flavors.
	 * The strength must be equal to or greater than either the
	 * system wide strength specifed in the admind CLI or the method's
	 * strength specified in the security entry; whichever is greater.
	 * If the method security entry specifies a specific auth flavor,
	 * we ignore the system wide strength.
	 *
	 * If all right, set the authentication type and use client
	 * identity from the RPC credentials.
	 * If not all right, we set the required authentication type
	 * and the list of valid authentication flavors in preference order.
	 * Mark the client demoted and ignore the client identity (we
	 * may still have access to the method under the nobody identity).
	 * If the authorization check as nobody fails, then the verify
	 * routine will still accept the request but cause the dispatch
	 * routine to send back a weak authentication report to the client,
	 * listing the authentication flavors to use in a retry request.
	 *
	 * NOTE: The client identity may have already been demoted from the
	 *	 get_auth routine.  If so, the reason will be set in the
	 *	 auth_fail flag in the auth structure.
	 *
	 * NOTE: Skip this check for local request dispatch; that is,
	 *	 agent code running in client process so authentication
	 *	 assumed.
	 */

	if (ap->auth_flag & AMSL_AUTH_LOCAL_IDS) {
		ADM_DBG("s", ("Auth:   Local request authenticated"));
		stat = ADM_AUTH_OK;
	} else {
		ADM_DBG("s", ("Auth:   Checking authentication, flavor=%d",
		    ap->auth_flavor));
		ap->auth_flavor_entries = ADM_AUTH_MAXFLAVORS;
		stat = adm_auth_chkauth(auth_ip, ap->auth_sys_type,
		    ap->auth_sys_flavor, ap->auth_flavor, &ap->auth_type,
		    &ap->auth_flavor_entries, ap->auth_flavor_list);
	}
	ap->auth_flag |= AMSL_AUTH_CHK_AUTH;
	if (stat != ADM_AUTH_OK) {
		if (stat == ADM_AUTH_DENIED)
			ap->auth_fail |= AMSL_AUTH_FAIL_WEAK;
		else if (stat == ADM_AUTH_WRONG_FLAVOR)
			ap->auth_fail |= AMSL_AUTH_FAIL_WRONG;
		else {
			stat = amsl_err(reqp, ADM_ERR_AMSL_AUTHCHECK,
			    amsl_ctlp->server_hostname, reqp->class_name,
			    classvers, reqp->method_name);
			return (stat);
		}
		ap->auth_flag |= AMSL_AUTH_DEMOTED;
		ap->auth_uid = (uid_t)UID_NOBODY;
		ap->auth_gid = (gid_t)GID_NOBODY;
		ADM_DBG("s", ("Auth:   Authentication failed, identity demoted; flavor=%d",
		    ap->auth_flavor));
	}

	/*
	 * Check that the client's mapped identity is authorized to
	 * execute this method.  If not, return an authorization denied
	 * error, unless we had an auth too weak or wrong flavor error 
	 * from the authentication check above, in which case, return
	 * a too weak error (we distinguish the weak type versus wrong
	 * flavor error later when returning the weak authentication
	 * callback report).
	 *
	 * If there is NO local identity for this client, but the client
	 * was authenticated, check the "other" ACL entry for access.
	 *
	 * If there is NO local identity for this client and the client
	 * was NOT authenticated, check the "nobody" ACL entry for access.
	 *
	 * If the ACL is turned off for this method, skip this check and
	 * set the AMSL_AUTH_ACL_OFF flag in the authorization structure.
	 * Note that this means client is authorized, even in the face
	 * of an authentication check failure!
	 */

	stat = ADM_AUTH_OK;
	if (acl_ip->flags & ADM_ACL_OFF) {
		ADM_DBG("s", ("Auth:   ACL checking off for method"));
		ap->auth_flag |= AMSL_AUTH_ACL_OFF;
	} else {
		check_perms = (u_short)ADM_ACL_EXECUTE;
		if (ap->auth_type == ADM_AUTH_NONE)
			stat = adm_auth_chknobody(auth_ip, check_perms);
		else if (ap->auth_flag & AMSL_AUTH_DEMOTED)
			stat = adm_auth_chknobody(auth_ip, check_perms);
		else if (ap->auth_fail & AMSL_AUTH_FAIL_NOUID) {
			stat = adm_auth_chkother(auth_ip, check_perms);
			if (stat == ADM_AUTH_DENIED)
				stat = adm_auth_chknobody(auth_ip,
				    check_perms);
		} else {
			stat = adm_auth_chkacl(auth_ip, check_perms,
			    ap->auth_uid, ap->auth_gid, ap->auth_gid_entries,
			    ap->auth_gid_list);
			if (stat == ADM_AUTH_DENIED)
				stat = adm_auth_chknobody(auth_ip,
				    check_perms);
		}
	}
	ap->auth_flag |= AMSL_AUTH_CHK_ACL;
	if (stat != ADM_AUTH_OK) {
		if (stat == ADM_AUTH_DENIED) {
			ADM_DBG("s", ("Auth:   Authorization denied"));
			if (ap->auth_flag & AMSL_AUTH_DEMOTED) {
				stat = ADM_ERR_AMSL_AUTHWEAK;
				(void) amsl_err(reqp, stat,
				    amsl_ctlp->server_hostname, "",
				    reqp->class_name, classvers,
				    reqp->method_name);
			}
			else {
				ap->auth_fail |= AMSL_AUTH_FAIL_ACL;
				if (ap->auth_uid != UID_NOBODY) {
					stat = ADM_ERR_AMSL_AUTHDENY;
					tbuff[0] = '\0';
					(void) adm_auth_cpn2str((u_int)0,
					    &ap->auth_cpn, tbuff,
					    (u_int)ADM_AUTH_NAMESIZE);
					(void) amsl_err(reqp, stat,
					    amsl_ctlp->server_hostname,
					    tbuff, reqp->class_name,
					    classvers, reqp->method_name);
				}
				else if (ap->auth_flag & AMSL_AUTH_ROOT_ID) {
					stat = ADM_ERR_AMSL_AUTHDENY1;
					tbuff[0] = '\0';
					(void) adm_auth_cpn2str((u_int)0,
					    &ap->auth_cpn, tbuff,
					    (u_int)ADM_AUTH_NAMESIZE);
					(void) amsl_err(reqp, stat,
					    amsl_ctlp->server_hostname,
					    tbuff, reqp->class_name,
					    classvers, reqp->method_name);
				}
				else {
					stat = ADM_ERR_AMSL_AUTHDENY2;
					(void) amsl_err(reqp, stat,
					    amsl_ctlp->server_hostname,
					    reqp->class_name, classvers,
					    reqp->method_name);
				}

			}
		}
		else {
			stat = ADM_ERR_AMSL_ACLCHECK;
			(void) amsl_err(reqp, stat,
		 	    amsl_ctlp->server_hostname, reqp->class_name,
			    classvers, reqp->method_name);
		}
		return (stat);
	}

	ADM_DBG("s", ("Auth:   Authorization passed"));

	/*
	 * Passed authentication and authorization check.
	 * Get appropriate user and group identities for executing this
	 * method.  If the client passed over the group in which s/he
	 * wants to be in when executing the method, validate that this
	 * user is, in fact, a member of that group.  If not, return
	 * an error.
	 */

	ap->auth_sid_flag = auth_ip->set_flag;
	ap->auth_sid_uid = ap->auth_uid;
	ap->auth_sid_gid = ap->auth_gid;
	stat = adm_auth_chksid(auth_ip, &ap->auth_sid_uid, &ap->auth_sid_gid);
	ap->auth_flag |= AMSL_AUTH_CHK_SID;
	if (stat < ADM_AUTH_OK) {
		stat = amsl_err(reqp, ADM_ERR_AMSL_SIDCHECK,
		    amsl_ctlp->server_hostname, reqp->class_name,
		    classvers, reqp->method_name);
		return (stat);
	}

	/* If no client credentials or client demoted, error if client id */
	if ((ap->auth_type == ADM_AUTH_NONE) ||
	    (ap->auth_flag & AMSL_AUTH_DEMOTED) ||
	    (ap->auth_fail & AMSL_AUTH_FAIL_NOUID))
		if (stat != ADM_AUTH_RESET) {
			ap->auth_fail |= AMSL_AUTH_FAIL_NOSETID;
			ADM_DBG("s", ("Auth:   No client identity for method"));
			stat = amsl_err(reqp, ADM_ERR_AMSL_CLIENTID,
			    amsl_ctlp->server_hostname, reqp->class_name,
			    classvers, reqp->method_name);
			return (stat);
		}

	/* If client identity is being used, check if a group name passed */
	if (stat != ADM_AUTH_RESET)
		if ((stat = auth_get_group(reqp)) != 0) {
			ADM_DBG("s", ("Auth:   Client group invalid; group=%s",
			    reqp->client_group_name));
			return (stat);
		}

	ADM_DBG("s", ("Auth:   Client identity set; uid=%ld gid=%ld",
	    (long)ap->auth_sid_uid, (long)ap->auth_sid_gid));

	/* Return with success */
	return (0);
}

/*
 * -------------------------------------------------------------------
 *  auth_chk_system - Check client's system name with local system name.
 *	Returns 0 if client system name is not null and matches the
 *	local system name; otherwise, returns -1.
 *	NOTE: A null string is NOT valid for a host name!
 * -------------------------------------------------------------------
 */

int
auth_chk_system(
	char  *sname)		/* Name of system to be checked */
{
	char   lname[SYS_NMLN+1]; /* Buffer for local system name */

	/* If the system name to be checked is null, return error */
	if (sname == (char *)NULL)
		return (-1);
	if (! strcmp(sname, ""))
		return (-1);

	/* Get the local system name */
	if (sysinfo((int)SI_HOSTNAME, lname, (long)(SYS_NMLN+1)) <= 0)
		return (-1);

	/* Compare system names.  If no match, return error */
	if (strcmp(sname, lname))
		return (-1);

	/* Match.  Return success */
	return (0);
}

/*
 * -------------------------------------------------------------------
 *  auth_get_group - Convert client group name to local group gid.
 *	Converts group name to local group identifier and checks that
 *	client's user identity on server system is a member of that group.
 *	Returns 0 if successful and resets the set identity group in the
 *	request's authorization structure; otherwise, returns an error
 *	code and error message in the request error structure.
 * -------------------------------------------------------------------
 */

int
auth_get_group(
	struct amsl_req *reqp)	/* Pointer to amsl request structure */
{
	struct amsl_auth *ap;	/* Local pointer to amsl auth structure */
	struct passwd *pwbuffp;	/* Local pointer to passwd entry */
	struct group  *grbuffp;	/* Local pointer to group entry */
	gid_t  t_gid;		/* Local group gid */
	char  *t_user;		/* Local user name */
	char  *classvers;	/* Local class version string pointer */
	char **tpp;		/* Pointer to null-term list of strings */
	int    stat;		/* Local error status code */

	/* Set some local pointers */
	ap = reqp->authp;
	if (reqp->class_version == (char *)NULL)
		classvers = "";
	else
		classvers = reqp->class_version;

	/* See if we were passed a group name by the client */
	if (reqp->client_group_name == (char *)NULL)
		return (0);
	if (! (strcmp(reqp->client_group_name, "")))
		return (0);

	/* Get the gid for this group on the server system */
	grbuffp = getgrent();
	grbuffp = getgrnam(reqp->client_group_name);
	if (grbuffp == (struct group *)NULL) {
		ap->auth_fail |= AMSL_AUTH_FAIL_NOGRP;
		stat = amsl_err(reqp, ADM_ERR_AMSL_AUTHGROUP,
		    amsl_ctlp->server_hostname, reqp->client_group_name,
		    reqp->class_name, classvers, reqp->method_name);
		(void) endgrent();
		return (stat);
	}
	t_gid = grbuffp->gr_gid;
	(void) endgrent();
  
	/* Get the user name for the client on the server system */
	pwbuffp = getpwuid(ap->auth_sid_uid);
	if (pwbuffp == (struct passwd *)NULL) {
		ap->auth_fail |= AMSL_AUTH_FAIL_BADGRP;
		stat = amsl_err(reqp, ADM_ERR_AMSL_AUTHUSER,
		    amsl_ctlp->server_hostname, (long)ap->auth_sid_uid,
		    reqp->class_name, classvers, reqp->method_name);
		(void) endpwent();
		return (stat);
	}
	t_user = pwbuffp->pw_name;
	(void) endpwent();

	/* Now make sure user exists in group */
	tpp = grbuffp->gr_mem;
	while (*tpp != (char *)NULL) {
		if (! (strcmp(*tpp, t_user)))
			break;
		tpp++;
	}
	if (*tpp == (char *)NULL) {
		ap->auth_fail |= AMSL_AUTH_FAIL_BADGRP;
		stat = amsl_err(reqp, ADM_ERR_AMSL_AUTHUSRGRP,
		    amsl_ctlp->server_hostname, reqp->client_group_name,
		    t_user, reqp->class_name, classvers, reqp->method_name);
		return (stat);
	}

	/* Reset set identity group to client specified group */
	ap->auth_sid_gid = t_gid;

	/* return success */
	return (0);
}

/*
 * -------------------------------------------------------------------
 *  auth_get_gidlist - Get list of supplementary group IDs for user.
 *	Accepts the user uid, a pointer to an array of gid slots, and
 *	a pointer to an integer number of groups returned.
 *	Access the group information for the given user.
 *	Returns 0 if successful and returns a list of alternate gid's.
 *	otherwise, returns a -1 if invalid uid or +1 if more gids than
 *	can fit into the list (returns as many as possible in list).. 
 * -------------------------------------------------------------------
 */

int
auth_get_gidlist(
	uid_t  uid,		/* User uid */
	u_int *numgidp,		/* Address to return number of gid's */
	gid_t gid_list[])	/* List of gid slots */
	
{
	struct passwd *pwbuffp;	/* Local pointer to passwd entry */
	gid_t  t_gid;		/* Local group gid */
	char  *t_user;		/* Local user name */
	int    tnum;		/* Temporary integer */
	int    stat;		/* Return status code */

	/*
	 * Use the passwd file to get userid associated with uid.  Note
	 * that we place the base group gid in this list as well in case
	 * the major group in the authentication credentials is not the
	 * base group for this user.
	 * If uid is not defined, we return no supplementary groups and
	 * an error code (-1).
	 * If we fail to get the supplementary groups or there are more
	 * groups than fit in the array, we return a warning code (1).
	 */

	*numgidp = 0;
	pwbuffp = getpwuid(uid);
	if (pwbuffp == (struct passwd *)NULL) {
		(void) endpwent();
		return (-1);
	}
	t_user = pwbuffp->pw_name;
	t_gid = pwbuffp->pw_gid;
	(void) endpwent();

	/* Set up group entries for this user, including base gid */
	if (initgroups(t_user, t_gid) != 0) {
		if (t_gid != GID_NOBODY) {
			*numgidp = 1;
			gid_list[0] = t_gid;
		}
		return (1);
	}

	/* Get the supplementary groups */
	stat = 0;
	if ((tnum = getgroups((int)0, gid_list)) > 0) {
		if (tnum > (int)NGRPS) {
			tnum = (int)NGRPS;
			stat = 1;
		}
		*numgidp = tnum;
		(void) getgroups(tnum, gid_list);
	}

	/* Return */
	return (stat);
}
