/*
 * Copyright (c) 1992  Sun Microsystems, Inc.  All Rights Reserved.
 */

/* ddi wrapper for smc/wd driver */

#pragma ident "@(#)smcddi.c	1.17	96/05/27 SMI"

#include <sys/types.h>
#include <sys/conf.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/stream.h>
#include <sys/errno.h>
#include <sys/modctl.h>
#include <sys/sunddi.h>
#include <sys/stat.h>
#include <sys/ddi.h>
#include <sys/devops.h>
#include <sys/ethernet.h>
#include <sys/smc.h>
#include <sys/smchdw.h>
#include <sys/smcboard.h>

/* ---- LOCAL PROTOTYPES ---- */
int	wd_check_address(int);
void	wd_set_address(int);
void	smc_oem_final(void);
int	smc_fcs_checks(dev_info_t *, int);
/* ---- END OF PROTOTYPES ---- */

#define	D_SD_COMMENT "SMC/WD 8003/8013/8216 Ethernet driver"
#define	D_SD_NAME "smc"

extern int	wd_minors;	/* number of minor devices supported */
extern struct mod_ops mod_driverops;
extern struct streamtab wdinfo;
extern struct wdparam wdparams[];	/* board specific parameters */
extern int	wd_boardcnt;	/* number of boards */
extern kmutex_t wd_lock;	/* lock for this module */
extern int	wd_address_list[];
extern int	wd_boards_found;
#if defined(WDDEBUG)
extern int	wd_debug;
#endif

static int wd_info(dev_info_t *devi, ddi_info_cmd_t infocmd, void *arg,
	void **result);
static int	wd_probe(dev_info_t *devi);
static int	wd_attach(dev_info_t *devi, ddi_attach_cmd_t cmd);
static int	wd_reset(dev_info_t *dip, ddi_reset_cmd_t rst);

/* macro sets up ops structure */
DDI_DEFINE_STREAM_OPS(wd_ops, nulldev, wd_probe, \
		wd_attach, nodev, wd_reset, wd_info, D_MP, &wdinfo);

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module */
	D_SD_COMMENT,
	&wd_ops,		/* driver ops */
};

static struct modlinkage modlinkage = {
MODREV_1, (void *) &modldrv, NULL};

int
_init()
{
	return (mod_install(&modlinkage));
}

