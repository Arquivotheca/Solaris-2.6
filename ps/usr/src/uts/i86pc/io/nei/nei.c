/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)nei.c	1.9	95/05/16 SMI"

/*
 * nei -- NE2000+ AT/LANTIC
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

/*
 * Modification history
 *
 * ------------------------------------------------------------------------
 * Date		Author			Changes
 * ------------------------------------------------------------------------
 * 270494	Dharam/Prabhu	Modified ne2k code to work with ne2k+
 *				and ne2k.
 *				In ne2kplus, it works in Shared Memory 
 *				a.w.a. I/O mode
 * 070794	Dharam		Ver 2.2 
 *				check_mode() : now mode property is a must.
 *				In case of ne2k, it is ignored.
 *				In case of ne2k+, DEFAULT="smmode"
 *				check_reg_prop() : Takes the second tuple
 *				instead of the first. ( Solves NFS Prob. )
 *				nei_setup() : macinfo->gldm_index changed
 *				from 0 to 1.
 *				find_irq() : generalised to find irq for
 *				both ne2k a.w.a ne2k+. (Mapping not available,
 *				so configures the index for which we get an 
 *				interrupt)
 *				kmem_free done for all allocated mem before
 *				returning.
 *				check_ioaddr() : removed. NO ioaddr check done
 *				
 */

#pragma ident "@(#) @(#)nei.c	1.11	94/03/22 SMI"

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
#include <sys/nei.h>

/*
 * External declarations 
 */
extern int hz ;

/*
 *  Declarations and Module Linkage
 */

static char ident[] = "NE2000+ AT/LANTIC";
/* Uncomment if DL_TPR device
static int Use_Group_Addr = 0;
*/

#ifdef NEIDEBUG
/* used for debugging */
int	neidebug = 0;
#endif
static int nei_forceload = -1;		/* default disabling due to conflicts */
static int neidelay = 1;

/* Required system entry points */
static	neiidentify(dev_info_t *);
static	neiprobe(dev_info_t *);
static	neidevinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	neiattach(dev_info_t *, ddi_attach_cmd_t);
static	neidetach(dev_info_t *, ddi_detach_cmd_t);

/* Required driver entry points for GLD */
int	nei_reset(gld_mac_info_t *);
int	nei_start_board(gld_mac_info_t *);
int	nei_stop_board(gld_mac_info_t *);
int	nei_saddr(gld_mac_info_t *);
int	nei_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
int	nei_prom(gld_mac_info_t *, int);
int	nei_gstat(gld_mac_info_t *);
int	nei_send(gld_mac_info_t *, mblk_t *);
u_int	neiintr(gld_mac_info_t *);

/* Internal functions */
int	nei_setup(dev_info_t *, gld_mac_info_t *) ;
void	nei_init_board(gld_mac_info_t *) ;
int	nei_getp(gld_mac_info_t *) ;
int	nei_SNIC_overflow_handler(gld_mac_info_t *) ;
u_int	nei_irq_intr_handler( caddr_t ) ;	/* Ver 1.2 */
int	nei_copy_data_to_buffer( unchar , mblk_t * , gld_mac_info_t * ) ;
void	nei_start_tx(int , struct neiinstance * , int ) ;
/* Internal functions --- ne2kplus */
void	nei_SMxfer(caddr_t, caddr_t, size_t, unchar, unchar,int);
int 	nei_io_probe(dev_info_t *,int ,int ,unchar *);
int 	nei_other_probe(dev_info_t *,int ,unchar *);
int 	nei_check_mode(dev_info_t *, int, int);
int 	nei_find_atl(int, int, int);
int 	nei_check_card(int ,unchar *);
int 	nei_check_reg_prop(dev_info_t *,gld_mac_info_t * );
void 	nei_check_media_type(dev_info_t *, gld_mac_info_t *);
int 	nei_find_irq(dev_info_t *,gld_mac_info_t *,int,int );

DEPENDS_ON_GLD;		/* this forces misc/gld to load -- DO NOT REMOVE */

/* Standard Streams initialization */

static struct module_info minfo = {
	NEIIDNUM, "nei", 0, INFPSZ, NEIHIWAT, NEILOWAT
};

static struct qinit rinit = {	/* read queues */
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
};

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, NULL, NULL, NULL, &minfo, NULL
};

struct streamtab neiinfo = {&rinit, &winit, NULL, NULL};

/* Standard Module linkage initialization for a Streams driver */

extern struct mod_ops mod_driverops;

static 	struct cb_ops cb_neiops = {
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
	&neiinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

struct dev_ops neiops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	neidevinfo,		/* devo_getinfo */
	neiidentify,		/* devo_identify */
	neiprobe,		/* devo_probe */
	neiattach,		/* devo_attach */
	neidetach,		/* devo_detach */
	nodev,			/* devo_reset */
	&cb_neiops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,			/* short description */
	&neiops			/* driver specific ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

static	kmutex_t nei_probe_lock;		/* Ver 1.2 */
static	kmutex_t nei_irq_lock;			/* Ver 1.2 */
static	unchar	nei_irq_found = 0 ;		/* Ver 1.2 */

/*
 * Default port map for the NE2000 board 
 */
/* ioport sequence changed to suite ne2kplus logic --- DO_NOT_CHANGE_SEQUENCE */
/* Note that 0x320, 0x340 and 0x360 cause conflicts (aha, aic, lp drivers) */
static int nei_ioports[NE2K_MAX_IOPORTS] = { 0x300, 0x240, 0x280, 0x2C0,
						0x320, 0x340, 0x360};
static int nei_mode[NE2K_MAX_IOPORTS] = { IO_MODE, IO_MODE, IO_MODE, IO_MODE,
						IO_MODE, IO_MODE, IO_MODE };
static int nei_nioports = 7;
/*
static int	irq_no[] = { 5, 2, 3, 4, 9, 10, 11, 12, 13, 15 };
*/

int
_init(void)
{
	mutex_init(&nei_probe_lock, 
	           "NE2000+ MP probe protection", 
	           MUTEX_DRIVER,
	           NULL);			/* Ver 1.2 */
	mutex_init(&nei_irq_lock, 
	           "NE2000+ lock for IRQ probing", 
	           MUTEX_DRIVER,
	           NULL);			/* Ver 1.2 */
	return mod_install(&modlinkage);
}

int
_fini(void)
{
        mutex_destroy(&nei_irq_lock);		/* Ver 1.2 */
        mutex_destroy(&nei_probe_lock);		/* Ver 1.2 */
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

neiidentify(dev_info_t *devinfo)
{

	if (strcmp(ddi_get_name(devinfo), "nei") == 0)
		return DDI_IDENTIFIED;
	else
		return DDI_NOT_IDENTIFIED;
}

/* getinfo(9E) -- Get device driver information */
/*ARGSUSED*/
neidevinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int error;
#ifdef NEIDEBUG
	if (neidebug & NEIDDI)
		cmn_err(CE_CONT, "neidevinfo\n");
#endif

	/* This code is not DDI compliant: the correct semantics */
	/* for CLONE devices is not well-defined yet.            */

	switch (cmd)
	{

		case DDI_INFO_DEVT2DEVINFO:

			if (devinfo == NULL) {
				error = DDI_FAILURE;	/* Unfortunate */
			}
			else {
				*result = (void *)devinfo;
				error = DDI_SUCCESS;
			}
			break;

		case DDI_INFO_DEVT2INSTANCE:

			/* This CLONEDEV always returns zero */

			*result = (void *) 0;
			error   = DDI_SUCCESS;
			break;

		default:
			error = DDI_FAILURE;
	}
	return error;
}

/* probe(9E) -- Determine if a device is present */

neiprobe(dev_info_t *devinfo)
{
	int 	i;	/* base port address */
	unchar 	ether[0x10];
	static	int	lastindex = - 1 ;		/* Ver 1.2 */
	int 	mode = NO_MODE;	/* for SM or I/O mode -- ne2kplus */
	int ne2kplus = 0;	/* default card = ne2k */
	
#ifdef NEIDEBUG
	if (neidebug & NEIDDI)
		cmn_err(CE_CONT, "nei:probe(0x%x)\n", devinfo);
	if (neidebug & 0x80000000) {
		neidebug &= ~0x80000000;
		(void)debug_enter(0);
	}
#endif
	/* disable by default to avoid probe conflicts */
	if (nei_forceload < 0)
		return (DDI_PROBE_FAILURE);

	mutex_enter(&nei_probe_lock);
	i = nei_io_probe(devinfo,IO_MODE,lastindex,ether);
	if (i >= nei_nioports ){
		ne2kplus =1;
		i = nei_io_probe(devinfo,SM_MODE,lastindex,ether);
		if (i >= nei_nioports ){
#ifdef NEIDEBUG
			if (neidebug & NEITRACE)
			cmn_err(CE_CONT,"nei:probe(): No boards found\n");
#endif
			lastindex = i;
			mutex_exit(&nei_probe_lock);
			return DDI_PROBE_FAILURE;
		}
	}
	else
		ne2kplus = nei_check_card(nei_ioports[i],ether);

	/* change mode only if property set */
	mode = nei_check_mode(devinfo,nei_ioports[i], ne2kplus);

	lastindex= i;	/* remember the index where the board was found */

	if (mode == NO_MODE){
		mutex_exit(&nei_probe_lock);
		return DDI_PROBE_FAILURE;
	}
	if (ne2kplus==0)			/* Ver 2.2 */
		mode = IO_MODE;

	/*   iobase	8/16bit_slot	flags
	 *---16bits------8bits-----------8bits---
	 *			    (bit0=mode, 1=SM 0=I/O)
	 *			    (bit1=ne2kplus card)
	 */
	ddi_set_driver_private(devinfo,(caddr_t)((nei_ioports[i] << 16)
				 | (ether[14] << 8) | (ne2kplus <<1) | mode ));
#ifdef NEIDEBUG
	if (neidebug & NEITRACE) {
  	if (ne2kplus)
	   cmn_err(CE_CONT,"nei:probe(): ne2kplus board found at iobase=0x%x\n",
		     nei_ioports[i]);
	  else
	    cmn_err(CE_CONT,"nei:probe(): ne2k board found at iobase=0x%x\n",
		     nei_ioports[i]);
   	delay ( 5 * neidelay * drv_usectohz( 1000 ) ) ;
	}
#endif
	mutex_exit(&nei_probe_lock);
	return DDI_PROBE_SUCCESS ;
}


/*
 *  attach(9E) -- Attach a device to the system
 *
 *  Called once for each board successfully probed.
 */

neiattach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */
	struct neiinstance *neip;		/* Our private device info */

#ifdef NEIDEBUG
	if (neidebug & NEIDDI)
		cmn_err(CE_CONT, "neiattach(0x%x)\n", devinfo);
#endif
	if (cmd != DDI_ATTACH)
		return DDI_FAILURE;
	/*
	 *  Allocate gld_mac_info_t and neiinstance structures
	 */
	macinfo = (gld_mac_info_t *)kmem_zalloc(
			sizeof(gld_mac_info_t)+sizeof(struct neiinstance),
			KM_NOSLEEP);
#ifdef	NEIDEBUG
	if (neidebug & NEITRACE)
	cmn_err(CE_CONT,"allocated 0x%x bytes\n",sizeof(gld_mac_info_t)+sizeof(struct neiinstance));
#endif
	if (macinfo == NULL)
		return DDI_FAILURE;
	neip = (struct neiinstance *)(macinfo+1);

