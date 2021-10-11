
/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 *	adm_find_method.c			9/14
 *
 *	A set of routines to for locating a class method.
 *	Major routine takes a class and a message, and returns
 *	a path to an executable method.  Provides multiple
 *	inheritance
 *
 * adm_find_super_class()
 *	Returns a list of superclasses of a class
 * adm_find_object_class()
 *	searches a path variable for a directory called object
 * adm_find_class()
 *	Given a class name, find subdirectory.
 * adm_find_method()
 *	Given a class and method, return path to executable.
 *	Calls adm_find_class to locate the class
 * adm_find_domain()
 *	Given a class and method, return a list of textdomains
 *
 * XXX: system calls which do not report EINTR
 *	fopen, fclose, fgets, getenv
 */

#pragma	ident	"@(#)adm_find_method.c	1.66	95/06/02 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/param.h>
#include <locale.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>

#include <stdarg.h>

#include "adm_fw.h"
#include "adm_err_msgs.h"	/* Also found via adm_fw.h		*/
#include "adm_fw_impl.h"
#include "adm_om_impl.h"	/* Definitions of constants, strings 	*/
#include "adm_om_proto.h"	/* External IF of this module		*/
#include "adm_cache.h"		/* Interface of OM cache		*/

extern char *getenv();
extern int closedir();
extern int close();

extern pid_t getpid();
extern int vprintf();

extern char *adm_obj_path;
extern char *adm_obj_pathlist[];
extern adm_cache adm_om_cache;
/*
extern static char *target; */	/* Global that holds target of match */

/* Function declarations */

void adm_build_path(char *s1, char *s2);
void adm_build_name (char *s1, char *s2);
void adm_remove_component(char *path);
int  adm_find_first(char *path, int buf_len, char *target, char match[],
	int (*cmp)(char *, char*));

static int find_method(char *class_name, char *c_version, char *method_name,
	char path[], int buf_len);
static int om_write_err(char *output_buf, ...);
static int pprefix(char *s1, char *s2);
static void load_adm_path_list(char adm_path_list[]);
static char *add_string (char *class_string, char *classes, char **pos, 
	int *size);
static char *get_next_class(char *p, char buf[], int *done);
static int search_class_string(const char *text, const char *pattern);

static void read_adm_pathlist_file(char adm_path_list[]);

/*
 *	Local Macros
 */

#define	P(s)	((s == (char *)NULL) ? "(null)" : s)
#define	SP(s)	((s == (char *)NULL) ? "" : s)
#define ADM_FIND_METHOD_DEBUG_ON 0

#define ADM_SEARCH_PATH_FILE	"/opt/SUNWadm/admind_search_paths"

typedef struct
{
	char	path[MAXPATHLEN];
	char	prod[BUFSIZ];
	char	vers[BUFSIZ];
} AdmProductPath;

/*
 *	Function definitions
 */

/*
 *	pprefix(s1, s2)
 *
 *	Is string s1 a proper prefix of s2?
 */

static int
pprefix(char *s1, char *s2)
{
	const int separator = '.';

	for (; *s1 == *s2; s1++, s2++)	{
	}

	return ((*s1 == '\0') && (*s2 == separator));
}


/*
 *	adm_find_first
 *
 * 	This function steps through a directory, looking for a file
 *	that matches an arbitrary predicate
 *
 *	If a match is found, it is copied back into string "match"
 *	Function value just holds error condition
 *
 *	This was designed to hold a functional argument
 */
int
adm_find_first(char *path, int buf_len, char *target, char match[],
	int (*cmp)(char *s1, char *s2))
{
	DIR *dir_d_p;		/* Pointer to directory descriptor */
	struct dirent *dir_e_p;	/* Pointer to directory entry 	*/

	match[0] = '\0';

	if ((path == (char *)NULL) || (match == (char *)NULL))	{
		return (om_write_err(path, buf_len, ADM_ERR_NULLSTRING,
		    FINDFIRST));
	}

	dir_d_p = opendir(path);
	if (dir_d_p == (DIR *)NULL)	{
		return (om_write_err (path, buf_len, ADM_ERR_OPENDIR,
		    FINDFIRST, P(path), P(strerror(errno))));
	}

	/* Step through the directory, looking for a file 	*/
	while ((void *)(dir_e_p = readdir(dir_d_p)) != NULL) {
		if (cmp(target, dir_e_p->d_name)) {
			strcpy(match, dir_e_p->d_name);
			break;
		}
	}

	if (closedir(dir_d_p))	{
		return (om_write_err(path, buf_len, ADM_ERR_CLOSEDIR, FINDFIRST,
		    P(path), P(strerror(errno))));
	} else {
		return (ADM_SUCCESS);
	}
}


/*
 * Utility functions for versioning.
 */

static void
extract_digits(const char *vers, int *maj, int *min, int *micro)
{

	const char	*p1, *p2;
	int		len;
	char		buf[32];


	*maj = *min = *micro = 0;

	p2 = NULL;
	if ((p1 = strchr(vers, '.')) != NULL) {
		p1++;
		if (p1 != NULL && (p2 = strchr(p1, '.')) != NULL) {
			p2++;
		}
	}

	if (p2 == NULL) {
		if (p1 == NULL) {
			/* only a single digit */
			*maj = atoi(vers);
		} else {
			/* only maj and min */
			len = p1 - vers + 1;
			strncpy(buf, vers, len);
			buf[len] = 0;
			*maj = atoi(buf);

			*min = atoi(p1);
		}
	} else {
		/* maj, min, and micro */

		len = p1 - vers + 1;
		strncpy(buf, vers, len);
		buf[len] = 0;
		*maj = atoi(buf);

		len = p2 - p1 + 1;
		strncpy(buf, p1, len);
		buf[len] = 0;
		*min = atoi(buf);

		*micro = atoi(p2);
	}
}


