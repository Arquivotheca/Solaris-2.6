#ifndef lint
#pragma ident "@(#)pf_main.c 2.62 96/10/10 SMI"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc. All Rights Reserved.
 */

#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/mntent.h>
#include <sys/vfstab.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "spmiapp_api.h"
#include "spmisvc_api.h"
#include "spmistore_api.h"
#include "spmisoft_api.h"
#include "spmicommon_api.h"
#include "profile.h"
#include "pf_strings.h"

extern int profile_upgrade;

/* Local Function Prototypes */

static int		verify_disk_config(Profile *, int);
static int		verify_space_config(Profile *);
static int		verify_bootdisk(void);

static void		_catch_sig(int);
static void		_usage(char *);
static int		_cacheos_create_swapfile(void);
static int		_cacheos_add_vfstab_swap(void);
static int		_pf_install(Profile *);
static int		_pf_upgrade(Profile *, int);

static int		PFU_UpgradeSystem(Profile *prop, int nonnative);
static int		PFU_UpgradeChildProcessing(Profile *prop, int nonnative);
static int		PFU_SetupSliceList(Profile *prop, TList SliceList);
static void		PFU_CollapseFileSystems(Profile *prop, FSspace **Space);

/*
 * Function:	main
 *
 * Return:	0	- processing successful; complete finish
 *			  scripts and reboot
 *		1	- processing partially successful; complete
 *			  finish scripts but do not reboot
 *		2	- processing failed; do not proceed
 */
void
main(int argc, char **argv)
{
	extern  char    *optarg;
	extern	int	optind;
	Profile		profile;
	Profile		*prop = &profile;
	Disk_t		*dp;
	char		*loc = NULL;
	char		locale[64] = "";
	char		*cp;
	char		prog[64];
	char		*rootmount = "/a";
	int		estatus;
	int		i;
	int		nonnative = 0;

	(void) sigignore(SIGHUP);
	(void) sigignore(SIGPIPE);
	(void) sigignore(SIGALRM);
	(void) sigignore(SIGTERM);
	(void) sigignore(SIGURG);
	(void) sigignore(SIGCONT);
	(void) sigignore(SIGTTIN);
	(void) sigignore(SIGTTOU);
	(void) sigignore(SIGIO);
	(void) sigignore(SIGXCPU);
	(void) sigignore(SIGXFSZ);
	(void) sigignore(SIGVTALRM);
	(void) sigignore(SIGPROF);
	(void) sigignore(SIGWINCH);
	(void) sigignore(SIGUSR1);
	(void) sigignore(SIGUSR2);
	(void) signal(SIGTSTP, _catch_sig);
	(void) signal(SIGINT, _catch_sig);
	(void) signal(SIGQUIT, _catch_sig);
	(void) signal(SIGILL, _catch_sig);
	(void) signal(SIGTRAP, _catch_sig);
	(void) signal(SIGEMT, _catch_sig);
	(void) signal(SIGFPE, _catch_sig);
	(void) signal(SIGBUS, _catch_sig);
	(void) signal(SIGSEGV, _catch_sig);
	(void) signal(SIGSYS, _catch_sig);
	(void) signal(SIGQUIT, _catch_sig);

	/*
	 * initialize the propfile structure
	 */

	(void) memset(prop, 0, sizeof (Profile));
	OPTYPE(prop) = SI_UNDEFINED;
	DISKPARTITIONING(prop) = LAYOUT_UNDEFINED;
	SYSTYPE(prop) = MT_UNDEFINED;
	CLIENTROOT(prop) = -1;
	CLIENTSWAP(prop) = -1;
	CLIENTCNT(prop) = -1;
	TOTALSWAP(prop) = -1;
	PROVERSION(prop) = -1;
	(void) BootobjSetAttribute(CFG_CURRENT,
		BOOTOBJ_PROM_UPDATE, 1,
		NULL);

	/*
	 * set all program locale categories from the environment
	 * and set the domain for gettext() calls
	 */
	if ((loc = setlocale(LC_ALL, "")) != NULL &&
			!streq(loc, "C") != 0)
		(void) strcpy(locale, loc);

	/* set the default text domain for messaging */
	(void) textdomain(TEXT_DOMAIN);

	while ((i = getopt(argc, argv, "Dc:d:x:L:H:ln")) != -1) {
		switch (i) {
		case 'D':
			(void) SetSimulation(SIM_EXECUTE, 1);
			break;

		case 'c':
			MEDIANAME(prop) = xstrdup(optarg);
			break;

		case 'd':
			(void) SetSimulation(SIM_EXECUTE, 1);
			(void) SetSimulation(SIM_SYSDISK, 1);
			/*
			 * When we can simulate mounting disks, SIM_SYSSOFT
			 * should possibly be set by a -S flag.
			 */
			(void) SetSimulation(SIM_SYSSOFT, 1);
			DISKFILE(prop) = xstrdup(optarg);
			break;

		case 'x':			/* private */
			(void) set_trace_level(atoi(optarg));
			break;

		case 'L':			/* private */
			rootmount = optarg;
			break;

		case 'H': /* private - setup the package history file */
			set_pkg_hist_file(xstrdup(optarg));
			break;

		case 'n':			/* private */
			nonnative = 1;
			break;

		default:
			if ((cp = strrchr(argv[0], '/')) != NULL)
				(void) strcpy(prog, ++cp);
			else
				(void) strcpy(prog, argv[0]);
			_usage(prog);
			break;
		}
	}

	/*
	 * check for argument and profile validity
	 */
	i = argc - optind;
	if (i != 1)
		_usage(argv[0]);

	/*
	 * if this is a disk simulation, warn the user about possible 
	 * size differences in the '/' file system
	 */
	if (GetSimulation(SIM_SYSDISK)) {
		/* i18n: translate the next 2 messages as a single sentence */
		write_status(SCR, LEVEL0,
			MSG0_DEVSIZE_DIFFERENCE_0);
		write_status(SCR, LEVEL0|CONTINUE,
			MSG0_DEVSIZE_DIFFERENCE_1);
	}

	/*
	 * parameter defaults
	 *
	 * get profile name
	 */
	PROFILE(prop) = xstrdup(argv[optind++]);

	/*
	 * set default media location if not already specified
	 */
	if (MEDIANAME(prop) == NULL)
		MEDIANAME(prop) = "/cdrom";

	/*
	 * load the disk data before parsing the profile since the
	 * rootdisk is used in the pfinstall grammar and must be
	 * defined before being referenced
	 */
	if (DiskobjInitList(DISKFILE(prop)) == 0) {
		if (DISKFILE(prop)) {
			fatal_exit(MSG1_DISKFILE_FAILED,
				DISKFILE(prop));
		} else {
			write_notice(ERRMSG, MSG0_DISKS_NONE);
			write_status(LOGSCR, LEVEL1|LISTITEM,
				MSG0_DISKS_CHECK_POWER);
			fatal_exit(NULL);
		}
	}

	if (ResobjInitList() != D_OK) {
		write_notice(ERRMSG, MSG0_RESOURCE_INIT_FAILED);
		fatal_exit(NULL);
	}

	/*
	 * make sure there is at least one good disk on the system for
	 * the boot disk
	 */
	WALK_DISK_LIST(dp) {
		if (disk_okay(dp))
			break;
	}

	/*
	 * if no good disks were found the exit with error message
	 */
	if (dp == NULL) {
		write_notice(ERRMSG, MSG0_DISKS_NONE_USABLE);
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG0_DISKS_UNUSABLE_PROB1);
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG0_DISKS_UNUSABLE_PROB2);
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG0_DISKS_UNUSABLE_PROB3);
		write_status(LOGSCR, LEVEL1|LISTITEM|CONTINUE,
			MSG0_DISKS_UNUSABLE_POWERPC1);
		write_status(LOGSCR, LEVEL1|LISTITEM|CONTINUE,
			MSG0_DISKS_UNUSABLE_POWERPC2);
		fatal_exit(NULL);
	}

	/*
	 * initialize the software media tree; this must be done
	 * before the profile is parsed due to validation requirements
	 */
	if (configure_media(prop) != D_OK)
		fatal_exit("");

	/*
	 * load (parse) the profile data
	 */
	if (parse_profile(prop, locale) != D_OK)
		fatal_exit("");

	write_status(LOGSCR, LEVEL0, MSG0_PROFILE_PROCESS);

	/*
	 * if the system is an autoclient, set the root mount point to
	 * indicate a direct install; this could also be implemented by
	 * requiring autoclient setups to use the '-L' option, but for now
	 * we are not doing so. Setting the root mount point is common to
	 * both upgrade and initial install. NOTE: the following action will
	 * force an override of any command-line options.
	 */
	if (get_machinetype() == MT_CCLIENT)
		set_rootdir("");
	else
		set_rootdir(rootmount);

	/*
	 * call the appropriate profile processing function based on
	 * the install_type specifier in the profile
	 */
	switch (OPTYPE(prop)) {
	    case SI_INITIAL_INSTALL:	/* install_type initial_install */
		estatus = _pf_install(prop) < 0 ? 1 : 0;
		break;
	    case SI_UPGRADE:		/* install_type upgrade */
		estatus = _pf_upgrade(prop, nonnative) != 0 ? 2 : 0;
		break;
	    default:			/* install_type not set */
		estatus = 2;
		break;
	}

	if ((estatus == 0) && (NOREBOOT(prop) != 0))
		estatus = 1;

	if (GetSimulation(SIM_EXECUTE) || (get_trace_level() > 0))
		write_status(LOGSCR, LEVEL0, MSG1_EXIT_STATUS, estatus);

	exit(estatus);
}


