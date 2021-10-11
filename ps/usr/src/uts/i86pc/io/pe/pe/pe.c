/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)pe.c	1.2	96/03/26 SMI"

/*
 * pe -- Xircom PE
 * Depends on the Generic LAN Driver utility functions in /kernel/misc/gld
 */

/*
 * This file is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this file without charge, but are not authorized to
 * license or distribute it to anyone else except as part of a product
 * or program developed by the user.
 * 
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * This file is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even
 * if Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#ifndef lint
static char     sccsid[] = "@(#)gldconfig 1.1 93/02/12 Copyright 1993 Sun Microsystems";
#endif

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/devops.h>
#include <sys/sunddi.h>
#include <sys/ksynch.h>
#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <sys/strsun.h>
#include <sys/stat.h>
#include <sys/modctl.h>
#include <sys/gld.h>
#include "pe.h"

/*
 * Xircom CDDK Required Variables
 */

int PE2_Media_Copy_Packet();
int PE2_Media_Send();
int PE2_Media_Poll();
int PE2_Media_ISR();
int PE2_Get_Physical_Address();
int PE2_Set_Physical_Address();
int PE2_Media_Unhook();
int PE2_Set_Receive_Mode();
int PE2_MCast_Change_Address();

int PE3_Media_Copy_Packet();
int PE3_Media_Send();
int PE3_Media_Poll();
int PE3_Media_ISR();
int PE3_Get_Physical_Address();
int PE3_Set_Physical_Address();
int PE3_Media_Unhook();
int PE3_Set_Receive_Mode();
int PE3_MCast_Change_Address();



static unsigned char Node_Addr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct Fragment_Descriptor {
	char	*Fragment_Pointer;
	int	Fragment_Length;
};

struct Media_Initialize_Params {
	int Media_IO_Address;
	int Media_IRQ;
	char *Send_Header;
	char *Receive_Header;
	int Send_Header_Size;
	int Receive_Header_Size;
	void (*User_Service_Routine)();
	unsigned char *Node_Address;
	void *Link_Pointer;
};
	

int Allow_EPP = 0;

/*
 *  Declarations and Module Linkage
 */



static char ident[] = "Xircom PE";

#ifdef PEDEBUG
/* used for debugging */
int	pedebug = 0;
#endif

/* Required system entry points */
static	peidentify(dev_info_t *);
static	pedevinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	peprobe(dev_info_t *);
static	peattach(dev_info_t *, ddi_attach_cmd_t);
static	pedetach(dev_info_t *, ddi_detach_cmd_t);

/* Required driver entry points for GLD */
int	pe_reset(gld_mac_info_t *);
int	pe_start_board(gld_mac_info_t *);
int	pe_stop_board(gld_mac_info_t *);
int	pe_saddr(gld_mac_info_t *);
int	pe_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
int	pe_prom(gld_mac_info_t *, int);
int	pe_gstat(gld_mac_info_t *);
int	pe_send(gld_mac_info_t *, mblk_t *);
u_int	peintr(gld_mac_info_t *);

DEPENDS_ON_GLD;		/* this forces misc/gld to load -- DO NOT REMOVE */

/* Standard Streams initialization */

static struct module_info minfo = {
	PEIDNUM, "pe", 0, INFPSZ, PEHIWAT, PELOWAT
};

static struct qinit rinit = {	/* read queues */
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
};

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, NULL, NULL, NULL, &minfo, NULL
};

struct streamtab peinfo = {&rinit, &winit, NULL, NULL};

/* Standard Module linkage initialization for a Streams driver */

extern struct mod_ops mod_driverops;