/*
 * Yes, this compare function works backwards, returning < 0 when
 * the first elt is greater than the second.  We're sorting the
 * admind search paths based on product version, and want the later
 * versions (higher number) to come first.
 */

static int
compare_prodvers(const void *elt1, const void *elt2)
{

	const AdmProductPath	*d1 = elt1;
	const AdmProductPath	*d2 = elt2;
	int			maj_1, maj_2;
	int			min_1, min_2;
	int			micro_1, micro_2;


	/* get maj, min, micro vers */

	extract_digits(d1->vers, &maj_1, &min_1, &micro_1);
	extract_digits(d2->vers, &maj_2, &min_2, &micro_2);

	if (maj_1 > maj_2) {
		return (-1);
	} else if (maj_1 < maj_2) {
		return (1);
	} else {
		if (min_1 > min_2) {
			return (-1);
		} else if (min_1 < min_2) {
			return (1);
		} else {
			if (micro_1 > micro_2) {
				return (-1);
			} else if (micro_1 < micro_2) {
				return (1);
			}
		}
	}

	return (0);
}


/*
 *	adm_find_first_prefix
 *
 *	This simply "curries" adm_find_first, calling it with
 *	a known function.  This allows us to match the parameters
 *	to adm_search_dir's pattern
 */

static int
adm_find_first_prefix(char *path, int buf_len, char *target)
{
	char match[MAXPATHLEN];

	return (adm_find_first(path, buf_len, target, match, pprefix));
}

static
int
acceptable_version_p(char *target, char *candidate, char *req_vers)
{

	int	candidate_base_len;
	int	target_len;
	int	can_major = 0;
	int	can_minor = 0;
	int	can_micro = 0;
	int	req_major = 0;
	int	req_minor = 0;
	int	req_micro = 0;
	char	*p;
	char	*can_vers;


	/*
	 * Determine if candidate is viable, given target and req_vers.
	 * target contains the class name, with no version number appended,
	 * such as "system" or "printer".  candidate will be a directory
	 * name from a classes directory, and may or may not have a version
	 * number appended, such as "system", "printer.1", "database.2.0".
	 * req_vers is the requested version.  In order for candidate to
	 * be viable, the major version must match and the minor version
	 * must be greater than or equal to the request.
	 */

	if (target == NULL || candidate == NULL) {
		return (0);
	}

	if ((p = strchr(candidate, '.')) != NULL) {

		candidate_base_len = p - candidate;
		target_len = strlen(target);

		if (candidate_base_len != target_len) {
			return (0);
		}
		if (strncmp(target, candidate, target_len) != 0) {
			return (0);
		}
	} else {
		if (strcmp(target, candidate) != 0) {
			return (0);
		}
	}

	/*
	 * If req_vers is NULL or "", this is a request for any version;
	 * find best will take care of returning the "best" (latest)
	 * version, so we just return TRUE here.
	 */

	if (req_vers == NULL || *req_vers == 0) {
		return (1);
	}

	extract_digits(req_vers, &req_major, &req_minor, &req_micro);

	if ((can_vers = strchr(candidate, '.')) != NULL) {
		can_vers++;
		if (*can_vers != 0) {
			extract_digits(can_vers, &can_major,
			    &can_minor, &can_micro);
		}
	}

 	if (can_major == req_major &&
	    (can_minor > req_minor ||
	    (can_minor == req_minor && can_micro >= req_micro))) {
		return (1);
	} else {
		return (0);
	}
}


static
int
better_version_p(char *dir1, char *dir2)
{

	char	*vers_ptr_1;
	char	*vers_ptr_2;
	int	maj_1, min_1, micro_1;
	int	maj_2, min_2, micro_2;


	/*
	 * At this point, we know that both dir1 and dir2 have passed
	 * the "filter" (acceptable_version_p()), which means that
	 * the class names are the same and the major versions are
	 * the same.  All we have to do is compare minor and micro,
	 * and return an indication of the "better" (greater).  If
	 * dir2 is better than dir1, return true, else false.
	 */

	vers_ptr_1 = strchr(dir1, '.');
	vers_ptr_2 = strchr(dir2, '.');

	if (vers_ptr_1 == NULL) {
		if (vers_ptr_2 == NULL) {
			/* both are unversioned, the same, return false */
			return (0);
		} else {
			/*
			 * dir2 is a version, which must be considered to
			 * be better than the "default" baseline unversioned
			 * class.
			 */
			return (1);
		}
	} else if (vers_ptr_2 == NULL) {
		/* dir1 has a version, dir 2 doesn't, dir1 is better. */
		return (0);
	}

	/* step past the '.' to the first digit */
	vers_ptr_1++;
	vers_ptr_2++;

	/* both have versions, need to compare. */
	min_1 = min_2 = micro_1 = micro_2 = 0;
	extract_digits(vers_ptr_1, &maj_1, &min_1, &micro_1);
	extract_digits(vers_ptr_2, &maj_2, &min_2, &micro_2);

	/* we know majors are the same, just compare minor and micro */

	if (min_2 > min_1 || (min_1 == min_2 && micro_2 > micro_1)) {
		return (1);
	}

	return (0);
}


