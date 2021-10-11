
/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _DPTGHD_SCSA_H
#define	_DPTGHD_SCSA_H

#pragma	ident	"@(#)dptghd_scsa.h	1.1	96/06/13 SMI"

#ifdef	__cplusplus
extern "C" {
#endif


#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/scsi/scsi.h>

/*
 * This really belongs in some sort of scsa include file since
 * it's used by the getcap/setcap interface.
 */
#define	HBA_SETGEOM(hd, sec) (((hd) << 16) | (sec))


struct scsi_pkt	*dptghd_tran_init_pkt(ccc_t *cccp, struct scsi_address *ap,
			struct scsi_pkt *pktp, struct buf *bp, int cmdlen,
			int statuslen, int tgtlen, int flags,
			int (*callback)(), caddr_t arg, int ccblen,
			ddi_dma_lim_t *sg_limitp, void *sg_func_arg);

void		 dptghd_tran_sync_pkt(struct scsi_address *ap,
			struct scsi_pkt *pktp);

void		 dptghd_pktfree(ccc_t *cccp, struct scsi_address *ap,
			struct scsi_pkt *pktp);

void		 dptghd_tran_dmafree(struct scsi_address *ap,
			struct scsi_pkt *pktp);

#ifdef	__cplusplus
}
#endif

#endif  /* _DPTGHD_SCSA_H */
