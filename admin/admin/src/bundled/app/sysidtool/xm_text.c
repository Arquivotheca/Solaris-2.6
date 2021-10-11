/*
 * Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
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

#pragma	ident	"@(#)xm_text.c 1.7 95/11/13"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include "xm_defs.h"
#include "xm_help.h"
#include "xm_msgs.h"

/*
 * tty-private string routine
 */
char *
_get_err_string(int errcode, int nargs, ...)
{
	va_list	ap;
	char	*errstr;

	va_start(ap, nargs);
	errstr = get_err_string(errcode, nargs, ap);
	va_end(ap);

	return (errstr);
}

/*ARGSUSED*/
char *
get_err_string(int errcode, int nargs, va_list ap)
{
	static char errbuf[BUFSIZ];
	char	*errstr;

	switch (errcode) {
	case SYSID_ERR_IPADDR_MAX:
	case SYSID_ERR_IPADDR_MIN:
		errstr = MAX_IP_PART;
		break;
	case SYSID_ERR_IPADDR_RANGE:
		errstr = IP_NOT_IN_RANGE;
		break;
	case SYSID_ERR_IPADDR_UNSPEC:
		errstr = IP_IN_UNSPEC_RANGE;
		break;
	case SYSID_ERR_IPADDR_FMT:
		errstr = FOUR_PART_IP_ADDR;
		break;
	case SYSID_ERR_HOSTNAME_LEN:
		(void) sprintf(errbuf, HOST_LENGTH, 2, MAX_HOSTNAME - 1);
		errstr = errbuf;
		break;
	case SYSID_ERR_HOSTNAME_CHARS:
		errstr = HOST_CHARS;
		break;
	case SYSID_ERR_HOSTNAME_MINUS:
		errstr = HOST_MINUS;
		break;
	case SYSID_ERR_DOMAIN_LEN:
		(void) sprintf(errbuf, DOMAIN_LENGTH, 1, MAX_DOMAINNAME - 1);
		errstr = errbuf;
		break;
	case SYSID_ERR_NETMASK_FMT:
		errstr = FOUR_PART_NETMASK;
		break;
	case SYSID_ERR_NETMASK_RANGE:
		errstr = MAX_NETMASK_PART;
		break;
	case SYSID_ERR_BAD_DIGIT:
		errstr = NOT_A_DIGIT;
		break;
	case SYSID_ERR_NO_VALUE:
		errstr = NOTHING_ENTERED;
		break;
	case SYSID_ERR_NO_SELECTION:
		errstr = NOTHING_SELECTED;
		break;
	case SYSID_ERR_MIN_VALUE_EXCEEDED:
		errstr = MIN_VALUE_EXCEEDED;
		break;
	case SYSID_ERR_MAX_VALUE_EXCEEDED:
		errstr = MAX_VALUE_EXCEEDED;
		break;
	case SYSID_ERR_DLOPEN_FAIL:
		errstr = DLOPEN_FAILED;
		break;
	case SYSID_ERR_XTINIT_FAIL:
		errstr = XTINIT_FAILED;
		break;
	case SYSID_ERR_BAD_TZ_FILE_NAME:
		errstr = BAD_TZ_FILE_NAME;
		break;
	case SYSID_ERR_BAD_YEAR:
		errstr = BAD_YEAR;
		break;
	case SYSID_SUCCESS:
		break;
	default:
		(void) sprintf(errbuf, UNKNOWN_ERROR, errcode);
		errstr = errbuf;
		break;
	}
	return (errstr);
}

/*
 * Provide titles for each attribute, to be used to
 * tag each screen in both the GUI and tty interfaces.
 * Only generic forms (the ones in the ui_* files)
 * get their titles via this routine.
 */
char *
get_attr_title(Sysid_attr attr)
{
	static char title[256];

	switch ((int)attr) {
	case ATTR_HOSTNAME:
		(void) strncpy(title, HOSTNAME_TITLE, sizeof (title));
		break;
	case ATTR_PRIMARY_NET:
		(void) strncpy(title, PRIMARY_NET_TITLE, sizeof (title));
		break;
	case ATTR_HOSTIP:
		(void) strncpy(title, HOSTIP_TITLE, sizeof (title));
		break;
	case ATTR_NAME_SERVICE:
		(void) strncpy(title, NAME_SERVICE_TITLE, sizeof (title));
		break;
	case ATTR_DOMAIN:
		(void) strncpy(title, DOMAIN_TITLE, sizeof (title));
		break;
	case ATTR_BROADCAST:
		(void) strncpy(title, BROADCAST_TITLE, sizeof (title));
		break;
	case ATTR_NETMASK:
		(void) strncpy(title, NETMASK_TITLE, sizeof (title));
		break;
	case ATTR_SUBNETTED:
		(void) strncpy(title, SUBNETTED_TITLE, sizeof (title));
		break;
	case ATTR_NISSERVERNAME:
	case ATTR_NISSERVERADDR:
		(void) strncpy(title, NISSERVER_TITLE, sizeof (title));
		break;
	case ATTR_DATE_AND_TIME:
	case ATTR_YEAR:
	case ATTR_MONTH:
	case ATTR_DAY:
	case ATTR_HOUR:
	case ATTR_MINUTE:
		(void) strncpy(title, DATE_AND_TIME_TITLE, sizeof (title));
		break;
	case ATTR_CONFIRM:
		(void) strncpy(title, CONFIRM_TITLE, sizeof (title));
		break;
	case ATTR_BAD_NIS:
		(void) strncpy(title, BAD_NIS_TITLE, sizeof (title));
		break;
	case ATTR_NETWORKED:
		(void) strncpy(title, NETWORKED_TITLE, sizeof (title));
		break;
	case ATTR_ERROR:
		(void) strncpy(title, ERROR_TITLE, sizeof (title));
		break;
	default:
		(void) strncpy(title, NOTICE_TITLE, sizeof (title));
		break;
	}
	title[sizeof (title) - 1] = '\0';	/* terminate */

	return (title);
}

