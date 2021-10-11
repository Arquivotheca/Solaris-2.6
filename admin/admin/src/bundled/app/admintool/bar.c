#include <stdio.h>
#include <X11/X.h>
#include <Xm/Scale.h>
#include <Xm/Form.h>
#include <Xm/DrawingA.h>
#include <Xm/RowColumn.h>
#include <Xm/LabelG.h>
#include "BarGauge.h"
#include "util.h"

#define DA_HEIGHT 8
#define DA_WIDTH  120
#define LINE_WIDTH 2



static void
barGaugeCallback(Widget da, XtPointer data, XmDrawingAreaCallbackStruct* cbs)
{
    BarGauge*	b = (BarGauge*) data;
    Display*	d = XtDisplay(da);
    Screen*	s = XtScreen(da);
    Dimension	h, w;
    Dimension	pw, fw;

    w = b->w;
    h = b->h;
    XSetForeground(d, b->gc, WhitePixelOfScreen(s));
    XSetLineAttributes(d, b->gc, LINE_WIDTH, LineSolid, CapRound, JoinRound);
    XDrawRectangle(d, b->p, b->gc, 0, 0, w, h);

    clear(b);

    XSetForeground(d, b->gc, b->fillColor);
    fw = (Dimension)((w * b->fillPercent) / 100);
    XFillRectangle(d, b->p, b->gc, 1, 1, fw, h-LINE_WIDTH);

    XSetForeground(d, b->gc, b->pendingColor);
    XSetFillStyle(d, b->gc, FillStippled);
    pw = (Dimension)((w * b->pendingPercent) / 100);
    XFillRectangle(d, b->p, b->gc, fw, 1, pw, h-LINE_WIDTH);

    XCopyArea(d, b->p, XtWindow(da), b->gc, 0, 0, w, h, 0, 0); 
}

void
clear(BarGauge* b)
{
    Display*	d = XtDisplay(b->da);
    Screen*	s = XtScreen(b->da);

    XSetForeground(d, b->gc, BlackPixelOfScreen(s));
    XFillRectangle(d, b->p, b->gc, 0, 0, b->w, (b->h)-1);
}

void
fillPending(BarGauge* b, int percent)
{
    b->pendingPercent = percent;
    barGaugeCallback(b->da, (XtPointer)b, NULL);
}

void
fill(BarGauge *b, int percent)
{
    b->fillPercent = percent;
    barGaugeCallback(b->da, (XtPointer)b, NULL);
}

BarGauge*
bar(Widget parent, char* lab)
{
    Widget 	da;
    Widget	form;
    XmString	xms;
    XmFontList	fontlist;
    Dimension	h;

    Display*	display;
    Screen*	screen;

    BarGauge*	b;

    b = (BarGauge*) malloc(sizeof(BarGauge));
    if (b == NULL)
	return (NULL);
    memset(b, 0, sizeof(BarGauge));

    b->tag = strdup(lab);

/* Parent is a RowColumn */
    if (!XmIsRowColumn(parent))
	return( NULL );

    display = XtDisplay(parent);
    screen  = XtScreen(parent);

    b->form = form = XtVaCreateManagedWidget("barGaugeForm",
	xmFormWidgetClass, parent,
	XmNfractionBase, 3,
	NULL);

    xms = XmStringCreateLocalized(lab);
    b->label = XtVaCreateManagedWidget("barGaugeLabel",
	xmLabelGadgetClass, form,
	XmNrightAttachment, XmATTACH_POSITION,
	XmNrightPosition, 1,
	XmNtopAttachment, XmATTACH_FORM,
        XmNlabelString, xms,
	NULL);

    XtVaGetValues(b->label, XmNfontList, &fontlist, NULL);

    b->h = XmStringHeight(fontlist, xms);
    b->w = DA_WIDTH;

    b->da = XtVaCreateManagedWidget("barGaugeDrawingArea", 
	xmDrawingAreaWidgetClass, form, 
	XmNleftAttachment, XmATTACH_POSITION,
	XmNleftPosition, 1,
	XmNleftOffset, 10,
	XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget, b->label,
	XmNheight, b->h,
	XmNwidth, b->w,
	XmNshadowThickness, 2,
	XmNresizePolicy, XmNONE,
	NULL);

     XtAddCallback(b->da, XmNexposeCallback, barGaugeCallback, b);

     XtVaCreateManagedWidget("0label",
	xmLabelGadgetClass, form,
	RSC_CVT(XmNlabelString, "0"),
	XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget, b->da,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, b->da,
	XmNalignment, XmALIGNMENT_BEGINNING,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

     XtVaCreateManagedWidget("100label",
	xmLabelGadgetClass, form,
	RSC_CVT(XmNlabelString, "100"),
	XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget, b->da,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, b->da,
	XmNalignment, XmALIGNMENT_END,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

     b->p = XCreatePixmap(display, 
	RootWindowOfScreen(screen), b->w, b->h,
	DefaultDepthOfScreen(screen));

     b->gc = XCreateGC(display, RootWindowOfScreen(screen), 0, NULL);

     XtVaSetValues(b->da, XmNuserData, b, NULL);

     setFillColor(b, "Red");
     setPendingColor(b, "Yellow");
     clear(b);
     return(b);
}

static
unsigned long
getColor(Widget w, char* name)
{
    Display*	display = XtDisplay(w);
    Screen*	screen = XtScreen(w);
    XColor	col, unused;
    Colormap	map = DefaultColormapOfScreen(screen);

    if (!XAllocNamedColor(display, map, name, &col, &unused)) {
	fprintf(stderr, "can't alloc color\n");
        return (-1);
    }
    return col.pixel;
}

void
setFillColor(BarGauge* b, char* name)
{
    if (b == NULL || b->da == NULL)
	return;
    b->fillColor = getColor(b->da, name);
}

void
setPendingColor(BarGauge* b, char* name)
{
    if (b == NULL || b->da == NULL)
	return;
    b->pendingColor = getColor(b->da, name);
}

   
void
on(BarGauge* b)
{
    XtVaSetValues(b->label, XmNsensitive, True, NULL);
}

void
off(BarGauge* b)
{
    XtVaSetValues(b->label, XmNsensitive, False, NULL);
}

