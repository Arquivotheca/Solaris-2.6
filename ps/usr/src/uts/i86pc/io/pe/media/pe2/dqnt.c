/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)dqnt.c	1.2	93/11/02 SMI"

/****************************************************************************************************************; */
/* Equates													; */
/*---------------------------------------------------------------------------------------------------------------; */
/*;;; Status Port Bit Definitions */
#define DATA_MASK_HI			0xb8					/* high half byte mask */
#define DATA_MASK_LO			0x47					/* low half byte mask */

/*;;; Data Port Definitions */
#define HI_SEL				0x40					/* byte high half select */
#define LO_SEL				0x00					/* byte low half select */
#define DQNT_STB_SEL			0x10

/*;;; Control Port Bit Definitions */
#define DQNT_READ			0x08

/****************************************************************************************************************; */
/* Initialization Data												; */
/*---------------------------------------------------------------------------------------------------------------; */

char MSG_Non_Bidirectional[]		= "Non-Bidirectional";


/****************************************************************************************************************; */
/* Start of Initialization Code											; */
/*---------------------------------------------------------------------------------------------------------------; */

/****************************************************************************************************************; */
/* Double Quasi-Nibble Transfer Register Read									; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	BH	8390 register number								; */
/*														; */
/*	Returns:	AL	register value									; */
/*			AH DX	destroyed, all other registers preserved					; */
/*														; */
/*	Description:	used to read a value from an 8390 register. this is the register read routine		; */
/*			when the parallel port does not support bidirectional transfers.			; */
/*---------------------------------------------------------------------------------------------------------------; */
PE2_DQNT_Get_Register(h,Reg)
int	Reg;
{
	int	i, Lo_Nibble, Hi_Nibble;

	OUTB(xcb[h].Media_IO_Address,0, Reg | SH_NIC_READ);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | SH_RDS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | SH_RDS | SH_NICSTB);
	for (i = 0; i < 16; i++)
		if (INB(xcb[h].Media_IO_Address,1) & SH_STB) break;
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | DQNT_READ);
	Lo_Nibble = INB(xcb[h].Media_IO_Address,1);
	Lo_Nibble = (char)Lo_Nibble >> 3;
	Lo_Nibble &= DATA_MASK_LO;
	OUTB(xcb[h].Media_IO_Address,0, HI_SEL);
	Hi_Nibble = INB(xcb[h].Media_IO_Address,1) & DATA_MASK_HI;
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return (Hi_Nibble | Lo_Nibble);
}

/****************************************************************************************************************; */
/* Double Quasi-Nibble Transfer Remote DMA Block Read Routine							; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	ES:DI	address to read block into 							; */
/*			CX	byte count of block to be read							; */
/*														; */
/*	Returns:	CX	number of bytes not read (0 == success)						; */
/*			AX BX DX DI destroyed, all other registers preserved					; */
/*														; */
/*	Description:	reads cx bytes of data from the pocket ethernet adapter writing them to es:di,		; */
/*			waiting for prq	to be active and then creating an edge on the register data		; */
/*			strobe (RDS) line for each byte.  the remote dma read must have previously		; */
/*			been set up and initiated.  this is the block read routine used when the		; */
/*			parallel port does not support bidirectional transfers.					; */
/*---------------------------------------------------------------------------------------------------------------; */
PE2_DQNT_Block_Read(h, Data, Count)
char far *Data;
int	  Count;
{
	unsigned char Hi_Nibble, Lo_Nibble, Image;

	OUTB(xcb[h].Media_IO_Address,0, 0);
	Image = xcb[h].LPT_Write_Ctrl | DQNT_READ;
	OUTB(xcb[h].Media_IO_Address,2, Image);

	for ( ; Count; Count--, Data++) {
		Lo_Nibble = (((char)INB(xcb[h].Media_IO_Address,1) >> 3) & DATA_MASK_LO);
		Image ^= SH_DMASTB;
		OUTB(xcb[h].Media_IO_Address,2, Image);
		Hi_Nibble = INB(xcb[h].Media_IO_Address,1) & DATA_MASK_HI;
		*Data = Hi_Nibble | Lo_Nibble;
		Image ^= SH_DMASTB;
		OUTB(xcb[h].Media_IO_Address,2, Image);
	}
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return Count;
}

/****************************************************************************************************************; */
/* Double Quasi-Nibble Transfer Remote DMA Memory Test Pattern Check Routine					; */
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
/*			the remote dma read must have previously been set up and initiated.  this is the	; */
/*			pattern check routine used when the parallel port does not support bidirectional	; */
/*			transfers.										; */
/*---------------------------------------------------------------------------------------------------------------; */
PE2_DQNT_Check_Test_Pattern(h, Seed, Count)
int	Seed, Count;
{
	unsigned char	Hi_Nibble, Lo_Nibble, Image;
	int		Errors = 0;

	Image = xcb[h].LPT_Write_Ctrl | DQNT_READ;
	OUTB(xcb[h].Media_IO_Address,0, 0);
	OUTB(xcb[h].Media_IO_Address,2, Image);
	for ( ; Count; Count--, Seed++) {
		Lo_Nibble = (((char)INB(xcb[h].Media_IO_Address,1) >> 3) & DATA_MASK_LO);
		Image ^= SH_DMASTB;
		OUTB(xcb[h].Media_IO_Address,2, Image);
		Hi_Nibble = INB(xcb[h].Media_IO_Address,1) & DATA_MASK_HI;
		Image ^= SH_DMASTB;
		OUTB(xcb[h].Media_IO_Address,2, Image);
		if ( (Hi_Nibble | Lo_Nibble) != (((Seed >> 8) & 0xff) ^ (Seed & 0xff)) )
			Errors++;
	}
	return (Errors);
}

PE2_DQNT_Register_Test(h)
{
	xcb[h].Put_Register		= PE2_SH_Put_Register;
	xcb[h].Get_Register		= PE2_DQNT_Get_Register;
	xcb[h].Block_Write		= PE2_SH_Block_Write;
	xcb[h].Block_Read		= PE2_DQNT_Block_Read;
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
	xcb[h].Check_Test_Pattern	= PE2_DQNT_Check_Test_Pattern;

	xcb[h].Media_Configuration	= NON_BIDIRECTIONAL_MODE;
	xcb[h].Message_Ptr		= MSG_Non_Bidirectional;

#ifdef SCPA
	scpa_access_method(xcb[h].cookie, SCPA_PE2_DQNT, 0, 0);
#endif

	Download_Bit_Stream(h, SH_Bit_Stream, sizeof(SH_Bit_Stream));
	Set_PGA_Ctrl_Reg(h, PGA_REG2+SH_NIC_RST+DQNT_STB_SEL);
	if (SH_PGA_Register_Test(h) == 0) {
		xcb[h].Reset_Flags |= DQNT_STB_SEL;
		return (0);
	} else	return (-1);
}
