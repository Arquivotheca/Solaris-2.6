/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CORVETTE_DELIVERY_H
#define _CORVETTE_DELIVERY_H

#pragma	ident	"@(#)delivery.h	1.1	95/01/28 SMI"

/*
 * delivery.h:	contains the data structures and definitions for the 
 *		following control areas in shared memory address
 *		space:
 *		- signalling control area
 *		- local enqueue control area
 *		- surrogate enqueue control area
 *		- local dequeue control area
 *		- surrogate dequeue control area
 *		- pipe descriptor
 */

#include <sys/types.h>

/*
 * SCB surrogate signalling area
 */ 

#pragma pack(1)

/*
 *	move mode generic control element structure
 */
typedef struct _gen_ctl_ele {

	ushort	length;		/* length of the ctrl element in bytes 	*/
	ushort	type;		/* format identifier - 0x0000 		*/
	ushort	resv;		/* reserved				*/
	ushort	opcode	:7;	/* function code 			*/
	ushort	expedite:1;	/* expediate the element 		*/
	ushort	fixed 	:3;	/* fixed to 0's 			*/
	ushort	cc 	:2;	/* chaining control 			*/
	ushort	sup	:1;	/* suppress reply element 		*/
	ushort	eid	:2;	/* element id 				*/
	union {

		struct {

			unchar 	dest_ent_id; 	/* destination entity id*/
			unchar	dest_unit_id;	/* destination unit id 	*/

		} dest_fld;

		ushort	dest_flds;

	} dest_id;

	ushort	srcid;		/* source id 				*/
	ulong	corrid;		/* correlation id 			*/

}ELE_HDR;

 
/* 
 * The fields within the Signalling Control Area (SCA) re used to identify
 * the source as well as the reason for signalling from one bus unit to
 * another (system to adapter).
 */
 
typedef struct _sca { 
	unchar	enq;			/* enqueue signal    */ 
	unchar	mgmt;			/* management signal */ 
	unchar	rsvd;			/* reserved          */ 
	unchar	deq;			/* dequeue signal    */ 
} SCA; 
 
/* 
 * values used in signal control area 
 */ 
 
#define SIGNAL_FLAG_OFF         0 
#define SIGNAL_FLAG_ON          1 
 
 
typedef struct _ctrlarea { 
	ushort	ssf;             /* surrogate start of free       */ 
	ushort	ses;             /* surrogate enqueue status      */ 
	ushort	sse;             /* surrogate start of elements   */ 
	ushort	sds;             /* surrogate dequeue status      */ 
	SCA	signal;          /* signalling control area (SCA) */ 
} CTRLAREA; 


/* 
 * adp_cfg/sys_cfg field settings 
 */ 
 
#define SIG_IO            0x0080 /* signalling area in I/O space        */ 
#define SIG_TIMER         0x0040 /* on timer expiration                 */ 
#define SIG_DEQ           0x0020 /* on every dequeue                    */ 
#define SIG_ENQ           0x0010 /* on every enqueue                    */ 
#define SIG_NOT_EMPTY     0x0008 /* on empty to not empty               */ 
#define SIG_FULL          0x0004 /* on not full to full                 */ 
#define SIG_EMPTY         0x0002 /* on not empty to empty               */ 
#define SIG_NOT_FULL      0x0001 /* on full to not full                 */ 
 
#define UT_IS_SYSTEM      0x0000 /* unit is a system unit               */ 
#define UT_IS_ADAPTER     0x0800 /* unit is an adapter                  */ 
 
#define PP_ADAP_MEM       0x8000 /* Adapter shared memory		*/
#define PP_SYS_MEM        0x4000 /* System shared memory		*/

/*
 * Local Enqueue Status bit definitions 
 * The flags within the Dequeue Status word are used by the dequeue logic
 * to maintain local state information at the dequeue end of pipe
 */ 
 
typedef struct { 
 
   unsigned full   : 1; 	/* queue is full		*/
   unsigned empty  : 1; 	/* queue is empty		*/
   unsigned b2     : 1; 
   unsigned b3     : 1; 
   unsigned b4     : 1; 
   unsigned b5     : 1; 
   unsigned b6     : 1; 
   unsigned b7     : 1; 
   unsigned wrap   : 1; 	/* enqueue wrap			*/
   unsigned b9     : 1; 	
   unsigned b10    : 1; 
   unsigned queued : 1; 	/* element started		*/
   unsigned b12    : 1; 
   unsigned b13    : 1; 
   unsigned b14    : 1; 
   unsigned b15    : 1; 
 
} LES_FLAGS; 

typedef ushort LES_FIELD; 
 
/*
 * Surrogate Enqueue Status bit definitions
 * The flags within the surrogate enqueue status word are used by the
 * enqueue logic to convey the state of the pipe to the dequeue logic
 * at the other end of the pipe
 */ 
 
