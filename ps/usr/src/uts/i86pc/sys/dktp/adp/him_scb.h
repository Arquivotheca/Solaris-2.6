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

#ifndef _SYS_DKTP_ADP_HIM_SCB_H
#define _SYS_DKTP_ADP_HIM_SCB_H


#pragma ident	"@(#)him_scb.h	1.2	94/11/03 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/* $Header:   V:\source\code\aic-7870\him\common\him_scb.hv_   1.54   05 Oct 1994 16:03:20   KLI  $ */

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
*  Module Name:   HIM_SCB.H
*
*  Description:
*					This file is comprised of three sections:
*
*             	1. O/S specific definitions that can be customized to allow
*                 for proper compiling and linking of the Hardware Interface
*                 Module.
*
*              2. Structure definitions:
*                   SP_STRUCT  - Sequencer Control Block Structure
*                   HIM_CONFIG - Host Adapter Configuration Structure
*                                (may be customized)
*                   HIM_STRUCT - Hardware Interface Module Data Structure
*                                (cannot be customized)
*
*              3. Function prototypes for the Hardware Interface Module.
*
*              Refer to the Hardware Interface Module specification for
*              more information.
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

/*                CUSTOMIZED OPERATING SPECIFIC DEFINITIONS                */

#if !defined( _EX_SEQ00_ )
#define  _STANDARD
#endif   /* !defined( _EX_SEQ00_ ) */

#if !defined( _EX_SEQ01_ )
#define  _OPTIMA
#endif   /* !defined( _EX_SEQ01_ ) */

#include "custom.h"

#ifdef   _BIOS
#include "bios_mac.h"
#endif   /* _BIOS */

#ifdef   _DRIVER
#include "drvr_mac.h"
#define  _FAULT_TOLERANCE
#endif   /* _DRIVER */


/*        STRUCTURE DEFINITIONS: SP_STRUCT, HIM_CONFIG, HIM_STRUCT         */

/* Solaris additions							*/
#define ADP_MAX_DMA_SEGS	32     	/* Max used Scatter/Gather seg	*/

/* Scatter Gather, data length and data pointer structure. */
struct adp_sg {
	unsigned long   data_addr; 
	unsigned long	data_len;
};

/****************************************************************************
                    %%% Sequencer Control Block (SCB) %%%
****************************************************************************/
#define  MAX_CDB_LEN    12          /* Maximum length of SCSI CDB          */
#define  MAX_SCBS       255         /* Maximum number of possible SCBs     */
#define  INVALID_SCBPTR 255         /*                                     */
#define  MIN_SCB_LENGTH 20          /* Min. # of SCB bytes required by HW  */
#define  LOW_SCB_INDEX  1           /* Lowest legal SCB index              */
#define  HIGH_SCB_INDEX 254         /* Highest legal SCB index             */
                                    /*                                     */
#define  THRESHOLD      30          /* SCB(1-30) = 30 Non-Tag SCB's/Lance  */
                                    /*             MAXIMUM                 */
typedef struct sp                   /*                                     */
{                                   /*                                     */

/************************************/
/*    Hardware Interface Module     */
/*        Specific members          */
/************************************/
#ifdef _DRIVER
   union                            /*                                     */
   {                                /*                                     */
      struct sp *Next;              /* next SCB pointer on queue           */
      DWORD _void;                  /* Dword entry to align rest of struct */
   } Sp_queue;                     /*                                     */
#endif   /* _DRIVER */
   union                            /*                                     */
   {                                /*                                     */
      struct cfp *ConfigPtr;        /* Pointer to config structure         */
      DWORD _void;                  /* Dword entry to align rest of struct */
   } Sp_config;                    /*                                     */
   struct                           /*                                     */
   {                                /*                                     */
      DWORD Cmd:8;                  /* SCB command type                    */
      DWORD Stat:8;                 /* SCB command status                  */
      DWORD Rsvd1:1;                /* Reserved = 0                        */
      DWORD NegoInProg:1;           /* Negotiation in progress             */
      DWORD Rsvd2:2;                /* Reserved = 0                        */
      DWORD OverlaySns:1;           /* 1 = Overlay SCB with Sense Data     */
      DWORD NoNegotiation:1;        /* No negotiation for the scb          */
      DWORD NoUnderrun:1;           /* 1 = Suppress underrun errors        */
      DWORD AutoSense:1;            /* 1 = Automatic Request Sense enabled */
      DWORD MgrStat:8;              /* Intermediate status of SCB          */
   } Sp_control;                   /*                                     */
   union                            /* Sequencer SCSI Cntl Block (32 bytes)*/
   {                                /*                                     */
      DWORD seq_scb_array[8];       /* Double-word array for SCB download  */
      struct                        /* Bit-wise definitions for HIM use    */
      {                             /*                                     */
         DWORD Tarlun:8;            /* Target/Channel/Lun                  */
         DWORD TagType:2;           /* Tagged Queuing type                 */
         DWORD Discon:1;            /* Target currently disconnected       */
         DWORD SpecFunc:1;          /* Defined for "special" opcode        */
         DWORD Rsvd3:1;             /* Reserved = 0                        */
         DWORD TagEnable:1;         /* Tagged Queuing enabled              */
         DWORD DisEnable:1;         /* Disconnection enabled               */
         DWORD RejectMDP:1;         /* Non-contiguous data                 */
         DWORD CDBLen:8;            /* Length of Command Descriptor Block  */
         DWORD SegCnt:8;            /* Number of Scatter/Gather segments   */
         DWORD SegPtr;              /* Pointer to Scatter/Gather list      */
         DWORD CDBPtr;              /* Pointer to Command Descriptor Block */
         DWORD TargStat:8;          /* Target status (overlays 'sreentry') */
         DWORD Holdoff:1;           /* Holdoff SCB flag                    */
         DWORD Concurrent:1;        /* Cuncurrent SCB flag                 */
         DWORD Abort:1;             /* Abort SCB flag                      */
         DWORD Aborted:1;           /* Aborted SCB flag                    */
         DWORD Progress:4;          /* Progress count (2's complement)     */
         DWORD NextPtr:8;           /* Pointer to next SCB to be executed  */
         DWORD Offshoot:8;          /* Pointer to offshoot SCB from chain  */
         DWORD ResCnt;              /* Residual byte count                 */
         DWORD Rsvd4;               /* Reserved = 0                        */
         DWORD Address;             /* Current host buffer address         */
         DWORD Length:24;           /* Current transfer length             */
         DWORD HaStat:8;            /* Host Adapter status                 */
      } seq_scb_struct;             /*                                     */
   } Sp_seq_scb;                       /*                                     */
   DWORD Sp_SensePtr;               /* Pointer to Sense Area               */
   DWORD Sp_SenseLen;               /* Sense Length                        */
   UBYTE Sp_CDB[MAX_CDB_LEN];       /* SCSI Command Descriptor Block       */
   UBYTE Sp_ExtMsg[8];              /* Work area for extended messages     */
#ifdef _DRIVER
   UBYTE Sp_Reserved[4];            /* Reserved for future expansion       */
   OS_TYPE Sp_OSspecific;           /* Pointer to custom data structures   */
                                    /* required by O/S specific interface. */
/*	Solaris extensions to scb					   */
   struct  adp_scsi_cmd *Sp_cmdp;	/* ptr to the scsi_cmd	*/
   unsigned long Sp_paddr;			/* ccb physical address */
   struct	scsi_arq_status Sp_sense;	/* Auto Request sense	*/
   struct  adp_sg Sp_sg_list[ADP_MAX_DMA_SEGS]; /* scatter/gather */
   unsigned char Sp_CDB_save[MAX_CDB_LEN]; 	/* saved CDB 		*/ 
   UBYTE Sp_SegCnt_save;           /* Number of Scatter/Gather segments   */
   DWORD Sp_SegPtr_save;           /* Pointer to Scatter/Gather list      */
   UBYTE Sp_CDBLen_save;           /* Length of Command Descriptor Block  */
   struct                           /*                                     */
   {                                /*                                     */
      DWORD Cmd:8;                  /* SCB command type                    */
      DWORD Stat:8;                 /* SCB command status                  */
      DWORD Rsvd1:1;                /* Reserved = 0                        */
      DWORD NegoInProg:1;           /* Negotiation in progress             */
      DWORD Rsvd2:2;                /* Reserved = 0                        */
      DWORD OverlaySns:1;           /* 1 = Overlay SCB with Sense Data     */
      DWORD NoNegotiation:1;        /* No negotiation for the scb          */
      DWORD NoUnderrun:1;           /* 1 = Suppress underrun errors        */
      DWORD AutoSense:1;            /* 1 = Automatic Request Sense enabled */
      DWORD MgrStat:8;              /* Intermediate status of SCB          */
   } Sp_control_save;              /*                                     */
   struct sp *Sp_pforw;
   struct sp *Sp_pback;
#ifdef ADP_DEBUG
   struct sp *Sp_forw;
   struct sp *Sp_back;
#endif

#endif   /* _DRIVER */
} sp_struct;

