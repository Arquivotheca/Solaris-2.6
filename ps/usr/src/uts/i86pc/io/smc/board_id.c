/*
 * Copyright (c) 1992, 1993, by Sun Microsystems, Inc.  All rights reserved.
 */

#pragma ident	"@(#)board_id.c	1.9	94/11/15 SMI"
/*
 * This module provides an algorithm for identifying the type of Western Digital
 * LAN Adapter being used e.g. WD8003E, WD8003ST/A, etc.
 *
 */

#include	"sys/types.h"
#include	"sys/ddi.h"
#include	"sys/sunddi.h"
#include	"sys/stream.h"
#include	"sys/ethernet.h"
#include	"sys/smc.h"
#include	"sys/smcboard.h"
#include	"sys/smchdw.h"

/* ---- LOCAL PROTOTYPES ---- */
u_long		GetBoardID(int, int);
static u_long	bid_get_base_info(int, u_long, int);
static u_long	bid_get_media_type(int, int);
static int	bid_has_interface_chip(int);
static int	bid_register_aliasing(int);
static int	bid_board_sixteen_bit(int, int);
static int	bid_slot_sixteen_bit(int);
static int	bid_check_for_690(int, int);
static int	bid_get_board_rev_number(int);
static u_long	bid_get_id_byte_info(int, u_long);
static u_long	bid_get_eeprom_info(int);
static void	bid_recall_engr_eeprom(int);
static void	bid_recall_lan_address(int);
static void	bid_wait_for_recall(int);
static void	bid_do_nothing();
static u_long	bid_get_ram_size(int, int, u_long);
static int	bid_check_for_585(int);
/* ---- END OF LOCAL PROTOTYPES ---- */
#define	BID_FALSE	0
#define	BID_TRUE	1

/*
 * Register offset definitions...since different boards have different names
 * for register offsets 0x00-0x07, generic names will be assigned
 */
#define	BID_REG_0		0x00
#define	BID_REG_1		0x01
#define	BID_REG_2		0x02
#define	BID_REG_3		0x03
#define	BID_REG_4		0x04
#define	BID_REG_5		0x05
#define	BID_REG_6		0x06
#define	BID_REG_7		0x07

/*
 * Register offset definitions...the names for this registers are consistant
 * across all boards, so specific names will be assigned
 */
#define	BID_LAN_ADDR_0		0x08	/* these 6 registers hold the */
#define	BID_LAN_ADDR_1		0x09	/* LAN address for this node */
#define	BID_LAN_ADDR_2		0x0A
#define	BID_LAN_ADDR_3		0x0B
#define	BID_LAN_ADDR_4		0x0C
#define	BID_LAN_ADDR_5		0x0D
#define	BID_BOARD_ID_BYTE	0x0E	/* identification byte for WD boards */
#define	BID_CHCKSM_BYTE		0x0F	/* the address ROM checksum byte */

/* Masks bits for the board revision number in the BID_BOARD_ID_BYTE */
#define	BID_BOARD_REV_MASK	0x1E

/* Misc. definitions */
#define	BID_MSZ_583_BIT		0x08	/* memory size bit in 583 */
#define	BID_SIXTEEN_BIT_BIT	0x01	/* bit has 16 bit capability info */

/* Defs for board rev numbers greater than 1 */
#define	BID_MEDIA_TYPE_BIT	0x01
#define	BID_SOFT_CONFIG_BIT	0x20
#define	BID_RAM_SIZE_BIT	0x40
#define	BID_BUS_TYPE_BIT	0x80

/* defs for identifying the 690 */
#define	BID_CR		0x10	/* Command Register	 */
#define	BID_TXP		0x04	/* transmit packet */
#define	BID_TCR		0x1D	/* Transmit Configuration Register */
#define	BID_TCR_VAL	0x18	/* test value for 690 or 8390 */
#define	BID_PS0		0x00	/* register page select - 0 */
#define	BID_PS1		0x40	/* register page select - 1 */
#define	BID_PS2		0x80	/* register page select - 2 */
#define	BID_PS_MASK	0x3F	/* to mask off page select bits */

