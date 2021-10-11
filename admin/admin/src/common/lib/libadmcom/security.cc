#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)security.cc	1.47 9/6/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/security.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)security.cc  1.47  91/09/06
 *
 *  Copyright (c) 1989 Sun Microsystems, Inc.  All Rights Reserved.
 *  Sun considers its source code as an unpublished, proprietary trade 
 *  secret, and it is available only under strict license provisions.  
 *  This copyright notice is placed here only to protect Sun in the event
 *  the source is deemed a published work.  Dissassembly, decompilation, 
 *  or other means of reducing the object code to human readable form is 
 *  prohibited by the license agreement under which this code is provided
 *  to the user or company in possession of this copy.
 * 
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
 *  Government is subject to restrictions as set forth in subparagraph 
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software 
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
 *  NASA FAR Supplement.
 *
 *  Comments:	security handling routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"
#include <pwd.h>
#ifdef _SVR4_
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <sys/param.h>
#else
#define UID_NOBODY 0
#define GID_NOBODY 0
#endif

static bool_t _netmgt_inNetgroup (struct passwd *pw, char *netgrp) ;
static bool_t _netmgt_nisUp (void) ;
  
/* -------------------------------------------------------------------
 *  NetmgtEntity::setSecurityLevels - set agent security levels
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtEntity::setSecurityLevels (char *name)
				 // application service name
{

  NETMGT_PRN (("NetmgtEntity::setSecurityLevels\n"));

  // get application security levels from configuration file
  char *value = this->getConfig (name);

  // default is no security
  if (!value)
    {
      this->readSecurity = NETMGT_NO_SECURITY;
      this->rdwrSecurity = NETMGT_NO_SECURITY;
      NETMGT_PRN (("security: read security == %d\n", 
		   this->readSecurity));
      NETMGT_PRN (("security: read/write security == %d\n", 
		   this->rdwrSecurity));
      return TRUE;
    }

  // the first value is the read security level
  char level [NETMGT_NAMESIZ] ;	 // security level buffer
  register char *cp = value ;	 // utitity pointer
  register char *cp1 = level ;	 // another utility pointer
  while (*cp && !isspace (*cp))
    {
      *cp1 = *cp;
      cp++;
      cp1++;
    }
  *cp1 = '\0';
  this->readSecurity = (NetmgtSecurityLevel) atoi (level);
  NETMGT_PRN (("security: read security == %d\n", 
	       this->readSecurity));

  //  verify read security level is within bounds 
  if (this->readSecurity > NETMGT_MAX_SECURITY)
    {
      NETMGT_PRN (("security: invalid read security level: %d\n", 
		   this->readSecurity));
      _netmgtStatus.setStatus (NETMGT_BADSECURITY, 0, NULL);
      return FALSE;
    }

  // if there's no read/write security level specified; 
  // set the read/write security level to the read security 
  // level
  bool_t isRdwrLevel = FALSE;
  while (*cp)
    {
      if (isdigit (*cp))
	{
	  isRdwrLevel = TRUE;
	  break;
	}
      cp++;
      cp1++;
    }
  if (!isRdwrLevel)
    {
      this->rdwrSecurity = this->readSecurity;
      return TRUE;
    }

  // get read/write security level
  cp1 = level ;
  while (*cp && !isspace (*cp))
    {
      *cp1 = *cp;
      cp++;
      cp1++;
    }
  *cp1 = '\0';
  this->rdwrSecurity = (NetmgtSecurityLevel) atoi (level);
  NETMGT_PRN (("security: read/write security == %d\n", 
	       this->rdwrSecurity));

  //  verify read/write security level is within bounds 
  if (this->rdwrSecurity > NETMGT_MAX_SECURITY)
    {
      NETMGT_PRN (("security: invalid read/write security level: %d\n", 
		   rdwrSecurity));
      _netmgtStatus.setStatus (NETMGT_BADSECURITY, 0, NULL);
      return FALSE;
    }
  return TRUE;
}

/* ----------------------------------------------------------------------
 *  NetmgtDispatcher::verifyCredentials - verify requestor's credentials
 *	returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
NetmgtEntity::verifyCredentials (struct svc_req *rqst, 
#ifdef _SVR4_
				 register SVCXPRT *xprt,
				 uid_t *uid,
				 gid_t *gid,
				 int *gidlen,
				 gid_t gidlist [],
				 char netname [])
#else
				 register SVCXPRT *xprt,
				 int *uid,
				 int *gid,
				 int *gidlen,
				 int gidlist [],
				 char netname [])
#endif _SVR4_
     // server request 
     // server transport handle 
     // user ID pointer
     // group ID pointer
     // group ID list length pointer
     // group ID list
     // max host name length
     // client netname
{

  int i;

  NETMGT_PRN (("security: NetmgtDispatcher::verifyCredentials\n")) ;

  assert (rqst != (struct svc_req *) NULL) ;
  assert (xprt != (SVCXPRT *) NULL) ;
#ifdef _SVR4_
  assert (uid != (uid_t *) NULL);
  assert (gid != (gid_t *) NULL);
  assert (gidlen != (int *) NULL);
  assert (gidlist != (gid_t *) NULL);
#else
  assert (uid != (int *) NULL);
  assert (gid != (int *) NULL);
  assert (gidlen != (int *) NULL);
  assert (gidlist != (int *) NULL);
#endif _SVR4_
  assert (netname != (char *) NULL);

  // clear error status
  _netmgtStatus.clearStatus ();

  // verify the requestor's credentials 
  struct authdes_cred *des_cred ;    // DES authentication credentials 
  struct authunix_parms *unix_cred ; // UNIX authentication credentials
  char hostname [NETMGT_NAMESIZ] ; // client hostname

  switch (rqst->rq_cred.oa_flavor)
    {
    case AUTH_UNIX:		// UNIX authentication
    case AUTH_SHORT:		// short hand UNIX authentication

      // get UNIX authentication credentials
      unix_cred = (struct authunix_parms *) rqst->rq_clntcred;
      *uid = unix_cred->aup_uid;
      *gid = unix_cred->aup_gid;
      *gidlen = unix_cred->aup_len;
      for (i = 0; i < *gidlen; i++)
	gidlist [i] = unix_cred->aup_gids [i];

      // return the client host name as the netname
      strncpy (netname, unix_cred->aup_machname, NETMGT_NAMESIZ);
      break;

    case AUTH_DES:		// DES authentication 

      // get DES authentication credentials
      des_cred = (struct authdes_cred *) rqst->rq_clntcred;
      NETMGT_PRN (("security: netname: %s\n", 
		   des_cred->adc_fullname.name));

      // We assume a regular user client if netname2user() succeeds
      // and a root client if netname2host() succeeds
      if (netname2user (des_cred->adc_fullname.name, 
			uid, 
			gid, 
			gidlen,
			gidlist))
	{
	  NETMGT_PRN (("security: regular user client uid = %d\n", *uid));
	  strncpy (netname, des_cred->adc_fullname.name, NETMGT_NAMESIZ);
	}
      else if (netname2host (des_cred->adc_fullname.name, 
			     hostname,
			     NETMGT_NAMESIZ))
	{
	  *uid = 0;
	  *gid = GID_NOBODY;
	  *gidlen = 0;
	  strncpy (netname, des_cred->adc_fullname.name, NETMGT_NAMESIZ);
	  NETMGT_PRN (("security: root client hostname = %s\n", hostname));
	}
      else
	{
	  NETMGT_PRN (("security: both netname2user and netname2host ")); 
	  NETMGT_PRN (("failed for: %s\n", des_cred->adc_fullname.name));
	  _netmgtStatus.setStatus (NETMGT_BADCREDENTIALS, 0, NULL);
	  *uid = UID_NOBODY;
	  *gid = GID_NOBODY;
	  *gidlen = 0;
	  strncpy (netname, des_cred->adc_fullname.name, NETMGT_NAMESIZ);
	  return FALSE;
	}

      break;

    case AUTH_NONE:		// no authorization 
    default:
      NETMGT_PRN (("security: no authentication\n"));
      *uid = UID_NOBODY;
      *gid = GID_NOBODY;
      *gidlen = 0;
      hostname [0] = '\0';
      if (this->readSecurity > NETMGT_NO_SECURITY ||
	  this->rdwrSecurity > NETMGT_NO_SECURITY)
	{
	  svcerr_weakauth (xprt);
	  _netmgtStatus.setStatus (NETMGT_WEAKCREDENTIALS, 0, NULL);
	  return FALSE;
	}
    }
  return TRUE;
}

/* ----------------------------------------------------------------------
 *  NetmgtDispatcher::verifyAuthorization - verify authorization
 *	returns TRUE if successful; otherwise returns FALSE
 * ----------------------------------------------------------------------
 */
