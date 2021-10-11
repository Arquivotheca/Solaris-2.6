/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

/* $Header:   Y:/source/aic-7870/him/common/him.cv_   1.89.1.2   14 Aug 1995 11:44:36   YU1868  $ */

#pragma ident	"@(#)adp_him.c	1.10	96/03/13 SMI"
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
*  Module Name:   HIM.C
*
*  Description:
*                 Codes common to HIM at run time are defined here. It should
*                 always be included independent of configurations and modes
*                 of operation. These codes should always remain resident.
*
*  Programmers:   Paul von Stamwitz
*                 Chuck Fannin
*                 Jeff Mendiola
*                 Harry Yang
*    
*  Notes:         NONE
*
*  Entry Point(s):
*     PH_EnableInt - Enable AIC-7870 interrupt
*     PH_DisableInt - Disable interrupt
*     PH_Special - Abort, Host Adapter reset operations.
*
*  Revisions -
*
***************************************************************************/

#include "him_scb.h"
#include "him_equ.h"
#include "seq_off.h"

/*********************************************************************
*
*  PH_EnableInt
*
*  Enable AIC-7870 interrupt
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

void PH_EnableInt (cfp_struct *config_ptr)
{
   AIC_7870 *base = config_ptr->CFP_Base;

   Ph_WriteHcntrl(base, (UBYTE) (INBYTE(AIC7870[HCNTRL]) | INTEN), config_ptr);
}

/*********************************************************************
*
*  PH_DisableInt
*
*  Disable AIC-7870 interrupt
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

void PH_DisableInt (cfp_struct *config_ptr)
{
   AIC_7870 *base = config_ptr->CFP_Base;

   Ph_WriteHcntrl(base, (UBYTE) (INBYTE(AIC7870[HCNTRL]) & ~INTEN), config_ptr);
}

/*********************************************************************
*  Ph_NonInit routine -
*
*  Parse non-initiator command request, activate abort, device reset
*  or read sense routines.
*
*  Return Value:  
*                  
*  Parameters:    
*
*  Activation: scb_send
*
*  Remarks:       
*                 
*********************************************************************/
#ifdef   _FAULT_TOLERANCE

UBYTE Ph_NonInit (sp_struct *scb_pointer)
{
   cfp_struct *config_ptr=scb_pointer->SP_ConfigPtr;
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   UBYTE retval = 0;

   switch (scb_pointer->SP_Cmd)
   {
      case HARD_RST_DEV:
         Ph_BusDeviceReset(scb_pointer);
         break;

      case NO_OPERATION:
         break;

      default:
         /*@ scb_enque( config_ptr, scb_pointer);                   */
         /*@ scb_pointer->SCB_MgrStat = SCB_DONE_ILL;               */
         /*@ ++ha_ptr->done_cmd;                                    */

         Ph_ChainAppendEnd(config_ptr, scb_pointer);
         scb_pointer->SP_MgrStat = SCB_DONE_ILL;
         ++DONE_CMD;
   }
   return (retval);
}
#endif   /* _FAULT_TOLERANCE */

/*********************************************************************
*
*  PH_Special routine -
*
*  Perform command not requiring an SCB: Abort,
*                                        Soft Reset,
*                                        Hard Reset
*
*  Return Value:  0 = Reset completed. (Also returned for abort opcode)
*                 1 = Soft reset failed, Hard reset performed. 
*                 2 = Reset failed, hardware error.
*              0xFF = Busy could not swap sequencers  MDL 
*                  
*  Parameters: UBYTE Opcode: 00 = Abort SCB
*                            01 = Soft Reset Host Adapter
*                            02 = Hard Reset Host Adapter
*
*              sp_struct *: ptr to SCB for Abort, NOT_DEFINED otherwise.
*
*  Activation:    
*              Ph_ScbSend
*  Remarks:       
*              A subset of the full PH_Special() has been created under
*              the conditional compile of _LESS_CODE.  This subset
*              version was created to accomodate BIOS in generating code
*              when BIOS call the FORCE_RENEGOTIATE function.
*
*********************************************************************/
#ifdef   _FAULT_TOLERANCE
int PH_Special ( UBYTE spec_opcode,
                   cfp_struct *config_ptr,
                   sp_struct *scb_ptr)
{
   AIC_7870 *base = config_ptr->CFP_Base;
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   register UBYTE hcntrl_data;
   UBYTE tarlun;
   /* UBYTE bus_rst = 0;   */
   UBYTE updateflag;

   int retval = 0;
   /* int debug_cnt = 0;   */
   UWORD semaphore;
   int orgint;

   INTR_OFF_SAV(orgint);
   semaphore = PHM_SEMSTATE(ha_ptr);
   PHM_SEMSET(ha_ptr,SEM_LOCK);

   switch (spec_opcode)
   {
      case ABORT_SCB:

         Ph_ScbAbort(scb_ptr);   
         break;

      case HARD_HA_RESET:
         Ph_HaHardReset (config_ptr);
         break;

      case FORCE_RENEGOTIATE:

         /* Pause sequencer                                         */
         hcntrl_data = INBYTE(AIC7870[HCNTRL]);
         Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data | PAUSE), config_ptr);
         while (~INBYTE(AIC7870[HCNTRL]) & PAUSEACK);

         tarlun = (UBYTE)(scb_ptr->SP_Tarlun);
         Ph_ScbRenego(config_ptr, tarlun);  

         /* Unpause sequencer                                       */
         hcntrl_data = INBYTE(AIC7870[HCNTRL]);
         Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data & ~PAUSE), config_ptr);
         break;

      case REALIGN_DATA:
         break;

      case BIOS_ON:
         /* EXIT, If STATE = REAL Mode Enabled. BIOS_ON has Already */
         /*                  been Completed & SCRATCH RAM has been  */
         /*                  SWAPPED with MEMORY.                   */
         if (ha_ptr->Hsp_SaveState == SCR_REALMODE)
         {
            break;
         }
         /* wait for all I/O activity to finish */                   
         /* busy back to driver, please try again */               
         if (ha_ptr->Head_Of_Queue != (sp_struct * ) NOT_DEFINED) 
         {
            retval = -1;                                          
            break;
         }
         /* pause */                                                 
         hcntrl_data = INBYTE(AIC7870[HCNTRL]); 
         Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data | PAUSE), config_ptr);
         while (~INBYTE(AIC7870[HCNTRL]) & PAUSEACK);
        
         /* Setup SCRATCH for REAL Mode. SWAP Entire 64 bytes of PCI*/
         /*  SCRATCH RAM with MEMORY Save Area.                     */
         updateflag = 1;
         /* Swap Scratch-RAM */                                      
         retval = SWAPCurrScratchRam(config_ptr, updateflag);

         /* Set STATE = REAL Mode Enabled. BIOS_ON Completed.SCRATCH*/
         /*             RAM has been SWAPPED with MEMORY.           */  
         ha_ptr->Hsp_SaveState = SCR_REALMODE;
         Ph_WriteHcntrl(base, (UBYTE) hcntrl_data, config_ptr);
         break;

      case H_BIOS_OFF:

         /* pause */                                                 
         hcntrl_data = INBYTE(AIC7870[HCNTRL]); 
         Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data | PAUSE), config_ptr);   
         while (~INBYTE(AIC7870[HCNTRL]) & PAUSEACK);

         /* Setup SCRATCH for PROTECTED Mode. SWAP Entire 64 bytes  */
         /*  of PCI SCRATCH & Restore Only those Bytes that are     */
         /*  required from the MEMORY Save Area.                    */
         updateflag = 1;
         retval = SWAPCurrScratchRam(config_ptr, updateflag);

         /* Set STATE = PROTECTED Mode Enabled. H_BIOS_OFF Completed.*/
         /*             SCRATCH RAM has been Restored with Previously*/
         /*             Saved MEMORY.                                */  
         ha_ptr->Hsp_SaveState = SCR_PROTMODE;
         Ph_WriteHcntrl(base, (UBYTE) hcntrl_data, config_ptr);
         break;
         
      case BIOS_OFF:
         break;                                                    

      default:
         break;
   }
   if ((spec_opcode == HARD_HA_RESET) || (spec_opcode == ABORT_SCB))
   {
      if (semaphore == SEM_RELS)
      {
         if (Ph_SendCommand(ha_ptr, base))
         {
            Ph_WriteHcntrl(base, (UBYTE) (INBYTE(AIC7870[HCNTRL]) | SWINT), config_ptr);
         }
      }
   }
   PHM_SEMSET(ha_ptr,semaphore);
   INTR_RESTORE(orgint);
   return (retval);
}

#endif         /* _FAULT_TOLERANCE  */


/****************      F  O  R     B  I  O  S    C  A  L  L   **************/

#ifdef _LESS_CODE             /* for BIOS FORCE_RENEGOTIATE function call  */
int PH_Special ( UBYTE spec_opcode,
                   cfp_struct *config_ptr,
                   sp_struct *scb_ptr)
{
   AIC_7870 *base = config_ptr->CFP_Base;
   register UBYTE hcntrl_data;
   UBYTE tarlun;

   int retval;
   int orgint;

   retval = 0;
   orgint = 0;

   INTR_OFF_SAV(orgint);

   switch (spec_opcode)
   {
      case FORCE_RENEGOTIATE:

         /* Pause sequencer                                         */
         hcntrl_data = INBYTE(AIC7870[HCNTRL]);
         Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data | PAUSE), config_ptr);   
         while (~INBYTE(AIC7870[HCNTRL]) & PAUSEACK);

         tarlun = (UBYTE)(scb_ptr->SP_Tarlun);
         Ph_ScbRenego(config_ptr, tarlun);  

         /* Unpause sequencer                                       */
         hcntrl_data = INBYTE(AIC7870[HCNTRL]);
         Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data & ~PAUSE), config_ptr);
         break;

      case REALIGN_DATA:
         break;

      default:
         break;
   }
   return (retval);
}
#endif         /* _LESS_CODE  */

/*********************************************************************
*  Ph_Abort -
*
*  Determine state of SCB to be aborted and abort if not ACTIVE.
*
*  Return Value:  
*                  
*  Parameters:    
*
*  Activation: Ph_Special
*
*  Remarks:       
*                 
*********************************************************************/
#ifdef   _FAULT_TOLERANCE
void Ph_Abort (sp_struct *scb_ptr)
{
}
#endif   /* _FAULT_TOLERANCE */

