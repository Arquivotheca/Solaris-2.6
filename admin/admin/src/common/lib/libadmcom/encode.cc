#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)encode.cc	1.38 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/encode.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)encode.cc  1.38  91/05/05
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
 *  Comments:	message argument list encoding routines
 *
 **************************************************************************
 */


/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* -------------------------------------------------------------------
 *  NetmgtArglist::serialUntagged - serialize generic arguments
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtArglist::serialUntagged (XDR *xdr, NetmgtServiceMsg *myServiceMsg)
				// XDR stream handle
				// sending message pointer
{
  NetmgtGeneric generic;	// generic argument buffer 

  NETMGT_PRN (("encode: NetmgtArglist::serialUntagged\n"));

  assert (xdr != (XDR *) NULL);

  // get and serialize generic arguments from arglist 
  while (TRUE)
    {
      // get next generic argument from arglist 
      if (!generic.getArg (myServiceMsg))
	return FALSE;

      if (!generic.serial (xdr, myServiceMsg, FALSE))
	return FALSE;

      if (generic.getTag () == NETMGT_END_TAG)
	return TRUE;
    }
  /*NOTREACHED*/
}

/* -------------------------------------------------------------------
 *  NetmgtArglist::serialTagged - XDR serialize tagged arglist
 *      returns TRUE is successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtArglist::serialTagged (XDR *xdr, NetmgtServiceMsg *myServiceMsg)
              			// XDR stream handle 
				// sending message pointer
{
  NetmgtArgTag tag;		// argument tag 
  NetmgtGeneric generic;	// generic argument 
  NetmgtSetarg setarg;		// set argument 
  char message[32];		// error message 

  NETMGT_PRN (("arglist: NetmgtArglist::serialTagged\n"));

  assert (xdr != (XDR *) NULL);

  // get message arguments from arglist and serialize
  while (TRUE)
    {

      // get argument tag 
      if (!this->peekTag(&tag))
	return FALSE;

      switch (tag)
	{
	case NETMGT_OPTION_TAG: // option argument 

	  // get option argument from arglist 
	  if (!generic.getArg (myServiceMsg))
	    return FALSE;

	  // serialize option argument to stream 
	  if (!generic.serial (xdr, myServiceMsg, TRUE))
	    return FALSE;

	  break;

	case NETMGT_SETARG_TAG:	// set value argument 
	
	  // get set value argument from arglist 
	  if (!setarg.getArg (myServiceMsg))
	    return FALSE;

	  // serialize set value argument to stream 
	  if (!setarg.serial (xdr, myServiceMsg))
	    return FALSE;

	  break;

	case NETMGT_END_TAG:	// end of argument list 
	  if (!this->serialTag (xdr, &tag))
	    return FALSE;
	  return TRUE;

	default:		// error 
	  (void) sprintf (message, "tag == %d\n", tag);
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, 0, message);
	  return FALSE;
	}
    }
  /*NOTREACHED*/
}

