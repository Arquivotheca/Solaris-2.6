/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)pga.c	1.3	93/12/01 SMI"

#include "pe2ppt.c"
#include "peppt.c"
#include "eeppt.c"

#define SEL0			0x01
#define SEL1			0x02
#define SEL2			0x04
#define SEL3			0x08
#define HISEL7			0x10
#define EDGE			0x20
#define CLK10MHZ		0x40
#define BIDIR			0x80

#define SEL_D0			0x00
#define SEL_D1			0x01
#define SEL_D2			0x02
#define SEL_D3			0x03
#define SEL_D4			0x04
#define SEL_D5			0x05
#define SEL_D6			0x06
#define SEL_D7			0x07
#define SEL_STROBE		0x08
#define SEL_AFEED		0x09
#define SEL_SELECTIN		0x0a
#define SEL_STROBE_RAW		0x0b
#define SEL_AFEED_RAW		0x0c
#define SEL_SELECTIN_RAW	0x0d
#define SEL_D0_EPP		0x0e

/* Bit Stream Definitions */
#define DIN			AUTO_FEED				/* data in bit for pga programming */

/* Miscellanious Definitions */
#define WORK_COUNT		0x7c
#define MAX_DELAY		0x7f

/***************************************************************************************************************; */
/* Initialization Data Definitions										; */
/*--------------------------------------------------------------------------------------------------------------; */
unsigned char Signal_On_Values[]  = { 0x01, 0x02, 0x04, 0x0c, 0x10, 0x20, 0x40, 0x80, 0x0d, 0x07, 0x0e, 0x0d, 0x0e, 0x0e, 0x01 };
char Signal_Off_Values[] = { 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x05, 0x06, 0x0c, 0x0c, 0x06, 0x00 };
char Signal_LPT_Offsets[]= { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04 };

/****************************************************************************************************************; */
/* Initialization Code												; */
/*---------------------------------------------------------------------------------------------------------------; */

void Prepare_Signal();
void Work_Signal();
void Setup_Delay();
void PGA_Set_IRQ_Signal();
void Download_Bit_Stream();
void Bidir_Prepare_Signal();

PE1_Check_Adapter(h)
{	return (PGA_Check_Adapter(h,PE_Bit_Stream, sizeof(PE_Bit_Stream)));
}

PE2_Check_Adapter(h)
{	return (PGA_Check_Adapter(h,PE2_Bit_Stream, sizeof(PE2_Bit_Stream)));
}

EE_Check_Adapter(h)
{	return (PGA_Check_Adapter(h,EE_Bit_Stream, sizeof(EE_Bit_Stream)));
}

PGA_Check_Adapter(h,Bit_Stream, Bit_Stream_Len)
char   *Bit_Stream;
int	Bit_Stream_Len;
{
	Download_Bit_Stream(h,Bit_Stream, Bit_Stream_Len);
	Prepare_Signal(h,SEL_D0);
	if (Read_Counter(h) != 0)
		return(-1);
	Work_Signal(h,SEL_D0, MAX_DELAY);
	if (Read_Counter(h) != WORK_COUNT)
		return(-1);
	else	return(0);
}

void PGA_Profile_LPT_Port(h)
{
	PS2_Setup(h);
	xcb[h].Hardware_Configuration |= NON_BIDIRECTIONAL_MODE;
	xcb[h].Hardware_Configuration |= Check_Bidir(h);
	if (Check_PS2_Writes(h) == 0)	xcb[h].Hardware_Configuration |= EWRITE_MODE;
	if (Check_EPP(h) == 0)		xcb[h].Hardware_Configuration |= EPP_MODE;
	if (Check_LPT_Delays(h) == 0)	xcb[h].Hardware_Configuration |= DELAYS_ARE_REQUIRED;
}

