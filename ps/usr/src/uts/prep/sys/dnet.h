/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * This file is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this file without charge, but are not authorized to
 * license or distribute it to anyone else except as part of a product
 * or program developed by the user.
 *
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * This file is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even
 * if Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * Hardware specific driver declarations for the D-LINK Ethernet device driver
 * driver conforming to the Generic LAN Driver model.
 */

#ifndef	_SYS_DNET_H
#define	_SYS_DNET_H

#pragma ident	"@(#)dnet.h	1.9	96/02/15 SMI"

#ifdef __cplusplus
extern "C" {
#endif

/* Performance */
#define	FULLDUPLEX 1			/* provide half or full duplex */

/* debug flags */
#define	DNETTRACE		0x01
#define	DNETERRS		0x02
#define	DNETRECV		0x04
#define	DNETDDI		0x08
#define	DNETSEND		0x10
#define	DNETINT		0x20

#ifdef DEBUG
#define	DNETDEBUG 1
#endif

/* Misc */
#define	DNETHIWAT		32768	/* driver flow control high water */
#define	DNETLOWAT		4096	/* driver flow control low water */
#define	DNETMAXPKT		1500	/* maximum media frame size */
#define	DNETIDNUM		0	/* DNET Id ; zero works */

/* board state */
#define	DNET_IDLE		0
#define	DNET_WAITRCV		1
#define	DNET_XMTBUSY		2
#define	DNET_ERROR		3

#define	SUCCESS		0
#define	FAILURE		1

#define	MAX_DNET_DEV		5

#define	DNET_PCI_IRQADDR	0x3C
#define	DNET_PCI_BASEADDR	0x10
#define	DNET_PCI_CSRADDR	0x04
#define	DNET_PCI_LATENCY	0x0C

#define	DNET_PCI_MASTER	0x04

#define	DEC_VENDOR_ID		0x1011
#define	DEVICE_ID_21040	0x0002
#define	DEVICE_ID_21140	0x0009
#define	COGENT_EM100		0x12
#define	VENDOR_ID_OFFSET	32
#define	VENDOR_REVISION_OFFSET	33

#define	GLD_TX_RESEND		1 	/* return code for GLD resend */
#define	GLD_TX_OK		0	/* return code for GLD Tx ok */

#define	MAX_TX_DESC		64	/* Should be a multiple of 4 */
#define	MAX_RX_DESC_21040	64	/* Should be a multiple of 4 */
#define	MAX_RX_DESC_21140	128	/* Should be a multiple of 4 */
#define	MAX_RX_DESC		128	/* Should be a multiple of 4 */
#define	TX_BUF_SIZE		256	/* Should be a multiple of 4 */
#define	RX_BUF_SIZE		256	/* Should be a multiple of 4 */
#define	SROM_SIZE		40
#define	SETUPBUF_SIZE		192	/* Setup buffer size */
#define	MCASTBUF_SIZE		512	/* mutlicast setup buffer */

#define	DNET_100MBPS		1	/* 21140 chip speeds */
#define	DNET_10MBPS		2

/* CSR  Description */
#define	BUS_MODE_REG		0x00
#define	TX_POLL_REG		0x08
#define	RX_POLL_REG		0x10
#define	RX_BASE_ADDR_REG	0x18
#define	TX_BASE_ADDR_REG	0x20
#define	STATUS_REG		0x28
#define	OPN_MODE_REG		0x30
#define	INT_MASK_REG		0x38
#define	MISSED_FRAME_REG	0x40
#define	ETHER_ROM_REG		0x48
#define	FULL_DUPLEX_REG	0x58
#define	SIA_STATUS_REG		0x60
#define	SIA_CONNECT_REG	0x68
#define	SIA_TXRX_REG		0x70
#define	SIA_GENERAL_REG	0x78

/* Bit descriptions of CSR registers */

/* BUS_MODE_REG, CSR0 */
#define	CACHE_ALIGN		0x04000	/* 8 long word boundary align */
#define	SW_RESET		0x01
#define	TX_AUTO_POLL		0x20000
#define	BURST_SIZE		0x2000

/* TX_POLL_REG, CSR1 */
#define	TX_POLL_DEMAND  	0x01

/* RX_POLL_REG, CSR2 */
#define	RX_POLL_DEMAND  	0x01

/* STATUS_REG , CSR5 */
#define	TX_INTR		0x01
#define	TX_STOPPED		0x02
#define	TX_BUFFER_UNAVAILABLE	0x04
#define	TX_JABBER_TIMEOUT	0x08
#define	TX_UNDERFLOW		0x20
#define	RX_INTR		0x40
#define	SYS_ERR		0x2000
#define	LINK_INTR		0x1000
#define	RX_STOP_INTR		0x0100
#define	RX_UNAVAIL_INTR	0x80
#define	ABNORMAL_INTR_SUMM	0x8000
#define	NORMAL_INTR_SUMM	0x10000
#define	CLEAR_INTR		0xFFFFFFFF

/* OPN_REG , CSR6  */
#define	START_TRANSMIT 	0x02000
#define	START_RECEIVE		0x02
#define	PROM_MODE		0x040
#define	HASH_FILTERING		0x01
#define	FULL_DUPLEX		0x200
#define	PASS_ALL_MULTI		0x80
#define	PORT_SELECT		0x40000
#define	TX_THRESHOLD_MODE	0x400000
#define	PCS_FUNCTION		0x800000
#define	SCRAMBLER_MODE		0x1000000
#define	HEARTBEAT_DISABLE	0x80000
#define	TX_THRESHOLD_160	0xC000

/* INT_MASK_REG , CSR7  */
#define	TX_INTERRUPT_MASK	0x01
#define	TX_STOPPED_MASK	0x02
#define	TX_JABBER_MASK		0x08
#define	TX_BUFFER_UNAVAIL_MASK	0x04
#define	RX_INTERRUPT_MASK	0x40
#define	NORMAL_INTR_MASK	0x10000
#define	ABNORMAL_INTR_MASK	0x08000
#define	SYSTEM_ERROR_MASK   	0x02000
#define	LINK_INTR_MASK   	0x01000
#define	RX_STOP_MASK		0x00100
#define	RX_UNAVAIL_MASK 	0x80
#define	TX_UNDERFLOW_MASK	0x20

/* MISSED_FRAME_REG, CSR8 */
#define	MISSED_FRAME_MASK	0x0ffff

/* Serial ROM Register CSR9 */
#define	READ_OP		0x4000
#define	SEL_ROM		0x800
#define	DATA_IN		0x04
#define	SEL_CLK		0x02
#define	SEL_CHIP		0x01
#define	SROM_MAX_CYCLES	5
#define	LAST_ADDRESS_BIT	5

/* SIA Connectivity reg, CSR13 */
#define	AUTO_CONFIG		0x04
#define	BNC_CONFIG		0x0C
#define	SIA_CONNECT_MASK	0xFFFF0000
#define	SIA_TXRX_MASK		0xFFFFFFFF
#define	SIA_GENERAL_MASK	0xFFFF0000
#define	HASH_CRC		0xFFFFFFFFU
#define	HASH_POLY		0x04C11DB6
#define	PRIORITY_LEVEL		5
#define	MASK_BUFSIZE		0x3FFFFF

#define	SIA_CONN_MASK_TP	0x8F01
#define	SIA_TXRX_MASK_TP	0xFFFFFFFF
#define	SIA_GENRL_MASK_TP	0x00

#define	SIA_CONN_MASK_BNCAUI	0xEF09
#define	SIA_TXRX_MASK_BNCAUI	0x0705
#define	SIA_GENRL_MASK_BNCAUI	0x0006

#define	FREE			0
#define	PROBED			1
#define	ATTACHED		2

#define	UP			2
#define	DOWN			3

#define	PARITY_ERROR		0x00
#define	MASTER_ABORT		0x00800000
#define	TARGET_ABORT		0x01000000

#define	CSR12_M_CONTROL		0x000001FF
#define	CSR12_M_MODE_DATA	0x000000FF

#define	CSR12_M_100TX_CD	0x00000040
#define	CSR12_M_100TX_LBE_TWISTER  	0x00000002
#define	CSR12_M_100TX_LBD_AMD  	0x00000001

#define	CSR12_M_10T_LINK_MISSING 	0x00000080
#define	CSR12_M_10T_100_OHM_SEL  	0x00000020
#define	CSR12_M_10T_LINK_ENABLE  	0x00000010
#define	CSR12_M_10T_OPN_MODE  		0x00000008
#define	CSR12_M_10T_SEEQ_POWER_ON 	0x00000004
#define	CSR12_M_10T_NOT_LONG_CABLE 	0x00000002
#define	CSR12_M_10T_SEEQ_XMTR_OFF 	0x00000001

#define	CSR12_K_PIN_SELECT		0x0000013F

#define	CSR12_K_RESET		CSR12_M_100TX_LBE_TWISTER  | \
					CSR12_M_10T_100_OHM_SEL | \
					CSR12_M_10T_SEEQ_XMTR_OFF

#define	CSR12_K_100TX		CSR12_M_100TX_LBD_AMD


#if _SOLARIS_PS_RELEASE < 250
typedef void *ddi_acc_handle_t;
#endif

/* Descriptor memory pointers */
struct desc_ptr {
	struct rx_desc_type	*rx_desc;
	struct tx_desc_type	*tx_desc;
};

/* driver specific declarations */
struct dnetinstance {
	ddi_acc_handle_t	io_handle;	/* ddi I/O handle */
	int		io_reg;			/* mapped register */
	int 		status;			/* Status of the board */
	int 		board_type;		/* board type: 21040 or 21140 */
	int		mode;			/* 21140 data rate: 10 or 100 */
	int		vendor_21140;
	int		vendor_revision;
	struct dnetdev 	*dnetdev;		/* private structure */
	caddr_t 	base_mem;		/* address of RAM allocated */
	int		base_memsize;		/* allocated memory size */
	struct desc_ptr	v_addr;			/* Virtual Address of desc's */
	struct desc_ptr	p_addr;			/* Physical Address of desc's */
	dev_info_t	devinfo;

	int		transmitted_desc; 	/* Descriptor count xmitted */
	int 		free_desc;		/* Descriptors available */
	int		tx_current_desc; 	/* Current Tx descriptor */
	u_char		*tx_virtual_addr[MAX_TX_DESC];	/* virtual address */

	int 		rx_current_desc; 	/* Current descriptor of Rx  */
	u_char		*rx_virtual_addr[MAX_RX_DESC];	/* virtual address */
	unsigned int	max_rx_desc;
	u_char 		setup_buffer[192]; 	/* Copy of setup buffer */
	char		multicast_cnt[512];
	int 		bnc_indicator;  	/* Flag for BNC connector */
	int		pgmask;
	int		pgshft;
	kmutex_t	init_lock;
#ifdef FULLDUPLEX
	int		full_duplex;
#endif
};

/*
 * Macro to convert Virtual to physical address
 */
#if defined(__ppc)
#define	DNET_KVTOP(vaddr) 	(((paddr_t)(hat_getkpfnum((caddr_t)(vaddr)) << \
				(dnetp->pgshft)) | ((paddr_t)(vaddr) & \
				(dnetp->pgmask))) | 0x80000000)
