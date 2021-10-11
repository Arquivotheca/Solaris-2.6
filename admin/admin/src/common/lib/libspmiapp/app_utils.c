#ifndef lint
#pragma ident "@(#)app_utils.c 1.2 96/08/28 SMI"
#endif

/*
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	app_utils.c
 * Group:	libspmiapp
 * Description:
 */

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <strings.h>

#include "spmiapp_api.h"
#include "app_utils.h"

int
UI_ScalePercent(int real_percent, int scale_start, float scale_factor)
{
	int factored_percent;
	int final_percent;

	factored_percent = (int) (real_percent * scale_factor);
	final_percent = scale_start + factored_percent;
	if (final_percent > 100)
		final_percent = 100;
	return (final_percent);
}

/*
 * Function: UI_ProgressBarTrimDetailLabel
 * Description:
 *	Routine to trim the secondary label in the progress bars if
 *	necessary to ensure that the detail label is not too long.
 * Scope:	PUBLIC
 * Parameters:
 *	char *main_label: main label
 *	char *detail_label: secondary label we want to trim, if necessary
 *		- trimmed version in here if it's changed
 *	int total_len: total length of "main label: detail label" string
 * Return: none
 * Globals: none
 * Notes:
 */
#define	APP_UI_UPG_PROGRESS_CUT_STR	"..."
void
UI_ProgressBarTrimDetailLabel(
	char *main_label,
	char *detail_label,
	int total_len)
{
	int main_len;
	int detail_len;

	if (!detail_label)
		return;

	write_debug(APP_DEBUG_L1, "Original detail label: %s\n", detail_label);

	if (main_label)
		main_len = strlen(main_label) + 2;
	else
		main_len = 0;

	if (detail_label)
		detail_len = strlen(detail_label);
	else
		detail_len = 0;

	if ((main_len + detail_len) > total_len) {
		detail_len = total_len - main_len -
			strlen(APP_UI_UPG_PROGRESS_CUT_STR);
		if (detail_len < 0) {
			/*
			 * If there is no room at all for the detail
			 * label, just null it out.
			 * Note that if the main label is too long,
			 * we're not handling that here.  This is just to
			 * trim the detail label, since that is usually a
			 * file name or package name that has gotten out
			 * of control, whereas the main label should be
			 * some constant string defined or provided by
			 * L10N that should be a reasonable length to
			 * begin with.
			 */
			detail_label[0] = '\0';
		} else {
			/*
			 * Cut off the detail label and append "..."
			 * so they know there's more to it.
			 */
			(void) strcpy(&detail_label[detail_len],
				APP_UI_UPG_PROGRESS_CUT_STR);
			detail_label[detail_len +
				strlen(APP_UI_UPG_PROGRESS_CUT_STR)] = '\0';
		}
	}

	write_debug(APP_DEBUG_L1, "Trimmed detail label: %s\n", detail_label);

}
