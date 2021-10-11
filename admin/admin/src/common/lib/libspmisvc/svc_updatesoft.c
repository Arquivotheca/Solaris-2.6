#ifndef lint
#pragma ident "@(#)svc_updatesoft.c 1.6 96/09/13 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	svc_updatesoft.c
 * Group:	libspmisvc
 * Description: Routines to install software objects onto the live
 *		system.
 */

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <ulimit.h>
#include <unistd.h>
#include <wait.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "spmisvc_lib.h"
#include "spmisoft_lib.h"
#include "spmicommon_api.h"
#include "svc_strings.h"

/* internal prototypes */

int		_setup_software(Module *, TransList **, TCallback *, void *);

/* private prototypes */

static int	_atconfig_restore(void);
static int	_atconfig_store(void);
static int 	_create_inst_release(Product *);
static int	_install_prod(Module *, PkgFlags *, Admin_file *, TransList **,
    TCallback *, void *);
static int _install_pkg(Node *np, caddr_t dummy,
    TCallback *ApplicationCallback, void *ApplicationData);
static int	_open_product_file(Product *);
static int 	_pkg_status(Node *, caddr_t);
static void	_print_results(Module *);
static int	_process_transferlist(TransList **, Node *);
static int _add_pkg (char *pkg_dir, PkgFlags *pkg_params, char *prod_dir,
    TCallback *ApplicationCallback, void *ApplicationData);
static int	_setup_transferlist(TransList **);
static int	_setup_software_results(Module *);

/* globals */

static PkgFlags		*pkg_params;
static Admin_file	*admin_f;
static char		*prod_dir;
static int		inst_status;

/* locale statics */

static ModStatus	cur_stat;
static short		have_one;
static char		product[32];

/*---------------------- internal functions -----------------------*/

/*
 * Function:	_setup_software
 * Description:
 * Scope:	internal
 * Parameters:	prod	- pointer to product structure
 *		trans	- A pointer to the list of files being transfered from
 *			  /tmp/root to the indirect install location.
 * Return:	NOERR	- success
 *		ERROR	- error occurred
 */
int
_setup_software(Module *prod,
    TransList **trans,
    TCallback *ApplicationCallback,
    void *ApplicationData)
{
	Admin_file	admin;
	PkgFlags	pkg_parms;

	if (get_machinetype() == MT_CCLIENT)
		return (NOERR);

	/* Read in the transferlist of files */
	if (_setup_transferlist(trans) == ERROR) {
		write_notice(ERRMSG, MSG0_TRANS_SETUP_FAILED);
		return (ERROR);
	}

	_setup_admin_file(&admin);
	_setup_pkg_params(&pkg_parms);

	/* print the solaris installation introduction  message */
	write_status(LOGSCR, LEVEL0, MSG0_SOLARIS_INSTALL_BEGIN);

	/* install software packages */
	if (_install_prod(prod,
	    &pkg_parms,
	    &admin,
	    trans,
	    ApplicationCallback,
	    ApplicationData) == ERROR)
		return (ERROR);

	/* print out the results of the installation */
	_print_results(prod);

	/*
	 * install the software related files on installed system
	 * for future upgrade
	 */
	if (_setup_software_results(prod) != NOERR) {
		write_notice(ERRMSG, MSG0_ADMIN_INSTALL_FAILED);
		return (ERROR);
	}

	return (NOERR);
}

/*---------------------- private functions -----------------------*/

/*
 * Function:	_atconfig_restore
 * Description:	Restore the atconfig file.
 * Scope:	private
 * Parameters:	none
 * Return:	NOERR	- restore successful
 *		ERROR	- restore failed
 */
