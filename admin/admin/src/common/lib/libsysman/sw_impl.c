/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)sw_impl.c	1.18	96/01/11 SMI"


#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sysman_impl.h"


#define	SPACE		1
#define	CMD_BUF_LEN	1024


/*
 * sysman_gui defines whether pkg command interaction should spawn an xterm
 * and run in the xterm, otherwise it'll just run in the shell that invoked
 * the sys mgr command.  Default is false, run in the shell; set_sysman_gui()
 * to change it.
 */

static boolean_t	sysman_gui = B_FALSE;
static const char	*sysman_display_string = NULL;

static const char	*cmdtool_cmd_fmt =
	"/usr/openwin/bin/cmdtool -display %s "
	"-title \"Admintool: %s\" "
	"-icon_label \"Admintool: %s\" "
	"-name \"Admintool: %s\"";

static const char	*script_dir = "/var/tmp";
static const char	*script_file_pfx = "adpkg";
static const char	*script_header = "#!/bin/sh\n\n";
static const char	*status_cmd_fmt = "\necho $? >> %s\n";
static const char	*script_trailer =
	"\n/bin/echo \"press <Return> to continue \\c\"\nread foo\n";

static const char	*pkgadd_cmd = "/usr/sbin/pkgadd";
static const char	*pkgrm_cmd = "/usr/sbin/pkgrm";

static void	(*hup_disp)(int);
static void	(*int_disp)(int);
static void	(*quit_disp)(int);
static void	(*ill_disp)(int);
static void	(*trap_disp)(int);
static void	(*abrt_disp)(int);
static void	(*emt_disp)(int);
static void	(*fpe_disp)(int);
static void	(*kill_disp)(int);
static void	(*bus_disp)(int);
static void	(*segv_disp)(int);
static void	(*sys_disp)(int);
static void	(*pipe_disp)(int);
static void	(*alrm_disp)(int);
static void	(*term_disp)(int);
static void	(*usr1_disp)(int);
static void	(*usr2_disp)(int);
static void	(*poll_disp)(int);
static void	(*vtalrm_disp)(int);
static void	(*prof_disp)(int);
static void	(*xcpu_disp)(int);
static void	(*xfsz_disp)(int);


static
void
cleanup_handler(int sig)
{

	int		n;
	DIR		*dirp;
	struct dirent	*direntp;
	char		fname[PATH_MAX];


	if ((dirp = opendir(script_dir)) == NULL) {
		return;
	}

	n = strlen(script_file_pfx) - 1;

	while ((direntp = readdir(dirp)) != NULL) {
		if (strncmp(direntp->d_name, script_file_pfx, n) == 0) {
			sprintf(fname, "%s/%s", script_dir, direntp->d_name);
			(void) unlink(fname);
		}
	}
}


static
void
register_cleanup_handler(void)
{
	hup_disp = signal(SIGHUP, cleanup_handler);
	int_disp = signal(SIGINT, cleanup_handler);
	quit_disp = signal(SIGQUIT, cleanup_handler);
	ill_disp = signal(SIGILL, cleanup_handler);
	trap_disp = signal(SIGTRAP, cleanup_handler);
	abrt_disp = signal(SIGABRT, cleanup_handler);
	emt_disp = signal(SIGEMT, cleanup_handler);
	fpe_disp = signal(SIGFPE, cleanup_handler);
	kill_disp = signal(SIGKILL, cleanup_handler);
	bus_disp = signal(SIGBUS, cleanup_handler);
	segv_disp = signal(SIGSEGV, cleanup_handler);
	sys_disp = signal(SIGSYS, cleanup_handler);
	pipe_disp = signal(SIGPIPE, cleanup_handler);
	alrm_disp = signal(SIGALRM, cleanup_handler);
	term_disp = signal(SIGTERM, cleanup_handler);
	usr1_disp = signal(SIGUSR1, cleanup_handler);
	usr2_disp = signal(SIGUSR2, cleanup_handler);
	poll_disp = signal(SIGPOLL, cleanup_handler);
	vtalrm_disp = signal(SIGVTALRM, cleanup_handler);
	prof_disp = signal(SIGPROF, cleanup_handler);
	xcpu_disp = signal(SIGXCPU, cleanup_handler);
	xfsz_disp = signal(SIGXFSZ, cleanup_handler);
}

