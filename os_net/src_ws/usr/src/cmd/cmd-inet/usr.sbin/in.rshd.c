/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)in.rshd.c	1.26	96/05/24 SMI"	/* SVr4.0 1.8 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 * 		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 * 	(c) 1986,1987,1988.1989, 1996  Sun Microsystems, Inc.
 *	All rights reserved.
 *
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *	All rights reserved.
 *
 */


#define _FILE_OFFSET_BITS 64

/*
 * remote shell server:
 *	remuser\0
 *	locuser\0
 *	command\0
 *	data
 */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <netdb.h>
#include <syslog.h>

#ifdef SYSV
#include <sys/resource.h>
#include <sys/filio.h>
#include <shadow.h>
#include <stdlib.h>

#include <security/pam_appl.h>

#define	killpg(a, b)	kill(-(a), (b))
#define	rindex strrchr
#define	index strchr
#endif	/* SYSV */

#ifndef NCARGS
#define	NCARGS	5120
#endif /* NCARGS */

int	errno;
char	*index(), *rindex(), *strncat();
void	error(), doit(), getstr();

/* Function decls. for functions not in any header file.  (Grrrr.) */
extern void audit_rshd_setup(), audit_rshd_success(), audit_rshd_fail();

pam_handle_t *pamh;
int retval;

/*ARGSUSED*/
void
main(argc, argv)
	int argc;
	char **argv;
{
	struct linger linger;
	int on = 1, fromlen;
	struct sockaddr_in from;

	openlog("rsh", LOG_PID | LOG_ODELAY, LOG_DAEMON);
	audit_rshd_setup();	/* BSM */
	fromlen = sizeof (from);
	if (getpeername(0, (struct sockaddr *) &from, &fromlen) < 0) {
		fprintf(stderr, "%s: ", argv[0]);
		perror("getpeername");
		_exit(1);
	}
	if (setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char *)&on,
	    sizeof (on)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
	linger.l_onoff = 1;
	linger.l_linger = 60;			/* XXX */
	if (setsockopt(0, SOL_SOCKET, SO_LINGER, (char *)&linger,
	    sizeof (linger)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_LINGER): %m");
	doit(dup(0), &from);
	/* NOTREACHED */
}

char	username[20] = "USER=";
char	homedir[64] = "HOME=";
char	shell[64] = "SHELL=";

#ifdef SYSV
char	*envinit[] =
	    {homedir, shell, (char *) 0, username, (char *) 0, (char *) 0};
#define	ENVINIT_PATH	2	/* position of PATH in envinit[] */
#define	ENVINIT_TZ	4	/* position of TZ in envinit[] */

/*
 *	See PSARC opinion 1992/025
 */
char	userpath[] = "PATH=/usr/bin:";
char	rootpath[] = "PATH=/usr/sbin:/usr/bin";
#else
char	*envinit[] =
	    {homedir, shell, "PATH=:/usr/ucb:/bin:/usr/bin", username, 0};
#endif /* SYSV */

static char cmdbuf[NCARGS+1];
char hostname [MAXHOSTNAMELEN + 1];

void
doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	char *cp;
	char locuser[16], remuser[16];

	struct passwd *pwd;
#ifdef SYSV
	char *tz, *tzenv;
	struct spwd *shpwd;
	struct stat statb;
#endif /* SYSV */

	int s;
	struct hostent *hp;
	short port;
	pid_t pid;
	int pv[2], cc;
	char buf[BUFSIZ], sig;
	int one = 1;
	int v = 0;
	int err = 0;

	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);
#ifdef SYSV
	(void) sigset(SIGCHLD, SIG_IGN);
#endif /* SYSV */
#ifdef DEBUG
	{ int t = open("/dev/tty", 2);
	    if (t >= 0) {
#ifdef SYSV
		setsid();
#else
		ioctl(t, TIOCNOTTY, (char *)0);
#endif SYSV
		(void) close(t);
	    }
	}
