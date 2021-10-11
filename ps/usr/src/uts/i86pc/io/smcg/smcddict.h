/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef	_SMCDDICT_H
#define	_SMCDDICT_H

#ident "@(#)smcddict.h	1.1	95/07/18 SMI"

/*
 * SMC ddict UMAC header file
 *
 * This	file provides generic UMAC/LMAC	definitions for	ddict.
 *
 * This file is not used to build any specific functioning driver.  Rather,
 * it is used for generic compile, lint, and ddict testing of the smcg.c
 * Upper MAC driver.  For example:
 * 	cc -D_KERNEL -DSMC_INCLUDE=\"smcddict.h\" -c smcg.c
 * 	ddict -DSMC_INCLUDE=\"smcddict.h\" smcg.c
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef REALMODE
#define	SMCG_IDENT	"SMCddict"
#error "This file is not meant for REALMODE use"
#else
#define	SMCG_IDENT	"smcddict -- SMC ddict UMAC GLD driver"
#endif

#define	SMCG_NAME	"smcddict"
#define	SMMAXPKT	1514
#define	SMTRANSMIT_BUFS	2
#define	SMNUMPORTS	2

#ifdef DEBUG
#define	DEBUG_SEND
#define	DEBUG_RECV
#define	DEBUG_INTR
#define	DEBUG_SEND_LONG
#define	DEBUG_RECV_LONG
#endif

/* UM functions	and variables */
#define	SMCG_identify		smcddict_identify
#define	SMCG_probe		smcddict_probe
#define	SMCG_attach		smcddict_attach
#define	SMCG_detach		smcddict_detach
#define	SMCG_devinfo		smcddict_devinfo
#define	SMCG_reset		smcddict_reset
#define	SMCG_start_board	smcddict_start_board
#define	SMCG_stop_board		smcddict_stop_board
#define	SMCG_saddr		smcddict_saddr
#define	SMCG_dlsdmult		smcddict_dlsdmult
#define	SMCG_prom		smcddict_prom
#define	SMCG_gstat		smcddict_gstat
#define	SMCG_send		smcddict_send
#define	SMCG_intr		smcddict_intr
#define	SMCG_init_board		smcddict_init_board
#define	SMCG_get_bustype	smcddict_get_bustype
/*
#define	UM_Receive_Packet	smcddict_UM_Receive_Packet
#define	UM_Status_Change	smcddict_UM_Status_Change
#define	UM_Receive_Copy_Complete smcddict_UM_Receive_Copy_Complete
#define	UM_Send_Complete	smcddict_UM_Send_Complete
#define	UM_Interrupt		smcddict_UM_Interrupt
*/
#define	SMCG_debug		smcddict_debug
#define	SMCG_old_probe_end	smcddict_old_probe_end
#define	SMCG_old_probe_lock	smcddict_old_probe_lock

/* LM functions	*/
/*
#define	LM_Nextcard		lm_stub
#define	LM_Get_Addr		lm_stub
#define	LM_GetCnfg		lm_stub
#define	LM_Initialize_Adapter	lm_stub
#define	LM_Open_Adapter		lm_stub
#define	LM_Close_Adapter	lm_stub
#define	LM_Add_Multi_Address	lm_stub
#define	LM_Delete_Multi_Address	lm_stub
#define	LM_Change_Receive_Mask	lm_stub
#define	LM_Send			lm_stub
#define	LM_Service_Events	lm_stub
#define	LM_Disable_Adapter	lm_stub
#define	LM_Enable_Adapter	lm_stub
#define	LM_Receive_Copy		lm_stub
*/

/*
 * Generic LMAC	definitions
 *
 * Here	are the	legal and valid	definitions from the SMC
 * LMAC/UMAC Specifications, version 2.11, March 1994.
 */

typedef	unsigned char	BYTE;
typedef	unsigned short	WORD;
typedef	unsigned long	DWORD;

typedef	struct {
	DWORD		fragment_count;
	struct {
		DWORD	fragment_ptr;
		DWORD	fragment_length;
	} fragment_list[8];
} Data_Buff_Structure;

typedef	struct {
	/* Common */
	BYTE		adapter_num;
	BYTE		pc_bus;
	WORD		io_base;
	BYTE		adapter_name[12];
	WORD		irq_value;
	WORD		ram_size;
	DWORD		ram_base;
	DWORD		ram_access;
	WORD		ram_usable;
	WORD		io_base_new;
	BYTE		node_address[6];
	WORD		max_packet_size;
	WORD		num_of_tx_buffs;
	WORD		receive_mask;
	WORD		adapter_status;
	WORD		media_type;
	WORD		adapter_bus;
	WORD		pos_id;
	WORD		adapter_flags;
	BYTE		slot_num;
	BYTE		rx_lookahead_size;
	void		*sm_private;		/* For UMAC use	*/
	int		sm_first_init_done;	/* unfortunate hack */

	/* Ethernet */
	BYTE		multi_address[6];
	DWORD		*ptr_rx_CRC_errors;
	DWORD		*ptr_rx_too_big;
	DWORD		*ptr_rx_lost_pkts;
	DWORD		*ptr_rx_align_errors;
	DWORD		*ptr_rx_overruns;
	DWORD		*ptr_tx_deferred;
	DWORD		*ptr_tx_total_collisions;
	DWORD		*ptr_tx_max_collisions;
	DWORD		*ptr_tx_one_collision;
	DWORD		*ptr_tx_mult_collisions;
	DWORD		*ptr_tx_ow_collision;
	DWORD		*ptr_tx_CD_heartbeat;
	DWORD		*ptr_tx_carrier_lost;
	DWORD		*ptr_tx_underruns;
	DWORD		*ptr_ring_OVW;
} Adapter_Struc;

