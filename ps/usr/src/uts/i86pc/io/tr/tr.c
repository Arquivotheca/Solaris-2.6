/*
 * Copyright (c) 1993-1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */
#pragma ident	"@(#)tr.c	1.26	96/06/10 SMI"

/*
 * tr - IBM 16/4 Token Ring Adapter driver
 */

#include "sys/types.h"
#include "sys/errno.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/stropts.h"
#include "sys/stream.h"
#include "sys/kmem.h"
#include "sys/conf.h"
#include "sys/devops.h"
#include "sys/ksynch.h"
#include "sys/dlpi.h"
#include "sys/ethernet.h"
#include "sys/strsun.h"
#include "sys/stat.h"
#include "sys/time.h"
#include "sys/kstat.h"
#include "sys/tr.h"
#include "sys/trreg.h"
#include <sys/debug.h>
#include <sys/byteorder.h>
#include "sys/ddi.h"
#include "sys/sunddi.h"

/*
 * function prototypes, etc.
 */

extern void tr_sr_init(void);
extern struct srtab *tr_sr_create_entry(trd_t *, uchar_t *);
extern struct srtab *tr_sr_lookup(trd_t *, uchar_t *);

extern void tr_ioctl(queue_t *, mblk_t *);

static tr_devinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static tr_probe(dev_info_t *);
static trchkid(dev_info_t *, struct trmmio *);
static trchkbnds(long, paddr_t);
static trchksums(struct trmmio *);
static tr_kstat_update(kstat_t *, int);
static micro_channel(dev_info_t *);
static trattach(queue_t *, mblk_t *, int *, int *);
static trunattach(queue_t *, mblk_t *, int *, int *);
static trxmit(queue_t *, mblk_t *, int *, int *);
static tr_get_phys(queue_t *, mblk_t *, int *, int *);
static tr_test_req_res(queue_t *, mblk_t *, int *, int *, int);
static tr_xid_req_res(queue_t *, mblk_t *, int *, int *, int);

static void traddwaiter(trd_t *, trs_t *);
static void traddholder(trd_t *, trs_t *);
static void trchgmultinoq(trd_t *);
static void trdescchg(trd_t *);
static void tr_form_udata(trs_t *, trd_t *, ushort_t, struct llc_info *,
				mblk_t *);
static void trgetxmitok(trd_t *, long, long, mblk_t *);
static void tr_test_ind_con(trs_t *, trd_t *, struct llc_info *, ushort_t,
				mblk_t *);
static void tr_test_reply(trs_t *, trd_t *, ushort_t, struct llc_info *,
				mblk_t *);
static void trupdatert(trd_t *, ushort_t, struct llc_info *);
static void tr_xid_ind_con(trs_t *, trd_t *, struct llc_info *, ushort_t,
				mblk_t *);
static void tr_xid_reply(trs_t *, trd_t *, ushort_t, struct llc_info *,
				mblk_t *);
static void trremwaiter(trd_t *, trs_t *);
static void trwakewaiters(trd_t *);
static void trwakeholder(trd_t *);
static void trreset(uchar_t *);
static void trcloseboard(trd_t *);
static void tropenboard(trd_t *);
static void tropendone(trd_t *);
static void tropensap(trd_t *, long);
static void tropensapdone(trd_t *);
static void trclosesap(trd_t *, long);
static void trclosesapdone(trd_t *);
static void trrcvmunch(mblk_t *, trd_t *, long, ushort_t, struct llc_info *);
static void trremstr(trs_t *);
static void trrcv(trd_t *);

static ushort_t trswab(ushort_t);
static ushort_t parsellc(mblk_t *, struct llc_info *);

static u_int tr_intr(caddr_t);

static void tr_getpos(int slot, unchar *pos);
static void tr_kstatinit(trd_t *trdp);
static void tr_dl_ioc_hdr_info(queue_t *q, mblk_t *mp);
static void trcompbufs(trd_t *trdp);
static void trremcmds(trd_t *trdp, trs_t *trsp);
static void trnxtcmd(trd_t *trdp);
static void trruncmd(trd_t *trdp);
static void trdqcmd(trd_t *trdp);
static void trqcmd(cmdq_t *cmd);
static int tr_disable_multi(queue_t *q, mblk_t *mp, int *err, int *uerr);
static void trsetfuncdone(trd_t *trdp);
static void trsetfunc(trd_t *trdp);
static void trsend(trd_t *trdp);
static int tr_enable_multi(queue_t *q, mblk_t *mp, int *err, int *uerr);
static int tr_rsrv(queue_t *q);
static int tr_broadcast(struct llc_info *hdr);
static int tr_multicast(struct llc_info *hdr, trs_t *trsp);
static int tr_fromme(struct llc_info *hdr, trd_t *trdp);
static int tr_forme(struct llc_info *hdr, trd_t *trdp);
static void trunbindnoq(trd_t *trdp, long sap);
static int trunbind(queue_t *q, mblk_t *mp, int *err, int *uerr);
static int trbind(queue_t *q, mblk_t *mp, int *err, int *uerr);
static int tr_promiscoff(queue_t *q, mblk_t *mp, int *err, int *uerr);
static int tr_promiscon(queue_t *q, mblk_t *mp, int *err, int *uerr);
static int tr_inforeq(queue_t *q, mblk_t *mp);
static int tr_unitdata(queue_t *q, mblk_t *mp, int *err, int *uerr);
static int tr_reboot(dev_info_t *dip, ddi_reset_cmd_t cmd);
static int tr_open(queue_t *q, dev_t *devp, int flag, int sflag,
    cred_t *cred);
static int tr_close(queue_t *q, int flag, cred_t *credp);
static int tr_wput(queue_t *q, mblk_t *mp);
static int tr_wsrv(queue_t *q);
static int tr_attach(dev_info_t *dip, ddi_attach_cmd_t cmd);
static int tr_detach(dev_info_t *dip, ddi_detach_cmd_t cmd);
static int tr_cmds(queue_t *q, mblk_t *mp, int *err, int *uerr);
static int tr_ismatch(dev_info_t *dip, int irq, caddr_t mmio_phys,
    int base_pio, int mca);
static int tr_devinfo(dev_info_t *dip, ddi_info_cmd_t cmd, void *arg,
    void **result);
static int tr_reopen(trd_t *trdp);
static void tr_reopensap(trd_t *trdp);
static void tr_wdog(caddr_t arg);
static void tr_reinit(trd_t *trdp);

/*
 * the standard streams glue for defining the type of streams entity and the
 * operational parameters.
 */

static struct module_info tr_minfo = {
	TRIDNUM, "tr", 0, TR_DEFMAX, TR_HIWATER, TR_LOWATER
};

static struct qinit tr_rinit = {
	NULL, tr_rsrv, tr_open, tr_close, NULL, &tr_minfo, NULL
};

static struct qinit tr_winit = {
	tr_wput, tr_wsrv, NULL,	NULL, NULL, &tr_minfo, NULL
};

struct streamtab tr_info = {
	&tr_rinit, &tr_winit, NULL, NULL
};

/*
 * loadable module/driver wrapper this allows tr to be unloaded later
 */
#include <sys/modctl.h>

extern struct mod_ops mod_driverops;

/* define the "ops" structure for a STREAMS driver */
DDI_DEFINE_STREAM_OPS(tr_ops, nulldev, tr_probe, \
			tr_attach, tr_detach, tr_reboot, \
			tr_devinfo, D_MP, &tr_info);

/*
 * Module linkage information for the kernel.
 */
static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	"IBM Token Ring 16/4 Driver v1.0",
	&tr_ops,		/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *) &modldrv, NULL
};

int
_init(void)
{
	return (mod_install(&modlinkage));
}

int
_fini(void)
{
	return (mod_remove(&modlinkage));
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*
 * kadb patchable flags
 */
#ifdef TR_DEBUG
extern int tr_debug = 0;
#endif
extern int TR_earlytokenrel = 0;	/* works only in 16Mb mode */
extern uchar_t TR_mtu = MTU_ARB;	/* Default mtu index */

/*
 * timeout set up by source routing
 */
extern int tr_timeout;

/*
 * Token Ring broadcast address definitions
 */
struct ether_addr tokenbroadcastaddr1 = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

struct ether_addr tokenbroadcastaddr2 = {
	0xc0, 0x00, 0xff, 0xff, 0xff, 0xff
};

/* IRQ values for microchannel */
static uchar_t mcirqmap[] = {9, 3, 10, 11};

/* IRQ values for ISA */
static uchar_t isairqmap[] = {9, 3, 6, 7};

/*
 * Linked list of device structures
 */
static trd_t		*trDevices;

/*
 * Linked list of upper streams info
 */
static trs_t		*trStreams;
static krwlock_t	trStreamsLock;

/*
 * Source routing table lock
 */
extern kmutex_t tr_srlock;		/* protect source route structure */

/*
 * Counter of number of physical points of attachment
 * (Need this because ddi_get_instance() under x86 will
 * give a different instance value for each line of the
 * config file and we need consecutive numbers starting
 * at zero for minor numbers)
 */
static int		tr_ppacount;

/*
 * Configuration error message defines
 */
#define	TR_MSG_IO_MISMATCH1 "!TR: Invalid I/O address property 0x%x."
#define	TR_MSG_IO_MISMATCH2 "!Expected either 0x%x or 0x%x.\n"
#define	TR_MSG_MMIO_MISMATCH1 "!TR:Config file has MMIO at %x."
#define	TR_MSG_MMIO_MISMATCH2 "!Board is set to have MMIO mapped to %x!\n"
#define	TR_MSG_SRAM_MISMATCH1 "!TR:Configuration file has shared RAM size "
#define	TR_MSG_SRAM_MISMATCH2 "!of 0x%x.\n.   Switch settings have the "
#define	TR_MSG_SRAM_MISMATCH3 "!size as 0x%x.\n"
#define	TR_MSG_BADRAMBND1 "!TR:Shared RAM address %x is not on correct boundary"
#define	TR_MSG_BADRAMBND2 "!or the full shared RAM won't fit in memory beyond "
#define	TR_MSG_BADRAMBND3 "!MMIO/ROM.\n 8 and 16k sizes must be on "
#define	TR_MSG_BADRAMBND4 "!16k boundaries, 32k sizes must be on \n"
#define	TR_MSG_BADRAMBND5 "!32k boundaries, and 64k sizes must be on 64k "
#define	TR_MSG_BADRAMBND6 "!boundaries.\n"
#define	TR_MSG_IRQ_MISMATCH1 "!TR:Configuration file has IRQ %d specified."
#define	TR_MSG_IRQ_MISMATCH2 "!   Switch settings have board using IRQ %d!!\n"
#define	TR_MSG_SEARCHPRIM   "!TR:Searching for primary adapter.\n"
#define	TR_MSG_SEARCHSECOND "!TR:Searching for secondary adapter.\n"

/*
 * tr_probe - Make sure a device is really there.
 */
tr_probe(dev_info_t *dip)
{

	int	regbuf[6];   /* Buffer for storing devinfo props */

	int	noboard = 1; /* Assume there is not a board */

	int	irq;		/* IRQ level for this board */

	uchar_t	*base_pio;	/* The base io address of programmed io area */
	uchar_t	bval;		/* Used to read from the pio area */

	paddr_t mmio_phys;   /* The base address of mmio area (from board) */

	/*
	 * The virtual base address of mmio
	 * area after mapping
	 */
	struct trmmio	*mmio_virt;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:PROBE, ");
#endif

	if (ddi_dev_is_sid(dip) == DDI_SUCCESS) {
		noboard = 0;
	} else {
		if (micro_channel(dip)) {
			unsigned char pos[6];
			static int slot = 1;
			int found;
			for (found = 0; !found && slot <= TR_MAXSLOTS; slot++) {
				tr_getpos(slot, pos);
				switch (*(ushort *)pos) {
				case TRPOSID_TRA:
				case TRPOSID_TR4_16A:
					/* skip any card that isn't enabled */
					if (!(pos[TRPOS_REG2] & TRPOS_ENABLED))
						continue;

					/* determine IRQ level */
					irq = ((pos[TRPOS_REG3] &
						TRPOS_IRQ_LSB) ? 1 : 0) |
						((pos[TRPOS_REG4] &
						    TRPOS_IRQ_MSB) ? 2 : 0);
					irq = (int)mcirqmap[irq];

					base_pio = (pos[TRPOS_REG3] &
						    TRPOS_SECONDARY)
						? SECONDARY_PIO : PRIMARY_PIO;

					mmio_phys = TRPOS_MMIO_ADDR(pos);

					if (!tr_ismatch(dip, irq,
					    (caddr_t)mmio_phys, (int)base_pio,
					    1))
						continue;

					regbuf[0] = (int)base_pio;
					(void) ddi_prop_create(DDI_DEV_T_NONE,
							dip, DDI_PROP_CANSLEEP,
							"ioaddr",
							(caddr_t)regbuf,
							sizeof (int));
					/*
					 * save the POS registers for
					 * later use
					 */
					ddi_set_driver_private(dip,
						(caddr_t)*(ulong *)(pos+2));
					noboard = 0;
					break;
				default:
					/* not what we want so continue */
					break;
				}
			}
		} else {
			static int adpsearch = 0;

			if (adpsearch == 0) {
				cmn_err(CE_CONT, TR_MSG_SEARCHPRIM);
				base_pio = PRIMARY_PIO;
			} else if (adpsearch == 1) {
				cmn_err(CE_CONT, TR_MSG_SEARCHSECOND);
				base_pio = SECONDARY_PIO;
			}
			else
				goto fail;

			adpsearch++;
			bval = inb((int)(base_pio + PIO_SETUP1));
			mmio_phys = MMIO_ADDR(bval);

			if (ddi_map_regs(dip, 0, (caddr_t *)&mmio_virt,
					mmio_phys, MMIO_SIZE) != DDI_FAILURE) {
				/*
				 *  confirm board is present
				 *    (Different for MC)
				 */
				if (trchkid(dip, mmio_virt) &&
				    trchksums(mmio_virt)) {
					noboard = 0;
					regbuf[0] = (int)base_pio;
					(void) ddi_prop_create(DDI_DEV_T_NONE,
						dip, DDI_PROP_CANSLEEP,
						"ioaddr", (caddr_t)regbuf,
						sizeof (int));
					/*
					 * save the physical mmio address for
					 * later use
					 */
					ddi_set_driver_private(dip,
						(caddr_t)mmio_phys);
				}
				ddi_unmap_regs(dip, 0, (caddr_t *)&mmio_virt,
						mmio_phys, MMIO_SIZE);
			}
		}
	}

#ifdef TR_DEBUG
	if (tr_debug)
		cmn_err(CE_CONT, "%s find a token ring board\n",
			noboard ? "Didn\'t" : "Did");
#endif

fail:
	if (noboard)
		return (DDI_PROBE_FAILURE);
	else
		return (DDI_PROBE_SUCCESS);

}

/*
 * tr_attach - init time attach support. When the hardware specific attach
 * is called, it must call this procedure with the device class structure
 */
static int
tr_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	struct irqprop {
	    int ipl;
	    int irq;
	} *irqprop;

	static	int once = 1;	/* For one-time driver operations */

	int	regbuf[12];	/* Buffer for storing devinfo props */
	int	buflen;		/* Storage for length of property */
	int	j;		/* loop index for reading board address */
	int	intregd = 0;    /* flag if we have registered our interrupt */

	short	irq_conf;	/* IRQ value specified in config file */
	short	irq_set;	/* IRQ value set on the board */

	uchar_t	*base_pio;	/* The base io address of PIO area */
	uchar_t	bval;		/* Used to read from the pio area */
	uchar_t  irq_read;	/* IRQ value encoded on the board */
	uchar_t	shrd_size_read;	/* Shared Ram size encoded on board */

	paddr_t	shrd_addr_conf;	/* Shared Ram address from config file */

	long	shrd_size_conf;	/* Shared Ram size specified in config file */

	trd_t	*trdp;	    /* soft structure pointer */

	ulong	mmio_base;  /* physical base address of mmio */

	struct trmmio	*mmio_virt; /* Virtual base address of mmio */
	struct trram	*shrd_virt; /* Virtual base address of shared RAM */
	unchar pos[6];

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	/*
	 * One time driver initializations
	 */
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:ATTACH, ");
#endif

	if (once) {
		once = 0;
		rw_init(&trStreamsLock, "tr streams linked list lock",
			RW_DRIVER, (void *)-1);

		mutex_init(&tr_srlock, "tr sr lock", MUTEX_DRIVER, (void *)0);
		tr_sr_init();
	}

	trdp = (trd_t *) NULL;
	mmio_virt = (struct trmmio *) NULL;
	shrd_virt = (struct trram *) NULL;

	/*
	 * Retrieve I/O address property. Won't verify it again, but assume
	 * its okay, since our probe succeeded.
	 */
	buflen = sizeof (regbuf);
	if (ddi_prop_op(DDI_DEV_T_NONE, dip, PROP_LEN_AND_VAL_BUF,
		DDI_PROP_DONTPASS, "ioaddr", (caddr_t) regbuf, &buflen)
		!= DDI_PROP_SUCCESS) {
		return (DDI_FAILURE);
	}

	base_pio = (uchar_t *) regbuf[0];
	trreset(base_pio);

	/*
	 * Allocate soft data structures
	 */
	if (!(trdp = kmem_zalloc(sizeof (trd_t), KM_NOSLEEP))) {
		cmn_err(CE_WARN, "TR:trdp kmem_alloc failed");
		return (DDI_FAILURE);
	}

	if (!(trdp->trd_statsd = kmem_zalloc(sizeof (struct trdstat),
						KM_NOSLEEP))) {
		cmn_err(CE_WARN, "TR:trd_stats kmem_alloc failed");
		goto fail;
	}

	/*
	 * Map the MMIO area into virtual memory
	 */
	if (micro_channel(dip)) {
		*(ulong *)(pos+2) = (ulong)ddi_get_driver_private(dip);
		mmio_base = TRPOS_MMIO_ADDR(pos);
	} else
		mmio_base = (ulong)ddi_get_driver_private(dip);

	ddi_set_driver_private(dip, (caddr_t) NULL);

	if (ddi_map_regs(dip, 0, (caddr_t *)&mmio_virt,
				(off_t)mmio_base, (off_t)MMIO_SIZE)
				== DDI_FAILURE) {
		cmn_err(CE_WARN, "TR:MMIO map failed");
		goto fail;
	}

	/* get/set shared RAM address */
	if (micro_channel(dip)) {
		shrd_addr_conf = TRPOS_RAM_ADDR(pos);
		shrd_size_conf = TRPOS_RAM_SIZE(pos);
	} else {
		if (mmio_virt->mmio[AIP_ADP_TYPE] > (uchar_t) AIP_AUTO)
			shrd_addr_conf = ddi_getprop(DDI_DEV_T_NONE, dip, 0,
						"sram", mmio_base + MMIO_SIZE);
		else {
			while (!mmio_virt->mmio[RRR])
				drv_usecwait(100);
			shrd_addr_conf = mmio_virt->mmio[RRR]<<SRAM_ADDR_SHIFT;
		}

		/*
		 * Validate configuration entries for shared ram
		 */
		shrd_size_read = (mmio_virt->mmio[RRR_ODD] & RRR_O_SMASK);
		switch ((int) shrd_size_read) {
		case RRR_O_8K:
			shrd_size_conf = 0x2000;
			break;
		case RRR_O_16K:
			shrd_size_conf = 0x4000;
			break;
		case RRR_O_32K:
			shrd_size_conf = 0x8000;
			break;
		case RRR_O_64K:
			shrd_size_conf = 0x10000;
			break;
		default:
			cmn_err(CE_WARN, "TR:??Impossible RAM size??");
			goto fail;
		}

		/*
		 * Make sure the shared ram address in the
		 * configuration file is on an appropriate
		 * boundary.
		 */
		if (trchkbnds(shrd_size_conf, shrd_addr_conf)) {
			cmn_err(CE_WARN, TR_MSG_BADRAMBND1,
				(int) shrd_addr_conf);
			cmn_err(CE_CONT, TR_MSG_BADRAMBND2);
			cmn_err(CE_CONT, TR_MSG_BADRAMBND3);
			cmn_err(CE_CONT, TR_MSG_BADRAMBND4);
			cmn_err(CE_CONT, TR_MSG_BADRAMBND5);
			cmn_err(CE_CONT, TR_MSG_BADRAMBND6);
			goto fail;
		}
	}

	trreset(base_pio);
	drv_usecwait(50 * 1000); /* wait for 50ms */

	/*
	 * Tell board where to put shared RAM by ORing the appropriate
	 * bits into the even byte of the RRR.
	 */
	mmio_virt->mmio[RRR] = SRAM_SET(shrd_addr_conf);

	/*
	 * Map the shared ram area into virtual memory
	 */
	if (ddi_map_regs(dip, 0, (caddr_t *)&shrd_virt,
			    shrd_addr_conf, shrd_size_conf) == DDI_FAILURE) {
		cmn_err(CE_WARN, "TR:Shared RAM regmap failed");
		goto fail;
	}

	trdp->trd_pioaddr = base_pio;
	trdp->trd_mmiophys = mmio_base;
	trdp->trd_mmio = mmio_virt;
	trdp->trd_sramphys = shrd_addr_conf;
	trdp->trd_sramaddr = shrd_virt;
	trdp->trd_sramsize = shrd_size_conf;
	trdp->trd_pending = NULL;
	trdp->not_first = 0;

	/*
	 * we have to zero out the last 512 bytes of the SRAM if
	 * we are working with 64K bytes of SRAM.  This is only needed
	 * when that part is used for some other function, but we do it
	 * all the time to be safe
	 */
	if (trdp->trd_sramsize == 0x10000)
		bzero((char *)trdp->trd_sramaddr + 0xfe00, 512);

	/*
	 * Get the irq property
	 */
	buflen = sizeof (regbuf);
	if (ddi_prop_op(DDI_DEV_T_NONE, dip, PROP_LEN_AND_VAL_BUF,
			DDI_PROP_DONTPASS, "intr", (caddr_t) regbuf, &buflen)
	    != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN, "TR:Unable to get intr property");
		goto fail;
	}

	irq_conf = 0;
	if (micro_channel(dip)) {
		irq_read = ((pos[TRPOS_REG3] & TRPOS_IRQ_LSB) ? 1 : 0) |
			((pos[TRPOS_REG4] & TRPOS_IRQ_MSB) ? 2 : 0);
		irq_set = mcirqmap[irq_read];
	} else {
		bval = inb((int)(base_pio + PIO_SETUP1));
		irq_read = (short) bval & TR_IRQ_MASK;
		if ((mmio_virt->mmio[AIP_ADP_TYPE] <= (uchar_t) AIP_AUTO) &&
		    (mmio_virt->mmio[AIP_SUP_INT] == (uchar_t) AIP_INT))
			irq_set = mcirqmap[irq_read];
		else irq_set = isairqmap[irq_read];
	}

	irqprop = (struct irqprop *)regbuf;
	for (irq_read = 0; irq_read < (buflen/sizeof (int)); irq_read++)
		if (irq_set == irqprop[irq_read].irq) {
			irq_conf = irq_set;
			break;
		}

	if (irq_set != irq_conf) {
		cmn_err(CE_WARN, TR_MSG_IRQ_MISMATCH1, irq_conf);
		cmn_err(CE_CONT, TR_MSG_IRQ_MISMATCH2, irq_set);
		goto fail;
	}
	trdp->trd_int = irq_read;

	/*
	 * read the node address.
	 */
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR board addr: ");
#endif
	for (j = 0; j < MAC_ADDR_LEN; j++) {
		uchar_t *ap;
		ap = &(trdp->trd_mmio->mmio[AIP_AEA_ADDR+4*j]);
		trdp->trd_factaddr[j] = trdp->trd_macaddr[j] =
			(*ap << 4) | (*(ap+2) & 0xf);
#ifdef TR_DEBUG
		if (tr_debug & TRTRACE)
			cmn_err(CE_CONT, "%x ", trdp->trd_macaddr[j]);
#endif
	}
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "\n");
#endif

	/*
	 * now do all the DDI stuff necessary
	 */

	/*
	 * Add interrupt to system.
	 */
	if (ddi_add_intr(dip, irq_read, &trdp->trd_intr_cookie, 0, tr_intr,
		(caddr_t) trdp)) {
		cmn_err(CE_WARN, "TR:ddi_add_intr failed");
		goto fail;
	}
	mutex_init(&trdp->trd_intrlock, "tr intr lock", MUTEX_DRIVER,
			trdp->trd_intr_cookie);
	intregd = 1;
	trdp->trd_flags |= TR_READY;

	/* Set up local ether address for netboot */
	localetheraddr((struct ether_addr *)trdp->trd_factaddr, NULL);

	ddi_set_driver_private(dip, (caddr_t) trdp);

	/*
	 * create the file system device node
	 */
	if (ddi_create_minor_node(dip, "tr", S_IFCHR,
		tr_ppacount, DDI_NT_NET, CLONE_DEV) == DDI_FAILURE) {
		cmn_err(CE_WARN, "TR: Ddi_create_minor_node failed");
		ddi_remove_minor_node(dip, NULL);
		goto fail;
	}
	trdp->trd_ppanum = tr_ppacount++;
	trdp->trd_devnode = dip;
	trdp->trd_bridgemtu = TR_mtu;

	tr_kstatinit(trdp);

	trdp->trd_nextd = trDevices;
	trDevices = trdp;

	/*
	 * Enable the interrupts to initialize the board
	 */
	drv_getparm(LBOLT, &trdp->trd_inittime);
	MMIO_SETBIT(ISRP, ISRP_E_IENB);

	ddi_report_dev(dip);

	trdp->detaching = 0;
	trdp->wdog_lbolt = 0;
	trdp->wdog_id = timeout(tr_wdog, (caddr_t)trdp, WDOGTICKS);

	return (DDI_SUCCESS);

