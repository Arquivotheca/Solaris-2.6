/*
 * Copyright (c) 1995, 1996 by Sun Microsystems, Inc.
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
 * Hardware specific driver declarations for the Intel IPRB-Based Cards
 * driver conforming to the Generic LAN Driver model.
 */

#ifndef _IPRB_H
#define	_IPRB_H 1

#pragma ident	"@(#)iprb.h	1.6	96/09/19 SMI"

/* debug flags */
#define	IPRBTRACE	0x01
#define	IPRBERRS	0x02
#define	IPRBRECV	0x04
#define	IPRBDDI		0x08
#define	IPRBSEND	0x10
#define	IPRBINT		0x20

#ifdef DEBUG
#define	IPRBDEBUG 1
#endif

/* Misc */
#define	IPRBHIWAT	32768		/* driver flow control high water */
#define	IPRBLOWAT	4096		/* driver flow control low water */
#define	IPRBMAXPKT	1500		/* maximum media frame size */
#define	IPRB_FRAMESIZE	1516		/* Allocation size of frame */
#define	IPRBIDNUM	0		/* should be a unique id;zero works */

/* board state */
#define	IPRB_IDLE	0
#define	IPRB_WAITRCV	1
#define	IPRB_XMTBUSY	2
#define	IPRB_ERROR	3

#define	IPRB_MAX_RECVS		256	/* Maximum number of receive frames */
#define	IPRB_MAX_XMITS		256	/* Maximum number of cmd/xmit buffers */
#define	IPRB_DEFAULT_RECVS	32	/* Default receives if no .conf */
#define	IPRB_DEFAULT_XMITS	32	/* Default cmds/xmits if no .conf */


#define IPRB_NULL_PTR	0xffffffffU

/* A D100 individual address setup command */
struct iprb_ias_cmd {
	short ias_bits;
	short ias_cmd;
	unsigned long ias_next;
	unsigned char addr[ETHERADDRL];
};

/* A D100 configure command */
struct iprb_cfg_cmd {
	short cfg_bits;
	short cfg_cmd;
	unsigned long cfg_next;
	unsigned char cfg_byte0;
	unsigned char cfg_byte1;
	unsigned char cfg_byte2;
	unsigned char cfg_byte3;
	unsigned char cfg_byte4;
	unsigned char cfg_byte5;
	unsigned char cfg_byte6;
	unsigned char cfg_byte7;
	unsigned char cfg_byte8;
	unsigned char cfg_byte9;
	unsigned char cfg_byte10;
	unsigned char cfg_byte11;
	unsigned char cfg_byte12;
	unsigned char cfg_byte13;
	unsigned char cfg_byte14;
	unsigned char cfg_byte15;
	unsigned char cfg_byte16;
	unsigned char cfg_byte17;
	unsigned char cfg_byte18;
	unsigned char cfg_byte19;
	unsigned char cfg_byte20;
	unsigned char cfg_byte21;
	unsigned char cfg_byte22;
	unsigned char cfg_byte23;
};

struct iprb_mcs_addr {
	char addr[ETHERADDRL];
};

/* Configuration bytes (from Intel) */

#define	IPRB_CFG_B0		0x16
#define	IPRB_CFG_B1		0x88
#define	IPRB_CFG_B2		0
#define	IPRB_CFG_B3		0
#define	IPRB_CFG_B4		0
#define	IPRB_CFG_B5		0
#define	IPRB_CFG_B6		0x3a
#define	IPRB_CFG_B6PROM		0x80
#define	IPRB_CFG_B7		3
#define	IPRB_CFG_B7PROM		0x2
#define	IPRB_CFG_B7NOPROM	0x3
#define	IPRB_CFG_B8_MII		1
#define	IPRB_CFG_B8_503		0
#define	IPRB_CFG_B9		0
#define	IPRB_CFG_B10		0x2e
#define	IPRB_CFG_B11		0
#define	IPRB_CFG_B12		0x60
#define	IPRB_CFG_B13		0
#define	IPRB_CFG_B14		0xf2
#define	IPRB_CFG_B15		0xc8
#define	IPRB_CFG_B15_PROM	1
#define	IPRB_CFG_B15_NOPROM	0
#define	IPRB_CFG_B16		0
#define	IPRB_CFG_B17		0x40
#define	IPRB_CFG_B18		0xf2
#define	IPRB_CFG_B19		0x80
#define	IPRB_CFG_B20		0x3f
#define	IPRB_CFG_B21		5
#define	IPRB_CFG_B22		0
#define	IPRB_CFG_B23		0

