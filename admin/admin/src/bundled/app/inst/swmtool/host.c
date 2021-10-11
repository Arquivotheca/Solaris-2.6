/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved. Sun
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

#ifndef lint
#ident	"@(#)host.c 1.11 94/10/13"
#endif

#include "defs.h"
#include "ui.h"
#include "host.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <poll.h>
#include <stropts.h>
#include <sys/param.h>
#include <sys/filio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mnttab.h>
#include <sys/mntent.h>

#define	HOST_TIMEOUT	10		/* seconds to wait if host down */

static sigjmp_buf env;

Hostlist	hostlist;		/* hosts we know about */
int		hosts_selected;		/* number of hosts selected */

static Hostlist	*freelist;

/*
 * local definitions
 */
static void timeout(int);
static void host_link(Hostlist *, Hostlist *);
static void host_unlink(Hostlist *);
static int rpassok(char *, u_short, char *, char *);
static void host_signal(int);
static void get_host_type(FILE *, Hostlist *);
static char *get_client_arch(Hostlist *);
static int stderr_off(void);
static void stderr_on(int);

/*ARGSUSED*/
static void
timeout(sig)
	int	sig;
{
	siglongjmp(env, 1);
}

Hostlist *
host_alloc(name)
	char	*name;
{
	Hostlist *new;

	if (freelist != (Hostlist *)0) {
		new = freelist;
		freelist = new->h_next;
	} else
		new = (Hostlist *)xmalloc(sizeof (Hostlist));
	new->h_name = xstrdup(name);
	new->h_passwd = (char *)0;
	new->h_rootdir = (char *)0;
	new->h_arch = (char *)0;
	new->h_type = unknown;
	new->h_status = 0;
	new->h_next = new->h_prev = new;
	return (new);
}

/*
 * Link node into list after
 * argument node
 */
static void
host_link(where, new)
	Hostlist *where;
	Hostlist *new;
{
	new->h_prev = where;
	new->h_next = where->h_next;
	where->h_next->h_prev = new;
	where->h_next = new;
}

static void
host_unlink(which)
	Hostlist *which;
{
	which->h_prev->h_next = which->h_next;
	which->h_next->h_prev = which->h_prev;
}

/*
 * Initialize list of target hosts from string
 * containing host names.
 */
void
host_init(hostnames)
	char	*hostnames;
{
	Hostlist *new;
	char	*name, *end;
	char	cmd[BUFSIZ];
	FILE	*fp;

	/*
	 * Initialize list head and first node (us)
	 */
	if (hostlist.h_next == (Hostlist *)0) {
		hostlist.h_next = hostlist.h_prev = &hostlist;
		new = host_alloc(thishost);
		(void) sprintf(cmd, "%s; %s", GETARCH, GETMNTTAB);
		fp = popen(cmd, "r");
		if (fp == (FILE *)0)
			die(gettext(
			    "PANIC:  cannot open local /etc/mnttab file\n"));
		get_host_type(fp, new);
		(void) pclose(fp);
		new->h_status =
		    HOST_SELECTED | HOST_UP;
		if (new->h_type != diskless)
			new->h_status |= (HOST_MOUNTED | HOST_LOCAL);
		hosts_selected = 1;
		host_link(&hostlist, new);
	} else
		return;

	if (hostnames != (char *)0) {
		name = hostnames;
		while (*name != '\0') {
			while (isspace((u_char)*name))
				name++;
			end = name;
			while (!isspace((u_char)*end) && *end != '\0')
				end++;
			if (*name != '\0') {
				char	namebuf[MAXHOSTNAMELEN];

				(void) strncpy(namebuf, name, end - name);
				namebuf[end - name] = '\0';
				name = end;
				if (host_unique(&hostlist, namebuf)) {
					new = host_alloc(namebuf);
					host_link(hostlist.h_prev, new);
				}
			}
		}
	}
}

/*
 * Insert the named host in the target host
 * list before the argument host.
 */
