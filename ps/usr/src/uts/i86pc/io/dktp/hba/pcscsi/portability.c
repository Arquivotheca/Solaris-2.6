/*
 * Copyright (c) 1995 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma ident   "@(#)portability.c 95/06/05 SMI"

/* ========================================================================== */
/*
 * *** Portability Layer routines ***
 *
 * These routines are called from various places in the AMD core code.
 * Basically they form the API between the AMD core and the (lower level)
 * OS functionality and the hardware.
 *
 * The routine names and parameters are specified by the AMD core code.
 * (There're in the AMD doc too...sort of.)
 *
 * The prototypes are in the AMD code too.  The prototypes were copied
 * and used in the function definition, which is why non-Solaris
 * types will be seen in them (e.g. PVOID).
 *
 * THE CORE MUTEX IS *HELD* WHEN EACH OF THESE ROUTINES IS CALLED
 * -EXCEPT-
 * ScsiPortInitialize, as nothing else is active when this runs.
 *
 * Most of these routines are called from INTERRUPT CONTEXT.
 * Those that aren't are initialization stuff:
 *	ScsiPortInitialize,
 *	ScsiPortGetUncachedExtension(
 *	ScsiPortFreeDeviceBase(
 *	ScsiPortGetDeviceBase(
 */


/* -------------------------------------------------------------------------- */
/*
 * Includes
 */
#include <sys/scsi/scsi.h>
#include <sys/dktp/hba.h>
#include <sys/types.h>
#include <sys/pci.h>


/*
 * Includes specifically for the AMD core code.
 */
#include "miniport.h"   /* Includes "srb.h"	 */
#include "scsi.h"
#include "ggmini.h"


/*
 * Driver-specific.
 * Order is significant.
 */
#include <sys/dktp/pcscsi/pcscsi_dma_impl.h>
#include <sys/dktp/pcscsi/pcscsi.h>


/* ========================================================================== */
/*
 * Globals
 */
#ifdef PCSCSI_DEBUG
extern uint	 pcscsig_debug_funcs;		/* Defined in pcscsi.c	*/
extern uint	 pcscsig_debug_gran;		/* Defined in pcscsi.c	*/
extern char	 pcscsig_dbgmsg[];		/* Defined in pcscsi.c	*/
#endif /* PCSCSI_DEBUG */

/*
 * Used by the PCSCSI_KVTOP macro.
 * Defined and initialized in pcscsi.c
 */
extern int pcscsig_pgsz;
extern int pcscsig_pgmsk;
extern int pcscsig_pgshf;


/* ========================================================================== */
/*
 * This routine called only from attach (via the core routine
 * DriverEntry).
 * It returns to the *core*, so be sure to return values the core
 * understands.
 */
ULONG
ScsiPortInitialize(
	IN PVOID Argument1,				/* Ptr to pcscsi_blk  */
	IN PVOID Argument2,				/* Null	*/
	IN HW_INITIALIZATION_DATA *HwInitializationData, /* AMD provides */
	IN PVOID HwContext)				/* AMD provides */
{
	struct pcscsi_blk	*pcscsi_blk_p = Argument1;
	int			i;
	ULONG			AMDReturnStatus;
	BOOLEAN			again;


	/*
	 * Save the SpecificLuExtensionSize for use in tran_tgt_init to
	 * allocate the struct for each (active) target.
	 */
	pcscsi_blk_p->pb_core_SpecificLuExtensionSize =
				HwInitializationData->SpecificLuExtensionSize;


	/*
	 * Save the SrbExtensionSize for use in the transport
	 * routine, to allocate and link this struct to each new Srb.
	 */
	pcscsi_blk_p->pb_core_SrbExtensionSize =
				HwInitializationData->SrbExtensionSize;


	/*
	 * Allocate space for:
	 *	DeviceExtension
	 *	PortConfigInfo
	 *	Access Ranges (whatever they are)
	 * in one big block.
	 * This memory is deallocated in pcscsi_device_teardown in
	 * pcscsi.c.
	 */
	pcscsi_blk_p->pb_core_DeviceExtension_p =
		(PSPECIFIC_DEVICE_EXTENSION)
			kmem_zalloc(
				HwInitializationData->DeviceExtensionSize   +
				sizeof (PORT_CONFIGURATION_INFORMATION)	  +
				(HwInitializationData->NumberOfAccessRanges
					* sizeof (ACCESS_RANGE)),
				KM_SLEEP);
	if (!pcscsi_blk_p->pb_core_DeviceExtension_p)
		return (SP_RETURN_ERROR);  /* Tell core we failed. */


	pcscsi_blk_p->pb_core_PortConfigInfo_p =
		(PPORT_CONFIGURATION_INFORMATION)
			(pcscsi_blk_p->pb_core_DeviceExtension_p + 1);


	/*
	 * Just set the AccessRanges up the same way AMD's SCO driver did.
	 */
	pcscsi_blk_p->pb_core_AccessRanges_p =
		(ACCESS_RANGE (*)[])
		(pcscsi_blk_p->pb_core_PortConfigInfo_p + 1);


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"ScsiPortInitialization: pcscsi_blk_p: %x\n",
			pcscsi_blk_p));
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"ScsiPortInitialization: DeviceExtension_p: %x\n",
			pcscsi_blk_p->pb_core_DeviceExtension_p));
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"ScsiPortInitialization: pb_core_PortConfigInfo_p: %x\n",
			pcscsi_blk_p->pb_core_PortConfigInfo_p));
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"ScsiPortInitialization: pb_core_AccessRanges_p: %x\n",
			pcscsi_blk_p->pb_core_AccessRanges_p));