bool_t
#ifdef _SVR4_
NetmgtEntity::verifyAuthorization (u_int type, uid_t uid)
#else
NetmgtEntity::verifyAuthorization (u_int type, int uid)
#endif _SVR4_
				// request type
				// user ID pointer
{

  NETMGT_PRN (("security: NetmgtDispatcher::verifyAuthorization\n")) ;

  // check if directory services are running 
  if (!_netmgt_nisUp ())
    {
      _netmgtStatus.setStatus (NETMGT_DIRSERVNOTUP, 0, NULL);
      return FALSE;
    }

  // get user name from uid 
  struct passwd *pw ;		  // password table entry 
  pw = getpwuid (uid);
  if (!pw)
    {
      NETMGT_PRN (("security: can't get user name\n"));
      _netmgtStatus.setStatus (NETMGT_BADCREDENTIALS, 0, NULL);
      return FALSE;
    }

  // verify that user is in the appropropriate security netgroup
  NetmgtSecurityLevel security ; // security level
  if (type == NETMGT_SET_REQUEST)
    security = this->rdwrSecurity;
  else
    security = this->readSecurity;

  switch (security)
    {
    case 0:			// no security 
      return TRUE;

    case 1:			// lowest security level 
      if (_netmgt_inNetgroup (pw, NETMGT_SECURITY_GROUP_ONE))
	{
	  NETMGT_PRN (("security: requestor in %s: %s\n",
		       "security netgroup", NETMGT_SECURITY_GROUP_ONE));
	  return TRUE;
	}
      // fall through 
    case 2:
      if (_netmgt_inNetgroup (pw, NETMGT_SECURITY_GROUP_TWO))
	{
	  NETMGT_PRN (("security: requestor in %s, %s\n",
		       "security netgroup", NETMGT_SECURITY_GROUP_TWO));
	  return TRUE;
	}
      // fall through 
    case 3:
      if (_netmgt_inNetgroup (pw, NETMGT_SECURITY_GROUP_THREE))
	{
	  NETMGT_PRN (("security: requestor in %s: %s\n",
		       "security netgroup", NETMGT_SECURITY_GROUP_THREE));
	  return TRUE;
	}
      // fall through 
    case 4:
      if (_netmgt_inNetgroup (pw, NETMGT_SECURITY_GROUP_FOUR))
	{
	  NETMGT_PRN (("security: requestor in %s: %s\n",
		       "security netgroup", NETMGT_SECURITY_GROUP_FOUR));
	  return TRUE;
	}
      // fall through 
    case 5:			// highest security level 
      if (_netmgt_inNetgroup (pw, NETMGT_SECURITY_GROUP_FIVE))
	{
	  NETMGT_PRN (("security: requestor in %s: %s\n",
		       "security netgroup", NETMGT_SECURITY_GROUP_FIVE));
	  return TRUE;
	}
      // fall through 
    }

  NETMGT_PRN (("security: bad credentials for security level %d\n",
	       security));
  _netmgtStatus.setStatus (NETMGT_BADCREDENTIALS, 0, NULL);
  return FALSE;
}

