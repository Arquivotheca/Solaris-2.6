/* Copyright 1996 Sun Microsystems, Inc. */

#pragma ident "@(#)BarGauge.h	1.1 96/09/18 Sun Microsystems"

#ifndef _BARGAUGE_H_ 
#define _BARGAUGE_H_

typedef struct bar_tag {
    char*	tag;
    Widget	form;
    Widget	label;
    Widget	da;
    Pixmap	p;
    Dimension	w, h;
    GC		gc;
    int		fillPercent;
    int		pendingPercent;
    long	pendingColor, fillColor;
} BarGauge;

void fill(BarGauge*, int);
BarGauge* bar(Widget, char*);
void clear(BarGauge*);
void setFillColor(BarGauge*, char*);
void setPendingColor(BarGauge*, char*);

#endif

