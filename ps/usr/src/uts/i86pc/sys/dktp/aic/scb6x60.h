/*
 * Copyright (c) 1993-94 Sun Microsystems, Inc.
 */

#pragma ident   "@(#)scb6x60.h 1.3     94/09/08 SMI"

/* $Header:   G:/source/code/aic-6x6x/him/scb6x60.h_v   1.3   28 Sep 1993 15:19:48   PV_ADMIN  $ */

/* Adaptec SCB Manager definitions used by both the OS specific code that
   invokes the Hardware Interface Module (HIM) and the 6X60 device driver code
   itself.  Note that because there is ONLY source level compatability between
   these two components, there is considerable flexibility in the definition
   of OS specific data structure fields, functions and procedures to maximize
   implementation efficiency.  The use of macros for 6X60 register IO
   functions are a good example of this approach.  For each of the other data
   structures used, you may ADD new fields at will. */

#define REGISTER register
#define STATIC static

#if !defined(DBG)
#define DBG 0
#endif

#if (DBG)
#define PRIVATE_FUNC
#else
#define PRIVATE_FUNC STATIC
#endif


#if defined(EZ_SCSI)
typedef BOOL BOOLEAN;
typedef unsigned long PHYSICAL_ADDRESS;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif

#if defined(NETWARE)
#define FALSE 0
#define TRUE !FALSE
#define VOID void
typedef unsigned char BOOLEAN;
typedef unsigned long PHYSICAL_ADDRESS;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif

#if defined(OS_2)
typedef unsigned char BOOLEAN;
typedef unsigned long PHYSICAL_ADDRESS;
#endif

#if defined(SCO_UNIX)
#define FALSE 0
#define TRUE !FALSE
#define VOID void
typedef unsigned char BOOLEAN;
typedef char CHAR;
typedef int INT;
typedef unsigned long PHYSICAL_ADDRESS;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif

#if defined(USL_UNIX)
#define FALSE 0
#define TRUE !FALSE
#define VOID void
typedef unsigned char BOOLEAN;
typedef char CHAR;
typedef int INT;
typedef unsigned long PHYSICAL_ADDRESS;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif

#if defined(SOLARIS)
#undef TRUE
#define FALSE 0
#define TRUE !FALSE
#define VOID void
typedef unsigned char BOOLEAN;
typedef char CHAR;
typedef int INT;
typedef unsigned long PHYSICAL_ADDRESS;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif

/* The SCSI Control Block (SCB), the basic IO request packet used to
   communicate work from the OS specific code to the 6X60 HIM and to
   return completion status. */

typedef struct _SCB {
   /* This MUST be the first element! */
   struct _SCB *chain;                  
   ULONG length;
   VOID *osRequestBlock;
   struct _SCB *linkedScb;
   UCHAR function;
   UCHAR scbStatus;
   USHORT flags;
   UCHAR targetStatus;
   UCHAR scsiBus;
   UCHAR targetID;
   UCHAR lun;
   UCHAR queueTag;
   UCHAR tagType;
   UCHAR cdbLength;
   UCHAR senseDataLength;
   UCHAR *cdb;
   UCHAR *senseData;
   UCHAR *dataPointer;
   ULONG dataLength;
   ULONG dataOffset;
   PHYSICAL_ADDRESS segmentAddress;
   ULONG segmentLength;
   ULONG transferLength;
   ULONG transferResidual;
   ULONG provisionalTransfer;
#if defined(SCO_UNIX)
   CHAR use_flag;
   CHAR smad_num;
   INT sgCount;
#endif
} SCB;

/* Values defined for the SCB 'function' field. */

#if defined(NETWARE)
#define SCB_IO_CONTROL          0x00
#define SCB_EXECUTE             0x02
#define SCB_RECEIVE_EVENT       0x03
#define SCB_BUS_DEVICE_RESET    0x04
#define SCB_SHUTDOWN            0x07
#define SCB_FLUSH               0x08
#define SCB_ABORT_REQUESTED     0x10
#define SCB_RELEASE_RECOVERY    0x11
#define SCB_SCSI_RESET          0x12
#define SCB_TERMINATE_IO        0x14
#define SCB_SUSPEND_IO          0x81
#define SCB_RESUME_IO           0x82
#endif

#if    defined(CHICAGO) || defined(EZ_SCSI) || defined(OS_2) \
    || defined(SCO_UNIX) || defined(USL_UNIX) || defined(WINDOWS_NT) \
	|| defined(SOLARIS)
