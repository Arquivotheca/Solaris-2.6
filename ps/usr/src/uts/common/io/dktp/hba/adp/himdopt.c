/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma ident	"@(#)himdopt.c	1.10	96/03/13 SMI"
/* $Header:   Y:\source\aic-7870\him\himd\optima\himdopt.cv_   1.86.1.4   06 Oct 1995 14:06:30   KLI  $ */
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
*  Module Name:   HIMDOPT.C
*
*  Description:
*                 Codes common to HIM configured for driver with OPTIMA
*                 mode at run time are defined here. It should be included 
*                 only if HIM is configured for driver with OPTIMA mode. 
*                 These codes should always remain resident.
*                 
*                 
*
*  Programmers:   Paul von Stamwitz
*                 Chuck Fannin
*                 Jeff Mendiola
*                 Harry Yang
*    
*  Notes:         NONE
*
*  Entry Point(s):
*
*  Revisions -
*
***************************************************************************/

#include "him_scb.h"
#include "him_equ.h"
#include "seq_off.h"

/*********************************************************************
*
*  Ph_OptimaEnque routine -
*
*  Queue an OPTIMA SCB for the Lance sequencer
*
*  Return Value:  None, for now
*                  
*  Parameters:    scb - scb number
*                 scb_ptr -  
*                 base - register base address
*
*  Activation:    Ph_SendCommand
*                  
*  Remarks:       Before calling this routine sequencer must have
*                 been paused already
*                  
*********************************************************************/
void Ph_OptimaEnque (UBYTE scb,
                     sp_struct *scb_ptr,
                     register AIC_7870 *base)
{
   cfp_struct *config_ptr=scb_ptr->SP_ConfigPtr;
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   DWORD phys_scb_addr;

   /* Calculate address to Start of SCB for use by Sequencer   */
   phys_scb_addr = ( (DWORD) scb_ptr->SP_CDBPtr -
                     ( (DWORD) scb_ptr->Sp_CDB -
                       (DWORD) scb_ptr->Sp_seq_scb.seq_scb_array));

   /* Write to physical table */
   SCB_PTR_ARRAY[scb] = phys_scb_addr;

   /* Write to logical table */
   ACTPTR[scb] = scb_ptr;

   /* Add New SCB to the QIN Pointer Array */
   QIN_PTR_ARRAY[ha_ptr->qin_index++] = scb;
   OUTBYTE(AIC7870[SCRATCH_QIN_CNT], INBYTE(AIC7870[SCRATCH_QIN_CNT]) + 1);

   return;
}

/*********************************************************************
*
*  Ph_OptimaEnqueHead routine -
*
*  Queue an OPTIMA SCB for the Lance sequencer
*
*  Return Value:  None, for now
*                  
*  Parameters:    scb - scb number
*                 scb_ptr -  
*                 base - register base address
*
*  Activation:    Ph_SendCommand
*                  
*  Remarks:       Before calling this routine sequencer must have
*                 been paused already
*                  
*********************************************************************/
void Ph_OptimaEnqueHead (UBYTE scb,
                         sp_struct *scb_ptr,
                         register AIC_7870 *base)
{
   cfp_struct *config_ptr=scb_ptr->SP_ConfigPtr;
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   DWORD phys_scb_addr;

   /* Calculate address to Start of SCB for use by Sequencer   */
   phys_scb_addr = ( (DWORD) scb_ptr->SP_CDBPtr -
                             ( (DWORD) scb_ptr->Sp_CDB -
                               (DWORD) scb_ptr->Sp_seq_scb.seq_scb_array));

   /* Write to physical table */
   SCB_PTR_ARRAY[scb] = phys_scb_addr;

   /* SCB_PTR_ARRAY[scb] = phys_scb_addr -- was Already setup    */
   /*   by caller (i.e. Ph_BusDeviceReset)                       */

   /* Enqueue current scb as Next SCB for Sequencer to Execute   */
   Ph_OptimaQHead(scb, scb_ptr, base);

   return;
}

