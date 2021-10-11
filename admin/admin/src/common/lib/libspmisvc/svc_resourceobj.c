#ifndef lint
#pragma ident   "@(#)svc_resourceobj.c 1.23 96/10/10"
#endif
/*
 * Copyright (c) 1996 Sun Microsystems, Inc.  All Rights Reserved.
 */
/*
 * Module:	svc_dfltrsrc.c
 * Group:	libspmisvc
 * Description: This module contains methods which manipulate resource
 *		objects.
 */
#include <stdlib.h>
#include "spmisvc_lib.h"
#include "spmistore_lib.h"
#include "spmisoft_api.h"
#include "spmicommon_api.h"

/* private prototypes */

static void		ResobjDeallocate(Resobj *);
static Resobj *		ResobjCreateDup(Resobj *);
static Resobj **	ResobjFindReference(Resobj *);
static void		ResobjAddToList(Resobj *);
static int		ResobjIsExistingPriv(ResobjHandle);
static int		coreResobjDestroy(Resobj *, int);
static Resobj * 	coreResobjCreate(ResType_t, char *, int, int, va_list);
static int		coreResobjSetAttribute(Resobj *, int, va_list);
static int		coreResobjGetAttribute(Resobj *, int, va_list);

/* global variable used to store resource list */

static Resobj *		_ResourceList = NULL;

/*
 * This table defines the default resource attributes which are used to
 * configure the default mount list.
 */
typedef struct {
	char *		name;
	ResType_t	type;
	ResClass_t	class;
	Sysstat		state;
	int		default_slice;
} ResDefault;

static ResDefault	_ResourceDefault[] = {
	{ ROOT,			/* name */
	  RESTYPE_DIRECTORY,	/* type */
	  RESCLASS_DYNAMIC,	/* class */
	  { { RESSTAT_INDEPENDENT,	RESMOD_SYS },	/* standalone */
	    { RESSTAT_INDEPENDENT,	RESMOD_SYS },	/* server */
	    { RESSTAT_IGNORED,		RESMOD_SYS } },	/* autoclient */
	  0			/* default slice */
	},
	{ SWAP,			/* name */
	  RESTYPE_SWAP,		/* type */
	  RESCLASS_STATIC,	/* class */
	  { { RESSTAT_INDEPENDENT,	RESMOD_ANY },	/* standalone */
	    { RESSTAT_INDEPENDENT, 	RESMOD_ANY },	/* server */
	    { RESSTAT_IGNORED,		RESMOD_SYS } },	/* autoclient */
	  1			/* default slice */
	},
	{ USR,			/* name */
	  RESTYPE_DIRECTORY,	/* type */
	  RESCLASS_STATIC,	/* class */
	  { { RESSTAT_INDEPENDENT,	RESMOD_ANY },	/* standalone */
	    { RESSTAT_INDEPENDENT,	RESMOD_ANY },	/* server */
	    { RESSTAT_IGNORED,		RESMOD_SYS } },	/* autoclient */
	  6			/* default slice */
	},
	{ OPT,			/* name */
	  RESTYPE_DIRECTORY,	/* type */
	  RESCLASS_REPOSITORY,	/* class */
	  { { RESSTAT_OPTIONAL,		RESMOD_ANY },	/* standalone */
	    { RESSTAT_INDEPENDENT,	RESMOD_ANY },	/* server */
	    { RESSTAT_IGNORED,		RESMOD_SYS } },	/* autoclient */
	  5			/* default slice */
	},
	{ EXPORT,		/* name */
	  RESTYPE_DIRECTORY,	/* type */
	  RESCLASS_REPOSITORY,	/* class */
	  { { RESSTAT_IGNORED,		RESMOD_SYS },	/* standalone */
	    { RESSTAT_INDEPENDENT,	RESMOD_ANY },	/* server */
	    { RESSTAT_IGNORED,		RESMOD_SYS } },	/* autoclient */
	  3			/* default slice */
	},
	{ EXPORTROOT,		/* name */
	  RESTYPE_DIRECTORY,	/* type */
	  RESCLASS_DYNAMIC,	/* class */
	  { { RESSTAT_IGNORED,		RESMOD_SYS },	/* standalone */
	    { RESSTAT_OPTIONAL,		RESMOD_ANY },	/* server */
	    { RESSTAT_IGNORED,		RESMOD_SYS } },	/* autoclient */
	  7			/* default slice */
	},
	{ EXPORTSWAP,		/* name */
	  RESTYPE_DIRECTORY,	/* type */
	  RESCLASS_STATIC,	/* class */
	  { { RESSTAT_IGNORED,		RESMOD_SYS },	/* standalone */
	    { RESSTAT_INDEPENDENT,	RESMOD_ANY },	/* server */
	    { RESSTAT_IGNORED,		RESMOD_SYS } },	/* autoclient */
	  4			/* default slice */
	},
	{ EXPORTEXEC,		/* name */
	  RESTYPE_DIRECTORY,	/* type */
	  RESCLASS_REPOSITORY,	/* class */
	  { { RESSTAT_IGNORED,		RESMOD_SYS },	/* standalone */
	    { RESSTAT_DEPENDENT,	RESMOD_ANY },	/* server */
	    { RESSTAT_IGNORED,		RESMOD_SYS } },	/* autoclient */
	  VAL_UNSPECIFIED	/* default slice */
	},
	{ USROWN,		/* name */
	  RESTYPE_DIRECTORY,	/* type */
	  RESCLASS_STATIC,	/* class */
	  { { RESSTAT_OPTIONAL,		RESMOD_ANY },	/* standalone */
	    { RESSTAT_OPTIONAL,		RESMOD_ANY },	/* server */
	    { RESSTAT_IGNORED,		RESMOD_SYS } },	/* autoclient */
	  VAL_UNSPECIFIED	/* default slice */
	},
	{ VAR,			/* name */
	  RESTYPE_DIRECTORY,	/* type */
	  RESCLASS_DYNAMIC,	/* class */
	  { { RESSTAT_OPTIONAL,		RESMOD_ANY },	/* standalone */
	    { RESSTAT_INDEPENDENT,	RESMOD_ANY },	/* server */
	    { RESSTAT_IGNORED,		RESMOD_SYS } },	/* autoclient */
	  VAL_UNSPECIFIED	/* default slice */
	},
	{ CACHE,		/* name */
	  RESTYPE_DIRECTORY,	/* type */
	  RESCLASS_DYNAMIC,	/* class */
	  { { RESSTAT_IGNORED,		RESMOD_SYS },	/* standalone */
	    { RESSTAT_IGNORED,		RESMOD_SYS },	/* server */
	    { RESSTAT_INDEPENDENT,	RESMOD_SYS } },	/* autoclient */
	  0			/* default slice */
	},
	{ CACHESWAP,		/* name */
	  RESTYPE_SWAP,		/* type */
	  RESCLASS_STATIC,	/* class */
	  { { RESSTAT_IGNORED,		RESMOD_SYS },	/* standalone */
	    { RESSTAT_IGNORED,		RESMOD_SYS },	/* server */
	    { RESSTAT_DEPENDENT,	RESMOD_SYS } },	/* autoclient */
	  VAL_UNSPECIFIED	/* default slice */
	},
	{ "",			/* list terminator */
	  RESTYPE_UNDEFINED,
	  RESCLASS_UNDEFINED,
	  { { RESSTAT_UNDEFINED,	RESMOD_UNDEFINED },
	    { RESSTAT_UNDEFINED,	RESMOD_UNDEFINED },
	    { RESSTAT_UNDEFINED,	RESMOD_UNDEFINED } },
	  VAL_UNSPECIFIED
	}
};

/* constants */

#define	DEBUG_ENTRY_FORMAT	"%-20s = %s"
#define	DEBUG_ENTRY_SIZE_FORMAT	"%-20s = %d sectors"
#define	DEBUG_ENTRY_NUM_FORMAT	"%-20s = %d"

/* -------------------------- public functions ------------------------ */

/*
 * Function:	ResobjPrintStatusList
 * Description:	Print the English representation of a resource status
 *		list.
 * Scope:	public
 * Parameters:	list	[RO, *RO] (ReeStatEntry *)
 *			Pointer to head of resource status list.
 * Return:	none
 */
void
ResobjPrintStatusList(ResStatEntry *list)
{
	ResStatEntry *	rep;
	ResobjHandle	res;

	write_status(SCR, LEVEL0|CONTINUE, "Resource Entry List:");
	WALK_LIST(rep, list) {
		if ((res = rep->resource) == NULL)
			continue;

		write_status(SCR, LEVEL1,
			"(%2d) %20s [%s]",
			Resobj_Instance(res),
			Resobj_Name(res),
			Resobj_Status(res) == RESSTAT_INDEPENDENT ?
				"independent" :
			Resobj_Status(res) == RESSTAT_DEPENDENT ?
				"dependent" :
			Resobj_Status(res) == RESSTAT_OPTIONAL ?
				"optional" : NULL);
	}
}

/*
 * Function:	ResobjGetStatusList
 * Description:	Retrieve a list of status entry structures for all non-ignored
 *		resources. This function dynamically allocates memory which
 *		should be freed by the caller using ResobjFreeStatusList().
 * Scope:	public
 * Parameters:	listp	[RO, *RW, **RW] (ResStatEntry)
 *			address of pointer to head of list of resource
 *			status entry structures.
 * Return:	D_OK	  successfully retrieved status list
 *		D_FAILED  failed to retrieve status list
 */
int
ResobjGetStatusList(ResStatEntry **listp)
{
	ResStatEntry ** rpp;
	ResobjHandle	res;

	/* validate parameters */
	if (listp == NULL)
		return (D_FAILED);

	/* if there was already a list allocated, free it */
	if (*listp != NULL) {
		ResobjFreeStatusList(*listp);
		*listp = NULL;
	}

	rpp = listp;
	WALK_RESOURCE_LIST(res, RESTYPE_UNDEFINED) {
		if (((*rpp) = (ResStatEntry *)xmalloc(
				sizeof (ResStatEntry))) == NULL) {
			ResobjFreeStatusList(*listp);
			return (D_FAILED);
		}

		(*rpp)->resource = res;
		(*rpp)->status = Resobj_Status(res);
		(*rpp)->next = NULL;

		rpp = &((*rpp)->next);
	}

	return (D_OK);
}

/*
 * Function:	ResobjSetStatusList
 * Description:	Update the resource object statuses based on the list provided.
 *
 *		WARNING: If this routine encounters an error during updating
 *			 it will leave the resource state partially updated.
 *
 * Scope:	public
 * Parameters:	list	[RO, *RO] (ResStatEntry *)
 *			Pointer to head of list of resource status entries
 *			to be used to update the current configuration of
 *			the resource list.
 * Return:	D_OK	  succesfully updated the resource statuses
 *		D_FAILED  failed to updated all resource statuses
 */
