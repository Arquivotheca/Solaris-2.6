#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)thresh.cc	1.43 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/thresh.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)thresh.cc  1.43  91/05/05
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
 *  Comments:	event threshold handling routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

// static functions 
static bool_t _netmgt_isExceeded (Netmgt_data *perf, Netmgt_thresh *thresh) ;

// static data 
static off_t _netmgt_startOfRow ; // start of row offset 
static bool_t _netmgt_rowEvent;	  // whether event occurred in this row 

/* -------------------------------------------------------------------
 *  NetmgtServiceMsg::clearEvent - clear event report argument 
 *	no return value
 * -------------------------------------------------------------------
 */
void
NetmgtServiceMsg::clearEvent (NetmgtPerformer *agent)
{
  NETMGT_PRN (("thresh: NetmgtServiceMsg::clearEvent\n"));
  
  // reset the arglist offset 
  this->myArglist.resetPtr ();

  // reset the start-of-row offset 
  _netmgt_startOfRow = this->myArglist.getOffset ();

  // clear the alarm flag 
  agent->setState (NETMGT_DISPATCHED);
  _netmgt_rowEvent = FALSE;

  // reset event report priority --- it's temporarily stored in
  // the request message header 
  this->setPriority ((u_int) 0);

  return;
}

/* ------------------------------------------------------------------
 *  netmgt_mark_end_of_row - append end-of-row marker to arglist
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
netmgt_mark_end_of_row (void)
{

  NETMGT_PRN (("thresh: netmgt_mark_end_of_row\n"));

  // get request information from agent
  NetmgtRequestInfo requestInfo ;   // request information buffer 
  aNetmgtPerformer->getRequestInfo (&requestInfo);

  // don't send this row if no events occurred
  if (requestInfo.type == NETMGT_EVENT_REQUEST && !_netmgt_rowEvent)
    {
      aNetmgtPerformer->myServiceMsg->myArglist.setOffset (_netmgt_startOfRow);
      if (aNetmgtPerformer->myServiceMsg->myArglist.isTagged ())
	return aNetmgtPerformer->myServiceMsg->myArglist.putEndTag (TRUE);
      else
	return aNetmgtPerformer->myServiceMsg->myArglist.putEndTag (FALSE);
    }

  // append and end-of-row marker to the arglist
  NetmgtGeneric aGeneric ;	// generic argument
  Netmgt_arg arg;		// optional argument buffer
  static char *endOfRow = NETMGT_ENDOFROW;

  (void) strcpy (arg.name, NETMGT_ENDOFROW);
  arg.type = NETMGT_STRING;
  arg.length = (u_int) strlen (NETMGT_ENDOFROW) + 1;
  arg.value = (caddr_t) endOfRow;
  if (!aGeneric.putOption (&arg, aNetmgtPerformer->myServiceMsg))
    return FALSE;

  // save start-of-row offset 
  _netmgt_startOfRow = aNetmgtPerformer->myServiceMsg->myArglist.getOffset ();
  
  // reset row event flag 
  _netmgt_rowEvent = FALSE;

  return TRUE;
}

/* --------------------------------------------------------------------
 *  _netmgt_isExceeded - determine whether threshold exceeded
 *	returns TRUE if exceeded; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
static bool_t
_netmgt_isExceeded (Netmgt_data *data, Netmgt_thresh *thresh)
				// performance data pointer
				// threshold queue data 
{
  struct timeval perf_time;	// performance time 
  struct timeval thresh_time;	// threshold time 
  struct timeval prev_time;	// previous time 
  bool_t alarm;			// whether threshold exceeded 
  short short_diff;		// short difference value 
  u_short u_short_diff;		// u_short difference value 
  int int_diff;			// int difference value 
  u_int u_int_diff;		// u_int difference value 
  long long_diff;		// long difference value 
  long long_diff1;		// long difference value 
  u_long u_long_diff;		// u_long difference value 
  float float_diff;		// float difference value 
  double double_diff;		// double difference value 
  static bool_t changed_tested;	// whether threshold tested yet 
  static bool_t incrby_tested;	// whether threshold tested yet 
  static bool_t decrby_tested;	// whether threshold tested yet 
  static bool_t incrbymore_tested; // whether threshold tested yet 
  static bool_t incrbyless_tested; // whether threshold tested yet 
  static bool_t decrbymore_tested; // whether threshold tested yet 
  static bool_t decrbyless_tested; // whether threshold tested yet 

  NETMGT_PRN (("thresh: _netmgt_isExceeded\n"));

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // get request information from agent
  NetmgtRequestInfo requestInfo ;   // request information buffer
  aNetmgtPerformer->getRequestInfo (&requestInfo);

  // check whether threshold exceeded 
  alarm = FALSE;

  switch (thresh->relop)
    {
    case NETMGT_EQ:		// is current value == threshold ? 
      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  if (*(short *) data->value == *(short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  if (*(u_short *) data->value == *(u_short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  if (*(int *) data->value == *(int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  if (*(u_int *) data->value == *(u_int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  if (*(long *) data->value == *(long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  if (*(u_long *) data->value == *(u_long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  if (*(float *) data->value == *(float *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  if (*(double *) data->value == *(double *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_STRING:
	  if (strcmp (data->value, thresh->thresh_val) == 0)
	    alarm = TRUE;
	  break;

	case NETMGT_OCTET:
	case NETMGT_INADDR:
	case NETMGT_OBJECT_IDENTIFIER:
	case NETMGT_IP_ADDRESS:
	case NETMGT_OCTET_STRING:
	case NETMGT_OPAQUE:
	  if (memcmp (data->value, thresh->thresh_val,
		      (int) data->length) == 0)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  thresh_time = *(struct timeval *) thresh->thresh_val;
	  if (perf_time.tv_sec == thresh_time.tv_sec &&
	      perf_time.tv_usec == thresh_time.tv_usec)
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}
      break;

    case NETMGT_NE:		// is current value != threshold ? 
      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  if (*(short *) data->value != *(short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  if (*(u_short *) data->value != *(u_short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  if (*(int *) data->value != *(int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  if (*(u_int *) data->value != *(u_int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  if (*(long *) data->value != *(long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  if (*(u_long *) data->value != *(u_long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  if (*(float *) data->value != *(float *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  if (*(double *) data->value != *(double *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_STRING:
	  if (strcmp (data->value, thresh->thresh_val) != 0)
	    alarm = TRUE;
	  break;

	case NETMGT_OCTET:
	case NETMGT_INADDR:
	case NETMGT_OBJECT_IDENTIFIER:
	case NETMGT_IP_ADDRESS:
	case NETMGT_OCTET_STRING:
	case NETMGT_OPAQUE:
	  if (memcmp (data->value, thresh->thresh_val,
		      (int) data->length) != 0)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  thresh_time = *(struct timeval *) thresh->thresh_val;
	  if (perf_time.tv_sec != thresh_time.tv_sec &&
	      perf_time.tv_usec != thresh_time.tv_usec)
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}
      break;

    case NETMGT_GT:		// is current value > threshold ? 
      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  if (*(short *) data->value > *(short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  if (*(u_short *) data->value > *(u_short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  if (*(int *) data->value > *(int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  if (*(u_int *) data->value > *(u_int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  if (*(long *) data->value > *(long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  if (*(u_long *) data->value > *(u_long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  if (*(float *) data->value > *(float *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  if (*(double *) data->value > *(double *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_STRING:
	  if (strcmp (data->value, thresh->thresh_val) > 0)
	    alarm = TRUE;
	  break;

	case NETMGT_OCTET:
	case NETMGT_INADDR:
	case NETMGT_OBJECT_IDENTIFIER:
	case NETMGT_IP_ADDRESS:
	case NETMGT_OCTET_STRING:
	case NETMGT_OPAQUE:
	  if (memcmp (data->value, thresh->thresh_val, (int) data->length) > 0)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  thresh_time = *(struct timeval *) thresh->thresh_val;
	  if (perf_time.tv_sec > thresh_time.tv_sec ||
	      (perf_time.tv_sec == thresh_time.tv_sec &&
	       perf_time.tv_usec > thresh_time.tv_usec))
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}
      break;

    case NETMGT_GE:		// is current value >= threshold ? 
      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  if (*(short *) data->value >= *(short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  if (*(u_short *) data->value >= *(u_short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  if (*(int *) data->value >= *(int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  if (*(u_int *) data->value >= *(u_int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  if (*(long *) data->value >= *(long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  if (*(u_long *) data->value >= *(u_long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  if (*(float *) data->value >= *(float *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  if (*(double *) data->value >= *(double *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_STRING:
	  if (strcmp (data->value, thresh->thresh_val) >= 0)
	    alarm = TRUE;
	  break;

	case NETMGT_OCTET:
	case NETMGT_INADDR:
	case NETMGT_OBJECT_IDENTIFIER:
	case NETMGT_IP_ADDRESS:
	case NETMGT_OCTET_STRING:
	case NETMGT_OPAQUE:
	  if (memcmp (data->value, thresh->thresh_val,
		      (int) data->length) >= 0)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  thresh_time = *(struct timeval *) thresh->thresh_val;
	  if (perf_time.tv_sec >= thresh_time.tv_sec ||
	      (perf_time.tv_sec == thresh_time.tv_sec &&
	       perf_time.tv_usec >= thresh_time.tv_usec))
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}
      break;

    case NETMGT_LT:		// is current value < threshold ? 
      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  if (*(short *) data->value < *(short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  if (*(u_short *) data->value < *(u_short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  if (*(int *) data->value < *(int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  if (*(u_int *) data->value < *(u_int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  if (*(long *) data->value < *(long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  if (*(u_long *) data->value < *(u_long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  if (*(float *) data->value < *(float *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  if (*(double *) data->value < *(double *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_STRING:
	  if (strcmp (data->value, thresh->thresh_val) < 0)
	    alarm = TRUE;
	  break;

	case NETMGT_OCTET:
	case NETMGT_INADDR:
	case NETMGT_OBJECT_IDENTIFIER:
	case NETMGT_IP_ADDRESS:
	case NETMGT_OCTET_STRING:
	case NETMGT_OPAQUE:
	  if (memcmp (data->value, thresh->thresh_val,
		      (int) data->length) < 0)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  thresh_time = *(struct timeval *) thresh->thresh_val;
	  if (perf_time.tv_sec < thresh_time.tv_sec ||
	      (perf_time.tv_sec == thresh_time.tv_sec &&
	       perf_time.tv_usec < thresh_time.tv_usec))
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}
      break;

    case NETMGT_LE:		// is current value <= threshold ? 
      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  if (*(short *) data->value <= *(short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  if (*(u_short *) data->value <= *(u_short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  if (*(int *) data->value <= *(int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  if (*(u_int *) data->value <= *(u_int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  if (*(long *) data->value <= *(long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  if (*(u_long *) data->value <= *(u_long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  if (*(float *) data->value <= *(float *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  if (*(double *) data->value <= *(double *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_STRING:
	  if (strcmp (data->value, thresh->thresh_val) <= 0)
	    alarm = TRUE;
	  break;

	case NETMGT_OCTET:
	case NETMGT_INADDR:
	case NETMGT_OBJECT_IDENTIFIER:
	case NETMGT_IP_ADDRESS:
	case NETMGT_OCTET_STRING:
	case NETMGT_OPAQUE:
	  if (memcmp (data->value, thresh->thresh_val,
		      (int) data->length) <= 0)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  thresh_time = *(struct timeval *) thresh->thresh_val;
	  if (perf_time.tv_sec <= thresh_time.tv_sec ||
	      (perf_time.tv_sec == thresh_time.tv_sec &&
	       perf_time.tv_usec <= thresh_time.tv_usec))
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}
      break;

    case NETMGT_CHANGED:	// has current value changed ? 
      if (!changed_tested)
	{
	  (void) memcpy (thresh->prev_val, data->value, (int) data->length);
	  changed_tested = TRUE;
	  break;
	}

      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  if (*(short *) data->value != *(short *) thresh->prev_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  if (*(u_short *) data->value != *(u_short *) thresh->prev_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  if (*(int *) data->value != *(int *) thresh->prev_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  if (*(u_int *) data->value != *(u_int *) thresh->prev_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  if (*(long *) data->value != *(long *) thresh->prev_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  if (*(u_long *) data->value != *(u_long *) thresh->prev_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  if (*(float *) data->value != *(float *) thresh->prev_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  if (*(double *) data->value != *(double *) thresh->prev_val)
	    alarm = TRUE;
	  break;

	case NETMGT_STRING:
	  if (strcmp (data->value, thresh->thresh_val) != 0)
	    alarm = TRUE;
	  break;

	case NETMGT_OCTET:
	case NETMGT_INADDR:
	case NETMGT_OBJECT_IDENTIFIER:
	case NETMGT_IP_ADDRESS:
	case NETMGT_OCTET_STRING:
	case NETMGT_OPAQUE:
	  if (memcmp (data->value, thresh->prev_val,
		      (int) data->length) != 0)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  prev_time = *(struct timeval *) thresh->prev_val;

	  if (perf_time.tv_sec != prev_time.tv_sec &&
	      perf_time.tv_usec != prev_time.tv_usec)
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}

      // save current performance value 
      (void) memcpy (thresh->prev_val, data->value, (int) data->length);

      break;

    case NETMGT_INCRBY:	// has current value increased by threshold ? 
      if (!incrby_tested)
	{
	  (void) memcpy (thresh->prev_val, data->value, (int) data->length);
	  incrby_tested = TRUE;
	  break;
	}

      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  short_diff = *(short *) data->value - *(short *) thresh->prev_val;
	  if (short_diff == *(short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  u_short_diff = *(u_short *) data->value -
	    *(u_short *) thresh->prev_val;
	  if (*(u_short *) data->value > *(u_short *) thresh->prev_val &&
	      u_short_diff == *(u_short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  int_diff = *(int *) data->value - *(int *) thresh->prev_val;
	  if (int_diff == *(int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  u_int_diff = *(u_int *) data->value - *(u_int *) thresh->prev_val;
	  if (*(u_int *) data->value > *(u_int *) thresh->prev_val &&
	      u_int_diff == *(u_int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  long_diff = *(long *) data->value - *(long *) thresh->prev_val;
	  if (long_diff == *(long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  u_long_diff = *(u_long *) data->value - *(u_long *) thresh->prev_val;
	  if (*(u_long *) data->value > *(u_long *) thresh->prev_val &&
	      u_long_diff == *(u_long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  float_diff = *(float *) data->value - *(float *) thresh->prev_val;
	  if (float_diff == *(float *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  double_diff = *(double *) data->value -
	    *(double *) thresh->prev_val;
	  if (double_diff == *(double *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  thresh_time = *(struct timeval *) thresh->thresh_val;
	  prev_time = *(struct timeval *) thresh->prev_val;

	  long_diff = perf_time.tv_sec - prev_time.tv_sec;
	  long_diff1 = perf_time.tv_usec - prev_time.tv_usec;

	  if (long_diff == thresh_time.tv_sec &&
	      long_diff1 == thresh_time.tv_usec)
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}

      // save current performance value 
      (void) memcpy (thresh->prev_val, data->value, (int) data->length);

      break;

    case NETMGT_DECRBY:	// has current value decreased by threshold ? 
      if (!decrby_tested)
	{
	  (void) memcpy (thresh->prev_val, data->value, (int) data->length);
	  decrby_tested = TRUE;
	  break;
	}

      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  short_diff = *(short *) thresh->prev_val - *(short *) data->value;
	  if (short_diff == *(short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  u_short_diff = *(u_short *) thresh->prev_val 
	    - *(u_short *) data->value;
	  if (*(u_short *) thresh->prev_val > *(u_short *) data->value &&
	      u_short_diff == *(u_short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  int_diff = *(int *) thresh->prev_val - *(int *) data->value;
	  if (int_diff == *(int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  u_int_diff = *(u_int *) thresh->prev_val - *(u_int *) data->value;
	  if (*(u_int *) thresh->prev_val > *(u_int *) data->value &&
	      u_int_diff == *(u_int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  long_diff = *(long *) thresh->prev_val - *(long *) data->value;
	  if (long_diff == *(long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  u_long_diff = *(u_long *) thresh->prev_val - *(u_long *) data->value;
	  if (*(u_long *) thresh->prev_val > *(u_long *) data->value &&
	      u_long_diff == *(u_long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  float_diff = *(float *) thresh->prev_val - *(float *) data->value;
	  if (float_diff == *(float *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  double_diff = *(double *) thresh->prev_val - *(double *) data->value;
	  if (double_diff == *(double *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  thresh_time = *(struct timeval *) thresh->thresh_val;
	  prev_time = *(struct timeval *) thresh->prev_val;

	  long_diff = prev_time.tv_sec - perf_time.tv_sec;
	  long_diff1 = prev_time.tv_usec - perf_time.tv_usec;

	  if (long_diff == thresh_time.tv_sec &&
	      long_diff1 == thresh_time.tv_usec)
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}

      // save current performance value 
      (void) memcpy (thresh->prev_val, data->value, (int) data->length);

      break;

    case NETMGT_INCRBYMORE:	/* has current value increased
				   by more than threshold ? */
      if (!incrbymore_tested)
	{
	  (void) memcpy (thresh->prev_val, data->value, (int) data->length);
	  incrbymore_tested = TRUE;
	  break;
	}

      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  short_diff = *(short *) data->value - *(short *) thresh->prev_val;
	  if (short_diff > *(short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  u_short_diff = *(u_short *) data->value -
	    *(u_short *) thresh->prev_val;
	  if (*(u_short *) data->value > *(u_short *) thresh->prev_val &&
	      u_short_diff > *(u_short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  int_diff = *(int *) data->value - *(int *) thresh->prev_val;
	  if (int_diff > *(int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  u_int_diff = *(u_int *) data->value - *(u_int *) thresh->prev_val;
	  if (*(u_int *) data->value > *(u_int *) thresh->prev_val &&
	      u_int_diff > *(u_int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  long_diff = *(long *) data->value - *(long *) thresh->prev_val;
	  if (long_diff > *(long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  u_long_diff = *(u_long *) data->value - *(u_long *) thresh->prev_val;
	  if (*(u_long *) data->value > *(u_long *) thresh->prev_val &&
	      u_long_diff > *(u_long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  float_diff = *(float *) data->value - *(float *) thresh->prev_val;
	  if (float_diff > *(float *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  double_diff = *(double *) data->value - *(double *) thresh->prev_val;
	  if (double_diff > *(double *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  thresh_time = *(struct timeval *) thresh->thresh_val;
	  prev_time = *(struct timeval *) thresh->prev_val;

	  long_diff = perf_time.tv_sec - prev_time.tv_sec;
	  long_diff1 = perf_time.tv_usec - prev_time.tv_usec;

	  if (long_diff > thresh_time.tv_sec ||
	      (long_diff == thresh_time.tv_sec &&
	       long_diff1 > thresh_time.tv_usec))
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}

      // save current performance value 
      (void) memcpy (thresh->prev_val, data->value, (int) data->length);

      break;

    case NETMGT_INCRBYLESS:	/* has current value increased by
				   less than threshold ? */
      if (!incrbyless_tested)
	{
	  (void) memcpy (thresh->prev_val, data->value, (int) data->length);
	  incrbyless_tested = TRUE;
	  break;
	}

      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  short_diff = *(short *) data->value - *(short *) thresh->prev_val;
	  if (short_diff < *(short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  u_short_diff = *(u_short *) data->value -
	    *(u_short *) thresh->prev_val;
	  if (*(u_short *) data->value > *(u_short *) thresh->prev_val &&
	      u_short_diff < *(u_short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  int_diff = *(int *) data->value - *(int *) thresh->prev_val;
	  if (int_diff < *(int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  u_int_diff = *(u_int *) data->value - *(u_int *) thresh->prev_val;
	  if (*(u_int *) data->value > *(u_int *) thresh->prev_val &&
	      u_int_diff < *(u_int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  long_diff = *(long *) data->value - *(long *) thresh->prev_val;
	  if (long_diff < *(long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  u_long_diff = *(u_long *) data->value - *(u_long *) thresh->prev_val;
	  if (*(u_long *) data->value > *(u_long *) thresh->prev_val &&
	      u_long_diff < *(u_long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  float_diff = *(float *) data->value - *(float *) thresh->prev_val;
	  if (float_diff < *(float *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  double_diff = *(double *) data->value - *(double *) thresh->prev_val;
	  if (double_diff < *(double *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  thresh_time = *(struct timeval *) thresh->thresh_val;
	  prev_time = *(struct timeval *) thresh->prev_val;

	  long_diff = perf_time.tv_sec - thresh_time.tv_sec;
	  long_diff1 = perf_time.tv_usec - thresh_time.tv_usec;

	  if (long_diff < thresh_time.tv_sec ||
	      (long_diff == thresh_time.tv_sec &&
	       long_diff1 < thresh_time.tv_usec))
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}

      // save current performance value 
      (void) memcpy (thresh->prev_val, data->value, (int) data->length);

      break;

    case NETMGT_DECRBYMORE:	/* has current value decreased
				   by more than threshold ? */
      if (!decrbymore_tested)
	{
	  (void) memcpy (thresh->prev_val, data->value, (int) data->length);
	  decrbymore_tested = TRUE;
	  break;
	}

      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  short_diff = *(short *) thresh->prev_val - *(short *) data->value;
	  if (short_diff > *(short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  u_short_diff = *(u_short *) thresh->prev_val -
	    *(u_short *) data->value;
	  if (*(u_short *) thresh->prev_val > *(u_short *) data->value &&
	      u_short_diff > *(u_short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  int_diff = *(int *) thresh->prev_val - *(int *) data->value;
	  if (int_diff > *(int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  u_int_diff = *(u_int *) thresh->prev_val - *(u_int *) data->value;
	  if (*(u_int *) thresh->prev_val > *(u_int *) data->value &&
	      u_int_diff > *(u_int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  long_diff = *(long *) thresh->prev_val - *(long *) data->value;
	  if (long_diff > *(long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  u_long_diff = *(u_long *) thresh->prev_val - *(u_long *) data->value;
	  if (*(u_long *) thresh->prev_val > *(u_long *) data->value &&
	      u_long_diff > *(u_long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  float_diff = *(float *) thresh->prev_val - *(float *) data->value;
	  if (float_diff > *(float *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  double_diff = *(double *) thresh->prev_val - *(double *) data->value;
	  if (double_diff > *(double *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  thresh_time = *(struct timeval *) thresh->thresh_val;
	  prev_time = *(struct timeval *) thresh->prev_val;

	  long_diff = prev_time.tv_sec - perf_time.tv_sec;
	  long_diff1 = prev_time.tv_usec - perf_time.tv_usec;

	  if (long_diff > thresh_time.tv_sec ||
	      (long_diff == thresh_time.tv_sec &&
	       long_diff1 > thresh_time.tv_usec))
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}

      // save current performance value 
      (void) memcpy (thresh->prev_val, data->value, (int) data->length);

      break;

    case NETMGT_DECRBYLESS:	/* has current value decreased by
				   less than threshold */
      if (!decrbyless_tested)
	{
	  (void) memcpy (thresh->prev_val, data->value, (int) data->length);
	  decrbyless_tested = TRUE;
	  break;
	}

      switch (thresh->type)
	{
	case NETMGT_SHORT:
	  short_diff = *(short *) thresh->prev_val - *(short *) data->value;
	  if (short_diff < *(short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_SHORT:
	  u_short_diff = *(u_short *) thresh->prev_val -
	    *(u_short *) data->value;
	  if (*(u_short *) thresh->prev_val > *(u_short *) data->value &&
	      u_short_diff < *(u_short *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_INT:
	case NETMGT_ENUM:
	  int_diff = *(int *) thresh->prev_val - *(int *) data->value;
	  if (int_diff < *(int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_INT:
	case NETMGT_INTEGER:
	case NETMGT_COUNTER:
	case NETMGT_GAUGE:
	case NETMGT_TIMETICKS:
	  u_int_diff = *(u_int *) thresh->prev_val - *(u_int *) data->value;
	  if (*(u_int *) thresh->prev_val > *(u_int *) data->value &&
	      u_int_diff < *(u_int *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_LONG:
	  long_diff = *(long *) thresh->prev_val - *(long *) data->value;
	  if (long_diff < *(long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_U_LONG:
	  u_long_diff = *(u_long *) thresh->prev_val - *(u_long *) data->value;
	  if (*(u_long *) thresh->prev_val > *(u_long *) data->value &&
	      u_long_diff < *(u_long *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_FLOAT:
	  float_diff = *(float *) thresh->prev_val - *(float *) data->value;
	  if (float_diff < *(float *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_DOUBLE:
	  double_diff = *(double *) thresh->prev_val - *(double *) data->value;
	  if (double_diff < *(double *) thresh->thresh_val)
	    alarm = TRUE;
	  break;

	case NETMGT_TIMEVAL:
	  perf_time = *(struct timeval *) data->value;
	  thresh_time = *(struct timeval *) thresh->thresh_val;
	  prev_time = *(struct timeval *) thresh->prev_val;

	  long_diff = prev_time.tv_sec - perf_time.tv_sec;
	  long_diff1 = prev_time.tv_usec - prev_time.tv_usec;

	  if (long_diff < thresh_time.tv_sec ||
	      (long_diff == thresh_time.tv_sec &&
	       long_diff1 < thresh_time.tv_usec))
	    alarm = TRUE;
	  break;

	default:
	  NETMGT_PRN (("thresh: unknown type: %d\n", data->type));
	  _netmgtStatus.setStatus (NETMGT_UNKNOWNTYPE, 0, NULL);
	  break;
	}

      // save current performance value 
      (void) memcpy (thresh->prev_val, data->value, (int) data->length);

      break;

    default:
      NETMGT_PRN (("thresh: unknown relop: %d\n", thresh->relop));
      _netmgtStatus.setStatus (NETMGT_BADRELOP, 0, NULL);
      break;
    }

  return alarm;
}

/* ---------------------------------------------------------------------
 *  NetmgtPerformer::buildEventReport - builds event report arglist
 *	return TRUE if successful; otherwise returns FALSE
 * ---------------------------------------------------------------------
 */
bool_t
NetmgtPerformer::buildEventReport (Netmgt_data *data, int *eventOccurred)
                       		// performance data pointer 
                            	// whether an event occurred 
{
 
  NETMGT_PRN (("thresh: NetmgtPerformer::buildEventReport\n"));
  assert (this->myServiceMsg != (NetmgtServiceMsg *) NULL);

  // reset error buffer 
  _netmgtStatus.clearStatus ();
  
  // clear event flag
  *eventOccurred = FALSE;

  // verify input 
  if (!data->name)
    {
      NETMGT_PRN (("thresh: no attribute name\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGNAME, 0, NULL);
      return FALSE;
    }
  if (!data->value)
    {
      NETMGT_PRN (("thresh: no attribute value\n"));
      _netmgtStatus.setStatus (NETMGT_NOARGVALUE, 0, NULL);
      return FALSE;
    }
  
  // return an error if the event report message argument list buffer
  // is greater than the maximum UDP-transport RPC buffer size
  u_long dataLength = this->myServiceMsg->myArglist.getOffset ()
			  + sizeof (NETMGT_END_TAG)
			  + sizeof (NETMGT_ENDOFARGS) + 1;

  if (dataLength > NETMGT_MAXARGSIZ)
    {
      NETMGT_PRN (("thresh: arglist too large: %d > %d\n",
		   dataLength, NETMGT_MAXARGSIZ));
      _netmgtStatus.setStatus (NETMGT_MSG2BIG, 
			       0, 
			       dgettext (NETMGT_TEXT_DOMAIN,
					 "Can't send event report"));
      return FALSE;
    }

  NetmgtQueueNode *currNode;	// current threshold queue node 
  static Netmgt_thresh *thresh;	// threshold argument
  NetmgtGeneric aGeneric ;	// generic argument
  Netmgt_event event ;		// event report argument
  
  // search the threshold argument queue looking for an
  // attribute name and type match
  currNode = this->myThreshQueue.getHead ();
  while (currNode)
    {
      
      // get threshold from queue node data 
      assert (currNode->isData ());
      thresh = (Netmgt_thresh *) currNode->getData ();
      
      // do we have a match ?
      if (strncmp (data->name, thresh->name, NETMGT_NAMESIZ) == 0 &&
	  data->type == thresh->type)
	{
	  // was the threshold exceeded ?
	  if (_netmgt_isExceeded (data, thresh))
	    {
	      // append an event report argument
	      (void) strncpy (event.name, data->name, NETMGT_NAMESIZ);
	      event.type = data->type;
	      event.length = data->length;
	      event.value = data->value;
	      event.relop = thresh->relop;
	      event.thresh_val = thresh->thresh_val;
	      event.priority = thresh->priority;
	      if (!aGeneric.putEvent (&event, this->myServiceMsg))
		return FALSE;
	      
	      // store event report priority in request object
	      if (thresh->priority > this->myRequest->getPriority ())
		this->myRequest->setPriority (thresh->priority);
	      
	      // remember an event occurred in this row	      
	      _netmgt_rowEvent = TRUE;

	      // change agent state to "event occurred"
	      this->setState (NETMGT_EVENT_OCCURRED);
	      *eventOccurred = TRUE;
	      return TRUE;
	    }
	}

      // get next threshold queue node
      currNode = currNode->getNext ();
    }
  // threshold wasn't exceeded -- append a data argument
  if (!aGeneric.putData (data, this->myServiceMsg))
	      return FALSE;
  return TRUE;
}
