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
#ident	"@(#)xv_subr.c 1.13 94/10/13"
#endif

#include "defs.h"
#include "ui.h"
#include <xview/notice.h>
#include <xview/ttysw.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "swmtool.h"

/*
 * msg() -- should write a line of output to where?
 *	command window, popup, or what?
 *	XXX needs work
 */
#ifdef __STDC__
void
msg(Xv_opaque owner, int new, char *fmt, ...)
{
	va_list ap;
	char	buf[BUFSIZ];

	va_start(ap, fmt);
#else
void
msg(va_alist)
	va_dcl
{
	va_list ap;
	Xv_opaque owner;
	int	new;
	char	*fmt;
	char	buf[BUFSIZ];

	va_start(ap);
	owner = va_arg(ap, Xv_opaque);
	new = va_arg(ap, int);
	fmt = va_arg(ap, char *);
#endif
	if (fmt != (char *)0)
		(void) vsprintf(buf, fmt, ap);
	else
		buf[0] = '\0';
	va_end(ap);

	if (owner == (Xv_opaque)0 || xv_get(owner, XV_SHOW) == FALSE)
		owner = Basescreen;

	if (tty_is_active() && owner == Termscreen) {
		/*
		 * XXX  Xview bug workaround
		 *
		 * Hack to work around bug in ttysw_output
		 * A single newline doesn't get expanded to
		 * cr-lf, so expand it ourselves.
		 */
		if (strcmp(buf, "\n") == 0)
			(void) strcpy(buf, " \n");
		/*
		 * Send to tty window
		 */
		ttysw_output(Cmd_CmdWin->Term, buf, strlen(buf));
	} else if (new) {
		/*
		 * Initialize owner's footer
		 */
		xv_set(owner, FRAME_LEFT_FOOTER, buf, 0);
	} else {
		/*
		 * Add to owner's footer
		 */
		char *footer = (char *)xv_get(owner, FRAME_LEFT_FOOTER);
		int	flen = footer ? strlen(footer) : 0;

		(void) memmove((void *)(buf + flen), (void *)buf,
			strlen(buf) + 1);
		(void) memcpy((void *)buf, (void *)footer, flen);
		xv_set(owner, FRAME_LEFT_FOOTER, buf, 0);
	}
}

/*
 * die() - print an informative message, then exit
 */
/* VARARGS1 */
void
#ifdef __STDC__
die(char *format, ...)
{
	va_list	args;

	va_start(args, format);
#else
die(va_alist)
	va_dcl
{
	va_list	args;
	char	*format;

	va_start(args);
	/*LINTED [alignment ok]*/
	format = va_arg(args, char *);
#endif
	(void) fprintf(stderr, "%s:  ", progname);
	(void) vfprintf(stderr, format, args);
	va_end(args);
	xv_destroy(Base_BaseWin->BaseWin);
	exit(-1);
}

Notify_value
cleanup(client, status)
	Notify_client	client;
	Destroy_status	status;
{
	Module	*media;
	char	*spooldir = get_spooldir();

	if (status == DESTROY_CLEANUP) {
		(void) rumount_fs(spooldir);
		(void) unshare_fs(spooldir);
		media = get_source_media();
		if (media != (Module *)0) {
			set_eject_on_exit(swm_eject_on_exit);
			unload_media(media);
			set_source_media((Module *)0);
		}
		/*
		 * XXX Workaround for bug 1104493
		 *
		 * Any text item with a non-zero PANEL_MASK_CHAR
		 * will blow its cookies in realfree() when the
		 * object is destroyed.  This bug occurs when
		 * running against the JLE openwindows xview.
		 * Remove this comment and the following line
		 * when this bug is fixed.
		 */
		xv_set(Props_PropsWin->HostPwd, PANEL_MASK_CHAR, 0, 0);
		return (notify_next_destroy_func(client, status));
	}
	return (NOTIFY_DONE);
}

#ifdef __STDC__
void
asktoproceed(Xv_opaque owner, char *fmt, ...)
{
	va_list		ap;
	char		buf[BUFSIZ];
	Xv_notice	notice;

	va_start(ap, fmt);
#else
void
asktoproceed(va_alist)
	va_dcl
{
	va_list		ap;
	Xv_opaque	owner;
	char		*fmt;
	char		buf[BUFSIZ];
	Xv_notice	notice;

	va_start(ap);
	owner = va_arg(ap, Xv_opaque);
	fmt = va_arg(ap, char *);
#endif

	if (fmt != (char *)0)
		(void) vsprintf(buf, fmt, ap);
	va_end(ap);

	if (owner == (Xv_opaque)0 || xv_get(owner, XV_SHOW) == FALSE)
		owner = Base_BaseWin->BaseWin;

	notice = xv_create(owner, NOTICE,
		NOTICE_MESSAGE_STRING,	buf,
		NOTICE_BUTTON_YES,	gettext("Dismiss Notice"),
		XV_SHOW,		TRUE,
		NULL);

	xv_destroy_safe(notice);
}

#ifdef __STDC__
int
confirm(Xv_opaque owner, char *yes, char *no, char *fmt, ...)
{
	va_list		ap;
	char		buf[BUFSIZ];
	Xv_notice	notice;
	int		status;

	va_start(ap, fmt);
#else
void
confirm(va_alist)
	va_dcl
{
	va_list		ap;
	Xv_opaque	owner;
	char		*yes;
	char		*no;
	char		*fmt;
	char		buf[BUFSIZ];
	Xv_notice	notice;
	int		status;

	va_start(ap);
	yes = va_arg(ap, char *);
	no = va_arg(ap, char *);
	fmt = va_arg(ap, char *);
#endif

	if (fmt != (char *)0)
		(void) vsprintf(buf, fmt, ap);
	va_end(ap);

	if (owner == (Xv_opaque)0 || xv_get(owner, XV_SHOW) == FALSE)
		owner = Base_BaseWin->BaseWin;

	notice = xv_create(owner, NOTICE,
		NOTICE_MESSAGE_STRING,	buf,
		NOTICE_BUTTON_YES,	yes,
		NOTICE_BUTTON_NO,	no,
		NOTICE_STATUS,		&status,
		XV_SHOW,		TRUE,
		NULL);

	xv_destroy_safe(notice);

	free(yes);
	free(no);
	free(fmt);

	if (status == NOTICE_YES)
		return (1);
	return (0);
}

void *
icon_load_x11bm(path, errstr)
	char	*path;
	char	*errstr;
{
	Server_image	simage;

	simage = (Server_image) xv_create(NULL, SERVER_IMAGE,
	    SERVER_IMAGE_BITMAP_FILE,	path,
	    NULL);

	if (simage == (Server_image)0 && errstr)
		(void) fprintf(stderr, "icon_load_x11bm:  %s:  %s\n",
			path, errstr);
	return ((void *)simage);
}

int
server_image_xid(image, height, width)  /* Create server image from bitmap */
	u_short	*image;
	int	height;
	int	width;
{
	Server_image si;

	si = (Server_image)xv_create(0, SERVER_IMAGE,
		XV_WIDTH, width,
		XV_HEIGHT, height,
		SERVER_IMAGE_BITS, image,
		0);

	if (si == (Server_image)0)
		die(gettext("Internal Error:  can't create server image"));

	return ((int)xv_get(si, XV_XID));
}

/*
 * Many pieces of the toolkit produce extraneous (and
 * sometimes completely bogus) error messages.  Only
 * display output of perror if we're running with non-
 * fatal error messages on (verbose).  This is gross
 * but is the only way to suppress the damn things.
 */
void
#ifdef __STDC__
perror(const char *string)
#else
perror(string)
	char	*string;
#endif
{
	if (verbose) {
		int	saverr = errno;		/* just in case */

		if (string && *string)
			(void) fprintf(stderr, "%s: %s\n",
				string, strerror(errno));
		else
			(void) fprintf(stderr, "%s\n", strerror(errno));
		errno = saverr;
	}
}