int
ResobjSetStatusList(ResStatEntry *list)
{
	ResStatEntry *  rp;
	int		status = D_OK;

	/* validate parameters */
	if (list == NULL)
		return (D_FAILED);

	WALK_LIST(rp, list) {
		if (!ResobjIsExisting(rp->resource) ||
				ResobjIsIgnored(rp->resource))
			continue;

		if ((status = ResobjSetAttribute(rp->resource,
				RESOBJ_STATUS,  rp->status,
				NULL)) != D_OK)
			break;
	}

	return (status);
}

/*
 * Function:	ResobjFreeStatusList
 * Description:	Deallocate all dynamically allocated space associated with a
 *		dynamically allcoated resource status entry list.
 * Scope:	public
 * Parameters:	list	[RO, *RW] (ResStatEntry *)
 *			Address of a pointer to head of resource status entries
 *			to be freed.
 * Return:	none
 */
void
ResobjFreeStatusList(ResStatEntry *list)
{
	ResStatEntry *	rp;
	ResStatEntry *	next;

	for (rp = list; rp != NULL; rp = next) {
		next = rp->next;
		free(rp);
	}
}

/*
 * Function:	ResobjSetAttribute
 * Description:	Modify the attributes associated with a resource object.
 *		Available attributes are:
 *
 *		Attribute		  Value	    Description
 *		-----------		  -------   -------------
 *		RESOBJ_DEV_PREFDISK	  char *    preferred disk for storage
 *		RESOBJ_DEV_PREFDEVICE	  int	    preferred slice for storage
 *		RESOBJ_DEV_EXPLDISK	  char *    required disk for storage
 *		RESOBJ_DEV_EXPLDEVICE	  int	    required device for storage
 *		RESOBJ_DEV_EXPLSTART	  int	    required starting cylinder
 *		RESOBJ_DEV_EXPLSIZE	  int	    required sectors for storage
 *		RESOBJ_DEV_EXPLMINIMUM	  int	    required minimum sectors
 *						    for storage
 *		RESOBJ_FS_ACTION	  FsAct_t   file system processing
 *						    directive for directory
 *						    resources with independent
 *						    storage
 *		RESOBJ_FS_MINFREE	  int	    explicit system percent
 *						    free space (VAL_UNSPECIFIED
 *						    for undefined)
 *		RESOBJ_FS_PERCENTFREE	  int	    explicit user percent free
 *						    space (VAL_UNSPECIFIED for
 *						    undefined)
 *		RESOBJ_FS_MOUNTOPTS	  char *    mount options for directory
 *						    resources with independent
 *						    storage
 *		RESOBJ_CONTENT_CLASS	  ResClass_t *
 *						    content behavior class
 *		RESOBJ_CONTENT_EXTRA	  int	    number of reserved (extra)
 *						    sectors
 *		RESOBJ_CONTENT_SERVICES	  int	    number of sectors for
 *						    client root/swap services
 *
 * Scope:	public
 * Parameters:	res	[RO] (ResobjHandle)
 *			Resource handle.
 *		...	keyword/attribute value pairs for modification
 * Return:	D_OK		modification successful
 *		D_BADARG 	modification failed
 */
int
ResobjSetAttribute(ResobjHandle res, ...)
{
	va_list	  ap;
	int	  status;

	/* validate parameters */
	if (res == NULL ||
			ResobjIsIgnored((Resobj *)res) ||
			!ResobjIsExisting(res))
		return (D_BADARG);

	va_start(ap, res);
	status = coreResobjSetAttribute((Resobj *)res, NOPRIVILEGE, ap);
	va_end(ap);

	return (status);
}

/*
 * Function:	ResobjGetAttribute
 * Description: Retrieve attributes associated with a specific resource.
 *		Available attribute values are:
 *
 *		Attribute		  Value	    Description
 *		-----------		  -------   -------------
 *		RESOBJ_NAME		  char[MAXNAMELEN]
 *						    resource name
 *		RESOBJ_INSTANCE		  int	    resource instance
 *		RESOBJ_ORIGIN		  ResOrigin_t
 *						    resource creation origin
 *		RESOBJ_STATUS		  ResStat_t resource status
 *		RESOBJ_MODIFY		  ResMod_t  resource status modification
 *						    permission
 *		RESOBJ_DEV_DFLTDEVICE	  int	    default slice to use during
 *						    layout (VAL_UNSPECIFIED if
 *						    undefined)
 *		RESOBJ_CONTENT_CLASS	  ResClass_t future behavior of content
 *		RESOBJ_CONTENT_SOFTWARE	  int	    number of sectors required
 *						    to hold installing software
 *		RESOBJ_CONTENT_EXTRA	  int	    number of reserved (extra)
 *						    sectors
 *		RESOBJ_CONTENT_SERVICES	  int	    number of sectors for
 *						    client root/swap services
 *		RESOBJ_DEV_PREFDISK	  char *    preferred disk for storage
 *		RESOBJ_DEV_PREFDEVICE	  int	    preferred slice for storage
 *		RESOBJ_DEV_EXPLDISK	  char *    required disk for storage
 *		RESOBJ_DEV_EXPLDEVICE	  int	    required device for storage
 *		RESOBJ_DEV_EXPLSTART	  int	    required starting cylinder
 *		RESOBJ_DEV_EXPLSIZE	  int	    required sectors for storage
 *		RESOBJ_DEV_EXPLMINIMUM	  int	    required minimum sectors
 *						    for storage
 *		RESOBJ_FS_ACTION	  FsAct_t   file system processing
 *						    directive for directory
 *						    resources with independent
 *						    storage
 *		RESOBJ_FS_MINFREE	  int	    explicit system percent
 *						    free space (VAL_UNSPECIFIED
 *						    for undefined)
 *		RESOBJ_FS_PERCENTFREE	  int	    explicit user percent free
 *						    space (VAL_UNSPECIFIED for
 *						    undefined)
 *		RESOBJ_FS_MOUNTOPTS	  char *    mount options for directory
 *						    resources with independent
 *						    storage
 *
 * Scope:	public
 * Parameters:	res	 [RO] (ResobjHandle)
 *			 Resource handle.
 *		...	 Null terminated list of keyword/return pairs.
 * Return:	D_OK	 retrieval successful
 *		D_BADARG retrieval failed
 */
int
ResobjGetAttribute(ResobjHandle res, ...)
{
	va_list	 ap;
	int	 status;

	/* validate parameters */
	if (res == NULL || ResobjIsIgnored((Resobj *)res) ||
			!ResobjIsExisting(res))
		return (D_BADARG);

	va_start(ap, res);
	status = coreResobjGetAttribute((Resobj *)res, NOPRIVILEGE, ap);
	va_end(ap);

	return (status);
}

/*
 * Function:	ResobjCreate
 * Description:	Add a new resource object to the resource list. The
 *		caller must specify both the name, and an instance number
 *		such that the name/instance number pair is unique among
 *		all resources.
 * Scope:	public
 * Parameters:	type	  [RO] (ResType_t)
 *			  Resource type specifier. Allowable values are:
 *			    RESTYPE_DIRECTORY
 *			    RESTYPE_SWAP
 *		name	  [RO, *RO] (char *)
 *			  Resource name. Valid values are:
 *			    (1) file system path name	RESTYPE_DIRECTORY
 *			    (2) "swap"			RESTYPE_SWAP
 *			    (3) ""			RESTYPE_UNNAMED
 *		instance  [RO] (int)
 *			  Instance specifier (# >= 0)
 *		...	  Null terminated list of attribute/value pairs
 *			  for resource attributes to use for initialization
 * Return:	NULL		add failed
 *		ResobjHandle	add succeeded; handle for new resource
 */
ResobjHandle
ResobjCreate(ResType_t type, char *name, int instance, ...)
{
	Resobj *	res = NULL;
	va_list		ap;

	/* validate parameters */
	if (name == NULL || !ResobjIsValidName(name) ||
			!ResobjIsValidInstance(instance))
		return (NULL);

	/* look for an exact resource match among all resources */
	if (ResobjFindPriv(name, instance) != NULL)
		return (NULL);

	/*
	 * look for a name match (ignoring instance) for which
	 * the resource status is ignored
	 */
	if ((res = (Resobj *)ResobjFindPriv(name, VAL_UNSPECIFIED)) != NULL &&
				ResobjIsIgnored(res))
		return (NULL);

	va_start(ap, instance);
	res = coreResobjCreate(type, name, instance, NOPRIVILEGE, ap);
	va_end(ap);

	return ((ResobjHandle)res);
}

/*
 * Function:	ResobjDestroy
 * Description:	Delete the specified resource. You can only delete resources
 *		which are RESORIGIN_APPEXT in origin.
 * Scope:	public
 * Parameters:	handle	[RO] (ResobjHandle)
 *			Valid resource handle associated with resource to
 *			destroy. The resource must exist in the resource
 *			list.
 * Return:	D_OK	    deletion successful
 *		D_BADARG    deletion failed
 */
int
ResobjDestroy(ResobjHandle handle)
{
	int	 status;
	Resobj * res;

	res = (Resobj *)handle;

	/* validate parameters */
	if (res == NULL || !ResobjIsExisting(res))
		return (D_BADARG);

	/* make sure the caller is trying to delete an application resource */
	if (Resobj_Origin(res) != RESORIGIN_APPEXT)
		return (D_BADARG);

	status = coreResobjDestroy(res, NOPRIVILEGE);
	return (status);
}

/*
 * Function:	ResobjFind
 * Description: Search through the resource list and find the resource
 *		the specified name and instance. Resource of type
 *		RESSTAT_IGNORED are "invisible" to this routine.
 * Scope:	public
 * Parameters:	name	 [RO, *RO] (char *)
 *			 Resource name.
 *		instance [RO] (int)
 *			 Resource instance (VAL_UNSPECIFIED for first
 *			 match with 'name' only).
 * Return:	NULL		Invalid argument or object does not exist.
 *		ResobjHandle	Resource handle.
 */
ResobjHandle
ResobjFind(char *name, int instance)
{
	Resobj *	res;

	/* validate parameters */
	if (!ResobjIsValidName(name) || !ResobjIsValidInstance(instance))
		return (NULL);

	/* walk the resource list looking for the name resource */
	WALK_RESOURCE_LIST(res, RESTYPE_UNDEFINED) {
		if (streq(Resobj_Name(res), name)) {
			if (instance == VAL_UNSPECIFIED ||
					instance == Resobj_Instance(res))
				break;
		}
	}

	return ((ResobjHandle)res);
}

/*
 * Function:	ResobjFirst
 * Description: Return a handle for the first non-RESSTAT_IGNORED
 *		resource of the specified type.
 * Scope:	public
 * Parameters:	type	[RO] (ResType_t)
 *			Optional resource type to constrain search.
 *			RESTYPE_UNDEFINED if no constraint.
 * Return:	NULL	Invalid argument or no more resource objects
 *		ResobjHandle 	Handle for first resource.
 */
ResobjHandle
ResobjFirst(ResType_t type)
{
	Resobj *    res;

	/*
	 * NOTE: you cannot use the WALK_RESOURCE_LIST() macro here
	 *	 because that macro uses this function
	 */
	WALK_LIST(res, _ResourceList) {
		if (ResobjIsIgnored(res))
			continue;

		if (type == RESTYPE_UNDEFINED || type == Resobj_Type(res))
			break;
	}

	return ((ResobjHandle)res);
}

