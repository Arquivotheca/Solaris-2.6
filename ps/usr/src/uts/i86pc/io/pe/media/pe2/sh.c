/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)sh.c	1.3	93/11/02 SMI"

/****************************************************************************************************************; */
/* Equates													; */
/*---------------------------------------------------------------------------------------------------------------; */
/*;;; EEPROM */
#define EE_READ			0x80
#define EE_AESK			0x02
#define EE_AECS			0x48

/*;;; 8390 Specific Control Register Equates */
#define SH_NIC_RST		0x20					/* 8390 reset bit */

/*;;; Control Port Bit Definitions */
#define SH_DMASTB		0x01
#define SH_RDS			0x02					/* register data strobe bit */
#define SH_NICSTB		0x08					/* If RDS is asserted */

/*;;; Status Port Bit Definitions */
#define SH_STB			0x20					/* strobe line */

/*;;; Data Port Definitions */
#define SH_AESK			0x02					/* address eeprom serial clk bit */
#define SH_AECS			0x10					/* address eeprom chip select */

#define SH_LI_DISABLE		0x02
#define SH_SLEEP_ENB		0x04
#define SH_NIC_READ		0x10
#define PGA_REG2		0x40
#define PGA_REG1		0x80

/****************************************************************************************************************; */
/* Proprietary Gate Array Definitions										; */
/*---------------------------------------------------------------------------------------------------------------; */

/*;;; Control Register Definitions */
#define SH_PGA_IRQ_INVERT	0x01					/* IRQ polarity control line (1 = invert) */
#define SH_PGA_IRQ_ENABLE	0x08					/* IRQ enable control line   (1 = enable) */


#ifdef notdef
int	Reset_Flags		= 0;
#endif

/****************************************************************************************************************; */
/* Set PGA Control Register											; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	bh = PGA control register value								; */
/*														; */
/*	Returns:	dx = lpt control port address								; */
/*			al destroyed, all other registers preserved						; */
/*														; */
/*	Description:	writes the value in bh to the lpt data port and then creates a strobe pulse		; */
/*			on the register address strobe (RAS) line.  this is the set pga control routine used	; */
/*			when the parallel port does not support hardware handshaking.				; */
/*---------------------------------------------------------------------------------------------------------------; */
void Set_PGA_Ctrl_Reg(h, Value)
int Value;
{
	OUTB(xcb[h].Media_IO_Address,0, Value);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | SH_RDS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
}

#include "../pga/fifo/pe2x.c"

void PE2_SH_Enable_Int(h)
{
	Set_PGA_Ctrl_Reg(h, PGA_REG1+SH_PGA_IRQ_ENABLE);
}

void PE2_SH_Disable_Int(h)
{
	Set_PGA_Ctrl_Reg(h, PGA_REG1+0);
}

void PE2_SH_Pulse_Int(h)
{
	Set_PGA_Ctrl_Reg(h, PGA_REG1+0);
	Set_PGA_Ctrl_Reg(h, PGA_REG1+SH_PGA_IRQ_ENABLE);
}

/****************************************************************************************************************; */
/* Software Handshake PGA Interface Loopback Test Routine							; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	ds = cs											; */
/*			dx = lpt control port address								; */
/*			INT = disabled										; */
/*														; */
/*	Returns:	cx = number of read after write errors (0 == success)					; */
/*			ax, bx, dx destroyed, all other registers preserved					; */
/*			INT = disabled										; */
/*														; */
/*	Description: 	called by Check_Bidirectional to test the PGA interface.  writes and reads		; */
/*			all possible (256) patterns to the PGA data register.					; */
/*---------------------------------------------------------------------------------------------------------------; */
SH_PGA_Register_Test(h)
{
	int	 i;

/*	Set_PGA_Ctrl_Reg(h, PGA_REG2+SH_NIC_RST); */
	for (i = 0xff; i; i--) {
		OUTB(xcb[h].Media_IO_Address,0, i);
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | SH_DMASTB);
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
		if (GET_REGISTER(h, 0) != i) break;
	}
	Set_PGA_Ctrl_Reg(h, PGA_REG2+0);
	return (i);
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
void PE2_SH_Adapter_Reset(h, Flags)
int	Flags;
{
	int	Value = SH_NIC_RST+PGA_REG2;

	Value |= xcb[h].Reset_Flags;
	if (xcb[h].IRQ_Ctrl_Polarity != 0) Value |= SH_PGA_IRQ_INVERT;
	if (Flags & EE_FLAGS_SLEEP) Value |= SH_SLEEP_ENB;
	if (Flags & EE_FLAGS_LI)    Value |= SH_LI_DISABLE;
	Set_PGA_Ctrl_Reg(h, Value);
	Set_PGA_Ctrl_Reg(h, Value & ~SH_NIC_RST);
}