/*
 *
 * Function:  _pf_install
 * Description:  Called by main to perform a profiled install.
 *
 * Scope: private
 *
 * Parameters:	prop	[RO, *RW]
 *			- pointer to initialized Profile structure
 *
 *		nonnative [RO]
 *			- Boolean flag for non-native operation
 *
 * Returns:	 0	- success
 *		 -1	- failure
 */
static int
_pf_install(Profile *prop)
{
	int		dstatus;
	int		sstatus;
	int		status = D_OK;
	int		sdiskconfig = 0;
	int		diskstatus = D_FAILED;

	TSUData		SUData;

	/*
	 * Initialize the software  library.
	 */
	sw_lib_init(NULL);

	/*
	 * update the software media tree with profile specific
	 * requests
	 */
	if (software_load(prop) != D_OK)
		fatal_exit("");

	/*
	 * customize the software tree
	 */
	if (configure_software(prop) != D_OK)
		fatal_exit("");

	/*
	 * select all drives explicitly or implicitly specified
	 */
	if (status == D_OK) {
		status = configure_disks(prop);
		diskstatus = status;
	}

	/*
	 * configure default mounts for space checking and disk
	 * validation; the must be done after disks are configured
	 * because of the "existing" configuration's impact on the
	 * dfltmnt table
	 */
	if (status == D_OK)
		status = configure_dfltmnts(prop);

	/*
	 * configure the sdisk partitions
	 */
	if (status == D_OK) {
		sdiskconfig++;
		status = configure_sdisk(prop);
	}

	/*
	 * configuration phase complete; ready to do some
	 * final checks and install
	 */

	/*
	 * deselect all drives which were not modified
	 */
	if (status == D_OK)
		status = configure_unused_disks(prop);

	/*
	 * if debugging, tracing,  or configuration is in
	 * error, display final disk setup
	 */
	if (status != D_OK || GetSimulation(SIM_EXECUTE) ||
			get_trace_level() > 0)
		if (diskstatus == D_OK ||
				DISKPARTITIONING(prop) == LAYOUT_EXIST)
			print_disk_layout();

	/*
	 * verify the disk configuration and space allocation
	 * are acceptable for the installation to proceed; only
	 * print disk information if the disks configured
	 * successfully; only print out software information
	 * if sdisk configuration was attempted
	 */
	if (diskstatus == D_OK)
		dstatus = verify_disk_config(prop, status);

	if (sdiskconfig > 0)
		sstatus = verify_space_config(prop);

	if (status != D_OK || dstatus > 0 || sstatus > 0)
		fatal_exit("");

	/*
	 * call the backend to do the actual installation
	 */

	SUData.Operation = SI_INITIAL_INSTALL;
	SUData.Info.Initial.prod = SWPRODUCT(prop);
	SUData.Info.Initial.cfs = REMOTEFS(prop);
	SUData.Info.Initial.SoftUpdateCallback = SoftUpdateHandler;
	SUData.Info.Initial.ApplicationData = NULL;

	if (SystemUpdate(&SUData)) {
		fatal_exit(MSG0_INSTALL_FAILED);
	}

	/*
	 * create /.cache/swap for autoclient installs
	 */
	if (_cacheos_create_swapfile() < 0)
		fatal_exit(MSG0_SWAPFILE_CREATE_FAILED);

	/*
	 * set the return status based on the
	 * configuration of the bootdisk
	 */
	return (verify_bootdisk() != 0 ? -1: 0);
}

