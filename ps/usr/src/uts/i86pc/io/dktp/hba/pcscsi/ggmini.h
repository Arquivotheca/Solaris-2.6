/*++
                  
   (C) Copyright 1993-1994 by Advanced Micro Devices, Inc. - All Rights 
   Reserved.

   This software is unpublished and contains the trade secrets and 
   confidential proprietary information of AMD.  Unless otherwise provided
   in the Software Agreement associated herewith, it is licensed in confidence
   "AS IS" and is not to be reproduced in whole or part by any means except
   for backup.  Use, duplication, or disclosure by the Government is subject
   to the restrictions in paragraph (b) (3) (B) of the Rights in Technical
   Data and Computer Software clause in DFAR 52.227-7013 (a) (Oct 1988).
   Software owned by Advanced Micro Devices, Inc., 901 Thompson Place,
   Sunnyvale, CA 94088.

   FILE: GGMINI.H

     This is the portable core header code for the AMD Golden Gate SCSI
     controller and DMA engine.

   REVISION HISTORY:

     DATE      VERSION INITIAL COMMENT
     --------  ------- ------- -------------------------------------------
     02/06/95  2.01    DM      Modified and released for Windows NT and
                               Windows 95
     12/07/94  2.0     DM      Modified and released for Windows NT and
                               Windows 95
     08/18/94  1.95    DM      Modified and released for NT 3.5 (Daytona 
                               Beta II)
     02/22/94  1.0     DM      Released for NT 3.1

--*/

//---------------------------------------------------------------
// Operating system flag definitions:
//    NT_31 - for Windows NT 3.1 which does not support PCI bus
//    OTHER_OS - for other operating system implementation which
//               utilizes I/O control functions.
// 
// If no operating system flag set(by default), the code is for 
// NT 3.5 and Windows 95.
//---------------------------------------------------------------

#ifndef _AMD53C794_
#define _AMD53C794_


//
// SCSI Protocol Chip Definitions.
//
// Define SCSI Protocol Chip Read registers structure.
//
typedef struct _SCSI_READ_REGISTERS {
    ULONG TransferCountLow;
    ULONG TransferCountHigh;
    ULONG Fifo;
    ULONG Command;
    ULONG ScsiStatus;
    ULONG ScsiInterrupt;
    ULONG SequenceStep;
    ULONG FifoFlags;
    ULONG Configuration1;
    ULONG Reserved1;
    ULONG Reserved2;
    ULONG Configuration2;
    ULONG Configuration3;
    ULONG Configuration4;
    ULONG TransferCountPage;
    ULONG FifoBottem;
} SCSI_READ_REGISTERS, *PSCSI_READ_REGISTERS;

//
// Define SCSI Protocol Chip Write registers structure.
//
typedef struct _SCSI_WRITE_REGISTERS {
    ULONG TransferCountLow;
    ULONG TransferCountHigh;
    ULONG Fifo;
    ULONG Command;
    ULONG DestinationId;
    ULONG SelectTimeOut;
    ULONG SynchronousPeriod;
    ULONG SynchronousOffset;
    ULONG Configuration1;
    ULONG ClockConversionFactor;
    ULONG TestMode;
    ULONG Configuration2;
    ULONG Configuration3;
    ULONG Configuration4;
    ULONG TransferCountPage;
    ULONG FifoBottem;
} SCSI_WRITE_REGISTERS, *PSCSI_WRITE_REGISTERS;

typedef union _SCSI_REGS {
    SCSI_READ_REGISTERS  ReadRegisters;
    SCSI_WRITE_REGISTERS WriteRegisters;
} SCSI_REGS, *PSCSI_REGS;

//
// Define SCSI Command Codes.
//
#define NO_OPERATION_DMA            0x80
#define FLUSH_FIFO                  0x1
#define RESET_SCSI_CHIP             0x2
#define RESET_SCSI_BUS              0x3
#define TRANSFER_INFORMATION        0x10
#define TRANSFER_INFORMATION_DMA    0x90
#define COMMAND_COMPLETE            0x11
#define MESSAGE_ACCEPTED            0x12
#define TRANSFER_PAD                0x18
#define SET_ATTENTION               0x1a
#define RESET_ATTENTION             0x1b
#define RESELECT                    0x40
#define SELECT_WITHOUT_ATTENTION    0x41
#define SELECT_WITH_ATTENTION       0x42
#define SELECT_WITH_ATTENTION_STOP  0x43
#define ENABLE_SELECTION_RESELECTION 0x44
#define DISABLE_SELECTION_RESELECTION 0x45
#define SELECT_WITH_ATTENTION3      0x46

