/* Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/* Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/* All Rights Reserved						*/

/* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF	*/
/* UNIX System Laboratories, Inc.			*/
/* The copyright notice above does not evidence any	*/
/* actual or intended publication of such source code.	*/

/*
 * Module: WD8003 Project: System V ViaNet
 *
 *
 * Copyright (c) 1987, 1988, 1989, 1990, 1991, 1992 by Western Digital
 * Corporation and Sun Microsystems, Inc. All rights reserved.  Contains
 * confidential information and trade secrets proprietary to Western Digital
 * Corporation 2445 McCabe Way Irvine, California  92714
 *
 * Sun Microsystems, Inc.
 */

#pragma ident	"@(#)smc.c	1.44	96/05/27 SMI"

/*
 * Streams driver for WD/SMC 8003 Ethernet/Starlan controller Implements an
 * extended version of the AT&T Data Link Provider Interface IEEE 802.2 Class
 * 1 type 1 protocol is partially implemented and supports receipt and
 * response to XID and TEST. Ethernet encapsulation is also supported by
 * binding to a SAP greater than 0xFF.
 *
 * The following items should be done later:
 * - Add support for DL_IOC_HDR_INFO ioctls to match the Lance Ethernet driver.
 * - Only minimal MT support has been added.  More sophisticated MT support
 *   should be added later when time permits.
 * - The device (wdparam) and queue (wddev) arrays should probably be replaced
 *   with linked lists.
 */
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/immu.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/errno.h"
#include "sys/dlpi.h"
#include "sys/socket.h"
#include "net/if.h"
#include "sys/kmem.h"
#include "sys/kstat.h"

#include "sys/ethernet.h"
#include "sys/strsun.h"
#include "sys/cmn_err.h"
#include "sys/systm.h"
#include "sys/cred.h"
#include "sys/byteorder.h"
#include "sys/ddi.h"
#include "sys/sunddi.h"

#include "sys/smc.h"
#include "sys/smcboard.h"
#include "sys/smchdw.h"

#define	nextsize(len) ((len+64)*2)

extern int	wd_minors;	/* number of minor devices supported */
extern int	wd_boardcnt;	/* number of boards */
extern int	wd_multisize;	/* number of multicast addrs/board */
extern kmutex_t	wd_lock;	/* lock for this module */
extern struct wddev	wddevs[];	/* queue specific parameters */
extern struct wdparam	wdparams[];	/* board specific parameters */
extern struct wdstat	wdstats[];	/* board statistics */
extern struct wdmaddr	wdmultiaddrs[];	/* storage for multicast addrs */

/* ---- LOCAL PROTOTYPES ---- */
static void	enable_slot(int);
u_char		wdhash(u_char *);
int		wdsend(struct wddev *, mblk_t *);
int		wdlocal(mblk_t *, struct wdparam *), wdbroadcast(mblk_t *);
struct wdmaddr	*wdmulticast(u_char *, struct wdmaddr *, struct wdmaddr *, int);
int		wdbind(queue_t *, mblk_t *), wdunbind(queue_t *, mblk_t *);
int		wdunitdata(queue_t *, mblk_t *), wdinforeq(queue_t *, mblk_t *);
extern void	wdbcopy(caddr_t, caddr_t, int, int);
void		wdinit_board(struct wddev *), wduninit_board(struct wdparam *);
int		wdopen(queue_t *, dev_t *, int, int, struct cred *);
int		wdclose(queue_t *);
int		wdwput(queue_t *, mblk_t *);
int		wdwsrv(queue_t *), wdrsrv(queue_t *);
void		wdrecv(mblk_t *, struct wdparam *);
void		wdsched(struct wdparam *);
void		wdsched_wrapper(struct wdparam *);
static int	recv_ether(struct wddev *, queue_t *, mblk_t *);
static void	wd_dl_ioc_hdr_info(queue_t *, mblk_t *);
void		wdloop(mblk_t *, struct wddev *);
static		llccmds(queue_t *, mblk_t *);
u_int		wdintr(caddr_t);
static void	smc_watchdog(struct wdparam *);
static int	can_write(struct wddev *);
int		wdlooped(mblk_t *, struct wdparam *);
int		wdnak(queue_t *, mblk_t *, int);
void		wdioctl(queue_t *, mblk_t *);
int		wdattachreq(queue_t *, mblk_t *);
int		wddetachreq(queue_t *, mblk_t *);
void		wddodetach(struct wddev *);
int		wdpromisconreq(queue_t *, mblk_t *);
int		wdpromiscoffreq(queue_t *, mblk_t *);
int		wdenabmultireq(queue_t *, mblk_t *);
int		wddisabmultireq(queue_t *, mblk_t *);
int		wdphysaddrreq(queue_t *, mblk_t *);
int		wdsetphysaddrreq(queue_t *, mblk_t *);
void		wd_addmulti(struct wdparam *, struct wdmaddr *);
void		wd_delmulti(struct wdparam *, struct wdmaddr *);
void		wdflush_multi(struct wddev *);
static u_char	*eaddrstr(u_char *);
void		get_node_addr(int, u_char *);
static void	printcfg(char *, u_char *);
void		smc_print(char *);
int		wdstat_kstat_update(kstat_t *, int);
void		wdstatinit(struct wdparam *);
int		smc_get_irq(int, int);
caddr_t		smc_get_base(int, int);
static void	get_setup(int, u_char *);
/* ---- END OF PROTOTYPES ---- */

/*
 * This is for lint.
 */
#define	BCOPY(from, to, len) \
	bcopy((caddr_t)(from), (caddr_t)(to), (size_t)(len))
#define	BCMP(s1, s2, len) bcmp((char *)(s1), (char *)(s2), (size_t)(len))

char wdcopyright[] =
"Copyright 1987, 1988, 1989, 1990, 1991, 1992 Sun Microsystems, Inc.\
\n\tand Western Digital Corporation/SMC\nAll Rights Reserved.";

/*
 * initialize the necessary data structures for the driver to get called. the
 * hi/low water marks will have to be tuned
 */

/* FIX - temporary module id number */
#define	ENETM_ID 2101

static struct module_info minfo = {
	ENETM_ID, "smc", 0, WDMAXPKT, WDHIWAT, WDLOWAT
};

/* read queue initial values */
static struct qinit rinit = {
	NULL, wdrsrv, wdopen, wdclose, NULL, &minfo, NULL,
};

/* write queue initial values */
static struct qinit winit = {
	wdwput, wdwsrv, NULL, NULL, NULL, &minfo, NULL,
};

struct streamtab wdinfo = {&rinit, &winit, NULL, NULL};

#if defined(WDDEBUG)
extern int	wd_debug;
#endif

unsigned char  *eaddrstr();

