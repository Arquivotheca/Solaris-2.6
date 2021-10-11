#ifndef lint
#pragma ident "@(#)pf_software.c 1.25 96/08/15"
#endif
/*
 * Copyright (c) 1994-1996 Sun Microsystems, Inc. All Rights Reserved.
 */
#include <string.h>
#include "spmisvc_api.h"
#include "spmistore_api.h"
#include "spmisoft_api.h"
#include "profile.h"
#include "pf_strings.h"

/* Local Variables */

static Module		*mod;
static int		remaining_kb = 0;

/* public prototypes */

int			configure_software(Profile *);
int			configure_media(Profile *);
void			software_print_size(void);
int			software_load(Profile *);
int 			ScriptHandler(void *UserData, void *SpecificData);
int 			SpaceCheckingHandler(void *UserData,
				void *SpecificData);
int 			ArchiveHandler(void *UserData, void *SpecificData);
int 			SoftUpdateHandler(void *UserData, void *SpecificData);

/* Local Function Prototypes */

static int		_configure_locales(Profile *);
static int		_configure_client_archs(Profile *);
static int		_match_module(Node *, caddr_t);
static int		_match_package(Node *, caddr_t);
static char *		_pkgid_from_pkgdir(char *);
static ModStatus 	_toggle_package(Modinfo *);
static void		_print_package_list(Module *);
static int		_print_package(Node *, caddr_t);
static void		_print_package_depends(Profile *);
static uint		_get_total_kb_to_install(void);
static void		_select_native_arch(void);
static Module *		_findpkgmod(Module *mod, char *name);
static int		_mark_v0_cluster(Profile *prop, Sw_unit *tmp);
static int		_mark_v0_package(Sw_unit *tmp);
static void		_v0_conf(Profile *prop);

/* ******************************************************************** */
/* 				PUBLIC FUNCTIONS			*/
/* ******************************************************************** */
/*
 * Function:	configure_software
 * Description:	Configure the software specified in pfinstall.
 * Scope:	public
 * Parameters:	prop	  - pointer to profile structure
 * Return:	D_OK	  - software configuration successful
 *		D_BADARG  - invalid argument
 *		D_FAILED  - software configuration failed
 */
int
configure_software(Profile *prop)
{
	/* validate parameters */
	if (prop == NULL)
		return (D_BADARG);

	/*
	 * CacheOS systems do not have software
	 */
	if (SYSTYPE(prop) == MT_CCLIENT)
		return (D_OK);

	/*
	 * customize the software tree for server configurations
	 */
	if (SYSTYPE(prop) == MT_SERVER) {
		set_action_for_machine_type(SWPRODUCT(prop));
		set_client_space(CLIENTCNT(prop), CLIENTROOT(prop),
			CLIENTSWAP(prop));
	}

	/*
	 * set up free space in fs for auto sizing - explicit
	 * and existing partitioning should have 0 required
	 * free space, while all others should have the default
	 * ( upgrade always sets this to 1 )
	 */
	if (ISOPTYPE(prop, SI_UPGRADE)) {
		sw_lib_init(PTYPE_UNKNOWN);
		set_percent_free_space(1);

	} else {	/* SI_INITIAL_INSTALL */

		sw_lib_init(NULL);
		if (DISKPARTITIONING(prop) == LAYOUT_DEFAULT)
			set_percent_free_space(DEFAULT_FS_FREE);
		else
			set_percent_free_space(NO_EXTRA_SPACE);

		/*
		 * select the native architecture of the system
		 */
		_select_native_arch();

		/*
		 * process the metacluster, all clusters, and all specified
		 * packages. This must occur before locales are processed.
		 */
		WALK_LIST(mod, SWPRODUCT(prop)->sub) {
			if (streq(mod->info.mod->m_pkgid, REQD_METACLUSTER)) {
				mark_required(mod);
				set_current(mod);
				break;
			}
		}

		WALK_LIST(mod, SWPRODUCT(prop)->sub) {
			if (streq(mod->info.mod->m_pkgid, METACLUSTER(prop))) {
				mod->info.mod->m_status = SELECTED;
				mark_module(mod, SELECTED);
				set_current(mod);
				set_default(mod);
				break;
			}
		}

		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG1_CLUSTER_SELECT,
			METACLUSTER(prop));
	}

	/*
	 *
	 * Flag clusters and packages to install/upgrade.  Due to different
	 * semantics between the original pfupgrade and pfinstall programs,
	 * there are two different ways to do this.
	 *
	 * The v0 refers to 'Version 0' of the profile grammar, expected
	 * to be expanded to other versions at a later date.
	 *
	 */
	switch (PROVERSION(prop)) {
	case PROFILE_VER_0:
		_v0_conf(prop);
		break;
	default:
		return (D_FAILED);
	}

	/*
	 * set the m_status on localization packages based on the locale
	 * list and the state of "affected" packages
	 */
	if (_configure_locales(prop) != D_OK)
		return (D_FAILED);

	if (SYSTYPE(prop) == MT_SERVER &&
			_configure_client_archs(prop) != D_OK)
		return (D_FAILED);

	_print_package_depends(prop);

	/*
	 * print out a list of the packages if running in dry-run
	 * or with level 3 or higher tracing
	 */
	if (GetSimulation(SIM_EXECUTE) || get_trace_level() > 2) {
		write_message(LOGSCR, STATMSG, LEVEL0,
		    dgettext("SUNW_INSTALL_LIBSVC",
		    "Packages to be installed"));
		_print_package_list(SWPRODUCT(prop));
	}

	return (D_OK);
}

