/*
 * Copyright (c) 1993-94 Sun Microsystems, Inc.
 */

/*
 * VIEW OPTIONS : Set tabstop = 4
 *
 * NAME
 *		aic.c Version 1.0, for Solaris 2.4, dated July 11, 1994
 *
 *
 * SYNOPIS
 *		Solaris 2.4 for x86 Host Bus Adapter driver for AHA-1522A SCSI
 *		controller.
 *		Uses the Hardware Interface Module for AIC-6X60 based adapters.
 *
 * DESCRIPTION
 *		The 'aic' driver implements a dynamcially loadable, multithread safe
 *  HBA driver for AHA-1522A SCSI controller. The driver handles the SCSI
 *  bus phases for upto seven devices connected to this controller and
 *	supports target disconnect for increased bus utilization. The driver
 *	uses Programmed I/O for its data transfers and supports Synchronous
 *	data transfer (If target supports it) for increased transfer rates.
 *	It also supports Auto Request Sense for handling SCSI check conditions.
 *	Upto two controllers can operate simultaneously and can be configured
 *  through entries in the aic.conf file.
 *
 *		The AHA-1522A SCSI HBA is based on AIC-6360 SCSI controller from
 *	Adaptec Inc. The AIC-6360 maps itself into the AT I/O address space
 *	and handles all timing and control on the SCSI bus. The driver uses
 *	the Hardware Interface Module from Adaptec Inc. for handling the
 *	SCSI phases and interacting with the adapter hardware. The	HIM implements
 *	a SCSI Host Adapter for Adapters based on the AIC-6X60 based SCSI
 *  controllers. For more details refer to aic(7).
 *
 * CAVEATS AND NOTES
 *		The driver supports upto a maximum of two host adapters. This is 
 *	because the board supports only two I/O base addresses viz. 0x340 and
 *	0x140. Moreover the adapter BIOS supports booting only at I/O address
 * 	0x340. There is code to support DMA here which is not fully implemented.
 *
 * MODIFICATION HISTORY
 */

/*
 *
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

#pragma ident   "@(#)aic.c 1.10     96/03/26 SMI"

#ifndef lint
static char     sccsid[] = "@(#)aic.c Ver. 1.0 Jul 11,1994 Copyright 1994 Sun Microsystems";
#endif

#include <sys/scsi/scsi.h>
#include <sys/ddi_impldefs.h>
#include <sys/dktp/hba.h>
#include <sys/dktp/aic/aic.h>
#include <sys/debug.h>

/* FLAGS FOR SELECTIVE DEBUGGING */

#ifdef AIC_DEBUG
#define	DINIT 		0x0001
#define DDMA		0x0002
#define DDATAIN 	0x0004
#define DDATAOUT 	0x0008
#define DAIC_TIMEOUT	0x0010
int aic_debug = 0;
#endif

/*
 * External SCSA Interface
 */

static int aic_transport(struct scsi_address *ap, struct scsi_pkt *pktp);
static int aic_abort(struct scsi_address *ap, struct scsi_pkt *pkt);
static int aic_reset(struct scsi_address *ap, int level);
static int aic_getcap(struct scsi_address *ap, char *cap, int tgtonly);
static int aic_setcap(struct scsi_address *ap, char *cap, int value,
				    int tgtonly);

#ifdef AIC_DMA
static struct scsi_pkt *aic_en_dmaget(struct scsi_pkt *pkt, opaque_t dmatoken,  
					int (*callback)(), caddr_t arg);
static void aic_en_dmafree(struct scsi_pkt *pkt);
#else
static struct scsi_pkt *aic_dmaget(struct scsi_pkt *pkt, opaque_t dmatoken,  
					int (*callback)(), caddr_t arg);
static void aic_dmafree(struct scsi_address *ap, struct scsi_pkt *pkt);
#endif

static struct scsi_pkt *aic_pktalloc(struct scsi_address *ap, int cmdlen, 
					int statuslen, int tgtlen, int (*callback)(), caddr_t arg);
static void aic_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt);

static void aic_childprop(dev_info_t *pdip, dev_info_t *cdip);

/*
 * New Prototypes
 */

static int aic_tran_tgt_init(dev_info_t *, dev_info_t *, scsi_hba_tran_t *, 
							struct scsi_device *);
static int aic_tran_tgt_probe(struct scsi_device *, int(*)());
static void aic_tran_tgt_free(dev_info_t *, dev_info_t *, scsi_hba_tran_t *, 
							struct scsi_device *);
static struct scsi_pkt *aic_tran_init_pkt(struct scsi_address *ap, 
					struct scsi_pkt *pkt, struct buf *bp, int cmdlen, 
					int statuslen, int tgtlen, int flags, 
					int (*callback)(caddr_t), caddr_t arg);
static void aic_tran_destroy_pkt(struct scsi_address *ap,
							struct scsi_pkt *pkt);
static void aic_sync_pkt(struct scsi_address *ap, struct scsi_pkt *pkt);
/*** 
static int aic_getinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
***/

/*
 * Local Function Prototypes
 */

static u_int aic_intr(caddr_t arg);		/* The driver interrupt routine */
void aic_pollret(SCB *);
void aic_timeout(caddr_t arg);
static u_int aic_dummy_intr(caddr_t arg);	/* dummy interrupt routine */
static void set_controller_options(ushort scsi_options, HACB *hacb);
int aic_comp_callback(struct scsi_pkt *);	/* Has to be done away with */
void aic_dummy();

/* Functions used by HIM  */
INT memcmp(const VOID *ptr1, const VOID *ptr2, USHORT size) ;
VOID * memset(VOID *ptr,UCHAR val,USHORT size) ;
VOID * memcpy(VOID *dest,const VOID *src,USHORT size); 

/*
 * LOCAL STATIC DATA
 */

/*
 * Soft state pointers (Per Controller)
 */

void *aic_state_p;			/* pointer for aic structure */
void *aic_state_mutex_p;	/* Pointer for the controller mutex */

static int aic_cb_id = 0;		/* Callback listid */

/* Auto configuration entry points */

static int aic_getinfo(dev_info_t *dip, ddi_info_cmd_t cmd, void *arg, 
		void **resultp);
static int aic_identify(dev_info_t *dev);
static int aic_probe(dev_info_t *);
static int aic_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int aic_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);

static struct dev_ops	aic_ops = {
	DEVO_REV,					/* devo_rev, */
	0,							/* refcnt  */
	aic_getinfo,				/* info */
	aic_identify,				/* identify */
	aic_probe,					/* probe */
	aic_attach,					/* attach */
	aic_detach,					/* detach */
	nodev,						/* reset */
	(struct cb_ops *)0,			/* driver operations */
	NULL						/* bus operations */
};

/* 
 * Make the system load these modules whenever this driver loads.  This
 * is required for constructing the set of modules needed for boot; they
 * must all be loaded before anything initializes.  Just use this
 * line as-is in your driver.
 */
char _depends_on[] = "misc/scsi";

/*
 * This is the loadable module wrapper.
 */
#include <sys/modctl.h>

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,				/* Type of module. This one is a driver */
	"Adaptec AIC SCSI HBA",		/* Name of the module. */
	&aic_ops,					/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

static ddi_dma_lim_t aic_dma_lim = {
	0,		/* address low				*/
	0x00ffffff,	/* address high			*/
	0,		/* counter max				*/
	1,		/* burstsize 				*/
	DMA_UNIT_8,	/* minimum xfer			*/
	0,		/* dma speed				*/
	DMALIM_VER0,	/* version			*/
	0x00ffffff,	/* address register		*/
	0x00007fff,	/* counter register		*/
	512,		/* sector size			*/
	AIC_MAX_DMA_SEGS,/* scatter/gather list length		*/
	0xffffffff	/* request size			*/ 
};

/*
 * Name			: _init()
 * Purpose		: Initializes a loadable scsi module. It calls mod_install to
 *				  to dynamically add a loadable module. A call to scsi_hba_init
 *				  initialises the scsi hba module. The driver softstate
 *				  structures are initialised here.
 * Called from	: Kernel
 * Arguments	: None
 * Returns		: The value returned by mod_install()
 * Side effects	: None
 */

int
_init(void)
{
	int ret_val;

	/* Initialize the Soft state variables here for efficiency */

	if( (ret_val = scsi_hba_init(&modlinkage)) !=0 ) {
			return (ret_val);
	}
	if ( (ret_val = ddi_soft_state_init(&aic_state_p, sizeof(struct aic), 
		  NUM_CTRLRS)) != 0 ){
		scsi_hba_fini(&modlinkage);
		return ret_val;
	}
	if ( (ret_val = ddi_soft_state_init(&aic_state_mutex_p, 
		  sizeof(kmutex_t), NUM_CTRLRS)) != 0 ){
		scsi_hba_fini(&modlinkage);
		return ret_val;
	}
	if ( (ret_val = mod_install(&modlinkage)) != 0 )
	{
		scsi_hba_fini(&modlinkage);
		ddi_soft_state_fini( &aic_state_p );
		ddi_soft_state_fini( &aic_state_mutex_p );
	}
	return ret_val;
}

/*
 * Name			: _fini()
 * Purpose		: Prepares a loadable module for unloading. Calls mod_remove
 *				  to dynamically unload a module and the driver softstate 
 *				  structures are released.
 * Called from	: Kernel
 * Arguments	: None
 * Returns		: The value returned by mod_remove()
 * Side effects	: None
 */

int
_fini(void)
{
	int ret_val;

	if ( (ret_val = mod_remove(&modlinkage)) == 0 ){
		scsi_hba_fini(&modlinkage);
		ddi_soft_state_fini( &aic_state_p );
		ddi_soft_state_fini( &aic_state_mutex_p );
	}
	return ( ret_val );
}

/*
 * Name			: _info()
 * Purpose		: Report driver module status information
 * Called from	: Kernel
 * Arguments	: modinfo *modinfop
 * Returns		: Whatever is returned by mod_info()
 * Side effects	: None
 */

int
_info(struct modinfo *modinfop)
{
	return(mod_info(&modlinkage, modinfop));
}

/* Autoconfiguration routines */

/* 
 * identify(9E).  See if driver matches this dev_info node.
 * Return DDI_IDENTIFIED if ddi_get_name(devi) matches your
 * name, otherwise return DDI_NOT_IDENTIFIED.
 */

/*
 * Name			: aic_identify()
 * Purpose		: Verify that this is the `aic' driver
 * Called from	: Kernel
 * Arguments	: dev_info_t *devi
 * Returns		: DDI_IDENTIFIED or DDI_NOT_IDENTIFIED
 * Side effects	: None
 */

static int
aic_identify(dev_info_t *devi)
{
	char *dname = ddi_get_name(devi);

	if (strcmp(dname, "aic") == 0) 
		return(DDI_IDENTIFIED);
	else 
		return(DDI_NOT_IDENTIFIED);
}

/* 
 * probe(9E).  Examine hardware to see if HBA device is actually present.  
 * Do no permanent allocations or permanent settings of device state,
 * as probe may be called more than once (for each entry in .conf file).
 * Return DDI_PROBE_SUCCESS if device is present and operable, 
 * else return DDI_PROBE_FAILURE.
 */
/*
 * Name		: aic_probe()
 * Purpose	: Check whether the HBA hardware is really present or not
 * Called from	: Kernel
 * Arguments	: register dev_info_t *devi
 * Returns	: DDI_PROBE_FAILURE or DDI_PROBE_SUCCESS
 * Side effects	: None
 */

