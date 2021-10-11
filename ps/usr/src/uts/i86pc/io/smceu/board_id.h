/*
 * This is an SMC LMAC driver.  It will periodically be updated with
 * new versions from SMC.  It is important that minimal changes be
 * made to this file, to facilitate maintenance and running diffs with
 * new versions as they arrive.  DO NOT cstyle or lint this file, or
 * make any other unnecessary changes.
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#ident	"@(#)board_id.h 1.2	95/07/18 SMI"

/*--

Copyright (c) 1993  Standard MicroSystems Corporation

Module Name:

    board_id.h

Abstract:

    Lower MAC Interface functions for the UNIX SMC driver.

Author:

    Timothy Ngo 10-June-93

Environment:

    Kernel mode

Revision History:

--*/

/*//
//
// Below here is LM Specific codes and structures..
//
*/


/******************************************************************************
 Definitions for the field:
    cnfg_mode_bits1
******************************************************************************/


#define    INTERRUPT_STATUS_BIT    0x8000    /* PC Interrupt Line: 0 = Not Enabled */
#define    BOOT_STATUS_MASK        0x6000    /* Mask to isolate BOOT_STATUS */
#define    BOOT_INHIBIT            0x0000    /* BOOT_STATUS is 'inhibited' */
#define    BOOT_TYPE_1             0x2000    /* Unused BOOT_STATUS value */
#define    BOOT_TYPE_2             0x4000    /* Unused BOOT_STATUS value */
#define    BOOT_TYPE_3             0x6000    /* Unused BOOT_STATUS value */
#define    ZERO_WAIT_STATE_MASK    0x1800    /* Mask to isolate Wait State flags */
#define    ZERO_WAIT_STATE_8_BIT   0x1000    /* 0 = Disabled (Inserts Wait States) */
#define    ZERO_WAIT_STATE_16_BIT  0x0800    /* 0 = Disabled (Inserts Wait States) */
#define    BNC_INTERFACE           0x0400
#define    AUI_10BT_INTERFACE      0x0200
#define    STARLAN_10_INTERFACE    0x0100
#define    INTERFACE_TYPE_MASK     0x0700
#define    MANUAL_CRC              0x0010

/*//
//
// General Register types
//
//
*/

#define WD_REG_0     0x00
#define WD_REG_1     0x01
#define WD_REG_2     0x02
#define WD_REG_3     0x03
#define WD_REG_4     0x04
#define WD_REG_5     0x05
#define WD_REG_6     0x06
#define WD_REG_7     0x07

#define WD_LAN_OFFSET   0x08

#define WD_LAN_0     0x08
#define WD_LAN_1     0x09
#define WD_LAN_2     0x0A
#define WD_LAN_3     0x0B
#define WD_LAN_4     0x0C
#define WD_LAN_5     0x0D

#define WD_ID_BYTE   0x0E

#define WD_CHKSUM    0x0F

#define WD_MSB_583_BIT  0x08

#define WD_SIXTEEN_BIT  0x01

#define WD_BOARD_REV_MASK  0x1E

/*//
// Definitions for board Rev numbers greater than 1
//
*/

#define WD_MEDIA_TYPE_BIT   0x01
#define WD_SOFT_CONFIG_BIT  0x20
#define WD_RAM_SIZE_BIT     0x40
#define WD_BUS_TYPE_BIT     0x80


/*
//
// Definitions for the 690 board
//
*/
#define WD_690_CR           0x10        // command register

#define WD_690_TXP          0x04        // transmit packet command
#define WD_690_TCR          0x0D        // transmit configuration register
#define WD_690_TCR_TEST_VAL 0x18        // Value to test 8390 or 690

#define WD_690_PS0          0x00        // Page Select 0
#define WD_690_PS1          0x40        // Page Select 1
#define WD_690_PS2          0x80        // Page Select 2
#define WD_690_PSMASK       0x3F        // For masking off the page select bits