	/*  Initialize our private fields in macinfo and neiinstance */
	macinfo->gldm_private = (caddr_t)neip;
	/* changed -- ne2kplus */
	macinfo->gldm_port = ( long )				/* Ver 1.5 */
	                     ddi_get_driver_private( devinfo ) >> 16 ;
	neip->memsize = ( (long)ddi_get_driver_private( devinfo ) >> 8 ) 
	                                            & 0xFF ; /* Ver 1.5 */
	neip->ne2kplus =( (int) ddi_get_driver_private( devinfo ) >> 1) & 0x01 ;
	neip->mode_sm = ( (int) ddi_get_driver_private( devinfo ) ) & 0x01 ;

#ifdef	NEIDEBUG
		if (neidebug & NEITRACE)
		cmn_err(CE_CONT,"attach(): addr= 0x%x, memsize = 0x%x mode = %d, ne2kplus = %d\n",macinfo->gldm_port,neip->memsize, neip->mode_sm, neip->ne2kplus);
#endif
	/*
	 * setup() will configure the driver and initialises the
	 * neiinstance structure 
	 */
	if ( nei_setup( devinfo , macinfo ) == DDI_FAILURE ){
		kmem_free( (caddr_t)macinfo , sizeof( gld_mac_info_t ) +
		                              sizeof( struct neiinstance ) ) ;
#ifdef	NEIDEBUG
	if (neidebug & NEITRACE)
	cmn_err(CE_CONT,"Freeing 0x%x bytes\n",sizeof(gld_mac_info_t)+
	sizeof(struct neiinstance));
#endif
		return ( DDI_FAILURE ) ;
	}

	macinfo->gldm_state = NEI_IDLE;

	/*
	 *  Initialize pointers to device specific functions which will be
	 *  used by the generic layer.
	 */
	macinfo->gldm_reset   = nei_reset;
	macinfo->gldm_start   = nei_start_board;
	macinfo->gldm_stop    = nei_stop_board;
	macinfo->gldm_saddr   = nei_saddr;
	macinfo->gldm_sdmulti = nei_dlsdmult;
	macinfo->gldm_prom    = nei_prom;
	macinfo->gldm_gstat   = nei_gstat;
	macinfo->gldm_send    = nei_send;
	macinfo->gldm_intr    = neiintr;
	macinfo->gldm_ioctl   = NULL;   	/* no ioclts now */

	/*
	 *  Initialize board characteristics neided by the generic layer.
	 */
	macinfo->gldm_ident = ident;
	macinfo->gldm_type = DL_ETHER;
	macinfo->gldm_minpkt = 0;		/* assumes we pad ourselves */
	macinfo->gldm_maxpkt = NEIMAXPKT;
	macinfo->gldm_addrlen = ETHERADDRL;
	macinfo->gldm_saplen = -2;

	/* Get the board's vendor-assigned hardware network address */
	bcopy((caddr_t)gldbroadcastaddr,
		(caddr_t)macinfo->gldm_broadcast, ETHERADDRL);
	bcopy((caddr_t)macinfo->gldm_vendor,
		(caddr_t)macinfo->gldm_macaddr, ETHERADDRL);

	/*
	 *  Register ourselves with the GLD interface
	 */

	if (gld_register(devinfo, "nei", macinfo) == DDI_SUCCESS) {

		if (neip->mode_sm == SM_MODE)
			neip->shared_mem = (unchar *)macinfo->gldm_memp;

		nei_init_board(macinfo);	/* Always succeeds */
		/* set our address on the board */
		(void)nei_saddr(macinfo);
#ifdef	NEIDEBUG
	if (neidebug & NEITRACE) {
		if (neip->mode_sm == SM_MODE)
		  cmn_err(CE_CONT,"nei:attach() successful, ioaddr=0x%x mode=Shared_mem\n",macinfo->gldm_port);
		else
		  cmn_err(CE_CONT,"nei:attach() successful, ioaddr=0x%x mode=I/O\n",macinfo->gldm_port);
	}
#endif

		return DDI_SUCCESS;
	} else {
		kmem_free((caddr_t)macinfo,
			sizeof(gld_mac_info_t)+sizeof(struct neiinstance));
		return DDI_FAILURE;
	}
}

/*
 * nei_setup()		  ( Internal funcion ) 
 *	devinfo pointer ;
 *	gld_macinfo pointer ;
 * Configure the board which we probed successfully
 * Called from neiattach() 
 * Returns : DDI_SUCCESS or DDI_FAILURE
 */

int	nei_setup(dev_info_t *devinfo , gld_mac_info_t *macinfo)
{
	struct	neiinstance	*neip = 
		(struct neiinstance *)macinfo->gldm_private ;
	int	iobase = macinfo->gldm_port ;
	int	core_iobase = macinfo->gldm_port;
	struct intrprop {
		int spl;
		int irq;
	} *intrprop = NULL;	/* Ver 1.2 */
	int	i,ret ;
	int	len  ;		/* Ver 1.2 */
	int	offset_8k = 0;	/* offset for Xmit and recv in the buffer */
	int	offset_16k = 0;
	int	no_irqs; 	/* Ver 2.2 */
	int mode_changed =0;

	if (neip->mode_sm == SM_MODE)	
		core_iobase += 0x10; /* if SM, offset=0x10 */

#ifdef NEIDEBUG
	if (neidebug & NEIDDI)
		cmn_err(CE_CONT, "nei_setup\n") ;
#endif

	if ( (ddi_getlongprop(DDI_DEV_T_ANY, 
	                      devinfo, 
	                      DDI_PROP_DONTPASS,
	                      "intr", 
	                      (caddr_t)&intrprop, 
	                      &len )) != DDI_PROP_SUCCESS ) {
		goto setup_failure;
	}
	no_irqs = len / sizeof( struct intrprop ) ;	/* number of irqs */

	if(neip->ne2kplus){	 /* Ver 2.2 -- (ignore ioaddr check) set regprop  */
		outb(NEI_INTRMASK,0) ;
		if ( neip->mode_sm == SM_MODE) 
			if (nei_check_reg_prop(devinfo,macinfo) == -1){
				goto setup_failure;
			}
	}
	/* disable further interrupts */
	outb(NEI_INTRMASK,0) ;

	if (no_irqs <= 0)
		goto setup_failure;

	/* find irqs */
	if (neip->ne2kplus){
		unchar	configA;
		
		if (neip->mode_sm == SM_MODE){
			/* change to IO_MODE */
			mode_changed = 1;
			configA = inb(iobase + 0x1A);
			configA &= ~(SMFLG);
			outb(iobase+0x1A, configA);
			delay ( 3 * neidelay * drv_usectohz( 1000 ) ) ;
			neip->mode_sm = IO_MODE;
		}
		if ((ret = nei_find_irq(devinfo, macinfo, 
								neip->ne2kplus, no_irqs)) == -1){
			goto setup_failure;
		}
		if (mode_changed){
			/* mode was SM_MODE, change back */
			configA = inb(iobase + 0x0A);
			configA |= SMFLG;
			outb(iobase+0x0A, configA);
			delay ( 3 * neidelay * drv_usectohz( 1000 ) ) ;
			neip->mode_sm = SM_MODE;
		}
	}	/* end of ne2kplus */
	else{
		if ((ret = nei_find_irq(devinfo, macinfo, neip->ne2kplus, no_irqs)) == -1){
												/* Ver 2.2 */
			goto setup_failure;
		}
	} 	/* end of ne2k card */
	macinfo->gldm_irq_index = ret ;		/* Ver 2.2 -- Now using  */
										/* reg prop to set irq   */
										/* for both ne2k & ne2k+ */
	if(neip->mode_sm == SM_MODE){
		offset_8k = -0x20;
		offset_16k = -0x40;
	}

	if ( neip->memsize == NEI_16BIT_SLOT ){		/* Ver 1.5 */
		neip->recvstart = NEI_RCVSTART16K ;
		neip->tx_buf_flag[0] = neip->tx_buf_flag[1] = 0;
		neip->tx_len[0] = neip->tx_len[1] = 0;
		neip->tx_curr = 0;
		neip->recvstop  = NEI_RCVSTOP16K ;
		neip->txstart[0]   = NEI_TXSTART16K ;
		neip->txstart[1]   = NEI_TXSTART16K  + 6;
	}
	else{
		neip->recvstart = NEI_RCVSTART8K ;
		neip->tx_buf_flag[0] = neip->tx_buf_flag[1] = 0;
		neip->tx_len[0] = neip->tx_len[1] = 0;
		neip->tx_curr = 0;
		neip->recvstop  = NEI_RCVSTOP8K ;
		neip->txstart[0]   = NEI_TXSTART8K ;
		neip->txstart[1]   = NEI_TXSTART8K + 6;
	}
	neip->nxtpkt = neip->recvstart + 1 ;	/* Ver 1.9 */

	bzero((char *)neip->multcntrs,sizeof(neip->multcntrs)) ;
	
	if (neip->mode_sm == IO_MODE){

	outb(NEI_COMMAND,NEI_CSTP|NEI_CDMA) ; /* Reset it */
	outb(NEI_RBYTCNT0, 0xC);	/* 6 byte  ethernet address */
	outb(NEI_RBYTCNT1, 0 );
	outb(NEI_RSTRTADR0, 0 );
	outb(NEI_RSTRTADR1, 0 );
	outb(NEI_COMMAND, NEI_CRREAD );	

	/*
	 * set the vendor specific ethernet address 
	 */
	for ( i = 0 ; i < ETHERADDRL ; i ++ ) {
		macinfo->gldm_vendor[i] = (unchar)inb(NEI_IOPORT) ;
	}

	} 	/* end of IO_MODE */
	else{	/* Shared_mem MODE */
		for ( i = 0 ; i < ETHERADDRL ; i ++ ) {
			macinfo->gldm_vendor[i] = (unchar)inb(NEI_ETHRPROM+i) ;
		}
	}	/* end of SM_MODE */

	/* Select Page 1 to initialise multicast registers*/
	outb(NEI_COMMAND,  NEI_CPG1 | NEI_CSTP);

	/* initiaize all the multicast address register */
	for (i = 0; i < 8; i++)
		outb(NEI_COMMAND+NEI_MULTOFF+i, 0x00);

	/* Select back page 0 */
	outb(NEI_COMMAND, NEI_CDMA | NEI_CSTP | NEI_CPG0);

	if (neip->ne2kplus)
		nei_check_media_type(devinfo, macinfo);
	else 
		macinfo->gldm_media = GLDM_UNKNOWN ;

	if ( neip->ne2kplus && neip->mode_sm == SM_MODE )
		macinfo->gldm_reg_index = 1 ;

	kmem_free(intrprop, len  );		/* Ver 2.2 */
	return ( DDI_SUCCESS ) ;
setup_failure:
		if (intrprop)
			kmem_free(intrprop, len  );	/* Ver 2.2 */
		return DDI_FAILURE;

}



