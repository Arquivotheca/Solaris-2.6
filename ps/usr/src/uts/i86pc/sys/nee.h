/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */
#pragma ident "@(#)nee.h	1.2	94/12/19 SMI"

/*
 * nee.h
 * Hardware specific driver declarations for the NE3200 586
 * driver conforming to the Generic LAN Driver model.
 */

#ifndef _NEE_H
#define _NEE_H 1
/*
 * Modification history
 *
 * ------------------------------------------------------------------------
 * Date		Author		Changes
 * ------------------------------------------------------------------------
 * 25 Feb 94	shiva (1.2)	Removed timer field from instance struct
 *				Added defines for
 * 				NEE_PORT_START ,NEE_NO_OVERRIDE, 
 *				NEE_OVERRIDE
 */

/* debug flags */
#define NEETRACE	0x01
#define NEEERRS		0x02
#define NEERECV		0x04
#define NEEDDI		0x08
#define NEESEND		0x10
#define NEEINT		0x20

#ifdef DEBUG
#define NEEDEBUG 1
#endif

/* Misc */
#define NEEHIWAT	32768		/* driver flow control high water */
#define NEELOWAT	4096		/* driver flow control low water */
#define NEEMAXPKT	1500		/* maximum media frame size */
#define NEEIDNUM	0		/* should be a unique id; zero works */
#define NEEMINSEND	60		/* minimum tx size */

/* board state */
#define NEE_IDLE	0
#define NEE_WAITRCV	1
#define NEE_XMTBUSY	2
#define NEE_ERROR	3

/* Ver 1.6 Transmitter state */
#define NEE_TX_FREE	0
#define NEE_TX_BUSY	1

/* EISA specific declarations */
/********** for Novel ne3200 ******/
#define MAX_EISABUF 	( EISA_MAXSLOT * 1024 ) 
#define BOARD_ID_1	( 0x3a )
#define BOARD_ID_2	( 0xcc )
#define BOARD_ID_3	( 0x07 )
#define BOARD_ID_4	( 0x01 )

/********** for ASUS L500 *********/
#define BOARD_ID_5      ( 0x06 )
#define BOARD_ID_6      ( 0x75 )
#define BOARD_ID_7      ( 0x05 )
#define BOARD_ID_8      ( 0x00 )

/* ne3200 eisa id register addresses */
#define NEE_IDREG1	( slot + 0xc80 )
#define NEE_IDREG2	( slot + 0xc81 )
#define NEE_IDREG3	( slot + 0xc82 )
#define NEE_IDREG4	( slot + 0xc83 )

/* ne3200 Reset port address */
#define	NEE_RESETPORT	( slot + 0x000 )

/* Enable/Disable EISA interrupts */
#define	NEE_EISAINTRENB		( slot + 0xC89 )
#define NEE_EISAINTRENB_BITS	( 0x01 )

/* ne3200 Host Doorbell register	*/
#define	NEE_DOORBELL		( slot + 0xC8F )
#define NEE_PKT_RX_INT 		( 0x01 )
#define NEE_PKT_TX_INT 		( 0x02 )
#define NEE_PKT_TX_INT_CLEAR 	( 0x02 )
#define NEE_PKT_RX_INT_CLEAR 	( 0x31 )

#define NEE_DOORBELL_BITS	( NEE_PKT_RX_INT|NEE_PKT_TX_INT|0x30 ) 

/* Enable/Disable Doorbell registers */
#define NEE_DOORBELLENB 	( slot + 0xC8E ) 

/* Post Office Commands  */
#define NEE_UPDT_PARAM_CMD	( 0x01 )
#define NEE_RESET_ADAP_CMD	( 0x02 )
#define NEE_UPDT_MCAST_CMD	( 0x04 )
#define NEE_ABEND_ADAP_CMD	( 0xF0 )
#define NEE_IDLE_ADAP_CMD	( 0x0F )
#define NEE_SET_PROM_CMD	( 0x10 )
#define NEE_CLEAR_PROM_CMD	( 0x20 )

/* Post Office flags */
#define NEE_IDLE_STATE		( 'I' )
#define NEE_ABEND_STATE		( 'A' )
#define NEE_FWARE_STATE		( 'J' )
#define NEE_FWAREOK_STATE	( 0xFEDCBA98L )	/* firmware's signature */
#define NEE_SINGLE_RCB		( 0xF0 ) 
#define NEE_MULTIPLE_RCB	( 0xFF ) 

/* NE3200 BMIC mail box register i/o addresses */
/* The slot val will used as (slot << 12)      */

#define VALID_RCB_MBOX 		(slot + 0xC90)	/* valid RCB is available    */
#define ABEND_IDLE_MBOX 	(slot + 0xC91)	/* 0xF0 = Abend, 0x0F = Idle */
#define UPDT_PARAM_MBOX		(slot + 0xC92)	/* Misc commands to f/w      */
#define VALID_TCB_MBOX 		(slot + 0xC93)	/* # frags to be sent	     */
#define BMTCB_XMIT_MBOX 	(slot + 0xC94)	/* xC94 - xC95 Byte count    */
#define RCB_PHYADR_MBOX 	(slot + 0xC98)  /* xC98 - B phy addr of rcb  */
#define PARAM_MBOX 		(slot + 0xC9C)  /* BMIC mail box	     */

/* host door bell register  */
#define HOST_DOOR_BELL		(slot + 0xC8E)

#define SWAP_WORD( x ) 		( ( (ushort)x >> 8 ) | ( (ushort)x << 8 ) )

