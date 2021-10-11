
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains private functions for handling administrative
 *	arguments for the administrative framework.  The functions defined
 *	here are exported to framework developers, but not the general public.
 *
 *	    adm_utoa()		Converts unsigned integer into string
 *	    adm_args_a2hdr()	Create the STDFMT pipe header for an arg.
 *	    adm_args_a2str()	Create string fmt representation of an arg.
 *	    adm_args_delr()	Remove a row from a handle's data table.
 *	    adm_args_eor()	Move current arg pointer to end of current row.
 *	    adm_args_finda()	Find a named argument and its predecessor.
 *	    adm_args_freea()	Free the space occupied by an argument.
 *	    adm_args_freer()	Free the space occupied by a table row.
 *	    adm_args_hdrsize()	Estimate size of an arg's STDFMT pipe header.
 *	    adm_args_init()	Initialize the C input argument handle.
 *	    adm_args_insa()	Insert a new argument into a data table.
 *	    adm_args_insr()	Insert a new row into a formatted data table.
 *	    adm_args_remv()	Remove an argument from a data table.
 *	    adm_args_str2a()	Parse a string format argument.
 *	    adm_args_str2h()	Convert a sequence of string args to a handle.
 *
 *	BUGS: Access to data handles is not thread-safe.  Each handle should
 *	      include a mutex lock to guard against concurrent access to
 *	      the handle.
 *
 *******************************************************************************
 */

#ifndef _adm_args_impl_c
#define _adm_args_impl_c

#pragma	ident	"@(#)adm_args_impl.c	1.12	92/09/15 SMI"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_HDRSIZE( name, type, length, valuep ):
 *
 *	Estimate the number of characters in an administrative argument's
 *	string representation header.  The string representation of an
 *	argument is:
 *
 *		+name(type,length)value\n
 *
 *	where length may be "-" to indicate that no value exists for
 *	the argument.  THe header includes everything but the value and
 *	appended newline.
 *
 *	This routine returns an estimate that is guaranteed to be at
 *	least as large as the number of characters actually required
 *	by the header, but may be larger than the number of characters
 *	required.
 *
 *---------------------------------------------------------------------
 */