/*
 * software_load()
 *	Customize the software tree to reflect profile specifications.
 *	The media should already be loaded before this routine is
 *	called. No software customization applies to CacheOS clients.
 * Parameters:
 *	prop	  - pointer to profile structure
 * Return:
 *	D_OK	  - successful software load
 *	D_BADARG  - invalid parameters
 *	D_FAILED  - software load failed
 * Status:
 *	public
 */
int
software_load(Profile *prop)
{
	Module		*cmeta;
	Sw_unit		**prev;
	Sw_unit		*p;
	int		advance = 1;

	/* validate parameters */
	if (prop == NULL)
		return (D_BADARG);

	/* cache client systems don't have software */
	if (SYSTYPE(prop) == MT_CCLIENT)
		return (D_OK);

	/*
	 * if the user has specified more than one metacluster in
	 * the profile, throw out all but the first one specified
	 */
	prev = &UNITS(prop);
	WALK_LIST(p, UNITS(prop)) {
		if (p->unit_type != CLUSTER)
			continue;
		WALK_LIST(cmeta, get_current_metacluster()) {
			if (streq(p->name, cmeta->info.mod->m_pkgid)) {
				if (METACLUSTER(prop) == NULL)
					METACLUSTER(prop) = p->name;
				else {
					write_notice(WARNMSG,
						MSG1_SOFTWARE_META_DUPLICATE,
						p->name);
				}

				/* remove meta-cluster from list */
				(*prev) = p->next;
				advance = 0;
				break;
			}
		}

		if (advance)
			prev = &p->next;
	}

	/* if no metacluster is selected, pick ENDUSER */
	if (METACLUSTER(prop) == NULL)
		METACLUSTER(prop) = ENDUSER_METACLUSTER;

	/* update the software tree and initialized service medias */
	set_instdir_svc_svr(SWPRODUCT(prop));

	return (D_OK);
}

/*
 * Function:	ScriptHandler
 * Description: This function is the status update callback called by the
 *		Upgrade Script  to provide progress status to the front end.
 * Scope:	private
 * Parameters:
 *		void *		The processing specific data pointer.
 *		void *		A void pointer to the processing state's
 *				specific data.
 * Return:
 *		int		0 	= Success
 *				not 0 	= Failure
 */
int
ScriptHandler(void *UserData, void *SpecificData)
{
	ValProgress *Progress;
	char		Buffer[PATH_MAX];

	/*
	 * Keep lint from complaining
	 */

	UserData = UserData;

	Progress = (ValProgress *)SpecificData;

	switch (Progress->valp_stage) {

	/*
	 * If we are beginning or ending then kick out a line feed
	 */

	case VAL_UPG_BEGIN:
		write_status(SCR, LEVEL0, "");
		break;
	case VAL_UPG_END:
		write_status(SCR, LEVEL0, "");
		write_status(SCR, LEVEL0, "");
		break;
	default:
		(void) sprintf(Buffer, "%s\r",
		    MSG1_UPGRADE_SCRIPT_PERCENT_COMPLETE);
		write_status(SCR, FMTPARTIAL, Buffer,
		    Progress->valp_percent_done);
		break;
	}

	return (0);
}

/*
 * Function:	SpaceCheckingHandler
 * Description: This function is the status update callback called by the
 *		Space Checking algorithm to provide progress status to the
 *		front end.
 * Scope:	private
 * Parameters:
 *		void *		The processing specific data pointer.
 *		void *		A void pointer to the processing state's
 *				specific data.
 * Return:
 *		int		0 	= Success
 *				not 0 	= Failure
 */
