/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)sysman_impl.h	1.32	96/09/09 SMI"

#ifndef _SYSMAN_IMPL_H
#define _SYSMAN_IMPL_H


#include <nl_types.h>
#include <sys/types.h>
#include "sysman_types.h"
#include "sysman_codes.h"

extern nl_catd  _catlibsysman;  /* for catgets(), defined in sysman_util.c */

/*
 * Data structures for shared mem IPC.  When an admin group user does
 * a "get" that requires the fork/setuid(0) trick, the child process
 * can't malloc memory to return to the parent process, so we need a
 * data structure that has enough space pre-allocated.
 */

typedef struct _shared_uas {
	char		username_key[32];
	char		username[32];
	char		passwd[32];
	char		uid[16];
	char		group[32];
	char		second_grps[128];
	char		comment[64];
	char		path[256];
	char		shell[32];
	char		lastchanged[32];
	char		minimum[32];
	char		maximum[32];
	char		warn[32];
	char		inactive[32];
	char		expire[32];
	char		flag[32];
} SysmanSharedUserArg;

typedef struct _shared_pas {
	char		printername[32];
	char		printertype[32];
	char		printserver[256];
	char		file_contents[256];
	char		comment[256];
	char		device[256];
	char		notify[32];
	char		protocol[32];	/* "s5" or "bsd" */
	int		num_restarts;
	boolean_t	default_p;
	boolean_t	banner_req_p;
	boolean_t	enable_p;
	boolean_t	accept_p;
	char		user_allow_list[1024];
} SysmanSharedPrinterArg;

#define	MAX_JOBSCHED_LIST	128

typedef struct _shared_jas {
	char		job_id_key[32];
	char		job_key[512];
	boolean_t	schedule_as_root_key;
	unsigned int	year_key;
	j_time_elt_t	month_key[32];
	j_time_elt_t	date_key[32];
	j_time_elt_t	weekday_key[32];
	j_time_elt_t	hour_key[32];
	j_time_elt_t	minute_key[32];
	j_frequency_t	frequency_key;
	char		job_id_return[32];
	char		job[512];
	boolean_t	schedule_as_root;
	unsigned int	year;
	j_time_elt_t	month[32];
	j_time_elt_t	date[32];
	j_time_elt_t	weekday[32];
	j_time_elt_t	hour[32];
	j_time_elt_t	minute[32];
	j_frequency_t	frequency;
} SysmanSharedJobschedArg;


/* interfaces to user account implementations */

extern int	_root_add_user(void *arg_p, char *output_buf, int len);
extern int	_root_delete_user(void *arg_p, char *output_buf, int len);
extern int	_root_modify_user(void *arg_p, char *output_buf, int len);
extern int	_get_user(
		    SysmanSharedUserArg	*arg_p,
		    char		*output_buf,
		    int			len);
extern int	_root_get_user(void *arg_p, char *output_buf, int len);
extern void	_free_user(SysmanUserArg *ua_p);
extern int	_list_user(SysmanUserArg **ua_pp, char *output_buf, int len);
extern void	_free_user_list(SysmanUserArg *ua_p, int cnt);
extern uid_t	_get_next_avail_uid(uid_t min_uid);

/* interfaces to group table mgt implementations */

extern int	_root_add_group(void *arg_p, char *output_buf, int len);
extern int	_root_delete_group(void *arg_p, char *output_buf, int len);
extern int	_root_modify_group(void *arg_p, char *output_buf, int len);
extern int	_get_group(void *arg_p, char *output_buf, int len);
extern void	_free_group(SysmanGroupArg *ga_p);
extern int	_list_group(SysmanGroupArg **ga_pp, char *output_buf, int len);
extern void	_free_group_list(SysmanGroupArg *ga_p, int cnt);
extern gid_t	_get_next_avail_gid(gid_t min_gid);

/* interfaces to host table mgt implementations */

extern int	_root_add_host(void *arg_p, char *output_buf, int len);
extern int	_root_delete_host(void *arg_p, char *output_buf, int len);
extern int	_root_modify_host(void *arg_p, char *output_buf, int len);
extern int	_get_host(void *arg_p, char *output_buf, int len);
extern void	_free_host(SysmanHostArg *ha_p);
extern int	_list_host(SysmanHostArg **ha_pp, char *output_buf, int len);
extern void	_free_host_list(SysmanHostArg *ha_p, int cnt);

/* interfaces to software pkg mgt implementations */