typedef struct { 
 
   unsigned full     : 1; 	/* pipe is full			*/
   unsigned b1       : 1; 
   unsigned b2       : 1; 
   unsigned b3       : 1; 
   unsigned b4       : 1; 
   unsigned b5       : 1; 
   unsigned b6       : 1; 
   unsigned b7       : 1; 
   unsigned wrap     : 1; 	/* wrap of pipe occured		*/
   unsigned b9       : 1; 
   unsigned b10      : 1; 
   unsigned b11      : 1; 
   unsigned b12      : 1; 
   unsigned b13      : 1; 
   unsigned b14      : 1; 
   unsigned b15      : 1; 
 
} SES_FLAGS; 

typedef ushort SES_FIELD; 
 
/*
 *    Surrogate Enqueue Status
 */ 
 
typedef union { 
  SES_FIELD     fld;             /* as a field                          */ 
  SES_FLAGS     flg;             /* as individual flag bits             */ 
} SES; 

typedef ushort SSF;              /* surrogate start of free             */ 
 
/*             
 * Local Dequeue Status bit definitions
 * The flags within the dequeue status word are used by the dequeue logic
 * to maintain local state information at the dequeue end of pipe
 */
 
typedef struct { 
   unsigned full     : 1; 	/* pipe is full			*/
   unsigned empty    : 1; 	/* pipe is empty		*/
   unsigned b2       : 1; 	
   unsigned b3       : 1; 
   unsigned b4       : 1; 
   unsigned b5       : 1; 
   unsigned b6       : 1; 
   unsigned b7       : 1; 
   unsigned wrap     : 1; 	/* wrap element toggle		*/
   unsigned b9       : 1; 
   unsigned preempt  : 1; 	/* stop dequeue operation	*/
   unsigned dequeued : 1; 	/* element removed		*/
   unsigned b12      : 1; 
   unsigned b13      : 1; 
   unsigned b14      : 1; 
   unsigned b15      : 1; 
 
} LDS_FLAGS; 
typedef ushort LDS_FIELD; 
 
/*
 * Surrogate Dequeue Status Bit Definitions
 * The flags within the surrogate dequeue status word are used by the
 * dequeue logic to convey the state of the pipe to the enqueue logic
 * at the other end of the pipe
 */
 
typedef struct { 
 
   unsigned b0       : 1; 
   unsigned empty    : 1; 	/* pipe is empty		*/
   unsigned b2       : 1; 
   unsigned b3       : 1; 
   unsigned b4       : 1; 
   unsigned b5       : 1; 
   unsigned b6       : 1; 
   unsigned b7       : 1; 
   unsigned wrap     : 1; 	/* wrap of pipe occured		*/
   unsigned b9       : 1; 
   unsigned b10      : 1; 
   unsigned b11      : 1; 
   unsigned b12      : 1; 
   unsigned b13      : 1; 
   unsigned b14      : 1; 
   unsigned b15      : 1; 
 
} SDS_FLAGS; 
typedef ushort SDS_FIELD; 
 
/*
 *    Surrogate Dequeue Status  
 */
 
typedef union { 
   SDS_FIELD     fld;            /* as a field                          */ 
   SDS_FLAGS     flg;            /* as individual flag bits             */ 
} SDS; 
typedef ushort SSE;              /* surrogate start of elements         */ 
 
/*             
 * Local Enqueue Control Area (LECA) 
 * The fields within the local enqueue control area identify the 
 * location of the pipe, indicates the status of the pipe, and
 * provides the information identifying the starting and ending offsets
 * of control elements within the pipe
 */
 
typedef struct {		/* enqueue control area                */ 
 
   caddr_t	base;           /* ..base pipe pointer                 */ 
   ushort	we;             /* ..wrap element                      */ 
   union {                      /* ..enqueue status                    */ 
     LES_FIELD  fld;            /* ....as a field                      */ 
     LES_FLAGS  flg;            /* ....as individual flags             */ 
   }            es;             /* ..enqueue status                    */ 
   ushort       sf;             /* ..start of free elements            */ 
   ushort       ef;             /* ..end of free elements              */ 
   ushort       end;            /* ..end of pipe                       */ 
   ushort       top;            /* ..top of pipe                       */ 
 
} LECA; 

/*
 * Local Dequeue Control Area (LDCA)
 * The fields within the local enqueue control area identify the 
 * location of the pipe, indicates the status of the pipe, and
 * provides the information identifying the starting and ending offsets
 * of control elements within the pipe
 */ 
 
