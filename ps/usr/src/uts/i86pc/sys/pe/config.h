/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)config.h	1.2	93/11/02 SMI"

#define SCPA
#define PROTO
#define DOS

#define uchar unsigned char
/****************************************************************************************************************;
/*Adapter Type Codes												 ; */
/*---------------------------------------------------------------------------------------------------------------; */

#define	POCKET_ADAPTER		0x10
#define	EXTERNAL_ADAPTER	0x20
#define	CREDITCARD_ADAPTER	0x40

#define	ETHERNET_ADAPTER	0x100
#define	TOKENRING_ADAPTER	0x200
#define	ARCNET_ADAPTER		0x400
#define	WIRELESS_ADAPTER	0x800
#define	MODEM_ADAPTER		0x1000

/* Pocket Ethernet Family */
#define	PE1			POCKET_ADAPTER   + ETHERNET_ADAPTER + 1
#define	PE2			POCKET_ADAPTER   + ETHERNET_ADAPTER + 2
#define	PE3			POCKET_ADAPTER   + ETHERNET_ADAPTER + 3
#define	EE			EXTERNAL_ADAPTER + ETHERNET_ADAPTER + 1

/* Pocket Token-Ring Family */
#define	PT1			POCKET_ADAPTER   + TOKENRING_ADAPTER + 1
#define	PT2			POCKET_ADAPTER   + TOKENRING_ADAPTER + 2
#define	ET			EXTERNAL_ADAPTER + TOKENRING_ADAPTER + 1

/* Pocket Arcnet Family */
#define	PA1			POCKET_ADAPTER + ARCNET_ADAPTER + 1
#define	PA2			POCKET_ADAPTER + ARCNET_ADAPTER + 2

/* CreditCard Ethernet Family */
#define	CE			CREDITCARD_ADAPTER + ETHERNET_ADAPTER + 1

/* Pocket Wireless Family */
#define	PW			POCKET_ADAPTER + WIRELESS_ADAPTER + 1

/****************************************************************************************************************; */
/* Media Type Codes												; */
/*---------------------------------------------------------------------------------------------------------------; */
#define MEDIA_10BT		0x0001
#define MEDIA_10B2		0x0002
#define MEDIA_10BC		0x0003
#define MEDIA_10BX		0x0004
#define MEDIA_10BU		0x0008

/****************************************************************************************************************; */
/* Hardware Status Codes												; */
/*---------------------------------------------------------------------------------------------------------------; */
#define HARDWARE_UNAVAILABLE	0x8000

/* Parallel Port Specific Hardware Status Codes */
#define PPX_PORT_B		0x0001

/****************************************************************************************************************; */
/* Initialization Error Codes											 ; */
/*---------------------------------------------------------------------------------------------------------------; */

/* Host Hardware Return Codes  (1 - 16) */
#define	ERR_ADAPTER_MISSING			1
#define	ERR_DRIVER_ALREADY_LOADED		2
#define	ERR_HARDWARE_INITIALIZATION		3
#define	ERR_SELECTED_INTERRUPT			4
#define	ERR_SELECTED_IO				5
#define	ERR_SELECTED_MEMORY			6

/* Parallel Port specific Hardware Return Codes */
#define	ERR_SELECTED_LPT			6

/* Media/Adapter Return Codes (17-32) */
#define	ERR_EEPROM_UNREADABLE			17
#define	ERR_FAILED_INITIALIZATION		18
#define	ERR_FAILED_MEMORY_TEST			19
#define	ERR_SELECTED_CONFIGURATION		20
#define	ERR_WRONG_ADAPTER			21

/* Parallel Port Configuration Codes */
#define	ALL_MODES				-1
#define	NON_BIDIRECTIONAL_MODE			1
#define	BIDIRECTIONAL_MODE			2
#define	EWRITE_MODE				4
#define	EPP_MODE				8

#define	TOSHIBA_MODE				0x0100
#define	AT_MODE					0x0200
#define	COMPAQ_MODE				0x0400
#define	PPX_HARDWARE				0x1000
#define	DELAYS_ARE_REQUIRED			0x2000
#define	FAST_STROBE_SIGNAL			0x4000
#define	FAST_AUTO_FEED_SIGNAL			0x8000

/* CreditCard Configuration Codes */
#define	MODE_16BIT_IO				1
#define	MODE_SHARED_MEMORY			2

/* Xircom Media Return Codes */

#define	XM_NO_CABLE				1				/* The cable is missing or the transmitter is stuck */
#define	XM_DRIVER_SHUTDOWN			2				/* The hardware is shutdown or missing */
#define	XM_UNAVAILABLE				3				/* The adapter is temporarily unavailable */
#define	XM_DATA_ERR				4				/* Data Transfer error */
#define	XM_OUT_OF_RESOURCES			5
