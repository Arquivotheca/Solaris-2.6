/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reeserved.
 */

#pragma	ident	"@(#)sol_getapps.c	1.1	95/01/13 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include "launcher_p.h"


int
solstice_get_apps(SolsticeApp **apps, const char *registry_tag)
{

	FILE		*reg_fp;
	int		status;
	int		cnt;
	int		i;
	struct stat	stat_buf;
	char		s[512];
	char		name[256];
	char		icon_path[256];
	char		app_path[256];
	char		app_args[256];


	if (apps == NULL) {
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

	/* attempt to lock the registry, fail if unsuccessful */

	if (lock_registry(registry_tag) == -1) {
		return (LAUNCH_LOCKED);
	}

	if ((reg_fp = fopen(registry_tag, "r")) == NULL) {
		unlock_registry(registry_tag);
		return (LAUNCH_ERROR);
	}

	cnt = 0;

	while (fgets(s, sizeof (s), reg_fp) != NULL) {
		if (s[0] != '#' && s[0] != '\n') {
			/* not a comment or blank line */
			cnt++;
		}
	}

	if (cnt == 0) {
		fclose(reg_fp);
		unlock_registry(registry_tag);
		return (cnt);
	}

	rewind(reg_fp);

	if ((*apps = (SolsticeApp *)malloc((unsigned)(cnt *
	    sizeof (SolsticeApp)))) == NULL) {

		fclose(reg_fp);
		unlock_registry(registry_tag);
		return (LAUNCH_ERROR);
	}

	i = 0;

	while (fgets(s, sizeof (s), reg_fp) != NULL) {

		if (s[0] != '#' && s[0] != '\n') {

			/*
			 * Not a comment or blank line, so it's a real
			 * entry.  Get the name (with escape chars removed)
			 * and add the data to the return array.
			 */

			status = parse_entry(s, name, sizeof (name),
			    icon_path, sizeof (icon_path),
			    app_path, sizeof (app_path),
			    app_args, sizeof (app_args));

			if (status != LAUNCH_OK) {
				/* problem; bail now */
				fclose(reg_fp);
				unlock_registry(registry_tag);
				return (LAUNCH_ERROR);
			}

			(*apps)[i].name = strdup(name);
			(*apps)[i].icon_path = strdup(icon_path);
			(*apps)[i].app_path = strdup(app_path);
			(*apps)[i].app_args = strdup(app_args);

			i++;
		}
	}

	fclose(reg_fp);
	unlock_registry(registry_tag);

	return (cnt);
}


void
solstice_free_app_list(SolsticeApp *apps, int cnt)
{

	int	i;


	if (apps == NULL || cnt <= 0) {
		return;
	}

	for (i = 0; i < cnt; i++) {
		if (apps[i].name != NULL) {
			free((void *)apps[i].name);
		}
		if (apps[i].icon_path != NULL) {
			free((void *)apps[i].icon_path);
		}
		if (apps[i].app_path != NULL) {
			free((void *)apps[i].app_path);
		}
		if (apps[i].app_args != NULL) {
			free((void *)apps[i].app_args);
		}
	}

	free((void *)apps);
}