/*
 * Function:  _pf_upgrade
 * Description:  Called by main to perform a profiled upgrade.
 *
 * Scope: private
 *
 * Parameters:	prop	[RO, *RW]
 *			- pointer to initialized Profile structure
 *
 *		nonnative [RO]
 *			- Boolean flag for non-native operation
 *
 * Return:	0 	- success
 *		!= 0	- failure
 */
static int
_pf_upgrade(Profile *prop, int nonnative)
{
	int		slices;
	StringList	*sp;
	StringList	*s;

	/*
	 * first, let's take stock of the number of root
	 * filesystems on this sytems which are eligible
	 * for upgrade
	 */

	SliceFindUpgradeable(&sp, NULL);

	/*
	 * no upgradeable filesystems found - give up
	 */
	if (sp == NULL)
		fatal_exit(MSG0_UPGRADE_NO_ELIGIBLE_DISKS);

	/*
	 * count them  (will return > 0)
	 */

	slices = StringListCount(sp);

	/*
	 * now, check for just 1 eligible filesystem
	 * (the typical case)
	 */

	if (slices == 1) {

		/*
		 * if no ROOTDEVICE was specified then set it
		 * to the proper slice
		 */

		if (ROOTDEVICE(prop) == NULL)
			ROOTDEVICE(prop) = xstrdup(sp->string_ptr);

		/*
		 * ROOTDEVICE was specified so then it must match
		 */

		else if (!streq(sp->string_ptr, ROOTDEVICE(prop)))
			fatal_exit(MSG1_UPGRADE_ROOTDEVICE_MISMATCH,
				ROOTDEVICE(prop));
	/*
	 * multiple eligible filesystems
	 */

	} else {

		/*
		 * multiple eligible fs's make ROOTDISK a
		 * required parameter
		 */

		if (ROOTDEVICE(prop) == NULL)
			fatal_exit(MSG0_ROOTDEV_MULTIUPGRADE);

		/*
		 * Search for the specified ROOTDEVICE filesystem
		 */
		s = StringListFind(sp, ROOTDEVICE(prop));
		if (s == NULL)
			fatal_exit(MSG1_UPGRADE_ROOTDEVICE_MISMATCH,
				ROOTDEVICE(prop));
	}
	StringListFree(sp);

	/*
	 * inform the library that this is a profiled upgrade
	 */

	(void) set_profile_upgrade();
	(void) sw_lib_init(PTYPE_UNKNOWN);

	/*
	 * mount the file systems and set up swap
	 */

	if (mount_and_add_swap(ROOTDEVICE(prop)) != 0) {

		/*
		 * If we are not running in simulation mode then the
		 * inability to mount the file system(s) is a failure
		 */

		if (!GetSimulation(SIM_EXECUTE)) {
			fatal_exit(MSG0_MOUNT_FAILURE);
		}
	}

	/*
	 * Upgrade the system to the new version ofthe O/S
	 */

	if (PFU_UpgradeSystem(prop, nonnative)) {
		return (-1);
	}

	return (0);
}

/*
 * FUNCTION NAME: PFU_UpgradeSystem
 *
 * DESCRIPTION:
 *  This function provides the mechanism to upgrade a system to the
 *  new release of the operating system.  The calling application
 *  is responsible for mounting the file system prior to calling
 *  this function.
 *
 *  This function will firxt check to see if a partial upgrade can
 *  be resumed.  If so then the upgrade will be continued from where
 *  it was interrupted.  If not, then the normal upgrade path is
 *  invoked.
 *
 *
 * RETURN:
 *  TYPE			DESCRIPTION
 *  int				 0 = Success
 *				-1 = Error
 *
 * PARAMETERS:
 *  TYPE			DESCRIPTION
 *  Profile *			The profile structure generated by
 *				parse_profile().
 *  int				Whether the installation is running
 *				native on the system or is being
 *				run on a client.
 *				0 = native
 *				1 = non-native
 *
 */