#define	IPRB_MAXMCSN		64	/* Max number of multicast addrs */
#define	IPRB_MAXMCS (IPRB_MAXMCSN*ETHERADDRL)


/* A D100 multicast setup command */
struct iprb_mcs_cmd {
	short mcs_bits;
	short mcs_cmd;
	unsigned long mcs_next;
	short mcs_count;
	char mcs_bytes[IPRB_MAXMCS];
};

/* A D100 transmit command */
struct iprb_xmit_cmd {
	short xmit_bits;
	short xmit_cmd;
	unsigned long xmit_next;
	unsigned long xmit_tbd;
	short xmit_count;
	unsigned char xmit_threshold;
	unsigned char xmit_tbdnum;
};

struct iprb_gen_cmd {
	short gen_status;
	short gen_cmd;
	unsigned long gen_next;
};

/* A generic control unit (CU) command */
union iprb_generic_cmd {
	struct iprb_ias_cmd ias_cmd;
	struct iprb_cfg_cmd cfg_cmd;
	struct iprb_mcs_cmd mcs_cmd;
	struct iprb_xmit_cmd xmit_cmd;
	struct iprb_gen_cmd gen_cmd;
};

struct iprb_xmit_buffer_desc {
	unsigned long iprb_buffer_addr;		/* DMAable buffer address */
	unsigned long iprb_buffer_size;		/* LSB 14-bits size in bytes */
};

/* A D100 receive frame descriptor */
struct iprb_rfd {
	unsigned short rfd_status;
	unsigned short rfd_control;
	unsigned long rfd_next;		/* DMAable address of next frame */
	unsigned long rfd_rbd;		/* DMAable address of buffer desc */
	short rfd_count;		/* Actual number of bytes received */
	short rfd_size;			/* # of bytes available in buffer */
};

struct iprb_rbd {
	unsigned short rbd_count;	/* w/ End-Of-Frame & Filled bits */
	unsigned short mbz1;		/* Must Be Zero */
	unsigned long rbd_next;		/* DMAable address of next RBD */
	unsigned long rbd_buffer;	/* DMAable address of buffer */
	unsigned short rbd_size;	/* size of attached buffer */
	unsigned short mbz2;		/* Must Be Zero */
};

/* Statistics structure returned by D100 SCB */
struct iprb_stats {
	unsigned long iprb_stat_xmits;
	unsigned long iprb_stat_maxcol;
	unsigned long iprb_stat_latecol;
	unsigned long iprb_stat_xunderrun;
	unsigned long iprb_stat_crs;
	unsigned long iprb_stat_defer;
	unsigned long iprb_stat_onecoll;
	unsigned long iprb_stat_multicoll;
	unsigned long iprb_stat_totcoll;
	unsigned long iprb_stat_rcvs;
	unsigned long iprb_stat_crc;
	unsigned long iprb_stat_align;
	unsigned long iprb_stat_resource;
	unsigned long iprb_stat_roverrun;
	unsigned long iprb_stat_cdt;
	unsigned long iprb_stat_short;
	unsigned long iprb_stat_chkword;
};

struct iprb_buf_free {
	frtn_t free_rtn;		/* desballoc() structure */
	struct iprbinstance *iprbp;
	ddi_acc_handle_t dma_acchdl;
};

