/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

/* $Header:   Y:\source\aic-7870\him\himd\himd.cv_   1.74.1.2.1.1   06 Oct 1995 14:04:00   KLI  $ */

#pragma ident	"@(#)himd.c	1.9	96/03/13 SMI"
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

/***************************************************************************
*
*  Module Name:   HIMD.C
*
*  Description:
*                 Codes common to HIM configured for driver at run time are 
*                 defined here. It should be included if HIM is configured 
*                 for driver. These codes should always remain resident.
*
*  Programmers:   Paul von Stamwitz
*                 Chuck Fannin
*                 Jeff Mendiola
*                 Harry Yang
*    
*  Notes:         NONE
*
*  Entry Point(s):
*    PH_RelocatePointers
*                    - Relocate all pointers based on value of relocation
*
*  Revisions -
*
***************************************************************************/

#include "him_scb.h"
#include "him_equ.h"
#include "seq_off.h"

#ifdef _TARGET_MODE
extern void PH_TargetScbCompleted(Target_SCB *);
#endif   /* _TARGET_MODE   */

/*********************************************************************
*
*   PH_ScbSend routine -
*
*   Entry for execution of a STB from the host.
*
*  Return Value:  
*                  
*  Parameters:    scb_ptr - SCB structure contains opcode and any
*                           needed parameters / pointers.
*
*  Activation:    ASPI layer, normal operation.
*
*  Remarks:       When HIM / Sequencer handling of an SCB is complete,
*                 the HIM layer calls PH_ScbCompleted in the ASPI layer.
*
*********************************************************************/

void PH_ScbSend (sp_struct *scb_ptr)
{
   register AIC_7870 *base;
   register hsp_struct *ha_ptr;
   register cfp_struct *config_ptr;
   register UBYTE hcntrl_data;
   register UWORD semaphore;
   int orgint;

   INTR_OFF_SAV(orgint);

   config_ptr = scb_ptr->SP_ConfigPtr;
   base = config_ptr->CFP_Base;
   ha_ptr = config_ptr->CFP_HaDataPtr;

   hcntrl_data = INBYTE(AIC7870[HCNTRL]);
   semaphore = PHM_SEMSTATE(ha_ptr);
   PHM_SEMSET(ha_ptr,SEM_LOCK);

   /* Check for non-initiator SCSI command */
   if ((scb_ptr->SP_Cmd != EXEC_SCB) &&
       (scb_ptr->SP_Cmd != NO_OPERATION))
   {
      Ph_NonInit(scb_ptr);
      PHM_SEMSET(ha_ptr,semaphore);
      INTR_RESTORE(orgint);
      return;
   }
   else
   {
      scb_ptr->Sp_seq_scb.seq_scb_struct.SpecFunc = 0;  
   }

   /* If STATE = REAL MODE is Enabled, BIOS_ON has already been     */
   /*            Completed                                          */
   /* THEN     =>It is NOW Time to go to PROTECTED Mode & Restore   */
   /*            Scratch previously stored in MEMORY, into SCRATCH  */
   /*            RAM                                                */
   if (ha_ptr->Hsp_SaveState == SCR_REALMODE)
   {
      PH_Special(H_BIOS_OFF, config_ptr, scb_ptr);
   }

   Ph_ChainAppendEnd(config_ptr,scb_ptr);/* append at end for driver only */
   Ph_ScbPrepare(config_ptr,scb_ptr);    /* prepare SCB before processing */

#ifdef _TARGET_MODE
   scb_ptr->SP_RejectMDP = 0;          /* Reset it because of target mode */
#endif   /* of _TARGET_MODE   */

   if (semaphore == SEM_RELS)
   {
      if (Ph_SendCommand(ha_ptr, base))              /* Try to Send to Lance */
      {
         hcntrl_data |= SWINT;
      }
   }
   Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data), config_ptr);
   PHM_SEMSET(ha_ptr,semaphore);
   INTR_RESTORE(orgint);

   return;
}