/*
 * Function:	ResobjNext
 * Description: Return a handle to the next resource in the resource list
 *		of the type specified.
 * Scope:	public
 * Parameters:	res	[RO] (ResobjHandle)
 *			Valid resource handle.
 * 		type	[RO] (ResType_t)
 *			Resource type to constrain search.
 *			RESTYPE_UNDEFINED if no constraint.
 * Return:	NULL		Invalid argument or no more resource objects.
 *		ResobjHandle 	Handle for next resource.
 */
ResobjHandle
ResobjNext(ResobjHandle res, ResType_t type)
{
	Resobj *	next = NULL;

	/* validate parameters */
	if (res == NULL)
		return (NULL);

	WALK_LIST(next, Resobj_Next((Resobj *)res)) {
		if (ResobjIsIgnored(next))
			continue;

		if (type == RESTYPE_UNDEFINED || type == Resobj_Type(next))
			break;
	}

	return ((ResobjHandle)next);
}

/*
 * Function:	ResobjIsComplete
 * Description: Check each independent resource to ensure there is
 *		sufficient storage space configured on the disks to meet
 *		either the minimum or default storage requirements for the
 *		resource. Return a pointer to local array containing all
 *		resources which had inadequate space allocated.
 * Scope:	public
 * Parameters:	flag	[RO] (ResSize_t)
 *			Sizing flag specifier. Valid values are:
 *			  RESSIZE_DEFAULT  default sizing
 *			  RESSIZE_MINIMUM  minimum sizing
 * Return:	NULL	all resources have sufficient space
 *		Space*	array of space structures containing required and
 *			allocated information for resources which have
 *			inadequate storage configuration
 */
Space *
ResobjIsComplete(ResSize_t flag)
{
	static Space    _SpaceError[200];
	SliceKey *	info;
	ResobjHandle	res;
	int		count = 0;
	int		size;
	int		allocsector;
	int		esector;

	/* validate parameters */
	if (flag != RESSIZE_DEFAULT && flag != RESSIZE_MINIMUM)
		return (NULL);

	/*
	 * check all the specific resource to make sure their storage
	 * needs are met
	 */
	WALK_RESOURCE_LIST(res, RESTYPE_UNDEFINED) {
		/* check only independent resources */
		if (!ResobjIsIndependent(res))
			continue;

		/* get the minimum storage requirements for the resource */
		size = ResobjGetStorage(res, ADOPT_ALL, flag);

		/*
		 * find the amount of storage allocated for the resource;
		 */
		if ((info = SliceobjFindUse(CFG_CURRENT, NULL,
				Resobj_Name(res), Resobj_Instance(res),
				TRUE)) != NULL) {
			allocsector = Sliceobj_Size(CFG_CURRENT,
					info->dp, info->slice);
		} else
			allocsector = 0;

		/*
		 * see if the amount of allocated space exceeds the minimum
		 * requirements
		 */
		if (size > allocsector) {
			(void) strcpy(_SpaceError[count].name,
					Resobj_Name(res));
			_SpaceError[count].instance = Resobj_Instance(res);
			_SpaceError[count].required = size;
			_SpaceError[count].allocated = allocsector;
			count++;
		}
	}

	/*
	 * check global swap to see if sufficient space has been allocated
	 */
	if (GlobalGetAttribute(GLOBALOBJ_SWAP, &esector) == D_OK) {
		if (esector != VAL_UNSPECIFIED)
			size = esector;
		else
			size = ResobjGetContent(SWAP, ADOPT_NONE, flag);

		/* add up all swap slices */
		allocsector = SliceobjSumSwap(NULL, SWAPALLOC_ALL);

		/* add in all swap files */
		WALK_SWAP_LIST(res) {
			if (NameIsPath(Resobj_Name(res)))
				allocsector += ResobjGetStorage(res, ADOPT_ALL,
					flag);
		}

		/*
		 * report the lack of swap space against the free swap hog;
		 * NOTE:  this will catch /.cache/swapfile even though it's
		 *	  a dependent resource (which is a good thing)
		 */
		if (size > allocsector) {
			WALK_SWAP_LIST(res) {
				if (Resobj_Dev_Explsize(res) == VAL_FREE)
					break;
			}

			if (res != NULL) {
				(void) strcpy(_SpaceError[count].name,
						Resobj_Name(res));
			} else {
				/* this "should" never occur, but could */
				(void) strcpy(_SpaceError[count].name, SWAP);
			}

			_SpaceError[count].instance = Resobj_Instance(res);
			_SpaceError[count].required = size;
			_SpaceError[count].allocated = allocsector;
			count++;
		}
	}

	/*
	 * NULL terminated the last space pointer and if there were
	 * any errors, return a pointer to the head of the array
	 */
	_SpaceError[count].name[0] = '\0';
	if (count == 0)
		return (NULL);

	return ((Space *)_SpaceError);
}

/*
 * Function:	ResobjUpdateContent
 * Description: Update the content fields for non-ignored, default resources.
 *		This call should be made after all machine type changes, or
 *		software configuration changes.
 * Scope:	public
 * Parameters:	none
 * Return:	D_OK	    update completed successfully
 *		D_BADARG    invalid argument specified
 *		D_FAILED    internal error
 */
int
ResobjUpdateContent(void)
{
	char **		dlist;
	ResobjHandle	res;
	FSspace **	sp;
	int		i;
	int		status = D_OK;
	int		size;

	/*
	 * reset the software size structures for directory resources only (not
	 * ignored resources)
	 */
	i = 0;
	WALK_DIRECTORY_LIST(res) {
		i++;
		if ((status = ResobjSetAttributePriv(res,
				RESOBJ_CONTENT_SOFTWARE, 0,
				NULL)) != D_OK)
			return (status);
	}

	/* dynamically allocate a directory list at least big enough */
	if ((dlist = (char **) xcalloc(sizeof (char *) * (i + 1))) == NULL)
		return (D_FAILED);

	/*
	 * calculate the space requirements required by the software selection
	 * using the space_meter() routine. Only RESTYPE_DIRECTORY resources
	 * which are not RESSTAT_IGNORED and have RESORIGIN_DEFAULT as their
	 * origin are updated
	 */
	i = 0;
	WALK_DIRECTORY_LIST(res) {
		dlist[i++] = xstrdup(Resobj_Name(res));
	}

	dlist[i] = NULL;

	/*
	 * do not return directly from this point without freeing up
	 * the space meter array
	 */
	sp = space_meter(dlist);

	for (i = 0; sp && sp[i]; i++) {
		if ((res = ResobjFind(sp[i]->fsp_mntpnt, 0)) != NULL) {
			size = kb_to_sectors(sp[i]->fsp_reqd_contents_space);
			if ((status = ResobjSetAttributePriv(res,
					RESOBJ_CONTENT_SOFTWARE,  size,
					NULL)) != D_OK)
				break;
		}
	}

	/*
	 * update the cache content size for /.cache to represent the
	 * minimum value only if /.cache is a valid directory name for
	 * the current machine type.
	 * NOTE:	if the user had updated this value explicitly
	 *		then we may be destroying user data at this
	 *		point
	 */
	if (status == D_OK) {
		res = ResobjFindPriv(CACHE, 0);
		if (get_machinetype() == MT_CCLIENT)
			size = mb_to_sectors(24);
		else
			size = 0;

		if (res != NULL) {
			status = ResobjSetAttributePriv(res,
				RESOBJ_DEV_EXPLMINIMUM,	 size,
				NULL);
		}
	}

	/* free of malloc'd space from space_meter() call */
	for (i = 0; dlist[i] != NULL; i++)
		free(dlist[i]);

	free (dlist);
	return (status);
}

/*
 * Function:	ResobjPrintList
 * Description:	Print out the data contents associated with the entire
 *		resource list. Use for debugging only.
 * Scope:	public
 * Parameters:	none
 * Return:	none
 */
void
ResobjPrintList(void)
{
	ResobjHandle	res;

	WALK_RESOURCE_LIST_PRIV(res, RESTYPE_UNDEFINED)
		ResobjPrint(res, NULL, VAL_UNSPECIFIED);
}

/*
 * Function:	ResobjPrint
 * Description:	Print out the data content associated with a specific
 *		resource object. Used for debugging only.
 * Scope:	internal
 * Parameters:	handle	[RO] (ResobjHandle)
 *			Resource handle to use when printing. NULL if
 *			specified with name/instance pair.
 *		name	[RO, *RO] (char *)
 *			Resource name. NULL if resource is specified with
 *			handle.
 *		instance [RO] (int)
 *			Resource instance. -1 if resource is specified with
 *			handle. # >= 0 if instance is specified with 'name'.
 * Return:	none
 */
