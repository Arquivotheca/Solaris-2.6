
/**************************************************************************
 *  File:	include/libnetmgt/generic.h
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
 *  SCCSID:	@(#)generic.h  1.26  91/02/07
 *
 *  Comments:	generic message argument class
 *
 **************************************************************************
 */

#ifndef	_generic_h
#define _generic_h

// generic argument clase
class NetmgtGeneric: public NetmgtArgument
{

  // public instantiation functions
public:

  // constructor
  NetmgtGeneric (void)
    {
      this->tag = NETMGT_GENERIC_TAG;
      this->name [0] = '\0';
      this->type = (u_int) 0;
      this->length = (u_int) 0;
      this->value = (caddr_t) NULL;
      this->relop = (u_int) 0;
      this->thresh_len = (u_int) 0;
      this->thresh_val = (caddr_t) NULL;
      this->priority = (u_int) 0;
    }

  // public access functions
public:

  // get argument from argument list
  bool_t getArg (NetmgtServiceMsg *aServiceMsg) ;

  // get data report argument from arglist
  bool_t getData (Netmgt_data *data, NetmgtServiceMsg *aServiceMsg) ;

  // get error report argument from arglist
  bool_t getError (NetmgtServiceMsg *aServiceMsg) ;

  // get event report argument from arglist
  bool_t getEvent (Netmgt_event *event, NetmgtServiceMsg *aServiceMsg) ;

  // get request option argument from arglist
  bool_t getOption (Netmgt_arg *option) ;

  // get threshold argument from arglist
  bool_t getThresh (Netmgt_thresh *thresh) ;

  // get argument tag 
  NetmgtArgTag getTag (void) { return this->tag; }


  // public update functions
public:

  // put argument on arglist
  bool_t putArg (NetmgtServiceMsg *aServiceMsg) ;

  // put data report argument on arglist
  bool_t putData (Netmgt_data *data, NetmgtServiceMsg *aServiceMsg) ;

  // put event report argument on arglist
  bool_t putEvent (Netmgt_event *event, NetmgtServiceMsg *aServiceMsg) ;

  // put request option argument on arglist
  bool_t putOption (Netmgt_arg *option, NetmgtServiceMsg *aServiceMsg) ;

  // put threshold argument on arglist
  bool_t putThresh (Netmgt_thresh *thresh, NetmgtServiceMsg *aServiceMsg) ;


  // public service functions
public:

  // deserialize argument from stream
  bool_t deserial (XDR *xdr, NetmgtServiceMsg *aServiceMsg, bool_t deserialTag) ;

  // serial argument to stream
  bool_t serial (XDR *xdr, NetmgtServiceMsg *aServiceMsg, bool_t serialTag) ;


  // private update functions
private:

  // set argument tag
  bool_t setTag (NetmgtServiceMsg *aServiceMsg, u_int relop, NetmgtArgTag *ptag) ;


  // private member variables
private:

  NetmgtArgTag tag;		// argument tag 
  char name [NETMGT_NAMESIZ] ;	// attribute name 
  u_int type ;			// attribute type 
  u_int length ;		// attribute value length 
  caddr_t value ;		// attribute value 
  u_int relop ;			// threshold relational operator 
  u_int thresh_len ;		// threshold value length 
  caddr_t thresh_val ;		// threshold value 
  u_int priority ;		// event priority 

} ;

#endif _generic_h 