#define SCB_EXECUTE             0x00
#define SCB_IO_CONTROL          0x02
#define SCB_RECEIVE_EVENT       0x03
#define SCB_SHUTDOWN            0x07
#define SCB_FLUSH               0x08
#define SCB_ABORT_REQUESTED     0x10
#define SCB_RELEASE_RECOVERY    0x11
#define SCB_SCSI_RESET          0x12
#define SCB_BUS_DEVICE_RESET    0x13
#define SCB_TERMINATE_IO        0x14
#define SCB_SUSPEND_IO          0x81
#define SCB_RESUME_IO           0x82
#endif

/* Values defined for the SCB 'status' field. */

#define SCB_PENDING                     0x00
#define SCB_COMPLETED_OK                0x01
#define SCB_ABORTED                     0x02
#define SCB_ABORT_FAILURE               0x03
#define SCB_ERROR                       0x04
#define SCB_BUSY                        0x05
#define SCB_INVALID_SCSI_BUS            0x07
#define SCB_TIMEOUT                     0x09
#define SCB_SELECTION_TIMEOUT           0x0A
#define SCB_MESSAGE_REJECTED            0x0D
#define SCB_SCSI_BUS_RESET              0x0E
#define SCB_PARITY_ERROR                0x0F
#define SCB_REQUEST_SENSE_FAILURE       0x10
#define SCB_DATA_OVERRUN                0x12
#define SCB_BUS_FREE                    0x13
#define SCB_PROTOCOL_ERROR              0x14
#define SCB_INVALID_LENGTH              0x15
#define SCB_INVALID_LUN                 0x20
#define SCB_INVALID_TARGET_ID           0x21
#define SCB_INVALID_FUNCTION            0x22
#define SCB_ERROR_RECOVERY              0x23
#define SCB_TERMINATED                  0x24
#define SCB_TERMINATE_IO_FAILURE        0x25

#define SCB_SENSE_DATA_VALID            0x80

#define SCB_STATUS(status) ((status) & ~SCB_SENSE_DATA_VALID)

/* Values defined for the SCB 'flags' field */

#define SCB_ENABLE_TAGGED_QUEUING       0x0002
#define SCB_DISABLE_DISCONNECT          0x0004
#define SCB_DISABLE_NEGOTIATIONS        0x0008
#define SCB_DISABLE_AUTOSENSE           0x0020
#define SCB_DATA_IN                     0x0040
#define SCB_DATA_OUT                    0x0080
#define SCB_NO_QUEUE_FREEZE             0x0100
#define SCB_ENABLE_CACHE                0x0200
#define SCB_VIRTUAL_SCATTER_GATHER      0x4000
#define SCB_DISABLE_DMA                 0x8000

/* Values defined for logging errors to the SCB Manager. */

#define SCB_ERROR_PARITY                0x0001
#define SCB_ERROR_UNEXPECTED_DISCONNECT 0x0002
#define SCB_ERROR_INVALID_RESELECTION   0x0003
#define SCB_ERROR_BUS_TIMEOUT           0x0004
#define SCB_ERROR_PROTOCOL              0x0005
#define SCB_ERROR_HOST_ADAPTER          0x0006
#define SCB_ERROR_REQUEST_TIMEOUT       0x0007
#define SCB_WARNING_NO_INTERRUPTS       0x0008
#define SCB_ERROR_FIRMWARE              0x0009
#define SCB_WARNING_FIRMWARE            0x000A
#define SCB_WARNING_DMA_HANG            0x8001

/* The Logical Unit Control Block (LUCB) structure.  It's principal use is to
   provide queues for the management of active and pending SCSI requests
   (SCB's). */

typedef struct {
   BOOLEAN busy;
   SCB *queuedScb;
   SCB *activeScb;
} LUCB;

/* Data structure to record debugging trace information (when enabled) */

typedef struct {
   UCHAR sequence;
   UCHAR event;
   UCHAR scsiPhase;
   UCHAR busID;
   USHORT data[2];
   UCHAR scsiSig;
   UCHAR sStat0;
   UCHAR sStat1;
   UCHAR sStat2;
   UCHAR dmaCntrl0;
   UCHAR dmaStat;
   UCHAR fifoStat;
   UCHAR currentState;
} TRACE_LOG;

/* Values defined for the Trace Log 'event' field */