/*
 *	adm_find_best
 *
 * 	This function steps through a directory, looking for a file
 *	that matches an arbitrary predicate.  It compares all matches,
 *	looking for a best match.
 *
 *	This was designed to hold two functional arguments
 *		Filter	- a condition that any candidate must meet
 *		cmp	- Method to compare candidates
 */
int
adm_find_best(
	char	*path,
	int	buf_len,
	char	*target,
	char	*req_version,
	char	match[],
	int	(*filter)(char *s1, char *s2, char *req_vers),
	int	(*cmp)(char *s1, char *s2))
{

	DIR		*dir_d_p;	/* Pointer to directory descriptor */
	struct dirent	*dir_e_p;	/* Pointer to directory entry 	*/
	int		found;
	char		best_match[MAXPATHLEN];


	match[0] = '\0';
	best_match[0] = '\0';

	found = 1;

	if ((path == (char *)NULL) || (match == (char *)NULL))	{
		return (om_write_err(path, buf_len, ADM_ERR_NULLSTRING,
		    FINDBEST));
	}

	dir_d_p = opendir(path);
	if (dir_d_p == (DIR *)NULL)	{
		return (om_write_err(path, buf_len, ADM_ERR_OPENDIR, FINDBEST,
		    P(path), P(strerror(errno))));
	}

	/* Step through the directory, looking for a match */
	while ((void *)(dir_e_p = readdir(dir_d_p)) != NULL) {
		if (filter(target, dir_e_p->d_name, req_version)) {
			strcpy(match, dir_e_p->d_name);
			if (found == 1)	{
				found = 0;
				strcpy(best_match, match);
			} else {
				if (cmp(best_match, match) > 0) {
					strcpy(best_match, match);
				}
			}
		}
	}
	strcpy(match, best_match);

	if (closedir(dir_d_p))	{
		return (om_write_err(path, buf_len, ADM_ERR_CLOSEDIR,
		    FINDBEST, P(path), P(strerror(errno))));
	} else {
		if (found == 1)	{
			return (ADM_ERR_NOMETHOD);
		} else {
			return (ADM_SUCCESS);
		}
	}
}

/*
 *	previous_version()
 *
 *	This predicate tells if one version preceeds another
 *	This uses sscanf, assumes that syntax is name.digit
 */

static int
previous_version(char *s1, char *s2, int len)
{
	int v1, v2;		/* Holds version numbers */
	int read1, read2;	/* Numbers read */
/*
	assert(s1);
	assert(s2);
	assert(strcmp(s1, s2));	*/ /* Names should be different */

	s1 = s1 + len;
	s2 = s2 + len;

	read1 = sscanf(s1, "%d", &v1);
	read2 = sscanf(s2, "%d", &v2);

	if ((read1 < 1) && (read2 < 1))	{	/* Both non-numeric */
		return (!strcmp(s1, s2));
	} else if ((read1 < 1) || (read2 < 1)){	/* One non-numeric */
		return (read2 - read1);
	} else if (v1 != v2)	{		/* Two != numbers */
		return (v2 - v1);
	} else {				/* Equal numbers: check rest */
		return (!strcmp(s1, s2));	/* names _should_ differ */
	}
}



/*
 * om_write_err();
 *
 * Return error message in the caller's buffer
 *
 * Expected Call: om_write_err(output_buff, buf_len, code, s1, s2, ...)
 *	char *output_buff; int buf_len; char *format; char *s1, *s2, ...)
 */

static int
om_write_err(char *output_buf, ...)
{
	int buf_len;
	int code;		/* Message code */
	char *msg_format;
	va_list ap;		/* Varargs argument list pointer */
	char local_buff[2*MAXPATHLEN];

	/* Get the error status code and message buffer arguments */

	va_start(ap, output_buff);
	buf_len = va_arg(ap, int);

	/* Internationalize the message */
	code = va_arg(ap, int);
	msg_format = adm_err_msg(code);

		/* Get the error message text and substitute arguments */
		/*   These arguments have not been internationalized   */
	vsprintf(local_buff, msg_format, ap);
	va_end(ap);

		/* Copy from staging area to user's buffer: check length */
	if ((int) strlen(local_buff) < buf_len) {
		strcpy(output_buf, local_buff);
	} else {
		strncpy(output_buf, local_buff, (size_t) buf_len);
		if (buf_len > 1) {
			*(output_buf + buf_len - 2) = '\n';
			*(output_buf + buf_len - 1) = '\0';
		}
	}
	return (code);		/* used as return message */
}

/*
 * adm_find_object_class() searches for a directory called classes,
 * Uses environment variable ADMINPATH as well as default directory
 * Sets global variable adm_path_list
 *
 * Returns path or null string
 */

