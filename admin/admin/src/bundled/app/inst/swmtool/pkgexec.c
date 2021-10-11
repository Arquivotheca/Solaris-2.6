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
#ident	"@(#)pkgexec.c 1.22 94/10/13"
#endif

#include "defs.h"
#include "ui.h"
#include "host.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/filio.h>
#include <sys/param.h>
#include <sys/wait.h>
#ifdef SVR4
#include <sys/statvfs.h>
#else
#include <sys/vfs.h>
#endif

#define	PKGADD_CMD	"/usr/sbin/pkgadd"
#define	PKGRM_CMD	"/usr/sbin/pkgrm"
#define	PKGSPOOL_CMD	"/bin/pkgtrans -o"

extern	pid_t		parent;

static Hostlist	*running_host;
static pid_t	running_pid;

static int	dospool;	/* are we spooling this package? */
static int	interactive;
static char	*spooldir;	/* where we're spooling package */
static char	*sharedir;	/* spooldir or pkgloc, for remote installs */

static void onusr2(int sig);
static int pkg_dispatch(Module *);
static int walk_instlist(Node *, caddr_t);
static int install_4x(Node *, caddr_t);
static int pkg_run_cmds(Modinfo *, char *);
static void pkg_done(void);
static int pkg_spool(Modinfo *, char *, char *);
static int pkg_despool(Modinfo *, char *);
static pid_t pkg_host(Hostlist *, Modinfo *, char *, int, SWM_mode);
static void pkg_local(Modinfo *, char *, SWM_mode);
static void pkg_remote(Hostlist *, Modinfo *, char *, SWM_mode);
static void pkg_diskless(Hostlist *, Modinfo *, char *, int, SWM_mode);
static int pkg_make_admin(void);
static int pkg_update_admin(Modinfo *);
static void pkg_errs(Modinfo *, SWM_mode);
static int count_ops(Modinfo *, SWM_mode, int *, int *);

static void
onusr2(int sig)
{
	(void) signal(sig, SIG_DFL);
}

/*
 * For XView programs, these routines must be run
 * in a wrapper process since they call routines
 * based on system(3).
 */
pid_t
pkg_exec(Module *media)
{
	int	status;

	interactive = get_interactive();
	spooldir = get_spooldir();
	/*
	 * Create an admin file in /tmp.
	 * We'll update it before we
	 * start each package installation.
	 */
	if (pkg_make_admin() != SUCCESS)
		return (SWM_ERR_MKADMIN);

	status = pkg_dispatch(media);
	pkg_done();

	exit (status);
#ifdef lint
	return ((pid_t)0);
#endif
}

/*
 * Install or remove all selected package on the selected hosts:
 *	We call this routine once and it is repsonsible for
 *	finding each package to be installed or removed.
 *
 * Loop through all available products.  For each product,
 *	perform the following:
 *
 * 1) Order (by dependency) all selected packages.
 *
 * 2) Determine the location of the product's packages.
 *
 * 3) Walk the list of packages, calling the installation
 *	or removal function, as appropriate.
 *
 */
static int
pkg_dispatch(Module *media)
{
	Module	*prod;
	SWM_mode mode = get_mode();
	char	pkgloc[MAXPATHLEN]; /* where source packages are located */

	set_current(media);
	for (prod = media->sub; prod; prod = get_next(prod)) {
		if (prod->type == PRODUCT)
			set_current(prod);
		if (mode == MODE_INSTALL) {
			/*
			 * Set pkgloc to where source
			 * packages are located.
			 */
			if (prod->type != NULLPRODUCT &&
			    prod->info.prod->p_name)
				(void) strcpy(pkgloc,
					prod->info.prod->p_pkgdir);
			else
				(void) strcpy(pkgloc,
					media->info.media->med_dir);
			(void) walklist(prod->info.prod->p_sw_4x,
				install_4x, (caddr_t)pkgloc);
		}
		(void) walklist(prod->info.prod->p_packages,
				walk_instlist, (caddr_t)pkgloc);
	}
	/*
	 * Handshake protocol with parent:
	 *	Send parent SIGUSR2:  reap all children (namely, me)
	 *	Wait for SIGUSR2:  I'm ready to reap you
	 *	Continue processing (will exit soon)
	 */
	(void) signal(SIGUSR2, onusr2);
	(void) sighold(SIGUSR2);
	(void) kill(parent, SIGUSR2);
	(void) sigpause(SIGUSR2);

	return (SUCCESS);
}

