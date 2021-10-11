/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CORVETTE_MOVE_H
#define _CORVETTE_MOVE_H

#pragma	ident	"@(#)move.h	1.1	95/01/28 SMI"

/*
 * move.h:	Contains the structure definintions for Move Mode
 *		operations
 */

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>

/* 
 *	General Purpose MACROS 
 */

#define	MAX_SG_SEGS	16

#define	DESTID		dest_id.dest_fld 	/* macro for entity id  */
#define	ADP_UID 	0			/* adapter unique id 	*/

#define	ON 		1
#define	OFF 		0
#define	TRUE		1
#define	FALSE		0
#define	SUCCESS		1
#define	FAILURE		0

#define	SCB_BASE_OFF            0       	/* base offset of pipe	*/ 

#pragma pack(1)

/*
 *	M O V E		M O D E 	D A T A 	S T R U C T U R E S
 */

/*
 *	move mode elemnts function codes list - opcode
 */
#define	READ_LIST		0x05
#define	READ_IMMEDIATE		0x06
#define	WRITE_LIST		0x08
#define	CANCEL			0x0c
#define	DIAGNOSE		0x0f
#define	MANAGEMENT		0x10
#define	WRAP_EVENT		0x3f
#define	ABORT_SCSI		0x40
#define	SEND_SCSI		0x41
#define	READ_DEV_CAPACITY	0x42
#define	READ_VERIFY		0x44
#define	SCSI_INFO_EVENT		0x47
#define	WRITE_VERIFY_LIST	0x48
#define	EST_BUF_POOL		0x49
#define	REL_BUF_POOL		0x4c
#define	DRY_POOL_EVENT		0x4d
#define	INIT_SCSI		0x52
#define	LOOP_SG_EVENT		0x60
#define	EXE_LOCATE_SCB		0x70
#define	REACT_QUEUE		0x7e

/*
 *	chaining control bits - cc
 */
#define	NO_CHAIN		0x0
#define	START_OF_CHAIN		0x1
#define	MIDDLE_OF_CHAIN		0x3
#define	END_OF_CHAIN		0x2

/*
 *	element id list - eid
 */
#define	REQUEST			0x0	/* request control element	*/
#define	REPLY			0x1	/* reply control element	*/
#define	EVENT			0x2	/* event control element	*/
#define	ERROR			0x3	/* error control element	*/

#define	WRAP_EL_LEN     	0x8	/* length of wrap element	*/ 

/*
 *	move mode request element structure
 */
typedef struct _gen_req_ele {

	ELE_HDR	gen_ctl;		/* generic control element 	*/
	unchar	flag1 ;
	unchar	specific1:4;		/* request specific 		*/
	unchar	flag2    :4;
	ushort	time_out_cnt;		/* time out count 		*/

}GEN_REQ_ELE;

/* 
 * values of flag1 for ABORT SCSI request
 */
#define	ABORT_RES	0x10

/* 
 * values of specific1 for ABORT SCSI request
 */
#define	ABORT_ALL	0x01		/* abort all cmds for all luns	*/
#define	ABORT_REQ	0x02		/* abort specific req related 	*/
					/* to correlation id 		*/
#define	ABORT_LUN	0x04		/* abort cmds for specific lun	*/
#define	ABORT_NER	0x08		/* abort cmds without error ele	*/

/*
 *	values for flag1  8 bits
 */
#define	DIR		0x01	/* direction of data transfer 		*/
#define	REACT_Q		0x02	/* reactivate SCSI command queue 	*/
#define	UNTAG_Q		0x04	/* untagged command 			*/
#define	ARQ		0x08	/* automatic request sense enable 	*/

#define	SG		0x10	/* loop scatter/gather enable 		*/
#define	SUP		0x20	/* suppress exception short 		*/
#define	NODISCON	0x40	/* no disconnect 			*/
#define	RESV		0x80	/* reserved - always 0 			*/

/*
 *	values for flag2
 */
#define	ADR		0x1	/* constant address enable 		*/
#define	PRT		0x2	/* io port data enable 			*/
#define	REQ		0x4	/* request specific 			*/
#define	TOU		0x8	/* unit of time secs or mins 		*/

/*
 *	abort scsi request element structure 
 */
typedef struct _abort_scsi_req {

	GEN_REQ_ELE	gen_req;	/* generic request 		*/
	ulong		abort_corr_id;	/* abort correlation id 	*/

}ABORT_SCSI_REQ;

/*
 *	cancel request element structure 
 */
typedef struct _cancel_req {

	ELE_HDR		gen_ctl;	/* generic request 		*/

}CANCEL_REQ;