/*********************************************************************
*  Ph_OptimaQHead -
*
*  - Enqueue current Optima scb as Next SCB for Sequencer to Execute
*
*  Return Value:  
*                  
*  Parameters:    
*
*  Activation: Ph_OptimaEnqueHead
*              Ph_OptimaRequestSense
*
*  Remarks:       
*                 
*********************************************************************/
#ifdef   _FAULT_TOLERANCE
void Ph_OptimaQHead (UBYTE scb, sp_struct *scb_ptr, AIC_7870 *base)
{
   register cfp_struct *config_ptr = scb_ptr->SP_ConfigPtr;
   register hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   UBYTE i, j, qin_index;
   DWORD port_addr;

   /* Get Correct Target/Lun Address */
   i = (UBYTE) (scb_ptr->SP_Tarlun >> 4);

   /* Insert into Next_SCB_Array */
   port_addr = SCRATCH_NEXT_SCB_ARRAY + i;

   /* Get Current "Next_SCB_Array" Element, if any
      and move it back to QIN_PTR_ARRAY */

   if ((j = INBYTE(AIC7870[port_addr])) != 0x7F)
   {
      OUTBYTE(AIC7870[port_addr], 0x7F);     /* Nullify NEXT_SCB_ARRAY entry */

      port_addr = SCRATCH_QIN_PTR_ARRAY;     /* Modify sequencer ptr to QIN Array */
      i = INBYTE(AIC7870[port_addr]) - 1;
      OUTBYTE(AIC7870[port_addr], i);

      i = INBYTE(AIC7870[SCRATCH_QIN_CNT]);  /* Calculate insertion point */
      qin_index = ha_ptr->qin_index - i - 1; 
      QIN_PTR_ARRAY[qin_index] = (j & 0x7F); /* Insert in QIN array */
      OUTBYTE(AIC7870[SCRATCH_QIN_CNT], ++i); /* Increment count */
   }
   /* Now place current SCB at head of queue */

   port_addr = SCRATCH_QIN_PTR_ARRAY;     /* Decrement sequencer ptr to QIN array */
   i = INBYTE(AIC7870[port_addr]) - 1;
   OUTBYTE(AIC7870[port_addr], i);

   port_addr = SCRATCH_QIN_CNT;           /* Calculate insertion point */
   i = INBYTE(AIC7870[SCRATCH_QIN_CNT]);
   qin_index = ha_ptr->qin_index - i - 1;

   /* Insert scb index at head of queue */
   QIN_PTR_ARRAY[qin_index] = scb;

   OUTBYTE(AIC7870[SCRATCH_QIN_CNT], ++i);   /* Increment count */

}
#endif   /* _FAULT_TOLERANCE */
/*********************************************************************
*
*  Ph_OptimaCmdComplete routine -
*
*  Process command complete mode for OPTIMA mode
*  (Deque a completed external SCB )
*
*  Return Value:  work.byte[1-4] =
*
*                      [0] =  BIOS Flag
*                      [1] =  Determination whether Interrupt for BIOS or
*                             Driver
*                      [2] =  NOT USED
*
*                      [3] =  0 - No more command complete to be processed
*                             1 - More command complete to be processed
*                  
*  Parameters:    config_ptr - 
*                 hcntrl_data
*
*                 DWORD - work.byte[0-3]
*
*  Activation:    PH_IntHandler
*                  
*  Remarks:       work.byte[0] will get updated to indicate
*                          BIOS command complete interrupt            
*                  
*********************************************************************/
DWORD Ph_OptimaCmdComplete (cfp_struct *config_ptr,
                           UBYTE hcntrl_data,
                           DWORD wrk_dword)
{
   sp_struct *scb_ptr;
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   AIC_7870 *base = config_ptr->CFP_Base;
   gen_union work;
   UBYTE scb_index, id;

   work.dword = wrk_dword;

   scb_index = QOUT_PTR_ARRAY[ha_ptr->qout_index];

   /* See if ALL OPTIMA Driver Interrupts have been Handled? */
   if (scb_index != NULL_SCB )
   {
      scb_ptr = ACTPTR[scb_index];
      /*NEC QOUT_PTR_ARRAY[ha_ptr->qout_index] = 0xFF; */
      ha_ptr->qout_index++;

      if (scb_ptr->SP_Cmd == HARD_RST_DEV)
      {
         /* Since this is Completion for a "Hard Device Reset"*/
         /*  Command, we must Setup the Sequencer XFER_OPTION */
         /*  with NEEDNEGO Option (Need to Negotiate)         */
         id = (UBYTE)(((UBYTE)scb_ptr->SP_Tarlun) & TARGET_ID) >> 4;

         if (config_ptr->Cf_ScsiOption[id] & (WIDE_MODE | SYNC_MODE))
         {
            /*  Setup Sequencer XFER_OPTION with NEEDNEGO (Need to Negotiate) */
            Ph_SetNeedNego(id, base, config_ptr);
         }
      }

      if (scb_ptr->SP_Cmd == BOOKMARK)    /* Is this a "BOOKMARK" SCB? */
      {
         /* YES - This a "BOOKMARK" SCB, Return Immediately         */
         if (BUSY_PTR_ARRAY[scb_ptr->SP_Tarlun] == scb_index)
         {
            BUSY_PTR_ARRAY[scb_ptr->SP_Tarlun]= (UBYTE) NOT_DEFINED;
         }
         Ph_OptimaReturnFreeScb( config_ptr, scb_index);
         work.ubyte[3] = 1;
         return( work.dword);
      }
      else if ((scb_ptr->SP_MgrStat == SCB_ABORTINPROG) &&
               (scb_ptr->Sp_seq_scb.seq_scb_struct.Aborted == 1))
      {
         work.ubyte[3] = 1;
         return( work.dword);
      }
      work.ubyte[1] |= OUR_CC_INT;

      if (scb_ptr->SP_MgrStat == SCB_AUTOSENSE)
      {
         if (scb_ptr->SP_HaStat != HOST_DETECTED_ERR)
         {
            scb_ptr->SP_HaStat   = HOST_NO_STATUS;
         }
         scb_ptr->SP_MgrStat  = SCB_DONE_ERR;
         scb_ptr->SP_TargStat = UNIT_CHECK;
      }
      Ph_TerminateCommand(scb_ptr, scb_index);

      work.ubyte[3] = 1;
      return( work.dword);
   }

   Ph_Pause(base, config_ptr);

   /* Handle if this is a BIOS Interrupt      */
   if ((INBYTE(AIC7870[QOUTCNT]) & 0x01)-work.ubyte[0] != 0)
   {
      /* BIOS Interrupt -- Return Immediately                       */
      Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data), config_ptr);  
      work.ubyte[1] |= NOT_OUR_INT;  
      work.ubyte[0] = 1;

      /* Return status = MORE cmds to be processed status           */
      work.ubyte[3] = 1;
      return( work.dword);
   }
   else  
   {
      /* Double check during Pause to determine whether we really
       * can clear Interrupts or not.
       */
      if (QOUT_PTR_ARRAY[ha_ptr->qout_index] == 0xff)
      {
         if (!(INBYTE(AIC7870[QOUTCNT]) & 0x01))
            OUTBYTE(AIC7870[CLRINT], CLRCMDINT);

         if ((INBYTE(AIC7870[QOUTCNT]) & 0x01)-work.ubyte[0] != 0)
         {
            work.ubyte[3] = 1;
         }            
         else
         {
            work.ubyte[3] = 0;
         }
         Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data), config_ptr);
         return( work.dword);
      }
      else
      {
      Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data), config_ptr);  
      work.ubyte[3] = 1;
      return( work.dword);
      }
   }
}
/*********************************************************************
*
*  Ph_OptimaRequestSense routine -
*
*  Perform request sense for OPTIMA mode
*
*  Return Value:  None
*                  
*  Parameters: scb_ptr -
*              base -
*
*  Activation: Ph_CheckCondition
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_OptimaRequestSense (sp_struct *scb_ptr,
                           UBYTE scb)

{
   cfp_struct *config_ptr=scb_ptr->SP_ConfigPtr;
   /*   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;  */
   AIC_7870 *base = config_ptr->CFP_Base;
   union sense {
      UBYTE sns_array[4];
      DWORD sns_ptr;
   } sns_segptr;

   UBYTE i, lun;

   /* Move in request sense CDB address */
   sns_segptr.sns_ptr = scb_ptr->SP_CDBPtr - ((DWORD) scb_ptr->Sp_CDB -
                        (DWORD) &scb_ptr->Sp_SensePtr);

   /* Init all (6) Bytes of CDB after isolating LUN value  */
   lun = scb_ptr->Sp_CDB[1] & 0xe0;
   for (i = 0; i < 6; ++i)
     scb_ptr->Sp_CDB[i] = 0x00;

   /* Modify CDB Scsi Command = Request Sense     */
   scb_ptr->Sp_CDB[0] = 0x03;

   /* Restore the original LUN value              */
   scb_ptr->Sp_CDB[1] |= lun;

   /* Setup Request Sense Lengthwithin CDB        */
   scb_ptr->Sp_CDB[4] = (UBYTE) scb_ptr->Sp_SenseLen;

