/*
 * *********************************************************************
 * The Legal Stuff:						       *
 * *********************************************************************
 * Copyright (c) 1996 Sun Microsystems, Inc.  All Rights Reserved. Sun *
 * considers its source code as an unpublished, proprietary trade      *
 * secret, and it is available only under strict license provisions.   *
 * This copyright notice is placed here only to protect Sun in the     *
 * event the source is deemed a published work.	 Dissassembly,	       *
 * decompilation, or other means of reducing the object code to human  *
 * readable form is prohibited by the license agreement under which    *
 * this code is provided to the user or company in possession of this  *
 * copy.							       *
 *								       *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the    *
 * Government is subject to restrictions as set forth in subparagraph  *
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software    *
 * clause at DFARS 52.227-7013 and in similar clauses in the FAR and   *
 *  NASA FAR Supplement.					       *
 * *********************************************************************
 */

#ifndef lint
#pragma ident "@(#)svc_slice_list.c 1.18 96/07/30 SMI"
#endif

#include <stdio.h>
#include <sys/vfstab.h>
#include <malloc.h>
#include "spmisoft_api.h"
#include "spmisvc_api.h"

/*
 * *********************************************************************
 * FUNCTION NAME: SLSetAllowedStates
 *
 * DESCRIPTION:
 *  This function sets the allowed states for the slice entry based on
 *  the selections set int he entry.
 *
 * RETURN:
 *  TYPE			     DESCRIPTION
 *  void
 *
 * PARAMETERS:
 *  TYPE			     DESCRIPTION
 *  TSLEntry *			     A pointer to the slice list entry.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

void
SLSetAllowedStates(TSLEntry *Entry)
{
	/*
	 * Set up the valid states mask based on whether it is in the VFSTab and
	 * the setting in the Space entry.
	 */

	if (Entry->InVFSTab) {

		/*
		 * If the slice is collapsed (marked as ignore in the
		 * space entry), then the only valid state is Collapseable
		 */
		if (SL_SLICE_IS_COLLAPSED(Entry)) {
			Entry->AllowedStates = SLCollapse;
		}

		/*
		 * If the slice has insufficient space
		 */

		else if (SL_SLICE_HAS_INSUFF_SPACE(Entry)) {

			/*
			 * If the slice is the root slice then the only
			 * valid state is changeable.
			 */

			if (streq(Entry->MountPoint, ROOT)) {
				Entry->AllowedStates = SLChangeable;
			} else {
				Entry->AllowedStates =
					SLChangeable | SLCollapse;
			}

		}

		/*
		 * Otherwise, the slice has sufficient space
		 */

		else {

			/*
			 * If the slice contains a swap
			 */

			if (Entry->FSType == SLSwap) {
				Entry->AllowedStates = SLFixed | SLMoveable |
					SLChangeable;

			/*
			 * If the file system is root then since we don't
			 * allow the root slice to move we set the allowed
			 * states to fixed and changeable
			 */

			} else if (streq(Entry->MountPoint, ROOT)) {
				Entry->AllowedStates = SLChangeable | SLFixed;

			/*
			 * Otherwise, it is a normal file system
			 */

			} else {

				/*
				 * If the slice is set to ignore in the space
				 * structure then that means that it has been
				 * collapsed
				 */

				if (Entry->Space &&
				    Entry->Space->fsp_flags & FS_IGNORE_ENTRY) {
					Entry->AllowedStates = SLCollapse;
				} else {
					Entry->AllowedStates =
						SLFixed | SLMoveable |
						SLChangeable;
				}
			}
		}
	}

	/*
	 * Otherwise it is not in the vfstab
	 */

	else {
		Entry->AllowedStates = SLFixed | SLAvailable;
	}
}