extern void	_sw_set_gui(boolean_t do_gui, const char *display_string);
extern char*	_start_batch_cmd(int *fd);
extern int	_add_batch_cmd(int fd, void *arg_p, char* script);
extern int	_finish_batch_cmd(int fd);
extern int	_root_add_sw_by_script(void *arg_p, char *output_buf, int len);
extern int	_root_add_sw(void *arg_p, char *output_buf, int len);
extern int	_root_delete_sw(void *arg_p, char *output_buf, int len);

/* interfaces to serial port mgr implementations */

extern int	_root_modify_serial(void *arg_p, char *buf, int len);
extern int	_root_enable_serial(void *arg_p, char *buf, int len);
extern int	_root_disable_serial(void *arg_p, char *buf, int len);
extern int	_root_delete_serial(void *arg_p, char *buf, int len);
extern int	_get_serial(
		    SysmanSerialArg	*sa_p,
		    const char		*alt_dev_dir,
		    char		*buf,
		    int			len);
extern void	_free_serial(SysmanSerialArg *sa_p);
extern int	_list_serial(
		    SysmanSerialArg	**sa_pp,
		    const char		*alt_dev_dir,
		    char		*buf,
		    int			len);
extern void	_free_serial_list(SysmanSerialArg *sa_p, int cnt);

/* interfaces to print mgt implementations */

extern int	_root_add_local_printer(void *arg_p, char *buf, int len);
extern int	_root_add_remote_printer(void *arg_p, char *buf, int len);
extern int	_root_delete_printer(void *arg_p, char *buf, int len);
extern int	_root_modify_local_printer(void *arg_p, char *buf, int len);
extern int	_root_modify_remote_printer(void *arg_p, char *buf, int len);
extern int	_list_printer_devices(char ***dev_pp, char *buf, int len);
extern void	_free_printer_devices_list(char **dev_p, int cnt);
extern int	_get_default_printer_name(char **printer, char *buf, int len);
extern int	_root_get_printer(void *arg_p, char *buf, int len);
extern void	_free_printer(SysmanPrinterArg *pa_p);
extern int	_list_printer(SysmanPrinterArg **pa_pp, char *buf, int len);
extern void	_free_printer_list(SysmanPrinterArg *pa_p, int cnt);

/* interfaces to job scheduling (at, cron) implementations */

extern int	_add_jobsched(
		    SysmanJobschedArg	*ja_p,
		    char		*output_buf,
		    int len);
extern int	_root_add_jobsched(void *arg_p, char *output_buf, int len);
extern int	_delete_jobsched(
		    SysmanJobschedArg	*ja_p,
		    char		*output_buf,
		    int			len);
extern int	_root_delete_jobsched(void *arg_p, char *output_buf, int len);
extern int	_modify_jobsched(
		    SysmanJobschedArg	*ja_p,
		    char		*output_buf,
		    int			len);
extern int	_root_modify_jobsched(void *arg_p, char *output_buf, int len);
extern int	_get_jobsched(
		    SysmanJobschedArg	*sja_p,
		    char		*output_buf,
		    int			len);
extern int	_root_get_jobsched(void *arg_p, char *output_buf, int len);
extern void	_free_jobsched(SysmanJobschedArg *ja_p);
extern int	_list_jobsched(
		    SysmanJobschedArg	**ja_pp,
		    char		*output_buf,
		    int			len);
extern int	_root_list_jobsched(
		    void		*arg_p,
		    char		*output_buf,
		    int			len);
extern void	_free_jobsched_list(SysmanJobschedArg *ja_p, int cnt);

/* interfaces to mail alias mgt implementations */

extern int	_root_add_alias(void *arg_p, char *output_buf, int len);
extern int	_root_delete_alias(void *arg_p, char *output_buf, int len);
extern int	_root_modify_alias(void *arg_p, char *output_buf, int len);
extern int	_get_alias(void *arg_p, char *output_buf, int len);
extern void	_free_alias(SysmanAliasArg *aa_p);
extern int	_list_alias(SysmanAliasArg **aa_pp, char *output_buf, int len);
extern void	_free_alias_list(SysmanAliasArg *aa_p, int cnt);


/* prototypes for "run as root" interfaces */

extern int	run_program_as_admin(
		    const char	*prgm,
		    boolean_t	interactive,
		    char	*output_buf,
		    int		len);

extern int	call_function_as_admin(
		    int		(*fn)(void *, char *, int),
		    void	*arg_struct_p,
		    int		arg_struct_len,
		    char	*output_buf,
		    int		len);


/* prototypes for utility functions */

extern void	init_err_msg(char *buf, int bufsiz, const char *msg);

extern int	cp_time_a_from_l(j_time_elt_t *a, int a_len, j_time_elt_t *l);

extern j_time_elt_t	*mk_time_l_from_a(j_time_elt_t *a);


#endif /*_SYSMAN_IMPL_H */
