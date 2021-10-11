/*
 * eepro  - Solaris device driver for Intel EtherExpress Pro Ethernet
 *
 * Copyright (c) 1995, Sun Microsystems, Inc. All rights reserved.
 */

/*
 * NAME
 *        eepro.c ver. 1.1
 *	  eepro.c ver. 1.2 (sccs) fix false probe problems by checking
 *			    the round robin counter while getting the
 *			    signature.
 *
 *
 * SYNOPIS
 *        Source code of the driver for the Intel EtherExpress Pro Ethernet
 * LAN adapter board on Solaris 2.4 (x86)
 *        Depends on the gld module of Solaris 2.4 (Generic LAN Driver)
 *
 * A> Exported functions.
 * (i) Entry points for the kernel-
 *        _init()
 *        _fini()
 *        _info()
 *
 * (ii) DDI entry points-
 *        eepro_identify()
 *        eepro_devinfo()
 *        eepro_probe()
 *        eepro_attach()
 *        eepro_detach()
 *        eepro_force_reset()
 *
 * (iii) Entry points for gld-
 *        eepro_reset()
 *        eepro_start_board()
 *        eepro_stop_board()
 *        eepro_saddr()
 *        eepro_dlsdmult()
 *        eepro_intr()
 *        eepro_prom()
 *        eepro_gstat()
 *        eepro_send()
 *        eepro_ioctl()
 *
 * B> Imported functions.
 * From gld-
 *        gld_register()
 *        gld_unregister()
 *        gld_recv()
 *
 * 
 * DESCRIPTION
 *    The eepro Ethernet driver is a multi-threaded, dynamically loadable,
 * gld-compliant, clonable STREAMS hardware driver that supports the
 * connectionless service mode of the Data Link Provider Interface,
 * dlpi (7) over an Intel EtherExpress-Pro (EEPRO) controller. The driver
 * can support multiple EEPRO controllers on the same system. It provides
 * basic support for the controller such as chip initialization,
 * frame transmission and reception, multicasting and promiscuous mode support,
 * maintenance of error statistic counters and the time domain reflectometry
 * tests.
 *    The eepro driver uses the Generic LAN Driver (gld) module of Solaris,
 * which handles all the STREAMS and DLPI specific functions for the driver.
 * It is a style 2 DLPI driver and supports only the connectionless mode of
 * data transfer. Thus, a DLPI user should issue a DL_ATTACH_REQ primitive
 * to select the device to be used. Refer dlpi (7) for more information.
 *    For more details on how to configure the driver, refer eepro (7).
 *
 *
 * CAVEATS AND NOTES
 *    Maximum number of boards supported is 0x20: hopefully, a
 * system administrator will run out of slots if he wishes to add more
 * than 0x20 boards to the system
 *
 *
 * SEE ALSO
 *  - /kernel/misc/gld
 *  - eepro (7)
 *  - dlpi (7)
 *  - "Skeleton Network Device Drivers",
 *        Solaris 2.1 Device Driver Writer's Guide-- February 1993
 *
 *
 * MODIFICATION HISTORY
 *
 * - Version 1.0 04 July 1994
 *     Released version to Sunsoft.
 *
 *
 * MISCELLANEOUS
 *      vi options for viewing this file::
 *                set ts=4 sw=4 ai wm=4
 *
 *   Global variable naming conventions followed in the file:
 *   a) base_io_address always refers to the start address of I/O registers.
 *   b) eeprop is always a pointer to the driver private structure
 *      eeproinstance, defined in eepro.h.
 *   c) macinfop is always a pointer to the driver private data structure.
 *   d) board_no is the instance number (interface number) of the driver.
 *
 *
 * COPYRIGHTS
 *      This file is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use provided that this legend is included on all tape
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

#pragma ident "@(#)eepro.c	1.2      95/04/25 SMI"

#ifndef lint
static char     sccsid[] = "@(#)gldconfig 1.1 93/02/12 Copyright 1993 Sun Microsystems";
#endif lint

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
#include <sys/eepro/eepro.h>


/*
 *  Declarations and Module Linkage
 */

static char ident[] = "EtherExpress-Pro";

/* 
 * Used for debugging 
 */

#ifdef EEPRODEBUG
int    eeprodebug = -1;
#endif EEPRODEBUG

/* 
 * The table attached_board_addresses contains the I/O addresses of the
 * boards attached 
 */

static int attached_board_addresses[MAX_EEPRO_BOARDS]; 
static ushort no_of_boards_attached; 

/*
 * Required system entry points
 */

static int eepro_identify(dev_info_t *);
static int eepro_devinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static int eepro_probe(dev_info_t *);
static int eepro_attach(dev_info_t *, ddi_attach_cmd_t);
static int eepro_detach(dev_info_t *, ddi_detach_cmd_t);
static int eepro_force_reset(dev_info_t *, ddi_reset_cmd_t);

/*
 * Required driver entry points for GLD
 */

static int   eepro_reset(gld_mac_info_t *);
static int   eepro_start_board(gld_mac_info_t *);
static int   eepro_stop_board(gld_mac_info_t *);
static int   eepro_saddr(gld_mac_info_t *);
static int   eepro_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
static int   eepro_prom(gld_mac_info_t *, int);
static int   eepro_gstat(gld_mac_info_t *);
static int   eepro_send(gld_mac_info_t *, mblk_t *);
static int   eepro_ioctl(queue_t *, mblk_t *);
static u_int eepro_intr(gld_mac_info_t *);


/*
 * Utility functions used by the driver
 */

static void  eepro_init_board(gld_mac_info_t *);
static void  read_eeprom(register int, int, ushort *);
static void  eepro_watchdog(caddr_t);
static void  eepro_rcv_packet(gld_mac_info_t *);
static int   eepro_tdr_test(gld_mac_info_t *);
static int   eepro_wait_exec(register int);

DEPENDS_ON_GLD;        /* this forces misc/gld to load -- DO NOT REMOVE */


/*
 * Standard STREAMS initialization
 */

static struct module_info minfo =
{
    EEPROIDNUM,
    "eepro",
    0,
    INFPSZ,
    EEPROHIWAT,
    EEPROLOWAT
};

/*
 * read queues
 */

static struct qinit rinit =
{
    NULL,
    gld_rsrv,
    gld_open,
    gld_close,
    NULL,
    &minfo,
    NULL
};

/*
 * write queues
 */

static struct qinit winit =
{ 
    gld_wput,
    gld_wsrv,
    NULL,
    NULL,
    NULL,
    &minfo,
    NULL
};

static struct streamtab eeproinfo =
{
    &rinit,
    &winit,
    NULL,
    NULL
};

/*
 * Standard Module linkage initialization for a STREAMS driver
 */

extern struct mod_ops mod_driverops;

static struct cb_ops cb_eeproops =
{
    nulldev,          /* cb_open */
    nulldev,          /* cb_close */
    nodev,            /* cb_strategy */
    nodev,            /* cb_print */
    nodev,            /* cb_dump */
    nodev,            /* cb_read */
    nodev,            /* cb_write */
    nodev,            /* cb_ioctl */
    nodev,            /* cb_devmap */
    nodev,            /* cb_mmap */
    nodev,            /* cb_segmap */
    nochpoll,         /* cb_chpoll */
    ddi_prop_op,      /* cb_prop_op */
    &eeproinfo,       /* cb_stream */
    (int)(D_MP)       /* cb_flag */
};

static struct dev_ops eeproops =
{
    DEVO_REV,                /* devo_rev */
    0,                       /* devo_refcnt */
    eepro_devinfo,           /* devo_getinfo */
    eepro_identify,          /* devo_identify */
    eepro_probe,             /* devo_probe */
    eepro_attach,            /* devo_attach */
    eepro_detach,            /* devo_detach */
    eepro_force_reset,       /* devo_reset */
    &cb_eeproops,            /* devo_cb_ops */
    (struct bus_ops *) NULL  /* devo_bus_ops */
};

static struct modldrv modldrv =
{
    &mod_driverops,        /* Type of module.  This one is a driver. */
    ident,                 /* short description */
    &eeproops              /* driver specific ops */
};

static struct modlinkage modlinkage =
{
    MODREV_1,
    (void *) &modldrv,
    NULL
};




/*
 *                     ROUTINES TO INTERFACE WITH THE KERNEL
 */


/*
 * Name           : _init()
 * Purpose        : Load an instance of the driver
 * Called from    : Kernel
 * Arguments      : None
 * Returns        : Whatever mod_install() returns
 * Side effects   : None
 */

int
_init(void)
{
    return mod_install(&modlinkage);
}


/*
 * Name           : _fini()
 * Purpose        : Unload an instance of the driver
 * Called from    : Kernel
 * Arguments      : None
 * Returns        : Whatever mod_remove() returns
 * Side effects   : None
 */

int
_fini(void)
{
    return mod_remove(&modlinkage);
}


/*
 * Name           : _info()
 * Purpose        : Obtain status information about the driver
 * Called from    : Kernel
 * Arguments      : modinfop - pointer to a modinfo structure that 
 *                             contains information on the module
 * Returns        : Whatever mod_info() returns
 * Side effects   : None
 */

int
_info(struct modinfo *modinfop)
{
    return mod_info(&modlinkage, modinfop);
}



/*
 *                        DDI ENTRY POINTS
 */


/*
 * Name           : eepro_identify()
 * Purpose        : Determine if the driver drives the device specified 
 *                  by the devinfo pointer
 * Called from    : Kernel
 * Arguments      : devinfo - pointer to a devinfo structure
 * Returns        : DDI_IDENTIFIED, if we know about this device
 *                  DDI_NOT_IDENTIFIED, otherwise
 * Side effects   : None
 */

eepro_identify(dev_info_t *devinfo)
{
    if (strcmp(ddi_get_name(devinfo), "eepro") == 0)
    {
        return (DDI_IDENTIFIED);
    }
    else
    {
        return (DDI_NOT_IDENTIFIED);
    }
}


/*
 * Name           : eepro_devinfo()
 * Purpose        : Reports the instance number and identifies the devinfo
 *                  node with which the instance number is associated
 * Called from    : Kernel
 * Arguments      : devinfo - pointer to a devinfo_t structure
 *                  cmd     - command argument: either 
 *                            DDI_INFO_DEVT2DEVINFO or 
 *                            DDI_INFO_DEVT2INSTANCE
 *                  arg     - command specific argument
 *                  result  - pointer to where requested information is 
 *                          stored
 * Returns        : DDI_SUCCESS, on success
 *                  DDI_FAILURE, on failure
 * Side effects   : None
 * Miscellaneous  : This code is not DDI compliant: the correct semantics
 *                  for CLONE devices is not well-defined yet.
 */

eepro_devinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
    register int error;

    switch (cmd) {
    case DDI_INFO_DEVT2DEVINFO:
        if (devinfo == NULL)
        {
            error = DDI_FAILURE;    /* Unfortunate */
        }
        else
        {
            *result = (void *)devinfo;
            error = DDI_SUCCESS;
        }
        break;

    case DDI_INFO_DEVT2INSTANCE:
        *result = (void *) 0;    /* This CLONEDEV always returns zero */
        error = DDI_SUCCESS;
        break;

    default:
        error = DDI_FAILURE;
    }
    return (error);
}


/*
 * Name           : eepro_probe()
 * Purpose        : Determine if the network controller is present 
 *                  on the system
 * Called from    : Kernel
 * Arguments      : devinfo - pointer to a devinfo structure
 * Returns        : DDI_PROBE_SUCCESS, if the controller is detected
 *                  DDI_PROBE_FAILURE, otherwise
 * Side effects   : None
 */

