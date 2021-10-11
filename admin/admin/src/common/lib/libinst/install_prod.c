#ifndef lint
#pragma ident "@(#)install_prod.c 1.49 95/02/17"
#endif
/*
 * Copyright (c) 1991-1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#include "disk_lib.h"
#include "ibe_lib.h"

#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <fcntl.h>
#include <ulimit.h>

/*
 * These routines are currently declared by the calling applications
 */
extern int	progress_done(void);
extern int	progress_init(void);

/* Public Function Prototypes */

/* Library Function Prototypes */

int		_install_prod(Module *, PkgFlags *, Admin_file *,
				TransList **);

/* Local Function Prototypes */

static int	_install_pkg(Node *, caddr_t);
static int	_process_transferlist(TransList **, Node *);
static int	_atconfig_restore(void);
static int	_atconfig_store(void);

/* Module Globals */

static PkgFlags		*pkg_params;
static Admin_file	*admin_f;
static char		*prod_dir;
static int		inst_status;

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _install_prod()
 *
 * Parameters:
 *	prods	  -
 *	pkg_parms -
 *	admin	  -
 *	trans	  -  This is a pointer to the transfer file list. To be
 *			processed in _process_transferlist();
 * Return:
 *
 * Status:
 *	semi-private (internal library use only)
 */
int
_install_prod(Module *prods, PkgFlags *pkg_parms, Admin_file *admin,
		TransList **trans)
{
	Module	*cur_prod;
	Node	*np;
	char	name[MAXNAMELEN];

	/*
	 * store atconfig for safe keeping
	 */
	if (_atconfig_store() != NOERR) {
		write_notice(ERRMSG, MSG0_PKG_PREP_FAILED);
		return (ERROR);
	}

	pkg_params = pkg_parms;
	admin_f = admin;

	for (cur_prod = prods; cur_prod != NULL; cur_prod = cur_prod->next) {

		/* if there are no packages in this product, skip it */
		if (cur_prod->info.prod->p_packages == NULL)
			continue;

		/* save prod dir for use when installing pkgs */
		prod_dir = cur_prod->info.prod->p_pkgdir;

		inst_status = NOERR;

		/*
		 * the progress display is initialized here, just before the
		 * pkgadds start.  The progress_init() function clears the
		 * screen, and puts up the actual progress display
		 */
		(void) progress_init();

		/* install the pkgs associated with this product */
		/* can't use walklist -- need to exit immediately on error */

		for (np = cur_prod->info.prod->p_packages->list->next;
				np != cur_prod->info.prod->p_packages->list;
				np = np->next) {
			/* ignore null packages */
			if (((Modinfo*)np->data)->m_shared == NULLPKG) {
				if (((Modinfo*)np->data)->m_instances == NULL) {
					write_notice(WARNMSG,
						MSG1_PKG_NONEXISTENT,
						((Modinfo*)np->data)->m_pkgid);
				}

				continue;
			}

			/* call pkgadd to install the package */
			if (_install_pkg(np, NULL) == ERROR) {
				write_notice(ERRMSG,
					MSG0_PKG_INSTALL_INCOMPLETE);
				(void) progress_done();
				return (ERROR);
			}

			/* restore atconfig file if necessary */
			if (_atconfig_restore() == ERROR) {
				(void) progress_done();
				return (ERROR);
			}

			/*
			 * Setup symlinks for any files found in the
			 * transfer_list which depend on this package.
			 */
			if (_process_transferlist(trans, np) == ERROR) {
				write_notice(ERRMSG,
					MSG0_PKG_INSTALL_INCOMPLETE);
				(void) progress_done();
				return (ERROR);
			}
		}

		/*
		 * the progress display is terminated here, just after all
		 * pkgadds are done.  The progress_done() function clears the
		 * screen, shuts down curses and sets the terminal mode to
		 * something reasonable for the printf()s which follow.
		 * when install_prod() returns, more stuff gets printed out,
		 * so there's a final call to progress_cleanup(), which
		 * does the last bits of screen and terminal mode resetting
		 * (this happens in ibe_sm.c). This is a callback function
		 * into the application code.
		 */
		(void) progress_done();

		/* open the product file */
		if (_open_product_file(cur_prod->info.prod) != NOERR) {
			write_notice(ERRMSG, MSG0_SOFTINFO_CREATE_FAILED);
			return (ERROR);
		}

		/* create release file for product */
		if (_create_inst_release(cur_prod->info.prod) != NOERR) {
			write_notice(ERRMSG, MSG0_RELEASE_CREATE_FAILED);
			return (ERROR);
		}

		(void) sprintf(name, "%s %s", cur_prod->info.prod->p_name,
			cur_prod->info.prod->p_version);

		if (inst_status == NOERR) {
			write_status(LOGSCR, LEVEL0,
				MSG1_PKG_INSTALL_SUCCEEDED,
				name);
		} else {
			write_status(LOGSCR, LEVEL0,
				MSG1_PKG_INSTALL_PARTFAIL,
				name);
		}
	}

	return (NOERR);
}