/* Return Codes	*/
#define	SUCCESS			0x0000
#define	ADAPTER_AND_CONFIG	0x0001
#define	ADAPTER_NO_CONFIG	0x0002
#define	NOT_MY_INTERRUPT	0x0003
#define	FRAME_REJECTED		0x0004
#define	EVENTS_DISABLED		0x0005
#define	OUT_OF_RESOURCES	0x0006
#define	INVALID_PARAMETER	0x0007
#define	INVALID_FUNCTION	0x0008
#define	ILLEGAL_FUNCTION	INVALID_FUNCTION
#define	INITIALIZE_FAILED	0x0009
#define	CLOSE_FAILED		0x000A
#define	MAX_COLLISIONS		0x000B
#define	NO_SUCH_DESTINATION	0x000C
#define	BUFFER_TOO_SMALL_ERROR	0x000D
#define	ADAPTER_CLOSED		0x000E
#define	UCODE_NOT_PRESENT	0x000F
#define	FIFO_UNDERRUN		0x0010
#define	DEST_OUT_OF_RESOURCES	0x0011
#define	ADAPTER_NOT_INITIALIZED	0x0012
#define	PENDING			0x0013
#define	UCODE_PRESENT		0x0014
#define	OPEN_FAILED		0x0080
#define	HARDWARE_FAILED		0x0081
#define	SELF_TEST_FAILED	0x0082
#define	RAM_TEST_FAILED		0x0083
#define	RAM_CONFLICT		0x0084
#define	ROM_CONFLICT		0x0085
#define	UNKNOWN_ADAPTER		0x0086
#define	CONFIG_ERROR		0x0087
#define	CONFIG_WARNING		0x0088
#define	EEROM_CKSUM_ERROR	0x008A
#define	LOBE_MEDIA_TEST_FAILED	0x0092
#define	CS_UNSUPPORTED_REV	0x0093
#define	CS_NOT_PRESENT		0x0094
#define	TUPLE_ERROR		0x0095
#define	REG_CLIENT_ERR		0x0096
#define	NOT_OUR_CARD		0x0097
#define	UNSUPPORTED_CARD	0x0098
#define	PCM_CONFIG_ERR		0x0099
#define	CARD_CONFIGURED		0x009A
#define	ADAPTER_NOT_FOUND	0xFFFF

/* Configuration Errors	*/
#define	IO_BASE_INVALID		0x0001
#define	IO_BASE_RANGE		0x0002
#define	IRQ_INVALID		0x0004
#define	IRQ_RANGE		0x0008
#define	RAM_BASE_INVALID	0x0010
#define	RAM_BASE_RANGE		0x0020
#define	RAM_SIZE_RANGE		0x0040

/* Configuration Warnings */
#define	IRQ_MISMATCH		0x0080
#define	RAM_BASE_MISMATCH	0x0100
#define	RAM_SIZE_MISMATCH	0x0200
#define	BUS_MODE_MISMATCH	0x0400

/* media_type */
#define	MEDIA_S10		0x0000
#define	MEDIA_AUI_UTP		0x0001
#define	MEDIA_BNC		0x0002
#define	MEDIA_AUI		0x0003
#define	MEDIA_STP_16		0x0004
#define	MEDIA_STP_4		0x0005
#define	MEDIA_UTP_16		0x0006
#define	MEDIA_UTP_4		0x0007
#define	MEDIA_UTP		0x0008
#define	MEDIA_BNC_UTP		0x0010
#define	MEDIA_UNKNOWN		0xFFFF

/* adapter_bus */
#define	BUS_UNK_TYPE		0x0000
#define	BUS_ISA16_TYPE		0x0001
#define	BUS_ISA8_TYPE		0x0002
#define	BUS_MCA_TYPE		0x0003
#define	BUS_EISA32M_TYPE	0x0004
#define	BUS_EISA32S_TYPE	0x0005
#define	BUS_PCMCIA_TYPE		0x0006

/* pc_bus */
#define	AT_BUS			0x00
#define	MCA_BUS			0x01
#define	EISA_BUS		0x02
#define	PCMCIA_BUS		0x03

/* adapter_status */
#define	OPEN			0x0001
#define	INITIALIZED		0x0002
#define	CLOSED			0x0003
#define	FAILED			0x0005
#define	NOT_INITIALIZED		0x0006
#define	CARD_REMOVED		0x0007
#define	CARD_INSERTED		0x0008

/* receive_mask	*/
#define	ACCEPT_PHYSICAL			0x0000
#define	ACCEPT_MULTICAST		0x0001
#define	ACCEPT_BROADCAST		0x0002
#define	PROMISCUOUS_MODE		0x0004
#define	ACCEPT_SOURCE_ROUTING		0x0008
#define	ACCEPT_ERR_PACKETS		0x0010
#define	ACCEPT_ATT_MAC_FRAMES		0x0020
#define	ACCEPT_MULTI_PROM		0x0040
#define	ACCEPT_EXT_MAC_FRAMES		0x0100
#define	ACCEPT_ALL_MAC_FRAMES		0x0120
#define	EARLY_RX_ENABLE			0x0200
#define	PKT_SIZE_NOT_NEEDED		0x0400
#define	ACCEPT_SOURCE_ROUTING_SPANNING	0x0808

/* adapter_flags */
#define	RX_VALID_LOOKAHEAD		0x0001
#define	USES_PHYSICAL_ADDR		0x0002
#define	NEEDS_PHYSICAL_ADDR		0x0003
#define	PERM_CLOSE			0x0004


#ifdef __cplusplus
}
#endif

#endif /* _SMCDDICT_H */