void
ResobjPrint(ResobjHandle handle, char *name, int instance)
{
	Resobj *    res;

	if (handle == NULL) {
		if ((res = ResobjFind(name, instance)) == NULL)
			return;
	} else
		res = (Resobj *)handle;

	write_status(SCR, LEVEL0,
		"Resource: %s (%d)",
		Resobj_Name(res),
		Resobj_Instance(res));

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		DEBUG_ENTRY_FORMAT,
		"type",
		Resobj_Type(res) == RESTYPE_DIRECTORY ? "directory" :
		Resobj_Type(res) == RESTYPE_SWAP ? "swap" :
		Resobj_Type(res) == RESTYPE_UNNAMED ? "unnamed" :
		"unknown");

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		DEBUG_ENTRY_FORMAT,
		"origin",
		Resobj_Origin(res) == RESORIGIN_DEFAULT ? "default" :
		Resobj_Origin(res) == RESORIGIN_APPEXT ? "appextension" :
		"unknown");

	write_status(SCR, LEVEL1|LISTITEM,
		"status");

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		DEBUG_ENTRY_FORMAT,
		"current",
		Resobj_Status(res) == RESSTAT_INDEPENDENT ?
			"independent" :
		Resobj_Status(res) == RESSTAT_DEPENDENT ?
			"dependent" :
		Resobj_Status(res) == RESSTAT_IGNORED ?
			"ignored" :
		Resobj_Status(res) == RESSTAT_OPTIONAL ?
			"optional" : "undefined");

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		DEBUG_ENTRY_FORMAT,
		"standalone",
		Resobj_Standalone_Status(res) == RESSTAT_INDEPENDENT ?
			"independent" :
		Resobj_Standalone_Status(res) == RESSTAT_DEPENDENT ?
			"dependent" :
		Resobj_Standalone_Status(res) == RESSTAT_IGNORED ?
			"ignored" :
		Resobj_Standalone_Status(res) == RESSTAT_OPTIONAL ?
			"optional" : "undefined");

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		DEBUG_ENTRY_FORMAT,
		"server",
		Resobj_Server_Status(res) == RESSTAT_INDEPENDENT ?
			"independent" :
		Resobj_Server_Status(res) == RESSTAT_DEPENDENT ?
			"dependent" :
		Resobj_Server_Status(res) == RESSTAT_IGNORED ?
			"ignored" :
		Resobj_Server_Status(res) == RESSTAT_OPTIONAL ?
			"optional" : "undefined");

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		DEBUG_ENTRY_FORMAT,
		"autoclient",
		Resobj_Autoclient_Status(res) == RESSTAT_INDEPENDENT ?
			"independent" :
		Resobj_Autoclient_Status(res) == RESSTAT_DEPENDENT ?
			"dependent" :
		Resobj_Autoclient_Status(res) == RESSTAT_IGNORED ?
			"ignored" :
		Resobj_Autoclient_Status(res) == RESSTAT_OPTIONAL ?
			"optional" : "undefined");

	write_status(SCR, LEVEL1|LISTITEM,
		"Modify");

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		DEBUG_ENTRY_FORMAT,
		"standalone",
		Resobj_Standalone_Modify(res) == RESMOD_SYS ? "system" :
		Resobj_Standalone_Modify(res) == RESMOD_ANY ? "any" :
		"undefined");

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		DEBUG_ENTRY_FORMAT,
		"server",
		Resobj_Server_Modify(res) == RESMOD_SYS ? "system" :
		Resobj_Server_Modify(res) == RESMOD_ANY ? "any" :
		"undefined");

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		DEBUG_ENTRY_FORMAT,
		"autoclient",
		Resobj_Autoclient_Modify(res) == RESMOD_SYS ? "system" :
		Resobj_Autoclient_Modify(res) == RESMOD_ANY ? "any" :
		"undefined");

	write_status(SCR, LEVEL1|LISTITEM,
	    "Content");

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
	    DEBUG_ENTRY_FORMAT,
	    "content",
	    Resobj_Content_Class(res) == RESCLASS_STATIC ? "static" :
	    Resobj_Content_Class(res) == RESCLASS_DYNAMIC ? "dynamic" :
	    Resobj_Content_Class(res) == RESCLASS_REPOSITORY ? "repository" :
	    "undefined");

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
	    DEBUG_ENTRY_SIZE_FORMAT,
	    "software",
	    Resobj_Content_Software(res));

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
	    DEBUG_ENTRY_SIZE_FORMAT,
	    "extra",
	    Resobj_Content_Extra(res));

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
	    DEBUG_ENTRY_SIZE_FORMAT,
	    "services",
	    Resobj_Content_Services(res));

	write_status(SCR, LEVEL1|LISTITEM,
	    "Device Configuration Constraints");

	if (Resobj_Dev_Explstart(res) == VAL_UNSPECIFIED) {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_FORMAT,
			"explicit start",
			"unspecified");
	} else {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_NUM_FORMAT,
			"explicit start",
			Resobj_Dev_Explstart(res));
	}

	if (Resobj_Dev_Explsize(res) == VAL_UNSPECIFIED) {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_FORMAT,
			"explicit size",
			"unspecified");
	} else if (Resobj_Dev_Explsize(res) == VAL_FREE) {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_FORMAT,
			"explicit size",
			"free space hog");
	} else {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_SIZE_FORMAT,
			"explicit size",
			Resobj_Dev_Explsize(res));
	}

	if (Resobj_Dev_Explmin(res) == VAL_UNSPECIFIED) {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_FORMAT,
			"explicit minimum",
			"unspecified");
	} else {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_SIZE_FORMAT,
			"explicit minimum",
			Resobj_Dev_Explmin(res));
	}

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
	    DEBUG_ENTRY_FORMAT,
	    "explicit disk",
	    Resobj_Dev_Expldisk(res));

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
	    DEBUG_ENTRY_FORMAT,
	    "preferred disk",
	    Resobj_Dev_Prefdisk(res));

	if (Resobj_Dev_Dfltdevice(res) == VAL_UNSPECIFIED) {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_FORMAT,
			"default device",
			"unspecified");
	} else {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_NUM_FORMAT,
			"default device",
			Resobj_Dev_Dfltdevice(res));
	}

	if (Resobj_Dev_Expldevice(res) == VAL_UNSPECIFIED) {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_FORMAT,
			"explicit device",
			"unspecified");
	} else {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_NUM_FORMAT,
			"explicit device",
			Resobj_Dev_Expldevice(res));
	}

	if (Resobj_Dev_Prefdevice(res) == VAL_UNSPECIFIED) {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_FORMAT,
			"preferred device",
			"unspecified");
	} else {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_NUM_FORMAT,
			"preferred device",
			Resobj_Dev_Prefdevice(res));
	}

	write_status(SCR, LEVEL1|LISTITEM,
	    "File System");

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
	    DEBUG_ENTRY_FORMAT,
	    "action",
	    Resobj_Fs_Action(res) == FSACT_CREATE ? "create" :
	    Resobj_Fs_Action(res) == FSACT_CHECK ? "check" :
	    "undefined");

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
	    DEBUG_ENTRY_FORMAT,
	    "mount options",
	    Resobj_Fs_Mountopts(res) == NULL ? "" : Resobj_Fs_Mountopts(res));

	if (Resobj_Fs_Minfree(res) == VAL_UNSPECIFIED) {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_FORMAT,
			"minfree",
			"unspecified");
	} else {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_NUM_FORMAT,
			"minfree",
			Resobj_Fs_Minfree(res));
	}

	if (Resobj_Fs_Percentfree(res) == VAL_UNSPECIFIED) {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_FORMAT,
			"percent free",
			"unspecified");
	} else {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			DEBUG_ENTRY_NUM_FORMAT,
			"percent free",
			Resobj_Fs_Percentfree(res));
	}
}

/*
 * Function:	ResobjReinitList
 * Description: If the resource list is already initialized, delete all
 *		resources. Call the resource initialization routine.
 * Scope:	public
 * Parameters:	none
 * Return:	D_OK	 resource lists constructed successfully
 *		D_FAILED resource list construction failed
 */
int
ResobjReinitList(void)
{
	Resobj *	res;
	Resobj *	next;

	for (res = _ResourceList; res != NULL; res = next) {
		next = Resobj_Next(res);
		/*
		 * must destroy with privilege in order to eliminate
		 * default origin resources
		 */
		if (ResobjDestroyPriv((ResobjHandle)res) != D_OK)
			return (D_FAILED);
	}

	return (ResobjInitList());
}

/*
 * Function:	ResobjInitList
 * Description: Initialize the resource list with default resources.
 *		This should be called only once during the execution of an
 *		application. Set all global attributes which are affected
 *		by environment variables:
 *			SYS_SWAPSIZE	- total swap size in MB
 * Scope:	public
 * Parameters:	none
 * Return:	D_OK	 resource lists constructed successfully
 *		D_FAILED resource list construction failed
 */
int
ResobjInitList(void)
{
	char *		swapenv;
	ResDefault *	dflt;
	int		i;
	ResobjHandle	res;

	if (get_trace_level() > 4) {
		write_status(SCR, LEVEL0, "Initializing resource list");
	}

	/*
	 * if the resource list is already initialized with default resources,
	 * don't do it again
	 */
	WALK_RESOURCE_LIST_PRIV(res, RESTYPE_UNDEFINED) {
		if (Resobj_Origin(res) == RESORIGIN_DEFAULT)
			return (D_FAILED);
	}

	/*
	 * initialize the resource list from the default resource table
	 */
	for (i = 0, dflt = &_ResourceDefault[0];
				dflt->type != RESTYPE_UNDEFINED;
				dflt = &_ResourceDefault[++i]) {
		if (ResobjCreatePriv(dflt->type, dflt->name, 0,
				RESOBJ_ORIGIN, RESORIGIN_DEFAULT,
				RESOBJ_MODIFY_STANDALONE,
					dflt->state.standalone.modify,
				RESOBJ_MODIFY_SERVER,
					dflt->state.server.modify,
				RESOBJ_MODIFY_AUTOCLIENT,
					dflt->state.autoclient.modify,
				RESOBJ_STATUS_STANDALONE,
					dflt->state.standalone.status,
				RESOBJ_STATUS_SERVER,
					dflt->state.server.status,
				RESOBJ_STATUS_AUTOCLIENT,
					dflt->state.autoclient.status,
				RESOBJ_DEV_DFLTDEVICE,
					dflt->default_slice,
				RESOBJ_CONTENT_CLASS,
					dflt->class,
				NULL) == NULL)
			return (D_FAILED);
	}

	/*
	 * set the swap free space hog  - since both are never active together,
	 * this is safe
	 */
	if ((res = ResobjFindPriv(CACHESWAP, 0)) != NULL) {
		(void) ResobjSetAttributePriv(res,
			RESOBJ_DEV_EXPLSIZE,  VAL_FREE,
			NULL);
	}
	if ((res = ResobjFindPriv(SWAP, 0)) != NULL) {
		(void) ResobjSetAttributePriv(res,
			RESOBJ_DEV_EXPLSIZE,  VAL_FREE,
			NULL);
	}

	/*
	 * set the initial explicit swap resource size if it was
	 * specified by an environment variable which is all numbers;
	 * the variable is set as MB (convert to sectors)
	 */
	if ((swapenv = getenv("SYS_SWAPSIZE")) != NULL) {
		if (is_allnums(swapenv)) {
			if (GlobalSetAttributePriv(
				GLOBALOBJ_SWAP,
					(void *)mb_to_sectors(
						atoi(swapenv))) != D_OK)
				return (D_FAILED);
		}
	}

	/*
	 * update the content fields
	 */
	if (ResobjUpdateContent() != D_OK)
		return (D_FAILED);

	return (D_OK);
}

/*
 * Function:	ResobjIsExisting
 * Description:	Boolean function to determine if a given handle represents a
 *		resource which is defined in the resource list.
 * Scope:	public
 * Parameters:	res	[RO] (ResobjHandle)
 *			Resource handle.
 * Return:	1	Resource is valid and exists
 *		0	Resource does not exist in resource list.
 */
int
ResobjIsExisting(ResobjHandle res)
{
	ResobjHandle	rp;

	/* validate parameters */
	if (res == NULL)
		return (0);

	WALK_RESOURCE_LIST(rp, RESTYPE_UNDEFINED) {
		if (rp == res)
			return (1);
	}

	return (0);
}

/* ---------------------- internal functions -------------------------- */

/*
 * Function:	ResobjFirstPriv
 * Description: Return a handle for the first object of the specified
 *		resource type (including ignored resources)
 * Scope:	internal
 * Parameters:	type	[RO] (ResType_t)
 *			Optional resource type to constrain search.
 *			RESTYPE_UNDEFINED if no constraint.
 * Return:	NULL		invalid argument or no more resource objects
 *		ResobjHandle 	handle for first resource
 */
ResobjHandle
ResobjFirstPriv(ResType_t type)
{
	Resobj *	first;

	/*
	 * NOTE: you cannot use the WALK_RESOURCE_LIST_PRIV() macro here
	 *	 because that macro uses this function
	 */
	WALK_LIST(first, _ResourceList) {
		if (type == RESTYPE_UNDEFINED || type == Resobj_Type(first))
			break;
	}

	return ((ResobjHandle)first);
}