Check_Bidir(h)
{
	int Mode = 0;

	Bidir_Prepare_Signal(h,SEL_D0);
	Work_Signal(h,SEL_D0, MAX_DELAY);
	OUTB(xcb[h].Media_IO_Address,0, 1);					/* 1 extra count to make d0 */
								/* hi since we have pull ups */
								/* when we turn the bus around */

	if (Check_Compaq_Signature(h) == 0)
		if (Bidir_Compaq_Read(h) == WORK_COUNT+1)
			Mode = COMPAQ_MODE+BIDIRECTIONAL_MODE;
	if (Mode == 0 && (Bidir_Buffered_Read(h) == WORK_COUNT+1) )
		Mode = BIDIRECTIONAL_MODE;
	if (Mode == 0 && (Bidir_Toshiba_Read(h) == WORK_COUNT+1) )
		Mode = TOSHIBA_MODE+BIDIRECTIONAL_MODE;
	if (Mode == 0 && (Bidir_AT_Read(h) == WORK_COUNT+1) )
		Mode = AT_MODE+BIDIRECTIONAL_MODE;
	Prepare_Signal(h,SEL_D0);
	return(Mode);
}

Check_PS2_Writes(h)
{
	Prepare_Signal(h,SEL_STROBE);
	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET | LPT_STROBE);
	Work_Signal(h,SEL_D0, MAX_DELAY);
	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET);
	if (Read_Counter(h) == (2*WORK_COUNT)+1)
		return(0);
	else
		return(-1);
}

Check_EPP(h)
{
	Prepare_Signal(h,SEL_AFEED);
	if (Enable_EPP_Mode(h,5) != 0)
		return(-1);
	Work_Signal(h,SEL_D0_EPP, MAX_DELAY);
	Disable_EPP_Mode(h);
	if (Read_Counter(h) == (2*WORK_COUNT) )
		return(0);
	else	return(-1);
}

Bidir_Compaq_Read(h)
{
	int Val;

	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET+DIR_CTRL);
	OUTB(CPQ_DIR_PORT,0, CPQ_READ_ENB);
	Val = INB(xcb[h].Media_IO_Address,0);
	OUTB(CPQ_DIR_PORT,0, CPQ_WRITE_ENB);
	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET+SELECT_IN);
	return(Val);
}

Bidir_Buffered_Read(h)
{
	int Val;

	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET+DIR_CTRL);
	pe_delay(h,10);
	Val = INB(xcb[h].Media_IO_Address,0);
	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET+SELECT_IN);
	return(Val);
}

Bidir_Toshiba_Read(h)
{
	int Val;

	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET+DIR_CTRL+DIR_RESERVED);
	pe_delay(h,10);
	Val = INB(xcb[h].Media_IO_Address,0);
	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET+SELECT_IN);
	return(Val);
}

Bidir_AT_Read(h)
{
	int Val;

	OUTB(xcb[h].Media_IO_Address,0, 0xff);
	pe_delay(h,10);
	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET);
	pe_delay(h,10);
	Val = INB(xcb[h].Media_IO_Address,0);
	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET+SELECT_IN);
	return(Val);
}

Check_LPT_Delays(h)
{
	int i, Delay = 0;

	for (i = EDGE; i < EDGE+14; i++) {
		Prepare_Signal(h,i);
		for ( ; Delay < MAX_DELAY; Delay++) {
			Work_Signal(h,i, Delay);
			if (Read_Counter(h) == 2*WORK_COUNT)
				break;
		}
		if (Delay >= MAX_DELAY)
			return(-1);
	}
	return 0;
}

void Prepare_Signal(h,Signal)
int Signal;
{

	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET | LPT_STROBE | AUTO_FEED | SELECT_IN);
	pe_delay(h,100);
	OUTB(xcb[h].Media_IO_Address,0, Signal | HISEL7);
	pe_delay(h,100);
	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET);
	Signal &= SEL0 | SEL1 | SEL2 | SEL3;
	OUTB(xcb[h].Media_IO_Address , Signal_LPT_Offsets[Signal], Signal_Off_Values[Signal]);
	pe_delay(h,100);
}

void Bidir_Prepare_Signal(h,Signal)
int Signal;
{

	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET | LPT_STROBE | AUTO_FEED | SELECT_IN);
	pe_delay(h,100);
	OUTB(xcb[h].Media_IO_Address,0, Signal | HISEL7 | BIDIR | CLK10MHZ);
	pe_delay(h,100);
	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET | SELECT_IN);
	pe_delay(h,100);
	Signal &= SEL0 | SEL1 | SEL2 | SEL3;
	OUTB(xcb[h].Media_IO_Address , Signal_LPT_Offsets[Signal], Signal_Off_Values[Signal]);
	pe_delay(h,100);
}

