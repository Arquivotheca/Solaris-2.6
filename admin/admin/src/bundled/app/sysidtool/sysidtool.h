#ifndef _SYSIDTOOL_
#define	_SYSIDTOOL_

/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)sysidtool.h	1.69 96/10/17"

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <locale.h>
#include <netdb.h>

#define	DEBUGFILE	"/var/sadm/system/logs/sysidtool.log"

#define	NETINIT		1
#define	NISINIT		2
#define	SYSINIT		3

#define	SUCCESS		0
#define	FAILURE		1

#ifndef	FALSE
#define	FALSE   (0)
#endif

#ifndef	TRUE
#define	TRUE    (1)
#endif

#define	TIMEHOST	"timehost"
#define	NO_DOMAIN	""
#define	DISPATCHFLAG	B_TRUE
#define	DEFAULT_NETMASK	"255.255.255.0"

#define	LOOPBACK_IP	"127.0.0.1"
#define	LOCAL_HOST	"localhost"
#define	LOG_HOST	"loghost"

#define	HOSTS_TBL	0
#define	NETMASKS_TBL	1
#define	TIMEZONE_TBL	2
#define	LOCALE_TBL	3
#define	PASSWD_TBL	4
#define	ETHERS_TBL	5
#define	BOOTPARAMS_TBL	6

#define	UFS		0
#define	NIS		1
#define	NISPLUS		2

#define	MAX_DOMAINNAME	256
#define	MAX_YEAR	4
#define	MAX_MONTH	2
#define	MAX_DAY		2
#define	MAX_HOUR	2
#define	MAX_MINUTE	2
#define	MAX_HOSTNAME	MAXHOSTNAMELEN
#define	MAX_IPADDR	15
#define	MAX_NETMASK	15
#define	MAX_PASSWORD	MAXPATHLEN
#define	MAX_TZ		MAXPATHLEN
#define	MAX_GMT_OFFSET	3
#define	MAX_LOCALE	20	/* 3 should be enough */
#define	MAX_LANG	20	/* 3 should be enough */
#define	MAX_TERM	256	/* is this sufficient??? */
#define	MAX_NETHW	4

typedef enum attr_type {
	ATTR_NONE,
	ATTR_LANG,
	ATTR_NLANGS,
	ATTR_LOCALE,
	ATTR_NLOCALES,
	ATTR_LOCALEPICK,
	ATTR_LOC_LANG,
	ATTR_LOC_INDEX,
	ATTR_TERMINAL,
	ATTR_HOSTNAME,
	ATTR_NETWORKED,
	ATTR_PRIMARY_NET,
	ATTR_HOSTIP,
	ATTR_CONFIRM,
	ATTR_NAME_SERVICE,
	ATTR_DOMAIN,
	ATTR_BROADCAST,
	ATTR_NISSERVERNAME,
	ATTR_NISSERVERADDR,
	ATTR_SUBNETTED,
	ATTR_NETMASK,
	ATTR_BAD_NIS,
	ATTR_TIMEZONE,		/* timezone string */
	ATTR_TZ_REGION,		/* index into region menu */
	ATTR_TZ_GMT,		/* offset from GMT */
	ATTR_TZ_FILE,		/* timezone rules file */
	ATTR_TZ_INDEX,		/* index into timezone menu */
	ATTR_DATE_AND_TIME,
	ATTR_YEAR,
	ATTR_MONTH,
	ATTR_DAY,
	ATTR_HOUR,
	ATTR_MINUTE,
	ATTR_PASSWORD,
	ATTR_EPASSWORD,
	ATTR_PROMPT,		/* arg is opaque prompt handle */
	ATTR_STRING,
	ATTR_DOEXIT,
	ATTR_ERROR,
	ATTR_INDEX,
	ATTR_ARG,
	ATTR_SIZE
} Sysid_attr;