#ifdef _TARGET_MODE
   scb_ptr->SP_RejectMDP = 0;
#else
   /* Setup to NOT Accept Modified Data Pointer within SEQUENCER */
   scb_ptr->SP_RejectMDP = 1;
#endif   /* of _TARGET_MODE   */
   
   /* Must disable disconnect for request sense */
   scb_ptr->SP_DisEnable = 0;
   
   /* Must disable tagged queuing for request sense */
   scb_ptr->SP_TagEnable = 0;

   scb_ptr->SP_SegCnt   = 0x01;
   scb_ptr->SP_CDBLen   = 0x06;
   scb_ptr->SP_SegPtr   = sns_segptr.sns_ptr;
   scb_ptr->SP_TargStat = 0x00;
   scb_ptr->SP_ResCnt   = (DWORD) 0;

   /**************************************************************/
   /* Initialize Last 16 bytes of SCB for Use by SEQUENCER Only  */
   /*                                                            */
   /*  ARROW USED : UBYTE SCB_RsvdX[16];                         */
   /*                                                            */
   /*  for LANCE  : Logically set start of "SCB_RsvdX" to the    */
   /*               16th byte of the SCB                         */
   /*                                                            */
   /*               ==> (DWORD) scb_ptr->seq_scb_array[4]        */
   /*                                                            */
   /**************************************************************/
   /* EXTERNAL SCB SCHEME needs all 32 bytes                     */
   /* RsvdX[8] = chain control,                                  */
   /*                                                            */
   /*  bits 7-4: 2's complement of progress                      */
   /*            count                                           */
   /*  bit 3:    aborted flag                                    */
   /*  bit 2:    abort_SCB flag                                  */
   /*  bit 1:    concurrent flag                                 */
   /*  bit 0:    holdoff_SCB flag                                */
   /*                                                            */
   /* RsvdX[10] = aborted HaStat                                 */
   /*                                                            */
   /**************************************************************/

   for (i = 4; i < 8; i++) scb_ptr->Sp_seq_scb.seq_scb_array[i] = 0;

   /**************************************************************/

   /* Enqueue current scb as Next SCB for Sequencer to Execute      */
   Ph_OptimaQHead(scb, scb_ptr, base);

   Ph_OptimaIndexClearBusy(config_ptr,(UBYTE) scb_ptr->SP_Tarlun);
   scb_ptr->SP_MgrStat = SCB_AUTOSENSE;
}


