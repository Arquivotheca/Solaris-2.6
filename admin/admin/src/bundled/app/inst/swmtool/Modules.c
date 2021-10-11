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
#ident	"@(#)Modules.c 1.25 94/10/13"
#endif

/*
 * Modules.c - Create and manipulate module canvas
 */

#include "defs.h"
#include "ui.h"
#include "Display.h"
#include <string.h>
#include <ctype.h>
#include <sys/param.h>
#include <xview/canvas.h>
#include <xview/defaults.h>
#include <xview/icon_load.h>
#include <xview/font.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>
#include <xview/tty.h>
#include <xview/cms.h>
#include <xview/openmenu.h>
#include <xview/rect.h>
#include <xview/win_input.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Base_ui.h"

typedef enum { INSTB_FALSE = 0, INSTB_TRUE = 1 } BOOLEAN;

#define	LOADED_NO_VERSION	0
#define	LOADED_THIS_VERSION	1
#define	LOADED_OTHER_VERSION	2

#define	PRODUCT_ICON_HEIGHT	64
#define	PRODUCT_ICON_WIDTH	64

#define	TYPE_ICON_WIDTH		16
#define	TYPE_ICON_HEIGHT	16

/*
 * Here's the way icons are laid out on the paint window
 *	(single cell shown):
 *
 *                               ^
 *                          top_margin
 *                               v
 *                               ^        ^        ^
 *                          row_offset  range      |
 *                               v        v        |
 *			     ---------             |
 *                           |       |             |
 *                    <range>| 64x64 |<range>      |
 *  <left_margin><col_offset>|  icon |<col_offset> |
 *                           |       |             | row_size
 *			     ---------             | (NB: depends on font
 *                               ^        ^        |  used for "Name")
 *                          name_margin range      |
 *                               v        |        |
 *               <name_margin> "Name"     |        |
 *                               ^        v        |
 *                          row_offset             |
 *                               v                 v
 *
 *               <-----------col_size------------>
 *
 * Notes:
 *	There is a (*_offset x 2) buffer around each icon.
 *	The area in which the icon is sensitive to mouse clicks
 *		extends outwards from the icon's border col_range
 *		and row_range in the horizontal and vertical
 *		directions, respectively.
 *	The displayed length of the product name is limited to
 *		col_size - (2 * namespace), and the name itself
 *		is centered within this space.
 */

struct display_params {
	short	max_major;	/* last available slot in major dimension */
	short	max_minor;	/* last available slot in minor dimension */
	short	end_major;	/* last used slot in major dimension */
	short	end_minor;	/* last used slot in minor dimension */
	short	col_size;	/* width (in pixels) of each column */
	short	row_size;	/* height (in pixels) of each row */
	short	col_offset;	/* horizontal buffer around each glyph */
	short	row_offset;	/* vertical buffer around each glyph */
	short	col_range;	/* specifies sensitivity boundary... */
	short	row_range;	/* ...in horizontal and vertical directions */
	short	top_margin;
	short	left_margin;
	short	name_margin;
	short	glyph_width;
	short	glyph_height;
};

#define	ICONIC		0	/* also row-major */
#define	TEXTUAL		1	/* also column-major */

static struct display_params dpy_params[2] = {
	{
		0,	/* max_major */
		0,	/* max_minor */
		0,	/* end_major */
		0,	/* end_minor */
		0,	/* col_size */
		0,	/* row_size */
		32,	/* col_offset */
		16,	/* row_offset */
		8,	/* col_range */
		8,	/* row_range */
		0,	/* top_margin */
		8,	/* left_margin */
		4,	/* name_margin */
		PRODUCT_ICON_WIDTH,
		PRODUCT_ICON_HEIGHT
	},
	{
		0,	/* max_major */
		0,	/* max_minor */
		0,	/* end_major */
		0,	/* end_minor */
		0,	/* col_size */
		0,	/* row_size */
		16,	/* col_offset */
		4,	/* row_offset */
		8,	/* col_range */
		4,	/* row_range */
		4,	/* top_margin */
		0,	/* left_margin */
		8,	/* name_margin */
		TYPE_ICON_WIDTH,
		TYPE_ICON_HEIGHT
	}
};
static struct display_params *params = &dpy_params[ICONIC];

/*
 * Here's the way text is laid out within a column:
 *
 *                               ^
 *                          top_margin
 *                               v
 *                               ^        ^        ^
 *                          row_offset  range      |
 *                               v        v        |
 *			     ---------             |
 *                           |       |             |
 *                    <range>| 16x16 |<name>       |
 *  <left_margin><col_offset>|  type |<margin>Product Name
 *                           |  icon |             | row_size
 *			     ---------    ^        | (NB: depends on font
 *                               ^      range      |  used for "Name")
 *                          row_offset    v        |
 *                               v                 v
 *
 *               <-----------col_size-------------------...>
 *
 * Notes:
 *	There is a (*_offset x 2) buffer around each line.
 *	The area in which the line is sensitive to mouse clicks
 *		extends outwards from the line's border col_range
 *		and row_range in the horizontal and vertical
 *		directions, respectively.
 *	The displayed length of the product name is fixed.
 */

#define	INDEX(i, j)	(((i) * (params->max_minor + 1)) + (j))

#define	NAME_SPACE	4

#define	INSTB_GT	'>'
#define	INSTB_BLANK	' '

#define	MULTI_CLICK_TIMEOUT	2	/* tenths of sec between clicks */

static	int	iconic;
static	int	mod_init;

static	u_long		Foreground;
static	u_long		Background;

/*
 * We use a variable-width font for names
 * and a fixed-width font for "dots" and
 * sizes.  Each can be bold (installed) or
 * normal weight.
 */
#define	NORMAL		0
#define	BOLD		1

static	Xv_Font		var_font[2];
static	Xv_Font		fixed_font[2];
static	int		sizewidth[2];
static	int		fontwidth[2];
static	int		lineheight;	/* for spacing, regardless of font */

static	GC		Mods_gc, Mods_tgc;
static	XGCValues	Gc_val;

static	Drawable	pkg_app_image;
static	Drawable	cluster_app_image;
static	Drawable	pkg_type_image;
static	Drawable	cluster_type_image;

static	int		Width;
static	int		Height;

static	Xv_window	Mods_pw;
static	Window		Mods_xid;
static	Xv_object	Mods_sb;
static	Xv_opaque	Mods_menu;

static	APP_INFO	*Mod_array;
static	Module		*Curapp;

static	int		click_usecs;

static	Module		*active_media;

extern	Base_BaseWin_objects *Base_BaseWin;

static void init_gc(void);
static void UpdateButtons(void);
static void click_timeout(Notify_client client, int which);
static void resize_modules(Canvas, int, int);
static void markdisplay(short, short, short, short);
static void display_rect(short, short);
static int sort_display(const void *, const void *);
static void reset_modules(void);
static void default_app(Module *, Xv_window, Event *);
static void clear_disp_array(short, short, short, short);
static void init_font(void);
static void menu_app(Module *, Xv_window, Event *);
static Menu gen_menu(Menu_item, char *, File **);
static void file_menu_notify(Menu menu, Menu_item menu_item);
static int app_is_displayed(Module *);
static int app_is_selected(APP_INFO *);
static int app_is_loaded(APP_INFO *);
static int check_components(Module *, Module *, int);
static int app_is_required(APP_INFO *);
static void RenderIcon(APP_INFO *, short, short);
static void RenderText(APP_INFO *, short, short);

/*
 * Complete initialization of the 'Mods' canvas
 */