/*
 * Provide text for each attribute, to be used as
 * intro on each screen.  Only generic forms (the
 * ones in the ui_* files) get their text via
 * this routine.
 */
char *
get_attr_text(Sysid_attr attr)
{
	char	*text;

	switch ((int)attr) {
	case ATTR_HOSTNAME:
		text = HOSTNAME_TEXT;
		break;
	case ATTR_NETWORKED:
		text = NETWORKED_TEXT;
		break;
	case ATTR_PRIMARY_NET:
		text = PRIMARY_NET_TEXT;
		break;
	case ATTR_HOSTIP:
		text = HOSTIP_TEXT;
		break;
	case ATTR_NAME_SERVICE:
		text = NAME_SERVICE_TEXT;
		break;
	case ATTR_DOMAIN:
		text = DOMAIN_TEXT;
		break;
	case ATTR_BROADCAST:
		text = BROADCAST_TEXT;
		break;
	case ATTR_NISSERVERNAME:
	case ATTR_NISSERVERADDR:
		text = NISSERVER_TEXT;
		break;
	case ATTR_SUBNETTED:
		text = SUBNETTED_TEXT;
		break;
	case ATTR_NETMASK:
		text = NETMASK_TEXT;
		break;
	case ATTR_DATE_AND_TIME:
	case ATTR_YEAR:
	case ATTR_MONTH:
	case ATTR_DAY:
	case ATTR_HOUR:
	case ATTR_MINUTE:
		text = DATE_AND_TIME_TEXT;
		break;
	case ATTR_CONFIRM:
		text = CONFIRM_TEXT;
		break;
	default:
		text = "";
		break;
	}
	return (text);
}

/*
 * Provide names for each attribute, to be used
 * in during prompting.  Only generic forms (the
 * ones in the ui_* files) get their text via
 * this routine.
 */
char *
get_attr_prompt(Sysid_attr attr)
{
	char	*prompt;

	switch ((int)attr) {
	case ATTR_HOSTNAME:
		prompt = HOSTNAME_PROMPT;
		break;
	case ATTR_NETWORKED:
		prompt = NETWORKED_PROMPT;
		break;
	case ATTR_PRIMARY_NET:
		prompt = PRIMARY_NET_PROMPT;
		break;
	case ATTR_HOSTIP:
		prompt = HOSTIP_PROMPT;
		break;
	case ATTR_NAME_SERVICE:
		prompt = NAME_SERVICE_PROMPT;
		break;
	case ATTR_DOMAIN:
		prompt = DOMAIN_PROMPT;
		break;
	case ATTR_BROADCAST:
		prompt = BROADCAST_PROMPT;
		break;
	case ATTR_NISSERVERNAME:
		prompt = NISSERVERNAME_PROMPT;
		break;
	case ATTR_NISSERVERADDR:
		prompt = NISSERVERADDR_PROMPT;
		break;
	case ATTR_SUBNETTED:
		prompt = SUBNETTED_PROMPT;
		break;
	case ATTR_NETMASK:
		prompt = NETMASK_PROMPT;
		break;
	case ATTR_TZ_REGION:
		prompt = TZ_REGION_PROMPT;
		break;
	case ATTR_TZ_INDEX:
		prompt = TZ_INDEX_PROMPT;
		break;
	case ATTR_TZ_GMT:
		prompt = TZ_GMT_PROMPT;
		break;
	case ATTR_TZ_FILE:
		prompt = TZ_FILE_PROMPT;
		break;
	case ATTR_YEAR:
		prompt = YEAR;
		break;
	case ATTR_MONTH:
		prompt = MONTH;
		break;
	case ATTR_DAY:
		prompt = DAY;
		break;
	case ATTR_HOUR:
		prompt = HOUR;
		break;
	case ATTR_MINUTE:
		prompt = MINUTE;
		break;
	case ATTR_BAD_NIS:
		prompt = BAD_NIS_PROMPT;
		break;
	default:	/* XXX shouldn't happen */
		prompt = "<UNKNOWN ATTRIBUTE>";
		break;
	}
	return (prompt);
}

/*
 * Provide names for each attribute, to be used
 * in confirmation screens.  Only generic forms
 * (the ones in the ui_* files) get their text
 * via this routine.
 */