static 	struct cb_ops cb_peops = {
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
	&peinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

struct dev_ops peops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	pedevinfo,		/* devo_getinfo */
	peidentify,		/* devo_identify */
	peprobe,		/* devo_probe */
	peattach,		/* devo_attach */
	pedetach,		/* devo_detach */
	nodev,			/* devo_reset */
	&cb_peops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,			/* short description */
	&peops			/* driver specific ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

int
_init(void)
{
	return mod_install(&modlinkage);
}

int
_fini(void)
{
	return mod_remove(&modlinkage);
}

int
_info(struct modinfo *modinfop)
{
	return mod_info(&modlinkage, modinfop);
}

/*
 *  DDI Entry Points
 */

/* identify(9E) -- See if we know about this device */

peidentify(dev_info_t *devinfo)
{
	if (strcmp(ddi_get_name(devinfo), "pe") == 0)
		return DDI_IDENTIFIED;
	else
		return DDI_NOT_IDENTIFIED;
}

/* getinfo(9E) -- Get device driver information */

pedevinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int error;

	/* This code is not DDI compliant: the correct semantics */
	/* for CLONE devices is not well-defined yet.            */
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
	return error;
}

/* probe(9E) -- Determine if a device is present */

peprobe(dev_info_t *devinfo)
{
	int portnum, handle, rval;
	caddr_t devaddr = NULL;
	int found_board = 0;
	void (*User_Service_Routine)();
	unsigned char *Node_Address;
	char *String;
	struct Media_Initialize_Params mip;


#ifdef PEDEBUG
	if (pedebug & PEDDI)
		cmn_err(CE_CONT, "peprobe(0x%x)", devinfo);
#endif

	/* Get the port number from the ioaddr property */
	portnum = ddi_getprop(DDI_DEV_T_ANY, devinfo,
			DDI_PROP_DONTPASS, "ioaddr", 0);
	if (!portnum) {
		cmn_err(CE_WARN, "peprobe(0x%x): no ioaddr property",
					devinfo);
		return DDI_PROBE_FAILURE;
	}

	/* Map in the device memory so we can look at it */
	if (ddi_map_regs(devinfo, 0, &devaddr, 0, 0) != 0)
		devaddr = NULL;

	/*
	 *  Probe for the board to see if it's there
	 */


	mip.User_Service_Routine = 0;
	mip.Node_Address = Node_Addr;
	mip.Media_IO_Address = portnum;
	mip.Media_IRQ = 7;
	mip.Send_Header = 0;
	mip.Receive_Header = 0;
	mip.Send_Header_Size = 0;
	mip.Receive_Header_Size = 14;
	mip.Link_Pointer = 0;

	rval = PE3_Media_Initialize(&handle, &String, &mip);

	if (rval) {
		rval = PE2_Media_Initialize(&handle, &String, &mip);
		if (!rval)
			PE2_Media_Unhook(handle);
	} else {
		PE3_Media_Unhook(handle);
	}
		
	if (!rval)
		found_board++;

#ifdef PEDEBUG
	if (!rval) {
		printf("peprobe, port (%d), err (%d): %s\n", 
			portnum, rval, String);
		cmn_err(CE_CONT, "peprobe: %s\n", String);
	}
#endif

#ifdef PEDEBUG
	if (pedebug & PEDDI) {
		printf("peprobe, port (%d), err (%d): %s\n", 
			portnum, rval, String);
		cmn_err(CE_CONT, "peprobe: %s\n", String);
	}
#endif

	/* Unmap the device memory */
	if (devaddr)
		ddi_unmap_regs(devinfo, 0, &devaddr, 0, 0);

	/*
	 *  Return whether the board was found.  If unable to determine
	 *  whether the board is present, return DDI_PROBE_DONTCARE.
	 */
	if (found_board)
	   	return DDI_PROBE_SUCCESS;
	else
		return DDI_PROBE_FAILURE;
}

/*
 *  attach(9E) -- Attach a device to the system
 *
 *  Called once for each board successfully probed.
 */

peattach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */
	struct peinstance *pep;		/* Our private device info */
	void (*User_Service_Routine)();
	int rval, handle;
	char *String;
	unsigned char *Node_Address;
	struct Fragment_Descriptor Frag;
	struct Media_Initialize_Params mip;

#ifdef PEDEBUG
	if (pedebug & PEDDI)
		cmn_err(CE_CONT, "peattach(0x%x)\n", devinfo);
