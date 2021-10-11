/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)wbt.c	1.2	93/11/02 SMI"

/***************************************************************************************************************; */
/* Equates													; */
/*--------------------------------------------------------------------------------------------------------------; */
/*;;; Control Port Bit Definitions */
#define WBT_READ	0x08

/***************************************************************************************************************; */
/* Initialization Data												; */
/*--------------------------------------------------------------------------------------------------------------; */
char MSG_Bidirectional[]	= "Bidirectional";

/***************************************************************************************************************; */
/* Initialization Code												; */
/*--------------------------------------------------------------------------------------------------------------; */

/***************************************************************************************************************; */
/* Whole Byte Transfer Register Read										; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	bh = 8390 register number								; */
/*														; */
/*	Returns:	al = register value									; */
/*			ah, dx destroyed, all other registers preserved						; */
/*														; */
/*	Description:	used to read a value from an 8390 register. this is the register read routine		; */
/*			when the parallel port supports bidirectional transfers.				; */
/*--------------------------------------------------------------------------------------------------------------; */
PE2_WBT_Get_Register(h, Reg)
int	Reg;
{
	int	i;

	OUTB(xcb[h].Media_IO_Address,0, Reg | SH_NIC_READ);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | SH_RDS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | SH_RDS | SH_NICSTB);
	for (i = 0; i < 16; i++)
		if ( !(INB(xcb[h].Media_IO_Address,1) & SH_STB) ) break;

	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Read_Ctrl | WBT_READ);
	OUTB(xcb[h].Media_IO_Address,0, 0xff);					/* For Dell's and others with AT style ports */
	i = (unsigned char)INB(xcb[h].Media_IO_Address,0);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return (i);
}

/***************************************************************************************************************; */
/* Whole Byte Transfer Remote DMA Block Read Routine								; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	ES:DI	address to read block into 							; */
/*			CX	byte count of block to be read							; */
/*			DS	DATA_GROUP									; */
/*														; */
/*	Returns:	CX	number of bytes not read (0 == success)						; */
/*			AL BL DX DI	destroyed, all other registers preserved				; */
/*														; */
/*	Description:	reads cx bytes of data from the pocket ethernet adapter writing them to es:di,		; */
/*			strobe (RDS) line for each byte.  the remote dma read must have previously		; */
/*			been set up and initiated.  this is the block read routine used when the		; */
/*			parallel port supports bidirectional transfers.						; */
/*--------------------------------------------------------------------------------------------------------------; */
PE2_WBT_Block_Read(h, Data, Count)
char far *Data;
int Count;
{
	unsigned char Image;

	OUTB(xcb[h].Media_IO_Address,0, 0xff);
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
	return Count;
}

/***************************************************************************************************************; */
/* Whole Byte Transfer Remote DMA Memory Test Pattern Check Routine						; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	SI	seed used for pattern generation						; */
/*			CX	byte count of block to be checked						; */
/*														; */
/*	Returns:	BP	number of mismatches (0 == success)						; */
/*			ax, bx, cx, dx, si, destroyed, all other registers preserved				; */
/*														; */
/*	Description:	reads cx bytes of data from the pocket ethernet adapter checking them against		; */
/*			the pattern that would have been created by Write Test Pattern.  an edge is		; */
/*			created on the register data strobe (RDS) line for each byte.				; */
/*			the remote dma read must have previously been set up and initiated. this is the		; */
/*			pattern check routine used when the parallel port supports bidirectional transfers.	; */
/*--------------------------------------------------------------------------------------------------------------; */
PE2_WBT_Check_Test_Pattern(h, Seed, Count)
int	Seed, Count;
{
	int	Image, Errors = 0;

	OUTB(xcb[h].Media_IO_Address,0, 0xff);
	Image = xcb[h].LPT_Read_Ctrl | WBT_READ;
	OUTB(xcb[h].Media_IO_Address,2, Image);
	for ( ; Count; Count--, Seed++) {
		if ( (unsigned char)INB(xcb[h].Media_IO_Address,0) != (((Seed >> 8) & 0xff) ^ (Seed & 0xff)) )
			Errors++;
		Image ^= SH_DMASTB;
		OUTB(xcb[h].Media_IO_Address,2, Image);
	}
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return (Errors);
}

PE2_WBT_Register_Test(h)
{
	if (xcb[h].Hardware_Configuration & COMPAQ_MODE)
		if (PE2_CPQ_Register_Test(h) == 0) return (0);

	xcb[h].Put_Register		= PE2_SH_Put_Register;
	xcb[h].Get_Register		= PE2_WBT_Get_Register;
	xcb[h].Block_Write		= PE2_SH_Block_Write;
	xcb[h].Block_Read		= PE2_WBT_Block_Read;
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
	xcb[h].Check_Test_Pattern	= PE2_WBT_Check_Test_Pattern;

	xcb[h].Media_Configuration	= BIDIRECTIONAL_MODE;
	xcb[h].Message_Ptr		= MSG_Bidirectional;

#ifdef SCPA
	scpa_access_method(xcb[h].cookie, SCPA_PE2_WBT, 0, 0);
#endif

	xcb[h].Reset_Flags		= 0;
	Download_Bit_Stream(h, SH_Bit_Stream, sizeof(SH_Bit_Stream));
	Set_PGA_Ctrl_Reg(h, PGA_REG2+SH_NIC_RST);
	if (SH_PGA_Register_Test(h) == 0) return (0);
	xcb[h].LPT_Read_Ctrl |= 0x80;
	if (SH_PGA_Register_Test(h) == 0) return (0);
	xcb[h].LPT_Read_Ctrl &= ~0x80;
	return (-1);
}
