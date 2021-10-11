/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)dqnt.c	1.1	93/10/28 SMI"

/* Status Port Bit Definitions */
#define DATA_MASK_HI	0xb8					/* high half byte mask */
#define DATA_MASK_LO	0x47					/* low half byte mask */

static char MSG_Non_Bidirectional[] = "Non-Bidirectional";


PE3_DQNT_Get_Register(h,Reg)
int Reg;
{
	int	Hi_Val, Lo_Val;

	OUTB(xcb[h].Media_IO_Address,0, Reg | PPI_READ | PPI_FALLING_EDGES);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | xcb[h].Media_Data_Strobe);
	Lo_Val = INB(xcb[h].Media_IO_Address,1);
	Lo_Val = (char)Lo_Val >> 3;
	Lo_Val &= DATA_MASK_LO;
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	Hi_Val = INB(xcb[h].Media_IO_Address,1) & DATA_MASK_HI;
	return(Hi_Val | Lo_Val);
}

PE3_DQNT_Block_Read(h,Buffer, Count)
char far *Buffer;
int Count;
{
	unsigned char Hi_Nibble, Lo_Nibble;

	for (; Count; Count--, Buffer++) {
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | xcb[h].Media_Data_Strobe);
		Lo_Nibble = (((char)INB(xcb[h].Media_IO_Address,1) >> 3) & DATA_MASK_LO);
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
		Hi_Nibble = INB(xcb[h].Media_IO_Address,1) & DATA_MASK_HI;
		*Buffer = Hi_Nibble | Lo_Nibble;
	}
	return(Count);
}

static DQNT_Register_Test(h)
{
	xcb[h].Setup_Block_Read_Value = NICE_DATA_PORT+PPI_READ+PPI_FALLING_EDGES;

	xcb[h].Put_Register = PE3_SH_Put_Register;
	xcb[h].Get_Register = PE3_DQNT_Get_Register;
	xcb[h].Block_Write  = PE3_SH_Block_Write;
	xcb[h].Block_Read   = PE3_DQNT_Block_Read;
	xcb[h].Set_Ctrl_Reg = PE3_SH_Set_Ctrl_Reg;
	xcb[h].Setup_Block_Read = PE3_Setup_Block_Read;
	xcb[h].Setup_Block_Write = PE3_Setup_Block_Write;
	xcb[h].Finish_Block_Read = PE3_Finish_Block_Read;
	xcb[h].Finish_Block_Write = PE3_Finish_Block_Write;

#ifdef SCPA
	scpa_access_method(xcb[h].cookie, SCPA_PE3_DQNT, 0, 0);
#endif

	xcb[h].Media_Configuration	= NON_BIDIRECTIONAL_MODE;
	xcb[h].Message_Ptr 		= MSG_Non_Bidirectional;
	Configure_PPIC(h,0, 0);
	return(0);
}
