#ifndef lint
#pragma ident "@(#)inst_parade.c 1.82 96/10/03 SMI"
#endif

/*
 * Copyright (c) 1991-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_parade.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/strredir.h>
#include <sys/fs/ufs_fs.h>

#include <locale.h>
#include <libintl.h>
#include <fcntl.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"

#include "v_check.h"
#include "v_types.h"
#include "v_lfs.h"
#include "v_disk.h"
#include "v_sw.h"
#include "v_misc.h"
#include "v_upgrade.h"

/* state global used to keep upgrade state */
unsigned int pfgState;
Profile	*pfProfile;
ParamUsage *param_usage;
tty_MsgAdditionalInfo ttyInfo;
UpgOs_t *UpgradeSlices = NULL;
int DebugDest = LOG;

TList DsrSLHandle = NULL;
FSspace **FsSpaceInfo = NULL;
TDSRArchiveList DsrALHandle = NULL;
char *StatusScrFileName = NULL;
char *ErrWarnLogFileName = NULL;

#define	PFC_DEFAULT_HELP_DIR "/usr/openwin/lib/locale/C/help/install.help"

/* private functions */
static void pfcPrintRestartMsg(void);

/* static variables */
static int _user_exit = FALSE;
static int _exit_confirmed = FALSE;

/*
 * FE getopts args string.
 */
static char _pfc_app_args[] = "uv";