/*
 *	diagnose request element structure 
 */
typedef struct _diag_req {

	ELE_HDR		gen_ctl;	/* generic request 		*/
	unchar		no_of_tests;	/* no of diag tests 		*/
	unchar		test;		/* diag test number 		*/
	unchar		tt;		
	ushort		resv_0;

}DIAG_REQ;

/*
 *	execute locate mode SCB request element structure
 */
typedef struct _exec_lmscb_req {

	GEN_REQ_ELE	gen_req;
	ulong		arq_addr;	/* auto request sense (ARS) addr*/
	unchar		arq_len;	/* ARS buffer length 		*/
	unchar		resv1_0;
	ushort		resv2_0;
	ulong		icmd_scb_addr;	/* immediate/SCB cmd address 	*/

}EXEC_LMSCB_REQ;

/*
 *	initialize scsi request element structure
 */
typedef struct _init_scsi_req {

	ELE_HDR		gen_ctl;	/* generic control element 	*/
	unchar		delay;		/* delay time in seconds 	*/
	unchar		flag;
	ushort		resv1_0;

}INIT_SCSI_REQ;

/*
 *	values for flag
 */
#define	HARD_RESET	0x01		/* reset both SCSI buses 	*/
#define	RESET_INTERNAL	0x04		/* reset internal SCSI bus 	*/
#define	RESET_EXTERNAL	0x08		/* reset external SCSI bus 	*/

/*
 *	loop scatter/gather request element structure
 */
typedef struct _loop_sg_req {

	ELE_HDR		gen_ctl;

}LOOP_SG_REQ;

/*
 *	reactivate scsi queue request element structure
 */
typedef struct _react_scsi_q_req {

	ELE_HDR		gen_ctl;

}REACT_SCSI_REQ;

/*
 *	read device capacity request element structure
 */
typedef struct _read_devcap_req {

	GEN_REQ_ELE	gen_req;
	ulong		arq_addr;	/* ARS address 			*/
	unchar		arq_len;	/* ARS buffer length 		*/
	unchar 		resv1_0;	
	ushort		resv2_0;	
	ulong		buf_addr;	/* Data buffer address 		*/
	ulong		buf_size;	/* Data buffer size - 0x08 	*/

}READ_DEVCAP_REQ;

/*
 *	read immediate request element structure
 */
typedef struct _read_immediate_req {

	ELE_HDR		gen_ctl;

}READ_IMMED_REQ;

/*
 *	read/write/verify list request element structure
 */
typedef struct _rdwr_list_req {

	GEN_REQ_ELE	gen_req;
	ulong		arq_addr;	/* ARS address 			*/
	unchar		arq_len;	/* ARS buffer length 		*/
	unchar 		resv1_0;	
	ushort		resv2_0;	
	ulong		lba_addr;	/* logical block address 	*/
	ushort		blk_cnt;	/* logical block count 		*/
	ushort		blk_len;	/* logical block length 	*/
	SG_SEGMENT 	buf_list[MAX_SG_SEGS]; /* buffer list */

}RDWR_LIST_REQ;

/*
 *	send scsi request element structure
 */
typedef struct _send_scsi_req {

	GEN_REQ_ELE	gen_req;
	ulong		arq_addr;	/* ARS address 			*/
	unchar		arq_len;	/* ARS buffer length 		*/
	unchar		resv1_0;
	ushort		resv2_0;
	unchar		cdb[12];	/* SCSI cmd descriptor block 	*/
	ulong		xfer_len;	/* total transfer length 	*/
	SG_SEGMENT	buf_list[MAX_SG_SEGS]; /* buffer list */

}SEND_SCSI_REQ;

/*
 *	write list request element structure
 */
typedef struct _write_list_req {

	GEN_REQ_ELE	gen_req;
	ulong		arq_addr;	/* ARS address 			*/
	unchar		arq_len;	/* ARS buffer length 		*/
	unchar 		resv1_0;	
	ushort		resv2_0;
	ulong		lba_addr;	/* logical block address 	*/
	ushort		blk_cnt;	/* logical block count 		*/
	ushort		blk_len;	/* logical block length 	*/
	SG_SEGMENT buf_list[MAX_SG_SEGS]; /* buffer list */

}WRITE_LIST_REQ;

/*
 *	generic reply element structure
 */
typedef struct _gen_reply_ele {
	ELE_HDR	gen_ctl;
}GEN_REPLY_ELE;

/*
 *	generic extended reply element structure
 */