fail:
	if (trdp && trdp->trd_kstatp)
		kstat_delete(trdp->trd_kstatp);
	if (!trDevices) {
		(void) untimeout(tr_timeout);
		mutex_destroy(&tr_srlock);
	}
	if (intregd) {
		mutex_destroy(&trdp->trd_intrlock);
		MMIO_RESETBIT(ISRP, (uchar_t) ISRP_E_IENB);
		ddi_remove_intr(dip, trdp->trd_int, trdp->trd_intr_cookie);
	}
	if (trdp && trdp->trd_statsd)
		kmem_free((caddr_t)trdp->trd_statsd,
				sizeof (*trdp->trd_statsd));
	if (trdp)
		kmem_free((caddr_t) trdp, sizeof (*trdp));
	if (mmio_virt)
		ddi_unmap_regs(dip, 0, (caddr_t *)&mmio_virt, (off_t)mmio_base,
				(off_t)MMIO_SIZE);
	if (shrd_virt)
		ddi_unmap_regs(dip, 0, (caddr_t *)&shrd_virt,
				(off_t)shrd_addr_conf, (off_t)shrd_size_conf);
	return (DDI_FAILURE);
}

/*
 * tr_detach - standard kernel interface routine
 */
static int
tr_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	trd_t	*trdp;	    /* soft structure pointer */

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:DETACH");
#endif

	if (cmd != DDI_DETACH) {
		return (DDI_FAILURE);
	}

	/* remove all mutex and locks */
	rw_destroy(&trStreamsLock);

	ddi_remove_minor_node(dip, NULL);

	/*
	 * Get the private data and use it to free
	 * up what we have kmem_alloced.
	 */
	trdp = (trd_t *) ddi_get_driver_private(dip);

	mutex_enter(&trdp->trd_intrlock);
	trdp->detaching = 1;
	mutex_exit(&trdp->trd_intrlock);

	untimeout(trdp->wdog_id);

	/* remove all mutex and locks */
	if (trdp) {
		int i;
		uchar_t intreg;

		/*
		 * Disable interrupts
		 */
		MMIO_RESETBIT(ISRP, (uchar_t) ISRP_E_IENB);

		/*
		 * Clear response bit then start close
		 */
		MMIO_RESETBIT(ISRP + 1, (uchar_t) ISRP_O_SRB_RES);
		trcloseboard(trdp);

		/*
		 *  Give command 500 millisecs for command to complete
		 */
		for (i = 0; i < 100; i++) {
			intreg = trdp->trd_mmio->mmio[ISRP+1];
			if (intreg & ISRP_O_SRB_RES)
				break;
			drv_usecwait(10*1000);  /* wait 10 millisecs */
		}
		mutex_destroy(&trdp->trd_intrlock);
		ddi_remove_intr(dip, trdp->trd_int, trdp->trd_intr_cookie);
		if (trdp->trd_kstatp)
			kstat_delete(trdp->trd_kstatp);
		if (trdp->trd_sramaddr)
			ddi_unmap_regs(dip, 0,
					(caddr_t *) &trdp->trd_sramaddr,
					(off_t)trdp->trd_sramphys,
					(off_t)trdp->trd_sramsize);
		if (trdp->trd_mmio)
			ddi_unmap_regs(dip, 0,
					(caddr_t *) &trdp->trd_mmio,
					(off_t)trdp->trd_mmiophys,
					(off_t)MMIO_SIZE);
		if (trdp->trd_statsd)
			kmem_free((caddr_t) trdp->trd_statsd,
					sizeof (*trdp->trd_statsd));
		kmem_free((caddr_t) trdp, sizeof (trd_t));
		ddi_set_driver_private(dip, (caddr_t) NULL);
	}

	/* clean up after source routing */
	(void) untimeout(tr_timeout);
	mutex_destroy(&tr_srlock);

	return (DDI_SUCCESS);
}

/*
 * tr_devinfo - standard kernel devinfo lookup function
 */
/*ARGSUSED*/
static int
tr_devinfo(dev_info_t *dip, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int error;

	switch (cmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if (dip == NULL) {
			error = DDI_FAILURE;
		} else {
			*result = (void *) dip;
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

/*
 * tr_open - TR open routine, called when device is opened by the user
 */
/*ARGSUSED*/
static int
tr_open(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *cred)
{
	trs_t	*trsp;
	trs_t	**strloop;
	trd_t   *dvlp;
	int	minordev;
	int	rc = 0;
	unsigned long	val;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:OPEN");
#endif

	ASSERT(q);

	/*
	 *  The following workaround brought to you by IP, who insists
	 *  on knowing a real max_sdu even before he's attached.
	 *  Because of this we make all opens sleep until all the
	 *  tr boards are finished initializing.
	 */
	for (dvlp = trDevices; dvlp; dvlp = dvlp->trd_nextd) {
		mutex_enter(&dvlp->trd_intrlock);
		drv_getparm(LBOLT, &val);
		if (!(dvlp->trd_flags & TR_INIT)) {
			if (dvlp->not_first) {
				while (!(dvlp->trd_flags & TR_OPEN)) {
					tr_reinit(dvlp);
					(void) tr_reopen(dvlp);
				}
				drv_getparm(LBOLT, &dvlp->trd_inittime);
			} else if (val - dvlp->trd_inittime <=
			    TR_INIT_TIME) {
				if (!cv_wait_sig(&dvlp->trd_initcv,
					&dvlp->trd_intrlock)) {
					mutex_exit(&dvlp->trd_intrlock);
					return (EINTR);
				}
			} else {
				mutex_exit(&dvlp->trd_intrlock);
				return (EIO);
			}
		}
		if (dvlp->not_first) {
			tr_reopensap(dvlp);
		}

		mutex_exit(&dvlp->trd_intrlock);
	}

	/*
	 * Serialize access through open/close this will serialize across all
	 * tr devices, but open and close are not frequent so should not
	 * induce much, if any delay.
	 */
	rw_enter(&trStreamsLock, RW_WRITER);

	/*
	 * Determine minor device number.
	 */
	strloop = &trStreams;
	if (sflag == CLONEOPEN) {
		minordev = 0;
		for (; (trsp = *strloop) != NULL;
		    strloop = &trsp->trs_nexts) {
			if (minordev < trsp->trs_minor)
					break;
			minordev++;
		}
		*devp = makedevice(getmajor(*devp), minordev);
	} else
		minordev = getminor(*devp);

	if (q->q_ptr)
		goto done;

	if (!(trsp = kmem_zalloc(sizeof (struct trs), KM_NOSLEEP))) {
		rc = ENOMEM;
		goto done;
	}

	trsp->trs_minor = minordev;
	trsp->trs_rq = q;
	trsp->trs_state = DL_UNATTACHED;
	trsp->trs_flags = 0x0;
	mutex_init(&trsp->trs_lock, "tr stream lock", MUTEX_DRIVER,
		(void *)0);

	/*
	 * Link new entry into the list of active entries.
	 */
	trsp->trs_nexts = *strloop;
	*strloop = trsp;

	q->q_ptr = WR(q)->q_ptr = (char *) trsp;
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:New stream is %x\n", trsp);
#endif

done:
	rw_exit(&trStreamsLock);
	qprocson(q);		/* start the queues running */
	return (rc);

}

/*
 * tr_close (q)
 * normal stream close call checks current status and cleans up
 * data structures that were dynamically allocated
 */
/*ARGSUSED1*/
static int
tr_close(queue_t *q, int flag, cred_t *credp)
{
	trs_t *trsp;
	trd_t *trdp;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:CLOSE. stream %x\n", q->q_ptr);
#endif

	ASSERT(q);
	ASSERT(q->q_ptr);

	qprocsoff(q);

	trsp = (trs_t *) q->q_ptr;
	trdp = trsp->trs_dev;

	/*
	 * Take stream off waiting lists so it won't be awakened after open
	 * its already gone.
	 */
	if (trdp) {
		mutex_enter(&trdp->trd_intrlock);
		trremwaiter(trdp, trsp);
		trremcmds(trdp, trsp);
		mutex_exit(&trdp->trd_intrlock);
	}

	/*  Unbind if bound */
	if (trsp->trs_state == DL_IDLE)
		trunbindnoq(trdp, trsp->trs_802sap);

	if (trsp->trs_multimask) {
		mutex_enter(&trdp->trd_intrlock);
		trsp->trs_multimask = 0;
		trchgmultinoq(trdp);
		mutex_exit(&trdp->trd_intrlock);
	}

	/* completely disassociate the stream from the device */
	rw_enter(&trStreamsLock, RW_WRITER);
	q->q_ptr = WR(q)->q_ptr = NULL;
	(void) trremstr(trsp);	/* remove from active list */
	mutex_destroy(&trsp->trs_lock);
	kmem_free(trsp, sizeof (struct trs));
	rw_exit(&trStreamsLock);

	return (0);

}

/*
 * tr_wput - general tr stream write put routine. Receives ioctl's from
 * user level and data from upper modules and processes them immediately.
 * M_PROTO/M_PCPROTO are queued for later processing by the service
 * procedure.
 */
static int
tr_wput(queue_t *q, mblk_t *mp)
{
	trs_t	*trsp = (trs_t *) (q->q_ptr);

#ifdef TR_DEBUG
	if (tr_debug & (TRTRACE | TRSEND | TRPROT))
		cmn_err(CE_CONT, "TR:WPUT, stream %x", trsp);
#endif

	switch (DB_TYPE(mp)) {

	case M_IOCTL:		/* no waiting in ioctl's */
#ifdef TR_DEBUG
		if (tr_debug & TRPROT)
			cmn_err(CE_CONT, "IOC, ");
#endif
		(void) tr_ioctl(q, mp);
		break;

	case M_FLUSH:		/* canonical flush handling */
#ifdef TR_DEBUG
		if (tr_debug & TRPROT)
			cmn_err(CE_CONT, "FLU, ");
#endif
		if (*mp->b_rptr & FLUSHW) {
			flushq(q, FLUSHDATA);
		}
		if (*mp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHDATA);
			*mp->b_rptr &= ~FLUSHW;
			qreply(q, mp);
		} else {
			freemsg(mp);
			mp = NULL;
		}
		break;

		/* for now, we will always queue */
	case M_PROTO:
	case M_PCPROTO:
#ifdef TR_DEBUG
		if (tr_debug & TRPROT)
			cmn_err(CE_CONT, "PRO, ");
#endif
		(void) putq(q, mp);
		break;

	case M_DATA:
#ifdef TR_DEBUG
		if (tr_debug & TRSEND)
			cmn_err(CE_CONT, "DAT, ");
#endif
		/* fast data / raw support */
		if (trsp->trs_flags & (TRS_RAW | TRS_FAST | TRS_WTURN) == 0 ||
		    trsp->trs_state != DL_IDLE) {
			(void) merror(q, mp, EPROTO);
			break;
		}
		/* need to do further checking */
		if (trsp->trs_flags & (TRS_PROMSAP | TRS_PROMMULTI))
			(void) putq(q, mp);
		else {
			int rc, err, uerr;
			rc = trxmit(q, mp, &err, &uerr);
			if (rc == TRE_SLEEP)
				return (0);
		}
		break;

	default:
		cmn_err(CE_WARN, "TR:Unknown msg type (%x)\n", DB_TYPE(mp));
		freemsg(mp);
		mp = NULL;
		break;
	}
	return (0);
}

/*
 * tr_wsrv - Incoming messages are processed according to the DLPI
 * protocol specification
 */
static int
tr_wsrv(queue_t *q)
{
	union DL_primitives	*prim;
	dl_unitdata_req_t	*udreq;
	mblk_t			*mp;
	int			err, rc, uerr;
#ifdef TR_DEBUG
	register trs_t		*trsp = (trs_t *) q->q_ptr;

	if (tr_debug & (TRTRACE | TRSEND | TRPROT))
		cmn_err(CE_CONT, "WSRV, stream %x, ", trsp);
#endif

	while ((mp = getq(q)) != NULL) {

		switch (mp->b_datap->db_type) {
		case M_PROTO:	/* Will be a DLPI message of some type */
		case M_PCPROTO:
#ifdef TR_DEBUG
			if (tr_debug & (TRTRACE | TRPROT))
				cmn_err(CE_CONT, "dlprim %x.\n",
					*((ulong *)mp->b_rptr));
#endif
			if ((rc = (tr_cmds(q, mp, &err, &uerr))) == TRE_ERR) {
#ifdef TR_DEBUG
				if (tr_debug & TRERRS)
					cmn_err(CE_CONT,
						"Error on cmd[0x%x]\n", err);
#endif
				prim = (union DL_primitives *) mp->b_rptr;
				dlerrorack(q, mp, prim->dl_primitive,
						err, uerr);
			} else if (rc == TRE_UDERR) {
#ifdef TR_DEBUG
				if (tr_debug & TRERRS) {
					cmn_err(CE_CONT, "Error on unitdata ");
					cmn_err(CE_CONT, "request[0x%x]\n",
						err);
				}
#endif
				udreq = (dl_unitdata_req_t *) mp->b_rptr;
				dluderrorind(q, mp, (uchar_t *)prim +
						udreq->dl_dest_addr_offset,
						udreq->dl_dest_addr_length,
						err, uerr);
			} else if (rc == TRE_SLEEP) {
#ifdef TR_DEBUG
				if (tr_debug & TRTRACE)
					cmn_err(CE_CONT, "%x ZZZZ\n", trsp);
#endif
				return (0);
#ifdef TR_DEBUG
			} else {
				if (tr_debug & TRTRACE) {
					cmn_err(CE_CONT, "%x command ", trsp);
					cmn_err(CE_CONT, "completed.\n");
				}
#endif
			}
			break;
		case M_DATA:
			/*
			 * retry of a previously processed
			 * UNITDATA_REQ or is a RAW message from
			 * above, so send it out. The transmit process
			 * will free the message later.
			 */
			rc = trxmit(q, mp, &err, &uerr);
			if (rc == TRE_SLEEP)
				return (0);
			break;

			/* This should never happen */
		default:
#ifdef TR_DEBUG
			cmn_err(CE_WARN, "TR:WSRV-? msg(%x)", DB_TYPE(mp));
#endif
			freemsg(mp);	/* unknown types are discarded */
			mp = NULL;
			break;
		}
	}
	return (0);
}

/*
 * tr_intr - Interrupt handler.
 */
static u_int
tr_intr(caddr_t	arg)
{
	register uchar_t	intreg;
	int			ackintr = 0;	/* ack the interrupt */
	int 			serviced = 0;	/* return 1 if we claim intr */
	trd_t			*trdp;
	struct dir_cmd		*arbcmd;
	struct xmt_cmd		*xc;

	trdp = (trd_t *) arg;

	/* If we aren't fully set up to handle intrs yet, don't claim intr */
	if (!(trdp->trd_flags & TR_READY))
		return (serviced);

	mutex_enter(&trdp->trd_intrlock);

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:INTR, ");
#endif

	intreg = trdp->trd_mmio->mmio[ISRP+1];

	/* verify we have an interrupt */
	if (!(intreg & ISRP_O_INTR)) {
#ifdef TR_DEBUG
		if (tr_debug & TRERRS)
			cmn_err(CE_CONT, "TR:Intr isn't ours!? %x", intreg);
#endif
		mutex_exit(&trdp->trd_intrlock);
		return (serviced);
	}
	serviced = 1;

	if (intreg & ISRP_O_CHCK) {
		MMIO_RESETBIT(ISRP + 1, (uchar_t) ISRP_O_CHCK);
		/*
		 * Disable interrupts
		 */
		cmn_err(CE_WARN, "TR:Disabling INTRs due to adapter check!!");
		MMIO_RESETBIT(ISRP, (uchar_t) ISRP_E_IENB);
		/* got to reset the board and close all minor devs */
	}
	if (intreg & ISRP_O_SRB_RES) {
		MMIO_RESETBIT(ISRP + 1, (uchar_t) ISRP_O_SRB_RES);
		if (!(trdp->trd_flags & TR_INIT)) {
			trdp->trd_statsd->trc_intrs++;
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "INIT intr\n");
#endif
			trdp->trd_wrbr = (uchar_t *)
				((caddr_t)trdp->trd_sramaddr +
				trswab(*((ushort_t *)
					&(trdp->trd_mmio->mmio[WRBR]))));
			trdp->trd_srb = (struct srb *)trdp->trd_wrbr;
			trdp->trd_adprate =
				(trdp->trd_srb->srb[1] & 0x1) ? 16 : 4;
			trcompbufs(trdp);
			trdp->trd_flags |= TR_INIT;
			cv_broadcast(&trdp->trd_initcv);
			trwakewaiters(trdp);
			outb((int)(trdp->trd_pioaddr + PIO_INT_ENB), 0);
			mutex_exit(&trdp->trd_intrlock);
			return (serviced);
		}
		arbcmd = (struct dir_cmd *)trdp->trd_srb;

#ifdef TR_DEBUG
		if (tr_debug & TRTRACE)
			cmn_err(CE_CONT, "SRBR, ");
#endif
		switch (arbcmd->dc_cmd) {
		case DIR_OPEN_ADP:
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "AdpOpenDone ");
#endif
			tropendone(trdp);
			break;
		case DIR_CLOSE_ADP:
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "AdpCloseDone ");
#endif
			trdp->trd_flags &= ~TR_OPEN;
			trdp->trd_srb = (struct srb *)trdp->trd_wrbr;
			break;
		case DLC_OPEN_SAP:
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "SAPOpenDone ");
#endif
			tropensapdone(trdp);
			break;
		case DLC_CLOSE_SAP:
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "SAPCloseDone ");
#endif
			trclosesapdone(trdp);
			break;
		case DIR_FUNC_ADDR:
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "FuncAddrDone ");
#endif
			trsetfuncdone(trdp);
			break;
		case DIR_GRP_ADDR:
		case DIR_MOD_PARAMS:
			break;
		case XMT_UI_FRAME:
		case XMT_XID_CMD:
		case XMT_TEST_CMD:
		case XMT_DIR_FRAME:
		case XMT_XRES_FIN:
		case XMT_XRES_NFIN:
			if (arbcmd->dc_retcode != OK_RC) {
#ifdef TR_DEBUG
				if (tr_debug & (TRTRACE | TRERRS | TRSEND))
					cmn_err(CE_CONT, "TR:XMT ERR (0x%x)\n",
						arbcmd->dc_retcode);
#endif
				trdp->trd_statsd->trc_oerrors++;
				if (trdp->trd_pkt)
					freemsg(trdp->trd_pkt);
				trdp->trd_pkt = (mblk_t *)0;
			}
			trnxtcmd(trdp);
			break;

		case DIR_INTERRUPT:
		case DIR_RES_PARAMS:
		case DIR_READ_LOG:
		case DLC_CLOSE_STN:
		case DLC_CON_STN:
		case DLC_FLOW_CNTL:
		case DLC_MODIFY:
		case DLC_OPEN_STN:
		case DLC_REALLOC:
		case DLC_RESET:
		case DLC_STAT:
		case XMT_I_FRAME:
		default:
#ifdef TR_DEBUG
			if (tr_debug & TRERRS)
				cmn_err(CE_NOTE, "TR:Unexpected srb (0x%x)\n",
					arbcmd->dc_cmd);
#endif
			break;
		}
	}

	if (intreg & ISRP_O_ASB_FREE) {
		MMIO_RESETBIT(ISRP + 1, (uchar_t) ISRP_O_ASB_FREE);
#ifdef TR_DEBUG
		if (tr_debug & TRTRACE)
			cmn_err(CE_CONT, "ASBFREE:");
#endif
	}
	if (intreg & ISRP_O_ARB_CMD) {
		MMIO_RESETBIT(ISRP + 1, (uchar_t) ISRP_O_ARB_CMD);
		arbcmd = (struct dir_cmd *)trdp->trd_arb;
		ackintr++;

		switch (arbcmd->dc_cmd) {
		case ADP_DLC_STAT:
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "DLCSTAT:");
#endif
			break;

		case ADP_RCV_DATA:
#ifdef TR_DEBUG
			if (tr_debug & (TRTRACE | TRRECV))
				cmn_err(CE_CONT, "RCVD:");
#endif
			trrcv(trdp);
			break;

		case ADP_RING_CHNG:
			mutex_exit(&trdp->trd_intrlock);
			trdescchg(trdp);
			mutex_enter(&trdp->trd_intrlock);
			break;

		case ADP_XMT_REQ:
#ifdef TR_DEBUG
			if (tr_debug & TRSEND)
				cmn_err(CE_CONT, "OK,XMIT:");
#endif
			trsend(trdp);
			break;

		default:
#ifdef TR_DEBUG
			if (tr_debug & (TRERRS | TRTRACE))
				cmn_err(CE_NOTE, "TR:Unexpected arb (0x%x)?",
					trdp->trd_arb->arb[0]);
#endif
			break;
		}
	}
	if (intreg & ISRP_O_SSB_RES) {
		MMIO_RESETBIT(ISRP + 1, (uchar_t) ISRP_O_SSB_RES);
		xc = (struct xmt_cmd *)trdp->trd_ssb;
#ifdef TR_DEBUG
		if (tr_debug & (TRSEND | TRERRS)) {
			cmn_err(CE_CONT, "XMTCOMP:");
			cmn_err(CE_CONT, "XMTRC(0x%x)", xc->xc_retcode);
			if (xc->xc_retcode) {
				cmn_err(CE_CONT, "FS byte 0x%x",
					xc->xr_fserror);
			}
		}
#endif
		if (xc->xc_retcode)
			trdp->trd_statsd->trc_oerrors++;
		else
			trdp->trd_statsd->trc_opackets++;

		trnxtcmd(trdp);
		MMIO_SETBIT(ISRA + 1, ISRA_O_SSB_FREE);
	}
	if (intreg & ISRP_O_BFF_DONE) {
		MMIO_RESETBIT(ISRP + 1, (uchar_t) ISRP_O_BFF_DONE);
#ifdef TR_DEBUG
		if (tr_debug & TRTRACE)
			cmn_err(CE_CONT, "BRDGFWD:");
#endif
	}

	/* ack the receipt of the interrupt */
	if (ackintr) {
		MMIO_SETBIT(ISRA + 1, ISRA_O_ARB_FREE);
#ifdef TR_DEBUG
		if (tr_debug & TRTRACE)
			cmn_err(CE_CONT, "IACK,");
#endif
	}

	/* reset the interrupt */
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "SVCD!\n");
#endif
	trdp->trd_statsd->trc_intrs++;
	outb((int)(trdp->trd_pioaddr + PIO_INT_ENB), 0);
	mutex_exit(&trdp->trd_intrlock);
	return (serviced);
}

