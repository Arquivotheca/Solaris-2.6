/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)socket.h	1.1	93/10/29 SMI"

#define SOCKET_SERVICES         0x1A
#define SS_PRESENT              0x5353		/* 'SS' */
#define RET_OK			0x01
#define RET_ERROR		0x00

/* Socket Services Functions */

#define GET_NUM_ADAPTERS        0x80
#define REG_STATUS_CHG_CALLBACK 0x81
#define REG_CARD_TECH_CALLBACK  0x82
#define GET_SS_VERSION_NUM      0x83
#define INQUIRE_ADAPTER         0x84
#define GET_ADAPTER             0x85
#define SET_ADAPTER             0x86
#define INQUIRE_WINDOW          0x87
#define GET_WINDOW              0x88
#define SET_WINDOW              0x89
#define GET_PAGE                0x8A
#define SET_PAGE                0x8B
#define INQUIRE_SOCKET          0x8C
#define GET_SOCKET              0x8D
#define SET_SOCKET              0x8E
#define GET_CARD                0x8F
#define RESET_CARD              0x90
#define READ_ONE                0x91
#define WRITE_ONE               0x92
#define READ_MULTIPLE           0x93
#define WRITE_MULTIPLE          0x94
#define INQUIRE_EDC             0x95
#define GET_EDC                 0x96
#define SET_EDC                 0x97
#define START_EDC               0x98
#define PAUSE_EDC               0x99
#define RESUME_EDC              0x9A
#define STOP_EDC                0x9B
#define READ_EDC                0x9C

/* Socket Services Return Codes */

#define BAD_ADAPTER             0x01
#define BAD_ATTRIBUTE           0x02
#define BAD_BASE                0x03
#define BAD_EDC                 0x04
#define BAD_INDICATOR           0x05
#define BAD_IRQ                 0x06
#define BAD_OFFSET              0x07
#define BAD_PAGE                0x08
#define BAD_READ                0x09
#define BAD_SIZE                0x0A
#define BAD_SOCKET              0x0B
#define BAD_TECHNOLOGY          0x0C
#define BAD_TYPE                0x0D
#define BAD_VCC                 0x0E
#define BAD_VPP                 0x0F
#define BAD_WAIT                0x10
#define BAD_WINDOW              0x11
#define BAD_WRITE               0x12
#define NO_ADAPTERS             0x13
#define NO_CARD                 0x14
#define UNSUPPORTED_FUNCTION    0x15
