/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)set_timezone.c	1.1	94/08/23 SMI"

#include <string.h>
#include <libintl.h>
#include <sys/param.h>	/* for definition of MAXPATHLEN */
#include <sys/types.h>
#include <sys/stat.h>	/* For x86 rtc stuff */
#include <tzfile.h>
#include "admutil.h"

#define	UFS_DB	"/etc/TIMEZONE"
#define TZ_VAR	"TZ"
#define RTC_PATH "/usr/sbin/rtc"

/*
 * return
 *  0 - ok
 *  > 0 - errno (system error)
 *  < 0 - internal error
 *	ADMUTIL_SETTZ_BAD: invalid timezone string
 *	ADMUTIL_SETTZ_RTC: error running rtc command
 */
int
set_timezone(char *timezone)
{
	int status;
	char path[MAXPATHLEN];
	struct stat rtc_stat;

	if ((strlen(timezone) == 0) || !valid_timezone(timezone))
		return (ADMUTIL_SETTZ_BAD);

	if ((status = set_env_var(UFS_DB, TZ_VAR, timezone)) != 0)
		return (status);

	/*
	 * If 'rtc' exists, then run it.
	 */
        if (stat(RTC_PATH, &rtc_stat) == 0) {
		sprintf(path, "%s -z %s", RTC_PATH, timezone);
                if (system(path) != 0)
			return (ADMUTIL_SETTZ_RTC);

		sprintf(path, "%s -c", RTC_PATH);
		if (system(path) != 0)
			return (ADMUTIL_SETTZ_RTC);
        }

	return (0);
}