/*********************************************************************
*
*  Ph_OptimaClearDevQue routine -
*
*  Clear active scb for the specified device in OPTIMA mode
*
*  Return Value:  None
*                  
*  Parameters: config_ptr -
*              scb_ptr -
*
*  Activation: Ph_HardReset
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_OptimaClearDevQue (sp_struct *scb_ptr,
                           AIC_7870 *base)
{
}


/*********************************************************************
*
*  Ph_OptimaIndexClearBusy routine -
*
*  Clear busy entry for the specified index
*
*  Return Value: None
*                  
*  Parameters: config_ptr
*              index = index to busy array
*
*  Activation: Ph_IntFree
*              Ph_IntSelTo
*              Ph_CheckCondition
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_OptimaIndexClearBusy (cfp_struct *config_ptr,
                              UBYTE index)
{
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   AIC_7870 *base = config_ptr->CFP_Base;
    
   /* Clear the "busy_ptr_array" element pointed to by the given
    *  index pointer.
    */
   BUSY_PTR_ARRAY[index]= (UBYTE) NOT_DEFINED;

}
/*********************************************************************
*
*  Ph_OptimaClearTargetBusy routine -
*
*  Clear target busy entry for the specified scb
*
*  Return Value: None
*                  
*  Parameters: config_ptr
*              scb = current scbptr
*
*  Activation: Ph_IntFree
*              Ph_IntSelTo
*              Ph_CheckCondition
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_OptimaClearTargetBusy (cfp_struct *config_ptr,
                         UBYTE scb)
{
   sp_struct *scb_ptr;
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;

   scb_ptr = ACTPTR[scb];
   Ph_OptimaIndexClearBusy(config_ptr, (UBYTE) scb_ptr->SP_Tarlun);
}
/*********************************************************************
*
*  Ph_OptimaClearChannelBusy -
*
*  Clear all target busy associated with OPTIMA channel
*
*  Return Value:  none
*                  
*  Parameters:    config_ptr -
*
*  Activation:    Ph_ResetChannel
*
*  Remarks:       
*                 
*********************************************************************/
void Ph_OptimaClearChannelBusy (cfp_struct *config_ptr)
{
}

/*********************************************************************
*
*   Ph_SetOptimaHaData routine -
*
*   This routine initializes HA structure parameters specific to
*   OPTIMA mode only.
*
*  Return Value:  none
*                  
*  Parameters:    config_ptr
*
*  Activation:    PH_InitDrvrHA
*                  
*  Remarks:                
*
*********************************************************************/

void Ph_SetOptimaHaData (cfp_struct *config_ptr)

{
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   UBYTE cnt = (UBYTE) config_ptr->Cf_NumberScbs;

   /* here we are going to allocate those memory dependent on  */
   /* number of scbs                                           */
   ACTPTR = (sp_struct **) ha_ptr->max_additional;
   FREE_STACK = (UBYTE *) (ACTPTR + config_ptr->Cf_NumberScbs+1);
   DONE_STACK = FREE_STACK + config_ptr->Cf_NumberScbs+1;

   /* Initialize Entire HA data structure, if clear */
   Ph_MemorySet((UBYTE *) ACTPTR, (UBYTE) NOT_DEFINED,
                          sizeof(DWORD) * config_ptr->Cf_NumberScbs+1);
   if (config_ptr->CFP_BiosActive)  /* Initialize ACTPTR[0], IF BIOS IS ACTIVE */
   {
      ACTPTR[0] = BIOS_SCB;
   }
   /* Initialize FREE_STACK */
   for (FREE_SCB = (UBYTE)config_ptr->Cf_NumberScbs; FREE_SCB; --cnt)
   {
      FREE_STACK[(FREE_SCB)--] = cnt;
   }

   /* initialize the scratch ram for Optima mode */
   Ph_SetOptimaScratch(config_ptr);
}

/*********************************************************************
*
*   Ph_SetOptimaScratch routine - (from: "int_scb_init_extscb")
* 
*   This routine initializes the scratch ram specific to OPTIMA mode
*   only.
*
*  Return Value:  none
*                  
*  Parameters:    config_ptr
*
*  Activation:    Ph_SetDrvrScratch
*                 Ph_SetOptimaHaData
*                  
*  Remarks:                
*
********************************************************************/ 