#endif /* PCSCSI_DEBUG */


	/*
	 *  Initialize the function pointers to the core routines.
	 *  Gotten from (temporary) structure allocated and initialized in Core.
	 */
	pcscsi_blk_p->pb_AMDInitializeAdapter	=
					HwInitializationData->HwInitialize;
	pcscsi_blk_p->pb_AMDStartIo			=
					HwInitializationData->HwStartIo;
	pcscsi_blk_p->pb_AMDInterruptServiceRoutine	 =
					HwInitializationData->HwInterrupt;
	pcscsi_blk_p->pb_AMDFindAdapter 		=
					HwInitializationData->HwFindAdapter;
	pcscsi_blk_p->pb_AMDResetScsiBus		=
					HwInitializationData->HwResetBus;

	/*
	 * Not used.  Both are defined in the struct, but not used here.
	 *
	 * pcscsi_blk_p->pb_AMDDmaStarted			=
	 *				HwInitializationData->HwDmaStarted;
	 * pcscsi_blk_p->pb_AMDAdapterState		=
	 *				HwInitializationData->HwAdapterState;
	 *
	 */


	/*
	 * HwInitializationData struct is completely initialized by the
	 * core (in ggmini.c: DriverEntry)
	 * NOTE HwInitializationData passed in is a TEMPORARY struct
	 * allocated on the stack.
	 *
	 * Save all this data for now - though it's not currently needed.
	 */
	pcscsi_blk_p->pb_core_HwInitializationData = *HwInitializationData;


	/* ----------------------------------------------------------- */
	/*
	 * Initialize PORT_CONFIGURATION_INFORMATION struct.
	 * (Defined in srb.h)
	 */

	pcscsi_blk_p->pb_core_PortConfigInfo_p->Length =
				sizeof (PORT_CONFIGURATION_INFORMATION);


	/*
	 * The following fields in the PORT_CONFIGURATION_INFORMATION
	 * struct are either initialized elsewhere or not used:
	 *
	 * ULONG SystemIoBusNumber;
	 *
	 * INTERFACE_TYPE AdapterInterfaceType;
	 *
	 * ULONG BusInterruptLevel;
	 *
	 * ULONG BusInterruptVector;	- Not used
	 *
	 * Interrupt mode (level-sensitive or edge-triggered)
	 * KINTERRUPT_MODE InterruptMode;
	 *	ConfigInfo->InterruptMode = LevelSensitive;
	 *	(set in ggmini.c: AMDFindAdapter)
	 *
	 * Maximum number of bytes that can be transferred in a single SRB
	 *	ULONG MaximumTransferLength;
	 *		ConfigInfo->MaximumTransferLength = 0x1000000-0x1000;
	 *		(set in ggmini.c: AMDFindAdapter)
	 *
	 * Number of contiguous blocks of physical memory
	 *	ULONG NumberOfPhysicalBreaks;
	 *    ConfigInfo->NumberOfPhysicalBreaks = MAXIMUM_MDL_DESCRIPTORS - 1;
	 *		(set in ggmini.c: AMDFindAdapter)
	 *
	 * DMA channel for devices using system DMA
	 * ULONG DmaChannel;	- Not used
	 * ULONG DmaPort;		- Not used
	 * DMA_WIDTH DmaWidth;	- Not used
	 * DMA_SPEED DmaSpeed;	- Not used
	 *
	 * Alignment masked required by the adapter for data transfers.
	 * ULONG AlignmentMask;	- Not used
	 */


	/*
	 * Record Number of access range elements which have been allocated.
	 */
	pcscsi_blk_p->pb_core_PortConfigInfo_p->NumberOfAccessRanges =
				HwInitializationData->NumberOfAccessRanges;


	/*
	 * The following fields in the PORT_CONFIGURATION_INFORMATION
	 * struct are either initialized elsewhere or not used:
	 *
	 * Pointer to array of access range elements.
	 *
	 * ACCESS_RANGE (*AccessRanges)[];
	 *	(Allocated above).
	 */


	/*
	 * Point our copy of PortconfigInfo.AccessRanges to the allocated
	 * space.
	 */
	pcscsi_blk_p->pb_core_PortConfigInfo_p->AccessRanges =
					pcscsi_blk_p->pb_core_AccessRanges_p;


	/*
	 * The following fields in the PORT_CONFIGURATION_INFORMATION
	 * struct are either initialized elsewhere or not used:
	 * Reserved field.
	 * PVOID Reserved;	- Not used
	 *
	 * Reserved field.
	 * PVOID Reserved;	- Not used
	 *
	 * Number of SCSI buses attached to the adapter.
	 * UCHAR NumberOfBuses;
	 *	ConfigInfo->NumberOfBuses = 1;
	 *	(set in ggmini.c: AMDFindAdapter)
	 */


	/*
	 * Set the InitiatorBusId.
	 *
	 * SCSI bus ID for adapter
	 *	CCHAR InitiatorBusId[8];
	 */
	pcscsi_blk_p->pb_core_PortConfigInfo_p->InitiatorBusId[0] =
		pcscsi_blk_p->pb_initiator_id;


	/*
	 * The following fields in the PORT_CONFIGURATION_INFORMATION
	 * struct are either initialized elsewhere or not used:
	 *
	 * Indicates that the adapter does scatter/gather
	 * BOOLEAN ScatterGather;
	 *	ConfigInfo->ScatterGather = TRUE;
	 *	(set in ggmini.c: AMDFindAdapter)
	 *
	 * Indicates that the adapter is a bus master
	 * BOOLEAN Master;
	 *	ConfigInfo->Master = TRUE;
	 *	(set in ggmini.c: AMDFindAdapter)
	 *
	 * Host caches data or state.
	 *	BOOLEAN CachesData;	- Not used
	 *	(Not used)
	 *
	 * Host adapter scans down for bios devices.
	 *	BOOLEAN AdapterScansDown;	- Not used
	 *	(Not used)
	 *
	 * Primary at disk address (0x1F0) claimed.
	 *	BOOLEAN AtdiskPrimaryClaimed;	- Not used
	 *	(Not used)
	 *
	 * Secondary at disk address (0x170) claimed.
	 *	BOOLEAN AtdiskSecondaryClaimed;	- Not used
	 *	(Not used)
	 *
	 * The master uses 32-bit DMA addresses.
	 *	BOOLEAN Dma32BitAddresses;
	 *	ConfigInfo->Dma32BitAddresses = TRUE;
	 *	(set in ggmini.c: AMDFindAdapter)
	 *
	 * Use Demand Mode DMA rather than Single Request.
	 * BOOLEAN DemandMode;	- Not used
	 *
	 * Data buffers must be mapped into virtual address space.
	 * BOOLEAN MapBuffers;
	 * (Not used; But NOTE identically-named field in HW_INITIALIZATION_INFO
	 * struct)
	 *
	 * The driver will need to tranlate virtual to physical addresses.
	 * BOOLEAN NeedPhysicalAddresses;
	 * (Not used; But NOTE identically-named field in HW_INITIALIZATION_INFO
	 * struct
	 *
	 * Supports tagged queuing
	 * BOOLEAN TaggedQueuing;
	 *	ConfigInfo->TaggedQueuing =
	 *			(BOOLEAN)DeviceExtension->SysFlags.EnableTQ;
	 * (set in ggmini.c: AMDFindAdapter - but never used)
	 *
	 * Supports auto request sense.
	 * BOOLEAN AutoRequestSense;
	 *	ConfigInfo->AutoRequestSense = FALSE;
	 *	(Set in ggmini.c: AMDFindAdapter)
	 *
	 * Supports multiple requests per logical unit.
	 * BOOLEAN MultipleRequestPerLu;
	 * (Not used.  NOTE field with identical name in HW_INITIALIZATION_DATA
	 * struct)
	 *
	 * Support receive event function.
	 * BOOLEAN ReceiveEvent; (Not used)
	 *
	 * Indicates the real-mode driver has initialized the card.
	 * BOOLEAN RealModeInitialized; (Not used)
	 *
	 * Indicate that the miniport will not touch the data buffers directly.
	 * BOOLEAN BufferAccessScsiPortControlled; (Not used)
	 *
	 * Indicator for wide scsi.
	 * UCHAR   MaximumNumberOfTargets; (Not used)
	 *
	 * Ensure quadword alignment.
	 * UCHAR   ReservedUchars[6];
	 */


	/* ---------------------------------------------------------- */
	/*
	 * Initialize DeviceExtension struct.
	 *
	 * Virtually all of this struct is initialized in ggmini.c:
	 * AMDFindAdapter and AMDFindPciAdapter.  Other fields are
	 * initialized in AMDInitializeAdapterAdapter (called as MiniPort
	 *
	 *	The only thing we have to initialize is the PciConfigInfo
	 *	struct (of type CONFIG_BLOCK, defined in ggmini.h):
	 *
	 *	Configuration information structure
	 *	typedef struct _CONFIG_BLOCK {
	 *		UCHAR Method;		PCI Config Space access method
	 *		UCHAR FunctionNumber;	PCI bus device:	Function number
	 *		UCHAR BusNumber;			Bus number
	 *		UCHAR DeviceNumber;			Device number
	 *		ULONG BaseAddress;	I/O space base address of regs
	 *		ULONG IRQ;		IRQ device presents to the PIC
	 *		UCHAR ChipRevision;	Chip Revision
	 *		BOOLEAN PciFuncSupport;
	 *	} CONFIG_BLOCK, *PCONFIG_BLOCK;
	 *
	 *	Of these, only
	 *		BaseAddress,
	 *		IRQ,
	 *		ChipRevision
	 *	need be set in the
	 *	Solaris driver.  The rest are not used, or not relevant
	 *	given the Solaris PCI framework.
	 *
	 */


	/*
	 * Provide a hook from the DeviceExtension struct back to the
	 * blk struct.
	 */
	pcscsi_blk_p->pb_core_DeviceExtension_p->pcscsi_blk_p = pcscsi_blk_p;


	/*
	 * Set BaseAddress, IRQ, and ChipRevision.
	 */
	pcscsi_blk_p->pb_core_DeviceExtension_p->PciConfigInfo.BaseAddress =
						pcscsi_blk_p->pb_ioaddr;

	pcscsi_blk_p->pb_core_DeviceExtension_p->PciConfigInfo.IRQ =
						pcscsi_blk_p->pb_intr;

	pcscsi_blk_p->pb_core_DeviceExtension_p->PciConfigInfo.ChipRevision =
			pci_config_getb(pcscsi_blk_p->pb_pci_config_handle,
					PCI_CONF_REVID);

