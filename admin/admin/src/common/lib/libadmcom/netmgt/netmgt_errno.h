
/**************************************************************************
 *  File:	include/netmgt_errno.h
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
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
 *  SCCSID:	@(#)netmgt_errno.h  1.46  91/01/14
 *
 *  Comments:	SunNet Manager error codes
 *
 **************************************************************************
 */

#ifndef	_netmgt_errno_h
#define _netmgt_errno_h

typedef enum
{
  
  NETMGT_SUCCESS = 0,		/* success */

  /* agent defined errors */
  NETMGT_WARNING = 1,		/* non-fatal error */
  NETMGT_FATAL = 2,		/* fatal error */

  /* RPC failures */
  NETMGT_RPCFAILED = 3,		/* RPC failures */
  NETMGT_RPCTIMEDOUT = 4,	/* RPC timed out */

  /* send request failures */
  NETMGT_NOMANAGERHOSTNAME = 5, /* no manager hostname */
  NETMGT_NOAGENTHOSTNAME = 6,	/* no agent hostname */
  NETMGT_NORENDEZHOSTNAME = 7,	/* no rendezvous hostname */
  NETMGT_UNKNOWNHOST = 8,	/* unknown host */
  NETMGT_UNKNOWNADDRESS = 9,	/* unknown host address */
  NETMGT_BADPROTO = 10,		/* unknown transport protocol */
  NETMGT_CANTCREATECLIENT = 11,	/* can't creat client handle */
  NETMGT_NODISPATCH = 12,	/* no callback dispatch function */
  NETMGT_NULLSOCKET = 13,	/* NULL socket pointer */

  /* argument handling failures */
  NETMGT_BADINTERVAL = 14,	/* invalid sampling interval */
  NETMGT_BADCOUNT = 15,		/* invalid sampling count */
  NETMGT_BADTIMEOUT = 16,	/* invalid RPC timeout value */
  NETMGT_BADTIMESTAMP = 17,	/* invalid request timestamp */
  NETMGT_NOSYSTEMNAME = 18,	/* no system name */
  NETMGT_BADSYSTEMNAME = 19,	/* invalid system name */
  NETMGT_NOGROUPNAME = 20,	/* no object group name */
  NETMGT_BADGROUPNAME = 21,	/* invalid object group name */
  NETMGT_BADKEYNAME = 22,	/* invalid object key name */
  NETMGT_NOARGNAME = 23,	/* no argument name */
  NETMGT_UNKNOWNARGNAME = 24,	/* unknown argument name */
  NETMGT_NAME2BIG = 25,		/* name too long */
  NETMGT_NOARGTYPE = 26,	/* no argument type */
  NETMGT_UNKNOWNTYPE = 27,	/* unknown data type */
  NETMGT_NOARGLENGTH = 28,	/* no argument length ??? */
  NETMGT_NOARGVALUE = 29,	/* no argument value buffer */
  NETMGT_NORELOP = 30,		/* no relational operator */
  NETMGT_BADRELOP = 31,		/* invalid relational operator */
  NETMGT_NOTHRESH = 32,		/* no threshold value */
  NETMGT_NOARGBUF = 33,		/* no argument buffer */
  NETMGT_NOTHRESHBUF = 34,	/* no threshold buffer */
  NETMGT_NOPERFBUF = 35,	/* no performance buffer */
  NETMGT_NOEVENTBUF = 36,	/* no event-report buffer */
  NETMGT_NOERRORBUF = 37,	/* no error-report buffer */
  NETMGT_NOINFOBUF = 38,	/* no message information buffer */

  /* message handling failures */
  NETMGT_INVALIDMSG = 39,	/* invalid message type */
  NETMGT_ENCODEHEADER = 40,	/* can't encode message header */
  NETMGT_DECODEHEADER = 41,	/* can't decode message header */
  NETMGT_ENCODEARGLIST = 42,	/* can't encode argument list */
  NETMGT_DECODEARGLIST = 43,	/* can't decode argument list */
  NETMGT_MSG2BIG = 44,		/* message too large */

  /* security failures */
  NETMGT_HOST2NETNAME = 45,	/* host2netname(3R) failed */
  NETMGT_CANTCREATEDESAUTH = 46, /* authdes_create(3R) failed */
  NETMGT_DIRSERVNOTUP = 47,	/* directory services are not running */
  NETMGT_WEAKCREDENTIALS = 48,	/* weak credentials */
  NETMGT_BADCREDENTIALS = 49,	/* bad credentials */

  /* confirmation errors */
  NETMGT_CONFIRM2BIG = 50,	/* confirmation too large */

  /* agent initialization failures */
  NETMGT_NOAGENTNAME = 51,	/* no agent name */
  NETMGT_BADSECURITY = 52,	/* invalid agent security level */
  NETMGT_SVCUDPCREATE = 53,	/* can't create udp transport */
  NETMGT_SVCTCPCREATE = 54,	/* can't create tcp transport */
  NETMGT_PMAPUNSET = 55,	/* pmap_unset(3R) failed */
  NETMGT_SVCREGISTER = 56,	/* can't register sevice */

  /* agent request handling failures */
  NETMGT_UNKNOWNREQUEST = 57,	/* unknown request type */
  NETMGT_CANTCACHEREQUEST = 58,	/* can't cache request */
  NETMGT_NOREQUESTLOG = 59,	/* no request logfile */

  /* event rendezvous control failures */
  NETMGT_CREATERENDEZ = 60,	/* can't add rendezvous */
  NETMGT_DELETERENDEZ = 61,	/* can't delete rendezvous */

  /* activity control failures */
  NETMGT_NOSUCHREQUEST = 62,	/* can't find request */
  NETMGT_KILLREQUEST = 63,	/* can't kill request */
  NETMGT_DELETEREQUEST = 64,	/* can't delete request */
  NETMGT_INSERTACTIVITY = 65,	/* can't insert activity */
  NETMGT_DELETEACTIVITY = 66,	/* can't delete activity */
  NETMGT_UNKNOWNACTIVITY = 67,	/* no such activity */

  /* system failures */
  NETMGT_FORK = 68,		/* can't fork */
  NETMGT_MALLOC = 69,		/* can't allocate memory */
  NETMGT_SOCKET = 70,		/* socket(2) failed */
  NETMGT_BIND = 71,		/* bind(2) failed */
  NETMGT_GETSOCKNAME = 72,	/* getsockname(2) failed */
  NETMGT_GETHOSTNAME = 73,	/* gethostname(2) failed */
  NETMGT_KVM_READ = 74,		/* kvm_read(2) failed */
  NETMGT_GETTIMEOFDAY = 75,	/* gettimeofday(2) failed */

  /* internal errors */
  NETMGT_NOQUEUE = 76,		/* NULL queue pointer */
  NETMGT_NOQNODE = 77,		/* NULL queue node pointer */
  NETMGT_NONODEDATA = 78,	/* NULL queue node data pointer */
  NETMGT_ATQUEUELIMIT = 79,	/* would exceed queue length limit */
  NETMGT_EMPTYQUEUE = 80,	/* empty queue */

  /* illegal functions */
  NETMGT_NOMANAGER = 81,	/* only an manager can call this function */
  NETMGT_NOAGENT = 82,		/* only an agent can call this function */
  NETMGT_NORENDEZ = 83,		/* only an rendezvous can call this function */
  NETMGT_BADAUTHFLAVOR = 84,    /* invalid RPC authentication flavor */

  /* number of error list entries */
  NETMGT_NERRS = 85

  }	Netmgt_stat ;

/* error report buffer */
typedef struct
  {
    Netmgt_stat service_error ;	/* service error code */
    u_int agent_error ;		/* agent error code */
    char *message ;		/* error message */
  } 	Netmgt_error ;

#endif /* !_netmgt_errno_h */
