#ifndef lint
#ident	 "@(#)svc_do_upgrade.c 1.21 96/08/19 SMI"
#endif
/*
 * Copyright (c) 1994-1996, by Sun Microsystems, Inc.  All rights reserved.
 */

#include <assert.h>
#include <fcntl.h>
#include <libintl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "spmicommon_api.h"
#include "spmisoft_lib.h"
#include "spmisvc_lib.h"

/* Public Function Prototypes */

int		gen_upgrade_script(void);
int		execute_upgrade(OpType, int (*)(void *, void *), void *);
void		rm_link_mv_file(char *, char *);
void		log_spacechk_failure(int);
void		MakePostKBIDirectories(void);
int 		SetupPreKBI(void);

/* Local Function Prototypes */

static char	*date_time(char *, time_t);

extern int sp_err_code;
extern int sp_err_subcode;
extern char *sp_err_path;

static char *new_logpath = "/var/sadm/system/logs/upgrade_log";
static char *old_logpath = "/var/sadm/install_data/upgrade_log";

static char *old_scriptpath = "/var/sadm/install_data/upgrade_script";
static char *new_scriptpath = "/var/sadm/system/admin/upgrade_script";

static void	catch_prog_sig(int);
static int	(*exec_callback_proc)(void *, void *);
static void	*exec_callback_arg;

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * Function:	MakePostKBIDirectories()
 * Description: This function creates the necessary directories for a
 *		post KBI system
 *
 * Scope:	public
 * Parameters:
 *		None
 * Return:
 *		int		0 	= Success
 *				not 0 	= Failure
 */

void
MakePostKBIDirectories(void)
{
	char	dir[MAXPATHLEN];
	char	tdir[MAXPATHLEN];

	/*
	 * Since this root is pre var/sadm change make the
	 * directories.
	 */

	(void) sprintf(dir, "%s/var/sadm/system",
	    get_rootdir());
	(void) mkdir(dir, (mode_t)00755);
	(void) sprintf(tdir, "%s/logs", dir);
	(void) mkdir(tdir, (mode_t)00755);
	(void) sprintf(tdir, "%s/data", dir);
	(void) mkdir(tdir, (mode_t)00755);
	(void) sprintf(tdir, "%s/admin", dir);
	(void) mkdir(tdir, (mode_t)00755);
	(void) sprintf(tdir, "%s/admin/services", dir);
	(void) mkdir(tdir, (mode_t)00755);
}

/*
 * Function:	SetupPreKBI()
 * Description: This function creates the necessary directories on pre
 *		KBI systems
 *
 * Scope:	private
 * Parameters:
 *		None
 * Return:
 *		int		0 	= Success
 *				not 0 	= Failure
 */

int
SetupPreKBI(void)
{

	Module		*mod;
	Module		*prodmod;

	/*
	 * Loop through the module list to find the product that is
	 * going to be used to upgrade the system.
	 */

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type != INSTALLED_SVC &&
		    mod->info.media->med_type != INSTALLED &&
		    mod->sub->type == PRODUCT &&
		    strcmp(mod->sub->info.prod->p_name, "Solaris") == 0)
			break;
	}

	/*
	 * If we found the upgrade product then assign the product module
	 * structure.
	 */

	if (mod)
		prodmod = mod->sub;

	/*
	 * Otherwise, we could not find the Product to use to upgrade the
	 * system.  This is a problem, so we're out of here.
	 */

	else
		return (-1);
	/*
	 * If we are not running in simulation and the system being upgraded
	 * is pre-KBI.
	 */

	if (is_KBI_service(prodmod->info.prod)) {

		/* make new directories if need be */

		if (! is_new_var_sadm("/")) {
			MakePostKBIDirectories();
		}
	}
	return (0);
}

/*
 * Function:	rm_link_mv_file
 * Description:	If there is a symbolic link in the old_location, remove
 *		it.  If there is a file in the old_location, not a symbolic
 *		link, move it to new_location in a dated form.
 * Scope:	PUBLIC
 * Parameters:	old_location -	RO
 *				type char *, full filepath
 *		new_locaiton -	RO
 *				type char *, full filepath
 */
void
rm_link_mv_file(char * old_location, char * new_location)
{
	char date_str[MAXNAMELEN];
	char name_buf[MAXPATHLEN];
	char logfile[MAXPATHLEN];
	struct stat buf;

	(void) sprintf(name_buf, "%s%s", get_rootdir(), old_location);
	if (lstat(name_buf, &buf) == 0) {
		if ((buf.st_mode & S_IFLNK) == S_IFLNK)
			(void) unlink(name_buf);
		else if ((buf.st_mode & S_IFREG) == S_IFREG) {
			(void) sprintf(logfile, "%s%s",
				get_rootdir(), new_location);
			(void) strcpy(date_str,
				date_time(logfile, buf.st_mtime));
			(void) strcat(logfile, "_");
			(void) strcat(logfile, date_str);
			(void) rename(name_buf, logfile);
		}
	}
}

