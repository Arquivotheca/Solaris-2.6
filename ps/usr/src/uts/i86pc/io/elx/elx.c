/*
 * Copyright (c) 1993, 1996, by Sun Microsystems, Inc.
 * All Rights Reserved.
 */
#ident "@(#)elx.c	1.41     96/07/09 SMI"

/*
 * elx -- 3COM EtherLink III family of Ethernet controllers
 * Depends on the Generic LAN Driver utility functions in /kernel/misc/gld
 */

#if defined(i386)
#define	_mca_bus_supported
#define	_eisa_bus_supported
#define	_isa_bus_supported
#endif

#if defined(__ppc)
#define	_isa_bus_supported
#endif

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/devops.h>
#include <sys/ksynch.h>
#include <sys/stat.h>
#include <sys/modctl.h>

#ifdef _DDICT
#include "sys/dlpi.h"
#include "sys/ethernet.h"
#include "sys/gld.h"
#include "sys/eisarom.h"
#include "sys/nvm.h"
#else
#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <sys/gld.h>
#if defined(_eisa_bus_supported)
#include <sys/eisarom.h>
#include <sys/nvm.h>
#endif
#endif

#include <sys/pci.h>
#include <sys/ddi.h>
#include <sys/debug.h>

#if	defined PCI_DDI_EMULATION || COMMON_IO_EMULATION
#include <sys/xpci/sunddi_2.5.h>
#else	/* PCI_DDI_EMULATION */
#include <sys/sunddi.h>
#endif	/* PCI_DDI_EMULATION */

#include "sys/elx.h"

/*
 *  Declarations and Module Linkage
 */

static char ident[] = "3COM EtherLink III";

#ifdef ELXDEBUG
/* used for debugging */
int	elxdebug = ELXDDI|ELXERRS|ELXTRACE|ELXDDIPROBE;
#endif

/* Required system entry points */
static	elxattach(dev_info_t *, ddi_attach_cmd_t);
static	elxdetach(dev_info_t *, ddi_detach_cmd_t);
static	elxdevinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	elxidentify(dev_info_t *);
static	elxprobe(dev_info_t *);

/* Required driver entry points for GLD */
static	int	elx_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
static	int	elx_gstat(gld_mac_info_t *);
static	int	elx_prom(gld_mac_info_t *, int);
static	int	elx_reset(gld_mac_info_t *);
static	int	elx_send(gld_mac_info_t *, mblk_t *);
static	int	elx_start_board(gld_mac_info_t *);
static	int	elx_stop_board(gld_mac_info_t *);
static	u_int	elxintr(gld_mac_info_t *);

/* Local Routines */
static	void	elx_config(gld_mac_info_t *);
static	void	elx_ad_failure(gld_mac_info_t *);
static	void	elx_enable_bus(gld_mac_info_t *, int);
static	void	elx_getp(gld_mac_info_t *);
static  int	elx_get_bustype(dev_info_t *);
static	int	elx_get_irq(dev_info_t *devinfo, elx_t *elxp);
static	int	elx_get_ioaddr(dev_info_t *, int,  ddi_acc_handle_t *);
static	int	elx_eisa_probe(ddi_acc_handle_t, int);
static	int	elx_isa_probe(ddi_acc_handle_t, int);
static	int	elx_isa_read_prom(ddi_acc_handle_t, int, int);
static	int	elx_isa_init(ddi_acc_handle_t, int, short *, int);
static  void    elx_isa_sort(short *, int);
static	int	elx_mca_probe(ddi_acc_handle_t, int);
static	void	elx_read_pos(ddi_acc_handle_t, int, unsigned char *);
static  int	elx_read_nvm(int, ushort *);
static	ushort	elx_read_prom(ddi_acc_handle_t, int, int);
static  int	elx_regs_map_setup(dev_info_t *, int, int *,
			ddi_acc_handle_t *);
static void	elx_regs_map_free(int, ddi_acc_handle_t *);
static	void	elx_restart_rx(gld_mac_info_t *);
static	void	elx_restart_tx(gld_mac_info_t *);
static	void	elx_set_ercv(gld_mac_info_t *);
static	int	elx_dma_attach(dev_info_t *, gld_mac_info_t *);

/* Exported routines */
void	elx_discard(gld_mac_info_t *, int);
void    elx_msdelay(int);
void	elx_poll_cip(elx_t *, int, int, int);
int	elx_restart(gld_mac_info_t *);
int	elx_saddr(gld_mac_info_t *);
int	elx_verify_id(ushort);
ushort	elx_set_imask(ddi_acc_handle_t, long, ushort, ushort);

/* Imported routines */
extern	void	elx_dma_intr(gld_mac_info_t *, elx_t *, int);
extern	int	elx_med_sense(gld_mac_info_t *, ushort, int);
extern	void	elx_med_set(gld_mac_info_t *);
extern	void	elx_pci_enable(dev_info_t *, int);
extern	int	elx_pci_get_irq(dev_info_t *);
extern	int	elx_pci_probe(dev_info_t *);
extern	void	elx_pio_recv(ddi_acc_handle_t, int, unchar *, int);
extern	int	elx_recv_msg(elx_t *, int, mblk_t *, int);
extern	int	elx_send_msg(gld_mac_info_t *, int, mblk_t *);
extern	int	eisa_nvm(char *, KEY_MASK, ...);
extern	ushort	ntohs(ushort);

extern	int	elx_xmt_dma_thresh;
extern	int	elx_rcv_dma_thresh;

#if	defined PCI_DDI_EMULATION || COMMON_IO_EMULATION
char _depends_on[] = "misc/gld misc/xpci";
#else
char _depends_on[] = "misc/gld";
#endif

/* Standard Streams initialization */

static struct module_info minfo = {
	ELXIDNUM, "elx", 0, INFPSZ, ELXHIWAT, ELXLOWAT
};

static struct qinit rinit = {	/* read queues */
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
};

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, NULL, NULL, NULL, &minfo, NULL
};

static struct streamtab elxinfo = {&rinit, &winit, NULL, NULL};

/* Standard Module linkage initialization for a Streams driver */

extern struct mod_ops mod_driverops;

static 	struct cb_ops cb_elxops = {
	nulldev,		/* cb_open */
	nulldev,		/* cb_close */
	nodev,			/* cb_strategy */
	nodev,			/* cb_print */
	nodev,			/* cb_dump */
	nodev,			/* cb_read */
	nodev,			/* cb_write */
	nodev,			/* cb_ioctl */
	nodev,			/* cb_devmap */
	nodev,			/* cb_mmap */
	nodev,			/* cb_segmap */
	nochpoll,		/* cb_chpoll */
	ddi_prop_op,		/* cb_prop_op */
	&elxinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

static struct dev_ops elxops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	elxdevinfo,		/* devo_getinfo */
	elxidentify,		/* devo_identify */
	elxprobe,		/* devo_probe */
	elxattach,		/* devo_attach */
	elxdetach,		/* devo_detach */
	nodev,			/* devo_reset */
	&cb_elxops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,			/* short description */
	&elxops			/* driver specific ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

static kmutex_t elx_probe_lock;

int
_init(void)
{
	int	status;

	mutex_init(&elx_probe_lock, "elx probe serializer",
		    MUTEX_DRIVER, NULL);

	status = mod_install(&modlinkage);
	if (status != 0) {
		mutex_destroy(&elx_probe_lock);
	}
	return (status);
}

int
_fini(void)
{
	int	status;

	status = mod_remove(&modlinkage);
	if (status != 0)
		return (status);

	mutex_destroy(&elx_probe_lock);

	return (0);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*
 *  DDI Entry Points
 */

elxidentify(dev_info_t *devinfo)
{
	char *name = ddi_get_name(devinfo);
	if (strcmp(name, "elx") == 0 ||
	    strcmp(name, "pci10b7,5950") == 0 ||
	    strcmp(name, "pci10b7,5900") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}

elxdevinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int error;
#ifdef lint
	arg = arg;
#endif

	/* This code is not DDI compliant: the correct semantics */
	/* for CLONE devices is not well-defined yet. */
	switch (cmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if (devinfo == NULL) {
			error = DDI_FAILURE;	/* Unfortunate */
		} else {
			*result = (void *)devinfo;
			error = DDI_SUCCESS;
		}
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *)0;	/* This CLONEDEV always returns zero */
		error = DDI_SUCCESS;
		break;
	default:
		error = DDI_FAILURE;
	}
	return (error);
}


