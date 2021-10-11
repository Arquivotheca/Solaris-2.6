/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992-1994 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)ws_subr.c	1.13	96/06/02 SMI"

#include "sys/types.h"
#include "sys/cmn_err.h"
#include "sys/stream.h"
#include "sys/kd.h"

/*
 *
 */

void
ws_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp)
{
	mblk_t	*tmp;

	mp->b_datap->db_type = M_IOCACK;
	if ((tmp = unlinkb(mp)) != (mblk_t *)NULL)
		freeb(tmp);
	iocp->ioc_count = iocp->ioc_error = 0;
	qreply(qp, mp);
}

/*
 *
 */

void
ws_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int error)
{
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = -1;
	iocp->ioc_error = error;
	qreply(qp, mp);
}
