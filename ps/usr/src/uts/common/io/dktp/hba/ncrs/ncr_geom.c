/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)ncr_geom.c	1.2	95/12/15 SMI"

/*
 * NCR-specific geometry configuration routines
 */

#include <sys/dktp/ncrs/ncr.h>
#include <sys/pci.h>

#if defined(i386)
#include <sys/eisarom.h>
#include <sys/nvm.h>

extern int eisa_nvm();

/*
 * Set ncrp fields if this is a PCI Compaq implementation.
 * Check for Compaq implementation by looking for an EISA function with
 * 1) an EISA ID matching a supported PCI vendorid/deviceid
 * 2) a type/subtype string of CPQGEO;<subtype>
 * subtype is A, B or C to indicate the BIOS INT 13H
 * geometry type
 * A: always 64x32
 * B: 64x32 <1GB, 255x63 > 1GB
 * C: always 255x63
 */

/*
 * Compaq PCI systems have ESCD-style EISA function records in them;
 * a free-form board header structure followed by one or more
 * PCI board-ID structures.  This is not exactly the ESCD high-level
 * organization, but matches the low-level structure definitions.
 * This structure info is copied from the v1.02A of the ESCD spec
 * from Compaq/Intel/Phoenix.
 */

#define	BYTE unsigned char
#define	WORD unsigned short
#define	DWORD unsigned long

#pragma pack(1)
typedef struct {
	/*
	 * I refuse to call this "dword dSignature"; that's stupid and
	 * byte-order-dependent
	 */
	BYTE 	bSignature[4];			/* "ACFG" */
	BYTE 	bVerMinor;
	BYTE 	bVerMajor;
	BYTE 	bBrdType;
	BYTE 	bEcdHdrReserved1;
	WORD	fwECDFuncsDisabled;		/* bitmap */
	WORD	fwECDFuncsCfgError;		/* bitmap */
	WORD	fwECDFuncsCannotConfig;		/* bitmap */
	BYTE	abEcdHdrReserved[2];
} ECD_FREEFORMBRDHDR;

typedef struct {
	BYTE 	bBusNum;
	BYTE 	bDevFuncNum;
	WORD	wDeviceID;
	WORD 	wVendorID;
	BYTE	abPciBrdReserved[2];
} ECD_PCIBRDID;

struct compaq_pciboard {
	BYTE len;
	ECD_FREEFORMBRDHDR bh;
	ECD_PCIBRDID pcibid;
};

#pragma pack()

#define	ECD_BT_ISA	0x01
#define	ECD_BT_EISA	0x02
#define	ECD_BT_PCI	0x04
#define	ECD_BT_PCMCIA	0x08
#define	ECD_BT_PNPISA	0x10
#define	ECD_BT_MCA	0x20

/*ARGSUSED*/
static int
ncrcompaq_geometry(
			ncr_t			*ncrp,
			struct	scsi_address	*ap)
{
	char buf[sizeof (short) + sizeof (NVM_SLOTINFO) +
	    sizeof (NVM_FUNCINFO)];
	NVM_SLOTINFO *slotp;
	NVM_FUNCINFO *funcp;
	struct compaq_pciboard *cpqpcip;
	int slot, func, nfuncs;
	int foundpcislot;
	char *p;
	ddi_acc_handle_t handle;
	unsigned short	vendorid;
	unsigned short	deviceid;
	unsigned short	bus;
	unsigned short	devfunc;
	unsigned int	totalsectors;
	unsigned int	heads;
	unsigned int	sectors;
	unsigned int	bid;

	/*
	 * Query compaq EISA geometry information
	 */

	slotp = (NVM_SLOTINFO *)(buf + sizeof (short));
	funcp = (NVM_FUNCINFO *)(buf + sizeof (short) + sizeof (NVM_SLOTINFO));

	cpqpcip = (struct compaq_pciboard *)(funcp->un.freeform);

