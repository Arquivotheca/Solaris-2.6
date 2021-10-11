/*
 * Copyright (c) 1995-96 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_ATA_H
#define	_SYS_DKTP_ATA_H

#pragma ident	"@(#)ata.h	1.18	96/08/29 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Controller port address defaults
 */
#define	ATA_BASE0	0x1f0
#define	ATA_BASE1	0x170

/*
 * port offsets from base address ioaddr1
 */
#define	AT_DATA		0x00	/* data register 			*/
#define	AT_ERROR	0x01	/* error register (read)		*/
#define	AT_FEATURE	0x01	/* features (write)			*/
#define	AT_COUNT	0x02    /* sector count 			*/
#define	AT_SECT		0x03	/* sector number 			*/
#define	AT_LCYL		0x04	/* cylinder low byte 			*/
#define	AT_HCYL		0x05	/* cylinder high byte 			*/
#define	AT_DRVHD	0x06    /* drive/head register 			*/
#define	AT_STATUS	0x07	/* status/command register 		*/
#define	AT_CMD		0x07	/* status/command register 		*/

/*
 * port offsets from base address ioaddr2
 */
#define	AT_ALTSTATUS	0x06	/* alternate status (read)		*/
#define	AT_DEVCTL	0x06	/* device control (write)		*/
#define	AT_DRVADDR	0x07 	/* drive address (read)			*/

/*	Device control register						*/
#define	AT_NIEN    	0x02    /* disable interrupts 			*/
#define	AT_SRST		0x04	/* controller reset			*/

/*
 * Status bits from AT_STATUS register
 */
#define	ATS_BSY		0x80    /* controller busy 			*/
#define	ATS_DRDY	0x40    /* drive ready 				*/
#define	ATS_DWF		0x20    /* write fault 				*/
#define	ATS_DSC    	0x10    /* seek operation complete 		*/
#define	ATS_DRQ		0x08	/* data request 			*/
#define	ATS_CORR	0x04    /* ECC correction applied 		*/
#define	ATS_IDX		0x02    /* disk revolution index 		*/
#define	ATS_ERR		0x01    /* error flag 				*/

/*
 * Status bits from AT_ERROR register
 */
#define	ATE_AMNF	0x01    /* address mark not found		*/
#define	ATE_TKONF	0x02    /* track 0 not found			*/
#define	ATE_ABORT	0x04    /* aborted command			*/
#define	ATE_IDNF	0x10    /* ID not found				*/
#define	ATE_MC		0x20    /* Media chane				*/
#define	ATE_UNC		0x40	/* uncorrectable data error		*/
#define	ATE_BBK		0x80	/* bad block detected			*/
/*
 * Additional atapi status bits (redefinitions)
 */
#define	ATE_ILI		0x01    /* Illegal length indication		*/
#define	ATE_EOM		0x02	/* End of media detected		*/
#define	ATE_MCR		0x08	/* Media change requested		*/
#define	ATS_SENSE_KEY	0xf0	/* 4 bit sense key -see ata_sense_table */

#define	ATS_SENSE_KEY_SHIFT 4	/* shift to get to ATS_SENSE_KEY	*/

/*
 * Drive selectors for AT_DRVHD register
 */
#define	ATDH_LBA	0x40	/* addressing in LBA mode not chs 	*/
#define	ATDH_DRIVE0	0xa0    /* or into AT_DRVHD to select drive 0 	*/
#define	ATDH_DRIVE1	0xb0    /* or into AT_DRVHD to select drive 1 	*/

/*
 * Status bits from ATAPI Interrupt reason register (AT_COUNT) register
 */
#define	ATI_COD		0x01    /* Command or Data			*/
#define	ATI_IO		0x02    /* IO direction 			*/

/*
 * ATA commands.
 */