#endif
	fromp->sin_port = ntohs((u_short)fromp->sin_port);
	if (fromp->sin_family != AF_INET) {
		syslog(LOG_ERR, "malformed from address\n");
		exit(1);
	}
	if (fromp->sin_port >= IPPORT_RESERVED ||
	    fromp->sin_port < (u_int) (IPPORT_RESERVED/2)) {
		syslog(LOG_NOTICE, "connection from bad port\n");
		exit(1);
	}
	(void) alarm(60);
	port = 0;
	for (;;) {
		char c;
		if ((cc = read(f, &c, 1)) != 1) {
			if (cc < 0)
				syslog(LOG_NOTICE, "read: %m");
			shutdown(f, 1+1);
			exit(1);
		}
		if (c == 0)
			break;
		port = port * 10 + c - '0';
	}
	(void) alarm(0);
	if (port != 0) {
		int lport = IPPORT_RESERVED - 1;
		s = rresvport(&lport);
		if (s < 0) {
			syslog(LOG_ERR, "can't get stderr port: %m");
			exit(1);
		}
		if (port >= IPPORT_RESERVED) {
			syslog(LOG_ERR, "2nd port not reserved\n");
			exit(1);
		}
		fromp->sin_port = htons((u_short)port);
		if (connect(s, (struct sockaddr *) fromp,
			    sizeof (*fromp)) < 0) {
			syslog(LOG_INFO, "connect second port: %m");
			exit(1);
		}
	}
	dup2(f, 0);
	dup2(f, 1);
	dup2(f, 2);
	hp = gethostbyaddr((char *)&fromp->sin_addr, sizeof (struct in_addr),
		fromp->sin_family);
	if (hp)
		strncpy(hostname, hp->h_name, sizeof (hostname));
	else
		strncpy(hostname, inet_ntoa(fromp->sin_addr),
			    sizeof (hostname));
	getstr(remuser, sizeof (remuser), "remuser");
	getstr(locuser, sizeof (locuser), "locuser");
	getstr(cmdbuf, sizeof (cmdbuf), "command");

	/*
	 * Note that there is no rsh conv functions at present.
	 */
	if ((err = pam_start("rsh", locuser, NULL, &pamh)) != PAM_SUCCESS) {
		syslog(LOG_ERR, "pam_start() failed: %s\n",
			pam_strerror(0, err));
		exit(1);
	}
	if ((err = pam_set_item(pamh, PAM_RHOST, hostname)) != PAM_SUCCESS) {
		syslog(LOG_ERR, "pam_set_item() failed: %s\n",
			pam_strerror(pamh, err));
		exit(1);
	}
	if ((err = pam_set_item(pamh, PAM_RUSER, remuser)) != PAM_SUCCESS) {
		syslog(LOG_ERR, "pam_set_item() failed: %s\n",
			pam_strerror(pamh, err));
		exit(1);
	}

	pwd = getpwnam(locuser);
	shpwd = getspnam(locuser);
	if ((pwd == NULL) || (shpwd == NULL)) {
		error("permission denied.\n");
		audit_rshd_fail("Login incorrect", hostname,
			remuser, locuser, cmdbuf);	/* BSM */
		exit(1);
	}

	/*
	 * maintain 2.1 and 4.* and BSD semantics with anonymous rshd
	 */
	if (shpwd->sp_pwdp != 0 && *shpwd->sp_pwdp != '\0' &&
	    (v = pam_authenticate(pamh, 0)) != PAM_SUCCESS) {
		error("permission denied\n");
		audit_rshd_fail("Permission denied", hostname,
			remuser, locuser, cmdbuf);	/* BSM */
		pam_end(pamh, v);
		exit(1);
	}

	if ((v = pam_acct_mgmt(pamh, 0)) != PAM_SUCCESS) {
		switch (v) {
		case PAM_AUTHTOKEN_REQD:
			error("password expired\n");
			audit_rshd_fail("Password expired", hostname,
				remuser, locuser, cmdbuf); /* BSM */
			break;
		case PAM_PERM_DENIED:
			error("account expired\n");
			audit_rshd_fail("Account expired", hostname,
				remuser, locuser, cmdbuf); /* BSM */
			break;
		case PAM_AUTHTOK_EXPIRED:
			error("password expired\n");
			audit_rshd_fail("Password expired", hostname,
				remuser, locuser, cmdbuf); /* BSM */
			break;
		default:
			error("login incorrect\n");
			audit_rshd_fail("Permission denied", hostname,
				remuser, locuser, cmdbuf); /* BSM */
			break;
		}
		pam_end(pamh, PAM_ABORT);
		exit(1);
	}

	/*
	 * XXX There is no session management currently being done
	 */

	(void) write(2, "\0", 1);
	if (port) {
		if (pipe(pv) < 0) {
			error("Can't make pipe.\n");
			pam_end(pamh, PAM_ABORT);
			exit(1);
		}
		pid = fork();
		if (pid == (pid_t)-1)  {
error("Fork (to start shell) failed on server.  Please try again later.\n");
			pam_end(pamh, PAM_ABORT);
			exit(1);
		}

#ifndef MAX
#define	MAX(a, b) (((u_int)(a) > (u_int)(b)) ? (a) : (b))
#endif /* MAX */

		if (pid) {
			int width = MAX(s, pv[0]) + 1;
			fd_set ready;
			fd_set readfrom;

			(void) close(0); (void) close(1); (void) close(2);
			(void) close(f); (void) close(pv[1]);
			FD_ZERO(&ready);
			FD_ZERO(&readfrom);
			FD_SET(s, &readfrom);
			FD_SET(pv[0], &readfrom);
			if (ioctl(pv[0], FIONBIO, (char *)&one) == -1)
				syslog(LOG_INFO, "ioctl FIONBIO: %m");
			/* should set s nbio! */
			do {
				ready = readfrom;
				if (select(width, &ready, (fd_set *)0,
				    (fd_set *)0, (struct timeval *)0) < 0)
					break;
				if (FD_ISSET(s, &ready)) {
					if (read(s, &sig, 1) <= 0)
						FD_CLR(s, &readfrom);
					else
						killpg(pid, sig);
				}
				if (FD_ISSET(pv[0], &ready)) {
					errno = 0;
					cc = read(pv[0], buf, sizeof (buf));
					if (cc <= 0) {
						shutdown(s, 1+1);
						FD_CLR(pv[0], &readfrom);
					} else
						(void) write(s, buf, cc);
				}
			} while (FD_ISSET(s, &readfrom) ||
				    FD_ISSET(pv[0], &readfrom));
			exit(0);
		}
		/* setpgrp(0, getpid()); */
		setsid();	/* Should be the same as above. */
		(void) close(s); (void) close(pv[0]);
		dup2(pv[1], 2);
		(void) close(pv[1]);
	}
	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = "/bin/sh";
	(void) close(f);

	/*
	 * write audit record before making uid switch
	 */
	audit_rshd_success(hostname, remuser, locuser, cmdbuf); /* BSM */

	/* set the real (and effective) GID */
	if (setgid(pwd->pw_gid) == -1) {
		error("Invalid gid.\n");
		pam_end(pamh, PAM_ABORT);
		exit(1);
	}

	/*
	 * Initialize the supplementary group access list.
	 */
	if (!locuser) {
		error("Initgroup failed.\n");
		pam_end(pamh, PAM_ABORT);
		exit(1);
	}
	if (initgroups(locuser, pwd->pw_gid) == -1) {
		error("Initgroup failed.\n");
		pam_end(pamh, PAM_ABORT);
		exit(1);
	}

	if (retval = pam_setcred(pamh, PAM_ESTABLISH_CRED)) {
		error("Insufficent credentials.\n");
		pam_end(pamh, retval);
		exit(1);
	}

	/* set the real (and effective) UID */
	if (setuid(pwd->pw_uid) == -1) {
		error("Invalid uid.\n");
		pam_end(pamh, PAM_ABORT);
		exit(1);
	}

	/* Change directory only after becoming the appropriate user. */
	if (chdir(pwd->pw_dir) < 0) {
		(void) chdir("/");
#ifdef notdef
		error("No remote directory.\n");
		exit(1);
#endif
	}

	pam_end(pamh, PAM_SUCCESS);