#ifdef PCSCSI_DEBUG
	/* Display the PciConfigInfo struct	*/
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
			"ScsiPortInitialize: PciConfigInfo.Method %x\n",
			pcscsi_blk_p->pb_core_DeviceExtension_p->
					PciConfigInfo.Method));
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
			"ScsiPortInitialize: PciConfigInfo.FunctionNumber %x\n",
			pcscsi_blk_p->pb_core_DeviceExtension_p->
					PciConfigInfo.FunctionNumber));
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
			"ScsiPortInitialize: PciConfigInfo.BusNumber %x\n",
			pcscsi_blk_p->pb_core_DeviceExtension_p->
					PciConfigInfo.BusNumber));
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
			"ScsiPortInitialize: PciConfigInfo.DeviceNumber %x\n",
			pcscsi_blk_p->pb_core_DeviceExtension_p->
					PciConfigInfo.DeviceNumber));
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
			"ScsiPortInitialize: PciConfigInfo.BaseAddress %x\n",
			pcscsi_blk_p->pb_core_DeviceExtension_p->
					PciConfigInfo.BaseAddress));
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
			"ScsiPortInitialize: PciConfigInfo.ChipRevision %x\n",
			pcscsi_blk_p->pb_core_DeviceExtension_p->
					PciConfigInfo.ChipRevision));
#endif /* PCSCSI_DEBUG */


	/* ---------------------------------------------------------- */
	/*
	 * Ask the core to go probe for an adapter.
	 *
	 * Note we've castrated the actual probe routine
	 * (AMDFindPciAdapter) by #ifdefing out everything that touches
	 * the PCI bus in AMD's code.  So, this routine now just
	 * does some further initialization of the PortConfigInfo struct,
	 * and returns.
	 */

	AMDReturnStatus = (*pcscsi_blk_p->pb_AMDFindAdapter)(
		pcscsi_blk_p->pb_core_DeviceExtension_p,  /* Just allocated */
		HwContext,		/* Argument passed from core	*/
		NULL,			/* "Bus Information" (unused)	*/
		"",			/* (unused)			*/
		pcscsi_blk_p->pb_core_PortConfigInfo_p, /* Just allocd/inited */
		&again);
		/*
		 * Ignore the 'again' value;
		 * this driver is designed * to keep seperate data structures
		 * for each attach (and hence each * instance of the h/w).
		 * Hence the core will always think it's dealing with
		 * only one board.
		 */


	if (AMDReturnStatus != SP_RETURN_FOUND)	{

		/*
		 * Shouldn't ever happen with the way ggmini is modified
		 * for Solaris.
		 */
		cmn_err(CE_WARN, "pcscsi: ScsiPortInitialize: "
				"Bad probe status");
		kmem_free(
				pcscsi_blk_p->pb_core_DeviceExtension_p,
			HwInitializationData->DeviceExtensionSize   +
			sizeof (PORT_CONFIGURATION_INFORMATION)	  +
			(HwInitializationData->NumberOfAccessRanges
				* sizeof (ACCESS_RANGE)));
		return (AMDReturnStatus);  /* Tell core we failed. */
	}


	/*
	 * See if the .conf file specifies Compaq-specific behavior
	 * be disabled.
	 */
	if (pcscsi_blk_p->pb_disable_compaq_specific)  {
		pcscsi_blk_p->pb_core_DeviceExtension_p->SysFlags.MachineType =
		    NON_COMPAQ;
	}


	/*
	 * These params are set to the following values in
	 * AMDFindAdapter:
	 *
	 *	Synchronous negotiation: Enable
	 * Make a .conf property?	*
	 * DeviceExtension->SysFlags.EnableSync = 1;
	*
	 *	Tagged queuing:		Enable
	 * Make a .conf property?
	 * We want it disabled.
	 * pcscsi_blk_p->pb_core_DeviceExtension_p->SysFlags.EnableTQ = 1;
	 */

	/*
	 * We want TQ disabled.
	 */
	pcscsi_blk_p->pb_core_DeviceExtension_p->SysFlags.EnableTQ = 0;


	/*
	 * These params are set to the following values in
	 * AMDFindAdapter:
	 *
	 *	SCSI bus parity:	Enable		***
	 * DeviceExtension->SysFlags.EnablePrity = 1;
	 *
	 *	CPU Mode (tracked to detect Real/Protected mode changes) ***
	 * DeviceExtension->SysFlags.CPUMode = PROTECTION_MODE;
	 */


	/*
	 *	DMA 'method' employed by the core...
	 * DeviceExtension->SysFlags.DMAMode = IO_DMA_METHOD_LINEAR;
	 *	Options for DMAMode (from ggmini.h):
	 *		#define IO_DMA_METHOD_LINEAR	0x01
	 *		#define IO_DMA_METHOD_MDL	   0x02
	 *		#define IO_DMA_METHOD_S_G_LIST  0x04
	 *
	 * DMAMode *must* be set to IO_DMA_METHOD_S_G_LIST.
	 * MDL is out because the functionality is broken on the (T1) chip.
	 * LINEAR means you're passing the address of a DMAable
	 * physically contiguous buffer.  Don't use it.
	 */
	pcscsi_blk_p->pb_core_DeviceExtension_p->SysFlags.DMAMode =
							IO_DMA_METHOD_S_G_LIST;


	/* ---------------------------------------------------------- */
	/*
	 * Call AMD's initialize adapter (or reset adapter) routine.
	 * Note this does some initialization of the structs as well.
	 */
	AMDReturnStatus = (*pcscsi_blk_p->pb_AMDInitializeAdapter)(
				pcscsi_blk_p->pb_core_DeviceExtension_p);

	/*
	 * Return status from AMDInitializeAdapter is currently
	 * hardwired to TRUE... but what the heck.
	 */
	if (AMDReturnStatus != TRUE)  {
		cmn_err(CE_WARN, "pcscsi: ScsiPortInitialize: "
				"Bad Initialization status");
		kmem_free(
				pcscsi_blk_p->pb_core_DeviceExtension_p,
			HwInitializationData->DeviceExtensionSize   +
			sizeof (PORT_CONFIGURATION_INFORMATION)	  +
			(HwInitializationData->NumberOfAccessRanges
				* sizeof (ACCESS_RANGE)));
		return (AMDReturnStatus);  /* Tell core we failed. */
	}

	return (SP_RETURN_FOUND);	/*  Return Success */

}