/*********************************************************************
*
*  PH_IntHandler interrupt service routine -
*
*  Return Value:  Interrupt Status
*                  
*  Parameters:    config_ptr
*
*  Activation:    Arrow interrupt via Aspi Layer
*                  
*  Remarks:       More than one interrupt status bit can legally be
*                 set. It is also possible that no bits are set, in
*                 the case of using PH_IntHandler for polling.
*                  
*********************************************************************/
UBYTE PH_IntHandler (cfp_struct *config_ptr)
{
   register AIC_7870   *base;
   register hsp_struct *ha_ptr;
   register sp_struct  *scb_ptr;
   register UBYTE hcntrl_data, i, byte_buf;
   gen_union working;
   UBYTE int_status, ret_status;
   /* UBYTE bios_int = 0;  */        /*@BIOSDETECT*/
   register UWORD semaphore;
   int orgint;
   int count = 0;
   base = config_ptr->CFP_Base;
   ha_ptr = config_ptr->CFP_HaDataPtr;

   INTR_OFF_SAV(orgint);

   hcntrl_data = INBYTE(AIC7870[HCNTRL]);

   if (!(ret_status = ((int_status = Ph_ReadIntstat(base, config_ptr)) & ANYINT))) 
   {
      INTR_RESTORE(orgint);
      return (ret_status);
   }

   /* Make sure there is no reentrant to PH_IntHandler */
   if ((semaphore = PHM_SEMSTATE(ha_ptr)) == SEM_LOCK && (hcntrl_data & SWINT))
   {
      INTR_RESTORE(orgint);
      return (ret_status);
   }                                
   else
   {
      PHM_SEMSET(ha_ptr,SEM_LOCK);
   }
   if (hcntrl_data & INTEN)   /* Keep int asserted if it was enabled */
      Ph_WriteHcntrl(base,(UBYTE)(hcntrl_data |= SWINT), config_ptr);    

   while (1)
   {
      working.ubyte[0] = 0;              /* initialize bios_int */
      working.ubyte[1] = ret_status;      /* load return status */

      for (; int_status & CMDCMPLT; int_status = Ph_ReadIntstat(base, config_ptr))
      {
#ifdef _TARGET_MODE
         UBYTE scb_index;

         scb_index = QOUT_PTR_ARRAY[ha_ptr->qout_index];
         scb_ptr = ACTPTR[scb_index];
         if (scb_ptr == (sp_struct *) &CFP_TargetSCB)
         {
            if (scb_ptr->SP_HaStat)
               scb_ptr->SP_Stat = (HIMSCB_COMP + SCB_DONE_ERR);
            else
               scb_ptr->SP_Stat = HIMSCB_COMP;
            ha_ptr->qout_index++;
            ha_ptr->returnfreescb(config_ptr, scb_index);
            working.ubyte[3] = 1;
            PH_TargetScbCompleted((Target_SCB *) scb_ptr);
            break;
         }
#endif
         working.dword = ha_ptr->cmdcomplete(config_ptr,hcntrl_data,
                                                               working.dword);
         if (!(working.ubyte[3])) 
            break;                    /* process each command complete  */

         INTR_RESTORE(orgint);      /* Open window for servicing higher */
         INTR_OFF;                           /* level interrupt channel */
      }

      ret_status = working.ubyte[1];      /* retrieve return status */


      for (; int_status & (SEQINT | BRKINT | SCSIINT);
             int_status = Ph_ReadIntstat(base, config_ptr))    /* Check for other interrupts */
      {
         byte_buf = INBYTE(AIC7870[SSTAT1]);
         if (byte_buf & SCSIRSTI) /* Special case - handle SCSI reset first */
         {
            Ph_IntSrst(config_ptr);
            int_status = SCSIINT; /* Need to set to clear SCSIINT if no BIOS */
            byte_buf = 0;         /* Clear byte_buf */
            i = INVALID_SCBPTR;   /* Set i to invalid */
         }
         else if (byte_buf & SELTO)
         {
            i = INBYTE(AIC7870[WAITING_SCB]);
         }
         else
         {
            i = INBYTE(AIC7870[ACTIVE_SCB]);
         }
         if (i == INVALID_SCBPTR)
         {
            scb_ptr = NOT_DEFINED;
         }
         else
         {
            scb_ptr = ACTPTR[i];
            if (scb_ptr == BIOS_SCB)
            {
               ret_status |= NOT_OUR_INT;
               break;
            }
         }
         ret_status |= OUR_OTHER_INT;

         if (int_status & SEQINT)                  /* Process sequencer interrupt */
         {
            OUTBYTE(AIC7870[CLRINT], SEQINT);
            if (scb_ptr == NOT_DEFINED)
            {
               int_status = (int_status & ANYINT) | ABORT_TARGET;
            }
            int_status &= INTCODE;
            if      (int_status == DATA_OVERRUN)     Ph_CheckLength(scb_ptr, base);
            else if (int_status == CDB_XFER_PROBLEM) Ph_CdbAbort(scb_ptr, base);
            else if (int_status == HANDLE_MSG_OUT)   Ph_SendMsgo(scb_ptr, base);
            else if (int_status == SYNC_NEGO_NEEDED) Ph_Negotiate(scb_ptr, base);
            else if (int_status == CHECK_CONDX)      Ph_CheckCondition(scb_ptr, base);
            else if (int_status == PHASE_ERROR)      Ph_BadSeq(config_ptr, base);
            else if (int_status == EXTENDED_MSG)     Ph_ExtMsgi(scb_ptr, base);
            else if (int_status == UNKNOWN_MSG)      Ph_HandleMsgi(scb_ptr, base);
            else if (int_status == ABORT_TARGET)     PHM_TARGETABORT(config_ptr, scb_ptr, base);
            else if (int_status == NO_ID_MSG)        PHM_TARGETABORT(config_ptr, scb_ptr, base);
         }
         else if (int_status & SCSIINT)            /* Process SCSI interrupt */
         {
            /* SCSI Reset is handled before any other interrupts.
               Select timeout must be checked before bus free since
               bus free will be set due to a selection timeout */

#ifdef _TARGET_MODE
            UBYTE stat0;
            stat0 = INBYTE(AIC7870[SSTAT0]);
            if ((stat0 & TARGET) && (stat0 & SELDI)) 
               Ph_TargetIntSelIn(config_ptr);
            else if ((stat0 & TARGET) && (byte_buf & ATNTARG)) 
               Ph_TargetATNIntr((Target_SCB *) scb_ptr, base);
            else
            {
#endif   /* _TARGET_MODE   */
               if      (byte_buf & SELTO)    Ph_IntSelto(config_ptr, scb_ptr);
               else if (byte_buf & BUSFREE)  Ph_IntFree(config_ptr, scb_ptr);
               else if (byte_buf & SCSIPERR) Ph_ParityError(scb_ptr, base);
#ifdef _TARGET_MODE
            }
#endif   /* _TARGET_MODE   */
            OUTBYTE(AIC7870[CLRINT], CLRSCSINT);
         }
         else if (int_status & BRKINT)             /* Process sequencer breakpoint */
         {
            OUTBYTE(AIC7870[CLRINT], BRKINT);
            {
               OUTBYTE(AIC7870[BRKADDR1], BRKDIS); /* Disable Breakpoint */

               byte_buf = INBYTE(AIC7870[SEQCTL]) & ~BRKINTEN;
               OUTBYTE(AIC7870[SEQCTL], byte_buf); /* Disable BP Interrupt */

               scb_ptr = ACTPTR[i];
               i = Ph_SendTrmMsg(config_ptr, MSG0C);/* Yes, send BDR message */
               Ph_TrmCmplt(config_ptr, i, MSG0C);
            }
         }
         hcntrl_data &= ~PAUSE;
         Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data), config_ptr); /* Unpause the sequencer */
      }

      Ph_SendCommand(ha_ptr, base);
      Ph_PostCommand(config_ptr);

      /* For BIOS interrupt we must break out of the loop and then */
      /* get BIOS interrupt serviced (interrupt still pending) */
      if ((ret_status & BIOS_INT) && (! DONE_CMD))
      {
         break;
      }

      int_status = Ph_ReadIntstat(base, config_ptr);
      if ((++count != MAX_INTRLOOP) && ((int_status & ANYINT) || (DONE_CMD)))
      {
         INTR_RESTORE(orgint);      /* Open window for servicing higher */
         INTR_OFF;                           /* level interrupt channel */
      }
      else
      {  /* Here we are going to unpause the chip and restore  */
         /* the software interrupt at the same time            */
         /* Here should be the only exit out of big loop       */
         /* A pulse will definitely be generated here          */
         Ph_WriteHcntrl(base, (UBYTE)(hcntrl_data & ~(INTEN | PAUSE)), config_ptr); 
         break;
      }
   }
   
   PHM_SEMSET(ha_ptr,semaphore);

   Ph_SendCommand(ha_ptr, base);

   Ph_WriteHcntrl(base, (UBYTE)(hcntrl_data & ~(PAUSE | SWINT)), config_ptr);

   INTR_RESTORE(orgint);

   return (ret_status);
}

/*********************************************************************
*
*  PH_PollInt
*
*  Modules used for polled mode of operation
*
*  Return Value:  None
*                  
*  Parameters:    config_ptr
*
*  Activation:    PH_IntHandler
*                  
*  Remarks:
*                  
*********************************************************************/

UBYTE PH_PollInt (cfp_struct *config_ptr)
{
   register AIC_7870 *base;
   hsp_struct *ha_ptr;
   UBYTE ival = 0,
         hc_data = 0;
   int orgint;

   INTR_OFF_SAV(orgint);

   base = config_ptr->CFP_Base;
   ha_ptr = config_ptr->CFP_HaDataPtr;

   hc_data = INBYTE(AIC7870[HCNTRL]);
   Ph_WriteHcntrl(base, (UBYTE) (hc_data | PAUSE), config_ptr);    /* Pause Sequencer */
   while ((INBYTE(AIC7870[HCNTRL]) & PAUSEACK) == 0);

   ival = (Ph_ReadIntstat(base, config_ptr) & ANYINT);   /* Get Arrow Interrupt status */

   Ph_WriteHcntrl(base, (UBYTE) (hc_data), config_ptr);            /* Restore Sequencer state */

   INTR_RESTORE(orgint);

   return (ival);
}

/*********************************************************************
*
*  PH_RelocatePointers -
*
*  Adjust structure pointers for OSMs that "throw away" initializtaion code.
*
*  Return Value:  None
*                  
*  Parameters:    config_ptr (new)
*                 UWORD relocate_value (Positive number which will be
*                                       subtracted from stucture pointers.)
*
*  Activation:    OSM
*
*  Remarks:       Must be called after data structures have been moved.
*                 Both HaDataPtr and HaDataPhy will be modified here
*                 so that the caller won't have to do it. All the caller
*                 has to do is copy (move) memory and call this function
*                 with new config_ptr and relocate value.
*                 
*********************************************************************/
void PH_RelocatePointers (cfp_struct *config_ptr,
                          UWORD relocate_value)
{
   config_ptr->Cf_hsp_ptr.HaDataPtrField -= relocate_value;

   /* setup mode dependent HA data structure */
#ifdef   _STANDARD
#ifdef   _OPTIMA
   /* both OPTIMA and STANDARD may be available */
   if (config_ptr->Cf_AccessMode == 2)
   {
      config_ptr->Cf_HaDataPhy -= relocate_value;
      Ph_SetOptimaHaData(config_ptr);
   }
   else
   {
      Ph_SetStandardHaData(config_ptr);
   }
#else
   /* standard mode only */
   Ph_SetStandardHaData(config_ptr);
#endif
#else
#ifdef   _OPTIMA
   /* optima mode only */
   config_ptr->Cf_HaDataPhy -= relocate_value;
   Ph_SetOptimaHaData(config_ptr);
#else
   /* Driver not in standard or optima mode? no way! */
#endif
#endif
}

