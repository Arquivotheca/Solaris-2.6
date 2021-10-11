#ifndef lint
#pragma ident "@(#)profile.h 2.16 96/07/09 SMI"
#endif

/*
 * Copyright (c) 1991-1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	profile.h
 * Group:	pfinstall
 * Description:
 */

#ifndef _PROFILE_H
#define	_PROFILE_H

#include "spmiapp_api.h"
#include "spmistore_api.h"

/* constants */

#define	DEFAULT_FS_FREE		15	/* default UFS free space */
#define	DOSPRIMARY		-1	/* fdisk partition type alias
					 * DOS12, DOS16, and DOSHUGE */

/* Global variables */

char		**environ;	/* for exec's */

/* ---------------------- prototypes ------------------------ */

/* pf_fdisk.c */
int		configure_fdisk(Fdisk *);
int		pf_convert_id(int);

/* pf_parse.c */
int		parse_profile(Profile *, char *);

/* pf_software.c */
int		configure_software(Profile *);
int		configure_media(Profile *);
int		software_load(Profile *);
void		software_print_size(void);
int		ScriptHandler(void *UserData, void *SpecificData);
int		SpaceCheckingHandler(void *UserData, void *SpecificData);
int		ArchiveHandler(void *UserData,void *SpecificData);
int		SoftUpdateHandler(void *UserData,void *SpecificData);

/* pf_util.c */
void		print_disk_layout(void);
void		print_space_layout(Space*, Label_t);
void		fatal_exit(char *, ...);

/* pf_configure.c */
int		configure_bootdisk(Profile *);
int		configure_disks(Profile *);
int		configure_unused_disks(Profile *);

#endif	/* _PROFILE_H */