Hostlist *
host_insert(where, name)
	Hostlist *where;
	char	*name;
{
	Hostlist *new;

	/*
	 * Initialize if necessary
	 */
	host_init((char *)0);

	new = host_alloc(name);
	host_link(where->h_prev, new);
	return (new);
}

void
host_remove(host)
	Hostlist *host;
{
	free((void *)host->h_name);
	free((void *)host->h_passwd);
	free((void *)host->h_rootdir);
	host->h_next = freelist;
	host->h_prev = (Hostlist *)0;
	host->h_name = (char *)0;
	freelist = host;
}

void
host_clear(host)
	Hostlist *host;
{
	Hostlist *hlp, *next;

#ifdef lint
	next = host;
#endif
	for (hlp = host->h_next; hlp != host; hlp = next) {
		next = hlp->h_next;
		host_unlink(hlp);
		host_remove(hlp);
	}
	hosts_selected = 0;
}

int
host_select(host)
	Hostlist *host;
{
	int	status;

	if ((host->h_status & HOST_SELECTED) == 0) {
		status = check_host_info(host);
		if (status == SUCCESS) {
			host->h_status ^= HOST_SELECTED;
			hosts_selected++;
		}
	} else {
		host->h_status ^= HOST_SELECTED;
		hosts_selected--;
		status = SUCCESS;
	}
	return (status);
}

/*
 * Attempt to contact a host and determine whether it is a
 * standalone/server, dataless client, or diskless client.
 * Also determine if we need a password and if so, get it.
 * Fills in the type, status, and passwd members of the
 * argument host structure.
 *
 * Todo:
 *	should do mount point and space checking
 *
 */
