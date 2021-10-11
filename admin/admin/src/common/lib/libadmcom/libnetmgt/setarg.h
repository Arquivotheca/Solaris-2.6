
/**************************************************************************
 *  File:	include/libnetmgt/setarg.h
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
 *  SCCSID:	@(#)setarg.h  1.25  90/12/05
 *
 *  Comments:	set request argument class
 *
 **************************************************************************
 */

#ifndef	_setarg_h
#define _setarg_h

// set request argument class
class NetmgtSetarg: public NetmgtArgument
{

  // public instantiation functions
public:

  // constructor
  NetmgtSetarg (void)
    {
      this->tag = NETMGT_SETARG_TAG;
      this->group [0] = '\0';
      this->key [0] = '\0';
      this->name [0] = '\0';
      this->type = (u_int) 0;
      this->length = (u_int) 0;
      this->value = (caddr_t) NULL;
    }

  // initialize set argument 
  bool_t initialize (Netmgt_setval *setval) ;


  // public service functions
public:

  // deserialize set argument
  bool_t deserial (XDR *xdr, NetmgtServiceMsg *aServiceMsg) ;
  
  // get argument from message arglist
  bool_t getArg (NetmgtServiceMsg *aServiceMsg) ;

  // append argument to message arglist
  bool_t putArg (NetmgtServiceMsg *aServiceMsg) ;

  // copy setval data and append argument to arglist
  bool_t putSetval (Netmgt_setval *setval, NetmgtServiceMsg *aServiceMsg) ;

  // serialize set argument
  bool_t serial (XDR *xdr, NetmgtServiceMsg *aServiceMsg) ;

  // serialize tag
  bool_t serialTag (XDR *xdr, NetmgtServiceMsg *aServiceMsg, NetmgtArgTag *tag) ;


  // private update functions
private:

  // set argument tag
  bool_t setTag (u_int relop, NetmgtArgTag *tag) ;


// private variables
private:

  NetmgtArgTag tag ;		// argument tag
  char group [NETMGT_NAMESIZ] ;	// group/table name 
  char key [NETMGT_NAMESIZ] ;	// optional key name 
  char name [NETMGT_NAMESIZ] ;	// attribute name 
  u_int type ;			// attribute type 
  u_int length ;		// attribute value length 
  caddr_t value ;		// attribute value 

} ;

#endif _setarg_h 