//
// Define SCSI Status Register structure.
//
typedef struct _SCSI_STATUS {
    UCHAR Phase                 : 3;
    UCHAR ValidGroup            : 1;
    UCHAR TerminalCount         : 1;
    UCHAR ParityError           : 1;
    UCHAR GrossError            : 1;
    UCHAR Interrupt             : 1;
} SCSI_STATUS, *PSCSI_STATUS;

//
// Define SCSI Phase Codes.
//
#define DATA_OUT        0x0
#define DATA_IN         0x1
#define COMMAND_OUT     0x2
#define STATUS_IN       0x3
#define TARGET_IDLE     0x4
#define TARGET_DISCNT   0x5
#define MESSAGE_OUT     0x6
#define MESSAGE_IN      0x7

//
// Define SCSI Interrupt Register structure.
//
typedef struct _SCSI_INTERRUPT {
    UCHAR Selected              : 1;
    UCHAR SelectedWithAttention : 1;
    UCHAR Reselected            : 1;
    UCHAR FunctionComplete      : 1;
    UCHAR BusService            : 1;
    UCHAR Disconnect            : 1;
    UCHAR IllegalCommand        : 1;
    UCHAR ScsiReset             : 1;
} SCSI_INTERRUPT, *PSCSI_INTERRUPT;

//
// Define SCSI Sequence Step Register structure.
//
typedef struct _SCSI_SEQUENCE_STEP {
    UCHAR Step                  : 3;
    UCHAR MaximumOffset         : 1;
    UCHAR Reserved              : 4;
} SCSI_SEQUENCE_STEP, *PSCSI_SEQUENCE_STEP;

//
// Define SCSI Fifo Flags Register structure.
//
typedef struct _SCSI_FIFO_FLAGS {
    UCHAR ByteCount             : 5;
    UCHAR FifoStep              : 3;
} SCSI_FIFO_FLAGS, *PSCSI_FIFO_FLAGS;

//
// Define SCSI Configuration 1 Register structure.
//
typedef struct _SCSI_CONFIGURATION1 {
    UCHAR HostBusId             : 3;
    UCHAR ChipTestEnable        : 1;
    UCHAR ParityEnable          : 1;
    UCHAR ParityTestMode        : 1;
    UCHAR ResetInterruptDisable : 1;
    UCHAR SlowCableMode         : 1;
} SCSI_CONFIGURATION1, *PSCSI_CONFIGURATION1;

//
// Define SCSI Configuration 2 Register structure.
//
typedef struct _SCSI_CONFIGURATION2 {
    UCHAR DmaParityEnable       : 1;
    UCHAR RegisterParityEnable  : 1;
    UCHAR TargetBadParityAbort  : 1;
    UCHAR Scsi2                 : 1;
    UCHAR HighImpedance         : 1;
    UCHAR EnableByteControl     : 1;
    UCHAR EnablePhaseLatch      : 1;
    UCHAR ReserveFifoByte       : 1;
} SCSI_CONFIGURATION2, *PSCSI_CONFIGURATION2;

//
// Define SCSI Configuration 3 Register structure.
//
typedef struct _SCSI_CONFIGURATION3 {
    UCHAR Threshold8            : 1;
    UCHAR AlternateDmaMode      : 1;
    UCHAR SaveResidualByte      : 1;
    UCHAR FastClock             : 1;
    UCHAR FastScsi              : 1;
    UCHAR EnableCdb10           : 1;
    UCHAR EnableQueue           : 1;
    UCHAR CheckIdMessage        : 1;
} SCSI_CONFIGURATION3, *PSCSI_CONFIGURATION3;

//
// Define SCSI Configuration 4 Register structure.
//
typedef struct _SCSI_CONFIGURATION4 {
    UCHAR Reserved              : 2;
    UCHAR ActiveNegation        : 2;
    UCHAR Am53c94               : 1;
    UCHAR SleepMode             : 1;
    UCHAR KillGlitch            : 2;
} SCSI_CONFIGURATION4, *PSCSI_CONFIGURATION4;

//
// Define Emulex FAS 216 unique part Id code.
//
typedef struct _AMD_PART_CODE {
    UCHAR RevisionLevel         : 3;
    UCHAR ChipFamily            : 5;
}AMD_PART_CODE, *PAMD_PART_CODE;

#define EMULEX_FAS_216  2


//--------------------------------------
// PCI configuration register structures
//--------------------------------------
//
// Register structure for method 1 to access PCI configuration space
//
typedef struct _PCI_CONFIG_M1_REGS {        
    ULONG ConfigAddressReg;
    ULONG ConfigDataReg;
} PCI_CONFIG_M1_REGS, *PPCI_CONFIG_M1_REGS;

//
// Register structure for method 2 to access PCI configuration space
//
typedef struct _PCI_CONFIG_M2_REGS {        
    UCHAR ConfigSpaceEnableReg;
    UCHAR Undefined;
    UCHAR ForwordAddressReg;
} PCI_CONFIG_M2_REGS, *PPCI_CONFIG_M2_REGS;
    
