/*
 * Copyright (c) 1990-1992, by Sun Microsystems, Inc.
 */

#ident	"@(#)iommu.c	1.16	93/05/26 SMI"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/cmn_err.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <sys/machparam.h>
#include <sys/devaddr.h>
#include <sys/iommu.h>
#include <sys/ddidmareq.h>

#include <vm/as.h>
#include <sys/cpu.h>
#include <sys/pte.h>
#include <sys/iommu.h>
#include <vm/hat_srmmu.h>

int iom = 1;

#define	TSUNAMI_CONTROL_STORE_BUG
#ifdef	TSUNAMI_CONTROL_STORE_BUG
extern tsunami_control_store_bug;
#endif
extern int use_cache;
extern iommu_pte_t *ioptes;

#define	iopte_to_dvma_addr(piopte) ((IOMMU_DVMA_BASE + \
		(u_int) iommu_ptob((u_int) ((piopte)-ioptes))))

void
iommu_init()
{
	union iommu_ctl_reg ioctl_reg;
	union iommu_base_reg iobase_reg;
	extern iommu_pte_t *ioptes;
	extern iommu_pte_t *phys_iopte;

	bzero((caddr_t)ioptes, IOMMU_N_PTES);

	/* load iommu base addr */
	iobase_reg.base_uint = 0;
	iobase_reg.base_reg.base = ((u_int)phys_iopte) >> IOMMU_PTE_BASE_SHIFT;
	iommu_set_base(iobase_reg.base_uint);

	/* set control reg and turn it on */
	ioctl_reg.ctl_uint = 0;
	ioctl_reg.ctl_reg.enable = 1;
	ioctl_reg.ctl_reg.range = IOMMU_CTL_RANGE;
	iommu_set_ctl(ioctl_reg.ctl_uint);

	/* kindly flush all tlbs for any leftovers */
	iommu_flush_all();
}

iommu_pteload(piopte, mempfn, flag)
	iommu_pte_t *piopte;
	int mempfn;
	int flag;
{
	iommu_pte_t tmp_pte;
	u_int dvma_addr;

	if (piopte == NULL) {
		printf("iommu_pteload: no iopte!\n");
		return (-1);
	}
	tmp_pte.iopte = 0;

	tmp_pte.iopte = (mempfn << IOPTE_PFN_SHIFT) | IOPTE_VALID;
	if (flag & IOM_WRITE)
		tmp_pte.iopte |= IOPTE_WRITE;

	/* cache means memory access */
	if (use_cache && (flag & IOM_CACHE))
		tmp_pte.iopte |= IOPTE_CACHE;

	*piopte = tmp_pte;

#ifdef	TSUNAMI_CONTROL_STORE_BUG
	if (tsunami_control_store_bug) {
		mmu_flushall();
		return (0);
	}
#endif
	dvma_addr = iopte_to_dvma_addr(piopte);
	iommu_addr_flush((int) dvma_addr & IOMMU_FLUSH_MSK);

	return (0);
}

iommu_unload(dvma_addr, npf)
	u_int dvma_addr;
	int npf;
{
	iommu_pte_t *piopte, tiopte;

#ifdef	TSUNAMI_CONTROL_STORE_BUG
	if (tsunami_control_store_bug) {
		mmu_flushall();
		return (0);
	}
#endif

	if ((piopte = iommu_ptefind(dvma_addr)) == NULL) {
		printf("iommu_unload: no iopte! dvma_addr= 0x%x\n", dvma_addr);
		return (-1);
	}

	/* In-line pteunload. invalid iopte */
	while (npf-- > 0) {
		iommu_readpte(piopte, &tiopte);
		tiopte.iopte &= ~IOPTE_VALID;
		*piopte = tiopte;

		/*
		 * Now flush old TLB entry.
		 */
		iommu_addr_flush((int) dvma_addr & IOMMU_FLUSH_MSK);
		piopte++;
		dvma_addr += IOMMU_PAGE_SIZE;
	}

	return (0);
}

iommu_pteunload(piopte)
	iommu_pte_t *piopte;
{
	u_int dvma_addr;
	struct iommu_pte tiopte;

#ifdef	TSUNAMI_CONTROL_STORE_BUG
	if (tsunami_control_store_bug) {
		mmu_flushall();
		return (0);
	}
#endif

	dvma_addr = iopte_to_dvma_addr(piopte);

	iommu_readpte(piopte, &tiopte);
	tiopte.iopte &= ~IOPTE_VALID;
	*piopte = tiopte;
	iommu_addr_flush((int) dvma_addr & IOMMU_FLUSH_MSK);

	return (0);
}

iommu_pte_t *
iommu_ptefind(dvma_addr)
	int dvma_addr;
{
	int dvma_pfn;
	extern iommu_pte_t *ioptes;

	if (dvma_addr >= IOMMU_DVMA_BASE) {
		dvma_pfn = iommu_btop(dvma_addr - IOMMU_DVMA_BASE);
		return (&ioptes[dvma_pfn]);
	} else {
		return (NULL);
	}
}
