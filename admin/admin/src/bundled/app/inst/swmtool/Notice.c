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
#ident	"@(#)Notice.c 1.6 93/04/09"
#endif

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/notice.h>
#include "swmtool.h"
#include "defs.h"

void
NoticeSave(Frame owner)
{
	static int status;
	static char *msg, *msg_save, *msg_exit;

	if (msg == (char *)0) {
		msg = xstrdup(gettext(
		    "You have made changes to Software Manager's\n"
		    "configuration which you have not yet saved.\n"
		    "If you exit the program you will lose these\n"
		    "changes.  Do you wish to save these changes?"));
		msg_save = xstrdup(gettext("Save and Exit"));
		msg_exit = xstrdup(gettext("Exit without Saving"));
	}

	xv_create(owner, NOTICE,
		NOTICE_MESSAGE_STRING,	msg,
		NOTICE_BUTTON_YES,	msg_save,
		NOTICE_BUTTON_NO,	msg_exit,
		NOTICE_STATUS,		&status,
		XV_SHOW,		TRUE,
		NULL);

	switch (status) {
	case NOTICE_YES:
		break;
	case NOTICE_NO:
		break;
	}
}

void
NoticeNativeArch(Frame owner)
{
	static char *msg;

	if (msg == (char *)0)
		msg = xstrdup(gettext(
		    "Installed client support for any given OS release must\n"
		    "include the server's native architecture, i.e., you must\n"
		    "install the server's architecture when you add client\n"
		    "support for a new OS release and you cannot remove this\n"
		    "support once it is installed.  These restrictions will\n"
		    "be removed in a future release of Solaris 2.x.\n"));

	xv_create(owner, NOTICE,
		NOTICE_MESSAGE_STRING,	msg,
		NOTICE_BUTTON_YES,	gettext("Dismiss"),
		XV_SHOW,		TRUE,
		NULL);
}
