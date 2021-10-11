/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)ps2.c	1.2	93/11/02 SMI"

static char MSG_PS2_Mode[] = "PS/2 Parallel Port";

void PE3_PS2_Block_Write(h, Buffer, Count)
char far *Buffer;
int Count;
{
	OUTB(xcb[h].Media_IO_Address,0, *Buffer);
	Buffer++;
	Count--;
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_STROBE);
	for ( ; Count; Count--, Buffer++) {
		OUTB(xcb[h].Media_IO_Address,0, *Buffer);
	}
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
}

PE3_PS2_Get_Register(h, Reg)
int Reg;
{
	int Val;

	OUTB(xcb[h].Media_IO_Address,0, Reg | PPI_READ+PPI_FALLING_EDGES);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);

	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Read_Ctrl | xcb[h].Media_Data_Strobe);
	Val = INB(xcb[h].Media_IO_Address,0);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);

	return(Val);
}

static PS2_Register_Test(h)
{
	xcb[h].Put_Register = PE3_SH_Put_Register;
	xcb[h].Get_Register = PE3_PS2_Get_Register;
	xcb[h].Block_Write  = PE3_PS2_Block_Write;
	xcb[h].Block_Read   = PE3_WBT_Block_Read;
	xcb[h].Set_Ctrl_Reg = PE3_SH_Set_Ctrl_Reg;
	xcb[h].Setup_Block_Read = PE3_Setup_Block_Read;
	xcb[h].Setup_Block_Write = PE3_Setup_Block_Write;
	xcb[h].Finish_Block_Read = PE3_Finish_Block_Read;
	xcb[h].Finish_Block_Write = PE3_Finish_Block_Write;

#ifdef SCPA
	scpa_access_method(xcb[h].cookie, SCPA_PE3_PS2, 0, 0);
#endif

	xcb[h].Media_Data_Strobe = PPI_STROBE;
	xcb[h].Media_Not_Data_Strobe = ~PPI_STROBE;

	xcb[h].Media_Configuration = EWRITE_MODE;
	xcb[h].Message_Ptr  = MSG_PS2_Mode;
	xcb[h].Setup_Block_Write_Value = NICE_DATA_PORT+PPI_FALLING_EDGES;
	Configure_PPIC(h,0, PPI_READ_EDGE+PPI_USE_STROBE);
	return(0);
}
