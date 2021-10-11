/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)serial_impl.h	1.5	95/06/19 SMI"

#ifndef _SERIAL_IMPL_H
#define _SERIAL_IMPL_H

#include <stdio.h>
#include "serial_iface.h"


#define FALSE	0
#define TRUE	1

#define SMALLBUF 128
#define MEDBUF   256
#define LGBUF	1024

#define PMDISABLED 10		/* port mon is there but disabled */
#define PMENABLED  11		/* port mon is there and enabled */
#define PMSTARTING 12		/* port mon is there and starting */
#define PMFAILED   13		/* port mon is there and failed */

#define PMTAG "zsmon"
#define ADD_VERS "`/usr/sbin/ttyadm -V`"
#define ADD_COMMENT "\"Serial Ports\""
#define DEVTERM	"/dev/term"	/* /dev/term/\* is the only place to look */

#define	SACADM_PATH	"/usr/sbin/sacadm"
#define	PMADM_PATH	"/usr/sbin/pmadm"
#define	TTYADM_PATH	"/usr/sbin/ttyadm"


extern int      do_eeprom(const char *port, const char *truefalse);
extern int	is_console(const char *port);
extern FILE	*pipe_execv(const char *path, char *const argv[], FILE **errf);
extern int	close_pipe_execl(FILE *fptr);
extern int	find_label(FILE *fp, const char *ttylabel);
extern int	check_label(const char *ttylabel);

#endif /*_SERIAL_IMPL_H */