unsigned char wdbroadaddr[LLC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#define	SRC_ALIGN	0
#define	DEST_ALIGN	1

typedef struct _intr_ {
	int	spl;
	int	intr;
} intr_t;

typedef struct _busreg_ {
	int	bustype;
	int	base_addr;
	int	mem_size;
} busreg_t;

/*
 * check_slot() - check for a SMC lan adapter in slot number "slot_num".
 * Returns board ID if found, 0 otherwise.
 */
static void
enable_slot(int slot_num)
{
	int select = SETUP + slot_num - 1;
	outb(ADAP_ENAB, select);
}

unsigned
check_slot(int slot_num)
{
	unsigned	board_id;
	unsigned	found = 0;

	enable_slot(slot_num);

	board_id = ((inb(POS_1) & 0xff) << 8) | (inb(POS_0) & 0xff);
	switch (board_id) {
		case WD_ETA_ID:
		case WD_STA_ID:
		case WD_WA_ID:
		case WD_EA1_ID:
		case WD_WA1_ID:
		case IBM_13E_ID:
		case IBM_13W_ID:
			found = board_id;
			break;
	}

	outb(ADAP_ENAB, DISSETUP);

	return (found);
}

/*
 * wdinit is called from wd_attach to do any general initialization needed.
 * it is not called with any parameters and is called only once.
 * What used to be wdinit under V.x has been split up into two routines.
 * wdinit, which performs generic driver initializations and wdsetup which
 * performs board-specific initialization.
 */
void
wdinit(dev_info_t *devi)
{
	register int    i;
	int		slot_found[8];
	int		board_count = 0;
	u_char		pos_regs[8];	/* Array of POS reg data */
	unsigned	option;		/* temp for POS information */
	struct wdparam	*wdp = wdparams;

	if (micro_channel(devi)) {
		for (i = 1; i <= MC_SLOTS; i++) {
			if (check_slot(i) != 0) {
				slot_found[board_count++] = i;
			}
		}
	}

	for (i = 0; i < wd_boardcnt; i++, wdp++) {
		if (micro_channel(devi) && (i < board_count)) {
			get_setup(slot_found[i], pos_regs);

			/* Interpret the POS register information */

			switch (check_slot(slot_found[i])) {
				/*
				 * Bad news. check_slot had returned a valid
				 * board id for this slot and now it doesn't.
				 * Warn the user and ignore this entry.
				 */
				case 0:
					cmn_err(CE_WARN,
		"smc: Board in slot %d returns inconsistent information.\n", i);
					break;

				case WD_ETA_ID:
				case WD_STA_ID:
				case WD_WA_ID:
					/*
					 * Base I/O address
					 */
					option = (unsigned)pos_regs[2] & 0xFE;
					wdp->wd_ioaddr = option << 4;

					/*
					 * IRQ
					 */
					option = (unsigned)pos_regs[5] & 0x03;
					switch (option) {
						case 0: wdp->wd_int =  3; break;
						case 1: wdp->wd_int =  4; break;
						case 2: wdp->wd_int = 10; break;
						case 3: wdp->wd_int = 15; break;
					}

					/*
					 * Base memory address
					 */
					option = (unsigned)(pos_regs[3] & 0xFC);
					wdp->wd_base = (caddr_t)(option << 12);
					wdp->wd_memsize = PS2_RAMSZ;
					break;

				case WD_EA1_ID:
				case WD_WA1_ID:
				case IBM_13E_ID:
				case IBM_13W_ID:
					wdp->wd_ioaddr = IOBASE_594 |
						((pos_regs[2] & 0xF0) << 8);
					option = ((int)pos_regs[5] & 0xC) >> 2;
					switch (option) {
						case 0: wdp->wd_int =  3; break;
						case 1: wdp->wd_int =  4; break;
						case 2: wdp->wd_int = 10; break;
						case 3: wdp->wd_int = 15; break;
					}
					option = pos_regs[3] & 0xff;
					if (option & 0x80)
						option = 0xFC0000 +
							((option & 0xF) << 13);
					else
						option = 0x0C0000 +
							((option & 0xF) << 13);
					wdp->wd_base = (caddr_t)option;
					wdp->wd_memsize = PS2_RAMSZ8 <<
							(((int)pos_regs[3] &
							0x30) >> 4);
					break;

				default:
					cmn_err(CE_WARN,
					    "smc: Unknown board in slot %d\n",
						i);
					break;
			}
		}		/* end if micro_channel() */
		wdp->wd_index = (short)i;	/* board index */
		wdp->wd_init = 0;	/* board initialized */
		wdp->wd_str_open = 0;	/* number of streams open */
		wdp->wd_nextq = -1;	/* next queue to process */
		wdp->wd_ncount = 0;	/* count of bufcalls */
		wdp->wd_proms = 0;	/* number of promiscuous strs */
		wdp->wd_firstd = 0;	/* first minor device */
		wdp->wd_noboard = 0;	/* board is not present */
		wdp->wd_devi = 0;	/* no dev_info */

		/*
		 * setup for multicast addressing
		 */
		wdp->wd_multiaddrs = &wdmultiaddrs[i * wd_multisize];
		wdp->wd_multicnt = 0;
		wdp->wd_multip = wdp->wd_multiaddrs;
	}

	/* zero out the queue-specific structure */
	bzero(wddevs, sizeof (struct wddev) * wd_boardcnt);

	/* initialize the mutex for this module */
	mutex_init(&wd_lock, "SMC mutex", MUTEX_DRIVER, (void *)0);
}


/*
 * wdsetup is called from wd_attach to do any per board initialization
 * needed. By the time wdsetup is called we have already determined that the
 * wd board is present by calculating the board checksum.  Returns SUCCESS or
 * FAILURE
 */
wdsetup(dev_info_t *devi)
{
	struct wdparam		*wdp;
	ddi_iblock_cookie_t	c;
	char			*board_name;
	int			regbuf[3];
	int			buflen = sizeof (regbuf);
	int			intr_index, reg_index;

	wdp = (struct wdparam *)ddi_get_driver_private(devi);
	get_node_addr(wdp->wd_ioaddr, wdp->wd_macaddr);
	localetheraddr((struct ether_addr *)wdp->wd_macaddr,
		(struct ether_addr *)0);
	wdstatinit(wdp);

	buflen = sizeof (int);

	/*
	 * Find out board type. This includes: board name memory size
	 * interface chip available alternate irq
	 */
	wdp->wd_boardtype = GetBoardID(wdp->wd_ioaddr, micro_channel(devi));

	if (!micro_channel(devi)) {
		/*
		 * If we have an interface chip and not being forced to use the
		 * config file then start probing for the correct values
		 */
		if (wdp->wd_boardtype & INTERFACE_CHIP) {
			/* get irq value from interface chip */
			wdp->wd_int = smc_get_irq(wdp->wd_ioaddr,
				wdp->wd_boardtype);

			/* get memory base from interface chip */
			wdp->wd_base = smc_get_base(wdp->wd_ioaddr,
				wdp->wd_boardtype);

			/*
			 * find our memory size. For boards that can't
			 * determine the ram size default to 8K which is the
			 * minimum.
			 */
			if ((wdp->wd_boardtype & RAM_SIZE_MASK) ==
			    RAM_SIZE_UNKNOWN)
				wdp->wd_memsize = 0x200;
			else
				switch (wdp->wd_boardtype & RAM_SIZE_MASK) {
				case RAM_SIZE_8K:
					wdp->wd_memsize = 0x2000; break;
				case RAM_SIZE_16K:
					wdp->wd_memsize = 0x4000; break;
				case RAM_SIZE_32K:
					wdp->wd_memsize = 0x8000; break;
				case RAM_SIZE_64K:
					wdp->wd_memsize = 0x10000; break;
				default:
					cmn_err(CE_NOTE,
						"smc: invalid memory size\n");
					return (DDI_FAILURE);
				}
		} else {
			/*
			 * On boards that don't have an interface chip we get
			 * the information that we need from the conf file.
			 */
			buflen = sizeof (regbuf);
			if (WD_PROP(devi, SmcReg, regbuf, &buflen) !=
			    DDI_PROP_SUCCESS)
				return (DDI_FAILURE);
			wdp->wd_base = (caddr_t)regbuf[1];
			wdp->wd_memsize = regbuf[2];
			buflen = sizeof (int) * 2;
			if (WD_PROP(devi, SmcIntr, regbuf, &buflen) !=
			    DDI_PROP_SUCCESS)
				return (DDI_FAILURE);
			wdp->wd_int = regbuf[1];
		}
	}

	/*
	 * Figure out which set of tuples need to be used. In the case of a
	 * dumb board the buflen will be 8 and we'll match on the first and
	 * only try.
	 */
	if (ddi_getproplen(DDI_DEV_T_NONE, devi, 0, SmcIntr, &buflen) ==
	    DDI_PROP_SUCCESS) {
		intr_t *ireg;
		int alloced = buflen;

		ireg = (intr_t *)kmem_alloc(buflen, KM_NOSLEEP);
		if (WD_PROP(devi, SmcIntr, ireg, &buflen) == DDI_PROP_SUCCESS) {
			int i;
			intr_index = 0;
			for (i = 0; i < (buflen / (sizeof (int) * 2)); i++)
				if (ireg[i].intr == wdp->wd_int) {
					intr_index = i;
					break;
				}
		}
		else
			intr_index = 0;		/* hope for the best */
		kmem_free(ireg, alloced);
	}
	else
		intr_index = 0;

	/*
	 * set up the interrupt handler NOTE: wdintr was changed to accept
	 * the wdp arg
	 */
	ddi_get_iblock_cookie(devi, intr_index, &c);

	mutex_init(&wdp->wd_intrlock, "wd intr lock", MUTEX_DRIVER, (void *)c);

	ddi_add_intr(devi, intr_index, &c, 0, wdintr, (caddr_t)wdp);

	/*
	 * Do the same here except for the reg spec.
	 */
	if (ddi_getproplen(DDI_DEV_T_NONE, devi, 0, SmcReg, &buflen) ==
	    DDI_PROP_SUCCESS) {
		busreg_t *busreg;
		int alloced = buflen;
		busreg = (busreg_t *)kmem_alloc(buflen, KM_NOSLEEP);
		if (WD_PROP(devi, SmcReg, busreg, &buflen) ==
		    DDI_PROP_SUCCESS) {
			int i;
			reg_index = 0;
			for (i = 0; i < (buflen / (sizeof (int) * 3)); i++)
				if ((busreg[i].base_addr ==
				    (int)wdp->wd_base) &&
				    (busreg[i].mem_size == wdp->wd_memsize)) {
					reg_index = i;
					break;
				}
		}
		else
			reg_index = 0;
		kmem_free(busreg, alloced);
	}
	else
		reg_index = 0;

	/*
	 * tell kernel about 8003 RAM & get a valid virtual addr for it
	 */
	if (ddi_map_regs(devi, reg_index, &(wdp->wd_rambase), 0,
	    wdp->wd_memsize) != 0)
		return (DDI_FAILURE);

	/*
	 * Show the configuration information if asked. This isn't really
	 * neccessary when the config file is used but, what the heck.
	 */
	buflen = sizeof (int);
	if (WD_PROP(devi, "config", regbuf, &buflen) == DDI_PROP_SUCCESS) {
		cmn_err(CE_CONT,
			"\nsmc%d: io 0x%x base mem. 0x%x, size 0x%x, IRQ %d\n",
			ddi_get_instance(devi), wdp->wd_ioaddr, wdp->wd_base,
			wdp->wd_memsize, wdp->wd_int);
		cmn_err(CE_CONT,
			"      %s %s\n",
			(wdp->wd_boardtype & INTERFACE_584_CHIP) ?
			"584 Interface chip" :
			((wdp->wd_boardtype & INTERFACE_CHIP) ?
			"Interface chip" : ""),
			(wdp->wd_boardtype & ALTERNATE_IRQ_BIT) ?
			"Alternate IRQ" : "");
	}

	switch (wdp->wd_boardtype & STATIC_ID_MASK) {
	case WD8003E:
		board_name = "8003E   ";
		break;
	case WD8003S:
		board_name = "8003S   ";
		break;
	case WD8003WT:
		board_name = "8003WT  ";
		break;
	case WD8003W:
		board_name = "8003W   ";
		break;
	case WD8003EB:
		if (wdp->wd_boardtype & INTERFACE_584_CHIP)
			board_name = "8003EP  ";
		else
			board_name = "8003EB  ";
		break;
	case WD8003ETA:
		board_name = "8003ET/A";
		break;
	case WD8003STA:
		board_name = "8003ST/A";
		break;
	case WD8003EA:
		board_name = "8003E/A ";
		break;
	case WD8003SHA:
		board_name = "8003SH/A";
		break;
	case WD8003WA:
		board_name = "8003W/A";
		break;
	case WD8013EBT:
		board_name = "8013EBT ";
		break;
	case WD8013EB:
		board_name = "8013EB  ";
		break;
	case WD8013EW:
		board_name = "8013EW  ";
		break;
	case WD8013W:
		board_name = "8013W  ";
		break;
	case WD8013EWC:
		board_name = "8013EWC ";
		break;
	case WD8013WC:
		board_name = "8013WC  ";
		break;
	case WD8013EPC:
		board_name = "8013EPC ";
		break;
	case WD8003EPC:
		board_name = "8003EPC ";
		break;
	case WD8003WC:
		board_name = "8003WC  ";
		break;
	case WD8216T:
		board_name = "8216T   ";
		wdp->wd_bflags |= WDF_UNSAFE8BIT;
		break;
	case WD8216:
		board_name = "8216    ";
		wdp->wd_bflags |= WDF_UNSAFE8BIT;
		break;
	case WD8216C:
		board_name = "8216C   ";
		wdp->wd_bflags |= WDF_UNSAFE8BIT;
		break;
	case PCM10BT:
		board_name = "PCM10BT ";
		break;
	default:
		board_name = "8003    ";
		break;
	}

	printcfg(board_name, eaddrstr((u_char *)wdp->wd_macaddr));

	bzero(wddevs, sizeof (struct wddev) * wd_boardcnt);
	return (DDI_SUCCESS);
}

/*
 * wdopen is called by the stream head to open a minor device. CLONEOPEN is
 * supported and opens the lowest numbered minor device which is not
 * currently opened.  Specific device open is allowed to open an already
 * opened stream.
 */

int
wdopen(queue_t *q, dev_t *dev, int flag, int sflag, struct cred *credp)
{
	struct wddev	*wd;
	mblk_t		*mp;
	struct stroptions	*opts;
	int		rval;
	major_t		devmajor;
	minor_t		devminor;

#ifdef lint
	flag = flag + 1;
#endif
	mutex_enter(&wd_lock);

	/* determine type of open, CLONE or otherwise */
	if (sflag == CLONEOPEN) {
		for (devminor = 0; devminor < wd_minors; devminor++)
			if (wddevs[devminor].wd_qptr == NULL)
				break;
	} else
		devminor = geteminor(*dev);
	devmajor = getemajor(*dev);

	/* couldn't find an available stream */
	if (devminor >= wd_minors) {
#if defined(WDDEBUG)
		if (wd_debug & WDSYSCALL)
			cmn_err(CE_CONT, "wdopen: no devices\n");
#endif
		rval = ECHRNG;
		qprocson(q);
		goto out1;
	}
	if (q->q_ptr) {		/* already open, just return it */
		*dev = makedevice(devmajor, devminor);
		rval = 0;
		qprocson(q);
		goto out1;
	}
	wd = &wddevs[devminor];
	WR(q)->q_ptr = (caddr_t)wd;
	q->q_ptr = (caddr_t)wd;
	wd->wd_qptr = WR(q);

	/* initialize the state information needed to support DL interface */

	wd->wd_state = DL_UNATTACHED;	/* it starts unattached */
	wd->wd_flags = WDS_OPEN;	/* and open */
	wd->wd_type = DL_ETHER;		/* reasonable default */
	wd->wd_no = (short)devminor;
	wd->wd_stats = NULL;	/* board specific stats */
	wd->wd_macpar = NULL;
	wd->wd_flags |= drv_priv(credp) ? 0 : WDS_SU;

	qprocson(q);
	/* update stream head for proper flow control */

	if ((mp = allocb(sizeof (struct stroptions), BPRI_MED)) == NULL) {
		rval = ENOSPC;
		goto out1;
	}
	mp->b_cont = NULL;
	mp->b_datap->db_type = M_SETOPTS;
	opts = (struct stroptions *)mp->b_rptr;
	opts->so_flags = SO_HIWAT | SO_LOWAT | SO_MAXPSZ;
	opts->so_readopt = 0;
	opts->so_wroff = 0;
	opts->so_minpsz = 0;
	opts->so_maxpsz = WDMAXPKT;
	opts->so_hiwat = WDHIWAT;
	opts->so_lowat = WDLOWAT;
	mp->b_wptr = mp->b_rptr + sizeof (struct stroptions);
	putnext(q, mp);


	/*
	 * return minor device of newly opened stream
	 */

#if defined(WDDEBUG)
	if (wd_debug & WDSYSCALL)
		cmn_err(CE_CONT, "\treturn %d\n", devminor);
#endif

	*dev = makedevice(devmajor, devminor);
	rval = 0;
out1:
	/* make this stream available */
	mutex_exit(&wd_lock);
	return (rval);
}

/*
 * wdclose is called on last close to a stream. it flushes any pending data
 * (assumes higher level protocols handle this properly) and resets state of
 * the minor device to unbound. The last close to the last open wd stream
 * will result in the board being shutdown.
 */

wdclose(queue_t *q)
{
	register struct wddev *wd;

#if defined(WDDEBUG)
	if (wd_debug & WDSYSCALL)
		cmn_err(CE_CONT, "wdclose(%x)\n", q);
#endif

	/*
	 * Queues are flushed by STREAMS.
	 *
	 * flushq(q); flushq(OTHERQ(q));
	 */

	qprocsoff(q);
	mutex_enter(&wd_lock);
	wd = (struct wddev *)q->q_ptr;
	wddodetach(wd);

	/* mark as closed */
	wd->wd_qptr = NULL;
	wd->wd_state = DL_UNATTACHED;	/* just in case */
	mutex_exit(&wd_lock);

	return (0);
}

int wdfastpath;
int wdfastqueue;

/*
 * wdwput is the "write put procedure" It's purpose is to filter messages to
 * ensure that only M_PROTO/M_PCPROTO and M_IOCTL messages come down from
 * above. Others are discarded while the selected ones are queued to the
 * "write service procedure".
 */
wdwput(queue_t *q, mblk_t *mp)
{
	register struct wddev	*wd;
	struct wdparam		*wdp;

	switch (mp->b_datap->db_type) {
		/* these three are processed in the */
		/* service procedure */
	case M_IOCTL:
	case M_PROTO:
	case M_PCPROTO:
		(void) putq(q, mp);
		break;
	case M_DATA:	/* "Fastpath" */
		wd = (struct wddev *)q->q_ptr;
		wdp = (struct wdparam *)wd->wd_macpar;

		if (mutex_owned(&wd_lock)) {
			wd->wd_flags |= WDS_XWAIT;
			(void) putq(q, mp);
			qenable(q);
			wdfastqueue++;
		} else if ((wd->wd_state == DL_IDLE) &&
		    ((wd->wd_flags & (WDS_FAST|WDS_RAW)) != 0)) {

			mutex_enter(&wd_lock);
			mutex_enter(&wdp->wd_intrlock);

			if (wdp->wd_txbuf_busy) {
				wd->wd_flags |= WDS_XWAIT;
				(void) putq(q, mp);
				wdfastqueue++;
			} else if ((wdp->wd_devmode & WDS_PROM) ||
			    (wd->wd_flags & WDS_PROM)) {
				wd->wd_flags |= WDS_XWAIT;
				(void) putq(q, mp);
				qenable(q);
				wdfastqueue++;
			} else {
				wdfastpath++;
				if (wdsend(wd, mp))
					printf("wdwput: wdsend failed\n");
				freemsg(mp);
			}
			mutex_exit(&wd->wd_macpar->wd_intrlock);
			mutex_exit(&wd_lock);
		} else {
			printf("smc: freeing packet in wdwput\n");
			freemsg(mp);	/* discard unknown messages */
		}

		break;

	case M_FLUSH:
		/* Standard M_FLUSH code */
		if (*mp->b_rptr & FLUSHW) {
			flushq(q, FLUSHDATA);
		}
		if (*mp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHDATA);
			*mp->b_rptr &= ~FLUSHW;
			qreply(q, mp);
		} else
			freemsg(mp);
		break;

		/*
		 * only M_PROTO's/M_IOCTL's/M_DATA's/M_FLUSH's of some type
		 * allowed
		 */
	default:
		/* Don't put up warning for the hwcksum probe from IP */
		if (mp->b_datap->db_type != M_CTL) {
			cmn_err(CE_WARN, "smc: unknown msg type = %d",
				mp->b_datap->db_type);
		}
		freemsg(mp);	/* discard unknown messages */
	}

	return (0);
}

/*
 * wdwsrv is the "write service procedure" Messages are processed after
 * determining their type. The LI protocol is handled here
 */
wdwsrv(queue_t *q)
{
	register mblk_t *mp;
	register struct wddev *wd;
	register struct wdparam *wdp;
	int		err;

	mutex_enter(&wd_lock);
	wd = (struct wddev *)q->q_ptr;	/* to get link proto output type */

	while ((mp = getq(q)) != NULL) {
		switch (mp->b_datap->db_type) {
		case M_IOCTL:
			wdioctl(q, mp);
			break;

			/* clean up whatever is enqueued */
		case M_FLUSH:
			if (*mp->b_rptr & FLUSHW)
				/* flush write queue */
				flushq(q, FLUSHDATA);

			if (*mp->b_rptr & FLUSHR) {
				/* flush read queue */
				flushq(RD(q), FLUSHDATA);

				*mp->b_rptr &= ~FLUSHW;
				qreply(q, mp);
			} else
				freemsg(mp);	/* get rid of this one */
			break;

			/* Will be an LI message of some type */
		case M_PROTO:
		case M_PCPROTO:
			switch (wd->wd_type) {
				/* CSMA/CD is 802.2 (and startup mode) */
			case DL_CSMACD:
				/*
				 * ETHER is old style (DIX) type
				 * encapsulation
				 */
			case DL_ETHER:
				if (err = llccmds(q, mp)) {
					/* error condition */
					if (err == WDE_NOBUFFER) {
						/* quit while we're ahead */
						wdp = wd->wd_macpar;

						/* lock this hardware device */
						mutex_enter(&wdp->wd_intrlock);

						if (wdp->wd_nextq < 0)
							wdp->wd_nextq =
								wd->wd_no;
						(void) putbq(q, mp);

						if (wdp->wd_ncount++ == 0)
						bufcall(
sizeof (union DL_primitives), BPRI_MED, (void (*)()) wdsched_wrapper,
							(long)(wdp));
						/*
						 * Be sure to unlock the
						 * hardware before we unlock
						 * the module
						 */
						mutex_exit(&wdp->wd_intrlock);
						mutex_exit(&wd_lock);
						return (0);
					} else if (err != WDE_OK) {
						if (wdnak(q, mp, err) == WDE_OK)
							freemsg(mp);
					}
				}
				break;

				/* No idea what this is, discard */
			default:
				/* error, discard */
				freemsg(mp);
				break;
			}
			break;

			/*
			 * This type of message could not have come from
			 * upstream, so must be special A later version may
			 * allow RAW output from the stream head
			 */
		case M_DATA:
			/*
			 * This is a retry of a previously processed
			 * UNITDATA_REQ or a packet from a stream in RAW mode
			 */
			wdp = wd->wd_macpar;
			mutex_enter(&wdp->wd_intrlock);
			if (wdsend(wd, mp)) {

				/*
				 * After we put the packet back on the queue
				 * return without processing another packet.
				 * Because if you do process another packet
				 * it'll be this packet again will the driver
				 * will loop.
				 */
				(void) putbq(q, mp);
				if (wdp->wd_nextq < 0)
					wdp->wd_nextq = wd->wd_no;
				mutex_exit(&wdp->wd_intrlock);
				mutex_exit(&wd_lock);
				return (0);
			} else
				freemsg(mp);
			mutex_exit(&wdp->wd_intrlock);
			break;
			/* This should never happen */
		default:
			cmn_err(CE_WARN, "smc: unknown msg type(wsrv) = %d\n",
				mp->b_datap->db_type);
			freemsg(mp);	/* unknown types are discarded */
			break;
		}
	}

	mutex_exit(&wd_lock);
	return (0);
}

/*
 * wdrsrv - "read queue service" routine called when data is put into the
 * service queue determines additional processing that might need to be done
 */
wdrsrv(queue_t *q)
{
	mblk_t			*mp;
	register struct wddev	*wd;

	mutex_enter(&wd_lock);
	wd = (struct wddev *)q->q_ptr;	/* get minor device info */
	while ((mp = getq(q)) != NULL) {
		/*
		 * determine where message goes, then call the proper handler
		 */
		if (wd->wd_flags & WDS_RAW) {
			putnext(q, mp);
			continue;
		}
		if (recv_ether(wd, q, mp) != WDE_OK) {
			freemsg(mp);
#if defined(WDDEBUG)
			if (wd_debug & WDRECV)
				cmn_err(CE_CONT, "dropped ETHER\n");
#endif
		}
	}

	mutex_exit(&wd_lock);
	return (0);
}

/*
 * wdrecv is called by the device interrupt handler to process an incoming
 * packet or by wdloop to handle a loopback packet.  The appropriate link
 * level parser/protocol handler is called if a stream is allocated for that
 * packet. Both the wd module mutex, wd_lock and the mutex for this device
 * are enabled when this routine is called.
 */