static int
aic_probe(register dev_info_t *devi)
{
	BOOLEAN retval;
	AIC6X60_REG *aic_reg;
	unsigned short bpaddr;
	unsigned short i;
	int irqIndex;
	HACB *aic_hacb;

	/* Get baseport address from conf file */
	bpaddr = ddi_getprop(DDI_DEV_T_ANY,devi,DDI_PROP_DONTPASS,
								"ioaddr", DEFAULT_BASE_ADDRESS);

#ifdef AIC_DEBUG
	if(aic_debug & DINIT)
		PRINT0("value of base addr 0x%x\n",bpaddr);
#endif

	/* 
	 * Allocate memory for the controller's I/O registers 
	 * This memory is deallocated before probe returns.
	 */
	aic_reg = kmem_zalloc(sizeof(*aic_reg),KM_NOSLEEP);
	if( !aic_reg )
	{
#ifdef AIC_DEBUG
		PRINT0("aic_probe : Alloc of aic_reg for ctrlr failed\n");
#endif
		return DDI_PROBE_FAILURE;
	}
	aic_hacb = kmem_zalloc(sizeof(HACB),KM_NOSLEEP);

	if( !aic_hacb )
	{
		kmem_free(aic_reg,sizeof(*aic_reg));
#ifdef AIC_DEBUG
		PRINT0( "Allocation of HACB structure for AHA-1522A at %d failed\n",
				bpaddr);
#endif
		return DDI_PROBE_FAILURE;
	}
	aic_hacb->baseAddress = aic_reg;

	for(i=0; i<NUM_AIC6360_REGS; i++)
	{
		aic_hacb->baseAddress->ioPort[i]=bpaddr+i;
	}

	aic_hacb->length=sizeof(HACB);

	/* check for presence of AHA-1522A */
	retval=HIM6X60FindAdapter(aic_hacb->baseAddress);

	if(retval) 		/* Adapter found  */
	{
#ifdef AIC_DEBUG
		if(aic_debug & DINIT)
			cmn_err(CE_CONT,"AHA-1522 type Adapter at 0x%x\n", 
				bpaddr);
#endif

		if ( HIM6X60GetConfiguration(aic_hacb) )
		{
			INT ret_prop;
			caddr_t prop_buf;
			INT	prop_length;

#ifdef AIC_DEBUG
	if( aic_debug & DINIT )
			printHacb(aic_hacb);		/* print the HACB members */
#endif
			/*
			 * Determine the index of the "intr" property in the conf file,
			 * corresponding to the adapter's IRQ. This is done only to
			 * check if the Adapter is set at a valid IRQ
			 */
			ret_prop = ddi_prop_op(DDI_DEV_T_ANY,devi,PROP_LEN_AND_VAL_ALLOC,
						DDI_PROP_DONTPASS, "intr", (caddr_t)&prop_buf, 
						&prop_length);
			if (ret_prop != DDI_PROP_SUCCESS)
			{
#ifdef AIC_DEBUG
				PRINT0( "aic_probe: failed to get 'intr' property\n");
#endif
				kmem_free(aic_hacb->baseAddress,sizeof(*aic_hacb->baseAddress));
				kmem_free(aic_hacb,sizeof(HACB));
				return(DDI_PROBE_FAILURE);
			}
			/* Determine the index of the "intr" property in conf file */
			irqIndex = 
				aic_get_intr_index((int *)prop_buf, aic_hacb->IRQ, 
					prop_length >> 2);

			/* Free up the prop_buf allocated in ddi_prop_op() */
			kmem_free((void *)prop_buf, (size_t)prop_length);

			if (irqIndex == ILLEGAL_VALUE)
			{
#ifdef AIC_DEBUG
				PRINT0( "Unsupported IRQ specified in conf file.\n");
#endif
				kmem_free(aic_hacb->baseAddress,sizeof(*aic_hacb->baseAddress));
				kmem_free(aic_hacb,sizeof(HACB));
				return(DDI_PROBE_FAILURE);
			}
		}
		else
		{
#ifdef AIC_DEBUG
			PRINT0( "aic_probe: HIM6X60GetConfiguration() Failure \n");
#endif
			kmem_free(aic_hacb->baseAddress,sizeof(*aic_hacb->baseAddress));
			kmem_free(aic_hacb,sizeof(HACB));
			return(DDI_PROBE_FAILURE);
		}

		if ( !HIM6X60Initialize(aic_hacb) )
		{
#ifdef AIC_DEBUG
			PRINT0( "aic_probe: HIM6X60Initialize() failed\n");
#endif
			kmem_free(aic_hacb->baseAddress,sizeof(*aic_hacb->baseAddress));
			kmem_free(aic_hacb,sizeof(HACB));
			return(DDI_PROBE_FAILURE);
		}
#ifdef AIC_DEBUG
	if( aic_debug & DINIT )
	{
		PRINT0("Probe for aic succeeded at 0x%x\n", bpaddr);
		PRINT0("Successfully initialized\n");
	}
#endif
/*		Turn off interrupts on Board						*/
		HIM6X60DisableINT(aic_hacb);
		kmem_free(aic_hacb->baseAddress,sizeof(*(aic_hacb->baseAddress)));
		kmem_free(aic_hacb,sizeof(HACB));
		return (DDI_PROBE_SUCCESS);
	}
	else 		/* Adapter not found */
	{
#ifdef AIC_DEBUG
		PRINT0( "aic_probe: aic probe failed\n");
#endif
		kmem_free(aic_hacb->baseAddress,sizeof(*(aic_hacb->baseAddress)));
		kmem_free(aic_hacb,sizeof(HACB));
		return(DDI_PROBE_FAILURE);
	}
}
/* 
 * attach(9E).  Set up all device state and allocate data structures, 
 * mutexes, condition variables, etc. for device operation.  Set mt-attr
 * property for driver to indicate MT-safety.  Add interrupts needed.
 * Return DDI_SUCCESS if device is ready, else return DDI_FAILURE.
 */

/*
 * Name			: aic_attach()
 * Purpose		: Allocate driver resources for the driver instance,
 *		  		  install interrupt service routine
 * Called from	: Kernel
 * Arguments	: dev_info_t *devi, ddi_attach_cmd_t cmd
 * Returns		: DDI_SUCCESS or DDI_FAILURE
 * Side effects	:  None
 */

static int
aic_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	register struct aic 	*aicp;
	int 						mt;
	int							irqIndex;
	int							instance;
	int ret_prop;
	caddr_t prop_buf;
	int	prop_length;
	int reg_index;
	kmutex_t	*aic_ctrlr_mutex;
	AIC6X60_REG *aic_reg;
	HACB *hacb;
	unsigned short bpaddr;
	scsi_hba_tran_t	*hba_tran;

	struct dev_ops *devopsp;
	struct bus_ops *busopsp;

	if (cmd != DDI_ATTACH)
	{
#ifdef AIC_DEBUG
		PRINT0( "aic_attach called with wrong cmd 0x%x\n", cmd);
#endif
		return (DDI_FAILURE);
	}

	/* Get baseport address from conf file */
	bpaddr = ddi_getprop(DDI_DEV_T_ANY,devi,DDI_PROP_DONTPASS,
								"ioaddr", DEFAULT_BASE_ADDRESS);

	/* allocate memory for the controller's I/O registers */
	aic_reg = kmem_zalloc(sizeof(*aic_reg),KM_SLEEP);
	if( !aic_reg )
	{
#ifdef AIC_DEBUG
		PRINT0("Alloc of aic_reg for ctrlr failed\n");
#endif
		return DDI_FAILURE;
	}
	hacb = kmem_zalloc(sizeof(HACB),KM_SLEEP);

	if( !hacb )
	{
		kmem_free(aic_reg,sizeof(*aic_reg));
#ifdef AIC_DEBUG
		PRINT0( "Allocation of HACB structure for AHA-1522A at %d failed\n",
				bpaddr);
#endif
		return DDI_FAILURE;
	}
	hacb->baseAddress = aic_reg;

	/*
	 * Set up the Controller I/O registers in the HACB structure
	 */

	for(reg_index=0; reg_index<NUM_AIC6360_REGS; reg_index++)
	{
		hacb->baseAddress->ioPort[reg_index]=bpaddr+reg_index;
	}
	/* Initialize the length of the HACB structure */
	hacb->length=sizeof(HACB);

	if ( !HIM6X60GetConfiguration(hacb) )
	{
#ifdef AIC_DEBUG
			PRINT0( "aic_attach: HIM6X60GetConfiguration() failed\n");
#endif
		kmem_free(hacb->baseAddress,sizeof(*hacb->baseAddress));
		kmem_free(hacb,sizeof(HACB));
		return(DDI_FAILURE);
	}

	/*
	 * Initialize the hacb structure
	 */
	if ( !HIM6X60Initialize(hacb) )
	{
#ifdef AIC_DEBUG
			PRINT0( "aic_attach: HIM6X60Initialize() failed\n");
#endif
		kmem_free(hacb->baseAddress,sizeof(*hacb->baseAddress));
		kmem_free(hacb,sizeof(HACB));
		return(DDI_FAILURE);
	}
/*	Turn off interrupts which have just been turned on by Initialize	*/
	HIM6X60DisableINT(hacb);

	/* Make a aic instance for this HBA */
	instance = ddi_get_instance(devi);
	if ( ddi_soft_state_zalloc( aic_state_p, instance) != DDI_SUCCESS )
	{
#ifdef AIC_DEBUG
		PRINT0("aic_attach:Allocation of softstate structure failed");
#endif
		kmem_free(hacb->baseAddress,sizeof(*hacb->baseAddress));
		kmem_free(hacb,sizeof(HACB));
		return DDI_FAILURE;
	}

	/*
	 * Get aicinfop so that we can access the right controller
	 */
	aicp = (struct aic *) ddi_get_soft_state( aic_state_p, instance );

	/* save the dev_info pointer of the HBA */
	aicp->aic_dip = devi;

	/* Let the system know we're MT-safe */

	mt = D_MP;
	(void)ddi_prop_create(DDI_DEV_T_NONE, devi, DDI_PROP_CANSLEEP,
	    "mt-attr", (caddr_t)&mt, sizeof(mt));

	/* ENABLE AIC6360 ENHANCED FEATURES MODE */
	if((INPUT(portB) & 0x01) == 0)
	{
#ifdef AIC_DEBUG
	if ( aic_debug & DINIT )
		PRINT0("Enabling AIC6360 Enhanced features mode \n");
#endif
		OUTPUT(portB, (INPUT(portB) | 0x01));
	}

	/*
	 * Determine the index of the "intr" property in the conf file,
	 * corresponding to the adapter's IRQ.
	 */

	ret_prop = ddi_prop_op(DDI_DEV_T_ANY,devi,PROP_LEN_AND_VAL_ALLOC,
				DDI_PROP_DONTPASS, "intr", (caddr_t)&prop_buf, 
				&prop_length);
	if (ret_prop != DDI_PROP_SUCCESS)
	{
#ifdef AIC_DEBUG
		PRINT0( "aic_attach: failed to get 'intr' property\n");
#endif
		kmem_free(hacb->baseAddress,sizeof(*hacb->baseAddress));
		kmem_free(hacb,sizeof(HACB));
		return DDI_FAILURE;
	}

    /*
	 * Determine the index of the "intr" property in conf file 
	 */

	irqIndex = 
		aic_get_intr_index((int *)prop_buf, hacb->IRQ, 
			prop_length >> 2);
#ifdef AIC_DEBUG
	if( aic_debug & DINIT )
		PRINT0("Value of IRQ Index in conf file : 0x%x\n", irqIndex);
#endif

	/* Free up the prop_buf allocated in ddi_prop_op() */
	kmem_free((void *)prop_buf, (size_t)prop_length);
	
	if (irqIndex == ILLEGAL_VALUE)
	{
#ifdef AIC_DEBUG
		PRINT0( "Unsupported IRQ specified in conf file.\n");
#endif
		kmem_free(hacb->baseAddress,sizeof(*hacb->baseAddress));
		kmem_free(hacb,sizeof(HACB));
		return(DDI_FAILURE);
	}

	/* 
	 * Establish a dummy interrupt vector to get the iblock cookie
	 */

	if( ddi_add_intr(devi, irqIndex, &aicp->aic_iblock, 
                         (ddi_idevice_cookie_t *)0, aic_dummy_intr, 
                         (caddr_t)aicp) ) 
	{
#ifdef AIC_DEBUG
		PRINT0( "aic_attach: add_intr failed");
#endif
		ddi_soft_state_free( aic_state_p, instance );
		kmem_free(hacb->baseAddress,sizeof(*hacb->baseAddress));
		kmem_free(hacb,sizeof(HACB));
		return (DDI_FAILURE);
	}

	/* make up a controller mutex  */
	if ( ddi_soft_state_zalloc( aic_state_mutex_p, instance )  != DDI_SUCCESS )
	{
#ifdef AIC_DEBUG
		PRINT0( "aic_attach: add_intr failed");
#endif
		ddi_soft_state_free( aic_state_p, instance );
		kmem_free(hacb->baseAddress,sizeof(*hacb->baseAddress));
		kmem_free(hacb,sizeof(HACB));
		return (DDI_FAILURE);
	}


	/*
	 * Set up the controller mutex
	 */
	aic_ctrlr_mutex = (kmutex_t *)ddi_get_soft_state( aic_state_mutex_p,
													instance );
	mutex_init(aic_ctrlr_mutex, "aic controller mutex", 
	   	   MUTEX_DRIVER, (void *)aicp->aic_iblock);

	/* Remove the dummy interrupt vector setup */
	ddi_remove_intr( devi, irqIndex, aicp->aic_iblock );

	mutex_enter(aic_ctrlr_mutex);
	/* Establish the real interrupt handler */
	if( ddi_add_intr(devi, irqIndex, &aicp->aic_iblock, 
                         (ddi_idevice_cookie_t *)0, aic_intr, 
                         (caddr_t)aicp) ) 
	{
#ifdef AIC_DEBUG
		PRINT0( "aic_attach: add_intr failed");
#endif
		mutex_destroy(aic_ctrlr_mutex);
		ddi_soft_state_free( aic_state_p, instance );
		kmem_free(hacb->baseAddress,sizeof(*hacb->baseAddress));
		kmem_free(hacb,sizeof(HACB));
		return (DDI_FAILURE);
	}
	mutex_exit(aic_ctrlr_mutex);

	/*
	 * Allocate a HBA transport structure
	 */
	if( (hba_tran = scsi_hba_tran_alloc(devi, 0)) == NULL )
	{
		cmn_err(CE_WARN,"aic attach : scsi_hba_tran_alloc failed\n");
		mutex_destroy(aic_ctrlr_mutex);
		ddi_remove_intr( devi, irqIndex, aicp->aic_iblock );
		ddi_soft_state_free( aic_state_p, instance );
		kmem_free(hacb->baseAddress,sizeof(*hacb->baseAddress));
		kmem_free(hacb,sizeof(HACB));
		return (DDI_FAILURE);
	}


	/* Set up the transport struct */

	aicp->aic_tran = hba_tran;
	hba_tran->tran_hba_private 	= aicp;
	hba_tran->tran_tgt_private 	= NULL;
	hba_tran->tran_tgt_init 	= aic_tran_tgt_init;
	hba_tran->tran_tgt_probe 	= aic_tran_tgt_probe;
	hba_tran->tran_tgt_free		= aic_tran_tgt_free;
	hba_tran->tran_start 		= aic_transport;
	hba_tran->tran_abort		= aic_abort;
	hba_tran->tran_reset		= aic_reset;
	hba_tran->tran_getcap		= aic_getcap;
	hba_tran->tran_setcap		= aic_setcap;
	hba_tran->tran_init_pkt 	= aic_tran_init_pkt;
	hba_tran->tran_destroy_pkt 	= aic_tran_destroy_pkt;
	hba_tran->tran_sync_pkt 	= aic_sync_pkt;

