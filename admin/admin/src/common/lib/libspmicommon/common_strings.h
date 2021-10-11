#ifndef lint
#pragma ident "@(#)common_strings.h 1.2 95/12/04 SMI"
#endif

/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	common_strings.h
 * Group:	libspmicommon
 * Description:	This header contains strings used in libspmicommon
 *		library modules.
 */

#include <libintl.h>

/* constants */

#ifndef	TEXT_DOMAIN
#define	TEXT_DOMAIN	"SUNW_INSTALL_LIBCOMMON"
#endif

#ifndef ILIBSTR
#define	ILIBSTR(x)	dgettext(TEXT_DOMAIN, x)
#endif

/* message strings */

#define	MSG_LEADER_ERROR	ILIBSTR("ERROR")
#define	MSG_LEADER_WARNING	ILIBSTR("WARNING")
#define	MSG_COPY_FAILED		ILIBSTR(\
	"Could not copy file (%s) to (%s)")
#define	CREATING_MNTPNT		ILIBSTR(\
	"Creating mount point (%s)")
#define	CREATE_MNTPNT_FAILED	ILIBSTR(\
	"Could not create mount point (%s)")
#define	SYNC_WRITE_SET_FAILED	ILIBSTR(\
	"Could not access %s to set synchronous writes")

/* common_scriptwrite.c */
#define	MSG1_BAD_TOKEN		ILIBSTR(\
	"Bad Token: %s\n")
