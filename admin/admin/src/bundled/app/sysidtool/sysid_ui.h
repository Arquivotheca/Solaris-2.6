#ifndef SYSID_UI_H
#define	SYSID_UI_H

/*
 * Copyright (c) 1993-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)sysid_ui.h 1.5 96/02/05"

#include "sysidtool.h"
#include "message.h"
#include "ui_form.h"

/*
 * The whole install "parade" is preceded by an
 * introductory screen.  The presence of the
 * following file flags the need to display this
 * screen.  Before prompting the user, each program
 * in the parade should verify the presence of this
 * file, display the screen (if the file is present),
 * and remove the file.
 */
#define	PARADE_INTRO_FILE	"/tmp/.run_install_intro"

#define	HELPROOT	"/usr/snadm/classes/locale"

typedef	void	(Form_proc)(char *, char *, Field_desc *, int, int);
typedef	void	(TZ_proc)(char *,
			Field_desc *, Field_desc *, Field_desc *, int);
typedef	void	(LL_proc)(char *, Field_desc *, int);
typedef	void	(Getattr_proc)(MSG *, int);
typedef	void	(Setattr_proc)(MSG *, int);

extern void	run_display(MSG *, int);

extern void	reply_error(Sysid_err, char *, int);
extern void	reply_string(Sysid_attr, char *, int);
extern void	reply_integer(Sysid_attr, int, int);
extern void	reply_void(int);
extern void	reply_fields(Field_desc *, int, int);

/*
 * Top level user-interface routines
 */
extern Getattr_proc	ui_get_hostname;
extern Getattr_proc	ui_get_hostIP;
extern Getattr_proc	ui_get_networked;
extern Getattr_proc	ui_get_primary_net_if;
extern Getattr_proc	ui_get_name_service;
extern Getattr_proc	ui_get_domain;
extern Getattr_proc	ui_get_broadcast;
extern Getattr_proc	ui_get_nisservers;
extern Getattr_proc	ui_get_bad_nis;
extern Getattr_proc	ui_get_subnetted;
extern Getattr_proc	ui_get_netmask;
extern Getattr_proc	ui_get_confirm;
extern Getattr_proc	ui_get_date;

extern void		ui_get_locale(MSG *, int);
extern void		ui_get_timezone(MSG *, int);

extern Setattr_proc	ui_set_locale;
extern Setattr_proc	ui_set_term;

extern void		ui_error(MSG *, int);
extern void		ui_cleanup(MSG *, int);

extern Validate_proc	ui_valid_host_ip_addr;
extern Validate_proc	ui_valid_hostname;
extern Validate_proc	ui_valid_integer;
extern Validate_proc	ui_valid_choice;

extern char		*tz_from_offset(char *);
extern char		*encrypt_pw(char *);

extern void		*xmalloc(size_t);
extern void		*xrealloc(void *, size_t);
extern char		*xstrdup(char *);
extern void		die(char *);

extern int		is_install_environment(void);

/*
 * Lower layer entry points into dynamic
 * library (ui-specific) interface
 */
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

extern char		*dl_get_err_string(Sysid_err, int, ...);
extern char		*dl_get_attr_title(Sysid_attr);
extern char		*dl_get_attr_text(Sysid_attr);
extern char		*dl_get_attr_prompt(Sysid_attr);
extern char		*dl_get_attr_name(Sysid_attr);
extern Field_help	*dl_get_attr_help(Sysid_attr, Field_help *);

#endif	/* !SYSID_UI_H */