/* ******************************************************************** */
/*			LOCAL SUPPORT FUNCTIONS				*/
/* ******************************************************************** */
/*
 * _install_pkg()
 *	Install the specified package onto the system.
 * Parameters:
 *	np	- Node pointer for package list
 *	dummy	- required for walklist, but not used in this routine
 * Return:
 *	NOERR	- success
 *	ERROR	- error occurred
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
_install_pkg(Node *np, caddr_t dummy)
{
	Modinfo *mp;
	int	results;

	mp = (Modinfo *)np->data;

	/* if pkg is not selected, or pkg_arch is not sys_arch, cont */
	if (mp->m_status == UNSELECTED ||
			_arch_cmp(mp->m_arch, get_default_impl(),
				get_default_inst()) != TRUE)
		return (NOERR);

	/* create admin file if package should be installed */
	admin_f->basedir = mp->m_basedir;
	if (_build_admin(admin_f) != NOERR)
		return (ERROR);

	/* add current package */
	results = add_pkg(mp->m_pkg_dir, pkg_params, prod_dir);
	if (results == NOERR || results == PKGREBOOT ||
			results == PKGIREBOOT)
		mp->m_status = INSTALL_SUCCESS;
	else {
		mp->m_status = INSTALL_FAILED;
		inst_status = ERROR;
	}

	return (NOERR);
}

/*
 * _process_transferlist()
 *	This function is called after every pkgadd to determine if any of
 *	the files in the transfer list are part of the installed package.
 *	If a file is to be processed the newly installed file is replaced
 *	by a symlink to the /tmp/root file and the access permissions are
 *	stored for latter use.
 * Parameters:
 *	transL	- a pointer to the TransList structure list to be
 *		  processed.
 *	np	- a node pointer for a package list (used to get the
 *		  package name).
 * Return:
 *	ERROR - processing of transfer list for this package
 *		failed. REASONS: couldn't determine package name, or
 *		transferlist is corrupted.
 *	NOERROR - processing of transfer list for this package succeeded,
 *		or this is not an indirect installation.
 * Status:
 *	private
 * NOTE:
 *	This assumes that the max length of a package name is 9
 *	characters. This is in compliance with the packaging API.
 */