/* Register offsets for reading the EEPROM in the 584 chip */
#define	BID_EEPROM_0		0x08
#define	BID_EEPROM_1		0x09
#define	BID_EEPROM_2		0x0A
#define	BID_EEPROM_3		0x0B
#define	BID_EEPROM_4		0x0C
#define	BID_EEPROM_5		0x0D
#define	BID_EEPROM_6		0x0E
#define	BID_EEPROM_7		0x0F

/* defs for manipulating the 584 */
#define	BID_OTHER_BIT			0x02
#define	BID_ICR_MASK			0x0C
#define	BID_EAR_MASK			0x0F
#define	BID_ENGR_PAGE			0xA0
#define	BID_RLA				0x10
#define	BID_EA6				0x80
#define	BID_RECALL_DONE_MASK		0x10
#define	BID_EEPROM_MEDIA_MASK		0x07
#define	BID_STARLAN_TYPE		0x00
#define	BID_ETHERNET_TYPE		0x01
#define	BID_TP_TYPE			0x02
#define	BID_EW_TYPE			0x03
#define	BID_EEPROM_IRQ_MASK		0x18
#define	BID_PRIMARY_IRQ			0x00
#define	BID_ALTERNATE_IRQ_1		0x08
#define	BID_ALTERNATE_IRQ_2		0x10
#define	BID_ALTERNATE_IRQ_3		0x18
#define	BID_EEPROM_RAM_SIZE_MASK	0xE0
#define	BID_EEPROM_RAM_SIZE_RES1	0x00
#define	BID_EEPROM_RAM_SIZE_RES2	0x20
#define	BID_EEPROM_RAM_SIZE_8K		0x40
#define	BID_EEPROM_RAM_SIZE_16K		0x60
#define	BID_EEPROM_RAM_SIZE_32K		0x80
#define	BID_EEPROM_RAM_SIZE_64K		0xA0
#define	BID_EEPROM_RAM_SIZE_RES3	0xC0
#define	BID_EEPROM_RAM_SIZE_RES4	0xE0
#define	BID_EEPROM_BUS_TYPE_MASK	0x07
#define	BID_EEPROM_BUS_TYPE_AT		0x00
#define	BID_EEPROM_BUS_TYPE_MCA		0x01
#define	BID_EEPROM_BUS_TYPE_EISA	0x02
#define	BID_EEPROM_BUS_SIZE_MASK	0x18
#define	BID_EEPROM_BUS_SIZE_8BIT	0x00
#define	BID_EEPROM_BUS_SIZE_16BIT	0x08
#define	BID_EEPROM_BUS_SIZE_32BIT	0x10
#define	BID_EEPROM_BUS_SIZE_64BIT	0x18

#define	inp(port)	inb(port)
#define	outp(port, val)	outb(port, val)

/*
 * this is the main level routine for finding the board type
 *
 * ENTER:	parameter1 = base I/O address of LAN adapter board
 *		parameter2 = micro
 * channel flag = 1 if micro channel = 0 if AT
 *
 * RETURN:	long value defining the boards ID
 *
 */
