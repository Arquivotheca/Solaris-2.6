/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)dmc960.c	1.1	95/10/16 SMI"

#include <sys/dktp/mlx/mlx.h>

/*
 * MC product ids accepted by this driver for slots 0x0000 to 0x7000
 */
int	dmc960_mc_board_ids[] = {
		0x8f82,	/* cheetah DMC960 card */
		0x8fbb	/* passplay DMC960 card */
};

/*ARGSUSED*/
static bool_t
dmc960_probe(dev_info_t	*dip,
			int		*regp,
			int		 len,
			int		*pidp,
			int		 pidlen,
			bus_t		 bus_type,
			bool_t		 probing)
{
	ioadr_t	slotadr = MLX_ADDR(*regp);
	int	cnt;

	if (bus_type != BUS_TYPE_MC)
		return (FALSE);

	/* allow product-id property to override the default EISA id */
	if (pidp == NULL) {
		if (slotadr > 0x7000)
			return (FALSE);

		/* Handle normal EISA cards in a normal EISA slot */
		pidp = dmc960_mc_board_ids;
		pidlen = sizeof (dmc960_mc_board_ids);
	}

	/* check the product id of the current slot for a match */
	for (cnt = pidlen / sizeof (int); cnt > 0; cnt--, pidp++) {
		if (mc_probe(slotadr, *pidp)) {
			MDBG4(("dmc960_probe success %d %d\n", *pidp, cnt));
			return (TRUE);
		}
	}
	return (FALSE);
}

/*ARGSUSED*/
static void
dmc960_reset(mlx_t *mlxp)
{
	MDBG1(("dmc960_reset: Software reset completed\n"));
}

/*ARGSUSED*/
static bool_t
dmc960_init(mlx_t 		*mlxp,
		dev_info_t	*dip)
{
	unchar ret;

#ifndef PCI_DDI_EMULATION
	static ddi_device_acc_attr_t attr = {
		DDI_DEVICE_ATTR_V0,
		DDI_NEVERSWAP_ACC,	/* not portable */
		DDI_STRICTORDER_ACC,
	};
	int	iobase;
#endif

	/*
	 * mlx_get_reg_mc() has discovered the shared memory
	 * address and placed into our private regs copy.
	 */
	if (ddi_map_regs(dip, mlxp->rnum, (caddr_t *)&mlxp->membase,
	    (off_t)mlxp->regp[1], (off_t)mlxp->regp[2]) != DDI_SUCCESS) {
		MDBG1(("dmc960_init: no memaddr\n"));
		return (FALSE);
	}

#ifndef PCI_DDI_EMULATION
	if (ddi_regs_map_setup(dip, mlxp->rnum, (caddr_t *)&iobase,
	    (offset_t)0, (offset_t)0, &attr, &mlxp->handle) != DDI_SUCCESS) {
		MDBG1(("dmc960_init: no ioaddr\n"));
		return (FALSE);
	}
#endif
	ret = ddi_io_getb(mlxp->handle, mlxp->reg + SCP_REG)
		| DMC_ENABLE_BUS_MASTERING;
	ddi_io_putb(mlxp->handle, mlxp->reg + SCP_REG, ret);

	ret = ddi_io_getb(mlxp->handle, mlxp->reg + SCP_REG) & ~DMC_CLR_ON_READ;
	ddi_io_putb(mlxp->handle, mlxp->reg + SCP_REG, ret);

	MDBG1(("dmc960_init: Init completed\n"));
	return (TRUE);
}

/*ARGSUSED*/
static void
dmc960_uninit(mlx_t 		*mlxp,
		dev_info_t	*dip)
{
	ddi_unmap_regs(dip, mlxp->rnum, (caddr_t *)&mlxp->membase,
		(off_t)0, (off_t)0);

#ifndef	PCI_DDI_EMULATION
	ddi_regs_map_free(&mlxp->handle);
#endif
	MDBG1(("dmc960_uninit: Uninit completed\n"));
}

static void
dmc960_enable(mlx_t *mlxp)
{
	int ret = ddi_io_getb(mlxp->handle, mlxp->reg + SCP_REG) |
		DMC_ENABLE_INTRS;

	ddi_io_putb(mlxp->handle, mlxp->reg + SCP_REG, ret);
	MDBG1(("dmc960_enable: Enable completed\n"));
}

static void
dmc960_disable(mlx_t *mlxp)
{
	int ret = ddi_io_getb(mlxp->handle, mlxp->reg + SCP_REG) &
		~DMC_ENABLE_INTRS;

	ddi_io_putb(mlxp->handle, mlxp->reg + SCP_REG, ret);
	MDBG1(("dmc960_enable: Disable completed\n"));
}

