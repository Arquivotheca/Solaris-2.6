/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)nee.c	1.3	95/06/05 SMI"


/*
 * nee -- NE3200 586
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
 * Date		Author		Changes
 * ------------------------------------------------------------------------
 * 27 Jan 94			Beta version
 *
 * 28 Jan 94	shiva		Firmware download problem fixed . Allocated
 *				Physical page size * 2 so that we get one
 *				physical page , copied the firmware code to
 *				that page and instructed the BMIC to do the
 *				transfer . nee_setup() takes the physical
 *				page size as the parameter.
 *
 * 29 Jan 94	shiva		Used the above logic for neeinstance structure
 *				so that the txbuffer,rxbuffer,statistics buffer
 *				all lie in the same physical page . Added a
 *				field to store the memory allocated ,in the
 *				instance structure 
 *
 * 30 Jan 94	shiva		Added code in neeattach() to use the fragmented
 *				memory allocated for instance structure for
 *				gld_mac structure . Added a field mac_overlayed
 *				in the neeinstance struct to know if the 
 *				gld_mac struct is overlayed on the fragmented 
 *				area .
 *				Added an internal function 
 *				nee_free_instance_memory() to free properly the
 *				macinfo and neep structures .
 *				Added code for nee_gstat() function
 *
 * 2  Feb 94	shiva		Added support for multicast . 
 *				nee_get_mcast_index() added to build the 
 *				multicast table ( data structure mcast_info 
 *				defined). Multicast routine gets the index of
 *				multicast address in the table and on/off's the
 *				the flag byte .
 *
 * 8  Feb 94	shiva		Sent the FCR
 *
 * 15 Feb 94	shiva		Sometimes the EISA interrupt enable register has
 *				0x3 set which prevents the enable . so clearing 
 *				it before enable .
 *
 * 25 Feb 94	** Code changes after review by Sunsoft **
 *		** Code put in to SCCS **
 *				Ver 1.2
 *		shiva		Mutex lock is provided for probe to avoid
 *				conflicts between instances
 *
 *				In Probe auto configuration is made by
 *				probing through all the slots
 *
 *				macinfo gldm_irq_index is assigned with 
 *				appropriate index (one which is matching
 *				with configured interrupt)
 *
 *				Entry point nee_saddr is written by down
 *				loading the firmware in override mode
 *				and the ether address is set. Function
 *				nee_firmware_download is written to do that.
 *
 *				Watchdog timer is removed from send and
 *				interrupt entrypoints since we have a way
 *				to know the completion of the previous 
 *				transmit
 * 26 Feb 94	shiva		Ver 1.3
 *				code included to get the IRQ from config
 *				port . changed the length in kmem_free
 *				when intr property is not specified .
 *				Added function nee_create_irq_prop() .
 *
 * 28 Feb 94	shiva		Ver 1.4
 *				In nee_setup() changed irq value checking
 *				from 0 to -1 , since nee_irq_intr_handler()
 *				sets irq value if the board was not
 *				configured with an irq .
 *
 * 04 Mar 94	shiva		Ver 1.5
 *				Added code in the send and interrupt routine to
 *				improve transmit performance .
 *
 *				Allocated space for 2 TCBs and also implemented
 *				packet queing . This will enable us not to
 *				send a 1 to GLD not to requeue the packet .
 *
 *				nee_copy_data_to_buffer() defined since we need
 *				to fill the TCB from both send and interrupt 
 *				routine .
 *
 *				Macros NEE_ADD_IN_Q() and NEE_REMOVE_FROM_Q()
 *				added . Variables to maintain Q were added in
 *				neeinstance structure .
 *
 * 15 Mar 94	shiva		Ver 1.6
 *				Using ddi_iopb_alloc() to get contiguous 
 *				physical memory . Using compiler directive 
 *				-DNEE_KMEM_ZALLOC_FOR_BMIC we can use the
 *				old verison
 *
 * 18 Mar 94	raja		Ver 1.7
 *				Included a flag tx_state in instance structure
 *				to monitor the tx status . Removed code that
 *				checks the VALID TCB MBOX for transmitter free .
 *
 *				While loop in ISR .
 */

#pragma ident "@(#)nee.c	1.7 	94/03/19 SMI"

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
#include <sys/nvm.h>
#include <sys/eisarom.h>
#include <sys/nee.h>

/*
 *  Declarations and Module Linkage
 */

static char ident[] = "NE3200 586";
/* Uncomment if DL_TPR device
static int Use_Group_Addr = 0;
*/

/* used for debugging */
# ifdef NEEDEBUG
int	needebug = 0;
# endif

/* Required system entry points */
static	neeidentify(dev_info_t *);
static	needevinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	neeprobe(dev_info_t *);
static	neeattach(dev_info_t *, ddi_attach_cmd_t);
static	needetach(dev_info_t *, ddi_detach_cmd_t);

/* Required driver entry points for GLD */
int	nee_reset(gld_mac_info_t *);
int	nee_start_board(gld_mac_info_t *);
int	nee_stop_board(gld_mac_info_t *);
int	nee_saddr(gld_mac_info_t *);
int	nee_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
int	nee_prom(gld_mac_info_t *, int);
int	nee_gstat(gld_mac_info_t *);
int	nee_send(gld_mac_info_t *, mblk_t *);
u_int	neeintr(gld_mac_info_t *);

/* Driver internal entry points */
int	nee_setup(dev_info_t *, gld_mac_info_t *);	/* Ver 1.2 */
int	nee_getp(gld_mac_info_t *);
void	nee_free_instance_memory(gld_mac_info_t *,struct neeinstance *) ;
int	nee_get_mcast_index(gld_mac_info_t *,unchar *) ;
int	nee_firmware_download(gld_mac_info_t *, unchar ) ; /* Ver 1.2 */
int	nee_create_irq_prop(dev_info_t * ,NVM_SLOTINFO *) ;  /* Ver 1.3 */
int	nee_copy_data_to_buffer(unchar , mblk_t *,struct neeinstance *) ;
void	nee_start_tx( int , struct neeinstance * , int ) ;	/* Ver 1.7 */

DEPENDS_ON_GLD;		/* this forces misc/gld to load -- DO NOT REMOVE */

/* Standard Streams initialization */