void Ph_SetOptimaScratch (cfp_struct *config_ptr)
{
   AIC_7870 *base = config_ptr->CFP_Base;
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   UWORD port_addr;
   DWORD ptr_buf;
   int i = 0;

   OUTBYTE(AIC7870[SCRATCH_QIN_CNT],0);

   ha_ptr->qin_index  = 0;
   ha_ptr->qout_index = 0;

   /* Replacement for act_chtar[] ==> acttarg[256] initialized in himdinit.c */

   /*  Adjust QIN_PTR_ARRAY to Start of Next 256 byte physical boundary */
   QIN_PTR_ARRAY = (UBYTE *) Ph_ScbPageJustifyQIN(config_ptr);

   QOUT_PTR_ARRAY= (UBYTE *) (QIN_PTR_ARRAY + 256);

   BUSY_PTR_ARRAY= (UBYTE *) (QOUT_PTR_ARRAY + 256);

   /*  Setup SCB_PTR_ARRAY Virtual Address*/
   SCB_PTR_ARRAY = (DWORD *) (BUSY_PTR_ARRAY + 256);

   /* Initialize External SCB ARRAY */
   Ph_MemorySet((UBYTE *) SCB_PTR_ARRAY,
      (UBYTE) NOT_DEFINED, sizeof(DWORD) * config_ptr->Cf_NumberScbs+1);

   /* Initialize: QIN, QOUT, BUSY */
 
   for (i = 0; i < 256 ; i++)
   {
      QIN_PTR_ARRAY[i] = (UBYTE) NOT_DEFINED;
      QOUT_PTR_ARRAY[i]= (UBYTE) NOT_DEFINED;
      BUSY_PTR_ARRAY[i]= (UBYTE) NOT_DEFINED;
   }

   FREE_LO_SCB = 1;
   FREE_HI_SCB = (UBYTE)config_ptr->Cf_NumberScbs;

   /* STANDARD MODE --just initialize scratch used in standard mode */
   OUTBYTE(AIC7870[SCRATCH_WAITING_SCB],0xFF);
   OUTBYTE(AIC7870[SCRATCH_ACTIVE_SCB],0Xff);

   /* Initialize Sequencer "scb_ptr_array" = External SCB address in
    *  Host Memory
    */
   port_addr = SCRATCH_SCB_PTR_ARRAY;
   ptr_buf = ((DWORD) config_ptr->Cf_HaDataPhy +
              ((DWORD) SCB_PTR_ARRAY - (DWORD) ha_ptr));
   Ph_MovPtrToScratch(port_addr, ptr_buf, config_ptr);

   /* Initialize Sequencer "qin_ptr_array" = QINFIFO address */
   port_addr = SCRATCH_QIN_PTR_ARRAY;
   ptr_buf = ((DWORD) config_ptr->Cf_HaDataPhy +
              ((DWORD) QIN_PTR_ARRAY - (DWORD) ha_ptr));
   Ph_MovPtrToScratch(port_addr, ptr_buf, config_ptr);

   /* Initialize Sequencer "qout_ptr_array" = QOUTFIFO address */
   port_addr = SCRATCH_QOUT_PTR_ARRAY;
   ptr_buf = ((DWORD) config_ptr->Cf_HaDataPhy +
              ((DWORD) QOUT_PTR_ARRAY - (DWORD) ha_ptr));
   Ph_MovPtrToScratch(port_addr, ptr_buf, config_ptr);

   /* Initialize Sequencer "busy_ptr_array" = address of External
    * "busy targets" table.
    */
   port_addr = SCRATCH_BUSY_PTR_ARRAY;
   ptr_buf = (DWORD) BUSY_PTR_ARRAY;
   ptr_buf = ((DWORD) config_ptr->Cf_HaDataPhy +
              ((DWORD) BUSY_PTR_ARRAY - (DWORD) ha_ptr));
   Ph_MovPtrToScratch(port_addr, ptr_buf, config_ptr);

   /* Initialize Sequencer "next_SCB_array" = 16 bytes of 7FH */
   port_addr = SCRATCH_NEXT_SCB_ARRAY;

   for (i = 0; i < 16 ; i++)
   {
      OUTBYTE(AIC7870[port_addr++], 0X7F);
   }
}

/*********************************************************************
*
*  Ph_ScbPageJustifyQIN -
*
*  adjust QIN_ptr_array pointers to 256 byte physical boundary
*
*  Return Value:  (UBYTE *) Logical pointer to start of QIN_PTR_ARRAY
*
*  Parameters:    config_ptr
*
*  Activation:    Ph_SetOptimaScratch
*                  
*  Remarks:       
*
*********************************************************************/
UBYTE *  Ph_ScbPageJustifyQIN (cfp_struct *config_ptr)

{
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   DWORD addr_buf, addr_off, residue;
   UBYTE *logical_addr;
   UBYTE cnt;

   /* Get logical offset to start of max_additional portion of hsp structure */
   addr_buf = ( (DWORD) ha_ptr->max_additional - (DWORD) ha_ptr);

   /* Compute Physical Address of max_additional portion of hsp structure */
   addr_buf = addr_buf + (DWORD)(config_ptr -> Cf_HaDataPhy);

   /* Compute Physical Address at End of "scb_ptr_array"
    *
    *  End of "scb_ptr_array"
    *               = Phy Address of max_additional portion of hsp structure
    *               + active_ptr_array
    *               + FREE_STACK
    *               + DONE_STACK
    *               + scb_ptr_array
    */
   addr_off = (  (DWORD)(4 * (config_ptr->Cf_NumberScbs+1))   /* active_ptr_array */
               + (DWORD)(2 * (config_ptr->Cf_NumberScbs+1))); /* FREE_STACK/DONE_STACK */

   addr_buf = addr_buf + addr_off;

   /* Check to see if Physical Address at end of "scb_ptr_array" falls on
    * a 256 byte boundary, or not
    */
   if (addr_buf  & 0x000000FF)
   {
      /* Get residue within current 256 byte segment */
      residue = (DWORD) (256 - (addr_buf & 0x000000FF));

      /* Start of QIN_PTR_ARRAY (at End of "active_ptr_array" ) doesn't
       * fall on a 256 byte boundary
       *  - We must recompute logical address using residue above inorder
       *    to put it on a 256 byte boundary.
       */
      logical_addr = (UBYTE *) ( (DWORD) (ha_ptr->max_additional)
                                +(DWORD) addr_off
                                +(DWORD) residue);
   }
   else
   {
      /* Start of QIN_PTR_ARRAY (at End of "active_ptr_array") falls on a
       * TRUE 256 byte boundary - setup return value
       */
      cnt = (UBYTE) config_ptr->Cf_NumberScbs;
      logical_addr = (UBYTE *) ( (DWORD) (ha_ptr->max_additional)
                                +(DWORD) addr_off);
   }
   return( (UBYTE *) logical_addr);
}