void
wdrecv(mblk_t *mp, struct wdparam *wdp)
{
	struct wddev	*wd;	/* device array pointer */
	int		msgtype, len_type_field;
	register int	i;
	int		valid;
	mblk_t		*nmp;

	/*
	 * length field value determines type of header parsing
	 */
	len_type_field = LLC_LENGTH(mp->b_rptr);
	msgtype = (len_type_field > WDMAXPKT) ? DL_ETHER : DL_CSMACD;


	/*
	 * if promiscous mode is enabled or multicast addresses are defined,
	 * test if really ours or not
	 */
	if ((wdp->wd_devmode & WDS_PROM) || (wdp->wd_multicnt > 0))
		valid = wdlocal(mp, wdp) || wdbroadcast(mp) ||
			wdmulticast(mp->b_rptr, wdp->wd_multiaddrs,
				    wdp->wd_multip, wdp->wd_multicnt);
	else
		valid = 1;

	/*
	 * Don't bother sending packets back up the SAPs if we're
	 * the ones who sent the sucker out in the first place. The
	 * only packets that return true for this test are ones we're
	 * dupping for promiscous mode.
	 * Fixes bug  3001781
	 */
	if (wdlooped(mp, wdp))
		valid = 0;

	wd = &wddevs[0];
	for (i = 0; i < wd_minors; i++, wd++) {

		/* skip an unopened stream or an open but not bound stream */
		if ((wd->wd_qptr == NULL) || (wd->wd_state != DL_IDLE) ||
		    (wdp != wd->wd_macpar))
			continue;


		/* only look at open streams of the correct type */
		if ((wd->wd_flags & WDS_PROM) ||
			(wd->wd_flags & WDS_PROM_SAP)) {
			if (!canput(RD(wd->wd_qptr))) {
				wd->wd_stats->wds_blocked++;
				wd->wd_flags |= WDS_RWAIT;
				continue;
			}
			nmp = dupmsg(mp);
			/* enqueue for the service routine to process */
			(void) putq(RD(wd->wd_qptr), mp);
			mp = nmp;
			if (nmp == NULL) {
				printf("smc%d: failed to dup message(1)\n",
				ddi_get_instance(wd->wd_macpar->wd_devi));
				break;
			}
		} else {
			if (valid &&
			    ((msgtype == DL_ETHER &&
			    wd->wd_sap == len_type_field) ||
			    ((msgtype == DL_CSMACD) &&
			    (wd->wd_sap <= WDMAXSAPVALUE)))) {
				/*
				 * if no room in the queue, skip this queue
				 * this is acceptable since it is the next
				 * higher protocol layer that has to deal
				 * with it - the LAN could lose it just as
				 * well as here -
				 */
				if (!canput(RD(wd->wd_qptr))) {
					wd->wd_flags |= WDS_RWAIT;
					wd->wd_stats->wds_blocked++;
					continue;
				}
				nmp = dupmsg(mp);

				/*
				 * enqueue for the service routine to process
				 */
				(void) putq(RD(wd->wd_qptr), mp);
				mp = nmp;
				if (nmp == NULL) {
					printf("smc%d: failed to dup msg(2)\n",
						wd->wd_macpar->wd_noboard - 1);
					break;
				}
			}
		}
	}
	if (mp != NULL)
		freemsg(mp);
}

/*
 * process receipt of non-LLC packets (type/length > 1500) Called from
 * wdrsrv.  The wd module mutex, wd_lock, is enabled on entry.
 */
static
recv_ether(struct wddev *wd, queue_t *q, mblk_t *mp)
{
	mblk_t				*nmp;	/* for control portion */
	register struct llca		*llcp;
	struct wd_machdr		*rcvhdr;
	register union DL_primitives	*dlp;

	if (canput(q->q_next)) {

		/* get a message block for new header information */
		if ((nmp = allocb(sizeof (union DL_primitives) +
		    (2 * LLC_ENADDR_LEN), BPRI_MED)) == NULL) {
			return (WDE_NOBUFFER);
		}
		nmp->b_cont = NULL;
		rcvhdr = (struct wd_machdr *)mp->b_rptr;

		/*
		 * Need to make it control info
		 */
		nmp->b_datap->db_type = M_PROTO;

		dlp = (union DL_primitives *)nmp->b_rptr;
		dlp->unitdata_ind.dl_primitive = DL_UNITDATA_IND;
		dlp->unitdata_ind.dl_dest_addr_length = LLC_ENADDR_LEN;
		dlp->unitdata_ind.dl_dest_addr_offset = DL_UNITDATA_IND_SIZE;
		dlp->unitdata_ind.dl_src_addr_length = LLC_ENADDR_LEN;
		dlp->unitdata_ind.dl_src_addr_offset = DL_UNITDATA_IND_SIZE +
			LLC_ENADDR_LEN;

		/*
		 * indicate if packet is multicast/broadcast or not
		 */
		dlp->unitdata_ind.dl_group_address = rcvhdr->mac_dst[0] & 0x01;

		/*
		 * insert real data for UNITDATA_ind message
		 */
		llcp = (struct llca *)(((caddr_t)dlp) + DL_UNITDATA_IND_SIZE);
		BCOPY(rcvhdr->mac_dst, llcp->lbf_addr, LLC_ADDR_LEN);
		llcp->lbf_sap = ETHER_TYPE(rcvhdr);
		llcp++;		/* get next position */
		BCOPY(rcvhdr->mac_src, llcp->lbf_addr, LLC_ADDR_LEN);
		llcp->lbf_sap = ETHER_TYPE(rcvhdr);
		nmp->b_wptr = nmp->b_rptr + (DL_UNITDATA_IND_SIZE +
			2 * LLC_ENADDR_LEN);

		mp->b_rptr += LLC_EHDR_SIZE;
		if (mp->b_rptr == mp->b_wptr) {
			mblk_t *nullblk;

			/* get rid of null block */
			nullblk = mp;
			mp = unlinkb(nullblk);
			freeb(nullblk);
		}
		linkb(nmp, mp);
		putnext(q, nmp);
#if defined(WDDEBUG)
		if (wd_debug & WDRECV)
			cmn_err(CE_CONT, "sent upstream\n");
#endif
	} else {
		freemsg(mp);	/* shouldn't flow control upward */
		wd->wd_stats->wds_blocked2++;
	}
	return (WDE_OK);
}

/*
 * process the DLPI commands as defined in dlpi.h Called from wdwsrv.  The wd
 * module mutex, wd_lock, is enabled on entry.
 */
static
llccmds(queue_t *q, mblk_t *mp)
{
	union DL_primitives *dlp;

	dlp = (union DL_primitives *)mp->b_rptr;

	switch ((int)dlp->dl_primitive) {
	case DL_BIND_REQ:
		return (wdbind(q, mp));

	case DL_UNBIND_REQ:
		return (wdunbind(q, mp));

	case DL_UNITDATA_REQ:
		return (wdunitdata(q, mp));

	case DL_INFO_REQ:
		return (wdinforeq(q, mp));

	case DL_ATTACH_REQ:
		return (wdattachreq(q, mp));

	case DL_DETACH_REQ:
		return (wddetachreq(q, mp));

	case DL_PROMISCON_REQ:
		return (wdpromisconreq(q, mp));

	case DL_PROMISCOFF_REQ:
		return (wdpromiscoffreq(q, mp));

	case DL_ENABMULTI_REQ:
		return (wdenabmultireq(q, mp));

	case DL_DISABMULTI_REQ:
		return (wddisabmultireq(q, mp));

	case DL_PHYS_ADDR_REQ:
		return (wdphysaddrreq(q, mp));

	case DL_SET_PHYS_ADDR_REQ:
		return (wdsetphysaddrreq(q, mp));

	/*
	 * these are known but unsupported
	 */
	case DL_XID_REQ:
	case DL_XID_RES:
	case DL_SUBS_BIND_REQ:
	case DL_SUBS_UNBIND_REQ:
	case DL_UDQOS_REQ:
	case DL_CONNECT_REQ:
	case DL_CONNECT_RES:
	case DL_TOKEN_REQ:
	case DL_DISCONNECT_REQ:
	case DL_RESET_REQ:
	case DL_RESET_RES:
	case DL_DATA_ACK_REQ:
	case DL_REPLY_REQ:
	case DL_REPLY_UPDATE_REQ:
	case DL_TEST_REQ:
	case DL_TEST_RES:
	case DL_GET_STATISTICS_REQ:
		return (DL_NOTSUPPORTED);
	default:
		return (DL_BADPRIM);	/* this is really bogus - tell user */
	}
}

/*
 * wdbind determines if a SAP is already allocated and whether it is legal to
 * do the bind at this time
 */
wdbind(queue_t *q, mblk_t *mp)
{
	int			sap, rval;
	union DL_primitives	*dlp;
	register struct wddev	*wd;

	dlp = (union DL_primitives *)mp->b_rptr;
	wd = (struct wddev *)q->q_ptr;
	sap = dlp->bind_req.dl_sap;

	if (wd->wd_qptr && wd->wd_state != DL_UNBOUND) {
		rval =  DL_OUTSTATE;
		goto out2;
	}
	if (dlp->bind_req.dl_service_mode != DL_CLDLS) {
		rval = DL_UNSUPPORTED;
		goto out2;
	}
	wd->wd_state = DL_IDLE;

	wd->wd_sap = (short)sap;

	wd->wd_type = DL_ETHER;
	wd->wd_sdu = WDMAXPKT;

	dlbindack(q, mp, sap, wd->wd_macpar->wd_macaddr, 6, 0, 0);
	rval = WDE_OK;
out2:
	return (rval);
}

/*
 * wdunbind performs an unbind of an LSAP or ether type on the stream The
 * stream is still open and can be re-bound
 */
wdunbind(queue_t *q, mblk_t *mp)
{
	struct wddev *wd;
	union DL_primitives *dlp;
	mblk_t *nmp;
	int rval;

	wd = (struct wddev *)q->q_ptr;

	if (wd->wd_state != DL_IDLE)
		return (DL_OUTSTATE);

	wd->wd_state = DL_UNBIND_PENDING;
	if ((nmp = allocb(sizeof (union DL_primitives), BPRI_MED)) == NULL) {
		/* failed to get buffer */
		wd->wd_state = DL_IDLE;
		rval = WDE_NOBUFFER;
		goto out3;
	}
	nmp->b_cont = NULL;

	freemsg(mp);		/* done with old */
	/* build the ack */
	nmp->b_datap->db_type = M_PCPROTO;	/* acks are PCproto's */

	dlp = (union DL_primitives *)nmp->b_rptr;
	dlp->ok_ack.dl_primitive = DL_OK_ACK;
	dlp->ok_ack.dl_correct_primitive = DL_UNBIND_REQ;
	nmp->b_wptr = nmp->b_rptr + DL_OK_ACK_SIZE;

	/*
	 * Note these flushes are tempoarily commented out to get around a
	 * bug in ip_if where an unbind, unattach, attach, and bind are all
	 * sent in sequence to the module without waiting for a response from
	 * wd
	 *
	 * flushq(q, FLUSHALL); flushq(RD(q), FLUSHALL);
	 */
	qreply(q, nmp);
	wd->wd_state = DL_UNBOUND;
	rval = WDE_OK;
out3:
	return (rval);
}

/*
 * wdunitdata sends a datagram destination address/lsap is in M_PROTO message
 * (1st message) data is in remainder of message
 */
wdunitdata(queue_t *q, mblk_t *mp)
{
	struct wddev			*wd;
	mblk_t				*nmp, *tmp;
	union DL_primitives		*dlp;
	register struct wd_machdr	*hdr;
	ushort				size = msgdsize(mp);

	wd = (struct wddev *)q->q_ptr;
	if (wd->wd_state != DL_IDLE)
		return (DL_OUTSTATE);

	if (size > wd->wd_sdu)
		return (DL_BADDATA);	/* reject packets larger than legal */

	/*
	 * just send it if it is an M_DATA make a valid header for transmit
	 */
	if ((nmp = allocb(sizeof (union DL_primitives), BPRI_MED)) == NULL) {
		/* failed to get buffer */
		return (WDE_NOBUFFER);
	}
	nmp->b_cont = NULL;
	dlp = (union DL_primitives *)mp->b_rptr;
	hdr = (struct wd_machdr *)nmp->b_rptr;
	BCOPY((caddr_t)dlp + dlp->unitdata_req.dl_dest_addr_offset,
		hdr->mac_dst, LLC_ADDR_LEN);
	BCOPY(wd->wd_macpar->wd_macaddr, hdr->mac_src, LLC_ADDR_LEN);

	if (wd->wd_sap < WDMAXSAPVALUE) {
		/*
		 * special case
		 */
		hdr->mac_llc.ether.ether_type = htons(size);
	} else
		hdr->mac_llc.ether.ether_type = ntohs(wd->wd_sap);
	nmp->b_wptr = nmp->b_rptr + LLC_EHDR_SIZE;
	tmp = rmvb(mp, mp);
	linkb(nmp, tmp);	/* form the new message */
	freeb(mp);		/* don't need the old head any more */

	/*
	 * make sure the device mutex is enabled before wdlocal, wdloop,
	 * wdsend, and wdmulticast, since it uses the board multicast here,
	 * are called
	 */
	mutex_enter(&wd->wd_macpar->wd_intrlock);

	if (wdsend(wd, nmp)) {
		if (wd->wd_macpar->wd_nextq < 0)
			/* we can be at head of queue for this board */
			wd->wd_macpar->wd_nextq = wd->wd_no;
		(void) putbq(q, nmp);
		mutex_exit(&wd->wd_macpar->wd_intrlock);
		return (WDE_OK);	/* this is almost correct, the result */
		/* is that the wdsend will happen again */
		/* immediately, get the same result and */
		/* then back off */
	}
	mutex_exit(&wd->wd_macpar->wd_intrlock);
	freemsg(nmp);		/* free on success */
	return (WDE_OK);
}

/*
 * wdinforeq generate the response to an info request
 */
wdinforeq(queue_t *q, mblk_t *mp)
{
	struct wddev			*wd;
	mblk_t				*nmp;
	register union DL_primitives	*dlp;
	dlpiaddr_t			*dlp_addr;

	wd = (struct wddev *)q->q_ptr;

	if ((nmp = allocb(sizeof (union DL_primitives) + ETHERADDRL +
	    DLPI_ADDR_LEN, BPRI_MED)) == NULL) {
#if defined(WDDEBUG)
		if (wd_debug & WDDLPRIM)
			cmn_err(CE_CONT, "wdinforeq nobuf...\n");
#endif

		return (WDE_NOBUFFER);
	}
	nmp->b_cont = NULL;
	freemsg(mp);
	nmp->b_datap->db_type = M_PCPROTO;	/* acks are PCproto's */

	dlp = (union DL_primitives *)nmp->b_rptr;
	dlp->info_ack.dl_primitive = DL_INFO_ACK;
	dlp->info_ack.dl_max_sdu = WDMAXPKT;
	dlp->info_ack.dl_min_sdu = 0;
	dlp->info_ack.dl_addr_length = DLPI_ADDR_LEN;	/* physical plus SAP */
	dlp->info_ack.dl_mac_type = wd->wd_type;
	dlp->info_ack.dl_reserved = 0;
	dlp->info_ack.dl_current_state = wd->wd_state;
	dlp->info_ack.dl_sap_length = -2;
	dlp->info_ack.dl_service_mode = DL_CLDLS;
	dlp->info_ack.dl_qos_length = 0;	/* No QOS yet */
	dlp->info_ack.dl_qos_offset = 0;
	dlp->info_ack.dl_qos_range_length = 0;
	dlp->info_ack.dl_qos_range_offset = 0;
	dlp->info_ack.dl_provider_style = DL_STYLE2;
	nmp->b_wptr += DL_INFO_ACK_SIZE;

	/* Return six bytes physical then two bytes SAP */
	if (wd->wd_state != DL_IDLE) {
		dlp->info_ack.dl_addr_offset = 0;
	} else {
		dlp->info_ack.dl_addr_offset = DL_INFO_ACK_SIZE;
		BCOPY(wd->wd_macpar->wd_macaddr, nmp->b_wptr, LLC_ADDR_LEN);
		dlp_addr = (dlpiaddr_t *)(nmp->b_rptr +
			dlp->info_ack.dl_addr_offset);
		dlp_addr->dlpi_sap = wd->wd_sap;
	}
	nmp->b_wptr += DLPI_ADDR_LEN;
	dlp->info_ack.dl_version = DL_VERSION_2;
	dlp->info_ack.dl_brdcst_addr_length = ETHERADDRL;
	dlp->info_ack.dl_brdcst_addr_offset = DL_INFO_ACK_SIZE + DLPI_ADDR_LEN;
	BCOPY(wdbroadaddr, nmp->b_wptr, ETHERADDRL);
	nmp->b_wptr = nmp->b_rptr + DL_INFO_ACK_SIZE +
		DLPI_ADDR_LEN + ETHERADDRL;

	dlp->info_ack.dl_growth = 0;
	qreply(q, nmp);

	return (WDE_OK);
}


/*
 * wdinit_board is called to initialize the 8003 and 8390 hardware. It is
 * called on a successfull call to wdattachreq if this particular board is
 * not currently initialized Called from wdattachreq with both device and wd
 * module mutexes enabled
 */