/* driver specific declarations */
struct iprbinstance {
	dev_info_t *iprb_dip;		/* Device Information Pointer */
	unsigned char iprb_phy_type;	/* Type of physical interface */
	long iprb_nframes;		/* Number of frames in ring (props) */
	long iprb_nxmits;		/* No. of simultaneous commands */
	mblk_t *iprb_xmit_mp[IPRB_MAX_XMITS];
	/* DMA resources needing allocating/freeing */
	/* Transmit portion */
	/* CU command blocks */
	ddi_dma_handle_t iprb_dma_handle_cu;
	union iprb_generic_cmd *iprb_cu_cmd_block[IPRB_MAX_XMITS];
	ddi_acc_handle_t iprb_cu_dma_acchdl[IPRB_MAX_XMITS];
	/* Transmit buffer descriptors */
	ddi_dma_handle_t iprb_dma_handle_txbda;
	int iprb_xmit_buf_desc_cnt[IPRB_MAX_XMITS];
	struct iprb_xmit_buffer_desc *iprb_xmit_buf_desc[IPRB_MAX_XMITS];
	unsigned long iprb_xmit_buf_desc_dma_addr[IPRB_MAX_XMITS];
	ddi_acc_handle_t iprb_xmit_desc_dma_acchdl[IPRB_MAX_XMITS];
	/* Receive portion */
	ddi_dma_handle_t iprb_dma_handle_ru;
	struct iprb_rfd *iprb_ru_frame_desc[IPRB_MAX_RECVS];
	unsigned long iprb_ru_frame_addr[IPRB_MAX_RECVS];
	ddi_acc_handle_t iprb_ru_dma_acchdl[IPRB_MAX_RECVS];
	ddi_dma_handle_t iprb_dma_handle_rcvbda;
	struct iprb_rbd *iprb_rcv_buf_desc[IPRB_MAX_RECVS];
	ddi_acc_handle_t iprb_rcv_desc_dma_acchdl[IPRB_MAX_RECVS];
	ddi_dma_handle_t iprb_dma_handle_rcv_buf;
	struct iprb_rcv_pool {
		char *iprb_rcv_buf;		/* virtual addr of buffer */
		unsigned long iprb_rcv_buf_addr;	/* DMA addr of buffer */
		ddi_acc_handle_t iprb_rcv_buf_dma_acchdl;
	} iprb_rcv_pool[IPRB_MAX_RECVS];
	/* end of DMA resources */
	kmutex_t iprb_rcv_buf_mutex;	/* protection for receive buf counter*/
	int iprb_rcv_bufs_outstanding;	/* receive buf counter */
	short iprb_first_rfd;		/* First RFD not processed */
	short iprb_last_rfd;		/* Last RFD available to IPRB */
	short iprb_current_rfd;		/* Next RFD to be filled */
	short iprb_first_cmd;		/* First cmd waiting to be executed */
	short iprb_last_cmd;		/* Last cmd waiting to be executed */
	short iprb_current_cmd;		/* Next command available for use */
	short iprb_first_xbd;		/* First tbd waiting for DMA */
	short iprb_last_xbd;		/* Last tbd waiting for DMA */
	struct iprb_mcs_addr iprb_mcs_addrs[IPRB_MAXMCSN]; /* List mc addrs */
	unsigned char iprb_mcs_addrval[IPRB_MAXMCSN]; /* List slots used */
	boolean_t iprb_receive_enabled;	/* Is receiving allowed? */
	boolean_t iprb_initial_cmd; 	/* Is this the first command? */
	unsigned long iprb_cxtnos;	/* CX/TNO Interrupt count */
	unsigned long iprb_cnacis;	/* CNA/CI Interrupt count */
};

#define	IPRB_TYPE_MII		0
#define	IPRB_TYPE_503		1

#define	IPRB_SCB_STATUS		0
#define	IPRB_SCB_CMD		2
#define	IPRB_SCB_PTR		4
#define	IPRB_SCB_PORT		8
#define	IPRB_SCB_FLSHCTL	0xc
#define	IPRB_SCB_EECTL		0xe
#define	IPRB_SCB_MDICTL		0x10
#define	IPRB_SCB_ERCVCTL	0x14

#define	IPRB_MDI_READ		2
#define	IPRB_MDI_READY		0x10000000
#define	IPRB_MDI_CREG		0
#define	IPRB_MDI_SREG		1

#define	IPRB_LOAD_RUBASE	6
#define	IPRB_CU_START		0x10
#define	IPRB_CU_RESUME		0x20
#define	IPRB_CU_LOAD_DUMP_ADDR	0x40
#define	IPRB_CU_DUMPSTAT	0x50
#define	IPRB_LOAD_CUBASE	0x60
#define	IPRB_RU_START		1

