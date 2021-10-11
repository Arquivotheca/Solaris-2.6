#ifndef lint
#pragma ident "@(#)common_strlist.c 1.4 96/04/30 SMI"
#endif
/*
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */
/*
 * Module:	common_strlist.c
 * Group:	libspmicommon
 * Description:	StringList manipulation routines.
 */

#include <stdlib.h>
#include <string.h>
#include "spmicommon_lib.h"

/* public prototypes */

void		StringListFree(StringList *);
StringList *	StringListFind(StringList *, char *);
int		StringListCount(StringList *);
int		StringListAdd(StringList **, char *);
StringList *	StringListBuild(char *, char);
StringList *	StringListDup(StringList *);

/* ------------------------- public functions ----------------------- */

/*
 * Function:	StringListFree
 * Description:	Free all dynamically allocated data associated with
 *		a linked list of StringList structures.
 * Scope:	public
 * Parameters:	list	[RO, *RO]
 *			Pointer to the nead of a StringList linked list.
 * Return:	none
 */
void
StringListFree(StringList *list)
{
	StringList *	next;

	while (list) {
		next = list->next;
		free(list->string_ptr);
		list->string_ptr = NULL;
		free(list);
		list = next;
	}
}

/*
 * Function:	StringListFind
 * Description:	Find a StringList member in the linked list provided
 *		which has a specified string_ptr field.
 * Scope:	public
 * Parameters:	list	[RO, *RO]
 *			Pointer to the nead of a StringList linked list.
 *		str	[RO, *RO]
 *			Name of string to match.
 * Return:	NULL	there is no element in the list with the specified
 *			string
 *		!NULL	pointer to the element containing a matching
 *			string component
 */
StringList *
StringListFind(StringList *list, char *str)
{
	StringList *	sp;

	/* validate parameters */
	if (str == NULL)
		return (0);

	for (sp = list; sp != NULL; sp = sp->next)
		if (sp->string_ptr != NULL && streq(sp->string_ptr, str))
			return (sp);

	return (NULL);
}

/*
 * Function:	StringListCount
 * Description:	Count the number of StringList elements in a link list.
 * Scope:	public
 * Parameters:	list	[RO, *RO]
 *			Pointer to the nead of a StringList linked list.
 * Return:	# >= 0	Number of StringList elements in the list.
 */
int
StringListCount(StringList *list)
{
	int	count;

	for (count = 0; list != NULL; list = list->next)
		count++;

	return (count);
}

/*
 * Function:	StringListAdd
 * Description:	Create a new StringList object with the string value
 *		initialized to a copy of the user supplied value and add
 *		it to the end of the linked list provided.
 * Scope:	public
 * Parameters:	listp	[RO, *RW]
 *			Address of the pointer to the head of a StringList
 *			linked list.
 *		str	[RO, *RO]
 *			Non-NULL pointer to string which is used to initialize
 *			the new element.
 * Return:	 0	entry added successfully to linked list
 *		-1	attempt to add entry failed
 */
int
StringListAdd(StringList **listp, char *str)
{
	StringList *	tmp;
	StringList **	p;

	/* validate parameters */
	if ((listp == NULL) || (str == NULL))
		return (-1);	

	if ((tmp = (StringList *)xcalloc(sizeof (StringList))) == NULL)
		return (-1);

	tmp->string_ptr = xstrdup(str);
	tmp->next = NULL;

	for (p = listp; *p != NULL; p = &((*p)->next))
		;
	*p = tmp;
	return (0);
}
/*
 * Function:	StringListBuild
 * Description:	Build a string list whose elements are the substrings of
 *		the "full_string" argument.  The "delimiter" char provides
 *		the delimeters to be used when splitting full_string" into
 *		sub-strings.  If "delimeter" is any "white" space character,
 *		of if "delimeter" is '\0', the sub-strings are assumed to
 *		be delimited by white space.  White space is always removed
 *		from the string when generating the substrings.  
 *
 *		The string pointed to by "full_string" is not modified by
 *		this function.  The calling code is expected to free the
 *		StringList returned by this function.
 *
 * Scope:	public
 * Parameters:	full_string	[RO, *RO]
 *			Pointer to a string to be split into sub-strings.
 *		delimiters	[RO]
 *			The character that delimits the substrings.
 * Return:	the string list (which could possibly be NULL) is
 *		returned.
 */
StringList *
StringListBuild(char *full_string, char delimeter)
{
	char	*cp, *str_start;
	int	white_delim = 0;
	int	len;
	StringList *head = NULL;
	enum {
		IN_LEADER,
		IN_TRAILER,
		IN_STRING
	} parse_state = IN_LEADER;
	char	hold[256];

	if (full_string == NULL)
		return (NULL);

	if (delimeter == '\0' || isspace(delimeter))
		white_delim = 1;

	for (cp = full_string; *cp; cp++) {
		/* if this is a delimeter character */
		if ((isspace(*cp) && white_delim) || *cp == delimeter) {
			if (parse_state == IN_LEADER)
				continue;
			else if (parse_state == IN_TRAILER) {
				parse_state = IN_LEADER;
				continue;
			} else {	/* delimeter terminates a string */
				/*LINTED [var set before used]*/
				len = cp - str_start;
				(void) strncpy(hold, str_start, len);
				hold[len] = '\0';
				(void) StringListAdd(&head, hold);
				parse_state = IN_LEADER;
			}
		} else { 	/* it's not a delimeter */
			if (isspace(*cp)) {
				if (parse_state == IN_STRING) {
					len = cp - str_start;
					(void) strncpy(hold, str_start, len);
					hold[len] = '\0';
					(void) StringListAdd(&head, hold);
					parse_state = IN_TRAILER;
				}
				continue;
			}
			if (parse_state == IN_LEADER) {
				str_start = cp;
				parse_state = IN_STRING;
			}
		}
	}

	if (parse_state == IN_STRING)
		(void) StringListAdd(&head, str_start);

	return (head);
}
/*
 * Function:	StringListDup
 * Description:	Build a string list which is a duplicate of another.
 *
 * Scope:	public
 * Parameters:	src_string	[RO]
 *			Pointer to a string to be duplicated.
 * Returns:	the duplicate string list.
 */
StringList *
StringListDup(StringList *src_string)
{
	StringList	*head = NULL;

	for ( ; src_string; src_string = src_string->next)
		(void) StringListAdd(&head, src_string->string_ptr);
	return (head);
}