#ifdef _DRIVER
#define  SP_Next        Sp_queue.Next
#endif   /* _DRIVER */
#define  SP_ConfigPtr   Sp_config.ConfigPtr
#define  SP_Cmd         Sp_control.Cmd
#define  SP_Stat        Sp_control.Stat
#define  SP_NegoInProg  Sp_control.NegoInProg
#define  SP_Head        Sp_control.Head
#define  SP_Queued      Sp_control.Queued
#define  SP_OverlaySns  Sp_control.OverlaySns
#define  SP_NoNegotiation Sp_control.NoNegotiation
#define  SP_NoUnderrun  Sp_control.NoUnderrun
#define  SP_AutoSense   Sp_control.AutoSense
#define  SP_MgrStat     Sp_control.MgrStat

#define  SP_Tarlun      Sp_seq_scb.seq_scb_struct.Tarlun
#define  SP_TagType     Sp_seq_scb.seq_scb_struct.TagType
#define  SP_Discon      Sp_seq_scb.seq_scb_struct.Discon
#define  SP_Wait        Sp_seq_scb.seq_scb_struct.Wait
#define  SP_TagEnable   Sp_seq_scb.seq_scb_struct.TagEnable
#define  SP_DisEnable   Sp_seq_scb.seq_scb_struct.DisEnable
#define  SP_RejectMDP   Sp_seq_scb.seq_scb_struct.RejectMDP
#define  SP_CDBLen      Sp_seq_scb.seq_scb_struct.CDBLen
#define  SP_SegCnt      Sp_seq_scb.seq_scb_struct.SegCnt
#define  SP_SegPtr      Sp_seq_scb.seq_scb_struct.SegPtr
#define  SP_CDBPtr      Sp_seq_scb.seq_scb_struct.CDBPtr
#define  SP_TargStat    Sp_seq_scb.seq_scb_struct.TargStat
#define  SP_Holdoff     Sp_seq_scb.seq_scb_struct.Holdoff 
#define  SP_Concurrent  Sp_seq_scb.seq_scb_struct.Concurrent
#define  SP_Abort       Sp_seq_scb.seq_scb_struct.Abort
#define  SP_Aborted     Sp_seq_scb.seq_scb_struct.Aborted
#define  SP_Progress    Sp_seq_scb.seq_scb_struct.Progress
#define  SP_NextPtr     Sp_seq_scb.seq_scb_struct.NextPtr
#define  SP_Offshoot    Sp_seq_scb.seq_scb_struct.Offshoot
#define  SP_ResCnt      Sp_seq_scb.seq_scb_struct.ResCnt
#define  SP_HaStat      Sp_seq_scb.seq_scb_struct.HaStat

/****************************************************************************
                        %%% Sp_Cmd values %%%
****************************************************************************/

#define  EXEC_SCB       0x02        /* Standard SCSI command               */
#define  READ_SENSE     0x01        /*                                     */
#define  SOFT_RST_DEV   0x03        /*                                     */
#define  HARD_RST_DEV   0x04        /*                                     */
#define  NO_OPERATION   0x00        /*                                     */

#define  BOOKMARK       0xFF        /* Used for hard host adapter reset */

#define  ABORT_SCB      0x00        /*                                     */
#define  SOFT_HA_RESET  0x01        /*                                     */
#define  HARD_HA_RESET  0x02        /*                                     */
#define  FORCE_RENEGOTIATE 0x03     /*                                     */

#define  REALIGN_DATA   0x04        /* Re-link config structures with him struct */
#define  BIOS_ON        0x05        /*                */
#define  BIOS_OFF       0x06        /*                */
#define  H_BIOS_OFF     0x10        /*                */

/****************************************************************************
                       %%% Sp_Stat values %%%
****************************************************************************/
#define  SCB_PENDING    0x00        /* SCSI request in progress            */
#define  SCB_COMP       0x01        /* SCSI request completed no error     */
#define  SCB_ABORTED    0x02        /* SCSI request aborted                */
#define  SCB_ERR        0x04        /* SCSI request completed with error   */
#define  INV_SCB_CMD    0x80        /* Invalid SCSI request                */

/****************************************************************************
                       %%% Sp_MgrStat values %%%
****************************************************************************/
#define  SCB_PROCESS    0x00        /* SCB needs to be processed           */
#define  SCB_DONE       SCB_COMP    /* SCB finished without error          */
#define  SCB_DONE_ABT   SCB_ABORTED /* SCB finished due to abort from host */
#define  SCB_DONE_ERR   SCB_ERR     /* SCB finished with error             */
#define  SCB_READY      0x10        /* SCB ready to be loaded into Lance   */
#define  SCB_WAITING    0x20        /* SCB waiting for another to finish   */
#define  SCB_ACTIVE     0x40        /* SCB loaded into Lance               */

#define  SCB_ABORTINPROG 0x44       /* Abort special function in progress  */
#define  SCB_BDR        0x41        /* Bus Device Reset special function   */

