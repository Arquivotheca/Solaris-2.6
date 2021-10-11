
/**************************************************************************
 *  File:	include/netmgt_arglist.h
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
 *  SCCSID:	@(#)netmgt_arglist.h  1.32  90/12/05
 *
 *  Comments:	SunNet Manager message argument list definitions
 *
 **************************************************************************
 */

#ifndef _netmgt_arglist_h
#define _netmgt_arglist_h

#define NETMGT_ENDOFARGS "netmgt_endofargs" /* end-of-data sentinal string */
#define NETMGT_ENDOFROW  "netmgt_endofrow"  /* end-of-row sentinal string */
#define NETMGT_TABLE_KEY "netmgt_table_key" /* table key attribute string */


/* argument list type definitions */
#define NETMGT_SHORT		1	/* short */
#define NETMGT_U_SHORT		2	/* unsigned short */
#define NETMGT_INT		3	/* int */
#define NETMGT_U_INT		4	/* unsigned int */
#define NETMGT_LONG		5	/* long */
#define NETMGT_U_LONG		6	/* unsigned long */
#define NETMGT_FLOAT		7	/* float */
#define NETMGT_DOUBLE	 	8	/* double */
#define NETMGT_STRING  	 	9	/* null-terminated ASCII string */
#define NETMGT_OCTET		10	/* opaque octet stream */
#define NETMGT_INADDR		11	/* Internet address */
#define NETMGT_TIMEVAL		12	/* struct timeval */
#define NETMGT_UNIXTIME		13	/* seconds since 1/1/70 */
#define NETMGT_ENUM		14	/* enumerated type */

/* rfc 1065 Primitive Types */
#define NETMGT_INTEGER		15	/* rfc 1065 INTEGER */
#define NETMGT_OCTET_STRING	16	/* rfc 1065 OCTET STRING */
#define NETMGT_OBJECT_IDENTIFIER 17	/* rfc 1065 OBJECT IDENTIFIER */

/* rfc 1065 Defined Types */
#define NETMGT_NET_ADDRESS	18	/* rfc 1065 NetworkAddress */
#define NETMGT_IP_ADDRESS    	19	/* rfc 1065 IpAddress */
#define NETMGT_COUNTER    	20	/* rfc 1065 Counter */
#define NETMGT_GAUGE    	21	/* rfc 1065 Gauge */
#define NETMGT_TIMETICKS    	22	/* rfc 1065 TimeTicks */
#define NETMGT_OPAQUE    	23	/* rfc 1065 Opaque */

/* argument list relational operator definitions */
#define NETMGT_NOP		0	/* no operation */
#define NETMGT_EQ		1	/* value == threshold */
#define NETMGT_NE		2	/* value != threshold */
#define NETMGT_LT		3	/* value <  threshold */
#define NETMGT_LE		4	/* value <= threshold */
#define NETMGT_GT		5	/* value >  threshold */
#define NETMGT_GE		6	/* value >= threshold */
#define NETMGT_CHANGED		7	/* value changed */
#define NETMGT_INCRBY		8	/* value increased by threshold */
#define NETMGT_DECRBY		9	/* value decreased by threshold */
#define NETMGT_INCRBYMORE	10	/* value increased by more than
					   threshold */
#define NETMGT_INCRBYLESS	11	/* value increased by less than
					   threshold */
#define NETMGT_DECRBYMORE	12	/* value decreased by more than
					   threshold */
#define NETMGT_DECRBYLESS	13	/* value decreased by less than
					   threshold */
#define NETMGT_TRAP		14      /* trap */

/* request argument */
typedef struct
{
  char name[NETMGT_NAMESIZ] ;	/* argument name */
  u_int type ;			/* argument type */
  u_int length ;		/* argument length */
  caddr_t value ;		/* argument value */
}      Netmgt_arg ;

/* data argument */
typedef struct
{
  char name[NETMGT_NAMESIZ] ;	/* attribute name */
  u_int type ;			/* attribute type */
  u_int length ;		/* attribute length */
  caddr_t value ;		/* attribute value */
}      Netmgt_data ;

/* threshold argument */
typedef struct
{
  char name[NETMGT_NAMESIZ] ;	/* attribute name */
  u_int type ;			/* attribute type */
  u_int relop ;			/* relational operator */
  u_int thresh_len ;		/* threshold value length */
  caddr_t thresh_val ;		/* threshold value */
  caddr_t prev_val ;		/* previous attribute value */
  u_int priority ;		/* event priority */
}      Netmgt_thresh ;

/* event argument */
typedef struct
{
  char name[NETMGT_NAMESIZ] ;	/* attribute name */
  u_int type ;			/* attribute type */
  u_int length ;		/* attribute length */
  caddr_t value ;		/* attribute value */
  u_int relop ;			/* relational operator */
  caddr_t thresh_val ;		/* threshold value */
  u_int priority ;		/* event priority */
}      Netmgt_event ;

/* set argument */
typedef struct
{
  char group[NETMGT_NAMESIZ] ;	/* attribute group/table */
  char key[NETMGT_NAMESIZ] ;	/* optional attribute key */
  char name[NETMGT_NAMESIZ] ;	/* attribute name */
  u_int type ;			/* attribute type */
  u_int length ;		/* attribute length */
  caddr_t value ;		/* attribute value */
}      Netmgt_setval ;

#endif _netmgt_arglist_h