	if (ncrp->n_bustype == BUS_TYPE_EISA) {
		/* we know which slot to check */
		slot = ncrp->n_reg / 0x1000;
	} else {

		/*
		 * gather pci information
		 */
		if (pci_config_setup(ncrp->n_dip, &handle) != DDI_SUCCESS)
			return (-1);

		vendorid = pci_config_getw(handle, PCI_CONF_VENID);
		deviceid = pci_config_getw(handle, PCI_CONF_DEVID);
		bid = vendorid << 16 | deviceid;

		pci_config_teardown(&handle);

		/*
		 * COMPAQ EISA record has swapped vendorid/deviceid id
		 */
		bid = ((bid & 0xFF) << 24) | 
	    		((bid & 0xFF00) << 8) | 
	    		((bid & 0xFF0000) >> 8) | 
	    		((bid & 0xFF000000) >> 24);

#ifdef PCI_DDI_EMULATION
		/* XXX - code should be common; define xpci macros for 2.4? */
		bus = (*ncrp->n_regp >> 8) & 0xFF;
		devfunc = *ncrp->n_regp & 0xFF;
#else
		bus = PCI_REG_BUS_G(*ncrp->n_regp);
		devfunc = PCI_REG_DEV_G(*ncrp->n_regp) << 3 |
				PCI_REG_FUNC_G(*ncrp->n_regp);
#endif

		/* PCI, must search all slots for a match */
		foundpcislot = 0;

		/* scan all slots looking for match in ACFG record */
		for (slot = 0; slot < EISA_MAXSLOT; slot++) {

			/*
			 * Search funcs for match with bus/dev/fn.
			 * Board id will match vendorid << 16 | devicide
			 */
			for (func = 0; ; func++) {
				if (eisa_nvm((char *)buf,
				    EISA_SLOT | EISA_CFUNCTION | EISA_BOARD_ID,
				    slot, func, bid, 0xffffffff) == 0)
					break;
				/*
				 * Must have freeform data, with the right sig,
				 * with board type PCI, with our bus/dev/fn
				 * to have a match
				 */
				if ((funcp->fib.data == 1) &&
				    (bcmp((char *)cpqpcip->bh.bSignature,
					"ACFG", 4) == 0) &&
				    (cpqpcip->bh.bBrdType == ECD_BT_PCI) &&
				    (cpqpcip->pcibid.bBusNum == bus) &&
				    (cpqpcip->pcibid.bDevFuncNum == devfunc)) {
					foundpcislot = 1;
					ncrp->n_compaq = 1;
					break;
				}
			}
			/* if we've got it, don't try any more slots */
			if (foundpcislot)
				break;
		}
	}

	/* now find the geometry type information, both EISA and PCI */

	for (func = 0; slot < EISA_MAXSLOT; func++) {
		if (eisa_nvm(buf, EISA_SLOT|EISA_CFUNCTION,
		    slot, func) == 0)
			break;
		/* do we have a type, and is it CPQGEO? */
		if ((funcp->fib.type == 0) ||
		    (strncmp((char *)funcp->type, "CPQGEO", 6) != 0))
			continue;

		/* it must be a Compaq if we have a geometry type string */
		ncrp->n_compaq = 1;

		/* type string is "CPQGEO;<x>", <x> is geomtype */
		ncrp->n_geomtype = 'U';
		p = strchr((char *)funcp->type, ';');
		if (p == NULL) {
cmn_err(CE_CONT, "?ncr_geometry: malformed CPQGEO type string: %s",
				funcp->type);
			break;
		}

		ncrp->n_geomtype = *(++p);
	}

	/*
	 * finally handle compaq cases settable from EISA information
	 */
	if (ncrp->n_compaq) {
		totalsectors = ADDR2NCRUNITP(ap)->nt_total_sectors;
		switch (ncrp->n_geomtype) {

		case 'A':
			heads = 64;
			sectors = 32;
			break;

		case 'C':
			heads = 255;
			sectors = 63;
			break;

		default:
cmn_err(CE_CONT, "?ncr_geometry: invalid geometry type %d, assuming type B\n",
				ncrp->n_geomtype);

		/*FALLTHROUGH*/
		case 'B':
			if (totalsectors <= 1024 * 64 * 32) {
				heads = 64;
				sectors = 32;
			} else {
				heads = 255;
				sectors = 63;
			}
			break;
		}
cmn_err(CE_CONT, "?ncr(%d,%d): ioaddr=0x%x, sectors=%d heads=%d sectors=%d\n",
			ap->a_target, ap->a_lun, ncrp->n_reg, totalsectors,
			heads, sectors);
		return (HBA_SETGEOM(heads, sectors));
	}

	/*
	 * otherwise fallback to generic geometry routines
	 */
	return (-1);
}
#endif	/* defined(i386) */

/*ARGSUSED*/
int
ncr_geometry(
			ncr_t			*ncrp,
			struct	scsi_address	*ap)
{
	int status;

#if defined(i386)
	if ((status = ncrcompaq_geometry(ncrp, ap)) != -1)
		return (status);
#endif	/* defined(i386) */

	return (NCR_GEOMETRY(ncrp, ap));
}
