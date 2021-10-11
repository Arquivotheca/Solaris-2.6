/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)jobsched_modify.c	1.5	95/06/27 SMI"


#include <stdlib.h>
#include <stddef.h>
#include "jobsched_iface.h"


int
jobsched_modify_at(JSAtArg *ap, char **ret_job_id)
{

	int	status;
	char	*job_id;
	JSAtArg	del;


	if (ap == NULL || ap->at_job_id_key == NULL || ap->job == NULL) {
		return (JOBSCHED_BAD_INPUT);
	}

	/*
	 * Modify is implemented by adding the new job and then deleting
	 * the old one.  If the add of the new fails, modify fails.  If
	 * the add of the new succeeds but delete of the old fails, we
	 * need to delete the one that we added and fail.
	 */

	status = jobsched_add_at(ap, &job_id);

	if (status != JOBSCHED_SUCCESS) {
		free((void *)job_id);
		return (status);
	}

	status = jobsched_delete_at(ap);

	if (status != JOBSCHED_SUCCESS) {

		del.at_job_id_key = job_id;

		if (jobsched_delete_at(&del) != JOBSCHED_SUCCESS) {
			/* really hosed; added a new job that we can't delete */
			free((void *)job_id);
			return (JOBSCHED_FAILED_DIRTY);
		}
	}

	if (ret_job_id != NULL) {
		*ret_job_id = job_id;
	} else {
		free((void *)job_id);
	}

	return (JOBSCHED_SUCCESS);
}


int
jobsched_modify_cron(JSCronArg *cp)
{

	int		status;
	JSCronArg	del;


	if (cp == NULL || cp->job == NULL ||
	    cp->minute == NULL || cp->hour == NULL || cp->month == NULL ||
	    cp->day_of_week == NULL ||
	    cp->job_key == NULL ||
	    cp->minute_key == NULL || cp->hour_key == NULL ||
	    cp->month_key == NULL || cp->day_of_week_key == NULL) {

		return (JOBSCHED_BAD_INPUT);
	}

	/*
	 * Modify is implemented by adding the new job and then deleting
	 * the old one.  If the add of the new fails, modify fails.  If
	 * the add of the new succeeds but delete of the old fails, we
	 * need to delete the one that we added and fail.
	 */

	status = jobsched_add_cron(cp);

	if (status != JOBSCHED_SUCCESS) {
		return (status);
	}

	status = jobsched_delete_cron(cp);

	if (status != JOBSCHED_SUCCESS) {

		del.job_key = cp->job;
		del.username_key = cp->username;
		del.minute_key = cp->minute;
		del.hour_key = cp->hour;
		del.month_key = cp->month;
		del.day_of_week_key = cp->day_of_week;

		if (jobsched_delete_cron(&del) != JOBSCHED_SUCCESS) {
			/* really hosed; added a new job that we can't delete */
			return (JOBSCHED_FAILED_DIRTY);
		}

		return (status);
	}

	return (JOBSCHED_SUCCESS);
}