int
adm_find_object_class(char object_path[], int buf_len)
{
	char *next;
	char  adm_path_list[MAXPATHLIST];
	char *source;
	int   j, i, err;
	static char terminal[] = ":";	/* separator in path variable */


	adm_path_list[0] = 0;

	if (adm_obj_pathlist[0] != NULL) {
		if (adm_obj_path == NULL) {
			/*
			 * amsl_log3() wants to log an object class. 
			 * Give it the first one.
			 */
			adm_obj_path = adm_obj_pathlist[0];
		}

		if ((int) strlen(adm_obj_path) > buf_len)	{
			return (om_write_err(object_path, buf_len,
			    ADM_ERR_BUF2SHORT, FINDOBJECT));
		} else	{
			(void) strcpy(object_path, adm_obj_path);
			if (ADM_FIND_METHOD_DEBUG_ON)
				ADM_DBG("o", 
				    ("ObjMgr: adm_find_object returns %s",
				    P(object_path)));

			return (ADM_SUCCESS);
		}
	}	/* else adm_obj_path is not defined */

	(void) load_adm_path_list(adm_path_list);

	source = adm_path_list;			/* Start stepping down copy */

	ADM_DBG("o", ("ObjMgr: Search for class hierarchy in list: %s",
	    P(source)));

	j = 0;
	for (i = 0; i < MAXOBJPATHS - 1; i++) {

		next = strtok(source, terminal);
		if (!next)	{
			break;
		}

		source = NULL;

		err = adm_search_dir(next, MAXPATHLEN, OBJ);
		if (err == FOUND) {
			if ((int)(strlen(next) + strlen(OBJ)) > buf_len) {
				return (om_write_err(object_path, buf_len,
				    ADM_ERR_BUF2SHORT, FINDOBJECT));
			}
			(void) strcpy(object_path, next);
			adm_build_path(object_path, OBJ);

				/* Make permanent record */
			if ((int) strlen(object_path) >= MAXPATHLIST)	{
				return (om_write_err(object_path, buf_len,
				    ADM_ERR_BUF2SHORT, FINDOBJECT));
			} else {
				adm_obj_pathlist[j] = strdup(object_path);
				j++;
			}
		} else if (err != NOTFOUND) {
			return (err);	/* search dir returns error message */
		} /* else err = NOTFOUND, and we try next component */
	}

	if (adm_obj_pathlist[0] == NULL) {
		return (om_write_err(object_path, buf_len,
		    ADM_ERR_NOOBJECT, FINDOBJECT));
	}
	return (ADM_SUCCESS);
}

/*
 * adm_find_super_class() takes a class name and searches an ASCII file
 * for the super-classes of the class.
 * Returns a string with superclasses, with ':' between each class.
 * Later versions will replace the ASCII file  with a NIS+ database.
 * For versioning: take the "full" name of the file
 *
 * Bugs: Does not use NIS+
 */

int
adm_find_super_class(
	char *class_name,
	char  super_class_list[],
	int   buf_len)
{
#define	COMMENT		'#'		/* Skip comments in file */
	FILE *fp = (FILE *)NULL;
	char *next_class, *p;
	char file_name[MAXPATHLEN];	/* Path to superclass file name */
	char buf[MAXSUPERCLASS];	/* Holds entry from table */
	static char terminal[] = "\n\t ";	/* To skip white space */
	int status = ADM_ERR_UNLISTEDCLASS;

	super_class_list[0] = '\0';	/* Clear result in case class=object */

			/* Class Object has no super classes: special case */
	if (strcmp(class_name, OBJECT) == 0) {
		return (ADM_SUCCESS);
	}

			/* See if this is in the cache */
	status = adm_cache_lookup(class_name, ADM_SUPER_CTYPE, super_class_list,
	    buf_len, &adm_om_cache);
	if ((status == ADM_SUCCESS) && (super_class_list[0] != '\0'))	{
		ADM_DBG("c", ("ObjMgr: adm_find_super cache hit: %s -> %s",
		    class_name, super_class_list));
		return (ADM_SUCCESS);   /* results in super_class_list */
	}

	status = adm_find_object_class(file_name, MAXPATHLEN);
	if (status != ADM_SUCCESS) {
		strncpy(super_class_list, file_name, (size_t) buf_len);
		return (status);
	}
	adm_build_path(file_name, SUPERCLASSFILE);

	fp = fopen(file_name, READONLY);
	if (fp == NULL) {
		return (ADM_FAILURE);
	}

	while (fgets(buf, MAXSUPERCLASS, fp) != NULL) {
		if ((buf[0] == COMMENT) || ((next_class = strtok(buf, terminal))
		    == (char *)NULL))	{
			continue;
		}

		if (strcmp(next_class, class_name) != 0) {
			continue;
		}

		next_class = strtok((char *)NULL, terminal);
		if (next_class == (char *) NULL) {
			break;	/* No superclass: return null */
				/* Caller assumes obj. class */
		}
				/* Remove trailing comments */
		p = strchr(next_class, COMMENT);
		if (p != (char *)NULL)	{
			*p = '\0';
		}

				/* Do we have enough room? */
		if ((int) strlen(next_class) > buf_len){
			status = om_write_err(super_class_list, buf_len,
			    ADM_ERR_BUF2SHORT, FINDSUPERCLASS);
			break;
		}

		strcpy(super_class_list, next_class);
		status = ADM_SUCCESS;
		ADM_DBG("o",  ("ObjMgr: adm_find_super found = %s",
		    P(super_class_list)));

				/* insert result in cache */
		(void) adm_cache_insert(class_name, ADM_SUPER_CTYPE,
		    super_class_list, strlen(super_class_list) + 1,
		    &adm_om_cache);

		break;
	}

	if (fclose(fp) != 0)	{
		return (om_write_err(super_class_list, buf_len,
		    ADM_ERR_CLOSEFILE, FINDSUPERCLASS, P(file_name),
		    P(strerror(errno))));
	} else if ((status == ADM_SUCCESS) || (status == ADM_ERR_BUF2SHORT)) {
		return (status);
	} else {
		return (om_write_err(super_class_list, buf_len,
		    ADM_ERR_UNLISTEDCLASS, P(class_name), P(file_name)));
	}
}

/*
 * load_adm_path_list() loads read the environment variable "ADMINPATH".
 */