int
check_host_info(host)
	Hostlist *host;
{
#ifdef __STDC__
	void	(*savesig)(int);
#else
	void	(*savesig)();
#endif
	FILE	*cmdp;
	struct servent *svc;
	char	*hostname = host->h_name;
	char	clientdir[MAXPATHLEN];
	char	cmd[BUFSIZ];
	int	s = -1;
	int	errfd;
	int	status;

	if (strcmp(hostname, thishost) == 0) {
		host->h_status |= (HOST_UP|HOST_PWDNONE);
		return (SUCCESS);
	}
	(void) sprintf(cmd, "%s; %s", GETARCH, GETMNTTAB);

	/*
	 * Short-cut for diskless clients:  ask the
	 * user for confirmation if we find an
	 * /export/root/<hostname> dir on our system.
	 */
	if (host->h_type == unknown) {
		(void) sprintf(clientdir, "%s/%.*s",
		    CLIENTROOT, MAXPATHLEN - (strlen(CLIENTROOT) + 2),
		    hostname);
		if (access(clientdir, R_OK|W_OK|X_OK) == 0) {
			if (confirm(Hostscreen,
			    xstrdup(gettext("Verify Diskless")),
			    xstrdup(gettext("Assume Diskless")),
			    xstrdup(gettext(
		"Host `%s' appears to be a diskless client of\n"
		"this system.  Choose whether or not to verify this fact.\n"
		"If you choose verification, you will be required to enter\n"
		"a password if %s does not allow root logins\n"
		"from this system.")),
			    hostname, hostname) == 0) {
				host->h_rootdir = xstrdup(clientdir);
				host->h_status &= ~HOST_PWDBITS;
				host->h_status |=
				    (HOST_UP|HOST_LOCAL|HOST_PWDNONE);
				host->h_type = diskless;
				host->h_arch = get_client_arch(host);
				return (SUCCESS);
			}
		}
	}

	/*
	 * For known diskless clients (by definition,
	 * they are on the local system -- if not, we
	 * consider them dataless) and the local host
	 * (assuming we know our type) we can return.
	 */
	if (host->h_status & HOST_LOCAL && host->h_type != unknown)
		return (SUCCESS);

	/*
	 * At a minimum, we're going to
	 * ping the remote host.
	 */
	host->h_status &= ~HOST_UP;

	errno = 0;
	/*
	 * If we don't know that we need a password,
	 * try command assuming hosts are equivalent.
	 * We need to turn stderr off to keep rcmd from
	 * scribbling on our screen in case the two roots
	 * aren't equivalent or there are other problems.
	 */
	if ((host->h_status & HOST_PWDREQ) == 0) {
		svc = getservbyname("cmd", "tcp");
		if (svc == (struct servent *)0)
			die(gettext(
		"PANIC:  cannot determine port number for rcmd service\n"));
		errfd = stderr_off();
		if (sigsetjmp(env, 1) == 0) {
			savesig = signal(SIGALRM, timeout);
			(void) alarm(HOST_TIMEOUT);
			s = rcmd(&hostname, (u_short)svc->s_port,
			    "root", "root", cmd, (int *)0);
			(void) alarm(0);
			(void) signal(SIGALRM, savesig);
		} else {
			(void) signal(SIGALRM, savesig);
			errno = ETIMEDOUT;
			s = -1;
		}
		stderr_on(errfd);
		if (s != -1) {
			host->h_status |= (HOST_UP|HOST_PWDNONE);
			if (host->h_passwd) {
				/*
				 * We don't need a password
				 * so free it.
				 */
				free(host->h_passwd);
				host->h_passwd = (char *)0;
			}
			if (host->h_type != unknown) {
				/*
				 * we now know up, f/s, and
				 * passwd status
				 */
				(void) close(s);
				return (SUCCESS);
			}
		}
	}
	/*
	 * If the two roots aren't equivalent, we'll have
	 * to use passwords.  We'll notify the user if we
	 * discover the remote root account has a password
	 * on it and we don't have the correct one.
	 */
	if (s < 0 && errno != ETIMEDOUT) {	/* XXX */
		status = check_host_passwd(host);
		if (status != SUCCESS)
			return (status);

		if (host->h_type != unknown)
			/*
			 * we now know up, f/s, and
			 * passwd status
			 */
			return (SUCCESS);
		/*
		 * Use the password we just verified
		 * and open a connection for reading the
		 * other guy's mnttab.
		 */
		svc = getservbyname("exec", "tcp");
		if (svc == (struct servent *)0)
			die(gettext(
	    "PANIC:  cannot determine port number for rexec service\n"));
		errfd = stderr_off();
		if (sigsetjmp(env, 1) == 0) {
			savesig = signal(SIGALRM, timeout);
			(void) alarm(HOST_TIMEOUT);
			s = rexec(&hostname, (u_short)svc->s_port,
			    "root", host->h_passwd, cmd, (int *)0);
			(void) alarm(0);
			(void) signal(SIGALRM, savesig);
		} else {
			(void) signal(SIGALRM, savesig);
			errno = ETIMEDOUT;
			s = -1;
		}
		stderr_on(errfd);
	}

	if (s != -1) {
		host->h_status |= HOST_UP;
		cmdp = fdopen(s, "r");
		if (cmdp == (FILE *)0)
			die(gettext(
	    "PANIC:  Failed to convert file descriptor to file stream."));

		if (host->h_type == unknown) {
			get_host_type(cmdp, host);
			if (host->h_type == unknown)
				status = ERR_FSTYPE;
		}

		(void) fclose(cmdp);
		(void) close(s);
		status = SUCCESS;
	} else {
		host->h_status &= ~HOST_UP;
		status = ERR_HOSTDOWN;
	}
	return (status);
}

/*
 * Test password for user on host.  This is
 * a rather crude hack...
 * Code mostly taken from rexec.c.
 *
 * Returns:
 *	-1			various local system errors
 *	0, errno != EPERM	remote system error (host down)
 *	0, errno == EPERM	password invalid
 *	1			password valid
 */
static int
#ifdef __STDC__
rpassok(char *host,
	u_short	rport,
	char	*name,
	char	*pass)
#else
rpassok(host, rport, name, pass)
	char	*host;
	u_short	rport;
	char	*name;
	char	*pass;
