
/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

/* $Header:   Y:\source\aic-7870\him\common\him_init.cv_   1.58.1.4   05 Oct 1995 14:37:16   KLI  $ */

#pragma ident	"@(#)adp_him_init.c	1.11	96/03/13 SMI"

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
*  Module Name:   AIC-7870/7850 HIM (Hardware Interface Module)
*
*  Version:       1.01
*                 
*  Source Code:   HIM_INIT.C  HIMBINIT.C HIMDINIT.C HIMDISTD.C HIMDIOPT.C
*                 HIM.C HIMB.C HIMD.C HIMDSTD.C HIMDOPT.C
*                 HIM_REL.H   HIM_EQU.H   HIM_SCB.H
*                 SEQUENCE.H  SEQ_OFF.H   MACRO.H
*
*  Base Code #:   xxxxxx-00
*
*  Description:   Hardware Interface Module for linking/compiling with
*                 software drivers supporting the AIC-7870 and AIC-7870
*                 based host adapters (ie. Javelin).
*
*  History:
*
***************************************************************************/

/****************************************************************************
*
*  Module Name:   HIM_INIT.C
*
*  Description:
*                 Codes common to HIM at initialization are defined here. 
*                 It should always be included independent of configurations
*                 and modes of operation. These codes can be thrown away 
*                 after HIM initialization.
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
*     PH_GetNumOfBuses - Return number of PCI buses present in system
*     PH_FindHA        - Look for Host Adapter at Bus/Device "slot"
*     PH_GetConfig     - Initialize HIM data structures
*     PH_InitHA        - Initialize Host Adapter
*
*  Revisions -
*
****************************************************************************/

#include "him_scb.h"
#include "him_equ.h"
#include "seq_off.h"
#include "sequence.h"
#include "him_rel.h"

/*********************************************************************
*
*   PH_GetNumOfBuses -
*
*   This routine will interrogate all the HOST-TO-PCI bridges as well
*   as all the PCI-TO-PCI bridges to determine the total number of 
*   PCI buses present in the system.
*
*  Return Value:  Highest number of PCI bus plus one
*
*  Parameters:    none
*
*  Activation:    ASPI layer, initialization.
*
*  Remarks:       The first attempt to retrieve the total number of
*                 buses shall be from OSM using SYSTEM BIOS call INT 1A,
*                 If it is available.  Otherwise, 0x55555555 shall be
*                 returned to this routine and the 2nd scheme is used.
*
*                 The Host-to_PCI bridge chip can be any device number
*                 on the PCI bus.
*
*********************************************************************/
#ifndef  _LESS_CODE
UWORD PH_GetNumOfBuses ()
{
   cfp_struct *NUL_PTR = (cfp_struct *) NOT_DEFINED;
   UWORD nofbus, next_busnumber;
   UBYTE subordinate_busnumber;
   UBYTE high_subordinate_busnumber;
   UBYTE i, host_bridge_dev;
   DWORD class_code, nofbus_dword;
   
   /* NUL_PTR = (cfp_struct *) NOT_DEFINED;  */

   if ((nofbus_dword=PH_GetNumOfBusesOSM()) != NO_CONFIG_OSM)
   {
      nofbus = (UWORD)(nofbus_dword & 0x0000ffff);
      return(nofbus);
   }

   nofbus = 0;
   subordinate_busnumber=0;         /* in case there are no pci-pci bridge */
   high_subordinate_busnumber=0;    /* same as above */

   /*
      starting with '0' as the root PCI bus number and search for the
      highest subordinate bus number of the current bridge, if one
      exists.  Move on to the next higher PCI bus number and check for
      the existence of host-to-pci bridge class code before scanning for
      other pci bridges.
   */
   for ( next_busnumber = 0;
         next_busnumber <= 255;
         next_busnumber =   (subordinate_busnumber==0)
                          ? (next_busnumber+1)
                          : (subordinate_busnumber+1) )
   {
      subordinate_busnumber = 0;

      /* scan for host-to-PCI bridge device.       */
      for (host_bridge_dev=0; host_bridge_dev<32; host_bridge_dev++)
      {
         class_code = 0xffffff00 & Ph_ReadConfig( NUL_PTR,
                                                (UBYTE)(next_busnumber&0xff),
                                                 host_bridge_dev,
                                                 DEV_REV_ID);

         if (   (class_code == 0x06000000)   /* either class code represents  */
             || (class_code == 0x00000000))  /* a possible host-to-pci bridge */
         {

            ++nofbus;

            /* go through each device on the current bus to find all */
            /* PCI to PCI bridges. 32 devices maximum per PCI bus    */
            for (i=0; i<32; i++)
            {
               /* make sure use class code etc match with the PCI to PCI and  */
               /* use the subordinate bus number to update the maximum number */
               /* of buses available up to this host-to-PCI bridge            */
               if (i==host_bridge_dev)       /* skip the host bridge device   */
                  continue;

               class_code = 0xffffff00 & Ph_ReadConfig( NUL_PTR,
                                                        (UBYTE)(next_busnumber&0xff),
                                                        i,
                                                        DEV_REV_ID);
               if (class_code == 0x06040000)
               {
                  subordinate_busnumber = (UBYTE) ((Ph_ReadConfig( NUL_PTR,
                                                                   (UBYTE)(next_busnumber&0xff),
                                                                   i,
                                                                   BUS_NUMBER) >> 16 ) & 0xFF);
                  if ( (UWORD)(subordinate_busnumber+1) > nofbus )
                  {
                     nofbus = (UWORD)(subordinate_busnumber+1);
                     high_subordinate_busnumber = subordinate_busnumber;
                  }
               }
            }
            break;         /* stop scanning the bus once host bridge is found */
         }
      }
   }
   /*
      If the system has a Mach 1 chip, the host-to-bridge chip would
      response for any scanning with device #0 and end up with 256
      PCI buses.  In this case, we would default to 1 PCI bus if no
      pci-to-pci bridge was ever detected, or the highest subordinate
      bus plus one if any pci-to-pci bridge was ever detected.
   */
   return((nofbus == 256) ? high_subordinate_busnumber + 1 : nofbus);
}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*   PH_FindMechanism routine -
*
*   This routine will interrogate the system configuaration space
*   to determine which one of two access mechanisms is supported.
*
*  Return Value:  PCI_MECHANISM1(0x01) - system supports mechanism #1
*                 PCI_MECHANISM2(0x02) - system supports mechanism #2
*
*  Parameters:    none
*
*  Activation:    ASPI layer, initialization.
*
*  Remarks:
*
*********************************************************************/
#ifndef  _LESS_CODE
#endif
#ifndef _NO_CONFIG
UBYTE PH_FindMechanism ()
{
   UWORD next_busnumber;

   for (next_busnumber = 0; next_busnumber <= 255; ++next_busnumber)
   {
      if (Ph_AccessConfig((UBYTE)next_busnumber) == PCI_MECHANISM1)
         return((UBYTE)PCI_MECHANISM1);
      else
         continue;         /* scan the next bus */
   }
   return((UBYTE)PCI_MECHANISM2);
}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*   PH_FindHA routine -
*
*   This routine will interrogate the hardware (if any) at the
*   specified port address to see if a supported host adapter is
*   present.
*
*  Return Value:  0x00 - no AIC-787x h.a. found
*                 0x01 - AIC-787x h.a. found
*                 0x81 - AIC-787x h.a. found but not enabled
*
*  Parameters:    unsigned int bus_number
*                 unsigned int device number
*
*  Activation:    ASPI layer, initialization.
*
*  Remarks:
*
*********************************************************************/
#ifndef  _LESS_CODE
UWORD PH_FindHA (WORD bus_number,
                 WORD device_number)
{
   cfp_struct *NUL_PTR = (cfp_struct *) NOT_DEFINED;
   DWORD device_id;
   DWORD status_cmd;
   UBYTE num_of_ha = 0;

   device_id = Ph_ReadConfig(NUL_PTR,
                             (UBYTE)bus_number,
                             (UBYTE)device_number,
                             (UBYTE)ID_REG); /* Get id                     */
                                             /* Is ID AIC-78xx?            */
   if (  (   ((device_id & 0x00ffffff) == HA_ID_MASK)   /* 9004 78xx?  */
          || ((device_id & 0x00ffffff) == SAMURAI_ID))  /* 9004 75xx?  */
       && PHM_INCLUDESABRE(device_id)
       && PHM_INCLUDEATHENA(device_id))
   {
      status_cmd = Ph_ReadConfig(NUL_PTR,
                                 (UBYTE)bus_number,
                                 (UBYTE)device_number,
                                 (UBYTE)STATUS_CMD_REG); /* Get cmd reg    */
      if (!(status_cmd & ISPACEEN))         /* Check if device enabled    */
      {
         num_of_ha = NOT_ENABLED;            /* If not, set bit 7 on       */
      }
      ++num_of_ha;                           /* Bump number of HA          */

      if (!(status_cmd & MASTEREN))         /* Check if bus master enabled  */
      {
         status_cmd |= MASTEREN;
         Ph_WriteConfig(NUL_PTR,(UBYTE) bus_number,(UBYTE) device_number,
            (UBYTE) STATUS_CMD_REG,status_cmd);
      }
   }
   return(num_of_ha);
}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*   PH_GetConfig routine -
*
*   This routine initializes the members of the ha_Config and ha_struct
*   structures.
*
*  Return Value:  None
*                  
*  Parameters:    config_ptr
*              In:
*                 Bus_Number
*                 Device_Number
*                 HA_Data_Ptr (for driver only)
*
*              Out:
*                 ha_config structure will be initialized.
*
*  Activation:    ASPI layer, driver initialization
*                  
*  Remarks:                
*                  
*********************************************************************/
#ifndef  _LESS_CODE
void PH_GetConfig (cfp_struct *config_ptr)
{
   register AIC_7870 *base;
   UBYTE hcntrl_data, i, j;

#ifdef no_longer_needed
   config_ptr->CFP_BaseAddress = (Ph_ReadConfig(config_ptr,
                                                config_ptr->Cf_BusNumber,
                                                config_ptr->Cf_DeviceNumber,
                                                BASE_ADDR_REG) & 0xfffffffc);
#endif

   base = config_ptr->CFP_Base;

   config_ptr->CFP_AdapterIdH = INBYTE(AIC7870[DEVID0]);
   config_ptr->CFP_AdapterIdL = INBYTE(AIC7870[DEVID1]);

   config_ptr->Cf_ReleaseLevel = REL_LEVEL;  /* Current Release Level    */

   config_ptr->Cf_RevisionLevel =           /* Get device revision level */
               (UBYTE) Ph_ReadConfig(config_ptr,
                                     config_ptr->Cf_BusNumber,
                                     config_ptr->Cf_DeviceNumber,DEV_REV_ID);

   if (!((hcntrl_data = INBYTE(AIC7870[HCNTRL])) & CHIPRESET))
   {
      Ph_Pause(base, config_ptr);
   }
   else
   {
      /* Set InitNeeded will either use default or information */
      /* SEEPROM */
      config_ptr->CFP_InitNeeded = 1;
   }

   PHM_GETCONFIG(config_ptr);

   config_ptr->Cf_IrqChannel=(UBYTE)Ph_ReadConfig(config_ptr,
                                                  config_ptr->Cf_BusNumber,
                                                  config_ptr->Cf_DeviceNumber,
                                                  INTR_LEVEL_REG);

   config_ptr->Cf_BusRelease = INBYTE(AIC7870[LATTIME]) & 0xfc;

   config_ptr->Cf_Threshold = (DFTHRSH1 + DFTHRSH0) >> 6;
   config_ptr->Cf_ScsiId = 7;
   config_ptr->CFP_ConfigFlags |= SCSI_PARITY;

   if (config_ptr->CFP_AdapterIdL == 0x74)      /* if differential board */
      config_ptr->CFP_ConfigFlags |= DIFF_SCSI; /* mark it here for OSM */

   if (INBYTE(AIC7870[SBLKCTL]) & SELWIDE)
   {
      config_ptr->Cf_MaxTargets = 16;
      j = SYNC_MODE + WIDE_MODE;
      config_ptr->CFP_AllowDscnt = 0xffff;
   }
   else
   {
      config_ptr->Cf_MaxTargets = 8;
      j = SYNC_MODE;
      config_ptr->CFP_AllowDscnt = 0x00ff;
   }

   Ph_MemorySet(config_ptr->Cf_ScsiOption,j,config_ptr->Cf_MaxTargets);

   config_ptr->CFP_TerminationLow = config_ptr->CFP_TerminationHigh = 1;

#ifdef _SCAM
   config_ptr->CFP_ScamEnable = 0;
#endif   /* of _SCAM */

#ifdef _TARGET_MODE
   CFP_TargetMode = 1; /* Target Mode code is in place  */
#endif   /* of _TARGET_MODE */

   if ((Ph_ReadEeprom(config_ptr, base)) && !config_ptr->CFP_InitNeeded)
   {
      config_ptr->Cf_Threshold = INBYTE(AIC7870[PCISTATUS]) >> 6;
      config_ptr->Cf_ScsiId = (INBYTE(AIC7870[SCSIID]) & OID);
      config_ptr->CFP_ConfigFlags |= INBYTE(AIC7870[SXFRCTL1]) & (STIMESEL + ENSPCHK);

      for (i = 0; i < config_ptr->Cf_MaxTargets; i++)
      {
         if ((j = INBYTE(AIC7870[XFER_OPTION + i])) != NEEDNEGO)
         {
            config_ptr->Cf_ScsiOption[i] = j & (WIDEXFER + SXFR);
            if (j & SOFS)
            {
               config_ptr->Cf_ScsiOption[i] |= SYNC_MODE;
            }
         }
      }
      config_ptr->CFP_AllowDscntL = ~INBYTE(AIC7870[DISCON_OPTION]);
      if (config_ptr->Cf_MaxTargets == 16)
      {
         config_ptr->CFP_AllowDscntH = ~INBYTE(AIC7870[DISCON_OPTION + 1]);
      }
   }

   /* The cable sensing autotermination scheme will be performed here */
   /* and config_ptr structure termination setting updated.           */
   if (!(config_ptr->CFP_AutoTermCurrent))
   {
      if (config_ptr->CFP_AutoTermCable)
         Ph_AutoTermCable(config_ptr);
   }

#ifdef _AUTOTERMINATION          /* for 2930-like current sensing scheme */
   if (!(config_ptr->CFP_AutoTermCable))
   {
      if (config_ptr->CFP_AutoTermCurrent)
      {
         ;                       /* OSM will initiate the current sensing
                                 /*  autotermination operation */
      }
   }
#endif

   /* fast20 further qualified with EXTVALID - external resister setting */
   config_ptr->CFP_EnableFast20 =  (Ph_ReadConfig(config_ptr,
                                                  config_ptr->Cf_BusNumber,
                                                  config_ptr->Cf_DeviceNumber,
                                                  DEVCONFIG) & REXTVALID) 
                                 ? (config_ptr->CFP_EnableFast20)
                                 : 0;

   if (!(hcntrl_data & CHIPRESET))
   {
      Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data), config_ptr);
   }

   /* since we don't have a easy way to tell the warm  */
   /* boot from reloading HIM we are going to always   */
   /* set CFP_InitNeeded for right now                 */
   config_ptr->CFP_InitNeeded = 1;
}
#endif         /* _LESS_CODE  */