/*
 * trchkid - check that the id string read from the MMIO
 * matches the expected string for this board.
 */
static int
trchkid(dev_info_t *dip, struct trmmio *mmiop)
{
	/*
	 * These two strings are encoded on the board for Micro Channel
	 * and PC respectively
	 */
	static uchar_t ch_id_mc[] = {0x4, 0xD, 0x4, 0x1, 0x5, 0x2, 0x5, 0x3,
		0x3, 0x6, 0x3, 0x3, 0x5, 0x8, 0x3, 0x4,
		0x3, 0x5, 0x3, 0x1, 0x3, 0x8, 0x2, 0x0};
	static uchar_t ch_id_pc[] = {0x5, 0x0, 0x4, 0x9, 0x4, 0x3, 0x4, 0xf,
		0x3, 0x6, 0x3, 0x1, 0x3, 0x1, 0x3, 0x0,
		0x3, 0x9, 0x3, 0x9, 0x3, 0x0, 0x2, 0x0};

	int	comp;		/* for looping through MMIO */
	int	retval;		/* return value, 0 for failure */
	uchar_t	*expect;	/* Pointer for looping thru expected id */

	if (micro_channel(dip))
		expect = ch_id_mc;
	else
		expect = ch_id_pc;

	/*
	 *  Compare lower nibbles at even bytes with known
	 *  identification string.
	 */
	for (comp = 0; comp < AIP_CH_ID_CNT; comp += 2) {
		if ((mmiop->mmio[AIP_CH_ID_ADDR + comp] & 0xf) != *expect++)
			break;
	}

	if (comp != AIP_CH_ID_CNT) {
		cmn_err(CE_NOTE, "!TR:FAILED id string match");
		retval = 0;
	} else {
		retval = 1;
	}

	return (retval);

}

/*
 * trchksums - check that the checksums read from the MMIO
 * have the expected values.
 */
static int
trchksums(struct trmmio *mmiop)
{

	int	chksum;		/* For calculating checksums */
	int	retval;		/* return value, 0 for failure */

	uchar_t  *stop;		/* Last address in a checksum area */
	uchar_t	*calcp;		/* Pointer for looping thru MMIO area */

	/*
	 * Calculate the two checksums. The sum
	 * of all the nibbles in the even locations
	 * of each checksum area should be 0.
	 */
	stop = (uchar_t *) &(mmiop->mmio[AIP_CHKSM1_ADDR]);

	for (chksum = 0, calcp = (uchar_t *) &(mmiop->mmio[AIP_AREA]);
		calcp <= stop; calcp++) {
		chksum += *calcp++ & 0xf;
	}

	if (chksum & 0xf) {
		retval = 0;
		cmn_err(CE_NOTE, "!TR:FAILED cksum #1");
	} else {
		stop = (uchar_t *) &(mmiop->mmio[AIP_CHKSM2_ADDR]);

		for (chksum = 0, calcp = (uchar_t *) &(mmiop->mmio[AIP_AREA]);
			calcp <= stop; calcp++) {
			chksum += *calcp++ & 0xf;
		}
		if (chksum & 0xf) {
			retval = 0;
			cmn_err(CE_NOTE, "!TR:FAILED cksum #2");
		} else {
			retval = 1;
		}
	}

	return (retval);
}

/*
 * trchkbnds -- Ensure that the shared ram address provided is on
 * a 16K boundary if the size is 8K or 16K, a 32K boundary for size 32K,
 * or a 64K boundary for size 64K.  Returns zero if address is ok.
 * Also ensure shared RAM remains below 0x100000.

 */
static int
trchkbnds(long size, paddr_t addr)
{
	long	bndsize;
	long	a;

	if (((long)addr + size) <= 0x100000) {
		a = (long) addr;
		bndsize = (size == 0x2000) ? 0x4000 : size;
		if (a == (a/bndsize) * bndsize)
			return (0);
		else
			return (1);
	} else
		return (0);
}

/*
 * tr_reboot - Machine is being rebooted. We want to reset the board so
 *            it no longer receives interrupts.
 */
/*ARGSUSED1*/
static int
tr_reboot(dev_info_t *dip, ddi_reset_cmd_t cmd)
{
	trd_t *trdp;

	trdp = (trd_t *) ddi_get_driver_private(dip);
	trreset(trdp->trd_pioaddr);
	return (DDI_SUCCESS);
}

/*
 * trreset - issue a board reset and wait for its completion
 */
static void
trreset(uchar_t *ioaddr)
{
	outb((int)(ioaddr + PIO_RST_LCH), 0);
	drv_usecwait(50*1000); 	/* wait at least 50 millisecs */
	outb((int)(ioaddr + PIO_RST_RLS), 0);
}

/*
 * tr_cmds - process the DL commands as defined in dlpi.h
 */
static int
tr_cmds(queue_t *q, mblk_t *mp, int *err, int *uerr)
{
	register union DL_primitives *dlp;
	trs_t *tr = (trs_t *) q->q_ptr;
	int	result;

	dlp = (union DL_primitives *) mp->b_rptr;

	mutex_enter(&tr->trs_lock);

	switch (dlp->dl_primitive) {
	case DL_BIND_REQ:
		result = trbind(q, mp, err, uerr);
		break;

	case DL_UNBIND_REQ:
		result = trunbind(q, mp, err, uerr);
		break;

	case DL_UNITDATA_REQ:
		result = tr_unitdata(q, mp, err, uerr);
		break;

	case DL_INFO_REQ:
		result = tr_inforeq(q, mp);
		break;

	case DL_ATTACH_REQ:
		result = trattach(q, mp, err, uerr);
		break;

	case DL_DETACH_REQ:
		result = trunattach(q, mp, err, uerr);
		break;

	case DL_ENABMULTI_REQ:
		result = tr_enable_multi(q, mp, err, uerr);
		break;

	case DL_DISABMULTI_REQ:
		result = tr_disable_multi(q, mp, err, uerr);
		break;

	case DL_XID_REQ:
		result = tr_xid_req_res(q, mp, err, uerr, 0);
		break;

	case DL_XID_RES:
		result = tr_xid_req_res(q, mp, err, uerr, 1);
		break;

	case DL_TEST_REQ:
		result = tr_test_req_res(q, mp, err, uerr, 0);
		break;

	case DL_TEST_RES:
		result = tr_test_req_res(q, mp, err, uerr, 1);
		break;

	case DL_PHYS_ADDR_REQ:
		result = tr_get_phys(q, mp, err, uerr);
		break;

	case DL_PROMISCON_REQ:
		result = tr_promiscon(q, mp, err, uerr);
		break;

	case DL_PROMISCOFF_REQ:
		result = tr_promiscoff(q, mp, err, uerr);
		break;

	default:
#ifdef TR_DEBUG
		if (tr_debug & TRERRS)
			cmn_err(CE_CONT, "tr_cmds: Unknown primitive: %d",
				dlp->dl_primitive);
#endif
		*err = DL_BADPRIM;
		*uerr = 0;
		result = TRE_ERR;
	}
	mutex_exit(&tr->trs_lock);
	return (result);
}

/*
 * tr_promiscon - If we can, turn requested promiscuous mode on.
 */
static int
tr_promiscon(queue_t *q, mblk_t *mp, int *err, int *uerr)
{
	trs_t	*trsp;
	trd_t	*trdp;

	trsp = (trs_t *) q->q_ptr;
	trdp = (trd_t *) trsp->trs_dev;

	if (MBLKL(mp) < DL_PROMISCON_REQ_SIZE) {
		*err = DL_BADPRIM;
		*uerr = 0;
		return (TRE_ERR);
	}

	switch (((dl_promiscon_req_t *) mp->b_rptr)->dl_level) {

	case DL_PROMISC_PHYS:
		*err = DL_NOTSUPPORTED;
		*uerr = 0;
		return (TRE_ERR);

	case DL_PROMISC_SAP:
		trsp->trs_flags |= TRS_PROMSAP;
		mutex_enter(&trdp->trd_intrlock);
		trdp->trd_flags |= TR_PROM;
		trdp->trd_nproms++;
		mutex_exit(&trdp->trd_intrlock);
		break;

	case DL_PROMISC_MULTI:
		trsp->trs_flags |= TRS_PROMMULTI;
		mutex_enter(&trdp->trd_intrlock);
		trdp->trd_flags |= TR_PROM;
		trdp->trd_nproms++;
		mutex_exit(&trdp->trd_intrlock);
		break;

	default:
		*err = DL_NOTSUPPORTED;
		*uerr = 0;
		return (TRE_ERR);
	}

	dlokack(q, mp, DL_PROMISCON_REQ);
	return (TRE_OK);
}

/*
 * tr_promiscoff - Turn promiscuous mode off (if it is on)
 */
static int
tr_promiscoff(queue_t *q, mblk_t *mp, int *err, int *uerr)
{
	trs_t 	*trsp;
	trd_t	*trdp;
	int 	flag = 0;

	trsp = (trs_t *) q->q_ptr;
	trdp = trsp->trs_dev;

	if (MBLKL(mp) < DL_PROMISCOFF_REQ_SIZE) {
		*err = DL_BADPRIM;
		*uerr = 0;
		return (TRE_ERR);
	}

	switch (((dl_promiscoff_req_t *) mp->b_rptr)->dl_level) {

	case DL_PROMISC_PHYS:
		*err = DL_NOTSUPPORTED;
		*uerr = 0;
		return (TRE_ERR);

	case DL_PROMISC_SAP:
		flag = TRS_PROMSAP;
		break;

	case DL_PROMISC_MULTI:
		flag = TRS_PROMMULTI;
		break;

	default:
		*err = DL_NOTSUPPORTED;
		*uerr = 0;
		return (TRE_ERR);
	}

	if ((trsp->trs_flags & flag) == 0) {
		*err = DL_NOTENAB;
		*uerr = 0;
		return (TRE_ERR);
	} else {
		mutex_enter(&trdp->trd_intrlock);
		trdp->trd_nproms--;
		if (!trdp->trd_nproms)
			trdp->trd_flags &= ~TR_PROM;
		mutex_exit(&trdp->trd_intrlock);
	}

	trsp->trs_flags &= ~flag;
	dlokack(q, mp, DL_PROMISCOFF_REQ);
	return (TRE_OK);
}

/*
 * tr_get_phys - answer DLPI request for physical address
 */
static int
tr_get_phys(queue_t *q, mblk_t *mp, int *err, int *uerr)
{
	trs_t *trsp;
	trd_t *trdp;
	union DL_primitives *dlp;
	int type;
	struct ether_addr addr;

	if (MBLKL(mp) < DL_PHYS_ADDR_REQ_SIZE) {
		*err = DL_BADPRIM;
		*uerr = 0;
		return (TRE_ERR);
	}

	trsp = (trs_t *) q->q_ptr;

	dlp = (union DL_primitives *) mp->b_rptr;
	type = dlp->physaddr_req.dl_addr_type;
	trdp = trsp->trs_dev;

	if (trdp == NULL) {
		*err = DL_OUTSTATE;
		*uerr = 0;
		return (TRE_ERR);
	}

	switch (type) {
	case DL_FACT_PHYS_ADDR:
		bcopy((caddr_t)trdp->trd_factaddr,
			(caddr_t)&addr, MAC_ADDR_LEN);
		break;
	case DL_CURR_PHYS_ADDR:
		bcopy((caddr_t)trdp->trd_macaddr,
			(caddr_t)&addr, MAC_ADDR_LEN);
		break;
	default:
		*err = DL_NOTSUPPORTED;
		*uerr = 0;
		return (TRE_ERR);
	}

	dlphysaddrack(q, mp, (caddr_t) &addr, ETHERADDRL);
	return (TRE_OK);
}

/*
 * trattach - DLPI DL_ATTACH_REQ this attaches the stream to a PPA
 */
int
trattach(queue_t *q, mblk_t *mp, int *err, int *uerr)
{
	dl_attach_req_t *at;
	trs_t 		*trsp = (trs_t *) q->q_ptr;
	trd_t 		*trdp;
	cmdq_t		*qopen;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:DL_ATT, stream %x, mp %x", trsp, mp);
#endif

	at = (dl_attach_req_t *) mp->b_rptr;

	if ((trsp->trs_state != DL_UNATTACHED) &&
		(trsp->trs_state != DL_ATTACH_PENDING)) {
		*err = DL_OUTSTATE;
		*uerr = 0;
		return (TRE_ERR);
	}

	/*
	 * Valid ppa?
	 */
	if (trsp->trs_state == DL_UNATTACHED) {
		for (trdp = trDevices; trdp; trdp = trdp->trd_nextd)
			if (at->dl_ppa == trdp->trd_ppanum)
				break;
		if (trdp == NULL) {
			*err = DL_BADPPA;
			*uerr = 0;
			return (TRE_ERR);
		}
		/*
		 * Set link to device and update our state.
		 */
		trsp->trs_dev = trdp;
		trsp->trs_state = DL_ATTACH_PENDING;
	} else {
#ifdef TR_DEBUG
		if (tr_debug & TRTRACE)
			cmn_err(CE_CONT, "DL_ATT, second pass %x\n", trsp);
#endif
		trdp = trsp->trs_dev;
	}

	trdp->open_trsp = trsp;
	/*
	 * After init is complete, an open adapter must be performed
	 * This should only be done once for the device.
	 */
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "Opened?;");
#endif
	mutex_enter(&trdp->trd_intrlock);
	if (!(trdp->trd_flags & (TR_OPEN | TR_WOPEN | TR_OPENFAIL))) {
		if (qopen = (cmdq_t *)kmem_alloc(sizeof (cmdq_t),
						    KM_NOSLEEP)) {
			qopen->cmd = DIR_OPEN_ADP;
			qopen->trdp = trdp;
			qopen->trsp = trsp;
			qopen->callback = trwakewaiters;
			qopen->callback_arg = trdp;
			trqcmd(qopen);
			traddwaiter(trdp, trsp);
			(void) putbq(q, mp);
			mutex_exit(&trdp->trd_intrlock);
			return (TRE_SLEEP);
		}
		cmn_err(CE_WARN, "TR:no space for open cmd!!");
		*err = DL_SYSERR;
		*uerr = ENOMEM;
		mutex_exit(&trdp->trd_intrlock);
		return (TRE_ERR);
	}
	mutex_exit(&trdp->trd_intrlock);

	/*
	 * If the open is not yet complete, we have to wait
	 * for it.
	 */
	mutex_enter(&trdp->trd_intrlock);
	if (trdp->trd_flags & TR_WOPEN) {
		traddwaiter(trdp, trsp);
		(void) putbq(q, mp);
		mutex_exit(&trdp->trd_intrlock);
#ifdef TR_DEBUG
		if (tr_debug & TRTRACE)
			cmn_err(CE_CONT, "WaitOpen;");
#endif
		return (TRE_SLEEP);
	}
	mutex_exit(&trdp->trd_intrlock);

	if (trdp->trd_flags & TR_OPENFAIL) {
		cmn_err(CE_WARN, "TR:Adapter Open FAILED!");
		*err = DL_INITFAILED;
		*uerr = 0;
		return (TRE_ERR);
	}

	if (trdp->trd_flags & TR_OPEN) {
#ifdef TR_DEBUG
		if (tr_debug & TRTRACE)
			cmn_err(CE_CONT, "Opend!;");
#endif
		trsp->trs_state = DL_UNBOUND;
		dlokack(q, mp, DL_ATTACH_REQ);
		return (TRE_OK);
	}

	cmn_err(CE_CONT, "!TR:Bummer. Why am I here???");
	return (TRE_OK);
}

/*
 * trunattach - DLPI DL_DETACH_REQ detaches the stream from a device
 */
trunattach(queue_t *q, mblk_t *mp, int *err, int *uerr)
{
	trs_t *trsp = (trs_t *) q->q_ptr;
	trd_t *trdp = trsp->trs_dev;
	int	state = trsp->trs_state;
	int	sap;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "DL_DET stream %x", trsp);
#endif

	if (state != DL_UNBOUND && state != DL_IDLE &&
	    state != DL_DETACH_PENDING) {
		*err = DL_OUTSTATE;
		*uerr = 0;
		return (TRE_ERR);
	}

	trsp->trs_state = DL_DETACH_PENDING;

	/* board still bound. Must unbind */
	if (state == DL_IDLE) {
		mutex_enter(&trdp->trd_intrlock);
		trsp->trs_flags &= ~TRS_BOUND;
		sap = trsp->trs_802sap;
		trsp->trs_802sap = trsp->trs_usersap = LLC_NULL_SAP;
		mutex_exit(&trdp->trd_intrlock);
		trunbindnoq(trdp, sap);
	}

	if (trsp->trs_multimask) {
		mutex_enter(&trdp->trd_intrlock);
		trsp->trs_multimask = 0;
		trchgmultinoq(trdp);
		mutex_exit(&trdp->trd_intrlock);
	}

	trsp->trs_state = DL_UNATTACHED;
	trsp->trs_dev = NULL;
	if (mp) {
		dlokack(q, mp, DL_DETACH_REQ);
	}
	return (TRE_OK);
}

/*
 * The following is used to determine if the code is running on a Micro
 * Channel machine. Returns 1 on a Micro Channel machine, 0 otherwise.
 */
static int
micro_channel(dev_info_t *devi)
{
	static int	return_val;
	static int	not_first_call = 0;
	char		parent_type[16];
	int		len = sizeof (parent_type);

	if (not_first_call)
		return (return_val);

	if (ddi_getlongprop_buf(DDI_DEV_T_NONE, ddi_get_parent(devi),
		DDI_PROP_DONTPASS, "device_type", (caddr_t)parent_type, &len)
		!= DDI_PROP_SUCCESS) {
#ifdef TR_DEBUG
		if (tr_debug & TRERRS)
			cmn_err(CE_CONT, "failed to get device_type");
#endif
		return (0);
	}
	if (strcmp(parent_type, DEVI_MCA_NEXNAME))
		return_val = 0;
	else
		return_val = 1;
	not_first_call = 1;
	return (return_val);
}

/*
 *	Swab swaps byte in a word
 */
static ushort_t
trswab(ushort_t w_to_swab)
{
	ushort_t	temp;
	uchar_t	*ctmp;

	ctmp = (uchar_t *) &w_to_swab;

	temp = 0;
	temp = (ushort_t) ctmp[0];
	temp <<= 8;
	temp |= (ushort_t) ctmp[1];
	return (temp);
}

/*
 * trcloseboard - execute board command to shut down adapter
 */
static void
trcloseboard(trd_t *trdp)
{
	struct dir_cmd *dc;

	dc = (struct dir_cmd *) trdp->trd_srb;
	dc->dc_cmd = DIR_CLOSE_ADP;
	dc->dc_retcode = NULL_RC;
	MMIO_SETBIT(ISRA + 1, ISRA_O_SRB_CMD);
}

/*
 * tropenboard -- This function starts the onboard DIR.OPEN.ADAPTER
 */
static
void
tropenboard(trd_t *trdp)
{
	struct dir_open_adp *oa;
	struct dir_open_res *or;

	trdp->trd_flags |= TR_WOPEN;
	oa = (struct dir_open_adp *) trdp->trd_srb;

	oa->oa_cmd = DIR_OPEN_ADP;
	oa->oa_openops = TR_OPEN_OPTS;
	bcopy((caddr_t)trdp->trd_macaddr, (caddr_t)oa->oa_node, MAC_ADDR_LEN);
	bcopy((caddr_t)trdp->trd_groupaddr + 2, (caddr_t)oa->oa_group,
		MAC_ADDR_LEN - 2);
	bcopy((caddr_t)trdp->trd_multiaddr + 2, (caddr_t)oa->oa_func,
		MAC_ADDR_LEN - 2);
	oa->oa_num_rcv_buf = trswab((ushort_t)trdp->trd_numrcvs);
	oa->oa_rcv_buf_len = trswab(TR_RCVBUF_LEN);
	oa->oa_dhb_len = trswab((ushort_t)trdp->trd_dhbsize);
	oa->oa_num_dhb = trdp->trd_numdhb;
	oa->oa_dlc_max_sap = trdp->trd_maxsaps;
	oa->oa_dlc_max_sta = 0;
	oa->oa_dlc_max_gsap = 0;
	oa->oa_dlc_max_gmem = 0;
	oa->oa_dlc_T1_1 = 0;
	oa->oa_dlc_T2_1 = 0;
	oa->oa_dlc_TI_1 = 0;
	oa->oa_dlc_T1_2 = 0;
	oa->oa_dlc_T2_2 = 0;
	oa->oa_dlc_TI_2 = 0;

	or = (struct dir_open_res *)oa;
	or->or_retcode = NULL_RC;

	MMIO_SETBIT(ISRA + 1, ISRA_O_SRB_CMD);
}

/*
 * trremstr -- Remove a stream from the global list of active streams
 */
void
trremstr(trs_t *trs)
{
	trs_t	*loops, *prevs = NULL;

	loops = trStreams;
	while (loops) {
		if (loops == trs)
			break;
		else {
			prevs = loops;
			loops = loops->trs_nexts;
		}
	}

	if (loops && prevs) {
		prevs->trs_nexts = loops->trs_nexts;
	} else if (loops) {
		trStreams = loops->trs_nexts; /* stream was first in the list */
	}
}

/*
 * tropendone -- Handle completion of on-board DIR.OPEN.ADAPTER command.
 */
static
void
tropendone(trdp)
	struct trd *trdp;
{
	struct dir_open_res 	*or;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "OCD&");
