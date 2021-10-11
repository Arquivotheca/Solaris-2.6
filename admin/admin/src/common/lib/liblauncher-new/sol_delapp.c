/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reeserved.
 */

#pragma	ident	"@(#)sol_delapp.c	1.2	95/07/10 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include "launcher_p.h"


int
solstice_del_app(const char *name, const char *registry_tag)
{

	FILE		*reg_fp;
	FILE		*tmp_fp;
	int		status;
	struct stat	stat_buf;
	char		tmpname[PATH_MAX];
	char		dir[PATH_MAX];
	char		s[512];
	char		e_name[256];
	char		icon_path[256];
	char		app_path[256];
	char		app_args[256];
	char		*last_slash;
	int		deleted = 0;


	if (name == NULL) {
		return (LAUNCH_BAD_INPUT);
	}

	if (registry_tag == NULL) {
		registry_tag = get_global_reg();
	}

	/* fail if can't stat the registry */

	if (stat(registry_tag, &stat_buf) != 0) {
		if (errno == ENOENT) {
			return (LAUNCH_NO_REGISTRY);
		} else {
			return (LAUNCH_ERROR);
		}
	}

	/* find the dir that the registry file lives in */

	strcpy(dir, registry_tag);
	last_slash = strrchr(dir, '/');
	if (last_slash != NULL) {
		*last_slash = 0;
	}

	strcpy(tmpname, dir);
	strcat(tmpname, "/.solXXXXXX");
	(void) mktemp(tmpname);

	/* attempt to lock the registry, fail if unsuccessful */

	if (lock_registry(registry_tag) == -1) {
		return (LAUNCH_LOCKED);
	}

	if ((reg_fp = fopen(registry_tag, "r")) == NULL) {
		unlock_registry(registry_tag);
		return (LAUNCH_ERROR);
	}

	if ((tmp_fp = fopen(tmpname, "w")) == NULL) {
		fclose(reg_fp);
		unlock_registry(registry_tag);
		return (LAUNCH_ERROR);
	}

	/*
	 * Read each record from the database, check if it's
	 * the one to be deleted; if not, write it to the temp
	 * file.  Then close the temp file, and move it back on
	 * top of the real file.
	 */

	while (fgets(s, sizeof (s), reg_fp) != NULL) {

		if (s[0] != '#' && s[0] != '\n') {

			/*
			 * Not a comment or blank line, so it's a real
			 * entry.  Check its name to make sure that the
			 * new entry coming in isn't a dup.
			 */

			status = parse_entry(s, e_name, sizeof (e_name),
			    icon_path, sizeof (icon_path),
			    app_path, sizeof (app_path),
			    app_args, sizeof (app_args));

			if (status != LAUNCH_OK) {
				/* problem; bail now */
				fclose(tmp_fp);
				fclose(reg_fp);
				(void) unlink(tmpname);
				unlock_registry(registry_tag);
				return (LAUNCH_ERROR);
			}

			if (strcmp(e_name, name) == 0) {
				deleted = 1;
				continue;
			}
		}

		/* write to the new file */
		fputs(s, tmp_fp);
	}

	/* close the files */

	fclose(tmp_fp);
	fclose(reg_fp);

	/* Move the new, updated file over the old one */

	if (rename(tmpname, registry_tag) == -1) {
		(void) unlink(tmpname);
		(void) unlock_registry(registry_tag);
		return (LAUNCH_ERROR);
	}

	/* Clean up, unlock the registry */

	(void) unlock_registry(registry_tag);

	if (deleted == 1) {
		return (LAUNCH_OK);
	} else {
		return (LAUNCH_NO_ENTRY);
	}
}