/*********************************************************************
*
*   PH_ChainAppendEnd routine -
*
*   Append a scb request to the end of scb chain
*
*  Return Value:  none
*                  
*  Parameters:    config_ptr - 
*                 scb_ptr - SCB structure contains opcode and any
*                           needed parameters / pointers.
*                 
*
*  Activation:    Ph_ScbSend
*
*  Remarks:
*
*********************************************************************/

void Ph_ChainAppendEnd (cfp_struct *config_ptr, sp_struct *scb_ptr)
{
   register hsp_struct *ha_ptr;

   ha_ptr = config_ptr->CFP_HaDataPtr;

   /* patch to no chain for driver only */
   PHM_PATCHTONOCHAIN(scb_ptr);

   if (ha_ptr->Head_Of_Queue != NOT_DEFINED)    /* Find insertion point */
   {
      ha_ptr->End_Of_Queue->SP_Next =  scb_ptr; /* Add at End of existing Q */
   }
   else
   {
      ha_ptr->Head_Of_Queue      = scb_ptr;
      config_ptr->CFP_DriverIdle = 0;
      ha_ptr->Hsp_MaxNonTagScbs  = (UBYTE) config_ptr->Cf_MaxNonTagScbs;
      ha_ptr->Hsp_MaxTagScbs     = (UBYTE) config_ptr->Cf_MaxTagScbs;
   }

   while (scb_ptr->SP_Next != NOT_DEFINED)
      scb_ptr = scb_ptr->SP_Next;

   ha_ptr->End_Of_Queue = scb_ptr;  /* Set end of queue pointer */
}

/*********************************************************************
*
*   PH_ChainInsertFront routine -
*
*   Insert a scb request at front of scb chain
*
*  Return Value:  none
*                  
*  Parameters:    config_ptr - 
*                 scb_ptr - SCB structure contains opcode and any
*                           needed parameters / pointers.
*
*  Activation:    Ph_NonInit
*
*  Remarks:
*
*********************************************************************/

void Ph_ChainInsertFront (cfp_struct *config_ptr,
                           sp_struct *scb_ptr)
{
}

/*********************************************************************
*
*   PH_ChainRemove routine -
*
*   Remove a scb request from scb chain
*
*  Return Value:  none
*                  
*  Parameters:    config_ptr - 
*                 scb_ptr - SCB structure contains opcode and any
*                           needed parameters / pointers.
*
*  Activation:    Ph_NonInit
*
*  Remarks:    System interrupt must be disabled before calling 
*              this routine.
*
*********************************************************************/

void Ph_ChainRemove (cfp_struct *config_ptr,
                        sp_struct *scb_ptr)
{
   register hsp_struct *ha_ptr;
   register sp_struct *previous_ptr;

   ha_ptr = config_ptr->CFP_HaDataPtr;
   previous_ptr = NOT_DEFINED;

   if (scb_ptr == ha_ptr->Head_Of_Queue)
   {
      ha_ptr->Head_Of_Queue = scb_ptr->SP_Next;
   }
   else
   {
      previous_ptr = Ph_ChainPrevious(ha_ptr,scb_ptr);
      previous_ptr->SP_Next = scb_ptr->SP_Next;
   }

   if (scb_ptr->SP_Next == NOT_DEFINED)
   {
      ha_ptr->End_Of_Queue = previous_ptr;
   }

   if (ha_ptr->Head_Of_Queue == NOT_DEFINED)
   {
      config_ptr->CFP_DriverIdle = 1;
   }
}

/*********************************************************************
*
*   PH_ChainPrevious routine -
*
*   Get previous scb pointer in scb chain
*
*  Return Value:  previous scb pointer
*                  
*  Parameters:    ha_ptr - 
*                 scb_ptr - SCB structure contains opcode and any
*                           needed parameters / pointers.
*
*  Activation:    Ph_HardReset
*
*  Remarks:
*
*********************************************************************/


sp_struct *Ph_ChainPrevious (hsp_struct *ha_ptr,
                              sp_struct *scb_ptr)
{
   register sp_struct *previous_ptr;

   previous_ptr = ha_ptr->Head_Of_Queue;

   if (previous_ptr == NOT_DEFINED)
      return (NOT_DEFINED);

   while ((previous_ptr->SP_Next != scb_ptr) &&
          (ha_ptr->End_Of_Queue != previous_ptr) &&
          (previous_ptr->SP_Next != NOT_DEFINED))
   {
      previous_ptr = previous_ptr->SP_Next;
   }
   if ((previous_ptr->SP_Next == scb_ptr) ||
      (scb_ptr == ha_ptr->Head_Of_Queue))
   {
      return(previous_ptr);
   }
   return (NOT_DEFINED);
}

/*********************************************************************
*
*   Ph_ScbPrepare routine -
*
*   Prepare a scb after it has been appended into scb chain
*
*  Return Value:  none
*                  
*  Parameters:    config_ptr - 
*                 scb_ptr - SCB structure contains opcode and any
*                           needed parameters / pointers.
*
*  Activation:    Ph_ScbSend
*
*  Remarks:
*
*********************************************************************/

void Ph_ScbPrepare (cfp_struct *config_ptr,
                     sp_struct *scb_ptr)
{
   register hsp_struct *ha_ptr;

   ha_ptr = config_ptr->CFP_HaDataPtr;
   while (1)           /* Change SCB state variables */
   {
      scb_ptr->SP_Stat = SCB_PENDING;
      ++(ha_ptr->acttarg[scb_ptr->SP_Tarlun]);
      scb_ptr->SP_MgrStat = Ph_GetScbStatus(ha_ptr, scb_ptr);
      if (scb_ptr->SP_MgrStat == SCB_READY)
      {
         ++ha_ptr->ready_cmd;
      }
      if (scb_ptr->SP_Next == NOT_DEFINED) 
      {
         break;
      }
      else
      {
         scb_ptr = scb_ptr->SP_Next;
      }
   }
}

/*********************************************************************
*
*  Ph_SendCommand routine -
*
*  Send SCB's to Arrow
*
*  Return Value:  ret_status (unsigned char)
*                  
*  Parameters: ha_ptr
*              base address of AIC-7870
*
*  Activation: PH_ScbSend
*              PH_IntHandler
*              PH_PollInt
*                  
*  Remarks:                
*                  
*********************************************************************/
UBYTE Ph_SendCommand (hsp_struct *nha_ptr,
                      register AIC_7870 *base)
{
   register hsp_struct *ha_ptr;
   register sp_struct *scb_ptr;
   register UBYTE i;
   register UBYTE hcntl_state = 0;
   UBYTE ret_status = 0;
   cfp_struct *config_ptr;

   ha_ptr = nha_ptr;
      
   for (scb_ptr = ha_ptr->Head_Of_Queue;
        (scb_ptr != NOT_DEFINED) && ha_ptr->ready_cmd && 
         ha_ptr->morefreescb(ha_ptr,scb_ptr);scb_ptr = scb_ptr->SP_Next)
   {
      if (scb_ptr->SP_MgrStat == SCB_READY)
      {
         /* Get a Free scb from the FREE scb Pool                   */
         i = ha_ptr->getfreescb(ha_ptr,scb_ptr);

         /* Decrement Number of Available scb's                     */
         --ha_ptr->ready_cmd;

         /* Set Current Scb asActive                                */
         scb_ptr->SP_MgrStat = SCB_ACTIVE;

         /* Put Scb into ACTIVE PTR QUEUE                           */
         ACTPTR[i] = scb_ptr;

         config_ptr = scb_ptr->SP_ConfigPtr;
         hcntl_state = INBYTE(AIC7870[HCNTRL]); /* pause sequencer  */
         Ph_WriteHcntrl(base, (UBYTE) (hcntl_state | PAUSE), config_ptr);
         while (~INBYTE(AIC7870[HCNTRL]) & PAUSEACK);

         /* Queue an OPTIMA/STANDARD SCB for the Lance Sequencer    */
         ha_ptr->enque(i,scb_ptr,base);

         Ph_WriteHcntrl(base, (UBYTE) (hcntl_state), config_ptr);
      }
   }
   return(ret_status);
}

