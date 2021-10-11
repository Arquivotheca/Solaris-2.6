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
#ident	"@(#)xview.h 1.14 93/04/12"
#endif

#ifndef	SWMTOOL_XVIEW_H
#define	SWMTOOL_XVIEW_H

#include "admin.h"
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/xv_xrect.h>

#define	SET_VALUE(f, v)		xv_set((f), PANEL_VALUE, (v) ? (v) : "", NULL)

/*
 * XXX This values must match the order in
 * which the strings are specified in the
 * properties setting in Props.G
 */
#define	PROPS_MEDIA	0
#define	PROPS_ADMIN	1
#define	PROPS_CATEGORY	2
#define	PROPS_BROWSER	3
#define	PROPS_HOSTS	4
#define	PROPS_LAST	5

/*
 * Error exit codes for package
 * installation/removal process
 */
#define	SWM_OK		0
#define	SWM_ERR_SHARE	1
#define	SWM_ERR_RMOUNT	2
#define	SWM_ERR_SPOOL	3
#define	SWM_ERR_UNSHARE	4
#define	SWM_ERR_RUMOUNT	5
#define	SWM_ERR_MKADMIN	6
#define	SWM_ERR_RMADMIN	7
#define	SWM_ERR_DESPOOL	8

typedef	void (*Exit_func)(caddr_t, int);

extern	Display	*display;

extern	int	use_color;	/* =1 if multi-plane display */
extern	char	*openwinhome;	/* getenv("OPENWINHOME") or /usr/openwin */

#ifdef __STDC__
/*
 * xv_init.c
 */
extern	void InitMain(int, char **);
extern	void init_usr_openwin(void);	/* XXX remove when 1107708 fixed */
/*
 * Modules.c
 */
extern	void InitModules(caddr_t, Xv_opaque);
extern	void BrowseModules(SWM_mode, SWM_view);
extern	void UpdateCategory(Module *);
extern	void UpdateModules(void);
extern	void SelectModules(Xv_window, Event *, Notify_arg, Notify_event_type);
extern	void RepaintModules(
		Canvas, Xv_window, Display *, Window, Xv_xrectlist *);
extern	void DisplayModules(void);
extern	void SelectAllModules(int);
extern	Module *GetCurrentModule(void);
/*
 * Levels.c
 */
extern	void InitLevels(caddr_t, Xv_opaque);
extern	void SelectLevel(Xv_window, Event *, Notify_arg, Notify_event_type);
extern	void RepaintLevels(
		Canvas, Xv_window, Display *, Window, Xv_xrectlist *);
extern	void ClearLevels(void);
extern	void SetDefaultLevel(char *, Module *);
extern	void SetCurrentLevel(char *, Module *);
extern	Module *GetCurrentLevel(void);
extern	Module *GetPreviousLevel(void);
/*
 * xv_category.c
 */
extern	void InitCategory(Module *);
extern	void SetCategory(void);
extern	void ResetCategory(void);
/*
 * xv_admin.c
 */
extern	void GetAdmin(void);
extern	void SetAdmin(void);
extern	void AddMail(void);
extern	void DeleteMail(void);
extern	void ChangeMail(void);
extern	void InitConfig(caddr_t);
extern	void ConfigFile(CF_mode);
/*
 * xv_host.c
 */
extern	void InitHosts(caddr_t);
extern	void GetHosts(void);
extern	void SetHosts(void);
extern	void AddHost(int);
extern	void DeleteHost(void);
extern	void ChangeHost(void);
extern	void ToggleHost(Xv_opaque);
extern	void SetSelectedHost(Xv_opaque, Panel_item, int, Xv_opaque);
/*
 * xv_load.c
 */
extern	int LoadMedia(MediaType, char *, char *, int);
extern	void SetMediaType(int, Xv_opaque, Xv_opaque, Xv_opaque, Xv_opaque);
extern	void EjectMedia(MediaType, char *, char *);
extern	MediaType LoadTypeToMediaType(int);
extern	void ResetMedia(Panel_item, Panel_item, Panel_item, Panel_item);
/*
 * xv_meter.c
 */
extern	void UpdateMeter(int);
/*
 * xv_subr.c
 */
extern	Notify_value cleanup(Notify_client, Destroy_status);
extern	void setup(void);
extern	void *icon_load_x11bm(char *, char *);
extern	int server_image_xid(u_short *, int, int);
extern	void perror(const char *);	/* XXX */
/*
 * Props.c
 */
extern	void ShowProperties(int, caddr_t);
extern	void SetDisplayModes(void);
extern	void ResetDisplayModes(void);
/*
 * Pkg.c
 */
extern	void GetPackageInfo(Module *);
extern	void SetPackageInfo(Xv_opaque);
extern	void ResetPackageInfo(Xv_opaque);
extern	void GetProductInfo(Module *, int);
extern	void SetProductInfo(Xv_opaque);
/*
 * Base.c
 */
extern	void BaseResize(Xv_opaque);
extern	void Apply(Xv_opaque);
extern	void Reset(caddr_t, int);
extern	void BaseModeSet(SWM_mode, int);
/*
 * Client.c
 */
extern	void InitClientInfo(Xv_opaque, SWM_mode);
extern	void SetClientInfo(Panel_item);
extern	void ResetClientInfo(Panel_item);
/*
 * Notice.c
 */
extern	void NoticeSave(Frame);
extern	void NoticeNativeArch(Frame);
extern	void NoticeNativeOs(Frame);
/*
 * xv_tty.c
 */
extern	int tty_is_active(void);
extern	int tty_exec_func(void (*)(caddr_t), caddr_t, Exit_func, caddr_t);
extern	int tty_exec_cmd(char *, Exit_func, caddr_t);
#endif

#endif	/* !SWMTOOL_XVIEW_H */
