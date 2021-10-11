#ifndef lint
#pragma ident "@(#)pfgprogressbar.h 1.3 96/07/08 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgprogressbar.h
 * Group:	installtool
 * Description:
 */

#ifndef _PFGPROGRESSBAR_H
#define	_PFGPROGRESSBAR_H

typedef struct {
	Widget dialog;
	WidgetList widget_list;
	Widget main_label;
	Widget detail_label;
	Widget scale;
	UIProgressBarScaleInfo	*scale_info;
} pfgProgressBarDisplayData;

/* functional prototypes */

#ifdef __cplusplus
extern "C" {
#endif

extern Widget pfgProgressBarCreate(
	UIProgressBarInitData *init_data,
	pfgProgressBarDisplayData **display_data,
	int scale_info_cnt);
extern void pfgProgressBarUpdate(
	pfgProgressBarDisplayData *display_data, int pause);
extern void pfgProgressBarCleanup(
	pfgProgressBarDisplayData *display_data);

#ifdef __cplusplus
}
#endif

#endif	/* _PFGPROGRESSBAR_H */
