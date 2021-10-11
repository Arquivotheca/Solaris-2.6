/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reeserved.
 */

#pragma	ident	"@(#)sol_addapp.c	1.4	95/01/16 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include "launcher_p.h"


static const char	*app_record_p_fmt = "\"%s\" %s %s \"%s\"\n";


int
solstice_add_app(const SolsticeApp *app, const char *registry_tag)
{

	FILE		*reg_fp;
	FILE		*tmp_fp;
	int		status;
	char		tmpname[PATH_MAX];
	const char	*resolved_icon_path;
	const char	*resolved_app_args;
	char		dir[PATH_MAX];
	char		s[512];
	char		name[256];
	char		icon_path[256];
	char		app_path[256];
	char		app_args[256];
	char		*last_slash;


	if (app == NULL || app->name == NULL || app->app_path == NULL ||
	    app->name[0] == '\0' || app->app_path[0] != '/' ||
	    (app->icon_path != NULL &&
	    (app->icon_path[0] != '\0' && app->icon_path[0] != '/')) ||
	    (registry_tag != NULL && registry_tag[0] != '/')) {
		return (LAUNCH_BAD_INPUT);
	}

	if (app->icon_path == NULL || app->icon_path[0] == '\0') {
		resolved_icon_path = get_default_icon_path();
	} else {
		resolved_icon_path = app->icon_path;
	}

	if (app->app_args != NULL) {
		resolved_app_args = app->app_args;
	} else {
		resolved_app_args = "";
	}

	if (registry_tag == NULL || registry_tag[0] == '\0') {
		registry_tag = get_global_reg();
	}

	/* touch the registry file if it doesn't exist */

	if ((status = touch_registry(registry_tag)) != LAUNCH_OK) {
		return (LAUNCH_ERROR);
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
	 * Read each record from the database, update it if
	 * necessary, write it to the temp file, write the
	 * new record to the end of the temp file, close the
	 * temp file, and move it back on top of the real
	 * file.
	 */

	while (fgets(s, sizeof (s), reg_fp) != NULL) {

		if (s[0] != '#' && s[0] != '\n') {

			/*
			 * Not a comment or blank line, so it's a real
			 * entry.  Check its name to make sure that the
			 * new entry coming in isn't a dup.
			 */

			status = parse_entry(s, name, sizeof (name),
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

			if (strcmp(name, app->name) == 0) {
				/* dup; bail now */
				fclose(tmp_fp);
				fclose(reg_fp);
				(void) unlink(tmpname);
				unlock_registry(registry_tag);
				return (LAUNCH_DUP);
			}
		}

		/* write to the new file */
		fputs(s, tmp_fp);
	}

	/* Write the new record */
	fprintf(tmp_fp, app_record_p_fmt,
	    app->name, resolved_icon_path, app->app_path, resolved_app_args);

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

	return (LAUNCH_OK);
}