/*********************************************************************
*
*  Ph_IntSrst routine -
*
*  Process case where other device resets scsi bus
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*
*  Activation: PH_IntHandler
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_IntSrst (cfp_struct *config_ptr)
{
   register AIC_7870 *base;
   UBYTE HaStatus;

   base = config_ptr->CFP_Base;

   while (INBYTE(AIC7870[SSTAT1]) & SCSIRSTI)
   {
      OUTBYTE(AIC7870[CLRSINT1], CLRSCSIRSTI);
   }

   PHM_ASYNCHEVENT(AE_3PTY_RST, config_ptr, 0L);   /* Asynchronous Event */

   if (config_ptr->CFP_DriverIdle == 0)
   {
      PHM_INSERTBOOKMARK(config_ptr);     /* Insert "Bookmark" in queue */

      OUTBYTE(AIC7870[SCSISEQ], (INBYTE(AIC7870[SCSISEQ]) &
             ~(TEMODEO+ENSELO+ENAUTOATNO+ENAUTOATNI)));       /* Disarm any outstanding selections */
      if (INBYTE(AIC7870[SCSISIG]))       /* If bus still not free...          */
      {
         OUTBYTE(AIC7870[SXFRCTL0], (CLRSTCNT+CLRCHN));
         Ph_ResetSCSI(base, config_ptr);              /* Reset it again!                   */
      }
      /* There had been a SCSI reset from a 3rd party, check sync anyway   */
      Ph_CheckSyncNego(config_ptr);       /* Adjust the sync nego parameters   */
      Ph_ResetChannel(config_ptr);

      HaStatus = HOST_ABT_HA;
      PHM_ABORTCHANNEL(config_ptr, HaStatus);

      PHM_REMOVEBOOKMARK(config_ptr);
   }
   else                                   /* if DriverIdle == 1   */
   {
      PHM_INSERTBOOKMARK(config_ptr);     /* just in case   */
#ifdef _TARGET_MODE
      OUTBYTE(AIC7870[SCSISEQ], (INBYTE(AIC7870[SCSISEQ]) &
       ~(TEMODEO+ENSELO+ENAUTOATNO+ENAUTOATNI))); /* Disarm any outstanding selections */
      if (INBYTE(AIC7870[SCSISIG]))       /* If bus still not free...    */
      {
         OUTBYTE(AIC7870[SXFRCTL0], (CLRSTCNT+CLRCHN));
         Ph_ResetSCSI(base, config_ptr);              /* Reset it again!              */
      }
#endif   /* of _TARGET_MODE   */      
      Ph_CheckSyncNego(config_ptr);       /* Adjust the sync nego parameters   */
      Ph_ResetChannel(config_ptr);
      PHM_REMOVEBOOKMARK(config_ptr);
   }

   OUTBYTE(AIC7870[SEQADDR0], (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(AIC7870[SEQADDR1], (UBYTE) IDLE_LOOP_ENTRY >> 10);
}

/*********************************************************************
*
*  Ph_ResetChannel routine -
*
*  Reset SCSI bus and clear all associated interrupts.
*  Also clear synchronous / wide mode.
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*
*  Activation: Ph_InitHA
*              Ph_BadSeq
*              Ph_HaHardReset
*              Ph_IntSrst
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_ResetChannel (cfp_struct *config_ptr)
{
   register AIC_7870 *base;
   UBYTE i;
   UBYTE j;

   base = config_ptr->CFP_Base;

   OUTBYTE(AIC7870[SCSISEQ], (INBYTE(AIC7870[SCSISEQ]) &
              ~(TEMODEO+ENSELO+ENAUTOATNO+ENAUTOATNI)));
   OUTBYTE(AIC7870[CLRSINT0], 0xff);
   OUTBYTE(AIC7870[CLRSINT1], 0xff); /* pvs 5/15/94 */

   OUTBYTE(AIC7870[SXFRCTL0], CLRSTCNT|CLRCHN);
   OUTBYTE(AIC7870[SXFRCTL1],
           ((UBYTE)config_ptr->CFP_ConfigFlags & (STIMESEL + SCSI_PARITY)) |
            (ENSTIMER + ACTNEGEN +    /* low byte termination get set here */
            ((UBYTE)(config_ptr->CFP_TerminationLow) ? STPWEN : 0)));

   OUTBYTE(AIC7870[DFCNTRL], FIFORESET);

   OUTBYTE(AIC7870[SIMODE1], ENSCSIPERR + ENSELTIMO + ENSCSIRST);

   if (config_ptr->CFP_MultiTaskLun)       /* Re-Initialize internal SCB's */
   {
      j = 128;
   }
   else
   {
      j = 16;
   }
   for (i = 0; i < j; i++)
   {
      PHM_INDEXCLEARBUSY(config_ptr, i);
   }
   OUTBYTE(AIC7870[SCBPTR], 00);
}

/*********************************************************************
*
*  Ph_CheckSyncNego routine -
*
*  Readjust the synchronous negotiation parameters based
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*
*  Activation: Ph_InitHA
*              Ph_BadSeq
*              Ph_HaHardReset
*              Ph_IntSrst
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_CheckSyncNego (cfp_struct *config_ptr)

{
   register AIC_7870 *base;
   UBYTE i;

   base = config_ptr->CFP_Base;

   /* Re-Initialize sync/wide negotiation parameters depending on */
   /* whether SuppressNego flag is set                            */
   for (i = 0; i < config_ptr->Cf_MaxTargets; i++)   
   {
      switch(config_ptr->CFP_SuppressNego)
      {
         case 0:           /* no suppress on negotiation */
            if (config_ptr->Cf_ScsiOption[i] & (WIDE_MODE | SYNC_MODE))
            {
               /*  Setup Sequencer XFER_OPTION with NEEDNEGO (Need to Negotiate) */
               Ph_SetNeedNego(i, base, config_ptr);
            }
            else
            {
               OUTBYTE(AIC7870[XFER_OPTION + i], 00);
            }         
            break;

         case 1:
            /* if OSM is doing scanning and suppress negotiaiton, */
            /* sequencer scratch RAM will be loaded with 0x00     */
            OUTBYTE(AIC7870[XFER_OPTION + i], 00);
            break;
      }
   }
}

/*********************************************************************
*
*  Ph_CheckLength routine -
*
*  Check for underrun/overrun conditions following data transfer
*
*  Return Value: None
*                  
*  Parameters: scb_ptr
*              base address of AIC-7870
*
*  Activation: PH_IntHandler
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_CheckLength (sp_struct *scb_ptr,
                     register AIC_7870 *base)
{
   gen_union res_lng;
   UBYTE phase;
   UBYTE i;
   cfp_struct *config_ptr=scb_ptr->SP_ConfigPtr;

   /* underrun if SCSI bus at STATUS phase */
   if ((phase = (INBYTE(AIC7870[SCSISIG]) & BUSPHASE)) == STPHASE)
   {
      if (scb_ptr->SP_NoUnderrun)   /* do nothing if underrun error supressed */
      {
         return;
      }
      if (scb_ptr->SP_TargStat == UNIT_CHECK)
      {
         return;
      }
      for (i = 0; i < 4; i++)
      {
         res_lng.ubyte[i] = INBYTE(AIC7870[SCB16 + i]);
      }
      scb_ptr->SP_ResCnt = res_lng.dword;
   }
   else if ((phase & CDO) == 0)     /* overrun if SCSI bus at Data phase */
   {
      OUTBYTE(AIC7870[SCSISIG], phase);
      i = INBYTE(AIC7870[SXFRCTL1]);
      OUTBYTE(AIC7870[SXFRCTL1], i | BITBUCKET);
      while ((INBYTE(AIC7870[SSTAT1]) & PHASEMIS) == 0);
      scb_ptr->SP_ResCnt = (DWORD)0;      /* SP_ResCnt should be zero */
      OUTBYTE(AIC7870[SXFRCTL1], i);
   }
   scb_ptr->SP_HaStat = HOST_DU_DO;
}

