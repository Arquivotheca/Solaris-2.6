#ifndef lint
#pragma ident "@(#)c_progress.c 1.67 96/09/03 SMI"
#endif

/*
 * Copyright (c) 1992-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	c_progress.c
 * Group:	ttinstall
 * Description:
 */

/*
 * implements a `progress' screen for various flavors of initial install.
 * basically a status display showing what's going on...

00000000001111111111222222222233333333334444444444555555555566666666667777777777
01234567890123456789012345678901234567890123456789012345678901234567890123456789
00
01
02
03   Solaris Initial Install Progress Display
04
05
06              Installing:  OpenWindows Required Package
07
08
09            Time Elapsed:    1:24
10          Time Remaining:     :35 (estimated)
11
12
13        MBytes Installed:  100.00
14        MBytes Remaining:  100.00
15
16
17        ####################### -\|/
18        |           |           |           |           |           |
19        0          20          40          60          80         100
20
21
22
23(interrupted!  one more to abort installation ...)
00000000001111111111222222222233333333334444444444555555555566666666667777777777
01234567890123456789012345678901234567890123456789012345678901234567890123456789

 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <libintl.h>
#include <signal.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <time.h>

#include "pf.h"
#include "c_progress.h"
#include "v_types.h"
#include "v_sw.h"
#include "v_misc.h"

/*
 * local functions
 */
static void
SUInitialUpdateBeginCB(pfcSUInitialData *pfc_su_data);

static void
SUInitialUpdateEndCB(pfcSUInitialData *pfc_su_data);

static void
SUInitialPkgAddBeginCB(pfcSUInitialData *pfc_su_data, char *pkgdir);

static void
SUInitialPkgAddEndCB(pfcSUInitialData *pfc_su_data, char *pkgdir);

static void progress_display_pkg(char *);
static void progress_update_sizes(float, int);
static void progress_update_remaining(int);
#if 0
static void progress_update_elapsed(int);
#endif
static void progress_display_init(void);
static void progress_update_gauge(int);
static char *pkgid_from_pkgdir(char *);

static void set_itimer(int, int);
static void remove_itimer();

static void interrupt(int);
static void second_interrupt(int);
static void reset_interrupt(int);

static void start_prop();
static void update_prop(int);

static void progress_exec_sh(int);
static void (*orig_exec_sh) (int);

#ifdef debug
static void progress_update_thruput(float, long);

#endif

/*
 * display variables
 */
static int toprow;

static int disp_col;		/* starting column for all per pkg output */
static int mb_inst_row;
static int mb_rem_row;
/* static int time_elapsed_row; */
static int time_rem_row;
static int pkg_name_row;
static int hash_mark_row;

#ifdef debug
static int thruput_row = mb_rem_row;

#endif

static int g_min_col = 10;
static int g_max_col = 70;
static int g_cur_col = 10;

static time_t start;
static char propeller[5] = "-\\|/";
#define	stop_prop()	{ remove_itimer(); 	(void) sigignore(SIGALRM); }

/*
 * Function:	pfcSystemUpdateInitialCB
 * Description:
 *	Main top level SystemUpdate callback for the initial install path.
 * Scope:	INTERNAL
 * Parameters:
 *	void *client_data
 *		application data
 *	void *call_data
 *		SystemUpdate provided data
 * Return:
 *	SUCCESS
 *	FAILURE
 * Globals:	None
 * Notes:
 */
int
pfcSystemUpdateInitialCB(void *client_data, void *call_data)
{
	pfcSUInitialData *pfc_su_data =
		(pfcSUInitialData *) client_data;
	TSoftUpdateStateData *cb_data =
		(TSoftUpdateStateData *) call_data;

	if (!pfc_su_data || !cb_data)
		return (FAILURE);

	write_debug(CUI_DEBUG_L1, "SU Initial: State = %d", cb_data->State);

	switch (cb_data->State) {
	case SoftUpdateBegin:
		SUInitialUpdateBeginCB(pfc_su_data);
		break;
	case SoftUpdateEnd:
		SUInitialUpdateEndCB(pfc_su_data);
		break;
	case SoftUpdatePkgAddBegin:
		SUInitialPkgAddBeginCB(
			pfc_su_data,
			cb_data->Data.PkgAddBegin.PkgDir);
		break;
	case SoftUpdatePkgAddEnd:
		SUInitialPkgAddEndCB(
			pfc_su_data,
			cb_data->Data.PkgAddEnd.PkgDir);
		break;
	case SoftUpdateInteractivePkgAdd: /* currently unused, I think?? */
		/* send this debug info to the log file */
		write_debug(CUI_DEBUG_L1, "SoftUpdateInteractivePkgAdd");
		break;
	default:
		return (FAILURE);
	}

	return (SUCCESS);
}

