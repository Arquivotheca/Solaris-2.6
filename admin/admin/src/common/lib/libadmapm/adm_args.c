
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 *******************************************************************************
 *
 *	This file contains functions for handling administrative arguments
 *	for the administrative framework.  It contains functions for
 *	handling both client and server (method) side arguments.
 *
 *	Client Argument Handling Functions:
 *
 *		adm_args_bor()	  Reset current arg pointer to beginning of row.
 *		adm_args_dela()   Delete an argument from a data table.
 *		adm_args_freeh()  Free the space used by a data handle.
 *		adm_args_geta()	  Return value of a named arg from a handle.
 *		adm_args_getu()	  Return unformatted data from a handle.
 *		adm_args_htype()  Return type of data stored in a handle.
 *		adm_args_newh()	  Create an empty data handle.
 *		adm_args_nexta()  Advance handle to read from next arg in row.
 *		adm_args_nextr()  Advance handle to read from next row in table.
 *		adm_args_reset()  Reset handle to read from start of table.
 *		adm_args_puta()	  Set value of an argument in a handle's table.
 *		adm_args_putu()	  Set the unformatted data value in a handle.
 *		
 *	Server Argument Handling Functions:
 *
 *		adm_args_geti()	  Return the value of a named input argument.
 *		adm_args_markr()  Mark the end of a row of output arguments.
 *		adm_args_nexti()  Return the next input argument to the method.
 *		adm_args_rsti()	  Reset the input handle to the first arg.
 *		adm_args_set()	  Set the value of an output argument.
 *
 *	BUGS: Access to data handles is not thread-safe.  Each handle should
 *	      include a mutex lock to guard against concurrent access to
 *	      the handle.
 *
 *******************************************************************************
 */

#ifndef _adm_args_c
#define _adm_args_c

#pragma	ident	"@(#)adm_args.c	1.7	92/01/28 SMI"

