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
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
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
#pragma ident	"@(#)custom.h	1.13	96/03/13 SMI"

/* $Header:   V:/source/code/aic-7870/him/common/custom.hvt   1.8   17 Aug 1994 14:20:26   HYANG  $ */

/***************************************************************************
*
*  Module Name:   CUSTOM.H
*
*  Description:
*		  Customized operating specific definitions 
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

#include <sys/scsi/scsi.h>
#include <sys/debug.h>
#include <sys/pci.h>
#include <sys/ddi.h>

#if defined(COMMON_IO_EMULATION)
#include <sys/xpci/sunddi_2.5.h>
#else
#include <sys/sunddi.h>
#endif

#define _DRIVER
/*
 * _NO_CONFIG is defined to disable him code that thinks it knows
 * how to do PCI configuration.
 */
#define _NO_CONFIG

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
	#define	INBYTE(port)		ddi_io_getb(config_ptr->ab_handle, port)
	#define	OUTBYTE(port, value)	ddi_io_putb(config_ptr->ab_handle, port, value)
#endif

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