/*
 * Function:	SUInitialUpdateBeginCB
 * Description:
 *	Initializes an initial install path 'progress' display.
 *	Clears the screen
 *	Displays a horizontally centered proportional gauge
 *	(ranging from 0% to 100%)
 *
 * Scope:	INTERNAL
 * Parameters:
 * Return:
 * Globals:
 * Notes:
 */
static void
SUInitialUpdateBeginCB(pfcSUInitialData *pfc_su_data)
{
	/*
	 * Now, we are past the disk
	 * partitioning phase, so make sure that any output
	 * is going to a log file, not the screen,
	 * and that curses is turned back on.
	 */
	(void) write_status_register_log(StatusScrFileName);
	(void) write_error_register_log(ErrWarnLogFileName);
	(void) write_warning_register_log(ErrWarnLogFileName);

	(void) start_curses();

	sw_lib_init(NULL);
	set_percent_free_space(0);
	set_memalloc_failure_func(progress_error);

	if (debug != 0)
		(void) sigset(SIGINT, interrupt);
	else
		(void) sigignore(SIGINT);	/* make sure its ignored */

	orig_exec_sh = signal(SIGQUIT, progress_exec_sh);
	orig_exec_sh = signal(SIGILL, progress_exec_sh);
	orig_exec_sh = signal(SIGTRAP, progress_exec_sh);
	orig_exec_sh = signal(SIGEMT, progress_exec_sh);
	orig_exec_sh = signal(SIGFPE, progress_exec_sh);
	orig_exec_sh = signal(SIGBUS, progress_exec_sh);
	orig_exec_sh = signal(SIGSEGV, progress_exec_sh);
	orig_exec_sh = signal(SIGSYS, progress_exec_sh);

	/*
	 * initialize the progress display
	 */
	progress_display_init();

	pfc_su_data->totalK = pfc_su_data->remainingK =
		v_get_total_kb_to_install();
	pfc_su_data->doneK = 0;

	start_prop();

#if 0
	progress_update_elapsed(0);
#endif

	progress_update_sizes(pfc_su_data->doneK, pfc_su_data->remainingK);

	start = time((time_t) NULL);

}

/*
 * Function:	SUInitialUpdateEndCB
 * Description:
 *	Close down an initial install path 'progress' display.
 * Scope:	INTERNAL
 * Parameters:
 * Return:
 * Globals:
 * Notes:
 */
static void
SUInitialUpdateEndCB(pfcSUInitialData *pfc_su_data)
{
	/*
	 * make it look like the install is done
	 */
	(void) wtimeout(stdscr, -1);
	stop_prop();
	progress_display_pkg("");
	progress_update_sizes(pfc_su_data->totalK, 0);
	progress_update_remaining(0);
	progress_update_gauge(100);

	doupdate();
	stop_prop();
	(void) sleep(0);

	(void) signal(SIGQUIT, orig_exec_sh);
	(void) signal(SIGILL, orig_exec_sh);
	(void) signal(SIGTRAP, orig_exec_sh);
	(void) signal(SIGEMT, orig_exec_sh);
	(void) signal(SIGFPE, orig_exec_sh);
	(void) signal(SIGBUS, orig_exec_sh);
	(void) signal(SIGSEGV, orig_exec_sh);
	(void) signal(SIGSYS, orig_exec_sh);

	(void) wstandend(stdscr);
	(void) wclear(stdscr);

	(void) write_status_register_log(NULL);
	(void) write_error_register_log(NULL);
	(void) write_warning_register_log(NULL);
	end_curses(TRUE, FALSE);

	/*
	 * SystemUpdate will still be doing printf()'s,
	 * want to make sure output is line buffered.
	 */
	(void) setvbuf(stdout, NULL, _IOLBF, 0);
	(void) setvbuf(stderr, NULL, _IOLBF, 0);
}