//
// Register structure for accessing PCI configuration space
// 
typedef union _PCI_CONFIG_REGS {
    PCI_CONFIG_M1_REGS PciConfigM1Regs;
    PCI_CONFIG_M2_REGS PciConfigM2Regs;
} PCI_CONFIG_REGS, *PPCI_CONFIG_REGS;

#define CONFIG_REG_BASE 0xcf8

//
// Bit definition for method 1 configuration address register
//
typedef struct _PCI_CONFIG_M1_DATA {
    ULONG Register       : 8;    // Register in config. space header to access
    ULONG FunctionNumber : 3;    // Function number
    ULONG DeviceNumber   : 5;    // Device to access
    ULONG BusNumber      : 8;    // Bus to acess
    ULONG ReservedBits   : 7;    // Bits reserved
    ULONG EnableConfig   : 1;    // Enable configuration operation bit
} PCI_CONFIG_M1_DATA, *PPCI_CONFIG_M1_DATA;

//
// Bit definition for method 2 configuration space enable register
//
typedef struct _PCI_CONFIG_M2_DATA {
    UCHAR ZeroBits       : 1;    // Must be cleared
    UCHAR FunctionNumber : 3;    // Funciton number
    UCHAR Key            : 4;    // Operation mode
} PCI_CONFIG_M2_DATA, *PPCI_CONFIG_M2_DATA;

#define NORMAL_MODE 0            // Key: Normal mode
#define CONFIG_MODE !NORMAL_MODE // Key: Config mode

//
// Initialization data structure
//
typedef struct _INIT_DATA {
    UCHAR busNumber;
    UCHAR deviceNumber;
    UCHAR functionNumber;
} INIT_DATA, *PINIT_DATA;

//
// Configuration information structure
//
typedef struct _CONFIG_BLOCK {
    UCHAR Method;
    UCHAR FunctionNumber;
    UCHAR BusNumber;
    UCHAR DeviceNumber;
    ULONG BaseAddress;
    ULONG IRQ;
    UCHAR ChipRevision;
    BOOLEAN PciFuncSupport;
} CONFIG_BLOCK, *PCONFIG_BLOCK;

//--------------------------------------
// PCI CONFIGRATION SPACE HEADER
//--------------------------------------
typedef struct _CONFIG_SPACE_HEADER {
    ULONG IDRegs;
    ULONG CmdStatusRegs;
    ULONG Misc1Regs;
    ULONG Misc2Regs;
    ULONG BaseAddress0Reg;
    ULONG BaseAddress1Reg;
    ULONG BaseAddress2Reg;
    ULONG BaseAddress3Reg;
    ULONG BaseAddress4Reg;
    ULONG BaseAddress5Reg;
    ULONG Reserved0Reg;
    ULONG Reserved1Reg;
    ULONG ExpROMCtrlReg;
    ULONG Reserved2Reg;
    ULONG Reserved3Reg;
    ULONG InterruptReg;
    USHORT SCSIStateReg[8];
} CONFIG_SPACE_HEADER, *PCONFIG_SPACE_HEADER;


//
// Bit definition for Command-Status register
//
typedef struct _PCI_SCSI_COMMAND {
    ULONG IOSpace        : 1;    // IO space enable
    ULONG MemorySpace    : 1;    // Memory space enable
    ULONG BusMaster      : 1;    // Bus master enable
    ULONG SpecialCycles  : 1;    // Special cycle monitor enable
    ULONG MemWrite       : 1;    // Memory write enable
    ULONG VGASnoop       : 1;    // VGA palette snooping enable
    ULONG ParityCheck    : 1;    // Parity check enable
    ULONG WaitCycle      : 1;    // Address / data stepping
    ULONG SERREnable     : 1;    // SERR# driver enable
    ULONG FastBacktoBack : 1;    // Fast back to back transactions enable
    ULONG Status         : 22;   // Status register bits
} PCI_SCSI_COMMAND, *PPCI_SCSI_COMMAND;


//
// Bit definition for SCSI target state register
//
typedef struct _PCI_TARGET_STATE {
    USHORT TargetExist    : 1;    // Target presents
    USHORT SCSIPhase      : 3;    // SCSI phase
    USHORT SyncOffset     : 4;    // Synchronous offset
    USHORT SyncPeriod     : 5;    // Synchronous period
    USHORT FastScsi       : 1;    // Fast SCSI target device
    USHORT Reserved       : 2;    // Reserved, don't touch
} PCI_TARGET_STATE, *PPCI_TARGET_STATE;