#endif
	or = (struct dir_open_res *) trdp->trd_srb;
	switch (or->or_retcode) {
	case OK_RC:
	case NULL_RC:
		or->or_retcode = NULL_RC;
		trdp->trd_srb = (struct srb *) ((caddr_t)trdp->trd_sramaddr +
						trswab(or->or_srb_addr));
		trdp->trd_ssb = (struct ssb *)((caddr_t)trdp->trd_sramaddr +
						trswab(or->or_ssb_addr));
		trdp->trd_arb = (struct arb *)((caddr_t)trdp->trd_sramaddr +
						trswab(or->or_arb_addr));
		trdp->trd_asb = (struct asb *)((caddr_t)trdp->trd_sramaddr +
						trswab(or->or_asb_addr));
		trdp->trd_flags &= ~TR_WOPEN;
		trdp->trd_flags |= TR_OPEN;
		trnxtcmd(trdp);
		return;
	case CMD_CNCL_RC:
		if (!(trdp->trd_srb->srb[SRB_INIT2] & 0x4)) {
			cmn_err(CE_CONT, "TR: adp speed must match ring speed");
			break;
		}
		if ((or->or_errcode == 0x2400) &&
		    (trdp->tr_open_retry < TR_OPEN_MAX_RETRY)) {
			trdp->trd_flags &= ~TR_WOPEN;
			trdp->tr_open_retry++;
			trnxtcmd(trdp);
			return;
		} else if (or->or_errcode == 0x2400) {
			cmn_err(CE_CONT, "TR: Too many retries on open cmd");
			break;
		} else if (or->or_errcode == 0x2D00) {
			cmn_err(CE_CONT, "TR: No monitor present");
			break;
		}
		break;
	default:
		break;
	}

	/*
	 * Disable interrupts
	 */
	cmn_err(CE_WARN, "TR:Disabling INTRs due to open fail!!");
	MMIO_RESETBIT(ISRP, (uchar_t) ISRP_E_IENB);
	trdp->trd_flags &= ~TR_WOPEN;
	trdp->trd_flags |= TR_OPENFAIL;
	cmn_err(CE_WARN, "TR:OPEN FAIL!(0x%x)", or->or_retcode);
	trnxtcmd(trdp);
}

/*
 * trbind - bind a stream to a SAP (if allowed)
 *          I no longer will have the stream wait for the
 *          hardware bind command to complete or even succeed. The
 *          sap should be bound by the time anyone tries to send from
 *          it and if it isn't that won't matter because the packet
 *          can be sent from the direct station (see trruncmd() for
 *          further details). If the hardware bind fails, then
 *          that still isn't tragic for the same reason (The only downfall
 *          being that TEST frames sent to that sap will not receive the
 *          automatic responses the board normally would send.  This is of
 *          course absolutely broken behavior but the benefits of being able
 *          to continue transmitting and receiving regular frames IMO far
 *          outweighs the loss of TEST response).
 */
static int
trbind(queue_t *q, mblk_t *mp, int *err, int *uerr)
{
	register dl_bind_req_t	*dlp;
	int			sap;
	trs_t			*trsp = (trs_t *) q->q_ptr;
	trd_t 			*trdp;
	cmdq_t			*qsopen;
	ulong_t 		xidack = 0;

	ASSERT(trsp);
	ASSERT(WR(trsp->trs_rq) == q);

	dlp = (dl_bind_req_t *) mp->b_rptr;
	sap = dlp->dl_sap;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TRBIND:0x%x, stream %x\n", sap, trsp);
#endif

	trdp = trsp->trs_dev;
	trdp->bind_trsp = trsp;

	if (trsp->trs_state != DL_UNBOUND) {
#ifdef TR_DEBUG
		if (tr_debug & TRERRS) {
			cmn_err(CE_CONT, "TR:Already bound or");
			cmn_err(CE_CONT, "not attached (%d)", trsp->trs_state);
		}
#endif
		*err = DL_OUTSTATE;
		*uerr = 0;
		return (TRE_ERR);
	}
	if (dlp->dl_service_mode != DL_CLDLS) {
		*err = DL_UNSUPPORTED;
		*uerr = 0;
		return (TRE_ERR);
	}
	if ((sap == LLC_NULL_SAP) ||
	    ((sap >= TR_MAX802SAP) &&
	    (sap < TR_MINSNAPSAP || sap > TR_MAXSNAPSAP))) {
		*err = DL_BADADDR;
		*uerr = 0;
		return (TRE_ERR);
	}

	/*
	 * Large sap values are automatically assumed to be SNAP saps
	 * which should be routed through the LLC sap reserved for SNAP
	 */
	trsp->trs_usersap = sap;
	if (sap <= TR_MAX802SAP) {
		trsp->trs_type = DL_802;
		trsp->trs_802sap = sap;
	} else {
		trsp->trs_type = DL_SNAP;
		trsp->trs_802sap = LLC_SNAP_SAP;
	}

	sap = trsp->trs_802sap;
	mutex_enter(&trdp->trd_intrlock);
	if (!(trdp->trd_saptab[sap].sin_refcnt)) {
		if (qsopen = (cmdq_t *)kmem_alloc(sizeof (cmdq_t),
						KM_NOSLEEP)) {
			qsopen->cmd = DLC_OPEN_SAP;
			qsopen->trdp = trdp;
			qsopen->trsp = trsp;
			qsopen->arg = sap;
			qsopen->callback = NULL;
			trqcmd(qsopen);
		} else {
			cmn_err(CE_NOTE, "TR:No memory for Bind command!");
		}
	}
	trdp->trd_saptab[sap].sin_refcnt++;
	trsp->trs_state = DL_IDLE;	/* bound and ready */
	trsp->trs_flags |= TRS_BOUND;
	if (dlp->dl_xidtest_flg & DL_AUTO_XID) {
		trsp->trs_flags |= TRS_AUTO_XID;
		xidack |= DL_AUTO_XID;
	}
	/*
	 * Board does AUTO_TEST for us and the driver can't prevent it
	 * from doing so - so I must set the flag.
	 */
	trsp->trs_flags |= TRS_AUTO_TEST;
	xidack |= DL_AUTO_TEST;

	/* ACK the BIND */
	dlbindack(q, mp, trsp->trs_usersap, trdp->trd_macaddr,
			MAC_ADDR_LEN, 0, xidack);
	mutex_exit(&trdp->trd_intrlock);
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:EXBIND[ref=%d]",
			trdp->trd_saptab[sap].sin_refcnt);
#endif
	return (TRE_OK);
}

/*
 * tropensap - send the board the command to open a particular SAP
 */
static void
tropensap(trd_t *trdp, long sap)
{
	struct dlc_open_sap *os;

	trdp->trd_saptab[sap].sin_status |= TRS_WBIND;
	os = (struct dlc_open_sap *) trdp->trd_srb;
	os->os_cmd = DLC_OPEN_SAP;
	os->os_retcode = NULL_RC;
	os->os_stn_id = 0;
	os->os_t1 = 0;
	os->os_t2 = 0;
	os->os_ti = 0;
	os->os_maxout = 0;
	os->os_maxin = 0;
	os->os_maxout_incr = 0;
	os->os_max_retry = 0;
	os->os_gsap_max =  0;
	os->os_max_ifield = 0;
	os->os_sap = (uchar_t)sap;
	os->os_sap_opts = OS_INDV_SAP | OS_XID;
	os->os_stn_cnt = 0;
	os->os_gsap_num = 0;

	MMIO_SETBIT(ISRA + 1, ISRA_O_SRB_CMD);
}

/*
 * tropensapdone - Called by the interrupt routine when an open_sap
 *		   is completed.
 */
static void
tropensapdone(trd_t *trdp)
{
	struct dlc_open_sap *os;
	long	sap;

	os = (struct dlc_open_sap *) trdp->trd_srb;
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "SAPDONE:0x%x,rc=%x", os->os_sap,
			os->os_retcode);
#endif

	/* May need to loop through saptab here to see who's waiting */
	sap = os->os_sap;

	switch (os->os_retcode) {
	case OS_OK:
		trdp->trd_saptab[sap].sin_status &= ~TRS_WBIND;
		trdp->trd_saptab[sap].sin_status |= TRS_BOUND;
		trdp->trd_saptab[sap].sin_station_id = os->os_stn_id;
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "tropensapdone: sap = %x, station_id = %x\n",
			os->os_sap, os->os_stn_id);
#endif
		break;

	case OS_NOTOPEN:
#ifdef TR_DEBUG
		if (tr_debug & (TRTRACE | TRERRS))
			cmn_err(CE_CONT, "AdpNotOpen ");
#endif
		trdp->trd_saptab[sap].sin_status &= ~TRS_WBIND;
		trdp->trd_saptab[sap].sin_status |= TRS_BINDFAILED;
		trdp->trd_saptab[sap].sin_reason = DL_NOTINIT;
		trdp->trd_saptab[sap].sin_ureason = 0;
		break;

	case OS_INVCMD:
	case OS_BADOPTION:
	case OS_TOOBIG:
	case OS_NOMEM:
#ifdef TR_DEBUG
		if (tr_debug & (TRTRACE | TRERRS))
			cmn_err(CE_CONT, "BadCmd ");
#endif
		trdp->trd_saptab[sap].sin_status &= ~TRS_WBIND;
		trdp->trd_saptab[sap].sin_status |= TRS_BINDFAILED;
		trdp->trd_saptab[sap].sin_reason = DL_SYSERR;
		trdp->trd_saptab[sap].sin_ureason = EIO;
		break;

	case OS_PERM:
#ifdef TR_DEBUG
		if (tr_debug & (TRTRACE | TRERRS))
			cmn_err(CE_CONT, "BadAccess ");
#endif
		trdp->trd_saptab[sap].sin_status &= ~TRS_WBIND;
		trdp->trd_saptab[sap].sin_status |= TRS_BINDFAILED;
		trdp->trd_saptab[sap].sin_reason = DL_ACCESS;
		trdp->trd_saptab[sap].sin_ureason = 0;
		break;

	case OS_BADSAP:
#ifdef TR_DEBUG
		if (tr_debug & (TRTRACE | TRERRS))
			cmn_err(CE_CONT, "BadSAP ");
#endif
		trdp->trd_saptab[sap].sin_status &= ~TRS_WBIND;
		trdp->trd_saptab[sap].sin_status |= TRS_BINDFAILED;
		trdp->trd_saptab[sap].sin_reason = DL_BADSAP;
		trdp->trd_saptab[sap].sin_ureason = 0;
		break;

	case OS_NOGROUP:
	case OS_GROUPFULL:
#ifdef TR_DEBUG
		if (tr_debug & (TRTRACE | TRERRS))
			cmn_err(CE_CONT, "BadGroup ");
#endif
		trdp->trd_saptab[sap].sin_status &= ~TRS_WBIND;
		trdp->trd_saptab[sap].sin_status |= TRS_BINDFAILED;
		trdp->trd_saptab[sap].sin_reason = DL_NOADDR;
		trdp->trd_saptab[sap].sin_ureason = 0;
		break;
	default:
#ifdef TR_DEBUG
		if (tr_debug & (TRTRACE | TRERRS))
			cmn_err(CE_CONT, "return code[%d] ", os->os_retcode);
#endif
		trdp->trd_saptab[sap].sin_status &= ~TRS_WBIND;
		trdp->trd_saptab[sap].sin_status |= TRS_BINDFAILED;
		trdp->trd_saptab[sap].sin_reason = DL_SYSERR;
		trdp->trd_saptab[sap].sin_ureason = EIO;
		break;
	}

	trnxtcmd(trdp);
}

/*
 * trunbind - frees the binding of the stream to a SAP. Stream is left open.
 */
static int
trunbind(queue_t *q, mblk_t *mp, int *err, int *uerr)
{
	trs_t				*trsp = (trs_t *) q->q_ptr;
	trd_t 				*trdp;
	int				sap;

	ASSERT(trsp);
	ASSERT(WR(trsp->trs_rq) == q);
	sap = trsp->trs_802sap;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:UNBIND[%x], stream %x", sap, trsp);
#endif

	trdp = trsp->trs_dev;
	if (trsp->trs_state != DL_IDLE) {
		*err = DL_OUTSTATE;
		*uerr = 0;
		return (TRE_ERR);
	}

	trunbindnoq(trdp, sap);
	/* ACK the UNBIND */
	trsp->trs_state = DL_UNBOUND;
	trsp->trs_flags &= ~TRS_BOUND;
	trsp->trs_flags &= ~TRS_AUTO_XID;
	trsp->trs_flags &= ~TRS_AUTO_TEST;
	trsp->trs_802sap = trsp->trs_usersap = LLC_NULL_SAP;
	dlokack(q, mp, DL_UNBIND_REQ);
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:EXUBIND[ref=%d]",
			trdp->trd_saptab[sap].sin_refcnt);
#endif
	return (TRE_OK);
}

/*
 * trunbindnoq - frees the binding of the stream to a SAP. No streams involved.
 */
static void
trunbindnoq(trd_t *trdp, long sap)
{
	cmdq_t	*qsclose;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:UNBINDNOQ [%x]\n", sap);
#endif

	mutex_enter(&trdp->trd_intrlock);
	if (!(--trdp->trd_saptab[sap].sin_refcnt)) {
		if (qsclose = (cmdq_t *) kmem_alloc(sizeof (cmdq_t),
						    KM_NOSLEEP)) {
			qsclose->cmd = DLC_CLOSE_SAP;
			qsclose->trdp = trdp;
			qsclose->trsp = NULL;
			qsclose->arg = sap;
			qsclose->callback = NULL;
			trqcmd(qsclose);
			mutex_exit(&trdp->trd_intrlock);
			return;
		}
		cmn_err(CE_WARN, "TR:No memory for UnBindnoq command!");
		mutex_exit(&trdp->trd_intrlock);
		return;
	}
	mutex_exit(&trdp->trd_intrlock);
}

/*
 * trclosesap - Send the board the command to close a SAP
 */
static void
trclosesap(trd_t *trdp, long sap)
{
	struct dlc_close_sap *cs;

	trdp->trd_saptab[sap].sin_status |= TRS_WUBIND;
	cs = (struct dlc_close_sap *) trdp->trd_srb;
	cs->cs_cmd = DLC_CLOSE_SAP;
	cs->cs_retcode = NULL_RC;
	cs->cs_stn_id = trdp->trd_saptab[sap].sin_station_id;
	MMIO_SETBIT(ISRA + 1, ISRA_O_SRB_CMD);
}

/*
 * trclosesapdone - Called by interrupt routine when board finished with close
 */
static void
trclosesapdone(trd_t *trdp)
{
	struct dlc_close_sap *cs;
	long	sap;

	cs = (struct dlc_close_sap *) trdp->trd_srb;

	/* Loop through saptab to see who's waiting */
	for (sap = 0;
		(sap <= TR_MAX802SAP &&
		    !(trdp->trd_saptab[sap].sin_status & TRS_WUBIND));
		sap++) {
		/* NULL */
	}

	if (sap > TR_MAX802SAP)
		return;

	switch (cs->cs_retcode) {
	case OS_OK:
		trdp->trd_saptab[sap].sin_status &= ~TRS_WUBIND;
		trdp->trd_saptab[sap].sin_status &= ~TRS_BOUND;
		trdp->trd_saptab[sap].sin_station_id = LLC_NULL_SAP;
		break;

	case OS_NOTOPEN:
#ifdef TR_DEBUG
		if (tr_debug & (TRTRACE | TRERRS))
			cmn_err(CE_CONT, "TR:UBF - 0x%x ", cs->cs_retcode);
#endif
		trdp->trd_saptab[sap].sin_status &= ~TRS_WUBIND;
		trdp->trd_saptab[sap].sin_status |= TRS_UNBNDFAILED;
		trdp->trd_saptab[sap].sin_reason = DL_NOTINIT;
		trdp->trd_saptab[sap].sin_ureason = 0;
		break;

	case OS_INVCMD:
	case OS_BADID:
	case OS_LINKS:
	case OS_NOTLAST:
	case OS_NOTDONE:
	default:
#ifdef TR_DEBUG
		if (tr_debug & (TRTRACE | TRERRS))
			cmn_err(CE_CONT, "TR:UBF - 0x%x ", cs->cs_retcode);
#endif
		trdp->trd_saptab[sap].sin_status &= ~TRS_WUBIND;
		trdp->trd_saptab[sap].sin_status |= TRS_UNBNDFAILED;
		trdp->trd_saptab[sap].sin_reason = DL_SYSERR;
		trdp->trd_saptab[sap].sin_ureason = EIO;
		break;
	}

	trnxtcmd(trdp);
}

/*
 * traddwaiter --  Adds a stream to the list of streams waiting on the device
 */
static void
traddwaiter(trd_t *trdp, trs_t *trsp)
{
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:ADDW %x\n", trsp);
#endif
	if (!(trsp->trs_flags & TRS_SLEEPER)) {
		trsp->trs_flags |= TRS_SLEEPER;
		if (trdp->trd_pending)
			trsp->trs_nextwait = trdp->trd_pending;
		else
			trsp->trs_nextwait = NULL;
		trdp->trd_pending = trsp;
		noenable(WR(trsp->trs_rq));
	}
}

/*
 * traddholder -- Adds a stream to the list of streams waiting to queue a xmit
 */
static void
traddholder(trd_t *trdp, trs_t *trsp)
{
#ifdef TR_DEBUG
	if (tr_debug & (TRTRACE | TRSEND))
		cmn_err(CE_CONT, "TR:ADDH %x\n", trsp);
#endif

	if (!(trsp->trs_flags & TRS_WTURN)) {
		trsp->trs_flags |= TRS_WTURN;
		trsp->trs_nexthold = NULL;
		if (trdp->trd_onhold) {
			trdp->trd_onholdtail->trs_nexthold = trsp;
			trdp->trd_onholdtail = trsp;
		} else {
			trdp->trd_onhold = trdp->trd_onholdtail = trsp;
		}
		noenable(WR(trsp->trs_rq));
	}
}

/*
 * trremwaiter -- Removes a stream from any list of streams waiting
 *                on the device (If its there).
 */
static void
trremwaiter(trd_t *trdp, trs_t *trsp)
{
	trs_t	*prevp = NULL;
	trs_t	*loopp = trdp->trd_pending;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:REMW %x\n", trsp);
#endif

	/* First check the pending on command q */
	while (loopp) {
		if (loopp == trsp)
			break;
		prevp = loopp;
		loopp = loopp->trs_nextwait;
	}
	if (loopp && prevp) {
		prevp->trs_nextwait = loopp->trs_nextwait;
	} else if (loopp) {
		trdp->trd_pending = loopp->trs_nextwait;
	}

	/* Now check the waiting to q a xmit list */
	loopp = trdp->trd_onhold;
	prevp = NULL;
	while (loopp) {
		if (loopp == trsp)
			break;
		prevp = loopp;
		loopp = loopp->trs_nexthold;
	}
	if (loopp && prevp) {
		prevp->trs_nexthold = loopp->trs_nexthold;
	} else if (loopp) {
		trdp->trd_onhold = loopp->trs_nexthold;
	}
}

/*
 * trwakewaiters -- Qenable all streams on the device waiting list and clear
 *		    the list.
 */
static void
trwakewaiters(trd_t *trdp)
{
	while (trdp->trd_pending) {
#ifdef TR_DEBUG
		if (tr_debug & TRTRACE)
			cmn_err(CE_CONT, "TR:ENAB %x\n", trdp->trd_pending);
#endif
		trdp->trd_pending->trs_flags &= ~TRS_SLEEPER;
		enableok(WR(trdp->trd_pending->trs_rq));
		qenable(WR(trdp->trd_pending->trs_rq));
		trdp->trd_pending = trdp->trd_pending->trs_nextwait;
	}
}

/*
 * trwakeholder -- Qenable the first stream on the list waiting to queue a xmit
 */
static void
trwakeholder(trd_t *trdp)
{
	trdp->trd_xmitcnt--;
	if (trdp->trd_onhold) {
#ifdef TR_DEBUG
		if (tr_debug & (TRTRACE | TRSEND))
			cmn_err(CE_CONT, "TR:ENABH %x\n", trdp->trd_onhold);
#endif
		trdp->trd_onhold->trs_flags &= ~TRS_WTURN;
		enableok(WR(trdp->trd_onhold->trs_rq));
		qenable(WR(trdp->trd_onhold->trs_rq));
		trdp->trd_onhold = trdp->trd_onhold->trs_nexthold;
	}
}

/*
 * trrcv - called directly by interrupt handler after frame has been received.
 *	It grabs the message directly off of the board and stuffs it in an
 *	mblk. Responds to the board to indicate frame received. Calls
 *	trrcvmunch() for further processing. Will drop clearly bad frames
 *	out of hand.
 */
static void
trrcv(trd_t *trdp)
{
	struct adp_rcv_data *rd;	/* receive data */
	struct rcv_data_res *res;	/* data response */
	struct rcv_buf *rb;		/* receive buffer */
	ushort_t	len;
	ushort_t	ptradj;
	uchar_t *dp, *sp;
	mblk_t	*bp;
	ushort_t	pkt_type;
	struct llc_info	pkt_info;

	trdp->trd_statsd->trc_ipackets++;
	rd = (struct adp_rcv_data *)trdp->trd_arb;
	res = (struct rcv_data_res *)trdp->trd_asb;
#ifdef TR_DEBUG
	if (tr_debug & TRRECV) {
		cmn_err(CE_CONT, "RCV stn_id (0x%x) ", rd->rd_stn_id);
		cmn_err(CE_CONT, "lan_hdr_len (0x%x) ", rd->rd_lan_hdr_len);
		cmn_err(CE_CONT, "dlc_hdr_len (0x%x)", rd->rd_dlc_hdr_len);
		cmn_err(CE_CONT, "frame_len (0x%x) ", trswab(rd->rd_frame_len));
		cmn_err(CE_CONT, "type (0x%x)\n", rd->rd_ncb_type);
	}
#endif
	/*
	 * set up the response, but do not interrupt the adapter
	 * until we are done processing the data
	 */
	res->dr_cmd = rd->rd_cmd;
	res->dr_retcode = OK_RC;
	res->dr_stn_id = rd->rd_stn_id;
	res->dr_rcv_buf = rd->rd_rcv_buf;

	len = trswab(rd->rd_frame_len);
	if (len > trdp->trd_dhbsize) {
#ifdef TR_DEBUG
		if (tr_debug & (TRRECV | TRERRS))
			cmn_err(CE_CONT, "TR: Rcv drop, bad len\n");
#endif
		MMIO_SETBIT(ISRA + 1, ISRA_O_ASB_RES);
		trdp->trd_statsd->trc_ierrors++;
		return;
	}

	/*
	 * Get a buffer from streams and copy packet. We want to try
	 * to align IP headers on a word boundary, so I'm going to get
	 * 3 more bytes than I need. That way I have leeway to move
	 * rptr to align the IP headers.  We want to align IP headers
	 * to avoid an ip_rput_pullup which costs extra time.
	 */
	if ((bp = allocb(len + 3, BPRI_MED)) == NULL)  {
#ifdef TR_DEBUG
		if (tr_debug & (TRRECV | TRERRS))
			cmn_err(CE_CONT, "TR: Rcv drop, no buffer\n");
#endif
		trdp->trd_statsd->trc_allocbfail++;
		trdp->trd_statsd->trc_norbufs++;
		trdp->trd_statsd->trc_ierrors++;
		MMIO_SETBIT(ISRA + 1, ISRA_O_ASB_RES);
		return;
	}
	DB_TYPE(bp) = M_DATA;

	/*
	 * transfer the data from the Shared RAM to the newly allocated
	 * stream mblk_t
	 */
	rb = (struct rcv_buf *)
		((caddr_t)trdp->trd_sramaddr + trswab(rd->rd_rcv_buf));

	/*
	 * Align rptr & wptr so that IP headers will be on word boundaries
	 * in the most common case.  This hopefully will eliminate an
	 * unnecessary ip_rput_pullup.
	 */
	ptradj = (((unsigned)bp->b_rptr + LLC_EHDR_SIZE)%4);
	dp = bp->b_wptr = (bp->b_rptr += ptradj);
	bp->b_wptr += len;
	while (len != 0) {
		register int buflen;

		sp = rb->rb_fr_data;
		buflen = trswab(rb->rb_buf_len);
		if (buflen > (int)len) {
#ifdef TR_DEBUG
			if (tr_debug & (TRRECV | TRERRS)) {
				cmn_err(CE_CONT, "TR: Rcv drop,");
				cmn_err(CE_CONT, "bad total length\n");
			}
#endif
			trdp->trd_statsd->trc_ierrors++;
			freemsg(bp);
			bp = NULL;
			MMIO_SETBIT(ISRA + 1, ISRA_O_ASB_RES);
			return;
		}
		bcopy((caddr_t)sp, (caddr_t)dp, buflen);
		dp += buflen;
		len -= buflen;
		/* the next pointer is actually 2 bytes ahead */
		rb = (struct rcv_buf *) ((caddr_t)trdp->trd_sramaddr +
						trswab(rb->rb_next) - 2);
	}
	if (len != 0) {
#ifdef TR_DEBUG
		if (tr_debug & (TRRECV | TRERRS)) {
			cmn_err(CE_CONT, "TR:Rcv drop, ");
			cmn_err(CE_CONT, "not enough data (%d)\n", len);
		}
#endif
		trdp->trd_statsd->trc_ierrors++;
		freemsg(bp);
		bp = NULL;
		MMIO_SETBIT(ISRA + 1, ISRA_O_ASB_RES);
		return;
	}
	/* Parse the LLC header and get information back */
	pkt_type = parsellc(bp, &pkt_info);
	trrcvmunch(bp, trdp, rd->rd_stn_id, pkt_type, &pkt_info);
	MMIO_SETBIT(ISRA + 1, ISRA_O_ASB_RES);
}

