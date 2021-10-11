
/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)ws_is_cons.c	1.10	95/05/11 SMI"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/openpromio.h>

extern FILE *debugfp;

/*
 * Uses openprom ioctl
 */

int
ws_is_cons(char *termtype)
{

	int fd;
	struct openpromio op;
	int status = 0;

	if ((fd = open("/dev/openprom", O_RDONLY)) >= 0) {
		(void) memset(&op, 0, sizeof (op));
		op.oprom_size = sizeof (char);
		if (ioctl(fd, OPROMGETCONS, &op) >= 0)
			status = (op.oprom_array[0] & OPROMCONS_STDIN_IS_KBD) &&
				(op.oprom_array[0] & OPROMCONS_STDOUT_IS_FB);

		(void) close(fd);
	}

	if (status)
#if defined(__i386)
		strcpy(termtype, "AT386");
#elif defined(__ppc)
		strcpy(termtype, "sun");
#elif defined(__sparc)
		strcpy(termtype, "sun");
#else
#error ISA not supported
#endif

	fprintf(debugfp, "cons: %d %s\n", status, termtype);
	return (status);
}