/*
 * Function:	log_spacechk_failure
 * Description:
 *
 * Scope:	PUBLIC
 * Parameters:
 */
void
log_spacechk_failure(int code)
{
	char *nullstring = "NULL";

	/*
	 *  If sp_err_path is null, make sure it points to a valid
	 *  string so that none of these printfs coredump.
	 */
	if (sp_err_path == NULL)
		sp_err_path = nullstring;

	switch (code) {
	case SP_ERR_STAT:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Stat failed: %s\n"), sp_err_path);
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "errno = %d\n"), sp_err_subcode);
		break;

	case SP_ERR_STATVFS:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Statvfs failed: %s\n"), sp_err_path);
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "errno = %d\n"), sp_err_subcode);
		break;

	case SP_ERR_GETMNTENT:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Getmntent failed: errno = %d\n"), sp_err_subcode);
		break;

	case SP_ERR_MALLOC:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Malloc failed.\n"));
		break;

	case SP_ERR_PATH_INVAL:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Internal error: invalid path: %s\n"), sp_err_path);
		break;

	case SP_ERR_CHROOT:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Failure doing chroot.\n"));
		break;

	case SP_ERR_NOSLICES:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "No upgradable slices found.\n"));
		break;

	case SP_ERR_POPEN:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Popen failed: %s\n"), sp_err_path);
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "error = %d\n"), sp_err_subcode);
		break;

	case SP_ERR_OPEN:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Open failed: %s\n"), sp_err_path);
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "errno = %d\n"), sp_err_subcode);
		break;

	case SP_ERR_PARAM_INVAL:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Internal error: invalid parameter.\n"));
		break;

	case SP_ERR_STAB_CREATE:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Space check failed: couldn't create file-system "
		    "table.\n"));
		if (sp_err_code != SP_ERR_STAB_CREATE) {
			(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
			    "Reason for failure:\n"));
			log_spacechk_failure(sp_err_code);
		}
		break;

	case SP_ERR_CORRUPT_CONTENTS:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Space check failed: package database is corrupted.\n"));
		break;

	case SP_ERR_CORRUPT_PKGMAP:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Space check failed: package's pkgmap is not in "
		    "the correct format.\n"));
		break;

	case SP_ERR_CORRUPT_SPACEFILE:
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Space check failed: package's spacefile "
		    "is not in the correct format.\n"));
		break;

	}
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*ARGSUSED0*/
/*
 * Function:	catch_prog_sig
 * Description:
 *
 * Scope:	PUBLIC
 * Parameters:
 */
static void
catch_prog_sig(int sig)
{
	FILE	*fp;
	char	buf[BUFSIZ + 1];
	char	stagestr[32];
	char	detail[MAXPATHLEN];
	ValProgress	Progress;
	int	num_fields = 0;
	ValStage	stage = VAL_UNKNOWN;
	int	total, completed;

	/*
	 * We may get another signal before we're done handling this
	 * one, so hold one in the queue if it comes in.  If more than
	 * one comes in, they get dropped - harmless.
	 */
	(void) sighold(SIGUSR1);

	if ((fp = fopen("/tmp/upg_prog", "r")) != NULL) {
		if (fgets(buf, BUFSIZ, fp))
			num_fields = sscanf(buf, "%s %s %d %d",
			    stagestr, detail, &total, &completed);
	}
	(void) fclose(fp);
	if (num_fields == 4) {
		if (streq(stagestr, "pkgadd"))
			stage = VAL_EXEC_PKGADD;
		else if (streq(stagestr, "pkgrm"))
			stage = VAL_EXEC_PKGRM;
		else if (streq(stagestr, "removef"))
			stage = VAL_EXEC_REMOVEF;
		else if (streq(stagestr, "spool_pkg"))
			stage = VAL_EXEC_SPOOL;
		else if (streq(stagestr, "rm_template"))
			stage = VAL_EXEC_RMTEMPLATE;
		else if (streq(stagestr, "rmdir"))
			stage = VAL_EXEC_RMDIR;
		else if (streq(stagestr, "remove_svc"))
			stage = VAL_EXEC_RMSVC;
		else if (streq(stagestr, "remove_patch"))
			stage = VAL_EXEC_RMPATCH;
		else if (streq(stagestr, "rm_template_dir"))
			stage = VAL_EXEC_RMTEMPLATEDIR;
		if (stage != VAL_UNKNOWN &&
		    total > 0 &&
		    exec_callback_proc) {
			Progress.valp_stage = stage;
			Progress.valp_detail = detail;
			Progress.valp_percent_done =
			    (int)(((float)completed/(float)total) * 100);
			(void) exec_callback_proc(exec_callback_arg,
			    (void *)&Progress);
		}
	}
	(void) signal(SIGUSR1, catch_prog_sig);
}

