
/**************************************************************************
 *  File:	include/libnetmgt/queue.h
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
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
 *  SCCSID:	@(#)queue.h  1.35  91/01/31
 *
 *  Comments:	queue node and queue classes
 *
 **************************************************************************
 */
#ifndef _queue_h
#define _queue_h

#ifdef _SVR4_
#include <malloc.h>
#endif

// forward declarations
class NetmgtQueue ;
class NetmgtQueueNode ;

// queue node class
class NetmgtQueueNode: public NetmgtContainer
 {

  // friends ---------------------------------------------------------------
  friend NetmgtQueue;

  // public access functions
public:

  // get next queue node
  NetmgtQueueNode *getNext (void)  { return this->next; }

  // get previous queue node
  NetmgtQueueNode *getPrev (void)  { return this->prev; }

  // get queue node data
  caddr_t getData (void)  { return this->data; }

  // is there data ?
  bool_t isData (void)  { return this->data != (caddr_t) NULL; }

  // is there a next queue node ?
  bool_t isNext (void)  { return this->next != (NetmgtQueueNode *) NULL; }

  // is there a previous queue node ?
  bool_t isPrev (void)  { return this->prev != (NetmgtQueueNode *) NULL; }

  // is the node stale ?
  bool_t isStale (void)  { return this->stale; }


  // public update functions
public:

  // mark queue node stale
  void markStale (void)  { this->stale = TRUE; }


  // private update functions
private:

  // free data
  void freeData (void)
    {
      (void) cfree (data);
      this->data = (caddr_t) NULL;
    }

  // set queue node data
  void setData (caddr_t set_data)  { this->data = set_data; }

  // set next queue node pointer
  void setNext (NetmgtQueueNode *set_next)  { this->next = set_next; }

  // set previous queue node pointer
  void setPrev (NetmgtQueueNode *set_prev)  { this->prev = set_prev; }


// private variables
private:

  NetmgtQueueNode *next ;	// next queue node
  NetmgtQueueNode *prev ;	// previous queue node
  bool_t stale ;		// whether queue node is stale
  caddr_t data ;		// queue node data pointer

} ;



// queue class 
class NetmgtQueue
{

  // public instantiation functions
public:

  // initialize queue instance
  bool_t myConstructor (u_int mylimit)
    {
      this->head = (NetmgtQueueNode *) NULL;
      this->tail = (NetmgtQueueNode *) NULL;
      this->limit = mylimit;
      this->length = (u_int) 0;
      this->curr = (NetmgtQueueNode *) NULL;
      this->isFirstIter = TRUE;
      return TRUE;
    }

  // destroy queue instance
  bool_t myDestructor (void) ;


  // public access functions
public:

  // get queue head
  NetmgtQueueNode *getHead (void)  { return this->head; }

  // get next node
  NetmgtQueueNode *getNext (void) ;

  // get queue tail
  NetmgtQueueNode *getTail (void)  { return this->tail; }

  // get queue length
  u_int getLength (void)  { return this->length; }

  // get queue length limit
  u_int getLimit (void)  { return this->limit; }

  // is queue empty
  bool_t isEmpty (void)  { return this->head == (NetmgtQueueNode *) NULL; }


  // public update functions
public:

  // append node to queue
  bool_t append (caddr_t data, int trim) ;

  // remove queue node
  bool_t remove (NetmgtQueueNode *pnode) ;

  // reset queue
  void reset (void) ;

  // private update functions
private:
  // allocate queue node
  bool_t alloc (NetmgtQueueNode **ppnode) ;

  // free queue node
  bool_t freeNode (NetmgtQueueNode *pnode) ;


  // private varibles
private:

  NetmgtQueueNode *head ;	// queue head
  NetmgtQueueNode *tail ;	// queue tail
  u_int limit ;			// queue length limit
  u_int length ;		// queue length
  NetmgtQueueNode *curr ;	// current queue node (for iterating)
  bool_t isFirstIter ;		// whether first iteration

} ;

#endif  _queue_h
