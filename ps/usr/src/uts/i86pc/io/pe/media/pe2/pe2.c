/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)pe2.c	1.3	94/04/06 SMI"

/* pe2.inc --- Pocket Ethernet Adapter II Adapter Control Module (c) 1992 Xircom */

/****************************************************************************************************************; */
/* Revision Information												; */
/*														; */
/*	Author: 	Eric Henderson										; */
/*	Started:	Jan 1991										; */
/*	Code From:	Pocket Ethernet Adapter Driver (Dirk Gates, 6/14/89)					; */
/*														; */
/*	Language:	MASM 5.1										; */
/*	Build:		MASM pe2;										; */
/*														; */
/*---------------------------------------------------------------------------------------------------------------; */
/* Release 	Date	Who		Why									; */
/* -------	----	---		--------------------------------------------------------		; */
/*														; */
/*---------------------------------------------------------------------------------------------------------------; */
/* Notes:													; */
/*														; */
/*---------------------------------------------------------------------------------------------------------------; */
/* History:													; */
/*														; */
/* Date	 Who    What												; */
/* ----	 ---	-------------------------------------------------------------------------------------------	; */
/*														; */
/*---------------------------------------------------------------------------------------------------------------; */

#include "config.h"
#ifdef SCPA
#include "scpa.h"
#endif
#include "xcb.h"
#include "xms.h"

/****************************************************************************************************************; */
/* Imports From Parallel Port specific Hardware Modules								; */
/*---------------------------------------------------------------------------------------------------------------; */

void	Download_Bit_Stream();

int	Enable_EPP_Mode(),
	Disable_EPP_Mode(),
	Programmable_Delay();

#ifdef notdef
extern	IRQ_Ctrl_Polarity,
	LPT_Write_Ctrl,
	LPT_Read_Ctrl,
	LPT_Port_Number;
#endif

/****************************************************************************************************************; */
/* Equates													; */
/*---------------------------------------------------------------------------------------------------------------; */
#define ADAPTER_TYPE_NUMBER	PE2

#define ADAPTER_FULLNAME	"Pocket Ethernet Adapter II"
#define ADAPTER_NAME

#define EE_FLAGS_LI		2					/* bit 0 of flags = link integrity */
#define EE_FLAGS_SLEEP		4					/* bit 2 of flags = sleep enable */

#define XIRCOM_ADDRESS_1	0x00
#define XIRCOM_ADDRESS_2	0x80
#define XIRCOM_ADDRESS_3	0xc7

#define CR			0xd
#define LF			0xa


/****************************************************************************************************************; */
/* Structure Definitions												; */
/*---------------------------------------------------------------------------------------------------------------; */
#ifdef notdef
/*// Configuration EEPROM Structure */
struct EEPROM_Register_Map {
char		EE_Model_Number[12];	/* model number, asciiz string */
long		EE_Serial_Number;	/* serial number, 32 bit long */
char		EE_Manufacture_Date[7];	/* manufacture date (w/o hsecs) */
char		EE_Flags;		/* miscellaneous flags */
unsigned char	EE_Network_Address[6];	/* 48 bit network address */
unsigned char	EE_Check_Sum;		/* 16 bit check sum */
};



/***************************************************************************************************************; */
/* Initialization Data Definitions										; */
/*--------------------------------------------------------------------------------------------------------------; */
int   (*Write_Test_Pattern)();
int   (*Check_Test_Pattern)();
void  (*EEPROM_Disable)();
void  (*EEPROM_Enable)();
void  (*EEPROM_Put_Bit)();
int   (*EEPROM_Get_Bit)();

/****************************************************************************************************************; */
/* Resident Data Definitions											; */
/*---------------------------------------------------------------------------------------------------------------; */

int	Media_Configuration	=	-1,
	Media_Memory_Address	=	-1,
	Media_IRQ_Number	=	 7,			/* Default to 7 until we fix Find_IRQ_Number */
	Media_IO_Address	=	-1;

struct	EEPROM_Register_Map PX_EEPROM_Buffer;
#endif

/****************************************************************************************************************; */
/* Resident Code Section												; */
/*---------------------------------------------------------------------------------------------------------------; */

/****************************************************************************************************************; */
/* Network Interface Controller Register Access Routines								; */
/*---------------------------------------------------------------------------------------------------------------; */

#ifdef notdef
void (*Put_Register)();
int  (*Get_Register)();

/****************************************************************************************************************; */
/* Low Level Block Read and Write Routines									; */
/****************************************************************************************************************; */

void (*Block_Write)();
void (*Block_Read)();

/****************************************************************************************************************; */
/* Control Routines												; */
/****************************************************************************************************************; */

#endif

static void Adapter_Reset(h)
{
	if (xcb[h].Adapter_Reset_Ptr)
		xcb[h].Adapter_Reset_Ptr(h, xcb[h].PE2_PX_EEPROM_Buffer.EE_Flags);
}

static void Adapter_Unhook(h)
{
	xcb[h].PE2_PX_EEPROM_Buffer.EE_Flags &= EE_FLAGS_SLEEP;
	Adapter_Reset(h);
	Hardware_Unhook(h);
	if (xcb[h].Adapter_Unhook_Ptr)
		xcb[h].Adapter_Unhook_Ptr(h);
}