typedef struct _ext_reply_ele {

	GEN_REPLY_ELE	gen_rep;
	CORV_TSB	tsb;	/* termination status block */
	ushort		resv;

}EXT_REPLY_ELE;

/*
 *	send scsi/read list reply element structure
 */
typedef struct _snd_rdlst_rep {

	GEN_REPLY_ELE	gen_rep;
	unchar		scsi_sts;	/* SCSI status 			*/
	unchar		valid;		/* is scsi_sts is valid ? 	*/
	ushort		reserved_0;	/* reserved - 0x0000 		*/
	long		residual_count;	/* residual byte count 		*/

}SND_RDLIST_REP;

/*
 *	generic error element structure
 */

typedef struct _gen_err_ele {
	EXT_REPLY_ELE	gen_err;
}GEN_ERR_ELE;

/*
 * 	generic event element structure	
 *	modified << only event_sts_ptr was there. replaced by 
 * 	separate sts1 and sts2>> 
 */
typedef struct _gen_event_ele {

	ELE_HDR		gen_ctl;
	ulong		event_sts1;	/* pointer to event status */
	ulong		event_sts2;	/* pointer to event status */

}GEN_EVENT_ELE;


/*
 *	loop scatter/gather event element structure
 */
typedef struct _loop_sg_event_ele {

	ELE_HDR		gen_ctl;

}LOOP_SG_EVENT_ELE;

/*
 *	read immediate event element structure
 */
typedef struct _read_immediate_ele {

	ELE_HDR		gen_ctl;
	ulong		ret_code;	/* return code 		*/

}READ_IMMEDIATE_ELE;

/*
 *	return codes for read immediate event
 */
#define	INT_SCSI_RESET_OCCURED	0x0001
#define	EXT_SCSI_RESET_OCCURED	0x0002
#define	TPCBO_ON_INT_SCSI	0x0201
#define	TPCBO_ON_EXT_SCSI	0x0202
#define	DIFF_SENSE_ERR_INT	0x0301
#define	DIFF_SENSE_ERR_EXT	0x0302

/*
 *	scsi info event element
 */

typedef struct _scsi_info_buf {

	ushort	resv1_0	:1;
	ushort	err_code:2;	/* error code 				*/
	ushort	resv2_0	:1;
	ushort	resv1	:4; 	/* be 0x2 				*/
	ushort	resv2	:4; 	/* be 0x1 				*/
	ushort	resv3	:3; 	/* be 0x0 				*/
	ushort	chain	:1;	/* chain bit 				*/
	ushort	desc_number;	/* descriptor number 			*/
	ulong	buf_size;	/* descriptor size 			*/
	ulong	buf_addr_msw;	/* buffer address MSW=0x00000000 	*/
	ulong	buf_addr_lsw;	/* buffer address LSW 			*/

}SCSI_INFO_BUF;
	
/* 
 * maximum of 10 buffer desc can be sent in each SCSI info event 	
 */
#define	MAX_BUF_DESC	10	

typedef struct _scsi_info_event_ele {

	ushort	len;		/* length 				*/
	ushort	resv_0;
	unchar	info_id;	/* SCSI info id 			*/
	unchar	sys_id;		/* system id 				*/
	unchar	ent_id;		/* entity id 				*/
	unchar	adp_id;		/* adapter id 				*/
	ushort	resv1_0	:1;
	ushort	err_code:2;	/* error code 				*/
	ushort	resv2_0	:1;
	ushort	resv1	:4; 	/* be 0x8 				*/
	ushort	resv2	:4; 	/* be 0x2 				*/
	ushort	resv3	:3; 	/* be 0x0 				*/
	ushort	chain	:1;
	ushort	resv4_0;
	unchar	bus_no;		/* bus number 				*/
	unchar	lun_no;		/* logical unit number 			*/
	unchar	pun_no;		/* Physical unit number 		*/
	unchar	resv5_0;
	ulong	resv6_0;
	ulong	Resv7_0;
	SCSI_INFO_BUF	buffers[MAX_BUF_DESC];	/* buffer descriptors 	*/

}SCSI_INFO_EVENT_ELE;

/*
 *	generic management request element structure
 */
typedef struct _gen_mgmt_req {

	ELE_HDR	gen_ctl;
	ushort	id;
	ushort	func_sts;

}GEN_MGMT_REQ;

/*
 *	management function codes
 */
