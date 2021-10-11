/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _SMCEU_H
#define	_SMCEU_H

#ident "@(#)smceu.h	1.1	95/07/18 SMI"

/*
 * SMC Elite Ultra (8232) UMAC header file
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef REALMODE
#define	SMCG_IDENT	"EliteUlt"
#else
#define	SMCG_IDENT	"smceu -- SMC Elite Ultra (8232) UMAC GLD driver"
#endif

#define	SMCG_NAME	"smceu"
#define	SMMAXPKT	1514
#define	SMTRANSMIT_BUFS	2
#define	SMNUMPORTS	1

#ifdef DEBUG
#define	DEBUG_SEND
#define	DEBUG_RECV
#define	DEBUG_INTR
/*
#define	DEBUG_SEND_LONG
#define	DEBUG_RECV_LONG
*/
#endif

#include "board_id.h"
#include "ebm.h"
#include "lmstruct.h"
#include "smchdw.h"
#ifdef EBM
#include "sebm.h"
#else
#error "EBM must be defined for Solaris"
#endif

/* UM functions and variables */
#define	SMCG_identify		smceu_identify
#define	SMCG_probe		smceu_probe
#define	SMCG_attach		smceu_attach
#define	SMCG_detach		smceu_detach
#define	SMCG_devinfo		smceu_devinfo
#define	SMCG_reset		smceu_reset
#define	SMCG_start_board	smceu_start_board
#define	SMCG_stop_board		smceu_stop_board
#define	SMCG_saddr		smceu_saddr
#define	SMCG_dlsdmult		smceu_dlsdmult
#define	SMCG_prom		smceu_prom
#define	SMCG_gstat		smceu_gstat
#define	SMCG_send		smceu_send
#define	SMCG_intr		smceu_intr
#define	SMCG_init_board		smceu_init_board
#define	SMCG_get_bustype	smceu_get_bustype
#define	UM_Send_Complete	UMB_Send_Complete
/*
#define	UM_Receive_Packet	smceu_UM_Receive_Packet
#define	UM_Status_Change	smceu_UM_Status_Change
#define	UM_Receive_Copy_Complete smceu_UM_Receive_Copy_Complete
#define	UM_Interrupt		smceu_UM_Interrupt
*/
#define	SMCG_debug		smceu_debug
#define	SMCG_old_probe_end	smceu_old_probe_end
#define	SMCG_old_probe_lock	smceu_old_probe_lock

/* LM functions */
#define	LM_Nextcard		LMB_Nextcard
#define	LM_Get_Addr		LMB_Get_Addr
#define	LM_GetCnfg		LMB_GetCnfg
#define	LM_Initialize_Adapter	LMB_Initialize_Adapter
#define	LM_Open_Adapter		LMB_Open_Adapter
#define	LM_Close_Adapter	LMB_Close_Adapter
#define	LM_Add_Multi_Address	LMB_Add_Multi_Address
#define	LM_Delete_Multi_Address	LMB_Delete_Multi_Address
#define	LM_Change_Receive_Mask	LMB_Change_Receive_Mask
#define	LM_Send			LMB_Send
#define	LM_Service_Events	LMB_Service_Events
#define	LM_Disable_Adapter	LMB_Disable_Adapter
#define	LM_Enable_Adapter	LMB_Enable_Adapter
#define	LM_Receive_Copy		LMB_Receive_Copy


#ifdef __cplusplus
}
#endif

#endif /* _SMCEU_H */