#define  SCB_DONE_ILL   INV_SCB_CMD /* SCB finished due to illegal command */

#define  SCB_AUTOSENSE  0x08        /* SCB w/autosense in progress */

#define  SCB_DONE_MASK  SCB_DONE+SCB_DONE_ABT+SCB_DONE_ERR+SCB_DONE_ILL

/****************************************************************************
                   %%% SCB_Tarlun (SCB00) values %%%
****************************************************************************/
#define  TARGET_ID      0xf0        /* SCSI target ID (4 bits)             */
#define  CHANNEL        0x08        /* SCSI Bus selector: 0=chan A,1=chan B*/
#define  LUN            0x07        /* SCSI target's logical unit number   */

/****************************************************************************
                   %%% SCB_Cntrl (SCB01) values %%%
****************************************************************************/
#define  REJECT_MDP     0x80        /* Non-contiguous data                 */
#define  DIS_ENABLE     0x40        /* Disconnection enabled               */
#define  TAG_ENABLE     0x20        /* Tagged Queuing enabled              */
#ifdef   _STANDARD
#define  SWAIT          0x08        /* Sequencer trying to select target   */
#endif   /* _STANDARD */
#define  SDISCON        0x04        /* Traget currently disconnected       */
#define  TAG_TYPE       0x03        /* Tagged Queuing type                 */

/****************************************************************************
                     %%% Sp_TargStat values %%%
****************************************************************************/
#define  UNIT_GOOD      0x00        /* Good status or none available       */
#define  UNIT_CHECK     0x02        /* Check Condition                     */
#define  UNIT_MET       0x04        /* Condition met                       */
#define  UNIT_BUSY      0x08        /* Target busy                         */
#define  UNIT_INTERMED  0x10        /* Intermediate command good           */
#define  UNIT_INTMED_GD 0x14        /* Intermediate condition met          */
#define  UNIT_RESERV    0x18        /* Reservation conflict                */
#define  UNIT_QUEFULL   0x28        /* Queue Full                          */

/***************************************************************************
                      %%% Sp_HaStat values %%%
****************************************************************************/
#define  HOST_NO_STATUS 0x00        /* No adapter status available         */
#define  HOST_ABT_HOST  0x04        /* Command aborted by host             */
#define  HOST_ABT_HA    0x05        /* Command aborted by host adapter     */
#define  HOST_SEL_TO    0x11        /* Selection timeout                   */
#define  HOST_DU_DO     0x12        /* Data overrun/underrun error         */
#define  HOST_BUS_FREE  0x13        /* Unexpected bus free                 */
#define  HOST_PHASE_ERR 0x14        /* Target bus phase sequence error     */
#define  HOST_INV_LINK  0x17        /* Invalid SCSI linking operation      */
#define  HOST_SNS_FAIL  0x1b        /* Auto-request sense failed           */
#define  HOST_TAG_REJ   0x1c        /* Tagged Queuing rejected by target   */
#define  HOST_HW_ERROR  0x20        /* Host adpater hardware error         */
#define  HOST_ABT_FAIL  0x21        /* Target did'nt respond to ATN (RESET)*/
#define  HOST_RST_HA    0x22        /* SCSI bus reset by host adapter      */
#define  HOST_RST_OTHER 0x23        /* SCSI bus reset by other device      */
#define  HOST_NOAVL_INDEX 0x30      /* SCSI bus reset by other device      */

/****************************************************************************
                        %%% SP_SegCnt values %%%
****************************************************************************/

#define  SPEC_OPCODE_00 0x00        /* "SPECIAL" Sequencer Opcode          */

#define  SPEC_BIT3SET   1           /* "SPECIAL" -- Set Bit 3 of the SCB   */
                                    /*              Control Byte           */

#define  ABTACT_CMP     0           /* Abort completed, post SCB */
#define  ABTACT_PRG     1           /* Abort in progress, don't post SCB */
#define  ABTACT_NOT     2           /* Abort unsuccessful */
#define  ABTACT_ERR     -1          /* Abort error */

/****************************************************************************
                      %%% Asynchronous Event Callback Parameters %%%
****************************************************************************/

#define  AE_3PTY_RST    0           /* 3rd party SCSI bus reset         */
#define  AE_HA_RST      1           /* Host Adapter SCSI bus reset      */
#define  AE_TGT_SEL     2           /* Host Adapter selected as target  */

#define  CALLBACK_ASYNC 0           /* Array Index of Asynchronous Event Callback */

/* Type used for OSM callbacks */

typedef void ((*FCNPTR)(DWORD, void *, void *));

/****************************************************************************
               %%% Host Adapter Configuration Structure %%%
****************************************************************************/

