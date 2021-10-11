/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CORVETTE_POS_H
#define _CORVETTE_POS_H

#pragma	ident	"@(#)pos.h	1.1	95/02/08 SMI"

/*
 * pos.h: Get POS SCB Locate Mode request and response structures
 */


#ifdef  __cplusplus
 extern "C" {
#endif


/*
 * Get POS SCB request structure
 */

typedef struct corv_pos_req {
	unchar	pos_opcode;	/* locate mode opcode cmds */
	unchar	pos_cmd_sup;	/* Command dependent */
	ushort  pos_options;	/* enable options */
	ulong	pos_resv;	/* reserved */
	paddr_t	pos_datap;	/* system address */
	ulong	pos_data_len;	/* system buffer count */
	paddr_t	pos_tsbp;	/* tsb address */
	paddr_t	pos_linkp;	/* scb chain address */
	ulong	pos_pad;	/* padding */
}CORV_POS_REQ;

/*
 * Get POS (Programmable Option Selection) response
 */

typedef struct  corv_pos {

        ushort          p_hbaid;        /* hba id                       */
        unsigned        p3_chan   : 4;  /* pos reg 3 - arbitration level*/
        unsigned        p3_fair   : 1;  /* pos reg 3 - fairness         */
        unsigned        p3_targid : 3;  /* pos reg 3 - hba target id    */
        unchar          p2_ehba   : 1;  /* pos reg 2 - hba enable       */
        unchar          p2_ioaddr : 3;  /* pos reg 2 - ioaddr           */
        unchar          p2_romseg : 4;  /* pos reg 2 - rom addr         */
        unchar          p_intr;         /* interrupt level              */
        unchar          p_pos4;         /* pos reg 4                    */
        unsigned        p_rev   : 12;   /* revision level               */
        unsigned        p_slotsz: 4;    /* 16 or 32 slot size           */
        unchar          p_luncnt;       /* # of lun per target          */
        unchar          p_targcnt;      /* # of targets                 */
        unchar          p_pacing;       /* dma pacing factor            */
        unchar          p_ldcnt;        /* number of logical device     */
        unchar          p_tmeoi;        /* time from eoi to intr off    */
        unchar          p_tmreset;      /* time from reset to busy off  */
        ushort          p_cache;        /* cache status                 */
        ushort          p_retry;        /* retry status                 */
	unchar		p_4B ;		/* 4B reg */
	unchar		p_3B ;		/* 3B reg */
	unchar		p_6  ;		/* 6 reg */
	unchar		p_5 ;		/* 5 reg */
	ushort		p_overlap;
	ushort		p_resv[3];	/* reserved */
	ushort		p_vpd[112];	/* vital product data */

} CORV_POS;


#ifdef  __cplusplus
 }
#endif

#endif	/* _CORVETTE_POS_H */