static void
load_adm_path_list(char adm_path_list[])
{
	char *source;
	static char defaultpath[] = "/usr/snadm";

	if (ADM_FIND_METHOD_DEBUG_ON)
		ADM_DBG("o", ("ObjMgr: Set adm_path_list"));

	/*
	 * ADM_DBG("c", ("ObjMgr: Initializing the cache"));
	 * adm_cache_on(&adm_om_cache);
	 */


	source = getenv(PATHENVV);

	if (source != NULL) {
		if ((int) strlen(source) > MAXPATHLIST) {
			(void) strncpy(adm_path_list, source, (size_t) MAXPATHLIST - 1);
			/* This should be an error: have no way to return */
		} else {
			(void) strcpy(adm_path_list, source);
		}
	} else {
		read_adm_pathlist_file(adm_path_list);
		if (strlen(adm_path_list) == (size_t) 0)
			(void) strcpy(adm_path_list, defaultpath);
	}
}


/*
 * Read list of paths for admind to search for methods
 * Format of file: search_path product_name version
 */
static void
read_adm_pathlist_file(char adm_path_list[])
{
	FILE		*fp;
	char		line[MAXPATHLEN + BUFSIZ];
	char		spath[MAXPATHLEN], prod[BUFSIZ], vers[BUFSIZ];
	int		i;
	size_t		cnt = 0;
	AdmProductPath	ppath[MAXOBJPATHS];
	int		rc;

	if (access(ADM_SEARCH_PATH_FILE, R_OK) != 0)
		return;

	fp = fopen(ADM_SEARCH_PATH_FILE, READONLY);
	if (fp == (FILE *) NULL)
		return;

	while (fgets(line, BUFSIZ, fp)) {
		line[strlen(line) - 1] = '\0';

		if (line[0] == '#' || line[0] == '\0')
                        continue;

		rc = sscanf(line, "%s %s %s", spath, prod, vers);
		if (rc != 3) {
			continue;
		}

		strcpy(ppath[cnt].path, spath);
		/* prod name not currently used, don't bother with strcpy */
		/* strcpy(ppath[cnt].prod, prod); */
		strcpy(ppath[cnt].vers, vers);
		cnt++;
	}

	/* sort based on "vers" */
	qsort((void *)ppath, cnt, sizeof (AdmProductPath), compare_prodvers);

	for (i = 0; i < cnt; i++) {
		if (strlen(adm_path_list) != 0)
			(void) strcat(adm_path_list, ":");
		(void) strcat(adm_path_list, ppath[i].path);
	}

	fclose (fp);
}


/*
 * adm_find_locale() returns a path to the directories containing
 * localized messages
 *
 * Returns path or null string
 */

int
adm_find_locale(char locale_path[], int buf_len)
{
	int   status;

	status = adm_find_object_class(locale_path, buf_len);
	if (status == ADM_SUCCESS) {
		if ((int) strlen(locale_path) + (int) strlen(LOCALE)
		    > buf_len)	{
			status = om_write_err(locale_path, buf_len,
			    ADM_ERR_BUF2SHORT, FINDLOCALE);
		} else	{
			adm_build_path(locale_path, LOCALE);
			status = ADM_SUCCESS;
		}
	}
	return (status);	/* Error message should be in place */
}


/*
 * adm_find_class() takes a string, and searches for a subdirectory
 * of the object directory of that name
 * Returns pathname for class directory, or null string
 *
 * Bugs: currently this assumes that there is a file
 * in the object directory with the right items in it.
 * This should be a NIS+ directory.
 * We may alter this design
 *
 * New design assumes a flat name-space: all class directories
 * are in the object directory
 *
 * Returns ADM_SUCCESS or error condition on file access
 */

int
adm_find_class(char *class_name, char *c_version, char path[], int buf_len)
{
	char  name[MAXPATHLEN];
	char  buf [MAXPATHLEN];
	char *c_name;
	char  empty = 0;
	int   status;
	int   no_version = 0;

	if (ADM_FIND_METHOD_DEBUG_ON)
		ADM_DBG("o", 
		    ("ObjMgr: Looking for class = %s.%s", P(class_name),
		    SP(c_version)));

	if ((class_name == NULL) || (class_name[0] == '\0'))	{
		return (om_write_err(path, buf_len, ADM_ERR_EMPTYSTRING,
		    CLASS));
	}

	if (c_version == (char *)NULL)	{
		c_version = &empty;
	}
	
	status = adm_find_object_class(path, buf_len);
	if (status != ADM_SUCCESS) {
		return (status);		/* Error is already recorded */
	}

	if ((int)(strlen(class_name) + strlen(c_version) + strlen(path))
	    >= buf_len) {
		return (om_write_err(path, buf_len, ADM_ERR_BUF2SHORT,
		    FINDCLASS));
	}

	if ((c_version != NULL) && (c_version[0] != '\0'))	{
		/* Class version is non-null */
		strcpy(name, class_name);
		adm_build_name(name, c_version);
		c_name = name;
	} else {	/* version is null */
		no_version = 1;
		c_name = class_name;
	}

	buf[0] = '\0';
	status = adm_cache_lookup(c_name, ADM_PATH_CTYPE, buf, buf_len,
	    &adm_om_cache);
	if (status == 0) {
		ADM_DBG("o", ("ObjMgr: Found class %s in cache: %s", 
		    P(c_name), P(buf)));
		strcpy(path, buf);
		return (ADM_SUCCESS);
	}

	if (adm_find_best(path, buf_len, class_name, c_version, buf,
	    acceptable_version_p, better_version_p) != ADM_SUCCESS) {
		status = om_write_err(path, buf_len, ADM_ERR_NOCLASS, 
		    P(c_name));
		return (status);
	}

	/* Found something: Was it right version? */
	if ((int) (strlen(path) + strlen(buf)) >= buf_len) {
		status = om_write_err(path, buf_len,
		    ADM_ERR_BUF2SHORT, FINDCLASS);
		return (status);
	}

	adm_build_path(path, buf);

	while (((status = access(path, X_OK)) < 0) && (errno == EINTR))	{
		ADM_DBG("o", ("ObjMgr: adm_find_class hit by interupt"));
	}

	if (status == ADM_SUCCESS)	{	/* insert into cache */
		(void) adm_cache_insert(c_name, ADM_PATH_CTYPE, path,
		    strlen(path) + 1, &adm_om_cache);
	}

	return (status);
}