u_int
adm_args_hdrsize(
	char *name,
	u_int type,
	u_int length,
	caddr_t valuep
	)
{
	u_int estimate;
						/* Estimated length of field: */

	estimate = strlen(name)					/* name */
		 + ADM_MAXINTLEN				/* type */
		 + (valuep == NULL ? 1 : ADM_MAXINTLEN)		/* length */
		 + 4;

	return(estimate);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_U2A ( value, string )
 *
 *	Convert an unsigned integer to a string.
 *
 *	The characers are written at the location pointed to by string and 
 *	NULL-terminated.
 *
 *---------------------------------------------------------------------
 */
void
adm_u2a(
	u_int u, 
	char **p
	)
{
	/* Like itoa, but we use knowledge that u is positive */

	char temp[ADM_MAXINTLEN];
	int i = 0;

	temp[i++] = (char)0;

	do {
		temp[i++] = (u % 10) + '0';
	} while ( (u /=10) > 0);

		/* Digits are stored in temp in reverse order */
	for (i--; i >= 0; i--, (*p)++)
		**p = temp[i];
	(*p)--;			/* Back off to before delimiter */
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_A2HDR( name, type, length, valuep, bufp, buflen, lenp ):
 *
 *	Create the header of an argument's string representation having
 *	the given name, type, length, and value.  Specifically, the string
 *	format is:
 *
 *		+name(type,length)value\n
 *
 *	where length may be "-" to indicate that no value exists for the
 *	argument.  The header consists of everything but the value and
 *	trailing newline.
 *
 *	The header is written at the location pointed to by bufp (and is
 *	NULL-terminated).  The length of the resulting string (less the NULL)
 *	is returned in *lenp.
 *
 *	Upon normal completion, this routine returns ADM_SUCCESS.  If the
 *	resulting string (including NULL) is longer than buflen bytes, the
 *	routine returns an error and does not write anything at bufp.
 *
 *	This version can overwrite the buffer: so could previous XXX
 *
 *---------------------------------------------------------------------
 */
int
adm_args_a2hdr(
	char *name,
	u_int type,
	u_int length,
	caddr_t valuep,
	char *bufp,
	u_int buflen,
	u_int *lenp
	)
{
	u_int name_len;		/* Length of argument name */
	char *out_p;		/* Pointer into bufp */
	char *in_p;		/* Pointer into text to be scanned */

	name_len = strlen(name);
        if (name_len > ADM_MAX_NAMESIZE) {
                return(ADM_ERR_NAMETOOLONG);
        }
        if (type != ADM_STRING) {
                return(ADM_ERR_BADTYPE);
        }

	/* Make sure string representation will fit in buffer */
	/* Eliminate call to adm_args_hdrsize, as it calls strlen again */

	if ((name_len + ADM_MAXINTLEN + 4
		 + (valuep == NULL ? 1 : ADM_MAXINTLEN)) >= buflen) {
		return(ADM_ERR_TOOLONG);
	}

        /* Write the argument to the buffer */

	out_p = bufp;	/* Start at beginning of buffer... */ 

			/* XXX Assumes ADM_ARGMARKER is a single character */
	in_p = ADM_ARGMARKER;
	*out_p++ = *in_p;

			/* Copy name into buffer */
	for (in_p = name; *in_p != '\0'; in_p++, out_p++)
		*out_p = *in_p;

	*out_p++ = '(';

	adm_u2a(type, &out_p);

	*out_p++ = ',';

			/* Enter length or '-' */
        if (valuep != NULL) {                           /* Length */
		adm_u2a(length, &out_p);
        } else {
		*out_p++ = '-';
        }

	*out_p++ = ')';
        *out_p = NULL;

	if (lenp != NULL) {
		*lenp = (u_int) (out_p - bufp);
	}
	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_A2STR( name, type, length, valuep, bufp, buflen, lenp ):
 *
 *	Create the string representation of an argument having the given
 *	name, type, length, and value.  Specifically, the string format is:
 *
 *		+name(type,length)value\n
 *
 *	where length may be "-" to indicate that no value exists for the
 *	argument.  The string representation is written at the location
 *	pointed to by bufp (and is NULL-terminated).  The length of the
 *	resulting string representation (less the NULL) is returned in
 *	*lenp.
 *
 *	Upon normal completion, this routine returns ADM_SUCCESS.  If the
 *	resulting string representation (including NULL) is longer than
 *	buflen bytes, the routine returns an error and does not write
 *	anything at bufp.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_a2str(
	char *name,
	u_int type,
	u_int length,
	caddr_t valuep,
	char *bufp,
	u_int buflen,
	u_int *lenp
	)
{
	u_int arg_len;		/* Total length of arg's string representation */
	u_int val_len;		/* Length of argument value */
	int stat;

	val_len = (valuep == NULL ? 0 : length);

	/* Write the argument header to the buffer */

	if (buflen <= val_len) {
		return(ADM_ERR_TOOLONG);
	}
	stat = adm_args_a2hdr(name, type, length, valuep, bufp,
		(u_int)(buflen - val_len - 1), &arg_len);
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

        /* Write the argument value to the buffer */

        if (valuep != NULL) {                           /* Value */
                memcpy((char *)(bufp+arg_len), valuep, (size_t) val_len);
		arg_len += val_len;
        }

        *((char *)(bufp+arg_len)) = '\n';
	arg_len += 1;
	*((char *)(bufp+arg_len)) = NULL;

	if (lenp != NULL) {
		*lenp = arg_len;
	}
	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_STR2A( s1, namep, typep, lengthp, valuepp ):
 *
 *	Parse an argument stored in string format.  The string format of
 *	an argument is:
 *
 *		+name(type,length)value value value ...\n
 *
 *	where length may be "-" to indicate that no value exists for the
 *	argument.  *s1 should point to the argument's string representation.
 *	This routine copies the argument's name, type, length, and value
 *	pointer to the specified locations.  *s1 is also advanced to point
 *	to the character immediately following the trailing newline character.
 *
 *	Upon normal completion, this routine returns ADM_SUCCESS.  If an
 *	error occurs, *s1 will not be altered.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_str2a(
	char **s1,
	char *namep,
	u_int *typep,
	u_int *lengthp,
	caddr_t *valuepp
	)
{
	char *tok_ptr;			/* Ptr to next token in arg spec. */
	boolean_t is_value;		/* Does the argument have a value? */
	char numbuf[ADM_MAXINTLEN + 1];	/* Buffer to hold numeric token */
	char sepbuf[2];			/* Buffer to hold separator character */
	int stat;

	tok_ptr = *s1;		/* Start scanning arg from beginning */

	/* Check for "+" signalling the start of an argument */

	if (*tok_ptr != *ADM_ARGMARKER) {
		return(ADM_ERR_BADFMT);
	} else {
		tok_ptr += 1;
	}

	/* Get the argument name */

	stat = adm_make_tok(&tok_ptr, namep, sepbuf, "(),\n", (u_int) ADM_MAX_NAMESIZE);
	if (stat != ADM_SUCCESS) {
		return(stat);
	}
	if (*sepbuf != '(') {
		return(ADM_ERR_BADFMT);
	}

	/* Get the argument type */

	*typep = (u_int) strtol(tok_ptr, &tok_ptr, 10);
	if (*typep != ADM_STRING) {
		return(ADM_ERR_BADTYPE);
	}
	if (*tok_ptr != ',') {
		return(ADM_ERR_BADFMT);
	}
	tok_ptr += 1;

	/* Get the argument length */

	stat = adm_make_tok(&tok_ptr, numbuf, sepbuf, "(),\n", (u_int) ADM_MAXINTLEN);
	if (stat != ADM_SUCCESS) {
		return(stat);
	}
	if (*sepbuf != ')') {
		return(ADM_ERR_BADFMT);
	}
	if (strcmp(numbuf, "-") == 0) {
		*lengthp = 0;
		is_value = B_FALSE;
	} else {
		if (!isdigit((int)(*numbuf))) {
			return(ADM_ERR_BADLENGTH);
		}
		*lengthp = atoi(numbuf);
		is_value = B_TRUE;
	}

	/* Get the argument value */

	if (is_value) {
		*valuepp = tok_ptr;
	} else {
		*valuepp = NULL;
	}
	if (*(tok_ptr + *lengthp) != '\n') {
		return(ADM_ERR_BADLENGTH);
	} else {
		*s1 = tok_ptr + *lengthp + 1;
		return(ADM_SUCCESS);
	}
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_STR2H( str_handle, handlepp ):
 *
 *	Convert a sequence of administrative arguments in string format
 *	to a C administrative data handle.  str_handle should point to a
 *	string of the form:
 *
 *		+name(type,length)value value value ...\n
 *		+name(type,length)value value value ...\n
 *		+name(type,length)value value value ...\n
 *			:
 *			:
 *
 *	where length may be "-" to indicate that no value exists for the
 *	argument.  The sequence may also contain the end-of-row marker
 *	"@\n" to indicate the end of a row of formatted arguments.
 *
 *	Upon successful completion, this routine sets *handlepp to point to
 *	a new C administrative data handle containing the specified argument
 *	names and values (in the same order they appear within the string
 *	handle), and returns ADM_SUCCESS.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_str2h(
	char *str_handle,
	Adm_data **handlepp
	)
{
	char *tok_ptr;				/* Ptr to next token in handle */
	char name[ADM_MAX_NAMESIZE + 1];	/* Argument name */
	u_int type;				/* Argument type */
	u_int length;				/* Argument length */
	caddr_t valuep;				/* Pointer to arg value */
	boolean_t new_row;			/* Start a new table row? */
	int stat;

	*handlepp = NULL;

	if (str_handle == NULL) {
		return(ADM_ERR_BADHANDLE);
	}

	tok_ptr = str_handle;	/* Start scanning string handle from beginning */


	/* Create a new C handle to store arguments */

	*handlepp = adm_args_newh();
	if (*handlepp == NULL) {
		return(ADM_ERR_NOMEM);
	}


	/* Convert the sequence of string args to C handle args */

	new_row = B_TRUE;

	while (*tok_ptr != NULL) {

	    if (new_row) {			/* Add new row if needed */
		stat = adm_args_insr(*handlepp);
		if (stat != ADM_SUCCESS) {
			adm_args_freeh(*handlepp);
			return(stat);
		}
		new_row = B_FALSE;
	    }

	    if (*tok_ptr == *ADM_ARGMARKER) {	/* Add an argument to handle */

		stat = adm_args_str2a(&tok_ptr, name, &type, &length, &valuep);
		if (stat != ADM_SUCCESS) {
			adm_args_freeh(*handlepp);
			return(stat);
		}
		stat = adm_args_puta(*handlepp, name, type, length, valuep);
		if (stat != ADM_SUCCESS) {
			adm_args_freeh(*handlepp);
			return(stat);
		}
						/* End the current row of args */
	    } else if ((*tok_ptr == *ADM_ROWMARKER) &&
		       (*((char *)(tok_ptr+1)) == '\n')) {

		new_row = B_TRUE;
		tok_ptr += 2;

	    } else {				/* Malformed string argument */

		adm_args_freeh(*handlepp);
		return(ADM_ERR_BADFMT);
	    }
	}

	adm_args_reset(*handlepp);
	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_REMV( handlep, predp, alinkp):
 *
 *	Remove an argument from the current row of a handle's formatted data 
 *	table.  alinkp should point to the Adm_arglink structure of the
 *	argument to remove, and predp should point to its predecessor's
 *	Adm_arglink structure.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.
 *
 *	Note: If alinkp points to the first argument in a row, predp should
 *	      be NULL.
 *
 *	Note: Any pointers to the removed argument or its assocaiated
 *	      structures will be invalid when this function completes.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_remv(
	Adm_data *handlep,
	Adm_arglink *predp,
	Adm_arglink *alinkp
	)
{
	Adm_arglink *del_alinkp;	/* Ptr to argument to delete */

	del_alinkp = alinkp;

	/*
	 * If this is the first arg in the current row, remove the reference
	 * to it from the row's Adm_rowlink structure.  If it is not, then
	 * remove the reference to it from its predecessor.
	 */

	if (handlep->current_rowp->first_alinkp == alinkp) {
		handlep->current_rowp->first_alinkp = alinkp->next_alinkp;
	} else {
		predp->next_alinkp = alinkp->next_alinkp;
	}

	/*
	 * If we are removing the current argument in the handle, then
	 * replace it with its predecessor.
	 */

	if (handlep->current_alinkp == del_alinkp) {
		handlep->current_alinkp = predp;
	}

	/* Free the space occupied by the removed argument */

	adm_args_freea(del_alinkp);

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_DELR( handlep, pred_rowp, rowp ):
 *
 *	Remove a row from the specified handle's formatted data table.
 *	rowp should point to the row to delete and pred_rowp should
 *	point to its preceding row.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.
 *
 *	Note: If rowp points to the first row in the table, pred_rowp
 *	      should be NULL.
 *
 *	Note: Any pointers to the removed row or its assocaiated
 *	      structures will be invalid when this function completes.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_delr(
	Adm_data *handlep,
	Adm_rowlink *pred_rowp,
	Adm_rowlink *rowp
	)
{
	/*
	 * If this is the first row in the table, remove the reference to
	 * it in the handle.  If it is not the first row, then remove the
	 * reference to it in the preceding row.
	 */

	if (handlep->first_rowp == rowp) {
		handlep->first_rowp = rowp->next_rowp;
	} else {
		pred_rowp->next_rowp = rowp->next_rowp;
	}

	/*
	 * If the row being deleted is the last row in the table, reset
	 * the last row pointer in the handle.
	 */

	if (handlep->last_rowp == rowp) {
		handlep->last_rowp = pred_rowp;
	}

	/*
	 * If we are deleting the current row of the handle, then reset
	 * the current row and current argument pointer appropriately.
	 */

	if (handlep->current_rowp == rowp) {
		if (rowp->next_rowp == NULL) {		/* Reset to prev row */
			handlep->current_rowp = pred_rowp;
			if (pred_rowp != NULL) {
				adm_args_eor(handlep);
			} else {
				handlep->current_alinkp = NULL;
			}
		} else {				/* Reset to next row */
			handlep->current_rowp = rowp->next_rowp;
			handlep->current_alinkp = NULL;
		}
	}

	/* Free the space occupied by the row */

	adm_args_freer(rowp);

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_EOR( handlep ):
 *
 *	Set the current argument pointer in the specified handle
 *	to the end of the current row.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_eor(
	Adm_data *handlep
	)
{
	Adm_arglink *cur_alinkp;	/* Ptr to current argument in row */
	Adm_arglink *prev_alinkp;	/* Ptr to previous argument in row */

	cur_alinkp = handlep->current_rowp->first_alinkp;
	prev_alinkp = NULL;

	while (cur_alinkp != NULL) {
		prev_alinkp = cur_alinkp;
		cur_alinkp = cur_alinkp->next_alinkp;
	}

	handlep->current_alinkp = prev_alinkp;

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_FINDA( handlep, arg_name, predpp, alinkp):
 *
 *	Find the named argument in the current row of the specified
 *	handle's formatted data table and return pointers to its
 *	Adm_arglink structure and its predecessor's Adm_arglink structure.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.  If the
 *	named argument does not exist in the current row of the table, the
 *	routine returns ADM_ERR_NOVALUE.
 *
 *	Note: This routine assumes that the specified handle is valid and
 *	      contains a current row specification.
 *
 *	Note: This routine does not alter the specified handle in any way.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_finda(
	Adm_data *handlep,
	char *arg_name,
	Adm_arglink **predpp,
	Adm_arglink **alinkpp
	)
{
	Adm_arglink *pred_alinkp; /* Ptr to previous arg link searched */
	Adm_arglink *cur_alinkp;  /* Ptr to current arg link being searched */
	boolean_t found;	  /* Named argument found in row? */

	*predpp = NULL;		/* Initialize return values */
	*alinkpp = NULL;	

	/* Find the named parameter in the current row */

	found = B_FALSE;
	pred_alinkp = NULL;
	cur_alinkp = handlep->current_rowp->first_alinkp;
	while ( (found != B_TRUE) && (cur_alinkp != NULL) ) {
		if (strcmp(cur_alinkp->argp->name, arg_name) == 0) {
			found = B_TRUE;
		} else {
			pred_alinkp = cur_alinkp;
			cur_alinkp = cur_alinkp->next_alinkp;
		}
	}

	/*
	 * Return pointers to the arguments Adm_arglink structure and
         * that of its predecessor.
	 */

	if (found == B_TRUE) {
		*predpp = pred_alinkp;
		*alinkpp = cur_alinkp;
		return(ADM_SUCCESS);
	} else {
		return(ADM_ERR_NOVALUE);
	}
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_FREEA( alinkp ):
 *
 *	Free the space occupied by an argument.  This routine frees the
 *	space occupied by the argument's Adm_arglink, and Adm_arg structures.
 *
 *	This routine always returns ADM_SUCCESS.
 *
 *	Note: Any pointers to the argument or its associated data structures
 *	      will be invalid when this function completes.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_freea(
	Adm_arglink *alinkp
	)
{
	if (alinkp != NULL) {
		if (alinkp->argp->valuep != NULL) {
			free(alinkp->argp->valuep);
		}
		free(alinkp->argp);
		free(alinkp);
	}

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_FREER( rowp ):
 *
 *	Free the space occupied by a row in a formatted data table.  This
 *	routine frees the space occupied by the Adm_rowlink, Adm_arglink,
 *	and Adm_arg structures in the row.
 *
 *	This routine always returns ADM_SUCCESS.
 *
 *	Note: Any pointers to the row or its associated data structures
 *	      will be invalid when this function completes.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_freer(
	Adm_rowlink *rowp
	)
{
	Adm_arglink *alinkp;	  /* Pointer to argument to free */
	Adm_arglink *next_alinkp; /* Pointer to next argument to free */

	if (rowp != NULL) {
		alinkp = rowp->first_alinkp;
		while (alinkp != NULL) {
			next_alinkp = alinkp->next_alinkp;
			adm_args_freea(alinkp);
			alinkp = next_alinkp;
		}
		free(rowp);
	}

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_INIT():
 *
 *	Initialize the argument handling routines.  Specifically, create
 *	a handle that contains the input arguments to the method and place
 *	a pointer to the handle in adm_input_args.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_init()
{
	char *input_env_args;	/* Environment variable (passed by AMSL) */
				/* that contains the method's input args */
	int stat;

	adm_input_args = NULL;

	input_env_args = getenv(ADM_INPUTARGS_NAME);

	if (input_env_args == NULL) {
		return(ADM_SUCCESS);
	}

	stat = adm_args_str2h(input_env_args, &adm_input_args);

	return(stat);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_INSA( handlep, name, type, length, valuep ):
 *
 *	Insert an argument with the specified name, type, length, and
 *	value into specified handle.  The argument is placed immediately
 *	after the current argument in the current row.  If the current
 *	argument pointer is NULL, the argument is inserted at the start
 *	of the row.  The current argument pointer is also set to point
 *	to this new argument.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_insa(
	Adm_data *handlep,
	char *name,
	u_int type,
	u_int length,
	caddr_t valuep
	)
{
	Adm_arg *new_argp;	/* Ptr to new argument Adm_arg structure */
	Adm_arglink *new_alinkp;/* Ptr to new argument Adm_arglink structure */
	caddr_t new_valuep;	/* Ptr to value of new argument */

	/* Validate argument name length and type */

	if (strlen(name) > (size_t) ADM_MAX_NAMESIZE) {
		return(ADM_ERR_NAMETOOLONG);
	}	
	if (type != ADM_STRING) {
		return(ADM_ERR_BADTYPE);
	}

	/* Allocate space to hold the new argument */

	new_argp = (Adm_arg *) malloc(sizeof(Adm_arg));
	if (new_argp == NULL) {
		return(ADM_ERR_NOMEM);
	}
	new_alinkp = (Adm_arglink *) malloc(sizeof(Adm_arglink));
	if (new_alinkp == NULL) {
		free(new_argp);
		return(ADM_ERR_NOMEM);
	}
	if (valuep != NULL) {
		new_valuep = malloc((size_t)(length + 1));
		if (new_valuep == NULL) {
			free(new_argp);
			free(new_alinkp);
			return(ADM_ERR_NOMEM);
		}
	
	}

	/* Create the new argument structure */

	new_alinkp->argp = new_argp;
	strcpy(new_argp->name, name);
	new_argp->type = type;
	new_argp->length = length;
	if (valuep == NULL) {
		new_argp->valuep = NULL;
	} else {
		memcpy(new_valuep, valuep, (size_t) length);
		new_valuep[length] = NULL;
		new_argp->valuep = new_valuep;
	}

	/* Insert the new argument into the current row of the table */

	if (handlep->current_alinkp == NULL) {	/* Insert at start of row */
		new_alinkp->next_alinkp = handlep->current_rowp->first_alinkp;
		handlep->current_rowp->first_alinkp = new_alinkp;
	} else {				/* Insert in middle of row */
		new_alinkp->next_alinkp = handlep->current_alinkp->next_alinkp;
		handlep->current_alinkp->next_alinkp = new_alinkp;
	}
	handlep->current_alinkp = new_alinkp;

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_INSR( handlep ):
 *
 *	Insert a new row into the formatted data table of the specified
 *	handle.  The row is inserted immediately after the current row
 *	in the table.  If there is no formatted data table in the handle,
 *	one is created with one row.  The current argument pointer is also
 *	set to the start of the new row.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_insr(
	Adm_data *handlep
	)
{
	Adm_rowlink *new_rowp;	/* Ptr to new row being created */

	new_rowp = (Adm_rowlink *) malloc(sizeof(Adm_rowlink));
	if (new_rowp == NULL) {
		return(ADM_ERR_NOMEM);
	}

	new_rowp->first_alinkp = NULL;	/* Row is initially empty */

	if (handlep->first_rowp == NULL) {	/* Create a table */
		handlep->first_rowp = new_rowp;
		handlep->last_rowp = new_rowp;
		new_rowp->next_rowp = NULL;
	} else {				/* Table already exists */
		new_rowp->next_rowp = handlep->current_rowp->next_rowp;
		handlep->current_rowp->next_rowp = new_rowp;
		if (new_rowp->next_rowp == NULL) {
			handlep->last_rowp = new_rowp;
		}
	}

	handlep->current_rowp = new_rowp;
	handlep->current_alinkp = NULL;

	return(ADM_SUCCESS);
}

#endif /* !_adm_args_impl_c */