#ifdef AIC_DMA
	hba_tran->tran_dmafree		= aic_en_dmafree;
#else
	hba_tran->tran_dmafree		= aic_dmafree;
#endif
	
	/*
	 * Call scsi_hba_attach to make available to the framework the  DMA
	 * limits of the device instance and the scsi_hba_tran structure.
	 */
	if( scsi_hba_attach(devi, &aic_dma_lim, hba_tran,
			0, NULL) != DDI_SUCCESS ){
		cmn_err(CE_WARN,"aic_attach : scsi_hba_attach failed\n");
		mutex_destroy(aic_ctrlr_mutex);
		ddi_remove_intr( devi, irqIndex, aicp->aic_iblock );
		ddi_soft_state_free( aic_state_p, instance );
		kmem_free(hacb->baseAddress,sizeof(*hacb->baseAddress));
		kmem_free(hacb,sizeof(HACB));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}
		

	/*  Zero the LUCB strcture before use */
	{
		int ii,jj;
		for ( ii=0; ii < NTGTS; ii++)
		{
			for ( jj=0; jj< NLUNS; jj++)
			{
				aicp->pt[ii*NLUNS + jj].tgt_state = AIC_TGT_DISCO;
				hacb->lucb[ii][jj].queuedScb = (SCB *) 0;
				hacb->lucb[ii][jj].activeScb = (SCB *) 0;
			}
		}
	}

/* 
 * Set up the DMA Options for DMA transfers, if supported
 */

#ifdef AIC_DMA

/* ENBLE DMA MODE OF AIC 6360 */

	if((INPUT(portB) & 0x40) == 0)
		OUTPUT(portB, (INPUT(portB) | 0x40));

	if( hacb->ac_u.ac_s.useDma )
	{
		int dma_prop;

	/* 
	 * Get the DMA channel
	 */
		dma_prop = ddi_getprop(DDI_DEV_T_ANY,devi,DDI_PROP_DONTPASS,
								"dmachan",0x00);
		hacb->dmaChannel = dma_prop;
#ifdef AIC_DEBUG
	if( aic_debug & DDMA )
		PRINT0( " DMA Channel : 0x%x\n", dma_prop );
#endif

	/* 
	 * Get the DMA Bus On Time
	 */
		dma_prop = ddi_getprop(DDI_DEV_T_ANY,devi,DDI_PROP_DONTPASS,
								"buson",0x05);
		hacb->dmaBusOnTime = dma_prop;
#ifdef AIC_DEBUG
	if( aic_debug & DDMA )
		PRINT0( " DMA Bus On Time : 0x%x\n", dma_prop );
#endif

	/* 
	 * Get the DMA Bus Off Time
	 */
		dma_prop = ddi_getprop(DDI_DEV_T_ANY,devi,DDI_PROP_DONTPASS,
								"busoff",0x09);
		hacb->dmaBusOffTime = dma_prop;
#ifdef AIC_DEBUG
	if( aic_debug & DDMA )
		PRINT0( " DMA Bus Off Time : 0x%x\n", dma_prop );
#endif

	}
#endif AIC_DMA

	/* 
	 * Our driver should retreive the parameters created by scsi_hba_attach
	 * and respect any settings for features 
	 */
	/**
	bpaddr = ddi_getprop(DDI_DEV_T_ANY,devi,DDI_PROP_DONTPASS,
								"scsi_options", DEFAULT_BASE_ADDRESS);
	set_controller_options(bpaddr,hacb);
	**/

	/*
	 * Initialise the hacb structure Queues
	 */
	hacb->eligibleScb = (SCB *) 0;
	hacb->deferredScb = (SCB *) 0;
	hacb->queueFreezeScb = (SCB *) 0;
	hacb->resetScb = (SCB *) 0;
	hacb->ne_u.ne_s.activeScb = (SCB *) 0;
	aicp->hacb = hacb;		/* per controller hacb */

	/* Print message about HBA being alive at address such-and-such */
	ddi_report_dev(devi);

	/* The dev_ops structure Pointer and the bus_operations structure */
	busopsp =(struct bus_ops *) DEVI(devi)->devi_ops->devo_bus_ops;
	devopsp =(struct dev_ops *) DEVI(devi)->devi_ops;
#ifdef AIC_DEBUG
	if(aic_debug & DINIT) {
		cmn_err(CE_CONT,"The Bus Ops struct Pointer : 0x%x\n", busopsp);
		cmn_err(CE_CONT,"The dev Ops struct pointer : 0x%x\n", devopsp);
	}
#endif

	HIM6X60EnableINT(hacb);
#ifdef AIC_DEBUG
	PRINT0("aic attach succeeded\n");
#endif

	return(DDI_SUCCESS);
}


/* 
 * detach(9E).  Remove all device allocations and system resources; 
 * disable device interrupts.
 * Return DDI_SUCCESS if done; DDI_FAILURE if there's a problem.
 */
/*
 * Name			: aic_detach()
 * Purpose		: Release resources allocated by aic_attach()
 * Called from	: Kernel
 * Arguments	: dev_info_t *devi, ddi_detach_cmd_t cmd
 * Returns		: DDI_SUCCESS or DDI_FAILURE
 * Side effects	: None
 */

/* ARGSUSED */
static int 
aic_detach(dev_info_t *devi, ddi_detach_cmd_t cmd)
{
	struct 	aic *aicp;
	HACB *hacb;
	int i,index;
	int irqIndex;
	int ret_prop;
	caddr_t prop_buf;
	int	prop_length;
	kmutex_t	*aic_ctrlr_mutex;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside aic_detach \n");
#endif

	switch (cmd) 
	{
		case DDI_DETACH:
		{
			scsi_hba_tran_t *tran;

			aicp = (struct aic *)ddi_get_driver_private(devi);
			if( aicp == NULL )
				return DDI_FAILURE;

			tran = aicp->aic_tran;
			hacb = (HACB *)aicp->hacb;

			index = ddi_get_instance( aicp->aic_dip );
			if ( !aicp )
				return(DDI_SUCCESS);

			if(aicp->num_targets_attached)
				return( DDI_FAILURE );

			/* Disable HBA interrupts */
			HIM6X60DisableINT(hacb);

			/*
			 * Determine the index of the "intr" property in the conf file,
			 * corresponding to the adapter's IRQ.
			 */
		
			ret_prop = ddi_prop_op(DDI_DEV_T_ANY,devi,PROP_LEN_AND_VAL_ALLOC,
						DDI_PROP_DONTPASS, "intr", (caddr_t)&prop_buf, 
						&prop_length);
			if (ret_prop != DDI_PROP_SUCCESS)
			{
#ifdef AIC_DEBUG
				PRINT0( "aic_attach: failed to get 'intr' property\n");
				PRINT0( "Disabling interrupt for AHA-1522A failed\n");
#endif
				return DDI_FAILURE;
			}
			/* Determine the index of the "intr" property in conf file */
			irqIndex = 
				aic_get_intr_index((int *)prop_buf, hacb->IRQ, 
					prop_length >> 2);

			/* Free up the prop_buf allocated in ddi_prop_op */
			kmem_free((void *)prop_buf, (size_t)prop_length);

			if (irqIndex == ILLEGAL_VALUE)
			{
#ifdef AIC_DEBUG
				PRINT0( "Unsupported IRQ specified in conf file.\n");
				PRINT0( "Disabling interrupt for AHA-1522A failed\n");
#endif
				return(DDI_FAILURE);
			}
			ddi_remove_intr(devi,irqIndex,aicp->aic_iblock);

			/* 
			 * Destroy mutex
			 */
			aic_ctrlr_mutex = (kmutex_t *)ddi_get_soft_state( aic_state_mutex_p, 																index);
			mutex_destroy(aic_ctrlr_mutex);
			ddi_soft_state_free( aic_state_mutex_p, index );
			kmem_free(hacb->baseAddress,sizeof(*(hacb->baseAddress)));
			kmem_free((caddr_t)hacb,sizeof(HACB));
			ddi_soft_state_free( aic_state_p , index );

	
			/* If driver creates any properties */
			if( scsi_hba_detach(devi) != DDI_SUCCESS )
			{
				cmn_err(CE_WARN,"aic: scsi_hba_detach failed\n");
			}
			ddi_prop_remove_all(devi);
			scsi_hba_tran_free(aicp->aic_tran);
			return (DDI_SUCCESS);
		}
		default:
			return (DDI_FAILURE); 	/* Changed from EINVAL */
	}
}

/****************** SCSI HBA service entry point routines *******************/

/* 
 * The (*tran_start) function.  Transport the command in pktp to the
 * addressed SCSI target device.  The command is not finished when this
 * returns, only sent to the target; aic_intr() will call 
 * (*pktp->pkt_comp)(pktp) when the target device has responded. If a
 * pkt is to be executed in the No disconnect mode, Then we wait till
 * the pkt is executed completely before returning from the routine.
 */

/*
 * Name			: aic_transport()
 * Purpose		: Do the necessary mapping, temporary buffer allocation
 *		 		  and queue the packet
 * Called from	: scsi_transport(9F)
 * Arguments	: struct scsi_pkt *pktp
 * Returns		: TRAN_ACCEPT or TRAN_BADPKT or TRAN_BUSY
 * Side effects	: bp associated with the pktp is mapped in if B_PAGEIO or
 *				  B_PHYS is set in bp->b_flags. Also set a per tgt cv if
 *				  no disconnect mode is set, which will be signalled in
 *			 	  in HIM6X60CompleteSCB().
 */

static int 
aic_transport(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	struct aic *aicp;
	volatile struct aic_scsi_cmd *cmd;
	volatile SCB *scb;
	struct buf *bp; 
	int target,lun;
	int index,i;
	struct per_tgt_info	*pt_ptr;
	kmutex_t *aic_ctrlr_mutex;

	cmd = PKT2CMD(pktp);
	aicp = ADDR2AIC(ap);
	target = ap->a_target;
	lun = ap->a_lun;

	ASSERT(aicp);

	index = ddi_get_instance(aicp->aic_dip);

	aic_ctrlr_mutex = (kmutex_t *)ddi_get_soft_state( aic_state_mutex_p, index);

	if( !cmd ) 
	{
#ifdef AIC_DEBUG
		PRINT0("Command not allocated\n");
#endif
		return (TRAN_BADPKT);
	}

	scb = (SCB *)(&cmd->scb);
	if( !scb )
	{
#ifdef AIC_DEBUG
		PRINT0("SCB Not Allocated for packet\n");
#endif
		return (TRAN_BADPKT);
	}

	/* clear the pkt fields as the packet may have been reallocated */
	pktp->pkt_state = 0L;
	pktp->pkt_statistics = 0L;
	pktp->pkt_reason = 0;

	scb->cdb = pktp->pkt_cdbp;
/*	XXX should xfer be set???; perhaps b_bcount is 0 */
	scb->flags = SCB_DISABLE_DMA | SCB_DATA_IN | SCB_DATA_OUT;

/* The NOINTR packets are also executed in No disconnect mode since interrupts
 * are not enabled
 */
	if ((pktp->pkt_flags & FLAG_NODISCON) ||
		(pktp->pkt_flags & FLAG_NOINTR)) 		/* No DISCONNECT */
	{
#ifdef AIC_DEBUG
	if(aic_debug & (DDATAIN | DDATAOUT))
		PRINT0("Packet called in No Disconnect mode\n");
#endif
		scb->flags |=  SCB_DISABLE_DISCONNECT ;
	}

	/* If ARQ is agreed upon between HBA and target drivers */
	if (AP2PTGTP(ap)->tgt_state & AIC_TGT_ARQ) 	
	{
		scb->senseData = 
			(UCHAR *)&((struct scsi_arq_status *)pktp->pkt_scbp)->sts_sensedata;
		scb->senseDataLength = sizeof(struct scsi_extended_sense);
	}
	else
	{
		scb->flags |= SCB_DISABLE_AUTOSENSE;
		scb->senseDataLength = 0;
	}

	/* Set SCB function for command execution */
	scb->function =  SCB_EXECUTE;

	/* Initialize other relevant fields of scb */
	pktp->pkt_resid = scb->dataLength;	 /* Set pkt_resid ONLY here */
	scb->osRequestBlock = (void *)pktp;
	scb->targetID = (UCHAR) pktp->pkt_address.a_target;
	scb->lun = pktp->pkt_address.a_lun;
	scb->scbStatus = 0x00;
	scb->chain = (SCB *) 0;
	scb->linkedScb = (SCB *) 0;
/*	XXX why is targetStatus set to an impossible value */
	scb->targetStatus = 0xFF;
	scb->queueTag = 0;
	scb->scsiBus = 0;

#ifdef AIC_TIMEOUT
	/* reset the per pkt timeout id */
	cmd->aic_timeout_id = 0;
#endif

	mutex_enter(aic_ctrlr_mutex);  
	pt_ptr = &aicp->pt[ target * NLUNS + lun];

/*	interrupts should always be enabled here - perhaps the him wants them disabled now */

	if (HIM6X60QueueSCB(aicp->hacb,(SCB *)scb) != SCB_PENDING) 
	{
		mutex_exit(aic_ctrlr_mutex);
		return (TRAN_BADPKT);
	}

#ifdef AIC_TIMEOUT
	if( pktp->pkt_time > 0 ) 
	{
#ifdef AIC_DEBUG
	if( aic_debug & DAIC_TIMEOUT )
		PRINT0("timeout set for packet 0x%x\n", pktp );
#endif
			/*
			cmd->aic_timeout_id =
				timeout(aic_timeout,(caddr_t) scb,pktp->pkt_time*HZ);
			*/
			cmd->aic_timeout_id =
				timeout(aic_timeout,(caddr_t) scb,
						 drv_usectohz(pktp->pkt_time*1000000));
	}
#endif AIC_TIMEOUT

/* IF operating in NOINTR mode, the packet must not assume interrupts to be on 
 * and must poll for the completion of execution of the packet 
 */

	if ( (pktp->pkt_flags & FLAG_NOINTR) && (pktp->pkt_comp == NULL) ) {
		aic_pollret((SCB *)scb);
		if ( cmd->nointr_pkt_comp == NOINTR_TIMEOUT )
		{
			mutex_exit(aic_ctrlr_mutex);
			return(TRAN_BUSY);
		}
	} else if (pktp->pkt_comp == NULL)  {
		/*
		 * Is this required. Can we ever have a No pkt_comp packet with the
		 * NOINTR flag also enabled?
		 */
		cv_wait(&pt_ptr->pt_cv, aic_ctrlr_mutex);  
	}
	/*
	 * We call the pkt_comp routine directly in the HIM6X60CompleteSCB
	 * routine, instead of scheduling it in a separate thread
	 */
	/*
	else
	{
		ddi_set_callback(aic_comp_callback,(caddr_t) pktp, &cmd->pkt_cb_id);
	}
	*/

	mutex_exit(aic_ctrlr_mutex);
	return(TRAN_ACCEPT);
}