/*********************************************************************
*
*  Ph_CdbAbort routine -
*
*  Send SCSI abort msg to selected target
*
*  Return Value:  None
*                  
*  Parameters:    scb_ptr
*                 base address of AIC-7870
*
*  Activation:    PH_IntHandler
*                  
*  Remarks:       limited implementation, at present
*                  
*********************************************************************/
#ifndef _LESS_CODE
void Ph_CdbAbort (sp_struct *scb_ptr,
                  register AIC_7870 *base)
{
   cfp_struct *config_ptr;
   UBYTE phase;

   config_ptr = scb_ptr->SP_ConfigPtr;
   if ((INBYTE(AIC7870[SCSISIG]) & BUSPHASE) != MIPHASE)
   {
      Ph_BadSeq(config_ptr, base);
      return;
   }
   if (INBYTE(AIC7870[SCSIBUSL]) == MSG03)
   {
      OUTBYTE(AIC7870[SCSISIG], MIPHASE);
      INBYTE(AIC7870[SCSIDATL]);
      OUTBYTE(AIC7870[SEQADDR0], (UBYTE) SIOSTR3_ENTRY >> 2);
      OUTBYTE(AIC7870[SEQADDR1], (UBYTE) SIOSTR3_ENTRY >> 10);
   }
   else
   {
      OUTBYTE(AIC7870[SCSISIG], MIPHASE | ATNO);
      do
      {
         INBYTE(AIC7870[SCSIDATL]);
         phase = Ph_Wt4Req(scb_ptr, base);
      } while (phase == MIPHASE);
      if (phase != MOPHASE)
      {
         Ph_BadSeq(config_ptr, base);
         return;
      }
      OUTBYTE(AIC7870[SCSISIG], MOPHASE);
      OUTBYTE(AIC7870[CLRSINT1], CLRATNO);
      OUTBYTE(AIC7870[SCSIDATL], MSG07);
      Ph_Wt4Req(scb_ptr, base);
      OUTBYTE(AIC7870[SEQADDR0], (UBYTE) SIO204_ENTRY >> 2);
      OUTBYTE((AIC7870[SEQADDR1]), (UBYTE) SIO204_ENTRY >> 10);
   }
}
#endif /* _LESS_CODE */
/*********************************************************************
*
*  Ph_ResetSCSI routine -
*
*  Perform SCSI bus reset
*
*  Return Value:  None
*                  
*  Parameters:    base address of AIC-7870
*
*  Activation:    stb_init
*                 Ph_BadSeq
*                  
*  Remarks:       Here we are going to use sequencer timer to control
*                 SCSI reset assert timing.
*                  
*********************************************************************/
void Ph_ResetSCSI (AIC_7870 *base, cfp_struct *config_ptr)
{
   UBYTE scb16, scb17;
   UBYTE simode0_value;
   UBYTE intstat;

   scb16 = INBYTE(AIC7870[SCB16]);           /* save SCB16 and SCB17 */
   scb17 = INBYTE(AIC7870[SCB17]);
   OUTBYTE(AIC7870[SCB16], 0xff);            /* set timer to 64 msec */
   OUTBYTE(AIC7870[SCB17], 0x0);
   OUTBYTE(AIC7870[SEQADDR0], (UBYTE) ATN_TMR_ENTRY >> 2);
   OUTBYTE(AIC7870[SEQADDR1], (UBYTE) ATN_TMR_ENTRY >> 10);
   simode0_value = INBYTE(AIC7870[SIMODE0]); /* Disable all SCSI */
   OUTBYTE(AIC7870[SIMODE0],0);
   OUTBYTE(AIC7870[SIMODE1],0);              /* clear interrupt */
   OUTBYTE(AIC7870[CLRINT], CLRSEQINT | CLRSCSINT | CLRCMDINT | CLRBRKINT);
   OUTBYTE(AIC7870[SCSISEQ], (INBYTE(AIC7870[SCSISEQ]) &
             ~(TEMODEO+ENSELO+ENAUTOATNO+ENAUTOATNI) | SCSIRSTO));      /* assert RESET SCSI bus   */
   Ph_UnPause(base, config_ptr);                         /* unpause sequencer */

   while (!(INBYTE(AIC7870[HCNTRL]) & PAUSEACK))
      ;                                      /* wait for seq'er to expire  */

   OUTBYTE(AIC7870[SCSISEQ], (INBYTE(AIC7870[SCSISEQ]) & ~SCSIRSTO));   /* remove RESET SCSI bus   */
   OUTBYTE(AIC7870[CLRSINT1], CLRSCSIRSTI);  /* Patch for unexpected */
                                             /* scsi interrupt       */
   /* Clear any possible pending scsi interrupt that may exists   */
   /* at this point.                                              */
   intstat = INBYTE(AIC7870[SSTAT1]);
   OUTBYTE(AIC7870[CLRSINT0], 0xff);
   OUTBYTE(AIC7870[CLRSINT1], (intstat & ~PHASEMIS));

   OUTBYTE(AIC7870[CLRINT], CLRSEQINT | CLRSCSINT);     /* clear interrupt */

   Ph_Delay(base, 4000, config_ptr);      /* delay 2 sec = 4000 * 500 usec */

   OUTBYTE(AIC7870[SIMODE0],simode0_value); /* Restore SIMODE0 and */
   OUTBYTE(AIC7870[SIMODE1], ENSCSIPERR + ENSELTIMO + ENSCSIRST);
   OUTBYTE(AIC7870[SCB16], scb16);            /* restore SCB16 and */
   OUTBYTE(AIC7870[SCB17], scb17);                       /* SCB 17 */
}

