#ifndef lint
#pragma ident "@(#)pferror.h 1.4 96/06/08 SMI"
#endif

/*
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pferror.h
 * Group:	ttinstall
 * Description:
 */

#ifndef	_PFERROR_H_
#define	_PFERROR_H_

/* error codes... */

#define	LONGEST_MESSAGE 2048
typedef enum {
	pfOK = 0,

	/* Fatal */
	pfErUSAGE = 1,
	pfErMEMORY = 2,
	pfErPARSER,
	pfErUNMOUNT,

/*
 * Lethal meaning fatal if parsing filename specified on command line,
 * 	verses from a "Open file" popup, where we try to recover
 */
	pfErNOTOKEN = 1000,
	pfErNOTSUNW,
	pfErSWNAMELEN,
	pfErINVALIDSWTOKEN,
	pfErCUTPRESERVE,
	pfErOPEN,
	pfErNOVALUE,
	pfErINSTTYPE,
	pfErPROD,
	pfErPART,
	pfErNOSWAP,
	pfErDISKNAME,
	pfErUNKNOWNCOM,
	pfErFSARGCOUNT,
	pfErIPADDR,
	pfErNOROOTDISK,
	pfErNOKNOWNRESOURCES,
	pfErUSRLESS,
	pfErKVMLESS,
	pfErEXISTPRESERVE,
	pfErNOSIZE,
	pfErEXPLICITDEV,
	pfErMOUNT,
	pfErFILENOTFOUND,
	pfErNOMEDIA,
	pfErBADMEDIA,
	pfErMOUNTMEDIA,
	pfErPRODUCTMEDIA,
	pfErMEDIA,
	pfErLOADVTOCFILE,
	pfErFINDDISKS,
	pfErSYSTYPE,
	pfErMOUNTNOSLASH,
	pfErMOUNTSPACECHAR,
	pfErMOUNTMAXCHARS,
	pfErMBSIZECHARS,
	pfErMBSIZERANGE,

	/* Warnings */
	pfErDUPSSYST = 2000,
	pfErDUPCLUSTER,
	pfNOTSERVER,
	pfErUSRIPLESS,
	pfErKVMIPLESS,
	pfErIPLESS,
	pfErNFSPRESERVE,
	pfErNODISKSUSED,
	pfErZERODISK,
	pfErDUPMOUNT,
	pfErSLICESOVERLAP,
	pfErSLICEOFF,
	pfErBEGINCYL,
	pfErENDCYL,
	pfErCYLGAP,
	pfErNOTHINGTOPRESERVE,
	pfErRENAMEPRESERVE,
	pfErMODIFYPRESERVE,
	pfErNOUPGRADEDISK,
	pfErNOTUPGRADEABLE,
	pfErBADDISK,
	pfErNOSPACE,
	pfErNOFIT,
	pfErBOOTFIXED,
	pfErDISKIGNORE,
	pfErLOCKED,
	pfErINVALIDSTART,
	pfErZEROPRES,
	pfErUNUSEDSPACE,
	pfErCHANGED,
	pfErGEOMCHANGED,
	pfErREQUIREDFS,
	pfErUNSUPPORTEDFS,
	pfErNOTSELECTED,
	pfErBADMOUNTLIST,
	pfErORDER,
	pfErOUTREACH,
	pfErDISKOUTREACH,
	pfErLANG,
	pfErATTR,
	pfErNOROOT,
	pfErUNKNOWN,

	/* Messages */
	pfEOF
} pfErCode;

/* function prototypes */
void	pfWarn(pfErCode, char *, ...);	/* display a warning message */
char	*pfErMessage(pfErCode);		/* return error message for code */
void	pfErExitIfFatal(pfErCode);
void	pfErExitIfLethal(pfErCode); 	/* if filename on command line */
int	pfErIsFatal(pfErCode);
int	pfErIsLethal(pfErCode);

#endif	/* _PFERROR_H_ */
