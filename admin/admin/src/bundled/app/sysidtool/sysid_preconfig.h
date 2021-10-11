
/*
 *  Copyright (c) 1996 Sun Microsystems, Inc.  All Rights Reserved.
 *  All rights reserved.
 */

/*
 * sysid_preconfig - support functions for accessing config data
 *
 * These functions are called by each of the sysid modules to read
 * in the configuration file and provide selected data from that
 * file.
 */

#pragma	ident	"@(#)sysid_preconfig.h	1.2	96/06/07 SMI"


/* function prototypes */
int read_config_file(void);
char *get_preconfig_value(int, char *, int);
void create_config_entry(int ,char *);
void create_config_attribute(int, char *,int ,char *);
void dump_config(void);
int config_entries_exist(void);

/* #define YYDEBUG 1 */

/* Configuration File Path */
#define PRECONFIG_FILE "/etc/sysidcfg"

/* Configuration File Defines */

/* wild card the value associated with a keyword */
#define CFG_DEFAULT_VALUE   NULL 

#define CFG_INSTALL_LOCALE  100
#define CFG_SYSTEM_LOCALE  101
#define CFG_TERMINAL  102
#define CFG_NAME_SERVICE  103
#define CFG_TIMEZONE 104
#define CFG_ROOT_PASSWORD 105
#define CFG_NETWORK_INTERFACE 106
#define CFG_HOSTNAME 107
#define CFG_IP_ADDRESS 108
#define CFG_NETMASK 109
#define CFG_DOMAIN_NAME 111
#define CFG_NAME_SERVER_NAME 112
#define CFG_NAME_SERVER_ADDR 113
#define CFG_KEYBOARD 114
#define CFG_LAYOUT 115
#define CFG_DISPLAY 116
#define CFG_SIZE 117
#define CFG_DEPTH 118
#define CFG_RESOLUTION 119
#define CFG_POINTER 120
#define CFG_NBUTTONS 121
#define CFG_IRQ 122
#define CFG_MONITOR 123