#endif

	if (cmd != DDI_ATTACH)
		return DDI_FAILURE;

	/*
	 *  Allocate gld_mac_info_t and peinstance structures
	 */
	macinfo = (gld_mac_info_t *)kmem_zalloc(
			sizeof(gld_mac_info_t)+sizeof(struct peinstance),
			KM_NOSLEEP);
	if (macinfo == NULL)
		return DDI_FAILURE;
	pep = (struct peinstance *)(macinfo+1);

	/*  Initialize our private fields in macinfo and peinstance */
	macinfo->gldm_private = (caddr_t)pep;
	macinfo->gldm_port = ddi_getprop(DDI_DEV_T_ANY, devinfo,
			DDI_PROP_DONTPASS, "ioaddr", 0);
	macinfo->gldm_state = PE_IDLE;
	macinfo->gldm_flags = 0;
	pep->pe_random = 0;

	mip.User_Service_Routine = 0;
	mip.Node_Address = Node_Addr;
	mip.Media_IO_Address = macinfo->gldm_port;
	mip.Media_IRQ = 7;

	pep->pe_Send_Header = mip.Send_Header = 0;
	pep->pe_Receive_Header = mip.Receive_Header = pep->pe_rbuf;
	pep->pe_Send_Header_Size = mip.Send_Header_Size = 0;
	pep->pe_Receive_Header_Size = 
		mip.Receive_Header_Size = sizeof(pep->pe_rbuf);

	mip.Link_Pointer = macinfo;

	rval = PE2_Media_Initialize(&handle, &String, &mip);
	if (!rval) {
		pep->Media_Copy_Packet=PE2_Media_Copy_Packet;
		pep->Media_Send=PE2_Media_Send;
		pep->Media_Poll=PE2_Media_Poll;
		pep->Media_ISR=PE2_Media_ISR;
		pep->MCast_Change_Address=PE2_MCast_Change_Address;
		pep->Get_Physical_Address=PE2_Get_Physical_Address;
		pep->Set_Physical_Address=PE2_Set_Physical_Address;
		pep->Media_Unhook=PE2_Media_Unhook;
		pep->Set_Receive_Mode=PE2_Set_Receive_Mode;
		pep->pe_handle = handle;
	} else {
		rval = PE3_Media_Initialize(&handle, &String, &mip);

		pep->Media_Copy_Packet=PE3_Media_Copy_Packet;
		pep->Media_Send=PE3_Media_Send;
		pep->Media_Poll=PE3_Media_Poll;
		pep->Media_ISR=PE3_Media_ISR;
		pep->MCast_Change_Address=PE3_MCast_Change_Address;
		pep->Get_Physical_Address=PE3_Get_Physical_Address;
		pep->Set_Physical_Address=PE3_Set_Physical_Address;
		pep->Media_Unhook=PE3_Media_Unhook;
		pep->Set_Receive_Mode=PE3_Set_Receive_Mode;
		pep->pe_handle = handle;
	}

	pep->pe_initialized = 1;


	/*
	 *  Initialize pointers to device specific functions which will be
	 *  used by the generic layer.
	 */

	macinfo->gldm_reset   = pe_reset;
	macinfo->gldm_start   = pe_start_board;
	macinfo->gldm_stop    = pe_stop_board;
	macinfo->gldm_saddr   = pe_saddr;
	macinfo->gldm_sdmulti = pe_dlsdmult;
	macinfo->gldm_prom    = pe_prom;
	macinfo->gldm_gstat   = pe_gstat;
	macinfo->gldm_send    = pe_send;
	macinfo->gldm_intr    = peintr;
	macinfo->gldm_ioctl   = NULL;    /* if you have one, NULL otherwise */

	/*
	 *  Initialize board characteristics needed by the generic layer.
	 */

	/***** Adjust the following values as necessary  *****/
	macinfo->gldm_ident = ident;
	macinfo->gldm_type = DL_ETHER;
	macinfo->gldm_minpkt = 0;		/* assumes we pad ourselves */
	macinfo->gldm_maxpkt = PEMAXPKT;
	macinfo->gldm_addrlen = ETHERADDRL;
	macinfo->gldm_saplen = -2;

	/*
	 *  Do anything necessary to prepare the board for operation
	 *  short of actually starting the board.
	 */

	pe_init_board(macinfo);

	/* Get the board's vendor-assigned hardware network address */
	{
		/***** Read the board and set its address into the *****/
		/***** unsigned char gldm_vendor[ETHERADDRL] array *****/
		pep->Get_Physical_Address(pep->pe_handle, macinfo->gldm_vendor);
	}

	/***** set the connector/media type if it can be determined *****/
	macinfo->gldm_media = GLDM_UNKNOWN;

	bcopy((caddr_t)gldbroadcastaddr,
		(caddr_t)macinfo->gldm_broadcast, ETHERADDRL);
	bcopy((caddr_t)macinfo->gldm_vendor,
		(caddr_t)macinfo->gldm_macaddr, ETHERADDRL);

	/* Make sure we have our address set */
	pe_saddr(macinfo);

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
	
	if (gld_register(devinfo, "pe", macinfo) == DDI_SUCCESS)
		return DDI_SUCCESS;
	else {
		kmem_free((caddr_t)macinfo,
			sizeof(gld_mac_info_t)+sizeof(struct peinstance));
		return DDI_FAILURE;
	}
}

