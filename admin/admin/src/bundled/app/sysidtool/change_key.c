
/*
 *  Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 */


/*
 *	File:		change_key.c
 *
 *	Description:	This file contains the routines needed to
 *			set up a publickey entry for a host.
 *
 *			We run /usr/bin/chkey to handle the actual
 *			details, providing it with the host's root
 *			password.
 */

#pragma	ident	"@(#)change_key.c	1.5	92/09/03 SMI"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <stropts.h>
#include <poll.h>
#include "sysidtool.h"

#define	KYS_EATPROMPT	0	/* eat the "Password:" prompt */
#define	KYS_PASSTHRU	1	/* pass thru everthing chkey outputs */

/*
 * Various states when reading characters from /usr/bin/chkey program, so we
 * can keep track of what is happening.
 */
#define	NO_HALT		0	/* buffer up characters */
#define	MAYBE		1	/* colon character received, possible EOL */
#define	FIRM_HALT	2	/* no more input from /usr/bin/chkey, so */
				/* process the message */

typedef	struct msg_map	Msg_map;
struct	msg_map {
	char		*msg;
	Sysid_msg	code;
};

static	Msg_map	ky_msgs[] = {
	{"Retype password:",				TAG(KY_REENTER)},
};

#define	SIZE_KY_MSGS	(sizeof (ky_msgs) / sizeof (Msg_map))



typedef	enum ky_status	KY_status;
enum	ky_status {
	KY_SUCCESS,		/* session completed, no error */
	KY_OK,			/* everything working normally */
	KY_FATAL_ERROR		/* internal technical error, notify user */
};

static	KY_status	set_key();
static	KY_status	grab_pty();
static	KY_status	exec_key();
static	KY_status	key_error();
static	KY_status	check_chkey_status();
static	void		restore_pty();


static	char		*line;
static	int		keylinestate;
static	int		fd_pty;
static	pid_t		ky_pid;


/*
 * set_publickey
 *
 *	This routine is the client interface routine used for
 *	setting up the publickey entry for a host.
 *
 *	Run /usr/sbin/chkey via a pty to actually do the work.
 *
 *	Input: Host's root password.
 *
 *	Output: If successful, this routine returns a status of 0.
 *		If an error occurs, this routine returns -1.
 */

int
set_publickey(passwd)
char *passwd;
{
	KY_status 	status;

	/* Make sure libcurses has been initialized */
	(void) start_curses();

	/* Grab a pty pair */
	if (grab_pty() != KY_OK)
		return (-1);

	/* Start up the chkey program */
	if (exec_key() != KY_OK)
		return (-1);

	/*
	 * Run (interact with) the /usr/bin/chkey program.
	 */
	status = set_key(passwd);

	if (status == KY_FATAL_ERROR) {
		clear();
		refresh();
		(void) prompt_error(TAG(CANT_DO_CHKEY));
		return (-1);
	}

	return (0);
}



/*
 * set_key
 *
 *	Main event loop for interacting with the /usr/bin/chkey
 *	program.  The host's root password is sent twice to the
 *	/usr/bin/chkey program via a pty.  Input from /usr/bin/chkey
 *	(via the pty) is scanned to determine what is happening in
 *	the program.
 *
 *	WARNING!  Changing code in this routine is not for the
 *	faint-of-heart.  You really need nerves of steel, even
 *	when traced under the debugger.  Take note of the comments
 *	that scream at you.  Nothing personal - they are there for
 *	your own good.
 *
 *	The caller should provide the host's root password (in cleartext)
 *	as input to this routine.
 *
 *	Return status code indicates the current status of this session.
 */

static KY_status
set_key(passwd)
char *passwd;
{
	int		i;
	char		c;
	KY_status	status;
	struct pollfd	fds;
	int		chars_passed, halt;
	int 		ndx;
	char 		msgbuf[256];	/* buffered input from chkey */
	char		pwbuf[MAX_PASSWORD+3];
	int		plen;

	/*
	 * Set up to catch input on the pty.
	 */
	fds.fd = fd_pty;	/* pty */
	fds.events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI;
	fds.revents = 0;
	halt = NO_HALT;

	/*
	 * Set up the password as input to the program.
	 */
	plen = sprintf(pwbuf, "%s\n", passwd);

	/*
	 * Main event loop.  For each loop, we need to check the status of
	 * the chkey program (due to problem with poll with infinite timeout).
	 */
	while ((status = check_chkey_status()) == KY_OK) {

		/*
		 * Wait for input on the pty.
		 * DO NOT MAKE THE POLL BLOCK BY SETTING A TIMEOUT!
		 * THAT DOESN'T SEEM TO WORK ON SVR4.
		 */
		if (poll(&fds, 1, 0) < 0) {
			restore_pty(line, fd_pty);
			return (key_error());
		}

		/* See if we got an event on the pty */
		if (fds.revents != 0) {		/* pty */
			i = read(fd_pty, &c, 1);
			if (i < 0) {
				if ((errno == EIO) || (errno == EINVAL))
					continue;
				restore_pty(line, fd_pty);
				return (key_error());
			}
			if (i == 0)		/* gone away */
				continue;

			/* what to do? */
			switch (keylinestate) {

			case KYS_EATPROMPT:
				/* prompt end in colon */
				if (c != ':')
					continue;	/* throw away */
				/* Ready for password */
				if (write(fd_pty, pwbuf, plen) != plen) {
					restore_pty(line, fd_pty);
					return (key_error());
				}
				keylinestate = KYS_PASSTHRU;
				break;

			case KYS_PASSTHRU:

				/*
				 * Buffer up all characters up to the newline.
				 * Note that a colon (:) could be the last
				 * character of a line, so we should be prepared
				 * for the case.
				 */
				if ((c != '\n') && (c != '\r')) {
					if (halt != FIRM_HALT) {
						msgbuf[ndx++] = c;
						msgbuf[ndx] = NULL;
						chars_passed = 1;
						if (c == ':')
							halt = MAYBE;
						else
							halt = NO_HALT;
					}
				} else {
					if (chars_passed)
						halt = FIRM_HALT;
				}
				break;
			}

		} else {

			/*
			 * No events on the pty.  Check to see whether or
			 * not we have a message from /usr/bin/chkey.
			 * If so, then signal that it should be processed.
			 */
			if (halt == MAYBE)
				halt = FIRM_HALT;
		}

		/*
		 * If we have a message from /usr/bin/chkey that needs to
		 * be processed, then do so.  Otherwise, ignore the message.
		 */
		if ((halt == FIRM_HALT) && (msgbuf[0] != NULL)) {

			/* Try to map it */
			for (i = 0; i < SIZE_KY_MSGS; i++)
				if (!strcmp(msgbuf, ky_msgs[i].msg))
					break;

			/*
			 * If the message is requesting the password to
			 * be re-entered, then send the password again
			 * to /usr/bin/chkey.
			 */
			if ((i < SIZE_KY_MSGS) &&
			    (ky_msgs[i].code == TAG(KY_REENTER))) {
				if (write(fd_pty, pwbuf, plen) != plen) {
					restore_pty(line, fd_pty);
					return (key_error());
				}
			}

			/* Set up for next message */
			msgbuf[0] = NULL;
			halt = NO_HALT;
			chars_passed = 0;
			ndx = 0;
		}
	}

	return (status);
}

