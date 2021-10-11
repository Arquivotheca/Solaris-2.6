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
#ident	"@(#)Levels.c 1.7 93/04/09"
#endif

/*
 * Level.c - Create and manipulate levels canvas
 */

#include "defs.h"
#include "ui.h"
#include <string.h>
#include <ctype.h>
#include <sys/param.h>
#include <xview/canvas.h>
#include <xview/icon_load.h>
#include <xview/font.h>
#include <xview/scrollbar.h>
#include <xview/svrimage.h>
#include <xview/termsw.h>
#include <xview/text.h>
#include <xview/tty.h>
#include <xview/cms.h>
#include <xview/openmenu.h>
#include <xview/win_input.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "swmtool.h"
#include "Display.h"

typedef enum { INSTB_FALSE = 0, INSTB_TRUE = 1 } BOOLEAN;

#define	GLYPH_HEIGHT	32
#define	GLYPH_WIDTH	32

#define	COL_OFFSET	20
#define	COL_SEGOFFSET	26
#define	COL_RANGE	8

#define	TOP_MARGIN	0
#define	LEFT_MARGIN	16

#define	NAME_SPACE	4

#define	INSTB_GT	'>'
#define	INSTB_HYPHEN	'-'
#define	INSTB_BLANK	' '

static	int	Column_incr;

static	u_long		Foreground;
static	u_long		Background;

static	int		level_init;
static	GC		level_gc, level_tgc;
static	XGCValues	Gc_val;
static	Xv_Font		level_font;

static	int		lineheight;

static	Menu_item	prev_level;

static	Drawable	open_image;
static	Drawable	closed_image;

static	int		Endcolumn;
static	int		Maxcolumn;

static	int		Width;
static	int		Height;

static	Xv_window	level_cnvs_pw;
static	Window		level_cnvs_xid;

static	APP_INFO	*Curlevel;

static void init_font(void);
static void init_gc(void);
static void init_icons(void);
static void markdisplay(short, short);
static void display_rect(short, Display *, Window);
static void clear_levels(int, int);

/*
 * Create object `level_cnvs' in the specified instance.
 */
/*ARGSUSED*/
void
InitLevels(caddr_t ip, Xv_opaque owner)
{
	/*LINTED [alignment ok]*/
	Base_BaseWin_objects *objects = (Base_BaseWin_objects *)ip;
	Menu	menu;

	/*
	 * Maintain pointer to Previous Level
	 * menu item so when user changes level
	 * we can update item.
	 *
	 * TODO:  move this out of this file
	 */
	menu = (Menu)xv_get(Base_BaseWin->BaseView, PANEL_ITEM_MENU);
	prev_level = (Menu_item)xv_find(menu, MENUITEM,
		MENU_STRING,	gettext("Prev Level"),
		NULL);

	Width = (int) xv_get(Base_BaseWin->Levels, CANVAS_WIDTH);
	Height = (int) xv_get(Base_BaseWin->Levels, CANVAS_HEIGHT);

	level_cnvs_pw = xv_get(objects->Levels, CANVAS_NTH_PAINT_WINDOW, 0);
	level_cnvs_xid = xv_get(level_cnvs_pw, XV_XID);

	init_font();
	init_gc();
	init_icons();

	/*
	 * Establish column geometry
	 */
	Column_incr = GLYPH_WIDTH + (2 * COL_OFFSET);
	Maxcolumn = MAXNUMCOLUMN - 1;
	Endcolumn = -1;

	clear_levels(0, Maxcolumn);
	level_init++;
}

static void
init_font(void)
{
	/*
	 * Set font for window
	 */
	level_font = (Xv_Font)xv_find(Base_BaseWin->BaseWin, FONT,
		FONT_FAMILY,	FONT_FAMILY_DEFAULT,
		FONT_STYLE,	FONT_STYLE_NORMAL,
		FONT_SCALE,	WIN_SCALE_SMALL,
		0);

	if (level_font == (Xv_Font)0) {
		fprintf(stderr, gettext(
		    "WARNING:  Small font unavailable, using frame font"));
		/* Use the frame's font */
		level_font = (Xv_Font) xv_get(Base_BaseWin->BaseWin, XV_FONT);
	}

	lineheight = xv_get(level_font, FONT_DEFAULT_CHAR_HEIGHT);
}