/*
 *  trrcvmunch -- Munch on the packet just retrieved from the board's receive
 *	buffer.  Decide here which streams get a copy of the message.
 */
static void
trrcvmunch(mblk_t *mp, trd_t *trdp, long stn, ushort_t pkt_type,
		struct llc_info *pkt_info)
{
	register int	forme, fromme, multiforme, msgsap;
	trs_t 		*lps;
	mblk_t 		*nmp;
	int		statcnt_normal = 0;
	int		statcnt_brdcst = 0;
	int		statcnt_multi = 0;

#ifdef TR_DEBUG
	if (tr_debug & (TRRECV | TRTRACE))
		cmn_err(CE_CONT, "TR:Rcv munch,");
#endif

	/* Update source routing info */
	trupdatert(trdp, pkt_type, pkt_info);

	nmp = NULL;
	msgsap = pkt_info->dsap;

	forme = 0;
	if (tr_broadcast(pkt_info)) {
#ifdef TR_DEBUG
		if (tr_debug & (TRRECV | TRTRACE))
			cmn_err(CE_CONT, "BDCAST.\n");
#endif
		forme = statcnt_brdcst = 1;
	} else if (ismulticast(pkt_info->mac_ptr->dhost)) {
#ifdef TR_DEBUG
		if (tr_debug & (TRRECV | TRTRACE))
			cmn_err(CE_CONT, "MCAST.\n");
#endif
		statcnt_multi = 1;
	} else {
		forme = tr_forme(pkt_info, trdp);
		statcnt_normal = msgdsize(mp);
	}

	fromme = tr_fromme(pkt_info, trdp);

	rw_enter(&trStreamsLock, RW_READER);
	for (lps = trStreams; lps && mp != NULL; lps = lps->trs_nexts) {

		/* Ignore streams not connected to this device */
		if (lps->trs_dev != trdp)
			continue;
#ifdef TR_DEBUG
		if (tr_debug & (TRRECV | TRTRACE)) {
			cmn_err(CE_CONT, "strm %x ", lps);
			cmn_err(CE_CONT, "type %x ", lps->trs_type);
			cmn_err(CE_CONT, "bndsap 0x%x ", lps->trs_usersap);
			cmn_err(CE_CONT, "pkt-dsap 0x%x\n", msgsap);
		}
#endif
		multiforme = statcnt_multi && tr_multicast(pkt_info, lps);

		if ((lps->trs_flags & TRS_PROMSAP) ||
		    ((statcnt_multi || statcnt_brdcst) &&
		    (lps->trs_flags & TRS_PROMMULTI))) {
			if (!canput(lps->trs_rq)) {
#ifdef TR_DEBUG
				if (tr_debug & (TRRECV | TRERRS)) {
					cmn_err(CE_CONT, "Upstream canput ");
					cmn_err(CE_CONT, "failed %x\n", lps);
				}
#endif
				lps->trs_dev->trd_statsd->trc_nocanput++;
				continue;
			}
			nmp = copymsg(mp);
			if (nmp)
				(void) putq(lps->trs_rq, nmp);
			continue;
		}

#define	SAP_STATION_ID	trdp->trd_saptab[lps->trs_802sap].sin_station_id

		if (((msgsap == LLC_GLOBAL_SAP) && (stn == SAP_STATION_ID)) ||
			(!fromme && (forme || multiforme) &&
			(lps->trs_usersap == msgsap))) {
#ifdef TR_DEBUG
			if (tr_debug & TRRECV) {
				cmn_err(CE_CONT, "PUTing packet on ");
				cmn_err(CE_CONT, "stream %x\n", lps);
			}
#endif
			if (!canput(lps->trs_rq)) {
#ifdef TR_DEBUG
				if (tr_debug & (TRRECV | TRERRS)) {
					cmn_err(CE_CONT, "Upstream canput ");
					cmn_err(CE_CONT, "failed %x\n", lps);
				}
#endif
				lps->trs_dev->trd_statsd->trc_nocanput++;
				continue;
			}

			nmp = dupmsg(mp);
			(void) putq(lps->trs_rq, mp);
			mp = nmp;
		}
	}
	rw_exit(&trStreamsLock);

	if (mp != NULL) {
		freemsg(mp);
		mp = NULL;
	}
	if (statcnt_brdcst) {
		trdp->trd_statsd->trc_brdcstrcv++;
	}
	if (statcnt_multi) {
		trdp->trd_statsd->trc_multircv++;
	}
	if (statcnt_normal) {
		trdp->trd_statsd->trc_rbytes += statcnt_normal;
	}
}

/*
 * parsellc --  Parses the llc header and returns information about the
 * 		received packet. It looks for routing information and gets
 *		the SAP out of the header. Returns pointers to the MAC
 *		header and the LLC portion of the header.
 */
static ushort_t
parsellc(mblk_t *mp, struct llc_info *pkt_info)
{
	ushort_t	pkt_type = 0;
	int 		hdrsize = 0;

	/*
	 * I assume that the header will be fully contained in the
	 * data section of the first mblk_t passed to me. The data,
	 * however, may start in a continuation mblk. This would
	 * only happen if we are parsing an outgoing packet.
	 */
	pkt_info->mac_ptr = (struct tr_mac_frm *) mp->b_rptr;

	if (pkt_info->mac_ptr->dhost[0] & TR_GR_ADDR)
		pkt_type |= DL_GROUP;
	if (pkt_info->mac_ptr->shost[0] & TR_SR_ADDR) {
		/* We have routing information get the size */
		pkt_type |= DL_ROUTE;
		pkt_info->rsize = pkt_info->mac_ptr->ri.len;
		pkt_info->ri_ptr = &(pkt_info->mac_ptr->ri);
		pkt_info->direction = pkt_info->mac_ptr->ri.dir;
	} else {
		pkt_info->rsize = 0;
		pkt_info->ri_ptr = NULL;
	}

	hdrsize = ACFCDASA_LEN + pkt_info->rsize;
	pkt_info->llc_ptr = (union llc_header *) (mp->b_rptr + hdrsize);
	pkt_info->ssap = pkt_info->llc_ptr->llc_sap.llc_ssap;
	pkt_info->dsap = pkt_info->llc_ptr->llc_sap.llc_dsap;
	pkt_info->control = pkt_info->llc_ptr->llc_sap.llc_control;

	if (pkt_info->control == LLC_UI) {
		if (pkt_info->dsap == LLC_SNAP_SAP) {
			pkt_info->snap =
				pkt_info->llc_ptr->llc_snap.ether_type;
			pkt_info->ssap =
				pkt_info->dsap = ntohs(pkt_info->snap);
			pkt_type |= DL_SNAP;
			hdrsize += LLC_SNAP_HDR_LEN;
		} else {
			pkt_type |= DL_802;
			hdrsize += LLC_8022_HDR_LEN;
		}
	} else {
		pkt_type |= DL_802;
		hdrsize += LLC_8022_HDR_LEN;
	}
	if (pkt_info->ssap & LLC_RESPONSE)
		pkt_type |= DL_RESPONSE;
	pkt_info->data_offset = hdrsize;
	return (pkt_type);
}

/*
 * tr_form_udata - format a DL_UNITDATA_IND message to be
 * sent to the user
 */
static void
tr_form_udata(trs_t *trsp, trd_t *trdp, ushort_t pkt_type,
		struct llc_info *info_ptr, mblk_t *mp)
{
	mblk_t *udmp;
	dl_unitdata_ind_t *udata;
	int	saplen;

#ifdef TR_DEBUG
	if (tr_debug & (TRRECV | TRTRACE))
		cmn_err(CE_CONT, "form udata, stream %x\n", trsp);
#endif
	saplen = (pkt_type & DL_SNAP) ? 2 : 1;

	/* allocate the DL_UNITDATA_IND M_PROTO header */
	udmp = allocb(sizeof (dl_unitdata_ind_t) + 2*MAC_ADDR_LEN + 2*saplen,
			BPRI_MED);

	if (udmp == NULL) {
		/* might as well discard since we can't go further */
#ifdef TR_DEBUG
		if (tr_debug & (TRERRS | TRRECV))
			cmn_err(CE_WARN, "TR:Allocb failure in form_udata %x\n",
				trsp);
#endif
		trdp->trd_statsd->trc_allocbfail++;
		freemsg(mp);
		mp = NULL;
		return;
	}

	/* Move the rptr past the header */
	adjmsg(mp, info_ptr->data_offset);

	/*
	 * now setup the DL_UNITDATA_IND header
	 */
	DB_TYPE(udmp) = M_PROTO;
	udata = (dl_unitdata_ind_t *) udmp->b_rptr;
	udmp->b_wptr += sizeof (dl_unitdata_ind_t);
	udata->dl_primitive = DL_UNITDATA_IND;
	udata->dl_dest_addr_length = MAC_ADDR_LEN + saplen;
	udata->dl_dest_addr_offset = sizeof (dl_unitdata_ind_t);

	bcopy((caddr_t) info_ptr->mac_ptr->dhost,
		((caddr_t) udata + udata->dl_dest_addr_offset),	MAC_ADDR_LEN);

	if (pkt_type & DL_802) {
		*((caddr_t) udata + udata->dl_dest_addr_offset +
			MAC_ADDR_LEN) = (uchar_t) info_ptr->dsap;
	} else {
		*((ushort_t *) ((caddr_t)udata + udata->dl_dest_addr_offset +
				MAC_ADDR_LEN)) = trswab(info_ptr->dsap);
	}
	udmp->b_wptr += udata->dl_dest_addr_length;

	udata->dl_src_addr_length = MAC_ADDR_LEN + saplen;
	udata->dl_src_addr_offset = udata->dl_dest_addr_offset +
					udata->dl_dest_addr_length;

	bcopy((caddr_t) info_ptr->mac_ptr->shost,
		((caddr_t) udata + udata->dl_src_addr_offset), MAC_ADDR_LEN);

	/* Take Route bit out so we send up ACTUAL source addr */
	if (pkt_type & DL_ROUTE)
		*((uchar_t *)udata + udata->dl_src_addr_offset) &= ~TR_SR_ADDR;

	if (pkt_type & DL_802) {
		*((caddr_t) udata + udata->dl_src_addr_offset + MAC_ADDR_LEN) =
			(uchar_t) info_ptr->ssap;
	} else {
		*((ushort_t *) ((caddr_t)udata + udata->dl_src_addr_offset +
				MAC_ADDR_LEN)) = trswab(info_ptr->ssap);
	}

	udata->dl_group_address = (pkt_type & DL_GROUP) ? 1 : 0;
	udmp->b_wptr += udata->dl_src_addr_length;

	udmp->b_cont = mp;

	/* send unitdata_ind upstream if we can */
	if (!canputnext(trsp->trs_rq)) {
#ifdef TR_DEBUG
		if (tr_debug & (TRRECV | TRERRS)) {
			cmn_err(CE_CONT, "TR:Upstream canputnext ");
			cmn_err(CE_CONT, "failed %x\n", trsp);
		}
#endif
		trdp->trd_statsd->trc_nocanput++;
		freemsg(udmp);
		udmp = NULL;
	} else {
		putnext(trsp->trs_rq, udmp);
	}
}

/*
 * tr_xid_ind_con - form a DL_XID_IND or DL_XID_CON message
 * to send to the user since it was requested that the user process these
 * messages
 */
static void
tr_xid_ind_con(trs_t *trsp, trd_t *trdp, struct llc_info *info_ptr,
		ushort_t pkt_type, mblk_t *mp)
{
	mblk_t *nmp;
	dl_xid_ind_t *xid;

	nmp = allocb(sizeof (dl_xid_ind_t) + 2*(MAC_ADDR_LEN + 1), BPRI_MED);
	if (nmp == NULL) {
		trdp->trd_statsd->trc_allocbfail++;
		freemsg(mp);
		mp = NULL;
		return;
	}

	xid = (dl_xid_ind_t *) nmp->b_rptr;
	xid->dl_flag = (info_ptr->control & LLC_P) ? DL_POLL_FINAL : 0;
	xid->dl_dest_addr_offset = sizeof (dl_xid_ind_t);
	xid->dl_dest_addr_length = MAC_ADDR_LEN + 1;
	bcopy((caddr_t) info_ptr->mac_ptr->dhost,
		((caddr_t) xid + xid->dl_dest_addr_offset), MAC_ADDR_LEN);
	*((caddr_t)xid + xid->dl_dest_addr_offset + MAC_ADDR_LEN) =
		(uchar_t) info_ptr->dsap;

	xid->dl_src_addr_offset =
		xid->dl_dest_addr_offset + xid->dl_dest_addr_length;
	xid->dl_src_addr_length = MAC_ADDR_LEN + 1;
	bcopy((caddr_t) info_ptr->mac_ptr->shost,
		((caddr_t) xid + xid->dl_src_addr_offset), MAC_ADDR_LEN);
	*((caddr_t)xid + xid->dl_src_addr_offset + MAC_ADDR_LEN) =
		(uchar_t) info_ptr->ssap & ~LLC_RESPONSE;

	/* Take Route bit out so we send up ACTUAL source addr */
	if (pkt_type & DL_ROUTE)
		*((uchar_t *)xid + xid->dl_src_addr_offset) &= ~TR_SR_ADDR;

	nmp->b_wptr = nmp->b_rptr + sizeof (dl_xid_ind_t) +
		2 * xid->dl_dest_addr_length;

	if (!(pkt_type & DL_RESPONSE)) {
		xid->dl_primitive = DL_XID_IND;
	} else {
		xid->dl_primitive = DL_XID_CON;
	}
	DB_TYPE(nmp) = M_PROTO;
	if (adjmsg(mp,
	    sizeof (struct tr_nori_mac_frm) + info_ptr->rsize +
	    sizeof (struct trllchdr))) {
		nmp->b_cont = mp;
	} else {
		freemsg(mp);
		mp = NULL;
	}

	/* send xid_ind upstream if we can */
	if (!canputnext(trsp->trs_rq)) {
#ifdef TR_DEBUG
		if (tr_debug & (TRRECV | TRERRS)) {
			cmn_err(CE_CONT, "TR:Upstream canputnext ");
			cmn_err(CE_CONT, "failed %x\n", trsp);
		}
#endif
		trdp->trd_statsd->trc_nocanput++;
		freemsg(nmp);
		nmp = NULL;
	} else {
		putnext(trsp->trs_rq, nmp);
	}
}

/*
 * tr_test_ind_con - form a DL_TEST_IND or DL_TEST_CON
 * message to send to the user since it was requested that the user process
 * these messages
 */
static void
tr_test_ind_con(trs_t *trsp, trd_t *trdp, struct llc_info *info_ptr,
		ushort_t pkt_type, mblk_t *mp)
{
	mblk_t *nmp;
	dl_test_ind_t *test;

	nmp = allocb(sizeof (dl_test_ind_t) + 2*(MAC_ADDR_LEN + 1), BPRI_MED);
	if (nmp == NULL) {
		trdp->trd_statsd->trc_allocbfail++;
		freemsg(mp);
		mp = NULL;
		return;
	}

	test = (dl_test_ind_t *) nmp->b_rptr;
	test->dl_flag = (info_ptr->control & LLC_P) ? DL_POLL_FINAL : 0;
	test->dl_dest_addr_offset = sizeof (dl_test_ind_t);
	test->dl_dest_addr_length = MAC_ADDR_LEN + 1;
	bcopy((caddr_t) info_ptr->mac_ptr->dhost,
		((caddr_t) test + test->dl_dest_addr_offset), MAC_ADDR_LEN);
	*((caddr_t)test + test->dl_dest_addr_offset + MAC_ADDR_LEN) =
		(uchar_t) info_ptr->dsap;

	test->dl_src_addr_offset =
		test->dl_dest_addr_offset + test->dl_dest_addr_length;
	test->dl_src_addr_length = MAC_ADDR_LEN + 1;
	bcopy((caddr_t) info_ptr->mac_ptr->shost,
		((caddr_t) test + test->dl_src_addr_offset), MAC_ADDR_LEN);
	*((caddr_t)test + test->dl_src_addr_offset + MAC_ADDR_LEN) =
		(uchar_t) info_ptr->ssap & ~LLC_RESPONSE;

	/* Take Route bit out so we send up ACTUAL source addr */
	if (pkt_type & DL_ROUTE)
		*((uchar_t *)test + test->dl_src_addr_offset) &= ~TR_SR_ADDR;

	nmp->b_wptr = nmp->b_rptr + sizeof (dl_test_ind_t) +
			2 * test->dl_dest_addr_length;

	if (!(pkt_type & DL_RESPONSE)) {
		test->dl_primitive = DL_TEST_IND;
	} else {
		test->dl_primitive = DL_TEST_CON;
	}
	DB_TYPE(nmp) = M_PROTO;
	if (adjmsg(mp,
	    sizeof (struct tr_nori_mac_frm) + info_ptr->rsize +
	    sizeof (struct trllchdr))) {
		nmp->b_cont = mp;
	} else {
		freemsg(mp);
		mp = NULL;
	}

	/* send test_ind upstream if we can */
	if (!canputnext(trsp->trs_rq)) {
#ifdef TR_DEBUG
		if (tr_debug & (TRRECV | TRERRS)) {
			cmn_err(CE_CONT, "TR:Upstream canputnext ");
			cmn_err(CE_CONT, "failed %x\n", trsp);
		}
#endif
		trdp->trd_statsd->trc_nocanput++;
		freemsg(nmp);
		nmp = NULL;
	} else {
		putnext(trsp->trs_rq, nmp);
	}
}

/*
 * tr_xid_reply - automatic reply to an XID command
 */
static void
tr_xid_reply(trs_t *trsp, trd_t *trdp, ushort_t pkt_type,
		struct llc_info *info_ptr, mblk_t *mp)
{
	mblk_t *nmp;
	struct tr_nori_mac_frm *outhdr;
	struct trllchdr *outllc;
	struct trhdr_xid *xid;
	cmdq_t	*xidrep;

#ifdef TR_DEBUG
	if (tr_debug & (TRTRACE | TRRECV))
		cmn_err(CE_CONT, "TR:auto xid reply\n");
#endif
	/* we only want to respond to commands; to avoid response loops */
	if (pkt_type & DL_RESPONSE)
		return;

	nmp = allocb(msgdsize(mp) + LLC_XID_INFO_SIZE, BPRI_MED);
	if (nmp == NULL) {
		trdp->trd_statsd->trc_allocbfail++;
		nmp = mp;
		mp = NULL;
	}
	/*
	 * now construct the XID reply frame
	 */
	outhdr = (struct tr_nori_mac_frm *) nmp->b_rptr;
	outhdr->ac = TR_AC;
	outhdr->fc = TR_LLC_FC;
	bcopy((caddr_t)info_ptr->mac_ptr->shost,
		(caddr_t)outhdr->dhost, MAC_ADDR_LEN);
	bcopy((caddr_t)trdp->trd_macaddr,
		(caddr_t)outhdr->shost, MAC_ADDR_LEN);

	/*
	 *  Get rid of source route bit in what was the source address
	 *  and add the bit to the outgoing source address
	 */
	if (pkt_type & DL_ROUTE) {
		outhdr->dhost[0] &= ~TR_SR_ADDR;
		outhdr->shost[0] |= TR_SR_ADDR;
	}
	nmp->b_wptr = nmp->b_rptr + sizeof (struct tr_nori_mac_frm);

	/*
	 * If the incoming packet had routing info, use it here.
	 * Reverse the direction bit.
	 */
	if (pkt_type & DL_ROUTE) {
		struct tr_ri *rtp = (struct tr_ri *) nmp->b_wptr;

		bcopy((caddr_t)info_ptr->ri_ptr, (caddr_t)rtp,
			info_ptr->rsize);
		rtp->dir = (rtp->dir) ? 0 : 1;
		nmp->b_wptr += info_ptr->rsize;
	}

	outllc = (struct trllchdr *) nmp->b_wptr;
	outllc->tr_dsap = info_ptr->ssap;
	outllc->tr_ssap = trsp->trs_802sap | LLC_RESPONSE;
	outllc->tr_ctl = info_ptr->control;
	nmp->b_wptr += sizeof (struct trllchdr);

	xid = (struct trhdr_xid *) nmp->b_wptr;
	xid->llcx_format = LLC_XID_FMTID;
	xid->llcx_class = LLC_XID_TYPE_1;
	xid->llcx_window = 0;	/* we don't have connections yet */
	nmp->b_wptr += sizeof (struct trhdr_xid);

	/* Free the XID received from the other side */
	if (mp)
		freemsg(mp);

	/*
	 * Note that we don't check the xmitcnt for this reply. This is
	 * because these are generally small quick xmits that are by
	 * definition supposed to be somewhat prompt.
	 */
	mutex_enter(&trdp->trd_intrlock);
	if (xidrep = (cmdq_t *)kmem_alloc(sizeof (cmdq_t), KM_NOSLEEP)) {
		xidrep->cmd = XMT_DIR_FRAME;
		xidrep->trdp = trdp;
		xidrep->trsp = NULL;
		xidrep->arg = info_ptr->dsap;
		xidrep->data = nmp;
		xidrep->callback = NULL;
		mutex_exit(&trdp->trd_intrlock);
		trqcmd(xidrep);
	} else {
		cmn_err(CE_WARN, "TR:No space for xid respond cmd!!");
		mutex_exit(&trdp->trd_intrlock);
	}
}

/*
 * tr_test_reply - automatic reply to a TEST message
 */
static void
tr_test_reply(trs_t *trsp, trd_t *trdp, ushort_t pkt_type,
		struct llc_info *info_ptr, mblk_t *mp)
{
	mblk_t *nmp;
	struct tr_nori_mac_frm *outhdr;
	struct trllchdr *outllc;
	uchar_t *wptr;
	cmdq_t *testrep;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:auto test reply\n");
#endif
	/* we only want to respond to commands to avoid response loops */
	if (pkt_type & DL_RESPONSE)
		return;