int
SpaceCheckingHandler(void *UserData, void *SpecificData)
{
	ValProgress 	*Progress;
	char		Buffer[PATH_MAX];

	/*
	 * Keep lint from complaining
	 */

	UserData = UserData;

	Progress = (ValProgress *)SpecificData;

	switch (Progress->valp_stage) {

	/*
	 * If we are beginning or ending then kick out a line feed
	 */

	case VAL_ANALYZE_BEGIN:
		write_status(SCR, LEVEL0, "");
		break;
	case VAL_ANALYZE_END:
		write_status(SCR, LEVEL0, "");
		write_status(SCR, LEVEL0, "");
		break;
	default:
		(void) sprintf(Buffer, "%s\r",
		    MSG1_SPACE_CHECKING_PERCENT_COMPLETE);
		write_status(SCR, FMTPARTIAL,
		    Buffer, Progress->valp_percent_done);
	}

	return (0);
}

/*
 * Function:	ArchiveHandler
 * Description: This function is the status update callback called by the
 *		DSRAL Object to provide progress status to the front end.
 * Scope:	private
 * Parameters:
 *		void *		The processing specific data pointer.
 *		void *		A void pointer to the processing state's
 *				specific data.
 * Return:
 *		int		0 	= Success
 *				not 0 	= Failure
 */
int
ArchiveHandler(void *UserData, void *SpecificData)
{
	char Buffer[PATH_MAX];
	TDSRALStateData *StateData;

	/*
	 * Keep lint from complaining
	 */

	UserData = UserData;

	StateData = (TDSRALStateData *)SpecificData;

	switch (StateData->State) {
	case DSRALNewMedia:
		write_status(SCR, LEVEL0, "");
		write_status(SCR, LEVEL0, "");

		switch (StateData->Data.NewMedia.Media) {
		case DSRALTape:
			write_status(SCR, LEVEL0, MSG2_REPLACE_MEDIA,
			    MSG0_TAPE_MEDIA,
			    StateData->Data.NewMedia.MediaNumber);
			break;
		case DSRALFloppy:
			write_status(SCR, LEVEL0, MSG2_REPLACE_MEDIA,
			    MSG0_DISKETTE_MEDIA,
			    StateData->Data.NewMedia.MediaNumber);
			break;
		default:
			break;
		}
		(void) gets(Buffer);
		break;
	case DSRALBackupBegin:
		switch (StateData->Data.BackupBegin.Media) {
		case DSRALFloppy:
			(void) sprintf(Buffer, "%s",
			    MSG1_BACKUP_DISKETTE_BEGIN);
			break;
		case DSRALTape:
			(void) sprintf(Buffer, "%s", MSG1_BACKUP_TAPE_BEGIN);
			break;
		case DSRALDisk:
			(void) sprintf(Buffer, "%s", MSG1_BACKUP_DISK_BEGIN);
			break;
		case DSRALRsh:
			(void) sprintf(Buffer, "%s", MSG1_BACKUP_RSH_BEGIN);
			break;
		case DSRALNFS:
			(void) sprintf(Buffer, "%s", MSG1_BACKUP_NFS_BEGIN);
			break;
		}
		write_status(SCR, LEVEL0, Buffer,
		    StateData->Data.BackupBegin.MediaString);
		break;
	case DSRALBackupUpdate:
		(void) sprintf(Buffer, "%s\r", MSG1_BACKUP_PERCENT_COMPLETE);
		write_status(SCR, FMTPARTIAL, Buffer,
		    StateData->Data.FileUpdate.PercentComplete);
		break;
	case DSRALBackupEnd:
		write_status(SCR, LEVEL0, "");
		write_status(SCR, LEVEL0, MSG0_BACKUP_END);
		break;
	case DSRALRestoreBegin:
		switch (StateData->Data.BackupBegin.Media) {
		case DSRALFloppy:
			(void) sprintf(Buffer, "%s",
			    MSG1_RESTORE_DISKETTE_BEGIN);
			break;
		case DSRALTape:
			(void) sprintf(Buffer, "%s", MSG1_RESTORE_TAPE_BEGIN);
			break;
		case DSRALDisk:
			(void) sprintf(Buffer, "%s", MSG1_RESTORE_DISK_BEGIN);
			break;
		case DSRALRsh:
			(void) sprintf(Buffer, "%s", MSG1_RESTORE_RSH_BEGIN);
			break;
		case DSRALNFS:
			(void) sprintf(Buffer, "%s", MSG1_RESTORE_NFS_BEGIN);
			break;
		}
		write_status(SCR, LEVEL0,
		    Buffer, StateData->Data.RestoreBegin.MediaString);
		break;
	case DSRALRestoreUpdate:
		(void) sprintf(Buffer, "%s\r",
		    MSG1_RESTORE_PERCENT_COMPLETE);
		write_status(SCR, FMTPARTIAL, Buffer,
		    StateData->Data.FileUpdate.PercentComplete);
		break;
	case DSRALRestoreEnd:
		write_status(SCR, LEVEL0, "");
		write_status(SCR, LEVEL0, MSG0_RESTORE_END);
		break;
	case DSRALGenerateBegin:
		write_status(SCR, LEVEL0, "");
		write_status(SCR, LEVEL0, MSG0_GENERATE_BEGIN);
		break;
	case DSRALGenerateUpdate:
		(void) sprintf(Buffer, "%s\r",
		    MSG1_GENERATE_PERCENT_COMPLETE);
		write_status(SCR, FMTPARTIAL, Buffer,
		    StateData->Data.GenerateUpdate.PercentComplete);
		break;
	case DSRALGenerateEnd:
		write_status(SCR, LEVEL0, "");
		write_status(SCR, LEVEL0, "");
		write_status(SCR, LEVEL0, MSG0_GENERATE_END);
		break;
	}
	return (0);
}

