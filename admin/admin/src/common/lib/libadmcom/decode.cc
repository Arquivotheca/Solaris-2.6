#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)decode.cc	1.37 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/decode.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)decode.cc  1.37  91/05/05
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
 *  Comments:	decode message argument list
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* -----------------------------------------------------------------
 *  NetmgtArglist::deserialUntagged - deserialize untagged arglist
 *	returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------
 */
bool_t
NetmgtArglist::deserialUntagged (XDR *xdr, NetmgtServiceMsg *aServiceMsg)
				// XDR stream 
				// message pointer
{
  NetmgtGeneric generic;	// generic argument

  NETMGT_PRN (("decode: NetmgtArglist::deserialUntagged\n"));

  assert (xdr != (XDR *) NULL);

  // deserialize arguments and copy then to arglist 
  while (TRUE)
    {
      if (!generic.deserial (xdr, aServiceMsg, FALSE))
	return FALSE;

      if (!generic.putArg (aServiceMsg))
	return FALSE;

      if (generic.getTag () == NETMGT_END_TAG)
	return TRUE;
    }
  /*NOTREACHED*/
}

/* -----------------------------------------------------------------
 *  NetmgtArglist::deserialTagged - deserialize tagged arglist
 *	returns TRUE if successful; otherwise returns FALSE
 * -----------------------------------------------------------------
 */
bool_t
NetmgtArglist::deserialTagged (XDR *xdr, NetmgtServiceMsg *aServiceMsg)
				// XDR stream 
				// message pointer
{
  NetmgtArgTag tag ;		// argument tag
  NetmgtSetarg setarg ;		// set argument
  NetmgtGeneric generic ;	// generic argument

  NETMGT_PRN (("decode: NetmgtArglist::deserialTagged\n"));

  assert (xdr != (XDR *) NULL);

  // deserialize arguments and copy then to arglist 
  while (TRUE)
    {
      // deserialize argument tag
      if (!this->deserialTag (xdr, &tag))
	return FALSE;

      switch (tag)
	{
	  case NETMGT_OPTION_TAG: // option argument
	  
	  // deserialize option argument
	  if (!generic.deserial (xdr, aServiceMsg, FALSE))
	    return FALSE;

	  // copy option argument to arglist
	  if (!generic.putArg (aServiceMsg))
	    return FALSE;
	  break;

	case NETMGT_SETARG_TAG:	// set argument

	  // deserialize set argument
	  if (!setarg.deserial (xdr, aServiceMsg))
	    return FALSE;

	  // copy set argument to arglist
	  if (!setarg.putArg (aServiceMsg))
	    return FALSE;
	  break;

	case NETMGT_END_TAG:	// end of arguments
	  return TRUE;
	  
	default:
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNREQUEST, 0, NULL);
	  return FALSE;
	}
    }  
  /*NOTREACHED*/
}

