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

#pragma	ident	"@(#)ui_utils.c 1.6 95/02/07"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <crypt.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sysid_ui.h"

static void	onpoll(int);

/*ARGSUSED*/
static void
onpoll(int sig)
{
	char	path[MAXPATHLEN];
	char	*fifo_dir;

	fifo_dir = get_fifo_dir();
	(void) sprintf(path, "%s/%s", fifo_dir, SYSID_UI_PID);
	(void) unlink(path);

	(void) sprintf(path, "%s/%s", fifo_dir, SYSID_UI_FIFO_IN);
	(void) unlink(path);

	(void) sprintf(path, "%s/%s", fifo_dir, SYSID_UI_FIFO_OUT);
	(void) unlink(path);
}

void
ui_get_confirm(MSG *mp, int reply_to)
{
	Field_desc	*f;
	char		value[MAX_FIELDVALLEN];
	int		i, nfields, flags;

	nfields = msg_get_nargs(mp) / 3;

	f = (Field_desc *)xmalloc(nfields * sizeof (Field_desc));

	for (i = 0; i < nfields; i++) {
		f[i].type = FIELD_TEXT;
		f[i].field_length = -1;
		f[i].value_length = -1;
		f[i].flags = FF_RDONLY | FF_LAB_RJUST | FF_LAB_ALIGN;
		f[i].validate = (Validate_proc *)0;
		/*
		 * Get the rest of the parameters
		 * from the incoming message args.
		 */
		(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
			&f[i].user_data, sizeof (Sysid_attr));
		f[i].label =
		    xstrdup(dl_get_attr_name((Sysid_attr)f[i].user_data));

		(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)value, MAX_FIELDVALLEN);
		f[i].value = (void *)xstrdup(value);

		(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
			(void *)&flags, sizeof (flags));
	}
	msg_delete(mp);

	dl_do_confirm(
		dl_get_attr_title(ATTR_CONFIRM),
		dl_get_attr_text(ATTR_CONFIRM),
		f, nfields, reply_to);
}

#define	MAXARGS		20

void
ui_error(MSG *mp, int reply_to)
{
	char	errstr[BUFSIZ];

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)errstr, sizeof (errstr));
	msg_delete(mp);

	dl_do_error(errstr, reply_to);
}

/*
 * Generic routine for validating fields containing
 * integers.  Uses the vmin and vmax values contained
 * in the field descripition.
 */
int
ui_valid_integer(Field_desc *f)
{
	char	*numbers = "0123456789-";
	char	*input = (char *)f->value;
	char	*cp;
	int	val;

	if ((f->flags & FF_VALREQ) && input[0] == '\0')
		return (SYSID_ERR_NO_VALUE);

	for (cp = input; *cp != '\0'; cp++) {
		if (strchr(numbers, *cp) == (char *)0)
			return (SYSID_ERR_BAD_DIGIT);
	}
	val = atoi(input);

	if (f->vmin != -1 && val < f->vmin)
		return (SYSID_ERR_MIN_VALUE_EXCEEDED);

	if (f->vmax != -1 && val > f->vmax)
		return (SYSID_ERR_MAX_VALUE_EXCEEDED);

	return (SYSID_SUCCESS);
}

/*
 * Generic routine for validating fields containing
 * choices.  The user must enter a choice.
 */
int
ui_valid_choice(Field_desc *f)
{
	Menu	*menu = (Menu *)f->value;

	if ((f->flags & FF_VALREQ) &&
	    menu->selected == NO_INTEGER_VALUE)
		return (SYSID_ERR_NO_SELECTION);

	return (SYSID_SUCCESS);
}

