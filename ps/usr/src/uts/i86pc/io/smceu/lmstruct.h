/*
 * This is an SMC LMAC driver.  It will periodically be updated with
 * new versions from SMC.  It is important that minimal changes be
 * made to this file, to facilitate maintenance and running diffs with
 * new versions as they arrive.  DO NOT cstyle or lint this file, or
 * make any other unnecessary changes.
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#ident	"@(#)lmstruct.h 1.2	95/07/18 SMI"

/***************
 * Return Codes
 ***************
 */

#define	SUCCESS			0x0000
#define	ADAPTER_AND_CONFIG	0x0001
#define	ADAPTER_NO_CONFIG	0x0002
#define	NOT_MY_INTERRUPT	0x0003
#define	FRAME_REJECTED		0x0004
#define	EVENTS_DISABLED		0x0005
#define	OUT_OF_RESOURCES	0x0006
#define	INVALID_PARAMETER	0x0007
#define	INVALID_FUNCTION	0x0008
#define	INITIALIZE_FAILED	0x0009
#define	CLOSE_FAILED		0x000A
#define	MAX_COLLISIONS		0x000B
#define	NO_SUCH_DESTINATION	0x000C
#define	BUFFER_TOO_SMALL_ERROR 	0x000D
#define	ADAPTER_CLOSED		0x000E
#define	UCODE_NOT_PRESENT	0x000F
#define	FIFO_UNDERRUN		0x0010
#define	DEST_OUT_OF_RESOURCES	0x0011
#define	ADAPTER_NOT_INITIALIZED	0x0012
#define	PENDING			0x0013

#define	OPEN_FAILED		0x0080
#define	HARDWARE_FAILED		0x0081
#define	SELF_TEST_FAILED	0x0082
#define	RAM_TEST_FAILED		0x0083
#define	RAM_CONFLICT		0x0084
#define	ROM_CONFLICT		0x0085
#define	UNKNOWN_ADAPTER		0x0086
#define	CONFIG_ERROR		0x0087
#define	CONFIG_WARNING		0x0088
#define	NO_FIXED_CNFG		0x0089
#define	EEROM_CKSUM_ERROR	0x008A
#define	ROM_SIGNATURE_ERROR	0x008B
#define	ROM_CHECKSUM_ERROR	0x008C
#define	ROM_SIZE_ERROR		0x008D
#define	UNSUPPORTED_NIC_CHIP	0x008E
#define	NIC_REG_ERROR		0x008F
#define	BIC_REG_ERROR		0x0090
#define	MICROCODE_TEST_ERROR	0x0091
#ifndef REALMODE
#define	ADAPTER_NOT_FOUND	0xFFFF
#else
#define	SM_ADAPTER_NOT_FOUND	0xFFFF
#endif

#define	ILLEGAL_FUNCTION	INVALID_FUNCTION
/*///////////////////////////////////////////////////////////////////////////
// Bit-Mapped codes returned in DX if return code from LM_GET_CONFIG is
// CONFIG_ERROR or CONFIG_WARNING:
/////////////////////////////////////////////////////////////////////////////
*/

/*// Errors:
*/
#define	IO_BASE_INVALID		0x0001
#define	IO_BASE_RANGE		0x0002
#define	IRQ_INVALID		0x0004
#define	IRQ_RANGE		0x0008
#define	RAM_BASE_INVALID	0x0010
#define	RAM_BASE_RANGE		0x0020
#define	RAM_SIZE_RANGE		0x0040
				    
/*// Warnings:
*/
#define	IRQ_MISMATCH		0x0080
#define	RAM_BASE_MISMATCH	0x0100
#define	RAM_SIZE_MISMATCH	0x0200
				    
/***************************************
 * Definitions for the field RING_STATUS
 ***************************************
 */