/*
 * For all instances, we may need to install the package
 * on some remote system.
 */
static int
walk_instlist(Node *node, caddr_t data)
{
	Modinfo *info = (Modinfo *)node->data;
	char	*pkgloc = (char *)data;
	SWM_mode mode = get_mode();
	Modinfo	*inst, *patch;
	char	*errstr;
	int	local;		/* number of local pkgadd/pkgrm operations */
	int	remote;		/* number of remote pkgadd/pkgrm ops */
	int	pkgops;		/* total number of pkgadd/pkgrm's */
	char	*pkgsrc = (char *)0;
	int	err;

	/*
	 * The first instance of a package corresponds
	 * to the native environment.  We key remote
	 * installations and removals from this instance,
	 * in addition to local operations.
	 */
	if (info->m_status != SELECTED && info->m_action != TO_BE_REMOVED)
		return (SUCCESS);

	for (inst = info; inst != (Modinfo *)0; inst = next_inst(inst)) {
		/*
		 * We have no provision for selecting patches
		 * for installation (yet), so break the [patch]
		 * loop after installing the package instance.
		 */
		for (patch = inst;
		    patch != (Modinfo *)0 &&
		    (mode == MODE_REMOVE || patch == inst);	/* break loop */
		    patch = next_patch(patch)) {
			pkgops = count_ops(patch, mode, &local, &remote);
			if (pkgops == 0)
				continue;
			dospool = 0;
			/*
			 * Determine if we need to set up
			 * a spool directory for this package.
			 * (if spooling) and update admin file
			 * with the package's base directory.
			 */
			if (mode == MODE_INSTALL && remote > 0)
				dospool = 1;

			if (pkg_update_admin(patch) != SUCCESS) {
				(void) fprintf(stderr, gettext(
	"\nInstallation of <%s> package instance on host <%s> failed\n"
	"because the admin file (%s) that controls the\n"
	"installation could not be updated:  %s.\n"),
					patch->m_pkg_dir, thishost,
					admin_file((char *)0),
					strerror(errno));
				return (SWM_ERR_MKADMIN);
			}

			if (dospool &&
			    pkg_spool(patch, pkgloc, spooldir) != SUCCESS) {
				err = errno;
				/*
				 * If the package directory is local, we
				 * can fall back to installing from the
				 * the source media instead of a spooled
				 * copy.  This enables us to do remote
				 * installs of packages too large to spool.
				 */
				if (path_is_local(pkgloc) != SUCCESS) {
					if (err == ENOSPC) {
						errstr = gettext(
	"\nInstallation of package instance <%s> cannot be attempted\n"
	"because the package is not located on source media local to this\n"
	"system (%s) and there is not enough space in the spool\n"
	"directory (%s) to spool the package.  To remedy this\n"
	"situation, you can do one of the following:\n\n"
	"\t- Make more space available on the file system containing\n"
	"\t  the spool directory (%s).\n"
	"\t- Change the \"spooldir\" entry in %s's configuration\n"
	"\t  file to directory on a file system with more space.\n"
	"\t- Make the spool directory (%s) a symbolic link to a\n"
	"\t  directory on another file system.\n\n"
	"Without corrective action, installation of other packages will\n"
	"proceed but may fail for this same reason.\n");
						(void) fprintf(stderr, errstr,
						    patch->m_pkg_dir,
						    thishost,
						    spooldir,
						    spooldir,
						    progname,
						    spooldir);
					} else {
						errstr = gettext(
	"\nInstallation of package instance <%s> cannot be attempted\n"
	"because the package is not located on source media local to this\n"
	"system (%s) and the package could not be spooled.  Any\n"
	"remaining package installations will be attempted but may fail for\n"
	"this same reason.\n");
						(void) fprintf(stderr, errstr,
						    patch->m_pkg_dir,
						    thishost);
					}
					return (SWM_ERR_SPOOL);
				} else
					dospool = 0;	/* fall-back */
			}

			if (dospool)
				pkgsrc = spooldir;
			else if (remote > 0)
				pkgsrc = pkgloc;
#ifndef DEMO
			/*
			 * This next step can be inefficient, but
			 * the file sharing code isn't really set
			 * up to handle exporting and remote-mounting
			 * more than one file system at a time.  So
			 * each time we need a new package dir, we
			 * must unmount and unshare the previous one.
			 */
			if (mode == MODE_INSTALL && remote > 0) {
				if (sharedir != (char *)0 &&
				    strcmp(pkgsrc, sharedir) != 0) {
					(void) rumount_fs(sharedir);
					(void) unshare_fs(sharedir);
					free(sharedir);
				}
				sharedir = xstrdup(pkgsrc);
				if (share_fs(pkgsrc) < 0)
					return (SWM_ERR_SHARE);
				(void) rmount_fs(pkgsrc);
			}
#endif
			(void) pkg_run_cmds(patch, pkgsrc ? pkgsrc : pkgloc);

			if (dospool)
				(void) pkg_despool(patch, spooldir);
		}
	}
	return (SUCCESS);
}

