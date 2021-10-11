#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)queue.cc	1.41 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/queue.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)queue.cc  1.41  91/05/05
 *
 *  Copyright (c) 1989 Sun Microsystems, Inc.  All Rights Reserved.
 *  Sun considers its source code as an unpublished, proprietary trade 
 *  secret, and it is available only under strict license provisions.  
 *  This copyright notice is placed here only to protect Sun in the event
 *  the source is deemed a published work.  Dissassembly, decompilation, 
 *  or other means of reducing the object code to human readable form is 
 *  prohibited by the license agreement under which this code is provided
 *  to the user or company in possession of this copy.
 * 
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
 *  Government is subject to restrictions as set forth in subparagraph 
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software 
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
 *  NASA FAR Supplement.
 *
 *  Comments:	queue manipulation routines
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* ------------------------------------------------------------------
 *  NetmgtQueue::reset - reset queue
 *     no return value
 * ------------------------------------------------------------------
 */
void
NetmgtQueue::reset (void)
{
  NETMGT_PRN (("queue: NetmgtQueue::reset\n"));

  this->curr = this->head;
  this->isFirstIter = TRUE;
  return;
}

/* ------------------------------------------------------------------
 *  NetmgtQueue::append - append new node to queue
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
NetmgtQueue::append (caddr_t data, int trim)
                  		// data pointer 
                 		// whether trim queue 
{
  sigset_t osigmask;		// previous signal mask 
  NetmgtQueueNode *newNode;	// new queue node pointer 

  NETMGT_PRN (("queue: NetmgtQueue::append\n"));

  assert (data != (caddr_t) NULL);

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // don't exceed queue limit 
  if (this->limit && this->length == this->limit)
    {
      // remove the node at the queue head if the trim flag is set 
      if (trim)
	{
	  NETMGT_PRN (("queue: trimming queue\n"));

	  // free queue node data 
	  if (this->head->isData ())
	    {
	      NETMGT_PRN (("queue: freeing queue node data\n"));
	      this->head->freeData ();
	    }

	  // free queue node  
	  if (!this->remove (this->head))
	    return FALSE;
	}
      // otherwise, return an error 
      else
	{
	  NETMGT_PRN (("queue: would exceed queue limit\n"));
	  _netmgtStatus.setStatus (NETMGT_ATQUEUELIMIT, 0, NULL);
	  return FALSE;
	}
    }

  // allocate new queue node 
  if (!alloc (&newNode))
    return FALSE;

  // mark the node "fresh"
  newNode->stale = FALSE;

  // block signals while in critical section 
  osigmask = newNode->blockSignals ();

  // append new queue node to queue 
  if (isEmpty ())
    {
      this->head = newNode;
      this->tail = newNode;
      newNode->setPrev ((NetmgtQueueNode *) NULL);
      newNode->setNext ((NetmgtQueueNode *) NULL);
    }
  else
    {
      this->tail->setNext (newNode);
      newNode->setPrev (this->tail);
      this->tail = newNode;
    }
  newNode->setData (data);

  // increment queue length 
  this->length++;

  // unblock signals 
  newNode->unblockSignals (osigmask);

  return TRUE;
}

/* -------------------------------------------------------------------
 *  NetmgtQueue::alloc - allocate queue node and return node pointer
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtQueue::alloc (NetmgtQueueNode **pnewNode)
                           	// address of queue node pointer 
{

  NETMGT_PRN (("queue: NetmgtQueue::alloc\n"));

  assert (pnewNode != (NetmgtQueueNode **) NULL);

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // allocate a new queue node 
  *pnewNode = (NetmgtQueueNode *) calloc (1, sizeof (NetmgtQueueNode));
  if (!*pnewNode)
    {
      if (netmgt_debug)
	perror ("queue: calloc failed");
      _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
      return FALSE;
    }
  
    return TRUE;
}

/* --------------------------------------------------------------------
 *  NetmgtQueue::myDestructor - destroy queue (doesn't free queue data)
 *      returns TRUE if successful; otherwise returns FALSE
 * --------------------------------------------------------------------
 */
bool_t
NetmgtQueue::myDestructor (void)
{

  NETMGT_PRN (("queue: NetmgtQueue::myDestructor\n")) ;

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  // repeatedly remove the first queue node until
  // the queue is empty
  while (this->head)
    {
      // free queue node 
      if (!this->remove (this->head))
	return FALSE;
    }
  assert (this->length == (u_int) 0);
  this->isFirstIter = TRUE;

  return TRUE;
}

/* ------------------------------------------------------------------
 *  NetmgtQueue::remove - remove node from queue
 *	returns TRUE if successful; otherwise returns FALSE
 * ------------------------------------------------------------------
 */
bool_t
NetmgtQueue::remove (NetmgtQueueNode *pnode)
                          	// pointer to node to remove 
{
  sigset_t osigmask;		// previous signal mask 
  NetmgtQueueNode *prevNode;	// previous node pointer 
  NetmgtQueueNode *nextNode;	// next node pointer 
  NetmgtQueueNode *currNode;	// current node pointer 

  NETMGT_PRN (("queue: NetmgtQueue::remove\n"));

  assert (pnode != (NetmgtQueueNode *) NULL);

  // reset internal error buffer 
  _netmgtStatus.clearStatus ();

  if (isEmpty ())
    {
      NETMGT_PRN (("queue: empty queue\n"));
      _netmgtStatus.setStatus (NETMGT_EMPTYQUEUE, 0, NULL);
      return FALSE;
    }

  // block signals while in critical section 
  osigmask = currNode->blockSignals ();

  // detach the node from the queue 
  prevNode = pnode->getPrev ();
  nextNode = pnode->getNext ();
  if (prevNode)
    prevNode->setNext (nextNode);
  if (nextNode)
    nextNode->setPrev (prevNode);

  // is the queue empty 
  if (--this->length == 0)
    {
     this->head = (NetmgtQueueNode *) NULL;
     this->tail = (NetmgtQueueNode *) NULL;
   }

  // reset head pointer if detached node was at beginning of the queue
  if (this->head == pnode)
    this->head = this->head->getNext ();

  // reset tail pointer if detached node was at end of the queue 
  if (this->tail == pnode)
    {
      currNode = this->head;
      while (currNode->isNext ())
	currNode = currNode->getNext ();
      this->tail = currNode;
    }

  // free queue node data
  if (pnode->isData ())
    {
      NETMGT_PRN (("queue: freeing queue node data\n"));
      pnode->freeData ();
    }

  // free queue node 
  (void) cfree ((caddr_t) pnode);
  pnode = (NetmgtQueueNode *) NULL;

  // unblock signals 
  currNode->unblockSignals (osigmask);

  return TRUE;
}

/*----------------------------------------------------------------------
 *  NetmgtQueue::getNext - get next queue node
 *    returns next queue node if there is one; otherwise NULL
 * ---------------------------------------------------------------------
 */
NetmgtQueueNode *
NetmgtQueue::getNext (void)
{
  static NetmgtQueueNode *nextNode ; // next node pointer

  NETMGT_PRN (("NetmgtQueue::getNext\n"));

  if (this->isFirstIter)
    {
      this->curr = this->head;
      this->isFirstIter = FALSE;
    }
  else
    this->curr = this->curr->getNext ();

  nextNode = this->curr;
  return nextNode;
}