/*********************************************************************
*
*   Ph_OptimaMoreFreeScb routine -
*
*   This routine will tell if there is more free scb available in the free
*   pool
*
*  Return Value:  number of scbs available in the pool
*                  
*  Parameters:    ha_ptr - 
*
*  Activation:    Ph_SendCommand
* 
*  Remarks:                
*
*********************************************************************/

UBYTE Ph_OptimaMoreFreeScb (hsp_struct *ha_ptr,
                      sp_struct *scb_ptr)

{
   cfp_struct *config_ptr=scb_ptr->SP_ConfigPtr;
   UBYTE scb_cnt;

   if (scb_ptr->SP_TagEnable)
   {
      /* Tagged SCB's, get index from top */
      if (config_ptr->Cf_NumberScbs >=THRESHOLD)
         scb_cnt = ((FREE_HI_SCB - THRESHOLD) +
                    (THRESHOLD - FREE_LO_SCB + 1));
      else
         scb_cnt = (config_ptr->Cf_NumberScbs - FREE_LO_SCB) + 1;
   }
   else
   {
      /* NonTagged SCB are being Used -- So, get index from bottom */
      if (config_ptr->Cf_NumberScbs >= THRESHOLD)
         scb_cnt = THRESHOLD - FREE_LO_SCB + 1;
      else
         scb_cnt = (config_ptr->Cf_NumberScbs - FREE_LO_SCB) + 1;
   }
   return( scb_cnt);
}

/*********************************************************************
*
*  Ph_OptimaGetFreeScb routine -
*
*  Get free scb from free scb pool
*
*  Return Value:  scb number. If = 0xff, No scb is Available.
*                  
*  Parameters: ha_ptr
*
*  Activation: Ph_SendCommand
*                  
*  Remarks:                
*                  
*********************************************************************/
UBYTE Ph_OptimaGetFreeScb (hsp_struct *ha_ptr,
                           sp_struct *scb_ptr)
{
   cfp_struct *config_ptr=scb_ptr->SP_ConfigPtr;
   UBYTE scb_index;
   UBYTE threshold;

   if (config_ptr->Cf_NumberScbs >= THRESHOLD)
      threshold = THRESHOLD;
   else
      threshold = (UBYTE) config_ptr->Cf_NumberScbs;

   /* Make sure there is a "free scb index" that conforms to the following
    * conditions:
    *              (NOTE:  THRESHOLD = 30)
    *
    *    1. TAG    cmd & "hi" index of Free Stack  >30               -OR-
    *
    *    2. TAG    cmd & "hi" index of Free Stack <=30 & 
    *                    "hi" index >= "lo" index                    -OR-
    *
    *    3. NONTAG cmd & "lo" index of Free Stack <=30.
    *                    "hi" index >= "lo" index                    -OR-
    */
   if (((scb_ptr->SP_TagEnable) &&
        ((FREE_HI_SCB>THRESHOLD) ||
         ((FREE_HI_SCB<=threshold) && (FREE_HI_SCB>=FREE_LO_SCB))))

       || ((!scb_ptr->SP_TagEnable) && (FREE_LO_SCB<=threshold)
                                    && (FREE_HI_SCB>=FREE_LO_SCB    ))   )
   {
      /* Get Index & Add physical SCB address to scb_array */
      if ((scb_ptr->SP_TagEnable) && (FREE_HI_SCB>threshold))
      {
         /* Tagged SCB's, get index from top */
         scb_index = FREE_STACK[FREE_HI_SCB--];
      }
      else
      {
         /* NonTagged SCB -OR- Tagged SCB & All Scb's >29 are being Used
          * So, get index from bottom
          */
         scb_index = FREE_STACK[FREE_LO_SCB++];
      }
   }
   else
   {
      /* NO scb's are Available -- setup Error return value */
      scb_index = 0xff;
   }
   return( scb_index);
}

/*********************************************************************
*
*  Ph_OptimaReturnFreeScb routine -
*
*  Return free scb to free scb pool
*
*  Return Value:  none
*                  
*  Parameters: config_ptr
*              scb = Index of current scbptr address
*
*  Activation: Ph_PostCommand
*              Ph_StandardCmdComplete
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_OptimaReturnFreeScb (cfp_struct *config_ptr,
                             UBYTE scb)
{
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;

   if (scb <= THRESHOLD)
   {
      /* Return Scb_index to Free_Stack from Below */
      FREE_LO_SCB--;
      FREE_STACK[FREE_LO_SCB] = scb;
   }
   else
   {
      /* Return Scb_index to Free_Stack from Above */
      FREE_HI_SCB++;
      FREE_STACK[FREE_HI_SCB] = scb;
   }
   /* Clear scb_array entry    */
   SCB_PTR_ARRAY[scb]  = 0xFFFFFFFF;

   ACTPTR[scb] = (sp_struct *) -1;
   return;
}

/*********************************************************************
*
*  Ph_OptimaClearQinFifo routine -
*
*  Clear Entire Queue in fifo in optima mode
*
*  Return Value:  none
*                  
*  Parameters: config_ptr
*
*  Activation: Ph_AbortChannel
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_OptimaClearQinFifo(cfp_struct *config_ptr)
{
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   AIC_7870 *base = config_ptr->CFP_Base;
   int i;
    
   /* Clear the entire "qin_ptr_array"
    */
   for (i = 0; i < 256; i++)
   {
      QIN_PTR_ARRAY[i] = (UBYTE) NOT_DEFINED;
   }
}