/*********************************************************************
*
*  Ph_TerminateCommand routine -
*
*  Mark SCB as done and find next SCB to execute
*
*  Return Value:  None
*                  
*  Parameters: scb_ptr
*
*  Activation: PH_IntHandler
*              Ph_AbortChannel  
*              Ph_BadSeq
*              Ph_CheckCondition
*              Ph_IntFree
*              Ph_IntSelto
*                
*  Remarks:
*                  
*********************************************************************/
void Ph_TerminateCommand (sp_struct *nscb_ptr,
                          UBYTE scb)
{
   register hsp_struct *ha_ptr;
   register sp_struct *scb_ptr;
   register UBYTE i;

   if (nscb_ptr == NOT_DEFINED)                 /* Exit if Null SCB */
   {
      return;
   }

   scb_ptr = nscb_ptr;
   ha_ptr = scb_ptr->SP_ConfigPtr->CFP_HaDataPtr;

   if (scb_ptr == &(ha_ptr->scb_mark))
   {
      return;
   }

   /* return to done stack only if scb is valid */
   if (scb != 0xff)
      DONE_STACK[(DONE_CMD)++] = scb;

   Ph_SetMgrStat(scb_ptr);                   /* Update status */

   i = (UBYTE)scb_ptr->SP_Tarlun;

   if (Ph_GetScbStatus(ha_ptr, scb_ptr) == SCB_WAITING)
   {
      for (;;)
      {
         /* add processing of BOOKMARK */
         if (scb_ptr->SP_Next != NOT_DEFINED &&
            scb_ptr->SP_Next->SP_Cmd != BOOKMARK)
         {
            scb_ptr = scb_ptr->SP_Next;
         }
         else
         {
            scb_ptr = ha_ptr->Head_Of_Queue;
         }

         if (scb_ptr->SP_MgrStat == SCB_WAITING)
         {
            if (scb_ptr->SP_Tarlun == i)
            {
               scb_ptr->SP_MgrStat = SCB_READY;
               ++ha_ptr->ready_cmd;
               break;
            }
         }
         if (scb_ptr == nscb_ptr)
         {
            break;
         }
      }
   }
   if (ha_ptr->acttarg[i])
   {
      --(ha_ptr->acttarg[i]);
   }
}

/*********************************************************************
*
*  Ph_PostCommand routine -
*
*  Return completed SCB to ASPI layer
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*
*  Activation: PH_IntHandler
*              Ph_BadSeq
*              Ph_IntSrst
*              Ph_TargetAbort
*
*  Remarks:
*
*********************************************************************/

extern void PH_ScbCompleted(sp_struct *);

void Ph_PostCommand (cfp_struct *config_ptr)
{
   register hsp_struct *ha_ptr;
   register sp_struct *scb_ptr;
   register UBYTE scb;
   int orgint;
#ifdef _TARGET_MODE
   register Target_SCB *target_ptr;
#endif

   ha_ptr = config_ptr->CFP_HaDataPtr;

   if (ha_ptr->Head_Of_Queue == NOT_DEFINED)    /* Exit if Null SCB Q */
   {
      return;
   }

   while (DONE_CMD)
   {
      scb = DONE_STACK[--(DONE_CMD)];
      scb_ptr = ACTPTR[scb];
      ha_ptr->returnfreescb(config_ptr,scb);

      Ph_ChainRemove(config_ptr, scb_ptr);     /* remove scb from chain */

      scb_ptr->SP_Stat = scb_ptr->SP_MgrStat;
      INTR_SAVE(orgint);
#ifdef _TARGET_MODE
      target_ptr = (Target_SCB *) scb_ptr;
      if (target_ptr->TSP_TgtSCB)
         PH_TargetScbCompleted(target_ptr);
      else
#endif      /* of _TARGET_MODE   */
         PH_ScbCompleted(scb_ptr);                     /* post */
      INTR_RESTORE(orgint);
   }
}

/*********************************************************************
*
*  Ph_RemoveAndPostScb routine -
*
*  Return completed (but non active) SCB to ASPI layer
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*
*  Activation: PH_PostCommand
*
*  Remarks:
*
*********************************************************************/
void Ph_RemoveAndPostScb (cfp_struct *config_ptr,
                           sp_struct *scb_ptr)
{
   int orgint;
#ifdef _TARGET_MODE
   Target_SCB *target_ptr;
#endif      /* of _TARGET_MODE   */

   Ph_ChainRemove(config_ptr, scb_ptr);     /* remove scb from chain */

   scb_ptr->SP_Stat = scb_ptr->SP_MgrStat;
   INTR_SAVE(orgint);
#ifdef _TARGET_MODE
   target_ptr = (Target_SCB *)scb_ptr;
   if (target_ptr->TSP_TgtSCB)
      PH_TargetScbCompleted(target_ptr);
   else
#endif      /* of _TARGET_MODE   */
      PH_ScbCompleted(scb_ptr);             /* post */
   INTR_RESTORE(orgint);
}

/*********************************************************************
*
*  Ph_PostNonActiveScb routine -
*
*  Return completed (but non active) SCB to ASPI layer
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*
*  Activation: PH_PostCommand
*
*  Remarks:
*
*********************************************************************/
void Ph_PostNonActiveScb (cfp_struct *config_ptr,
                           sp_struct *scb_ptr)
{
   register hsp_struct *ha_ptr;
   register UBYTE i;

   ha_ptr = config_ptr->CFP_HaDataPtr;
   i = (UBYTE)scb_ptr->SP_Tarlun;
   if (ha_ptr->acttarg[i])
   {
      --(ha_ptr->acttarg[i]);
   }
   Ph_RemoveAndPostScb(config_ptr,scb_ptr);
}

/*********************************************************************
*
*  Ph_TermPostNonActiveScb routine -
*
*  Terminate and return completed (but non active) SCB to ASPI layer
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*
*  Activation: PH_AbortChannel
*
*  Remarks:
*
*********************************************************************/
void Ph_TermPostNonActiveScb (sp_struct *scb_ptr)
{
   register cfp_struct *config_ptr = scb_ptr->SP_ConfigPtr;

   Ph_TerminateCommand(scb_ptr, 0xff);
   Ph_RemoveAndPostScb(config_ptr, scb_ptr);
}

