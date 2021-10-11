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
#ident	"@(#)Display.h 1.3 92/11/16"
#endif

#include <X11/X.h>
#include <xview/svrimage.h>

#define	MAXNUMROW	10
#define	MAXNUMCOLUMN	20

/* One structure of this type is kept for each application */
struct app_info {
        char		*app_name;	/* pointer to module's name */
        int		app_width;	/* width of name in pixels */
        Server_image	app_simage;	/* server image for module's icon */
        Drawable	app_drawable;	/* drawable for icon */
	Module		*app_data;	/* pointer to module info */
	int		app_repaint;	/* repaint flag */
};
typedef struct app_info APP_INFO;

APP_INFO	Level_array[MAXNUMCOLUMN];	/* XXX */
