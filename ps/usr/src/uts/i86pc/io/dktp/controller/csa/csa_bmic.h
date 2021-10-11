/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CSA_CSA_BMIC_H
#define	_CSA_CSA_BMIC_H

#pragma	ident	"@(#)csa_bmic.h	1.1	95/05/17 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Memory mapped I/O port addresses
 */
#define	BMIC_BASE	0x0c80	/* all the BMIC ports are offset from here */

#define	CREG_INTERRUPT		0x0C89	/* Interrupt Enable/Control register */

/*
 * doorbell registers and masks
 */
#ifdef ___this_register_is_not_writable_by_the_driver
#define	CREG_DISK_MASK		0x0C8C	/* disk interrupt mask */
#endif

#define	CREG_DISK_DOORBELL	0x0C8D	/* disk doorbell interrupt register */

#define	CREG_SYS_MASK		0x0C8E	/* system interrupt mask */
#define	CREG_SYS_DOORBELL	0x0C8F	/* system doorbell interrupt register */

/*
 * mailboxes
 */
#define	CREG_SUBMIT		0x0C90	/* Command List Submit Channel */
#define	CREG_SUBMIT_ADDR	0x0C90	/* long: command list address */
#define	CREG_SUBMIT_LENGTH	0x0C94	/* short: command list length */
#define	CREG_SUBMIT_RESERVE	0x0C96	/* char: reserved */
#define	CREG_SUBMIT_TAGID	0x0C97	/* char: tag id */

#define	CREG_COMPLETE		0x0C98	/* Command List Complete Channel */
#define	CREG_COMPLETE_ADDR	0x0C98	/* long: command list address */
#define	CREG_COMPLETE_OFFSET	0x0C9C	/* short: request block offset */
#define	CREG_COMPLETE_STATUS	0x0C9E	/* char: list status */
#define	CREG_COMPLETE_TAGID	0x0C9F	/* char: tag id */

#define	CREG_CONFIG		0x0CC0	/* controller configuration */
#define	CREG_HARDWARE_REV	0x0CC1	/* hardware revision */


/*
 * Definitions of bit fields within the above registers
 */

/* CREG_INTERRUPT - System Interupt Enable/Control Register */
#define	CBIT_INTERRUPT_ENABLE	0x01	/* enable interrupts */
#define	CBIT_INTERRUPT_PENDING	0x02	/* interrupt pending */

/* CREG_DISK_DOORBELL - Disk (local) Doorbell Interrupt Register */
#define	CBIT_DDIR_SUBMIT	0x01	/* command list submit */
#define	CBIT_DDIR_CLEAR		0x02	/* command list submit-channel clear */
#define	CBIT_DDIR_RESET		0x80	/* warm reset SMART */


#ifdef ___this_register_is_not_writable_by_the_driver
/* CREG_DISK_MASK - Disk (local) Doorbell Mask register */
#define	CBIT_DDIMR_SUBMIT	0x01	/* command list submit */
#define	CBIT_DDIMR_CLEAR	0x02	/* command list submit-channel clear */
#endif


/* CREG_SYS_DOORBELL - System Doorbell Interrupt Register */
#define	CBIT_SDIR_READY		0x01	/* command list complete notification */
#define	CBIT_SDIR_CLEAR		0x02	/* command list submit-channel clear */

/* CREG_SYS_MASK - System Doorbell Interrupt Mask Register */
#define	CBIT_SDIMR_READY	0x01	/* command list complete notification */
#define	CBIT_SDIMR_CLEAR	0x02	/* command list submit-channel clear */



/* CREG_CONFIG - Controller Configuration Register */
#define	CBIT_CONFIG_SIEN	0x01	/* Standard Interface enabled */
#define	CBIT_CONFIG_SIPR	0x02	/* Standard Interface I/O port select */

#define	CBIT_CONFIG_SPLEDM	0x01	/* SystemPro LED Mode on SMART */
#define	CBIT_CONFIG_FPENA	0x02	/* Front Panel Disable on SMART */

#define	CBIT_CONFIG_BMEN	0x04	/* Bus Master Interrupt disable */
#define	CBIT_CONFIG_PRST	0x08	/* software reset bit on SMART */
#define	CBIT_CONFIG_ALARM	0x08	/* Alarm disable on IDA, IDA-2, IAES */

