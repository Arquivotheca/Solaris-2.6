#ifndef lint
#pragma ident "@(#)v_misc.c 1.89 96/09/03 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_misc.c
 * Group:	ttinstall
 * Description:
 */

#include <locale.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <termio.h>
#include <termios.h>
#include <signal.h>

#include "pf.h"
#include "v_types.h"
#include "v_misc.h"
#include "v_disk.h"
#include "v_lfs.h"
#include "v_rfs.h"
#include "v_sw.h"
#include "v_misc.h"

/* UI specific funcitons for going into and out of ``raw'' mode */

/* typedefs and defines ... */

/* structure definition ... */

/* private procedure decls ... */

#define	DEFAULT_CACHE_ONLY_CLIENTS	0

static char *_progname = (char *) NULL;
static char **_envp = (char **) NULL;
static int _n_d_clients = DEFAULT_NUMBER_OF_CLIENTS;
static int _diskless_swap = DEFAULT_SWAP_PER_CLIENT;
static int _root_size = DEFAULT_ROOT_PER_CLIENT;
static int _n_c_clients = DEFAULT_CACHE_ONLY_CLIENTS;

static int _reboot = TRUE;


/*
 * get default instruction set and implementation as char strings
 */
char *
v_get_default_impl(void)
{
	return (get_default_impl());
}

char *
v_get_default_inst(void)
{
	return (get_default_inst());
}

/* get/set reboot flag */
void
v_set_reboot(int val)
{
	_reboot = val;
}

int
v_get_reboot(void)
{
	return (_reboot);
}

/* set system type. */
void
v_set_system_type(V_SystemType_t type)
{

	extern void v_save_default_fs_table(V_SystemType_t);

	switch (type) {
	case V_STANDALONE:
		set_machinetype(MT_STANDALONE);
		v_save_default_fs_table(V_STANDALONE);
		break;

	case V_SERVER:
		set_machinetype(MT_SERVER);
		v_save_default_fs_table(V_SERVER);
		(void) v_set_n_diskless_clients(_n_d_clients);
		(void) v_set_diskless_swap(_diskless_swap);
		v_set_root_client_size(_root_size);
		break;

	default:
		set_machinetype(MT_STANDALONE);
		v_save_default_fs_table(V_STANDALONE);
		break;

	}

	/*
	 * this sets the `action' field on each software module to reflect
	 * the machinetype being installed.  Need to do this so that the
	 * space calculations are corrected for machinetype... (something in
	 * the software library copes with the underlying details.
	 */
	set_action_for_machine_type(get_current_product());
}

/* get system type. */
V_SystemType_t
v_get_system_type(void)
{
	V_SystemType_t val;

	switch (get_machinetype()) {
	case MT_STANDALONE:
		val = V_STANDALONE;
		break;

	case MT_SERVER:
		val = V_SERVER;
		break;

	default:
		val = V_STANDALONE;
		break;

	}
	return (val);
}

/* gets system type as a string */
char *
v_get_string_from_type(V_SystemType_t type)
{
	static char buf[128];

	switch (type) {
	case V_STANDALONE:
		(void) strcpy(buf, gettext("Standalone"));
		break;

	case V_SERVER:
		(void) strcpy(buf, gettext("OS server"));
		break;

	default:
		buf[0] = '\0';
		break;

	}

	return (buf);
}

/* get number of diskless clients */
int
v_get_n_diskless_clients(void)
{
	return (_n_d_clients);
}

/* set number of diskless clients */
V_Status_t
v_set_n_diskless_clients(int val)
{
	_n_d_clients = val;

	set_client_space(_n_d_clients, mb_to_sectors(_root_size),
		mb_to_sectors(_diskless_swap));
	return (V_OK);
}

/* get size of diskless client's swap file */
int
v_get_diskless_swap(void)
{
	return (_diskless_swap);
}

/* set size of diskless client's swap file */
V_Status_t
v_set_diskless_swap(int val)
{
	_diskless_swap = val;

	set_client_space(_n_d_clients, mb_to_sectors(_root_size),
		mb_to_sectors(_diskless_swap));
	return (V_OK);
}

/* set root size per client */
void
v_set_root_client_size(int val)
{

	_root_size = val;
	set_client_space(_n_d_clients, mb_to_sectors(_root_size),
		mb_to_sectors(_diskless_swap));
}

/* get root size per client */
int
v_get_root_client_size()
{
	return (_root_size);
}


/* get number of cache-only clients */
int
v_get_n_cache_clients(void)
{
	return (_n_c_clients);
}

/* set number of cache clients */
V_Status_t
v_set_n_cache_clients(int val)
{
	_n_c_clients = val;

	set_client_space(_n_c_clients, mb_to_sectors(_root_size),
		mb_to_sectors(_diskless_swap));
	return (V_OK);
}