/*ARGSUSED1*/
static int
install_4x(Node *node, caddr_t data)
{
	Modinfo *info = (Modinfo *)node->data;
	char	cmd[BUFSIZ];
	pid_t	childpid;
	int	fd;

	if (info->m_status != SELECTED || !info->m_install)
		return (SUCCESS);

	(void) fprintf(stderr, gettext(
	    "Invoking installation script for <%s>\n"), info->m_name);

	if ((childpid = fork()) == -1)
		return (FAILURE);
	if (childpid != 0) {
		int	status = 0;
		/*
		 * Parent:  wait for child
		 */
		while (waitpid(childpid, &status, 0) < 0 && errno == EINTR)
			;
		(void) fprintf(stderr, gettext(
		    "Installation script for <%s> completed (status %d)\n"),
			info->m_name, WEXITSTATUS(status));
		return (SUCCESS);
	}
	/*
	 * Child:  enable signal handling
	 */
	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);

	(void) setbuf(stdout, (char *)0);
	/*
	 * Work around an interaction
	 * problem with "more"
	 */
	fd = open("/dev/tty", O_RDWR);
	if (fd != -1) {
		(void) dup2(fd, 0);
		(void) dup2(fd, 1);
		(void) dup2(fd, 2);
		(void) close(fd);
	}
	(void) sprintf(cmd, "%s %s",
	    info->m_install->f_path,
	    info->m_install->f_args ? info->m_install->f_args : "");
#ifdef DEMO
	(void) fprintf(stderr, "%s\n", cmd);
	exit(0);
#else
	(void) execl("/sbin/sh", "sh", "-c", cmd, (char *)0);
	exit(-1);	/* shouldn't happen */
#endif
	/*NOTREACHED*/
}

/*
 * Run a package command (pkgadd or pkgrm) on all selected hosts:
 *
 * Perform the following steps for each package:
 *
 * 1) Spool the package if necessary.
 *
 * 2) Update the pkgadd -a file with the package's base
 *	directory (if installing).
 *
 * 3) Run pkgadd or pkgrm on all selected hosts.
 *
 * 4) Remove the spooled copy of the package.
 */