/* --------------------------------------------------------------------
 *  NetmgtArglist::serialValue - serialize value
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtArglist::serialValue (XDR *xdr, u_int type, u_int length, caddr_t value)
              			// transport handle 
                		// value type 
                  		// value length 
                   		// value buffer 
{
  int int_val;			// int value 
  u_int u_int_val;		// u_int value 
  long long_val;		// long value 
  u_long u_long_val;		// u_long value 
  short short_val;		// short value 
  u_short u_short_val;		// u_short value 
  float float_val;		// float value 
  double double_val;		// double value 
  struct timeval timeval;	// timeval structure 
  char *pvalue;			// value pointer 
  u_long *netOrder;		// network byte order OBJECT IDENTIFIER 
  u_long *hostOrder;		// host byte order OBJECT IDENTIFIER 
  u_int elements;		// # OBJECT IDENTIFER elements 
  register i;			// loop counter 

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  switch (type)
    {
    case NETMGT_SHORT:
      short_val = *(short *) value;
      if (!xdr_short (xdr, &short_val))
	{
	  NETMGT_PRN (("encode: can't encode short\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    case NETMGT_U_SHORT:
      u_short_val = *(u_short *) value;
      if (!xdr_u_short (xdr, &u_short_val))
	{
	  NETMGT_PRN (("encode: can't encode u_short\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    case NETMGT_INT:
      int_val = *(int *) value;
      if (!xdr_int (xdr, &int_val))
	{
	  NETMGT_PRN (("encode: can't encode int\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    case NETMGT_U_INT:
    case NETMGT_UNIXTIME:
    case NETMGT_ENUM:
    case NETMGT_INTEGER:
    case NETMGT_COUNTER:
    case NETMGT_GAUGE:
    case NETMGT_TIMETICKS:
      u_int_val = *(u_int *) value;
      if (!xdr_u_int (xdr, &u_int_val))
	{
	  NETMGT_PRN (("encode: can't encode u_int\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    case NETMGT_LONG:
      long_val = *(long *) value;
      if (!xdr_long (xdr, &long_val))
	{
	  NETMGT_PRN (("encode: can't encode long\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    case NETMGT_U_LONG:
      u_long_val = *(u_long *) value;
      if (!xdr_u_long (xdr, &u_long_val))
	{
	  NETMGT_PRN (("encode: can't encode u_long\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    case NETMGT_FLOAT:
      float_val = *(float *) value;
      if (!xdr_float (xdr, &float_val))
	{
	  NETMGT_PRN (("encode: can't encode float\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    case NETMGT_DOUBLE:
      double_val = *(double *) value;
      if (!xdr_double (xdr, &double_val))
	{
	  NETMGT_PRN (("encode: can't encode double\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    case NETMGT_STRING:
      pvalue = value;
      if (!xdr_string (xdr, &pvalue, NETMGT_LONG_MAXARGSIZ))
	{
	  NETMGT_PRN (("encode: can't encode string\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    case NETMGT_OBJECT_IDENTIFIER:
      // allocate a temporary buffer for holding OBJECT IDENTIFIER
      // in network byte order 
      netOrder = (u_long *) calloc (length, sizeof (u_long));
      if (!netOrder)
	{
	  if (netmgt_debug)
	    perror ("encode: calloc");
	  _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
	  xdr_destroy (xdr);
	  return FALSE;
	}

      // convert OBJECT IDENTIFER to network byte order 
      hostOrder = (u_long *) value;
      elements = length / sizeof (u_long);

      for (i = 0; i < elements; i++)
	netOrder[i] = htonl (hostOrder[i]);
      
      // XDR encode and serialize OBJECT IDENTIFIER 
      pvalue = (caddr_t) netOrder;
      if (!xdr_bytes (xdr, &pvalue, &length, NETMGT_MAXARGSIZ))
	{
	  NETMGT_PRN (("encode: can't encode bytes\n"));
	  xdr_destroy (xdr);
	  (void) cfree ((caddr_t) netOrder);
	  netOrder = (u_long *) NULL;
	  return FALSE;
	}
      (void) cfree ((caddr_t) netOrder);
      netOrder = (u_long *) NULL;
      break;
				 
    case NETMGT_OCTET:
    case NETMGT_INADDR:
    case NETMGT_OCTET_STRING:
    case NETMGT_NET_ADDRESS:
    case NETMGT_IP_ADDRESS:
    case NETMGT_OPAQUE:
      pvalue = value;
      if (!xdr_bytes (xdr, &pvalue, &length, NETMGT_LONG_MAXARGSIZ))
	{
	  NETMGT_PRN (("encode: can't encode bytes\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    case NETMGT_TIMEVAL:
      timeval = *(struct timeval *) value;
      if (!xdr_long (xdr, &timeval.tv_sec))
	{
	  NETMGT_PRN (("encode: can't encode seconds\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      if (!xdr_long (xdr, &timeval.tv_usec))
	{
	  NETMGT_PRN (("encode: can't encode microseconds\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    default:
      NETMGT_PRN (("encode: unknown type: %d\n", type));
      _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
      return FALSE;
    }

  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtArglist::serialTag - XDR serialize set argument tag
 *      returns TRUE is successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t 
NetmgtArglist::serialTag (XDR *xdr, NetmgtArgTag *tag)
{
  assert (xdr != (XDR *) NULL);
  assert (tag != (NetmgtArgTag *) NULL);

  if (!xdr_u_int (xdr, (u_int *) tag))
    {
      NETMGT_PRN (("encode: can't encode argument tag\n"));
      xdr_destroy (xdr);
      return FALSE;
    }
  return TRUE;
}

