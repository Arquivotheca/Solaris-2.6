#ifndef lint
#pragma ident   "@(#)disk_dfltmnt.c 1.61 95/04/07"
#endif
/*
 * Copyright (c) 1991-1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyrigh
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Governmen
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */
/*
 * This module contains functions which manipulate the default mount
 * structures
 */
#include "disk_lib.h"

/* Public Function Prototypes */

Defmnt_t **	get_dfltmnt_list(Defmnt_t **);
int		set_dfltmnt_list(Defmnt_t **);
Defmnt_t **	free_dfltmnt_list(Defmnt_t **);
int 		get_dfltmnt_ent(Defmnt_t *, char *);
int		set_dfltmnt_ent(Defmnt_t *, char *);
void 		update_dfltmnt_list(void);
int		set_client_space(int, int, int);

/* Library Function Prototypes */

/* Local Function Prototypes */

/* Default status fields for each machine type */

static int 	defmnts_server[] = 	{ 1, 1, 1, 1, 1, 2, 1, 0, 2, 1, 0};
static int 	defmnts_dataless[] = 	{ 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int 	defmnts_standalone[] = 	{ 1, 1, 1, 2, 0, 0, 0, 0, 2, 2, 0};
static int 	defmnts_cacheos[] = 	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

/* status field modification masks for each machine type */

static int 	defallow_server[] = 	{ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0};
static int 	defallow_dataless[] = 	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int 	defallow_standalone[] =	{ 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0};
static int 	defallow_cacheos[] =	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*
 * This table contains a list of the supported default mount points used
 * during install. Note that explicit fs should precede wildsliced filesystems
 * for best fit during default placement.
 */

static Defmnt_t  defmnts[] = {
	{ "/",			1,	ROOT_SLICE,	1,	0,	-1 },
	{ SWAP,			1,	SWAP_SLICE,	1,	0,	-1 },
	{ "/usr",		1,	6,		1,	0,	-1 },
	{ "/opt",		1,	5,		1,	0,	-1 },
	{ "/export",		1,	3,		1,	0,	-1 },
	{ "/export/root",	1,	7,		1,	0,	 0 },
	{ "/export/swap",	1,	4,		1,	0,	 0 },
	{ "/export/exec",	1,	WILD_SLICE,	1,	0,	-1 },
	{ "/usr/openwin",	1,	WILD_SLICE,	1,	0,	-1 },
	{ "/var",		1,	WILD_SLICE,	1,	0,	-1 },
	{ "/.cache",		1,	ROOT_SLICE,	1,	0,	-1 },
	};

int		numdefmnt = sizeof (defmnts) / sizeof (Defmnt_t);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ********************************************************************	*/
/*
 * get_dfltmnt_list()
 *	Retrieve an array of Defmnt_t pointers correlating to the default
 *	mount point list. The 'status' and 'allowed' field values depend
 *	upon the machinetype (MT_SERVER, MT_DATALESS, MT_STANDALONE,
 *	MT_CCLIENT). The array is terminated with a NULL Defmnt_t pointer.
 * Parameters:
 *	mpp	- address of mount list pointer as returned by a previous
 *		  call to get_dfltmnt_list() (NULL if you want the call
 *		  to malloc a new structure for you)
 * Return:
 *	Defmnt ** - non-NULL pointer to array of default mount structures
 *		    (same as passed in, if 'mpp' is not NULL)
 *	NULL	  - malloc failure
 * Status:
 *	public
 * Note:
 *	malloc
 */
Defmnt_t **
get_dfltmnt_list(Defmnt_t **mpp)
{
	int	  i;

	/* malloc and initialize the list structure if one was not passed in */

	if (mpp == NULL) {
		mpp = (Defmnt_t **) malloc(sizeof (Defmnt_t *) *
					(NUMDEFMNT + 1));
		if (mpp == NULL)
			return (NULL);
		(void) memset(mpp, 0, sizeof (Defmnt_t *) * (NUMDEFMNT + 1));
	}

	for (i = 0; i < numdefmnt; i++) {
		if (mpp[i] == NULL)
			mpp[i] = (Defmnt_t *) malloc(sizeof (Defmnt_t));
		(void) get_dfltmnt_ent(mpp[i], defmnts[i].name);
	}

	mpp[i] = NULL;		/* NULL terminate list */
	return (mpp);
}

/*
 * set_dfltmnt_list()
 *	Set the status fields for the entire default mount points list
 *	based on the current machine type (MT_SERVER, MT_STANDALONE,
 *	MT_DATALESS, or MT_CCLIENT only). Only those entries with a
 *	corresponding "allowed" value of '1' will actually be set.
 * Parameters:
 *	mpp	- pointer to an array of Defmnt_t mount point entries
 *		  (most likely as returned by get_dfltmnt_list()) from
 *		  which the status fields are extracted and used to set
 *		  the current machine type status map.
 * Return:
 *	D_OK	 - mount list set successfully
 *	D_BADARG - 'mpp' is NULL, current machinetype is not valid, or 'mpp'
 *		   has an entry which is a non-standard default mount point
 * Status:
 *	public
 */
int
set_dfltmnt_list(Defmnt_t **mpp)
{
	int	i;

	if (mpp == (Defmnt_t **)NULL)
		return (D_BADARG);

	for (i = 0; mpp[i] != (Defmnt_t *)NULL; i++) {
		if (mpp[i]->allowed == 1) {
			if (set_dfltmnt_ent(mpp[i], mpp[i]->name)
						!= D_OK)
				return (D_BADARG);
		}
	}
	return (D_OK);
}

/*
 * free_dfltmnt_list()
 *	Free heap space used by a Defmnt_t array as allocated by
 *	a get_dfltmnt_list() call. To "NULL" the array pointer,
 *	use this routine as:
 *
 *		mpp = free_dfltmnt_list(mpp);
 *
 * Parameters:
 *	mpp	- pointer to an array of Defmnt_t structure pointers
 *		  as allocated by get_dfltmnt_list()
 * Return:
 *	(Defmnt_t **)NULL
 * Status:
 * 	public
 */
Defmnt_t **
free_dfltmnt_list(Defmnt_t **mpp)
{
	int	i;

	if (mpp != (Defmnt_t **)NULL) {
		for (i = 0; mpp[i] != (Defmnt_t *)NULL; i++)
			free(mpp[i]);
		free(mpp);
	}
	return ((Defmnt_t **)NULL);
}

/*
 * set_dfltmnt_ent()
 *	Set the default mount status field for a specific default mount
 *	filesystem based on the current	machine type (MT_SERVER, MT_DATALESS,
 *	MT_STANDALONE, or MT_CCLIENT). The "allowed" field for the entry must
 *	be '1' or the set request will be denied.
 * Parameters:
 *	mp	 - pointer to structure used to retrieve data (NULL if
 *		   function is being used as a boolean test)
 *	fs	 - non-NULL default mount point name for which the
 *		   status is being set (must be in defmnt[])
 * Return:
 *	D_OK	 - mount entry set successfully
 *	D_BADARG - status is invalid, 'fs' is NULL, 'fs' is "", 'fs'
 *		   is not a default mount point. Current machine type
 *		   is not MT_STANDALONE, MT_DATALESS, MT_SERVER, or
 *		   MT_CCLIENT
 * Status:
 *	public
 */
int
set_dfltmnt_ent(Defmnt_t *mp, char *fs)
{
	int	i;

	if (mp->status != DFLT_IGNORE &&
			mp->status != DFLT_SELECT &&
			mp->status != DFLT_DONTCARE)
		return (D_BADARG);

	if (fs == NULL || *fs == '\0')
		return (D_BADARG);

	for (i = 1; i < numdefmnt; i++) {
		if (strcmp(fs, defmnts[i].name) == 0)
			break;
	}

	if (i == numdefmnt)
		return (D_BADARG);

	defmnts[i].expansion = mp->expansion;

	switch (get_machinetype()) {

	case MT_DATALESS:
		if (defallow_dataless[i] != 0)
			defmnts_dataless[i] = mp->status;
		break;
	case MT_SERVER:
		if (defallow_server[i] != 0)
			defmnts_server[i] = mp->status;
		break;
	case MT_STANDALONE:
		if (defallow_standalone[i] != 0)
			defmnts_standalone[i] = mp->status;
		break;
	case MT_CCLIENT:
		if (defallow_cacheos[i] != 0)
			defmnts_cacheos[i] = mp->status;
		break;
	default:
		return (D_BADARG);
	}

	return (D_OK);
}

/*
 * get_dfltmnt_ent()
 *	Get the default mount table entry for 'fs' with the status
 *	field and allowed set according to the current machine type.
 *	If 'mp' is NULL the function can be used as a Boolean test
 *	(D_OK if 'fs' is a default mount point, and D_BADARG if it
 *	is not).
 * Parameters:
 *	mp	- pointer to structure used to retrieve data (NULL if
 *		  function is being used as a boolean test)
 *	fs	- non-NULL name default mount point. Valid values are:
 *
 *			/
 *			swap
 *			/usr
 *			/var
 *			/opt
 *			/export
 *			/export/root
 *			/export/swap
 *			/export/exec
 *			/usr/openwin
 *			/.cache
 * Return:
 *	D_OK		- entry retrieved successfully
 *	D_BADARG	- 'fs' is NULL, is "", or, 'fs' is not a default
 *			  mount point, or the current machine type is not
 *			  MT_DATALESS, MT_SERVER, MT_STANDALONE, MT_CCLIENT,
 *			  or 'mp' is NULL
 * Status:
 *	public
 */
int
get_dfltmnt_ent(Defmnt_t *mp, char *fs)
{
	int	i;

	/* validate parameters */
	if (fs == NULL || *fs == '\0')
		return (D_BADARG);

	/* look for the specified default mount entry */
	for (i = 0; i < numdefmnt; i++) {
		if (strcmp(fs, defmnts[i].name) == 0)
			break;
	}

	if (i == numdefmnt)
		return (D_BADARG);

	if (mp != NULL) {
		switch (get_machinetype()) {
		    case MT_DATALESS:
			mp->status = defmnts_dataless[i];
			mp->allowed = defallow_dataless[i];
			break;

		    case MT_SERVER:
			mp->status = defmnts_server[i];
			mp->allowed = defallow_server[i];
			break;

		    case MT_STANDALONE:
			mp->status = defmnts_standalone[i];
			mp->allowed = defallow_standalone[i];
			break;

		    case MT_CCLIENT:
			mp->status = defmnts_cacheos[i];
			mp->allowed = defallow_cacheos[i];
			break;

		    default:
			return (D_BADARG);
		}

		(void) strcpy(mp->name, defmnts[i].name);
		mp->slice = defmnts[i].slice;
		mp->size = defmnts[i].size;
		mp->expansion = defmnts[i].expansion;
	}

	return (D_OK);
}

/*
 * update_dfltmnt_list()
 *	Get the current software space requirements for the default
 *	disk mount points and update defmnt[].size fields accordingly.
 *	Size data is stored in sectors, though returned as KB from
 *	space_meter();
 *
 *	NOTE:	As of S494, the add_fs_overhead() routine in the
 *		software library fails to add min_req_space() to
 *		the sizes reported if the percent_free_space() is
 *		'0'. This is necessary for swmtool, but creates
 *		a behavior inconsistency in this routine. To
 *		compensate for this behavior, the min_req_space()
 *		is manually added in this routine when percent_free_space()
 *		is '0'.
 *
 * Parameters:
 *	none
 * Return:
 *	none
 * Status:
 *	public
 */
void
update_dfltmnt_list(void)
{
	static char	*_defmplist[NUMDEFMNT + 1];
	static int	_defmpvalid = 0;
	Space 		**sp;
	int		i, j;
	MachineType	mt = get_machinetype();

	for (i = 0; i < numdefmnt; i++)
		defmnts[i].size = 0;

	if (_defmpvalid == 0) {
		for (i = 0; i < numdefmnt; i++)
			_defmplist[i] = xstrdup(defmnts[i].name);
		_defmplist[i] = NULL;	/* terminator */
		_defmpvalid++;
	}

	if (mt != MT_CCLIENT)
		sp = space_meter(_defmplist);

	/*
	 * for software related default mount entries, extract data
	 * from the space table
	 */
	for (j = 0; j < numdefmnt; j++) {
		if (strcmp(defmnts[j].name, SWAP) == 0) {
			defmnts[j].size = get_minimum_fs_size(SWAP,
					NULL, ROLLUP);
		} else {
			if (mt == MT_CCLIENT)
				defmnts[j].size = 0;
			else {
				for (i = 0; sp && sp[i]; i++) {
					if (strcmp(sp[i]->mountp,
							defmnts[j].name) == 0) {
						defmnts[j].size =
						    kb_to_sectors(sp[i]->bused);
						if (percent_free_space() == 0) {
							    defmnts[j].size =
							min_req_space(
							    defmnts[j].size);
						}

						break;
					}
				}
			}
		}
	}
}

/*
 * set_client_space()
 *      Set the client root and swap space expansion requirements for /export/swap
 *	and /export/root.
 * Parameters:
 *      num	- number of clients (valid: # >= 0)
 *      root	- size of a single client root in sectors (valid: # >= 0)
 *      swap	- size of a single client swap in sectors (valid: # >= 0)
 * Return:
 *	D_OK	 - set successful
 *	D_BADARG - invalid argument
 *	D_FAILED - internal error
 * Status:
 *	public
 */
int
set_client_space(int num, int root, int swap)
{
	Defmnt_t	def;

	/* validate parameters */
	if (num < 0 || root < 0 || swap < 0)
		return (D_BADARG);

	/* get the /export/swap entry and set the expansion value */
	if (get_dfltmnt_ent(&def, EXPORTSWAP) != D_OK)
		return (D_FAILED);

	def.expansion = min_req_space(num * swap);
	if (set_dfltmnt_ent(&def, EXPORTSWAP) != D_OK)
		return (D_FAILED);

	/* get the /export/root entry and set the expansion value */
	if (get_dfltmnt_ent(&def, EXPORTROOT) != D_OK)
		return (D_FAILED);

	def.expansion = min_req_space(num * root);
	if (set_dfltmnt_ent(&def, EXPORTROOT) != D_OK)
		return (D_FAILED);

	return (D_OK);
}