static
void
unregister_cleanup_handler(void)
{
	if (hup_disp != SIG_ERR) {
		(void) signal(SIGHUP, hup_disp);
	}
	if (int_disp != SIG_ERR) {
		(void) signal(SIGINT, int_disp);
	}
	if (quit_disp != SIG_ERR) {
		(void) signal(SIGQUIT, quit_disp);
	}
	if (ill_disp != SIG_ERR) {
		(void) signal(SIGILL, ill_disp);
	}
	if (trap_disp != SIG_ERR) {
		(void) signal(SIGTRAP, trap_disp);
	}
	if (abrt_disp != SIG_ERR) {
		(void) signal(SIGABRT, abrt_disp);
	}
	if (emt_disp != SIG_ERR) {
		(void) signal(SIGEMT, emt_disp);
	}
	if (fpe_disp != SIG_ERR) {
		(void) signal(SIGFPE, fpe_disp);
	}
	if (kill_disp != SIG_ERR) {
		(void) signal(SIGKILL, kill_disp);
	}
	if (bus_disp != SIG_ERR) {
		(void) signal(SIGBUS, bus_disp);
	}
	if (segv_disp != SIG_ERR) {
		(void) signal(SIGSEGV, segv_disp);
	}
	if (sys_disp != SIG_ERR) {
		(void) signal(SIGSYS, sys_disp);
	}
	if (pipe_disp != SIG_ERR) {
		(void) signal(SIGPIPE, pipe_disp);
	}
	if (alrm_disp != SIG_ERR) {
		(void) signal(SIGALRM, alrm_disp);
	}
	if (term_disp != SIG_ERR) {
		(void) signal(SIGTERM, term_disp);
	}
	if (usr1_disp != SIG_ERR) {
		(void) signal(SIGUSR1, usr1_disp);
	}
	if (usr2_disp != SIG_ERR) {
		(void) signal(SIGUSR2, usr2_disp);
	}
	if (poll_disp != SIG_ERR) {
		(void) signal(SIGPOLL, poll_disp);
	}
	if (vtalrm_disp != SIG_ERR) {
		(void) signal(SIGVTALRM, vtalrm_disp);
	}
	if (prof_disp != SIG_ERR) {
		(void) signal(SIGPROF, prof_disp);
	}
	if (xcpu_disp != SIG_ERR) {
		(void) signal(SIGXCPU, xcpu_disp);
	}
	if (xfsz_disp != SIG_ERR) {
		(void) signal(SIGXFSZ, xfsz_disp);
	}
}


static
char *
make_status_filename(const char *script_filename)
{

	char	*status_filename;


	if (script_filename == NULL) {
		return (NULL);
	}

	status_filename = (char *)malloc(strlen(script_filename) + 8);

	if (status_filename == NULL) {
		return (NULL);
	}

	sprintf(status_filename, "%s.status", script_filename);

	return (status_filename);
}


static
int
get_pkgcmd_status(const char *status_filename)
{

	FILE	*fp;
	int	stat;
	char	buf[32];


	if (status_filename == NULL) {
		return (-1);
	}

	if ((fp = fopen(status_filename, "r")) == NULL) {
		return (-1);
	}

	/* check for end-of-file */
	if (fgetc(fp) == EOF) {
		fclose(fp);
		return (-1);
	}
	rewind(fp);

	/* read the status int(s) out of the file */
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		/* remove the newline */
		buf[strlen(buf) - 1] = '\0';

		stat = atoi((const char*)buf);
		if (stat == 0)
			break;
	}

	fclose(fp);

	return (stat);
}


static
int
append_cmd_flag(char *buf, int buflen, const char *flag)
{

	int	new_len;


	if (buf == NULL || flag == NULL) {
		return (-1);
	}

	/*
	 * Check to make sure that the new command will fit in the buffer
	 * and leave room for a terminating NULL.  The calculation for the
	 * new command length is the sum of the lengths of the current command
	 * and the flag, plus a space.
	 */

	if ((new_len = strlen(buf) + SPACE + strlen(flag)) >= buflen) {

		return (-1);
	}

	(void) strcat(buf, " ");
	(void) strcat(buf, flag);

	return (new_len);
}


