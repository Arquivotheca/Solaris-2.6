/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#ident	"@(#)swmtool.c 1.7 92/12/17"
#endif

/*
 * swmtool.c - Contains main() for project swmtool
 * This file was generated by `gxv'.
 */

#include "defs.h"
#include "ui.h"
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include "Base_ui.h"
#include "Client_ui.h"
#include "Cmd_ui.h"
#include "File_ui.h"
#include "Meter_ui.h"
#include "Pkg_ui.h"
#include "Props_ui.h"


/*
 * External variable declarations.
 */
Base_BaseWin_objects	*Base_BaseWin;
Client_ClientWin_objects	*Client_ClientWin;
Cmd_CmdWin_objects	*Cmd_CmdWin;
File_FileWin_objects	*File_FileWin;
Meter_MeterWin_objects	*Meter_MeterWin;
Pkg_ProdWin_objects	*Pkg_ProdWin;
Pkg_PkgWin_objects	*Pkg_PkgWin;
Props_PropsWin_objects	*Props_PropsWin;

#ifdef MAIN

/*
 * Instance XV_KEY_DATA key.  An instance is a set of related
 * user interface objects.  A pointer to an object's instance
 * is stored under this key in every object.  This must be a
 * global variable.
 */
Attr_attribute	INSTANCE;

/*
 * main for project swmtool
 */
void
main(int argc, char **argv)
{
	/*
	 * As the default, use the current directory as a pointer to
	 * the message object (.mo) file(s). dgettext() will search for
	 * the file(s) in the "./<current_locale>/LC_MESSAGES" directory.
	 * Change the bindtextdomain call path argument if the .mo files
	 * reside in some other location.
	 */
	bindtextdomain("SUNW_INSTALL_SWM", SWM_LOCALE_DIR);
	/*
	 * Work around for bugID 1107708
	 */
	init_usr_openwin();

	/*
	 * Initialize XView.
	 */
	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
		XV_USE_LOCALE, TRUE,
		XV_LOCALE_DIR, SWM_LOCALE_DIR, 
		XV_AUTO_CREATE, FALSE,
		NULL);
	INSTANCE = xv_unique_key();
	
	/*
	 * Initialize user interface components.
	 * Do NOT edit the object initializations by hand.
	 */
	Base_BaseWin = Base_BaseWin_objects_initialize(NULL, NULL);
	Client_ClientWin = Client_ClientWin_objects_initialize(NULL, Base_BaseWin->BaseWin);
	Cmd_CmdWin = Cmd_CmdWin_objects_initialize(NULL, Base_BaseWin->BaseWin);
	File_FileWin = File_FileWin_objects_initialize(NULL, Base_BaseWin->BaseWin);
	Meter_MeterWin = Meter_MeterWin_objects_initialize(NULL, Base_BaseWin->BaseWin);
	Pkg_ProdWin = Pkg_ProdWin_objects_initialize(NULL, Base_BaseWin->BaseWin);
	Pkg_PkgWin = Pkg_PkgWin_objects_initialize(NULL, Base_BaseWin->BaseWin);
	Props_PropsWin = Props_PropsWin_objects_initialize(NULL, Base_BaseWin->BaseWin);
	
	/*
	 * Turn control over to XView.
	 */
	InitMain(argc, argv);
	xv_main_loop(Base_BaseWin->BaseWin);
	exit(0);
}

#endif