/*********************************************************************
*
*  Ph_AbortChannel routine -
*
*  <brief description>
*
*  Return Value:  None
*
*  Parameters:    config_ptr
*
*  Activation:    Ph_IntSrst
*                 Ph_BadSeq
*
*  Remarks:
*
*********************************************************************/
void Ph_AbortChannel (cfp_struct *config_ptr, UBYTE HaStatus)
{
   sp_struct *scb_ptr;
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   AIC_7870 *base = config_ptr->CFP_Base;
   UBYTE hc_data;
   sp_struct *head_ptr;
   sp_struct *tail_ptr;

   hc_data = INBYTE(AIC7870[HCNTRL]);
   Ph_WriteHcntrl(base, (UBYTE) (hc_data | PAUSE), config_ptr);    /* Pause Sequencer */
   while ((INBYTE(AIC7870[HCNTRL]) & PAUSEACK) == 0)
      ;

   for ( scb_ptr=ha_ptr->Head_Of_Queue;
         (scb_ptr != NOT_DEFINED) && (scb_ptr->SP_Cmd != BOOKMARK);
         scb_ptr = ha_ptr->Head_Of_Queue)
   {
      scb_ptr->SP_HaStat = HaStatus;
      scb_ptr->SP_MgrStat = SCB_DONE_ERR;
      Ph_PostNonActiveScb(config_ptr, scb_ptr);
   }

   /* Save values for Head_of_Queue & End_of_Queue, Since they will */
   /*  be Destroyed by the Ph_SetHaData call.                       */
   /*                                                               */
   head_ptr = ha_ptr->Head_Of_Queue;
   tail_ptr = ha_ptr->End_Of_Queue;

   /* Reinitialize Entire HA Structure so that Sequencer is Cleared */
   Ph_SetHaData(config_ptr);

   /* Restore values Saved before Ph_SetHaData call                 */
   /*                                                               */
   ha_ptr->Head_Of_Queue = head_ptr;
   ha_ptr->End_Of_Queue  = tail_ptr;

   /* Re-setup all new requests after bookmark. This is necessary   */
   /* because the brand new him data structure just get setup with  */
   /* call to Ph_SetHaData                                          */
   for (scb_ptr=ha_ptr->Head_Of_Queue;
        scb_ptr != NOT_DEFINED;
        scb_ptr = scb_ptr->SP_Next)
   {
      if (scb_ptr->SP_Cmd != BOOKMARK)
         Ph_ScbPrepare(config_ptr,scb_ptr);
   }

   Ph_WriteHcntrl(base, (UBYTE) (hc_data), config_ptr);            /* Restore Sequencer state */
}
/*********************************************************************
*
*  Ph_GetCurrentScb routine -
*
*  Get current scb
*
*  Return Value:  scb number
*
*  Parameters:    config_ptr
*
*  Activation:    Ph_AbortChannel
*
*  Remarks:
*
*********************************************************************/
/*
UBYTE Ph_GetCurrentScb (cfp_struct *config_ptr)
{
}
*/

