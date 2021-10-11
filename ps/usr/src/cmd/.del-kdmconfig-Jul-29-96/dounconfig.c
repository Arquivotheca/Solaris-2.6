/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 *
 */

#ident "@(#)dounconfig.c 1.6 94/03/15 SMI"

#include <sys/param.h>
#include <sys/stat.h>
#include "kdmconfig.h"

#define	OWCONFIG "/etc/openwin/server/etc/OWconfig"
#define	DEFAULTKB "/etc/defaultkb"

static void
_preserve_remove( char * filename )
{
static char buf[MAXPATHLEN];
struct stat sbuf;

	if (!stat(filename, &sbuf)) {
		sprintf(buf, "%s.save", filename);
		unlink(buf);
		rename(filename, buf);
	}
	return;
}


void
do_unconfig(void)
{

	/*
	 * Possibly there could be a warning added to this logic, so
	 * someone does not inadvertantly remove a client entry.
	 */
	if (get_server_mode()) {
		bootparam_remove(get_client());
	}
	else {
		/*
	 	 * If the OWconfig file is there, remove it, saving it first
	 	 */

		_preserve_remove(OWCONFIG);
		/*
	 	 * If the defaultkb file is there, remove it, saving it first
	 	 */
		_preserve_remove(DEFAULTKB);
	}
	
}
