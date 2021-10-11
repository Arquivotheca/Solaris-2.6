#ifndef lint
#pragma ident "@(#)pfgmain.c 1.102 96/10/03 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgmain.c
 * Group:	installtool
 * Description:
 */

#include <sys/types.h>
#include <unistd.h>

#include "pf.h"
#include "pfg.h"

#include <stdarg.h>

/* PROGRAM GLOBALS, see pf.h for details */

int		LastServiceChoice;
TParentReinit	pfgParentReinitData;
Widget 		pfgTopLevel;
XtAppContext 	pfgAppContext;
unsigned int	pfgState;
Atom 		pfgWMDeleteAtom;
ParamUsage	*param_usage;
Profile		*pfProfile;
int		DebugDest = SCR;
xm_MsgAdditionalInfo xmInfo;
UpgOs_t		 *UpgradeSlices = NULL;
char *		StatusScrFileName = NULL;

TList DsrSLHandle = NULL;
FSspace **FsSpaceInfo = NULL;
TDSRArchiveList DsrALHandle = NULL;

/* static globals */
static int	user_exit = FALSE;

/*
 * FE getopts args string.
 * -M not mentioned here since it is parsed using X resources instead
 * and is removed from argc/argv.
 */
static char _pfg_app_args[] = "uv";

/* local function prototypes */

static void _pfgParamsModForX(int argc, char **argv, int original_argv);
static void pfCaughtSignal(int);
static void pfSetSignalCatcher(void);

/*
 * Parse command line, set globals, send a change, which causes the widgets
 * to instantiate as part of the first paint of the internal calculated data
 * structures.  Main loop here, since pfgInit called here to eat -xrm
 * comline.
 */