/*********************************************************************
*  Ph_HaHardReset routine -
*
*  Perform Hard host adapter reset.
*
*  Return Value:  none
*                  
*  Parameters:    
*
*  Activation:    Ph_Special
*
*  Remarks:       
*                 
*********************************************************************/
void Ph_HaHardReset (cfp_struct *config_ptr)
{
   AIC_7870 *base = config_ptr->CFP_Base;
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   register sp_struct  *scb_ptr;
   /* sp_struct *mark_ptr = &(ha_ptr->scb_mark);   */
   /* UWORD j = 0;   */
   UBYTE index, HaStatus;

   /* Pause the Sequencer so that we can setup Hard Reset Ending Flag */
   Ph_WriteHcntrl(base, (UBYTE) (INBYTE(AIC7870[HCNTRL]) | PAUSE), config_ptr);   
   while (~INBYTE(AIC7870[HCNTRL]) & PAUSEACK);


   OUTBYTE(AIC7870[CLRINT],(CLRCMDINT | SEQINT | BRKINT | CLRSCSINT));

   Ph_InsertBookmark(config_ptr);

   /* Return the active scb pointer, if any, in the sequencer through AEN */
   /* call to OSM prior to reset bus and abort all pending commands.      */
   index = INBYTE(AIC7870[ACTIVE_SCB]);
   if (   (INBYTE(AIC7870[SIMODE1]) & ENBUSFREE)
       && (index != INVALID_SCBPTR))
      scb_ptr = ACTPTR[index];
   else
      scb_ptr = NOT_DEFINED;

   PHM_ASYNCHEVENT(AE_HA_ABTACTIVE, config_ptr, (void *)scb_ptr);   /* Asynchronous Event */

   if (config_ptr->CFP_ResetBus)          /* Reset bus if told to */
   {
      Ph_ResetSCSI(base, config_ptr);
      Ph_CheckSyncNego(config_ptr);       /* Adjust the sync nego parameters   */
   }
   Ph_ResetChannel(config_ptr);                                       

   HaStatus = HOST_ABT_HA;
   Ph_AbortChannel(config_ptr, HaStatus);

   /* swap process done and remove pre-bookmark list */
   Ph_RemoveBookmark(config_ptr);   /* Remove "Bookmark" from queue */

   /* Now, Restart Sequencer */
   OUTBYTE(AIC7870[SEQADDR0], IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(AIC7870[SEQADDR1], IDLE_LOOP_ENTRY >> 10);

   Ph_WriteHcntrl(base, (UBYTE) (INBYTE(AIC7870[HCNTRL]) & ~PAUSE), config_ptr);
}

/*********************************************************************
*
*   Ph_SetHaData routine -
*
*   This routine initializes HA structure parameters which are common
*   to both OPTIMA mode and STANDARD mode
*
*  Return Value:  none
*                  
*  Parameters:    config_ptr
*
*  Activation:    PH_InitDriverHA
*                  
*  Remarks:                
*
*********************************************************************/

void Ph_SetHaData (cfp_struct *config_ptr)
{
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;

   /* initialize fields which are common to both OPTIMA and */
   /* STANDARD mode                                         */
   ha_ptr->Head_Of_Queue = ha_ptr->End_Of_Queue = NOT_DEFINED;
   Ph_MemorySet(ha_ptr->acttarg, 0, 256);
   ha_ptr->ready_cmd = 0;
   ha_ptr->Hsp_MaxNonTagScbs  = (UBYTE) config_ptr->Cf_MaxNonTagScbs;
   ha_ptr->Hsp_MaxTagScbs     = (UBYTE) config_ptr->Cf_MaxTagScbs;
   ha_ptr->done_cmd = 0;

   /* Setup Physical Address of CDB within ha_ptr->scb_mark */
   Ph_SetScbMark(config_ptr);

#ifdef _TARGET_MODE
   Ph_SetTargetSCB(config_ptr);
#endif      /* of _TARGET_MODE  */   

   /* here we are not going to initialize free_hi_scb, active_ptr,   */
   /* and free_stack because their set up will be mode dependent.    */
   /* done_stack does not really need to be initialized.             */
   
   /* setup mode dependent HA data structure */
#ifdef   _STANDARD
#ifdef   _OPTIMA
   /* both OPTIMA and STANDARD may be available */
   if (config_ptr->Cf_AccessMode == 2)
   {
      Ph_SetOptimaHaData(config_ptr);
   }
   else
   {
      Ph_SetStandardHaData(config_ptr);
   }
#else
   /* standard mode only */
   Ph_SetStandardHaData(config_ptr);
#endif
#else
#ifdef   _OPTIMA
   /* optima mode only */
   Ph_SetOptimaHaData(config_ptr);
#else
   /* Driver not in standard or optima mode? no way! */
#endif
#endif

}




/*********************************************************************
*
*  Ph_SetScbMark -
*
*     Compute Physical Address of CDB.
*
*  Return Value:  NONE
*
*  Parameters:    config_ptr
*
*  Activation:    Ph_SetOptimaScratch
*                  
*  Remarks:       
*
*********************************************************************/
void  Ph_SetScbMark(cfp_struct *config_ptr)

{
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   sp_struct *mark_ptr = &(ha_ptr->scb_mark);

   DWORD addr_buf, phys_cdbaddr;

   /* Get logical offset to start of "scb_mark" portion of hsp structure */
   addr_buf = ( (DWORD) mark_ptr - (DWORD) ha_ptr);

   /* Compute Physical Address to start of "scb_mark" portion of hsp structure */
   addr_buf = addr_buf + (DWORD)(config_ptr -> Cf_HaDataPhy);

   /* Calculate address to Start of CDB for use by Sequencer   */
   phys_cdbaddr = (addr_buf + ((DWORD) mark_ptr->Sp_CDB - (DWORD) mark_ptr));

   /* Store Physical CDB Address into scb_mark structure */
   mark_ptr->SP_CDBPtr = phys_cdbaddr;

   /* Store Pointer to Config Structure into scb_mark structure */
   mark_ptr->SP_ConfigPtr = config_ptr;

}




/*********************************************************************
*  Ph_HaSoftReset routine -
*
*  Perform soft host adapter reset.
*
*  Return Value:  none
*                  
*  Parameters:    
*
*  Activation:    Ph_Special
*
*  Remarks:       
*                 
*********************************************************************/
void  Ph_HaSoftReset (cfp_struct *config_ptr)
{
}

/*********************************************************************
*  Ph_BusDeviceReset routine -
*
*  Determine state of target to be reset, issue Bus Reset and clean
*  up all SCBs.
*
*  Return Value:  none
*                  
*  Parameters:    config_ptr
*                 scb_ptr
*
*  Activation:    Ph_NonInit
*
*  Remarks:       
*                 
*********************************************************************/
#ifdef   _FAULT_TOLERANCE

void Ph_BusDeviceReset (sp_struct *scb_ptr)
{
   cfp_struct *config_ptr=scb_ptr->SP_ConfigPtr;
   AIC_7870 *base = config_ptr->CFP_Base;
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   sp_struct *tmp_ptr;
   register UBYTE hcntl_state = 0;
   /*   UBYTE scsi_state = 0, j = 0;   */
   UBYTE tgtid;
   UBYTE scb_index;
#ifdef _TARGET_MODE
   Target_SCB *target_ptr;
#endif      /* of _TARGET_MODE   */


   /* Check to see if a Free scb index is Available                 */
   if (!(ha_ptr->morefreescb(ha_ptr, scb_ptr)))
   {
      /* No scb_index is Available from FREE Pool                   */
      scb_ptr->SP_HaStat = HOST_NOAVL_INDEX;
      scb_ptr->SP_MgrStat = SCB_DONE_ERR;

      /* Since NO scb_index was ever Removed from the FREE Pool,    */
      /* Just Return the Completed scb directly back to the Host.   */

#ifdef _TARGET_MODE
      target_ptr = (Target_SCB *)scb_ptr;
      if (target_ptr->TSP_TgtSCB)
         PH_TargetScbCompleted(target_ptr);
      else
#endif      /* of _TARGET_MODE   */
         PH_ScbCompleted(scb_ptr);
      return;
   }

   /* Check to see if the current scsi id (for this scb) = our Host Adapter */
   if ( (tgtid = (UBYTE) (scb_ptr->SP_Tarlun >> 4)) == config_ptr->Cf_ScsiId)
   {
      /* Yes - The current scsi id (for this scb) = our Host Adapter */

      /* NOTE - We have NOT YET gotten an SCB INDEX from the FREE    */
      /*        Queue since the following Ph_HaHardReset call and    */
      /*        subsequent call to Ph_AbortChannel will require      */
      /*        special logic to Abort all previous queued scb's     */
      /*        without touching the SCB that contains the current   */
      /*        Device Reset command Sequence.                       */
      /*                                                             */
      /* Reset Entire Host Adapter                                   */
      Ph_HaHardReset (config_ptr);

      /* append at end for driver only */
      Ph_ChainAppendEnd(config_ptr, scb_ptr);

      /* prepare SCB before processing */
      Ph_ScbPrepare(config_ptr,scb_ptr);

      /* NOW -- Get an SCB INDEX Value from the FREE Queue for use  */
      /*        by the current scb                                  */
      /*                                                            */
      scb_index = ha_ptr->getfreescb(ha_ptr,scb_ptr);

      /* Update Number of Ready Scb's                               */
      --ha_ptr->ready_cmd;

      /* Set Current Scb as Active                                  */
      scb_ptr->SP_MgrStat = SCB_ACTIVE;

      /* Put Scb into ACTIVE PTR QUEUE                              */
      ACTPTR[scb_index] = scb_ptr;

      /* AT THIS POINT IN TIME - WE WILL JUST RETURN SCB TO HOST    */
      /*                         AFTER PUR SETUP                    */
      scb_ptr->SP_TargStat = UNIT_GOOD;
      scb_ptr->SP_HaStat   = HOST_NO_STATUS;

      Ph_TerminateCommand(scb_ptr, scb_index);
      Ph_PostCommand(config_ptr);
      return;
   }
   else
   {
      /* NO - The current scsi id (for this scb) != our Host Adapter */
      tmp_ptr = ha_ptr->Head_Of_Queue;
      
      /* Clear SCB's from Queue */
      while (tmp_ptr != NOT_DEFINED)
      {
         /* Check All SCB's for Matching Target SCSI ID and NOT     */
         /*  our current Scb. If so, Abort each scb separately      */
         if ((tmp_ptr != scb_ptr) && ((tmp_ptr->SP_Tarlun >> 4) == tgtid))
         {
            /* Abort Current outstanding SCB for Target SCSI ID     */
            Ph_ScbAbort(tmp_ptr);
         }
         tmp_ptr = tmp_ptr->SP_Next;
      }

      /* append at end for driver only */
      Ph_ChainAppendEnd(config_ptr, scb_ptr);

      /* prepare SCB before processing */
      Ph_ScbPrepare(config_ptr,scb_ptr);

      /* *****  Setup to issue device reset & Use Input SCB    ******/    

      /* Setup "special" Bus Device Reset Message Value             */
      scb_ptr->SP_SegCnt = SPEC_OPCODE_00;

      /* Setup "special" opcode for Bus Device Reset Command        */
      scb_ptr->SP_SegPtr = MSG0C;

      /* SET "special" OPCODE -- Bit 3 of the SCB Control Byte      */
      scb_ptr->Sp_seq_scb.seq_scb_struct.SpecFunc = SPEC_BIT3SET;  

      /* NO  -- Get an SCB INDEX Value from the FREE Queue for use  */
      /*        by the current scb                                  */
      /*                                                            */
      scb_index = ha_ptr->getfreescb(ha_ptr,scb_ptr);

      --ha_ptr->ready_cmd;       /* Update Number of available scb's */

      scb_ptr->SP_MgrStat = SCB_BDR;   /* Set Current Scb as Active */

      ACTPTR[scb_index] = scb_ptr;     /* Put Scb into ACTIVE PTR QUEUE */

      hcntl_state = INBYTE(AIC7870[HCNTRL]); /* pause sequencer     */
      Ph_WriteHcntrl(base, (UBYTE) (hcntl_state | PAUSE), config_ptr);
      while (~INBYTE(AIC7870[HCNTRL]) & PAUSEACK);

      /* Enqueue current scb as Next SCB for Sequencer to Execute   */
      ha_ptr->enquehead(scb_index,scb_ptr,base);

      hcntl_state = INBYTE(AIC7870[HCNTRL]) & ~PAUSE;
      Ph_WriteHcntrl(base, (UBYTE) (hcntl_state), config_ptr);

      return;
   }
}

#endif

/*********************************************************************
*  Ph_SoftReset
*
*  Determine state of target to be reset and handle inactive SCBs.
*
*  Return Value:  none
*                  
*  Parameters:    
*
*  Activation:    Ph_NonInit
*
*  Remarks:       
*                 
*********************************************************************/
void  Ph_SoftReset (cfp_struct *config_ptr, sp_struct *scb_ptr)
{

}

/*********************************************************************
*
*  Ph_GetScbStatus routine -
*
*  Get status of SCB
*
*  Return Value: SCB_WAITING or SCB_READY
*
*  Parameters: ha_ptr
*              scb_ptr
*
*  Activation: PH_ScbSend
*              Ph_TerminateCommand
*
*  Remarks:
*
*********************************************************************/
UBYTE Ph_GetScbStatus (hsp_struct *ha_ptr,
                      sp_struct *scb_ptr)
{
   UBYTE i, ret_val = SCB_READY;

   if (scb_ptr->SP_TagEnable)
   {
      i = ha_ptr->Hsp_MaxTagScbs;
   }
   else
   {
      i = ha_ptr->Hsp_MaxNonTagScbs;
   }
   if (ha_ptr->acttarg[scb_ptr->SP_Tarlun] > i)
   {
      ret_val = SCB_WAITING;
   }
   return(ret_val);
}

/*********************************************************************
*
*  Ph_SetMgrStat routine -
*
*  Set status value to MgrStat
*
*  Return Value:  None
*                  
*  Parameters: scb_ptr
*
*  Activation: PH_TerminateCommand
*                
*  Remarks:
*                  
*********************************************************************/
void Ph_SetMgrStat (sp_struct *scb_ptr )
{
   scb_ptr->SP_MgrStat = SCB_DONE;           /* Update status */
   if (scb_ptr->SP_HaStat)
   {
      scb_ptr->SP_MgrStat = SCB_DONE_ERR;
   }
   switch (scb_ptr->SP_TargStat)
   {
      case UNIT_GOOD:
      case UNIT_MET:
      case UNIT_INTERMED:
      case UNIT_INTMED_GD:
         break;
      default:
         scb_ptr->SP_MgrStat = SCB_DONE_ERR;
   }
}

/*********************************************************************
*  Ph_WriteBiosInfo -
*
*  Write BIOS information into hardware depending on mode of operation
*
*  Return Value:  none
*                  
*  Parameters:    config_ptr -
*
*  Activation:    BIOS OSM only
*
*  Remarks:       The place where BIOS information will be put is
*                 dependent on hardware. For Rev_B Lance it will be
*                 put on SCB 2. For Rev_C Lance and Dagger it will be put
*                 on new scratch bank.
*                 
*********************************************************************/

void Ph_WriteBiosInfo (cfp_struct *config_ptr,
                     UWORD offset,
                     UBYTE *buffer,
                     UWORD buflen)
{
   register AIC_7870 *base;
   UBYTE hcntrl_data,scb;

   base = config_ptr->CFP_Base;
   hcntrl_data = INBYTE(AIC7870[HCNTRL]);
   Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data | PAUSE), config_ptr);   /* Pause sequencer */
   while (~INBYTE(AIC7870[HCNTRL]) & PAUSEACK); /* and write the bios */
   scb = INBYTE(AIC7870[SCBPTR]);                   /* info into scb 2*/
   OUTBYTE(AIC7870[SCBPTR],2);
   Ph_OutBuffer((AIC_7870 *) &base->aic7870[SCB00+offset],buffer,buflen,config_ptr);
   OUTBYTE(AIC7870[SCBPTR],scb);
   Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data), config_ptr);
}

