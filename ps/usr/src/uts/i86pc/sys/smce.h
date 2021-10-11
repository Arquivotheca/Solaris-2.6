/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _SYS_SMCE_H
#define	_SYS_SMCE_H

#pragma ident	"@(#)smce.h	1.4	95/03/22 SMI"

/*
 * Hardware specific driver declarations for the SMC Elite32 EISA
 * Dual channel driver conforming to the Generic LAN Driver model.
 */

#ifdef	__cplusplus
extern "C" {
#endif

/* debug flags */
#define	SMCETRACE	0x01
#define	SMCEERRS	0x02
#define	SMCERECV	0x04
#define	SMCEDDI		0x08
#define	SMCESEND	0x10
#define	SMCEINT		0x20
#define	SMCEALAN	0x40

/* Misc */
#define	SMCEHIWAT	32768		/* driver flow control high water */
#define	SMCELOWAT	4096		/* driver flow control low water */
#define	SMCEMAXPKT	1500		/* maximum media frame size */
#define	SMCEIDNUM	0		/* should be a unique id; zero works */

/* board state */
#define	SMCE_IDLE	0
#define	SMCE_WAITRCV	1
#define	SMCE_XMTBUSY	2
#define	SMCE_ERROR	3

/* EISA */
#define	SMCE_MAX_EISABUF	(16*1024)
#define	SMCE_MFG_ID		0x0
#define	SMCE_PRODUCT_ID		0x2
#define	SMCE_PRODID_MASK	0xffff
#define	SMCE_ID1		0xa34d
#define	SMCE_ID2		0x1001

/* per channel driver specific declarations */
struct smceinstance {
	int		channel;	/* which channel to use on the board */
	dev_info_t	*devinfo;
	struct smparam	*smp;		/* used for the wrapper to LMAC */
	struct smceboard *smcebp;	/* points to per board structure */
};

/* per board structure */
struct smceboard {
	kmutex_t		smcelock;	/* per board lock */
	struct smceinstance	*smce1p;	/* points to channel 1 */
	struct smceinstance	*smce2p;	/* points to channel 2 */
	int			board_initialized;
};

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_SMCE_H */