/*********************************************************************
*
*  Ph_MovPtrToScratch
*
*  Move physical memory pointer to scratch RAM in Intel order ( 0, 1, 2, 3 )
*
*
*  Return Value:  VOID
*
*  Parameters:    port_addr - starting address in Lance scratch/scb area
*
*                 ptr_buf - Physical address to be moved
*
*  Activation:    Ph_SetOptimaScratch
*                  
*  Remarks:       
*
*********************************************************************/
void Ph_MovPtrToScratch (UWORD port_addr, DWORD ptr_buf,
                         cfp_struct *config_ptr)
{
   AIC_7870 *base = config_ptr->CFP_Base;
   union outbuf
   {
     UBYTE byte_buf[4];
     DWORD mem_ptr;
   } out_array;

   out_array.mem_ptr = ptr_buf;

   OUTBYTE(AIC7870[port_addr],    out_array.byte_buf[0]);
   OUTBYTE(AIC7870[port_addr+1],  out_array.byte_buf[1]);
   OUTBYTE(AIC7870[port_addr+2],  out_array.byte_buf[2]);
   OUTBYTE(AIC7870[port_addr+3],  out_array.byte_buf[3]);
}

/*********************************************************************
*
*   Ph_OptimadLoadFuncPtrs routine -
*
*   This routine will load OPTIMA mode function pointers
*
*  Return Value:  none
*
*  Parameters:    config_ptr
*
*  Activation:    Ph_GetDrvrConfig
*
*  Remarks:
*
*********************************************************************/