/*  detach(9E) -- Detach a device from the system */

pedetach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */
	struct peinstance *pep;		/* Our private device info */

#ifdef PEDEBUG
	if (pedebug & PEDDI)
		cmn_err(CE_CONT, "pedetach(0x%x)", devinfo);
#endif

	if (cmd != DDI_DETACH) {
		return DDI_FAILURE;
	}

	/* Get the driver private (gld_mac_info_t) structure */
	macinfo = (gld_mac_info_t *)ddi_get_driver_private(devinfo);
	pep = (struct peinstance *)(macinfo->gldm_private);

	/* stop the board if it is running */
	(void)pe_stop_board(macinfo);
	Hardware_Disable_Int();

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
			sizeof(gld_mac_info_t)+sizeof(struct peinstance));
		return DDI_SUCCESS;
	}
	return DDI_FAILURE;
}

/*
 *  GLD Entry Points
 */

/*
 *  pe_reset() -- reset the board to initial state; restore the machine
 *  address afterwards.
 */

int
pe_reset(gld_mac_info_t *macinfo)
{
#ifdef PEDEBUG
	if (pedebug & PETRACE)
		cmn_err(CE_CONT, "pe_reset(0x%x)", macinfo);
#endif

	(void)pe_stop_board(macinfo);
	(void)pe_init_board(macinfo);
	(void)pe_saddr(macinfo);
}

/*
 *  pe_init_board() -- initialize the specified network board.
 */

int
pe_init_board(gld_mac_info_t *macinfo)
{
	struct peinstance *pep =		/* Our private device info */
		(struct peinstance *)macinfo->gldm_private;

	/***** do whatever is necessary to initialize the hardware *****/
}

/*
 *  pe_start_board() -- start the board receiving and allow transmits.
 */

pe_start_board(gld_mac_info_t *macinfo)
{
	struct peinstance *pep =		/* Our private device info */
		(struct peinstance *)macinfo->gldm_private;

#ifdef PEDEBUG
	if (pedebug & PETRACE)
		cmn_err(CE_CONT, "pe_start_board(0x%x)", macinfo);
#endif

	pep->pe_receive_mode = PE_RMODE_ABP | PE_RMODE_ADP;
	pep->Set_Receive_Mode(pep->pe_handle, pep->pe_receive_mode);

}

/*
 *  pe_stop_board() -- stop board receiving
 */

