/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)dac960.c	1.2	95/10/30 SMI"

#include <sys/dktp/mlx/mlx.h>

/*
 * EISA product ids accepted by this driver for slots 0x1000 to 0xf000
 */
int	dac960_eisa_board_ids[] = {
		0x70009835
};

/*ARGSUSED*/
static bool_t
dac960_probe(dev_info_t	*dip,
			int		*regp,
			int		 len,
			int		*pidp,
			int		 pidlen,
			bus_t		 bus_type,
			bool_t		 probing)
{
	ioadr_t	slotadr = MLX_ADDR(*regp);
	int	cnt;

	if (bus_type != BUS_TYPE_EISA)
		return (FALSE);

	/* allow product-id property to override the default EISA id */
	if (pidp == NULL) {
		if (slotadr < 0x1000)
			return (FALSE);

		/* Handle normal EISA cards in a normal EISA slot */
		pidp = dac960_eisa_board_ids;
		pidlen = sizeof (dac960_eisa_board_ids);
	}

	/* check the product id of the current slot for a match */
	for (cnt = pidlen / sizeof (int); cnt > 0; cnt--, pidp++) {
		if (eisa_probe(slotadr, *pidp)) {
			MDBG4(("dac960_probe success %d %d\n", *pidp, cnt));
			return (TRUE);
		}
	}
	return (FALSE);
}

/*ARGSUSED*/
static void
dac960_reset(mlx_t *mlxp)
{
	MDBG1(("dac960_reset: Software reset completed\n"));
}

/*ARGSUSED*/
static bool_t
dac960_init(mlx_t 		*mlxp,
		dev_info_t	*dip)
{
#ifndef	PCI_DDI_EMULATION
	int reg;
	static ddi_device_acc_attr_t attr = {
		DDI_DEVICE_ATTR_V0,
		DDI_NEVERSWAP_ACC,	/* not portable */
		DDI_STRICTORDER_ACC,
	};

	/* note that reg is overloaded on eisa */
	if (ddi_regs_map_setup(dip, mlxp->rnum, (caddr_t *)&reg,
	    (offset_t)0, (offset_t)0, &attr, &mlxp->handle) != DDI_SUCCESS) {
		MDBG1(("dac960_init: no ioaddr\n"));
		return (FALSE);
	}
#endif
	MDBG1(("dac960_init: Init completed\n"));
	return (TRUE);
}

/*ARGSUSED*/
static void
dac960_uninit(mlx_t 		*mlxp,
		dev_info_t	*dip)
{
#ifndef	PCI_DDI_EMULATION
	ddi_regs_map_free(&mlxp->handle);
#endif
	MDBG1(("dac960_uninit: Uninit completed\n"));
}

static void
dac960_enable(mlx_t *mlxp)
{
	ddi_io_putb(mlxp->handle, mlxp->reg + MLX_INTR_DEF1, 1);
	ddi_io_putb(mlxp->handle, mlxp->reg + MLX_INTR_DEF2, 1);
	MDBG1(("dac960_enable: Enable completed\n"));
}

static void
dac960_disable(mlx_t *mlxp)
{
	ddi_io_putb(mlxp->handle, mlxp->reg + MLX_INTR_DEF1, 0);
	ddi_io_putb(mlxp->handle, mlxp->reg + MLX_INTR_DEF2, 0);
	MDBG1(("dac960_disable: Disable completed\n"));
}

static unchar
dac960_cready(mlx_t *mlxp)
{
	MDBG1(("dac960_cready: Cpoll completed\n"));
	return (((ddi_io_getb(mlxp->handle, mlxp->reg + MLX_EISA_LOCAL_DBELL)
		& 0x1) == MLX_CMBXFREE));
}

static bool_t
dac960_csend(mlx_t *mlxp,
		void *ccbp)
{
	register uint *ip = (uint *)ccbp;
	register ushort	op = mlxp->reg + MLX_MBXOFFSET;

	ddi_io_putl(mlxp->handle, op, *ip);
	ddi_io_putl(mlxp->handle, op+MLX_MBX4, *(ip+1));
	ddi_io_putl(mlxp->handle, op+MLX_MBX8, *(ip+2));
	ddi_io_putb(mlxp->handle, op+MLX_MBXC, (unchar) *(ip+3));
	ddi_io_putb(mlxp->handle, mlxp->reg + MLX_EISA_LOCAL_DBELL, MLX_NEWCMD);

	MDBG1(("dac960_send: Send completed\n"));
	return (TRUE);
}

static unchar
dac960_iready(mlx_t *mlxp)
{
	MDBG1(("dac960_iready: Ipoll completed\n"));
	return ((ddi_io_getb(mlxp->handle, mlxp->reg + MLX_EISA_SYS_DBELL) &
		MLX_STATREADY));
}

static unchar
dac960_get_istat(mlx_t *mlxp,
			void *hw_stat,
			int clear)
{
	register unchar *ip = (unchar *)hw_stat;
	register ushort	op = mlxp->reg + MLX_MBXOFFSET;
	unchar ret = ddi_io_getb(mlxp->handle, mlxp->reg + MLX_EISA_SYS_DBELL);

	if (hw_stat) {
		/* copy status bytes */
		*ip = ddi_io_getb(mlxp->handle, op+MLX_MBXD);
		*(ushort *)(ip+1) = ddi_io_getw(mlxp->handle, op+MLX_MBXE);
	}

	if (clear) {
		/*
		 * The way to clear the BMIC eisa_sys_reg is to set it
		 * to whatever it was set when we read it.
		 */
		ddi_io_putb(mlxp->handle, mlxp->reg + MLX_EISA_SYS_DBELL, ret);
		ddi_io_putb(mlxp->handle, mlxp->reg + MLX_EISA_LOCAL_DBELL,
			MLX_STATCMP);
	}
	MDBG1(("dac960_istat: Istat completed\n"));
	return (ret & MLX_STATREADY);
}

/*ARGSUSED*/
static int
dac960_geometry(mlx_t			*mlxp,
			struct	scsi_address	*ap,
			ulong			blk)
{
	ulong	heads = 64;
	ulong	sectors = 32;

	if (ap)
cmn_err(CE_CONT, "?mlx(%d,%d): reg=0x%x, sectors=%d heads=%d sectors=%d\n",
		ap->a_target, ap->a_lun, mlxp->reg, blk, heads, sectors);
	return (HBA_SETGEOM(heads, sectors));
}

nops_t	dac960_nops = {
	"dac960",
	dac960_probe,
	mlx_get_irq_eisa,
	mlx_xlate_irq_no_sid,
	mlx_get_reg_eisa,
	dac960_reset,
	dac960_init,
	dac960_uninit,
	dac960_enable,
	dac960_disable,
	dac960_cready,
	dac960_csend,
	dac960_iready,
	dac960_get_istat,
	dac960_geometry
};