unsigned long
GetBoardID(base_io, machine_flag)
	int		base_io; /* base I/O address of LAN adapter board */
	int		machine_flag;
{
	unsigned long	board_id;	/* holds return value */
	int		board_rev_number;

	board_id = 0;
	board_rev_number = bid_get_board_rev_number(base_io);
	if (!(board_rev_number))
		return (0);	/* it must be an 8000 board */
	if (machine_flag)
		board_id |= MICROCHANNEL;
	board_id |= bid_get_base_info(base_io, board_id, board_rev_number);
	board_id |= bid_get_media_type(base_io, board_rev_number);
	if (board_rev_number == 2)
		board_id |= bid_get_id_byte_info(base_io, board_id);
	if (board_rev_number >= 3) {
		board_id &= (long) ~MEDIA_MASK;	/* remove media stuff */
		if (bid_check_for_585(base_io))
			board_id |= INTERFACE_585_CHIP;
		else
			board_id |= INTERFACE_584_CHIP;
		board_id |= bid_get_eeprom_info(base_io);
	} else
		board_id |= bid_get_ram_size(base_io, board_rev_number,
			board_id);

	if (board_rev_number >= 4) {
		unsigned short  RegValue;

		board_id |= ADVANCED_FEATURES;
		if (board_id & INTERFACE_585_CHIP) {
			RegValue = inp(base_io + WD_REG_7) >> 4;
			board_id &= (~INTERFACE_CHIP_MASK);

			if (RegValue == 2) {
				board_id |= NIC_790_BIT + INTERFACE_585_CHIP;
				return (board_id);
			} else if (RegValue == 3) {
				board_id |= INTERFACE_585_CHIP;
			} else if (RegValue == 4) {		/* smc [1.01] */
				board_id |= NIC_790_BIT + INTERFACE_585_CHIP
				    + NIC_SUPERSET;
				return (board_id);
			}
		}
	}
	if (bid_check_for_690(base_io, board_rev_number))
		board_id |= NIC_690_BIT;
	return (board_id);
}

static u_long
bid_get_base_info(address, current_id, rev_number)
	int		address;
	u_long		current_id;
	int		rev_number;
{
	u_long		base_bits;

	base_bits = 0;
	if (bid_check_for_585(address)) {
		base_bits |= INTERFACE_CHIP;

		/* smc [1.01] */
		base_bits |= BOARD_16BIT;
		if (bid_slot_sixteen_bit(address))
			base_bits |= SLOT_16BIT;
		return (base_bits);
	}
	if (current_id & MICROCHANNEL) {
		if (bid_has_interface_chip(address))
			base_bits |= INTERFACE_CHIP;
		return (base_bits);
	} else {
		if (bid_register_aliasing(address))
			return (base_bits);
		if (bid_has_interface_chip(address)) {
			base_bits |= INTERFACE_CHIP;
		} else if (bid_board_sixteen_bit(address, rev_number)) {
			base_bits |= BOARD_16BIT;
			if (bid_slot_sixteen_bit(address))
				base_bits |= SLOT_16BIT;
			return (base_bits);
		}
	}
	return (base_bits);	/* return the proper board ID */
}

/*
 * this returns the Media Type
 */
static unsigned long
bid_get_media_type(address, rev_number)
	int		address;
	int		rev_number;
{
	if (inp(address + BID_BOARD_ID_BYTE) & BID_MEDIA_TYPE_BIT)
		return (ETHERNET_MEDIA);
	else {
		if (rev_number == 1)
			return (STARLAN_MEDIA);
		else
			return (TWISTED_PAIR_MEDIA);
	}
}

/*
 * this checks to see if an interface chip is present
 * (i.e. WD83C583 or WD83C593)
 *	RETURNS:	1 if present
 *			0 if not
 */
static int
bid_has_interface_chip(address)
	int		address;
{
	int		ret_val;
	int		temp_val;

	temp_val = inp(address + BID_REG_7);	/* save old value */
	outp(address + BID_REG_7, 0x35);	/* write general purpose reg */
	inp(address + BID_REG_0);	/* put something else on bus */
	if ((inp(address + BID_REG_7) & 0xFF) != 0x35)	/* if it didn't stick */
		ret_val = BID_FALSE;	/* then show no chip */
	else {
		outp(address + BID_REG_7, 0x3A);	/* try another value */
		inp(address + BID_REG_0);	/* put something else on bus */
		if ((inp(address + BID_REG_7) & 0xFF) != 0x3A)
			ret_val = BID_FALSE;	/* then show no chip */
		else
			ret_val = BID_TRUE;	/* both stuck, show true */
	}
	if (ret_val)		/* if a chip is there */
		outp(address + BID_REG_7, temp_val);
	/* otherwise, we dont know what a restore will do */
	/* could read 0xFF and alias to REG_0 on write */
	return (ret_val);
}