int
main(int argc, char ** argv)
{
	pfErCode	er;	/* result of swlib initialization.... */
	pfErCode	disk_err;	/* result of loading disks.... */
	int		numDisks;
	int		optindex;
	char *envp;
	int i;

	/*
	 * Set up to catch signals and exit with a message.
	 * This is expecially important with the parent/child process
	 * setup in upgrade so that the child can catch a signal,
	 * exit cleanly, and the parent can then exit cleanly as well.
	 */
	pfSetSignalCatcher();

	/*
	 * For the parent process to reinit itself in the upgrade case.
	 */
	pfgParentReinitData.argc = argc;
	pfgParentReinitData.argv = (char **) xmalloc(argc * sizeof (char *));
	for (i = 0; i < argc; i++) {
		pfgParentReinitData.argv[i] = xstrdup(argv[i]);
	}

	pfProfile = (Profile *) xmalloc(sizeof (Profile));

	/* setup FE command line parameter info */
	param_usage = (ParamUsage *) xmalloc(sizeof (ParamUsage));
	ParamsGetProgramName(argv[0], param_usage);
	param_usage->app_args = _pfg_app_args;
	param_usage->app_public_usage = PFG_PARAMS_PUBLIC_USAGE;
	param_usage->app_private_usage = PFG_PARAMS_PRIVATE_USAGE;
	param_usage->app_trailing_usage = PFG_PARAMS_TRAILING_USAGE;

	LastServiceChoice = 0;

	/* i18n set up */
	(void) XtSetLanguageProc(NULL, (XtLanguageProc) NULL, NULL);

	/* parse X based arguments and do other X/Xt initialization */
	_pfgParamsModForX(argc, argv, 1);
	pfgInit(&argc, argv);
	_pfgParamsModForX(argc, argv, 0);

	/*
	 * change for partial locale support
	 */
	(void) setlocale(LC_ALL, "");

	(void) textdomain(SUNW_INSTALL_INSTALL);

	/* initialize the profile structure */
	ProfileInitialize(pfProfile);

	/* parse the ttinstall/installtool specific arguments */
	ParamsParseUIArgs(argc, argv, param_usage);

	/* parse the common install arguments */
	optindex = ParamsParseCommonArgs(argc, argv, param_usage, pfProfile);

	/*
	 * make final check on number of arguments - i.e. make sure
	 * that there are no unused/invalid trailing argmuments.
	 */
	ParamsValidateUILastArgs(argc, optindex, param_usage);

	/* do some basic arg checking */
	ParamsValidateCommonArgs(pfProfile);

	/*
	 * Is there an environment var telling where to send debug output?
	 * The env var can be used to set a file to send SCR output to.
	 * i.e. all output is always sent to a file if debugging is
	 * turned on at all.
	 */
	if (debug || get_trace_level() > 0) {
		envp = getenv("INSTALL_STATUS_LOG");
		if (envp) {
			StatusScrFileName = xstrdup(envp);
			if (write_status_register_log(StatusScrFileName)
				== FAILURE) {
				free(StatusScrFileName);
				StatusScrFileName = DFLT_STATUS_LOG_FILE;
			}
		} else {
			StatusScrFileName = DFLT_STATUS_LOG_FILE;
		}
		(void) write_status_register_log(StatusScrFileName);
	}

	write_debug(GUI_DEBUG_L1, "Application data: maptop = %d",
		pfgAppData.maptop);
	write_debug(GUI_DEBUG_L1,
		"Application data: dsrSpaceReqColumnSpace = %d",
		pfgAppData.dsrSpaceReqColumnSpace);
	write_debug(GUI_DEBUG_L1,
		"Application data: dsrFSSummColumnSpace = %d",
		pfgAppData.dsrFSSummColumnSpace);
	write_debug(GUI_DEBUG_L1,
		"Application data: dsrFSRedistColumnSpace = %d",
		pfgAppData.dsrFSRedistColumnSpace);
	write_debug(GUI_DEBUG_L1,
		"Application data: dsrFSCollapseColumnSpace = %d",
		pfgAppData.dsrFSCollapseColumnSpace);

	if (geteuid() && !DISKFILE(pfProfile)) {
		(void) fprintf(stderr, "%s: %s: %s\n",
			param_usage->app_name_base, PFG_MN_ERROR, PFG_MN_ROOT);
			exit(EXIT_INSTALL_FAILURE);
	}

	write_debug(GUI_DEBUG_L1,
		"Disk file name = %s",
		DISKFILE(pfProfile) ? DISKFILE(pfProfile) : "NULL");
	write_debug(GUI_DEBUG_L1_NOHD,
		"CD file name   = %s",
		MEDIANAME(pfProfile) ? MEDIANAME(pfProfile) : "NULL");

	/* cleanup previous install */
	if (reset_system_state() == -1) {
		(void) printf(APP_ER_UNMOUNT);
		exit(EXIT_INSTALL_FAILURE);
	}
	write_debug(GUI_DEBUG_L1, "reset_system_state successful");

	/* load the software struct or fail */

	if (er = pfLoadCD()) {
		(void) fprintf(stderr, "%s: %s: %s %s\n",
			param_usage->app_name_base,
			(pfErIsLethal(er) ? PFG_MN_ERROR : PFG_MN_WARN),
			pfErMessage(er),
			(MEDIANAME(pfProfile) ? MEDIANAME(pfProfile) : ""));
		pfErExitIfLethal(er);
		MEDIANAME(pfProfile) = NULL;
	}
	write_debug(GUI_DEBUG_L1, "loading CD successful");

	/* build list of installable disks */
	disk_err = pfLoadDisks(&numDisks);
	write_debug(GUI_DEBUG_L1, "loading disks successful");

	/* initialize the software library */
	(void) pfInitializeSw();
	write_debug(GUI_DEBUG_L1, "intializing SW lib successful");

	/*
	 * initialize the root directory as an indirect install; this
	 * will be overridden later if needed
	 */
	set_rootdir("/a");

	/* get the list of upgradeable slices & releases */
	SliceGetUpgradeable(&UpgradeSlices);

	setSystemType(MT_STANDALONE);
	(void) setDefaultLocale(setlocale(LC_ALL, (char *)NULL));
	initNativeArch();

	XtRealizeWidget(pfgTopLevel);

	/* set up ui msg function stuff */
	xmInfo.parent = NULL;
	xmInfo.toplevel = pfgTopLevel;
	xmInfo.app_context = pfgAppContext;
	xmInfo.delete_atom = pfgWMDeleteAtom;
	xmInfo.delete_func = pfgExit;
	UI_MsgGenericInfoSet((void *)&xmInfo);
	UI_MsgFuncRegister(xm_MsgFunction);

	/*
	 * Now that the toplevel widget has been realized, check the
	 * disk status and exit with an error message if there's a problem,
	 * otherwise start the parade.
	 */
	if (disk_err != pfOK) {
		pfgExitError(pfgTopLevel, disk_err);
	} else {
		pfgParade(parIntro);
	}

	XtAppMainLoop(pfgAppContext);

#ifdef lint
	return (0);
#else
	exit(EXIT_INSTALL_SUCCESS_NOREBOOT);
#endif
}

