/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)xmca.c	1.1	95/01/28 SMI"

#include <sys/types.h>
#include <sys/modctl.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/cmn_err.h>

#include "xmca.h"

#ifdef netyet

#define MCA_MUTEX_ENTER(mutexp)	mutex_enter(mutexp)
#define MCA_MUTEX_EXIT(mutexp)	mutex_exit(mutexp)

extern struct mod_ops mod_miscops;

/*
 * Module linkage information for the kernel.
 */
static struct modlmisc modlmisc = {
	&mod_miscops, "interim MCA Bus support functions "
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modlmisc, NULL
};


static	kmutex_t mca_mutex;


int
_init(void)
{
	dev_info_t	*dip = ddi_root_node();

#ifdef	MCA_DEBUG
	debug_enter("\nmca init\n");
#endif	MCA_DEBUG

	mutex_init(&mca_mutex, "MCA Global Mutex", MUTEX_DRIVER, (void *)NULL);

	return (mod_install(&modlinkage));
}

int
_fini(void)
{
	return (mod_remove(&modlinkage));
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}



#else /* notyet */

#define MCA_MUTEX_ENTER(mutexp)	
#define MCA_MUTEX_EXIT(mutexp)

#endif /* notyet */

static void
mca_pos_enter(	unchar	 slot,
		unchar	*tempp )
{

	*tempp = inb(MCA_SETUP_PORT);
	*tempp &= MCA_SETUP_MASK;
	outb(MCA_SETUP_PORT, (*tempp | MCA_SETUP_ON | slot));
	return;
}

static void
mca_pos_exit(	unchar	slot,
		unchar	*tempp )
{
	outb(MCA_SETUP_PORT, *tempp);
	return;
}



int
mca_find_slot(	ushort	card_id,
		int	port_num,
		unchar	port_mask,
		unchar	port_val,
		unchar	*slotp )
{
	unchar	slot;
	unchar	setup_temp;
	ushort	pos_id;

	if (slotp)
		*slotp = 0xff;

	/* validity check the port_num */
	if (port_num) {
		if (port_num < MCA_POS_BASE || port_num > MCA_POS_MAX)
			return (FALSE);
	}

	MCA_MUTEX_ENTER(&mca_mutex);
	setup_temp = inb(MCA_SETUP_PORT);
	setup_temp &= MCA_SETUP_MASK;
	for (slot= 0 ; slot < 8 ; slot++)  {
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
		MCA_MUTEX_EXIT(&mca_mutex);
		if (slotp)
			*slotp = slot;
		return (TRUE);
	}
	outb(MCA_SETUP_PORT, setup_temp);
	MCA_MUTEX_EXIT(&mca_mutex);
	return (FALSE);
}


unchar
mca_getb(
	unchar	slot,
	ushort	port_num
	)
{
	unchar	setup_temp;
	unchar	port_val;

	/* validity check the port_num */
	if (port_num < MCA_POS_BASE || port_num > MCA_POS_MAX)
		return (0xff);

	MCA_MUTEX_ENTER(&mca_mutex);
	mca_pos_enter(slot, &setup_temp);

	port_val = inb(port_num);

	mca_pos_exit(slot, &setup_temp);
	MCA_MUTEX_EXIT(&mca_mutex);
	return (port_val);
}


unchar
mca_getw(
	unchar	slot,
	ushort	port_num
	)
{
	unchar	setup_temp;
	ushort	port_val;

	/* validity check the port_num */
	if (port_num < MCA_POS_BASE || port_num > MCA_POS_MAX)
		return (0xff);

	MCA_MUTEX_ENTER(&mca_mutex);
	mca_pos_enter(slot, &setup_temp);

	port_val = inw(port_num);

	mca_pos_exit(slot, &setup_temp);
	MCA_MUTEX_EXIT(&mca_mutex);
	return (port_val);
}



unchar
mca_getl(
	unchar	slot,
	ushort	port_num
	)
{
	unchar	setup_temp;
	ulong	port_val;

	/* validity check the port_num */
	if (port_num < MCA_POS_BASE || port_num > MCA_POS_MAX)
		return (0xff);

	MCA_MUTEX_ENTER(&mca_mutex);
	mca_pos_enter(slot, &setup_temp);

	port_val = inl(port_num);

	mca_pos_exit(slot, &setup_temp);
	MCA_MUTEX_EXIT(&mca_mutex);
	return (port_val);
}


void
mca_putb(
	unchar	slot,
	ushort	port_num,
	unchar	port_val
	)
{
	unchar	setup_temp;

	/* validity check the port_num */
	if (port_num < MCA_POS_BASE || port_num > MCA_POS_MAX)
		return;

	MCA_MUTEX_ENTER(&mca_mutex);
	mca_pos_enter(slot, &setup_temp);

	outb(port_num, port_val);

	mca_pos_exit(slot, &setup_temp);
	MCA_MUTEX_EXIT(&mca_mutex);
	return;
}	


void
mca_putw(
	unchar	slot,
	ushort	port_num,
	ushort	port_val
	)
{
	unchar	setup_temp;

	/* validity check the port_num */
	if (port_num < MCA_POS_BASE || port_num > MCA_POS_MAX)
		return;

	MCA_MUTEX_ENTER(&mca_mutex);
	mca_pos_enter(slot, &setup_temp);

	outw(port_num, port_val);

	mca_pos_exit(slot, &setup_temp);
	MCA_MUTEX_EXIT(&mca_mutex);
	return;
}	


void
mca_putl(
	unchar	slot,
	ushort	port_num,
	ulong	port_val
	)
{
	unchar	setup_temp;

	/* validity check the port_num */
	if (port_num < MCA_POS_BASE || port_num > MCA_POS_MAX)
		return;

	MCA_MUTEX_ENTER(&mca_mutex);
	mca_pos_enter(slot, &setup_temp);

	outl(port_num, port_val);

	mca_pos_exit(slot, &setup_temp);
	MCA_MUTEX_EXIT(&mca_mutex);
	return;
}	
