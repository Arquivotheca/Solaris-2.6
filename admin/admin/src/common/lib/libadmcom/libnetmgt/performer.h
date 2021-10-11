
/**************************************************************************
 *  File:	include/libnetmgt/performer.h
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
 *  SCCSID:	@(#)performer.h  1.15  91/02/04
 *
 *  Comments:	performer agent class
 *
 **************************************************************************
 */

#ifndef _performer_h
#define _performer_h

// forward declaration
  class NetmgtDispatcher ;

// performer performer class
class NetmgtPerformer: public NetmgtAgent
{

  // public instantiation functions
public:

  // initialize performer instance
  bool_t myConstructor (NetmgtDispatcher *aDispatcher) ;

  // destroy performer instance
  void myDestructor (void) ;

  
  // public access functions
public:

  // whether cached report messages
  bool_t areCachedReports (void) { return this->myReportQueue.getLength (); }


  // public update functions
public:

  // append report message to deferred report queue
  appendReport (NetmgtServiceMsg *aReportMsg) ;

  // delete request from activity logfile
  bool_t deleteActivity (NetmgtControlMsg *aControlMsg) ;


  // public service functions
public:

  // build data report
  bool_t buildDataReport (Netmgt_data *data, bool_t *alarm) ;

  // build event report
  bool_t buildEventReport (Netmgt_data *data, bool_t *alarm) ;

 // build trap report
  bool_t buildTrapReport (Netmgt_data *data) ;

  // cache report message
  bool_t cacheReport (void) ;

  // resend cached reports
  bool_t resendReports (void) ;

  // send deferred reports
#ifdef _SVR4_
  void sendDeferred (int sig) ;
#else
  void sendDeferred (int sig, int code, struct sigcontext *scp, char *addr) ;
#endif _SVR4_


  // private update functions
private:

  // clean report queue
  bool_t cleanReports (void) ;


  // private objects
private:

  NetmgtQueue myReportQueue;	// cached reports queue 

} ;

#endif _performer_h