static int
pkg_run_cmds(Modinfo *info, char *pkgloc)
{
	Hostlist *hlp;
	int	childstat;
	pid_t	childpid;
	SWM_mode mode = get_mode();
	int	total_errors;
	int	hosts_left;		/* counts remaining hosts */
	int	notdone;		/* number of hosts yet to complete */
	int	status;

	total_errors = 0;
	/*
	 * Count number of hosts on which package
	 * is to be installed or removed.
	 */
	hosts_left = 0;
	for (hlp = hostlist.h_next; hlp != &hostlist; hlp = hlp->h_next) {
		if (hlp->h_status & HOST_SELECTED)
			hosts_left++;
	}
	notdone = hosts_left;

	running_pid = -1;
	running_host = (Hostlist *)0;

	/*
	 * Loop until we successfully start an
	 * installation/removal on some host.
	 */
	for (hlp = hostlist.h_next;
	    hlp != &hostlist && running_pid == -1 && hosts_left > 0;
	    hlp = hlp->h_next) {
		if ((hlp->h_status & HOST_SELECTED) == 0)
			continue;
		hlp->h_status &= ~HOST_ERROR;
		running_host = hlp;
		running_pid = pkg_host(hlp, info, pkgloc, dospool, mode);
		if (running_pid == -1)
			notdone--;	/* error, count [bad] host as done */
		hosts_left--;
	}

	while (notdone > 0) {
		/*
		 * Do a blocking wait on any
		 * child in our process group.
		 */
		while ((childpid = waitpid((pid_t)0, &childstat, 0)) < 0 &&
		    errno == EINTR)
			;
		if (childpid != -1) {
			if (running_pid == childpid) {
				notdone--;
				running_pid = -1;
			}
			status = WEXITSTATUS(childstat);
			if (status != 0 && status != 10 && status != 20) {
				running_host->h_status |= HOST_ERROR;
				total_errors++;
			}
		} else if (errno == ECHILD && running_pid != -1) {
			/*
			 * Potentailly lost the child's
			 * status -- ping process
			 */
			if (kill(running_pid, 0) < 0 && errno == ESRCH) {
				/* yup, we lost it */
				notdone--;
				running_pid = -1;
			}
		}
		while (hlp != &hostlist &&	/* hosts on list */
		    running_pid == -1 &&	/* failure to start host */
		    hosts_left > 0) {		/* selected hosts left */
			if (hlp->h_status & HOST_SELECTED) {
				hlp->h_status &= ~HOST_ERROR;
				running_host = hlp;
				running_pid = pkg_host(
					hlp, info, pkgloc, dospool, mode);
				if (running_pid == -1)
					notdone--;
				hosts_left--;
			}
			hlp = hlp->h_next;
		}
	}

	if (total_errors > 0)
		pkg_errs(info, mode);

	return (SUCCESS);
}

/*
 * Dismantle package set-up.
 *	We call this routine after each batch of
 *	package installations or removals.
 *
 *	TODO:  we should call this once on exit, but
 *	this requires that hosts cannot be removed
 *	(since we need their names and mount status).
 *
 * 1) Remove the admin file
 *
 * 2) Unmount the file system from all hosts that
 *	have mounted it.
 *
 * 3) Remove remote-mount privileges from the spool
 *	file system
 */
static void
pkg_done(void)
{
	(void) unlink(admin_file((char *)0));
#ifndef DEMO
	(void) rumount_fs(sharedir);
	(void) unshare_fs(sharedir);
#endif
	free(sharedir);
	sharedir = (char *)0;
}

/*
 * Spool a package to the indicated directory
 */
static int
pkg_spool(Modinfo *info, char *pkgloc, char *spooldir)
{
	struct statvfs vfs;
	char	cmd[MAXPATHLEN*3];
	u_long	free;
	daddr_t	used = info->m_spooled_size;

	/*
	 * Check spool directory for sufficient space -- if it
	 * doesn't have enough we abort.
	 */
	if (statvfs(spooldir, &vfs) == 0)
		/*
		 * get free space in Kbytes
		 */
		free = vfs.f_bavail * (vfs.f_frsize / 1024);
	else
		free = 0;

	if (used == 0) {
		(void) sprintf(cmd, "%s/%s", pkgloc, info->m_pkgid);
		used = (daddr_t)get_spooled_size(cmd);
		if (used < 0) {
			register int i;
			used = 0;
			for (i = 0; i < N_LOCAL_FS; i++)
				used += info->m_deflt_fs[i];
		}
	}

	if (used > (daddr_t)free) {
		errno = ENOSPC;
		return (FAILURE);
	} else {
		(void) sprintf(cmd, "%s %s %s %s",
			PKGSPOOL_CMD, pkgloc, spooldir, info->m_pkg_dir);
#ifdef DEMO
		(void) fprintf(stderr, "%s\n", cmd);
		return (SUCCESS);
#else
		return (system(cmd));
#endif
	}
}

/*
 * Remove spooled copy of package.  Removal from
 * the spool directory is always non-interactive.
 */
