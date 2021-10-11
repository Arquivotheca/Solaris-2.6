/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)epp.c	1.2	93/11/02 SMI"

#ifdef MAIN
#include <stdio.h>
#endif

#include "xcb.h"


/*****************************************************************************************************************/
/* Revision Information                                                                                          */
/*                                                                                                               */
/*       Author:         David Rosen                                                                             */
/*       Started:        December 30, 1991                                                                       */
/*                                                                                                               */
/*       Language:       MASM 5.1                                                                                */
/*       Build:          MASM eppint17/*                                                                          */
/*                       LINK eppint17/*                                                                          */
/*                       EXE2BIN eppint17 eppint17.com                                                           */
/*                                                                                                               */
/* ------------------------------------------------------------------------------------------------------------- */
/* Release       Date    Who             Why                                                                     */
/* -------       ----    ---             -----------------------------------------------                         */
/* 1.04          012193  RSS             added 486SL code             
/* 1.03          111392  RSS             386SL 'Member of Family' value changed
/* 1.02          060292  MAT             production release "only changed text string messages"                  */
/* 1.00          051192  DBR             beta release                                                            */
/* 0.90          010292  DBR             alpha release                                                           */
/*                                                                                                               */
/* ------------------------------------------------------------------------------------------------------------- */
/* Notes:                                                                                                        */
/*                                                                                                               */
/* ------------------------------------------------------------------------------------------------------------- */
/* History:                                                                                                      */
/*                                                                                                               */
/* Date   Who    What                                                                                            */
/* ----   ---    ----------------------------------------------------------------------------------------------- */
/* 010292 DBR    alpha release                                                                                   */
/* 123091 DBR    began coding                                                                                    */
/*                                                                                                               */
/*---------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------*/

/* Definitions */

#define EPP_ENABLE_FCN          0x40
#define EPP_DISABLE_FCN         0x41
#define EPP_STATUS_FCN          0x42
#define EPP_OLD_ENABLE          0xe0
#define EPP_OLD_DISABLE         0xe1
#define EPP_OLD_STATUS          0xe2
#define EPP_INSTALLED_FLAG      0xff


/*;;; 386SL EPP select definitions */

#define CPUWRMODE               0x22	/* cpu write mode register */
#define CPUWRMODE_UNLOCK_STATUS 0x01
#define CPUWRMODE_UNIT_ENABLE   0x02
#define CPUWRMODE_MCU           0x00	/* memory control unit */
#define CPUWRMODE_CU            0x04	/* cache unit */
#define CPUWRMODE_IBU           0x08	/* internal bus unit */
#define CPUWRMODE_EBU           0x0c	/* external bus unit */
#define CPUWRMODE_CPUCNFG       0x100	/* write lock CPUWRMODE register */

#define CFGSTAT                 0x23	/* configuration status register */
#define IOCFGOPN                0x80	/* bit 7 of CFGSTAT */
					/* low = 82360SL config open */
#define CFGINDEX                0x24	/* 82360SL configuration index reg */
#define CFGDATA                 0x25	/* 82360SL configuration data reg */
#define CFGR2                   0x61	/* system configuration register 2 */
#define CFGR2_SFIO_EN           0x08	/* enable special features set(82360) */
#define SFS_ENBABLE             0xfb	/* SFS Enable register (any write) */
#define SFS_DISABLE             0xf9	/* SFS Disable register (any write) */
#define SIGNATURE_REG           0x30e	/* signature register (16 bits) */
#define SL486_SIGNATURE_REG     0x70a	/* signature register (16 bits) */
#define SIGNATURE_REG_386SL     0x43	/* 386 SL ID at signature register */
#define SIGNATURE_386SL_PLUS    0x73	/* new (11-1-92) 386 SL ID at */
					/* signature register */
#define SIGNATURE_REG_486SL     0x44	/* 486 SL ID at signature register */
#define SFSINDEX                0xae	/* SFS index reg */
#define SFSDATA                 0xaf	/* window to SFS indexed registers */
#define SFSINDEX_FPP_CNTL       0x02	/* SFS indexed register */
#define FPP_CNTL_FAST_MODE      0x80	/* enable epp */
#define FPP_CNTL_EXT_MODE       0x40	/* enable bidirectionality */
#define FPP_CNTL_LPT1           0x10	/* pp is lpt1, irq7, @378h */
#define FPP_CNTL_LPT2           0x20	/* pp is lpt2, irq5, @278h */