/* 
 * (*tran_reset).  Reset the SCSI bus, or just one target device,
 * depending on level.  Return 1 on success, 0 on failure.  If level is
 * RESET_TARGET, all commands on that target should have their pkt_comp
 * routines called, with pkt_reason set to CMD_RESET.  
 */

/*
 * Name			: aic_reset()
 * Purpose		: Reset the SCSI bus or a specified target/LUN 
 * Called from	: scsi_reset(9F)
 * Arguments	: scsi_address * ap, int level
 * Returns		: Returns 0 or 1
 * Side effects	: None
 */

static int
aic_reset(struct scsi_address *ap, int level)
{
	SCB *reset_scb;
	struct aic *aicp;
	int index;
	kmutex_t *aic_ctrlr_mutex;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	PRINT0("Inside aic_reset\n");
#endif

	aicp = ADDR2AIC(ap);

	index = ddi_get_instance( aicp->aic_dip );
	aic_ctrlr_mutex = (kmutex_t *)ddi_get_soft_state( aic_state_mutex_p, index);

	switch (level)
	{
		case RESET_ALL:
		{
			int tgt_cnt,lun_cnt; 
			int pt_index;
			BOOLEAN retval;
#ifdef AIC_DEBUG
			PRINT0("Resetting All devices on bus \n");
#endif
			/*
			 * We don't untimeout the packets here. They will be
			 * untimedout in the HIM6X60CompleteSCB() routine, which
			 * will anyway be called for each packet on being reset.
			 */

			mutex_enter(aic_ctrlr_mutex);  
			retval = HIM6X60ResetBus(aicp->hacb, (UCHAR)0);
			mutex_exit(aic_ctrlr_mutex);
			return retval;
		}
		
		case RESET_TARGET:
		/*
		 *	Use mutex as in aic_transport().
		 */

#ifdef AIC_DEBUG
			PRINT0("Resetting target %d \n",ap->a_target);
#endif
			reset_scb = (SCB *)kmem_zalloc( sizeof(SCB), KM_SLEEP);
			if( !reset_scb )
			{
#ifdef AIC_DEBUG
				PRINT0("kmem_zalloc for reset pkt failed\n");
#endif
				return 0;
			}

			/* Set up scb requesting the reset of a target */
			reset_scb->osRequestBlock = (void *)aicp;
			reset_scb->length = sizeof(SCB);
			reset_scb->scbStatus = 0x00;
			reset_scb->chain = (SCB *)0;
			reset_scb->linkedScb = (SCB *)0;
			reset_scb->cdb = (UCHAR *)0;
			reset_scb->dataLength = 0;
			reset_scb->dataPointer = (UCHAR *)0;
			reset_scb->targetID = ap->a_target;
			reset_scb->lun = ap->a_lun;
			reset_scb->length = sizeof(SCB);
			reset_scb->function = SCB_BUS_DEVICE_RESET;

			mutex_enter(aic_ctrlr_mutex);  
			if (HIM6X60QueueSCB(aicp->hacb, reset_scb) == SCB_PENDING) 
			{
			/*
			 * Must wait till it is processed by CompleteSCB() and return 
			 * the value
			 */
				
			/*
		 	 * Here again all the packetes for this target will be 
			 * untimedout in the HIM6X60CompleteSCB() routine.
			 */
				cv_wait(&(AP2PTGTP(ap)->pt_cv), aic_ctrlr_mutex);
				mutex_exit(aic_ctrlr_mutex);
				return 1;
			}
			else
			{
				kmem_free(reset_scb,sizeof(SCB));
				mutex_exit(aic_ctrlr_mutex);
				return 0;
			}
		default:
			return 0;
	}
}

/* 
 * (*tran_abort).  Abort specific command on target device, or all commands
 * on that target/LUN.
 */

/*
 * Name			: aic_abort()
 * Purpose		: Abort the indicated/all commands queued for a 
 *		  		  particular target/LUN
 * Called from	: scsi_abort(9F)
 * Arguments	: scsi_address *ap, struct scsi_pkt *pktp
 * Returns		: Returns 0 or 1
 * Side effects	: None
 */

static int 
aic_abort(struct scsi_address *ap, struct scsi_pkt *pktp) 
{
	/*
 	 * Abort the command pktp on the target/lun in ap.  If pktp is
	 * NULL, abort all outstanding commands on that target/lun.
	 * If you can abort them, return 1, else return 0.
	 * Each packet that's aborted should be sent back to the target
	 * driver through the callback routine, with pkt_reason set to
	 * CMD_ABORTED.
 	 */

	SCB *abort_scb;
	SCB *scb_to_abort;
	struct aic_scsi_cmd *cmd;
	struct aic *aicp;
	register int index;
	register kmutex_t *aic_ctrlr_mutex;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside aic_abort\n");
#endif
	
	if (pktp)		/* Single pkt */
	{
		/*
		 *	Use mutex as in aic_transport().
		 */

		aicp = PKT2AIC(pktp);
		cmd = PKT2CMD(pktp);

		index = ddi_get_instance( aicp->aic_dip );
		aic_ctrlr_mutex = (kmutex_t *) ddi_get_soft_state( aic_state_mutex_p,
															index);
		scb_to_abort =(SCB *)(&cmd->scb);
		abort_scb = (SCB *)kmem_zalloc( sizeof(SCB), KM_SLEEP);
		if( !abort_scb )
		{
#ifdef AIC_DEBUG
			PRINT0("Unable to allocate scb for abort\n");
#endif
			return 0;
		}
#ifdef AIC_DEBUG
		PRINT0("Pkt abort for Tgt : %x & Lun %x\n",ap->a_target,ap->a_lun);
#endif

		/* Set up the scb requesting abort */
		abort_scb->osRequestBlock = (void *)aicp;
		abort_scb->length = sizeof(SCB);
		abort_scb->scbStatus = 0x00;
		abort_scb->chain = (SCB *)0;
		abort_scb->linkedScb = (SCB *)0;
		abort_scb->cdb = (UCHAR *)0;
		abort_scb->dataLength = 0;
		abort_scb->dataPointer = (UCHAR *)0;
		abort_scb->targetID = ap->a_target;
		abort_scb->lun = ap->a_lun;
		abort_scb->length = sizeof(SCB);
		abort_scb->function = SCB_ABORT_REQUESTED;
		scb_to_abort->linkedScb = (SCB *)0;

		mutex_enter(aic_ctrlr_mutex);

		if ( HIM6X60AbortSCB(aicp->hacb, abort_scb, scb_to_abort)
				!= SCB_ABORT_FAILURE )
		{
			if( abort_scb->scbStatus != SCB_COMPLETED_OK)
			{
				cv_wait(&(AP2PTGTP(ap)->pt_cv), aic_ctrlr_mutex);
				mutex_exit(aic_ctrlr_mutex);
			}
			else
			{
				kmem_free(abort_scb,sizeof(SCB));
				mutex_exit(aic_ctrlr_mutex);
			}
			return 1;
		}
		else
		{
			kmem_free(abort_scb,sizeof(SCB));
			mutex_exit(aic_ctrlr_mutex);
			return 0;
		}
	}
	return 0;	
}
	  
/* 
 * (*tran_getcap).  Get the capability named, and return its value.
 */

/*
 * Name			: aic_getcap()
 * Purpose		: Get the scsi capability named
 * Called from	: scsi_ifsetcap(9F)
 * Arguments	: struct scsi_address *ap, char *cap, int tgtonly
 * Returns		: the requested capability's value or UNDEFINED
 * Side effects	: None
 */

static int 
aic_getcap(struct scsi_address *ap, char *capstr, int whom) 
{
	int	 	ckey;
	struct aic *aicp;
	int instance;
	kmutex_t *aic_ctrlr_mutex;
	int rval;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside aic_getcap\n");
#endif

	aicp = ADDR2AIC(ap);
	instance = ddi_get_instance(aicp->aic_dip);
	aic_ctrlr_mutex = (kmutex_t *)
					  ddi_get_soft_state(aic_state_mutex_p, instance);

	mutex_enter(aic_ctrlr_mutex);

	switch (scsi_hba_lookup_capstr(capstr)) 
	{
		case SCSI_CAP_DMA_MAX:
			rval = AP2PTGTP(ap)->dma_lim.dlim_reqsize;
			break;
		case SCSI_CAP_MSG_OUT:
			rval = 1;
			break;
		case SCSI_CAP_DISCONNECT:
			if(whom == 0)
				rval = 1;
			else
				rval = (AP2PTGTP(ap)->tgt_state & AIC_TGT_DISCO) ? 1 : 0;
			break;
		case SCSI_CAP_SYNCHRONOUS:
			if(whom == 0)
				rval = 1;
			else
				rval = (AP2PTGTP(ap)->tgt_state & AIC_TGT_SYNC) ? 1 : 0;
			break;
		case SCSI_CAP_WIDE_XFER:
			rval = 0;
			break;
		case SCSI_CAP_INITIATOR_ID:
			rval =  aicp->hacb->ownID;
			break;
		case SCSI_CAP_UNTAGGED_QING:
			rval = 0;
			break;
		case SCSI_CAP_TAGGED_QING:
			rval = 0;
			break;
		case SCSI_CAP_ARQ:
			if (whom == 0)
				rval = 1;
			else
				rval = (AP2PTGTP(ap)->tgt_state & AIC_TGT_ARQ) ? 1 : 0;
			break;
		case SCSI_CAP_LINKED_CMDS:
			rval = 0;
			break;
		case SCSI_CAP_SECTOR_SIZE:
			if (whom == 0)
				rval = 0;
			else
				rval = (AP2PTGTP(ap)->dma_lim.dlim_granular);
			break;
		case SCSI_CAP_TOTAL_SECTORS:
			if (whom == 0)
				rval = 0;
			else
				rval = (AP2PTGTP(ap)->total_sectors);
			break;
		case SCSI_CAP_GEOMETRY:
		{
			int total_sectors=0;
			int heads=0;
			int sectors=0;
			HACB *hacb;

			if (whom != 0 && whom != 1) {
				rval = UNDEFINED;
				break;
			}

			total_sectors = AP2PTGTP(ap)->total_sectors;
			if (total_sectors == 0) 
				rval = 0 ;
			else
			{
/*
 * Enable the 255 head 63 sector BIOS Geometry translation only if AIC6360
 * Enhanced features mode is set.
 */

				hacb = (HACB *)aicp->hacb;
				if((INPUT(portB) & 0x01) == 0)
				{
					heads = 64 ;
					sectors = 32 ;
				}
				else
				{
					if (total_sectors < 0x200000)
					{ 
						heads = 64 ;
						sectors = 32 ;
					} else {
						heads = 255 ;
						sectors = 63;
					}
				}
				rval = ((heads << 16)|sectors);
			}
			break;
		}
		default:
			rval = -1;
			break;
	}
	mutex_exit(aic_ctrlr_mutex);
	return (rval);
}


/* 
 * (*tran_setcap).  Set the capability named to the value given.
 */
/*
 * Name			: aic_setcap()
 * Purpose		: Set the capability's value
 * Called from	: scsi_ifsetcap(9F)
 * Arguments	: struct scsi_address * ap, char * cap, int value, int tgtonly
 * Returns		: TRUE or FALSE or UNDEFINED
 * Side effects	: The capability value is set in the per target structure
 *		  	   	  within the aic structure (aicp->pt)
 */