static int
pkg_despool(Modinfo *info, char *spooldir)
{
	char	cmd[MAXPATHLEN*2];

	(void) sprintf(cmd, "%s -n -s %s %s",
		PKGRM_CMD, spooldir, info->m_pkg_dir);

#ifdef DEMO
	(void) fprintf(stderr, "%s\n", cmd);
	return (SUCCESS);
#else
	return (system(cmd));
#endif
}

/*
 * pkg_host -- determine if package is to be installed
 * on argument host, fork child process, enable interrupt
 * and quit processing in child, and call package routine
 * appropriate for host type.  Once in child, this routine
 * never returns, as the host type-specific routines all
 * call exec/exit.
 */
static pid_t
pkg_host(Hostlist *hlp,
	Modinfo	*info,
	char	*pkgloc,
	int	spooled,	/* =1 if installing from spool directory */
	SWM_mode mode)
{
	pid_t	childpid;

#ifndef DEMO
	/*
	 * Skip host if software not accessible
	 */
	if (mode == MODE_INSTALL &&
	    (hlp->h_status & (HOST_MOUNTED | HOST_LOCAL)) == 0)
		return ((pid_t)-1);
#endif

	/*
	 * Skip if package is arch-specific and
	 * does not match arch of target host
	 * and we are installing.
	 *
	 * XXX This means you can't use this code
	 * to install for diskless clients and
	 * services
	 */
	if (mode == MODE_INSTALL &&
	    supports_arch(hlp->h_arch, info->m_arch) == FALSE &&
	    strcmp(info->m_arch, "all") != 0 &&
	    strcmp(info->m_arch, "all.all") != 0) {
#ifdef DEBUG
		(void) fprintf(stderr, gettext(
	"WARNING:  <%s> arch (%s) incompatible with arch of host `%s' (%s)\n"),
			info->m_pkgid, info->m_arch,
			hlp->h_name, hlp->h_arch);
#endif
		return ((pid_t)-1);
	}

	childpid = fork();
	if (childpid != 0)
		return (childpid);

	/*
	 * Now in child
	 *	Enable signal handling
	 */
	(void) setbuf(stdin, (char *)0);
	(void) setbuf(stdout, (char *)0);
	(void) setbuf(stderr, (char *)0);

	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);

	if (hlp->h_type == diskless)
		pkg_diskless(hlp, info, pkgloc, spooled, mode);
	else if (strcmp(hlp->h_name, thishost) == 0 && geteuid() == 0)
		pkg_local(info, pkgloc, mode);
	else
		/*
		 * If we're not running as root and doing a
		 * local install, we need to use the remote
		 * host code so we become root using the
		 * user-supplied root password.
		 */
		pkg_remote(hlp, info, pkgloc, mode);
	/* NOTREACHED */
}

/*
 * Host type specific installation and removal routines.  Sometimes we
 * need to use the package ID (m_pkgid), sometimes the instance name
 * (m_pkg_dir) as arguments to pkgadd and pkgrm.  Here are the rules:
 *
 * For all installations we use the instance name (dir name) because
 * we are either installing directly from the distribution media or
 * from a spool directory and m_pkg_dir is the name of the package
 * directory on eitehr source.
 *
 * For local removes, we also use the instance (dir) name, since we have
 * an exact record of what is installed on the local system and the user
 * picked a specific instance to remove.
 *
 * For remote removes, we have something of a problem since we don't know
 * exactly what's installed on the remote machine (we only keep this infor-
 * mation about the local host).  For diskless and dataless clients, we
 * remove all instances of the installed package.  Note that pkgrm's syntax
 * for this "pkg.*" requires escaping the wild-card or entire command so as
 * not to conflict with shell metacharacter expansion.  For server/standalone
 * systems, we make the assumption that the remote host is being administered
 * in parallel to the local host and remove only the "m_pkg_dir" instance
 * (the one the user picked).  A future version of swm will be smarter and
 * more flexible about this.
 */
/*
 * Local host package operation routine.  Called to
 * install packages on the local filesystem.
 */
