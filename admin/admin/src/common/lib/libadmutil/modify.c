#ident "@(#)modify.c 1.1 95/01/09 SMI"

/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <sys/param.h>

/*
 * This routine modifies the specified client's timezone file.
 * Returns:
 *	0 = ok
 *	1 = failure.
 */

int
modify_timezone(char *clientname, char *clientroot, char *timezone) {
    char	 filename[MAXPATHLEN];
    char	 cmd[MAXPATHLEN];
    FILE	*file;

    sprintf(filename, "%s/etc/default/init", clientroot);
    sprintf(cmd, "/bin/grep -v '^TZ' %s > /tmp/.timezone.%d ; "
		"/bin/echo TZ=%s  >> /tmp/.timezone.%d ; "
		"/bin/mv /tmp/.timezone.%d %s",
	filename, getpid(),
	timezone, getpid(),
	getpid(), filename);

    if ( (file = popen(cmd, "r" )) == NULL ) {
	return(1);
    }
    pclose(file);
    return(0);
}