/*
 * Function:	SoftUpdateHandler
 * Description: This function is the status update callback called by the
 *		software update processing during an initial install.
 * Scope:	private
 * Parameters:
 *		void *		The processing specific data pointer.
 *		void *		A void pointer to the processing state's
 *				specific data.
 * Return:
 *		int		0 	= Success
 *				not 0 	= Failure
 */
int
SoftUpdateHandler(void *UserData, void *SpecificData)
{
	Module	*prod = get_current_product();
	List	*pkgs = prod->info.prod->p_packages;
	char	*pkgid;
	Node	*tmp;
	int	size = 0;
	float	fsize = 0.0;

	int i;

	TSoftUpdateStateData *StateData;

	/*
	 * Keep lint from complaining
	 */

	UserData = UserData;

	StateData = (TSoftUpdateStateData *)SpecificData;

	switch (StateData->State) {
	case SoftUpdateBegin:
		write_status(SCR, LEVEL0, MSG0_SOFTWARE_UPDATE_BEGIN);
		break;
	case SoftUpdatePkgAddBegin:
		if (StateData->Data.PkgAddBegin.PkgDir == NULL)
			break;

		write_status(SCR, LEVEL1|CONTINUE|FMTPARTIAL,
		    StateData->Data.PkgAddBegin.PkgDir);

		for (i = strlen(StateData->Data.PkgAddBegin.PkgDir);
		    i < 12;
		    i++)
			write_status(SCR, LEVEL0|CONTINUE|FMTPARTIAL, ".");

		break;
	case SoftUpdatePkgAddEnd:

		if (StateData->Data.PkgAddBegin.PkgDir == NULL)
			break;

		pkgid = (char *)_pkgid_from_pkgdir
			(StateData->Data.PkgAddBegin.PkgDir);
		tmp = findnode(pkgs, pkgid);
		if (tmp != NULL && tmp->data != NULL)
			size = tot_pkg_space((Modinfo *)tmp->data);

		remaining_kb -= size;
		if (remaining_kb > 0)
			fsize = (float)remaining_kb / 1024.0;

		/* log completion to screen only */
		write_status(SCR, LEVEL0|CONTINUE,
		    MSG1_SOFTWARE_END_PKGADD, fsize);

		break;
	case SoftUpdateInteractivePkgAdd:
		write_status(SCR, LEVEL0,
		    MSG0_SOFTWARE_UPDATE_INTERACTIVE_PKGADD);
		fatal_exit(NULL);
		break;
	case SoftUpdateEnd:
		write_status(SCR, LEVEL0, MSG0_SOFTWARE_UPDATE_END);
		break;
	}
	return (0);
}

/* ******************************************************************** */
/* 				PRIVATE FUNCTIONS			*/
/* ******************************************************************** */

/*
 * _pkgid_from_pkgdir()
 *	Extract the package id from the package directory name.
 *	If the pkgid has an extension (e.g. XXX.[cmde]), the
 *	extension is removed.
 *
 *	NOTE:	this code is duplicated in the cui and gui and
 *		should be put in the library
 * Parameters:
 *	path	- string containing directory path name
 * Return:
 *	char *	- pointer to local buffer
 * Status:
 *	public
 */