/*
 * Function:	SUInitialPkgAddBeginCB
 * Description:
 *	initial install path callback
 *	called just before pkgadd is started on pkg in `pkgdir'
 * Scope:	INTERNAL
 * Parameters:
 * Return:
 * Globals:
 * Notes:
 */
/* ARGSUSED */
static void
SUInitialPkgAddBeginCB(pfcSUInitialData *pfc_su_data, char *pkgdir)
{
	char *current_pkgid;

	current_pkgid = xstrdup(pkgid_from_pkgdir(pkgdir));
	progress_display_pkg(current_pkgid);
}

/*
 * Function:	SUInitialPkgAddBeginCB
 * Description:
 *	initial install path callback
 *	called just after pkgadd completes for pkg in `pkgdir'
 *
 *	get end time & do various computations deriving metrics
 *	of interest.
 *
 *	update display with derived information.
 * Scope:	INTERNAL
 * Parameters:
 * Return:
 * Globals:
 * Notes:
 */
static void
SUInitialPkgAddEndCB(pfcSUInitialData *pfc_su_data, char *pkgdir)
{
	int size;
	char *cp;

	cp = pkgid_from_pkgdir(pkgdir);
	size = v_get_size_in_kbytes(cp);

	/*
	 * compute remaining time
	 */
	pfc_su_data->remainingK -= size;

	if (pfc_su_data->remainingK < 0)
		pfc_su_data->remainingK = 0;

	pfc_su_data->doneK += size;

	/*
	 * update time remaining display Time remaining and time elapsed are
	 * updated after each pkg. The time remaining is estimated using the
	 * average thruput (KB/sec) acheived so far and the remaining amount
	 * of software to be installed.  This figure may oscillate as the
	 * average thruput changes.
	 */

	progress_update_sizes(pfc_su_data->doneK, pfc_su_data->remainingK);

	progress_update_gauge((int) (100 * ((float) pfc_su_data->doneK /
		    (float) pfc_su_data->totalK)));
	touchwin(stdscr);
	(void) wnoutrefresh(stdscr);

	if (debug) {
		/* sleep(1) does not work here for some reason... */
		int i;
		for (i = 0; i < 5000000; i++);
	}
}

/*
 * signal handler wrapped around v_exec_sh(). need to turn off timer before
 * exiting.
 */
static void
progress_exec_sh(int sig)
{
	stop_prop();
	(void) sleep(0);

	(void) wtimeout(stdscr, -1);
	v_exec_sh(sig);
}


/*
 * this gives IBE a way to error out cleanly...
 */
void
progress_error(int err)
{
	stop_prop();
	(void) sleep(0);

	progress_cleanup();

	/* exit with failure status */
	v_int_error_exit(err);
}

void
progress_cleanup()
{

	(void) wtimeout(stdscr, -1);
	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGALRM, SIG_DFL);

	end_curses(FALSE, FALSE);
}

static void
progress_display_init(void)
{
	int l_width;

	l_width = 28;

	disp_col = l_width + 2;

	touchwin(stdscr);
	wclear(stdscr);
	werase(stdscr);

	toprow = (LINES - 16) / 2;

	mb_inst_row = toprow + 3;
	mb_rem_row = toprow + 4;
	/*
	time_elapsed_row = toprow + 6;
	time_rem_row = toprow + 7;
	 */
	pkg_name_row = toprow + 6;
	hash_mark_row = toprow + 11;

	(void) mvwprintw(stdscr, toprow, 5, "%-.*s", (COLS - 10),
	/* i18n: 65 chars max */
	    gettext("Solaris Initial Install"));

	(void) mvwprintw(stdscr, mb_inst_row, 0, "%*.*s:",
	/* i18n: 28 chars max */
	    l_width, l_width, gettext("MBytes Installed"));

	(void) mvwprintw(stdscr, mb_rem_row, 0, "%*.*s:",
	/* i18n: 28 chars max */
	    l_width, l_width, gettext("MBytes Remaining"));

	(void) mvwprintw(stdscr, pkg_name_row, 0, "%*.*s:",
	/* i18n: 28 chars max */
	    l_width, l_width, gettext("Installing"));

	(void) mvwprintw(stdscr, hash_mark_row + 1, 10,
	    "|           |           |           |           |           |");
	(void) mvwprintw(stdscr, hash_mark_row + 2, 10,
	    "0          20          40          60          80         100");

	(void) wnoutrefresh(stdscr);

}