	nmp = copymsg(mp);	/* so info field is duplicated */
	if (nmp == NULL) {
		nmp = mp;
		mp = NULL;
	}
	/*
	 * now construct the TEST reply frame
	 */
	outhdr = (struct tr_nori_mac_frm *) nmp->b_rptr;
	outhdr->ac = TR_AC;
	outhdr->fc = TR_LLC_FC;
	bcopy((caddr_t) info_ptr->mac_ptr->shost,
		(caddr_t) outhdr->dhost, MAC_ADDR_LEN);
	bcopy((caddr_t) trdp->trd_macaddr,
		(caddr_t) outhdr->shost, MAC_ADDR_LEN);

	/*
	 *  Get rid of source route bit in what was the source address
	 *  and add the bit to the outgoing source address
	 */
	if (pkt_type & DL_ROUTE) {
		outhdr->dhost[0] &= ~TR_SR_ADDR;
		outhdr->shost[0] |= TR_SR_ADDR;
	}
	wptr = nmp->b_rptr + sizeof (struct tr_nori_mac_frm);

	/*
	 * If the incoming packet had routing info, use it here.
	 * Reverse the direction bit.
	 */
	if (pkt_type & DL_ROUTE) {
		struct tr_ri *rtp = (struct tr_ri *) wptr;

		bcopy((caddr_t)info_ptr->ri_ptr, (caddr_t)rtp,
			info_ptr->rsize);
		rtp->dir = (rtp->dir) ? 0 : 1;
		wptr += info_ptr->rsize;
	}

	outllc = (struct trllchdr *) wptr;
	outllc->tr_dsap = info_ptr->ssap;
	outllc->tr_ssap = trsp->trs_802sap | LLC_RESPONSE;
	outllc->tr_ctl = info_ptr->control;

	/* Free the TEST received from the other side */
	if (mp)
		freemsg(mp);

	/*
	 * Note that we don't check the xmitcnt for this reply. This is
	 * because these are generally small quick xmits that are by
	 * definition supposed to be somewhat prompt.
	 */
	mutex_enter(&trdp->trd_intrlock);
	if (testrep = (cmdq_t *)kmem_alloc(sizeof (cmdq_t), KM_NOSLEEP)) {
		testrep->cmd = XMT_DIR_FRAME;
		testrep->trdp = trdp;
		testrep->trsp = NULL;
		testrep->arg = info_ptr->dsap;
		testrep->data = nmp;
		testrep->callback = NULL;
		trqcmd(testrep);
		mutex_exit(&trdp->trd_intrlock);
	} else {
		cmn_err(CE_WARN, "TR:No space for test respond cmd!!");
		mutex_exit(&trdp->trd_intrlock);
	}
}

/*
 * tr_xid_req_res - the user wants to send an XID message
 * or response. Construct a proper message and put on the wire.
 */
tr_xid_req_res(queue_t *q, mblk_t *mp, int *err, int *uerr, int req_or_res)
{
	dl_xid_req_t *xid = (dl_xid_req_t *) mp->b_rptr;
	trs_t *trsp = (trs_t *) q->q_ptr;
	trd_t *trdp;
	mblk_t *nmp;
	struct tr_nori_mac_frm *hdr;
	struct trllchdr *llchdr;
	struct trdlpiaddr *daddr;
	struct srtab *sr = (struct srtab *)0;
	int srpresent = 0;
	cmdq_t *xidreq;

	if (trsp == NULL || trsp->trs_state == DL_UNATTACHED) {
		*err = DL_OUTSTATE;
		*uerr = 0;
		return (TRE_ERR);
	}

	if (!req_or_res && (trsp->trs_flags & TRS_AUTO_XID)) {
		*err = DL_XIDAUTO;
		*uerr = 0;
		return (TRE_ERR);
	}

	trdp = trsp->trs_dev;
	if (MBLKL(mp) < sizeof (dl_xid_req_t) ||
	    !MBLKIN(mp, xid->dl_dest_addr_offset, xid->dl_dest_addr_length)) {
		*err = DL_BADPRIM;
		*uerr = 0;
		return (TRE_ERR);
	}
	nmp = allocb(sizeof (struct tr_mac_frm) + sizeof (struct trllchdr),
			BPRI_MED);
	if (nmp == NULL) {
		trdp->trd_statsd->trc_allocbfail++;
		*err = DL_SYSERR;
		*uerr = ENOSR;
		return (TRE_ERR);
	}

	hdr = (struct tr_nori_mac_frm *) nmp->b_rptr;
	hdr->ac = TR_AC;
	hdr->fc = TR_LLC_FC;
	bcopy((caddr_t)xid + xid->dl_dest_addr_offset, (caddr_t)hdr->dhost,
		MAC_ADDR_LEN);
	bcopy((caddr_t)trdp->trd_macaddr, (caddr_t)hdr->shost, MAC_ADDR_LEN);
	nmp->b_wptr = nmp->b_rptr + sizeof (struct tr_nori_mac_frm);

	mutex_enter(&tr_srlock);
	sr = tr_sr_lookup(trdp, (uchar_t *)xid + xid->dl_dest_addr_offset);
	if (!sr) {
		/*
		 * must be broadcast, make it an all paths explorer
		 */
		struct tr_ri *rtp;
#ifdef TR_DEBUG
		if (tr_debug & TRSEND)
			cmn_err(CE_CONT, "No SR->APE\n");
#endif
		rtp = (struct tr_ri *) nmp->b_wptr;
		rtp->rt = RT_APE;
		rtp->len = 2;
		rtp->dir = 0;
		rtp->mtu = trdp->trd_bridgemtu;
		rtp->res = 0;
		srpresent = 1;
		nmp->b_wptr += 2;
	} else if (!(sr->sr_flags & SRF_LOCAL)) {
#ifdef TR_DEBUG
		if (tr_debug & TRSEND)
			cmn_err(CE_CONT, "Have sr info, adding it.\n");
#endif
		bcopy((caddr_t)&sr->sr_ri, (caddr_t)nmp->b_wptr,
			sr->sr_ri.len);
		srpresent = 1;
		nmp->b_wptr += sr->sr_ri.len;
	}
	mutex_exit(&tr_srlock);

	llchdr = (struct trllchdr *) nmp->b_wptr;
	daddr = (struct trdlpiaddr *) ((uchar_t *)xid +
					xid->dl_dest_addr_offset);
	llchdr->tr_dsap = daddr->dlpi_sap.sap;
	llchdr->tr_ssap = trsp->trs_802sap | (req_or_res ? LLC_RESPONSE : 0);
	llchdr->tr_ctl =
		LLC_XID | ((xid->dl_flag & DL_POLL_FINAL) ? LLC_P : 0);

	if (srpresent) {
		hdr->shost[0] |= TR_SR_ADDR;
	} else {
		hdr->shost[0] &= ~TR_SR_ADDR;
	}

	nmp->b_wptr += sizeof (struct trllchdr);
	nmp->b_cont = mp->b_cont;
	freeb(mp);

	/*
	 * Note that we don't check the xmitcnt for this reply. This is
	 * because these are generally small quick xmits that are by
	 * definition supposed to be somewhat prompt.
	 */
	mutex_enter(&trdp->trd_intrlock);
	if (xidreq = (cmdq_t *)kmem_alloc(sizeof (cmdq_t), KM_NOSLEEP)) {
		xidreq->cmd = XMT_DIR_FRAME;
		xidreq->trdp = trdp;
		xidreq->trsp = NULL;
		xidreq->arg = trsp->trs_802sap;
		xidreq->data = nmp;
		xidreq->callback = NULL;
		trqcmd(xidreq);
		mutex_exit(&trdp->trd_intrlock);
		return (TRE_OK);
	} else {
		cmn_err(CE_WARN, "TR:No space for xid respond cmd!!");
		*err = DL_SYSERR;
		*uerr = ENOMEM;
		mutex_exit(&trdp->trd_intrlock);
		return (TRE_ERR);
	}
}

/*
 * tr_test_req_res - the user wants to send an TEST
 * message or response. Construct a proper message and put it on the wire.
 */
tr_test_req_res(queue_t *q, mblk_t *mp, int *err, int *uerr, int req_or_res)
{
	dl_test_req_t *test = (dl_test_req_t *) mp->b_rptr;
	trs_t *trsp = (trs_t *) q->q_ptr;
	trd_t *trdp;
	mblk_t *nmp;
	struct tr_nori_mac_frm *hdr;
	struct trllchdr *llchdr;
	struct trdlpiaddr *daddr;
	struct srtab *sr = (struct srtab *)0;
	int srpresent = 0;
	cmdq_t *testreq;

	if (trsp == NULL || trsp->trs_state == DL_UNATTACHED) {
		*err = DL_OUTSTATE;
		*uerr = 0;
		return (TRE_ERR);
	}

	if (!req_or_res && (trsp->trs_flags & TRS_AUTO_TEST)) {
		*err = DL_TESTAUTO;
		*uerr = 0;
		return (TRE_ERR);
	}

	trdp = trsp->trs_dev;
	if (MBLKL(mp) < sizeof (dl_test_req_t) ||
	    !MBLKIN(mp, test->dl_dest_addr_offset,
		    test->dl_dest_addr_length)) {
		*err = DL_BADPRIM;
		*uerr = 0;
		return (TRE_ERR);
	}

	nmp = allocb(sizeof (struct tr_mac_frm) + sizeof (struct trllchdr),
			BPRI_MED);
	if (nmp == NULL) {
		trdp->trd_statsd->trc_allocbfail++;
		*err = DL_SYSERR;
		*uerr = ENOSR;
		return (TRE_ERR);
	}

	hdr = (struct tr_nori_mac_frm *) nmp->b_rptr;
	hdr->ac = TR_AC;
	hdr->fc = TR_LLC_FC;
	bcopy((caddr_t)test + test->dl_dest_addr_offset, (caddr_t)hdr->dhost,
		MAC_ADDR_LEN);
	bcopy((caddr_t)trdp->trd_macaddr, (caddr_t)hdr->shost, MAC_ADDR_LEN);
	nmp->b_wptr = nmp->b_rptr + sizeof (struct tr_nori_mac_frm);

	mutex_enter(&tr_srlock);
	sr = tr_sr_lookup(trdp, (uchar_t *)test + test->dl_dest_addr_offset);
	if (!sr) {
		/*
		 * must be broadcast, make it an all paths explorer
		 */
		struct tr_ri *rtp;
#ifdef TR_DEBUG
		if (tr_debug & TRSEND)
			cmn_err(CE_CONT, "No SR->APE\n");
#endif
		rtp = (struct tr_ri *) nmp->b_wptr;
		rtp->rt = RT_APE;
		rtp->len = 2;
		rtp->dir = 0;
		rtp->mtu = trdp->trd_bridgemtu;
		rtp->res = 0;
		srpresent = 1;
		nmp->b_wptr += 2;
	} else if (!(sr->sr_flags & SRF_LOCAL)) {
#ifdef TR_DEBUG
		if (tr_debug & TRSEND)
			cmn_err(CE_CONT, "Have sr info, adding it.\n");
#endif
		bcopy((caddr_t)&sr->sr_ri, (caddr_t)nmp->b_wptr,
			sr->sr_ri.len);
		srpresent = 1;
		nmp->b_wptr += sr->sr_ri.len;
	}
	mutex_exit(&tr_srlock);

	llchdr = (struct trllchdr *) nmp->b_wptr;
	daddr = (struct trdlpiaddr *) ((uchar_t *)test +
					test->dl_dest_addr_offset);
	llchdr->tr_dsap = daddr->dlpi_sap.sap;
	llchdr->tr_ssap = trsp->trs_802sap | (req_or_res ? LLC_RESPONSE : 0);
	llchdr->tr_ctl =
		LLC_TEST | ((test->dl_flag & DL_POLL_FINAL) ? LLC_P : 0);

	if (srpresent) {
		hdr->shost[0] |= TR_SR_ADDR;
	} else {
		hdr->shost[0] &= ~TR_SR_ADDR;
	}

	nmp->b_wptr += sizeof (struct trllchdr);
	nmp->b_cont = mp->b_cont;
	freeb(mp);

	/*
	 * Note that we don't check the xmitcnt for this reply. This is
	 * because these are generally small quick xmits that are by
	 * definition supposed to be somewhat prompt.
	 */
	mutex_enter(&trdp->trd_intrlock);
	if (testreq = (cmdq_t *)kmem_alloc(sizeof (cmdq_t), KM_NOSLEEP)) {
		testreq->cmd = XMT_DIR_FRAME;
		testreq->trdp = trdp;
		testreq->trsp = NULL;
		testreq->arg = trsp->trs_802sap;
		testreq->data = nmp;
		testreq->callback = NULL;
		trqcmd(testreq);
		mutex_exit(&trdp->trd_intrlock);
		return (TRE_OK);
	} else {
		cmn_err(CE_WARN, "TR:No space for test respond cmd!!");
		*err = DL_SYSERR;
		*uerr = ENOMEM;
		mutex_exit(&trdp->trd_intrlock);
		return (TRE_ERR);
	}
}

/*
 * tr_forme - check to see if the message is addressed to this system by
 * comparing with the board's address.
 */
static int
tr_forme(struct llc_info *hdr, trd_t *trdp)
{
	return (bcmp((caddr_t) hdr->mac_ptr->dhost,
			(caddr_t) trdp->trd_macaddr,
			MAC_ADDR_LEN) == 0);
}


/*
 * tr_fromme - check to see if the message was sent from this system by
 * comparing with the board's address.
 */
static int
tr_fromme(struct llc_info *hdr, trd_t *trdp)
{
	return (((hdr->mac_ptr->shost[0] & ~TR_SR_ADDR) ==
			trdp->trd_macaddr[0]) &&
			bcmp((caddr_t)&(hdr->mac_ptr->shost[1]),
				(caddr_t)&(trdp->trd_macaddr[1]),
				MAC_ADDR_LEN-1) == 0);
}

/*
 * tr_broadcast - check to see if a broadcast address is the destination of
 * this received packet
 */
static int
tr_broadcast(struct llc_info *hdr)
{
	return ((bcmp((caddr_t)hdr->mac_ptr->dhost,
			(caddr_t)tokenbroadcastaddr1.ether_addr_octet,
			MAC_ADDR_LEN) == 0) ||
		(bcmp((caddr_t)hdr->mac_ptr->dhost,
			(caddr_t)tokenbroadcastaddr2.ether_addr_octet,
			MAC_ADDR_LEN) == 0));
}


/*
 * tr_multicast used to determine if the address is a multicast address for
 * this user.
 */
static int
tr_multicast(struct llc_info *hdr, trs_t *trsp)
{
	return (trsp->trs_multimask &
		ntohl(*(ulong *)&hdr->mac_ptr->dhost[2]));
}

/*
 * tr_rsrv - upstream service routine. tr_intr offloads some protocol
 * interpretation onto this routine.
 */
static int
tr_rsrv(queue_t *q)
{
	register trs_t  *trsp;
	register trd_t  *trdp;
	struct llc_info pkt_info;
	mblk_t 		*mp;
	ushort_t	pkt_type;

	trsp = (trs_t *)q->q_ptr;
#ifdef TR_DEBUG
	if (tr_debug & (TRRECV | TRTRACE))
		cmn_err(CE_CONT, "TR_RSRV: stream is %x\n", trsp);
#endif
	trdp = trsp->trs_dev;
	while ((mp = getq(q)) != NULL) {
		pkt_type = parsellc(mp, &pkt_info);

		if ((trsp->trs_flags & TRS_FAST) && !(pkt_type & DL_GROUP)) {
#ifdef TR_DEBUG
			if (tr_debug & (TRFAST | TRRECV | TRTRACE))
				cmn_err(CE_CONT, "Up fast stream %x\n", trsp);
#endif
			adjmsg(mp, pkt_info.data_offset);
			putnext(trsp->trs_rq, mp);
		} else if (trsp->trs_flags & TRS_RAW) {
#ifdef TR_DEBUG
			if (tr_debug & (TRRECV | TRTRACE))
				cmn_err(CE_CONT, "Up raw stream %x\n", trsp);
#endif
			putnext(trsp->trs_rq, mp);
		} else {
			switch (pkt_info.control) {
			case LLC_UI:
				/*
				 * this is an Unnumbered Information packet
				 * so form a DL_UNITDATA_IND and send to user
				 */
				tr_form_udata(trsp, trdp, pkt_type,
						&pkt_info, mp);
				break;
			case LLC_XID:
			case LLC_XID | LLC_P:
				/*
				 * this is either an XID request or response.
				 * We either handle directly (if user hasn't
				 * requested to handle itself) or send to
				 * user.  We also must check if a response if
				 * user handled so that we can send correct
				 * message form
				 */
				if (trsp->trs_flags & TRS_AUTO_XID) {
					tr_xid_reply(trsp, trdp, pkt_type,
							&pkt_info, mp);
				} else {
					/*
					 * hand to the user for handling. if
					 * this is a "request", generate a
					 * DL_XID_IND.	If it is a "response"
					 * to one of our requests, generate a
					 * DL_XID_CON.
					 */
					tr_xid_ind_con(trsp, trdp, &pkt_info,
							pkt_type, mp);
				}
				break;
			case LLC_TEST:
			case LLC_TEST | LLC_P:
				/*
				 * this is either a TEST request or response.
				 * We either handle directly (if user hasn't
				 * requested to handle itself) or send to
				 * user.  We also must check if a response if
				 * user handled so that we can send correct
				 * message form
				 */
				if (trsp->trs_flags & TRS_AUTO_TEST) {
					tr_test_reply(trsp, trdp, pkt_type,
							&pkt_info, mp);
				} else {
					/*
					 * hand to the user for handling. if
					 * this is a "request", generate a
					 * DL_TEST_IND.	 If it is a
					 * "response" to one of our requests,
					 * generate a DL_TEST_CON.
					 */
					tr_test_ind_con(trsp, trdp, &pkt_info,
							pkt_type, mp);
				}
				break;
			default:
#ifdef TR_DEBUG
				if (tr_debug & (TRRECV | TRERRS)) {
					cmn_err(CE_CONT, "TR_RSRV:Clueless ");
					cmn_err(CE_CONT, "about msg type ");
					cmn_err(CE_CONT, "%x, control %x.\n",
						DB_TYPE(mp), pkt_info.control);
				}
#endif
				freemsg(mp);
				mp = NULL;
				break;
			}
		}
	}
	return (0);
}

/*
 * tr_ioctl handles all ioctl requests passed downstream. This routine is
 * passed a pointer to the message block with the ioctl request in it, and a
 * pointer to the queue so it can respond to the ioctl request with an ack.
 */
void
tr_ioctl(queue_t * q, mblk_t * mp)
{
	struct iocblk *iocp = (struct iocblk *) mp->b_rptr;
	register trs_t *trsp = (trs_t *) q->q_ptr;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR_IOCTL stream is %x!", trsp);
#endif

	switch (iocp->ioc_cmd) {
	case DLIOCRAW:			/* raw M_DATA mode */
		trsp->trs_flags |= TRS_RAW;
		miocack(q, mp, 0, 0);
		break;

	case DL_IOC_HDR_INFO:	/* M_DATA "fastpath" info request */
		tr_dl_ioc_hdr_info(q, mp);
		break;

	default:
#ifdef TR_DEBUG
		if (tr_debug & TRTRACE)
			cmn_err(CE_CONT, "Requested IOCTL %x", iocp->ioc_cmd);
#endif
		miocnak(q, mp, 0, EINVAL);
	}
}


/*
 * tr_inforeq - generate the response to an info request
 */
static int
tr_inforeq(queue_t *q, mblk_t *mp)
{
	trs_t *trsp;
	trd_t *trdp;
	trd_t *dvlp;
	mblk_t *nmp;
	dl_info_ack_t *dlp;
	int	bufsize;
	ulong   smallest = 0xffffffffU;

	trsp = (trs_t *) q->q_ptr;
	ASSERT(trsp);

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:INFOR, stream %x\n", trsp);
#endif
	trdp = (trd_t *) trsp->trs_dev;

	bufsize = sizeof (dl_info_ack_t) + 2*MAC_ADDR_LEN + 2;
	nmp = mexchange(q, mp, bufsize, M_PCPROTO, DL_INFO_ACK);

	if (nmp) {
		dlp = (dl_info_ack_t *) nmp->b_rptr;
		nmp->b_wptr = nmp->b_rptr + sizeof (dl_info_ack_t);
		bzero((caddr_t)dlp, DL_INFO_ACK_SIZE);
		dlp->dl_primitive = DL_INFO_ACK;
		dlp->dl_reserved = 0;
		dlp->dl_qos_length = 0;
		dlp->dl_qos_offset = 0;
		dlp->dl_qos_range_length = 0;
		dlp->dl_qos_range_offset = 0;
		dlp->dl_growth = 0;
		if (trdp) {
			dlp->dl_max_sdu = trdp->trd_maxupkt;
		} else {
			/*
			 * IP will ask for the max sdu before it even
			 * attaches. (and won't ask again). Therefore,
			 * we have to send back the worst case size amongst
			 * all the devices instead of more reasonable value
			 * like "We dont know yet, stupid, you haven't
			 *  attached".
			 */
			for (dvlp = trDevices; dvlp; dvlp = dvlp->trd_nextd) {
				if (dvlp->trd_maxupkt < smallest)
					smallest = dvlp->trd_maxupkt;
			}
			dlp->dl_max_sdu = smallest;
		}
		dlp->dl_min_sdu = 1;
		dlp->dl_mac_type = DL_TPR;
		dlp->dl_service_mode = DL_CLDLS;
		dlp->dl_current_state = trsp->trs_state;
		dlp->dl_provider_style = DL_STYLE2;

		/* now append physical address */
		if (trsp->trs_state == DL_IDLE) {
			dlp->dl_addr_length = MAC_ADDR_LEN;
			dlp->dl_addr_offset = DL_INFO_ACK_SIZE;
			bcopy((caddr_t) trdp->trd_macaddr,
				((caddr_t) dlp) + dlp->dl_addr_offset,
				MAC_ADDR_LEN);
			if (trsp->trs_type == DL_802) {
				dlp->dl_sap_length = -1;
				*(((caddr_t) dlp) + dlp->dl_addr_offset +
				    dlp->dl_addr_length) = trsp->trs_usersap;
				dlp->dl_addr_length += 1;
			} else {
				dlp->dl_sap_length = -2;
				*((ushort_t *)(((caddr_t)dlp) +
					dlp->dl_addr_offset +
					dlp->dl_addr_length)) =
						trsp->trs_usersap;
				dlp->dl_addr_length += 2;
			}
			nmp->b_wptr += dlp->dl_addr_length;
			dlp->dl_brdcst_addr_offset = DL_INFO_ACK_SIZE +
							dlp->dl_addr_length;
		} else {
			dlp->dl_addr_offset = NULL;
			/* Violate DLPI here to make IP work */
			dlp->dl_addr_length = 8;   /* ETHERADDRL + 2 */
			dlp->dl_sap_length = -2;
			dlp->dl_brdcst_addr_offset = DL_INFO_ACK_SIZE;
		}
		dlp->dl_brdcst_addr_length = MAC_ADDR_LEN;
		nmp->b_wptr += dlp->dl_brdcst_addr_length;
		bcopy((caddr_t) tokenbroadcastaddr1.ether_addr_octet,
			((caddr_t)dlp) + dlp->dl_brdcst_addr_offset,
			MAC_ADDR_LEN);
		dlp->dl_version = DL_VERSION_2;
		qreply(q, nmp);
	}
	return (TRE_OK);
}

/*
 * tr_unitdata
 * send a datagram.  Destination address/lsap is in M_PROTO
 * message (first mblock), data is in remainder of message.
 *
 * NOTE: We are reusing the DL_unitdata_req mblock; if tr header gets any
 * bigger, recheck to make sure it still fits!	We assume that we have a
 * 64-byte dblock for this, since a DL_unitdata_req is 20 bytes and the next
 * larger dblock size is 64.
 */
