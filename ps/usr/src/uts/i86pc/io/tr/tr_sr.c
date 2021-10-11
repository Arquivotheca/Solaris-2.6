/*
 * Copyright (c) 1993, 1996 by Sun Microsystems, Inc.
 */
#pragma ident	"@(#)tr_sr.c	1.4	96/06/07 SMI"

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/time.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/strlog.h>
#include <sys/cmn_err.h>
#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <sys/kstat.h>
#include "sys/tr.h"
#include "sys/trreg.h"

struct srtab	*sr_hash_tbl[SR_HASH_SIZE];
int		tr_timeout;
kmutex_t	tr_srlock;			/* lock source routes */

extern struct ether_addr tokenbroadcastaddr1;
extern struct ether_addr tokenbroadcastaddr2;

struct srtab **
tr_sr_hash(unchar *addr, int addr_length)
{
	u_int hashval = 0;

	while (--addr_length >= 0)
		hashval ^= *addr++;

	return (&sr_hash_tbl[hashval % SR_HASH_SIZE]);
}

struct srtab *
tr_sr_lookup_entry(trd_t *trdp, unchar *macaddr)
{
	struct srtab *sr;

	for (sr = *tr_sr_hash(macaddr, MAC_ADDR_LEN); sr; sr = sr->sr_next)
		if (sr->sr_tr == trdp && !ether_cmp(macaddr, sr->sr_mac))
			return (sr);

	return ((struct srtab *) 0);
}

struct srtab *
tr_sr_lookup(trd_t *trdp, unchar *macaddr)
{
	struct srtab *sr;

	if (!(sr = tr_sr_lookup_entry(trdp, macaddr)))
		return ((struct srtab *)0);

	if (sr->sr_flags & SRF_RESOLVED)
		return (sr);

	return ((struct srtab *)0);
}

struct srtab *
tr_sr_create_entry(trd_t *trdp, unchar *macaddr)
{
	struct srtab *sr;
	struct srtab **srp;
	struct tr_ri *rtp;

	/*
	 * do not make entries for broadcast addresses
	 */
	if (!ether_cmp(&tokenbroadcastaddr1, macaddr) ||
			!ether_cmp(&tokenbroadcastaddr2, macaddr)) {
		return ((struct srtab *)0);
	}

	srp = tr_sr_hash(macaddr, MAC_ADDR_LEN);

	for (sr = *srp; sr; sr = sr->sr_next)
		if (sr->sr_tr == trdp && !ether_cmp(macaddr, sr->sr_mac)) {
			return (sr);
		}

	sr = kmem_zalloc(sizeof (struct srtab), KM_NOSLEEP);
	if (!sr)
		cmn_err(CE_PANIC, "TR:tr_sr_create_entry kmem_alloc failed");

	trdp->trd_statsd->trc_sralloc++;

	sr->sr_flags = SRF_PENDING;
	bcopy((caddr_t)macaddr, (caddr_t)sr->sr_mac, MAC_ADDR_LEN);
	sr->sr_tr = trdp;
	sr->sr_timer = 0;

	rtp = &sr->sr_ri;

	rtp->len = 2;
	rtp->dir = 0;
	rtp->mtu = trdp->trd_bridgemtu;
	rtp->rt = RT_APE;  /* All paths explorer */

	sr->sr_next = *srp;
	*srp = sr;

	return (sr);
}

void
tr_sr_timeout()
{
	struct srtab **srp, *sr;
	int i;

	mutex_enter(&tr_srlock);
	/*
	 * Walk through the table, deleting stuff that is old
	 */
	for (i = 0; i < SR_HASH_SIZE; i++) {

		for (srp = &sr_hash_tbl[i]; (sr = *srp) != NULL; ) {

			if (++sr->sr_timer == SR_TIMEOUT) {

				*srp = sr->sr_next;
				ASSERT(sr->sr_tr);
				++sr->sr_tr->trd_statsd->trc_srfree;

				kmem_free((char *)sr, sizeof (struct srtab));
			} else
				srp = &sr->sr_next;
		}
	}
	mutex_exit(&tr_srlock);

	tr_timeout = timeout((void (*)())tr_sr_timeout, 0, 60*HZ);
}

void
tr_sr_init(void)
{
	/*
	 * Start the timer, timeout every min.
	 */
	tr_timeout = timeout((void (*)())tr_sr_timeout, 0, 60*HZ);
}