static struct module_info minfo = {
	NEEIDNUM, "nee", 0, INFPSZ, NEEHIWAT, NEELOWAT
};

static struct qinit rinit = {	/* read queues */
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
};

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, NULL, NULL, NULL, &minfo, NULL
};

struct streamtab neeinfo = {&rinit, &winit, NULL, NULL};

/* Standard Module linkage initialization for a Streams driver */

extern struct mod_ops mod_driverops;

static 	struct cb_ops cb_neeops = {
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
	&neeinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

struct dev_ops neeops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	needevinfo,		/* devo_getinfo */
	neeidentify,		/* devo_identify */
	neeprobe,		/* devo_probe */
	neeattach,		/* devo_attach */
	needetach,		/* devo_detach */
	nodev,			/* devo_reset */
	&cb_neeops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,			/* short description */
	&neeops			/* driver specific ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

/* Mutex to protect probe data */
static kmutex_t 	nee_probe_lock;		/* Ver 1.2 */

/*	external declarations */
extern	int	neeFirmwareSize ;
extern	unchar	neeFirmware[] ;
extern	int	eisa_nvm(caddr_t, int, int);

/*
 * Module specific entry points
 */

int
_init(void)
{
	mutex_init(&nee_probe_lock, 
	           "NE3200 MP probe protection", 
	           MUTEX_DRIVER,
	           NULL);		/* Ver 1.2 */
        return mod_install(&modlinkage);
}

int
_fini(void)
{
	mutex_destroy(&nee_probe_lock);	/* Ver 1.2 */
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

neeidentify(dev_info_t *devinfo)
{
	if (strcmp(ddi_get_name(devinfo), "nee") == 0)
		return DDI_IDENTIFIED;
	else
		return DDI_NOT_IDENTIFIED;
}

/* getinfo(9E) -- Get device driver information */
/*ARGSUSED*/
needevinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
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

neeprobe(dev_info_t *devinfo)
{
	caddr_t	eisa_data ;
	int	slot ;
	NVM_SLOTINFO	*nvm ;
	unchar	probe_status = DDI_PROBE_FAILURE ;
			/* set to DDI_PROBE_SUCCESS if board found */
	int	port ;					/* Ver 1.2 */
	static	int 	lastslot = - 1 ;		/* Ver 1.2 */
			/* 
			 * using -1 since threre could be an onboard
			 * implementation of the board ( slot 0 ) 
			 */

# ifdef NEEDEBUG
	if (needebug & NEEDDI)
		cmn_err(CE_CONT, "neeprobe(0x%x)\n", devinfo);
# endif

	/* The following code till the end is Ver 1.2 */

	/* alloacate memory to store the eisa config information */
	eisa_data = ( caddr_t ) kmem_zalloc( MAX_EISABUF , KM_NOSLEEP ) ;
	if ( eisa_data == NULL )
		return ( DDI_PROBE_FAILURE ) ;

	mutex_enter(&nee_probe_lock);

	/*
	 * Loop through all EISA slots, and check the ID string for
	 * this board.  Each time we come through this routine, we
	 * update the static 'lastslot' variable so we can start
	 * searching from the next one
	 */
	for (slot = lastslot + 1; slot < EISA_MAXSLOT; slot++) {
		if ( eisa_nvm( eisa_data , EISA_SLOT , slot ) == 0 )
			continue;

		nvm = (NVM_SLOTINFO *)( eisa_data + sizeof( short ) ) ;
		if ( (nvm->boardid[0]  == BOARD_ID_1 &&
		     nvm->boardid[1]  == BOARD_ID_2 &&
		     nvm->boardid[2]  == BOARD_ID_3 &&
		     nvm->boardid[3]  == BOARD_ID_4) ||
		     (nvm->boardid[0]  == BOARD_ID_5 &&
		     nvm->boardid[1]  == BOARD_ID_6 &&
		     nvm->boardid[2]  == BOARD_ID_7 &&
		     nvm->boardid[3]  == BOARD_ID_8) ) {
			/* verify that the hardware is in the system */
			port = slot * 0x1000 + NEE_PORT_START ;

			if ((inb(port + 0) == BOARD_ID_1 &&
			    inb(port + 1) == BOARD_ID_2 &&
			    inb(port + 2) == BOARD_ID_3 &&
			    inb(port + 3) == BOARD_ID_4) ||
			    (inb(port + 0) == BOARD_ID_5 &&
			    inb(port + 1) == BOARD_ID_6 &&
			    inb(port + 2) == BOARD_ID_7 &&
			    inb(port + 3) == BOARD_ID_8)) {

				(void)ddi_prop_create(DDI_DEV_T_NONE,
				                      devinfo,
				                      DDI_PROP_CANSLEEP,
				                      "ioaddr",
				                      (caddr_t)&port,
				                      sizeof(port));

				nee_create_irq_prop( devinfo ,nvm ) ;

				/* save the port base in private */
				ddi_set_driver_private(devinfo,
				                       (caddr_t)(slot << 12));
				probe_status = DDI_PROBE_SUCCESS ;
				break;
			}
		}
	}

	lastslot = slot;	/* remember the slot found */
	kmem_free(eisa_data, MAX_EISABUF);
	mutex_exit(&nee_probe_lock);

	return ( probe_status ) ;
}

int
nee_create_irq_prop( dev_info_t *devinfo ,NVM_SLOTINFO *nvm )
{

	int	i ;
	NVM_FUNCINFO	*nvm_funcinfo = ( NVM_FUNCINFO *)(nvm +1) ;
	int	irq  = -1 ;

	/*
	 * The following comment was added in Ver 1.4
	 *
	 * The tech spec for NE3200 specifies that port 0x?800 ( ? - slot )
 	 * is a configuration register ( 1 byte ) which should be written
	 * by the EISA Configuration utility only . The following is an
	 * extract from NE3200 BOARD THEORY OF OPERATION ( Date March 20 1990 )
	 * section 7.2.3 CONFIGURATION REFERENCE 
	 *
	 * 'IT IS VERY IMPORTANT TO UNDERSTAND THAT THIS I/O PORT IS WRITE
	 * ONLY . ANY READ FROM I/O ADDRESS 0x?800 WILL CORRUPT ANY PREVIOUS
	 * DATA WRITTEN TO IT . THIS SHOULD ONLY BE WRITTEN BY EISA 
	 * CONFIGURATION UTILITY . THE HOST CAN READ CONFIGURATION INFORMATION
	 * USING SYSTEM BIOS CALLS'
	 *
	 * Because of the above reason we are using the NVM data to get the
	 * IRQ configured
	 */

	for (i = 0; i < nvm->functions; i++ ) {
                if (nvm_funcinfo->fib.irq) {
                        irq = nvm_funcinfo->un.r.irq[0].line;
                        break;
                }
		nvm_funcinfo ++ ;
        }

	/* 
	 * Create the IRQ property anyway ( even if irq = -1 ) and
	 * let the user of the property validate the value 
	 */
	(void)ddi_prop_create(DDI_DEV_T_NONE,
	                      devinfo,
	                      DDI_PROP_CANSLEEP,
	                      "irq",
	                      (caddr_t)&irq,
	                      sizeof(irq));
	return (0);
}

/*
 *  attach(9E) -- Attach a device to the system
 *
 *  Called once for each board successfully probed.
 *  This also downloads the firmware to the board
 */

neeattach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */
	struct neeinstance *neep;		/* Our private device info */
	int	i, len, pgsiz ;

# ifdef NEE_KMEM_ZALLOC_FOR_BMIC
	unchar	*neepstart ;
	int	free_mem_top;
	int 	free_mem_bottom; 
	unchar 	*top,	*bottom;
# endif

# ifdef NEEDEBUG
	if (needebug & NEEDDI)
		cmn_err(CE_CONT, "neeattach(0x%x)\n", devinfo);
# endif

	if (cmd != DDI_ATTACH)
		return DDI_FAILURE;

	/* get the physical page size */
	pgsiz = ddi_ptob ( devinfo, 1L);

	/* compute number of bits to shift to get the physical address */
	for ( i=pgsiz, len = 0; i > 1 ; len ++ )
		i >>= 1;

# ifdef NEE_KMEM_ZALLOC_FOR_BMIC
	/* Allocate 2 physical pages */
	neepstart = (unchar *)kmem_zalloc(pgsiz*2,KM_NOSLEEP) ;
	if (neepstart == NULL)
		return DDI_FAILURE;

	/* Get memory address starting at page boundary for neeinstance */
	neep = (struct neeinstance *)
	       ( neepstart + ( pgsiz - ((ulong)neepstart % pgsiz) ) ) ;
	neep->pgmask = pgsiz - 1;
	neep->pgshft = len ;

	neep->phypagestart = neepstart ;	/* save the memory to free */

	/* 
	 * Check if free memory from the chunk allocated can be used for 
	 * gld_mac_info_t structure . If so use it and make a note of it
	 * in neeinstance structure . If not available allocate memory for
	 * gld_mac_info_t structure
	 */

	free_mem_top = NEE_FREEMEM_TOP;
	free_mem_bottom = NEE_FREEMEM_BOTTOM;
	top = (unchar *) neepstart;
	bottom = (unchar *) (((unchar *)neep) + pgsiz);
	neep->tcbptr[NEE_NORMAL_TCB] = &neep->tcb_normal;
	neep->mac_overlayed = 1 ;	/* overlayed */
	neep->tx_curr = 0 ;
	neep->tx_buf_flag[0] = neep->tx_buf_flag[1] = 0 ;

	/* Ver 1.5 Try to fit a TCB in the memory allocated */
	if( TCB_SIZ < free_mem_top){
		neep->tcbptr[NEE_LOOKAHEAD_TCB] = (struct TCB *)top;
		free_mem_top -= TCB_SIZ;
		top += TCB_SIZ;
	} else
	if( TCB_SIZ < free_mem_bottom){
		neep->tcbptr[NEE_LOOKAHEAD_TCB] = (struct TCB *)bottom;
		free_mem_bottom -= TCB_SIZ;
		bottom += TCB_SIZ;
	}
	else{
		kmem_free( neepstart , pgsiz*2 ) ;
		return ( DDI_FAILURE ) ;
	}

	/* Ver 1.5 Try to fit in the GLD macinfo if memory available */
	if ( sizeof(gld_mac_info_t) < free_mem_top )
		macinfo = ( gld_mac_info_t *)top ;
	else
	if ( sizeof(gld_mac_info_t) < free_mem_bottom )
		macinfo = ( gld_mac_info_t *)bottom;
	else{
		/* sizeof gld_mac_info_t is too big to fit in frag area */
		neep->mac_overlayed = 0 ;	/* not overlayed */
		macinfo = ( gld_mac_info_t *)
		          kmem_zalloc( sizeof(gld_mac_info_t), KM_NOSLEEP) ;
		if ( macinfo == NULL ){
			kmem_free( neepstart , pgsiz*2 ) ;
			return ( DDI_FAILURE ) ;
		}
	}
# else
	/* Ver 1.6 using iopb_alloc() to get contiguous free memory */
	/* RSS - CHECK */

	if ( ddi_iopb_alloc( devinfo,
	                     NULL,	/* use default dma_limits */
	                     sizeof(gld_mac_info_t)+sizeof(struct neeinstance),
	                     (caddr_t *)&macinfo ) == DDI_FAILURE )
		return ( DDI_FAILURE ) ;

	/* Ver 1.10 */
	bzero( (caddr_t)macinfo ,
	       sizeof(gld_mac_info_t) + sizeof(struct neeinstance));

	neep = (struct neeinstance * )(macinfo + 1) ;

	neep->tcbptr[NEE_NORMAL_TCB] = &neep->tcb_normal;
	neep->tcbptr[NEE_LOOKAHEAD_TCB] = &neep->tcb_lookahead;
	neep->tx_curr = 0 ;
	neep->pgmask = pgsiz - 1;
	neep->pgshft = len ;
	neep->tx_buf_flag[0] = neep->tx_buf_flag[1] = 0 ;
# endif 
			
	/*  Initialize our private fields in macinfo and neeinstance */
	macinfo->gldm_private = (caddr_t)neep;
	macinfo->gldm_port = (long)ddi_get_driver_private( devinfo ) ;
	macinfo->gldm_state = NEE_IDLE;
	macinfo->gldm_flags = 0;
	macinfo->gldm_devinfo = devinfo ;	/* Ver 1.6 */

	/*
	 *  Initialize pointers to device specific functions which will be
	 *  used by the generic layer.
	 */

	macinfo->gldm_reset   = nee_reset;
	macinfo->gldm_start   = nee_start_board;
	macinfo->gldm_stop    = nee_stop_board;
	macinfo->gldm_saddr   = nee_saddr;
	macinfo->gldm_sdmulti = nee_dlsdmult;
	macinfo->gldm_prom    = nee_prom;
	macinfo->gldm_gstat   = nee_gstat;
	macinfo->gldm_send    = nee_send;
	macinfo->gldm_intr    = neeintr;
	macinfo->gldm_ioctl   = NULL; 

	/*
	 *  Initialize board characteristics needed by the generic layer.
	 */
	macinfo->gldm_ident = ident;
	macinfo->gldm_type = DL_ETHER;
	macinfo->gldm_minpkt = 0;		/* assumes we pad ourselves */
	macinfo->gldm_maxpkt = NEEMAXPKT;
	macinfo->gldm_addrlen = ETHERADDRL;
	macinfo->gldm_saplen = -2;

	/* setup physical address for firmware specific data */
	neep->updt_parm.node_adr_ptr   = NEE_KVTOP( macinfo->gldm_vendor) ;
	neep->updt_parm.recv_frame_ptr = NEE_KVTOP( &neep->rcb );
	neep->updt_parm.stat_area_ptr  = NEE_KVTOP( &neep->stats );
	neep->updt_parm.multicast_listptr  = NEE_KVTOP( neep->mcasttbl );

	/* 	This is specific to the firmware 	*/
	neep->updt_parm.num_cust_statcnt   = 10;

	/* No address in table to set */
	neep->updt_parm.multicast_cnt      = 0;
	neep->count = neep->read_ptr = neep->write_ptr = 0;

	if (nee_setup( devinfo, macinfo ) == DDI_FAILURE ){
# ifdef NEE_KMEM_ZALLOC_FOR_BMIC
		nee_free_instance_memory( macinfo , neep ) ;
# else
		ddi_iopb_free( (caddr_t )macinfo ) ;
# endif
		return ( DDI_FAILURE ) ;
	}

	/* From now on saddr() routine will use the address set in macaddr */
	neep->updt_parm.node_adr_ptr = 
		NEE_KVTOP( macinfo->gldm_macaddr ) ; 	/* Ver 1.2 */

	/***** set the connector/media type if it can be determined *****/
	macinfo->gldm_media = GLDM_UNKNOWN;

	bcopy((caddr_t)gldbroadcastaddr,
		(caddr_t)macinfo->gldm_broadcast, ETHERADDRL);
	bcopy((caddr_t)macinfo->gldm_vendor,
		(caddr_t)macinfo->gldm_macaddr, ETHERADDRL);

	neep->tx_state = NEE_TX_FREE ;	/* Ver 1.7 */

	/*
	 *  Register ourselves with the GLD interface
	 */
	if (gld_register(devinfo, "nee", macinfo) == DDI_SUCCESS) 
		/*
		 * nee_setup() has done the job of init'ing the board and
		 * setting the ethernet address on board . so we do not
		 * do anything here 
		 */
		return DDI_SUCCESS;
	
# ifdef NEE_KMEM_ZALLOC_FOR_BMIC
	nee_free_instance_memory( macinfo , neep ) ;
# else
	ddi_iopb_free( (caddr_t)macinfo ) ;
# endif
	return DDI_FAILURE;
}

int
nee_setup(dev_info_t *devinfo , gld_mac_info_t *macinfo )
{
	struct intrprop {
		int spl;
		int irq;
	} *intrprop;		/* Ver 1.2 */
	int	len;		/* Ver 1.2 */
	int	i ;
	int	irq ;	/* Ver 1.2 */

	if ( (ddi_getlongprop(DDI_DEV_T_ANY, 
	                      devinfo, 
	                      DDI_PROP_DONTPASS,
	                      "intr", 
	                      (caddr_t)&intrprop, 
	                      &len)) != DDI_PROP_SUCCESS ) {
		kmem_free(intrprop, len );
		return DDI_FAILURE;
	}
      
	len /= sizeof(struct intrprop);

	/* Get the IRQ configured for the board */
        irq = ddi_getprop(DDI_DEV_T_ANY, 
	                  devinfo,
                          DDI_PROP_DONTPASS, "irq", 0);

	for (i = 0; i < len; i++)
		if (irq == intrprop[i].irq)
			break;

	kmem_free(intrprop, len * sizeof(struct intrprop));

	if (i >= len || irq == -1 ) 	/* Ver 1.4 */
		/* cannot find a matching IRQ the .con file */
		return DDI_FAILURE;

	macinfo->gldm_irq_index = i;

	return ( nee_firmware_download( macinfo , NEE_NO_OVERRIDE ) ) ;
}

/* This will perform the firmware download and return the firmware status */
/* Extracted from neesetup() of 1.1 	Ver 1.2 */

int
nee_firmware_download( gld_mac_info_t *macinfo,unchar nodeflag )
{
	unchar  *firmware ;
	int     slot = macinfo->gldm_port ;
	struct 	neeinstance *neep;     /* Our private device info */
	int     i, j;
# ifdef NEE_KMEM_ZALLOC_FOR_BMIC
	int	pagesize ;
	unchar  *fptrstart ;
# endif

	neep = (struct neeinstance *)macinfo->gldm_private ;
# ifdef NEE_KMEM_ZALLOC_FOR_BMIC
	pagesize = neep->pgmask + 1 ;	/* Ver 1.2 */
# endif

	/* give a hardware reset to the board */
	outl (NEE_RESETPORT, (long)1);

	/* clear the first four mail box registers */
	outb ( VALID_RCB_MBOX  , 0x00 );                
	outb ( ABEND_IDLE_MBOX , 0x00 );                
	outb ( UPDT_PARAM_MBOX , 0x00 );                
	outb ( VALID_TCB_MBOX  , 0x00 );                
	
	delay ( 1 * drv_usectohz( 1 ) ) ;
        
	/* pull out of reset */
	outl (NEE_RESETPORT, (long)0);

	for ( i = 0 ; i < 0x7fff8 ; i ++) 
		if ( ( j = inb ( VALID_RCB_MBOX ) ) )
			break ;

	/* Adapter never responded */
	if ( i >= 0x7fff8 || j != 0x80 ) {
# ifdef NEEDEBUG
		printf("NE3200 Adapter never responded\n");
# endif
		return (DDI_FAILURE);
	}

	/*
	 * the adapter is responding and the time has come to download the
	 * firmware code to the 186
	 */
	outw( ABEND_IDLE_MBOX ,(ushort)neeFirmwareSize);  /* mbox 1 */

# ifdef NEE_KMEM_ZALLOC_FOR_BMIC
	/*
	 * The array neeFirmware may not be put in contiguous pages in memory
	 * which makes the BMIC to transfer the f/w wrongly . We allaocte
	 * (pagesize*2) bytes and we are bound to get atleast one contiguous 
	 * page , we copy the array to the page and program the BMIC 
	 */
	fptrstart = firmware = (unchar *)kmem_zalloc( ( pagesize * 2 ) ,
	                                              KM_NOSLEEP ) ;
	if ( firmware == NULL ){
# ifdef NEEDEBUG
		printf("No Memory for NE3200 firmware for download\n") ;
# endif
		return ( DDI_FAILURE ) ;
	}

	/* put the firmware at a physical page start */
	firmware += ( pagesize - ((ulong)firmware % pagesize) ) ;
# else
	if ( ddi_iopb_alloc( macinfo->gldm_devinfo,
	                     NULL,	/* use default dma_limits */
	                     neeFirmwareSize , 
	                     (caddr_t *)&firmware ) == DDI_FAILURE )
		return ( DDI_FAILURE ) ;
# endif

	bcopy ( (caddr_t)neeFirmware,(caddr_t)firmware,neeFirmwareSize ) ;

	/* Fill in the source address here (mbox 4) */
	outl( BMTCB_XMIT_MBOX , NEE_KVTOP ( (caddr_t)firmware)); 

	/* Fill in the destination address here (mbox 8) */        
	outl( RCB_PHYADR_MBOX ,0x0);

	/* Ask the adapter to download the code  */
	outb( VALID_RCB_MBOX, 0 );

	/* Wait for the firmware to come up  */
	for ( i = 0 ; i < 0x7fff8 ; i ++) 
		if (  inl ( PARAM_MBOX )  == NEE_FWAREOK_STATE ) 
			break ;

	/* Free the memory alloacated for the firmware download */
# ifdef NEE_KMEM_ZALLOC_FOR_BMIC
	kmem_free( fptrstart , (pagesize * 2) ) ;
# else
	ddi_iopb_free( (caddr_t)firmware ) ;
# endif

	if ( i >= 0x7fff8 ) {
# ifdef NEEDEBUG
		printf( "NE3200 firmware not running\n");
# endif
		return DDI_FAILURE ;
	}

	/*      
	 * Get the phy addr of the updt_parm struct and load it into
	 * the BMIC param mail box register 
	 */
	outl( PARAM_MBOX , (long) NEE_KVTOP ( (caddr_t)&neep->updt_parm) ) ;
	
	/*
	 * Write into update param mbox to set no node override/override
	 * information .
	 * NO OVERRIDE 
	 *      This makes the firmware to read the ethernet address from
	 *      the PROM. The firmware will automatically update the 
	 *      gldm_vendor to the ethernet address of the board.
	 * OVERRIDE
	 *	This will cause the f/w to set the node address from the
	 * 	gldm_macaddr array 
	 */
	outb (UPDT_PARAM_MBOX, nodeflag );	/* Ver 1.2 */

	/* Wait for the adapter to be in ready state */
	for ( i = 0xffffff ; i > 0 ; i --)  {
		switch ( inb ( ABEND_IDLE_MBOX ) )  {
			case NEE_ABEND_STATE :
# ifdef NEEDEBUG
				printf("NE3200 firmware in Abend state\n");
# endif
				return DDI_FAILURE ;

			case NEE_FWARE_STATE :
				return DDI_SUCCESS ;
		}
	}

	/* the f/w might have hung */
# ifdef NEEDEBUG
	printf("NE3200 firmware in Hung state\n");
# endif
	return ( DDI_FAILURE ) ;
}

/*  detach(9E) -- Detach a device from the system */

needetach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */
	int slot ;
# ifdef NEE_KMEM_ZALLOC_FOR_BMIC
	struct neeinstance *neep;		/* Our private device info */
# endif

# ifdef NEEDEBUG
	if (needebug & NEEDDI)
		cmn_err(CE_CONT, "needetach(0x%x)\n", devinfo);
# endif