#define BUS_FREE 0x00
#define TARGET_REQ 0x01
#define DATA_PHASE_PIO 0x02
#define DATA_PHASE_DMA 0x03
#define QUIESCE_DMA 0x04
#define UPDATE_DATA_POINTER 0x05
#define MESSAGE_IN 0x10
#define NEGOTIATE_SDTR 0x11
#define BITBUCKET_AND_ABORT 0x12
#define RESET_DEVICE 0x17
#define RESET_BUS 0x1F
#define INITIATE_IO 0x40
#define RESELECTION 0x41
#define SELECTION 0x42
#define BUS_RESET 0x4F
#define ISR 0x07F
#define INTERRUPT 0x0FF

/* The 6X60 IO registers are viewed as an array of UCHAR, USHORT or ULONG as
   appropriate and are referenced by an IO space "pointer" to this array. */

typedef union {
   USHORT ioPort[32];
   struct {
      USHORT scsiSeq;
      USHORT sXfrCtl0;
      USHORT sXfrCtl1;
      USHORT scsiSig;
      USHORT scsiRate;
      union {USHORT scsiID; USHORT selID;} u1;
      USHORT scsiDat;
      USHORT scsiBus;
      USHORT stCnt0;
      USHORT stCnt1;
      USHORT stCnt2;
      union {USHORT clrSInt0; USHORT sStat0;} u2;
      union {USHORT clrSInt1; USHORT sStat1;} u3;
      USHORT sStat2;
      union {USHORT scsiTest; USHORT sStat3;} u4;
      union {USHORT clrSErr; USHORT sStat4;} u5;
      USHORT sIMode0;
      USHORT sIMode1;
      USHORT dmaCntrl0;
      USHORT dmaCntrl1;
      USHORT dmaStat;
      USHORT fifoStat;
      USHORT dmaData;
      USHORT ioPortX57;
      union {USHORT brstCntrl; USHORT dmaData32;} u6;
      USHORT ioPortX59;
      USHORT portA;
      USHORT portB;
      USHORT rev;
      USHORT stack;
      USHORT test;
      USHORT ID;
   } top;
} AIC6X60_REG;

/* The Host Adapter Control Block (HACB) structure used to record the current
   state of SCSI bus "conversations" in progress. */

typedef struct _HACB {
   ULONG length;
   AIC6X60_REG *baseAddress;
   UCHAR scsiPhase;
   UCHAR ownID;
   UCHAR busID;
   UCHAR lun;
   union {
      UCHAR adapterConfiguration;
      struct {
	 BOOLEAN fastSCSI:1;
	 BOOLEAN defaultConfiguration:1;
	 BOOLEAN initialReset:1;
	 BOOLEAN noDisconnect:1;
	 BOOLEAN checkParity:1;
	 BOOLEAN initiateSDTR:1;
	 BOOLEAN useDma:1;
	 BOOLEAN synchronous:1;
      }ac_s;
   }ac_u;
   union {
      UCHAR currentState;
      struct {
	 BOOLEAN bitBucket:1;
	 BOOLEAN disconnectOK:1;
	 BOOLEAN dmaActive:1;
	 BOOLEAN msgParityError:1;
	 BOOLEAN parityError:1;
	 BOOLEAN queuesFrozen:1;
	 BOOLEAN deferredIsrActive:1;
	 BOOLEAN isrActive:1;
      }cs_s;
   }cs_u;
   ULONG disableINT;
   SCB *deferredScb;
   SCB *eligibleScb;
   SCB *queueFreezeScb;
   SCB *resetScb;
   union {
      ULONG nexus[  (  2 * sizeof(VOID *) + sizeof(PHYSICAL_ADDRESS)
		     + 3 * sizeof(ULONG) + 16 * sizeof(UCHAR))
		  / sizeof(ULONG)];
      struct {
	 SCB *activeScb;
	 UCHAR *dataPointer;
	 ULONG dataLength;
	 PHYSICAL_ADDRESS segmentAddress;
	 ULONG segmentLength;
	 ULONG dataOffset;
	 UCHAR msgOutLen;
	 UCHAR msgOut[7];
	 UCHAR msgInLen;
	 UCHAR msgIn[7];
      }ne_s;
   }ne_u;
   UCHAR targetStatus;
   UCHAR reservedForAlignment1;
   UCHAR syncCycles[8];
   UCHAR syncOffset[8];
   ULONG cQueuedScb;
   ULONG cActiveScb;
   UCHAR negotiateSDTR;
   struct {
      UCHAR extMsgCode;
      UCHAR extMsgLength;
      UCHAR extMsgType;
      UCHAR transferPeriod;
      UCHAR reqAckOffset;
   } sdtrMsg;
   UCHAR requestSenseCdb[6];
   UCHAR sStat0;
   UCHAR maskedSStat0;
   UCHAR sStat1;
   UCHAR maskedSStat1;
   USHORT selectTimeLimit;
   UCHAR sXfrCtl1Image;
   BOOLEAN irqConnected;
   UCHAR clockPeriod;
   UCHAR IRQ;
   UCHAR dmaChannel;
   UCHAR revision;
   UCHAR dmaBusOnTime;
   UCHAR dmaBusOffTime;
   ULONG signature;
   ULONG scsiCount;
#if defined(CHICAGO) || defined(WINDOWS_NT)
   SCB internalScb;
#endif
#if (DBG_TRACE)
   ULONG traceCount;
   ULONG traceIndex;
   TRACE_LOG traceLog[128];
   BOOLEAN traceEnabled;
#endif
#if defined(EZ_SCSI) || defined(SOLARIS)
   LUCB lucb[8][8];
#endif
#if defined(NETWARE) || defined(SCO_UNIX) 
   LUCB lucb[64];
#endif
} HACB;