/*
 * *********************************************************************
 * FUNCTION NAME: SLAdd
 *
 * DESCRIPTION:
 *  This function allows the calling application to add slices to the
 *  list of slices.
 *
 * RETURN:
 *  TYPE			     DESCRIPTION
 *  TSLError			     This is the enumerated error
 *				     code defined in the public
 *				     header.  Upon success, SLSuccess
 *				     will be returned.	Upon error,
 *				     the appropriate error code will
 *				     returned.
 *
 * PARAMETERS:
 *  TYPE			     DESCRIPTION
 *  TList			     This is the SliceList returned
 *				     by the call to LLCreateList().
 *  char *			     The device name for the slice to
 *				     added to the archive list.
 *				     (e.g. c0t3d0s0)
 *  char *			     The mount point for the slice.
 *  TBoolean			     Whether the slice is in the current
 *				     /etc/vfstab.
 *  TSLState			     The current state of the slice.
 *  void *			     A pointer for any extra data that
 *				     the user might want to append onto
 *				     the list.
 *  TSLEntry **			     A double pointer to the data
 *				     structure that is allocated and
 *				     inserted into the slice list.
 *				     If NULL then the pointer is not set.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TSLError
SLAdd(TList SliceList,
    char *SliceName,
    char *MountPoint,
    int MountPointInstance,
    TBoolean InVFSTab,
    TSLFSType FSType,
    TSLState State,
    unsigned long Size,
    FSspace *Space,
    void *Extra,
    TSLEntry ** SLEntry)
{
	TSLEntry	*LocalSLEntry;
	TLink		NewLink;

	FILE		*VFSFile;
	struct vfstab	VFSEntry;
	char		VFSPath[PATH_MAX];
	char		VFSSpecial[PATH_MAX];

	/*
	 * Malloc out space for the new slice
	 */

	if (!(LocalSLEntry = (TSLEntry *) malloc(sizeof (TSLEntry)))) {
		return (SLMemoryAllocationError);
	}

	/*
	 * Copy in the name of the slice along with the device ID
	 */

	(void) strcpy(LocalSLEntry->SliceName, SliceName);
	LocalSLEntry->State = State;
	LocalSLEntry->InVFSTab = InVFSTab;
	LocalSLEntry->FSType = FSType;
	LocalSLEntry->Size = Size;
	LocalSLEntry->Space = Space;
	LocalSLEntry->Extra = Extra;

	/*
	 * See if a MountPoint string was provided
	 */

	if (MountPoint) {
		(void) strcpy(LocalSLEntry->MountPoint, MountPoint);

	/*
	 * If not, then lets go open the controlling /etc/vfstab and
	 * scan it looking for the slice name.
	 */

	} else {

		if (*get_rootdir() == '\0') {
			(void) strcpy(VFSPath, "/etc/vfstab");
		} else {
			(void) strcpy(VFSPath, get_rootdir());
			(void) strcat(VFSPath, "/etc/vfstab");
		}

		if (!(VFSFile = fopen(VFSPath, "r"))) {
			return (SLSystemCallFailure);
		}
		(void) sprintf(VFSSpecial, "/dev/dsk/%s", SliceName);
		if (getvfsspec(VFSFile, &VFSEntry, VFSSpecial)) {
			LocalSLEntry->MountPoint[0] = '\0';
			LocalSLEntry->InVFSTab = False;
			LocalSLEntry->FSType = SLUnknown;
		} else {
			if (VFSEntry.vfs_mountp) {
				(void) strcpy(LocalSLEntry->MountPoint,
				    VFSEntry.vfs_mountp);
			}
			if (VFSEntry.vfs_fstype) {
				if (strcmp(VFSEntry.vfs_fstype, "swap") == 0) {
					LocalSLEntry->FSType = SLSwap;
				} else if (strcmp(VFSEntry.vfs_fstype,
				    "ufs") == 0) {
					LocalSLEntry->FSType = SLUfs;
				} else {
					LocalSLEntry->FSType = SLUnknown;
				}
			}
			LocalSLEntry->InVFSTab = True;
		}
		(void) fclose(VFSFile);
	}

	/*
	 * If the instance for the mountpoint is set to SL_GENERATE_INSTANCE
	 * then generate the instance based on what is already in the slice
	 * list
	 */

	if (MountPointInstance == SL_GENERATE_INSTANCE) {
		LocalSLEntry->MountPointInstance = MountPointInstance;
	}

	/*
	 * Otherwise, just assign it the value that was given
	 */

	else {
		LocalSLEntry->MountPointInstance = MountPointInstance;
	}

	SLSetAllowedStates(LocalSLEntry);

	/*
	 * Check to make sure that the State and InVFSTab variables
	 * are an allowed combination.
	 */

	if (!(LocalSLEntry->AllowedStates & LocalSLEntry->State)) {
		free(LocalSLEntry);
		return (SLStateNotAllowed);
	}

	/*
	 * Create a new link for the entry
	 */

	if ((LLCreateLink(&NewLink, LocalSLEntry)) != LLSuccess) {
		return (SLListManagementError);
	}

	/*
	 * Add the new link to the File System's file list.
	 */

	if ((LLAddLink(SliceList,
		    NewLink,
		    LLTail)) != LLSuccess) {
		return (SLListManagementError);
	}

	/*
	 * If the calling appliation provided a non-null pointer then set it
	 * to point to the allocated data astructure.
	 */

	if (SLEntry) {
		*SLEntry = LocalSLEntry;
	}
	return (SLSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: SLNameInList
 *
 * DESCRIPTION:
 *
 * RETURN:
 *  TYPE			     DESCRIPTION
 *  TSLError			     This is the enumerated error
 *				     code defined in the public
 *				     header.  Upon success, SLSuccess
 *				     will be returned.	Upon error,
 *				     the appropriate error code will
 *				     returned.
 *
 * PARAMETERS:
 *  TYPE			     DESCRIPTION
 *  TList			     This is the SliceList returned by
 *				     the call to LLCreateList().
 *  char *			     The name of the slice that
 *				     is to found in the list.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TSLError
SLNameInList(TList SliceList, char *SliceName)
{
	TSLEntry	*SLEntry;
	TLLError	LLError;
	TLink		CurrentLink;
	TBoolean	Done;

	if ((LLError = LLGetLinkData(SliceList,
		    LLHead,
		    &CurrentLink,
		    (void **) &SLEntry))) {
		switch (LLError) {
		case LLListEmpty:
			return (SLSliceNotFound);
		default:
			return (SLListManagementError);
		}
	}

	/*
	 * Loop through all of the slices to find a match
	 */

	Done = False;
	while (!Done) {
		if (strcmp(SliceName, SLEntry->SliceName) == 0) {
			return (SLSuccess);
		}
		if ((LLError = LLGetLinkData(SliceList,
			    LLNext,
			    &CurrentLink,
			    (void **) &SLEntry))) {
			switch (LLError) {
			case LLEndOfList:
				return (SLSliceNotFound);
			default:
				return (SLListManagementError);
			}
		}
	}
	return (SLSliceNotFound);
}

/*
 * *********************************************************************
 * FUNCTION NAME: SLRemove
 *
 * DESCRIPTION:
 *  This function allows the user to remove a specific slice from the
 *  slice list.
 *
 * RETURN:
 *  TYPE			     DESCRIPTION
 *  TSLError			     This is the enumerated error
 *				     code defined in the public
 *				     header.  Upon success, SLSuccess
 *				     will be returned.	Upon error,
 *				     the appropriate error code will
 *				     returned.
 *
 * PARAMETERS:
 *  TYPE			     DESCRIPTION
 *  TList			     This is the SliceList returned by
 *				     the call to LLCreateList().
 *  char *			     The name of the slice to remove.
 *				     (e.g. c0t3d0s0)
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TSLError
SLRemove(TList SliceList, char *SliceName)
{
	TLink		CurrentLink;
	TSLEntry	*SLEntry;
	TLLError	LLError;
	TBoolean	Done;

	if ((LLError = LLGetLinkData(SliceList,
		    LLHead,
		    &CurrentLink,
		    (void **) &SLEntry))) {
		switch (LLError) {
		case LLListEmpty:
			return (SLSliceNotFound);
		default:
			return (SLListManagementError);
		}
	}

	/*
	 * Loop through all of the slices to find a match
	 */

	Done = False;
	while (!Done) {
		if (strcmp(SliceName, SLEntry->SliceName) == 0) {
			if ((LLRemoveLink(SliceList,
				    CurrentLink)) != LLSuccess) {
				return (SLListManagementError);
			}
			if ((LLDestroyLink(&CurrentLink, NULL)) != LLSuccess) {
				return (SLListManagementError);
			}
			free(SLEntry);
			return (SLSuccess);
		}
		if ((LLError = LLGetLinkData(SliceList,
			    LLNext,
			    &CurrentLink,
			    (void **) &SLEntry))) {
			switch (LLError) {
			case LLEndOfList:
				return (SLSliceNotFound);
			default:
				return (SLListManagementError);
			}
		}
	}
	return (SLSliceNotFound);
}