/* 	Initialize the board for operation  */
void	nei_init_board(gld_mac_info_t *macinfo) 
{
	int	core_iobase = macinfo->gldm_port;
	int 	i ;
	struct	neiinstance	*neip = 
		(struct neiinstance *)macinfo->gldm_private ;

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */
#ifdef NEIDEBUG
	if (neidebug & NEIDDI)
		cmn_err(CE_CONT, "nei_init_board\n") ;
#endif
	outb(NEI_COMMAND, NEI_CPG0 | NEI_CSTP | NEI_CDMA );

	/* reset the status */
	outb(NEI_INTRSTS, 0xFF);

	/* program the DCR depending on neip->memsize 	Ver 1.5 */
	if ( neip->memsize == NEI_16BIT_SLOT )
		/* enable word wide transfer */
		outb(NEI_DCR, NEI_DWTS|NEI_DFT1|NEI_DNORM) ;
	else
		/* enable byte wide transfer */
		outb(NEI_DCR, NEI_DFT1|NEI_DNORM);

	/* Initialize multicast support */
	outb(NEI_RCR,  NEI_RCAM | NEI_RCAB);
	
	/* disable interrupts */
	outb(NEI_INTRMASK, 0);

	/* reset the status */
	outb(NEI_INTRSTS, 0xFF);

	/* Init the page stop and start register in page 0 */
	outb(NEI_PSTART, neip->recvstart);
	outb(NEI_PSTOP,  neip->recvstop);
	outb(NEI_BNDRY,  neip->recvstart);
	neip->nxtpkt = neip->recvstart + 1 ;	/* Ver 1.9 */
	 
	/* Select Page 1 */
	outb(NEI_COMMAND,  NEI_CPG1 | NEI_CSTP);

	/* set the current receive page register */
	outb(NEI_CURRENT, neip->recvstart+1);		/* Ver 1.9 */

	/* initialize the physical address register	*/
	for (i = 0; i < 6; i++)
		outb(NEI_COMMAND+NEI_PHYOFF+i, 0x00);

	/* Reselect Page 0 and be ready for operation */
	outb(NEI_COMMAND,NEI_CDMA | NEI_CSTP );
}

/*  detach(9E) -- Detach a device from the system */

neidetach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */

#ifdef NEIDEBUG
	if (neidebug & NEIDDI)
		cmn_err(CE_CONT, "neidetach(0x%x)", devinfo);
#endif

	if (cmd != DDI_DETACH) {
		return DDI_FAILURE;
	}

	/* Get the driver private (gld_mac_info_t) structure */
	macinfo = (gld_mac_info_t *)ddi_get_driver_private(devinfo);

	/* stop the board if it is running */
	(void)nei_stop_board(macinfo);

	/*
	 *  Unregister ourselves from the GLD interface
	 */
	if (gld_unregister(macinfo) == DDI_SUCCESS) {
		kmem_free((caddr_t)macinfo,
			sizeof(gld_mac_info_t)+sizeof(struct neiinstance));
		return DDI_SUCCESS;
	}
	return DDI_FAILURE;
}

/*
 *  GLD Entry Points
 */

/*
 *  nei_reset() -- reset the board to initial state; restore the machine
 *  address afterwards.
 */

int
nei_reset(gld_mac_info_t *macinfo)
{
#ifdef NEIDEBUG
	if (neidebug & NEITRACE)
		cmn_err(CE_CONT, "nei_reset\n");
#endif

	(void)nei_stop_board(macinfo);
	nei_init_board(macinfo);
	(void)nei_saddr(macinfo);
	return (DDI_SUCCESS);
}


/*
 *  nei_start_board() -- start the board receiving and allow transmits.
 */

nei_start_board(gld_mac_info_t *macinfo)
{
	int	core_iobase = macinfo->gldm_port;
	struct neiinstance *neip =		/* Our private device info */
		(struct neiinstance *)macinfo->gldm_private;

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */
#ifdef NEIDEBUG
	if (neidebug & NEITRACE)
		cmn_err(CE_CONT, "nei_start_board\n");
#endif
	outb(NEI_COMMAND, NEI_CPG0 | NEI_CSTP | NEI_CDMA );

	/* Reset the interrupts */
	outb(NEI_INTRSTS, 0xFF );

	/* Set the interrupts */
	outb(NEI_INTRMASK, 0xFF);
	
	/* Reset all error registers */
	(void) inb(NEI_FAETALLY);
	(void) inb(NEI_CRCTALLY);
	(void) inb(NEI_MISPKTTLY);
	
	outb(NEI_TCR, 0 );	/* normal mode */

	/* set receiver modes */

	if ( macinfo->gldm_flags & GLD_PROMISC )
		/* enable promisuous mode */
		outb(NEI_RCR, NEI_RCPRO | NEI_RCAB | NEI_RCAM );
	else
		outb(NEI_RCR, NEI_RCAB | NEI_RCAM );

	/* Ver 1.8 Initialise Q and transmit buffer variables */
	neip->tx_state = NEI_TX_FREE ;
	neip->tx_buf_flag[0] = neip->tx_buf_flag[1] = 0 ;

	/* Ver 1.8 Free any messages in the Q */
	for (;;){
		mblk_t	*mp ;
		NEI_REMOVE_FROM_Q(mp) ;
		if ( !mp ) 
			break ;
		freemsg( mp ) ;
	}

	/* start the board for operation */
	outb(NEI_COMMAND, NEI_CPG0 | NEI_CSTA | NEI_CDMA );
	return (DDI_SUCCESS);
}

/*
 *  nei_stop_board() -- stop board receiving
 */

nei_stop_board(gld_mac_info_t *macinfo)
{
	int	core_iobase = macinfo->gldm_port;
	struct neiinstance *neip =		/* Our private device info */
		(struct neiinstance *)macinfo->gldm_private;

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */
#ifdef NEIDEBUG
	if (neidebug & NEITRACE)
		cmn_err(CE_CONT, "nei_stop_board(0x%x)", macinfo);
#endif

	outb(NEI_COMMAND, NEI_CPG0 | NEI_CSTP | NEI_CDMA );

	/* disable interrupts */
	outb(NEI_INTRMASK, 0);

	/* reset the status */
	outb(NEI_INTRSTS, 0xFF);

	if (macinfo->gldm_state != NEI_ERROR )
		macinfo->gldm_state = NEI_IDLE ;
	return (DDI_SUCCESS);
}


nei_SNIC_reset_and_restart( gld_mac_info_t * macinfo )
{
	struct neiinstance *neip =		/* Our private device info */
		(struct neiinstance *)macinfo->gldm_private;
	int	core_iobase = macinfo->gldm_port;

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */
#ifdef NEIDEBUG
	if (neidebug & NEITRACE)
		cmn_err(CE_CONT, "nei_SNIC_reset\n");
#endif

	(void)nei_stop_board( macinfo ) ;

	/* Removed untimeout() code in Ver 1.2 */

	/* Init the page stop and start register in page 0 */
	outb(NEI_PSTART, neip->recvstart);
	outb(NEI_PSTOP,  neip->recvstop);
	outb(NEI_BNDRY,  neip->recvstart);
	neip->nxtpkt = neip->recvstart + 1 ;	/* Ver 1.9 */
	 
	/* Select Page 1 */
	outb(NEI_COMMAND,  NEI_CPG1 | NEI_CSTP);

	/* set the current receive page register */
	outb(NEI_CURRENT, neip->recvstart+1);	/* Ver 1.9 */

	(void)nei_start_board( macinfo ) ;
	return (DDI_SUCCESS);
}

/*
 *  nei_saddr() -- set the physical network address on the board
 */

int
nei_saddr(gld_mac_info_t *macinfo)
{
	int	core_iobase = macinfo->gldm_port;
	struct neiinstance *neip =		/* Our private device info */
		(struct neiinstance *)macinfo->gldm_private;
	int	i ;
	unchar  ch = inb(NEI_INTRMASK);

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */
	/* disable interrupts */
	outb(NEI_INTRMASK, 0);
#ifdef NEIDEBUG
	if (neidebug & NEITRACE)
		cmn_err(CE_CONT, "nei_saddr\n");
#endif
	/* select page 1 */
	outb(NEI_COMMAND, NEI_CSTP | NEI_CPG1 | NEI_CDMA ) ;
	for ( i = 0; i < ETHERADDRL ; i++)
		/* Changed gldm_vendor to gldm_macaddr in Ver 1.2 */
		outb(NEI_COMMAND + NEI_PHYOFF + i,macinfo->gldm_macaddr[i]);

	/* reenable interrupts */
	outb(NEI_INTRMASK, ch);
	outb(NEI_COMMAND, NEI_CSTP | NEI_CDMA ) ;
	return (DDI_SUCCESS);
}

