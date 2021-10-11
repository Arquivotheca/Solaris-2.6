/***************************************************************************
*                                                                          *
* Copyright 1993 Adaptec, Inc.,  All Rights Reserved.                      *
*                                                                          *
* This software contains the valuable trade secrets of Adaptec.  The       *
* software is protected under copyright laws as an unpublished work of     *
* Adaptec.  Notice is for informational purposes only and does not imply   *
* publication.  The user of this software may make copies of the software  *
* for use with parts manufactured by Adaptec or under license from Adaptec *
* and for no other use.                                                    *
*                                                                          *
***************************************************************************/
/*
 * Copyright (c) 1995 Sun Microsystems, Inc.
 * All Rights Reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
#pragma ident	"@(#)custom.h	1.5	95/07/05 SMI"

/* $Header:   V:/source/code/aic-7870/him/common/custom.hvt   1.8   17 Aug 1994 14:20:26   HYANG  $ */

/***************************************************************************
*
*  Module Name:   CUSTOM.H
*
*  Description:
*						Customized operating specific definitions 
*
*  Programmers:   Paul von Stamwitz
*                 Chuck Fannin
*                 Jeff Mendiola
*                 Harry Yang
*    
*  Notes:         NONE
*
*  Revisions -
*
***************************************************************************/

#define _DRIVER

/* default to IO map */
#ifndef  _IOMAP
#ifndef  _MEMAP
#define  _IOMAP
#endif
#endif 

/****************************************************************************
*                            %%% Type Sizes %%%
****************************************************************************/
#define UBYTE     unsigned char        /* Must be 8 bits                   */
#define WORD      short int            /* Must be 16 bits                  */
#define UWORD     unsigned short 	/* Must be 16 bits                  */
#define DWORD     unsigned long int    /* Must be 32 bits                  */
#define OS_TYPE   DWORD

/****************************************************************************
*                      %%% Run-Time Library Functions %%%
****************************************************************************/

/****************************************************************************
* device programming could be either memory mapped programming or
* IO mappoed programming. The default is IO mapped programming
****************************************************************************/
#ifdef _MEMMAP
/* TBD */
#define FAR                            /* defined if far pointer necessary */
#endif

#ifdef _IOMAP
#define INBYTE    inb                  /* read byte from port              */
#define INDWORD   inl                 /* read double word from port       */
#define OUTBYTE   outb                 /* write byte to port               */
#define OUTDWORD  outl                /* write double word to port        */
/*
extern DWORD INDWORD(UWORD port);
extern DWORD OUTDWORD(UWORD port, DWORD value);
*/
#endif

/****************************************************************************
* macro defined for configuration space programming 
* becaues it is always io mapped programming
****************************************************************************/
#define GETBYTE   inb                  /* get byte from port               */
#define GETDWORD  inl                 /* get double word from port        */
#define PUTBYTE   outb                 /* put byte to port                 */
#define PUTDWORD  outl                /* put double word to port          */
/*
extern DWORD GETDWORD(UWORD port);
extern DWORD PUTDWORD(UWORD port, DWORD value);
*/

/* the next five defines are no ops for an OS like Solaris		    */
#define INTR_OFF  			/* disable system interrupts        */
#define INTR_ON   			/* enable system interrupts         */
#define INTR_SAVE
#define INTR_OFF_SAV
#define INTR_RESTORE

#define Seq_00    P_Seq_00             /* Resolve name conflict with arrow */
#define Seq_01    P_Seq_01             /* Resolve name conflict with arrow */
#define SeqExist  P_SeqExist           /* Resolve name conflict with arrow */ 
#define MAX_INTRLOOP    -1             /* Maximum intr service loop count  */      