static char *
_pkgid_from_pkgdir(char *path)
{
	static char	buf[64];
	char		*cp;

	buf[0] = '\0';

	/* validate prameters */
	if (path == NULL)
		return (buf);

	/*
	 * path = "/blah/blah/blah/pkgdir[.{c|e|d|m|...}]
	 *						 ^
	 *						 |
	 * cp	--------------------+
	 */

	/* put into private buffer, and trim off any extension */

	if (path && (cp = strrchr(path, '/')))
		(void) strcpy(buf, cp + 1);
	else
		(void) strcpy(buf, path);

	if (cp = strrchr(buf, '.'))
		*cp = '\0';

	return (buf);
}

/*
 * configure_media()
 *	Initialize the software library with the specified media.
 * Parameters:
 *	prop	- pointer to profile structure containing
 *		  media specification
 * Return:
 *	D_OK	  - media configured successfully
 *	D_BADARG  - invalid argument
 *	D_FAILED  - media configuration failed
 * Status:
 *	public
 */
int
configure_media(Profile *prop)
{
	int	status;

	/* validate parameters */
	if (prop == NULL)
		return (D_BADARG);

	/*
	 * do not load the media tree if the media specified on the
	 * command line is "none"
	 */
	if (strcmp(MEDIANAME(prop), "none") == 0)
		return (D_OK);

	/* load software modules */
	set_default(add_media(MEDIANAME(prop)));

	if ((status = load_media(NULL, TRUE)) != SUCCESS) {
		switch (status) {
		    case ERR_NOMEDIA:
			write_notice(ERRMSG, MSG0_MEDIA_NONE);
			return (D_FAILED);

		    case ERR_INVALIDTYPE:
			write_notice(ERRMSG,
				MSG1_MEDIA_INVALID, MEDIANAME(prop));
			return (D_FAILED);

		    case ERR_UMOUNTED:
			write_notice(ERRMSG,
				MSG1_MEDIA_UNMOUNT, MEDIANAME(prop));
			return (D_FAILED);

		    case ERR_NOPROD:
			write_notice(ERRMSG,
				MSG1_MEDIA_NOPRODUCT, MEDIANAME(prop));
			return (D_FAILED);

		    default:
			write_notice(ERRMSG,
				MSG1_MEDIA_LOAD_FAILED, MEDIANAME(prop));
			return (D_FAILED);
		}
	}

	/* if no product is selected, get the default one */
	if (SWPRODUCT(prop) == NULL)
		SWPRODUCT(prop) = get_current_product();

	return (D_OK);
}

/*
 * software_print_size()
 *	Print the total size estimate for software specified.
 * Parameters:
 *	none
 * Return:
 *	none
 * Status:
 *	public
 */
void
software_print_size(void)
{
	float	size = 0.0;

	remaining_kb = _get_total_kb_to_install();
	if (remaining_kb > 0)
		size = (float)remaining_kb / 1024.0;

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG1_SOFTWARE_TOTAL, size);
}

/* ******************************************************************** */
/*				 LOCAL FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _configure_locales()
 *	Select the locales. Use the default (if there is one), and any
 *	specified explicitly in the profile. Will fail if unable to select
 *	either specified or default locale.
 * Parameters:
 *	prop	 - pointer to profile structure
 * Return:
 *	D_OK	 - configuration of locales successful
 *	D_BADARG - invalid argument
 *	D_FAILED - locale configuration failed
 * Status:
 *	private
 */
static int
_configure_locales(Profile *prop)
{
	Modinfo		*mi;
	Namelist	*p;
	Sw_unit		*tmp;

	/* validate parameters */
	if (prop == NULL)
		return (D_BADARG);

	/* select all locales */
	WALK_LIST(p, LOCALES(prop)) {
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG1_LOCALE_SELECT,
			p->name);
		if (select_locale(SWPRODUCT(prop), p->name) != SUCCESS) {
			write_notice(ERRMSG,
				MSG1_LOCALE_SELECT_FAILED,
				p->name);
			return (D_FAILED);
		}
	}

	/*
	 * walk thru the list of modules again, resetting the state on
	 * localization packages only, as requested (silently)
	 */
	WALK_LIST(tmp, UNITS(prop)) {
		if (tmp->unit_type != PACKAGE)
			continue;

		if (walklist(SWPRODUCT(prop)->info.prod->p_packages,
				_match_package, (caddr_t)tmp->name) == 0)
			continue;

		mi = (Modinfo *)mod;
		/* process only localization packages */
		if (mi->m_loc_strlist == NULL)
			continue;

		if (mi->m_status != UNSELECTED && tmp->delta == UNSELECTED)
			(void) _toggle_package(mi);
		else if (mi->m_status == UNSELECTED && tmp->delta == SELECTED)
			(void) _toggle_package(mi);
	}

	return (D_OK);
}