/*
 *  nei_dlsdmult() -- set (enable) or disable a multicast address
 *
 *  Program the hardware to enable/disable the multicast address
 *  in "mcast".  Enable if "op" is non-zero, disable if zero.
 */

int
nei_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	unchar row,col,i,mval;
	unchar command;
	int	core_iobase = macinfo->gldm_port;
	struct neiinstance *neip =		/* Our private device info */
		(struct neiinstance *)macinfo->gldm_private;

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */

	if (macinfo->gldm_state == NEI_ERROR)
		return (DDI_FAILURE);

	/* Calculate the row and col of the mcast addr to be programmed */

	i = gldcrc32(mcast->ether_addr_octet) >> 26 ;
	row = i/8;
	col = i%8;

	command = inb(NEI_COMMAND);
	/* Select Page 1 */

	outb(NEI_COMMAND, NEI_CPG1 | (command & 0x3f) );
	mval = inb( NEI_COMMAND+NEI_MULTOFF+row);

	if ( op ) {
		mval |= ( 1 << col );
		neip->multcntrs[i]++ ;
	}
	else {
		if ( --neip->multcntrs[i] == 0)
			mval &= ~(1 << col );
	}
	outb( NEI_COMMAND+NEI_MULTOFF+row, mval );
	outb( NEI_COMMAND, command);
	return (DDI_SUCCESS);
}


/*
 * nei_prom() -- set or reset promiscuous mode on the board
 *
 *  Program the hardware to enable/disable promiscuous mode.
 *  Enable if "on" is non-zero, disable if zero.
 */

int
nei_prom(gld_mac_info_t *macinfo, int on)
{
	int	core_iobase = macinfo->gldm_port;
	struct neiinstance *neip =		/* Our private device info */
		(struct neiinstance *)macinfo->gldm_private;

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */
	if (macinfo->gldm_state == NEI_ERROR)
		return (DDI_FAILURE);

	if (on) {
		macinfo->gldm_flags |= GLD_PROMISC ;
		outb(NEI_RCR, NEI_RCPRO | NEI_RCAB | NEI_RCAM );
	}
	else {
		macinfo->gldm_flags &= ~(GLD_PROMISC);
		outb(NEI_RCR, NEI_RCAB | NEI_RCAM );
	}
	return (DDI_SUCCESS);
}

/*
 * nei_gstat() -- update statistics
 *
 *  GLD calls this routine just before it reads the driver's statistics
 *  structure.  If your board maintains statistics, this is the time to
 *  read them in and update the values in the structure.  If the driver
 *  maintains statistics continuously, this routine need do nothing.
 */

int
nei_gstat(gld_mac_info_t *macinfo)
{
	struct neiinstance *neip =		/* Our private device info */
		(struct neiinstance *)macinfo->gldm_private;
	int	core_iobase = macinfo->gldm_port;

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */
#ifdef NEIDEBUG
	if (neidebug & NEITRACE)
		cmn_err(CE_CONT, "nei_gstat(0x%x)", macinfo);
#endif

	macinfo->gldm_stats.glds_crc += inb ( NEI_CRCTALLY);
	macinfo->gldm_stats.glds_frame += inb ( NEI_FAETALLY) ;
	macinfo->gldm_stats.glds_missed += inb ( NEI_MISPKTTLY) ;
	return (DDI_SUCCESS);
}

/*
 *  nei_send() -- send a packet
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
nei_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	int 	i;
	int	core_iobase = macinfo->gldm_port;
	int	length;
	struct neiinstance *neip =		/* Our private device info */
			(struct neiinstance *)macinfo->gldm_private;
	/* Ver 1.5 	moved wordptr inside outw() code */
	unchar  tx_buf;

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */
	/* 
	 * Ver 1.6 if set this means the tx is free and both the buffers and
	 * Qs are full ( a wierd state if transmitter was stopped by any other
 	 * routine )
	 */

	/* 
	 * Ver 1.8 
	 * Removed the variable tx_stopped  and all code related to that 
	 */
	
#ifdef NEIDEBUG
	if (neidebug & NEITRACE)
		cmn_err(CE_CONT, "nei_send\n");
#endif

	/* Ver 1.6 the send routine modified for better performance */

	/* check if both the buffers are full */
	if ( neip->tx_buf_flag[0] && neip->tx_buf_flag[1] ){
		NEI_ADD_TO_Q( i ) ;
		if ( !i ){
			/* Ver 1.8 */
			macinfo->gldm_stats.glds_defer ++ ;
			return 1 ;
		}
		else 
			return 0;
	}

	/* get the index of the free buffer */
	tx_buf = neip->tx_buf_flag[ 0 ] ;

	/* Ver 1.9 if the next buffer is full start the transmission */
 	if ( neip->tx_state == NEI_TX_FREE &&
             neip->tx_buf_flag[tx_buf ^ 0x01]) {
		nei_start_tx( core_iobase , neip , tx_buf ^ 0x01 ) ;

	}
	/* fill buffer */
	if ((length = nei_copy_data_to_buffer(tx_buf,mp,macinfo))<= 0 ){
		if ( length < 0 )
			return 1 ;	/* GLD resend me ! please */
		return 0 ;
	}

	/* Ver 1.2 removed the timer call */

	/* transmit the packet */
	if ( neip->tx_state == NEI_TX_FREE  && 
	     neip->tx_buf_flag[ tx_buf ] ) {
		nei_start_tx( core_iobase , neip , tx_buf ) ;

	}
	return 0;		/* successful transmit attempt */
}

int
nei_copy_data_to_buffer( unchar tx_buf, register mblk_t *mp,
			 gld_mac_info_t *macinfo )
{
	register int	rlen;
	register int	core_iobase = macinfo->gldm_port;
	unchar	savebytes[2] ;
	unchar	oddbytestored ;
	int	length;
	int 	iobase = macinfo->gldm_port ;
	struct neiinstance *neip =		/* Our private device info */
			(struct neiinstance *)macinfo->gldm_private;
	unchar *temp = (unchar*)(neip->shared_mem +
			   (neip->txstart[tx_buf]<<8)) ;

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */
	rlen = length = 0;
	oddbytestored = 0;

	outb(NEI_RSTRTADR0, 0x0 );
	outb(NEI_RSTRTADR1, neip->txstart[tx_buf] );

	for ( ; mp ; mp = mp->b_cont ){
		rlen = mp->b_wptr - mp->b_rptr ;

		if ( rlen <= 0 )
			continue ;
		if (neip->mode_sm == IO_MODE){

		   length += rlen;
		   rlen += oddbytestored;

		   if ( neip->memsize == NEI_16BIT_SLOT ){
			outb(NEI_RBYTCNT0, rlen & 0xfe );
			outb(NEI_RBYTCNT1, rlen >> 8 );
			outb(NEI_COMMAND, NEI_CSTA | NEI_CRWRIT );
			if(oddbytestored){
				savebytes[1] = *mp->b_rptr++;
				outw(NEI_IOPORT, *(ushort *)savebytes);
				rlen -= 2;
			}

			if ( (oddbytestored = (rlen & 0x01)) != 0 )
				savebytes[0] = mp->b_wptr[-1] ;
			repoutsw(NEI_IOPORT, (ushort *)mp->b_rptr, rlen >> 1);
		   } else {
			/* it is 8 bit slot */
			outb(NEI_RBYTCNT0, rlen & 0xff  );
			outb(NEI_RBYTCNT1, rlen >> 8  );
			outb(NEI_COMMAND, NEI_CSTA | NEI_CRWRIT );
			repoutsb(NEI_IOPORT, mp->b_rptr, rlen);
		   }
				
		   /* wait for remote DMA to complete */
		   rlen = 1000000 * neidelay;
		   while ( !(inb(NEI_INTRSTS) & NEI_IRDCE) && rlen )
			rlen -- ;

		   /* remote DMA did not complete */
		   if (rlen == 0) {
			/*
			 * the byte count registers would be non zero 
			 * so clear them and restart later 
			 */
			outb(NEI_RBYTCNT0, 0 );
			outb(NEI_RBYTCNT1, 0 );
			(void)nei_SNIC_reset_and_restart( macinfo ) ;	
			macinfo->gldm_stats.glds_defer ++  ;
			return (-1);
		   }
		} 	/* end of IO_MODE */
		else{	/* Shared_mem MODE */
		     nei_SMxfer((caddr_t)mp->b_rptr, 
		        (caddr_t)temp + length,
				 rlen, neip->cntrl1, neip->cntrl2, iobase);
		   	length += rlen;
		}	/* end of SM_MODE */
	}

	if(oddbytestored && neip->mode_sm == IO_MODE){
		outb(NEI_RBYTCNT0, 0x2 );
		outb(NEI_RBYTCNT1, 0x0 );
		outb(NEI_COMMAND, NEI_CSTA | NEI_CRWRIT );
		outw(NEI_IOPORT, *(ushort *)savebytes);
	}

	if ( length <= 0 )	/* no data to send */
		return 0 ;	/* it is a success for us */

	if (length < NEIMINSEND)	/* pad packet length if needed */
		length = NEIMINSEND;

	/* big packet length  */
	if (length > NEIMAXPKT + GLD_EHDR_SIZE )
		return 0;	/* Ver 1.2 , let GLD drop this packet */

	neip->tx_buf_flag[tx_buf] = 1;

	/* Ver 1.7 Moved these code from send */
	length += length & 0x01;
	neip->tx_len[tx_buf] = length;

	return length ;
}
	
/*
 *  neiintr() -- interrupt from board to inform us that a receive or
 *  transmit has completed.
 */

