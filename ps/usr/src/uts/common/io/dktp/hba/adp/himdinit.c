/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

/* $Header:   Y:/source/aic-7870/him/himd/himdinit.cv_   1.33.1.0   14 Aug 1995 12:00:50   YU1868  $ */

#pragma ident	"@(#)himdinit.c	1.11	96/03/13 SMI"

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
*  Module Name:   HIMDINIT.C
*
*  Description:
*                 Codes specific to initialize HIM configured for driver
*                 are defined here. It should only be included if HIM is
*                 configured for driver and it can be thrown away after
*                 HIM get initialized.
*
*  Programmers:   Paul von Stamwitz
*                 Chuck Fannin
*                 Jeff Mendiola
*                 Harry Yang
*    
*  Notes:         NONE
*
*  Entry Point(s):
*     PH_CalcDataSize  - Returns size of HA data structure according to the
*                        number of SCBs (Driver only)
*     PH_GetBiosInfo   - Returns information of the BIOS (Driver only)
*
*  Revisions -
*
***************************************************************************/

#include "him_scb.h"
#include "him_equ.h"
#include "seq_off.h"

/*********************************************************************
*
*   Ph_GetDrvrConfig routine -
*
*   This routine initializes the members of the ha_Config and ha_struct
*   structures which are specific to driver only.
*
*  Return Value:  None
*                  
*  Parameters:    config_ptr
*
*  Activation:    PH_GetConfig
*                  
*  Remarks:                
*                  
*********************************************************************/

void Ph_GetDrvrConfig (cfp_struct *config_ptr)
{
   register AIC_7870 *base;
   UBYTE hcntrl_data;

   base = config_ptr->CFP_Base;
   config_ptr->CFP_DriverIdle = 1;
   if (!((hcntrl_data = INBYTE(AIC7870[HCNTRL])) & CHIPRESET))
      if (Ph_CheckBiosPresence(config_ptr))
      {
         config_ptr->CFP_BiosActive = 1;
      }

   config_ptr->Cf_MaxNonTagScbs = 2;
   config_ptr->Cf_MaxTagScbs = 32;

   /* Here we must use conditional compile because the STANDARD */
   /* mode may coexist with OPTIMA mode at compile time */
#ifdef   _STANDARD
#ifdef   _OPTIMA
   /* both OPTIMA and STANDARD may be available */
   if (! config_ptr->Cf_AccessMode)             /* Default to optima mode */
      config_ptr->Cf_AccessMode = 2;
   if (config_ptr->Cf_AccessMode == 2)
   {
      Ph_GetOptimaConfig(config_ptr);
   }
   else
   {
      Ph_GetStandardConfig(config_ptr);
   }
#else
   /* standard mode only */
   config_ptr->Cf_AccessMode = 1;
   Ph_GetStandardConfig(config_ptr);
#endif
#else
#ifdef   _OPTIMA
   /* optima mode only */
   config_ptr->Cf_AccessMode = 2;
   Ph_GetOptimaConfig(config_ptr);
#else
   /* Driver not in standard or optima mode? no way! */
#endif
#endif
}

/*********************************************************************
*
*   Ph_InitDrvrHA routine -
*
*   This routine initializes the host adapter.
*
*  Return Value:  0x00      - Initialization successful
*                 <nonzero> - Initialization failed
*                  
*  Parameters:    config_ptr
*
*  Activation:    PH_InitHA
*                  
*  Remarks:                
*
*********************************************************************/

