
/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)ncr_sni.h	1.1	94/08/26 SMI"

#define	CMOS_ADDR	0x70
#define	CMOS_DATA	0x71
#define	GET_CMOS_BYTE(addr) ((void)outb(CMOS_ADDR, (addr)), inb(CMOS_DATA))

#define	NCR53C710_SNI_CTL1_ADDR		0x840
#define	NCR53C710_SNI_CTL2_ADDR		0x800

#define	SNI_CMOS_IRQ1_MASK		0x0c
#define	SNI_CMOS_IRQ2_MASK		0x30
#define	SNI_CMOS_ENABLE_MASK		0xc0


/* the controller 1 and 2 enable/disable bits */
#define	SNI_CMOS_SCSI_DISABLED		0x00
#define	SNI_CMOS_SCSI_1_2		0x40
#define	SNI_CMOS_SCSI_1_ONLY		0x80
#define	SNI_CMOS_SCSI_2_1		0xc0


/* controller 1 IRQ bits */
#define	SNI_CMOS_SCSI_IRQ1_10		0x00
#define	SNI_CMOS_SCSI_IRQ1_11		0x04
#define	SNI_CMOS_SCSI_IRQ1_14		0x08
#define	SNI_CMOS_SCSI_IRQ1_15		0x0c

/* controller 2 IRQ bits */
#define	SNI_CMOS_SCSI_IRQ2_10		0x00
#define	SNI_CMOS_SCSI_IRQ2_11		0x10
#define	SNI_CMOS_SCSI_IRQ2_14		0x20
#define	SNI_CMOS_SCSI_IRQ2_15		0x30

