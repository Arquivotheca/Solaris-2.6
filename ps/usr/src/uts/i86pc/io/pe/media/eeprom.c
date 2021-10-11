/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)eeprom.c	1.2	93/11/02 SMI"

static void Microsec_Delay();
static void Cleanup();
static void Out();
static void Setup();
static void Command();
static void Write_Wait();
static void Write_EEPROM();

/* Commands */
#define	EE_READ		0x80					/* read command  (0 - 3 = address) */
#define	EE_WRITE	0x40					/* write command (0 - 3 = address) */
#define	EE_ERASE	0xc0					/* erase command (0 - 3 = address) */
#define	EE_EWEN		0x30					/* erase/write enable */
#define	EE_EWDS		0x00					/* erase/write disable */
#define	EE_ERAL		0x20					/* erase all command */
#define	EE_WRAL		0x10					/* write all command */
 
/*extern	EEPROM_Put_Bit(),					// Strobes one bit into EEPROM */
/*	EEPROM_Get_Bit(),					// Reads one bit from EEPROM */
/*	EEPROM_Disable(),					// Disables EEPROM (deasserts Chip Select) */
/*	EEPROM_Enable();					// Enables EEPROM (asserts Chip Select) */

static Read_EEPROM(h, Buffer)
char *Buffer;
{
	int		i, j; 
	unsigned short	CheckSum = 0;
	unsigned short	Word;

	for (i = 16; i; i--) {
		Setup(h);
		Out(h,(unsigned char)EE_READ | (i-1));
		for (j = 0, Word = 0; j < 16; j++) {
			xcb[h].EEPROM_Put_Bit(h,0);
			Word <<= 1;
			Word |= xcb[h].EEPROM_Get_Bit(h);
		}
		CheckSum += Word;
#ifdef MSC
		(unsigned int)*Buffer = Word;
#else
		*(unsigned short *)Buffer = Word;
#endif
		Buffer += 2;
		xcb[h].EEPROM_Put_Bit(h,0);
	}
	return CheckSum;
}

static void Write_EEPROM(h, Buffer)
char *Buffer;
{
	int		i, CheckSum = 0;
	unsigned	Word;

	for (i = 0; i < 16; i++) {
		Command(h,EE_EWEN);
		Command(h,EE_ERAL);
		Write_Wait(h);
		Setup(h);
		Command(h,EE_WRITE);
		CheckSum += (unsigned)Buffer[i*2];
		Word = (unsigned)Buffer[i*2];
		if (i == 15)
			Word -= CheckSum;
		Out(h,Word >> 8);				/* Hi byte */
		Out(h,Word);				/* Lo byte */
		Cleanup(h);
		Write_Wait(h);
	}
	Command(h, EE_EWDS);
}

static void Write_Wait(h)
{
	Microsec_Delay(h, 5000);
	Setup(h);
	Cleanup(h);
}

static void Command(h, Cmd)
int Cmd;
{
	Setup(h);
	Out(h, (unsigned char) Cmd);
	Cleanup(h);
}

static void Setup(h)
{
	xcb[h].EEPROM_Disable(h);
	xcb[h].EEPROM_Put_Bit(h, 0);
	xcb[h].EEPROM_Enable(h);
	xcb[h].EEPROM_Put_Bit(h, 0);
	xcb[h].EEPROM_Put_Bit(h, 1);
}

static void Out(h, Byte)
unsigned char Byte;
{
	int i;

	for (i = 0; i < 8; i++)
		xcb[h].EEPROM_Put_Bit(h, (int) (Byte >> (7 - i)) & 0x01);
}

static void Cleanup(h)
{
	xcb[h].EEPROM_Disable(h);
	xcb[h].EEPROM_Put_Bit(h, 0);
}

static void Microsec_Delay(h, ticks)
{
	int	i;

	for (i = 0 ; i < 1696; i++) {
		if (i) {
			i++;
			i--;
		}
	}
}