	if (cmd != DDI_DETACH) 
		return DDI_FAILURE;

	/* Get the driver private (gld_mac_info_t) structure */
	macinfo = (gld_mac_info_t *)ddi_get_driver_private(devinfo);
	slot = macinfo->gldm_port ;


	/* put the f/w into endless sleep ... */
	outb( NEE_EISAINTRENB, 0x0 );

	/* clear the doorbell bits 	  */
	outb( NEE_DOORBELLENB, 0x0 );

	/* hard kill the f/w */
	outb( ABEND_IDLE_MBOX ,  NEE_ABEND_ADAP_CMD ) ;

	/*
	 *  Unregister ourselves from the GLD interface
	 */
	if (gld_unregister(macinfo) == DDI_SUCCESS) {
# ifdef NEE_KMEM_ZALLOC_FOR_BMIC
		neep = (struct neeinstance *)(macinfo->gldm_private);
		nee_free_instance_memory( macinfo , neep ) ;
# else
		ddi_iopb_free( (caddr_t)macinfo ) ;
# endif
		return DDI_SUCCESS;
	}
	return DDI_FAILURE;
}

/*
 *  GLD Entry Points
 */

/*
 *  nee_reset() -- reset the board to initial state; restore the machine
 *  address afterwards.
 */

int
nee_reset( gld_mac_info_t *macinfo)
{
	struct neeinstance *neep =		/* Our private device info */
		(struct neeinstance *)macinfo->gldm_private;
	int	i ;
	int	slot = macinfo->gldm_port ;

# ifdef NEEDEBUG
	if (needebug & NEETRACE)
		cmn_err(CE_CONT, "nee_reset(0x%x)\n", macinfo);
# endif

	/* Tell the firmware to initilaise the 586 again */
	outb( UPDT_PARAM_MBOX , NEE_RESET_ADAP_CMD ) ;

	for ( i = 0 ; i < 0x00ffffff ; i ++ )
		if ( inb( UPDT_PARAM_MBOX ) == 0 )
			break ;

	macinfo->gldm_state = NEE_IDLE ;
	neep->tx_state = NEE_TX_FREE ;
	/* no need for init_board() and saddr() routines */
	return (0);
}