/*
 * _match_module()
 *	Check to see if the package module package id provided matches
 *	the specified package id.
 * Parameters:
 *	np	- pointer to package module
 *	name	- name to which the package should be compared
 * Return:
 *	1	- the package id matches the specified name
 *	0	- the package id does nto match the specified name
 * Status:
 *	private
 */
static int
_match_module(Node *np, caddr_t name)
{
	Module	*mp;

	mp = (Module *) np->data;
	if (streq(mp->info.mod->m_pkgid, name)) {
		mod = mp;
		return (1);
	}

	return (0);
}

/*
 * _match_package()
 *	Check to see if the package modinfo package id provided matches
 *	the specified package id.
 * Parameters:
 *	np	- pointer to package module
 *	name	- name to which the package should be compared
 * Return:
 *	1	- the package id matches the specified name
 *	0	- the package id does nto match the specified name
 * Status:
 *	private
 */
static int
_match_package(Node *np, caddr_t name)
{
	Modinfo	* mp;

	mp = (Modinfo *) np->data;
	if (streq(mp->m_pkgid, name)) {
		mod = (Module *)mp;
		return (1);
	}

	return (0);
}

/*
 * _toggle_package
 *	Toggle the state of a package
 * Parameters:
 *	mi	- valid	(non-NULL) Modinfo pointer
 * Return:
 *	Resulting Modinfo status (see enumerated type definition for ModStatus)
 */
static ModStatus
_toggle_package(Modinfo *mi)
{
	/*
	 * mark module as SELECTED or UNSELECTED update reference counter if
	 * already selected...
	 */
	if (mi->m_status != REQUIRED) {
		if (mi->m_status == UNSELECTED) {
			mi->m_status = SELECTED;
			mi->m_refcnt++;
		} else {
			mi->m_status = UNSELECTED;
			mi->m_refcnt = 0;
		}
	}

	return (mi->m_status);
}

/*
 * _print_package_list()
 *	Print out the name and possibly the status (depending on
 *	trace level) of the packages associated with the product.
 * Parameters:
 *	prod	- pointer to a product structure whose packages are
 *		  to be printed
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_print_package_list(Module *prod)
{
	(void) walklist(prod->info.prod->p_packages, _print_package, NULL);
}

/*
 * _print_package()
 *	Print out the package name and package status for each package
 *	in the software library. Make sure not to print out NULLPKG
 *	packages as some of their fields may be NULL.
 * Parameters:
 *	np	- package modinfo node pointer
 *	dummy	- not used
 * Return:
 *	0	- always returns this value (required by walklist())
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
_print_package(Node *np, caddr_t dummy)
{
	Modinfo		*mp;
	int		count = 0;

	mp = (Modinfo *) np->data;

	/*
	 * print out the main package if it is not NULL
	 */
	if (mp->m_shared != NULLPKG) {
		if (get_trace_level() > 2) {
			write_status(LOGSCR,
				LEVEL1|LISTITEM|CONTINUE|FMTPARTIAL,
				"%-9s %d", mp->m_pkg_dir, mp->m_status);
			count++;
		} else if (mp->m_status > 0) {
			write_status(LOGSCR,
				LEVEL1|LISTITEM|CONTINUE|FMTPARTIAL,
				"%-9s", mp->m_pkg_dir);
			count++;
		}
	}

	/*
	 * walk the instances chain and print out each instance
	 * found in the list
	 */
	for (mp = next_inst(mp); mp; mp = next_inst(mp)) {
		if (mp->m_shared != NULLPKG) {
			if (get_trace_level() > 2) {
				write_status(LOGSCR,
					LEVEL1|LISTITEM|CONTINUE|FMTPARTIAL,
					"%-9s %d", mp->m_pkg_dir, mp->m_status);
				count++;
			} else if (mp->m_status > 0) {
				write_status(LOGSCR,
					LEVEL1|LISTITEM|CONTINUE|FMTPARTIAL,
					"%-9s", mp->m_pkg_dir);
				count++;
			}
		}
	}

	if (count > 0)
		write_status(LOGSCR, LEVEL0, "");

	return (0);
}