/*
 * Set font graphical context for window
 */
static void
init_gc(void)
{
	Foreground = BlackPixel(display, DefaultScreen(display));
	Background = WhitePixel(display, DefaultScreen(display));

	Gc_val.foreground = Foreground;
	Gc_val.background = Background;
	Gc_val.fill_style = FillOpaqueStippled;

	level_gc = (GC)XCreateGC(display, level_cnvs_xid,
	    (GCForeground | GCBackground | GCFillStyle), &Gc_val);

	level_tgc =  (GC)XCreateGC(display, level_cnvs_xid,
	    (GCForeground|GCBackground), &Gc_val);

	(void) XSetFont(display, level_tgc, xv_get(level_font, XV_XID));

	if (level_gc == 0 || level_tgc == 0) {
		fprintf(stderr, gettext("PANIC:  GC create failed\n"));
		exit(1);
	}
}

/*
 * Create server image and xid for default glyphs
 */
static void
init_icons(void)
{
	static u_short open[] = {
#include	"icons/OpenBox.icon"
	};
	static u_short closed[] = {
#include	"icons/ClosedBox.icon"
	};

	open_image = server_image_xid(open, GLYPH_HEIGHT, GLYPH_WIDTH);
	closed_image = server_image_xid(closed, GLYPH_HEIGHT, GLYPH_WIDTH);
}

/*
 * Event callback function for `Levels'.
 */
/*ARGSUSED*/
void
SelectLevel(win, event, arg, type)
	Xv_Window	win;
	Event		*event;
	Notify_arg	arg;
	Notify_event_type type;
{
	int	col;
	int 	xpos;
	int	ypos;
	APP_INFO	*newapp;

	if (event_action(event) == ACTION_SELECT && event_is_up(event)) {
		/*
		 * Determine the row and column of the mouse
		 */
		xpos = event_x(event) - LEFT_MARGIN;

		ypos = event_y(event) - TOP_MARGIN;

		/* Are we in the margins */
		if (xpos < 0 || ypos < 0)
			return;

		col = xpos / Column_incr;

		/*
		 * Check if we have a position with a real application
		 * in it
		 */
		/* Is it beyond existing apps */
		if (col > Endcolumn)
			return;
		/* Is it between apps */
		if (((xpos % Column_incr) < (COL_OFFSET - COL_RANGE)) ||
		    ((xpos % Column_incr) > COL_OFFSET +
		    COL_RANGE + GLYPH_WIDTH))
			return;

		/* The mouse is pointing to a real application */
		newapp = &Level_array[col];
		SetCurrentLevel((char *)0, newapp->app_data);
	}
}

/*
 * LevelRepaint -- it's a small and easy canvas -- just
 * repaint the whole thing.
 */
/*ARGSUSED*/
void
RepaintLevels(canvas, paint_window, dpy, xwin, rects)
	Canvas		canvas;
	Xv_window	paint_window;
	Display		*dpy;
	Window		xwin;
	Xv_xrectlist	*rects;
{
	short 	lowcol;
	int	newwidth, newheight;

	if (!level_init)
		return;

	newwidth = (int) xv_get(Base_BaseWin->Levels, XV_WIDTH);
	newheight = (int) xv_get(Base_BaseWin->Levels, XV_HEIGHT);

	if (newwidth != Width) {
#ifdef DEBUG
		fprintf(stderr,
		    "Width Resize event, old width = %d, new width = %d\n",
			Width, newwidth);
#endif DEBUG
		Width = newwidth;
		xv_set(Base_BaseWin->Levels, CANVAS_WIDTH, Width, 0);
	}
	if (newheight != Height) {
#ifdef DEBUG
		fprintf(stderr,
		    "Height Resize event, old height = %d, new height = %d\n",
			Height, newheight);
#endif DEBUG
		xv_set(Base_BaseWin->Levels, CANVAS_HEIGHT, Height, 0);
	}

	markdisplay(0, Maxcolumn);

	for (lowcol = 0; lowcol <= (int)Maxcolumn; lowcol++)
		display_rect(lowcol, dpy, xwin);
	return;
}

