/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)sh.c	1.2	93/11/02 SMI"

void PE3_SH_Set_Ctrl_Reg(h, Val)
int Val;
{
	OUTB(xcb[h].Media_IO_Address,0, Val);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
}

void PE3_SH_Put_Register(h, Reg, Val)
int Reg, Val;
{
	OUTB(xcb[h].Media_IO_Address,0, Reg | PPI_FALLING_EDGES);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	OUTB(xcb[h].Media_IO_Address,0, Val);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | 
		xcb[h].Media_Data_Strobe);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
}


void PE3_SH_Block_Write(h, Buffer, Count)
char far *Buffer;
int Count;
{

	for (; Count > 0; Buffer++, Count--) {
		OUTB(xcb[h].Media_IO_Address,0, *Buffer);
 		xcb[h].LPT_Write_Ctrl ^= xcb[h].Media_Data_Strobe;
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	}
}