/*
 * Function:	ResobjNextPriv
 * Description: Privileged routine to return a handle to the next resource of
 *		the same type as the one specified (including ignored
 *		resources).
 * Scope:	internal
 * Parameters:	res	[RO, *RO] (ResobjHandle)
 *			Resource handle.
 * 		type	[RO] (ResType_t)
 *			Optional resource type to constrain search.
 *			RESTYPE_UNDEFINED if no constraint.
 * Return:	NULL	Invalid argument or no more resource objects.
 *		ResobjHandle	Handle for next resource.
 */
ResobjHandle
ResobjNextPriv(ResobjHandle res, ResType_t type)
{
	Resobj *	next = NULL;

	/* validate parameters */
	if (res == NULL || !ResobjIsExistingPriv(res))
		return (NULL);

	/*
	 * NOTE: you cannot use the WALK_RESOURCE_LIST_PRIV() macro here
	 *	 because that macro uses this function
	 */
	WALK_LIST(next, Resobj_Next((Resobj *)res)) {
		if (type == RESTYPE_UNDEFINED || type == Resobj_Type(next))
			break;
	}

	return ((ResobjHandle)next);
}

/*
 * Function:	ResobjDestroyPriv
 * Description: Privileged routine to delete a specified resource.
 * Scope:	internal
 * Parameters:	res	 [RO, *RO] (ResobjHandle)
 *			 Resource handle.
 * Return:	D_OK	 deletion successful
 *		D_BADARG deletion failed
 */
int
ResobjDestroyPriv(ResobjHandle res)
{
	int		status;

	/* validate parameters */
	if (res == NULL || !ResobjIsExistingPriv(res))
		return (D_BADARG);

	status = coreResobjDestroy((Resobj *)res, PRIVILEGE);

	return (status);
}

/*
 * Function:	ResobjGetAttributePriv
 * Description: Privileged resource attribute retrieval interface.
 * Scope:	internal
 * Parameters:	res	[RO, *RO] (ResobjHandle)
 *			pointer to existing resource object
 *		...	keyword/return address pair list, NULL
 *			terminated
 * Return:	D_OK	 retrieval successful
 *		D_BADARG retrieval failed
 */
int
ResobjGetAttributePriv(ResobjHandle res, ...)
{
	va_list		ap;
	int		status;

	/* validate parameters */
	if (res == NULL)
		return (D_BADARG);

	va_start(ap, res);
	status = coreResobjGetAttribute((Resobj *)res, 1, ap);
	va_end(ap);

	return (status);
}

/*
 * Function:	ResobjFindPriv
 * Description: Privileged routine to search the resource list and find the
 *		resource of the specified name and instance. Ignored resources
 *		are visible to this routine.
 * Scope:	internal
 * Parameters:	name	 [RO, *RO] (char *)
 *			 Resource name.
 *		instance [RO] (int)
 *			 Valid resource instance (# >= 0).
 * Return:	NULL		invalid argument or object does not exist
 *		ResobjHandle	pointer to resource object
 */
ResobjHandle
ResobjFindPriv(char *name, int instance)
{
	Resobj *    res;

	/* validate parameters */
	if (!ResobjIsValidName(name) || !ResobjIsValidInstance(instance))
		return (NULL);

	/*
	 * walk through the resource list looking for a resource with
	 * the caller specified name; if the caller did not specify
	 * an explicit instance, take the first match on the name
	 */
	WALK_RESOURCE_LIST_PRIV(res, RESTYPE_UNDEFINED) {
		if (streq(Resobj_Name(res), name)) {
			if (instance == VAL_UNSPECIFIED ||
					instance == Resobj_Instance(res))
				break;
		}
	}

	return ((ResobjHandle)res);
}

/*
 * Function:	ResobjSetAttributePriv
 * Description:	Privileged routine to modify resource object attributes.
 * Scope:	internal
 * Parameters:	res	[RO, *RO] (ResobjHandle)
 *			Handle for pointer to existing resource object
 *		...	attribute value pairs for modification attributes,
 *			NULL termianted.
 * Return:	D_OK	 attributes set successful
 *		D_BADARG attribute set failed
 */
int
ResobjSetAttributePriv(ResobjHandle res, ...)
{
	va_list		ap;
	int		status;

	/* validate parameters */
	if (res == NULL)
		return (D_BADARG);

	va_start(ap, res);
	status = coreResobjSetAttribute((Resobj *)res, PRIVILEGE, ap);
	va_end(ap);

	return (status);
}

/*
 * Function:	ResobjCreatePriv
 * Description: Add a resource object to the resource list for the
 *		specified state.
 * Scope:	internal
 * Parameters:	type	[RO] (ResType_t)
 *			Resource type to be set during creation.
 *		name	[RO, *RO] (char *)
 *			Name of resource object.
 *
 *		...	keyword/attribute pairs to initialize the
 *			resource object
 * Return:	NULL	 invalid parameter
 *		Resobj * pointer to new directory resource object
 */
ResobjHandle
ResobjCreatePriv(ResType_t type, char *name, int instance, ...)
{
	Resobj *    res = NULL;
	va_list	    ap;

	/* validate parameters */
	if (!ResobjIsValidName(name) || !ResobjIsValidInstance(instance))
		return (NULL);

	/* look for an exact resource match */
	if (ResobjFindPriv(name, instance) != NULL)
		return (NULL);

	/*
	 * look for a name match (ignoring instance) for which
	 * the resource status is ignored
	 */
	if ((res = (Resobj *)ResobjFindPriv(name, VAL_UNSPECIFIED)) != NULL &&
				ResobjIsIgnored(res))
		return (NULL);

	va_start(ap, instance);
	res = coreResobjCreate(type, name, instance, PRIVILEGE, ap);
	va_end(ap);

	return ((ResobjHandle)res);
}

/* ----------------------- private functions -------------------------- */

/*
 * Function:	coreResobjSetAttribute
 * Description: Low level routine to modify the attributes associated with
 *		a resource. Fields which are restricted for privileged
 *		modification only are only flagged as an error if their
 *		requested attribute differs from the existing attribute and
 *		the caller does not have privilege.
 *
 *		NOTE:	The resource type must be set before using this call.
 * Scope:	private
 * Parameters:	res	  [RO, *RW] (Resobj *)
 *			  Pointer to existing resource object
 *		privilege [RO] (int)
 *			  Privilege flag to modify restricted attributes.
 *			  Valid values are:
 *				0 - no privilege
 *				1 - privilege
 *		va_list   [RO] (va_list)
 *			  keyword/attribute varargs list
 * Return:	D_OK	  modification successful
 *		D_BADARG  invalid argument
 *		D_FAILED  modification failed
 */