//
// Bit definition for SCSI host state register
//
typedef struct _PCI_HOST_STATE {
    USHORT TargetExist    : 1;    // Target presents
    USHORT InitRealMode   : 1;    // Real mode driver initialized
    USHORT InitProtMode   : 1;    // Protection mode initialized
    USHORT HostAdapter    : 1;    // Host adapter
    USHORT BIOSNumber     : 3;    // BIOS number
    USHORT BIOSNumValid   : 1;    // BIOS number valid
    USHORT BusReset       : 1;    // SCSI bus reset has taken place
    USHORT CompaqSystem   : 1;    // Compaq system
    USHORT ZeroBits       : 2;    // The bits must be set to zero
    USHORT Reserved       : 4;    // Reserved, don't touch
} PCI_HOST_STATE, *PPCI_HOST_STATE;


//
// Operation definition for information update
//
typedef enum _UPDATE_OPERATION {
    HBAInitialization,
    Reset,
    Synchronous,
    Phase,
    TargetIdle,
    TargetDisconnected,
    TargetNotExist
} UPDATE_OPERATION, *PUPDATE_OPERATION;


#define AMD_PCI_SCSI_ID 0x20201022   // AMD PCI Golden Gate SCSI ID pattern
#define INVALID_VENDOR_ID 0xffff     // Invalid device ID pattern
#define INTEL_PCI_ID 0x8086          // Intel PCI chipset ID
#define DEVICE_NOT_SEEN (ULONG)~0    // Device not found
#define DEVICE_DISABLED 0            // Device disabled
#define METHOD2_CONFIG_SPACE 0xc000  // Method 2 config. space map address
#define METHOD2_CONFIG_SPACE_SIZE 0xd000-0xc000 // Size of method 2 config. space
#define MAXIMUM_BUS_NUMBER 8         // Maximum number of PCI buses supported
#define M1_MAX_DEVICE_NUMBER 32      // Max. number of PCI devices on each bus for method 1
#define M2_MAX_DEVICE_NUMBER 16      // Max. number of PCI devices on each bus for method 2

//--------------------------------------
// GOLDEN GATE register structures
//--------------------------------------
//
// SCSI DMA Engine register structure
//
typedef struct _DMA_CCB {
    ULONG CommandReg;
    ULONG StartTransferCountReg;
    ULONG StartTransferAddressReg;
    ULONG WorkByteCountReg;
    ULONG WorkAddressReg;
    ULONG StatusReg;
    ULONG MDLAddressReg;
    ULONG WorkMDLAddressReg;
    ULONG ReservedRegs[4];
    ULONG MiscReg;
} DMA_CCB, *PDMA_CCB;

//
// Bits definition for DMA CCB command register
//
typedef struct _DMA_CMD_DATA {
    ULONG Command              : 2;  // Command field
    ULONG Reserved             : 2;  // Reserved
    ULONG EnableScatterGather  : 1;  // Enable the mapping of SLA through MDL
    ULONG EnablePageInterrupt  : 1;  // Interrupt after each page transfered
    ULONG EnableDMAInterrupt   : 1;  // Interrupt after DMA stoped
    ULONG TransferDirection    : 1;  // Transfer direction. 1 = Read
    ULONG DontCare             : 24; // Don't care
} DMA_CMD_DATA, *PDMA_CMD_DATA;

//
// Bits definition for DMA CCB command register
//
typedef struct _DMA_MISC_DATA {
    ULONG DataBus              : 8;  // Command field
    ULONG ParityBit            : 1;  // Parity bit
    ULONG ControlBits          : 3;  // Control signals
    ULONG ATN                  : 1;  // Attension bit
    ULONG SEL                  : 1;  // Selection bit
    ULONG BSY                  : 1;  // Busy bit
    ULONG RET                  : 1;  // Reset bit
    ULONG ACK                  : 1;  // Ack bit
    ULONG REQ                  : 1;  // Req bit
    ULONG SCAMEnable           : 1;  // Scam bit
    ULONG SCSIClockPresent     : 1;  // Clock bit
    ULONG BusBusy              : 1;  // SCSI bus busy
    ULONG PowerDown            : 1;  // Power down control
    ULONG Reserved             : 2;  // Reserved bits
    ULONG WriteClearEnable     : 1;  // Enable DMA status write/clear func.
    ULONG PCIAbortIntEnable    : 1;  // Enable master/target abort interrupt
    ULONG DontCare             : 6;  // Don't care
} DMA_MISC_DATA, *PDMA_MISC_DATA;

// Definition for DMA command register command field
#define DMA_IDLE  0x00
#define DMA_BLAST 0x01
#define DMA_ABORT 0x02
#define DMA_START 0x03

//
// Bits definition for PCI status register
//
#define PowerDown           0x0001    // Power down interrupt
#define DMAError            0x0002    // DMA error
#define DMAAbort            0x0004    // DMA abort operation
#define DMADone             0x0008    // DMA stoped
#define SCSICoreInterrupt   0x0010    // SCSI Core interrupt
#define DMAFlushDone        0x0020    // DMA flush done
#define PCIAbort            0x0040    // PCI master/target abort

