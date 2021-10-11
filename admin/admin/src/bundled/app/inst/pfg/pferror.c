#ifndef lint
#pragma ident "@(#)pferror.c 1.42 96/07/26 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pferror.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include <stdarg.h>

void
pfWarn(pfErCode code, char *format, ...)
{
	va_list ap;
	char buf[LONGEST_MESSAGE];

	va_start(ap, format);
	(void) vsprintf(buf, format, ap);
	pfAppWarn(code, buf);
}

int
pfErIsFatal(pfErCode code)
{
	if (code == 0)
		return (0);
	else
		return (!(code/1000));
}

int
pfErIsLethal(pfErCode code)  /* used if filename specified on comline */
{
	if (code == 0)
		return (0);
	else
		return (code < 2000);
}

void
pfErExitIfFatal(pfErCode code)
{
	if (pfErIsFatal(code))
		pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
}

void
pfErExitIfLethal(pfErCode code)
{
	if (pfErIsLethal(code))
		pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
}

char *
pfErMessage(pfErCode code)
{
	write_debug(GUI_DEBUG_L1, "pfErMessage %d", code);

	switch (code) {
	case pfErMODIFYPRESERVE:
		return (PFG_ER_MODIFY);
	case pfErCUTPRESERVE:
		return (PFG_ER_CUT);
	case pfErNOTSUNW:
		return (PFG_ER_SUNW);
	case pfErSWNAMELEN:
		return (PFG_ER_ARGLONG);
	case pfErINVALIDSWTOKEN:
		return (PFG_ER_ARG2);
	case pfErNOTHINGTOPRESERVE:
		return (PFG_ER_NOFS);
	case pfErZEROPRES:
		return (PFG_ER_ZERO);

	case pfErPARSER:
		return (PFG_ER_PARSER);
	case pfErOPEN:
		return (PFG_ER_CANTOPEN);
	case pfErNOTOKEN:
		return (PFG_ER_NOTOKEN);

	case pfErSYSTYPE:
		return (PFG_ER_INVALID);

	case pfErZERODISK:
		return (PFG_ER_SLICE);
	case pfErDUPMOUNT:
		return (PFG_ER_DUPMOUNT);
	case pfErSLICESOVERLAP:
		return (PFG_ER_OVERLAP);
	case pfErSLICEOFF:
		return (PFG_ER_EXTENDS);
	case pfErBEGINCYL:
		if (disk_fdisk_req(first_disk()))
			return (PFG_ER_CYLZERO);
		else
			return (PFG_ER_BEGIN);
	case pfErENDCYL:
		return (PFG_ER_END);
	case pfErCYLGAP:
		return (PFG_ER_SPACE);
	case pfErNODISKSUSED:
		return (PFG_ER_NODISKS);
	case pfErLOADVTOCFILE:
		return (PFG_ER_VTOC);
	case pfErFINDDISKS:
		return (PFG_ER_FINDDSKS);
	case pfErNOMEDIA:
		return (PFG_ER_NOMEDIA);
	case pfErBADMEDIA:
		return (PFG_ER_BADMEDIA);
	case pfErMOUNTMEDIA:
		return (PFG_ER_NOTMOUNT);
	case pfErPRODUCTMEDIA:
		return (PFG_ER_NOPROD);
	case pfErMEDIA:
		return (PFG_ER_LOADMED);
	case pfErNOVALUE:
		return (PFG_ER_NOVALUE);
	case pfErINSTTYPE:
		return (PFG_ER_INSTALL);
	case pfErDUPSSYST:
		return (PFG_ER_DUPSYS);
	case pfErPROD:
		return (PFG_ER_INVPROD);
	case pfErPART:
		return (PFG_ER_INVPART);
	case pfErMEMORY:
		return (PFG_ER_NOMEM);
	case pfErNOSWAP:
		return (PFG_ER_NOSWAP);
	case pfErDUPCLUSTER:
		return (PFG_ER_DUPCONF);
	case pfErDISKNAME:
		return (PFG_ER_INVALDSK);
	case pfErUNKNOWNCOM:
		return (PFG_ER_UNKCOMM);
	case pfNOTSERVER:
		return (PFG_ER_NOTSRVR);
	case pfErFSARGCOUNT:
		return (PFG_ER_MISSPARM);
	case pfErIPADDR:
		return (PFG_ER_IPADDR);
	case pfErNOROOTDISK:
		return (PFG_ER_ROOTDISK);
	case pfErUSRIPLESS:
		return (PFG_ER_MISSIP);
	case pfErNFSPRESERVE:
		return (PFG_ER_NFS);
	case pfErEXISTPRESERVE:
		return (PFG_ER_EXISTING);
	case pfErRENAMEPRESERVE:
		return (PFG_ER_CANTPRE);
	case pfErNOSIZE:
		return (PFG_ER_NOSIZE);
	case pfErNOSPACE:
		return (PFG_ER_NOSPACE);
	case pfErNOFIT:
		return (PFG_ER_NOFIT);
	case pfErLOCKED:
		return (PFG_ER_MODLOCK);
	case pfErBOOTFIXED:
		return (PFG_ER_NONBOOT);
	case pfErDISKIGNORE:
		return (PFG_ER_IGNORE);
	case pfErEXPLICITDEV:
		return (PFG_ER_EXPLICIT);
	case pfErMOUNT:
		return (PFG_ER_INVALMNT);
	case pfErMOUNTNOSLASH:
		return (PFG_ER_MNTPNT);
	case pfErMOUNTSPACECHAR:
		return (PFG_ER_MNTSPACE);
	case pfErMOUNTMAXCHARS:
		return (PFG_ER_MNTLONG);
	case pfErMBSIZECHARS:
		return (PFG_ER_SIZE);
	case pfErMBSIZERANGE:
		return (PFG_ER_RANGE);
	case pfErNOUPGRADEDISK:
		return (APP_ER_NOUPDSK);
	case pfErNOTSELECTED:
		return (PFG_ER_NOTSEL);
	case pfErBADDISK:
		return (PFG_ER_BADDISK);
	case pfErINVALIDSTART:
		return (PFG_ER_INVALCYL);
	case pfErUNUSEDSPACE:
		return (PFG_ER_UNALLOC);
	case pfErCHANGED:
		return (PFG_ER_CHANGED);
	case pfErGEOMCHANGED:
		return (PFG_ER_GEOMCHG);
	case pfErREQUIREDFS:
		return (PFG_ER_RQDFS);
	case pfErUNSUPPORTEDFS:
		return (PFG_ER_UNSUPFS);
	case pfErBADMOUNTLIST:
		return (PFG_ER_BADMNT);
	case pfErORDER:
		return (PFG_ER_ORDER);
	case pfErDISKOUTREACH:
		return (PFG_ER_DISKOUTREACH);
	case pfErOUTREACH:
		return (PFG_ER_OUTREACH);
	case pfErLANG:
		return (PFG_ER_LANG);
	case pfErATTR:
		return (PFG_ER_ATTR);
	case pfErUNMOUNT:
		return (APP_ER_UNMOUNT);
	case pfErNOROOT:
		return (PFG_ER_BOOTDISK);
	case pfErNOKNOWNDISKS:
		return (APP_ER_NOKNOWNDISKS);
	case pfErNOKNOWNRESOURCES:
		return (APP_ER_NOKNOWNRESOURCES);
	case pfErNOUSABLEDISKS:
		return (APP_ER_NOUSABLEDISKS);
	case pfErUNKNOWN:
		return (PFG_ER_UNKNOWN);
	default:
		return ("");
	}
}
