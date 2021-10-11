/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

#ifndef lint
#ident	"@(#)host.h 1.4 93/04/12"
#endif

#ifndef SWM_HOST_H
#define	SWM_HOST_H

#ifdef SVR4
#include <netdb.h>
#else
#include <sys/param.h>
#endif

#ifdef XVIEW
#include <xview/xview.h>
#endif

typedef enum host_type {
	unknown,		/* don't known anything about host */
	standalone,		/* or server (has local /, /usr, etc) */
	dataless,		/* or another server's diskless client */
	diskless,		/* one of our diskless clients */
	error			/* any of a number of error conditions */
} host_t;

typedef struct host_list {
	char	*h_name;		/* name of host */
	char	*h_passwd;		/* root password for host */
	char	*h_rootdir;		/* its root directory (if diskless) */
	host_t	h_type;			/* standalone, dataless, diskless... */
	char	*h_arch;		/* architecture (if known) */
	int	h_status;		/* up/down, selected */
	struct host_list *h_next;	/* doubly linked for easy... */
	struct host_list *h_prev;	/* ...removal and reorganization */
} Hostlist;

/*
 * Host status bits
 */
#define	HOST_SELECTED	0x1	/* selected for install/remove */
#define	HOST_UP		0x2	/* host is up and reachable */
#define	HOST_MOUNTED	0x4	/* host has mounted the pkg spool directory */
#define	HOST_LOCAL	0x8	/* host file systems on this [local] system */

#define	HOST_ERROR	0x10	/* encountered error during install/remove */

/*
 * NB:  if status has been checked, one or
 * the other of PWDNONE or PWDREQ will be set
 */
#define	HOST_PWDNONE	0x100	/* no password required */
#define	HOST_PWDREQ	0x200	/* host requires password */
#define	HOST_PWDBAD	0x400	/* password is bad */
#define	HOST_PWDOK	0x800	/* password is ok */

#define	HOST_PWDBITS	(HOST_PWDNONE|HOST_PWDREQ|HOST_PWDBAD|HOST_PWDOK)

/*
 * Op-codes for host_next
 */
#define	NEXT_LOCAL	0
#define	NEXT_REMOTE	1

#define	ARCHSTR		"ARCH="
#define	GETARCH		"echo ARCH=`uname -p`.`uname -m`"
#define	MNTSTR		"MNTTAB="
#define	GETMNTTAB	"echo MNTTAB=; cat /etc/mnttab 2>&1"
#define	CLIENTROOT	"/export/root"

extern Hostlist hostlist;
extern int	hosts_selected;

/*
 * function declarations
 */
extern void host_init(char *);
extern Hostlist *host_alloc(char *);
extern void host_remove(Hostlist *);
extern Hostlist *host_insert(Hostlist *, char *);
extern void host_remove(Hostlist *);
extern void host_clear(Hostlist *);
extern int host_select(Hostlist *);
extern int host_run_cmd(Hostlist *, char *);
extern char *host_canon(char *);
extern int host_unique(Hostlist *, char *);
extern char *host_string(Hostlist *);
extern Hostlist *host_next(Hostlist *, int);
extern int check_host_passwd(Hostlist *);
extern int check_host_info(Hostlist *);

#endif	/* !SWM_HOST_H */