static int
_process_transferlist(TransList **transL, Node *np)
{
	char		*pkg_id;	/* the name pf the package	*/
	int		i = 0;		/* loop counter			*/
	static int	done = 0;	/*  found and processed counter	*/
	struct stat	Stat_buf,
			tmpBuf;		/* Junk buffer for test read 	*/
	char		tmpFile[MAXPATHLEN];	/* /tmp/root file name	*/
	char		aFile[MAXPATHLEN];	/* name of /a file	*/
	TransList	*trans = *transL;
	Modinfo 	*mp;

	mp = (Modinfo *)np->data;

	if (DIRECT_INSTALL || (get_install_debug() > 0))
		return (NOERR);

	/* Determine the name (id) of the package */
	if (mp->m_pkgid != NULL)
		pkg_id = mp->m_pkgid;
	else if (mp->m_pkginst != NULL)
		pkg_id = mp->m_pkginst;
	else				/* no valid name found		*/
		return (ERROR);

	/* Make sure the 1st element of array has a good size */
	if (trans[0].found <= 0) {
		write_notice(ERRMSG, MSG0_TRANS_CORRUPT);
		return (ERROR);
	}

	/* Step through the transfer array looking for items to process */
	for (i = 1; i <= trans[0].found; i++) {
		/* Check to see if all the items have been processed */
		if (done == trans[0].found)
			break;

		/* has this item been found? */
		if (trans[i].found != 0)
			continue;

		/* does the package match this element? */
		if (strcmp(trans[i].package, pkg_id) == 0) {
			/* Check to see if the file name is not null */
			if (trans[i].file == NULL)
				continue;
			/*
			 * Make up the file names.
			 * file being checked is in /a
			 * file being created is in /tmp/root
			 */
			(void) sprintf(tmpFile, "/tmp/root%s", trans[i].file);
			(void) sprintf(aFile, "%s%s",
				get_rootdir(), trans[i].file);
			/*
			 * Get the information about the newly installed
			 * file. It is maybe OK if this fails. It could
			 * just mean that the file is not in this package.
			 */
			if (stat(aFile, &Stat_buf) >= 0) {
				/* If the file is not in /tmp/root don't */
				/* do anything */
				if (stat(tmpFile, &tmpBuf) > 0) {
					/* First: Remove the pkgadded file */
					(void) unlink(aFile);

					/* Then: Link the files together */
					if (symlink(tmpFile, aFile) < 0) {
						write_notice(WARNMSG,
							MSG2_LINK_FAILED,
							tmpFile,
							aFile);
						return (ERROR);
					}
				}
			} else {
				/* If the aFile does not exist get the */
				/* info for the /tmp/root file */
				if (stat(tmpFile, &Stat_buf) < 0)
					continue;
			}

			/* Store the file information for later use. */
			trans[i].mode = Stat_buf.st_mode;
			trans[i].uid = Stat_buf.st_uid;
			trans[i].gid = Stat_buf.st_gid;
			trans[i].found = 1;
			done++;
		}
	}
	*transL = trans;
	return (NOERR);
}

/*
 * _atconfig_restore()
 *	Restore the atconfig file.
 * Parameters:
 *	none
 * Return:
 *	NOERR	- restore successful
 *	ERROR	- restore failed
 * Status:
 *	private
 */
static int
_atconfig_restore(void)
{
	static char	path[128] = "";
	static char	save[64] = "";
	static int	complete = 0;

	/* bypass for dry-run */
	if (get_install_debug() > 0)
		return (NOERR);

	/* we only call this routine the first time the file is found */
	if (complete > 0)
		return (NOERR);

	if (path[0] == '\0') {
		(void) sprintf(save, "%s%s", get_rootdir(), IDSAVE);
		(void) sprintf(path, "%s%s", get_rootdir(), IDKEY);
	}

	/*
	 * if the id key file just appeared with this package add,
	 * restore the saved copy if there is one
	 */
	if (access(path, F_OK) == 0) {
		if (access(save, F_OK) == 0) {
			if (_copy_file(path, save) != NOERR)
				return (ERROR);

			(void) unlink(save);
		}

		complete++;
	}

	return (NOERR);
}

/*
 * _atconfig_store()
 *	Store the atconfig file for safe keeping.
 * Parameters:
 *	none
 * Return:
 *	NOERR	- storage successful
 *	ERROR	- storage failed
 * Status:
 *	private
 */
static int
_atconfig_store(void)
{
	char	path[MAXPATHLEN];
	char	save[64];

	/* bypass for dry-run */
	if (get_install_debug() > 0)
		return (NOERR);

	/*
	 * if the id key file exists, save it on the target file
	 * system in case for disaster recovery
	 */
	(void) sprintf(path, "/tmp/root/%s", IDKEY);
	if (access(path, F_OK) == 0) {
		(void) sprintf(save, "%s%s", get_rootdir(), IDSAVE);
		if (_copy_file(save, path) != NOERR)
			return (ERROR);
	}

	return (NOERR);
}
