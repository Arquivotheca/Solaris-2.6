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
#ident	"@(#)admin.h 1.7 95/10/20"
#endif

#ifndef SWM_ADMIN_H
#define	SWM_ADMIN_H

#include <stdio.h>
#include <sys/param.h>
#include <spmisoft_api.h>
#include "host.h"

#define	ADMIN_DEFAULT	"/var/sadm/install/admin/default"
#define	CONFIG_DEFAULT	"./.swmrc"
#define	SPOOL_DEFAULT	"/var/spool/pkg"
#define	RMNT_DEFAULT	"/tmp_mnt/nfs"

#define	MAIL_DEFAULT	""		/* don't send mail by default */

/*
 * This structure allows us to display
 * choices in localized format and write
 * out config files using C locale keywords.
 * The value field is unique to each admin
 * structure; the rest of the values are
 * shared/copied.  A value of -1 during a
 * copy operation leaves the existing value
 * unchanged.
 */
#define	MAXCHOICE	4

typedef struct admin_choice {
	int	value;			/* value of choice */
	int	nchoice;		/* the number of choice strings */
	char	*keywords[MAXCHOICE];	/* C locale keywords */
	char	*cprompts[MAXCHOICE];	/* C locale prompts */
	char	*prompts[MAXCHOICE];	/* translated prompt strings */
} Choice;

#define	MAXMAIL		1024
/*
 * The two structures used to hold administrative
 * configuration info
 */
typedef struct admin {
	char	*mail;		/* list of mail recipients */
	Choice	instance;	/* package instance already installed */
	Choice	partial;	/* allow partial installations? */
	Choice	runlevel;	/* run-level checking */
	Choice	idepend;	/* installation dependency checking */
	Choice	rdepend;	/* removal dependency checking */
	Choice	space;		/* controls space checking */
	Choice	setuid;		/* allow install of set-id programs */
	Choice	action;		/* set-id pre/postinstall scripts ok */
	Choice	conflict;	/* target file already installed */
	char	*basedir;	/* default base directory */
} Admin;

typedef struct config {
	Admin	admin;		/* stuff that goes in pkg admin file */
	Choice	showcr;		/* show copyrights during installs? */
	Choice	askok;		/* allow interactive package commands */
	Choice	eject;		/* eject last mounted CD on close? */
	Choice	gui_mode;	/* display format (GUI only) */
	Choice	namelen;	/* name length */
	Choice	ncols;		/* number of columns */
	Choice	notice;		/* display start-up notice */
	char	*spooldirs;	/* candidate spool directories */
	char	*rmntdir;	/* remote mount point for pkg spool dir */
	char	*hosts;		/* target host list */
	int	saved;		/* =1 if this configuration saved to file */
} Config;

#ifdef __STDC__
extern void	config_init(void);
extern int	config_read(char *, Config *);
extern int	config_write(char *, Config *);
extern void	config_set(Config *);
extern void	config_get(Config *);
extern char	*get_spooldir(void);
extern char	*get_rmntdir(void);
extern int	get_interactive(void);
extern int	get_showcr(void);
extern char	*config_file(char *);
extern Admin_file *admin_get(void);
#endif

#endif	/* !SWM_ADMIN_H */
