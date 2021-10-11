/*
 * findconf.h: Public interface to sysconfig lib
 *		"@(#)findconf.h 1.11 94/03/06"
 *
 * Copyright (c) 1983-1993 Sun Minrosystems Inc.
 *
 */

#define	DO_CONFIG "-c"
#define	DO_UNCONFIG "-u"

#ifndef NULL
#define	NULL 0
#endif

/* linked list used to hold names of apps */
typedef struct apps_ll {
	char *name;
	struct apps_ll *next;
} AppsList;

/*
 * Type is used to hold the handle to the
 * config data internals.
 */
typedef void * CFG_HANDLE;

/* prototypes */
void execAllCfgApps(char *, char *);
CFG_HANDLE open_cfg_data(char *);
AppsList *get_cfg_apps(CFG_HANDLE);
void add_cfg_app(CFG_HANDLE, char *);
void rem_cfg_app(CFG_HANDLE, char *);
void close_cfg_data(CFG_HANDLE);
void fail_error(void);
void warn_error(void);
void free_list(AppsList *);
void write_list(FILE *, AppsList *);
void init_cfg_err_msgs();

/* Globals for exception handling */
extern char *appname;
extern int mod_cfg_err;
extern int mod_cfg_arg;
extern int verbosemode;

/* global for basedir option */
extern char * basedir;

/* Error returns */
#define	MOD_CFG_SUCCESS		0
#define	MOD_CFG_NOFILE		1
#define	MOD_CFG_DUPFILE		2
#define	MOD_CFG_NOAPP		3
#define	MOD_CFG_FILEIO		4
#define	MOD_CFG_BADNAME		5
#define	MOD_CFG_EXECFAIL	6
#define	MOD_CFG_APPFAIL		7
#define	MOD_CFG_APPSIG  	8
#define	MOD_CFG_STAT		9
#define	MOD_CFG_BADHANDLE	10
#define	MOD_CFG_MEMORY		11
#define	MOD_CFG_NOTROOT		12
#define	MOD_CFG_NOTERM		13

/* This last is not a message, but a count of how many there are */
#define	MOD_CFG_MAXERRMSGS 13

extern char * mod_cfg_errmsg[];