static void
markdisplay(short lowcol, short highcol)
{
	int i;

	if (lowcol < 0)
		lowcol = 0;
	if (highcol < 0)
		highcol = 0;

	for (i = lowcol; i <= highcol; i++)
		Level_array[i].app_repaint = 1;
}
/*
 * Display the application in the position col
 */
static void
display_rect(short col, Display *dpy, Window xwin)
{
	Font_string_dims dims;
	APP_INFO	*level;
	unsigned long	func;
	Drawable	draw_xid;
	short		y_pos;
	short		x_pos;
	short		start_posx;
	short		i, w;
	char		name[MAXNAMELEN];
	char		name2[MAXNAMELEN];
	int		splitline;

	if (col > Endcolumn)
		return;

	level = &Level_array[col];
	level->app_repaint = 0;

	x_pos = col * Column_incr + LEFT_MARGIN;
	y_pos = TOP_MARGIN;

	Gc_val.foreground = Foreground;
	Gc_val.background = Background;

	if (level <= Curlevel)
		level->app_drawable = open_image;
	else
		level->app_drawable = closed_image;

	draw_xid = level->app_drawable;

	/*
	 * Now display icon correctly
	 */
	Gc_val.stipple = draw_xid;
	Gc_val.ts_y_origin = y_pos;
	Gc_val.ts_x_origin = x_pos + COL_OFFSET;
	func = GCStipple | GCTileStipXOrigin | GCTileStipYOrigin |
		GCForeground | GCBackground;

	XChangeGC(dpy, level_gc, func, &Gc_val);

	XFillRectangle(dpy, xwin, level_gc, x_pos + COL_OFFSET,
	    y_pos, GLYPH_WIDTH, GLYPH_HEIGHT);

	/* Now display the level name */
	(void) strcpy(name, level->app_name);
	xv_get(level_font, FONT_STRING_DIMS, name, &dims);
	w = dims.width;
	splitline = 0;
	/*
	 * Is the name too wide in pixels
	 */
	if (w > (Column_incr - (2 * NAME_SPACE))) {
		/*
		 * truncate first line and add a `-`
		 */
		w = xv_get(level_font, FONT_CHAR_WIDTH, INSTB_HYPHEN);
		for (i = 0; (name[i] != NULL) &&
		    w < (Column_incr - (2 * NAME_SPACE)); i++)
			w += xv_get(level_font, FONT_CHAR_WIDTH, name[i]);
		if (name[i]) {
			name[i] = '\0';
			splitline = i - 1;
			if (isspace((u_char)name[splitline]) == 0 &&
			    isspace((u_char)name[splitline - 1]) == 0)
				name[splitline] = INSTB_HYPHEN;
			else
				name[splitline] = INSTB_BLANK;
		}
		/*
		 * truncate second line and add a '>'
		 * (if neccessary)
		 */
		while (isspace((u_char)level->app_name[splitline]))
			splitline++;
		(void) strcpy(name2, &level->app_name[splitline]);
		w = xv_get(level_font, FONT_CHAR_WIDTH, INSTB_GT);
		for (i = 0; (name2[i] != NULL) &&
		    w < (Column_incr - (2 * NAME_SPACE)); i++)
			w += xv_get(level_font, FONT_CHAR_WIDTH, name[i]);
		if (name2[i]) {
			name2[i] = '\0';
			name2[i - 1] = INSTB_GT;
		}
		start_posx = 0;
	} else
		start_posx =
			(Column_incr - (2 * NAME_SPACE) - w) / 2;

	XDrawString(dpy, xwin, level_tgc,
	    x_pos + NAME_SPACE + start_posx,
	    y_pos + GLYPH_HEIGHT + lineheight / 2,
	    name, strlen(name));

	if (splitline) {
		XDrawString(dpy, xwin, level_tgc,
		    x_pos + NAME_SPACE + start_posx,
		    y_pos + GLYPH_HEIGHT + (int)(lineheight * 1.5),
		    name2, strlen(name2));
	}
}

/*
 * This function clears the display array Level_array
 * from lowrow, lowcol through highrow, highcol
 *
 * It does NOT set the next valid position
 * this must be done by the calling routine.
 */
