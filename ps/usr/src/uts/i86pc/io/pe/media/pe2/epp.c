/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)epp.c	1.3	93/11/02 SMI"

/****************************************************************************************************************; */
/* Equates													; */
/*---------------------------------------------------------------------------------------------------------------; */
/*;;; pga control register equates */
#define HH_BIDIR_ENB			0x80		/* read enable */
#define HH_NIC_ENB			0x20		/* 8390 access enable */
#define HH_DMA_ENB			0x40		/* dma mode enable */
#define HH_EXT_REG			0x10		/* extra register set */

/*;;; extra register 0 equates */
#define HH_AEDI				0x01
#define HH_AESK				0x02

/*;;; extra register 2 equates */
#define HH_NIC_RST			0x02		/* 8390 reset bit */
#define HH_SLEEP_ENB			0x04		/* enable sleep mode */
#define HH_LI_DISABLE			0x08		/* link integrity */

/*;;; extra register 3 equates */
#define HH_IRQ_ENB			0x01		/* irq enable bit */

/*;;; misc */
#define XLPT_PE				0x20

char MSG_EPP[]			= "Enhanced Parallel Port";

#include "../pga/epp/pe2x.c"

/****************************************************************************************************************; */
/* Hardware Handshake PGA Interface Loopback Test Routine							; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	ds  = cs										; */
/*			dx  = lpt control port address								; */
/*			INT = disabled										; */
/*														; */
/*	Returns:	cx = 255 - pass number on which read after write error occurred (0 == success)		; */
/*			ax, bx, dx destroyed, all other registers (except cx) preserved				; */
/*			INT = disabled										; */
/*														; */
/*	Description: 	called by Reset_Pocket_Ethernet to test the PGA interface.  writes and reads		; */
/*			255 patterns to the PGA data register.							; */
/*---------------------------------------------------------------------------------------------------------------; */
HH_PGA_Register_Test(h)
{
	int	 i;


	if (Enable_EPP_Mode(h) != 0) return (-1);
	for (i = 0xff; i; i--) {
		PUT_REGISTER(h, HH_EXT_REG+2, HH_NIC_RST);
		OUTB(xcb[h].Media_IO_Address,3, HH_DMA_ENB);
		OUTB(xcb[h].Media_IO_Address,4, i);
		OUTB(xcb[h].Media_IO_Address,3, HH_DMA_ENB+HH_BIDIR_ENB);	/* go to read mode */
		if ((unsigned char)INB(xcb[h].Media_IO_Address,4) != i) break;
	}
	OUTB(xcb[h].Media_IO_Address,3, 0);
	if (i) Disable_EPP_Mode(h);
	return (i);
}

/****************************************************************************************************************; */
/* Hardware Handshake Put Register										; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	bl = value to written									; */
/*			bh = 8390 register number								; */
/*														; */
/*	Returns:	dx = EPP data register									; */
/*			al, destoyed, all other registers preserved						; */
/*														; */
/*	Description:	used to write a value to an 8390 register.  this is the put register routine used on	; */
/*			parallel ports that support hardware hanshaking.					; */
/*---------------------------------------------------------------------------------------------------------------; */
void PE2_HH_Put_Register(h, Reg, Value)
int	Reg, Value;
{
	OUTB(xcb[h].Media_IO_Address,3, Reg | HH_NIC_ENB);
	OUTB(xcb[h].Media_IO_Address,4, Value);
}

/****************************************************************************************************************; */
/* Hardware Handshake Get Register										; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	bh = 8390 register number								; */
/*														; */
/*	Returns:	dx = EPP data register									; */
/*			al = register value									; */
/*			ah destoyed, all other registers preserved						; */
/*														; */
/*	Description:	used to read a value from an 8390 register.  this is the register read routine used	; */
/*			when the parallel port supports hardware handshaking.					; */
/*---------------------------------------------------------------------------------------------------------------; */
PE2_HH_Get_Register(h, Reg)
int	Reg;
{
	OUTB(xcb[h].Media_IO_Address,3, Reg | HH_NIC_ENB+HH_BIDIR_ENB);
	return( (unsigned char)INB(xcb[h].Media_IO_Address,4) );
}

