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
#ident	"@(#)xv_meter.c 1.7 93/10/13"
#endif

#include "defs.h"
#include "ui.h"
#include <group.h>
#include "swmtool.h"

typedef struct fs_meter {
	Panel_item	gauge;
	Panel_item	used;
	Panel_item	free;
	Space		*sp;
} Fs_meter;

void
UpdateMeter(show)
	int	show;
{
	static Fs_meter *fsitems;
	static int init = 0;
	static int ok_last_time = 1;
	int ok_this_time;
	register int i;
	Space	**spp;

	meter = swm_space_meter((char **)0);

	if (!init) {
		fsitems = (Fs_meter *)xmalloc(FS_MAX * sizeof (Fs_meter));

		fsitems[FS_ROOT].gauge = Meter_MeterWin->RootGauge;
		fsitems[FS_ROOT].used = Meter_MeterWin->RootUsed;
		fsitems[FS_ROOT].free = Meter_MeterWin->RootFree;
		fsitems[FS_ROOT].sp = (Space *)0;

		fsitems[FS_USR].gauge = Meter_MeterWin->UsrGauge;
		fsitems[FS_USR].used = Meter_MeterWin->UsrUsed;
		fsitems[FS_USR].free = Meter_MeterWin->UsrFree;
		fsitems[FS_USR].sp = (Space *)0;

		fsitems[FS_OPT].gauge = Meter_MeterWin->OptGauge;
		fsitems[FS_OPT].used = Meter_MeterWin->OptUsed;
		fsitems[FS_OPT].free = Meter_MeterWin->OptFree;
		fsitems[FS_OPT].sp = (Space *)0;

		fsitems[FS_VAR].gauge = Meter_MeterWin->VarGauge;
		fsitems[FS_VAR].used = Meter_MeterWin->VarUsed;
		fsitems[FS_VAR].free = Meter_MeterWin->VarFree;
		fsitems[FS_VAR].sp = (Space *)0;

		fsitems[FS_EXPORT].gauge = Meter_MeterWin->ExpGauge;
		fsitems[FS_EXPORT].used = Meter_MeterWin->ExpUsed;
		fsitems[FS_EXPORT].free = Meter_MeterWin->ExpFree;
		fsitems[FS_EXPORT].sp = (Space *)0;

		fsitems[FS_USROWN].gauge = Meter_MeterWin->OwnGauge;
		fsitems[FS_USROWN].used = Meter_MeterWin->OwnUsed;
		fsitems[FS_USROWN].free = Meter_MeterWin->OwnFree;
		fsitems[FS_USROWN].sp = (Space *)0;

		for (spp = meter; *spp; spp++) {
			Space	*sp = *spp;
			if (strcmp(sp->mountp, "/") == 0) {
				fsitems[FS_ROOT].sp = sp;
				xv_set(fsitems[FS_ROOT].gauge,
				    PANEL_LABEL_STRING, sp->mountp, NULL);
			} else if (strcmp(sp->mountp, "/usr") == 0) {
				fsitems[FS_USR].sp = sp;
				xv_set(fsitems[FS_USR].gauge,
				    PANEL_LABEL_STRING, sp->mountp, NULL);
			} else if (strcmp(sp->mountp, "/opt") == 0) {
				fsitems[FS_OPT].sp = sp;
				xv_set(fsitems[FS_OPT].gauge,
				    PANEL_LABEL_STRING, sp->mountp, NULL);
			} else if (strcmp(sp->mountp, "/var") == 0) {
				fsitems[FS_VAR].sp = sp;
				xv_set(fsitems[FS_VAR].gauge,
				    PANEL_LABEL_STRING, sp->mountp, NULL);
			} else if (strcmp(sp->mountp, "/export") == 0) {
				fsitems[FS_EXPORT].sp = sp;
				xv_set(fsitems[FS_EXPORT].gauge,
				    PANEL_LABEL_STRING, sp->mountp, NULL);
			} else if (strcmp(sp->mountp, "/usr/openwin") == 0) {
				fsitems[FS_USROWN].sp = sp;
				xv_set(fsitems[FS_USROWN].gauge,
				    PANEL_LABEL_STRING, "/openwin", NULL);
			}
		}
		init++;
	}

	/*
	 * Initialize contents of meter
	 */
	ok_this_time = 1;
	for (i = 0; i < FS_MAX; i++) {
		if (fsitems[i].sp != (Space *)0 &&
		    fsitems[i].sp->fsi != (Fsinfo *)0) {
			Space	*sp = fsitems[i].sp;
			Fsinfo	*fsi = fsitems[i].sp->fsi;
			char	usedstr[20];
			char	freestr[20];
			long	pct, avail;
			/*
			 * real mount point
			 */
			pct = 100 - ((100 * fsi->f_bavail) / fsi->f_blocks);
			avail = fsi->f_blocks - sp->bused;
			(void) sprintf(usedstr, "%6.2f",
			    ((float)sp->bused * fsi->f_frsize) / (1024 * 1024));
			(void) sprintf(freestr, "%6.2f",
			    ((float)avail * fsi->f_frsize) / (1024 * 1024));
			xv_set(fsitems[i].gauge,
				PANEL_INACTIVE, FALSE,
				PANEL_VALUE,	pct,
				NULL);
			xv_set(fsitems[i].used,
				PANEL_INACTIVE, FALSE,
				PANEL_VALUE,	usedstr,
				NULL);
			xv_set(fsitems[i].free,
				PANEL_INACTIVE, FALSE,
				PANEL_VALUE,	freestr,
				NULL);
			if (avail < 0)
				ok_this_time = 0;
		} else {
			/*
			 * theoretical mount point, part
			 * of another file system.
			 */
			xv_set(fsitems[i].gauge,
				PANEL_INACTIVE, TRUE,
				PANEL_VALUE,	0,
				NULL);
			xv_set(fsitems[i].used,
				PANEL_INACTIVE, TRUE,
				PANEL_VALUE,	"000.00",
				NULL);
			xv_set(fsitems[i].free,
				PANEL_INACTIVE, TRUE,
				PANEL_VALUE,	"000.00",
				NULL);
		}
	}
	/*
	 * Display the meter if requested, or if the
	 * user just selected software that ran a file
	 * system out of space.  Once the user dismisses
	 * the meter, we don't redisplay it until we
	 * encounter another space to no space transition
	 * or until explicitly requested.
	 */
	if (show || (!ok_this_time && ok_last_time))
		xv_set(Meter_MeterWin->MeterWin,
			FRAME_CMD_PUSHPIN_IN,	TRUE,
			XV_SHOW,		TRUE,
			NULL);
	ok_last_time = ok_this_time;
}