eepro_probe(dev_info_t *devinfo)
{
    register int base_io_address;
    int      found_board = 0; /* flag */
    char     signature[4];    /* board signature */
    int      i, j;            /* scratch variables */
	unsigned char highcnt = 0; /* high bit counter */

    /*
     * Table of all supported I/O base addresses
     */

    static uint base_io_tab[] = { 0x200, 0x210, 0x220, 0x230, 0x240, 0x250,
                                  0x260, 0x280, 0x290, 0x2a0, 0x2b0, 0x2c0,
                                  0x2d0, 0x2e0, 0x2f0, 0x300, 0x310, 0x320,
                                  0x330, 0x340, 0x350, 0x360, 0x370, 0x380,
                                  0x390
                                };
    int     tab_size  = sizeof (base_io_tab) / sizeof (uint);
    int     regbuf[3];         /* to create I/O address property */

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_DDI)
    {
        cmn_err(CE_CONT, "eepro_probe(0x%x)\n", devinfo);
    }
#endif EEPRODEBUG

    for (i = 0; i < tab_size; i++)
    {
        base_io_address = base_io_tab[i];
#ifdef EEPRODEBUG
		if (eeprodebug & DEBUG_DDI){
			cmn_err(CE_CONT,"eepro: probing address =0x%x\n",base_io_address);
		}
#endif EEPRODEBUG

        /*
         * Do not probe at an I/O address if a board is already attached
         * at base_io_address
         */

        for (j = 0; j < no_of_boards_attached; j++)
        {
            if (attached_board_addresses[j] == base_io_address)
            {
                break;
            }
        }
        if (j < no_of_boards_attached)
        {
            continue;
        }

        /*
         * Probe for the board to see if it's there.
         * Read in SIGLEN bytes of the signature and map them in such a way
         * that it becomes an ascii string.
         * Note: normally, one would map in the device memory here, but
         * we don't need to since the EEPRO is I/O mapped.
         */

        for (j = 0; j < SIGLEN; j++)
        {
			unsigned char temp_sig;
    		unsigned char highbits;		/* top two high bits from temp_sig */

            temp_sig = inb(base_io_address + ID_REG);
			highbits = (temp_sig >> 6 ) & 0x3 ;		/* top two bits of sig */
			
			if (j == 0 )
				highcnt = highbits;		/* start with current counter 	*/
			else
				highcnt = ++highcnt % 4;

            /*
             * In addition to the signature, check if the two least
             * significant (reserved) bits of the ID_REG are zero and
			 * that the top two bits match the counter value j (these
			 * bits are a counter which is incremented on each access).
			 * If either case isn't true then current probe fails.
             */

#ifdef EEPRODEBUG
			if (eeprodebug & DEBUG_TRACE){
				cmn_err(CE_CONT, "highbits=0x%x, highcnt=0x%x, temp_sig=0x%x\n",highbits, highcnt, temp_sig);
			}
#endif EEPRODEBUG

            if (((temp_sig & 0x03) == 0) && (highbits == highcnt))
            {
                signature[j] = (temp_sig >> 2) & 0xf;
                if (signature[j] > 9) 
                {
                    signature[j] += (- 0xa + 'a');
                }
                else
                {
                    signature[j] += '0';
                }
            }
            else
            {
                 goto probe_next_board;
            }
        }

        /* 
         * "dddd" and "9999" are valid signatures 
         */

        if ((strncmp(signature, "dddd", SIGLEN) == 0)
            || (strncmp(signature, "9999", SIGLEN) == 0))
		{
#ifdef EEPRODEBUG
            if (eeprodebug & (DEBUG_DDI|DEBUG_TRACE))
            {
                int board_no = ddi_get_instance(devinfo);

                cmn_err(CE_CONT,
                    "eepro%d: board found at io addr 0x%x with signature ",
                    board_no, base_io_address);
                for (j = 0; j < SIGLEN; j++)
                {
                    cmn_err (CE_CONT, "%x", signature[j]);
                }
                cmn_err(CE_CONT, "\n");
            }
#endif EEPRODEBUG

            regbuf[0] = base_io_address;
            (void) ddi_prop_create(DDI_DEV_T_NONE, devinfo, DDI_PROP_CANSLEEP,
                        "ioaddr", (caddr_t) regbuf, sizeof (int));
            found_board++;
            break;
        }

probe_next_board: ;

    }

    /*
     *  Return whether the board was found.  If unable to determine
     *  whether the board is present, return DDI_PROBE_DONTCARE.
     */

    if (found_board)
    {
#ifdef EEPRODEBUG
		if (eeprodebug & DEBUG_TRACE){
			cmn_err(CE_CONT,"eepro_probe: returning DDI_PROBE_SUCCESS\n");
		}
#endif
        return (DDI_PROBE_SUCCESS);
    }
    else
    {

#ifdef EEPRODEBUG
		if (eeprodebug & DEBUG_TRACE){
			cmn_err(CE_CONT,"eepro_probe: returning DDI_PROBE_FAILURE\n");
		}
#endif
        return (DDI_PROBE_FAILURE);
    }
}


/*
 * Name           : eepro_attach()
 * Purpose        : Attach a driver instance to the system. This 
 *                  function is called once for each board successfully 
 *                  probed.
 *                  gld_register() is called after macinfop's fields are
 *                  initialized, to register the driver with gld.
 * Called from    : Kernel
 * Arguments      : devinfo - pointer to the device's devinfo structure
 *                  cmd     - should be set to DDI_ATTACH
 * Returns        : DDI_SUCCESS on success
 *                  DDI_FAILURE on failure
 * Side effects   : macinfop is initialized before calling gld_register()
 */

eepro_attach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
    gld_mac_info_t *macinfop;        
    struct   eeproinstance *eeprop;  
    register int base_io_address; 
    int      board_no = ddi_get_instance(devinfo);
    ushort   intr_value;     /* interrupt value */
    ushort   map_table;      /* bit array to map intr value to irq value */
    ushort   port_info1;     /* Connector type info from EEPROM (in reg 1) */
    ushort   port_info2;     /* Connector type info from EEPROM (in reg 5) */
    int      rval, len;      /* scratch */
    int      i, count;       /* scratch */
    unchar   val;            /* scratch */
    int      irq_value;      /* irq value */
    unchar   bank;           /* current bank in which 82595TX is */
    struct intrprop
    {
        int    spl;       /* priority level */
        int    irq;       /* irq level */
    } *intrprop;


#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_DDI)
    {
        cmn_err(CE_CONT, "eepro_attach(0x%x)", devinfo);
    }
#endif EEPRODEBUG

    if (cmd != DDI_ATTACH)
    {
        return (DDI_FAILURE);
    }

    /*
     *  Allocate gld_mac_info_t and eeproinstance structures
     */

    macinfop = (gld_mac_info_t *) kmem_zalloc(sizeof (gld_mac_info_t) +
                           sizeof (struct eeproinstance), KM_NOSLEEP);

    if (macinfop == NULL)
    {
        return (DDI_FAILURE);
    }
    eeprop = (struct eeproinstance *) (macinfop + 1);

    /*
     * Initialize our private fields in macinfop and eeproinstance
     */

    macinfop->gldm_private = (caddr_t) eeprop;
    macinfop->gldm_devinfo = devinfo;
    base_io_address = macinfop->gldm_port =
            ddi_getprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
                        "ioaddr", 0);
    macinfop->gldm_state = EEPRO_IDLE;
    macinfop->gldm_flags = 0;

    /*
     *  Initialize pointers to device specific functions which will be
     *  used by the generic layer.
     */

    macinfop->gldm_reset   = eepro_reset;
    macinfop->gldm_start   = eepro_start_board;
    macinfop->gldm_stop    = eepro_stop_board;
    macinfop->gldm_saddr   = eepro_saddr;
    macinfop->gldm_sdmulti = eepro_dlsdmult;
    macinfop->gldm_prom    = eepro_prom;
    macinfop->gldm_gstat   = eepro_gstat;
    macinfop->gldm_send    = eepro_send;
    macinfop->gldm_intr    = eepro_intr;
    macinfop->gldm_ioctl   = eepro_ioctl;

    /*
     * Initialize board characteristics needed by the generic layer
     */

    macinfop->gldm_ident   = ident;
    macinfop->gldm_type    = DL_ETHER;
    macinfop->gldm_minpkt  = 0;
    macinfop->gldm_maxpkt  = EEPROMAXPKT;
    macinfop->gldm_addrlen = ETHERADDRL;
    macinfop->gldm_saplen  = -2;

    /*
     * Do anything necessary to prepare the board for operation short of
     * actually starting the board.
     */

    bank = inb(base_io_address + CMD_REG) >> 6;
    if (bank != 0)
    {
        outb(base_io_address + CMD_REG, SEL_BANK0 | CR_SWITCH_BANK);
        drv_usecwait(20);
    }

    /*
     * Wait for the execution unit to become idle 
     */

    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_BOARD)
        {
            cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                    board_no, __LINE__);
        }
#endif EEPRODEBUG

            outb(base_io_address + CMD_REG, CR_ABORT);
            drv_usecwait(100);

    }

    outb(base_io_address + CMD_REG, CR_RESET);
    drv_usecwait(300);

    count = 0;
    while (((inb(base_io_address + STAT_REG) & STAT_EXEC_INT) == 0) &&
            (count++ <= 100))
    {
        drv_usecwait(50);
    }

    if (count > 100)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_BOARD)
        {
			cmn_err(CE_NOTE,
					"eepro%d: board not responding -- attach failed\n",
                    board_no);
		}
#endif EEPRODEBUG

        return (DDI_FAILURE);
    }

    EEPRO_CLR_EXEC_STAT(base_io_address);    /* clear status register */

    /* 
     * Select I/O bank 2
     */

    outb(base_io_address + CMD_REG, (SEL_BANK2 | CR_SWITCH_BANK));
    drv_usecwait(25);

    /*
     * Get the board's vendor-assigned hardware network address
     */

    for (i = 0; i < ETHERADDRL / 2; i++)
    {
        ushort eaddr;

        read_eeprom(base_io_address, (EEPROM_REG2 + 2 - i), &eaddr);
        macinfop->gldm_vendor[2 * i] = ((ushort) (eaddr & 0xff00)) >> 8;
        macinfop->gldm_vendor[2 * i + 1] = eaddr & 0xff;
    }

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_BOARD)
    {
        cmn_err(CE_CONT, "eepro%d: ethernet address in EEPROM is ",
                board_no);
        EEPRO_PRINT_EADDR(macinfop->gldm_vendor);
        cmn_err(CE_CONT,"\n");
    }