static int
PFU_UpgradeSystem(Profile *prop, int nonnative)
{
	TSUData		SUData;

	pid_t		ChildPID;
	int		ChildStatus;
	int		ExitCode;

	/*
	 * check to see if we are resuming a partially
	 * completed upgrade.
	 */

	if (UpgradeResume() != 0) {
		write_status(LOGSCR, LEVEL0,
		    MSG0_RESUME_UPGRADE);

		SUData.Operation = SI_RECOVERY;
		SUData.Info.UpgradeRecovery.ArchiveCallback = ArchiveHandler;
		SUData.Info.UpgradeRecovery.ArchiveData = NULL;
		SUData.Info.UpgradeRecovery.ScriptCallback = ScriptHandler;
		SUData.Info.UpgradeRecovery.ScriptData = NULL;

		if (SystemUpdate(&SUData)) {
			return (-1);
		}

		write_status(LOGSCR, LEVEL0,
		    MSG1_RESUME_UPGRADE_STATUS, 0);
	} else {

		/*
		 * Fork off the child process to actually generate the
		 * upgrade script.  We have to do this so that on 16MB
		 * systems we can add the swap to allow us to load the
		 * data models into memory for the space calculations.
		 * However, after the space calculations are complete,
		 * if we have run out of space on a file system then
		 * we have to be able to unmount the swap.  So, the
		 * child process does all of the work necessary to
		 * generate the upgrade script and the list of files to
		 * be archived and then exits.  After the child process
		 * exits, the parent then proceeds with the upgrade
		 * which in turn unmounts the swap and does the newfs.
		 */

		ChildPID = fork();

		/*
		 * If an error occured
		 */

		if (ChildPID == (pid_t) -1) {
			write_notice(ERRMSG, MSG0_UNABLE_TO_FORK_CHILD);
			return (-1);
		}

		/*
		 * If this is the child.
		 */

		else if (ChildPID == 0) {

			/*
			 * I have to do an exit here because I do
			 * not want to go through the normal pf_main
			 * exit processing which normalizes all exit
			 * return codes to either 0 or 2 for the
			 * upgrade path so that the calling program
			 * knows whether to reboot the system.
			 */

			exit(PFU_UpgradeChildProcessing(prop, nonnative));
		}

		/*
		 * Otherwise, this is the parent
		 */

		else {

			/*
			 * Wait for the child process to exit.
			 */

			if (waitpid(ChildPID, &ChildStatus, 0) < 0) {
				return (-1);
			}

			if (WIFEXITED(ChildStatus)) {
				ExitCode = (int)((char)
				    (WEXITSTATUS(ChildStatus)));
				if (ExitCode < 0) {
					return (-1);
				}
			} else if (WIFSIGNALED(ChildStatus)) {
				return (-1);
			} else if (WIFSTOPPED(ChildStatus)) {
				return (-1);
			}

			/* Call SystemUpdate() to finish upgrading the system */

			switch ((OpType)ExitCode) {
			case SI_UPGRADE:
				SUData.Operation = SI_UPGRADE;
				SUData.Info.Upgrade.ScriptCallback =
					ScriptHandler;
				SUData.Info.Upgrade.ScriptData = NULL;
				break;
			case SI_ADAPTIVE:
				SUData.Operation = SI_ADAPTIVE;
				SUData.Info.AdaptiveUpgrade.ArchiveCallback =
					ArchiveHandler;
				SUData.Info.AdaptiveUpgrade.ArchiveData = NULL;

				SUData.Info.AdaptiveUpgrade.ScriptCallback =
					ScriptHandler;
				SUData.Info.AdaptiveUpgrade.ScriptData = NULL;
				break;
			default:
				return (-1);
			}

			/*
			 * Call SystemUpdate() to do the remaining processing
			 */

			if (SystemUpdate(&SUData)) {
				return (-1);
			}
		}
	}
	return (0);
}

/*
 * FUNCTION NAME: PFU_UpgradeChildProcessing
 *
 * DESCRIPTION:
 *  This function performs all of the processing that must be done
 *  in the child process due to the 16MB design limitation.  The
 *  following outlines the major steps that this function follows:
 *	-loads the software to be installed from the destination media
 *	-configure the software that will be installed from the
 *	 list of possible software
 *	-determines the file system layout based on the /etc/vfstab
 *	-compute the required file system sizes based on the software
 *	 to be installed.
 *	-generate the upgrade script
 *	-if there is insufficient space to proceed with the upgrade
 *		-determine the new file system layout.
 *		-Generate the list of files to be archived
 *
 * RETURN:
 *  TYPE			DESCRIPTION
 *  int				SI_UPGRADE	= Request the parent
 *						  process to perform a
 *						  normal upgrade
 *				SI_ADAPTIVE	= Request the parent
 *						  process to perform a
 *						  adaptive upgrade
 *				-1		= Failure
 *
 * PARAMETERS:
 *  TYPE			DESCRIPTION
 *  Profile *			The profile structure generated by
 *				parse_profile().
 *  int				Whether the installation is running
 *				native on the system or is being
 *				run on a client.
 *				0 = native
 *				1 = non-native
 *
 */