/* --------------------------------------------------------------------
 *  NetmgtArglist::deserialValue - deserialize argument value
 *	returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtArglist::deserialValue (XDR *xdr, 
			      u_int type, 
			      u_int length, 
			      caddr_t value)
     // XDR stream
     // value type 
     // value length 
     // value buffer 
{
  caddr_t pvalue;		// buffer pointer 
  int int_val;			// int numeric value 
  u_int u_int_val;		// u_int numeric value 
  long long_val;		// long numeric value 
  u_long u_long_val;		// u_long numeric value 
  short short_val;		// short numeric value 
  u_short u_short_val;		// u_short numeric value 
  float float_val;		// float numeric value 
  double double_val;		// double numeric value 
  struct timeval timeval;	// timeval structure 
  u_long *netOrder;		// network byte order OBJECT IDENTIFIER 
  u_long *hostOrder;		// host byte order OBJECT IDENTIFIER 
  u_int elements;		// # OBJECT IDENTIFER elements 
  register i;			// loop counter 

  NETMGT_PRN (("decode: NetmgtArglist::deserialValue\n"));

  assert (value != (caddr_t) NULL);

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  switch (type)
    {
    case NETMGT_SHORT:
      if (!xdr_short (xdr, &short_val))
	{
	  NETMGT_PRN (("decode: can't decode short\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      (void) memcpy (value, (caddr_t) & short_val, sizeof (short));
      break;

    case NETMGT_U_SHORT:
      if (!xdr_u_short (xdr, &u_short_val))
	{
	  NETMGT_PRN (("decode: can't decode u_short\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      (void) memcpy (value, (caddr_t) & u_short_val, sizeof (u_short));
      break;

    case NETMGT_INT:
      if (!xdr_int (xdr, &int_val))
	{
	  NETMGT_PRN (("decode: can't decode int\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      (void) memcpy (value, (caddr_t) & int_val, sizeof (int));
      break;

    case NETMGT_U_INT:
    case NETMGT_UNIXTIME:
    case NETMGT_ENUM:
    case NETMGT_INTEGER:
    case NETMGT_COUNTER:
    case NETMGT_GAUGE:
    case NETMGT_TIMETICKS:
      if (!xdr_u_int (xdr, &u_int_val))
	{
	  NETMGT_PRN (("decode: can't decode u_int\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      (void) memcpy (value, (caddr_t) & u_int_val, sizeof (u_int));
      break;

    case NETMGT_LONG:
      if (!xdr_long (xdr, &long_val))
	{
	  NETMGT_PRN (("decode: can't decode long\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      (void) memcpy (value, (caddr_t) & long_val, sizeof (long));
      break;

    case NETMGT_U_LONG:
      if (!xdr_u_long (xdr, &u_long_val))
	{
	  NETMGT_PRN (("decode: can't decode u_long\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      (void) memcpy (value, (caddr_t) & u_long_val, sizeof (u_long));
      break;

    case NETMGT_FLOAT:
      if (!xdr_float (xdr, &float_val))
	{
	  NETMGT_PRN (("decode: can't decode float\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      (void) memcpy (value, (caddr_t) & float_val, sizeof (float));
      break;

    case NETMGT_DOUBLE:
      double_val = atof (value);
      if (!xdr_double (xdr, &double_val))
	{
	  NETMGT_PRN (("decode: can't decode double\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      (void) memcpy (value, (caddr_t) & double_val, sizeof (double));
      break;

    case NETMGT_STRING:
      pvalue = value;
      if (!xdr_string (xdr, &pvalue, NETMGT_LONG_MAXARGSIZ))
	{
	  NETMGT_PRN (("decode: can't decode string\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    case NETMGT_OBJECT_IDENTIFIER:
      // allocate a temporary buffer 
      netOrder = (u_long *) calloc (length, sizeof (u_long));
      if (!netOrder)
	{
	  if (netmgt_debug)
	    perror ("decode: calloc");
	  _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
	  xdr_destroy (xdr);
	  return FALSE;
	}

      // deserialize and XDR decode OBJECT IDENTIFER 
      pvalue = (caddr_t) netOrder;
      if (!xdr_bytes (xdr, &pvalue, &length, NETMGT_MAXARGSIZ))
	{
	  NETMGT_PRN (("decode: can't decode bytes\n"));
	  xdr_destroy (xdr);
	  (void) cfree ((caddr_t) netOrder);
	  netOrder = (u_long *) NULL;
	  return FALSE;
	}

      // convert OBJECT IDENTIFER to host byte order 
      hostOrder = (u_long *) value;
      elements = length / sizeof (u_long);
      for (i = 0; i < elements; i++)
	hostOrder[i] = ntohl (netOrder[i]);

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
	  NETMGT_PRN (("decode: can't decode bytes\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      break;

    case NETMGT_TIMEVAL:
      if (!xdr_long (xdr, &timeval.tv_sec))
	{
	  NETMGT_PRN (("decode: can't decode seconds\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      if (!xdr_long (xdr, &timeval.tv_usec))
	{
	  NETMGT_PRN (("decode: can't decode microseconds\n"));
	  xdr_destroy (xdr);
	  return FALSE;
	}
      (void) memcpy (value, (caddr_t) & timeval, sizeof (struct timeval));
      break;

    default:
      NETMGT_PRN (("decode: unknown argument type: %d\n", type));
      _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
      return FALSE;
    }
  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtArglist::deserialTag - XDR deserialize set argument tag
 *      returns TRUE is successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t 
NetmgtArglist::deserialTag (XDR *xdr, NetmgtArgTag *tag)
{
  assert (xdr != (XDR *) NULL);
  assert (tag != (NetmgtArgTag *) NULL);

  NETMGT_PRN (("decode: NetmgtArglist::deserialTag\n"));

  if (!xdr_u_int (xdr, (u_int *) tag))
    {
      NETMGT_PRN (("decode: can't decode argument tag\n"));
      xdr_destroy (xdr);
      return FALSE;
    }
  return TRUE;
}
