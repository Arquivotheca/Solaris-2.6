/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)dac960p.c	1.2	95/10/30 SMI"

#include <sys/dktp/mlx/mlx.h>
#include <sys/esunddi.h>

/*ARGSUSED*/
static bool_t
dac960p_probe(dev_info_t	*dip,
			int		*regp,
			int		 len,
			int		*pidp,
			int		 pidlen,
			bus_t		 bus_type,
			bool_t		 probing)
{
	unsigned short	vendorid;
	unsigned short	deviceid;
	ddi_acc_handle_t handle;
	int 		rval;

	if (bus_type != BUS_TYPE_PCI)
		return (FALSE);

#ifdef	PCI_DDI_EMULATION
	/*
	 * XXX 2.4 limitations on the width of the reg property
	 * force the mlx channel number to be overloadded into
	 * pci information. This information is modified here to
	 * allow pci config operations to succeed.
	 * See mlx.h for details.
	 */
	if (probing) {
		int chn = MLX_CHN(NULL, *regp);
		int clen = sizeof (chn);

		/* remove channel number from reg property */
		*regp = *regp >> 8;
		if (e_ddi_prop_modify(DDI_DEV_T_NONE, dip, 0, "reg",
		    (caddr_t)regp, len) != DDI_PROP_SUCCESS)
			return (FALSE);

		/* save MSCSI_BUSPROP channel number property */
		len = sizeof (chn);
		(void) ddi_prop_create(DDI_DEV_T_NONE, dip, 0, MSCSI_BUSPROP,
			(caddr_t)&chn, clen);
	}
#endif

	if (pci_config_setup(dip, &handle) != DDI_SUCCESS)
		return (FALSE);

	vendorid = pci_config_getw(handle, PCI_CONF_VENID);
	deviceid = pci_config_getw(handle, PCI_CONF_DEVID);

	pci_config_teardown(&handle);

	rval = FALSE;
	switch (vendorid) {
	case 0x1069:
		switch (deviceid) {
		case 0x0001:
			rval = TRUE;
		}
	}

	if (probing) {
		if (rval == TRUE)
			cmn_err(CE_CONT,
			    "?dac960p_probe() vendor=0x%x device=0x%x\n",
			    vendorid, deviceid);
		else
			MDBG4(("dac960p_probe: Not found\n"));
	}

	MDBG4(("dac960p_probe: okay\n"));
	return (rval);
}

/*ARGSUSED*/
static void
dac960p_reset(mlx_t *mlxp)
{
	MDBG1(("dac960p_reset: Software reset completed\n"));
}

/*ARGSUSED*/
static bool_t
dac960p_init(mlx_t 		*mlxp,
		dev_info_t	*dip)
{
#ifndef	PCI_DDI_EMULATION
	static ddi_device_acc_attr_t attr = {
		DDI_DEVICE_ATTR_V0,
		DDI_NEVERSWAP_ACC,	/* not portable */
		DDI_STRICTORDER_ACC,
	};

	if (ddi_regs_map_setup(dip, mlxp->rnum, (caddr_t *)&mlxp->reg,
	    (offset_t)0, (offset_t)0, &attr, &mlxp->handle) != DDI_SUCCESS) {
		MDBG1(("dac960p_init: no ioaddr\n"));
		return (FALSE);
	}
#endif
	MDBG1(("dac960p_init: Init completed\n"));
	return (TRUE);
}

/*ARGSUSED*/
static void
dac960p_uninit(mlx_t 		*mlxp,
		dev_info_t	*dip)
{
#ifndef	PCI_DDI_EMULATION
	ddi_regs_map_free(&mlxp->handle);
#endif
	MDBG1(("dac960p_uninit: Uninit completed\n"));
}

static void
dac960p_enable(mlx_t *mlxp)
{
	ddi_io_putb(mlxp->handle, mlxp->reg + MLX_PCI_INTR, 1);
	MDBG1(("dac960p_enable: Enable completed\n"));
}

static void
dac960p_disable(mlx_t *mlxp)
{
	ddi_io_putb(mlxp->handle, mlxp->reg + MLX_PCI_INTR, 0);
	MDBG1(("dac960p_disable: Enable completed\n"));
}