static int
coreResobjSetAttribute(Resobj *res, int privilege, va_list ap)
{
	ResobjAttr_t	keyword;
	ResobjHandle	tmp;
	Resobj *	nres;
	int		status = D_OK;
	char *		cp;
	int		ival;
	ResStat_t	rstat;
	ResMod_t	rmod;
	ResOrigin_t	rorig;
	ResClass_t	rclass;
	Resobj *	next;
	FsAction_t	fsact;

	/* validate parameters */
	if (res == NULL)
		return (D_BADARG);

	/* initialize a temporary resource structure for data loading */
	if ((nres = ResobjCreateDup(res)) == NULL)
		return (D_FAILED);

	while ((keyword = va_arg(ap, ResobjAttr_t)) != NULL &&
				status == D_OK) {
		switch (keyword) {
		    case RESOBJ_NAME:			/* RESTRICTIONS */
			cp = va_arg(ap, char *);
			if (cp <= (char *)0)
				status = D_BADARG;
			else if (cp == NULL || *cp == '\0')
				status = D_BADARG;
			else if (privilege == NOPRIVILEGE &&
				    Resobj_Origin(res) == RESORIGIN_DEFAULT)
				status = D_BADARG;
			else if (ResobjIsDirectory(res) && !NameIsPath(cp))
				status = D_BADARG;
			else if (ResobjIsSwap(res) && !NameIsSwap(cp) &&
					!NameIsPath(cp))
				status = D_BADARG;
			else if (ResobjIsUnnamed(res) && *cp != '\0')
				status = D_BADARG;
			else
				(void) strcpy(Resobj_Name(nres), cp);
			break;

		    case RESOBJ_INSTANCE:
			ival = va_arg(ap, int);		/* RESTRICTIONS */
			if (ResobjIsDirectory(res) && ival != 0)
				status = D_BADARG;
			else if (ival < 0 || !ResobjIsValidInstance(ival))
				status = D_BADARG;
			else if (ResobjFind(Resobj_Name(res), ival))
				status = D_BADARG;
			else
				Resobj_Instance(nres) = ival;
			break;

		    case RESOBJ_ORIGIN:			/* RESTRICTIONS */
			rorig = va_arg(ap, ResOrigin_t);
			if (rorig == RESORIGIN_UNDEFINED)
				status = D_BADARG;
			else if (privilege == NOPRIVILEGE &&
					rorig != RESORIGIN_APPEXT)
				status = D_BADARG;
			else
				Resobj_Origin(nres) = rorig;
			break;

		    case RESOBJ_NEXT:			/* RESTRICTED */
			next = va_arg(ap, Resobj *);
			if (privilege == NOPRIVILEGE)
				status = D_BADARG;
			else if (next < (Resobj *)0)
				status = D_BADARG;
			else
				Resobj_Next(nres) = next;
			break;

		    case RESOBJ_STATUS:
			rstat = va_arg(ap, ResStat_t);
			if (rstat == RESSTAT_UNDEFINED)
				status = D_BADARG;
			else if (privilege == NOPRIVILEGE &&
					Resobj_Modify(res) == RESMOD_SYS)
				status = D_BADARG;
			else if (privilege == NOPRIVILEGE &&
					rstat == RESSTAT_IGNORED)
				status = D_BADARG;
			else {
				switch (get_machinetype()) {
				    case MT_STANDALONE:
					Resobj_Standalone_Status(nres) =
						rstat;
					break;
				    case MT_SERVER:
					Resobj_Server_Status(nres) = rstat;
					break;
				    case MT_CCLIENT:
					Resobj_Autoclient_Status(nres) =
						rstat;
					break;
				}
			}
			break;

		    case RESOBJ_STATUS_STANDALONE:	/* RESTRICTED */
			rstat = va_arg(ap, ResStat_t);
			if (privilege == NOPRIVILEGE)
				status = D_BADARG;
			else if (rstat == RESSTAT_UNDEFINED)
				status = D_BADARG;
			else
				Resobj_Standalone_Status(nres) = rstat;
			break;

		    case RESOBJ_STATUS_SERVER:		/* RESTRICTED */
			rstat = va_arg(ap, ResStat_t);
			if (privilege == NOPRIVILEGE)
				status = D_BADARG;
			else if (rstat == RESSTAT_UNDEFINED)
				status = D_BADARG;
			else
				Resobj_Server_Status(nres) = rstat;
			break;

		    case RESOBJ_STATUS_AUTOCLIENT:	/* RESTRICTED */
			rstat = va_arg(ap, ResStat_t);
			if (privilege == NOPRIVILEGE)
				status = D_BADARG;
			else if (rstat == RESSTAT_UNDEFINED)
				status = D_BADARG;
			else
				Resobj_Autoclient_Status(nres) = rstat;
			break;

		    case RESOBJ_MODIFY:
			rmod = va_arg(ap, ResMod_t);
			if (privilege == NOPRIVILEGE)
				status = D_BADARG;
			else if (rmod == RESMOD_UNDEFINED)
				status = D_BADARG;
			else {
				switch (get_machinetype()) {
				    case MT_STANDALONE:
					Resobj_Standalone_Modify(nres) = rmod;
					break;
				    case MT_SERVER:
					Resobj_Server_Modify(nres) = rmod;
					break;
				    case MT_CCLIENT:
					Resobj_Autoclient_Modify(nres) = rmod;
					break;
				}
			}
			break;

		    case RESOBJ_MODIFY_STANDALONE:	/* RESTRICTED */
			rmod = va_arg(ap, ResMod_t);
			if (privilege == NOPRIVILEGE)
				status = D_BADARG;
			else if (rmod == RESMOD_UNDEFINED)
				status = D_BADARG;
			else
				Resobj_Standalone_Modify(nres) = rmod;
			break;

		    case RESOBJ_MODIFY_SERVER:		/* RESTRICTED */
			rmod = va_arg(ap, ResMod_t);
			if (privilege == NOPRIVILEGE)
				status = D_BADARG;
			else if (rmod == RESMOD_UNDEFINED)
				status = D_BADARG;
			else
				Resobj_Server_Modify(nres) = rmod;
			break;

		    case RESOBJ_MODIFY_AUTOCLIENT:	/* RESTRICTED */
			rmod = va_arg(ap, ResMod_t);
			if (privilege == NOPRIVILEGE)
				status = D_BADARG;
			else if (rmod == RESMOD_UNDEFINED)
				status = D_BADARG;
			else
				Resobj_Autoclient_Modify(nres) = rmod;
			break;

		    case RESOBJ_CONTENT_SOFTWARE:	/* RESTRICTED */
			ival = va_arg(ap, int);
			if (privilege == NOPRIVILEGE)
				status = D_BADARG;
			else if (ival < 0)
				status = D_BADARG;
			else if (!ResobjIsDirectory(res) && ival != 0)
				status = D_BADARG;
			else
				Resobj_Content_Software(nres) = ival;
			break;

		    case RESOBJ_CONTENT_CLASS:
			rclass = va_arg(ap, ResClass_t);
			if (rclass == RESCLASS_UNDEFINED)
				status = D_BADARG;
			else
				Resobj_Content_Class(nres) = rclass;
			break;

		    case RESOBJ_CONTENT_EXTRA:
			ival = va_arg(ap, int);
			if (ival < 0)
				status = D_BADARG;
			else
				Resobj_Content_Extra(nres) = ival;
			break;

		    case RESOBJ_CONTENT_SERVICES:
			ival = va_arg(ap, int);
			if (ival < 0)
				status = D_BADARG;
			else
				Resobj_Content_Services(nres) = ival;
			break;

		    case RESOBJ_DEV_DFLTDEVICE:
			ival = va_arg(ap, int);
			if (ival != VAL_UNSPECIFIED && \
					!valid_sdisk_slice(ival))
				status = D_BADARG;
			else
				Resobj_Dev_Dfltdevice(nres) = ival;
			break;

		    case RESOBJ_DEV_EXPLMINIMUM:
			ival = va_arg(ap, int);
			if (ival < 0 && ival != VAL_UNSPECIFIED)
				status = D_BADARG;
			else
				Resobj_Dev_Explmin(nres) = ival;
			break;

		    case RESOBJ_DEV_EXPLSIZE:	/* PARTIALLY RESTRICTED */
			ival = va_arg(ap, int);
			if (ival == VAL_FREE &&
					Resobj_Type(nres) != RESTYPE_SWAP)
				status = D_BADARG;
			else if (privilege == NOPRIVILEGE && ival == VAL_FREE)
				status = D_BADARG;
			else if (privilege == NOPRIVILEGE && ival < 0 &&
					ival != VAL_FREE)
				status = D_BADARG;
			else
				Resobj_Dev_Explsize(nres) = ival;
			break;

		    case RESOBJ_DEV_EXPLSTART:
			ival = va_arg(ap, int);
			if (ival < 0 && ival != VAL_UNSPECIFIED)
				status = D_BADARG;
			else
				Resobj_Dev_Explstart(nres) = ival;
			break;

		    case RESOBJ_DEV_EXPLDISK:
			cp = va_arg(ap, char *);
			if (cp != NULL && !is_disk_name(cp))
				status = D_BADARG;
			else if (cp != NULL)
				(void) strcpy(Resobj_Dev_Expldisk(nres), cp);
			else
				(void) strcpy(Resobj_Dev_Expldisk(nres), "");
			break;

		    case RESOBJ_DEV_EXPLDEVICE:
			ival = va_arg(ap, int);
			if (ival != VAL_UNSPECIFIED && \
					!valid_sdisk_slice(ival))
				status = D_BADARG;
			else
				Resobj_Dev_Expldevice(nres) = ival;
			break;

		    case RESOBJ_DEV_PREFDISK:
			cp = va_arg(ap, char *);
			if (cp != NULL && !is_disk_name(cp))
				status = D_BADARG;
			else if (cp != NULL)
				(void) strcpy(Resobj_Dev_Prefdisk(nres), cp);
			else
				(void) strcpy(Resobj_Dev_Prefdisk(nres), "");
			break;

		    case RESOBJ_DEV_PREFDEVICE:
			ival = va_arg(ap, int);
			if (ival != VAL_UNSPECIFIED && \
					!valid_sdisk_slice(ival))
				status = D_BADARG;
			else
				Resobj_Dev_Prefdevice(nres) = ival;
			break;

		    case RESOBJ_FS_ACTION:		/* RESTRICTED */
			fsact = va_arg(ap, FsAction_t);
			if (privilege == NOPRIVILEGE)
				status = D_BADARG;
			else if (!ResobjIsDirectory(res) &&
					fsact != FSACT_UNDEFINED)
				status = D_BADARG;
			else
				Resobj_Fs_Action(nres) = fsact;
			break;

		    case RESOBJ_FS_MOUNTOPTS:
			cp = va_arg(ap, char *);
			if (!ResobjIsDirectory(res) && cp != NULL)
				status = D_BADARG;
			else if ((int)cp < 0)
				status = D_BADARG;
			else if (cp != NULL) {
				if (Resobj_Fs_Mountopts(nres) != NULL)
					free(Resobj_Fs_Mountopts(nres));
				Resobj_Fs_Mountopts(nres) = xstrdup(cp);
			} else {
				if (Resobj_Fs_Mountopts(nres) != NULL)
					free(Resobj_Fs_Mountopts(nres));
				Resobj_Fs_Mountopts(nres) = NULL;
			}
			break;

		    case RESOBJ_FS_MINFREE:
			ival = va_arg(ap, int);
			if (!ResobjIsDirectory(res) && ival != VAL_UNSPECIFIED)
				status = D_BADARG;
			else if (ival < 0 && ival != VAL_UNSPECIFIED)
				status = D_BADARG;
			else
				Resobj_Fs_Minfree(nres) = ival;
			break;

		    case RESOBJ_FS_PERCENTFREE:
			ival = va_arg(ap, int);
			if (!ResobjIsDirectory(res) && ival != VAL_UNSPECIFIED)
				status = D_BADARG;
			else if (ival < 0 && ival != VAL_UNSPECIFIED)
				status = D_BADARG;
			else
				Resobj_Fs_Percentfree(nres) = ival;
			break;

		    default:
			status = D_BADARG;
			break;
		}
	}

	/*
	 * check for inter-attribute dependency conflicts
	 */
	/* you can't have an explicit minimum greater than an explicit size */
	if (status == D_OK && Resobj_Dev_Explsize(nres) >= 0 &&
			Resobj_Dev_Explmin(nres) >= 0 &&
			Resobj_Dev_Explmin(nres) >
				Resobj_Dev_Explsize(nres)) {
		status = D_BADARG;
	}

	/*
	 * make sure there is only one VAL_FREE swap resource in the real
	 * resource list
	 */
	if (status == D_OK && Resobj_Type(nres) == RESTYPE_SWAP) {
		if (Resobj_Dev_Explsize(nres) == VAL_FREE) {
			WALK_SWAP_LIST(tmp) {
				if (Resobj_Dev_Explsize(tmp) == VAL_FREE &&
						tmp != res) {
					status = D_BADARG;
					break;
				}
			}
		}
	}

	/*
	 * swap files can't be anything but dependent
	 */
	if (status == D_OK && Resobj_Type(nres) == RESTYPE_SWAP) {
		if (privilege == NOPRIVILEGE &&
				NameIsPath(Resobj_Name(nres)) &&
				!ResobjIsDependent(nres))
			status = D_BADARG;
	}

	/* swap resources should not use the content fields */
	if (status == D_OK && Resobj_Type(nres) == RESTYPE_SWAP) {
		if (Resobj_Content_Extra(nres) != 0 ||
				Resobj_Content_Software(nres) != 0 ||
				Resobj_Content_Services(nres) != 0) {
			status = D_BADARG;
		}
	}

	/* update the real resource structure */
	if (status == D_OK) {
		if (!streq(Resobj_Name(res), Resobj_Name(nres)))
			(void) strcpy(Resobj_Name(res), Resobj_Name(nres));

		if (Resobj_Instance(res) != Resobj_Instance(nres))
			Resobj_Instance(res) = Resobj_Instance(nres);

		if (Resobj_Standalone_Status(res) !=
					Resobj_Standalone_Status(nres))
			Resobj_Standalone_Status(res) =
				Resobj_Standalone_Status(nres);

		if (Resobj_Server_Status(res) != Resobj_Server_Status(nres))
			Resobj_Server_Status(res) = Resobj_Server_Status(nres);

		if (Resobj_Autoclient_Status(res) !=
					Resobj_Autoclient_Status(nres))
			Resobj_Autoclient_Status(res) =
				Resobj_Autoclient_Status(nres);

		if (Resobj_Standalone_Modify(res) !=
					Resobj_Standalone_Modify(nres))
			Resobj_Standalone_Modify(res) =
				Resobj_Standalone_Modify(nres);

		if (Resobj_Server_Modify(res) != Resobj_Server_Modify(nres))
			Resobj_Server_Modify(res) = Resobj_Server_Modify(nres);

		if (Resobj_Autoclient_Modify(res) !=
					Resobj_Autoclient_Modify(nres))
			Resobj_Autoclient_Modify(res) =
				Resobj_Autoclient_Modify(nres);

		if (Resobj_Origin(res) != Resobj_Origin(nres))
			Resobj_Origin(res) = Resobj_Origin(nres);

		if (Resobj_Next(res) != Resobj_Next(nres))
			Resobj_Next(res) = Resobj_Next(nres);

		if (Resobj_Dev_Dfltdevice(res) != Resobj_Dev_Dfltdevice(nres))
			Resobj_Dev_Dfltdevice(res) =
				Resobj_Dev_Dfltdevice(nres);

		if (!streq(Resobj_Dev_Prefdisk(res), Resobj_Dev_Prefdisk(nres)))
			(void) strcpy(Resobj_Dev_Prefdisk(res),
					Resobj_Dev_Prefdisk(nres));

		if (Resobj_Dev_Prefdevice(res) != Resobj_Dev_Prefdevice(nres))
			Resobj_Dev_Prefdevice(res) =
				Resobj_Dev_Prefdevice(nres);

		if (Resobj_Dev_Explmin(res) != Resobj_Dev_Explmin(nres))
			Resobj_Dev_Explmin(res) = Resobj_Dev_Explmin(nres);

		if (!streq(Resobj_Dev_Expldisk(res), Resobj_Dev_Expldisk(nres)))
			(void) strcpy(Resobj_Dev_Expldisk(res),
				Resobj_Dev_Expldisk(nres));

		if (Resobj_Dev_Expldevice(res) != Resobj_Dev_Expldevice(nres))
			Resobj_Dev_Expldevice(res) =
				Resobj_Dev_Expldevice(nres);

		if (Resobj_Dev_Explsize(res) != Resobj_Dev_Explsize(nres))
			Resobj_Dev_Explsize(res) = Resobj_Dev_Explsize(nres);

		if (Resobj_Dev_Explstart(res) != Resobj_Dev_Explstart(nres))
			Resobj_Dev_Explstart(res) = Resobj_Dev_Explstart(nres);

		if (Resobj_Fs_Action(res) != Resobj_Fs_Action(nres))
			Resobj_Fs_Action(res) = Resobj_Fs_Action(nres);

		if (Resobj_Fs_Mountopts(res) != NULL) {
			free(Resobj_Fs_Mountopts(res));
			Resobj_Fs_Mountopts(res) = NULL;
		}

		if (Resobj_Fs_Mountopts(nres) != NULL) {
			Resobj_Fs_Mountopts(res) = xstrdup(
				Resobj_Fs_Mountopts(nres));
		}

		if (Resobj_Fs_Minfree(res) != Resobj_Fs_Minfree(nres))
			Resobj_Fs_Minfree(res) = Resobj_Fs_Minfree(nres);

		if (Resobj_Fs_Percentfree(res) != Resobj_Fs_Percentfree(nres))
			Resobj_Fs_Percentfree(res) =
				Resobj_Fs_Percentfree(nres);

		if (Resobj_Content_Class(res) != Resobj_Content_Class(nres))
			Resobj_Content_Class(res) = Resobj_Content_Class(nres);

		if (Resobj_Content_Software(res) !=
					Resobj_Content_Software(nres))
			Resobj_Content_Software(res) =
				Resobj_Content_Software(nres);

		if (Resobj_Content_Extra(res) != Resobj_Content_Extra(nres))
			Resobj_Content_Extra(res) =
				Resobj_Content_Extra(nres);

		if (Resobj_Content_Services(res) !=
					Resobj_Content_Services(nres))
			Resobj_Content_Services(res) =
				Resobj_Content_Services(nres);
	}

	ResobjDeallocate(nres);
	return (status);
}