static int
PFU_UpgradeChildProcessing(Profile *prop, int nonnative)
{
	OpType			Operation;
	FSspace			**Space;

	TDSRArchiveList		ArchiveList;
	unsigned long long	BytesToTransfer;
	TDSRALError		DSRALError;

	TList			SliceList;
	char			Buffer[MAXPATHLEN];

	int 			status;

	write_status(LOGSCR, LEVEL0, MSG0_LOADING_LOCAL_ENV);

	/*
	 * load the software library structures with
	 * the list of currently istalled s/w
	 */

	if (load_installed("/", FALSE) == NULL) {
		write_notice(ERRMSG, MSG0_LOADING_LOCAL_ENV_FAILURE);
		return (-1);
	}

	/*
	 * load the software library structures with
	 * the list of installed clients
	 */

	load_clients();

	write_status(LOGSCR, LEVEL0, MSG0_GEN_UPGRADE_ACTIONS);

	/*
	 * if the nonnative flag is set then setup
	 * the upgrade only for the client services
	 */

	if (nonnative) {
		status = nonnative_upgrade(NULL);
	}

	/*
	 * otherwise, setup the upgrade for the local
	 * host and all clients of the same OS version
	 */
	else {
		status = upgrade_all_envs();
	}

	/*
	 * an error here means that for some reason
	 * either it was the wrong media, not the
	 * right architecture, or perhaps the indicated
	 * version is already installed
	 */

	if (status != 0) {
		if (get_trace_level() > 0) {
			write_status(LOGSCR, LEVEL0,
			    "upgrade setup status = %d", status);
		}
		return (-1);

	} else {

		/*
		 * add the package, clusters, and locales as
		 * specified in the profile to the software
		 * library data structures.
		 */

		(void) load_view((get_default_media())->sub,
		    get_localmedia());

		/*
		 * customize the software tree
		 */

		if (configure_software(prop) != D_OK)
			return (-1);

		/*
		 * If we are running in simulation mode then set the space
		 * report to be generated in the /tmp directory
		 */

		if (GetSimulation(SIM_EXECUTE)) {
			(void) sprintf(Buffer,
			    "/tmp/upgrade_space_required");

		/*
		 * Otherwise, it goes in the real location
		 */

		} else {
			(void) sprintf(Buffer,
			    "%s/var/sadm/system/data/upgrade_space_required",
			    get_rootdir());
		}

		/*
		 * Check to see if the system being upgraded is pre-KBI
		 * and if so create the necessary directories.
		 */

		if (SetupPreKBI()) {
			return (-1);
		}

		/*
		 * Get the current file system layout
		 */

		Space = get_current_fs_layout(TRUE);
		if (Space == NULL) {
			return (-1);
		}

		/*
		 * Set the upgrade software script generation logic
		 * to preserve identical packages across the upgrade.
		 * This results in the upgrade not re-installing
		 * packages of the same revision.
		 */

		if (set_action_code_mode(PRESERVE_IDENTICAL_PACKAGES)) {
			return (-1);
		}

		/*
		 * Verify that the current file system layout will
		 * work for the upgrade.
		 */

		status = verify_fs_layout(Space, SpaceCheckingHandler, NULL);

		/*
		 * If the return code is not SUCCESS
		 */

		if (status != SUCCESS) {

			/*
			 * If the reason for the error was insufficient space
			 */

			if (status == SP_ERR_NOT_ENOUGH_SPACE) {

				Operation = SI_ADAPTIVE;

				/*
				 * Set the upgrade software script generation logic
				 * to replace identical packages across the upgrade.
				 * This is done so that packages that have the same
				 * identical versions will still be installed since
				 * they were not archived as a part of DSR and thus
				 * must be added back on.
				 */

				if (set_action_code_mode(
					REPLACE_IDENTICAL_PACKAGES)) {
					return (-1);
				}

				/*
				 * Write out the space report for the user
				 */

				print_final_results(Space, Buffer);
				write_message(LOGSCR, WARNMSG, LEVEL0,
				    dgettext("SUNW_INSTALL_LIBSVC",
					"Insufficient space for the upgrade."));
				write_message(LOGSCR, STATMSG, LEVEL0,
				    dgettext("SUNW_INSTALL_LIBSVC",
					"Space required in each file "
					"system is:"));
				CatFile(Buffer, LOGSCR, STATMSG, LEVEL0);
				if (profile_upgrade &&
				    !GetSimulation(SIM_EXECUTE)) {
					write_message(LOGSCR, STATMSG, LEVEL0,
					    dgettext("SUNW_INSTALL_LIBSVC",
						"After rebooting, the space "
						"report displayed above will "
						"be in"));
					/*
					 * Chop off rootdir
					 */
					write_message(LOGSCR, STATMSG, LEVEL2,
					    Buffer+strlen(get_rootdir()));
				}

			/*
			 * Otherwise, we don't treat it as a special error
			 * so log the space checking failure.
			 */

			} else {
				log_spacechk_failure(status);
			}

			/*
			 * If the trace level is set then print out the final
			 * space report.
			 */

			if (get_trace_level() > 0)
				print_final_results(Space, Buffer);

		/*
		 * Otherwise, the space verification completed successfully
		 */

		} else {

			Operation = SI_UPGRADE;

			write_message(LOGSCR, STATMSG, LEVEL0,
			    dgettext("SUNW_INSTALL_LIBSVC",
				"Space check complete."));
			if (GetSimulation(SIM_EXECUTE)) {
				print_final_results(Space, Buffer);
				write_message(LOGSCR, STATMSG, LEVEL0,
				    dgettext("SUNW_INSTALL_LIBSVC",
					"Space required in each file "
					"system is:"));
				CatFile(Buffer, LOGSCR, STATMSG, LEVEL0);
			} else if (get_trace_level() > 0) {
				print_final_results(Space, Buffer);
			}
		}

		write_message(LOGSCR, STATMSG, LEVEL0,
		    dgettext("SUNW_INSTALL_LIBSVC",
			"Building upgrade script"));

		/*
		 * Generate the upgrade script.
		 */

		(void) gen_upgrade_script();


		/*
		 * Check to see if we are performing a space adapting
		 * upgrade.
		 */

		if (Operation == SI_ADAPTIVE) {

			/*
			 * Collapse any file systems specified in
			 * the layout constraints
			 */

			PFU_CollapseFileSystems(prop, Space);

			/*
			 * Create a SliceList
			 */

			if (DsrSLCreate(&SliceList, NULL, Space)) {
				return (-1);
			}

			if (LAYOUTCONSTRAINT(prop) == NULL) {

				/*
				 * If the user did not provide any
				 * layout_constraints in the profile
				 * then use the default set.  This is
				 * generated by finding the set of file
				 * systems that failed the space checking
				 * and then selecting all other slices
				 * that are in the current vfstab and
				 * are resident on the same disk as
				 * a failed slice.
				 */

				if (DsrSLAutoLayout(SliceList,
				    Space,
				    True)) {
					write_notice(ERRMSG,
					    MSG0_AUTO_LAYOUT_FAILED);
					return (-1);
				}

			}

			/*
			 * Otherwise, at least one layout constraint was
			 * supplied in the profile so use the layout
			 * constraints to seed the autolayout logic.
			 */

			else {

				/*
				 * load the layout constraints into the slice
				 * list.
				 */

				if (PFU_SetupSliceList(prop, SliceList)) {
					return (-1);
				}

				/*
				 * Call the auto layout logic
				 */


				if (DsrSLAutoLayout(SliceList, Space, False)) {
					write_notice(ERRMSG,
					    MSG0_AUTO_LAYOUT_FAILED);
					return (-1);
				}
			}

			/*
			 * Ok, if we are running in simulation mode
			 * print out a copy of the disk list
			 */

			if (GetSimulation(SIM_EXECUTE)) {
				print_disk_layout();
			}

			/*
			 * Create an instance of the DSR Archive List object
			 */

			if ((DSRALError = DSRALCreate(&ArchiveList))) {
				write_notice(ERRMSG,
				    DSRALGetErrorText(DSRALError));
				return (-1);
			}

			/*
			 * If the media is set to either Tape or Diskette
			 * then prompt the user to load the media
			 */

			switch (BACKUPMEDIA(prop)) {
			case DSRALTape:
				write_status(LOGSCR, LEVEL0,
				    MSG1_INSERT_FIRST_MEDIA,
				    MSG0_TAPE_MEDIA);

				(void) gets(Buffer);
				break;
			case DSRALFloppy:
				write_status(LOGSCR, LEVEL0,
				    MSG1_INSERT_FIRST_MEDIA,
				    MSG0_DISKETTE_MEDIA);

				(void) gets(Buffer);
				break;
			default:
				break;
			}

			write_status(LOGSCR, LEVEL0,
			    MSG0_VALIDATING_MEDIA);

			/*
			 * Set the media for the archive
			 */

			if ((DSRALError = DSRALSetMedia(ArchiveList,
			    SliceList,
			    BACKUPMEDIA(prop),
			    MEDIAPATH(prop))) != DSRALSuccess) {
				write_notice(ERRMSG,
				    DSRALGetErrorText(DSRALError));
				return (-1);
			}

			/*
			 * Generate the list of files to archive.
			 */

			if ((DSRALError = DSRALGenerate(ArchiveList,
			    SliceList,
			    ArchiveHandler,
			    NULL,
			    &BytesToTransfer))) {
				write_notice(ERRMSG,
				    DSRALGetErrorText(DSRALError));
				return (-1);
			}

			/*
			 * Check to see if the media is large enough to
			 * hold the archive.
			 */

			if ((DSRALError = DSRALCheckMediaSpace(ArchiveList))) {
				write_notice(ERRMSG,
				    DSRALGetErrorText(DSRALError));
				return (-1);
			}

			/*
			 * Destroy the ArchiveList Object.
			 */

			if ((DSRALError = DSRALDestroy(&ArchiveList)))	{
				write_notice(ERRMSG,
				    DSRALGetErrorText(DSRALError));
				return (-1);
			}

			if (LLClearList(SliceList, SLClearCallback)) {
				write_notice(ERRMSG,
				    MSG0_LIST_MANAGEMENT_ERROR);
				return (-1);
			}

			if (LLDestroyList(&SliceList, NULL)) {
				write_notice(ERRMSG,
				    MSG0_LIST_MANAGEMENT_ERROR);
				return (-1);
			}
		}
	}
	return (Operation);
}