elxprobe(dev_info_t *devinfo)
{
	int bustype;
	int ioaddr, found_board = 0;
	ddi_acc_handle_t handle;

#ifdef ELXDEBUG
	if (elxdebug & ELXDDIPROBE)
		cmn_err(CE_CONT, "elxprobe(0x%x)", devinfo);
#endif

	/* defer if anyone else is probing (MP systems) */
	mutex_enter(&elx_probe_lock);

	if ((bustype = elx_get_bustype(devinfo)) == ELX_NOBUS)
		goto failure;

	if (bustype != ELX_PCI)
		if (elx_regs_map_setup(devinfo, bustype, &ioaddr, &handle)
		    != DDI_SUCCESS)
			goto failure;

	switch (bustype) {
	case ELX_PCI:
		found_board = elx_pci_probe(devinfo);
		break;
	case ELX_MCA:	/* microchannel boards */
		found_board = elx_mca_probe(handle, ioaddr - ELX_MCA_OFFSET);
		break;

	case ELX_EISA:	/* eisa & eisa-mode boards */
		found_board = elx_eisa_probe(handle, ioaddr);
		break;

	case ELX_ISA:
	/*
	 * note that most ISA boards would have been found above for EISA
	 * based systems.  This is just to catch those not configured and
	 * those on a real ISA system.
	 */
		found_board = elx_isa_probe(handle, ioaddr);
		break;

	default:
		cmn_err(CE_WARN, "!elx: unknown bus type 0x%x!",
			elx_get_bustype(devinfo));
		break;
	}

	if (bustype != ELX_PCI)
		elx_regs_map_free(bustype, &handle);

	mutex_exit(&elx_probe_lock);

	if (found_board)
		return (DDI_PROBE_SUCCESS);
	else
		return (DDI_PROBE_FAILURE);
failure:
	mutex_exit(&elx_probe_lock);
	return (DDI_PROBE_FAILURE);
}


elxattach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	int port, i;
	elx_t *elxp;
	gld_mac_info_t *macinfo;
	ddi_acc_handle_t handle;

#ifdef ELXDEBUG
	if (elxdebug & ELXDDI)
		cmn_err(CE_CONT, "elxattach(0x%x)", devinfo);
#endif

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	/*
	 *  Allocate gld_mac_info_t and elxinstance structures
	 */
	macinfo = (gld_mac_info_t *)kmem_zalloc(sizeof (elx_t) +
			sizeof (gld_mac_info_t), KM_NOSLEEP);
	if (macinfo == NULL) {
		cmn_err(CE_WARN, "elx: kmem_zalloc failure for macinfo");
		return (DDI_FAILURE);
	}

	/*
	 * Initialize our private fields in macinfo and elxp.
	 */
	macinfo->gldm_private = (caddr_t)(macinfo+1);
	elxp = (elx_t *)macinfo->gldm_private;
	if ((elxp->elx_bus = elx_get_bustype(devinfo)) == ELX_NOBUS ||
	    (port = elx_get_ioaddr(devinfo, elxp->elx_bus, &handle)) == -1)
		goto failure;

	macinfo->gldm_port = port;
	elxp->io_handle = handle;

	if (elxp->elx_bus == ELX_EISA && port < MIN_EISA_ADDR)
		elxp->elx_bus = ELX_ISA;

	macinfo->gldm_devinfo = devinfo;
	elx_enable_bus(macinfo, 1);
	/*
	 * Without the following global reset elx_read_prom, when
	 * called by elx_config, will hang on EEPROM_BUSY after
	 * a warm start.
	 */
	if ((elxp->elx_bus == ELX_EISA) || (elxp->elx_bus == ELX_PCI)) {
		DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_GLOBAL_RESET, 0));
		elx_msdelay(4);
	}

	(void) elx_set_imask(handle, port, 0, ELINTR_DISABLE);

	/*
	 *  Do anything necessary to prepare the board for operation
	 *  short of actually starting the board.
	 */
	elx_config(macinfo);

	/*  If controller can do dma setup buffers */
	if (ELX_CAN_DMA(elxp))
		if (elx_dma_attach(devinfo, macinfo) == DDI_FAILURE)
			goto failure;

	macinfo->gldm_state = ELX_IDLE;
	macinfo->gldm_flags = 0;
	macinfo->gldm_irq_index = elx_get_irq(devinfo, elxp);
	macinfo->gldm_reg_index = -1;

	/*
	 *  Initialize pointers to device specific functions which will be
	 *  used by the generic layer.
	 */
	macinfo->gldm_reset   = elx_reset;
	macinfo->gldm_start   = elx_start_board;
	macinfo->gldm_stop    = elx_stop_board;
	macinfo->gldm_saddr   = elx_saddr;
	macinfo->gldm_sdmulti = elx_dlsdmult;
	macinfo->gldm_prom    = elx_prom;
	macinfo->gldm_gstat   = elx_gstat;
	macinfo->gldm_send    = elx_send;
	macinfo->gldm_intr    = elxintr;
	macinfo->gldm_ioctl   = NULL;    /* if you have one, NULL otherwise */

	/*
	 *  Initialize board characteristics needed by the generic layer.
	 */
	macinfo->gldm_ident = ident;
	macinfo->gldm_type = DL_ETHER;
	macinfo->gldm_minpkt = 0;		/* assumes we pad ourselves */
	macinfo->gldm_maxpkt = ELXMAXPKT;
	macinfo->gldm_addrlen = ETHERADDRL;
	macinfo->gldm_saplen = -2;

	/*
	 * Get the board's vendor-assigned hardware network address.
	 */
	for (i = 0; i < 3; i ++)
		*(((u_short *)(macinfo->gldm_vendor)) + i) =
			ntohs(elx_read_prom(handle, port, i));
	bcopy((caddr_t)macinfo->gldm_vendor,
		(caddr_t)macinfo->gldm_macaddr, ETHERADDRL);

	bcopy((caddr_t)gldbroadcastaddr,
		(caddr_t)macinfo->gldm_broadcast, ETHERADDRL);

	/*
	 *  Register ourselves with the GLD interface
	 *
	 *  gld_register will:
	 *	link us with the GLD system;
	 *	set our ddi_set_driver_private(9F) data to the macinfo pointer;
	 *	save the devinfo pointer in macinfo->gldm_devinfo;
	 *	map the registers, putting the kvaddr into macinfo->gldm_memp;
	 *	add the interrupt, putting the cookie in gldm_cookie;
	 *	init the gldm_intrlock mutex which will block that interrupt;
	 *	create the minor node.
	 */
	if (gld_register(devinfo, "elx", macinfo) == DDI_SUCCESS) {
		return (DDI_SUCCESS);
	} else
		cmn_err(CE_WARN,
		    "elx: failed to successfully register with GLD utility");

failure:
	kmem_free((caddr_t)macinfo,
		sizeof (gld_mac_info_t) + sizeof (elx_t));
	return (DDI_FAILURE);
}

elxdetach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */
	elx_t *elxp;

#ifdef ELXDEBUG
	if (elxdebug & ELXDDI)
		cmn_err(CE_CONT, "elxdetach(0x%x)", devinfo);
#endif

	if (cmd != DDI_DETACH)
		return (DDI_FAILURE);

	/* Get the driver private (gld_mac_info_t) structure */
	macinfo = (gld_mac_info_t *)ddi_get_driver_private(devinfo);

	/* Stop the board and reset it */
	(void) elx_reset(macinfo);

	/* Release buffers */
	elxp = (elx_t *)macinfo->gldm_private;
	if (elxp->elx_dma_rbuf)
		ddi_iopb_free(elxp->elx_rbuf);
	if (elxp->elx_dma_xbuf)
		ddi_iopb_free(elxp->elx_xbuf);

	/* Release handle */
	elx_regs_map_free(elxp->elx_bus, &elxp->io_handle);

	/*
	 *  Unregister ourselves from the GLD interface
	 *
	 *  gld_unregister will:
	 *	remove the minor node;
	 *	unmap the registers;
	 *	remove the interrupt;
	 *	destroy the gldm_intrlock mutex;
	 *	unlink us from the GLD system.
	 */
	if (gld_unregister(macinfo) == DDI_SUCCESS) {
		kmem_free((caddr_t)macinfo,
			sizeof (gld_mac_info_t) + sizeof (elx_t));
		return (DDI_SUCCESS);
	}
	return (DDI_FAILURE);
}

/*
 * elx_poll_cip()
 *	check the command in progress bit to make sure things are OK
 *	There is a bound on the number of times to check and
 *	an optional call to hardware reset if things aren't working
 *	out.
 */
void
elx_poll_cip(elx_t *elxp, int port, int times, int do_restart)
{
	int i;
	ddi_acc_handle_t handle = elxp->io_handle;

	for (i = times; i > 0 && DDI_INW(port + ELX_STATUS) & ELSTATUS_CIP; i--)
		drv_usecwait(100);
	if (do_restart && DDI_INW(port + ELX_STATUS) & ELSTATUS_CIP) {
		cmn_err(CE_WARN, "!elx%d: rx discard failure (resetting)", 
			elxp->elx_mac->gldm_ppa);
		(void) elx_restart(elxp->elx_mac);
	}
}

