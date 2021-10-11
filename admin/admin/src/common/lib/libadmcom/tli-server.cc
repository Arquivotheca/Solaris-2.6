#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)tli-server.cc	1.12 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/tli-server.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)tli-server.cc  1.12  91/05/05
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
 *  Comments:   streams RPC service initialization routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#ifdef _SVR4_

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"
#include <sys/stropts.h>

/* ------------------------------------------------------------------------
 *  NetmgtAgent::registerTliService - register agent streams RPC service
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtAgent::registerTliService (void)
{
  
  NETMGT_PRN (("tli-server: NetmgtAgent::registerTliService\n"));

#ifdef notdef
  //  unregister the server RPC with portmapper 
  (void) rpcb_unset (this->program, this->version, (struct netconfig*)NULL);
#endif notdef
  
  char moduleName [FMNAMESZ + 1];	// streams module name
  
  // were we started from inetd ?
  if (!ioctl(0, I_LOOK, moduleName) &&
      (strcmp(moduleName, "sockmod") == 0 || 
       strcmp(moduleName, "timod") == 0))
    {
      char *netid;
      struct netconfig *netconfig; // network configuration info
      netconfig = (struct netconfig *)NULL;
      
      // we were started by inetd
      this->flags |= NETMGT_STARTED_BY_INETD;
      
      // get transport name
      netid = getenv ("NLSPROVIDER");
      if (!netid) 
	{
	  NETMGT_PRN (("tli-server: can't get transport name\n"));
	}
      // get transport information
      else
	{
	  netconfig = this->myGetnetconfigent (netid);
	  if (!netconfig)
	    NETMGT_PRN (("tli-server: can't get transport information\n"));
	}
      
      // make sure right module is on stack
      if (strcmp (moduleName, "sockmod") == 0) 
	{
	  if (ioctl (0, I_POP, 0) || ioctl (0, I_PUSH, "timod")) 
	    {
	      NETMGT_PRN (("tli-server: can't push \"timod\"\n"));
	      _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 
				       0, 
				       "can't push \"timod\"");
	      return FALSE;
	    }
	}
      
      // get server transport handle and register service 
      SVCXPRT *transport ;		// server tranport handle 
      
      transport = svc_tli_create (0, netconfig, (struct t_bind *)NULL, 0, 0);
      if (!transport)
	{
	  NETMGT_PRN (("tli-server: can't create UDP transport\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCUDPCREATE, 0, NULL);
	  return FALSE;
	}
      
      if (netconfig)
	freenetconfigent (netconfig);
      
      if (!svc_reg (transport, 
		    this->program,
		    this->version,
                    (SIG_PFV2) _netmgt_receiveRequest,
		    (struct netconfig *)NULL))
	{
	  NETMGT_PRN (("tli-server: can't register RPC server\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return FALSE;
	}
    }
  
  // we were started from the shell command line
  else
    {
      if (!svc_create ((SIG_PFV2) _netmgt_receiveRequest, 
		       this->program,
		       this->version,
		       "netpath"))
	{
	  NETMGT_PRN (("tli-server: can't register RPC server\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return FALSE;
	}
    }
  return TRUE;
}
  

/* ------------------------------------------------------------------------
 *  NetmgtRendez::registerTliService - register agent streams RPC service
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------------
 */
bool_t
NetmgtRendez::registerTliService (void)
{

  NETMGT_PRN (("tli-server: NetmgtRendez::registerTliService\n"));
  
#ifdef notdef
  //  unregister the server RPC with portmapper 
  (void) rpcb_unset (this->program, this->version, (struct netconfig*)NULL);
#endif notdef
  
  char moduleName [FMNAMESZ + 1];	// streams module name
  
  // were we started from inetd ?
  if (!ioctl(0, I_LOOK, moduleName) &&
      (strcmp(moduleName, "sockmod") == 0 || 
       strcmp(moduleName, "timod") == 0))
    {
      char *netid;
      struct netconfig *netconfig;	// network configuration info
      
      // we were started by inetd
      this->flags |= NETMGT_STARTED_BY_INETD;
      
      // get transport name
      netid = getenv ("NLSPROVIDER");
      if (!netid) 
	{
	  NETMGT_PRN (("tli-server: can't get transport name\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 
				   0, 
				   "can't get transport name");
	  return FALSE;
	} 
      
      // get transport information
      netconfig = this->myGetnetconfigent (netid);
      if (!netconfig)
	{
	  NETMGT_PRN (("tli-server: can't get transport information\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 
				   0, 
				   "can't get transport information");
	  return FALSE;
	}
      
      // make sure right module is on stack
      if (strcmp (moduleName, "sockmod") == 0) 
	{
	  if (ioctl (0, I_POP, 0) || ioctl (0, I_PUSH, "timod")) 
	    {
	      NETMGT_PRN (("tli-server: can't push \"timod\"\n"));
	      _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 
				       0, 
				       "can't push \"timod\"");
	      return FALSE;
	    }
	}
      
      // get server transport handle and register service 
      SVCXPRT *transport ;		// server tranport handle 
      
      transport = svc_tli_create (0, netconfig, (struct t_bind *)NULL, 0, 0);
      if (!transport)
	{
	  NETMGT_PRN (("tli-server: can't create UDP transport\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCUDPCREATE, 0, NULL);
	  return FALSE;
	}
      
      if (netconfig)
	freenetconfigent (netconfig);
      
      if (!svc_reg (transport, 
		    this->program,
		    this->version,
                    (SIG_PFV2) _netmgt_receiveReport,
		    (struct netconfig *)NULL))
	{
	  NETMGT_PRN (("tli-server: can't register RPC server\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return FALSE;
	}
    }

  // we were started from the shell command line
  else
    {
      if (!svc_create ( (SIG_PFV2) _netmgt_receiveReport, 
		       this->program,
		       this->version,
		       "udp"))
	{
	  NETMGT_PRN (("tli-server: can't register RPC server\n"));
	  _netmgtStatus.setStatus (NETMGT_SVCREGISTER, 0, NULL);
	  return FALSE;
	}
    }
  return TRUE;
}
  
#endif _SVR4_
