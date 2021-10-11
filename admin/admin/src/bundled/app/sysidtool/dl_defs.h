#ifndef DL_DEFS_H
#define	DL_DEFS_H

/*
 * Copyright (c) 1993-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)dl_defs.h 1.4 96/02/05"

extern Sysid_err	dl_init(char *, int *, char **, int, int);

extern void		dl_do_cleanup(char *, int *);
extern void		dl_do_message(MSG *, int);
extern void		dl_do_dismiss(MSG *, int);
extern void		dl_do_error(char *, int);
extern Form_proc	dl_do_form;
extern Form_proc	dl_do_confirm;

extern void		dl_get_terminal(MSG *, int);
extern void		dl_get_password(MSG *, int);
extern TZ_proc		dl_get_timezone;
extern LL_proc		dl_get_locale;

extern char		*dl_get_err_string(Sysid_err);
extern char		*dl_get_attr_title(Sysid_attr);
extern char		*dl_get_attr_text(Sysid_attr);
extern char		*dl_get_attr_prompt(Sysid_attr);
extern char		*dl_get_attr_name(Sysid_attr);
extern Field_help	*dl_get_attr_help(Sysid_attr, Field_help *);

#endif	/* !DL_DEFS_H */
