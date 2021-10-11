/*
 * Copyright (c) 1992-1994, Sun Microsystems, Inc.
 */

#ifndef	_SYS_DKTP_HBA_H
#define	_SYS_DKTP_HBA_H

#pragma ident	"@(#)hba.h	1.6	94/02/04 SMI"

#include <sys/scsi/impl/pkt_wrapper.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define	TRUE		1
#define	FALSE		0
#define	UNDEFINED 	-1

#define	SEC_INUSEC	1000000

#define	PRF		prom_printf


#define	HBA_INTPROP(devi, pname, pval, plen) \
	(ddi_prop_op(DDI_DEV_T_NONE, (devi), PROP_LEN_AND_VAL_BUF, \
		DDI_PROP_DONTPASS, (pname), (caddr_t)(pval), (plen)))

#define	HBA_KVTOP(vaddr, shf, msk) \
		((paddr_t)(hat_getkpfnum((caddr_t)(vaddr)) << (shf)) | \
			    ((paddr_t)(vaddr) & (msk)))

#define	HBA_MAX_ATT_DEVICES 56	    /* Max devices attached to one dev	*/
#define	HBA_MAX_CDB_LEN  CDB_GROUP5 /* Max size of a SCSI CDB		*/

#define	HBA_CFLAG_FREE	0x0001	    /* packet is on free list 		*/
#define	HBA_CFLAG_ERROR	0x0002	    /* packet is retry for error	*/

#define	SDEV2ADDR(devp) (&((devp)->sd_address))
#define	SDEV2TRAN(devp) ((devp)->sd_address.a_hba_tran)
#define	PKT2TRAN(pktp)	((pktp)->pkt_address.a_hba_tran)
#define	ADDR2TRAN(ap)	((ap)->a_hba_tran)

#define	HBA_SETGEOM(hd, sec) (((hd) << 16) | (sec))
#define	HBA_KMFLAG(callback) (((callback) == DDI_DMA_SLEEP) \
	? KM_SLEEP: KM_NOSLEEP)

#ifdef  _KERNEL
#ifdef  __STDC__
extern opaque_t scsi_create_cbthread(ddi_iblock_cookie_t lkarg, int sleep);
extern void	scsi_destroy_cbthread(opaque_t cbhdl);
extern void 	scsi_run_cbthread(opaque_t cbhdl, struct scsi_cmd *cmd);
extern void	scsi_iopb_fast_free(caddr_t *base, caddr_t p);
extern int 	scsi_iopb_fast_zalloc(caddr_t *listp, dev_info_t *dip,
			ddi_dma_lim_t *limp, u_int len, caddr_t *iopbp);
extern int 	scsi_iopb_fast_alloc(caddr_t *listp, dev_info_t *dip,
			ddi_dma_lim_t *limp, u_int len, caddr_t *iopbp);
extern void 	scsi_htos_3byte(unchar *, paddr_t);
extern void 	scsi_htos_long(unchar *, ulong);
extern void 	scsi_htos_short(unchar *, ushort);
extern ulong 	scsi_stoh_3byte(unchar *);
extern ulong 	scsi_stoh_long(ulong);
extern ushort 	scsi_stoh_short(ushort);
extern struct scsi_pkt *scsi_impl_dmaget(struct scsi_pkt *pkt,
	opaque_t dmatoken, int (*callback)(), caddr_t callback_arg,
	ddi_dma_lim_t *dmalimp);

#else   /* __STDC__ */

extern opaque_t scsi_create_cbthread();
extern void	scsi_destroy_cbthread();
extern void 	scsi_run_cbthread();
extern void 	scsi_iopb_fast_free();
extern int 	scsi_iopb_fast_zalloc();
extern int 	scsi_iopb_fast_alloc();
extern void 	scsi_htos_3byte();
extern void 	scsi_htos_long();
extern void 	scsi_htos_short();
extern ulong 	scsi_stoh_3byte();
extern ulong 	scsi_stoh_long();
extern ushort 	scsi_stoh_short();
extern struct scsi_pkt *scsi_impl_dmaget();

#endif  /* __STDC__ */

#define	SCSI_CB_DESTROY		0x0001

struct	scsi_cbthread {
	int	cb_flag;		/* misc flags			*/
	kmutex_t cb_mutex;		/* mutex on local struct 	*/
	kcondvar_t cb_cv;		/* condition variable		*/
	kthread_t *cb_thread;
	struct	scsi_cmd *cb_head;	/* queue head pointer		*/
	struct	scsi_cmd *cb_tail;	/* queue tail pointer		*/
};


extern u_char	inb(int port);
extern u_short	inw(int port);
extern u_long	inl(int port);
extern void	repinsb(int port, u_char *addr, int count);
extern void	repinsw(int port, u_short *addr, int count);
extern void	repinsd(int port, u_long *addr, int count);

extern void	outb(int port, u_char value);
extern void	outw(int port, u_short value);
extern void	outl(int port, u_long value);
extern void	repoutsb(int port, u_char *addr, int count);
extern void	repoutsw(int port, u_short *addr, int count);
extern void	repoutsd(int port, u_long *addr, int count);

#endif	/* _KERNEL */
#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_DKTP_HBA_H */