//
// SCSI and DMA register address
//
typedef struct _REG_GROUP {
    SCSI_REGS ScsiRegs;
    DMA_CCB DmaCcbRegs;
} REG_GROUP, *PREG_GROUP;

//---------------------------------------
// Definition for maximum transfer length
//---------------------------------------
#define MAXIMUM_MDL_DESCRIPTORS      17

typedef struct _MDL {
    ULONG MDLTable[MAXIMUM_MDL_DESCRIPTORS];
} MDL, *PMDL;

#define INITIATOR_BUS_ID             0x7
#define SELECT_TIMEOUT_FACTOR        250000/8192  // Time out factor
#define SYNCHRONOUS_OFFSET           0xf
#define ASYNCHRONOUS_OFFSET          0
#define ASYNCHRONOUS_PERIOD          0x9
#define RESET_STALL_TIME             25     // The minimum assertion time for
                                            //   a SCSI bus reset.
#define INTERRUPT_STALL_TIME         5      // Time to wait for the next interrupt.
#define UNLIMITED                    0xffffffff  // Unlimited


//
// AMD 53c94 specific port driver device extension flags.
//
#define PD_SYNCHRONOUS_RESPONSE_SENT    0x0001
#define PD_SYNCHRONOUS_TRANSFER_SENT    0x0002
#define PD_PENDING_START_IO             0x0004
#define PD_MESSAGE_OUT_VALID            0x0008
#define PD_DISCONNECT_EXPECTED          0x0010
#define PD_SEND_MESSAGE_REQUEST         0x0020
#define PD_POSSIBLE_EXTRA_MESSAGE_OUT   0x0040
#define PD_PENDING_DATA_TRANSFER        0x0080
#define PD_PARITY_ERROR_LOGGED          0x0100
#define PD_EXPECTING_RESET_INTERRUPT    0x0200
#define PD_EXPECTING_QUEUE_TAG          0x0400
#define PD_TAGGED_SELECT                0x0800
#define PD_BUFFERED_DATA                0x1000
#define PD_ONE_BYTE_XFER                0x2000
#define PD_DATA_OVERRUN                 0x4000
#define PD_REQUEST_POSTING              0x8000
#define PD_BUFFERED_BLAST_DATA         0x10000

//
// The following defines specify masks which are used to clear flags when
// specific events occur, such as reset or disconnect.
//
#define PD_ADAPTER_RESET_MASK ( PD_SYNCHRONOUS_TRANSFER_SENT  | \
                                PD_PENDING_START_IO           | \
                                PD_MESSAGE_OUT_VALID          | \
                                PD_SEND_MESSAGE_REQUEST       | \
                                PD_POSSIBLE_EXTRA_MESSAGE_OUT | \
                                PD_PENDING_DATA_TRANSFER      | \
                                PD_PARITY_ERROR_LOGGED        | \
                                PD_EXPECTING_QUEUE_TAG        | \
                                PD_BUFFERED_DATA              | \
                                PD_BUFFERED_BLAST_DATA        | \
                                PD_TAGGED_SELECT              | \
                                PD_ONE_BYTE_XFER              | \
                                PD_DATA_OVERRUN               | \
                                PD_REQUEST_POSTING            | \
                                PD_DISCONNECT_EXPECTED          \
                                )

#define PD_ADAPTER_DISCONNECT_MASK ( PD_SYNCHRONOUS_TRANSFER_SENT  | \
                                     PD_MESSAGE_OUT_VALID          | \
                                     PD_SEND_MESSAGE_REQUEST       | \
                                     PD_POSSIBLE_EXTRA_MESSAGE_OUT | \
                                     PD_PENDING_DATA_TRANSFER      | \
                                     PD_PARITY_ERROR_LOGGED        | \
                                     PD_EXPECTING_QUEUE_TAG        | \
                                     PD_BUFFERED_DATA              | \
                                     PD_BUFFERED_BLAST_DATA        | \
                                     PD_ONE_BYTE_XFER              | \
                                     PD_DATA_OVERRUN               | \
                                     PD_TAGGED_SELECT              | \
                                     PD_REQUEST_POSTING            | \
                                     PD_DISCONNECT_EXPECTED          \
                                     )

//
// The largest SCSI bus message expected.
//
#define MESSAGE_BUFFER_SIZE     8

//
// Retry count limits.
//
#define RETRY_SELECTION_LIMIT   1
#define RETRY_ERROR_LIMIT       2
#define MAX_INTERRUPT_COUNT     64

//
// Bus and chip states.
//
typedef enum _ADAPTER_STATE {
    BusFree,
    AttemptingSelect,
    CommandComplete,
    CommandOut,
    DataTransfer,
    DisconnectExpected,
    MessageAccepted,
    MessageIn,
    MessageOut
} ADAPTER_STATE, *PADAPTER_STATE;