/*
 * this checks to see if the board has register aliasing
 *	that is, reading from register offsets 0-7 will return the contents
 *	of register offsets 8-F
 */
static int
bid_register_aliasing(address)
	int		address;
{
	int		i;

	for (i = 0; i < 4; i++) {
		if ((inp(address + BID_REG_1 + i) & 0xFF) !=
		    (inp(address + BID_LAN_ADDR_1 + i) & 0xFF)) {
			return (BID_FALSE);
		}
	}
	if ((inp(address + BID_REG_7) & 0xFF) !=
	    (inp(address + BID_CHCKSM_BYTE) & 0xFF)) {
		return (BID_FALSE);
	}
	return (BID_TRUE);	/* all were equal...show aliasing true */
}

/*
 * this checks to see if the board is capable of 16 bit memory moves
 *	it does this by checking bit zero of register one
 *	if this bit is unchangable by software, then the card has 16 bit
 *	capability
 *	RETURNS:	1 if capable of 16 bit moves
 *			0 if not
 */
static int
bid_board_sixteen_bit(address, rev_number)
	int		address;
	int		rev_number;
{
	int		ret_val;
	int		register_hold;

	if (rev_number < 3)
		return (0);

	/* read previous contents */
	register_hold = (inp(address + BID_REG_1) & 0xFF);

	/* write back what was read with bit zero flipped */
	outp(address + BID_REG_1, register_hold ^ BID_SIXTEEN_BIT_BIT);
	inp(address + BID_REG_0);	/* put something else on bus */

	/*
	 * if we changed the value, write it back and show no 16 bit
	 * capability, else show 16 bit capability
	 */
	if ((register_hold & BID_SIXTEEN_BIT_BIT) ==
	    (inp(address + BID_REG_1) & BID_SIXTEEN_BIT_BIT)) {
		ret_val = 1;	/* we did not change it */
		/* if sixteen bit board, always reset bit one */
		register_hold &= 0xFE;
	} else
		ret_val = 0;
	outp(address + BID_REG_1, register_hold);
	return (ret_val);
}

/*
 * this finds out if the 16 bit capable board is plugged in a 16 bit slot
 *	it does this by reading bit 0 of the register one,
 *	if the value is 1 => 16 bit board in 16 bit slot
 *	else not
 *	RETURNS:	1 if 16 bit board in 16 bit slot
 *			0 if not
 */
static int
bid_slot_sixteen_bit(address)
	int		address;
{
	if (bid_check_for_585(address)) {
		if (inp(address + BID_REG_4) & HWR_HOST16)
			return (BID_TRUE);
		else
			return (BID_FALSE);
	}
	if (inp(address + BID_REG_1) & BID_SIXTEEN_BIT_BIT)
		return (BID_TRUE);
	else
		return (BID_FALSE);
}

/*
 * this finds out if the board is using a 690 and not an 8390 NIC
 *	it tests the multicast address registers
 *	the 690 will not write these registers, but the 8390 will
 *
 *	RETURNS:	1 if using 690
 *			0 if using 8390
 */
