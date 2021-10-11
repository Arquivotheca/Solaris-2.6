#ifndef lint
#pragma ident "@(#)svc_checkpoint.c 1.2 96/07/11 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	svc_checkpoint.c
 * Group:	libspmisvc
 * Description:	Modules used to create, compare, restore, and destroy
 *		checkpointing information for disk and resource configurations.
 *		This is used internally by the library during autolayout
 *		retries.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include "spmisvc_lib.h"
#include "spmistore_lib.h"
#include "spmicommon_api.h"

/* ---------------------- internal functions ----------------------- */

/*
 * Function:	CheckpointCompare
 * Description:	Compare the state of the slices within a given checkpoint
 *		against the current, committed, or existing slice state.
 *		This function is only valid if the machine type has not
 *		changed since the checkpoint was made.
 * Scope:	internal
 * Parameters:	state	[RO] (Label_t)
 *			The state of the disk object against which the disk
 *			state of the checkpoint should be compared.  Valid
 *			values are:
 *			  CFG_CURRENT	- current configuration
 *			  CFG_COMMIT	- committed configuration
 *			  CFG_EXIST	- existing configuration
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk object pointer to be used in the disk state
 *			comparison.
 *		save	[RO] (CheckHandle)
 *			Valid checkpoint handle to be used in the comparison.
 *		flags	[RO] (u_char)
 *			Flags indicating constraints on the comparison.
 *			Valid values are:
 *			  CHECKPOINT_DISKS	compare slice configurations
 *			  CHECKPOINT_RESOURCES	compare resource statuses
 * Return:	D_OK	  The checkpoint is identical to the specified state
 *		D_FAILED  Invalid parameter, or the checkpoint differs from
 *			  the specified state
 */
int
CheckpointCompare(Label_t state, Disk_t *dp, CheckHandle save, u_char flags)
{
	ResStatEntry *  rp;
	Checkpoint *	chkpt;
	Disk_t *	sdp;
	int		s;

	/* validate parameters */
	if (!sdisk_is_usable(dp) || save == NULL || flags == 0)
		return (D_FAILED);

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (D_FAILED);

	chkpt = (Checkpoint *)save;

	/* the caller requested a resource status comparison */
	if (flags & CHECKPOINT_RESOURCES) {
		WALK_LIST(rp, chkpt->resources) {
			if (ResobjIsExisting(rp->resource)) {
				if (rp->status != Resobj_Status(rp->resource))
					return (D_FAILED);
			}
		}
	}

	/* the caller requested a disk configuration comparison */
	if (flags & CHECKPOINT_DISKS) {
		/*
		 * for each disk in the checkpoint configuration which has a
		 * valid sdisk, check to see if any of the components of the
		 * slices within the specified state have changed
		 */
		WALK_LIST(sdp, chkpt->disks) {
			if (!sdisk_is_usable(sdp))
				continue;

			if (streq(disk_name(sdp), disk_name(dp))) {
				WALK_SLICES_STD(s) {
					if (memcmp(Sliceobj_Addr(state, sdp, s),
						    Sliceobj_Addr(state, dp, s),
						    sizeof (Slice_t)) != 0) {
						return (D_FAILED);
					}
				}
			}
		}
	}

	return (D_OK);
}

/*
 * Function:	CheckpointCreate
 * Description:	Create a checkpoint containing a copy of the resource status
 *		and disk state information. Return a checkpoint handle to
 *		the dynamically allocated structure. Note that the caller is
 *		responsible for deallocating the memory associated with the
 *		checkpoint structure using CheckpointDestroy().
 * Scope:	internal
 * Parameters:	none
 * Return:	CheckHandle  	checkpoint handle
 *		NULL		checkpoint creation failure
 */