typedef struct cfp
{                                   /*    HOST ADAPTER IDENTIFICATION      */
   union
   {
      UWORD AdapterId;              /* Host Adapter ID (ie. 0x7870)        */
      struct
      {
         UBYTE AdapterIdL;          /* Host Adapter ID low  (ie. 0x70)     */
         UBYTE AdapterIdH;          /* Host Adapter ID high (ie. 0x78)     */
      } id_struct;
   } Cf_id;
   UBYTE Cf_ReleaseLevel;           /* HW Interface Module release level   */
   UBYTE Cf_RevisionLevel;          /* HW Interface Module revision level  */
   union
   {
      DWORD BaseAddress;            /* Base address (I/O or memory)        */
      struct aic7870_reg *Base;
   }Cf_base_addr;
                                    /*      INITIALIZATION PARAMETERS      */
   UBYTE Cf_BusNumber;              /* PCI bus number                      */
   UBYTE Cf_DeviceNumber;           /* PCI device number                   */
   union
   {
      DWORD ConfigFlags;            /* Configuration Flags                 */
      struct
      {
         DWORD PrChId:1;            /* AIC-7770 B Channel is primary       */
         DWORD BiosActive:1;        /* SCSI BIOS presently active          */
         DWORD EdgeInt:1;           /* AIC-7770 High Edge Interrupt        */
         DWORD ScsiTimeOut:2;       /* Selection timeout (2 bits)          */
         DWORD ScsiParity:1;        /* Parity checking enabled if equal 1  */
         DWORD ResetBus:1;          /* Reset SCSI bus at 1st initialization*/
         DWORD InitNeeded:1;        /* Initialization required             */
         DWORD MultiTaskLun:1;      /* 1 = Multi-task on target/lun basis  */
                                    /* 0 = Multi-task on target basis only */
         DWORD BiosCached:1;        /* BIOS activated, Driver suspended (AIC-7770)  */
         DWORD BiosAvailable:1;     /* BIOS available for activation (AIC-7770)     */
         DWORD RsevBit11:1;         /* Reserved = 0                        */
         DWORD TerminationLow:1;    /* Low byte termination flag           */
         DWORD TerminationHigh:1;   /* High byte termination flag          */
         DWORD ExtdCfg:1;           /* AIC-7770 configured for two channel */
         DWORD DriverIdle:1;        /* HIM Driver and sequencer idle       */
         DWORD SuppressNego:1;      /* Suppress sync/wide negotiation      */
         DWORD ResvByte2:7;         /* Reserved for future use             */
         DWORD ResvByte3:7;         /* Reserved for future use             */
         DWORD WWQioCnt:1;          /* WW for QINCNT/QOUTCNT (Dagger A WW) */
      } flags_struct;
   } Cf_flags;
   UWORD Cf_AccessMode;             /* SCB handling mode to use:           */
                                    /* 0 = Use default                     */
                                    /* 1 = Internal method (4 SCBs max.)   */
                                    /* 2 = Optima method (255 SCBs max.)   */
                                    /*          HOST CONFIGURATION         */
   UBYTE Cf_ScsiChannel;            /* SCSI channel designator             */
   UBYTE Cf_IrqChannel;             /* Interrupt channel #                 */
   UBYTE Cf_DmaChannel;             /* DMA channel # (ISA only)            */
   UBYTE Cf_BusRelease;             /* PCI Latency time                    */
   UBYTE Cf_Threshold;              /* Data FIFO threshold                 */
   UBYTE Cf_Reserved;               /* Reserved for future expansion       */
   UBYTE Cf_BusOn;                  /* DMA bus-on timing (ISA only)        */
   UBYTE Cf_BusOff;                 /* DMA bus-off timing (ISA only)       */
   UWORD Cf_StrobeOn;               /* DMA bus timing (ISA only)           */
   UWORD Cf_StrobeOff;              /* DMA bus timing (ISA only)           */
                                    /*          SCSI CONFIGURATION         */
   UBYTE Cf_ScsiId;                 /* Host Adapter's SCSI ID              */
   UBYTE Cf_MaxTargets;             /* Maximum number of targets on bus    */
   UBYTE Cf_ScsiOption[16];         /* SCSI config options (1 byte/target) */
   union
   {
      UWORD AllowDscnt;             /* Bit map to allow disconnection      */
      struct
      {
         UBYTE AllowDscntL;         /* Disconnect bit map low              */
         UBYTE AllowDscntH;         /* Disconnect bit map high             */
      } dscnt_struct;
   } Cf_dscnt;
#ifdef   _DRIVER
   union
   {
      struct hsp *HaDataPtr;        /* Pointer to HIM data stucture        */
      DWORD HaDataPtrField;         /* HIM use only for pointer arithmetic */
   } Cf_hsp_ptr;
   DWORD Cf_HaDataPhy;              /* Physical pointer to HIM data str    */
   UWORD Cf_HimDataSize;            /* Size of HIM data structure in bytes */
                                    /* Valid after call to get_config      */
   UWORD Cf_MaxNonTagScbs;          /* Max nontagged SCBs per target/LUN   */
                                    /* Valid settings are 1 or 2           */
   UWORD Cf_MaxTagScbs;             /* Max tagged SCBs per target/LUN      */
                                    /* Valid settings are 1 to 32          */
   UWORD Cf_NumberScbs;             /* Number of SCBs                      */
                                    /* Valid setting are 1 to 255          */
   UBYTE Cf_DReserved[32];          /* Reserved for future expansion       */
   OS_TYPE Cf_OSspecific;           /* Pointer to custom data structures   */
                                    /* required by O/S specific interface. */

   /* The Following parameters are available in extended mode only. */

   FCNPTR Cf_CallBack[8];           /* OSM Callbacks */

   DWORD Cf_ExtdFlags;              /* Extended flag word */

#endif

/*	Solaris additions to block			*/
	dev_info_t	*ab_dip;
	struct sp 	*ab_scbp;
	struct sp 	*ab_scbp_que;
	struct sp 	*ab_last_scbp;
	unsigned int	ab_ioaddr;
	kmutex_t 	ab_mutex;
	u_int 		ab_intr_idx;
	void 		*ab_iblock;          /* ddi_iblock cookie ptr */
	ushort 		ab_child;
	ushort		ab_pkts_done;
	unsigned char 	ab_hostid;
	unsigned char 	ab_flag;
	ddi_softintr_t	ab_softid;
#ifdef ADP_DEBUG
	struct sp *ab_scboutp;
	ushort	ab_pkts_out;
#endif

} cfp_struct;

/* ab_flag defines for Solaris in adp_block */
#define ADP_POLLING		1
#define ADP_POLL_TRIGGER_ON	2
#define ADP_GT1GIG		4

#define  CFP_AdapterId        Cf_id.AdapterId
#define  CFP_AdapterIdL       Cf_id.id_struct.AdapterIdL
#define  CFP_AdapterIdH       Cf_id.id_struct.AdapterIdH
#define  CFP_BaseAddress      Cf_base_addr.BaseAddress
#define  CFP_Base             Cf_base_addr.Base
#define  CFP_ConfigFlags      Cf_flags.ConfigFlags
#define  CFP_BiosActive       Cf_flags.flags_struct.BiosActive
#define  CFP_ScsiTimeOut      Cf_flags.flags_struct.ScsiTimeOut
#define  CFP_ScsiParity       Cf_flags.flags_struct.ScsiParity
#define  CFP_ResetBus         Cf_flags.flags_struct.ResetBus
#define  CFP_InitNeeded       Cf_flags.flags_struct.InitNeeded
#define  CFP_MultiTaskLun     Cf_flags.flags_struct.MultiTaskLun
#define  CFP_OptimaMode       Cf_flags.flags_struct.OptimaMode
#define  CFP_DriverIdle       Cf_flags.flags_struct.DriverIdle
#define  CFP_SuppressNego     Cf_flags.flags_struct.SuppressNego
#define  CFP_ExtdCfg          Cf_flags.flags_struct.ExtdCfg
#define  CFP_TerminationLow   Cf_flags.flags_struct.TerminationLow
#define  CFP_TerminationHigh  Cf_flags.flags_struct.TerminationHigh
#define  CFP_WWQioCnt         Cf_flags.flags_struct.WWQioCnt
#ifdef   _DRIVER
#define  CFP_HaDataPtr        Cf_hsp_ptr.HaDataPtr
#define  CFP_HaDataPtrField   Cf_hsp_ptr.HaDataPtrField
#endif   /* _DRIVER */
#define  CFP_AllowDscnt       Cf_dscnt.AllowDscnt
#define  CFP_AllowDscntL      Cf_dscnt.dscnt_struct.AllowDscntL
#define  CFP_AllowDscntH      Cf_dscnt.dscnt_struct.AllowDscntH

/****************************************************************************
                     %%% HA_config_flags values %%%
****************************************************************************/
#define  DRIVER_IDLE    0x8000      /* HIM Driver and sequencer idle       */
#define  DIFF_SCSI      0x0200      /* BIOS sequencer currently swapped in */
#define  INIT_NEEDED    0x0080      /* Initialization required             */
#define  RESET_BUS      0x0040      /* Reset SCSI bus at 1st initialization*/
#define  SCSI_PARITY    ENSPCHK     /* Parity checking enabled if equal 1  */
                                    /* ENSPCHK defined in SXFRCTL1         */