void Work_Signal(h,Signal, Delay)
int Signal, Delay;
{
	int	i, On, Off;

	Setup_Delay(h,Delay);
	Signal &= SEL0 | SEL1 | SEL2 | SEL3;
	On = Signal_On_Values[Signal];
	Off= Signal_Off_Values[Signal];
	if (Delay)
		for (i = 0; i < WORK_COUNT/2; i++) {
			OUTB(xcb[h].Media_IO_Address,Signal_LPT_Offsets[Signal], On);
			Programmable_Delay(h);
			OUTB(xcb[h].Media_IO_Address,Signal_LPT_Offsets[Signal], Off);
			Programmable_Delay(h);
			OUTB(xcb[h].Media_IO_Address,Signal_LPT_Offsets[Signal], On);
			Programmable_Delay(h);
			OUTB(xcb[h].Media_IO_Address,Signal_LPT_Offsets[Signal], Off);
			Programmable_Delay(h);
		}
	else
		for (i = 0; i < WORK_COUNT/4; i++) {
			OUTB(xcb[h].Media_IO_Address,Signal_LPT_Offsets[Signal], On);
			OUTB(xcb[h].Media_IO_Address,Signal_LPT_Offsets[Signal], Off);
			OUTB(xcb[h].Media_IO_Address,Signal_LPT_Offsets[Signal], On);
			OUTB(xcb[h].Media_IO_Address,Signal_LPT_Offsets[Signal], Off);
			OUTB(xcb[h].Media_IO_Address,Signal_LPT_Offsets[Signal], On);
			OUTB(xcb[h].Media_IO_Address,Signal_LPT_Offsets[Signal], Off);
			OUTB(xcb[h].Media_IO_Address,Signal_LPT_Offsets[Signal], On);
			OUTB(xcb[h].Media_IO_Address,Signal_LPT_Offsets[Signal], Off);
		}
}

void Setup_Delay(h,Delay)
int Delay;
{
	int i;

	return;

#ifdef NOTPORTABLE
	for (i = 0; i < 0x100; i ++)
		((char *)Programmable_Delay)[i] = 0x90;		/* NOP opcode */
	if (Delay)
		Delay--;
	((char *)Programmable_Delay)[Delay] = 0xc3;
#endif
}

Read_Counter(h)
{
	int Hi, Lo;

	Lo = (INB(xcb[h].Media_IO_Address,1) >> 3) & 0x0f;
	OUTB(xcb[h].Media_IO_Address,0, 0x80);				/* select high nibble */
	Hi = (INB(xcb[h].Media_IO_Address,1) << 1) & 0xf0;
	return(Hi | Lo);
}

void PGA_Set_IRQ_Signal(h,Polarity)
int Polarity;
{
	int	i;

	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | LPT_STROBE | AUTO_FEED | SELECT_IN);
	pe_delay(h,10);
	OUTB(xcb[h].Media_IO_Address,0, SEL_D0 | EDGE | HISEL7 | SELECT_IN);
	pe_delay(h,10);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	pe_delay(h,10);
	OUTB(xcb[h].Media_IO_Address,0, 0);
	pe_delay(h,10);					/* IRQ is now off */
	if (Polarity)
		for (i = 0; i < 4; i++) {
			OUTB(xcb[h].Media_IO_Address,0, 1);
			pe_delay(h,10);
			OUTB(xcb[h].Media_IO_Address,0, 0);
			pe_delay(h,10);
		}
}

void Download_Bit_Stream(h,Start, Len)
char	*Start;
int	Len;
{
	int	i, Value;

	OUTB(xcb[h].Media_IO_Address,0, 0);
	OUTB(xcb[h].Media_IO_Address,2, 0);
	pe_delay(h,6);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	pe_delay(h,500);

	for ( ; Len; Start++, Len--)
		for (i = 0; i < 8; i++) {
			Value = xcb[h].LPT_Write_Ctrl | ( ( (*Start << i) & 0x80) ? 0 : DIN);
			OUTB(xcb[h].Media_IO_Address,2, Value);
			OUTB(xcb[h].Media_IO_Address,2, Value | SELECT_IN);
			OUTB(xcb[h].Media_IO_Address,2, Value);
		}
}

