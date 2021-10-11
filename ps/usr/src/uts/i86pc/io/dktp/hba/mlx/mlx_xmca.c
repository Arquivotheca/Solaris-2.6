/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)mlx_xmca.c	1.1@(#)	1.1	95/10/16 SMI"

#include <sys/dktp/mlx/mlx.h>

static void
mc_pos_enter(unchar slot, unchar *tempp)
{
	*tempp = inb(MCA_SETUP_PORT);
	*tempp &= MCA_SETUP_MASK;
	outb(MCA_SETUP_PORT, (*tempp | MCA_SETUP_ON | slot));
}

/*ARGSUSED*/
static void
mc_pos_exit(unchar slot, unchar *tempp)
{
	outb(MCA_SETUP_PORT, *tempp);
}

static int
mc_find_slot(ushort card_id, int port_num, unchar port_mask, unchar port_val,
		unchar slot, unchar eslot, unchar *slotp)
{
	unchar	setup_temp;
	ushort	pos_id;

	if (slotp)
		*slotp = 0xff;

	/* validity check the port_num */
	if (port_num) {
		if (port_num < MCA_POS_BASE || port_num > MCA_POS_MAX)
			return (FALSE);
	}

	setup_temp = inb(MCA_SETUP_PORT);
	setup_temp &= MCA_SETUP_MASK;
	for (; slot < 8 && slot < eslot; slot++)  {
		outb(MCA_SETUP_PORT, (setup_temp | MCA_SETUP_ON | slot));

		pos_id = inw(MCA_ID_PORT);
		if (pos_id != card_id)
			continue;

		/* check the specified port for the specified value */
		if (port_num) {
			unchar	temp;

			temp = inb(port_num);
			temp &= port_mask;
			if (temp != port_val)
				continue;
		}

		outb(MCA_SETUP_PORT, setup_temp);
		if (slotp)
			*slotp = slot;
		return (TRUE);
	}
	outb(MCA_SETUP_PORT, setup_temp);
	return (FALSE);
}


static unchar
mc_getb(unchar	slot, ushort port_num)
{
	unchar	setup_temp;
	unchar	port_val;

	/* validity check the port_num */
	if (port_num < MCA_POS_BASE || port_num > MCA_POS_MAX)
		return (0xff);

	mc_pos_enter(slot, &setup_temp);

	port_val = inb(port_num);

	mc_pos_exit(slot, &setup_temp);
	return (port_val);
}

/* mca pos setup register specific routines */

/*
 * Function:	mc_bba()
 *
 * Purpose:	mc_ the BIOS Base Address value of the DMC960 controller
 *		specified in the "setupPort102".
 *
 * Input:    	setupPort102 :: (unchar) value contained at micro-channel port
 *		address 102.
 *
 * Returns:  	biosBaseAddress :: BIOS Base Address value for the current
 *		DMC960 controller found at micro-channel port address 102.
 *
 * Notes:	The BIOS Base Address is located in the 5-2 bit positions of the
 *		setupPort102.  The corresponding values are:
 *
 * Port 102H:	Bit	7  6  5  4  3  2  1  0
 * ----------	---	-  -  -  -  -  -  -  -
 *			x  x  0  0  0  0  x  x	BIOS base = 0C0000H
 *			x  x  0  0  0  1  x  x	BIOS base = 0C2000H
 *			x  x  0  0  1  0  x  x	BIOS base = 0C4000H
 *			x  x  0  0  1  1  x  x	BIOS base = 0C6000H
 *
 *			x  x  0  1  0  0  x  x	BIOS base = 0C8000H
 *			x  x  0  1  0  1  x  x	BIOS base = 0CA000H
 *			x  x  0  1  1  0  x  x	BIOS base = 0CC000H
 *			x  x  0  1  1  1  x  x	BIOS base = 0CE000H
 *
 *			x  x  1  0  0  0  x  x	BIOS base = 0D0000H
 *			x  x  1  0  0  1  x  x	BIOS base = 0D2000H
 *			x  x  1  0  1  0  x  x	BIOS base = 0D4000H
 *			x  x  1  0  1  1  x  x	BIOS base = 0D6000H
 *
 *			x  x  0  1  0  0  x  x	BIOS base = 0D8000H
 *			x  x  0  1  0  1  x  x	BIOS base = 0DA000H
 *			x  x  0  1  1  0  x  x	BIOS base = 0DC000H
 *			x  x  0  1  1  1  x  x	BIOS base = 0DE000H
 */
static unchar
*mc_bba(unchar setupPort102)
{
	unchar		*biosBaseAddr;
	ushort		bbaIndex;
	static ulong 	memmap[16] = {	/* valid bios addresses	*/
		0xc0000, 0xc2000, 0xc4000, 0xc6000,
		0xc8000, 0xca000, 0xcc000, 0xce000,
		0xd0000, 0xd2000, 0xd4000, 0xd6000,
		0xd8000, 0xda000, 0xdc000, 0xde000
	};

	bbaIndex = (setupPort102 & BIOS_BASE_ADDR_MASK);
	bbaIndex >>= BBA_MASK_SHIFT;

	biosBaseAddr = (unchar *)(memmap[bbaIndex]);

	return (biosBaseAddr);
}	/* mc_bba() */


/*
 * Function:	mc_ioba()
 *
 * Purpose:  	mc_ the IRQ value of the DMC960 controller specified in the
 *		"setupPort105".
 *
 * Input:	setupPort105 :: (unchar) value contained at micro-channel port
 *		address 105.
 *
 * Returns:  	ioBaseAddr :: The I/O Base Address value for the current DMC960
 *		controller found at micro-channel port address 105.
 *
 * Notes:	The I/O Base Address is located in the 5-2 bit positions of the
 *		setupPort105.  The corresponding values are:
 *
 * Port 105H:	Bit	7  6  5  4  3  2  1  0
 * ----------	---	-  -  -  -  -  -  -  -
 *			x  x  0  0  0  x  x  x	I/O base = 01C00H
 *			x  x  0  0  1  x  x  x	I/O base = 03C00H
 *			x  x  0  1  0  x  x  x	I/O base = 05C00H
 *			x  x  0  1  1  x  x  x	I/O base = 07C00H
 *
 *			x  x  1  0  0  x  x  x	I/O base = 09C00H
 *			x  x  1  0  1  x  x  x	I/O base = 0BC00H
 *			x  x  1  1  0  x  x  x	I/O base = 0DC00H
 *			x  x  1  1  1  x  x  x	I/O base = 0FC00H
 */
static ushort
mc_ioba(unchar setupPort105)
{
	ushort		segment	  = 0x0;
	ushort		iobaIndex = 0x0;
	static ushort	iomap[8]  = {	/* valid io addresses	*/
		0x1c00, 0x3c00, 0x5c00, 0x7c00,
		0x9c00, 0xbc00, 0xdc00, 0xfc00
	};


	iobaIndex =  (setupPort105 & IO_BASE_ADDR_MASK);
	iobaIndex >>= IOBA_MASK_SHIFT;

	segment = iomap[iobaIndex];

	return (segment);
}	/* mc_ioba() */


/*
 * check if an MC slot has the right type of board
 */
bool_t
mc_probe(ioadr_t	slotadr,
		ulong	board_id)
{
	/*
	 * As the probe of this particular card and its channels is fairly
	 * simple and fast it is not worth sharing it with other hba instances
	 * (channels).  The alternative would be to grab the global_mutex
	 * and traverse the list headed by cards to see if any t with
	 * the same reg already exists (attached) and if so, return
	 * DDI_PROBE_SUCCESS, otherwise proceed with the following.
	 * ... but it is not worth it!
	 */
	slotadr = (slotadr >> 12);	/* use slotadr for the slot number */
	if (mc_find_slot(board_id, DUMMY, DUMMY, DUMMY, slotadr,
				slotadr + 1, (unchar *)DUMMY) == FALSE)
		return (FALSE);
	return (board_id);
}


/*
 * Check the MC function records for this slot one at a
 * time until we get the one that defines the irq.
 */
bool_t
mc_get_irq(ioadr_t	 slotadr,
		unchar	*irqp)
{
	short	slotnum = MLX_SLOT(slotadr);
	u_char	irq;
	int	rc = TRUE;

	irq = (u_char)(mc_getb(slotnum, 0x102) &
		DMC_IRQ_MASK) >> DMC_IRQ_MASK_SHIFT;

	switch (irq & 0x03) {
	case 0:
		*irqp = MLX_IRQ14;
		break;
	case 1:
		*irqp = MLX_IRQ12;
		break;
	case 2:
		*irqp = MLX_IRQ11;
		break;
	case 3:
		*irqp = MLX_IRQ10;
		break;
	default:
		rc = FALSE;
	}

	return (rc);
}

/*ARGSUSED*/
int
mlx_get_reg_mc(mlx_t	*mlxp,
			int	*regp,
			int	reglen)
{
	/*
	 * The actual io register and shared memory
	 * addresses are discovered and stored here.
	 */
	mlxp->reg = mc_ioba(mc_getb(MLX_SLOT(*regp), 0x105));
	regp[1] = (int)mc_bba(mc_getb(MLX_SLOT(*regp), 0x102));
	regp[2] = 0x2000;

	return (MLX_EISA_RNUMBER);
}