ushort
elx_set_imask(ddi_acc_handle_t handle, long port, ushort on, ushort off)
{
	ushort imask, oimask;
	ushort window;

	SWTCH_WINDOW(port, 5, window);
	oimask = DDI_INW(port + ELX_INTR_MASK);
	RESTORE_WINDOW(port, window, 5);
	if ((on|off) == 0)
		return (oimask);
	if (off)
		imask = oimask & ~off;
	if (on)
		imask = oimask | on;
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_SET_READ_ZERO, imask));
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_SET_INTR, imask));

	return (oimask);
}

/*
 *  GLD Entry Points
 */

/*
 *  elx_reset () -- reset the board to initial state; save the machine
 *  address and restore it afterwards.
 */
static int
elx_reset(gld_mac_info_t *macinfo)
{
	int port = macinfo->gldm_port;
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_reset(0x%x)", macinfo);
#endif
	(void) elx_stop_board(macinfo);
	if (elxp->elx_bus == ELX_EISA || elxp->elx_bus == ELX_PCI) {
		DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_GLOBAL_RESET, 0));
		elx_msdelay(3);
	}
	SET_WINDOW(port, 1);

	return (0);
}

/*
 *  elx_start_board () -- start the board receiving and allow transmits.
 */
static int
elx_start_board(gld_mac_info_t *macinfo)
{
	ushort value, window;
	int port = macinfo->gldm_port;
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_start_board(0x%x)", macinfo);
#endif

	elx_enable_bus(macinfo, 1);

	SWTCH_WINDOW(port, 0, window);

	elx_med_set(macinfo);

	elxp->elx_rcvbuf = NULL;

	if (elxp->elx_bus != ELX_PCI && elxp->elx_irq) {
	    value = DDI_INW(port + ELX_RESOURCE_CFG);
	    value = (value & 0xfff) | (elxp->elx_irq << 12);
	    DDI_OUTW(port + ELX_RESOURCE_CFG, value);
	}

	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_RX_RESET, 0));
	elx_poll_cip(elxp, port, ELX_CIP_RETRIES, 0);
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_TX_RESET, 0));
	elx_poll_cip(elxp, port, ELX_CIP_RETRIES, 0);
	(void) elx_saddr(macinfo);
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_STAT_ENABLE, 0));
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_TX_ENABLE, 0));
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_RX_ENABLE, 0));
	DDI_OUTW(port + ELX_COMMAND,
			COMMAND(ELC_SET_RX_FILTER, elxp->elx_rxbits));

	/*
	 * Request an interrupt to measure the interrupt latency
	 * and set the early receive threshold.
	 */
	(void) elx_set_imask(handle, port,
		ELINTR_DEFAULT(ELX_CAN_DMA(elxp)), 0);
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_REQ_INTR, 0));

	RESTORE_WINDOW(port, window, 0);

	return (0);
}

/*
 *  elx_stop_board () -- stop board receiving
 */
static int
elx_stop_board(gld_mac_info_t *macinfo)
{
	register port = macinfo->gldm_port;
	register elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_stop_board(0x%x)", macinfo);
#endif
	(void) elx_set_imask(handle, port, 0, ELINTR_DISABLE);

	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_STAT_DISABLE, 0));
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_RX_DISABLE, 0));
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_TX_DISABLE, 0));

	elx_enable_bus(macinfo, 0);

	if (elxp->elx_rcvbuf != NULL) {
		freeb(elxp->elx_rcvbuf);
		ELX_LOG(elxp->elx_rcvbuf, ELXLOG_FREE);
		elxp->elx_rcvbuf = NULL;
	}

	elxp->elx_flags &= ~ELF_DMA_XFR;

	return (0);
}

/*
 *  elx_saddr() -- set the physical network address on the board.
 */
int
elx_saddr(gld_mac_info_t *macinfo)
{
	int i;
	ushort window;
	register int port = macinfo->gldm_port;
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_saddr(0x%x)", macinfo);
#endif
	SWTCH_WINDOW(port, 2, window);
	for (i = 0; i < ETHERADDRL; i++)
		DDI_OUTB(port + ELX_PHYS_ADDR + i, macinfo->gldm_macaddr[i]);
	RESTORE_WINDOW(port, window, 2);
	return (0);
}

/*
 *  elx_dlsdmult() -- set (enable) or disable a multicast address
 *
 *  Program the hardware to enable/disable the multicast address
 *  in "mcast".  Enable if "op" is non-zero, disable if zero.
 */
static int
elx_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

#ifdef lint
	mcast = mcast;
#endif
#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_dlsdmult(0x%x, %s)", macinfo,
				op ? "ON" : "OFF");
#endif

	if (op) {
		elxp->elx_rxbits |= ELRX_MULTI_ADDR;
		elxp->elx_mcount++;
	} else {
		if (--elxp->elx_mcount == 0)
			elxp->elx_rxbits &= ~ELRX_MULTI_ADDR;
	}
	DDI_OUTW(macinfo->gldm_port + ELX_COMMAND, COMMAND(ELC_SET_RX_FILTER,
					    elxp->elx_rxbits));
	return (0);
}

/*
 * elx_prom() -- set or reset promiscuous mode on the board
 *
 *  Program the hardware to enable/disable promiscuous mode.
 *  Enable if "on" is non-zero, disable if zero.
 */
static int
elx_prom(gld_mac_info_t *macinfo, int on)
{
	register int port = macinfo->gldm_port;
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_prom(0x%x, %s)", macinfo,
				on ? "ON" : "OFF");
#endif
	if (on)
	    elxp->elx_rxbits = ELRX_PROMISCUOUS;
	else
	    elxp->elx_rxbits = (elxp->elx_mcount > 0) ?
	    (ELRX_MULTI_ADDR|ELRX_IND_ADDR) : ELRX_INIT_RX_FILTER;

	elx_poll_cip(elxp, port, ELX_CIP_RETRIES, 0);
	DDI_OUTW(port + ELX_COMMAND,
	    COMMAND(ELC_SET_RX_FILTER, elxp->elx_rxbits));
	elx_poll_cip(elxp, port, ELX_CIP_RETRIES, 0);
	return (0);
}

/*
 * elx_gstat() -- update statistics
 *
 *  GLD calls this routine just before it reads the driver's statistics
 *  structure.  If your board maintains statistics, this is the time to
 *  read them in and update the values in the structure.  If the driver
 *  maintains statistics continuously, this routine need do nothing.
 */
static int
elx_gstat(gld_mac_info_t *macinfo)
{
	register int port = macinfo->gldm_port;
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;
	ushort window;

	SWTCH_WINDOW(port, 6, window);

	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_STAT_DISABLE, 0));

	(void) DDI_INW(port + ELX_STATUS);

	elxp->elx_no_carrier += DDI_INB(port + ELX_CARRIER_LOST);
	elxp->elx_no_sqe += DDI_INB(port + ELX_NO_SQE);
	(void) DDI_INB(port + ELX_TX_FRAMES);
	(void) DDI_INB(port + ELX_RX_FRAMES);
	(void) DDI_INW(port + ELX_RX_BYTES);
	(void) DDI_INW(port + ELX_TX_BYTES);

	macinfo->gldm_stats.glds_defer += DDI_INB(port + ELX_TX_DEFER);
	macinfo->gldm_stats.glds_xmtlatecoll +=
			DDI_INB(port + ELX_TX_LATE_COLL);
	macinfo->gldm_stats.glds_collisions +=
			DDI_INB(port + ELX_TX_MULT_COLL) +
				DDI_INB(port + ELX_TX_ONE_COLL);
	macinfo->gldm_stats.glds_missed += DDI_INB(port + ELX_RX_OVERRUN);

	if (elxp->elx_speed == 100) {
		SET_WINDOW(port, 4);
		elxp->elx_bad_ssd += DDI_INB(port + ELX_BAD_SSD);
	}

	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_STAT_ENABLE, 0));

	if (elxp->elx_speed == 100) {
		RESTORE_WINDOW(port, window, 4);
	} else {
		RESTORE_WINDOW(port, window, 6);
	}

	return (0);
}

static void
elx_restart_tx(gld_mac_info_t *macinfo)
{
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;
	int port = macinfo->gldm_port;

	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_TX_RESET, 0));
	elx_poll_cip(elxp, port, ELX_CIP_RETRIES, 1);
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_TX_ENABLE, 0));

	if (elxp->elx_flags & ELF_DMA_SEND)
		elxp->elx_flags &= ~ELF_DMA_SEND;
}