/****************************************************************************************************************; */
/* Hardware Handshake Remote DMA Block Write Routine								; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	ds:si = address of block to be written 							; */
/*			cx    = byte count of block to be written						; */
/*														; */
/*	Returns:	cx    = number of bytes not written (0 == success)					; */
/*			ax, bx, dx, si, di destroyed, all other registers preserved				; */
/*														; */
/*	Description:	writes cx bytes of data from ds:si to pocket ethernet adapter.  this is the block	; */
/*			write routine used when the parallel port supports hardware handshaking.		; */
/*---------------------------------------------------------------------------------------------------------------; */
void PE2_HH_Block_Write(h, Data, Count)
char far *Data;
int	  Count;
{
	int	i;

	OUTB(xcb[h].Media_IO_Address,3, 0);
	OUTB(xcb[h].Media_IO_Address,3, HH_DMA_ENB);
	for ( ; Count; Count--, Data++)
		OUTB(xcb[h].Media_IO_Address,4, *Data);
	for (i = 0; i < 10000; i++)
		if ( !(INB(xcb[h].Media_IO_Address,1) & XLPT_PE) ) break;
	OUTB(xcb[h].Media_IO_Address,3, 0);
}

/****************************************************************************************************************; */
/* Hardware Hnadshake Remote DMA Block Read Routine								; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	es:di = address to read block into 							; */
/*			cx    = byte count of block to be read							; */
/*														; */
/*	Returns:	cx    = number of bytes not read (0 == success)						; */
/*			ax, bx, dx, si, di destroyed, all other registers preserved				; */
/*														; */
/*	Description:	reads cx bytes of data from the pocket ethernet adapter writing them to es:di.		; */
/*			this is the block read routine used when the parallel port supports hardware		; */
/*			handshaking.										; */
/*---------------------------------------------------------------------------------------------------------------; */
PE2_HH_Block_Read(h, Data, Count)
char far *Data;
int Count;
{
	OUTB(xcb[h].Media_IO_Address,3, HH_BIDIR_ENB);
	OUTB(xcb[h].Media_IO_Address,3, HH_BIDIR_ENB | HH_DMA_ENB);
	for ( ; Count; Count--, Data++)
		*Data = INB(xcb[h].Media_IO_Address,4);
	OUTB(xcb[h].Media_IO_Address,3, HH_BIDIR_ENB);
	return Count;
}

/****************************************************************************************************************; */
/* Hardware Handshake Remote DMA Memory Test Pattern Write Routine						; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	si = seed for pattern generation							; */
/*			cx = byte count of block to be written							; */
/*														; */
/*	Returns:	ax, bx, cx, dx, si destroyed, all other registers preserved				; */
/*														; */
/*	Description:	writes cx bytes of a test pattern dervived from the seed to the pocket ethernet		; */
/*			adapter.  this is the write test pattern routine used when the parallel port supports	; */
/*			hardware handshaking.									; */
/*---------------------------------------------------------------------------------------------------------------; */
PE2_HH_Write_Test_Pattern(h, Seed, Count)
int	Seed, Count;
{
	OUTB(xcb[h].Media_IO_Address,3, HH_NIC_ENB);
	OUTB(xcb[h].Media_IO_Address,3, HH_DMA_ENB);

	for ( ; Count; Count--, Seed++)
		OUTB(xcb[h].Media_IO_Address,4, ((Seed >> 8) & 0xff) ^ (Seed & 0xff) );
	OUTB(xcb[h].Media_IO_Address,3, 0);
	return(0);
}