/*********************************************************************
*
*  Ph_BadSeq routine -
*
*  Terminate SCSI command sequence because sequence that is illegal,
*  or if we just can't handle it.
*
*  Return Value:  None
*                  
*  Parameters: scb_ptr
*              base address of AIC-7870
*
*  Activation: PH_IntHandler
*              Ph_CdbAbort
*              Ph_ExtMsgi
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_BadSeq (cfp_struct *config_ptr,
                register AIC_7870 *base)
{
   UBYTE HaStatus;

   PHM_ASYNCHEVENT(AE_HA_RST, config_ptr, 0L);   /* Asynch. Event Notification */

   /* Don't reset/abort if there is a SCSI bus free interrupt pending */
   if (!((Ph_ReadIntstat(base, config_ptr) & SCSIINT) && (INBYTE(AIC7870[SSTAT1]) & BUSFREE)))
   {
      Ph_ResetSCSI(base, config_ptr);
      Ph_CheckSyncNego(config_ptr);       /* Adjust the sync nego parameters   */
      Ph_ResetChannel(config_ptr);
      HaStatus = HOST_PHASE_ERR;
      PHM_ABORTCHANNEL(config_ptr,HaStatus);
   }

   OUTBYTE(AIC7870[SEQADDR0], (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(AIC7870[SEQADDR1], (UBYTE) IDLE_LOOP_ENTRY >> 10);
}
/*********************************************************************
*
*  Ph_CheckCondition routine -
*
*  handle response to target check condition
*
*  Return Value:  None
*                  
*  Parameters:    scb_ptr
*                 base address of AIC-7870
*
*  Activation:    PH_IntHandler
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_CheckCondition (sp_struct *scb_ptr,
                        register AIC_7870 *base)
{
   UBYTE i;
   UBYTE scb;
   UBYTE status;
   cfp_struct *config_ptr=scb_ptr->SP_ConfigPtr;
      
   if (PHM_UNITCHECKCONDITION(scb_ptr))   /* check condition on request sense command */
   {
      scb_ptr->SP_HaStat = HOST_SNS_FAIL;
      status = 0;
   }
   else 
   {
      status = INBYTE(AIC7870[PASS_TO_DRIVER]);
      scb_ptr->SP_TargStat = status;
   }
   scb = INBYTE(AIC7870[ACTIVE_SCB]);
   if ((status == UNIT_CHECK) && (scb_ptr->SP_AutoSense))
   {
      scb_ptr->SP_NegoInProg = 0;
      PHM_REQUESTSENSE(scb_ptr,scb);
   }
   else
   {
      /* clear target busy must be done here also */
      PHM_CLEARTARGETBUSY(scb_ptr->SP_ConfigPtr,scb);
      Ph_TerminateCommand(scb_ptr, scb);
   }

   /* Reset synchronous/wide negotiation only for CHECK CONDITION */
   /* reset sync/wide as long as configured to do so   */
   /* even if it's negotiated without sync/wide        */
   if (status == UNIT_CHECK)
   {
     i = (UBYTE)(((UBYTE)scb_ptr->SP_Tarlun) & TARGET_ID) >> 4;

     switch(config_ptr->CFP_SuppressNego)
     {
         case 0:
            if (scb_ptr->SP_ConfigPtr->Cf_ScsiOption[i] & (WIDE_MODE | SYNC_MODE))
            {
               OUTBYTE(AIC7870[XFER_OPTION + i], NEEDNEGO);
            }
            break;
         case 1:
            OUTBYTE(AIC7870[XFER_OPTION + i], 0x00);
            break;
     }
   }

   OUTBYTE(AIC7870[SXFRCTL0], INBYTE(AIC7870[SXFRCTL0]) | CLRCHN);
   OUTBYTE(AIC7870[SIMODE1], INBYTE(AIC7870[SIMODE1]) & ~ENBUSFREE);
   INBYTE(AIC7870[SCSIDATL]);
#ifdef _TARGET_MODE
   OUTBYTE(AIC7870[SCSISIG], 0);    /* To fix the hardware bug on SCSISIG */
#endif   /* of _TARGET_MODE   */
   OUTBYTE(AIC7870[SEQADDR0], (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(AIC7870[SEQADDR1], (UBYTE) IDLE_LOOP_ENTRY >> 10);
}

/*********************************************************************
*
*  Ph_TargetAbort routine -
*
*  Abort current target
*
*  Return Value: none
*             
*  Parameters: scb_ptr
*              base address of AIC-7870
*
*  Activation: PH_IntHandler
*              Ph_HandleMsgi
*             
*  Remarks:   
*             
*********************************************************************/
#ifndef _LESS_CODE
UBYTE Ph_TargetAbort (cfp_struct *config_ptr,
                      sp_struct *scb_ptr,
                      register AIC_7870 *base)
{
   UBYTE abt_msg = MSG06, i;

   if ((Ph_ReadIntstat(base, config_ptr) & INTCODE) == NO_ID_MSG)
   {
      OUTBYTE(AIC7870[SCSISIG], MIPHASE | ATNO);
      INBYTE(AIC7870[SCSIDATL]);
   }
   if (Ph_Wt4Req(scb_ptr, base) == MOPHASE)
   {
      if (INBYTE(AIC7870[SCRATCH_ACTIVE_SCB]) != NULL_SCB)     /* SCB pointer valid?  */
      {
         if (scb_ptr->SP_MgrStat == SCB_ABORTINPROG)
         {
            scb_ptr->SP_MgrStat = SCB_ABORTED;
            return(0);
         }
         else if (scb_ptr->SP_MgrStat == SCB_BDR)
         {
            return(0);
         }
         if (INBYTE(AIC7870[SCB01]) & TAG_ENABLE)
         {
            abt_msg = MSG0D;           /* Abort tag */
         }
      }
#ifdef   _FAULT_TOLERANCE
      if (Ph_SendTrmMsg(config_ptr, abt_msg) & BUSFREE)
      {
         OUTBYTE(AIC7870[CLRSINT1], CLRBUSFREE);
         OUTBYTE(AIC7870[CLRINT], CLRSCSINT);
         i = INBYTE(AIC7870[SCBPTR]);
         if (PHM_SCBACTIVE(config_ptr,i))
         {
            OUTBYTE(AIC7870[SCB01], 00);
            scb_ptr->SP_HaStat = HOST_ABT_HA;
            scb_ptr->SP_MgrStat = SCB_ABORTED;
            Ph_TerminateCommand(scb_ptr, i);
            PHM_POSTCOMMAND(config_ptr);
            OUTBYTE(AIC7870[SEQADDR0], (UBYTE) IDLE_LOOP_ENTRY >> 2);
            OUTBYTE(AIC7870[SEQADDR1], (UBYTE) IDLE_LOOP_ENTRY >> 10);
         }
         return(0);
      }
#endif   /* _FAULT_TOLERANCE */
   }
   Ph_BadSeq(config_ptr, base);
   return(1);
}
#endif /* _LESS_CODE */
/*********************************************************************
*  Ph_SendTrmMsg routine -
*
*  Send termination message out (Abort or Bus Device Reset) to target.
*
*  Return Value: High 3 bits - Bus Phase from SCSISIG
*                Bit 3 - Bus Free from SSTAT1
*                Bit 0 - Reqinit from SSTAT1
*
*  Parameters: term_msg - Message to send (Bus Device Reset,
*                         Abort or Abort Tag)
*
*              tgtid    - SCSI ID of target to send message to.
*
*              scsi_state - 0C : No SCSI devices connected.
*                           00 : Specified target currently connected.
*                           88 : Specified target selection in progress.
*                           40 : Other device connected.
*
*  Activation: Ph_AbortActive
*
*  Remarks:
*
*********************************************************************/
#ifdef   _FAULT_TOLERANCE
UBYTE Ph_SendTrmMsg ( cfp_struct *config_ptr,
                         UBYTE term_msg)
{
   return(0);
}
#endif   /* _FAULT_TOLERANCE */

/*********************************************************************
*  Ph_Wt4BFree routine -
*
*  Wait for bus free
*
*  Return Value: High 3 bits - Bus Phase from SCSISIG
*                Bit 3 - Bus Free from SSTAT1
*                Bit 0 - Reqinit from SSTAT1
*
*  Parameters: term_msg - Message to send (Bus Device Reset,
*                         Abort or Abort Tag)
*
*              tgtid    - SCSI ID of target to send message to.
*
*              scsi_state - 0C : No SCSI devices connected.
*                           00 : Specified target currently connected.
*                           88 : Specified target selection in progress.
*                           40 : Other device connected.
*
*  Activation: Ph_AbortActive
*
*  Remarks:
*
*********************************************************************/
/*
#ifdef   _FAULT_TOLERANCE
UBYTE Ph_Wt4BFree (cfp_struct *config_ptr)
{
}
#endif
*/
/*********************************************************************
*  Ph_TrmCmplt routine -
*
*  Process terminate and complete
*
*  Return Value:
*
*  Parameters:
*
*  Activation: PH_IntHandler
*              Ph_ProcBkpt
*
*  Remarks:
*
*********************************************************************/
#ifdef   _FAULT_TOLERANCE
UBYTE Ph_TrmCmplt ( cfp_struct *config_ptr,
                        UBYTE busphase,
                        UBYTE term_msg)
{
   return(0);
}
#endif   /* _FAULT_TOLERANCE */

/*********************************************************************
*  Ph_ProcBkpt routine -
*
*  Process break point interrupt
*
*  Return Value:
*
*  Parameters:
*
*  Activation: PH_IntHandler
*
*  Remarks:
*
*********************************************************************/
#ifdef   _FAUL_TOLERANCE
UBYTE Ph_ProcBkpt (cfp_struct *config_ptr)
{
}
#endif   /* _FAULT_TOLERANCE */

/*********************************************************************
*  Ph_BusReset routine -
*
*  Perform SCSI Bus Reset and clear SCB queue.
*
*  Return Value:  
*                  
*  Parameters:    
*
*  Activation:    Ph_HardReset
*                 Ph_SendTrmMsg
*                 Ph_TrmComplt
*
*  Remarks:       
*                 
*********************************************************************/
#ifdef   _FAULT_TOLERANCE
void  Ph_BusReset (cfp_struct *config_ptr)
{
}
#endif   /* _FAULT_TOLERANCE */

/*********************************************************************
*  Following functions related to synchronous/wide negotiation
*  may be reorgnized/rewritten in the future:
*     Ph_SendMsgo
*     Ph_Negotiate
*     Ph_SyncSet
*     Ph_SyncNego 
*     Ph_ExtMsgi
*     Ph_ExtMsgo
*     Ph_HandleMsgi
*********************************************************************/

/*********************************************************************
*
*  Ph_SendMsgo routine -
*
*  send message out
*
*  Return Value:  None
* 
*  Parameters:    scb_ptr
*                 base address of AIC-7870
*
*  Activation:    PH_IntHandler
*                 Ph_Negotiate                  
*
*  Remarks:                
*                  
*********************************************************************/
void Ph_SendMsgo (sp_struct *scb_ptr,
                  register AIC_7870 *base)
{
   register UBYTE j;
   register UBYTE scsi_rate;
   cfp_struct *config_ptr=scb_ptr->SP_ConfigPtr;

   j = (UBYTE)(((UBYTE)scb_ptr->SP_Tarlun) & TARGET_ID) >> 4;
   OUTBYTE(AIC7870[SCSISIG], MOPHASE);
   if (INBYTE(AIC7870[SCSISIG]) & ATNI)
   {
      if (scb_ptr->SP_NegoInProg)
      {
         if (scb_ptr->Sp_ExtMsg[0] == 0xff)
         {
            scb_ptr->Sp_ExtMsg[0] = 0x01;
            if (Ph_ExtMsgo(scb_ptr, base) != MIPHASE)
            {
               return;
            }
            if (INBYTE(AIC7870[SCSIBUSL]) != MSG07)
            {
               return;
            }
            if (scb_ptr->Sp_ExtMsg[2] == MSGSYNC)
            {
               scsi_rate = INBYTE(AIC7870[XFER_OPTION + j]) & WIDEXFER;
            }
            else
            {
               scsi_rate = 0;
            }
            OUTBYTE(AIC7870[XFER_OPTION + j], scsi_rate);
            OUTBYTE(AIC7870[SCSIRATE], scsi_rate);
            INBYTE(AIC7870[SCSIDATL]);
            return;
         }
         scb_ptr->SP_NegoInProg = 0;
         if (scb_ptr->Sp_ExtMsg[0] == 0x01)
         {
            Ph_SyncNego(scb_ptr, base);
         }
         else if (INBYTE(AIC7870[XFER_OPTION + j]) == NEEDNEGO)
         {
            Ph_Negotiate(scb_ptr, base);
         }
         return;
      }
      j = INBYTE(AIC7870[SXFRCTL1]);            /* Turn off parity checking to*/
      OUTBYTE(AIC7870[SXFRCTL1], j & ~ENSPCHK); /* clear any residual error.  */
      OUTBYTE(AIC7870[SXFRCTL1], j | ENSPCHK);  /* Turn it back on explicitly */
                                                /* because it may have been   */
                                                /* cleared in 'Ph_ParityError'. */
                                                /* (It had to been previously */
                                                /* set or we wouldn't have    */
                                                /* gotten here.)              */
      OUTBYTE(AIC7870[CLRSINT1], CLRSCSIPERR | CLRATNO);
      OUTBYTE(AIC7870[CLRINT], CLRSCSINT);
      OUTBYTE(AIC7870[SCSIDATL], MSG05);
   }
   else
      OUTBYTE(AIC7870[SCSIDATL], MSG08);
   while (INBYTE(AIC7870[SCSISIG]) & ACKI);
   return;
}


/*********************************************************************
*
*  Ph_SetNeedNego routine -
*
*  Setup Sequencer XFER_OPTION with NEEDNEGO (Need to Negotiate)
*
*  Return Value:  None
*                  
*  Parameters: index into XFER_OPTION
*              base address of AIC-7870
*
*  Activation: Ph_ResetChannel
*              Ph_CheckCondition
*              Ph_ExtMsgi
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_SetNeedNego (UBYTE index,
                     register AIC_7870 *base, cfp_struct *config_ptr)
{

   OUTBYTE(AIC7870[XFER_OPTION + index], NEEDNEGO);

}      


/*********************************************************************
*
*  Ph_Negotiate routine -
*
*  Initiate synchronous and/or wide negotiation
*
*  Return Value:  None
*                  
*  Parameters: scb_ptr
*              base address of AIC-7870
*
*  Activation: PH_IntHandler
*              Ph_SendMsgo
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_Negotiate (sp_struct *scb_ptr,
                   register AIC_7870 *base)
{
   cfp_struct *config_ptr;
   UBYTE i;

   config_ptr = scb_ptr->SP_ConfigPtr;

   OUTBYTE(AIC7870[SEQADDR0], (UBYTE) SIOSTR3_ENTRY >> 2);
   OUTBYTE(AIC7870[SEQADDR1], (UBYTE) SIOSTR3_ENTRY >> 10);

   i = (UBYTE)(((UBYTE)scb_ptr->SP_Tarlun) & TARGET_ID) >> 4;

   if ((INBYTE(AIC7870[SCSISIG]) & BUSPHASE) != MOPHASE)
   {                                      /* default to async/narrow mode */
      OUTBYTE(AIC7870[XFER_OPTION + i], 00);
      OUTBYTE(AIC7870[SCSIRATE], 00);
      Ph_ClearFast20Reg(config_ptr, scb_ptr);
      return;
   }      

   if (INBYTE(AIC7870[XFER_OPTION + i]) == NEEDNEGO)
   {
      OUTBYTE(AIC7870[XFER_OPTION + i], 00);
      OUTBYTE(AIC7870[SCSIRATE], 00);
      Ph_ClearFast20Reg(config_ptr, scb_ptr);

      scb_ptr->Sp_ExtMsg[0] = MSG01;
      switch (config_ptr->Cf_ScsiOption[i] & (WIDE_MODE | SYNC_MODE))
      {
         case (WIDE_MODE | SYNC_MODE):
         case WIDE_MODE:
            scb_ptr->Sp_ExtMsg[1] = 2;
            scb_ptr->Sp_ExtMsg[2] = MSGWIDE;
            scb_ptr->Sp_ExtMsg[3] = WIDE_WIDTH;

            if (Ph_ExtMsgo(scb_ptr, base) != MIPHASE)    /* ship out WIDE message */
            {
               return;              
            }

            switch (INBYTE(AIC7870[SCSIBUSL]))  /* if target responds.. */
            {
               case MSG01:
                  scb_ptr->SP_NegoInProg = 1;   /* let ph_extmsgi() handle */
                  return;

               case MSG07:
                  if (config_ptr->Cf_ScsiOption[i] & SYNC_MODE)
                  {
                     OUTBYTE(AIC7870[SCSISIG], MIPHASE | ATNO);
                     INBYTE(AIC7870[SCSIDATL]);
                     if (Ph_Wt4Req(scb_ptr, base) == MOPHASE)
                     {
                        OUTBYTE(AIC7870[SCSISIG], MOPHASE | ATNO);
                        break;
                     }
                  }
                  else
                  {
                     OUTBYTE(AIC7870[SCSISIG], MIPHASE);
                     INBYTE(AIC7870[SCSIDATL]);
                  }

               default:
                  return;
            }

         case SYNC_MODE:
            scb_ptr->Sp_ExtMsg[1] = 3;
            scb_ptr->Sp_ExtMsg[2] = MSGSYNC;

            if(!(config_ptr->CFP_EnableFast20))
            {
               scb_ptr->Sp_ExtMsg[3] = (UBYTE)((config_ptr->Cf_ScsiOption[i] & SYNC_RATE) >> 4) * 6;
               scb_ptr->Sp_ExtMsg[3] += 25;

               if (config_ptr->Cf_ScsiOption[i] & SXFR2)
                  ++scb_ptr->Sp_ExtMsg[3];
            }
            else           /* if double speed mode is ON */
            {
               /* ScsiOption register bit 4-6:                          */
               /* 000 - 20MB/s      period:  50ns     12(12.5)          */
               /* 001 - 16MB/s               62.5ns   16(15.6)          */
               /* 010 - 13.4MB/s             75ns     19(18.75)         */
               /* 011 - (NOT SUPPORTED)                                 */
               /* 100 - 10MB/s               100ns    25(25)            */
               switch ((UBYTE)((config_ptr->Cf_ScsiOption[i] & SXFR) >> 4))
               {
                  case 0x00:       /* 20MB/s */
                     scb_ptr->Sp_ExtMsg[3] = 12;
                     break;

                  case 0x01:       /* 16MB/s */
                     scb_ptr->Sp_ExtMsg[3] = 16;
                     break;

                  case 0x02:       /* 13.3MB/s */
                     scb_ptr->Sp_ExtMsg[3] = 19;
                     break;

                  case 0x04:       /* 10MB/s */
                     scb_ptr->Sp_ExtMsg[3] = 25;
                     break;
               }
            }

            scb_ptr->Sp_ExtMsg[4] = NARROW_OFFSET;
            Ph_SyncNego(scb_ptr, base);
            return;
      }
   }
   Ph_SendMsgo(scb_ptr, base);
}