static void
elx_restart_rx(gld_mac_info_t *macinfo)
{
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	int port = macinfo->gldm_port;
	ddi_acc_handle_t handle = elxp->io_handle;

	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_RX_RESET, 0));
	elx_poll_cip(elxp, port, ELX_CIP_RETRIES, 1);
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_RX_ENABLE, 0));
	DDI_OUTW(port + ELX_COMMAND,
			COMMAND(ELC_SET_RX_FILTER, elxp->elx_rxbits));

	if (elxp->elx_rcvbuf != NULL) {
		freeb(elxp->elx_rcvbuf);
		ELX_LOG(elxp->elx_rcvbuf, ELXLOG_FREE);
		elxp->elx_rcvbuf = NULL;
	}

	if (ELX_CAN_DMA(elxp) == NULL)
		DDI_OUTW(port + ELX_COMMAND,
			COMMAND(ELC_SET_RX_EARLY, elxp->elx_earlyrcv));

	if (elxp->elx_flags & ELF_DMA_RECV)
		elxp->elx_flags &= ~ELF_DMA_RECV;
}

/*
 *  elx_send () -- send a packet
 *
 *  Called when a packet is ready to be transmitted. A pointer to an M_PROTO
 *  or M_PCPROTO message that contains the packet is passed to this routine
 *  as a parameter. The complete LLC header is contained in the message block's
 *  control information block, and the remainder of the packet is contained
 *  within the M_DATA message blocks linked to the first message block.
 *
 *  This routine may NOT free the packet.
 */
static int
elx_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	ushort diag, oimask;
	ushort window, txstat;
	int result = 1;		/* assume failure */
	int port = macinfo->gldm_port;
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

#ifdef ELXDEBUG
	if (elxdebug & ELXSEND)
		cmn_err(CE_CONT, "elx_send(0x%x, 0x%x)", macinfo, mp);
#endif

	SWTCH_WINDOW(port, 5, window);

	oimask = DDI_INW(port + ELX_INTR_MASK);
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_SET_INTR, 0));

	SET_WINDOW(port, 4);
	diag = DDI_INW(port + ELX_NET_DIAGNOSTIC);

	SET_WINDOW(port, 1);
	if (!(diag & ELD_NET_TX_ENABLED)) {
#if defined(ELXDEBUG)
		if (elxdebug & ELXSEND) {
			cmn_err(CE_WARN,
				"elx: transmit disabled! netdiag=%x, txstat=%x",
				diag, DDI_INB(port + ELX_TX_STATUS));
		}
#endif
		DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_TX_ENABLE, 0));
		macinfo->gldm_stats.glds_errxmt++;
	}

	while ((txstat = DDI_INB(port + ELX_TX_STATUS)) & ELTX_COMPLETE) {
		DDI_OUTB(port + ELX_TX_STATUS, txstat);
		if (txstat & ELTX_ERRORS) {
			macinfo->gldm_stats.glds_errxmt++;
			if (txstat & (ELTX_JABBER|ELTX_UNDERRUN)) {
				elx_restart_tx(macinfo);
				macinfo->gldm_stats.glds_underflow++;
			}
			if (txstat & ELTX_MAXCOLL)
				macinfo->gldm_stats.glds_excoll++;
			DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_TX_ENABLE, 0));
		}
	}

	result = elx_send_msg(macinfo, port, mp);

	txstat = DDI_INB(port + ELX_TX_STATUS);
	if (txstat & (ELTX_UNDERRUN|ELTX_JABBER)) {
		cmn_err(CE_WARN, "elx: transmit or jabber underrun: %b", txstat,
		"\020\2RECLAIM\3STATOFL\4MAXCOLL\5UNDER\6JABBER\7INTR\10CPLT");
		elx_restart_tx(macinfo);
		result = 1;	/* force a retry */
		if (txstat & ELTX_UNDERRUN)
			macinfo->gldm_stats.glds_underflow++;
		else
			macinfo->gldm_stats.glds_errxmt++;
	}

	SET_WINDOW(port, 5);
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_SET_INTR, oimask));

	RESTORE_WINDOW(port, window, 5);
	return (result);
}

/*
 * elx_discard(macinfo, port)
 *	discard top packet and cleanup any partially received buffer
 */
void
elx_discard(gld_mac_info_t *macinfo, int port)
{
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_RX_DISCARD_TOP, 0));
	if (elxp->elx_rcvbuf) {
		freeb(elxp->elx_rcvbuf);
		ELX_LOG(elxp->elx_rcvbuf, ELXLOG_FREE);
		elxp->elx_rcvbuf = NULL;
		elxp->elx_flags &= ~(ELF_DMA_RECV|ELF_DMA_RCVBUF);
	}
	elx_poll_cip(elxp, port, ELX_CIP_RETRIES, 1);
	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_ACK_INTR, ELINTR_RX_COMPLETE));
}

/*
 * Set the early receive threshold.
 */
void
elx_set_ercv(gld_mac_info_t *macinfo)
{
	int latency;
	int port = macinfo->gldm_port;
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

	latency = elxp->elx_latency;

	elxp->elx_earlyrcv = ELX_EARLY_RECEIVE - (latency * 4);

	if (elxp->elx_earlyrcv <= (ELXMAXPKT / 2)) {
#if defined(ELXDEBUG)
		if (elxdebug && latency > 300)
			cmn_err(CE_WARN, "elx%d: high latency %d us",
			    macinfo->gldm_ppa, ((latency * 32) + 9) / 10);
#endif
		elxp->elx_earlyrcv = (ELXMAXPKT / 2) + 4;
	}

	DDI_OUTW(port + ELX_COMMAND,
			COMMAND(ELC_SET_RX_EARLY, elxp->elx_earlyrcv));
}

/*
 *  elxintr () -- interrupt from board to inform us that a receive or
 *  transmit has completed.
 */
static u_int
elxintr(gld_mac_info_t *macinfo)
{
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	register int port = macinfo->gldm_port;
	ushort window, status, value;
	ddi_acc_handle_t handle = elxp->io_handle;

	/*
	 * This check is added to handle an undocumented error
	 * which prevents accessing the adapters registers.
	 * The adapter appears to be dead for a short
	 * period of time. This error manifests itself on MP
	 * machines in particular.
	 * We are not sure at this time [4/4/95] whether it is
	 * really necessary to restart the adapter to recover.
	 */

	if ((status = DDI_INW(port + ELX_STATUS)) == 0xffff) {
		(void) elx_restart(macinfo);
		return (DDI_INTR_UNCLAIMED);
	}

#ifdef ELXDEBUG
	if (elxdebug & ELXINT)
		cmn_err(CE_CONT, "elxintr(0x%x) IntSts=%x\n", macinfo, status);
#endif

	if (!(status & ELINTR_LATCH))
		return (DDI_INTR_UNCLAIMED);

	while (status & ELINTR_LATCH) {
		DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_SET_INTR, 0));

		/* Acknowledge interrupt latch */
		DDI_OUTW(port + ELX_COMMAND,
				COMMAND(ELC_ACK_INTR, ELINTR_LATCH));

		macinfo->gldm_stats.glds_intr++;

		SWTCH_WINDOW(port, 1, window);

#if defined(ELXDEBUG)
		if (elxdebug & ELXINT) {
			SET_WINDOW(port, 4);
			value = DDI_INW(port + ELX_NET_DIAGNOSTIC);
			cmn_err(CE_CONT, "\tnetdiag=%x\n", value);
			SET_WINDOW(port, 1);
			value = DDI_INW(port + ELX_RX_STATUS);
			cmn_err(CE_CONT, "\trxstatus=%x\n", value);
		}