void
wdinit_board(struct wddev *wd)
{
	register int		i;
	register int		inval;
	register struct wdparam	*wdp;
	short			ctl_reg, cmd_reg;
	unsigned char		init_dcr = INIT_DCR;

	wdp = wd->wd_macpar;
	ctl_reg = wdp->wd_ioaddr;
	cmd_reg = ctl_reg + 0x10;

	/* reset the 8003 & program the memory decode bits */
	outb(ctl_reg, SFTRST);
	outb(ctl_reg, 0);
	if (WD16BITBRD(wdp->wd_boardtype)) {
		/*
		 * if a sixteen bit AT bus board in a sixteen bit slot,
		 * enable sixteen bit operation and include LA bits 23-19
		 */
		outb(ctl_reg,
			(char)(((long)wdp->wd_base >> 13) & 0x3E) + MEMENA);
		wdp->wd_memenable =
			(unchar)((((long)wdp->wd_base >> 19) & 0x1F) |
			MEM16ENB | LAN16ENB);
		wdp->wd_memdisable =
			(unchar)((((long)wdp->wd_base >> 19) & 0x1F) |
			LAN16ENB);
		if (wdp->wd_bflags & WDF_UNSAFE8BIT)
			outb(ctl_reg + LAAR, wdp->wd_memenable);
		else
			outb(ctl_reg + LAAR, wdp->wd_memdisable);
		/* use word transfer mode of 8390 */
		init_dcr |= WTS;
	} else if (wdp->wd_boardtype & BOARD_16BIT) {
		/*
		 * if a sixteen bit AT bus board in an eight bit slot, can
		 * only enable sixteen bit LAN operation, but not sixteen bit
		 * memory operation (include LA19).
		 * We cannot guard against an UNSAFE8BIT board such as the
		 * 8216 being put into an 8 bit slot ..., this can only be
		 * a documentation issue.
		 */
		outb(ctl_reg,
			(char)(((long)wdp->wd_base >> 13) & 0x3E) + MEMENA);
		outb(ctl_reg + LAAR,
			(unchar)(((long)wdp->wd_base >> 19) & 0x01) |
			LAN16ENB);
		/* use word transfer mode of 8390 */
		init_dcr |= WTS;
	} else
		outb(ctl_reg,
			(char)(((long)wdp->wd_base >> 13) & 0x3F) + MEMENA);

	/* initialize the 8390 lan controller device */
	inval = inb(cmd_reg);
	outb(cmd_reg, inval & PG_MSK & ~TXP);


	/* board transfer mode for PS/2 is different than AT */
	/*
	 * In 2.0 kernel (Merged product), machenv struct has changed
	 */
	if (micro_channel(wdp->wd_devi)) {
		/* Micro Channel cards all use word transfer mode of 8390 */
		init_dcr |= WTS;

		/*
		 * if Micro Channel board with interface chip
		 * (WD8003E/A), need to enable interrupts
		 */
		if (wdp->wd_boardtype & INTERFACE_CHIP)
			outb(ctl_reg + CCR, EIL);
	} else {
		/*
		 * Only touch the IRR on boards with interface chip.
		 * The WD8003E board hangs the bus if this register is touched.
		 * Rick McNeal 28-Aug-1992
		 */
		if (wdp->wd_boardtype & INTERFACE_CHIP_MASK) {
			/*
			 * make sure interrupts are enabled even if EEROM has
			 * disabled
			 */
			if ((wdp->wd_boardtype & INTERFACE_CHIP_MASK) !=
			    INTERFACE_585_CHIP) {
				inval = inb(ctl_reg + IRR);
				outb(ctl_reg + IRR, inval | IRR_IEN);
			}

			/*
			 * Turn off disable int mask on 585 chips
			 */
			if ((wdp->wd_boardtype & INTERFACE_CHIP_MASK) ==
			    INTERFACE_585_CHIP) {
				inval = inb(ctl_reg + ICR_585);
				inval &= ~ICR_MASK2;
				outb(ctl_reg + ICR_585, inval | ICR_EIL);
			}
		}
	}

	outb(cmd_reg + DCR, init_dcr);
	outb(cmd_reg + RBCR0, 0);
	outb(cmd_reg + RBCR1, 0);
	outb(cmd_reg + RCR, RCRMON);
	outb(cmd_reg + TCR, INIT_TCR);

	outb(cmd_reg + PSTART, TX_BUF_LEN >> 8);
	outb(cmd_reg + BNRY, TX_BUF_LEN >> 8);
	outb(cmd_reg + PSTOP, wdp->wd_memsize >> 8);

	outb(cmd_reg + ISR, CLR_INT);
	outb(cmd_reg + IMR, INIT_IMR);

	inval = inb(cmd_reg);
	outb(cmd_reg, (inval & PG_MSK & ~TXP) | PAGE_1);
	for (i = 0; i < LLC_ADDR_LEN; i++)
		outb(cmd_reg + PAR0 + i, wdp->wd_macaddr[i]);
	/*
	 * clear the multicast filter bits
	 */
	for (i = 0; i < 8; i++)
		outb(cmd_reg + MAR0 + i, 0);

	outb(cmd_reg + CURR, (TX_BUF_LEN >> 8) + 1);
	wdp->wd_nxtpkt = (unsigned char)((TX_BUF_LEN >> 8) + 1);

	outb(cmd_reg, PAGE_0 + ABR + STA);
	outb(cmd_reg + RCR, INIT_RCR);

	wdp->wd_txbuf_busy = 0;

	wdp->wd_init++;

	/* clear status counters */
	bzero(wd->wd_stats, sizeof (struct wdstat));
	wd->wd_stats->wds_nstats = WDS_NSTATS;	/* no of statistics counters */
}

/*
 * wduninit_board is called by the last call to close for a given board. This
 * routine disables that board's 8390 lan controller and masks out all
 * interrupts on that board. Called from wddodetach.  Has both the device and
 * wd module mutexes enabled.
 */
void
wduninit_board(struct wdparam *wdp)
{
	short		ctl_reg, cmd_reg;
	unsigned char	reg;

#if defined WDDEBUG
	if (wd_debug & WDBOARD)
		cmn_err(CE_CONT, "wduninit_board: entered\n");
#endif
	ctl_reg = wdp->wd_ioaddr;
	cmd_reg = ctl_reg + 0x10;

	/* issue STOP mode command */
	outb(cmd_reg, PAGE_0 + ABR + STP);

	/* mask out all board interrupts (to be   */
	/* sure board no longer interrupts CPU) */
	outb(cmd_reg + IMR, NO_INT);

	if (wdp->wd_boardtype & BOARD_16BIT) {
		if (!(wdp->wd_bflags & WDF_UNSAFE8BIT)) {
			/* disable LAN16ENB and MEM16ENB */
			outb(ctl_reg + LAAR,
				(char)(((long)wdp->wd_base >> 19) & 0x01));
		}
	}
	/* disable memory */
	outb(ctl_reg, (char)(((long)wdp->wd_base >> 13) & 0x3F));

	/* make sure that the ID registers are exposed */
	reg = inb(ctl_reg + 4);
	reg &= 0x7f;
	outb(ctl_reg + 4, reg);

	/* reinitialize board multicast data structures */
	wdp->wd_multiaddrs = &wdmultiaddrs[(wdp->wd_index) * wd_multisize];
	wdp->wd_multicnt = 0;
	wdp->wd_multip = wdp->wd_multiaddrs;

	wdp->wd_init = 0;
}

/*
 * wdintr is the interrupt handler for the WD8003. This routine pro- cesses
 * all types of interrupts that can be generated by the 8390 LAN controller.
 */
u_int
wdintr(caddr_t arg)
{
	struct wdparam	*wdp = (struct wdparam *)arg;
	u_char		int_reg;
	register int	inval;
	register int	i;
	unsigned char	ts_reg, orig;
	mblk_t		*bp;
	/* XXX temp fix for 3000949 -- always call wdsched */
	int		call_wdsched = 1;
	int		clear_watch_dog = 0;
	unsigned char   maxpkt;
	caddr_t		rcv_start, rcv_stop;
	short		cmd_reg, ctl_reg;
	int		ovw_tx_pending;

	/*
	 * in order for the locking to work correctly, the module mutex must
	 * always be enabled before the the device mutex
	 */
	mutex_enter(&wd_lock);
	mutex_enter(&wdp->wd_intrlock);

	/* keep track of total interrupt count for this board */
	wdstats[wdp->wd_index].wds_intrs++;

	ctl_reg = wdp->wd_ioaddr;
	cmd_reg = ctl_reg + 0x10;

	if ((int_reg = (inb(cmd_reg + ISR) & 0x7f)) == NO_INT) {
		mutex_exit(&wdp->wd_intrlock);
		mutex_exit(&wd_lock);
		return (DDI_INTR_UNCLAIMED);
	} else {
		if (!wdp->wd_init) {
			cmn_err(CE_WARN,
				"wdintr: interrupt when not initialized");
			mutex_exit(&wdp->wd_intrlock);
			mutex_exit(&wd_lock);
			return (DDI_INTR_UNCLAIMED);
		}
	}

	/* last valid packet */
	maxpkt = (wdp->wd_memsize >> 8) - 1;

	/* skip past transmit buffer */
	rcv_start = wdp->wd_rambase + TX_BUF_LEN;

	/* want end of memory */
	rcv_stop = wdp->wd_rambase + wdp->wd_memsize;

	/* disable interrupts */
	outb(cmd_reg + IMR, NO_INT);

	/* make sure CR is at page 0 */
	orig = inb(cmd_reg);
	outb(cmd_reg, orig & PG_MSK & ~TXP);

	/* mask off bits that will be handled */
	outb(cmd_reg + ISR, int_reg);

	if (int_reg & CNT) {
		wdstats[wdp->wd_index].wds_align += inb(cmd_reg + CNTR0);
		wdstats[wdp->wd_index].wds_crc += inb(cmd_reg + CNTR1);
		wdstats[wdp->wd_index].wds_lost += inb(cmd_reg + CNTR2);
	}
	if (int_reg & OVW) {
		wdstats[wdp->wd_index].wds_ovw++;

		/*
		 * In overflow only do special fix if 8390. rest of handling
		 * done after removing packets from ring in PRX handling
		 */
		if ((wdp->wd_boardtype & (NIC_690_BIT | NIC_790_BIT)) == 0) {
			ovw_tx_pending = inb(cmd_reg) & TXP;

			/* issue STOP mode command */
			outb(cmd_reg, PAGE_0 + ABR + STP);

			/* clear the Remote Byte Counter Registers */
			outb(cmd_reg + RBCR0, 0);
			outb(cmd_reg + RBCR1, 0);

			/* Make sure the 8390 has stopped */
			i = 0;
			while (i < 1500) {
				inval = inb(cmd_reg + ISR);
				if ((inval & RST) != 0)
					break;
				i++;
			}

			/* Place NIC in LOOPBACK (mode 1) */
			outb(cmd_reg + TCR, LB0_1);

			/* issue START mode command */
			outb(cmd_reg, PAGE_0 + ABR + STA);
		}
		/*
		 * Case where we got Ring Overflow and there is nothing in
		 * the ring.  Just rewrite boundary.
		 */
		inval = inb(cmd_reg);
		outb(cmd_reg, (inval & PG_MSK & ~TXP) | PAGE_1);

		if (wdp->wd_nxtpkt == (unsigned char)inb(cmd_reg + CURR)) {
#if defined(WDDEBUG)
			if (wd_debug & WDINTR)
				cmn_err(CE_CONT, "OVW and empty ring\n");
#endif
			/* set CR to page 0 & set BNRY to new value */
			inval = inb(cmd_reg);
			outb(cmd_reg, inval & PG_MSK & ~TXP);
			inval = inb(cmd_reg + BNRY);
			outb(cmd_reg + BNRY, inval);
		}
	}
	if (int_reg & PRX) {
		inval = inb(cmd_reg);
		outb(cmd_reg, (inval & PG_MSK & ~TXP) | PAGE_1);

		while (wdp->wd_nxtpkt != (unsigned char)inb(cmd_reg + CURR)) {
			rcv_buf_t *rp, *ram_rp;
			static rcv_buf_t rbuf;
			unsigned short length;

			wdstats[wdp->wd_index].wds_rpkts++;

			/* set up ptr to packet & update nxtpkt */
			ram_rp = (rcv_buf_t *)
				(wdp->wd_rambase + (int)(wdp->wd_nxtpkt << 8));

			if (WD16BITBRD(wdp->wd_boardtype) &&
			    (!(wdp->wd_bflags & WDF_UNSAFE8BIT)))
				outb(ctl_reg + LAAR, wdp->wd_memenable);
			wdbcopy((char *)ram_rp,
				(char *)&rbuf, sizeof (rcv_buf_t),
				SRC_ALIGN);
			if (WD16BITBRD(wdp->wd_boardtype) &&
			    (!(wdp->wd_bflags & WDF_UNSAFE8BIT)))
				outb(ctl_reg + LAAR, wdp->wd_memdisable);

			rp = &rbuf;
			if ((rp->status & 0x01) == 0) {
				wdp->wd_nxtpkt = (unsigned char)(rp->nxtpg);
				continue;
			}
			if ((unsigned char)(rp->nxtpg) > maxpkt) {
				break;
			}
			/* get length of packet w/o CRC field */
			length = LLC_LENGTH(&rp->pkthdr);
			if (length > WDMAXPKT) {
				/* DL_ETHER */
				if ((wdp->wd_boardtype & MEDIA_MASK) ==
				    STARLAN_MEDIA) {
					int high_count, low_count, pgs_used;

					/* Workaround the 8390/StarLAN bug */
					low_count = rp->datalen & 0xff;
					if (rp->nxtpg > wdp->wd_nxtpkt)
						/*
						 * packet didn't wrap around
						 * end of rcv ring
						 */
						pgs_used = rp->nxtpg -
							wdp->wd_nxtpkt;
					else
						pgs_used =
							(wdp->wd_memsize >> 8) -
							wdp->wd_nxtpkt +
							rp->nxtpg -
							(TX_BUF_LEN >> 8);
					if ((low_count + 4) > 0x100) {
						/*
						 * if the 4-byte header added
						 * on the receiving end
						 * causes an extra page to be
						 * used, subtract that out
						 * now
						 */
						high_count = pgs_used - 2;
					} else
						high_count = pgs_used - 1;

					/* subtract the 4 bytes for the CRC */
					length = (high_count << 8) +
						low_count - 4;
				} else
					/*
					 * length should be correct if not
					 * StarLAN
					 */
					length = rp->datalen - 4;
			} else {
				/*
				 * DL_CSMACD rp->datalen can be wrong
				 * (hardware bug) -- use llc length. the llc
				 * length is 18 bytes shorter than datalen...
				 */
				length += 14;
			}

			wdp->wd_nxtpkt = (unsigned char)(rp->nxtpg);

			if (((int)length > WDMAXPKT + LLC_EHDR_SIZE) ||
			    ((int)length < LLC_EHDR_SIZE)) {
				/* garbage packet? - toss it */
				call_wdsched++;
				/* set CR to page 0 & set BNRY to new value */
				inval = inb(cmd_reg);
				outb(cmd_reg, inval & PG_MSK & ~TXP);
				if ((int)(wdp->wd_nxtpkt - 1) <
				    (TX_BUF_LEN >> 8))
					outb(cmd_reg + BNRY,
						(wdp->wd_memsize >> 8) - 1);
				else
					outb(cmd_reg + BNRY,
						wdp->wd_nxtpkt - 1);
				break;
			}
			wdstats[wdp->wd_index].wds_rbytes += length;

			/*
			 * get buffer to put packet in & move it there
			 * NOTE: We add four to the first length so that
			 * 	that we can guarantee we'll have room to
			 *	align things such that the ip header is
			 *	on a four byte boundary.
			 */
			if ((bp = allocb(length + 4, BPRI_MED)) != NULL ||
			    (bp = allocb(nextsize(length), BPRI_MED)) != NULL) {
				caddr_t dp, cp;
				u_int cnt;

				bp->b_cont = NULL;

				/*
				 * Start the read pointer out on a 2 byte
				 * boundary so that the ip header ends up
				 * being 4 byte aligned. By doing this we
				 * avoid doing a pullupmsg() in the ip layer.
				 */
				bp->b_rptr = (u_char *)((int)bp->b_rptr | 2);

				dp = (caddr_t)bp->b_rptr;
				cp = (caddr_t)&ram_rp->pkthdr;

				/*
				 * set new value for b_wptr
				 */
				bp->b_wptr = bp->b_rptr + length;

				/*
				 * See if there is a wraparound. If there is
				 * remove the packet from its start to
				 * rcv_stop, set cp to rcv_start and remove
				 * the rest of the packet. Otherwise, re-
				 * move the entire packet from the given
				 * location.
				 */
				if (cp + length >= rcv_stop) {
					cnt = (int)rcv_stop - (int)cp;

					if (WD16BITBRD(wdp->wd_boardtype) &&
					    (!(wdp->wd_bflags &
						WDF_UNSAFE8BIT)))
						outb(ctl_reg + LAAR,
							wdp->wd_memenable);
					wdbcopy(cp, dp, cnt, SRC_ALIGN);
					if (WD16BITBRD(wdp->wd_boardtype) &&
					    (!(wdp->wd_bflags &
						WDF_UNSAFE8BIT)))
						outb(ctl_reg + LAAR,
							wdp->wd_memdisable);

					length -= cnt;
					cp = rcv_start;
					dp += cnt;
				}

				if (WD16BITBRD(wdp->wd_boardtype) &&
				    (!(wdp->wd_bflags & WDF_UNSAFE8BIT)))
					outb(ctl_reg + LAAR, wdp->wd_memenable);
				wdbcopy(cp, dp, length, SRC_ALIGN);
				if (WD16BITBRD(wdp->wd_boardtype) &&
				    (!(wdp->wd_bflags & WDF_UNSAFE8BIT)))
					outb(ctl_reg + LAAR,
						wdp->wd_memdisable);

				/* Call service routine */
				wdrecv(bp, wdp);

			} else {
				/*
				 * keep track for possible management
				 */
				wdstats[wdp->wd_index].wds_nobuffer++;
				call_wdsched++;
			}

			/*
			 * set CR to page 0 & set BNRY to new value
			 */
			inval = inb(cmd_reg);
			outb(cmd_reg, inval & PG_MSK & ~TXP);
			if (((int)wdp->wd_nxtpkt - 1) < (TX_BUF_LEN >> 8))
				outb(cmd_reg + BNRY,
					(wdp->wd_memsize >> 8) - 1);
			else
				outb(cmd_reg + BNRY, wdp->wd_nxtpkt - 1);

			inval = inb(cmd_reg);
			outb(cmd_reg, (inval & PG_MSK & ~TXP) | PAGE_1);

		}

	}			/* end if PRX int */

	/*
	 * restore CR to page 0
	 */
	inval = inb(cmd_reg);
	outb(cmd_reg, inval & PG_MSK & ~TXP);

	if ((int_reg & OVW) &&
	    ((wdp->wd_boardtype & (NIC_690_BIT | NIC_790_BIT)) == 0)) {
		/*
		 * Take NIC out of LOOPBACK
		 */
		outb(cmd_reg + TCR, INIT_TCR);

		/*
		 * If transmit bit was set, issue the transmit command again
		 */
		if (ovw_tx_pending & TXP) {
			ovw_tx_pending = 0;
			outb(cmd_reg, TXP + STA);
		}
	}

	if (int_reg & RXE) {
		/*
		 * collect network tally counters
		 */
		wdstats[wdp->wd_index].wds_align += inb(cmd_reg + CNTR0);
		wdstats[wdp->wd_index].wds_crc += inb(cmd_reg + CNTR1);
		wdstats[wdp->wd_index].wds_lost += inb(cmd_reg + CNTR2);
	}

	if (int_reg & PTX) {
		/*
		 * Save the watchdog value here because wdsched() can
		 * call wdsend which will setup another timer.
		 */
		clear_watch_dog = wdp->wd_txbuf_timeout;
		wdstats[wdp->wd_index].wds_xpkts++;

		/*
		 * free the transmit buffer
		 */
		wdp->wd_txbuf_busy = 0;
		ts_reg = inb(cmd_reg + TPSR);
		if (ts_reg & TSR_COL) {
			unsigned cnt = inb(cmd_reg + TBCR0);
			wdstats[wdp->wd_index].wds_coll += cnt;
		}
		call_wdsched++;
	}

	if (int_reg & TXE) {
		clear_watch_dog = wdp->wd_txbuf_timeout;

		/*
		 * free the transmit buffer
		 */
		wdp->wd_txbuf_busy = 0;

		ts_reg = inb(cmd_reg + TPSR);
		if (ts_reg & TSR_COL) {
			unsigned cnt = inb(cmd_reg + TBCR0);
			wdstats[wdp->wd_index].wds_coll += cnt;
		}

		if (ts_reg & TSR_ABT)
			wdstats[wdp->wd_index].wds_excoll++;

		call_wdsched++;
	}

	/*
	 * Reschedule blocked writers.
	 */
	if (call_wdsched)
		wdsched(wdp);

	/*
	 * it should be safe to do this here
	 * NOTE: I don't know why the above statement was made. This
	 *	comment was in the code when I inheirented the driver.
	 *	Kind of implies a problem, but not why.
	 *	Rick McNeal -- 09-Dec-1993
	 */
	outb(cmd_reg + IMR, INIT_IMR);
	outb(cmd_reg, orig & ~TXP);

	/*
	 * Gather total input errors here
	 */
	wdstats[wdp->wd_index].wds_ierrs =
		wdstats[wdp->wd_index].wds_align +
		wdstats[wdp->wd_index].wds_crc +
		wdstats[wdp->wd_index].wds_lost;

	mutex_exit(&wdp->wd_intrlock);
	mutex_exit(&wd_lock);

	/*
	 * Clear the watchdog timeouts if set here to avoid
	 * a dead lock condition. See untimeout(9F) if you
	 * have more questions.
	 */
	if (clear_watch_dog)
		(void) untimeout(clear_watch_dog);

	return (DDI_INTR_CLAIMED);
}