#define  STIMESEL       STIMESEL0 + STIMESEL1 /* Selection timeout (2 bits)*/
                                    /* STIMESEL(0-1) defined in SXFRCTL1   */
#define  BIOS_ACTIVE    0x0002      /* SCSI BIOS presently active          */
#define  OPTIMA_MODE    0x4000      /* Driver configured for OPTIMA mode   */

/****************************************************************************
                      %%% scsi_options values %%%
****************************************************************************/
#define  WIDE_MODE      0x80        /* Allow wide negotiation if equal 1   */
#define  SYNC_RATE      0x70        /* 3-bit mask for maximum transfer rate*/
#define  SYNC_MODE      0x01        /* Allow synchronous negotiation if = 1*/

/*****************************************/
/*    Generic union of DWORD/UWORD/UBYTE */
/*****************************************/
typedef union genunion
{
   UBYTE ubyte[4];
   UWORD uword[2];
   DWORD dword;
} gen_union;

/***************************************/
/*    DWORD alligned sp_struct pointer */
/***************************************/
typedef union sp_dword
{
   DWORD dword;
   sp_struct *sp;
}sp_dword;

/****************************************************************************
                  %%% Host Adapter Data Structure %%%
****************************************************************************/

#ifdef   _DRIVER
#ifdef   _OPTIMA
#define  MAX_ADDITIONAL 256*4+256+256+4*256+256+256+256+256
#else    /* !_OPTIMA */
#define  MAX_ADDITIONAL 256*4+256+256
#endif   /* _OPTIMA */
typedef struct hsp
{
   union sp_dword Head_Of_Q;        /* First SCB ptr on Queue              */
   union sp_dword End_Of_Q;         /* Last SCB ptr on Queue               */
   UBYTE acttarg[256];              /* Array: # of active SCB's per target */
                                    /* HIM use only for pointer arithmetic */
   UWORD sel_cmp_brkpt;             /* Sequencer address for breakpoint on
                                       target selection complete           */
   UWORD ready_cmd;                 /* Number of ready SCBs                */
   UBYTE Hsp_MaxNonTagScbs;         /* Max nontagged SCBs per target/LUN   */
   UBYTE Hsp_MaxTagScbs;            /* Max tagged SCBs per target/LUN      */
   UBYTE done_cmd;                  /* Index to done SCB array             */
   UBYTE free_hi_scb;               /* High free SCB allocation index      */
   union
   {
      sp_struct **active_ptr;       /* active SCB pointer array            */
      DWORD active_ptr_field;       /* HIM use only for pointer arithmetic */
   }a_ptr;
   union
   {
      UBYTE *free_stack;            /* Pointer to free SCB array           */
      DWORD free_stack_field;       /* HIM use only for pointer arithmetic */
   }f_ptr;
   union
   {
      UBYTE *done_stack;            /* Pointer to done command array       */
      DWORD done_stack_field;       /* HIM use only for pointer arithmetic */
   }d_ptr;


   UWORD SemState;                  /* Semaphore for send command          */

   UBYTE HaFlags;                   /* Flags USED by HA                    */

   UBYTE Hsp_SaveState;             /* Used to describe current SCRATCH RAM*/
                                    /*  State.                             */

   UBYTE Hsp_SaveScr[64];           /* SCRATCH RAM Save Area within        */
                                    /*  him_struct                         */

   struct sp scb_mark;              /* Used in Ph_HahardReset as scb for   */
                                    /*  Bookmark                           */
   gen_union zero;                  /* DWORD/UWORD/UBYTE zero              */
   UWORD (*calcmodesize)();         /* calculate mode dependent size       */
   UBYTE (*morefreescb)();          /* check if more free scb available    */
   DWORD (*cmdcomplete)();          /* command complete handler            */
   UBYTE (*getfreescb)();           /* get free scb                        */
   void  (*returnfreescb)();        /* return free scb                     */
   int   (*abortactive)();          /* abort (active) scb                  */
   void  (*clearqinfifo)();         /* clear qin fifo                      */
   void  (*indexclearbusy)();       /* clear busy indexed entry            */
   void  (*cleartargetbusy)();      /* clear target busy                   */
   void  (*requestsense)();         /* request sense                       */
   void  (*enque)();                /* enqueue for sequencer               */
   void  (*enquehead)();            /* enqueue scb on Head for sequencer   */
   void  (*enablenextscbarray)();   /* enable next scb array               */

#ifdef _OPTIMA
   UBYTE free_lo_scb;               /* Low free SCB allocation index       */
   UBYTE qin_index;                 /* Queue index                         */
   UBYTE qout_index;                /* Queue out index                     */
   UBYTE patch[1];                  /* patch to make DWORD allignment      */
   union
   {
      DWORD *scb_ptr_array;         /* Pointer to external SCB array       */
      DWORD scb_ptr_array_field;    /* HIM use only for pointer arithmetic */
   }s_ptr;
   union
   {
      UBYTE *qin_ptr_array;         /* Pointer to external queue in array  */
      DWORD qin_ptr_array_field;    /* HIM use only for pointer arithmetic */
   }qi_ptr;
   union
   {
      UBYTE *qout_ptr_array;        /* Pointer to external queue out array */
      DWORD qout_ptr_array_field;   /* HIM use only for pointer arithmetic */
   }qo_ptr;
   union
   {
      UBYTE *busy_ptr_array;        /* Pointer to busy target array */
      DWORD busy_ptr_array_field;   /* HIM use only for pointer arithmetic */
   }b_ptr;
#endif   /* _OPTIMA */
#ifdef _HIM_DEBUG
   DWORD debug_value;
#endif /* _HIM_DEBUG */

   UBYTE max_additional[MAX_ADDITIONAL]; /* maximum additional memory      */
} hsp_struct;
#endif   /* _DRIVER */

#define  ACTTARG        ha_ptr->acttarg
#define  FREE_LO_SCB    ha_ptr->free_lo_scb
#define  FREE_HI_SCB    ha_ptr->free_hi_scb
#define  FREE_STACK     ha_ptr->f_ptr.free_stack
#define  DONE_CMD       ha_ptr->done_cmd
#define  DONE_STACK     ha_ptr->d_ptr.done_stack
#define  ACTPTR         ha_ptr->a_ptr.active_ptr
#define  FREE_SCB       FREE_HI_SCB
#define  NULL_SCB       0xFF

#define  SCB_PTR_ARRAY  ha_ptr->s_ptr.scb_ptr_array
#define  QIN_PTR_ARRAY  ha_ptr->qi_ptr.qin_ptr_array
#define  QOUT_PTR_ARRAY ha_ptr->qo_ptr.qout_ptr_array
#define  BUSY_PTR_ARRAY ha_ptr->b_ptr.busy_ptr_array

#define  Head_Of_Queue  Head_Of_Q.sp
#define  End_Of_Queue   End_Of_Q.sp