/* ========================================================================== */
/*
 * Called but does nothing under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortFreeDeviceBase(
	IN PVOID HwDeviceExtension,
	IN PVOID MappedAddress
)
{
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
ULONG
ScsiPortGetBusData(
	IN PVOID DeviceExtension,
	IN ULONG BusDataType,
	IN ULONG SystemIoBusNumber,
	IN ULONG SlotNumber,
	IN PVOID Buffer,
	IN ULONG Length
)
{
	cmn_err(CE_PANIC, "ScsiPortGetBusdata:  Should never be called!\n");
	return (DDI_SUCCESS);
}

/* ========================================================================== */
/*
 * Holdover from Windows NT.
 * Does nothing but return the IoAddress - from the IoAddress passed in.
 */
SCSIPORT_API
PVOID
ScsiPortGetDeviceBase(
	IN PVOID HwDeviceExtension,
	IN INTERFACE_TYPE BusType,
	IN ULONG SystemIoBusNumber,
	IN SCSI_PHYSICAL_ADDRESS IoAddress,
	IN ULONG NumberOfBytes,
	IN BOOLEAN InIoSpace
)
{

	/*
	 * Following is copied from the AMD SCO driver:
	 * #########################################################
	 * This returns a base address suitable for use by the
	 * hardware access function.
	 * To make HBA miniport drivers portable between systems,
	 * the addresses used must be translated.
	 * #########################################################
	 */

#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"ScsiPortGetDeviceBase: "
			"IoAddress.u.LowPart  %x  IoAddress.u.HighPart %x\n",
			IoAddress.u.LowPart, IoAddress.u.HighPart));
#endif	/* PCSCSI_DEBUG	*/

	return ((PVOID)IoAddress.u.LowPart);
}

/* ========================================================================== */
/*
 * Return a pointer to the per-Lun structure maintained by the core,
 * given Path/Target/Lun.
 */