typedef enum error_codes {
	SYSID_SUCCESS = 0,
	SYSID_ERR_TTY_INIT,
	SYSID_ERR_XM_INIT,
	SYSID_ERR_IPADDR_MAX,
	SYSID_ERR_IPADDR_MIN,
	SYSID_ERR_IPADDR_RANGE,
	SYSID_ERR_IPADDR_UNSPEC,
	SYSID_ERR_IPADDR_FMT,
	SYSID_ERR_HOSTNAME_CHARS,
	SYSID_ERR_HOSTNAME_MINUS,
	SYSID_ERR_HOSTNAME_LEN,
	SYSID_ERR_NETMASK_FMT,
	SYSID_ERR_NETMASK_RANGE,
	SYSID_ERR_DOMAIN_LEN,
	SYSID_ERR_BAD_YP_BINDINGS1,
	SYSID_ERR_BAD_YP_BINDINGS2,
	SYSID_ERR_BAD_NIS_SERVER1,
	SYSID_ERR_BAD_NIS_SERVER2,
	SYSID_ERR_NIS_SERVER_ACCESS,
	SYSID_ERR_STORE_NETMASK,
	SYSID_ERR_BAD_NETMASK,
	SYSID_ERR_BAD_DOMAIN,
	SYSID_ERR_NSSWITCH_FAIL1,
	SYSID_ERR_NSSWITCH_FAIL2,
	SYSID_ERR_BAD_NISSERVER_ENT,
	SYSID_ERR_BAD_YP_ALIASES,
	SYSID_ERR_NO_NETMASK,
	SYSID_ERR_BAD_TIMEZONE,
	SYSID_ERR_BAD_DATE,
	SYSID_ERR_BAD_YEAR,
	SYSID_ERR_GET_ETHER,
	SYSID_ERR_BAD_ETHER,
	SYSID_ERR_BAD_BOOTP,
	SYSID_ERR_BAD_NETMASK_ENT,
	SYSID_ERR_BAD_TIMEZONE_ENT,
	SYSID_ERR_CANT_DO_PASSWORD_PLUS,
	SYSID_ERR_CANT_DO_PASSWORD,
	SYSID_ERR_CANT_DO_KEYLOGIN,
	SYSID_ERR_BAD_NODENAME,
	SYSID_ERR_BAD_LOOPBACK,
	SYSID_ERR_BAD_HOSTS_ENT,
	SYSID_ERR_BAD_IP_ADDR,
	SYSID_ERR_BAD_UP_FLAG,
	SYSID_ERR_NO_IPADDR,
	SYSID_ERR_BAD_DIGIT,
	SYSID_ERR_MIN_VALUE_EXCEEDED,
	SYSID_ERR_MAX_VALUE_EXCEEDED,
	SYSID_ERR_BAD_TZ_FILE_NAME,
	SYSID_ERR_NO_VALUE,
	SYSID_ERR_DLOPEN_FAIL,
	SYSID_ERR_XTINIT_FAIL,
	SYSID_ERR_NO_SELECTION,
	SYSID_ERR_OP_UNSUPPORTED
} Sysid_err;

typedef	struct confirm_data	Confirm_data;
struct	confirm_data {
	Sysid_attr field_attr;	/* Tag to place on field for display */
	char	*value;		/* Value of field */
	u_long	flags;		/* flag/status bits - see below */
};

#define	AUTO_DETERMINED		0x1

#define	MAXNETHW	4

/* list of network interfaces that are present on the system */
struct if_list {
	char		name[MAXNETHW+2];
	struct if_list	*next;
};

extern char *mb_locales[];

extern	void	status(void);
extern	int		do_locale(void);
extern	int		do_locale_simulate(int, char **);
extern	void	save_locale(char *);
extern  int 	get_num_locales(void);
extern  int 	get_lang_strings(char ***);
extern  void	free_lang_strings(char ***);
extern  int		get_net_if_list(struct if_list **);
extern  int 	get_lang_locale_strings(char *, char ***);
extern  void	free_lang_locale_strings(char ***);
extern	int	get_state(int *sys_configured, int *sys_bootparamed,
    int *sys_networked, int *sys_autobound, int *sys_subnetted,
    int *sys_passwd, int *sys_locale, char *termtype, int *err_num);
extern	void	put_state(int sys_configured, int sys_bootparamed,
    int sys_networked, int sys_autobound, int sys_subnetted,
    int sys_passwd, int sys_locale, char *termtype);
extern	void	get_net_domainname(char *);
extern	int	set_root_password(char *, char *);
extern	int	set_date(char *, char *, char *);
extern	int	set_rdate();
extern	int	set_domainname(char *, char *);
extern	int	set_ent_hosts(char *, char *, char *, char *);
extern	int	set_ent_netmask(char *, char *, char *);
extern	int	get_net_hostname(char *);
extern	int	set_net_hostname(char *, char *, char *, int);
extern	int	get_net_if_name(char *);
extern	void	get_net_name_num(char *, char *);
extern	int	set_net_ifup(char *);
extern	int	get_net_ipaddr(char *, char *);
extern	int	set_net_ipaddr(char *, char *);
extern	int	set_net_netmask(char *, char *, char *);
extern	int	nodename_set();
extern	int	get_entry(int, int, char *, char *);
extern	int	conv_ns(char *);
extern	int	bootargset(char *);
extern	int	do_netmask(char *, char *, char *);
extern	int	free_vm();
extern	int	set_env_timezone(char *, char *);
extern	int	init_ns(char *, char *, int, char *, char *, char *);
extern	int	autobind(char *, char *, char *, char *);
extern	int	init_yp_aliases(char *, char *);
extern	int	init_yp_binding(char *, int, char *, char *);
extern	int	init_nis_plus(int, char *, char *);
extern	int	setup_nsswitch(char *, char *);
extern	int	lookup_locale();
extern	int	get_l10n_string(int, char *, char *, char *, int);
extern	int	read_locale_file(FILE *, char *, char *, char *, char *,
			char *, char *, char *);
