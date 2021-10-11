/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)ppi.c	1.2	93/11/02 SMI"

#include "ppi.h"

#define TEST_DATA	0xa5

/***************************************************************************************************************; */
/* Initialization Data												; */
/*--------------------------------------------------------------------------------------------------------------; */
#ifdef notdef
int Data_Strobe		=	PPI_RDS;
#endif

/***************************************************************************************************************; */
/* Initialization Code												; */
/*--------------------------------------------------------------------------------------------------------------; */

void PPI_Write_Register();
void PPI_Configure();

PPI_Check_Adapter(h)
{
	PPI_Write_Register(h,0, 0);
	PPI_Configure(h,0, 0, 0);
	PPI_Write_Register(h,GENREG3, TEST_DATA);
	if ( PPI_Read_Register(h,GENREG3) == TEST_DATA) return (0);
	else return (-1);
}

void PPI_Profile_Port(h)
{
	PPI_Control_Signals(h);
	PPI_Write_Register(h,0);
	PS2_Setup(h);
	xcb[h].Hardware_Configuration |= NON_BIDIRECTIONAL_MODE;
	xcb[h].Hardware_Configuration |= PPI_Check_Bidir(h);
	if (PPI_Check_PS2_Writes(h) == 0) xcb[h].Hardware_Configuration |= EWRITE_MODE;
	if (PPI_Check_EPP(h) == 0) xcb[h].Hardware_Configuration |= EPP_MODE;
}

PPI_Check_Bidir(h)
{


	PPI_Configure(h,0, PPI_READ_PULSE, 0);
	PPI_Write_Register(h,GENREG3, TEST_DATA);
	if (Check_Compaq_Signature(h) == 0)
		if (PPI_Bidir_Compaq_Read(h) == TEST_DATA)
			return (COMPAQ_MODE+BIDIRECTIONAL_MODE);
	if (PPI_Bidir_Buffered_Read(h) == TEST_DATA) return (BIDIRECTIONAL_MODE);
	if (PPI_Bidir_Toshiba_Read(h)  == TEST_DATA) return (BIDIRECTIONAL_MODE+TOSHIBA_MODE);
	if (PPI_Bidir_AT_Read(h)       == TEST_DATA) return (BIDIRECTIONAL_MODE+AT_MODE);
	return (NON_BIDIRECTIONAL_MODE);
}

PPI_Check_PS2_Writes(h)
{
	int	Value;

	PPI_Configure(h,0, PPI_READ_PULSE+PPI_USE_STROBE, 0);
	OUTB(xcb[h].Media_IO_Address,0, GENREG3+PPI_FALLING_EDGES);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_STROBE);
	pe_delay(h,1);
	OUTB(xcb[h].Media_IO_Address,0, ~TEST_DATA);
	pe_delay(h,1);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	OUTB(xcb[h].Media_IO_Address,0, GENREG3+PPI_READ+PPI_FALLING_EDGES);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Read_Ctrl | PPI_STROBE);
	pe_delay(h,1);
	Value = INB(xcb[h].Media_IO_Address,0);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	if (Value == ~TEST_DATA) return (0);
	else return (-1);
}

PPI_Check_EPP(h)
{
	int Value;

	if (Enable_EPP_Mode(h) != 0) return (-1);
	PPI_Configure(h,0, PPI_READ_PULSE, 3);			/* 100-150 ns transfer delay, clock = 20Mhz */
	OUTB(xcb[h].Media_IO_Address,0, GENREG3+PPI_FALLING_EDGES);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	OUTB(xcb[h].Media_IO_Address,4, TEST_DATA);
	OUTB(xcb[h].Media_IO_Address,3, GENREG6+PPI_FALLING_EDGES);
	OUTB(xcb[h].Media_IO_Address,4, 0);
	OUTB(xcb[h].Media_IO_Address,3, GENREG3+PPI_FALLING_EDGES+PPI_READ);
	Value = INB(xcb[h].Media_IO_Address,4);
	Disable_EPP_Mode(h);
	if (Value == TEST_DATA) return (0);
	else return (-1);
}

PPI_Bidir_Compaq_Read(h)
{
	int Value;

	OUTB(xcb[h].Media_IO_Address,0, GENREG3+PPI_FALLING_EDGES+PPI_READ);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Read_Ctrl | xcb[h].Data_Strobe);
	OUTB(CPQ_DIR_PORT,0, CPQ_READ_ENB);
	pe_delay(h,1);
	Value = INB(xcb[h].Media_IO_Address,0);
	OUTB(CPQ_DIR_PORT,0, CPQ_WRITE_ENB);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return (Value);
}

PPI_Bidir_Buffered_Read(h)
{
	int	Value;

	OUTB(xcb[h].Media_IO_Address,0, GENREG3+PPI_READ+PPI_FALLING_EDGES);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Read_Ctrl | xcb[h].Data_Strobe);
	pe_delay(h,1000);
	Value = INB(xcb[h].Media_IO_Address,0);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return (Value);
}

