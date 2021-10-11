/*
 * Copyright (c) 1993-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)ui_dlib.c 1.4 96/02/05"

#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include "sysid_ui.h"
#include "sysid_msgs.h"

static void	*dl_handle;

/*ARGSUSED*/
Sysid_err
dl_init(char *dl_path, int *argcp, char **argv, int source, int reply_to)
{
	Sysid_err	(*do_init)(int *, char **, int, int);
	Sysid_err	status;

	dl_handle = dlopen(dl_path, RTLD_LAZY);
	if (dl_handle == (void *)0) {
		/*
		 * Fail silently on open except when debugging.
		 */
#ifdef DEBUG
		perror(dlerror());
#endif
		return (SYSID_ERR_DLOPEN_FAIL);
	}
	do_init = (Sysid_err(*)(int *, char **, int, int))
						dlsym(dl_handle, "do_init");
	if (do_init == (Sysid_err(*)(int *, char **, int, int))0) {
		perror(dlerror());
		return (SYSID_ERR_OP_UNSUPPORTED);
	}
	status = (*do_init)(argcp, argv, source, reply_to);
	if (status != SYSID_SUCCESS)
		(void) dlclose(dl_handle);
	return (status);
}

/*
 * clean up the UI process
 */
void
dl_do_cleanup(char *text, int *do_exit)
{
	void	(*do_cleanup)(char *, int *);

	do_cleanup = (void(*)(char *, int *))dlsym(dl_handle, "do_cleanup");
	if (do_cleanup == (void(*)(char *, int *))0)
		perror(dlerror());
	else
		(*do_cleanup)(text, do_exit);
}

/*ARGSUSED*/
void
dl_do_error(char *errstr, int reply_to)
{
	void	(*do_error)(char *, int);

	do_error = (void(*)(char *, int))dlsym(dl_handle, "do_error");
	if (do_error == (void(*)(char *, int))0)
		reply_error(SYSID_ERR_OP_UNSUPPORTED, dlerror(), reply_to);
	else
		(*do_error)(errstr, reply_to);
}

/*ARGSUSED*/
void
dl_do_message(MSG *mp, int reply_to)
{
	void	(*do_message)(MSG *, int);

	do_message = (void(*)(MSG *, int))dlsym(dl_handle, "do_message");
	if (do_message == (void(*)(MSG *, int))0)
		reply_error(SYSID_ERR_OP_UNSUPPORTED, dlerror(), reply_to);
	else
		(*do_message)(mp, reply_to);
}

/*ARGSUSED*/
void
dl_do_dismiss(MSG *mp, int reply_to)
{
	void	(*do_dismiss)(MSG *, int);

	do_dismiss = (void(*)(MSG *, int))dlsym(dl_handle, "do_dismiss");
	if (do_dismiss == (void(*)(MSG *, int))0)
		reply_error(SYSID_ERR_OP_UNSUPPORTED, dlerror(), reply_to);
	else
		(*do_dismiss)(mp, reply_to);
}

/*ARGSUSED*/
void
dl_do_form(
	char		*title,
	char		*text,
	Field_desc	*fields,
	int		nfields,
	int		reply_to)
{
	Form_proc	*do_form;

	do_form = (Form_proc *)dlsym(dl_handle, "do_form");
	if (do_form == (Form_proc *)0)
		reply_error(SYSID_ERR_OP_UNSUPPORTED, dlerror(), reply_to);
	else
		(*do_form)(title, text, fields, nfields, reply_to);
}

/*ARGSUSED*/
void
dl_do_confirm(
	char		*title,
	char		*text,
	Field_desc	*fields,
	int		nfields,
	int		reply_to)
{
	Form_proc	*do_confirm;

	do_confirm = (Form_proc *)dlsym(dl_handle, "do_confirm");
	if (do_confirm == (Form_proc *)0)
		reply_error(SYSID_ERR_OP_UNSUPPORTED, dlerror(), reply_to);
	else
		(*do_confirm)(title, text, fields, nfields, reply_to);
}

void
dl_get_terminal(MSG *mp, int reply_to)
{
	void	(*do_get_terminal)(MSG *, int);

	do_get_terminal = (void(*)(MSG *, int))
				dlsym(dl_handle, "do_get_terminal");
	if (do_get_terminal == (void(*)(MSG *, int))0)
		reply_error(SYSID_ERR_OP_UNSUPPORTED, dlerror(), reply_to);
	else
		(*do_get_terminal)(mp, reply_to);
}