/****************************************************************************************************************; */
/* Hardware Handshake Remote DMA Memory Test Pattern Check Routine						; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	si = seed used for pattern generation							; */
/*			cx = byte count of block to be checked							; */
/*														; */
/*	Returns:	bp = number of mismatches (0 == success)						; */
/*			ax, bx, cx, dx, si, destroyed, all other registers preserved				; */
/*														; */
/*	Description:	reads cx bytes of data from the pocket arcnet adapter checking them against		; */
/*			the pattern that would have been created by Write Test Pattern.  this is the check	; */
/*			test pattern routine used when the parallel port supports hardware handshaking.		; */
/*---------------------------------------------------------------------------------------------------------------; */
PE2_HH_Check_Test_Pattern(h, Seed, Count)
int	Seed, Count;
{
	int	Errors = 0;

	OUTB(xcb[h].Media_IO_Address,3, HH_NIC_ENB+HH_BIDIR_ENB);		/* ensure were in read mode */
	OUTB(xcb[h].Media_IO_Address,3, HH_DMA_ENB+HH_BIDIR_ENB);		/* put pga in dma mode */
	for ( ; Count; Count--, Seed++)
		if ( (unsigned char)INB(xcb[h].Media_IO_Address,4) != (((Seed >> 8) & 0xff) ^ (Seed & 0xff)) )
			Errors++;
	OUTB(xcb[h].Media_IO_Address,3, 0);
	return(Errors);
}

/****************************************************************************************************************; */
/* Address EEPROM Support Routines for Zenith Auto Parallel Port							; */
/****************************************************************************************************************; */
void PE2_HH_Enable_EECS(h)
{
	OUTB(xcb[h].Media_IO_Address,3, HH_EXT_REG);
}

void PE2_HH_Disable_EECS(h)
{
	OUTB(xcb[h].Media_IO_Address,3, 0);
}

void PE2_HH_EEPROM_Put_Bit(h, Bit)
int	Bit;
{
	OUTB(xcb[h].Media_IO_Address,4, Bit);
	OUTB(xcb[h].Media_IO_Address,4, Bit | HH_AESK);
	OUTB(xcb[h].Media_IO_Address,4, Bit);
}

/****************************************************************************************************************; */
/* EEPROM Get Bit												; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	ds = cs or cs alias selector								; */
/*			dx = address of lpt data port								; */
/*			EEPROM Read command has been given to EEPROM						; */
/*														; */
/*	Returns:	al = bit 0 contains bit									; */
/*			registers preserved: al, bx, cx, dx, ds, es, bp, ss, sp, flags (DI)			; */
/*			registers destroyed: ah, flags (OSZAPC)							; */
/*														; */
/*	Description: 	outputs one bit in serial stream to EEPROM						; */
/*---------------------------------------------------------------------------------------------------------------; */
PE2_HH_EEPROM_Get_Bit(h)
{
		return ( ((INB(xcb[h].Media_IO_Address,1) >> 3) & 1) );
}

/****************************************************************************************************************; */
/* Enable Pocket LAN Adapter Interrupt										; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	interrupts disabled (may be temporarily enabled)					; */
/*														; */
/*	Returns:	nothing											; */
/*			registers preserved: ah, bx, cx, si, di, ds, es, bp, ss, sp, flags (DI)			; */
/*			registers destroyed: al, dx, flags (OSZAPC)						; */
/*														; */
/*	Description:	This is called as if it were Set_PGA_Ctrl, because HH doesn't use that routine		; */
/*			but uses a different bit for IRQ_Enable.						; */
/*---------------------------------------------------------------------------------------------------------------; */
void PE2_HH_Enable_Int(h)
{
	OUTB(xcb[h].Media_IO_Address,3, HH_EXT_REG+3);
	OUTB(xcb[h].Media_IO_Address,4, HH_IRQ_ENB);
}

/****************************************************************************************************************; */
/* Disable Pocket LAN Adapter Interrupt										; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	interrupts disabled (may be temporarily enabled)					; */
/*														; */
/*	Returns:	nothing											; */
/*			registers preserved: ah, bx, cx, si, di, ds, es, bp, ss, sp, flags (DI)			; */
/*			registers destroyed: al, dx, flags (OSZAPC)						; */
/*														; */
/*	Description:	This is called as if it were Set_PGA_Ctrl, because HH doesn't use that routine		; */
/*			but uses a different bit for IRQ_Enable.						; */
/*---------------------------------------------------------------------------------------------------------------; */
void PE2_HH_Disable_Int(h)
{
	OUTB(xcb[h].Media_IO_Address,3, HH_EXT_REG+3);
	OUTB(xcb[h].Media_IO_Address,4, 0);
}