/*
//
// Definitions for the 584 board
//
*/

#define WD_584_EEPROM_0     0x08
#define WD_584_EEPROM_1     0x09
#define WD_584_EEPROM_2     0x0A
#define WD_584_EEPROM_3     0x0B
#define WD_584_EEPROM_4     0x0C
#define WD_584_EEPROM_5     0x0D
#define WD_584_EEPROM_6     0x0E
#define WD_584_EEPROM_7     0x0F

#define WD_584_OTHER_BIT    0x02
#define WD_584_ICR_MASK     0x0C
#define WD_584_EAR_MASK     0x0F
#define WD_584_ENGR_PAGE    0xA0
#define WD_584_RLA          0x10
#define WD_584_EA6          0x80
#define WD_584_RECALL_DONE  0x10

#define WD_584_ID_EEPROM_OVERRIDE       0x0000FFB0
#define WD_584_EXTRA_EEPROM_OVERRIDE    0xFFD00000

#define WD_584_EEPROM_MEDIA_MASK        0x07
#define WD_584_STARLAN_TYPE             0x00
#define WD_584_ETHERNET_TYPE            0x01
#define WD_584_TP_TYPE                  0x02
#define WD_584_EW_TYPE                  0x03

#define WD_584_EEPROM_IRQ_MASK          0x18
#define WD_584_PRIMARY_IRQ              0x00
#define WD_584_ALT_IRQ_1                0x08
#define WD_584_ALT_IRQ_2                0x10
#define WD_584_ALT_IRQ_3                0x18

#define BID_EEPROM_PAGING_MASK       0xC0
#define BID_EEPROM_RAM_PAGING        0x40
#define BID_EEPROM_ROM_PAGING        0x80

#define WD_584_EEPROM_RAM_SIZE_MASK     0xE0
#define WD_584_EEPROM_RAM_SIZE_RES1     0x00
#define WD_584_EEPROM_RAM_SIZE_RES2     0x20
#define WD_584_EEPROM_RAM_SIZE_8K       0x40
#define WD_584_EEPROM_RAM_SIZE_16K      0x60
#define WD_584_EEPROM_RAM_SIZE_32K      0x80
#define WD_584_EEPROM_RAM_SIZE_64K      0xA0
#define WD_584_EEPROM_RAM_SIZE_RES3     0xC0
#define WD_584_EEPROM_RAM_SIZE_RES4     0xE0

#define WD_584_EEPROM_BUS_TYPE_MASK     0x07
#define WD_584_EEPROM_BUS_TYPE_AT       0x00
#define WD_584_EEPROM_BUS_TYPE_MCA      0x01
#define WD_584_EEPROM_BUS_TYPE_EISA     0x02

#define WD_584_EEPROM_BUS_SIZE_MASK     0x18
#define WD_584_EEPROM_BUS_SIZE_8BIT     0x00
#define WD_584_EEPROM_BUS_SIZE_16BIT    0x08
#define WD_584_EEPROM_BUS_SIZE_32BIT    0x10
#define WD_584_EEPROM_BUS_SIZE_64BIT    0x18