/****************************************************************************
                        %%% Miscellaneous %%%
****************************************************************************/
#define  SCB_LENGTH     0x1c        /* # of SCB bytes to download to Lance */
#define  NOERR          0           /* Error return value                  */
#define  ADP_ERR        0xff        /* Error return value                  */
#define  NONEGO         0x00        /* Responding to target's negotiation  */ 
#define  NEEDNEGO       0x8f        /* Need negotiation response from targ */

/****************************************************************************
                       %%% Hsp_SaveState values %%%
****************************************************************************/
#define  SCR_REALMODE        0x02    /* REAL MODE Enabled.BIOS_ON Completed*/
                                     /*   SCRATCH RAM has been SWAPPED with*/
                                     /*   MEMORY.                          */
#define  SCR_PROTMODE        0x03    /* PROTECTED MODE Enabled. H_BIOS_OFF */
                                     /*   Completed. SCRATCH RAM has been  */
                                     /*   Restored from MEMORY & SWAP done.*/


/****************************************************************************
                      %%% targ_option values %%%
****************************************************************************/
#define  ALLOW_DISCON   0x04        /* Allow targets to disconnect         */
#define  SYNCNEG        0x08        /* Init for synchronous negotiation    */
#define  PARITY_ENAB    0x20        /* Enable SCSI parity checking         */
#define  WIDENEG        0x80        /* Init for Wide negotiation           */
#define  MAX_SYNC_RATE  0x07        /* Maximum synchronous transfer rate   */

/****************************************************************************
                      %%% SemState values %%%
****************************************************************************/
#define  SEM_RELS       0           /* semaphore released                  */
#define  SEM_LOCK       1           /* semaphore locked                    */


/*              CONSTANTS FOR FAULT TOLERANT CODE                          */

/* Return Codes for scb_deque */

#define  DQ_GOOD        0x00        /* SCB successfully dequeued */
#define  DQ_FAIL        0xFF        /* SCB is ACTIVE, cannot dequeue */

/* Values for actstat array in ha_struct */

#define  AS_SCBFREE        0x00
#define  AS_SCB_ACTIVE     0x01
#define  AS_SCB_ABORT      0x02
#define  AS_SCB_ABORT_CMP  0x03
#define  AS_SCB_BIOS       0x40
#define  AS_SCB_RST        0x10
                        
#define  ABORT_DONE     0x00
#define  ABORT_INPROG   0x01
#define  ABORT_FAILED   0xFF

/* "NULL ptr-like" value (Don't want to include libraries ) */

#define  NOT_DEFINED    (sp_struct *) - 1
#define  BIOS_SCB       (sp_struct *) - 2

/* Values for scsi_state in send_trm_msg */

#define  NOT_CONNECTED     0x0C
#define  CONNECTED         0x00
#define  WAIT_4_SEL        0x88
#define  OTHER_CONNECTED   0x40

/* Return code mask when int_handler detects BIOS interrupt */

#define  BIOS_INT          0x80

#define  NOT_OUR_INT       0x80
#define  OUR_CC_INT        0x40
#define  OUR_OTHER_INT     0x20
#define  OUR_INT           0x60
#define  OUR_SW_INT        0x10

/* Return code mask when findha detects that the chip is disabled */

#define  NOT_ENABLED       0x80

/* Return code when ReadConfigOSM cannot access configuration space */

#define  NO_CONFIG_OSM     0x55555555

/* Return code when AccessConfig determines the config space access mechanism */

#define  PCI_MECHANISM1        0x01
#define  PCI_MECHANISM2        0x02

/****************************************************************************
               %%% BIOS Information Structure %%%
****************************************************************************/

#define  BI_BIOS_ACTIVE    0x01
#define  BI_GIGABYTE       0x02        /* Gigabyte support enabled */
#define  BI_DOS5           0x04        /* DOS 5 support enabled */

#define  BI_MAX_DRIVES     0x08        /* DOS 5 maximum, 2 for DOS 3,4 */

#define SCRATCH_BANK       0           /* additional scratch bank    */

#define  BIOS_BASE         0
#define  BIOS_DRIVES       BIOS_BASE + 4
#define  BIOS_GLOBAL       BIOS_BASE + 12
#define  BIOS_FIRST_LAST   BIOS_BASE + 14
#define  BIOS_INTVECTR     BIOS_BASE + 16 /* storage for BIOS int vector  */

#define  BIOS_GLOBAL_DUAL  0x08
#define  BIOS_GLOBAL_WIDE  0x04
#define  BIOS_GLOBAL_GIG   0x01

#define  BIOS_GLOBAL_DOS5  0x40

typedef struct bios_info_block {

   UBYTE bi_global;
   UBYTE bi_first_drive;
   UBYTE bi_last_drive;
   UBYTE bi_drive_info[BI_MAX_DRIVES];
   UWORD bi_bios_segment;
   UBYTE bi_reserved[3];

} bios_info;

/****************************************************************************

 LANCE SCSI REGISTERS DEFINED AS AN ARRAY IN A STRUCTURE

****************************************************************************/

typedef struct aic7870_reg
{
   UBYTE aic7870[192];
}aic_7870;

#ifdef _MEMMAP
#define  AIC7870 (DWORD) &base->aic7870
#define  AIC_7870 aic_7870 FAR
#endif   /* _MEMMAP */

#ifdef _IOMAP
#if defined(_X86_)
#define  AIC7870 (UWORD) &base->aic7870
#else
#define  AIC7870 (DWORD) &base->aic7870
#endif
#define  AIC_7870 aic_7870
#endif   /* _IOMAP */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  Function Prototypes
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*********************************************************************
*  HIM_INIT.C
*********************************************************************/
#ifndef _LESS_CODE
UWORD PH_GetNumOfBuses ();
#endif   /* _LESS_CODE */
UWORD PH_FindHA (WORD,WORD);
void PH_GetConfig (cfp_struct *);
UWORD PH_InitHA (cfp_struct *);
                 
DWORD Ph_ReadConfig (cfp_struct *,UBYTE,UBYTE,UBYTE);
UBYTE Ph_AccessConfig (UBYTE);
int Ph_WriteConfig (cfp_struct *,UBYTE,UBYTE,UBYTE,DWORD);
UWORD Ph_ReadEeprom (cfp_struct *,register AIC_7870 *);
UWORD Ph_ReadE2Register (UWORD,register AIC_7870 *,int);
void Ph_Wait2usec (UBYTE,register AIC_7870 *);
int Ph_LoadSequencer (cfp_struct *);
DWORD PH_ReadConfigOSM (cfp_struct *,UBYTE,UBYTE,UBYTE);
DWORD PH_WriteConfigOSM (cfp_struct *,UBYTE,UBYTE,UBYTE,DWORD);
DWORD PH_GetNumOfBusesOSM ();

/*********************************************************************
*  HIM.C
*********************************************************************/
int PH_Special (UBYTE,cfp_struct *,sp_struct *);
void PH_EnableInt (cfp_struct *);
void PH_DisableInt (cfp_struct *);

#ifdef   _FAULT_TOLERANCE
UBYTE Ph_NonInit (sp_struct *);
void Ph_Abort (sp_struct *);
#endif   /* _FAULT_TOLERANCE */