#define	ATC_DIAG	0x90    /* diagnose command 			*/
#define	ATC_RECAL	0x10	/* restore cmd, bottom 4 bits step rate */
#define	ATC_SEEK	0x70    /* seek cmd, bottom 4 bits step rate 	*/
#define	ATC_RDVER	0x40	/* read verify cmd			*/
#define	ATC_RDSEC	0x20    /* read sector cmd			*/
#define	ATC_RDLONG	0x23    /* read long without retry		*/
#define	ATC_WRSEC	0x30    /* write sector cmd			*/
#define	ATC_SETMULT	0xc6	/* set multiple mode			*/
#define	ATC_RDMULT	0xc4	/* read multiple			*/
#define	ATC_WRMULT	0xc5	/* write multiple			*/
#define	ATC_FORMAT	0x50	/* format track command 		*/
#define	ATC_SETPARAM	0x91	/* set parameters command 		*/
#define	ATC_READPARMS	0xec    /* Read Parameters command 		*/
#define	ATC_READDEFECTS	0xa0    /* Read defect list			*/
#define	ATC_SET_FEAT	0xef	/* set features				*/
#define	ATC_IDLE_IMMED	0xe1	/* idle immediate			*/
#define	ATC_STANDBY_IM	0xe0	/* standby immediate			*/
#define	ATC_ACK_MC	0xdb	/* acknowledge media change		*/
#define	ATC_DOOR_LOCK	0xde	/* door lock				*/
#define	ATC_DOOR_UNLOCK	0xdf	/* door unlock				*/
#define	ATC_PI_SRESET	0x08    /* ATAPI soft reset			*/
#define	ATC_PI_ID_DEV	0xa1	/* ATAPI identify device		*/
#define	ATC_PI_PKT	0xa0	/* ATAPI packet command 		*/
				/* conflicts with ATC_READDEFECTS !	*/

/*
 * Low bits for Read/Write commands...
 */
#define	ATCM_ECCRETRY	0x01    /* Enable ECC and RETRY by controller 	*/
				/* enabled if bit is CLEARED!!! 	*/
#define	ATCM_LONGMODE	0x02    /* Use Long Mode (get/send data & ECC) 	*/
				/* enabled if bit is SET!!! 		*/

/*
 * direction bits
 * for ac_direction
 */
#define	AT_NO_DATA	0		/* No data transfer */
#define	AT_OUT		1		/* for writes */
#define	AT_IN		2		/* for reads */

/*
 * status bits for ab_ctl_status
 */
#define	ATA_ONLINE	0
#define	ATA_OFFLINE	1

/*
 * bits for struct atarpbuf, configuration word (atapi only)
 */
#define	ATARP_PKT_SZ	0x3
#define	ATARP_PKT_12B	0x0
#define	ATARP_PKT_16B	0x1

#define	ATARP_DRQ_TYPE	0x60
#define	ATARP_DRQ_INTR	0x20

#define	ATARP_REM_DRV	0x80

#define	ATARP_DEV_TYPE	0x1f00
#define	ATARP_DEV_CDR	0x500

/*
 * bits and options for set features (ATC_SET_FEAT)
 */
#define	SET_TFER_MODE	3
#define	FC_PIO_MODE 	0x8		/* Flow control pio mode */

/*
 * ata device type
 */
#define	ATA_DEV_NONE	0
#define	ATA_DEV_DISK	1
#define	ATA_DEV_12	2 /* atapi 1.2 spec unit */
#define	ATA_DEV_17	3 /* atapi 1.7B spec unit */

/* for looping on registers */
#define	ATA_LOOP_CNT	10000

/* atarp_cap Capabilities */
#define	ATAC_DMA_SUPPORT	0x0100
#define	ATAC_LBA_SUPPORT	0x0200
#define	ATAC_IORDY_DISABLE	0x0400
#define	ATAC_IORDY_SUPPORT	0x0800
#define	ATAC_PIO_RESERVED	0x1000
#define	ATAC_STANDBYTIMER	0x2000

/*
 * ATAPI bits
 */