static int
tr_unitdata(queue_t *q, mblk_t *mp, int *err, int *uerr)
{
	trs_t	*trsp;
	trd_t	*trdp;
	dl_unitdata_req_t *dludp;
	mblk_t *nmp, *newmp, *freeme = NULL;
	struct trdlpiaddr *dlap;
	struct tr_mac_frm *headerp;
	int off, len, msglen;
	struct llc_snap_hdr *llcsnap;
	struct trllchdr *llcsap;
	int rtsize = 0;
	int saplen;
	struct srtab *sr = (struct srtab *)0;
	int srpresent = 0;

	trsp = (trs_t *) q->q_ptr;
	trdp = trsp->trs_dev;

#ifdef TR_DEBUG
	if (tr_debug & (TRSEND | TRTRACE))
		cmn_err(CE_CONT, "TR:Build outgoing\n");
#endif

	if (trsp->trs_state != DL_IDLE) {
		*err = DL_OUTSTATE;
		*uerr = 0;
		return (TRE_UDERR);
	}

	dludp = (dl_unitdata_req_t *) mp->b_rptr;

	off = dludp->dl_dest_addr_offset;
	len = dludp->dl_dest_addr_length;
	saplen = len - MAC_ADDR_LEN;

	/*
	 * Validate destination address format.
	 */
	if (!MBLKIN(mp, off, len) || ((saplen != 1) && (saplen != 2))) {
		*err = DL_BADADDR;
		*uerr = 0;
		return (TRE_UDERR);
	}

	/*
	 * Error if no M_DATA follows.
	 */
	nmp = mp->b_cont;
	if (nmp == NULL) {
		*err = DL_BADDATA;
		*uerr = 0;
		return (TRE_UDERR);
	}

	dlap = (struct trdlpiaddr *) (mp->b_rptr + off);

	mutex_enter(&tr_srlock);
	if (sr = tr_sr_lookup(trdp, dlap->dlpi_phys))
		rtsize = sr->sr_ri.len;
	else
		rtsize = 2;

	/*
	 * Catch attempts to send packets which are beyond capacity
	 */
	msglen = msgsize(nmp) + ACFCDASA_LEN + rtsize + LLC_SNAP_HDR_LEN;

	if (msglen > trdp->trd_dhbsize) {
#ifdef TR_DEBUG
		if (tr_debug & (TRSEND | TRERRS))
			cmn_err(CE_CONT, "TR:Msg too big: %d\n", msglen);
#endif
		trdp->trd_statsd->trc_oerrors++;
		*err = DL_BADDATA;
		*uerr = 0;
		mutex_exit(&tr_srlock);
		return (TRE_UDERR);
	}

	/*
	 * Create tokenring and llcsnap header (if needed) by either
	 * prepending it onto the next mblk if possible, or reusing
	 * the M_PROTO block if not.
	 */
	if ((DB_REF(nmp) == 1) &&
	    (MBLKHEAD(nmp) >= (ACFCDASA_LEN + rtsize + LLC_SNAP_HDR_LEN))) {
		freeme = mp;
		mp = nmp;
	} else {
		if ((MBLKSIZE(mp) - off - sizeof (struct trdlpiaddr)) >=
		    (ACFCDASA_LEN + LLC_SNAP_HDR_LEN + rtsize)) {
			mp->b_rptr = mp->b_wptr = DB_LIM(mp);
			DB_TYPE(mp) = M_DATA;
		} else {
			/*
			 * Allocate new mblk
			 */
			if ((newmp = allocb(ACFCDASA_LEN + LLC_SNAP_HDR_LEN +
					    rtsize, BPRI_LO)) == NULL) {
				trdp->trd_statsd->trc_allocbfail++;
				trdp->trd_statsd->trc_notbufs++;
				trdp->trd_statsd->trc_oerrors++;
				freeb(mp);
				*err = DL_SYSERR;
				*uerr = ENOSR;
				return (TRE_UDERR);
			}
			newmp->b_rptr = newmp->b_wptr = DB_LIM(newmp);
			DB_TYPE(newmp) = M_DATA;

			freeme = mp;
			linkb(newmp, nmp);
			mp = newmp;
		}
	}

	/*
	 * Write appropriate header in front of data
	 */
	if (saplen == 2) {
#ifdef TR_DEBUG
		if (tr_debug & TRSEND)
			cmn_err(CE_CONT, "BUILDING SNAP header,\n");
#endif
		mp->b_rptr -= LLC_SNAP_HDR_LEN;
		llcsnap = (struct llc_snap_hdr *)mp->b_rptr;
		llcsnap->d_lsap = LLC_SNAP_SAP;
		llcsnap->s_lsap = LLC_SNAP_SAP;
		llcsnap->control = LLC_UI;
		bzero((caddr_t)llcsnap->org,
				(u_int)sizeof (llcsnap->org));
/*
 * IP doesnt seem to be putting the proper type into the unitdata_req,
 * so we have to screw around and instead of taking type from the dsap
 * field of the request, we assume it wants to send to the receive
 * counterpart sap equal to the sap it is bound to on the sending side.
 * If IP gets fixed we should replace with the following:
 *	llcsnap->type = trswab(dlap->dsap.dl_sap_SNAP);
 */
		llcsnap->type = trswab(trsp->trs_usersap);
	} else {
#ifdef TR_DEBUG
		if (tr_debug & TRSEND)
			cmn_err(CE_CONT, "BUILDING plain header\n");
#endif
		mp->b_rptr -= LLC_8022_HDR_LEN;
		llcsap = (struct trllchdr *)mp->b_rptr;
		llcsap->tr_dsap = dlap->dlpi_sap.sap;
		llcsap->tr_ssap = trsp->trs_usersap;
		llcsap->tr_ctl = LLC_UI;
	}

	/*
	 * fill in source route header
	 */
	if (!sr) {
		/*
		 * must be broadcast, make it an all paths explorer
		 */
		struct tr_ri *rtp;

#ifdef TR_DEBUG
		if (tr_debug & TRSEND)
			cmn_err(CE_CONT, "No SR->APE\n");
#endif
		mp->b_rptr -= 2;
		rtp = (struct tr_ri *) mp->b_rptr;

		rtp->rt = RT_APE;
		rtp->len = 2;
		rtp->dir = 0;
		rtp->mtu = trdp->trd_bridgemtu;
		rtp->res = 0;
		srpresent = 1;
	} else if (!(sr->sr_flags & SRF_LOCAL)) {
#ifdef TR_DEBUG
		if (tr_debug & TRSEND)
			cmn_err(CE_CONT, "Have sr info, adding it.\n");
#endif
		mp->b_rptr -= sr->sr_ri.len;
		bcopy((caddr_t)&sr->sr_ri, (caddr_t)mp->b_rptr, sr->sr_ri.len);
		srpresent = 1;
	}
	mutex_exit(&tr_srlock);

	/*
	 * fill in token ring header
	 */
	mp->b_rptr -= ACFCDASA_LEN;
	headerp = (struct tr_mac_frm *) mp->b_rptr;
	headerp->ac = TR_AC;
	headerp->fc = TR_LLC_FC;
	bcopy((caddr_t)&(dlap->dlpi_phys), (caddr_t)&headerp->dhost,
		MAC_ADDR_LEN);
	bcopy((caddr_t)&trdp->trd_macaddr, (caddr_t)&headerp->shost,
		MAC_ADDR_LEN);

	if (srpresent) {
		headerp->shost[0] |= TR_SR_ADDR;
	} else {
		headerp->shost[0] &= ~TR_SR_ADDR;
	}

#ifdef TR_DEBUG
	if (tr_debug & TRSEND) {
		int tim;
		uchar_t *bytes;
		cmn_err(CE_CONT, "OUTGOING:\n");
		for (tim = 0, bytes = mp->b_rptr;
		    tim < (ACFCDASA_LEN + rtsize +
			((saplen == 2) ? LLC_SNAP_HDR_LEN :
			    LLC_8022_HDR_LEN));
		    tim++)
			cmn_err(CE_CONT, "%x ", *bytes++);
		cmn_err(CE_CONT, "\n");
	}
#endif

	/* Free no longer needed mbuf */
	if (freeme) {
		freeb(freeme);
	}
	return (trxmit(q, mp, err, uerr));
}

/*
 *	trxmit - Try to transmit a packet.
 */
int
trxmit(queue_t *q, mblk_t *mp, int *err, int *uerr)
{
	trs_t *trsp = (trs_t *) q->q_ptr;
	trd_t *trdp = trsp->trs_dev;
	cmdq_t *xmitcmd;

	/* Try to add this packet to the tail of the send queue */
	mutex_enter(&trdp->trd_intrlock);
	if ((trdp->trd_xmitcnt >= TRMAXQDPKTS) ||
	    (trsp->trs_flags & TRS_WTURN)) {
		traddholder(trdp, trsp);
		(void) putbq(q, mp);
		mutex_exit(&trdp->trd_intrlock);
		return (TRE_SLEEP);
	}

	if (xmitcmd = (cmdq_t *)kmem_alloc(sizeof (cmdq_t), KM_NOSLEEP)) {
		trdp->trd_xmitcnt++;
		if (trsp->trs_flags & TRS_RAW)
			xmitcmd->cmd = XMT_DIR_FRAME;
		else
			xmitcmd->cmd = XMT_UI_FRAME;
		xmitcmd->trdp = trdp;
		xmitcmd->trsp = trsp;
		xmitcmd->arg = trsp->trs_802sap;
		xmitcmd->data = mp;
		xmitcmd->callback = trwakeholder;
		xmitcmd->callback_arg = trdp;
		trqcmd(xmitcmd);
	} else {
		cmn_err(CE_WARN, "TR:No space for xmit cmd!!");
		*err = DL_SYSERR;
		*uerr = ENOMEM;
		mutex_exit(&trdp->trd_intrlock);
		return (TRE_ERR);
	}
	mutex_exit(&trdp->trd_intrlock);

	return (TRE_OK);

}

/*
 * trgetxmitok - send the command to tell the board we want to transmit.
 */
static void
trgetxmitok(trd_t *trdp, long cmd, long sap, mblk_t *pkt)
{
	struct xmt_cmd  *xc;

	/* Allow frames to be sent even if the hardware SAP binding failed */
	if ((cmd == XMT_UI_FRAME) &&
	    (!(trdp->trd_saptab[sap].sin_status & TRS_BOUND)))
		cmd = XMT_DIR_FRAME;

	trdp->trd_pkt = pkt;
	xc = (struct xmt_cmd *) trdp->trd_srb;
	xc->xc_cmd = (uchar_t)cmd;
	xc->xc_retcode = NULL_RC;
	xc->xc_stn_id = (cmd == XMT_DIR_FRAME) ? 0 :
		trdp->trd_saptab[sap].sin_station_id;
#ifdef TR_DEBUG
	if (tr_debug & TRSEND)
		cmn_err(CE_CONT, "TR:GETOK cmd=%x,sap=%x,id=%x\n",
			cmd, sap, xc->xc_stn_id);
#endif
	MMIO_SETBIT(ISRA + 1, ISRA_O_SRB_CMD);
}

/*
 * trsend - adapter is ready for us to transmit. Fill in the DHB and send
 * 	the puppy.
 */
static void
trsend(trd_t *trdp)
{
	struct xmt_cmd *xc;	/* xmit command */
	struct adp_xmt_req *ar;	/* adapter's request for data */
	struct adp_xmt_res *xr;	/* xmit response */
	struct 	llc_info	pkt_info;
	ushort_t pkt_type;
	mblk_t *mp;
	uchar_t *dhb;
	int len;
	int sum = 0;

	ASSERT(trdp->trd_pkt);
	pkt_type = parsellc(trdp->trd_pkt, &pkt_info);
#ifdef TR_DEBUG
	if (tr_debug & TRSEND)
		cmn_err(CE_CONT, "TRSEND: type 0x%x,", pkt_type);
#endif

	ar = (struct adp_xmt_req *)trdp->trd_arb;
	xr = (struct adp_xmt_res *)trdp->trd_asb;
	xc = (struct xmt_cmd *) trdp->trd_srb;
	xr->xr_cmd = xc->xc_cmd;
	xr->xr_retcode = OK_RC;
	xr->xr_cmdcor = ar->ar_cmdcor;
	xr->xr_stn_id = ar->ar_stn_id;
	if (pkt_type & DL_SNAP)
		xr->xr_rsap = LLC_SNAP_SAP;
	else
		xr->xr_rsap = pkt_info.dsap;
	mp = trdp->trd_pkt;
	xr->xr_hdr_len = 2 * MAC_ADDR_LEN + 2 + pkt_info.rsize;
	dhb = (uchar_t *) trdp->trd_sramaddr + trswab(ar->ar_dhb_addr);
	while (mp) {
		len = mp->b_wptr - mp->b_rptr;
		bcopy((caddr_t)mp->b_rptr, (caddr_t)dhb, len);
		dhb += len;
		sum += len;
		mp = mp->b_cont;
	}
	xr->xr_fr_len = trswab(sum);
	trdp->trd_statsd->trc_tbytes += sum;
	MMIO_SETBIT(ISRA + 1, ISRA_O_ASB_RES);

	if ((trdp->trd_flags & TR_PROM) && !(tr_broadcast(&pkt_info))) {
		/* Send up the promiscuous saps */
		trrcvmunch(trdp->trd_pkt, trdp, 0, pkt_type, &pkt_info);
	} else {
		freemsg(trdp->trd_pkt);
	}
	trdp->trd_pkt = (mblk_t *)0;
}

/*
 * tr_enable_multi -- enables multicast address on the stream
 */
static int
tr_enable_multi(queue_t *q, mblk_t *mp, int *err, int *uerr)
{
	trs_t *trsp;
	trd_t *trdp;
	struct ether_addr *maddr;
	dl_enabmulti_req_t *multi;
	int	len, off;
	long	funcaddr;

	if (MBLKL(mp) < (DL_ENABMULTI_REQ_SIZE + ETHERADDRL)) {
		*err = DL_BADPRIM;
		*uerr = 0;
		return (TRE_ERR);
	}

	trsp = (trs_t *) q->q_ptr;
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "TR:Enab multi, stream %x\n", trsp);
#endif

	if (trsp->trs_state == DL_UNATTACHED) {
		*err = DL_OUTSTATE;
		*uerr = 0;
		return (TRE_ERR);
	}

	multi = (dl_enabmulti_req_t *) mp->b_rptr;

	len = multi->dl_addr_length;
	off = multi->dl_addr_offset;
	maddr = (struct ether_addr *) (mp->b_rptr + off);

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE) {
		int tim;
		uchar_t *bytes;
		cmn_err(CE_CONT, "TR:Attempted multicast\n");
		for (tim = 0; tim < MAC_ADDR_LEN; tim++)
			cmn_err(CE_CONT, "%x ", maddr->ether_addr_octet[tim]);
	}
#endif

	if ((len != ETHERADDRL) || !MBLKIN(mp, off, len) ||
	    !(ismulticast(maddr))) {
		*err = DL_BADADDR;
		*uerr = 0;
		return (TRE_ERR);
	}

	funcaddr = ((maddr->ether_addr_octet[2] << 24) |
		    (maddr->ether_addr_octet[3] << 16) |
		    (maddr->ether_addr_octet[4] << 8) |
		    maddr->ether_addr_octet[5]);

	trdp = trsp->trs_dev;

	mutex_enter(&trdp->trd_intrlock);
	trsp->trs_multimask |= funcaddr;
	trchgmultinoq(trdp);
	mutex_exit(&trdp->trd_intrlock);
	dlokack(q, mp, DL_ENABMULTI_REQ);

	return (TRE_OK);
}

/*
 * trchgmultinoq - queues a command to change the multicast address
 *		   for the device (no streams associated with the command)
 */
static void
trchgmultinoq(trd_t *trdp)
{
	cmdq_t	*qmask;

	if (qmask = (cmdq_t *)kmem_alloc(sizeof (cmdq_t), KM_NOSLEEP)) {
		qmask->cmd = DIR_FUNC_ADDR;
		qmask->trdp = trdp;
		qmask->trsp = NULL;
		qmask->callback = NULL;
		trqcmd(qmask);
	} else {
		cmn_err(CE_WARN, "TR:No space for multicast cmd!!");
	}
}

/*
 * trsetfunc - set the board functional address (if necessary)
 */
static void
trsetfunc(trd_t *trdp)
{
	struct dir_grp_addr *fa;
	trs_t	*lps;
	long	newmask = 0;

	/*
	 * Possible rethink required here, dont want to grab stream
	 * reader writer lock since we are holding intr mutex. Perhaps
	 * change access to reader/writer to always be within mutex??
	 */
	for (lps = trStreams; lps; lps = lps->trs_nexts)
		if (lps->trs_dev == trdp)
			newmask |= lps->trs_multimask;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE) {
		cmn_err(CE_CONT, "Current board mask %x, ",
			trdp->trd_multimask);
		cmn_err(CE_CONT, "Newmask %x\n", newmask);
	}
#endif

	/* Dont bother to run command if the board mask doesn't change */
	if (newmask != trdp->trd_multimask) {
		trdp->trd_multimask = newmask;
		fa = (struct dir_grp_addr *)trdp->trd_srb;
		fa->ga_cmd = DIR_FUNC_ADDR;
		fa->ga_retcode = NULL_RC;
		fa->ga_addr[0] = (uchar_t) (newmask>>24);
		fa->ga_addr[1] = (uchar_t) ((newmask&0xff0000)>>16);
		fa->ga_addr[2] = (uchar_t) ((newmask&0xff00)>>8);
		fa->ga_addr[3] = (uchar_t) (newmask&0xff);
#ifdef TR_DEBUG
		if (tr_debug & TRTRACE) {
			cmn_err(CE_CONT, "Requesting mask %x:%x:%x:%x\n",
				fa->ga_addr[0], fa->ga_addr[1], fa->ga_addr[2],
				fa->ga_addr[3]);
		}
#endif
		MMIO_SETBIT(ISRA + 1, ISRA_O_SRB_CMD);
	} else {
		trnxtcmd(trdp);
	}
}

/*
 * trsetfuncdone - board has completed setting functional address.
 */
static void
trsetfuncdone(trd_t *trdp)
{
	struct dir_grp_addr *fr;
	fr = (struct dir_grp_addr *)trdp->trd_srb;
	if (fr->ga_retcode)
		cmn_err(CE_WARN, "TR:Multiset problem? [%x]", fr->ga_retcode);
	trnxtcmd(trdp);
}

/*
 * tr_disable_multi disable the multicast address of this stream
 */
static int
tr_disable_multi(queue_t *q, mblk_t *mp, int *err, int *uerr)
{
	trs_t *trsp;
	dl_disabmulti_req_t *dldmp;
	struct ether_addr *addrp;
	int off;
	int len;
	ulong funcaddr;
	trd_t *trdp;

	if (MBLKL(mp) < (DL_DISABMULTI_REQ_SIZE + ETHERADDRL)) {
		*err = DL_BADPRIM;
		*uerr = 0;
		return (TRE_ERR);
	}

	trsp = (trs_t *) q->q_ptr;
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "disable multi, stream %x\n", trsp);
#endif

	if (trsp->trs_state == DL_UNATTACHED) {
		*err = DL_OUTSTATE;
		*uerr = 0;
		return (TRE_ERR);
	}

	dldmp = (dl_disabmulti_req_t *) mp->b_rptr;

	len = dldmp->dl_addr_length;
	off = dldmp->dl_addr_offset;
	addrp = (struct ether_addr *) (mp->b_rptr + off);

	if ((len != ETHERADDRL) || !MBLKIN(mp, off, len) ||
	    !(ismulticast(addrp))) {
		*err = DL_BADADDR;
		*uerr = 0;
		return (TRE_ERR);
	}

	funcaddr = (addrp->ether_addr_octet[2] << 24) |
		    (addrp->ether_addr_octet[3] << 16) |
		    (addrp->ether_addr_octet[4] << 8) |
		    addrp->ether_addr_octet[5];

	if ((trsp->trs_multimask & funcaddr) != funcaddr) {
		*err = DL_NOTENAB;
		*uerr = 0;
		return (TRE_ERR);
	}

	trdp = trsp->trs_dev;

	mutex_enter(&trdp->trd_intrlock);
	trsp->trs_multimask &= ~funcaddr;
	trchgmultinoq(trdp);
	mutex_exit(&trdp->trd_intrlock);
	dlokack(q, mp, DL_DISABMULTI_REQ);

	return (TRE_OK);
}

/*
 * trqcmdonly - queue up a command to be run by the board.
 */
static void
trqcmdonly(cmdq_t *cmd)
{
	trd_t	*trdp;

	trdp = cmd->trdp;

	cmd->next = NULL;
	if (trdp->trd_cmdsq) {
		trdp->trd_cmdstail->next = cmd;
		trdp->trd_cmdstail = cmd;
	} else {
		trdp->trd_cmdsq = trdp->trd_cmdstail = cmd;
	}
	trruncmd(trdp);
}

/*
 * trqcmd - Reinitialize the board when needed, o/w queue the command.
 */
void
trqcmd(cmdq_t *cmd)
{
	trd_t	*trdp;

	trdp = cmd->trdp;

	if (trdp->not_first) {
		trdp->not_first = 0;
		while (!(trdp->trd_flags & TR_OPEN)) {
			tr_reinit(trdp);
			(void) tr_reopen(trdp);
		}
		tr_reopensap(trdp);
		drv_usecwait(50 * 1000); /* wait for 50ms */
		if (trdp->trd_onhold) {
#ifdef TR_DEBUG
			if (tr_debug & (TRTRACE | TRSEND))
			    cmn_err(CE_CONT, "TR:ENABH %x\n", trdp->trd_onhold);
#endif
			trwakeholder(trdp);
		}
	} else trqcmdonly(cmd);
}

/*
 * trdqcmd - dequeue the command just run by the board.
 */
static void
trdqcmd(trd_t *trdp)
{
	if (trdp->trd_cmdsq == trdp->trd_cmdstail) {
		trdp->trd_cmdsq = trdp->trd_cmdstail = (cmdq_t *)NULL;
	}
	else
		trdp->trd_cmdsq = trdp->trd_cmdsq->next;
}

/*
 * trruncmd - If board not busy, run the first command in this board's queue.
 */
static void
trruncmd(trd_t *trdp)
{
	cmdq_t	*cmd;

	if (!(trdp->trd_flags & TR_XBUSY) && (cmd = trdp->trd_cmdsq)) {
		trdp->trd_flags |= TR_XBUSY;
#ifdef TR_DEBUG
		if (tr_debug & TRTRACE)
			cmn_err(CE_CONT, "TR:RUNCMD-> for stream %x\n",
				cmd->trsp);
#endif
		switch (cmd->cmd) {
		case (NO_CMD):
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "None.");
#endif
			trnxtcmd(trdp);
			break;
		case (DIR_OPEN_ADP):
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "Open Board.");
#endif
			tropenboard(trdp);
			break;
		case (DIR_CLOSE_ADP):
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "Close Board.");
#endif
			trcloseboard(trdp);
			break;
		case (DIR_FUNC_ADDR):
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "Set Func Addr.");
#endif
			trsetfunc(trdp);
			break;
		case (DLC_OPEN_SAP):
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "Open SAP.");
#endif
			tropensap(trdp, cmd->arg);
			break;
		case (DLC_CLOSE_SAP):
#ifdef TR_DEBUG
			if (tr_debug & TRTRACE)
				cmn_err(CE_CONT, "Close SAP.");