/*
 * check_chkey_status
 *
 *	Check on status of /usr/bin/chkey program.
 *	Report that either the program is still alive,
 *	or has failed.
 */
static KY_status
check_chkey_status()
{
	int	status;
	pid_t	state;

	state = waitpid(ky_pid, &status, WNOHANG);
	if (state == ky_pid) {
		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) != 0)
				return (KY_FATAL_ERROR);
			return (KY_SUCCESS);
		}
	} else if (state == -1L) {
		restore_pty(line, fd_pty);
		return (key_error());
	}

	return (KY_OK);
}


/*
 * grab_pty
 *
 *	Scan /dev and open an unused pty
 */

static KY_status
grab_pty()
{
	int		c, i;
	int		pgrp;
	struct stat	stb;

	/* grab a pty pair */
	for (c = 'p'; c <= 's'; c++) {

		line = "/dev/ptyXX";
		line[strlen("/dev/pty")] = c;
		line[strlen("/dev/ptyp")] = '0';
		if (stat(line, &stb) < 0)
			break;

		for (i = 0; i < 16; i++) {
			line[strlen("/dev/ptyp")] = "0123456789abcdef"[i];
			line[strlen("/dev/")] = 't';
			/*
			 * Lock the slave side so that no one else can
			 * open it after this.
			 */
			if (chmod (line, 0600) == -1)
				continue;
			line[strlen("/dev/")] = 'p';
			if ((fd_pty = open(line, O_RDWR)) == -1) {
				restore_pty(line, fd_pty);
				continue;
			}

			/*
			 * XXX - Using a side effect of TIOCGPGRP on ptys.
			 * May not work as we expect in anything other than
			 * SunOS 4.1.
			 */
			if ((ioctl(fd_pty, TIOCGPGRP, &pgrp) == -1) &&
			    (errno == EIO))
				return (KY_OK);
			else {
				restore_pty(line, fd_pty);
			}
		}
	}

	return (key_error());
}

/*
 * restore_pty
 *
 * 	Restore the permissions on the pty and close it
 */

static void
restore_pty(pty_name, pty_fd)
	char *pty_name;
	int   pty_fd;
{

	pty_name[strlen("/dev/")] = 't';
	(void) chmod(pty_name, 0666);
	pty_name[strlen("/dev/")] = 'p';

	if (pty_fd != -1)
		close(pty_fd);
}

/*
 * exec_key
 *
 *	fork a child process and exec /usr/bin/chkey
 */

static KY_status
exec_key()
{
	int	tt;

	keylinestate = KYS_EATPROMPT;

	/* fork, the child then runs /usr/bin/chkey */
	ky_pid = fork();
	if (ky_pid < 0) {
		restore_pty(line, fd_pty);
		return (key_error());
	}


	/*
	 * This is the child  process that runs the chkey command.
	 * This process redirects its stdin, stdout, and stderr to
	 * the pty, which is being read by the parent.
	 */
	if (ky_pid == 0) {

		/*
		 * The child process needs to be the session leader
		 * and have the pty as its controlling tty.
		 */
		setpgrp();		/* setsid */
		line[strlen("/dev/")] = 't';
		tt = open(line, O_RDWR);
		if (tt < 0)
			(void) exit(1);

		(void) close(fd_pty);
		if (tt != 0)
			(void) dup2(tt, 0);
		if (tt != 1)
			(void) dup2(tt, 1);
		if (tt != 2)
			(void) dup2(tt, 2);
		if (tt > 2)
			(void) close(tt);

		/*
		 * Force default English message for passwd, so we
		 * at least have something we can parse.
		 */
		putenv("LC_MESSAGES=C");

		execl("/usr/bin/chkey", "chkey", (char *)0);

		(void) exit(1);
		/*NOTREACHED*/
	}

	/* This is the parent. */
	return (KY_OK);
}




/*
 * key_error
 *
 *	dummy routine, convenient for setting a break point
 *	during debugging to trap on any fatal error
 */
static KY_status
key_error()
{
	return (KY_FATAL_ERROR);
}