#endif EEPRODEBUG


    /*
     * Read configured interrupt value from the EEPROM 
     */

    read_eeprom(base_io_address, EEPROM_REG1, &intr_value); 
    read_eeprom(base_io_address, EEPROM_REG7, &map_table);
    intr_value &= EEPROM_INT_SEL_MASK;

    /*
     * Save the interrupt value for use in eepro_init_board() later
     */

    eeprop->intr_level = intr_value;

    count = irq_value = -1;
    for (i = 0; i < 16; i++)
    {
        if ((map_table >> i) & 1)
        {
            count++;
        }    
        if (count == intr_value)
        {
    		/*
			 * IRQ levels 2 and 9 are tied to each other; since Solaris does
			 * not run on PC/XTs, we forcibly select irq 9
			 */

			if (i == 2)
			{
				irq_value = 9;
			}
			else
			{
				irq_value = i;
			}
            break;
        }
    }

    /*
     * Compare the irq_value with the value of the intr property in the
     * eepro.conf file
     */

    rval = ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
                           "intr", (caddr_t) &intrprop, &len);
    if (rval != DDI_PROP_SUCCESS)
    {
        cmn_err(CE_NOTE,
                "eepro%d: no intr property in .conf file-- attach failed",
                board_no);
        kmem_free((caddr_t) macinfop,
            sizeof (gld_mac_info_t) + sizeof (struct eeproinstance));
        return (DDI_FAILURE);
    }
    for (i = 0; i < (len / (sizeof (struct intrprop))); i++)
    {
        /* 
         * Check if the irq_level read from EEPROM matches the value given
         * in the .conf file
         */

        if (irq_value == intrprop[i].irq)
        {
            break;
        }
    }
    if (i < (len / (sizeof (struct intrprop))))
    {
        kmem_free(intrprop, len);
        macinfop->gldm_irq_index = i;
    }
    else 
    {
        /* 
         * irq value in eepro.conf file does not match that in EEPROM 
         */

        kmem_free(intrprop, len);
        kmem_free((caddr_t) macinfop,
            sizeof (gld_mac_info_t) + sizeof (struct eeproinstance));
        return (DDI_FAILURE);
    }

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_BOARD)
    {
        cmn_err(CE_CONT, "eepro%d: intr value :: %d, irq value :: %d\n", 
                board_no, intr_value, irq_value);
    }
#endif EEPRODEBUG

    /*
     * Set the connector/media type if it can be determined
     */

    read_eeprom(base_io_address, EEPROM_REG1, &port_info1); 
    read_eeprom(base_io_address, EEPROM_REG5, &port_info2); 

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_BOARD)
    {
        cmn_err(CE_CONT, "eepro%d: port_info1: 0x%x, port_info2: 0x%x ",
                board_no, port_info1, port_info2);
    }
#endif EEPRODEBUG

    eeprop->autoport_flag = FALSE;

    if ((port_info1 & EEPROM_APORT_MASK) != EEPROM_APORT_MASK)
    {
        eeprop->autoport_flag = TRUE;

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_BOARD)
        {
            cmn_err(CE_CONT, "(autodetect)\n");
        }
#endif EEPRODEBUG

    }
    else if ((port_info1 & EEPROM_TPE_AUI_MASK) &&
        ((port_info2 & EEPROM_TPE_BNC_MASK) == 0) &&
        (port_info2 & EEPROM_CONNECT_TPE))
    {
        macinfop->gldm_media  = GLDM_TP;

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_BOARD)
        {
            cmn_err(CE_CONT, "(TPE)\n");
        }
#endif EEPRODEBUG

    }
    else if ((port_info1 & EEPROM_TPE_AUI_MASK) == 0 &&
             (port_info2 & EEPROM_CONNECT_AUI))
    {
        macinfop->gldm_media  = GLDM_AUI;

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_BOARD)
        {
            cmn_err(CE_CONT, "(AUI)\n");
        }
#endif EEPRODEBUG

    }
    else if ((port_info2 & EEPROM_TPE_BNC_MASK) &&
             (port_info2 & EEPROM_CONNECT_BNC))
    {
        macinfop->gldm_media  = GLDM_BNC;

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_BOARD)
        {
            cmn_err(CE_CONT, "(BNC)\n");
        }
#endif EEPRODEBUG

    }
    else
    {
        cmn_err(CE_NOTE,
           "eepro%d: connector type could not be determined -- attach failed",
           board_no);
        kmem_free((caddr_t) macinfop, sizeof (gld_mac_info_t) +
                  sizeof (struct eeproinstance));
        return (DDI_FAILURE);
    }

    bcopy((caddr_t) gldbroadcastaddr,
        (caddr_t) macinfop->gldm_broadcast, ETHERADDRL);
    bcopy((caddr_t) macinfop->gldm_vendor,
        (caddr_t) macinfop->gldm_macaddr, ETHERADDRL);

    outb(base_io_address + CMD_REG, CR_SWITCH_BANK | SEL_BANK0);
    drv_usecwait(300);
    EEPRO_CLR_EXEC_STAT(base_io_address);    /* clear status register */

    /*
     * Initialize the board
     */

    eepro_init_board(macinfop);

    EEPRO_CLR_EXEC_STAT(base_io_address);

    /*
     *  Register ourselves with the GLD interface
     *
     *  gld_register will:
     *    link us with the GLD system;
     *    set our ddi_set_driver_private(9F) data to the macinfop pointer;
     *    save the devinfo pointer in macinfop->gldm_devinfo;
     *    map the registers, putting the kvaddr into macinfop->gldm_memp;
     *    add the interrupt, putting the cookie in gldm_cookie;
     *    init the gldm_intrlock mutex which will block that interrupt;
     *    create the minor node.
     */
    
    if (gld_register(devinfo, "eepro", macinfop) == DDI_SUCCESS)
    {
        /*
         * Check if shared memory can be accessed by performing a
         * read/write test to memory
         */

        outb(base_io_address + CMD_REG, SEL_BANK0 | CR_SWITCH_BANK);
        outw(base_io_address + HAR_LOW, 0);
        val = inb(base_io_address + INTR_REG); 
        outb(base_io_address + INTR_REG, val | ENAB_32_BIT_IO);
        for (i = 0; i < 100; i++)
        {
            outl(base_io_address + PORT_32_BIT_IO, 0xabcddcba + i);
        }

        outb(base_io_address + INTR_REG, val & ~ENAB_32_BIT_IO);
        outw(base_io_address + HAR_LOW, 0);
        outb(base_io_address + INTR_REG, val | ENAB_32_BIT_IO);

        for (i = 0; i < 100; i++)
        {
            ulong tmp;

            tmp = inl(base_io_address + PORT_32_BIT_IO);
            if (tmp != (0xabcddcba + i))
            {
                cmn_err(CE_NOTE,
                    "eepro%d: R/W test failed on board memory-- attach failed",
                    board_no);
                if (gld_unregister(macinfop) == DDI_SUCCESS)
                {
                    kmem_free((caddr_t) macinfop, sizeof (gld_mac_info_t) +
                              sizeof (struct eeproinstance));
                    return (DDI_FAILURE);
                }
            }
        }
        outb(base_io_address + INTR_REG, val & ~ENAB_32_BIT_IO);
        attached_board_addresses[no_of_boards_attached++] = base_io_address;

        eeprop->timeout_id =
             timeout(eepro_watchdog, (caddr_t) macinfop, EEPRO_WDOG_TICKS);
        eeprop->eepro_watch = 0;

        return (DDI_SUCCESS);
    }
    else
    {
        cmn_err(CE_NOTE, "eepro%d: gld_unregister() failed-- attach failed",
                board_no);
        kmem_free((caddr_t) macinfop,
            sizeof (gld_mac_info_t) + sizeof (struct eeproinstance));
        return (DDI_FAILURE);
    }
}


/*
 * Name           : eepro_detach()
 * Purpose        : Detach a driver instance from the system. This 
 *                  includes unregistering the driver from gld
 * Called from    : Kernel
 * Arguments      : devinfo - pointer to the device's dev_info structure
 *                  cmd     - type of detach, should be DDI_DETACH always
 * Returns        : DDI_SUCCESS if the instance associated with the given 
 *                              device was successfully removed
 *                  DDI_FAILURE otherwise
 * Side effects   : None
 */

eepro_detach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
    gld_mac_info_t *macinfop;
    struct eeproinstance *eeprop;

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_DDI)
    {
        cmn_err(CE_CONT, "eepro_detach(0x%x)", devinfo);
    }
#endif EEPRODEBUG

    if (cmd != DDI_DETACH)
    {
        return (DDI_FAILURE);
    }

    /*
     * Get the driver private (gld_mac_info_t) structure
     */

    macinfop = (gld_mac_info_t *) ddi_get_driver_private(devinfo);
    eeprop = (struct eeproinstance *)(macinfop->gldm_private);

    /*
     * cancel callbacks to eepro_watchdog()
     */
    
    if (eeprop->timeout_id >= 0)
    {
        if (untimeout(eeprop->timeout_id) == FAILURE)
        {

#ifdef EEPRODEBUG
            if (eeprodebug & DEBUG_WDOG)
            {
                int board_no = ddi_get_instance(devinfo);

                cmn_err(CE_WARN,
                       "eepro%d: cannot cancel timeout (invalid id?)",
                    board_no);
            }
#endif EEPRODEBUG

        }
    }

    /*
     * Stop the board if it is running
     */

    (void) eepro_stop_board(macinfop);

    /*
     *  Unregister ourselves from the GLD interface
     *
     *  gld_unregister will:
     *    remove the minor node;
     *    unmap the registers;
     *    remove the interrupt;
     *    destroy the gldm_intrlock mutex;
     *    unlink us from the GLD system.
     */

    if (gld_unregister(macinfop) == DDI_SUCCESS)
    {
        kmem_free((caddr_t) macinfop,
                  sizeof (gld_mac_info_t) + sizeof (struct eeproinstance));
        return (DDI_SUCCESS);
    }
    return (DDI_FAILURE);
}



/*
 *                      GLD ENTRY POINTS
 */


/*
 * Name           : eepro_reset()
 * Purpose        : Reset the board to its initial state
 * Called from    : gld
 * Arguments      : macinfop - pointer to a gld_mac_info_t structure
 * Returns        : TRUE always
 * Side effects   : All data structures and lists maintained by the 
 *                  82595TX are flushed
 */

static int
eepro_reset(gld_mac_info_t *macinfop)
{

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_TRACE)
    {
        cmn_err(CE_CONT, "eepro_reset(0x%x)", macinfop);
    }
#endif EEPRODEBUG

    (void) eepro_stop_board(macinfop);
    eepro_init_board(macinfop);

    return (TRUE);
}


/*
 * Name           : eepro_start_board()
 * Purpose        : Start the device's receiver and enable interrupts
 * Called from    : gld
 * Arguments      : macinfop - pointer to a gld_mac_info_t structure
 * Returns        : TRUE always
 * Side effects   : Receiver unit of the 82595TX and interrupts get enabled.
 *                  Driver's private data structure gets modified.
 */

static int
eepro_start_board(gld_mac_info_t *macinfop)
{
    register int base_io_address = macinfop->gldm_port;
    unchar   val;    /* scratch */

    /*
     * Acknowledge all pending interrupts and enable interrupts for all
     * events : packet sent, packet received, command execution completed
     * and receive stop register hit
     */

    val = inb(base_io_address + INTR_REG); 

    outb(base_io_address + INTR_REG,
         val & ~(TX_MASK | RX_MASK | RX_STOP_MASK));

    outb(base_io_address + STAT_REG,
         (STAT_EXEC_INT | STAT_RX_INT | STAT_TX_INT | STAT_RX_STOP));

    /*
     * Ensure that the execution unit is idle before issuing the
     * CR_RCV_ENABLE command.
     */

    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_BOARD)
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                    board_no, __LINE__);
        }
#endif EEPRODEBUG

    }

    if (((inb(base_io_address + STAT_REG))
          & (RCVR_ACTIVE_MASK | RCVR_READY_MASK)))
    {

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_BOARD)
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: receiver already ready/active\n",
                    board_no);
        }