/*
 * FUNCTION NAME: PFU_SetupSliceList
 *
 * DESCRIPTION:
 *  This function takes in the user's profile and makes the appropriate
 *  selections into the internal data models slice list to reflect
 *  the provided layout_constraints.
 *
 * RETURN:
 *  TYPE			DESCRIPTION
 *  int				 0 = Success
 *				-1 = Failure
 *
 * PARAMETERS:
 *  TYPE			DESCRIPTION
 *  Profile *			The profile structure generated by
 *				parse_profile().
 *  TList			The linked list that represents all
 *				of the slices that are contained on the
 *				system.  (e.g. both slices listed in
 *				the vfstab and those slices not listed
 *				in the vfstab)
 *
 */

static int
PFU_SetupSliceList(Profile *prop, TList SliceList)
{
	TSLEntry		*SLEntry;
	TLink			CurrentLink;
	TLLError		LLError;
	LayoutConstraint	*lc;

	TBoolean		Validated;

	/*
	 * Set the allowed states for the slice list
	 */

	DsrSLSetDefaults(SliceList);

	/*
	 * Ok, now loop though the list of layout constraints and make sure
	 * that the user has specifically selected all of the failed slices.
	 */

	LL_WALK(SliceList, CurrentLink, SLEntry, LLError) {

		/*
		 * If the slice is a space limited file system
		 */

		Validated = False;
		if (SL_SLICE_HAS_INSUFF_SPACE(SLEntry)) {
			lc = LAYOUTCONSTRAINT(prop);

			/*
			 * Loop through all of the layout constraints
			 * provided in the profile
			 */

			while (lc) {

				/*
				 * If the SliceName and the layout constraint
				 * name match
				 */

				if (ci_streq(SLEntry->SliceName,
				    LCDEVNAME(lc))) {

					/*
					 * Check to see if the state
					 * provided in the profile is
					 * allowed
					 */

					if (!(SLEntry->AllowedStates &
					    LCSTATE(lc))) {
						write_notice(ERRMSG,
						    MSG2_INVALID_LAYOUT_CONSTRAINT,
						    SLStateToString(LCSTATE(lc)),
						    SLEntry->SliceName);
						return (-1);

					/*
					 * If a size was provided then check
					 * to make sure that it is greater than
					 * the required size
					 */

					} else if (LCSIZE(lc) &&
					    LCSIZE(lc) < SLEntry->Size) {
						write_notice(ERRMSG,
						    MSG2_INVALID_LAYOUT_CONSTRAINT_SIZE,
						    SLEntry->SliceName,
						    SLEntry->Size);
						return (-1);
					} else {
						Validated = True;
						break;
					}
				}
				lc = LCNEXT(lc);
			}

			/*
			 * If we could not validate the profile entry
			 */

			if (!Validated) {
				write_notice(ERRMSG,
				    MSG1_MUST_PROVIDE_LAYOUT_CONSTRAINT,
				    SLEntry->SliceName);
				return (-1);
			}
		}
	}
	if (LLError != LLSuccess && LLError != LLEndOfList) {
		write_notice(ERRMSG,
		    MSG0_LIST_MANAGEMENT_ERROR);
		return (-1);
	}

	/*
	 * Now, if a layout constraint was provided, loop through all of them
	 * and set their information into the slice list.
	 */

	for (lc = LAYOUTCONSTRAINT(prop); lc != NULL; lc = LCNEXT(lc)) {

		/*
		 * Now walk the slice list looking for a match on slice
		 * name (e.g. c0t3d0s0)
		 */

		LL_WALK(SliceList, CurrentLink, SLEntry, LLError) {
			if (ci_streq(SLEntry->SliceName, LCDEVNAME(lc))) {

				/*
				 * Check to see if the given state is allowed
				 */

				if (!(SLEntry->AllowedStates & LCSTATE(lc))) {

					/*
					 * The assigned state is not allowed for
					 * the slice
					 */

					write_notice(ERRMSG,
					    MSG2_INVALID_LAYOUT_CONSTRAINT,
					    SLStateToString(LCSTATE(lc)),
					    SLEntry->SliceName);

					return (-1);
				}

				SLEntry->State = LCSTATE(lc);

				/*
				 * If a size was provided
				 */

				if (LCSIZE(lc)) {
					SLEntry->Size = LCSIZE(lc);
				}
				break;
			}
		}
		if (LLError != LLSuccess && LLError != LLEndOfList) {
			write_notice(ERRMSG,
			    MSG0_LIST_MANAGEMENT_ERROR);
			return (-1);
		}
	}
	return (0);
}