/*********************************************************************
*  Ph_ReadBiosInfo -
*
*  Read BIOS information from hardware depending on mode of operation
*
*  Return Value:  none
*                  
*  Parameters:    config_ptr -
*
*  Activation:    BIOS OSM only
*
*  Remarks:       The place where BIOS information will be read from is
*                 dependent on hardware. For Rev_B Lance it will be
*                 read from SCB 2. For Rev_C Lance and Dagger it will be
*                 read from new scratch bank.
*
*                 This routine is for HIM internal use and PH_GetBiosInfo
*                 is mainly for OSM to use.
*
*********************************************************************/

void Ph_ReadBiosInfo (cfp_struct *config_ptr,
                     UWORD offset,
                     UBYTE *buffer,
                     UWORD buflen)
{
   register AIC_7870 *base;
   UBYTE hcntrl_data;
   UBYTE scb;

   base = config_ptr->CFP_Base;
   hcntrl_data = INBYTE(AIC7870[HCNTRL]);
   Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data | PAUSE), config_ptr); /* Pause sequencer   */
   while (~INBYTE(AIC7870[HCNTRL]) & PAUSEACK); /* and read the bios */
   scb = INBYTE(AIC7870[SCBPTR]);                 /* from into scb 2 */
   OUTBYTE(AIC7870[SCBPTR],2);
   Ph_InBuffer((AIC_7870 *) &base->aic7870[SCB00+offset],buffer,buflen,config_ptr);
   OUTBYTE(AIC7870[SCBPTR],scb);
   Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data), config_ptr);
}

/*********************************************************************
*
*  Ph_OutBuffer routine -
*
*  Output buffer routine
*
*  Return Value:  none phase
*
*  Parameters     base - start port address
*                 buffer - buffer to be out
*                 buflen - bufer length
*
*  Activation:    
*
*  Remarks: This function should be used only when speed is not
*           critical
*
*********************************************************************/

void Ph_OutBuffer (register AIC_7870 *base,UBYTE *buffer,int buflen,
    cfp_struct *config_ptr)
{
   int i;

   for (i = 0; i < buflen; i++)
   {
      OUTBYTE(AIC7870[i],buffer[i]);
   }
}

/*********************************************************************
*
*  Ph_InBuffer routine -
*
*  Input buffer routine
*
*  Return Value:  none phase
*
*  Parameters     base - start port address
*                 buffer - buffer to be filled
*                 buflen - bufer length
*
*  Activation:    
*
*  Remarks: This function should be used only when speed is not
*           critical
*
*********************************************************************/

void Ph_InBuffer (register AIC_7870 *base,UBYTE *buffer,int buflen,
    cfp_struct *config_ptr)
{
   int i;

   for (i = 0; i < buflen; i++)
   {
      buffer[i] = INBYTE(AIC7870[i]);
   }
}
/*********************************************************************
*  Ph_ScbAbort -
*
*  - Aborts inactive SCBs
*  - Calls ha_ptr->abortactive() for active SCBs
*
*  Return Value:  
*                  
*  Parameters:    
*
*  Activation: PH_Special()
*
*  Remarks:       
*                 
*********************************************************************/
#ifdef   _FAULT_TOLERANCE
void Ph_ScbAbort ( sp_struct *scb_pointer)
{
   register cfp_struct *config_ptr = scb_pointer->SP_ConfigPtr;
   register AIC_7870 *base = config_ptr->CFP_Base;
   register hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   sp_struct *scb_ptr;
   sp_struct *tmp_ptr;

   /* Verify SCB exists */
   tmp_ptr = Ph_ChainPrevious(ha_ptr, scb_pointer);

   if(tmp_ptr == NOT_DEFINED)
   {
      return;
   }

   scb_ptr = scb_pointer;

   /* Take action based on current SCB state */

   switch(scb_ptr->SP_MgrStat)
   {
      case  SCB_ACTIVE:       /* Active SCB, call mode specific routine */
      case  SCB_AUTOSENSE:    /* Also an active SCB, generated an REQ SENSE */
         scb_ptr->SP_MgrStat = SCB_ABORTED;
         scb_ptr->SP_HaStat  = HOST_ABT_HOST;
         if (ha_ptr->abortactive( scb_pointer) == ABTACT_CMP)
         {
            Ph_TermPostNonActiveScb( scb_ptr);     /* Terminate and Post */
         }
         break;

      case  SCB_WAITING:
         scb_ptr->SP_HaStat  = HOST_ABT_HOST;
         scb_ptr->SP_MgrStat = SCB_ABORTED;
         Ph_PostNonActiveScb(config_ptr, scb_ptr); /* Terminate and Post */
         break;

      case  SCB_READY:
         /* Decrement Number of Available scb's                     */
         --ha_ptr->ready_cmd;
         scb_ptr->SP_HaStat  = HOST_ABT_HOST;
         scb_ptr->SP_MgrStat = SCB_ABORTED;
         Ph_TermPostNonActiveScb(scb_ptr);         /* Terminate and Post */
         break;

      case  SCB_PROCESS:      /* SCB not active, mark and post */
      case  SCB_DONE:         /* SCB already completed, just return */
      case  SCB_DONE_ABT:
      case  SCB_DONE_ERR:
      case  SCB_DONE_ILL:
      default:
         break;
   }
   return;
}
#endif