/*
 * *********************************************************************
 * FUNCTION NAME: SLClearCallback
 *
 * DESCRIPTION:
 *  This is the callback for the LLClear function.  This function free's
 *  the memory pointed to by the given data pointer.
 *
 * RETURN:
 *  TYPE			     DESCRIPTION
 *  TLLError			     The enumerated LinkList return
 *				     error type.  The valid values from
 *				     the callback are LLSuccess and
 *				     LLFailure.
 *
 * PARAMETERS:
 *  TYPE			     DESCRIPTION
 *  TDLLData			     A pointer to the data to be free'd
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TLLError
SLClearCallback(TLLData Data)
{
	free(Data);
	return (LLSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: SLStateToString
 *
 * DESCRIPTION:
 *  This is a helper function that converts the enumerated state into
 *  a human readable string.
 *
 * RETURN:
 *  TYPE			DESCRIPTION
 *  char *			The string corresponding to the provided
 *				enumerated type.
 *
 * PARAMETERS:
 *  TYPE			DESCRIPTION
 *  TSLState			The enumerated state to be converted.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

char *
SLStateToString(TSLState State)
{
	switch (State) {
	case SLFixed:
		return ("Fixed");
	case SLMoveable:
		return ("Movable");
	case SLChangeable:
		return ("Changeable");
	case SLAvailable:
		return ("Available");
	case SLCollapse:
		return ("Collapse");
	default:
		return ("Unknown");
	}
}

/*
 * *********************************************************************
 * FUNCTION NAME: SLStateToString
 *
 * DESCRIPTION:
 *  This is a helper function that converts the enumerated file system
 *  type into a human readable string.
 *
 * RETURN:
 *  TYPE			DESCRIPTION
 *  char *			The string corresponding to the provided
 *				enumerated type.
 *
 * PARAMETERS:
 *  TYPE			DESCRIPTION
 *  TSLFSType			The enumerated state to be converted.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

char *
SLFSTypeToString(TSLFSType FSType)
{
	switch (FSType) {
	case SLUfs:
		return ("UFS");
	case SLSwap:
		return ("Swap");
	case SLUnknown:
	default:
		return ("Unknown");
	}
}

/*
 * *********************************************************************
 * FUNCTION NAME: SLPrint
 *
 * DESCRIPTION:
 *  This is a helper function that allows the calling application to
 *  print the contents of the slice list to the specified
 *  file descriptor.
 *
 * RETURN:
 *  TYPE			     DESCRIPTION
 *  TSLError			     This is the enumerated error
 *				     code defined in the public
 *				     header.  Upon success, SLSuccess
 *				     will be returned.	Upon error,
 *				     the appropriate error code will
 *				     returned.
 *
 * PARAMETERS:
 *  TYPE			     DESCRIPTION
 *  TList			     This is the SliceList returned by
 *				     the call to LLCreateList().
 *  int	                             The file descriptor to direct the
 *				     output to.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TSLError
SLPrint(TList SliceList,
	void (*SliceEntryPrintFunction) (TSLEntry *SLEntry))
{
	TSLEntry	*SLEntry;
	int		i;
	TLink		CurrentLink;
	TBoolean	Done;

	i = 0;
	if (LLGetLinkData(SliceList,
		    LLHead,
		    &CurrentLink,
		    (void **) &SLEntry)) {
		return (SLListManagementError);
	} else {
		Done = False;
		while (!Done) {
			write_status(LOGSCR, LEVEL0, "SliceList[%d]", i);

			write_status(LOGSCR, LEVEL1,
			    "MountPoint         = %s", SLEntry->MountPoint);

			write_status(LOGSCR, LEVEL1,
			    "MountPointInstance = %d",
			    SLEntry->MountPointInstance);

			write_status(LOGSCR, LEVEL1,
			    "SliceName          = %s", SLEntry->SliceName);

			write_status(LOGSCR, LEVEL1|FMTPARTIAL,
			    "InVFSTab           = ");
			if (SLEntry->InVFSTab) {
				write_status(LOGSCR, CONTINUE, "True");
			} else {
				write_status(LOGSCR, CONTINUE, "False");
			}

			write_status(LOGSCR, LEVEL1|FMTPARTIAL,
			    "FSType             = ");

			write_status(LOGSCR,
			    CONTINUE,
			    SLFSTypeToString(SLEntry->FSType));

			write_status(LOGSCR, LEVEL1 | FMTPARTIAL,
			    "State              = ");

			write_status(LOGSCR,
			    CONTINUE,
			    SLStateToString(SLEntry->State));

			write_status(LOGSCR, LEVEL1 | FMTPARTIAL,
			    "AllowedStates      = ");

			if (SLEntry->AllowedStates & SLFixed) {
				write_status(LOGSCR,
				    FMTPARTIAL,
				    SLStateToString(SLFixed));
				write_status(LOGSCR,
				    FMTPARTIAL,
				    " ");
			}

			if (SLEntry->AllowedStates & SLMoveable) {
				write_status(LOGSCR,
				    FMTPARTIAL,
				    SLStateToString(SLMoveable));
				write_status(LOGSCR,
				    FMTPARTIAL,
				    " ");
			}

			if (SLEntry->AllowedStates & SLChangeable) {
				write_status(LOGSCR,
				    FMTPARTIAL,
				    SLStateToString(SLChangeable));
				write_status(LOGSCR,
				    FMTPARTIAL,
				    " ");
			}

			if (SLEntry->AllowedStates & SLAvailable) {
				write_status(LOGSCR,
				    FMTPARTIAL,
				    SLStateToString(SLAvailable));
				write_status(LOGSCR,
				    FMTPARTIAL,
				    " ");
			}

			if (SLEntry->AllowedStates & SLCollapse) {
				write_status(LOGSCR,
				    FMTPARTIAL,
				    SLStateToString(SLCollapse));
				write_status(LOGSCR,
				    FMTPARTIAL,
				    " ");
			}

			write_status(LOGSCR, CONTINUE, "");

			write_status(LOGSCR, LEVEL1|FMTPARTIAL,
			    "Searched           = ");
			if (SLEntry->Searched) {
				write_status(LOGSCR, CONTINUE, "True");
			} else {
				write_status(LOGSCR, CONTINUE, "False");
			}

			write_status(LOGSCR, LEVEL1,
			    "Size               = %u", SLEntry->Size);

			write_status(LOGSCR, LEVEL1,
			    "Space              = 0x%x", SLEntry->Space);

			write_status(LOGSCR, LEVEL1,
			    "Extra              = 0x%x", SLEntry->Extra);

			i++;

			/*
			 * Let the caller print any more data they want
			 * for this entry.
			 */
			if (SliceEntryPrintFunction) {
				(*SliceEntryPrintFunction) (SLEntry);
			}

			if (LLGetLinkData(SliceList,
				LLNext,
				&CurrentLink,
				(void **) &SLEntry)) {
				break;
			}
		}
	}
	return (SLSuccess);
}