/****************************************************************************************************************; */
/* Initialization Data Section											; */
/*---------------------------------------------------------------------------------------------------------------; */
char	ModeKeyWord[]			= "NON";
char	LPTKeyWord[]			= "LPT";

void	Process_Mode_Keyword();
void	Process_LPT_Keyword();

int	Media_Keywords			= 2;
char	*Media_Keyword_Text[2]		= { ModeKeyWord, LPTKeyWord };
int	Media_Keyword_Length[2] 	= { sizeof(ModeKeyWord)-1, sizeof(LPTKeyWord)-1 };
void	(*Media_Keyword_Function[2])()	= { Process_Mode_Keyword, Process_LPT_Keyword };

/*// Local Storage */
char	MSG_EEPROM_Failed[]		= ADAPTER_NAME " Address EEPROM unreadable";
char	MSG_Bad_Mode[]			= "The selected hardware configuration is not supported";
char	MSG_Wrong_Adapter[]		= "This driver does not support the currently connected adapter";
char	MSG_Bad_LPT_Keyword[]		= "The LPT keyword must specify 1, 2, or 3, value ignored";
char	MSG_Unrecognized[]		= "ERROR: Unrecognized command line option: ";
char	MSG_Keyword[25];

char	MSG_EPP_Mode[]			= ADAPTER_NAME " running in EPP mode";
char	MSG_EWrite_Mode[]		= ADAPTER_NAME " running in Enhanced Write mode";
char	MSG_Bidir_Mode[]		= ADAPTER_NAME " running in Bidirectional mode";
char	MSG_Non_Bidir_Mode[]		= ADAPTER_NAME " running in Non-Bidirectional mode";
char	MSG_COMPAQ_Mode[]		= ADAPTER_NAME " running in Compaq Bidirectional mode";
char	MSG_PPX_Loaded[]		= "Parallel Port Multiplexor support loaded";

#ifdef notdef
char   *Message_Ptr			= MSG_Bad_Mode;			/* sanity default */
#endif

char	MSG_Configuration[80]		= "LPT1:, No IRQ, ";
char	MSG_IRQ[]			= "IRQ7, ";
char	MSG_PPX[]			= ", PPX";

int	Not_Implemented(), PE2_DQNT_Register_Test(),
	PE2_WBT_Register_Test(), PE2_HH_Register_Test(),
	PE2_PS2_Register_Test();

int   (*PE2_Setup_Table[8])()		= {
	PE2_DQNT_Register_Test,
	PE2_WBT_Register_Test,
	PE2_PS2_Register_Test,
	PE2_HH_Register_Test,
	Not_Implemented,
	Not_Implemented,
	Not_Implemented,
	Not_Implemented
};

/****************************************************************************************************************; */
/* Adapter Initialize												; */
/*														; */
/* Type:		Near												; */
/*														; */
/* Assumes:	DS  = DATA_GROUP										; */
/*		INT = disabled											; */
/*														; */
/* Returns:	AX  = offset in DATA_GROUP of Error string if CX > 0						; */
/*               CX  = error code:										; */
/*			0 -> initialization successful								; */
/*		     1-16 -> Hardware module return codes							; */
/*		       17 -> EEPROM unreadable									; */
/*		       18 -> Selected configuration unavailable							; */
/*		       19 -> Wrong Adapter									; */
/*		INT = disabled											; */
/* Preserves:	ES, DS, BP											; */
/*														; */
/* Description:	Called from the Media Control Module to initialize the adapter. This module			; */
/*		calls Hardware_Initialize and is then responsible for moving the correct procedures		; */
/*		into executable memory based on the hardware configuration. The EEPROM is also read		; */
/*		in at this time.										; */
/*---------------------------------------------------------------------------------------------------------------; */
static PE2_Adapter_Initialize(h, Err_String)
char   **Err_String;
{
	int	Adapter, Value;

	if ( (Value = Hardware_Initialize(h, &Adapter, Err_String)) != 0) return (Value);
	if ( Adapter != ADAPTER_TYPE_NUMBER) {
		*Err_String = MSG_Wrong_Adapter;
		return (ERR_WRONG_ADAPTER);
	}
	if (Setup_Configuration(h, xcb[h].Hardware_Configuration & xcb[h].Media_Configuration) != 0) {
		*Err_String = MSG_Bad_Mode;
		return (ERR_SELECTED_CONFIGURATION);
	}
	if (Setup_Configuration(h, xcb[h].Hardware_Configuration & xcb[h].Media_Configuration) != 0) {
		*Err_String = MSG_Bad_Mode;
		return (ERR_SELECTED_CONFIGURATION);
	}
	if (Read_PX_EEPROM(h) != 0) {
		*Err_String = MSG_EEPROM_Failed;
		return (ERR_EEPROM_UNREADABLE);
	}
	return(0);
}

