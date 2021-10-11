/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_DSA_H
#define _SYS_DKTP_DSA_H

#pragma ident	"@(#)dsa.h	1.1	93/10/25 SMI"


#ifdef	__cplusplus
extern "C" {
#endif

#define DSA_KVTOP(vaddr) (HBA_KVTOP((vaddr), dsa_pgshf, dsa_pgmsk))

/*
 * The Dell Scsi Array Host Adapter Command Control Block (CCB)
 */

#define DSA_MAX_HANDLES		256 	/* outstanding cmds per board +1*/
#define DSA_DMAMAX		32 	/* SG segs			*/
#define DSA_MAXDRIVE		28 	/* drives per card 		*/
#define DSA_NO_MUTEX		0
#define DSA_NEED_MUTEX		1

#pragma pack(1)

/* a Scatter/Gather DMA Segment Descriptor */
struct dsa_dma_seg {
	uint  data_ptr;     	/* segment address 			*/
	uint  data_len;     	/* segment length 			*/
};

struct dsa_cmpkt {
	struct	cmpkt	dc_pkt;

	char		dc_cdb;		/* target driver command	*/
	char		dc_scb;		/* controller status aft cmd	*/
	u_short		dc_flags;	/* controller flags		*/

	caddr_t		dc_start_v_addr;/* start I/O address		*/

/*	task file registers setting					*/

	unchar	dc_command;		/* command to controller	*/
	unchar	dc_drive_unit;		/* unit 			*/
	unchar	dc_bytes;		/* bytes 			*/
	unchar	dc_handle;		/* identifier for a command 	*/
	uint	dc_block;		/* sector 			*/
	uint	dc_paddr;		/* physical address 		*/
	char 	*dc_vaddr;		/* I/O should be done to/from 	*/
	ddi_dma_handle_t	dc_dma_handle;
	ddi_dma_win_t		dc_dmawin;
	ddi_dma_seg_t		dc_dmaseg;

	struct  dsa_dma_seg dc_sg_list[DSA_DMAMAX]; 	/* SG segs	*/

/*	error status							*/
	u_char		dc_error;
	u_char		dc_status;

/*	link pointer for callback of queued completing packets		*/
	struct dsa_cmpkt *dc_linkp;
};

struct  dsa_blk {
	kmutex_t db_mutex;
	void	*db_lkarg;
	dev_info_t *db_dip;

	ushort	db_ioaddr;
        unchar	db_flag;
	unchar	db_intr;

	struct dsarpbuf	*db_rpbp[DSA_MAXDRIVE];
	struct scsi_inquiry *db_inqp[DSA_MAXDRIVE];

	unchar	db_intmask;
	unchar	db_dmachan;
	unchar  db_max_sglen;		/* max length of scatter gather list */
	unchar	db_queue_len;		/* max queue depth per drive	*/

	unchar	db_numdev;
	unchar  db_refcount;
	ushort  db_pkts_out;

	int	db_mbx_allocmem;
	int	db_max_handles;
	int	db_active_handles;

	int		db_tos_handle;
	struct dsa_cmpkt  *db_pktp[DSA_MAX_HANDLES];
};

/*	structure for 'Get Capacity [NTV_GETCUNDATA]' command 			*/
struct dsarpbuf
	{
	uint	dsarp_cap;		/* total sectors 		*/
	unchar	dsarp_heads;		/* virtual heads		*/
	unchar	dsarp_sectors;		/* sectors			*/
	ushort	dsarp_cylinders;	/* cylinders			*/
	ushort	dsarp_secs_per_track;	/* sectors per track		*/
	unchar	dsarp_phys_heads;	/* physical heads		*/
	unchar	dsarp_status;
};

struct dsa_unit {
	ddi_dma_lim_t	du_lim;
	unchar		du_drive_unit;		/* unit 		*/
	uint		du_cap;			/* total sectors 		*/
	unchar		du_heads;		/* virtual heads		*/
	unchar		du_sectors;		/* sectors			*/
	ushort		du_cylinders;		/* cylinders			*/
	ushort		du_secs_per_track;	/* sectors per track		*/
	unchar		du_phys_heads;		/* physical heads		*/
	struct 		dsarpbuf *du_rpbuf;
	struct 		dsa_cmpkt *du_head; 
	struct 		dsa_cmpkt *du_last; 
};

#define CK2DSAUNITP(C) (struct dsa_unit *)(((struct dsa *)(C))->d_unitp)
#define PKT2DSAUNITP(pktp) (CK2DSAUNITP((pktp)->pkt_address.a_cookie))
#define ADDR2DSAUNITP(ap) (CK2DSAUNITP((ap)->a_cookie))

#define CK2DSABLKP(C) (struct dsa_blk *)(((struct dsa *)(C))->d_blkp)
#define PKT2DSABLKP(pktp) (CK2DSABLKP((pktp)->pkt_address.a_cookie))
#define ADDR2DSABLKP(ap) (CK2DSABLKP((ap)->a_cookie))

#define DSA_BLKP(X) (((struct dsa *)(X))->d_blkp)
struct dsa {
	struct dsa_blk	*d_blkp;
	struct dsa_unit	*d_unitp;
	struct ctl_obj	*d_ctlobjp;
	struct dsa	*d_back;
};

typedef struct
{
    unchar  structVer;		/*				*/
    unchar  valid;		/* true if data valid		*/
    ushort  structSize;		/*				*/
    uint    totalSectors;	/* total num of avail sectors	*/
    ushort  heads;		/* logical heads		*/
    ushort  sectors;		/* logical sectors per track	*/
    ushort  cylinders;		/* logical cylinders available	*/
    ushort  pheads;		/* physical heads		*/
    ushort  psectors;		/* physical sectors per track	*/
    ushort  pcylinders;		/* physical cylinders		*/
    unchar  type;		/* mirrored, guarded, etc	*/
    unchar  status;		/* status of the cun		*/
    ushort  blockSize;		/* unit of striping in sectors	*/
    uint    dataBM;		/* data drive bitmap		*/
    uint    parityBM;		/* parity drive bitmap		*/
    char    volumeLabel[40];	/* user volume label		*/
    uint    numEventsLogged;	/* number events logged 	*/
    char    firmwareRev[4];	/* firmware revision string	*/
    ushort  revMajor;		/* major firmware revision	*/
    ushort  revMinor;		/* minor firmware revision	*/
    unchar  EmulMode;		/* AHA or NONE			*/
    unchar  autoRebuild; 	/* max delay for writeback	*/
    unchar  WriteStrategy;	/* write strategy		*/
    unchar  CacheStrategy;	/* caching strategy		*/
    unchar  PrefetchStrategy;	/* prefetch strategy		*/
    unchar  MaxReadAhead;	/* number of sectors readahead	*/
    unchar  historySize; 	/* depth of prefetch hostory	*/
    unchar  writeDelay;		/* max delay for writeback	*/
} ntvIdentify;

struct dsa_mboxes
{
    unchar	m_box0;
    unchar	m_box1;
    unchar	m_box2;
    unchar	m_box3;
    unchar	m_box4;
    unchar	m_box5;
    unchar	m_box6;
    unchar	m_box7;
    unchar	m_box8;
    unchar	m_box9;
    unchar	m_box10;
    unchar	m_box11;
    unchar	m_box12;
    unchar	m_box13;
    unchar	m_box14;
    unchar	m_box15;
};


#define SECSIZE 	512	/* default sector size */
#define SECSHFT 	9
#define SECMASK 	(SECSIZE-1)

#define IN_SEM_TO	10      /* poll for input semaphore timeout     */
#define EXTEND_TO	100000  /* for extende command complete timeout */
#define MAXXFER		255     /* maximum number of sectors            */

/*----------------- BMIC Doorbell bit position assignments ------------------*/

#define NTV_FOREIGN1_DOORBELL	0x01
#define NTV_FOREIGN2_DOORBELL	0x02
#define NTV_FOREIGN3_DOORBELL	0x04
#define NTV_SRESET_DOORBELL	0x08
#define NTV_LOGICAL_DOORBELL	0x10
#define NTV_PHYSICAL_DOORBELL	0x20
#define NTV_EXTENDED_DOORBELL	0x40
#define NTV_HRESET_DOORBELL	0x80

/*-----------------  Logical command enumerations ---------------------------*/

#define NTV_RECAL	0x00	/* recalibrate command		*/
#define NTV_READ	0x01	/* read sector(s)		*/
#define NTV_WRITE	0x02	/* write sector(s)		*/
#define NTV_VERIFY	0x03	/* verify sector(s)		*/
#define NTV_SEEK	0x04	/* seek 			*/
#define NTV_GUARDED	0x05	/* verify guard on sector(s)	*/
#define NTV_READAHEAD	0x06	/* read sector(s) but no xfer	*/
#define NTV_READBUF	0x07	/* native readbuffer diag	*/
#define NTV_WRITEBUF	0x08	/* native writebuffer diag	*/
#define NTV_WRITEVER	0x09	/* write and verify sector(s)	*/
#define NTV_IDENTIFY	0x0A	/* logical unit info command	*/
#define NTV_READCRC	0x0B	/* native readbuffer w/ crc	*/
#define NTV_WRITECRC	0x0C	/* native writebuffer w/ crc	*/
#define NTV_READLOG	0x0D	/* read first/next errlog entry */
#define NTV_READSCATTER 0x0E	/* read w/ scatter list 	*/
#define NTV_WRITEGATHER 0x0F	/* write w/ gather list 	*/
#define NTV_INITLOG	0x10	/* initialize error log 	*/
#define NTV_FLUSHLOG	0x11	/* flush partial TRACE or NOP	*/
#define NTV_REMAP	0x12	/* native remap block		*/
#define NTV_SYNCWRITE	0x13	/* syncronous write		*/
#define NTV_WVERFGATHER	0x14	/* syncronous write		*/
#define NTV_SYNCWRITEV	0x15	/* syncronous write		*/
#define NTV_SWRITEGATH	0x16	/* syncronous write		*/
#define NTV_SWVERFGATH	0x17	/* syncronous write		*/
#define NTV_BREADSCATT	0x18	/* scatter gather byte read 	*/
#define NTV_BWRITEGATH	0x19	/* syncronous write		*/
#define NTV_BWVERFGATH	0x1a	/* syncronous write		*/
#define NTV_BSWRITEGATH	0x1b	/* syncronous write		*/
#define NTV_BSWVERFGATH	0x1c	/* syncronous write		*/
#define NTV_SYNCHRONIZE	0x1d	/* syncronous write		*/
#define NTV_READPUNLOG	0x1e	/* none greater than this value */
#define NTV_INITPUNLOG	0x1f	/* none greater than this value */
#define NTV_READCTLRLOG	0x20	/* none greater than this value */
#define NTV_INITCTLRLOG	0x21	/* none greater than this value */
#define NTV_CONVERTDEV	0x22	/* none greater than this value */
#define NTV_QUIESCEPUN	0x23	/* none greater than this value */
#define NTV_SCANDEVICES	0x24	/* none greater than this value */
#define NTV_MAXCOMMAND	0x25	/* none greater than this value */
#define NTV_NOCOMMAND	0xff	/* no command pending		*/

/*-------------------- Extended command enumerations -------------------------*/

#define NTV_DIAG	0x01	/* perform ctlr diagnostics	*/
#define NTV_GETVERSION	0x02	/* get RAD version numbers	*/
#define NTV_GETNTVSIZE	0x03	/* get max logical cmd handle	*/
#define NTV_GETPHYSCFG	0x04	/* get physical disk config	*/
#define NTV_SETPARM	0x05	/* set parms of logical disk	*/
#define NTV_GETNUMCUNS	0x06	/* get number of composit disks */
#define NTV_GETCUNDATA	0x07	/* get capacity of logical disk */
#define NTV_SYNC	0x08	/* wait until RAD has finished	*/
#define NTV_PUPSTAT	0x09	/* status of power up		*/
#define NTV_DORESTORE	0x0a	/* begin restore process	*/
#define NTV_PROGRESS	0x0b	/* give restore progress	*/
#define NTV_DIAGREAD	0x0c	/* manf IDE port read		*/
#define NTV_DIAGWRITE	0x0d	/* manf IDE port write		*/
#define NTV_DATETIME	0x0e	/* set date and time		*/
#define NTV_GETHWCFG	0x0f	/* get hardware configuration	*/
#define NTV_PINGINTRVL	0x10	/* set ping interval for watch	*/
#define NTV_DIAGINTCNT	0x11	/* manf interrupt count 	*/
#define NTV_WRITESTRAT	0x12	/* native mode write strategy	*/
#define NTV_TRACECNTRL	0x13	/* I/O trace control		*/
#define NTV_GETCACHEHIT 0x14	/* get cache hit rate		*/
#define NTV_RCACHE	0x15	/* reset cache statistics	*/

/*-----------------------Physical  Command defines --------------------------*/

#define AT_RECAL	0x10	/* recalibrate command			*/
#define AT_READ 	0x20	/* read sector				*/
#define AT_READL	0x22	/* read sector long			*/
#define AT_READNR	0x21	/* read sector no retries		*/
#define AT_READLNR	0x23	/* read sector long no retries		*/
#define AT_WRITE	0x30	/* write sector 			*/
#define AT_WRITEL	0x32	/* write sector long			*/
#define AT_WRITENR	0x31	/* write sector no retries		*/
#define AT_WRITELNR	0x33	/* write sector long no retries 	*/
#define AT_VERIFY	0x40	/* read verify				*/
#define AT_VERIFYNR	0x41	/* read verify no retries		*/
#define AT_FORMAT	0x50	/* format track 			*/
#define AT_SEEK 	0x70	/* seek 				*/
#define AT_DIAG 	0x90	/* perform diagnostics			*/
#define AT_INIT 	0x91	/* initialize drive parameters		*/
#define AT_READBUF	0xE4	/* read sector buffer			*/
#define AT_WRITEBUF	0xE8	/* write sector buffer			*/
#define AT_IDENTIFY	0xEC	/* identify drive			*/
#define AT_SETBUF	0xEF	/* set buffer mode			*/
#define AT_READM	0xC4	/* read multiple			*/
#define AT_WRITEM	0xC5	/* write multiple			*/
#define AT_SETMULT	0xC6	/* set multiple mode			*/
#define MAXTOR_DOWNLOAD 0xFF	/* download microcode to a Maxtor drive */

/*--------------------- Bits in the physical status register -----------*/

#define AT_STATUS_BUSY	0x80	/* drive is busy			*/
#define AT_STATUS_DRDY	0x40	/* drive is ready			*/
#define AT_STATUS_DWF	0x20	/* write fault				*/
#define AT_STATUS_DSC	0x10	/* seek complete			*/
#define AT_STATUS_DRQ	0x08	/* data request 			*/
#define AT_STATUS_CORR	0x04	/* correctable error			*/
#define AT_STATUS_IDX	0x02	/* index pulse				*/
#define AT_STATUS_ERR	0x01	/* uncorrectable error			*/

/*--------------------- Bits in the physical error register ------------*/

#define AT_ERROR_BBK	0x80	/* bad block found			*/
#define AT_ERROR_UNC	0x40	/* uncorrectable error			*/
#define AT_ERROR_IDNF	0x10	/* sector ID not found			*/
#define AT_ERROR_TO	0x08	/* time out				*/
#define AT_ERROR_ABRT	0x04	/* command aborted			*/
#define AT_ERROR_TK0	0x02	/* track 0 not found			*/
#define AT_ERROR_AMNF	0x01	/* address mark not found		*/

/*----------------------- bits in the logical status register ----------*/

#define DSA_BADBLOCK	0x80	/* bad block found			*/
#define DSA_UNCORECT	0x40	/* uncorrectable fault			*/
#define DSA_WRITEFLT	0x20	/* write fault				*/
#define DSA_IDNFOUND	0x10	/* sector id not found			*/
#define DSA_CORRECT		0x08	/* correctable fault			*/
#define DSA_ABORT		0x04	/* received abort from drive		*/
#define DSA_TRACK0NF	0x02	/* track 0 not found			*/
#define DSA_LTIMEOUT	0x01	/* logical drive timed out somehow	*/
#define DSA_OK		0x00	/* no error				*/

/*----------------------- power up status ------------------------------*/

#define PUP_DEAD	0	/* controller died		 	*/
#define PUP_OK		1	/* normal			 	*/
#define PUP_NOTCONFIG	2	/* no configuration (virgin)	 	*/
#define PUP_BADCONFIG	3	/* bad drive configuration	 	*/
#define PUP_RECOVER	4	/* new drive - recovery possible 	*/
#define PUP_DF_CORR	5	/* drive failed - correctable	 	*/
#define PUP_DF_UNCORR	6	/* drive failed - uncorrectable  	*/
#define PUP_NODRIVES	7	/* no drives attached		 	*/
#define PUP_DRIVESADDED 8	/* more drives than expected	 	*/
#define PUP_MAINTAIN	9	/* maintain mode		 	*/
#define PUP_MANFMODE	10	/* manufacturing mode		 	*/
#define PUP_NEW 	11	/* new - needs remap generated	 	*/
#define PUP_NEWR	12	/* same as PUP_NEW with rebuild 	*/
#define PUP_NEWC	13	/* same as PUP_NEW but correctble	*/
#define PUP_NONE	14	/* no drive configuration		*/

/* flags in dc_error							*/
#define DSA_TRANS_ERROR	1

#ifdef	__cplusplus
}
#endif

#endif  /* _SYS_DKTP_DSA_H */