u_int
neiintr(register gld_mac_info_t *macinfo)
{
	register int	core_iobase = macinfo->gldm_port;
	register struct neiinstance *neip =	/* Our private device info */
		(struct neiinstance *)macinfo->gldm_private;
	int	rxsts;
	mblk_t	*mp ;

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */
	outb(NEI_INTRMASK,0) ;		/* disable further interrupts */

#ifdef NEIDEBUG
	if (neidebug & NEIINT)
		cmn_err(CE_CONT, "neiintr(0x%x)", macinfo);
#endif
	/* is it our interrupt ??? */
	if ( !( inb( NEI_INTRSTS ) ) ) {
		/* enable interrupts */
		outb(NEI_INTRMASK, 0xFF);
		return(DDI_INTR_UNCLAIMED) ;
	}

	macinfo->gldm_stats.glds_intr++;
	/*
	 **** Multiple status set so check for everything *****
	 */

	while(inb( NEI_INTRSTS) & (0xFF)){
		/* Status bit reset in handler */
		if ( inb( NEI_INTRSTS ) & NEI_IOVWE ) 	/* overflow warning */
			(void)nei_SNIC_overflow_handler ( macinfo ) ;
			
		if ( inb( NEI_INTRSTS ) & NEI_IRXEE ){
					/* packet received with error */
			rxsts = inb(NEI_RCVSTS) ;
			if ( rxsts & NEI_RSFO )
				macinfo->gldm_stats.glds_overflow ++ ;

			if ( rxsts & NEI_RSCRC )
				macinfo->gldm_stats.glds_crc ++ ;

			if ( rxsts & NEI_RSFAE )
				macinfo->gldm_stats.glds_frame ++ ;

			if ( rxsts & NEI_RSMPA )
				macinfo->gldm_stats.glds_missed ++ ;

			macinfo->gldm_stats.glds_errrcv ++ ;

			outb( NEI_INTRSTS , NEI_IRXEE ) ;
		}

		/*
		 **** Transmit Interrupts 
		 */
		if ( inb( NEI_INTRSTS ) & NEI_ITXEE ){	/* transmit error */
			rxsts = inb( NEI_TXSTATUS ) ;
			if ( rxsts &  NEI_TSABT )
				macinfo->gldm_stats.glds_excoll ++ ;
	
			if ( rxsts &  NEI_TSFU )
				macinfo->gldm_stats.glds_underflow ++ ;
	
			if ( rxsts &  NEI_TSCRS )
				macinfo->gldm_stats.glds_nocarrier ++ ;
	
			if ( rxsts &  NEI_TSCOL ){
				rxsts = inb( NEI_NCR ) ;
				macinfo->gldm_stats.glds_collisions += rxsts ;
			}
			macinfo->gldm_stats.glds_errxmt ++ ;
		}
	
		if ( inb( NEI_INTRSTS ) & (NEI_IPTXE | NEI_ITXEE)){  
						/* packet transmitted ok */
			/*  clear transmit status */
			if ( inb( NEI_INTRSTS ) & NEI_IPTXE ){
				(void)inb( NEI_TXSTATUS ) ;	/* Ver 1.9 */
				outb( NEI_INTRSTS , NEI_IPTXE )	; /* Ver 1.9 */
			}
			if ( inb( NEI_INTRSTS ) & NEI_ITXEE ) {
				(void)inb( NEI_TXSTATUS ) ;	/* Ver 1.9 */
				outb( NEI_INTRSTS , NEI_ITXEE )	; /* 1.10 */
			}

			/*
	 		 * Ver 1.2 Removed timer specific code
	 		 */
			neip->tx_buf_flag[neip->tx_curr] = 0 ;

			/*
			 * Ver 1.9 the following code of transmit added now 
			 * we transmit the filled buffer first and then 
			 * do filling of any empty buffers 
			 */
	
			/* get the next buffer */
			rxsts = neip->tx_curr ^ 0x01 ; /* Ver 1.9 */

			neip->tx_state = NEI_TX_FREE ;
	
			/* if the next buffer is full the transmitt */
			if ( neip->tx_buf_flag[rxsts] )
				nei_start_tx(core_iobase,neip,rxsts) ;

			rxsts ^= 0x01 ;
	
			/* if any buffer is empty fill it */
			if ( !neip->tx_buf_flag[rxsts] ){
				NEI_REMOVE_FROM_Q( mp ) ;
				if ( mp ) {
					(void)nei_copy_data_to_buffer(rxsts,
					                        mp,macinfo);
					freemsg(mp) ;
				}
			}

			/* 
			 * if the transmitter is still free the transmit 
			 * the buffer just filled above
			 */
			if ( neip->tx_state == NEI_TX_FREE &&
			     neip->tx_buf_flag[rxsts] )
				nei_start_tx(core_iobase,neip,rxsts) ;
		}
			
		/*
		 **** Counter overflow Interrupt
		 */
		if ( inb( NEI_INTRSTS ) & NEI_ICNTE ){	
					/* counter overflow error */
			(void)nei_gstat( macinfo ) ;
			outb(NEI_INTRSTS , NEI_ICNTE ) ;
		}

		/*
		 * Ver 1.10
		 * We try to overlap the Remote DMA Read ( in getp ) with
		 * the transmit if started above .
		 */	
		if ( inb( NEI_INTRSTS ) & NEI_IPRXE ){	/* packet received ok */
			outb( NEI_INTRSTS , NEI_IPRXE ) ;
			(void)nei_getp( macinfo )  ;
		}

		/* Ver 1.9 if Remote DMA interrupt set clear it */
		if ( inb( NEI_INTRSTS ) & NEI_IRDCE )
			outb(NEI_INTRSTS , NEI_IRDCE ) ;

		if ( inb( NEI_INTRSTS ) & NEI_IRRST )
			outb(NEI_INTRSTS , NEI_IRRST ) ;
	}
	
	/* Ver 1.9 Resetting interrupt status then and there */

#ifdef NEIDEBUG
	if (neidebug & NEIINT && inb(NEI_INTRSTS))
		cmn_err(CE_CONT, "neiintr sts (0x%x)", inb(NEI_INTRSTS));
#endif
	/* enable interrupts */
	outb(NEI_INTRMASK, 0xFF);

	return DDI_INTR_CLAIMED;	/* Indicate it was our interrupt */
}

int
nei_SNIC_overflow_handler(gld_mac_info_t *macinfo)
{
	int	core_iobase = macinfo->gldm_port;
	struct neiinstance *neip =		/* Our private device info */
		(struct neiinstance *)macinfo->gldm_private;
	unchar	resend ;
	
	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */
# ifdef NEIDEBUG
	cmn_err(CE_CONT,"overflow\n") ;
# endif

	/* 
	 * Ver 1.8
	 * Removed resend variable and its code , since when we stop the
	 * board any transmit/receive will finish to completion 
	 * Ver 1.9
	 * The above code was removed since the overflow handler was modified
	 */

	/* issue a stop command */
	outb( NEI_COMMAND , NEI_CSTP|NEI_CDMA ) ;

	/********** wait for 1.6 mseconds ( app 2 ms here )************/
	delay ( drv_usectohz( 2 ) ) ;	/* portable than loops */

	/* clear remote DMA byte count registers */
	outb( NEI_RBYTCNT0 , 0 ) ;
	outb( NEI_RBYTCNT1 , 0 ) ;

	/* put SNIC in loopback and issue a start command */
	outb( NEI_TCR , NEI_TCLOOP2 ) ;

	/* Ver 1.9
	 * resend is set to 0 if tx over or error else
	 * it is set to 1 for a later transmit
	 */
	resend = 0 ;
	if ( neip->tx_state == NEI_TX_BUSY )
		if( !(inb(NEI_INTRSTS) & (NEI_IPTXE | NEI_ITXEE)) )
                        resend = 1;
		else
			/* clear tx status bits */
			outb( NEI_INTRSTS , NEI_IPTXE|NEI_ITXEE ) ;
		
	outb( NEI_COMMAND , NEI_CSTA|NEI_CDMA ) ;

	/* get all the packets from the buffer ring */
	(void)nei_getp( macinfo ) ;
	outb( NEI_INTRSTS , NEI_IOVWE ) ;

	/* take SNIC out of loopback mode */
	outb( NEI_TCR , 0 ) ;

 	if( resend )
		/* Restart the suspended transmission */
		nei_start_tx(core_iobase , neip , neip->tx_curr ) ;
	else{
                neip->tx_buf_flag[neip->tx_curr] = 0;
                neip->tx_state = NEI_TX_FREE;
		if ( neip->tx_buf_flag[ neip->tx_curr ^ 0x01 ] )
			nei_start_tx(core_iobase , neip ,neip->tx_curr ^ 0x01 );
        }
	return (DDI_SUCCESS);
}