static int 
aic_setcap(struct scsi_address *ap, char *capstr, int value, int whom) 
{
	struct aic *aicp;	
	int targ, lun;
	int instance;
	kmutex_t *aic_ctrlr_mutex;
	int rval;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside aic_setcap\n");
#endif

	targ = ap->a_target;
	lun = ap->a_lun;
	aicp = ADDR2AIC(ap);
	instance = ddi_get_instance(aicp->aic_dip);
	aic_ctrlr_mutex = (kmutex_t *)
					  ddi_get_soft_state(aic_state_mutex_p, instance);

	mutex_enter(aic_ctrlr_mutex);

	switch (scsi_hba_lookup_capstr(capstr))
	{
		case SCSI_CAP_DMA_MAX:
		case SCSI_CAP_MSG_OUT:
		case SCSI_CAP_PARITY:
		case SCSI_CAP_WIDE_XFER:
		case SCSI_CAP_INITIATOR_ID:
		case SCSI_CAP_LINKED_CMDS:
		case SCSI_CAP_UNTAGGED_QING:
		case SCSI_CAP_TAGGED_QING:
		case SCSI_CAP_GEOMETRY:
			rval = 0;
			break;
		case SCSI_CAP_DISCONNECT:
			if(whom == 0)
				rval = 0;
			else if(value == 0){
				AP2PTGTP(ap)->tgt_state &= ~AIC_TGT_DISCO;
				rval = 1;
			}
			else if(value == 1){
				AP2PTGTP(ap)->tgt_state |= AIC_TGT_DISCO;
				rval = 1;
			}
			else
				rval = -1;
			break;
		case SCSI_CAP_SYNCHRONOUS:
			if(whom == 0)
				rval = 0;
			else if(value == 0){
				AP2PTGTP(ap)->tgt_state &= ~AIC_TGT_SYNC;
				rval = 1;
			}
			else if(value == 1){
				AP2PTGTP(ap)->tgt_state |= AIC_TGT_SYNC;
				rval = 1;
			}
			else
				rval = -1;
			break;
		case SCSI_CAP_ARQ:
			if (whom == 0) 
				rval = 0;
			else if (value == 0) {
				AP2PTGTP(ap)->tgt_state &= ~AIC_TGT_ARQ;
				rval = 1;
			}
			else if(value == 1){
				AP2PTGTP(ap)->tgt_state |= AIC_TGT_ARQ;
				rval = 1;
			}
			else
				rval = -1;
			break;

		case SCSI_CAP_SECTOR_SIZE:
			if(whom == 0)
				rval = 0;
			else{
				AP2PTGTP(ap)->dma_lim.dlim_granular = value;
				rval = 1;
			}
			break;

		case SCSI_CAP_TOTAL_SECTORS:
			if(whom == 0)
				rval = 0;
			else{
				AP2PTGTP(ap)->total_sectors = (u_int)value;
				rval = 1;
			}
			break;
	}
	mutex_exit(aic_ctrlr_mutex);
	return (rval);
}

/*
 * (*tran_dmaget)() . This routine is used to obtain the buffer pointer
 * for the data transfers. 
 */

/*
 * Name			: aic_dmaget()
 * Purpose		: Allocate dma resources required by the packet 
 * Called from	: scsi_init_pkt(9F)
 * Arguments	: scsi_pkt *pktp, opaque_t dmatoken, int (*callback)(),
 *		  		  caddr_t arg
 * Returns		: pktp
 * Side effects : The bp supplied as dmatoken is saved in cmd->cmd_private
 *		  		  field of cmd, the packet wrapper structure for pktp.
 *				  Also lock the pages in memory by mapping them into kernel
 *			 	  space to prevent them from being swapped out.
 */

struct scsi_pkt *
aic_dmaget(struct scsi_pkt *pktp, opaque_t dmatoken, int (*callback)(), 
	    caddr_t arg)
{
	struct buf *bp = (struct buf *)dmatoken;
	volatile register struct aic_scsi_cmd *cmd;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
		PRINT0("Inside the dmaget entry point\n");
#endif

	cmd = PKT2CMD(pktp);
	if ( !cmd ) 
	{
#ifdef AIC_DEBUG
		PRINT0( "Transport structure not defined\n");
#endif
		return ( (struct scsi_pkt *) NULL );
	}

	/* Ensure that the pages will not be swapped out if target disconncts */
	if ((bp->b_flags & B_PHYS) || (bp->b_flags & B_PAGEIO)) 
		bp_mapin(bp);

	cmd->cmd_private = (opaque_t)bp;
	return(pktp);
}

/**************   THE DMA GET ROUTINE IF DMA IS ENABLED ***************/

#ifdef AIC_DMA
/*
 * (*tran_dmaget)() . This routine is used to obtain the buffer pointer
 * for the data transfers. 
 */

/*
 * Name			: aic_en_dmaget()
 * Purpose		: Allocate dma resources required by the packet 
 * Called from	: scsi_init_pkt(9F)
 * Arguments	: scsi_pkt *pktp, opaque_t dmatoken, int (*callback)(),
 *		  		  caddr_t arg
 * Returns		: pktp
 * Side effects : The bp supplied as dmatoken is saved in cmd->cmd_private
 *		  		  field of cmd, the packet wrapper structure for pktp.
 *				  Also lock the pages in memory by mapping them into kernel
 *			 	  space to prevent them from being swapped out. Also allocates
 *				  the scatter gather list for dma transfers.
 */

struct scsi_pkt *
aic_en_dmaget(struct scsi_pkt *pktp, opaque_t dmatoken, int (*callback)(), 
	    caddr_t arg)
{
	struct buf *bp = (struct buf *)dmatoken;
	volatile register struct aic_scsi_cmd *cmd;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside aic_en_dmaget\n");
#endif
	
	cmd = PKT2CMD(pktp);

	ddi_dma_cookie_t dmac;
	ddi_dma_cookie_t *dmacp = &dmac;
	int		cnt;
	int		total_bytes;
	off_t		offset;
	off_t		len;
	ddi_dma_cookie_t *dmap;		


	if ( !cmd ) 
	{
#ifdef AIC_DEBUG
		PRINT0( "Transport structure not defined\n");
#endif
		return ( (struct scsi_pkt *) NULL );
	}

	/* Ensure that the pages will not be swapped out if target disconncts */
	if ((bp->b_flags & B_PHYS) || (bp->b_flags & B_PAGEIO)) 
		bp_mapin(bp);

	/*
	 * If your HBA hardware has flags for "data connected to this
	 * command" or "no data for this command", here's the 
	 * appropriate place to set them, based on bp->b_bcount.
	 */

	cmd->cmd_private = (opaque_t)bp;

	if (!bp->b_bcount) 
	{
		return(pktp);
	}


	cmd->cmd_dmaseg = NULL;

	if (!bp->b_bcount) {
		/* set for target examination */
		pktp->pkt_resid = 0;
		return(pktp);
	}
	if (bp->b_bcount < PKTP2PTGTP(pktp)->dma_lim.dlim_granular) {
		/* set for target examination */
#ifdef AIC_DEBUG
	if(aic_debug & DDMA)
		PRINT0("aic_dmaget: bp->bcount = 0x%x\n",bp->b_bcount);
#endif
		return(pktp);
	}

	if( bp->b_flags & B_READ ){
		cmd->cmd_cflags &= ~CFLAG_DMASEND;
	}
	else {
		cmd->cmd_cflags |= CFLAG_DMASEND;
	}

	if (!scsi_impl_dmaget(pktp, (opaque_t)bp, callback, arg,
		&(PKTP2PTGTP(pktp)->dma_lim)))
	{
#ifdef AIC_DEBUG
	if(aic_debug & DDMA)
		PRINT0("aic_dmaget: scsi_impl_dmaget() failed...\n");
#endif
		return(NULL);
	}

	/* 
 	 * Set up DMA memory and position to the next DMA segment.
	 * Information will be in aic_scsi_cmd on return; most usefully,
	 * in cmd->cmd_dmaseg.
	 */

	if( ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset, &len, dmacp) 
		== DDI_FAILURE)
	{
#ifdef AIC_DEBUG
		PRINT0( "ddi_dma_segtocookie failed.\n");
#endif
		return (NULL);
	}

#ifdef AIC_DEBUG
	if(aic_debug & DDMA)
	PRINT0("dmacp: add = 0x%x, len = %d \n", dmacp->dmac_address,
		dmacp->dmac_size);
#endif

	/* Check for transfer in one segment */
	if( bp->b_bcount <= dmacp->dmac_size )
	{
		PKTP2PTGTP(pktp)->sg = 
			kmem_zalloc(sizeof(ddi_dma_cookie_t), KM_SLEEP);
		dmap = PKTP2PTGTP(pktp)->sg ;		/* Assign dmap */
#ifdef AIC_DEBUG
	if(aic_debug & DDMA)
		PRINT0("aic_dmaget: sg_list = 0x%X \n", dmap);
#endif
	
		if( !dmap )
		{
#ifdef AIC_DEBUG
			PRINT0("Alloc of scatter_gather list failed\n");
#endif
			return NULL;
		}
		dmap->dmac_size = dmacp->dmac_size;
		dmap->dmac_address = dmacp->dmac_address;
		dmap->dmac_type = dmacp->dmac_type;	
		pktp->pkt_resid = bp->b_bcount;
#ifdef AIC_DEBUG
	if(aic_debug & DDMA)
	{
		PRINT0("bp->bcount = 0x%x \n", bp->b_bcount);
		PRINT0("dmac size : %x\n",dmap->dmac_size);
		PRINT0("dmac address : %x\n",dmap->dmac_address);
		PRINT0("dmac type : %x\n",dmap->dmac_type);
		PRINT0("dmap address : %x\n",dmap);
	}
#endif
	}

	else
	{

	/* Allocating space for scatter_gather list.  */

		PKTP2PTGTP(pktp)->sg = 
			kmem_zalloc((sizeof(ddi_dma_cookie_t) * AIC_MAX_DMA_SEGS),
						KM_SLEEP);
		dmap = PKTP2PTGTP(pktp)->sg ;		/* Assign dmap */
#ifdef AIC_DEBUG
	if(aic_debug & DDMA)
		PRINT0("aic_dmaget: sg_list = 0x%X \n", dmap);
#endif
	
		if( !dmap )
		{
#ifdef AIC_DEBUG
			PRINT0("Alloc of scatter_gather list failed\n");
#endif
			return NULL;
		}

		/*
		 * The loop below stores physical addresses of DMA segments, from
		 * DMA cookies, into the HBA's scatter gather list pointed to by
		 * dmap.
		 */

		for( total_bytes=0, cnt=1; ; cnt++,dmap++ )
		{
			total_bytes += dmacp->dmac_size;
			dmap->dmac_size = dmacp->dmac_size;
			dmap->dmac_address = dmacp->dmac_address;
			dmap->dmac_type = dmacp->dmac_type;	

#ifdef AIC_DEBUG
	if(aic_debug & DDMA)
	{
			PRINT0("dmac size : %x\n",dmap->dmac_size);
			PRINT0("dmac address : %x\n",dmap->dmac_address);
			PRINT0("dmac type : %x\n",dmap->dmac_type);
			PRINT0("dmap address : %x\n",dmap);
	}
#endif

			if( bp->b_bcount <= total_bytes )
				break;

			/* check for end of scatter gather list */
			if( cnt >= AIC_MAX_DMA_SEGS )
				break;
			
			if( ddi_dma_nextseg(cmd->cmd_dmawin, cmd->cmd_dmaseg,
								 &cmd->cmd_dmaseg) != DDI_SUCCESS )
			{
#ifdef AIC_DEBUG
				PRINT0("ddi_dma_nextseg failed in dmaget\n");
#endif
				break;
			}
			ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset, &len, dmacp);
		}
		pktp->pkt_resid = total_bytes;
	}
	return (pktp);
}
#endif AIC_DMA
/************************ DMAGET ROUTINE ENDS *******************************/


/* 
 * (*tran_dmafree).  Free DMA resources associated with a scsi_pkt.  
 * Memory for the packet itself is freed in aic_pktfree.
 */

/*
 * Name			: aic_dmafree()
 * Purpose		: Undo whatever is done in aic_dmaget()
 * Called from	: scsi_destroy_pkt(9F)
 * Arguments	: struct scsi_address *ap, struct scsi_pkt *pktp
 * Returns		: None
 * Side effects	: None
 */

void
aic_dmafree(struct scsi_address *ap, struct scsi_pkt *pktp)
{
	volatile struct aic_scsi_cmd *cmd;
	struct buf *bp;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside aic_dmafree \n");
#endif
	
	cmd = PKT2CMD(pktp);
	/*
	 * We don't have to do a bp_mapout here, since the biodone routine
	 * does that
	 */

	/* undo whatever was done in dmaget function */
	cmd->cmd_private = (opaque_t)0;
	cmd->cmd_dmaseg = (ddi_dma_seg_t) NULL;
}

/*****************  DMA FREE ROUTINE TO BE ENABLED IF DMA IS USED ************/

#ifdef AIC_DMA
/* 
 * (*tran_dmafree).  Free DMA resources associated with a scsi_pkt.  
 * Memory for the packet itself is freed in aic_pktfree.
 */

/*
 * Name			: aic_en_dmafree()
 * Purpose		: Undo whatever is done in aic_dmaget()
 * Called from	: scsi_destroy_pkt(9F)
 * Arguments	: struct scsi_pkt *pktp
 * Returns		: None
 * Side effects	: Frees the scatter gather list allocated for dma transfers.
 */

void
aic_en_dmafree(struct scsi_pkt *pktp)
{
	volatile struct aic_scsi_cmd *cmd;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside  aic_en_dmafree\n");
#endif
	
	cmd =  PKT2CMD(pktp);

	/* undo whatever was done in dmaget function */
	if( cmd->cmd_dmahandle )
	{
		if (pktp->pkt_resid < PKTP2PTGTP(pktp)->dma_lim.dlim_granular) 
		{
			return;
		}
 		if (PKTP2PTGTP(pktp)->sg)
			kmem_free(PKTP2PTGTP(pktp)->sg, 
				(sizeof(ddi_dma_cookie_t) * AIC_MAX_DMA_SEGS));
		scsi_impl_dmafree(pktp);
		cmd->cmd_dmahandle = NULL;
		cmd->cmd_dmaseg = (ddi_dma_seg_t) NULL;
	}
	return;	
}
#endif AIC_DMA
/***********************  DMA FREE FUNCTION ENDS *************************/

/*
 * Name			: aic_tran_tgt_init()
 * Purpose		: Allows HBA to allocate/initialize any per-target resources
 *				  as may be necessary. 
 * Called from	: SCSA
 * Arguments	: dev_info_t *hba_dip, dev_info_t *tgt_dip, 
 * 				  scsi_hba_tran_t *hba_tran, struct scsi_device *sd
 * Returns		: DDI_SUCCESS/DDI_FAILURE
 * Side effects	: None
 */