static
int
append_cmd_arg(char *buf, int buflen, const char *flag, const char *value)
{

	int	new_len;


	if (buf == NULL || flag == NULL || value == NULL) {
		return (-1);
	}

	/*
	 * Check to make sure that the new command will fit in the buffer
	 * and leave room for a terminating NULL.  The calculation for the
	 * new command length is the sum of the lengths of the current command,
	 * the flag, and the value, plus spaces.
	 */

	if ((new_len = strlen(buf) + SPACE + strlen(flag) + SPACE +
	    strlen(value)) >= buflen) {

		return (-1);
	}

	(void) strcat(buf, " ");
	(void) strcat(buf, flag);
	(void) strcat(buf, " ");
	(void) strcat(buf, value);

	return (new_len);
}


static char *
begin_pkg_script(int* fd)
{
	int	script_len = 0;
	char	*filename;

	if ((filename = tempnam(script_dir, script_file_pfx)) == NULL) {
		return (NULL);
	}

	script_len = strlen(script_header);

	if ((*fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0700)) == -1) {
		return (NULL);
	}

	if (write(*fd, script_header, script_len) != script_len) {
		free((void *)filename);
		close(fd);
		return (NULL);
	}

	return (filename);
}

static int
add_pkg_script_cmd(int fd, const char *pkg_cmd, char *script)
{
	int	script_len = 0;
	char	*script_buf;
	char	*status_filename;
	char	status_cmd[CMD_BUF_LEN];

	status_filename = make_status_filename((const char *)script);
	if (status_filename == NULL) {
		return (0);
	}

	sprintf(status_cmd, status_cmd_fmt, status_filename);

	free((void *)status_filename);

	script_len += strlen(pkg_cmd);
	script_len += strlen(status_cmd);

	if ((script_buf = (char*)malloc((unsigned)(script_len + 1))) == NULL) {
		return (0);
	}

	script_buf[0] = '\0';
	strcat(script_buf, pkg_cmd);
	strcat(script_buf, status_cmd);

	if (write(fd, script_buf, script_len) != script_len) {
		close(fd);
		return (-1);
	}

	free((void *)script_buf);
}

static int
end_pkg_script(int fd)
{
	int	script_len = 0;

	script_len += strlen(script_trailer);

	if (write(fd, script_trailer, script_len) != script_len) {
		close(fd);
		return (-1);
	}

	close(fd);
}

static
char *
build_pkg_script(const char *pkg_cmd)
{

	int	script_len = 0;
	char	*script_buf;
	char	*filename;
	char	*status_filename;
	char	status_cmd[CMD_BUF_LEN];
	int	fd;


	if ((filename = tempnam(script_dir, script_file_pfx)) == NULL) {
		return (NULL);
	}

	status_filename = make_status_filename((const char *)filename);
	if (status_filename == NULL) {
		return (NULL);
	}

	sprintf(status_cmd, status_cmd_fmt, status_filename);

	free((void *)status_filename);

	script_len += strlen(script_header);
	script_len += strlen(pkg_cmd);
	script_len += strlen(status_cmd);
	script_len += strlen(script_trailer);

	if ((script_buf = (char *)malloc((unsigned)(script_len + 1))) == NULL) {
		free((void *)filename);
		return (NULL);
	}

	script_buf[0] = '\0';

	strcat(script_buf, script_header);
	strcat(script_buf, pkg_cmd);
	strcat(script_buf, status_cmd);
	strcat(script_buf, script_trailer);

	if ((fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0700)) == -1) {
		free((void *)script_buf);
		return (NULL);
	}

	if (write(fd, script_buf, script_len) != script_len) {
		free((void *)filename);
		free((void *)script_buf);
		close(fd);
		return (NULL);
	}

	free((void *)script_buf);
	close(fd);

	return (filename);
}


static
char *
build_cmdtool_cmd(const char *cmdtool_title, const char *script_file)
{

	char	cmdtool_cmd[256];
	int	cmd_len = 0;
	char	*cmd_buf;


	sprintf(cmdtool_cmd, cmdtool_cmd_fmt, sysman_display_string,
	    cmdtool_title, cmdtool_title, cmdtool_title);

	/* command is "cmdtool_cmd script_file" */
	cmd_len += strlen(cmdtool_cmd);
	cmd_len += 1;			/* space */
	cmd_len += strlen(script_file);

	if ((cmd_buf = (char *)malloc((unsigned)(cmd_len + 1))) == NULL) {
		return (NULL);
	}

	cmd_buf[0] = '\0';

	strcat(cmd_buf, cmdtool_cmd);
	strcat(cmd_buf, " ");
	strcat(cmd_buf, script_file);

	return (cmd_buf);
}


