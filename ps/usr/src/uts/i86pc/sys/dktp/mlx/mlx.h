/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#ifndef _SYS_DKTP_MLX_MLX_H
#define	_SYS_DKTP_MLX_MLX_H

#pragma ident	"@(#)mlx.h	1.15	95/10/30 SMI"

#ifdef	__cplusplus
extern "C" {
#endif


/*
 * Mylex DAC960 Host Adapter Driver Header File.  Driver private
 * interfaces, common between all the SCSI and non-SCSI instances.
 */

#if defined(ppc)
#define	static				/* vla fornow */
#define	printf	prom_printf		/* vla fornow */
#endif

#include <sys/types.h>
#include <sys/ddidmareq.h>
#include <sys/modctl.h>
#include <sys/sunddi.h>
#include <sys/ddi_impldefs.h>

#include <sys/dktp/hba.h>
#include <sys/scsi/scsi.h>
#include <sys/scsi/impl/transport.h>

#include <sys/dktp/objmgr.h>
#include <sys/dktp/dadkio.h>
#include <sys/dktp/controller.h>
#include <sys/dktp/cmpkt.h>
#include <sys/dktp/gda.h>
#include <sys/dktp/tgdk.h>
#include <sys/file.h>

#include <sys/dktp/mscsi.h>

#include <sys/pci.h>

#ifdef	PCI_DDI_EMULATION
#define	ddi_io_getb(a, b)	inb(b)
#define	ddi_io_getw(a, b)	inw(b)
#define	ddi_io_getl(a, b)	inl(b)
#define	ddi_io_putb(a, b, c)	outb(b, c)
#define	ddi_io_putw(a, b, c)	outw(b, c)
#define	ddi_io_putl(a, b, c)	outl(b, c)
#endif

typedef	unsigned char bool_t;
typedef	ulong	ioadr_t;

#include <sys/dktp/mlx/mlx_dac.h>
#include <sys/dktp/mlx/mlx_scsi.h>

#define	MLX_PCI_RNUMBER			1
#define	MLX_EISA_RNUMBER		0

/* PCI ID Address offsets */
#define	MLX_PCI_LOCAL_DBELL		0x40   	/* Local Doorbell reg	*/
#define	MLX_PCI_SYS_DBELL 		0x41   	/* System Doorbell reg 	*/
#define	MLX_PCI_INTR			0x43	/* intr enable/disable reg */
#define	MLX_PCI_VENDOR_ID		0x1069	/* Mylex vendor id	*/
#define	MLX_PCI_DEVICE_ID		0x0001	/* device id		*/

/* EISA ID Address offsets */
#define	MLX_EISA_CONFIG0		0xC80	/* For EISA ID 0 */
#define	MLX_EISA_CONFIG1		0xC81	/* For EISA ID 1 */
#define	MLX_EISA_CONFIG2		0xC82	/* For EISA ID 2 */
#define	MLX_EISA_CONFIG3		0xC83	/* For EISA ID 3 */
#define	MLX_EISA_LOCAL_DBELL		0xC8D   /* Local Doorbell register */
#define	MLX_EISA_SYS_DBELL 		0xC8F   /* System Doorbell register */

/* EISA IDs for Different models */
#define	MLX_EISA_ID0		0x35	/* EISA ID 0 */
#define	MLX_EISA_ID1		0x98	/* EISA ID 1 */
#define	MLX_EISA_ID2		0x00	/* EISA ID 2 */
#define	MLX_EISA_ID3		0x70	/* EISA ID 3 */
#define	MLX_EISA_ID3_MASK	0xF0	/* For any DAC960 */

/* To Get the Configured IRQ */
#define	MLX_IRQCONFIG	0xCC3	 /* offset from Base */
#define	MLX_IRQMASK	0x60
#define	MLX_IRQ15	15
#define	MLX_IRQ14	14
#define	MLX_IRQ12	12
#define	MLX_IRQ11	11
#define	MLX_IRQ10	10

/* Offsets for status information */
#define	MLX_NSG  	0xC9C	/* # of s/g elements  */
#define	MLX_STAT_ID	0xC9D	/* Command Identifier passed */
#define	MLX_STATUS	0xC9E	/* Status(LSB) for completed command */
#define	MLX_INTR_DEF1	0xC8E	/* enable/disable register */
#define	MLX_INTR_DEF2	0xC89	/* enable/disable register */
#define	MLX_MBXOFFSET	0xC90   /* offset from base */

/* Mail Box offsets from MLX_MBXOFFSET */
#define	MLX_MBX0	0x00	/* Command Code */
#define	MLX_MBX1	0x01	/* Command ID */
#define	MLX_MBX2	0x02	/* Block count,Chn,Testno, bytecount */
#define	MLX_MBX3	0x03	/* Tgt,Pas,bytecount	*/
#define	MLX_MBX4	0x04	/* State,Chn	*/
#define	MLX_MBX5	0x05
#define	MLX_MBX6	0x06
#define	MLX_MBX7	0x07	/* Drive */
#define	MLX_MBX8	0x08	/* Paddr */
#define	MLX_MBX9	0x09	/* Paddr */
#define	MLX_MBXA	0x0A	/* Paddr */
#define	MLX_MBXB	0x0B	/* Paddr */
#define	MLX_MBXC	0x0C	/* Scatter-Gather type	*/
#define	MLX_MBXD	0x0D	/* Command ID Passed in MLX_MBX1 */
#define	MLX_MBXE	0x0E	/* Status */
#define	MLX_MBXF	0x0F	/* Status */

/* Equates Same as above */
#define	MLX_MBXCMD	MLX_MBX0	/* Command Opcode */
#define	MLX_MBXCMDID	MLX_MBX1	/* Command Identifier */
#define	MLX_MBXBCOUNT	MLX_MBX2	/* Number of blocks */
#define	MLX_MBXBLOCK0	MLX_MBX4	/* Start Block Number:LSB */
#define	MLX_MBXBLOCK1	MLX_MBX5	/* Start Block Number:LSB */
#define	MLX_MBXBLOCK2	MLX_MBX6	/* Start Block Number:LSB */
#define	MLX_MBXBLOCK3	MLX_MBX3	/* Start Block Number:MSB (2bits) */
#define	MLX_MBXDRIVE	MLX_MBX7	/* Drive Number */
#define	MLX_MBXPADDR0	MLX_MBX8	/* Physical Address in Host Mem:LSB */
#define	MLX_MBXPADDR1	MLX_MBX9	/* Physical Address in Host Mem:LSB */
#define	MLX_MBXPADDR2	MLX_MBXA	/* Physical Address in Host Mem:LSB */
#define	MLX_MBXPADDR3	MLX_MBXB	/* Physical Address in Host Mem:MSB */
#define	MLX_MBXCHAN	MLX_MBX2	/* SCSI Chan on which device is */
#define	MLX_MBXTRGT	MLX_MBX3	/* SCSI Target ID of device */
#define	MLX_MBXSTATE	MLX_MBX4	/* Final  state expected out of dev */
#define	MLX_MBXPARAM	MLX_MBX2	/* For Type 5 command's param */

#define	MLX_CMBXFREE	0x0	/* MLX Command mail box is free */
#define	MLX_STATREADY	0x1	/* MLX Command Status ready */
#define	MLX_NEWCMD	0x1	/* Put in EISA Local Doorbell for new cmd */
#define	MLX_STATCMP	0x2	/* Set LDBELL for status completion */
#define	MLX_DAC_ACKINTR	0x1	/* DAC960P interrupt acknowledgement	*/

#define	MLX_MAX_RETRY		300000
#define	MLX_BLK_SIZE		512
#define	MLX_MAX_NSG		17	/* max number of s/g elements */
#define	MLX_MAXCMDS		64
#define	MLX_CARD_SCSI_ID	7	/* never changes regardless of slot */
#define	MLX_DISABLE_INTERRUPTS	0
#define	MLX_ENABLE_INTERRUPTS	1

/* Status or Error Codes */
#define	MLX_SUCCESS		0x00	/* Normal Completion */
#define	MLX_E_UNREC		0x01	/* Unrecoverable data error */
#define	MLX_E_NODRV		0x02	/* System Drive does not exist */
#define	MLX_E_RBLDONLINE	0x02	/* Attempt to rebuild online drive */
#define	MLX_E_DISKDEAD		0x02	/* SCSI Disk on Sys Drive is dead */
#define	MLX_E_BADBLK		0x03	/* Some Bad Blks Found */
#define	MLX_E_RBLDFAIL		0x04	/* New Disk Failed During Rebuild */
#define	MLX_E_NDEVATAD		0x102	/* No Device At Address Specified */
#define	MLX_E_INVALSDRV		0x105	/* A RAID 0 Drive */
#define	MLX_E_LIMIT		0x105	/* Attempt to read beyond limit */
#define	MLX_E_INVALCHN		0x105	/* Invalid Address (Channel) */
#define	MLX_E_INVALTGT		0x105	/* Invalid Address (Target) */
#define	MLX_E_NEDRV		0x105	/* Non Existent System Drive */
#define	MLX_E_NORBLDCHK		0x105	/* No REBUILD/CHECK in progress */
#define	MLX_E_CHN_BUSY		0x106 	/* Channel Busy */
#define	MLX_E_INPROGRES		0x106	/* Rebuild/Check is in progress */

#define	MLX_INV_STATUS		0xa5a5	/* pre-init values for status	*/

#pragma	pack(1)

/* Scatter-Gather format for Scatter-Gather write-read commands */
#define	MLX_SGTYPE0	0x0	/* 32 bit addr and count */
#define	MLX_SGTYPE1	0x1	/* 32 bit addr and 16 bit count */
#define	MLX_SGTYPE2	0x2	/* 32 bit count and 32 bit addr */
#define	MLX_SGTYPE3	0x3	/* 16 bit count and 32 bit addr */

typedef struct mlx_sg_element {
	union {
		struct {
			ulong	data01_ptr32;	/* 32 bit data pointer */
			ulong	data02_len32;	/* 32 bit data length  */
		} type0;
		struct {
			ulong	data11_ptr32;	/* 32 bit data pointer */
			ushort	data12_len16;	/* 16 bit data length  */
		} type1;
		struct {
			ulong	data21_len32;	/* 32 bit data length  */
			ulong	data22_ptr32;	/* 32 bit data pointer */
		} type2;
		struct {
			ushort  data31_len32;	/* 32 bit data length  */
			ulong   data32_ptr16;	/* 16 bit data pointer */
		} type3;
	} fmt;
} mlx_sg_element_t;

#define	data01_ptr32	fmt.type0.data01_ptr32
#define	data02_len32	fmt.type0.data02_len32

#define	data11_ptr32	fmt.type1.data11_ptr32
#define	data12_len16	fmt.type1.data12_len16

#define	data21_len32	fmt.type2.data21_len32
#define	data22_ptr32	fmt.type2.data22_ptr32

#define	data31_len32	fmt.type3.data31_len32
#define	data32_ptr16	fmt.type3.data32_ptr16

/* Status Block */
#define	MLX_BAD_OPCODE	0x104
typedef struct mlx_stat	{
	u_char stat_id;		/* MLX_MBXD */
	u_short status;		/* MLX_MBXE MLX_MBXF, 0 == success */
} mlx_stat_t;

/* Command Block */
typedef struct mlx_cmd {
	u_char opcode;		/* MLX_MBX0 or MLX_MBXCMD   */
	u_char cmdid;		/* MLX_MBX1 or MLX_MBXCMDID */

	union {
		struct {
			u_char arr[0xB]; /* MLX_MBX2-C */
		} type0;
		struct {
			u_char cnt;	/* MLX_MBX2 */
			u_char blk[4];	/* MLX_MBX3-6 */
			u_char drv;	/* MLX_MBX7 */
			u_long ptr;	/* MLX_MBX8-B */
			u_char sg_type; /* MLX_MBXC */
		} type1;
		struct {
			u_char chn;	/* MLX_MBX2 */
			u_char tgt;	/* MLX_MBX3 */
			u_char state;	/* MLX_MBX4 */
			u_char fill[3];	/* MLX_MBX4-7 */
			u_long ptr;	/* MLX_MBX8-B */
			u_char fill1;	/* MLX_MBXC */
		} type2;
		struct {
			u_char fill[6];	/* MLX_MBX2-7 */
			u_long ptr;	/* MLX_MBX8-B */
			u_char fill1;	/* MLX_MBXC */
		} type3;
		struct {
			u_char test;	/* MLX_MBX2 */
			u_char pass;	/* MLX_MBX3 */
			u_char chn;	/* MLX_MBX4 */
			u_char fill[3];	/* MLX_MBX5-7 */
			u_long ptr;	/* MLX_MBX8-B */
			u_char fill1;	/* MLX_MBXC */
		} type4;
		struct {
			u_char param;	/* MLX_MBX2 */
			u_char fill[5];	/* MLX_MBX3-7 */
			u_long ptr;	/* MLX_MBX8-B */
			u_char fill1;	/* MLX_MBXC */
		} type5;
		struct {
			u_short cnt;	/* MLX_MBX2-3 */
			u_long off;	/* MLX_MBX4-7 */
			u_long ptr;	/* MLX_MBX8-B */
			u_char fill;	/* MLX_MBXC */
		} type6;
	} fmt;
	mlx_stat_t hw_stat;	/* MLX_MBXD-F */
} mlx_cmd_t;

#pragma	pack()

/* Command Control Block.  One per SCSI or Mylex specific command. */
#define	MLX_INVALID_CMDID	0xFF

typedef struct mlx_ccb {
	mlx_cmd_t cmd;			/* physical mlx command */

	u_char	intr_wanted;
	u_char  type;
	paddr_t paddr;			/* paddr of this mlx_ccb_t */
	long bytexfer;			/* xfer size requested */

	union {
		struct {
			mlx_dacioc_args_t da;
			ksema_t da_sema;
		} dacioc_args;
		struct {
			mlx_cdbt_t *cdbt;
			struct scsi_arq_status arq_stat;
		} scsi_args;	/* Direct CDB w/ or w/o scatter-gather */
	} args;

	union {
		struct { /* types 1 and 3 are capable of Scatter-Gather xfer */
			mlx_sg_element_t list[MLX_MAX_NSG];
			u_char type;				/* MLX_MBXC */
		} sg;
		struct {
			ushort ubuf_len;	/* expected correct ubuf_len */
			u_char flags;				/* see below */
		} ioc;
	} si;

	union {
		char *scratch;  	/* spare buffer space		*/
		struct  scsi_cmd *ownerp; /* owner pointer- to 	*/
					/* point back to the packet	*/
	} cc;
} mlx_ccb_t;

#define	ccb_opcode		cmd.opcode
#define	ccb_cmdid		cmd.cmdid
#define	ccb_drv			cmd.fmt.type1.drv
#define	ccb_blk			cmd.fmt.type1.blk

#define	CCB_BLK(ccbp, v)	ccbp->ccb_blk[1] = (unchar)(v);		\
				ccbp->ccb_blk[2] = (unchar)((v)>>8);	\
				ccbp->ccb_blk[3] = (unchar)((v)>>16);	\
				ccbp->ccb_blk[0] = (unchar)((v)>>18) & 0xC0;

#define	ccb_arr			cmd.fmt.type0.arr
#define	ccb_cnt			cmd.fmt.type1.cnt
#define	ccb_chn			cmd.fmt.type2.chn
#define	ccb_tgt			cmd.fmt.type2.tgt
#define	ccb_dev_state		cmd.fmt.type2.state
#define	ccb_test		cmd.fmt.type4.test
#define	ccb_pass		cmd.fmt.type4.pass
#define	ccb_chan   		cmd.fmt.type4.chn
#define	ccb_param		cmd.fmt.type5.param
#define	ccb_count		cmd.fmt.type6.cnt
#define	ccb_offset		cmd.fmt.type6.off
#define	ccb_xferpaddr		cmd.fmt.type1.ptr
#define	ccb_sg_type		cmd.fmt.type1.sg_type

#define	ccb_hw_stat		cmd.hw_stat
#define	ccb_stat_id		cmd.hw_stat.stat_id
#define	ccb_status		cmd.hw_stat.status

#define	ccb_gen_args		args.dacioc_args.da.type_gen.gen_args
#define	ccb_gen_args_len	args.dacioc_args.da.type_gen.gen_args_len
#define	ccb_xferaddr_reg	args.dacioc_args.da.type_gen.xferaddr_reg
#define	ccb_da_sema		args.dacioc_args.da_sema
#define	ccb_cdbt		args.scsi_args.cdbt
#define	ccb_arq_stat		args.scsi_args.arq_stat
#define	ccb_sg_list		si.sg.list
#define	ccb_flags		si.ioc.flags
#define	ccb_ubuf_len		si.ioc.ubuf_len
#define	ccb_scratch		cc.scratch
#define	ccb_ownerp		cc.ownerp

/* Possible values for ccb_flags field of mlx_ccb_t */
#define	MLX_CCB_DACIOC_UBUF_TO_DAC	0x1 /* data xfer from user to DAC960 */
#define	MLX_CCB_DACIOC_DAC_TO_UBUF	0x2 /* data xfer from DAC960 to user */
#define	MLX_CCB_DACIOC_NO_DATA_XFER	0x4 /* no data xfer during dacioc op */
#define	MLX_CCB_UPDATE_CONF_ENQ		0x8 /* update mlx->conf and mlx->enq */
#define	MLX_CCB_GOT_DA_SEMA		0x10 /* ccb_da_sema initialized */

/* ccb stack element */
typedef struct mlx_ccb_stk {
	/*
	 * As -1 is an invalid index and used as an indicator, and to
	 * prevent future expansion problems, such as 255 max_cmd,
	 * type short is taken for the field next instread of char.
	 */
	short next;		/* next free index in the stack */
	mlx_ccb_t *ccb;
} mlx_ccb_stk_t;

/*
 * Per DAC960 card info shared by all its the channels.
 * The head of this linked list is pointed to by mlx_cards.
 */
typedef struct mlx {
	/*
	 * The following fields are initialized only once while being
	 * protected by a global lock, and read many times after that
	 * without any locks.
	 */
	struct mlxops *ops;	/* ptr to MLX SIOP ops table */
	int reg;		/* actual io address */
	int *regp;		/* copy of regs property */
	int reglen;		/* length of regs property */
	volatile u_char *membase; /* shared memory address */
	int rnum;		/* register number */

	ddi_acc_handle_t handle;	/* io access handle */

	u_char initiatorid;	/* Always 7 but worth keeping this around */
	u_char nchn;		/* NCHN, number of channels */
	u_char max_tgt;		/* MAX_TGT, max # of targets PER channel */
	u_char max_cmd;		/* # of simultaneous cmds, including ncdb  */
	enum {UNKNOWN, R1V22, R1V23, R1V5x} fw_version;

	u_char irq;		/* IRQ */
	u_int intr_idx;		/* index of the interrupt vector */
	ddi_iblock_cookie_t iblock_cookie;
	opaque_t scsi_cbthdl;	/* call back thread to handle interrupts */
	dev_info_t *dip;	/* of the instance which cardinit'd */
	dev_info_t *idip;	/* of the instance which added intr handler */

	ksema_t scsi_ncdb_sema;	/* controls # of simultaneous DCDB's  */
	kmutex_t mutex;
	/*
	 * Access to the following need to be protected
	 *	- Only by mlx_global_mutex during attach or detach.
	 * 	- Only by the above mutex at run time.
	 */
	mlx_dac_conf_t *conf;
	mlx_dac_enquiry_t *enq;
	kcondvar_t ccb_stk_cv;
	mlx_ccb_stk_t *ccb_stk;		/* stack of max_cmd outstanding ccb's */
	mlx_ccb_stk_t *free_ccb;	/* head of free list in ccb_stk	*/
	ushort flags;			/* see below */
	u_char refcount;
	int attach_calls;
	int sgllen;			/* per unit sgllen */

	/* The following has to be protected only by the mlx_global_mutex. */
	struct mlx *next;
} mlx_t;

/* Possible values for flags field of mlx_t */
#define	MLX_CARD_CREATED	0x1
#define	MLX_GOT_ROM_CONF	0x2
#define	MLX_GOT_ENQUIRY		0x4
#define	MLX_CCB_STK_CREATED	0x8
#define	MLX_INTR_IDX_SET	0x10
#define	MLX_INTR_SET		0x20
#define	MLX_CBTHD_CREATED	0x40
#define	MLX_SUPPORTS_SG		0x80	/* f/w supports scatter-gather io */
#define	MLX_NO_HOT_PLUGGING	0x100
/*
 * Per channel(hba) info shared by all the units on the channel, plus one
 * extra (chn == 0) for all the System Drives.
 */
typedef struct mlx_hba {
	/*
	 * The following fields are initialized only once while being
	 * protected by a global lock, and read many times after that
	 * without any locks.
	 */
	dev_info_t *dip;
	u_char chn;			/* channel number */
	mlx_t *mlx;			/* back ptr to the card info */
	mlx_ccb_t *ccb;			/* used only during init */
	struct scsi_inquiry *scsi_inq;	/* NULL if System-Drive hba */
	u_char flags;			/* see below */
	int callback_id;	/* will be protected by framework locks */

	kmutex_t mutex;
	/*
	 * Access to the following need to be protected
	 *	- Only by mlx_global_mutex during attach or detach.
	 * 	- Only by the above mutex at run time.
	 */
	caddr_t pkt_pool;
	caddr_t ccb_pool;
	ushort refcount;	/* # of active children */
} mlx_hba_t;

/* Possible values for flags field of mlx_hba_t */
#define	MLX_HBA_DAC		1
#define	MLX_HBA_ATTACHED	2

/* Per SCSI unit or System-Drive */
typedef struct mlx_unit {
	scsi_hba_tran_t	*scsi_tran;
	mlx_dac_unit_t dac_unit;
	mlx_hba_t *hba;				/* back ptr to the hba info */
	u_int capacity;				/* scsi capacity */
	ddi_dma_lim_t dma_lim;
	u_char	scsi_auto_req	: 1,		/* auto-request sense enable */
		scsi_tagq	: 1,		/* tagged queueing enable */
		reserved	: 6;
} mlx_unit_t;

/* Convenient macros. */
#define	MLX_DAC(hba)	((hba)->flags & MLX_HBA_DAC)
#define	MLX_SCSI(hba)	(!MLX_DAC(hba))
#define	MLX_OFFSET(basep, fieldp)    ((char *)(fieldp) - (char *)(basep))
#define	MLX_MIN(a, b)	((a) <= (b) ? (a) : (b))
#define	MLX_MAX(a, b)	((a) >= (b) ? (a) : (b))

#define	MLX_EISA(mlx)	(((mlx_t *)mlx)->ops == &dac960_nops)
#define	MLX_MC(mlx)	(((mlx_t *)mlx)->ops == &dmc960_nops)
#define	MLX_PCI(mlx)	(((mlx_t *)mlx)->ops == &dac960p_nops)

#define	MLX_ADDR(iobase)(((uint)iobase) & ~0xff)
#define	MLX_SLOT(iobase)(((uint)iobase) >> 12)

/*
 * 2.4 reg layout: In 2.4, the base_port/reg bustype variable
 * must represent all possible eisa, mc, and pci cards, and
 * all channels associated with each card.
 *
 * eisa/mc hwconf entries may overlap, because they will never
 * coexist. The layout is
 *
 *	0xS0CC
 *
 *	S = slot (0x0-F for eisa, 0x0-7 for mc)
 *	CC = channel (0xFF for raid channel, 0-FE for scsi channels)
 *
 * 2.4 pci entries must have distinct values because pci/eisa boards
 * may both appear. The MLX_PCI_BUS (0x800) value is used to uniqify
 * pci entries from eisa entries. This layout restricts the 2.4 pci
 * implementation to supporting a maximum of 15 channels, and 7 pci
 * busses. The layout is
 *
 *	0xBBDFCC
 *
 *	BB = pci bus
 *	D = pci device (bits 0-3 for pci device (plus high bit of pci function)
 *	F = pci function (bits 0-2 for pci function, high bit for pci device)
 *	CC = channel (0xFF for raid channel, 0-FE for scsi channel numbers)
 *
 * Note that the 2.4 driver cannot use the mscsi bus nexus driver until
 * the 2.4 realmode framework allows relative bootpaths to be passed,
 * and the 2.4 ufsboot constructs bootpaths based on this relative bootpath.
 *
 *
 * 2.5 reg layout: In 2.5, the base_port/reg bustype variable
 * must represent all possible eisa and mc cards.
 *
 * The layout is as above, except that a single entry in the hwconf
 * file is used to represent the system driver virtual channel of each
 * card.
 *
 * The channel number for scsi channels is set by the mscsi-bus
 * property from the child mscsi hba bus nexus driver.
 *
 * eisa/mc hwconf entries may overlap, because they will never
 * coexist. The layout is as above. Note that the raid system drive
 * channel MLX_DAC_CHN_NUM (0xff) is associated with the single entry
 * in the hwconf file per card.
 *
 * 2.5 pci entries will not appear in the hwconf file. The default
 * channel value for the devinfo nodes associated with the pci card
 * will be set to MLX_DAC_CHN_NUM.
 *
 * The channel number for scsi channels is set by the mscsi-bus
 * property from the child mscsi hba bus nexus driver.
 *
 */
#define	MLX_CHN(p, b)	(p && MLX_PCI(p) ? MLX_DAC_CHN_NUM : (((uint)b) & 0xff))

#if defined(DEBUG)
#define	MLX_DEBUG 1
#endif /* DEBUG */

/* Card Types and Ids */
typedef	unchar	bus_t;
#define	BUS_TYPE_EISA		((bus_t)1)
#define	BUS_TYPE_MC		((bus_t)2)
#define	BUS_TYPE_PCI		((bus_t)3)

#define	DMC960_ID1		0x8F82	/* cheetah DMC960 card id. */
#define	DMC960_ID2		0x8FBB	/* passplay DMC960 card id. */

/* from xmca.h */

#define	MCA_SETUP_PORT	0x96
#define	MCA_SETUP_ON	((unchar)0x08)
#define	MCA_SETUP_MASK	((unchar)~0x0f)

#define	MCA_POS_BASE	0x100
#define	MCA_POS_MAX	0x107
#define	MCA_ID_PORT	(MCA_POS_BASE + 0)
#define	MCA_SETUP_102	(MCA_POS_BASE + 2)
#define	DUMMY		0x0	/* dummy parameter-don't care */
#define	MCA_OFF		0xC00

/* from dmc.h */

#define	DMC_IRQ_MASK		0xC0		/* 11000000 binary. */
#define	DMC_IRQ_MASK_SHIFT	6		/* Number of bits-to-shift */
#define	BIOS_BASE_ADDR_MASK	0x3C		/* 00111100 binary. */
#define	BBA_MASK_SHIFT		2		/* Number of bits-to-shift */
#define	IO_BASE_ADDR_MASK	0x38		/* 00111000 binary. */
#define	IOBA_MASK_SHIFT		3		/* Number of bits-to-shift */
/* dmc specific registers */

#define	CMD_PORT		0x00		/* Command Port. */
#define	ATTENTION_PORT		0x04		/* Attention Port. */
#define	SCP_REG			0x05		/* System Control Port. */
#define	ISP_REG			0x06		/* Interrupt Status Port. */
#define	CBSP_REG		0x07		/* Command Busy/Status Port. */
#define	DIIP_REG		0x08		/* Device Interrupt ID Port */

#define	DMC_RESET_ADAPTER	0x40	/* Reset DMC960 Adapter. */

#define	DMC_ENABLE_BUS_MASTERING 0x02	/* Enable Bus Mastering on DMC960. */
#define	DMC_ENABLE_INTRS	0x01	/* Enable (MIAMI) DMC960 intrs. */
#define	DMC_DISABLE_INTRS	0x00	/* Disable (MIAMI) DMC960 intrs. */
#define	DMC_CLR_ON_READ		0x40	/* Disable clear IV thru read. */

#define	DMC_INTR_VALID		0x02	/* interrupt valid (IV) bit	*/

#define	DMC_NEWCMD	0xD0	/* Put in ATTENTION port for new cmd.	*/
#define	DMC_ACKINTR	0xD1	/* "status-accepted" interrupt-ack	*/

#ifdef	DADKIO_RWCMD_READ
#define	RWCMDP	((struct dadkio_rwcmd *)(cmpkt->cp_bp->b_back))
#endif

/*
 * Handy constants
 */

/* For returns from xxxcap() functions */

#define	FALSE		0
#define	TRUE		1
#define	UNDEFINED	-1

/*
 * Handy macros
 */
#if defined(ppc)
#define	MLX_KVTOP(vaddr) \
		CPUPHYS_TO_IOPHYS( \
		((paddr_t)(hat_getkpfnum((caddr_t)(vaddr)) << (PAGESHIFT)) | \
			    ((paddr_t)(vaddr) & (PAGEOFFSET))))
#else
#define	MLX_KVTOP(vaddr)	(HBA_KVTOP((vaddr), mlx_pgshf, mlx_pgmsk))
#endif

/* Make certain the buffer doesn't cross a page boundary */
#define	PageAlignPtr(ptr, size)	\
	(caddr_t)(btop((unsigned)(ptr)) != btop((unsigned)(ptr) + (size)) ? \
	ptob(btop((unsigned)(ptr)) + 1) : (unsigned)(ptr))

/*
 * Debugging stuff
 */
#define	Byte0(x)		(x&0xff)
#define	Byte1(x)		((x>>8)&0xff)
#define	Byte2(x)		((x>>16)&0xff)
#define	Byte3(x)		((x>>24)&0xff)

/*
 * include all of the function prototype declarations
 */
#include <sys/dktp/mlx/mlxops.h>
#include <sys/dktp/mlx/mlxdefs.h>
#include <sys/dktp/mlx/debug.h>

#ifdef	__cplusplus
}
#endif

#endif /* _SYS_DKTP_MLX_MLX_H */
