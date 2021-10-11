/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)printer_impl.h	1.8	95/09/11 SMI"

#ifndef _PRINTER_IMPL_H
#define _PRINTER_IMPL_H

#include "printer_iface.h"

/* from cl_printer_parms.h */
#define PRT_DEF_PRINTER_TYPE_NEWSPRINT	"NeWSprint"
#define	PRT_DEF_PRINTER_TYPE_PS		"PS"
#define	PRT_ALTERNATE_PORTS_PAR		"alternate_ports"

/* from cl_printer_impl.h */
#define PRT_MAXSTRLEN 256
#define PRT_UNIVERSAL_ADDR_SIZE	33

#define PRT_SUCCESS		0
#define PRT_METHODFAIL		1

#define	PRT_LPSYSTEM		"/usr/sbin/lpsystem"
#define	PRT_LPSYSTEM_A		"/usr/sbin/lpsystem -A"
#define	PRT_LPSYSTEM_L		"/usr/sbin/lpsystem -l"
#define PRT_LPFILTER		"/usr/sbin/lpfilter"
#define	PRT_LPADMIN		"/usr/sbin/lpadmin"
#define PRT_SACADM		"/usr/sbin/sacadm"
#define PRT_PMADM		"/usr/sbin/pmadm"
#define PRT_TCP_PORTMON		"tcp"
#define	PRT_CHOWN		"/bin/chown"
#define	PRT_CHMOD		"/bin/chmod"


#define	PRT_LPSTAT_ACCEPT	"/bin/lpstat -L -a"
#define	PRT_LPSTAT_LIST		"/bin/lpstat -L -p -D"
#define	PRT_LPSTAT_LIST_NODESC	"/bin/lpstat -L -p"
#define	PRT_LPSTAT_LIST_DEVS	"/bin/lpstat -L -s"
#define	PRT_LPSTAT_VIEW		"/bin/lpstat -L -D -l -p"
#define	PRT_LPSTAT_VIEW_DEFAULT	"/bin/lpstat -L -d"
#define PRT_SACADM_LIST		"/usr/sbin/sacadm -l"
#define PRT_PMADM_LIST		"/usr/sbin/pmadm -l"
#define	PRT_SAC_TCP_CREATE	"/usr/sbin/sacadm -a -p tcp -t listen -c \"/usr/lib/saf/listen tcp\" -v `/usr/sbin/nlsadmin -V`"

#define PRT_PM_ADD		"/usr/sbin/pmadm -a -p tcp -s %s -i root"
#define	PRT_PM_LS5		PRT_PM_ADD" -m `/usr/sbin/nlsadmin -o /var/spool/lp/fifos/listenS5` -v `/usr/sbin/nlsadmin -V`"
#define	PRT_PM_LBSD		PRT_PM_ADD" -m `/usr/sbin/nlsadmin -o /var/spool/lp/fifos/listenBSD -A '\\x%s'` -v `/usr/sbin/nlsadmin -V`"

#define	PRT_PM_L0		PRT_PM_ADD" -m `/usr/sbin/nlsadmin -c /usr/lib/saf/nlps_server -A '\\x%s'` -v `/usr/sbin/nlsadmin -V`"

#define PRT_PM_VERSION		" -v `/usr/sbin/nlsadmin -V`"

#define	PRT_FILTERDIR		"/etc/lp/fd"

#define	PRT_LP			"lp"
#define PRT_LPD			"lpd"
#define	PRT_SVC0		"0"

#define	PRT_PORT_ACLS		0600

#define	PRT_LINEPRINTER_PORT_NUM	"0203"
#define	PRT_SERVICE0_PORT_NUM		"0ACE"

#define	PRT_DEF_RESTARTS		"999"

#define	PRT_LISTENBSD		"listenBSD"
#define	PRT_LISTENS5		"listenS5"
#define PRT_LISTEN0		"nlps_server"


#define	PRT_LISTING_PRINTER	"printer"
#define PRT_LISTING_BOGUS_PRT	"new printer"
#define	PRT_LISTING_DESCRIPTION	"Description:"
#define	PRT_PRINTER_PATH	"/etc/lp/printers"
#define	PRT_REMOTE_KEYWORD	"system for"
#define	PRT_LOCAL_KEYWORD	"device for"
#define	PRT_AS_PRINTER_KEYWORD	"(as printer"
#define	PRT_VIEW_PRINTER_TYPE	"Printer types"
#define PRT_VIEW_PRINTER_INTERFACE	"Interface"
#define PRT_INTERFACE_NEWSPRINT	"/etc/lp/model/newsprint"
#define	PRT_VIEW_CONTENT_TYPE	"Content types"
#define	PRT_VIEW_USERS		"Users allowed"
#define PRT_VIEW_BANNER		"Banner"
#define	PRT_VIEW_FAULT_NOT	"On fault"
#define	PRT_VIEW_DEFAULT	"system default destination:"
#define	PRT_VIEW_USERS_ALL	"(all)"
#define	PRT_VIEW_USERS_NONE	"(none)"
#define	PRT_VIEW_ENABLED	"enabled since"
#define	PRT_VIEW_REJECTED	"not accepting requests"
#define PRT_SYSTEM_TYPE		"Type:"
#define PRT_VIEW_TYPE_BSD	"bsd"
#define	PRT_BANNER_NOT_REQUIRED		"not_required"
#define	PRT_FAULT_MAIL			"mail"
#define	PRT_FAULT_WRITE			"write"

#define	PRT_PRINTER_NOT_FOUND	1
#define	PRT_PRINTER_FOUND	2
#define	PRT_PRINTER_ERROR	3
/* returns from setup_network_access: */
#define	PRT_L0_CREATE_FAILED		1
#define	PRT_SAC_LIST_FAILED		2
#define	PRT_TCP_PM_CREATE_FAILED	3
#define	PRT_PM_LIST_FAILED		4
#define	PRT_LS5_CREATE_FAILED		5
#define	PRT_LPSYSTEM_FAILED		6
#define	PRT_LBSD_CREATE_FAILED		7


/* returns from do_user_list */
#define	PRT_DIR_OPEN_FAILED		1
#define	PRT_OPEN_USERS_FAILED		2
#define	PRT_MALLOC_FAILED		3

#define	PRT_SERIAL_PORT_DIRECTORY	"/dev/term"
#define	PRT_DEVICE_DIRECTORY		"/dev"
#define	PRT_DEV_PATH_LEN		20

#define PARENT_DIRECTORY	".."
#define CURRENT_DIRECTORY	"."


/* stuff for nisplus table manipulation */
#define	PRT_NIS_TABLE_NAME		"printers.org_dir"
#define PRT_NIS_EXISTING_TABLE_NAME	"hosts.org_dir"
#define	PRT_NIS_TABLE_COLUMN_NUMBER	3
#define	PRT_NIS_TABLE_SEPARATOR		'\t'
#define	PRT_DOTDOMAIN			"."


#define PRINT_CLIENT_SW_SHARED_LIB	"/usr/lib/libprint.so"


extern int	print_client_installed_p(void);
extern int	verify_unique_printer_name(const char *printername);

#endif /*_PRINTER_IMPL_H */
