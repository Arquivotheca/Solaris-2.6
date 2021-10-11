/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_TGTP_H
#define	_SYS_DKTP_TGTP_H

#pragma ident	"@(#)tgtp.h	1.5	94/09/03 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#define	TP_VARIABLE	0x0001

struct	tgtp_ext {
	int		tg_type;
	int		tg_blksz;
};

struct	tgtp_obj {
	opaque_t		tg_data;
	struct tgtp_objops	*tg_ops;
	struct tgtp_ext		*tg_ext;
	struct tgtp_ext		tg_extblk;	/* extended blk defined	*/
						/* for easy of alloc	*/
};

struct	tgtp_objops {
	int	(*tg_init)();
	int	(*tg_free)();
	int	(*tg_probe)();
	int 	(*tg_attach)();
	int 	(*tg_chk_unload)();
	int 	(*tg_detach)();
	int	(*tg_open)();
	int	(*tg_close)();
	int	(*tg_ioctl)();
	int	(*tg_strategy)();
	int	(*tg_partial_xfer)();
	int	tg_resv[2];
};

#define	TGTP_GETTYPE(X) (((struct tgtp_obj *)(X))->tg_ext->tg_type)
#define	TGTP_GETBLKSZ(X) (((struct tgtp_obj *)(X))->tg_ext->tg_blksz)

#define	TGTP_INIT(X, devp) (*((struct tgtp_obj *)(X))->tg_ops->tg_init) \
	(((struct tgtp_obj *)(X))->tg_data, (devp))
#define	TGTP_FREE(X) (*((struct tgtp_obj *)(X))->tg_ops->tg_free) ((X))
#define	TGTP_PROBE(X, WAIT) (*((struct tgtp_obj *)(X))->tg_ops->tg_probe) \
	(((struct tgtp_obj *)(X))->tg_data, (WAIT))
#define	TGTP_ATTACH(X) (*((struct tgtp_obj *)(X))->tg_ops->tg_attach) \
	(((struct tgtp_obj *)(X))->tg_data)
#define	TGTP_CHK_UNLOAD(X) (*((struct tgtp_obj *)(X))->tg_ops->tg_chk_unload) \
	(((struct tgtp_obj *)(X))->tg_data)
#define	TGTP_DETACH(X) (*((struct tgtp_obj *)(X))->tg_ops->tg_detach) \
	(((struct tgtp_obj *)(X))->tg_data)
#define	TGTP_OPEN(X, dev, flag) (*((struct tgtp_obj *)(X))->tg_ops->tg_open) \
	(((struct tgtp_obj *)(X))->tg_data, (dev), (flag))
#define	TGTP_CLOSE(X, dev, flag) (*((struct tgtp_obj *)(X))->tg_ops->tg_close) \
	(((struct tgtp_obj *)(X))->tg_data, (dev), (flag))
#define	TGTP_STRATEGY(X, bp) (*((struct tgtp_obj *)(X))->tg_ops->tg_strategy) \
	(((struct tgtp_obj *)(X))->tg_data, (bp))
#define	TGTP_IOCTL(X, cmd, arg, flag) \
	(*((struct tgtp_obj *)(X))->tg_ops->tg_ioctl) \
	(((struct tgtp_obj *)(X))->tg_data, (cmd), (arg), (flag))
#define	TGTP_PARTIAL_XFER(X) \
	(*((struct tgtp_obj *)(X))->tg_ops->tg_partial_xfer) \
	(((struct tgtp_obj *)(X))->tg_data)

/*
 * Maximum variable length record size for a single request
 */
#define	TP_MAXRECSIZE_VARIABLE	65535

/*
 * If the requested record size exceeds TP_MAXRECSIZE_VARIABLE,
 * then the following define is used.
 */
#define	TP_MAXRECSIZE_VARIABLE_LIMIT	65534

#define	TP_MAXRECSIZE_FIXED	(128<<10)	/* maximum fixed record size */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_DKTP_TGTP_H */
