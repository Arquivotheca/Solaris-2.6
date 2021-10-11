/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)sysman_types.h	1.20	95/12/16 SMI"

#ifndef	_SYSMAN_TYPES_H
#define	_SYSMAN_TYPES_H


#include <sys/types.h>

typedef enum { s_inactive, s_enabled, s_disabled } s_serv_state_t;

typedef enum { j_once, j_hourly, j_daily, j_weekly, j_monthly,
    j_yearly, j_mon_thru_fri, j_mon_wed_fri, j_tue_thu,
    j_freq_other } j_frequency_t;

typedef enum { j_sun = 0, j_mon, j_tue, j_wed, j_thu,
    j_fri, j_sat, j_weekday_other } j_weekday_t;

/*
 * The next three data types are used to represent crontab fields.
 * A field is either an asterisk ("wildcard"), or a list of elements,
 * where an element is either an atom (a single integer) or a range
 * of integers, representing all integer values from the low value
 * to the high value, inclusive.  Lists are represented in the crontab
 * file as a comma separated list of elements, ranges are represented
 * as a dash separated pair of integers.
 */

typedef enum { j_atom, j_range, j_wildcard } j_field_type_tag_t;

typedef struct _j_range {
	unsigned int	range_low;
	unsigned int	range_high;
} j_range_t;

typedef struct _jobsched_elt {
	j_field_type_tag_t	type_tag;
	union {
		unsigned int	atom;
		j_range_t	range;
	} value;
	struct _jobsched_elt	*next;
} j_time_elt_t;


typedef struct _user_arg_struct {
	const char	*username_key;
	const char	*username;
	const char	*passwd;
	const char	*uid;
	const char	*group;		/* group name or gid, either is ok */
	const char	*second_grps;
	const char	*comment;
	boolean_t	home_dir_flag;
	const char	*path;
	const char	*shell;
	const char	*lastchanged;
	const char	*minimum;
	const char	*maximum;
	const char	*warn;
	const char	*inactive;
	const char	*expire;
	const char	*flag;
	boolean_t	get_shadow_flag;
} SysmanUserArg;

typedef struct _group_arg_struct {
	const char	*groupname_key;
	const char	*gid_key;
	const char	*groupname;
	const char	*passwd;
	const char	*gid;
	const char	*members;
} SysmanGroupArg;

typedef struct _host_arg_struct {
	const char	*ipaddr_key;
	const char	*hostname_key;
	const char	*ipaddr;
	const char	*hostname;
	const char	*aliases;
	const char	*comment;
} SysmanHostArg;

typedef struct _sw_arg_struct {
	int		non_interactive;
	int		show_copyrights;
	const char	*root_dir;
	const char	*admin;
	const char	*device;
	const char	*response;
	const char	*spool;
	int		num_pkgs;
	const char	**pkg_names;
} SysmanSWArg;

typedef struct _serial_arg_struct {
	const char	*pmtag_key;
	const char	*svctag_key;
	const char	*port;		/* required for add */
	const char	*pmadm;		/* required for add */
	const char	*pmtag;
	const char	*pmtype;
	const char	*svctag;	/* required for add */
	const char	*identity;	/* optional for add */
	const char	*portflags;	/* optional for add */
	const char	*comment;	/* optional for add */
	const char	*ttyflags;	/* optional for add */
	const char	*modules;	/* optional for add */
	const char	*prompt;	/* optional for add */
	const char	*termtype;	/* optional for add */
	const char	*service;	/* optional for add */
	const char	*device;	/* optional for add */
	const char	*baud_rate;	/* "ttylabel", optional for add */
	const char	*timeout;	/* optional for add */
	boolean_t	softcar;
	s_serv_state_t	service_enabled;
	boolean_t	initialize_only;
	boolean_t	bidirectional;
	boolean_t	create_utmp_entry;
	boolean_t	connect_on_carrier;
} SysmanSerialArg;

typedef struct _printer_arg_struct {
	const char	*printername;
	const char	*printertype;
	const char	*printserver;
	const char	*file_contents;
	const char	*comment;
	const char	*device;
	const char	*notify;
	const char	*protocol;	/* "s5" or "bsd" */
	int		num_restarts;
	boolean_t	default_p;
	boolean_t	banner_req_p;
	boolean_t	enable_p;
	boolean_t	accept_p;
	const char	*user_allow_list;
} SysmanPrinterArg;

typedef struct _jobsched_arg_struct {
	const char	*job_id_key;
	const char	*job_key;
	boolean_t	schedule_as_root_key;
	unsigned int	year_key;
	j_time_elt_t	*month_key;			/* 1 - 12 */
	j_time_elt_t	*date_key;			/* 1 - 31 */
	j_time_elt_t	*weekday_key;			/* 0 - 6, 0 = Sun */
	j_time_elt_t	*hour_key;			/* 0 - 23 */
	j_time_elt_t	*minute_key;			/* 0 - 59 */
	j_frequency_t	frequency_key;
	const char	*job_id_return;
	const char	*job;
	boolean_t	schedule_as_root;
	unsigned int	year;				/* 1970 - 2038 */
	j_time_elt_t	*month;
	j_time_elt_t	*date;
	j_time_elt_t	*weekday;
	j_time_elt_t	*hour;
	j_time_elt_t	*minute;
	j_frequency_t	frequency;
} SysmanJobschedArg;

typedef struct _alias_arg_struct {
	const char	*alias_key;
	const char	*alias;
	const char	*expansion;
	const char	*options;
	const char	*comment;
} SysmanAliasArg;


#endif	/* _SYSMAN_TYPES_H */