/*ARGSUSED*/
static int
aic_tran_tgt_init( dev_info_t *hba_dip, dev_info_t *tgt_dip,
				   scsi_hba_tran_t *hba_tran, struct scsi_device *sd )
{
	int targ;
	int lun;
	volatile struct aic *aicp;
	kmutex_t  *aic_ctrlr_mutex;
	int index;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside tran_tgt_init\n");
#endif

	aicp = SDEV2AIC(sd);
	index = ddi_get_instance(aicp->aic_dip);
	aic_ctrlr_mutex = (kmutex_t *)
					  ddi_get_soft_state(aic_state_mutex_p, index);
	targ = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT, "%s%d:%s%d<%d,%d>\n", ddi_get_name(hba_dip),
			ddi_get_instance(hba_dip), ddi_get_name(tgt_dip), 
			ddi_get_instance(tgt_dip), targ,lun);
#endif

	if( (targ < 0) || (targ >7) || (lun < 0) || (lun > 7) ) {
		cmn_err(CE_WARN, "%s%d:%s%d<%d,%d>\n", ddi_get_name(hba_dip),
				ddi_get_instance(hba_dip), ddi_get_name(tgt_dip), 
				ddi_get_instance(tgt_dip), targ,lun);
	}

	mutex_enter(aic_ctrlr_mutex);
	aicp->pt[targ*NLUNS+lun].dma_lim = aic_dma_lim;
	aicp->pt[targ*NLUNS+lun].dma_lim.dlim_granular = SECTOR_SIZE;

	/* Increment the No. of targets initialized */
	aicp->num_targets_attached ++;
	mutex_exit(aic_ctrlr_mutex);

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"aic_tran_tgt_init: <%d,%d>\n", targ, lun);
#endif

	return(DDI_SUCCESS);
}

/*
 * Name			: aic_tran_tgt_probe()
 * Purpose		: Allows HBA to customize the operation of scsi_probe if
 *				  necessary. Here we merely call scsi_hba_probe.
 * Called from	: SCSA (scsi_probe)
 * Arguments	: struct scsi_device *sd, int (*callback)()
 * Returns		: the value returned by scsi_hba_probe
 * Side effects	: None
 */
/*
 * The aic_tran_tgt_probe routine is optional and scsa simply calls 
 * scsi_hba_probe() directly if the driver does not have this entry point.
 */

/*ARGSUSED*/
static int
aic_tran_tgt_probe( struct scsi_device *sd, int (*callback)() )
{
	int ret_val;
#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside tran_tgt_probe\n");
#endif

	ret_val = scsi_hba_probe(sd, callback );

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	{
		char *s;
		struct aic *aicp = SDEV2AIC(sd);
		if (aic_debug & DINIT)
		cmn_err(CE_CONT,"In tgt_probe aicp : 0x%x\n", aicp );

		switch(ret_val) {
			case SCSIPROBE_NOMEM :
				s="scsi_probe_nomem";
				break;
			case SCSIPROBE_EXISTS:
				s="scsi_probe_exists";
				break;
			case SCSIPROBE_NONCCS:
				s="scsi_probe_nonccs";
				break;
			case SCSIPROBE_FAILURE:
				s="scsi_probe_failure";
				break;
			case SCSIPROBE_BUSY:
				s="scsi_probe_busy";
				break;
			case SCSIPROBE_NORESP:
				s="scsi_probe_noersp";
				break;
			default:
				s="<error>";
				break;
		}
		cmn_err(CE_CONT,"aic%d:%s target %d lun %d %s\n", 
				ddi_get_instance(aicp->aic_dip), ddi_get_name(sd->sd_dev),
				sd->sd_address.a_target, sd->sd_address.a_lun, s);
	}
#endif
	return (ret_val);
}

/*
 * Name			: aic_tran_tgt_free()
 * Purpose		: Allows HBA to perform any deallocation or clean up 
 *				  procedures for an instance of a target.
 * Called from	: SCSA
 * Arguments	: dev_info_t *hba_dip, dev_info_t *tgt_dip, 
 * 				  scsi_hba_tran_t *hba_tran, struct scsi_device *sd
 * Returns		: None
 * Side effects	: None
 */

/*ARGSUSED*/
static void
aic_tran_tgt_free( dev_info_t *hba_dip, dev_info_t *tgt_dip, 
				   scsi_hba_tran_t *hba_tran, struct scsi_device *sd )
{
	struct aic *aicp;
	int	targ,lun;
	kmutex_t  *aic_ctrlr_mutex;
	int index;

	aicp = SDEV2AIC(sd);
	index = ddi_get_instance(aicp->aic_dip);
	aic_ctrlr_mutex = (kmutex_t *)
					  ddi_get_soft_state(aic_state_mutex_p, index);

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside tran_tgt_free\n");
#endif

	targ= sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT, "aic_tran_tgt_free %s%d:%s%d<%d,%d>\n", 
			ddi_get_name(hba_dip),
			ddi_get_instance(hba_dip), ddi_get_name(tgt_dip), 
			ddi_get_instance(tgt_dip), targ,lun);
#endif

	mutex_enter(aic_ctrlr_mutex);
	aicp->num_targets_attached --;
	mutex_exit(aic_ctrlr_mutex);
	
}

/*
 * Name			: aic_tran_init_pkt()
 * Purpose		: Allocates scsi_pkt structures on behalf of the target driver.
 * Called from	: SCSA
 * Arguments	: struct scsi_address *ap, struct scsi_pkt *pkt,
				  struct buf *bp, int cmdlen, int statuslen, int tgtlen,
				  int flags, int (*callback)(), caddr_t arg 
 * Returns		: a pointer to the scsi_pkt
 * Side effects	: None
 */

static struct scsi_pkt *
aic_tran_init_pkt( struct scsi_address *ap, struct scsi_pkt *pkt,
				   struct buf *bp, int cmdlen, int statuslen, int tgtlen,
				   int flags, int (*callback)(), caddr_t arg )
{
	struct scsi_pkt *new_pktp = NULL;
	volatile struct aic *aicp;
	volatile struct aic_scsi_cmd *cmd;
	volatile SCB *scb;
	u_int statbuflen;
	int i;
	kmutex_t *aic_ctrlr_mutex;
	int index;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	PRINT0("Inside tran_init_pkt\n");
#endif

	aicp = ADDR2AIC(ap);

	/* allocate a pkt */
	if ( !pkt ) {
		if( tgtlen < PKT_PRIV_LEN )
			tgtlen = PKT_PRIV_LEN;
		
	 	if( AP2PTGTP(ap)->tgt_state & AIC_TGT_ARQ )
			statbuflen = sizeof(struct scsi_arq_status);
		else
			statbuflen = STATUS_SIZE;
		if ( statuslen > statbuflen )
			statbuflen = statuslen;
		pkt = scsi_hba_pkt_alloc(aicp->aic_dip, ap, cmdlen, statbuflen, 
						tgtlen, sizeof(struct aic_scsi_cmd), callback, arg);
		if( pkt == NULL ){
#ifdef AIC_DEBUG
			PRINT0("Unable to allocate pkt\n");
#endif AIC_DEBUG
			return NULL;
		}
		new_pktp = pkt;
	}
	else {
		new_pktp = NULL;
	}

	cmd = (struct aic_scsi_cmd *)pkt->pkt_ha_private;

	cmd->cmd_flags = flags;
	cmd->cmd_dmahandle = (ddi_dma_handle_t)0;
	cmd->cmd_pkt = pkt;
	cmd->cmd_cdblen = cmdlen;
	cmd->cmd_scblen = statbuflen;
	scb = (SCB *)(&cmd->scb);
	scb->cdb = pkt->pkt_cdbp;
	scb->cdbLength = cmdlen;
	scb->dataPointer = (UCHAR *)0;
	scb->dataLength = 0;
	scb->senseDataLength = 0;
	pkt->pkt_resid = 0;


	/* set up dma info */

	if( bp != NULL ) {
#ifdef AIC_DMA
		if( (pkt = aic_en_dmaget(pkt, (opaque_t) bp, callback, arg)) == NULL ) {
			if( new_pktp )
				scsi_hba_pkt_free(ap, new_pktp);
			return NULL;
		}
#else
		if( (pkt = aic_dmaget(pkt, (opaque_t) bp, callback, arg)) == NULL ) {
			if( new_pktp )
				scsi_hba_pkt_free(ap, new_pktp);
			return NULL;
		}
#endif
	}
	scb->length = sizeof(SCB);
	if( bp )
	{
		scb->dataPointer = (UCHAR *) bp->b_un.b_addr;
		if( pkt->pkt_resid ) {
			scb->dataLength = pkt->pkt_resid;
			pkt->pkt_resid = 0;
		}
		else
			scb->dataLength = bp->b_bcount;
	}
	return(pkt);
}

/*
 * Name			: aic_tran_destroy_pkt()
 * Purpose		: Deallocates the scsi_pkt structure.
 * Called from	: SCSA
 * Arguments	: struct scsi_address *ap, struct scsi_pkt *pkt
 * Returns		: None
 * Side effects	: None
 */

static void
aic_tran_destroy_pkt( struct scsi_address *ap, struct scsi_pkt *pkt )
{
	volatile struct aic_scsi_cmd *cmd;
	int i;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside tran_destroy_pkt\n");
#endif

	cmd = PKT2CMD(pkt);
#ifdef AIC_DMA
	aic_en_dmafree(ap,pkt);
#else
	aic_dmafree(ap,pkt);
#endif
	scsi_hba_pkt_free(ap,pkt);
}

/*
 * Name			: aic_sync_pkt()
 * Purpose		: Synchronizes the DMA object allocated for the scsi_pkt 
 *				  structure before or after a DMA transfer.
 *				  as may be necessary. 
 * Called from	: SCSA
 * Arguments	: struct scsi_address *ap, struct scsi_pkt *pktp
 * 				  scsi_hba_tran_t *hba_tran, struct scsi_device *sd
 * Returns		: None
 * Side effects	: None
 */

static void
aic_sync_pkt( struct scsi_address *ap, register struct scsi_pkt *pktp)
{
#ifdef AIC_DMA
	register int i;
	register struct aic_scsi_cmd *cmd;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside aic_sync_pkt\n");
#endif
	
	cmd = PKT2CMD(pktp);

	if( cmd->cmd_dmahandle ) {
		i = ddi_dma_sync(cmd->cmd_dmahandle, 0 ,0,
			(cmd->cmd_cflags & CFLAG_DMASEND) ?
			DDI_DMA_SYNC_FORDEV : DDI_DMA_SYNC_FORCPU);
		if( i != DDI_SUCCESS ) {
			cmn_err(CE_CONT,"aic:sync pkt failed\n");
		}
	}
#endif
}

/*
 * A dummy interrupt routine used to setup iblock_cookie
 */

static u_int
aic_dummy_intr(caddr_t arg) 
{

}

/*
 * Name			: set_controller_options()
 * Purpose		: Overrides any default property values
 * Called from	: aic_attach
 * Arguments	: ushort scsi_options, HACB *hacb
 * Returns		: None
 * Side effects	: None
 */

static void
set_controller_options(ushort scsi_options, HACB *hacb)
{
#ifdef AIC_DEBUG
	PRINT0("scsi-options property :0x%x\n", scsi_options);
#endif AIC_DEBUG
	/* Disconnect Option */
	if( !(scsi_options & SCSI_OPTIONS_DR) )
		hacb->ac_u.ac_s.noDisconnect = TRUE;
	else
		hacb->ac_u.ac_s.noDisconnect = FALSE;

	/* Parity Check Option */
	if( (scsi_options & SCSI_OPTIONS_PARITY) )
		hacb->ac_u.ac_s.checkParity = TRUE;
	else
		hacb->ac_u.ac_s.checkParity = FALSE;

	/* SDTR option */
	if( (scsi_options & SCSI_OPTIONS_SYNC) )
		hacb->ac_u.ac_s.initiateSDTR = TRUE;
	else
		hacb->ac_u.ac_s.initiateSDTR = FALSE;

	/* Fast SCSI option
	if( (scsi_options & SCSI_OPTIONS_FAST) )
		hacb->ac_u.ac_s.fastSCSI = TRUE;
	else
		hacb->ac_u.ac_s.fastSCSI = FALSE;
	this is commented out because x86 does not set this
	value in any meaningful way */
}

/*
 * Name			: aic_getinfo()
 * Purpose		: Returns requested device driver information. Since our driver
 *				  has no cb_ops structure, we just return DDI_FAILURE.
 * Called from	: SCSA
 * Arguments	: dev_info_t *dip, ddi_info_cmd_t *cmd, void *arg,
 *				  void **resultp
 * Returns		: DDI_FAILURE
 * Side effects	: None
 */

static int
aic_getinfo(dev_info_t *dip, ddi_info_cmd_t cmd, void *arg, void **resultp)
{
	return(DDI_FAILURE);
}

/*  
 * Autovector interrupt entry point.  Passed to ddi_add_intr() in 
 * aic_attach().
 */
/*
 * Name			: aic_intr
 * Purpose		: Handle reselection interrupts
 * Called from	: Kernel
 * Arguments	: caddr_t arg
 * Returns		: DDI_INTR_CLAIMED or DDI_INTR_UNCLAIMED
 * Side effects	: None
 */

