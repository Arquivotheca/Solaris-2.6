#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/LabelG.h>

#include "launcher.h"
#include "util.h"
#include "xpm.h"

#include "default.xpm"

static int numRows;
static char msg[256];


extern Pixmap	solstice_image;
extern int chosenNumColumns;

void display_icons(Widget, Widget);
Widget create_field(Widget, int, int, AppInfo *);
void delete_field(Widget);
void create_insensitive_pixmap(Widget, Pixmap);

extern void exec_callback(Widget, XtPointer, XtPointer);

static void
reset_palette_grid(Widget form)
{
	int cnt = launcherContext->l_appCount;

	numRows = (cnt / chosenNumColumns) + (cnt % chosenNumColumns ? 1 : 0);

	numRows = numRows ? numRows : 2;
	
	XtVaSetValues(form, 
		XmNfractionBase,   chosenNumColumns,
		NULL);
}

/*
 *
 * Display on icon on palette, where pos is linear index of icons
 * positiom. If pos == -1, then add icon at end.
 *
 * In honor of Margaret McCormack, affectionately and reluctantly
 * known as "marge".
 * 
 */
void
slapIconOnPalette(Widget form, AppInfo * ai, int pos)
{
	int row, col;

	if (pos == -1)
		pos = launcherContext->l_appCount;

	row = pos / chosenNumColumns;
	col = pos % chosenNumColumns;

	ai->a_iconW = create_field(form, row, col, ai);
}

/*
 * Remove icon from display
 */

void
unSlapIconFromPalette(AppInfo * ai)
{
	delete_field(ai->a_iconW);
}


void
delete_field(Widget w)
{
	if (w)
		XtUnmapWidget(w);
}


#define WRAPPER_HEIGHT (48 + 48)
#define NO_ICON_PATH(x) (((x)->a_iconPath == NULL)||((x)->a_iconPath[0] == '\0'))
#define ICON_TOP_OFFSET 8

/*
 * Create Form Widget to contain selectable pixmap. 
 * Each display area on the palette is a "field." A
 * field is occupied by an XPM pixmap in a Form.
 */

Widget
create_field(Widget form, int row, int col, AppInfo * ai)
{
	Pixel fg, bg;
	Pixmap 	pix_image, ipix_image;
	Widget wrapper, w;
	XmString label;
	int rc = 0;

	wrapper = XtVaCreateManagedWidget("wrapper", xmFormWidgetClass,
                          form,
                          XmNfractionBase,      3,
                          /* form constraints */
                          XmNtopAttachment,
                                XmATTACH_FORM,
			  XmNtopOffset, row * WRAPPER_HEIGHT,
                          XmNleftAttachment,
                                XmATTACH_POSITION,
                          XmNleftPosition,
                                col,
                          XmNrightAttachment,
                                XmATTACH_POSITION,
                          XmNrightPosition,
                                (col + 1),
                          NULL);

	XtVaGetValues(wrapper,
                XmNforeground, &fg,
                XmNbackground, &bg,
                NULL);

	
	rc = -1;
	if (NO_ICON_PATH(ai) ||
            (rc = load_XPM(form, ai->a_iconPath, bg, &pix_image)) != XpmSuccess)
	{
		if ((rc != XpmSuccess) && !ai->a_errReported) {
			xpm_error(rc, ai->a_iconPath);
			ai->a_errReported = True;
		}
	        create_XPM(form, default_xpm, bg, &pix_image);
	        create_XPM(form, default_xpm, bg, &ipix_image);
	}

	if (rc == XpmSuccess) {
		load_XPM(form, ai->a_iconPath, bg, &ipix_image);
	}

        ai->a_toggle = w = XtVaCreateManagedWidget(ai->a_iconPath,
                    xmToggleButtonWidgetClass, wrapper,
                    XmNlabelType,       XmPIXMAP,
                    XmNlabelPixmap,     pix_image,
		    XmNshadowThickness, 2,
		    XmNpushButtonEnabled, True,
		    XmNshadowType, XmSHADOW_ETCHED_OUT,
                    XmNmultiClick, XmMULTICLICK_DISCARD,
		    XmNindicatorOn, False,
                    /* form constraints */
                    XmNtopAttachment,   XmATTACH_FORM,
                    XmNtopOffset,       ICON_TOP_OFFSET,
                    XmNleftAttachment,
                                XmATTACH_POSITION,
                    XmNleftPosition,    1,
                    XmNrightAttachment,
                                XmATTACH_POSITION,
                    XmNrightPosition,   2,
		    NULL);

	create_insensitive_pixmap(ai->a_toggle, pix_image);

	XtAddCallback(w, XmNvalueChangedCallback, exec_callback, ai);

	label = XmStringCreateLocalized(ai->a_appName);
        ai->a_label = XtVaCreateManagedWidget(ai->a_appName,
                   xmLabelGadgetClass, wrapper,
                   XmNlabelString,     label,
                   XmNtopAttachment,   XmATTACH_WIDGET,
                   XmNtopWidget,       w,
                   XmNtopOffset,       ICON_TOP_OFFSET/2,
                   XmNleftAttachment,  XmATTACH_FORM,
                   XmNrightAttachment, XmATTACH_FORM,
                   NULL);
        XmStringFree(label);
	return(wrapper);
}

