/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)jobsched_iface.c	1.4	96/09/09 SMI"


#include <string.h>
#include <sys/types.h>
#include "sysman_iface.h"
#include "sysman_impl.h"


int
sysman_add_jobsched(SysmanJobschedArg *ja_p, char *buf, int bufsiz)
{

	SysmanSharedJobschedArg	sja;
	int			status;


	if (ja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,16, "add scheduled job failed"));

	if (ja_p->schedule_as_root == B_TRUE && getuid() != 0) {

		memset((void *)&sja, 0, sizeof (sja));

		/* copy in fields */
		
		strcpy(sja.job, ja_p->job);
		sja.schedule_as_root = ja_p->schedule_as_root;
		sja.year = ja_p->year;
		cp_time_a_from_l(sja.month,
		    sizeof (sja.month) / sizeof (sja.month[0]),
		    ja_p->month);
		cp_time_a_from_l(sja.date,
		    sizeof (sja.date) / sizeof (sja.date[0]),
		    ja_p->date);
		cp_time_a_from_l(sja.weekday,
		    sizeof (sja.weekday) / sizeof (sja.weekday[0]),
		    ja_p->weekday);
		cp_time_a_from_l(sja.hour,
		    sizeof (sja.hour) / sizeof (sja.hour[0]),
		    ja_p->hour);
		cp_time_a_from_l(sja.minute,
		    sizeof (sja.minute) / sizeof (sja.minute[0]),
		    ja_p->minute);
		sja.frequency = ja_p->frequency;

		status = call_function_as_admin(_root_add_jobsched,
		    (void *)&sja, sizeof (SysmanSharedJobschedArg),
		    buf, bufsiz);

		if (status == SYSMAN_SUCCESS && sja.job_id_return[0] != '\0') {
			/* A job id was returned */
			ja_p->job_id_return = strdup(sja.job_id_return);
		} else {
			ja_p->job_id_return = NULL;
		}
	} else {
		status = _add_jobsched(ja_p, buf, bufsiz);
	}

	return (status);
}


int
sysman_delete_jobsched(SysmanJobschedArg *ja_p, char *buf, int bufsiz)
{

	int	status;


	if (ja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,17, "delete scheduled job failed"));

	if (ja_p->schedule_as_root_key == B_TRUE && getuid() != 0) {
		status = call_function_as_admin(_root_delete_jobsched,
		    (void *)ja_p, sizeof (SysmanJobschedArg), buf, bufsiz);
	} else {
		status = _delete_jobsched(ja_p, buf, bufsiz);
	}

	return (status);
}


int
sysman_modify_jobsched(SysmanJobschedArg *ja_p, char *buf, int bufsiz)
{

	SysmanSharedJobschedArg	sja;
	int			status;


	if (ja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	if (ja_p->schedule_as_root_key != ja_p->schedule_as_root) {
		/*
		 * can't modify across users; that is, you can't use modify
		 * to delete from root's crontab and add to the user's
		 * crontab (or the other way, from user to root).
		 */
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,18, "modify scheduled job failed"));

	if (ja_p->schedule_as_root == B_TRUE && getuid() != 0) {

		memset((void *)&sja, 0, sizeof (sja));

		/* copy in fields */
		
		strcpy(sja.job_id_key, ja_p->job_id_key);
		strcpy(sja.job_key, ja_p->job_key);
		sja.schedule_as_root_key = ja_p->schedule_as_root_key;
		sja.year_key = ja_p->year_key;
		cp_time_a_from_l(sja.month_key,
		    sizeof (sja.month_key) / sizeof (sja.month_key[0]),
		    ja_p->month_key);
		cp_time_a_from_l(sja.date_key,
		    sizeof (sja.date_key) / sizeof (sja.date_key[0]),
		    ja_p->date_key);
		cp_time_a_from_l(sja.weekday_key,
		    sizeof (sja.weekday_key) / sizeof (sja.weekday_key[0]),
		    ja_p->weekday_key);
		cp_time_a_from_l(sja.hour_key,
		    sizeof (sja.hour_key) / sizeof (sja.hour_key[0]),
		    ja_p->hour_key);
		cp_time_a_from_l(sja.minute_key,
		    sizeof (sja.minute_key) / sizeof (sja.minute_key[0]),
		    ja_p->minute_key);
		sja.frequency_key = ja_p->frequency_key;

		strcpy(sja.job, ja_p->job);
		sja.schedule_as_root = ja_p->schedule_as_root;
		sja.year = ja_p->year;
		cp_time_a_from_l(sja.month,
		    sizeof (sja.month) / sizeof (sja.month[0]),
		    ja_p->month);
		cp_time_a_from_l(sja.date,
		    sizeof (sja.date) / sizeof (sja.date[0]),
		    ja_p->date);
		cp_time_a_from_l(sja.weekday,
		    sizeof (sja.weekday) / sizeof (sja.weekday[0]),
		    ja_p->weekday);
		cp_time_a_from_l(sja.hour,
		    sizeof (sja.hour) / sizeof (sja.hour[0]),
		    ja_p->hour);
		cp_time_a_from_l(sja.minute,
		    sizeof (sja.minute) / sizeof (sja.minute[0]),
		    ja_p->minute);
		sja.frequency = ja_p->frequency;

		status = call_function_as_admin(_root_modify_jobsched,
		    (void *)&sja, sizeof (SysmanSharedJobschedArg),
		    buf, bufsiz);

		if (status == SYSMAN_SUCCESS && sja.job_id_return[0] != '\0') {
			/* A job id was returned */
			ja_p->job_id_return = strdup(sja.job_id_return);
		} else {
			ja_p->job_id_return = NULL;
		}
	} else {
		status = _modify_jobsched(ja_p, buf, bufsiz);
	}

	return (status);
}