/*
 * _configure_client_archs()
 *	Update the software tree according to the client architectures
 *	specified in the profile. Invalid architectures are considered
 *	fatal.
 * Parameters:
 *	prop	 - pointer to profile structure
 * Return:
 *	D_OK	 - client architectures set successfully
 *	D_BADARG - invalid argument
 *	D_FAILED - set failed
 * Status:
 *	private
 */
static int
_configure_client_archs(Profile *prop)
{
	Namelist	*p;

	/* validate parameters */
	if (prop == NULL)
		return (D_BADARG);

	/*
	 * architectures have been validated
	 */
	WALK_LIST(p, CLIENTPLATFORM(prop)) {
		if (select_arch(SWPRODUCT(prop), p->name) != SUCCESS) {
			write_notice(ERRMSG,
				MSG1_PLATFORM_SELECT_FAILED,
				p->name);
			return (D_FAILED);
		}
	}

	mark_arch(SWPRODUCT(prop));

	return (D_OK);
}

/*
 * Function:	_print_package_depends
 * Description:	Check for software dependencies which do not exist. Print
 *		warning messages only in debug mode.
 * Scope:	private
 * Parameters:	prop	- pointer to profile structure
 * Return:	none
 */
static void
_print_package_depends(Profile *prop)
{
	Depend	*d;

	/* validate parameters */
	if (prop == NULL)
		return;

	/*
	 * print out dependency warning message when running in
	 * dryrun mode
	 */
	if (GetSimulation(SIM_EXECUTE) || get_trace_level() > 0) {
		if (check_sw_depends() != SUCCESS) {
			for (d = get_depend_pkgs(); d != NULL; d = d->d_next) {
				write_notice(WARNMSG,
					MSG2_PACKAGE_DEPEND,
					d->d_pkgid, d->d_pkgidb);
			}
		}
	}
}

/*
 * _get_total_kb_to_install()
 *	Determine the total size of the software being installed,
 *	expanded with the basic file system overhead.
 *
 *	NOTE:	same routine as used by the CUI
 *
 * Parameters:
 *	none
 * Return:
 *	total size of software to be installed, expanded with
 *	basic filesystem overhead
 * Status:
 *	private
 */
static uint
_get_total_kb_to_install(void)
{
	ResobjHandle	res;
	uint	sum;
	int	instance;
	char	name[MAXNAMELEN];
	int	size;

	/* add up Kbytes used in each file system */
	sum = 0;
	WALK_DIRECTORY_LIST(res) {
		if (ResobjGetAttribute(res,
				RESOBJ_NAME,		  name,
				RESOBJ_INSTANCE,	  &instance,
				RESOBJ_CONTENT_SOFTWARE, &size,
				NULL) != D_OK)
			continue;

		if (strneq(name, "/export", 7))
			continue;

		sum += sectors_to_kb(size);
	}

	return (sum);
}

/*
 * _select_native_arch()
 *	Select the default platform architecture for the system
 *	and make sure all packages affected by that architecture
 *	are updated in the software library. This must be run
 *	before explicit package selection/deselection.
 * Parameters:
 *	none
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_select_native_arch(void)
{
	Module  *prod = get_current_product();
	char	*arch = get_default_arch();

	select_arch(prod, arch);
	mark_arch(prod);
}



/*
 * _findpkgmod()
 *	finds the module that owns the package
 *
 * Parameters:	mod	[RW] - pointer to Module structure
 *		name	[RO] - pointer to name to search for
 *
 * Return:	mod 	- set to matching package or NULL on failure
 *
 * Status: 	private
 *
 * Note:	recursive
 *
 */
static Module *
_findpkgmod(Module *mod, char *name)
{
	Modinfo *mi;
	Module	*child;

	/*
	 * Do a depth-first search of the module tree, marking
	 * modules appropriately.
	 */
	mi = mod->info.mod;
	if ((mod->type == PACKAGE) && streq(mi->m_pkgid, name))
		return (mod);

	child = mod->sub;
	while (child) {
		mod = _findpkgmod(child, name);
		if (mod != NULL)
			return (mod);
		child = child->next;
	}
	return (NULL);
}


/*
 * _v0_conf()
 *	 Version 0 cluster and package flagging routine. Compatible with
 *	 original pfinstall and pfupgrade profile grammar parsing order.
 *
 * Parameters:	prop	- [RO] pointer to Profile structure
 *
 * Return:	none
 *
 * Status:	private
 *
 */
