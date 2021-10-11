
/**************************************************************************
 *  File:	include/libnetmgt/message.h
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
 *  SCCSID:	@(#)message.h  1.31  90/12/05
 *
 *  Comments:	message class
 *
 **************************************************************************
 */

#ifndef	_message_h
#define _message_h

// message class 
class NetmgtMessage: public NetmgtObject
{

  // public service functions
public:

  // ask my client for a new or cached client handle
  bool_t getClient (struct in_addr addr, 
		    u_long proto,
		    u_long prog,
		    u_long vers,	
		    u_int security,
		    struct timeval timeout) ; 


  // protected access functions

  // get cached client handle
  CLIENT *getHandle (void) { return this->myClient.getHandle (); }
  

  // protected service functions
protected:

  // get a DES authenication handle
  bool_t getAuthdes (CLIENT *client, 
		     struct sockaddr_in *pname, 
		     u_int lifetime) ;

  // public objects
public:

  NetmgtClient myClient ;	 // client object

} ;

#endif	_message_h
