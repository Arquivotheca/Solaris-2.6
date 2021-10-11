/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#ifndef _SYS_DKTP_NCRS_NCROPS_H
#define	_SYS_DKTP_NCRS_NCROPS_H

#pragma	ident	"@(#)ncrops.h	1.8	95/12/12 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

typedef	struct ncrops {
	char		*ncr_chip;
	bool_t	(*ncr_script_init)(void);
	void	(*ncr_script_fini)(void);
	bool_t	(*ncr_probe)(dev_info_t *dip, int *regp, int len
				      , int *pidp, int pidlen
				      , bus_t bus_type, bool_t probing);
	bool_t	(*ncr_get_irq)(ncr_t *ncrp, int *regp, int reglen);
	bool_t	(*ncr_xlate_irq)(ncr_t *ncrp);
#ifdef	PCI_DDI_EMULATION
	bool_t	(*ncr_get_ioaddr)(ncr_t *ncrp, int *regp, int reglen);
#else
	int	ncr_rnumber;
#endif
	void	(*ncr_reset)(ncr_t *ncrp);
	void	(*ncr_init)(ncr_t *ncrp);
	void	(*ncr_enable)(ncr_t *ncrp);
	void	(*ncr_disable)(ncr_t *ncrp);
	unchar	(*ncr_get_istat)(ncr_t *ncrp);
	void	(*ncr_halt)(ncr_t *ncrp);
	void	(*ncr_set_sigp)(ncr_t *ncrp);
	void	(*ncr_reset_sigp)(ncr_t *ncrp);
	ulong	(*ncr_get_intcode)(ncr_t *ncrp);
	void	(*ncr_check_error)(npt_t *nptp, struct scsi_pkt *pktp);
	ulong	(*ncr_dma_status)(ncr_t *ncrp);
	ulong	(*ncr_scsi_status)(ncr_t *ncrp);
	bool_t	(*ncr_save_byte_count)(ncr_t *ncrp, npt_t *nptp);
	bool_t	(*ncr_get_target)(ncr_t *ncrp, unchar *tp);
	unchar	(*ncr_encode_id)(unchar id);
	void	(*ncr_setup_script)(ncr_t *ncrp, npt_t *nptp);
	void	(*ncr_start_script)(ncr_t *ncrp, int script);
	void	(*ncr_set_syncio)(ncr_t *ncrp, npt_t *nptp);
	void	(*ncr_bus_reset)(ncr_t *ncrp);
	int	(*ncr_geometry)(ncr_t *ncrp, struct scsi_address *ap);
} nops_t;


/* array of ptrs to ops tables for supported chip types */
extern	nops_t	*ncr_conf[];

#define	NCR_GET_IRQ(P, regp, reglen)	((P)->n_ops->ncr_get_irq)(P, regp \
						, reglen)
#define	NCR_XLATE_IRQ(P)	((P)->n_ops->ncr_xlate_irq)(P)
#define	NCR_GET_IOADDR(P, regp, reglen)	((P)->n_ops->ncr_get_ioaddr)(P, regp \
						, reglen)
#define	NCR_RNUMBER(P)		((P)->n_ops->ncr_rnumber)

#define	NCR_RESET(P)			((P)->n_ops->ncr_reset)(P)
#define	NCR_INIT(P)			((P)->n_ops->ncr_init)(P)
#define	NCR_ENABLE_INTR(P)		((P)->n_ops->ncr_enable)(P)
#define	NCR_DISABLE_INTR(P)		((P)->n_ops->ncr_disable)(P)
#define	NCR_GET_ISTAT(P)		((P)->n_ops->ncr_get_istat)(P)
#define	NCR_HALT(P)			((P)->n_ops->ncr_halt)(P)
#define	NCR_SET_SIGP(P)			((P)->n_ops->ncr_set_sigp)(P)
#define	NCR_RESET_SIGP(P)		((P)->n_ops->ncr_reset_sigp)(P)
#define	NCR_GET_INTCODE(P)		((P)->n_ops->ncr_get_intcode)(P)
#define	NCR_CHECK_ERROR(P, nptp, pktp)	((P)->n_ops->ncr_check_error)(nptp, \
						pktp)
#define	NCR_DMA_STATUS(P)		((P)->n_ops->ncr_dma_status)(P)
#define	NCR_SCSI_STATUS(P)		((P)->n_ops->ncr_scsi_status)(P)
#define	NCR_SAVE_BYTE_COUNT(P, nptp)	((P)->n_ops->ncr_save_byte_count)(P, \
						nptp)
#define	NCR_GET_TARGET(P, tp)		((P)->n_ops->ncr_get_target)(P, tp)
#define	NCR_ENCODE_ID(P, id)		((P)->n_ops->ncr_encode_id)(id)
#define	NCR_SETUP_SCRIPT(P, nptp)	((P)->n_ops->ncr_setup_script)(P, nptp)
#define	NCR_START_SCRIPT(P, script)	((P)->n_ops->ncr_start_script)(P, \
						script)
#define	NCR_SET_SYNCIO(P, nptp)		((P)->n_ops->ncr_set_syncio)(P, nptp)
#define	NCR_BUS_RESET(P)		((P)->n_ops->ncr_bus_reset)(P)

#define	NCR_GEOMETRY(P, ap)		((P)->n_ops->ncr_geometry)(P, ap)

/*
 * All models of the NCR are assumed to have consistent definitions
 * of the following bits in the ISTAT register. The ISTAT register
 * can be at different offsets but these bits must be the same.
 * If this isn't true then we'll have to add functions to the
 * ncrops table to access these bits similar to how the ncr_get_intcode()
 * function is defined.
 */

#define	NB_ISTAT_CON		0x08	/* connected */
#define	NB_ISTAT_SIP		0x02	/* scsi interrupt pending */
#define	NB_ISTAT_DIP		0x01	/* dma interrupt pending */

/*
 * Max SCSI Synchronous Offset bits in the SXFER register. Zero sets
 * asynchronous mode.
 */
#define	NB_SXFER_MO		0x0f	/* max scsi offset */

#ifdef	__cplusplus
}
#endif
#endif  /* _SYS_DKTP_NCRS_NCROPS_H */