static int
bid_check_for_690(address, rev_number)
	int		address;
	int		rev_number;
{
	int		temp_CR, work_CR, temp_TCR;
	int		ret_val;

	ret_val = BID_FALSE;
	if (rev_number < 3)
		return (ret_val);
	/* dont maintain TX bit */
	temp_CR = ((inp(address + BID_CR) & ~BID_TXP) & 0xFF);
	work_CR = temp_CR & BID_PS_MASK;	/* mask off pg sel bits */
	outp(address + BID_CR, work_CR | BID_PS2);	/* select page 2 */
	temp_TCR = inp(address + BID_TCR);	/* save orig TCR */

	outp(address + BID_CR, work_CR);	/* select page 0 */
	outp(address + BID_TCR, BID_TCR_VAL);	/* write test value */
	outp(address + BID_CR, work_CR | BID_PS2);	/* select page 2 */
	if ((inp(address + BID_TCR) & BID_TCR_VAL) == BID_TCR_VAL)
		ret_val = BID_FALSE;	/* show 8390 */
	else
		ret_val = BID_TRUE;	/* show 690 */

	outp(address + BID_CR, work_CR);	/* select page 0 */
	outp(address + BID_TCR, temp_TCR);	/* put back original value */
	outp(address + BID_CR, temp_CR);	/* put back original value */
	return (ret_val);
}

/*
 * this returns the board revision number found in the board id byte
 * of the lan address ROM
 *	first it will read the value of that register,
 *	then it will mask off the relavent bits,
 *	and finally shift them into a 0 justified position
 */
static int
bid_get_board_rev_number(int address)
{
	return ((int)(inp(address + BID_BOARD_ID_BYTE) & BID_BOARD_REV_MASK)
	    >> 1);
}

/*
 * this gets the extra information from the ID Byte
 * the extra info is based on the revision number and also the current board id
 */
static unsigned long
bid_get_id_byte_info(address, current_id)
	int		address;
	u_long		current_id;
{
	int		temp;
	u_long		temp_id;
	u_long		extra_bits;

	extra_bits = 0;
	temp = inp(address + BID_BOARD_ID_BYTE);
	if (temp & BID_SOFT_CONFIG_BIT) {
		temp_id = current_id & STATIC_ID_MASK;
		if ((temp_id == WD8003EB) || (temp_id == WD8003W))
			extra_bits |= ALTERNATE_IRQ_BIT;
	}
	return (extra_bits);
}

/*
 * this gets the extra information from the EEPROM in the 584
 */
static u_long
bid_get_eeprom_info(int address)
{
	u_long		new_bits;
	int		temp;
	u_char		RegVal;

	new_bits = 0;
	bid_recall_engr_eeprom(address);
	temp = inp(address + BID_EEPROM_1);
	switch (temp & BID_EEPROM_BUS_TYPE_MASK) {
	case BID_EEPROM_BUS_TYPE_AT:
		break;
	case BID_EEPROM_BUS_TYPE_MCA:
		new_bits |= MICROCHANNEL;
		break;
	default:
		break;
	}

	RegVal = temp & BID_EEPROM_PAGING_MASK;
	if (RegVal & BID_EEPROM_RAM_PAGING) {
		new_bits |= PAGED_RAM;
	}
	if (RegVal & BID_EEPROM_ROM_PAGING) {
		new_bits |= PAGED_ROM;
	}
	switch (temp & BID_EEPROM_BUS_SIZE_MASK) {
	case BID_EEPROM_BUS_SIZE_8BIT:
		break;
	case BID_EEPROM_BUS_SIZE_16BIT:
		new_bits |= BOARD_16BIT;
		if (bid_slot_sixteen_bit(address))
			new_bits |= SLOT_16BIT;
		break;
	default:
		break;
	}
	temp = inp(address + BID_EEPROM_0);
	switch (temp & BID_EEPROM_MEDIA_MASK) {
	case BID_STARLAN_TYPE:
		new_bits |= STARLAN_MEDIA;
		break;
	case BID_ETHERNET_TYPE:
		new_bits |= ETHERNET_MEDIA;
		break;
	case BID_TP_TYPE:
		new_bits |= TWISTED_PAIR_MEDIA;
		break;
	case BID_EW_TYPE:
		new_bits |= EW_MEDIA;
		break;
	default:
		new_bits |= ETHERNET_MEDIA;
		break;
	}
	switch (temp & BID_EEPROM_IRQ_MASK) {
	case BID_ALTERNATE_IRQ_1:
		new_bits |= ALTERNATE_IRQ_BIT;
		break;
	default:
		break;
	}
	switch (temp & BID_EEPROM_RAM_SIZE_MASK) {
	case BID_EEPROM_RAM_SIZE_8K:
		new_bits |= RAM_SIZE_8K;
		break;
	case BID_EEPROM_RAM_SIZE_16K:
		if ((new_bits & BOARD_16BIT) &&
		    (!(new_bits & SLOT_16BIT)))
			new_bits |= RAM_SIZE_8K;
		else
			new_bits |= RAM_SIZE_16K;
		break;
	case BID_EEPROM_RAM_SIZE_32K:
		new_bits |= RAM_SIZE_32K;
		break;
	case BID_EEPROM_RAM_SIZE_64K:
		if ((new_bits & BOARD_16BIT) &&
		    (!(new_bits & SLOT_16BIT)))
			new_bits |= RAM_SIZE_32K;
		else
			new_bits |= RAM_SIZE_64K;
		break;
	default:
		new_bits |= RAM_SIZE_UNKNOWN;
		break;
	}

	/* smc [1.01] */
	temp = inp(address+BID_EEPROM_3);
	if (temp & BID_EEPROM_LITE)
		new_bits |= LITE_VERSION;

	bid_recall_lan_address(address);
	return (new_bits);
}

