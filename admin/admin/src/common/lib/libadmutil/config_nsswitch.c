
/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)config_nsswitch.c	1.1	94/08/26 SMI"

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/param.h>
#include <errno.h>
#include <libintl.h>
#include "db_entry.h"
#include "admutil.h"

#define	NSSWITCH		"/etc/nsswitch.conf"

#define BUFSIZE			512

/*
 * Follow links to the switch file and set up a temporary file name.
 */

static int
trav_switch_files(char *switch_file, char *work)
{
	char *path;

	/* Find the real switch file */

	path = switch_file;
	if (trav_link(&path) == -1)
		return (ADMUTIL_SWITCH_LINK);

	strcpy(switch_file, path);

	/* Set up a temporary work file */

	strcpy(work, path);
	remove_component(work);
	if (strlen(work) == 0) {
		strcat(work, ".");
	}
	path = tempfile(work);
	if (path == NULL)
		return (ADMUTIL_SWITCH_WORK);

	strcpy(work, path);

	return (0);
}

/*
 * Open the template nsswitch.conf file and the temporary work
 * file, using the real nsswitch.conf file to set the permissions
 * on the temporary work file.
 */

static int
open_switch_files(char *switch_file, char *template, int *tfdp, char *work,
	int *wfdp)
{
	struct stat sb;

	/* Get the real nsswitch file permissions and owner */

	if (stat(switch_file, &sb) != 0)
		return (ADMUTIL_SWITCH_STAT);

	/* Open the template nsswitch.conf file for reading */

	*tfdp = open(template, O_RDONLY);
	if (*tfdp == -1)
		return (ADMUTIL_SWITCH_TOPEN);

	/* Create the temporary work file */

	*wfdp = open(work, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (*wfdp == -1) {
		close (*tfdp);
		return (ADMUTIL_SWITCH_OPEN);
	}

	/* Set permissions and ownership on the work file */

	if (fchmod(*wfdp, sb.st_mode) == -1) {
		close(*tfdp);
		close(*wfdp);
		unlink(work);
		return (ADMUTIL_SWITCH_WCHMOD);
	}
	if (fchown(*wfdp, sb.st_uid, sb.st_gid) == -1) {
		close(*tfdp);
		close(*wfdp);
		unlink(work);
		return (ADMUTIL_SWITCH_WCHOWN);
	}

	return (0);
}

/*
 * Copy the template nsswitch.conf file into the work file.
 */

static int
copy_file(int ifd, char *outfile, int ofd)
{
	int nbytes;
	char buf[BUFSIZE];
        boolean_t forever = B_TRUE;

	while (forever) {
		nbytes = read(ifd, buf, (size_t) BUFSIZE);
		if (nbytes == 0) {
			break;
		}
		if (nbytes == -1) {
			close(ifd);
			close(ofd);
			unlink(outfile);
			return (ADMUTIL_SWITCH_READ);
		}
		if (write(ofd, buf, (size_t) nbytes) == -1) {
			close(ifd);
			close(ofd);
			unlink(outfile);
			return (ADMUTIL_SWITCH_WRITE);
		}
	}

	return (0);
}

/*
 * Close the open files and move the work file over top of the
 * real nsswitch.conf file.
 */

static int
move_switch_file(char *switch_file, int tfd, char *work, int wfd)
{
	close(tfd);
	close(wfd);
	if (rename(work, switch_file) == -1) {
		unlink(work);
		return (ADMUTIL_SWITCH_REN);
	}

	return (0);
}

config_nsswitch(char *template)
{
	char switch_file[MAXPATHLEN+1];
	char work_file[MAXPATHLEN+1];
	int tfd, wfd, status;

	/*
	 * Set up the real files to work with.
	 */

	strcpy(switch_file, NSSWITCH);
	if ((status = trav_switch_files(switch_file, work_file)) < 0)
		return (status);
	if ((status = open_switch_files(switch_file, template, &tfd, work_file,
	    &wfd)) < 0)
		return (status);

	/*
	 * Create the new nsswitch.conf file.
	 */

	if ((status = copy_file(tfd, work_file, wfd)) < 0)
		return (status);
	if ((status = move_switch_file(switch_file, tfd, work_file, wfd)) < 0)
		return (status);
	sync();
	sync();

        return(0);
}
