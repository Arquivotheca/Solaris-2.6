
/**************************************************************************
 *  File:	include/netmgt_types.h
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
 *  SCCSID:	@(#)netmgt_types.h  1.21  90/02/02
 *
 *  Comments:	SunNet Manager type definitions
 *
 **************************************************************************
 */
#ifndef _netmgt_types_h
#define _netmgt_types_h

/* queue node type */
typedef struct netmgt_qnode
{
  struct netmgt_qnode *next ;	/* pointer to next queue node */
  caddr_t data ;		/* queue data pointer */
}      Netmgt_qnode ;

/* queue node manipulation macros */
#define NETMGT_QNEXT(n)         (n)->next
#define NETMGT_QDATA(n)         (n)->data

/* queue header type */
typedef struct
{
  Netmgt_qnode *head ;		/* queue head */
  Netmgt_qnode *tail ;		/* queue tail */
  u_int limit ;			/* queue length limit */
  u_int length ;		/* queue length */
}      Netmgt_queue ;

/* queue minipulation macros */
#define NETMGT_QEMPTY(q)        (q)->head == (Netmgt_qnode *)NULL
#define NETMGT_QHEAD(q)         (q)->head
#define NETMGT_QTAIL(q)         (q)->tail
#define NETMGT_QLIMIT(q)        (q)->limit
#define NETMGT_QLENGTH(q)       (q)->length

/* buffer type */
typedef struct
{
  caddr_t base ;		/* buffer base address */
  off_t length ;		/* buffer length */
  off_t offset ;		/* buffer read/write offset */
}      Netmgt_buf ;

/* buffer manipulation macros */
#define NETMGT_BUFBASE          (b)->base
#define NETMGT_BUFLEN(b)        (b)->length
#define NETMGT_BUFOFFSET(b)     (b)->offset
#define NETMGT_RESETBUF(b)	(b)->offset = 0

/* macros for specifying maximum integer values */
#define NETMGT_MAX_SHORT	((short)~0 - (1 << (NBBY * sizeof(short) - 1)))
#define NETMGT_MAX_USHORT	(short)~0
#define NETMGT_MAX_INT		(~(int)0 - (1 << (NBBY * sizeof(int) - 1)))
#define NETMGT_MAX_UINT		~(u_int)0
#define NETMGT_MAX_LONG		(~(long)0 - (1 << (NBBY * sizeof(long) - 1)))
#define NETMGT_MAX_ULONG	~(u_long)0

#endif  _netmgt_types_h
