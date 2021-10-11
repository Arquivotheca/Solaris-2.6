#ifndef lint
#pragma ident "@(#)pfg.h 1.84 96/09/03 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfg.h
 * Group:	installtool
 * Description:
 */

#ifndef	_PFG_H_
#define	_PFG_H_

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>


/* gui toolkit header files */
#include <Xm/Xm.h>
#include <Xm/DragDrop.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/Scale.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/FileSB.h>
#include <Xm/MessageB.h>
#include <Xm/DialogS.h>
#include <Xm/PanedW.h>
#include <Xm/CascadeBG.h>
#include <Xm/DrawingA.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/Frame.h>
#include <Xm/List.h>
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>

#include "pf.h"
#include "pfg_strings.h"
#include "pfg_labels.h"

/* for debugging */
#define	INSTALLTOOL_NAME	"INSTALLTOOL"
#define	GUI_DEBUG	DebugDest, debug, INSTALLTOOL_NAME, DEBUG_LOC
#define	GUI_DEBUG_NOHD	DebugDest, debug, NULL, DEBUG_LOC
#define	GUI_DEBUG_L1	GUI_DEBUG, LEVEL1
#define	GUI_DEBUG_L1_NOHD	GUI_DEBUG_NOHD, LEVEL1

/* CONSTANTS */

#define	PFG_MBSIZE_LENGTH    6
#define	PFG_CYLSIZE_LENGTH  10
/*
 * used in pfgclient_setup.c to record the last menu choice
 * so it can be remembered between continue and goback
 */
#define	BOTH_CHOICE	1
#define	ROOT_CHOICE	2
#define	SWAP_CHOICE	3

#define	PFG_MBSIZE_FORMAT "%*ld MB", PFG_MBSIZE_LENGTH

/* bit definitions for MwmHints.decorations */
#define	MWM_DECOR_ALL		(1L << 0)
#define	MWM_DECOR_BORDER	(1L << 1)
#define	MWM_DECOR_RESIZEH	(1L << 2)
#define	MWM_DECOR_TITLE		(1L << 3)
#define	MWM_DECOR_MENU		(1L << 4)
#define	MWM_DECOR_MINIMIZE	(1L << 5)
#define	MWM_DECOR_MAXIMIZE	(1L << 6)

/* key types */
typedef enum {
	/*
	 * Start these at 1, not 0, because they are used
	 * as vararg values and 0 is used to determine the
	 * end of the varargs list.
	 */
	ButtonContinue = 1,
	ButtonOk,
	ButtonCancel,
	ButtonGoback,
	ButtonChange,
	ButtonExit,
	ButtonHelp
} ButtonType;

/* generic widget list */

typedef struct pfWid_t_tag {
	struct pfWid_t_tag *next;
	Widget w;
} pfWid_t;


/* query codes */

typedef enum {
	pfQREBOOT,
	pfQCUSTOM,
	pfQBELOWMINDISK,
	pfQFDISKCHANGE,
	pfQLOSECHANGES,
	pfQLOADVTOC,
	pfQDEPENDS,
	pfQAUTOFAIL
} pfQueryCode;

/* types */

typedef struct pfQuery_t_tag {
	char *message, *yes, *no;
	int def;
} pfQuery_t;

typedef struct {
	Boolean maptop;
	int dsrSpaceReqColumnSpace;
	int dsrFSSummColumnSpace;
	int dsrFSRedistColumnSpace;
	int dsrFSCollapseColumnSpace;
} pfg_app_data_t;

typedef struct {
	int argc;
	char **argv;
} TParentReinit;

/* GLOBALS */
extern TParentReinit pfgParentReinitData;
extern Widget pfgTopLevel;		/* changeable widgets */
extern pfg_app_data_t pfgAppData;	/* gui toolkit app resources */
extern XtAppContext pfgAppContext;	/* gui toolkit toplevel state */
extern unsigned int pfgState;		/* state definitions in spmiapp_api.h */
extern Atom pfgWMDeleteAtom;		/* for quit from window menu */
extern int pfgLowResolution;		/* low screen resolution? */
extern int DebugDest;
extern UpgOs_t *UpgradeSlices;		/* upgradeable slices */
extern FSspace **FsSpaceInfo;
extern char *StatusScrFileName;