/*
 * *********************************************************************
 * FUNCTION NAME: SLSortIntegerCompare
 *
 * DESCRIPTION:
 *  This is a helper function to the Sort Callback.  It takes in two
 *  integer values and compares them to find out if the first is
 *  less than, equal or greater than the second.
 *
 * RETURN:
 *  TYPE			     DESCRIPTION
 *  TLLCompare			     The enumerated return type defined
 *				     in the link list object for the
 *				     comparison return values.
 *
 * PARAMETERS:
 *  TYPE			     DESCRIPTION
 *  int				     The first integer to compare.
 *  int				     The socond integer to compare.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TLLCompare
SLSortIntegerCompare(int Number1, int Number2)
{

	if (Number1 < Number2)
		return (LLCompareLess);
	else if (Number1 == Number2)
		return (LLCompareEqual);
	else
		return (LLCompareGreater);
}


/*
 * *********************************************************************
 * FUNCTION NAME: SLSliceScan
 *
 * DESCRIPTION:
 *  This function scans a slice name and breaks it up into it's
 *  respsective c#, t#, d#, s#.
 *  Since t# is optional, t# is returned as -1 if there is none.
 *
 * RETURN:
 *  TYPE			     DESCRIPTION
 *  TSLError			     This is the enumerated error
 *				     code defined in the public
 *				     header.  Upon success, SLSuccess
 *				     will be returned.	Upon error,
 *				     SLFailure is returned.
 *
 * PARAMETERS:
 *  TYPE			     DESCRIPTION
 *  char *			     The slice name to scan.
 *  int *			     ptr to srea to store slice name's
 *				     controller number in.
 *  int *			     ptr to srea to store slice name's
 *				     target number in.
 *  int *			     ptr to srea to store slice name's
 *				     disk number in.
 *  int *			     ptr to srea to store slice name's
 *				     slice number in.
 *
 * DESIGNER/PROGRAMMER: Jo Pelkey/RMTC (719)528-3616
 * *********************************************************************
 */