/*
 * FUNCTION NAME: CollpaseFileSystems
 *
 * DESCRIPTION:
 *  This function takes the user's profile and for each
 *  layout_constraint that is marked as collapsed sets the corresponding
 *  attribute in the space structure.  Once this is completed for all
 *  file systems being collapsed the file system space checking logic
 *  is run to determine the space requirements based on the new
 *  file system layout.
 *
 * RETURN:
 *  TYPE			DESCRIPTION
 *  (void)
 *
 * PARAMETERS:
 *  TYPE			DESCRIPTION
 *  Profile *			The profile structure generated by
 *				parse_profile().
 *  FSspace **			The double pointer to the space
 *				structure generated by the call to
 *				get_current_fs_layout().
 *
 */

static void
PFU_CollapseFileSystems(Profile *prop, FSspace **Space)
{
	LayoutConstraint	*lc;
	int 			i;

	/*
	 * Now loop through the given layout constraints and
	 * look for the collapse identifier.  For all file
	 * systems set to collapse I have to set the file
	 * system to ignore so that the file system space will
	 * get rolled up into the parent file system
	 */

	for (lc = LAYOUTCONSTRAINT(prop); lc != NULL; lc = LCNEXT(lc)) {
		if (LCSTATE(lc) == SLCollapse) {
			for (i = 0; Space && Space[i]; i++) {
				if (streq(basename(
					Space[i]->fsp_fsi->fsi_device),
				    LCDEVNAME(lc))) {
					Space[i]->fsp_flags |=
						FS_IGNORE_ENTRY;
				}
			}
		}
	}

	/*
	 * Now that the file systems have been collapsed re-run the verify
	 * routine to get the required file systems sizes based on the
	 * colapsed file systems.  Note, I do not listen to the return
	 * code here because, even if the verify were successful at this
	 * point I don't care since the only way to migrate the contents
	 * of the collapsed file systems is to proceed throught the DSR path.
	 */

	(void) verify_fs_layout(Space, SpaceCheckingHandler, NULL);
}

/*
 * Function:	_catch_sig
 * Description: Signal catcher which exits when execution simulation
 *		or internal tracing are active.
 * Scope:	private
 * Parameters:	sig	[RO] (int)
 *			Signal number to be caught.
 * Return:	none
 */
static void
_catch_sig(int sig)
{
	if (GetSimulation(SIM_EXECUTE) || get_trace_level() > 0)
		fatal_exit(MSG1_SIGNAL_RECEIVED, sig);
}

/*
 * verify_bootdisk()
 *	Determine if the bootdisk has changed from the one originally
 *	configured as the default. If so, print out a warning message
 *	and return with a failure. Eventually, this is where the
 *	PROM on SPARC systems would be modified. This test is not
 *	required on Autoclient systems as of 2.5.1.
 * Parameters:
 *	none
 * Return:
 *	 0 	- the default boot disk is the same as the configured
 *		  boot disk
 *	-1	- the default boot disk differs from the configured
 *		  boot disk
 */
static int
verify_bootdisk(void)
{
	int	slice;
	Disk_t  *dp;
	char	*promdev;
	char	*device;
	int	prom_update;
	int	prom_updateable;

	/* Autoclient systems don't need this check as of 2.5.1 */
	if (get_machinetype() == MT_CCLIENT)
		return (0);

	if (BootobjCompare(CFG_CURRENT, CFG_EXIST, 0) != D_OK) {
		(void) BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DEVICE, &slice,
			BOOTOBJ_PROM_UPDATEABLE, &prom_updateable,
			BOOTOBJ_PROM_UPDATE, &prom_update,
			NULL);
		if (!prom_updateable || !prom_update) {
			(void) DiskobjFindBoot(CFG_CURRENT, &dp);
			if (IsIsa("sparc")) {
				promdev = MSG0_PROM_EEPROM;
				device = make_slice_name(disk_name(dp), slice);
			} else if (IsIsa("i386")) {
				promdev = MSG0_PROM_BIOS;
				device = disk_name(dp);
			} else {
				promdev = MSG0_PROM_FIRMWARE;
				device = disk_name(dp);
			}

			/* i18n: #1 %s -> MSG0_PROM_*   #2 %s -> disk/slice name */
			write_status(LOGSCR, LEVEL0,
				MSG2_RECONFIG_PROM_INTRO,
				promdev, device);

			return (-1);
		}
	}

	return (0);
}