#ifdef notdef
#define BIOS_LPT_LOCATION       word ptr (0 + 17h)*4
#define BIOS_DATA_SEG           0x40                                    /* BIOS data segment
#define LPT_TABLE               0x08                                    /* lpt address table 

/* Interrupt Vector Definitions */

#define IRQ7_SW_INT             7 + 8
#define IRQ7_LOCATION           word ptr (7 + 8) * 4
#define IRQ5_SW_INT             5 + 8
#define IRQ5_LOCATION           word ptr (5 + 8) * 4
#define TIMER_IRQ               0 + 8h
#define TIMER_LOCATION          word ptr TIMER_IRQ*4

/* resident data */

Current_Mode            db              0
Requested_Mode          db              0
LPT_Address             dw              0
#endif

int Current_Mode;
int Requested_Mode;
#ifdef notdef
int LPT_Address = 0x378;
#endif



/*****************************************************************************************************************/
/* Enable Enhanced PP Mode                                                                                       */
/*                                                                                                               */
/*       Procedure Type: Near                                                                                    */
/*                                                                                                               */
/*       Assumes:        interrupts disabled                                                                     */
/*                       al = 1, 2, or 5 for 100, 200, or 500 ns cycles                                          */
/*                                                                                                               */
/*       Returns:        nothing                                                                                 */
/*                       registers preserved: cx, dx, si, di, ds, es, bp, ss, sp, flags (DI)                     */
/*                       registers destroyed: ax, bx, flags (OSZAPC)                                             */
/*                                                                                                               */
/*       Description:                                                                                            */
/*                                                                                                               */
/*       Notes:          currently only 500 ns cycles are supported                                              */
/*---------------------------------------------------------------------------------------------------------------*/

void
EPP_Enable(h, Mode)
int Mode;
{
	int tmp, wr_mode;
	int bh, bl;

	Requested_Mode = Mode;

	wr_mode = tmp = in(CPUWRMODE);	/* save CPUWRMODE value for later */

	if (!(tmp & CPUWRMODE_UNLOCK_STATUS)) {
		outb(CFGSTAT, 0);	/* CPUWRMODE register (taken */
		outb(CPUWRMODE, 0x80);	/* ref manual) */
		out(CPUWRMODE, 0x80);
	}

EEM_CPUWRMODE_Is_Open:  
	tmp = inb(CPUWRMODE);
	tmp &= 0xf0;		/* only preserve bits 4-7 */
	tmp |= CPUWRMODE_UNIT_ENABLE+CPUWRMODE_MCU+CPUWRMODE_UNLOCK_STATUS;
	outb(CPUWRMODE, tmp);	/* enable signature reg */

	tmp = in(SIGNATURE_REG);	/* (memory ctrl unit) */
					/* read 386SL signature reg */
	if ((tmp >> 8) != SIGNATURE_REG_386SL) {

		if ((tmp>>8) != SIGNATURE_386SL_PLUS) {
			tmp = in(SL486_SIGNATURE_REG);
			if ((tmp>>8) != SIGNATURE_REG_486SL)
				goto EEM_Restore_CPUWRMODE;
		}
	}
Is_EEM_OK:
	Current_Mode = 5;              /* setup 500 ns cycle */
	/* lock CPUWRMODE register and unit spaces */
	out(CPUWRMODE, CPUWRMODE_CPUCNFG);
	tmp = inb(CFGSTAT);	/* CFGSTAT bit 7 now shows IOCFGOPN# status */

	bl = tmp;                          /* save it for later restoring */

	if (tmp & IOCFGOPN) {
		inb(0xfc23);  /* enable 82360SL config space */
		inb(0xf023);
		inb(0xc023);
		inb(0x0023);
	}
EEM_82360SL_Is_Open:    
	/* CFGR2 address is an index */
	outb(CFGINDEX, CFGR2);	/* write the index */
	tmp = inb(CFGDATA);	/* read the CFGR2 register */

	bh = tmp;	/* save for later restoring */

	tmp |= CFGR2_SFIO_EN;	/* enable special feature register access */
	outb(CFGDATA, tmp);

	outb(SFS_ENBABLE, tmp);	/* dummy write enables the rest of sf set */

	outb(SFSINDEX, SFSINDEX_FPP_CNTL);	/* select fpp control reg */
	tmp = inb(SFSDATA);

	tmp &= ~ (FPP_CNTL_LPT1+FPP_CNTL_LPT2+FPP_CNTL_EXT_MODE);

	if (xcb[h].Media_IO_Address == 0x278) /* enable the proper address bits for epp */
		tmp |= (FPP_CNTL_FAST_MODE+FPP_CNTL_LPT2);
	else
		tmp |= (FPP_CNTL_FAST_MODE+FPP_CNTL_LPT1);


EEM_Set_Fast_Mode:      
	outb(SFSDATA, tmp);
	outb(SFS_DISABLE, tmp);	/* dummy write disables sf registers */
	outb(CFGINDEX, CFGR2);	/* restore previous CFRG2 contents */
	outb(CFGDATA, bh);
	if ((bl & IOCFGOPN)) {	/* was 82360SL config space previously open? */
		outb(CFGINDEX, 0xfa);	/* disable 82360SL config space */
		outb(CFGDATA, 0x1);
	}
EEM_Restore_CPUWRMODE:  
				/* this sequence unlocks the */
	outb(CFGSTAT, 0x00);	/* CPUWRMODE register (taken */
				/* from Intel 386SL programmers */
	outb(CPUWRMODE, 0x80);	/* ref manual) */
	out(CPUWRMODE, 0x80);

	out(CPUWRMODE, wr_mode);	 /* get saved CPUWRMODE value */
	if (!(wr_mode & CPUWRMODE_UNLOCK_STATUS))
		out(CPUWRMODE, wr_mode | CPUWRMODE_CPUCNFG);

EEM_Exit:
	;
}


