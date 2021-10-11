#ident "@(#)string_op.c 1.12 91/09/26 SMI"

/*
 * Copyright (c) 1991-1994 by Sun Microsystems, Inc.
 */


/*
 * *************************************************************************
 *
 * this file holds all the string manipulation functions
 * that aren't provided in the regular string manipulation library
 * and any other "convenient/frequently used" functions that sorta
 * involve strings that aren't provided as part of the OS libraries
 *
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <arpa/nameser.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <regexpr.h>
#include <limits.h>
extern char *loc1, *loc2, *locs;
extern int nbra, regerrno, reglength;
extern char *braslist[], *braelist[];
#include <errno.h>
#include "string_op.h"

#define	WORKBUFLEN 2048
#define TMP_FILENAME "/admtmp_XXXXXX"
#ifndef TESTING
/*
 * ************************************************************************
 * function : (char *) strrstr(cs, ct)
 *
 * Returns a pointer to the first occurrence of string ct in cs starting at
 * the end of the string and searching in reverse order.
 *
 * RETURN VALUE char *   :  points to the first occurence of ct in cs
 *
 * *************************************************************************
 */
char *
strrstr(register char *s1, register char *s2) {
    char *last = s1 + strlen(s1);
    int s2len = strlen(s2);

    if (s2len == 0)
	return(s1);
    while (s1 <= last) {
	if (strncmp(last, s2, s2len) == 0)
	    return(last);
	last--;
    }
    return(s1);
}
/*
 * ************************************************************************
 * function : (char *) strlwr()
 *
 * This function takes a string and converts it to lower case.
 *
 * WARNING : it is a destructive function and will change
 * 		string passed to it.
 *
 * PARAMETERS : the string to convert to lower case, which must be null
 *		terminated.
 *
 * RETURN VALUE char *   :  points to the lower case string
 *
 * *************************************************************************
 */
char *
strlwr(char *string)
{
	char *p;

	p = string;

	while (*p) {
		if (isupper(*p))
			*p = tolower(*p);
		p++;
	}
	return (string);
}

/*
 * ***********************************************************************
 * function : (char *) strupr()
 *
 * This function takes a string and converts it to upper case.
 *
 * WARNING : it is a destructive function and will change
 * 		string passed to it.
 *
 * PARAMETERS : the string to convert to upper case must be null terminated.
 *
 * RETURN VALUE char *   :  pointer to the upper case string
 *
 * *************************************************************************
 */
char *
strupr(char *string)
{
	char *p;

	p = string;

	while (*p) {
		if (islower(*p))
			*p = toupper(*p);
		p++;
	}
	return (string);
}

/*
 * return a newly allocated duplicate of the input string,
 * modified if necessary to make it a comment by placing
 * a sharp ('#') in column 1
 * (unless the input pointer is NULL or the input string
 * is zero length, in which case return a zero length string)
 *
 * inverse of struncomment()
 */
char *
strcomment(const char *instring)
{
	char c;
	const char *p;
	char *commentstring;

	/* check for null input parameter */
	if ((! instring) || (*instring == '\0'))
		return (strdup(""));
	p = instring;
	while ((c = *(p++)) && ((c == ' ') || (c == '\t')));

	if (c != '#') {
		commentstring = (char *)malloc(strlen(instring) + 1);
		commentstring[0] = '#';
		strcpy(&(commentstring[1]), instring);
	} else {
		commentstring = strdup(instring);
	}
	return (commentstring);
}

/*
 * return a newly allocated duplicate of the input string,
 * modified if necessary to make it _not_ a comment by
 * removing all the leading sharps and the white space
 * to the left of or between them
 * (unless the input pointer is NULL or the input string
 * is zero length, in which case return a zero length string)
 *
 * inverse of strcomment()
 */
char *
struncomment(const char *instring)
{
	char c;
	const char *p;
	const char *realthing;

	/* check for null input parameter */
	if ((! instring) || (*instring == '\0'))
		return (strdup(""));
	p = instring;
	realthing = p;
	while ((c = *(p++)) && ((c == ' ') || (c == '\t') || (c == '#')))
		if (c == '#')
			realthing = p;

	return (strdup(realthing));
}

/*
 * Utility mini-function used by next routine.
 * This routine is not available to includers
 * of this package, but only locally.
 */
static void
remove_component(char *path)
{
	char *p;

	p = strrchr(path, '/'); 		/* find last '/' 	*/
	if (p == NULL) {
		*path = '\0';			/* set path to null str	*/
	} else {
		*p = '\0';			/* zap it 		*/
	}
}

/*
 * Function to modify a line in a file.
 *
 * lineidentifier may be a "regular expression" string
 *
 * modtype is one of a small set of possibilities
 * which are defined in the header file
 * currently only 'comment' and 'uncomment' are supported
 *
 * returns:
 *  0 for success,
 *  >0 for system-supplied errno code which may or may not be in errno any more,
 *  <0 for user-supplied errno-like code, including
 *   -ENOENT for "no matching line found"
 *   -EEXIST for "file not modified since requested change had already been
 *		done"
 *   -EINVAL 'modtype' argument requests unknown operation
 */
