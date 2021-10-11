
/****************************************************************
 *
 * Name      : pfgSoftware_ui.h
 *
 * File is generated by TeleUSE
 * Version : TeleUSE v3.0.2 / Solaris 2.4
 *
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 *
 ****************************************************************/

#ifndef _PFGSOFTWARE_UI_H_
#define _PFGSOFTWARE_UI_H_

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include "tu_runtime.h"


/* widget creation functions */

extern Widget tu_software_dialog_widget(char * name,
                                        Widget parent,
                                        Widget ** warr_ret);

#define WI_SOFTWARE_DIALOG 0
#define WI_PANELHELPTEXT 1
#define WI_SOFTWAREFORM 2
#define WI_CLUSTERLABEL 3
#define WI_SIZELABEL 4
#define WI_PACKAGESSCROLLEDWINDOW 5
#define WI_SOFTWARECLUSTERSROWCOLUMN 6
#define WI_TOTALSIZEFORM 7
#define WI_TOTALSIZELABEL 8
#define WI_TOTALSIZEVALUE 9
#define WI_LEGENDFRAME 10
#define WI_LEGENDFORM 11
#define WI_LEGENDROWCOLUMN 12
#define WI_EXPANDFORM 13
#define WI_EXPANDLEGENDARROW 14
#define WI_EXPANDLEGENDLABEL 15
#define WI_CONTRACTFORM 16
#define WI_CONTRACTLEGENDARROW 17
#define WI_CONTRACTLEGENDLABEL 18
#define WI_REQUIREDFORM 19
#define WI_REQUIREDLEGENDBUTTON 20
#define WI_REQUIREDLEGENDLABEL 21
#define WI_PARTIALFORM 22
#define WI_PARTIALLEGENDBUTTON 23
#define WI_PARTIALLEGENDLABEL 24
#define WI_SELECTEDFORM 25
#define WI_SELECTEDLEGENDBUTTON 26
#define WI_SELECTEDLEGENDLABEL 27
#define WI_UNSELECTEDFORM 28
#define WI_UNSELECTEDLEGENDBUTTON 29
#define WI_UNSELECTEDLEGENDLABEL 30
#define WI_SOFTWAREPANEDWINDOW 31
#define WI_SOFTWAREINFOFRAME 32
#define WI_SOFTWAREINFOLABEL 33
#define WI_SOFTWAREINFOSCROLLEDWINDOW 34
#define WI_SOFTWAREINFOFORM 35
#define WI_SOFTWAREINFOROWCOLUMN 36
#define WI_PRODLABEL 37
#define WI_ABBREVLABEL 38
#define WI_VENDORLABEL 39
#define WI_DESCRIPTLABEL 40
#define WI_ESTLABEL 41
#define WI_SOFTWAREINFOROWCOLUMN1 42
#define WI_PRODVALUE 43
#define WI_ABBREVVALUE 44
#define WI_VENDORVALUE 45
#define WI_DESCRIPTVALUE 46
#define WI_SIZEROWCOLUMN 47
#define WI_SOFTWAREDEPENDENCIESFRAME 48
#define WI_SOFTWAREDEPENDENCIESSCROLLEDLIST 49
#define WI_SOFTWAREDEPENDENCIESLABEL 50
#define WI_MESSAGEBOX 51
#define WI_OKBUTTON 52
#define WI_HELPBUTTON 53
#define WI_BUTTON3 54
#define WI_BUTTON4 55
#define WI_BUTTON5 56


#endif