#endif
			trclosesap(trdp, cmd->arg);
			break;
		case (XMT_UI_FRAME):
		case (XMT_DIR_FRAME):
#ifdef TR_DEBUG
			if (tr_debug & (TRTRACE | TRSEND))
				cmn_err(CE_CONT, "XMIT. ");
#endif
			trgetxmitok(trdp, cmd->cmd, cmd->arg, cmd->data);
			break;
		default:
#ifdef TR_DEBUG
			if (tr_debug & (TRTRACE | TRERRS))
				cmn_err(CE_CONT, "Unrecognized command.");
#endif
			trnxtcmd(trdp);
		}
	}
}

/*
 * trnxtcmd - Clean up after first command on board's queue.
 */
static void
trnxtcmd(trd_t *trdp)
{
	cmdq_t	*cmd;

	trdp->trd_flags &= ~TR_XBUSY;
	if ((cmd = trdp->trd_cmdsq) != NULL) {
		trdqcmd(trdp);
		if (cmd->callback)
			cmd->callback(cmd->callback_arg);
		kmem_free(cmd, sizeof (*cmd));
	} else {
		cmn_err(CE_CONT, "TR:?? How did I get here ??");
	}
	trruncmd(trdp);
}

/*
 * trremcmds - Remove commands from the queue associated with the given stream.
 *	If it can't be removed, we must at least render its callbacks harmless.
 */
static void
trremcmds(trd_t *trdp, trs_t *trsp)
{
	cmdq_t	*lpc;

	for (lpc = trdp->trd_cmdsq; lpc; lpc = lpc->next) {
		if (lpc->trsp == trsp) {
			switch (lpc->cmd) {
			/*
			 * Go ahead with opens, since other streams may
			 * be waiting on the same open.
			 */
			case (DLC_OPEN_SAP):
			case (DIR_OPEN_ADP):
				continue;
			/*
			 * Also have to go ahead with functional addr
			 * calls because the same command is used for
			 * enable and disable and we don't know which is
			 * which here.
			 */
			case (DIR_FUNC_ADDR):
				lpc->trsp = NULL; /* disassociate the stream */
				lpc->callback = NULL; /* cancel wakeup call */
				break;
			case (XMT_UI_FRAME):
				lpc->cmd = NO_CMD;
			default:
				lpc->cmd = NO_CMD;
				break;
			}
		}
	}
}

/*
 * trdescchg - Examine the status for description of why we received ring
 *	change notification.
 */
static void
trdescchg(trd_t *trdp)
{
	struct adp_ring_chng *rc;
	ushort_t s;

#ifdef TR_DEBUG
	if (tr_debug & TRERRS)
		cmn_err(CE_CONT, "Ring Change: ");
#endif
	mutex_enter(&trdp->trd_intrlock);
	rc = (struct adp_ring_chng *) trdp->trd_arb;
	s = trswab(rc->rc_status);

	/* Fatal errors that result in adapter closings */
	if (s & 0x800) {
		drv_getparm(LBOLT, &trdp->wdog_lbolt);
		cmn_err(CE_WARN, "TR: Lobe wire fault, Adapter closing");
		trdp->not_first = 1;
		trdp->trd_pending = NULL;
		trdp->trd_flags &=
			~(TR_INIT | TR_OPEN | TR_PROM | TR_WOPEN | TR_XBUSY | TR_OPENFAIL);
		trwakewaiters(trdp);

	}
	if (s & 0x400) {
		cmn_err(CE_WARN, "TR: Auto removal error, Adapter closing");
	}
	if (s & 0x100) {
		cmn_err(CE_WARN, "TR: Remove frame received, Adapter closing");
	}

#ifdef TR_DEBUG
	if (tr_debug & TRERRS) {
		if (s & 0x8000)
			cmn_err(CE_CONT, "Signal loss,");
		if (s & 0x4000)
			cmn_err(CE_CONT, "Hard error,");
		if (s & 0x2000)
			cmn_err(CE_CONT, "Soft error,");
		if (s & 0x1000)
			cmn_err(CE_CONT, "Beacon transmit,");
		if (s & 0x80)
			cmn_err(CE_CONT, "Counter overflow,");
		if (s & 0x40)
			cmn_err(CE_CONT, "Lonely,");
		if (s & 0x20)
			cmn_err(CE_CONT, "Ring recovery");
	}
#endif
	mutex_exit(&trdp->trd_intrlock);

}

/*
 * trupdatert - Process route information in incoming packet.
 */
static void
trupdatert(trd_t *trdp, ushort_t pkt_type, struct llc_info *info_ptr)
{
	struct srtab *sr;

	/* Take Route bit out so we store ACTUAL source addr */
	if (pkt_type & DL_ROUTE)
		info_ptr->mac_ptr->shost[0] &= ~TR_SR_ADDR;

#ifdef TR_DEBUG
	if (tr_debug & TRSRTE) {
		int tim;
		uchar_t *bytes = info_ptr->mac_ptr->shost;
		cmn_err(CE_CONT, "srU ");
		for (tim = 0; tim < MAC_ADDR_LEN; tim++)
			cmn_err(CE_CONT, "%x ", *bytes++);
		cmn_err(CE_CONT, "\n");
	}
#endif

	mutex_enter(&tr_srlock);
	if (sr = tr_sr_create_entry(trdp, info_ptr->mac_ptr->shost)) {
		sr->sr_timer = 0;
		if (!(pkt_type & DL_ROUTE) || info_ptr->rsize <= 2) {
			sr->sr_flags = (SRF_LOCAL | SRF_RESOLVED);
			sr->sr_ri.len = 0;
		} else if (sr->sr_flags == SRF_PENDING ||
				info_ptr->rsize < sr->sr_ri.len) {
			bcopy((caddr_t)info_ptr->ri_ptr, (caddr_t)&sr->sr_ri,
				info_ptr->rsize);
			sr->sr_ri.len = info_ptr->rsize;
			sr->sr_ri.dir = (info_ptr->direction ? 0 : 1);
			sr->sr_ri.res = 0;
			sr->sr_ri.rt = RT_SRF;
			sr->sr_flags = SRF_RESOLVED;
			/*
			 * XXX need to set mtu
			 */
		}
	}
	mutex_exit(&tr_srlock);

	/* Turn route bit back on so we'll recognize routing info in RSRV */
	if (pkt_type & DL_ROUTE)
		info_ptr->mac_ptr->shost[0] |= TR_SR_ADDR;

}

/*
 * trcompbufs -- Compute buffer sizes for this board to use when opened.
 */
static void
trcompbufs(trd_t *trdp)
{
	switch (trdp->trd_sramsize) {
	case (0x2000):
		trdp->trd_dhbsize = TR_SMALL_DHB;
		trdp->trd_numdhb = 1;
		break;
	case (0x4000):
	default:
		trdp->trd_dhbsize = TR_MED_DHB;
		trdp->trd_numdhb = 1;
		break;
	case (0x8000):
		if (trdp->trd_adprate == 4) {
			trdp->trd_dhbsize = TR_MED_DHB;
			trdp->trd_numdhb = 2;
		} else {
			trdp->trd_dhbsize = TR_LRG_DHB;
			trdp->trd_numdhb = 1;
		}
		break;
	case (0x10000):
		if (trdp->trd_adprate == 4) {
			trdp->trd_dhbsize = TR_MED_DHB;
			trdp->trd_numdhb = 2;
		} else {
			trdp->trd_dhbsize = TR_XLRG_DHB;
			trdp->trd_numdhb = 1;
		}
		break;
	}
	trdp->trd_numrcvs = (trdp->trd_dhbsize * 2)/TR_RCVBUF_LEN;
	trdp->trd_numrcvs *= trdp->trd_numdhb;
	trdp->trd_maxsaps = ddi_getprop(DDI_DEV_T_NONE, trdp->trd_devnode, 0,
					"maxsaps", TR_MAXSAP);
	trdp->trd_maxupkt = trdp->trd_dhbsize - TR_XMT_OVHD -
				LLC_EHDR_SIZE - MAX_ROUTE_FLD;
}

/*
 * M_DATA "fastpath" info request. I assume this is only for TCPIP,
 * this is an important assumption as a snap header will be created.
 * Following the M_IOCTL mblk should come a DL_UNITDATA_REQ mblk.
 * We ack with an M_IOCACK pointing to the original DL_UNITDATA_REQ mblk
 * followed by an mblk containing the raw tokenring + llcsnap hdr corresponding
 * to the destination address. Subsequently, we may receive M_DATA
 * msgs which start with this header and may send up
 * up M_DATA msgs with b_rptr pointing to the network-layer data
 * (IP packet header). This is all selectable on a per-Stream basis.
 */
static void
tr_dl_ioc_hdr_info(queue_t *q, mblk_t *mp)
{
	mblk_t *nmp;
	trs_t  *trsp;
	trd_t  *trdp;
	dl_unitdata_req_t *dludp;
	struct tr_mac_frm *headerp;
	struct trdlpiaddr *dlap;
	struct llc_snap_hdr *llcsnap;
	struct srtab *sr;
	int size = 0;

	trsp = (trs_t *) q->q_ptr;
#ifdef TR_DEBUG
	if (tr_debug & (TRFAST | TRTRACE))
		cmn_err(CE_CONT, "Fastpath for stream %x\n", trsp);
#endif
	/*
	 * Sanity check the request.
	 */
	if ((mp->b_cont == NULL) ||
	    (MBLKL(mp->b_cont) < sizeof (dl_unitdata_req_t) +
		sizeof (struct trdlpiaddr)) ||
	    (*((u_long *)mp->b_cont->b_rptr) != DL_UNITDATA_REQ) ||
	    (trsp->trs_state != DL_IDLE) ||
	    ((trdp = trsp->trs_dev) == NULL)) {
		miocnak(q, mp, 0, EINVAL);
		return;
	}

	/*
	 * Sanity check the DL_UNITDATA_REQ destination address
	 * offset and length values.
	 */
	dludp = (dl_unitdata_req_t *) mp->b_cont->b_rptr;
	if (!MBLKIN(mp->b_cont, dludp->dl_dest_addr_offset,
		    dludp->dl_dest_addr_length) ||
	    dludp->dl_dest_addr_length != sizeof (struct trdlpiaddr)) {
		miocnak(q, mp, 0, EINVAL);
		return;
	}

	dlap = (struct trdlpiaddr *) (mp->b_cont->b_rptr +
			dludp->dl_dest_addr_offset);

	mutex_enter(&tr_srlock);

	if (sr = tr_sr_lookup(trdp, dlap->dlpi_phys))
		size = sr->sr_ri.len;

	/*
	 * Allocate a new mblk to hold the tokenring and llcsnap header.
	 */
	size += sizeof (struct tr_mac_frm) + LLC_SNAP_HDR_LEN;
	if ((nmp = allocb(size, BPRI_MED)) == NULL) {
		miocnak(q, mp, 0, ENOMEM);
		mutex_exit(&tr_srlock);
		return;
	}

	nmp->b_wptr = nmp->b_datap->db_lim;
	nmp->b_rptr = nmp->b_wptr;

	/*
	 * fill in llc snap header
	 */
	nmp->b_rptr -= LLC_SNAP_HDR_LEN;
	llcsnap = (struct llc_snap_hdr *)nmp->b_rptr;
	llcsnap->d_lsap = LLC_SNAP_SAP;
	llcsnap->s_lsap = LLC_SNAP_SAP;
	llcsnap->control = LLC_UI;
	bzero((caddr_t)llcsnap->org, (u_int)sizeof (llcsnap->org));
	llcsnap->type = trswab(trsp->trs_usersap);

	/*
	 * fill in source route header
	 */
	if (sr && sr->sr_ri.len) {
		nmp->b_rptr -= sr->sr_ri.len;
		bcopy((caddr_t)&sr->sr_ri, (caddr_t)nmp->b_rptr, sr->sr_ri.len);
	}

	mutex_exit(&tr_srlock);

	/*
	 * Fill in the token ring header.
	 */
	nmp->b_rptr -= ACFCDASA_LEN;
	headerp = (struct tr_mac_frm *) nmp->b_rptr;
	headerp->ac = TR_AC;
	headerp->fc = TR_LLC_FC;
	bcopy((caddr_t)&(dlap->dlpi_phys), (caddr_t)&headerp->dhost,
		MAC_ADDR_LEN);
	bcopy((caddr_t)&trdp->trd_macaddr, (caddr_t)&headerp->shost,
		MAC_ADDR_LEN);

	if (sr && sr->sr_ri.len)
		headerp->shost[0] |= TR_SR_ADDR;
	else
		headerp->shost[0] &= ~TR_SR_ADDR;

	/*
	 * Link new mblk in after the "request" mblks.
	 */
	linkb(mp, nmp);

	trsp->trs_flags |= TRS_FAST;
	miocack(q, mp, msgsize(mp->b_cont), 0);

}

static void
tr_kstatinit(trd_t *trdp)
{
	kstat_t *ksp;
	struct trkstat *tksp;

	if (!(ksp = kstat_create("tr", ddi_get_instance(trdp->trd_devnode),
			NULL, "net", KSTAT_TYPE_NAMED,
			sizeof (struct trkstat) / sizeof (kstat_named_t),
			0))) {
		cmn_err(CE_CONT, "TR: kstat_create_failed");
		trdp->trd_kstatp = NULL;
		return;
	}

	trdp->trd_kstatp = ksp;
	tksp = (struct trkstat *) ksp->ks_data;

	kstat_named_init(&tksp->trs_ipackets, "ipackets", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_ierrors, "ierrors", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_opackets, "opackets", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_oerrors, "oerrors", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_notbufs, "notbufs", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_norbufs, "norbufs", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_nocanput, "nocanput", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_allocbfail, "noallocb", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_sralloc, "sralloc", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_srfree, "srfree", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_intrs, "intrs", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_rbytes, "rbytes", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_tbytes, "tbytes", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_brdcstrcv, "brdcstrcv", KSTAT_DATA_ULONG);
	kstat_named_init(&tksp->trs_multircv, "multircv", KSTAT_DATA_ULONG);

	ksp->ks_update = tr_kstat_update;
	ksp->ks_private = (void *) trdp;
	kstat_install(ksp);
}

static int
tr_kstat_update(kstat_t *ksp, int rw)
{
	trd_t *trdp;
	struct trkstat *tksp;

	if (rw == KSTAT_WRITE)
		return (1);

	trdp = (trd_t *) ksp->ks_private;
	tksp = (struct trkstat *) ksp->ks_data;

	tksp->trs_ipackets.value.ul = trdp->trd_statsd->trc_ipackets;
	tksp->trs_ierrors.value.ul = trdp->trd_statsd->trc_ierrors;
	tksp->trs_opackets.value.ul = trdp->trd_statsd->trc_opackets;
	tksp->trs_oerrors.value.ul = trdp->trd_statsd->trc_oerrors;
	tksp->trs_notbufs.value.ul = trdp->trd_statsd->trc_notbufs;
	tksp->trs_norbufs.value.ul = trdp->trd_statsd->trc_norbufs;
	tksp->trs_nocanput.value.ul = trdp->trd_statsd->trc_nocanput;
	tksp->trs_allocbfail.value.ul = trdp->trd_statsd->trc_allocbfail;
	tksp->trs_sralloc.value.ul = trdp->trd_statsd->trc_sralloc;
	tksp->trs_srfree.value.ul = trdp->trd_statsd->trc_srfree;
	tksp->trs_intrs.value.ul = trdp->trd_statsd->trc_intrs;
	tksp->trs_rbytes.value.ul = trdp->trd_statsd->trc_rbytes;
	tksp->trs_tbytes.value.ul = trdp->trd_statsd->trc_tbytes;
	tksp->trs_brdcstrcv.value.ul = trdp->trd_statsd->trc_brdcstrcv;
	tksp->trs_multircv.value.ul = trdp->trd_statsd->trc_multircv;

	return (0);
}

static void
tr_getpos(int slot, unchar *pos)
{
	int i;

	/* enable the slot for POS read */
	outb(TRPOS_ADAP_ENAB, (slot - 1) + TRPOS_SETUP);
	for (i = 0; i < 6; i++)
		pos[i] = inb(TRPOS_REG_BASE + i);
	outb(TRPOS_ADAP_ENAB, TRPOS_DISABLE);	/* turn off */
}

static int
tr_ismatch(dev_info_t *dip, int irq, caddr_t mmio_phys, int base_pio, int mca)
{
	struct intrspec {
	    int	ipl;
	    int	irq;
	} *intrspec;
	struct regspec {
	    int	bus;
	    caddr_t base;
	    int	len;
	} *regspec;
	int len, items;
	int i, found = 0;

	/* only bother if ioaddress is valid */
	if (base_pio == (int) PRIMARY_PIO || base_pio == (int) SECONDARY_PIO) {
#ifdef notdef
		if (ddi_getprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS,
				"ioaddr", 0) == 0)
			return (0);
#endif

		/* check for IRQ being valid for this dev info */
		if (ddi_getlongprop(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS,
		    "intr", (caddr_t)&intrspec, &len) == DDI_PROP_SUCCESS) {
			items = len / sizeof (struct intrspec);
			for (i = 0; i < items; i++)
				if (intrspec[i].irq == irq)
					break;

			kmem_free((caddr_t)intrspec, len);

			if (i < items) {
				/* IRQ valid so continue; */
				if (ddi_getlongprop(DDI_DEV_T_ANY, dip,
					DDI_PROP_DONTPASS, "reg",
					(caddr_t)&regspec, &len) ==
				    DDI_PROP_SUCCESS) {
					items = len / sizeof (struct regspec);
					if (mca) {
						if (regspec[0].base <=
						    mmio_phys &&
						    (regspec[0].base +
							regspec[0].len) >=
							(mmio_phys+8192))
							found++;
						else if (items == 3) {
							if (regspec[2].base <=
							    mmio_phys &&
							    (regspec[2].base +
							    regspec[2].len) >=
							    (mmio_phys+8192))
							found++;
						}
					} else {
						/*
						 * If ISA and the first memory
						 * address matches, go for it
						 */
						if (regspec[0].base ==
						    mmio_phys) {
							kmem_free(
							    (caddr_t)regspec,
							    len);
							found++;
						}
					}
				}
				kmem_free((caddr_t)regspec, len);
			}
		}
	}
	return (found);
}

static
void
tr_reinit(trd_t *trdp)
{
	uchar_t	*base_pio;	/* The base io address of PIO area */
	paddr_t	shrd_addr_conf;	/* Shared Ram address from config file */
	struct trmmio	*mmio_virt; /* Virtual base address of mmio */

	outb((int)(trdp->trd_pioaddr + PIO_INT_ENB), 1);
	/*
	 * Retrieve I/O address property. Won't verify it again, but assume
	 * its okay, since our probe succeeded.
	 */
	base_pio = trdp->trd_pioaddr;
	mmio_virt = trdp->trd_mmio;
	shrd_addr_conf = trdp->trd_sramphys;

	trdp->trd_pending = NULL;

	trreset(base_pio);
	mutex_exit(&trdp->trd_intrlock);
	drv_usecwait(50 * 1000); /* wait for 50ms */
	mutex_enter(&trdp->trd_intrlock);

	/*
	 * Tell board where to put shared RAM by ORing the appropriate
	 * bits into the even byte of the RRR.
	 */
	mmio_virt->mmio[RRR] = SRAM_SET(shrd_addr_conf);

	/*
	 * we have to zero out the last 512 bytes of the SRAM if
	 * we are working with 64K bytes of SRAM.  This is only needed
	 * when that part is used for some other function, but we do it
	 * all the time to be safe
	 */
	if (trdp->trd_sramsize == 0x10000)
		bzero((char *)trdp->trd_sramaddr + 0xfe00, 512);

	while (trdp->trd_cmdsq)
		trdqcmd(trdp);

	trdp->trd_flags &=
	    ~(TR_INIT | TR_OPEN | TR_PROM | TR_WOPEN | TR_XBUSY | TR_OPENFAIL);
	trdp->trd_flags |= TR_READY;

	/*
	 * Enable the interrupts to initialize the board
	 */
	MMIO_SETBIT(ISRP, ISRP_E_IENB);

	mutex_exit(&trdp->trd_intrlock);
	while (!(trdp->trd_flags & TR_INIT)) {
		drv_usecwait(100);
	}
	mutex_enter(&trdp->trd_intrlock);
}

static
int
tr_reopen(trd_t *trdp)
{
	trs_t			*trsp = trdp->bind_trsp;
	cmdq_t			*qopen;

#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "Opened?;");
#endif
	if (!(qopen = (cmdq_t *)kmem_alloc(sizeof (cmdq_t), KM_NOSLEEP))) {
		cmn_err(CE_WARN, "TR:no space for open cmd!!");
		return (TRE_ERR);
	}
	qopen->cmd = DIR_OPEN_ADP;
	qopen->trdp = trdp;
	qopen->trsp = trsp;
	qopen->callback = NULL;
	qopen->callback_arg = NULL;

	trqcmdonly(qopen);

	mutex_exit(&trdp->trd_intrlock);
	while (!(trdp->trd_flags & (TR_OPEN | TR_OPENFAIL))) {
		drv_usecwait(100);
	}
	mutex_enter(&trdp->trd_intrlock);

	if (trdp->trd_flags & TR_OPENFAIL) {
		cmn_err(CE_WARN, "TR:Adapter Open FAILED!");
		trdp->trd_flags &= ~TR_OPENFAIL;
	}

#ifdef TR_DEBUG
	if (trdp->trd_flags & TR_OPEN) {
		if (tr_debug & TRTRACE)
			cmn_err(CE_CONT, "Opend!;");
	}
#endif
	return (TRE_OK);
}

static
void
tr_reopensap(trd_t *trdp)
{
	trs_t			*trsp = trdp->bind_trsp;
	int			sap;
	cmdq_t			*qsopen;

	sap = LLC_SNAP_SAP;
#ifdef TR_DEBUG
	if (tr_debug & TRTRACE)
		cmn_err(CE_CONT, "Reopening sap: %x\n", sap);
#endif
	if (qsopen = (cmdq_t *)kmem_alloc(sizeof (cmdq_t),
		KM_NOSLEEP)) {
		qsopen->cmd = DLC_OPEN_SAP;
		qsopen->trdp = trdp;
		qsopen->trsp = trsp;
		qsopen->arg = sap;
		qsopen->callback = NULL;
		trqcmdonly(qsopen);
	} else {
		cmn_err(CE_NOTE, "TR:No memory for Bind command!");
	}

	trwakewaiters(trdp);
}

static
void
tr_wdog(caddr_t arg)
{
	trd_t	*trdp = (trd_t *) arg;	    /* soft structure pointer */
	cmdq_t			*qcmd;

	unsigned long	val;

	mutex_enter(&trdp->trd_intrlock);
	drv_getparm(LBOLT, &val);

	if ((val-trdp->wdog_lbolt > TIMEOUT) && (trdp->wdog_lbolt != 0)) {
		if (qcmd = (cmdq_t *)kmem_alloc(sizeof (cmdq_t),
			KM_NOSLEEP)) {
			qcmd->cmd = NO_CMD;
			qcmd->trdp = trdp;
			trqcmd(qcmd);
		} else {
			cmn_err(CE_NOTE, "TR:No memory for NO_CMD command!");
		}
		trwakewaiters(trdp);
		trdp->wdog_lbolt = 0;
	}
	if (!trdp->detaching)
		trdp->wdog_id = timeout(tr_wdog, (caddr_t)trdp, WDOGTICKS);
	mutex_exit(&trdp->trd_intrlock);
}