#define	CBIT_CONFIG_BIE3	0x10	/* Normal Mode IRQ 11 */
#define	CBIT_CONFIG_BIE2	0x20	/* Normal Mode IRQ 10 */
#define	CBIT_CONFIG_BIE1	0x40	/* Normal Mode IRQ 14 */
#define	CBIT_CONFIG_BIE0	0x80	/* Normal Mode IRQ 15 */


/* clear and set bits in an i/o register */
#define	CLR_SET_BITS(reg, clr, set) outb((reg), (inb(reg) & ~(clr)) | (set))



/*
 * The interrupt ACK bits are "sticky" bits. The controller
 * generates an interrupt by setting a bit to 1. The driver
 * acks (i.e., clears the bit) the interrupt by also setting
 * it to 1. Storing a 0 into a bit doesn't clear it.
 * Bits set to 0 "stick" to their prior value.
 */

/*
 * These are the two interrupts the controller can send the driver.
 */
#define	CSA_BMIC_ACK_LIST_COMPLETE(C, P)		\
	outb((P) + CREG_SYS_DOORBELL, CBIT_SDIR_READY)

#define	CSA_BMIC_ACK_SUBMIT_CHNL_CLEAR(C, P)	\
	outb((P) + CREG_SYS_DOORBELL, CBIT_SDIR_CLEAR)

/*
 * The mask bits are 1 to enable an interrupt and 0 to disable.
 * Use the CLR_SET_BITS() macro so that only a single mask changes.
 * The macro reads the old value and twiddles only the specified bit.
 * This should be optimized by keeping a shadow copy of the mask
 * registers in the csa_blk_t???
 */
#define	CSA_BMIC_MASK_COMMAND_COMPLETE(C, P)	\
	CLR_SET_BITS((P) + CREG_SYS_MASK, CBIT_SDIMR_READY, 0)

#define	CSA_BMIC_UNMASK_COMMAND_COMPLETE(C, P)	\
	CLR_SET_BITS((P) + CREG_SYS_MASK, 0, CBIT_SDIMR_READY)

#define	CSA_BMIC_MASK_SUBMIT_CHNL_CLEAR(C, P)	\
	CLR_SET_BITS((P) + CREG_SYS_MASK, CBIT_SDIMR_CLEAR, 0)

#define	CSA_BMIC_UNMASK_SUBMIT_CHNL_CLEAR(C, P)	\
	CLR_SET_BITS((P) + CREG_SYS_MASK, 0, CBIT_SDIMR_CLEAR)


/*
 * The System Interrupt Enable/Control Register has a single bit
 * which overrides the individual interrupt mask bits
 */
/* enable all unmasked bus master interrupts */
#define	CSA_BMIC_ENABLE(C, P)	\
	CLR_SET_BITS(ioaddr + CREG_INTERRUPT, 0, CBIT_INTERRUPT_ENABLE)

/* disable all bus master interrupts */
#define	CSA_BMIC_DISABLE(C, P)	\
	CLR_SET_BITS(ioaddr + CREG_INTERRUPT, CBIT_INTERRUPT_ENABLE, 0)


/*
 * These are the interrupts the driver can send to the controller
 */

/* Notify the controller I've received the previous interrupt status */
#define	CSA_BMIC_SET_COMPLETE_CHNL_CLEAR(C, P)	\
	outb((P) + CREG_DISK_DOORBELL, CBIT_DDIR_CLEAR)

/* Submit a command to the controller */
#define	CSA_BMIC_COMMAND_SUBMIT(C, P)		\
	outb((P) + CREG_DISK_DOORBELL, CBIT_DDIR_SUBMIT)

/*
 * Macro to test for when the controller has accepted the prior command
 */
#define	CSA_BMIC_STATUS_SUBMIT_CHNL_CLEAR(C, P)	\
	((inb((P) + CREG_DISK_DOORBELL) & CBIT_DDIR_SUBMIT) == 0)


#ifdef	__cplusplus
}
#endif

#endif /* _CSA_CSA_BMIC_H */