#endif
		if (status & ELINTR_INTR_REQUESTED) {
			DDI_OUTW(port + ELX_COMMAND,
			    COMMAND(ELC_ACK_INTR, ELINTR_INTR_REQUESTED));
			if (!elxp->elx_latency) {
				elxp->elx_latency = DDI_INB(port + ELX_TIMER);
				if (!ELX_CAN_DMA(elxp))
					elx_set_ercv(macinfo);
			}
		}

		if (status & ELINTR_UPDATE_STATS) {
			DDI_OUTW(port + ELX_COMMAND,
			    COMMAND(ELC_ACK_INTR, ELINTR_UPDATE_STATS));
			(void) elx_gstat(macinfo);
		}

		if (status & ELINTR_ADAPT_FAIL)
			elx_ad_failure(macinfo);

		if (status & ELINTR_DMA_COMPLETE)
			elx_dma_intr(macinfo, elxp, port);

		if (status & (ELINTR_RX_COMPLETE|ELINTR_RX_EARLY))
			elx_getp(macinfo);

		if (!(status & ELINTR_TX_COMPLETE) &&
		    DDI_INB(port + ELX_TX_STATUS) & ELTX_COMPLETE) {
			status |= ELINTR_TX_COMPLETE;
		}
		if (status & ELINTR_TX_COMPLETE) {
			while ((value = DDI_INB(port + ELX_TX_STATUS)) &
			    ELTX_COMPLETE) {
				DDI_OUTB(port + ELX_TX_STATUS, value);
				if (value & ELTX_ERRORS) {
					macinfo->gldm_stats.glds_errxmt++;
					if (value&(ELTX_JABBER|ELTX_UNDERRUN)) {
						elx_restart_tx(macinfo);
						macinfo->gldm_stats
						    .glds_underflow++;
					}
#if defined(ELXDEBUG)
					if (elxdebug & ELXINT &&
					    value & ELTX_STAT_OVERFLOW) {
						cmn_err(CE_WARN,
						    "elx%d: tx stat overflow",
						    macinfo->gldm_ppa);
					}
#endif
				}
				if (value & ELTX_MAXCOLL) {
					macinfo->gldm_stats.glds_excoll++;
				}
				if (value & (ELTX_MAXCOLL|ELTX_ERRORS)) {
					DDI_OUTW(port + ELX_COMMAND,
					    COMMAND(ELC_TX_ENABLE, 0));
				}
				status = DDI_INW(port + ELX_STATUS);
			}
		}

		/*
		 * error detection and recovery for strange conditions
		 */
		SET_WINDOW(port, 4);
		value = DDI_INW(port + ELX_NET_DIAGNOSTIC);
		if (!(value & ELD_NET_RX_ENABLED)) {
			(void) elx_restart(macinfo);
		}
		value = DDI_INW(port + ELX_FIFO_DIAGNOSTIC);
		if (value & ELD_FIFO_RX_OVER) {
#if defined(ELXDEBUG)
			if (elxdebug & ELXERRS) {
				cmn_err(CE_WARN, "elx%d: rx fifo over",
					macinfo->gldm_ppa);
			}
#endif
			SET_WINDOW(port, 1);
			(void) elx_restart(macinfo);
		}
		SET_WINDOW(port, 5);
		value = DDI_INW(port + ELX_RX_FILTER) & 0xF;
		if (value != elxp->elx_rxbits) {
			cmn_err(CE_WARN, "elx%d: rx filter %x/%x",
				macinfo->gldm_ppa, elxp->elx_rxbits, value);
			(void) elx_restart(macinfo);
		}

		status = DDI_INW(port + ELX_STATUS);
	}

	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_SET_INTR,
					ELINTR_DEFAULT(ELX_CAN_DMA(elxp))));

	RESTORE_WINDOW(port, window, 1);

	return (DDI_INTR_CLAIMED);	/* Indicate it was our interrupt */
}

/*
 *  elx_ad_failure() -- error handling for elxintr adapter failure
 */
static	void
elx_ad_failure(gld_mac_info_t *macinfo)
{
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	register int port = macinfo->gldm_port;
	ddi_acc_handle_t handle = elxp->io_handle;
	int x;

	DDI_OUTW(port + ELX_COMMAND,
	    COMMAND(ELC_ACK_INTR, ELINTR_ADAPT_FAIL));
	SET_WINDOW(port, 4);
	x = DDI_INW(port + ELX_FIFO_DIAGNOSTIC);
	if (x & ELD_FIFO_TX_OVER) {
		    cmn_err(CE_WARN,
			"Adapter failed: fifo diag %x", x);
		(void) elx_restart_tx(macinfo);
	}
	SET_WINDOW(port, 1);
	x &= ~(ELD_FIFO_RX_NORM | ELD_FIFO_RX_STATUS);

	if (x) {
		/*
		 * Only reset if it is a real error
		 * According to spec (12/93), receive
		 * underruns can be spurious and should be
		 * essentially ignored -- not an error
		 * see page 10-2 for specifics.
		 * only do a reset
		 */
		if ((x & 0xfc00) & ~ELD_FIFO_RX_UNDER) {
		    cmn_err(CE_WARN,
			"Adapter failed: fifo diag %b", x,
			"\020\001TXBC\002TXBF\003TXBFC"
			"\004TXBIST\005RXBC\006RXBF\007RXBFC"
			"\010RXBIST\013TXO\014RXO\015RXSO"
			"\016RXU\017RES\020RXR");
			(void) elx_restart(macinfo);
		} else if (x & ELD_FIFO_RX_UNDER)
			(void) elx_restart_rx(macinfo);
		(void) DDI_INW(port + ELX_STATUS);
	}
}


/*
 * Conversion table between old and new RX error conditions.
 */
ushort
elx_rxetab[6] = {
	0x1,	/* ver 0 ELRX_OVERRUN	= 0x0 */
	0x10,	/* ver 0 ELRX_OVERSIZE	= 0x1 */
	0x80,	/* ver 0 ELRX_DRIBBLE	= 0x2 */
	0x2,	/* ver 0 ELRX_RUNT	= 0x3 */
	0x4,	/* ver 0 ELRX_FRAME	= 0x4 */
	0x8	/* ver 0 ELRX_CRC	= 0x5 */
};

/*
 * elx_getp (macinfo)
 *	get packet from the receive FIFO
 *	for performance, we allow an early receive interrupt
 *	and keep track of partial receives so we can free bytes
 *	fast enough to not lose packets (at least not as many as
 *	not doing this will result in)
 */
static void
elx_getp(gld_mac_info_t *macinfo)
{
	int ver;
	int err;
	int port;
	int len, bplen;
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ushort value, window, status;
	ddi_acc_handle_t handle = elxp->io_handle;

	port = macinfo->gldm_port;

	SWTCH_WINDOW(port, 1, window);

	ver = NEW_ELX(elxp);

	while ((status = DDI_INW(port + ELX_STATUS)) &
	    (ELINTR_RX_COMPLETE|ELINTR_RX_EARLY)) {
		mblk_t *mp;

		/*
		 * Acknowledge an early receive interrupt.  In case
		 * of a completion, the interrupt will be acked by
		 * the discarding the packet from the receive FIFO.
		 */
		if (status & ELINTR_RX_EARLY)
			DDI_OUTW(port + ELX_COMMAND,
				COMMAND(ELC_ACK_INTR, ELINTR_RX_EARLY));

		value = DDI_INW(port + ELX_RX_STATUS);

		if (value == 0xFFFF) {
			/*
			 * must be an MP system bashing us pretty badly
			 * do a hardware reset and then exit.  We lose
			 * the packet and recover.  This only happens
			 * at startup so no problem.
			 */
			(void) elx_restart(macinfo);
			RESTORE_WINDOW(port, window, 1);
			return;
		} else if (value & ELRX_ERROR) {
			int discard;

			discard = 1;
			err = ELX_GET_RXERR(ver, value, port);
			err = ELRX_GET_ERR(ver, err);

			if (ver) {
				/* 3c59x boards can report multiple errors */
				if (err & ELRX_OVERRUN)
					macinfo->gldm_stats.glds_overflow++;
				if (err & ELRX_RUNT)
					macinfo->gldm_stats.glds_short++;
				if (err & ELRX_FRAME)
					macinfo->gldm_stats.glds_frame++;
				if (err & ELRX_CRC)
					macinfo->gldm_stats.glds_crc++;
				if (err == ELRX_DRIBBLE)
					discard = 0;

			} else {
				/* 3c5x9 boards report single errors only */
				switch (err) {
				case ELRX_OVERRUN:
					macinfo->gldm_stats.glds_overflow++;
					break;
				case ELRX_RUNT:
					macinfo->gldm_stats.glds_short++;
					break;
				case ELRX_FRAME:
					macinfo->gldm_stats.glds_frame++;
					break;
				case ELRX_CRC:
					macinfo->gldm_stats.glds_crc++;
					break;
				case ELRX_OVERSIZE:
					break;
				case ELRX_DRIBBLE:
					discard = 0;
					break;
				default:
					/* don't know what it is so discard */
					cmn_err(CE_WARN,
					"elx: unknown receive error 0x%x", err);
					break;
				}
			}

			if (discard) {
				elx_discard(macinfo, port);
				continue;
			}
		}
		/*
		 * if a receive dma is already in progress let
		 * it complete before retrieving the next
		 * message.
		 */
		if (ELX_CAN_DMA(elxp) &&
			(status & ELINTR_DMA_INPROGRESS) &&
			(elxp->elx_flags & ELF_DMA_RECV))
				continue;

		len = ELRX_GET_LEN(ver, value);

		if ((mp = elxp->elx_rcvbuf) == NULL)
			bplen = 0;
		else
			bplen = mp->b_wptr - mp->b_rptr;

		/*
		 * Make sure we don't have a jumbogram.
		 */
		if (bplen + len > ELXMAXFRAME) {
#if defined(ELXDEBUG)
			if (elxdebug & ELXRECV)
				cmn_err(CE_WARN, "elx%d: jumbogram received"
				    " (%d) bytes from %s",
				    macinfo->gldm_ppa, MLEN(mp) + len,
				    ether_sprintf((struct ether_addr *)
					(mp->b_rptr + 6)));
#endif
			elx_discard(macinfo, port);
			macinfo->gldm_stats.glds_errrcv++;
			continue;
		} else if (mp == NULL) {
			/*
			 * 6 bytes extra are allocated to
			 * allow for pad bytes at the end
			 * of the frame and to allow aligning
			 * the data portion of the packet on
			 * a 4-byte boundary.  They shouldn't
			 * be necessary but don't hurt.
			 */
			if (status & ELINTR_RX_EARLY)
				bplen = ELXMAXFRAME + 6;
			else
				bplen = len + 6;

			if ((mp = allocb(bplen, BPRI_MED)) == NULL) {
				elx_discard(macinfo, port);
				macinfo->gldm_stats.glds_norcvbuf++;
				continue;
			} else {
				/*
				 * 32-bit aligned but want 16-bit to
				 * put the data on a 32-bit boundary
				 */
				mp->b_rptr += 2;
				mp->b_wptr = mp->b_rptr;
				elxp->elx_rcvbuf = mp;
			}
			ELX_LOG(mp, ELXLOG_ALLOC);
		}

		if (status & ELINTR_RX_EARLY) {
			ELX_LOG(mp, ELXLOG_PIO_IN);
			elx_pio_recv(handle, port, (unchar *)mp->b_wptr, len);
			mp->b_wptr += len;
		} else {
			err = elx_recv_msg(elxp, port, mp, len);

			/* dma receive in progress */
			if (elxp->elx_flags & ELF_DMA_RECV)
				break;

			if (!err) {
				ELX_LOG(mp, ELXLOG_SEND_UP);
				gld_recv(macinfo, mp);
				elxp->elx_rcvbuf = NULL;
			}

			elx_discard(macinfo, port);

		}

		if (ELX_CAN_DMA(elxp) == 0)
			DDI_OUTW(port + ELX_COMMAND,
				COMMAND(ELC_SET_RX_EARLY, elxp->elx_earlyrcv));
	}

	RESTORE_WINDOW(port, window, 1);
}

