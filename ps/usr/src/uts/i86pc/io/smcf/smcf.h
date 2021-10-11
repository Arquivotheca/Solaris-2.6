/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _SMCF_H
#define	_SMCF_H

#ident "@(#)smcf.h	1.1	95/07/18 SMI"

/*
 * SMC Ether100 (9232) UMAC header file
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef REALMODE
#define	SMCG_IDENT	"SMC/E100"
#else
#define	SMCG_IDENT	"smcf -- SMC Ether100 (9232) UMAC GLD driver"
#endif

#define	SMCG_NAME	"smcf"
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

#include "lft_macr.h"
#include "lmstruct.h"

typedef DataBufferStructure Data_Buff_Structure;
typedef AdapterStructure Adapter_Struc;

/* UM functions and variables */
#define	SMCG_identify		smcf_identify
#define	SMCG_probe		smcf_probe
#define	SMCG_attach		smcf_attach
#define	SMCG_detach		smcf_detach
#define	SMCG_devinfo		smcf_devinfo
#define	SMCG_reset		smcf_reset
#define	SMCG_start_board	smcf_start_board
#define	SMCG_stop_board		smcf_stop_board
#define	SMCG_saddr		smcf_saddr
#define	SMCG_dlsdmult		smcf_dlsdmult
#define	SMCG_prom		smcf_prom
#define	SMCG_gstat		smcf_gstat
#define	SMCG_send		smcf_send
#define	SMCG_intr		smcf_intr
#define	SMCG_init_board		smcf_init_board
#define	SMCG_get_bustype	smcf_get_bustype
/*
#define	UM_Receive_Packet	smcf_UM_Receive_Packet
#define	UM_Status_Change	smcf_UM_Status_Change
#define	UM_Receive_Copy_Complete smcf_UM_Receive_Copy_Complete
#define	UM_Send_Complete	smcf_UM_Send_Complete
#define	UM_Interrupt		smcf_UM_Interrupt
*/
#define	SMCG_debug		smcf_debug
#define	SMCG_old_probe_end	smcf_old_probe_end
#define	SMCG_old_probe_lock	smcf_old_probe_lock

/* 9232 LM functions have their correct names */

#ifdef __cplusplus
}
#endif

#endif /* _SMCF_H */
