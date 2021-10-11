/* Copyright 1996 Sun Microsystems, Inc. */

#pragma ident "@(#)Meter.h	1.1 96/09/18 Sun Microsystems"

#ifndef _METER_H_
#define _METER_H_

#include <X11/X.h>
#include "BarGauge.h"

typedef struct space_meter {
    BarGauge** barList;
    Widget     dialog;
} SpaceMeter;

void updateMeter();
void showMeter(Widget);
void hideMeter();
void deleteMeter();

#endif