/*
// 83c583 registers
/*

#define BASE_REG                    0x00
#define MEMORY_SELECT_REG           0x00    /* MSR	*/
#define INTERFACE_CONFIG_REG        0x01    /* ICR	*/
#define BUS_SIZE_REG                0x01    /* BSR (read only)*/
#define IO_ADDRESS_REG              0x02    /* IAR	*/
#define BIOS_ROM_ADDRESS_REG        0x03    /* BIO (583, 584)*/
#define EEPROM_ADDRESS_REG          0x03    /* EAR (584)*/
#define INTERRUPT_REQUEST_REG       0x04    /* IRR	*/
#define GENERAL_PURPOSE_REG1        0x05    /* GP1*/
#define LA_ADDRESS_REG              0x05    /* LAAR (write only)*/
#define IO_DATA_LATCH_REG           0x06    /* IOD (583)*/
#define INITIALIZE_JUMPER_REG       0x06    /* IJR (584)*/
#define GENERAL_PURPOSE_REG2        0x07    /* GP2	 */
#define LAN_ADDRESS_REG             0x08    /* LAR	 */
#define LAN_ADDRESS_REG2            0x09    /* LAR2 */
#define LAN_ADDRESS_REG3            0x0A    /* LAR3 */
#define LAN_ADDRESS_REG4            0x0B    /* LAR4 */
#define LAN_ADDRESS_REG5            0x0C    /* LAR5 */
#define LAN_ADDRESS_REG6            0x0D    /* LAR6 */
#define LAN_ADDRESS_REG7            0x0E    /* LAR7 */
#define LAN_ADDRESS_REG8            0x0F    /* LAR8 */

/*
// ICR
*/

#define STORE           0x80        /* Store into EEProm */
#define RECALL          0x40        /* Recall from EEProm */
#define RECALL_ALL_BUT_IO 0x20      /* Recall all but IO and LAN Address*/
#define RECALL_LAN      0x10        /* Recall LAN Address*/
#define MEMORY_SIZE     0x08        /* Shared Memory size  */
#define DMA_ENABLE      0x04        /* DMA Enable (583)*/
#define IR2             0x04        /* IRQ index MSB (584)*/
#define IO_PORT_ENABLE  0x02        /* (583)*/
#define OTHER           0x02        /* (584)*/
#define WORD_TRANSFER_SELECT 0x01


/*
// IRR
*/

#define INTERRUPT_ENABLE            0x80
#define INTERRUPT_REQUEST_BIT1      0x40
#define INTERRUPT_REQUEST_BIT0      0x20
#define ALTERNATE_MODE              0x10
#define ALTERNATE_INTERRUPT         0x08
#define BIOS_WAIT_STATE_BIT1        0x04
#define BIOS_WAIT_STATE_BIT0        0x02
#define ZERO_WAIT_STATE_ENABLE      0x01

/*
// LAAR
*/

#define MEMORY_16BIT_ENABLE         0x80
#define LAN_16BIT_ENABLE            0x40
#define LAAR_ZERO_WAIT_STATE        0x20
#define LAN_ADDRESS_BIT23           0x10
#define LAN_ADDRESS_BIT22           0x08
#define LAN_ADDRESS_BIT21           0x04
#define LAN_ADDRESS_BIT20           0x02
#define LAN_ADDRESS_BIT19           0x01

#define INIT_LAAR_VALUE             0x01   /* To set bit 19 to 1 */


/******************************************************************************
 BOARD ID DEFINITIONS

 32 Bits of information are returned by 'GetBoardID ()'.

	The low order 16 bits correspond to the Feature Bits which make
	up a unique ID for a given class of boards.

		e.g. STARLAN MEDIA, INTERFACE_CHIP, MICROCHANNEL

		note: board ID should be ANDed with the STATIC_ID_MASK
		      before comparing to a specific board ID


	The high order 16 bits correspond to the Extra Bits which do not
	change the boards ID.

		e.g. INTERFACE_584/585_CHIP, 16 BIT SLOT, ALTERNATE IRQ

******************************************************************************/


#define    STARLAN_MEDIA         0x00000001    /* StarLAN */
#define    ETHERNET_MEDIA        0x00000002    /* Ethernet */
#define    TWISTED_PAIR_MEDIA    0x00000003    /* Twisted Pair */
#define    EW_MEDIA              0x00000004    /* Ethernet and Twisted Pair */
#define    TOKEN_MEDIA           0x00000005    /* Token Ring */