void Ph_InitDrvrHA (cfp_struct *config_ptr)
{
   register AIC_7870 *base;
   hsp_struct *ha_ptr;
   UBYTE cnt;
   UBYTE updateflag;

   base = config_ptr->CFP_Base;
   ha_ptr = config_ptr->CFP_HaDataPtr;

   ha_ptr->zero.dword = (DWORD) 0;        /* Initialize zero field   */

   PHM_SEMSET(ha_ptr,SEM_RELS);

   /* SAVE Entire 64 bytes of PCI SCRATCH RAM into Hsp_SaveScr      */
   /*  MEMORY Area -AND- Don't Update SCRATCH RAM from MEMORY       */
   updateflag = 0;
   SWAPCurrScratchRam(config_ptr, updateflag);

   /* Set STATE = SCRATCH RAM moved into MEMORY State. NO SWAP done.*/
   ha_ptr->Hsp_SaveState = SCR_PROTMODE;

   /* set HA data structure */
   Ph_SetHaData(config_ptr);

   /* Calculate device reset breakpoint */
   OUTBYTE(AIC7870[SEQCTL], PERRORDIS);
   OUTBYTE(AIC7870[SEQCTL], PERRORDIS + LOADRAM);
   OUTBYTE(AIC7870[SEQADDR0], START_LINK_CMD_ENTRY >> 2);
   OUTBYTE(AIC7870[SEQADDR1], 00);        /* Entry points always low page  */
   INBYTE(AIC7870[SEQRAM]);
   INBYTE(AIC7870[SEQRAM]);
   cnt  = (INBYTE(AIC7870[SEQRAM]) & 0xFF);
   cnt |= ((INBYTE(AIC7870[SEQRAM]) & 0x01) << 8);
   ha_ptr->sel_cmp_brkpt = cnt + 1;

   OUTBYTE(AIC7870[SEQCTL], PERRORDIS);
   /* Here we must use conditional compile because the STANDARD */
   /* mode may coexist with OPTIMA mode at compile time */
#ifdef   _STANDARD
#ifdef   _OPTIMA
   /* both OPTIMA and STANDARD may be available */
   if (config_ptr->Cf_AccessMode == 2)
   {
      Ph_OptimaLoadFuncPtrs(config_ptr);
   }
   else
   {
      Ph_StandardLoadFuncPtrs(config_ptr);
   }
#else    /* !_OPTIMA */
   /* standard mode only */
   Ph_StandardLoadFuncPtrs(config_ptr);
#endif   /* _STANDARD */
#else    /* !_STANDARD */
#ifdef   _OPTIMA
   /* optima mode only */
   Ph_OptimaLoadFuncPtrs(config_ptr);
#else    /* !_OPTIMA */
   /* Driver not in standard or optima mode? no way! */
#endif   /* _OPTIMA */
#endif   /* !_STANDARD */

}
/*********************************************************************
*
*  PH_GetBiosInfo -
*
*  This routine retrieves information about the Arrow BIOS
*  configuration to the caller.
*
*  Return Value:  0x00 - Active BIOS, configuration info valid
*                 0xFF - No Active BIOS
*                 
*  Parameters:    config_ptr
*                 
*                 *bi_ptr - ptr to structure that BIOS
*                 information is copied to.
*
*  Activation:    Driver, initialization
*                  
*  Remarks:       Can be called before driver initializes Arrow,
*                 if desired.
*
*********************************************************************/

UWORD PH_GetBiosInfo( cfp_struct *config_ptr,
			UBYTE bus_number,
                      UBYTE device_number,
                      bios_info *bi_ptr)
{
   UWORD port_addr;
   gen_union bv_exist, bscb_addr;
   UBYTE hcntrl_data, io_buf, i, j;
   int   retval = 0xFF;
   register AIC_7870 *base;

#ifdef no_longer_needed
   /* get device base address */
   base = (AIC_7870 *) (Ph_ReadConfig(config_ptr,
                                      bus_number,
                                      device_number,
                                      BASE_ADDR_REG) & 0xfffffffc);
#endif
   base = config_ptr->CFP_Base;

   bi_ptr->bi_global = 0;           /* initialize bios_info and assume  */
   bi_ptr->bi_first_drive = bi_ptr->bi_last_drive = 0xFF;   /* no bios  */
   for (i = 0; i < 8 ; i++)                     /* available as default */
   {
      bi_ptr->bi_drive_info[i] = 0xFF;
   }

   hcntrl_data = INBYTE(AIC7870[HCNTRL]); /* save original hcntrl value */
   Ph_Pause(base, config_ptr);                               /* and Pause Sequencer */

   i = INBYTE(AIC7870[SCBPTR]);      /* save the current SCBPTR value and */
   OUTBYTE(AIC7870[SCBPTR], 2);        /* address to SCB 2 for bios info */

   /* Must have BIOS loaded and active for INT13 drive info
      to be valid */
   
                                    /* Check for BIOS int. vector */
   bv_exist.ubyte[0] = INBYTE(AIC7870[SCB00 + BIOS_BASE + 0]);
   bv_exist.ubyte[1] = INBYTE(AIC7870[SCB00 + BIOS_BASE + 1]);
   bv_exist.ubyte[2] = INBYTE(AIC7870[SCB00 + BIOS_BASE + 2]);
   bv_exist.ubyte[3] = INBYTE(AIC7870[SCB00 + BIOS_BASE + 3]);

                                   /* Check for BIOS SCB address */
   bscb_addr.ubyte[0] = INBYTE(AIC7870[SCB00 + BIOS_INTVECTR + 0]);
   bscb_addr.ubyte[1] = INBYTE(AIC7870[SCB00 + BIOS_INTVECTR + 1]);
   bscb_addr.ubyte[2] = INBYTE(AIC7870[SCB00 + BIOS_INTVECTR + 2]);
   bscb_addr.ubyte[3] = INBYTE(AIC7870[SCB00 + BIOS_INTVECTR + 3]);

   /* verify bios information does exist */
   if (   (!(INBYTE(AIC7870[HCNTRL]) & CHIPRESET))
       && bv_exist.dword
       && bscb_addr.dword
       && (bv_exist.dword != 0xFFFFFFFF)
       && (bscb_addr.dword != 0xFFFFFFFF))
   {
      bi_ptr->bi_global = BI_BIOS_ACTIVE;

      io_buf = INBYTE(AIC7870[SCB00+BIOS_GLOBAL]); /* Get Global BIOS */
      if (io_buf & BIOS_GLOBAL_DOS5)               /* parameters */
         bi_ptr->bi_global |= BI_DOS5;
      io_buf = INBYTE(AIC7870[SCB00+BIOS_GLOBAL+1]);
      if (io_buf & BIOS_GLOBAL_GIG)
         bi_ptr->bi_global |= BI_GIGABYTE;

      io_buf = INBYTE(AIC7870[SCB00+BIOS_FIRST_LAST]);    /* First, Last */
                                                               /* drives */
      bi_ptr->bi_first_drive = io_buf & 0x0F;
      bi_ptr->bi_last_drive  = (UBYTE)(io_buf & 0xF0) >> 4;

      /* Get individual drive IDs */
      port_addr = SCB00 + BIOS_DRIVES;
      j = bi_ptr->bi_last_drive - bi_ptr->bi_first_drive + 1;

      for (i = 0; i < j ; i++)
      {
         /* Get drive SCSI ID */
         bi_ptr->bi_drive_info[i] = INBYTE(AIC7870[port_addr++]) & 0x0F;
      }
      *((UBYTE *) &bi_ptr->bi_bios_segment) = INBYTE(AIC7870[SCB00+BIOS_INTVECTR+2]);
      *(((UBYTE *) &bi_ptr->bi_bios_segment)+1) = INBYTE(AIC7870[SCB00+BIOS_INTVECTR+3]);
      retval = 0;
   }
   OUTBYTE(AIC7870[SCBPTR], i);            /* Restore state and exit */
   Ph_WriteHcntrl(base, (UBYTE) (hcntrl_data), config_ptr);  
   return(retval);
}

