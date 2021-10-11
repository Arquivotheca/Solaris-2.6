/*
 * Copyright (c) 1994-1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)prclose.c	1.3	96/06/18 SMI"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "pcontrol.h"

int	/* close() system call -- executed by subject process */
prclose(process_t *Pr, int fd)
{
	struct sysret rval;		/* return value from close() */
	struct argdes argd[1];		/* arg descriptors for close() */
	register struct argdes *adp;

	adp = &argd[0];		/* fd argument */
	adp->value = (int)fd;
	adp->object = (char *)NULL;
	adp->type = AT_BYVAL;
	adp->inout = AI_INPUT;
	adp->len = 0;

	rval = Psyscall(Pr, SYS_close, 1, &argd[0]);

	if (rval.errno < 0)
		rval.errno = ENOSYS;

	if (rval.errno == 0)
		return (rval.r0);
	errno = rval.errno;
	return (-1);
}