/*ARGSUSED*/
void
ui_set_locale(MSG *mp, int reply_to)
{
	char	locale[MAX_LOCALE+1];
	FILE	*mfp;
	char	path[MAXPATHLEN];

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)locale, sizeof (locale));
	msg_delete(mp);

	(void) sprintf(path, "/usr/lib/locale/%s/locale_map", locale);
	if ((mfp = fopen(path, "r")) == NULL) {
		/* allow operation without the full localization installed */
		if (setlocale(LC_ALL, locale) == (char *)0)
			(void) setlocale(LC_MESSAGES, locale);
	} else {
		char	lc_collate[MAX_LOCALE], lc_ctype[MAX_LOCALE],
			lc_messages[MAX_LOCALE], lc_monetary[MAX_LOCALE],
			lc_numeric[MAX_LOCALE], lc_time[MAX_LOCALE],
			lang[MAX_LOCALE];

		(void) read_locale_file(mfp, lang, lc_collate, lc_ctype,
			lc_messages, lc_monetary, lc_numeric, lc_time);

		fclose(mfp);

		(void) setlocale(LC_COLLATE, lc_collate);
		(void) setlocale(LC_CTYPE, lc_ctype);
		(void) setlocale(LC_MESSAGES, lc_messages);
		(void) setlocale(LC_MONETARY, lc_monetary);
		(void) setlocale(LC_NUMERIC, lc_numeric);
		(void) setlocale(LC_TIME, lc_time);
	}
}

/*
 * ui_set_term
 *
 *	Set TERM environment variable
 */
/*ARGSUSED*/
void
ui_set_term(MSG *mp, int reply_to)
{
	char	termtype[MAX_TERM+1];
	char	*termattr = "TERM=";
	char	*termstr;

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)termtype, sizeof (termtype));
	msg_delete(mp);

	termstr = (char *)xmalloc(strlen(termattr) + strlen(termtype) + 1);
	(void) sprintf(termstr, "TERM=%s", termtype);
	(void) putenv(termstr);
}

/*ARGSUSED*/
void
ui_cleanup(MSG *mp, int reply_to)
{
	char    buf[1024];
	int	do_exit;

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)buf, sizeof (buf));
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&do_exit, sizeof (do_exit));
	msg_delete(mp);

	dl_do_cleanup(buf, &do_exit);

	if (do_exit) {
		(void) signal(SIGPOLL, onpoll);
		(void) sighold(SIGPOLL);
	}

	reply_integer(ATTR_DOEXIT, do_exit, reply_to);

	if (do_exit) {
		(void) sigpause(SIGPOLL);
		exit(0);
		/*NOTREACHED*/
	}
}

void
free_menu(Menu *menu)
{
	int	i;

	if (menu->labels != (char **)0) {
		for (i = 0; i < menu->nitems; i++)
			if (menu->labels[i] != (char *)0)
				free(menu->labels[i]);
		free(menu->labels);
	}
	if (menu->values != (void *)0) {
		char	**valstrs = (char **)menu->values;

		for (i = 0; i < menu->nitems; i++)
			if (valstrs[i] != (char *)0)
				free(valstrs[i]);
		free(menu->values);
	}
}

char *
encrypt_pw(char *passwd)
{
	char	*e_pw;
	time_t	salt;
	char	saltc[2];
	int	i;
	int	m;

	(void) time((time_t *)&salt);
	salt += (long)getpid();
	saltc[0] = salt & 077;
	saltc[1] = (salt >> 6) & 077;
	for (i = 0; i < 2; i++) {
		m = saltc[i] + '.';
		if (m > '9') m += 7;
		if (m > 'Z') m += 6;
		saltc[i] = m;
	}
	e_pw = crypt((const char *)passwd, (const char *)saltc);
	return (e_pw);
}

#define	STATE_FILE	"/etc/.sysIDtool.state"

/*
 * Returns true if we're running in the install
 * environment, false otherwise.  If the state
 * file exists and is a symlink, we're in the
 * install environment.
 */
int
is_install_environment(void)
{
	struct stat sbuf;
	int	status;

	if (lstat(STATE_FILE, &sbuf) < 0 || S_ISLNK(sbuf.st_mode) == 0)
		status = 0;
	else
		status = 1;
#ifdef DEV
	status = 1;
#endif
	return (status);
}