static int
_atconfig_restore(void)
{
	static char	path[128] = "";
	static char	save[64] = "";
	static int	complete = 0;

	/* if execution is simulated, return immediately */
	if (GetSimulation(SIM_EXECUTE))
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
 * Function:	_atconfig_store
 * Description:	Store the atconfig file for safe keeping.
 * Scope:	private
 * Parameters:	none
 * Return:	NOERR	- storage successful
 *		ERROR	- storage failed
 */
static int
_atconfig_store(void)
{
	char	path[MAXPATHLEN];
	char	save[64];

	/* if execution is simulated, return immediately */
	if (GetSimulation(SIM_EXECUTE))
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

/*
 * Function:	_create_inst_release
 * Description:	Create the softinfo INST_RELEASE file on the image being
 *		created and log the current Solaris release, version, and
 *		revision representing the product installed on the system.
 * Scope:	private
 * Parameters:	prod	- non-NULL product structure pointer for the Solaris
 *			  product
 * Return:	NOERR	- action completed (or skipped if debugging is turned on)
 *		ERROR	- unable to create INST_RELEASE file
 */
static int
_create_inst_release(Product *prod)
{
	FILE 	*fp;
	char	entry[256];

	/* if execution is simulated, return immediately */
	if (GetSimulation(SIM_EXECUTE))
		return (NOERR);

	(void) sprintf(entry, "%s%s/INST_RELEASE",
			get_rootdir(), SYS_ADMIN_DIRECTORY);

	if ((fp = fopen(entry, "w")) == NULL)
		return (ERROR);

	(void) fprintf(fp, "OS=%s\nVERSION=%s\nREV=%s\n",
			prod->p_name, prod->p_version, prod->p_rev);
	(void) fclose(fp);

	return (NOERR);
}

/*
 * Function:	_install_prod
 * Description:
 * Scope:	private
 * Parameters:	prods	  -
 *		pkg_parms -
 *		admin	  -
 *		trans	  - This is a pointer to the transfer file list. To be
 *			    processed in _process_transferlist();
 * Return:
 */
static int
_install_prod(Module *prods,
    PkgFlags *pkg_parms,
    Admin_file *admin,
    TransList **trans,
    TCallback *ApplicationCallback,
    void *ApplicationData)
{
	Module	*cur_prod;
	Node	*np;
	char	name[MAXNAMELEN];
	TSoftUpdateStateData StateData;

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
		 * If a callback has been provided by the calling application
		 * then call it with the state set to Begin.
		 */
		if (ApplicationCallback) {
			StateData.State = SoftUpdateBegin;
			if (ApplicationCallback(ApplicationData,&StateData)) {
				return (ERROR);
			}
		}

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
			if (_install_pkg(np, NULL, ApplicationCallback, ApplicationData) == ERROR) {
				write_notice(ERRMSG,
					MSG0_PKG_INSTALL_INCOMPLETE);
				if (ApplicationCallback) {
					StateData.State = SoftUpdateEnd;
					if (ApplicationCallback(ApplicationData,&StateData)) {
						return (ERROR);
					}
				}
				return (ERROR);
			}

			/* restore atconfig file if necessary */
			if (_atconfig_restore() == ERROR) {
				if (ApplicationCallback) {
					StateData.State = SoftUpdateEnd;
					if (ApplicationCallback(ApplicationData,&StateData)) {
						return (ERROR);
					}
				}
				return (ERROR);
			}

			/*
			 * Setup symlinks for any files found in the
			 * transfer_list which depend on this package.
			 */
			if (_process_transferlist(trans, np) == ERROR) {
				write_notice(ERRMSG,
					MSG0_PKG_INSTALL_INCOMPLETE);
				if (ApplicationCallback) {
					StateData.State = SoftUpdateEnd;
					if (ApplicationCallback(ApplicationData,&StateData)) {
						return (ERROR);
					}
				}
				return (ERROR);
			}
		}

		/*
		 * the progress display is terminated here, just after all
		 * pkgadds are done.  If the callback is provided call it to
		 * signify that the processing is complete.
		 */
		if (ApplicationCallback) {
			StateData.State = SoftUpdateEnd;
			if (ApplicationCallback(ApplicationData,&StateData)) {
				return (ERROR);
			}
		}

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

/*
 * Function:	_install_pkg
 * Description:	Install the specified package onto the system.
 * Scope:	private
 * Parameters:	np	- Node pointer for package list
 *		dummy	- required for walklist, but not used in this routine
 * Return:	NOERR	- success
 *		ERROR	- error occurred
 */
/*ARGSUSED1*/
static int
_install_pkg(Node *np, 
    caddr_t dummy,
    TCallback *ApplicationCallback,
    void *ApplicationData)
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
	results = _add_pkg(mp->m_pkg_dir,
	    pkg_params,
	    prod_dir,
	    ApplicationCallback,
	    ApplicationData);
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
 * Function:	_open_product_file
 * Description: Open/create the product release file on the targetted install
 *		image for appended writing. Log the current product information.
 *		The softinfo directory is also created if one does not already
 *		exist. The file is in the softinfo directory, and has a name of
 *		the form:
 *
 *			<PRODUCT>_<VERSION>
 *
 *		The file is set to no buffering to avoid the need to
 *		close the file upon completion. The file format is:
 *			OS=<product name>
 *			VERSION=<product version>
 *			REV=<product revision>
 * Scope:	private
 * Parameters:	prod	- non-NULL Product structure pointer
 * Return:	NOERR	- product file open
 *		ERROR	- product file open failed
 */
static int
_open_product_file(Product *prod)
{
	char	path[256];
	FILE	*fp;

	/* if execution is simulated, return immediately */
	if (GetSimulation(SIM_EXECUTE))
		return (NOERR);

	(void) sprintf(path, "%s%s/%s_%s",
		get_rootdir(),
		SYS_SERVICES_DIRECTORY, prod->p_name, prod->p_version);

	if ((fp = fopen(path, "a")) != NULL) {
		(void) fprintf(fp, "OS=%s\nVERSION=%s\nREV=%s\n",
			prod->p_name, prod->p_version, prod->p_rev);
		(void) fclose(fp);
		return (NOERR);
	}

	return (ERROR);
}

/*
 * Function:	_pkg_status
 * Description: Function used in walklist() to print the status of the node.
 * Scope:	private
 * Parameters:	np	- node pointer to current node being processed
 *		dummy	- required parameter for walklist, but not used here
 * Return:	0	- always returns this value
 */
/*ARGSUSED1*/
static int
_pkg_status(Node *np, caddr_t dummy)
{
	Modinfo * 	mp;
	u_char		log;

	mp = (Modinfo *) np->data;

	/* log successful packages only for execution simulation */
	log = (GetSimulation(SIM_EXECUTE) ? LOGSCR : LOG);

	if (mp->m_status == cur_stat) {
		if (cur_stat == INSTALL_SUCCESS) {
			if (have_one == 0) {
				write_status(log, LEVEL0,
					PKGS_FULLY_INSTALLED, product);
			}
			write_status(log, LEVEL2, mp->m_pkgid);
		} else if (cur_stat == INSTALL_FAILED) {
			if (have_one == 0) {
				write_status(LOGSCR, LEVEL0,
					PKGS_PART_INSTALLED, product);
			}
			write_status(LOGSCR, LEVEL2, mp->m_pkgid);
		}
		have_one++;
	}
	return (0);
}

/*
 * Function:	_print_results
 * Description: Walk through the linked list of products. Walk the package
 *		chain for each product and print out the names of those
 *		packages which have an INSTALL_SUCCESS status. Thne walk
 *		through the chain and print out then names of those packages
 *		which have an INSTALL_FAILED status (partials).
 * Scope:	private
 * Parameters:	prod	  - pointer to the head of the product list to be printed
 * Return:	none
 */
static void
_print_results(Module *prod)
{
	Module	*t;

	for (t = prod; t != NULL; t = t->next) {
		(void) sprintf(product, "%s %s",
			t->info.prod->p_name, t->info.prod->p_version);

		/* look for all packages with a successful install status */

		have_one = 0;
		cur_stat = INSTALL_SUCCESS;
		(void) walklist(t->info.prod->p_packages, _pkg_status,
				(caddr_t) NULL);
		if (have_one == 0)
			write_status(LOG, LEVEL2, NONE_STRING);

		/* look for all packages with an unsuccessful install status */
		have_one = 0;
		cur_stat = INSTALL_FAILED;
		(void) walklist(t->info.prod->p_packages, _pkg_status,
				(caddr_t) NULL);
	}
}

/*
 * Function:	_process_transferlist
 * Description: This function is called after every pkgadd to determine if any of
 *		the files in the transfer list are part of the installed package.
 *		If a file is to be processed the newly installed file is replaced
 *		by a symlink to the /tmp/root file and the access permissions are
 *		stored for latter use.
 * Scope:	private
 * Parameters:	transL	- a pointer to the TransList structure list to be
 *			  processed.
 *		np	- a node pointer for a package list (used to get the
 *	 		  package name).
 * Return:	ERROR - processing of transfer list for this package
 *			failed. REASONS: couldn't determine package name, or
 *			transferlist is corrupted.
 *		NOERROR - processing of transfer list for this package succeeded,
 *			or this is not an indirect installation.
 * Note:	This assumes that the max length of a package name is 9
 *		characters. This is in compliance with the packaging API.
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

	/*
	 * do not process the transferlist for direct installations or
	 * execution simulations
	 */
	if (DIRECT_INSTALL || GetSimulation(SIM_EXECUTE))
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
 * Function:	_setup_transferlist
 * Description:	Initialize the transfer list with the files to be transfered to
 *		the indirect installation directory after the initial
 *		installation. The data structures are initialized with data from
 *		the transfer_list file.
 * Scope:	private
 * Parameters:	transL	- a pointer to the TransList structure list to be
 *			  initialized.
 * Return:	NOERROR - setup of transfer list succeeded
 *		ERROR - setup of transfer list failed. Reasons: could not open
 *			file, couldn't read file, couldn't malloc space, or
 *			transfer-file list corrupted.
 */
static int
_setup_transferlist(TransList **transL)
{
	FILE		*TransFile;	/* transferlist file pointer	*/
	int		i, allocCount;	/* Simple counter		*/
	TransList 	*FileRecord;	/* tmp trans file item		*/
	char		file[MAXPATHLEN], /* individual transfer files	*/
			package[32];	/* String for the  package name	*/

	/*
	 * do not process the transferlist for direct installations or
	 * execution simulations
	 */
	if (DIRECT_INSTALL || GetSimulation(SIM_EXECUTE))
		return (NOERR);

	if ((TransFile = fopen(TRANS_LIST, "r")) == NULL) {
		write_notice(ERRMSG,
			MSG_OPEN_FAILED,
			TRANS_LIST);
		return (ERROR);
	}

	/*
	 * Allocate the array for files and packages
	 * I get 50 entries a time (malloc 50 then realloc 50 more)
	 */
	if ((FileRecord = (TransList *) xcalloc(
			sizeof (TransList) * 50)) == NULL)
		return (ERROR);

	/* initialize the array counter and allocation count */
	i = 1;
	allocCount = 1;

	while (fscanf(TransFile, "%s %s\n", file, package) != EOF) {
		/* Verify that the read was good and the file and package */
		/* are of the correct length. */
		if ((file == NULL) || (package == NULL) ||
		    (strlen(file) > (size_t) MAXPATHLEN) ||
		    (strlen(package) > (size_t) 32)) {
			write_notice(WARNMSG,
				MSG_READ_FAILED,
				TRANS_LIST);
			return (ERROR);
		}

		/* See if we have to reallocate space */
		if ((i / 50) > allocCount) {
			if ((FileRecord = (TransList *) xrealloc(FileRecord,
					sizeof (TransList) *
					(50 * ++allocCount))) == NULL) {
				return (ERROR);
			}
		}

		/* Initialize the record for this file */
		FileRecord[i].file = (char *)xstrdup(file);
		FileRecord[i].package = (char *)xstrdup(package);
		FileRecord[i].found = 0;

		/* increment counter */
		i++;
	}
	/* Store the size of the array in the found filed of the 1st entry */
	FileRecord[0].found = --i;

	/* Just for safety NULL out the package and file */
	FileRecord[0].file = NULL;
	FileRecord[0].package = NULL;

	*transL = FileRecord;

	return (NOERR);
}

/*
 * Function:	_setup_software_results
 * Description:	Copy the .clustertoc to the installed system and create
 *		the CLUSTER software administration files.
 * Scope:	private
 * Parameters:	prod	- pointer to product structure
 * Return:	NOERR	- results file set up successfully
 *		ERROR	- results file failed to set up
 */
static int
_setup_software_results(Module *prod)
{
	char	path[64] = "";
	FILE	*fp;
	Module  *mp;

	if (GetSimulation(SIM_EXECUTE))
		return (NOERR);

	/* copy the .clustertoc file.  */
	(void) sprintf(path, "%s%s/.clustertoc",
		get_rootdir(), SYS_ADMIN_DIRECTORY);
	if (_copy_file(path, get_clustertoc_path(NULL)) != NOERR)
		return (ERROR);

	/* create the .platform file */
	if (write_platform_file(get_rootdir(), prod) != SUCCESS)
		return (ERROR);

	WALK_LIST(mp, get_current_metacluster()) {
		if (mp->info.mod->m_status == SELECTED ||
				mp->info.mod->m_status == REQUIRED)
			break;
	}

	if (mp == NULL)
		return (ERROR);

	/*
	 * Create the CLUSTER file based on the current metacluster
	 */
	(void) sprintf(path, "%s%s/CLUSTER",
		get_rootdir(), SYS_ADMIN_DIRECTORY);
	if ((fp = fopen(path, "a")) != NULL) {
		(void) fprintf(fp, "CLUSTER=%s\n", mp->info.mod->m_pkgid);
		(void) fclose(fp);
		return (NOERR);
	}

	return (ERROR);
}



/* Local Function Prototypes */



/*
 * Function:	_add_pkg
 * Description:	Adds the package specified by "pkgdir", using the command line
 *		arguments specified by "pkg_params".  "prod_dir" specifies the
 *		location of the package to be installed. Has both an interactive
 *		and non-interactive mode.
 * Scope:	private
 * Parameters:	pkg_dir	   - directory containing package
 *		pkg_params - packaging command line arguments
 *		prod_dir   - pathname for package to be installed
 * Returns	NOERR	   - successful
 *		ERROR	   - Exit Status of pkgadd
 */
static int
_add_pkg (char *pkg_dir,
    PkgFlags *pkg_params,
    char *prod_dir,
    TCallback *ApplicationCallback,
    void *ApplicationData)
{
	int	pid;
	u_int	status_loc = 0;
	int	options = WNOHANG;
	int	n;
	pid_t	waitstat;
	int	fdout[2];
	int	fderr[2];
	int	fdin[2];
	char	buffer[256];
	char	buf[MAXPATHLEN];
	int	size, nfds;
	struct timeval timeout;
	fd_set	readfds, writefds, execptfds;
	long	fds_limit;
	char	*cmdline[20];
	int	spool = FALSE;
	TSoftUpdateStateData StateData;

	/*
	 * If the calling application provided a callback then call it
	 * with the state set to SoftUpdatePkgAddBegin.
	 */

	if (ApplicationCallback) {
		StateData.State = SoftUpdatePkgAddBegin;
		(void) strcpy(StateData.Data.PkgAddBegin.PkgDir,pkg_dir);
		if (ApplicationCallback(ApplicationData,&StateData)) {
			return (ERROR);
		}
	}

	if (GetSimulation(SIM_ANY)) {
		if (ApplicationCallback) {
			StateData.State = SoftUpdatePkgAddEnd;
			(void) strcpy(StateData.Data.PkgAddBegin.PkgDir,pkg_dir);
			if (ApplicationCallback(ApplicationData,&StateData)) {
				return (ERROR);
			}
		}
		return (SUCCESS);
	}

	/* set up pipes to collect output from pkgadd */
	if ((pipe(fdout) == -1) || (pipe(fderr) == -1))
		return (ERROR);

	if ((pkg_params->notinteractive == 0) && (pipe(fdin) == -1))
		return (ERROR);

	if ((pid = fork()) == 0) {
		/*
		 * set stderr and stdout to pipes; set stdin if interactive
		 */
		if (pkg_params->notinteractive == 0)
			(void) dup2(fdin[0], 0);

		(void) dup2(fdout[1], 1);
		(void) dup2(fderr[1], 2);
	   	(void) close(fdout[1]);
	   	(void) close(fdout[0]);
	   	(void) close(fderr[1]);
	   	(void) close(fderr[0]);

		if ((fds_limit = ulimit(UL_GDESLIM)) <= 0)
			return (ERROR);

		/* close all file descriptors in child */
		for (n = 3; n < fds_limit; n++)
			(void) close(n);

		/* build args for pkgadd command line */
		n = 0;
		cmdline[n++] = "/usr/sbin/pkgadd";

		/* use pkg_params to set command line */
		if (pkg_params != NULL) {
			if (pkg_params->spool != NULL) {
				spool = TRUE;
				n = 0;
				cmdline[n++] = "/usr/bin/pkgtrans";
				cmdline[n++] = "-o";
				if (prod_dir != NULL)
					cmdline[n++] = prod_dir;
				else
					cmdline[n++] = "/var/spool/pkg";

				if (pkg_params->basedir != NULL) {
					(void) sprintf(buf,"%s/%s",
						pkg_params->basedir,
						pkg_params->spool);
					cmdline[n++] = buf;
				} else
					cmdline[n++] = pkg_params->spool;
			} else {
				if (pkg_params->accelerated == 1)
					cmdline[n++] = "-I";
				if (pkg_params->silent == 1)
					cmdline[n++] = "-S";
				if (pkg_params->checksum == 1)
					cmdline[n++] = "-C";
				if (pkg_params->basedir != NULL) {
					cmdline[n++] = "-R";
					cmdline[n++] = pkg_params->basedir;
				}
				if (getset_admin_file(NULL) != NULL) {
					cmdline[n++] = "-a";
					cmdline[n++] =
					    (char*)getset_admin_file(NULL);
				}
				if (pkg_params->notinteractive == 1)
					cmdline[n++] = "-n";
			}

		} else {
			if (getset_admin_file(NULL) != NULL) {
				cmdline[n++] = "-a";
				cmdline[n++] = (char*) getset_admin_file(NULL);
			}
		}

		if (prod_dir != NULL && spool == FALSE) {
			cmdline[n++] = "-d";
			cmdline[n++] = prod_dir;
		}

		cmdline[n++] = pkg_dir;
		cmdline[n++] = (char*) 0;

		(void) sigignore(SIGALRM);
		(void) execv(cmdline[0], cmdline);
		write_notice(ERROR, MSG0_PKGADD_EXEC_FAILED);

		return(ERROR);

	} else if (pid == -1) {
		if (pkg_params->notinteractive == 0)
			(void) close(fdin[0]);

		(void) close(fdout[1]);
		(void) close(fdout[0]);
		(void) close(fderr[1]);
		(void) close(fderr[0]);
		return (ERROR);
	}

	if (pkg_params->notinteractive == 0) {
		if (ApplicationCallback) {
			StateData.State = SoftUpdateInteractivePkgAdd;
			if (ApplicationCallback(ApplicationData,&StateData)) {
				return (ERROR);
			}
		}
		(void) close(fdin[0]);
		(void) close(fdin[1]);
	} else {
		nfds = (fdout[0] > fderr[0]) ? fdout[0] : fderr[0];
		nfds++;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		do {
			FD_ZERO(&execptfds);
			FD_ZERO(&writefds);
			FD_ZERO(&readfds);
			FD_SET(fdout[0], &readfds);
			FD_SET(fderr[0], &readfds);

			if (select (nfds, &readfds, &writefds,
					&execptfds, &timeout) != -1) {
				if (FD_ISSET(fdout[0], &readfds)) {
					if ((size = read(fdout[0], buffer,
						  sizeof (buffer))) != -1 ) {
						buffer[size] = '\0';
						write_status_nofmt(LOG,
						    LEVEL0|CONTINUE|FMTPARTIAL,
						    buffer);
					}
				}

				if (FD_ISSET(fderr[0], &readfds)) {
					if ((size = read(fderr[0], buffer,
						  sizeof (buffer))) != -1 ) {
						buffer[size] = '\0';
						write_status_nofmt(LOG,
						    LEVEL0|CONTINUE|FMTPARTIAL,
						    buffer);
					}
				}
			}

			waitstat = waitpid(pid, (int*)&status_loc, options);
		} while ((!WIFEXITED(status_loc) &&
				!WIFSIGNALED(status_loc)) || (waitstat == 0));
	}

	(void) close(fdout[0]);
	(void) close(fdout[1]);
	(void) close(fderr[0]);
	(void) close(fderr[1]);

	if (ApplicationCallback) {
		StateData.State = SoftUpdatePkgAddEnd;
		(void) strcpy(StateData.Data.PkgAddBegin.PkgDir,pkg_dir);
		if (ApplicationCallback(ApplicationData,&StateData)) {
			return (ERROR);
		}
	}
	return (WEXITSTATUS(status_loc) == 0 ? NOERR : ERROR);
}