/****************************************************************************************************************; */
/* Media Config Text												; */
/*														; */
/* Type:		Near												; */
/*														; */
/* Assumes:	DS  = DATA_GROUP alias										; */
/*														; */
/* Returns:	AX  = offset in DATA_GROUP of ASCII Configuration string short form (LPT1:, IRQ7, Bidirectional); */
/*		DX  = offset in DATA_GROUP of ASCII Mode string							; */
/*		BX  = offset in DATA_GROUP of ASCII Mode string continued					; */
/*		    = 0 if no continuation									; */
/*														; */
/* Description:	Sets up the configuration strings based on the media variables.					;												; */
/*---------------------------------------------------------------------------------------------------------------; */
static void Media_Config_Text(h, Config, Mode, Mode_Continued)
char	**Config, **Mode, **Mode_Continued;
{
	char	*p, *q;

	MSG_Configuration[3] = xcb[h].LPT_Port_Number + '1';
	if (xcb[h].Media_IRQ_Number) {
		MSG_IRQ[3] = xcb[h].Media_IRQ_Number + '0';
		for ( p = MSG_IRQ, q = MSG_Configuration+7; *p; p++, q++)
			*q = *p;
	} else q = MSG_Configuration + 15;
	for (p = xcb[h].Message_Ptr; *p; q++, p++) *q = *p;
	if (xcb[h].Hardware_Configuration & PPX_HARDWARE) {
		for (p = MSG_PPX; *p; q++, p++) *q = *p;
		xcb[h].Media_Configuration |= PPX_HARDWARE;
		*Mode_Continued = MSG_PPX_Loaded;
	}
	*q = '\0';
	*Config = MSG_Configuration;
	if (xcb[h].Media_Configuration & EPP_MODE)		  *Mode = MSG_EPP_Mode;
	if (xcb[h].Media_Configuration & EWRITE_MODE)		  *Mode = MSG_EWrite_Mode;
	if (xcb[h].Media_Configuration & COMPAQ_MODE)		  *Mode = MSG_COMPAQ_Mode;
	if (xcb[h].Media_Configuration & BIDIRECTIONAL_MODE)	  *Mode = MSG_Bidir_Mode;
	if (xcb[h].Media_Configuration & NON_BIDIRECTIONAL_MODE) *Mode = MSG_Non_Bidir_Mode;
}

static Setup_Configuration(h, Mode)
int Mode;
{
	int	i;

	for (i = 7; i >= 0; i--)
		if ((Mode >> i) & 1)
			if (PE2_Setup_Table[i](h, Mode) == 0)
				return (0);
	return (ERR_SELECTED_CONFIGURATION);
}

static Not_Implemented(h)
{
	return (-1);
}

/****************************************************************************************************************; */
/* Process Keyword Functions											; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	interrupts disabled, remain disabled							; */
/*			AX contains value if SI == 0								; */
/*			SI  = 0 if keyword is to be set								; */
/*			SI -> ASCII value to parse */
/*														; */
/*	Returns:	DX  = 0 if success									; */
/*			DX -> Error message if failure								; */
/*			BX  = number of	bytes used from SI if SI != 0						; */
/*														; */
/*	Preserves:	CX											; */
/*														; */
/*	Description:	These routines are vectored into from the Media_Keyword_Function Table. If		; */
/*			the link layer determines that a keyword match has been found, it calls these		; */
/*			routines to set the appropriate values. If the link layer parses the strings, it 	; */
/*			sets SI to 0 and passes the numeric value in AX, otherwise these functions return	; */
/*			the number of bytes parsed from SI							; */
/*---------------------------------------------------------------------------------------------------------------; */
static void Process_Mode_Keyword(h)
{
	xcb[h].Media_Configuration	= NON_BIDIRECTIONAL_MODE;
}

static void Process_LPT_Keyword(h)
{	/* Needs some thought */
}

/****************************************************************************************************************; */
/* Read Pocket LAN EEPROM											; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	es:di = address to store EEPROM data (must be at least a 32 byte buffer)		; */
/*														; */
/*	Returns:	cx = checksum of EEPROM (0 == success)							; */
/*			interrupts enabled									; */
/*			registers preserved: ds, es, bp, ss, sp							; */
/*			registers destroyed: ax, bx, cx, dx, si, di, flags (ODISZAPC)				; */
/*														; */
/*	Description: 	block reads the address EEPROM into the EEPROM buffer passed in es:di.			; */
/*---------------------------------------------------------------------------------------------------------------; */
Read_PX_EEPROM(h)
{
	if (Read_EEPROM(h, (char *) &xcb[h].PE2_PX_EEPROM_Buffer) != 0) return (-1);
	if (xcb[h].PE2_PX_EEPROM_Buffer.EE_Network_Address[0] != XIRCOM_ADDRESS_1) return (-1);
	if (xcb[h].PE2_PX_EEPROM_Buffer.EE_Network_Address[1] != XIRCOM_ADDRESS_2) return (-1);
	if (xcb[h].PE2_PX_EEPROM_Buffer.EE_Network_Address[2] != XIRCOM_ADDRESS_3) return (-1);
	return (0);
}



#include "sh.c"
#include "dqnt.c"
#include "wbt.c"
#include "ps2.c"
#include "epp.c"
#include "compaq.c"

#include "../8390/eth.c"
#include "../eeprom.c"