char *
get_attr_name(Sysid_attr attr)
{
	char	*name;

	switch ((int)attr) {
	case ATTR_HOSTNAME:
		name = HOSTNAME_CONFIRM;
		break;
	case ATTR_NETWORKED:
		name = NETWORKED_CONFIRM;
		break;
	case ATTR_PRIMARY_NET:
		name = PRIMARY_NET_CONFIRM;
		break;
	case ATTR_HOSTIP:
		name = HOSTIP_CONFIRM;
		break;
	case ATTR_NAME_SERVICE:
		name = NAME_SERVICE_CONFIRM;
		break;
	case ATTR_DOMAIN:
		name = DOMAIN_CONFIRM;
		break;
	case ATTR_BROADCAST:
		name = BROADCAST_CONFIRM;
		break;
	case ATTR_NISSERVERNAME:
		name = NISSERVERNAME_CONFIRM;
		break;
	case ATTR_NISSERVERADDR:
		name = NISSERVERADDR_CONFIRM;
		break;
	case ATTR_SUBNETTED:
		name = SUBNETTED_CONFIRM;
		break;
	case ATTR_NETMASK:
		name = NETMASK_CONFIRM;
		break;
	case ATTR_TIMEZONE:
		name = TIMEZONE_CONFIRM;
		break;
	case ATTR_DATE_AND_TIME:
		name = DATE_AND_TIME_CONFIRM;
		break;
	default:	/* XXX shouldn't happen */
		name = "<UNKNOWN ATTRIBUTE>";
		break;
	}
	return (name);
}

char	*helpdir;
/*
 * Provide help location for each attribute
 */
Field_help *
get_attr_help(Sysid_attr attr, Field_help *help)
{
	static char help_path[MAXPATHLEN];
	static char lastlocale[MAX_LOCALE];
	char	*locale;
	/* XXX extern char *helpdir; */

	locale = setlocale(LC_MESSAGES, (char *)0);
	if (strcmp(locale, lastlocale)) {
		char	*helproot = getenv("SYSID_HELPROOT");
		/*
		 * Locale has changed or it's the
		 * first time through this code.
		 */
		(void) sprintf(help_path, "%s/%s/help/%s",
			helproot ? helproot : HELPROOT, locale, HELPDIR);
		if (access(help_path, X_OK) < 0)
			(void) sprintf(help_path, "%s/C/help/%s",
				helproot ? helproot : HELPROOT, HELPDIR);
		helpdir = help_path;
		(void) strncpy(lastlocale, locale, MAX_LOCALE);
		lastlocale[MAX_LOCALE-1] = '\0';
	}

	help->reference = GLOSSARY;

	switch ((int)attr) {
	case ATTR_HOSTNAME:
		help->howto = HOSTNAME_HOWTO;
		help->topics = HOSTNAME_TOPICS;
		break;
	case ATTR_NETWORKED:
		help->howto = NETWORKED_HOWTO;
		help->topics = NETWORKED_TOPICS;
		break;
	case ATTR_PRIMARY_NET:
		help->howto = NETIF_HOWTO;
		help->topics = NETIF_TOPICS;
		break;
	case ATTR_HOSTIP:
		help->howto = HOSTIP_HOWTO;
		help->topics = HOSTIP_TOPICS;
		break;
	case ATTR_NAME_SERVICE:
		help->howto = NS_HOWTO;
		help->topics = NS_TOPICS;
		break;
	case ATTR_DOMAIN:
		help->howto = DOMAIN_HOWTO;
		help->topics = DOMAIN_TOPICS;
		break;
	case ATTR_BROADCAST:
		help->howto = BROADCAST_HOWTO;
		help->topics = BROADCAST_TOPICS;
		break;
	case ATTR_NISSERVERNAME:
	case ATTR_NISSERVERADDR:
		help->howto = SERVER_HOWTO;
		help->topics = SERVER_TOPICS;
		break;
	case ATTR_SUBNETTED:
		help->howto = SUBNET_HOWTO;
		help->topics = SUBNET_TOPICS;
		break;
	case ATTR_NETMASK:
		help->howto = NETMASK_HOWTO;
		help->topics = NETMASK_TOPICS;
		break;
	case ATTR_TIMEZONE:
	case ATTR_TZ_REGION:
	case ATTR_TZ_INDEX:
		help->howto = TZREGION_HOWTO;
		help->topics = TZREGION_TOPICS;
		break;
	case ATTR_TZ_FILE:
		help->howto = TZFILE_HOWTO;
		help->topics = TZFILE_TOPICS;
		break;
	case ATTR_TZ_GMT:
		help->howto = TZGMT_HOWTO;
		help->topics = TZGMT_TOPICS;
		break;
	case ATTR_DATE_AND_TIME:
		help->howto = DATE_HOWTO;
		help->topics = DATE_TOPICS;
		break;
	case ATTR_CONFIRM:
		help->howto = CONFIRM_HOWTO;
		help->topics = CONFIRM_TOPICS;
		break;
	default:
		help->howto = NAV_HOWTO;
		help->topics = NAV_TOPICS;
		break;
	}
	return (help);
}