void Ph_OptimaLoadFuncPtrs (cfp_struct *config_ptr)
{
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   ha_ptr->morefreescb = Ph_OptimaMoreFreeScb;
   ha_ptr->enque = Ph_OptimaEnque;
   ha_ptr->enquehead = Ph_OptimaEnqueHead;
   ha_ptr->cmdcomplete = Ph_OptimaCmdComplete;
   ha_ptr->getfreescb = Ph_OptimaGetFreeScb;
   ha_ptr->returnfreescb = Ph_OptimaReturnFreeScb;
   ha_ptr->abortactive = Ph_OptimaAbortActive; 
   ha_ptr->clearqinfifo = Ph_OptimaClearQinFifo;
   ha_ptr->indexclearbusy = Ph_OptimaIndexClearBusy;
   ha_ptr->cleartargetbusy = Ph_OptimaClearTargetBusy;
   ha_ptr->requestsense = Ph_OptimaRequestSense;
   ha_ptr->enablenextscbarray = Ph_OptimaEnableNextScbArray;
}
/*********************************************************************
*  Ph_OptimaAbortActive -
*
*  Pause Arrow and initiate abort of active SCB. Remove from QIN or
*  QOUT fifos if needed.
*
*  Return Value:  0 -  SCB Abort completed
*                 1 -  SCB Abort in progress
*                 -1 - SCB not aborted
*                  
*  Parameters:    
*
*  Activation: Ph_ScbAbort()
*
*  Remarks: For active SCB in Optima mode
*                 
*********************************************************************/
#ifdef   _FAULT_TOLERANCE
int Ph_OptimaAbortActive ( sp_struct *scb_pointer)
{
   register cfp_struct *config_ptr = scb_pointer->SP_ConfigPtr;
   AIC_7870 *base = config_ptr->CFP_Base;
   hsp_struct *ha_ptr = config_ptr->CFP_HaDataPtr;
   int scb_index, i, tarlun,
       retval = ABTACT_NOT;
   UBYTE scbsave, bytebuf, j, k;


   Ph_WriteHcntrl(base, (UBYTE) (INBYTE(AIC7870[HCNTRL]) | PAUSE), config_ptr);
   while (~INBYTE(AIC7870[HCNTRL]) & PAUSEACK);

   for (i = 0; i < 1000; i++)
   {
      if (~INBYTE(AIC7870[DFSTATUS]) & MREQPEND)
         break;
   }
   for (i = 1; i < (int) config_ptr->Cf_NumberScbs + 1; i++)        
   {
      if (ACTPTR[i] == scb_pointer)
      {
         scb_index = i;             /* Found it! */
         break;
      }
   }

   /* Check various steps of SCB progression in sequencer. Handle abort
      based on step SCB is in when located, then exit. */

   for (;;)
   {
      if (i == (int) config_ptr->Cf_NumberScbs + 1)
      {
         retval = ABTACT_NOT;           /* If SCB ptr not found, return */
         break;
      }
      /* If SCB is in QIN, QOUT or NEXT_SCB arrays, substitute bookmark
         SCB INDEX with aborted bit set in array. Complete aborted SCB
         to host.  */
      /* (Bookmarks are used so as not to require "shuffling" of array) */

      for (i = ha_ptr->qout_index; QOUT_PTR_ARRAY[i] != 0xFF; i++)
      {                                      /* Check QOUT Array First */
         if (QOUT_PTR_ARRAY[i] == scb_index) /* In QOUT array, complete now */
         {
            if (i != ha_ptr->qout_index)     /* If at the head, just bump qout_index */
            {
               for ( j = (UBYTE)(i);
                     j != ha_ptr->qout_index;
                     j = (UBYTE) (j-1))       
               {                             /* Take it out of QOUT array */
                  QOUT_PTR_ARRAY[(UBYTE)(j)] = QOUT_PTR_ARRAY[(UBYTE)(j-1)];
               }
            }
            ha_ptr->qout_index++;            /* Increment QOUT index */
            retval = ABTACT_CMP;
            break;
         }
      }

      if (retval != ABTACT_NOT)              /* Exit if SCB aborted */
         break;

      /* Get LSB of Sequencer ptr to QIN Array */
      j = k = INBYTE(AIC7870[SCRATCH_QIN_PTR_ARRAY]);

      while ( j != ha_ptr->qin_index)        /* Check QIN Array */
      {
         if (QIN_PTR_ARRAY[j] == scb_index)  /* In QIN array, complete now */
         {
            if ( j == k )     /* If at top of queue, just set abort bit */
            {
               scb_pointer->Sp_seq_scb.seq_scb_struct.Aborted = 1;
               retval = ABTACT_PRG;
            }
            else              /* Not top of queue, abort now */
            {
               for (; j != ha_ptr->qin_index; j = (UBYTE)(j+1))
               {              /* Take it out of QINFIFO */
                  QIN_PTR_ARRAY[j] = QIN_PTR_ARRAY[(UBYTE)(j+1)];
               }
               --(ha_ptr->qin_index);           /* Decrement QIN index */
               OUTBYTE(AIC7870[SCRATCH_QIN_CNT],   /* Decrement QINCNT */
                                    INBYTE(AIC7870[SCRATCH_QIN_CNT]) - 1);
               retval = ABTACT_CMP;
            }
            break;
         }
         j++;
      }
      if (retval != ABTACT_NOT)              /* Exit if SCB aborted */
         break;
   
      /* Check NEXT_SCB_ARRAY */

      tarlun = scb_pointer->SP_Tarlun>>4;    /* Index for NEXT_SCB_ARRAY */

      if (scb_index == INBYTE(AIC7870[SCRATCH_NEXT_SCB_ARRAY + tarlun]))
      {
         scb_pointer->Sp_seq_scb.seq_scb_struct.Aborted = 1;
         retval = ABTACT_PRG;
         break;
      }
      /* Check active SCB */

      if (scb_index == INBYTE(AIC7870[SCRATCH_ACTIVE_SCB]))
      {
         /* Abort active SCB in chip, also   */

         scbsave = INBYTE(AIC7870[SCBPTR]);        /* Save current SCB ptr */
         OUTBYTE(AIC7870[SCBPTR], HWSCB_DRIVER);   /* Select Driver SCB */

         /* Or aborted bit into current chain control byte */
         bytebuf = INBYTE(AIC7870[SCB13]) | SCB_ABORTED; 
         OUTBYTE(AIC7870[SCB13], bytebuf);         /* Set in HW SCB */
         OUTBYTE(AIC7870[SCBCNT], 00);             /* Reset SCB address count */
         OUTBYTE(AIC7870[SCBPTR], scbsave);        /* Restore SCB ptr */
      }
      scb_pointer->SP_MgrStat = SCB_ABORTINPROG;
      scb_pointer->Sp_seq_scb.seq_scb_struct.Aborted = 1;

                                          /* Set SpecFunc bit in SCB Control Byte */
      scb_pointer->Sp_seq_scb.seq_scb_struct.SpecFunc = SPEC_BIT3SET;

      scb_pointer->SP_SegCnt = SPEC_OPCODE_00;  /* Setup "SpecFunc" opcode */

                                          /* Setup Message Value     */
      if (scb_pointer->Sp_seq_scb.seq_scb_struct.TagEnable)
      {
         scb_pointer->SP_SegPtr = MSG0D;  /* "Abort Tag" for tagged  */
      }
      else
      {
         scb_pointer->SP_SegPtr = MSG06;  /* "Abort" for non-tagged  */
      }
      /* Insert at Head of Queue only if we are not in the process   */      
      /* of selecting the target. If the selection is in progress    */
      /* a interrupt to the HIM is guaranteed. Either the result of  */
      /* command execution or selection time out will hapen          */ 
      if (!((INBYTE(AIC7870[SCSISEQ]) & ENSELO) &&
         (INBYTE(AIC7870[SCRATCH_WAITING_SCB]) == scb_index)))
         ha_ptr->enquehead(scb_index, scb_pointer, base);

      retval = ABTACT_PRG;                /* Return Abort in Progress */
      break;
   }

   Ph_WriteHcntrl(base, (UBYTE) ((INBYTE(AIC7870[HCNTRL]) & ~PAUSE)), config_ptr);
   return( retval);
}
#endif   /* _FAULT_TOLERANCE */

/*********************************************************************
*  Ph_OptimaEnableNextScbArray -
*
*  Enable asssociated valid entry in next scb array
*
*  Return Value:  
*                  
*  Parameters:    
*
*  Activation: Ph_IntSelto
*              Ph_IntFree
*
*  Remarks:       
*                 
*********************************************************************/
void Ph_OptimaEnableNextScbArray (sp_struct *scb_ptr)
{
   register AIC_7870 *base;
   UBYTE target;
   UBYTE value;
   cfp_struct *config_ptr=scb_ptr->SP_ConfigPtr;

   base = config_ptr->CFP_Base;
   target = (UBYTE) scb_ptr->SP_Tarlun >> 4;
   if (!((value = INBYTE(AIC7870[SCRATCH_NEXT_SCB_ARRAY+target])) & 0x80))
      if (value != 0x7f)
         OUTBYTE(AIC7870[SCRATCH_NEXT_SCB_ARRAY+target], value | 0x80);
}