static void
_v0_conf(Profile *prop)
{
	Sw_unit		*tmp;
	int		res;

	/*
	 * walk thru the list of modules setting clusters as requested
	 *
	 * Note: the UNITS field contains the list of clusters
	 *	and packages in the order specified in the profile
	 *	and are differentiated by the tmp->unit_type field.
	 */
	if (ISOPTYPE(prop, SI_INITIAL_INSTALL)) {
		/* all clusters first */
		WALK_LIST(tmp, UNITS(prop)) {

			if (tmp->unit_type == CLUSTER)
				(void) _mark_v0_cluster(prop, tmp);

		}
		/* all packages second */
		WALK_LIST(tmp, UNITS(prop)) {

			if (tmp->unit_type == PACKAGE)
				(void) _mark_v0_package(tmp);

		}

	} else {	/* SI_UPGRADE */
		WALK_LIST(tmp, UNITS(prop)) {
			if (tmp->unit_type == CLUSTER)
				res = _mark_v0_cluster(prop, tmp);
			else
				res = _mark_v0_package(tmp);
			/*
			 * global 'mod' is set by _mark routines
			 */
			if (res == 0)
				update_action(mod);
		}
	}
}


/*
 * _mark_v0_cluster()
 *	Version 0 cluster flagging for both initial installs and upgrade.
 *
 * Parameters:	prop	- [RO] pointer to Profile structure
 *		tmp	- [RO] pointer to Sw_unit structure
 *
 * Returns:	-1	- unknown item, global variable 'mod' not set
 *		0	- marked OK, global variable 'mod' is set
 *
 * Status:	private
 */
static int
_mark_v0_cluster(Profile *prop, Sw_unit *tmp)
{
	if (walklist(SWPRODUCT(prop)->info.prod->p_clusters,
			_match_module, tmp->name) == 0) {
		write_notice(WARNMSG,
			MSG1_CLUSTER_UNKNOWN,
			tmp->name);
		return (-1);
	}
	/*
	 * global 'mod' has been set to point to the cluster Module
	 * by _match_module()
	 */
	if (tmp->delta == SELECTED) {
		mark_module(mod, SELECTED);
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG1_CLUSTER_SELECT,
			mod->info.mod->m_pkgid);
	} else {
		mark_module(mod, UNSELECTED);
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG1_CLUSTER_DESELECT,
			mod->info.mod->m_pkgid);
	}
	return (0);
}


/*
 * _mark_v0_package()
 *	Version 0 package flagging for both initial installs and upgrade.
 *
 * Parameters:	tmp	- [RO] pointer to Sw_unit structure
 *
 * Returns:	0	- unknown item, global variable 'mod' not set
 *		1	- marked OK, global variable 'mod' is set
 *
 * Status:	private
 */
static int
_mark_v0_package(Sw_unit *tmp)
{

	Modinfo		*mi;
	Module		*softmedia = get_default_media();

	if ((mod = _findpkgmod(softmedia, tmp->name)) == 0) {
		write_notice(WARNMSG,
			MSG1_PACKAGE_UNKNOWN,
			tmp->name);
		return (-1);
	}

	mi = mod->info.mod;
	if (tmp->delta == SELECTED) {
		if (mi->m_status != UNSELECTED) {
			write_notice(WARNMSG,
				MSG1_PACKAGE_SELECT_SELECT,
				mi->m_pkgid);
		} else {
			if (_toggle_package(mi) == SELECTED) {
				write_status(LOGSCR, LEVEL1|LISTITEM,
					MSG1_PACKAGE_SELECT,
					mi->m_pkgid);
			} else
				write_notice(WARNMSG,
					MSG1_PACKAGE_SELECT_FAILED,
					mi->m_pkgid);
		}
	} else if (tmp->delta == UNSELECTED) {
		if (mi->m_status == UNSELECTED) {
			write_notice(WARNMSG,
				MSG1_PACKAGE_DESELECT_DESELECT,
				mi->m_pkgid);
		} else if (mi->m_status == REQUIRED) {
			write_notice(WARNMSG,
				MSG1_PACKAGE_DESELECT_REQD,
				mi->m_pkgid);
		} else if (_toggle_package(mi) == UNSELECTED) {
			write_status(LOGSCR, LEVEL1|LISTITEM,
				MSG1_PACKAGE_DESELECT, mi->m_pkgid);
		} else {
			write_notice(WARNMSG,
				MSG1_PACKAGE_DESELECT_FAILED,
				mi->m_pkgid);
		}
	}
	return (0);
}