#define BUS_FREE_PHASE 0xFF             /* Special token */
#define DISCONNECTED 0xFF

#define WATCHDOG_DMA_HANG 50000         /* Microseconds before checking DMA */
#define WATCHDOG_POLL_IRQ 10000         /* Microseconds before polling IRQ */

/* Public functions and procedures used for communications between the OS
   specific code and the 6X60 HIM.  Some are defined within the HIM code
   (HIM6X60.C) and the others are the responsibility of the implementor for a
   particular OS/hardware platform.  Note that in cases where the
   functionality is a superset of that provided by the OS (e.g. NETWARE, OS/2
   and Unix do not support the concept of a "deferred ISR") it is possible to
   use macros to eliminate the need to implement the function in the OS
   specific code.*/

UCHAR HIM6X60AbortSCB(HACB *hacb, SCB *scb, SCB *scbToAbort);

VOID HIM6X60AssertINT(HACB *hacb);

#if defined(EZ_SCSI)
#define HIM6X60CompleteSCB(hacb, scb)
#else
VOID HIM6X60CompleteSCB(HACB *hacb, SCB *scb);
#endif

#if   defined(EZ_SCSI) || defined(NETWARE) || defined(OS_2) \
   || defined(SCO_UNIX) || defined(USL_UNIX) || defined(SOLARIS)
#define HIM6X60DeferISR(hacb, deferredProcedure) deferredProcedure(hacb)
#elif defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60DeferISR(hacb, deferredProcedure) \
   ScsiPortNotification(CallEnableInterrupts, hacb, deferredProcedure)
#else
VOID HIM6X60DeferISR(HACB *hacb, VOID (*deferredProcedure)(HACB *hacb));
#endif

#if defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60Delay(microseconds) ScsiPortStallExecution(microseconds)
#elif defined(SCO_UNIX)
#define HIM6X60Delay(microseconds) suspend((microseconds + 99) /100)
#else
VOID HIM6X60Delay(ULONG microseconds);
#endif

VOID HIM6X60DisableINT(HACB *hacb);

VOID HIM6X60DmaProgrammed(HACB *hacb);

BOOLEAN HIM6X60EnableINT(HACB *hacb);

/* Modified HIM6X60Event routine to have a third argument */
VOID HIM6X60Event(HACB *hacb, UCHAR event, int val);

#if   defined(EZ_SCSI) || defined(NETWARE) || defined(OS_2) \
   || defined(SCO_UNIX) || defined(USL_UNIX) || defined(SOLARIS)
#define HIM6X60ExitDeferredISR(hacb, deferredProcedure) deferredProcedure(hacb)
#elif defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60ExitDeferredISR(hacb, deferredProcedure) \
   ScsiPortNotification(CallDisableInterrupts, hacb, deferredProcedure)
#else
VOID HIM6X60ExitDeferredISR(HACB *hacb, VOID (*deferredProcedure)(HACB *hacb));
#endif

BOOLEAN HIM6X60FindAdapter(AIC6X60_REG *baseAddress);

#if defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60FlushDMA(hacb) ScsiPortFlushDma(hacb)
#elif defined(EZ_SCSI)
#define HIM6X60FlushDMA(hacb)
#else
VOID HIM6X60FlushDMA(HACB *hacb);
#endif

BOOLEAN HIM6X60GetConfiguration(HACB *hacb);