/* See _pfgParamsModForX function comments for info on these structures */
typedef struct {
	char *original;
	char *modified;
} ParamMod;

/*
 * defines the list of parms to temporarily modify and what to modify
 * them to
 */
static ParamMod _pfg_param_mods[] = {
	{"-d", "-1"},
	{NULL, NULL}
};

/*
 * Function: _pfgParamsModForX
 * Description:
 * Modifies argv arguments so that they can be temporarily changed to
 * allow passing certain arguments through X command line parsing without
 * being processed by X.  Also used to modify these args back to normal
 * after X parsing has been done.
 *
 * Scope:	private
 * Parameters:  <name> -	[<RO|RW|WO>]
 *				[<validation conditions>]
 *				<description>
 *		argc - [RO]
 *		argv - [RW]
 *		original_argv - [RO]
 *			Are we supposed to be replacing the
 *			original argv args with their replacements
 *			(original_argv == 1), or are we resetting the
 *			modified args back to the original argv settings
 *			(original_argv == 0).
 *
 * Return:	void
 *
 * Globals:
 *		_pfg_param_mods - MODULE
 * Notes:
 * We have to jump through some hoops in order to make sure that the
 * X/Xt command line parsing does not eat our -d disk_simulation_file
 * option as an X display variable instead.  X will grab -d for -display
 * since it looks for the smallest unique string for each of the options
 * it recognizes.  (We must keep -d disk_sim_file because it is an arc'd
 * interface for pfinstall already). To work around this, we temporarily
 * change any -d command line options to something X will not recognize,
 * then do the X command line parsing, and then change things back so
 * that we can pass the options on normally.
 * Due to how X parse command line args, we only need to look for
 * separate "-d <display" arguments; arguments like getopts takes, like
 * "-ud <display> are not a problem here - X will not eat this.
 */
static void
_pfgParamsModForX(int argc, char **argv, int original_argv)
{
	int argv_i, mods_i;
	char *original;
	char *modified;

	/* go thru each command line arg */
	for (argv_i = 0; argv_i < argc; argv_i++) {
		write_debug(GUI_DEBUG_L1,
			"original argv[%d] = %s",
			argv_i, argv[argv_i]);

		/*
		 * Do any of the params in argv match a param
		 * we need to change temporarily?
		 * Go thru each of the modify spec's and see if any
		 * match.
		 */
		for (mods_i = 0; _pfg_param_mods[mods_i].original; mods_i++) {

			/*
			 * original = if this is in the argv list now, then
			 *	this should be modified.
			 * modified = what the arg should be changed to
			 */
			if (original_argv) {
				original = _pfg_param_mods[mods_i].original;
				modified = _pfg_param_mods[mods_i].modified;
			} else {
				original = _pfg_param_mods[mods_i].modified;
				modified = _pfg_param_mods[mods_i].original;
			}

			/* if an arg matches original, then modify it */
			if (strcmp(argv[argv_i], original) == 0) {
				argv[argv_i] = xstrdup(modified);
			}
		}

		write_debug(GUI_DEBUG_L1,
			"modified argv[%d] = %s",
			argv_i, argv[argv_i]);
	}

}

static void
pfSetSignalCatcher(void)
{
	/* see bottom of file for listing of values and meanings */

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
	(void) sigignore(SIGTSTP);
	(void) signal(SIGINT, pfCaughtSignal);
	(void) signal(SIGQUIT, pfCaughtSignal);
	(void) signal(SIGILL, pfCaughtSignal);
	(void) signal(SIGTRAP, pfCaughtSignal);
	(void) signal(SIGEMT, pfCaughtSignal);
	(void) signal(SIGFPE, pfCaughtSignal);
	(void) signal(SIGBUS, pfCaughtSignal);
	(void) signal(SIGSEGV, pfCaughtSignal);
	(void) signal(SIGSYS, pfCaughtSignal);
}