/*ARGSUSED*/
void
dl_get_password(MSG *mp, int reply_to)
{
	void	(*do_get_password)(MSG *, int);

	do_get_password = (void(*)(MSG *, int))
				dlsym(dl_handle, "do_get_password");
	if (do_get_password == (void(*)(MSG *, int))0)
		reply_error(SYSID_ERR_OP_UNSUPPORTED, dlerror(), reply_to);
	else
		(*do_get_password)(mp, reply_to);
}

/*ARGSUSED*/
void
dl_get_timezone(
	char		*timezone,
	Field_desc	*regions,
	Field_desc	*gmt_offset,
	Field_desc	*tz_filename,
	int		reply_to)
{
	TZ_proc	*do_get_timezone;

	do_get_timezone = (TZ_proc *)dlsym(dl_handle, "do_get_timezone");
	if (do_get_timezone == (TZ_proc *)0)
		reply_error(SYSID_ERR_OP_UNSUPPORTED, dlerror(), reply_to);
	else
		(*do_get_timezone)(
			timezone,
			regions,
			gmt_offset,
			tz_filename,
			reply_to);
}

void
dl_get_locale(
	char		*locale,
	Field_desc	*languages,
	int		reply_to)
{
	LL_proc	*do_get_locale;

	do_get_locale = (LL_proc *)dlsym(dl_handle, "do_get_locale");
	if (do_get_locale == (LL_proc *)0)
		reply_error(SYSID_ERR_OP_UNSUPPORTED, dlerror(), reply_to);
	else
		(*do_get_locale)(
			locale,
			languages,
			reply_to);
}

char *
dl_get_err_string(Sysid_err errcode, int nargs, ...)
{
	va_list	ap;
	char	*(*do_get_err_string)(Sysid_err errcode, int, va_list);
	char	*errstr;

	do_get_err_string = (char *(*)(Sysid_err, int, va_list))
				dlsym(dl_handle, "get_err_string");
	if (do_get_err_string == (char *(*)(Sysid_err, int, va_list))0)
		return (NO_ERROR_FUNC);
	else {
		va_start(ap, nargs);
		errstr = (*do_get_err_string)(errcode, nargs, ap);
		va_end(ap);

		return (errstr);
	}
}

char *
dl_get_attr_title(Sysid_attr attr)
{
	char	*(*do_get_attr_title)(Sysid_attr attr);

	do_get_attr_title = (char *(*)(Sysid_attr))
				dlsym(dl_handle, "get_attr_title");
	if (do_get_attr_title == (char *(*)(Sysid_attr))0)
		return (NO_TITLE_FUNC);
	else
		return ((*do_get_attr_title)(attr));
}

char *
dl_get_attr_text(Sysid_attr attr)
{
	char	*(*do_get_attr_text)(Sysid_attr attr);

	do_get_attr_text = (char *(*)(Sysid_attr))
				dlsym(dl_handle, "get_attr_text");
	if (do_get_attr_text == (char *(*)(Sysid_attr))0)
		return (NO_TEXT_FUNC);
	else
		return ((*do_get_attr_text)(attr));
}

char *
dl_get_attr_prompt(Sysid_attr attr)
{
	char	*(*do_get_attr_prompt)(Sysid_attr attr);

	do_get_attr_prompt = (char *(*)(Sysid_attr))
				dlsym(dl_handle, "get_attr_prompt");
	if (do_get_attr_prompt == (char *(*)(Sysid_attr))0)
		return (NO_PROMPT_FUNC);
	else
		return ((*do_get_attr_prompt)(attr));
}

char *
dl_get_attr_name(Sysid_attr attr)
{
	char	*(*do_get_attr_name)(Sysid_attr attr);

	do_get_attr_name = (char *(*)(Sysid_attr))
				dlsym(dl_handle, "get_attr_name");
	if (do_get_attr_name == (char *(*)(Sysid_attr))0)
		return (NO_NAME_FUNC);
	else
		return ((*do_get_attr_name)(attr));
}

Field_help *
dl_get_attr_help(Sysid_attr attr, Field_help *help)
{
	Field_help	*(*do_get_attr_help)(Sysid_attr, Field_help *);

	do_get_attr_help = (Field_help *(*)(Sysid_attr, Field_help *))
				dlsym(dl_handle, "get_attr_help");
	if (do_get_attr_help == (Field_help *(*)(Sysid_attr, Field_help *))0)
		return ((Field_help *)0);
	else
		return ((*do_get_attr_help)(attr, help));
}
