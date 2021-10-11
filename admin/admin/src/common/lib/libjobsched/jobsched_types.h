/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)jobsched_types.h	1.6	95/07/21 SMI"

#ifndef	_JOBSCHED_TYPES_H
#define	_JOBSCHED_TYPES_H


#include <sys/types.h>


typedef enum { js_csh, js_ksh, js_sh } js_shell_t;


typedef struct _js_at_arg_struct {
	const char	*at_job_id_key;
	char		*at_job_id;
	char		*job;
	char		*owner;
	boolean_t	job_is_filename_p;
	js_shell_t	shell;
	boolean_t	send_at_mail_p;
	int		year;
	int		month;
	int		day_of_month;
	int		hour;
	int		minute;
	int		sec;
} JSAtArg;

typedef struct _js_cron_arg_struct {
	const char	*username_key;
	const char	*minute_key;
	const char	*hour_key;
	const char	*day_key;
	const char	*month_key;
	const char	*day_of_week_key;
	const char	*job_key;
	char		*username;
	char		*minute;
	char		*hour;
	char		*day;
	char		*month;
	char		*day_of_week;
	char		*job;
} JSCronArg;

#endif	/* _JOBSCHED_TYPES_H */
