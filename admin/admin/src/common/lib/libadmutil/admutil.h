/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#ifndef _ADMUTIL_H
#define	_ADMUTIL_H

#pragma ident   "@(#)admutil.h 1.14     95/05/17 SMI"

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * NIS+ host cred defs
 */
#define	NISPLUS_DES	"DES"
#define	DEF_PASSWORD	"nisplus"

int
modify_timezone(char *clientname, char *clientroot, char *timezone);

/*
 * General extern defs.
 */
extern void remove_component(char *);
extern char *basename(char *);
extern char *tempfile(const char *);
extern int trav_link(char **);
extern int lock_db(char *, int, int *);
extern int unlock_db(int *);

extern int set_timezone(char *);
extern int get_nodename(char *, char *);
extern int set_nodename(char *, int);
extern int get_domain(char *, char *);
extern int set_domain(char *, int);
extern int config_nsswitch(char *);
extern int set_net_if_ip_netmask(char *, char *);
extern int get_net_if_ip_addr(char *, char *);
extern int set_net_if_status(char *, char *, char *, char *, char *);
extern int set_lb_ntoa_entry(char *);
extern int remove_lb_ntoa_entry(char *);
/* most files don't include net/if.h, so struct ifconf is not defined */
/* extern int get_net_if_names(struct ifconf *); */
extern int set_run_level(char *);
extern int is_local_host(char *);

/* control flags for some of the functions */

#define	TE_NOW_BIT		1
#define	TE_BOOT_BIT	        2
#define	TE_NOWANDBOOT_BITS	(TE_NOW_BIT | TE_BOOT_BIT)

/* config_nsswitch */

#define	TEMPLATE_FILES		"/etc/nsswitch.files"
#define	TEMPLATE_NIS		"/etc/nsswitch.nis"
#define	TEMPLATE_NIS_PLUS	"/etc/nsswitch.nisplus"

#define	ADMUTIL_UP	"up"
#define	ADMUTIL_DOWN	"down"
#define	ADMUTIL_YES	"yes"
#define	ADMUTIL_NO	"no"

/* set_lb_ntoa_entry, remove_lb_ntoa_entry */
#define	UFS_DEFAULT_COLUMN_SEP  "\t "
#define	UFS_LB_NTOA_NAME_COL    0
#define	UFS_LB_NTOA_ADDR_COL    1

/*
 * Return codes for utility functions.
 * These functions all return 0 if they ran ok.  They return >0 if there
 * was a system problem.  The return code in this case is the errno.  They
 * return < 0 if there was an internal failure in the function.  The following
 * defines represent the failures in this case.
 */

/* set_timezone */
#define	ADMUTIL_SETTZ_BAD -1		/* invalid timezone */
#define	ADMUTIL_SETTZ_RTC -2		/* rtc failed */

/* get_nodename */
#define	ADMUTIL_GETNN_SYS -1		/* sysinfo failed */

/* set_nodename */
#define	ADMUTIL_SETNN_BAD -1		/* invalid nodename */
#define	ADMUTIL_SETNN_SYS -2		/* sysinfo failed */

/* get_domain */
#define	ADMUTIL_GETDM_SYS -1		/* sysinfo failed */

/* set_domain */
#define	ADMUTIL_SETDM_BAD -1		/* invalid domainname */
#define	ADMUTIL_SETDM_SYS -2		/* sysinfo failed */

/* config_nsswitch */
#define	ADMUTIL_SWITCH_LINK -1		/* trav_link */
#define	ADMUTIL_SWITCH_WORK -2		/* work file */
#define	ADMUTIL_SWITCH_STAT -3		/* stat nsswitch */
#define	ADMUTIL_SWITCH_TOPEN -4		/* temp file open */
#define	ADMUTIL_SWITCH_OPEN -5		/* work file open */
#define	ADMUTIL_SWITCH_WCHMOD -6	/* work file chmod */
#define	ADMUTIL_SWITCH_WCHOWN -7	/* work file chown */
#define	ADMUTIL_SWITCH_READ -8		/* nsswitch file read */
#define	ADMUTIL_SWITCH_WRITE -9		/* tempfile write */
#define	ADMUTIL_SWITCH_REN -10		/* tempfile rename */

/* set_net_if_ip_netmask */
#define	ADMUTIL_SETMASK_BAD -1		/* invalid netmask */
#define	ADMUTIL_SETMASK_SOCK -2		/* socket */
#define	ADMUTIL_SETMASK_IOCTL -3	/* ioctl */

/* get_net_if_ip_addr */
#define ADMUTIL_GETIP_SOCK -1		/* socket */
#define ADMUTIL_GETIP_IOCTL -2		/* ioctl */

/* set_net_if_status */
#define ADMUTIL_SETIFS_SOCK -1		/* socket */
#define ADMUTIL_SETIFS_IOCTL -2		/* ioctl */

/* set_lb_ntoa_entry */
#define	ADMUTIL_SETLB_BAD -1		/* invalid hostname */
#define	ADMUTIL_SETLB_CLN -2		/* fail clean */
#define	ADMUTIL_SETLB_DIRT -3		/* fail dirty */

/* remove_lb_ntoa_entry */
#define	ADMUTIL_REMLB_NET -1		/* setnetconfig */
#define	ADMUTIL_REMLB_MEM -2		/* no memory */
#define	ADMUTIL_REMLB_CLN -3		/* fail clean */
#define	ADMUTIL_REMLB_DIRT -4		/* fail dirty */

/* get_net_if_names */
#define	ADMUTIL_GETIFN_SOCK -1		/* socket */
#define	ADMUTIL_GETIFN_IOCTL -2		/* ioctl */
#define	ADMUTIL_GETIFN_MEM -3		/* malloc */

/* unconfig_files */
#define	ADMUTIL_UNCONF_TZ -1		/* timezone */
#define	ADMUTIL_UNCONF_COLD -2		/* unlink coldstart */
#define	ADMUTIL_UNCONF_DOM -3		/* get_domain */
#define	ADMUTIL_UNCONF_YP -4		/* yp cleanup */
#define	ADMUTIL_UNCONF_NS -5		/* nsswitch */
#define	ADMUTIL_UNCONF_DFD -6		/* defaultdomain */
#define	ADMUTIL_UNCONF_DFR -7		/* default router */
#define	ADMUTIL_UNCONF_NTM -8		/* netmasks */
#define	ADMUTIL_UNCONF_NN -9		/* nodename */
#define	ADMUTIL_UNCONF_SI -10		/* sysinfo */
#define	ADMUTIL_UNCONF_LB -11		/* remove_lb_ntoa_entry */
#define	ADMUTIL_UNCONF_SH -12		/* save hosts file */
#define	ADMUTIL_UNCONF_IF -13		/* get_net_if_names */
#define	ADMUTIL_UNCONF_OP -14		/* open */
#define	ADMUTIL_UNCONF_PW -15		/* passwd */
#define	ADMUTIL_UNCONF_HF -16		/* hosts files */

#ifdef  __cplusplus
}
#endif

#endif /* _ADMUTIL_H */