#endif
{
	int s, timo = 1;
	struct sockaddr_in sin;
	struct hostent *hp;
	char	str[BUFSIZ];
	char	*cp;
	int	status;
	char	*cmd = "/bin/echo OK";
#ifdef __STDC__
	void	(*savesig)(int);
#else
	void	(*savesig)();
#endif

	hp = gethostbyname(host);
	if (hp == (struct hostent *)0)
		return (-1);
	host = hp->h_name;
retry:
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return (-1);
	sin.sin_family = hp->h_addrtype;
	sin.sin_port = rport;
	(void) memcpy((caddr_t)&sin.sin_addr, hp->h_addr, hp->h_length);
	if (sigsetjmp(env, 1) == 0) {
		savesig = signal(SIGALRM, timeout);
		(void) alarm(HOST_TIMEOUT);
		status = connect(s, (struct sockaddr *)&sin, sizeof (sin));
		(void) alarm(0);
		(void) signal(SIGALRM, savesig);
		if (status < 0) {
			if (errno == ECONNREFUSED && timo <= 16) {
				(void) close(s);
				(void) sleep(timo);
				timo *= 2;
				goto retry;
			}
			(void) close(s);
			return (-1);
		}
	} else {
		(void) signal(SIGALRM, savesig);
		errno = ETIMEDOUT;
		return (0);
	}
	(void) write(s, "", 1);
	(void) write(s, name, strlen(name) + 1);
	(void) write(s, pass, strlen(pass) + 1);
	(void) write(s, cmd, strlen(cmd) + 1);
	cp = str;
	status = 0;
	if (read(s, cp, 1) == 1) {
		if (*cp != '\0') {	/* this 1st byte is discarded */
			while (read(s, cp, 1) == 1) {
				if (*cp++ == '\n')
					break;
			}
			*cp = '\0';
			/*
			 * i18n may screw this up -- depends on what
			 * locale the originators of these messages are
			 * running under.  Since we can't differentiate
			 * between translated versions of these message
			 * and other messages (like "No home directory"),
			 * we have to check for command success if we
			 * don't see a match.
			 */
			if (strcmp(str, "Permission denied.\n") &&
			    strcmp(str, "Password incorrect.\n")) {
				cp = str;
				*cp = '\0';
				while (read(s, cp, 1) == 1) {
					if (*cp++ == '\n')
						break;
				}
				*cp = '\0';
				if (strcmp(str, "OK\n") == 0)
					status = 1;
			}
		} else
			status = 1;
	}

	(void) close(s);
	if (status == 0)
		errno = EPERM;
	return (status);
}

/*
 * Sets password status bits in host.
 */
int
check_host_passwd(host)
	Hostlist *host;
{
	struct servent *svc;
	char	*pass, *name;
	int	status;

	/*
	 * XXX  don't need to check
	 */
	if (host->h_status & HOST_PWDNONE)
		return (SUCCESS);

	host->h_status &= ~HOST_PWDBITS;	/* clear all, we'll reset */
	host->h_status |= HOST_PWDREQ;

	svc = getservbyname("exec", "tcp");
	if (svc == (struct servent *)0)
		die(gettext(
	    "PANIC:  cannot determine port number for rexec service\n"));

	name = host->h_name;
	pass = host->h_passwd ? host->h_passwd : "";
	/*
	 * Check to see if argument password
	 * is correct, or if no password is
	 * required.
	 */
	status = rpassok(name, (u_short)svc->s_port, "root", pass);

	if (status > 0) {
		host->h_status |= (HOST_PWDOK|HOST_UP);
		if (host->h_passwd == (char *)0)
			host->h_passwd = xstrdup("");
		return (SUCCESS);
	}

	if (status < 0 || errno != EPERM) {
		host->h_status &= ~HOST_UP;
		return (ERR_HOSTDOWN);	/* system error or timed out */
	}

	host->h_status |= HOST_UP;

	if (host->h_passwd != (char *)0) {
		host->h_status |= HOST_PWDBAD;
		return (ERR_INVPASSWD);
	} else
		return (ERR_NOPASSWD);
}