#define	ENTER_BYPASS_STATE		0x0000
#define	ENTER_INSERTED_STATE		0x0001
#define	ENTER_INITIALIZE_STATE		0x0002
#define	ENTER_TX_CL_TK_STATE		0x0003
#define	ENTER_STANDBY_STATE		0x0004
#define	ENTER_TX_BEACON_STATE		0x0005
#define	ENTER_ACTIVE_STATE		0x0006
#define	ENTER_TX_PURGE_STATE		0x0007
#define	FR_BCN_S12			0x0008
#define	FR_BCN_S21			0x0009
#define	FR_DAT_S21			0x000A
#define	TSM_EXP_S21			0x000B
#define	FR_REMOVED_S42			0x000C
#define	TBR_EXP_BR_SET_S42		0x000D
#define	TBT_EXP_S53			0x000E
#define	EXTENDED_RING_STATUS_FLAG	0x8000

/****************************************************
 * Definitions for the bit-field EXTENDED_RING_STATUS
 ****************************************************
 */ 

#define	SIGNAL_LOSS			0x8000
#define	HARD_ERROR			0x4000
#define	SOFT_ERROR			0x2000
#define	TRANSMIT_BEACON			0x1000
#define	LOBE_WIRE_FAULT			0x0800
#define	AUTO_REMOVAL_ERROR		0x0400
#define	REMOVE_RECEIVED			0x0100
#define	COUNTER_OVERFLOW		0x0080
#define	SINGLE_STATION			0x0040
#define	RING_RECOVERY			0x0020

/************************************
 * Definitions for the field BUS_TYPE
 ************************************
 */
#define	AT_BUS			0x00
#define	MCA_BUS			0x01
#define	EISA_BUS		0x02
#define	PCMCIA_BUS		0x03

/************************
 * Defs for adapter_flags 
 ************************
 */

#define	RX_VALID_LOOKAHEAD	0x0001
#define	FORCED_16BIT_MODE	0x0002
#define	ADAPTER_DISABLED	0x0004
#define	TRANSMIT_CHAIN_INT	0x0008
#define	EARLY_RX_FRAME		0x0010
#define	EARLY_TX		0x0020
#define	EARLY_RX_COPY		0x0040
#define	USES_PHYSICAL_ADDR	0x0080
#define	NEEDS_PHYSICAL_ADDR	0x0100
#define	RX_STATUS_PENDING	0x0200
#define	ERX_DISABLED		0x0400
#define ENABLE_TX_PENDING	0x08
#define ENABLE_RX_PENDING	0x1000
#define PERM_CLOSE		0x2000

/***********************
 * Adapter Status Codes
 ***********************
 */
 
#define	OPEN			0x0001
#define	INITIALIZED		0x0002
#define	CLOSED			0x0003	
#define	FAILED			0x0005
#define	NOT_INITIALIZED		0x0006
#define	IO_CONFLICT		0x0007
#define	CARD_REMOVED		0x0008

/************************
 * Mode Bit Definitions
 ************************
 */

#define	INTERRUPT_STATUS_BIT	0x8000	/* PC Interrupt Line: 0 = Not Enabled */
#define	BOOT_STATUS_MASK	0x6000	/* Mask to isolate BOOT_STATUS */
#define	BOOT_INHIBIT		0x0000	/* BOOT_STATUS is 'inhibited' */
#define	BOOT_TYPE_1		0x2000	/* Unused BOOT_STATUS value */
#define	BOOT_TYPE_2		0x4000	/* Unused BOOT_STATUS value */
#define	BOOT_TYPE_3		0x6000	/* Unused BOOT_STATUS value */
#define	ZERO_WAIT_STATE_MASK	0x1800	/* Mask to isolate Wait State flags */
#define	ZERO_WAIT_STATE_8_BIT	0x1000	/* 0 = Disabled (Inserts Wait States) */
#define	ZERO_WAIT_STATE_16_BIT	0x0800	/* 0 = Disabled (Inserts Wait States) */
#define	LOOPING_MODE_MASK	0x0003
#define	LOOPBACK_MODE_0		0x0000
#define	LOOPBACK_MODE_1		0x0001
#define	LOOPBACK_MODE_2		0x0002
#define	LOOPBACK_MODE_3		0x0003
#define	MANUAL_CRC		0x0010
#define	EARLY_TOKEN_REL		0x0020	/* Early Token Release for Token Ring*/
#define	NDIS_UMAC		0x0040/* Indicates to LMAC that UMAC is NDIS.*/
#define	UTP_INTERFACE		0x0500	/* Ethernet UTP Only.*/
#define	BNC_INTERFACE		0x0400
#define	AUI_INTERFACE		0x0300
#define	AUI_10BT_INTERFACE	0x0200
#define	STARLAN_10_INTERFACE	0x0100
#define	INTERFACE_TYPE_MASK	0x0700

