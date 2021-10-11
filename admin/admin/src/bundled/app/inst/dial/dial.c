#ifndef lint
#ident   "@(#)dial.c 1.3 95/01/04 SMI"
#endif				/* lint */
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

void end();

main(int argc, char **argv)
{
	int state = 0;

	if (argc > 1) {
		/*
		 * used to send a signal to earlier instance of dial when
		 * we want to do error recovery and kill isn't avialable yet.
		 * Arg is a pid to send sigterm to.
		 */

		(void) kill(atoi(argv[1]), SIGTERM);
		exit(0);
	}

	signal(SIGTERM, end);

	for (;;) {
		switch(state) {
		case 0:
			fputs("|\b", stdout);
			state = 1;
			break;
		case 1:
			fputs("/\b", stdout);
			state = 2;
			break;
		case 2:
			fputs("-\b", stdout);
			state = 3;
			break;
		case 3:
			fputs("\\\b", stdout);
			state = 0;
			break;
		}
		sleep(1);
	}
}

void end()
{
	exit(0);
}
