/*
 * Copyright 1993 by Sun Microsystems, Inc.
 */

#ifndef	_BT485REG_H
#define	_BT485REG_H

#pragma ident	"@(#)bt485reg.h	1.1	94/06/27 SMI"

/*
 * The following describes the structure of the Brooktree Bt485  135
 * Mhz Monolithic CMOS True-Color RAMDAC.  Since the BT485 may occur
 * at  any  set of addresses, define BT485_BASE, BT485_A0, BT485_A1,
 * BT485_A2, BT485_A3 to set  the  addresses  of  the  ports.   This
 * allows the header to be used for both i/o and memory addresses.
 */

#ifdef	__cplusplus
extern "C" {
#endif

#define BT485_RAM_WRITE     BT485_BASE
#define BT485_PALET_DATA    (BT485_BASE + BT485_A0)
#define BT485_PIXEL_MASK    (BT485_BASE + BT485_A1)
#define BT485_RAM_READ      (BT485_BASE + BT485_A1 + BT485_A0)
#define BT485_COLOR_WRITE   (BT485_BASE + BT485_A2)
#define BT485_COLOR_DATA    (BT485_BASE + BT485_A2 + BT485_A0)
#define BT485_COMREG0       (BT485_BASE + BT485_A2 + BT485_A1)
#define BT485_COLOR_READ    (BT485_BASE + BT485_A2 + BT485_A1 + BT485_A0)
#define BT485_COMREG1       (BT485_BASE + BT485_A3)
#define BT485_COMREG2       (BT485_BASE + BT485_A3 + BT485_A0)
#define BT485_STAT_REG      (BT485_BASE + BT485_A3 + BT485_A1)
#define BT485_CURSOR_DATA   (BT485_BASE + BT485_A3 + BT485_A1 + BT485_A0)
#define BT485_CURSOR_X_LOW  (BT485_BASE + BT485_A3 + BT485_A2)
#define BT485_CURSOR_X_HIGH (BT485_BASE + BT485_A3 + BT485_A2 + BT485_A0)
#define BT485_CURSOR_Y_LOW  (BT485_BASE + BT485_A3 + BT485_A2 + BT485_A1)
#define BT485_CURSOR_Y_HIGH (BT485_BASE + BT485_A3 + BT485_A2 + BT485_A1 + \
	BT485_A0)
#define BT485_COMREG3       BT485_STAT_REG

	/* Cursor/Overscan Addresses */

#define BT485_OVERSCAN_COLOR    0x00
#define BT485_CURSOR1_COLOR     0x01
#define BT485_CURSOR2_COLOR     0x02
#define BT485_CURSOR3_COLOR     0x03

	/* Values to write to address register when command register */
	/* 3 enabled */

#define BT485_STATUS_SELECT     0x00
#define BT485_COMREG3_SELECT    0x01

	/* Command register 0 */

#define	BT485_POWER_DOWN_ENABLE             0x01
#define	BT485_DAC_6_BIT_RESOLUTION          0x00
#define	BT485_DAC_8_BIT_RESOLUTION          0x02
#define	BT485_RED_SYNC_ENABLE               0x04
#define	BT485_GREEN_SYNC_ENABLE             0x08
#define	BT485_BLUE_SYNC_ENABLE              0x10
#define	BT485_SETUP_ENABLE                  0x20
#define	BT485_DISABLE_INTERNAL_CLOCKING     0x40
#define	BT485_COMMAND_REGISTER_3_ENABLE     0x80

	/* Command register 1 */

#define	BT485_16_BIT_AB_MULTIPLEX           0x00
#define	BT485_16_BIT_CD_MULTIPLEX           0x01
#define	BT485_16_BIT_REAL_TIME_ENABLE       0x02
#define	BT485_16_BIT_2_1_MULTIPLEX          0x00
#define	BT485_16_BIT_1_1_MULTIPLEX          0x04
#define	BT485_16_BIT_5_5_5_FORMAT           0x00
#define	BT485_16_BIT_5_6_5_FORMAT           0x08
#define	BT485_TRUE_COLOR_ENABLE             0x10
#define	BT485_BIT_PIXEL_SELECT              0x60
#define	BT485_24_BIT_PIXELS                 0x00
#define	BT485_16_BIT_PIXELS                 0x20
#define	BT485_8_BIT_PIXELS                  0x40
#define	BT485_4_BIT_PIXELS                  0x60

	/* Command register 2 */

#define	BT485_CURSOR_MODE_SELECT            0x03
#define	BT485_CURSOR_DISABLED               0x00
#define	BT485_THREE_COLOR_CURSOR            0x01
#define	BT485_TWO_COLOR_HIGHLIGHT_CURSOR    0x02
#define	BT485_TWO_COLOR_X_WINDOWS_CURSOR    0x03
#define	BT485_16_BIT_CONTIGUOUS_INDEXING    0x04
#define	BT485_INTERLACED                    0x08
#define	BT485_PCLK1_SELECTED                0x10
#define	BT485_PORTSEL_UNMASKED              0x20
#define	BT485_TEST_PATH_ENABLED             0x40
#define	BT485_SCLK_DISABLED                 0x80

	/* Command register 3 */

#define	BT485_A8                            0x01
#define	BT485_A9                            0x02
#define	BT485_A8_A9                         (BT485_A8 | BT485_A9)
#define	BT485_64_64_2_CURSOR                0x04
#define BT485_32_32_2_CURSOR                0x00
#define	BT485_2X_CLOCK_MULTIPLIER_ENABLED   0x08

	/* Status Register */

#define BT485_STATUS_COMPONENT_MASK         0x03
#define BT485_STATUS_RED_COLOR_COMPONENT    0x00
#define BT485_STATUS_GREEN_COLOR_COMPONENT  0x01
#define BT485_STATUS_BLUE_COLOR_COMPONENT   0x02
#define BT485_STATUS_WRITE_CYCLE            0x00
#define BT485_STATUS_READ_CYCLE             0x04
#define BT485_STATUS_SENSE                  0x08
#define BT485_STATUS_REVISION_MASK          0x30
#define BT485_STATUS_IDENTIFICATION_MASK    0xc0

#define BT485_CURSOR_COORDINATE_MASK        0x0fff
#define BT485_CURSOR_OFFPOS                 0x0000

#ifdef	__cplusplus
}
#endif

#endif	/* !_BT485REG_H */