#define	ATAPI_SIG_HI	0xeb		/* in high cylinder register	*/
#define	ATAPI_SIG_LO	0x14		/* in low cylinder register	*/

#define	ATAPI(X)  ((X)->au_atapi)

#define	ATAPI_MAX_XFER	0xf800 /* 16 bits - 2KB  ie 62KB */
#define	ATA_CD_SCTRSHFT	11
#define	ATA_CD_SECSIZ	2048

struct ata_cmpkt {
	struct	cmpkt	ac_pkt;

	char		ac_cdb;		/* target driver command	*/
	char		ac_scb;		/* controller status aft cmd	*/
	u_short		ac_flags;	/* controller flags		*/

	long		ac_bytes_per_block; /* blk mode factor per xfer	*/
	caddr_t		ac_v_addr;	/* I/O should be done to/from 	*/
	caddr_t		ac_start_v_addr; /* start I/O address		*/
	char		ac_direction;	/* AT_IN - read AT_OUT - write  */

/*	task file registers setting					*/
					/* sec count in ac_pkt		*/
	u_char		ac_devctl;
	u_char		ac_sec;
	u_char		ac_count;
	u_char		ac_lwcyl;
	u_char		ac_hicyl;
	u_char		ac_hd;
	u_char		ac_cmd;

/*	error status							*/
	u_char		ac_error;
	u_char		ac_status;

/*	atapi								*/
	u_char		ac_atapi;	/* set if atapi cmd 		*/
	union scsi_cdb	ac_scsi_pkt;	/* 12 byte scsi packet		*/
};

#define	ATA_MAXDRIVE	8
struct	ata_blk {
	kmutex_t 	ab_mutex;
	void		*ab_lkarg;
	u_short		ab_status_flag;
	u_short		ab_resv;
	/*
	 * Even though we can only have 2 targets,  we need 8 slots
	 * for the generic code
	 */
	struct atarpbuf	*ab_rpbp[ATA_MAXDRIVE];
	struct scsi_inquiry *ab_inqp[ATA_MAXDRIVE];
	u_char		ab_dev_type[ATA_MAXDRIVE];
	dev_info_t	*ab_dip;
/*
 * port addresses associated with ioaddr1
 */
	u_short		ab_data;	/* data register 		*/
	u_short		ab_error;	/* error register (read)	*/
	u_short		ab_feature;	/* features (write)		*/
	u_short		ab_count;	/* sector count 		*/
	u_short		ab_sect;	/* sector number 		*/
	u_short		ab_lcyl;	/* cylinder low byte 		*/
	u_short		ab_hcyl;	/* cylinder high byte 		*/
	u_short		ab_drvhd;	/* drive/head register 		*/
	u_short		ab_status;	/* status/command register 	*/
	u_short		ab_cmd;		/* status/command register 	*/

/*
 * port addresses associated with ioaddr2
 */
	u_short		ab_altstatus;	/* alternate status (read)	*/
	u_short		ab_devctl;	/* device control (write)	*/
	u_short		ab_drvaddr;	/* drive address (read)		*/
	struct ata	*ab_link;	/* linked units			*/
	struct ata_cmpkt *ab_active;	/* outstanding requests		*/
	int		ab_pio_mode[2]; /* the max pio I should attempt */
	int		ab_block_factor[2]; /* hold dev blk factor until */
					/* unit structure is alloc	*/
	u_char		ab_rd_cmd[2];	/* hold read command until	*/
					/* unit structure is alloc	*/
	u_char		ab_wr_cmd[2];	/* hold write command until	*/
					/* unit structure is alloc	*/
	int		ab_max_transfer;
	int		ab_timing_flags;/* flags to tweak ata timing	*/
};


