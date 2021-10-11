/* $Header:   Y:/source/aic-7870/him/common/seq_off.hv_   1.7.1.1   14 Aug 1995 11:57:00   YU1868  $ */
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

#pragma ident	"@(#)seq_off.h	1.8	96/03/13 SMI"

/***************************************************************************
*
*  Module Name:   SEQUENCE.H
*
*  Description:   Sequencer code address definitions.
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

#define FAST20_LOW         0x0030         /* Fast 20 device map: 0 thru 7  */
#define FAST20_HIGH        0x0031         /*                     8 thru 15 */
#define DISCON_OPTION 		0x0032
#define SIOSTR3_ENTRY 		0x0008
#define ATN_TMR_ENTRY 		0x0010
#define PASS_TO_DRIVER 		0x003a
#define START_LINK_CMD_ENTRY 		0x0004
#define IDLE_LOOP_ENTRY 		0x0000
#define ACTIVE_SCB 		0x003c
#define WAITING_SCB 		0x003b
#define SEQUENCER_PROGRAM 		0x0000
#define XFER_OPTION 		0x0020
#define SIO204_ENTRY 		0x000c
#define TARGET_DATA_ENTRY  0x0014

#if !defined( _EX_SEQ00_ )
#define TARG_LUN_MASK0_0 		0x05a0
#define ARRAY_PARTITION0_0 		0x05a8
#endif

#if !defined( _EX_SEQ01_ )

#define TARG_LUN_MASK0_1 		0x0000       /*NOT DEFINED IN SEQ_01 */
#define ARRAY_PARTITION0_1 		0x0000    /*NOT DEFINED IN SEQ_01 */

#endif
