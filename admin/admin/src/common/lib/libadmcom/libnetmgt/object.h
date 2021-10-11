
/**************************************************************************
 *  File:	include/libnetmgt/object.h
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
 *  SCCSID:	@(#)object.h  1.28  91/02/04
 *
 *  Comments:   root class
 *
 **************************************************************************
 */
#ifndef _object_h
#define _object_h

//  root class ***********************************************************

class NetmgtObject
{

  // public service functions
public:
  
  // block signals while in critical section
  sigset_t blockSignals (void) ;

  // get configuration value
  char *getConfig (char *name) ;

  // lock file
  bool_t lockFile (int fd) ;

  // unblock signals after leaving critical section
  void unblockSignals (sigset_t oldmask) ;

  // unlock file
  bool_t unlockFile (int fd) ;

} ;

#endif  _object_h