/*
 *  nee_start_board() -- start the board receiving and allow transmits.
 */

nee_start_board(gld_mac_info_t *macinfo)
{
	int	i ;
	int	slot = macinfo->gldm_port ;

# ifdef NEEDEBUG
	if (needebug & NEETRACE)
		cmn_err(CE_CONT, "nee_start_board(0x%x)\n", macinfo);
# endif

	/*
	 * Sending the RESET command to the f/w will reset the 586 ,inits
	 * the required registers and also sets the physical address on the
	 * board and restarts the board again
	 */

	/* make sure not to go into f/w ABEND loop */
	outb ( ABEND_IDLE_MBOX , 0x0 ) ;
	outb ( UPDT_PARAM_MBOX , NEE_RESET_ADAP_CMD ) ;

	/* 	wait for the adapter to get reset */
	for ( i = 0 ; i < 0x00ffffff ; i ++ )
		if ( inb( ABEND_IDLE_MBOX ) == NEE_FWARE_STATE )
			break ;

	/* enable doorbell and system interrupts */
	outb ( NEE_DOORBELLENB, NEE_DOORBELL_BITS);

	outb ( NEE_EISAINTRENB, 0x0);
	outb ( NEE_EISAINTRENB, NEE_EISAINTRENB_BITS);
	return (0);
}

