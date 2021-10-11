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
#ident	"@(#)xv_category.c 1.4 94/10/13"
#endif

#include "defs.h"
#include "ui.h"
#include "Props_ui.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

extern  Props_PropsWin_objects *Props_PropsWin;

static	int	last_selected;	/* for "reset" */

void
InitCategory(Module *media)
{
	Xv_opaque list = Props_PropsWin->CatList;
	Module	*mod;
	Category *catp;
	char	*lastname = (char *)0;
	int	nrows = (int)xv_get(list, PANEL_LIST_NROWS);
	int	row = (int)xv_get(list, PANEL_LIST_FIRST_SELECTED);

	if (row != -1)	/* previous selection */
		lastname =
		    xstrdup((char *)xv_get(list, PANEL_LIST_STRING, row));

	if (nrows > 0)
		xv_set(list, PANEL_LIST_DELETE_ROWS, 0, nrows, NULL);

	if (media != (Module *)0) {
		nrows = 0;
		media_category(media);
		for (mod = get_default_category(MEDIA); mod != (Module *)0;
		    mod = get_next(mod)) {
			catp = mod->info.cat;
			xv_set(list,
				PANEL_LIST_INSERT,	nrows,
				PANEL_LIST_STRING,	nrows,	catp->cat_name,
				PANEL_LIST_CLIENT_DATA,	nrows,	mod,
				NULL);
			/*
			 * Select the previously-selected category
			 */
			if (lastname != (char *)0 &&
			    strcmp(lastname, catp->cat_name) == 0) {
				free(lastname);
				lastname = (char *)0;
				xv_set(list,
					PANEL_LIST_SELECT, nrows, TRUE, NULL);
			}
			nrows++;
		}
		/*
		 * If no previous selection or we
		 * didn't find the previous selection
		 * in the current list of categories,
		 * select the first category ("All").
		 */
		if (row == -1 || lastname != (char *)0)
			xv_set(list, PANEL_LIST_SELECT, 0, TRUE, NULL);
	}
	free(lastname);
	SetCategory();
}

void
SetCategory(void)
{
	Xv_opaque list = Props_PropsWin->CatList;
	Module	*mod;
	int	row;

	row = (int)xv_get(list, PANEL_LIST_FIRST_SELECTED);
	last_selected = row;
	if (row == -1)
		mod = (Module *)0;
	else {
		mod = (Module *)xv_get(list, PANEL_LIST_CLIENT_DATA, row);
		set_current(mod);
	}
	UpdateCategory(mod);
}

void
#ifdef __STDC__
ResetCategory(void)
#else
ResetCategory()
#endif
{
	Xv_opaque list = Props_PropsWin->CatList;
	int	row = last_selected;

	if (row != -1)
		xv_set(list, PANEL_LIST_SELECT, row, TRUE, NULL);
	else {
		row = (int)xv_get(list, PANEL_LIST_FIRST_SELECTED);
		xv_set(list, PANEL_LIST_SELECT, row, FALSE, NULL);
	}
}