/*
 * wdsched this function is called when the interrupt handler determines that
 * the board is now capable of handling a transmit buffer.  it scans all
 * queues for a specific board to determine the order in which they should be
 * rescheduled, if at all. round-robin scheduling is done by default with
 * modifications to determine which queue was really the first one to attempt
 * to transmit.  Priority is not considered at present.
 */
void
wdsched(struct wdparam *fwdp)
{
	struct wddev *fwd = &wddevs[0];	/* first stream */
	register int i, j;
	static int rrval = 0;
	register struct wddev *wd;
	register int sent = 0;

	i = (fwdp->wd_nextq < 0) ? rrval : fwdp->wd_nextq;
	wd = fwd + i;
	for (j = 0; j < fwdp->wd_minors; j++) {
		/* only look at streams associated with this device */
		if (wd->wd_macpar != fwdp) {
			continue;
		}
		if (wd->wd_flags & WDS_XWAIT) {
			mblk_t *mp;
			if (wd->wd_qptr != NULL)
				qenable(wd->wd_qptr);
			wd->wd_flags &= ~WDS_XWAIT;
			fwdp->wd_ncount--;

			/*
			 * if there is a preformatted output message
			 * (M_DATA), then send it now rather than waiting for
			 * the Streams scheduler.  Only do one on any given
			 * pass.
			 */
			if (!sent && (mp = getq(wd->wd_qptr))) {
				if (mp->b_datap->db_type != M_DATA ||
				    wdsend(wd, mp))
					(void) putbq(wd->wd_qptr, mp);
				else {
					freemsg(mp);
					sent++;
				}
			}
		}
		if (wd->wd_flags & WDS_RWAIT) {
			if (RD(wd->wd_qptr) != NULL)
				qenable(RD(wd->wd_qptr));
			wd->wd_flags &= ~WDS_RWAIT;
		}
		i = (i + 1) % fwdp->wd_minors;	/* wrap around */
		if (i == 0)
			wd = fwd;
		else
			wd++;
	}
	rrval = (rrval + 1) % fwdp->wd_minors;
	fwdp->wd_nextq = -1;
}


/*
 * wdsched_wrapper this function is a wrapper to wdsched when it is called as
 * a callback.  It simply enables the proper mutexes and calls wdsched.
 */
void
wdsched_wrapper(struct wdparam *wdp)
{
	mutex_enter(&wd_lock);
	mutex_enter(&wdp->wd_intrlock);
	wdsched(wdp);
	mutex_exit(&wdp->wd_intrlock);
	mutex_exit(&wd_lock);
}

static void
smc_watchdog(struct wdparam *wdp)
{
	mutex_enter(&wd_lock);
	mutex_enter(&wdp->wd_intrlock);
	if (wdp->wd_txbuf_busy) {
		/*
		 * we have a situation in that in the new Elite Ultra
		 * card (model 8216 that uses the 790 chip), it is using the
		 * base+0x6 register bit 0 as the interrupt enable bit.  This
		 * bit can be accidentally turned off during a drvconfig when
		 * the el_probe() routine is being called.  This results in
		 * the smc card stop generating interrupts and is disastrous
		 * in a netboot/netinstall environment.  So when this watchdog
		 * timeout happens, we have a kludge to go and look at that
		 * register and if it is wrong, fix it.  The real fix should be
		 * prohibiting driver probe routines from outb'ing.
		 */
		if ((wdp->wd_boardtype & INTERFACE_CHIP_MASK) ==
		    INTERFACE_585_CHIP) {
			unsigned char   reg;

			reg = inb(wdp->wd_ioaddr + ICR_585);
			if (!(reg & ICR_EIL)) {
				reg &= ~ICR_MASK2;
				outb(wdp->wd_ioaddr + ICR_585, reg | ICR_EIL);
			}
		}
		wdstats[wdp->wd_index].wds_dog++;
		wdp->wd_txbuf_busy = 0;
		wdsched(wdp);
	}
	mutex_exit(&wdp->wd_intrlock);
	mutex_exit(&wd_lock);
}

/*
 * wdsend is called when a packet is ready to be transmitted. A pointer to a
 * M_PROTO or M_PCPROTO message that contains the packet is passed to this
 * routine as a parameter. The complete LLC header is contained in the
 * message block's control information block, and the remainder of the packet
 * is contained within the M_DATA message blocks linked to the main message
 * block. This routine should have the device mutex and module mutex enabled
 * when called.  This is a bit tricky since this routine is called from
 * wdwsrv, wdunitdata, and wdsched.
 */
wdsend(struct wddev *wd, mblk_t *mb)
{
	register unsigned int length;	/* total length of packet */
	caddr_t		txbuf;	/* ptr to transmission buffer area on 8003 */
	register mblk_t *mp;	/* ptr to M_DATA block containing the packet */
	register int    i;
	register struct wdparam *wdp;
	short cmd_reg, ctl_reg;
	mblk_t		*prom_mblk = NULL;

	/* see if the transmission buffer is free */

	if (!can_write(wd)) {
		/* can't get to board so exit w/ error value */
		return (1);
	}

	wdp = wd->wd_macpar;
	ctl_reg = wdp->wd_ioaddr;
	cmd_reg = ctl_reg + 0x10;
	txbuf = wdp->wd_rambase;
	length = 0;

	/*
	 * If the interface is in promiscous mode save a copy of the
	 * data. After we've kicked off the transfer we'll send the packet
	 * on it's way back up the stream during our idle time.
	 */
	if (wdp->wd_devmode & WDS_PROM)
		prom_mblk = copymsg(mb);

	/* load the packet header onto the board */
	if (WD16BITBRD(wdp->wd_boardtype) &&
	    (!(wdp->wd_bflags & WDF_UNSAFE8BIT)))
		outb(ctl_reg + LAAR, wdp->wd_memenable);

	i = (int)(mb->b_wptr - mb->b_rptr);
	wdbcopy((caddr_t)mb->b_rptr, txbuf, i, DEST_ALIGN);

	if (WD16BITBRD(wdp->wd_boardtype) &&
	    (!(wdp->wd_bflags & WDF_UNSAFE8BIT)))
		outb(ctl_reg + LAAR, wdp->wd_memdisable);

	length += (unsigned int)i;
	mp = mb->b_cont;

	/*
	 * load the rest of the packet onto the board by chaining through the
	 * M_DATA blocks attached to the M_PROTO header. The list of data
	 * messages ends when the pointer to the current message block is
	 * NULL
	 */
	while (mp != NULL) {
		if (WD16BITBRD(wdp->wd_boardtype) &&
		    (!(wdp->wd_bflags & WDF_UNSAFE8BIT)))
			outb(ctl_reg + LAAR, wdp->wd_memenable);
		wdbcopy((caddr_t)mp->b_rptr, txbuf + length,
			(int)(mp->b_wptr - mp->b_rptr), DEST_ALIGN);
		if (WD16BITBRD(wdp->wd_boardtype) &&
		    (!(wdp->wd_bflags & WDF_UNSAFE8BIT)))
			outb(ctl_reg + LAAR, wdp->wd_memdisable);

		length += (unsigned int)(mp->b_wptr - mp->b_rptr);
		mp = mp->b_cont;
	}

	wdstats[wdp->wd_index].wds_xbytes += length;
	/* check length field for proper value; pad if needed */
	if (length < WDMINSEND)
		length = WDMINSEND;

	/* packet loaded; now tell 8390 to start the transmission */
	i = inb(cmd_reg);
	outb(cmd_reg, i & PG_MSK);
	outb(cmd_reg + TPSR, 0);
	outb(cmd_reg + TBCR0, (unsigned char)length);
	outb(cmd_reg + TBCR1, (unsigned char)(length >> 8));
	i = inb(cmd_reg);
	outb(cmd_reg, i | TXP);

	wdp->wd_txbuf_timeout = timeout(smc_watchdog, (caddr_t)wdp, HZ);

	/*
	 * Now loop the packet if the interface was in promiscous mode
	 */
	if (prom_mblk != NULL)
		wdloop(prom_mblk, wd);

	/* transmission started; report success */
	return (0);

}

/*
 * can_write provides a semaphore mechanism for the operation of the
 * tx_buf_busy flag. Called from wdsend with both the module and device
 * mutexes enabled
 */
static
can_write(struct wddev *wd)
{
	if (wd->wd_qptr == NULL) {
		panic("can_write: WARNING: called with NULL qptr.\n");
	}
	if (wd->wd_macpar->wd_txbuf_busy) {
		/*
		 * buffer is in use; restore interrupts and return failure
		 * value
		 */
		wd->wd_flags |= WDS_XWAIT;	/* wait for transmitter */
		return (0);
	} else {
		/*
		 * buffer is free; claim it, restore interrupts and return
		 * true
		 */
		wd->wd_macpar->wd_txbuf_busy++;

		return (1);
	}
}

/*
 * wdlocal checks to see if the message is addressed to this system by
 * comparing with the board's address
 */
wdlocal(mblk_t *mp, struct wdparam *wd)
{
	struct ether_header *hdr = (struct ether_header *)mp->b_rptr;

	return (BCMP(hdr->ether_dhost.ether_addr_octet, wd->wd_macaddr,
		LLC_ADDR_LEN) == 0);
}

wdlooped(mblk_t *mp, struct wdparam *wd)
{
	struct ether_header *hdr = (struct ether_header *)mp->b_rptr;

	return (BCMP(hdr->ether_shost.ether_addr_octet, wd->wd_macaddr,
		LLC_ADDR_LEN) == 0);
}

wdbroadcast(mblk_t *mp)
{
	return (BCMP(mp->b_rptr, wdbroadaddr, LLC_ADDR_LEN) == 0);
}

/*
 * wdmulticast checks to see if a multicast address that is being listened to
 * is being addressed.  A side effect here is that firstp, if not NULL, is
 * altered.  This is used to update the wd_multip field is updated.
 */
struct wdmaddr *
wdmulticast(u_char *cp, struct wdmaddr *listp, struct wdmaddr *firstp,
	int count)
{
	register int    i;
	struct wdmaddr *savep;

	if (count == 0)
		return (NULL);

	/*
	 * If we're avoiding the incerement side effect of this routine by
	 * passing a NULL firstp, set it to something reasonable
	 */
	if (firstp == NULL)
		firstp = listp;
	savep = firstp;

	/*
	 * search the list of multicast addrs for this board, starting with
	 * the most recently referenced;  this may require wraparound on the
	 * list
	 */
	i = listp - firstp;
	do {
		if (BCMP(cp, firstp->entry, LLC_ADDR_LEN) == 0)
			return (firstp);
		if (++i >= wd_multisize) {	/* wraparound */
			i = 0;
			firstp = listp;
		} else
			firstp++;
	} while (firstp != savep);

	return (NULL);
}

/*
 * wdloop puts a formerly outbound message onto the input queue.  Needed
 * since the board can't receive messages it sends to itself This is called
 * from wdunitdata which has enabled the wd module mutex and the device
 * mutex.
 */
void
wdloop(mblk_t *mp, struct wddev *wd)
{
	if (mp != NULL)
		wdrecv(mp, wd->wd_macpar);
}

/*
 * wdnack builds an error acknowledment for the primitive All operations have
 * potential error acknowledgments unitdata does not have a positive ack
 */
