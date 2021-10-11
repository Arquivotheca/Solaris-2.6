#ifndef lint
#pragma ident "@(#)inst_progressbar.h 1.3 96/06/23 SMI"
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

#ifndef _INST_PROGRESSBAR_H
#define	_INST_PROGRESSBAR_H

typedef enum {
	/* cannot start with 0 - used in varargs routines */
	PROGRESSBAR_MAIN_LABEL = 1,
	PROGRESSBAR_DETAIL_LABEL,
	PROGRESSBAR_PERCENT
} ProgressBarAttr;

typedef struct {
	/* curses window to display to - filled in by app */
	WINDOW *win;

	/* main label information - filled in by Create routine */
	struct {
		int row;
		int col;
		int len;
	} main_label;

	/* space between main and detail label - filled in by Create routine */
	int space_len;

	/* progresss scale info - filled in by Create routine */
	struct {
		int row;
		int min_col;
		int max_col;
		int width;
	} scale;

	/*
	 * scale/factor data for this progress display
	 * default setting in Create/app may override
	 */
	UIProgressBarScaleInfo *scale_info;
} pfcProgressBarDisplayData;

/* functional prototypes */

#ifdef __cplusplus
extern "C" {
#endif

extern void pfcProgressBarCreate(
	UIProgressBarInitData *init_data,
	pfcProgressBarDisplayData **display_data,
	WINDOW *win,
	int scale_info_cnt);
extern void pfcProgressBarUpdate(
	pfcProgressBarDisplayData *display_data,
	int pause,
	...);
extern void pfcProgressBarCleanup(pfcProgressBarDisplayData *display_data);

#ifdef __cplusplus
}
#endif

#endif	/* _INST_PROGRESSBAR_H */
