/*
 * Copyright (c) 1995 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma ident   "@(#)ggmini_solaris.c 95/05/29 SMI"

/* ========================================================================== */
/*	Includes	*/

#include <sys/scsi/scsi.h>
#include <sys/dktp/hba.h>
#include <sys/pci.h>


/*	Includes specifically for the AMD core code	*/

#include "miniport.h"	/* Includes "srb.h"	*/
#include "scsi.h"
#include "ggmini.h"


/*	Driver-specific		*/

#include <sys/dktp/pcscsi/pcscsi_dma_impl.h>
#include <sys/dktp/pcscsi/pcscsi.h>


/* -------------------------------------------------------------------------- */
/*	Globals		*/

#ifdef PCSCSI_DEBUG
extern uint	pcscsig_debug_funcs;		/* Defined in pcscsi.c  */
extern uint	pcscsig_debug_gran;		/* Defined in pcscsi.c  */
extern char	pcscsig_dbgmsg[];		/* Defined in pcscsi.c  */
#endif /* PCSCSI_DEBUG */


/* ========================================================================== */
/*
 * This routine is a replacement for the AMD routine AMDAccessPCIRegister
 * (in ggmini.c).
 *
 * The AMD version of this function does things which are quite illegal
 * under Solaris (such as messing with the PCI bridge without any MT or MP
 * protection).  This replacement is equivalent in function and adheres to
 * the AMD API.
 *
 * It is prototyped in the AMD code (ggmini.c).
 *
 * From AMD's description:
 *
 *	Routine Description:
 *
 *	This routine accesses PCI configuration register.  Assume all the
 *	PCI device information (bus, device and function) are available.
 *
 *	Arguments:
 *
 *	DeviceExtension - HBA miniport driver's adapter data storage.
 *	PCIRegister - Register wanted to access.
 *	RegLength - Register length.
 *	DataPtr - Address used by data
 *	Operation - Read or write operation
 *
 *	Return Value:
 *
 *	None.  Note: Access should not cross double word register boundary.
 *
 */
void
AMDAccessPCIRegister(
	IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
	IN ULONG PCIRegister,	/* Offset from config base */
	IN ULONG RegLength,	/* OneByte, OneWord, OneDWord */
	IN PVOID DataPtr,	/* Data buffer address	*/
	IN ULONG Operation)	/* 'ReadPCI', 'WritePCI'  */
{
	ushort_t		i;
	ddi_acc_handle_t	handle;
	ULONG			offset;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_CORE, DBG_VERBOSE, sprintf(pcscsig_dbgmsg,
		"AMDAccessPCIRegister: DeviceExtension %x\n", DeviceExtension));
	pcscsi_debug(DBG_CORE, DBG_VERBOSE, sprintf(pcscsig_dbgmsg,
		"PCIRegister     %x\n", PCIRegister));
	pcscsi_debug(DBG_CORE, DBG_VERBOSE, sprintf(pcscsig_dbgmsg,
		"RegLength       %x\n", RegLength));
	pcscsi_debug(DBG_CORE, DBG_VERBOSE, sprintf(pcscsig_dbgmsg,
		"DataPtr         %x\n", DataPtr));
	pcscsi_debug(DBG_CORE, DBG_VERBOSE, sprintf(pcscsig_dbgmsg,
		"Operation       %x\n", Operation));
#endif  /* PCSCSI_DEBUG */


	handle = DeviceExtension->pcscsi_blk_p->pb_pci_config_handle;
	offset = PCIRegister;


	/*
	 * Disable interception of PCI config space SCSI scratch regs.
	 *
	 * The latest core code version contains a #define DISABLE_SREG
	 * which should prevent the core from constantly accessing PCI
	 * config space.
	 *
	 * Note that intercepting accesses to the PCI config space SCSI
	 * scratch registers here creates a problem: it *always*
	 * will bypass access to the SCSI scratch regs in PCI config space.
	 * There is one case in the core where a read of the physical SCSI
	 * scratch regs is actually needed:
	 * To determine if the chip is installed in a Compaq system.
	 * If so, the core needs to manually turn the 'disk activity' light
	 * on and off.
	 *
	 * The code is left here for ease of debugging future updates to the
	 * AMD core code.
	 *
	 * DISABLE_SREG is defined at compile time by Makefile.rules.
	 */