/*****************************************************************************************************************/
/* Disable Enhanced PP Mode                                                                                      */
/*                                                                                                               */
/*       Procedure Type: Near                                                                                    */
/*                                                                                                               */
/*       Assumes:        interrupts disabled                                                                     */
/*                                                                                                               */
/*       Returns:        nothing                                                                                 */
/*                       registers preserved: cx, dx, si, di, ds, es, bp, ss, sp, flags (DI)                     */
/*                       registers destroyed: ax, bx, flags (OSZAPC)                                             */
/*                                                                                                               */
/*       Description:                                                                                            */
/*---------------------------------------------------------------------------------------------------------------*/

void EPP_Disable(h) {
	int tmp, wr_mode;
	int bl, bh;

	Current_Mode = 0;
	tmp = in(CPUWRMODE);		/* save CPUWRMODE value for later */
	wr_mode = tmp;			/* restoration */

	if (!(tmp & CPUWRMODE_UNLOCK_STATUS)) {
					/* this sequence unlocks the */
		outb(CFGSTAT, 0x00);	/* CPUWRMODE register (taken */
					/* from Intel 386SL programmers */
		outb(CPUWRMODE, 0x80);                   /* ref manual) */
		out(CPUWRMODE, 0x80);
	}

DEM_CPUWRMODE_Is_Open:  
	tmp = in(CPUWRMODE);
	tmp &= 0xf0;
	tmp |= CPUWRMODE_UNIT_ENABLE+CPUWRMODE_MCU+CPUWRMODE_UNLOCK_STATUS;
	outb(CPUWRMODE, tmp);		/* enable signature reg */
					/* (memory ctrl unit) */

	tmp = in(SIGNATURE_REG); 	/* read 386SL signature reg */

	if ((tmp>>8) != SIGNATURE_REG_386SL) {
		if ((tmp>>8) != SIGNATURE_386SL_PLUS) {
			if ((in(SL486_SIGNATURE_REG) >> 8)!=SIGNATURE_REG_486SL)
				goto DEM_Restore_CPUWRMODE;
		}
	}

Is_DEM_OK:
	/* lock CPUWRMODE register and unit spaces */
	out(CPUWRMODE, CPUWRMODE_CPUCNFG);
	tmp = inb(CFGSTAT);	/* CFGSTAT bit 7 now shows IOCFGOPN# status */

	bl = tmp;		/* save it for later restoring */

	if (tmp & IOCFGOPN) {
		inb(0xfc23);	/* enable 82360SL config space */
		inb(0xf023);
		inb(0xc023);
		inb(0x0023);
	}

DEM_82360SL_Is_Open:    
				/* CFGR2 address is an index */
	outb(CFGINDEX, CFGR2);	/* write the index */
				/* read the CFGR2 register */
	tmp = in(CFGDATA);      /* read the CFGR2 register */

	bh = tmp;		/* save for later restoring */

	tmp |= CFGR2_SFIO_EN;	/* enable special feature register access */
	outb(CFGDATA, tmp);

	outb(SFS_ENBABLE, tmp);	/* dummy write enables the rest of sf set */

	outb(SFSINDEX, SFSINDEX_FPP_CNTL);	/* select fpp control reg */

	tmp = inb(SFSDATA);
	tmp &= ~(FPP_CNTL_FAST_MODE+FPP_CNTL_LPT1+FPP_CNTL_LPT2);

	if (xcb[h].Media_IO_Address == 0x278) /* enable the proper address bits for epp */
		tmp |= (FPP_CNTL_EXT_MODE+FPP_CNTL_LPT2);
	else
		tmp |= (FPP_CNTL_EXT_MODE+FPP_CNTL_LPT1);


DEM_Clr_Fast_Mode:      
	outb(SFSDATA, tmp);
	outb(SFS_DISABLE, tmp);	/* dummy write disables sf registers */

	outb(CFGINDEX, CFGR2);	/* restore previous CFRG2 contents */
	outb(CFGDATA, bh);
	if (bl & IOCFGOPN) {	/* was 82360SL config space previously open? */
		outb(CFGINDEX, 0xfa);	/* disable 82360SL config space */
		outb(CFGDATA, 0x01);
	}

DEM_Restore_CPUWRMODE:
	outb(CFGSTAT, 0x00);
	outb(CPUWRMODE, 0x80);
	out(CPUWRMODE, 0x80);

	out(CPUWRMODE, wr_mode);	/* get saved CPUWRMODE value */

	if (!(wr_mode & CPUWRMODE_UNLOCK_STATUS))
		/* if it was locked originally, lock it again */
		out(CPUWRMODE, wr_mode | CPUWRMODE_CPUCNFG);

DEM_Exit:
	;
}