static void
progress_display_pkg(char *pkgid)
{
	char *pkgname = v_get_pkgname_from_pkgid(pkgid);

	stop_prop();
	(void) wmove(stdscr, pkg_name_row, disp_col);
	(void) wclrtoeol(stdscr);

	/* pkgname is a max of 256 chars, allow 5 lines */
	wmove(stdscr, pkg_name_row, disp_col);
	wclrtoeol(stdscr);
	wmove(stdscr, pkg_name_row + 1, disp_col);
	wclrtoeol(stdscr);
	wmove(stdscr, pkg_name_row + 2, disp_col);
	wclrtoeol(stdscr);
	wmove(stdscr, pkg_name_row + 3, disp_col);
	wclrtoeol(stdscr);
	wmove(stdscr, pkg_name_row + 4, disp_col);
	wclrtoeol(stdscr);

	(void) wword_wrap(stdscr, pkg_name_row, disp_col, 50,
	    pkgname ? pkgname : "");

	(void) wnoutrefresh(stdscr);
	start_prop();
}

static void
progress_update_sizes(float inst_kb, int rem_kb)
{
	stop_prop();
	(void) mvwprintw(stdscr, mb_inst_row, disp_col, "%8.2f",
	    inst_kb / KBYTE);
	(void) mvwprintw(stdscr, mb_rem_row, disp_col, "%8.2f",
	    (float) rem_kb / KBYTE);

	(void) wnoutrefresh(stdscr);
	start_prop();
}

static void
progress_conv_time(int elapsed, int *hrs, int *min, int *sec)
{
	register int minutes;

	minutes = (int) (elapsed / 60);

	*hrs = (int) (minutes / 60);
	*min = (int) (minutes % 60);
	*sec = (int) (elapsed % 60);

	if (*hrs <= 0)
		*hrs = 0;

	if (*min < 0)
		*min = 0;

	if (*sec < 0)
		*sec = 0;
}

static void
progress_update_remaining(int remaining)
{
	int hrs;
	int min;
	int sec;

	/* convert time remaining  */
	progress_conv_time(remaining, &hrs, &min, &sec);

	stop_prop();
	(void) wmove(stdscr, time_rem_row, disp_col);
	(void) wclrtoeol(stdscr);
	(void) mvwprintw(stdscr, time_rem_row, disp_col, "%02d:%02d:%02d",
	    hrs, min, sec);

	(void) wnoutrefresh(stdscr);
	start_prop();

}

#if 0
static void
progress_update_elapsed(int elapsed)
{
	/*
	 * removed per bug id 1188269 - the GUI doesn't have elapsed time,
	 * so the CUI shouldn't either - it serves no purpose.
	 * I chose not to remove it completely, just in case someone
	 * decides that they really want it back.
	 */

	int hrs;
	int min;
	int sec;


	/* convert time elapsed  */
	progress_conv_time(elapsed, &hrs, &min, &sec);

	stop_prop();
	(void) wmove(stdscr, time_elapsed_row, disp_col);
	(void) wclrtoeol(stdscr);
	(void) mvwprintw(stdscr, time_elapsed_row, disp_col, "%02d:%02d:%02d",
	    hrs, min, sec);

	(void) wnoutrefresh(stdscr);
	start_prop();

}
#endif

/*
 * void progress_update_gauge(int val)
 *
 * The proportional indicator is updated with the specified percentage. `val'
 * must be in the range 0-100 inclusive.
 *
 */
void
progress_update_gauge(int val)
{
	static int width = 0;
	static int lastval;
	int proportion;

	if (width == 0)
		width = g_max_col - g_min_col;

	/*
	 * update gauge only if new value is greater... hopefully this
	 * avoids any `backwards' movement of the gauge which makes it look
	 * like the install is regressing
	 */
	if (val > lastval && val <= 100) {

		proportion = (int) ((float) width * ((float) val / 100.0));

		stop_prop();
		wcolor_on(stdscr, CURSOR);
		(void) mvwprintw(stdscr, hash_mark_row, g_min_col, "%*.*s ",
		    proportion, proportion, " ");
		wcolor_off(stdscr, CURSOR);

		lastval = val;

		/*
		 * only need this for the spinning propellor
		 */
		g_cur_col = g_min_col + proportion + 1;

		wnoutrefresh(stdscr);
		start_prop();
	}
}


