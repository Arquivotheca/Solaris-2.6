/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)sysman_iface.h	1.22	96/01/14 SMI"

#ifndef	_SYSMAN_IFACE_H
#define	_SYSMAN_IFACE_H


#include <sys/types.h>
#include "sysman_types.h"
#include "sysman_codes.h"


/* interfaces to user account functions */

extern int	sysman_add_user(
		    SysmanUserArg	*ua_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_delete_user(
		    SysmanUserArg	*ua_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_modify_user(
		    SysmanUserArg	*ua_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_get_user(
		    SysmanUserArg	*ua_p,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_user(SysmanUserArg *ua_p);

extern int	sysman_list_user(
		    SysmanUserArg	**ua_p,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_user_list(SysmanUserArg *ua_p, int cnt);

extern uid_t	sysman_get_next_avail_uid(void);


/* interfaces to group account functions */

extern int	sysman_add_group(
		    SysmanGroupArg	*ga_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_delete_group(
		    SysmanGroupArg	*ga_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_modify_group(
		    SysmanGroupArg	*ga_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_get_group(
		    SysmanGroupArg	*ga_p,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_group(SysmanGroupArg *ga_p);

extern int	sysman_list_group(
		    SysmanGroupArg	**ga_p,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_group_list(SysmanGroupArg *ga_p, int cnt);

extern gid_t	sysman_get_next_avail_gid(void);


/* interfaces to host table mgt functions */

extern int	sysman_add_host(
		    SysmanHostArg	*ha_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_delete_host(
		    SysmanHostArg	*ha_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_modify_host(
		    SysmanHostArg	*ha_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_get_host(
		    SysmanHostArg	*ha_p,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_host(SysmanHostArg *ha_p);

extern int	sysman_list_host(
		    SysmanHostArg	**ha_p,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_host_list(SysmanHostArg *ha_p, int cnt);


/* interfaces to software pkg mgt functions */

extern void	sysman_sw_do_gui(
		    boolean_t		do_gui,
		    const char		*display_string);

extern int	sysman_sw_start_script(void);

extern int	sysman_sw_add_cmd_to_script(
		    int			fd,
		    SysmanSWArg		*swa_p);

extern int	sysman_sw_finish_script(int fd);

extern int	sysman_add_sw_by_script(
		    char		*buf,
		    int			bufsiz);

extern int	sysman_add_sw(
		    SysmanSWArg		*swa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_delete_sw(
		    SysmanSWArg		*swa_p,
		    char		*buf,
		    int			bufsiz);


/* interfaces to serial port mgt functions */

extern int	sysman_modify_serial(
		    SysmanSerialArg	*sa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_enable_serial(
		    SysmanSerialArg	*sa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_disable_serial(
		    SysmanSerialArg	*sa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_delete_serial(
		    SysmanSerialArg	*sa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_get_serial(
		    SysmanSerialArg	*sa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_get_alt_serial(
		    SysmanSerialArg	*sa_p,
		    const char		*alt_dev_dir,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_serial(SysmanSerialArg *sa_p);

extern int	sysman_list_serial(
		    SysmanSerialArg	**sa_pp,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_list_alt_serial(
		    SysmanSerialArg	**sa_pp,
		    const char		*alt_dev_dir,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_serial_list(
		    SysmanSerialArg	*sa_p,
		    int			cnt);


/* interfaces to printer mgt functions */

extern int	sysman_add_local_printer(
		    SysmanPrinterArg	*pa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_add_remote_printer(
		    SysmanPrinterArg	*pa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_delete_printer(
		    SysmanPrinterArg	*pa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_modify_local_printer(
		    SysmanPrinterArg	*pa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_modify_remote_printer(
		    SysmanPrinterArg	*pa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_list_printer_devices(
		    char		***devices_pp,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_printer_devices_list(
		    char		**devices_p,
		    int			cnt);

extern int	sysman_get_default_printer_name(
		    char		**printer,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_get_printer(
		    SysmanPrinterArg	*pa_p,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_printer(SysmanPrinterArg *sa_p);

extern int	sysman_list_printer(
		    SysmanPrinterArg	**pa_pp,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_printer_list(
		    SysmanPrinterArg	*sa_p,
		    int			cnt);


/* interfaces to mail alias functions */

extern int	sysman_add_alias(
		    SysmanAliasArg	*aa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_delete_alias(
		    SysmanAliasArg	*aa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_modify_alias(
		    SysmanAliasArg	*aa_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_get_alias(
		    SysmanAliasArg	*aa_p,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_alias(SysmanAliasArg *aa_p);

extern int	sysman_list_alias(
		    SysmanAliasArg	**aa_p,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_alias_list(SysmanAliasArg *aa_p, int cnt);


/* interfaces to job scheduling (cron, at)  functions */

extern int	sysman_add_jobsched(
		    SysmanJobschedArg	*ja_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_delete_jobsched(
		    SysmanJobschedArg	*ja_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_modify_jobsched(
		    SysmanJobschedArg	*ja_p,
		    char		*buf,
		    int			bufsiz);

extern int	sysman_get_jobsched(
		    SysmanJobschedArg	*ja_p,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_jobsched(SysmanJobschedArg *ja_p);

extern int	sysman_list_jobsched(
		    SysmanJobschedArg	**ja_p,
		    char		*buf,
		    int			bufsiz);

extern void	sysman_free_jobsched_list(SysmanJobschedArg *ja_p, int cnt);


/* interfaces to utility functions */

extern boolean_t	in_admin_group_p(uid_t uid);

extern int		check_ns_user_conflicts(
			    const char	*username,
			    uid_t	uid);

extern int		check_ns_group_conflicts(
			    const char	*groupname,
			    gid_t	gid);

extern int		check_ns_host_conflicts(
			    const char	*hostname,
			    const char	*ip_addr);

extern char *		cron_list_to_field_string(j_time_elt_t *list);

extern j_time_elt_t *	field_string_to_malloc_cron_list(const char *str);

extern void		free_malloc_cron_list(j_time_elt_t *l);

extern int		field_string_to_cron_list(
			    const char		*str,
			    j_time_elt_t	*t_p,
			    int			t_p_len);


#endif /* _SYSMAN_IFACE_H */
