#ifndef lint
#pragma ident "@(#)svc_dfltmnt.c 1.13 96/07/12 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	svc_dfltmnt.c
 * Group:	libspmisvc
 * Description:	Obsoleted default mount entry interfaces used to obtain
 *		status information about "default mount points". These
 *		interfaces have been replaced by the Resobj*() interfaces.
 *		All default entries refer to instance-0 resources.
 */

#include <string.h>
#include <stdlib.h>
#include "spmisvc_lib.h"
#include "spmicommon_api.h"
#include "svc_strings.h"

/* ---------------------- public functions ----------------------- */

/*
 * Function:	free_dfltmnt_list (OBSOLETE)
 * Description:	Free heap space used by a Defmnt_t array as allocated by
 *		a get_dfltmnt_list() call.
 * Scope:	public
 * Parameters:	mpp	[RO, *RW, **RW] (Defmnt_t **)
 *			Pointer to an array of Defmnt_t structure pointers
 *			allocated by get_dfltmnt_list(), to be deallocated.
 * Return:	NULL	always returns this value
 */
Defmnt_t **
free_dfltmnt_list(Defmnt_t **mpp)
{
	int	i;

	/* validate parameters */
	if (mpp == NULL)
		return (NULL);

	/* free each of the structures within the array */
	for (i = 0; mpp[i] != NULL; i++)
		free(mpp[i]);

	/* free the array itself */
	free(mpp);

	return (NULL);
}

/*
 * Function:	get_dfltmnt_ent (OBSOLETE)
 * Description:	Get the status related resource information for a specific
 *		default entry. The status is set according to the current
 *		machine type. Data returned includes status (DFLT_SELECT,
 *		DFLT_IGNORE, DFLT_DONTCARE) and status modification
 *		permissions (0 - no modification allowed; 1 - modifications
 *		permitted).
 * Scope:	public
 * Parameters:	mp	[RO, *RO] (Defmnt_t *)
 *			Pointer to structure used to retrieve data; NULL if
 *			function is being used as a boolean test
 *		name	[RO, *RO] (char *)
 *			Non-NULL name default mount point. Valid values are:
 *			    /
 *			    swap
 *			    /usr
 *			    /usr/openwin
 *			    /var
 *			    /opt
 *			    /export
 *			    /export/root
 *			    /export/swap
 *			    /export/exec
 *			    /.cache
 * Return:	D_OK	  entry retrieved successfully
 *		D_BADARG  invalid argument, or name specified is not a default
 *			  name
 */
int
get_dfltmnt_ent(Defmnt_t *mp, char *name)
{
	ResobjHandle	res;
	ResOrigin_t	origin;
	ResStat_t	status;
	ResMod_t	modify;
	int		slice;
	int		size;
	int		extra;
	int		services;

	/* validate parameters */
	if (!ResobjIsValidName(name))
		return (D_BADARG);

	if ((res = ResobjFind(name, 0)) == NULL)
		return (D_BADARG);

	/* get the resource attributes */
	if (ResobjGetAttribute(res,
			RESOBJ_STATUS,		  &status,
			RESOBJ_MODIFY,		  &modify,
			RESOBJ_ORIGIN,		  &origin,
			RESOBJ_DEV_DFLTDEVICE,	  &slice,
			RESOBJ_CONTENT_SOFTWARE,  &size,
			RESOBJ_CONTENT_EXTRA,	  &extra,
			RESOBJ_CONTENT_SERVICES,  &services,
			NULL) != D_OK)
		return (D_BADARG);

	/* make sure the entry has a default origin */
	if (origin != RESORIGIN_DEFAULT)
		return (D_BADARG);

	/* if the user provided a retrieval structure, populate it */
	if (mp != NULL) {
		(void) strcpy(mp->name, name);
		switch (status) {
		    case RESSTAT_INDEPENDENT:
			mp->status = DFLT_SELECT;
			break;
		    case RESSTAT_DEPENDENT:
			mp->status = DFLT_IGNORE;
			break;
		    case RESSTAT_OPTIONAL:
			mp->status = DFLT_DONTCARE;
			break;
		}

		switch (modify) {
		    case RESMOD_SYS:
			mp->allowed = 0;
			break;
		    case RESMOD_ANY:
			mp->allowed = 1;
			break;
		}
	}

	return (D_OK);
}

/*
 * Function:	get_dfltmnt_list (OBSOLETE)
 * Description: Retrieve an array of Defmnt_t pointers correlating to the
 *		default entry list. The status and allowed values depend upon
 *		the machinetype (MT_SERVER, MT_STANDALONE, MT_CCLIENT). The
 *		array is terminated with a NULL Defmnt_t pointer.
 *
 *		WARNING: the resource object list MUST be initialized before
 *			 calling this function if the user expects to reuse
 *			 the array provided
 *
 * Scope:	public
 * Parameters:	mpp	[RO, *RW, **RW] (Defmnt_t **)
 *			Address of mount list pointer as returned by a previous
 *			call to get_dfltmnt_list(); NULL if you want a new
 *			structure malloc'd
 * Return:	Defmnt **   non-NULL pointer to array of default mount
 *			    structures (same as passed in, if 'mpp' is not
 *			    NULL)
 *		NULL	    malloc failure
 */