/*
 * Called after changes in configuration to re-display icons
 * A new Form is created so that it is reasonably sized.
 * XmMainWindowSetAreas gets size right.
 */

void
update_display_layout()
{
	Widget form;
	configContext_t * c = configContext;
	
	form = XtVaCreateWidget("workForm",
			xmFormWidgetClass, launcherContext->l_mainWindow,
			NULL);
	display_icons(form, c->c_appList);
	XtUnmanageChild(launcherContext->l_workForm);
	XtManageChild(form);
	XmMainWindowSetAreas(launcherContext->l_mainWindow, 
			     launcherContext->l_menuBar, (Widget) NULL,
			     (Widget) NULL, (Widget) NULL, form);
	XtDestroyWidget(launcherContext->l_workForm);
	launcherContext->l_workForm = form;
}

/*
 * Iterates over list (i.e. list_w) and displays each icon.
 * Updates a_displayOrdinal field of each appTable entry.
 */

void
display_icons(Widget form, Widget list_w)
{
	int n, icnt, i;
	launcherContext_t * c;
	XmStringTable	ilist;
	char * str;
	
	XtVaGetValues(launchermain, XmNuserData, &c, NULL);
	XtVaGetValues(list_w, XmNitemCount, &icnt, XmNitems, &ilist, NULL);

	reset_palette_grid(form);

	/* 
         * Keep track of how many are visible. When 
         * config file is written, all HIDDEN icons
         * will follow SHOW entries.
         */
	c->l_showCount = icnt;
	for (i = 0; i < icnt; i++) {
		XmStringGetLtoR(ilist[i], XmSTRING_DEFAULT_CHARSET, &str);

		/* 
                 * This should never happen but if it does,
		 * continue at least averts a seg violation.
		 */
		if ((n = lookup_appTable_entry(str)) < 0)
			continue;
		slapIconOnPalette(form, &(c->l_appTable[n]), i);
		c->l_appTable[n].a_displayOrdinal = i;
	}
}

static const Pattern50_width = 2;
static const Pattern50_height = 2;
static char Pattern50_bits[] = {0x01, 0x02};

void
create_insensitive_pixmap(Widget pb, Pixmap pmmap)
{
        Pixel bg;
        unsigned long vmask;
        XGCValues values;
	Pixmap stipple, dim_pixmap;
	int depth;	
	GC gc;
 
    XtVaGetValues(pb, XmNbackground, &bg, 
		      XmNdepth, &depth, NULL);

    dim_pixmap = XCreatePixmap(Gdisplay, pmmap, 48, 48, depth);
    vmask = GCFunction | GCForeground;
    values.function = GXcopy;
    values.foreground = bg;
    gc = XCreateGC(Gdisplay, dim_pixmap, vmask, &values);
    XCopyArea(Gdisplay, pmmap, dim_pixmap, gc, 0, 0, 48, 48, 0 , 0);
    stipple = XCreateBitmapFromData(Gdisplay, pmmap, Pattern50_bits,
                                    Pattern50_width, Pattern50_height);
    XSetFillStyle(Gdisplay, gc, FillStippled);
    XSetStipple(Gdisplay, gc, stipple);
    XFillRectangle(Gdisplay, dim_pixmap, gc, 0, 0, 48, 48);
    XtVaSetValues(pb, XmNlabelInsensitivePixmap, dim_pixmap, NULL);
    XFreePixmap(Gdisplay, stipple);
    XFreeGC(Gdisplay, gc);
}