int
modify_line_in_file(const char *filename, const char *lineidentifier,
    const int modtype)
{
	FILE *ifp = NULL, *ofp = NULL;
	char *tmp;
	char linebuff[WORKBUFLEN];
	char regexprbuff[WORKBUFLEN];
	int linefound = 0;
	char *modifiedline;
	int serrno;
	int status;
	int fd = -1; /* Make it -1 so if we never lock, unlock returns fast */
	const char *tfilename;

#ifdef DEBUG
	fflush(stdout);
	printf("going to modify_line_in_file(\"%s\", \"%s\", %d)\n",
	    filename, lineidentifier, modtype);
	fflush(stdout);
#endif
	/*
	 * compile the regular expression we'll use while matching
	 * note that it may not contain any meta-characters, in which
	 * case it functionally becomes an strstr()
	 */
	if (compile(lineidentifier, regexprbuff, &(regexprbuff[WORKBUFLEN]))
	    == NULL)
		return (1000+regerrno);

	/*
	 * Generate temporary file name to use.  We make sure it's in the same
	 * directory as the file we're processing so that we can use rename to
	 * do the replace later.  Otherwise we run the risk of being on the
	 * wrong filesystem and having rename() fail for that reason.
	 */
	tfilename = filename;
	if (trav_link(&tfilename) == -1)
		return (errno);
	tmp = strdup(tfilename);
	remove_component(tmp);
	if (strlen(tmp) == 0)
		strcat(tmp, ".");
	realloc(tmp, strlen(tmp) + strlen(TMP_FILENAME) + 1);
	strcat(tmp, TMP_FILENAME);
	(void) mktemp(tmp);
	if ((ofp = fopen(tmp, "w")) == NULL) {
	        free (tmp);
	        return (errno);
	}

	if (lock_db(filename, F_WRLCK, &fd) == -1) {
		(void) fclose(ofp);
		(void) unlink(tmp);
	        free (tmp);
		return (-ETXTBSY);
	}

	/*
	 * Process file, line at a time.  When we know that we've done the
	 * modification, just pass the rest of the data through.
	 *
	 * Since we're modifying but never adding, we expect to not get an
	 * error opening the file.
	 *
	 * If the modified line is the same as the original line, return
	 * to our caller but just keep the original file intact
	 * rather than writing a new one that's identical to it.
	 * Return -EEXIST in this case so the caller can distinguish it.
	 */

	if ((ifp = fopen(filename, "r")) == NULL) {
		serrno = errno;
		(void) fclose(ifp);
		(void) fclose(ofp);
		(void) unlink(tmp);
		(void) unlock_db(&fd);
		(void) free(tmp);
		return (serrno);
	}

#ifdef DEBUG
	printf("file was opened and temp file was created, now read file\n");
	fflush(stdout);
#endif
	while (fgets(linebuff, sizeof (linebuff), ifp) != NULL) {
#ifdef DEBUG
	printf(".");
	fflush(stdout);
#endif
		if (!linefound && (step(linebuff, regexprbuff) != 0)) {
#ifdef DEBUG
	fflush(stdout);
	printf("\nfound line containing \"%s\"--\n before: %s",
	    lineidentifier, linebuff);
	fflush(stdout);
#endif
			++linefound;
			switch (modtype) {
			case MLIF_COMMENT:
				modifiedline = strcomment(linebuff);
#ifdef DEBUG
	fflush(stdout);
	printf(" after %s: %s", "MLIF_COMMENT", modifiedline);
	fflush(stdout);
#endif
				if (strcmp(modifiedline, linebuff) == 0) {
					(void) fclose(ifp);
					(void) fclose(ofp);
					(void) unlink(tmp);
					(void) unlock_db(&fd);
					(void) free(tmp);
					return (-EEXIST);
				}
				strcpy(linebuff, modifiedline);
				(void) free(modifiedline);
				break;
			case MLIF_UNCOMMENT:
				modifiedline = struncomment(linebuff);
#ifdef DEBUG
	fflush(stdout);
	printf(" after %s: %s", "MLIF_UNCOMMENT", modifiedline);
	fflush(stdout);
#endif
				if (strcmp(modifiedline, linebuff) == 0) {
					(void) fclose(ifp);
					(void) fclose(ofp);
					(void) unlink(tmp);
					(void) unlock_db(&fd);
					(void) free(tmp);
					return (-EEXIST);
				}
				strcpy(linebuff, modifiedline);
				(void) free(modifiedline);
				break;
			default:
				(void) fclose(ifp);
				(void) fclose(ofp);
				(void) unlink(tmp);
				(void) unlock_db(&fd);
				(void) free(tmp);
				return (-EINVAL);
			}
		}
		if (fputs(linebuff, ofp) == EOF) {
			serrno = errno;
			(void) fclose(ifp);
			(void) fclose(ofp);
			(void) unlink(tmp);
			(void) unlock_db(&fd);
			(void) free(tmp);
			return (serrno);
		}
	}

#ifdef DEBUG
	printf("\ndone writing modified file, now switch files and return\n");
	fflush(stdout);
#endif
	(void) fclose(ifp);
	(void) fclose(ofp);
	if (linefound) {
		/* we did something */
		if (rename(tmp, filename) != 0) {
			serrno = errno;
			(void) unlink(tmp);
			(void) unlock_db(&fd);
			(void) free(tmp);
			return (serrno);
		} else {
			(void) unlock_db(&fd);
			(void) free(tmp);
			return (0);
		}
	} else {
		/* we never found a line that matched the pattern */
		(void) unlink(tmp);
		(void) unlock_db(&fd);
		(void) free(tmp);
		return (-ENOENT);
	}
}