/*
 * verify_disk_config()
 *	Verify the configured layout to ensure it provides sufficient
 *	space for all system resource requirements. This routine sets
 *	the library error message chain.
 * Parameters:
 *	prop	- pointer to profile structure
 *	status	[RO] (int)
 *		Installation execution status (D_*).
 * Return:
 *	# >= 0	- number of fatal errors
 * Status:
 *	private
 */
static int
verify_disk_config(Profile *prop, int status)
{
	int		error = 0;
	Errmsg_t	*emp;

	/* validate parameters */
	if (prop == NULL)
		return (D_BADARG);

	write_status(LOGSCR, LEVEL0, MSG0_VERIFY_DISK_CONFIG);

	if (check_disks() > 0) {
		WALK_LIST(emp, get_error_list()) {
			if (emp->code < 0) {
				error++;
				write_status(LOGSCR, LEVEL1|LISTITEM,
					"%s%s",
					MSG_STD_ERROR,
					emp->msg);
			} else  {
				if (status == D_OK) {
					write_status(LOGSCR, LEVEL1|LISTITEM,
						"%s%s",
						MSG_STD_WARNING,
						emp->msg);
				}
			}
		}
	}

	return (error);
}

/*
 * verify_space_config()
 * 	Verify that there is sufficient storage space allocated to
 *	meet all the system resource requirements.
 * Parameters:
 *	prop	- pointer to profile structure
 * Return:
 *	# >= 0	- number of insufficient resource allocations
 * Status:
 *	private
 */
static int
verify_space_config(Profile *prop)
{
	Space *	swstatus;
	int	error = 0;

	/* validate parameters */
	if (prop == NULL)
		return (0);

	write_status(LOGSCR, LEVEL0, MSG0_VERIFY_SPACE_CONFIG);

	if (DISKPARTITIONING(prop) == LAYOUT_DEFAULT) {
		swstatus = ResobjIsComplete(RESSIZE_DEFAULT);
	} else {
		swstatus = ResobjIsComplete(RESSIZE_MINIMUM);
	}

	if (swstatus != NULL) {
		print_space_layout(swstatus, DISKPARTITIONING(prop));
		error++;
	}

	/*
	 * print out the total software size if there were no
	 * errors
	 */
	if (error == 0)
		software_print_size();

	return (error);
}

/*
 * _usage()
 *	Print the usage line and exit in error.
 * Parameters:
 *	progname	- name of calling program
 * Return:
 *	none
 */
static void
_usage(char *progname)
{
	write_status(SCR, LEVEL0, MSG1_USAGE, progname);
	exit(0);
}

/*
 * _cacheos_create_swapfile()	(OBSOLETE FOR 2.5)
 *	Determine how much swap needs to be supplied by
 *	/.cache/swap and create the file accordingly.
 * Parameters:
 *	none
 * Return:
 *	1	- creation unnecessary
 *	0	- creation necessary and successful
 *	-1	- creation necessary but unsuccessful
 */
static int
_cacheos_create_swapfile(void)
{
	char	cmd[64];
	int	size;
	int	actual;

	if (get_machinetype() != MT_CCLIENT)
		return (1);

	if (ResobjFind(CACHESWAP, 0) == NULL)
		return (1);

	size = ResobjGetSwap(RESSIZE_DEFAULT) -
			SliceobjSumSwap(NULL, SWAPALLOC_ALL);

	/*
	 * swap slices are more than ample to meet swap requirements,
	 * so /.cache/swap is not necessary
	 */
	if (size <= 0)
		return (1);

	/*
	 * /.cache/swap is needed, so try to create it and return
	 * according to the success of the creation
	 */
	if (size < 5)
		actual = 5;
	else
		actual = sectors_to_mb(size);

	write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_SWAPFILE_CREATE);
	write_status(SCR, LEVEL1|LISTITEM,
		MSG1_TRACE_CACHEOS_SWAP,
		actual);

	if (!GetSimulation(SIM_EXECUTE)) {
		(void) sprintf(cmd,
			"/usr/sbin/mkfile %dm %s >/dev/null 2>&1",
			actual, CACHESWAP);
		if (system(cmd) != 0) {
			return (-1);
		}
	}

	if (_cacheos_add_vfstab_swap() < 0)
		return (-1);

	return (0);
}

/*
 * _cacheos_add_vfstab_swap()	(OBSOLETE FOR 2.5)
 *	Add a vfstab entry for /.cache/swap to the /etc/vfstab file.
 * Parameters:
 *	none
 * Return:
 *	0	- add of entry successful
 *	-1	- add of entry failed
 */
static int
_cacheos_add_vfstab_swap(void)
{
	struct vfstab	vfs;
	FILE		*fp;

	if (get_machinetype() != MT_CCLIENT)
		return (0);

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG0_SWAPFILE_CREATE_ENTRY);
	vfsnull(&vfs);
	vfs.vfs_special = xstrdup(CACHESWAP);
	vfs.vfs_fstype = MNTTYPE_SWAP;

	(void) write_status(LOGSCR, LEVEL1|LISTITEM|CONTINUE,
		gettext("Swap file vfstab entry:"));

	(void) write_status(LOGSCR, LEVEL1|LISTITEM|CONTINUE,
		"%s\t%s\t%s\t%s\t%s\t%s\t%s",
		vfs.vfs_special ? vfs.vfs_special : "-", \
		vfs.vfs_fsckdev ? vfs.vfs_fsckdev : "-", \
		vfs.vfs_mountp ? vfs.vfs_mountp : "-", \
		vfs.vfs_fstype ? vfs.vfs_fstype : "-", \
		vfs.vfs_fsckpass ? vfs.vfs_fsckpass : "-", \
		vfs.vfs_automnt ? vfs.vfs_automnt : "-", \
		vfs.vfs_mntopts ? vfs.vfs_mntopts : "-");

	if (GetSimulation(SIM_EXECUTE))
		return (0);

	if ((fp = fopen(VFSTAB, "a")) == NULL)
		return (-1);

	(void) putvfsent(fp, &vfs);
	(void) fclose(fp);
	return (0);
}
