/*
 * Copyright (c) 1992, 1993 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef	_SYS_SMCBOARD_H
#define	_SYS_SMCBOARD_H

#pragma ident	"@(#)smcboard.h	1.6	94/12/22 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * BOARD ID DEFINITIONS
 *
 * 32 Bits of information are returned by 'GetBoardID ()'.
 *
 * The low order 16 bits correspond to the Feature Bits which make
 * up a unique ID for a given class of boards.
 *
 *	e.g. STARLAN MEDIA, INTERFACE_CHIP, MICROCHANNEL

 *	note: board ID should be ANDed with the STATIC_ID_MASK
 *	before comparing to a specific board ID
 *
 * The high order 16 bits correspond to the Extra Bits which do not
 * change the boards ID.
 *
 *	e.g. RAM SIZE, 16 BIT SLOT, ALTERNATE IRQ
 */

#define	STARLAN_MEDIA		0x00000001	/* StarLAN */
#define	ETHERNET_MEDIA		0x00000002	/* Ethernet */
#define	TWISTED_PAIR_MEDIA	0x00000003	/* Twisted Pair */
#define	EW_MEDIA		0x00000004	/* Ethernet and Twisted Pair */
#define	MICROCHANNEL		0x00000008	/* MicroChannel Adapter */
#define	INTERFACE_CHIP		0x00000010	/* Soft Config Adapter */
#define	ADVANCED_FEATURES	0x00000020	/* Advance network interface */
						/* features */
/* #define	UNUSED		0x00000020 */	/* used to be INTELLIGENT */
#define	BOARD_16BIT		0x00000040	/* 16 bit capability */
#define	PAGED_RAM		0x00000080	/* Is there RAM paging? */
#define	PAGED_ROM		0x00000100	/* Is there ROM paging? */
#define	PCM_ADAPTER		0x00000200	/* PCMCIA adapter */
#define	LITE_VERSION		0x00000400	/* Reduced Feature Adapter */
#define	NIC_SUPERSET		0x00000800	/* Superset of 790 */
#define	RAM_SIZE_UNKNOWN	0x00000000	/* 000 => Unknown RAM Size */
#define	RAM_SIZE_RESERVED_1	0x00010000	/* 001 => Reserved */
#define	RAM_SIZE_8K		0x00020000	/* 010 => 8k RAM */
#define	RAM_SIZE_16K		0x00030000	/* 011 => 16k RAM */
#define	RAM_SIZE_32K		0x00040000	/* 100 => 32k RAM */
#define	RAM_SIZE_64K		0x00050000	/* 101 => 64k RAM */
#define	RAM_SIZE_RESERVED_6	0x00060000	/* 110 => Reserved */
#define	RAM_SIZE_RESERVED_7	0x00070000	/* 111 => Reserved */
#define	SLOT_16BIT		0x00080000	/* 16 bit board - 16 bit slot */
#define	NIC_690_BIT		0x00100000	/* NIC is 690 */
#define	ALTERNATE_IRQ_BIT	0x00200000	/* Alternate IRQ is used */
#define	INTERFACE_5X3_CHIP	0x00000000	/* 0000 = 583 or 593 chips */
#define	INTERFACE_584_CHIP	0x00400000	/* 0100 = 584 chip */
#define	INTERFACE_594_CHIP	0x00800000	/* 1000 = 594 chip */
#define	INTERFACE_585_CHIP	0x01000000	/* 585 BIC Chip */

#define	MEDIA_MASK		0x00000007	/* Isolates Media Type */
#define	RAM_SIZE_MASK		0x00070000	/* Isolates RAM Size */
#define	STATIC_ID_MASK		0x0000FFFF	/* Isolates Board ID */
#define	INTERFACE_CHIP_MASK	0x03C00000	/* Isolates Intfc Chip Type */
#define	NIC_790_BIT		0x08000000	/* NIC is 790 BIC/NIC Chip */

