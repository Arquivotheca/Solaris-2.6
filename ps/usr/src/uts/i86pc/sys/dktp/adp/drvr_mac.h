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
 * Copyrighted as an unpublished work.
 * (c) Copyright Sun Microsystems, Inc. 1994
 * All rights reserved.
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
#pragma ident	"@(#)drvr_mac.h	1.2	94/11/04 SMI"

/***************************************************************************
*
*  Module Name:   DRVR_MAC.H
*
*  Description:
*                 Macro definitions referenced only when building HIM for
*                 driver (OPTIMA mode or STANDARD mode)
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

/***************************************************************************
*  Macro referenced for all configurations and all modes
***************************************************************************/
#define  PHM_PAUSE(hcntrl) \
		OUTBYTE(hcntrl, INBYTE((hcntrl)) | PAUSE);  \
		while (!(INBYTE((hcntrl)) & PAUSEACK))
#define  PHM_UNPAUSE(hcntrl,tmp) \
		OUTBYTE((hcntrl), INBYTE((hcntrl)) & ~PAUSE); \
		tmp = INBYTE((hcntrl))
#define  PHM_TURNLEDON
#define  PHM_TURNLEDOFF

/***************************************************************************
*  Macro referenced for common to driver
***************************************************************************/
#define  PHM_PATCHTONOCHAIN(scb_ptr)   (scb_ptr)->SP_Next = NOT_DEFINED
#define  PHM_POSTCOMMAND(config_ptr)   Ph_PostCommand((config_ptr))
#define  PHM_ABORTCHANNEL(config_ptr,HaStatus) \
            Ph_AbortChannel((config_ptr),(HaStatus))
#define  PHM_INDEXCLEARBUSY(config_ptr,scb) \
				(config_ptr)->CFP_HaDataPtr->indexclearbusy((config_ptr),(scb))
#define  PHM_REQUESTSENSE(scb_ptr,base) \
				(scb_ptr)->SP_ConfigPtr->CFP_HaDataPtr->requestsense((scb_ptr),(base))
#define  PHM_BIOSACTIVE(config_ptr) (config_ptr)->CFP_BiosActive
#define  PHM_SCBACTIVE(config_ptr,scb)  \
            ((config_ptr)->CFP_HaDataPtr->a_ptr.active_ptr[(scb)] != \
            NOT_DEFINED)
#define  PHM_CLEARTARGETBUSY(config_ptr,scb) \
				(config_ptr)->CFP_HaDataPtr->cleartargetbusy((config_ptr),(scb))
#define  PHM_TARGETABORT(config_ptr,scb_ptr,base) \
               Ph_TargetAbort((config_ptr),(scb_ptr),(base))
#define  PHM_SCBPTRISBOOKMARK(config_ptr,scb_ptr) ((scb_ptr) == \
               &((config_ptr)->CFP_HaDataPtr->scb_mark))
#define  PHM_INSERTBOOKMARK(config_ptr)   Ph_InsertBookmark((config_ptr))
#define  PHM_REMOVEBOOKMARK(config_ptr)   Ph_RemoveBookmark((config_ptr))
#define  PHM_ENABLENEXTSCBARRAY(config_ptr,scb_ptr) \
               (config_ptr)->CFP_HaDataPtr->enablenextscbarray((scb_ptr))

#define  PHM_ASYNCHEVENT(EventType, config_ptr, data)    \
               Ph_AsynchEvent(EventType, config_ptr, data)

/***************************************************************************
*  Macro referenced for standard mode driver only
***************************************************************************/
#define  PHM_GETCONFIG(config_ptr) Ph_GetDrvrConfig((config_ptr))
#define  PHM_INITHA(config_ptr)  Ph_InitDrvrHA((config_ptr))
#define  PHM_SEMSTATE(ha_ptr) (ha_ptr)->SemState
#define  PHM_SEMSET(ha_ptr,state) (ha_ptr)->SemState = (state)

#ifdef   _SABRE_HW
#define  PHM_INCLUDESABRE(device_id) 1
#else
#define  PHM_INCLUDESABRE(device_id) (device_id) != SABRE_ID
#endif