/****************************
  Media Type Bit Definitions
 ****************************
 *
 * legend: 	TP = Twisted Pair
 *		STP = Shielded twisted pair
 *		UTP = Unshielded twisted pair
 */

/*#define	CNFG_MEDIA_TYPE_MASK	0x001e	/* POS Register 3 Mask         */
/* define in board_id.h */

#define	MEDIA_S10		0x0000	/* Ethernet adapter, TP.	*/
#define	MEDIA_AUI_UTP		0x0001	/* Ethernet adapter, AUI/UTP media */
#define	MEDIA_BNC		0x0002	/* Ethernet adapter, BNC media.	*/
#define	MEDIA_AUI		0x0003	/* Ethernet Adapter, AUI media.	*/
#define	MEDIA_STP_16		0x0004	/* TokenRing adap, 16Mbit STP.	*/
#define	MEDIA_STP_4		0x0005	/* TokenRing adap, 4Mbit STP.	*/
#define	MEDIA_UTP_16		0x0006	/* TokenRing adap, 16Mbit UTP.	*/
#define	MEDIA_UTP_4		0x0007	/* TokenRing adap, 4Mbit UTP.	*/
#define	MEDIA_UTP		0x0008	/* Ethernet adapter, UTP media (no AUI) */

#define	MEDIA_UNKNOWN		0xFFFF	/* Unknown adapter/media type	*/

/****************************************************************************
 * Definitions for the field:
 * bic_type (Bus interface chip type)
 ****************************************************************************
 */

#define	BIC_NO_CHIP		0x0000	/* Bus interface chip not implemented */
#define	BIC_583_CHIP		0x0001	/* 83C583 bus interface chip */
#define	BIC_584_CHIP		0x0002	/* 83C584 bus interface chip */
#define	BIC_585_CHIP		0x0003	/* 83C585 bus interface chip */
#define	BIC_593_CHIP		0x0004	/* 83C593 bus interface chip */
#define	BIC_594_CHIP		0x0005	/* 83C594 bus interface chip */
#define	BIC_564_CHIP		0x0006	/* PCMCIA Bus interface chip */
#define	BIC_790_CHIP		0x0007	/* 83C790 bus i-face/Ethernet NIC chip */
#define	BIC_571_CHIP		0x0008	/* 83C571 bus EISA bus master i-face */

/****************************************************************************
 * Definitions for the field:
 * nic_type (Bus interface chip type)
 ****************************************************************************
 */
#define	NIC_UNK_CHIP		0x0000	/* Unknown NIC chip      */
#define	NIC_8390_CHIP		0x0001	/* DP8390 Ethernet NIC   */
#define	NIC_690_CHIP		0x0002	/* 83C690 Ethernet NIC	 */
#define	NIC_825_CHIP		0x0003	/* 83C825 Token Ring NIC */
/* 	#define	NIC_???_CHIP	0x0004	*/ /* Not used		 */
/*	#define	NIC_???_CHIP	0x0005	*/ /* Not used		 */
/*	#define	NIC_???_CHIP	0x0006	*/ /* Not used		 */
#define	NIC_790_CHIP		0x0007	/* 83C790 bus i-face/Ethernet NIC chip */

/****************************************************************************
 * Definitions for the field:
 * adapter_type	The adapter_type field describes the adapter/bus
 *		configuration.
 *****************************************************************************
 */
