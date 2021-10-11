/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */
/*#pragma ident "@(#)eepro.h	1.2	94/12/14 SMI"***/
#pragma ident	"@(#)eepro.h	1.2	94/12/14 SMI"

/*
 * NAME
 *        eepro.h for eepro driver ver. 1.0
 *
 *
 * SYNOPIS
 *      Hardware specific driver declarations for the "Intel EtherExpress Pro"
 * driver conforming to the Generic LAN Driver model on Solaris 2.4 (x86).
 *      Depends on the gld module of Solaris 2.4 (Generic LAN Driver)
 *
 * 
 * DESCRIPTION
 *      The eepro Ethernet driver is a multi-threaded, dynamically loadable,
 * gld-compliant, clonable STREAMS hardware driver that supports the
 * connectionless service mode of the Data Link Provider Interface,
 * dlpi (7) over an Intel EtherExpress Pro controller. The driver
 * can support multiple EEPRO controllers on the same system. It provides
 * basic support for the controller such as chip initialization,
 * frame transmission and reception, multicasting and promiscuous mode support,
 * and maintenance of error statistic counters.
 *      For more details refer eepro (7).
 *
 *
 * SEE ALSO
 *  - eepro.c, the corresponding source file
 *  - /kernel/misc/gld
 *  - eepro (7)
 *  - dlpi (7)
 *  - "Skeleton Network Device Drivers",
 *        Solaris 2.1 Device Driver Writer's Guide-- February 1993
 *
 *
 * MODIFICATION HISTORY
 *   - Version 1.0 04 July 1994
 *     Release made to Sunsoft
 *
 *
 * MISCELLANEOUS
 *      vi options for viewing this file :
 *                set ts=4 sw=4 ai wm=4
 */


#ifndef _EEPRO_H
#define _EEPRO_H 1

/*
 * Return flags used by the driver
 */

#ifndef SUCCESS
#define SUCCESS         0
#define FAILURE         -1
#endif

#define RETRY           1

/*
 * Ioctls supported by the driver
 */

#define TDR_TEST        0x1    /* time domain reflectometry test */

#ifdef EEP_DEBUG
#define EEPRODEBUG      1
#endif

/*
 * Debug flags
 */

#define DEBUG_ALL       0x0fff   /* get swamped with ALL messages ;( */
#define DEBUG_DDI		0x0001   /* debug DDI entry points */
#define DEBUG_BOARD		0x0002   /* debug board features */
#define DEBUG_INIT      0x0004   /* debug board initialization routines */
#define DEBUG_WDOG		0x0008   /* debug watchdog routine */
#define DEBUG_MCAST     0x0010   /* debug multicast function */
#define DEBUG_PROM      0x0020   /* debug promiscuous mode function */
#define DEBUG_STAT      0x0040   /* debug error statistics */
#define DEBUG_RECV      0x0080   /* debug receive packet handler */
#define DEBUG_SEND		0x0100   /* debug transmit packet routine */
#define DEBUG_INTR		0x0200   /* debug interrupt handler */
#define DEBUG_IOCTL		0x0400   /* debug ioctl handler */
#define DEBUG_TRACE     0x0800   /* trace entry into functions */


/*
 * Defines used to set up STREAMS paramters
 */

#define EEPROHIWAT         32768     /* driver flow control high water mark */
#define EEPROLOWAT         4096     /* driver flow control low water */
#define EEPROMAXPKT        1500     /* maximum media frame size */
#define EEPROIDNUM         0       /* should be a unique id; zero works */

/*
 * Board states
 */

#define EEPRO_IDLE        0      /* Board is idle */
#define EEPRO_WAITRCV     1      /* (Unused by the driver ) */
#define EEPRO_XMTBUSY     2      /* (Unused by the driver ) */
#define EEPRO_ERROR       3      /* (Unused by the driver ) */

#define SIGLEN              4    /* length of the signature */
#define MAX_EEPRO_BOARDS    20   /* maximum number of boards supported */
#define MAX_RCV_PACKET_SIZE 1530 /* Max. size of frame received */
#define MIN_PACKET_SIZE     60   /* Min. size of ethernet frame sans
                                  * CRC checksum
                                  */
#define MAX_XMT_BUF_SIZE    1514 /* maximum size of transmit buffer */
#define MAX_XMT_RETRIES     5    /* max number of retransmission attempts */

/*
 * Defines used to implement the watchdog routine
 */

#define EEPRO_WDOG_TICKS    100 * 2  /* Timeouts every 2 seconds (assuming
                                      * HZ is 100 ticks)
                                      */
