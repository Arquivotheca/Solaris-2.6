/* Copyright 1996 Sun Microsystems, Inc. */

#pragma ident "@(#)meter.c	1.1 96/09/18 Sun Microsystems"

#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <stdio.h>

#include "util.h"
#include "software.h"
#include "spmisoft_api.h"
#include "spmisvc_api.h"
#include "BarGauge.h"
#include "Meter.h"

#include <nl_types.h>
extern nl_catd	_catd;	/* for catgets(), defined in main.c */

SpaceMeter* spaceMeter = NULL;

static void
meterOkCB(Widget w, XtPointer cd, XtPointer cbs)
{
    hideMeter();
}

static struct space_meter *
buildSpaceMeter(Widget parent, char** fslist)
{
    int i = 0;
    int barListSize = 0;
    Widget shell, okButton, form, rc;

#ifdef DEBUG
fprintf(stderr, "in buildSpaceMeter\n");
#endif

    spaceMeter = (struct space_meter*) malloc(sizeof(struct space_meter));
    if (spaceMeter == NULL)
	return NULL;

    memset(spaceMeter, 0, sizeof(struct space_meter));

    shell = XtVaCreatePopupShell( "spaceMeter",
	xmDialogShellWidgetClass, parent,
	XmNshellUnitType, XmPIXELS,
	XmNallowShellResize, True,
	XmNtitle, catgets(_catd, 8, 784, "Space Meter"),
	NULL);

    spaceMeter->dialog = XtVaCreateWidget("barForm",
	xmFormWidgetClass, shell,
	NULL);

    rc = XtVaCreateManagedWidget("barRowColumn",
		xmRowColumnWidgetClass, spaceMeter->dialog,
		XmNnumColumns, 2, 
		XmNorientation, XmVERTICAL,
		XmNpacking, XmPACK_COLUMN,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 10,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 10,
		NULL);

    XtRealizeWidget(shell);
    XtRealizeWidget(spaceMeter->dialog);

    i = 0;

    barListSize = (N_INSTALL_FS * sizeof(BarGauge));
    spaceMeter->barList = (BarGauge**) malloc(barListSize);
    if (spaceMeter->barList == NULL)
	return NULL;

    memset(spaceMeter->barList, 0, barListSize);

    while (installFileSystems[i]) {
        spaceMeter->barList[i] = bar(rc, installFileSystems[i]);
        fill(spaceMeter->barList[i], 0);
	i++;
    }

    create_button_box(spaceMeter->dialog, rc, NULL, 
		&okButton, NULL, NULL, NULL, NULL);

    XtAddCallback(okButton, XmNactivateCallback, meterOkCB, NULL);

    return (spaceMeter);
}

static BarGauge*
getBar(char* fs)
{
    int i;
    BarGauge** bl = spaceMeter->barList;

    for (i = 0; i < N_INSTALL_FS; i++) {
        BarGauge* b = spaceMeter->barList[i];
        if (strcmp(b->tag, fs) == 0)
	    return(b);
    } 
}



void
initMeter(Widget parent)
{
  FSspace** fsp;
  Fsinfo*   fsi;
  int i;

#ifdef DEBUG
fprintf(stderr, "in initMeter spaceMeter=%x\n", spaceMeter);
#endif

  if (spaceMeter)
	return;

  if (parent == NULL)
	parent = find_parent(parent); 

  spaceMeter = buildSpaceMeter(parent, installFileSystems);

  fsp = load_current_fs_layout();

  i = 0;
  while (installFileSystems[i]) {
        BarGauge* b = getBar(installFileSystems[i]);
	FSspace* fs;
        Fsinfo* fsi;

        if ((fs = get_fs(fsp, installFileSystems[i])) && 
            (fsi = fs->fsp_fsi)) {
	    long used = fsi->f_blocks - fsi->f_bfree;
            int per = (used * 100) / fsi->f_blocks;

            fill(b, per);
	} else {
	    fill(b, 0);
            off(b);
	}
        i++;
  }
}

void
updateMeter()
{
  int i, j;
  FSspace** add_fsp = get_current_fs_layout();
  FSspace** curr_fsp = (FSspace**) installed_fs_layout();
  FSspace* fs;
  char**    dlist;

#ifdef DEBUG
fprintf(stderr, "in updateMeter spaceMeter=%x\n", spaceMeter);
#endif

  if (spaceMeter == NULL)
	initMeter(NULL);

  dlist = (char**) malloc(MAX_SPACE_FS * sizeof(char*));
  if (dlist == NULL)
	return;
  memset(dlist, 0, MAX_SPACE_FS * sizeof(char*));
  i = 0; j = 0;
  while (fs = curr_fsp[i]) {
    if (fs->fsp_fsi) {
	dlist[j] = fs->fsp_mntpnt;
 	j++;
    }
    i++;
  }
	
  add_fsp = admintool_space_meter(dlist);

  free(dlist);

  i = 0;
  while (installFileSystems[i]) {
        BarGauge* b = getBar(installFileSystems[i]);
	FSspace* curr_fs;
        Fsinfo* curr_fsi;
        Fsinfo* fsi;
        int per;
	long to_add;

        fs = get_fs(add_fsp, installFileSystems[i]);
	if (fs == NULL) {
	    i++;
	    continue;
	}

 	curr_fs = get_fs(curr_fsp, installFileSystems[i]);
        curr_fsi = curr_fs->fsp_fsi;

        if (curr_fs && curr_fsi) {

	    to_add = fs->fsp_reqd_contents_space;
            per = (to_add * 100) / curr_fsi->f_blocks;

            fillPending(b, per);
	} else {
	    fillPending(b, 0);
            off(b);
	}
        i++;
  }
}


void
showMeter(Widget parent)
{
#ifdef DEBUG
fprintf(stderr, "in showMeter spaceMeter=%x\n", spaceMeter);
#endif

    if (spaceMeter == NULL)
	initMeter(parent);

    XtManageChild(spaceMeter->dialog);
    XtPopup(XtParent(spaceMeter->dialog), XtGrabNone);
}

void
hideMeter()
{
#ifdef DEBUG
fprintf(stderr, "in hidemeter spaceMeter=%x\n", spaceMeter);
#endif
    XtUnmanageChild(spaceMeter->dialog);
    XtPopdown(XtParent(spaceMeter->dialog));
}

void
deleteMeter()
{
    int i;

#ifdef DEBUG
fprintf(stderr, "in deleteMeter spaceMeter=%x\n", spaceMeter);
#endif

    if (spaceMeter == NULL)
	return;

    hideMeter();

    i = 0;
    while (installFileSystems[i]) {
        BarGauge* b = spaceMeter->barList[i];
	free(b->tag);
        i++;
    }
    free(spaceMeter->barList);

    XtDestroyWidget(spaceMeter->dialog);
    free(spaceMeter);

    spaceMeter = NULL;
}
