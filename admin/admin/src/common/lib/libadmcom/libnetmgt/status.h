
/**************************************************************************
 *  File:	include/libnetmgt/status.h
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
 *  SCCSID:	@(#)status.h  1.23  90/12/05
 *
 *  Comments:	management services status class
 *
 **************************************************************************
 */

#ifndef _status_h
#define _status_h

// management status class
class NetmgtStatus: public NetmgtObject
{

  // public access functions
public:
  
  // clear status
  void clearStatus (void) ;

  // set status from error report
  bool_t getError (void) ;

  // get status
  bool_t getStatus (Netmgt_error *error) ;

  // whether requested operation succeeded
  bool_t wasSuccess (void)
    {
      return netmgt_error.service_error == NETMGT_SUCCESS;
    }


  // public update functions
public:

  // set status 
  bool_t setStatus (Netmgt_stat service_error, u_int agent_error, char *message) ;

} ;

#endif _status_h 