#endif EEPRODEBUG

    }
    else
    {
        outb(base_io_address + CMD_REG, CR_RCV_ENABLE);
    }

    drv_usecwait(25);
    
    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_BOARD)
        {
             int board_no = ddi_get_instance(macinfop->gldm_devinfo);

             cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                     board_no, __LINE__);
        }
#endif EEPRODEBUG

    }

    EEPRO_CLR_EXEC_STAT(base_io_address);

    drv_usecwait(200);

    outb(base_io_address + CMD_REG, CR_SWITCH_BANK | SEL_BANK1);
    drv_usecwait(50);
    
    /*
     * Enable interrupts if they are not already enabled
     */

    val = inb(base_io_address + ENAB_INTR_REG);
    if ((val & EEPRO_ENAB_INTR) != EEPRO_ENAB_INTR)
    {
        outb(base_io_address + ENAB_INTR_REG, (val | EEPRO_ENAB_INTR));
    }

    drv_usecwait(200);

    outb(base_io_address + CMD_REG, (CR_SWITCH_BANK | SEL_BANK0));
    drv_usecwait(50);

    return (TRUE);
}


/*
 * Name           : eepro_stop_board()
 * Purpose        : Stop the device's receiver and disables interrupts
 * Called from    : gld
 * Arguments      : macinfop - pointer to a gld_mac_info_t structure
 * Returns        : TRUE always
 * Side effects   : Receiver unit of the 82595TX and interrupts are disabled
 */

static int
eepro_stop_board(gld_mac_info_t *macinfop)
{
    register int base_io_address = macinfop->gldm_port;
    int      board_no = ddi_get_instance(macinfop->gldm_devinfo);
    int      count;  /* scratch */
    unchar   val;    /* scratch */

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_INIT)
    {
        cmn_err(CE_CONT, "eepro_stop_board(0x%x)", macinfop);
    }
#endif EEPRODEBUG

    /*
     * Wait for the execution unit to become idle
     */

    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {
        cmn_err(CE_CONT, "eepro%d: could not stop board\n", board_no);
        return (TRUE);
    }

    EEPRO_CLR_EXEC_STAT(base_io_address);

    outb(base_io_address + CMD_REG, CR_RCV_DISABLE);
    drv_usecwait(300);

    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {
        cmn_err(CE_CONT, "eepro%d: could not stop board\n", board_no);
        return (TRUE);
    }

    EEPRO_CLR_EXEC_STAT(base_io_address);

    val = inb(base_io_address + INTR_REG);

    outb(base_io_address + INTR_REG,
         (val | (RX_MASK | TX_MASK | RX_STOP_MASK)));

    /*
     * Acknowledge all interrupts
     */

    outb(base_io_address + STAT_REG,
         (STAT_EXEC_INT | STAT_RX_INT | STAT_TX_INT | STAT_RX_STOP));

    EEPRO_CLR_EXEC_STAT(base_io_address);

    /*
     * Switch to bank 1
     */

    outb(base_io_address + CMD_REG, CR_SWITCH_BANK | SEL_BANK1);
    drv_usecwait(50);

    /*
     * Disable board interrupts
     */

    val = inb(base_io_address + ENAB_INTR_REG);
    outb(base_io_address + ENAB_INTR_REG, val & ~EEPRO_ENAB_INTR);

    drv_usecwait(50);
    outb(base_io_address + CMD_REG, CR_SWITCH_BANK | SEL_BANK0);
    drv_usecwait(50);
    EEPRO_CLR_EXEC_STAT(base_io_address);

    /*
     * Reset the board before quitting
     */

    outb(base_io_address + CMD_REG, CR_RESET);
    drv_usecwait(300);

    count = 0;
    while (((inb(base_io_address + STAT_REG) & STAT_EXEC_INT) == 0) &&
            (count++ <= 100))
    {
        drv_usecwait(50);
    }

    EEPRO_CLR_EXEC_STAT(base_io_address);    /* clear status register */
    return (TRUE);
}


/*
 * Name           : eepro_saddr()
 * Purpose        : Set the physical network address on the board
 * Called from    : gld
 * Arguments      : macinfop - pointer to a gld_mac_info_t structure
 * Returns        : TRUE  on success
 *                  FALSE on failure
 * Side effects   : None
 */

static int
eepro_saddr(gld_mac_info_t *macinfop)
{
    register int base_io_address = macinfop->gldm_port;
    int      i;     /* scratch */

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_INIT)
    {
        cmn_err(CE_CONT, "eepro_saddr(0x%x)", macinfop);
    }
#endif EEPRODEBUG

    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_BOARD)
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                    board_no, __LINE__);
        }
#endif EEPRODEBUG

        return (FALSE);
    }

    /* 
     * Select the I/O bank 2
     */

    outb(base_io_address + CMD_REG, SEL_BANK2 | CR_SWITCH_BANK); 
    drv_usecwait(25);

    /*
     * Program current machine address into the hardware
     */

    for (i = 0; i < ETHERADDRL; i++)
    {
        outb(base_io_address + IA_REG0 + i, macinfop->gldm_macaddr[i]);
    }

    outb(base_io_address + CMD_REG, SEL_BANK0 | CR_SWITCH_BANK); 
    drv_usecwait(25);

    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_BOARD)
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                    board_no, __LINE__);
        }
#endif EEPRODEBUG

        return (FALSE);
    }

    return (TRUE);
}


/*
 * Name           : eepro_dlsdmult()
 * Purpose        : Enable/disable device level reception of specific
 *                  multicast addresses
 * Called from    : gld
 * Arguments      : macinfop - pointer to a gld_mac_info_t structure
 *                  mcast   - multicast address
 *                  op      - enable(1) / disable(0) flag
 * Returns        : TRUE   on success
 *                  FALSE  on failure
 * Side effects   : None
 */

int
eepro_dlsdmult(gld_mac_info_t *macinfop, struct ether_addr *mcast, int op)
{
    struct eeproinstance *eeprop =
        (struct eeproinstance *)macinfop->gldm_private;
    register int base_io_address = macinfop->gldm_port;
    ushort xmtbar;      /* transmit BAR register value */
    ushort val;         /* scratch */
    int    i, count1;   /* scratch */
    cmd_preamble_t mcast_cmd; /* multicast command's preamble */
    int board_no = ddi_get_instance(macinfop->gldm_devinfo);

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_MCAST)
    {
        cmn_err(CE_CONT, "eepro_dlsdmult(0x%x, %s)", macinfop,
                op ? "ON" : "OFF");
    }
#endif EEPRODEBUG

    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & (DEBUG_MCAST | DEBUG_BOARD))
        {
            cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                    board_no, __LINE__);
        }
#endif EEPRODEBUG

        return (FALSE);
    }

    EEPRO_CLR_EXEC_STAT(base_io_address);

    /* 
     * Enable or disable the multicast 
     */

    if (op)
    {
        /*
         * Add a multicast address:
         * update local table and then give list of addresses to 82595TX
         */

        for (i = 0; i <  GLD_MAX_MULTICAST; i++)
        {
            if (eeprop->eepro_multiaddr[i].entry[0] == 0)
            {
                /* 
                 * Free entry found 
                 */

                bcopy((caddr_t) mcast->ether_addr_octet, 
                      (caddr_t)eeprop->eepro_multiaddr[i].entry, ETHERADDRL);
                eeprop->multicast_count++;
                break;
            }
        }
        if (i >= GLD_MAX_MULTICAST)
        {
            cmn_err(CE_WARN, "eepro%d: multicast table full", board_no);
            return (FALSE);
        }
    }
    else 
    {
        /* 
         * Remove a multicast address: update local table first
         */

        for (i = 0; i <  GLD_MAX_MULTICAST; i++)
        {
            if (bcmp((caddr_t) mcast->ether_addr_octet, 
                     (caddr_t) eeprop->eepro_multiaddr[i].entry, 
                     ETHERADDRL) == 0)
            {
                /*
                 * Matching entry found - invalidate it
                 */

                eeprop->eepro_multiaddr[i].entry[0] = 0;
                eeprop->multicast_count--;
                break;
            }
        }
        if (i == GLD_MAX_MULTICAST)
        {
#ifdef EEPRODEBUG
            if (eeprodebug & DEBUG_MCAST)
            {
                cmn_err(CE_WARN,
                    "eepro%d: no matching multicast entry found", board_no);
            }
#endif EEPRODEBUG

            return (FALSE);
        }
    }

    outb(base_io_address + CMD_REG, SEL_BANK0 | CR_SWITCH_BANK);

    /*
     * Fill up the cmd_preamble_t structure
     */

    mcast_cmd.cmd           = CR_MC_SETUP;
    mcast_cmd.status        = 0;
    mcast_cmd.nxt_chain_ptr = 0;
    mcast_cmd.byte_count    = eeprop->multicast_count * ETHERADDRL;

    outw(base_io_address + HAR_LOW, NONXMT_AREA_START); 
    repoutsw(base_io_address + LMEM_IO_LOW, (ushort *) &mcast_cmd,
             sizeof (cmd_preamble_t) / 2);

    for (i = 0; i < GLD_MAX_MULTICAST; i++)
    {
        if (eeprop->eepro_multiaddr[i].entry[0] == 0)
        {
            continue;
        }

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_MCAST)
        {
            cmn_err(CE_CONT, "eepro%d: Adding multicast addr : ", board_no);
            EEPRO_PRINT_EADDR(eeprop->eepro_multiaddr[i].entry);
            cmn_err(CE_CONT, "\n");
        }
#endif EEPRODEBUG

        repoutsw(base_io_address + LMEM_IO_LOW,
                  (ushort *) eeprop->eepro_multiaddr[i].entry, ETHERADDRL / 2);
    }
    xmtbar = inw(base_io_address + XMT_BAR_LOW); 
    outw(base_io_address + XMT_BAR_LOW, NONXMT_AREA_START);

    outb(base_io_address + CMD_REG, CR_MC_SETUP);
    drv_usecwait(50);

    count1 = 0;
    do 
    {
        outw(base_io_address + HAR_LOW, NONXMT_AREA_START); 
        val = inw(base_io_address + LMEM_IO_LOW);
        drv_usecwait(50);
    } while (((val & CMD_DONE) != CMD_DONE) && (count1++ < 100));
    EEPRO_CLR_EXEC_STAT(base_io_address);
    outw(base_io_address + XMT_BAR_LOW, xmtbar); 

    return (TRUE);
}


/*
 * Name           : eepro_prom()
 * Purpose        : Enable/disable physical level promiscuous mode
 * Called from    : gld
 * Arguments      : macinfop - pointer to a gld_mac_info_t structure
 * Returns        : TRUE  on success
 *                  FALSE on failure
 * Side effects   : Board gets thrown into (or returns from) promiscuous 
 *                  mode
 */

static int
eepro_prom(gld_mac_info_t *macinfop, int on)
{
    register int base_io_address = macinfop->gldm_port;
    unchar val;     /* scratch */

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_PROM)
    {
        cmn_err(CE_CONT, "eepro_prom(0x%x, %s)", macinfop, on ? "ON" : "OFF");
    }
#endif EEPRODEBUG

    outb(base_io_address + CMD_REG, (SEL_BANK0 | CR_SWITCH_BANK));


    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & (DEBUG_PROM | DEBUG_BOARD))
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                    board_no, __LINE__);
        }