void
_sw_set_gui(boolean_t do_gui, const char *display_string)
{
	sysman_gui = do_gui;
	if (sysman_display_string != NULL) {
		free((void *)sysman_display_string);
	}
	sysman_display_string = strdup(display_string);
}


char*
_start_batch_cmd(int *fd)
{
	return begin_pkg_script(fd);
}

int
_add_batch_cmd(int fd, void *arg_p, char* script)
{
	int		i;
	char		cmd_buf[CMD_BUF_LEN];
	SysmanSWArg	*swa_p = (SysmanSWArg *)arg_p;


	if (swa_p == NULL) {
		return (-1);
	}

	(void) strcpy(cmd_buf, pkgadd_cmd);

	if (swa_p->spool != NULL) {

		/* Moving pkg to a spool, only -s and -d make sense */

		append_cmd_arg(cmd_buf, CMD_BUF_LEN, "-s", swa_p->spool);

		if (swa_p->device != NULL) {
			append_cmd_arg(cmd_buf, CMD_BUF_LEN,
			    "-d", swa_p->device);
		}
	} else {

		/* Installing packages, check all args */

		if (swa_p->non_interactive != 0) {
			append_cmd_flag(cmd_buf, CMD_BUF_LEN, "-n");
		}

		if (swa_p->show_copyrights == 0) {
			append_cmd_flag(cmd_buf, CMD_BUF_LEN, "-S");
		}

		if (swa_p->admin != NULL) {
			append_cmd_arg(cmd_buf, CMD_BUF_LEN,
			    "-a", swa_p->admin);
		}

		if (swa_p->device != NULL) {
			append_cmd_arg(cmd_buf, CMD_BUF_LEN,
			    "-d", swa_p->device);
		}

		if (swa_p->root_dir != NULL) {
			append_cmd_arg(cmd_buf, CMD_BUF_LEN,
			    "-R", swa_p->root_dir);
		}

		if (swa_p->response != NULL) {
			append_cmd_arg(cmd_buf, CMD_BUF_LEN,
			    "-r", swa_p->response);
		}
	}

	/* Now append the package names */

	for (i = 0; i < swa_p->num_pkgs; i++) {
		append_cmd_flag(cmd_buf, CMD_BUF_LEN, swa_p->pkg_names[i]);
	}

	return add_pkg_script_cmd(fd, cmd_buf, script);
}

int
_finish_batch_cmd(int fd)
{
	return end_pkg_script(fd);
}

int
_root_add_sw_by_script(void *arg_p, char *buf, int len)
{

	int		i;
	char		*status_file;
	char		*cmdtool_cmd;
	int		status;
	char		*script_file = (char *)arg_p;


	if ((script_file == NULL) ||
	    (sysman_gui != B_TRUE)) {
		return (-1);
	}

	register_cleanup_handler();

	if ((cmdtool_cmd =
	    build_cmdtool_cmd("Add Software", script_file)) == NULL) {
		return (-1);
	}

	/* not interested in exit status of the xterm, cast to (void) */
	(void) run_program_as_admin(cmdtool_cmd, B_FALSE, buf, len);

	if ((status_file = make_status_filename(script_file)) != NULL) {
		status = get_pkgcmd_status(status_file);
	} else {
		status = -1;
	}

	(void) unlink(script_file);
	(void) unlink(status_file);

	free((void *)cmdtool_cmd);

	unregister_cleanup_handler();

	return (status);
}