static void
clear_levels(lowcol, highcol)
	int	lowcol;
	int	highcol;
{
	int i;
	for (i = lowcol; i <= highcol; i++) {
		if (Level_array[i].app_name)
			free(Level_array[i].app_name);
		(void) memset((void *)&Level_array[i], 0, sizeof (APP_INFO));
	}
}

void
ClearLevels(void)
{
	xv_set(prev_level, MENU_INACTIVE, TRUE, NULL);

	Endcolumn = -1;
	clear_levels(0, Maxcolumn);
	XClearArea(display, level_cnvs_xid, 0, 0, Width, Height, FALSE);
}

void
SetDefaultLevel(name, mod)
	char	*name;
	Module	*mod;
{
	APP_INFO *lp;
	register int i;

	if (name == (char *)0)
		name = get_full_name(mod);

	ClearLevels();		/* reset level path */

	lp = Level_array;
	lp->app_name = xstrdup(name);
	lp->app_repaint = 1;
	lp->app_simage = (Server_image)0;
	lp->app_drawable = (Drawable)0;
	lp->app_data = mod;

	Curlevel = lp;
	Endcolumn = 0;

	for (i = 0; i <= Endcolumn; i++)
		display_rect(i, display, level_cnvs_xid);

	UpdateCategory((Module *)0);
}

/*
 * Sets current level.  Look for level in level
 * path.  Caller should get parent with a call
 * to PrevLevel.
 *
 * Sets Endcolumn.
 */
void
SetCurrentLevel(name, mod)
	char	*name;
	Module	*mod;
{
	register int i;
	APP_INFO *lp;
	int	col;

	if (name == (char *)0)
		name = get_full_name(mod);
	/*
	 * See if this level is in the path.
	 */
	for (lp = Level_array, col = 0; lp->app_name; lp++, col++) {
		if (lp->app_data == mod)
			break;
	}
	if (lp->app_name == (char *)0) {
		/*
		 * If not in path, it's either the
		 * next level or a new branch.
		 */
		if (Endcolumn == col) {
			/*
			 * next level
			 */
			Endcolumn += 1;
#ifdef DEBUG
			fprintf(stderr, "Add level %s\n", name);
#endif
		} else {
			/*
			 * new branch
			 */
			col = (Curlevel - Level_array) + 1;
			clear_levels(col, Maxcolumn);
			Endcolumn = col;
			lp = &Level_array[col];
			XClearArea(display, level_cnvs_xid,
				0, 0, Width, Height, FALSE);
#ifdef DEBUG
			fprintf(stderr, "New branch %s\n", name);
#endif
		}
		lp->app_name = xstrdup(name);
		lp->app_data = mod;
	}
	if (lp != Curlevel) {
		if (lp != Level_array)
			xv_set(prev_level, MENU_INACTIVE, FALSE, NULL);
		else
			xv_set(prev_level, MENU_INACTIVE, TRUE, NULL);

		lp->app_repaint = 1;
		lp->app_simage = (Server_image)0;
		lp->app_drawable = (Drawable)0;

		Curlevel = lp;

		for (i = 0; i <= Endcolumn; i++)
			display_rect(i, display, level_cnvs_xid);

		UpdateCategory((Module *)0);
	}
	UpdateModules();
}

/*
 * Get current level
 *
 * Returns:
 *	Non-null if current level set.
 *	NULL if no level set.
 */
Module *
GetCurrentLevel(void)
{
	Module	*level = (Module *)0;

	if (Curlevel && Curlevel->app_data)
		level = Curlevel->app_data;

	return (level);
}

/*
 * Get previous level
 *
 * Returns:
 *	Non-null, non-root if not at root.
 *	Non-null, root if at root.
 *	NULL if no levels set.
 */
Module *
GetPreviousLevel(void)
{
	Module	*prev;

	if (Curlevel == (APP_INFO *)0 || Curlevel->app_data == (Module *)0)
		/*
		 * No current level (Curlevel) set?
		 */
		prev = (Module *)0;
	else if (Curlevel == Level_array)
		/*
		 * At root?
		 */
		prev = Curlevel->app_data;
	else
		/*
		 * Return previous level
		 */
		prev = (Curlevel - 1)->app_data;

	return (prev);
}