void Ph_IntSrst (cfp_struct *);
void Ph_ResetChannel (cfp_struct *);
void Ph_CheckSyncNego (cfp_struct *);
void Ph_CheckLength (sp_struct *,register AIC_7870 *);

#ifndef _LESS_CODE
void Ph_CdbAbort (sp_struct *,register AIC_7870 *);
UBYTE Ph_TargetAbort (cfp_struct *, sp_struct *, register AIC_7870 *);
#endif   /* _LESS_CODE */

void Ph_ResetSCSI (AIC_7870 *);
void Ph_BadSeq (cfp_struct *,register AIC_7870 *);
void Ph_CheckCondition (sp_struct *,register AIC_7870 *);
void Ph_HandleMsgi (sp_struct *,register AIC_7870 *);
UBYTE Ph_SendTrmMsg ( cfp_struct *,UBYTE);

#ifdef   _FAULT_TOLERANCE
UBYTE Ph_Wt4BFree (cfp_struct *);
UBYTE Ph_TrmCmplt ( cfp_struct *,UBYTE,UBYTE);
UBYTE Ph_ProcBkpt (cfp_struct *);
#endif   /* _FAULT_TOLERANCE */

void  Ph_BusReset (cfp_struct *);
void Ph_SendMsgo (sp_struct *,register AIC_7870 *);
void Ph_SetNeedNego (UBYTE, register AIC_7870 *);
void Ph_Negotiate (sp_struct *,register AIC_7870 *);
UBYTE Ph_SyncSet (sp_struct *);
void Ph_SyncNego (sp_struct *,register AIC_7870 *);
void Ph_ExtMsgi (sp_struct *,register AIC_7870 *);
UBYTE Ph_ExtMsgo (sp_struct *,register AIC_7870 *);
void Ph_IntSelto (cfp_struct *,sp_struct *);
void Ph_IntFree (cfp_struct *,sp_struct *);
void Ph_ParityError (register AIC_7870 *);
UBYTE Ph_Wt4Req (register AIC_7870 *);
void Ph_MultOutByte (register AIC_7870 *,UBYTE,int);
void Ph_MemorySet(UBYTE *,UBYTE,int);
void Ph_Pause (register AIC_7870 *);
void Ph_UnPause (register AIC_7870 *);
void Ph_WriteHcntrl (register AIC_7870 *,UBYTE);
UBYTE Ph_ReadIntstat (register AIC_7870 *);
void Ph_DelaySeconds(register AIC_7870 *,int);
void Ph_ScbRenego (cfp_struct *,UBYTE);

#ifdef _BIOS
/*********************************************************************
*  HIMBINIT.C
*********************************************************************/
#ifdef PUT_BIOS_INFO
UWORD PH_PutBiosInfo (cfp_struct *,UBYTE *);
#endif   /* PUT_BIOS_INFO */

void Ph_GetBiosConfig (cfp_struct *);
UWORD Ph_InitBiosHA (cfp_struct *);
UWORD Ph_SetBiosScratch (cfp_struct *);

/*********************************************************************
*  HIMB.C
*********************************************************************/
void PH_ScbSend (sp_struct *);
UBYTE PH_IntHandler (cfp_struct *);
UBYTE PH_PollInt (cfp_struct *);

void Ph_TerminateCommand (sp_struct *,UBYTE);
UBYTE Ph_GetFreeScb (cfp_struct *);
void Ph_ReturnFreeScb (cfp_struct *,UBYTE);
void Ph_PostCommand (cfp_struct *);
#ifdef   _FAULT_TOLERANCE
void Ph_AbortChannel (cfp_struct *,UBYTE);
void Ph_BusDeviceReset (sp_struct *);
#endif   /* _FAULT_TOLERANCE */
UBYTE Ph_GetCurrentScb (cfp_struct *);
void  Ph_HaHardReset (cfp_struct *);
void  Ph_HaSoftReset (cfp_struct *);
void Ph_HardReset (cfp_struct *,sp_struct *);
void  Ph_SoftReset (cfp_struct *, sp_struct *);
void Ph_BiosEnque (sp_struct *,register AIC_7870 *);
void Ph_BiosRequestSense (sp_struct *,UBYTE);
void Ph_BiosClearDevQue (sp_struct *,AIC_7870 *);
void Ph_BiosIndexClearBusy (cfp_struct *,UBYTE);
int Ph_BiosAbortActive (cfp_struct *,sp_struct *);
void Ph_ClearChannelBusy (cfp_struct *);
void Ph_SetMgrStat (sp_struct *);
void Ph_BiosClearTargetBusy (cfp_struct *,UBYTE);
#endif   /* _BIOS */
      
#ifdef _DRIVER                   
/*********************************************************************
*  HIMDINIT.C
*********************************************************************/
/* First parm cfp_struct added for SunSoft				*/
UWORD PH_GetBiosInfo (cfp_struct *, UBYTE,UBYTE,bios_info *);
UWORD PH_CalcDataSize (UBYTE,UWORD);

void Ph_GetDrvrConfig (cfp_struct *);
void Ph_InitDrvrHA (cfp_struct *);
void Ph_SetHaData (cfp_struct *);
UWORD Ph_CheckBiosPresence (cfp_struct *);
UWORD Ph_SetDrvrScratch (cfp_struct *);
                      
/*********************************************************************
*  HIMD.C
*********************************************************************/
void PH_ScbSend (sp_struct *);
UBYTE PH_IntHandler (cfp_struct *);
UBYTE PH_PollInt (cfp_struct *);

void PH_RelocatePointers (cfp_struct *,UWORD);
void Ph_ChainAppendEnd (cfp_struct *,sp_struct *);
void Ph_ChainInsertFront (cfp_struct *,sp_struct *);
void Ph_ChainRemove (cfp_struct *,sp_struct *);
sp_struct *Ph_ChainPrevious (hsp_struct *,sp_struct *);
void Ph_ScbPrepare (cfp_struct *,sp_struct *);
UBYTE Ph_SendCommand (hsp_struct *,register AIC_7870 *);
void Ph_TerminateCommand (sp_struct *,UBYTE);
void Ph_PostCommand (cfp_struct *);
void Ph_RemoveAndPostScb (cfp_struct *,sp_struct *);
void Ph_PostNonActiveScb (cfp_struct *,sp_struct *);
void Ph_TermPostNonActiveScb (sp_struct *);
void Ph_AbortChannel (cfp_struct *,UBYTE);
UBYTE Ph_GetCurrentScb (cfp_struct *);
void Ph_HaHardReset (cfp_struct *);
void Ph_SetScbMark (cfp_struct *);
void  Ph_HaSoftReset (cfp_struct *);
void Ph_HardReset (cfp_struct *,sp_struct *);
void  Ph_SoftReset (cfp_struct *, sp_struct *);
UBYTE Ph_GetScbStatus (hsp_struct *,sp_struct *);
void Ph_SetMgrStat (sp_struct *);
void Ph_WriteBiosInfo (cfp_struct *,UWORD,UBYTE *,UWORD);
void Ph_ReadBiosInfo (cfp_struct *,UWORD,UBYTE *,UWORD);
void Ph_OutBuffer (register AIC_7870 *,UBYTE *,int);
void Ph_InBuffer (register AIC_7870 *,UBYTE *,int);
int SWAPCurrScratchRam (cfp_struct *,UBYTE );
void Ph_InsertBookmark (cfp_struct *);
void Ph_RemoveBookmark (cfp_struct *);
#ifdef   _FAULT_TOLERANCE
void Ph_AbortChannel (cfp_struct *,UBYTE);
void Ph_BusDeviceReset (sp_struct *);
void Ph_ScbAbort (sp_struct *);
void Ph_ScbAbortActive (cfp_struct *,sp_struct *);
int  Ph_AsynchEvent( DWORD, cfp_struct *, void *);
#endif   /* _FAULT_TOLERANCE */