/*********************************************************************
*
*  Ph_SyncSet routine -
*
*  Set synchronous transfer rate based on negotiation
*
*  Return Value:  synchronous transfer rate (unsigned char)
*                  
*  Parameters: scb_ptr
*
*  Activation: Ph_ExtMsgi
*              Ph_ExtMsgo
*                  
*  Remarks:
*                  
*********************************************************************/
UBYTE Ph_SyncSet (sp_struct *scb_ptr)
{
   UBYTE sync_rate;

   if (scb_ptr->Sp_ExtMsg[3] == 12)          /* double speed checking */
      sync_rate = 0x00;
   else if (scb_ptr->Sp_ExtMsg[3] <= 16)
      sync_rate = 0x10;
   else if (scb_ptr->Sp_ExtMsg[3] <= 19)
      sync_rate = 0x20;
   else if (scb_ptr->Sp_ExtMsg[3] <= 25)     /* single speed checking */
      sync_rate = 0x00;
   else if (scb_ptr->Sp_ExtMsg[3] <= 31)
      sync_rate = 0x10;
   else if (scb_ptr->Sp_ExtMsg[3] <= 37)
      sync_rate = 0x20;
   else if (scb_ptr->Sp_ExtMsg[3] <= 43)
      sync_rate = 0x30;
   else if (scb_ptr->Sp_ExtMsg[3] <= 50)
      sync_rate = 0x40;
   else if (scb_ptr->Sp_ExtMsg[3] <= 56)
      sync_rate = 0x50;
   else if (scb_ptr->Sp_ExtMsg[3] <= 62)
      sync_rate = 0x60;
   else
      sync_rate = 0x70;
   return(sync_rate);
}

/*********************************************************************
*
*  Ph_SyncNego routine -
*
*  <brief description>
*
*  Return Value:  None
*                  
*  Parameters: stb_ptr
*              base address of AIC-7870
*
*  Activation: Ph_Negotiate
*              Ph_SendMsgo
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_SyncNego (sp_struct *scb_ptr,
                  register AIC_7870 *base)
{
   cfp_struct *config_ptr=scb_ptr->SP_ConfigPtr;

   if (Ph_ExtMsgo(scb_ptr, base) != MIPHASE)
   {
      return;
   }
   switch (INBYTE(AIC7870[SCSIBUSL]))
   {
   case MSG01:
      scb_ptr->SP_NegoInProg = 1;
      return;
   case MSG07:
      while (1)            /* Process any number of Message Rejects */
      {
         OUTBYTE(AIC7870[SCSISIG], MIPHASE);
         INBYTE(AIC7870[SCSIDATL]);
         if (Ph_Wt4Req(scb_ptr, base) != MIPHASE)
         {
            break;
         }
         if (INBYTE(AIC7870[SCSIBUSL]) != MSG07)
         {
            break;
         }
      }
      return;
   }
}

/*********************************************************************
*
*  Ph_ExtMsgi routine -
*
*  Receive and interpret extended message in
*
*  Return Value:  None
*                  
*  Parameters: scb_ptr
*              base address of AIC-7870
*
*  Activation: PH_IntHandler
*                  
*  Remarks:
*                  
*********************************************************************/
void Ph_ExtMsgi (sp_struct *scb_ptr,
                 register AIC_7870 *base)
{
   cfp_struct *config_ptr;
   UBYTE count, index, target_id, max_rate, phase, scsi_rate;
   UBYTE sxfrctl0, sxfrctl1;
   UBYTE nego_flag = NONEGO;
   UBYTE max_width = 0;
   UBYTE max_offset = NARROW_OFFSET;

   config_ptr = scb_ptr->SP_ConfigPtr;

   target_id = (UBYTE)(((UBYTE)scb_ptr->SP_Tarlun) & TARGET_ID) >> 4;

   count = INBYTE(AIC7870[PASS_TO_DRIVER]);

   scb_ptr->Sp_ExtMsg[0] = MSG01;
   scb_ptr->Sp_ExtMsg[1] = count--;
   scb_ptr->Sp_ExtMsg[2] = INBYTE(AIC7870[SCSIBUSL]);

   for (index = 3; count > 0; --count)
   {
      INBYTE(AIC7870[SCSIDATL]);

      if ((phase = Ph_Wt4Req(scb_ptr, base)) != MIPHASE)
      {
         if ((INBYTE(AIC7870[SCSISIG]) & (BUSPHASE | ATNI)) == (MOPHASE | ATNI))
         {
            if (scb_ptr->SP_NegoInProg)
            {
               OUTBYTE(AIC7870[SCSISIG], MOPHASE | ATNO);

               /*  Setup Sequencer XFER_OPTION with NEEDNEGO        */
               /*     (Need to Negotiate)                           */
               Ph_SetNeedNego(target_id, base, config_ptr);
            }
            else
            {
               OUTBYTE(AIC7870[CLRSINT1], CLRATNO);
               OUTBYTE(AIC7870[SCSISIG], MOPHASE);
            }

            sxfrctl0 = INBYTE(AIC7870[SXFRCTL0]);
            sxfrctl1 = INBYTE(AIC7870[SXFRCTL1]);
            OUTBYTE(AIC7870[SXFRCTL1], sxfrctl1 & ~ENSPCHK); /* Clear parity error   */
            OUTBYTE(AIC7870[SXFRCTL1], sxfrctl1 | ENSPCHK);
            OUTBYTE(AIC7870[CLRINT], CLRSCSINT);
            OUTBYTE(AIC7870[SXFRCTL0], sxfrctl0 & ~SPIOEN);  /* Place message parity */
            OUTBYTE(AIC7870[SCSIDATL], MSG09);               /* error on bus without */
            OUTBYTE(AIC7870[SXFRCTL0], sxfrctl0 | SPIOEN);   /* an ack.              */
         }
         else
         {
            if (phase == ADP_ERR)
               return;

            Ph_BadSeq(config_ptr, base);
         }
         return;
      }
      if (index < 5)       /* pull in the rest of the extended message */
      {
         scb_ptr->Sp_ExtMsg[index++] = INBYTE(AIC7870[SCSIBUSL]);
      }
   }           /* end of    for (index = 3; count > 0; --count) */

   /* calculate the maximum transfer rate, either double speed, or non-double speed */
   if (config_ptr->CFP_EnableFast20)
   {
      /* ScsiOption register bit 4-6:                          */
      /* 000 - 20MB/s      period:  50ns     12(12.5)          */
      /* 001 - 16MB/s               62.5ns   16(15.6)          */
      /* 010 - 13.4MB/s             75ns     19(18.75)         */
      /* 011 - (NOT SUPPORTED)                                 */
      /* 100 - 10MB/s               100ns    25(25)            */
      switch ((UBYTE)((config_ptr->Cf_ScsiOption[target_id] & SXFR) >> 4))
      {
         case 0x00:       /* 20MB/s */
            max_rate = 12;
            break;

         case 0x01:       /* 16MB/s */
            max_rate = 16;
            break;

         case 0x02:       /* 13.3MB/s */
            max_rate = 19;
            break;

         case 0x04:       /* 10MB/s */
            max_rate = 25;
            break;
      }
   }
   else        /* for non-double speed */
   {
      max_rate = (UBYTE)(((config_ptr->Cf_ScsiOption[target_id] & SYNC_RATE) >> 4) * 6) + 25;
      if (config_ptr->Cf_ScsiOption[target_id] & SXFR2)
         ++max_rate;
   }

   /* Respond as a 16-bit device only if 2940W AND we are not  */
   /* suppressing negotiation.                                 */
   if (    (config_ptr->CFP_SuppressNego)
       && !(scb_ptr->SP_NegoInProg))
   {
      max_width = 0;                /* 8-bit device   */
      max_offset = 0;
      max_rate = 0;
   }
   else if (INBYTE(AIC7870[SBLKCTL]) & SELWIDE)
   {
      max_width = WIDE_WIDTH;
   }

   /* Respond to target's extended message */
   switch (scb_ptr->Sp_ExtMsg[2])
   {
      case MSGWIDE:        /* target sends us WIDE message */
         if (scb_ptr->Sp_ExtMsg[1] == 2)
         {
            OUTBYTE(AIC7870[XFER_OPTION + target_id], 00);
            OUTBYTE(AIC7870[SCSIRATE], 00);
            Ph_ClearFast20Reg(config_ptr, scb_ptr);

            if (scb_ptr->Sp_ExtMsg[3] > max_width)
            {
               scb_ptr->Sp_ExtMsg[3] = max_width;  /* take the less one */
               nego_flag = NEEDNEGO;
            }

            if (!scb_ptr->SP_NegoInProg)     /* if target initiated negotiation */
            {
               config_ptr->Cf_ScsiOption[target_id] |= WIDE_MODE;
               break;                        /* then stop here */
            }

            scb_ptr->SP_NegoInProg = 0;      /* tgt responds to HA initiated op */

            if (nego_flag == NONEGO)
            {
               if (scb_ptr->Sp_ExtMsg[3])    /* if target supports wide xfer */
               {                             /* log info into seq'cer */
                  OUTBYTE(AIC7870[XFER_OPTION + target_id], WIDEXFER);
                  OUTBYTE(AIC7870[SCSIRATE], WIDEXFER);
                  max_offset = WIDE_OFFSET;
               }

               /* start to process the synchronous data transfer part */
               if (config_ptr->Cf_ScsiOption[target_id] & SYNC_MODE)
               {
                  scb_ptr->Sp_ExtMsg[1] = 3;
                  scb_ptr->Sp_ExtMsg[2] = MSGSYNC;
                  scb_ptr->Sp_ExtMsg[3] = max_rate;
                  scb_ptr->Sp_ExtMsg[4] = max_offset;
                  OUTBYTE(AIC7870[SCSISIG], ATNO | MIPHASE);
                  scb_ptr->SP_NegoInProg = 1;   /* HA initiates sync nego */
               }
               return;
            }
         }
         scb_ptr->Sp_ExtMsg[1] = 2;

      case MSGSYNC:     /* target sends us SYNC DATA XFER message */
         if (scb_ptr->Sp_ExtMsg[1] == 3)
         {
            scsi_rate = INBYTE(AIC7870[XFER_OPTION + target_id]);
            if (scsi_rate == NEEDNEGO)       /* if no nego ever took place    */
            {
               scsi_rate = 0;
            }
            else                             /* otherwise, isolate WIDE bit   */
            {
               scsi_rate &= WIDEXFER;
            }

            OUTBYTE(AIC7870[XFER_OPTION + target_id], scsi_rate);
            OUTBYTE(AIC7870[SCSIRATE], scsi_rate);

            if (scb_ptr->Sp_ExtMsg[4])       /* if offset is non-zero, tgt */
            {                                /* will do synchronous xfer   */
               if (scsi_rate)                /* wide xfer was preset was done */
               {
                  max_offset = WIDE_OFFSET;
               }

               if (scb_ptr->Sp_ExtMsg[4] > max_offset)
               {
                  scb_ptr->Sp_ExtMsg[4] = max_offset;
                  nego_flag = NEEDNEGO;
               }

               if (scb_ptr->Sp_ExtMsg[3] < max_rate)  /* the smaller, the faster */
               {
                  scb_ptr->Sp_ExtMsg[3] = max_rate;   /* take the slower of the two */
                  nego_flag = NEEDNEGO;
               }
               else if (scb_ptr->Sp_ExtMsg[3] > 68)   /* slower than supported */
               {                                      /* synchronous xfer, set */
                  scb_ptr->Sp_ExtMsg[4] = 0;          /* to asynchronous xfer  */
                  nego_flag = NEEDNEGO;
                  OUTBYTE(AIC7870[SCSISIG],ATNO | MIPHASE);
                  scb_ptr->SP_NegoInProg = 1;
                  return;
               }
            }

            if (!scb_ptr->SP_NegoInProg)        /* tgt initiated nego */
               break;

            scb_ptr->SP_NegoInProg = 0;
            if (nego_flag == NONEGO)
            {
               scsi_rate += Ph_SyncSet(scb_ptr) + scb_ptr->Sp_ExtMsg[4];
               OUTBYTE(AIC7870[XFER_OPTION + target_id], scsi_rate);
               OUTBYTE(AIC7870[SCSIRATE], scsi_rate);
               Ph_LogFast20Map(config_ptr, scb_ptr);  /* record in Scratch RAM */

               return;
            }
         }

      default:         /* any other message rejected as unsupported messages  */
         OUTBYTE(AIC7870[SCSISIG], MIPHASE | ATNO);
         INBYTE(AIC7870[SCSIDATL]);
         if ((phase = Ph_Wt4Req(scb_ptr, base)) == MOPHASE)
         {
            OUTBYTE(AIC7870[SCSISIG], MOPHASE);
            OUTBYTE(AIC7870[CLRSINT1], CLRATNO);
            OUTBYTE(AIC7870[SXFRCTL0], INBYTE(AIC7870[SXFRCTL0]) & ~SPIOEN);
            OUTBYTE(AIC7870[SCSIDATL], MSG07);
            OUTBYTE(AIC7870[SXFRCTL0], INBYTE(AIC7870[SXFRCTL0]) | SPIOEN);
         }
         else
         {
            if (phase == ADP_ERR)
            {
               return;
            }
            Ph_BadSeq(config_ptr, base);
         }
         return;
   }        /* end of    switch (scb_ptr->Sp_ExtMsg[2])  */

   scb_ptr->Sp_ExtMsg[0] = 0xff;
   OUTBYTE(AIC7870[SCSISIG], ATNO | MIPHASE);
   scb_ptr->SP_NegoInProg = 1;
}