typedef struct _POS_DATA {
    USHORT AdapterId;
    UCHAR OptionData1;
    UCHAR OptionData2;
    UCHAR OptionData3;
    UCHAR OptionData4;
} POS_DATA, *PPOS_DATA;

//
// Definition for I/O control status
//
#define IO_STATUS_PENDING        0x00
#define IO_STATUS_SUCCESS        0x01
#define IO_STATUS_ERROR          0x04

//
// I/O control function codes
//
#define POWER_MANAGE_FUNCTION    0x00
#define ACTIVE_NEGATION_FUNCTION 0x01
#define CPU_WORK_MODE_FUNCTION   0x02
#define DMA_SUPPORT_FUNCTION     0x03
#define PCI_MECHANISM_FUNCTION   0x04
#define PARITY_OPTION_FUNCTION   0x05
#define REAL_MODE_FLAG_FUNCTION  0x06

//
// Prototype for I/O control function callback routine
//
typedef BOOLEAN (*PPWR_INTERRUPT) (IN PVOID DeviceExtension);
typedef BOOLEAN (*PCPU_MODE_CHANGE) (IN PVOID DeviceExtension, IN BOOLEAN ResetFlag);

//
// DMA I/O control field definition
//
typedef struct _DMA_SG {
    USHORT PreferredDMA;
    USHORT Status;
    USHORT SupportedDMA;
} DMA_SG;

//
// DMA I/O control field DMA definition
//
#define IO_DMA_METHOD_LINEAR    0x01
#define IO_DMA_METHOD_MDL       0x02
#define IO_DMA_METHOD_S_G_LIST  0x04

#ifdef OTHER_OS

//
// DMA I/O control field status definition
//
#define IO_PASS  0x00
#define IO_FAIL  0x01

//
// Data structure passed by I/O control function call
//
typedef struct _AMD_IO_CONTROL {
    SRB_IO_CONTROL IOControlBlock;
    union _AMD_SPECIFIC_FIELD {
      PPWR_INTERRUPT IO_PwrMgmtInterrupt; // Power managament callback pointer
      PCPU_MODE_CHANGE CPU_ModeChange;    // CPU mode change callback pointer
      BOOLEAN EnableActiveNegation;       // Active negation flag
      DMA_SG SgMode;                      // Scather/Gather method
      UCHAR PciMechanism;                 // PCI Mechanism
      BOOLEAN EnableParity;               // Disable parity check
    } AMD_SPECIFIC_FIELD, *PAMD_SPECIFIC_FIELD;
} AMD_IO_CONTROL, *PAMD_IO_CONTROL;

#endif

//
// AMD 53c94 specific port driver logical unit flags.
//
#define PD_SYNCHRONOUS_NEGOTIATION_DONE    0x0001
#define PD_DO_NOT_NEGOTIATE                0x0002
#define PD_STATUS_VALID                    0x0004
#define PD_DO_NOT_CHECK_TRANSFER_LENGTH    0x0008
#define PD_INITIATE_RECOVERY               0x0010
#define PD_QUEUED_COMMANDS_EXECUTING       0x0020
#define PD_SYNCHRONOUS_NEGOTIATION         0x0040
#define PD_SYNCHRONOUS_RENEGOTIATION       0x0080

//
// The following defines specify masks which are used to clear flags when
// specific events occur, such as reset or command complete.
//
#define PD_LU_COMPLETE_MASK ( PD_STATUS_VALID                   | \
                              PD_DO_NOT_CHECK_TRANSFER_LENGTH   | \
                              PD_INITIATE_RECOVERY                \
                              )

#define PD_LU_RESET_MASK ( PD_SYNCHRONOUS_NEGOTIATION_DONE  | \
                           PD_STATUS_VALID                  | \
                           PD_DO_NOT_CHECK_TRANSFER_LENGTH  | \
                           PD_QUEUED_COMMANDS_EXECUTING     | \
                           PD_INITIATE_RECOVERY             | \
                           PD_SYNCHRONOUS_RENEGOTIATION     | \
                           PD_SYNCHRONOUS_NEGOTIATION         \
                           )

//
// AMD 53c94 specific port driver SRB extension.
//
typedef struct _SRB_EXTENSION {
    ULONG SrbExtensionFlags;
    ULONG SavedSGTEntryOffset;         // Entry offset for a SG entry
    PULONG SavedSGEntry;               // Scather/Gather table pointer
    PULONG SavedSGPhyscialEntry;       // Saved SG physical entry for MDL input
    ULONG SavedDataPointer;            // Data pointer for NON-SG system
    ULONG SavedDataLength;             // Number of data need to be transferred
    ULONG SavedDataTransferred;        // Saved data transfered
    ULONG TotalDataTransferred;        // Number of data transferred
}SRB_EXTENSION, *PSRB_EXTENSION;