#define IPRB_GEN_SWI		(1 << 9)	/* Generate software intr */

#define	IPRB_PORT_SW_RESET	0
#define	IPRB_PORT_SELF_TEST	1
#define	IPRB_PORT_DUMP	3

#define	IPRB_STAT_COMPLETE	0xa005
#define	IPRB_PCI_VENID		0x8086
#define	IPRB_PCI_DEVID		0x1229

#define	IPRB_RFD_COMPLETE	0x8000
#define IPRB_RFD_OK		0x2000
#define IPRB_RFD_CRC_ERR	(1 << 11)
#define IPRB_RFD_ALIGN_ERR	(1 << 10)
#define IPRB_RFD_NO_BUF_ERR	(1 << 9)
#define IPRB_RFD_DMA_OVERRUN	(1 << 8)
#define IPRB_RFD_SHORT_ERR	(1 << 7)
#define IPRB_RFD_TYPE		(1 << 5)
#define IPRB_RFD_PHY_ERR	(1 << 4)
#define IPRB_RFD_IA_MATCH	(1 << 1)
#define IPRB_RFD_COLLISION	(1 << 0)
#define IPRB_RFD_SF		0x0008		/* within rfd_control */
#define IPRB_RFD_H		0x0010		/* within rfd_control */
#define	IPRB_RFD_EL		0x8000
#define	IPRB_EL			0x80000000
#define	IPRB_RBD_COUNT_MASK	0x3fff

#define	IPRB_RBD_EL		0x8000		/* with rbd_size */

/*
 * According to Intel, this could be optimized by keeping track of the
 * number of transmit overruns, and adjusting on the fly.  Apart from
 * doing that (which would mean timeouts in the driver), 0x64 is a good
 * value to use.  Intel was clear not to use the default from their header
 * file.
 */

#define	IPRB_XMIT_THRESHOLD	0x64

#define IPRB_NOP_CMD		0
#define	IPRB_IAS_CMD		1
#define	IPRB_CFG_CMD		2
#define	IPRB_MCS_CMD		3
#define IPRB_XMIT_CMD		4
#define IPRB_RESV_CMD		5
#define IPRB_DUMP_CMD		6
#define IPRB_DIAG_CMD		7

/* Command Unit command word bits */
#define IPRB_SF		0x0008		/* Simplified = 0 Flexible = 1 */
#define	IPRB_INTR	0x2000		/* Interrupt upon completion */
#define	IPRB_SUSPEND	0x4000		/* Suspend upon completion */
#define	IPRB_EOL	0x8000		/* End Of List */

/* Command unit status word bits */
#define IPRB_CMD_OK		0x2000
#define	IPRB_CMD_COMPLETE	0x8000

#define	IPRB_SCB_INTR_MASK	0xff00
#define	IPRB_INTR_CXTNO		0x8000
#define	IPRB_INTR_FR		0x4000
#define	IPRB_INTR_CNACI		0x2000
#define	IPRB_INTR_RNR		0x1000
#define	IPRB_INTR_MDI		0x800
#define	IPRB_INTR_SWI		0x400

#define	IPRB_EEDI		0x04
#define	IPRB_EEDO		0x08
#define	IPRB_EECS		0x02
#define	IPRB_EESK		0x01
#define	IPRB_EEPROM_READ	0x06

#define	IPRB_STATSIZE		68

#define	IPRB_TBD_COUNT(xmit_count)	(xmit_count & ~(IPRB_XMIT_EOF))

#define	IPRB_RBD_COUNT(rbd_count)	(rbd_count & IPRB_RBD_COUNT_MASK)

#define	IPRB_CMD_MASK 0xff	/* Command portion of command register */

#define	IPRB_SCBWAIT(macinfo) { \
	register int ntries = 10000;					\
	do {								\
		if ((inb((macinfo)->gldm_port + IPRB_SCB_CMD)		\
				& IPRB_CMD_MASK) == 0)			\
			break;						\
		drv_usecwait(10);					\
	} while (--ntries > 0);						\
	if (ntries == 0)						\
		cmn_err(CE_WARN, "iprb: device never responded!\n");	\
}
#endif
