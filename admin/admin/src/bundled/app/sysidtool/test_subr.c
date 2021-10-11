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

#pragma	ident	"@(#)test_subr.c 1.2 93/11/01"

#include <stdio.h>
#include <signal.h>
#include "sysid_ui.h"

/*ARGSUSED*/
Sysid_err
do_init(char *init_arg, int *argcp, char **argv, int source, int reply_to)
{
	MSG	*mp;

	(void) signal(SIGPOLL, SIG_IGN);	/* stream hangups */
	(void) signal(SIGUSR1, SIG_IGN);	/* start signal -- just GO! */

	/* insert whatever processing you want to do here */

	for (;;) {
		mp = msg_receive(source);
		run_display(mp, reply_to);
	}
	/*NOTREACHED*/
#ifdef lint
	return (SYSID_SUCCESS);
#endif
}

/*
 * clean up the UI process
 */
void
do_cleanup(char *text, int *do_exit)
{
	/* insert whatever processing you want to do here */
	/*
	 * This line tells the ui layer you
	 * want it to exit regardless of
	 * prompt_close()'s 2nd argument.
	 */
	*do_exit = 1;
}

/*ARGSUSED*/
void
do_error(char *errstr, int reply_to)
{
	/* insert whatever processing you want to do here */

	reply_void(reply_to);
}

/*ARGSUSED*/
void
do_message(MSG *mp, int reply_to)
{
	char	buf[MAX_FIELDVALLEN];
	void	*arg;

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				arg, sizeof (arg));
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)buf, sizeof (buf));
        msg_delete(mp);

	/* insert whatever processing you want to do here */

	reply_integer(ATTR_PROMPT, (int)arg, reply_to);
}

/*ARGSUSED*/
void
do_dismiss(MSG *mp, int reply_to)
{
	void	*arg;

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)arg, sizeof (arg));
	msg_delete(mp);

	/* insert whatever processing you want to do here */

	reply_void(reply_to);
}

/*ARGSUSED*/
void
do_form(char *text, Field_desc *fields, int nfields, int reply_to)
{
	Menu	*menu;
	MSG	*mp;
	int	i;

	/*
	 * The "input" section starts here.  We
	 * are handed a list of fields and are
	 * supposed to supply values (in f->value)
	 * for those that are not FIELD_INSENSITIVE
	 * (read-only).
	 *
	 * Note that do_form() handles the "generic"
	 * attributes -- complicated attributes like
	 * "timezone" have their own input routines.
	 */
	for (i = 0; i < nfields; i++) {
		Field_desc *f = &fields[i];

		switch (f->attr) {
		case ATTR_HOSTNAME:
			break;
		case ATTR_ISIT_STANDALONE:
			break;
		case ATTR_HOSTIP:
			break;
		case ATTR_PRIMARY_NET:
			break;
		case ATTR_NAME_SERVICE:
			break;
		case ATTR_DOMAIN:
			break;
		case ATTR_BROADCAST:
			break;
		case ATTR_NISSERVERNAME:
			break;
		case ATTR_NISSERVERADDR:
			break;
		case ATTR_ISIT_SUBNET:
			break;
		case ATTR_NETMASK:
			break;
		case ATTR_BAD_NIS:
			break;
		case ATTR_DATE_AND_TIME:	/* read-only "summary" field */
			break;
		case ATTR_YEAR:
			break;
		case ATTR_MONTH:
			break;
		case ATTR_DAY:
			break;
		case ATTR_HOUR:
			break;
		case ATTR_MINUTE:
			break;
		default:
			break;
		}
	}

	/*
	 * The "reply" section starts here.  The
	 * function of this code is to create a
	 * reply message with arguments containing
	 * the values we've just set.
	 */
	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);

	for (i = 0; i < nfields; i++) {
		Field_desc *f = &fields[i];

		if ((f->flags & FIELD_INSENSITIVE) == 0) {
			switch (f->type) {
			case FIELD_TEXT:
				(void) msg_add_arg(mp, f->attr, VAL_STRING,
				    f->value, strlen((char *)f->value) + 1);
				break;
			case FIELD_EXCLUSIVE_CHOICE:
				menu = (Menu *)f->value;
				(void) msg_add_arg(mp, f->attr, VAL_INTEGER,
				    (void *)&menu->selected, sizeof (int));
				break;
			case FIELD_CONFIRM:
				(void) msg_add_arg(mp, f->attr, VAL_INTEGER,
				    (void *)&f->value, sizeof (int));
				break;
			}
		}
	}
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}

/*ARGSUSED*/
void
do_confirm(char *text, Field_desc *fields, int nfields, int reply_to)
{
	int	confirmed = CONFIRM_NOTYET;

	/*
	 * You have two choices when you finish
	 * the implementation of this routine.
	 * You can ignore the incoming values and
	 * just send back an answer or you can
	 * duplicate (or pull out into a common
	 * routine, e.g., form_common) the code
	 * from above that does the input parsing.
	 */

	reply_integer(ATTR_CONFIRM, confirmed, reply_to);
}

void
get_locale(MSG *mp, int reply_to)
{
	char	**locale_domain;
	int	n_locales;
	int	select = -1;

	locale_domain = msg_get_array(mp, ATTR_LOCALE, &n_locales);
	msg_delete(mp);

	/* insert whatever processing you want to do here */

	msg_free_array(locale_domain, n_locales);	/* cleanup */

	reply_integer(ATTR_LOCALE, select, reply_to);
}

void
get_terminal(MSG *mp, int reply_to)
{
	char	*termtype = "";

	msg_delete(mp);

	/* insert whatever processing you want to do here */

	reply_string(ATTR_TERMINAL, termtype, reply_to);
}

void
get_password(MSG *mp, int reply_to)
{
	char	*pw = "";
	char	*e_pw = "";

	msg_delete(mp);

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_add_arg(mp, ATTR_PASSWORD, VAL_STRING,
					(void *)pw, strlen(pw) + 1);

	/* Encrypt the password */
	if (*pw != NULL) {
		e_pw = encrypt_pw(pw);
		(void) msg_add_arg(mp, ATTR_EPASSWORD, VAL_STRING,
					(void *)e_pw, strlen(e_pw) + 1);
	}
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}

/*ARGSUSED*/
void
get_timezone(char	 *text,
	char		*timezone,
	Field_desc	*regions,
	Field_desc	*gmt_offset,
	Field_desc	*tz_filename,
	int		reply_to)
{
	Menu	*region_menu = (Menu *)regions->value;
	Menu	**tz_menus = (Menu **)region_menu->values;
	int	region_pick;
	int	tz_pick;
	MSG	*mp;

	region_pick = region_menu->selected;
	if (region_pick >= 0 && region_pick < region_menu->nitems)
		tz_pick = tz_menus[region_pick]->selected;
	else
		tz_pick = NO_INTEGER_VALUE;

	/* insert whatever processing you want to do here */

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_add_arg(mp, ATTR_TIMEZONE, VAL_STRING,
			(void *)timezone, MAX_TZ+1);
	(void) msg_add_arg(mp, ATTR_TZ_REGION, VAL_INTEGER,
			(void *)&region_pick, sizeof (region_pick));
	(void) msg_add_arg(mp, ATTR_TZ_INDEX, VAL_INTEGER,
			(void *)&tz_pick, sizeof (tz_pick));
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}