#define EEPRO_ACTIVE        0x01     /* board is active */
#define EEPRO_NOXVR         0x02     /* lost carrier sense */

/*
 * defines for the DPRAM layout: note that all addresses should be
 * word aligned
 */

#define DPRAM_END           0x7ffe      /* address of last word in DPRAM */
#define NUM_XMT_BUFS        0x3         /* number of xmit buffers */
#define XMT_BUF_BASE        0x0         /* base of the first transmit buffer */
#define XMT_BUF_SIZE        0x600       /* size of the transmit buffer
                                         * rounded up to the nearest
                                         * multiple of 256 bytes
                                         */
#define NONXMT_AREA_START   (XMT_BUF_BASE + XMT_BUF_SIZE * NUM_XMT_BUFS)
                                        /* for non transmit commands */
#define NONXMT_AREA_SIZE    0x100       /* size of non-transmit cmd area */
#define XMT_AREA_END        (NONXMT_AREA_START - 2)
                                        /* end of transmit area */
#define RCV_AREA_START      (NONXMT_AREA_START + NONXMT_AREA_SIZE)
                                        /* start of receive buffer area */


/*
 * Interpretation of event field in a received frame
 */

#define FRAME_BEING_RECEIVED     0x0     /* frame not yet received fully */
#define FRAME_RECEIVED_EOF       0x08    /* frame reception over */
#define FRAME_RECLAIM_BIT_SET    0x40    /* reclaim bit is set in the frame */

/*
 * Status field of a received frame
 */

#define FRAME_OK            0x2000  /* received frame is ok */

/*
 * Status field of a transmitted frame
 */

#define NUM_COLLISIONS_MASK  0x000F /* number of collisions */
#define LATE_COLLISIONS_MASK 0x0800 /* late collision detected */
#define MAX_COLLISIONS_MASK  0x0020 /* maximum number of collisions */
#define LOST_CARRIER_SENSE   0x0400 /* lost carrier sense */
#define TX_OK                0x2000 /* xmit okay */

/*
 * Miscellaneous masks and defines
 */

#define CMD_DONE            0x80        /* command completed */
#define RCV_HDR_SIZE        8           /* header size in received frame */


/*
 * defines used during board initialization/configuration
 */

#define EEPRO_ENAB_INTR     0x80    /* Enable board interrupts */
#define ENAB_32_BIT_IO      0x10    /* Enable 32 bit i/o */
#define SEL_CUR_REG_MASK    0x20    /* select current addr register */
#define CLR_TST1_TST2_MASK  0x3f    /* bit mask for clearing TEST1 and TEST2
                                     * bits in register 3 of i/o bank 2
                                     */
#define CLR_CONNTYPE_BITS_MASK 0xDB /* clear the BNC/TPE and TPE/AUI bits */
#define NO_APORT_MASK       0x10    /* no automatic port detection */
#define NO_SRC_ADDR_INS     0x10    /* no automatic src address insertion */
#define TX_CHAIN_ERROR_DONT_STOP  0x40


/*
 * Board registers
 */

#define    CMD_REG                0    /* command reg common for all banks */

/* 
 * I/O bank 0 registers 
 */

#define    STAT_REG            1    /* Status register */
#define    ID_REG              2    /* Board ID register */
#define    INTR_REG            3    /* Interrupt mask register */
#define    RCV_CAR_LOW         4    /* RCV current address register (low) */
#define    RCV_BAR_LOW         4    /* RCV base address register (low) */
#define    RCV_CAR_HIGH        5    /* RCV current address register (high) */
#define    RCV_BAR_HIGH        5    /* RCV base address register (high) */
#define    RCV_STOP_LOW        6    /* RCV area end (low) */
#define    RCV_STOP_HIGH       7    /* RCV area end (high) */
#define    RCV_COPY_THRES_REG  8    /* RCV copy threshold register */
#define    XMT_CAR_LOW         10   /* XMT current address register (low) */
#define    XMT_BAR_LOW         10   /* XMT base address register (low) */
#define    XMT_CAR_HIGH        11   /* XMT current address register (high) */
#define    XMT_BAR_HIGH        11   /* XMT base address register (high) */
#define    HAR_LOW             12   /* Host address register (low) */
#define PORT_32_BIT_IO         12   /* 32-bit local memory i/o port */
#define    HAR_HIGH            13   /* Host address register (high) */
#define    LMEM_IO_LOW         14   /* Local Memory I/O (low) */
#define    LMEM_IO_HIGH        15   /* Local Memory I/O (high) */

/* 
 * I/O bank 1 registers 
 */