static void
get_host_type(mntp, host)
	FILE	*mntp;		/* file stream of open mnttab-format file */
	Hostlist *host;		/* host we're checking */
{
	char	buf[BUFSIZ];
	struct mnttab mnt;
	char	server[MAXHOSTNAMELEN];
	int	seenroot = 0;	/* =1 if we find root mnttab entry */
	int	remoteusr = 0;	/* =1 if usr is remote mounted */
	char	*remoteroot = (char *)0;
	char	*cp;

	server[0] = '\0';

	if (fgets(buf, sizeof (buf), mntp) &&
	    strncmp(buf, ARCHSTR, strlen(ARCHSTR)) == 0) {
		cp = strchr(buf, '\n');
		if (cp != (char *)0)
			*cp = '\0';
		host->h_arch = xstrdup(&buf[strlen(ARCHSTR)]);
	} else
		host->h_arch = xstrdup("");

	if (fgets(buf, sizeof (buf), mntp) &&
	    strncmp(buf, MNTSTR, strlen(MNTSTR)) == 0) {
		while (getmntent(mntp, &mnt) != -1) {
			/*
			 * Look in mnttab for /
			 */
			if (strcmp(mnt.mnt_mountp, "/") == 0) {
				seenroot = 1;
				if (strcmp(mnt.mnt_fstype, "nfs") == 0) {
					cp = strchr(mnt.mnt_special, ':');
					if (cp != (char *)0) {
						*cp++ = '\0';
						(void) strcpy(
						    server, mnt.mnt_special);
						remoteroot = xstrdup(cp);
					}
				}
				continue;
			}
			if (strcmp(mnt.mnt_mountp, "/usr") == 0) {
				if (strcmp(mnt.mnt_fstype, "nfs") == 0)
					remoteusr = 1;
				continue;
			}
		}
	}

	if (seenroot == 0) {
		/*
		 * Return error if we don't have
		 * enough information.  NB:  We'll
		 * allow single-partition systems
		 * (i.e., it's not an error if we
		 * don't see a /usr).
		 */
		host->h_type = error;
	} else if (remoteroot) {
		/*
		 * If / is remote and we are the named server
		 * for it, it's diskless.  If we're not the
		 * server, we'll treat it as dataless.
		 */
		if (strcmp(host_canon(server), host_canon(thishost)) == 0) {
			host->h_rootdir = remoteroot;
			host->h_type = diskless;
			host->h_status |= HOST_LOCAL;
		} else
			host->h_type = dataless;
	} else if (remoteusr) {
		/*
		 * If /usr is remote, it's dataless.
		 */
		host->h_type = dataless;
	} else {
		/*
		 * Must be standalone/server
		 */
		host->h_type = standalone;
	}
}

#define	STATUS_PREFIX	"STATUS=="

static int	errfd;

/*
 * Called when signal is received while running remote
 * command -- pass the signal on to the remote process.
 */
static void
host_signal(sig)
	int	sig;
{
	u_char	c = (u_char)sig;

	if (errfd >= 0)
		(void) write(errfd, &c, 1);
}

