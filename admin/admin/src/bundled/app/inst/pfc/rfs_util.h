#ifndef lint
#pragma ident "@(#)rfs_util.h 1.8 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	rfs_util.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _RFS_UTIL_H
#define	_RFS_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		HelpEntry help;
		NRowCol loc;
		FieldType type;
		char *label;
		int len;
		int maxlen;
		void *data;
	} _RFS_item_t;

	typedef struct {

		/*
		 * one item for the field's label, one for the editable
		 * contents
		 */
		_RFS_item_t f[2];

	} _RFS_row_t;

	/*
	 * this structure provides the `backing' store for the displayed and
	 * edited remote file systems.
	 */
	typedef struct {
		char server[257];		/* Server's hostname */
		char ip_addr[16];		/* Server's IP address */
		char mnt_pt[MAXMNTLEN];	/* local mount point */
		char server_path[MAXMNTLEN];	/* fullpath on server */
	} RFS_t;

#define	N_RFS_FIELDS		4
#define	RFS_TMP_MOUNT_POINT	"/tmp/a"

	extern int get_rfs_spec(WINDOW *, char *, char *, RFS_t *, int *, 
	    int, HelpEntry);

#ifdef __cplusplus
}

#endif

#endif	/* _RFS_UTIL_H */