/*********************************************************************
*
*  Ph_ExtMsgo routine -
*
*  Send extended message out
*
*  Return Value: current scsi bus phase (unsigned char)
*             
*  Parameters: scb_ptr
*              base address of AIC-7870
*
*  Activation: Ph_Negotiate
*              Ph_SendMsgo
*              Ph_SyncNego
*             
*  Remarks:   
*             
*********************************************************************/
UBYTE Ph_ExtMsgo (sp_struct *scb_ptr,
                  register AIC_7870 *base)
{
   UBYTE c;
   UBYTE i = 0;
   UBYTE j;
   UBYTE scsi_rate = 0;
   UBYTE savcnt0,savcnt1,savcnt2;   /* save for stcnt just in case of pio */
   cfp_struct *config_ptr=scb_ptr->SP_ConfigPtr;

   savcnt0 = INBYTE(AIC7870[STCNT0]);        /* save STCNT here */
   savcnt1 = INBYTE(AIC7870[STCNT1]);        /* save STCNT here */
   savcnt2 = INBYTE(AIC7870[STCNT2]);        /* save STCNT here */

   /* Transfer all but the last byte of the extended message */
   for (c = scb_ptr->Sp_ExtMsg[1] + 1; c > 0; --c)
   {
      OUTBYTE(AIC7870[SCSIDATL], scb_ptr->Sp_ExtMsg[i++]);
      if (Ph_Wt4Req(scb_ptr, base) != MOPHASE)
      {
         OUTBYTE(AIC7870[CLRSINT1], CLRATNO);
         break;
      }
   }
   if (c == 0)             /* Removed semi-colon 1/7/94 */
   {
      if (scb_ptr->SP_NegoInProg)
      {
         j = (UBYTE)(((UBYTE)scb_ptr->SP_Tarlun) & TARGET_ID) >> 4;
         if (scb_ptr->Sp_ExtMsg[2] == MSGWIDE)
         {
            if (scb_ptr->Sp_ExtMsg[3])    /* tgt supports 16-bit xfer */
            {
               scsi_rate = WIDEXFER;
            }
         }
         else        /* the only possibility would be synchronous xfer */
         {
            scsi_rate = INBYTE(AIC7870[XFER_OPTION + j])
                      + Ph_SyncSet(scb_ptr)
                      + scb_ptr->Sp_ExtMsg[4];
            Ph_LogFast20Map(config_ptr, scb_ptr);  /* record in Scratch RAM */
         }

         OUTBYTE(AIC7870[XFER_OPTION + j], scsi_rate);
         OUTBYTE(AIC7870[SCSIRATE], scsi_rate);
      }
      scb_ptr->SP_NegoInProg ^= 1;
      OUTBYTE(AIC7870[CLRSINT1], CLRATNO);
      OUTBYTE(AIC7870[SCSIDATL], scb_ptr->Sp_ExtMsg[i]); /* xfer the last byte */
   }

   OUTBYTE(AIC7870[STCNT0],savcnt0);            /* restore STCNT */
   OUTBYTE(AIC7870[STCNT1],savcnt1);            /* restore STCNT */
   OUTBYTE(AIC7870[STCNT2],savcnt2);            /* restore STCNT */

   return(Ph_Wt4Req(scb_ptr, base));
}

/*********************************************************************
*
*  Ph_HandleMsgi routine -
*
*  Handle Message In
*
*  Return Value: none
*
*  Parameters: scb_ptr
*              base address of AIC-7870
*
*  Activation: PH_IntHandler
*
*  Remarks:
*
*********************************************************************/
void Ph_HandleMsgi (sp_struct *scb_ptr,
                    register AIC_7870 *base)
{
   cfp_struct *config_ptr = scb_ptr->SP_ConfigPtr;
   UBYTE rejected_msg;
   UBYTE phase, ignore_xfer;
   DWORD rescnt_temp;
   gen_union reg_value;

   if ((INBYTE(AIC7870[SCSISIG]) & ATNI) == 0)
   {
      switch (INBYTE(AIC7870[SCSIBUSL]))     /* reading without ACK */
      {
         case MSG07:
            rejected_msg = INBYTE(AIC7870[PASS_TO_DRIVER]); /* Get rejected msg    */
            if (rejected_msg & (MSGID | MSGTAG))           /* If msg Identify or  */ 
            {                                              /* tag type, abort     */
               OUTBYTE(AIC7870[SCSISIG], MIPHASE | ATNO);
               INBYTE(AIC7870[SCSIDATL]);
               PHM_TARGETABORT(config_ptr, scb_ptr, base);
               return;
            }
            break;

         case MSG23:                      /* ignore wide residue */
            /* To handle the IGNORE WIDE RESIDUE message:
               1. ACK the 0x23h message.
               2. read w/o ACK the 2nd message byte.
               3. Increment STCNT0-3 to back up due to the extra bad byte(s).
               4. Increment rescnt field of SCB by the reduced number.
               5. Read SHADDR0-3 registers, decrement by the reduced number,
                  and write to HADDR0-3 which will shine thru to SHADDR0-3.
               6. ACK the 2nd message byte.(Done outside of the switch).
               7. Unpause the sequencer.(Done by PH_IntHandler() when return).
            */
            INBYTE(AIC7870[SCSIDATL]);    /* ACK MSG23 message byte */
            phase = Ph_Wt4Req(scb_ptr, base);      /* Wait for target to assert REQ */
            if (ignore_xfer = INBYTE(AIC7870[SCSIBUSL]))   /* rd 2nd byte, no ACK */
            {                          /* do nothing if zero */
               reg_value.dword = 0;
               reg_value.ubyte[0] = INBYTE(AIC7870[STCNT0]);
               reg_value.ubyte[1] = INBYTE(AIC7870[STCNT1]);
               reg_value.ubyte[2] = INBYTE(AIC7870[STCNT2]);
               reg_value.dword += (DWORD)ignore_xfer;      /* restore the ACK */
               OUTBYTE(AIC7870[STCNT0], reg_value.ubyte[0]);
               OUTBYTE(AIC7870[STCNT1], reg_value.ubyte[1]);
               OUTBYTE(AIC7870[STCNT2], reg_value.ubyte[2]);

               rescnt_temp = scb_ptr->SP_ResCnt;           /* adj residual count */
               scb_ptr->SP_ResCnt += (DWORD)ignore_xfer;
               scb_ptr->SP_ResCnt =  (scb_ptr->SP_ResCnt & 0x0fff)
                                   | (rescnt_temp & 0xf000);

               reg_value.dword = 0;
               reg_value.ubyte[0] = INBYTE(AIC7870[SHADDR0]);
               reg_value.ubyte[1] = INBYTE(AIC7870[SHADDR1]);
               reg_value.ubyte[2] = INBYTE(AIC7870[SHADDR2]);
               reg_value.ubyte[3] = INBYTE(AIC7870[SHADDR3]);
               reg_value.dword -= (DWORD)ignore_xfer;
               OUTBYTE(AIC7870[HADDR0], reg_value.ubyte[0]);
               OUTBYTE(AIC7870[HADDR1], reg_value.ubyte[1]);
               OUTBYTE(AIC7870[HADDR2], reg_value.ubyte[2]);
               OUTBYTE(AIC7870[HADDR3], reg_value.ubyte[3]);
            }
            break;

         default:
            OUTBYTE(AIC7870[SCSISIG], MIPHASE | ATNO);
            do {
               INBYTE(AIC7870[SCSIDATL]);
               phase = Ph_Wt4Req(scb_ptr, base);
            } while (phase == MIPHASE);
            if (phase != MOPHASE)
            {
               Ph_BadSeq(config_ptr, base);
               return;
            }
            OUTBYTE(AIC7870[SCSISIG], MOPHASE);
            OUTBYTE(AIC7870[CLRSINT1], CLRATNO);
            OUTBYTE(AIC7870[SCSIDATL], MSG07);
            return;
      }                          /* end of switch statement */
   }
   INBYTE(AIC7870[SCSIDATL]);    /* Drive ACK active to release SCSI bus */
}