#endif EEPRODEBUG

        return (FALSE);
    }

    EEPRO_CLR_EXEC_STAT(base_io_address);

    outb(base_io_address + CMD_REG, (SEL_BANK2 | CR_SWITCH_BANK));
    val = inb(base_io_address + CONF_REG2);

    /*
     * Enable or disable promiscuous mode 
     */

    if (on)
    {
        outb(base_io_address + CONF_REG2, val | ENAB_PROM_MODE); 
    }
    else
    {
        outb(base_io_address + CONF_REG2, val & ~ENAB_PROM_MODE); 
    }
    drv_usecwait(50);

    /*
     * Trigger the configuration process by writing to the configuration
     * register 3 of I/O bank 2
     */
	
	val = inb(base_io_address + CONF_REG3);
	outb(base_io_address + CONF_REG3, val);
	drv_usecwait(300);
    
    outb(base_io_address + CMD_REG, (SEL_BANK0 | CR_SWITCH_BANK));
	drv_usecwait(50);

    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & (DEBUG_PROM | DEBUG_BOARD))
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                    board_no, __LINE__);
        }
#endif EEPRODEBUG

        return (FALSE);
    }

    EEPRO_CLR_EXEC_STAT(base_io_address);

    return (TRUE);
}


/*
 * Name           : eepro_gstat()
 * Purpose        : Gather statistics from the hardware and update the
 *                  gldm_stats structure. 
 * Called from    : gld, just before it reads the driver's statistics 
 *                  structure
 * Arguments      : macinfop - pointer to a gld_mac_info_t structure
 * Returns        : TRUE always
 * Side effects   : None
 * Miscellaneous  : Dummy routine since statistics are maintained on the fly
 */

static int
eepro_gstat(gld_mac_info_t *macinfop)
{

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_STAT)
    {
        cmn_err(CE_CONT, "eepro_gstat(0x%x)", macinfop);
    }
#endif EEPRODEBUG

    return (TRUE);
}


/*
 * Name           : eepro_send()
 * Purpose        : Transmit a packet on the network. Note that this 
 *                  function returns even before transmission by the 
 *                  82595TX completes. Hence, return value of SUCCESS is 
 *                  no guarantee that the packet was successfully 
 *                  transmitted (that is, without errors during 
 *                  transmission)
 * Called from    : gld, when a packet is ready to be transmitted
 * Arguments      : macinfop - pointer to a gld_mac_info_t structure
 *                  mp      - pointer to an M_DATA message that contains 
 *                          the packet. The complete LLC header is 
 *                          contained in the message's first message 
 *                          block, and the remainder of the packet is 
 *                          contained within additional M_DATA message 
 *                          blocks linked to the first message block
 * Returns        : SUCCESS if a command was issued to the 82595TX to 
 *                          transmit a packet
 *                  RETRY   on failure so that gld may retry later
 * Side effects   : None
 */

static int
eepro_send(gld_mac_info_t *macinfop, mblk_t *mp)
{
    struct eeproinstance *eeprop =
            (struct eeproinstance *) macinfop->gldm_private;
    register int base_io_address = macinfop->gldm_port;
    struct ether_header *eh = (struct ether_header *) mp->b_rptr;
    cmd_preamble_t xmt_cmd;  /* transmit command structure */
    ushort status;           /* status field of a transmit buffer */
    int    count;            /* scratch */
    mblk_t *mp1;             /* message block :: scratch variable */
    ushort xmit_size;        /* size of the frame to be transmitted */
    ushort nbytes = 0;       /* number of bytes */
    short  this_xmtbuf;      /* current transmit buffer being used */
    short  prev_xmtbuf;      /* previous transmit buffer being used */
    ushort chain = 0;        /* flag to detect if chaining can be used */

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_SEND)
    {
        cmn_err(CE_CONT, "eepro_send(0x%x, 0x%x)", macinfop, mp);
    }
#endif EEPRODEBUG

    this_xmtbuf = eeprop->nxt_xmtbuf;
    prev_xmtbuf = (this_xmtbuf - 1) % NUM_XMT_BUFS;
    if (prev_xmtbuf < 0)
    {
        prev_xmtbuf = NUM_XMT_BUFS - 1;
    }

    outb(base_io_address + CMD_REG, (SEL_BANK0 | CR_SWITCH_BANK));

    outw(base_io_address + HAR_LOW, eeprop->xmt_buf_addr[prev_xmtbuf]);
    status = inw(base_io_address + LMEM_IO_LOW);

    /*
     * Chain a command to the previous xmit command if that command has
     * still not completed, and it is safe to chain 
     */

    if (((status & CMD_DONE) != CMD_DONE) && eeprop->chain_flag)
    {
#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_SEND)
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: chain set\n", board_no);
        }
#endif EEPRODEBUG

        chain = 1;
        (void) inw(base_io_address + LMEM_IO_LOW); /* discard status */
        (void) inw(base_io_address + LMEM_IO_LOW); /* discard nxt buf ptr */
        nbytes = inw(base_io_address + LMEM_IO_LOW);
    }
    else
    {
        chain = 0;
    }

    if (chain == 0)
    {
        if (eepro_wait_exec(base_io_address) != SUCCESS)
        {

#ifdef EEPRODEBUG
            if (eeprodebug & (DEBUG_BOARD | DEBUG_SEND))
            {
                int board_no = ddi_get_instance(macinfop->gldm_devinfo);

                cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                        board_no, __LINE__);
            }
#endif EEPRODEBUG

            macinfop->gldm_stats.glds_defer++;    
            eeprop->chain_flag = 0;
            return (RETRY);
        }
    }
    else
    {
        /*
         * Check if this transmit buffer is free
         */

        count = 0;
        do
        {
            drv_usecwait(10);
            outw(base_io_address + HAR_LOW, eeprop->xmt_buf_addr[this_xmtbuf]);
            status = inw(base_io_address + LMEM_IO_LOW);
        } while (((status & CMD_DONE) != CMD_DONE) && (status != 0) &&
                  count++ <= 100);
        if (count > 100)
        {
    #ifdef EEPRODEBUG
            if (eeprodebug & DEBUG_SEND)
            {
                int board_no = ddi_get_instance(macinfop->gldm_devinfo);

                cmn_err(CE_CONT,
                    "eepro%d: count > 100! send failed with status 0x%x\n",
                    board_no, status);
            }
    #endif EEPRODEBUG

            eeprop->chain_flag = 0;
            return (RETRY);
        }
    }

    EEPRO_CLR_EXEC_STAT(base_io_address);


    /*
     * Copy the data part of the message to the temporary transmit buffer
     * before transferring it to the DPRAM
     */
    
    mp->b_rptr += sizeof (struct ether_header);
    xmit_size = 0;
    for (mp1 = mp; mp1 != NULL; mp1 = mp1->b_cont) 
    {
        bcopy((char *) (mp1->b_rptr), eeprop->xmit_buf + xmit_size,
              (mp1->b_wptr - mp1->b_rptr));
        xmit_size += (mp1->b_wptr - mp1->b_rptr);
    }
    xmit_size += sizeof (struct ether_header);

    if (xmit_size < MIN_PACKET_SIZE)
    {
        xmit_size = MIN_PACKET_SIZE;
    }

    /*
     * Initialize the transmit BAR only if a CR_TRANSMIT command is about to
     * be issued
     */
    
    if (chain == 0)
    {
        outw(base_io_address + XMT_BAR_LOW,
             eeprop->xmt_buf_addr[this_xmtbuf]);
    }

    /*
     * Transfer data to the transmit buffer (located at XMT_BUF_BASE) :
     * - First fill up the preamble for the command.
     * - Initialize the size of the packet. Note that the source address is
     *   automatically inserted by the 82595TX; so the size of the frame to
     *   be copied to local memory is actually ETHERADDRL bytes less than
     *   what is computed.
     * - No chaining is used and transmission is in standard mode.
     */

    xmt_cmd.cmd           = CR_TRANSMIT;
    xmt_cmd.status        = 0;
    xmt_cmd.nxt_chain_ptr = 0;
    xmt_cmd.byte_count    = (xmit_size - ETHERADDRL);

    outw(base_io_address + HAR_LOW, eeprop->xmt_buf_addr[this_xmtbuf]);
    repoutsw(base_io_address + LMEM_IO_LOW, (ushort *) &xmt_cmd,
             sizeof (cmd_preamble_t) / 2);

    /*
     * Copy the relevant parts of the standard ethernet header to board
     * memory, viz. destination address and the SAP value. The source
     * address is automatically inserted by the 82595TX.
     */

    repoutsw(base_io_address + LMEM_IO_LOW, (ushort *) eh, ETHERADDRL / 2); 
    outw(base_io_address + LMEM_IO_LOW, eh->ether_type);

    /*
     * Standard processing (with chaining) is being used; so, issue the
     * transmit (or the transmit resume) command to the 82595TX after
     * copying the entire contents of the frame to board memory
     */

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_SEND)
    {
        int i;
        cmn_err(CE_CONT, "Ether type is %x\n", eh->ether_type);
        cmn_err(CE_CONT, "Data :: ");
        for (i = 0; i < sizeof (struct ether_header); i++)
        {
            cmn_err(CE_CONT, "%x ", *(mp->b_rptr + i));
        }
        for (i = 0; i < xmit_size - sizeof (struct ether_header); i++)
        {
            cmn_err(CE_CONT, "%x ", eeprop->xmit_buf[i]);
        }
    }
#endif EEPRODEBUG

    SAFE_OUTCOPY(base_io_address, eeprop->xmit_buf,
                 (xmit_size - sizeof (struct ether_header)));

    /*
     * Issue a CR_TRANSMIT_RESUME or a CR_TRANSMIT command if chaining is
     * enabled or not enabled respectively.
     */

    if (chain)
    {
        /*
         * Manipulate the previous transmit buffer's next buffer pointer and
         * byte count fields
         */

        outw(base_io_address + HAR_LOW,
             eeprop->xmt_buf_addr[prev_xmtbuf] + 4);
        outw(base_io_address + LMEM_IO_LOW,
             eeprop->xmt_buf_addr[this_xmtbuf]);
        outw(base_io_address + LMEM_IO_LOW, (nbytes | TX_CHAIN_ENAB_MASK));

        outb(base_io_address + CMD_REG, CR_TRANSMIT_RESUME);

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_SEND)
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: xmt_resume issued\n", board_no);
        }
#endif EEPRODEBUG

    }
    else
    {
        outb(base_io_address + CMD_REG, CR_TRANSMIT);
    }

    eeprop->nxt_xmtbuf = (this_xmtbuf + 1) % NUM_XMT_BUFS;
    return (SUCCESS);        /* successful transmit attempt */
}


/*
 * Name           : eepro_intr()
 * Purpose        : Interrupt handler for the device
 * Called from    : gld
 * Arguments      : macinfop - pointer to a gld_mac_info_t structure
 * Returns        : DDI_INTR_CLAIMED   if the interrupt was serviced
 *                  DDI_INTR_UNCLAIMED otherwise
 * Side effects   : None
 */

u_int
eepro_intr(gld_mac_info_t *macinfop)
{
    struct eeproinstance *eeprop =        /* Our private device info */
        (struct eeproinstance *)macinfop->gldm_private;
    register int base_io_address = macinfop->gldm_port;
    unchar   stat, val;          /* scratch variables */
    ushort   addr, status;       /* scratch variables */

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_INTR)
    {
        cmn_err(CE_CONT, "eeprointr(0x%x)", macinfop);
    }
#endif EEPRODEBUG

    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_INTR)
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                    board_no, __LINE__);
        }