/*
 *  nee_stop_board() -- stop board receiving
 */

nee_stop_board(gld_mac_info_t *macinfo)
{
	int	slot = macinfo->gldm_port ;
	int	i ;

# ifdef NEEDEBUG
	if (needebug & NEETRACE)
		cmn_err(CE_CONT, "nee_stop_board(0x%x)\n", macinfo);
# endif

	/*
	 * The ABEND_IDLE mailbox is used to terminate the f/w or to
	 * temporarily put it in idle state . The f/w could be brought back
	 * to action by writing a RESET command to this mailbox 
	 */

	/* put the f/w in to idle state */
	outb( ABEND_IDLE_MBOX , NEE_IDLE_ADAP_CMD ) ;

	/* 	wait for the adapter to get idle */
	for ( i = 0 ; i < 0x00ffffff ; i ++ )
		if ( inb( ABEND_IDLE_MBOX ) == NEE_IDLE_STATE )
			break ;

	/* disable doorbell and system interrupts */
	outb ( NEE_DOORBELLENB, 0x0);
	outb ( NEE_EISAINTRENB, 0x0);
	return (0);
}

/*
 *  nee_saddr() -- set the physical network address on the board
 */

int
nee_saddr(gld_mac_info_t *macinfo)
{
# ifdef NEEDEBUG
	if (needebug & NEETRACE)
		cmn_err(CE_CONT, "nee_saddr(0x%x)\n", macinfo);
# endif


	/* The code till the end is added in  Ver 1.2 */

	/*
	 * we call the nee_firmware_download() function with NEE_OVERRIDE
	 * flag which will set the node address from gldm_macaddr array
	 * The physical address of gldm_macaddr[] is loaded in 
	 * neep->updt_parm.node_adr_ptr in the attach() . The Multicast
	 * addresses that were previously enabled/disabled are not 
	 * affected by this operation .
	 * SIDE EFFECTS :
	 * 	# All packets received will be lost and if the board was in
	 *	the process of sending a packet , the download will terminate
	 *	the transmit .
	 * 	# Also if the firmware download fails for some reason the 
	 *	the board will not be operational after this call
	 */

	if ( nee_firmware_download( macinfo , NEE_OVERRIDE ) == DDI_FAILURE )
# ifdef NEEDEBUG
		printf("NE3200 saddr: f/w download fail\n") ;
# else
		;
# endif 
	return (0);
}

