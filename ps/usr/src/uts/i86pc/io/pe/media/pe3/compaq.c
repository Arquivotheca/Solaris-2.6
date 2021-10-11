/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)compaq.c	1.2	93/11/02 SMI"

/* COMPAQ Specific Definitions */
#define CPQ_WRITE_ENB	0x80
#define CPQ_DIR_PORT	0x65

static char MSG_COMPAQ[] = "COMPAQ Parallel Port";

PE3_CPQ_Get_Register(h, Reg)
int Reg;
{
	int Val;

	OUTB(xcb[h].Media_IO_Address,0, Reg | PPI_READ+PPI_FALLING_EDGES);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);

	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Read_Ctrl | xcb[h].Media_Data_Strobe);
	OUTB(CPQ_DIR_PORT,0, CPQ_READ_ENB);
	Val = INB(xcb[h].Media_IO_Address,0);
	OUTB(CPQ_DIR_PORT,0, CPQ_WRITE_ENB);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return(Val);
}

PE3_CPQ_Block_Read(h, Buffer, Count)
char far *Buffer;
int Count;
{
	OUTB(xcb[h].Media_IO_Address,0, -1);
	OUTB(CPQ_READ_ENB,0, CPQ_DIR_PORT);
	for (; Count; Count--, Buffer++) {
		xcb[h].LPT_Read_Ctrl ^= xcb[h].Media_Data_Strobe;
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Read_Ctrl);
		*Buffer = INB(xcb[h].Media_IO_Address, 0);
	}
	OUTB(CPQ_DIR_PORT,0, CPQ_WRITE_ENB);
	return Count;
}

static CPQ_Register_Test(h)
{
	xcb[h].Put_Register = PE3_SH_Put_Register;
	xcb[h].Get_Register = PE3_CPQ_Get_Register;
	xcb[h].Block_Write  = PE3_SH_Block_Write;
	xcb[h].Block_Read   = PE3_CPQ_Block_Read;
	xcb[h].Set_Ctrl_Reg = PE3_SH_Set_Ctrl_Reg;
	xcb[h].Setup_Block_Read = PE3_Setup_Block_Read;
	xcb[h].Setup_Block_Write = PE3_Setup_Block_Write;
	xcb[h].Finish_Block_Read = PE3_Finish_Block_Read;
	xcb[h].Finish_Block_Write = PE3_Finish_Block_Write;

#ifdef SCPA
	scpa_access_method(xcb[h].cookie, SCPA_PE3_COMPAQ, 0, 0);
#endif

	xcb[h].Media_Configuration= COMPAQ_MODE;

	
	Configure_PPIC(h,0, PPI_READ_EDGE);
	xcb[h].Message_Ptr = MSG_COMPAQ;
	return(0);
}
