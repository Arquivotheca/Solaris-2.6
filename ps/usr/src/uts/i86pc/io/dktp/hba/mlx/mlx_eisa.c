/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)mlx_eisa.c	1.1@(#)	1.1	95/10/16 SMI"

#include <sys/dktp/mlx/mlx.h>
#include <sys/eisarom.h>
#include <sys/nvm.h>

extern int eisa_nvm();

#define	EISA_CFG0	0xc80	/* EISA configuration port 0 */
#define	REV_MASK	0xF0FFFFFF


/*
 * check if an EISA slot has the right type of board
 */
bool_t
eisa_probe(ioadr_t	slotadr,
		ulong	board_id)
{
	ulong	bid;
	char	buf[sizeof (short) + sizeof (NVM_SLOTINFO) +
			sizeof (NVM_FUNCINFO)];

	/* Check nvram FIRST for match. Note how SLOT|CFUNCTION bounds rval */
	if (!eisa_nvm(buf,
			(EISA_SLOT | EISA_CFUNCTION | EISA_BOARD_ID),
			slotadr>>12,
			0,
			board_id, REV_MASK))
		return (FALSE);

	outb(slotadr + EISA_CFG0, 0xFF);	/* precharge the register */
	bid = inl(slotadr + EISA_CFG0);

	/* Slot is empty if high bit of low ID byte is set */
	if (bid & 0x80)
		return (FALSE);

	/* Check for match with a known board, ignore the revision number */
	return ((board_id & REV_MASK) == (bid & REV_MASK));
}


/*
 * Check the EISA function records for this slot one at a
 * time until we get the one that defines the irq.
 */
bool_t
eisa_get_irq(ioadr_t	 slotadr,
		unchar	*irqp)
{
	struct	{
		short	slotnum;
		NVM_SLOTINFO	slot;
		NVM_FUNCINFO	func;
	} buff;
	int	function_num;
	int	slotnum = (slotadr >> 12);
	int	rc;

	/* check all the functions for this slot until the irq is found */
	for (function_num = 0; ; function_num++) {
		/* get the slot record and the next function record */
		rc = eisa_nvm(&buff,
			(EISA_SLOT | EISA_CFUNCTION),
			slotnum, function_num);

		if (rc == 0) {
			/* end of functions, no irq defined */
			return (FALSE);
		}

		if (slotnum != buff.slotnum) {
			/* shouldn't happen */
			return (FALSE);
		}

		/* check if we got the irq function record */
		if (buff.func.fib.irq)
			break;
	}
	*irqp = buff.func.un.r.irq[0].line;
	return (TRUE);
}

/*ARGSUSED*/
int
mlx_get_reg_eisa(mlx_t	*mlxp,
			int	*regp,
			int	 reglen)
{
	/* EISA, reg is the same as the slot address */
	mlxp->reg = MLX_ADDR(*regp);
	return (MLX_EISA_RNUMBER);
}
