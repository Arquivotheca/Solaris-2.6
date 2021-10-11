#if !defined(lint) && !defined(NOID)
  static char sccsid[] = "@(#)buffer.cc	1.39 5/5/91 SMI";
#endif

/**************************************************************************
 *  File:	lib/libnetmgt/buffer.cc
 *
 *  Author:	Lynn Monsanto, Sun Microsystems Inc.
 *
 *  SCCSID:     @(#)buffer.cc  1.39  91/05/05
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
 *  Comments:	buffer class member functions
 *
 **************************************************************************
 */

/*LINTLIBRARY*/

#include "netmgt/netmgt_hdrs.h"
#include "netmgt/netmgt.h"
#include "libnetmgt/libnetmgt.h"

/* -------------------------------------------------------------------
 *  _netmgt_buffer::alloc - allocate buffer space 
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtBuffer::alloc (u_int length, u_int min)
                   		// requested buffer length 
                      		// allocation increaments 
{
  u_int newBlocks ;		// #blocks to allocate 
  u_int newSize ;		// new buffer size 
  caddr_t newBase ;		// new buffer base 

  NETMGT_PRN (("buffer: NetmgtBuffer::alloc %d bytes\n", length)) ;
  NETMGT_PRN (("buffer: this->base: 0x%x, this->size %d, this->offset %d\n",
	       this->base, this->size, this->offset));

  // allocate initial buffer space 
  if (this->size == 0)
    {
      newBlocks = u_int (length / min + 1);
      newSize = u_int (newBlocks * min);

      this->base = (caddr_t) calloc (1, newSize);
      if (!this->base)
	{
	  if (netmgt_debug)
	    perror ("buffer: calloc failed");
	  _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
	  return FALSE;
	}
      this->size = newSize;
      NETMGT_PRN (("buffer: calloc: length: %d offset: %d\n", this->size, 
		   this->offset));
      return TRUE;
    }

  // no need to allocate more space 
  else if (this->offset + length < this->size)
    {
      NETMGT_PRN (("buffer: calloc: no allocation: length: %d offset: %d\n", 
		   this->size, this->offset));
      return TRUE;
    }

  // reallocate more buffer space 
  else
    {
      newBlocks = u_int ((this->offset + length) / min + 1);
      newSize = u_int (newBlocks * min);

      newBase = (caddr_t) realloc (this->base, newSize);
      if (!newBase)
	{
	  if (netmgt_debug)
	    perror ("buffer: realloc failed");

	  _netmgtStatus.setStatus (NETMGT_MALLOC, 0, strerror (errno));
	  return FALSE;
	}
      this->base = newBase;
      this->size = newSize;
      NETMGT_PRN (("buffer: realloc: length: %d offset: %d\n", this->size, 
		   this->offset));
      return TRUE;
    }
}

/* -------------------------------------------------------------------
 *  _netmgt_buffer::dealloc - deallocate buffer space 
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtBuffer::dealloc (void)
{

  NETMGT_PRN (("buffer: NetmgtBuffer::dealloc\n"));
  
  (void) cfree (this->base);
  this->base = (caddr_t) NULL;
  this->size = (u_int) 0;
  this->offset = (off_t) 0;
  return TRUE;
}

/* -------------------------------------------------------------------
 *  _netmgt_buffer::setBuffer - set buffer state
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtBuffer::setBuffer (caddr_t newBase, u_int newSize, off_t newOffset)
{

  NETMGT_PRN (("buffer: NetmgtBuffer::setBuffer\n"));
  
  this->base = newBase;
  this->size = newSize;
  this->offset = newOffset;
  NETMGT_PRN (("buffer: this->base: 0x%x, this->size %d, this->offset %d\n",
	       this->base, this->size, this->offset));
  return TRUE;
}

/* -------------------------------------------------------------------
 *  _netmgt_buffer::copyIn - byte copy to buffer
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtBuffer::copyIn (caddr_t from, u_int length)
{

  assert (from != (caddr_t) NULL);

  // make sure we have enought space to copy into buffer
  if (!this->alloc (length, NETMGT_MINARGSIZ))
    return FALSE;

  (void) memcpy (this->base + this->offset, from, length) ;
  return TRUE;
}

/* -------------------------------------------------------------------
 *  _netmgt_buffer::copyOut - byte copy from buffer
 *	returns TRUE if successful; otherwise returns FALSE
 * -------------------------------------------------------------------
 */
bool_t
NetmgtBuffer::copyOut (caddr_t to, u_int length)
{

  assert (to != (caddr_t) NULL);

  if (length > this->size)
    return FALSE;

  (void) memcpy (to, this->base + this->offset, length) ;
  return TRUE;
}