int
_fini()
{
	return (mod_remove(&modlinkage));
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

static dev_info_t *_mi_driver_dev_info;

/*
 * There shouldn't be an smc card below this address.
 */
static int last_addr = 0x200;
static int last_slot;

/*
 * Determine if non-self-identifying hardware is present.  This routine is
 * called before attach().
 */
/* ARGSUSED */
static int
wd_probe(dev_info_t *devi)
{
	int	regbuf;
	int	buflen = sizeof (regbuf);

	if (micro_channel(devi)) {
		/*
		 * MCA cards don't have an ioaddr spec because everything
		 * is slot based.
		 */
		if (WD_PROP(devi, "ioaddr", &regbuf, &buflen) !=
		    DDI_PROP_SUCCESS) {
			for (; last_slot < MC_SLOTS; last_slot++) {
				/*
				 * If the current slot has already been found
				 * just continue and search the next slot.
				 */
				if (wd_check_address(last_slot))
					continue;
				if (check_slot(last_slot)) {
					wd_set_address(last_slot);
					return (DDI_PROBE_SUCCESS);
				}
			}
		}
	} else {
		/*
		 * If the config file has "ioaddr" assume that this board
		 * doesn't have an interface chip and just use the values
		 * found in the config file.
		 */
		if (WD_PROP(devi, "ioaddr", &regbuf, &buflen) ==
		    DDI_PROP_SUCCESS) {
			if (regbuf == 0)
				smc_oem_final();
			else if ((wd_check_address(regbuf) == 0) &&
				(check_addr_ROM(regbuf) == 0) &&
				smc_fcs_checks(devi, regbuf)) {
					/* save the address for future use */
					ddi_set_driver_private(devi,
						(caddr_t)regbuf);
					wd_set_address(regbuf);
					return (DDI_PROBE_SUCCESS);
			}
		} else {
			for (; last_addr < 0x400; last_addr += 0x20) {
				if ((wd_check_address(last_addr) == 0) &&
				    (check_addr_ROM(last_addr) == 0) &&
				    smc_fcs_checks(devi, last_addr)) {
					ddi_set_driver_private(devi,
						(caddr_t)last_addr);
					wd_set_address(last_addr);
					return (DDI_PROBE_SUCCESS);
				}
			}
		}

	}

	return (DDI_PROBE_FAILURE);
}

/*
 * Return 1 if smc driver should use this board. Else 0.
 */
smc_fcs_checks(dev_info_t *devi, int addr)
{
	int board = GetBoardID(addr, micro_channel(devi));
	int propbuf[2], buflen;

	buflen = sizeof (int) * 2;
	/*
	 * Boards with interface chips should have 'intr' properties
	 * with multiple tuples so that we can do autoconfiguration.
	 * If that's the case this WD_PROP call will fail.
	 */
	if (WD_PROP(devi, SmcIntr, propbuf, &buflen) == DDI_PROP_SUCCESS) {
		if (board & INTERFACE_CHIP)
			/* smart card, dumb entry */
			return (0);
		else
			/* dumb card, dumb entry == match */
			return (1);
	} else {
		if (board & INTERFACE_CHIP)
			/* smart card, smart entry == match */
			return (1);
		else
			/* dumb card, smart entry */
			return (0);
	}
}

/*
 * Go though the address range once making sure that every board found
 * has also been probed.
 */
static int smc_oem_final_once;
void
smc_oem_final(void)
{
	int i;

	/*
	 * There's a bug in the parsing routine for the conf files.
	 * The last entry gets called twice for some reason. This will
	 * stop multiple complants from appearing on the screen.
	 */
	if (smc_oem_final_once == 1)
		return;
	else
		smc_oem_final_once = 1;

	for (i = 0x200; i < 0x400; i += 0x20) {
		if (check_addr_ROM(i) == 0) {
			/*
			 * found board. Make sure that it's been probed
			 * successfully. If not, complain.
			 */
			if (wd_check_address(i) == 0) {
cmn_err(CE_CONT, "SMC: smc board at 0x%x not configured successfully.", i);
cmn_err(CE_CONT, "     Please add a correct entry in /kernel/drv/smc.conf\n");
			}
		}
	}
}

/*
 * SMC boards with the interface chip can be autoconfigured. The other boards
 * must have everything spelled out in the .conf file. It's possible to have
 * a dumb board at 0x280 first in the config file followed by an entry
 * without an address indicating that the board supports autoconfig. During
 * the probe of the second board we'd search the i/o address space looking
 * for a board. We must ignore boards that we've already found which have an
 * i/o address in the conf file.
 */
void
wd_set_address(int addr)
{
	wd_address_list[wd_boards_found++] = addr;
}

wd_check_address(int addr)
{
	int i;

	for (i = 0; i < wd_boards_found; i++)
		if (wd_address_list[i] == addr)
			return (1);
	return (0);
}

/*
 * Attach a device to the system.
 * Attach should return either DDI_SUCCESS or DDI_FAILURE
 */
/* ARGSUSED */
static int
wd_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	static once = 1;
	struct wdparam *wdp = wdparams;
	int i;

#if defined(WDDEBUG)
	if (wd_debug & WDDDI)
		cmn_err(CE_CONT, "wd_attach: entered devi = x%x cmd = %d\n",
			devi, cmd);
#endif

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	/* Do any one time initializations */
	if (once) {
		wdinit(devi);
		once = 0;
	}
	for (i = 0; i < wd_boardcnt; i++, wdp++) {
		if (wdp->wd_noboard == 0)
			break;
	}

	if (i >= wd_boardcnt)
		return (DDI_FAILURE);

	/*
	 * Get the ioaddr that was stored in devi from wdprobe and replace
	 * the value with a pointer to this wdparam structure.
	 * NOTE: This is only valid on ISA machines. During wdinit() on MCA
	 * types the ioaddr is retrieved from the POS registers.
	 */
	if (micro_channel(devi)) {
		if (wdp->wd_ioaddr == 0)
			return (DDI_FAILURE);
	}
	else
		wdp->wd_ioaddr = (short)(int)ddi_get_driver_private(devi);
	ddi_set_driver_private(devi, (caddr_t)wdp);

	wdp->wd_devi = devi;
	wdp->wd_noboard = i + 1;
	wdp->wd_minors = wd_minors;

	/*
	 * fill in a wddev structure for this device including ioaddr,
	 * memaddr, memlen and irq
	 */
	if (wdsetup(devi) == DDI_FAILURE)
		return (DDI_FAILURE);

	/* announce that this device has been found */
	ddi_report_dev(devi);
	ddi_create_minor_node(devi, "smc", S_IFCHR, 0, DDI_NT_NET, 1);
	return (DDI_SUCCESS);
}

/* ARGSUSED */
static int
wd_info(dev_info_t *devi, ddi_info_cmd_t infocmd, void *arg,
	void **result)
{
	register int    error;

	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if (_mi_driver_dev_info == NULL) {
			error = DDI_FAILURE;
		} else {
			*result = (void *) _mi_driver_dev_info;
			error = DDI_SUCCESS;
		}
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *) 0;
		error = DDI_SUCCESS;
		break;
	default:
		error = DDI_FAILURE;
	}
	return (error);
}

static int
wd_reset(dev_info_t *dip, ddi_reset_cmd_t rst)
{
	struct wdparam *wdp;
	int i;

	/* call function in smc.c to reset board */

	switch (rst) {
		case DDI_RESET_FORCE:
			for (i = 0, wdp = wdparams;
			    i < wd_boardcnt;
			    i++, wdp++) {
				if (wdp->wd_devi == dip) {
					wduninit_board(wdp);
					break;
				}
			}
			return (0);

		default:
			return (0);
	}
}