/****************************************************************************************************************; */
/* Pulse Pocket LAN Adapter Interrupt										;
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	interrupts disabled (may be temporarily enabled)					; */
/*														; */
/*	Returns:	nothing											; */
/*			registers preserved: ah, bx, cx, si, di, ds, es, bp, ss, sp, flags (DI)			; */
/*			registers destroyed: al, dx, flags (OSZAPC)						; */
/*														; */
/*	Description:	This is called as if it were Set_PGA_Ctrl, because HH doesn't use that routine		; */
/*			but uses a different bit for IRQ_Enable.						; */
/*---------------------------------------------------------------------------------------------------------------; */
void PE2_HH_Pulse_Int(h)
{
	OUTB(xcb[h].Media_IO_Address,3, HH_EXT_REG+3);
	OUTB(xcb[h].Media_IO_Address,4, 0);
	OUTB(xcb[h].Media_IO_Address,4, HH_IRQ_ENB);
}


/****************************************************************************************************************; */
/* Reset Pocket Ethernet Network Interface Controller (8390)							; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	interrupts disabled (may be temporarily enabled)					; */
/*														; */
/*	Returns:	nothing											; */
/*														; */
/*	Description:	applies a hard reset to the 8390 reset input, setting link integrity as directed by	; */
/*			bit 0 of the flags byte of the EEPROM.							; */
/*---------------------------------------------------------------------------------------------------------------; */
void PE2_HH_Adapter_Reset(h, Flags)
int	Flags;
{
	int	Value = HH_NIC_RST;

	OUTB(xcb[h].Media_IO_Address,3, HH_EXT_REG+2);
	if (Flags & EE_FLAGS_SLEEP) Value |= HH_SLEEP_ENB;
	if (Flags & EE_FLAGS_LI)    Value |= HH_LI_DISABLE;
	OUTB(xcb[h].Media_IO_Address,4, Value);
	pe_delay(h,1);
	OUTB(xcb[h].Media_IO_Address,4, Value & ~HH_NIC_RST);
}

void PE2_HH_Adapter_Unhook(h)
{
	Disable_EPP_Mode(h);
}

PE2_HH_Register_Test(h)
{
	xcb[h].Put_Register		= PE2_HH_Put_Register;
	xcb[h].Get_Register		= PE2_HH_Get_Register;
	xcb[h].Block_Write		= PE2_HH_Block_Write;
	xcb[h].Block_Read		= PE2_HH_Block_Read;
	xcb[h].Adapter_Enable_Int	= PE2_HH_Enable_Int;
	xcb[h].Adapter_Disable_Int	= PE2_HH_Disable_Int;
	xcb[h].Adapter_Pulse_Int	= PE2_HH_Pulse_Int;
	xcb[h].Adapter_Reset_Ptr	= PE2_HH_Adapter_Reset;
	xcb[h].Adapter_Unhook_Ptr	= PE2_HH_Adapter_Unhook;

	xcb[h].EEPROM_Enable		= PE2_HH_Enable_EECS;
	xcb[h].EEPROM_Disable		= PE2_HH_Disable_EECS;
	xcb[h].EEPROM_Put_Bit		= PE2_HH_EEPROM_Put_Bit;
	xcb[h].EEPROM_Get_Bit		= PE2_HH_EEPROM_Get_Bit;

	xcb[h].Write_Test_Pattern	= PE2_HH_Write_Test_Pattern;
	xcb[h].Check_Test_Pattern	= PE2_HH_Check_Test_Pattern;

	xcb[h].Media_Configuration	= EPP_MODE;
	xcb[h].Message_Ptr		= MSG_EPP;

#ifdef SCPA
	scpa_access_method(xcb[h].cookie, SCPA_PE2_EPP, 0, 0);
#endif

	Download_Bit_Stream(h,HH_Bit_Stream, sizeof(HH_Bit_Stream));
	return (HH_PGA_Register_Test(h));
}