static u_int
aic_intr(caddr_t arg) 
{
	struct aic 	*aicp;
	register int		index;
	int target,lun;
	register kmutex_t *aic_ctrlr_mutex;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside aic_intr \n");
#endif

	aicp = (struct aic*) arg ;
	index = ddi_get_instance ( aicp->aic_dip );
	aic_ctrlr_mutex = (kmutex_t *)
					  ddi_get_soft_state( aic_state_mutex_p, index);

	/* lock the controller */

	mutex_enter(aic_ctrlr_mutex);

	/* Call the HIM's ISR routine to handle the interrupt conditions */
	/* We call HIM6X60IRQ since we are not polling for interrupts */

	if (HIM6X60IRQ(aicp->hacb) == FALSE)	
	{
		mutex_exit(aic_ctrlr_mutex);
		return DDI_INTR_UNCLAIMED;
	}
	mutex_exit(aic_ctrlr_mutex);
	return(DDI_INTR_CLAIMED);
}

/*
 * Name			: aic_comp_callback()
 * Purpose		: To execte packet completion routine as a call back thread
 * Called from	: aic_transport
 * Arguments	: struct scsi_pkt *pktp
 * Returns		: TRUE
 * Side effects	: None
 */

INT 
aic_comp_callback(struct scsi_pkt *pktp)
{
#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside aic_comp_callback\n");
#endif
	if( pktp->pkt_comp)
		(*pktp->pkt_comp)(pktp);
	return 1;
}
		

/*
 * Name			: aic_pollret()
 * Purpose		: To continuously poll for the completion of a packet sent with
 *				  FLAG_NOINTR. Interrups can't be assumed to be on.
 * Called from	: aic_transport
 * Arguments	: SCB *scb
 * Returns		: None
 * Side effects	: None
 */

void
aic_pollret(SCB *scb)
{
	struct aic *aicp;
	HACB *hacb;
	struct scsi_pkt *pktp;
	kmutex_t *aic_ctrlr_mutex;
	int index;
	struct aic_scsi_cmd *cmd;
	unsigned long aic_watchdog_count = 0xfffff;		/* A large count */

#ifdef AIC_DEBUG
	PRINT0("In aic_pollret\n");
#endif
	pktp = (struct scsi_pkt *) scb->osRequestBlock;
	aicp = PKT2AIC(pktp);
	cmd = PKT2CMD(pktp);
	hacb = (HACB *) aicp->hacb;

	cmd->nointr_pkt_comp = FALSE;
	while( cmd->nointr_pkt_comp == FALSE  && aic_watchdog_count)
	{
		HIM6X60ISR(hacb);
		aic_watchdog_count--;
	}
	if ( aic_watchdog_count == 0 )
	{
		/* Reset the Bus. Is this necessary */
#if 0
		while( HIM6X60ResetBus(aicp->hacb,(UCHAR)0) != TRUE )
			;
		/* Remove any pending interrupts caused due to the Bus Reset? */
		HIM6X60ISR(hacb);
#endif
		cmd->nointr_pkt_comp = NOINTR_TIMEOUT;
	}
	return;
}


/*
 * Name			: aic_get_intr_index()
 * Purpose		: Determines the index of the interrupt level specified in 
 *				  conf file
 * Called from	: aic_probe
 * Arguments	: INT *buf, INT irq, INT len
 * Returns		: index of the interrupt, ILLEGAL_VALUE
 * Side effects	: None
 */

INT
aic_get_intr_index(INT *buf, INT irq, INT len)
{
	INT i;

	for (i=1; i < len; i = i + 2)
	{
		if (buf[i] == irq) 
		{
			return(i / 2);
		}
	}
	return ILLEGAL_VALUE;
}

#ifdef AIC_DEBUG

/*
 * Name			: printHacb
 * Purpose		: Debugging utility to print contents of HACB
 * Called from	: 
 * Arguments	: HACB *hacb
 * Returns		: None
 * Side effects	: None
 */
printHacb(hacb)
HACB *hacb;
{
	PRINT0("Get Configuration Succeeded\n");
	PRINT0("hacb values ... \n");
	PRINT0("ownId = 0x%x ", hacb->ownID);
	PRINT0("IRQ = 0x%x ", hacb->IRQ);
	PRINT0("signature = 0x%x ", hacb->signature);
	PRINT0("revision = 0x%x ", hacb->revision);
	PRINT0("adapterConfiguration = 0x%x ", hacb->ac_u.adapterConfiguration);
}
#endif

#ifdef AIC_DEBUG
/*
 * Name			: aic_dummy()
 * Purpose		: To introduce breakpoints during debugging.
 * Called from	: -
 * Arguments	: None
 * Returns		: None
 * Side effects	: None
 */

void aic_dummy()
{
}

#endif


#ifdef AIC_TIMEOUT
/*
 * Name			: aic_timeout()
 * Purpose		: To timeout a pkt if it does not complete execution after
 *				  the pkt_time duration. The pkt is aborted with its
 *				  reason set to CMD_TIMEOUT
 * Called from	: OS in response to a 'timeout'
 * Arguments	: caddr_t arg
 * Returns		: None
 * Side effects	: None
 */

void
aic_timeout( caddr_t arg )
{
		SCB *scb;
		SCB *abort_scb;
		int index;
		struct scsi_pkt *pktp;
		struct aic *aicp;
		kmutex_t	*aic_ctrlr_mutex;
		struct aic_scsi_cmd *cmd;
	
		scb = (SCB *) arg;


		/* Checks to insure against dummy timeouts */

		if( scb == NULL || (scb->length != sizeof(SCB)) )
			return;
		pktp = (struct scsi_pkt *) scb->osRequestBlock;
		if( pktp == NULL )
			return;

		cmd = PKT2CMD(pktp);

		if( scb != (SCB *) (&cmd->scb) )
			return;
		aicp = PKT2AIC(pktp);
		if( aicp == NULL )
			return;

		index = ddi_get_instance ( aicp->aic_dip );
		aic_ctrlr_mutex = (kmutex_t *) ddi_get_soft_state(aic_state_mutex_p,
															index);
		abort_scb = (SCB *)kmem_zalloc( sizeof(SCB), KM_SLEEP);
		if( !abort_scb )
		{
#ifdef AIC_DEBUG
			PRINT0("Unable to allocate scb for abort\n");
			PRINT0("Unable to timeout\n");
#endif
			return;
		}
#ifdef AIC_DEBUG
	if (aic_debug & DAIC_TIMEOUT )
		PRINT0("Timeout called for packet : 0x%x\n", pktp);
#endif
		pktp->pkt_reason = CMD_TIMEOUT;
		abort_scb->osRequestBlock = (void *)aicp;
		abort_scb->length = sizeof(SCB);
		abort_scb->scbStatus = 0x00;
		abort_scb->chain = (SCB *)0;
		abort_scb->linkedScb = (SCB *)0;
		abort_scb->cdb = (UCHAR *)0;
		abort_scb->dataLength = 0;
		abort_scb->dataPointer = (UCHAR *)0;
		abort_scb->targetID = scb->targetID;
		abort_scb->lun = scb->lun;
		abort_scb->length = sizeof(SCB);
		abort_scb->function = SCB_ABORT_REQUESTED;
		scb->linkedScb = (SCB *)0;

		mutex_enter(aic_ctrlr_mutex);
		if( scb == NULL || pktp == NULL )
		{
			kmem_free(abort_scb,sizeof(SCB));
			mutex_exit(aic_ctrlr_mutex);
			return;
		}
		if (HIM6X60AbortSCB(aicp->hacb, abort_scb, scb)
				== SCB_PENDING)
		{
			if( abort_scb->scbStatus != SCB_COMPLETED_OK)
			{
#ifdef AIC_DEBUG
		if (aic_debug & DAIC_TIMEOUT )
			PRINT0("Waiting in Timeout  : \n");
#endif
				cv_wait(&(PKTP2PTGTP(pktp)->pt_cv), aic_ctrlr_mutex);
				mutex_exit(aic_ctrlr_mutex);
			}
			else
			{

#ifdef AIC_DEBUG
		if (aic_debug & DAIC_TIMEOUT )
			PRINT0(" Timeout  failed : \n");
#endif
				kmem_free(abort_scb,sizeof(SCB));
				mutex_exit(aic_ctrlr_mutex);
			}
			return ;
		}
		else
		{
			pktp->pkt_reason = 0x00;
			kmem_free(abort_scb,sizeof(SCB));
			mutex_exit(aic_ctrlr_mutex);
			return ;
		}
}
#endif

/*
 * PRINT routine for debug messages. It simplifies use of cmn_err
 */

#ifdef AIC_DEBUG
PRINT0(fmt, a, b, c, d, e, f)
char *fmt;
{
	if (aic_debug)
		cmn_err(CE_CONT,fmt,a, b, c, d, e, f);
}
#endif

/********************************************************************** 
					OS SPECIFIC HIM ROUTINES
**********************************************************************/

/*
 * Name			: HIM6X60GetPhysicalAddress()
 * Purpose		: To return the physical address of the supplied virtual
 * 				  address.
 * Called from	: HIM module
 * Arguments	: HACB *hacb, SCB *scb, void *virtualAddress, 
 *				  ULONG bufferOffset, ULONG *length
 * Returns		: Physical address
 * Side effects	: None
 */

PHYSICAL_ADDRESS
HIM6X60GetPhysicalAddress(HACB *hacb, SCB *scb,
					   VOID *virtualAddress,
					   ULONG bufferOffset, ULONG *length)
{

	PHYSICAL_ADDRESS ret_addr = 0;
#ifdef AIC_DMA
	struct scsi_pkt *pktp;
	struct aic_scsi_cmd * cmdp;
	struct buf *bp;
	struct aic *aicp;
	ddi_dma_cookie_t *dmap;

	pktp = (struct scsi_pkt *) scb->osRequestBlock;

#ifdef AIC_DEBUG
	if(aic_debug & DDMA)
		PRINT0("GetPhysicalAddress : bufferOffset :0x%x & pkt_resid : 0x%x\n"
				,bufferOffset, pktp->pkt_resid);
#endif
	if ( bufferOffset >= pktp->pkt_resid )
		return ( 0 );
	dmap = PKTP2PTGTP(pktp)->sg;
	*length = dmap->dmac_size;
	ret_addr = (PHYSICAL_ADDRESS)(dmap->dmac_address);
	if ( (dmap->dmac_size + bufferOffset) < pktp->pkt_resid )
	{
#ifdef AIC_DEBUG
	if(aic_debug & DDMA)
		PRINT0("GetPhysicalAddress Incrementing scat-gath ptr \n");
#endif
		dmap++;
		PKTP2PTGTP(pktp)->sg = dmap;
	}
#endif
	return ret_addr;
}

/*
 * Name			: HIM6X60GetVirtualAddress()
 * Purpose		: To return the virtual address of the data area.
 * Called from	: HIM module
 * Arguments	: HACB *hacb, SCB *scb, void *virtualAddress, 
 *				  ULONG bufferOffset, ULONG *length
 * Returns		: Virtual address
 * Side effects	: None
 */

VOID *
HIM6X60GetVirtualAddress(HACB *hacb, SCB *scb, VOID *virtualAddress,
			       ULONG bufferOffset, ULONG *length)
{
	return ( (VOID *)virtualAddress );
}


/*
 * Name			: HIM6X60Delay()
 * Purpose		: To return Suspend the processing.
 * Called from	: HIM module
 * Arguments	: ULONG delay
 * Returns		: None
 * Side effects	: None
 */

VOID
HIM6X60Delay(ULONG delay)
{
	drv_usecwait((clock_t)(5*delay));
}


/*
 * Name			: HIM6X60CompleteSCB()
 * Purpose		: HIM module invokes this routine after it gets the target
 *				  status from the target device.  This acts as an interface
 *				  between Driver developer and HIM.  It extracts the data
 *	   			  from the SCB structure and updates the scsi_pkt structure
 *			      and calls packet completion routine. 
 * Called from	: HIM module
 * Arguments	: HACB *hacb, SCB *scb
 * Returns		: None
 * Side effects	: None
 */

