/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)ncr_clock.c	1.3@(#)	1.3	95/09/13 SMI"

#include <sys/dktp/ncrs/ncr.h>


typedef	struct	{
	caddr_t	s_rate;
	int	s_period;
} srt_t;

/*
 * These are values I accept for the max-sync-rate property
 * in the ncrs.conf file. The max-sync-rate only sets an upper
 * bound on the transfer rate, the target device can always 
 * negotiate something lower.
 */
static	srt_t	syncio_rate_table[] = {
	{ "10.0",	100 },
	{ "10",		100 },
	{ "6.7",	150 },
	{ "6.67",	150 },
	{ "6.66",	150 },
	{ "6.6",	150 },
	{ "5.0",	200 },
	{ "5",		200 },
	{ "4.0",	250 },
	{ "4",		250 },
	{ "3.33",	300 },
	{ "3.3",	300 },
	{ "0.0",	-1 },
	NULL
};

/*
 * The chip uses a two stage divisor chain. The first stage can
 * divide by 1, 1.5, 2, or 3 (except the 53c710 which can't easily
 * divide by 3).  The second stage can divide by values from 4 to 11
 * (inclusive). If the board is using a 40MHz clock this allows sync
 * i/o rates  from 10 MB/sec to 1.212 MB/sec. The following table
 * factors a desired overall divisor into the appropriate values for
 * each of the two stages.  The first and third columns of this table
 * are scaled by a factor of 10 to handle the 1.5 case without using
 * floating point numbers.
 */

typedef	struct ncr_divisor_table {
	int	divisorX10;	/* divisor times 10 */
	unchar	sxfer;		/* sxfer period divisor */
	unchar	sscfX10;	/* synchronous scsi clock divisor times 10 */
} ndt_t;

static	ndt_t	DivisorTable[] = {
	{ 40,	4,	10 },
	{ 50,	5,	10 },
	{ 60,	4,	15 },
	{ 70,	7,	10 },
	{ 75,	5,	15 },
	{ 80,	4,	20 },
	{ 90,	6,	15 },
	{ 100,	5,	20 },
	{ 105,	7,	15 },
	{ 110,	11,	10 },
	{ 120,	8,	15 },
	{ 135,	9,	15 },
	{ 140,	7,	20 },
	{ 150,	10,	15 },
	{ 160,	8,	20 },
	{ 165,	11,	15 },
	{ 180,	9,	20 },
	{ 200,	10,	20 },
	{ 210,	7,	30 },
	{ 220,	11,	20 },
	{ 240,	8,	30 },
	{ 270,	9,	30 },
	{ 300,	10,	30 },
	{ 330,	11,	30 },
	0
};


/*
 * Look at the max-sync-rate property, parse it, and convert
 * it into the corresponding minimum allowed time period in
 * nanoseconds (period = 1/F).
 */

static bool_t
ncr_max_sync_rate_parse(	ncr_t	*ncrp,
				caddr_t	 cp )
{
	caddr_t	savecp;
	int	period;
	int	target;
	int	cnt;

	
	for (target = 0; target < NTARGETS; target++) {
		savecp = cp;
		cnt = 0;

		/* skip to the next comma or end of string */
		while (*cp != '\0' && *cp != ',') {
			cnt++;
			cp++;
		}

		if (cnt == 0)
			break;

		/* if 0.0 was specified disable sync i/o on this target */
		if ((period = ncr_max_sync_lookup(savecp, cnt)) == -1) {
			ncrp->n_syncstate[target] = NSYNC_SDTR_REJECT;
		} else {
			ncrp->n_syncstate[target] = NSYNC_SDTR_NOTDONE;
		}

		ncrp->n_minperiod[target] = period;
		if (*cp == ',')
			cp++;
	}

	/* if the property is invalid then disable sync i/o */
	return (*cp == '\0');
}


/*
 * convert the sync I/O rate property string into
 * an integer via table lookup
 */

static int
ncr_max_sync_lookup(	caddr_t	cp,
			int	cnt )
{
	srt_t	*srtp;

	for (srtp = syncio_rate_table; srtp->s_rate != NULL; srtp++) {
		if (cnt != strlen(srtp->s_rate))
			continue;
		if (strncmp(cp, srtp->s_rate, cnt) != 0)
			continue;
		return (srtp->s_period);
	}
	return (-1);
}


/* 
 * Find the clock divisor which gives a period that at least
 * as long as syncioperiod. If an divisor can't be found that
 * gives the exactly desired syncioperiod then the divisor which
 * results in the next longer valid period will be returned.
 *
 * In the above divisor lookup table the periods and divisors
 * are scaled by a factor of ten to handle the .5 fractional values.
 * I could have just scaled everything by a factor of two but I
 * think x10 is easier to understand and easier to setup.
 *
 * Note: the 53c710 can't divide SCLK by three. It's only divisors
 * are 1, 1.5, and 2.
 */
bool_t
ncr_max_sync_divisor(	ncr_t	*ncrp,
			int	 syncioperiod,
			unchar	*sxferp,
			unchar	*sscfX10p )
{
	ndt_t	*dtp;
	int	 divX10;

	divX10 = 10 * syncioperiod;
	divX10 /= ncrp->n_speriod;

	for (dtp = DivisorTable; dtp->divisorX10 != 0; dtp++) {
		if (dtp->sscfX10 == 30 && ncrp->n_is710 == TRUE)
			continue;
		if (dtp->divisorX10 >= divX10)
			goto got_it;
	}
	return (FALSE);

   got_it:
	*sxferp = dtp->sxfer;
	*sscfX10p = dtp->sscfX10;
	return (TRUE);
}


int
ncr_period_round(	ncr_t	*ncrp,
			int	 syncioperiod )
{
	int	clkperiod;
	unchar	sxfer;
	unchar	sscfX10;
	int	tmp;


	if (ncr_max_sync_divisor(ncrp, syncioperiod, &sxfer, &sscfX10)) {
		clkperiod = ncrp->n_speriod;
		switch (sscfX10) {
		case 10:
			/* times 1 */
			return (ncrp->n_speriod * sxfer);

		case 15:
			/* times 1.5 */
			tmp = 15 * ncrp->n_speriod * sxfer;
			return ((tmp + 5) / 10);

		case 20:
			/* times 2 */
			return (2 * ncrp->n_speriod * sxfer);
		}
	}
	return (-1);
}

/*
 * Determine frequency of the HBA's clock chip and determine what
 * rate to use for synchronous i/o on each target. Because of the
 * way the chip's divisor chain works it's only possible to achieve
 * timings that are integral multiples of the clocks fundamental 
 * period. 
 */

void
ncr_max_sync_rate_init(	ncr_t	*ncrp,
			bool_t	 is710 )
{
	dev_info_t	*dip = ncrp->n_dip;
	caddr_t		 maxsync;
	int		 val;
	int		 len;
	int		 target;
	int		 period;

	ncrp->n_is710 = is710;

	/* default to 40 MHz SCLK */
	ncrp->n_sclock   = 40;

	/* get clock frequency (in MHz) */
	if (HBA_INTPROP(dip, "clock", &val, &len) == DDI_PROP_SUCCESS) {
		if (val < 16 || val > 75) {
			goto bad_clock;
		}
		ncrp->n_sclock   = val;
	}

	/* calculate the fundamental period in nanoseconds */
	ncrp->n_speriod = 1000 / ncrp->n_sclock;

	/* Assume we want the default Fast-SCSI-2 sync i/o rate of */
	/* 100 nsec.. But round it to the closest value the hba's */
	/* divisor chain can produce */
	if ((period = ncr_period_round(ncrp, 100)) <= 0)
		goto bad_clock;

	/* set each target to 10MB/sec synchronous i/o */
	for (target = 0; target < NTARGETS; target++)
		ncrp->n_minperiod[target] = period;

	/* check the ncrs.conf file for the max-sync-rate property in case */
	/* the user wants to override the rate for a particular target */
	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS
				, "max-sync-rate", (caddr_t)&maxsync, &len)
	!= DDI_PROP_SUCCESS) {
		/* no overrides, all done */
		return;
	}

	/* convert the property string into the equivalent rate */
	if (ncr_max_sync_rate_parse(ncrp, maxsync)) {
		kmem_free(maxsync, len);
		return;
	}
	kmem_free(maxsync, len);
	goto bad_rate;


    bad_clock:
	cmn_err(CE_WARN
		, "ncr_max_sync_rate_init: property value invalid: clock=%d\n"
		, val);

    bad_rate:
	ncr_syncio_disable(ncrp);
	return;
}
