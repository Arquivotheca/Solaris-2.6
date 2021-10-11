/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)epp.c	1.2	93/11/02 SMI"

static char MSG_EPP[] = "Enhanced Parallel Port";


void PE3_HH_Set_Ctrl_Reg(h, Val)
int Val;
{
	OUTB(xcb[h].Media_IO_Address,3, Val);
}


void PE3_HH_Put_Register(h, Reg, Val)
int Reg, Val;
{
	OUTB(xcb[h].Media_IO_Address,3, Reg | PPI_RISING_EDGES);
	OUTB(xcb[h].Media_IO_Address,4, Val);
}

PE3_HH_Get_Register(h, Reg)
int Reg;
{
	OUTB(xcb[h].Media_IO_Address,3, Reg | PPI_FALLING_EDGES+PPI_READ);
	return(INB(xcb[h].Media_IO_Address,4));
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
void PE3_HH_Block_Write(h, Buffer, Count)
char far *Buffer;
int Count;
{
	for ( ; Count; Count--, Buffer++)
		OUTB(xcb[h].Media_IO_Address,4, *Buffer);
}

PE3_HH_Block_Read(h, Buffer, Count)
char far *Buffer;
int Count;
{
	for ( ; Count; Count--, Buffer++)
		*Buffer = INB(xcb[h].Media_IO_Address,4);

	return Count;
}

static HH_Register_Test(h)
{
	xcb[h].Put_Register = PE3_HH_Put_Register;
	xcb[h].Get_Register = PE3_HH_Get_Register;
	xcb[h].Block_Write  = PE3_HH_Block_Write;
	xcb[h].Block_Read   = PE3_HH_Block_Read;
	xcb[h].Set_Ctrl_Reg = PE3_HH_Set_Ctrl_Reg;
	xcb[h].Setup_Block_Read = PE3_Setup_Block_Read;
	xcb[h].Setup_Block_Write = PE3_Setup_Block_Write;
	xcb[h].Finish_Block_Read = PE3_Finish_Block_Read;
	xcb[h].Finish_Block_Write = PE3_Finish_Block_Write;

#ifdef SCPA
	scpa_access_method(xcb[h].cookie, SCPA_PE3_EPP, 0, 0);
#endif

	xcb[h].Media_Configuration = EPP_MODE;
	xcb[h].Message_Ptr = MSG_EPP;

	xcb[h].Media_Data_Strobe = PPI_RDS;
	xcb[h].Media_Not_Data_Strobe = ~PPI_RDS;

	xcb[h].Setup_Block_Read_Value  = NICE_DATA_PORT+PPI_READ+PPI_FALLING_EDGES;
	xcb[h].Setup_Block_Write_Value = NICE_DATA_PORT+PPI_FALLING_EDGES;
	Configure_PPIC(h,0, PPI_READ_PULSE);
	Enable_EPP_Mode();
	return 0;
}
