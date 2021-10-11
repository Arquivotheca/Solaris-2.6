/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CORVETTE_XMCA_H
#define _CORVETTE_XMCA_H

#pragma	ident	"@(#)xmca.h	1.1	95/01/28 SMI"

/*
 * xmca.h:	stuff for generic MCA support
 */

#ifdef	__cplusplus
extern "C" {
#endif


#ifndef	TRUE
#define	TRUE	1
#define FALSE	0
#endif

#define	MCA_SETUP_PORT	0x96
#define	MCA_SETUP_ON	((unchar)0x08)
#define	MCA_SETUP_MASK	((unchar)~0x0f)

#define	MCA_POS_BASE	0x100
#define MCA_POS_MAX	0x107
#define	MCA_ID_PORT	(MCA_POS_BASE + 0)
#define	MCA_SETUP_102	(MCA_POS_BASE + 2)


int	mca_find_slot(ushort card_id, int port_num, unchar port_mask,
				      unchar port_val, unchar *slotp);
unchar	mca_getb(unchar slot, ushort port_num);
unchar	mca_getw(unchar slot, ushort port_num);
unchar	mca_getl(unchar slot, ushort port_num);
void	mca_putb(unchar slot, ushort port_num, unchar port_val);
void	mca_putw(unchar slot, ushort port_num, ushort port_val);
void	mca_putl(unchar slot, ushort port_num, ulong port_val);

#ifdef	__cplusplus
}
#endif

#endif /* _CORVETTE_XMCA_H */
