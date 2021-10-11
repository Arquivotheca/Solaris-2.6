/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#ifndef	_SYS_DKTP_NCRS_NCRDEFS_H
#define	_SYS_DKTP_NCRS_NCRDEFS_H

#pragma	ident	"@(#)ncrdefs.h	1.10	95/12/12 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * busops routines
 */

static	void	ncr_childprop(dev_info_t *pdip, dev_info_t *cdip);

/*
 * devops routines
 */
int		ncr_identify(dev_info_t *devi);
int		ncr_probe(dev_info_t *devi);
int		ncr_attach(dev_info_t *devi, ddi_attach_cmd_t cmd);
int		ncr_detach(dev_info_t *devi, ddi_detach_cmd_t cmd);
static	bool_t	ncr_propinit(dev_info_t *devi, ncr_t *ncrp);
static	bool_t	ncr_prop_default(dev_info_t *dip, caddr_t propname,
			caddr_t propdefault);

/*
 * init routines
 */
void		ncr_saverestore(ncr_t *ncrp, nrs_t *nrsp, unchar *regbufp,
			int nregs, bool_t savethem);
static	void	ncr_table_init(ncr_t *ncrp, npt_t *nptp, int target, int lun);
static	bool_t	ncr_target_init(ncr_t *ncrp);
bool_t		ncr_hba_init(ncr_t *ncrp);
void		ncr_hba_uninit(ncr_t *ncrp);



/*
 * Interrupt routines
 */
u_int		ncr_intr(caddr_t arg);
static	void	ncr_process_intr(ncr_t *ncrp, unchar istat);
static	ulong	ncr_decide(ncr_t *ncrp, ulong action);
static	ulong	ncr_ccb_decide(ncr_t *ncrp, npt_t *nptp, ulong action);
bool_t		ncr_wait_intr(ncr_t *ncrp);
static	int	ncr_setup_npt(ncr_t *ncrp, npt_t *nptp, nccb_t *nccbp);
static	void	ncr_start_next(ncr_t *ncrp);
static	void	ncr_wait_for_reselect(ncr_t *ncrp, ulong action);
static	void	ncr_restart_current(ncr_t *ncrp, ulong action);
static	void	ncr_restart_hba(ncr_t *ncrp, ulong action);
void		ncr_queue_target(ncr_t *ncrp, npt_t *nptp);
static	npt_t	*ncr_get_target(ncr_t *ncrp);
static	ulong	ncr_check_intcode(ncr_t *ncrp, npt_t *nptp, ulong action);
ulong		ncr_parity_check(unchar phase);

/*
 * External SCSA Interface
 */

int	ncr_tran_tgt_init(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);
int	ncr_tran_tgt_probe(struct scsi_device *, int (*)());
void	ncr_tran_tgt_free(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);
struct scsi_pkt *ncr_tran_init_pkt(struct scsi_address *,
			struct scsi_pkt *, struct buf *, int, int, int, int,
			int (*)(), caddr_t);
void	ncr_tran_destroy_pkt(struct scsi_address *, struct scsi_pkt *);

int	ncr_transport(struct scsi_address *, struct scsi_pkt *);
int	ncr_reset(struct scsi_address *, int);
int	ncr_abort(struct scsi_address *, struct scsi_pkt *);
int	ncr_getcap(struct scsi_address *, char *, int);
int	ncr_setcap(struct scsi_address *, char *, int, int);
int	ncr_capchk(char *, int, int *);

struct scsi_pkt *ncr_pktalloc(struct scsi_address *, int, int, int,
			int (*)(), caddr_t);
void	ncr_pktfree(struct scsi_pkt *);
static	struct scsi_pkt *ncr_dmaget(struct scsi_pkt *, opaque_t, int (*)()
						     , caddr_t);
void	ncr_dmafree(struct scsi_address *, struct scsi_pkt *);
void	ncr_sync_pkt(struct scsi_address *, struct scsi_pkt *);

/*
 * queue manipulation routines
 */
void		 ncr_addfq(ncr_t *ncrp, npt_t *nptp);
void		 ncr_addbq(ncr_t *ncrp, npt_t *nptp);
npt_t		*ncr_rmq(ncr_t *ncrp);
void		 ncr_delq(ncr_t *ncrp, npt_t *nptp);
void		 ncr_doneq_add(ncr_t *ncrp, nccb_t *nccbp);
nccb_t		*ncr_doneq_rm(ncr_t *ncrp);
void		 ncr_waitq_add(npt_t *nptp, nccb_t *nccbp);
nccb_t		*ncr_waitq_rm(npt_t *nptp);
void		 ncr_waitq_delete(npt_t *nptp, nccb_t *nccbp);

/*
 * DMA Scatter/Gather list routines
 */