#else
#define	DNET_KVTOP(vaddr) 	((paddr_t)(hat_getkpfnum((caddr_t)(vaddr)) << \
				(dnetp->pgshft)) | ((paddr_t)(vaddr) & \
				(dnetp->pgmask)))
#endif
#pragma pack(1)

/*
 * Receive descriptor description
 */
struct rx_desc_type {
	struct {
		volatile u_long
				overflow	: 01,
				crc 		: 01,
				dribbling	: 01,
				resvd		: 01,
				rcv_watchdog 	: 01,
				frame_type	: 01,
				collision	: 01,
				frame2long   	: 01,
				last_desc	: 01,
				first_desc	: 01,
				multi_frame  	: 01,
				runt_frame	: 01,
				u_data_type	: 02,
				length_err	: 01,
				err_summary  	: 01,
				frame_len	: 15,
				own		: 01;
	} desc0;
	struct {
		volatile u_long
				buffer_size1 	: 11,
				buffer_size2 	: 11,
				not_used	: 02,
				chaining	: 01,
				end_of_ring	: 01,
				rsvd1		: 06;
	} desc1;
	u_char 		*buffer1;
	u_char 		*buffer2;
};

/*
 * Receive descriptor description
 */
struct tx_desc_type {
	struct {
		volatile u_long
				deferred	: 1,
				underflow	: 1,
				link_fail	: 1,
				collision_count : 4,
				heartbeat_fail	: 1,
				excess_collision : 1,
				late_collision	: 1,
				no_carrier	: 1,
				carrier_loss	: 1,
				rsvd1		: 2,
				tx_jabber_to	: 1,
				err_summary	: 1,
				rsvd		: 15,
				own		: 1;
	} desc0;
	struct {
		volatile u_long
				buffer_size1 	: 11,
				buffer_size2 	: 11,
				filter_type0 	: 1,
				disable_padding : 1,
				chaining 	: 1,
				end_of_ring  	: 1,
				crc_disable  	: 1,
				setup_packet 	: 1,
				filter_type1 	: 1,
				first_desc   	: 1,
				last_desc    	: 1,
				int_on_comp  	: 1;
	} desc1;
	u_char 		*buffer1;
	u_char 		*buffer2;
};


#define	DNET_END_OF_RING	0x2000000

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_DNET_H */