int
nei_getp(gld_mac_info_t *macinfo)
{
	register struct neiinstance *neip =	/* Our private device info */
		(struct neiinstance *)macinfo->gldm_private;
	register int	core_iobase = macinfo->gldm_port;
	int 	iobase = macinfo->gldm_port ;
	ushort	pkthdr[ sizeof(struct recv_pkt_hdr)/2 ] ;
	register struct	recv_pkt_hdr *ptrpkthdr =
					 ( struct recv_pkt_hdr *)pkthdr ;
	mblk_t	*mesgptr ;
	char	*dataptr ;
	unchar	*virtual_addr;
	int	nxtptr ;
	int	pkt_len;
	unchar	boundary ;

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */
#ifdef NEIDEBUG
	if (neidebug & NEIINT)
		cmn_err(CE_CONT, "nei_getp(0x%x)", macinfo);
#endif

	/*
	 * Ver 1.8
	 * Removed the programming of Command with TXP set if there was
	 * pending transmit . Same reason as explained in the overflow
	 * handler 
	 * Ver 1.9 
	 * Removed the above code ( see overflow handler )
	 */
	if ( !( inb( NEI_RCVSTS ) & NEI_RSPRX ) )
		return 0 ;	/* packet was not received properly */

	outb( NEI_COMMAND , NEI_CDMA | NEI_CPG1 | NEI_CSTA ) ;

	while ( neip->nxtpkt != inb(NEI_CURRENT) ){
		outb( NEI_COMMAND , NEI_CDMA | NEI_CPG0 | NEI_CSTA ) ;
		boundary = neip->nxtpkt ;

		if (neip->mode_sm == IO_MODE){

			/* program the remote byte count registers */
			outb( NEI_RBYTCNT0,0x04 ) ;	/* first 4 bytes */
			outb( NEI_RBYTCNT1,0 ) ;

			/* program the remote address registers */
			outb( NEI_RSTRTADR0,0 ) ;
			outb( NEI_RSTRTADR1,boundary ) ;
		
			/* Do remote DMA read */
			outb(NEI_COMMAND, NEI_CRREAD | NEI_CSTA );

			/* read the packet header */
			if ( neip->memsize == NEI_16BIT_SLOT ) {
				*(ushort *)ptrpkthdr = inw(NEI_IOPORT);
				*((ushort *)ptrpkthdr+1) = inw(NEI_IOPORT);
			} else {
				*(unchar *)ptrpkthdr = inb(NEI_IOPORT);
				*((unchar *)ptrpkthdr+1) = inb(NEI_IOPORT);
				*((unchar *)ptrpkthdr+2) = inb(NEI_IOPORT);
				*((unchar *)ptrpkthdr+3) = inb(NEI_IOPORT);
			}

		} 	/* end of IO_MODE */
		else{	/* Shared_mem MODE */
	  	     virtual_addr = (unchar *)( neip->shared_mem +
						   (boundary<<8) );
		     nei_SMxfer((caddr_t)virtual_addr, (caddr_t)ptrpkthdr, 4, 
				neip->cntrl1,neip->cntrl2,iobase);
		     virtual_addr += 4;
		}
		/*
		 * Check if next packet pointer is set ok by the SNIC
		 */
		pkt_len = ptrpkthdr->pktlen;
		nxtptr = neip->nxtpkt + 
			((pkt_len+sizeof(struct recv_pkt_hdr)) >> 8 );

		/* the packet is not aligned on 256 byte boundary */
		if (( pkt_len + sizeof(struct recv_pkt_hdr) ) & 0xff )
			nxtptr ++ ;

		if ( nxtptr >= neip->recvstop )
			nxtptr = neip->recvstart + (nxtptr - neip->recvstop);

		if ( nxtptr != ptrpkthdr->nxtpkt || 
                     ptrpkthdr->nxtpkt  < neip->recvstart ){
			(void)nei_SNIC_reset_and_restart( macinfo ) ;
			return 0 ;
		}

		/* Assign next packet pointer */
		neip->nxtpkt = ptrpkthdr->nxtpkt ;

		if ( pkt_len < NEIMINSEND || 
		     (pkt_len > NEIMAXPKT + GLD_EHDR_SIZE + NEICRCLENGTH) ) { 
			/* skip packet */
    			if((neip->nxtpkt - 1) < neip->recvstart) /* Ver 1.10 */
				outb(NEI_BNDRY, neip->recvstop - 1);
			else
				outb(NEI_BNDRY, neip->nxtpkt - 1);

			/* set page 1 for current register */
			outb( NEI_COMMAND , NEI_CDMA | NEI_CPG1 | NEI_CSTA ) ;
			macinfo->gldm_stats.glds_short ++ ;
			continue ;
		}

		if (ptrpkthdr->status == 0 || ( ptrpkthdr->status & 0x4e ) ){
			/* skip packet */
    			if((neip->nxtpkt - 1) < neip->recvstart) /* Ver 1.10 */
				outb(NEI_BNDRY, neip->recvstop - 1);
			else
				outb(NEI_BNDRY, neip->nxtpkt - 1);

			/* set page 1 for current register */
			outb( NEI_COMMAND , NEI_CDMA | NEI_CPG1 | NEI_CSTA ) ;
			continue ;
		}

		/* align the read on a word boundry */
		ptrpkthdr->pktlen += ptrpkthdr->pktlen & 0x01;

		/*
	 	 * allocate a STREAMS message block ( mblk_t )  
		 * to send it upstream
	 	 */
		if ( ( mesgptr = allocb( ptrpkthdr->pktlen , 0 ) ) == NULL ){
# ifdef NEIDEBUG
cmn_err(CE_CONT,"allocb failure\n") ;
# endif
			macinfo->gldm_stats.glds_norcvbuf ++ ;
    			if((neip->nxtpkt - 1) < neip->recvstart) /* Ver 1.10 */
				outb(NEI_BNDRY, neip->recvstop - 1);
			else
				outb(NEI_BNDRY, neip->nxtpkt - 1);

			/* set page 1 for current register */
			outb( NEI_COMMAND , NEI_CDMA | NEI_CPG1 | NEI_CSTA ) ;
			continue ;
		}

		dataptr = (char *)mesgptr->b_wptr ;
		mesgptr->b_wptr = mesgptr->b_rptr + ptrpkthdr->pktlen ;

		if (neip->mode_sm == IO_MODE){ 	/* IO_MODE */

			outb( NEI_RBYTCNT0,(ptrpkthdr->pktlen & 0xff) ) ;
			outb( NEI_RBYTCNT1,((ptrpkthdr->pktlen & 0xff00) >> 8));

			/* program the remote address registers */
			outb( NEI_RSTRTADR0,4 ) ; /* skip packet hdr in Ring */
			outb( NEI_RSTRTADR1,boundary ) ;
			outb( NEI_COMMAND ,  NEI_CRREAD | NEI_CSTA ) ;
		
			if ( neip->memsize == NEI_16BIT_SLOT )
				repinsw(NEI_IOPORT, (ushort *)dataptr,
					ptrpkthdr->pktlen/2);
			else
				repinsb(NEI_IOPORT, (unchar *)dataptr,
					ptrpkthdr->pktlen);

		} 	/* end of IO_MODE */
		else{	/* Shared_mem MODE */
		    if ( ((boundary << 8) + pkt_len) >= (neip->recvstop<<8)){
			nxtptr = ((ulong)(neip->recvstop - boundary)<<8) - 4;	
		        nei_SMxfer((caddr_t)virtual_addr, dataptr, nxtptr,
				neip->cntrl1, neip->cntrl2, iobase);
			dataptr += nxtptr;
			pkt_len -= nxtptr;
	  	        virtual_addr = (unchar *)( neip->shared_mem +
						   (neip->recvstart << 8) );
		   }
	           nei_SMxfer((caddr_t)virtual_addr,(caddr_t)dataptr, pkt_len, 
			  neip->cntrl1,neip->cntrl2, iobase);
		}	/* end of SM_MODE */


    		if((neip->nxtpkt - 1) < neip->recvstart) /* Ver 1.10 */
			outb(NEI_BNDRY, neip->recvstop - 1);
		else
			outb(NEI_BNDRY, neip->nxtpkt - 1);

		gld_recv( macinfo , mesgptr ) ;

		/* select page 1 to read the current address register */
		outb( NEI_COMMAND , NEI_CDMA | NEI_CPG1 | NEI_CSTA ) ;
	} 

	outb( NEI_COMMAND , NEI_CDMA | NEI_CPG0 | NEI_CSTA ) ;
	return 1;
}

u_int
nei_irq_intr_handler( caddr_t mac )
{
	gld_mac_info_t	*macinfo = ( gld_mac_info_t *)mac ;
	struct neiinstance *neip =		/* Our private device info */
		(struct neiinstance *)macinfo->gldm_private;
	unchar	intrsts ;
	int	core_iobase = macinfo->gldm_port;

	if (neip->mode_sm == SM_MODE)	core_iobase += 0x10; /* if SM, offset=0x10 */

	outb(NEI_INTRMASK,0) ;		/* disable further interrupts */

#ifdef NEIDEBUG
	if (neidebug & NEIINT)
		cmn_err(CE_CONT,"nei_irq_intr_handler(0x%x) %x", macinfo,
				core_iobase);
#endif
	/* is it our interrupt ??? */
	if ( !( intrsts = inb( NEI_INTRSTS ) ) ) 
		return(DDI_INTR_UNCLAIMED) ;

	outb(NEI_INTRSTS , intrsts ) ;

	if ( intrsts & NEI_IRDCE ) 
		/* indicate that we have received the intr */
		nei_irq_found ++ ;

	return(DDI_INTR_CLAIMED) ;
}

void
nei_start_tx( int core_iobase ,struct neiinstance *neip , int tx_buf )
{
	neip->tx_state = NEI_TX_BUSY ;
	neip->tx_curr = (unchar)tx_buf ;
	outb(NEI_TXPAGE, neip->txstart[neip->tx_curr]);
	outb(NEI_TXBYTCNT0, (neip->tx_len[neip->tx_curr] & 0xff));
	outb(NEI_TXBYTCNT1,(((neip->tx_len[neip->tx_curr] & 0xff00)) >> 8) );
	outb(NEI_COMMAND, NEI_CSTA | NEI_CTXP) ;
}


/* nei_SMxfer() : internal function , called for data transfer (bcopy)
 * in case of Shared_Memory mode of ne2kplus card.
 */
void
nei_SMxfer(caddr_t PCaddr, caddr_t BuffAddr, size_t size, 
		unchar cntrl1, unchar cntrl2,int iobase)
{
	outb(NEI_CNTRREG1,(cntrl1|NEI_REG1MEME));
	outb(NEI_CNTRREG2,(cntrl2|NEI_REG28OR16));
	bcopy(PCaddr,BuffAddr,size);
	outb(NEI_CNTRREG1,cntrl1);
	outb(NEI_CNTRREG2,cntrl2);
}

/* nei_other_probe() : internal function , called from probe().
 * Probes for other boards, returns -1 if not found
 * else returns the index in the io_ports array
 */
/*ARGSUSED*/
int
nei_other_probe(dev_info_t *devinfo, int iobase, unchar *buf)
{
	int i;
	int type;
	int asic_addr;

	/*
	 * WD80x3
	 * Read what should be the station prom and address.
	 * Can't trust the checksum.
	 */
	for (i = 0; i < 8; ++i)
		buf[i] = inb(iobase + NEI_WD_PROM + i);

	/*
	 * General
	 * If more than half the bytes are 0xff give up
	 */
	for (i = 0; i < 8; ++i)
		if (buf[i] != 0xff)
			break;
	if (i >= 8)
		return (1);

	type = inb(iobase + NEI_WD_CARD_ID);
	/*
	 * Set initial values for width/size.
	 */
	switch (type) {
	case NEI_TYPE_WD8003S:
	case NEI_TYPE_WD8003E:
	case NEI_TYPE_WD8003EB:
	case NEI_TYPE_WD8003W:
	case NEI_TYPE_WD8013EBT:
	case NEI_TYPE_WD8013W:
	case NEI_TYPE_WD8013EP:	/* also WD8003EP */
	case NEI_TYPE_WD8013WC:
	case NEI_TYPE_WD8013EBP:
	case NEI_TYPE_WD8013EPC:
	case NEI_TYPE_SMC8216C:
	case NEI_TYPE_SMC8216T:
	case NEI_TYPE_TOSHIBA1:
	case NEI_TYPE_TOSHIBA4:
		return (1);
	}

	asic_addr = iobase + NEI_3COM_ASIC_OFFSET;
	/*
	 * 3Com
	 * Verify that the kernel configured I/O address matches the board
	 * configured address
	 */
	switch (inb(asic_addr + NEI_3COM_BCFR)) {
	case NEI_3COM_BCFR_300:
	case NEI_3COM_BCFR_310:
	case NEI_3COM_BCFR_330:
	case NEI_3COM_BCFR_350:
	case NEI_3COM_BCFR_250:
	case NEI_3COM_BCFR_280:
	case NEI_3COM_BCFR_2A0:
	case NEI_3COM_BCFR_2E0:
		/*
	 	 * Verify that kernel shared memory address matches the board
	 	 * configured address.
	 	 */
		switch (inb(asic_addr + NEI_3COM_PCFR)) {
		case NEI_3COM_PCFR_DC000:
		case NEI_3COM_PCFR_D8000:
		case NEI_3COM_PCFR_CC000:
		case NEI_3COM_PCFR_C8000:
			return (1);
		}
	}
   	delay (5 * neidelay * drv_usectohz(1000));
	return (0);
}