#include <stdlib.h>
#include <string.h>
#include <stdio.h> 
#include <stddef.h>
#include <sys/types.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_BOR( handlep ):
 *
 *	Reset the current argument pointer in the specified handle to
 *	the beginning of the current row.
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_bor(
	Adm_data *handlep
	)
{
	if (handlep == NULL) {			/* Handle valid? */
		return(ADM_ERR_BADHANDLE);
	}
	if (handlep->first_rowp == NULL) {	/* Handle has formatted data ? */
		return(ADM_ERR_NOFRMT);
	}
	if (handlep->current_rowp == NULL) {	/* Handle has current row? */
		return(ADM_ERR_NOROW);
	}

	handlep->current_alinkp = NULL;		/* Reset ptr to start of row */
	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_DELA( handlep, arg_name ):
 *
 *	Delete the named argument from the current row of the
 *	specified administrative data table.
 *
 *	If successful, this routine returns ADM_SUCCESS and sets the
 *	handle's current argument pointer to the predecessor of the
 *	deleted argument.
 *
 *	Note: Any pointers to the removed argument or its assocaiated
 *	      structures will be invalid when this function completes.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_dela(
	Adm_data *handlep,
	char *arg_name
	)
{
	Adm_arglink *alinkp;		/* Pointer to named argument */
	Adm_arglink *predp;		/* Pointer to named args' predecessor */
	int stat;

	if (handlep == NULL) {			/* Valid data handle? */
		return(ADM_ERR_BADHANDLE);
	}
	if (handlep->current_rowp == NULL) {	/* Current row? */
		return(ADM_ERR_NOROW);
	}
	if (arg_name == NULL) {			/* Valid argument name */
		return(ADM_ERR_BADNAME);
	}

	/* Find the named parameter in the current row */

	stat = adm_args_finda(handlep, arg_name, &predp, &alinkp);
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	/*
	 * Delete the argument from the row and set the current argument
	 * pointer to it's predecessor.
	 */

	stat = adm_args_remv(handlep, predp, alinkp);
	if (stat != ADM_SUCCESS) {
		return(ADM_SUCCESS);
	}

	handlep->current_alinkp = predp;

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_FREEH( handlep ):
 *
 *	Free the space occupied by the specified handle.  This function
 *	frees the Adm_data structure along with its associated Adm_rowlink,
 *	Adm_arglink, and Adm_arg structures.
 *
 *	This routine always returns ADM_SUCCESS.
 *
 *	Note: Any pointers to the handle or its associated structures
 *	      will be invalid when this function completes.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_freeh(
	Adm_data *handlep
	)
{
	Adm_rowlink *crowp;	/* Ptr to current row of table being free */
	Adm_rowlink *nrowp;	/* Ptr to next row in handle table to free */

	if (handlep == NULL) {		/* Ignore NULL handles */
		return(ADM_SUCCESS);
	}

	/* Free unformatted data stored in handle */

	if (handlep->unformattedp != NULL) {
		free(handlep->unformattedp);
	}

	/* Free each row of arguments in the handle table */

	crowp = handlep->first_rowp;
	while (crowp != NULL) {
		nrowp = crowp->next_rowp;
		adm_args_freer(crowp);
		crowp = nrowp;
	}

	/* Free the data handle structure */

	free(handlep);

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_GETA( handlep, arg_name, arg_type, argpp):
 *
 *	Return the value of the named argument from the current row
 *	of the specified handle's formatted data table.  arg_type should
 *	specify the type of value expected for the argument.  If the caller
 *	is willing to accept any type of value, arg_type should be set to
 *	ADM_ANYTYPE.  This routine returns a pointer to the argument's
 *	Adm_arg structure and sets the handle's current argument pointer
 *	to point to this argument.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.  If the
 *	named argument does not exist in the current row of the table,
 *	the routine returns ADM_ERR_NOVALUE and leaves the handle intact.
 *	If the argument exists, but is of the wrong type, the routine
 *	returns ADM_ERR_WRONGTYPE and leaves the handle intact.
 *
 *	Note: This routine returns a pointer to the actual Adm_arg
 *	      structure stored in the handle.  The caller should not
 *	      therefore alter or destroy this structure.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_geta(
	Adm_data *handlep,
	char *arg_name,
	u_int arg_type,
	Adm_arg **argpp
	)
{
	Adm_arglink *predp;	/* Ptr to named arg's predecessor */
	Adm_arglink *alinkp;	/* Ptr to named arg */
	int stat;

	*argpp = NULL;		/* Initialize return value */

	if (handlep == NULL) {
		return(ADM_ERR_BADHANDLE);
	}
	if (handlep->current_rowp == NULL) {
		return(ADM_ERR_NOROW);
	}

	/* Find the named parameter in the current row */

	stat = adm_args_finda(handlep, arg_name, &predp, &alinkp);

	/*
	 * Return the named argument's Adm_arg structure  and advance the
         * handle's current argument pointer.
	 */

	switch(stat) {

	    case ADM_SUCCESS:

		if ((alinkp->argp->type == arg_type) || (arg_type == ADM_ANYTYPE)) {
			*argpp = alinkp->argp;
			handlep->current_alinkp = alinkp;
			return(ADM_SUCCESS);
		} else  {
			return(ADM_ERR_WRONGTYPE);
		}

	    default:

		return(ADM_ERR_NOVALUE);
	}
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_GETI( arg_name, arg_type, argpp):
 *
 *	Return the value of the named input argument to the method.  arg_type
 *	should specify the type of value expected for the argument.  If
 *	the caller is willing to accept any type, arg_type should be set
 *	to ADM_ANYTYPE.  This routine returns a pointer to the argument's
 *	Adm_arg structure and sets the input handle's current argument pointer
 *	to point to this argument.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.  If the
 *	named argument does not exist in the current row of the table, the
 *	routine returns ADM_ERR_NOVALUE and leaves the handle intact.  If the
 *	argument exists, but is of the wrong type, the routine returns
 *	ADM_ERR_WRONGTYPE and leaves the handle intact.
 *
 *	Note: This routine returns a pointer to the actual Adm_arg
 *	      structure stored in the handle.  The caller should not
 *	      therefore alter or destroy this structure.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_geti(
	char *arg_name,
	u_int arg_type,
	Adm_arg **argpp
	)
{
	int stat;

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	if (adm_input_args == NULL) {
		return(ADM_ERR_METHONLY);
	}

	/* Get the named input argument */

	stat = adm_args_geta(adm_input_args, arg_name, arg_type, argpp);

	if (stat == ADM_ERR_NOROW) {
		return(ADM_ERR_NOVALUE);
	}

	return(stat);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_GETU( handlep, datapp, datalenp ):
 *
 *	Return the unformatted data stored in the specified administrative
 *	data handle.  This routine returns a pointer the unformatted data
 *	in *datapp and the length of the unformatted data block in
 *	*datalenp.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.
 *
 *	Note: This routine returns a pointer that points within the
 *	      data structures of the handle itself.  The caller should
 *	      not therefore alter or destroy this structure.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_getu(
	Adm_data *handlep,
	caddr_t *datapp,
	u_int *datalenp
	)
{
	*datapp = NULL;		/* Initialize return value */
	*datalenp = 0;

	if (handlep == NULL) {
		return(ADM_ERR_BADHANDLE);
	}
	if (handlep->unformattedp == NULL) {
		return(ADM_ERR_NOUNFRMT);
	}
	*datapp = handlep->unformattedp;
	*datalenp = handlep->unformatted_len;
	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_HTYPE( handlep ):
 *
 *	Return the type of data stored in the specified handle.  The
 *	return value is formed by ORing the appropriate values:
 *
 *		ADM_FORMATTED	    The handle contains formatted data.
 *		ADM_UNFORMATTED	    The handle contains unformatted data.
 *		ADM_INVALID	    The handle is invalid.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_htype(
	Adm_data *handlep
	)
{
	int htype;	/* Type of data stored in handle */

	if (handlep == NULL) {
		return(ADM_INVALID);
	}

	htype = 0;
	if (handlep->unformattedp != NULL) {
		htype |= ADM_UNFORMATTED;
	}
	if (handlep->first_rowp != NULL) {
		htype |= ADM_FORMATTED;
	}
	return(htype);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_MARKR():
 *
 *	This routine is used to mark the end of a row of formatted data
 *	output.  Specifically, this routine writes the end-of-row marker
 *
 *		<size>@\n
 *
 *	to the standard formatted output file descriptor (where <size>
 *	is the total number of characters between the end-of-row indication
 *	and the newline -- i.e. <size> is 2).
 *
 *	Upon successful completion, this routine returns ADM_SUCCESS.
 *
 *	Note: This routine is not thread safe.  If multiple threads
 *	      concurrently attempt to write arguments or errors to
 *	      STDFMT, unpredictable results will occur.  A mutex lock
 *	      on the STDMFT pipe is needed to gaurd against concurrent
 *	      access.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_markr()
{
	int stat;

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	if (adm_stdfmt == NULL) {
		return(ADM_ERR_METHONLY);
	}

	stat = fprintf(adm_stdfmt, "%lu%s\n", (u_long)(strlen(ADM_ROWMARKER) + 1),
					     ADM_ROWMARKER);

	return((stat == EOF ? ADM_ERR_OUTFAIL : ADM_SUCCESS));
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_NEWH():
 *
 *	Create a new administrative data handle and initialize it to
 *	be empty.  Upon successful completion, this routine returns
 *	a pointer to the new handle.  If an error occurs, the function
 *	returns a NULL pointer.
 *
 *---------------------------------------------------------------------
 */
Adm_data *
adm_args_newh()
{
	Adm_data *newh;	/* New administrative data handle */

	newh = (Adm_data *) malloc(sizeof(Adm_data));
	if (newh == NULL) {
		return(NULL);
	}

	newh->unformatted_len = 0;
	newh->unformattedp = NULL;
	newh->first_rowp = NULL;
	newh->last_rowp = NULL;
	newh->current_rowp = NULL;
	newh->current_alinkp = NULL;

	return(newh);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_NEXTA( handlep, arg_type, argpp):
 *
 *	Return the value of the next argument in the current row of
 *	the specified handle's formatted data table.  This routine
 *	returns a pointer to the argument's Adm_arg structure and sets
 *	the handle's current argument pointer to point to this argument.
 *	arg_type should specify the type of value expected for the next
 *	argument.  If the caller is willing to accept any argument type
 *	(and usually will if they're calling this routine), arg_type should
 *	be set to ADM_ANYTYPE.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.  If there
 *	are no arguments remaining in the current row of the table, the
 *	routine returns ADM_ERR_ENDOFROW and leaves the handle intact.  If there
 *	is another argument, but its type does not match arg_type, this
 *	routine will return ADM_ERR_WRONGTYPE and leave the handle intact.
 *
 *	Note: This routine returns a pointer to the actual Adm_arg
 *	      structure stored in the handle.  The caller should not
 *	      therefore alter or destroy this structure.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_nexta(
	Adm_data *handlep,
	u_int arg_type,
	Adm_arg **argpp
	)
{
	Adm_arglink *nalinkp;	/* Ptr to next arg link in current row */

	*argpp = NULL;		/* Initialize return value */

	if (handlep == NULL) {			/* Handle valid? */
		return(ADM_ERR_BADHANDLE);
	}
	if (handlep->first_rowp == NULL) {	/* Handle has formatted data ? */
		return(ADM_ERR_NOFRMT);
	}
	if (handlep->current_rowp == NULL) {	/* Handle has current row? */
		return(ADM_ERR_NOROW);
	}

	/*
	 * If we are at the start of a row, return the first argument
	 * in the row.  Otherwise, return the next argument in row.
	 */

	if (handlep->current_alinkp == NULL) {
		nalinkp = handlep->current_rowp->first_alinkp;
	} else {
		nalinkp = handlep->current_alinkp->next_alinkp;
	}

	if (nalinkp == NULL) {
		return(ADM_ERR_ENDOFROW);
	} else {
		if ((nalinkp->argp->type == arg_type) || (arg_type == ADM_ANYTYPE)) {
			*argpp = nalinkp->argp;
			handlep->current_alinkp = nalinkp;
			return(ADM_SUCCESS);
		} else {
			return(ADM_ERR_WRONGTYPE);
		}
	}
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_NEXTI( arg_type, argpp ):
 *
 *	Return a pointer to the next input argument to the method.  This
 *	routine returns a pointer to the argument's Adm_arg structure and
 *	advances the input handle's current argument pointer to this
 *	argument.  arg_type specifies the type of argument expected by the
 *	caller.  If the caller is willing to accept any type (and usually
 *	will be if calling this routine), arg_type should be set to ADM_ANYTYPE.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.  If there
 *	are no remaining input arguments to the method, this routine
 *	returns ADM_ERR_ENDOFINPUTS.  If the type of the next argument does not
 *	match arg_type, the routine returns ADM_ERR_WRONGTYPE and leaves the
 *	input handle intact.
 *
 *	Note: This routine returns a pointer to the actual Adm_arg
 *	      structure stored in the handle.  The caller should not
 *	      therefore alter or destroy this structure.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_nexti(
	u_int arg_type,
	Adm_arg **argpp
	)
{
	int stat;

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	if (adm_input_args == NULL) {
		return(ADM_ERR_METHONLY);
	}

	/* Get the next input argument */

	stat = adm_args_nexta(adm_input_args, arg_type, argpp);

	if ((stat == ADM_ERR_NOFRMT) || (stat == ADM_ERR_NOROW) ||
	    (stat == ADM_ERR_ENDOFROW)) {
		return(ADM_ERR_ENDOFINPUTS);
	}

	return(stat);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_NEXTR( handlep ):
 *
 *	This routine sets the specified handle to read from the start
 *	of the next row in its formatted data table.  The function sets
 *	the handle's current row pointer to the next row in the table,
 *	and sets it's current argument pointer to the start of that row.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.  If the
 *	there are no more rows remaining in the table, this function
 *	returns ADM_ERR_ENDOFTABLE and leaves the handle intact.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_nextr(
	Adm_data *handlep
	)
{
	if (handlep == NULL) {			/* Handle valid? */
		return(ADM_ERR_BADHANDLE);
	}
	if (handlep->first_rowp == NULL) {	/* Handle has formatted data ? */
		return(ADM_ERR_NOFRMT);
	}
	if (handlep->current_rowp == NULL) {	/* Handle has current row? */
		return(ADM_ERR_NOROW);
	}

	/* Advance the handle to the next row in its table */

	if (handlep->current_rowp->next_rowp == NULL) {
		return(ADM_ERR_ENDOFTABLE);
	}

	handlep->current_rowp = handlep->current_rowp->next_rowp;
	handlep->current_alinkp = NULL;
	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_RESET( handlep ):
 *
 *	Reset the specified administrative data handle to read from the
 *	start of its formatted data table.  This function sets the handle's
 *	current row pointer to the first row in the table, and sets the
 *	handle's current argument pointer to the beginning of that row.
 *	This routine returns ADM_SUCCESS upon normal completion.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_reset(
	Adm_data *handlep
	)
{
	if (handlep == NULL) {			/* Handle valid? */
		return(ADM_ERR_BADHANDLE);
	}
	if (handlep->first_rowp == NULL) {	/* Handle has formatted data ? */
		return(ADM_ERR_NOFRMT);
	}

	/* Reset the handle to the start of the table */

	handlep->current_rowp = handlep->first_rowp;
	handlep->current_alinkp = NULL;
	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_RSTI():
 *
 *	Reset the current argument pointer in the method's input argument
 *	handle to the beginning of the input arguments.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_rsti()
{
	int stat;

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	if (adm_input_args == NULL) {
		return(ADM_ERR_METHONLY);
	}

	/* Reset the input argument handle */

	stat = adm_args_reset(adm_input_args);

	return(stat);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_PUTA( handlep, name, type, length, valuep ):
 *
 *	Create an argument with the specified name, type, length, and
 *	value in the current row of the specified handle's formatted
 *	data table.  The new argument will be located immediately
 *	following the current argument (if the current argument pointer
 *	is NULL, the new argument is placed at the beginning of the
 *	row).  If no formatted data table exists in the handle, one will
 *	be created containing one row with the new argument.  This function
 *	will set the current argument pointer in the handle to point to
 *	the new created argument.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_puta(
	Adm_data *handlep,
	char *name,
	u_int type,
	u_int length,
	caddr_t valuep
	)
{
	boolean_t created_table;  /* Did we create a table for this handle? */
	boolean_t change_pred;	  /* Will predecessor of exising copy of arg */
				  /* change when the new copy is inserted? */
	Adm_arglink *old_alinkp;  /* Ptr to exiting copy of arg being replaced */
				  /* (if there is an old copy of the arg) */
	Adm_arglink *old_predp;   /* Ptr to predecessor of existing copy */
	int stat;

	if (handlep == NULL) {
		return(ADM_ERR_BADHANDLE);
	}

	/*
	 * Validate that name is not NULL.
	 */
	if (name == NULL) {
		return(ADM_ERR_NULLSTRPTR);
	}

	/* Validate argument name length and type */

	if (strlen(name) > (size_t) ADM_MAX_NAMESIZE) {
		return(ADM_ERR_NAMETOOLONG);
	}
	if (type != ADM_STRING) {
		return(ADM_ERR_BADTYPE);
	}

	/*
	 * If the handle does not contain a table, then created one in it.
	 * If the table already exists, search for an existing occurence
	 * of the specified argument.
	 */

	if (handlep->first_rowp == NULL) {
		stat = adm_args_insr(handlep);
		if (stat != ADM_SUCCESS) {
			return(stat);
		}
		created_table = B_TRUE;
		old_predp = NULL;
		old_alinkp = NULL;
	} else {
		adm_args_finda(handlep, name, &old_predp, &old_alinkp);
		created_table = B_FALSE;
	}
	
	/*
	 * Insert the new argument into the formatted data table.  If this
	 * insertion changes the predecessor of the existing copy of the
	 * argument, then change the existing copies predecessor pointer.
	 */

	change_pred = (old_predp == handlep->current_alinkp);
	stat = adm_args_insa(handlep, name, type, length, valuep);
	if (stat != ADM_SUCCESS) {
		if (created_table) {
			adm_args_delr(handlep, NULL, handlep->first_rowp);
		}
		return(stat);
	}

	if (change_pred) {
		old_predp = handlep->current_alinkp;
	}

	/* Delete the old copy of the argument, if there was one */

	if (old_alinkp != NULL) {
		adm_args_remv(handlep, old_predp, old_alinkp);
	}

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_PUTU( handlep, datap, datalen ):
 *
 *	Set the value of the unformatted data block in the specified
 *	handle.  datap should point to a block of length datalen
 *	containing the unformatted data.  If datap is NULL, this function
 *	will erase any unformatted data stored in the handle.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_putu(
	Adm_data *handlep,
	caddr_t datap,
	u_int datalen
	)
{
	u_int new_unformatted_len;	/* Length of new data block */
	caddr_t new_unformattedp;	/* Pointer to copy of new data block */

	if (handlep == NULL) {
		return(ADM_ERR_BADHANDLE);
	}

	/* Create a copy of the unformatted data block */

	if (datap != NULL) {
		new_unformattedp = malloc((size_t)(datalen + 1));
		if (new_unformattedp == NULL) {
			return(ADM_ERR_NOMEM);
		}
		memcpy(new_unformattedp, datap, (size_t) datalen);
		new_unformattedp[datalen] = '\0';
		new_unformatted_len = datalen;
	} else {
		new_unformattedp = NULL;
		new_unformatted_len = 0;
	}

	/* Free the old data block */

	if (handlep->unformattedp != NULL) {
		free(handlep->unformattedp);
	}

	/* Set the new data block value in the handle */

	handlep->unformatted_len = new_unformatted_len;
	handlep->unformattedp = new_unformattedp;

	return(ADM_SUCCESS);
}

/*
 *---------------------------------------------------------------------
 *
 * ADM_ARGS_SET( name, type, length, valuep ):
 *
 *	Set the value of an output argument.  The specified argument is
 *	written in string format to the standard formatted output file
 *	descriptor in the form:
 *
 *		<size>+name(type,length)value value value ...\n
 *
 *	where <size> is the total number of characters between the
 *	argument indication character (+) and the appended newline (\n),
 *	inclusive.
 *
 *	This routine returns ADM_SUCCESS upon normal completion.
 *
 *	Note: This routine is not thread safe.  If multiple threads
 *	      concurrently attempt to write arguments or errors to
 *	      STDFMT, unpredictable results will occur.  A mutex lock
 *	      on the STDMFT pipe is needed to gaurd against concurrent
 *	      access.
 *
 *---------------------------------------------------------------------
 */
int
adm_args_set(
	char *name,
	u_int type,
	u_int length,
	caddr_t valuep
	)
{
	u_int arg_size; /* Size of argument string representation */
	u_int estimate;	/* Estimate of argument header size */
	u_int actual;	/* Actual size of argument header */
	char *buf;	/* Buffer to hold argument header */
	int nitems;
	int stat;

	if (name == NULL) {
		return(ADM_ERR_BADNAME);
	}
	if (strlen(name) > (size_t) ADM_MAX_NAMESIZE) {
		return(ADM_ERR_NAMETOOLONG);
	}
	if (type != ADM_STRING) {
		return(ADM_ERR_BADTYPE);
	}

	stat = adm_init();
	if (stat != ADM_SUCCESS) {
		return(stat);
	}

	if (adm_stdfmt == NULL) {
		return(ADM_ERR_METHONLY);
	}

	/*
	 * Create the header for the string representation of the argument.
	 */

	estimate = (u_int) adm_args_hdrsize(name, type, length, valuep);
	buf = malloc((size_t)(estimate + 1));
	if (buf == NULL) {
		return(ADM_ERR_NOMEM);
	}
	stat = adm_args_a2hdr(name, type, length, valuep,
				buf, (u_int)(estimate + 1), &actual);
	if (stat != ADM_SUCCESS) {
		free(buf);
		return(stat);
	}

	/*
	 * Write the argument to the standard formatted file descriptor.
	 */

	arg_size = actual
		 + (valuep == NULL ? 0 : length)
		 + 1;
	stat = fprintf(adm_stdfmt, "%u", arg_size);
	if (stat == EOF) {
		free(buf);
		return(ADM_ERR_OUTFAIL);
	}
	nitems = fwrite(buf, (size_t) actual, (size_t) 1, adm_stdfmt);
	free(buf);
	if ((nitems != 1) || (ferror(adm_stdfmt) != 0)) {
		return(ADM_ERR_OUTFAIL);
	}
	if ((valuep != NULL) && (length != 0)) {
		nitems = fwrite(valuep, (size_t) length, (size_t) 1, adm_stdfmt);
		if ((nitems != 1) || (ferror(adm_stdfmt) != 0)) {
			return(ADM_ERR_OUTFAIL);
		}
	}
	nitems = fwrite("\n", (size_t) 1, (size_t) 1, adm_stdfmt);
	if ((nitems != 1) || (ferror(adm_stdfmt) != 0)) {
		return(ADM_ERR_OUTFAIL);
	}

	return(ADM_SUCCESS);
}

#endif /* !_adm_args_c */

