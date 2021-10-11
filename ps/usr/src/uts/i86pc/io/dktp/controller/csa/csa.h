/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CSA_CSA_H
#define	_CSA_CSA_H

#pragma	ident	"@(#)csa.h	1.3	95/06/05 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/scsi/scsi.h>
#include <sys/dktp/dadev.h>
#include <sys/dktp/hba.h>
#include <sys/dktp/dadkio.h>
#include <sys/dktp/tgdk.h>

#include "csa_types.h"
#include "csa_queue.h"

extern	ddi_dma_lim_t	csa_dma_lim;

typedef struct  csa_blk {
	ushort			 cb_ioaddr;
	unchar			 cb_irq;
	int			 cb_nccbs;	/* max # ccbs to allocate */
#ifdef CSA_DEBUG
	ulong			 cb_nsent;
	ulong			 cb_ncomplete;
	ulong			 cb_nclear;
	ulong			 cb_nclear_not;
#endif
	dev_info_t		*cb_dip;
	Que_t			 cb_free_ccbs;
	Que_t			 cb_waitq;
	Que_t			 cb_doneq;
	uint			 cb_inumber;
	ddi_iblock_cookie_t	 cb_iblock;
	kmutex_t		 cb_mutex;
	kmutex_t		 cb_rmutex;	/* mutex for cb_free_ccbs */
	int			 cb_ccb_id;	/* callback for cb_free_ccbs */
#ifdef CSA_DEBUG
	unchar			 cb_status[256];
#endif CSA_DEBUG
	struct ccb		*cb_tags[256];
} csa_blk_t;

typedef struct csa_unit {
	ddi_dma_lim_t	cu_dmalim;
} csa_unit_t;


typedef struct csa {
	csa_blk_t	*c_blkp;
	csa_unit_t	*c_unitp;
	struct ctl_obj	*c_ctlobjp;
	ushort		 c_drive;	/* the drive number */
	ushort		 pad;		/* unused */
} csa_t;

#define	COOK2CSAUNITP(C)	((C)->c_unitp)
#define	PKTP2CSAUNITP(pktp)	(COOK2CSAUNITP((pktp)->pkt_address.a_cookie))
#define	ADDR2CSAUNITP(ap)	(COOK2CSAUNITP((ap)->a_cookie))

#define	COOK2CSABLKP(C)		((C)->c_blkp)
#define	PKTP2CSABLKP(pktp)	(COOK2CSABLKP((pktp)->pkt_address.a_cookie))
#define	ADDR2CSABLKP(ap)	(COOK2CSABLKP((ap)->a_cookie))

#define	CSAP2CSABLKP(P)		((P)->c_blkp)

#define	PKTP2CCBP(pktp)		((struct ccb *)((pktp)->cp_ctl_private))
#define	CCBP2PKTP(ccbp)		((ccbp)->ccb_pktp)

#define	CCBP2CSACCBP(ccbp)	((csa_ccb_t *)((ccbp)->ccb_private))
#define	TAGID2CCBP(BLKP, T)	((BLKP)->cb_tags[(T)])

#include "csa_impl.h"
#include "csa_cmds.h"
#include "ccb.h"
#include "csa_bmic.h"
#include "csa_common_eisa.h"
#include "csa_debug.h"

/*
 * common low-level functions
 */
void		 common_uninitchild(dev_info_t *mdip, dev_info_t *cdip);
int		 common_initchild(dev_info_t *mdip, dev_info_t *cdip,
			opaque_t ctl_data, struct ctl_objops *ctl_ops,
			struct ctl_obj **ctlobjpp);
struct cmpkt	*common_pktalloc(ccb_t *(*ccballoc_func)(opaque_t),
			void (*ccbfree_func)(opaque_t, ccb_t *),
			opaque_t ccb_arg);

typedef void (*SG_Func_t)(struct cmpkt *pktp, ccb_t *ccbp,
		ddi_dma_cookie_t *cp, int segno, opaque_t arg);

Bool_t		 common_iosetup(struct cmpkt *pktp, ccb_t *ccbp, int max_sgllen,
			SG_Func_t sg_func, opaque_t sg_func_arg);

Bool_t		 common_xlate_irq(dev_info_t *dip, unchar irq, u_int *inumber);
Bool_t		 common_intr_init(dev_info_t *dip, int inumber,
			ddi_iblock_cookie_t *iblock_cookiep, kmutex_t *mutexp,
			char *mutex_name, u_int (*int_handler)(caddr_t),
			caddr_t int_handler_arg);

void		 common_intr(opaque_t arg, opaque_t status,
			void (*process_intr)(opaque_t arg, opaque_t status),
			int (*get_status)(opaque_t arg, opaque_t *status),
			kmutex_t *mutexp, Que_t  *doneq);


struct cmpkt	*csa_pktalloc(csa_t *csap, int (*callback)(),
			caddr_t arg);
void		 csa_pktfree(csa_t *csap, struct cmpkt *pktp);
struct cmpkt	*csa_memsetup(csa_t *csap, struct cmpkt *pktp,
			struct buf *bp, int (*callback)(), caddr_t arg);
void		 csa_memfree(csa_t *csap, struct cmpkt *pktp);
struct cmpkt	*csa_iosetup(csa_t *csap, struct cmpkt *pktp);

#include <sys/dkio.h>
#include <sys/scsi/generic/inquiry.h>

#ifdef	__cplusplus
}
#endif

#endif  /* _CSA_CSA_H */