typedef struct {                 /* dequeue control area                */ 
 
   caddr_t 	 base;           /* ..base pipe pointer                 */ 
   ushort        we;             /* ..wrap element                      */ 
   union {                       /* ..dequeue status                    */ 
     LDS_FIELD   fld;            /* ....as a field                      */ 
     LDS_FLAGS   flg;            /* ....as individual flags             */ 
   }             ds;             /* ..dequeue status                    */ 
   ushort        se;             /* ..start of elements                 */ 
   ushort        ee;             /* ..end of elements                   */ 
   ushort        end;            /* ..end of pipe                       */ 
   ushort        top;            /* ..top of pipe                       */ 
 
} LDCA; 
 

/*    
 * Pipe Structure 
 * The fields within the pipe descriptor are used at initialization time
 * to identify the location of control areas in shared memory address 
 * space and the options to be used when using the delivery service
 * between a pair of bus units ( system and adapter ).
 */
 
typedef struct _pipeds {	/* pipe data structure			*/ 
 
   ushort     	status;		/* state of pipes                       */ 
   ushort     	pipe_id;	/* id assigned to pipe                  */ 
   ushort     	unit_id;	/* unit identifer                       */ 
   ushort     	res;		/* reserved                             */ 
  
   SCA		*adp_sca;	/* local unit signalling area           */ 
   SCA 		*sys_sca;	/* remote unit signalling area          */ 
 
   ushort     	adp_cfg;	/* local unit signalling options        */ 
   ushort     	sys_cfg;	/* remote unit signalling options       */ 
 
   struct {                     /* inbound pipe control information     */ 
	SES *ses;         	/* ..surrogate enqueue status           */ 
	SSF *ssf;         	/* ..surrogate start of free            */ 
	SDS *sds;         	/* ..surrogate dequeue status           */ 
	SSE *sse;         	/* ..surrogate start of elements        */ 
	union { 
		LECA	enq;	/* ...local enqueue control area        */ 
		LDCA	deq;	/* ...local dequeue control area        */ 
	} ca; 
   } ib_pipe; 
 
   struct {                     /* outbound pipe control information    */ 
	SES *ses;         	/* ..surrogate enqueue status           */ 
	SSF *ssf;         	/* ..surrogate start of free            */ 
	SDS *sds;         	/* ..surrogate dequeue status           */ 
	SSE *sse;         	/* ..surrogate start of elements        */ 
	union { 
		LECA	enq;    /* ...local enqueue control area        */ 
		LDCA    deq;    /* ...local dequeue control area        */ 
     	} ca; 
   } ob_pipe; 
 
   struct {                      /* performance counters                */ 
     ulong       enqctr;         /* ..count of elements enqueued        */ 
     ulong       deqctr;         /* ..count of elements dequeued        */ 
     ulong       fullctr;        /* ..count of pipe full occurrences    */ 
   } stats; 
 
} PIPEDS; 
 
/* 
 *	Macros for accessing Pipe Control Structures 
 */

#define enq_ctrl         ob_pipe.ca.enq
#define deq_ctrl         ib_pipe.ca.deq

/* 
 * PIPEDS - Status field settings
 */
 
#define PIPESALLOCATED   0x8000 	/* resources allocated		*/ 


/* 
 * Return codes for all SCB Unit Manager Service Calls 
 */ 
 
#define DS_SUCCESS              0x0000 
#define DS_UNKNOWN_ELEMENT      0x8001 
#define DS_INIT_ERR             0x8002 
#define DS_WRN_PREEMPT          0x8003 
 
#define DS_MAX_USERS            0x8004 
#define DS_NOT_SUPPORTED        0x8005 
 
#define DS_MAX_UNITS            0x8006 
#define DS_USER_INVALID         0x8007 
#define DS_DUPLICATE_UID        0x8008 
#define DS_IRQ_ERROR            0x8009 
 
#define DS_MAX_ENTITIES         0x800A 
#define DS_UNIT_INVALID         0x800B 
#define DS_DUPLICATE_EID        0x800C 
 
#define DS_MAX_PIPES            0x800D 
#define DS_PIPE_INVALID         0x800E 
 
#define DS_PIPE_FULL            0x800F 
#define DS_PIPE_EMPTY           0x8010 
#define DS_PIPE_NOT_ALLOCATED   0x8011 
#define DS_DEQ_PREEMPTED        0x8012 
#define DS_DEQ_TRUNCATED        0x8013 
#define DS_UNIT_INACTIVE        0x8014 
#define DS_ENTITY_INACTIVE      0x8015 
#define DS_IO_ACCESS_ERR        0x8020 
#define DS_IO_TIMEOUT           0x8021 
#define DS_IO_EXCEPTION         0x8022 

/* delivery service routines - delivery level */
int	enq_element(PIPEDS *pipep, ELE_HDR *ep, ushort ioaddr);
void	deq_element(PIPEDS *pipep, ushort ioaddr,
			void (*func)(void *arg, ELE_HDR *ep), void *arg);

#endif /* _CORVETTE_DELIVERY_H */
