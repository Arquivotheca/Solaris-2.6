
/**************************************************************************
 *  File:	include/libnetmgt/define.h
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
 *  SCCSID:	@(#)security.h  1.27  90/12/05
 *
 *  Comments:	security definitions
 *
 **************************************************************************
 */

#ifndef	_security_h
#define _security_h

// security levels
typedef enum
{
  NETMGT_NO_SECURITY = 0,
  NETMGT_SECURITY_ONE = 1,
  NETMGT_SECURITY_TWO = 2,
  NETMGT_SECURITY_THREE	= 3,
  NETMGT_SECURITY_FOUR = 4,
  NETMGT_SECURITY_FIVE = 5
} NetmgtSecurityLevel ;

// security netgroups 
#define NETMGT_SECURITY_GROUP_ONE	"netmgt_security_one"
#define NETMGT_SECURITY_GROUP_TWO	"netmgt_security_two"
#define NETMGT_SECURITY_GROUP_THREE	"netmgt_security_three"
#define NETMGT_SECURITY_GROUP_FOUR	"netmgt_security_four"
#define NETMGT_SECURITY_GROUP_FIVE	"netmgt_security_five"

// minimum and maximum agent security levels 
#define NETMGT_MIN_SECURITY		NETMGT_NO_SECURITY
#define NETMGT_MAX_SECURITY		NETMGT_SECURITY_FIVE

#endif _security_h