VOID
HIM6X60CompleteSCB(HACB *hacb, SCB *scb)
{
	struct scsi_pkt *pktp;
	struct aic_scsi_cmd *cmd;
	int target,lun,index;
	struct aic * aicp;
	int i;
	kmutex_t *aic_ctrlr_mutex;
	struct buf *bp;

#ifdef AIC_DEBUG
	if (aic_debug & DINIT)
	cmn_err(CE_CONT,"Inside CompleteSCB\n");
#endif


	if( scb->cdb )		/* SCB representing a SCSI command */
	{
		pktp = (struct scsi_pkt *)scb->osRequestBlock;	/* Get scsi_pkt */
		cmd = PKT2CMD(pktp);
		pktp->pkt_resid = scb->transferResidual;	/* Bytes not Xferred */

#ifdef AIC_DEBUG
	if (aic_debug & (DDATAIN | DDATAOUT) )
	{
		PRINT0("scb TranResid and tran Len : 0x%x , 0x%x\n 0x%x",
				scb->transferResidual, scb->transferLength, scb->dataLength);
		PRINT0("Pkt_resid in CompleteSCB routine : 0x%x\n", pktp->pkt_resid);
		PRINT0("scb_status = 0x%x, targetStatus = 0x%x\n", scb->scbStatus,
				scb->targetStatus);
	}
#endif

	/*
	 * Depending on SCB status, set pkt_reason, state or statistics fields
	 * in scsi_pkt and call COMPletion routine.
	 */

		 switch(SCB_STATUS(scb->scbStatus))
		 {
			case SCB_PENDING:
				break;
	
			case SCB_COMPLETED_OK:
				SET_REASON(CMD_CMPLT);
				SET_STATE(STAT_GOT_STATUS);
				SET_STAT(0);
				break;
	
			case SCB_ABORTED:
#ifdef AIC_TIMEOUT
				if( pktp->pkt_reason != CMD_TIMEOUT )
				{
#endif
					SET_REASON(CMD_ABORTED);
					SET_STAT(STAT_ABORTED);
#ifdef AIC_TIMEOUT
				}
				else
				{
					SET_REASON(CMD_TIMEOUT);
					SET_STAT(STAT_TIMEOUT);
				}
#endif
				break;
	
			case SCB_ABORT_FAILURE:
				SET_REASON(CMD_ABORT_FAIL);
				break;
	
			case SCB_ERROR:
				/* FALLTHROUGH */
			case SCB_BUSY:
				/* FALLTHROUGH */
			case SCB_INVALID_SCSI_BUS:
				break;
			
			case SCB_TIMEOUT:
				/* FALLTHROUGH */
			case SCB_SELECTION_TIMEOUT:
				SET_REASON(CMD_TIMEOUT);
				SET_STAT(STAT_TIMEOUT);
				break;
	
			case SCB_SCSI_BUS_RESET:
				SET_REASON(CMD_RESET);
				SET_STAT(STAT_BUS_RESET);
				break;
	
			case SCB_PARITY_ERROR:
				SET_STAT(STAT_PERR);
				break;
	
			case SCB_REQUEST_SENSE_FAILURE:
#ifdef AIC_DEBUG
	if(aic_debug & (DDATAIN | DDATAOUT ))
				PRINT0("Request sense failed \n");
#endif
				break;
	
			case SCB_DATA_OVERRUN:
#ifdef AIC_DEBUG
				PRINT0("Data Overrun\n");
#endif AIC_DEBUG
				SET_REASON(CMD_DATA_OVR);
				SET_STATE(STAT_GOT_STATUS);
				break;
	
			case SCB_BUS_FREE:
#ifdef AIC_DEBUG
				PRINT0("Unexpected BUs Free\n");
#endif AIC_DEBUG
				SET_REASON(CMD_UNX_BUS_FREE);
				break;
	
			case SCB_PROTOCOL_ERROR:
				/* FALLTHROUGH */
			case SCB_INVALID_LENGTH:
				/* FALLTHROUGH */
			case SCB_INVALID_LUN:
				/* FALLTHROUGH */
			case SCB_INVALID_TARGET_ID:
				/* FALLTHROUGH */
			case SCB_INVALID_FUNCTION:
				/* FALLTHROUGH */
			case SCB_ERROR_RECOVERY:
				/* Our mistake ... */
				break;
	
			case SCB_TERMINATED:
#ifdef AIC_DEBUG
				PRINT0("Some Problem : Terminated\n");
#endif AIC_DEBUG
				SET_REASON(CMD_INCOMPLETE);
				break;
			
			case SCB_TERMINATE_IO_FAILURE:
				break;
	
			case SCB_SENSE_DATA_VALID:
				break;
	
			default:
#ifdef AIC_DEBUG
	if(aic_debug & (DDATAIN | DDATAOUT ))
				PRINT0("Invalid scb_status - 0x%x\n", scb->scbStatus);
#endif
				break;
		}
	
		if (scb->scbStatus & SCB_SENSE_DATA_VALID)
		{
			union {
				struct scsi_status st;
				UCHAR val;
			} tstat;
	

			/* The target status is not set in interpret msg in for ARQ */
			tstat.val = scb->targetStatus;
			((struct scsi_arq_status *)(pktp->pkt_scbp))->sts_status = tstat.st;
	
			/*
			 * The target status for the Req.Sense SCB is in hacb->targetStatus
		     */
			tstat.val = hacb->targetStatus;
			((struct scsi_arq_status *)(pktp->pkt_scbp))->sts_rqpkt_status  
					  = tstat.st;
	
			PKT_SCBP->sts_rqpkt_resid = scb->transferResidual;
			PKT_SCBP->sts_rqpkt_state = ARQ_DONE_STATUS;		/* ARQ_DONE */

		}
		else
		{
			*(pktp->pkt_scbp) = scb->targetStatus;		/* Set status byte */
		}

		target = scb->targetID;
		lun = scb->lun;
		aicp = PKT2AIC(pktp);

		index = ddi_get_instance ( aicp->aic_dip );
		aic_ctrlr_mutex = (kmutex_t *)
						  ddi_get_soft_state(aic_state_mutex_p, index);
	
		/*** UNTIMEOUT HERE ***/
	
#ifdef AIC_TIMEOUT
		if( pktp->pkt_reason != CMD_TIMEOUT )
		{
			if(  cmd->aic_timeout_id )
			{
				untimeout( cmd->aic_timeout_id );
			}
		}
#endif

		cmd->cmd_flags = CFLAG_COMPLETED;
		if ( pktp->pkt_flags & FLAG_NOINTR )
		{
#ifdef AIC_DEBUG
			PRINT0("In CompleteSCB for NOINTR pkt, setting flag\n");
#endif
			cmd->nointr_pkt_comp = TRUE;
		}
		else if (pktp->pkt_comp ) 
		{
			mutex_exit(aic_ctrlr_mutex);
			if( pktp->pkt_comp)
				(*pktp->pkt_comp)(pktp);
			mutex_enter(aic_ctrlr_mutex);
			/***
			ddi_run_callback(&cmd->pkt_cb_id);
			***/
		}
		else 
		{
			/* Is this required ?? */
			cv_signal(&aicp->pt[ target * NLUNS + lun].pt_cv);
		}
	}
	else if ( scb->function == SCB_ABORT_REQUESTED ||
			  scb->function == SCB_BUS_DEVICE_RESET )
	{
		/*
		 * Check the status !!!
		 */

		target = scb->targetID;
		lun = scb->lun;
		/*
		 * For the reset/abort packets we set the aic struct pointer in the
		 * osRequestBlock
		 */
		aicp = (struct aic *)scb->osRequestBlock;

		/* Free the SCBs requesting an abort/reset */
		kmem_free((caddr_t)scb,sizeof(*scb));

		cv_signal(&aicp->pt[ target * NLUNS + lun].pt_cv);

	}
}

/*
 * Name			: HIM6X60GetLUCB()
 * Purpose		: Returns a pointer to a LUCB structure for the supplied 
 *				  target and lun combination.
 * Called from	: HIM module
 * Arguments	: HACB *hacb, UCHAR scsiBus, UCHAR targetID, UCHAR lun
 * Returns		: Pointer to an LUCB structure
 * Side effects	: None
 */

LUCB *
HIM6X60GetLUCB(HACB *hacb, UCHAR scsiBus, UCHAR targetID, UCHAR lun)
{
#ifdef AIC_DEBUG
	PRINT0("In Get LUCB routine\n");
#endif
	return (&(hacb->lucb[targetID][lun]));
}

/**********************************************************************
						STUB  ROUTINES
**********************************************************************/

/*
 * Name			: HIM6X60Watchdog(), HIM6X60FlushDMA(), HIM6X60MapDMA()
 *				  HIM6X60Event()
 * Purpose		: STUB routines provided to HIM module
 * Called from	: HIM module
 * Arguments	: ... 
 * Returns		: None
 * Side effects	: None
 */

VOID
HIM6X60Watchdog(HACB *hacb, VOID (*watchdogProcedure)(HACB *hacb),
		     ULONG microseconds)
{
	return;
}

VOID
HIM6X60FlushDMA(HACB *hacb)
{

#ifdef AIC_DMA
	struct aic *aicp;
	dev_info_t *dip;
	struct scsi_pkt	*pktp;
	pktp= (struct scsi_pkt *) (hacb->ne_u.ne_s.activeScb->osRequestBlock);

	/*
	 * Using activeScb to get aicp		********** Is it ok ? ***********
	 */
	aicp = PKT2AIC(pktp);

	dip = aicp->aic_dip;				

#ifdef AIC_DEBUG
	if(aic_debug & DDMA)
		PRINT0("In Flush DMA routine\n");
#endif

	ddi_dmae_disable(dip,(INT) hacb->dmaChannel);
	ddi_dmae_release(dip,(INT) hacb->dmaChannel);
#endif
	return;
}

VOID
HIM6X60MapDMA(HACB *hacb, SCB *scb, VOID *virtualAddress,
		   PHYSICAL_ADDRESS physicalAddress, ULONG length,
		   BOOLEAN memoryWrite)
{

#ifdef AIC_DMA

	struct aic *aicp;
	dev_info_t *dip;
	struct scsi_pkt *pktp;
	struct aic_scsi_cmd *cmd;
	ddi_dma_cookie_t *dmap;
	int cnt,total_bytes;
	struct ddi_dmae_req dma_req;
	int i;
	char *cp;


	pktp = (struct scsi_pkt *)scb->osRequestBlock;
	cmd = PKT2CMD(pktp);
	aicp = PKT2AIC(pktp);
	dip = aicp->aic_dip;				
	dmap = (ddi_dma_cookie_t *)physicalAddress;

#ifdef AIC_DEBUG
	if( aic_debug & DDMA)
	{
		PRINT0("In Map DMA routine\n");
		PRINT0("add = 0x%x :length = 0x%x : direction = %d\n", physicalAddress,
					length, memoryWrite);
		PRINT0("Value of the dmap (sg ptr) : %x\n",dmap);
		PRINT0("dmac size : %x\n",dmap->dmac_size);
		PRINT0("dmac address : %x\n",dmap->dmac_address);
		PRINT0("dmac type : %x\n",dmap->dmac_type);
		PRINT0("dmap address : %x\n",dmap);
	}
#endif

	/*
	 * We are ignoring the address and length supplied as parameters.
	 */
		cp = (char *)&dma_req;
		for (i = 0; i < sizeof(struct ddi_dmae_req); i++)
		{
			*(cp + i) = 0;
		}
		/*
		 * memoryWrite is False for DATAOUT operations i.e from system
		 * memory to HBA to the device and True for DATAIN operations.
		 * A value of DMAE_CMD_WRITE for der_command signifies that the
		 * data be transferred from the system mem to I/O device i.e when
		 * memoryWrite is FALSE and DMAE_CMD_READ otherwise.
		 */

		if (memoryWrite)
			dma_req.der_command = DMAE_CMD_READ;
		else
			dma_req.der_command = DMAE_CMD_WRITE;
		dma_req.der_bufprocess = DMAE_BUF_AUTO;
		dma_req.der_step = DMAE_STEP_INC;
		dma_req.der_path = DMAE_PATH_DEF;
		dma_req.der_cycles = DMAE_CYCLES_1;
		dma_req.der_trans = DMAE_TRANS_SNGL;
		dma_req.proc = NULL;
		dma_req.procparms = (void *) 0;

#ifdef AIC_DEBUG
	if(aic_debug &DDMA)
		PRINT0("The channel : 0x%x\n",hacb->dmaChannel);
#endif

		if (ddi_dmae_alloc(dip,(INT)hacb->dmaChannel,DDI_DMA_DONTWAIT,
			(caddr_t )0) != DDI_SUCCESS)
		{
#ifdef AIC_DEBUG
			PRINT0("MapDMA: ddi_dmae_alloc() failure ... ??? \n");
#endif AIC_DEBUG
			return;
		}
		
		if (ddi_dmae_prog(dip, &dma_req,
			(ddi_dma_cookie_t *)dmap, hacb->dmaChannel) != DDI_SUCCESS)
		{
#ifdef AIC_DEBUG
			PRINT0("MapDMA: ddi_dmae_prog() failure ... ??? \n");
#endif AIC_DEBUG
		}
		
		/* Dma channel is now programmed, so start DMA operation */
		HIM6X60DmaProgrammed(hacb);

#endif AIC_DMA

		return;
}

VOID
HIM6X60Event(HACB *hacb, UCHAR event, int val)
{
   return;
}

/**********************************************************************
					UTILITY ROUTINES
				memset, memcmp, memcpy
**********************************************************************/

VOID *
memset(VOID *ptr,UCHAR val,USHORT size) 
{
	
	int i;
	char *myptr = (char *)ptr;
	for( i = 0; i < size; i++ )
		*(myptr + i) = val;
	return ( (VOID *)myptr );
}

INT 
memcmp(const VOID *ptr1, const VOID *ptr2, USHORT size) 
{
	return ( bcmp( (char *)ptr1, (char *)ptr2, (size_t) size ) );
}

VOID *
memcpy(VOID *dest,const VOID *src,USHORT size) 
{
	bcopy( (caddr_t)src, (caddr_t)dest, (size_t) size );
	return ( dest );
}
	
/**********************************************************************
					DEBUGGING ROUTINES
**********************************************************************/

#ifdef AIC_DEBUG
print_offsets(HACB *hacb)
{
	PRINT0("Offsets of HACB structure members :\n");
	PRINT0("\tLUCB: 0x%x\n",  (int)hacb->lucb - (int)hacb );
	PRINT0("\tmsgOut: 0x%x\n",(int)hacb->ne_u.ne_s.msgOut - (int)hacb );
	PRINT0("\tmsgIn : 0x%x\n",(int)hacb->ne_u.ne_s.msgIn - (int)hacb );
	PRINT0("targetStatus 0x%x\n",  (int)&hacb->targetStatus - (int)hacb );
	PRINT0("cActiveScb is 0x%x,\n",  (int)&hacb->cActiveScb - (int)hacb );
}
#endif

#ifdef AIC_DEBUG
aic_dump_data(struct scsi_pkt *pktp)
{
	struct aic_scsi_cmd *cmd;
	int i,j;
	struct buf *bp;

	cmd = PKT2CMD(pktp);
	bp = (struct buf *) cmd->cmd_private;

	for (i=0; i < bp->b_bcount; i ++) 
	{
		PRINT0("%x ",*(bp->b_un.b_addr+i));
	}
}
#endif
