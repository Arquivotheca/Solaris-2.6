/*
 * Copyright 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef	_RGB525REG_H
#define	_RGB525REG_H

#pragma ident   "@(#)rgb525reg.h 1.3     95/04/19 SMI"


/*
 * The following describes the structure of the IBM RGB525
 * 170/220/250 Mhz High Performance Palette DAC.  Since the RGB525
 * may occur at any set of addresses, define RGB525_BASE, RGB525_A0,
 * RGB525_A1, RGB525_A2 to set the addresses of the ports.  This
 * allows the header to be used for both i/o and memory addresses.
 */

#ifdef	__cplusplus
extern "C" {
#endif

#define RGB525_PALET_WRITE   RGB525_BASE
#define RGB525_PALET_DATA    (RGB525_BASE + RGB525_A0)
#define RGB525_PIXEL_MASK    (RGB525_BASE + RGB525_A1)
#define RGB525_PALET_READ    (RGB525_BASE + RGB525_A1 + RGB525_A0)
#define RGB525_INDEX_LOW     (RGB525_BASE + RGB525_A2)
#define RGB525_INDEX_HIGH    (RGB525_BASE + RGB525_A2 + RGB525_A0)
#define RGB525_INDEX_DATA    (RGB525_BASE + RGB525_A2 + RGB525_A1)
#define RGB525_INDEX_CONTROL (RGB525_BASE + RGB525_A2 + RGB525_A1 + RGB525_A0)

	/* Palette Data */

#define RGB525_PALDATA_6BIT_MASK                    0x3f

	/* Palette Access State */

#define RGB525_ACCSTATE_MASK                        0x03
#define RGB525_ACCSTATE_WRITE_MODE                  0x00
#define RGB525_ACCSTATE_READ_MODE                   0x03

	/* Indexes */

#define RGB525_INDEX_MASK                           0x07ff
#define RGB525_INDEX_REVISION_LEVEL                 0x0000  /* ro */
#define RGB525_INDEX_ID                             0x0001  /* ro */
#define RGB525_INDEX_MISCELLANEOUS_CLOCK_CONTROL    0x0002  /* rw */
#define RGB525_INDEX_SYNC_CONTROL                   0x0003  /* rw */
#define RGB525_INDEX_HORIZONTAL_SYNC_POSITION       0x0004  /* rw */
#define RGB525_INDEX_POWER_MANAGEMENT               0x0005  /* rw */
#define RGB525_INDEX_DAC_OPERATION                  0x0006  /* rw */
#define RGB525_INDEX_PALLETTE_CONTROL               0x0007  /* rw */
#define RGB525_INDEX_PIXEL_FORMAT                   0x000a  /* rw */
#define RGB525_INDEX_8BPP_CONTROL                   0x000b  /* rw */
#define RGB525_INDEX_16BPP_CONTROL                  0x000c  /* rw */
#define RGB525_INDEX_24BPP_CONTROL                  0x000d  /* rw */
#define RGB525_INDEX_32BPP_CONTROL                  0x000e  /* rw */
#define RGB525_INDEX_PLL_CONTROL_1                  0x0010  /* rw */
#define RGB525_INDEX_PLL_CONTROL_2                  0x0011  /* rw */
#define RGB525_INDEX_FIXED_PLL_REFERENCE_DIVIDER    0x0014  /* rw */
#define RGB525_INDEX_F0                             0x0020  /* rw */
#define RGB525_INDEX_M0                             0x0020  /* rw */
#define RGB525_INDEX_F1                             0x0021  /* rw */
#define RGB525_INDEX_N0                             0x0021  /* rw */
#define RGB525_INDEX_F2                             0x0022  /* rw */
#define RGB525_INDEX_M1                             0x0022  /* rw */
#define RGB525_INDEX_F3                             0x0023  /* rw */
#define RGB525_INDEX_N1                             0x0023  /* rw */
#define RGB525_INDEX_F4                             0x0024  /* rw */
#define RGB525_INDEX_M2                             0x0024  /* rw */
#define RGB525_INDEX_F5                             0x0025  /* rw */
#define RGB525_INDEX_N2                             0x0025  /* rw */
#define RGB525_INDEX_F6                             0x0026  /* rw */
#define RGB525_INDEX_M3                             0x0026  /* rw */
#define RGB525_INDEX_F7                             0x0027  /* rw */
#define RGB525_INDEX_N3                             0x0027  /* rw */
#define RGB525_INDEX_F8                             0x0028  /* rw */
#define RGB525_INDEX_M4                             0x0028  /* rw */
#define RGB525_INDEX_F9                             0x0029  /* rw */
#define RGB525_INDEX_N4                             0x0029  /* rw */
#define RGB525_INDEX_F10                            0x002a  /* rw */
#define RGB525_INDEX_M5                             0x002a  /* rw */
#define RGB525_INDEX_F11                            0x002b  /* rw */
#define RGB525_INDEX_N5                             0x002b  /* rw */
#define RGB525_INDEX_F12                            0x002c  /* rw */
#define RGB525_INDEX_M6                             0x002c  /* rw */
#define RGB525_INDEX_F13                            0x002d  /* rw */
#define RGB525_INDEX_N6                             0x002d  /* rw */
#define RGB525_INDEX_F14                            0x002e  /* rw */
#define RGB525_INDEX_M7                             0x002e  /* rw */
#define RGB525_INDEX_F15                            0x002f  /* rw */
#define RGB525_INDEX_N7                             0x002f  /* rw */
#define RGB525_INDEX_CURSOR_CONTROL                 0x0030  /* rw */
#define RGB525_INDEX_CURSOR_X_LOW                   0x0031  /* rw */
#define RGB525_INDEX_CURSOR_X_HIGH                  0x0032  /* rw */
#define RGB525_INDEX_CURSOR_Y_LOW                   0x0033  /* rw */
#define RGB525_INDEX_CURSOR_Y_HIGH                  0x0034  /* rw */
#define RGB525_INDEX_CURSOR_HOT_SPOT_X              0x0035  /* rw */
#define RGB525_INDEX_CURSOR_HOT_SPOT_Y              0x0036  /* rw */
#define RGB525_INDEX_CURSOR_COLOR_1_RED             0x0040  /* rw */
#define RGB525_INDEX_CURSOR_COLOR_1_GREEN           0x0041  /* rw */
#define RGB525_INDEX_CURSOR_COLOR_1_BLUE            0x0042  /* rw */
#define RGB525_INDEX_CURSOR_COLOR_2_RED             0x0043  /* rw */
#define RGB525_INDEX_CURSOR_COLOR_2_GREEN           0x0044  /* rw */
#define RGB525_INDEX_CURSOR_COLOR_2_BLUE            0x0045  /* rw */
#define RGB525_INDEX_CURSOR_COLOR_3_RED             0x0046  /* rw */
#define RGB525_INDEX_CURSOR_COLOR_3_GREEN           0x0047  /* rw */
#define RGB525_INDEX_CURSOR_COLOR_3_BLUE            0x0048  /* rw */
#define RGB525_INDEX_BORDER_COLOR_RED               0x0060  /* rw */
#define RGB525_INDEX_BORDER_COLOR_GREEN             0x0061  /* rw */
#define RGB525_INDEX_BORDER_COLOR_BLUE              0x0062  /* rw */
#define RGB525_INDEX_MISCELLANEOUS_CONTROL_1        0x0070  /* rw */
#define RGB525_INDEX_MISCELLANEOUS_CONTROL_2        0x0071  /* rw */
#define RGB525_INDEX_MISCELLANEOUS_CONTROL_3        0x0072  /* rw */
#define RGB525_INDEX_DAC_SENSE                      0x0082  /* ro */
#define RGB525_INDEX_MISR_RED                       0x0084  /* ro */
#define RGB525_INDEX_MISR_GREEN                     0x0086  /* ro */
#define RGB525_INDEX_MISR_BLUE                      0x0088  /* ro */
#define RGB525_INDEX_PLL_VCO_DIVIDER_INPUT          0x008e  /* ro */
#define RGB525_INDEX_PLL_REFERENCE_DIVIDER_INPUT    0x008f  /* ro */
#define RGB525_INDEX_VRAM_MASK_LOW                  0x0090  /* rw */
#define RGB525_INDEX_VRAM_MASK_HIGH                 0x0091  /* rw */
#define RGB525_INDEX_CURSOR_ARRAY                   0x0100  /* rw */
#define RGB525_INDEX_SMALL_CURSOR_SIZE              0x0100
#define RGB525_INDEX_NUMBER_SMALL_CURSORS           4

	/* ID 0x0001 ro */

#define RGB525_ID_PRODUCT_IDENTIFICATION_CODE       0x01

	/* Miscellaneous Clock Control 0x0002 rw */

#define RGB525_MISCCLK_DDOT_DISABLE                 0x80
#define RGB525_MISCCLK_SCLK_DISABLE                 0x40
#define RGB525_MISCCLK_B24P_CLOCK_SCLK              0x20
#define RGB525_MISCCLK_DDOT_DIVIDE_MASK             0x0e
#define RGB525_MISCCLK_DDOT_DIVIDE_1                0x00
#define RGB525_MISCCLK_DDOT_DIVIDE_2                0x02
#define RGB525_MISCCLK_DDOT_DIVIDE_4                0x04
#define RGB525_MISCCLK_DDOT_DIVIDE_8                0x06
#define RGB525_MISCCLK_DDOT_DIVIDE_16               0x08
#define RGB525_MISCCLK_PLL_ENABLE                   0x01

	/* Sync Control 0x0003 rw */

#define RGB525_SYNC_DELAY_CONTROL                   0x80
#define RGB525_SYNC_COMPOSITE_SYNC_INVERT           0x40
#define RGB525_SYNC_VSYNC_INVERT                    0x20
#define RGB525_SYNC_HSYNC_INVERT                    0x10
#define RGB525_SYNC_VSYNC_MASK                      0x0c
#define RGB525_SYNC_VSYNC_NORMAL                    0x00
#define RGB525_SYNC_VSYNC_HIGH                      0x04
#define RGB525_SYNC_VSYNC_LOW                       0x08
#define RGB525_SYNC_VSYNC_DISABLED                  0x0c
#define RGB525_SYNC_HSYNC_MASK                      0x03
#define RGB525_SYNC_HSYNC_NORMAL                    0x00
#define RGB525_SYNC_HSYNC_HIGH                      0x01
#define RGB525_SYNC_HSYNC_LOW                       0x02
#define RGB525_SYNC_HSYNC_DISABLED                  0x03

	/* Horizontal Sync Control 0x0004 rw */

#define RGB525_HSYNC_POSITION_MASK                  0x0f

	/* Power Management 0x0005 rw */

#define RGB525_POWER_DISABLE_SCLK_POWER             0x10
#define RGB525_POWER_DISABLE_DDOT_POWER             0x08
#define RGB525_POWER_DISABLE_SYNC_POWER             0x04
#define RGB525_POWER_DISABLE_INTERNAL_POWER         0x02
#define RGB525_POWER_DISABLE_DAC_POWER              0x01

	/* DAC Operation 0x0006 rw */

#define RGB525_DAC_COMPOSITE_SYNC_ON_GREEN          0x08
#define RGB525_DAC_BLANK_RED_AND_BLUE_DACS          0x04
#define RGB525_DAC_FAST_DAC_SLEW_RATE               0x02
#define RGB525_DAC_BLANKING_PEDESTAL_ENABLED        0x01

	/* Palette Control 0x0007 rw */

#define RGB525_PALET_6BIT_LINEAR_DISABLED           0x80
#define RGB525_PALET_PARTITION_MASK                 0x0f

	/* Pixel Format 0x000a rw */

#define RGB525_PIXEL_FORMAT_MASK                    0x07
#define RGB525_PIXEL_FORMAT_4BPP                    0x02
#define RGB525_PIXEL_FORMAT_8BPP                    0x03
#define RGB525_PIXEL_FORMAT_16BPP                   0x04
#define RGB525_PIXEL_FORMAT_24BPP                   0x05
#define RGB525_PIXEL_FORMAT_32BPP                   0x06

	/* 8 Bit Pixel Control 0x000b rw */

#define RGB525_8BPP_DIRECT_COLOR                    0x01

	/* 16 Bit Pixel Control 0x000c rw */

#define RGB525_16BPP_DIRECT_COLOR_CONTROL_MASK      0xc0
#define RGB525_16BPP_INDIRECT_COLOR                 0x00
#define RGB525_16BPP_DYNAMIC_COLOR                  0x40
#define RGB525_16BPP_DIRECT_COLOR                   0xc0
#define RGB525_16BPP_BYPASS_BIT_POLARITY            0x20
#define RGB525_16BPP_BIT_FILL_LINEAR                0x04
#define RGB525_16BPP_555_COLOR                      0x00
#define RGB525_16BPP_565_COLOR                      0x02
#define RGB525_16BPP_CONTIGUOUS                     0x01

	/* 24 Bit Pixel Control 0x000d rw */

#define RGB525_24BPP_DIRECT_COLOR                   0x01

	/* 32 Bit Pixel Control 0x000e rw */

#define RGB525_32BPP_BYPASS_BIT_POLARITY            0x04
#define RGB525_32BPP_DIRECT_COLOR_CONTROL_MASK      0x03
#define RGB525_32BPP_INDIRECT_COLOR                 0x00
#define RGB525_32BPP_DYNAMIC_COLOR                  0x01
#define RGB525_32BPP_DIRECT_COLOR                   0x03

	/* PLL Control 1 0x0010 rw */

#define RGB525_PLL1_REF_SRC_REFCLK                  0x00
#define RGB525_PLL1_REF_SRC_EXTCLK                  0x10
#define RGB525_PLL1_SOURCE_MASK                     0x07
#define RGB525_PLL1_SOURCE_EXTERNAL_FS              0x00
#define RGB525_PLL1_SOURCE_EXTERNAL_MN              0x01
#define RGB525_PLL1_SOURCE_DIRECT_FS                0x02
#define RGB525_PLL1_SOURCE_DIRECT_MN                0x03

	/* PLL Control 2 0x0011 rw */

#define RGB525_PLL2_INT_FS_MASK                     0x0f

	/* Fixed PLL Reference Divider 0x0014 rw */
	/* F0 - F15, M0 - M7, N0 - N7 0x0020 - 0x002f rw */
	/* PLL VCO Divider Input 0x008e ro */
	/* PLL Reference Divider Input 0x008f ro */

#define RGB525_CLK_REFERENCE_DIVIDE_MASK           0x1f
#define RGB525_CLK_REFERENCE_DIVIDE_FACTOR         2
#define RGB525_CLK_DESIRED_FREQUENCY_MASK          0xc0
#define RGB525_CLK_DESIRED_FREQUENCY_SHIFT         6
#define RGB525_CLK_DESIRED_FREQUENCY_STEP_FOURTH   0x00
#define RGB525_CLK_DESIRED_FREQUENCY_STEP_HALF     0x40
#define RGB525_CLK_DESIRED_FREQUENCY_STEP_ONE      0x80
#define RGB525_CLK_DESIRED_FREQUENCY_STEP_TWO      0xc0
#define RGB525_CLK_VCO_DIVIDE_MASK                 0x3f
#define RGB525_CLK_VCO_DIVIDE_OFFSET               65

	/* Cursor Control 0x0030 rw */

#define RGB525_CURSOR_SMALL_PARTITION_MASK          0xc0
#define RGB525_CURSOR_SMALL_PARTITION_SHIFT         6
#define RGB525_CURSOR_RIGHT_TO_LEFT                 0x00
#define RGB525_CURSOR_LEFT_TO_RIGHT                 0x20
#define RGB525_CURSOR_ACTUAL_LOCATION               0x10
#define RGB525_CURSOR_IMMEDIATE_UPDATE              0x08
#define RGB525_CURSOR_SIZE_32X32                    0x00
#define RGB525_CURSOR_SIZE_64X64                    0x04
#define RGB525_CURSOR_MODE_MASK                     0x03
#define RGB525_CURSOR_MODE_OFF                      0x00
#define RGB525_CURSOR_MODE_MODE_0                   0x01
#define RGB525_CURSOR_MODE_MODE_1                   0x02
#define RGB525_CURSOR_MODE_MODE_2                   0x03

	/* Hot Spot 0x0035 - 0x0036 rw */

#define RGB525_HOTSPOT_MASK_32                      0x1f
#define RGB525_HOTSPOT_MASK_64                      0x3f


	/* Miscellaneous Control 1 0x0070 rw */

#define RGB525_MISC1_MISR_CONTROL_ON                0x80
#define RGB525_MISC1_VRAM_MASK_ON                   0x40
#define RGB525_MISC1_PALET_ADDR_READ_FORMAT         0x20
#define RGB525_MISC1_SENSE_DISABLE                  0x10
#define RGB525_MISC1_SENSE_SELECT                   0x08
#define RGB525_MISC1_VRAM_SIZE_32BITS               0x00
#define RGB525_MISC1_VRAM_SIZE_64BITS               0x01

	/* Miscellaneous Control 2 0x0071 rw */

#define RGB525_MISC2_PIXEL_CLOCK_MASK               0xc0
#define RGB525_MISC2_PIXEL_CLOCK_INT_LCLK_INPUT     0x00
#define RGB525_MISC2_PIXEL_CLOCK_EXT_PLL_OUTPUT     0x40
#define RGB525_MISC2_PIXEL_CLOCK_EXT_OSC_INPUT      0x80
#define RGB525_MISC2_NON_INTERLACED                 0x00
#define RGB525_MISC2_INTERLACED                     0x20
#define RGB525_MISC2_BLANKED                        0x10
#define RGB525_MISC2_COLOR_RESOLUTION_6BIT          0x00
#define RGB525_MISC2_COLOR_RESOLUTION_8BIT          0x04
#define RGB525_MISC2_PORT_SELECT_VRAM               0x01

	/* Miscellaneous Control 3 0x0072 rw */

#define RGB525_MISC3_SWAP_RED_AND_BLUE              0x80
#define RGB525_MISC3_SWAP_WORDS                     0x10
#define RGB525_MISC3_SWAP_NIBBLES                   0x02

	/* Dac Sense 0x0082 ro */

#define RGB525_DACSENSE_LATCHED_SENSE               0x80
#define RGB525_DACSENSE_LATCHED_BLUE_DAC            0x40
#define RGB525_DACSENSE_LATCHED_GREEN_DAC           0x20
#define RGB525_DACSENSE_LATCHED_RED_DAC             0x10
#define RGB525_DACSENSE_SENSE                       0x08
#define RGB525_DACSENSE_BLUE_DAC                    0x04
#define RGB525_DACSENSE_GREEN_DAC                   0x02
#define RGB525_DACSENSE_RED_DAC                     0x01

	/* VRAM Mask Low 0x0090 rw */

#define RGB525_MASKLOW_VRAM_PIXELS_28_TO_31         0x80
#define RGB525_MASKLOW_VRAM_PIXELS_24_TO_27         0x40
#define RGB525_MASKLOW_VRAM_PIXELS_20_TO_23         0x20
#define RGB525_MASKLOW_VRAM_PIXELS_16_TO_19         0x10
#define RGB525_MASKLOW_VRAM_PIXELS_12_TO_15         0x08
#define RGB525_MASKLOW_VRAM_PIXELS_08_TO_11         0x04
#define RGB525_MASKLOW_VRAM_PIXELS_04_TO_07         0x02
#define RGB525_MASKLOW_VRAM_PIXELS_00_TO_03         0x01

	/* VRAM Mask High 0x0091 rw */

#define RGB525_MASKHIGH_VRAM_PIXELS_60_TO_63        0x80
#define RGB525_MASKHIGH_VRAM_PIXELS_56_TO_59        0x40
#define RGB525_MASKHIGH_VRAM_PIXELS_52_TO_55        0x20
#define RGB525_MASKHIGH_VRAM_PIXELS_48_TO_51        0x10
#define RGB525_MASKHIGH_VRAM_PIXELS_44_TO_47        0x08
#define RGB525_MASKHIGH_VRAM_PIXELS_40_TO_43        0x04
#define RGB525_MASKHIGH_VRAM_PIXELS_36_TO_39        0x02
#define RGB525_MASKHIGH_VRAM_PIXELS_32_TO_35        0x01

	/* Index Control rw */

#define RGB525_INDEXCTRL_AUTO_INCREMENT             0x01

#ifdef	__cplusplus
}
#endif

#endif	/* !_RGB525REG_H */