#define ENAB_INTR_REG        1    /* Enable interrupts etc. */
#define SEL_INTR_REG         2    /* select interrupt etc regsiter */
#define RCV_BOF_THRES_REG    7    /* RCV BOF Threshold Register */
#define RCV_LOW_LIM_REG      8    /* start addr (low) of receive area */
#define RCV_UP_LIM_REG       9    /* end addr (low) of receive area */
#define XMT_LOW_LIM_REG      10   /* start addr (low) of transmit area */
#define XMT_UP_LIM_REG       11   /* end addr (high) of transmit area */

/* 
 * I/O bank 2 registers 
 */

#define CONF_REG1         1    /* Enable/disable bad frame reception */
#define CONF_REG2         2    /* Loop back, type of receive etc. */
#define CONF_REG3         3    /* Connector type information */
#define IA_REG0           4    /* Individual address register 0 */
#define IA_REG1           5    /* Individual address register 1 */
#define IA_REG2           6    /* Individual address register 2 */
#define IA_REG3           7    /* Individual address register 3 */
#define IA_REG4           8    /* Individual address register 4 */
#define IA_REG5           9    /* Individual address register 5 */
#define EEPROM_REG        10   /* EEPROM Register */
#define RCV_NO_RSRC       11   /* Receive no resources counter */


/*
 * 82595TX opcodes recognized by the command register
 */

#define CMD_MASK                0x1f    /* mask to access opcode field */
#define CR_SWITCH_BANK          0x00    /* switch memory bank */
#define CR_MC_SETUP             0x03    /* multicast address setup */
#define CR_TRANSMIT             0x04    /* transmit packet */
#define CR_TDR                  0x05    /* time domain reflectometry test */
#define CR_DUMP                 0x06    /* dump command */
#define CR_DIAGNOSE             0x07    /* diagnose command */
#define CR_RCV_ENABLE           0x08    /* enable receiver */
#define CR_RCV_DISABLE          0x0a    /* disable receiver */
#define CR_RCV_STOP             0x0b    /* stop receiver */
#define CR_ABORT                0x0d    /* abort operation */
#define CR_RESET                0x0e    /* reset chip and board */
#define CR_SET_TRISTATE         0x16    /* reset tristate */
#define CR_RESET_TRISTATE       0x17    /* reset tristate */
#define CR_POWER_DOWN           0x18    /* power down */
#define CR_TRANSMIT_RESUME      0x1c    /* transmit resume command */
#define CR_SEL_RESET            0x1e    /* selective reset */

/*
 * Bank selection opcodes
 */

#define BANK_MASK               0xc0    /* mask for extracting current bank */
#define SEL_BANK0               0x00    /* bank register 0 */
#define SEL_BANK1               0x40    /* bank register 1 */
#define SEL_BANK2               0x80    /* bank register 2 */


/*
 * Status mask
 */

#define ABORT_MSK            0x20    /* MC_SETUP or DUMP was aborted */
#define STAT_RX_STOP         0x01    /* recieve stopped interrupt */
#define STAT_RX_INT          0x02    /* recieve interrupt */
#define STAT_TX_INT          0x04    /* transmit interrupt */
#define STAT_EXEC_INT        0x08    /* execution interrupt */
#define EXEC_STATES          0x30    /* execution state */
#define RECV_STATES          0xC0    /* recieve state */

/*
 * Exec states
 */

#define EXEC_IDLE            0x00    /* execution unit is idle */
#define EXEC_ACTIVE          0x02    /* execution unit is active */
#define EXEC_ABORT_IN_PROG   0x03    /* execution aborted before completion */

/*
 * receiver states
 */

#define RCVR_STATE_MASK     0xC0    /* receiver state mask */
#define RCVR_ACTIVE_MASK    0x80    /* receiver active */
#define RCVR_READY_MASK     0x40    /* receiver ready */

/*
 * Transmit modes
 */

#define TX_CON_PROC_ENAB    0x01    /* tx concurrent processing mode */
#define TX_CHAIN_ENAB_MASK  0x8000  /* enable chain bit */


/*
 * Receive modes
 */

#define ENAB_PROM_MODE        0x01    /* enable promiscuous mode */
#define DISAB_BCAST_MODE      0x02    /* disable broadcast frames reception */
#define DISCARD_BAD_FRAMES    0x80    /* disable reception of bad frames */


/*
 * Masks for use in the interrupt register
 */

#define EXEC_MASK              0x08    /* mask for EXEC bit */
#define TX_MASK                0x04    /* mask for TX bit */
#define RX_MASK                0x02    /* mask for RX bit */
#define RX_STOP_MASK           0x01    /* mask for RX Stop bit */