/* nei_io_probe() : internal function , called from probe().
 * Probes for the board, returns -1 if not found
 * else returns the index in the io_ports array
 */
int
nei_io_probe(dev_info_t *devinfo,int mode,int index,unchar *buf)
{
	int 	iobase,core_iobase;
	int	i,j, csum, orgcsum ;
	int	*ioaddr, iolen;
	unchar 	ch ;
	unchar	configA;
	int	other;
	
#ifdef NEIDEBUG
	if (neidebug & NEIDDI)
		cmn_err(CE_CONT, "neiprobe(0x%x)\n", devinfo);
#endif
	/* set ioaddr property on first probe */
	if (index == -1 && ddi_getlongprop (DDI_DEV_T_ANY, devinfo,
				DDI_PROP_DONTPASS, "ioaddr",
				(caddr_t)&ioaddr, &iolen) == DDI_PROP_SUCCESS) {
		i = iolen/sizeof(int);			/* number of props */
		j = (sizeof(nei_ioports)/sizeof(int));	/* max space to fit */
		nei_nioports = i > j ? j : i; 
		for (i = nei_nioports, j = 0; i > 0; i--, j++) {
			nei_ioports[j] = ioaddr[j];
			nei_mode[j] = IO_MODE;
		}
		kmem_free(ioaddr, iolen);
	}

	/* The code till the end is done in Ver 1.2 */
	for (i = index + 1; i < nei_nioports && nei_ioports[i]; i++) {
		iobase = nei_ioports[i];	/* get the port base */
		core_iobase = iobase;

		/* query if a known non-nei card */
		other = nei_other_probe(devinfo, iobase, buf);
		/* query if atlantic */
		j = nei_find_atl(iobase, 1, other);
		if (other && j == NO_MODE) {
			/* skip other cards which don't have atlantic */
			continue;
		} else if (j == SM_MODE && mode == IO_MODE) {
			/* reset atlantic to IO */
			configA = inb(iobase + 0x1A);
			configA &= ~(SMFLG);
			outb(iobase+0x1A, configA);
			delay ( 3 * neidelay * drv_usectohz( 1000 ) ) ;
		} else if (j == IO_MODE && mode == SM_MODE) {
			/* reset atlantic to SM */
			configA = inb(iobase + 0x0A);
			configA |= SMFLG;
			outb(iobase+0x0A, configA);
			delay ( 3 * neidelay * drv_usectohz( 1000 ) ) ;
		}

		if (mode == IO_MODE) {
			/* 
		 	 * Before we start reading the ID registers we need to 
		 	 * put the board in a proper state .The NEI_COMMAND reg
		 	 * should have a pattern 0x21 . This can be done by
		 	 * doing an outb() NEI_COMMAND or by doing an inb() on
		 	 * the NEI_RESETPORT . inb()ing from RESETPORT also
		 	 * initialises other registers
		 	 */

			/* Do a hard reset by inb()ing from (iobase + 0x1F ) */
			ch = inb(NEI_RESETPORT);

			/* Wait for the board to get reset */
			delay ( 1 * neidelay * drv_usectohz( 1000 ) ) ;

			/* take it out of reset */
			outb(NEI_RESETPORT,ch);

			delay ( 1 * neidelay * drv_usectohz( 1000 ) ) ;

		} else {
			/* attempt SM_MODE reset */
			/* Do a hard reset by inb()ing from cntrl1 */
			ch = inb(NEI_CNTRREG1);
			outb(NEI_CNTRREG1, ch | SMFLG);

			/* take it out of reset */
			outb(NEI_CNTRREG1, ch & ~SMFLG);
			core_iobase += 0x10; /* if SM, offset=0x10 */

			/* Wait for the board to get reset */
			delay ( 2 * neidelay * drv_usectohz( 1000 ) ) ;
		}

		/*
		 * Ver 1.2 
		 * Here we are not checking for reset patterns from the
		 * NEI_COMMAND register since the inb() might cause data
		 * corruption if a 3C509 card is also present at the same
		 * iobase ( A known problem with 3C509 )
		 */

		/*
		 * After a hard reset the NEI_COMMAND register might have
		 * bits 7 and 6 set , which will not allow us to access
		 * the other registers below . The reset operation only
		 * sets bit 0 and bit 5 and clears bits  1 and 2 .
		 * All other bits are don't care . So to access the board
		 * we need to set bit 0 and bit 5 and clear others .
	 	 */
		outb( NEI_COMMAND ,NEI_CDMA | NEI_CSTP ) ; 

		 /* word transfer in the Data configuration register */
		outb( NEI_DCR,NEI_DWTS|NEI_DFT1|NEI_DNORM);

		/* 
		 * Get the ETHERNET ID and calculate the Checksum .
		 * Set remote byte count register for number of ID bytes
		 * to be read. The byte count should be double the # of
		 * bytes to be read, because word mode transfers 
 		 * decrements the RBYTCNT registers by 2 for every read/write
 		 * operation
		 */
		outb(NEI_RBYTCNT0, 0x20);       /* 0x10 * 2 bytes */
		outb(NEI_RBYTCNT1, 0 );

		/* Set remote start addr register to start of PROM */
		outb(NEI_RSTRTADR0, 0 );
		outb(NEI_RSTRTADR1, 0 );

		/* Do remote DMA read */
		outb(NEI_COMMAND, NEI_CRREAD ); 
        
		csum = 0;

		/* Read PROM bytes from the io port */
		for(j = 0; j < 0x10 ; j++){
			buf[j] = (unchar)inb(NEI_IOPORT);
			if ( j < 12)
				csum += buf[j]; 
		}

		/* Compute the check sum to identify the NE2000A board */
		csum   += buf[14] + buf[15] ;
		orgcsum = buf[12] ;
		orgcsum |= (( buf[13] << 8 ) & 0xff00) ;

		/*
		 * Ver 1.8
		 * The token NEI_CHECKSUM_COMPARE shold be defined only
		 * for NE2000A boards , Even if not defined will not cause
		 * any problems . This is done since the checksum computation
		 * differs between NE2000 family of boards 
		 */

# ifdef  NEI_CHECKSUM_COMPARE
		/* 
		 * NE2000 boards built on DP83902 does not have checksum
		 * bytes ( byte 13 and byte 12 are zero ) so for those
		 * boards we do not check the checksum and we still go
		 * further to check for BB or WW in the PROM bytes
		 */
		if ( orgcsum && csum != orgcsum  )
			continue;
# endif

		/* Check for BB or WW from the bytes read from the PROM */
		if ( ( buf[14] == 'B' && buf[15] == 'B' ) ||
		     ( buf[14] == 'W' && buf[15] == 'W' ) ){
			/* ne2k/ne2kplus board is present */
			/* put it no tx/rx mode */
			outb( NEI_COMMAND ,NEI_CDMA | NEI_CSTP ) ; 
			break ;
                }
        }
	return ( i ) ;
}


/* nei_find_atl() : internal function , called from probe().
 * finds atlantic chip, saves original state in nei_mode[].
 */
/*ARGSUSED*/
int
nei_find_atl(int iobase, int init, int other)
{
	int core_iobase = iobase;
	unchar configA;
	unchar addr;
	int ret;
	int i;
	
	for (i = 0; i <  NE2K_MAX_IOPORTS; i++)
		if (iobase == nei_ioports[i])
			break;

	/* only attempt to read configuration once if non-atl chip */
	if (i >= NE2K_MAX_IOPORTS || nei_mode[i] == NO_MODE)
		return (NO_MODE);

	configA = inb(iobase + 0x0A);
	addr = configA & SMIOMSK;

	/* atl chip may be in IO mode */
	if ((configA & SMFLG) == 0) {
		ret = IO_MODE;
		if (init)
			nei_mode[i] = ret;
		switch (iobase) {
		case 0x300:
			if (addr == 0)
				return (ret);
			break;
		case 0x240:
			if (addr == 2)
				return (ret);
			break;
		case 0x280:
			if (addr == 3)
				return (ret);
			break;
		case 0x2C0:
			if (addr == 4)
				return (ret);
			break;
		case 0x320:
			if (addr == 5)
				return (ret);
			break;
		case 0x340:
			if (addr == 6)
				return (ret);
			break;
		case 0x360:
			if (addr == 7)
				return (ret);
			break;
		default:
			if (addr == 1) {
				cmn_err(CE_CONT,
					"nei: disabled ne2kplus at 0x%x\n", 
					iobase);
			}
		}
	}

	/* atl chip may be in SM mode */
	configA = inb(iobase + 0x1A);
	if ((configA & SMFLG) == 0) {
		/* atl chip can't be in SM mode */
		if (!other) {
			/* a non-atl reset has started: clean up */

			/* Wait for the board to get reset */
			delay ( 1 * neidelay * drv_usectohz( 1000 ) ) ;

			/* take it out of reset */
			outb(iobase + 0x1A, configA);
			outb( NEI_COMMAND ,NEI_CDMA | NEI_CSTP ) ; 
			outb( NEI_DCR,NEI_DWTS|NEI_DFT1|NEI_DNORM);
			delay ( 1 * neidelay * drv_usectohz( 1000 ) ) ;
		}
		ret = NO_MODE;
		if (init)
			nei_mode[i] = NO_MODE;
		return (ret);
	}
	ret = SM_MODE;
	if (init)
		nei_mode[i] = SM_MODE;

	addr = configA & SMIOMSK;
	switch (iobase) {
	case 0x300:
		if (addr == 0)
			return (ret);
		break;
	case 0x240:
		if (addr == 2)
			return (ret);
		break;
	case 0x280:
		if (addr == 3)
			return (ret);
		break;
	case 0x2C0:
		if (addr == 4)
			return (ret);
		break;
	case 0x320:
		if (addr == 5)
			return (ret);
		break;
	case 0x340:
		if (addr == 6)
			return (ret);
		break;
	case 0x360:
		if (addr == 7)
			return (ret);
		break;
	default:
		if (addr == 1) {
			cmn_err(CE_CONT, "nei: disabled ne2kplus at 0x%x\n", 
				iobase);
		}
	}

	if (!other) {
		/* a non-atl chip reset has started: clean up */
		delay ( 1 * neidelay * drv_usectohz( 1000 ) ) ;
	
		/* take it out of reset */
		outb(iobase + 0x1A, configA);
		outb( NEI_COMMAND ,NEI_CDMA | NEI_CSTP ) ; 
		outb( NEI_DCR,NEI_DWTS|NEI_DFT1|NEI_DNORM);
		delay ( 1 * neidelay * drv_usectohz( 1000 ) ) ;
	}

	ret = NO_MODE;
	if (init)
		nei_mode[i] = NO_MODE;
	return (ret);
}

