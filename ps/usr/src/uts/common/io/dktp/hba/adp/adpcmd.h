/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
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

#pragma ident	"@(#)adpcmd.h	1.7	96/07/30 SMI"


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

#if defined(i86pc)
#define	HBA_KVTOP(vaddr, shf, msk) \
		((paddr_t)(hat_getkpfnum((caddr_t)(vaddr)) << (shf)) | \
			    ((paddr_t)(vaddr) & (msk)))
#elif defined(prep)
#define	HBA_KVTOP(vaddr, shf, msk) \
		((paddr_t)(hat_getkpfnum((caddr_t)(vaddr)) << (shf)) | \
			    ((paddr_t)(vaddr) & (msk)) | 0x80000000)
#endif


/*
 * Wrapper for GHD cmd structure so that it's
 * linked to the adp SCB structure
 */

typedef struct adp_cmd {
	gcmd_t	cmd_gcmd;
	sp_struct *cmd_sp;
	int	cmd_cdblen;
	caddr_t cmd_sensep;
	int	cmd_senselen;
	int	cmd_sg_cnt;
} adpcmd_t;

#define	CMDP2GCMDP(cmdp)	(&(cmdp)->cmd_gcmd)
#define	CMDP2SCBP(cmdp)		((cmdp)->cmd_sp)

#define	GCMDP2CMDP(gcmdp)	((adpcmd_t *)(gcmdp)->cmd_private)
#define	GCMDP2SCBP(gcmdp)	(CMDP2SCBP(GCMDP2CMDP(gcmdp)))

#define	PKTP2CMDP(pktp)		GCMDP2CMDP(PKTP2GCMDP(pktp))
#define	PKTP2SCBP(pktp)		CMDP2SCBP(PKTP2CMDP(pktp))

#define SCBP2CMDP(scbp)		((scbp)->Sp_cmdp)
#define SCBP2GCMDP(scbp)	CMDP2GCMDP(SCBP2CMDP(scbp))


/*VARARGS1*/
void prom_printf(char *fmt, ...);

#define	PRF	prom_printf

#ifdef	__cplusplus
}
#endif

#endif  /* _SYS_DKTP_ADPCMD_H */