#if defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60GetLUCB(hacb, scsiBus, targetID, lun) \
   ScsiPortGetLogicalUnit(hacb, scsiBus, targetID, lun)
#elif defined(EZ_SCSI)
#define HIM6X60GetLUCB(hacb, scsiBus, targetID, lun) &hacb->lucb[targetID][lun]
#else
LUCB *HIM6X60GetLUCB(HACB *hacb, UCHAR scsiBus, UCHAR targetID, UCHAR lun);
#endif

#if defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60GetPhysicalAddress(hacb, scb, virtualAddress, bufferOffset, \
				  length) \
   ScsiPortConvertUlongToPhysicalAddress(0); *length = 0x00FFFFFF
#elif defined(EZ_SCSI)
#define HIM6X60GetPhysicalAddress(hacb, scb, virtualAddress, bufferOffset, \
				  length) \
   (PHYSICAL_ADDRESS) 0; *length = 0x00FFFFFF;
#else
PHYSICAL_ADDRESS HIM6X60GetPhysicalAddress(HACB *hacb, SCB *scb,
					   VOID *virtualAddress,
					   ULONG bufferOffset, ULONG *length);
#endif

USHORT HIM6X60GetStackContents(HACB *hacb, VOID *stackContents,
			       USHORT maxStackSize);

#if defined(CHICAGO) || defined(EZ_SCSI) || defined(NETWARE) || defined(WINDOWS_NT)
#define HIM6X60GetVirtualAddress(hacb, scb, virtualAddress, bufferOffset, \
				 length) \
   virtualAddress; *length = *length
#else
VOID *HIM6X60GetVirtualAddress(HACB *hacb, SCB *scb, VOID *virtualAddress,
			       ULONG bufferOffset, ULONG *length);
#endif

BOOLEAN HIM6X60Initialize(HACB *hacb);

BOOLEAN HIM6X60IRQ(HACB *hacb);

BOOLEAN HIM6X60ISR(HACB *hacb);

#if defined(EZ_SCSI) || defined(SCO_UNIX) || defined(USL_UNIX)  \
	|| defined(SOLARIS)
#define HIM6X60LogError(hacb, scb, scsiBus, targetID, lun, errorClass, errorID)
#else
VOID HIM6X60LogError(HACB *hacb, SCB *scb, UCHAR scsiBus, UCHAR targetID,
		     UCHAR lun, USHORT errorClass, USHORT errorID);
#endif

#if defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60MapDMA(hacb, scb, virtualAddress, bufferOffset, length, \
		      memoryWrite) \
   ScsiPortIoMapTransfer(hacb, scb->osRequestBlock, virtualAddress, length)
#elif defined(EZ_SCSI)
#define HIM6X60MapDMA(hacb, scb, virtualAddress, bufferOffset, length, \
		      memoryWrite)
#else
VOID HIM6X60MapDMA(HACB *hacb, SCB *scb, VOID *virtualAddress,
		   PHYSICAL_ADDRESS physicalAddress, ULONG length,
		   BOOLEAN memoryWrite);
#endif

UCHAR HIM6X60QueueSCB(HACB *hacb, SCB *scb);

BOOLEAN HIM6X60ResetBus(HACB *hacb, UCHAR scsiBus);

UCHAR HIM6X60TerminateSCB(HACB *hacb, SCB *scb, SCB *scbToTerminate);

#if defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60Watchdog(hacb, watchdogProcedure, microseconds) \
   ScsiPortNotification(RequestTimerCall, hacb, watchdogProcedure, \
			microseconds)
#elif defined(EZ_SCSI)
#define HIM6X60Watchdog(hacb, watchdogProcedure, microseconds)
#else
VOID HIM6X60Watchdog(HACB *hacb, VOID (*watchdogProcedure)(HACB *hacb),
		     ULONG microseconds);
#endif

/* 'Event' definitions to use with HIM6X60Event */

#define EVENT_SCSI_BUS_RESET 0x02

/* Macros for convenience */

#define LAST(array) ((sizeof(array) / sizeof(array[0])) - 1)
#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#if (DBG_TRACE)
#define DISABLE_TRACE hacb->traceEnabled = FALSE
#define ENABLE_TRACE hacb->traceEnabled = TRUE
#define TRACE(hacb, event, data0, data1) debugTrace(hacb, event, data0, data1)
#else
#define DISABLE_TRACE
#define ENABLE_TRACE
#define TRACE(hacb, event, data0, data1)
#endif