/* --------------------------------------------------------------------------
 *  _netmgt_inNetgroup - verify that requestor is in netmgt netgroup
 *      returns TRUE if successful; otherwise returns FALSE
 *---------------------------------------------------------------------------
 */
static bool_t
_netmgt_inNetgroup (struct passwd *pw, char *netgrp)
                       		// password table entry 
                  		// netgroup name 
{

  NETMGT_PRN (("security: _netmgt_inNetgroup\n"));

  assert (pw != (struct passwd *) NULL);
  assert (netgrp != (char *) NULL);

  if (!innetgr (netgrp, (char *) NULL, pw->pw_name, (char *) NULL))
    {
      NETMGT_PRN (("security: `%s' not in `%s' netgrp\n", pw->pw_name,
		   netgrp));
      return FALSE;
    }

  NETMGT_PRN (("security: user `%s' is in `%s' netgrp\n", pw->pw_name,
	       netgrp));
  return TRUE;
}

/* -------------------------------------------------------------------------
 *  _netmgt_nisUp - returns boolean value indicating whether
 *      directory services are running in local domain
 * -----------------------------------------------------------------------
 */
static bool_t
_netmgt_nisUp (void)
{

  char domain[256];		// domain name 

  NETMGT_PRN (("security: _netmgt_nisUp\n"));

#ifdef _SVR4_
  if (sysinfo (SI_SRPC_DOMAIN, domain, sizeof (domain)) == -1)
#else
  if (getdomainname (domain, sizeof (domain)) == -1)
#endif
    {
      NETMGT_PRN (("security: can't get domain name\n"));
      return FALSE;
    }

  return !yp_bind (domain);
}