/*
 * Function:	coreResobjGetAttribute
 * Description:	Low level routine to retrieve resource attributes.
 * Scope:	private
 * Parameters:	res	    pointer to existing resource object
 *		privilege   privilege to modify restricted attributes
 *				(0 - no privilege; 1 - privilege)
 *		ap	    keyword/return varargs list
 * Return:	D_OK	    retrieval successful
 *		D_BADARG    retrieval failed
 */
static int
coreResobjGetAttribute(Resobj *res, int privilege, va_list ap)
{
	ResobjAttr_t	keyword;
	Resobj **	resp;
	FsAction_t *	fsactp;
	int *		valp;
	char *		cp;
	ResStat_t *	statusp;
	ResOrigin_t *	origp;
	ResClass_t *	classp;
	ResMod_t *	modp;

	/* validate parameters */
	if (res == NULL)
		return (D_BADARG);

	while ((keyword = va_arg(ap, ResobjAttr_t)) != NULL) {
		switch (keyword) {
		    case RESOBJ_NAME:
			cp = va_arg(ap, char *);
			if (cp == NULL)
				return (D_BADARG);
			else
				(void) strcpy(cp, Resobj_Name(res));
			break;

		    case RESOBJ_INSTANCE:
			valp = va_arg(ap, int *);
			if (valp == NULL)
				return (D_BADARG);
			else
				*valp = Resobj_Instance(res);
			break;

		    case RESOBJ_ORIGIN:
			origp = va_arg(ap, ResOrigin_t *);
			if (origp == NULL)
				return (D_BADARG);
			(*origp) = Resobj_Origin(res);
			break;

		    case RESOBJ_NEXT:			/* RESTRICTED */
			resp = va_arg(ap, Resobj **);
			if (privilege == NOPRIVILEGE)
				return (D_BADARG);
			else if (resp == NULL || *resp == NULL)
				return (D_BADARG);
			else
				(*resp) = Resobj_Next(res);
			break;

		    case RESOBJ_STATUS:
			statusp = (ResobjHandle)va_arg(ap, ResStat_t *);
			if (statusp == NULL)
				return (D_BADARG);
			else
				(*statusp) = Resobj_Status(res);
			break;

		    case RESOBJ_STATUS_STANDALONE:	/* RESTRICTED */
			statusp = (ResobjHandle)va_arg(ap, ResStat_t *);
			if (statusp == NULL)
				return (D_BADARG);
			else
				(*statusp) = Resobj_Standalone_Status(res);
			break;

		    case RESOBJ_STATUS_SERVER:		/* RESTRICTED */
			statusp = (ResobjHandle)va_arg(ap, ResStat_t *);
			if (statusp == NULL)
				return (D_BADARG);
			else
				(*statusp) = Resobj_Server_Status(res);
			break;

		    case RESOBJ_STATUS_AUTOCLIENT:	/* RESTRICTED */
			statusp = (ResobjHandle)va_arg(ap, ResStat_t *);
			if (statusp == NULL)
				return (D_BADARG);
			else
				(*statusp) = Resobj_Autoclient_Status(res);
			break;

		    case RESOBJ_MODIFY:
			modp = va_arg(ap, ResMod_t *);
			if (modp == NULL)
				return (D_BADARG);
			else
				(*modp) = Resobj_Modify(res);
			break;

		    case RESOBJ_MODIFY_STANDALONE:	/* RESTRICTED */
			modp = va_arg(ap, ResMod_t *);
			if (privilege == NOPRIVILEGE)
				return (D_BADARG);
			else if (modp == NULL)
				return (D_BADARG);
			else
				(*modp) = Resobj_Standalone_Modify(res);
			break;

		    case RESOBJ_MODIFY_SERVER:		/* RESTRICTED */
			modp = va_arg(ap, ResMod_t *);
			if (privilege == NOPRIVILEGE)
				return (D_BADARG);
			else if (modp == NULL)
				return (D_BADARG);
			else
				(*modp) = Resobj_Server_Modify(res);
			break;

		    case RESOBJ_MODIFY_AUTOCLIENT:	/* RESTRICTED */
			modp = va_arg(ap, ResMod_t *);
			if (privilege == NOPRIVILEGE)
				return (D_BADARG);
			if (modp == NULL)
				return (D_BADARG);
			else
				(*modp) = Resobj_Autoclient_Modify(res);
			break;

		    case RESOBJ_CONTENT_CLASS:
			classp = va_arg(ap, ResClass_t *);
			if (classp == NULL)
				return (D_BADARG);
			else
				(*classp) = Resobj_Content_Class(res);
			break;

		    case RESOBJ_CONTENT_SOFTWARE:
			valp = va_arg(ap, int *);
			if (valp == NULL)
				return (D_BADARG);
			else
				(*valp) = Resobj_Content_Software(res);
			break;

		    case RESOBJ_CONTENT_EXTRA:
			valp = va_arg(ap, int *);
			if (valp == NULL)
				return (D_BADARG);
			else
				(*valp) = Resobj_Content_Extra(res);
			break;

		    case RESOBJ_CONTENT_SERVICES:
			valp = va_arg(ap, int *);
			if (valp == NULL)
				return (D_BADARG);
			else
				(*valp) = Resobj_Content_Services(res);
			break;

		    case RESOBJ_DEV_DFLTDEVICE:
			valp = va_arg(ap, int *);
			if (valp == NULL)
				return (D_BADARG);
			else
				(*valp) = Resobj_Dev_Dfltdevice(res);
			break;

		    case RESOBJ_DEV_EXPLMINIMUM:
			valp = va_arg(ap, int *);
			if (valp == NULL)
				return (D_BADARG);
			else
				(*valp) = Resobj_Dev_Explmin(res);
			break;

		    case RESOBJ_DEV_EXPLDISK:
			cp = va_arg(ap, char *);
			if (cp ==  NULL)
				return (D_BADARG);
			else
				(void) strcpy(cp,
					Resobj_Dev_Expldisk(res));
			break;

		    case RESOBJ_DEV_EXPLDEVICE:
			valp = va_arg(ap, int *);
			if (valp == NULL)
				return (D_BADARG);
			else
				(*valp) = Resobj_Dev_Expldevice(res);
			break;

		    case RESOBJ_DEV_EXPLSIZE:
			valp = va_arg(ap, int *);
			if (valp == NULL)
				return (D_BADARG);
			else
				(*valp) = Resobj_Dev_Explsize(res);
			break;

		    case RESOBJ_DEV_EXPLSTART:
			valp = va_arg(ap, int *);
			if (valp == NULL)
				return (D_BADARG);
			else
				(*valp) = Resobj_Dev_Explstart(res);
			break;

		    case RESOBJ_DEV_PREFDISK:
			cp = va_arg(ap, char *);
			if (cp == NULL)
				return (D_BADARG);
			else
				(void) strcpy(cp, Resobj_Dev_Prefdisk(res));
			break;

		    case RESOBJ_DEV_PREFDEVICE:
			valp = va_arg(ap, int *);
			if (valp == NULL)
				return (D_BADARG);
			else
				(*valp) = Resobj_Dev_Prefdevice(res);
			break;


		    case RESOBJ_FS_ACTION:
			fsactp = va_arg(ap, FsAction_t *);
			if (fsactp == NULL)
				return (D_BADARG);
			else
				(*fsactp) = Resobj_Fs_Action(res);
			break;

		    case RESOBJ_FS_MOUNTOPTS:	/* user must pass in array */
			cp = va_arg(ap, char *);
			if (cp == NULL)
				return (D_BADARG);
			else if (Resobj_Fs_Mountopts(res) == NULL)
				*cp = '\0';
			else
				(void) strcpy(cp, Resobj_Fs_Mountopts(res));
			break;

		    case RESOBJ_FS_MINFREE:
			valp = va_arg(ap, int *);
			if (valp == NULL)
				return (D_BADARG);
			else
				(*valp) = Resobj_Fs_Minfree(res);
			break;

		    case RESOBJ_FS_PERCENTFREE:
			valp = va_arg(ap, int *);
			if (valp == NULL)
				return (D_BADARG);
			else
				(*valp) = Resobj_Fs_Percentfree(res);
			break;

		    default:
			return (D_BADARG);
		}
	}

	return (D_OK);
}