#ifdef _OPTIMA
/*********************************************************************
*  HIMDIOPT.C
*********************************************************************/
void Ph_GetOptimaConfig (cfp_struct *);
UWORD Ph_CalcOptimaSize (UWORD);

/*********************************************************************
*  HIMDOPT.C
*********************************************************************/
void Ph_OptimaEnque(UBYTE,sp_struct *, register AIC_7870 *);
void Ph_OptimaEnqueHead(UBYTE,sp_struct *, AIC_7870 *);
void Ph_OptimaQHead (UBYTE,sp_struct *, AIC_7870 *);
DWORD Ph_OptimaCmdComplete(cfp_struct *, UBYTE,DWORD);
void Ph_OptimaRequestSense(sp_struct *, UBYTE);
void Ph_OptimaClearDevQue(sp_struct *, AIC_7870 *);
void Ph_OptimaIndexClearBusy(cfp_struct *, UBYTE);
void Ph_OptimaCheckClearBusy(cfp_struct *, UBYTE);
int Ph_OptimaAbortActive(sp_struct *);
void Ph_OptimaClearChannelBusy(cfp_struct *);
void Ph_SetOptimaHaData(cfp_struct *);
void Ph_SetOptimaScratch(cfp_struct *);
UBYTE Ph_OptimaMoreFreeScb(hsp_struct *, sp_struct *);
UBYTE Ph_OptimaGetFreeScb(hsp_struct *, sp_struct *);
void Ph_OptimaReturnFreeScb(cfp_struct *, UBYTE);
void Ph_OptimaAbortScb(sp_struct *, UBYTE);
void Ph_OptimaClearQinFifo(cfp_struct *);
void Ph_OptimaLoadFuncPtrs(cfp_struct *);
void Ph_MovPtrToScratch(UWORD,DWORD, cfp_struct *);
UBYTE * Ph_ScbPageJustifyQIN(cfp_struct *);
void Ph_OptimaClearTargetBusy (cfp_struct *, UBYTE);
void Ph_OptimaEnableNextScbArray (sp_struct *);
#endif   /* _OPTIMA */

#ifdef _STANDARD
/*********************************************************************
*  HIMDISTD.C
*********************************************************************/
void Ph_GetStandardConfig (cfp_struct *);
UWORD Ph_CalcStandardSize (UWORD);
UWORD Ph_FindNumOfScbs (register AIC_7870 *);

/*********************************************************************
*  HIMDSTD.C
*********************************************************************/
void Ph_StandardEnque (UBYTE,sp_struct *,register AIC_7870 *);
void Ph_StandardEnqueHead (UBYTE,sp_struct *,AIC_7870 *);
DWORD Ph_StandardCmdComplete (cfp_struct *,UBYTE,DWORD);
void Ph_StandardRequestSense (sp_struct *,UBYTE);
void Ph_StandardClearDevQue (sp_struct *,AIC_7870 *);
void Ph_StandardIndexClearBusy (cfp_struct *,UBYTE);
int Ph_StandardAbortActive (cfp_struct *,sp_struct *);
void Ph_StandardClearChannelBusy (cfp_struct *);
void Ph_SetStandardHaData (cfp_struct *);
void Ph_SetStandardScratch (cfp_struct *);
UBYTE Ph_StandardMoreFreeScb (hsp_struct *,sp_struct *);
UBYTE Ph_StandardGetFreeScb (hsp_struct *,sp_struct *);
void Ph_StandardReturnFreeScb (cfp_struct *,UBYTE);
void Ph_StandardAbortScb(sp_struct *,UBYTE);
void Ph_StandardClearQinFifo(cfp_struct *);
void Ph_StandardLoadFuncPtrs (cfp_struct *);
void Ph_StandardClearTargetBusy (cfp_struct *,UBYTE);
void Ph_StandardEnableNextScbArray (sp_struct *);
#endif   /* _STANDARD */
#endif   /* _DRIVER */
/****************************************************************************

 LANCE SCRATCH RAM AREA

 Included for Debug Purposes. This documentation is very important to
 an individual who owns a bunch of firearms. Please do not remove.

****************************************************************************/
/* #define SCRATCH                  0x20           scratch base address       */
/* #define SCRATCH_WAITING_SCB      SCRATCH+27     waiting scb             3B */
/* #define SCRATCH_ACTIVE_SCB       SCRATCH+28     active scb              3C */

/* The Following SCRATCH Registers Modified for LANCE                         */

/* #define SCRATCH_SCB_PTR_ARRAY    SCRATCH+29     scb pointer array       3D */
/* #define SCRATCH_QIN_CNT          SCRATCH+33     queue in count          41 */
/* #define SCRATCH_QIN_PTR_ARRAY    SCRATCH+34     queue in pointer array  42 */
/* #define SCRATCH_NEXT_SCB_ARRAY   SCRATCH+38     next scb array          46 */
/* #define SCRATCH_QOUT_PTR_ARRAY   SCRATCH+54     queue out ptr array     56 */
/* #define SCRATCH_BUSY_PTR_ARRAY   SCRATCH+58     busy ptr array          5A */

/* Seqint Driver interrupt codes identify action to be taken by the Driver.*/

/* #define  SYNC_NEGO_NEEDED  0x00     initiate synchronous negotiation    */
/* #define  ATN_TIMEOUT       0x00     timeout in atn_tmr routine          */
/* #define  CDB_XFER_PROBLEM  0x10     possible parity error in cdb:  retry*/
/* #define  HANDLE_MSG_OUT    0x20     handle Message Out phase            */
/* #define  DATA_OVERRUN      0x30     data overrun detected               */
/* #define  UNKNOWN_MSG       0x40     handle the message in from target   */
/* #define  CHECK_CONDX       0x50     Check Condition from target         */
/* #define  PHASE_ERROR       0x60     unexpected scsi bus phase           */
/* #define  EXTENDED_MSG      0x70     handle Extended Message from target */
/* #define  ABORT_TARGET      0x80     abort connected target              */
/* #define  NO_ID_MSG         0x90     reselection with no id message      */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_DKTP_ADP_HIM_SCB_H */