/*********************************************************************
*
*  Ph_IntSelto routine -
*
*  Handle SCSI selection timeout
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*              scb_ptr
*
*  Activation: PH_IntHandler
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_IntSelto (cfp_struct *config_ptr,
                  sp_struct *scb_ptr)
{
   register AIC_7870 *base;
   UBYTE scb;

   base = config_ptr->CFP_Base;
   scb = INBYTE(AIC7870[WAITING_SCB]);

   OUTBYTE(AIC7870[SCSISEQ], (INBYTE(AIC7870[SCSISEQ]) & 
      ~(ENSELO + ENAUTOATNO + ENAUTOATNI + TEMODEO)));
   PHM_CLEARTARGETBUSY(config_ptr, scb);
   OUTBYTE(AIC7870[CLRSINT1], CLRSELTIMO + CLRBUSFREE);

   if ((scb_ptr != NOT_DEFINED) && ! PHM_SCBPTRISBOOKMARK(config_ptr,scb_ptr))
   {
      PHM_ENABLENEXTSCBARRAY(config_ptr,scb_ptr);
      scb_ptr->SP_HaStat = HOST_SEL_TO;
      Ph_TerminateCommand(scb_ptr, scb);
   }
   return;
}

/*********************************************************************
*
*  Ph_IntFree routine -
*
*  Acknowledge and clear SCSI Bus Free interrupt
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*              scb_ptr
*
*  Activation: PH_IntHandler
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_IntFree (cfp_struct *config_ptr,
                 sp_struct *scb_ptr)
{
   register AIC_7870 *base;
   UBYTE scb;

   base = config_ptr->CFP_Base;
   scb = INBYTE(AIC7870[ACTIVE_SCB]);

   /* Reset DMA & SCSI transfer logic */
   OUTBYTE(AIC7870[DFCNTRL],FIFORESET);
   OUTBYTE(AIC7870[SXFRCTL0], INBYTE(AIC7870[SXFRCTL0]) | (CLRSTCNT | CLRCHN | SPIOEN));

   OUTBYTE(AIC7870[SIMODE1], INBYTE(AIC7870[SIMODE1]) & ~ENBUSFREE);
   OUTBYTE(AIC7870[SCSIRATE], 0x00);
   OUTBYTE(AIC7870[CLRSINT1], CLRBUSFREE);

   OUTBYTE(AIC7870[SEQADDR0], (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(AIC7870[SEQADDR1], (UBYTE) IDLE_LOOP_ENTRY >> 10);

   if (scb_ptr != NOT_DEFINED)
   {
      PHM_ENABLENEXTSCBARRAY(config_ptr,scb_ptr);
      PHM_CLEARTARGETBUSY(config_ptr, scb);
      scb_ptr->SP_HaStat = HOST_BUS_FREE;
      Ph_TerminateCommand(scb_ptr, scb);
   }
   return;
}
/*********************************************************************
*
*  Ph_ParityError routine -
*
*  handle SCSI parity errors
*
*  Return Value:  none
*                  
*  Parameters: scb_ptr
*              base address of AIC-7870
*
*  Activation: PH_IntHandler
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_ParityError (sp_struct *scb_ptr,
                     register AIC_7870 *base)
{
   UBYTE i;
   cfp_struct *config_ptr = scb_ptr->SP_ConfigPtr;

   Ph_Wt4Req(scb_ptr, base);

   i = INBYTE(AIC7870[SXFRCTL1]);            /* Turn parity checking off.  */
   OUTBYTE(AIC7870[SXFRCTL1], i & ~ENSPCHK); /* It will be turned back on  */
                                             /* in message out phase       */
   scb_ptr->SP_HaStat = HOST_DETECTED_ERR;
}

/*********************************************************************
*
*  Ph_Wt4Req routine -
*
*  wait for target to assert REQ.
*
*  Return Value:  current SCSI bus phase
*
*  Parameters     Base address of AIC-7870
*
*  Activation:    most other HIM routines
*
*  Remarks:       bypasses sequencer
*
*********************************************************************/
UBYTE Ph_Wt4Req (sp_struct *scb_ptr, register AIC_7870 *base)
{
   UBYTE stat;
   UBYTE phase;
   int orgint;
   DWORD count;
   cfp_struct *config_ptr = scb_ptr->SP_ConfigPtr;
   
   INTR_SAVE(orgint);

   count = 0x800000;          /* approx. 10 sec time out */

   for (;;)
   {
      while (INBYTE(AIC7870[SCSISIG]) & ACKI)
         ;
      while (((stat = INBYTE(AIC7870[SSTAT1])) & REQINIT) == 0)
      {
         --count;
         if (   (stat & (BUSFREE | SCSIRSTI))
             || (!count))           /* time out check */
         {
            return(ADP_ERR);
         }
         INTR_ON;                   /* Open window for servicing higher */
         INTR_RESTORE(orgint);      /* level interrupt, e.g. scsi reset */
      }
      OUTBYTE(AIC7870[CLRSINT1], CLRSCSIPERR);
      phase = INBYTE(AIC7870[SCSISIG]) & BUSPHASE;
      if ((phase & IOI) &&
          (phase != DIPHASE) &&
          (INBYTE(AIC7870[SSTAT1]) & SCSIPERR))
      {
         OUTBYTE(AIC7870[SCSISIG], phase);
         INBYTE(AIC7870[SCSIDATL]);
         continue;
      }
      return(phase);
   }
}

/*********************************************************************
*
*  Ph_MultOutByte routine -
*
*  Output same value start from the port and for the length specified
*
*  Return Value:  none phase
*
*  Parameters     base - start port address
*                 value - byte value sent to port
*                 length - number of bytes get sent
*
*  Activation:    
*
*  Remarks: This function should be used only when speed is not
*           critical
*
*********************************************************************/
#ifdef   MULT_OUT_BYTE
void Ph_MultOutByte (register AIC_7870 *base,UBYTE value,int length)
{
   int i;

   for (i =0; i<length; i++)
   {
      OUTBYTE(AIC7870[i],value);
   }
}
#endif   /* MULT_OUT_BYTE */

/*********************************************************************
*
*  Ph_MemorySet routine -
*
*  Set memory buffer with fixed value
*
*  Return Value:  none phase
*
*  Parameters     memptr - memory buffer pointer
*                 value - byte value set to buffer
*                 length - length of buffer to be set
*
*  Activation:    
*
*  Remarks: This function should be used only when speed is not
*           critical
*
*********************************************************************/

void Ph_MemorySet(UBYTE *memptr,UBYTE value,int length)
{
   WORD i;

   for (i=0; i<length; i++ )
      *memptr++ = value;
}

/*********************************************************************
*
*  Ph_Pause routine -
*
*  Pause AIC-7870 sequencer
*
*  Return Value:  none
*
*  Parameters     base
*
*  Activation:    
*
*  Remarks: This function should be used only when speed is not
*           critical
*
*********************************************************************/

void Ph_Pause (register AIC_7870 *base, cfp_struct *config_ptr)
{
   Ph_WriteHcntrl(base, (UBYTE) (INBYTE(AIC7870[HCNTRL]) | PAUSE), config_ptr);
   while (!(INBYTE(AIC7870[HCNTRL]) & PAUSEACK))
      ;
}

/*********************************************************************
*
*  Ph_UnPause routine -
*
*  UnPause AIC-7870 sequencer
*
*  Return Value:  none
*
*  Parameters     base
*
*  Activation:    
*
*  Remarks: This function should be used only when speed is not
*           critical
*
*********************************************************************/

void Ph_UnPause (register AIC_7870 *base, cfp_struct *config_ptr)
{
   Ph_WriteHcntrl(base, (UBYTE) (INBYTE(AIC7870[HCNTRL]) & ~PAUSE), config_ptr);
}

/*********************************************************************
*
*  Ph_WriteHcntrl routine -
*
*  Write to HCNTRL
*
*  Return Value:  none
*
*  Parameters     base
*                 output value
*
*  Activation:    
*
*  Remarks: This function is designed to test a work-around for the
*           asynchronous pause problem in Lance
*
*********************************************************************/