/*****************************************************************************************************************/
/* Check If EPP Capable                                                                                          */
/*                                                                                                               */
/*       Procedure Type: Near                                                                                    */
/*                                                                                                               */
/*       Assumes:        interrupts disabled                                                                     */
/*                                                                                                               */
/*       Returns:        cx = 0 if host is not EPP capable, else cx <> 0                                         */
/*                       registers preserved: dx, si, di, ds, es, bp, ss, sp, flags (DI)                         */
/*                       registers destroyed: ax, bx, cx, flags (OSZAPC)                                         */
/*                                                                                                               */
/*       Description:                                                                                            */
/*---------------------------------------------------------------------------------------------------------------*/

Check_If_EPP_Capable(h) {
	int rval = 0;
	int tmp, bx;

	tmp = in(CPUWRMODE);	/* save CPUWRMODE value for later */
	bx = tmp;		/* restoration */

	if (!(tmp & CPUWRMODE_UNLOCK_STATUS)) {
					/* this sequence unlocks the */
		outb(CFGSTAT, 0x00);	/* CPUWRMODE register (taken */
					/* from Intel 386SL programmers */
		outb(CPUWRMODE, 0x80);                   /* ref manual) */
		out(CPUWRMODE, 0x80);
	}

CIEC_CPUWRMODE_Is_Open: 
	tmp = in(CPUWRMODE);
	tmp &= 0xf0;		/* only preserve bits 4-7 */
	tmp |= CPUWRMODE_UNIT_ENABLE+CPUWRMODE_MCU+CPUWRMODE_UNLOCK_STATUS;
	outb(CPUWRMODE, tmp);	/* enable signature reg */

	tmp = in(SIGNATURE_REG);               /* (memory ctrl unit) */
						/* read 386SL signature reg */
	tmp >>= 8;

	if (tmp == SIGNATURE_REG_386SL || tmp == SIGNATURE_386SL_PLUS)
		goto Is_CIEC_OK;

	if (in(SL486_SIGNATURE_REG) >> 8 != SIGNATURE_REG_486SL)
		goto CIEC_Restore_CPUWRMODE;

Is_CIEC_OK:
	rval = -1;

CIEC_Restore_CPUWRMODE: 
				/* this sequence unlocks the */
	outb(CFGSTAT, 0x00);	/* CPUWRMODE register (taken */
				/* from Intel 386SL programmers */
	outb(CPUWRMODE, 0x80);                   /* ref manual) */
	out(CPUWRMODE, 0x80);

	out(CPUWRMODE, bx);

	if (!(bx & CPUWRMODE_UNLOCK_STATUS))
		/* if it was locked originally, lock it again */
		out(CPUWRMODE, bx | CPUWRMODE_CPUCNFG);
CIEC_Exit:
	return rval;
}

#ifdef MAIN

main() {
	printf("Check_If_EPP_Capable returns %d\n",
		Check_If_EPP_Capable());
	Enable_EPP_Mode();
	Disable_EPP_Mode();
}

#endif
