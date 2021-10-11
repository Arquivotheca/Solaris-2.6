/*
 *	autod_autofs.c
 *
 *	Copyright (c) 1988-1996 Sun Microsystems Inc
 *	All Rights Reserved.
 */

#pragma ident	"@(#)autod_autofs.c	1.17	96/04/05 SMI"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mntent.h>
#include <sys/mnttab.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <sys/tiuser.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <fslib.h>
#include <sys/vfs.h>
#include <assert.h>
#include "automount.h"

static int process_opts(char *options, int *directp);
void netbuf_free(struct netbuf *);

int
mount_autofs(
	struct mapent *me,
	char *mntpnt,
	action_list *alp,
	char *rootp,
	char *subdir,
	char *key
)
{
	int mntflags = 0;
	struct utsname utsname;
	struct autofs_args *fnip = NULL;
	int mount_timeout = AUTOFS_MOUNT_TIMEOUT;
	int error = 0;
	char rel_mntpnt[MAXPATHLEN];

	if (trace > 1)
		trace_prt(1, "  mount_autofs %s on %s\n",
		me->map_fs->mfs_dir, mntpnt);

	if (strcmp(mntpnt, "/-") == 0) {
		syslog(LOG_ERR, "invalid mountpoint: /-");
		return (ENOENT);
	}

	/*
	 * get relative mountpoint
	 */
	sprintf(rel_mntpnt, ".%s", mntpnt+strlen(rootp));

	if (trace > 2)
		trace_prt(1, "rel_mntpnt = %s\n", rel_mntpnt);

	if (uname(&utsname) < 0) {
		error = errno;
		syslog(LOG_ERR, "uname %s", strerror(error));
		return (error);
	}

	if ((fnip = (struct autofs_args *)
	    malloc(sizeof (struct autofs_args))) == NULL) {
		goto free_mem;
	}
	(void) memset((void *) fnip, 0, sizeof (*fnip));

	if ((fnip->addr.buf = (char *)malloc(MAXADDRLEN)) == NULL)
		goto free_mem;

	(void) strcpy(fnip->addr.buf, utsname.nodename);
	(void) strcat(fnip->addr.buf, ".autofs");

	if ((fnip->opts = strdup(me->map_mntopts)) == NULL)
		goto free_mem;

	if (process_opts(fnip->opts, &fnip->direct) != 0)
		goto free_mem;

	fnip->addr.len = strlen(fnip->addr.buf);
	fnip->addr.maxlen = fnip->addr.len;

	/*
	 * get absolute mountpoint
	 */
	if ((fnip->path = strdup(mntpnt)) == NULL)
		goto free_mem;

	if ((fnip->map = strdup(me->map_fs->mfs_dir)) == NULL)
		goto free_mem;
	if ((fnip->subdir = strdup(subdir)) == NULL)
		goto free_mem;
	/*
	 * This timeout is really ignored by autofs, it uses the
	 * parent directory's timeout since it's really the one
	 * specified/inherited from the original mount by 'automount'
	 */
	fnip->mount_to = mount_timeout;	/* IGNORED */
	fnip->rpc_to = AUTOFS_RPC_TIMEOUT;

	if (fnip->direct) {
		if (me->map_modified == TRUE || me->map_faked == TRUE) {
			if ((fnip->key = strdup(key)) == NULL)
				goto free_mem;
		} else {
			/* wierd case of a direct map pointer in another map */
			if ((fnip->key = strdup(fnip->path)) == NULL)
				goto free_mem;
		}
	} else {
		fnip->key = NULL;
	}

	/*
	 * Fill out action list.
	 */
	alp->action.action = AUTOFS_MOUNT_RQ;
	if ((alp->action.action_list_entry_u.mounta.spec =
	    strdup(me->map_fs->mfs_dir)) == NULL)
		goto free_mem;
	if ((alp->action.action_list_entry_u.mounta.dir =
	    strdup(rel_mntpnt)) == NULL)
		goto free_mem;

	alp->action.action_list_entry_u.mounta.flags = mntflags | MS_DATA;
	if ((alp->action.action_list_entry_u.mounta.fstype =
	    strdup(MNTTYPE_AUTOFS)) == NULL)
		goto free_mem;
	alp->action.action_list_entry_u.mounta.dataptr = (char *)fnip;
	alp->action.action_list_entry_u.mounta.datalen = sizeof (*fnip);

	return (0);

free_mem:
	/*
	 * We got an error, free the memory we allocated.
	 */
	syslog(LOG_ERR, "mount_autofs: memory allocation failure");
	free_autofs_args(fnip);
	alp->action.action_list_entry_u.mounta.dataptr = NULL;
	alp->action.action_list_entry_u.mounta.datalen = 0;
	free_mounta(&alp->action.action_list_entry_u.mounta);

	return (error ? error : ENOMEM);
}

/*
 * Remove pseudo-options "direct", "indirect", "nest", and "ignore" from
 * option list.  Set *directp to 1 if "direct" is found, and 0 otherwise
 * (mounts are indirect by default).  If both "direct" and "indirect" are
 * found, the last one wins.
 */
static int
process_opts(char *options, int *directp)
{
	char *opt, *opts, *lasts;
	char buf[AUTOFS_MAXOPTSLEN];

	assert(strlen(options)+1 < AUTOFS_MAXOPTSLEN);

	strcpy(buf, options);
	opts = buf;
	options[0] = '\0';
	*directp = 0;

	while ((opt = strtok_r(opts, ",", &lasts)) != NULL) {
		opts = NULL;
		while (isspace(*opt)) {
			opt++;
		}
		if (strcmp(opt, "direct") == 0) {
			*directp = 1;
		} else if (strcmp(opt, "indirect") == 0) {
			*directp = 0;
		} else if ((strcmp(opt, "nest") != 0) &&
				(strcmp(opt, "ignore") != 0)) {
			if (options[0] != '\0') {
				(void) strcat(options, ",");
			}
			(void) strcat(options, opt);
		}
	};
	return (0);
}

/*
 * Free autofs_args structure
 */
void
free_autofs_args(struct autofs_args *p)
{
	if (p == NULL)
		return;
	if (p->addr.buf)
		free(p->addr.buf);
	if (p->path)
		free(p->path);
	if (p->opts)
		free(p->opts);
	if (p->map)
		free(p->map);
	if (p->subdir)
		free(p->subdir);
	if (p->key)
		free(p->key);
	free(p);
}

/*
 * free mounta structure. Assumes that m->dataptr has
 * been freed already
 */
void
free_mounta(struct mounta *m)
{
	if (m == NULL)
		return;
	if (m->spec)
		free(m->spec);
	if (m->dir)
		free(m->dir);
	if (m->fstype)
		free(m->fstype);
	assert(m->dataptr == NULL);
	assert(m->datalen == 0);
	/*
	 * no need to free 'm' since it is part of the
	 * action_list_entry structure.
	 */
}
