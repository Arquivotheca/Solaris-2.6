#ifndef lint
#pragma ident   "@(#)locale_lookup.c 1.3 95/02/24 SMI"
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
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include "sw_lib.h"

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
char	*get_locale_desc_from_media(char *, char *);

/* local function prototypes */
static void read_liblocale_directory(Module *, char *);
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
	DIR		*usr_dir;
	struct dirent	*usr_dirent;
	char		path[MAXPATHLEN];
	struct stat	statbuf;

	/*
	 * Build directory of possible run environments.
	 * (<media>/export/exec/<isa>.Solaris_<ver>/lib/locale
	 */
	(void) sprintf(path, "%s/export/exec", media->info.media->med_dir);
	if ((usr_dir = opendir(path)) == NULL)
		return;
	while ((usr_dirent = readdir(usr_dir)) != NULL) {

		/* Exclude the current/parent directories */
		if (strcmp(usr_dirent->d_name, ".") == 0 ||
		    strcmp(usr_dirent->d_name, "..") == 0)
			continue;

		/* Does this entry have a lib/locale subdirectory?  */
		(void) sprintf(path, "%s/export/exec/%s/%s",
		    media->info.media->med_dir, usr_dirent->d_name,
		    "lib/locale");
		if (lstat(path, &statbuf) || !S_ISDIR(statbuf.st_mode))
			continue;

		read_liblocale_directory(media, path);
	}

	if (usr_dir)
		(void) closedir(usr_dir);

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

/*
 * get_locale_desc_from_media
 *
 * Get the English-language description of the locale specified
 * by the "locale" argument.  It is returned in English, since the
 * the English description is the message key used to translate the
 * string.
 *
 * The "media" pointer is assumed to point to the top level
 * of a Solaris OS CD.  The function searches for locale descriptions
 * in <media>/export/exec/<isa>.Solaris_<ver>/lib/locale/<*>/locale_description
 * 
 * The pointer returned is a pointer to static storage.  It is
 * expected that the caller will make a copy of the string, if
 * the string needs to be saved for later use.
 */
char	*
get_locale_desc_from_media(char *media, char *locale)
{
	char		*cp;
	DIR		*usr_dir;
	struct dirent	*usr_dirent;
	char		path[MAXPATHLEN];
	struct stat	statbuf;

	/*
	 * Build directory of possible run environments.
	 * (<media>/export/exec/<isa>.Solaris_<ver>/lib/locale
	 */
	(void) sprintf(path, "%s/export/exec", media);
	if ((usr_dir = opendir(path)) == NULL)
		return (NULL);
	while ((usr_dirent = readdir(usr_dir)) != NULL) {

		/* Exclude the current/parent directories */
		if (strcmp(usr_dirent->d_name, ".") == 0 ||
		    strcmp(usr_dirent->d_name, "..") == 0)
			continue;

		/* Does this entry have a lib/locale subdirectory?  */
		(void) sprintf(path, "%s/export/exec/%s/%s",
		    media, usr_dirent->d_name, "lib/locale");
		if (lstat(path, &statbuf) || !S_ISDIR(statbuf.st_mode))
			continue;

		if ((cp = read_locale_description_file(path, locale)) != NULL) {
			(void) closedir(usr_dir);
			return (cp);
		}
	}

	if (usr_dir)
		(void) closedir(usr_dir);

	return (get_lang_from_loc_array(locale));
}

static void
read_liblocale_directory(Module *media, char *localedir)
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
			link_to((Item **)&media->info.media->med_locmap,
			    (Item *)lmap);
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

		link_to((Item **)&media->info.media->med_locmap,
		    (Item *)lmap);
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

	strcpy(lc_collate, "C");
	strcpy(lc_ctype, "C");
	strcpy(lc_messages, "C");
	strcpy(lc_monetary, "C");
	strcpy(lc_numeric, "C");
	strcpy(lc_time, "C");

	while (fgets(line, BUFSIZ, fp) != NULL) {
		if (strncmp(STR_LANG, line, LEN_LANG) == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lang, line + LEN_LANG);
			status = 1;
		} else if (strncmp(STR_LC_COLLATE, line, LEN_LC_COLLATE) == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lc_collate, line + LEN_LC_COLLATE);
			status = 2;
		} else if (strncmp(STR_LC_CTYPE, line, LEN_LC_CTYPE) == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lc_ctype, line + LEN_LC_CTYPE);
			status = 2;
		} else if (strncmp(STR_LC_MESSAGES, line, LEN_LC_MESSAGES)
		    == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lc_messages, line + LEN_LC_MESSAGES);
			status = 2;
		} else if (strncmp(STR_LC_MONETARY, line, LEN_LC_MONETARY)
		    == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lc_monetary, line + LEN_LC_MONETARY);
			status = 2;
		} else if (strncmp(STR_LC_NUMERIC, line, LEN_LC_NUMERIC) == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lc_numeric, line + LEN_LC_NUMERIC);
			status = 2;
		} else if (strncmp(STR_LC_TIME, line, LEN_LC_TIME) == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lc_time, line + LEN_LC_TIME);
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

	strcpy(s_locale_description, buf);

	return (s_locale_description);
}
