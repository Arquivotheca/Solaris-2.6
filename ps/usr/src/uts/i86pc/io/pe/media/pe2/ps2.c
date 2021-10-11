/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)ps2.c	1.3	93/11/02 SMI"

/****************************************************************************************************************; */
/* Equates													; */
/*---------------------------------------------------------------------------------------------------------------; */
#define FIFO_DEPTH		4

/*;;; Control Port Bit Definitions */
#define PS2_DMASTB		0x01
#define PS2_RAS			0x08				/* register address strobe bit */
#define PS2_PRQ			0x10
#define PS2_AUTOSTB		0x80

char MSG_PS2_Mode[]		= "PS/2 Parallel Port";

#include "../pga/fifoas/pe2as.c"

unsigned char PS2_Test_Pattern[]	= { 3, 0xc, 0x30, 0xc0, 0xc0 };

/****************************************************************************************************************; */
/* Model 90 Remote DMA Block Write Routine									; */
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
/*			write routine used when the parallel port supports Model 90 autostrobing 		; */
/*---------------------------------------------------------------------------------------------------------------; */
void PE2_PS2_Block_Write(h, Data, Count)
char far *Data;
int Count;
{
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl ^ PS2_DMASTB);
	for ( ; Count; Count--, Data++) {
		pe_delay(h, 1);					/* Could be smaller */
		OUTB(xcb[h].Media_IO_Address,0, *Data);
	}
}

/****************************************************************************************************************; */
/* Model 90 PGA Interface Loopback Test Routine							; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	ds = cs											; */
/*			dx = lpt control port address								; */
/*														; */
/*	Returns:	cx = number of read after write errors (0 == success)					; */
/*			ax, bx, dx destroyed, all other registers preserved					; */
/*														; */
/*	Description: 	called by Check_Bidirectional to test the PGA interface.  writes and reads		; */
/*			all possible (256) patterns to the PGA data register.					; */
/*---------------------------------------------------------------------------------------------------------------; */
PS2_PGA_Register_Test(h)
{
	int	i, Image;

	BLOCK_WRITE(h, (char far *)PS2_Test_Pattern, FIFO_DEPTH);
	Image = xcb[h].LPT_Read_Ctrl  | PS2_RAS;
	OUTB(xcb[h].Media_IO_Address,2, Image);
	for (i = 0; i < FIFO_DEPTH; i++) {
		if ((unsigned char)INB(xcb[h].Media_IO_Address,0) != PS2_Test_Pattern[i])
			break;
		Image ^= PS2_RAS;
		OUTB(xcb[h].Media_IO_Address,2, Image);
	}
	Set_PGA_Ctrl_Reg(h, 0);
	return (i == FIFO_DEPTH);
}

/****************************************************************************************************************; */
/* Model 90 Remote DMA Memory Test Pattern Write Routine								; */
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
int PE2_PS2_Write_Test_Pattern(h, Seed, Count)
int	Seed, Count;
{
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PS2_DMASTB);
	for ( ; Count; Count--, Seed++)
		OUTB(xcb[h].Media_IO_Address,0, ((Seed >> 8) & 0xff) ^ (Seed & 0xff) );
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return 0;
}

PE2_PS2_Register_Test(h)
{
	xcb[h].Put_Register		= PE2_SH_Put_Register;
	xcb[h].Get_Register		= PE2_WBT_Get_Register;
	xcb[h].Block_Write		= PE2_PS2_Block_Write;
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

	xcb[h].Write_Test_Pattern	= PE2_PS2_Write_Test_Pattern;
	xcb[h].Check_Test_Pattern	= PE2_WBT_Check_Test_Pattern;

	xcb[h].Media_Configuration	= EWRITE_MODE;
	xcb[h].Message_Ptr		= MSG_PS2_Mode;

#ifdef SCPA
	scpa_access_method(xcb[h].cookie, SCPA_PE3_PS2, 0, 0);
#endif

	xcb[h].Reset_Flags		= 0;
	Download_Bit_Stream(h, PS2_Bit_Stream, sizeof(PS2_Bit_Stream));
	return (PS2_PGA_Register_Test(h));
}