/*	structure of 'Read Parameters' (Identify drive info) command	*/
struct atarpbuf {
/*  					WORD				*/
/* 					OFFSET COMMENT			*/
	ushort  atarp_config;	  /*   0  general configuration bits 	*/
	ushort  atarp_fixcyls;	  /*   1  # of fixed cylinders		*/
	ushort  atarp_remcyls;	  /*   2  # of removable cylinders	*/
	ushort  atarp_heads;	  /*   3  # of heads			*/
	ushort  atarp_trksiz;	  /*   4  # of unformatted bytes/track 	*/
	ushort  atarp_secsiz;	  /*   5  # of unformatted bytes/sector	*/
	ushort  atarp_sectors;    /*   6  # of sectors/track		*/
	ushort  atarp_resv1[3];   /*   7  "Vendor Unique"		*/
	char	atarp_drvser[20]; /*  10  Serial number			*/
	ushort	atarp_buftype;	  /*  20  Buffer type			*/
	ushort	atarp_bufsz;	  /*  21  Buffer size in 512 byte incr  */
	ushort	atarp_ecc;	  /*  22  # of ecc bytes avail on rd/wr */
	char	atarp_fw[8];	  /*  23  Firmware revision		*/
	char	atarp_model[40];  /*  27  Model #			*/
	ushort	atarp_mult1;	  /*  47  Multiple command flags	*/
	ushort	atarp_dwcap;	  /*  48  Doubleword capabilities	*/
	ushort	atarp_cap;	  /*  49  Capabilities			*/
	ushort	atarp_resv2;	  /*  50  Reserved			*/
	ushort	atarp_piomode;	  /*  51  PIO timing mode		*/
	ushort	atarp_dmamode;	  /*  52  DMA timing mode		*/
	ushort	atarp_validinfo;  /*  53  bit0: wds 54-58, bit1: 64-70	*/
	ushort	atarp_curcyls;	  /*  54  # of current cylinders	*/
	ushort	atarp_curheads;	  /*  55  # of current heads		*/
	ushort	atarp_cursectrk;  /*  56  # of current sectors/track	*/
	ushort	atarp_cursccp[2]; /*  57  current sectors capacity	*/
	ushort	atarp_mult2;	  /*  59  multiple sectors info		*/
	ushort	atarp_addrsec[2]; /*  60  LBA only: no of addr secs	*/
	ushort	atarp_sworddma;	  /*  62  single word dma modes		*/
	ushort	atarp_dworddma;	  /*  63  double word dma modes		*/
	ushort	atarp_advpiomode; /*  64  advanced PIO modes supported	*/
	ushort	atarp_minmwdma;   /*  65  min multi-word dma cycle info	*/
	ushort	atarp_recmwdma;   /*  66  rec multi-word dma cycle info	*/
	ushort	atarp_minpio;	  /*  67  min PIO cycle info		*/
	ushort	atarp_minpioflow; /*  68  min PIO cycle info w/flow ctl */
};

struct	ata_unit {
	u_char		au_targ;
	u_char		au_drive_bits;
	u_char		au_ctl_bits;
	u_char		au_atapi;
	u_char		au_17b;
	u_char		au_rd_cmd;
	u_char		au_wr_cmd;
	u_short		au_acyl;
	u_short		au_bioscyl;	/* BIOS: number of cylinders */
	u_short		au_bioshd;	/* BIOS: number of heads */
	u_short		au_biossec;	/* BIOS: number of sectors */
	u_short		au_phhd;	/* number of physical heads */
	u_short		au_phsec;	/* number of physical sectors */
	short		au_block_factor;
	short		au_bytes_per_block;
	long		au_blksz;
	struct atarpbuf *au_rpbuf;
	struct ata_cmpkt *au_head;
	struct ata_cmpkt *au_last;
	struct scsi_capacity au_capacity;
};

struct ata {
	struct ata_blk	*a_blkp;
	struct ata_unit	*a_unitp;
	struct ctl_obj	*a_ctlobjp;
	struct ata	*a_forw;	/* linked list for all ata's 	*/
	struct ata	*a_back;
};

#ifdef	__cplusplus
}
#endif

#endif /* _SYS_DKTP_ATA_H */