/* adm_remove_component() takes a string, and removes the last
 * path component.  Given /etc/admin/obj/str it returns /etc/admin/obj
 * Writes over path
 */

void
adm_remove_component(char *path)
{
	char *p;

	p = strrchr(path, (int) SEPARATOR); 	/* find last '/' */
	if (p == NULL) {
		*path = '\0';			/* set path to null str	*/
	} else {
		*p = '\0';			/* zap it 		*/
	}
}


/*
 * adm_search_dir()	takes a path name and a file name and returns
 * a bit telling if the file was found in the directory
 */

int
adm_search_dir(char *path, int buf_len, char *file_name)
{
	char full_path[MAXPATHLEN];
	int len;
	int status;

/*
 *	The next test is unduly stringent: asks if there is room enough in the
 *	local buffer.  However, users slap these together in their buf
 */

	len = strlen(path) + strlen(file_name);
	if ((len > buf_len) || (len > MAXPATHLEN))	{
		return (om_write_err(path, buf_len, ADM_ERR_BUF2SHORT,
		    SEARCHDIR));
	}

	strcpy(full_path, path);
	adm_build_path(full_path, file_name);

	while (((status = access(full_path, F_OK)) < 0) && 
	    (errno == EINTR))	{
		ADM_DBG("o", ("ObjMgr: adm_search_dir hit by interupt"));
	}

	if (status < 0)	{
		if (errno == ENOENT)	{
			ADM_DBG("o", ("ObjMgr: adm_search_dir didn't find %s",
			    P(full_path)));
		} else {
			ADM_DBG("o", 
			    ("ObjMgr: adm_search_dir hit errno %d on access",
			    errno));
		}
		return (NOTFOUND);
	} else {
		ADM_DBG("o", ("ObjMgr: Found %s", P(full_path)));
		return (FOUND);
	}
}


/*
 * adm_build_path() takes two strings and glues them together with SLASH
 * It overwrites the first parameter.
 */

void
adm_build_path(char *s1, char *s2)
{
	(void) strcat(strcat(s1, SLASH), s2);
}

/*
 * adm_build_name() takes two strings and glues them together to make
 * a file name.  It overwrites the first parameter.
 */

void
adm_build_name(char *s1, char *s2)
{
	(void) strcat(strcat(s1, EXTENSION), s2);
}

/*
 * adm_find_domain() takes a class, and returns a blank
 * separated list of text domains for internationalization
 *
 * Returns ADM_SUCCESS or error condition on file access
 */

int
adm_find_domain(char *class_name, char domain[], int buf_len)
{
	char  path[MAXPATHLEN];
	char  buf [MAXPATHLEN];
	char *temp, *p;
	int   status;
	static char terminal[] = "\n\t ";	/* To skip white space */
	FILE *fp = (FILE *)NULL;

	domain[0] = '\0';

	if ((class_name == NULL) || (class_name[0] == '\0'))	{
		return (om_write_err(domain, buf_len, ADM_ERR_EMPTYSTRING,
		    CLASS));
	}

	status = adm_cache_lookup(class_name, ADM_DOMAIN_CTYPE, domain, buf_len,
	    &adm_om_cache);
	if (status == ADM_SUCCESS)	{
		ADM_DBG("o", ("ObjMgr: Find domain %s in cache", P(domain)));
		return (ADM_SUCCESS);
	}

	status = adm_find_object_class(path, buf_len);
	if (status != ADM_SUCCESS) {
		return (status);		/* Error is already recorded */
	}

	adm_build_path(path, CLASS_2_DOMAIN);

	fp = fopen(path, READONLY);

	if (fp == NULL) {
		/*
		return (om_write_err(domain, buf_len, ADM_ERR_OPENFILE,
		    FINDDOMAIN, P(path), P(strerror(errno))));
		*/
		strcpy(domain, "");
		return (ADM_SUCCESS);
	}

	while (fgets(buf, MAXPATHLEN, fp) != NULL) {
		p = &buf[0];
		if ((buf[0] == COMMENT) || ((temp = adm_strtok(&p, terminal))
		    == (char *)NULL))	{
			continue;
		}
		buf[MAXPATHLEN-1] = '\0';

		if (strcmp(buf, class_name) != 0) {
			continue;
		}

		temp = p;	/* p points to next token */

				/* Remove trailing comments */
		p = strchr(temp, COMMENT);
		if (p != (char *)NULL)	{
			*p = '\0';
		}
				/* Remove trailing newline */
		p = strrchr(temp, '\n');
		if (p != (char *)NULL)	{
			*p = '\0';
		}
				/* Do we have enough room? */
		if ((int) strlen(temp) > buf_len){
			status = om_write_err(domain, buf_len,
			    ADM_ERR_BUF2SHORT, FINDDOMAIN);
			break;
		}

		strcpy(domain, temp);

		status = ADM_SUCCESS;
		ADM_DBG("o",  ("ObjMgr: Found domain %s", P(domain)));

				/* insert result in cache */
		(void) adm_cache_insert(class_name, ADM_DOMAIN_CTYPE, domain,
		    strlen(domain) + 1, &adm_om_cache);

		break;	/* We found it: let's quit */
	}

	if (fclose(fp) != 0)	{
		return (om_write_err(path, buf_len, ADM_ERR_CLOSEFILE,
		    FINDDOMAIN, P(path), P(strerror(errno))));
	} else if ((status == ADM_SUCCESS) || (status == ADM_ERR_BUF2SHORT)) {
		return (status);
	} else {
		return (om_write_err(domain, buf_len, ADM_ERR_UNLISTEDCLASS,
		    P(class_name), P(path)));
	}
}