/* OS specific macros to provide access to the 6X60 IO registers and to allow
   the manipulation of a PHYSICAL_ADDRESS type which may vary in different
   machine and OS environments.  The conventions adopted for conditional
   compilation for supported operating systems are (at present) NETWARE,
   WINDOWS_NT, OS_2, SCO_UNIX and USL_UNIX. */

#if defined(CHICAGO) || defined(WINDOWS_NT)

#define BLOCKINDWORD(port, buffer, count) \
   ScsiPortReadPortBufferUlong((ULONG *) &hacb->baseAddress->port, \
			       (ULONG *) buffer, count)

#define BLOCKINPUT(port, buffer, count) \
   ScsiPortReadPortBufferUchar(&hacb->baseAddress->port, buffer, count)

#define BLOCKINWORD(port, buffer, count) \
   ScsiPortReadPortBufferUshort((USHORT *) &hacb->baseAddress->port, \
				(USHORT *) buffer, count)

#define BLOCKOUTDWORD(port, buffer, count) \
   ScsiPortWritePortBufferUlong((ULONG *) &hacb->baseAddress->port, \
				(ULONG *) buffer, count)

#define BLOCKOUTPUT(port, buffer, count) \
   ScsiPortWritePortBufferUchar(&hacb->baseAddress->port, buffer, count)

#define BLOCKOUTWORD(port, buffer, count) \
   ScsiPortWritePortBufferUshort((USHORT *) &hacb->baseAddress->port, \
				 (USHORT *) buffer, count)
#if (DBG)
VOID DbgBreakPoint(VOID);
#define BREAKPOINT DbgBreakPoint()
#else
#define BREAKPOINT
#endif

#define INPUT(port) \
   ScsiPortReadPortUchar(&hacb->baseAddress->port)

#define OUTPUT(port, value) \
   ScsiPortWritePortUchar(&hacb->baseAddress->port, value)

#define PHYSICAL_TO_ULONG(physicalAddress) \
   ScsiPortConvertPhysicalAddressToUlong(physicalAddress)

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count)

#define ZERO_PHYSICAL_ADDRESS(physicalAddress)

#endif

#if defined (EZ_SCSI)

int repinsd(int port, UCHAR *buffer, int count);
int repinsb(int port, UCHAR *buffer, int count);
int repinsw(int port, UCHAR *buffer, int count);
int repoutsd(int port, UCHAR *buffer, int count);
int repoutsb(int port, UCHAR *buffer, int count);
int repoutsw(int port, UCHAR *buffer, int count);

#define BLOCKINDWORD(port, buffer, count) \
   repinsd((int) ((long) &hacb->baseAddress->port), buffer, (int) count)

#define BLOCKINPUT(port, buffer, count) \
   repinsb((int) ((long) &hacb->baseAddress->port), buffer, (int) count)

#define BLOCKINWORD(port, buffer, count) \
   repinsw((int) ((long) &hacb->baseAddress->port), buffer, (int) count)

#define BLOCKOUTDWORD(port, buffer, count) \
   repoutsd((int) ((long) &hacb->baseAddress->port), buffer, (int) count)

#define BLOCKOUTPUT(port, buffer, count) \
   repoutsb((int) ((long) &hacb->baseAddress->port), buffer, (int) count)

#define BLOCKOUTWORD(port, buffer, count) \
   repoutsw((int) ((long) &hacb->baseAddress->port), buffer, (int) count)

#define BREAKPOINT

#define INPUT(port) _inp((int) ((long) &hacb->baseAddress->port))

#define OUTPUT(port, value) _outp((int) ((long) &hacb->baseAddress->port), \
				  (int) (value))

#define PHYSICAL_TO_ULONG(physicalAddress) physicalAddress

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count) \
   physicalAddress += count

#define ZERO_PHYSICAL_ADDRESS(physicalAddress) physicalAddress = 0

#endif

#if defined(NETWARE)

#define BLOCKINDWORD(port, buffer, count) \
   repinsd((ULONG *) &hacb->baseAddress->port, buffer, count)

#define BLOCKINPUT(port, buffer, count) \
   repinsb(&hacb->baseAddress->port, buffer, count)

#define BLOCKINWORD(port, buffer, count) \
   repinsw((USHORT *) &hacb->baseAddress->port, buffer, count)

#define BLOCKOUTDWORD(port, buffer, count) \
   repoutsd((ULONG *) &hacb->baseAddress->port, buffer, count)