/*
 * **********************************************************************
 * function : (void) itoa()
 *
 * This function takes an integer and converts it to a string.
 *
 * PARAMETERS : the integer "number" is put into the character array
 *		pointed to by "string"
 *
 * RETURN VALUE : none
 *
 * *************************************************************************
 */
void
itoa(const int number, char *string)
/* convert binary integer number to equivalent readable ascii string */
{
	(void) sprintf(string, "%d", number);
}

/*
 * this routine is defined in the system header files,
 * but wasn't actually present in the system libraries for jup_dev3
 * and still didn't work right in the system libraries for JA1.2
 *
 * so we'll fake one here -- hopefully we can remove this someday
 */
/*
 * removed 10/15/96
 *
 *unsigned long
 *inet_network(const char *ipaddr)
 *{
 *	struct in_addr ia;
 *
 *	ia.s_addr = inet_addr(ipaddr);
 *	if (ia.s_addr == -1)
 *		return (-1);
 *
 *	if (ia.S_un.S_un_b.s_b1 < 128) {
 *		ia.S_un.S_un_b.s_b2 = '\0';
 *		ia.S_un.S_un_b.s_b3 = '\0';
 *		ia.S_un.S_un_b.s_b4 = '\0';
 *		return (ia.s_addr);
 *	} else if (ia.S_un.S_un_b.s_b1 < 192) {
 *		ia.S_un.S_un_b.s_b3 = '\0';
 *		ia.S_un.S_un_b.s_b4 = '\0';
 *		return (ia.s_addr);
 *	} else if (ia.S_un.S_un_b.s_b1 < 224) {
 *		ia.S_un.S_un_b.s_b4 = '\0';
 *		return (ia.s_addr);
 *	} else {
 *		return (-1);
 *	}
 *}
 */

/*
 * input arg is any IP address in the form of a
 * dotted decimal ASCII string
 *
 * for compatibility, the routine also attempts to accept
 * incomplete IP network addresses in the "old" form
 * that omitted the subnet/host octets altogether rather
 * than carrying them around as zeros
 *
 * note that this compatibility with the "old" way of
 * expressing IP network addresses means this routine will
 * _not_ work right for the abbreviated net.host forms
 * of system IP addresses
 *
 * successful return value is a pointer to a freshly malloc()ed
 * dotted decimal ASCII string which is the IP address
 * of the IP "network" (e.g. the host part [and subnet
 * part if any] are "zeros")
 *
 * if the input arg didn't appear valid or couldn't be
 * processed (or perhaps was the "broadcast" address
 * 255.255.255.255 which many IP address processing routines
 * don't properly distinguish from the error return -1),
 * the return value will be NULL
 */
char *
ip_addr_2_netnum(const char *ip_addr)
{
	struct in_addr ia;
	char input_work_buffer[25];
	char *traverse;
	char *reshown;
	int dot_count = 0;
	char c;

	/*
	 * for compatibility with the "old" way of carrying
	 * around IP network numbers that was used in /etc/netmasks
	 * in SunOS 4.x, try to accept input addresses
	 * with fewer than four octets by setting the "missing"
	 * octets to 0
	 */
	strcpy(input_work_buffer, ip_addr);
	traverse = input_work_buffer;
	while (c = *(traverse++)) {
		if (c == '.')
			++dot_count;
	}
	if (dot_count == 3)
		/* ok as is */;
	else if (dot_count == 2)
		strcat(input_work_buffer, ".0");
	else if (dot_count == 1)
		strcat(input_work_buffer, ".0.0");
	else if (dot_count == 0)
		strcat(input_work_buffer, ".0.0.0");
	else
		return (NULL);

	/*
	 * convert ASCII->binary, then convert back binary->ASCII
	 * the real point is to use a "standard" system routine
	 * [inet_network()] to isolate the network part of the IP address
	 */
	if ((ia.s_addr = inet_network(input_work_buffer)) == -1)
		return (NULL);
	reshown = inet_ntoa(ia);
	if (*reshown == '0') {
		/* convert possible error return to a more usable form */
		*reshown = '\0';
		return (NULL);
	}
	/*
	 * malloc() the returned string into dynamic memory to be
	 * absolutely sure that repeated calls to this routine
	 * don't overwrite previously returned results
	 */
	reshown = strdup(reshown);
	return (reshown);
}
#endif /* ! TESTING */
