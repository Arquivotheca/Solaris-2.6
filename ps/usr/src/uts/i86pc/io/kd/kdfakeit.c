/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident "@(#)kdfakeit.c	1.2	96/06/02 SMI"


#include "sys/types.h"
#include "sys/cram.h"
#include "sys/ddi.h"
#include "sys/sunddi.h"

int	maxminor = 255;
int	inrtnfirm = 0;

unsigned char
CMOSread(loc)
int loc;
{
	outb(CMOS_ADDR, loc);	/* request cmos address */
	return (inb(CMOS_DATA));	/* return contents */
}

void
softreset(void)
{
}

/*
 * This should be in ml/misc.s implemented as a repeated move instruction.
 */
void
vcopy(caddr_t from, caddr_t to, int count, int dir)
{
	int	i;

	switch (dir) {
	case 0:	/* move up */
		for (i = 0; i < count; i++)
			*to++ = *from++;
		break;

	case 1:	/* move down */
		for (i = 0; i < count; i++)
			*to-- = *from--;
		break;

	default:
		break;
	}
}