#endif EEPRODEBUG

        return (DDI_INTR_UNCLAIMED);
    }

    stat = inb(base_io_address + STAT_REG);
    if (stat & STAT_EXEC_INT)
    {
        outb(base_io_address + STAT_REG, STAT_EXEC_INT);
    }

    /*
     * Receive interrupts and stop register hit events get priority over
     * transmit done interrupts
     */

    if (stat & STAT_RX_INT)
    {
        eepro_rcv_packet(macinfop);
        outb(base_io_address + STAT_REG, STAT_RX_INT);
    }

    if (stat & STAT_RX_STOP)
    {

#ifdef EEPRODEBUG 
        if (eeprodebug & DEBUG_INTR)
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_WARN, "eepro%d: RCV stop register hit", board_no);
        }
#endif EEPRODEBUG

        /*
         * Update gld_stats structure by reading the RCV_NO_RSRC counter and
         * reset this counter again
         */

        macinfop->gldm_stats.glds_norcvbuf +=
                 inb(base_io_address + RCV_NO_RSRC);
        outb(base_io_address + RCV_NO_RSRC, 0);

        outb(base_io_address + STAT_REG, STAT_RX_STOP);
        addr = (eeprop->nxt_rcv_frame != RCV_AREA_START) ?
                (eeprop->nxt_rcv_frame - 2) : DPRAM_END;
        outw(base_io_address + RCV_STOP_LOW, addr);
        eepro_rcv_packet(macinfop);
    }

    /*
     * Process interrupts resulting from transmit done events
     */

    if (stat & STAT_TX_INT)
    {
        unchar this_xmtbuf = eeprop->nxt_xmtintr;

        outb(base_io_address + STAT_REG, STAT_TX_INT);
        outw(base_io_address + HAR_LOW, eeprop->xmt_buf_addr[this_xmtbuf]);
        (void) inw(base_io_address + LMEM_IO_LOW);   /* event field */
        status = inw(base_io_address + LMEM_IO_LOW); /* status field */
        if (status & NUM_COLLISIONS_MASK)
        {
            macinfop->gldm_stats.glds_collisions +=
                     (status & NUM_COLLISIONS_MASK);
        }
        if (status & LATE_COLLISIONS_MASK)
        {
            macinfop->gldm_stats.glds_xmtlatecoll++;
        }
        if (status & LOST_CARRIER_SENSE)
        {
            /*
             * Do not print out the warning for missing carrier now;
             * the eepro_watchdog() routine will take care of it
             */

            macinfop->gldm_stats.glds_nocarrier++;
            eeprop->eepro_watch |= EEPRO_NOXVR;
        }
        else
        {
            eeprop->eepro_watch &= ~EEPRO_NOXVR;
        }

        if (status & TX_OK)
        {
            eeprop->chain_flag = 1;
            eeprop->nxt_xmtintr = (eeprop->nxt_xmtintr + 1) % NUM_XMT_BUFS;
        }
        else
        {
            int    i;

            macinfop->gldm_stats.glds_errxmt++;
            eeprop->chain_flag = 0;

            /*
             * Walk through the command chain and clear the event field
             * of each transmit buffer. This is because in some cases,
             * the CMD_DONE bit does not get set if the transmit failed.
             */

            for (i = 0; i < NUM_XMT_BUFS; i++)
            {
                ushort nxt_xmtbuf = (this_xmtbuf + 1) % NUM_XMT_BUFS;

                outw(base_io_address + HAR_LOW,
                     eeprop->xmt_buf_addr[this_xmtbuf]);
                outw(base_io_address + LMEM_IO_LOW, 0); /* clear event */
                drv_usecwait(20);
                outw(base_io_address + HAR_LOW,
                     eeprop->xmt_buf_addr[this_xmtbuf] + 4);
                eeprop->nxt_xmtintr = this_xmtbuf = nxt_xmtbuf;
            }
        }

        /*
         * Maximum number of collisions exceeded
         */

        if (status & MAX_COLLISIONS_MASK)
        {
            macinfop->gldm_stats.glds_collisions += 16;
        }
    }
         
    /*
     * Acknowledge the interrupt by setting the appropriate bit in the
     * interrupt mask register
     */

    val = inb(base_io_address + INTR_REG);

    if (val & EXEC_MASK) 
    {
        outb(base_io_address + INTR_REG, (val & ~EXEC_MASK));
    }

    if (val & RX_MASK) 
    {
        outb(base_io_address + INTR_REG, (val & ~RX_MASK));
    }

    if (val & TX_MASK) 
    {
        outb(base_io_address + INTR_REG, (val & ~TX_MASK));
    }

    if (val & RX_STOP_MASK) 
    {
        outb(base_io_address + INTR_REG, (val & ~RX_STOP_MASK));
    }

    macinfop->gldm_stats.glds_intr++;
    eeprop->eepro_watch |= EEPRO_ACTIVE;
    return (DDI_INTR_CLAIMED);
}


/*
 * Name           : eepro_ioctl()
 * Purpose        : Ioctl handler for the device
 * Called from    : gld
 * Arguments      : q  - pointer to a queue_t structure
 *                  mp - message block, pointer to a mblk_t
 * Returns        : SUCCESS if the ioctl was recognized
 *                  FAILURE otherwise
 * Side effects   : None
 */

static int 
eepro_ioctl(queue_t *q, mblk_t *mp)
{
    gld_t *gldp = (gld_t *)(q->q_ptr);
    gld_mac_info_t *macinfop = gldp->gld_mac_info;
    int retval = 0;           /* scratch */

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_IOCTL)
    {
        cmn_err(CE_CONT, "eeproioctl(0x%x, 0x%x)", q, mp);
    }
#endif EEPRODEBUG

    if (((struct iocblk *) mp->b_rptr)->ioc_count == TRANSPARENT)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_IOCTL)
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_NOTE, "eepro%d: xparent ioctl", board_no);
        }
#endif EEPRODEBUG

        goto err;
    }

    switch (((struct iocblk *) mp->b_rptr)->ioc_cmd)
    {
        default:

#ifdef EEPRODEBUG
            if (eeprodebug & DEBUG_IOCTL)
            {
                int board_no = ddi_get_instance(macinfop->gldm_devinfo);

                cmn_err(CE_CONT, "eepro%d: unknown ioctl 0x%x",
                        board_no, ((struct iocblk *) mp->b_rptr)->ioc_cmd);
            }
#endif EEPRODEBUG

            goto err;

        case TDR_TEST :
            if ((retval = eepro_tdr_test(macinfop)) == FAILURE)
            {
                goto err;
            }
            break;

    }    /* end of switch */

    /*
     * Acknowledge the ioctl
     */

    ((struct iocblk *) mp->b_rptr)->ioc_rval = retval;
    mp->b_datap->db_type = M_IOCACK;
    qreply(q, mp);
    return (SUCCESS);

err:
    ((struct iocblk *) mp->b_rptr)->ioc_rval = FAILURE;
    mp->b_datap->db_type = M_IOCNAK;
    qreply(q, mp);
    return (FAILURE);
}



/*
 *                     UTILITY ROUTINES SPECIFIC TO THE DRIVER
 */


/*
 * Name           : eepro_init_board()
 * Purpose        : Initialize the specified network board. Initialize the
 *                  receive area and the transmit area of the board and
 *                  set the individual address of the board.
 *                  DO NOT enable the Receive Unit. 
 * Called from    : eepro_attach()
 * Arguments      : macinfop - pointer to a gld_mac_info_t structure
 * Returns        : None
 * Side effects   : Previous state of the 82595TX and its data structures 
 *                  are lost
 */

static void
eepro_init_board(gld_mac_info_t *macinfop)
{
    struct eeproinstance *eeprop =
             (struct eeproinstance *) macinfop->gldm_private; 
    register int base_io_address = macinfop->gldm_port;
    unchar   val;     /* scratch */
    int      i;       /* scratch */
        
#ifdef EEPRODEBUG
    if (eeprodebug & (DEBUG_INIT | DEBUG_BOARD))
    {
        cmn_err(CE_CONT, "eepro_init_board(0x%x)", macinfop);
    }
#endif EEPRODEBUG

    /*
     * Set the address of the board first; it is recommended that this is
     * done before the configuration registers 2 and 3 in bank 2 are
     * written to (see sec. 7.3.3, pg. 7-17 of 82595TX user manual).
     */

    (void) eepro_saddr(macinfop);
    drv_usecwait(100);

    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & (DEBUG_INIT | DEBUG_BOARD))
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                    board_no, __LINE__);
        }
#endif EEPRODEBUG

        return;
    }

    EEPRO_CLR_EXEC_STAT(base_io_address);

    outb(base_io_address + CMD_REG, CR_SWITCH_BANK | SEL_BANK2);
    drv_usecwait(50);

    /*
     * Discard bad frames to decrease chances of a receive stop register
     * hit
     */

    val = inb(base_io_address + CONF_REG1);

    /*
     * Discard reception of bad frames and use standard mode with chaining.
     */

    outb(base_io_address + CONF_REG1, val | DISCARD_BAD_FRAMES |
         TX_CHAIN_ERROR_DONT_STOP);

    val = inb(base_io_address + CONF_REG3);
    val &= CLR_TST1_TST2_MASK;

    /*
     * Clear the BNC/TPE and TPE/AUI bits
     */

    val &= CLR_CONNTYPE_BITS_MASK; /* clear the BNC/TPE and TPE/AUI bits */
    if (eeprop->autoport_flag == TRUE)
    {
        val &= ~NO_APORT_MASK;  /* forcibly enable auto port selection */
    }
    else
    {
		val |= NO_APORT_MASK;  /* forcibly deselect autoport selection */
        switch (macinfop->gldm_media)
        {
            case GLDM_BNC :
                    val |= (0x20 | 0x04);
                    break;

            case GLDM_TP :
                    val |= 0x04;
                    break;

            case GLDM_AUI :
                    val |= 0x20;
                    break;
        }
    }

    outb(base_io_address + CONF_REG3, val);
    drv_usecwait(300);

    outb(base_io_address + CMD_REG, CR_SWITCH_BANK | SEL_BANK0);
    drv_usecwait(50);
    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & (DEBUG_INIT | DEBUG_BOARD))
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                    board_no, __LINE__);
        }
#endif EEPRODEBUG

        return;
    }

    /*
     * Issue a SEL_RESET command to configure the board correctly (see
     * sec. 7.3.3, pg. 7-17 of 82595TX user manual).
     */

    outb(base_io_address + CMD_REG, CR_SEL_RESET);
    drv_usecwait(300);

    /*
     * Reset the receive no resource counter
     */
    
    outb(base_io_address + RCV_NO_RSRC, 0);

    /*
     * Switch to Bank 1 
     */ 

    outb(base_io_address + CMD_REG, CR_SWITCH_BANK | SEL_BANK1);
    drv_usecwait(50);

    /*
     * Program the irq level at which the board should interrupt
     */

    val = inb(base_io_address + SEL_INTR_REG);
    outb(base_io_address + SEL_INTR_REG, (val | eeprop->intr_level));

    /*
     * Indicate the start of receive and transmit areas in DPRAM to 82595TX.
     */

    outb(base_io_address + RCV_LOW_LIM_REG, HIGH(RCV_AREA_START));
    outb(base_io_address + RCV_UP_LIM_REG, HIGH(DPRAM_END));

    outb(base_io_address + XMT_LOW_LIM_REG, XMT_BUF_BASE);
    outb(base_io_address + XMT_UP_LIM_REG, HIGH(XMT_AREA_END));

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_INIT)
    {
        int board_no = ddi_get_instance(macinfop->gldm_devinfo);

        cmn_err(CE_CONT, "eepro%d: rcv area start @ 0x%x, ",
                board_no, RCV_AREA_START);
        cmn_err(CE_CONT, "xmt start @ 0x%x, xmt end @ 0x%x\n",
                         XMT_BUF_BASE, XMT_AREA_END);
    }