static void
pfCaughtSignal(int sig)
{
	(void) fprintf(stderr, ABORTED_BY_SIGNAL_FMT, sig);
	(void) fflush(stderr);

	/* make sure an upgrade child exits with a known error code */
	if (pfgState & AppState_UPGRADE_CHILD) {
		exit (ChildUpgExitSignal);
	} else {
		pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
	}

}

#ifdef _Section_of_sys_signal_h_

#define	SIGHUP	1	/* hangup */
#define	SIGINT	2	/* interrupt (rubout) */
#define	SIGQUIT	3	/* quit (ASCII FS) */
#define	SIGILL	4	/* illegal instruction (not reset when caught) */
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
#define	SIGIOT	6	/* IOT instruction */
#define	SIGABRT 6	/* used by abort, replace SIGIOT in the future */
#define	SIGEMT	7	/* EMT instruction */
#define	SIGFPE	8	/* floating point exception */
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
#define	SIGBUS	10	/* bus error */
#define	SIGSEGV	11	/* segmentation violation */
#define	SIGSYS	12	/* bad argument to system call */
#define	SIGPIPE	13	/* write on a pipe with no one to read it */
#define	SIGALRM	14	/* alarm clock */
#define	SIGTERM	15	/* software termination signal from kill */
#define	SIGUSR1	16	/* user defined signal 1 */
#define	SIGUSR2	17	/* user defined signal 2 */
#define	SIGCLD	18	/* child status change */
#define	SIGCHLD	18	/* child status change alias (POSIX) */
#define	SIGPWR	19	/* power-fail restart */
#define	SIGWINCH 20	/* window size change */
#define	SIGURG	21	/* urgent socket condition */
#define	SIGPOLL 22	/* pollable event occured */
#define	SIGIO	SIGPOLL	/* socket I/O possible (SIGPOLL alias) */
#define	SIGSTOP 23	/* stop (cannot be caught or ignored) */
#define	SIGTSTP 24	/* user stop requested from tty */
#define	SIGCONT 25	/* stopped process has been continued */
#define	SIGTTIN 26	/* background tty read attempted */
#define	SIGTTOU 27	/* background tty write attempted */
#define	SIGVTALRM 28	/* virtual timer expired */
#define	SIGPROF 29	/* profiling timer expired */
#define	SIGXCPU 30	/* exceeded cpu limit */
#define	SIGXFSZ 31	/* exceeded file size limit */
#define	SIGWAITING 32	/* process's lwps are blocked */
#define	SIGLWP	33	/* special signal used by thread library */

#endif  _Section_of_sys_signal_h_

static pfQuery_t * get_query(pfQueryCode);
static void exitCB(Widget, XtPointer, XmAnyCallbackStruct *);

/*
 * pop up an information dialog with just an ok button
 */
void
pfAppInfo(char *title, char *message)
{
	UI_MsgStruct *msg_info;

	if (message == NULL || *message == '\0')
		return;

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->title = title;
	msg_info->msg_type = UI_MSGTYPE_INFORMATION;
	msg_info->msg = message;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;

	/* invoke the message */
	UI_MsgFunction(msg_info);

	/* cleanup */
	UI_MsgStructFree(msg_info);
}

/* ARGSUSED */
void
pfAppWarn(pfErCode code, char *message)	/* add help based on er code */
{
	UI_MsgStruct *msg_info;

	if (message == NULL || *message == '\0')
		return;

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->msg_type = UI_MSGTYPE_WARNING;
	msg_info->msg = message;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;

	/* invoke the message */
	UI_MsgFunction(msg_info);

	/* cleanup */
	UI_MsgStructFree(msg_info);
}

/* ARGSUSED */
void
pfAppError(char *title, char *message)	/* add help based on er code */
{
	UI_MsgStruct *msg_info;

	if (message == NULL || *message == '\0')
		return;

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->msg_type = UI_MSGTYPE_ERROR;
	msg_info->msg = message;
	msg_info->title = title;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;

	/* invoke the message */
	UI_MsgFunction(msg_info);

	/* cleanup */
	UI_MsgStructFree(msg_info);
}


/* ARGSUSED */
void
pfgWarning(Widget parent, pfErCode er)
{
	UI_MsgStruct *msg_info;

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->msg_type = UI_MSGTYPE_WARNING;
	msg_info->msg = pfErMessage(er);
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;

	/* invoke the message */
	UI_MsgFunction(msg_info);

	/* cleanup */
	free(msg_info->msg);
	UI_MsgStructFree(msg_info);
}