main(argc, argv, environ)
	int argc;
	char **argv;
	char **environ;
{
	char *default_locale;
	int optindex;
	char *envp;

	/*
	 * Is there an environment var telling where to send debug output?
	 * The env var can be used to set a file to send SCR output to.
	 * i.e. all output is ALWAYS sent to a file.
	 * This is because printing any screen output in the CUI totally
	 * hoses the curses output and is unreadable.
	 * This is true also of Error and Warning output generated
	 * by the backend.
	 * Note that when we really do want SCR output (e.g. in the
	 * backend phase), we explicitly:
	 *	- turn curses off
	 *	- reset the SCR output to go to the SCR
	 *	- print the output
	 *	- reset the SCR output back to the file
	 *	- turn curses back on
	 *
	 */
	envp = getenv("INSTALL_STATUS_LOG");
	if (envp) {
		StatusScrFileName = xstrdup(envp);
		if (write_status_register_log(StatusScrFileName) == FAILURE) {
			free(StatusScrFileName);
			StatusScrFileName = DFLT_STATUS_LOG_FILE;
		}
	} else {
		StatusScrFileName = DFLT_STATUS_LOG_FILE;
	}
	(void) write_status_register_log(StatusScrFileName);

	/*
	 * Initially start the Error/Warning messages file to be
	 * install_log.  We will switch it to upgrade_log if they choose
	 * an upgrade.
	 * We only have to do this in the CUI (not the GUI) because the
	 * CUI can't be writing to stderr while curses has screen control.
	 * The GUI, on the other hand, can just dump stuff out to the
	 * console...
	 */
	ErrWarnLogFileName = DFLT_INSTALL_LOG_FILE;
	(void) write_error_register_log(ErrWarnLogFileName);
	(void) write_warning_register_log(ErrWarnLogFileName);

	pfProfile = (Profile *) xmalloc(sizeof (Profile));

	/* setup FE command line parameter info */
	param_usage = (ParamUsage *) xmalloc(sizeof (ParamUsage));
	ParamsGetProgramName(argv[0], param_usage);
	param_usage->app_args = _pfc_app_args;
	param_usage->app_public_usage = PFC_PARAMS_PUBLIC_USAGE;
	param_usage->app_private_usage = PFC_PARAMS_PRIVATE_USAGE;
	param_usage->app_trailing_usage = PFC_PARAMS_TRAILING_USAGE;

	/*
	 * Set all of the program's locale categories from the environment,
	 * and set the domain for gettext() calls.  Use the returned locale
	 * from setlocale() to save off the current locale
	 */

	default_locale = setlocale(LC_ALL, "");
	(void) textdomain("SUNW_INSTALL_TTINSTALL");

	HelpInitialize(PFC_DEFAULT_HELP_DIR);

	v_set_progname(argv[0]);
	v_set_environ(environ);

	/* parse FE args, BE args, and validate */
	ProfileInitialize(pfProfile);
	ParamsParseUIArgs(argc, argv, param_usage);
	optindex = ParamsParseCommonArgs(argc, argv, param_usage, pfProfile);
	ParamsValidateUILastArgs(argc, optindex, param_usage);
	ParamsValidateCommonArgs(pfProfile);

	(void) sigignore(SIGHUP);
	(void) sigignore(SIGPIPE);
	(void) sigignore(SIGALRM);
	(void) sigignore(SIGTERM);
	(void) sigignore(SIGURG);
	(void) sigignore(SIGCONT);
	/* (void) sigignore(SIGCHLD); */
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
	(void) sigignore(SIGTSTP);

	(void) signal(SIGILL, v_exec_sh);
	(void) signal(SIGTRAP, v_exec_sh);
	(void) signal(SIGEMT, v_exec_sh);
	(void) signal(SIGFPE, v_exec_sh);
	(void) signal(SIGBUS, v_exec_sh);
	(void) signal(SIGSEGV, v_exec_sh);
	(void) signal(SIGSYS, v_exec_sh);

	if (debug == 0) {
		(void) sigignore(SIGINT);
		(void) sigignore(SIGQUIT);
	} else {
		/*
		 * catch these only if we're debugging
		 */
		(void) signal(SIGINT, v_exec_sh);
		(void) signal(SIGQUIT, v_exec_sh);
	}

	set_clearscr(1);	/* clear screen on exit */

	/*
	 * get rid of any stuff left laying around... if this fails, install
	 * cannot proceeed...
	 */
	if (v_cleanup_prev_install() != 0) {
		tty_cleanup();
		(void) fprintf(stderr, INIT_CANT_RECOVER_ERROR);
		exit(EXIT_INSTALL_FAILURE);
	}

	/*
	 * load software modules
	 */
	if (v_init_sw(MEDIANAME(pfProfile)) == V_FAILURE) {
		tty_cleanup();
		(void) fprintf(stderr, INIT_CANT_LOAD_MEDIA,
			MEDIANAME(pfProfile));
		exit(EXIT_INSTALL_FAILURE);
	}
	(void) v_set_init_sw_config();

	/* set up curses */
	if (init_curses() == 0) {
		/*
		 * screen too small or some other problem...
		 * init_curses() prints out the error msg, so don't
		 * print one here.
		 */
		tty_cleanup();
		exit(EXIT_INSTALL_FAILURE);
	}
	pfc_fkeys_init();

	/* set up message dialog info */
	ttyInfo.parent = stdscr;
	ttyInfo.dialog_fkeys[UI_MSGBUTTON_OK] = F_OKEYDOKEY;
	ttyInfo.dialog_fkeys[UI_MSGBUTTON_OTHER1] = F_GOBACK;
	ttyInfo.dialog_fkeys[UI_MSGBUTTON_OTHER2] = F_EDIT;
	ttyInfo.dialog_fkeys[UI_MSGBUTTON_CANCEL] = F_CANCEL;
	ttyInfo.dialog_fkeys[UI_MSGBUTTON_HELP] = F_HELP;
	UI_MsgGenericInfoSet((void *)&ttyInfo);
	UI_MsgFuncRegister(tty_MsgFunction);

	/*
	 * load disks
	 */
	(void) v_init_disks();

	/*
	 * set up initial state
	 *
	 * default locale selection, native architecture, etc.
	 */
	v_set_system_type(V_STANDALONE);
	(void) v_set_default_locale(default_locale);
	v_init_native_arch();

	/*
	 * Check the disk status and exit with an error message if
	 * there's a problem, otherwise start the parade.
	 */
	pfCheckDisks();

	/*
	 * set initial mount point table status
	 */
	v_restore_default_fs_table();

	/* eat any pre-input */
	flush_input();

        /*
	 * initialize the root directory as an indirect install; this
	 * will be overridden later if needed
	 */
	set_rootdir("/a");

	/* get the list of upgradeable slices & releases */
	SliceGetUpgradeable(&UpgradeSlices);

	pfgParade(parIntro);

	tty_cleanup();

	exit(EXIT_INSTALL_SUCCESS_NOREBOOT);
	/*NOTREACHED*/
}

void
pfcExit(void)
{
	if (confirm_exit(stdscr)) {
		pfcCleanExit(EXIT_INSTALL_FAILURE, (void *) 1);
	}
}

/*
 * puts up the exit confirmation `notice'.
 *
 * input:
 *	parent:		WINDOW * to curses window to repaint when notice
 *			is dismissed.
 *
 * returns:
 *	1 - user really wants to exit
 *	0 - user does not want to exit
 *
 */
