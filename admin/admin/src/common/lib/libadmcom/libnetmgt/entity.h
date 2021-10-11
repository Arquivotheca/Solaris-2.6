
/**************************************************************************
 *  File:	include/libnetmgt/entity.h
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
 *  SCCSID:	@(#)entity.h  1.37  91/08/01
 *
 *  Comments:   management entity class
 *
 **************************************************************************
 */
#ifndef _entity_h
#define _entity_h

// entity class 
class NetmgtEntity: public NetmgtObject
{

  // public access functions
public:

  // fetch optional argument from options queue
  bool_t fetchOption (char *name, Netmgt_arg *option) ;

  // get request information
  void getRequestInfo (NetmgtRequestInfo *info) ;

  // get read security level
  NetmgtSecurityLevel getReadSecurity (void) { return this->readSecurity; }

  // get read/write security level
  NetmgtSecurityLevel getRdwrSecurity (void) { return this->rdwrSecurity; }


  // public update functions
public:

  // save request information 
  void saveRequestInfo (NetmgtRequestInfo *info) ;


  // protected update functions
protected:

  // append optional argument to options queue
  bool_t appendOption (Netmgt_arg *option) ;

  // set security levels
  bool_t setSecurityLevels (char *name) ;


  // protected service functions
protected:

#ifdef _SVR4_
  // get netconfig database entry
  struct netconfig *myGetnetconfigent (char *nettype) ;
#endif _SVR4_
  
  // verify security credentials
#ifdef _SVR4_
  bool_t verifyAuthorization (u_int type, uid_t uid);
#else
  bool_t verifyAuthorization (u_int type, int uid);
#endif _SVR4_

  // verify security credentials
  bool_t verifyCredentials (struct svc_req *rqst, 
			    register SVCXPRT *xprt,
#ifdef _SVR4_
			    uid_t *uid,
			    gid_t *gid,
			    int *gidlen,
			    gid_t gidlist [],
			    char netname []) ;
#else
			    int *uid,
			    int *gid,
			    int *gidlen,
			    int gidlist [],
  			    char netname []) ;
#endif _SVR4_


  // public objects --- they can protect themselves ...
public:
  
  NetmgtServiceMsg *myServiceMsg ; // service message pointer
  NetmgtQueue myOptionsQueue ;	   // optional argument queue 
  NetmgtRequest *myRequest ;	   // request information pointer


// protected member variables
protected:

  NetmgtSecurityLevel readSecurity ; // read security level
  NetmgtSecurityLevel rdwrSecurity ; // read/write security level

} ;

#endif  _entity_h
