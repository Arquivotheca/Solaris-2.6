#ifndef DL_MSGS_H
#define	DL_MSGS_H

/*
 * Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
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

/*
 * This file contains text strings generic to the dynamic library interface.
 *
 * The strings are organized by screen.  Each screen is responsible
 * for entry or confirmation of one or more attributes, and typically
 * has strings of the following types:
 *
 *	*_TITLE		The title at the top of the screen
 *	*_TEXT		The summary text for the screen
 *	*_PROMPT	The prompt string for an attribute
 *	*_CONFIRM	The name of the attribute for confirmation screens
 *
 * In addition, attributes with multiple choices (menus) will have
 * a string per choice.  If the menu and confirmation strings for
 * these choices are different, the will be differentiated as
 * *_PROMPT_* and *_CONFIRM_*.
 */

#pragma	ident	"@(#)dl_msgs.h 1.2 93/10/14"

#include "sysid_msgs.h"

#endif DL_MSGS_H