wdnak(queue_t *q, mblk_t *mp, int err)
{
	mblk_t *nmp;
	union DL_primitives *dlp, *orig;

	if ((nmp = allocb(sizeof (union DL_primitives), BPRI_MED)) == NULL) {
		(void) putbq(q, mp);
		bufcall(sizeof (union DL_primitives), BPRI_MED,
			qenable, (long)q);
		return (WDE_NOBUFFER);
	}
	nmp->b_cont = NULL;

	/*
	 * sufficient resources to make an ACK. make original message easily
	 * used to get primitive in question
	 */
	orig = (union DL_primitives *)mp->b_rptr;
	dlp = (union DL_primitives *)nmp->b_rptr;
	dlp->error_ack.dl_primitive = DL_ERROR_ACK;

	/* this is the failing opcode */
	dlp->error_ack.dl_error_primitive = orig->dl_primitive;

	dlp->error_ack.dl_errno = (err & WDE_SYSERR) ? DL_SYSERR : err;
	dlp->error_ack.dl_unix_errno = (err & WDE_SYSERR) ?
		(err & WDE_ERRMASK) : 0;

	nmp->b_wptr = nmp->b_rptr + DL_ERROR_ACK_SIZE;
	nmp->b_datap->db_type = M_PCPROTO;	/* acks are PCproto's */

	qreply(q, nmp);
	return (WDE_OK);
}

/*
 * wdioctl handles all ioctl requests passed downstream. This routine is
 * passed a pointer to the message block with the ioctl request in it, and a
 * pointer to the queue so it can respond to the ioctl request with an ack.
 * The module mutex is already enabled on entry.  Device mutexing is pretty
 * gross here.
 */
void
wdioctl(queue_t *q, mblk_t *mb)
{
	struct iocblk	*iocp;
	struct wddev	*wd;
	mblk_t		*stats, *bt;
	short		cmd_reg;
	struct wdparam	*wdp;

	iocp = (struct iocblk *)mb->b_rptr;
	wd = (struct wddev *)q->q_ptr;

	/* can't do anything if the device is not attached */
	if ((wdp = wd->wd_macpar) == NULL)
		goto iocnak;

	mb->b_datap->db_type = M_IOCACK;	/* assume ACK */

	/* lock this device */
	cmd_reg = wdp->wd_ioaddr + 0x10;

	/* XXX make sure we support all of the ioctls that le does */
	switch (iocp->ioc_cmd) {
	case DLIOCRAW:		/* raw M_DATA mode */
		wd->wd_flags |= WDS_RAW;
		qreply(q, mb);
		break;

	case DL_IOC_HDR_INFO:	/* M_DATA "fastpath" info request */
		wd_dl_ioc_hdr_info(q, mb);
		break;

	case NET_GETBROAD:
	case NET_ADDR: {
			unsigned char  *dp;
			register int    i;

			/* if we don't have a six byte data buffer send NAK */
			if (iocp->ioc_count != LLC_ADDR_LEN) {
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}
			/* get address of location to put data */
			dp = mb->b_cont->b_rptr;

			/*
			 * see which address is requested & move it in buffer
			 */
			if (iocp->ioc_cmd == NET_GETBROAD) {
				/* copy Ethernet/Starlan broadcast address */
				for (i = 0; i < LLC_ADDR_LEN; i++) {
					*dp = wdbroadaddr[i];
					dp++;
				}
			} else {
				mutex_enter(&wdp->wd_intrlock);

				/* copy host's physical address */
				for (i = 0; i < LLC_ADDR_LEN; i++) {
					*dp = wdp->wd_macaddr[i];
					dp++;
				}
				mutex_exit(&wdp->wd_intrlock);
			}

			/* send the ACK */
			qreply(q, mb);
			break;
		}

	case DLSADDR:		/* set MAC address to new value */
		mutex_enter(&wdp->wd_intrlock);
		if (mb->b_cont && (mb->b_cont->b_wptr - mb->b_cont->b_rptr) ==
		    LLC_ADDR_LEN) {
			BCOPY(mb->b_cont->b_rptr, wdp->wd_macaddr,
				LLC_ADDR_LEN);
		} else {
			iocp->ioc_error = EINVAL;
			mb->b_datap->db_type = M_IOCNAK;
		}
		mutex_exit(&wdp->wd_intrlock);
		qreply(q, mb);
		break;

#ifdef notdef
	/*
	 * This IOCTL is defined in smc.h and doesn't appear to be used
	 * anywhere else and also causes conflict with DL_IOC_HDR_INFO
	 * which is needed for fastpath.
	 */
	case DLSMULT:
		{		/* turn on a multicast address */
			register struct wdmaddr	*mp;
			register int		i;
			int			row, col;
			u_char			val;
			int			err = 0;

			mutex_enter(&wdp->wd_intrlock);
			if (wdp->wd_multicnt >= wd_multisize) {
				iocp->ioc_error = ENOSPC;
				err++;
				/* keep compiler happy */
				mp = NULL;
			} else if (mb->b_cont &&
			    ((mb->b_cont->b_wptr - mb->b_cont->b_rptr) ==
			    LLC_ADDR_LEN) &&
			    ((unsigned char)mb->b_cont->b_rptr[0] & 0x01)) {
				/*
				 * check for illegal or previously defined
				 * addresses
				 */
				if (wdmulticast(mb->b_cont->b_rptr,
						wdp->wd_multiaddrs,
						wdp->wd_multip,
						wdp->wd_multicnt) != 0) {
					mutex_exit(&wdp->wd_intrlock);
					qreply(q, mb);
					break;
				}
				mp = wdp->wd_multiaddrs;
				for (i = 0; i < wd_multisize; i++, mp++)
					/* find the first empty slot */
					if (mp->entry[0] == 0)
						break;
				wdp->wd_multicnt++;
				BCOPY(mb->b_cont->b_rptr, mp->entry,
					LLC_ADDR_LEN);
			} else {
				iocp->ioc_error = EINVAL;
				err++;
				/* keep compiler happy */
				mp = NULL;
			}

			if (err) {
				mb->b_datap->db_type = M_IOCNAK;
				mutex_exit(&wdp->wd_intrlock);
				qreply(q, mb);
				return;
			}
			qreply(q, mb);
			/*
			 * map the address to the range [0,63] and set the
			 * corresponding multicast register filter bit
			 */
			mp->filterbit = wdhash(mp->entry);
			row = mp->filterbit / 8;
			col = mp->filterbit % 8;
			outb(cmd_reg, PAGE_1);
			val = inb(cmd_reg + MAR0 + row);
			val |= 0x01 << col;
			outb(cmd_reg + MAR0 + row, val);
			outb(cmd_reg, PAGE_0);
			mutex_exit(&wdp->wd_intrlock);
			break;
		}

	case DLDMULT:
		{		/* delete a multicast address */
			register unsigned char index;
			register int	i;
			struct wdmaddr	*mp;
			unsigned char	val;
			int		row, col;

			mutex_enter(&wdp->wd_intrlock);
			if (mb->b_cont &&
			    ((mb->b_cont->b_wptr - mb->b_cont->b_rptr) ==
			    LLC_ADDR_LEN) &&
			    (wdp->wd_multicnt > 0) &&
			    (wdmulticast(mb->b_cont->b_rptr,
					wdp->wd_multiaddrs, wdp->wd_multip,
					wdp->wd_multicnt))) {
				unsigned char *c = wdp->wd_multip->entry;
				for (index = 0; index < LLC_ADDR_LEN;
				    index++, c++)
					*c = 0;
				wdp->wd_multip->filterbit = 0xFF;
				wdp->wd_multicnt--;
			} else {
				iocp->ioc_error = EINVAL;
				mb->b_datap->db_type = M_IOCNAK;
				mutex_exit(&wdp->wd_intrlock);
				qreply(q, mb);
				return;
			}
			qreply(q, mb);

			/*
			 * map the address to the range [0,63] and turn the
			 * corresponding multicast register filter bit off
			 */
			index = wdhash((unsigned char *)mb->b_cont->b_rptr);

			/*
			 * make sure the filter bit isn't referenced by
			 * another defined multicast address
			 */
			mp = wdp->wd_multiaddrs;
			for (i = 0; i < wd_multisize; i++, mp++) {
				if (mp->entry[0] && index == mp->filterbit) {
					mutex_exit(&wdp->wd_intrlock);
					return;
				}
			}

			outb(cmd_reg, PAGE_1);
			row = index / 8;
			col = index % 8;
			val = inb(cmd_reg + MAR0 + row);
			val &= ~(0x01 << col);
			outb(cmd_reg + MAR0 + row, val);
			outb(cmd_reg, PAGE_0);
			mutex_exit(&wdp->wd_intrlock);
			break;
		}

	case DLGMULT:
		{		/* get multicast address list */
			register struct wdmaddr *mp;
			register int		i;
			int			found = 0;
			u_char			*dp;

			if (iocp->ioc_count <= 0) {
				/*
				 * no space provided; return number of
				 * multicast addresses defined
				 */
				iocp->ioc_rval = wdp->wd_multicnt;
			} else {
				/* copy as many addresses as space allows */
				dp = mb->b_cont->b_rptr;
				mutex_enter(&wdp->wd_intrlock);
				mp = wdp->wd_multiaddrs;
				for (i = 0;
				    (i < wd_multisize) &&
				    (dp < mb->b_cont->b_wptr); i++, mp++)
					if (mp->entry[0]) {
						BCOPY(mp->entry, dp,
							LLC_ADDR_LEN);
						dp += LLC_ADDR_LEN;
						found++;
					}
				mutex_exit(&wdp->wd_intrlock);
				iocp->ioc_rval = found;
				mb->b_cont->b_wptr = dp;
			}
			qreply(q, mb);
			break;
		}
#endif
	case NET_WDBRDTYPE:

		if ((bt = allocb(sizeof (wdp->wd_boardtype), BPRI_MED)) ==
		    NULL) {
			iocp->ioc_error = ENOSR;
			goto iocnak;
		}
		BCOPY(&wdp->wd_boardtype, bt->b_wptr,
			sizeof (wdp->wd_boardtype));
		bt->b_wptr += sizeof (wdp->wd_boardtype);
		linkb(mb, bt);
		iocp->ioc_count = sizeof (wdp->wd_boardtype);

		qreply(q, mb);
		break;

	case NET_WDSTATUS:

		if ((stats = allocb(sizeof (struct wdstat), BPRI_MED)) ==
		    NULL) {
			iocp->ioc_error = ENOSR;
			goto iocnak;
		}
		BCOPY(wd->wd_stats, stats->b_wptr, sizeof (struct wdstat));
		stats->b_wptr += sizeof (struct wdstat);
		linkb(mb, stats);
		iocp->ioc_count = sizeof (struct wdstat);

		qreply(q, mb);
		break;

	case NET_GETSTATUS:

		/* copy the errstats counters into data block. */
		if (mb->b_cont == NULL || iocp->ioc_count <= 0) {
			iocp->ioc_error = EINVAL;
			goto iocnak;
		}
		BCOPY(wd->wd_stats, mb->b_cont->b_rptr,
			min(iocp->ioc_count, sizeof (struct wdstat)));

		qreply(q, mb);
		break;

	case NET_SETPROM:	/* toggle promiscuous mode */
		if (wd->wd_flags & WDS_SU || iocp->ioc_uid == 0) {

			mutex_enter(&wdp->wd_intrlock);
			if (wd->wd_flags & WDS_PROM) {
				/* disable promiscuous mode */
				wd->wd_flags &= ~WDS_PROM;
				wdp->wd_proms--;
				if (wdp->wd_proms <= 0) {
					outb(cmd_reg, PAGE_0);

					/* just to be sure */
					wdp->wd_proms = 0;

					wdp->wd_devmode &= ~WDS_PROM;
					outb(cmd_reg + RCR, INIT_RCR);
				}
			} else {
				/* enable promiscuous mode */
				wd->wd_flags |= WDS_PROM;
				wdp->wd_proms++;
				if (!(wdp->wd_devmode & WDS_PROM)) {
					outb(cmd_reg, PAGE_0);
					wdp->wd_devmode |= WDS_PROM;
					outb(cmd_reg + RCR, INIT_RCR | PRO);
				}
			}
			mutex_exit(&wdp->wd_intrlock);
			qreply(q, mb);
			break;
		}
		goto iocnak;

	default:
		/* iocp_error = --some default error value--; */
iocnak:
		/* NAK the ioctl request */
		mb->b_datap->db_type = M_IOCNAK;
		qreply(q, mb);

	}			/* end switch */

}				/* end wdioctl */

/*
 * M_DATA "fastpath" info request.
 * Following the M_IOCTL mblk should come a DL_UNITDATA_REQ mblk.
 * We ack with an M_IOCACK pointing to the original DL_UNITDATA_REQ mblk
 * followed by an mblk containing the raw ethernet header corresponding
 * to the destination address.  Subsequently, we may receive M_DATA
 * msgs which start with this header and may send up
 * up M_DATA msgs with b_rptr pointing to a (ulong) group address
 * indicator followed by the network-layer data (IP packet header).
 * This is all selectable on a per-Stream basis.
 */
static void
wd_dl_ioc_hdr_info(wq, mp)
queue_t	*wq;
mblk_t	*mp;
{
	mblk_t	*nmp;
	struct	wddladdr	*dlap;
	dl_unitdata_req_t	*dludp;
	struct	ether_header	*headerp;
	struct	wddev		*wd = (struct wddev *)wq->q_ptr;
	struct	wdparam		*wdp = wd->wd_macpar;
	int	off, len;
	int	minsize;

	minsize = sizeof (dl_unitdata_req_t) + WDADDRL;

	/*
	 * Sanity check the request.
	 */
	if ((mp->b_cont == NULL) ||
		(MBLKL(mp->b_cont) < minsize) ||
		(*((u_long *)mp->b_cont->b_rptr) != DL_UNITDATA_REQ) ||
		(wdp == NULL)) {
		miocnak(wq, mp, 0, EINVAL);
		return;
	}

	/*
	 * Sanity check the DL_UNITDATA_REQ destination address
	 * offset and length values.
	 */
	dludp = (dl_unitdata_req_t *)mp->b_cont->b_rptr;
	off = dludp->dl_dest_addr_offset;
	len = dludp->dl_dest_addr_length;
	if (!MBLKIN(mp->b_cont, off, len) || (len != WDADDRL)) {
		miocnak(wq, mp, 0, EINVAL);
		return;
	}

	dlap = (struct wddladdr *)(mp->b_cont->b_rptr + off);

	/*
	 * Allocate a new mblk to hold the ether header.
	 */
	if ((nmp = allocb(sizeof (struct ether_header), BPRI_MED)) == NULL) {
		miocnak(wq, mp, 0, ENOMEM);
		return;
	}
	nmp->b_wptr += sizeof (struct ether_header);

	/*
	 * Fill in the ether header.
	 */
	headerp = (struct ether_header *)nmp->b_rptr;
	ether_copy(&dlap->dl_phys, &headerp->ether_dhost);
	ether_copy(&wdp->wd_macaddr, &headerp->ether_shost);
	headerp->ether_type = htons(wd->wd_sap);

	/*
	 * Link new mblk in after the "request" mblks.
	 */
	linkb(mp, nmp);

	wd->wd_flags |= WDS_FAST;

	/*
	 * XXX Don't bother calling lesetipq() here.
	 */

	miocack(wq, mp, msgsize(mp->b_cont), 0);
}

unsigned char
wdhash(unsigned char addr[])
{
	register int    i, j;
	union crc_reg   crc;
	unsigned char   fb, ch;

	crc.value = (unsigned int)-1;
	for (i = 0; i < LLC_ADDR_LEN; i++) {
		ch = addr[i];
		for (j = 0; j < 8; j++) {
			fb = crc.bits.a31 ^ ((ch >> j) & 0x01);
			crc.bits.a25 ^= fb;
			crc.bits.a22 ^= fb;
			crc.bits.a21 ^= fb;
			crc.bits.a15 ^= fb;
			crc.bits.a11 ^= fb;
			crc.bits.a10 ^= fb;
			crc.bits.a9 ^= fb;
			crc.bits.a7 ^= fb;
			crc.bits.a6 ^= fb;
			crc.bits.a4 ^= fb;
			crc.bits.a3 ^= fb;
			crc.bits.a1 ^= fb;
			crc.bits.a0 ^= fb;
			crc.value = (crc.value << 1) | fb;
		}
	}
	return ((unsigned char)(crc.value >> 26));
}


/*
 * wdattachreq associate a physical point of attachment (PPA) with a stream
 */