/*
 *  elx_restart () -- reset the board to initial state; save the machine
 *  address and restore it afterwards. Then start the board receiving
 *  and allow transmits.
 */
int
elx_restart(gld_mac_info_t *macinfo)
{
	(void) elx_reset(macinfo);
	(void) elx_start_board(macinfo);
	return (0);
}

/*
 * elx_msdelay (ms)
 *	delay in terms of milliseconds.
 */
void
elx_msdelay(int ms)
{
	drv_usecwait(1000 * ms);
}


/*
 * elx_mca_probe(slot)
 */
static int
elx_mca_probe(ddi_acc_handle_t handle, int slot)
{
	unchar mcapos[6];
	int port;

	if (slot < 1  || slot > ELX_MCA_MAXSLOT)
		return (0);

	/* get the MCA POS registers */
	elx_read_pos(handle, slot, mcapos);

	if (!elx_verify_id(*(ushort *)mcapos))
		return (0);
	/* only consider enabled boards */
	if (!(mcapos[ELX_POS_CDEN] & ELX_CDEN))
		return (0);
	port = mcapos[ELX_POS_IO_XCVR] & 0x3f;
	port = ELXPOS_ADDR(port);
	SET_WINDOW(port, 0);

	/* although we should have a board, check further */
	if (DDI_INW(port + ELX_MFG_ID) != EL_3COM_ID)
		return (0);

	return (1);
}


/*
 * elx_eisa_probe(ioaddr)
 */
static int
elx_eisa_probe(ddi_acc_handle_t handle, int ioaddr)
{
	KEY_MASK key;
	NVM_SLOTINFO *nvm;
	caddr_t data;
	int foundboard = 0;
	ushort value;

	*(int *)&key = 0;
	key.slot = EISA_SLOT;

	if ((data = (caddr_t)kmem_zalloc(ELX_MAX_EISABUF, KM_NOSLEEP)) == NULL)
		return (0);

	if (eisa_nvm(data, key, ioaddr/MIN_EISA_ADDR)) {
		nvm = (NVM_SLOTINFO *)(data + sizeof (short));
		if (gld_check_boardid(nvm, EL_3COM_ID)) {
			value = DDI_INW(ioaddr + 0xc80 + ELX_PRODUCT_ID);
			if (elx_verify_id(value))
				foundboard++;
		}
	}
	kmem_free(data, ELX_MAX_EISABUF);

	return (foundboard);
}


/*
 * elx_isa_probe(ioaddr)
 */
static int
elx_isa_probe(ddi_acc_handle_t handle, int ioaddr)
{
	int i, found_board = 0;
	static int nboards;
	static short boards[ELX_MAX_ISA];
	if (nboards == 0) {
		int idport = ELX_ID_PORT;
		nboards = elx_isa_init(handle, idport, boards,
					ELX_MAX_ISA);
	}
	if (nboards == 0)
		return (0);

	for (i = 0; i < nboards; i++) {
		int port, value;
		port = boards[i];
		if (port != ioaddr)
			continue;
		SET_WINDOW(port, 0);
		value = DDI_INW(port + ELX_PRODUCT_ID);
		if (!elx_verify_id(value))
			break;
		found_board++;
	}
	return (found_board);
}

/*
 * elx_isa_idseq (id)
 *	write the 3C509 ID sequence to the specified ID port
 *	in order to put all ISA boards into the ID_CMD state.
 */
static void
elx_isa_idseq(ddi_acc_handle_t handle, int id)
{
	int cx, al;

	/* get the boards' attention */
	DDI_OUTB(id, ELISA_RESET);
	DDI_OUTB(id, ELISA_RESET);
	/* send the ID sequence */
	for (cx = ELISA_ID_INIT, al = ELISA_ID_PATLEN; cx > 0; cx--) {
		DDI_OUTB(id, al);
		al <<= 1;
		if (al & 0x100)
			al = (al ^ ELISA_ID_OPAT) & 0xFF;
	}
}

/*
 * elx_isa_init (id)
 *	initialize ISA boards (3c509) at the specified port ID
 *	this will get called once to find boards and
 *	activate them then return the list of boards found
 */
static int
elx_isa_init(ddi_acc_handle_t handle, int id, short *boards, int max)
{
	int value;
	int nboards;

	if (id != 0) {
		elx_isa_idseq(handle, id);
		DDI_OUTB(id, ELISA_SET_TAG(0));	/* reset any tags to zero */

		/*
		 * The following global reset is necessary because it
		 * forces the state of the boards to be the same after
		 * <Ctrl-Alt-Del> as it is after RESET.  But it also
		 * means that we have to initialize everything from
		 * scratch.  This is particularly annoying on ISA boards
		 * in EISA mode because we have to be very careful setting
		 * up the interrupts to avoid getting spurious ones on
		 * the IRQ that the boards assume at reset as opposed to
		 * their EISA-configured IRQ.
		 */
		/* make all boards consistent */
		DDI_OUTB(id, ELISA_GLOBAL_RESET);
		elx_msdelay(40);
	}

	for (nboards = 0; nboards < max; nboards++) {
		elx_isa_idseq(handle, id);
		/* now we have board(s) in ID_CMD state */
		/* keep only non-tagged boards */
		DDI_OUTB(id, ELISA_TEST_TAG(0));

		/* make sure all contention is done */

		value = elx_isa_read_prom(handle, id, EEPROM_PHYS_ADDR);
		value = elx_isa_read_prom(handle, id, EEPROM_PHYS_ADDR+1);
		value = elx_isa_read_prom(handle, id, EEPROM_PHYS_ADDR+2);

		/* read in the product ID since that should force things */
		value = elx_isa_read_prom(handle, id, EEPROM_PROD_ID);

		/* if it doesn't match, then no more boards */
		if (!elx_verify_id(value))
			break;

		value = elx_isa_read_prom(handle, id, EEPROM_ADDR_CFG);

		/* now have info, so tag board */
		DDI_OUTB(id, ELISA_SET_TAG(nboards+1));
		boards[nboards] = (value&0x1F)*0x10 + 0x200;
	}

	elx_isa_idseq(handle, id);
	elx_isa_sort(boards, nboards);
	DDI_OUTB(id, ELISA_ACTIVATE);
	return (nboards);
}

/*
 * elx_isa_sort (boards, nboards)
 *	need to sort boards by I/O address to avoid having
 *	boards found in random order.
 */
static void
elx_isa_sort(short *boards, int nboards)
{
	int i, j;
	short tmp;

	if (nboards == 1)
		return;

	for (i = 0; i < nboards; i++)
		for (j = 0; j < i; j++)
			if (boards[j] > boards[i]) {
				tmp = boards[i];
				boards[i] = boards[j];
				boards[j] = tmp;
			}
}