int
sysman_get_jobsched(SysmanJobschedArg *ja_p, char *buf, int bufsiz)
{

	SysmanSharedJobschedArg	sja;
	int			status;


	if (ja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,19, "get scheduled job failed"));

	if (ja_p->schedule_as_root_key == B_TRUE && getuid() != 0) {

		memset((void *)&sja, 0, sizeof (sja));

		/* copy in key fields */

		if (ja_p->job_id_key != NULL) {
			strcpy(sja.job_id_key, ja_p->job_id_key);
		} else {
			sja.job_id_key[0] = '\0';
		}
		strcpy(sja.job_key, ja_p->job_key);
		sja.schedule_as_root_key = ja_p->schedule_as_root_key;
		sja.year_key = ja_p->year_key;
		cp_time_a_from_l(sja.month_key,
		    sizeof (sja.month_key) / sizeof (sja.month_key[0]),
		    ja_p->month_key);
		cp_time_a_from_l(sja.date_key,
		    sizeof (sja.date_key) / sizeof (sja.date_key[0]),
		    ja_p->date_key);
		cp_time_a_from_l(sja.weekday_key,
		    sizeof (sja.weekday_key) / sizeof (sja.weekday_key[0]),
		    ja_p->weekday_key);
		cp_time_a_from_l(sja.hour_key,
		    sizeof (sja.hour_key) / sizeof (sja.hour_key[0]),
		    ja_p->hour_key);
		cp_time_a_from_l(sja.minute_key,
		    sizeof (sja.minute_key) / sizeof (sja.minute_key[0]),
		    ja_p->minute_key);
		sja.frequency_key = ja_p->frequency_key;
	
		status = call_function_as_admin(_root_get_jobsched,
		    (void *)&sja, sizeof (SysmanSharedJobschedArg),
		    buf, bufsiz);

		if (status == SYSMAN_SUCCESS) {
			ja_p->job_id_return = sja.job_id_return[0] ?
			    strdup(sja.job_id_return) : NULL;
			ja_p->job = sja.job[0] ? strdup(sja.job) : NULL;
			ja_p->year = sja.year;
			ja_p->month = mk_time_l_from_a(sja.month);
			ja_p->date = mk_time_l_from_a(sja.date);
			ja_p->weekday = mk_time_l_from_a(sja.weekday);
			ja_p->hour = mk_time_l_from_a(sja.hour);
			ja_p->minute = mk_time_l_from_a(sja.minute);
			ja_p->frequency = sja.frequency;
		}
	} else {
		status = _get_jobsched(ja_p, buf, bufsiz);
	}

	return (status);
}


void
sysman_free_jobsched(SysmanJobschedArg *ja_p)
{
	_free_jobsched(ja_p);
}


int
sysman_list_user_jobsched(SysmanJobschedArg **ja_p, char *buf, int bufsiz)
{

	int	status;


	if (ja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,20, "list scheduled jobs failed"));

	status = _list_jobsched(ja_p, buf, bufsiz);

	return (status);
}


int
sysman_list_root_jobsched(SysmanJobschedArg **ja_p, char *buf, int bufsiz)
{

	SysmanSharedJobschedArg	sjaa[MAX_JOBSCHED_LIST];
	int			i;
	int			cnt;


	if (ja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	open_sysman_cat();
	init_err_msg(buf, bufsiz, catgets(_catlibsysman,7,21, "list scheduled jobs failed"));

	cnt = call_function_as_admin(_root_list_jobsched,
	    (void *)sjaa, sizeof (sjaa), buf, bufsiz);

	if (cnt > 0) {
		*ja_p = (SysmanJobschedArg *)malloc(cnt *
		    sizeof (SysmanJobschedArg));

		if (*ja_p == NULL) {
			return (SYSMAN_MALLOC_ERR);
		}

		for (i = 0; i < cnt; i++) {
			memset((void *)(&(*ja_p)[i]), 0,
			    sizeof (SysmanJobschedArg));

			if (sjaa[i].job_id_return[0] != '\0') {
				(*ja_p)[i].job_id_return =
				    strdup(sjaa[i].job_id_return);
			} else {
				(*ja_p)[i].job_id_return = NULL;
			}
			(*ja_p)[i].job = strdup(sjaa[i].job);
			(*ja_p)[i].year = sjaa[i].year;

			(*ja_p)[i].month = mk_time_l_from_a(sjaa[i].month);
			(*ja_p)[i].date = mk_time_l_from_a(sjaa[i].date);
			(*ja_p)[i].weekday = mk_time_l_from_a(sjaa[i].weekday);
			(*ja_p)[i].hour = mk_time_l_from_a(sjaa[i].hour);
			(*ja_p)[i].minute = mk_time_l_from_a(sjaa[i].minute);

			(*ja_p)[i].frequency = sjaa[i].frequency;
		}
	}

	return (cnt);
}


void
sysman_free_jobsched_list(SysmanJobschedArg *ja_p, int cnt)
{
	_free_jobsched_list(ja_p, cnt);
}