#define SRB_EXT(x) ((PSRB_EXTENSION)(x->SrbExtension))

//
// AMD 53c94 specific port driver logical unit extension.
//
typedef struct _SPECIFIC_LOGICAL_UNIT_EXTENSION {
    USHORT LuFlags;
    UCHAR SynchronousPeriod;
    UCHAR SynchronousOffset;
    UCHAR RetryCount;
    SCSI_CONFIGURATION3 Configuration3;
    SCSI_CONFIGURATION4 Configuration4;
    PSCSI_REQUEST_BLOCK ActiveLuRequest;
    PSCSI_REQUEST_BLOCK ActiveSendRequest;
}SPECIFIC_LOGICAL_UNIT_EXTENSION, *PSPECIFIC_LOGICAL_UNIT_EXTENSION;

//
// Flags used for system information
//
typedef struct _SYS_FLAGS {
    USHORT DMAMode:4;           // DMA working mode (MDL / S/G)
    USHORT MachineType:1;       // Machine type (Compaq / Non-Compaq)
    USHORT CPUMode:1;           // CPU working mode (Real / Pretect)
    USHORT ChipType:1;          // Chip type
    USHORT EnableSync:1;        // User flag - enable synchonous transfer
    USHORT EnableTQ:1;          // User flag - enable tagged queuing
    USHORT EnablePrity:1;       // User flag - enable SCSI bus parity check
    USHORT Reserved:6;          // Reserved bits
} SYS_FLAGS, *PSYS_FLAGS;

//
// Machine information
//
typedef enum _MACHINE_TYPES {
    COMPAQ,
    NON_COMPAQ
} MACHINE_TYPES, *PMACHINE_TYPES;

//
// CPU mode information
//
typedef enum _CPU_MODES {
    REAL_MODE,
    PROTECTION_MODE
} CPU_MODES, *PCPU_MODES;

//
// Define the types of chips this driver will support.
//
typedef enum _CHIP_TYPES {
    Fas216,
    Fas216Fast
}CHIP_TYPES, *PCHIP_TYPES;

//
// AMD 53c94 specific port driver device object extension.
//
typedef struct _SPECIFIC_DEVICE_EXTENSION {
    ULONG AdapterFlags;
    ADAPTER_STATE AdapterState;         // Current state of the adapter
    PREG_GROUP Adapter;                 // Address of the GOLDEN GATE adapter
    SCSI_STATUS AdapterStatus;          // Saved status register value
    SCSI_SEQUENCE_STEP SequenceStep;    // Saved sequence step register value
    SCSI_INTERRUPT AdapterInterrupt;    // Saved interrupt status register
    UCHAR AdapterBusId;                 // This adapter's SCSI bus ID
    UCHAR AdapterBusIdMask;             // This adapter's SCSI bus ID bit mask
    UCHAR MessageBuffer[MESSAGE_BUFFER_SIZE]; // SCSI bus message buffer
    UCHAR MessageCount;                 // Count of bytes in message buffer
    UCHAR MessageSent;                  // Count of bytes sent to target
    UCHAR ClockSpeed;                   // Chip clock speed in megahetrz.
    UCHAR TargetId;                     // Saved target id.
    UCHAR Lun;                          // Saved lun id.
    SCSI_CONFIGURATION3 Configuration3;
    SCSI_CONFIGURATION4 Configuration4;
    PULONG ActiveSGEntry;               // Scather/Gather table pointer
    ULONG ActiveSGTEntryOffset;         // Entry offset for a SG entry
    ULONG ActiveDataPointer;            // Data buffer pointer for NON-SG system
    ULONG ActiveDataLength;             // Total number of data need to be transferred
    ULONG ActiveDMAPointer;             // DMA start transfer address
    ULONG ActiveDMALength;              // Number of data to be transferred by DMA
    ULONG TempPhysicalBuf;              // Temporay buffer physical address
    ULONG TempLinearBuf;                // Temporay buffer linear address
    UCHAR dataPhase;                    // Data phase
//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      Fix warnings for 'assignment type mismatch' and 'incompatible types..'.
//
#ifdef SOLARIS
    PPWR_INTERRUPT powerDownPost;       // Power down post
#else   // SOLARIS
    void (*powerDownPost) (struct _SPECIFIC_DEVICE_EXTENSION *);  // Power down post
#endif  // SOLARIS


#ifdef OTHER_OS
//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      Fix warnings for 'assignment type mismatch' and 'incompatible types..'.
//
#ifdef SOLARIS
    PCPU_MODE_CHANGE cpuModePost;       // CPU mode post
#else   // SOLARIS
    void (*cpuModePost) (struct _SPECIFIC_DEVICE_EXTENSION *, BOOLEAN);// CPU mode post
#endif  // SOLARIS


#endif

    PSCSI_REQUEST_BLOCK ActiveLuRequest;// Pointer to the acitive request.
    PSPECIFIC_LOGICAL_UNIT_EXTENSION ActiveLogicalUnit;
    PSCSI_REQUEST_BLOCK NextSrbRequest; // Pointer to the next SRB to process.

#ifndef T1
#ifndef DISABLE_MDL

    PMDL MdlPointer;                    // Pointer to MDL
    PMDL MdlPhysicalPointer;            // Pointer to physical MDL address

#endif
#endif

    PPCI_CONFIG_REGS PciConfigBase;     // PCI configuration register mapped base
    PCONFIG_SPACE_HEADER PciMappedBase; // PCI I/O 0xc000 mapped base
    CONFIG_BLOCK PciConfigInfo;         // PCI device information structure
    UCHAR TargetInitFlags;              // Flags for target phase initialization
    SYS_FLAGS SysFlags;                 // System flags

#ifdef DISABLE_SREG

    USHORT SCSIState[8];               // Target information

#endif

#ifdef SOLARIS
//
// Specific mods for Solaris 2.4 pcscsi driver
//
//	Need a hook to get to the instance-specific data structures
//
    struct pcscsi_blk *pcscsi_blk_p;	// Pointer to the instance-specific
					// _blk struct
#endif	// SOLARIS

} SPECIFIC_DEVICE_EXTENSION, *PSPECIFIC_DEVICE_EXTENSION;


