/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)jobsched_iface.h	1.4	95/06/28 SMI"

#ifndef	_JOBSCHED_IFACE_H
#define	_JOBSCHED_IFACE_H


#include <sys/types.h>
#include "jobsched_types.h"
#include "jobsched_codes.h"


extern int	jobsched_add_at(JSAtArg *ap, char **ret_job_id);
extern int	jobsched_add_cron(JSCronArg *cp);
extern int	jobsched_delete_at(JSAtArg *ap);
extern int	jobsched_delete_cron(JSCronArg *cp);
extern int	jobsched_modify_at(JSAtArg *ap, char **ret_job_id);
extern int	jobsched_modify_cron(JSCronArg *cp);
extern int	jobsched_list_at(JSAtArg **app);
extern int	jobsched_get_at(JSAtArg *ap);
extern void	jobsched_free_at(JSAtArg *ap);
extern void	jobsched_free_at_list(JSAtArg *ap, int cnt);
extern int	jobsched_list_cron(JSCronArg **cpp, const char *username);
extern int	jobsched_get_cron(JSCronArg *cp);
extern void	jobsched_free_cron(JSCronArg *cp);
extern void	jobsched_free_cron_list(JSCronArg *cp, int cnt);


#endif	/* _JOBSCHED_IFACE_H */