/*	Convert virtual address to physical address	*/
#define NEE_KVTOP(vaddr)  \
	((paddr_t)(hat_getkpfnum((caddr_t)(vaddr)) << (neep->pgshft))| \
        ((paddr_t)(vaddr) & (neep->pgmask)))

# define NEE_FREEMEM_TOP  	( (ulong)neep - (ulong)neepstart ) 
# define NEE_FREEMEM_BOTTOM 	( ((ulong)neepstart + pgsiz*2) - \
					((ulong)neep + pgsiz) )

# define NEE_MAXMCAST_ADDR	16	/* f/w specific */

# define NEE_PORT_START		(0xC80)	/* Ver 1.2 */
# define NEE_NO_OVERRIDE	(0xFF)	/* Ver 1.2 */
# define NEE_OVERRIDE		(0x0F)	/* Ver 1.2 */

# define NEE_NORMAL_TCB		0	/* Ver 1.5 */
# define NEE_LOOKAHEAD_TCB	1	/* Ver 1.5 */

/* Ver 1.5 	Q defines and macros */
# define NEE_MAX_TX_BUF		2
# define NEE_Q_SIZE 		16	/* size should be 2 ^ n  */
# define NEE_Q_MASK 		15	/* should be Q_SIZE - 1 */

# define	NEE_ADD_IN_Q(rValue)\
{\
	if(neep->count == NEE_Q_SIZE)\
		rValue = 0;\
	else\
	if(!(neep->send_q[neep->write_ptr & NEE_Q_MASK] = dupmsg(mp)))\
		rValue = 0;\
	else{\
	neep->write_ptr++;\
	neep->count++;\
	rValue = 1;\
	}\
}

# define	NEE_REMOVE_FROM_Q(rValue)\
{\
	if(!neep->count)\
		rValue = 0;\
	else{\
		neep->count--;\
		rValue = neep->send_q[neep->read_ptr++ & NEE_Q_MASK];\
	}\
}
	
#pragma pack(1) 

/*
 * Multicast Address structure
 */
struct	mcast_info{
	unchar	mcast_eaddr[ ETHERADDRL ] ;
	ushort	flag ;	/* enable or disable ( f/w protocol ) */
	} ;

/*
 * Driver parameter block ( f/w protocol )
 */
struct	update_parm {
	paddr_t  node_adr_ptr ;		/* Phy Node Address */
	paddr_t  recv_frame_ptr ;	/* Reveive Frame Buffer */
	paddr_t  stat_area_ptr ;	/* Statistics Buffer */
	ushort	 num_cust_statcnt ;	/* f/w specific counter */
	ushort	 multicast_cnt ;	/* # Multicast addresses */
	paddr_t  multicast_listptr ;	/* Multicast List */
	};

# define UPARM_SIZ	( sizeof( struct update_parm ) )

/* 	
 * Transmit Control Block 
 */
struct 	TCB {
	ushort	frame_len ;	/* size of the frame */
	ushort	data_len ;	/* data in txbuf ( same as frame_len here ) */
	ushort	num_frags ;	/* always 1 */
	unchar  txbuf[ 1514 ] ;	/* buffer to hold the transmit data */
	};

# define TCB_SIZ	( sizeof( struct TCB ) )


/*
 * Receive Control Block 
 */
struct 	RCB {
	ushort	recv_frame_size ;	/* frame size received */
	unchar	rxbuf[ 1520 ] ;		/* buffer to hold receive data */
	};

# define RCB_SIZ	( sizeof( struct RCB ) )

/*
 * Statistics structure
 */
struct 	STAT {
	ulong	rx_overflow ;
	ulong	rx_toobig ;
	ulong	rx_toosmall ;
	ulong	tx_misc ;
	ulong	rx_misc ;
	ulong  	tx_retrycnt ;
	ulong	rx_crc ;
	ulong	rx_mismatch ;
	ushort	cust_counter_cnt ;
	ulong	num_586_resets ;
	ulong  	tx_retryfail ;
	ulong  	tx_carrier_lost ;
	ulong	tx_clr_to_send ;
	ulong  	tx_underrun ;
	ulong  	tx_defers ;
	ulong  	tx_max_collision ;
	ulong  	tx_num_collision ;
	ulong	rx_dmaover ;
	ulong	rx_noEOF ;
	} ;
	
# define STAT_SIZ	( sizeof( struct STAT ) )

/*
 * Driver private instance structure
 * Note : This should be properly aligned on the start of the physical
 *        page for the BMIC to work properly
 */
struct neeinstance {
	struct	update_parm 	updt_parm ;
	struct 	TCB		tcb_normal ;	
# ifndef NEE_KMEM_ZALLOC_FOR_BMIC
	struct 	TCB		tcb_lookahead ;		/* Ver 1.6 */
# endif
	struct 	RCB		rcb ;
	struct 	STAT		stats ;
	struct	mcast_info	mcasttbl[NEE_MAXMCAST_ADDR] ;	
	int  			pgmask ;
	int  			pgshft ;
	/* Ver 1.2 Removed timer field */
# ifdef NEE_KMEM_ZALLOC_FOR_BMIC
	unchar			*phypagestart ;
	unchar			mac_overlayed ;
# endif

	/* Ver 1.5 The following fields were added for better performance */
	unchar			tx_buf_flag[NEE_MAX_TX_BUF] ;
	unchar			tx_curr ;
	struct	TCB		*tcbptr[NEE_MAX_TX_BUF] ;

	/* Ver 1.5 The following fields were added for Q support */
	mblk_t 			*send_q[NEE_Q_SIZE];
	unchar			read_ptr;
	unchar 			write_ptr;
	unchar 			count;

	/* Ver 1.6 */
	unchar			tx_state ;
};

#pragma pack() 
# endif /* _NEE_H */