/*
 * Function:	coreResobjCreate
 * Description: Low-level function used to create a new resource object
 *		and initialize it's attributes to a default value set,
 *		which can also be overridden by user supplied attributes.
 *
 *		NOTE:	only privileged calls are permitted to create
 *			resources with a status of RESSTAT_IGNORED
 *
 * Scopt:	private
 * Parameters:	type	[RO] (ResType_t)
 *			Resource type. Allowable values are:
 *			    RESTYPE_DIRECTORY
 *			    RESTYPE_SWAP
 *			    RESTYPE_UNNAMED
 *		name	[RO, *RO] (char *)
 *			Resource name proposed. Valid values are:
 *			(1) file system path	RESTYPE_DIRECTORY/RESTYPE_SWAP
 *			(2) "swap"		RESTYPE_SWAP only
 *			(3) ""			RESTYPE_UNNAMED only
 *		privilege [RO] (int)
 *			Privilege to modify restricted attributes. Valid
 *			valid are:
 *			  PRIVILEGE
 *			  NOPRIVILEGE
 *		ap	  [RO] (va_list)
 *			Keyword/attribute pairs.
 * Return:	NULL	  add failed
 *		Resobj *  pointer to new resource object
 */
static Resobj *
coreResobjCreate(ResType_t type, char *name,
		int instance, int privilege, va_list ap)
{
	int		status = D_OK;
	Resobj *	res;
	ResStat_t	resstat;

	/* validate parameters */
	if (!ResobjIsValidName(name) ||
			!ResobjIsValidInstance(instance) ||
			instance == VAL_UNSPECIFIED)
		return (NULL);

	if (privilege != NOPRIVILEGE && privilege != PRIVILEGE)
		return (NULL);

	/*
	 * make sure the name is legal for the resource type; note
	 * that swap resources can be either swap* or a file name
	 */
	switch (type) {
	    case RESTYPE_DIRECTORY:
		if (!is_pathname(name))
			return (NULL);
		break;
	    case RESTYPE_SWAP:
		if (privilege == NOPRIVILEGE && !NameIsSwap(name) &&
				!NameIsPath(name))
			return (NULL);
		break;
	    case RESTYPE_UNNAMED:
		if (!streq(name, ""))
			return (NULL);
		break;
	    default:
		return (NULL);
	}

	/*
	 * check to make sure we are not adding a duplicate resource
	 * name; make sure to check all resources and not just resources
	 * with a status != RESSTAT_IGNORED
	 */
	if (ResobjFindPriv(name, instance) != NULL)
		return (NULL);

	/*
	 * create a new basic resource object structure
	 */
	if ((res = (Resobj *)xcalloc(sizeof (Resobj))) == NULL)
		return (NULL);

	/*
	 * the type must be set explicitly before calling the attribute
	 * setting routines
	 */
	Resobj_Type(res) = type;

	/*
	 * set default values for common attributes; the origin is always
	 * RESORIGIN_APPEXT and should be overridden by privileged calls
	 * if necessary
	 */
	if (status == D_OK) {
		if (Resobj_Type(res) == RESTYPE_SWAP && NameIsPath(name))
			resstat = RESSTAT_DEPENDENT;
		else
			resstat = RESSTAT_INDEPENDENT;

		status = ResobjSetAttributePriv((ResobjHandle)res,
			RESOBJ_NAME,		  name,
			RESOBJ_INSTANCE,	  instance,
			RESOBJ_ORIGIN,	  RESORIGIN_APPEXT,
			RESOBJ_MODIFY_STANDALONE, RESMOD_ANY,
			RESOBJ_MODIFY_SERVER,	  RESMOD_ANY,
			RESOBJ_MODIFY_AUTOCLIENT, RESMOD_ANY,
			RESOBJ_STATUS_STANDALONE, resstat,
			RESOBJ_STATUS_SERVER,	  resstat,
			RESOBJ_STATUS_AUTOCLIENT, resstat,
			RESOBJ_DEV_EXPLMINIMUM,	  VAL_UNSPECIFIED,
			RESOBJ_DEV_EXPLSIZE,	  VAL_UNSPECIFIED,
			RESOBJ_DEV_EXPLSTART,	  VAL_UNSPECIFIED,
			RESOBJ_DEV_EXPLDISK,	  NULL,
			RESOBJ_DEV_EXPLDEVICE,	  VAL_UNSPECIFIED,
			RESOBJ_DEV_PREFDISK,	  NULL,
			RESOBJ_DEV_PREFDEVICE,	  VAL_UNSPECIFIED,
			RESOBJ_CONTENT_CLASS,	  RESCLASS_REPOSITORY,
			RESOBJ_CONTENT_SOFTWARE,  0,
			RESOBJ_CONTENT_EXTRA,	  0,
			RESOBJ_CONTENT_SERVICES,  0,
			NULL);
	}

	/*
	 * initialize file system specific data if for directory resources
	 */
	if (status == D_OK) {
		if (ResobjIsDirectory(res)) {
			status = ResobjSetAttributePriv(res,
				RESOBJ_FS_ACTION,  	FSACT_CREATE,
				RESOBJ_FS_MOUNTOPTS,	NULL,
				RESOBJ_FS_MINFREE,	VAL_UNSPECIFIED,
				RESOBJ_FS_PERCENTFREE,	VAL_UNSPECIFIED,
				NULL);
		}
	}

	/*
	 * override the current settings with whatever the caller
	 * specified explicitly
	 */
	if (status == D_OK)
		status = coreResobjSetAttribute(res, privilege, ap);

	/*
	 * if there was a failure anywhere during setup, deallocate
	 * the malloced space before returning
	 */
	if (status != D_OK) {
		ResobjDeallocate(res);
		return (NULL);
	} else {
		/* add the resource to the resource list */
		ResobjAddToList(res);
		return (res);
	}
}

/*
 * Function:	coreResobjDestroy
 * Description: Low level routine used to free all remove a resource object
 *		from the list and free all associated memory.
 * Scope:	private
 * Parameters:	res	   [RO, *RO] (Resobj *)
 *			   Pointer to resource object to be deleted.
 *		privilege  [RO] (int)
 *			   privilege to modify restricted attributes
 *			   (NOPRIVILEGE/PRIVILEGE)
 * Return:	D_OK	   directory structure deleted from list
 *		D_BADARG   directory structure deletion failed
 */
static int
coreResobjDestroy(Resobj *res, int privilege)
{
	Resobj **	resp;

	/* validate parameters */
	if (res == NULL ||
			(privilege != NOPRIVILEGE && privilege != PRIVILEGE))
		return (D_BADARG);

	/* only privileged calls can delete default resources */
	if (privilege != PRIVILEGE && Resobj_Origin(res) == RESORIGIN_DEFAULT)
		return (D_BADARG);

	/*
	 * the resource must exist in the resource list; get a pointer
	 * to it's reference pointer
	 */
	if ((resp = ResobjFindReference(res)) == NULL)
		return (D_OK);

	/*
	 * unlink the entry from the linked list and deallocate all
	 * dynamically allocated memory
	 */
	(*resp) = Resobj_Next(res);
	ResobjDeallocate(res);

	return (D_OK);
}

/*
 * Function:	ResobjAddToList
 * Description:	Add a resource object to the resource list, keeping
 *		the list in strcmp order.
 *		WARNING: No checking for replications is done.
 * Scope:	private
 * Parameters:	res	[RO, *RO] (Resobj *)
 *			Pointer to resource object
 * Return:	none
 */
static void
ResobjAddToList(Resobj *res)
{
	Resobj **	resp;


	WALK_LIST_INDIRECT(resp, _ResourceList) {
		if (strcmp(Resobj_Name(res), Resobj_Name(*resp)) < 0)
			break;
	}

	Resobj_Next(res) = *resp;
	(*resp) = res;
}

/*
 * Function:	ResobjFindReference
 * Description: Search for the address of the pointer which references
 *		the resource specified.
 * Scope:	private
 * Parameters:	res	[RO, *RO] (Resobj *)
 *			Resource object pointer.
 * Return:	NULL	  Invalid parameter, or could not find specified
 *			  resource
 *		Resobj ** address of reference to resource object
 */
static Resobj **
ResobjFindReference(Resobj *res)
{
	Resobj **	resp = NULL;

	/* validate parameters */
	if (res == NULL)
		return (NULL);

	WALK_LIST_INDIRECT(resp, _ResourceList)
		if (*resp == res)
			break;

	return (resp);
}

/*
 * Function:	ResobjCreateDup
 * Description:	Duplicate a resource object, including duplicating
 *		dynamically allocated components, but not altering
 *		the "next" pointer component. This is used for save
 *		and restore mechanisms when updating the resource
 *		object.
 * Scope:	private
 * Parameters:	res	[RO, *RO] (Resobj *)
 *			Pointer to resource object.
 * Return:	NULL	  Invalid argument or malloc failure
 *		Resobj *  pointer to dynamically allocated duplicated
 *			  resource object
 */
static Resobj *
ResobjCreateDup(Resobj *res)
{
	Resobj *	nres;

	/* validate parameters */
	if (res == NULL)
		return (NULL);

	if ((nres = (Resobj *)xcalloc(sizeof (Resobj))) == NULL)
		return (NULL);

	/* copy all the obvious data and update pointers explicitly */
	(void) memcpy(nres, res, sizeof (Resobj));
	Resobj_Fs_Mountopts(nres) = xstrdup(Resobj_Fs_Mountopts(res));

	return (nres);
}

/*
 * Function:	ResobjDeallocate
 * Description:	Deallocate a resource object, including all dynamically
 *		allocated components.
 * Scope:	private
 * Parameters:	res	[RO, *RO] (Resobj *)
 *			Pointer to resource object.
 * Return:	none
 */
static void
ResobjDeallocate(Resobj *res)
{
	/* validate parameters */
	if (res == NULL)
		return;

	/* free the mount options if they were allocated */
	if (Resobj_Fs_Mountopts(res) != NULL)
		free(Resobj_Fs_Mountopts(res));

	/* free the resource object itself */
	free(res);
}

/*
 * Function:	ResobjIsExistingPriv
 * Description:	Determine if the resource specified is in the resource
 *		list, including ignored resources.
 * Scope:	private
 * Parameters:	res	[RO] (ResobjHandle)
 *			Resource handle.
 * Return:	1	Resource is valid and exists
 *		0	Resource does not exist in resource list.
 */
static int
ResobjIsExistingPriv(ResobjHandle res)
{
	ResobjHandle	rp;

	/* validate parameters */
	if (res == NULL)
		return (0);

	WALK_RESOURCE_LIST_PRIV(rp, RESTYPE_UNDEFINED) {
		if (rp == res)
			return (1);
	}

	return (0);
}
