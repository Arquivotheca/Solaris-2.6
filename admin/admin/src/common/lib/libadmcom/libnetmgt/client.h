
/**************************************************************************
 *  File:	include/libnetmgt/client.h
 *
 *  Author:	Bill Danielson, Sun Microsystems Inc.
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
 *  SCCSID:	@(#)client.h  1.15  91/01/14
 *
 *  Comments:	RPC client class
 *
 **************************************************************************
 */

#ifndef _client_h
#define _client_h

// client class 
class NetmgtClient: public NetmgtObject
{

  // public access functions
public:

  // get current client handle
  CLIENT *getHandle (void) { return this->client; }
  
  // get a new client 
  bool_t newClient (struct in_addr addr,
		    u_long proto,	
		    u_long prog,
		    u_long vers,	
		    int flavor,
		    struct timeval timeout) ;
  

  // public update functions
public:

  // destroy client
  void destroyClient (void) ;

  // public service functions
public:

  // get a RPC client
  bool_t getClient (struct in_addr addr, 
		    u_long proto,	
		    u_long prog,	
		    u_long vers,
		    int flavor,
		    struct timeval timeout) ;
  

  // private access functions
private:
  
  // create DES authentication credentials
  bool_t authdesCreate (struct sockaddr_in *pname, u_int lifetime);


  // private variables
private:
  
  struct in_addr addr ;		// remote address
  u_long proto ;		// RPC transport protocol 
  u_long prog ; 		// RPC program number
  u_long vers ;			// RPC version number
  int flavor;			// authentication flavor
  struct timeval timeout ;	// request timeout
  int sock;			// socket
  CLIENT *client;		// cached client handle
  
} ;

#endif _client_h
