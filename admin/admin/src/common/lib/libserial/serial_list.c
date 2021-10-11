/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)serial_list.c	1.4	95/03/07 SMI"


#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include "serial_impl.h"


int
list_modem(ModemInfo **mi_pp, const char *alt_dev_dir)
{

	int		i;
	int		j;
	int		retval;
	int		size;
	int		found;
	int		mi_idx;
	FILE		*pp = NULL;
	char		name[PATH_MAX + 1];
	char		dirname[PATH_MAX + 1];
	DIR		*dirp;
	struct dirent	*direntp;
	struct stat	stat_buf;
	const char	*head;
	const char	*tail;
	int		serviceless_ports;
	int		term_cnt;
	char		**term = NULL;
	char		*port_markers = NULL;
	int		svc_cnt;
	char		**svc = NULL;
	char		**svc_to_port = NULL;
	char		buf[PMADM_INFO_BUF];
	const int	chunk = 10;
	char		*devfield = NULL;
	char		*colon = NULL;
	char		port[PATH_MAX + 1];


	term_cnt = 0;

	/* Count files in /dev/term */

	if ((dirp = opendir(DEVTERM)) != NULL) {

		while ((direntp = readdir(dirp)) != NULL) {
			if (strcmp(direntp->d_name, "..") == 0 ||
			    strcmp(direntp->d_name, ".") == 0) {
				continue;
			}
			term_cnt++;
		}

		(void) closedir(dirp);
	}

	/* Count files in alternate device directories */

	for (head = alt_dev_dir; head != NULL; head = tail + 1) {

		if ((tail = strchr(head, ':')) != NULL) {
			strncpy(dirname, head, tail - head);
			dirname[tail - head] = 0;
		} else {
			strcpy(dirname, head);
		}

		if ((dirp = opendir(dirname)) == NULL) {
			continue;
		}

		while ((direntp = readdir(dirp)) != NULL) {
			if (strcmp(direntp->d_name, "..") == 0 ||
			    strcmp(direntp->d_name, ".") == 0) {
				continue;
			}
			term_cnt++;
		}

		(void) closedir(dirp);

		if (tail == NULL) {
			break;
		}
	}

	if (term_cnt == 0) {
		return (0);
	}

	/* allocate return space for info about each of the files */

	if ((term = 
	    (char **)malloc((unsigned)(sizeof (char *) * term_cnt))) == NULL) {
		return (M_MALLOC_FAIL);
	}

	i = 0;

	if ((dirp = opendir(DEVTERM)) != NULL) {

		while ((direntp = readdir(dirp)) != NULL) {
			if (strcmp(direntp->d_name, "..") == 0 ||
			    strcmp(direntp->d_name, ".") == 0) {
				continue;
			}
			sprintf(name, "%s/%s", DEVTERM, direntp->d_name);
			term[i++] = strdup(name);
		}

		(void) closedir(dirp);
	}

	for (head = alt_dev_dir; head != NULL; head = tail + 1) {

		if ((tail = strchr(head, ':')) != NULL) {
			strncpy(dirname, head, tail - head);
			dirname[tail - head] = 0;
		} else {
			strcpy(dirname, head);
		}

		if ((dirp = opendir(dirname)) == NULL) {
			continue;
		}

		while ((direntp = readdir(dirp)) != NULL) {
			if (strcmp(direntp->d_name, "..") == 0 ||
			    strcmp(direntp->d_name, ".") == 0) {
				continue;
			}
			sprintf(name, "%s/%s", dirname, direntp->d_name);
			term[i++] = strdup(name);
		}

		(void) closedir(dirp);

		if (tail == NULL) {
			break;
		}
	}

	/* for security */
	putenv("IFS= \t");

	/*
	 * Read all services information known to pmadm.
	 * This command will write "No services defined" to stdout
	 * if there are no services, so redirect to /dev/null.
	 */

	if ((pp =
	    popen("/usr/sbin/pmadm -L -t ttymon 2>/dev/null", "r")) == NULL) {
		retval = SERIAL_ERR_PMADM;
		goto unwind;
	}

	svc_cnt = 0;
	svc = NULL;

	while (fgets(buf, sizeof (buf), pp) != NULL) {

		/* device name is 8th field in pmadm -L output */

		devfield = buf;
		for (i = 0; i < 8; i++) {
			devfield = strchr(devfield, ':') + 1;
		}

		colon = strchr(devfield, ':');

		strncpy(port, devfield, colon - devfield);
		port[colon - devfield] = '\0';

		if (stat(port, &stat_buf) == -1) {

			/*
			 * Probably a bogus entry in pmtab, rather
			 * than an actual serial port device; don't
			 * process it.
			 */

			continue;
		}

		/* It's a service on a real device, so we're interested */

		if (svc_cnt % chunk == 0) {
			size = (svc_cnt / chunk + 1) * chunk;
			svc = (char **)realloc(svc, size * sizeof (char *));
			if (svc == NULL) {
				pclose(pp);
				retval = SERIAL_FAILURE;
				goto unwind;
			}
		}

		if ((svc[svc_cnt] = strdup(buf)) == NULL) {
			pclose(pp);
			retval = SERIAL_FAILURE;
			goto unwind;
		}
		svc_cnt++;
	}

	pclose(pp);

	/*
	 * Now we're finally ready to build up the return list.
	 * In order to work just like the pre-source-split
	 * method worked, we need to return an entry for each
	 * service; in addition, if there are any pmadm -L
	 * ports that don't have a service, we need to
	 * return an entry for the port that indicates "no
	 * service".  And we also need to return things in
	 * the same order as the old method, which was the
	 * order in which the opendir/readdir found them
	 * (which is somewhat bogus, as it isn't necessarily
	 * the same from system to system; I'd prefer to sort
	 * them, but I'm not going to change the way it works).
	 */

	/*
	 * allocate an array for pointers from a service to the
	 * corresponding port
	 */

	svc_to_port = (char **)malloc(svc_cnt * sizeof (char *));
	if (svc_to_port == NULL) {
		retval = SERIAL_FAILURE;
		goto unwind;
	}

	/* and another array to mark ports when a service is found on them */

	port_markers = (char *)malloc(term_cnt * sizeof (char));
	if (port_markers == NULL) {
		retval = SERIAL_FAILURE;
		goto unwind;
	}
	memset((void *)port_markers, 0, term_cnt * sizeof (char));

	serviceless_ports = term_cnt;

	for (i = 0; i < svc_cnt; i++) {

		for (j = 0; j < term_cnt; j++) {

			/*
			 * Make sure that when we look for something like
			 * /dev/term/a in the svc line, we don't find
			 * something like /dev/term/a00 instead by appending
			 * a ':' to the /dev/term/<name> since the svc line
			 * has the : separator characters in it.
			 */

			strcpy(port, term[j]);
			strcat(port, ":");

			if (strstr(svc[i], port) != NULL) {

				/* svc[i] is on term[j] */
				svc_to_port[i] = term[j];

				/*
				 * mark term[j] as having a service,
				 * and decrement the number of ports
				 * without a service is this is the
				 * first service found for this port
				 */

				if (++port_markers[j] == 1) {
					--serviceless_ports;
				}

				break;
			}
		}
	}

	/*
	 * Allocate space for the return info, the number of services
	 * plus the number of serviceless ports
	 */

	(*mi_pp) = (ModemInfo *)malloc((svc_cnt + serviceless_ports) *
	    sizeof (ModemInfo));

	if (*mi_pp == NULL) {
		retval = SERIAL_FAILURE;
		goto unwind;
	}

	retval = svc_cnt + serviceless_ports;

	mi_idx = 0;

	for (i = 0; i < term_cnt; i++) {

		found = 0;

		for (j = 0; j < svc_cnt; j++) {

			if (svc_to_port[j] == term[i]) {
				strcpy((*mi_pp)[mi_idx].port,
				    strrchr(term[i], '/') + 1);
				strcpy((*mi_pp)[mi_idx].pmadm_info, svc[j]);
				mi_idx++;
				found++;
			}
		}

		if (found == 0) {
			/*
			 * ran down the entire service list and didn't
			 * find a service on this port
			 */

			strcpy((*mi_pp)[mi_idx].port,
				    strrchr(term[i], '/') + 1);
			strcpy((*mi_pp)[mi_idx].pmadm_info, "");
			mi_idx++;
		}
	}

 unwind:

	if (term != NULL) {
		for (i = 0; i < term_cnt; i++) {
			free((void *)term[i]);
		}
		free((void *)term);
	}

	if (svc != NULL) {
		for (i = 0; i < svc_cnt; i++) {
			free((void *)svc[i]);
		}
		free((void *)svc);
	}

	if (svc_to_port != NULL) {
		free((void *)svc_to_port);
	}

	if (port_markers != NULL) {
		free((void *)port_markers);
	}

	return (retval);
}
