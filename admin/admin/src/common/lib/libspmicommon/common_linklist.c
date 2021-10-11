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
#pragma ident "@(#)common_linklist.c 1.6 1 SMI"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common_linklist_in.h"

/*
 * *********************************************************************
 * MODULE NAME: LLCreateList
 *
 * DESCRIPTION:
 *  This function creates a new instance of a linked list.  This
 *  function must be called prior to using any of the other
 *  functions to manipulate the list.
 *
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TList *        The pointer to the list to be created.  Upon success
 *                 the contents of the pointer is set to the new list.
 *  TLLData        The pointer to the user data to be associated with
 *                 the list.  If the user does not have any list level
 *                 data to store, then NULL can be passed.
 *
 * CHANGE ACTIVITY:
 *       Date   Developer Name    Description of Changes
 *    05-Dec-94 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLCreateList(TList * List, TLLData Data)
{
	TLinkList	*LocalList;

	/*
	 * Get space for the linked list information.
	 */

	LocalList = (TLinkList *) malloc(sizeof (TLinkList));
	if (LocalList == NULL) {
		*List = NULL;
		return (LLMemoryAllocationError);
	}

	/*
	 * Initialize the linked list.
	 */

	LocalList->Initialized = LLINITIALIZED;
	LocalList->NumberLinks = 0;
	LocalList->Data = Data;
	LocalList->Head = NULL;
	LocalList->Tail = NULL;
	LocalList->Current = NULL;

	/*
	 * Point the user's list at the created list.
	 */

	*List = (TList) LocalList;

	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLCreateLink
 *
 * DESCRIPTION:
 *  This function creates an instance of a link.  This function must
 *  be called prior to using any function that manipulates the link.
 *  The provided Data pointer is associated with the created link.
 *
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TLink *        The pointer to the link to be created.  Upon success
 *                 the contents of the pointer is set to the new link.
 *  TLLData        A pointer to the user data to be associated with the
 *                 link.
 *
 * CHANGE ACTIVITY:
 *       Date   Developer Name    Description of Changes
 *    05-Dec-94 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLCreateLink(TLink * Link, TLLData Data)
{
	TDoubleLink	*LocalLink;

	/*
	 * Get space for the link information.
	 */

	LocalLink = (TDoubleLink *) malloc(sizeof (TDoubleLink));
	if (LocalLink == NULL) {
		*Link = NULL;
		return (LLMemoryAllocationError);
	}

	/*
	 * Initialize the link.
	 */

	LocalLink->Initialized = LLINITIALIZED;
	LocalLink->Next = NULL;
	LocalLink->Prev = NULL;
	LocalLink->Data = Data;

	/*
	 * Point the user's link at the created link.
	 */

	*Link = (TLink) LocalLink;

	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLAddLink
 *
 * DESCRIPTION:
 *  This function is used to add the provided link into the
 *  provided list.
 *
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TList          The list to add the link into.
 *  TLink          The link to add to the list.
 *  TLLOperation   Where the link is to be inserted.  Valid options are:
 *                   LLPrev    - Previous to the currrent link.
 *                   LLCurrent - Insert before the current link.  This
 *                               is equivalent to LLPrev.
 *                   LLNext    - Insert the link after the current link.
 *                   LLHead    - The beginning of the list
 *                   LLTail    - The end of the list
 *
 * CHANGE ACTIVITY:
 *       Date   Developer Name    Description of Changes
 *    05-Dec-94 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLAddLink(TList List, TLink Link, TLLOperation Operation)
{
	TLinkList	*LocalList;
	TDoubleLink	*LocalLink;

	/*
	 * Check to see if the user gave us good pointers to the Linked List
	 * and Link to be removed.
	 */

	if (List == NULL)
		return (LLInvalidList);

	if (Link == NULL)
		return (LLInvalidLink);

	/*
	 * Typecast the void pointers to usable pointers.
	 */

	LocalList = (TLinkList *) List;
	LocalLink = (TDoubleLink *) Link;

	/*
	 * Check to see if the user gave us a correctly initialized list and
	 * link.
	 */

	if (LocalList->Initialized != LLINITIALIZED) {
		return (LLInvalidList);
	}
	if (LocalLink->Initialized != LLINITIALIZED) {
		return (LLInvalidLink);
	}

	/*
	 * If the link's next and prev pointers are not set to NULL then the
	 * link is already in use so return an error.
	 */

	if (LocalLink->Prev != NULL && LocalLink->Next != NULL) {

		return (LLLinkInUse);
	}

	/*
	 * If the internal Linked List's head is set to NULL then this is
	 * first link to be added to the list.
	 */

	if (LocalList->Head == NULL && LocalList->Tail == NULL) {
		LocalList->Head = LocalLink;
		LocalList->Tail = LocalLink;
		LocalList->Current = LocalLink;
		LocalLink->Prev = NULL;
		LocalLink->Next = NULL;
	}

	/*
	 * Otherwise there is at least one link in the list so lets go add
	 * the new link like the user requested.
	 */

	else {
		switch (Operation) {

			/*
			 * The user requested that the new link be inserted
			 * at the current position or before the current link
			 * (Which to me means the same thing.
			 */

		case LLPrev:
		case LLCurrent:

			/*
			 * If the Head pointer is pointing to the current
			 * link then we need to set it to point at the new
			 * link.
			 */

			if (LocalList->Head == LocalList->Current)
				LocalList->Head = LocalLink;

			/*
			 * Assign the new link's next and previous pointers
			 * to point the current link's previous link and the
			 * current link respectively.
			 */

			LocalLink->Prev = LocalList->Current->Prev;
			LocalLink->Next = LocalList->Current;

			/*
			 * If the current link's prev pointer is not NULL (ie
			 * The current link is not the first link in the
			 * list) then update the previous links next pointer
			 * to point at the new link.
			 */

			if (LocalList->Current->Prev)
				LocalList->Current->Prev->Next = LocalLink;

			LocalList->Current->Prev = LocalLink;

			/*
			 * Reset the Current Pointer to point to the new
			 * link.
			 */

			LocalList->Current = LocalLink;

			break;

			/*
			 * The user requested that the new link be interted
			 * after the current link.
			 */

		case LLNext:

			/*
			 * If the Tail pointer is pointing to the current
			 * link then we need to set it to point at the new
			 * link.
			 */

			if (LocalList->Tail == LocalList->Current)
				LocalList->Tail = LocalLink;

			/*
			 * Assign the new link's next and previous pointers
			 * to point the current link and the current link's
			 * next link respectively.
			 */

			LocalLink->Prev = LocalList->Current;
			LocalLink->Next = LocalList->Current->Next;

			/*
			 * If the current link's next pointer is not NULL (ie
			 * The current link is not the last link in the list)
			 * then update the next links prev pointer to point
			 * at the new link.
			 */

			if (LocalList->Current->Next)
				LocalList->Current->Next->Prev = LocalLink;

			LocalList->Current->Next = LocalLink;

			/*
			 * Reset the Current Pointer to point to the new
			 * link.
			 */

			LocalList->Current = LocalLink;

			break;

			/*
			 * The user requested that the new link be interted
			 * at the beginning of the list.
			 */

		case LLHead:

			/*
			 * Since this link is at the head of the list set
			 * it's previous pointer to NULL and the next pointer
			 * to the previous head link.
			 */

			LocalLink->Prev = NULL;
			LocalLink->Next = LocalList->Head;

			/*
			 * Finally, patch the prev pointer for the link being
			 * moved to point at the new head link.
			 */

			LocalList->Head->Prev = LocalLink;

			/*
			 * Patch the list head pointer to point to the new
			 * link.
			 */

			LocalList->Head = LocalLink;

			/*
			 * Reset the Current Pointer to point to the new
			 * link.
			 */

			LocalList->Current = LocalLink;

			break;

			/*
			 * The user requested that the new link be interted
			 * at the end of the list.
			 */

		case LLTail:

			/*
			 * Since this link is at the end of the list set it's
			 * next pointer to NULL and the prev pointer to the
			 * previous tail link.
			 */

			LocalLink->Next = NULL;
			LocalLink->Prev = LocalList->Tail;

			/*
			 * Finally, patch the next pointer for the link being
			 * moved to point at the new tail link.
			 */

			LocalList->Tail->Next = LocalLink;

			/*
			 * Patch the list tail pointer to point to the new
			 * link.
			 */

			LocalList->Tail = LocalLink;

			/*
			 * Reset the Current Pointer to point to the new
			 * link.
			 */

			LocalList->Current = LocalLink;

			break;

		default:
			return (LLInvalidOperation);
		}
	}
	LocalList->NumberLinks++;

	/*
	 * Set the owning list to the current list
	 */

	LocalLink->OwningList = List;

	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLRemoveLink
 *
 * DESCRIPTION:
 *  This function is used to remove the specified link from the given
 *  list.
 *
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TList          The list to remove the link from.
 *  TLink          The link to remove.
 *
 *  Change Activity:
 *       Date   Developer Name    Description of Changes
 *    05-Dec-94 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLRemoveLink(TList List, TLink Link)
{
	TLinkList 	*LocalList;
	TDoubleLink 	*LocalLink;

	/*
	 * Check to see if the user gave us good pointers to the Linked List
	 * and Link to be removed.
	 */

	if (List == NULL)
		return (LLInvalidList);

	if (Link == NULL)
		return (LLInvalidLink);

	/*
	 * Typecast the void pointers to usable pointers.
	 */

	LocalList = (TLinkList *) List;
	LocalLink = (TDoubleLink *) Link;

	/*
	 * Check to see if the user gave us a correctly initialized list and
	 * link.
	 */

	if (LocalList->Initialized != LLINITIALIZED)
		return (LLInvalidList);

	if (LocalLink->Initialized != LLINITIALIZED)
		return (LLInvalidLink);

	/*
	 * If the link's next and prev pointers are already set to NULL then
	 * the link is not in use so return an error.
	 */

	if (LocalLink->Prev == NULL &&
	    LocalLink->Next == NULL &&
	    LocalList->NumberLinks != 1) {
		return (LLLinkNotInUse);
	}

	/*
	 * Check to see if the owning list is the same as the given list.
	 */

	if (LocalLink->OwningList != List) {
		return (LLInvalidList);
	}

	/*
	 * If the local link's next pointer is not NULL then set the next
	 * link's prev pointer to point at the previous link.
	 */

	if (LocalLink->Next)
		LocalLink->Next->Prev = LocalLink->Prev;

	/*
	 * If the local link's prev pointer is not NULL then set the previous
	 * link's next pointer to point at th next link.
	 */

	if (LocalLink->Prev)
		LocalLink->Prev->Next = LocalLink->Next;

	/*
	 * Now check to see if the link that was just removed was the current
	 * link and if so reset the link
	 */

	if (LocalList->Current == LocalLink) {

		/*
		 * If the prev pointer is not NULL then set the current link
		 * to point at the previous link.
		 */

		if (LocalLink->Prev != NULL)
			LocalList->Current = LocalLink->Prev;

		/*
		 * If the prev pointer is set to NULL then check to see if
		 * the next pointer is not NULL and if so set the current
		 * link to point at the next link.
		 */

		else if (LocalLink->Next != NULL)
			LocalList->Current = LocalLink->Next;

		/*
		 * If both the previous and next links were NULL then we just
		 * removed the last link so set the current link to NULL.
		 */

		else
			LocalList->Current = NULL;
	}

	/*
	 * Now check to see if the link that was just removed was the head
	 * link and if so reset the link
	 */

	if (LocalList->Head == LocalLink)
		LocalList->Head = LocalLink->Next;

	/*
	 * Finally check to see if the link that was just removed was the
	 * tail link and if so reset the link
	 */

	if (LocalList->Tail == LocalLink)
		LocalList->Tail = LocalLink->Prev;

	/*
	 * NULL out the current links prev and next pointers and set the
	 * owning list to NULL.
	 */

	LocalLink->Prev = NULL;
	LocalLink->Next = NULL;
	LocalLink->OwningList = NULL;
	LocalList->NumberLinks--;

	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLDestroyLink
 *
 * DESCRIPTION:
 *  This function will destroy the provided link.
 *
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TLink *        The pointer to the link to destroy.  Upon success
 *                 the contents of the pointer are set to NULL.
 *  TLLData *      The pointer to the data associated with the link
 *                 being destroyed.  If NULL is provided then the
 *                 data pointer is not returned.
 *
 * CHANGE ACTIVITY:
 *       Date   Developer Name    Description of Changes
 *    05-Dec-94 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLDestroyLink(TLink * Link, TLLData * Data)
{
	TDoubleLink	*LocalLink;

	/*
	 * Check to see if the user gave us a good pointer.
	 */

	if (*Link == NULL)
		return (LLInvalidLink);

	/*
	 * Typecast the void pointers to usable pointers.
	 */

	LocalLink = (TDoubleLink *) * Link;

	/*
	 * Check to see if the user gave us a correctly initialized link
	 */

	if (LocalLink->Initialized != LLINITIALIZED)
		return (LLInvalidLink);

	/*
	 * Check to make sure that the link is not in use.
	 */

	if (LocalLink->Prev != NULL || LocalLink->Next != NULL) {
		return (LLLinkInUse);
	}

	/*
	 * If the user provided a pointer to store their user data then set
	 * the poionter to point to the link data.
	 */

	if (Data) {
		*Data = LocalLink->Data;
	} else if (LocalLink->Data) {
		return (LLMemoryLeak);
	}

	/*
	 * Ok, the link is not in use so free up it's memory.
	 */

	free(*Link);
	*Link = NULL;

	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLDestroyList
 *
 * DESCRIPTION:
 *  This function destroys the provided list.
 *
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TList *        The pointer to the list to be destroyed.  Upon success
 *                 the contents of the pointer is set to NULL.
 *  TLLData *      The pointer to the data associated with the list
 *                 being destroyed.  If NULL is provided then the
 *                 data pointer is not returned.
 *
 * CHANGE ACTIVITY:
 *       Date   Developer Name    Description of Changes
 *    05-Dec-94 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLDestroyList(TList * List, TLLData * Data)
{
	TLinkList	*LocalList;

	/*
	 * Check to see if the user gave us a good pointer.
	 */

	if (*List == NULL)
		return (LLInvalidList);

	/*
	 * Typecast the void pointer to a usable pointer.
	 */

	LocalList = (TLinkList *) * List;

	/*
	 * Check to see if the user gave us a correctly initialized list
	 */

	if (LocalList->Initialized != LLINITIALIZED)
		return (LLInvalidList);

	/*
	 * Check to make sure that the list is not in use.
	 */

	if (LocalList->Head != NULL ||
	    LocalList->Current != NULL ||
	    LocalList->Tail != NULL) {
		return (LLListInUse);
	}

	/*
	 * If the user provided a pointer to store their user data then set
	 * the poionter to point to the list data.
	 */

	if (Data) {
		*Data = LocalList->Data;
	} else if (LocalList->Data) {
		return (LLMemoryLeak);
	}

	/*
	 * Ok, the list is not in use so free up it's memory.
	 */

	free(*List);
	*List = NULL;

	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLUpdateCurrent
 *
 * DESCRIPTION:
 *  This function will update the internal current link pointer based
 *  on the provided operation.
 *
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TList          The list whose current link pointer is to be modified.
 *
 *  TLLOperation   The operation to apply to determine the current link.
 *                 The values are:
 *                   LLPrev    - Set the current pointer to be the previous
 *                               link.
 *                   LLNext    - Set the current pointer to be the next
 *                               link.
 *                   LLHead    - Set the current pointer to be the
 *                               beginning of the list.
 *                   LLTail    - Set the current pointer to be the
 *                               end of the list.
 * CHANGE ACTIVITY:
 *       Date   Developer Name    Description of Changes
 *    05-Dec-94 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLUpdateCurrent(TList List, TLLOperation Operation)
{
	TLinkList	*LocalList;

	/*
	 * Check to see if the user gave us a good pointer.
	 */

	if (List == NULL)
		return (LLInvalidList);

	/*
	 * Typecast the void pointer to a usable pointer.
	 */

	LocalList = (TLinkList *) List;

	/*
	 * Check to see if the user gave us a correctly initialized list
	 */

	if (LocalList->Initialized != LLINITIALIZED)
		return (LLInvalidList);

	/*
	 * If there are no links in the list then return the List Empty
	 * error.
	 */

	if (LocalList->NumberLinks == 0)
		return (LLListEmpty);

	/*
	 * Set the current link pointer to the location specified by the
	 * user.
	 */

	switch (Operation) {
	case LLHead:
		LocalList->Current = LocalList->Head;
		break;
	case LLTail:
		LocalList->Current = LocalList->Tail;
		break;
	case LLPrev:
		LocalList->Current = LocalList->Current->Prev;
		if (LocalList->Current == NULL) {
			LocalList->Current = LocalList->Head;
			return (LLBeginningOfList);
		}
		break;
	case LLNext:
		LocalList->Current = LocalList->Current->Next;
		if (LocalList->Current == NULL) {
			LocalList->Current = LocalList->Tail;
			return (LLEndOfList);
		}
		break;
	default:
		return (LLInvalidOperation);
	}
	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLGetLinkData
 *
 * DESCRIPTION:
 *
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TList          The list to retrieve the link from.
 *  TLLOperation   The operation to apply to determine the link to retrieve.
 *                 The values are:
 *                   LLPrev    - Retrieve the previous link.
 *                   LLNext    - Retrieve the next link.
 *                   LLHead    - Retrieve the first link in the list.
 *                   LLTail    - Retrieve the last link in the list.
 *  TLink *        The pointer to the link being retrieved.  Upon success
 *                 the contents will be set to the retrieved link.
 *  TLLData *      The pointer to the data associated with the link being
 *                 retrieved.
 *
 * CHANGE ACTIVITY:
 *       Date   Developer Name    Description of Changes
 *    05-Dec-94 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLGetLinkData(TList List,
    TLLOperation Operation,
    TLink * Link,
    TLLData * Data)
{
	TLinkList 	*LocalList;
	TLLError	LLError;

	/*
	 * Do the operation requested to the list
	 */

	if ((LLError = LLUpdateCurrent(List, Operation))) {
		return (LLError);
	}

	/*
	 * Check to see if the user gave us a good pointer.
	 */

	if (List == NULL)
		return (LLInvalidList);

	/*
	 * Typecast the void pointer to a usable pointer.
	 */

	LocalList = (TLinkList *) List;

	/*
	 * Check to see if the user gave us a correctly initialized list
	 */

	if (LocalList->Initialized != LLINITIALIZED)
		return (LLInvalidList);

	/*
	 * Check to see if the list is empty.
	 */

	if (LocalList->NumberLinks < 1) {
		return (LLListEmpty);
	}

	/*
	 * If the user gave us a non-NULL pointer then assign the pointer.
	 */

	if (Link) {
		*Link = LocalList->Current;
	}

	/*
	 * If the user gave us a non-NULL pointer then assign the pointer.
	 */

	if (Data) {
		*Data = LocalList->Current->Data;
	}
	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLGetCurrentLinkData
 *
 * DESCRIPTION:
 *  This function will return the current link along with the data
 *  associated with the link.
 *
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TList          The list to retrieve the current link from.
 *  TLink *        The pointer to the link being retrieved.  Upon success
 *                 the contents of the pointer will be set to the retrieved
 *                 link.
 *  TLLData *      The pointer to the data associated with the link being
 *                 retrieved.
 *
 * CHANGE ACTIVITY:
 *       Date   Developer Name    Description of Changes
 *    05-Dec-94 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLGetCurrentLinkData(TList List,
    TLink * Link,
    TLLData * Data)
{
	TLinkList	*LocalList;

	/*
	 * Check to see if the user gave us a good pointer.
	 */

	if (List == NULL)
		return (LLInvalidList);

	/*
	 * Typecast the void pointer to a usable pointer.
	 */

	LocalList = (TLinkList *) List;

	/*
	 * Check to see if the user gave us a correctly initialized list
	 */

	if (LocalList->Initialized != LLINITIALIZED)
		return (LLInvalidList);

	/*
	 * Check to see if the list is empty.
	 */

	if (LocalList->NumberLinks < 1)
		return (LLListEmpty);

	if (Link) {
		*Link = LocalList->Current;
	}
	if (Data) {
		*Data = LocalList->Current->Data;
	}

	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLGetSuppliedListData
 *
 * DESCRIPTION:
 *  This function will return the data associated with the supplied
 *  list along with the number of link in the list.
 *
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TList          The list of interest.
 *  int *          A pointer to the number of links in the list.  Upon
 *                 successful completion, the contents of the pointer is
 *                 set to the number of links in the provided list.
 *  TLLData *      A pointer to the data associated with the provided
 *                 list.  Upon successful completion, the contents of the
 *                 pointer are set to the data associated with the list.
 *
 * CHANGE ACTIVITY:
 *       Date   Developer Name    Description of Changes
 *    30-Sep-95 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLGetSuppliedListData(TList List,
    int *NumberLinks,
    TLLData * Data)
{
	TLinkList	*LocalList;

	/*
	 * Check to see if the user gave us a NULL pointer.
	 */

	if (List == NULL) {
		return (LLInvalidList);
	}

	/*
	 * Reassign the passed pointer to the internal representation of a
	 * list.
	 */

	LocalList = (TLinkList *) List;

	/*
	 * Ok, the pointer given us by the user is not NULL but it still may
	 * be bogus.  So check to see if it has been initialized.
	 */

	if (LocalList->Initialized != LLINITIALIZED) {
		return (LLInvalidList);
	}
	if (NumberLinks) {
		*NumberLinks = LocalList->NumberLinks;
	}
	if (Data) {
		*Data = LocalList->Data;
	}

	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLGetSuppliedLinkData
 *
 * DESCRIPTION:
 *  This function returns the data associated with the supplied link.
 *
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TLink          The link of interset.
 *  TLLData *      The pointer to the data associated with the link being
 *                 retrieved.
 *
 * CHANGE ACTIVITY:
 *       Date   Developer Name    Description of Changes
 *    05-Dec-94 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLGetSuppliedLinkData(TLink Link, TLLData *Data)
{
	TDoubleLink	*LocalLink;

	/*
	 * Check to see if the user gave us a good pointer.
	 */

	if (Link == NULL)
		return (LLInvalidLink);

	/*
	 * Typecast the void pointer to a usable pointer.
	 */

	LocalLink = (TDoubleLink *) Link;

	/*
	 * Check to see if the user gave us a correctly initialized list
	 */

	if (LocalLink->Initialized != LLINITIALIZED)
		return (LLInvalidLink);

	if (Data) {
		*Data = LocalLink->Data;
	}
	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLSortList
 * DESCRIPTION:
 *   This function takes in a Link List and sorts it based upon the
 *   return values of the provided compare function.
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TList          The list to be sorted.
 *  <Callback>     This is the users compare callback.  It will be
 *                 invoked for each link in the list to determine
 *                 where the current link of the old list should be
 *                 inserted relative to the current link of the new
 *                 list.
 *  void *         A pointer to user data that will be passed to the
 *                 callback as the first argument when invoked.
 *
 *  Change Activity:
 *       Date   Developer Name    Description of Changes
 *    15-May-96 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLSortList(TList ListToSort,
    TLLCompare(*Compare) (void *, TLLData, TLLData),
    void *UserPtr)
{
	TList		SortedList;
	TLink 		SortedLink;

	TLLError	LLError;
	TLLCompare	LLCompare;
	TLink		CurrentLink;

	TLLData		SortedData;
	TLLData		LinkData;

	TBoolean	Done;
	TBoolean 	Found;

	/*
	 * Create a temporary list to do the alphabetic sorting
	 */

	if ((LLError = LLCreateList(&SortedList, NULL))) {
		return (LLError);
	}

	/*
	 * Set to the beginning of the list to sort
	 */

	LLError = LLUpdateCurrent(ListToSort, LLHead);
	switch (LLError) {
	case LLSuccess:
		break;
	case LLListEmpty:
		return (LLSuccess);
	default:
		return (LLError);
	}

	/*
	 * For all of the Entries in the Directory
	 */

	Done = False;
	while (!Done) {

		/*
		 * Get the current entry from the list.
		 */

		if ((LLError = LLGetCurrentLinkData(ListToSort,
			    &CurrentLink,
			    (void **) &LinkData))) {
			return (LLError);
		}

		/*
		 * Remove the link from the list to sort.
		 */

		if ((LLError = LLRemoveLink(ListToSort,
			    CurrentLink))) {
			return (LLError);
		}

		/*
		 * Now, let's insert the link into the sorted list.  The
		 * first step is to set the sorted list to the first enrty.
		 */

		LLError = LLUpdateCurrent(SortedList, LLHead);
		switch (LLError) {
		case LLSuccess:

			/*
			 * While the insertion point for the current link has
			 * not been found
			 */

			Found = False;
			while (!Found) {

				/*
				 * Get the current link from the sorted list.
				 */

				if ((LLError = LLGetCurrentLinkData(SortedList,
					    &SortedLink,
					    (void **) &SortedData))) {
					return (LLError);
				}

				/*
				 * Call the callback to determine if the
				 * insert point has been found.
				 */

				LLCompare = Compare(UserPtr,
						    LinkData,
						    SortedData);
				switch (LLCompare) {
				case LLCompareLess:
				case LLCompareEqual:

					/*
					 * We found the insertion point, so
					 * add the link and were done
					 */

					if ((LLError = LLAddLink(SortedList,
						    CurrentLink,
						    LLPrev))) {
						return (LLError);
					}
					Found = True;
					break;
				case LLCompareGreater:
					break;
				case LLCompareError:
			defualt:
					return (LLCallbackError);
				}

				/*
				 * Otherwise, we need to continue down the
				 * list to find the insertion point.  So
				 * update to the next link in the sorted
				 * list.
				 */

				LLError = LLUpdateCurrent(SortedList, LLNext);
				switch (LLError) {
				case LLSuccess:
					break;
				case LLEndOfList:

					/*
					 * We hit the end of the list before
					 * finding an insertion point so
					 * insert the link at the end of the
					 * list
					 */

					if ((LLError = LLAddLink(SortedList,
						    CurrentLink,
						    LLTail))) {
						return (LLError);
					}
					Found = True;
					break;
				default:
					return (LLError);
				}
			}
			break;
		case LLListEmpty:

			/*
			 * Since the list is empty, this is the first entry
			 * so just add it to the sorted list
			 */

			if ((LLError = LLAddLink(SortedList,
				    CurrentLink,
				    LLHead))) {
				return (LLError);
			}
			break;
		default:
			return (LLError);
		}


		/*
		 * Update to the head of the list.
		 */

		LLError = LLUpdateCurrent(ListToSort, LLHead);
		switch (LLError) {
		case LLSuccess:
			break;
		case LLEndOfList:
		case LLListEmpty:
			Done = True;
			break;
		default:
			return (LLError);
		}
	}

	/*
	 * Finally, lets copy the sorted list back into the original list
	 */

	if ((LLError = LLGetLinkData(SortedList,
		    LLHead,
		    &SortedLink,
		    (void **) &SortedData))) {
		return (LLError);
	}
	Done = False;
	while (!Done) {

		/*
		 * Remove the link from the sorted list.
		 */

		if ((LLError = LLRemoveLink(SortedList,
			    SortedLink))) {
			return (LLError);
		}

		/*
		 * Add the link back into the directory entry list.
		 */

		if ((LLError = LLAddLink(ListToSort,
			    SortedLink,
			    LLNext))) {
			return (LLError);
		}

		/*
		 * Set to the head of the list
		 */

		LLError = LLGetLinkData(SortedList,
		    LLHead,
		    &SortedLink,
		    (void **) &SortedData);

		switch (LLError) {
		case LLSuccess:
			break;
		case LLListEmpty:
		case LLEndOfList:
			Done = True;
			break;
		default:
			return (LLError);
		}
	}

	/*
	 * Blow away the temporary list
	 */

	if ((LLError = LLDestroyList(&SortedList, NULL))) {
		return (LLError);
	}
	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLClearList
 * DESCRIPTION:
 *  This function allows the calling application to clear the contents
 *  of the link list.  The calling application must provide a callback
 *  that can be invoked to do any clean up prior to destroying a link
 *  within the list.
 * RETURN:
 *  TYPE           DESCRIPTION
 *  TLLError       This is the enumerated error
 *                 code defined in the public
 *                 header.  Upon success, LLSuccess
 *                 will be returned.  Upon error,
 *                 the appropriate error code will
 *                 returned.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TList          The list to remove all of the links from.
 *  <Callback>     The callback to call with the data pointer
 *                 for the link about to be removed and destroyed.
 *
 *  Change Activity:
 *       Date   Developer Name    Description of Changes
 *    15-May-96 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

TLLError
LLClearList(TList List,
    TLLError(*CleanUp) (TLLData Data))
{

	TLink		CurrentLink;
	TLLData 	LLData;
	TLLError	LLError;
	TBoolean	Done;

	/*
	 * Set up the current link to be the head of the list.
	 */

	LLError = LLUpdateCurrent(List, LLHead);
	switch (LLError) {
	case LLSuccess:
		break;
	case LLListEmpty:
		return (LLSuccess);
	default:
		return (LLError);
	}

	/*
	 * For all of the links in the list.
	 */

	Done = False;
	while (!Done) {

		/*
		 * Get the current Link's data.
		 */

		LLError = LLGetCurrentLinkData(List,
		    &CurrentLink,
		    (void **) &LLData);

		/*
		 * Since I am removing links from the list and I started with
		 * the head of the list, as I remove each link the current
		 * pointer gets set to the next link in the list.  So all I
		 * need to do is continue to get the current link until the
		 * return code is LLListEmpty.
		 */

		switch (LLError) {
		case LLSuccess:

			/*
			 * Remove the link from the list
			 */

			if ((LLError = LLRemoveLink(List, CurrentLink))) {
				return (LLError);
			}

			/*
			 * Destroy the Link
			 */

			if ((LLError = LLDestroyLink(&CurrentLink,
			    (void **)&LLData))) {
				return (LLError);
			}

			/*
			 * Call the calling applications callback to clean up
			 * the data pointer
			 */

			if (CleanUp(LLData) != LLSuccess) {
				return (LLCallbackError);
			}
			break;
		case LLListEmpty:
			return (LLSuccess);
		default:
			return (LLError);
		}
	}
	return (LLSuccess);
}

/*
 * *********************************************************************
 * MODULE NAME: LLLinkListErrorString
 * DESCRIPTION:
 *  This function converts the enumerated error value passed back by
 *  the Link List Functions into a NULL terminated string.
 * RETURN:
 *  TYPE           DESCRIPTION
 *  char *         The ASCII string for the provided enumerated error.
 *
 * PARAMETERS:
 *  TYPE           DESCRIPTION
 *  TLLError       The error code of interest.
 *
 * CHANGE ACTIVITY:
 *       Date   Developer Name    Description of Changes
 *    05-Dec-94 Craig Vosburgh    Creation of Module
 * *********************************************************************
 */

char *
LLErrorString(TLLError Error)
{

	/*
	 * Case on the enumerated Error value.
	 */

	switch (Error) {
		case LLSuccess:
		return ("Successful Completion");
	case LLMemoryAllocationError:
		return ("Unable to Allocate Necessary Memory");
	case LLInvalidList:
		return
		    ("The List supplied was not initialized or is corrupted");
	case LLInvalidLink:
		return
		    ("The Link supplied was not initialized or is corrupted");
	case LLInvalidOperation:
		return ("Invalid operation specified");
	case LLLinkNotInUse:
		return
		    ("Supplied Link is currently not used in a Link List");
	case LLLinkInUse:
		return
		    ("Supplied Link is currently used in a Link List");
	case LLListInUse:
		return
		    ("Supplied List contains links.  Remove all links \
before destroying");
	case LLBeginningOfList:
		return ("Beginning of Link List reached");
	case LLEndOfList:
		return ("Endof Link List reached");
	case LLCallbackError:
		return ("The supplied callback returned a non-zero");
	default:
		return ("Invalid Link List Error.");
	}
}