static unchar
dmc960_cready(mlx_t *mlxp)
{
	MDBG1(("dmc960_cready: Cpoll completed\n"));
	return (*(mlxp->membase + MLX_MBXOFFSET + MCA_OFF) == MLX_CMBXFREE);
}

static bool_t
dmc960_csend(mlx_t *mlxp,
		void *ccbp)
{
	register unchar *ip = (unchar *)ccbp;
	register unchar *op = (unchar *)mlxp->membase + MLX_MBXOFFSET + MCA_OFF;
	register int cntr;
	register int i;

	/*
	 * Verify that the DMC960 has transferred all opcode to
	 * the shared memory. Too bad we have to do this!
	 */
	for (cntr = MLX_MAX_RETRY; cntr > 0; drv_usecwait(10), cntr--) {
		for (i = 1; i < MLX_MBXD; i++)
			op[i] = ip[i];

		/*
		 * write opcode last and check that it has been seen
		 */

		*op = *ip;
		if (*(mlxp->membase + MLX_MBXOFFSET + MCA_OFF) == *ip)
			break;
	}
	if (!cntr) {
		cmn_err(CE_WARN, "dmc960_csend: not ready to accept "
		    "commands, retried %u times", MLX_MAX_RETRY);
		return (FALSE);
	}

	/* Issue the command. */
	ddi_io_putb(mlxp->handle, mlxp->reg + ATTENTION_PORT, DMC_NEWCMD);

	MDBG1(("dmc960_send: Send completed\n"));
	return (TRUE);
}

static unchar
dmc960_iready(mlx_t *mlxp)
{
	MDBG1(("dmc960_iready: Ipoll completed\n"));
	return ((ddi_io_getb(mlxp->handle, mlxp->reg + CBSP_REG) &
		DMC_INTR_VALID));
}

static unchar
dmc960_get_istat(mlx_t *mlxp,
			void *hw_stat,
			int clear)
{
	register unchar *ip = (unchar *)hw_stat;
	unchar ret = ddi_io_getb(mlxp->handle, mlxp->reg + CBSP_REG);

	if (hw_stat) {
		/* copy status bytes */
		*ip = *(mlxp->membase + MLX_MBXOFFSET + MLX_MBXD + MCA_OFF);
		*(ushort *)(ip+1) = *(ushort *)(mlxp->membase +
			MLX_MBXOFFSET + MLX_MBXE + MCA_OFF);
	}

	if (clear) {
		/*
		 * The way to clear the BMIC eisa_sys_reg is to set it
		 * to whatever it was set when we read it.
		 */
		ddi_io_putb(mlxp->handle, mlxp->reg + SCP_REG,
			ddi_io_getb(mlxp->handle, mlxp->reg + SCP_REG)
				| DMC_CLR_ON_READ);
		(void) ddi_io_getb(mlxp->handle, mlxp->reg + CBSP_REG);
		ddi_io_putb(mlxp->handle, mlxp->reg + SCP_REG,
			ddi_io_getb(mlxp->handle, mlxp->reg + SCP_REG)
				& ~DMC_CLR_ON_READ);
		ddi_io_putb(mlxp->handle, mlxp->reg + ATTENTION_PORT,
			DMC_ACKINTR);
	}
	MDBG1(("dmc960_istat: Istat completed\n"));
	return (ret & DMC_INTR_VALID);
}

/*ARGSUSED*/
static int
dmc960_geometry(mlx_t			*mlxp,
			struct	scsi_address	*ap,
			ulong			blk)
{
	ulong	heads = 128;
	ulong	sectors = 32;

	if (ap)
cmn_err(CE_CONT, "?mlx(%d,%d): reg=0x%x, sectors=%d heads=%d sectors=%d\n",
		ap->a_target, ap->a_lun, mlxp->reg, blk, heads, sectors);
	return (HBA_SETGEOM(heads, sectors));
}


nops_t	dmc960_nops = {
	"dmc960",
	dmc960_probe,
	mlx_get_irq_mc,
	mlx_xlate_irq_no_sid,
	mlx_get_reg_mc,
	dmc960_reset,
	dmc960_init,
	dmc960_uninit,
	dmc960_enable,
	dmc960_disable,
	dmc960_cready,
	dmc960_csend,
	dmc960_iready,
	dmc960_get_istat,
	dmc960_geometry
};