/* DSR related stuff */
extern TList DsrSLHandle;
extern TDSRArchiveList DsrALHandle;

/* FUNCTION PROTOTYPES */

/* pfgmain.c */
extern void pfAppInfo(char *title, char *message);
extern void pfAppWarn(pfErCode, char *);
extern void pfAppError(char *title, char *message);
extern void pfgWarning(Widget parent, pfErCode er);
extern void pfgExitError(Widget, pfErCode);
extern int  pfgQuery(Widget parent, pfQueryCode);
extern int pfgAppQuery(Widget parent, char * buff);
extern void pfgInfo(char *title, char *buf);
extern void pfgHelp(Widget, XtPointer, XmAnyCallbackStruct*);
extern void pfgExit(void);
extern void pfgPrintRestartMsg(void);
extern void pfgCleanExit(int exit_code, void *exit_data);
extern void pfgParentReinit(void *reinit_data);
extern void pfgChildShutdown(TChildAction exit_code);

/* pfgbootdiskquery.c */
extern Widget pfgCreateBootDiskQuery(Widget, char *);
extern int get_answer(void);

/* pfgbootdiskselect.c */
extern Widget	pfgCreateBootDisk(Widget);

/* pfgsummary.c */
extern Widget	pfgCreateSummary(void);

/* pfgfilesys.c */
extern Widget  pfgCreateFilesys(void);
extern void 	pfgUpdateFilesysSummary(void);

/* pfgtutor.c */
extern void 	pfgParade(parWin_t);
extern int history_prev(void);

/* pfgupgrade.c */
extern Widget	pfgCreateUpgrade(void);
extern void	pfgCreateUpgradeProgress(void);


/* pfgos.c */
extern Widget	pfgCreateOs(void);

/* pfgclient_setup.c */
extern Widget pfgCreateClientSelector(void);
extern Widget pfgCreateClientSetup(void);

/* pfgclients.c */
extern Widget	pfgCreateClients(void);

/* pfgtoplevel.c */
extern void	pfgInit(int *, char **); /* parse X args and get Xt globals */

/* pfgmeta.c */
extern void pfgUpdateMetaSize(void);

/*
 *	the following "creator" functions build the windows and return the
 *	top level (XtManage()) widget.  The argument is the pointer to
 *	an array of widget handles.  The functions set the values appropriate
 * 	for the update functions below
 */
extern Widget 	pfgCreateMain(Widget *),
	pfgCreateSoftware(Widget),
	pfgCreateDisks(Widget),
	pfgCreateFileSystems(Widget *);

extern Widget pfgCreateAllocateSvcQuery(void);
extern Widget	pfgCreateSw(void);
extern void	pfgResetPackages(void);
extern Widget	pfgCreatePreQuery(void);
extern Widget	pfgCreatePreserve(Widget);
extern Widget	pfgCreateUseDisks(void);

/* pfgcyl.c */
extern Widget pfgCreateCylinder(Widget parent, Disk_t *diskPtr);
extern void pfgCylPopup(Widget parent, struct disk *);
extern void pfgUpdateCylinder(void);
extern int modifyStart(Widget);
extern void startActivateCB(Widget, XtPointer, XtPointer);
extern void startLosingFocus(Widget, XtPointer, XtPointer);
extern void pfgUpdateCylinders(void);


