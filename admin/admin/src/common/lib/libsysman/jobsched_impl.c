/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)jobsched_impl.c	1.4	95/08/29 SMI"


#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <pwd.h>
#include "sysman_impl.h"
#include "jobsched_iface.h"


static
int
jobsched_code_to_sysman_code(int status)
{

	int	s;


	switch (status) {
	case JOBSCHED_SUCCESS:
		s = SYSMAN_SUCCESS;
		break;
	case JOBSCHED_FAILURE:
		s = SYSMAN_JOBSCHED_FAILED;
		break;
	case JOBSCHED_FAILED_DIRTY:
		s = SYSMAN_JOBSCHED_FAILED;
		break;
	case JOBSCHED_BAD_INPUT:
		s = SYSMAN_BAD_INPUT;
		break;
	case JOBSCHED_MALLOC_ERR:
		s = SYSMAN_MALLOC_ERR;
		break;
	case JOBSCHED_PERM_DENIED:
		s = SYSMAN_JOBSCHED_PERM_DENIED;
		break;
	case JOBSCHED_SCHEDULING_IN_THE_PAST:
		s = SYSMAN_JOBSCHED_FAILED;
		break;
	case JOBSCHED_NOT_FOUND:
		s = SYSMAN_JOBSCHED_FAILED;
		break;
	default:
		s = SYSMAN_JOBSCHED_FAILED;
		break;
	}

	return (s);
}


static
j_frequency_t
interpret_job_frequency(const SysmanJobschedArg *j)
{

	if (j->minute->type_tag != j_atom || j->minute->next != NULL) {
		/* we only handle atomic, non-list minute fields */
		return (j_freq_other);
	}

	/*
	 * only ranges that we have a frequency type for are some
	 * weekday ranges; any other ranges are type "other"
	 */

	if (j->minute->type_tag == j_range || j->hour->type_tag == j_range ||
	    j->date->type_tag == j_range || j->month->type_tag == j_range) {
		return (j_freq_other);
	}

	/*
	 * only lists that we have a frequency type for are some
	 * weekday lists; any other lists are type "other"
	 */

	if (j->minute->next != NULL || j->hour->next != NULL ||
	    j->date->next != NULL || j->month->next != NULL) {
		return (j_freq_other);
	}

	if (j->weekday->type_tag == j_wildcard) {
		if (j->month->type_tag == j_wildcard) {
			if (j->date->type_tag == j_wildcard) {
				if (j->hour->type_tag == j_wildcard) {
					return (j_hourly);
				} else {
					return (j_daily);
				}
			} else {
				if (j->hour->type_tag != j_wildcard) {
					return (j_monthly);
				}
			}
		} else {
			if (j->date->type_tag != j_wildcard &&
			    j->hour->type_tag != j_wildcard) {
				return (j_yearly);
			}
		}
	} else {
		/* is weekday a list? */
		if (j->weekday->next != NULL) {
			/* list, we only handle 1,3,5 and 2,4 */
			if (j->weekday->next->next == NULL) {
				/* two elts in list, might be 2,4 */
				if (j->weekday->type_tag == j_atom &&
				    j->weekday->next->type_tag == j_atom &&
				    j->weekday->value.atom == 2 &&
				    j->weekday->next->value.atom == 4) {
					return (j_tue_thu);
				}
			} else if (j->weekday->next->next->next == NULL) {
				/* three elts in list, might be 1,3,5 */
				if (j->weekday->type_tag == j_atom &&
				    j->weekday->next->type_tag == j_atom &&
				    j->weekday->next->next->type_tag ==
				    j_atom &&
				    j->weekday->value.atom == 1 &&
				    j->weekday->next->value.atom == 3 &&
				    j->weekday->next->next->value.atom == 5) {
					return (j_mon_wed_fri);
				}
			} else {
				return (j_freq_other);
			}
		} else {
			if (j->weekday->type_tag == j_atom) {
				if (j->month->type_tag == j_wildcard &&
				    j->date->type_tag == j_wildcard &&
				    j->hour->type_tag == j_atom) {
					return (j_weekly);
				}
			} else if (j->weekday->type_tag == j_range) {
				if (j->month->type_tag == j_wildcard &&
				    j->date->type_tag == j_wildcard &&
				    j->weekday->value.range.range_low == 1 &&
				    j->weekday->value.range.range_high == 5) {
					return (j_mon_thru_fri);
				}
			}
		}
	}

	return (j_freq_other);
}


