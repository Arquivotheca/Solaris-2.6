#ifndef lint
#pragma ident "@(#)common_misc.c 1.10 96/10/11 SMI"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <stdlib.h>
#include <limits.h>
#include <regex.h>
#include "spmicommon_lib.h"

/* internal prototypes */
void		link_to(Item **, Item *);

/* private prototypes */
void		error_and_exit(int);
static char * _sh_to_regex(char *s);

/* Local Statics and Constants */
static char		rootdir[BUFSIZ] = "";
static MachineType	machinetype = MT_STANDALONE;
static void 		(*fatal_err_func)() = &error_and_exit;

/* ----------------------- public functions --------------------------- */

/*
 * get_rootdir()
 *	Returns the rootdir previously set by a call to set_rootdir(). If
 *	set_rootdir() hasn't been called this returns a pointer to a NULL
 *	string.
 * Parameters:
 *	none
 * Return:
 *	char *	- pointer to current rootdir string
 * Status:
 *	public
 */
char *
get_rootdir(void)
{
	return (rootdir);
}

/*
 * set_rootdir()
 *	Sets the global 'rootdir' variable. Used to install packages
 *	to 'newrootdir'.
 * Parameters:
 *	newrootdir	- pathname used to set rootdir
 * Return:
 *	none
 * Status:
 *	public
 */
void
set_rootdir(char *newrootdir)
{
	(void) strcpy(rootdir, newrootdir);
	canoninplace(rootdir);

	if (streq(rootdir, "/"))
		rootdir[0] = '\0';
}

/*
 * xcalloc()
 * 	Allocate 'size' bytes from the heap using calloc()
 * Parameters:
 *	size	- number of bytes to allocate
 * Return:
 *	NULL	- calloc() failure
 *	void *	- pointer to allocated structure
 * Status:
 *	public
 */
void *
xcalloc(size_t size)
{
	void *	tmp;

	if ((tmp = (void *) malloc(size)) == NULL) {
		fatal_err_func(ERR_MALLOC_FAIL);
		return (NULL);
	}

	(void) memset(tmp, 0, size);
	return (tmp);
}

/*
 * xmalloc()
 * 	Alloc 'size' bytes from heap using malloc()
 * Parameters:
 *	size	- number of bytes to malloc
 * Return:
 *	NULL	- malloc() failure
 *	void *	- pointer to allocated structure
 * Status:
 *	public
 */
void *
xmalloc(size_t size)
{
	void *tmp;

	if ((tmp = (void *) malloc(size)) == NULL) {
		fatal_err_func(ERR_MALLOC_FAIL);
		return (NULL);
	} else
		return (tmp);
}

/*
 * xrealloc()
 *	Calls realloc() with the specfied parameters. xrealloc()
 *	checks for realloc failures and adjusts the return value
 *	automatically.
 * Parameters:
 *	ptr	- pointer to existing data block
 * 	size	- number of bytes additional
 * Return:
 *	NULL	- realloc() failed
 *	void *	- pointer to realloc'd structured
 * Status:
 *	public
 */
void *
xrealloc(void *ptr, size_t size)
{
	void *tmp;

	if ((tmp = (void *)realloc(ptr, size)) == (void *)NULL) {
		fatal_err_func(ERR_MALLOC_FAIL);
		return ((void *)NULL);
	} else
		return (tmp);
}

/*
 * xstrdup()
 *	Allocate space for the string from the heap, copy 'str' into it,
 *	and return a pointer to it.
 * Parameters:
 *	str	- string to duplicate
 * Return:
 *	NULL	- duplication failed or 'str' was NULL
 * 	char *	- pointer to newly allocated/initialized structure
 * Status:
 *	public
 */
char *
xstrdup(char * str)
{
	char *tmp;

	if (str == NULL)
		return ((char *)NULL);

	if ((tmp = strdup(str)) == NULL) {
		fatal_err_func(ERR_MALLOC_FAIL);
		return ((char *)NULL);
	} else
		return (tmp);
}

/*
 * Function: strip_whitespace
 * Description:
 *	Strip the leading and trailing whitespace off of the
 *	str passed in.
 *
 * Scope:	PUBLIC
 * Parameters:
 *	str - [RW]
 *		char ** ptr to a str to be stripped.
 *		value of *str may change.
 * Return:	none
 * Globals:	none
 * Notes:
 */
void
strip_whitespace(char *str)
{
	char *ptr;
	char *tmp_str;

	if (!str || !strlen(str))
		return;


	/*
	 * Strip spaces off front:
	 * 	- find 1st non-blank char
	 *	- copy remaining str back to beginning of str.
	 */
	for (ptr = str; *ptr == ' '; ptr++);

	/* if there were leading spaces */
	if (ptr != str) {
		tmp_str = (char *) xmalloc(strlen(ptr) + 1);
		(void) strcpy(tmp_str, ptr);
		(void) strcpy(str, tmp_str);
	}

	/* strip spaces off end */
	for (ptr = str; *ptr && *ptr != ' '; ptr++);
	*ptr = '\0';
}

