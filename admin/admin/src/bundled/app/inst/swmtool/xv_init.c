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
#ident	"@(#)xv_init.c 1.18 94/08/26"
#endif

#include "defs.h"
#include "ui.h"
#include <xview/cms.h>
#include "swmtool.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#ifdef SVR4
#include <netdb.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#endif

Space	**meter;		/* global space meter */

char	*progname;
char	*openwinhome;
char	thishost[MAXHOSTNAMELEN];

char	*device_name;		/* initial load device */

int	browse_mode;		/* if != 0, browse (read-only) mode */
int	swm_eject_on_exit;	/* if != 0, eject mounted CD before exit */
int	verbose;		/* if != 0, print all error messages */

u_long	Basescreen;
u_long	Loadscreen;
u_long	Adminscreen;
u_long	Hostscreen;
u_long	Termscreen;

Display	*display;
int	use_color;

#ifdef __STDC__
extern Notify_value cleanup(Notify_client, Destroy_status);

static Notify_value onsig(Notify_client, int, Notify_signal_mode);
static void usage(void);
#else
static void onsig();
static void usage();
#endif

static void
#ifdef __STDC__
usage(void)
#else
usage()
#endif
{
	(void) fprintf(stderr, gettext(
	    "Usage: %s [ -e ] [ -d directory ] [ -c config_file ]\n"),
		progname);
}

static void
panic(int err)
{
	fprintf(stderr, gettext("PANIC:  %s\n"), get_err_str(err));
	xv_destroy(Base_BaseWin->BaseWin);
	exit(-1);
}

void
InitMain(argc, argv)
	int	argc;
	char	**argv;
{
	char	hostname[MAXHOSTNAMELEN];
	char	*cp;
	int	c;
#ifndef SVR4
	extern char *optarg;
#endif
	char	*helppath, buf[BUFSIZ];

#ifndef TEXT_DOMAIN
#define	TEXT_DOMAIN	"SYS_TEST"
#endif
	(void) textdomain(TEXT_DOMAIN);

	(void) sprintf(buf, "HELPPATH=%.*s", sizeof (buf) - 1, SWM_LOCALE_DIR);
	helppath = getenv("HELPPATH");
	if (helppath != (char *)0)
		(void) sprintf(&buf[strlen(buf)], ":%.*s",
			sizeof (buf) - (strlen(buf) + 1), helppath);
	helppath = xstrdup(buf);
	(void) putenv(helppath);

	cp = strrchr(argv[0], '/');
	if (cp == (char *)0)
		progname = argv[0];
	else
		progname = ++cp;

	if (geteuid() != 0)
		browse_mode = 1;

	/*
	 * Set up global "thishost" to contain
	 * our canonicalized host name.  All host
	 * name checks use canonical form.
	 */
	(void) sysinfo(SI_HOSTNAME, hostname, sizeof (hostname));
	(void) strcpy(thishost, host_canon(hostname));

	/* parse args */
	while ((c = getopt(argc, argv, "c:ed:nvb")) != EOF)
	{
		switch (c) {
		case 'b':	/* browse (read-only) mode */
			browse_mode = 1;
			break;
		case 'c':	/* configuration file */
			(void) config_file(optarg);
			break;
		case 'e':	/* eject mounted CD on exit */
			swm_eject_on_exit = 1;
			break;
		case 'd':	/* initial load device/directory */
			device_name = optarg;
			break;
		case 'v':	/* verbose -- print all error messages */
			verbose = 1;
			break;
		case '?':
			usage();
			exit(1);
		}
	}

	(void) notify_set_signal_func(
		Base_BaseWin->BaseWin, onsig, SIGINT, NOTIFY_SYNC);
	(void) notify_set_signal_func(
		Base_BaseWin->BaseWin, onsig, SIGQUIT, NOTIFY_SYNC);

	(void) notify_interpose_destroy_func(Base_BaseWin->BaseWin, cleanup);

	display = (Display *)xv_get(Base_BaseWin->BaseWin, XV_DISPLAY);
	use_color = ((int)xv_get(Base_BaseWin->BaseWin, WIN_DEPTH) > 1) ? 1 : 0;

	/*
	 * Initialize global window handles
	 * for message output
	 */
	Basescreen = Base_BaseWin->BaseWin;
	Loadscreen = Props_PropsWin->PropsWin;
	Adminscreen = Props_PropsWin->PropsWin;
	Hostscreen = Props_PropsWin->PropsWin;
	Termscreen = Cmd_CmdWin->CmdWin;

	BaseResize((Xv_opaque)Base_BaseWin);	/* size buttons */
	notify_dispatch();
	/*
	 * Render at least the base window,
	 * so we can start showing status
	 * in its footer.
	 */
	xv_set(Base_BaseWin->BaseWin,
		XV_SHOW,	TRUE,
		FRAME_BUSY,	TRUE,
		NULL);
	XFlush(display);
	notify_dispatch();

	(void) msg(Basescreen, TRUE,
		gettext("Initializing interface components..."));
	notify_dispatch();
	/*
	 * Initialize software library
	 */
	sw_lib_init(panic, PTYPE_UNKNOWN, NO_EXTRA_SPACE);
	/*
	 * Initialize administrative defaults
	 */
	config_init();
	notify_dispatch();
	/*
	 * Initialize various pieces of the UI
	 */
	InitConfig((caddr_t)Props_PropsWin);
	InitModules((caddr_t)Base_BaseWin, Base_BaseWin->BaseWin);
	InitLevels((caddr_t)Base_BaseWin, Base_BaseWin->BaseWin);
	InitHosts((caddr_t)Props_PropsWin);
	notify_dispatch();
	/*
	 * If the user specified an initial
	 * source software distribution via
	 * the "-d" option, load it.
	 */
	if (device_name != (char *)0) {
		(void) msg(Basescreen, TRUE,
			gettext("Checking source software distribution..."));
		if (LoadMedia(MOUNTED_FS, (char *)0, device_name, 0) < 0)
			device_name = (char *)0;
		notify_dispatch();
	}

	/*
	 * Get the information about software
	 * installed on the local system.
	 */
	(void) msg(Basescreen, TRUE,
		gettext("Checking installed software..."));
	load_installed("/", FALSE);
	notify_dispatch();

	reset_selections(1, 1, 1);

	UpdateMeter(0);

	BrowseModules(MODE_UNSPEC, VIEW_UNSPEC);

	xv_set(Base_BaseWin->BaseWin, FRAME_BUSY, FALSE, NULL);
}

/*ARGSUSED*/
static Notify_value
onsig(client, sig, when)
	Notify_client	client;
	int		sig;
	Notify_signal_mode when;
{
	sig = notify_get_signal_code();

	if (Base_BaseWin)
		xv_destroy(Base_BaseWin->BaseWin);

/*	notify_stop(); */

	if (sig == SIGQUIT || sig == SIGSEGV ||
	    sig == SIGBUS || sig == SIGFPE) {
		fprintf(stderr,
		    gettext("PANIC:  signal(%d) -- aborting!\n"), sig);
		abort();
	}
	exit(sig);
#ifdef lint
	return (NOTIFY_DONE);
#endif
}

#define	OPENWINHOME	"/usr/openwin"

void
init_usr_openwin(void)
{
	char	env[MAXPATHLEN+32];

	openwinhome = getenv("OPENWINHOME");
	if (openwinhome == (char *)0) {
		openwinhome = xstrdup(OPENWINHOME);
		(void) sprintf(env, "OPENWINHOME=%s", OPENWINHOME);
		(void) putenv(xstrdup(env));
	}
}