static unchar
dac960p_cready(mlx_t *mlxp)
{
	MDBG1(("dac960p_cready: Cpoll completed\n"));
	return ((ddi_io_getb(mlxp->handle, mlxp->reg + MLX_PCI_LOCAL_DBELL)
		& 1) == MLX_CMBXFREE);
}

static bool_t
dac960p_csend(mlx_t *mlxp,
		void *ccbp)
{
	register ushort	op = mlxp->reg;
	register uint *ip = (uint *)ccbp;

	ddi_io_putl(mlxp->handle, op, *ip);
	ddi_io_putl(mlxp->handle, op+MLX_MBX4, *(ip+1));
	ddi_io_putl(mlxp->handle, op+MLX_MBX8, *(ip+2));
	ddi_io_putb(mlxp->handle, op+MLX_MBXC, (unchar)*(ip+3));
	ddi_io_putb(mlxp->handle, mlxp->reg + MLX_PCI_LOCAL_DBELL, MLX_NEWCMD);

	MDBG1(("dac960p_send: Send completed\n"));
	return (TRUE);
}

static unchar
dac960p_iready(mlx_t *mlxp)
{
	MDBG1(("dac960p_iready: Ipoll completed\n"));
	return (ddi_io_getb(mlxp->handle, mlxp->reg + MLX_PCI_SYS_DBELL) &
		MLX_STATREADY);
}

static unchar
dac960p_get_istat(mlx_t *mlxp,
			void *hw_stat,
			int clear)
{
	register unchar *ip = (unchar *)hw_stat;
	register ushort	op = mlxp->reg;
	unchar ret = ddi_io_getb(mlxp->handle, mlxp->reg + MLX_PCI_SYS_DBELL);

	if (hw_stat) {
		/* copy status bytes */
		*ip = ddi_io_getb(mlxp->handle, op+MLX_MBXD);
		*(ushort *)(ip+1) = ddi_io_getw(mlxp->handle, op+MLX_MBXE);
	}

	if (clear) {
		/*
		 * Clear the System Doorbell bit 0 by writing a
		 * 1 and then issuing an interrupt through Local
		 * Doorbell bit 1 to notify the DAC960P that the
		 * status mailbox is free.
		 */
		ddi_io_putb(mlxp->handle, mlxp->reg + MLX_PCI_SYS_DBELL, 1);
		ddi_io_putb(mlxp->handle, mlxp->reg + MLX_PCI_LOCAL_DBELL,
			MLX_STATCMP);
	}
	MDBG1(("dac960p_istat: Istat completed\n"));
	return (ret & MLX_STATREADY);
}

static int
dac960p_geometry(mlx_t			*mlxp,
			struct	scsi_address	*ap,
			ulong			blk)
{
	ulong			heads = 64, sectors = 32;

	/*
	 * Upto 1G (200000 blocks) :   64 x 32
	 *	2G (400000 blocks) :   65 x 63
	 *	4G (800000 blocks) :  128 x 63
	 * Over 4G (800001 blocks) :  255 x 63
	 */
	if (blk <= 0x200000) {
		heads   = 64;
		sectors = 32;
	} else if (blk <= 0x400000) {
		heads   = 65;
		sectors = 63;
	} else if (blk <= 0x800000) {
		heads   = 128;
		sectors = 63;
	} else {
		heads   = 255;
		sectors = 63;
	}

	if (ap)
cmn_err(CE_CONT, "?mlx(%d,%d): reg=0x%x, sectors=%d heads=%d sectors=%d\n",
		ap->a_target, ap->a_lun, mlxp->reg, blk, heads, sectors);
	return (HBA_SETGEOM(heads, sectors));
}

nops_t	dac960p_nops = {
	"dac960p",
	dac960p_probe,
	mlx_get_irq_pci,
#ifdef	PCI_DDI_EMULATION
	mlx_xlate_irq_no_sid,
#else
	mlx_xlate_irq_sid,
#endif
	mlx_get_reg_pci,
	dac960p_reset,
	dac960p_init,
	dac960p_uninit,
	dac960p_enable,
	dac960p_disable,
	dac960p_cready,
	dac960p_csend,
	dac960p_iready,
	dac960p_get_istat,
	dac960p_geometry
};
