/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)compaq.c	1.2	93/11/02 SMI"

/****************************************************************************************************************; */
/* Equates													; */
/*---------------------------------------------------------------------------------------------------------------; */
/*;;; COMPAQ Specific Definitions */
#define CPQ_READ_ENB		0x0
#define CPQ_WRITE_ENB		0x80
#define CPQ_DIR_PORT		0x65

/****************************************************************************************************************; */
/* Initialization Code												; */
/*---------------------------------------------------------------------------------------------------------------; */

char MSG_COMPAQ[]		= "COMPAQ Parallel Port";

/****************************************************************************************************************; */
/* Initialization Code												; */
/*---------------------------------------------------------------------------------------------------------------; */

/****************************************************************************************************************; */
/* Compaq Register Read												; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	bh = 8390 register number								; */
/*														; */
/*	Returns:	al = register value									; */
/*			ah, bh, dx destoyed, all other registers preserved						; */
/*														; */
/*	Description:	used to read a value from an 8390x register. tis is the register read routine		; */
/*			when the parallel port supports bidirectional transfers.				; */
/*---------------------------------------------------------------------------------------------------------------; */
PE2_CPQ_Get_Register(h,Reg)
int	Reg;
{
	int	i;

	OUTB(xcb[h].Media_IO_Address,0, Reg | SH_NIC_READ);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | SH_RDS);
	for (i = 0; i < 16; i++)
		if ( !(INB(xcb[h].Media_IO_Address,1) & SH_STB) ) break;
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Read_Ctrl | WBT_READ);
	OUTB(CPQ_DIR_PORT,0, CPQ_READ_ENB);
	i = (unsigned char)INB(xcb[h].Media_IO_Address,0);
	OUTB(CPQ_DIR_PORT,0, CPQ_WRITE_ENB);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return (i);
}

/****************************************************************************************************************; */
/* Compaq Remote DMA Block Read Routine										; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	es:di = address to read block into 							; */
/*			cx    = byte count of block to be read							; */
/*														; */
/*	Returns:	cx    = number of bytes not read (0 == success)						; */
/*			ax, bx, dx, si, di destroyed, all other registers preserved				; */
/*														; */
/*	Description:	reads cx bytes of data from the pocket ethernet adapter writing them to es:di,		; */
/*			waiting for prq	to be active and then creating an edge on the register data		; */
/*			strobe (RDS) line for each byte.  the remote dma read must have previously		; */
/*			been set up and initiated.  this is the block read routine used when the		; */
/*			parallel port supports bidirectional transfers.						; */
/*---------------------------------------------------------------------------------------------------------------; */
PE2_CPQ_Block_Read(h, Data, Count)
char far *Data;
int Count;
{

	unsigned char Image;

	OUTB(xcb[h].Media_IO_Address,0, 0xff);
	OUTB(CPQ_DIR_PORT,0, CPQ_READ_ENB);
	Image = (unsigned char)INB(xcb[h].Media_IO_Address,2);
	Image &= SH_DMASTB;
	Image |= WBT_READ | xcb[h].LPT_Read_Ctrl;
	OUTB(xcb[h].Media_IO_Address,2, Image);

	for ( ; Count; Count--, Data++) {
		*Data = INB(xcb[h].Media_IO_Address,0);
		Image ^= SH_DMASTB;
		OUTB(xcb[h].Media_IO_Address,2, Image);
	}
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | (Image & SH_DMASTB));
	OUTB(CPQ_DIR_PORT,0, CPQ_WRITE_ENB);
	return Count;
}

/****************************************************************************************************************; */
/* Whole Byte Transfer Remote DMA Memory Test Pattern Check Routine						; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	si = seed used for pattern generation							; */
/*			cx = byte count of block to be checked							; */
/*														; */
/*	Returns:	bp = number of mismatches (0 == success)						; */
/*			ax, bx, cx, dx, si, destroyed, all other registers preserved				; */
/*														; */
/*	Description:	reads cx bytes of data from the pocket ethernet adapter checking them against		; */
/*			the pattern that would have been created by Write Test Pattern.  an edge is		; */
/*			created on the register data strobe (RDS) line for each byte.				; */
/*			the remote dma read must have previously been set up and initiated. this is the		; */
/*			pattern check routine used when the parallel port supports bidirectional transfers.	; */
/*---------------------------------------------------------------------------------------------------------------; */
PE2_CPQ_Check_Test_Pattern(h, Seed, Count)
int	Seed, Count;
{
	int	Image, Errors = 0;


	OUTB(xcb[h].Media_IO_Address,0, 0xff);
	Image = xcb[h].LPT_Read_Ctrl | WBT_READ;

	OUTB(CPQ_DIR_PORT,0, CPQ_READ_ENB);
	OUTB(xcb[h].Media_IO_Address,2, Image);
	for ( ; Count; Count--, Seed++) {
		if ( (unsigned char)INB(xcb[h].Media_IO_Address,0) != (((Seed >> 8) & 0xff) ^ (Seed & 0xff)) )
			Errors++;
		Image ^= SH_DMASTB;
		OUTB(xcb[h].Media_IO_Address,2, Image);
	}
	OUTB(CPQ_DIR_PORT,0, CPQ_WRITE_ENB);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return (Errors);
}

PE2_CPQ_Register_Test(h)
{
	xcb[h].Put_Register		= PE2_SH_Put_Register;
	xcb[h].Get_Register		= PE2_CPQ_Get_Register;
	xcb[h].Block_Write		= PE2_SH_Block_Write;
	xcb[h].Block_Read		= PE2_CPQ_Block_Read;
	xcb[h].Adapter_Enable_Int	= PE2_SH_Enable_Int;
	xcb[h].Adapter_Disable_Int	= PE2_SH_Disable_Int;
	xcb[h].Adapter_Pulse_Int	= PE2_SH_Pulse_Int;
	xcb[h].Adapter_Reset_Ptr	= PE2_SH_Adapter_Reset;
	xcb[h].Adapter_Unhook_Ptr	= PE2_SH_Adapter_Unhook;

	xcb[h].EEPROM_Enable		= PE2_SH_Enable_EECS;
	xcb[h].EEPROM_Disable		= PE2_SH_Disable_EECS;
	xcb[h].EEPROM_Put_Bit		= PE2_SH_EEPROM_Put_Bit;
	xcb[h].EEPROM_Get_Bit		= PE2_SH_EEPROM_Get_Bit;

	xcb[h].Write_Test_Pattern	= PE2_SH_Write_Test_Pattern;
	xcb[h].Check_Test_Pattern	= PE2_CPQ_Check_Test_Pattern;

	xcb[h].Media_Configuration	= COMPAQ_MODE;
	xcb[h].Message_Ptr		= MSG_COMPAQ;

#ifdef SCPA
	scpa_access_method(xcb[h].cookie, SCPA_PE2_COMPAQ, 0, 0);
#endif

	xcb[h].Reset_Flags		= 0;
	Download_Bit_Stream(h, SH_Bit_Stream, sizeof(SH_Bit_Stream));
	xcb[h].LPT_Read_Ctrl |= 0x80;
	if (SH_PGA_Register_Test(h) == 0) return (0);
	xcb[h].LPT_Read_Ctrl &= ~0x80;
	return (-1);
}
