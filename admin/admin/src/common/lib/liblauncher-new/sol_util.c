/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reeserved.
 */

#pragma	ident	"@(#)sol_util.c	1.3	95/01/16 SMI"

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include "launcher_p.h"


#define DEF_ICON_DIR		"/opt/SUNWadm/etc"
#define DB_DIR			"/opt/SUNWadm/etc"
#define DB_FILE			".solstice_registry"

static const char	*global_reg_dir = DB_DIR;
static const char	*global_reg_file = DB_FILE;
static const char	*global_reg = DB_DIR"/"DB_FILE;
static const char	*default_icon_path = DEF_ICON_DIR"/default.xpm";


const char *
get_global_reg(void)
{
	return (global_reg);
}


const char *
get_default_icon_path(void)
{
	return (default_icon_path);
}


int
lock_registry(const char *tag)
{
	return (1);
}


int
unlock_registry(const char *tag)
{
	return (1);
}


static
void
backout(const char *path)
{

	char	cmd[1024];


	if (path == NULL) {
		return;
	}

	sprintf(cmd, "set -f ; /bin/rm -r %s 1>/dev/null 2>&1", path);
	(void) system(cmd);
}


int
touch_registry(const char *registry_tag)
{

	struct stat	stat_buf;
	int		fd;
	char		*working_path = NULL;
	char		*slash_ptr = NULL;
	char		*end_ptr = NULL;
	char		path_comp[PATH_MAX];
	char		backout_path[PATH_MAX];


	backout_path[0] = path_comp[0] = 0;

	if (stat(registry_tag, &stat_buf) == 0) {

		/* file exists, return */

		return (LAUNCH_OK);
	}

	working_path = strdup(registry_tag);
	slash_ptr = working_path;

	if ((end_ptr = strrchr(working_path, '/')) == NULL) {
		return (LAUNCH_ERROR);
	}

	if (end_ptr == working_path) {

		/*
		 * There's just a single '/' character, don't
		 * have to do any path-component creation.
		 * Just do the open() and return.
		 */

		if ((fd = open(registry_tag, O_CREAT | O_EXCL, 0644)) <= 0) {
			return (LAUNCH_ERROR);
		} else {
			(void) close(fd);
			return (LAUNCH_OK);
		}
	}

	/*
	 * Now working_path contains the directory where the
	 * registry file will live.
	 */

	/* Create any necessary path components */

	while (slash_ptr != NULL) {

		while (*slash_ptr == '/') {
			slash_ptr++;
		}

		strncpy(path_comp, working_path, slash_ptr - working_path);
		path_comp[slash_ptr - working_path] = '\0';

		if (stat(path_comp, &stat_buf) != 0) {
			if (errno == ENOENT) {
				if (mkdir(path_comp, 0755) == -1) {
					backout(backout_path);
					return (LAUNCH_ERROR);
				} else {
					/* update backout path */
					strcpy(backout_path, path_comp);
				}
			} else {
				/* hosed */
				backout(backout_path);
				return (LAUNCH_ERROR);
			}
		}

		slash_ptr = strchr(slash_ptr, '/');
	}

	/* Now try to create the registry */

	if ((fd = open(registry_tag, O_CREAT | O_EXCL, 0644)) <= 0) {
		backout(backout_path);
		return (LAUNCH_ERROR);
	}

	(void) close(fd);

	return (LAUNCH_OK);
}


int
parse_entry(
	const char	*entry,
	char		*name,
	size_t		name_size,
	char		*icon_path,
	size_t		icon_path_size,
	char		*app_path,
	size_t		app_path_size,
	char		*app_args,
	size_t		app_args_size)
{

	const char	*p1;
	const char	*p2;


	p1 = entry;

	while ((p1 = strchr(p1, ' ')) != NULL) {
		if (*(p1 - 1) == '"' && *(p1 - 2) != '\\') {

			/*
			 * p - 1 is probably pointing at the closing
			 * quote for the name.  However, somebody could
			 * have mucked it up by having a name that looks
			 * like " foo bar", where the byte before the
			 * name happens to have a backslash in it.  So
			 * make sure that p - 2 is still greater than
			 * entry.
			 */

			if (p1 - 2 > entry) {
				strncpy(name, entry + 1, p1 - entry - 2);
				name[p1 - entry - 2] = 0;
				break;
			}
		}
		p1++;
	}

	p1++;

	p2 = strchr(p1, ' ');

	strncpy(icon_path, p1, p2 - p1);
	icon_path[p2 - p1] = 0;

	p1 = p2 + 1;

	p2 = strchr(p1, ' ');

	strncpy(app_path, p1, p2 - p1);
	app_path[p2 - p1] = 0;

	/* app_args are always surrounded in quotes, so this time skip " */
	p1 = p2 + 2;

	/* -2 for newline char from fgets() and " */
	p2 = entry + strlen(entry) - 2;

	if (p2 > p1) {
		strncpy(app_args, p1, p2 - p1);
		app_args[p2 - p1] = 0;
	} else {
		app_args[0] = 0;
	}

	return (LAUNCH_OK);
}