#define BLOCKOUTPUT(port, buffer, count) \
   repoutsb(&hacb->baseAddress->port, buffer, count)

#define BLOCKOUTWORD(port, buffer, count) \
   repoutsw((USHORT *) &hacb->baseAddress->port, buffer, count)

#define BREAKPOINT

#define INPUT(port) inp(&hacb->baseAddress->port)

#define OUTPUT(port, value) outp(&hacb->baseAddress->port, value)

#define PHYSICAL_TO_ULONG(physicalAddress) physicalAddress

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count) \
   physicalAddress += count

#define ZERO_PHYSICAL_ADDRESS(physicalAddress) physicalAddress = 0

#endif

#if defined(OS_2)

VOID breakpoint(VOID);
UCHAR inp(UCHAR *port);
VOID outp(UCHAR *port, UCHAR value);
VOID repinsb(UCHAR *port, VOID *buffer, ULONG count);
VOID repinsd(ULONG *port, VOID *buffer, ULONG count);
VOID repinsw(USHORT *port, VOID *buffer, ULONG count);
VOID repoutsb(UCHAR *port, VOID *buffer, ULONG count);
VOID repoutsd(ULONG *port, VOID *buffer, ULONG count);
VOID repoutsw(USHORT *port, VOID *buffer, ULONG count);

#define BLOCKINDWORD(port, buffer, count) \
   repinsd((ULONG *) &hacb->baseAddress->port, buffer, count)

#define BLOCKINPUT(port, buffer, count) \
   repinsb(&hacb->baseAddress->port, buffer, count)

#define BLOCKINWORD(port, buffer, count) \
   repinsw((USHORT *) &hacb->baseAddress->port, buffer, count)

#define BLOCKOUTDWORD(port, buffer, count) \
   repoutsd((ULONG *) &hacb->baseAddress->port, buffer, count)

#define BLOCKOUTPUT(port, buffer, count) \
   repoutsb(&hacb->baseAddress->port, buffer, count)

#define BLOCKOUTWORD(port, buffer, count) \
   repoutsw((USHORT *) &hacb->baseAddress->port, buffer, count)

#if (DBG)
#define BREAKPOINT breakpoint()
#else
#define BREAKPOINT
#endif

#define INPUT(port) inp(&hacb->baseAddress->port)

#define OUTPUT(port, value) outp(&hacb->baseAddress->port, value)

#define PHYSICAL_TO_ULONG(physicalAddress) physicalAddress

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count) \
   physicalAddress += count

#define ZERO_PHYSICAL_ADDRESS(physicalAddress) physicalAddress = 0

#endif

#if defined(SCO_UNIX)

INT inb(INT port);
INT outb(INT port, INT value);
INT repinsd(INT port, CHAR *buffer, INT count);
INT repinsb(INT port, CHAR *buffer, INT count);
INT repinsw(INT port, CHAR *buffer, INT count);
INT repoutsd(INT port, CHAR *buffer, INT count);
INT repoutsb(INT port, CHAR *buffer, INT count);
INT repoutsw(INT port, CHAR *buffer, INT count);

#define BLOCKINDWORD(port, buffer, count) \
   repinsd((INT) &hacb->baseAddress->port, buffer, (INT) count)

#define BLOCKINPUT(port, buffer, count) \
   repinsb((INT) &hacb->baseAddress->port, buffer, (INT) count)

#define BLOCKINWORD(port, buffer, count) \
   repinsw((INT) &hacb->baseAddress->port, buffer, (INT) count)

#define BLOCKOUTDWORD(port, buffer, count) \
   repoutsd((INT) &hacb->baseAddress->port, buffer, (INT) count)

#define BLOCKOUTPUT(port, buffer, count) \
   repoutsb((INT) &hacb->baseAddress->port, buffer, (INT) count)

#define BLOCKOUTWORD(port, buffer, count) \
   repoutsw((INT) &hacb->baseAddress->port, buffer, (INT) count)

#define BREAKPOINT

#define INPUT(port) inb((INT) &hacb->baseAddress->port)

#define OUTPUT(port, value) outb((INT) &hacb->baseAddress->port, (CHAR) value)

#define PHYSICAL_TO_ULONG(physicalAddress) physicalAddress

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count) \
   physicalAddress += count

#define ZERO_PHYSICAL_ADDRESS(physicalAddress) physicalAddress = 0

#endif

#if defined(USL_UNIX) 