/*********************************************************************
*
*   PH_CalcDataSize routine -
*
*   This routine will calculate the size of the HIM data structure
*   according to the number of SCBs
*
*  Return Value:  size of HIM data structure
*
*  Parameters:    mode: same as AccessMode
*                       1 - STANDARD mode
*                       2 - OPTIMA mode
*                       mode will be referenced only if both STANDARD and
*                       and OPTIMA mode are enabled at compile time.
*                 unsigned int number_scbs
*
*  Activation:    Driver, initialization
*
*  Remarks:
*
*********************************************************************/

UWORD PH_CalcDataSize (UBYTE mode,UWORD number_scbs)
{
   /* here we have to use conditional compile because the   */
   /* function may not accessable when mode was not enabled */
   /* at compile time                                       */
#ifdef _OPTIMA
#ifdef _STANDARD
   if ( mode )
   {
      return(Ph_CalcOptimaSize(number_scbs));
   }
   else
   {
      return(Ph_CalcStandardSize(number_scbs));
   }
#else /* !_STANDARD */
      return(Ph_CalcOptimaSize(number_scbs));
#endif   /* _OPTIMA */   
#else    /* !_OPTIMA */
#ifdef _STANDARD
      return(Ph_CalcStandardSize(number_scbs));
#else    /* !_STANDARD */
      /* no mode enabled? */ 
#endif   /* _STANDARD */
#endif   /* _OPTIMA */
}

/*********************************************************************
*
*   Ph_CheckBiosPresence -
*
*   This routine will check the scratch ram for a BIOS interrupt vector.
*   If no BIOS was installed, the scratch ram locations will ne null.
*
*  Return Value:  non-zero if BIOS present, 0 if BIOS not present
*
*  Parameters:    config_ptr
*
*  Activation:    PH_GetConfig
*
*  Remarks:
*
*********************************************************************/

UWORD Ph_CheckBiosPresence (cfp_struct *config_ptr)

{
   /* Use Cf_ScsiOption as temporary working area since it will be  */
   /* get initialized anyway (only the first 4 bytes will be used)   */
   Ph_ReadBiosInfo(config_ptr,BIOS_INTVECTR,config_ptr->Cf_ScsiOption,4);
   return( *((DWORD *)config_ptr->Cf_ScsiOption) != 0 &&
      *((DWORD *)config_ptr->Cf_ScsiOption) != 0xffffffff);
}

/*********************************************************************
*
*   Ph_SetDrvrScratch routine -
*
*   This routine initializes the scratch ram for driver
*
*  Return Value:  none
*                  
*  Parameters:    config_ptr
*
*  Activation:    Ph_InitDrvrHA
*                  
*  Remarks:                
*
*********************************************************************/

/*
UWORD Ph_SetDrvrScratch (cfp_struct *config_ptr)
{
}
*/