/*
 *  nee_dlsdmult() -- set (enable) or disable a multicast address
 *
 *  Program the hardware to enable/disable the multicast address
 *  in "mcast".  Enable if "op" is non-zero, disable if zero.
 */

int
nee_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	struct neeinstance *neep =		/* Our private device info */
		(struct neeinstance *)macinfo->gldm_private;
	int	slot = macinfo->gldm_port ;
	int	i ;
	
# ifdef NEEDEBUG
	if (needebug & NEETRACE)
		cmn_err(CE_CONT, "nee_dlsdmult(0x%x)", macinfo) ;
# endif

	i = nee_get_mcast_index( macinfo , mcast->ether_addr_octet ) ;

	if ( i == -1 )
		return (DDI_FAILURE);

	neep->mcasttbl[i].flag = (ushort)op ;	/* Ver 1.2 */

	outb( UPDT_PARAM_MBOX , NEE_UPDT_MCAST_CMD ) ;

	for ( i = 0 ; i < 0x00ffffff ; i ++ )
		if ( !inb( UPDT_PARAM_MBOX ) )
			break ;

# ifdef NEEDEBUG
	if ( i >= 0x00fffff )
		printf("NE3200 Multicast operation not over\n") ;
# endif
	return (DDI_SUCCESS);
}

/*
 * nee_prom() -- set or reset promiscuous mode on the board
 *
 *  Program the hardware to enable/disable promiscuous mode.
 *  Enable if "on" is non-zero, disable if zero.
 */

