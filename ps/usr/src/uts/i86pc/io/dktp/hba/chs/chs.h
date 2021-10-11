/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _SYS_DKTP_CHS_CHS_H
#define	_SYS_DKTP_CHS_CHS_H

#pragma ident	"@(#)chs.h	1.4	96/07/17 SMI"

#ifdef	__cplusplus
extern "C" {
#endif


/*
 * IBM PCI RAID Host Adapter Driver Header File.  Driver private
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

#include "chs_dac.h"
#include "chs_scsi.h"
#include "chs_viper.h"
#include "chs_queue.h"

#define	CHS_PCI_RNUMBER			1

/* PCI ID Address offsets */
#define	CHS_PCI_LOCAL_DBELL		0x40   	/* Local Doorbell reg	*/
#define	CHS_PCI_SYS_DBELL 		0x41   	/* System Doorbell reg 	*/
#define	CHS_PCI_INTR		0x43	/* intr enable/disable reg */

#define	CHS_PCI_VPR_VENDOR_ID	0x1014	/* Copperhead vendor id	*/
#define	CHS_PCI_VPR_DEVICE_ID	0x002E	/* device id		*/


/* Mail Box offsets from CHS_MBXOFFSET */
#define	CHS_MBX0	0x00	/* Command Code */
#define	CHS_MBX1	0x01	/* Command ID */
#define	CHS_MBX2	0x02	/* Block count,Chn,Testno, bytecount */
#define	CHS_MBX3	0x03	/* Tgt,Pas,bytecount	*/
#define	CHS_MBX4	0x04	/* State,Chn	*/
#define	CHS_MBX5	0x05
#define	CHS_MBX6	0x06
#define	CHS_MBX7	0x07	/* Drive */
#define	CHS_MBX8	0x08	/* Paddr */
#define	CHS_MBX9	0x09	/* Paddr */
#define	CHS_MBXA	0x0A	/* Paddr */
#define	CHS_MBXB	0x0B	/* Paddr */
#define	CHS_MBXC	0x0C	/* Scatter-Gather type	*/
#define	CHS_MBXD	0x0D	/* Command ID Passed in CHS_MBX1 */
#define	CHS_MBXE	0x0E	/* Status */
#define	CHS_MBXF	0x0F	/* Status */

/* Equates Same as above */
#define	CHS_MBXCMD	CHS_MBX0	/* Command Opcode */
#define	CHS_MBXCMDID	CHS_MBX1	/* Command Identifier */
#define	CHS_MBXBCOUNT	CHS_MBX2	/* Number of blocks */
#define	CHS_MBXBLOCK0	CHS_MBX4	/* Start Block Number:LSB */
#define	CHS_MBXBLOCK1	CHS_MBX5	/* Start Block Number:LSB */
#define	CHS_MBXBLOCK2	CHS_MBX6	/* Start Block Number:LSB */
#define	CHS_MBXBLOCK3	CHS_MBX3	/* Start Block Number:MSB */
						/* (2bits) */
#define	CHS_MBXDRIVE	CHS_MBX7	/* Drive Number */
#define	CHS_MBXPADDR0	CHS_MBX8	/* Physical Address in Host */
						/* Mem:LSB */
#define	CHS_MBXPADDR1	CHS_MBX9	/* Physical Address in Host */
						/* Mem:LSB */
#define	CHS_MBXPADDR2	CHS_MBXA	/* Physical Address in Host */
						/* Mem:LSB */
#define	CHS_MBXPADDR3	CHS_MBXB	/* Physical Address in Host */
						/* Mem:MSB */
#define	CHS_MBXCHAN	CHS_MBX2	/* SCSI Chan on which device is */
#define	CHS_MBXTRGT	CHS_MBX3	/* SCSI Target ID of device */
#define	CHS_MBXSTATE	CHS_MBX4	/* Final state expected out */
						/* of dev */
#define	CHS_MBXPARAM	CHS_MBX2	/* For Type 5 command's param */

#define	CHS_CMBXFREE	0x0	/* CHS Command mail box is free */
#define	CHS_STATREADY	0x1	/* CHS Command Status ready */


#define	CHS_MAX_RETRY		300000
#define	CHS_BLK_SIZE		512
#define	CHS_MAX_NSG		17	/* max number of s/g elements */
#define	CHS_MAXCMDS		64
#define	CHS_CARD_SCSI_ID	7	/* never changes regardless of slot */

/*
 * Status or Error Codes
 * Set of uniform status codes to be used in case of future
 * expansion of the driver for other chips.
 * Chip specific statuses will be translated to these as needed
 */

#define	CHS_SUCCESS		0x00	/* Normal Completion */
#define	CHS_FAILURE		0x01	/* Failure */
#define	CHS_INVALCHNTGT	0x02	/* invalid channel */
#define	CHS_NORBLD		0x04	/* No rebuild check */
#define	CHS_INV_OPCODE	0x08



#pragma	pack(1)

/* Scatter-Gather format for Scatter-Gather write-read commands */
#define	CHS_SGTYPE0	0x0	/* 32 bit addr and count */
#define	CHS_SGTYPE1	0x1	/* 32 bit addr and 16 bit count */
#define	CHS_SGTYPE2	0x2	/* 32 bit count and 32 bit addr */
#define	CHS_SGTYPE3	0x3	/* 16 bit count and 32 bit addr */

typedef struct chs_sg_element {
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
} chs_sg_element_t;

#define	data01_ptr32	fmt.type0.data01_ptr32
#define	data02_len32	fmt.type0.data02_len32

#define	data11_ptr32	fmt.type1.data11_ptr32
#define	data12_len16	fmt.type1.data12_len16

#define	data21_len32	fmt.type2.data21_len32
#define	data22_ptr32	fmt.type2.data22_ptr32

#define	data31_len32	fmt.type3.data31_len32
#define	data32_ptr16	fmt.type3.data32_ptr16

/* Status Block */
typedef struct chs_stat	{
	u_char	reserved;
	u_char	stat_id;	/* CHS_MBXD */
	u_short status;		/* CHS_MBXE CHS_MBXF, 0 == success */
} chs_stat_t;

/* Command Block */
typedef struct chs_cmd {
	u_char opcode;		/* CHS_MBX0 or CHS_MBXCMD   */
	u_char cmdid;		/* CHS_MBX1 or CHS_MBXCMDID */

	union {
		struct {
			u_char arr[0xB]; /* CHS_MBX2-C */
		} type0;
		struct {
			u_char cnt;	/* CHS_MBX2 */
			u_char blk[4];	/* CHS_MBX3-6 */
			u_char drv;	/* CHS_MBX7 */
			u_long ptr;	/* CHS_MBX8-B */
			u_char sg_type; /* CHS_MBXC */
		} type1;
		struct {
			u_char chn;	/* CHS_MBX2 */
			u_char tgt;	/* CHS_MBX3 */
			u_char state;	/* CHS_MBX4 */
			u_char fill[3];	/* CHS_MBX4-7 */
			u_long ptr;	/* CHS_MBX8-B */
			u_char fill1;	/* CHS_MBXC */
		} type2;
		struct {
			u_char fill[6];	/* CHS_MBX2-7 */
			u_long ptr;	/* CHS_MBX8-B */
			u_char fill1;	/* CHS_MBXC */
		} type3;
		struct {
			u_char test;	/* CHS_MBX2 */
			u_char pass;	/* CHS_MBX3 */
			u_char chn;	/* CHS_MBX4 */
			u_char fill[3];	/* CHS_MBX5-7 */
			u_long ptr;	/* CHS_MBX8-B */
			u_char fill1;	/* CHS_MBXC */
		} type4;
		struct {
			u_char param;	/* CHS_MBX2 */
			u_char fill[5];	/* CHS_MBX3-7 */
			u_long ptr;	/* CHS_MBX8-B */
			u_char fill1;	/* CHS_MBXC */
		} type5;
		struct {
			u_short cnt;	/* CHS_MBX2-3 */
			u_long off;	/* CHS_MBX4-7 */
			u_long ptr;	/* CHS_MBX8-B */
			u_char fill;	/* CHS_MBXC */
		} type6;


		struct {
			u_char	chn;
			u_char	tgt;		/* Target or Parameter */
			u_char	fill[8];
			u_char	newchn;		/* New Channel */
			u_char	newtgt;		/* New Target */
			u_char	devstat;	/* State of Device */
			u_char	fill1[1];
		} typea;

		struct {
			u_char	chn;		/* Channel or Logical Drive */
			u_char	tgt;		/* Target or Parameter */
			u_char	fill[4];
			u_long ptr;		/* Pointer to Host Memory */
			u_char	fill1[4];
		} typeb;

		struct {
			u_char	drv;
			u_char	sg_count;	/* Target or Parameter */
			u_char	blkaddr[4];	/* Block Address */
			u_long	ptr;		/* Pointer to Host Memory */
			u_short	count;
			u_char	fill[2];
		} typec;
	} fmt;
	u_char addition[8];
	chs_stat_t hw_stat;	/* CHS_MBXD-F */
} chs_cmd_t;

#pragma	pack()


/* Command Control Block.  One per SCSI or Mylex specific command. */
#define	CHS_INVALID_CMDID	0xFF
/*
 * This cmdid is used for workaround of the Neptune Chipset bug.
 * This cmdid will be used to identify a second spurious I/O write.
 * See bug #1259213
 */
#define	CHS_NEPTUNE_CMDID	0xFE

typedef struct chs_ccb {
	chs_cmd_t cmd;			/* physical chs command */

	u_char	intr_wanted;
	u_char  type;
	paddr_t paddr;			/* paddr of this chs_ccb_t */
	long bytexfer;			/* xfer size requested */

	union {
		struct {
			chs_dacioc_args_t da;
			ksema_t da_sema;
		} dacioc_args;
		struct {
			chs_cdbt_t *cdbt;
			struct scsi_arq_status arq_stat;
		} scsi_args;	/* Direct CDB w/ or w/o scatter-gather */
	} args;

	union {
		struct { /* types 1 and 3 are capable of Scatter-Gather xfer */
			chs_sg_element_t list[CHS_MAX_NSG];
			u_char type;			/* CHS_MBXC */
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
	Qel_t	ccb_q;			/* used for queueing to process*/
					/* in polling or interrupt handling */

} chs_ccb_t;

#define	ccb_opcode		cmd.opcode
#define	ccb_cmdid		cmd.cmdid
#define	ccb_drv			cmd.fmt.type1.drv
#define	ccb_blk			cmd.fmt.type1.blk


#define	CCB_CBLK(ccbp, v)	ccbp->ccb_blkaddr[0] = (unchar)(v);	\
				ccbp->ccb_blkaddr[1] = (unchar)((v)>>8); \
				ccbp->ccb_blkaddr[2] = (unchar)((v)>>16); \
				ccbp->ccb_blkaddr[3] = (unchar)((v)>>24);

#define	ccb_arr			cmd.fmt.type0.arr
#define	ccb_cnt			cmd.fmt.type1.cnt
#define	ccb_chn			cmd.fmt.type2.chn
#define	ccb_tgt			cmd.fmt.type2.tgt
#define	ccb_dev_state		cmd.fmt.type2.state
#define	ccb_ptr			cmd.fmt.type2.ptr
#define	ccb_ptrx		cmd.fmt.type3.ptr

#define	ccb_test		cmd.fmt.type4.test
#define	ccb_pass		cmd.fmt.type4.pass
#define	ccb_chan   		cmd.fmt.type4.chn
#define	ccb_param		cmd.fmt.type5.param
#define	ccb_count		cmd.fmt.type6.cnt
#define	ccb_offset		cmd.fmt.type6.off
#define	ccb_xferpaddr		cmd.fmt.type1.ptr
#define	ccb_sg_type		cmd.fmt.type1.sg_type


#define	ccb_sg_count		cmd.fmt.typec.sg_count
#define	ccb_blkaddr 		cmd.fmt.typec.blkaddr
#define	ccb_cptr		cmd.fmt.typec.ptr
#define	ccb_ccnt		cmd.fmt.typec.count
#define	ccb_cdrv		cmd.fmt.typec.drv
/*
 * hwstat is the same offset for all type0 to type6, a-c commands. so cmd.hwstat
 * even though is defined as type0.hw_stat, it is referring to all
 * types0-6, a-c hw_stats.
 */

#define	ccb_hw_stat		cmd.hw_stat
#define	ccb_stat_id		ccb_hw_stat.stat_id
#define	ccb_status		ccb_hw_stat.status
#define	ccb_viper_stat_id	cmd.viper_hw_stat.stat_id
#define	ccb_viper_status	cmd.viper_hw_stat.status
#define	ccb_viper_hw_stat	cmd.viper_hw_stat


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

/* Possible values for ccb_flags field of chs_ccb_t */
#define	CHS_CCB_DACIOC_UBUF_TO_DAC	0x1 /* data xfer from user to hba */
#define	CHS_CCB_DACIOC_DAC_TO_UBUF	0x2 /* data xfer from hba to user */
#define	CHS_CCB_DACIOC_NO_DATA_XFER	0x4 /* no data xfer during dacioc op */

#define	CHS_CCB_UPDATE_CONF_ENQ	0x8	/*
						 * update chs->conf and
						 * chs->enq
						 */
#define	CHS_CCB_GOT_DA_SEMA		0x10 /* ccb_da_sema initialized */

/* ccb stack element */
typedef struct chs_ccb_stk {
	/*
	 * As -1 is an invalid index and used as an indicator, and to
	 * prevent future expansion problems, such as 255 max_cmd,
	 * type short is taken for the field next instead of char.
	 */
	short next;		/* next free index in the stack */
	chs_ccb_t *ccb;
} chs_ccb_stk_t;

/*
 * Per IBM PCI RAID card info shared by all its the channels.
 * The head of this linked list is pointed to by chs_cards.
 */
typedef struct chs {
	/*
	 * The following fields are initialized only once while being
	 * protected by a global lock, and read many times after that
	 * without any locks.
	 */
	struct chsops *ops;	/* ptr to CHS SIOP ops table */
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
	 *	- Only by chs_global_mutex during attach or detach.
	 * 	- Only by the above mutex at run time.
	 */
	chs_dac_conf_t *conf;
	chs_dac_enquiry_t *enq;
	kcondvar_t ccb_stk_cv;
	chs_ccb_stk_t *ccb_stk;	/* stack of max_cmd outstanding ccb's */
	chs_ccb_stk_t *free_ccb;	/* head of free list in ccb_stk	*/
	ushort flags;			/* see below */
	u_char refcount;
	int attach_calls;
	int sgllen;			/* per unit sgllen */

	caddr_t	vsq;			/*
					 * Virtual address of Viper
					 * start of status queue
					 */

	paddr_t	psq;			/*
					 * Physical address of Viper
					 * start of status queue
					 */
	paddr_t sqtr;			/* Physical address of tail of queue */
	paddr_t	sqhr;

	caddr_t vipercmdblkp;		/* Virtual address of Viper's cmd blk */
	paddr_t vipercmdphys;		/* physical address of vipercmdblkp */

	Que_t	doneq;			/* used as head of done queue for */
					/* processing completed commands */

	/* The following has to be protected only by the chs_global_mutex */
	struct chs *next;
} chs_t;

/* Possible values for flags field of chs_t */
#define	CHS_CARD_CREATED	0x1
#define	CHS_GOT_ROM_CONF	0x2
#define	CHS_GOT_ENQUIRY	0x4
#define	CHS_CCB_STK_CREATED	0x8
#define	CHS_INTR_IDX_SET	0x10
#define	CHS_INTR_SET	0x20
#define	CHS_CBTHD_CREATED	0x40
#define	CHS_SUPPORTS_SG	0x80	/* f/w supports scatter-gather io */
#define	CHS_NO_HOT_PLUGGING	0x100
/*
 * Per channel(hba) info shared by all the units on the channel, plus one
 * extra (chn == 0) for all the System Drives.
 */
typedef struct chs_hba {
	/*
	 * The following fields are initialized only once while being
	 * protected by a global lock, and read many times after that
	 * without any locks.
	 */
	dev_info_t *dip;
	u_char chn;			/* channel number */
	chs_t *chs;		/* back ptr to the card info */
	chs_ccb_t *ccb;		/* used only during init */
	struct scsi_inquiry *scsi_inq;	/* NULL if System-Drive hba */
	u_char flags;			/* see below */
	int callback_id;	/* will be protected by framework locks */

	kmutex_t mutex;
	/*
	 * Access to the following need to be protected
	 *	- Only by chs_global_mutex during attach or detach.
	 * 	- Only by the above mutex at run time.
	 */
	ushort refcount;	/* # of active children */
} chs_hba_t;

/* Possible values for flags field of chs_hba_t */
#define	CHS_HBA_DAC		1
#define	CHS_HBA_ATTACHED	2

/* Per SCSI unit or System-Drive */
typedef struct chs_unit {
	scsi_hba_tran_t	*scsi_tran;
	chs_dac_unit_t dac_unit;
	chs_hba_t *hba;			/* back ptr to the hba info */
	u_int capacity;				/* scsi capacity */
	ddi_dma_lim_t dma_lim;
	u_char	scsi_auto_req	: 1,		/* auto-request sense enable */
		scsi_tagq	: 1,		/* tagged queueing enable */
		reserved	: 6;
} chs_unit_t;

/* Convenient macros. */
#define	CHS_DAC(hba)	((hba)->flags & CHS_HBA_DAC)
#define	CHS_SCSI(hba)	(!CHS_DAC(hba))
#define	CHS_OFFSET(basep, fieldp)    ((char *)(fieldp) - (char *)(basep))
#define	CHS_MIN(a, b)	((a) <= (b) ? (a) : (b))
#define	CHS_MAX(a, b)	((a) >= (b) ? (a) : (b))

#define	CHS_PCI(chs)	(((chs_t *)chs)->ops == &viper_nops)

#define	CHS_ADDR(iobase)(((uint)iobase) & ~0xff)
#define	CHS_SLOT(iobase)(((uint)iobase) >> 12)

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
 * may both appear. The CHS_PCI_BUS (0x800) value is used to uniqify
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
 * channel CHS_DAC_CHN_NUM (0xff) is associated with the single entry
 * in the hwconf file per card.
 *
 * 2.5 pci entries will not appear in the hwconf file. The default
 * channel value for the devinfo nodes associated with the pci card
 * will be set to CHS_DAC_CHN_NUM.
 *
 * The channel number for scsi channels is set by the mscsi-bus
 * property from the child mscsi hba bus nexus driver.
 *
 */
#define	CHS_CHN(p, b)	(p && CHS_PCI(p) ? CHS_DAC_CHN_NUM :\
				(((uint)b) & 0xff))

#if defined(DEBUG)
#define	CHS_DEBUG 1
#endif /* DEBUG */

/* Card Types and Ids */
typedef	unchar	bus_t;
#define	BUS_TYPE_EISA		((bus_t)1)
#define	BUS_TYPE_MC		((bus_t)2)
#define	BUS_TYPE_PCI		((bus_t)3)


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

/* Viper registers */

#define	CPR_REG			0x00		/* Command Port */
#define	APR_REG			0x04		/* Attention Port */
#define	SCPR_REG		0x05		/* Subsystem Control Port */
#define	ISPR_REG		0x06		/* Interrupt Status Port */
#define	CBSP_REG		0x07		/* Command Busy/Status Port */
#define	HIST_REG		0x08		/* Host Interrupt status */
#define	CCSAR_REG		0x10		/* Command Channel address */
#define	CCCR_REG		0x14		/* Command Channel Control */
#define	SQHR_REG		0x20		/* Status Queue head */
#define	SQTR_REG		0x24		/* Status Queue Tail */
#define	SQER_REG		0x28		/* Status Queue End */
#define	SQSR_REG		0x2c		/* Status Queue Start */

#define	VIPER_STATUS_QUEUE_NUM	64


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
#define	CHS_KVTOP(vaddr) \
		CPUPHYS_TO_IOPHYS(\
		((paddr_t)(hat_getkpfnum((caddr_t)(vaddr)) << (PAGESHIFT)) | \
			    ((paddr_t)(vaddr) & (PAGEOFFSET))))
#else
#define	CHS_KVTOP(vaddr)	(HBA_KVTOP((vaddr), chs_pgshf, \
					chs_pgmsk))
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
#include "chsops.h"
#include "chsdefs.h"
#include "chs_debug.h"

#ifdef	__cplusplus
}
#endif

#endif /* _SYS_DKTP_CHS_CHS_H */