void		ncr_sg_setup(ncr_t *ncrp, npt_t *nptp, nccb_t *nccbp);
void		ncr_sg_update(ncr_t *ncrp, npt_t *nptp, unchar index,
			ulong remain);
ulong		ncr_sg_residual(ncr_t *ncrp, npt_t *nptp);

/*
 * Synchronous I/O routines
 */
void		ncr_syncio_state(ncr_t *ncrp, npt_t *nptp, unchar state,
			unchar sxfer, unchar sscf);
void		ncr_syncio_reset(ncr_t *ncrp, npt_t *nptp);
void		ncr_syncio_msg_init(ncr_t *ncrp, npt_t *nptp);
static bool_t	ncr_syncio_enable(ncr_t *ncrp, npt_t *nptp);
static bool_t	ncr_syncio_respond(ncr_t *ncrp, npt_t *nptp);
ulong		ncr_syncio_decide(ncr_t *ncrp, npt_t *nptp, ulong action);

/*
 * nccb routines
 */
void		ncr_queue_ccb(ncr_t *ncrp, npt_t *nptp, nccb_t *nccbp
					 , unchar rqst_type);
bool_t		ncr_send_dev_reset(struct scsi_address *ap, ncr_t *ncrp
							  , npt_t *nptp);
bool_t		ncr_abort_ccb(struct scsi_address *ap, ncr_t *ncrp
						     , npt_t *nptp);
void		ncr_chkstatus(ncr_t *ncrp, npt_t *nptp, nccb_t *nccbp);
void		ncr_pollret(ncr_t *ncrp, nccb_t *poll_nccbp);
void		ncr_flush_lun(ncr_t *ncrp, npt_t *nptp, bool_t flush_all
					 , u_char pkt_reason, u_long pkt_state
					 , u_long pkt_statistics);
void		ncr_flush_target(ncr_t *ncrp, ushort target, bool_t flush_all
					    , u_char pkt_reason
					    , u_long pkt_state
					    , u_long pkt_statistics);
void		ncr_flush_hba(ncr_t *ncrp, bool_t flush_all, u_char pkt_reason
					 , u_long pkt_state
					 , u_long pkt_statistics);
void		ncr_set_done(ncr_t *ncrp, npt_t *nptp, nccb_t *poll_nccbp
					, u_char pkt_reason, u_long pkt_state
					, u_long pkt_statistics);

/*
 * Script functions
 */
static	int	ncr_script_offset(int func);
bool_t		ncr_script_init(void);
void		ncr_script_fini(void);


/*
 * Synchronous I/O functions
 */
static	bool_t	ncr_max_sync_rate_parse(ncr_t *ncrp, char *cp);
static	int	ncr_max_sync_lookup(char *savecp, int cnt);
bool_t		ncr_max_sync_divisor(ncr_t *ncrp, int syncioperiod,
			unchar *sxferp, unchar *sscfp);
int		ncr_period_round(ncr_t *ncrp, int syncioperiod);
void		ncr_max_sync_rate_init(ncr_t *ncrp, bool_t is710);



/*
 * Auto-config functions
 */
nops_t		*ncr_hbatype(dev_info_t *, int **, int *, bus_t *, bool_t);
bool_t		ncr_get_irq_pci(ncr_t *ncrp, int *regp, int reglen);
bool_t		ncr_get_ioaddr_pci(ncr_t *ncrp, int *regp, int reglen);
bool_t		ncr_cfg_init(dev_info_t *, ncr_t *);
int		ncr_geometry(ncr_t *, struct scsi_address *);
#if defined(i386)
bool_t		ncr_get_irq_eisa(ncr_t *ncrp, int *regp, int reglen);
bool_t		ncr_get_ioaddr_eisa(ncr_t *ncrp, int *regp, int reglen);
bool_t		eisa_probe(ioadr_t slotadr, ulong board_id);
bool_t		eisa_get_irq(ioadr_t slotadr, unchar *irqp);
#endif	/* defined(i386) */


static	bool_t	add_intr(dev_info_t *devi, u_int inumber
				, ddi_iblock_cookie_t *iblockp, kmutex_t *mutexp
				, char *mutexnamep, u_int (*intr_func)(caddr_t)
				, caddr_t intr_arg);
bool_t		ncr_find_irq(dev_info_t	*devi, unchar irq, int *intrp,
			int len, u_int *inumber);
bool_t		ncr_xlate_irq_sid(ncr_t *ncrp);
bool_t		ncr_xlate_irq_no_sid(ncr_t *ncrp);
bool_t		ncr_intr_init(dev_info_t *devi, ncr_t *ncrp, caddr_t intr_arg);

#ifdef	__cplusplus
}
#endif

#endif  /* _SYS_DKTP_NCRS_NCR_H */