int
nee_prom(gld_mac_info_t *macinfo, int on)
{
	int	slot = macinfo->gldm_port ;
	int	i ;

# ifdef NEEDEBUG
	if (needebug & NEETRACE)
		cmn_err(CE_CONT, "nee_prom(0x%x)\n", macinfo ) ;
# endif

	if ( on )
		outb( UPDT_PARAM_MBOX , NEE_SET_PROM_CMD ) ;
	else
		outb( UPDT_PARAM_MBOX , NEE_CLEAR_PROM_CMD ) ;

	for ( i = 0 ; i < 0x00ffffff ; i ++ )
		if ( !inb( UPDT_PARAM_MBOX ) )
			break ;

# ifdef NEEDEBUG
	if ( i >= 0x00fffff )
		printf("NE3200 Promiscuous operation not over\n") ;
# endif
	return (0);
}

/*
 * nee_gstat() -- update statistics
 *
 *  GLD calls this routine just before it reads the driver's statistics
 *  structure.  If your board maintains statistics, this is the time to
 *  read them in and update the values in the structure.  If the driver
 *  maintains statistics continuously, this routine need do nothing.
 */

int
nee_gstat(gld_mac_info_t *macinfo)
{
	struct neeinstance *neep =		/* Our private device info */
		(struct neeinstance *)macinfo->gldm_private;

# ifdef NEEDEBUG
	if (needebug & NEETRACE)
		cmn_err(CE_CONT, "nee_gstat(0x%x)\n", macinfo);
# endif

	/*
	 * The STAT structure is periodically loaded by the firmware 
	 * if it encounters any errors on tx/rx . We just copy the statistics
	 * from the STAT structure to the GLD statistics structure
	 */
	macinfo->gldm_stats.glds_xmtretry = neep->stats.tx_retryfail ;
	macinfo->gldm_stats.glds_nocarrier = neep->stats.tx_carrier_lost ;
	macinfo->gldm_stats.glds_xmtlatecoll = neep->stats.tx_max_collision ;
	macinfo->gldm_stats.glds_overflow = neep->stats.rx_overflow ;
	macinfo->gldm_stats.glds_crc = neep->stats.rx_crc ;
	macinfo->gldm_stats.glds_defer = neep->stats.tx_defers ;
	macinfo->gldm_stats.glds_collisions =  neep->stats.tx_num_collision ;
	macinfo->gldm_stats.glds_errrcv = neep->stats.rx_toobig +
	                                  neep->stats.rx_toosmall +
	                                  neep->stats.rx_dmaover +
	                                  neep->stats.rx_noEOF ;
	macinfo->gldm_stats.glds_errxmt =  neep->stats.tx_clr_to_send +
	                                   neep->stats.tx_underrun ;
	return (0);
}

/*
 *  nee_send() -- send a packet
 *
 *  Called when a packet is ready to be transmitted. A pointer to an
 *  M_DATA message that contains the packet is passed to this routine.
 *  The complete LLC header is contained in the message's first message
 *  block, and the remainder of the packet is contained within
 *  additional M_DATA message blocks linked to the first message block.
 *
 *  This routine may NOT free the packet.
 */

int nee_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	struct neeinstance *neep =		/* Our private device info */
			(struct neeinstance *)macinfo->gldm_private;
	int	slot = macinfo->gldm_port ;
	int	i ;
	unchar	tx_buf ;

# ifdef NEEDEBUG
	if (needebug & NEESEND)
		cmn_err(CE_CONT, "nee_send(0x%x)\n", macinfo);
# endif

	/* Ver 1.5 	Routine modified for better performance */

	/* check if both transmit buffers are full */
	if ( neep->tx_buf_flag[0] && neep->tx_buf_flag[1] ){
		NEE_ADD_IN_Q( i ) ;
		if(! i ){
		 	macinfo->gldm_stats.glds_defer ++ ;
			return 1 ;
		}
		else 
			return 0;
	}

	/* get the index of the free buffer */
	tx_buf = neep->tx_buf_flag[ 0 ] ;

	if ( neep->tx_state == NEE_TX_FREE &&
 	     neep->tx_buf_flag[ tx_buf ^ 0x01 ] )
		nee_start_tx( slot , neep , tx_buf ^ 0x01 ) ;

	/* copy data to the buffer that is free */
	if ( nee_copy_data_to_buffer( tx_buf , mp ,neep) <= 0 )
		return 0 ;

	/* if the transmitter is free then send the packet */
	if ( neep->tx_state == NEE_TX_FREE  &&
	     neep->tx_buf_flag[ tx_buf ] )	/* Ver 1.7 */
		nee_start_tx( slot , neep , tx_buf ) ;
	/* 
	 * Ver 1.2 The watchdog timer was removed since we have a way of
	 * finding if the previous transmit was ok ( inb() from VALID_TCB_MBOX)
	 */
	return 0;		/* successful transmit attempt */
}

int
nee_copy_data_to_buffer( unchar tx_buf , register mblk_t *mp ,
			 register struct neeinstance *neep)
{
	register struct	TCB *curtcb ;
	int		    len , length ;

	curtcb = neep->tcbptr[tx_buf];

	len = length = 0 ;
	while ( mp ){
		len = mp->b_wptr - mp->b_rptr ;
		if ( len <= 0 ){
			mp = mp->b_cont;
			continue ;
		}

		if ( ( length + len )  > GLD_EHDR_SIZE + NEEMAXPKT ){
			return ( 0 ) ;	/* Ver 1.2 */
					/* This makes the GLD not to resend
					 * this packet again */
		}

		bcopy( (caddr_t)mp->b_rptr , 
		       (caddr_t)(&curtcb->txbuf[length]), len ) ;

		length += len ;
		mp = mp->b_cont;
	}

	if ( length <= 0 )
		return ( 0 ) ;		/* Ver 1.2 */
					/* Do not resend this packet */

	if (length < NEEMINSEND)	/* pad packet length if needed */
		length = NEEMINSEND;

	neep->tx_buf_flag[ tx_buf ] =  1 ;
	curtcb->frame_len = curtcb->data_len = (ushort)length ;
	curtcb->num_frags = 0 ;	/* no fragments */

	return ( length ) ;
}


/*
 *  neeintr() -- interrupt from board to inform us that a receive or
 *  transmit has completed.
 */

