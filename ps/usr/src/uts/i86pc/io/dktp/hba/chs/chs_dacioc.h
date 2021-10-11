/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef	_SYS_DKTP_CHS_DACIOC_H
#define	_SYS_DKTP_CHS_DACIOC_H

#pragma	ident	"@(#)chs_dacioc.h	1.2	96/06/18 SMI"

/*
 * The are no public ioctls. These are left overs from mlx driver's
 * code.
 */

#include <sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define	CHS_DACIOC	(('M' << 24) | ('D' << 16) | (0x04 << 8))

#define	CHS_DACIOC_FLUSH	(CHS_DACIOC | 1) /* Flush */

/* no data xfer during dacioc op */
#define	CHS_CCB_DACIOC_NO_DATA_XFER	0x4

typedef struct chs_dacioc_generic_args {
	/*
	 * A byte value is passed to be assigned to a known register
	 * (physical) address.  e.g. opcode.
	 */
	u_char val;
	ushort reg_addr;	/* absolute register address */
} chs_dacioc_generic_args_t;

typedef union chs_dacioc_args {
	/* Type 0 requires no args */

	struct {
		u_char drv;	/* REG7 */
		u_int blk;	/* 26 bits of REG4 REG5 REG6 REG3 */
		u_char cnt;	/* REG2 */
	} type1;

	struct {
		u_char chn;		/* REG2 */
		u_char tgt;		/* REG3 */
		u_char dev_state;	/* REG4 */
	} type2;

	struct {
		u_char test;	/* REG2 */
		u_char pass;	/* REG3 */
		u_char chan;	/* REG4 */
	} type4;

	/* Type 5 */
	u_char param;		/* REG2 */

	struct {
		u_short count;	/* REG2 and REG3 */
		u_int offset;	/* REG4 REG5 REG6 REG7 */
	} type6;

	/* Type generic */
	struct {
		chs_dacioc_generic_args_t *gen_args;
		ulong gen_args_len;	/* total length of gen_args in bytes */

		/*
		 * 1st of the 4 consecutive registers which will contain the
		 * physical address of transfer.  This register will contain
		 * the LSB of that physical address.
		 *
		 * In type1 to type6 this register is 0xzC98 (z is the slot
		 * number), but it may be assigned a different value in
		 * type_gen.
		 */
		ushort xferaddr_reg;
	} type_gen;
} chs_dacioc_args_t;

/*
 * XXX - IBM claims that CHS_DAC_MAX_XFER could be
 * 128 blocks (0x10000). Unfortunately the driver
 * isn't written to take advantage of this length.
 */
#define	CHS_DAC_MAX_XFER	0xF800

typedef struct chs_dacioc {
	chs_dacioc_args_t args;
	caddr_t ubuf;		/* virtual addr of the user buffer */
	ushort ubuf_len; /* length of ubuf in bytes <=  CHS_DAC_MAX_XFER */
	u_char flags;		/* see below */
	ushort status;		/* returned after command completion */
} chs_dacioc_t;		/* type of the 3rd arg to ioctl() */

/* Possible values for flags field of chs_dacioc_t */
#define	CHS_DACIOC_UBUF_TO_DAC	1	/*
						 * data transfer from user
						 * to hba
						 */

#define	CHS_DACIOC_DAC_TO_UBUF	2	/*
						 * data transfer from hba
						 * to user
						 */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_DKTP_CHS_DACIOC_H */