//
// Define the synchrouns data transfer parameters structure.
//
typedef struct _SYNCHRONOUS_TYPE_PARAMETERS {
    UCHAR MaximumPeriodCyles;
    UCHAR SynchronousPeriodCyles;
    UCHAR InitialRegisterValue;
}SYNCHRONOUS_TYPE_PARAMETERS, *PSYNCHRONOUS_TYPE_PARAMETERS;

//
// SCSI Protocol Chip Control read and write macros.
//
#define SCSI_READ(BaseAddr, Register) \
    ScsiPortReadPortUchar((PUCHAR)&((BaseAddr)->ScsiRegs.ReadRegisters.Register))
#define SCSI_WRITE(BaseAddr, Register, Value) \
    ScsiPortWritePortUchar((PUCHAR)&((BaseAddr)->ScsiRegs.WriteRegisters.Register), (Value))
#define DMA_READ(BaseAddr, Register) \
    ScsiPortReadPortUlong(&((BaseAddr)->DmaCcbRegs.Register))
#define DMA_WRITE(BaseAddr, Register, Value) \
    ScsiPortWritePortUlong(&((BaseAddr)->DmaCcbRegs.Register), (Value))

//
// Scatter/gather table parameter macros
//
#define SGPointer *DeviceExtension->ActiveSGEntry
#define SGCount   *(DeviceExtension->ActiveSGEntry + 1)
		  
//
// Masks
//
#define BYTE_MASK  0x0ff
#define PAGE_MASK	 0x0fff
#define WORD_MASK  0x0ffff
#define PAGE_SIZE  0x1000

//
// Access length
//
typedef enum _ACCESS_LENGTH {
    OneByte,
    OneWord,
    OneDWord
} ACCESS_LENGTH, *PACCESS_LENGTH;

//
// Access type
//
typedef enum _ACCESS_TYPE {
    ReadPCI,
    WritePCI
} ACCESS_TYPE, *PACCESS_TYPE;

//
// PCI BIOS interface definitions
//
// Functions
//
#define PCI_FUNCTION_ID          0xB1
#define PCI_BIOS_PRESENT         0x01
#define FIND_PCI_DEVICE          0x02
#define FIND_PCI_CLASS_CODE      0x03
#define GENERATE_SPECIAL_CYCLE   0x06
#define READ_CONFIG_BYTE         0x08
#define READ_CONFIG_WORD         0x09
#define READ_CONFIG_DWORD        0x0A
#define WRITE_CONFIG_BYTE        0x0B
#define WRITE_CONFIG_WORD        0x0C
#define WRITE_CONFIG_DWORD       0x0D
#define FOR_32_BIT               0x80

//
// PCI command register initialization value
//
#define PCICommandData           0x1c5

//
// PCI BIOS status
//
#define SUCCESSFUL               0x00
#define FUNC_NOT_SUPPORTED       0x81
#define BAD_VENDOR_ID            0x83
#define DEVICE_NOT_FOUND         0x86
#define BAD_REGISTER_NUMBER      0x87

typedef enum _PCI_BIOS_STATUS {
    PCIBiosExist,
    PCIBiosNotExist,
    PCIDeviceNotExist,
    GotPCIInformation
} PCI_BIOS_STATUS, *PPCI_BIOS_STATUS;

#define OVER_COUNT 25
#endif