u_int
neeintr(register gld_mac_info_t *macinfo)
{
	register struct neeinstance *neep =	/* Our private device info */
		(struct neeinstance *)macinfo->gldm_private;
	register int	slot = macinfo->gldm_port ;
	mblk_t 	*mp;
	int	tx_buf ;

	if ( inb( NEE_DOORBELL ) == 0 )
		return(DDI_INTR_UNCLAIMED) ;

	macinfo->gldm_stats.glds_intr++;

	/* disable the interrupts */
	outb ( NEE_EISAINTRENB, 0x0);


	/* Ver 1.7 */
	while(inb(NEE_DOORBELL) & ( NEE_PKT_TX_INT | NEE_PKT_RX_INT)){
		/* Ver 1.5 Code added for better transmit using lookahead */
		if ( inb(NEE_DOORBELL) & NEE_PKT_TX_INT ){  
						/* packet transmitted */
			/* clear transmit status */
			outb ( NEE_DOORBELL , NEE_PKT_TX_INT_CLEAR ) ;

			/*
	 		 * Version 1.2
	 		 * Removed timer specific code
	 		 */
			neep->tx_buf_flag[neep->tx_curr] = 0 ;

			tx_buf = neep->tx_curr ^ 0x01 ;

			neep->tx_state = NEE_TX_FREE;

			/* Transmit the next buffer if full */
			if ( neep->tx_buf_flag[tx_buf] )
				nee_start_tx( slot , neep , tx_buf ) ;

			tx_buf ^= 0x01 ;

			if ( !neep->tx_buf_flag[tx_buf] ){
				NEE_REMOVE_FROM_Q( mp ) ;
				if ( mp ) {
					nee_copy_data_to_buffer(tx_buf , 
					                        mp ,neep);
					freemsg(mp) ;
				}
			}

			if ( neep->tx_state == NEE_TX_FREE &&
		       	     neep->tx_buf_flag[tx_buf] )
				nee_start_tx( slot , neep , tx_buf ) ;
		}


		if ( inb(NEE_DOORBELL) & NEE_PKT_RX_INT )	
						/* packet received */
			(void)nee_getp( macinfo ) ;
	}

	/* check if the f/w has set more info for us */
	/* enable the interrupts */
	outb ( NEE_EISAINTRENB, 0x0);	/* clear it before enable */
	outb ( NEE_EISAINTRENB, NEE_EISAINTRENB_BITS);
	return DDI_INTR_CLAIMED;	/* Indicate it was our interrupt */
}

int
nee_getp(register gld_mac_info_t *macinfo)
{
	register struct neeinstance *neep =	/* Our private device info */
		(struct neeinstance *)macinfo->gldm_private;
	int 	slot = macinfo->gldm_port ;
	register mblk_t	*mesgptr ;

# ifdef NEEDEBUG
	if (needebug & NEEINT)
		cmn_err(CE_CONT, "nee_getp(0x%x)\n", macinfo);
# endif

	if ( neep->rcb.recv_frame_size < NEEMINSEND || 
	     neep->rcb.recv_frame_size > (NEEMAXPKT + GLD_EHDR_SIZE) ){
		/* skip packet */
		macinfo->gldm_stats.glds_short ++ ;
	}
	else{
		/*
		 * allocate a STREAMS message block ( mblk_t )  
		 * to send it upstream
		 */
		if ((mesgptr = allocb( neep->rcb.recv_frame_size,0)) == NULL ){
# ifdef NEEDEBUG
printf("allocb failure\n") ;
# endif
			macinfo->gldm_stats.glds_norcvbuf ++ ;
		}
		else{
			bcopy( (caddr_t)neep->rcb.rxbuf , 
			       (caddr_t)mesgptr->b_wptr  , 
			       neep->rcb.recv_frame_size ) ;
			mesgptr->b_wptr += neep->rcb.recv_frame_size;
			gld_recv( macinfo , mesgptr ) ;
		}
	}
 
	outb ( NEE_DOORBELL , NEE_PKT_RX_INT_CLEAR ) ;
	outb ( VALID_RCB_MBOX , NEE_SINGLE_RCB ) ;
	return (0);
}

/*ARGSUSED*/
void
nee_free_instance_memory(gld_mac_info_t *macinfo,struct neeinstance *neep)
{
# ifdef NEE_KMEM_ZALLOC_FOR_BMIC
	/* check if macinfo is overlayed */
	if ( neep->mac_overlayed == 0 ) 	/* not overlayed */
		kmem_free( macinfo , sizeof( gld_mac_info_t ) );

	/* free the neeinstance page anyway */
	kmem_free( neep->phypagestart , (neep->pgmask+1)*2 ) ;
# endif
}

/*
 * nee_get_mcast_index() 
 * This returns the index of the mcast address from the table if found
 * if not found it adds it to the list .
 * Returns -1 if the table is full
 */

int
nee_get_mcast_index( gld_mac_info_t *macinfo , unchar *addr )
{
	struct neeinstance *neep =		/* Our private device info */
		(struct neeinstance *)macinfo->gldm_private;
	int	i ;

	/* is the table full */
	if ( neep->updt_parm.multicast_cnt >= NEE_MAXMCAST_ADDR )
		return ( -1 ) ;	/* false table full */

	/* check if the address is already present in the table */
	for ( i = 0 ; i < neep->updt_parm.multicast_cnt ; i ++ )
		if ( bcmp((caddr_t)addr,
		          (caddr_t)neep->mcasttbl[i].mcast_eaddr,
		          ETHERADDRL) == 0 )
			break ;

	/* if not present in the table then add it */
	if ( i == neep->updt_parm.multicast_cnt ){
		neep->updt_parm.multicast_cnt ++ ;
		bcopy((caddr_t)addr,
		      (caddr_t)neep->mcasttbl[i].mcast_eaddr,
		      ETHERADDRL ) ;
		neep->mcasttbl[i].flag = (ushort)0 ;
	}

	return ( i ) ;	/* return the index */
}

void
nee_start_tx( int slot , struct neeinstance *neep , int tx_buf )
{
	ushort	length ;

	neep->tx_state = NEE_TX_BUSY;
	neep->tx_curr = tx_buf ;
	length = neep->tcbptr[neep->tx_curr]->data_len + 6 ;
	outw( BMTCB_XMIT_MBOX , SWAP_WORD( length) ) ;
	outl( PARAM_MBOX , NEE_KVTOP (neep->tcbptr[neep->tx_curr]) ) ;
	outb( VALID_TCB_MBOX , 1 ) ; 	/* no of fragments + 1 */
}