static void
bid_recall_engr_eeprom(address)
	int		address;
{
	if (bid_check_for_585(address)) {
		unsigned char   RegValue;

		RegValue = 0x0A | 0x40;
		outp(address + BID_REG_1, RegValue);
		while (RegValue & 0x40) {
			RegValue = inp(address + BID_REG_1);
		}
	} else {
		outp(address + BID_REG_1,
		((inp(address + BID_REG_1) & BID_ICR_MASK) | BID_OTHER_BIT));
		outp(address + BID_REG_3,
		((inp(address + BID_REG_3) & BID_EAR_MASK) | BID_ENGR_PAGE));
		outp(address + BID_REG_1,
			((inp(address + BID_REG_1) & BID_ICR_MASK) |
			(BID_RLA | BID_OTHER_BIT)));
		bid_wait_for_recall(address);
	}
}

/*
 * this recalls the LAN Address ROM
 *	it reads register offset 1, masks out all but MSZ and IR2,
 *	resets the 'other' bit, and sets the RLA bit
 *	then it waits for the recall operation to complete
 */
static void
bid_recall_lan_address(address)
	int		address;
{
	if (bid_check_for_585(address)) {
		unsigned char   RegValue;

		RegValue = inp(address + BID_REG_4) & 0x43;
		outp(address + BID_REG_4, RegValue);
		RegValue = 0x06 | 0x40;
		outp(address + BID_REG_1, RegValue);
		while (RegValue & 0x40) {
			RegValue = inp(address + BID_REG_1);
		}
	} else {
		outp(address + BID_REG_1,
		((inp(address + BID_REG_1) & BID_ICR_MASK) | BID_OTHER_BIT));
		outp(address + BID_REG_3,
			((inp(address + BID_REG_3) & BID_EAR_MASK) | BID_EA6));
		outp(address + BID_REG_1,
			((inp(address + BID_REG_1) & BID_ICR_MASK) | BID_RLA));
		bid_wait_for_recall(address);
	}
}

static void
bid_wait_for_recall(address)
	int		address;
{
	while (inp(address + BID_REG_1) & BID_RECALL_DONE_MASK) {
		bid_do_nothing();
	}
}

static void
bid_do_nothing()
{
}

/*
 * this gets the size of the RAM on board the LAN adapter
 *	could return an 'I dont know' answer
 */