static void
pkg_local(Modinfo *info,
	char	*pkgloc,
	SWM_mode mode)
{
	char	cmd[MAXPATHLEN*2 + 300];

	if (mode == MODE_INSTALL) {
		(void) sprintf(cmd,
		    "%s %s%s-d %s -a %s %s",
			PKGADD_CMD, get_showcr() ? " " : "-S ",
			interactive ? " " : "-n ", pkgloc,
			admin_file((char *)0),
			info->m_pkg_dir);
	} else {
		(void) sprintf(cmd, "%s %s-a %s %s",
		    PKGRM_CMD, interactive ? " " : "-n ",
		    admin_file((char *)0), info->m_pkg_dir);
	}

	(void) fprintf(stderr, gettext(
	    "Processing <%s> package instance on host <%s>\n"),
		info->m_pkg_dir, thishost);

#ifdef DEMO
	(void) fprintf(stderr, "%s\n", cmd);
	exit(0);
#else
	(void) execl("/sbin/sh", "sh", "-c", cmd, (char *)0);
	exit(-1);	/* shouldn't happen */
#endif
}

/*
 * Remote host package operation routine.  Called to install packages
 * on any *remote* host regardless of type (server, standalone, dataless).
 */
/*ARGSUSED*/
static void
pkg_remote(Hostlist *hlp,
	Modinfo	*info,
	char	*pkgloc,
	SWM_mode mode)
{
	char	cmd[BUFSIZ];
	char	*rmntdir = get_rmntdir();
	int	status;
	struct stat	statbuf;
	char	*buf = (char *)0;

	/*
	 * The admin file is sitting in the local /tmp
	 * directory.  Copy it to the remote machine's
	 * /tmp directory.  This is a little tricky since
	 * we can't do both local and remote redirection...
	 */
	(void) sprintf(cmd, "cat >%s.%s <<EOF 2>&1\n",
		admin_file((char *)0), thishost);
	if (stat(admin_file((char *)0), &statbuf) == 0) {
		size_t	len = strlen(cmd);
		size_t	size = statbuf.st_size + len + 4; 	/* 4 == EOF\0 */
		FILE	*fp;

		buf = (char *)xmalloc(size);
		(void) strcpy(buf, cmd);
		fp = fopen(admin_file((char *)0), "r");
		if (fp != (FILE *)0) {
			size = fread(&buf[len], 1, statbuf.st_size, fp);
			len += size;
			(void) fclose(fp);
		}
		(void) strcpy(&buf[len], "EOF");
	} else
		status = FAILURE;

	if (buf == (char *)0 || (status = host_run_cmd(hlp, buf)) != SUCCESS) {
		(void) fprintf(stderr, gettext(
	"Installation of <%s> package instance on host <%s> failed\n"
	"because the admin file (%s.%s) that controls the\n"
	"installation could not be created.\n"),
			info->m_pkg_dir, hlp->h_name,
			admin_file((char *)0), thishost);
		exit(status);
	}

	if (mode == MODE_INSTALL) {
		(void) sprintf(cmd,
		    "%s %s%s-d %s/%s.swm_pkg -a %s.%s %s 2>&1",
			PKGADD_CMD,
			get_showcr() ? " " : "-S ",
			interactive ? " " : "-n ",
			rmntdir, thishost,
			admin_file((char *)0), thishost,
			info->m_pkg_dir);
	} else {
		/*
		 * If a dataless client, remove all package
		 * instances (same as diskless client).  If
		 * server/standalone, remove the named instance.
		 */
		if (hlp->h_type == dataless)
			(void) sprintf(cmd,
			    "%s %s-a %s.%s \"%s.*\" 2>&1",
				PKGRM_CMD,
				interactive ? " " : "-n ",
				admin_file((char *)0), thishost,
				info->m_pkgid);
		else
			(void) sprintf(cmd,
			    "%s %s-a %s.%s %s 2>&1",
				PKGRM_CMD,
				interactive ? " " : "-n ",
				admin_file((char *)0), thishost,
				info->m_pkg_dir);
	}

	(void) fprintf(stderr, gettext(
	    "Processing <%s> package instance on host <%s>\n"),
		info->m_pkg_dir, hlp->h_name);

	status = host_run_cmd(hlp, cmd);
	/*
	 * Remove the admin file
	 */
	(void) sprintf(cmd, "rm %s.%s 2>&1", admin_file((char *)0), thishost);
	(void) host_run_cmd(hlp, cmd);
	free(buf);

	exit (status);
}