/* Word definitions for board types */
#define	WD8003E		ETHERNET_MEDIA
#define	WD8003EBT	WD8003E		/* functionally identical to WD8003E */
#define	WD8003S		STARLAN_MEDIA
#define	WD8003SH	WD8003S		/* functionally identical to WD8003S */
#define	WD8003WT	TWISTED_PAIR_MEDIA
#define	WD8003W		(TWISTED_PAIR_MEDIA | INTERFACE_CHIP)
#define	WD8003EB	(ETHERNET_MEDIA | INTERFACE_CHIP)
#define	WD8003EP	WD8003EB	/* with INTERFACE_584_CHIP bit set */
#define	WD8003EW	(EW_MEDIA | INTERFACE_CHIP)
#define	WD8003ETA	(ETHERNET_MEDIA | MICROCHANNEL)
#define	WD8003STA	(STARLAN_MEDIA | MICROCHANNEL)
#define	WD8003EA	(ETHERNET_MEDIA | MICROCHANNEL | INTERFACE_CHIP)
#define	WD8003EPA	WD8003EA	/* with INTERFACE_594_CHIP */
#define	WD8003SHA	(STARLAN_MEDIA | MICROCHANNEL | INTERFACE_CHIP)
#define	WD8003WA	(TWISTED_PAIR_MEDIA | MICROCHANNEL | INTERFACE_CHIP)
#define	WD8003WPA	WD8003WA	/* with INTERFACE_594_CHIP */
#define	WD8013EBT	(ETHERNET_MEDIA | BOARD_16BIT)
#define	WD8013EB	(ETHERNET_MEDIA | BOARD_16BIT | INTERFACE_CHIP)
#define	WD8013W		(TWISTED_PAIR_MEDIA | BOARD_16BIT | INTERFACE_CHIP)
#define	WD8013EW	(EW_MEDIA | BOARD_16BIT | INTERFACE_CHIP)
#define	WD8013EWC	(WD8013EW | ADVANCED_FEATURES)
#define	WD8013WC	(WD8013W | ADVANCED_FEATURES)
#define	WD8013EPC	(WD8013EB | ADVANCED_FEATURES)
#define	WD8003WC	(WD8003W | ADVANCED_FEATURES)
#define	WD8003EPC	(WD8003EP | ADVANCED_FEATURES)
#define	WD8115TA	(TOKEN_MEDIA | MICROCHANNEL | INTERFACE_CHIP | \
				PAGED_RAM)
#define	WD8115T		(TOKEN_MEDIA | INTERFACE_CHIP | BOARD_16BIT | \
				PAGED_RAM)
#define	WD8203W		(WD8003WC | PAGED_ROM)
#define	WD8203EP	(WD8003EPC | PAGED_ROM)
#define	WD8216T		(WD8013WC | PAGED_ROM | PAGED_RAM)
#define	WD8216		(WD8013EPC | PAGED_ROM | PAGED_RAM)
#define	WD8216C		(WD8013EWC | PAGED_ROM | PAGED_RAM)
#define	PCM10BT		(TWISTED_PAIR_MEDIA | PCM_ADAPTER | PAGED_RAM | \
				ADVANCED_FEATURES)

#define	CNFG_ICR_583		0x01	/* ICR */
#define	CNFG_ICR_IR2_584	0x04	/* IRQ index MSB (584) */
#define	CNFG_LAAR_584		0x05	/* LAAR (write only) */
#define	CNFG_LAAR_MASK		LAAR_MASK

#define	WD_REG_0	0x00
#define	WD_REG_1	0x01
#define	WD_REG_2	0x02
#define	WD_REG_3	0x03
#define	WD_REG_4	0x04
#define	WD_REG_5	0x05
#define	WD_REG_6	0x06
#define	WD_REG_7	0x07

#define	BID_EEPROM_PAGING_MASK	0xC0
#define	BID_EEPROM_RAM_PAGING	0x40
#define	BID_EEPROM_ROM_PAGING	0x80
#define	BID_EEPROM_LITE		0x08


/*
 * Declaration for the Routine Provided in the 'Board ID' Library.
 */
#if M_XENIX || AT286 || AT386 || M_UNIX
unsigned long	GetBoardID();
#else
unsigned long	GetBoardID(int, int);
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_SMCBOARD_H */
