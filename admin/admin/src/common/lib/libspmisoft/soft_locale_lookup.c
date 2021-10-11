#ifndef lint
#pragma ident "@(#)soft_locale_lookup.c 1.4 96/06/25 SMI"
#endif
/*
 *  Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *	File:		locale_lookup.c
 *
 *	Description:	This file contains the routines needed to prompt
 *			the user for the desired locale.
 */

#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include "spmisoft_lib.h"

extern char 	*boot_root;
extern struct locmap *global_locmap;

#define MAX_LOCALE	20

#define	STR_LANG	"LANG="
#define	LEN_LANG	(sizeof (STR_LANG) - 1)
#define	STR_LC_COLLATE	"LC_COLLATE="
#define	LEN_LC_COLLATE	(sizeof (STR_LC_COLLATE) - 1)
#define	STR_LC_CTYPE	"LC_CTYPE="
#define	LEN_LC_CTYPE	(sizeof (STR_LC_CTYPE) - 1)
#define	STR_LC_MESSAGES	"LC_MESSAGES="
#define	LEN_LC_MESSAGES	(sizeof (STR_LC_MESSAGES) - 1)
#define	STR_LC_MONETARY	"LC_MONETARY="
#define	LEN_LC_MONETARY	(sizeof (STR_LC_MONETARY) - 1)
#define	STR_LC_NUMERIC	"LC_NUMERIC="
#define	LEN_LC_NUMERIC	(sizeof (STR_LC_NUMERIC) - 1)
#define	STR_LC_TIME	"LC_TIME="
#define	LEN_LC_TIME	(sizeof (STR_LC_TIME) - 1)

/* library function prototypes */
void read_locale_table(Module *);
char	*get_locale_description(char *, char *);

/* local function prototypes */
static void read_liblocale_directory(char *);
static int read_locale_file(FILE *fp, char *, char *, char *,
	char *, char *, char *, char *);
static char *read_locale_description_file(char *, char *);

/* static variables */
static char	s_locale_description[256];

/*
 * read_locale_table:
 *	returns 1 if locale should be determined, 0 if no need
 *
 *	The idea is to scan the directories under /usr/lib/locale
 *	for locales and categories that are in the proper arrangement,
 *	and create a menu of available locales for user selection.
 *	Only create the menu if non-default locale is found.
 *	The menu is used in case the user must be prompted.
 *	This should be called before prompt_locale().
 */

void
read_locale_table(Module *media)
{
	char		path[MAXPATHLEN];
	struct stat	statbuf;

	if (media->sub == NULL || media->sub->info.prod == NULL ||
	    media->sub->info.prod->p_pkgdir == NULL)
		return;
	/*
	 *  This next line is a temporary workaround for build 11.
	 *  By creating a symlink that allows this pathname to
	 *  be resolved, RE can make the build 11 CD build work (and
 	 *  find the /usr/lib/locale directory.  Eventually, the
	 *  alternate location for the /usr/lib/locale directory needs
	 *  to be provided as an argument to buildtoc, which will then
	 *  provide it (somehow) to this function.  This function should
	 *  try the alternate location if provided, or should use
	 *  /usr/lib/locale.
	 */
	(void) sprintf(path, "%s/../Tools/Boot/usr/lib/locale",
	    media->sub->info.prod->p_pkgdir);
	if (lstat(path, &statbuf) || !S_ISDIR(statbuf.st_mode)) {
		(void) strcpy(path, "/usr/lib/locale");
		if (lstat(path, &statbuf) || !S_ISDIR(statbuf.st_mode))
			return;
	}

	read_liblocale_directory(path);

	return;
}

/*
 * get_locale_description
 *
 * Get the English-language description of the locale specified
 * by the "locale" argument.  It is returned in English, since the
 * the English description is the message key used to translate the
 * string.
 * 
 * The pointer returned is a pointer to static storage.  It is
 * expected that the caller will make a copy of the string, if
 * the string needs to be saved for later use.
 */
char	*
get_locale_description(char *root, char *locale)
{
	char	*cp;
	char	path[MAXPATHLEN];

	(void) sprintf(path, "%s/usr/lib/locale", root);
	if ((cp = read_locale_description_file(path, locale)) != NULL)
		return (cp);

	return (get_lang_from_loc_array(locale));
}

