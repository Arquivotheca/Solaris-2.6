/*
 * Copyright (c) 1995 Sun Microsystems, Inc.
 * All Rights Reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifndef _SYS_DKTP_ADPCMD_H
#define _SYS_DKTP_ADPCMD_H

#pragma ident	"@(#)adpcmd.h	1.4	95/05/17 SMI"


#ifdef	__cplusplus
extern "C" {
#endif

#define	TRUE		1
#define	FALSE		0
#define UNDEFINED	-1

#define	SEC_INUSEC	1000000

#define	ADP_INTPROP(devi, pname, pval, plen) \
	(ddi_prop_op(DDI_DEV_T_NONE, (devi), PROP_LEN_AND_VAL_BUF, \
		DDI_PROP_DONTPASS, (pname), (caddr_t)(pval), (plen)))

#define	ADP_SETGEOM(hd, sec) (((hd) << 16) | (sec))

#define	HBA_KVTOP(vaddr, shf, msk) \
		((paddr_t)(hat_getkpfnum((caddr_t)(vaddr)) << (shf)) | \
			    ((paddr_t)(vaddr) & (msk)))

/*VARARGS1*/
void prom_printf(char *fmt, ...);

#define	PRF	prom_printf

/*
 * HBA interface macros
 */
#define	SDEV2TRAN(sd)		((sd)->sd_address.a_hba_tran)
#define	SDEV2ADDR(sd)		(&((sd)->sd_address))
#define	PKT2TRAN(pkt)		((pkt)->pkt_address.a_hba_tran)
#define	ADDR2TRAN(ap)		((ap)->a_hba_tran)


/*
 * Macros to flip from scsi_pkt to adp_scsi_cmd (hba pkt private data)
 */
#define	PKT2CMD(pkt)	((struct adp_scsi_cmd *)(pkt)->pkt_ha_private)
#define	CMD2PKT(cmd)	((struct scsi_pkt *)(cmd)->cmd_pkt)

struct 	adp_scsi_cmd {
	struct scsi_pkt	*cmd_pkt;
	ulong		cmd_flags;	/* flags from scsi_init_pkt */
	u_int		cmd_cflags;	/* private hba CFLAG flags */
	struct adp_scsi_cmd *cmd_linkp;	/* link ptr for callbacks */
	ddi_dma_handle_t cmd_dmahandle;	/* dma handle 			*/
	ddi_dma_win_t	cmd_dmawin;	/* dma window		*/
	ddi_dma_seg_t	cmd_dmaseg;
	void		*cmd_private;
	u_char		cmd_cdblen;	/* length of cdb 		*/
	u_char		cmd_scblen;	/* length of scb 		*/
	u_char		cmd_privlen;	/* length of target private 	*/
	u_char		cmd_rqslen;	/* len of requested rqsense	*/
	long		cmd_totxfer;	/* total transfer for cmd	*/
	struct sequencer_ctrl_block *cmd_scbp;
};


#define	SC_XPKTP(X)	((struct target_private *)((X)->pkt_private))
struct	target_private {
	struct scsi_pkt *x_fltpktp;	/* link to autosense packet	*/
	struct buf	*x_bp;		/* request buffer		*/
	union {
		struct buf	*xx_rqsbp; /* request sense buffer	*/
		struct uscsi_cmd *xx_scmdp; /* user scsi command 	*/
	} targ;

	daddr_t		x_srtsec;	/* starting sector		*/
	int		x_seccnt;	/* sector count			*/
	int		x_byteleft;	/* bytes left to do		*/
	int		x_bytexfer;	/* bytes xfered for this ops	*/
	int		x_tot_bytexfer;	/* total bytes xfered per cmd	*/

	u_short		x_cdblen;	/* cdb length			*/
	short		x_retry;	/* retry count			*/
	int		x_flags;	/* flags			*/

	opaque_t	x_sdevp;	/* backward ptr target unit	*/
	void		(*x_callback)(); /* target drv internal cb func	*/
};

#define PKT_PRIV_LEN		sizeof(struct target_private)

/*
 * cmd_cflags definitions
 */

#define	CFLAG_CMDDISC	0x0001		/* cmd currently disconnected */
#define	CFLAG_WATCH	0x0002		/* watchdog time for this command */
#define	CFLAG_FINISHED	0x0004		/* command completed */
#define	CFLAG_CHKSEG	0x0008		/* check cmd_data within seg */
#define	CFLAG_COMPLETED	0x0010		/* completion routine called */
#define	CFLAG_PREPARED	0x0020		/* pkt has been init'ed */
#define	CFLAG_IN_TRANSPORT 0x0040	/* in use by host adapter driver */
#define	CFLAG_TRANFLAG	0x00ff		/* covers transport part of flags */
#define	CFLAG_CMDPROXY	   0x000100	/* cmd is a 'proxy' command */
#define	CFLAG_CMDARQ	   0x000200	/* cmd is a 'rqsense' command */
#define	CFLAG_DMAVALID	   0x000400	/* dma mapping valid */
#define	CFLAG_DMASEND	   0x000800	/* data is going 'out' */
#define	CFLAG_CMDIOPB	   0x001000	/* this is an 'iopb' packet */
#define	CFLAG_CDBEXTERN	   0x002000	/* cdb kmem_alloc'd */
#define	CFLAG_SCBEXTERN	   0x004000	/* scb kmem_alloc'd */
#define	CFLAG_FREE	   0x008000	/* packet is on free list */
#define	CFLAG_EXTCMDS_ALLOC	0x10000	/* pkt has EXTCMDS_SIZE and */
					/* been fast alloc'ed */
#define	CFLAG_PRIVEXTERN   0x020000	/* target private kmem_alloc'd */
#define	CFLAG_DMA_PARTIAL  0x040000	/* partial xfer OK */


#ifdef	__cplusplus
}
#endif

#endif  /* _SYS_DKTP_ADPCMD_H */