/* ARGSUSED */
void
pfgExitError(Widget parent, pfErCode er)
{
	UI_MsgStruct *msg_info;

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->msg_type = UI_MSGTYPE_ERROR;
	msg_info->msg = pfErMessage(er);
	msg_info->btns[UI_MSGBUTTON_OK].button_text = UI_BUTTON_OK_STR;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;

	/* invoke the message */
	UI_MsgFunction(msg_info);

	/* cleanup and exit */
	UI_MsgStructFree(msg_info);

	pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
}

static pfQuery_t *
get_query(pfQueryCode qnum)
{
	static pfQuery_t ret;

	/* the default values */
	ret.yes = PFG_OKAY;
	ret.no = PFG_CANCEL;
	ret.def = FALSE;

	switch (qnum) {
	case pfQFDISKCHANGE:
		ret.message = PFG_MN_WIPEOUT;
		break;
	case pfQBELOWMINDISK:
		ret.message = PFG_MN_BELOW;
		break;
	case pfQLOADVTOC:
		ret.message = PFG_MN_LOADVTOC;
		break;
	case pfQDEPENDS:
		ret.message = MSG_DEPENDS;
		break;
	case pfQLOSECHANGES:
		ret.message = PFG_MN_CHANGES;
		ret.yes = PFG_MN_PRESERVE;
		ret.no = PFG_CANCEL;
		ret.def = FALSE;
		break;
	case pfQAUTOFAIL:
		ret.message = MSG_AUTOFAIL;
		ret.yes = PFG_AQ_MANLAY;
		ret.no = PFG_CANCEL;
		ret.def = FALSE;
		break;
	case pfQREBOOT:
		ret.message = MSG_REBOOT;
		ret.yes = PFG_MN_REBOOT;
		ret.no = PFG_MN_NO_REBOOT;
		ret.def = TRUE;
		break;
	default:
		return (NULL);
	}
	return (&ret);
}


/* ARGSUSED */
int
pfgQuery(Widget parent, pfQueryCode qnum)
{
	UI_MsgStruct *msg_info;
	pfQuery_t *q;
	int answer;

	write_debug(GUI_DEBUG_L1, "Entering pfgQuery");

	q = get_query(qnum);
	if (!q)
		return (TRUE);

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->msg_type = UI_MSGTYPE_QUESTION;
	msg_info->title = " ";
	msg_info->msg = q->message;
	msg_info->btns[UI_MSGBUTTON_OK].button_text = q->yes;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = q->no;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;
	msg_info->default_button =
		q->def ? UI_MSGBUTTON_OK : UI_MSGBUTTON_CANCEL;

	/* invoke the message */
	UI_MsgFunction(msg_info);

	/* cleanup */
	UI_MsgStructFree(msg_info);

	switch (UI_MsgResponseGet()) {
	case UI_MSGRESPONSE_OK:
		answer = TRUE;
		break;
	case UI_MSGRESPONSE_CANCEL:
	default:
		answer = FALSE;
		break;
	}
	return ((Boolean)answer);
}

/* ARGSUSED */
int
pfgAppQuery(Widget parent, char * buff)
{
	UI_MsgStruct *msg_info;
	int answer;

	write_debug(GUI_DEBUG_L1, "Entering pfgAppQuery");

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->msg_type = UI_MSGTYPE_QUESTION;
	msg_info->title = " ";
	msg_info->msg = buff;
	msg_info->btns[UI_MSGBUTTON_OK].button_text = UI_BUTTON_OK_STR;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = UI_BUTTON_CANCEL_STR;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;
	msg_info->default_button = UI_MSGBUTTON_CANCEL;

	/* invoke the message */
	UI_MsgFunction(msg_info);

	/* cleanup */
	UI_MsgStructFree(msg_info);

	switch (UI_MsgResponseGet()) {
	case UI_MSGRESPONSE_OK:
		answer = TRUE;
		break;
	case UI_MSGRESPONSE_CANCEL:
	default:
		answer = FALSE;
		break;
	}
	return ((Boolean)answer);
}

