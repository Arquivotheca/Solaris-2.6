/*
 * Copyright (c) 1994-1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)uname-m.c	1.1	96/10/15 SMI"

#include <sys/param.h>
#include <sys/modctl.h>
#include <sys/salib.h>

#include <sys/platnames.h>

/*
 * Supplied by modpath.c
 *
 * Making these externs here allows all sparc machines to share
 * mod_path_uname_m().
 */
extern char *default_name;
extern char *default_path;

/*
 * Is path "/platform/"dir"/" ?
 */
static int
platcmp(char *path, char *dir)
{
	static char prefix[] = "/platform/";
	static char suffix[] = "/kernel";
	int len;

	if (strncmp(path, prefix, sizeof (prefix) - 1) != 0)
		return (0);
	len = strlen(dir);
	path += sizeof (prefix) - 1;
	if (strncmp(path, dir, len) != 0)
		return (0);
	path += len;
	if (strcmp(path, suffix) != 0)
		return (0);
	return (1);
}

/*
 * This function provides a hook for enhancing the module_path.
 */
/*ARGSUSED*/
void
mod_path_uname_m(char *mod_path, char *impl_arch_name)
{

	/*
	 * If we found the kernel in the default dir, prepend the impl_arch_name
	 * directory (e.g. /platform/SUNW,foo/kernel) to the mod_path unless
	 * impl_arch_name is the same as the default dir.  This can happen if we
	 * found the kernel via the "compatible" property.
	 *
	 * If we found the kernel in the impl_arch_name dir, append the default
	 * directory to the modpath.
	 *
	 * If neither of the above are true, we were given a specific kernel
	 * to boot, so we leave things well enough alone.
	 */
	if (platcmp(mod_path, default_name)) {
		if (strcmp(impl_arch_name, default_name) != 0) {
			char tmp[MOD_MAXPATH];

			(void) strcpy(tmp, mod_path);
			(void) strcpy(mod_path, "/platform/");
			(void) strcat(mod_path, impl_arch_name);
			(void) strcat(mod_path, "/kernel ");
			(void) strcat(mod_path, tmp);
		}
	} else if (platcmp(mod_path, impl_arch_name))
		(void) strcat(mod_path, default_path);
}