Defmnt_t **
get_dfltmnt_list(Defmnt_t **mpp)
{
	ResobjHandle	res;
	int		count;
	int		i;

	/*
	 * count the maximum number of default origin resources, including
	 * ignored resources, and adding one extra for the list terminator
	 */
	count = 1;
	WALK_RESOURCE_LIST_PRIV(res, RESTYPE_UNDEFINED) {
		if (Resobj_Origin(res) == RESORIGIN_DEFAULT)
			count++;
	}

	/* allocate a new default list if one wasn't provided */
	if (mpp == NULL) {
		mpp = (Defmnt_t **) xcalloc(sizeof (Defmnt_t *) * (count));
		if (mpp == NULL)
			return (NULL);
	}

	/* get the information for each non-ignored default origin resource */
	i = 0;
	WALK_RESOURCE_LIST(res, RESTYPE_UNDEFINED) {
		if (Resobj_Origin(res) != RESORIGIN_DEFAULT)
			continue;

		/* allocate a new structure if one doesn't alreday exist */
		if (mpp[i] == NULL)
			mpp[i] = (Defmnt_t *) xcalloc(sizeof (Defmnt_t));

		/* get the entry */
		if (get_dfltmnt_ent(mpp[i], Resobj_Name(res)) == D_OK)
			i++;
	}

	/*
	 * free all remaining entries which may have been previously
	 * allocated
	 */
	for (; i < count; i++) {
		if (mpp[i] != NULL) {
			free(mpp[i]);
			mpp[i] = NULL;
		}
	}

	return (mpp);
}

/*
 * Function:	print_dfltmnt_list (OBSOLETE)
 * Description:	Print out the contents of a default entry list.
 * Scope:	public
 * Parameters:	comment	[RO, *RO] (const char *)
 *			Comment string to be put in message; NULL if none.
 *		mlp	[RO, *RO, **RO] (Defmnt_t **)
 *			Pointer to array of default mount list structures.
 * Return:	none
 */
void
print_dfltmnt_list(const char *comment, Defmnt_t **mlp)
{
	Defmnt_t	**dfltp;
	Defmnt_t	*def_me;

	write_status(SCR, LEVEL0|CONTINUE,
		"%s %s",
		MSG0_TRACE_MOUNT_LIST, comment == NULL ? "" : comment);

	for (dfltp = mlp; *dfltp; dfltp++) {
		def_me = *dfltp;
		write_status(SCR, LEVEL1,
			"\t%-25.25s %10s",
			def_me->name,
			def_me->status == DFLT_SELECT ? "selected" :
				def_me->status == DFLT_DONTCARE ?
					"optional" : "ignore");
	}
}

/*
 * Function:	set_dfltmnt_ent (OBSOLETE)
 * Description:	Set the default entry status for a specific entry given the
 *		current machine type (MT_SERVER, MT_STANDALONE, or MT_CCLIENT).
 *		Only entries which allow status modification will be honored.
 * Scope:	public
 * Parameters:	mp	[RO, *RO] (Defmnt_t *)
 *			Pointer to structure used to retrieve data; NULL if
 *			function is being used as a boolean test
 *		name	[RO, *RO] (char *)
 *			Non-NULL default entry name for which the status is
 *			to be set
 * Return:	D_OK	  entry status set successfully
 *		D_BADARG  invalid entry name or invalid status
 *		D_FAILED  attempt to set entry status failed
 */
int
set_dfltmnt_ent(Defmnt_t *mp, char *name)
{
	ResobjHandle	res;
	ResMod_t	modify;
	ResStat_t	status;

	/* validate arguments */
	if (mp->status != DFLT_IGNORE &&
			mp->status != DFLT_SELECT &&
			mp->status != DFLT_DONTCARE)
		return (D_BADARG);

	/* find the default instance for the specified resource */
	if ((res = ResobjFind(name, 0)) == NULL)
		return (D_BADARG);

	if (ResobjGetAttribute(res,
			RESOBJ_MODIFY,	&modify,
			NULL) != D_OK)
		return (D_FAILED);

	if (modify != RESMOD_SYS) {
		switch (mp->status) {
		    case DFLT_SELECT:
			status = RESSTAT_INDEPENDENT;
			break;
		    case DFLT_IGNORE:
			status = RESSTAT_DEPENDENT;
			break;
		    case DFLT_DONTCARE:
			status = RESSTAT_OPTIONAL;
			break;
		}

		if (ResobjSetAttribute(res,
				RESOBJ_STATUS,	  status,
				NULL) != D_OK)
			return (D_FAILED);
	}

	return (D_OK);
}

/*
 * Function:	set_dfltmnt_list (OBSOLETE)
 * Description: Set the status fields for all default entries based on the
 *		current machine type (MT_SERVER, MT_STANDALONE, or MT_CCLIENT).
 *		Modifications will only be made for entries which allow status
 *		update.
 * Scope:	public
 * Parameters:	mpp	[RO, *RO, **RO] (Defmnt_t **)
 *			Defmnt_t mount entry pointer array returned by
 *			get_dfltmnt_list() containing updated status fields.
 * Return:	D_OK	  mount list set successfully
 *		D_BADARG  invalid argument, or failure to update a modifiable
 */
int
set_dfltmnt_list(Defmnt_t **mpp)
{
	int	i;

	/* validate parameters */
	if (mpp == NULL)
		return (D_BADARG);

	for (i = 0; mpp[i] != NULL; i++) {
		if (mpp[i]->allowed) {
			if (set_dfltmnt_ent(mpp[i], mpp[i]->name) != D_OK)
				return (D_BADARG);
		}
	}

	return (D_OK);
}
