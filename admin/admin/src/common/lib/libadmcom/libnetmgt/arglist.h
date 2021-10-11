
/**************************************************************************
 *  File:	include/libnetmgt/arglist.h
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
 *  SCCSID:	@(#)arglist.h  1.25  90/12/05
 *
 *  Comments:	message argument list class
 *
 **************************************************************************
 */

#ifndef _arglist_h
#define _arglist_h

// argument tags 
typedef enum
{
  NETMGT_GENERIC_TAG = 0,	// generic argument
  NETMGT_OPTION_TAG = 1,	// request options 
  NETMGT_DATA_TAG = 2,		// data report 
  NETMGT_THRESH_TAG = 3,	// event threshold 
  NETMGT_EVENT_TAG = 4,		// event report 
  NETMGT_ERROR_TAG = 5,		// error report 
  NETMGT_SETARG_TAG = 6,	// set argument 
  NETMGT_END_TAG = 7,		// end of argument list 
  NETMGT_BAD_TAG = 8		// bad argument tag 
}	NetmgtArgTag;

// forward declarations
class NetmgtServiceMsg ;

// message argument list class 
class NetmgtArglist: public NetmgtBuffer
{

  // public access functions
public:
  // get request message argument list
  bool_t getArglist (caddr_t *pbase, u_int *plength) ;

  // get argument name
  bool_t getName (char *name) ;

  // get value length
  bool_t getLength (u_int *plength) ;

  // get event priority
  bool_t getPriority (u_int *ppriority) ;

  // get threshold  relational operator
  bool_t getRelop (u_int *prelop) ;

  // get argument tag
  bool_t getTag (NetmgtArgTag *ptag) ;

  // get argument type
  bool_t getType (u_int *ptype) ;

  // get value
  bool_t getval (u_int type, u_int length, caddr_t base) ;

  // are arguments tagged ?
  bool_t isTagged (void) { return tagged; }

  // peek at argument tag
  bool_t peekTag (NetmgtArgTag *ptag) ;


  // public update functions
public:

  // deallocate argument list memory
  bool_t dealloc (void) ;

  // put request message argument list
  bool_t putArglist (caddr_t base, u_int length) ;

  // put argument tag
  bool_t putTag (NetmgtArgTag tag) ;

  // put arglist sentinal tag
  bool_t putEndTag (bool_t isTagged) ;

  // put attribute name
  bool_t putName (char *name) ;
  
  // put value length 
  bool_t putLength (u_int length) ;

  // put threshold relational operator
  bool_t putRelop (u_int relop) ;

  // put event priority
  bool_t putPriority (u_int priority);

  // put attribute type
  bool_t putType (u_int type) ;

  // put value
  bool_t putValue (u_int type, u_int length, caddr_t value) ;


  // public manipulation functions
public:

  // deserialize argument list from XDR stream
  bool_t deserialize (XDR *xdr, NetmgtServiceMsg *msg) ;

  // deserialize value from XDR stream
  bool_t deserialValue (XDR *xdr, u_int type, u_int length, caddr_t value) ;

  // serialize argument list to XDR stream
  bool_t serialize (XDR *xdr, NetmgtServiceMsg *msg) ;

  // serialize value to XDR stream
  bool_t serialValue (XDR *xdr, u_int type, u_int length, caddr_t value) ;


  // public object containments
public:
  
  // these objects have public access because they control
  // access to class members by themselves
  NetmgtBuffer myValue1 ;	        // attribute value buffer
  NetmgtBuffer myValue2 ;	        // another attribute value buffer


  // private manipulation functions
private:

  // deserialize argument tag from XDR stream
  bool_t deserialTag (XDR *xdr, NetmgtArgTag *tag) ;

  // deserialize tagged arglist
  bool_t deserialTagged (XDR *xdr, NetmgtServiceMsg *msg) ;

  // deserialize untagged arglist
  bool_t deserialUntagged (XDR *xdr, NetmgtServiceMsg *msg) ;

  // serial argument tag
  bool_t serialTag (XDR *xdr, NetmgtArgTag *tag) ;

  // serialize tagged arglist
  bool_t serialTagged (XDR *xdr, NetmgtServiceMsg *msg) ;

  // serialize untagged arglist
  bool_t serialUntagged (XDR *xdr, NetmgtServiceMsg *msg) ;

  // serialize argument tag to XDR stream
  bool_t serialArgtag (XDR *xdr, NetmgtArgTag *tag) ;


  // private variables
private:

  bool_t tagged ;		// whether arguments are tagged

} ;

#endif _arglist_h 