int
confirm_exit(WINDOW * parent)
{
	int row;
	int ch;
	unsigned long fkeys;

	WINDOW *win;

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);

	(void) werase(win);
	(void) wclear(win);

	wheader(win, TITLE_EXIT);
	row = HeaderLines;
	row = wword_wrap(win, row, INDENT1, COLS - (2 * INDENT1), MSG_EXIT);

	fkeys = F_EXITINSTALL | F_CANCEL;

	wfooter(win, fkeys);
	wcursor_hide(win);

	for (;;) {

		ch = wzgetch(win, fkeys);

		if (is_exitinstall(ch) != 0) {
			break;
		} else if (is_cancel(ch) != 0) {
			break;
		} else if (is_escape(ch) != 0) {
			continue;
		} else
			beep();
	}

	(void) delwin(win);
	(void) clearok(curscr, TRUE);
	(void) touchwin(parent);
	(void) wnoutrefresh(parent);
	(void) clearok(curscr, FALSE);

	if (is_exitinstall(ch) != 0) {
		_user_exit = TRUE;
		_exit_confirmed = TRUE;
		return (1);
	} else {
		_user_exit = FALSE;
		_exit_confirmed = FALSE;
		return (0);
	}

}

void
pfgUnmountOrSwapError()
{
	(void) simple_notice(stdscr, F_OKEYDOKEY,
		gettext("Reboot"),
		APP_ER_UNMOUNT);
	pfcCleanExit(EXIT_INSTALL_FAILURE, (void *) 1);
}

parAction_t
pfgConfirmExit(void)
{
	if (_exit_confirmed)
		return (parAExit);

	if (confirm_exit(stdscr)) {
		return (parAExit);
	} else {
		return (parANone);
	}
}

void
pfcSetConfirmExit(int confirmed)
{
	_exit_confirmed = confirmed;
}

int
pfcGetConfirmExit(void)
{
	return (_exit_confirmed);
}

/*
 * exit_data is currently used to indicate if the
 * app should clear the screen when it exits. (i.e. this is used
 * so we can figure out between the parent/child upgrade processes if we
 * should clear the screen or not when we end curses.  The child ends
 * curses and clears the screen and prints a message.  We then don't want
 * the parent to clear the screen since that blows away the message the
 * child just wrote to stderr.
 */
void
pfcCleanExit(int exit_code, void *exit_data)
{
	int edata = (int) exit_data;

	/*
	 * Should we clear the screen on this exit or not?
	 * In the case of an exit due to receiving a signal from the
	 * child, we do not want to clear the screen because the child has
	 * already done so and printed out a message that we don't want to
	 * clear away...
	 */
	if (edata == 0)
		set_clearscr(FALSE);
	else
		set_clearscr(TRUE);

	write_debug(CUI_DEBUG_L1, "exit data = %d", edata);

	if (pfgState & AppState_UPGRADE) {
#if 0
		if (FsSpaceInfo)
			swi_free_space_tab(FsSpaceInfo);
#endif
		if (pfgState & AppState_UPGRADE_CHILD) {
			/*
			 * make sure the child exits with values the
			 * parent understands.
			 */
			switch (exit_code) {
			case EXIT_INSTALL_FAILURE:
				if (_user_exit)
					exit_code = ChildUpgUserExit;
				else
					exit_code = ChildUpgExitFailure;
				break;
			case EXIT_INSTALL_SUCCESS_REBOOT:
				exit_code = ChildUpgExitOkReboot;
				break;
			case EXIT_INSTALL_SUCCESS_NOREBOOT:
				exit_code = ChildUpgExitOkNoReboot;
				break;
			}
			pfcChildShutdown((TChildAction) exit_code);
		} else {
			/*
			 * We only want this done once while exitting
			 * upgrade (i.e. don't let both the parent and
			 * the child do it.)
			 */
			(void) umount_and_delete_swap();
			tty_cleanup();

			/*
			 * Was the exit user requested within the child process?
			 * If so, then have the parent print the
			 * restart message.
			 */
			if (_user_exit || edata == 2)
				_user_exit = TRUE;

			pfcPrintRestartMsg();
		}
	} else {
		/* initial install */
		tty_cleanup();
		pfcPrintRestartMsg();
	}

	exit(exit_code);
}

static void
pfcPrintRestartMsg(void)
{
	if (_user_exit)
		(void) fprintf(stderr, PFC_RESTART);
	_user_exit = FALSE;
}

void
pfcChildShutdown(TChildAction exit_code)
{
	exit((int) exit_code);
}