/*********************************************************************
*
*   PH_InitHA routine -
*
*   This routine initializes the host adapter.
*
*  Return Value:  0x00      - Initialization successful
*                 <nonzero> - Initialization failed
*                  
*  Parameters:    config_ptr
*                 h.a. config structure will be filled in
*                 upon initialization.
*
*  Activation:    Aspi layer, initialization.
*                  
*  Remarks:                
*
*********************************************************************/

UWORD PH_InitHA (cfp_struct *config_ptr)
{
   register AIC_7870 *base;

   UWORD i;

   base = config_ptr->CFP_Base;

   Ph_WriteHcntrl(base, (UBYTE) (PAUSE), config_ptr);

   PHM_INITHA(config_ptr);

   if (config_ptr->CFP_InitNeeded)
      if (i = Ph_LoadSequencer(config_ptr))
         return(i);

   if (config_ptr->CFP_InitNeeded)
   {
      OUTBYTE(AIC7870[SCSIID], config_ptr->Cf_ScsiId);
      if (config_ptr->CFP_ResetBus)       /* Reset bus if told to */
      {
         Ph_ResetSCSI(base, config_ptr);
         Ph_CheckSyncNego(config_ptr);    /* Adjust the sync nego parameters   */
      }
      else
         config_ptr->CFP_SuppressNego = 0;

      Ph_ResetChannel(config_ptr);

      i = INBYTE(AIC7870[SBLKCTL]);                      /* Turn off led   */
      OUTBYTE(AIC7870[SBLKCTL], i & ~(DIAGLEDEN + DIAGLEDON));
   }
   OUTBYTE(AIC7870[SEQCTL], FAILDIS + FASTMODE + SEQRESET);
   OUTBYTE(AIC7870[SEQADDR0], IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(AIC7870[SEQADDR1], 00);        /* Entry points always low page  */
   OUTBYTE(AIC7870[BRKADDR1], BRKDIS);

   OUTBYTE(AIC7870[PCISTATUS], config_ptr->Cf_Threshold << 6);

   OUTBYTE(AIC7870[SXFRCTL0], DFON);   /* Turn on digital filter 1/7/94 */

   /* high byte termination disable/enable only if it's wide bus */
   if (config_ptr->Cf_MaxTargets == 16)
   {
      OUTBYTE(AIC7870[SEEPROM],SEEMS);       /* process high byte termination */
      while(INBYTE(AIC7870[SEEPROM]) & EXTARBACK)
         ;
      OUTBYTE(AIC7870[SEEPROM],SEEMS|SEECS);
   
      if (config_ptr->CFP_TerminationHigh)          
      {                                      /* enable high byte termination */
         OUTBYTE(AIC7870[BRDCTL],BRDDAT6|BRDSTB|BRDCS); 
         OUTBYTE(AIC7870[BRDCTL],BRDDAT6|BRDCS);        
      }
      else
      {
         OUTBYTE(AIC7870[BRDCTL],BRDSTB|BRDCS);     /* disable termination */
         OUTBYTE(AIC7870[BRDCTL],BRDCS);               
      }
      OUTBYTE(AIC7870[BRDCTL],0);
      OUTBYTE(AIC7870[SEEPROM],0);
   }

   /* init dev map unconditionally */  
   {
      OUTBYTE(AIC7870[FAST20_LOW],0);
      OUTBYTE(AIC7870[FAST20_HIGH],0);
   }

   OUTBYTE(AIC7870[DISCON_OPTION], ~config_ptr->CFP_AllowDscntL);
   OUTBYTE(AIC7870[DISCON_OPTION + 1], ~config_ptr->CFP_AllowDscntH);

#ifdef _TARGET_MODE
   if (CFP_TargetMode)
      OUTBYTE(AIC7870[SCSISEQ],(INBYTE(AIC7870[SCSISEQ])|ENSELI)); /* Enbl SelIn*/
#endif   /* of _TARGET_MODE   */

   Ph_WriteHcntrl(base, (UBYTE) (INTEN), config_ptr);

   return(0);
}

/*********************************************************************
*
*   Ph_ReadConfig routine -
*
*   This routine will interrogate the hardware's configuration space
*   using PCI mechanism #1. If value from configuration space are all
*   FF's, then the routine will interrogate the hardware's configuration
*   space using PCI mechanism #2.
*
*  Return Value:  32-bit configuration data from specified register
*
*  Parameters:    unsigned char bus_number
*                 unsigned char device number
*                 unsigned char register number
*
*  Activation:    PH_FindHA, PH_GetConfig
*
*  Remarks:
*
*********************************************************************/
#ifndef  _LESS_CODE
DWORD Ph_ReadConfig (cfp_struct *config_ptr,
                     UBYTE bus_number,
                     UBYTE device_number,
                     UBYTE register_number)
{
   DWORD config_data_value;
   UBYTE access_mechanism;

   union
   {
      DWORD config_address_value;
      struct
      {
         DWORD reg:8;
         DWORD function:3;
         DWORD device:5;
         DWORD bus:8;
         DWORD rsvd:7;
         DWORD enable:1;
      } caf_struct;
   } caf;
   union
   {
      UWORD config_data_area;
      struct
      {
         UWORD reg:8;
         UWORD device:4;
         UWORD config_space:4;
      } cda_struct;
   } cda;

   /* try with OSMs version of read configuration space first.  If OSM  */
   /* is not able to access on its own.  HIM will first determine the   */
   /* access mechanism that is allowed by the system before actually    */
   /* start accessing the config space.                                 */
   config_data_value = PH_ReadConfigOSM(config_ptr,
                                        bus_number,
                                        device_number,
                                        register_number);

   if ((config_data_value & 0xffffff00) == (NO_CONFIG_OSM & 0xffffff00))
      access_mechanism = (UBYTE)(config_data_value & 0x000000ff);
   else
      return(config_data_value);

#ifndef _NO_CONFIG

   if (   (access_mechanism != PCI_MECHANISM1)  /* OSM does not know config */
       && (access_mechanism != PCI_MECHANISM2)) /*   space access mechanism */
      access_mechanism = Ph_AccessConfig(bus_number);

   switch (access_mechanism)
   {
      case  PCI_MECHANISM1:         /* CONFIG SPACE ACCESS MECHANISM #1 */
         /*
            Build config_address_value:

               31        24 23        16 15      11 10  8 7        0 
               ------------------------------------------------------
               |1| 0000000 | bus_number | device # | 000 | register |
               ------------------------------------------------------
         */
         caf.caf_struct.reg = register_number;
         caf.caf_struct.function = 0;
         caf.caf_struct.device = device_number;
         caf.caf_struct.bus = bus_number;
         caf.caf_struct.rsvd = 0;
         caf.caf_struct.enable = 1;

         OUTDWORD(CONFIG_ADDRESS, caf.config_address_value); /* Enable config cycle  */
         config_data_value = INDWORD(CONFIG_DATA);           /* Read config data     */
         caf.config_address_value = 0;                       /* Disable config cycle */
         OUTDWORD(CONFIG_ADDRESS, caf.config_address_value);
         break;

      case  PCI_MECHANISM2:       /* CONFIG SPACE ACCESS MECHANISM #2   */
         if (device_number < 0x10)
         {
            cda.cda_struct.config_space = 0x0c;
            cda.cda_struct.device = device_number;
            cda.cda_struct.reg = register_number;

            OUTBYTE(FORWARD_REG, bus_number);
            OUTBYTE(CONFIG_ADDRESS, 0x60);                     /* Enable config cycle  */
            config_data_value = INDWORD(cda.config_data_area); /* Read config data     */
            OUTBYTE(FORWARD_REG, 00);
            OUTBYTE(CONFIG_ADDRESS, 00);                       /* Disable config cycle */
         }
         else
            config_data_value = 0xffffffff;
         break;
   }
   return(config_data_value);
#endif /* _NO_CONFIG */
}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*   Ph_WriteConfig routine -
*
*   This routine will write to the hardware's configuration space
*   using PCI mechanism #1. If value from configuration space are all
*   FF's, then the routine will interrogate the hardware's configuration
*   space using PCI mechanism #2.
*
*  Return Value:  0 - Write successful
*                 -1 - Write failed
*
*  Parameters:    unsigned char bus_number
*                 unsigned char device number
*                 unsigned char register number
*                 32-bit configuration data to specified register
*
*  Activation:    PH_FindHA, PH_GetConfig
*
*  Remarks:
*
*********************************************************************/
#ifndef  _LESS_CODE
int Ph_WriteConfig (cfp_struct *config_ptr,
                     UBYTE bus_number,
                     UBYTE device_number,
                     UBYTE register_number,
                     DWORD config_data_value)
{
   int   ret_code;
   UBYTE access_mechanism;
   DWORD cdv;

   union
   {
      DWORD config_address_value;
      struct
      {
         DWORD reg:8;
         DWORD function:3;
         DWORD device:5;
         DWORD bus:8;
         DWORD rsvd:7;
         DWORD enable:1;
      } caf_struct;
   } caf;
   union
   {
      UWORD config_data_area;
      struct
      {
         UWORD reg:8;
         UWORD device:4;
         UWORD config_space:4;
      } cda_struct;
   } cda;

   ret_code = 0;                 /* initialize into good return code */

   /* try with OSMs version of write configuration space first.  If OSM */
   /* is not able to access on its own.  HIM will first determine the   */
   /* access mechanism that is allowed by the system before actually    */
   /* start accessing the config space.                                 */
   cdv = PH_WriteConfigOSM(config_ptr,
                           bus_number,
                           device_number,
                           register_number,
                           config_data_value);

   if ((cdv & 0xffffff00) == (NO_CONFIG_OSM & 0xffffff00))
      access_mechanism = (UBYTE)(cdv & 0x000000ff);
   else
      return(ret_code);

#ifndef _NO_CONFIG

   if (   (access_mechanism != PCI_MECHANISM1)  /* OSM does not know config */
       && (access_mechanism != PCI_MECHANISM2)) /*   space access mechanism */
      access_mechanism = Ph_AccessConfig(bus_number);

   switch (access_mechanism)
   {
      case  PCI_MECHANISM1:         /* CONFIG SPACE ACCESS MECHANISM #1 */
         /*
            Build config_address_value:

               31        24 23        16 15      11 10  8 7        0 
               ------------------------------------------------------
               |1| 0000000 | bus_number | device # | 000 | register |
               ------------------------------------------------------
         */
         caf.caf_struct.reg = register_number;
         caf.caf_struct.function = 0;
         caf.caf_struct.device = device_number;
         caf.caf_struct.bus = bus_number;
         caf.caf_struct.rsvd = 0;
         caf.caf_struct.enable = 1;

         OUTDWORD(CONFIG_ADDRESS, caf.config_address_value); /* Enable config cycle  */
         cdv = INDWORD(CONFIG_DATA);                         /* Read config data     */

         if (cdv != 0xffffffff)                              /* If read is good then */ 
         {                                                   /* we can write with    */
            OUTDWORD(CONFIG_DATA,config_data_value);         /* mech #1              */
         }
         else
            ret_code = -1;                         /* failed to write config space  */

         caf.config_address_value = 0;                       /* Disable config cycle */
         OUTDWORD(CONFIG_ADDRESS, caf.config_address_value);
         break;                        /* end of mechanism #1  */
         
      case  PCI_MECHANISM2:       /* CONFIG SPACE ACCESS MECHANISM #2   */
         if (device_number < 0x10)
         {
            cda.cda_struct.config_space = 0x0c;
            cda.cda_struct.device = device_number;
            cda.cda_struct.reg = register_number;

            OUTBYTE(FORWARD_REG, bus_number);
            OUTBYTE(CONFIG_ADDRESS, 0x60);                     /* Enable config cycle  */
            OUTDWORD(cda.config_data_area,config_data_value);  /* Write config data    */
            OUTBYTE(FORWARD_REG, 00);
            OUTBYTE(CONFIG_ADDRESS, 00);                       /* Disable config cycle */
         }
      else
         ret_code = -1;

      break;
   }

   return(ret_code);
#endif /* _NO_CONFIG */
}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*  Ph_AccessConfig  -
*
*  This routine will determine the access mechanism for the PCI config
*  space.  Firstly, PCI bus #0 is scanned for any PCI device under
*  Mechanism #1.  If no device at all is found, the target PCI buses is
*  scanned for any PCI device, again under mechanism #1.  Otherwise,
*  access mechanism #1 is assumed. Access mechanism #2 is assumed when
*  no valid PCI devices are found at the target PCI bus.
*
*  Return Value:  PCI_MECHANISM1 if ID Reg is readable with mechanism #1
*                 PCI_MECHANISM2 if ID Reg is not readable with mechanism #1
*
*  Parameters:    PCI bus_number
*
*  Activation:    Ph_ReadConfig
*
*  Remarks:
*
*********************************************************************/
#ifndef _NO_CONFIG
UBYTE Ph_AccessConfig (UBYTE bus_number)
{
   register UBYTE device_number;
   register UBYTE next_bus;
   DWORD class_code;
   union
   {
      DWORD config_address_value;
      struct
      {
         DWORD reg:8;
         DWORD function:3;
         DWORD device:5;
         DWORD bus:8;
         DWORD rsvd:7;
         DWORD enable:1;
      } caf_struct;
   } caf;

   next_bus = 0;              /* always start with PCI bus #0 */
   while (1)
   {
      for (device_number=0;
           device_number<32;
           device_number++)
      {
         caf.caf_struct.reg = (UBYTE)DEV_REV_ID;
         caf.caf_struct.function = 0;
         caf.caf_struct.device = device_number;
         caf.caf_struct.bus = next_bus;
         caf.caf_struct.rsvd = 0;
         caf.caf_struct.enable = 1;

         OUTDWORD(CONFIG_ADDRESS, caf.config_address_value); /* Enable config cycle  */
         class_code = INDWORD(CONFIG_DATA);                  /* Read config data     */
         caf.config_address_value = 0;                       /* Disable config cycle */
         OUTDWORD(CONFIG_ADDRESS, caf.config_address_value);

         class_code &= 0xff000000;        /* isolate bass class info only */
         if (class_code != 0xff000000)    /* check for any device */
            break;                        /* found a pci device, use #1 */
      }                 /* end of for(device_number=0; etc. )  */

      if (class_code == 0xff000000)    /* no PCI device was found */
      {
         if (next_bus == bus_number)   /* completed both buses, or bus #0 */
            return(PCI_MECHANISM2);    /* and found no device, use #2 */
         else
            next_bus = bus_number;     /* scan the non-0 target bus */
      }
      else                             /* valid PCI device was found */
         return(PCI_MECHANISM1);       /* yes, use #1 */
   }
}
#endif /* _NO_CONFIG */

/*********************************************************************
*
*  Ph_AutoTermCable routine -
*  
*  Do the cable-sensing autotermination based upon whether the internal
*  and external cable is installed.  The termination setting in the
*  HA config_ptr and the setting in SEEPROM will be updated.  However,
*  the actual termination setting is left to PH_InitHA().
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*
*  Activation: PH_InitHA
*
*  Remarks:                
*                  
*********************************************************************/
#ifndef  _LESS_CODE                 /* drivers only, no BIOS */
void Ph_AutoTermCable (cfp_struct *config_ptr)
{
   register AIC_7870 *base;

   termination_struct term_stat;
   UBYTE cable_stat;

   base = config_ptr->CFP_Base;

   switch (config_ptr->CFP_AutoTermCable)    /* autoterm thru cable detection */
   {
      case 0:                          /* no software assist */
        Ph_NoAssistTerm(config_ptr);
        break;

      case 1:                    /* software assisted autotermination */
         /* get the installed cable configuration info */
         cable_stat = Ph_ReadCableStatus(config_ptr);
         /* need to change HA termination control according to cable info. */
         /* if eeprom setting is incorrect, eeprom would be adjusted for   */
         /* backward compatibility.                                        */
         if (INBYTE(AIC7870[SBLKCTL]) & SELWIDE)   /* wide HA */
         {
            /* determining the low byte termination control */
            if (   ( (cable_stat & CABLE_EXT) && !(cable_stat & CABLE_INT50) &&  (cable_stat & CABLE_INT68))
                || (!(cable_stat & CABLE_EXT) &&  (cable_stat & CABLE_INT50) &&  (cable_stat & CABLE_INT68))
                || ( (cable_stat & CABLE_EXT) &&  (cable_stat & CABLE_INT50) && !(cable_stat & CABLE_INT68)))
            {              /* need to turn OFF low termination */
               term_stat.AutoTermLow = 0;
            }
            else           /* need to turn ON low termination */
            {
               term_stat.AutoTermLow = 1;
            }
            /* determining the high byte termination control */
            if ( (cable_stat & CABLE_EXT) && !(cable_stat & CABLE_INT50) && (cable_stat & CABLE_INT68))
            {              /* need to turn OFF high termination */
               term_stat.AutoTermHigh = 0;
            }
            else           /* need to turn ON high termination */
            {
               term_stat.AutoTermHigh = 1;
            }
         }
         else                                   /* narrow HA */
         {
            if (  (cable_stat & CABLE_EXT) && (cable_stat & CABLE_INT50) )    /* both int and ext are connected */
            {
               term_stat.AutoTermLow = 0;
            }
            else
            {
               term_stat.AutoTermLow = 1;
            }
         }
         /* Update eeprom if the setting in eeprom is incorrect */
         if (   (term_stat.AutoTermHigh != config_ptr->CFP_TerminationHigh)
             || (term_stat.AutoTermLow != config_ptr->CFP_TerminationLow))
         {
            /* update config_ptr flags for PH_InitHA() */
            config_ptr->CFP_TerminationLow = term_stat.AutoTermLow;
            config_ptr->CFP_TerminationHigh = term_stat.AutoTermHigh;

            /* Update E2C word in eeprom if setting in eeprom incorrect */
            Ph_UpdateEeprom( config_ptr,
                             base,
                             (UWORD)(EE_CONTROL >> 1));
         }
         break;
   }        /* end of switch (config_ptr->CFP_AutoTermCable) */
}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*  Ph_NoAssistTerm routine -
*  
*  Update Termination for the Lance chip based upon the user specified
*  settings.
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*
*  Activation: PH_InitHA
*
*  Remarks:                
*                  
*********************************************************************/
#ifndef  _LESS_CODE
void Ph_NoAssistTerm (cfp_struct *config_ptr)
{
   register AIC_7870 *base;
   UBYTE register_value;

   base = config_ptr->CFP_Base;

   /* get resident value */
   register_value = INBYTE(AIC7870[SXFRCTL1]);

   /* select LOW for active state of STPWCTL output */
   switch (config_ptr->CFP_TerminationLow)
   {
      case 0:
         OUTBYTE(AIC7870[SXFRCTL1], (register_value & ~STPWEN));
         break;
      case 1:
         OUTBYTE(AIC7870[SXFRCTL1], (register_value | STPWEN));
         break;
   }

   /* high byte termination disable/enable only if it's wide bus */
   if (config_ptr->Cf_MaxTargets == 16)
   {
      OUTBYTE(AIC7870[SEEPROM],SEEMS);       /* process high byte termination */
      while(INBYTE(AIC7870[SEEPROM]) & EXTARBACK)
         ;
      OUTBYTE(AIC7870[SEEPROM],SEEMS|SEECS);

      switch (config_ptr->CFP_TerminationHigh)
      {
         case 0:                /* disable high byte termination */
            OUTBYTE(AIC7870[BRDCTL],BRDSTB|BRDCS);
            OUTBYTE(AIC7870[BRDCTL],BRDCS);               
            break;

         case 1:                /* enable high byte termination */
            OUTBYTE(AIC7870[BRDCTL],BRDDAT6|BRDSTB|BRDCS); 
            OUTBYTE(AIC7870[BRDCTL],BRDDAT6|BRDCS);        
            break;
      }
      OUTBYTE(AIC7870[BRDCTL],0);
      OUTBYTE(AIC7870[SEEPROM],0);
   }
}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*   Ph_RebuildEEControl -
*
*   This routine will rebuild the EE_CONTROL value to be updated into
*   SEEPROM register.
*
*  Return Value:  updated EE_CONTROL word
*
*  Parameters:    config_ptr pointer
*
*  Activation:    Ph_AutoTermCable
*
*  Remarks:
*
*********************************************************************/
#ifndef _LESS_CODE
UWORD Ph_RebuildEEControl (cfp_struct *config_ptr, UWORD e2value)
{

   e2value = (config_ptr->CFP_TerminationLow)
               ? e2value | E2C_TERM_LOW
               : e2value & ~E2C_TERM_LOW;
   e2value = (config_ptr->CFP_TerminationHigh)
               ? e2value | E2C_TERM_HIGH
               : e2value & ~E2C_TERM_HIGH;

   return(e2value);
}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*   Ph_ReadEeprom -
*
*   This routine will read the serial eeprom and modify the configuration
*   structure accordingly. 
*
*  Return Value:  1 if eeprom NOT present
*                 0 if eeprom present, configuration structure modified
*
*  Parameters:    ha_config pointer
*                 base address of AIC-7870
*
*  Activation:    PH_GetConfig
*
*  Remarks:
*
*********************************************************************/
#ifndef  _LESS_CODE
UWORD Ph_ReadEeprom (cfp_struct *config_ptr,
                     register AIC_7870 *base)

{
   UWORD chksum = 0;
   UWORD disc = 0;
   UWORD E2Value;
   UWORD reg_base;
   UWORD i;
   int eeprom_type;

   /* Make sure EEPROM interface availabel       */
   /* This is necessary because chip like Dagger */
   /* which does not have regsiters SEEPROM and  */
   /* BRDCTL defined may get hang                */
   OUTBYTE(AIC7870[SEEPROM],SEECS);
   if (!(INBYTE(AIC7870[SEEPROM]) & SEECS))
      return(1);

   /* Enable EEPROM access */
   OUTBYTE(AIC7870[SEEPROM], SEEMS);
   while ( ! (INBYTE(AIC7870[SEEPROM]) & SEERDY) );

   eeprom_type = EETYPE_C06C46;          /* Default to C06 and C46 */
   reg_base = 0;           /* Default always to the first 64 bytes */
   switch(config_ptr->CFP_AdapterId)
   {
      case VLIGHT_LANCE:
         if (config_ptr->Cf_DeviceNumber != 4)    /* For Channel B */
            reg_base = EEPROM_SIZE / 2;     /* use second 64 bytes */
         break;

      case VIKING_LANCE:
         eeprom_type = EETYPE_C56C66;        /* Set to C46 and C56 */
         switch(config_ptr->Cf_DeviceNumber)
         {
            case 1:
            case 4:
               /* base at the first section */
               break;

            case 2:
            case 8:
               /* base at the second section */
               reg_base = EEPROM_SIZE / 2;
               break;

            case 3:
            case 12:
               /* base at the third section */
               reg_base = (EEPROM_SIZE / 2) * 2;
               break; 
         }
         break;

      case VIKING_SABRE:
         break;
   }

   /* make sure checksum and maximum no of targets matched */
   for ( i = 0; i < (UWORD)(EEPROM_SIZE/2-1); i++ )
   {
      chksum += ( E2Value = Ph_ReadE2Register( (UWORD)(reg_base+i), base, eeprom_type, config_ptr ) );
      if ( i * 2 == EE_MAX_TARGET && 
         config_ptr->Cf_MaxTargets != (UBYTE) E2Value )
         break;
   }

   if ( i != EEPROM_SIZE/2-1 || chksum != Ph_ReadE2Register( (UWORD)(reg_base+i), base, eeprom_type, config_ptr ) )
   {
      OUTBYTE(AIC7870[SEEPROM], 0);
      return(1);
   }      

   /* process EEPROM information word by word */
   for ( i = 0; i < (UWORD)(EEPROM_SIZE/2-1); i++ )
   {
      E2Value = Ph_ReadE2Register( (UWORD)(reg_base + i), base, eeprom_type, config_ptr );
      if ( (UBYTE) i < config_ptr->Cf_MaxTargets )
      {
         /* set scsi option flag */
         config_ptr->Cf_ScsiOption[i] = 
            (UBYTE) ( ( ( E2Value & E2T_WIDE ) << 2 ) |
            ( (UBYTE)( E2Value & E2T_SYNCRATE ) << 4 ) |
            ( (UBYTE)( E2Value & E2T_SYNCMODE ) >> 3 ) );

         /* build disconnect flag */
         disc |= ( (UWORD)( E2Value & E2T_DISCONNECT ) >> 4 ) << i;
      }
      else
         if ( i < 16 )
            config_ptr->Cf_ScsiOption[i] = 0;
         else
            switch( i * 2 )
            {
            case EE_CONTROL:
               config_ptr->CFP_ScsiParity = (UWORD)( E2Value & E2C_PARITY ) >> 4;
/*             config_ptr->CFP_ResetBus = (UWORD)( E2Value & E2C_RESET ) >> 6;   */
               config_ptr->CFP_EnableFast20 = (UWORD)( E2Value & E2C_FAST20 ) >> 1;

               config_ptr->CFP_TerminationLow = ((E2Value & E2C_TERM_LOW) != 0);
               config_ptr->CFP_TerminationHigh = ((E2Value & E2C_TERM_HIGH) != 0);
               config_ptr->CFP_AutoTermCable = (E2Value & E2C_AUTOTERM_CABLE);

#ifdef _AUTOTERMINATION
               config_ptr->CFP_AutoTermCurrent = (E2Value & E2C_AUTOTERM_CURRENT) >> 7;
#endif
               break;

            case EE_ID:
               config_ptr->Cf_ScsiId = (UBYTE) ( E2Value & 0xf );
               config_ptr->Cf_BusRelease = (UBYTE) ( E2Value >> 8 );
               break;

#ifdef _SCAM
            case EE_BIOSFLAG:
                config_ptr->CFP_ScamEnable = (E2Value & E2B_SCAMENABLE) >> 8;
                break;
#endif
            }
   } 

   /* set disconnect flag */
   config_ptr->CFP_AllowDscnt = disc;

   /* Disable EEPROM access */
   OUTBYTE(AIC7870[SEEPROM], 0);
   return(0);
}
#endif   /* _LESS_CODE  */

/*********************************************************************
*
*   Ph_UpdateEeprom -
*
*   This routine will update the content of serial eeprom with the
*   supplied address and data.
*
*  Return Value:  1 if eeprom NOT present
*                 0 if eeprom present and entry modified
*
*  Parameters:    ha_config pointer
*                 base address of AIC-7870
*                 word address of the word to be modified
*
*  Activation:    PH_InitHA
*
*  Remarks:
*
*********************************************************************/
#ifndef  _LESS_CODE
UWORD Ph_UpdateEeprom (cfp_struct *config_ptr,
                      register AIC_7870 *base,
                      UWORD address)
{
   UWORD chksum;
   UWORD E2Content[EEPROM_SIZE/2];  /* holding area for eeprom content */
   UWORD reg_base, i;
   int eeprom_type;

   /* Make sure EEPROM interface availabel       */
   /* This is necessary because chip like Dagger */
   /* which does not have regsiters SEEPROM and  */
   /* BRDCTL defined may get hang                */
   OUTBYTE(AIC7870[SEEPROM],SEECS);
   if (!(INBYTE(AIC7870[SEEPROM]) & SEECS))
      return(1);

   eeprom_type = EETYPE_C06C46;          /* Default to C06 and C46 */
   reg_base = 0;           /* Default always to the first 64 bytes */

   switch(config_ptr->CFP_AdapterId)
   {
      case VLIGHT_LANCE:
         if (config_ptr->Cf_DeviceNumber != 4)    /* For Channel B */
            reg_base = EEPROM_SIZE / 2;     /* use second 64 bytes */
         break;

      case VIKING_LANCE:
         eeprom_type = EETYPE_C56C66;        /* Set to C46 and C56 */
         switch(config_ptr->Cf_DeviceNumber)
         {
            case 1:
            case 4:
               /* base at the first section */
               break;

            case 2:
            case 8:
               /* base at the second section */
               reg_base = EEPROM_SIZE / 2;
               break;

            case 3:
            case 12:
               /* base at the third section */
               reg_base = (EEPROM_SIZE / 2) * 2;
               break; 
         }
         break;

      case VIKING_SABRE:
         break;
   }

   /* Enable EEPROM access */
   OUTBYTE(AIC7870[SEEPROM], SEEMS);
   while ( ! (INBYTE(AIC7870[SEEPROM]) & SEERDY) )
      ;

   /* read the entire EEPROM information including the last checksum word */
   for (i = 0; i <= (EEPROM_SIZE/2-1); i++ )
   {
      E2Content[i] = Ph_ReadE2Register((UWORD)(reg_base+i), base, eeprom_type, config_ptr);
   }

   /* rebuild the 16-bit data value */
   E2Content[(UWORD)address] = Ph_RebuildEEControl(config_ptr,
                                                   E2Content[(UWORD)address]);

   /* calculate the checksum - adding every word except the last word */
   for (i = 0, chksum = 0; i < (EEPROM_SIZE/2-1); i++ )
   {
      chksum += E2Content[i];
   }

   E2Content[(EEPROM_SIZE/2)-1] = chksum;

   /* 1. Enable programming mode                */
   Ph_EnableEraseWriteEE(base, eeprom_type, config_ptr);

   /* 2. Erase/Write the new entry and the new checksum */
   Ph_WriteE2Register((UWORD)(reg_base + address),
                      base,
                      eeprom_type,
                      E2Content[address], config_ptr);

   Ph_WriteE2Register((UWORD)(reg_base + (EEPROM_SIZE/2-1)),
                      base,
                      eeprom_type,
                      E2Content[EEPROM_SIZE/2-1], config_ptr);

   /* 3. Disable programming access             */
   Ph_DisableEraseWriteEE(base, eeprom_type, config_ptr);

   /* Disable EEPROM access */
   OUTBYTE(AIC7870[SEEPROM], 0);
   return(0);
}
#endif      /* of ifdef  (!_LESS_CODE) */

/*********************************************************************
*
*   Ph_ReadE2Register -
*
*   Read from EEPROM register word
*
*  Return Value:  register value (word)
*
*  Parameters:    EEPROM register address
*                 base address of AIC-7870
*
*  Activation:    read_eeprom
*
*  Remarks:
*
*********************************************************************/
#ifndef  _LESS_CODE
UWORD Ph_ReadE2Register (UWORD address,
                         register AIC_7870 *base,
                         int eeprom_type,
			 cfp_struct *config_ptr)
{
   UWORD retval;
   int i;
   int count;

   /* send EEPROM register address with op code for read */
   if (eeprom_type == EETYPE_C06C46)
   {                                   /* Setup for NM93C06 or NM93C46 */
      address |= 0x0080;                       /* OR with read command */
      count = 8;                                   /* Setup loop count */
   }
   else
   {                                   /* Setup for NM93C56 or NM93C66 */
      address |= 0x0200;                       /* OR with read command */
      count = 10;                                  /* Setup loop count */
   }

   Ph_SendStartBitEE(base, config_ptr);         /* send the start bit */
   Ph_SendAddressEE(base, count, address, config_ptr);
   
   OUTBYTE( AIC7870[SEEPROM], SEEMS | SEECS );
   Ph_Wait2usec(SEEMS | SEECS, base, config_ptr);

   /* get value for the register */
   for ( i = 0, retval = 0; i < 16; i++ )
   {
      retval <<= 1;
      OUTBYTE( AIC7870[SEEPROM], SEEMS | SEECS | SEECK );
      Ph_Wait2usec(SEEMS | SEECS | SEECK, base, config_ptr);
      OUTBYTE( AIC7870[SEEPROM], SEEMS | SEECS );
      if ( INBYTE( AIC7870[SEEPROM] ) & SEEDI )
         retval |= 1;
      Ph_Wait2usec(SEEMS | SEECS, base, config_ptr);
   }

   OUTBYTE( AIC7870[SEEPROM], SEEMS );
   Ph_Wait2usec(SEEMS, base, config_ptr);
   return( retval );
}
#endif   /* _LESS_CODE  */

/*********************************************************************
*
*   Ph_WriteE2Register -
*
*   This routine performs the ERASE/WRITE operation to the SEEPROM.
*
*  Return Value:  1 if eeprom NOT present
*                 0 if eeprom present and entry modified
*
*  Parameters:    word address of register to be updated
*                 base address of AIC-7870
*                 type of SEEPROM device
*                 data to be updated
*
*  Activation:
*
*  Remarks:    The write operation needs to be preceded by an erase
*              operation.
*
*              The command for erase is:
*              NM93C06 or NM93C46: 11xxxxxxB(8-bit command)
*              NM93C56 or NM93C66: 11xxxxxxxxB(10-bit command)
*
*              The command for write is:
*              NM93C06 or NM93C46: 11xxxxxxB(8-bit command)
*              NM93C56 or NM93C66: 11xxxxxxxxB(10-bit command)
*
*********************************************************************/
#ifndef _LESS_CODE
UWORD Ph_WriteE2Register (UWORD address,
                          register AIC_7870 *base,
                          int eeprom_type,
                          UWORD data,
			  cfp_struct *config_ptr)
{
   UWORD command;
   int count;

   /* the ERASE operation */
   if (eeprom_type == EETYPE_C06C46)
   {                                   /* Setup for NM93C06 or NM93C46 */
      command = address | 0x00c0;      /* command for erase */
      count = 8;                                   /* Setup loop count */
   }
   else
   {                                   /* Setup for NM93C56 or NM93C66 */
      command = address | 0x0300;      /* command for erase */
      count = 10;                                  /* Setup loop count */
   }

   Ph_SendStartBitEE(base, config_ptr);         /* send the start bit */

   /* send EEPROM register address with op code for erase command */
   Ph_SendAddressEE(base, count, command, config_ptr);
   
   OUTBYTE( AIC7870[SEEPROM], SEEMS | SEECS );  /* drop clock */
   Ph_Wait2usec(SEEMS | SEECS, base, config_ptr);
   OUTBYTE( AIC7870[SEEPROM],  SEEMS);          /* drop CS */
   Ph_Wait2usec(SEEMS, base, config_ptr);
   OUTBYTE( AIC7870[SEEPROM], SEEMS | SEECS );  /* raise CS */
   Ph_Wait2usec(SEEMS | SEECS, base, config_ptr);

   /* wait for erase command completion */
   while (!(INBYTE(AIC7870[SEEPROM]) & SEEDI))
      ;

   OUTBYTE( AIC7870[SEEPROM],  SEEMS);          /* drop CS, ERASE done */
   Ph_Wait2usec(SEEMS, base, config_ptr);

   /* now the WRITE operation */
   if (eeprom_type == EETYPE_C06C46)
   {                                   /* Setup for NM93C06 or NM93C46 */
      command = address | 0x0040;      /* command for write */
      count = 8;                                   /* Setup loop count */
   }
   else
   {                                   /* Setup for NM93C56 or NM93C66 */
      command = address | 0x0100;      /* command for write */
      count = 10;                                  /* Setup loop count */
   }

   /* now the WRITE operation */
   Ph_SendStartBitEE(base, config_ptr);         /* send the start bit */
   
   /* send EEPROM register address with op code for write command */
   Ph_SendAddressEE(base, count, command, config_ptr);

   count = 16;             /* Setup loop count for data phase */

   /* send EEPROM register address with op code for write command */
   Ph_SendAddressEE(base, count, data, config_ptr);

   OUTBYTE( AIC7870[SEEPROM], SEEMS | SEECS);   /* drop the clock */
   Ph_Wait2usec(SEEMS | SEECS, base, config_ptr);
   OUTBYTE( AIC7870[SEEPROM], SEEMS);           /* drop CS */
   Ph_Wait2usec(SEEMS, base, config_ptr);
   OUTBYTE( AIC7870[SEEPROM], SEEMS | SEECS);   /* raise CS */
   Ph_Wait2usec(SEEMS | SEECS, base, config_ptr);

   /* wait for write command completion */
   while (!(INBYTE(AIC7870[SEEPROM]) & SEEDI))
      ;
   
   OUTBYTE( AIC7870[SEEPROM], SEEMS);           /* drop CS */
   Ph_Wait2usec(SEEMS, base, config_ptr);

   return(0);
}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*   Ph_EnableEraseWriteEE -
*
*   This routine enables the ERASE/WRITE access to the SEEPROM for
*   future erase/write operation.
*
*  Return Value:  1 if eeprom NOT present
*                 0 if eeprom present and entry modified
*
*  Parameters:    base address of AIC-7870
*                 type of SEEPROM device
*
*  Activation:
*
*  Remarks: The command for enable erase/write is:
*              NM93C06 or NM93C46: 0011xxxxB(8-bit command)
*              NM93C56 or NM93C66: 0011xxxxxxB(10-bit command)
*
*********************************************************************/
#ifndef _LESS_CODE
UWORD Ph_EnableEraseWriteEE (register AIC_7870 *base, int eeprom_type,
			  cfp_struct *config_ptr)
{
   UWORD address;
   int count;

   if (eeprom_type == EETYPE_C06C46)
   {                                   /* Setup for NM93C06 or NM93C46 */
      address = 0x0030;                /* command for enable erase/write */
      count = 8;                                   /* Setup loop count */
   }
   else
   {                                   /* Setup for NM93C56 or NM93C66 */
      address = 0x00c0;                /* command for enable erase/write */
      count = 10;                                  /* Setup loop count */
   }

   Ph_SendStartBitEE(base, config_ptr);         /* send the start bit */
   Ph_SendAddressEE(base, count, address, config_ptr);

   OUTBYTE( AIC7870[SEEPROM], SEEMS | SEECS );  /* turn OFF the clock */
   Ph_Wait2usec(SEEMS | SEECS, base, config_ptr);
   OUTBYTE( AIC7870[SEEPROM], SEEMS);           /* drop CS */
   Ph_Wait2usec(SEEMS, base, config_ptr);

   return(0);
}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*   Ph_DisableEraseWriteEE -
*
*   This routine disables the ERASE/WRITE access to the SEEPROM for
*   any future erase/write operation.
*
*  Return Value:  1 if eeprom NOT present
*                 0 if eeprom present and entry modified
*
*  Parameters:    base address of AIC-7870
*                 type of SEEPROM device
*
*  Activation:
*
*  Remarks: The command for disable erase/write is:
*              NM93C06 or NM93C46: 0000xxxxB(8-bit command)
*              NM93C56 or NM93C66: 0000xxxxxxB(10-bit command)
*
*********************************************************************/
#ifndef _LESS_CODE
UWORD Ph_DisableEraseWriteEE (register AIC_7870 *base, int eeprom_type,
			  cfp_struct *config_ptr)
{
   UWORD address;
   int count;

   if (eeprom_type == EETYPE_C06C46)
   {                                   /* Setup for NM93C06 or NM93C46 */
      address = 0x0000;                /* command for disable erase/write */
      count = 8;                       /* Setup loop count */
   }
   else
   {                                   /* Setup for NM93C56 or NM93C66 */
      address = 0x0000;                /* command for disable erase/write */
      count = 10;                      /* Setup loop count */
   }

   Ph_SendStartBitEE(base, config_ptr);         /* send the start bit */
   Ph_SendAddressEE(base, count, address, config_ptr);
   
   OUTBYTE( AIC7870[SEEPROM], SEEMS | SEECS );  /* turn OFF the clock */
   Ph_Wait2usec(SEEMS | SEECS, base, config_ptr);
   OUTBYTE( AIC7870[SEEPROM], SEEMS);           /* drop CS */
   Ph_Wait2usec(SEEMS, base, config_ptr);

   return(0);
}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*   Ph_SendStartBitEE -
*
*   This routine will send the start bit when issuing commands to the
*   EEPROM.
*
*  Return Value:  none
*
*  Parameters:    base address of AIC-7870
*                 type of SEEPROM device
*
*  Activation:
*
*  Remarks: The routine returns with SEEMS and SEECS asserted
*
*********************************************************************/
#ifndef _LESS_CODE
void Ph_SendStartBitEE (register AIC_7870 *base,
			  cfp_struct *config_ptr)
{
   UBYTE seeprom_value;

   /* start bit  of the SEEPROM command */
   OUTBYTE( AIC7870[SEEPROM], seeprom_value = SEEMS | SEECS | SEEDO );
   Ph_Wait2usec(seeprom_value, base, config_ptr);
   OUTBYTE( AIC7870[SEEPROM], seeprom_value |= SEECK );
   Ph_Wait2usec(seeprom_value, base, config_ptr);
   OUTBYTE( AIC7870[SEEPROM], seeprom_value &= ~ SEECK );
   Ph_Wait2usec(seeprom_value, base, config_ptr);
}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*   Ph_SendAddressEE -
*
*   This routine sends the command to SEEPROM
*
*  Return Value:  none
*
*  Parameters:    base address of AIC-7870
*                 no. of bit in the command
*                 register no. of command
*
*  Activation:
*
*  Remarks: The routine returns with SEEMS and SEECS asserted
*
*********************************************************************/
#ifndef _LESS_CODE
void Ph_SendAddressEE (register AIC_7870 *base, int count, UWORD address,
			  cfp_struct *config_ptr)
{
   UBYTE seeprom_value;
   UWORD mask;
   int   bit;

   switch (count)
   {
      case 8:  mask = 0x0080;
               break;
      case 10: mask = 0x0200;
               break;
      case 16: mask = 0x8000;
               break;
   }

   /* send EEPROM register address with op code */
   for ( bit = 0; bit < count; bit++ )
   {
      seeprom_value = SEEMS | SEECS | ( ( address & mask ) ? SEEDO : 0 );
      OUTBYTE( AIC7870[SEEPROM], seeprom_value );
      Ph_Wait2usec(seeprom_value, base, config_ptr);
      OUTBYTE( AIC7870[SEEPROM], seeprom_value |= SEECK );
      Ph_Wait2usec(seeprom_value, base, config_ptr);
      address <<= 1;
   }

}
#endif      /* _LESS_CODE  */

/*********************************************************************
*
*   Ph_Wait2usec -
*
*   This routine will use Lance hardware supported timer to wait for 
*   2.4 usec before return.
*
*  Return Value:  none
*
*  Parameters:    SEEPROM register value for timer reset
*                 base address of AIC-7870
*
*  Activation:    Ph_ReadE2Register
*
*  Remarks:       timer has been reset before calling this routine
*
*********************************************************************/
#ifndef  _LESS_CODE
void Ph_Wait2usec (UBYTE seeprom_value,
                   register AIC_7870 *base,
		   cfp_struct *config_ptr)
{
int i;

   /* wait for 3 * 800 nsec = 2.4 usec */
   for ( i = 0; i < 3; i++ )
   {
      /* wait for 800 nsec */
      while ( ! ( INBYTE( AIC7870[SEEPROM] ) & SEERDY ) );

      /* reset timer */
      OUTBYTE( AIC7870[SEEPROM], seeprom_value );
   }     
}
#endif   /* _LESS_CODE  */

/*********************************************************************
*
*   Ph_LoadSequencer -
*
*   This routine will down load sequencer code into AIC-7870 host
*   adapter
*
*  Return Value:  0 - good
*                 Others - error
*
*  Parameters:    seqptr - pointer to sequencer code
*                 seqsize - sequencer code size
*
*  Activation:    PH_InitHA
*
*  Remarks:       timer has been reset before calling this routine
*
*********************************************************************/

int Ph_LoadSequencer (cfp_struct *config_ptr)
{
   UBYTE *seq_code;
   UWORD seq_size;
   int index,cnt;
   UBYTE i;
   register AIC_7870 *base;

   base = config_ptr->CFP_Base;
   /* depending on mode we are going to load different sequqncer */
   if (config_ptr->Cf_AccessMode == 2)
   {
      index = 1;                    /* Load optima mode sequencer          */
#if !defined( _EX_SEQ01_ )
      seq_code = Seq_01;
#endif /* !_EX_SEQ01 */
   }
   else
   {
      index = 0;                    /* Load standard mode sequencer        */
#if !defined( _EX_SEQ00_ )
      seq_code = Seq_00;
#endif /* !_EX_SEQ00 */
   }

   /* if the sequencer code size is zero then we should return */
   /* with error immediately (no sequencer code available) */
   if ( ! ( seq_size = SeqExist[index].seq_size ) )
      return( -2 );
   
   /* patch sequencer */
   i = TARGET_ID;
   if (config_ptr->CFP_MultiTaskLun)
   {
      i |= LUN;
   }
             
   /* PAUL - if TARG_LUN_MASK0 = 0, skip patch */
   if (SeqExist[index].seq_trglunmsk0 != 0)
   {
      seq_code[SeqExist[index].seq_trglunmsk0] = i;
   }


   OUTBYTE(AIC7870[SEQCTL], FASTMODE + PERRORDIS + LOADRAM);
   OUTBYTE(AIC7870[SEQADDR0], 00);
   OUTBYTE(AIC7870[SEQADDR1], 00);
   for (cnt = 0; cnt < (int) seq_size; cnt++)
   {
      OUTBYTE(AIC7870[SEQRAM], seq_code[cnt]);
   }
   OUTBYTE(AIC7870[SEQCTL], FASTMODE + PERRORDIS);
   OUTBYTE(AIC7870[SEQCTL], FASTMODE + PERRORDIS + LOADRAM);
   OUTBYTE(AIC7870[SEQADDR0], 00);
   OUTBYTE(AIC7870[SEQADDR1], 00);
   for (cnt = 0; cnt < (int) seq_size; cnt++)
   {
      if (INBYTE(AIC7870[SEQRAM]) != seq_code[cnt])
      {
        return(ADP_ERR);
      }
   }
   OUTBYTE(AIC7870[SEQCTL], FASTMODE + PERRORDIS);

   return(0);
}

/*********************************************************************
*
*  Ph_ReadCableStatus routine -
*  
*  Read the internal/external cable installation status
*
*  Return Value:  cable_stat
*                  
*  Parameters: config_ptr
*
*  Activation: PH_InitHA
*
*  Remarks:                
*                  
*********************************************************************/
#ifndef  _LESS_CODE
UBYTE Ph_ReadCableStatus (cfp_struct *config_ptr)
{
   register AIC_7870 *base;
   UBYTE cable_stat, regval_low, regval_high, index;

   base = config_ptr->CFP_Base;
   cable_stat = 0;                        /* initialize */

   OUTBYTE(AIC7870[SEEPROM],SEEMS);       /* process high byte termination */
   while(INBYTE(AIC7870[SEEPROM]) & EXTARBACK)
      ;

   switch (config_ptr->CFP_AdapterId & 0xfff0)
   {
      case LANCE_BASED:
      case KATANA_BASED:
         
         OUTBYTE(AIC7870[SEEPROM],SEEMS|SEECS);

         /* LANCE/KATANA based - bit 7: internal 68-pin connector status */
         /*                      bit 6: internal 50-pin connector        */
         OUTBYTE(AIC7870[BRDCTL],BRDSTB|BRDCS); /* write 0 to bit 5 */
         OUTBYTE(AIC7870[BRDCTL],BRDCS);
         OUTBYTE(AIC7870[BRDCTL],0);            /* end of 1st write */

         OUTBYTE(AIC7870[BRDCTL],BRDRW|BRDCS);  /* read BRDCTL */
         regval_low = INBYTE(AIC7870[BRDCTL]);
         OUTBYTE(AIC7870[BRDCTL],0);

         /* LANCE/KATANA based - bit 7: flash eeprom */
         /*                      bit 6: external connector */
         OUTBYTE(AIC7870[BRDCTL],BRDDAT5|BRDSTB|BRDCS); /* write 1 to bit 5 */
         OUTBYTE(AIC7870[BRDCTL],BRDDAT5|BRDCS);
         OUTBYTE(AIC7870[BRDCTL],BRDDAT5);      /* end of 2nd write */

         OUTBYTE(AIC7870[BRDCTL],BRDRW|BRDCS);  /* read BRDCTL */
         regval_high = INBYTE(AIC7870[BRDCTL]);
         OUTBYTE(AIC7870[BRDCTL],0);
         OUTBYTE(AIC7870[SEEPROM],0);

         if (INBYTE(AIC7870[SBLKCTL]) & SELWIDE)   /* for 2940 UW */
         {
            if (!(regval_low & 0x80))    /* a zero indicates cable installed */
               cable_stat |= CABLE_INT68;

            if (!(regval_low & 0x40))
               cable_stat |= CABLE_INT50;

            if (!(regval_high & 0x40))
               cable_stat |= CABLE_EXT;
         }
         else                 /* for 2940U */
         {
            if (!(regval_low & 0x80))    /* a zero indicates cable installed */
               cable_stat |= CABLE_EXT;

            if (!(regval_low & 0x40))
               cable_stat |= CABLE_INT50;
         }

         break;

   /* DAGGER plus based -  bit 7: external connector           */
   /*                      bit 6: internal 50-pin connector    */
      case DAGGER_BASED:
      case DAGGER2_BASED:
         regval_low = INBYTE(AIC7870[SPIOCAP]);   /* Disable soft commands */
         regval_low &= ~SOFTCMDEN;
         regval_low |= EXT_BRDCTL;                 /* enable external board port */

         /* write it out */
         OUTBYTE(AIC7870[SPIOCAP],regval_low);

         /* enable board r/w and chip select */
         regval_low = INBYTE(AIC7870[BRDCTL]);
         regval_low |= BRDRW | BRDCS;

         for (index=0; index<0x10; ++index)  /* software delay */
            ;

         OUTBYTE(AIC7870[BRDCTL],regval_low);

         regval_low = INBYTE(AIC7870[SEEPROM]);
         OUTBYTE(AIC7870[SEEPROM], regval_low | SEEMS);

         for (index=0; index<0x10; ++index)  /* software delay */
            ;

         OUTBYTE(AIC7870[SEEPROM], regval_low &= ~SEEMS);

         regval_low = INBYTE(AIC7870[BRDCTL]);

         if (!(regval_low & 0x20))       /* zero indicates cable installed */
            cable_stat |= CABLE_INT50;

         if (!(regval_low & 0x40))
            cable_stat |= CABLE_EXT;
         
         break;
   }

   return(cable_stat);
}
#endif      /* _LESS_CODE  */


/*********************************************************************
*
*  Ph_UpdateTerm routine -
*  
*  Update Termination for the Lance chip
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*
*  Activation: PH_CheckTermination
*
*  Remarks:                
*                  
*********************************************************************/
#ifdef _AUTOTERMINATION
void Ph_UpdateTerm (cfp_struct *config_ptr)
{
   register AIC_7870 *base;
   UBYTE register_value;

   base = config_ptr->CFP_Base;

   /* get resident value */
   register_value = INBYTE(AIC7870[SXFRCTL1]);

    /* select LOW for active state of STPWCTL output */
   if (!(config_ptr->CFP_TerminationLow))
      OUTBYTE(AIC7870[SXFRCTL1], (register_value & ~STPWEN));
   else
      OUTBYTE(AIC7870[SXFRCTL1], (register_value | STPWEN));
   
   /* reset scsi bus */ 
   Ph_ResetSCSI(base, config_ptr);
}
#endif         /* end of #ifdef _AUTOTERMINATION */

/*********************************************************************
*
*  PH_CheckTermination routine -
*
*  Check termination and try to fix termination if automatic term is enabled 
*
*  Return Value:  termination byte
*                  
*  Parameters: config_ptr
*
*  Activation: aspi8 layer
*                  
*  Remarks:                
*                  
*********************************************************************/
#ifdef _AUTOTERMINATION
UWORD PH_CheckTermination (cfp_struct *config_ptr)
{
   register AIC_7870 *base;
   UBYTE  i,j;   
   UBYTE hcntrl_data;
   UBYTE register_value;
   UWORD returned_value = 0;
   UWORD termination_flags;

   base = config_ptr->CFP_Base;
   hcntrl_data = INBYTE(AIC7870[HCNTRL]);    

   /* make sure power is not down, interrupts off, and sequencer paused */
   Ph_WriteHcntrl(base, (UBYTE)(hcntrl_data &  ~(POWRDN | INTEN) ), config_ptr);   
  
   /* Pause the sequencer */
   Ph_Pause(base, config_ptr);
   
   /* Reset the SCSI Bus */
   Ph_ResetSCSI(base, config_ptr);

   j = INBYTE(AIC7870[SPIOCAP]);   /* Disable soft commands */
   j &= ~SOFTCMDEN;
   j |= EXT_BRDCTL;                 /* enable external board port */

   /* write it out */
   OUTBYTE(AIC7870[SPIOCAP],j);
   
   /* enable board r/w and chip select */
   j = INBYTE(AIC7870[BRDCTL]);
   j = j | BRDRW | BRDCS;    
   OUTBYTE(AIC7870[BRDCTL],j);
   j = INBYTE(AIC7870[BRDCTL]);
   register_value = INBYTE(AIC7870[SEEPROM]);
   OUTBYTE(AIC7870[SEEPROM], register_value | SEEMS);
   
   Ph_Delay(base,1, config_ptr);       /* wait 8 usec */

   OUTBYTE(AIC7870[SEEPROM], register_value &= ~SEEMS);
   
   Ph_Delay(base,1, config_ptr);       /* wait 8 usec */
   
   /* read board value */
   register_value = INBYTE(AIC7870[BRDCTL]);
   i = register_value;   
   returned_value = (UWORD)register_value;
   returned_value <<= 8;
   returned_value |= j;
  
   if (!(config_ptr->CFP_AutoTermCurrent))   
      return(returned_value);
   else
   {
      termination_flags =  (UWORD)(config_ptr->CFP_TerminationLow)
                         | (UWORD)(config_ptr->CFP_TerminationHigh);

      i >>= 5;                      /* cable detection status */
      if (i == 3)                   /* no cable attached    */ 
         return(returned_value);

      if (i == 0)                   /* internal and external cable attached */
      {
         /* disable Host Adapter termination */
         config_ptr->CFP_TerminationLow = 0;

         /* update the termination for Lance */
         Ph_UpdateTerm(config_ptr);
      }
      else
      {
         j >>= 5;

         if ((j == 1) || (j == 2))
            return(returned_value);
         else
         if (j == 0)
         {
            /* less than 2 termination */
            /* try to fix termination automatically */
            config_ptr->CFP_TerminationLow = 1;

            /* update the termination for Lance */
            Ph_UpdateTerm(config_ptr);
         }
         else if (j == 3)
         {
            /* more than 2 termination */
            config_ptr->CFP_TerminationLow = 0;

            /* update the termination for Lance */
            Ph_UpdateTerm(config_ptr);
         }
      }
   }

   /* then read it again */
   j = INBYTE(AIC7870[BRDCTL]);
   j = j | BRDRW | BRDCS;    
   OUTBYTE(AIC7870[BRDCTL],j);
   j = INBYTE(AIC7870[BRDCTL]);
   register_value = INBYTE(AIC7870[SEEPROM]);
   OUTBYTE(AIC7870[SEEPROM], register_value | SEEMS);

   Ph_Delay(base,1, config_ptr);       /* wait 8 usec */

   OUTBYTE(AIC7870[SEEPROM], register_value &= ~SEEMS);
   
   Ph_Delay(base,1, config_ptr);       /* wait 8 usec */
   
   register_value = INBYTE(AIC7870[BRDCTL]);
   returned_value = (UWORD)register_value;
   returned_value <<= 8;
   returned_value |= j;

   return(returned_value);
}

#endif         /* end of #ifdef _AUTOTERMINATION */

#ifdef _SCAM
/*********************************************************************
*
*  Ph_ScamTransferInfoIn routine - Transfer info in from SCAM Targets
*                                  under TRANSFER CYCLE protocol of SCAM
*  
*  Scam transfer read scam info from the SCAM targets
*
*  Return Value:  data from the SCSI bus
*                  
*  Parameters: base, function code
*
*  Activation: Ph_AssignID
*                  
*********************************************************************/
UBYTE Ph_ScamTransferInfoIn (register AIC_7870 *base, UBYTE function_code)
{
   UBYTE register_value = 0;
   UBYTE i = 0;

   while (!(INBYTE(AIC7870[SCSIBUSL]) & DB7))
      ;                 /* wait for DB7 to be asserted */

   OUTBYTE(AIC7870[SCSIDATL], DB5);          /* assert DB5 */
   Ph_Wait4Release(SCSIBUSL, DB7, base);     /* wait for release of DB7 */

   /* wait till the bus is settled */
   for (i=0; i<16; i++)
      register_value = INBYTE(AIC7870[SCSIBUSL]) & 0x1f;

   OUTBYTE(AIC7870[SCSIDATL], DB6);  /* assert DB6 and drop DB5 */
   Ph_Wait4Release(SCSIBUSL, DB5, base);  /* wait for release of DB5 */

   OUTBYTE(AIC7870[SCSIDATL], DB7);       /* drop DB6 and keep DB7 */
   Ph_Wait4Release(SCSIBUSL, DB6 , base); /* Wait for DB6 to be released */

   return(register_value & 0x1f);
}

/*********************************************************************
*
*  Ph_ScamTransferInfoOut routine - Transfer info out to SCAM Targets
*                                   under TRANSFER CYCLE protocol of SCAM
*
*  Return Value:  none
*                  
*  Parameters: base, function code
*
*  Activation: Ph_AssignID
*
*********************************************************************/
void Ph_ScamTransferInfoOut (register AIC_7870 *base, UBYTE function_code)
{
   UBYTE register_value = 0;
   UBYTE i = 0;

   function_code &= 0x1f;

   function_code |= (DB5 | DB7);    /* assert DB5 and DB7 */
   OUTBYTE(AIC7870[SCSIDATL], function_code);

   function_code &= ~DB7;           /* drop DB7, keep DB5 */
   OUTBYTE(AIC7870[SCSIDATL], function_code);
   Ph_Wait4Release(SCSIBUSL, DB7, base);  /* wait for release of DB7 */

   register_value = INBYTE(AIC7870[SCSIBUSL]) & 0x1f;

   register_value |=  DB5 + DB6;        /* assert DB5 and DB6 */
   OUTBYTE(AIC7870[SCSIDATL], register_value);

   register_value &= ~DB5;                /* drop db5, keep DB6 */
   OUTBYTE(AIC7870[SCSIDATL], register_value);
   Ph_Wait4Release(SCSIBUSL, DB5, base);  /* wait for release of DB5 */

   register_value =  DB6 + DB7;           /* assert DB6 and DB7 */
   OUTBYTE(AIC7870[SCSIDATL], register_value);

   register_value &= ~DB6;                /* drop db6, keep DB7 */
   OUTBYTE(AIC7870[SCSIDATL], register_value);
   Ph_Wait4Release(SCSIBUSL, DB6, base);  /* wait for release of DB6 */

   return;
}

/*********************************************************************
*
*  Ph_BusDeviceResets routine -
*
*  Terminate the scsi command and thus, scsi request
*
*  Return Value:  None
*                  
*  Parameters: config_ptr - pointer to configuration structure 
*              base -       base address of AIC_7870
*             
*  Activation: Ph_NonInt
*              Ph_SelectTarget
*              
*  Remarks:                
*                  
*********************************************************************/
void Ph_BusDeviceResets (cfp_struct *config_ptr, register AIC_7870 *base)
{ 
   UBYTE register_value;
     
   base = config_ptr->CFP_Base;

   while(1)
   {
      /* read scsi signal */
      register_value = INBYTE(AIC7870[SCSISIG]);
      if ((register_value & MSGI) && (register_value & REQI))
         break;
   }

   OUTBYTE(AIC7870[CLRSINT1], CLRATNO);
   OUTBYTE(AIC7870[SCSIDATL],MSG0C);      /* issue bus device reset message */
   
   do
   {
      register_value = INBYTE(AIC7870[SCSISIG]);
   }  while (register_value & REQI);
   
   Ph_Delay(base, 1, config_ptr);
   
   OUTBYTE(AIC7870[SCSISEQ],(INBYTE(AIC7870[SCSISEQ]) &
             ~(TEMODEO+ENSELO+ENAUTOATNO+ENAUTOATNI)));     /* zero out scsi sequence register */
   OUTBYTE(AIC7870[SCSISIG],0);     /* zero out scsi signal register */

   register_value = INBYTE(AIC7870[SSTAT0]);
   if (register_value != (DMADONE && SDONE))
   {
      OUTBYTE(AIC7870[SCSISEQ], SCSIRSTO);         /* reset SCSI bus */
      Ph_Delay(base, 1, config_ptr);
      OUTBYTE(AIC7870[SCSISEQ], 0);                /* deassert reset line */
      Ph_Delay(base, 500, config_ptr);                         /* 250 ms delay */
   }
}

/*********************************************************************
*
*  Ph_SelectTarget routine -
*
*  Select Target for finding non-scam devices
*
*  Return Value:  0: if no target resides at target_id ID.
*                 1: if target resides at target_id ID.
*                          
*  Parameters: config_ptr
*              target_id
*             
*  Activation: Ph_FindNonScamDevs
*                  
*  Remarks:                
*                  
*********************************************************************/
UWORD Ph_SelectTarget (cfp_struct *config_ptr, UBYTE target_id)
{ 
   register AIC_7870 *base;

   UBYTE ret_value, register_value;
   UWORD index;
   
   base = config_ptr->CFP_Base;
   
   OUTBYTE(AIC7870[SIMODE0],0);
   OUTBYTE(AIC7870[SIMODE1],0);

   register_value = INBYTE(AIC7870[SXFRCTL0]);
   register_value |= CLRSTCNT + SPIOEN + CLRCHN;
   OUTBYTE(AIC7870[SXFRCTL0], register_value);  

   /* load target id */   
   target_id <<= 4;
   target_id &= 0xf0;
   
   /* and also the HA id */
   target_id |= config_ptr->Cf_ScsiId;
   OUTBYTE(AIC7870[SCSIID],target_id);
   
   register_value = INBYTE(AIC7870[SXFRCTL1]);
   register_value |= ENSTIMER;
   OUTBYTE(AIC7870[SXFRCTL1], register_value);  

   /* enable selection out and auto attention out */
   OUTBYTE(AIC7870[SCSISEQ],ENSELO+ENAUTOATNO);
   
   /* expect message out phase */
   OUTBYTE(AIC7870[SCSISIG], MOPHASE);
   
   /* wait for selecting timeout to occur.  SCAM tolerant device must */
   /* respond within 2ms.                                             */
   for (index=0; index<4; index++)  /* each delay unit is 500us */
   {
      register_value = INBYTE(AIC7870[SSTAT0]);
      if (register_value & SELDO)
         break;

      Ph_Delay(base, 1, config_ptr);            /* wait 500 usec */
   }

   if (!(register_value & SELDO))   /* No scam tolerant device at ID */
      ret_value = 0;
   else                             /* SCAM Tolerant device responded */
   {
      /* terminate scsi command */
      Ph_BusDeviceResets(config_ptr, base);
      ret_value = 1;
   }

   OUTBYTE(AIC7870[SCSISEQ],0);
   OUTBYTE(AIC7870[SCSISIG],0);

   /* wait for bus free */
   do
   {
      register_value = INBYTE(AIC7870[SSTAT1]);

      /* reset scsi bus if a phase mismatch occurs */
      if (register_value & PHASEMIS)
      {
         OUTBYTE(AIC7870[SCSISEQ], SCSIRSTO);         /* reset SCSI bus */
         Ph_Delay(base, 1, config_ptr);
         OUTBYTE(AIC7870[SCSISEQ], 0);                /* deassert reset line */
         Ph_Delay(base, 500, config_ptr);                         /* 250 ms delay */
      }

   }

   while(!(register_value & BUSFREE))
      ;

   return(ret_value); 
}

/*********************************************************************
*
*  Ph_FindNonScamDevs routine -
*
*  Find non scam devices on the bus
*
*  Return Value:  UWORD of which the bit position indicates either
*                 the existence of scam-tolerant device or HA
*                  
*  Parameters: config_ptr
*             
*
*  Activation: Ph_ScamProtocol
*                  
*  Remarks:                
*                  
*********************************************************************/
UWORD Ph_FindNonScamDevs (cfp_struct *config_ptr)
{
   UWORD ret_value;
   UBYTE target_id;
   register AIC_7870 *base;

   base = config_ptr->CFP_Base;

   ret_value = 0;
   
   /* search through the maximum allowable device range */
   for ( target_id=0; target_id < config_ptr->Cf_MaxTargets; target_id++) 
   {
      if (target_id != config_ptr->Cf_ScsiId)   /* select on non-HA id's */
      {
         if (Ph_SelectTarget(config_ptr, target_id))  /* try to select tgt */
            ret_value |= (UWORD)(0x0001 << target_id); /* selection successful */
      }
      else     /* also record the HA id */
         ret_value |= (UWORD)(0x0001 << config_ptr->Cf_ScsiId);

      Ph_Delay(base, 500, config_ptr);             /* wait 250ms */
   }

   OUTBYTE(AIC7870[SCSISEQ], SCSIRSTO);         /* reset SCSI bus */
   Ph_Delay(base, 1, config_ptr);
   OUTBYTE(AIC7870[SCSISEQ], 0);                /* deassert reset line */
   Ph_Delay(base, 500, config_ptr);                         /* 250 ms delay */

   return(ret_value);
}

/*********************************************************************
*
*  Ph_Wait4Release routine -
*  
*  Waits for the release of a signal on the SCSI bus
*
*  Return Value:  None
*                  
*  Parameters:    register offset
*                 signal - the scsi signal to be waited for release
*                 base -   AIC7870 base address
*
*  Activation:    Ph_ScamTransferInfoIn
*                 Ph_ScamTransferInfoOut
*                 Ph_FindScamDevs
*                  
*********************************************************************/
void Ph_Wait4Release ( register UBYTE register_offset,
                       UBYTE signal,
                       register AIC_7870 *base)
{
   /* maximum of glitches */
   UWORD Max_Glitches = 16;

   while (Max_Glitches != 0)
   {

      /* see if signal is deasserted */
      if ((INBYTE(AIC7870[register_offset]) & signal) == 0)
         Max_Glitches--;
   }
}

/*********************************************************************
*
*  Ph_AssignID routine - Assign SCAM id to scam targets
*
*  Return Value:  DWORD - MSW contains the latest device map
*                         LSW contains the status
*                             ASSIGN_INRPOG - assignment still in progress
*                             ASSIGN_DONE - assignment completed
*                  
*  Parameters:    config_ptr
*                 DeviceMap - bit of position, up entry, where a device
*                             (scam or non-scam or HA) resides.
*
*  Activation:    Ph_ScamProtocol
*                  
*  Remarks:       The ID assigned ID is dependent upon three factors:
*
*                    1. The maximum allowed ID based upon byte 0 in type code
*                    2. If the device is wide or narrow.
*                    3. The assigned ID shall never exceed 0x0f.  This is a
*                       limitation of the current SCAM code.  It is subjected
*                       to future review.
*
*                 ID preservation is implemented here as follows:
*
*********************************************************************/
DWORD Ph_AssignID (cfp_struct *config_ptr, UWORD DeviceMap)
{
   register AIC_7870 *base;
   UBYTE numbytes;
   UBYTE numbits;
   UBYTE type_code[2];        /* byte 0 & 1 - TYPE CODE */
   UBYTE bus_value, cycle_done;
   UBYTE parity;
   UBYTE assigned_id;
   UWORD bit_position;
   gen_union DeviceMapStat;    /* MSW - Dev Map returned; LSW - Scam Status */

   base = config_ptr->CFP_Base;
   cycle_done = 0x00;

   /* send SYNC pattern */
   Ph_ScamTransferInfoOut(base, SYNC_PATTERN); /* do the transfer cycle */

   /* send function code ISOLATE/ASSIGN to targets */
   Ph_ScamTransferInfoOut(base, ASSIGN);

   /* start the isolation stage - */
   /* if ID preservation would be done, this is where to capture  */
   /* the device type code and ID.                                */
   type_code[0] = type_code[1] = 0x00;       /* initialize scam type code */

   for ( numbytes = 0;
         numbytes <= 31;
         numbytes++)
   {
      for (numbits = 0;
           numbits <= 7;
           numbits++)
      {
         bus_value = Ph_ScamTransferInfoIn(base,0); /* read scsi bus */

         if ( (bus_value & 0x1f) == ISOLATE_TERMINATE)
         {
            if (   (numbits == 0x00)      /* return scanning */
                && (numbytes == 0x00))
               cycle_done = 0xff;
            else                       /* target has less than 32 bytes */
               cycle_done = 0x01;

            break;
         }

         else if (numbytes <= 1)      /* 1st & 2nd byte of ID string - TYPE CODE */
         {
            type_code[numbytes] <<= 1;
            type_code[numbytes] |= (UBYTE)((bus_value >> 1) & 0x01);
         }
      }                          /* end of bit transfer */

      if (cycle_done != 0x00)
         break;
   }

   if (cycle_done == 0xff)
   {
      DeviceMapStat.uword[1] = DeviceMap;
      DeviceMapStat.uword[0] = ASSIGN_DONE;
      return(DeviceMapStat.dword);   /* return device map & stat info */
   }

   /* end of isolation stage reached */
   /* see if the default is available else will get an id assigned */
   /* returned value:   bit 15-8   =  ID value of assigned ID  */
   /*                   bit  7-0   =  ID value of requested ID */
   /* Note(FKL): there is no id preservation implemented at this stage */

   assigned_id = (UBYTE)((Ph_ChooseSCAMID(config_ptr,
                                          DeviceMap,
                                          type_code[0],
                                          (UBYTE)(type_code[1] & 0x1f)) >> 8) & 0x00ff);

   if (assigned_id != 0xff)         /* if ID successfully chosen */
   {
      assigned_id &= 0x0f;          /* assigned maximum is 0x0f */
      bit_position = (UWORD)(0x0001 << assigned_id);  /* find bit position of assigned */
      DeviceMap |= bit_position;    /* update the Scam dev map info */

      /* generate the parity info of the assigned ID */
      parity = 0;
      if (!(assigned_id & 0x01))
         parity++;
      if (!(assigned_id & 0x02))
         parity++;
      if (!(assigned_id & 0x04))
         parity++;

      /* send the first quintet - assign ID command */
      if (assigned_id < 0x08)    /* id between 0x00 and 0x07 */
         Ph_ScamTransferInfoOut(base, ASSIGN_00ID);
      else                       /* id between 0x08 and 0x0f */
         Ph_ScamTransferInfoOut(base, ASSIGN_01ID);

      /* send the 2nd quintet - assigned target ID with parity information */
      Ph_ScamTransferInfoOut(base,
                            (UBYTE)((assigned_id & 0x07) | (parity << 3)));
      
      DeviceMapStat.uword[1] = DeviceMap;
      DeviceMapStat.uword[0] = ASSIGN_INPROG;
      return(DeviceMapStat.dword);   /* return device map & stat info */
   }
   else                 /* failed to assign ID */
      DeviceMapStat.uword[1] = DeviceMap;
      DeviceMapStat.uword[0] = ASSIGN_DONE;
      return(DeviceMapStat.dword);   /* return device map & stat info */
}

/*********************************************************************
*
*  Ph_ChooseScamID routine -
*
*  checks to see if it can assign the current/default ID.  If not,
*  this routine wil return with an alternate ID, if one can be found.
*
*  Return Value:  Default/current ID in LSB;
*                 and the assigned ID in MSB.
*                 0xffff if no alternate ID can be found.
*
*  Parameters: Type code info(2 bytes) of the device
*              base address of AIC-7870
*
*  Activation: Ph_AssignID
*                  
*  Remarks:                
*                  
*********************************************************************/
UWORD Ph_ChooseSCAMID ( cfp_struct *config_ptr,
                        UWORD DeviceMap,
                        UBYTE type_code,
                        UBYTE default_id)
{
   register AIC_7870 *base;
   UWORD ret_value;
   UBYTE current_id, next_id;
   UWORD bit_position;

   base = config_ptr->CFP_Base;

   switch ((type_code & ID_VALID) >> 1)
   {
      case  NOT_VALID:
         /* this is the case when the device does not have a preferred  */
         /* ID and the ID field type code has no significance           */
         switch ((type_code & MAX_ID_CODE) >> 4)
         {
            case  UPTO_0F:
            case  UPTO_1F:
               if (INBYTE(AIC7870[SBLKCTL]) & SELWIDE)
                  current_id = 0x0f;
               else
                  current_id = 0x07;   /* ID higher than 0x0f is not supported */
               break;

            case  UPTO_07:
            default:
               current_id = 0x07;
               break;
         }
         break;

      case  VALID_BUT_UNASSIGN:
         /* This the case when the device has a preferred ID and it is  */
         /* up to the SCAM protocol to assign give it the preferrence,  */
         /* if it is not already assigned, or an alternate, if it is.   */
         current_id = default_id;      /* current ID */
         break;

      case  VALID_AND_ASSIGN:
         /* this is the case when the device has already been assigned  */
         /* an ID by the SCAM protocol, as reflected in the ID field.   */
         current_id = default_id;      /* current ID */
         break;
   }

   /* start with the requested bit position and scan downward until */
   /* an empty slot is captured.                                    */

   bit_position = 0x0001 << current_id;   /* record ID in bit */
   next_id = current_id;                  /* back it up */

   do
   {
      if (!(DeviceMap & bit_position))                /* chk if available */
      {                                               /* yes, it is */
         ret_value = (UWORD)(next_id) << 8;           /* record assigned MSW */
         ret_value |= (UWORD)(default_id);            /* requested LSW */
         break;
      }
      else
      {
         next_id--;
         bit_position >>= 1;           /* try the next position */
      }
      
      if (next_id == 0xff)             /* end of the downward scan */
      {
         if (   (INBYTE(AIC7870[SBLKCTL]) & SELWIDE)
             && (   (((type_code & MAX_ID_CODE) >> 4) == UPTO_0F)
                 || (((type_code & MAX_ID_CODE) >> 4) == UPTO_1F)))
            next_id = 0x0f;            /* start with 0x0f if wide and allowed */
         else
            next_id = 0x07;            /* start with ID 0x07 otherwise */

         bit_position = 0x0001 << next_id;
      }

      if (next_id == current_id)       /* wrapped around to the start */
         ret_value = 0xffff;           /* can not assign - error! */

   } while (next_id != current_id);

   return(ret_value);
}

/*********************************************************************
*
*  Ph_ScamInitiate -  This routine will initiate the SCAM protocol.
*                     At the end, SEL, BUSY, CD, and IO are asserted.
*
*  Return Value:  None
*                  
*  Parameters: config_ptr
*
*  Activation: Ph_ScamProtocol
*                  
*  Remarks:                
*                  
*********************************************************************/
void Ph_ScamInitiate (cfp_struct *config_ptr)
{
  
   register AIC_7870 *base;
   UBYTE register_value;

   base = config_ptr->CFP_Base;

   register_value = INBYTE(AIC7870[SXFRCTL0]);
   register_value |= CLRSTCNT + SPIOEN + CLRCHN + SCAMEN;
   OUTBYTE(AIC7870[SXFRCTL0], register_value);  

   /* turn on active negation */
   register_value = INBYTE(AIC7870[SXFRCTL1]);
   register_value |= ENSTIMER + ACTNEGEN;
   OUTBYTE(AIC7870[SXFRCTL1], register_value);  

   /* Grab the bus by winning the arbitration */
   OUTBYTE(AIC7870[SCSIDATL], (0x01<<config_ptr->Cf_ScsiId));  /* HA id */
   OUTBYTE(AIC7870[SCSISIG], BSYO); /* do the arbitration */
   Ph_Delay(base, 1, config_ptr);               /* arbitration delay - 2.4us min */
   OUTBYTE(AIC7870[SCSISIG],(BSYO | SELO));  /* assert both BSY & SEL */
   Ph_Delay(base, 0, config_ptr);               /* wait for deskew delay */
   
   OUTBYTE(AIC7870[SCSIDATL], 0); /* release SCSI bus */
   OUTBYTE(AIC7870[SCSISIG], (BSYO | MSGO | SELO));   /* Assert MSG+BSY+SEL */
   Ph_Delay(base, 0, config_ptr);               /* wait for deskew delay */

   OUTBYTE(AIC7870[SCSISIG],(SELO | MSGO));  /* assert MSG & SEL, release BUSY */
   Ph_Delay(base, 500, config_ptr);             /* wait 250ms */

   OUTBYTE(AIC7870[SCSISIG],SELO);           /* release MSG */
   Ph_Wait4Release(SCSISIG, MSGO, base);     /* wait for release of MSGO */

   OUTBYTE(AIC7870[SCSISIG] ,(BSYO | SELO)); /* assert BUSY */

   Ph_Delay(base, 0, config_ptr);               /* wait for deskew delay */

   OUTBYTE(AIC7870[SCSISIG], (BSYO | SELO | CDO | IOO)); /* assert CD and IO */
   OUTBYTE(AIC7870[SCSIDATL], (DB6 | DB7));  /* assert DB6 & DB7 */

   Ph_Delay(base, 0, config_ptr);               /* wait for deskew delay */

   OUTBYTE(AIC7870[SCSISIG], (BSYO | CDO | IOO));  /* release SEL */
   Ph_Wait4Release(SCSISIG, SELO, base);     /* wait for SEL to negate */

   OUTBYTE(AIC7870[SCSIDATL],DB7);           /* de-assert DB6 */
   Ph_Wait4Release(SCSIBUSL, DB6, base);  /* wait for DB6 to negate */

   OUTBYTE(AIC7870[SCSISIG], (BSYO | SELO | CDO | IOO)); /* assert SEL */

   /* By now, the initiation of the SCAM protocol has been completed   */
   /* with SEL & DB7 asserted. we are ready to go into TRANSFER CYCLES */
}

/*********************************************************************
*
*  Ph_ScamProtocol routine -  Routine for scam protocol level 1.
*                             This protocol should be invoked whenever
*                             SCSI bus has been reset.
*
*  Return Value:  ScamMap - bit map with ID where only scam device resides
*
*  Parameters: config_ptr
*
*  Activation: 
*                  
*  Remarks:                
*                  
*********************************************************************/
UWORD PH_ScamProtocol (cfp_struct *config_ptr)
{
   register AIC_7870 *base;
   UBYTE hcntrl_data;
   UWORD NonScamDeviceMap, DeviceMap;
   gen_union DeviceMapStat;    /* MSW - Dev Map returned; LSW - Scam Status */

   base = config_ptr->CFP_Base;
   
   DeviceMap = 0;              /* Initialize the device map */

   /* make sure power is not down, interrupts off, and sequencer paused */
   hcntrl_data = INBYTE(AIC7870[HCNTRL]);    
   Ph_WriteHcntrl(base, (UBYTE)(hcntrl_data & ~(POWRDN | INTEN) ), config_ptr);   
   Ph_Pause(base, config_ptr);             /* Pause the sequencer */
   
   OUTBYTE(AIC7870[SCSISEQ], SCSIRSTO);         /* reset SCSI bus */
   Ph_Delay(base, 1, config_ptr);
   OUTBYTE(AIC7870[SCSISEQ], 0);                /* deassert reset line */
   Ph_Delay(base, 500, config_ptr);                         /* 250 ms delay */

   /* find non-scam devices and record in bit postion of DeviceMap */
   NonScamDeviceMap = Ph_FindNonScamDevs(config_ptr); /* contains only non-scam device */
   DeviceMap |= NonScamDeviceMap;   /* record non-scam devices */

   Ph_ScamInitiate(config_ptr);     /* do scam protocol initiation phase */
   
   /* assign ids to all possible scam drives.  Assignment failure would */
   /* mean the end of assigning all devices.                            */
   while (1)
   {
      DeviceMapStat.dword = Ph_AssignID(config_ptr, DeviceMap);
      DeviceMap = DeviceMapStat.uword[1];    /* update DeviceMap */

      if (DeviceMapStat.uword[0] == ASSIGN_DONE)  /* end or failure of assignment */
         break;
   }

   DeviceMap &= ~NonScamDeviceMap;           /* remove non-scam device ID's */

   while(1)          /* wait for stabilization of bus signals */
   {
      OUTBYTE(AIC7870[SXFRCTL0], 0x04);
      OUTBYTE(AIC7870[SCSISEQ],0);
      OUTBYTE(AIC7870[SCSIDATL],0);
      OUTBYTE(AIC7870[SCSISIG],0);

      if (!(INBYTE(AIC7870[SSTAT0]) & TARGET))
         break;
   }
   
   Ph_Delay(base, 250, config_ptr);

   return(DeviceMap);      /* DeviceMap contains only SCAM device location */
}
#endif         /* end of #ifdef _SCAM */