/*
 * Function:	date_time
 * Description:	Given a filename and a time, in seconds, create a
 *		unique dated filename in the following format:
 *
 *			filename_MON_DAY_YEAR[_INDEX]
 *
 *		where
 *			MON   ::= 3 character abbreviated month name
 *			DAY   ::= 2 character day of the month - 1 to 31
 *			YEAR  ::= 4 character year string
 *			INDEX ::= A character string comprised of a '_'
 *				  and a integer.  The INDEX is optional
 *				  and is used only to create a unique filename
 *				  in the event of a name collision.
 *
 * Scope:	PRIVATE
 * Parameters:	logname - RO
 *			  type char *, filename
 * 		seconds - RO
 *			  type time_t, representing the time in seconds
 *			  since 00:00:00 UTC, January 1, 1970.
 */
static char *
date_time(char *logname, time_t seconds)
{
	static char	ndx_str[MAXPATHLEN];
	static char	mdy[MAXPATHLEN];
	char		stat_name[MAXPATHLEN];
	struct stat	buf;
	int		ndx;

	(void) strftime(mdy, MAXPATHLEN, "%h%d_%Y", localtime(&seconds));
	(void) sprintf(stat_name, "%s_%s", logname, mdy);
	ndx_str[0] = '\0';

	for (ndx = 1; stat(stat_name, &buf) == 0; ndx++) {
		(void) sprintf(ndx_str, "%s_%d", mdy, ndx);
		(void) sprintf(stat_name, "%s_%s", logname, ndx_str);
	}
	if (ndx_str[0] != '\0')
		return (ndx_str);
	else
		return (mdy);
}

/*
 * Function:	gen_upgrade_script
 * Description:
 *
 * Scope:	PUBLIC
 * Parameters:
 */

int
gen_upgrade_script(void)
{
	Module	*mod, *prodmod;

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type != INSTALLED_SVC &&
		    mod->info.media->med_type != INSTALLED &&
		    mod->sub->type == PRODUCT &&
		    strcmp(mod->sub->info.prod->p_name, "Solaris") == 0)
			break;
	}
	if (mod)
		prodmod = mod->sub;
	else
		return (FAILURE);

	/*
	 * If there is a symbolic link in the old location,
	 * remove it.  If there is a file, not a sym link,
	 * move it to the new location in the dated form.
	 */
	rm_link_mv_file(old_scriptpath, new_scriptpath);

	/*
	 * If there is a symbolic link in the new location,
	 * remove it.  If there is a file, not a sym link,
	 * move it to the new location in the dated form.
	 */
	rm_link_mv_file(new_scriptpath, new_scriptpath);

	set_umount_script_fcn(gen_mount_script, gen_installboot);
	(void) write_script(prodmod);

	return (SUCCESS);
}

/*
 * Function:	execute_upgrade
 * Description:
 *
 * Scope:	PUBLIC
 * Parameters:
 */