void
InitModules(caddr_t ip, Xv_opaque owner)
{
	/*LINTED [alignment ok]*/
	Base_BaseWin_objects *objects = (Base_BaseWin_objects *)ip;

	Width = (int) xv_get(Base_BaseWin->Mods, CANVAS_WIDTH);
	Height = (int) xv_get(Base_BaseWin->Mods, CANVAS_HEIGHT);

	click_usecs = defaults_get_integer("openWindows.multiClickTimeout",
		"OpenWindows.multiClickTimeout", MULTI_CLICK_TIMEOUT);
	click_usecs *= 100000;	/* 1/10ths to microseconds */

	Mods_pw = (Xv_window)xv_get(objects->Mods, CANVAS_NTH_PAINT_WINDOW, 0);
	Mods_xid = (Window)xv_get(Mods_pw, XV_XID);
	Mods_sb = (Xv_object)
		xv_get(objects->Mods, OPENWIN_VERTICAL_SCROLLBAR, Mods_pw);

	Mods_menu = Base_BaseMenu_create(ip, owner);

	xv_set(objects->Mods,
		CANVAS_FIXED_IMAGE,	FALSE,
		CANVAS_CMS_REPAINT,	FALSE,
		CANVAS_RESIZE_PROC,	resize_modules,
		0);
	/*
	 * Set colormap, font, and graphical context for window
	 */
	init_font();
	init_gc();

	mod_init++;
}

/*
 * The main entry into the browser package.
 *	Sets static variable "active_media"
 */
