/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)serial_iface.h	1.8	95/05/19 SMI"

#ifndef _SERIAL_IFACE_H
#define _SERIAL_IFACE_H

#include <limits.h>


/* Error codes returned by the library */

#define SERIAL_SUCCESS			0
#define SERIAL_FAILURE			1
#define SERIAL_ERR_IS_CONSOLE		-11
#define SERIAL_ERR_ADD_SACADM		-12
#define SERIAL_ERR_ENABLE_PORTMON	-13
#define SERIAL_ERR_STARTING_PORTMON	-14
#define SERIAL_ERR_CHECK_PORTMON	-15
#define SERIAL_ERR_DO_EEPROM		-16
#define SERIAL_ERR_GET_STTYDEFS		-17
#define SERIAL_ERR_TTYADM		-18
#define SERIAL_ERR_FORK_PROC_TTYADM	-19
#define SERIAL_ERR_PMADM		-20
#define SERIAL_ERR_FORK_PROC_PMADM	-21
#define SERIAL_ERR_TTYADM_VERSION	-22
#define SERIAL_ERR_PIPE_IO		-23
#define SERIAL_ERR_EXEC_CHILD		-24
#define SERIAL_ERR_BAD_INPUT		-25

#define M_MALLOC_FAIL	-1
#define M_OPENDIR_FAIL	-2

#define PMADM_INFO_BUF	1024

#define	NUM_FIELDS_IN_PMADM_OUTPUT 20

typedef struct s_modem_info {
	char	port[PATH_MAX + 1];
	char	pmadm_info[PMADM_INFO_BUF];
} ModemInfo;

typedef struct s_pmtab_info {
	const char	*pmtag_key;	/* existing pmtag for lookup */
	const char	*svctag_key;	/* existing svctag for lookup */
	const char	*pmtag;		/* portmon tag			 */
	const char	*pmtype;	/* portmon type (must be 'ttymon') */
	const char	*svctag;	/* service tag	(a/b)		 */
	const char	*portflags;	/* portflags			 */
	const char	*identity;	/* id for service to run as	 */
	const char	*rsv1, *rsv2, *rsv3;	/* reserved */
	/* ttyadm fields */
	const char	*device;	/* full path name of device	 */
	const char	*ttyflags;	/* ttyflags			 */
	const char	*count;		/* wait_read count		 */
	const char	*service;	/* full service cmd line	 */
	const char	*timeout;	/* timeout for input 		 */
	const char	*ttylabel;	/* ttylabel in /etc/ttydefs	 */
	const char	*modules;	/* modules to push		 */
	const char	*prompt;	/* prompt message		 */
	const char	*disable;	/* disable message		 */
	const char	*termtype;	/* terminal type		 */
	const char	*softcar;	/* use softcarrier		 */
	const char	*comment;	/* comment			 */
	/* not returned from pmadm but handy to have */
	const char	*port;		/* the port, as in /dev/term/<port> */
} PmtabInfo;


extern int	serial_errno;

extern int	modify_modem(PmtabInfo *pi_p);
extern int	enable_modem(PmtabInfo *pi_p);
extern int	disable_modem(PmtabInfo *pi_p);
extern int	delete_modem(const char *pmtag, const char *svctag);
extern int	list_modem(ModemInfo **mi_pp, const char *alt_dev_dir);

#endif /*_SERIAL_IFACE_H */