void Ph_WriteHcntrl (register AIC_7870 *base, UBYTE value,
    cfp_struct *config_ptr)
{
   UBYTE hcntrl_data;
                                                        /* If output will  */
   if (!(value & PAUSE))                                /* pause chip, just*/
   {                                                    /* do the output.  */
      hcntrl_data = INBYTE(AIC7870[HCNTRL]);            
      if (!(hcntrl_data & PAUSEACK))                    /* If chip is not  */
      {                                                 /* paused, pause   */
         OUTBYTE(AIC7870[HCNTRL], hcntrl_data | PAUSE); /* the chip first. */
         while (!(INBYTE(AIC7870[HCNTRL]) & PAUSEACK));
      }                                                 /* If the chip is  */
      if (INBYTE(AIC7870[INTSTAT]) & ANYPAUSE)          /* paused due to an*/
      {                                                 /* interrupt, make */
         value |= PAUSE;                                /* sure we turn the*/
      }                                                  /* pause bit on.   */
   }
   OUTBYTE(AIC7870[HCNTRL], value);
}

/*********************************************************************
*
*  Ph_ReadIntstat routine -
*
*  Read from INTSTAT
*
*  Return Value:  INTSTAT value
*
*  Parameters     base
*
*  Activation:    
*
*  Remarks: This function is designed to test a work-around for the
*           asynchronous pause problem in Lance
*
*********************************************************************/

UBYTE Ph_ReadIntstat (register AIC_7870 *base, cfp_struct *config_ptr)
{
   UBYTE hcntrl_data;
   UBYTE value;

               
   hcntrl_data = INBYTE(AIC7870[HCNTRL]);               /* If output will  */
   if (!(hcntrl_data & PAUSE))                          /* pause chip, just*/
   {                                                    /* do the output.  */
      OUTBYTE(AIC7870[HCNTRL], hcntrl_data | PAUSE);    /* pause the chip  */
      while (!(INBYTE(AIC7870[HCNTRL]) & PAUSEACK));    /* first.          */
      if ((value = INBYTE(AIC7870[INTSTAT])) & ANYPAUSE)/* paused due to an*/
      {                                                 /* interrupt, make */
         hcntrl_data |= PAUSE;                          /* sure we turn the*/
      }                                                 /* pause bit on.   */
      OUTBYTE(AIC7870[HCNTRL], hcntrl_data);            /* Restore HCNTRL  */
   }
   else
   {                                                    /* Already paused  */
      value = INBYTE(AIC7870[INTSTAT]);                 /* just read it    */
   }
   return(value);
}

/*********************************************************************
*
*  Ph_Delay routine -
*
*  Delay for no of 500us specified
*
*  Return Value:  None
*                  
*  Parameters:    base address of AIC-7870
*                 no of 500 us to delay
*
*  Activation:    Ph_ResetSCSI
*                  
*  Remarks:       This module should be only called when there is
*                 no SCSI or PCI activity at all
*                  
*********************************************************************/
void Ph_Delay (AIC_7870 *base, int count, cfp_struct *config_ptr)
{
   UBYTE scb16, scb17;
   gen_union msec;
   int i;

   if (!count)
      return;                              /* use zero count for deskew delay */

   msec.uword[0] = 2900;                   /* setup delay for 500 msec */

   scb16 = INBYTE(AIC7870[SCB16]);         /* save SCB16 and SCB17 */
   scb17 = INBYTE(AIC7870[SCB17]);
   for (i = 0; i != count ; i++)
   {
      OUTBYTE(AIC7870[SCB16], msec.ubyte[0]);          /* set timer to 64 usec */
      OUTBYTE(AIC7870[SCB17], msec.ubyte[1]);
      OUTBYTE(AIC7870[SEQADDR0], (UBYTE) ATN_TMR_ENTRY >> 2);
      OUTBYTE(AIC7870[SEQADDR1], (UBYTE) ATN_TMR_ENTRY >> 10);
      Ph_UnPause(base, config_ptr);                          /* unpause sequencer */

      while (!(INBYTE(AIC7870[HCNTRL]) & PAUSEACK))
         ;

      OUTBYTE(AIC7870[CLRINT], CLRSEQINT);      /* clear seq'er int  */
   }
   OUTBYTE(AIC7870[SCB16], scb16);              /* restore SCB16 & SCB17 */
   OUTBYTE(AIC7870[SCB17], scb17);
}

/*********************************************************************
*  
*  Ph_ScbRenego -
*  
*  Reset scratch RAM to initiate or suppress sync/wide negotiation.
*
*  Return Value:  void
*             
*  Parameters: config_ptr
*              tarlun      - Target SCSI ID / Channel / LUN,
*                            same format as in SCB.
*
*  Activation: scb_special
*
*  Remarks:    
*                 
*********************************************************************/
void Ph_ScbRenego (cfp_struct *config_ptr, UBYTE tarlun)
{
   AIC_7870 *base = config_ptr->CFP_Base;
   /* hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr; */
   UBYTE option_index, scratch_index, scratch_value;

   /* Extract SCSI ID */
   option_index = scratch_index = (UBYTE)(tarlun & TARGET_ID) >> 4;

   /* Write scratch RAM and possibly renegotiate based upon the following  */
   /* criteria:                                                            */
   /*    1. If the config_ptr indicates that the device does either SYNC   */
   /*       or WIDE, set the scratch RAM to 0x8f to start a new round of   */
   /*       negotiation.                                                   */
   /*    2. Otherwise, if neither SYNC nor WIDE is set, check to see if    */
   /*       the WIDE and SYNC bit is set in scratch RAM.  If either is set */
   /*       in scratch RAM, set the same bit in config_ptr before setting  */
   /*       the scratch to 0x8f.  If both cleared in scratch, set scratch  */
   /*       to 0x00.                                                       */
   if (config_ptr->Cf_ScsiOption[option_index] & (WIDE_MODE | SYNC_MODE))
   {
      /*  Setup Sequencer XFER_OPTION with NEEDNEGO (Need to Negotiate) */
      Ph_SetNeedNego(scratch_index, base, config_ptr);
   }
   else        /* if config_ptr show no SYNC and no WIDE */
   {
      scratch_value = INBYTE(AIC7870[XFER_OPTION + scratch_index]);
      if (   !(scratch_value & SOFS)      /* currently xfring at ASYNC */
          && !(scratch_value & WIDEXFER)) /* and narrow mode */
         OUTBYTE(AIC7870[XFER_OPTION + scratch_index], 0x00);
      else     /* currently at either SYNC or WIDE, or both */
      {
         if (scratch_value & SOFS)        /* currently xfring at SYNC */
            config_ptr->Cf_ScsiOption[option_index] |= SYNC_MODE;

         if (scratch_value & WIDEXFER)    /* currently xfring at WIDE */
            config_ptr->Cf_ScsiOption[option_index] |= WIDE_MODE;

         /*  Setup Sequencer XFER_OPTION with NEEDNEGO */
         Ph_SetNeedNego(scratch_index, base, config_ptr);
      }
   }
   return;
}

/*********************************************************************
*  
*  Ph_ClearFast20Reg -
*  
*  Initializing control register that related to Fast20 to non-fast20
*  mode.
*
*  Return Value:  void
*             
*  Parameters: config_ptr
*              scb_ptr     - scb of the target ID will be extracted
*
*  Activation: Ph_Negotiate, Ph_ExtMsgi
*
*  Remarks:
*                 
*********************************************************************/
void Ph_ClearFast20Reg (cfp_struct *config_ptr, sp_struct *scb_ptr)
{
   register AIC_7870 *base;
   UBYTE target_id, id_mask, fast20map, reg_value;

   if (scb_ptr->Sp_ExtMsg[2] != MSGSYNC)
      return;

   base = config_ptr->CFP_Base;
   target_id = (UBYTE)(((UBYTE)scb_ptr->SP_Tarlun) & TARGET_ID) >> 4;

   if (target_id <= 7)
   {
      id_mask = (0x01) << target_id;
      fast20map = INBYTE(AIC7870[FAST20_LOW]);
      fast20map &= ~id_mask;     /* clear the fast20 device map */
      OUTBYTE(AIC7870[FAST20_LOW], fast20map);
   }
   else
   {
      id_mask = (0x01) << (target_id-8);
      fast20map = INBYTE(AIC7870[FAST20_HIGH]);
      fast20map &= ~id_mask;     /* clear the fast20 device map */
      OUTBYTE(AIC7870[FAST20_HIGH], fast20map);
   }
   reg_value = INBYTE(AIC7870[SXFRCTL0]) & ~FAST20;
   OUTBYTE(AIC7870[SXFRCTL0], reg_value);
}

/*********************************************************************
*  
*  Ph_LogFast20Map -
*  
*  Log into scratch RAM locations the fast20 device map
*
*  Return Value:  void
*             
*  Parameters: config_ptr
*              scb_ptr     - scb of the target ID will be extracted
*
*  Activation: Ph_ExtMsgi
*
*  Remarks:    This routine assumes the two internal registers of the
*              sequencer were set to zero upon power on/reset.
*                 
*********************************************************************/
void Ph_LogFast20Map (cfp_struct *config_ptr, sp_struct *scb_ptr)
{
   register AIC_7870 *base;
   UBYTE fast20map, target_id, id_mask, reg_value;

   if (scb_ptr->Sp_ExtMsg[2] != MSGSYNC)
      return;

   base = config_ptr->CFP_Base;
   target_id = (UBYTE)(((UBYTE)scb_ptr->SP_Tarlun) & TARGET_ID) >> 4;

   if (target_id <= 7)
   {
      id_mask = (0x01) << target_id;
      fast20map = INBYTE(AIC7870[FAST20_LOW]);

      if (scb_ptr->Sp_ExtMsg[3] < FAST20_THRESHOLD)
      {
         fast20map |= id_mask;      /* set the fast20 device map */
         reg_value = INBYTE(AIC7870[SXFRCTL0]) | FAST20;
      }
      else
      {
         fast20map &= ~id_mask;     /* clear the fast20 device map */
         reg_value = INBYTE(AIC7870[SXFRCTL0]) & ~FAST20;
      }
      OUTBYTE(AIC7870[FAST20_LOW], fast20map);
   }
   else
   {
      id_mask = (0x01) << (target_id-8);
      fast20map = INBYTE(AIC7870[FAST20_HIGH]);

      if (scb_ptr->Sp_ExtMsg[3] < FAST20_THRESHOLD)
      {
         fast20map |= id_mask;      /* set the fast20 device map */
         reg_value = INBYTE(AIC7870[SXFRCTL0]) | FAST20;
      }
      else
      {
         fast20map &= ~id_mask;     /* clear the fast20 device map */
         reg_value = INBYTE(AIC7870[SXFRCTL0]) & ~FAST20;
      }
      OUTBYTE(AIC7870[FAST20_HIGH], fast20map);
   }

   OUTBYTE(AIC7870[SXFRCTL0], reg_value);

   return;
}