static TSLError
SLSliceScan(char *slice_name, int *c1, int *t1, int *d1, int *s1)
{
	int		ScanRet;

	/*
	 * first try scanning it as slice name that looks like
	 * cot0d0s0
	 */
	ScanRet = sscanf(slice_name,
		    "c%dt%dd%ds%d",
		    c1, t1, d1, s1);
	if (ScanRet == 4) {
		return (SLSuccess);
	}

	/*
	 * Ok, it doesn't look like c0t0d0s0.
	 * try and see if it looks like c0d0s0.
	 */
	ScanRet = sscanf(slice_name,
		    "c%dd%ds%d",
		    c1, d1, s1);
	if (ScanRet == 3) {
		*t1 = -1;
		return (SLSuccess);
	}

	return (SLFailure);
}

/*
 * *********************************************************************
 * FUNCTION NAME: SLSortCallback
 *
 * DESCRIPTION:
 *  This is the callback for the LLSort function.  This function compares
 *  the contents of the two paths and determines which is alphabetically
 *  smaller.
 *
 * RETURN:
 *  TYPE			     DESCRIPTION
 *  TLLCompare			     The enumerated return type defined
 *				     in the link list object for the
 *				     comparison return values.
 *
 * PARAMETERS:
 *  TYPE			     DESCRIPTION
 *  void *			     User supplied data pointer.  Given
 *				     to the LLSort function when called.
 *  TDLLData			     A pointer to the entry to be
 *				     inserted.
 *  TDLLData			     A pointer to the current entry in
 *				     the sorted list.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

static TLLCompare
SLSortCallback(void *UserPtr, TLLData Insert, TLLData Sorted)
{
	TSLEntry	*InsertEntry;
	TSLEntry	*SortedEntry;
	TSLSortBy	*SortBy;

	int		c1,
			t1,
			d1,
			s1;
	int		c2,
			t2,
			d2,
			s2;
	TLLCompare	Compare;

	SortBy = (TSLSortBy *) UserPtr;
	InsertEntry = (TSLEntry *) Insert;
	SortedEntry = (TSLEntry *) Sorted;

	switch (*SortBy) {
	case SLSliceNameAscending:
	case SLSliceNameDescending:

		switch (*SortBy) {
		case SLSliceNameAscending:

			/*
			 * Scan 1st slice for all ints
			 */
			if (SLSliceScan(InsertEntry->SliceName,
				&c1, &t1, &d1, &s1) == SLFailure)
				return (LLCompareError);

			/*
			 * Scan 2nd slice for all ints
			 */

			if (SLSliceScan(SortedEntry->SliceName,
				&c2, &t2, &d2, &s2) == SLFailure)
				return (LLCompareError);
			break;

		case SLSliceNameDescending:

			/*
			 * Scan 1st slice for all ints
			 */

			if (SLSliceScan(SortedEntry->SliceName,
			    &c1, &t1, &d1, &s1) == SLFailure)
				return (LLCompareError);

			/*
			 * Scan 2nd slice for all ints
			 */

			if (SLSliceScan(InsertEntry->SliceName,
			    &c2, &t2, &d2, &s2)  == SLFailure)
				return (LLCompareError);
			break;
		}

		/*
		 * Compare the controller numbers
		 */

		Compare = SLSortIntegerCompare(c1, c2);
		if (Compare != LLCompareEqual)
			return (Compare);

		/*
		 * Compare the target numbers.
		 * Note that if there aren't any target numbers to
		 * compare at this point, that they're actually equal,
		 * so this will just fall through to the next
		 * comparsion...
		 */

		Compare = SLSortIntegerCompare(t1, t2);
		if (Compare != LLCompareEqual)
			return (Compare);

		/*
		 * Compare the disk numbers
		 */

		Compare = SLSortIntegerCompare(d1, d2);
		if (Compare != LLCompareEqual)
			return (Compare);

		/*
		 * Compare the slice numbers
		 */

		Compare = SLSortIntegerCompare(s1, s2);
		return (Compare);
	case SLMountPointAscending:
		if (strcmp(InsertEntry->MountPoint,
				SortedEntry->MountPoint) <= 0) {
			return (LLCompareLess);
		} else {
			return (LLCompareGreater);
		}
	case SLMountPointDescending:
		if (strcmp(InsertEntry->MountPoint,
				SortedEntry->MountPoint) >= 0) {
			return (LLCompareLess);
		} else {
			return (LLCompareGreater);
		}
	}
	return (LLCompareError);
}

/*
 * *********************************************************************
 * FUNCTION NAME: SLSort
 *
 * DESCRIPTION:
 *  This function sorts the slice list based on the sort by option.
 *
 * RETURN:
 *  TYPE			     DESCRIPTION
 *  TSLError			     This is the enumerated error
 *				     code defined in the public
 *				     header.  Upon success, SLSuccess
 *				     will be returned.	Upon error,
 *				     the appropriate error code will
 *				     returned.
 *
 * PARAMETERS:
 *  TYPE			     DESCRIPTION
 *  TList			     This is the SliceList returned by
 *				     the call to LLCreateList().
 *  int				     The file descriptor to direct the
 *				     output to.
 *
 * DESIGNER/PROGRAMMER: Craig Vosburgh/RMTC (719)528-3647
 * *********************************************************************
 */

TSLError
SLSort(TList SliceList, TSLSortBy SortBy)
{
	if (LLSortList(SliceList,
		SLSortCallback,
		&SortBy)) {
		return (SLListManagementError);
	}
	return (SLSuccess);
}