/*
 * EEPROM Masks
 */

#define EEDO        0x08    /* Serial data out */
#define EEDI        0x04    /* Serial data in */
#define EECS        0x02    /* EEPROM chip select */
#define EESK        0x01    /* EEPROM shift clock */
#define TURN_OFF    0x10    /* Turn off mode in 82595TX */

/*
 * EEPROM codes
 */

#define READ_EEPROM        0x06    /* read eeprom code; start bit included */
#define WRITE_EEPROM       0x05    /* write eeprom code; start bit included */
#define CLK_ENABLE         0x01    /* enable clock by writing to EESK bit */
#define CLK_DISABLE        0x0     /* disable clock by writing to EESK bit */

/* 
 * EEPROM Regisers  
 */

#define EEPROM_REG0        0   /* Interrupt selection etc.*/    
#define EEPROM_REG1        1   /* Interrupt selection etc.*/    
#define EEPROM_REG2        2   /* IA address bytes 5 & 4 */ 
#define EEPROM_REG3        3   /* IA address bytes 3 & 2 */
#define EEPROM_REG4        4   /* IA address bytes 1 & 0 */
#define EEPROM_REG5        5   /* Port Selection etc. */
#define EEPROM_REG7        7   /* INT - IRQ Bit map */

/*
 * defines used to interpret values read from the EEPROM
 */

#define EEPROM_INT_SEL_MASK     0x0007  /* Interrupt select mask */
#define EEPROM_TPE_AUI_MASK     0x0020  /* TPE/AUI mask */
#define EEPROM_TPE_BNC_MASK     0x0001  /* TPE/BNC mask */
#define EEPROM_NUM_CONN_MASK    0x0008  /* Number of connections mask */
#define EEPROM_PORT_SEL_MASK    0x00e0  /* Port selection mask */
#define EEPROM_CONNECT_AUI      0x0080  /* Connector type AUI present */
#define EEPROM_CONNECT_BNC      0x0040  /* Connector type BNC present */
#define EEPROM_CONNECT_TPE      0x0020  /* Connector type TPE present */
#define EEPROM_APORT_MASK       0x0080  /* auto port selection enabled */



/*
 *                     USEFUL MACROS
 */


/*
 * Name           : EEPRO_PRINT_EADDR
 * Purpose        : Prints the 6 bytes of the Ethernet address pointed at by
 *                  ether_addr
 * Called from    : eepro_attach()
 * Arguments      : ether_addr - pointer to an unchar byte array
 * Side effects   : None
 */

#define EEPRO_PRINT_EADDR(ether_addr)  \
{\
    int byte;\
    for(byte = 0; byte < ETHERADDRL; byte++)\
        cmn_err(CE_CONT, "%2x ", ether_addr[byte]);\
}


/*
 * Name           : EEPRO_CLR_EXEC_STAT
 * Purpose        : Clears the status register of the 82595TX
 * Called from    : Whenever a command needs to be issued to the 82595TX
 * Arguments      : base_io_address - I/O base address of the board
 * Side effects   : None
 */

#define EEPRO_CLR_EXEC_STAT(base_io_address) \
{\
    outb(base_io_address + STAT_REG, STAT_EXEC_INT);\
}


/*
 * Name           : SAFE_INCOPY
 * Purpose        : Copies data from board memory to the host memory through
 *                  the 82595TX data port. It is assumed that the host address
 *                  register is pointing to the right location before this
 *                  macro is called.
 * Called from    : Whenever data needs to be copied from board memory
 * Arguments      : base_io_address - I/O base address of the board
 *                  buf             - Buffer in host memory where data should
 *                                    be placed
 *                  size            - Number of bytes of data to be copied
 * Side effects   : None
 */

#define SAFE_INCOPY(base_io_address, buf, size)\
{\
    unchar val;\
    ushort tmplen, word;\
    \
    val = inb((base_io_address) + INTR_REG);\
    outb(base_io_address + INTR_REG, val | ENAB_32_BIT_IO);\
    repinsd(base_io_address + PORT_32_BIT_IO, ((ulong *) (buf)), size / 4);\
    tmplen = (((size) / 4) * 4);\
    outb(base_io_address + INTR_REG, val & ~ENAB_32_BIT_IO);\
    switch (size & 3)\
    {\
        case 3 :\
            *((buf) + tmplen) = inb(base_io_address + LMEM_IO_LOW);\
            *((buf) + tmplen + 1) = inb(base_io_address + LMEM_IO_HIGH);\
            word = inw(base_io_address + LMEM_IO_LOW);\
            *((buf) + tmplen + 2) = LOW(word);\
            break;\
        case 2 :\
            *((buf) + tmplen) = inb(base_io_address + LMEM_IO_LOW);\
            *((buf) + tmplen + 1) = inb(base_io_address + LMEM_IO_HIGH);\
            break;\
        case 1 :\
            word = inw(base_io_address + LMEM_IO_LOW);\
            *((buf) + tmplen) = LOW(word); \
            break;\
        default :\
            break;\
    }\
}