wdattachreq(queue_t *wq, mblk_t *mp)
{
	union DL_primitives	*dlp;
	int			ppa;
	struct wddev		*wd;
	struct wdparam		*wdp = wdparams;
	int			i;

	wd = (struct wddev *)wq->q_ptr;
	dlp = (union DL_primitives *)mp->b_rptr;

	if (MBLKL(mp) < DL_ATTACH_REQ_SIZE) {
		dlerrorack(wq, mp, DL_ATTACH_REQ, DL_BADPRIM, 0);
		return (WDE_OK);
	}
	if (wd->wd_state != DL_UNATTACHED) {
		dlerrorack(wq, mp, DL_ATTACH_REQ, DL_OUTSTATE, 0);
		return (WDE_OK);
	}
	ppa = dlp->attach_req.dl_ppa;

	/*
	 * figure out which board to attach to by matching the ppa
	 */
	for (i = wd_boardcnt; i; wdp++, i--) {
		if (wdp->wd_devi) {
			/* noboard is 1 plus it's relative number */
			if (ppa == (wdp->wd_noboard - 1))
				break;
		}
	}

	if (i == 0 || wdp->wd_noboard == 0) {
		dlerrorack(wq, mp, dlp->dl_primitive, DL_BADPPA, 0);
		return (WDE_OK);
	}
	/*
	 * fill in board-specific information
	 */
	wd->wd_macpar = wdp;	/* board specific parameters */
	mutex_enter(&wdp->wd_intrlock);
	wdp->wd_str_open++;	/* indicate how many opens */
	wd->wd_stats = &wdstats[wdp->wd_index];	/* board specific stats */

	/*
	 * Has device been initialized?  Do so if necessary.
	 */
	if (wdp->wd_init == 0) {
		wdinit_board(wd);
	}
	mutex_exit(&wdp->wd_intrlock);

	/*
	 * update our state.
	 */
	wd->wd_state = DL_UNBOUND;

	dlokack(wq, mp, DL_ATTACH_REQ);
	return (WDE_OK);
}


/*
 * wddetachreq disassociate a physical point of attachment (PPA) with a
 * stream
 */
wddetachreq(queue_t *wq, mblk_t *mp)
{
	struct wddev   *wd;

	wd = (struct wddev *)wq->q_ptr;

	if (MBLKL(mp) < DL_DETACH_REQ_SIZE) {
		dlerrorack(wq, mp, DL_DETACH_REQ, DL_BADPRIM, 0);
		return (WDE_OK);
	}
	if (wd->wd_state != DL_UNBOUND) {
		dlerrorack(wq, mp, DL_DETACH_REQ, DL_OUTSTATE, 0);
		return (WDE_OK);
	}
	wddodetach(wd);
	dlokack(wq, mp, DL_DETACH_REQ);
	return (WDE_OK);
}


/*
 * wddodetach undo any modifications to the board
 */
void
wddodetach(struct wddev *wd)
{
	struct wdparam	*wdp;
	short		cmd_reg;

	/* return if the device is already detached */
	if (wd == NULL || wd->wd_state == DL_UNATTACHED)
		return;

	wdp = wd->wd_macpar;
	if (wdp == NULL)
		return;

	/* lock this decvice */
	mutex_enter(&wdp->wd_intrlock);

	/* disable any promiscuous stuff */
	if ((wd->wd_flags & WDS_PROM) && (--(wdp->wd_proms) <= 0)) {
		cmd_reg = wdp->wd_ioaddr + 0x10;

		outb(cmd_reg, PAGE_0);
		outb(cmd_reg + RCR, INIT_RCR);	/* turn promiscuous mode off */
		wdp->wd_proms = 0;	/* just to be sure */
		wdp->wd_devmode &= ~WDS_PROM;	/* so it will work next time */
	}
	/* disable any multicast addresses */
	wdflush_multi(wd);
	wd->wd_flags = 0;	/* nothing pending */

	/*
	 * decrement number of open streams, turn board off when none left
	 */
	if (--(wdp->wd_str_open) == 0) {
		wduninit_board(wdp);
	}
	mutex_exit(&wdp->wd_intrlock);

	/* detach from the device structure */
	wd->wd_macpar = NULL;
	wd->wd_state = DL_UNATTACHED;
}

/*
 * wdpromisconreq enable promiscuous mode on a per Stream basis, either at
 * the physical SAP, or multicast level
 */
wdpromisconreq(queue_t *wq, mblk_t *mp)
{
	struct wddev	*wd;
	struct wdparam	*wdp;
	short		cmd_reg;

	wd = (struct wddev *)wq->q_ptr;

	if (MBLKL(mp) < DL_PROMISCON_REQ_SIZE) {
		dlerrorack(wq, mp, DL_PROMISCON_REQ, DL_BADPRIM, 0);
		return (WDE_OK);
	}
	switch (((dl_promiscon_req_t *)mp->b_rptr)->dl_level) {
	case DL_PROMISC_PHYS:
		/* promiscuous mode at the physical level */
		/* TODO - should we add a check for superuser? */
		/*
		 * Only set promiscuous mode if not already on
		 */
		if (!(wd->wd_flags & WDS_PROM_PHY)) {
			wdp = wd->wd_macpar;
			mutex_enter(&wdp->wd_intrlock);
			cmd_reg = wdp->wd_ioaddr + 0x10;
			wd->wd_flags |= WDS_PROM_PHY;
			wdp->wd_proms++;
			if (!(wdp->wd_devmode & WDS_PROM)) {
				outb(cmd_reg, PAGE_0);
				wdp->wd_devmode |= WDS_PROM;
				outb(cmd_reg + RCR, INIT_RCR | PRO);
			}
			mutex_exit(&wdp->wd_intrlock);
		}
		break;

	case DL_PROMISC_SAP:
		/* promiscuous mode at the SAP level */
		wd->wd_flags |= WDS_PROM_SAP;
		break;

	case DL_PROMISC_MULTI:
		/* promiscuous mode for all multicast addresses */
		wd->wd_flags |= WDS_PROM_MLT;
		break;

	default:
		dlerrorack(wq, mp, DL_PROMISCON_REQ,
			DL_NOTSUPPORTED, 0);
		return (WDE_OK);
	}

	dlokack(wq, mp, DL_PROMISCON_REQ);
	return (WDE_OK);
}


/*
 * wdpromiscoffreq disable promiscuous mode on a per Stream basis, either at
 * the physical level or at the SAP level
 */
wdpromiscoffreq(queue_t *wq, mblk_t *mp)
{
	struct wddev	*wd;
	struct wdparam	*wdp;
	short		cmd_reg;

	wd = (struct wddev *)wq->q_ptr;

	if (MBLKL(mp) < DL_PROMISCOFF_REQ_SIZE) {
		dlerrorack(wq, mp, DL_PROMISCOFF_REQ, DL_BADPRIM, 0);
		return (WDE_OK);
	}
	switch (((dl_promiscoff_req_t *)mp->b_rptr)->dl_level) {
	case DL_PROMISC_PHYS:
		/* promiscuous mode at the physical level */
		/* we can only turn it off if it is already on */
		if (!(wd->wd_flags & WDS_PROM_PHY)) {
			dlerrorack(wq, mp, DL_PROMISCOFF_REQ, DL_NOTENAB, 0);
			return (WDE_OK);
		}
		wdp = wd->wd_macpar;
		mutex_enter(&wdp->wd_intrlock);
		cmd_reg = wdp->wd_ioaddr + 0x10;
		wd->wd_flags &= ~WDS_PROM_PHY;
		wdp->wd_proms--;
		if (wdp->wd_proms <= 0) {
			outb(cmd_reg, PAGE_0);
			wdp->wd_proms = 0;	/* just to be sure */
			wdp->wd_devmode &= ~WDS_PROM_PHY;
			outb(cmd_reg + RCR, INIT_RCR);
		}
		mutex_exit(&wdp->wd_intrlock);
		break;

	case DL_PROMISC_SAP:
		/* promiscuous mode at the SAP level */
		/* we can only turn it off if it is already on */
		if (!(wd->wd_flags & WDS_PROM_SAP)) {
			dlerrorack(wq, mp, DL_PROMISCOFF_REQ, DL_NOTENAB, 0);
			return (WDE_OK);
		}
		wd->wd_flags &= ~WDS_PROM_SAP;
		break;

	case DL_PROMISC_MULTI:
		/* promiscuous mode for all multicast addresses */
		/* we can only turn it off if it is already on */
		if (!(wd->wd_flags & WDS_PROM_MLT)) {
			dlerrorack(wq, mp, DL_PROMISCOFF_REQ, DL_NOTENAB, 0);
			return (WDE_OK);
		}
		wd->wd_flags &= ~WDS_PROM_MLT;
		break;

	default:
		dlerrorack(wq, mp, DL_PROMISCOFF_REQ,
			DL_NOTSUPPORTED, 0);
		return (WDE_OK);
	}

	dlokack(wq, mp, DL_PROMISCOFF_REQ);
	return (WDE_OK);
}


/*
 * wdenabmultireq enable specific multicast addresses on a per Stream basis.
 * It is invalid for a DLS provider to pass upstream messages that are
 * destined for any address other than those explicitly enabled on that
 * Stream.
 */
wdenabmultireq(queue_t *wq, mblk_t *mp)
{
	struct wddev		*wd;
	struct wdparam		*wdp;
	union DL_primitives	*dlp;
	struct ether_addr	*addrp;
	int			off;
	int			len;
	struct wdmaddr		*maddrp;
	int			i;

	wd = (struct wddev *)wq->q_ptr;

	if (MBLKL(mp) < DL_ENABMULTI_REQ_SIZE) {
		dlerrorack(wq, mp, DL_ENABMULTI_REQ, DL_BADPRIM, 0);
		return (WDE_OK);
	}
	if (wd->wd_state == DL_UNATTACHED) {
		dlerrorack(wq, mp, DL_ENABMULTI_REQ, DL_OUTSTATE, 0);
		return (WDE_OK);
	}
	dlp = (union DL_primitives *)mp->b_rptr;
	len = dlp->enabmulti_req.dl_addr_length;
	off = dlp->enabmulti_req.dl_addr_offset;
	addrp = (struct ether_addr *)(mp->b_rptr + off);

	/*
	 * sanity check the requested address
	 */
	if ((len != ETHERADDRL) ||
	    !MBLKIN(mp, off, len) ||
	    ((addrp->ether_addr_octet[0] & 01) == 0)) {
		dlerrorack(wq, mp, DL_ENABMULTI_REQ, DL_BADADDR, 0);
		return (WDE_OK);
	}
	/*
	 * if this multicast address is already set for this stream, return
	 * ok
	 */
	if (wdmulticast((u_char *)addrp, wd->wd_maddrs, (struct wdmaddr *)NULL,
			wd->wd_mcount)) {
		dlokack(wq, mp, DL_ENABMULTI_REQ);
		return (WDE_OK);
	}
	/*
	 * if there isn't any room to add this multicast address for this
	 * stream, return error
	 */
	if (wd->wd_mcount >= wd_multisize) {
		dlerrorack(wq, mp, DL_ENABMULTI_REQ, DL_TOOMANY, 0);
		return (WDE_OK);
	}
	/*
	 * add this multicast address to the board list only if it isn't
	 * already set and there is room, otherwise bump up the stream count
	 * for this multicast
	 */
	wdp = wd->wd_macpar;
	mutex_enter(&wdp->wd_intrlock);
	if ((maddrp = (wdmulticast((u_char *)addrp, wdp->wd_multiaddrs,
	    wdp->wd_multip, wdp->wd_multicnt))) == NULL) {
		/* do we have the space? */
		if (wdp->wd_multicnt >= wd_multisize) {
			mutex_exit(&wdp->wd_intrlock);
			dlerrorack(wq, mp, DL_ENABMULTI_REQ, DL_TOOMANY, 0);
			return (WDE_OK);
		}
		/* find the first empty slot */
		maddrp = wdp->wd_multiaddrs;
		for (i = 0; i < wd_multisize; i++, maddrp++) {
			if (maddrp->entry[0] == 0)
				break;
		}
		BCOPY(addrp, maddrp->entry, LLC_ADDR_LEN);

		/*
		 * program the board for the new multicast address
		 */
		wd_addmulti(wdp, maddrp);
		wdp->wd_multicnt++;
	} else
		maddrp->count++;
	mutex_exit(&wdp->wd_intrlock);

	/*
	 * add the address for this stream and increment the count of
	 * multicast addresses for this stream
	 */
	maddrp = wd->wd_maddrs;
	for (i = 0; i < wd_multisize; i++, maddrp++) {
		if (maddrp->entry[0] == 0)
			break;
	}

	BCOPY(addrp, maddrp->entry, LLC_ADDR_LEN);
	wd->wd_mcount++;
	dlokack(wq, mp, DL_ENABMULTI_REQ);

	return (WDE_OK);
}


/*
 * wddisabmultireq disable specific multicast addresses on a per Stream basis
 */
wddisabmultireq(queue_t *wq, mblk_t *mp)
{
	struct wddev		*wd;
	struct wdparam		*wdp;
	union DL_primitives	*dlp;
	struct ether_addr	*addrp;
	int			off;
	int			len;
	int			i;
	struct wdmaddr		*maddrp;
	unsigned char		*cp;

	wd = (struct wddev *)wq->q_ptr;

	if (MBLKL(mp) < DL_DISABMULTI_REQ_SIZE) {
		dlerrorack(wq, mp, DL_DISABMULTI_REQ, DL_BADPRIM, 0);
		return (WDE_OK);
	}
	if (wd->wd_state == DL_UNATTACHED) {
		dlerrorack(wq, mp, DL_DISABMULTI_REQ, DL_OUTSTATE, 0);
		return (WDE_OK);
	}
	dlp = (union DL_primitives *)mp->b_rptr;
	len = dlp->disabmulti_req.dl_addr_length;
	off = dlp->disabmulti_req.dl_addr_offset;
	addrp = (struct ether_addr *)(mp->b_rptr + off);

	/*
	 * sanity check the address given
	 */
	if ((len != ETHERADDRL) || !MBLKIN(mp, off, len)) {
		dlerrorack(wq, mp, DL_DISABMULTI_REQ, DL_BADADDR, 0);
		return (WDE_OK);
	}
	/*
	 * return error if this address is not set for this stream
	 */
	if (((maddrp = wdmulticast((u_char *)addrp, wd->wd_maddrs,
	    (struct wdmaddr *)NULL, wd->wd_mcount))) ==
	    (struct wdmaddr *)NULL) {
		dlerrorack(wq, mp, DL_DISABMULTI_REQ, DL_NOTENAB, 0);
		return (WDE_OK);
	}
	/*
	 * remove the address from the list of multicasts for this stream
	 */
	cp = maddrp->entry;
	for (i = 0; i < ETHERADDRL; i++)
		cp[i] = 0;
	wd->wd_mcount--;

	/*
	 * return error if the board doesn't know about this
	 */
	wdp = wd->wd_macpar;
	mutex_enter(&wdp->wd_intrlock);
	if (((maddrp = wdmulticast((u_char *)addrp, wdp->wd_multiaddrs,
	    wdp->wd_multip, wdp->wd_multicnt))) == NULL) {
		mutex_exit(&wdp->wd_intrlock);
		dlerrorack(wq, mp, DL_DISABMULTI_REQ, DL_NOTENAB, 0);
		return (WDE_OK);
	}
	/*
	 * remove the address from the list of multicasts for the board if
	 * this is the only stream using it
	 */
	if (maddrp->count <= 1) {
		wd_delmulti(wdp, maddrp);
	} else
		maddrp->count--;
	mutex_exit(&wdp->wd_intrlock);

	dlokack(wq, mp, DL_DISABMULTI_REQ);
	return (WDE_OK);
}

/*
 * wdphysaddrreq return either the default (factory) or the current value of
 * the address associated with the stream depending on the value of the
 * address type selected in the request NOTE: this primitive is optional for
 * DLPI version 2
 */
wdphysaddrreq(queue_t *wq, mblk_t *mp)
{
	struct wddev   *wd;
	struct wdparam *wdp;
	union DL_primitives *dlp;
	int type;
	struct ether_addr addr;

	wd = (struct wddev *)wq->q_ptr;

	if (MBLKL(mp) < DL_PHYS_ADDR_REQ_SIZE) {
		dlerrorack(wq, mp, DL_PHYS_ADDR_REQ, DL_BADPRIM, 0);
		return (WDE_OK);
	}
	dlp = (union DL_primitives *)mp->b_rptr;
	type = dlp->physaddr_req.dl_addr_type;

	if ((wdp = wd->wd_macpar) == NULL) {
		dlerrorack(wq, mp, DL_PHYS_ADDR_REQ, DL_OUTSTATE, 0);
		return (WDE_OK);
	}
	/*
	 * user can request either the board's default address or what it is
	 * currently set to.  These two may or may not be the same
	 */
	switch (type) {
	case DL_FACT_PHYS_ADDR:
		mutex_enter(&wdp->wd_intrlock);
		get_node_addr(wdp->wd_ioaddr, (u_char *)&addr);
		mutex_exit(&wdp->wd_intrlock);
		break;

	case DL_CURR_PHYS_ADDR:
		BCOPY(wdp->wd_macaddr, &addr, ETHERADDRL);
		break;

	default:
		dlerrorack(wq, mp, DL_PHYS_ADDR_REQ,
			DL_NOTSUPPORTED, 0);
		return (WDE_OK);
	}

	dlphysaddrack(wq, mp, (caddr_t)&addr, ETHERADDRL);
	return (WDE_OK);
}