int
host_run_cmd(host, cmd)
	Hostlist *host;
	char	*cmd;
{
#ifdef __STDC__
	void	(*savesig)(int);
#else
	void	(*savesig)();
#endif
	struct pollfd fds[2];
	struct servent *svc;
	struct sigaction sa, saveint, savequit;
	char	*hostname = host->h_name;
	char	buf[BUFSIZ+1], *cp;
	int	slen = strlen(STATUS_PREFIX);
	int	nl = 1;
	int	s = 0;

	(void) sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = host_signal;

	errfd = -1;

#ifdef DEMO
	(void) fprintf(stderr, "rsh %s %s\n", hostname, cmd);
#else
	(void) sprintf(buf, "%s; echo %s$?", cmd, STATUS_PREFIX);

	if (host->h_status & HOST_PWDREQ) {
		svc = getservbyname("exec", "tcp");
		if (svc == (struct servent *)0)
			die(gettext(
	    "PANIC:  cannot determine port number for rexec service\n"));
		if (sigsetjmp(env, 1) == 0) {
			savesig = signal(SIGALRM, timeout);
			(void) alarm(HOST_TIMEOUT);
			s = rexec(&hostname, (u_short)svc->s_port,
			    "root", host->h_passwd ? host->h_passwd : "",
			    buf, &errfd);
			(void) alarm(0);
			(void) signal(SIGALRM, savesig);
		} else {
			(void) signal(SIGALRM, savesig);
			errno = ETIMEDOUT;
			s = -1;
		}
	} else {
		svc = getservbyname("cmd", "tcp");
		if (svc == (struct servent *)0)
			die(gettext(
	    "PANIC:  cannot determine port number for rcmd service\n"));
		if (sigsetjmp(env, 1) == 0) {
			savesig = signal(SIGALRM, timeout);
			(void) alarm(HOST_TIMEOUT);
			s = rcmd(&hostname, (u_short)svc->s_port,
			    "root", "root", buf, &errfd);
			(void) alarm(0);
			(void) signal(SIGALRM, savesig);
		} else {
			(void) signal(SIGALRM, savesig);
			errno = ETIMEDOUT;
			s = -1;
		}
	}

	if (s < 0) {
		(void) fprintf(stderr, gettext(
		    "Communications error with host %s:  %s\n"),
			hostname, strerror(errno));
		return (-1);
	}

	(void) sigaction(SIGINT, &sa, &saveint);
	(void) sigaction(SIGQUIT, &sa, &savequit);

	fds[0].fd = 0;
	fds[0].events = POLLIN | POLLRDNORM | POLLRDBAND;

	fds[1].fd = s;
	fds[1].events = POLLIN | POLLRDNORM | POLLRDBAND;

	for (;;) {
		int	n;

		if (poll(fds, 2, INFTIM) < 0)
			break;	/* ??? */

		if (fds[1].revents & (POLLHUP | POLLERR))
			break;	/* EOF ??? */

		if (fds[1].revents & POLLIN) {
			char	*cp1, *cp2;

			n = read(s, buf, sizeof (buf) - 1);
			if (n <= 0)
				break;	/* EOF */
			buf[n] = '\0';
			cp1 = buf;
			while (n > 0) {
				/*
				 * Since output from several hosts may
				 * be interleaved we precede each line
				 * with the name of the host that generated
				 * the output.
				 */
				cp2 = strchr(cp1, '\n');
				if (cp2++ != (char *)0) {
					/*
					 * Don't print the status indication
					 * at the end of the cmd's output.
					 */
					if (strncmp(cp1, STATUS_PREFIX, slen)) {
						if (nl)
							(void) printf("%s: ",
								hostname);
						(void) printf("%.*s",
							cp2 - cp1, cp1);
						(void) fflush(stdout);
					}
					n -= (cp2 - cp1);
					cp1 = cp2;
					nl = 1;
				} else {
					if (nl) {
						(void) printf("%s: ", hostname);
						nl = 0;
					}
					(void) printf("%.*s", n, cp1);
					(void) fflush(stdout);
					n = 0;
				}
			}
		}
		if (fds[0].revents & POLLIN) {
			n = read(0, buf, sizeof (buf));
			if (n > 0)
				(void) write(s, buf, n);
			else if (n <= 0)
				break;
		}
	}
	(void) close (s);

	(void) sigaction(SIGINT, &saveint, (struct sigaction *)0);
	(void) sigaction(SIGQUIT, &savequit, (struct sigaction *)0);

	buf[BUFSIZ] = '\0';
	cp = strstr(buf, STATUS_PREFIX);
	if (cp != (char *)0)
		return (atoi(cp + strlen(STATUS_PREFIX)));
#endif
	return (0);
}

/*
 * Canonicalize a host name.
 */
