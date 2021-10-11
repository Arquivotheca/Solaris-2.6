#ifndef lint
#pragma ident "@(#)soft_progress.c 1.6 96/09/06 SMI"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */
#include "spmisoft_lib.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FIND_MODIFIED_LINES_PER_TIC	15
#define LINES_PER_CONTENTS_FILE_TIC	300
#define PKGMAP_PROGRESS_UNIT		4000

struct s_progress_items {
	int find_mod_lines;
	int progress_dir_du;
	int contents_lines;
	ulong pkgmap_bytes;
};

struct s_progress_items s_total_items;
struct s_progress_items s_current_items;

/* data that is global within this file */

/* binary value indicating whether in progress-counting mode */
static int	s_action_count_mode = 0;
static int	s_progress_meter_mode = 0;

/* total actions to be performed in validation */
static ulong	s_total_actions;

/* current number of actions performed */
static ulong	s_current_actions;

/* callback function and callback argument */
int (*s_callback_proc)(void *, void *);
void *s_callback_arg;

/* Library function prototypes */
void	ProgressBeginActionCount(void);
int	ProgressInCountMode(void);
void	ProgressCountActions(ProgressActionType, ulong);
void	ProgressBeginMetering(int (*)(void *, void*), void *);
void	ProgressEndMetering(void);
void	ProgressAdvance(ProgressActionType, ulong, ValStage, char *);

/* Local Function Prototypes */
static ulong	get_progress_count_total(void);

/* ******************************************************************** */
/* 			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * ProgressBeginActionCount()
 *	Begin the process of counting the total number of
 *	actions to be performed.  This total is required in order
 *	to show a "percent complete" figure in the progress displays.
 *	This function clears all of the action counts, and puts
 *	the system in "action counting mode".  "Action counting mode"
 *	is assumed to be complete when "progress_metering_mode" is
 *	started (by a call to ProgresBeginMetering()).
 *
 * Parameters:
 * Returns: none
 * Status: library-public
 */
void
ProgressBeginActionCount(void)
{
	s_action_count_mode = 1;
	s_progress_meter_mode =  0;
	s_total_items.find_mod_lines = 0;
	s_total_items.progress_dir_du = 0;
	s_total_items.contents_lines = 0;
	s_total_items.pkgmap_bytes = 0;
}

/*
 * ProgressInCountMode()
 *	Determine whether in action-count mode.
 *
 * Parameters:
 * Returns: non-zero if in action-count mode, else returns 0.
 * Status: library-public
 */
int
ProgressInCountMode(void)
{
	return (s_action_count_mode);
}

/*
 * ProgressCountActions()
 *	Count a particular type of action to be performed in
 * 	the progress-metering stage.
 * Parameters:
 *	action_type - type of action being counted.
 *	count - number of actions being counted (number will mean
 *		different things, based on the value of action_type).
 * Returns: non-zero if in action-count mode, else returns 0.
 * Status: library-public
 */
void
ProgressCountActions(ProgressActionType action_type, ulong count)
{
	if (!ProgressInCountMode())
		return;

	switch(action_type) {
	case PROG_FIND_MODIFIED:
		s_total_items.find_mod_lines += count;
		break;
	case PROG_DIR_DU:
		s_total_items.progress_dir_du += count;
		break;
	case PROG_CONTENTS_LINES:
		s_total_items.contents_lines += count;
		break;
	case PROG_PKGMAP_SIZE:
		s_total_items.pkgmap_bytes += count;
		break;
	}
}

/*
 * ProgressBeginMetering()
 *	Begin the process of metering progress.  ActionCodeMode
 *	is now false.
 * Parameters:
 *	callback_proc - function to be called at the appropriate
 *		intervals.
 *	arg	- argument to be passed to the callback function.
 * Returns: 
 * Status: library-public
 */
void
ProgressBeginMetering(int (*callback_proc)(void *, void*), void *arg)
{
	s_callback_proc = callback_proc;
	s_callback_arg = arg;
	s_current_items.find_mod_lines = 0;
	s_current_items.progress_dir_du = 0;
	s_current_items.contents_lines = 0;
	s_current_items.pkgmap_bytes = 0;
	s_total_actions = get_progress_count_total();
	s_current_actions = 0;
	s_action_count_mode = 0;
	s_progress_meter_mode = 1;
}

/*
 * ProgressEndMetering()
 *	Begin the process of metering progress.  ActionCodeMode
 *	is now false.
 * Parameters:
 *	callback_proc - function to be called at the appropriate
 *		intervals.
 *	arg	- argument to be passed to the callback function.
 * Returns: 
 * Status: library-public
 */
void
ProgressEndMetering(void)
{
	s_progress_meter_mode = 0;
	s_callback_proc = NULL;
	s_callback_arg = 0;
}

/*
 * ProgressAdvance()
 *	Record an advance in progress.  The function records the
 *	progress and calls the callback function if a "unit"
 *	of progress has occurred.
 * Parameters:
 *	action_type - Type of action that occurred.
 *	arg	- count of actions that occurred (count will mean
 *		different things, depending on the action_type).
 * Returns: 
 * Status: library-public
 */
void
ProgressAdvance(ProgressActionType action_type, ulong count, ValStage stage,
   char *detail)
{
	int	n;
	int	steps = 0;
	static	int prior_pkgmap_units = 0;
	ValProgress ValP;

	if (!s_progress_meter_mode || s_callback_proc == NULL)
		return;

	switch(action_type) {
	case PROG_BEGIN:
	case PROG_END:
		steps = 0;
		break;
	case PROG_DIR_DU:
		s_current_items.progress_dir_du += count;
		steps = count;
		break;
	case PROG_FIND_MODIFIED:
		for (n = 0; n < count; n++)
			if ((++s_current_items.find_mod_lines % 
			    FIND_MODIFIED_LINES_PER_TIC) == 0)
				steps++;
		break;
	case PROG_CONTENTS_LINES:
		for (n = 0; n < count; n++)
			if ((++s_current_items.contents_lines % 
			    LINES_PER_CONTENTS_FILE_TIC) == 0)
				steps++;
		break;
	case PROG_PKGMAP_SIZE:
		s_current_items.pkgmap_bytes += count;
		n = s_current_items.pkgmap_bytes /
			PKGMAP_PROGRESS_UNIT;
		if (n > prior_pkgmap_units) {
			steps = n - prior_pkgmap_units;
			prior_pkgmap_units = n;
		}
		break;
	}
	
	ValP.valp_detail = NULL;
	if (steps != 0) {
		s_current_actions += steps;
		ValP.valp_percent_done = (int)
		    (((float)s_current_actions/(float)s_total_actions) * 100);
		ValP.valp_stage = stage;
		if (detail)
			ValP.valp_detail = xstrdup(detail);
		(void) s_callback_proc(s_callback_arg, (void *)&ValP);
	} else if (action_type == PROG_BEGIN || action_type == PROG_END) {
		ValP.valp_stage = stage;
		ValP.valp_percent_done = 
			(action_type == PROG_BEGIN) ? 0 : 100;
		(void) s_callback_proc(s_callback_arg, (void *)&ValP);
	}
}

/* ******************************************************************** */
/* 			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

static ulong
get_progress_count_total(void)
{
	return (s_total_items.progress_dir_du +
	    (int)(s_total_items.find_mod_lines / FIND_MODIFIED_LINES_PER_TIC) +
	    (int)(s_total_items.contents_lines / LINES_PER_CONTENTS_FILE_TIC) +
	    (int)(s_total_items.pkgmap_bytes/PKGMAP_PROGRESS_UNIT));
}
