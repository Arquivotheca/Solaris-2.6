#ident	"@(#)rt.c	1.3	93/05/28 SMI"		/* SVr4.0 1.1.2.1 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *		Copyright (C) 1991  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 *		RESTRICTED RIGHTS LEGEND:
 *   Use, duplication, or disclosure by the Government is subject
 *   to restrictions as set forth in subparagraph (c)(1)(ii) of
 *   the Rights in Technical Data and Computer Software clause at
 *   DFARS 52.227-7013 and in similar clauses in the FAR and NASA
 *   FAR Supplement.
 */

/*
 * This file contains code for the crash functions:  rtproc, rtdptbl
 */

#include <stdio.h>
#include <stdlib.h>
#include <nlist.h>
#include <sys/types.h>
#include <sys/rt.h>
#include "crash.h"

static void prrtproc();
static void prrtdptbl();

int
getrtproc()
{
	int c;
	struct nlist Rtproc;	/* namelist symbol */
	struct rtproc rtbuf, *rtp;

	optind = 1;
	while ((c = getopt(argcnt, args, "w:")) != EOF) {
		switch (c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}

	if (nl_getsym("rt_plisthead", &Rtproc))
		error("rt_plisthead not found in symbol table\n");

	readmem((long)Rtproc.n_value, 1, -1, (char *)&rtbuf,
		sizeof (rtbuf), "rtproc table");

	fprintf(fp, "PQUANT  TMLFT  PRI  FLAGS   THREADP   PSTATP    ");
	fprintf(fp, "PPRIP   PFLAGP    NEXT     PREV\n");
	rtp = rtbuf.rt_next;

	for (; rtp != (rtproc_t *)Rtproc.n_value; rtp = rtbuf.rt_next) {
		readmem((long)rtp, 1, -1, (char *)&rtbuf,
			sizeof (rtbuf), "rtproc table");
		prrtproc(&rtbuf);
	}
	return (0);
}




/* print the real time process table */
static void
prrtproc(rtp)
struct rtproc *rtp;
{
	fprintf(fp, "%4d    %4d %4d %5x %10x %8x %8x %8x %8x %8x\n",
		rtp->rt_pquantum, rtp->rt_timeleft, rtp->rt_pri,
		rtp->rt_flags, rtp->rt_tp, rtp->rt_pstatp, rtp->rt_pprip,
		rtp->rt_pflagp, rtp->rt_next, rtp->rt_prev);
}


/* get arguments for rtdptbl function */

int
getrtdptbl()
{
	int slot = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	long rt_addr;
	int rtmaxpri;
	struct nlist Rtdptbl, Rtmaxpri;
	rtdpent_t *rtdptbl;

	char *rtdptblhdg = "SLOT     GLOBAL PRIORITY     TIME QUANTUM\n\n";

	if (nl_getsym("rt_dptbl", &Rtdptbl))
		error("rt_dptbl not found in symbol table\n");

	if (nl_getsym("rt_maxpri", &Rtmaxpri))
		error("rt_maxpri not found in symbol table\n");

	optind = 1;
	while ((c = getopt(argcnt, args, "w:")) != EOF) {
		switch (c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}

	readmem(Rtmaxpri.n_value, 1, -1, (char *)&rtmaxpri, sizeof (int),
		"rt_maxpri");

	rtdptbl = (rtdpent_t *)malloc((rtmaxpri + 1) * sizeof (rtdpent_t));

	readmem(Rtdptbl.n_value, 1, -1, (char *)&rt_addr, sizeof (rt_addr),
		"address of rt_dptbl");
	readmem(rt_addr, 1, -1, (char *)rtdptbl,
		(rtmaxpri + 1) * sizeof (rtdpent_t), "rt_dptbl");

	fprintf(fp, "%s", rtdptblhdg);

	if (args[optind]) {
		do {
			getargs(rtmaxpri + 1, &arg1, &arg2, 0);
			if (arg1 == -1)
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					prrtdptbl(slot, rtdptbl);
			else {
				if (arg1 < rtmaxpri + 1)
					slot = arg1;
				prrtdptbl(slot, rtdptbl);
			}
			slot = arg1 = arg2 = -1;
		} while (args[++optind]);
	} else
		for (slot = 0; slot < rtmaxpri + 1; slot++)
			prrtdptbl(slot, rtdptbl);

	free(rtdptbl);
	return (0);
}

/* print the real time dispatcher parameter table */
static void
prrtdptbl(slot, rtdptbl)
int  slot;
rtdpent_t *rtdptbl;
{
	fprintf(fp, "%3d           %4d           %10ld\n",
	    slot, rtdptbl[slot].rt_globpri, rtdptbl[slot].rt_quantum);
}