/*
 * Function: _sh_to_regex
 * Description:
 *	Convert a shell regular expression to an
 *	extended RE (regular expression)
 *	(thanks to Sam Falkner)
 * Scope:	PRIVATE
 * Parameters:
 *	char *pattern - shell regular expression to convert
 * Return:
 *	char * - converted shell expression; has been dynamically
 *	allocated - caller should free.
 * Globals:	none
 * Notes:
 */
static char *
_sh_to_regex(char *pattern)
{
	char *vi;
	char *c;
	char *tmp_pattern;
	char *tmp_pattern_ptr;

	if (!pattern)
		return (NULL);

	/* copy the pattern passed in so we don't modify it permanently */
	tmp_pattern = tmp_pattern_ptr = xstrdup(pattern);

	/*
	 * we'll never expand it to more than twice the original length
	 * plus the ^, $ anchors.
	 */
	vi = (char *) xmalloc((strlen(tmp_pattern) * 2) + 3);

	vi[0] = '^';
	for (c = vi+1; *tmp_pattern_ptr; ++c, ++tmp_pattern_ptr) {
		if (*tmp_pattern_ptr == '\\') {
			*(c++) = *(tmp_pattern_ptr++);
		} else if (*tmp_pattern_ptr == '*') {
			*(c++) = '.';
		} else if ((*tmp_pattern_ptr == '.') ||
			(*tmp_pattern_ptr == '$') ||
			(*tmp_pattern_ptr == '^')) {
			*(c++) = '\\';
		} else if (*tmp_pattern_ptr == '?') {
			*tmp_pattern_ptr = '.';
		}
		*c = *tmp_pattern_ptr;
		if (*tmp_pattern_ptr == '\0') {
			++c;
			break;
		}
	}
	*(c++) = '$';
	*c = '\0';

	free(tmp_pattern);
	return (vi);
}
/*
 * Function: re_match
 * Description:
 *	Perform regular expression matching on the search_str passed in
 *	using the RE pattern.
 * Scope:	PUBLIC
 * Parameters:
 * 	search_str - [R0] - string to look for RE pattern in
 * 	pattern - [R0] - RE pattern string
 *	shell_re_flag - [RO] -
 *		!0: treat pattern as a shell regular expression
 *		0: treat pattern as an regex() extended regular expression
 * Return:	REError (see typedef)
 * Globals:	none
 * Notes:
 */
REError
re_match(char *search_str, char *orig_pattern, int shell_re_flag)
{
	regex_t re;
	int ret;
	char errbuf[PATH_MAX];
	char *pattern;

	if (!orig_pattern || !search_str)
		return (REBadArg);

	/* convert the shell re pattern to a ERE pattern if requested */
	if (shell_re_flag) {
		pattern = _sh_to_regex(orig_pattern);
	} else {
		pattern = xstrdup(orig_pattern);
	}

	ret = regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB);
	if (ret != REG_OK) {
		(void) regerror(ret, &re, errbuf, PATH_MAX);
		regfree(&re);
		free(pattern);
		return (RECompFailure);
	}

	ret = regexec(&re, search_str, 0, NULL, 0);
	regfree(&re);
	free(pattern);
	if (ret == REG_NOMATCH) {
		return (RENoMatch);
	} else {
		return (REMatch);
	}

	/* NOTREACHED */
}

/* ---------------------- internal functions -------------------------- */

#ifdef notdef
/*
 * keyvalue_parse()
 *	Convert a key-value pair line into canonical form.  The
 *	operation is performed in-place.
 *	The following conversions are performed:
 *	- remove leading white space.
 *	- remove any white space before or after the "=".
 *	- remove any comments (anything after a '#')
 *	- null-terminate the keyword.
 *	- remove trailing blanks.
 *	- if the line is empty after these conversions, convert the
 *	  string to the null string and return a value pointer of NULL.
 *
 * Parameters:	buf - a pointer to the string to be converted to canonical
 *		form.
 * Return:
 *	a pointer to the value field.  Return NULL if none.
 *	at return, the original buffer now points to the null-terminated
 *	keyword only.
 * Status:
 *	semi-private (internal library use only)
 */