/*
 * Function:	pfgInfo
 * Input(s):	char *title
 *		char *buf
 * Type:	public
 *
 * this function creates an information dialog with the
 * title and message strings passed in, currently the
 * information dialog created only has one button, OK
 * the dialog, since it is a child of pfgTopLevel can
 * be left up, but in the event that another call to pfgInfo
 * were made with a different title the previous title
 * and message would be replaced, a separate dialog would
 * not be created, this reduces the chances of memory related
 * problems
 */
void
pfgInfo(char *title, char *buf)
{
	static Widget	information_dialog = NULL;
	Widget		button;
	XmString	string, title_string;

	/*
	 * note that XmStringCreateLtoR is used for the message string here
	 * instead of XmStringCreateLocalized which is really the correct
	 * way to do this - the same is true for ALL messages in all functions
	 * in this file, XmStringCreateLocalized should replace every call
	 * to XmStringCreateLtoR, but due to a problem with
	 * XmStringCreateLocalized
	 * not recognizing \n imbedded in the strings we cannot use it
	 * right now, in the future all uses of XmStringCreateLtoR should
	 * be replaced to completely fulfill the I18N API
	 */

	string = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
	title_string = XmStringCreateLocalized(title);

	if (!information_dialog) {
		information_dialog = XmCreateInformationDialog(pfgTopLevel,
				"information_dialog", NULL, 0);
		xm_SetNoResize(pfgTopLevel, information_dialog);
		XtVaSetValues(information_dialog,
			XmNdeleteResponse, XmDO_NOTHING,
			XmNallowShellResize, False,
			NULL);
		button = XmMessageBoxGetChild(information_dialog,
			XmDIALOG_CANCEL_BUTTON);
		if (button)
			XtUnmanageChild(button);
		button = XmMessageBoxGetChild(information_dialog,
			XmDIALOG_HELP_BUTTON);
		if (button)
			XtUnmanageChild(button);
	}

	XtVaSetValues(information_dialog,
		XmNdialogTitle, title_string,
		XmNmessageString, string,
		NULL);

	XmStringFree(string);
	XmStringFree(title_string);
	XtManageChild(information_dialog);
}

/* ARGSUSED */
void
pfgHelp(Widget w, XtPointer client, XmAnyCallbackStruct *cbs)
{
	char *word, letter, raw, *buf;
	static Widget another_top_level_shell;

	if (!another_top_level_shell)
		another_top_level_shell = XtAppCreateShell("installtool",
			"Admin", applicationShellWidgetClass,
			XtDisplay(pfgTopLevel), (ArgList) NULL, 0);


	word = (char *) client;
	if (!word) {
		(void) xm_adminhelp(pfgTopLevel, NULL, NULL);
		return;
	}

	switch (raw = word[strlen(word) - 1]) {
	case 't':
		letter = 'C';
		break;
	case 'h':
		letter = 'P';
		break;
	case 'r':
		letter = 'R';
		break;
	default:
		(void) fprintf(stderr, "%s: unknown help value '%c'\n",
			"installtool", raw);
		return;
	}
	buf = (char *) xmalloc(strlen(word)+5);
	(void) strcpy(buf, word);
	(void) strcat(buf, ".hlp");

	write_debug(GUI_DEBUG_L1, "pfgHelp: '%c', \"%s\"\n", letter, buf);

	(void) xm_adminhelp(pfgTopLevel, letter, buf);
	free(buf);
}

void
pfgExit(void)
{
	UI_MsgStruct *msg_info;

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->msg_type = UI_MSGTYPE_WARNING;
	msg_info->msg = MSG_EXIT;
	msg_info->btns[UI_MSGBUTTON_OK].button_text = PFG_EXIT;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = PFG_CANCEL;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;
	msg_info->default_button = UI_MSGBUTTON_CANCEL;

	/*
	 * temporarily undo xmInfo so that delete from exit window
	 * doesn't bring up another exit window.
	 */
	xmInfo.delete_func = NULL;

	/* invoke the message */
	UI_MsgFunction(msg_info);

	/* cleanup */
	xmInfo.delete_func = pfgExit;
	UI_MsgStructFree(msg_info);

	if (UI_MsgResponseGet() == UI_MSGRESPONSE_OK) {
		user_exit = TRUE;
		exitCB((Widget) NULL, (XtPointer) NULL,
			(XmAnyCallbackStruct*) NULL);
	} else {
		/*
		 * Reset msg response to NONE in case we are nested
		 * inside another msg call.  If we are nested, then we
		 * want the one we're nested inside of to stay in it's
		 * event loop. We force this by setting the response to
		 * NONE.
		 */
		UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
	}
}