#define    MICROCHANNEL          0x00000008    /* MicroChannel Adapter */
#define    INTERFACE_CHIP        0x00000010    /* Soft Config Adapter */
#define    ADVANCED_FEATURES     0x00000020    /* Advance netw interface features */
#define    BOARD_16BIT           0x00000040    /* 16 bit capability */
#define    PAGED_RAM             0x00000080    /* Is there RAM paging? */
#define    PAGED_ROM             0x00000100    /* Is there ROM paging? */
#define	  PCM_ADAPTER		0x00000200	/* PCMCIA adapter */

#define    RAM_SIZE_UNKNOWN      0x00000000    /* 000 => Unknown RAM Size */
#define    RAM_SIZE_RESERVED_1   0x00010000    /* 001 => Reserved */
#define    RAM_SIZE_8K           0x00020000    /* 010 => 8k RAM */
#define    RAM_SIZE_16K          0x00030000    /* 011 => 16k RAM */
#define    RAM_SIZE_32K          0x00040000    /* 100 => 32k RAM */
#define    RAM_SIZE_64K          0x00050000    /* 101 => 64k RAM */
#define    RAM_SIZE_RESERVED_6   0x00060000    /* 110 => Reserved */
#define    RAM_SIZE_RESERVED_7   0x00070000    /* 111 => Reserved */
#define    SLOT_16BIT            0x00080000    /* 16 bit board - 16 bit slot */
#define    NIC_690_BIT           0x00100000    /* NIC is 690 */
#define    ALTERNATE_IRQ_BIT     0x00200000    /* Alternate IRQ is used */
#define    INTERFACE_5X3_CHIP    0x00000000    /* 0000 = 583 or 593 chips */
#define    INTERFACE_584_CHIP    0x00400000    /* 0100 = 584 chip */
#define    INTERFACE_594_CHIP    0x00800000    /* 1000 = 594 chip */
#define	INTERFACE_585_CHIP	0x01000000	/* 585 BIC Chip*/


#define    MEDIA_MASK            0x00000007    /* Isolates Media Type */
#define    RAM_SIZE_MASK         0x00070000    /* Isolates RAM Size */
#define    STATIC_ID_MASK        0x0000FFFF    /* Isolates Board ID */
#define    INTERFACE_CHIP_MASK   0x03C00000    /* Isolates Intfc Chip Type */
#define	NIC_825_BIT		0x04000000				  /* NIC is 825 Token Ring */
#define	NIC_790_BIT		0x08000000				  /* NIC is 790 BIC/NIC Chip*/


/* Word definitions for board types */

