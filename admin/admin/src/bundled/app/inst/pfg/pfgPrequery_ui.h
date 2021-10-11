
/****************************************************************
 *
 * Name      : pfgPrequery_ui.h
 *
 * File is generated by TeleUSE
 * Version : TeleUSE v3.0.2 / Solaris 2.4
 *
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 *
 ****************************************************************/

#ifndef _PFGPREQUERY_UI_H_
#define _PFGPREQUERY_UI_H_

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include "tu_runtime.h"


/* widget creation functions */

extern Widget tu_prequery_dialog_widget(char * name,
                                        Widget parent,
                                        Widget ** warr_ret);

#define WI_PREQUERY_DIALOG 0
#define WI_PANELHELPTEXT 1
#define WI_MESSAGEBOX 2
#define WI_CONTINUEBUTTON 3
#define WI_GOBACKBUTTON 4
#define WI_PRESERVEBUTTON 5
#define WI_EXITBUTTON 6
#define WI_HELPBUTTON 7


#endif