#define	PIPE_CONFIG	0x8010	/* move mode delivery pipe config 	*/
#define	PIPE_DECONFIG	0x8011	/* move mode delivery pipe deconfig 	*/
#define	PIPE_SUSPND	0x8012	/* move mode delivery pipe suspend 	*/
#define	PIPE_RESUME	0x8013	/* move mode delivery pipe resume 	*/
#define	ASSIGN_EID	0xA000	/* move mode assign entity id 		*/
#define	REL_EID		0xA001	/* release specific entity id 		*/
#define	REL_ALL_EID	0xA002	/* release all specific entity id 	*/
#define	SUSPND_EID	0xA003	/* suspend selected entity id 		*/
#define	RESUME_EID	0xA004	/* resume selected entity id 		*/
#define	QUERY_EID	0xA010	/* query selected entity id 		*/

#define	PIPE_SIZE	2048	/* pipe size - minimum 8K 		*/

/* ??? which value should I use ??? */

#ifdef LATER	/* changed by badri for testing */
#define	PIPE_SIZE	8192	/* pipe size - minimum 8K 		*/
#endif

/*
 *	configure delivery pipes management request element structure
 */
typedef struct _mgmt_cfg_pipes {

	GEN_MGMT_REQ	gen_mgmt;
	unchar		adp_uid;	/* adapter unit id 		*/
	unchar		peer_uid;	/* peer unit id 		*/
	ushort		config_sts;	/* configuration status field 	*/
	ulong		peer_sigp;	/* peer unit signalling address */
	ulong		adp_sigp;	/* adp unit signalling address 	*/
	ushort		adp_ioaddr;	/* adapter base I/O address 	*/
	ushort		peer_ioaddr;	/* peer base I/O address 	*/
	ushort		timer_freq;	/* timer frequency 		*/
	unchar		time_unit;	/* unit of time 		*/
	unchar		sys_mid;	/* system management id 	*/
	ushort		adp_cfg_opt;	/* adapter configuartion option */
	ushort		peer_cfg_opt;	/* peer configuartion option 	*/
	ushort		ob_pipe_sz;	/* outbound pipe size 		*/
	ushort		ib_pipe_sz;	/* inbound pipe size 		*/
	ulong		ib_pipep;	/* inbound pipe address 	*/
	ulong		isdsp;		/* inbound dequeue status 	*/
	ulong		issep;		/* inbound start of elements 	*/
	ulong		isesp;		/* inbound enqueue status 	*/
	ulong		issfp;		/* inbound start of free 	*/
	ulong		ob_pipep;	/* outbound pipe address 	*/
	ulong		osdsp;		/* outbound dequeue status 	*/
	ulong		ossep;		/* outbound start of elements 	*/
	ulong		osesp;		/* outbound enqueue status 	*/
	ulong		ossfp;		/* outbound start of free 	*/

}MGMT_CFG_PIPES;

/*
 *	entity id management request structure
 */

typedef struct _eid_mgmt_req {

	GEN_MGMT_REQ	gen_mgmt;
	unchar		entity_id;
	unchar		info_id;
	ushort		lunn_1		:4;
	ushort		targ		:4;
	ushort		mode		:4;

#define	TARGET		0x1000
#define	INITIATOR	0x0000

	ushort		bus		:4;

#define	INTERNAL	0x0000
#define	EXTERNAL	0x0001

	unchar		tag_queue	:1;
	unchar		wide_xfer	:1;
	unchar		resv1_0		:6;
	unchar		resv2_0;		
	unchar		lunn_2		:5;
	unchar		resv3_0		:2;
	unchar		lun_no_flag	:1;
	unchar		resv4_0;

}EID_MGMT_REQ;

/*
 *	management reply/error element structure
 */
typedef struct _gen_mgmt_reply_err_ele {

	ELE_HDR		gen_ctl;
	ushort		id;
	ushort		sts;

}GEN_MGMT_REPLY_ERR_ELE;

/*
 *	management status codes	
 */
#define	REPL_UNQUA_SUCC		0x0000	/* reply unqualified success */
#define	REPL_QUA_SUCC		0x0100	/* reply qualify success */
#define	ERR_CMD_INVALID		0x0201	/* err command invalid code */
#define	ERR_CMD_NOT_SUP		0x0202	/* err command not supported */
#define	ERR_CMD_INV_MODIFIER	0x0204	/* err command invalid modifier */
#define	ERR_CMD_UNEXPECTED	0x0301	/* err command unexpected */
#define	ERR_CMD_RES_NOT_AVAIL	0x0303	/* err command resource not avail */

#define	CDPM_LENGTH	0xC4		/* length of cntrl elements in bytes */
#define	CORV_IDENTIFIER	0x00		/* corvette identifier field */
#define	EXPEDITE	0x01

#ifdef	__cplusplus
}
#endif

#endif /* _CORVETTE_MOVE_H */