/*
 * adm_find_method() takes two strings: a class name and a method
 * name.  It searches for an image to run, starting with the
 * class directory and working upwards in order to inherit methods
 *
 * It calls adm_find_class to return a hierarchy.
 *
 * adm_find_method is a front end to find_method and search super class.  
 *
 * Bugs
 * 	Does not check that file is executable: has wrong uid to do so
 * For versions, we pass in strings with the versions
 */

int
adm_find_method(
	char *class_name,
	char *c_version,
	char *method_name,
	char  path[],
	int   buf_len)
{
	int i, err;
	char buf[MAXPATHLEN];	/* Build class version for error msg */


ADM_DBG("o", ("ObjMgr: Look for method %s, in class %s.%s",
    P(method_name), P(class_name), SP(c_version)));

	if ((class_name == NULL) || (class_name[0] == '\0'))	{
		return (om_write_err(path, buf_len, ADM_ERR_EMPTYSTRING,
		    CLASS));
	}

	if ((method_name == NULL) || (method_name[0] == '\0'))	{
		return (om_write_err(path, buf_len, ADM_ERR_EMPTYSTRING,
		    METHOD));
	}

	path[0] = '\0';

	/* Tack version on the end of class name? */
	strcpy(buf, class_name);
	if ((c_version != (char *)NULL) && (*c_version)) {
		adm_build_name(buf, c_version);
	}
	adm_build_path(buf, method_name);
	buf[MAXPATHLEN-1] = '\0';

	/* try the cache */
	err = adm_cache_lookup(buf, ADM_PATH_CTYPE, path, buf_len, 
	    &adm_om_cache);
	if (err == ADM_SUCCESS)	{
		if (path[0] == '\0')	{
			ADM_DBG("o", ("ObjMgr: cache returned empty"));
			ADM_DBG("C", ("ObjMgr: cache returned empty"));
		} else { 
			ADM_DBG("o", ("ObjMgr: Found method %s in cache %s",
			    P(method_name), P(path)));
			return (ADM_SUCCESS);
		}
	}

	for (i = 0; i < MAXOBJPATHS - 1; i++) {
		if ((adm_obj_path = adm_obj_pathlist[i]) == NULL)
			break;

		err = find_method(class_name, c_version, method_name, path, buf_len);
	
		if (err != ADM_SUCCESS)	{
			/* We didn't find it here: does it have a superclass? */
			if (strcmp(class_name, OBJECT) == 0)	{
				if (ADM_FIND_METHOD_DEBUG_ON)
				    ADM_DBG("o", ("ObjMgr: Hit class object"));
				continue;
			}

			/* Let's try in the superclass */
			err = search_super_class(class_name, c_version, method_name,
			    path, buf_len);
		}

		if (err == ADM_SUCCESS)
			break;
	}

	if (err == ADM_SUCCESS)	{
		/* insert result in cache */
		(void) adm_cache_insert(buf, ADM_PATH_CTYPE, path,
		    strlen(path) + 1, &adm_om_cache);
		return (ADM_SUCCESS);		/* path holds result */
	} else {
		ADM_DBG("o", ("Error finding method"));
		if (err != ADM_ERR_NOMETHOD) {
			return (err);		/* path holds error */
		} else {
			om_write_err(path, buf_len, ADM_ERR_NOMETHOD,
			    P(method_name), P(class_name), P(c_version));
			return (ADM_ERR_NOMETHOD);
		}
	}
}

/*
 * See if the method is defined in this class
 * adm_find_method takes the burden of an inheritance check
 */

static int
find_method(
	char *class_name,
	char *c_version,
	char *method_name,
	char  path[],
	int   buf_len)
{
	char method_path[MAXPATHLEN];
	int err;

ADM_DBG("o", ("ObjMgr: Find method %s in class %s(%s)", P(method_name),
	P(class_name), P(c_version)));

	if (ADM_FIND_METHOD_DEBUG_ON)
		ADM_DBG("o", ("ObjMgr: Look for method %s in class %s(%s)",
		    P(method_name), P(class_name), SP(c_version)));

	
	err = adm_find_class(class_name, c_version, path, buf_len);

	if (err != ADM_SUCCESS)
	{
		ADM_DBG("o", ("ObjMgr: find_method could not find class %s",
		    P(class_name)));
		return (err);	/* Error should have been recorded */
	}

	if ((int) (strlen(path) + strlen(method_name)) > buf_len) {
		return (om_write_err(path, buf_len, ADM_ERR_BUF2SHORT,
		    FINDMETHOD));
	}

	strcpy(method_path, path);
	adm_build_path(method_path, method_name);

	while (((err = access(method_path, X_OK)) < 0) && (errno == EINTR)) {
		ADM_DBG("o", ("ObjMgr: adm_find_method hit by interupt"));
	}

	if (err == ADM_SUCCESS) {
		ADM_DBG("o", ("ObjMgr: found method %s", P(method_path)));
		strcpy(path, method_path);
		return (ADM_SUCCESS);
	} else {
		if (errno != ENOENT)	{
			ADM_DBG("o", 
			    ("ObjMgr: find_method access: errno %d", errno));
		/* XXX Need new error message */
		}
		ADM_DBG("o", ("ObjMgr: Did not find it: errno = %d", errno));
	}

	return (ADM_ERR_NOMETHOD);
}