void PE2_SH_Adapter_Unhook(h)
{}

/****************************************************************************************************************; */
/* Software Handshake Put Register										; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	bl = value to written									; */
/*			bh = 8390 register number								; */
/*														; */
/*	Returns:	dx = lpt data port address								; */
/*			al destroyed, all other registers preserved						; */
/*														; */
/*	Description:	used to write a value to an 8390 register.  this is the put register routine used on	; */
/*			parallel ports that do not support hardware hanshaking.					; */
/*---------------------------------------------------------------------------------------------------------------; */
void PE2_SH_Put_Register(h, Reg, Val)
int	Reg, Val;
{
	int	i;

	OUTB(xcb[h].Media_IO_Address,0, Reg);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | SH_RDS);
	OUTB(xcb[h].Media_IO_Address,0, Val);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | SH_RDS | SH_NICSTB);
	for (i = 0; i < 16; i++)
		if (INB(xcb[h].Media_IO_Address,1) & SH_STB) break;
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
}

/****************************************************************************************************************; */
/* Software Handshake Remote DMA Block Write Routine								; */
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
/*			write routine used when the parallel port does not support hardware handshaking.	; */
/*---------------------------------------------------------------------------------------------------------------; */
void PE2_SH_Block_Write(h, Data, Count)
char far *Data;
int	  Count;
{
	int	Image;
	
	Image = (xcb[h].LPT_Write_Ctrl | (INB(xcb[h].Media_IO_Address,2) & SH_DMASTB));
	for ( ; Count; Count--, Data++) {
		OUTB(xcb[h].Media_IO_Address,0, *Data);
		Image ^= SH_DMASTB;
		OUTB(xcb[h].Media_IO_Address,2, Image);
	}
}

/****************************************************************************************************************; */
/* Software Handshake Remote DMA Memory Test Pattern Write Routine						; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	si = seed for pattern generation							; */
/*			cx = byte count of block to be written							; */
/*														; */
/*	Returns:	ax, bx, cx, dx, si destroyed, all other registers preserved				; */
/*														; */
/*	Description:	writes cx bytes of a test pattern dervived from the seed to the pocket ethernet		; */
/*			adapter, creating an edge on the register data strobe (RDS) line for each byte.		; */
/*			the remote dma write must have previously been set up and initiated.			; */
/*---------------------------------------------------------------------------------------------------------------; */
PE2_SH_Write_Test_Pattern(h, Seed, Count)
int	Seed, Count;
{
	int	Image;

	Image = xcb[h].LPT_Write_Ctrl;

	for ( ; Count; Count--, Seed++) {
		OUTB(xcb[h].Media_IO_Address,0, ((Seed >> 8) & 0xff) ^ (Seed & 0xff) );
		Image ^= SH_DMASTB;
		OUTB(xcb[h].Media_IO_Address,2, Image);
	}
	return (0);
}

/****************************************************************************************************************; */
/* Address EEPROM Support Routines										; */
/****************************************************************************************************************; */
void PE2_SH_Enable_EECS(h)
{
	Set_PGA_Ctrl_Reg(h, PGA_REG1+SH_AECS);				/* set CS */
}

void PE2_SH_Disable_EECS(h)
{
	Set_PGA_Ctrl_Reg(h, PGA_REG1);					/* reset CS */
}

/****************************************************************************************************************; */
/* EEPROM Put Bit												; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	ds = cs or cs alias selector								; */
/*			al = bit 0 contains bit to be output							; */
/*			dx = address of lpt data port								; */
/*														; */
/*	Returns:	nothing											; */
/*			registers preserved: al, bx, cx, dx, ds, es, bp, ss, sp, flags (DI)			; */
/*			registers destroyed: ah, flags (OSZAPC)							; */
/*														; */
/*	Description: 	outputs one bit in serial stream to EEPROM						; */
/*---------------------------------------------------------------------------------------------------------------; */

void PE2_SH_EEPROM_Put_Bit(h, Bit)
int Bit;
{
	OUTB(xcb[h].Media_IO_Address,0, Bit);
	OUTB(xcb[h].Media_IO_Address,0, Bit | EE_AESK);
	OUTB(xcb[h].Media_IO_Address,0, Bit);
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
PE2_SH_EEPROM_Get_Bit(h)
{
	int rval;
	rval = ( ((INB(xcb[h].Media_IO_Address,1) >> 3) & 1) );
	return rval;
}
