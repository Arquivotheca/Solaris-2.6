#ifndef lint
static  char sccsid[] = "@(#)admpipe.c 1.6 94/11/02 SMI";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <admin/adm_fw.h>
#include <libintl.h>
#include <fcntl.h>
#include <sys/systeminfo.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stropts.h>
#include "admutil.h"
#include "admpipe.h"

static void setup_pipe(struct hostent *ph, char *port, int which_fd);

#define	METHOD_NAME	SYS_ADM_PIPE_MTHD

main()
{
	Adm_arg		*argp;
	char		*host, *snd_port, *rcv_port, *cmd = NULL;
	int		fd;
	struct hostent	*ph;

	adm_init();	/* give framework a hook */

	/*
	 * Get the addrs & cmd from the input arguments
	 */
	if (adm_args_geti(SYS_AP_SADDR_PAR, ADM_STRING, &argp) != ADM_SUCCESS) {
		adm_err_setf(ADM_NOCATS, 0,
			gettext("Error: Missing parameter %s"),
			SYS_AP_SADDR_PAR);
		adm_err_cleanup(ADM_NOCATS, ADM_FAILCLEAN);
		exit(ADM_FAILURE);
	}

	rcv_port = argp->valuep;

	if (adm_args_geti(SYS_AP_RADDR_PAR, ADM_STRING, &argp) != ADM_SUCCESS) {
		adm_err_setf(ADM_NOCATS, 0,
			gettext("Error: Missing parameter %s"),
			SYS_AP_RADDR_PAR);
		adm_err_cleanup(ADM_NOCATS, ADM_FAILCLEAN);
		exit(ADM_FAILURE);
	}

	snd_port = argp->valuep;

	if (adm_args_geti(SYS_AP_CMD_PAR, ADM_STRING, &argp) == ADM_SUCCESS) {
		cmd = argp->valuep;
	}


	if ((host = getenv("ADM_CLIENT_HOST")) == NULL) {
		adm_err_setf(ADM_NOCATS, 0,
		    gettext("Error: missing host env variable"));
		adm_err_cleanup(ADM_NOCATS, ADM_FAILCLEAN);
		exit(ADM_FAILURE);
	}

	if ((ph = gethostbyname(host)) == NULL) {
		adm_err_setf(ADM_NOCATS, 0,
		    gettext("Error: unknown host %s"), host);
		adm_err_cleanup(ADM_NOCATS, ADM_FAILCLEAN);
		exit(ADM_FAILURE);
	}

	switch (fork()) {
	case -1:
		adm_err_setf(ADM_NOCATS, 0, gettext("Error: fork failed"));
		adm_err_cleanup(ADM_NOCATS, ADM_FAILCLEAN);
		exit(ADM_FAILURE);
		break;

	default: /* parent */
		exit(ADM_SUCCESS);
		break;

	case 0: /* child */
		break;
	}

	/* now running in the child process */

	/*
	 * we need to clean up fd's in order to get admind to know that
	 * the child is done and it can send status back to the caller.
	 */
	for (fd = 0; fd < 64; fd++)
		close(fd);

	setup_pipe(ph, rcv_port, 0);
	setup_pipe(ph, snd_port, 1);

	if (cmd)
		execl("/bin/sh", "/bin/sh", "-c", cmd, (char *) 0);
	else
		execl("/bin/sh", "/bin/sh", "-i", (char *) 0);

	exit(1);
}

static void
setup_pipe(struct hostent *ph, char *port, int which_fd)
{
	int fd;
	struct sockaddr_in server;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		exit(1);

	server.sin_family = AF_INET;

	(void) memcpy((char *) &server.sin_addr, (char *) ph->h_addr,
		ph->h_length);
	server.sin_port = htons(atoi(port));

	if (connect(fd, (struct sockaddr *) &server, sizeof (server)) == -1)
		exit(1);

	if (which_fd == 0) {
		if (fd != 0) {
			close(0);
			dup(fd);
			close(fd);
		}
	} else {
		if (fd != 1) {
			close(1);
			dup(fd);
		}
		if (fd != 2) {
			close(2);
			dup(fd);
		}
		if (fd != 1 && fd != 2)
			close(fd);
	}
}