/*
 * elx_isa_read_prom (handle, id, addr)
 *	read the board's PROM at PROM address addr using
 *	the id port.  This is an ISA only function.
 */
static int
elx_isa_read_prom(ddi_acc_handle_t handle, int id, int addr)
{
	register int i, value;

	/* select EEPROM to resolve contention and get I/O port */
	DDI_OUTB(id, ELISA_READEEPROM(addr));

	/* wait for the EEPROM register to settle */
	drv_usecwait(ELISA_READ_DELAY*10);
	/*
	 * now read the value in, 1 bit at a time
	 * Note that this forces the boards to resolve their contention.
	 */
	for (i = 16, value = 0; i > 0; i--) {
		value = (value << 1) | (DDI_INB(id) & 1);
	}
	return (value);
}

/*
 * elx_read_pos (slot, pos)
 *	given a slot number, read all 6 bytes of POS information
 *	This is an MCA only function.
 */
static void
elx_read_pos(ddi_acc_handle_t handle, int slot, unsigned char *pos)
{
	int i;
	DDI_OUTB(ELX_ADAP_ENABLE, (slot-1) + ELPOS_SETUP);
	for (i = 0; i < 6; i++)
		pos[i] = DDI_INB(ELX_POS_REG_BASE + i);
	DDI_OUTB(ELX_ADAP_ENABLE, ELPOS_DISABLE);
}

static int
elx_read_nvm(int slot, ushort *media)
{
	KEY_MASK key;
	NVM_SLOTINFO *nvm;
	NVM_FUNCINFO *func;
	caddr_t data;
	int i, nvm_irq = -1;

	*(int *)&key = 0;
	key.slot = EISA_SLOT;

	if ((data = (caddr_t)kmem_zalloc(ELX_MAX_EISABUF, KM_NOSLEEP)) == NULL)
		return (-1);

	if (eisa_nvm(data, key, slot) == 0)
		goto out;

	nvm = (NVM_SLOTINFO *)(data + sizeof (short));
	if (!gld_check_boardid(nvm, EL_3COM_ID))
		goto out;

	for (i = 0, func = (NVM_FUNCINFO *)(nvm+1); i < nvm->functions;
	    i++, func++) {

	    /* check all functions present and extract info */

	    if (func->fib.init) {
		int more;
		NVM_INIT *init = (NVM_INIT *)(func->un.r.init);
		do {
			more = init->more;
			switch (init->type) {
			case NVM_IOPORT_BYTE:
				if (init->mask)
					init = (NVM_INIT *)(((caddr_t)init)+5);
				else
					init = (NVM_INIT *)(((caddr_t)init)+3);
				break;
			case NVM_IOPORT_WORD:
				/* extract the media value */
				if ((init->port&0xf) == ELX_ADDRESS_CFG) {
					if (media)
						*media = init->un.word_vm.value;
				}
				if (init->mask)
					init = (NVM_INIT *)(((caddr_t)init)+7);
				else
					init = (NVM_INIT *)(((caddr_t)init)+5);
				break;
			case NVM_IOPORT_DWORD:
				if (init->mask)
					init = (NVM_INIT *)(((caddr_t)init)+11);
				else
					init = (NVM_INIT *)(((caddr_t)init)+7);
				break;
			}
		} while (more);
	    }
	    if (func->fib.irq)
		nvm_irq = func->un.r.irq[0].line;
	    }
out:
	kmem_free(data, ELX_MAX_EISABUF);
	return (nvm_irq);
}

static ushort
elx_read_prom(ddi_acc_handle_t handle, int port, int promaddr)
{
	ushort value, window;
	int i;

	SWTCH_WINDOW(port, 0, window);

	for (i = 2000; i > 0 &&
	    (((value = DDI_INW(port + ELX_EEPROM_CMD)) & EEPROM_BUSY) ==
	    EEPROM_BUSY); i--)
		elx_msdelay(1);

	DDI_OUTW(port + ELX_EEPROM_CMD, EEPROM_CMD(EEPROM_READ, promaddr));

	for (i = 2000; i > 0 &&
	    (((value = DDI_INW(port + ELX_EEPROM_CMD)) & EEPROM_BUSY) ==
	    EEPROM_BUSY); i--)
		elx_msdelay(1);

	value = DDI_INW(port + ELX_EEPROM_DATA);

	RESTORE_WINDOW(port, window, 0);

	return (value);
}

/*
 * elx_regs_map_setup (devinfo)
 * 	setup access to the board ioaddr
 */
static int
elx_regs_map_setup(dev_info_t *devinfo, int bustype, int *ioaddr,
	ddi_acc_handle_t *handle)
{
	static ddi_device_acc_attr_t accattr = {
		DDI_DEVICE_ATTR_V0,
		DDI_NEVERSWAP_ACC,
		DDI_STRICTORDER_ACC,
	};
#if defined PCI_DDI_EMULATION
	int *regarr, arrsize;

	if (bustype != ELX_PCI) {
		if (ddi_prop_op(DDI_DEV_T_ANY, devinfo, PROP_LEN_AND_VAL_ALLOC,
			    (DDI_PROP_DONTPASS|DDI_PROP_CANSLEEP), "reg",
			    (caddr_t) &regarr, &arrsize)
			!= DDI_PROP_SUCCESS) {
			cmn_err(CE_WARN,
			"elx: base address not specified as reg property");
			return (DDI_FAILURE);
		}
		if (arrsize == 0) {
			kmem_free(regarr, arrsize);
			cmn_err(CE_WARN, "elx: bad reg property");
			return (DDI_FAILURE);
		}
		*ioaddr = regarr[0];
		kmem_free(regarr, arrsize);
#if defined(ELXDEBUG)
		cmn_err(CE_CONT,
			"elx_regs_map_setup: ioaddr:%x", *ioaddr);
#endif
		return (DDI_SUCCESS);
	}
#endif	/* PCI_DDI_EMULATION */
	return ddi_regs_map_setup(devinfo,
			(uint_t) (bustype == ELX_PCI ? 1 : 0),
			(caddr_t *)ioaddr, (offset_t) 0,
			(offset_t) 0, &accattr, handle);
}

/*
 * elx_regs_map_free (bustype, devinfo)
 * 	release resources to access the board ioaddr
 */
/*ARGSUSED*/
static void
elx_regs_map_free(int bustype, ddi_acc_handle_t *handle)
{
#if defined PCI_DDI_EMULATION
	if (bustype == ELX_PCI)
#endif	/* PCI_DDI_EMULATION */
		ddi_regs_map_free(handle);
}

/*
 * elx_get_bustype (devinfo)
 * 	return the bus type as encoded in the devinfo tree
 */
static int
elx_get_bustype(dev_info_t *devi)
{
	int i;
	char bus_type[16];
	int len;
	static char *prop[3] = {
		"parent-type",
		"device_type",
		"bus-type"
	};

	for (i = 0; i < 3; i++) {
		bus_type[0] = 0;
		len = sizeof (bus_type);
		if (ddi_prop_op(DDI_DEV_T_ANY, devi, PROP_LEN_AND_VAL_BUF, 0,
			prop[i], (caddr_t)bus_type, &len) == DDI_PROP_SUCCESS)
				break;
	}

#ifdef ELXDEBUG
	if (elxdebug & ELXDDIPROBE)
		cmn_err(CE_CONT, " bus:%s ", bus_type);
#endif
	if (strcmp(bus_type, "isa") == 0)
		return (ELX_ISA);
	else if (strcmp(bus_type, "eisa") == 0)
		return (ELX_EISA);
	else if (strcmp(bus_type, "pci") == 0)
		return (ELX_PCI);
	else if (strcmp(bus_type, "mc") == 0)
		return (ELX_MCA);
	else
		return (ELX_NOBUS);
}

static int
elx_get_irq(dev_info_t *devinfo, elx_t *elxp)
{
	int i, nirq, found, index;
	int *irqarr, irqlen;

#if !defined PCI_DDI_EMULATION
	if (elxp->elx_bus == ELX_PCI)
		return (0);
#endif

	if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
	    "intr", (caddr_t)&irqarr, &irqlen) != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN, "elx: interrupts property not found "
		    "in devices property list");
		return (-1);
	}

	index = 0;

	nirq = irqlen / sizeof (int);

	for (i = 1, found = 0; i < nirq; i += 2, index++)
		if (irqarr[i] == elxp->elx_irq) {
			found++;
			break;
		}
	if (!found)
		index = -1;

	kmem_free(irqarr, irqlen);
	return (index);
}

