
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains private functions for handling diagnostic
 *	(or tracing) information in the administrative framework.  The
 *	functions defined here are exported to framework developers, but
 *	not the general public.
 *
 *	BUGS: Access to tracing handles is not thread-safe.  Each handle
 *	      should include a mutex lock to guard against concurrent
 *	      access to the handle.
 *
 *******************************************************************************
 */

#ifndef _adm_diag_impl_c
#define _adm_diag_impl_c

#pragma	ident	"@(#)adm_diag_impl.c	1.4	92/02/26 SMI"

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_om_impl.h"

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_CATCATS( n, categories1, categories2, ... ):
 *
 *	Concatenate the n specified lists of categories into one
 *	list.  If successful, this routine returns a pointer to the
 *	concatenated list.  Otherwise, it returns NULL.
 *
 *	NOTE: The caller is expected to free the space used by the
 *	      concatenated list when the list is no longer needed.
 *
 *---------------------------------------------------------------------
 */

char *
adm_diag_catcats(
	u_int n,
	...
	)
{
	boolean_t prev_cats;	/* Previous categories in the list? */
	u_int bufest;		/* Size estimate of concatenated list */
	u_int buflen;		/* Actual length of concatenated list */
	char *catcats;		/* Concatenated list of categoires */
	char *categories;
	int len;
	va_list cat_args;
	u_int i;

	/*
	 * Estimate the size of the buffer needed to hold the
	 * concatenated list.
	 */

	bufest = 1;		/* Need space for at least a NULL */
	prev_cats = B_FALSE;
	va_start(cat_args, n);
	for (i = 0; i < n; i++) {
		categories = va_arg(cat_args, char *);
		if (categories != ADM_NOCATS) {
			bufest += strlen(categories) + (prev_cats ? 1 : 0);
			prev_cats = B_TRUE;
		}
	}
	va_end(cat_args);

	/*
	 * Allocate the buffer to hold the concatenated lists.
	 */

	catcats = malloc(bufest);
	if (catcats == NULL) {
		return(NULL);
	}

	/*
	 * Form the concatenated list.
	 */

	buflen = 0;
	prev_cats = B_FALSE;
	va_start(cat_args, n);
	for (i = 0; i < n; i++) {
		categories = va_arg(cat_args, char *);
		if (categories != ADM_NOCATS) {
			if (prev_cats) {
				*(char *)(catcats + buflen++) = *ADM_CATSEP;
			}
			len = sprintf((char *)(catcats + buflen), "%s",
				      categories);
			if (len < 0) {
				free(catcats);
				return(NULL);
			}
			buflen += len;
			prev_cats = B_TRUE;
		}
	}
	va_end(cat_args);
	*((char *)(catcats + buflen)) = NULL;

	return(catcats);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_CATCMP( and_flag, categories1, categories2 ):
 *
 *	Determine if any of the categories listed in category list
 *	categories1 are also listed in category list categories2.
 *
 *	If and_flag is set to B_TRUE, this routine checks to see
 *	if all of the categories listed in categories1 are also in
 *	categories2.  If so, the routine returns B_TRUE, otherwise
 *	it returns B_FALSE.
 *
 *	If and_flag is set to B_FALSE, this routine checks to see
 *	if any of the categories listed in categories1 are also listed
 *	in categories2.  If so, this routine returns B_TURE, otherwise
 *	it returns B_FALSE.
 *
 *	NOTE: If categories1 is set to ADM_NOCATS, this routine
 *	      always returns B_TRUE.
 *
 *---------------------------------------------------------------------
 */

boolean_t
adm_diag_catcmp(
	boolean_t and_flag,
	char *categories1,
	char *categories2
	)
{
	char *catptr;		/* Ptr to next category to search for */
	u_int catlen;		/* Length of next category */
	char *newptr;		/* Ptr to following category to search for */

	if (categories1 == ADM_NOCATS) {
		return(and_flag);
	}
	if (categories2 == ADM_NOCATS) {
		return(B_FALSE);
	}

	/*
	 * Successively try each category listed in categories1 and
	 * see if it's listed in categories2.
	 */

	catptr =  categories1;
	while (adm_diag_nextcat(&catptr, &catlen, &newptr)) {

		if (adm_is_allcats(catptr, catlen) && !and_flag) {
			return(B_TRUE);
		}

		switch(adm_diag_catin(catptr, catlen, categories2)) {

		    case B_TRUE:

			if (!and_flag) {
				return(B_TRUE);
			}
			break;

		    case B_FALSE:

			if (and_flag) {
				return(B_FALSE);
			}
			break;
		}

		catptr = newptr;
	}

	return((and_flag ? B_TRUE : B_FALSE));
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_CATIN( cat_name, catlen, categories ):
 *
 *	Determine if the category name "cat_name" (which has length
 *	"catlen") is also listed in category list "categories".  If
 *	so, return B_TRUE, otherwise return B_FALSE.
 *
 *---------------------------------------------------------------------
 */

boolean_t
adm_diag_catin(
	char *cat_name,
	u_int catlen,
	char *categories
	)
{
	char *this_cat;		/* Ptr. to current category in categories list */
	u_int this_len;		/* Length of current category name */
	char *next_cat;		/* Ptr. to next category in categories list */

	if ((cat_name == NULL) || (categories == NULL)) {
		return(B_FALSE);
	}

	/*
	 * Successively compare cat_name to each category in categories.
	 */

	this_cat = categories;
	while (adm_diag_nextcat(&this_cat, &this_len, &next_cat)) {
		if ((adm_is_allcats(this_cat, this_len)) ||
		    ((this_len == catlen) &&
		     (strncmp(cat_name, this_cat, catlen) == 0))) {
			return(B_TRUE);
		}
		this_cat = next_cat;
	}

	return(B_FALSE);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_MSG2STR( msgtime, categories1, categories2, message,
 *		     buflen, bufp, lenp ):
 *
 *	Convert the specified tracing message (and related categories)
 *	to string format.  The string format of tracing information is:
 *
 *		#<time>:<catlen>:<msglen>: <categories> <message>\n
 *
 *	where <time> is the time(2) of the tracing message, <catlen> is
 *	the total number of bytes in the <categories> string (or "-" if
 *	both category specifications are ADM_NOCATS), <msglen> is the total
 *	number of bytes in the <message> string (or "-" if the message
 *	specification is NULL), <categories> is a comma-separated list
 *	of diagnostic categories to which the tracing message belongs
 *	(the concatenation of both category lists), and <message> is
 *	the tracing message.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS
 *	and writes the specified tracing information (in string format)
 *	to the buffer pointed to by bufp (along with an appended NULL).
 *	The length of the converted string is also returned in *lenp.
 *	If the specified buffer length (buflen) is insufficent to
 *	hold the converted string, this routine returns ADM_ERR_TOOLONG.
 *	
 *	NOTE: The space preceding the <categories> or <message>
 *	      fields will not be included in the tracing specification
 *	      if either of those fields is undefined (i.e. the
 *	      corresponding length is set to "-").
 *
 *---------------------------------------------------------------------
 */

int
adm_diag_msg2str(
	time_t msgtime,
	char *categories1,
	char *categories2,
	char *message,
	u_int buflen,
	char *bufp,
	u_int *lenp
	)
{
	u_int estimate;		/* Estimate of tracing info string length */
	u_int actual;		/* Actual length of tracing info string */
	u_int msglen;		/* Length of tracing message */
	u_int catlen;		/* Length of categories string */

	/*
	 * Make sure that tracing info (with terminating NULL) will
	 * fit in buffer.
	 */

	estimate = adm_diag_strsize(msgtime, categories1, categories2,
				    message, &catlen, &msglen);
	if (estimate >= buflen) {
		return(ADM_ERR_TOOLONG);
	}

	/*
	 * Write the tracing message (in string format) to the
	 * specified buffer.
	 */

	actual = sprintf(bufp, "%s%ld:", ADM_DIAGMARKER, msgtime);
	if ((categories1 != ADM_NOCATS) || (categories2 != ADM_NOCATS)) {
		actual += sprintf((char *)(bufp + actual), "%u:", catlen);
	} else {
		actual += sprintf((char *)(bufp + actual), "-:");
	}
	if (message != NULL) {
		actual += sprintf((char *)(bufp + actual), "%u:", msglen);
	} else {
		actual += sprintf((char *)(bufp + actual), "-:");
	}
	if ((categories1 != ADM_NOCATS) || (categories2 != ADM_NOCATS)) {
		*(char *)(bufp + actual) = ' ';
		actual += 1;
	}
	if (categories1 != ADM_NOCATS) {
		actual += sprintf((char *)(bufp + actual), "%s", categories1);
	}
	if (categories2 != ADM_NOCATS) {
		if (categories1 != ADM_NOCATS) {
			*(char *)(bufp + actual) = *ADM_CATSEP;
			actual += 1;
		}
		actual += sprintf((char *)(bufp + actual), "%s", categories2);
	}
	if (message != NULL) {
		*(char *)(bufp + actual) = ' ';
		actual += 1;
		memcpy((char *)(bufp + actual), message, msglen);
		actual += msglen;
	}
	*(char *)(bufp + actual) = '\n';
	actual += 1;
	*(char *)(bufp + actual) = NULL;

	if (lenp != NULL) {
		*lenp = actual;
	}
	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_NEXTCAT( categoriespp, catlenp, nextcatpp ):
 *
 *	Find the first category name in the category list pointed
 *	to by *categoriespp.  If a category is found, this routine
 *	advances *categoriespp to point to the start of the category
 *	name, sets *catlenp to the number of bytes in the category
 *	name, and sets *nextcatpp so that it may be used to get
 *	the next category in the list (by including this value as
 *	the first argument of the routine).  The routine also
 *	returns B_TRUE as its value.
 *
 *	If there are no categories in the list, the routine
 *	returns B_FALSE as its value.
 *
 *---------------------------------------------------------------------
 */

boolean_t
adm_diag_nextcat(
	char **categoriespp,
	u_int *catlenp,
	char **nextcatpp
	)
{
	char *cat_name;		   /* Pointer to first category name in list */
	boolean_t no_cat_found;	   /* No category found in list, yet? */
	char *end_cat;		   /* End of category name */
	boolean_t looking_for_end; /* Still looking for end of category name? */
	char *cp;

	if ((categoriespp == NULL) || (*categoriespp == NULL)) {
		return(B_FALSE);
	}

	/*
	 * Skip over white space and empty category name specifications
	 * to find the start of the first real category name.
	 */

	cat_name = *categoriespp;
	no_cat_found = B_TRUE;
	while (no_cat_found) {
		if (*cat_name == NULL) {
			return(B_FALSE);
		}
		if ((isspace((int)(*cat_name))) || (*cat_name == *ADM_CATSEP)) { 
			cat_name++;
		} else {
			no_cat_found = B_FALSE;
		}
	}

	/*
	 * Find separator or end-of-list that indicates the end of
 	 * category name.
	 */

	end_cat = cat_name;
	cp = cat_name + 1;
	looking_for_end = B_TRUE;
	while(looking_for_end) {
		if ((*cp == NULL) || (*cp == *ADM_CATSEP)) {
			looking_for_end = B_FALSE;
		} else if (isspace((int)*cp)) {
			cp++;
		} else {
			end_cat = cp++;
		}
	}

	/*
	 * Return found category name.
	 */

	*categoriespp = cat_name;

	if (catlenp != NULL) {
		*catlenp = (u_int)(end_cat - cat_name) + 1;
	}

	if (nextcatpp != NULL) {
		*nextcatpp = (*cp == NULL ? cp : (char *)(cp + 1));
	}

	return(B_TRUE);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_SET2( categories1, categories2, message ):
 *
 *	Specify a tracing message and associated sets of diagnostic
 *	categories to which that message belongs.  The tracing message
 *	is written to STDFMT as:
 *
 *	    <size>#<time>:<catlen>:<msglen>: <categories> <message>\n
 *
 *	where <size> is the total number of bytes between the tracing
 *	message specification indicator ("#") and the appended newline
 *	("\n") inclusive, <time> is the time(2) of the tracing message,
 *	<catlen> is the total number of bytes in the <categories> string
 *	(or "-" if both category specifications are ADM_NOCATS), <msglen>
 *	is the total number of bytes in the <message> string (or "-" if
 *	the message specification is NULL), <categories> is a comma-
 *	separated list of diagnostic categories to which the tracing
 *	message belongs (the concatenation of both category lists),
 *	and <message> is the tracing message.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *	NOTE: The space preceding the <categories> or <message>
 *	      fields will not be included in the tracing specification
 *	      if either of those fields is undefined (i.e. the
 *	      corresponding length is set to "-").
 *
 *---------------------------------------------------------------------
 */

int
adm_diag_set2(
	char *categories1,
	char *categories2,
	char *message
	)
{
	time_t msgtime;			/* Tracing message timestamp */
	u_int catlen;			/* Length of category spec. */
	u_int msglen;			/* Length of tracing message */
	char *buf;
	u_int buflen;
	u_int speclen;
	int stat;

	/*
	 * Initialize process and be sure we are running as a method.
	 */

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	if (adm_stdfmt == NULL) {
		return(ADM_ERR_METHONLY);
	}

	/*
	 * Allocate timestamp and create message specification.
	 */

	msgtime = time(NULL);
	buflen = adm_diag_strsize(msgtime, categories1, categories2, message,
				  &catlen, &msglen) + 1;
	buf = malloc((size_t)buflen);
	if (buf == NULL) {
		return(ADM_ERR_NOMEM);
	}
	stat = adm_diag_msg2str(msgtime, categories1, categories2, message,
			        buflen, buf, &speclen);
	if (stat != ADM_SUCCESS) {
		free(buf);
		return(stat);
	}

	/*
	 * Output tracing message specification to STDFMT.
	 */


	stat = fprintf(adm_stdfmt, "%u%s", speclen, buf);
	free(buf);
	if (stat < 0) {
		return(ADM_ERR_OUTFAIL);
	}

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_STDCATS( class, class_vers, method, host, domain, reqID_str ):
 *
 *	This routine formulates the standard list of categories to
 *	which all tracing messages in a specific method invocation
 *	belong.  The caller specifies the class name, class version,
 *	method name, host, domain, and request ID (in string format),
 *	of the method being invoked.
 *
 *	If successful, this routine returns a pointer to the standard
 *	category list.  Otherwise, it returns NULL.
 *	
 *	NOTE: The caller is responsible for freeing the space used by
 *	      the returned list when it is no longer needed.
 *
 *---------------------------------------------------------------------
 */

char *
adm_diag_stdcats(
	char *class,
	char *class_vers,
	char *method,
	char *host,
	char *domain,
	char *reqID_str
	)
{
	char *stdcats;		/* Standard category list */
	u_int bufest;		/* Estimate of buffer length needed to hold */
				/* the formulated category list */
	u_int buflen;		/* Actual length of category list. */
	boolean_t other_ents;	/* Other entries currently in list being */
				/* formulated? */
	int len;

/*
 *--------------------------------------
 * The following macros are used within this routine to create the
 * list of standard categories.  The first macro appends a "," to the
 * list if needed to separate a new entry being added to the list.
 * The second macro adds two formatted strings to the list.
 *--------------------------------------
 */

#define ADM_ADD_CATSEP(other_ents, buf, buflen)			\
								\
	if (other_ents) {					\
		*(char *)(buf + buflen) = *ADM_CATSEP;		\
		buflen += 1;					\
	}

#define ADM_ADD_CAT(buf, buflen, len, arg1, arg2)		\
								\
	len = sprintf((char *)(buf + buflen), "%s%s", arg1, arg2); \
	if (len < 0) {						\
		free(buf);					\
		return(NULL);					\
	}							\
	buflen += len;						\
/*
 *--------------------------------------
 */
	/*
	 * Malloc a buffer to hold the standard category list.
	 */

	bufest = (((class != NULL) || (class_vers != NULL)) ?
				sizeof(ADM_PREF_CLASS) : 0)
	       + (class != NULL ? strlen(class) : 0)
	       + (class_vers != NULL ? strlen(class_vers) + 1 : 0)
	       + (method != NULL ? sizeof(ADM_PREF_METHOD) + strlen(method) : 0)
	       + (reqID_str != NULL ? sizeof(ADM_PREF_REQID) + strlen(reqID_str) : 0)
	       + 1;

	stdcats = malloc((size_t)bufest);
	if (stdcats == NULL) {
		return(NULL);
	}

	/*
	 * Build the standard category list.
	 */

	buflen = 0;
	other_ents = B_FALSE;

	if ((class != NULL) || (class_vers != NULL)) {
		ADM_ADD_CAT(stdcats, buflen, len, ADM_PREF_CLASS, "");
		other_ents = B_TRUE;
	}
	if (class != NULL) {
		ADM_ADD_CAT(stdcats, buflen, len, class, "");
	}
	if (class_vers != NULL) {
		ADM_ADD_CAT(stdcats, buflen, len, EXTENSION, class_vers);
	}
	if (method != NULL) {
		ADM_ADD_CATSEP(other_ents, stdcats, buflen);
		ADM_ADD_CAT(stdcats, buflen, len, ADM_PREF_METHOD, method);
		other_ents = B_TRUE;
	}
	if (reqID_str != NULL) {
		ADM_ADD_CATSEP(other_ents, stdcats, buflen);
		ADM_ADD_CAT(stdcats, buflen, len, ADM_PREF_REQID, reqID_str);
		other_ents = B_TRUE;
	}

	*(char *)(stdcats + buflen) = NULL;

	return(stdcats);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_STR2MSG( strp, len, msgtimep, categoriespp, messagepp,
 *		     catlenp, msglenp ):
 *
 *	Parse an administrative framework tracing message specification.
 *	The format of a tracing specification is:
 *
 *		#<time>:<catlen>:<msglen>: <categories> <message>\n
 *
 *	where <time> is the time(2) of the tracing message, <catlen> is
 *	the total number of bytes in the <categories> string (or "-" if
 *	the categories specification is ADM_NOCATS), <msglen> is the total
 *	number of bytes in the <message> string (or "-" if the message
 *	specification is NULL), <categories> is a comma-separated list
 *	of diagnostic categories to which the tracing message belongs,
 *	and <message> is the tracing message.
 *
 *	"strp" should point to the leading "#" in the trace message
 *	message specification and "len" should be the number of bytes
 *	in the specification between the leading "#" and appended
 *	"\n", inclusive.  The <time>, pointer to COPY of category list,
 *	pointer to COPY of message, length of category list, and length
 *	of message are returned in *msgtimep, *categoriespp,
 *	*messagepp, *catlenp, and *msglenp, respectively.
 *
 *	Upon success, this routine returns ADM_SUCCESS.
 *
 *	NOTE: The space preceding the <categories> or <message>
 *	      fields will not be included in the tracing specification
 *	      if either of those fields is undefined (i.e. the
 *	      corresponding length is set to "-").
 *
 *---------------------------------------------------------------------
 */

int
adm_diag_str2msg(
	char *strp,
	u_int len,
	time_t *msgtimep,
	char **categoriespp,
	char **messagepp,
	u_int *catlenp,
	u_int *msglenp
	)
{
	time_t msgtime;		/* Time of tracing message */
	boolean_t is_cat;	/* Is there a category specification? */
	u_int catlen;		/* Length of category list */
	boolean_t is_msg;	/* Is there a tracing message? */
	u_int msglen;		/* Length of tracing message */
	char *tokptr;
	char *newptr;

	if (strp == NULL) {
		return(ADM_ERR_BADDIAG);
	}
	/*
	 * Check for leading "*".
	 */

	if (*strp != *ADM_DIAGMARKER) {
		return(ADM_ERR_BADDIAG);
	}

	/*
	 * Get message time.
	 */

	tokptr = strp + 1;
	msgtime = strtol(tokptr, &newptr, 10);
	if ((tokptr == newptr) || (*newptr != ':')) {
		return(ADM_ERR_BADDIAG);
	}
	tokptr = newptr + 1;

	if (msgtimep != NULL) {
		*msgtimep = msgtime;
	}

	/*
	 * Get category list length.
	 */

	if (*tokptr == '-') {
		catlen = 0;
		is_cat = B_FALSE;
		newptr = tokptr + 1;
	} else {
		catlen = strtol(tokptr, &newptr, 10);
		is_cat = B_TRUE;
	}
	if ((tokptr == newptr) || (*newptr != ':')) {
		return(ADM_ERR_BADDIAG);
	}
	tokptr = newptr + 1;

	if (catlenp != NULL) {
		*catlenp = catlen;
	}

	/*
	 * Get tracing message length.
	 */

	if (*tokptr == '-') {
		msglen = 0;
		is_msg = B_FALSE;
		newptr = tokptr + 1;
	} else {
		msglen = strtol(tokptr, &newptr, 10);
		is_msg = B_TRUE;
	}
	if ((tokptr == newptr) || (*newptr != ':')) {
		return(ADM_ERR_BADDIAG);
	}
	tokptr = newptr + 1;

	if (msglenp != NULL) {
		*msglenp = msglen;
	}

	/*
	 * Get category list.
	 */

	if (is_cat) {
		if (categoriespp != NULL) {
			*categoriespp = malloc((size_t)(catlen + 1));
			if (*categoriespp == NULL) {
				return(ADM_ERR_NOMEM);
			}
			memcpy(*categoriespp, (char *)(tokptr + 1),
			       (size_t)catlen);
			*((char *)(*categoriespp + catlen)) = NULL;
		}
		tokptr += catlen + 1;
	} else {
		if (categoriespp != NULL) {
			*categoriespp = NULL;
		}
	}

	/*
	 * Get tracing message.
	 */

	if (is_msg) {
		if (messagepp != NULL) {
			*messagepp = malloc((size_t)(msglen + 1));
			if (*messagepp == NULL) {
				return(ADM_ERR_NOMEM);
			}
			memcpy(*messagepp, (char *)(tokptr + 1), (size_t)msglen);
			*((char *)(*messagepp + msglen)) = NULL;
		}
		tokptr += msglen + 1;
	} else {
		if (messagepp != NULL) {
			*messagepp = NULL;
		}
	}

	/*
	 * Check for terminating newline.
	 */

	if ((*tokptr != '\n') ||
	    ((u_int)(tokptr - strp + 1) != len)) {
		return(ADM_ERR_BADDIAG);
	}
	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_DIAG_STRSIZE( msgtime, categories1, categories2, message,
 *		     catlenp, msglenp ):
 *
 *	Estimate the number of characters in the string representation
 *	of the specified tracing message (and related categories).
 *	The string representation of a tracing message is:
 *
 *		#<time>:<catlen>:<msglen>: <categories> <message>\n
 *
 *	where <time> is the time(2) of the tracing message, <catlen> is
 *	the total number of bytes in the <categories> string (or "-" if
 *	both category specifications are ADM_NOCATS), <msglen> is the total
 *	number of bytes in the <message> string (or "-" if the message
 *	specification is NULL), <categories> is a comma-separated list
 *	of diagnostic categories to which the tracing message belongs
 *	(the concatenation of both category lists), and <message> is the
 *	tracing message.
 *
 *	This routine always returns an estimate that is guaranteed to
 *	be at least as large as the number of bytes between the
 *	tracing message indicator (#) and the appended newline (\n),
 *	inclusive.  It also returns the total number of bytes in
 *	the <categories> and <message> strings in *catlenp and
 *	*msglenp, respectively.
 *	
 *	NOTE: The space preceding the <categories> or <message>
 *	      fields will not be included in the tracing specification
 *	      if either of those fields is undefined (i.e. the
 *	      corresponding length is set to "-").
 *
 *---------------------------------------------------------------------
 */

u_int
adm_diag_strsize(
	time_t msgtime,
	char *categories1,
	char *categories2,
	char *message,
	u_int *catlenp,
	u_int *msglenp
	)
{
	u_int estimate;		/* Character count estimate */
	u_int catlen;		/* Length of "categories" string */
	u_int msglen;		/* Length of "message" string */

	/*
	 * Form estimate.
	 */
						/* Estimate includes: */

	estimate = ADM_MAXLONGLEN + 5;			/* <time> and extras */

	if ((categories1 != ADM_NOCATS) ||		/* <catlen> */
	    (categories2 != ADM_NOCATS)) {
		estimate += ADM_MAXINTLEN + 1;		/* and space */
	} else {
		estimate += 1;
	}
	catlen = 0;
	if (categories1 != ADM_NOCATS) {
		catlen = strlen(categories1);
	}
	if (categories2 != ADM_NOCATS) {
		catlen += strlen(categories2);
	}
	if ((categories1 != ADM_NOCATS) && (categories2 != ADM_NOCATS)) {
		catlen += 1;
	}
	estimate += catlen;				/* <categories> */

	if (message != NULL) {
		msglen = strlen(message);
		estimate += ADM_MAXINTLEN		/* <msglen> */
			  + msglen			/* <message> */
			  + 1;				/* space */
	} else {
		msglen = 0;
		estimate += 1;				/* <msglen> = "-" */
	}

	/*
	 * Return the estimates.
	 */

	if (catlenp != NULL) {
		*catlenp = catlen;
	}
	if (msglenp != NULL) {
		*msglenp = msglen;
	}
	return(estimate);
}

#endif /* !_adm_diag_impl_c */