/* ARGSUSED */
static void
exitCB(Widget w, XtPointer client, XmAnyCallbackStruct* cbs)
{
	pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
}

void
pfgPrintRestartMsg(void)
{
	if (user_exit)
		(void) fprintf(stderr, PFG_MN_RESTART);

	user_exit = FALSE;
}

/*
 * exit the application as cleanly as possible
 * exit_data is not actually used in the GUI right now, but
 * must exist in the prototype so it matches the prototype for the
 * function passed into some of the upgrade routines.
 */
/* ARGSUSED */
void
pfgCleanExit(int exit_code, void *exit_data)
{
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
			write_debug(GUI_DEBUG_L1, "exitting child: %d",
				exit_code);
			switch (exit_code) {
			case EXIT_INSTALL_FAILURE:
				exit_code = ChildUpgExitFailure;
				break;
			case EXIT_INSTALL_SUCCESS_REBOOT:
				exit_code = ChildUpgExitOkReboot;
				break;
			case EXIT_INSTALL_SUCCESS_NOREBOOT:
				exit_code = ChildUpgExitOkNoReboot;
				break;
			}
			pfgPrintRestartMsg();
			pfgChildShutdown((TChildAction) exit_code);
		} else {
			/*
			 * We only want this done once while exitting
			 * upgrade (i.e. don't let both the parent and
			 * the child do it.)
			 */
			write_debug(GUI_DEBUG_L1, "exitting parent: %d",
				exit_code);
			(void) umount_and_delete_swap();
			pfgPrintRestartMsg();
		}
	} else {
		/* initial install */
		pfgPrintRestartMsg();
	}

	exit (exit_code);
}

/*
 * Function:	pfgParentReinit
 * Description:
 *	During upgrade we split off a child process to do some of the
 *	processing and which displays some of the parade windows.
 *	Apparently, whent eh child exits, it munges some of the X data
 *	that it inherited from the parent so that the parent can no longer
 *	successfully bring up more displays.
 *	So, upon return to the parent process, we have to
 *	reinitialize the X environment so that the parent can continue.
 *
 *	Reinitializing includes:
 *	- saving the original argc/argv passed into the app so that
 *	  any args parsed by X can be done so again in the X reinit
 *	  calls.
 *	- recall pfgInit to redo all X initialization stuff...
 *	- reinitialize the admin help stuff so that the admin help knows
 *	  to redisplay to the right parent.
 *	- reinitialize the additional info registered with the messaging
 *	  routines.
 *
 * Scope:	PUBLIC
 * Parameters:
 * Return:
 * Globals:
 * Notes:
 */
void
pfgParentReinit(void *reinit_data)
{
	int argc = 0;
	char **argv = NULL;

	if (reinit_data) {
		argc = ((TParentReinit *) reinit_data)->argc;
		argv = ((TParentReinit *) reinit_data)->argv;
	}

	pfgInit(&argc, argv);
	xm_adminhelp_reinit(FALSE);

	xmInfo.parent = NULL;
	xmInfo.toplevel = pfgTopLevel;
	xmInfo.app_context = pfgAppContext;
	xmInfo.delete_atom = pfgWMDeleteAtom;
	xmInfo.delete_func = pfgExit;

	XtRealizeWidget(pfgTopLevel);

}

/*
 * Function:	pfgChildShutdown
 * Description:
 *	Used to cleanly shut down the child process during an upgrade.
 *	The child calls this in order to exit appropriately.
 *	The main intent is close down various UI things so that the
 *	parent can successfully proceed to display everything OK.
 * Scope:	PUBLIC
 * Parameters:
 *	TChildAction exit_code:
 *		The exit code the child is to exit with.
 *		Thsi code is then interpreted by the parent.
 * Return:	none
 * Globals:
 *	- uses	pfgTopLevel
 * Notes:
 */
void
pfgChildShutdown(TChildAction exit_code)
{
	/* make sure the adminhelp window is history */
	xm_adminhelp_reinit(TRUE);

	XFlush(XtDisplay(pfgTopLevel));
	XSync(XtDisplay(pfgTopLevel), True);

	exit((int) exit_code);
}