char *
keyvalue_parse(char *buf)
{
	char	*rp, *wp;
	char	*cp;
	int	len;

	if (buf == NULL)
		return (NULL);
	rp = buf;
	wp = buf;

	/* eat leading blanks */
	while (isspace(*rp))
		rp++;

	/* trim comments */

	if ((cp = strchr(rp, '#')) != NULL)
		*cp = '\0';

	/* trim trailing white space */

	len = strlen(rp);
	if (len > 0) {
		cp = rp + len - 1;  /* *cp points to last char */
		while (isspace(*cp) && cp >= rp - 1)
			cp--;
		/* cp points to last non-white char, or to rp - 1 */
		++cp;
		*cp = '\0';
	}

	if (strlen(rp) == 0) {
		*buf = '\0';
		return (NULL);
	}

	/*
	 *  We now know that there is at least one non-null char in the
	 *  line pointed to by rp (though not necessarily in the line
	 *  pointed to by buf, since we haven't collapsed buf yet.)
	 *  Leading and trailing blanks are gone, and comments are gone.
	 */

	/*  Move the keyword to the beginning of buf */
	while (!isspace(*rp) && *rp != '=' && *rp != '\0')
		*wp++ = *rp++;

	*wp++ = '\0';	/* keyword is now null-terminated */

	/* find the '=' (if there is one) */

	while (*rp != '\0' && isspace(*rp))
		rp++;

	if (*rp != '=')		/* there is no keyword-value */
		return (NULL);

	/* now skip over white space between the '=' and the value */
	while (*rp != '\0' && isspace(*rp))
		rp++;

	/*
	 *  rp now either points to the end of the string, or to the
	 *  beginning of the keyword's value.  If end-of-string, there is no
	 *  keyword value.
	 */

	if (*rp == '\0')
		return (NULL);
	else
		return (rp);
}
#endif /* notdef */

/*
 * append a linked list to the end of another linked list.  Assume
 * that both linked lists are properly terminated.
 */
void
link_to(Item **head, Item *item)
{
	if (item == NULL)
		return;
	while (*head != (Item *)NULL)
		head = &((*head)->next);
	*head = item;
}


/*
 * get_machintype()
 * Parameters:
 *	none
 * Return:
 * Status:
 *	public
 */
MachineType
get_machinetype(void)
{
	return (machinetype);
}

/*
 * set_machinetype()
 *	Set the global machine "type" specifier
 * Parameters:
 *	type	- machine type specifier (valid types: MT_SERVER,
 *		  MT_DATALESS, MT_DISKLESS, MT_CCLIENT, MT_SERVICE)
 * Return:
 *	none
 * Status:
 *	Public
 */
void
set_machinetype(MachineType type)
{
	machinetype = type;
}

/*
 * path_is_readable()
 *	Determine if a pathname is accessable and readable.
 * Parameters:
 *	fn	- pathname
 * Return:
 *	SUCCESS	- path is accessable and readable
 *	FAILURE	- path access/read failed
 * Status:
 *	public
 */
int
path_is_readable(char * fn)
{
	return ((access(fn, R_OK) == 0) ? SUCCESS : FAILURE);
}

/*
 * set_memalloc_failure_func()
 *	Allows an appliation to specify the function to be called when
 *	a memory allocation function fails.
 * Parameters:
 *	(*alloc_proc)(int)	- specifies the function to call if fatal error
 *			  (such as being unable to allocate memory) occurs.
 * Return:
 *	none
 * Status:
 *	Public
 */
void
set_memalloc_failure_func(void (*alloc_proc)(int))
{
	if (alloc_proc != (void (*)())NULL)
		fatal_err_func = alloc_proc;
}

/*
 * get_err_str()
 *	Retrieve the error message associated with 'errno'. Provided
 *	to allow applications which specify their own fatal error
 *	function to turn the error code passed to this function into
 *	a meaningful string.
 * Parameters:
 *	errno	- install-library specific error codes
 * Return:
 *	char *  - pointer to internationalized error string associated
 *		  with 'errno'
 * Status:
 *	Public
 */
char *
get_err_str(int errno)
{
	char *ret;

	switch (errno) {

	case ERR_MALLOC_FAIL:
		ret = dgettext("SUNW_INSTALL_SWLIB",
					"Allocation of memory failed");
		break;
	case ERR_IBE:
		ret = dgettext("SUNW_INSTALL_SWLIB",
		    "Install failed.  See /tmp/install_log for more details");
		break;
	default:
		ret = dgettext("SUNW_INSTALL_SWLIB", "Fatal Error");
		break;
	}

	return (ret);
}

/* ----------------------- private functions -------------------------- */

/*
 * error_and_exit()
 *	Abort routine. An exit code of '2' is used by all applications
 *	to indicate a non-recoverable fatal error.
 * Parameters:
 *	errno	- error index number:
 *			ERR_IBE
 *			ERR_MALLOC_FAIL
 * Return:
 *	none
 * Status:
 *	public
 */
void
error_and_exit(int errno)
{
	(void) printf("%s\n", get_err_str(errno));
	exit(EXIT_INSTALL_FAILURE);
}