/*
 * get fullpath to pkg: need to trim off leading path to get to pkgid if
 * pkgid has an extension (XXX.[cmde]), need to chop this off too.
 */
static char *
pkgid_from_pkgdir(char *path)
{
	char *cp;
	static char buf[MAXPATHLEN];

	/*
	 * path = "/blah/blah/blah/pkgdir[.{c|e|d|m|...}] ^ | cp
	 * --------------------+
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

/* -------------- interrupt (^C) handling -------------- */
extern int errno;
/* ARGSUSED */
static void
interrupt(int sig)
{
	/* i18n: 75 chars max */
	(void) mvwprintw(stdscr, LINES - 1, 0, "%-.75s",
	    gettext("Interrupt!  One more to abort... "));
	(void) wrefresh(stdscr);

	if (signal(SIGINT, second_interrupt) == SIG_ERR)
		(void) mvwprintw(stdscr, LINES - 1, 0, "can't set SIGINT: %d",
		    errno);

	set_itimer(5, 0);

	if (signal(SIGALRM, reset_interrupt) == SIG_ERR)
		(void) mvwprintw(stdscr, LINES - 1, 0, "can't set SIGALRM: %d",
		    errno);
}

/* ARGSUSED */
static void
second_interrupt(int sig)
{
	remove_itimer();
	(void) wmove(stdscr, LINES - 1, 0);
	(void) wclrtoeol(stdscr);
	(void) wrefresh(stdscr);

	if (debug != 0) {
		progress_cleanup();
		v_exec_sh(0);
	}
}

/* ARGSUSED */
static void
reset_interrupt(int sig)
{
	/* restart prop */
	(void) wmove(stdscr, LINES - 1, 0);
	(void) wclrtoeol(stdscr);
	(void) wrefresh(stdscr);
	start_prop();

	(void) sigset(SIGINT, interrupt);	/* reset interrupt handler */
}

/* ------ spinning propeller.... ------------ */

static void
start_prop()
{

	set_itimer(0, 333333);	/* 1/3 sec */
	(void) sigset(SIGALRM, &update_prop);
}

/*
 * spins propeller, each time called, displays next character of prop
 * sequence.
 */
/* ARGSUSED */
static void
update_prop(int sig)
{
	static unsigned int cp = 0;
	static int delta = 0;
	time_t end;
	int ch;

	end = time((time_t) NULL);

	(void) sighold(SIGCHLD);

	/*
	 * It's necessary to turn the keypad off here in order to ignore
	 * escape sequences. These sequences will cause an alarm timeout,
	 * killing the installation in progress. The only input we want to
	 * catch is '\014' (CTRL-L), which causes a screen refresh.
	 */
	keypad(stdscr, 0);

	(void) wtimeout(stdscr, 1);
	if ((ch = wgetch(stdscr)) != ERR && ch == '\014') {
		(void) touchwin(curscr);
		(void) wrefresh(curscr);
		(void) flushinp();
	}
	(void) wtimeout(stdscr, -1);

	/*
	 * elapsed time is time since we start pkgadding only update every
	 * 15 propeller twirls....
	 */
	if (++delta == 15) {
		delta = (int) (end - start);
#if 0
		progress_update_elapsed(delta);
#endif

		delta = 0;
	}

	/*
	 * spinning prop...
	 */
	(void) mvwaddch(stdscr, hash_mark_row, g_cur_col, propeller[cp++ % 4]);
	(void) wmove(stdscr, hash_mark_row, g_cur_col);
	(void) wnoutrefresh(stdscr);
	(void) doupdate();

	(void) setsyx(hash_mark_row, g_cur_col);

	(void) sigrelse(SIGCHLD);
}

static void
set_itimer(int sec, int usec)
{
	struct timeval val;
	struct itimerval itval;

	remove_itimer();

	val.tv_sec = (long) sec;
	val.tv_usec = (long) usec;

	itval.it_interval = val;
	itval.it_value = val;

	(void) setitimer(ITIMER_REAL, &itval, (struct itimerval *) NULL);

}

static void
remove_itimer()
{
	struct timeval val;
	struct itimerval itval;

	/*
	 * setting these to 0 should disable the timer
	 */
	val.tv_sec = (long) 0;
	val.tv_usec = (long) 0;

	itval.it_interval = val;
	itval.it_value = val;

	(void) setitimer(ITIMER_REAL, &itval, (struct itimerval *) NULL);
}
