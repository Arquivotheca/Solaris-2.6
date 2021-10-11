#ident	"@(#)status.c	1.5	94/06/09 SMI"

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
 * This file contains code for the crash functions: status.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/elf.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include "crash.h"

Elf32_Sym	*Sys, *Time;
extern uint_t hz;
extern Elf32_Sym *Panic;
Elf32_Sym	*Lbolt;

#define	DATE_FMT	"%a %b %e %H:%M:%S %Y\n"

/*
 * %a	abbreviated weekday name
 * %b	abbreviated month name
 * %e	day of month
 * %H	hour
 * %M	minute
 * %S	second
 * %Y	year
 */

static char	time_buf[50];	/* holds data and time string */

int
getstat()
{
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "w:")) != EOF) {
		switch (c) {
			case 'w':
				redirect();
				break;
			default:
				longjmp(syn, 0);
		}
	}
	if (args[optind])
		longjmp(syn, 0);
	else
		prstat();
}

/* print system statistics */
int
prstat()
{
#if i386
	int toc, lbolt;
	extern char panic_str[];
	extern int panic_eip, panic_esp;
#else /* !i386 */
	int toc, lbolt, panicp;
	char panic_char;
#endif /* !i386 */
	struct utsname utsbuf;
	label_t regs;

	/*
	 * Locate, read, and print the system name, node name, release
	 * number, version number, and machine name.
	 */

	if (!Sys)
		if (!(Sys = symsrch("utsname")))
			error("utsname not found in symbol table\n");
	readmem((long) Sys->st_value, 1, -1, (char *) &utsbuf,
		sizeof (utsbuf), "utsname structure");

	fprintf(fp, "system name:\t%s\nrelease:\t%s\n",
		utsbuf.sysname,
		utsbuf.release);
	fprintf(fp, "node name:\t%s\nversion:\t%s\n",
		utsbuf.nodename,
		utsbuf.version);
	fprintf(fp, "machine name:\t%s\n", utsbuf.machine);

	/*
	 * Locate, read, and print the time of the crash.
	 */


	if (!Time)
		if (!(Time = symsrch("time")))
			error("time not found in symbol table\n");

	readmem((long) Time->st_value, 1, -1, (char *) &toc,
		sizeof (toc), "time of crash");
	cftime(time_buf, DATE_FMT, (long *) &toc);
	fprintf(fp, "time of crash:\t%s", time_buf);

	/*
	 * Locate, read, and print the age of the system since the last boot.
	 */

	if (!Lbolt)
		if (!(Lbolt = symsrch("lbolt")))
			if (!(Lbolt = symsrch("lbolt")))
				error("lbolt not found in symbol table\n");

	readmem((long) Lbolt->st_value, 1, -1, (char *) &lbolt,
		sizeof (lbolt), "lbolt");

	fprintf(fp, "age of system:\t");
	lbolt = lbolt / (60 * hz);
	if (lbolt / (long) (60 * 24))
		fprintf(fp, "%d day, ", lbolt / (long) (60 * 24));
	lbolt %= (long) (60 * 24);
	if (lbolt / (long) 60)
		fprintf(fp, "%d hr., ", lbolt / (long) 60);
	lbolt %= (long) 60;
	if (lbolt)
		fprintf(fp, "%d min.", lbolt);
	fprintf(fp, "\n");
#if i386
	fprintf(fp, "panicstr:  %s\npanic registers:\n\teip: %x      esp: %x\n",
	    panic_str, panic_eip, panic_esp);
#else /* !i386 */

	/*
	 * Determine if a panic occured by examining the size of the panic
	 * string. If no panic occurred return to main(). If a panic did
	 * occur locate, read, and print the panic registers. Note: in
	 * examining an on-line system, the panic registers will always
	 * appear to be zero.
	 */

	fprintf(fp, "panicstr:\t");
	if (kvm_read(kd, Panic->st_value, (char *)&panicp, sizeof (char *)) ==
	    sizeof (char *)) {
		while (kvm_read(kd, panicp++, &panic_char, sizeof (panic_char))
			== sizeof (panic_char) && panic_char != '\0')
			(void) fputc(panic_char, fp);
	}
	fprintf(fp, "\n");
	readsym("panic_regs", &regs, sizeof (regs));
	fprintf(fp, "panic registers:\n");
	fprintf(fp, "\tpc: %x      sp: %x\n", regs.val[0], regs.val[1]);
#endif /* !i386 */
}
