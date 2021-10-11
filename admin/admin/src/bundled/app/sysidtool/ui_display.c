/*
 * Copyright (c) 1993-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)ui_display.c 1.9 96/02/05"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/conf.h>
#include <stdlib.h>
#include <unistd.h>
#include "sysid_ui.h"
#include "sysid_msgs.h"

#define	XM_LIBRARY_PATH		"sysidxm"
#define	TTY_LIBRARY_PATH	"sysidtty"

void
init_display(int *argcp, char **argv, char *fifo_dir)
{
	char		*display;	/* X11 display name */
	Sysid_err	status;
	char		*dl_path;
	char		*tty_path = TTY_LIBRARY_PATH;
	char		fifo_path[MAXPATHLEN];
	char		**av;
	int		to, from;
	int		ac;

	(void) setsid();

	dl_path = (char *)0;

	ac = *argcp;
	av = argv;
	/*
	 * NB:  can't use getopt processing here
	 * because we may be invoking the GUI
	 * and passing in toolkit initialization
	 * arguments.
	 */
	while (--ac && **++av == '-') {
		switch ((*av)[1]) {
		case 'u':	/* path to user interface module */
			ac--;
			dl_path = *++av;
			break;
		default:
			break;
		}
	}
	if (dl_path == (char *)0) {
		display = getenv("DISPLAY");
		if (display != (char *)0 && *display != '\0')
			dl_path = XM_LIBRARY_PATH;
		else
			dl_path = tty_path;
	}
	/*
	 * Open the FIFOs through which we'll
	 * be communicating with sysidtool
	 */
	(void) sprintf(fifo_path, "%s/%s", fifo_dir, SYSID_UI_FIFO_IN);
	from = open(fifo_path, O_RDONLY);

	(void) sprintf(fifo_path, "%s/%s", fifo_dir, SYSID_UI_FIFO_OUT);
	to = open(fifo_path, O_WRONLY);

	/*
	 * Now open the appropriate dynamic
	 * library and start accepting messages.
	 */
	status = dl_init(dl_path, argcp, argv, from, to);
	if ((status == SYSID_ERR_DLOPEN_FAIL ||
	    status == SYSID_ERR_XTINIT_FAIL) &&
	    strcmp(dl_path, tty_path) != 0) {
		status = dl_init(tty_path, argcp, argv, from, to);
	}
	exit(status);
}

void
run_display(MSG *mp, int reply_to)
{
#ifdef MSGDEBUG
	msg_dump(mp);
#endif /* MSGDEBUG */
	switch (msg_get_type(mp)) {
	case GET_LOCALE:
		ui_get_locale(mp, reply_to);
		break;
	case GET_TERMINAL:
		dl_get_terminal(mp, reply_to);
		break;
	case GET_HOSTNAME:
		ui_get_hostname(mp, reply_to);
		break;
	case GET_NETWORKED:
		ui_get_networked(mp, reply_to);
		break;
	case GET_PRIMARY_NET:
		ui_get_primary_net_if(mp, reply_to);
		break;
	case GET_HOSTIP:
		ui_get_hostIP(mp, reply_to);
		break;
	case GET_CONFIRM:
		ui_get_confirm(mp, reply_to);
		break;
	case GET_NAME_SERVICE:
		ui_get_name_service(mp, reply_to);
		break;
	case GET_DOMAIN:
		ui_get_domain(mp, reply_to);
		break;
	case GET_BROADCAST:
		ui_get_broadcast(mp, reply_to);
		break;
	case GET_NISSERVERS:
		ui_get_nisservers(mp, reply_to);
		break;
	case GET_SUBNETTED:
		ui_get_subnetted(mp, reply_to);
		break;
	case GET_NETMASK:
		ui_get_netmask(mp, reply_to);
		break;
	case GET_BAD_NIS:
		ui_get_bad_nis(mp, reply_to);
		break;
	case GET_TIMEZONE:
		ui_get_timezone(mp, reply_to);
		break;
	case GET_DATE_AND_TIME:
		ui_get_date(mp, reply_to);
		break;
	case GET_PASSWORD:
		dl_get_password(mp, reply_to);
		break;
	case SET_LOCALE:
		ui_set_locale(mp, reply_to);
		break;
	case SET_TERM:
		ui_set_term(mp, reply_to);
		break;
	case DISPLAY_MESSAGE:
		dl_do_message(mp, reply_to);
		break;
	case DISMISS_MESSAGE:
		dl_do_dismiss(mp, reply_to);
		break;
	case ERROR:
		ui_error(mp, reply_to);
		break;
	case CLEANUP:
		ui_cleanup(mp, reply_to);
		break;
	default:
		{
			char	buf[256];

			(void) sprintf(buf, dgettext(TEXT_DOMAIN,
			    "warning: unknown message code %d\n"),
				msg_get_type(mp));
			reply_error(SYSID_ERR_OP_UNSUPPORTED, buf, reply_to);
		}
	}
}