char *
host_canon(name)
	char	*name;
{
	struct hostent *h;
	u_long	addr;
	int	len, type;

	if (strcmp(name, "localhost") == 0)
		return (thishost);

	h = gethostbyname(name);
	if (h == (struct hostent *)0)
		return (name);

	/*LINTED [alignment ok]*/
	addr = *(u_long *)h->h_addr_list[0];
	len = h->h_length;
	type = h->h_addrtype;

	h = gethostbyaddr((char *)&addr, len, type);
	if (h == (struct hostent *)0)
		return (name);

	return (h->h_name);
}

int
host_unique(list, name)
	Hostlist *list;
	char	*name;
{
	Hostlist *hlp;
#ifdef notdef
	char	*h1, *h2;

	h1 = host_canon(name);
	for (hlp = list->h_next; hlp != list; hlp = hlp->h_next) {
		h2 = host_canon(hlp->h_name);
		if (strcmp(h1, h2) == 0)
			return (0);
	}
#endif
	for (hlp = list->h_next; hlp != list; hlp = hlp->h_next) {
		if (strcmp(name, hlp->h_name) == 0)
			return (0);
	}
	return (1);
}

/*
 * Convert a list of hosts into a string suitable
 * for passing to the config file writing routines.
 * Space for the string is dynamically allocated
 * and can be freed with free().
 */
char *
host_string(list)
	Hostlist *list;
{
	Hostlist *hlp;
	char	buf[MAXHOSTNAMELEN+32];
	char	*hstr = (char *)0;
	int	len = 0;

	for (hlp = list->h_next; hlp != list; hlp = hlp->h_next) {
		if (len == 0) {
			(void) sprintf(buf, "%s", hlp->h_name);
			hstr = (char *)xmalloc(strlen(buf) + 1);
			(void) strcpy(hstr, buf);
		} else {
			(void) sprintf(buf, " %s", hlp->h_name);
			hstr = (char *)xrealloc(hstr, len + strlen(buf) + 1);
			(void) strcat(hstr, buf);
		}
		len += strlen(buf);
	}
	return (hstr);
}

/*
 * Get client's architecture by looking at its vfstab file.
 * We get it from the kvm entry, which looks like this:
 *
 *	server:/export/exec/kvm/Solaris_2.2_sparc.sun4c ...
 *
 * If this entry doesn't exist or is in a format we can't
 * parse, we return a null string.
 */
static char *
get_client_arch(Hostlist *host)
{
	char	vfspath[MAXPATHLEN];
	char	archstr[MAXPATHLEN];
	char	buf[1024];
	char	*arch, *cp;
	FILE	*fp;
	int	n;

	arch = (char *)0;

	(void) sprintf(vfspath, "%s/etc/vfstab", host->h_rootdir);
	fp = fopen(vfspath, "r");
	if (fp != (FILE *)0) {
		while (fgets(buf, sizeof (buf), fp)) {
			/*
			 * When we find kvm path, figure
			 * out architecture and break
			 */
			cp = strstr(buf, "/export/exec/kvm");
			if (cp) {
				n = sscanf(cp, "/export/exec/kvm/%s", archstr);
				if (n == 1) {
					cp = strrchr(archstr, '_');
					if (cp++)
						arch = xstrdup(cp);
				}
				break;
			}
		}
		(void) fclose(fp);
	}
	if (arch == (char *)0)
		arch = xstrdup("");	/* XXX no "real" default */
	return (arch);
}

/*
 * Turn off stderr.  This exists so rcmd and rexec don't
 * scribble on the screen if they encounter errors.
 */
static int
stderr_off(void)
{
	int	errfd;

	(void) fflush(stderr);

	errfd = dup(2);
	(void) fclose(stderr);
	(void) close(2);
	(void) open("/dev/null", O_WRONLY);
	return (errfd);
}

static void
stderr_on(errfd)
{
	FILE *fp;

	(void) close(2);
	(void) dup(errfd);
	(void) close(errfd);
	fp =  fdopen(2, "w");
	if (fp != (FILE *)0)
		*stderr = *fp;
}
