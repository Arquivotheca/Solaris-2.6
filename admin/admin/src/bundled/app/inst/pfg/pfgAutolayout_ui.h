
/****************************************************************
 *
 * Name      : pfgAutolayout_ui.h
 *
 * File is generated by TeleUSE
 * Version : TeleUSE v3.0.2 / Solaris 2.4
 *
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 *
 ****************************************************************/

#ifndef _PFGAUTOLAYOUT_UI_H_
#define _PFGAUTOLAYOUT_UI_H_

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include "tu_runtime.h"


/* widget creation functions */

extern Widget tu_autolayout_dialog_widget(char * name,
                                          Widget parent,
                                          Widget ** warr_ret);

#define WI_AUTOLAYOUT_DIALOG 0
#define WI_PANELHELPTEXT 1
#define WI_AUTOLAYOUTFORM 2
#define WI_CREATELABEL 3
#define WI_AUTOLAYOUTCHECKBOX 4
#define WI_MESSAGEBOX 5
#define WI_CONTINUEBUTTON 6
#define WI_CANCELBUTTON 7
#define WI_HELPBUTTON 8
#define WI_BUTTON4 9
#define WI_BUTTON5 10


#endif