/*********************************************************************
*
*   SWAPCurrScratchRAM routine -
*
*   This routine SWAPS: the Current  SCRATCH RAM AREA 
*                WITH : the PREVIOUS SCRATCH RAM AREA that is stored 
*                       in Memory.
*
*  Return Value:  0x00      - successful
*                 (Use Return status, in case this option
*                  needs to be implemented at a later date)
*                  
*  Parameters:    config_ptr
*
*                 updateflag = 0 ---> Don't Update SCRATCH RAM from MEMORY
*                            = 1 --->       Update SCRATCH RAM from MEMORY
*
*  Activation:    PH_InitHa, scb_special
*                  
*  Remarks:                
*
*********************************************************************/
int SWAPCurrScratchRam (cfp_struct *config_ptr, UBYTE updateflag)
{
   AIC_7870 *base = config_ptr->CFP_Base;
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;

   UBYTE temp;
   /* UBYTE hc_data = 0;   */
   UWORD i, sav_bufstrt, sav_bufend;

   /* Limits of Restore Parameters for MEMORY --> PCI SCRATCH RAM Area */
   sav_bufstrt = 0x3b;
   sav_bufend  = 0x5f;

   /* SWAP 64 bytes from PCI Chip SCRATCH RAM -with- 64 bytes from  */
   /*  Hsp_SaveScr Array, if enabled                                */

   for (i = SCRATCH; i < (UWORD)(SCRATCH + 0x40); i++)
   {
      /* Get Current SCRATCH RAM Element & save temporarily         */
      temp = INBYTE(AIC7870[ i ]);

      /* Don't Restore Current Memory Save Element -UNLESS- it      */
      /*  Conforms to following requirements                        */

      if (updateflag && (i >= sav_bufstrt) && (i <= sav_bufend))
      {
         /* Move Current Memory Save Element into SCRATCH RAM       */
         /*  Location (SWAP ENABLED).                               */
         OUTBYTE(AIC7870[i], ha_ptr->Hsp_SaveScr[i - SCRATCH]);
      }
      /* Since the saved scratch area must, by definition, represent
         an idle HIM, QIN_CNT and the NEXT_SCB_ARRAY are cleared before
         being saved. */

      if (i == SCRATCH_QIN_CNT)
      {
         temp = 0;                     /* Set QIN_CNT to zero */ 
      }
      else if ((i >= (UWORD)(SCRATCH_NEXT_SCB_ARRAY)) &&
               (i <  (UWORD)(SCRATCH_NEXT_SCB_ARRAY + 16)))
      {
         temp = 0x7F;                  /* Set NEXT_SCB_ARRAY to null value */
      }
      /* Complete SWAP by moving Current SCRATCH RAM Element into   */
      /*  corresponding Memory SAVE AREA location                   */

      ha_ptr->Hsp_SaveScr[ i - SCRATCH ] = temp;
   }
   /* Reset sequencer address */

   OUTBYTE(AIC7870[SEQADDR0], IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(AIC7870[SEQADDR1], IDLE_LOOP_ENTRY >> 10);

   /* Do Specific Update of Current SCRATCH RAM Processing State on */
   /*  Return Back to Caller                                        */

   return(0);
}

/*********************************************************************
*
*  Ph_InsertBookmark routine -
*
*  This routine inserts a "bookmark" SCB allocated in him_struct onto
*  the queue. It is used to abort all commands up to, but not including
*  the bookmark. 
*
*  Return Value:  void
*                  
*  Parameters:    config_ptr
*
*  Activation:    intrst
*                 ha_hard_reset
*                  
*  Remarks:       INTR_OFF (i.e. system interrupts off) must be executed
*                 prior to calling this routine.
*       
*********************************************************************/
void Ph_InsertBookmark (cfp_struct *config_ptr)
{
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   sp_struct *mark_ptr = &(ha_ptr->scb_mark);

   mark_ptr->SP_Cmd  = BOOKMARK;
/*   mark_ptr->SP_Next = NOT_DEFINED;  */

   Ph_ChainAppendEnd(config_ptr, mark_ptr);
}
/*********************************************************************
*
*  Ph_RemoveBookmark routine -
*
*  This routine removes a "bookmark" SCB from the queue.
*
*  Return Value:  void
*                  
*  Parameters:    ha_ptr
*
*  Activation:    intrst
*                 ha_hard_reset
*                  
*  Remarks:       INTR_OFF (i.e. system interrupts off) must be executed
*                 prior to calling this routine.
*
*********************************************************************/
void Ph_RemoveBookmark (cfp_struct *config_ptr)
{
   Ph_ChainRemove(config_ptr,&config_ptr->CFP_HaDataPtr->scb_mark);
}
/*********************************************************************
*
*  Ph_AsynchEvent -
*
*  Notify host of an asynchronous event.
*
*  Return Value:  0x00 - Notification Successful
*                 0xff - Notification Unsuccessful
*
*  Parameters:
*
*  Activation:
*
*  Remarks:
*
*********************************************************************/
int Ph_AsynchEvent (DWORD EventType,
                    cfp_struct *config_ptr, void *data)
{
   if ((config_ptr->CFP_ExtdCfg) &&
       ((config_ptr->Cf_CallBack[CALLBACK_ASYNC]) != (FCNPTR)(NOT_DEFINED)))
   {
      config_ptr->Cf_CallBack[CALLBACK_ASYNC]( EventType, config_ptr, data );
      return(NOERR);
   }
   return(ADP_ERR);
}

#ifdef _TARGET_MODE
/*********************************************************************
*
*  Ph_SetTargetSCB -
*
*     Compute Physical Address of CDB.
*
*  Return Value:  NONE
*
*  Parameters:    config_ptr
*
*  Activation:    Ph_SetOptimaScratch
*                  
*  Remarks:       
*
*********************************************************************/
void  Ph_SetTargetSCB(cfp_struct *config_ptr)
{
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   Target_SCB *tgtscb_ptr = &(ha_ptr->target_scb);

   DWORD addr_buf, phys_cdbaddr;

   /* Get logical offset to start of "scb_mark" portion of hsp structure */
   addr_buf =   (DWORD) tgtscb_ptr - (DWORD) ha_ptr;

   /* Compute Physical Address to start of "scb_mark" portion of hsp structure */
   addr_buf = addr_buf + (DWORD)(config_ptr -> Cf_HaDataPhy);

   /* Calculate address to Start of CDB for use by Sequencer   */
   phys_cdbaddr = addr_buf + ((DWORD) tgtscb_ptr->Sp_CDB - (DWORD) tgtscb_ptr);

   /* Store Physical CDB Address into scb_mark structure */
   tgtscb_ptr->Sp_seq_scb.target_seq_struct.CDBPtr = phys_cdbaddr;

   /* Store Pointer to Config Structure into scb_mark structure */
   tgtscb_ptr->SP_ConfigPtr = config_ptr;
   tgtscb_ptr->Sp_seq_scb.target_seq_struct.TargetScb = 1;
}

#endif      /* of _TARGET_MODE  */

