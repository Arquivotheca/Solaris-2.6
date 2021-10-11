/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)ppi.h	1.1	93/10/29 SMI"

/********************************************************************************; */
/* LPT Control Port Equates							; */
/*-------------------------------------------------------------------------------; */
#define PPI_RAS			0x08
#define PPI_RDS			0x02
#define PPI_STROBE		0x01

/*******************************************************************************; */
/* LPT Status Port Equates							; */
/*------------------------------------------------------------------------------; */
#define PPI_NOT_BUSY		0x80
#define DQNT_MASK_HI		0xb8					/* high half byte mask */
#define DQNT_MASK_LO		0x47					/* low half byte mask */

/*******************************************************************************; */
/* General Register Equates							; */
/*------------------------------------------------------------------------------; */
#define GENREG0			0x10
#define GENREG1			0x11
#define GENREG2			0x12
#define GENREG3			0x13
#define GENREG4			0x14
#define GENREG5			0x15
#define GENREG6			0x16
#define GENREG7			0x17
#define GENREG8			0x18

/*******************************************************************************; */
/* Control Register 0 Equates							; */
/*------------------------------------------------------------------------------; */
#define GPR_SEL			0x10
#define PPI_READ		0x20
#define PPI_FALLING_EDGES	0x40
#define PPI_RISING_EDGES	0x80

/*******************************************************************************; */
/* Control Register 1 Equates							; */
/*------------------------------------------------------------------------------; */


/*******************************************************************************; */
/* Control Register 2 Equates							; */
/*------------------------------------------------------------------------------; */
#define PPI_READ_EDGE		0x01
#define PPI_READ_PULSE		0x02
#define PPI_USE_STROBE		0x04
#define PPI_WRITE_EDGE		0x08
#define PPI_RDS_INVERT		0x10
#define PPI_BUSY_INVERT		0x20
#define PPI_COWCATCHER		0x00
#define PPI_FASTCOW		0x40
#define PPI_DELAY		0x80
#define PPI_NO_DELAY		0xc0

/*******************************************************************************; */
/* Control Register 3 Equates 							; */
/*------------------------------------------------------------------------------; */
#define PPI_TDG_20MZ		0x00
#define PPI_TDG_10MZ		0x10
#define PPI_TDG_5MHZ		0x20
#define PPI_TDG_2_5MHZ		0x30

#define PPI_BDG_20MZ		0x00
#define PPI_BDG_10MZ		0x40
#define PPI_BDG_5MHZ		0x80
#define PPI_BDG_2_5MHZ		0xc0

/*******************************************************************************; */
/* General Register 0 Equates 							; */
/*------------------------------------------------------------------------------; */
#define PPI_IRQ_ENABLE			0x01
#define PPI_IRQ_INVERT			0x02

/*******************************************************************************; */
/* General Register 1 Equates 							; */
/*------------------------------------------------------------------------------; */
#define PPI_EEDI			0x01
#define PPI_EESK			0x02
#define PPI_EECS			0x04

/*// Read */
#define PPI_EEDO			0x01

/*******************************************************************************; */
/* General Register 2 Equates 							; */
/*------------------------------------------------------------------------------; */
#define PPI_RAS_INVERT			0x01
#define PPI_PE_INVERT			0x02
#define PPI_SLCT_INVERT			0x04
#define PPI_ERR_INVERT			0x08
#define PPI_DQNXACK			0x10
#define PPI_USE_PARITY			0x20

/*******************************************************************************; */
/* General Register 4 Equates 							; */
/*------------------------------------------------------------------------------; */
#define PPI_COUNT_RISE			0x04
#define PPI_COUNT_FALL			0x08
#define PPI_COUNT_BOTH			0x10
#define PPI_COUNT_RAS			0x20
#define PPI_COUNT_NO_HYSTERESIS		0x40
#define PPI_COUNT_HYSTERESIS		0x80

/*******************************************************************************; */
/* General Register 5 Equates 							; */
/*------------------------------------------------------------------------------; */
#define PPI_GPR_0			001
#define PPI_GPR_1			0x02
#define PPI_WATCHDOG			0x10
#define PPI_PPX				0x0c