INT inb(INT port);
INT outb(INT port, CHAR value);
INT repinsd(INT port, CHAR *buffer, INT count);
INT repinsb(INT port, CHAR *buffer, INT count);
INT repinsw(INT port, CHAR *buffer, INT count);
INT repoutsd(INT port, CHAR *buffer, INT count);
INT repoutsb(INT port, CHAR *buffer, INT count);
INT repoutsw(INT port, CHAR *buffer, INT count);

#define BLOCKINDWORD(port, buffer, count) \
   repinsd((INT) &hacb->baseAddress->top.port, buffer, (INT) count)

#define BLOCKINPUT(port, buffer, count) \
   repinsb((INT) &hacb->baseAddress->top.port, buffer, (INT) count)

#define BLOCKINWORD(port, buffer, count) \
   repinsw((INT) &hacb->baseAddress->top.port, buffer, (INT) count)

#define BLOCKOUTDWORD(port, buffer, count) \
   repoutsd((INT) &hacb->baseAddress->top.port, buffer, (INT) count)

#define BLOCKOUTPUT(port, buffer, count) \
   repoutsb((INT) &hacb->baseAddress->top.port, buffer, (INT) count)

#define BLOCKOUTWORD(port, buffer, count) \
   repoutsw((INT) &hacb->baseAddress->top.port, buffer, (INT) count)

#define BREAKPOINT

#define INPUT(port) inb((INT) hacb->baseAddress->top.port)

#define OUTPUT(port, value) outb((INT) hacb->baseAddress->top.port, (CHAR) value)

#define PHYSICAL_TO_ULONG(physicalAddress) physicalAddress

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count) \
   physicalAddress += count

#define ZERO_PHYSICAL_ADDRESS(physicalAddress) physicalAddress = 0

#endif


/************************** SOLARIS    *************************/

#if defined(SOLARIS)

#define BLOCKINDWORD(port, buffer, count) \
   blockind(hacb->baseAddress->top.port,buffer,(INT)(count))

#define BLOCKINPUT(port, buffer, count) \
   blockinb(hacb->baseAddress->top.port,buffer,(INT) count)

#define BLOCKINWORD(port, buffer, count) \
   blockinw(hacb->baseAddress->top.port,buffer,(INT)(count))

#define BLOCKOUTDWORD(port, buffer, count) \
	blockoutd(hacb->baseAddress->top.port,buffer,(INT) (count))

#define BLOCKOUTPUT(port, buffer, count) \
	blockoutb(hacb->baseAddress->top.port,buffer,(INT) count)

#define BLOCKOUTWORD(port, buffer, count) \
	blockoutw(hacb->baseAddress->top.port,buffer,(INT) (count))

#define BREAKPOINT

#define INPUT(port) inb((INT) hacb->baseAddress->top.port)

#define OUTPUT(port, value) outb((INT) hacb->baseAddress->top.port,(CHAR) value)

#define PHYSICAL_TO_ULONG(physicalAddress) physicalAddress

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count) \
   physicalAddress += count

#define ZERO_PHYSICAL_ADDRESS(physicalAddress) physicalAddress = 0

#endif

#if defined(SOLARIS1)

#define BLOCKINDWORD(port, buffer, count) \
   repinsd((INT)hacb->baseAddress->top.port,(ULONG *)buffer,(INT)(count))

#define BLOCKINPUT(port, buffer, count) \
   repinsb((INT)hacb->baseAddress->top.port,(UCHAR *)buffer,(INT) count)

#define BLOCKINWORD(port, buffer, count) \
   repinsw((INT)hacb->baseAddress->top.port,(USHORT *)buffer,(INT)(count))

#define BLOCKOUTDWORD(port, buffer, count) \
	repoutsd((INT)hacb->baseAddress->top.port,(ULONG *)buffer,(INT) (count))

#define BLOCKOUTPUT(port, buffer, count) \
	repoutsb((INT)hacb->baseAddress->top.port,(UCHAR *)buffer,(INT) count)

#define BLOCKOUTWORD(port, buffer, count) \
	repoutsw((INT)hacb->baseAddress->top.port,(USHORT *)buffer,(INT) (count))

#define BREAKPOINT

#define INPUT(port) inb((INT) hacb->baseAddress->top.port)

#define OUTPUT(port, value) outb((INT) hacb->baseAddress->top.port,(CHAR) value)

#define PHYSICAL_TO_ULONG(physicalAddress) physicalAddress

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count) \
   physicalAddress += count

#define ZERO_PHYSICAL_ADDRESS(physicalAddress) physicalAddress = 0

#endif