#endif EEPRODEBUG

    /*
     * Disable BOF interrupts for concurrent processing
     */

    outb(base_io_address + RCV_BOF_THRES_REG, 0);

    outb(base_io_address + CMD_REG, CR_SWITCH_BANK | SEL_BANK0);
    drv_usecwait(50);
    
    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_INIT)
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                    board_no, __LINE__);
        }
#endif EEPRODEBUG

        return;
    }

    /*
     * Forcibly select BAR and initialize the receive and transmit base
     * address registers
     */

    val = inb(base_io_address + INTR_REG);
    outb(base_io_address + INTR_REG, val & ~SEL_CUR_REG_MASK);

    outb(base_io_address + RCV_BAR_LOW, LOW(RCV_AREA_START));
    outb(base_io_address + RCV_BAR_HIGH, HIGH(RCV_AREA_START));

    outb(base_io_address + XMT_BAR_LOW, LOW(XMT_BUF_BASE));
    outb(base_io_address + XMT_BAR_HIGH, HIGH(XMT_BUF_BASE));

    outw(base_io_address + RCV_STOP_LOW, DPRAM_END);
    eeprop->nxt_rcv_frame = RCV_AREA_START;

    drv_usecwait(50);

    /*
     * Deselect concurrent processing during receive
     */

    outb(base_io_address + RCV_COPY_THRES_REG, 0);

    /*
     * Zero the first 100 bytes in DPRAM
     */
    
    outw(base_io_address + HAR_LOW, 0);

    for (i = 0; i < XMT_BUF_BASE / 2; i++)
    {
        outw(base_io_address + LMEM_IO_LOW, 0);
    }

    /*
     * Set up the transmit buffer
     */

    for (i = 0; i < NUM_XMT_BUFS; i++)
    {
        outw(base_io_address + HAR_LOW, XMT_BUF_BASE + i * XMT_BUF_SIZE);
        outw(base_io_address + LMEM_IO_LOW, 0);
        outw(base_io_address + LMEM_IO_LOW, TX_OK); /* status field */
        outw(base_io_address + LMEM_IO_LOW, 0x0); /* next frame pointer */
        outw(base_io_address + LMEM_IO_LOW, 0x0); /* byte count field */

        eeprop->xmt_buf_addr[i] = XMT_BUF_BASE + i * XMT_BUF_SIZE;

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_INIT)
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: %dth xmt buffer @ 0x%x\n", board_no,
                    i, eeprop->xmt_buf_addr[i]);
        }
#endif EEPRODEBUG

    }

    eeprop->nxt_xmtbuf = 0; /* zeroeth transmit buffer is initially used */
    eeprop->chain_flag = 0; /* no chaining initially */

    if (eepro_wait_exec(base_io_address) != SUCCESS)
    {

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_INIT)
        {
            int board_no = ddi_get_instance(macinfop->gldm_devinfo);

            cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                    board_no, __LINE__);
        }
#endif EEPRODEBUG

        return;
    }

    EEPRO_CLR_EXEC_STAT(base_io_address);
}


/*
 * Name           : eepro_rcv_packet()
 * Purpose        : Get a packet that has been received by the hardware 
 *                  and pass it up to gld
 * Called from    : eepro_intr()
 * Arguments      : macinfop - pointer to a gld_mac_info_t structure
 * Returns        : None
 * Side effects   : None
 */

static void
eepro_rcv_packet(gld_mac_info_t *macinfop)
{
    struct eeproinstance *eeprop =
                (struct eeproinstance *) macinfop->gldm_private;
    register int base_io_address = macinfop->gldm_port;
    int board_no = ddi_get_instance(macinfop->gldm_devinfo);
    rcv_frame_hdr_t frame_hdr;  /* header of a received frame */
    mblk_t *mp;         /* message block into which pkt is to be copied */
    ushort this_frame;  /* address of this frame in DPRAM */
    ushort nxt_frame;   /* address of the next frame in DPRAM */
    ushort addr;        /* scratch */
    ushort val;         /* scratch */

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_RECV)
    {
        cmn_err(CE_CONT, "eepro_rcv_packet(0x%x)", macinfop);
    }
#endif EEPRODEBUG

    this_frame = nxt_frame = eeprop->nxt_rcv_frame;
    if (this_frame < RCV_AREA_START)
    {
        /*
         * Sanity check but if this happens, it is a catastrophic error;
         * there is no other option but to reset the board.
         */

        cmn_err(CE_NOTE,
            "eepro%d: receive frame crept into xmt area! resetting board",
            board_no);
        (void) eepro_reset(macinfop);
        (void) eepro_start_board(macinfop);
        return;
    }
    outw(base_io_address + HAR_LOW, nxt_frame);
    repinsw(base_io_address + LMEM_IO_LOW, (ushort *) &frame_hdr,
            sizeof (rcv_frame_hdr_t) / 2);

    /*
     * Process as many frames as are received by the 82595TX. A zero in the
     * event field indicates that a received frame is in the process of being
     * received by the 82595TX
     */

    while (frame_hdr.event)
    {

        /*
         * (Sanity check)
         * The reclaim bit case: this condition should never arise ideally,
         * since concurrent processing is not being used and bad frames are
         * being discarded
         */

        if ((frame_hdr.event & FRAME_RECLAIM_BIT_SET) &&
            ((frame_hdr.event & FRAME_RECEIVED_EOF) != FRAME_RECEIVED_EOF))
        {

#ifdef EEPRODEBUG
            if (eeprodebug & DEBUG_RECV)
            {
                cmn_err(CE_CONT, "eepro%d: reclaim bit set! event: 0x%x\n",
                        board_no, frame_hdr.event);
            }
#endif EEPRODEBUG

            macinfop->gldm_stats.glds_errrcv++;
            addr = (this_frame != RCV_AREA_START) ? (this_frame - 2) :
                   DPRAM_END;
            outw(base_io_address + RCV_STOP_LOW, addr);
            return;
        }

        /*
         * Sanity check: drop packets received in error; actually the
         * receiver is programmed to drop frames in error
         */

        if ((frame_hdr.status & FRAME_OK) != FRAME_OK)
        {

#ifdef EEPRODEBUG
            if (eeprodebug & DEBUG_RECV)
            {
                if ((frame_hdr.status & FRAME_OK) != FRAME_OK)
                {
                    cmn_err(CE_WARN, "eepro%d: frame received in error",
                            board_no);
                }
            }
#endif EEPRODEBUG

            macinfop->gldm_stats.glds_errrcv++;
            goto iterate;
        }

        /*
         * Check for runt packets
         */

        if (frame_hdr.len < 60)
        {

#ifdef EEPRODEBUG
            if (eeprodebug & DEBUG_RECV)
            {
                cmn_err(CE_WARN, "eepro%d: runt frame received", board_no);
            }
#endif EEPRODEBUG

            macinfop->gldm_stats.glds_short++;
            goto iterate;
        }

        /*
         * Check for giant packets
         */

        if (frame_hdr.len > 1514)
        {

#ifdef EEPRODEBUG
            if (eeprodebug & DEBUG_RECV)
            {
                cmn_err(CE_WARN, "eepro%d: giant packet received",
                        board_no);
            }
#endif EEPRODEBUG

            goto iterate;
        }

        /*
         * Allocate a message block. Note: add 4 to the length to make sure
         * that even if we have modify the mp->b_wptr for alignment on a
         * long word boundary, we would still have "frame_hdr.len" bytes of
         * space available.
         */

        if ((mp = allocb(frame_hdr.len + 4, BPRI_MED)) == NULL)
        {
            cmn_err(CE_WARN, "eepro%d: no STREAMS buffers", board_no);
            macinfop->gldm_stats.glds_norcvbuf++;
			/*
            addr = (frame_hdr.nxt_frame_ptr != RCV_AREA_START) ?
					(frame_hdr.nxt_frame_ptr - 2) : DPRAM_END;
            outw(base_io_address + RCV_STOP_LOW, addr);
            return;
			*/
			goto iterate;
        }

        /*
         * Align mp->b_wptr on a long word boundary
         */
        
        if ((val = ((ulong) (mp->b_wptr) & 3)) != 0)
        {
            mp->b_wptr += 4 - val;

#ifdef EEPRODEBUG
            if (eeprodebug & DEBUG_RECV)
            {
                cmn_err(CE_CONT, 
                    "eepro%d: mp->b_wptr not on long word boundary\n",
                    board_no);
            }
#endif EEPRODEBUG

        }

        SAFE_INCOPY(base_io_address, (mp->b_wptr), frame_hdr.len);
        mp->b_wptr += frame_hdr.len;

#ifdef EEPRODEBUG
        if (eeprodebug & DEBUG_RECV)
        {
            int i;

            cmn_err(CE_CONT, "eepro%d: received packet size: %d bytes\n",
                    board_no, frame_hdr.len); 
            cmn_err(CE_CONT, "eepro%d: Destination Address is ", board_no);
            for (i = 0; i < ETHERADDRL; i++)
            {
                cmn_err(CE_CONT, "%x ", *(mp->b_rptr + i));
            }
            cmn_err(CE_CONT, "\n");
        }
#endif EEPRODEBUG

        gld_recv(macinfop, mp);

iterate:
        this_frame = frame_hdr.nxt_frame_ptr;

        outw(base_io_address + HAR_LOW, frame_hdr.nxt_frame_ptr);
        repinsw(base_io_address + LMEM_IO_LOW, (ushort *) &frame_hdr,
                    sizeof (rcv_frame_hdr_t) / 2);

        addr = (this_frame != RCV_AREA_START) ? (this_frame - 2) : DPRAM_END;
        outw(base_io_address + RCV_STOP_LOW, addr);
    }

    eeprop->nxt_rcv_frame = this_frame;
}


/*
 * Name           : eepro_wait_exec()
 * Purpose        : Detect if the command unit of the 82595TX is busy
 *                  executing a previously issued command.
 * Called from    : Any function that needs to issue a command to the 
 *                  82595TX
 * Arguments      : base_io_address : I/O base address of the board
 * Returns        : SUCCESS if the 82595TX had accepted the previous command
 *                  FAILURE otherwise
 * Side effects   : None
 */

static int
eepro_wait_exec(register int base_io_address)
{
    register int count = 0;

    outb(base_io_address + CMD_REG, (SEL_BANK0 | CR_SWITCH_BANK));

    while (((inb(base_io_address + STAT_REG) & EXEC_STATES) != EXEC_IDLE) &&
            (count++ <= 1000))
    {
        drv_usecwait(50);
    }
    if (count > 1000)
    {
        return (FAILURE);
    }

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_BOARD)
    {
        cmn_err(CE_CONT, "eepro: eepro_wait_exec() returned success.\n");
    }
#endif EEPRODEBUG

    return (SUCCESS);
}


/*
 * Name           : read_eeprom()
 * Purpose        : read a word from the eeprom
 * Called from    : eepro_attach(), for reading the Ethernet address
 * Arguments      : base_io_address - I/O base address of the board
 *                  reg             - register number in EEPROM
 *                  result          - pointer to a ushort that contains the
 *                                    result on return
 * Returns        : None (but the value read is placed in "result")
 * Side effects   : None
 */

