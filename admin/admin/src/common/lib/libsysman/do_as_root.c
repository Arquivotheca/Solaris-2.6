/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)do_as_root.c	1.13	95/07/21 SMI"


#include <unistd.h>
#include <limits.h>
#include <sys/systeminfo.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include "sysman_impl.h"


extern int		errno;
static const gid_t	admin_grp = 14;

static const char	*sec_violation_fmt =
	"Security exception on host %s.  USER ACCESS DENIED. The user "
	"identity %d (\"%s\") was received, but that user is not authorized "
	"to execute the requested functionality on this system.  Is this user "
	"a member of an appropriate security group (%d) on this system?";

static const char	*popen_fail_fmt = "Can't run \"%s\": popen failed";


boolean_t
in_admin_group_p(uid_t uid)
{

	int		i;
	int		cnt;
	struct passwd	*p;
	gid_t		gid;
	gid_t		grouplist[NGROUPS_MAX];


	/* Is user's primary group group the admin group? */

	gid = getgid();

	if (gid == admin_grp) {
		return (B_TRUE);
	}

	p = getpwuid(uid);
	if (initgroups(p->pw_name, gid) < 0) {
		return (B_FALSE);
	}

	/* Is one of user's secondary groups the admin group? */

	cnt = getgroups(NGROUPS_MAX, grouplist);

	for (i = 0; i < cnt; i++) {
		if (grouplist[i] == admin_grp) {
			return (B_TRUE);
		}
	}

	return (B_FALSE);
}


int
run_program_as_admin(
	const char	*prgm,
	boolean_t	interactive,
	char		*output_buf,
	int		len)
{

	FILE		*pp;
	int		n;
	uid_t		uid;
	struct passwd	*pw_p;
	char		h_name[257];
	char		sec_violation_str[1024];
	char		popen_fail_str[1024];
	int		pipe_fds[2];
	int		s;
	pid_t		pid;
	int		wstat;


	uid = getuid();
	pw_p = getpwuid(uid);

	if (uid == 0) {

		/* Already root, just run the program! */

		if (interactive == B_FALSE) {

			if ((pp = popen(prgm, "r")) == NULL) {
				return (-1);
			}

			(void) read(fileno(pp), output_buf, len);

			s = pclose(pp);
		} else {
			s = system(prgm);
		}

		return (s);

	}

	if (in_admin_group_p(uid) == B_FALSE) {

		(void) sysinfo(SI_HOSTNAME, h_name, sizeof (h_name));

		(void) sprintf(sec_violation_str, sec_violation_fmt,
		    h_name, getuid(),
		    pw_p ? pw_p->pw_name : "unknown",
		    admin_grp);

		(void) strncpy(output_buf, sec_violation_str, len - 1);
		output_buf[len - 1] = '\0';

		return (SYSMAN_SECURITY_ERR);
	}

	/*
	 * Fork, and try to setuid(0) in the child.  This will
	 * retain the REAL uid in the parent process.  If the
	 * child successfully changes to uid 0, it will do the
	 * root work, write the results down a pipe to the parent,
	 * and exit with the status of the program that it ran.
	 *
	 * If "interactive" is set, this means that the program
	 * that is being run is interactive (such as the package
	 * commands).  Don't bother with the pipe between parent
	 * and child, just let the child work with the parent's
	 * stdio.
	 *
	 * The setuid(0) will only succeed in the case where the
	 * effective uid is 0.  This is best accomplished by
	 * installing the program calling this function as owned
	 * by root with the suid bit set.
	 */

	if (interactive == B_FALSE) {
		(void) pipe(pipe_fds);
	}

	if ((pid = fork()) == 0) {

		if (interactive == B_FALSE) {

			/* child -- close the read end of the pipe */

			(void) close(pipe_fds[0]);

			if ((s = setuid(0)) != 0) {
				(void) write(pipe_fds[1], strerror(errno),
				    strlen(strerror(errno)) + 1);
				_exit(s);
			}

			if ((pp = popen(prgm, "r")) == NULL) {

				(void) sprintf(popen_fail_str,
				    popen_fail_fmt, prgm);

				(void) write(pipe_fds[1], popen_fail_str,
					     strlen(popen_fail_str) + 1);

				_exit(-1);
			}

			n = read(fileno(pp), output_buf, len);
			(void) write(pipe_fds[1], output_buf, n);
			/* write a NULL */
			(void) write(pipe_fds[1], "", 1);

			s = pclose(pp);
		} else {
			s = system(prgm);
		}

		_exit(s);

	} else if (pid == -1) {

		/* fork failed */

		(void) strncpy(output_buf, strerror(errno), sizeof (len) - 1);
		return (-1);

	} else {

		if (interactive == B_FALSE) {

			/* parent -- close the write end of the pipe */

			(void) close(pipe_fds[1]);

			(void) read(pipe_fds[0], output_buf, len);
			output_buf[len - 1] = '\0';

			/* If last character is a newline, kill it */
			if (output_buf[s = strlen(output_buf) - 1] == '\n') {
				output_buf[s] = '\0';
			}
		}

		(void) wait(&wstat);

		if (WIFEXITED(wstat) != 0) {
			/* normal termination of child process */
			return (WEXITSTATUS(wstat));
		} else {
			return (0);
		}
	}

	return (-1);
}