SCSIPORT_API
PVOID
ScsiPortGetLogicalUnit(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun
)
{

	/*
	 * PathId should always be zero, so it's of no interest.
	 */


	/*
	 * Make sure this unit has been initialized before letting
	 * the core access it.
	 *
	 * ASSERT(((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
	 *	->pcscsi_blk_p->pb_unit_structs[TargetId][Lun]
	 *		!= NULL);
	 *
	 * ...but we can't do this.
	 * The core code (ggmini.c) contains loops for all possible
	 * luns - not just those that have been initialized.
	 * We need to return NULL for unitiialized luns, and the core
	 * 'does the right thing'.
	 * Everything is initialized to NULL, so this will work.
	 */
	if (
		((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
			->pcscsi_blk_p->pb_unit_structs[TargetId][Lun]
			== NULL)
		return (NULL);	/* Target/lun not initialized. */

	return ((PVOID) (
		((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
			->pcscsi_blk_p->pb_unit_structs[TargetId][Lun]
				->pu_SpecificLuExtension));

}

/* ========================================================================== */
/*
 * Return a pointer to the SRB struct, given the Path/Target/Lun/QueueTag
 * for the request.
 * Called for outstanding requests to regain access to the SRB.
 */
SCSIPORT_API
PSCSI_REQUEST_BLOCK
ScsiPortGetSrb(
	IN PVOID DeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN LONG QueueTag)
{
	struct pcscsi_unit	*unit_p;


	/*
	 * PathId should always be zero, so it's of no interest.
	 */


	unit_p = ((PSPECIFIC_DEVICE_EXTENSION)(DeviceExtension))
			->pcscsi_blk_p->pb_unit_structs[TargetId][Lun];

	/*
	 * Make sure this unit has been initialized before letting
	 * the core access it.
	 */
	ASSERT(unit_p != NULL);


	/*
	 * Make sure there really is a ccb here.
	 */

	/*
	 * In some cases the core *may* make
	 * this call to see if the SRB is still active, when it's
	 * already been completed.
	 * This may be overkill -
	 */
	ASSERT(unit_p->pu_active_ccbs[QueueTag] != NULL);

	return ((SCSI_REQUEST_BLOCK *)
			(unit_p->pu_active_ccbs[QueueTag]->ccb_hw_request_p));
}

/* ========================================================================== */
/*
 * Return the 'physical address" (32 bit) given a 'virtual address' (64-bit).
 * This is a holdover artifact from Windows NT.
 * The upper 32 bits aren't used anywhere, just schlepped around.
 */
SCSIPORT_API
SCSI_PHYSICAL_ADDRESS
ScsiPortGetPhysicalAddress(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb,
	IN PVOID VirtualAddress,
	OUT ULONG *Length)
{
	SCSI_PHYSICAL_ADDRESS PhysicalAddress;


	/*
	 *	Do *not* implement this generically (via KVTOP)!
	 *	The core should only *ever* call this routine with the
	 *	address of the TempLinearBuffer (allocated via ddi_iopb_alloc
	 *	(via ScsiPortAllocateUncachedExtension),
	 *	If it ever tries to get the address of something else,
	 *	we should panic because we don't know what's happening..
	 */

	if (VirtualAddress !=
		((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
			->pcscsi_blk_p->pb_tempbuf_p) {
		cmn_err(CE_PANIC, "pcscsi: "
		"ScsiPortGetPhysicalAddress called with unexpected address");
	}


	/*
	 * Ignore the uppper 32 bits; return the lower 32.
	 */
	PhysicalAddress.u.HighPart = 0;
	PhysicalAddress.u.LowPart  = (ULONG)
		((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
			->pcscsi_blk_p->pb_tempbuf_physaddr;

	return (PhysicalAddress);
}

/* ========================================================================== */
/*
 * Return a virtual address, given a physical address.
 *
 * There are two cases where this is called.
 *
 *	One is where the core requests the physical address of the
 *	temporary internal physically contiguous buffer it allocated
 *	via GetUncachedExtension (which allocated via ddi_iopb_alloc).
 *	In this case we just get the physical address (saved by
 *	GetUncachedExtension) and return it.
 *
 *	The other is where the core requests the physical address
 *	of an *artbitrary* virtual address within the data buffer.
 *	It this case we have to step through the array of
 *	virtual-address/byte-count/physical-address that we set up
 *	for this SRB when setting up DMA resources, until we discover which
 *	physical chunk of memory the address is in, then calculate
 *	the offset into the physical chunk to the byte the core
 *	wants the address of.
 */
SCSIPORT_API
PVOID
ScsiPortGetVirtualAddress(
	IN PVOID HwDeviceExtension,
	IN SCSI_PHYSICAL_ADDRESS PhysicalAddress,
	IN PSCSI_REQUEST_BLOCK Srb,
	IN ULONG DataTransferLength)
{
	paddr_t	phys_addr;
	struct	pcscsi_blk	*pcscsi_blk_p;
	struct	pcscsi_ccb	*ccb_p;
	PSCSI_REQUEST_BLOCK	srb_p;
	ulong_t			offset_into_seg;
	int			seg;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"ScsiPortGetVirtualAddress: DevExt:%x PhysAddr:%x\n",
		HwDeviceExtension, PhysicalAddress));
#endif /* PCSCSI_DEBUG */


	pcscsi_blk_p = ((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
					->pcscsi_blk_p;


	/*
	 *	Get the part of the SCSI_PHYSICAL_ADDRESS that the core
	 *	actually uses - PhysicalAddress.u.HighPart is never used.
	 */
	phys_addr = (paddr_t)PhysicalAddress.u.LowPart;


	/*
	 *	See if the physaddr is in the Temp buf allocated earlier.
	 */
	if (
		phys_addr >= pcscsi_blk_p->pb_tempbuf_physaddr &&
		phys_addr <= (pcscsi_blk_p->pb_tempbuf_physaddr
					+ pcscsi_blk_p->pb_tempbuf_length -1)) {

		/*
		 *	Buffer's length is (currently) hardwired to 7 bytes in
		 *	the core.
		 *	We can't assume it will stay that way, but it will be
		 *	small.
		 *
		 *	(It changed to 68+3 bytes in the 3/23/95 version of
		 *	ggmini.c.)
		 */

		offset_into_seg =
			phys_addr - pcscsi_blk_p->pb_tempbuf_physaddr;


		/*
		 * Note that there's an assumption here:
		 * That adding one to a paddr_t corresponds
		 * to adding one to a caddr_t.  Caveat emptor.
		 */

		return ((PVOID) (pcscsi_blk_p->pb_tempbuf_physaddr
						+ offset_into_seg));

	}	/* End if (address in temp buf)	*/


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS,
		"ScsiPortGetVirtualAddress: Address NOT in temp buf\n");
#endif /* PCSCSI_DEBUG */


	/*
	 *	If we get here, the physical address must be somewhere among
	 *	the DMA segments allocated for this transfer.
	 */

	srb_p = ((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
					->ActiveLuRequest;

	ccb_p = ((struct pcscsi_ccb *)srb_p->OriginalRequest);


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PORTABILITY, DBG_VERBOSE, sprintf(pcscsig_dbgmsg,
		"	srbp:%x ccb_p:%x\n", srb_p, ccb_p));
#endif /* PCSCSI_DEBUG */


	/*
	 * Step through each DMA segment for this transfer...
	 */
	for (seg = 0; seg < ccb_p->ccb_dma_p->dma_sg_nbr_entries; seg++)  {


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PORTABILITY, DBG_VERBOSE, sprintf(pcscsig_dbgmsg,
		"	seg:%x virtaddrs[seg]:%x entry[seg].addr:%x .len:%x\n",
		seg,
		ccb_p->ccb_dma_p->dma_sg_list_virtaddrs[seg],
		ccb_p->ccb_dma_p->dma_sg_list_p->sg_entries[seg].data_addr,
		ccb_p->ccb_dma_p->dma_sg_list_p->sg_entries[seg].data_len));
#endif /* PCSCSI_DEBUG */


		/*
		 * See if the address we want is in this segment.
		 */
		if (
			phys_addr  >=
		ccb_p->ccb_dma_p->dma_sg_list_p->sg_entries[seg].data_addr &&
			phys_addr <
		(ccb_p->ccb_dma_p->dma_sg_list_p->sg_entries[seg].data_addr +
		ccb_p->ccb_dma_p->dma_sg_list_p->sg_entries[seg].data_len))  {


			/*
			 * Then phys_addr is in this segment.
			 * Return the appropiate *virtual* address.
			 */
			offset_into_seg = phys_addr -
		ccb_p->ccb_dma_p->dma_sg_list_p->sg_entries[seg].data_addr;


			return ((PVOID)
				(ccb_p->ccb_dma_p->dma_sg_list_virtaddrs[seg]
							+ offset_into_seg));


		}	/* else not in this seg; continue	*/

	}	/* else phys_addr is not in the s/g list for this srb */

	cmn_err(CE_PANIC,
		"ScsiPortGetVirtualAddress:  Virt/Phys Translation failed!\n");
}

/* ========================================================================== */
/*
 * No-op under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortFreeVirtualAddress(
	IN PVOID VirtualAddress,
	IN PSCSI_REQUEST_BLOCK Srb,
	IN ULONG DataTransferLength)
{
}

/* ========================================================================== */
/*
 * Allocate a contiguous DMAable buffer.
 *
 * In the core this is only called once, to allocate an internal 'temp'
 * buffer, which the core uses.
 *
 * Because we have to provide the core with on-demand physical->virtual
 * address translation, this routine must save the virtual and physical
 * addresses of the buffer.
 *
 * Thus this routine is intimately tied with ScsiPortGetVirtualAddress.
 *
 * So, we limit this routine to only a single use (per driver instance),
 * for allocating the core's internal buffer.
 */
SCSIPORT_API
PVOID
ScsiPortGetUncachedExtension(
	IN PVOID HwDeviceExtension,
	IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	IN ULONG NumberOfBytes)
{
	caddr_t	new_space;


	/*
	 * This routine can only be called once per driver instance.
	 */
	ASSERT(((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
			->pcscsi_blk_p->pb_tempbuf_p
				== NULL);


	/*
	 * Allocate the buffer.
	 */
	if (ddi_iopb_alloc(
			((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
				->pcscsi_blk_p->pb_dip,
			((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
				->pcscsi_blk_p->pb_dma_lim_p,
			(u_int)NumberOfBytes,
			&new_space)
		!= DDI_SUCCESS) {

		cmn_err(CE_WARN, "(pcscsi) ScsiPortGetUncachedExtension: "
			"Could not allocate iopb space\n");
		return (NULL);
	}


	/*
	 * Save the pointers for
	 * 1) deallocation at detach time,
	 * 2) so we can ensure that the core isn't trying to access
	 * arbitrary physical addresses (via calls to GetPhysicalAddress -
	 * q.v.).
	 * 3) (physical address) so we're not constantly doing the
	 * virtual->physical address translation on the same core-internal
	 * buffer
	 */
	((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
			->pcscsi_blk_p->pb_tempbuf_p	/* Virtual addr	*/
				= new_space;

	((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
			->pcscsi_blk_p->pb_tempbuf_physaddr	/* Physical */
				= (paddr_t)PCSCSI_KVTOP(new_space);

	((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
			->pcscsi_blk_p->pb_tempbuf_length	/* Length */
				= (int)NumberOfBytes;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
	"GetUncachedExt: addr:%x physaddr:%x length:%x\n",
	((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
			->pcscsi_blk_p->pb_tempbuf_p,	/* Virtual addr	*/

	((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
			->pcscsi_blk_p->pb_tempbuf_physaddr,	/* Physical */

	((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
			->pcscsi_blk_p->pb_tempbuf_length));	/* Length */
#endif	/* PCSCSI_DEBUG */


	return ((PVOID) new_space);
}

/* ========================================================================== */
/*
 * Not used under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortFlushDma(
	IN PVOID DeviceExtension)
{
	cmn_err(CE_PANIC, "ScsiPortFlushDma:  Should never be called!\n");
}

/* ========================================================================== */
/*
 * Not used under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortIoMapTransfer(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb,
	IN PVOID LogicalAddress,
	IN ULONG Length)
{
	cmn_err(CE_PANIC, "ScsiPortIoMapTransfer:  Should never be called!\n");
}

/* ========================================================================== */
/*
 * This is the routine the core calls when it wants to notify us
 * of a significant event.
 *
 * The events are:
 *	- Request complete
 *	- Core is ready to accept (to start execution of) another request.
 *
 * There are others which are either are stubs in the core code, or aren't
 * used in this driver.
 */
SCSIPORT_API
VOID
ScsiPortNotification(
	IN SCSI_NOTIFICATION_TYPE NotificationType,
	IN PVOID HwDeviceExtension,
	...)
{
	va_list argptr;
	UCHAR PathId;
	UCHAR TargetId;
	UCHAR Lun;
	PSCSI_REQUEST_BLOCK srb_p;


	TargetId = NO_TARGET;
	Lun	 = NO_LUN;
	srb_p	 = NULL;


	switch (NotificationType) {

	case RequestComplete:
		/* Additional parameters passed: SRB that completed */


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS,
		"ScsiPortNotification: Request complete\n");
#endif /* PCSCSI_DEBUG */


		/*
		 * Get the SRB pointer for the request just completed.
		 */
		va_start(argptr, DeviceExtension);
			srb_p = va_arg(argptr, PSCSI_REQUEST_BLOCK);
		va_end(argptr);


		/*
		 * Invoke the driver-level routine to handle completion.
		 */
		pcscsi_request_completion(
			((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
				->pcscsi_blk_p,
				srb_p);
		break;


	case NextRequest:
		/* No additional parameters returned */


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS,
		"ScsiPortNotification: Next request\n");
#endif /* PCSCSI_DEBUG */


		/*
		 * Attempt to start another request.
		 */
		pcscsi_start_next_request(
			((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
				->pcscsi_blk_p,
			TargetId, Lun);
		/*
		 * No point in checking status; nothing we could do anyway.
		 */

		break;


	case NextLuRequest:
		/* Additional parameters passed: Path, Target, Lun */


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS,
		"ScsiPortNotification: Next LU request\n");
#endif /* PCSCSI_DEBUG */


		/*
		 * Get the passed target/lun/tag-queue info.
		 */
		va_start(argptr, HwDeviceExtension);
		PathId   = va_arg(argptr, UCHAR);
		TargetId = va_arg(argptr, UCHAR);
		Lun	  = va_arg(argptr, UCHAR);
		va_end(argptr);


		/*
		 * Attempt to start another request.
		 */
		pcscsi_start_next_request(
			((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))
				->pcscsi_blk_p,
			TargetId, Lun);
		/*
		 * No point in checking status; nothing we could do anyway.
		 */

		break;


	case ResetDetected:


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS,
		"ScsiPortNotification: SCSI bus reset detected\n");
#endif /* PCSCSI_DEBUG */


		/*
		 * No Srb provided; just a notification.
		 */

		break;


	/*
	 * Following are documented, but not implemented or called by the core:
	 *
	 *	case CallDisableInterrupts:
	 *		break;
	 *
	 *
	 *	case CallEnableInterrupts:
	 *		break;
	 *
	 *
	 *	case RequestTimerCall:
	 *		break;
	 *
	 */


	default:

		cmn_err(CE_CONT, "pcscsi: ScsiPortNotification: "
					"UNKNOWN notification type!\n");
		break;

	}	/* End switch(NotificationType)	*/

}

/* ========================================================================== */
/*
 * Glue routine which allows the core to print/log internal errors.
 */
SCSIPORT_API
VOID
ScsiPortLogError(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN ULONG ErrorCode,
	IN ULONG UniqueId)
{

	switch (ErrorCode) {
	case SP_PROTOCOL_ERROR:

		cmn_err(CE_WARN, "(pcscsi): SP_PROTOCOL_ERROR %d",
								UniqueId);
		break;

	case SP_BUS_PARITY_ERROR:

		cmn_err(CE_WARN, "(pcscsi): SP_BUS_PARITY_ERROR %d",
								UniqueId);
		break;

	case SP_INTERNAL_ADAPTER_ERROR:

		cmn_err(CE_WARN, "(pcscsi): SP_INTERNAL_ADAPTER_ERROR %d",
								UniqueId);
		break;

	case SP_UNEXPECTED_DISCONNECT:

		cmn_err(CE_WARN, "(pcscsi): SP_UNEXPECTED_DISCONNECT %d",
								UniqueId);
		break;

	case SP_INVALID_RESELECTION:

		cmn_err(CE_WARN, "(pcscsi): SP_INVALID_RESELECTION %d",
								UniqueId);
		break;

	case SP_BUS_TIME_OUT:

		cmn_err(CE_WARN, "(pcscsi): SP_BUS_TIME_OUT %d", UniqueId);
		break;

	default:

		cmn_err(CE_WARN, "(pcscsi): UNKNOWN ERROR %d", UniqueId);
		break;

	}	/* End switch(ErrorCode)	*/

}

/* ========================================================================== */
/*
 * This routine is misnamed.
 * It is called by the core to 'complete' -all- outstanding requests
 * on the given target/lun.
 * It's intended to be called to clean up after an abort or reset.
 *
 * NOTE this approach has a design flaw -
 * There is no provision to -not- kill off the request that initiated the
 * abort or reset.  Hence such a request will never return with a
 * 'success' status.  Need to do something about this.
 */
SCSIPORT_API
VOID
ScsiPortCompleteRequest(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN UCHAR SrbStatus)
{
	int			QueueTag;
	struct pcscsi_ccb	*ccb_p;
	PSCSI_REQUEST_BLOCK	SrbPtr;
	struct pcscsi_blk	*pcscsi_blk_p;
	struct pcscsi_unit	*unit_p;


	/*
	 * PathId should always be zero, so it's of no interest.
	 */


	/*
	 * Get the pointer to blk and unit struct for this target/lun.
	 */
	pcscsi_blk_p =
		((PSPECIFIC_DEVICE_EXTENSION)(HwDeviceExtension))->pcscsi_blk_p;
	unit_p = pcscsi_blk_p->pb_unit_structs[TargetId][Lun];

	/*
	 * Make sure this unit has been initialized before letting
	 * the core access it.
	 */
	ASSERT(unit_p != NULL);


	/*
	 * Try to find every active (busy) Srb and 'complete' it with
	 * the passed-in status.
	 */
	for (QueueTag = 0; QueueTag < MAX_QUEUE_TAGS_PER_LUN; QueueTag++)  {

		ccb_p = unit_p->pu_active_ccbs[QueueTag];
		if (ccb_p == NULL)  {
			continue;
		}


		/*
		 * Get the SRB for the in-progress request.
		 */
		SrbPtr =  (SCSI_REQUEST_BLOCK *)
			(unit_p->pu_active_ccbs[QueueTag]->ccb_hw_request_p);

		SrbPtr->SrbStatus = SrbStatus; /* Set completion status */

		/*
		 * Indicate that this SRB is 'done'.
		 */
		ScsiPortNotification(RequestComplete, HwDeviceExtension,
								SrbPtr);

	}	/* End for(all queue tags)	*/


#ifdef	UNIT_QUEUE_SIZE
	/*
	 * Clean out any queued requests for this target/lun.
	 */
	for (;;)  {
		ccb_p = pcscsi_dequeue_request((int)TargetId, (int)Lun,
						pcscsi_blk_p);

		if (ccb_p == NULL)  {		/* None left queued. */
			break;
		}


		/*
		 * Make this request the 'active' request, so we can 
		 * finish it off via the normal code path.
		 * Arbitrarily use tag 0.
		 */
		ASSERT(unit_p->pu_active_ccbs[0] == NULL);

		unit_p->pu_active_ccbs[0] = ccb_p;
		unit_p->pu_active_ccb_cnt ++;


		/*
		 * Get the SRB for the queued request.
		 */
		SrbPtr = ccb_p->ccb_hw_request_p;


		SrbPtr->SrbStatus = SrbStatus; /* Set completion status */


		/*
		 * Indicate that this SRB is 'done'.
		 */
		ScsiPortNotification(RequestComplete, HwDeviceExtension,
								SrbPtr);

	}
#endif	/* UNIT_QUEUE_SIZE	*/


}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortMoveMemory(
	IN PVOID WriteBuffer,
	IN PVOID ReadBuffer,
	IN ULONG Length)
{
	cmn_err(CE_PANIC, "ScsiPortMoveMemory:  Should never be called!\n");
}

/* ========================================================================== */
/*
 * Read a uchar from an I/O port.
 */
SCSIPORT_API
UCHAR
ScsiPortReadPortUchar(
	IN PUCHAR Port)
{
	return ((UCHAR)inb((int)Port));
}

/* ========================================================================== */
/*
 * Read a ushort from an I/O port.
 */
SCSIPORT_API
USHORT
ScsiPortReadPortUshort(
	IN PUSHORT Port)
{
	return ((USHORT)inw((int)Port));
}

/* ========================================================================== */
/*
 * Read a ulong from an I/O port.
 */
SCSIPORT_API
ULONG
ScsiPortReadPortUlong(
	IN PULONG Port)
{
	return ((ULONG)inl((int)Port));
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortReadPortBufferUchar(
	IN PUCHAR Port,
	IN PUCHAR Buffer,
	IN ULONG  Count)
{
	cmn_err(CE_PANIC, "ScsiPortReadPortBufferUchar: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortReadPortBufferUshort(
	IN PUSHORT Port,
	IN PUSHORT Buffer,
	IN ULONG Count)
{
	cmn_err(CE_PANIC, "ScsiPortReadPortBufferUshort: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortReadPortBufferUlong(
	IN PULONG Port,
	IN PULONG Buffer,
	IN ULONG Count)
{
	cmn_err(CE_PANIC, "ScsiPortReadPortBufferUlong: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
UCHAR
ScsiPortReadRegisterUchar(
	IN PUCHAR Register)
{
	cmn_err(CE_PANIC, "ScsiPortReadRegisterUchar: "
				"Should never be called!\n");
	return ((UCHAR) DDI_SUCCESS);
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
USHORT
ScsiPortReadRegisterUshort(
	IN PUSHORT Register)
{
	cmn_err(CE_PANIC, "ScsiPortReadRegisterUshort: "
				"Should never be called!\n");
	return ((USHORT) DDI_SUCCESS);
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
ULONG
ScsiPortReadRegisterUlong(
	IN PULONG Register)
{
	cmn_err(CE_PANIC, "ScsiPortReadRegisterUlong: "
				"Should never be called!\n");
	return ((ULONG) DDI_SUCCESS);
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortReadRegisterBufferUchar(
	IN PUCHAR Register,
	IN PUCHAR Buffer,
	IN ULONG  Count)
{
	cmn_err(CE_PANIC, "ScsiPortReadRegisterBufferUchar: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortReadRegisterBufferUshort(
	IN PUSHORT Register,
	IN PUSHORT Buffer,
	IN ULONG Count)
{
	cmn_err(CE_PANIC, "ScsiPortReadRegisterBufferUshort: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Implement a timed delay.
 * Used to implement the SCSI bus reset setting delay.
 */
SCSIPORT_API
VOID
ScsiPortStallExecution(
	IN ULONG Delay)
{
	int i = 0;


	drv_usecwait((clock_t) PCSCSI_BUSY_WAIT_USECS);
}

/* ========================================================================== */
/*
 * Write a uchar to an I/O port.
 */
SCSIPORT_API
VOID
ScsiPortWritePortUchar(
	IN PUCHAR Port,
	IN UCHAR Value)
{
	outb((int)Port, Value);
}

/* ========================================================================== */
/*
 * Write a ushort to an I/O port.
 */
SCSIPORT_API
VOID
ScsiPortWritePortUshort(
	IN PUSHORT Port,
	IN USHORT Value)
{
	outw((int)Port, Value);
}

/* ========================================================================== */
/*
 * Write a ulong to an I/O port.
 */
SCSIPORT_API
VOID
ScsiPortWritePortUlong(
	IN PULONG Port,
	IN ULONG Value)
{
	outl((int)Port, Value);
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortWritePortBufferUchar(
	IN PUCHAR Port,
	IN PUCHAR Buffer,
	IN ULONG  Count)
{
	cmn_err(CE_PANIC, "ScsiPortWritePortBufferUchar: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortWritePortBufferUshort(
	IN PUSHORT Port,
	IN PUSHORT Buffer,
	IN ULONG Count)
{
	cmn_err(CE_PANIC, "ScsiPortWritePortBufferUshort: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortWritePortBufferUlong(
	IN PULONG Port,
	IN PULONG Buffer,
	IN ULONG Count)
{
	cmn_err(CE_PANIC, "ScsiPortWritePortBufferUlong: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortWriteRegisterUchar(
	IN PUCHAR Register,
	IN UCHAR Value)
{
	cmn_err(CE_PANIC, "ScsiPortWriteRegisterUchar: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortWriteRegisterUshort(
	IN PUSHORT Register,
	IN USHORT Value)
{
	cmn_err(CE_PANIC, "ScsiPortWriteRegisterUshort: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API

VOID
ScsiPortWriteRegisterUlong(
	IN PULONG Register,
	IN ULONG Value)
{
	cmn_err(CE_PANIC, "ScsiPortWriteRegisterUlong: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortWriteRegisterBufferUchar(
	IN PUCHAR Register,
	IN PUCHAR Buffer,
	IN ULONG  Count)
{
	cmn_err(CE_PANIC, "ScsiPortWriteRegisterBufferUchar: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortWriteRegisterBufferUshort(
	IN PUSHORT Register,
	IN PUSHORT Buffer,
	IN ULONG Count)
{
	cmn_err(CE_PANIC, "ScsiPortWriteRegisterBufferUshort: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Never called under Solaris.
 */
SCSIPORT_API
VOID
ScsiPortWriteRegisterBufferUlong(
	IN PULONG Register,
	IN PULONG Buffer,
	IN ULONG Count)
{
	cmn_err(CE_PANIC, "ScsiPortWriteRegisterBufferUlong: "
				"Should never be called!\n");
}

/* ========================================================================== */
/*
 * Another Windows NT artifact.
 * Windows NT apparently considers a 'physical address' to be 64 bits.
 * This routine was copied from AMD's SCO driver.
 */
SCSIPORT_API
SCSI_PHYSICAL_ADDRESS
ScsiPortConvertUlongToPhysicalAddress(
	ULONG UlongAddress)
{
	SCSI_PHYSICAL_ADDRESS phyAdr;


	phyAdr.u.LowPart = (ULONG) UlongAddress;
	phyAdr.u.HighPart = 0x0000;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"ScsiPortConvertUlongToPhysicalAddress: UlongAddress	  %x\n",
			UlongAddress));
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"ScsiPortConvertUlongToPhysicalAddress: phyAdr.u.LowPart  %x\n",
			phyAdr.u.LowPart));
	pcscsi_debug(DBG_PORTABILITY, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"ScsiPortConvertUlongToPhysicalAddress: phyAdr.u.HighPart %x\n",
			phyAdr.u.HighPart));
#endif	/* PCSCSI_DEBUG	*/

	return (phyAdr);
}

/* ========================================================================== */
/*
 * This 'function' is defined by the following macro in srb.h:
 * #define ScsiPortConvertPhysicalAddressToUlong(Address) ((Address).u.LowPart)
 *
 *
 * SCSIPORT_API
 * ULONG
 * ScsiPortConvertPhysicalAddressToUlong(
 * SCSI_PHYSICAL_ADDRESS Address)
 *	{
 *	return ((ULONG) DDI_SUCCESS);
 *	}
 */

/* ========================================================================== */

SCSIPORT_API
VOID
ScsiDebugPrint(
	ULONG DebugPrintLevel,
	PCCHAR DebugMessage,
	...)
{


#ifdef PCSCSI_DEBUG
	if (pcscsig_debug_funcs & DBG_CORE)	{

		va_list argptr;
		UCHAR	arg;
		UCHAR	arg2;


		/*
		 * This is a kludge, as AMD didn't set up their AMDPrint routine
		 * properly.  We have no way of knowing what type the third arg
		 * will be.  So assume UCHAR, which should work for most cases.
		 * There's no way of knowing IF a third arg has been passed.
		 */
		va_start(argptr, DebugMessage);
		arg   = va_arg(argptr, UCHAR);
		arg2  = va_arg(argptr, UCHAR);
		va_end(argptr);

		prom_printf(DebugMessage, arg, arg2);

	}

#endif /* PCSCSI_DEBUG	*/

}