#define    WD8003E     ETHERNET_MEDIA
#define    WD8003EBT   WD8003E        /* functionally identical to WD8003E */
#define    WD8003S     STARLAN_MEDIA
#define    WD8003SH    WD8003S        /* functionally identical to WD8003S */
#define    WD8003WT    TWISTED_PAIR_MEDIA
#define    WD8003W     (TWISTED_PAIR_MEDIA | INTERFACE_CHIP)
#define    WD8003EB    (ETHERNET_MEDIA | INTERFACE_CHIP)
#define    WD8003EP    WD8003EB       /* with INTERFACE_584_CHIP */
#define    WD8003EW    (EW_MEDIA | INTERFACE_CHIP)
#define    WD8003ETA   (ETHERNET_MEDIA | MICROCHANNEL)
#define    WD8003STA   (STARLAN_MEDIA | MICROCHANNEL)
#define    WD8003EA    (ETHERNET_MEDIA | MICROCHANNEL | INTERFACE_CHIP)
#define    WD8003EPA   WD8003EA       /* with INTERFACE_594_CHIP */
#define    WD8003SHA   (STARLAN_MEDIA | MICROCHANNEL | INTERFACE_CHIP)
#define    WD8003WA    (TWISTED_PAIR_MEDIA | MICROCHANNEL | INTERFACE_CHIP)
#define    WD8003WPA   WD8003WA       /* with INTERFACE_594_CHIP */
#define    WD8013EBT   (ETHERNET_MEDIA | BOARD_16BIT)
#define    WD8013EB    (ETHERNET_MEDIA | BOARD_16BIT | INTERFACE_CHIP)
#define    WD8013W     (TWISTED_PAIR_MEDIA | BOARD_16BIT | INTERFACE_CHIP)
#define    WD8013EW    (EW_MEDIA | BOARD_16BIT | INTERFACE_CHIP)
#define WD8013EWC	(WD8013EW | ADVANCED_FEATURES)
#define WD8013WC	(WD8013W | ADVANCED_FEATURES)
#define WD8013EPC	(WD8013EB | ADVANCED_FEATURES)
#define WD8003WC	(WD8003W | ADVANCED_FEATURES)
#define	WD8003EPC	(WD8003EP | ADVANCED_FEATURES)
#define	WD8115TA	(TOKEN_MEDIA | MICROCHANNEL | INTERFACE_CHIP | PAGED_RAM)
#define	WD8115T		(TOKEN_MEDIA | INTERFACE_CHIP | BOARD_16BIT | PAGED_RAM)
#define	WD8203W		(WD8003WC | PAGED_ROM)
#define	WD8203EP	(WD8003EPC | PAGED_ROM)
#define	WD8216T		(WD8013WC | PAGED_ROM | PAGED_RAM)
#define	WD8216		(WD8013EPC | PAGED_ROM | PAGED_RAM)
#define	WD8216C		(WD8013EWC | PAGED_ROM | PAGED_RAM)
#define	PCM10BT		(TWISTED_PAIR_MEDIA | PCM_ADAPTER | PAGED_RAM | ADVANCED_FEATURES)
#ifndef  EISA_BRD_ID
#define  EISA_BRD_ID	0x8010
#endif
#define  S82M32		EISA_BRD_ID



#define CNFG_ID_8003E       0x6FC0
#define CNFG_ID_8003S       0x6FC1
#define CNFG_ID_8003W       0x6FC2
#define CNFG_ID_8013E       0x61C8
#define CNFG_ID_8013W       0x61C9
#define CNFG_ID_8115TRA     0x6EC6
#define CNFG_ID_BISTRO03E   0xEFE5
#define CNFG_ID_BISTRO13E   0xEFD5
#define CNFG_ID_BISTRO13W   0xEFD4

#define CNFG_MSR_583        MEMORY_SELECT_REG
#define CNFG_ICR_583        INTERFACE_CONFIG_REG
#define CNFG_IAR_583        IO_ADDRESS_REG
#define CNFG_BIO_583        BIOS_ROM_ADDRESS_REG
#define CNFG_IRR_583        INTERRUPT_REQUEST_REG
#define CNFG_LAAR_584       LA_ADDRESS_REG
#define CNFG_GP2            GENERAL_PURPOSE_REG2
#define CNFG_LAAR_MASK      LAAR_MASK
#define CNFG_LAAR_ZWS       LAAR_ZERO_WAIT_STATE
#define CNFG_ICR_IR2_584    IR2
#define CNFG_IRR_IRQS       (INTERRUPT_REQUEST_BIT1 | INTERRUPT_REQUEST_BIT0)
#define CNFG_IRR_IEN        INTERRUPT_ENABLE
#define CNFG_IRR_ZWS        ZERO_WAIT_STATE_ENABLE
#define CNFG_GP2_BOOT_NIBBLE 0xF

#define CNFG_SIZE_8KB       8
#define CNFG_SIZE_16KB      16
#define CNFG_SIZE_32KB      32
#define CNFG_SIZE_64KB      64

#define ROM_DISABLE         0x0

#define CNFG_SLOT_ENABLE_BIT 0x8

#define CNFG_MEDIA_TYPE_MASK 0x07

#define CNFG_INTERFACE_TYPE_MASK 0x700
#define CNFG_POS_CONTROL_REG 0x96
#define CNFG_POS_REG0       0x100
#define CNFG_POS_REG1       0x101
#define CNFG_POS_REG2       0x102
#define CNFG_POS_REG3       0x103
#define CNFG_POS_REG4       0x104
#define CNFG_POS_REG5       0x105

