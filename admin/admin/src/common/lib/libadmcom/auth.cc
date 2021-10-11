#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)auth.cc	1.42 8/1/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/auth.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)auth.cc  1.42  91/08/01
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
 *  Comments:	DES authentication routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* -------------------------------------------------------------------
 *  _netmgt_set_auth_flavor - set request authentication flavor
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
_netmgt_set_auth_flavor (int flavor)
{
  NETMGT_PRN (("auth: _netmgt_set_auth_flavor\n"));

  // create and inititialize a manager object if necessary 
  if (!aNetmgtManager)
    {
      if (!_netmgt_init_rpc_manager ())
	return FALSE;
    }
  return aNetmgtManager->myRequest->setAuthFlavor (flavor);
}

/* -------------------------------------------------------------------
 *  _netmgt_get_auth_flavor - get request authentication flavor
 *	returns authentication flavor if successful; otherwise '-1'
 * -------------------------------------------------------------------
 */
int
#ifdef _SVR4_
_netmgt_get_auth_flavor (uid_t *uid, 
			 gid_t *gid, 
			 int *gidlen, 
			 gid_t gidlist [],
			 u_int maxnetnamelen,
			 char netname [])
#else
_netmgt_get_auth_flavor (int *uid, 
			 int *gid, 
			 int *gidlen, 
			 int gidlist [],
			 u_int maxnetnamelen,
			 char netname [])
#endif _SVR4_
{
  NETMGT_PRN (("auth: _netmgt_get_auth_flavor\n"));

  if (!aNetmgtDispatcher)
    {
      NETMGT_PRN (("auth: no agent instance\n"));
      _netmgtStatus.setStatus (NETMGT_NOAGENT, 0, NULL);
      return -1;
    }
  return aNetmgtDispatcher->myRequest->getAuthFlavor (uid, 
						      gid, 
						      gidlen, 
						      gidlist,
						      maxnetnamelen,
						      netname);
}

/* -------------------------------------------------------------------
 *  NetmgtRequest::getAuthFlavor - get request authentication flavor
 *	returns authentication flavor if successful; otherwise '-1'
 * -------------------------------------------------------------------
 */
int
#ifdef _SVR4_
NetmgtRequest::getAuthFlavor (uid_t *uid, 
			      gid_t *gid, 
			      int *gidlen, 
			      gid_t gidlist [],
			      u_int maxnetnamelen,
			      char netname [])
#else
NetmgtRequest::getAuthFlavor (int *uid, 
			      int *gid, 
			      int *gidlen, 
			      int gidlist [],
			      u_int maxnetnamelen,
			      char netname [])
#endif _SVR4_
{
  NETMGT_PRN (("auth: NetmgtRequest::getAuthFlavor\n"));

  *uid = this->info.uid;
  *gid = this->info.gid;
  *gidlen = this->info.gidlen;
  for (int i = 0; i < *gidlen; i++)
    gidlist [i] = this->info.gidlist [i];
  
  u_int maxCopy = (maxnetnamelen < NETMGT_NAMESIZ)
    ? maxnetnamelen : NETMGT_NAMESIZ;
  strncpy (netname, this->info.netname, maxCopy);

  return this->info.flavor;
}

/* -------------------------------------------------------------------
 *  NetmgtClient::authdesCreate - get client DES authentication handle
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtClient::authdesCreate (struct sockaddr_in *pname, u_int lifetime)
				// entity IP name pointer
				// handle lifetime
{
  struct hostent *hp ;		 // host table entry 
  char netname [MAXNETNAMELEN] ; // agent net name 
  char hostbuff [SYS_NMLN] ;	 // client host name buffer
  char *timehost ;		 // time host name

  NETMGT_PRN (("auth: NetmgtClient::authdesCreate\n")) ;

  assert (client != (CLIENT *) NULL) ;
  assert (pname != (struct sockaddr_in *) NULL) ;

  // reset internal error code 
  _netmgtStatus.clearStatus ();

  // get server host name 
  hp = gethostbyaddr ((char *) &pname->sin_addr, 
		      sizeof (struct in_addr),
		      AF_INET) ;
  if (!hp)
    {
      NETMGT_PRN (("auth: gethostbyaddr failed\n"));
      _netmgtStatus.setStatus (NETMGT_UNKNOWNADDRESS, 0, NULL);
      return FALSE;
    }
  NETMGT_PRN (("auth: agent hostname: %s\n", hp->h_name));

  // get the operating-system independent netname 
  if (!host2netname (netname, hp->h_name, (char *) NULL))
    {
      NETMGT_PRN (("auth: can't get server netname\n"));
      _netmgtStatus.setStatus (NETMGT_HOST2NETNAME, 0, NULL);
      return FALSE;
    }
  NETMGT_PRN (("auth: agent netname: %s\n", netname));

  // get client's hostname and check for client host = server host
  // if so, we do not need a timehost.  if not, make server the timehost
  // Note that we take a server host name of "localhost" as client host
  timehost = (char *)NULL;
  if (strcasecmp(hp->h_name, NETMGT_LOCALHOST)) {
	hostbuff[0] = '\0';
	(void) sysinfo(SI_HOSTNAME, hostbuff, (long)SYS_NMLN);
	hostbuff[SYS_NMLN - 1] = '\0';
	if (strcasecmp(hp->h_name, hostbuff))
		timehost = hp->h_name;
  }
  NETMGT_PRN (("auth: timehost = %s\n", (timehost == NULL?"(nil)":timehost)));

  // get DES authentication credentials
  this->client->cl_auth = authdes_seccreate (netname, lifetime, timehost,
	(des_block *)NULL);

  // if error, set error message and return false
  if (!this->client->cl_auth)
    {
      NETMGT_PRN (("auth: authdes_create failed\n"));
      _netmgtStatus.setStatus (NETMGT_CANTCREATEDESAUTH, 0, NULL);
      return FALSE;
    }
  return TRUE;
}