#define	BUS_UNK_TYPE		0x0000	/*  */
#define	BUS_ISA16_TYPE		0x0001	/* 16 bit adap in 16 bit (E)ISA slot  */
#define	BUS_ISA8_TYPE		0x0002	/* 8/16b adap in 8 bit XT/(E)ISA slot */
#define	BUS_MCA_TYPE		0x0003	/* Micro Channel adapter	      */
#define	BUS_EISA32M_TYPE	0x0004	/* EISA 32 bit bus master adapter     */
#define	BUS_EISA32S_TYPE	0x0005	/* EISA 32 bit bus slave adapter      */
#define	BUS_PCMCIA_TYPE		0x0006	/* PCMCIA adapter */
/***************************
 * Receive Mask definitions
 ***************************
 */

#define	ACCEPT_MULTICAST	0x0001
#define	ACCEPT_BROADCAST	0x0002
#define	PROMISCUOUS_MODE	0x0004
#define	ACCEPT_SOURCE_ROUTING	0x0008
#define	ACCEPT_ERR_PACKETS	0x0010
#define	ACCEPT_ATT_MAC_FRAMES	0x0020
#define	ACCEPT_MULTI_PROM	0x0040
#define TRANSMIT_ONLY		0x0080
#define	ACCEPT_EXT_MAC_FRAMES	0x0100
#define	EARLY_RX_ENABLE		0x0200

#define	ACCEPT_ALL_MAC_FRAMES	0x0120

/****************************	     
 config_mode defs
****************************/

#define	STORE_EEROM		0x0001	/* Store config in EEROM. */
#define	STORE_REGS		0x0002	/* Store config in register set. */

#define	CS_SIG			0x5343	/* ASCII 'CS' */
#define	SMC_PCMCIA_ID		0x0108	/* SMC ID Byte value. */
#define	IO_PORT_RANGE		0x0020	/* Number of IO Ports.*/
#define	ENABLE_IRQ_STEER	0x0002	/* For ReqCfgStruct.ReqCfgAttributes*/
#define	FIVE_VOLTS		50	/* For ReqCfgStruct.ReqCfgVcc, .Vpp1, .Vpp2 */
#define	TWELVE_VOLTS		120	/* For ReqCfgStruct.ReqCfgVcc, .Vpp1, .Vpp2*/
#define	MEM_AND_IO		0x0002	/* For ReqCfgStruct.ReqCfgIntType */
#define	ATTRIBUTE_REG_OFFSET	0x0001	/* Hi word of offset of attribute registers*/
#define	REG_COR_VALUE		0x0041	/* Value for Config Option Register*/
#define	REGS_PRESENT_VALUE	0x000F	/* Value for ReqCfgStruct.ReqCfgPresent*/
#define	LEVEL_IRQ		0x0020	/* For ReqIrqStruct.IRQInfo1*/
#define	IRQ_INFO2_VALID		0x0010	/* For ReqIrqStruct.IRQInfo1*/
#define	VALID_IRQS		0x8EBC	/* For ReqIrqStruct.IRQInfo2*/
#define	OFFSET_SHARED_MEM	0x0002	/* Hi word of shared ram offset*/
#define	OFFSET_REGISTER_MEM	0x0003	/* Hi word of register memory offset*/
#define	AMD_ID			0xA701	/* Mfg/Product ID for AMD flash ROM*/
#define	ATMEL_ID		0xD51F	/* Mfg/Product ID for ATMEL flash ROM*/
#define	SHMEM_SIZE		0x4000	/* Size of shared memory device.*/
#define	SHMEM_NIC_OFFSET	0xD000	/* Offset of start of shared memory space for NIC.*/
#define	REG_OFFSET		0x3000	/* Offset of PCM card's mem-mapped register set.*/

#define	MAX_8023_SIZE		1500	/* Max 802.3 size of frame.*/
#define	DEFAULT_ERX_VALUE	4	/* Number of 16-byte blocks for 790B early Rx.*/
#define	DEFAULT_ETX_VALUE	64	/* Number of bytes for 790B early Tx.*/
#define	DEFAULT_TX_RETRIES	3	/* Number of transmit retries*/
#define	LPBK_FRAME_SIZE		1024	/* Default loopback frame for Rx calibration test.*/
#define	MAX_LOOKAHEAD_SIZE	252	/* Max lookahead size for ethernet.*/