static void
read_liblocale_directory(char *localedir)
{
	DIR		*locale_dirp;
	struct dirent	*locale;
	char		path[MAXPATHLEN];
	struct stat	statbuf;
	LocMap		*lmap;
	FILE		*fp;
	char		locstr[7][MAX_LOCALE];
	StringList	*str;
	int		i;
	char		*cp;

	/*
	 * Loop thru all entries in the locale directory,
	 * checking for directories which contain locale
	 * subdirectories.  If there is at least one, then
	 * do the locale menu.
	 */
	locale_dirp = opendir(localedir);
	while (locale_dirp && (locale = readdir(locale_dirp))) {

		/* Obviously, exclude the current/parent directories */
		if (strcmp(locale->d_name, ".") == 0 ||
		    strcmp(locale->d_name, "..") == 0)
			continue;

		/* exclude the default locale */
		if (strcmp(locale->d_name, "C") == 0)
			continue;

		/* Is this entry a directory? */
		(void) sprintf(path, "%s/%s", localedir, locale->d_name);
		if (lstat(path, &statbuf) || !S_ISDIR(statbuf.st_mode))
			continue;

		/* check for a locale_description */
		if ((cp = read_locale_description_file(localedir,
		    locale->d_name)) == NULL)
			continue;

		/* we have a valid locale description, add it to table */

		lmap = (LocMap *) xcalloc((size_t) sizeof (LocMap));
		lmap->locmap_partial = xstrdup(locale->d_name);
		lmap->locmap_description = xstrdup(cp);

		/* now try to read a locale_map file */

		(void) sprintf(path, "%s/%s/locale_map", localedir,
			locale->d_name);

		if ((fp = fopen(path, "r")) == NULL) {
			link_to((Item **)&global_locmap, (Item *)lmap);
			continue;
		}

		for (i = 0; i < 7; i++)
			locstr[i][0] = '\0';

		(void) read_locale_file(fp, locstr[0], locstr[1], locstr[2],
		    locstr[3], locstr[4], locstr[5], locstr[6]);

		for (i = 0; i < 7; i++)
			if (locstr[i] != NULL && locstr[i][0] != '\0' &&
			    strcmp(locale->d_name, locstr[i]) != 0) {
				/* if locale is already on list, continue */
				for (str = lmap->locmap_base; str != NULL;
				    str = str->next)
					if (strcmp(str->string_ptr, locstr[i])
					    == 0)
						break;
				if (str)
					continue;

				/* locale isn't already on list.  Add it */
				str = (StringList *)xcalloc((size_t) sizeof
				    (StringList));
				str->string_ptr = xstrdup(locstr[i]);
				link_to((Item **)&lmap->locmap_base,
				    (Item *)str);
				str = NULL;
			}

		fclose(fp);

		link_to((Item **)&global_locmap, (Item *)lmap);
	}

	/* Close locale directory */
	if (locale_dirp)
		(void) closedir(locale_dirp);

	return;
}

static int
read_locale_file(FILE *fp, char *lang, char *lc_collate, char *lc_ctype,
	char *lc_messages, char *lc_monetary, char *lc_numeric, char *lc_time)
{
	int status = 0;
	char line[BUFSIZ];

	(void) strcpy(lc_collate, "C");
	(void) strcpy(lc_ctype, "C");
	(void) strcpy(lc_messages, "C");
	(void) strcpy(lc_monetary, "C");
	(void) strcpy(lc_numeric, "C");
	(void) strcpy(lc_time, "C");

	while (fgets(line, BUFSIZ, fp) != NULL) {
		if (strncmp(STR_LANG, line, LEN_LANG) == 0) {
			line[strlen(line) - 1] = '\0';
			(void) strcpy(lang, line + LEN_LANG);
			status = 1;
		} else if (strncmp(STR_LC_COLLATE, line, LEN_LC_COLLATE) == 0) {
			line[strlen(line) - 1] = '\0';
			(void) strcpy(lc_collate, line + LEN_LC_COLLATE);
			status = 2;
		} else if (strncmp(STR_LC_CTYPE, line, LEN_LC_CTYPE) == 0) {
			line[strlen(line) - 1] = '\0';
			(void) strcpy(lc_ctype, line + LEN_LC_CTYPE);
			status = 2;
		} else if (strncmp(STR_LC_MESSAGES, line, LEN_LC_MESSAGES)
		    == 0) {
			line[strlen(line) - 1] = '\0';
			(void) strcpy(lc_messages, line + LEN_LC_MESSAGES);
			status = 2;
		} else if (strncmp(STR_LC_MONETARY, line, LEN_LC_MONETARY)
		    == 0) {
			line[strlen(line) - 1] = '\0';
			(void) strcpy(lc_monetary, line + LEN_LC_MONETARY);
			status = 2;
		} else if (strncmp(STR_LC_NUMERIC, line, LEN_LC_NUMERIC) == 0) {
			line[strlen(line) - 1] = '\0';
			(void) strcpy(lc_numeric, line + LEN_LC_NUMERIC);
			status = 2;
		} else if (strncmp(STR_LC_TIME, line, LEN_LC_TIME) == 0) {
			line[strlen(line) - 1] = '\0';
			(void) strcpy(lc_time, line + LEN_LC_TIME);
			status = 2;
		}
	}

	return (status);
}

/*
 * read_locale_description_file
 *
 * read a locale_description file, if possible.  Return a pointer
 * to the locale description (untranslated) in a static buffer.
 */
static char *
read_locale_description_file(char *localedir, char *locale)
{
	char		path[MAXPATHLEN];
	char		buf[BUFSIZ+1];
	struct stat	statbuf;
	FILE		*fp;

	(void) sprintf(path, "%s/%s/locale_description", localedir,
	    locale);
	if (stat(path, &statbuf) || !S_ISREG(statbuf.st_mode) ||
	    (fp = fopen(path, "r")) == NULL)
		return (NULL);

	buf[0] = '\0';
	if (fgets(buf, BUFSIZ, fp) != NULL) {
		int l;

		l = strlen(buf) - 1;
		if (buf[l] == '\n')
			buf[l] = 0;
	}
	fclose(fp);

	if (strlen(buf) == 0 || (u_int) strlen(buf) > 256)
		return (NULL);

	(void) strcpy(s_locale_description, buf);

	return (s_locale_description);
}