static void
read_eeprom(register int base_io_address, int reg, ushort *result)
{
    unchar initval;    /* initial value of the EEPROM_REG */
    unchar outval;     /* used to output values to EEPROM_REG each time */
    int i;             /* scratch */
        
    drv_usecwait(25);
    outb(base_io_address + CMD_REG, SEL_BANK2 | CR_SWITCH_BANK);
    drv_usecwait(25);
    initval = inb(base_io_address + EEPROM_REG) & ~(EEDO | EEDI | EESK);
    outb(base_io_address + EEPROM_REG, initval | EECS); 
    initval |= EECS;

    /*
     * Shift the READ_EEPROM opcode (including the start bit) into the
     * EEDI serial input port of the EEPROM control register.
     */

    for (i = 2; i >= 0; i--)
    {
        if ((READ_EEPROM >> i) & 0x01)
        {
            outval = initval | EEDI;
        }
        else
        {
            outval = initval;
        }

        /*
         * Data is clocked in to the EEPROM register by raising and then
         * lowering the clock after writing the desired value into the EEDI
         * bit (refer pg. 4-11 of 82595TX user manual)
         */

        outb(base_io_address + EEPROM_REG, outval);
        drv_usecwait(25);
        outb(base_io_address + EEPROM_REG, outval | EESK);
        drv_usecwait(25);
        outb(base_io_address + EEPROM_REG, outval & ~EESK);
        drv_usecwait(25);
    }

    /*
     * Shift the register number into the EEDI serial input port of the
     * EEPROM control register.
     */

    for (i = 5; i >= 0; i--)
    {
        if ((reg >> i) & 0x01)
        {
            outval = initval | EEDI;
        }
        else
        {
            outval = initval;
        }
        /*
         * Data is clocked in to the EEPROM register by raising and then
         * lowering the clock after writing the desired value into the EEDI
         * bit
         */

        outb(base_io_address + EEPROM_REG, outval);
        drv_usecwait(25);
        outb(base_io_address + EEPROM_REG, outval | EESK);
        drv_usecwait(25);
        outb(base_io_address + EEPROM_REG, outval & ~EESK);
        drv_usecwait(25);
    }

    /*
     * Now, read 16 bits of data from the EEDO serial output port of the
     * EEPROM control register.
     */

    *result = 0;
    for (i = 15; i >= 0; i--)
    {
        outb(base_io_address + EEPROM_REG, initval | EESK);
        drv_usecwait(25);
        if (inb(base_io_address + EEPROM_REG) & EEDO)
        {
            *result |= (1 << i);
        }
        outb(base_io_address + EEPROM_REG, initval & ~EESK);
        drv_usecwait(25);
    }

    /*
     * Deassert the EEPROMCS bit before returning
     */

    outb(base_io_address + EEPROM_REG, initval & ~EECS);
    drv_usecwait(25);
    outb(base_io_address + CMD_REG, SEL_BANK0 | CR_SWITCH_BANK);
    drv_usecwait(25);

    return;
}


/*
 * Name         : eepro_watchdog()
 * Purpose      : Watchdog routine to handle board idiosyncrasies and
 *                dys/mal-functionality
 * Called from  : Kernel
 * Arguments    : ptr - a caddr_t pointer to be interpreted as a pointer
 *                      to a gld_mac_info_t structure
 * Returns      : None
 * Side effects : eepro_watch is reset to 0 on return.
 *                timeout_id is updated to have the new return value from
 *                timeout().
 *                Board is reset if it is detected to be dys/mal-functional.
 */

static void
eepro_watchdog(caddr_t ptr)
{
    gld_mac_info_t *macinfop = (gld_mac_info_t *) ptr;
    struct eeproinstance *eeprop;
    register int base_io_address;
    int    board_no;    /* instance number of the board */
    unchar val;         /* scratch */

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_WDOG)
    {
        cmn_err(CE_NOTE, "eepro: watchdog(0x%x) entered", ptr);
    }
#endif EEPRODEBUG

    if (macinfop == NULL || macinfop->gldm_private == NULL)
    {
        cmn_err(CE_NOTE,
                "eepro: fatal error in watchdog(): private pointer NULL");
        return;
    }
    mutex_enter(&macinfop->gldm_maclock);
    eeprop = (struct eeproinstance *) macinfop->gldm_private;

    /*
     * Display cable related errors if necessary
     */

    if (eeprop->eepro_watch & EEPRO_NOXVR)
    {
        board_no = ddi_get_instance(macinfop->gldm_devinfo);
        cmn_err(CE_CONT,
            "eepro%d: lost carrier (cable or transceiver problem?)\n",
            board_no);

        /*
         * Restart timeouts again and release the mutex
         */

        eeprop->timeout_id =
            timeout(eepro_watchdog, (caddr_t) macinfop, EEPRO_WDOG_TICKS);
        eeprop->eepro_watch = 0;
        mutex_exit(&macinfop->gldm_maclock);
        return;
    }

    /*
     * Check if the board is active since the last call to this function
     */

    if ((eeprop->eepro_watch & EEPRO_ACTIVE) != EEPRO_ACTIVE)
    {
        /*
         * Issue a DIAGNOSE command to the board and check if it responds
         */

#ifdef EEPRODEBUG 
        if (eeprodebug & DEBUG_WDOG) 
        {
            board_no = ddi_get_instance(macinfop->gldm_devinfo);
            cmn_err(CE_WARN, "eepro%d: restarting board in eepro_watchdog()",
                    board_no);
        }
#endif EEPRODEBUG 

        base_io_address = macinfop->gldm_port;

        if (eepro_wait_exec(base_io_address) != SUCCESS)
        {

#ifdef EEPRODEBUG
            if (eeprodebug & DEBUG_WDOG) 
            {
                board_no = ddi_get_instance(macinfop->gldm_devinfo);
                cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                                 board_no, __LINE__);
            }
#endif EEPRODEBUG

            goto resume_watch;
        }

        EEPRO_CLR_EXEC_STAT(base_io_address);
        outb(base_io_address + CMD_REG, CR_DIAGNOSE);

        if (eepro_wait_exec(base_io_address) != SUCCESS)
        {

#ifdef EEPRODEBUG
            if (eeprodebug & DEBUG_WDOG) 
            {
                board_no = ddi_get_instance(macinfop->gldm_devinfo);
                cmn_err(CE_CONT, "eepro%d: exec unit busy @ line %d\n",
                        board_no, __LINE__);
            }
#endif EEPRODEBUG

            goto resume_watch;
        }

        EEPRO_CLR_EXEC_STAT(base_io_address);
        val = inb(base_io_address + CMD_REG);
        if ((val & CMD_MASK) != CR_DIAGNOSE)
        {
            /*
             * Board failure! Give up and reset the board
             */

            cmn_err(CE_NOTE, "eepro%d: board in slot %d failed- resetting",
                    ddi_get_instance(macinfop->gldm_devinfo),
                    base_io_address / 0x1000);
            (void) eepro_reset(macinfop);
            (void) eepro_start_board(macinfop);
        }
    }

resume_watch:

    /*
     * Restart timeouts again and release the mutex
     */

    eeprop->timeout_id =
        timeout(eepro_watchdog, (caddr_t) macinfop, EEPRO_WDOG_TICKS);
    eeprop->eepro_watch = 0;
    mutex_exit(&macinfop->gldm_maclock);
    return;
}


/*
 * Name         : eepro_tdr_test()
 * Purpose      : Perform the TDR test to detect presence of any
 *                cable/tranceiver faults
 * Called from  : eepro_ioctl()
 * Arguments    : macinfop - pointer to a gld_mac_info_t structure
 * Returns      : Result of the TDR test outputted by 82595TX
 * Side effects : None
 */

static
int
eepro_tdr_test(gld_mac_info_t *macinfop)
{
    register int base_io_address = (int) macinfop->gldm_port;
    ushort   xmtbar;    /* value of the XMT BAR register */
    ushort   status;    /* status field in the result of TDR command */
    ushort   val;       /* scratch */
    int      count;     /* scratch */
    cmd_preamble_t tdr_cmd; /* TDR command's preamble */

    outb(base_io_address + CMD_REG, SEL_BANK0 | CR_SWITCH_BANK);

    /*
     * Fill up the cmd_preamble_t structure
     */

    tdr_cmd.cmd           = CR_TDR;
    tdr_cmd.status        = 0;
    tdr_cmd.nxt_chain_ptr = 0;
    tdr_cmd.byte_count    = 0;

    outw(base_io_address + HAR_LOW, NONXMT_AREA_START); 
    repoutsw(base_io_address + LMEM_IO_LOW, (ushort *) &tdr_cmd,
             sizeof (cmd_preamble_t) / 2);

    /*
     * Save the original value of the XMT BAR before initializing it
     * again
     */

    xmtbar = inw(base_io_address + XMT_BAR_LOW); 
    outw(base_io_address + XMT_BAR_LOW, NONXMT_AREA_START);


    outb(base_io_address + CMD_REG, CR_TDR);
    drv_usecwait(50);

    /*
     * Wait in a tight loop till the command is completed
     */
    
    count = 0;
    do 
    {
        outw(base_io_address + HAR_LOW, NONXMT_AREA_START); 
        val = inw(base_io_address + LMEM_IO_LOW);
        drv_usecwait(50);
    } while (((val & CMD_DONE) != CMD_DONE) && (count++ < 100));

    EEPRO_CLR_EXEC_STAT(base_io_address);
    outw(base_io_address + XMT_BAR_LOW, xmtbar); 

    /*
     * Gather result of the TDR command and pass it up
     */

    outb(base_io_address + CMD_REG, SEL_BANK0 | CR_SWITCH_BANK);
    outw(base_io_address + HAR_LOW, NONXMT_AREA_START); 

    val = inw(base_io_address + LMEM_IO_LOW);
    status = inw(base_io_address + LMEM_IO_LOW);

#ifdef EEPRODEBUG
    if (eeprodebug & DEBUG_IOCTL)
    {
        int board_no = ddi_get_instance(macinfop->gldm_devinfo);

        cmn_err(CE_CONT, "eepro%d: result of TDR test 0x%x\n", board_no,
                status);
    }
#endif EEPRODEBUG

    return (status);
}


/*
 * Name         : eepro_force_reset()
 * Purpose      : Forces reset of the board when the system is to be shut
 *                down
 * Called from  : Kernel
 * Arguments    : devinfop - pointer to a dev_info_t structure
 *              : cmd      - argument whoe value should be DDI_RESET_FORCE
 * Returns      : DDI_SUCCESS on success
 *                DDI_FAILURE on failure
 * Side effects : None
 */

static int
eepro_force_reset(dev_info_t *devinfop, ddi_reset_cmd_t cmd)
{
    gld_mac_info_t *macinfop;

#ifdef RILESDEBUG
    if (eeprodebug & DEBUG_DDI)
    {
        cmn_err(CE_CONT, "eepro_force_reset(0x%x)", (int) devinfop);
    }
#endif RILESDEBUG

    if (cmd != DDI_RESET_FORCE)
    {
        return (DDI_FAILURE);
    }

    /*
     * Get the gld private (gld_mac_info_t) and the driver private
     * data structures
     */

    macinfop = (gld_mac_info_t *) ddi_get_driver_private(devinfop);

    /*
     * Stop the board if it is running
     */

    (void) eepro_stop_board(macinfop);

    return (DDI_SUCCESS);
}
