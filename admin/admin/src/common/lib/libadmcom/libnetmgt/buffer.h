
/**************************************************************************
 *  File:	include/libnetmgt/buffer.h
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
 *  SCCSID:	@(#)buffer.h  1.31  90/12/05
 *
 *  Comments:	buffer container class
 *
 **************************************************************************
 */
#ifndef _buffer_h
#define _buffer_h

// buffer class
class NetmgtBuffer: public NetmgtContainer
{

  // public access functions
public:

  // byte copy from buffer
  bool_t copyOut (caddr_t to, u_int length) ;

  // get buffer base address
  caddr_t getBase () { return this->base;}
  
  // get buffer size
  u_int getSize (void) { return this->size; }

  // get read/write offset
  off_t getOffset (void)  { return this->offset; }

  // get read/write pointer
  caddr_t getPtr (void) { return caddr_t (this->base + this->offset); }


  // public update functions
public:

  // byte copy to buffer
  bool_t copyIn (caddr_t from, u_int length) ;

  // deallocate buffer
  bool_t dealloc (void) ;

  // set read/write offset
  void setOffset (off_t set_offset) { this->offset = set_offset; }

  // reset read/write pointer
  void resetPtr (void)  { this->offset = (off_t) 0; }

  // set buffer state
  bool_t setBuffer (caddr_t newBase, u_int newSize, off_t newOffset) ;

  // increment read/write pointer
  void incrPtr (off_t increment)  { this->offset += increment; }


  // public manipulation functions
public:

  // allocate buffer space
  bool_t alloc (u_int length, u_int min) ;


  // private variables
private:

  caddr_t base ;		// buffer base address
  u_int size ;			// buffer size
  off_t offset ;		// buffer read/write offset

} ;

#endif  _buffer_h