PPI_Bidir_Toshiba_Read(h)
{
	int	Value;

	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Read_Ctrl | xcb[h].Data_Strobe | DIR_RESERVED);
	pe_delay(h,1000);
	Value = INB(xcb[h].Media_IO_Address,0);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return(Value);
}

PPI_Bidir_AT_Read(h)
{
	int	Value;

	OUTB(xcb[h].Media_IO_Address,0, 0xff);				/* setup for at style reads */
	pe_delay(h,1000);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Read_Ctrl | xcb[h].Data_Strobe);
	pe_delay(h,1000);
	Value = INB(xcb[h].Media_IO_Address,0);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	return(Value);
}

void PPI_Check_Delays(h)
{}

void PPI_Set_IRQ_Signal(h,Hi)
int Hi;
{
	OUTB(xcb[h].Media_IO_Address,0, GENREG0+PPI_FALLING_EDGES);
	pe_delay(h,1);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | xcb[h].Data_Strobe);	/* Reset RAS Ctrl */
	pe_delay(h,1);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	pe_delay(h,1);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	OUTB(xcb[h].Media_IO_Address,0, (Hi ? PPI_IRQ_ENABLE+PPI_IRQ_INVERT : 0) );
	pe_delay(h,1);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | xcb[h].Data_Strobe);
	pe_delay(h,1);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
}

/****************************************************************************************************************; */
/* PPI_Configure													; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	DS  = DATA_GROUP									; */
/*			BH  = PPI Ctrl Reg 1 value								; */
/*			CL  = PPI Ctrl Reg 2 value								; */
/*			CH  = PPI Ctrl Reg 3 value								; */
/*														; */
/*	Returns:	nothing											; */
/*	Destroys:	AX, DX											; */
/*														; */
/*	Description:	Configures the PPIEC control registers according to the arguments. This routine		; */
/*			leaves the register address set to General register #8 (dummy).				; */
/*---------------------------------------------------------------------------------------------------------------; */
void PPI_Configure(h,CR1, CR2, CR3)
int	CR1, CR2, CR3;
{
	OUTB(xcb[h].Media_IO_Address,0, GENREG8+PPI_FALLING_EDGES);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);

	OUTB(xcb[h].Media_IO_Address,0, CR1);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);

	OUTB(xcb[h].Media_IO_Address,0, CR2);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);

	OUTB(xcb[h].Media_IO_Address,0, CR3);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);

	if (CR2 & PPI_USE_STROBE) {
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_STROBE);
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
		xcb[h].Data_Strobe = PPI_STROBE;
	} else {
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RDS);
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
		xcb[h].Data_Strobe = PPI_RDS;
	}
}

void PPI_Write_Register(h,Reg, Val)
int	Reg, Val;
{
	OUTB(xcb[h].Media_IO_Address,0, Reg | PPI_RISING_EDGES);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	OUTB(xcb[h].Media_IO_Address,0, Val);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | xcb[h].Data_Strobe);
	pe_delay(h,1);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
}

PPI_Read_Register(h,Reg)
int	Reg;
{
	int	Hi_Nibble, Lo_Nibble;

	OUTB(xcb[h].Media_IO_Address,0, Reg | PPI_FALLING_EDGES+PPI_READ);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_RAS);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | xcb[h].Data_Strobe);
	pe_delay(h,1);
	Lo_Nibble = INB(xcb[h].Media_IO_Address,1);
	Lo_Nibble >>= 3;
	Lo_Nibble &= DQNT_MASK_LO;
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	pe_delay(h,1);
	Hi_Nibble = INB(xcb[h].Media_IO_Address,1) & DQNT_MASK_HI;
	return (Hi_Nibble | Lo_Nibble);
}

PPI_Control_Signals(h)
{
	int	i;

	PPI_Configure(h,0, PPI_USE_STROBE, 0);
	for (i = 0; i < 252; i++)
		if (PPI_Control_Line_Delay(h,i,xcb[h].Data_Strobe) == 0) break;
	if (i != 0) {
		PPI_Configure(h,0, 0, 0);
		for (i = 0; i < 252; i++)
			if (PPI_Control_Line_Delay(h,i,xcb[h].Data_Strobe) == 0) break;
		if (i == 0) return (0);
	}
	if (xcb[h].Data_Strobe == PPI_STROBE)   xcb[h].Hardware_Configuration |= FAST_STROBE_SIGNAL;
	else if (xcb[h].Data_Strobe == PPI_RDS) xcb[h].Hardware_Configuration |= FAST_AUTO_FEED_SIGNAL;
	return (0);
}

PPI_Control_Line_Delay(h,Count, Strobe)
int	Count, Strobe;
{
	int	i;

	PPI_Write_Register(h,GENREG4, PPI_COUNT_HYSTERESIS+PPI_COUNT_BOTH);
	PPI_Write_Register(h,GENREG6, 0);
	PPI_Write_Register(h,GENREG8, 0);
	for (i = 0; i < Count; i++)
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl ^ Strobe);
	i = PPI_Read_Register(h,GENREG6);
	if ( (i - (Count & 0xfe)) == 2) return (0);
	else return (-1);
}