#ifndef DISABLE_SREG

	/*
	 * Accesses to the PCI config space 'scratch' registers are
	 * intercepted here, and translated to acceses to locations
	 * in the blk struct.
	 *
	 * See if the offset passed corresponds to one of the 8
	 * 16-bit 'SCSI scratch' registers.
	 */
	i = offset - PCSCSI_SCRATCH_REGS_BASE;	/* byte offset */
	i = i>>1;				/* byte->word (reg) offset */
	if (i > 8)	{ /* Then we're accessing one of them */


#ifdef PCSCSI_DEBUG
		pcscsi_debug(DBG_CORE, DBG_VERBOSE,
			"AMDAccessPCIRegister: PCI reg access\n");
#endif  /* PCSCSI_DEBUG */


		switch (Operation)  {
		case ReadPCI:

			switch (RegLength)  {
			case OneWord:
				*(ushort_t *)DataPtr = (ushort_t)
				DeviceExtension->
					pcscsi_blk_p->
						pb_scratch_regs[i];
				break;

			case OneByte:
			case OneDWord:
				cmn_err(CE_PANIC,
				"AMDAccessPCIRegister:"
				"Non-word read access to scratch reg");
				break;

			default:	/* Should *never* happen */
				cmn_err(CE_PANIC,
				"AMDAccessPCIRegister:"
				"Unrecognized read length: %d",
				RegLength);

			}	/* End switch (RegLength)	*/

			break;

		case WritePCI:

			switch (RegLength)  {
			case OneWord:
				DeviceExtension->
					pcscsi_blk_p->
						pb_scratch_regs[i]
						= *(ushort_t *)DataPtr;
				break;

			case OneByte:
			case OneDWord:
				cmn_err(CE_PANIC,
				"AMDAccessPCIRegister:"
				"Non-word write access to scratch reg");
				break;

			default:	/* Should *never* happen */
				cmn_err(CE_PANIC,
				"AMDAccessPCIRegister:"
				"Unrecognized write length: %d",
				RegLength);

			}	/* End switch (RegLength)	*/

			break;

		default:	/* Should *never* happen */
			cmn_err(CE_PANIC,
			"AMDAccessPCIRegister:"
			"Unrecognized operation code: %d",
			Operation);

		}	/* End switch (Operation)	*/


	}	/* End if offset=scratch reg...	*/




	else {		/* Not a SCSI scratch reg access */


#endif	/* DISABLE_SREG	*/



	/*
	 * This is a 'real' access to PCI config space, not just a
	 * 'scratch' reg.
	*/


#ifdef PCSCSI_DEBUG
		pcscsi_debug(DBG_CORE, DBG_VERBOSE,
			"AMDAccessPCIRegister: PCI reg access\n");
#endif  /* PCSCSI_DEBUG */


		switch (Operation)  {
		case ReadPCI:

			switch (RegLength)  {
			case OneByte:
				*(uchar_t *)DataPtr =
				    pci_config_getb(handle, (ulong_t) offset);
				break;

			case OneWord:
				*(ushort_t *)DataPtr =
				    pci_config_getw(handle, (ulong_t) offset);
				break;

			case OneDWord:
				*(ulong_t *)DataPtr =
				    pci_config_getl(handle, (ulong_t) offset);
				break;

			default:	/* Should *never* happen */
				cmn_err(CE_PANIC,
				"AMDAccessPCIRegister:"
				"Unrecognized read length: %d", RegLength);

			}	/* End switch (RegLength)	*/

			break;

		case WritePCI:

			switch (RegLength)  {
			case OneByte:
				pci_config_putb(handle,
						(ulong_t) offset,
						*(uchar_t *) DataPtr);
				break;

			case OneWord:
				pci_config_putw(handle,
						(ulong_t) offset,
						*(ushort_t *) DataPtr);
				break;

			case OneDWord:
				pci_config_putl(handle,
						(ulong_t) offset,
						*(ulong_t *) DataPtr);
				break;

			default:	/* Should *never* happen */
				cmn_err(CE_PANIC,
				"AMDAccessPCIRegister:"
				"Unrecognized write length", RegLength);

			}	/* End switch (RegLength)	*/

			break;

		default:	/* Should *never* happen */
			cmn_err(CE_PANIC,
			"AMDAccessPCIRegister: Unrecognized operation code: %d",
			Operation);

		}	/* End switch (Operation)	*/


#ifndef DISABLE_SREG
	}	/* end if (not a scratch reg)	*/
#endif	/* DISABLE_SREG	*/


}