static unsigned long
bid_get_ram_size(address, rev_number, current_id)
	int		address;
	int		rev_number;
	u_long		current_id;
{
	int		large_ram_flag;

	if (rev_number < 2) {	/* if pre-board rev */
		if (current_id & BOARD_16BIT) {
			if (current_id & SLOT_16BIT)
				return (RAM_SIZE_16K);
			else
				return (RAM_SIZE_8K);
		}
		if (current_id & MICROCHANNEL)
			return (RAM_SIZE_16K);
		if (current_id & INTERFACE_CHIP) {
			if (inp(address + BID_REG_1) & BID_MSZ_583_BIT)
				return (RAM_SIZE_32K);
			else
				return (RAM_SIZE_8K);
		}
		return (RAM_SIZE_UNKNOWN);	/* default for old boards */
	}
	if (rev_number == 2) {
		large_ram_flag = inp(address + BID_BOARD_ID_BYTE) &
			BID_RAM_SIZE_BIT;
		switch ((int) (current_id & STATIC_ID_MASK)) {
		case WD8003E:
		case WD8003S:
		case WD8003WT:
		case WD8003W:
		case WD8003EB:
			if (large_ram_flag)
				return (RAM_SIZE_32K);
			else
				return (RAM_SIZE_8K);
		case WD8003ETA:
		case WD8003STA:
		case WD8003EA:
		case WD8003SHA:
		case WD8003WA:
			return (RAM_SIZE_16K);
		case WD8013EBT:
			if (current_id & SLOT_16BIT) {
				if (large_ram_flag)
					return (RAM_SIZE_64K);
				else
					return (RAM_SIZE_16K);
			} else {
				if (large_ram_flag)
					return (RAM_SIZE_32K);
				else
					return (RAM_SIZE_8K);
			}
		default:
			return (RAM_SIZE_UNKNOWN);
		}		/* switch */
	}			/* rev_number == 2 */
	if ((current_id & INTERFACE_CHIP_MASK) == INTERFACE_585_CHIP) {
		unsigned char   reg, reg1;
		unsigned long   sz;

		reg = inb(address + WD_REG_4);
		reg &= 0xc3;
		outb(address + WD_REG_4, reg | HWR_SWH);

		reg1 = inb(address + 0x0b);
		reg1 &= 0x30;
		switch (reg1) {
		case 0x00:
			sz = RAM_SIZE_8K;
			break;
		case 0x10:
			sz = RAM_SIZE_16K;
			break;
		case 0x20:
			sz = RAM_SIZE_32K;
			break;
		case 0x30:
			sz = RAM_SIZE_64K;
			break;
		}
		outb(address + WD_REG_4, reg);
		return (sz);
	}
	return (RAM_SIZE_UNKNOWN);
}

/*
 * This routine will check for presence of 585/790 interface.
 *
 * Arguments:
 * BaseAddr - The base address for I/O to the board.
 *
 * Return:
 *	0, if not 585 nor 790.
 *	1, if 585/790.
 */
static int
bid_check_for_585(address)
	int		address;
{
	u_char		RegValue, RegValue1, RegValue2;
	u_char		SavValue;

	RegValue = inp(address + BID_REG_4);
	SavValue = RegValue;
	RegValue &= 0xC3;
	RegValue |= HWR_SWH;	/* 0x80 */
	outp(address + BID_REG_4, RegValue);
	RegValue1 = RegValue2 = 0;

	for (RegValue = 0; RegValue < 6; RegValue++) {
		RegValue1 = inp(address + 0x8 + RegValue);
		RegValue2 = inp(address + BID_REG_4);
		RegValue2 &= 0xC3;
		RegValue2 ^= HWR_SWH;
		outp(address + BID_REG_4, RegValue2);
		RegValue2 = inp(address + 0x8 + RegValue);

		if (RegValue1 != RegValue2)
			break;
	}

	if (RegValue == 6) {
		outp(address + BID_REG_4, SavValue);
		return (0);
	}
	outp(address + BID_REG_4, SavValue & 0xC3);
	return (1);
}
/* end add */