/* get/set environment pointer... used for exec'ing a new shell. */
void
v_set_environ(char **envptr)
{
	_envp = envptr;
}

char **
v_get_environ(void)
{
	return (_envp);
}

/* get/set program name... used for error/abort messages (?) */
void
v_set_progname(char *str)
{
	char *cp;

	if ((cp = strrchr(str, '/')) != (char *) NULL)
		_progname = (cp + 1);
	else
		_progname = str;
}

char *
v_get_progname(void)
{
	return (_progname);
}

/*
 * umount any file systems and unswap... if this returns anything but 0,
 * then install should not be allowed to progress.
 */
int
v_cleanup_prev_install(void)
{
	return (reset_system_state());
}

void
v_reset_view_libraries(void)
{
	int n;
	int i;

	/* cleanup from any previous install attemps */
	(void) reset_system_state();

	/* return all disks to their orginal state... */
	n = v_get_n_disks();
	for (i = 0; i < n; i++)
		(void) v_unconfig_disk(i);

	v_set_disp_units(V_MBYTES);
	v_set_default_showcyls(FALSE);
	v_set_default_overlap(FALSE);

	/* reset software library to just loaded state */
	(void) v_init_sw((char *) NULL);
	(void) v_set_init_sw_config();

	/* unselect all locales... */
	n = v_get_n_locales();
	for (i = 0; i < n; i++)
		(void) v_set_locale_status(i, FALSE);

	(void) v_set_default_locale(v_get_default_locale());

	/* unselect all arches except `native' arch */
	v_clear_nonnative_arches();

	/* select native arch (it is req'd) */
	v_init_native_arch();

	/* clear out all remote file systems specs */
	v_delete_all_rfs();
	v_clear_export_fs();

	/* set default server parameters */
	_n_d_clients = DEFAULT_NUMBER_OF_CLIENTS;
	_diskless_swap = DEFAULT_SWAP_PER_CLIENT;
	_n_c_clients = DEFAULT_CACHE_ONLY_CLIENTS;
	_root_size = DEFAULT_ROOT_PER_CLIENT;
}

/*
 * this is a quick&dirty hack to get installs up & running. need to re-visit
 * and clean up
 */
parAction_t
v_do_install(void)
{
	pfcSUInitialData pfc_su_data;
	TSUData su_data;
	TSUError ret;

	write_debug(CUI_DEBUG_L1,
		gettext("Installation starting..."));

	(void) setvbuf(stdout, NULL, _IOLBF, 0);
	(void) setvbuf(stderr, NULL, _IOLBF, 0);

	/*
	 * call installation back-end code...
	 */
	(void) memset((void *) &pfc_su_data, '\0', sizeof (pfcSUInitialData));

	su_data.Operation = SI_INITIAL_INSTALL;
	su_data.Info.Initial.prod = get_current_product();
	su_data.Info.Initial.cfs = v_get_first_rfs();
	su_data.Info.Initial.SoftUpdateCallback = pfcSystemUpdateInitialCB;
	su_data.Info.Initial.ApplicationData = (void *) &pfc_su_data;

	/*
	 * Make sure SCR output is set to get back to the screen
	 * at the beginning of SystemUpdate.
	 * This is because SystemUpdate first prints disk partitioning data to
	 * the screen while the disks are being partitioned.
	 * During this time, we turn curses off, and then turn it back
	 * on in the begin initial install callback.
	 */
	(void) write_status_register_log(NULL);
	(void) write_error_register_log(NULL);
	(void) write_warning_register_log(NULL);
	end_curses(TRUE, TRUE);

	ret = SystemUpdate(&su_data);
	if (ret == SUSuccess) {
		/* the installation was successful, continue */
		return (parAContinue);
	} else {
		simple_notice(stdscr, F_OKEYDOKEY,
			TITLE_ERROR,
			SUGetErrorText(ret));

		/* the installation failed, exit */
		return (parAExit);
	}
}

/* ------------------------ */

void
v_int_error_exit(int err)
{
	end_curses(FALSE, TRUE);

	(void) fprintf(stderr, "%s\n", get_err_str(err));

	/* exit with a failure status */

	(void) tcflush(0, TCIFLUSH);
	exit(EXIT_INSTALL_FAILURE);

}

void
v_exec_sh(int sig)
{

	end_curses(TRUE, TRUE);
/* 	(void) tcflush(0, TCIFLUSH); */

	if (sig) {
		write_debug(CUI_DEBUG_L1, ABORTED_BY_SIGNAL_FMT, sig);
		(void) fprintf(stderr, ABORTED_BY_SIGNAL_FMT, sig);
	}

	if (pfgState & AppState_UPGRADE_CHILD) {
		exit(ChildUpgExitSignal);
	} else {
		exit(EXIT_INSTALL_FAILURE);
	}
}