/* pfgdisks.c */
extern void    pfgDiskError(Widget, char *, int);
extern void	pfgUpdateDisks(Widget);
extern void sizeVerifyCB(Widget, XtPointer, XmTextVerifyCallbackStruct *);
extern void mountVerifyCB(Widget, XtPointer, XmTextVerifyCallbackStruct *);
extern void mountFocusCB(Widget, XtPointer, XtPointer);
extern void sizeFocusCB(Widget, XtPointer, XtPointer);
extern void mountLosingFocus(Widget, XtPointer, XtPointer);
extern void sizeLosingFocus(Widget, XtPointer, XtPointer);
extern void cellValueChanged(Widget, XtPointer, XtPointer);
extern void mainFocusCB(Widget, XtPointer, XtPointer);
extern void mainLosingFocus(Widget, XtPointer, XtPointer);
extern void mainMotionCB(Widget, XtPointer, XmTextVerifyCallbackStruct *);
extern void mainActivateCB(Widget, XtPointer, XtPointer);
extern void sizeActivateCB(Widget, XtPointer, XtPointer);
extern void mountActivateCB(Widget, XtPointer, XtPointer);
extern int modifyMount(Widget w);
extern int modifySize(Widget w);

/* pfgos.c */
extern void pfgResetView(void);

/* pfgswquery.c */
extern Widget	pfgCreateSwQuery(void);

/* pfgremotes.c */
extern Widget	pfgCreateRemote(Widget);

/* pfgremquery.c */
extern Widget	pfgCreateRemquery(void);


/* pfgprogress */
extern void	pfgCreateProgress(void);
extern parAction_t pfgSystemUpdateInitial(void);

/* pfgdsr_al.c */
extern Widget pfgCreateDsrALGenerateProgress(void);
extern int dsr_al_progress_cb(void *client_data, void *call_data);

/* pfgdsr_analyze.c */
extern Widget pfgCreateDsrAnalyze(void);
extern int pfg_upgrade_progress_cb(void *mydata, void *progress_data);

/* pfgdsr_fsredist.c */
extern Widget pfgCreateDsrFSRedist(void);

/* pfgdsr_fssummary.c */
extern Widget pfgCreateDsrFSSummary(void);

/* pfgdsr_media.c */
extern Widget pfgCreateDsrMedia(void);

/* pfgdsr_spacereq.c */
extern Widget pfgCreateDsrSpaceReq(int main_parade);

/* pfgsolarispart.c */
extern void	pfgCreateSolarPart(Widget, Disk_t *);

/* pfgsolarcust.c */
extern void	pfgCreateSolarCust(Widget, Disk_t *);

/* pfgautolayout.c */
extern Widget	pfgCreateAutoLayout(Widget);

/* pfgautoquery.c */
extern Widget pfgCreateAutoQuery(void);

/* pfgutil.c */
extern parAction_t	pfgEventLoop(void);
extern void		pfgSetAction(parAction_t action);
extern parAction_t	pfgGetAction(void);
extern void		pfgSetCurrentScreen(Widget screen);
extern void 		pfgSetCurrentScreenWidget(Widget screen);
extern void		pfgDestroyPrevScreen(Widget previousScreen);
extern Widget		pfgShell(Widget w);
extern void pfgDestroyDialog(Widget parent, Widget dialog);
extern void		pfgBusy(Widget w);
extern void		pfgUnbusy(Widget w);
extern void		pfgSetWidgetString(
			WidgetList widget_list,
			char * name,
			char * message_text);
extern Widget		pfgGetNamedWidget(
			WidgetList widget_list,
			char * name);
extern void pfgSetStandardButtonStrings(WidgetList widget_list, ...);
extern void		start_itimer(void *data);
extern void		clear_itimer(void *data);
extern void pfgSetMaxWidgetHeights(
	WidgetList widget_list, char **labels);
extern void pfgSetMaxColumnWidths(
	WidgetList widget_list, WidgetList *entries,
	char **labels, char **values,
	Boolean offset_first, int offset);
extern void pfgSetMaxWidths(Widget *widget_array);

/* pfgusedisks.c */
extern void moveDisk(Disk_t * diskPtr, int add);
extern int pfgChangeBootQuery(Widget parent, char * buff);

/* pfglocale.c */
extern Widget pfgCreateLocales(void);

/* pfgintro.c */
extern Widget pfgCreateIntro(parWin_t);

#endif	/* _PFG_H_ */