/*
 * Name           : SAFE_OUTCOPY
 * Purpose        : Copies data from host memory to the board memory through
 *                  the 82595TX data port. It is assumed that the host address
 *                  register is pointing to the right location before this
 *                  macro is called.
 * Called from    : Whenever data needs to be copied to board memory
 * Arguments      : base_io_address - I/O base address of the board
 *                  buf             - Pointer to the data buffer in host
 *                                    memory where data is present
 *                  size            - Number of bytes of data to be copied
 * Side effects   : None
 */

#define SAFE_OUTCOPY(base_io_address, buf, size) \
{\
    ushort tmplen;\
    unchar val;\
    \
    val = inb((base_io_address) + INTR_REG); \
    outb(base_io_address + INTR_REG, val | ENAB_32_BIT_IO); \
    repoutsd(base_io_address + PORT_32_BIT_IO, ((ulong *) (buf)), size / 4);\
    tmplen = (((size) / 4) * 4);\
    outb(base_io_address + INTR_REG, val & ~ENAB_32_BIT_IO);\
    switch (size & 3)\
    {\
        case 3 :\
            outw(base_io_address + LMEM_IO_LOW, *((ushort *) (buf) + \
                 tmplen / 2));\
            outw(base_io_address + LMEM_IO_LOW, *((ushort *) (buf) + \
                 tmplen / 2 + 1));\
            break;\
        case 2 :\
            outw(base_io_address + LMEM_IO_LOW, *((ushort *) (buf) + \
                 tmplen / 2));\
            break;\
        case 1 :\
            outw(base_io_address + LMEM_IO_LOW, *((ushort *) (buf) + \
                 tmplen / 2));\
            break;\
        default :\
            break;\
    }\
}


/*
 *               DEFINITIONS OF STRUCTURES USED BY THE DRIVER
 */


/*
 * Structures for 82595TX commands
 */

/*
 * Preamble of every 82595TX parametric command
 */

typedef struct cmd_preamble
{
    ushort cmd;           /* command */
    ushort status;        /* status */
    ushort nxt_chain_ptr; /* pointer to next command in the command chain */
    ushort byte_count;    /* number of bytes that follow the preamble */
} cmd_preamble_t;

/*
 * Header of each received frame
 */

typedef struct rcv_frame_hdr
{
    ushort event;         /* event field */
    ushort status;        /* status of each received frame */
    ushort nxt_frame_ptr; /* pointer to the next received frame */
    ushort len;           /* length of each received frame */
} rcv_frame_hdr_t;


/*
 * Ethernet address type
 */

typedef unsigned char enet_address_t[ETHERADDRL];


/*
 * Format of each entry in the multicast addresses list
 */

typedef struct 
{
    enet_address_t entry;  /* multicast addresses are 6 bytes */
} multicast_t;


/*
 * The eepro driver's private data structure
 */

struct eeproinstance
{
    ushort      nxt_rcv_frame;      /* start of next frame to be read */
    ushort      intr_level;         /* interrupt value in EEPROM */
    char        xmit_buf[MAX_XMT_BUF_SIZE];
                                    /* temporary transmit buffer --
                                     * MAKE SURE THIS IS ON A LONG WORD
                                     * BOUNDARY
                                     */
    multicast_t eepro_multiaddr[GLD_MAX_MULTICAST];
                                    /* table of mcast addresses maintained */
    int         multicast_count;    /* number of multicast addresses */
    int         autoport_flag;      /* flag to detect if autoport is enabled */
    int         eepro_watch;        /* to keep track of functional boards */
    int         timeout_id;         /* used to cancel a pending timeout */
    int         retry_count;        /* count of retransmissions */
    int         nxt_xmtbuf;         /* next xmt buffer to be used */
    int         xmt_buf_addr[NUM_XMT_BUFS];
                                    /* address of each xmit buffer */
    int         chain_flag;         /* to detect if chaining can be used */
    int         nxt_xmtintr;        /* next transmit interrupt */
};
#endif
