/*
 * Copyright (c) 1992-1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)unix_open_session.c	1.2	96/04/09 SMI"	/* PAM 2.6 */

/*
 * pam_sm_open_session 	- session management for individual users
 */

#include "unix_headers.h"


int
pam_sm_open_session(
	pam_handle_t	*pamh,
	int	flags,
	int	argc,
	const char	**argv)
{
	int	error;
	char	*ttyn, *rhost, *user;
	int	fdl;
	struct lastlog	newll;
	struct passwd pwd;
	char	buffer[2048];
	int	i;
	int	debug = 0;
	long long	offset;

	for (i = 0; i < argc; i++) {
		if (strcasecmp(argv[i], "debug") == 0)
			debug = 1;
		else
			syslog(LOG_ERR, "illegal option %s", argv[i]);
	}

	if ((error = pam_get_item(pamh, PAM_TTY, (void **)&ttyn))
							!= PAM_SUCCESS ||
	    (error = pam_get_item(pamh, PAM_USER, (void **)&user))
							!= PAM_SUCCESS ||
	    (error = pam_get_item(pamh, PAM_RHOST, (void **)&rhost))
							!= PAM_SUCCESS) {
		return (error);
	}

	if (getpwnam_r(user, &pwd, buffer, sizeof (buffer)) == NULL) {
		return (PAM_USER_UNKNOWN);
	}

	if ((fdl = open(LASTLOG, O_RDWR|O_CREAT, 0444)) >= 0) {

		/*
		 * The value of lastlog is read by the UNIX
		 * account management module
		 */

		offset = (long long) pwd.pw_uid *
					(long long) sizeof (struct lastlog);

		if (llseek(fdl, offset, SEEK_SET) != offset) {
			/*
			 * XXX uid too large for database
			 */
			return (PAM_SUCCESS);
		}

		(void) time(&newll.ll_time);
		strncpy(newll.ll_line,
			(ttyn + sizeof ("/dev/")-1),
			sizeof (newll.ll_line));
		strncpy(newll.ll_host, rhost, sizeof (newll.ll_host));

		(void) write(fdl, (char *)&newll, sizeof (newll));
		(void) close(fdl);
	}

	return (PAM_SUCCESS);
}