int
_root_add_sw(void *arg_p, char *buf, int len)
{

	int		i;
	int		status;
	char		cmd_buf[CMD_BUF_LEN];
	SysmanSWArg	*swa_p = (SysmanSWArg *)arg_p;


	if (swa_p == NULL) {
		return (-1);
	}

	(void) strcpy(cmd_buf, pkgadd_cmd);

	if (swa_p->spool != NULL) {

		/* Moving pkg to a spool, only -s and -d make sense */

		append_cmd_arg(cmd_buf, CMD_BUF_LEN, "-s", swa_p->spool);

		if (swa_p->device != NULL) {
			append_cmd_arg(cmd_buf, CMD_BUF_LEN,
			    "-d", swa_p->device);
		}
	} else {

		/* Installing packages, check all args */

		if (swa_p->non_interactive != 0) {
			append_cmd_flag(cmd_buf, CMD_BUF_LEN, "-n");
		}

		if (swa_p->show_copyrights == 0) {
			append_cmd_flag(cmd_buf, CMD_BUF_LEN, "-S");
		}

		if (swa_p->admin != NULL) {
			append_cmd_arg(cmd_buf, CMD_BUF_LEN,
			    "-a", swa_p->admin);
		}

		if (swa_p->device != NULL) {
			append_cmd_arg(cmd_buf, CMD_BUF_LEN,
			    "-d", swa_p->device);
		}

		if (swa_p->root_dir != NULL) {
			append_cmd_arg(cmd_buf, CMD_BUF_LEN,
			    "-R", swa_p->root_dir);
		}

		if (swa_p->response != NULL) {
			append_cmd_arg(cmd_buf, CMD_BUF_LEN,
			    "-r", swa_p->response);
		}
	}

	/* Now append the package names */

	for (i = 0; i < swa_p->num_pkgs; i++) {
		append_cmd_flag(cmd_buf, CMD_BUF_LEN, swa_p->pkg_names[i]);
	}

	register_cleanup_handler();

	if (sysman_gui == B_TRUE) {

		char		*script_file;
		char		*status_file;
		char		*cmdtool_cmd;

		if ((script_file = build_pkg_script(cmd_buf)) == NULL) {
			return (-1);
		}
		if ((cmdtool_cmd =
		    build_cmdtool_cmd("Add Software", script_file)) == NULL) {
			return (-1);
		}

		/* not interested in exit status of the xterm, cast to (void) */
		(void) run_program_as_admin(cmdtool_cmd, B_FALSE, buf, len);

		if ((status_file = make_status_filename(script_file)) != NULL) {
			status = get_pkgcmd_status(status_file);
		} else {
			status = -1;
		}

		(void) unlink(script_file);
		(void) unlink(status_file);

		free((void *)script_file);
		free((void *)cmdtool_cmd);
	} else {
		status = run_program_as_admin(cmd_buf, B_TRUE, buf, len);
	}

	unregister_cleanup_handler();

	return (status);
}


int
_root_delete_sw(void *arg_p, char *buf, int len)
{

	int		i;
	int		status;
	char		cmd_buf[CMD_BUF_LEN];
	SysmanSWArg	*swa_p = (SysmanSWArg *)arg_p;


	if (swa_p == NULL) {
		return (-1);
	}

	(void) strcpy(cmd_buf, pkgrm_cmd);

	if (swa_p->spool != NULL) {

		/* Removing from a spool, only -s makes sense */

		append_cmd_arg(cmd_buf, CMD_BUF_LEN, "-s", swa_p->spool);
	} else {

		/* Removing installed packages, check all args */

		if (swa_p->non_interactive != 0) {
			append_cmd_flag(cmd_buf, CMD_BUF_LEN, "-n");
		}

		if (swa_p->admin != NULL) {
			append_cmd_arg(cmd_buf, CMD_BUF_LEN,
			    "-a", swa_p->admin);
		}

		if (swa_p->root_dir != NULL) {
			append_cmd_arg(cmd_buf, CMD_BUF_LEN,
			    "-R", swa_p->root_dir);
		}
	}

	/* Now append the package names */

	for (i = 0; i < swa_p->num_pkgs; i++) {
		append_cmd_flag(cmd_buf, CMD_BUF_LEN, swa_p->pkg_names[i]);
	}

	register_cleanup_handler();

	if (sysman_gui == B_TRUE) {

		char		*script_file;
		char		*status_file;
		char		*cmdtool_cmd;

		if ((script_file = build_pkg_script(cmd_buf)) == NULL) {
			return (-1);
		}
		if ((cmdtool_cmd =
		    build_cmdtool_cmd("Delete Software", script_file)) ==
		    NULL) {
			return (-1);
		}
		/* not interested in exit status of the xterm, cast to (void) */
		(void) run_program_as_admin(cmdtool_cmd, B_FALSE, buf, len);

		if ((status_file = make_status_filename(script_file)) != NULL) {
			status = get_pkgcmd_status(status_file);
		} else {
			status = -1;
		}

		(void) unlink(script_file);
		(void) unlink(status_file);

		free((void *)script_file);
		free((void *)cmdtool_cmd);
	} else {
		status = run_program_as_admin(cmd_buf, B_TRUE, buf, len);
	}

	unregister_cleanup_handler();

	return (status);
}