int
execute_upgrade(OpType Operation,
    int (*callback_proc)(void *, void *),
    void *callback_arg)
{
	Module	*mod, *prodmod;
	int	status;
	char	*cp;
	char	logfile[MAXPATHLEN];
	char	date_str[MAXNAMELEN];
	time_t	time_seconds;
	char	cmd[MAXPATHLEN];
	char	name_buf[MAXNAMELEN];
	int	pid;
	pid_t	w;
	int	fd;
	ValProgress Progress;

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type != INSTALLED_SVC &&
		    mod->info.media->med_type != INSTALLED &&
		    mod->sub->type == PRODUCT &&
		    strcmp(mod->sub->info.prod->p_name, "Solaris") == 0)
			break;
	}
	if (mod)
		prodmod = mod->sub;
	else
		return (-1);

	if (is_KBI_service(prodmod->info.prod)) {

		/*
		 * If there is a symbolic link in the old location,
		 * remove it.  If there is a file, not a sym link,
		 * move it to the new location in the dated form.
		 */

		rm_link_mv_file(old_logpath, new_logpath);

		/*
		 * If there is a symbolic link in the new location,
		 * remove it.  If there is a file, not a sym link,
		 * move it to the new location in the dated form.
		 */

		rm_link_mv_file(new_logpath, new_logpath);

		/*
		 * generate a dated log file name in the new location
		 */

		(void) sprintf(logfile, "%s%s",
			get_rootdir(), new_logpath);
		(void) strcpy(date_str,
			date_time(logfile, time(&time_seconds)));
		(void) strcat(logfile, "_");
		(void) strcat(logfile, date_str);
		if (Operation == SI_RECOVERY) {
			(void) sprintf(cmd,
			    "/bin/sh %s/var/sadm/system/admin/upgrade_script "
			    "%s %ld restart",
			    get_rootdir(),
			    (streq(get_rootdir(), "") ? "/" : get_rootdir()),
			    getpid());
		} else {
			(void) sprintf(cmd,
			    "/bin/sh %s/var/sadm/system/admin/upgrade_script "
			    "%s %ld",
			    get_rootdir(),
			    (streq(get_rootdir(), "") ? "/" : get_rootdir()),
			    getpid());
		}
	} else {
		(void) sprintf(logfile, "%s%s", get_rootdir(), old_logpath);
		if (Operation == SI_RECOVERY) {
			(void) sprintf(cmd,
			    "/bin/sh %s/var/sadm/install_data/upgrade_script "
			    "%s %ld restart",
			    get_rootdir(),
			    (streq(get_rootdir(), "") ? "/" : get_rootdir()),
			    getpid());
		} else {
			(void) sprintf(cmd,
			    "/bin/sh %s/var/sadm/install_data/upgrade_script "
			    "%s %ld",
			    get_rootdir(),
			    (streq(get_rootdir(), "") ? "/" : get_rootdir()),
			    getpid());
		}
	}

	exec_callback_proc = callback_proc;
	exec_callback_arg = callback_arg;
	(void) unlink(logfile);
	(void) signal(SIGUSR1, catch_prog_sig);

	/*
	 * Call the user's callback if provided with the begin state
	 */

	if (exec_callback_proc) {
		Progress.valp_percent_done = 0;
		Progress.valp_stage = VAL_UPG_BEGIN;
		Progress.valp_detail = NULL;

		if (exec_callback_proc(exec_callback_arg,
		    (void *) &Progress)) {
			return (-1);
		}
	}

	if ((pid = fork()) == 0) {
		fd = open(logfile, O_WRONLY | O_CREAT,
		    S_IRUSR | S_IRGRP | S_IROTH);
		if (fd == -1) {
			/* report error */
			return (-1);
		}
		(void) close(1);
		(void) dup(fd);
		(void) close(2);
		(void) dup(fd);
		(void) execlp("/bin/sh", "sh", "-c", cmd, (char *) NULL);
		/* shouldn't be here, but ... */
		return (-1);
	} else if (pid == -1) {
		return (-1);
	} else {

		/*
		 * Wait for the chlid process to exit
		 */

		do {
			w = waitpid(pid, &status, 0);
		} while (w == -1 && errno == EINTR);

		/*
		 * Get the exit status from the child process
		 */

		if (WIFEXITED(status)) {
			status = (int)((char)(WEXITSTATUS(status)));
		} else if (WIFSIGNALED(status)) {
			return (-1);
		} else if (WIFSTOPPED(status)) {
			return (-1);
		}

		/*
		 * Call the user's callback if provided with the end state
		 */

		if (exec_callback_proc) {
			Progress.valp_percent_done = 100;
			Progress.valp_stage = VAL_UPG_END;
			Progress.valp_detail = NULL;

			if (exec_callback_proc(exec_callback_arg,
			    (void *) &Progress)) {
				return (-1);
			}
		}
	}
	(void) sigignore(SIGUSR1);

	(void) chmod(logfile, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	/*
	 * For new post-KBI systems.
	 * Setup new symbolic link structure, IF, upgrade_log exists.
	 */

	if (is_KBI_service(prodmod->info.prod)) {
		if (access(logfile, F_OK) == 0) {
			(void) sprintf(name_buf, "%s%s",
				get_rootdir(), new_logpath);
			if ((cp = strrchr(logfile, '/')) != NULL)
				cp++;
			else
				cp = logfile;
			(void) symlink(cp, name_buf);
		}
	}
	/*
	 *  Call the driver-upgrade script, if present.
	 *  It doesn't take the rootdir as an argument (though
	 *  it should).
	 */
	if (access("/tmp/diskette_rc.d/inst9.sh", X_OK) == 0) {
		(void) system("/sbin/sh /tmp/diskette_rc.d/inst9.sh");
	}
	return (status);
}