void
BrowseModules(SWM_mode mode,	/* install/remove/upgrade/etc. */
	SWM_view view)		/* software/services */
{
	Base_BaseWin_objects *ip = Base_BaseWin;
	SWM_view oldview = get_view();
	SWM_mode oldmode = get_mode();
	Module	*installed_media;	/* service we're modifying */
	int	activate_mode = 0;
	char	footer[BUFSIZ];

	if (mode == MODE_UNSPEC)
		mode = oldmode;

	if (view == VIEW_UNSPEC)
		view = oldview;

	set_mode(mode);
	set_view(view);

	if (oldview != view || oldmode != mode) {
		reset_selections(1, 0, 1);	/* reset selections & locales */
		GetProductInfo((Module *)0, 0);
	}

	if (view == VIEW_NATIVE)
		activate_mode = 1;

	switch (mode) {
	case MODE_INSTALL:
		switch (view) {
		case VIEW_NATIVE:
		default:
			installed_media = find_media("/", (char *)0);
			set_installed_media(installed_media);
			break;
		case VIEW_SERVICES:
			installed_media = get_installed_media();
			break;
		}
		active_media = get_source_media();

		if (active_media == (Module *)0)
			(void) strcpy(footer, dgettext("SUNW_INSTALL_SWM",
		    "No source media loaded; set the Source Media property."));
		else if (installed_media == (Module *)0)
			(void) strcpy(footer, dgettext("SUNW_INSTALL_SWM",
	    "No client support selected; select with Add Client Support."));
		else
			(void) strcpy(footer, dgettext("SUNW_INSTALL_SWM",
		    "Click select on software to mark for installation."));

		xv_set(ip->BaseDoit,
			PANEL_LABEL_STRING,	gettext("Begin Installation"),
			NULL);
		BaseModeSet(MODE_INSTALL, activate_mode);
		break;
	case MODE_REMOVE:
		switch (view) {
		case VIEW_NATIVE:
		default:
			installed_media = find_media("/", (char *)0);
			set_installed_media(installed_media);
			(void) strcpy(footer, dgettext("SUNW_INSTALL_SWM",
			    "Click select on software to mark for removal."));
			break;
		case VIEW_SERVICES:
			installed_media = get_installed_media();
			if (installed_media == (Module *)0)
				(void) strcpy(footer,
				    dgettext("SUNW_INSTALL_SWM",
	    "No client support selected; select with Remove Client Support."));
			else {
				(void) sprintf(footer,
				    dgettext("SUNW_INSTALL_SWM",
			    "Press Begin Removal to remove `%s' support..."),
				    installed_media->info.media->med_volume);
				/*
				 * XXX
				 */
				mark_selection(
				    installed_media->sub, MOD_SELECT);
			}
			break;
		}
		active_media = installed_media;
		xv_set(ip->BaseDoit,
			PANEL_LABEL_STRING,	gettext("Begin Removal"),
			NULL);
		BaseModeSet(MODE_REMOVE, activate_mode);
		break;
	case MODE_UPGRADE:
		switch (view) {
		case VIEW_NATIVE:
			break;
		case VIEW_SERVICES:
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	xv_set(ip->BaseWin,
		FRAME_LEFT_FOOTER,	footer,
		NULL);
	if (active_media != (Module *)0 && installed_media != (Module *)0) {
		set_current(active_media);
		InitCategory(active_media);
		SetDefaultLevel(get_short_name(active_media), active_media);
	} else
		ClearLevels();
	UpdateModules();
}

void
UpdateCategory(Module *category)
{
	Module	*level = GetCurrentLevel();
	Module	*prod;
	char	*name;

	if (category == (Module *)0)
		category = get_current_category(MEDIA);

	if (category != (Module *)0)
		name = category->info.cat->cat_name;
	else
		name = gettext("All Software");
	/*
	 * If at anything from product level down,
	 * use the product name as the category.
	 */
	if (level != (Module *)0 && level->type != MEDIA &&
	    (prod = get_current_product()) != (Module *)0)
		name = prod->info.prod->p_name;
	/*
	 * else use name from above
	 */
	xv_set(Base_BaseWin->BaseCat, PANEL_VALUE, name, NULL);
}

void
UpdateModules(void)
{
	UpdateButtons();
	UpdateMeter(0);
	reset_modules();
	DisplayModules();

	Curapp = GetCurrentLevel();
	if (Curapp && Curapp->type == MEDIA)
		Curapp = (Module *)0;
	menu_app(Curapp, Mods_pw, (Event *)0);
	GetPackageInfo(Curapp);
}

static void
init_gc(void)
{
	Gc_val.foreground = BlackPixel(display, DefaultScreen(display));
	Gc_val.background = WhitePixel(display, DefaultScreen(display));
	Foreground = BlackPixel(display, DefaultScreen(display));
	Background = WhitePixel(display, DefaultScreen(display));

	Mods_gc = (GC)XCreateGC(display, Mods_xid,
	    (GCForeground | GCBackground), &Gc_val);
	Mods_tgc =  (GC)XCreateGC(display, Mods_xid,
	    (GCForeground|GCBackground), &Gc_val);
	(void) XSetFont(display, Mods_tgc, xv_get(var_font[NORMAL], XV_XID));
	if (Mods_gc == 0 || Mods_tgc == 0) {
		fprintf(stderr, gettext("PANIC:  GC create failed\n"));
		exit(1);
	}
}

static void
UpdateButtons(void)
{
	if (count_selections(active_media) > 0 && !browse_mode)
		xv_set(Base_BaseWin->BaseDoit, PANEL_INACTIVE, FALSE, NULL);
	else
		xv_set(Base_BaseWin->BaseDoit, PANEL_INACTIVE, TRUE, NULL);
}

static APP_INFO	*app_pending;	/* app with selection event pending */

/*
 * This routine is called if a second mouse click is
 * not received within the allotted multi-click time-
 * out interval.  It means a previous mouse click was
 * an attempt to select a module, so we go ahead and
 * finish up the work initiated by the first click.
 */
/*ARGSUSED*/
static void
click_timeout(client, which)
	Notify_client client;
	int	which;
{
	Module	*mod;

	if (app_pending == (APP_INFO *)0)
		return;

	mod = app_pending->app_data;

	mark_selection(mod, MOD_TOGGLE);

	UpdateButtons();
	UpdateMeter(0);

	app_pending = (APP_INFO *)0;

	if (mod->type == METACLUSTER)	/* otherwise display already updated */
		DisplayModules();
}

/*
 * Event callback function for `Mods'.
 */
/*ARGSUSED*/
void
SelectModules(win, event, arg, type)
	Xv_window	win;
	Event		*event;
	Notify_arg	arg;
	Notify_event_type type;
{
	struct itimerval timer;
	short	major;
	short	minor;
	int 	xpos;
	int	ypos;
	APP_INFO *newapp;

	if (GetCurrentLevel() &&
	    ((event_action(event) == ACTION_SELECT && event_is_down(event)) ||
	    (event_action(event) == ACTION_ADJUST && event_is_up(event)) ||
	    (event_action(event) == ACTION_MENU && event_is_down(event)))) {
		/*
		 * XXX  Race condition
		 *
		 * Turn the multi-click timer off.  There is
		 * a potential race if a previously-set timer
		 * expires before we execute this call.
		 */
		notify_set_itimer_func((Notify_client)win,
		    (Notify_func)click_timeout, ITIMER_REAL,
		    (struct itimerval *)0, (struct itimerval *)0);

		/*
		 * Determine the major and minor
		 * positions of the mouse
		 */
		xpos = event_x(event) - params->left_margin;
		ypos = event_y(event) - params->top_margin;

		/*
		 * Are we in the margins?
		 */
		if (xpos < 0 || ypos < 0)
			return;

		if (iconic) {
			minor = xpos / params->col_size;
			major = ypos / params->row_size;
		} else {
			major = xpos / params->col_size;
			minor = ypos / params->row_size;
		}

		/*
		 * Is it between apps?
		 */
		if (((xpos % params->col_size) <
		    (params->col_offset - params->col_range)) ||
		    ((ypos % params->row_size) <
		    (params->row_offset - params->row_range)) ||
		    ((xpos % params->col_size) > params->col_size -
		    (params->col_offset - params->col_range) ||
		    ((ypos % params->row_size) > params->row_size -
		    (params->row_offset - params->row_range))))
			return;

		/*
		 * Check if we have a position with a
		 * real application in it
		 */
		if (major > params->end_major ||
		    (major < params->end_major && minor > params->max_minor) ||
		    (major == params->end_major && minor > params->end_minor))
			return;

		/*
		 * The mouse is pointing to a real application
		 */
		newapp = &Mod_array[INDEX(major, minor)];

		/*
		 * Now that we know where the mouse event
		 * occurred, figure out what to do with it.
		 */
		if (event_action(event) == ACTION_MENU) {
			/*
			 * Menu button.  Finish up previous
			 * selection processing if neccessary,
			 * then throw up menu.
			 */
			if (app_pending)
				click_timeout((Notify_client)win, ITIMER_REAL);
			Curapp = newapp->app_data;
		} else if (event_action(event) == ACTION_SELECT) {
			if (app_pending && newapp->app_data == Curapp) {
				/*
				 * It's a double click -- repaint
				 * showing true selection status,
				 * then run the application's default
				 * menu item.
				 */
				app_pending = (APP_INFO *)0;
				newapp->app_repaint = 1;
				display_rect(major, minor);
				default_app(Curapp, win, event);
			} else {
				if (app_pending)
					/*
					 * It's a click on a different
					 * application -- finish up the
					 * old one.
					 */
					click_timeout(
					    (Notify_client)win, ITIMER_REAL);
				/*
				 * Process this click; it may be the first
				 * click in a double click.  Turn on the
				 * timer and mark click pending.  To avoid
				 * the appearance of sluggishness, we go
				 * ahead and toggle the selection's color.
				 * If it turns out to be a double-click we'll
				 * toggle it back later.
				 */
				Curapp = newapp->app_data;
				timer.it_value.tv_usec = click_usecs;
				timer.it_value.tv_sec = 0;
				timer.it_interval.tv_usec = 0;
				timer.it_interval.tv_sec = 0;
				notify_set_itimer_func((Notify_client)win,
				    (Notify_func)click_timeout, ITIMER_REAL,
				    &timer, (struct itimerval *)0);
				app_pending = newapp;
				newapp->app_repaint = 1;
				display_rect(major, minor);
			}
		} else if (event_action(event) == ACTION_ADJUST)
			Curapp = newapp->app_data;

		menu_app(Curapp, win, event);
		GetPackageInfo(Curapp);
	}
}

/*ARGSUSED*/
static void
resize_modules(Canvas canvas, int width, int height)
{
	if (!mod_init)
		return;

	if (width != Width || height != Height) {
		Width = width;
		Height = height;
		reset_modules();
	}
}

/*
 * ModuleRepaint
 * Repaint callback function for `Mods'.
 *
 * Repaints a superset of the area of the window that has changed.
 *
 * 1. If current category is NULL, indicating no software to be displayed,
 *	then simply clears the entire screen
 * 2. If the width of the window has changed from what it was previously
 * then redraw the entire screen, since glyphs will be moved on the screen.
 * 3. Determine the actual rectangle bounds using rl_bound, rl_x, and rl_y
 *  Clear the area that has been changed
 *
 * 4. Set appwidth = MAX(GLYPH_WIDTH, width of largest appname in category)
 *    Then, number of rectangles per line = Window_width / (appwidth + offset)
 * 5. Set appheight = GLYPH_HEIGHT + appname height + offset
 * ****For now, pay no attention to the width. Simply redisplay all lines
 * ****that have been damaged.
 * 6. Starting file for redisplay is j =
 *	(((top of damaged area) - TOP MARGIN) / app height) * max files per line
 * 7. At this point, simply cycle through all files in list
 * from file #j on. Use XFillRectangle to display the icon, and
 * XDrawString to display the name. If the application has its own
 * icon, then display it. Else display the default icon.
 *  Make sure to start a new line if the width
 * of the current line is beyond the max width allowed
 */
/*ARGSUSED*/
void
RepaintModules(canvas, paint_window, dpy, xwin, rects)
	Canvas		canvas;
	Xv_window	paint_window;
	Display		*dpy;
	Window		xwin;
	Xv_xrectlist	*rects;
{
	short j, k;
	int rectnum;
	static int Init_done = 0;

	/*
	 * First repaint is done automatically when we enter xv_main_loop
	 */
	if (Init_done == 0) {
		Init_done = 1;
		return;
	}

	if (GetCurrentLevel() == (Module *)0)
		return;

	if (rects == NULL) {
		/*
		 * If we receive a NULL pointer, repaint the entire window
		 */
		markdisplay(0, 0, params->end_major, params->max_minor);
	} else {
		/*
		 * Repaint the area specified by the Xv_rectlist rects
		 * The algorithm is simple.  For each rectangle passed
		 * to this routine, determine the lower and upper bounds
		 * of rows and columns affected, then convert to major and
		 * minor indicies.  If there are applications in that area,
		 * mark them as needing repainting.
		 *
		 * NB:  rows and columns are those of the display, not
		 * storage (thus the conversion).
		 */
		short	col_size = params->col_size;
		short	row_size = params->row_size;
		short	max_col = iconic ?
				params->max_minor : params->end_major;
		short	max_row = iconic ?
				params->end_major : params->max_minor;
		short	lowrow, lowcol, highrow, highcol;

		for (rectnum = 0; rectnum < rects->count; rectnum++) {
			short x = (rects->rect_array[rectnum].x -
				params->left_margin);
			short y = (rects->rect_array[rectnum].y -
				params->top_margin);
			short width = rects->rect_array[rectnum].width;
			short height = rects->rect_array[rectnum].height;

			lowrow = (y / row_size <= max_row) ?
			    y / row_size : max_row;

			lowcol = (x / col_size <= max_col) ?
			    x / col_size : max_col;

			highrow = ((y + height) / row_size <= max_row) ?
			    (y + height) / row_size : max_row;

			highcol = ((x + width) / col_size <= max_col) ?
			    (x + width) / col_size : max_col;

			if (iconic)
				markdisplay(lowrow, lowcol, highrow, highcol);
			else
				markdisplay(lowcol, lowrow, highcol, highrow);
		}
	}
	for (j = 0; j <= params->end_major; j++) {
		for (k = 0; k <= params->max_minor; k++) {
			display_rect(j, k);
		}
	}
}

static void
markdisplay(short lowmajor,
	short	lowminor,
	short	highmajor,
	short	highminor)
{
	int j, k;

	if (lowmajor < 0)
		lowmajor = 0;
	if (lowminor < 0)
		lowminor = 0;
	if (highmajor < 0)
		highmajor = 0;
	if (highminor < 0)
		highminor = 0;

	for (j = lowmajor; j <= highmajor; j++) {
		for (k = lowminor; k <= highminor; k++) {
			if (Mod_array[INDEX(j, k)].app_name != (char *)0)
				Mod_array[INDEX(j, k)].app_repaint = 1;
		}
	}
}

void
#ifdef __STDC__
DisplayModules(void)
#else
DisplayModules(void)
#endif
{
	RepaintModules(Base_BaseWin->Mods, Mods_pw, display, Mods_xid, 0);
}

/*
 * Display the application in the position row, col
 */
static void
#ifdef __STDC__
display_rect(short major, short minor)
#else
display_rect(major, minor)
	short	major;
	short	minor;
#endif
{

	APP_INFO	*app;
	short		y_pos;
	short		x_pos;

	if (major > params->end_major || minor > params->max_minor ||
	    (major == params->end_major && minor > params->end_minor))
		return;

	app = &Mod_array[INDEX(major, minor)];

	if (app->app_repaint != 1)
		return;

	app->app_repaint = 0;

	if (iconic) {
		x_pos = minor * params->col_size + params->left_margin;
		y_pos = major * params->row_size + params->top_margin;
		RenderIcon(app, x_pos, y_pos);
	} else {
		x_pos = major * params->col_size + params->left_margin;
		y_pos = minor * params->row_size + params->top_margin;
		RenderText(app, x_pos, y_pos);
	}
}


/*
 * Implements a case-insensitive, localized sort
 */
static int
sort_display(const void *p1, const void *p2)
{
	APP_INFO *app1 = (APP_INFO *)p1;
	APP_INFO *app2 = (APP_INFO *)p2;
	char	name1[256];
	char	name2[256];
	char	*src, *dest;

	src = app1->app_name;
	dest = name1;
	do {
		*dest++ = tolower((u_char)*src);
	} while (*src++);

	src = app2->app_name;
	dest = name2;
	do {
		*dest++ = tolower((u_char)*src);
	} while (*src++);

	return (strcoll(name1, name2));
}

/*
 * Sets up the display array for the current category,
 * but does not display the modules (call DisplayModules
 * for this).  This routine does fill in the number of
 * dipsplayed and available modules, since DisplayModules
 * does not maintain this information.
 */
static void
reset_modules(void)
{
	int	namefmt, namelen, cols;
	int	icons;
	APP_INFO *app;
	int	newsize;
	int	pw_height;
	int	nmods, ndisp;
	Config	config;
	Module *category;
	Module	*mod, *parent;
	static	Module *last_category;
	static	int arraysize;

	parent = GetCurrentLevel();

	config_get(&config);

	namefmt = config.gui_mode.value;
	namelen = config.namelen.value;
	cols = config.ncols.value + 1;

	category = get_current_category(MEDIA);

	/*
	 * Free previously-used resources
	 */
	if (category != last_category && Mod_array != (APP_INFO *)0)
		clear_disp_array(0, 0, params->max_major, params->max_minor);

	icons = 0;
	nmods = 0;
	ndisp = 0;

	/*
	 * Count up the number of total and
	 * displayable modules at this level.
	 * Also keep track of whether any icons
	 * are explicitly defined (for automatic
	 * display mode).
	 */
	if (parent != (Module *)0) {
		for (mod = get_sub(parent); mod; mod = get_next(mod)) {
			if (app_is_displayed(mod) &&
			    in_category(category, mod)) {
				if (get_module_icon(mod) != (File *)0)
					icons++;
				ndisp++;
			}
			nmods++;
		}
	}

	/*
	 * Determine our display mode.  If automatic,
	 * switch to iconic if there are any icons to
	 * display, otherwise go textual.
	 */
	switch (namefmt) {
	case 0:	/* automatic */
		if (icons)
			iconic = 1;
		else
			iconic = 0;
		break;
	case 1:
		iconic = 1;
		break;
	case 2:
	default:
		iconic = 0;
		break;
	}

	if (iconic)
		params = &dpy_params[0];
	else
		params = &dpy_params[1];

	/*
	 * Determine space allocated to icon or textual name.  In
	 * iconic mode, display is laid out in row-major order, with
	 * the number of columns determined by the size of the canvas
	 * and the size required for each icon.  In textual mode, the
	 * diplay is column-major (and paginated) and the number of columns
	 * is determined by the user and the text adjusted accordingly.
	 * We re-size the paint window so we can render all the objects
	 * and let the scrollbar do most of the work.
	 */
	if (iconic) {
		params->col_size = params->glyph_width +
					(2 * params->col_offset);
		params->row_size = params->glyph_height +
				(2 * params->row_offset) + lineheight;
		params->max_minor =
		    ((Width - params->left_margin) / params->col_size) - 1;
		if (params->max_minor > ndisp)
			params->max_minor = ndisp;
		params->max_major =
		    ((ndisp + params->max_minor) / (params->max_minor + 1)) - 1;
		pw_height = params->top_margin +
			(params->row_size * (params->max_major + 1));
		if (pw_height < Height)
			pw_height = Height;
		xv_set(Base_BaseWin->Mods,
			CANVAS_PAINTWINDOW_ATTRS,
				XV_HEIGHT,	pw_height,
				0,
			0);
		xv_set(Mods_sb,
		    SCROLLBAR_PIXELS_PER_UNIT,	params->row_size,
		    SCROLLBAR_OBJECT_LENGTH,	params->max_major + 1,
		    SCROLLBAR_PAGE_LENGTH, Height / params->row_size,
		    SCROLLBAR_VIEW_LENGTH, Height / params->row_size,
		    SCROLLBAR_VIEW_START, 0,
		    0);
	} else {
		params->col_size = (Width - params->left_margin) / cols;
		params->row_size = params->glyph_height > lineheight ?
			params->glyph_height + (2 * params->row_offset) :
			lineheight + (2 * params->row_offset);
		if (ndisp == 0)
			params->max_minor = params->max_major = 0;
		else {
			params->max_minor = ((ndisp + (cols - 1)) / cols) - 1;
			params->max_major = ((ndisp + params->max_minor) /
				(params->max_minor + 1)) - 1;
		}
		pw_height = params->top_margin +
			(params->row_size * (params->max_minor + 1));
		if (pw_height < Height)
			pw_height = Height;
		xv_set(Base_BaseWin->Mods,
			CANVAS_PAINTWINDOW_ATTRS,
				XV_HEIGHT,	pw_height,
				0,
			0);
		xv_set(Mods_sb,
		    SCROLLBAR_PIXELS_PER_UNIT,	params->row_size,
		    SCROLLBAR_OBJECT_LENGTH,	params->max_minor + 1,
		    SCROLLBAR_PAGE_LENGTH, Height / params->row_size,
		    SCROLLBAR_VIEW_LENGTH, Height / params->row_size,
		    SCROLLBAR_VIEW_START, 0,
		    0);
	}
	XClearArea(display, Mods_xid, 0, 0, Width, pw_height, FALSE);

	if (parent == (Module *)0)
		return;

	newsize = (params->max_major + 1) * (params->max_minor + 1);
	if (newsize > arraysize) {
		if (Mod_array == (APP_INFO *)0)
			Mod_array = (APP_INFO *)
			    xmalloc(newsize * sizeof (APP_INFO));
		else
			Mod_array = (APP_INFO *)
			    xrealloc(Mod_array, newsize * sizeof (APP_INFO));
		(void) memset((void *)&Mod_array[arraysize], 0,
			(newsize - arraysize) * sizeof (APP_INFO));
		arraysize = newsize;
	}

	params->end_major =
		((ndisp + params->max_minor) / (params->max_minor + 1)) - 1;
	params->end_minor = ndisp % (params->max_minor + 1);
	if (params->end_minor == 0)
		params->end_minor = params->max_minor;
	else
		params->end_minor -= 1;

	app = Mod_array;
	for (mod = get_sub(parent); mod; mod = get_next(mod)) {
		if (!app_is_displayed(mod) || !in_category(category, mod))
			continue;

		app->app_data = mod;
		app->app_repaint = 1;

		if (namelen == 0)	/* full name */
			app->app_name = xstrdup(get_full_name(mod));
		else			/* short names */
			app->app_name = xstrdup(get_short_name(mod));

		if (category != last_category) {
			app->app_simage = (Server_image)0;
			app->app_drawable = (Drawable)0;
		}
		++app;
	}
	last_category = category;

	qsort((void *)Mod_array, ndisp, sizeof (APP_INFO), sort_display);
}

/*
 * This function clears the display array Mod_array
 * from lowmajor, lowminor through highmajor, highminor
 *
 * It does NOT set the next valid position
 * this must be done by the calling routine.
 */
static void
#ifdef __STDC__
clear_disp_array(short lowmajor,
	short	lowminor,
	short	highmajor,
	short	highminor)
#else
clear_disp_array(lowmajor, lowminor, highmajor, highminor)
	short	lowmajor;
	short	lowminor;
	short	highmajor;
	short	highminor;
#endif
{
	APP_INFO	*app;
	int		i, j;

	for (i = lowmajor; i <= highmajor; i++) {
		for (j = lowminor; j <= highminor; j++) {
			app = &Mod_array[INDEX(i, j)];
			free(app->app_name);
			(void) memset((void *)app, 0, sizeof (APP_INFO));
		}
	}
}

static u_short large_package_image[] = {
#include "icons/LargePackage.icon"
};

static u_short small_package_image[] = {
#include "icons/TinyPackage.icon"
};

static u_short large_cluster_image[] = {
#include "icons/LargeCluster.icon"
};

static u_short small_cluster_image[] = {
#include "icons/TinyCluster.icon"
};

static void
#ifdef __STDC__
init_font(void)
#else
init_font()
#endif
{
	Font_string_dims dims;

	/* Calculate individual font widths */

	/* Use the frame's font */
	var_font[NORMAL] = (Xv_Font) xv_get(Base_BaseWin->BaseWin, XV_FONT);

	fixed_font[NORMAL] = (Xv_Font) xv_find(Base_BaseWin->BaseWin, FONT,
		FONT_FAMILY,    FONT_FAMILY_DEFAULT_FIXEDWIDTH,
		FONT_STYLE,	xv_get(var_font[NORMAL], FONT_STYLE),
		FONT_SCALE,	xv_get(var_font[NORMAL], FONT_SCALE),
		0);

	if (fixed_font[NORMAL] == (Xv_Font)0)
		fixed_font[NORMAL] = var_font[NORMAL];

	var_font[BOLD] = (Xv_Font) xv_find(Base_BaseWin->BaseWin, FONT,
		FONT_FAMILY,	xv_get(var_font[NORMAL], FONT_FAMILY),
		FONT_STYLE,	FONT_STYLE_BOLD,
		FONT_SCALE,	xv_get(var_font[NORMAL], FONT_SCALE),
		0);

	if (var_font[BOLD] == (Xv_Font)0)
		var_font[BOLD] = var_font[NORMAL];

	fixed_font[BOLD] = (Xv_Font) xv_find(Base_BaseWin->BaseWin, FONT,
		FONT_FAMILY,	FONT_FAMILY_DEFAULT_FIXEDWIDTH,
		FONT_STYLE,	FONT_STYLE_BOLD,
		FONT_SCALE,	xv_get(var_font[NORMAL], FONT_SCALE),
		0);

	if (fixed_font[BOLD] == (Xv_Font)0)
		fixed_font[BOLD] = fixed_font[NORMAL];

	fontwidth[NORMAL] = xv_get(fixed_font[NORMAL], FONT_DEFAULT_CHAR_WIDTH);
	xv_get(fixed_font[NORMAL], FONT_STRING_DIMS, "000.00 Mb", &dims);
	sizewidth[NORMAL] = dims.width;

	fontwidth[BOLD] = xv_get(fixed_font[BOLD], FONT_DEFAULT_CHAR_WIDTH);
	xv_get(fixed_font[BOLD], FONT_STRING_DIMS, "000.00 Mb", &dims);
	sizewidth[BOLD] = dims.width;

	lineheight = xv_get(var_font[NORMAL], FONT_DEFAULT_CHAR_HEIGHT);

	/*
	 * Create server image and xid for default glyphs
	 */
	pkg_app_image = server_image_xid(large_package_image,
		PRODUCT_ICON_HEIGHT, PRODUCT_ICON_WIDTH);
	cluster_app_image = server_image_xid(large_cluster_image,
		PRODUCT_ICON_HEIGHT, PRODUCT_ICON_WIDTH);
	pkg_type_image = server_image_xid(small_package_image,
		TYPE_ICON_HEIGHT, TYPE_ICON_WIDTH);
	cluster_type_image = server_image_xid(small_cluster_image,
		TYPE_ICON_HEIGHT, TYPE_ICON_WIDTH);
}

static Menu
gen_menu(Menu_item menu_item, char *name, File **client_data)
{
	Menu_item mi;
	Menu	menu;
	register int i;

	/*
	 * Sanity check to make sure we have
	 * something on which to base sub-menu
	 */
	if (client_data == (File **)0 || Curapp == (Module *)0)
		return ((Menu)0);
	menu = (Menu)xv_get(menu_item, MENU_PULLRIGHT);
	if (menu != (Menu)0) {
		if ((Module *)xv_get(menu, MENU_CLIENT_DATA) == Curapp) {
			/*
			 * If the app pointer attached to the menu
			 * matches the current application, we're
			 * already done.
			 */
#ifdef DEBUG
			fprintf(stderr,
				"using existing menu '%s'\n", name);
#endif
			return (menu);
		}
		xv_set(menu, MENU_CLIENT_DATA, Curapp, NULL);
		/*
		 * Don't delete the first item; it's
		 * the title item and doesn't change
		 * from app to app.
		 */
		for (i = (int)xv_get(menu, MENU_NITEMS); i > 1; i--) {
#ifdef DEBUG
			fprintf(stderr, "Destroying item '%s'\n",
			    xv_get(xv_get(menu, MENU_NTH_ITEM, i),
			    MENU_STRING));
#endif
			mi = (Menu_item)xv_get(menu, MENU_NTH_ITEM, i);
			xv_set(menu, MENU_REMOVE, i, NULL);
			xv_destroy(mi);
		}
#ifdef DEBUG
		fprintf(stderr,
		    "re-initializing existing menu '%s'\n", name);
#endif
	} else {
		menu = (Menu)xv_create(NULL, MENU,
		    MENU_TITLE_ITEM,	name,
		    MENU_CLIENT_DATA,	Curapp,
		    MENU_GEN_PIN_WINDOW, Base_BaseWin->BaseWin, name,
		    NULL);
#ifdef DEBUG
		if (menu)
			fprintf(stderr, "created menu '%s'\n", name);
#endif
	}
	while (*client_data) {
		mi = (Menu_item)xv_create(NULL, MENUITEM,
		    MENU_STRING,	(*client_data)->f_name,
		    MENU_CLIENT_DATA,	*client_data,
		    MENU_NOTIFY_PROC,	file_menu_notify,
		    NULL);
		xv_set(menu, MENU_APPEND_ITEM, mi, NULL);
		client_data++;
	}
	xv_set(menu_item, MENU_PULLRIGHT, menu, NULL);
	return (menu);
}

/*ARGSUSED*/
static void
file_menu_notify(Menu menu, Menu_item menu_item)
{
	/*
	 * Generic file run routine
	 */
#ifdef DEBUG
	fprintf(stderr,
	    "file_menu_notify:  %s\n", xv_get(menu_item, MENU_STRING));
#endif
	(void) runfile((File *)xv_get(menu_item, MENU_CLIENT_DATA));
}

/*
 * Customize the menu for the selected module
 * then display it.
 */
static void
menu_app(Module *mod, Xv_window win, Event *event)
{
	register int i;
	Menu_item mi;
	char	*name;
	Modinfo	*modinfo;

	if ((Module *)xv_get(Mods_menu, MENU_CLIENT_DATA) == mod) {
		if (event && event_action(event) == ACTION_MENU)
			menu_show(Mods_menu, win, event, 0);
		return;
	}
	xv_set(Mods_menu, MENU_CLIENT_DATA, mod, NULL);

	if (mod == (Module *)0 || mod->type == PRODUCT)
		modinfo = (Modinfo *)0;
	else
		modinfo = mod->info.mod;

	for (i = (int)xv_get(Mods_menu, MENU_NITEMS); i > 0; i--) {
		mi = (Menu_item)xv_get(Mods_menu, MENU_NTH_ITEM, i);
		name = (char *)xv_get(mi, MENU_STRING);
		if (name == (char *)0)
			continue;

		if (strcmp(name, gettext("Read About")) == 0) {
			if (modinfo && modinfo->m_text) {
				(void) gen_menu(mi, name, modinfo->m_text);
				xv_set(mi, MENU_INACTIVE, FALSE, NULL);
			} else
				xv_set(mi, MENU_INACTIVE, TRUE, NULL);
		} else if (strcmp(name, gettext("Run")) == 0) {
			if (modinfo && modinfo->m_demo) {
				(void) gen_menu(mi, name, modinfo->m_demo);
				xv_set(mi, MENU_INACTIVE, FALSE, NULL);
			} else
				xv_set(mi, MENU_INACTIVE, TRUE, NULL);
		} else if (strcmp(name,
		    gettext("Set Base Directory...")) == 0) {
			if (get_mode() == MODE_INSTALL && mod &&
			    mod->type != UNBUNDLED_4X)
				xv_set(mi, MENU_INACTIVE, FALSE, NULL);
			else
				xv_set(mi, MENU_INACTIVE, TRUE, NULL);
		} else if (strcmp(name, gettext("Expand")) == 0) {
			if (mod && mod->sub && mod != GetCurrentLevel())
				xv_set(mi, MENU_INACTIVE, FALSE, NULL);
			else
				xv_set(mi, MENU_INACTIVE, TRUE, NULL);
		} else if (strcmp(name,
		    gettext("Package Properties...")) == 0) {
			if (mod)
				xv_set(mi, MENU_INACTIVE, FALSE, NULL);
			else
				xv_set(mi, MENU_INACTIVE, TRUE, NULL);
		} else if (strcmp(name,
		    gettext("Select Localizations...")) == 0) {
			Module	*prod = mod ?
			    get_parent_product(mod) : (Module *)0;

			if (get_mode() == MODE_INSTALL &&
			    prod && prod->info.prod->p_locale)
				xv_set(mi, MENU_INACTIVE, FALSE, NULL);
			else
				xv_set(mi, MENU_INACTIVE, TRUE, NULL);
		}
	}

	if (event && event_action(event) == ACTION_MENU)
		menu_show(Mods_menu, win, event, 0);
}

static void
default_app(Module *mod, Xv_window win, Event *event)
{
	Menu_item mi;
	Menu (*gen_proc)();
	Menu pullright;

	/*
	 * Double-clicking a product, cluster, or metacluster
	 * expands the module.  For modules without descendents,
	 * double-clicking invokes the menu default.
	 */
	if (mod && mod->sub)
		SetCurrentLevel((char *)0, mod);
	else if ((mi = (Menu_item)xv_get(
	    Mods_menu, MENU_DEFAULT_ITEM)) == (Menu_item)0)
		/*
		 * If no default, show properties.
		 */
		GetPackageInfo(mod);
	else if (xv_get(mi, MENU_INACTIVE) == FALSE) {
		gen_proc = (Menu(*)())xv_get(mi, MENU_GEN_PROC);
		if (gen_proc) {
			/*
			 * If the default item has a
			 * gen proc, run it.
			 */
			gen_proc(mi, MENU_NOTIFY);
		} else {
			/*
			 * If the default item has a
			 * pullright menu, show it.
			 */
#ifdef DEBUG
			fprintf(stderr, "menu item %s has no GEN_PROC\n",
				(char *)xv_get(mi, MENU_STRING));
#endif
			pullright = (Menu)xv_get(mi, MENU_PULLRIGHT);
			if (pullright)
				menu_show(pullright, win, event, 0);
		}
	}
}

static int
app_is_displayed(Module *mod)
{
	Module	*child;
	Modinfo	*info;

	if (mod == (Module *)0 || mod->type == NULLPRODUCT)
		return (FALSE);

	if (mod->sub) {
		for (child = mod->sub; child; child = child->next)
			if (app_is_displayed(child))
				return (TRUE);
	} else {
		for (info = mod->info.mod; info; info = next_inst(info)) {
			if (info->m_shared == NOTDUPLICATE ||
			    info->m_shared == SPOOLED_NOTDUP)
				return (TRUE);
		}
	}
	return (FALSE);
}

static int
app_is_selected(APP_INFO *app)
{
	Module		*mod = app->app_data;
	ModStatus	status;
	int		selected = 0;

	if (get_mode() == MODE_INSTALL) {
		if (mod->type == PRODUCT)
			status = mod->info.prod->p_status;
		else
			status = mod->info.mod->m_status;
	} else {
		if (mod->info.mod->m_action == TO_BE_REMOVED)
			status = SELECTED;
		else
			status = UNSELECTED;
	}

	if ((app != app_pending && status == SELECTED) ||
	    (app == app_pending && status != SELECTED) ||
	    status == REQUIRED)
		selected = 1;

	return (selected);
}

static int
app_is_loaded(APP_INFO *app)
{
	Module	*mod = app->app_data;	/* Module of interest */
	Module	*inst_media;		/* installed media pointer */
	Module	*inst_prod;		/* installed version of product */
	Modinfo	*inst_mod;		/* installed version of module */
	Modinfo	*new_mod;		/* module on distribution media */
	Node	*tnode;
	int	loaded = LOADED_NO_VERSION;

	if ((inst_media = get_installed_media()) == (Module *)0)
		return (loaded);
	/*
	 * XXX When installed products are represented on
	 * the media chain insert code here that matches
	 * the installed product and the [new] source product.
	 */
	inst_prod = inst_media->sub;

	if (get_mode() == MODE_REMOVE) {
		/*
		 * Status during removal is straightforward:
		 * any non-NULLPKG is loaded; clusters are
		 * fully loaded if no component is a NULLPKG.
		 */
		if (mod->type == PRODUCT || mod->type == NULLPRODUCT) {
			if (mod->info.prod->p_status == LOADED)
				loaded = LOADED_THIS_VERSION;
		} else
			loaded = check_components(mod, inst_prod, loaded);
		return (loaded);
	}
	/*
	 * Status during installation is a bit more
	 * complicated:  we must look for installed
	 * counterparts to the modules on the source
	 * media.
	 */
	new_mod = mod->info.mod;
	if (mod->type == PRODUCT || mod->type == NULLPRODUCT) {
		/*
		 * Since installed products aren't represented
		 * as such, examine each one's [package] components
		 * looking for installed counterparts.  Product
		 * is considered completely installed if all
		 * its parts are installed.
		 */
		loaded = check_components(mod, inst_prod, loaded);
	} else if (mod->type == METACLUSTER || mod->type == CLUSTER) {
		/*
		 * Find counterpart installed cluster.
		 * Examine version numbers first, then
		 * components if versions match.
		 */
		tnode = findnode(
		    inst_prod->info.prod->p_clusters,
		    new_mod->m_pkgid);
		if (tnode != (Node *)0) {
			inst_mod = ((Module *)tnode->data)->info.mod;
			if (strcmp(inst_mod->m_version,
			    new_mod->m_version) == 0)
				/*
				 * Check all component packages
				 */
				loaded =
				    check_components(mod, inst_prod, loaded);
			else
				loaded = LOADED_OTHER_VERSION;
		}
	} else
		/*
		 * Find counterpart installed package.
		 * Find a real instance (dups are ok)
		 * then examine version numbers.
		 */
		loaded = check_components(mod, inst_prod, loaded);

	return (loaded);
}

/*
 * Determine if all components of a cluster, metacluster,
 * or product are loaded.  Returns load status based on
 * the argument value and whether components matched as
 * follows:
 *
 *	loaded == LOADED_NO_VERSION
 *		Returns LOADED_NO_VERSION	if no match found
 *		Returns LOADED_OTHER_VERSION	if different version found
 *		Returns LOADED_THIS_VERSION	if same version found
 *
 *	loaded == LOADED_OTHER_VERSION
 *		Returns LOADED_OTHER_VERSION
 *
 *	loaded == LOADED_THIS_VERSION
 *		Returns LOADED_OTHER_VERSION	if no match or diff version
 *		Returns LOADE_THIS_VERSION	if same version found
 */
static int
check_components(Module *mod, Module *inst_prod, int loaded)
{
	Module	*subp;
	Modinfo	*inst_mod = (Modinfo *)0;
	Modinfo	*info;
	Node	*tnode;

	if (loaded == LOADED_OTHER_VERSION)
		return (LOADED_OTHER_VERSION);

	if (mod->type == PACKAGE) {	/* just check packages? */
		if (get_mode() == MODE_INSTALL) {
			tnode = findnode(
			    inst_prod->info.prod->p_packages,
			    mod->info.mod->m_pkgid);
			if (tnode != (Node *)0)
				inst_mod = (Modinfo *)tnode->data;
		} else
			inst_mod = mod->info.mod;
		if (inst_mod == (Modinfo *)0 || inst_mod->m_shared == NULLPKG) {
			/*
			 * No version is installed
			 */
			if (loaded == LOADED_NO_VERSION)
				return (loaded);
			else
				return (LOADED_OTHER_VERSION);
		} else if (inst_mod->m_version == (char *)0) {
			return (LOADED_OTHER_VERSION);	/* or this_version? */
		} else {
			/*
			 * Some version is installed, look through
			 * all instances and find out if source
			 * version is installed
			 */
			for (info = mod->info.mod;
			    info != (Modinfo *)0;
			    info = next_inst(info)) {
				if (info->m_shared == NULLPKG ||
				    info->m_version == (char *)0)
					continue;
				if (strcmp(inst_mod->m_version,
				    info->m_version) == 0)
					break;
			}
			if (info != (Modinfo *)0) {
				/*
				 * matched versions
				 */
				if (loaded == LOADED_NO_VERSION)
					return (LOADED_THIS_VERSION);
				else
					return (loaded);
			} else
				/*
				 * mis-matched versions
				 */
				return (LOADED_OTHER_VERSION);
		}
	} else {
		int	partial = FALSE;

		for (subp = mod->sub; subp != (Module *)0; subp = subp->next) {
			loaded = check_components(subp, inst_prod, loaded);
			if (loaded == LOADED_NO_VERSION)
				partial = TRUE;
			else if (loaded == LOADED_OTHER_VERSION)
				break;	/* optimization */
		}

		if (partial == TRUE && loaded == LOADED_THIS_VERSION)
			loaded = LOADED_OTHER_VERSION;
	}
	return (loaded);
}

static int
app_is_required(APP_INFO *app)
{
	Module	*mod = app->app_data;	/* Module of interest */
	int	required;

	if (mod->type == PRODUCT || mod->type == NULLPRODUCT)
		required = FALSE;
	else {
		if (mod->info.mod->m_status == REQUIRED)
			required = TRUE;
		else
			required = FALSE;
	}
	return (required);
}

static void
RenderIcon(APP_INFO *app, short x_pos, short y_pos)
{
	Drawable	draw_xid;
	Font_string_dims dims;
	int		need_drawable = 1;
	u_long		func;
	short		start_posx;
	short		i, w;
	char		name[MAXNAMELEN];
	Module		*mod = app->app_data;
	File		*icon;
	int		selected, loaded;
	int		font_style;

	selected = app_is_selected(app);
	loaded = app_is_loaded(app);

	if (app_is_required(app) == TRUE)
		font_style = BOLD;
	else
		font_style = NORMAL;

	icon = get_module_icon(mod);
	/*
	 * Create server image for application, if needed
	 */
	if (icon != NULL) {
		if (icon->f_data == (void *)0) {
			if (icon->f_type == X11BITMAP)
				icon->f_data = (void *)icon_load_x11bm(
					icon->f_path, gettext("Bad icon file"));
			else
				icon->f_data = (void *)icon_load_svrim(
					icon->f_path, gettext("Bad icon file"));
		}
		app->app_simage = (Server_image)icon->f_data;

		if (app->app_simage != XV_NULL) {
			app->app_drawable =
				(Drawable)xv_get(app->app_simage, XV_XID);
			need_drawable = 0;
		} else {
			app->app_drawable = XV_NULL;
			need_drawable = 1;
		}
	}

	if (need_drawable) {
		if (mod->sub != (Module *)0)
			draw_xid = cluster_app_image;
		else
			draw_xid = pkg_app_image;
	} else
		draw_xid = app->app_drawable;

	/*
	 * Set drawing colors -- draw in reverse
	 * video if the application is selected
	 * but not clicked on, or if not selected
	 * but clicked on (potentially selected).
	 */
	if (selected) {
		Gc_val.foreground = Background;
		Gc_val.background = Foreground;
	} else {
		Gc_val.foreground = Foreground;
		Gc_val.background = Background;
	}

	x_pos += params->col_offset;
	y_pos += params->row_offset;

	/* Now display icon correctly */
	Gc_val.fill_style = FillOpaqueStippled;
	Gc_val.stipple = draw_xid;
	Gc_val.ts_y_origin = y_pos;
	Gc_val.ts_x_origin = x_pos;
	func = GCFillStyle|GCTileStipXOrigin|GCTileStipYOrigin|
	    GCStipple|GCForeground|GCBackground;

	XChangeGC(display, Mods_gc, func, &Gc_val);

	XFillRectangle(display, Mods_xid, Mods_gc,
	    x_pos, y_pos,
	    params->glyph_width, params->glyph_height);

	/*
	 * Draw border around application if already
	 * installed on system:  solid border if same
	 * version fully installed, dashed border if
	 * same version partially installed or different
	 * version partially or fully installed.
	 */
	if (loaded != LOADED_NO_VERSION) {
		if (loaded == LOADED_OTHER_VERSION)
			Gc_val.line_style = LineOnOffDash;
		else
			Gc_val.line_style = LineSolid;
		Gc_val.line_width = 2;
		Gc_val.foreground = Foreground;
		Gc_val.background = Background;
		Gc_val.fill_style = FillSolid;
		Gc_val.stipple = draw_xid;
		Gc_val.ts_y_origin = y_pos - 3;
		Gc_val.ts_x_origin = x_pos - 3;
		func = GCLineStyle|GCLineWidth|GCFillStyle|GCTileStipXOrigin|
			GCTileStipYOrigin|GCStipple|GCForeground|GCBackground;

		XChangeGC(display, Mods_gc, func, &Gc_val);

		XDrawRectangle(display, Mods_xid, Mods_gc,
		    x_pos - 3, y_pos - 3,
		    params->glyph_width + 6,
		    params->glyph_height + 6);
	}

	/*
	 * Now display the application name
	 */
	(void) strcpy(name, app->app_name);
	(void) xv_get(var_font[font_style], FONT_STRING_DIMS, name, &dims);
	w = dims.width;
	/*
	 * Is the name too wide in pixels?
	 */
	if (w > (params->col_size - (2 * NAME_SPACE)) || w > params->col_size) {
		/* truncate and add a `>` */
		w = xv_get(var_font[font_style], FONT_CHAR_WIDTH, '>');
		for (i = 0; (name[i] != NULL) &&
		    (w < (params->col_size - (2 * NAME_SPACE))); i++)
			w += xv_get(
				var_font[font_style], FONT_CHAR_WIDTH, name[i]);

		if (name[i] != NULL) {
			name[i] = NULL;
			name[i - 1] = INSTB_GT;
		}
	}
	start_posx = ((params->col_size - (2 * params->name_margin) - w) / 2) -
		params->col_offset;

	Gc_val.foreground = Foreground;
	Gc_val.background = Background;
	func = GCForeground|GCBackground;
	XChangeGC(display, Mods_tgc, func, &Gc_val);

	XSetFont(display, Mods_tgc, xv_get(var_font[font_style], XV_XID));

	XDrawString(display, Mods_xid, Mods_tgc,
	    x_pos + params->name_margin + start_posx,
	    y_pos + params->name_margin + params->glyph_height + lineheight,
	    name, strlen(name));
}

static void
RenderText(APP_INFO *app, short x_pos, short y_pos)
{
	Drawable	draw_xid;
	Font_string_dims dims;
	u_long		func, space;
	char		name[MAXNAMELEN];
	char		dots[256];
	char		spacestr[20];
	Module		*mod = app->app_data;
	register	int i, w;
	short		start_x, end_x, dot_x;
	int		selected, loaded;
	int		font_style;

	selected = app_is_selected(app);
	loaded = app_is_loaded(app);

	if (app_is_required(app) == TRUE)
		font_style = BOLD;
	else
		font_style = NORMAL;

	if (mod->sub != (Module *)0)
		draw_xid = cluster_type_image;
	else
		draw_xid = pkg_type_image;
	/*
	 * Set drawing colors -- draw in reverse
	 * video if the application is selected
	 * but not clicked on, or if not selected
	 * but clicked on (potentially selected).
	 */
	if (selected) {
		Gc_val.foreground = Background;
		Gc_val.background = Foreground;
	} else {
		Gc_val.foreground = Foreground;
		Gc_val.background = Background;
	}

	y_pos += params->row_offset;

	/* Now display icon correctly */
	Gc_val.fill_style = FillOpaqueStippled;
	Gc_val.stipple = draw_xid;
	Gc_val.ts_y_origin = y_pos;
	Gc_val.ts_x_origin = x_pos + params->col_offset;
	func = GCFillStyle|GCTileStipXOrigin|GCTileStipYOrigin|
	    GCStipple|GCForeground|GCBackground;

	XChangeGC(display, Mods_gc, func, &Gc_val);

	XFillRectangle(display, Mods_xid, Mods_gc,
	    x_pos + params->col_offset, y_pos,
	    params->glyph_width, params->glyph_height);

	Gc_val.foreground = Foreground;
	Gc_val.background = Background;
	/*
	 * Draw border around application if already
	 * installed on system:  solid border if same
	 * version fully installed, dashed border if
	 * same version partially installed or different
	 * version partially or fully installed.
	 */
	if (loaded != LOADED_NO_VERSION) {
		if (loaded == LOADED_OTHER_VERSION)
			Gc_val.line_style = LineOnOffDash;
		else
			Gc_val.line_style = LineSolid;
		Gc_val.line_width = 1;
		Gc_val.fill_style = FillSolid;
		Gc_val.stipple = draw_xid;
		Gc_val.ts_y_origin = y_pos - 2;
		Gc_val.ts_x_origin = x_pos + params->col_offset - 2;
		func = GCLineStyle|GCLineWidth|GCFillStyle|GCTileStipXOrigin|
			GCTileStipYOrigin|GCStipple|GCForeground|GCBackground;

		XChangeGC(display, Mods_gc, func, &Gc_val);

		XDrawRectangle(display, Mods_xid, Mods_gc,
		    x_pos + params->col_offset - 2, y_pos - 2,
		    params->glyph_width + 4,
		    params->glyph_height + 4);
	}

	func = GCForeground|GCBackground;
	XChangeGC(display, Mods_tgc, func, &Gc_val);

	start_x = x_pos + params->col_offset +
		params->glyph_width + params->name_margin;
	end_x = x_pos + params->col_size -
		(sizewidth[font_style] + (2 * params->name_margin));

	XSetFont(display, Mods_tgc, xv_get(var_font[font_style], XV_XID));

	/*
	 * Now display the application name
	 */
	(void) strcpy(name, app->app_name);
	(void) xv_get(var_font[font_style], FONT_STRING_DIMS, name, &dims);
	w = dims.width;
	/*
	 * Is the name too wide in pixels?
	 */
	if (w > (int)(end_x - start_x)) {
		/* truncate and add a `>` */
		w = xv_get(var_font[font_style], FONT_CHAR_WIDTH, '>');
		for (i = 0;
		    (name[i] != NULL) && (w < (int)(end_x - start_x)); i++)
			w += xv_get(
				var_font[font_style], FONT_CHAR_WIDTH, name[i]);

		if (name[i] != NULL) {
			name[i] = NULL;
			name[i - 1] = INSTB_GT;
		}
		dot_x = -1;
	} else {
		char	*str = dots;
		/*
		 * Align the dots
		 */
		dot_x = start_x + w;
		dot_x +=
		    (fontwidth[font_style] - (dot_x % fontwidth[font_style]));
		for (i = dot_x; i < (int)end_x; i += fontwidth[font_style])
			*str++ = '.';
		*str = 0;
	}
	XDrawString(display, Mods_xid, Mods_tgc,
	    start_x, y_pos + lineheight, name, strlen(name));
	XSetFont(display, Mods_tgc, xv_get(fixed_font[font_style], XV_XID));

	if (dot_x != -1) {
		XDrawString(display, Mods_xid, Mods_tgc,
		    dot_x, y_pos + lineheight,
		    dots, strlen(dots));
	}

	start_x = end_x + params->name_margin;

	space = calc_total_space(mod);
	(void) sprintf(spacestr, "%6.2f Mb", ((float)space) / 1024);

	XDrawString(display, Mods_xid, Mods_tgc,
	    start_x, y_pos + lineheight,
	    spacestr, strlen(spacestr));
}

/*
 * Called when user chooses select/deselect-all
 * from edit menu.  Selection only applies to
 * modules matching current category and cluster;
 * de-selection applies to ALL selected modules
 * (on current media).
 */
void
SelectAllModules(selecting)
	int	selecting;	/* =1 if selecting, 0 if de-selecting */
{
	Module *category = get_current_category(MEDIA);
	Module	*mod, *parent;
	register int i, j;

	parent = GetCurrentLevel();
	if (parent == (Module *)0 || active_media == (Module *)0)
		return;

	/*
	 * Semantics of select and de-select are
	 * slightly different.  Select operates
	 * on modules that match category and
	 * cluster criteria, de-select operates
	 * on ALL selected modules.
	 */
	if (selecting) {
		for (mod = get_sub(parent); mod; mod = get_next(mod)) {
			if (!in_category(category, mod))
				continue;
			mark_selection(mod, MOD_SELECT);
		}
	} else {
		for (mod = get_sub(active_media);
		    mod != (Module *)0; mod = get_next(mod))
			mark_selection(mod, MOD_DESELECT);
	}

	UpdateButtons();
	UpdateMeter(0);

	markdisplay(0, 0, params->end_major, params->max_minor);

	for (i = 0; i <= params->end_major; i++)
		for (j = 0; j <= params->max_minor; j++)
			display_rect(i, j);
}

Module *
GetCurrentModule(void)
{
	return (Curapp);
}