/*
 * Diskless client host package operation routine.  Called
 * to install packages on diskless clients via use of
 * pkgadd -R root/package-database relocation option.
 */
/*ARGSUSED*/
static void
pkg_diskless(Hostlist *hlp,
	Modinfo	*info,
	char	*pkgloc,
	int	spooled,	/* =1 if installing from spool directory */
	SWM_mode mode)
{
	char	cmd[BUFSIZ];

	if (mode == MODE_INSTALL) {
		(void) sprintf(cmd,
		    "%s %s%s-d %s -a %s -R %s %s",
			PKGADD_CMD,
			get_showcr() ? " " : "-S ",
			interactive ? " " : "-n ",
			pkgloc, admin_file((char *)0),
			hlp->h_rootdir, info->m_pkg_dir);
	} else {
		(void) sprintf(cmd,
		    "%s %s-a %s -R %s \"%s.*\"",
			PKGRM_CMD,
			interactive ? " " : "-n ",
			admin_file((char *)0),
			hlp->h_rootdir,
			info->m_pkgid);
	}

	(void) fprintf(stderr, gettext(
	    "Processing <%s> package instance on host <%s>\n"),
		info->m_pkg_dir, hlp->h_name);

#ifdef DEMO
	(void) fprintf(stderr, "%s\n", cmd);
	exit(0);
#else
	(void) execl("/sbin/sh", "sh", "-c", cmd, (char *)0);
	exit(-1);	/* shouldn't happen */
#endif
}

static int
pkg_make_admin(void)
{
	Admin_file *adminf;

	adminf = admin_get();
	/*
	 * Create a temporary admin file in /tmp.
	 */
	return (admin_write((char *)0, adminf));
}

static int
pkg_update_admin(Modinfo *info)
{
	Admin_file *adminf = admin_get();

	adminf->basedir = info->m_instdir ? info->m_instdir :
			info->m_basedir ? info->m_basedir : "";
	return (admin_write(admin_file((char *)0), adminf));
}

static void
pkg_errs(Modinfo *info, SWM_mode mode)
{
	Hostlist *hlp;
	int	i;

	if (mode == MODE_INSTALL)
		(void) fprintf(stderr, gettext(
		    "WARNING:  Errors were encountered installing "
		    "<%s> package instance on the following hosts:"),
			info->m_pkg_dir);
	else
		(void) fprintf(stderr, gettext(
		    "WARNING:  Errors were encountered removing "
		    "<%s> package instance on the following hosts:"),
			info->m_pkg_dir);

	i = 0;
	for (hlp = hostlist.h_next; hlp != &hostlist; hlp = hlp->h_next) {
		if ((hlp->h_status & HOST_ERROR) == 0)
			continue;
		(void) fprintf(stderr,
			"%s%s", i == 0 ? "\n\t" : ", ", hlp->h_name);
		if (++i == 3)
			i = 0;
	}
	(void) fprintf(stderr, "\n");
}

/*
 * Returns the statistics about the number of times
 * a package instance will be installed in various
 * environments.  Uses the set of selected hosts.
 */
static int
count_ops(Modinfo *info,
	SWM_mode mode,
	int	*local,		/* number of local pkgadd/pkgrm operations */
	int	*remote)	/* number of remote pkgadd/pkgrm ops */
{
	Hostlist *hlp;
	int	supported;

	*local = *remote = 0;

	for (hlp = hostlist.h_next; hlp != &hostlist; hlp = hlp->h_next) {
		if ((hlp->h_status & HOST_SELECTED) == 0)
			continue;
		if (mode == MODE_REMOVE ||
		    (supports_arch(hlp->h_arch, info->m_arch) == TRUE ||
		    strcmp(info->m_arch, "all") == 0 ||
		    strcmp(info->m_arch, "all.all") == 0))
			supported = TRUE;
		else
			supported = FALSE;
		if (strcmp(hlp->h_name, thishost) == 0 ||
		    hlp->h_type == diskless) {
			if (supported == TRUE)
				(*local)++;
		} else if (supported == TRUE)
			(*remote)++;
	}
	return (*local + *remote);
}