#ifdef	SYSV
	if (pwd->pw_uid)
		envinit[ENVINIT_PATH] = userpath;
	else
		envinit[ENVINIT_PATH] = rootpath;
	if (tzenv = getenv("TZ")) {
		/*
		 *	In the line below, 4 is strlen("TZ=") + 1 null byte.
		 *	We have to malloc the space because it's difficult to
		 *	compute the maximum size of a timezone string.
		 */
		tz = (char *) malloc(strlen(tzenv) + 4);
		if (tz) {
			strcpy(tz, "TZ=");
			strcat(tz, tzenv);
			envinit[ENVINIT_TZ] = tz;
		}
	}
#endif	/* SYSV */
	strncat(homedir, pwd->pw_dir, sizeof (homedir)-6);
	strncat(shell, pwd->pw_shell, sizeof (shell)-7);
	strncat(username, pwd->pw_name, sizeof (username)-6);
	cp = rindex(pwd->pw_shell, '/');
	if (cp)
		cp++;
	else
		cp = pwd->pw_shell;
#ifdef	SYSV
	/*
	 * rdist has been moved to /usr/bin, so /usr/ucb/rdist might not
	 * be present on a system.  So if it doesn't exist we fall back
	 * and try for it in /usr/bin.  We take care to match the space
	 * after the name because the only purpose of this is to protect
	 * the internal call from old rdist's, not humans who type
	 * "rsh foo /usr/ucb/rdist".
	 */
#define	RDIST_PROG_NAME	"/usr/ucb/rdist -Server"
	if (strncmp(cmdbuf, RDIST_PROG_NAME, strlen(RDIST_PROG_NAME)) == 0) {
		if (stat("/usr/ucb/rdist", &statb) != 0) {
			strncpy(cmdbuf + 5, "bin", 3);
		}
	}
#endif
	execle(pwd->pw_shell, cp, "-c", cmdbuf, (char *)0, envinit);
	perror(pwd->pw_shell);
	exit(1);
}

void
getstr(buf, cnt, err)
	char *buf;
	int cnt;
	char *err;
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		*buf++ = c;
		if (--cnt == 0) {
			error("%s too long\n", err);
			exit(1);
		}
	} while (c != 0);
}

/*VARARGS1*/
void
error(fmt, a1, a2, a3)
	char *fmt;
	int a1, a2, a3;
{
	char buf[BUFSIZ];

	buf[0] = 1;
	(void) sprintf(buf+1, fmt, a1, a2, a3);
	(void) write(2, buf, strlen(buf));
}