pe_stop_board(gld_mac_info_t *macinfo)
{
	struct peinstance *pep =		/* Our private device info */
		(struct peinstance *)macinfo->gldm_private;

#ifdef PEDEBUG
	if (pedebug & PETRACE)
		cmn_err(CE_CONT, "pe_stop_board(0x%x)", macinfo);
#endif

	/***** stop the board and disable receiving *****/
	pep->pe_receive_mode = 0;
	pep->Set_Receive_Mode(pep->pe_handle, pep->pe_receive_mode);

}

/*
 *  pe_saddr() -- set the physical network address on the board
 */

int
pe_saddr(gld_mac_info_t *macinfo)
{
	struct peinstance *pep =		/* Our private device info */
		(struct peinstance *)macinfo->gldm_private;

#ifdef PEDEBUG
	if (pedebug & PETRACE)
		cmn_err(CE_CONT, "pe_saddr(0x%x)", macinfo);
#endif

	/***** program current gldm_macaddr address into the hardware *****/
	pep->Set_Physical_Address(pep->pe_handle, macinfo->gldm_macaddr);

}

/*
 *  pe_dlsdmult() -- set (enable) or disable a multicast address
 *
 *  Program the hardware to enable/disable the multicast address
 *  in "mcast".  Enable if "op" is non-zero, disable if zero.
 */

int
pe_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	struct peinstance *pep =		/* Our private device info */
		(struct peinstance *)macinfo->gldm_private;

#ifdef PEDEBUG
	if (pedebug & PETRACE)
		cmn_err(CE_CONT, "pe_dlsdmult(0x%x, %s)", macinfo,
				op ? "ON" : "OFF");
#endif
	/***** enable or disable the multicast *****/
	pep->MCast_Change_Address(pep->pe_handle, op, (char *) mcast);
}

/*
 * pe_prom() -- set or reset promiscuous mode on the board
 *
 *  Program the hardware to enable/disable promiscuous mode.
 *  Enable if "on" is non-zero, disable if zero.
 */

int
pe_prom(gld_mac_info_t *macinfo, int on)
{
	struct peinstance *pep =		/* Our private device info */
		(struct peinstance *)macinfo->gldm_private;

#ifdef PEDEBUG
	if (pedebug & PETRACE)
		cmn_err(CE_CONT, "pe_prom(0x%x, %s)", macinfo,
				on ? "ON" : "OFF");
#endif

	if (on)
		pep->pe_receive_mode |= PE_RMODE_PRO;
	else
		pep->pe_receive_mode &= ~PE_RMODE_PRO;

	pep->Set_Receive_Mode(pep->pe_handle, pep->pe_receive_mode);

}

/*
 * pe_gstat() -- update statistics
 *
 *  GLD calls this routine just before it reads the driver's statistics
 *  structure.  If your board maintains statistics, this is the time to
 *  read them in and update the values in the structure.  If the driver
 *  maintains statistics continuously, this routine need do nothing.
 */

int
pe_gstat(gld_mac_info_t *macinfo)
{
	struct peinstance *pep =		/* Our private device info */
		(struct peinstance *)macinfo->gldm_private;

#ifdef PEDEBUG
	if (pedebug & PETRACE)
		cmn_err(CE_CONT, "pe_gstat(0x%x)", macinfo);
#endif

	/***** update statistics from board if necessary *****/

}

/*
 *  pe_send() -- send a packet
 *
 *  Called when a packet is ready to be transmitted. A pointer to an
 *  M_DATA message that contains the packet is passed to this routine.
 *  The complete LLC header is contained in the message's first message
 *  block, and the remainder of the packet is contained within
 *  additional M_DATA message blocks linked to the first message block.
 *
 *  This routine may NOT free the packet.
 */

int
pe_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	register int len = 0;
	register int frags = 0;
	register unsigned length;
	unsigned char *txbuf;
	mblk_t *omp = mp;
	struct Fragment_Descriptor frag[32];
	struct peinstance *pep =		/* Our private device info */
			(struct peinstance *)macinfo->gldm_private;

#ifdef PEDEBUG
	if (pedebug & PESEND)
		cmn_err(CE_CONT, "pe_send(0x%x, 0x%x)", macinfo, mp);