extern	void	set_lang(char *);
extern	void	halt();
extern	void	kill_ypbind();
extern	void	log_time();
extern	int	unconfig_files();
extern	int	run(char *const argv[]);
extern	FILE	*open_log(char *name);
extern	void	system_namesrv(char *, char *);

extern	int	ws_is_cons(char *);

extern 	int	sysid_valid_host_ip_addr(const char *);
extern 	int	sysid_valid_ip_netmask(const char *);
extern 	int	sysid_valid_domainname(const char *);
extern 	int	sysid_valid_hostname(const char *);
extern 	int	sysid_valid_timezone(const char *);
extern 	int	sysid_valid_system_locale(const char *);
extern 	int	sysid_valid_install_locale(const char *);
extern 	int	sysid_valid_network_interface(const char *);
extern 	int	sysid_valid_terminal(const char *);
extern 	int	sysid_valid_passwd(const char *);

#ifdef notdef
extern	int	do_menu2(char *, Menu_desc *, int, int);
extern	int	do_menu(Sysid_msg, Menu_desc *, int, int);
extern	void	do_form(Sysid_msg, Field_desc *, int, int, void (*)(FORM *));
extern	char	**format_message(char *, int);
extern	void	print_message(char *, int, int, int *);
extern	void	print_message_args(Sysid_msg, int, int *, ...);
extern	int	set_publickey(char *);
extern	void	start_curses();
extern	void	welcome();
extern	void	end_curses();
extern	void	goodbye();
extern	int	get_default_msg_width();
extern	void	set_default_msg_width(int);
extern	int	inetplumb(char *);
extern	int	dlpi_open_attach(char *);

#ifdef __STDC__
#define	TAG(m)		(SYSID_##m)
#else
#define	TAG(m)		(SYSID_/**/m)
#endif

#define	MSG(i)	\
	dgettext(SYSID_MSGS_TEXTDOMAIN, sysid_msgs[TAG(i)-SYSID_MSGS_BASE])
#define	SMSG(i)	\
	dgettext(SYSID_MSGS_TEXTDOMAIN, sysid_msgs[i-SYSID_MSGS_BASE])

#endif

/* unconfig_files error returns */

#define	ADMUTIL_UNCONF_TZ	-1	/* timezone */
#define	ADMUTIL_UNCONF_COLD	-2	/* unlink coldstart */
#define	ADMUTIL_UNCONF_DOM	-3	/* get_domain */
#define	ADMUTIL_UNCONF_YP	-4	/* yp cleanup */
#define	ADMUTIL_UNCONF_NS	-5	/* nsswitch */
#define	ADMUTIL_UNCONF_DFD	-6	/* defaultdomain */
#define	ADMUTIL_UNCONF_DFR	-7	/* default router */
#define	ADMUTIL_UNCONF_NTM	-8	/* netmasks */
#define	ADMUTIL_UNCONF_NN	-9	/* nodename */
#define	ADMUTIL_UNCONF_SI	-10	/* sysinfo */
#define	ADMUTIL_UNCONF_LB	-11	/* remove_lb_ntoa_entry */
#define	ADMUTIL_UNCONF_SH	-12	/* save hosts file */
#define	ADMUTIL_UNCONF_IF	-13	/* get_net_if_names */
#define	ADMUTIL_UNCONF_OP	-14	/* open */
#define	ADMUTIL_UNCONF_PW	-15	/* passwd */
#define	ADMUTIL_UNCONF_HF	-16	/* hosts files */
#define	ADMUTIL_UNCONF_PF	-17	/* sysid configuration file */

/* init_ns error returns */
#define	INIT_NS_OK	0
#define	INIT_NS_SWITCH	1
#define	INIT_NS_DOMAIN	2
#define	INIT_NS_HOSTS	3
#define	INIT_NS_ALIASES	4
#define	INIT_NS_BIND	5
#define	INIT_NS_NISP	6
#define	INIT_NS_NISP_ACC 7
#define	INIT_NS_YPSRV	8

/* get_entry error returns */
#define	NS_SUCCESS	0
#define	NS_NOTFOUND	1
#define	NS_TRYAGAIN	-1

#include "sysidtool_test.h"
#include <stdio.h>
extern FILE *debugfp;

#endif /* _SYSIDTOOL_ */