CheckHandle
CheckpointCreate(void)
{
	Checkpoint *	chkpt;
	Disk_t **	dpp;
	Disk_t *	dp;

	/* create a new state object */
	if ((chkpt = (Checkpoint *)xcalloc(sizeof (Checkpoint))) == NULL)
		return (NULL);

	/* create the resource status entry duplication list */
	if (ResobjGetStatusList(&(chkpt->resources)) != D_OK) {
		(void) CheckpointDestroy((CheckHandle)chkpt);
		return (NULL);
	}

	/* duplicate all selected disks */
	dpp = &(chkpt->disks);
	WALK_DISK_LIST(dp) {
		if (!sdisk_is_usable(dp))
			continue;

		if (((*dpp) = (Disk_t *)xmalloc(sizeof (Disk_t))) == NULL) {
			(void) CheckpointDestroy((CheckHandle)chkpt);
			return (NULL);
		}

		(void) memcpy((*dpp), dp, sizeof (Disk_t));
		(*dpp)->next = NULL;
		dpp = &((*dpp)->next);
	}

	return ((CheckHandle)chkpt);
}

/*
 * Function:	CheckpointDestroy
 * Description:	Free all dynamically allocated memory associated with a
 *		checkpoint.
 * Scope:	internal
 * Parameters:	handle	[RO] (CheckHandle)
 *			Handle for existing checkpoint to be destroyed.
 * Return:	D_OK	  successfully destroyed checkpoint
 *		D_FAILED  failed to destroy checkpoint
 */
int
CheckpointDestroy(CheckHandle handle)
{
	Checkpoint *	chkpt;
	Disk_t *	next;
	Disk_t *	dp;

	/* validate parameters */
	if (handle == NULL)
		return (D_FAILED);

	chkpt = (Checkpoint *)handle;

	/* free the disk list component */
	for (dp = chkpt->disks; dp != NULL; dp = next) {
		next = next_disk(dp);
		DiskobjDestroy(dp);
	}

	/* free the resource list component */
	ResobjFreeStatusList(chkpt->resources);

	/* free the checkpoint structure itself */
	free(chkpt);

	return (D_OK);
}

/*
 * Function:	CheckpointRestore
 * Description:	Restore the status fields of the resource structure, and/or the
 *		entire state of the disk list (entire disk objects), using
 *		the specified checkpoint as the source of configuration.  If
 *		a resource has been deleted from the resource list since
 *		the checkpoint was made, the old resource in the checkpoint is
 *		ignored. Resource list manipulation of this nature should
 *		not be occuring between checkpoints.
 * Scope:	internal
 * Parameters:	handle	[RO] (CheckHandle)
 *			Valid checkpoint handle to use for the restoration.
 *		flags	[RO] (u_char)
 *			Flags identifying which parts of the checkpoint
 *			should be restored. Valid bit fields are:
 *			    CHECKPOINT_DISKS
 *			    CHECKPOINT_RESOURCES
 *			    CHECKPOINT_ALL
 * Return:	D_OK	  restoration from checkpoint was successful
 *		D_FAILED  restoration from cehckpoint failed
 */
int
CheckpointRestore(CheckHandle handle, u_char flags)
{
	Checkpoint *	  chkpt;
	ResStatEntry *    rp;
	Disk_t *	  dp;
	Disk_t *	  cdp;
	Disk_t *	  next;

	/* validate parameters */
	if (handle == NULL)
		return (D_FAILED);

	if (flags == 0)
		return (D_FAILED);

	chkpt = (Checkpoint *)handle;

	/* restore the resource statuses */
	if (flags & CHECKPOINT_RESOURCES) {
		WALK_LIST(rp, chkpt->resources) {
			if (ResobjIsExisting(rp->resource)) {
				(void) ResobjSetAttribute(rp->resource,
					RESOBJ_STATUS,	rp->status,
					NULL);
			}
		}
	}

	/* restore the disk configurations */
	if (flags & CHECKPOINT_DISKS) {
		WALK_LIST(cdp, chkpt->disks) {
			if ((dp = find_disk(disk_name(cdp))) != NULL) {
				/*
				 * save the original "next" pointer, copy the
				 * checkpoint config to the current config, and
				 * restore the original "next" pointer
				 */
				next = dp->next;
				(void) memcpy(dp, cdp, sizeof (Disk_t));
				dp->next = next;
			}
		}
	}

	return (D_OK);
}