/* nei_check_mode() : internal function , called from probe().
 * Checks whether mode property is present, if not returns card mode,
 * else returns the mode being set.
 */
int
nei_check_mode(dev_info_t *devinfo,int iobase, int ne2kplus)
{
	int i;
	unchar configA;
	char 	*modeprop = (char *)NULL;
	int	mode;

	/* get saved original mode */
	for (i = 0; i <  NE2K_MAX_IOPORTS; i++)
		if (iobase == nei_ioports[i])
			break;
	if (i >= NE2K_MAX_IOPORTS || nei_mode[i] == NO_MODE)
		mode = IO_MODE;
	else
		mode = nei_mode[i];

	modeprop = 0;
	if ( (ddi_getlongprop(DDI_DEV_T_ANY, 
		      devinfo, 
		      DDI_PROP_DONTPASS,
		      "mode", 
		      (caddr_t)&modeprop, 
		      &i )) == DDI_PROP_SUCCESS ) {
		if (strcmp(modeprop,"iomode")==0){		/* Ver 2.2 */
			mode = IO_MODE;
		} else if( (strcmp(modeprop,"smmode")==0) && ne2kplus ) {
			mode = SM_MODE;
		}
	}

	/* place card in SM_MODE */
	if (mode == SM_MODE) {
		configA = inb(iobase + 0x0A);
		configA |= SMFLG;
		outb(iobase+0x0A, configA);
		delay ( 3 * neidelay * drv_usectohz( 1000 ) ) ;
		kmem_free(modeprop, i  );	/* Ver 2.2 */
	}
	if (modeprop)
		kmem_free(modeprop, i  );	/* Ver 2.2 */
	return mode ;	/* Bad mode in .conf file */
}



/* nei_check_card() : internal function , called from probe().
 * Checks whether it is a ne2k or ne2kplus card.
 * returns 1 if ne2kplus card, else 0.
 */
int
nei_check_card(int iobase,unchar *ether)
{
	unchar	configA;
	int	i;

	/* if not atlantic give up */
	if (nei_find_atl(iobase, 0, 0) == NO_MODE)
		return (0);
	/* change to SM_MODE */
	configA = inb(iobase + 0x0A);
	configA |= SMFLG;
	outb(iobase+0x0A, configA);
	delay ( 3 * neidelay * drv_usectohz( 1000 ) ) ;

	for (i=0;i<0x06;i++){
		if(ether[i] != (unchar)inb(NEI_ETHRPROM + i))
			return 0;
	}

	/* change back to IO_MODE */
	configA = inb(iobase + 0x1A);
	configA &= ~(SMFLG);
	outb(iobase+0x1A, configA);
	delay ( 3 * neidelay * drv_usectohz( 1000 ) ) ;
	
	return 1;
}


/* nei_check_reg_prop() : internal function , called from setup.
 * In SM_MODE (ne2kplus), sets the address in the proper regesters.
 * returns -1 if no property present.
 */
int
nei_check_reg_prop(dev_info_t *devinfo,gld_mac_info_t * macinfo)
{
	struct	neiinstance	*neip = 
		(struct neiinstance *)macinfo->gldm_private ;
	int	iobase = macinfo->gldm_port ;
	int	core_iobase = macinfo->gldm_port ;
	struct regprop {
		int resvd;
		int addr;
		int size;
	} *regprop;
	int	reglen;

	if (neip->mode_sm == SM_MODE)	
		core_iobase += 0x10; /* if SM, offset=0x10 */

	if ( (ddi_getlongprop(DDI_DEV_T_ANY, 
		      devinfo, 
		      DDI_PROP_DONTPASS,
		      "reg", 
		      (caddr_t)&regprop, 
		      &reglen )) != DDI_PROP_SUCCESS ) {
		kmem_free(regprop, reglen  );	/* Ver 2.2 */
		return DDI_FAILURE;
	}

	/* configure ControlReg1 and 2 */		
	/* Ver 2.2 -- now second tuple is taken from the reg property -- NFS prob */
	neip->cntrl1 = (unchar)(((ulong)regprop[1].addr >> 13)&0x3F);
	neip->cntrl2 = (unchar)((((ulong)regprop[1].addr >> 19)&0x01));
	if (neip->memsize == 0x57)
		neip->cntrl2 |= 0x40;
	outb(NEI_CNTRREG1, neip->cntrl1);
	outb(NEI_CNTRREG2, neip->cntrl2);
	kmem_free(regprop, reglen  );		/* Ver 2.2 */
	return  1;
}




/* nei_check_media_type() : internal function called from setup for ne2kplus.
 * Sets macinfo->gldm_media
 */
void
nei_check_media_type(dev_info_t *devinfo, gld_mac_info_t *macinfo)
{
	char 	*media;
	int 	i;

	media=0;
	if ( (ddi_getlongprop(DDI_DEV_T_ANY, 
	                      devinfo, 
	                      DDI_PROP_DONTPASS,
	                      "media", 
	                      (caddr_t)&media, 
	                      &i )) == DDI_PROP_SUCCESS ) {
		if (strcmp(media,"thick")==0 || strcmp(media,"dix")==0)
			macinfo->gldm_media = GLDM_AUI;
		else if (strcmp(media,"tp")==0) 
			macinfo->gldm_media = GLDM_TP;
		else if (strcmp(media,"thin")==0 || strcmp(media,"bnc")==0)
			macinfo->gldm_media = GLDM_BNC;
		else	macinfo->gldm_media = GLDM_UNKNOWN ;
	}
		else	macinfo->gldm_media = GLDM_UNKNOWN ;
	if(media) kmem_free(media, i  );		/* Ver 2.2 */
}


/* nei_find_irq():	internal function, called from setup()
 * 	        Called in IO_MODE only.
 * Returns   :  -1 = irq not found else index in the irq_no array
 */
/*ARGSUSED*/
int
nei_find_irq(dev_info_t *devinfo,gld_mac_info_t *macinfo,int ne2kplus,int no_irqs)
{
	int	iobase = macinfo->gldm_port ;
	int	core_iobase = macinfo->gldm_port ;
	int	i;

	/* Ver 1.2 code for finding IRQ done here */
	/*
	 * In NE2000 the IRQs are set on the board using jumpers ,and
	 * there could be 3 possible IRQs for each iobase . Board doesn't
	 * give any information about which IRQ is been selected,so we
	 * add interrupt handlers for all possible IRQs one by one and
	 * do remote DMA operation which will result in an interrupt .
	 * The interrupt handler sets a flag which is used to find out
         * the IRQ no which is set on the board.
	 */

	mutex_enter(&nei_irq_lock) ;
	nei_irq_found = 0 ;	/* clear it */
	for ( i = 0 ; i < no_irqs ; i ++ ){
		int	j ;
		ddi_iblock_cookie_t	iblock_cookie ;

# ifdef NEIDEBUG
		if (neidebug & NEITRACE)
			cmn_err(CE_CONT,"nei_find_irq(%x %d)\n",iobase, i);
# endif
		/* install intr handler for irq number 'i'*/
		if ( (j = ddi_add_intr( devinfo ,
		                   i , /* IRQ index in intr property */
		                   &iblock_cookie ,
		                   NULL ,
		                   nei_irq_intr_handler ,
		                   (caddr_t)macinfo )) != DDI_SUCCESS ){
# ifdef NEIDEBUG
			if (neidebug & NEITRACE)
			cmn_err(CE_CONT,"NE2000 add_intr failed %d\n",j ) ;
# endif
			/* go to next index */
			continue;			/* Ver 2.2 -- Bug fixed, no returning */
		}

		/* enable remote DMA over interrupt */
		outb( NEI_INTRSTS , 0xFF ) ;
		outb( NEI_INTRMASK , NEI_IRDCE) ;
   
		/* do remote DMA operation */
		outb(NEI_COMMAND,NEI_CSTP|NEI_CDMA) ; /* Reset it */
		outb(NEI_RBYTCNT0, 0xC); /* 6 byte  ethernet address */
		outb(NEI_RBYTCNT1, 0 );
		outb(NEI_RSTRTADR0, 0 );
		outb(NEI_RSTRTADR1, 0 );
		outb(NEI_COMMAND, NEI_CRREAD );	
    
		for ( j = 0 ; j < ETHERADDRL ; j ++ ) 
			(void)inb(NEI_IOPORT) ;
    
		/* wait till the interrupt handler is called or timeout */
		j = 1000000 * neidelay;
		while ( j && nei_irq_found == 0 )
			j -- ;
    
		/* remove the interrupt handler registered */
		ddi_remove_intr( devinfo , 
		                 i ,  /* IRQ index in intr property */
		                 &iblock_cookie ) ;

		outb( NEI_INTRMASK , 0) ;
		outb( NEI_INTRSTS , 0xFF ) ;

		if ( nei_irq_found ) {
			mutex_exit(&nei_irq_lock) ;
			return i;  /* our IRQ is intrprop[i].irq */
		}

		/*if different IRQ is set clear the status to stop interrupts */
		outb( NEI_INTRSTS , inb( NEI_INTRSTS ) ) ;
	}
#ifdef	NEIDEBUG
	cmn_err(CE_WARN,"No intr found\n");
#endif
	mutex_exit(&nei_irq_lock) ;
	return -1;

}