/*
 * wdsetphysaddrreq sets the physical value for all streams for that provider
 * for a particular PPA NOTE: this primitive is optional for DLPI version 2
 */
wdsetphysaddrreq(queue_t *wq, mblk_t *mp)
{
	struct wddev		*wd;
	struct wdparam		*wdp;
	union DL_primitives	*dlp;
	int			off, len, inval, i;
	struct ether_addr	*addrp;
	short			cmd_reg;

	wd = (struct wddev *)wq->q_ptr;

	if (MBLKL(mp) < DL_SET_PHYS_ADDR_REQ_SIZE) {
		dlerrorack(wq, mp, DL_SET_PHYS_ADDR_REQ, DL_BADPRIM, 0);
		return (WDE_OK);
	}
	dlp = (union DL_primitives *)mp->b_rptr;
	len = dlp->set_physaddr_req.dl_addr_length;
	off = dlp->set_physaddr_req.dl_addr_offset;
	if ((off < 0) || (len < 0)) {
		dlerrorack(wq, mp, DL_SET_PHYS_ADDR_REQ, DL_BADDATA, 0);
		return (WDE_OK);
	}

	if (!MBLKIN(mp, off, len)) {
		dlerrorack(wq, mp, DL_SET_PHYS_ADDR_REQ, DL_BADPRIM, 0);
		return (WDE_OK);
	}
	addrp = (struct ether_addr *)(mp->b_rptr + off);

	/*
	 * Error if length of address isn't right or the address specified is
	 * a multicast or broadcast address.
	 */
	if ((len != ETHERADDRL) ||
	    ((addrp->ether_addr_octet[0] & 01) == 1) ||
	    (ether_cmp(addrp, wdbroadaddr) == 0)) {
		dlerrorack(wq, mp, DL_SET_PHYS_ADDR_REQ, DL_BADADDR, 0);
		return (WDE_OK);
	}
	/*
	 * Error if this stream is not attached to a device.
	 */
	if ((wdp = wd->wd_macpar) == NULL) {
		dlerrorack(wq, mp, DL_SET_PHYS_ADDR_REQ, DL_OUTSTATE, 0);
		return (WDE_OK);
	}
	/*
	 * Set new interface local address and re-init device. This is
	 * destructive to any other streams attached to this device.
	 */
	mutex_enter(&wdp->wd_intrlock);
	BCOPY(addrp, wdp->wd_macaddr, LLC_ADDR_LEN);
	cmd_reg = wdp->wd_ioaddr + 0x10;

	/* also tell 8390 of new address */
	inval = inb(cmd_reg);
	outb(cmd_reg, (inval & PG_MSK & ~TXP) | PAGE_1);
	for (i = 0; i < LLC_ADDR_LEN; i++)
		outb(cmd_reg + PAR0 + i, wdp->wd_macaddr[i]);
	outb(cmd_reg, inval & ~TXP);
	mutex_exit(&wdp->wd_intrlock);

	dlokack(wq, mp, DL_SET_PHYS_ADDR_REQ);
	return (WDE_OK);
}

/*
 * wd_addmulti program the board for the new multicast address map the
 * address to the range [0,63] and set the corresponding multicast register
 * filter bit
 */
void
wd_addmulti(struct wdparam *wdp, struct wdmaddr *maddrp)
{
	int	row, col;
	u_char	val;
	short	cmd_reg;

	cmd_reg = wdp->wd_ioaddr + 0x10;
	maddrp->filterbit = wdhash(maddrp->entry);
	row = maddrp->filterbit / 8;
	col = maddrp->filterbit % 8;

	outb(cmd_reg, PAGE_1);
	val = inb(cmd_reg + MAR0 + row);
	val |= 0x01 << col;
	outb(cmd_reg + MAR0 + row, val);
	outb(cmd_reg, PAGE_0);

	/* set the count of streams with this multicast */
	maddrp->count = 1;
}


/*
 * wd_delmulti delete this mutlicast from the board
 */
void
wd_delmulti(struct wdparam *wdp, struct wdmaddr *maddrp)
{
	int	row, col;
	u_char	val, *cp;
	short	cmd_reg;

	/*
	 * delete the entry from the board
	 */
	val = maddrp->filterbit;
	cmd_reg = wdp->wd_ioaddr + 0x10;
	outb(cmd_reg, PAGE_1);
	row = val / 8;
	col = val % 8;
	val = inb(cmd_reg + MAR0 + row);
	val &= ~(0x01 << col);
	outb(cmd_reg + MAR0 + row, val);
	outb(cmd_reg, PAGE_0);

	/*
	 * clear out the entry in the device table
	 */
	maddrp->filterbit = 0xff;
	cp = maddrp->entry;
	maddrp->count = 0;
	for (row = 0; row < ETHERADDRL; row++)
		cp[row] = 0;
	wdp->wd_multicnt--;
}


/*
 * wdflush_multi remove all multicasts set for this stream, deleting from the
 * board any that are only used by this stream
 */
void
wdflush_multi(struct wddev *wd)
{
	struct wdparam *wdp;
	register int    i;
	struct wdmaddr *maddrp, *board_maddrp;

	/* Are there any multicasts set for this stream? */
	if (wd && wd->wd_mcount > 0 && wd->wd_macpar) {
		wdp = wd->wd_macpar;
		maddrp = wd->wd_maddrs;

		/*
		 * loop through all of the multicasts for this stream
		 */
		for (i = 0; i < wd_multisize; i++, maddrp++) {
			/*
			 * if this queue's multicast entry is good and is to
			 * only reference to this multicast for the board,
			 * delete it from the board
			 */
			if (maddrp->entry[0] &&
			    ((board_maddrp = wdmulticast(
			    (u_char *)maddrp->entry,
			    wdp->wd_multiaddrs, wdp->wd_multip,
			    wdp->wd_multicnt)) != NULL) &&
			    board_maddrp->count <= 1) {
				wd_delmulti(wdp, board_maddrp);
			}
		}
	}
	bzero(wd->wd_maddrs, sizeof (struct wdmaddr) * MAXMULTI);
	wd->wd_mcount = 0;
}


static unsigned char *
eaddrstr(unsigned char *s)
{
	static unsigned char fmtbuf[3 * LLC_ADDR_LEN + 1];
	register unsigned char *fp = fmtbuf;
	int i;
	u_int	val, highval;

	for (i = 0; i < LLC_ADDR_LEN; i++) {
		val = *s++;
		highval = (val & 0xf0) >> 4;
		val &= 0xf;

		if (highval > 9)
			*fp++ = highval - 10 + 'a';
		else
			*fp++ = highval + '0';

		if (val > 9)
			*fp++ = val - 10 + 'a';
		else
			*fp++ = val + '0';

		*fp++ = ' ';
	}
	*fp = '\0';

	return (fmtbuf);
}

/*
 * this stores the LAN address from the adapter board into the char string if
 * the first three bytes do not reflect that of a SMC adapter, it will return
 * an error
 */
void
get_node_addr(int address, unsigned char *char_ptr)
{
	/*
	 * Always return the LAN address and do not check for WD node address
	 */
	*char_ptr++ = inb(address + LAN_ADDR_0);
	*char_ptr++ = inb(address + LAN_ADDR_1);
	*char_ptr++ = inb(address + LAN_ADDR_2);
	*char_ptr++ = inb(address + LAN_ADDR_3);
	*char_ptr++ = inb(address + LAN_ADDR_4);
	*char_ptr++ = inb(address + LAN_ADDR_5);
}

/*
 * this verifies the address ROM checksum
 */
int
check_addr_ROM(int address)
{
	register int    i;
	unsigned char   sum = 0;

	for (i = 0; i < 8; i++)
		sum += inb(address + LAN_ADDR_0 + i);
	if (sum != (unsigned char)0xFF)
		return (1);
	else
		return (0);
}

/*
 * The following is used to determine if the code is running on a Micro
 * Channel machine. Returns 1 on a Micro Channel machine, 0 otherwise.
 */
int
micro_channel(dev_info_t *devi)
{
	static int	return_val;
	static int	not_first_call = 0;
	char		bus_type[16];
	int		len = sizeof (bus_type);

	if (not_first_call)
		return (return_val);

	if (ddi_prop_op(DDI_DEV_T_ANY, devi, PROP_LEN_AND_VAL_BUF, 0,
	    "bus-type", (caddr_t)&bus_type[0], &len) != DDI_PROP_SUCCESS) {
			printf("smc: failed to get bus-type\n");
			return (0);
	}
	if (strcmp(bus_type, DEVI_MCA_NEXNAME))
		return_val = 0;
	else
		return_val = 1;
	not_first_call = 1;
	return (return_val);
}

static void
printcfg(char *board_name, u_char *ether_addr)
{
	cmn_err(CE_CONT, "?SMC WD8003/WD8013 driver: type=%s addr=%s\n",
		board_name, ether_addr);
}

/*
 * DEBUG Be cautious of using this routine. It calls cnputc() directly
 * without any locks.
 */
void
smc_print(char *s)
{
	if (!s)
		return;		/* sanity check for s == 0 */
	while (*s)
		cnputc(*s++, 0);
}

wdstat_kstat_update(kstat_t *ksp, int rw)
{
	struct wdstat *stats;
	struct wdkstat *wdsp;

	if (rw == KSTAT_WRITE)
		return (EACCES);

	stats = (struct wdstat *)ksp->ks_private;
	wdsp = (struct wdkstat *)ksp->ks_data;

	wdsp->wd_ipackets.value.ul	= stats->wds_rpkts;
	wdsp->wd_ierrors.value.ul	= stats->wds_ierrs;
	wdsp->wd_opackets.value.ul	= stats->wds_xpkts;
	wdsp->wd_oerrors.value.ul	= stats->wds_excoll;
	wdsp->wd_collisions.value.ul	= stats->wds_coll;
	wdsp->wd_nobuffer.value.ul	= stats->wds_nobuffer;
	wdsp->wd_blocked.value.ul	= stats->wds_blocked;
	wdsp->wd_blocked2.value.ul	= stats->wds_blocked2;
	wdsp->wd_multicast.value.ul	= stats->wds_multicast;
	wdsp->wd_xbytes.value.ul	= stats->wds_xbytes;
	wdsp->wd_rbytes.value.ul	= stats->wds_rbytes;
	wdsp->wd_crc.value.ul		= stats->wds_crc;
	wdsp->wd_align.value.ul		= stats->wds_align;
	wdsp->wd_fifoover.value.ul	= stats->wds_fifoover;
	wdsp->wd_lost.value.ul		= stats->wds_lost;
	wdsp->wd_intrs.value.ul		= stats->wds_intrs;
	wdsp->wd_ovw.value.ul		= stats->wds_ovw;
	wdsp->wd_dog.value.ul		= stats->wds_dog;

	return (0);
}

void
wdstatinit(struct wdparam *wdp)
{
	kstat_t *ksp;
	struct wdkstat *wdsp;		/* wd stat pointer */

	if ((ksp = kstat_create("smc", wdp->wd_noboard - 1,
	    NULL, "net", KSTAT_TYPE_NAMED,
	    sizeof (struct wdkstat) / sizeof (kstat_named_t),
				0)) == NULL) {
		return;
	}

	ksp->ks_update = wdstat_kstat_update;
	ksp->ks_private = (void *)&wdstats[wdp->wd_noboard - 1];
	wdsp = (struct wdkstat *)(ksp->ks_data);

	kstat_named_init(&wdsp->wd_ipackets,		"ipackets",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_ierrors,		"ierrors",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_opackets,		"opackets",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_oerrors,		"oerrors",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_collisions,		"collisions",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_nobuffer,		"nobuffer",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_blocked,		"blocked",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_blocked2,		"blocked2",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_multicast,		"multicast",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_xbytes,		"xbytes",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_rbytes,		"rbytes",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_crc,		"crc",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_align,		"alignment",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_fifoover,		"fifoover",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_lost,		"lost",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_intrs,		"intrs",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_ovw,		"ovw",
				KSTAT_DATA_ULONG);
	kstat_named_init(&wdsp->wd_dog,		"watchdog",
				KSTAT_DATA_ULONG);
	kstat_install(ksp);
}

/*
 * smc_get_irq() - part of the autoconfiguration process for boards that have
 * an interface chip.
 */
smc_get_irq(int adr, int type)
{
	static u_char IrqBrd8[]  = {0, 0x2, 0x3, 0x4, 0x5, 0x6, 0, 0};
	static u_char IrqBrd16[] = {0, 0x2, 0x3, 0x5, 0x7, 0xA, 0xB, 0xF};
	int adjust, irq, irr;

	/* Check and see if we need to adjust the irq levels */
	adjust = 0;
	if ((type & INTERFACE_CHIP_MASK) == INTERFACE_585_CHIP) {
		unsigned char	reg;

		reg = inb(adr + WD_REG_4);
		reg &= 0xc3;
		outb(adr + WD_REG_4, reg | HWR_SWH);

		adjust = inb(adr + 0x0d);
		irr = adjust;
		adjust &= 0x0c;
		adjust >>= 2;
		irr &= 0x40;
		irr >>= 4;
		adjust |= irr;
		outb(adr + WD_REG_4, reg);
		irq = (type & BOARD_16BIT) ? IrqBrd16[adjust] : IrqBrd8[adjust];
		return (irq);
	}

	if ((type & INTERFACE_CHIP_MASK) != INTERFACE_5X3_CHIP) {
		adjust = inb(adr + CNFG_ICR_583);
		adjust &= CNFG_ICR_IR2_584;
	}

	/* Get the current irr before mapping */
	irr = (int)(inb(adr + IRR) & (IRR_IR0 | IRR_IR1)) >> 5;

	switch (irr) {
	case 0:
		irq = adjust ? 10 : 2;
		break;

	case 1:
		irq = adjust ? 11 : 3;
		break;

	case 2:
		irq = adjust ? 15 : (type & ALTERNATE_IRQ_BIT) ? 5 : 4;
		break;

	case 3:
		irq = adjust ? 4 : 7;
		break;

	default:
		/* keep compiler happy on reference before set checks */
		irq = -1;
		cmn_err(CE_PANIC, "SMC: Invalid irr value %d\n", irr);
		break;
	}

	return (irq);
}

/*
 * smc_get_base - this routine gets the base ram address for the card during
 * autoconfiguration. NOTE: This routine was converted from an assembly
 * language routine without any documentation. The variables reg_a, reg_b and
 * reg_d correlate to regs a[hl], b[hl] and d[hl] in the intel world. Rick
 * McNeal 1-Sep-1992
 */
caddr_t
smc_get_base(int adr, int type)
{
	int	reg_a, reg_b, reg_d;
	u_int	base;

	reg_a = inb(adr) & 0x3f;
	if ((type & INTERFACE_CHIP_MASK) == INTERFACE_5X3_CHIP) {
		reg_a |= 0x40;
		return ((caddr_t)(reg_a << 13));
	} else {
		if ((type & INTERFACE_CHIP_MASK) == INTERFACE_585_CHIP) {
			unsigned char reg;

			reg = inb(adr + WD_REG_4);
			reg &= 0xc3;
			outb(adr + WD_REG_4, reg | HWR_SWH);
			reg_a = inb(adr + 0x0b);

			reg_a &= 0x30;	/* get windowsize */
			reg_a >>= 4;	/* 0,1,2,3 */
			reg_b = 0x08;
			reg_b <<= reg_a;

			reg_a = inb(adr + 0x0b);
			reg_d = reg_a & 0x0f;	/* get RA16-13 */
			reg_d |= ((reg_a & 0x40) >> 2 | 0x60);
			base = (reg_d << 13);

			if (reg_a & 0x80)
				base |= 0x0f00000;
			outb(adr + WD_REG_4, reg);
			return ((caddr_t)base);
		} else {    /* ram_base_584 */
			reg_d = inb(adr + CNFG_LAAR_584);
			reg_d &= CNFG_LAAR_MASK;
			reg_d <<= 3;
			reg_d |= ((reg_a & 0x38) >> 3);

			return ((caddr_t)((reg_d << 16) +
				((reg_a & 0x7) << 13)));
		}
	}
}

/*
 *  get_setup() - get the setup information for WD board in
 * 	slot number "slot_num".  Returns POS register information
 *	in pos_regs array.
 *
 */
static void
get_setup(int slot_num, u_char *pos_regs)
{
	register int i;
	char select;
	unsigned pos_addr;

	select = SETUP + slot_num - 1;
	outb(ADAP_ENAB, select);

	pos_addr = POS_0;
	for (i = 0; i < 8; i++) {
		pos_regs[i] = inb(pos_addr);
		pos_addr++;
	}

	outb(ADAP_ENAB, DISSETUP);
}
