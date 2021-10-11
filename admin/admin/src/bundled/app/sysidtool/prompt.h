#ifndef PROMPT_H
#define	PROMPT_H	1

/*
 * Copyright (c) 1993-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)prompt.h 1.6 96/02/05"

#include "sysidtool.h"

#define	SYSID_UI_PID		".sysidtool_pid"
#define	SYSID_UI_FIFO_IN	".sysidtool_in"
#define	SYSID_UI_FIFO_OUT	".sysidtool_out"

typedef enum op_type {
	GET_LANG,
	GET_LOCALE,
	GET_TERMINAL,
	GET_HOSTNAME,
	GET_NETWORKED,
	GET_PRIMARY_NET,
	GET_HOSTIP,
	GET_CONFIRM,
	GET_NAME_SERVICE,
	GET_DOMAIN,
	GET_BROADCAST,
	GET_NISSERVERS,
	GET_SUBNETTED,
	GET_NETMASK,
	GET_BAD_NIS,	/* special case of ERROR? */
	GET_TIMEZONE,
	GET_DATE_AND_TIME,
	GET_PASSWORD,
	SET_LANG,
	SET_LOCALE,
	SET_TERM,
	DISPLAY_MESSAGE,
	DISMISS_MESSAGE,
	REPLY_OK,
	REPLY_ERROR,
	ERROR,
	CLEANUP
} Sysid_op;

typedef	void *	Prompt_t;	/* opaque prompt handle */

extern int	prompt_open(int *, char **);
extern int	prompt_close(char *, int);

extern void	prompt_locale(char *, int *, int *);
extern void	prompt_terminal(char *);
extern void	prompt_hostname(char *);
extern int	prompt_isit_standalone(int);
extern int	prompt_primary_net(char **, int, int);
extern void	prompt_hostIP(char *);
extern int	prompt_confirm(Confirm_data *, int);
extern void	prompt_error(Sysid_err, ...);
extern int	prompt_name_service(char **, int, int);
extern void	prompt_domain(char *);
extern int	prompt_broadcast(int);
extern void	prompt_nisservers(char *, char *);
extern int	prompt_isit_subnet(int);
extern void	prompt_netmask(char *);
extern int	prompt_bad_nis(Sysid_err, ...);
extern void	prompt_timezone(char *, int *, int *);
extern void	prompt_date(char *, char *, char *, char *, char *);
extern int	prompt_save_to_NIS(int);
extern void	prompt_password(char *, char *);
extern Prompt_t	prompt_message(Prompt_t, char *);
extern void	prompt_dismiss(Prompt_t);

extern void	init_display(int *, char **, char *);
extern char	*get_fifo_dir(void);

#endif /* !GET_H */
