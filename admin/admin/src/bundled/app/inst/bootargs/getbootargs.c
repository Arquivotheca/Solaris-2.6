#ifndef lint
#pragma ident "@(#)getbootargs.c 1.9 96/07/26"
#endif
/*
 * Copyright (c) 1991-1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/openpromio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

/*
 * define the openprom boot argument ioctl constant for systems which don't
 * have it defined in their headers
 */
#ifndef	OPROMGETBOOTARGS
#define	OPROMGETBOOTARGS	(OIOC|12)
#endif

/* prototype definitions */

int		openprom(void);

/*
 * main()
 * Parameters:
 *	argc	- unused
 *	argv	- unused
 * Return:
 *	0	- success
 *	1	- failure
 */
/*ARGSUSED0*/
int
main(int argc, char **argv)
{
	if (openprom() == 1)
		exit(0);

	exit(1);
}

/*
 * openprom()
 *	Try to open the /dev/openprom device and retrieve the
 * 	OPROMGETBOOTARGS boot argument attribute.
 * Parameters:
 *	none
 * Return:
 *	0	- failed to retrieve bootargs from /dev/openprom
 *	1	- openprom opended and bootarg successfully
 *		  retrieved and printed
 */
int
openprom(void)
{
	int	fd;
	char	foo[256];
	struct openpromio *op = (struct openpromio *)foo;

	op->oprom_size = 128;

	if ((fd = open("/dev/openprom", O_RDONLY)) == -1)
		return (0);

	if (ioctl(fd, OPROMGETBOOTARGS, (caddr_t)op) == -1) {
		(void) close(fd);
		return (0);
	}

	(void) printf("%s\n", op->oprom_array);
	(void) close(fd);
	return (1);
}