/******************************************************************************

Declaration for the Routine Provided in the 'Board ID' Library.

******************************************************************************/
#if M_XENIX || AT286 || AT386 || M_UNIX
unsigned	long	GetBoardID ();
#else
unsigned	long	GetBoardID (unsigned int, int);
#endif

#define	BID_FALSE	0
#define	BID_TRUE	1

/* Register offset definitions...since different boards have different names
	for register offsets 0x00-0x07, generic names will be assigned */
#define	BID_REG_0		0x00
#define	BID_REG_1		0x01
#define	BID_REG_2		0x02
#define	BID_REG_3		0x03
#define	BID_REG_4		0x04
#define	BID_REG_5		0x05
#define	BID_REG_6		0x06
#define	BID_REG_7		0x07

/* Register offset definitions...the names for this registers are consistant
	across all boards, so specific names will be assigned */
#define	BID_LAN_ADDR_0		0x08	/* these 6 registers hold the */
#define	BID_LAN_ADDR_1		0x09	/*     LAN address for this node */
#define	BID_LAN_ADDR_2		0x0A
#define	BID_LAN_ADDR_3		0x0B
#define	BID_LAN_ADDR_4		0x0C
#define	BID_LAN_ADDR_5		0x0D
#define	BID_BOARD_ID_BYTE	0x0E	/* identification byte for WD boards */
#define	BID_CHCKSM_BYTE		0x0F	/* the address ROM checksum byte */

/**** Masks bits for the board revision number in the BID_BOARD_ID_BYTE ****/
#define	BID_BOARD_REV_MASK	0x1E

/*** Misc. definitions ***/
#define	BID_MSZ_583_BIT		0x08	/* memory size bit in 583 */
#define BID_SIXTEEN_BIT_BIT	0x01	/* bit has 16 bit capability info */

/*** Defs for board rev numbers greater than 1 ***/
#define	BID_MEDIA_TYPE_BIT	0x01
#define	BID_SOFT_CONFIG_BIT	0x20
#define	BID_RAM_SIZE_BIT	0x40
#define	BID_BUS_TYPE_BIT	0x80

/**** defs for identifying the 690 ****/
#define BID_CR		0x10		/* Command Register	*/
#define BID_TXP		0x04		/* transmit packet */
#define	BID_TCR		0x1D		/* Transmit Configuration Register */
#define	BID_TCR_VAL	0x18		/* test value for 690 or 8390 */
#define	BID_PS0		0x00		/* register page select - 0 */
#define BID_PS1		0x40		/* register page select - 1 */
#define	BID_PS2		0x80		/* register page select - 2 */
#define	BID_PS_MASK	0x3F		/* to mask off page select bits */

/* Register offsets for reading the EEPROM in the 584 chip */
#define	BID_EEPROM_0		0x08
#define	BID_EEPROM_1		0x09
#define	BID_EEPROM_2		0x0A
#define	BID_EEPROM_3		0x0B
#define	BID_EEPROM_4		0x0C
#define	BID_EEPROM_5		0x0D
#define	BID_EEPROM_6		0x0E
#define	BID_EEPROM_7		0x0F

/**** defs for manipulating the 584 ****/
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

/* MEDIA OPTS in eeprom */
#define MED_OPT_BNC		0x01
#define MED_OPT_UTP		0x02
#define MED_OPT_AUI		0x04
#define MED_OPT_10MB		0x08
#define MED_OPT_100MB	0x10
#define MED_OPT_S10		0x20

#if M_XENIX || AT286 || AT386 || M_UNIX || REALMODE
#ifdef BADLINT
#define inp(port)	inb(port)
#define outp(port, val)	outb(port, val)
#else
#define inp(port)	inb((ushort)(port))
#define outp(port, val)	outb((ushort)(port), (unchar)(val))
#endif
#endif