static
int
alloc_init_cron(j_frequency_t freq, const SysmanJobschedArg *j, JSCronArg *c)
{

	char	buf[32];


	if (j == NULL || c == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	c->minute = cron_list_to_field_string(j->minute);

	switch (freq) {
	case j_hourly:
		/* make it happen every hour of every day of every month ... */
		c->hour = strdup("*");
		c->day = strdup("*");
		c->month = strdup("*");
		c->day_of_week = strdup("*");
		break;
	case j_daily:
		/* wildcard the day, month, and day of week */
		c->hour = cron_list_to_field_string(j->hour);
		c->day = strdup("*");
		c->month = strdup("*");
		c->day_of_week = strdup("*");
		break;
	case j_weekly:
		/* day of week controls weekly, so wildcard day and month */
		c->hour = cron_list_to_field_string(j->hour);
		c->day = strdup("*");
		c->month = strdup("*");
		c->day_of_week = cron_list_to_field_string(j->weekday);
		break;
	case j_monthly:
		c->hour = cron_list_to_field_string(j->hour);
		c->day = cron_list_to_field_string(j->date);
		c->month = strdup("*");
		c->day_of_week = strdup("*");
		break;
	case j_yearly:
		c->hour = cron_list_to_field_string(j->hour);
		c->day = cron_list_to_field_string(j->date);
		c->month = cron_list_to_field_string(j->month);
		c->day_of_week = strdup("*");
		break;
	case j_mon_thru_fri:
		c->hour = cron_list_to_field_string(j->hour);
		c->day = strdup("*");
		c->month = strdup("*");
		c->day_of_week = strdup("1-5");
		break;
	case j_mon_wed_fri:
		c->hour = cron_list_to_field_string(j->hour);
		c->day = strdup("*");
		c->month = strdup("*");
		c->day_of_week = strdup("1,3,5");
		break;
	case j_tue_thu:
		c->hour = cron_list_to_field_string(j->hour);
		c->day = strdup("*");
		c->month = strdup("*");
		c->day_of_week = strdup("2,4");
		break;
	default:
		return (SYSMAN_BAD_INPUT);
	}

	return (SYSMAN_SUCCESS);
}


static
int
alloc_init_ckey(j_frequency_t freq, const SysmanJobschedArg *j, JSCronArg *c)
{

	char	buf[32];


	if (j == NULL || c == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	c->minute_key = cron_list_to_field_string(j->minute_key);

	switch (freq) {
	case j_hourly:
		/* make it happen every hour of every day of every month ... */
		c->hour_key = strdup("*");
		c->day_key = strdup("*");
		c->month_key = strdup("*");
		c->day_of_week_key = strdup("*");
		break;
	case j_daily:
		/* wildcard the day, month, and day of week */
		c->hour_key = cron_list_to_field_string(j->hour_key);
		c->day_key = strdup("*");
		c->month_key = strdup("*");
		c->day_of_week_key = strdup("*");
		break;
	case j_weekly:
		/* day of week controls weekly, so wildcard day and month */
		c->hour_key = cron_list_to_field_string(j->hour_key);
		c->day_key = strdup("*");
		c->month_key = strdup("*");
		c->day_of_week_key =
		    cron_list_to_field_string(j->weekday_key);
		break;
	case j_monthly:
		c->hour_key = cron_list_to_field_string(j->hour_key);
		c->day_key = cron_list_to_field_string(j->date_key);
		c->month_key = strdup("*");
		c->day_of_week_key = strdup("*");
		break;
	case j_yearly:
		c->hour_key = cron_list_to_field_string(j->hour_key);
		c->day_key = cron_list_to_field_string(j->date_key);
		c->month_key = cron_list_to_field_string(j->month_key);
		c->day_of_week_key = strdup("*");
		break;
	case j_mon_thru_fri:
		c->hour_key = cron_list_to_field_string(j->hour_key);
		c->day_key = strdup("*");
		c->month_key = strdup("*");
		c->day_of_week_key = strdup("1-5");
		break;
	case j_mon_wed_fri:
		c->hour_key = cron_list_to_field_string(j->hour_key);
		c->day_key = strdup("*");
		c->month_key = strdup("*");
		c->day_of_week_key = strdup("1,3,5");
		break;
	case j_tue_thu:
		c->hour_key = cron_list_to_field_string(j->hour_key);
		c->day_key = strdup("*");
		c->month_key = strdup("*");
		c->day_of_week_key = strdup("2,4");
		break;
	default:
		return (SYSMAN_BAD_INPUT);
	}

	return (SYSMAN_SUCCESS);
}


static
void
free_cron(JSCronArg *c)
{
	if (c != NULL) {
		if (c->minute_key != NULL) {
			free((void *)c->minute_key);
		}
		if (c->hour_key != NULL) {
			free((void *)c->hour_key);
		}
		if (c->day_key != NULL) {
			free((void *)c->day_key);
		}
		if (c->month_key != NULL) {
			free((void *)c->month_key);
		}
		if (c->day_of_week_key != NULL) {
			free((void *)c->day_of_week_key);
		}

		if (c->minute != NULL) {
			free((void *)c->minute);
		}
		if (c->hour != NULL) {
			free((void *)c->hour);
		}
		if (c->day != NULL) {
			free((void *)c->day);
		}
		if (c->month != NULL) {
			free((void *)c->month);
		}
		if (c->day_of_week != NULL) {
			free((void *)c->day_of_week);
		}
	}
}


int
_add_jobsched(SysmanJobschedArg *ja_p, char *buf, int len)
{

	int		status;
	char		*at_id;
	JSAtArg		at;
	JSCronArg	cron;


	if (ja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	if (ja_p->frequency == j_once) {

		/* schedule via "at" */

		if (ja_p->month == NULL || ja_p->date == NULL ||
		    ja_p->hour == NULL || ja_p->minute == NULL) {
			return (SYSMAN_BAD_INPUT);
		}

		at.job = (char *)ja_p->job;
		at.job_is_filename_p = B_FALSE;
		at.shell = js_sh;
		at.send_at_mail_p = B_FALSE;
		at.year = ja_p->year;
		at.month = ja_p->month->value.atom;
		at.day_of_month = ja_p->date->value.atom;
		at.hour = ja_p->hour->value.atom;
		at.minute = ja_p->minute->value.atom;
		at.sec = 0;

		status = jobsched_add_at(&at, &at_id);

		if (status == JOBSCHED_SUCCESS) {
			ja_p->job_id_return = strdup(at_id);
		} else {
			if ((status = jobsched_code_to_sysman_code(status)) ==
			    SYSMAN_JOBSCHED_FAILED) {
				status = SYSMAN_JOBSCHED_ADD_FAILED;
			}
		}

	} else {

		/* schedule via "crontab" */

		memset((void *)&cron, 0, sizeof (JSCronArg));

		if ((status = alloc_init_cron(ja_p->frequency, ja_p, &cron)) ==
		    SYSMAN_SUCCESS) {

			cron.username = NULL;
			cron.job = (char *)ja_p->job;

			status = jobsched_add_cron(&cron);

			if ((status = jobsched_code_to_sysman_code(status)) ==
			    SYSMAN_JOBSCHED_FAILED) {
				status = SYSMAN_JOBSCHED_ADD_FAILED;
			}

			(void) free_cron(&cron);
		}
	}

	return (status);
}


int
_root_add_jobsched(void *arg_p, char *buf, int len)
{

	SysmanSharedJobschedArg	*sja_p = (SysmanSharedJobschedArg *)arg_p;
	SysmanJobschedArg	ja;
	int			s;


	if (sja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	memset((void *)&ja, 0, sizeof (ja));

	ja.job = sja_p->job;
	ja.schedule_as_root = sja_p->schedule_as_root;
	ja.year = sja_p->year;
	ja.month = sja_p->month;
	ja.date = sja_p->date;
	ja.weekday = sja_p->weekday;
	ja.hour = sja_p->hour;
	ja.minute = sja_p->minute;
	ja.frequency = sja_p->frequency;

	s = _add_jobsched(&ja, buf, len);

	if (s == SYSMAN_SUCCESS) {
		if (ja.job_id_return != NULL) {
			(void) strcpy(sja_p->job_id_return, ja.job_id_return);
			free((void *)ja.job_id_return);
		} else {
			sja_p->job_id_return[0] = '\0';
		}
	}

	return (s);
}


int
_delete_jobsched(SysmanJobschedArg *ja_p, char *buf, int len)
{

	int		status;
	JSAtArg		at;
	JSCronArg	cron;


	if (ja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	if (ja_p->frequency == j_once) {

		/* remove from "at" queue */

		at.at_job_id_key = ja_p->job_id_key;

		status = jobsched_delete_at(&at);

		if ((status = jobsched_code_to_sysman_code(status)) ==
		    SYSMAN_JOBSCHED_FAILED) {
			status = SYSMAN_JOBSCHED_DEL_FAILED;
		}
	} else {

		/* remove from "crontab" */

		memset((void *)&cron, 0, sizeof (JSCronArg));

		if ((status = alloc_init_ckey(ja_p->frequency, ja_p, &cron)) ==
		    SYSMAN_SUCCESS) {

			cron.username_key = NULL;
			cron.job_key = ja_p->job_key;

			status = jobsched_delete_cron(&cron);

			if ((status = jobsched_code_to_sysman_code(status)) ==
			    SYSMAN_JOBSCHED_FAILED) {
				status = SYSMAN_JOBSCHED_DEL_FAILED;
			}

			(void) free_cron(&cron);
		}
	}

	return (status);
}


int
_root_delete_jobsched(void *arg_p, char *buf, int len)
{

	SysmanJobschedArg	*ja_p = (SysmanJobschedArg *)arg_p;


	if (ja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	return (_delete_jobsched(ja_p, buf, len));
}


int
_modify_jobsched(SysmanJobschedArg *ja_p, char *buf, int len)
{

	int		status;
	char		*at_id;
	JSAtArg		at;
	JSCronArg	cron;


	if (ja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	if (ja_p->frequency == j_once) {

		/* schedule via "at" */

		if (ja_p->month == NULL || ja_p->date == NULL ||
		    ja_p->hour == NULL || ja_p->minute == NULL) {
			return (SYSMAN_BAD_INPUT);
		}

		/* set up key */

		at.at_job_id_key = ja_p->job_id_key;

		/* set up new values */

		at.job = (char *)ja_p->job;
		at.job_is_filename_p = B_FALSE;
		at.shell = js_sh;
		at.send_at_mail_p = B_FALSE;
		at.year = ja_p->year;
		at.month = ja_p->month->value.atom;
		at.day_of_month = ja_p->date->value.atom;
		at.hour = ja_p->hour->value.atom;
		at.minute = ja_p->minute->value.atom;
		at.sec = 0;

		status = jobsched_modify_at(&at, &at_id);

		if (status == JOBSCHED_SUCCESS) {
			ja_p->job_id_return = strdup(at_id);
		} else {
			if ((status = jobsched_code_to_sysman_code(status)) ==
			    SYSMAN_JOBSCHED_FAILED) {
				status = SYSMAN_JOBSCHED_MOD_FAILED;
			}
		}
	} else {

		/* schedule via "crontab" */

		/* set up keys */

		memset((void *)&cron, 0, sizeof (JSCronArg));

		if ((status = alloc_init_ckey(ja_p->frequency, ja_p, &cron)) !=
		    SYSMAN_SUCCESS) {

			return (status);
		}

		cron.username_key = NULL;
		cron.job_key = ja_p->job_key;

		/* set up new values */

		if ((status = alloc_init_cron(ja_p->frequency, ja_p, &cron)) !=
		    SYSMAN_SUCCESS) {

			/* clean up before return */

			(void) free_cron(&cron);

			return (status);
		}

		cron.username = NULL;
		cron.job = (char *)ja_p->job;

		status = jobsched_modify_cron(&cron);

		if ((status = jobsched_code_to_sysman_code(status)) ==
		    SYSMAN_JOBSCHED_FAILED) {
			status = SYSMAN_JOBSCHED_MOD_FAILED;
		}

		(void) free_cron(&cron);
	}

	return (status);
}


int
_root_modify_jobsched(void *arg_p, char *buf, int len)
{

	SysmanSharedJobschedArg	*sja_p = (SysmanSharedJobschedArg *)arg_p;
	SysmanJobschedArg	ja;
	int			s;


	if (sja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	memset((void *)&ja, 0, sizeof (ja));

	ja.job_id_key = sja_p->job_id_key;
	ja.job_key = sja_p->job_key;
	ja.schedule_as_root_key = sja_p->schedule_as_root_key;
	ja.year_key = sja_p->year_key;
	ja.month_key = sja_p->month_key;
	ja.date_key = sja_p->date_key;
	ja.weekday_key = sja_p->weekday_key;
	ja.hour_key = sja_p->hour_key;
	ja.minute_key = sja_p->minute_key;
	ja.frequency_key = sja_p->frequency_key;

	ja.job = sja_p->job;
	ja.schedule_as_root = sja_p->schedule_as_root;
	ja.year = sja_p->year;
	ja.month = sja_p->month;
	ja.date = sja_p->date;
	ja.weekday = sja_p->weekday;
	ja.hour = sja_p->hour;
	ja.minute = sja_p->minute;
	ja.frequency = sja_p->frequency;

	s = _modify_jobsched(&ja, buf, len);

	if (s == SYSMAN_SUCCESS) {
		if (ja.job_id_return != NULL) {
			(void) strcpy(sja_p->job_id_return, ja.job_id_return);
			free((void *)ja.job_id_return);
		} else {
			sja_p->job_id_return[0] = '\0';
		}
	}

	return (s);
}


int
_get_jobsched(SysmanJobschedArg *ja_p, char *buf, int len)
{

	int		s;
	JSAtArg		at;
	JSCronArg	cron;


	if (ja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	if (ja_p->frequency_key == j_once) {

		memset((void *)&at, 0, sizeof (JSAtArg));

		at.at_job_id_key = ja_p->job_id_key;

		s = jobsched_get_at(&at);

		if (s != JOBSCHED_SUCCESS) {
			return (SYSMAN_JOBSCHED_GET_FAILED);
		}

		ja_p->job_id_return = strdup(at.at_job_id);
		ja_p->job = strdup(at.job);

		ja_p->year = at.year;

		ja_p->month =
		    (j_time_elt_t *)malloc(sizeof (j_time_elt_t));
		ja_p->month->type_tag = j_atom;
		ja_p->month->value.atom = at.month;
		ja_p->month->next = NULL;

		ja_p->date =
		    (j_time_elt_t *)malloc(sizeof (j_time_elt_t));
		ja_p->date->type_tag = j_atom;
		ja_p->date->value.atom = at.day_of_month;
		ja_p->date->next = NULL;

		ja_p->weekday = NULL;

		ja_p->hour =
		    (j_time_elt_t *)malloc(sizeof (j_time_elt_t));
		ja_p->hour->type_tag = j_atom;
		ja_p->hour->value.atom = at.hour;
		ja_p->hour->next = NULL;

		ja_p->minute =
		    (j_time_elt_t *)malloc(sizeof (j_time_elt_t));
		ja_p->minute->type_tag = j_atom;
		ja_p->minute->value.atom = at.minute;
		ja_p->minute->next = NULL;

		jobsched_free_at(&at);

	} else {

		memset((void *)&cron, 0, sizeof (JSCronArg));

		cron.minute_key = cron_list_to_field_string(ja_p->minute_key);
		cron.hour_key = cron_list_to_field_string(ja_p->hour_key);
		cron.day_key = cron_list_to_field_string(ja_p->date_key);
		cron.month_key = cron_list_to_field_string(ja_p->month_key);
		cron.day_of_week_key =
		    cron_list_to_field_string(ja_p->weekday_key);
		cron.job_key = ja_p->job_key;

		s = jobsched_get_cron(&cron);

		if (s != JOBSCHED_SUCCESS) {
			return (SYSMAN_JOBSCHED_GET_FAILED);
		}

		ja_p->minute = field_string_to_malloc_cron_list(cron.minute);
		if (ja_p->minute == NULL) {
			return (SYSMAN_MALLOC_ERR);
		}
		ja_p->hour = field_string_to_malloc_cron_list(cron.hour);
		if (ja_p->hour == NULL) {
			return (SYSMAN_MALLOC_ERR);
		}
		ja_p->date = field_string_to_malloc_cron_list(cron.day);
		if (ja_p->date == NULL) {
			return (SYSMAN_MALLOC_ERR);
		}
		ja_p->month = field_string_to_malloc_cron_list(cron.month);
		if (ja_p->month == NULL) {
			return (SYSMAN_MALLOC_ERR);
		}
		ja_p->weekday =
		    field_string_to_malloc_cron_list(cron.day_of_week);
		if (ja_p->weekday == NULL) {
			return (SYSMAN_MALLOC_ERR);
		}
		ja_p->job = strdup(cron.job);

		jobsched_free_cron(&cron);
	}

	ja_p->frequency = ja_p->frequency_key;

	return (SYSMAN_SUCCESS);
}


int
_root_get_jobsched(void *arg_p, char *buf, int len)
{

	SysmanSharedJobschedArg	*sja_p = (SysmanSharedJobschedArg *)arg_p;
	SysmanJobschedArg	ja;
	int			s;


	if (sja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	memset((void *)&ja, 0, sizeof (ja));

	if (sja_p->frequency_key == j_once) {
		ja.job_id_key = sja_p->job_id_key;
	} else {
		ja.job_id_key = NULL;
	}
	ja.job_key = sja_p->job_key;
	ja.schedule_as_root_key = sja_p->schedule_as_root_key;
	ja.year_key = sja_p->year_key;
	ja.month_key = sja_p->month_key;
	ja.date_key = sja_p->date_key;
	ja.weekday_key = sja_p->weekday_key;
	ja.hour_key = sja_p->hour_key;
	ja.minute_key = sja_p->minute_key;
	ja.frequency_key = sja_p->frequency_key;

	s = _get_jobsched(&ja, buf, len);

	if (s == SYSMAN_SUCCESS) {

		/* copy back for return */

		sja_p->frequency = ja.frequency;
		if (sja_p->frequency == j_once) {
			strcpy(sja_p->job_id_return, ja.job_id_return);
		} else {
			sja_p->job_id_return[0] = '\0';
		}
		strcpy(sja_p->job, ja.job);
		sja_p->year = ja.year;
		cp_time_a_from_l(sja_p->month,
		    sizeof (sja_p->month) / sizeof (sja_p->month[0]),
		    ja.month);
		cp_time_a_from_l(sja_p->date,
		    sizeof (sja_p->date) / sizeof (sja_p->date[0]),
		    ja.date);
		cp_time_a_from_l(sja_p->weekday,
		    sizeof (sja_p->weekday) / sizeof (sja_p->weekday[0]),
		    ja.weekday);
		cp_time_a_from_l(sja_p->hour,
		    sizeof (sja_p->hour) / sizeof (sja_p->hour[0]),
		    ja.hour);
		cp_time_a_from_l(sja_p->minute,
		    sizeof (sja_p->minute) / sizeof (sja_p->minute[0]),
		    ja.minute);
	}

	if ((s = jobsched_code_to_sysman_code(s)) == SYSMAN_JOBSCHED_FAILED) {
		s = SYSMAN_JOBSCHED_GET_FAILED;
	}

	return (s);
}


void
_free_jobsched(SysmanJobschedArg *ja_p)
{

	j_time_elt_t	*t1;
	j_time_elt_t	*t2;


	if (ja_p == NULL) {
		return;
	}

	if (ja_p->job != NULL) {
		free((void *)ja_p->job);
	}

	if (ja_p->job_id_return != NULL) {
		free((void *)ja_p->job_id_return);
	}

	if (ja_p->month != NULL) {
		for (t1 = t2 = ja_p->month; t2 != NULL; t1 = t2) {
			t2 = t1->next;
			free((void *)t1);
		}
	}
	if (ja_p->date != NULL) {
		for (t1 = t2 = ja_p->date; t2 != NULL; t1 = t2) {
			t2 = t1->next;
			free((void *)t1);
		}
	}
	if (ja_p->weekday != NULL) {
		for (t1 = t2 = ja_p->weekday; t2 != NULL; t1 = t2) {
			t2 = t1->next;
			free((void *)t1);
		}
	}
	if (ja_p->hour != NULL) {
		for (t1 = t2 = ja_p->hour; t2 != NULL; t1 = t2) {
			t2 = t1->next;
			free((void *)t1);
		}
	}
	if (ja_p->minute != NULL) {
		for (t1 = t2 = ja_p->minute; t2 != NULL; t1 = t2) {
			t2 = t1->next;
			free((void *)t1);
		}
	}
}


void
_free_jobsched_list(SysmanJobschedArg *ja_p, int cnt)
{

	int	i;


	if (ja_p == NULL) {
		return;
	}
	for (i = 0; i < cnt; i++) {
		_free_jobsched(ja_p + i);
	}

	free((void *)ja_p);
}



int
_list_jobsched(SysmanJobschedArg **ja_p, char *buf, int bufsiz)
{

	int		i;
	int		j;
	int		cnt;
	char		*user;
	int		at_cnt;
	int		cron_cnt;
	j_time_elt_t	*t;
	JSAtArg		*ap;
	JSCronArg	*cp;


	if (ja_p == NULL) {
		return (SYSMAN_BAD_INPUT);
	}

	user = getpwuid(getuid())->pw_name;

	at_cnt = jobsched_list_at(&ap);
	cron_cnt = jobsched_list_cron(&cp, user);

	*ja_p = (SysmanJobschedArg *)malloc((unsigned)((at_cnt + cron_cnt) *
	    sizeof (SysmanJobschedArg)));

	if (*ja_p == NULL) {
		jobsched_free_at_list(ap, at_cnt);
		jobsched_free_cron_list(cp, cron_cnt);
		return (SYSMAN_MALLOC_ERR);
	}

	cnt = 0;

	for (i = 0; i < at_cnt; i++) {
		if (strcmp(user, ap[i].owner) == 0) {

			memset((void *)&((*ja_p)[i]), 0,
			    sizeof (SysmanJobschedArg));

			(*ja_p)[i].job_id_return = strdup(ap[i].at_job_id);
			(*ja_p)[i].job = strdup(ap[i].job);

			(*ja_p)[i].year = ap[i].year;

			(*ja_p)[i].month = t =
			    (j_time_elt_t *)malloc(sizeof (j_time_elt_t));
			t->type_tag = j_atom;
			t->value.atom = ap[i].month;
			t->next = NULL;

			(*ja_p)[i].date = t =
			    (j_time_elt_t *)malloc(sizeof (j_time_elt_t));
			t->type_tag = j_atom;
			t->value.atom = ap[i].day_of_month;
			t->next = NULL;

			(*ja_p)[i].hour = t =
			    (j_time_elt_t *)malloc(sizeof (j_time_elt_t));
			t->type_tag = j_atom;
			t->value.atom = ap[i].hour;
			t->next = NULL;

			(*ja_p)[i].minute = t =
			    (j_time_elt_t *)malloc(sizeof (j_time_elt_t));
			t->type_tag = j_atom;
			t->value.atom = ap[i].minute;
			t->next = NULL;

			(*ja_p)[i].frequency = j_once;

			cnt++;
		}
	}

	for (i = cnt, j = 0; j < cron_cnt; i++, j++) {

		memset((void *)(*ja_p + i), 0, sizeof (SysmanJobschedArg));

		(*ja_p)[i].job = strdup(cp[j].job);

		(*ja_p)[i].minute =
		   field_string_to_malloc_cron_list(cp[j].minute);
		(*ja_p)[i].hour = field_string_to_malloc_cron_list(cp[j].hour);
		(*ja_p)[i].date = field_string_to_malloc_cron_list(cp[j].day);
		(*ja_p)[i].weekday =
		    field_string_to_malloc_cron_list(cp[j].day_of_week);
		(*ja_p)[i].month =
		    field_string_to_malloc_cron_list(cp[j].month);

		(*ja_p)[i].frequency = interpret_job_frequency(*ja_p + i);

		cnt++;
	}

	return (cnt);
}


int
_root_list_jobsched(void *arg_p, char *buf, int bufsiz)
{

	SysmanSharedJobschedArg	*sja_p = (SysmanSharedJobschedArg *)arg_p;
	SysmanJobschedArg	*ja_p;
	int			i;
	int			cnt;


	cnt = _list_jobsched(&ja_p, buf, bufsiz);

	if (cnt <= 0) {
		return (cnt);
	}

	memset((void *)&sja_p[i], 0, MAX_JOBSCHED_LIST *
	    sizeof (SysmanSharedJobschedArg));

	for (i = 0; i < cnt && i < MAX_JOBSCHED_LIST; i++) {

		if (ja_p[i].job_id_return != NULL) {
			strcpy(sja_p[i].job_id_return, ja_p[i].job_id_return);
		} else {
			sja_p[i].job_id_return[0] = '\0';
		}
		strcpy(sja_p[i].job, ja_p[i].job);
		sja_p[i].year = ja_p[i].year;
		cp_time_a_from_l(sja_p[i].month,
		    sizeof (sja_p[i].month) / sizeof (sja_p[i].month[0]),
		    ja_p[i].month);
		cp_time_a_from_l(sja_p[i].date,
		    sizeof (sja_p[i].date) / sizeof (sja_p[i].date[0]),
		    ja_p[i].date);
		cp_time_a_from_l(sja_p[i].weekday,
		    sizeof (sja_p[i].weekday) / sizeof (sja_p[i].weekday[0]),
		    ja_p[i].weekday);
		cp_time_a_from_l(sja_p[i].hour,
		    sizeof (sja_p[i].hour) / sizeof (sja_p[i].hour[0]),
		    ja_p[i].hour);
		cp_time_a_from_l(sja_p[i].minute,
		    sizeof (sja_p[i].minute) / sizeof (sja_p[i].minute[0]),
		    ja_p[i].minute);
		sja_p[i].frequency = ja_p[i].frequency;
	}

	_free_jobsched_list(ja_p, cnt);

	/*
	 * i will always have the correct return count; however, if the
	 * list was truncated at MAX_JOBSCHED_LIST, there should be
	 * some indication of this.  Currently there isn't.
	 */

	return (i);
}
