/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)pp.h	1.1	93/10/29 SMI"

#define	H_TRUE			-1
#define	H_FALSE			0

#define BIOS_LPT_IRQ			0 + 0x17
#define BIOS_LPT_LOCATION		word ptr BIOS_LPT_IRQ*4

/*****************************************************************************************************************/
/* Parallel Port Definitions											 */
/*---------------------------------------------------------------------------------------------------------------*/
/* LPT Port Address Table */

#define BIOS_DATA_SEG			0x40					/* BIOS data segment */
#define LPT_TABLE			0x08					/* lpt address table  */
#define LPT_TABLE_SIZE			0x06					/* size of bios lpt table */
#define LPT_BOGUS			0x01					/* Bogus address for Windows VPD */
#define EQUIPMENT_FLAG			0x10					/* equipment flag in BIOS data seg */

/* Control Register Bit Definitions */
#define LPT_STROBE			0x01
#define AUTO_FEED			0x02
#define NOT_RESET			0x04
#define SELECT_IN			0x08
#define LPT_IRQ_ENB			0x10					/* interrupt request enable bit */
#define DIR_CTRL			0x20
#define DIR_RESERVED			0xC0					/* reserved direction ctrl bits */

/* Status Port Bit Definitions */
#define LPT_IRQ				0x40					/* interrupt request bit */
#define LPT_PE				0x20					/* paper end bit */

/* PS/2 programmable option select definitions */
#define SYS_SERV			0x15					/* system services interrupt */
#define POS_CTRL			0xc400					/* pos control sub-function */
#define SYS_ENB_REG			0x094					/* system board enable/setup register */
#define SYS_POS_REG_2			0x102					/* system board pos register 2 */
#define SETUP_SYS_BRD			0x7f					/* setup system board functions command */
#define ENABLE_SYS_BRD			0xff					/* enable system board functions */
#define PP_SELECT_MASK			0x60					/* logical printer port selection mask */
#define DIS_PP_EXT_MODE			0x80					/* disable parallel port extended mode bit */
#define MODEL_BYTE_HIGH_ADDRESS      	0x0f                                  	/* physical address of model byte in ROM */
#define MODEL_BYTE_LOW_ADDRESS       	0xfffe                                  /* physical address of model byte in ROM */
#define MODEL1_PS2                   	0xf8                                    /* PS2 identification */
#define MODEL2_PS2                   	0xfc                                    /* PS2 identification */
#define SUBMODEL1_PS2                	0x04                                    /* PS2 submodel identification */
#define SUBMODEL2_PS2                	0x05                                    /* PS2 submodel identification */

/* EPP INT 17 definitions */
#define EPP_ENABLE_FCN			0x40
#define EPP_DISABLE_FCN			0x41
#define EPP_STATUS_FCN			0x42

/* COMPAQ Specific Definitions */
#define CPQ_READ_ENB			0x0
#define CPQ_WRITE_ENB			0x80
#define CPQ_DIR_PORT			0x65
#define COMPAQ_SIGNATURE_ADDR		0xffea