#endif


	/***** If the board is too busy to write,		*****/
	/***** 		macinfo->gldm_stats.glds_defer++;	*****/
	/*****		return(1)				*****/
	/***** GLD will retry later *****/

	/*
	 *  Load the packet onto the board by chaining through the M_DATA
	 *  blocks of the STREAMS message.  The list of data messages
	 *  ends when the pointer to the current message block is NULL.
	 *
	 *  Note that if the mblock is going to have to * stay around, it
	 *  must be dupmsg() since the caller is going to freemsg() the
	 *  message.
	 */

	/***** This EXAMPLE code writes to the board's memory *****/
	/***** The programming of your board is likely to be different *****/
	length = 0;
	frags = 32;
	do {
		len = (int)(mp->b_wptr - mp->b_rptr);
		/*bcopy((caddr_t)mp->b_rptr, (caddr_t)txbuf+length, len);*/
		if (frags < 32) {
			frag[frags].Fragment_Pointer = (char *) mp->b_rptr;
			frag[frags++].Fragment_Length = len;
		}
		length += (unsigned int)len;
		mp = mp->b_cont;
	} while (mp != NULL);

	if (length < 64)	/* pad packet length if needed */
		length = 64;

	if (frags >= 32) { /* too fragmented, alloc a single buff and send it */
		mblk_t *nmp;
		int rval;

		if (!(nmp = allocb(length, BPRI_MED)))
			return 1;

		mp = omp;

		while (mp) {
			int len;

			len = mp->b_wptr - mp->b_rptr;
			if (len > 0) {
				bcopy((caddr_t)mp->b_rptr, (caddr_t)nmp->b_wptr, len);
				nmp->b_wptr += len;
			}
			mp = mp->b_cont;
		}

		frag[0].Fragment_Pointer = (char *) nmp->b_rptr;
		frag[0].Fragment_Length = length;
		rval = pep->Media_Send(pep->pe_handle, length, 
			length, 1, (char *)&frag);
		freeb(nmp);
		if (rval)
			return 1;	/* count a output error */
	} else {
		if (pep->Media_Send(pep->pe_handle, length, 
		    length, frags, (char *)&frag))
			return 1;	/* count a output error */
	}
	return 0;		/* successful transmit attempt */
}

/*
 *  peintr() -- interrupt from board to inform us that a receive or
 *  transmit has completed.
 */

u_int
peintr(gld_mac_info_t *macinfo)
{
	struct peinstance *pep =		/* Our private device info */
			(struct peinstance *)macinfo->gldm_private;

	if (!pep->pe_initialized)
		return DDI_INTR_UNCLAIMED;

	pep->Media_ISR(pep->pe_handle);

#ifdef notdef
	struct peinstance *pep =		/* Our private device info */
		(struct peinstance *)macinfo->gldm_private;

#ifdef PEDEBUG
	if (pedebug & PEINT)
		cmn_err(CE_CONT, "peintr(0x%x)", macinfo);
#endif

	/***** If interrupt was not from this board,
				return(DDI_INTR_UNCLAIMED) here *****/

	macinfo->gldm_stats.glds_intr++;

	/***** Inform the board that the interrupt has been received *****/
	
	/***** Check for transmit complete interrupt *****/
	
	/***** Check for transmit error *****/
		/***** macinfo->gldm_stats.glds_errxmt++; *****/
		/***** underflow: macinfo->gldm_stats.glds_underflow++; *****/
		/***** no carrer: macinfo->gldm_stats.glds_nocarrier++; *****/
		/***** collisions: macinfo->gldm_stats.glds_collisions++; *****/
		/***** excess coll: macinfo->gldm_stats.glds_excoll++; *****/
		/***** late coll: macinfo->gldm_stats.glds_xmtlatecoll++; *****/

	/***** Check for receive error *****/
		/***** macinfo->gldm_stats.glds_errrcv++; *****/
		/***** overflow: macinfo->gldm_stats.glds_overflow++; *****/
		/***** frame err: macinfo->gldm_stats.glds_frame++; *****/
		/***** CRC err: macinfo->gldm_stats.glds_crc++; *****/
		/***** short pkt: macinfo->gldm_stats.glds_short++; *****/
		/***** missed pkt: macinfo->gldm_stats.glds_missed++; *****/

#ifdef EXAMPLE_CODE

	/***** Check for receive completed *****/
	while (there are still packets on the board) {
		mblk_t *mp;
		caddr_t rp = address of packet on the board;
		int length = length_of_packet_from_card;
		caddr_t dp;

		/* get buffer to put packet in */
		if ((mp = allocb(length, BPRI_MED)) == NULL) {
			/* drop packet */
			macinfo->gldm_stats.glds_norcvbuf++;
		} else {
			dp = (caddr_t)mp->b_wptr;	/* dp is data dest */
			mp->b_wptr = mp->b_rptr + length;
			bcopy(cp, dp, length);		/* copy the packet */
		}

		/***** tell card we're done with the packet *****/

		/*
		 *  Use gld_recv(macinfo, mp) to process each packet
		 *  pulled from the board.
		 */
#ifdef PEDEBUG
		if (pedebug & PERECV)
			cmn_err(CE_CONT, "peintr calls gld_recv(0x%x, 0x%x)",
					macinfo, mp);
#endif
		gld_recv(macinfo, mp);
	}

#endif /* EXAMPLE_CODE */
#endif

	return DDI_INTR_CLAIMED;	/* Indicate it was our interrupt */
}

