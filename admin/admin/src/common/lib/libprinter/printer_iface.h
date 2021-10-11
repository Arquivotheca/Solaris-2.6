/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)printer_iface.h	1.12	95/06/23 SMI"

#ifndef _PRINTER_IFACE_H
#define _PRINTER_IFACE_H


#include <sys/types.h>


/* Error codes returned by the library */

#define PRINTER_SUCCESS				0
#define PRINTER_FAILURE				1
#define PRINTER_ERR_NON_UNIQUE			-11
#define PRINTER_ERR_L0_CREATE_FAILED		-12
#define PRINTER_ERR_SAC_LIST_FAILED		-13
#define PRINTER_ERR_TCP_PM_CREATE_FAILED	-14
#define PRINTER_ERR_PM_LIST_FAILED		-15
#define PRINTER_ERR_LS5_CREATE_FAILED		-16
#define PRINTER_ERR_LPSYSTEM_FAILED		-17
#define PRINTER_ERR_LBSD_CREATE_FAILED		-18
/* popen/system() failures when running lp commands */
#define PRINTER_ERR_PIPE_FAILED			-19
#define PRINTER_ERR_SYSTEM_CMD_FAILED		-20
#define PRINTER_ERR_SYSTEM_ACCEPT_FAILED	-21
#define PRINTER_ERR_SYSTEM_ENABLE_FAILED	-22
#define PRINTER_ERR_SYSTEM_REJECT_FAILED	-23
#define PRINTER_ERR_SYSTEM_DISABLE_FAILED	-24
#define PRINTER_ERR_SYSTEM_LPSYSTEM_FAILED	-25
#define PRINTER_ERR_SYSTEM_LPADMIN_FAILED	-26
#define PRINTER_ERR_SYSTEM_LPSTAT_FAILED	-27

#define PRINTER_ERR_DIR_OPEN_FAILED		-28
#define PRINTER_ERR_MALLOC_FAILED		-29
#define PRINTER_ERR_OPEN_USERS_FAILED		-30
#define PRINTER_ERR_GET_DEFAULT_FAILED		-31
#define PRINTER_ERR_PRINTER_NAME_ERROR		-32
#define PRINTER_ERR_SERVER_NAME_ERROR		-33
#define PRINTER_ERR_COMMENT_ERROR		-34
#define PRINTER_ERR_PORT_NAME_ERROR		-35
#define PRINTER_ERR_PRINTER_TYPE_ERROR		-36
#define PRINTER_ERR_NAME_LIST_ERROR		-37

#define MAX_PRINTER_NAME_LENGTH  14

#define LOCAL_CONTEXT_NAME	"etc"

typedef struct _lpinfo_struct {
	const char	*printername;
	const char	*printertype;
	const char	*printhost; 	/* the original host machine */
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
	const char	*context;	/* "etc", "nis", or "nisplus" */
} LpInfo;


extern int	add_local_printer(LpInfo *lpi_p);
extern int	add_remote_printer(LpInfo *lpi_p);
extern int	delete_printer(LpInfo *lpi_p);
extern int	modify_local_printer(LpInfo *lpi_p);
extern int	modify_remote_printer(LpInfo *lpi_p);
extern int	do_list_devices(char ***ports);
extern void	do_free_devices(char **ports, int cnt);
extern int	get_default_printer(
		    char	**printername,
		    const char	*context);
extern int	get_host_or_device_name(
		    const char	*printername,
		    char	**hostname,
		    char	**devname);

#endif /*_PRINTER_IFACE_H */