/*
 * Keeps a list of the classes seen
 * We look at each class in turn, and add it's super classes to tail of list
 *
 * We have two lists: 
 *	List of classes we've seen
 *	List of superclasses of current class
 * Each list is ':' spearated
 */
int
search_super_class(
	char *class_name,
	char *c_version,
	char *method_name,
	char  path[],
	int   buf_len)
{
		/* Data structure: list of classes, with ":" between */
	char *classes, *p, *next;	/* pointers into the strings */
	int size = 512;			/* Size of buffer */
	int found = 0;
	int err = 0;
	char super_class[MAXPATHLEN];	/* The super classes of current class */
	char super_class_list[MAXPATHLEN]; /* All superclasses we've seen */
	char next_class[MAXPATHLEN];	/* The current class to consider */

	int last_class = 0;		/* Any more classes to look at? */
	int junk;			/* place holder */

	ADM_DBG("o", ("ObjMgr: Search Super looks for %s in class %s(%s)", 
	    P(method_name), P(class_name), P(c_version)));

	classes = malloc(size);
	p = classes;
	if (!classes)	{
		err = ADM_ERR_NOMEM;
	} else {
		*classes = (char)0;
	}

	if (adm_find_super_class(class_name, super_class_list, MAXPATHLEN) ||
	    !super_class_list[0]) {
		ADM_DBG("o", ("ObjMgr: Search Super: %s(%s) has no superclass", 
		    P(class_name), P(c_version)));
		last_class = 1;
	} else {
		ADM_DBG("o", ("ObjMgr: Found superclasses %s", 
		    P(super_class_list)));
	}

	/* While there are classes to consider */
	while (1) {

		ADM_DBG("o", ("ObjMgr: add classes -%s- to list -%s-", 
		    P(super_class_list), P(classes)));

		if (!super_class_list[0]) 
			if (!(p && *p))
				break;

		next = &super_class_list[0];

		/* Pull off the classes one by one: check and perhaps add */
		for (last_class = 0; !last_class;) {
		    next = get_next_class(next, next_class, &last_class);
			if (!search_class_string(classes, next_class)) {
				classes = add_string(next_class, classes, 
				    &p, &size);
				if (!classes) {
					err = ADM_ERR_NOMEM;
					p = (char *)NULL;
					break;
				}
			}
		}

		if (!p || !(*p))	/* Nothing left */
			break;

		p = get_next_class(p, super_class, &junk);
		if (!super_class[0]) {
			break;
		}

		if (!find_method(super_class, "", method_name, path, buf_len)) {
			found = 1;
			break;
		}

		if (adm_find_super_class(super_class, super_class_list,
		    MAXPATHLEN)) {
			err = ADM_ERR_NOMEM;
			break;
		}
	}

	if (classes)
		(void) free(classes);

	if (found)
		return ADM_SUCCESS;
	else if (err == ADM_ERR_NOMEM) {
		om_write_err(path, buf_len, ADM_ERR_NOMEM);
		return ADM_ERR_NOMEM;
	} else 
		return ADM_ERR_NOMETHOD;
}


/*
 *	add_string
 *
 *	We keep our list of classes to look for in a string with
 *	place marker "pos".   This appends to string: must
 *		make room if we have none - and then
 *			update position
 */
char *
add_string (char *new_string, char *list, char **pos, int *size)
{
	int length;

			/* The following will usually happen at most once */
	while (strlen(list) + strlen(new_string) + 1 >= (size_t)*size) {
		*size = (*size)*2;
		length = (int) (*pos - list);
		list = realloc(list, (unsigned)*size);
		if (!list) {
			printf("Error: ran out of memory\n");
			return list;
		}
		*pos = list + length;
	}

	strcat(list, ":");
	strcat(list, new_string);

	return list;
}

/*
 * get_next_class
 *
 * Search a list for the next work.  
 * Copy name into buffer buf
 * Return tail of list
 */
char *
get_next_class(char *p, char buf[], int *done)
{
	int i;

	buf[0] = (char)0;
	*done = 0;

	if (p && *p)	{
		while(*p == ':')
			p++;
		for(i=0; (*p != ':') && (*p != (char)0); buf[i++] = *p, p++)
			;
		buf[i] = (char)0;	
				/* Is there anything left? */
		if (*p == ':')
			p++;
	} /* p is non-null */ 

	if ((!p) || (!*p))
		*done = 1;

	return p;
}

/*
 * search_class_string
 *
 * Is the new class I'm about to add in this string?
 * Glorified sequence of calls to strstr
 */
int
search_class_string(const char *text, const char *pattern)
{
	int c;
	int found = 0;
	char *cl;

	cl = (char *)text;

	while (cl = strstr(cl, pattern)) {

		/* Did we find anything? */
		if (*cl == (char)0)
			break;

		/* Was this a match? */
		cl = cl + strlen(pattern);
		c = *cl;
		if ((c == ':') || (c == (char)0)) {
			found = 1;
			break;
		}

		/* Anything left to check? */
		if (*cl == (char)0)
			break;
	}
		
	return found;
}