/*
 * Xircom CDDK Required Procedures
 */

void Link_Transmit_Complete(h, Tx_Length, Status, macinfo)
int Tx_Length;
int Status;
gld_mac_info_t *macinfo;
{
	if (Status & 0x10 || Status & 0x40)
		macinfo->gldm_stats.glds_errxmt++;

	if (Status & 0x10) macinfo->gldm_stats.glds_nocarrier++;
	if (Status & 0x04) macinfo->gldm_stats.glds_collisions++;
}

Link_Receive_Packet(h, Count, Status, macinfo)
int Count;
int Status;
gld_mac_info_t *macinfo;
{
	mblk_t * mp;
	struct peinstance *pep =		/* Our private device info */
			(struct peinstance *)macinfo->gldm_private;

	if (Count > 0) {
		if (!(mp = allocb(Count, BPRI_MED)))
			return 1;

		bcopy(pep->pe_Receive_Header,(caddr_t)mp->b_wptr,min(pep->pe_Receive_Header_Size,Count));

		if (Count > pep->pe_Receive_Header_Size) {
			struct Fragment_Descriptor Frag;

			Frag.Fragment_Pointer= (char *)
				mp->b_wptr + pep->pe_Receive_Header_Size;
			Frag.Fragment_Length = Count - pep->pe_Receive_Header_Size;

			pep->Media_Copy_Packet(pep->pe_handle, pep->pe_Receive_Header_Size,	/* Offset */
					Count - pep->pe_Receive_Header_Size, /* Len */
					1,			/* Frags */
					&Frag);			/* Buffer */
		}

		mp->b_wptr = mp->b_rptr + Count;

		gld_recv(macinfo, mp);

#ifdef notdef
	   	p3es->p3es_rpkts++;	/* count a packet Received */
		p3es->p3es_rbytes += Count;
#endif
	}

	return 1;
}

Link_Driver_Shutdown() {}

void Link_Receive_Error(h, FAE_Errs, Missed, CRC_Errs, macinfo)
int h, FAE_Errs, Missed, CRC_Errs;
gld_mac_info_t *macinfo;
{
	/***** Check for receive error *****/

		if (FAE_Errs || Missed || CRC_Errs)
			macinfo->gldm_stats.glds_errrcv++;

		macinfo->gldm_stats.glds_frame += FAE_Errs;
		macinfo->gldm_stats.glds_crc += CRC_Errs;
		macinfo->gldm_stats.glds_missed += Missed;
}