static int
elx_get_ioaddr(dev_info_t *devinfo, int bus, ddi_acc_handle_t *handle)
{
	int port;

	if (elx_regs_map_setup(devinfo, bus, &port, handle) != DDI_SUCCESS)
		port = -1;
	if (bus == ELX_MCA) {
		unchar mcapos[6];

		/* get the MCA POS registers */
		elx_read_pos(*handle, port - ELX_MCA_OFFSET, mcapos);

		port = mcapos[ELX_POS_IO_XCVR] & 0x3f;
		port = ELXPOS_ADDR(port);
	}

	return (port);
}

typedef struct elxprod {
	ushort id;
	ushort mask;
	ushort ver;
} elxprod_t;

elxprod_t elx_ptab[] = {
	0x9050, 0xf0ff, 0,	/* 3C509, 3C509B, 3C579 (ISA/EISA) */
	0x6270, 0xfff0, 0,	/* 3C529 (MCA) */
	0x2059, 0xf0ff, 1,	/* 3C592 (EISA 10) */
	0x7059, 0xf0ff, 1,	/* 3C597 (EISA 10/100) */
	0x5950, 0xffff, 1,	/* 3C595 (PCI 10/100) */
	0x5900, 0xffff, 1,	/* 3C590 (PCI 10) */
	0, 0
};

static void
elx_config(gld_mac_info_t *macinfo)
{
	int port;
	int fpart;
	ushort pid;
	ushort value;
	ushort window;
	elx_t *elxp;
	elxprod_t *p;
	ddi_acc_handle_t handle;

	elxp = (elx_t *)macinfo->gldm_private;
	port = macinfo->gldm_port;
	handle = elxp->io_handle;

	elxp->elx_mac = macinfo;

	SWTCH_WINDOW(port, 0, window);

	pid = elx_read_prom(handle, port, EEPROM_PROD_ID);

	for (p = elx_ptab; p->id; p++)
		if ((pid & p->mask) == p->id)
			break;
	if (p->id == 0)
		return;

	elxp->elx_media = (ushort)-1;

	switch (elxp->elx_bus) {
	case ELX_EISA:
		elxp->elx_irq =
			elx_read_nvm(port / MIN_EISA_ADDR, &(elxp->elx_media));
		break;
	case ELX_PCI:
		elxp->elx_irq = elx_pci_get_irq(macinfo->gldm_devinfo);
		break;
	default:
		elxp->elx_irq =
			((DDI_INW(port + ELX_RESOURCE_CFG) >> 12) & 0xf);
		break;
	}

	elxp->elx_ver = p->ver;
	elxp->elx_caps = elx_read_prom(handle, port, EEPROM_CAPABILITIES);
	elxp->elx_softinfo = elx_read_prom(handle, port, EEPROM_SOFTINFO);
	if (NEW_ELX(elxp))
		elxp->elx_softinfo2 =
				elx_read_prom(handle, port, EEPROM_SOFTINFO2);
	elxp->elx_rxbits = ELRX_INIT_RX_FILTER;

	fpart = 0;

	/*
	 * Determine if we can partition the adapter fifo.
	 * All 3C59x and the 3C509B can be partitioned.
	 */
	if (p->ver)
		fpart = 1;
	else if (p->id == 0x9050) {
		/*
		 * Adapters with this id are either the 3C509, 3C509B,
		 * or 3C579. The 3C509B, which has additional features
		 * (fifo partitioning and autosense) can be distinguished
		 * from the others by examining the ASIC revision level.
		 */
		SWTCH_WINDOW(port, 4, window);
		value = ELX_ASIC_REVISION(DDI_INW(port + ELX_NET_DIAGNOSTIC));
		if (value == ELX_3C5X9B) {
			fpart = 1;
			elxp->elx_flags |= ELF_AUTOSENSE;
		}
		RESTORE_WINDOW(port, window, 4);
	}
	if (fpart) {
		ushort fifosz;

		SWTCH_WINDOW(port, 3, window);

		value = DDI_INL(port + ELX_INTERNAL_CONFIG);
		fifosz = (value & ELICONF_SIZE_MASK);
		if (fifosz == ELICONF_MEM_8K || fifosz == ELICONF_MEM_64K)
			elxp->elx_flags |= ELF_FIFO_PART;

		RESTORE_WINDOW(port, window, 3);
	}

	elx_enable_bus(macinfo, 1);

	if (elxp->elx_flags & ELF_FIFO_PART) {
		SWTCH_WINDOW(port, 3, window);
		value = DDI_INL(port + ELX_INTERNAL_CONFIG);
		DDI_OUTL(port + ELX_INTERNAL_CONFIG,
			ELICONF_SET_PARTITION(value, 1));
		RESTORE_WINDOW(port, window, 3);
	}

	if (elxp->elx_speed == 0)
		elxp->elx_speed = elx_med_sense(macinfo,
			elxp->elx_media, fpart);

	elx_med_set(macinfo);

	DDI_OUTW(port + ELX_COMMAND, COMMAND(ELC_RX_DISABLE, 0));
	RESTORE_WINDOW(port, window, 0);
}

static void
elx_enable_bus(gld_mac_info_t *macinfo, int on)
{
	int port = macinfo->gldm_port;
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	ddi_acc_handle_t handle = elxp->io_handle;

	if (elxp->elx_bus == ELX_PCI)
		elx_pci_enable(macinfo->gldm_devinfo, on);
	else {
		ushort window, value;

		SWTCH_WINDOW(port, 0, window);
		value = DDI_INW(port + ELX_CONFIG_CTL);

		if (on)
			value |= ELCONF_ENABLED;
		else
			value &= ~ELCONF_ENABLED;
		/*
		 * if EISA adapter is disabled the Config Control
		 * register is no longer accessible through
		 * window 0 so must output directly to zC84
		 */
		if (elxp->elx_bus == ELX_EISA)
			DDI_OUTW(port + 0xc80 + ELX_CONFIG_CTL, value);
		else
			DDI_OUTW(port + ELX_CONFIG_CTL, value);

		RESTORE_WINDOW(port, window, 0);
	}
}

int
elx_verify_id(ushort pid)
{
	elxprod_t *p;

	for (p = elx_ptab; p->id; p++)
		if ((pid & p->mask) == p->id)
			break;

	return (p->id);
}

int
elx_dma_attach(dev_info_t *devinfo, gld_mac_info_t *macinfo)
{
	elx_t *elxp = (elx_t *)macinfo->gldm_private;
	long align;
	int rval;

	elxp->pagesize = ddi_ptob(devinfo, 1L);
	/*
	 * Allocate memory for Rx buffers.
	 * Memory needs to be contiguous.
	 */
	rval = ddi_iopb_alloc(devinfo, NULL, elxp->pagesize,
				(caddr_t *)&(elxp->elx_rbuf));
	if (rval == DDI_FAILURE) {
		cmn_err(CE_WARN, "elx_dma_attach");
		cmn_err(CE_WARN, "Could not allocate Rx buffers");
		return (DDI_FAILURE);
	}

	/*
	 * Errata for eisa hw requires dma buffer to be 32 byte aligned.
	 * Additionally, errata for pci hw requires receive dma buffer
	 * to be on odd 32 byte boundary.
	 */
	align = (long)(elxp->elx_rbuf + 31) & ~31;
	if ((align & 0x20) == 0)
		align += 32;
	elxp->elx_dma_rbuf = (caddr_t) align;
	elxp->elx_phy_rbuf = (caddr_t) ELX_KVTOP(align);

	/*
	 * Allocate memory for Tx buffers.
	 * Memory needs to be contiguous.
	 */
	rval = ddi_iopb_alloc(devinfo, NULL, elxp->pagesize,
				(caddr_t *)&(elxp->elx_xbuf));
	if (rval == DDI_FAILURE) {
		ddi_iopb_free(elxp->elx_rbuf);
		cmn_err(CE_WARN, "elx_dma_attach");
		cmn_err(CE_WARN, "Could not allocate Tx buffers");
		return (DDI_FAILURE);
	}
	/*
	 * See note above.  Although nothing was said specifically
	 * about the pci transmit buffer, treat it the same.
	 */
	align = (long)(elxp->elx_xbuf + 31) & ~31;
	if ((align & 0x20) == 0)
		align += 32;
	elxp->elx_dma_xbuf = (caddr_t) align;
	elxp->elx_phy_xbuf = (caddr_t) ELX_KVTOP(align);

	if ((rval = ddi_getprop(DDI_DEV_T_ANY, devinfo, 0,
		"rcv-dma-threshold", -1)) != -1)
		elx_rcv_dma_thresh = rval;

	if ((rval = ddi_getprop(DDI_DEV_T_ANY, devinfo, 0,
		"xmt-dma-threshold", -1)) != -1)
		elx_xmt_dma_thresh = rval;

	return (DDI_SUCCESS);

}
