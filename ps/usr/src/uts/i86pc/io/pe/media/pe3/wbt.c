/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)wbt.c	1.2	93/11/02 SMI"

static char MSG_Bidirectional[] = "Bidirectional";

PE3_WBT_Get_Register(h, Reg)
int Reg;
{
	int Val;

	OUTB(xcb[h].Media_IO_Address,0, Reg | PPI_READ+PPI_FALLING_EDGES);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);

	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Read_Ctrl | xcb[h].Media_Data_Strobe);
	if (xcb[h].Hardware_Configuration & AT_MODE)
		OUTB(xcb[h].Media_IO_Address,0, 0xff);
	Val = INB(xcb[h].Media_IO_Address,0);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return(Val);
}

PE3_WBT_Block_Read(h, Buffer, Count)
char far *Buffer;
int Count;
{
	if (xcb[h].Hardware_Configuration & AT_MODE)
		OUTB(xcb[h].Media_IO_Address,0, 0xff);
	for ( ; Count; Count--, Buffer++) {
		xcb[h].LPT_Read_Ctrl ^= xcb[h].Media_Data_Strobe;
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Read_Ctrl);
		*Buffer = INB(xcb[h].Media_IO_Address,0);
	}
	return Count;
}

static WBT_Register_Test(h, Mode)
int	Mode;
{
	if (Mode & COMPAQ_MODE)
		return(CPQ_Register_Test(h));

	xcb[h].Put_Register = PE3_SH_Put_Register;
	xcb[h].Get_Register = PE3_WBT_Get_Register;
	xcb[h].Block_Write  = PE3_SH_Block_Write;
	xcb[h].Block_Read   = PE3_WBT_Block_Read;
	xcb[h].Set_Ctrl_Reg = PE3_SH_Set_Ctrl_Reg;

	xcb[h].Setup_Block_Read = PE3_Setup_Block_Read;
	xcb[h].Setup_Block_Write = PE3_Setup_Block_Write;
	xcb[h].Finish_Block_Read = PE3_Finish_Block_Read;
	xcb[h].Finish_Block_Write = PE3_Finish_Block_Write;

#ifdef SCPA
	scpa_access_method(xcb[h].cookie, SCPA_PE3_WBT, 0, 0);
#endif

	xcb[h].Media_Configuration = BIDIRECTIONAL_MODE;
	xcb[h].Message_Ptr = MSG_Bidirectional;
	if (Mode & TOSHIBA_MODE)
		xcb[h].LPT_Read_Ctrl |= 0x80;
	Configure_PPIC(h, 0, PPI_READ_PULSE);
	return(0);
}