int
call_function_as_admin(
	int	(*fn)(void *, char *, int),
	void	*arg_struct_p,
	int	arg_struct_len,
	char	*output_buf,
	int	len)
{

	int		fd;
	caddr_t		arg_region;
	caddr_t		buf_region;
	uid_t		uid;
	struct passwd	*pw_p;
	char		h_name[257];
	char		sec_violation_str[1024];
	int		s;
	pid_t		pid;
	char		signed_status;
	int		wstat;


	uid = getuid();
	pw_p = getpwuid(uid);

	if (uid == 0) {

		/* Already root, just call the function! */

		return ((*fn)(arg_struct_p, output_buf, len));
	}

	if (in_admin_group_p(uid) == B_FALSE) {

		if (output_buf != NULL) {

			(void) sysinfo(SI_HOSTNAME, h_name, sizeof (h_name));

			(void) sprintf(sec_violation_str, sec_violation_fmt,
			    h_name, getuid(),
			    pw_p ? pw_p->pw_name : "unknown",
			    admin_grp);

			(void) strncpy(output_buf, sec_violation_str, len - 1);
			output_buf[len - 1] = '\0';
		}

		return (SYSMAN_SECURITY_ERR);
	}


	/*
	 * Fork, and try to setuid(0) in the child.  This will
	 * retain the REAL uid in the parent process.  If the
	 * child successfully changes to uid 0, it will do the
	 * root work and exit with the value that was returned
	 * by the function.
	 *
	 * The setuid(0) will only succeed in the case where the
	 * effective uid is 0.  This is best accomplished by
	 * installing the program calling this function as owned
	 * by root with the suid bit set.
	 *
	 * The child passes data back to the parent by calling the
	 * function with a shared mmap region.
	 */

	if ((fd = open("/dev/zero", O_RDWR)) < 0) {
		return (-1);
	}

	if ((arg_region = mmap(0, arg_struct_len, PROT_READ | PROT_WRITE,
	    MAP_SHARED, fd, 0)) == (caddr_t)-1) {
		close(fd);
		return (-1);
	}

	if ((buf_region = mmap(0, len, PROT_READ | PROT_WRITE,
	    MAP_SHARED, fd, 0)) == (caddr_t)-1) {
		munmap(arg_region, arg_struct_len);
		close(fd);
		return (-1);
	}

	close(fd);

	/* Copy in the passed-in arg struct. */
	memcpy((void *)arg_region, (const void *)arg_struct_p, arg_struct_len);

	if ((pid = fork()) == 0) {

		/* child */

		if ((s = setuid(0)) != 0) {
			strncpy(buf_region, strerror(errno), len - 1);
			buf_region[len - 1] = '\0';
			_exit(s);
		}

		s = (*fn)((void *)arg_region, (char *)buf_region, len);

		_exit(s);

	} else if (pid == -1) {

		/* fork failed */

		return (-1);

	} else {

		/* parent */

		(void) wait(&wstat);

		/* copy back return data from shared memory regions. */

		memcpy((void *)arg_struct_p,
		    (const void *)arg_region, arg_struct_len);

		/* Don't overwrite default error message with blank one */
		if (buf_region[0] != 0) {
			memcpy((void *)output_buf,
			    (const void *)buf_region, len);
		}

		munmap(arg_region, arg_struct_len);
		munmap(buf_region, len);

		if (WIFEXITED(wstat) != 0) {
			/*
			 * Normal termination of child process.  Since
			 * WEXITSTATUS turns the wstat into an 8-bit
			 * unsigned int, we assign it to a signed char
			 * to get a signed 8-bit return code.  We want
			 * this since we're simulating a function call
			 * with the child process, and we want the
			 * function call to be able to return a negative
			 * number to indicate failure.  This limits the
			 * range of return values of functions called
			 * by call_function_as_admin() to [-128,127]
			 */
			signed_status = WEXITSTATUS(wstat);
			return ((int)signed_status);
		} else {
			return (0);
		}
	}

	return (-1);
}
