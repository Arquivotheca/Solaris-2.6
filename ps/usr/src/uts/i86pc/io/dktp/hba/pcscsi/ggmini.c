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

   FILE: GGMINI.C

   DESCRIPTION:

     This is the portable core source code for the AMD Golden Gate
     SCSI controller and DMA engine.

   REVISION HISTORY:

     DATE      VERSION INITIAL COMMENT
     --------  ------- ------- -------------------------------------------
     02/15/95  2.01    DM      Added flags for disabling directly PCI I/O
                               access and for disabling updating PCI scratch
                               registers
     02/06/95  2.0     DM      Modified and released for Windows NT and
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
// Set operating system flag here if any
//---------------------------------------------------------------
#ifndef OTHER_OS
#define OTHER_OS
#endif

//--------------------------------------------------
// Disable directly I/O accessing PCI register flag
//--------------------------------------------------
#define DISABLE_DIO
#ifdef NT_31
#ifdef DISABLE_DIO
#undef DISABLE_DIO
#endif
#endif

//---------------------------------------
// Disable updating PCI scratch register
//---------------------------------------
#ifndef DISABLE_SREG
#define DISABLE_SREG
#endif

//------------------
// Disable MDL flag
//------------------
#ifndef DISABLE_MDL
#define DISABLE_MDL
#endif

//----------------------------------------------
// Hardware flag for Compaq machine display LED
//----------------------------------------------
#ifndef COMPAQ_MACHINE
#define COMPAQ_MACHINE
#endif

//--------------
// Include files
//--------------
#include "miniport.h"
#include "scsi.h"
#include "ggmini.h"

//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      #ifdef out all of this routine which touches PCI config space;
//      the Solaris driver handles this.
//
#ifdef SOLARIS
#include <sys/debug.h>
#endif    // #ifndef SOLARIS

//-----------------------------------------------------
// Debug display and break point flags
//    DBG - enable debug;
//    MSG - enable debug message when if debug enabled;
//    BRK - enable break point if debug enabled.
//-----------------------------------------------------
#ifdef DBG

#define LOCAL

#ifdef MSG
int AMDDebug;
#define AMDPrint(arg) ScsiDebugPrint arg
#else
#define AMDPrint(arg)
#endif

#ifdef  BRK
#define SetBreakPoint _asm int 3
#else
#define SetBreakPoint
#endif

#else

#define AMDPrint(arg)
#define SetBreakPoint
#define LOCAL static

#endif

//-------------------------
// Clock speeds
//-------------------------
#define SCSIClockFreq  40
#define PCIClockFreq   33

#ifdef OTHER_OS

//-----------------------------
// AMD I/O control function ID
//-----------------------------
const UCHAR AMD_IO_CONTROL_ID[] = "AMD SCSI";

//---------------
// PCI mechanism
//---------------
UCHAR PciMechanism = 0;
BOOLEAN UserSetPciMechanism = FALSE;

#endif

//-----------------------------------------------
// Define the table of synchronous transfer types
//-----------------------------------------------
const SYNCHRONOUS_TYPE_PARAMETERS SynchronousTransferTypes[] = {
    {0x13,   0x5,    0x5},
    {0x0d,   0x4,    0x4}
};


//
// Functions passed to the OS-specific port driver.
//
LOCAL
ULONG
AMDFindAdapter(
    IN PVOID ServiceContext,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

LOCAL
BOOLEAN
AMDInitializeAdapter(
    IN PVOID ServiceContext
    );

LOCAL
BOOLEAN
AMDInterruptServiceRoutine(
    IN PVOID ServiceContext
    );

LOCAL
VOID
AMDErrorHandle(
    PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN ULONG ErrorCode,
    IN ULONG UniqeCode
    );

LOCAL
BOOLEAN
AMDResetScsiBus(
    IN PVOID ServiceContext,
    IN ULONG PathId
    );

LOCAL
BOOLEAN
AMDStartIo(
    IN PVOID ServiceContext,
    IN PSCSI_REQUEST_BLOCK Srb
    );

//
// Internal mini-port driver functions.
//
LOCAL
VOID
AMDAcceptMessage(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN SetAttention,
    IN BOOLEAN SetSynchronousParameters
    );

LOCAL
VOID
AMDCleanupAfterReset(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN ExternalReset
    );

LOCAL
VOID
AMDCompleteSendMessage(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN ULONG SrbStatus
    );

LOCAL
BOOLEAN
AMDDecodeSynchronousRequest(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN PSPECIFIC_LOGICAL_UNIT_EXTENSION LuExtension,
    IN BOOLEAN ResponseExpected
    );

LOCAL
BOOLEAN
AMDMessageDecode(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    );

LOCAL
VOID
AMDLogError(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN ULONG ErrorCode,
    IN ULONG UniqueCode
    );

LOCAL
VOID
AMDProcessRequestCompletion(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    );

LOCAL
VOID
AMDResetScsiBusInternal(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN ULONG PathId
    );

LOCAL
BOOLEAN
AMDSelectTarget(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN PSPECIFIC_LOGICAL_UNIT_EXTENSION LuExtension
    );

LOCAL
VOID
AMDSendMessage(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN PSPECIFIC_LOGICAL_UNIT_EXTENSION LuExtension
    );

LOCAL
VOID
AMDStartExecution(
    PSCSI_REQUEST_BLOCK Srb,
    PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    PSPECIFIC_LOGICAL_UNIT_EXTENSION LuExtension
    );

#ifdef OTHER_OS

LOCAL
VOID
AMDUpdateSGTPointer(
    IN OUT PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    ULONG Offset
    );

#endif

LOCAL
ULONG
AMDFindPciAdapter(
    IN OUT PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
    );

#ifndef NT_31

LOCAL
VOID
AMDSysGetPciConfig(
    IN OUT PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    );

#endif

#ifndef DISABLE_DIO

LOCAL
VOID
AMDIoGetPCIConfig(
    IN OUT PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    );

LOCAL
ULONG
AMDPciReadM1(
            IN PPCI_CONFIG_REGS ConfigBase,
            IN ULONG InfoData
            );

#endif	// #ifndef DISABLE_DIO

LOCAL
VOID
AMDStartDataTransfer(
    IN PVOID ServiceContext
    );


#ifndef T1
#ifndef DISABLE_MDL

LOCAL
VOID
AMDSgMakeMDL(
    IN OUT PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    );

LOCAL
VOID
AMDPtrMakeMDL(
    IN OUT PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    );

LOCAL
BOOLEAN
AMDBuildMDL(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN ULONG physicalAddress,
    IN ULONG dataLength,
    IN PULONG MDLIndexPtr
    );

#endif
#endif

LOCAL
VOID
AMDStartDMA(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    );

LOCAL
VOID
AMDFlushDMA(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    );


//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      #ifdef out all of this routine which touches PCI config space;
//      the Solaris driver handles this.
//
#ifdef SOLARIS
extern
#else
LOCAL
#endif    // #ifndef SOLARIS

VOID
AMDAccessPCIRegister(
                    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
                    IN ULONG PCIRegister,
                    IN ULONG RegLength,
                    IN PVOID DataPtr,
                    IN ULONG Operation
                    );

#ifndef DISABLE_SREG

LOCAL
VOID
AMDUpdateInfo (
              IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
              IN ULONG Operation
              );

#endif

LOCAL
VOID
AMDSaveDataPointer(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    PSCSI_REQUEST_BLOCK Srb
    );

LOCAL
VOID
AMDRestoreDataPointer(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    PSCSI_REQUEST_BLOCK Srb
    );

#ifdef COMPAQ_MACHINE

LOCAL
VOID
AMDTurnLEDOn(
    IN BOOLEAN TurnOn
    );

#endif

#ifndef OTHER_OS

LOCAL
VOID
AMDParseFlagString (
                   IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
                   IN PUCHAR InputString
                   );

LOCAL
BOOLEAN
AMDStringCompare (
                 IN PUCHAR sPtr1, 
                 IN PUCHAR sPtr2
                 );

#endif

LOCAL
BOOLEAN
AMDCPUModeSwitchHandler (
                 IN PVOID DeviceExtension,
                 IN PVOID Context,
                 IN BOOLEAN SaveState
                 );


LOCAL
VOID
AMDAcceptMessage(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN SetAttention,
    IN BOOLEAN SetSynchronousParameters
    )
/*++

Routine Description:

    This procedure tells the adapter to accept a pending message on the SCSI
    bus.  Optionally, it will set the synchronous transfer parameters and the
    attention signal.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension.

    SetAttention - Indicates the attention line on the SCSI bus should be set.

    SetSynchronousParameters - Indicates the synchronous data transfer
        parameters should be set.

Return Value:

    None.

--*/

{

    PSPECIFIC_LOGICAL_UNIT_EXTENSION luExtension;

    /* Powerfail */

    //
    // Added in for debug 
    //
    AMDPrint((0,"AMDAcceptMessage.\n"));

    //
    // Check to see if the synchonous data transfer parameters need to be set.
    //
    if (SetSynchronousParameters) {

        //
        // Added in for debug 
        //
        AMDPrint((0,"AMDAcceptMessage: Synchronous Parameters.\n"));

        //
        // These must be set before a data transfer is started.
        //
        luExtension = DeviceExtension->ActiveLogicalUnit;
        SCSI_WRITE( DeviceExtension->Adapter,
                    SynchronousPeriod,
                    luExtension->SynchronousPeriod
                    );
        SCSI_WRITE( DeviceExtension->Adapter,
                    SynchronousOffset,
                    luExtension->SynchronousOffset
                    );
        SCSI_WRITE( DeviceExtension->Adapter,
                    Configuration3,
                    *((PUCHAR) &luExtension->Configuration3)
                    );
        SCSI_WRITE( DeviceExtension->Adapter,
                    Configuration4,
                    *((PUCHAR) &luExtension->Configuration4)
                    );
    }

    //
    // Check to see if the attention signal needs to be set.
    //
    if (SetAttention) {

        //
        // Added in for debug 
        //
        AMDPrint((0,"AMDAcceptMessage: Set Attention.\n"));

        //
        // This requests that the target enter the message-out phase.
        //
        SCSI_WRITE( DeviceExtension->Adapter, Command, SET_ATTENTION );

    }

    //
    // Indicate to the adapter that the message-in phase may now be completed.
    //
    SCSI_WRITE(DeviceExtension->Adapter, Command, MESSAGE_ACCEPTED);
}

LOCAL
VOID
AMDCleanupAfterReset(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN ExternalReset
    )

/*++

Routine Description:

    This routine cleans up the adapter-specific and logical-unit-specific 
    data structures.  Any active requests are completed and the synchronous 
    negotiation flags are cleared.

Arguments:

    DeviceExtension - Supplies a pointer to device extension for the bus that
        was reset.

    ExternalReset - When set, indicates that the reset was generated by a
        SCSI device other than this host adapter.

Return Value:

    None.

--*/

{
    ULONG pathId = 0;
    ULONG targetId;
    ULONG luId;
    PSPECIFIC_LOGICAL_UNIT_EXTENSION luExtension;

    //
    // Added for debug 
    //
    AMDPrint((0,"AMDCleanupAfterReset.\n"));

    //
    // Check to see if a data transfer was in progress, if so, flush the DMA.
    //
    if (DeviceExtension->AdapterState == DataTransfer) {

        SCSI_WRITE(DeviceExtension->Adapter, Command, FLUSH_FIFO);
        AMDFlushDMA(DeviceExtension);

    }

    //
    // If there is a pending request, clear it
    //
    if (DeviceExtension->AdapterFlags & PD_PENDING_START_IO) {

        DeviceExtension->NextSrbRequest = NULL;
        DeviceExtension->AdapterFlags &= ~PD_PENDING_START_IO;

    }

    //
    // If there was an active request, then complete it with RESET status
    //
    if (DeviceExtension->ActiveLuRequest != NULL) {

        //
        // Set the SrbStatus in the SRB, complete the request and
        // clear the active pointers
        //
        luExtension = DeviceExtension->ActiveLogicalUnit;

        DeviceExtension->ActiveLuRequest->SrbStatus = SRB_STATUS_BUS_RESET;

        ScsiPortNotification(
            RequestComplete,
            DeviceExtension,
            DeviceExtension->ActiveLuRequest
            );

        //
        // Check to see if there was a synchronous negotiation in progress.  If
        // there was then do not try to negotiate with this target again.
        //
        if (DeviceExtension->AdapterFlags & (PD_SYNCHRONOUS_RESPONSE_SENT |
            PD_SYNCHRONOUS_TRANSFER_SENT)) {

            //
            // This target cannot negotiate properly.  Set a flag to prevent
            // further attempts and set the synchronous parameters to use
            // asynchronous data transfer.
            //
            /* TODO: Consider propagating this flag to all the Lus on this target. */
            luExtension->LuFlags |= PD_DO_NOT_NEGOTIATE;
            luExtension->SynchronousOffset = ASYNCHRONOUS_OFFSET;
            luExtension->SynchronousPeriod = ASYNCHRONOUS_PERIOD;
            luExtension->Configuration3.FastScsi = 0;

        }

        luExtension->ActiveLuRequest = NULL;
        luExtension->RetryCount  = 0;
        DeviceExtension->ActiveLogicalUnit = NULL;
        DeviceExtension->ActiveLuRequest = NULL;
    }

    //
    // Clear the appropriate state flags as well as the next request.
    // The request will actually be cleared when the logical units are processed.
    // Note that it is not necessary to fail the request waiting to be started
    // since it will be processed properly by the target controller, but it
    // is cleared anyway.
    //
    for (targetId = 0; targetId < SCSI_MAXIMUM_TARGETS; targetId++) {

        //
        // Loop through each of the possible logical units for this target.
        //
        for (luId = 0; luId < SCSI_MAXIMUM_LOGICAL_UNITS; luId++) {

            luExtension = ScsiPortGetLogicalUnit( DeviceExtension,
                                                  (UCHAR) pathId,
                                                  (UCHAR) targetId,
                                                  (UCHAR) luId
                                                  );

            if (luExtension == NULL) {
                continue;
            }

            ScsiPortCompleteRequest(
                DeviceExtension,
                (UCHAR) pathId,
                (UCHAR) targetId,
                (UCHAR) luId,
                SRB_STATUS_BUS_RESET
                );

            luExtension->ActiveLuRequest = NULL;

            if (luExtension->ActiveSendRequest != NULL) {

                //
                // Set the SrbStatus in the SRB, complete the request and
                // clear the active pointers
                //
                luExtension->ActiveSendRequest->SrbStatus =
                    SRB_STATUS_BUS_RESET;

                //
                // Complete the request.
                //
                ScsiPortNotification(
                    RequestComplete,
                    DeviceExtension,
                    luExtension->ActiveSendRequest
                    );

                luExtension->ActiveSendRequest = NULL;

            }

            //
            // Clear the necessary logical unit flags.
            //
            luExtension->LuFlags &= ~PD_LU_RESET_MASK;

        } /* for luId */
    } /* for targetId */

    //
    // Clear the adapter flags and set the bus state to free.
    //
    DeviceExtension->AdapterState = BusFree;
    DeviceExtension->AdapterFlags &= ~PD_ADAPTER_RESET_MASK;

    //
    // Ready for new request
    //
    ScsiPortNotification( NextRequest, DeviceExtension, NULL );

}

LOCAL
VOID
AMDCompleteSendMessage(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN ULONG SrbStatus
    )
/*++

Routine Description:

    This function does the cleanup necessary to complete a send-message request.
    This includes completing any affected execute-I/O requests and cleaning
    up the device extension state.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension of the SCSI bus
        adapter.  The active logical unit is stored in ActiveLogicalUnit.

    SrbStatus - Indicates the status that the request should be completeted with
        if the request did not complete normally, then any active execute
        requests are not considered to have been affected.

Return Value:

    None.

--*/

{
    PSPECIFIC_LOGICAL_UNIT_EXTENSION luExtension;
    PSCSI_REQUEST_BLOCK srb;
    PSCSI_REQUEST_BLOCK srbAbort;
    ULONG pathId = 0;
    ULONG targetId;
    ULONG luId;
    BOOLEAN srb_exists = TRUE;

    luExtension = DeviceExtension->ActiveLogicalUnit;
    srb = luExtension->ActiveSendRequest;

    //
    // Added for debug 
    //
    AMDPrint((0,"AMDCompleteSendMessage.\n"));

//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      Fix warnings for 'assignment type mismatch' and 'incompatible types..'.
//
#ifdef SOLARIS
    // Fix bug:  After a device reset has occurred, all the srbs for this 
    // lun have been 'completed' (see case SRB_FUNCTION_RESET_DEVICE: below).
    // luExtension->ActiveSendRequest is set to NULL.
    // ScsiPortCompleteRequest 'completes' all the requests, which ultimately
    // deallocates their srb memory.
    // The AMD code below assumes that the srb is still accessible 
    // (luExtension->ActiveSendRequest != NULL).
    // If it doesn't... Disaster.
    // This should *never* happen...
    ASSERT(srb != NULL);
#endif  // SOLARIS

    //
    // Clean up any EXECUTE requests which may have been affected by this
    // message.
    //
    if (SrbStatus == SRB_STATUS_SUCCESS) {

        switch (srb->Function) {

        case SRB_FUNCTION_TERMINATE_IO:
        case SRB_FUNCTION_ABORT_COMMAND:

            //
            // Make sure there is still a request to complete.  If so complete
            // it with an SRB_STATUS_ABORTED status.
            //
            srbAbort = ScsiPortGetSrb(
                DeviceExtension,
                srb->PathId,
                srb->TargetId,
                srb->Lun,
                srb->QueueTag
                );

            if (srbAbort != srb->NextSrb) {

                //
                // If there is no request, then fail the abort.
                //
                SrbStatus = SRB_STATUS_ABORT_FAILED;
                break;
            }

            srbAbort->SrbStatus = SRB_STATUS_ABORTED;

            ScsiPortNotification(
                RequestComplete,
                DeviceExtension,
                srbAbort
                );

            if (DeviceExtension->ActiveLuRequest == srbAbort) {

                DeviceExtension->ActiveLuRequest = NULL;
            }

            luExtension->ActiveLuRequest = NULL;
            luExtension->LuFlags &= ~PD_LU_COMPLETE_MASK;
            luExtension->RetryCount = 0;

            break;

        case SRB_FUNCTION_RESET_DEVICE:


            // Fix bug:  Remove the 'queued' request in NextSrbRequest if
            // it is for this target (since it no longer exists as far
            // as the upper layer is concerned after ScsiPortCompleteRequest
	    // has been called).
            //
            // If there is a pending request for this target, clear it.
	    // It will be 'completed' in a call to ScsiPortComleteRequest 
	    // (below).
            //
            if ( (DeviceExtension->AdapterFlags & PD_PENDING_START_IO)   &&
                 (DeviceExtension->NextSrbRequest->PathId   == pathId)   &&
                 (DeviceExtension->NextSrbRequest->TargetId == targetId) )  {

                    DeviceExtension->NextSrbRequest = NULL;
                    DeviceExtension->AdapterFlags &= ~PD_PENDING_START_IO;
            }


            //
            // Cycle through each of the possible logical units looking
            // for requests which have been cleared by the target reset.
            //
            targetId = srb->TargetId;

            for (luId = 0; luId < SCSI_MAXIMUM_LOGICAL_UNITS; luId++) {

		// Fix bug: A local luExtension is needed here, else
		// this loop overwrites the one passed in (probably with NULL).
		// When access are made via luExtension at the bottom
		// of the routine - panic.
		PSPECIFIC_LOGICAL_UNIT_EXTENSION unit_luExtension; // SOLARIS

                unit_luExtension = ScsiPortGetLogicalUnit( DeviceExtension,
                                                      (UCHAR) pathId,
                                                      (UCHAR) targetId,
                                                      (UCHAR) luId
                                                      );

		// Fix bug: Don't access luExtensions which don't exist.
                if (unit_luExtension == NULL) {			// SOLARIS
                    continue;
                }

		// Bug: Shouldn't complete the 'reset target' srb here;
		// should leave that one, so it will complete with 
		// 'RequestComplete' at the end of this routine.
		// But ScsiPortCompleteRequest routine does not allow for this,
		// because there is no mechanism for indicating which srb or
		// which queue tag to NOT abort; also the intended design
		// is to abort *all* outstanding requests for this lun - 
		// which of course includes *this* one, 
		// AND any the might be pending StartIo
		// (in NextRequest).
		// Can't fix this short of a redesign.

		// Abort/complete/free *all* outstanding requests for this 
		// target/lun.
                ScsiPortCompleteRequest(
                    DeviceExtension,
                    (UCHAR) pathId,
                    (UCHAR) targetId,
                    (UCHAR) luId,
                    SRB_STATUS_BUS_RESET
                    );

		// Make sure the rest of the core knows that the reset-target
		// request (the one that got us here) no longer exists.
                unit_luExtension->ActiveLuRequest = NULL; 	// SOLARIS
                unit_luExtension->RetryCount = 0; 		// SOLARIS

                //
                // Clear the necessary logical unit flags.
                //
                unit_luExtension->LuFlags &= ~PD_LU_RESET_MASK; // SOLARIS

            } /* for luId */

	    // The srb for the message request that got us here no longer
	    // exists.
            // Note that this situation also requires special handling at the 
	    // end of this routine (see below).
	    srb_exists = FALSE;

        /* TODO: Handle CLEAR QUEUE */
        }
    } else {

        //
        // If an abort request fails then complete target of the abort;
        // otherwise the target of the ABORT may never be compileted.
        //
        if (srb->Function == SRB_FUNCTION_ABORT_COMMAND) {

            //
            // Make sure there is still a request to complete.  If so
            // it with an SRB_STATUS_ABORTED status.
            //
            srbAbort = ScsiPortGetSrb(
                DeviceExtension,
                srb->PathId,
                srb->TargetId,
                srb->Lun,
                srb->QueueTag
                );

            if (srbAbort == srb->NextSrb) {

                srbAbort->SrbStatus = SRB_STATUS_ABORTED;

                ScsiPortNotification(
                    RequestComplete,
                    DeviceExtension,
                    srbAbort
                    );

                luExtension->ActiveLuRequest = NULL;
                luExtension->LuFlags &= ~PD_LU_COMPLETE_MASK;
                luExtension->RetryCount  = 0;

            }
        }
    }

    //
    // Complete the actual send-message request.
    // ...if it still exists for the upper layer...
    //
    if (srb_exists)  {
        srb->SrbStatus = (UCHAR) SrbStatus;
        ScsiPortNotification(
            RequestComplete,
            DeviceExtension,
            srb
            );
    }

    //
    // Clear the active send request and PD_SEND_MESSAGE_REQUEST flag.
    //
    luExtension->RetryCount = 0;
    luExtension->ActiveSendRequest = NULL;
    DeviceExtension->AdapterFlags &= ~PD_SEND_MESSAGE_REQUEST;
}

LOCAL
BOOLEAN
AMDMessageDecode(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This function decodes the SCSI bus message-in the device extension message
    buffer.  After the message is decoded it decides what action to take in
    response to the message.  If an outgoing message needs to be sent, then
    it is placed in the message buffer and TRUE is returned. If the message
    is acceptable, then the device state is set either to DisconnectExpected or
    MessageAccepted and the MessageCount is reset to 0.

    Some messages are made up of several bytes.  This funtion will simply
    return false when an incomplete message is detected, allowing the target
    to send the rest of the message.  The message count is left unchanged.

Arguments:

    DeviceExtension - Supplies a pointer to the specific device extension.

Return Value:

    TRUE - Returns true if there is a reponse message to be sent.

    FALSE - If there is no response message.

--*/

{
    PSCSI_REQUEST_BLOCK srb;
    PSPECIFIC_LOGICAL_UNIT_EXTENSION luExtension;
    ULONG offset;
    ULONG i;
    ULONG savedAdapterFlags;
    PSCSI_EXTENDED_MESSAGE extendedMessage;

    //
    // Added for debug 
    //
    AMDPrint((0,"AMDMessageDecode.\n"));
    SetBreakPoint

    //
    // NOTE:  The ActivelogicalUnit field could be invalid if the
    // PD_DISCONNECT_EXPECTED flag is set, so luExtension cannot
    // be used until this flag has been checked.
    //
    luExtension = DeviceExtension->ActiveLogicalUnit;
    savedAdapterFlags = DeviceExtension->AdapterFlags;
    srb = DeviceExtension->ActiveLuRequest;

    //
    // If a queue message is expected then it must be the first message byte.
    //
    if (DeviceExtension->AdapterFlags & PD_EXPECTING_QUEUE_TAG &&
        DeviceExtension->MessageBuffer[0] != SRB_SIMPLE_TAG_REQUEST) {

        AMDPrint((0, "AMDMessageDecode: Unexpected message recieved when que tag expected.\n"));

        //
        // The target did not reselect correctly Send a
        // message reject of this message.
        //
        DeviceExtension->MessageCount = 1;
        DeviceExtension->MessageSent = 0;
        DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
        DeviceExtension->AdapterState = MessageOut;

        return(TRUE);
    }

    //
    // A number of special cases must be handled if a special message has
    // just been sent.  These special messages are synchronous negotiations
    // or a messages which implie a disconnect.  The special cases are:
    //
    // If a disconnect is expected because of a send-message request,
    // then the only valid message-in is a MESSAGE REJECT; other messages
    // are a protocol error and are rejected.
    //
    // If a synchronous negotiation response was just sent and the message
    // in was not a MESSAGE REJECT, then the negotiation has been accepted.
    //
    // If a synchronous negotiation request was just sent, then valid responses
    // are a MESSAGE REJECT or an extended synchronous message back.
    //
    if (DeviceExtension->AdapterFlags & (PD_SYNCHRONOUS_RESPONSE_SENT |
        PD_DISCONNECT_EXPECTED | PD_SYNCHRONOUS_TRANSFER_SENT)) {

        if (DeviceExtension->AdapterFlags & PD_DISCONNECT_EXPECTED &&
            DeviceExtension->MessageBuffer[0] != SCSIMESS_MESSAGE_REJECT) {

            //
            // The target is not responding correctly to the message.  Send a
            // message reject of this message.
            //
            DeviceExtension->MessageCount = 1;
            DeviceExtension->MessageSent = 0;
            DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
            DeviceExtension->AdapterState = MessageOut;

            return(TRUE);
        }

        if (DeviceExtension->AdapterFlags & PD_SYNCHRONOUS_RESPONSE_SENT &&
            DeviceExtension->MessageBuffer[0] != SCSIMESS_MESSAGE_REJECT) {

            //
            // The target did not reject our response so the synchronous
            // transfer negotiation is done.  Clear the adapter flags and
            // set the logical unit flags indicating this. Continue processing
            // the message which is unrelated to negotiation.
            //
            DeviceExtension->AdapterFlags &= ~PD_SYNCHRONOUS_RESPONSE_SENT;
            luExtension->LuFlags |= PD_SYNCHRONOUS_NEGOTIATION_DONE;
        }

        //
        // Save the adapter flags for later use.
        //
        savedAdapterFlags = DeviceExtension->AdapterFlags;

        if (DeviceExtension->AdapterFlags & PD_SYNCHRONOUS_TRANSFER_SENT ) {

            //
            // The target is sending a message after a synchronous transfer
            // request was sent.  Valid responses are a MESSAGE REJECT or an
            // extended synchronous message; any other message negates the
            // fact that a negotiation was started.  However, since extended
            // messages are multi-byte, it is difficult to determine what the
            // incoming message is.  So at this point, the fact that a
            // sychronous transfer was sent will be saved and cleared from the
            // AdapterFlags.  If the message looks like a synchronous transfer
            // request, then restore this fact back into the AdapterFlags. If
            // the complete message is not the one expected, then opening
            // negotiation will be forgotten. This is an error by the target,
            // but minor so nothing will be done about it.  Finally, to prevent
            // this cycle from reoccurring on the next request indicate that
            // the negotiation is done.
            //
            DeviceExtension->AdapterFlags &= ~PD_SYNCHRONOUS_TRANSFER_SENT;
            luExtension->LuFlags |= PD_SYNCHRONOUS_NEGOTIATION_DONE;
        }

    }

    switch (DeviceExtension->MessageBuffer[0]) {

    case SCSIMESS_COMMAND_COMPLETE:

        //
        // Added for debug
        //
        AMDPrint((0,"AMDMessageDecode: Command complete message.\n"));

        //
        // For better or worse the command is complete.  Process request which
        // set the SrbStatus and clean up the device and logical unit states.
        //
        AMDProcessRequestCompletion(DeviceExtension);
        
        //
        // Everything is ok with the message so do not send one and set the
        // state to DisconnectExpected.
        //
        DeviceExtension->MessageCount = 0;
        DeviceExtension->AdapterState = DisconnectExpected;
        DeviceExtension->AdapterFlags |= PD_REQUEST_POSTING;
        return(FALSE);

    case SCSIMESS_DISCONNECT:

        //
        // Added for debug
        //
        AMDPrint((0,"AMDMessageDecode: Disconnection message.\n"));

        //
        // At least one device sends disconnection message without
        // save data pointer.  So save data pointer anyway
        //
        AMDSaveDataPointer (DeviceExtension, srb);

        //
        // The target wants to disconnect.  Set the state to DisconnectExpected,
        // and do not request a message-out.
        //
        DeviceExtension->AdapterState = DisconnectExpected;
        DeviceExtension->MessageCount = 0;
        return(FALSE);

    case SCSIMESS_EXTENDED_MESSAGE:

        //
        // Added for debug
        //
        AMDPrint((0,"AMDMessageDecode: Extended message.\n"));

        //
        // The format of an extended message is:
        //    Extended Message Code
        //    Length of Message
        //    Extended Message Type
        //            .
        //            .
        //
        // Until the entire message has been read in, just keep getting bytes
        // from the target, making sure that the message buffer is not
        // overrun.
        //
        extendedMessage = (PSCSI_EXTENDED_MESSAGE)
            DeviceExtension->MessageBuffer;

        if (DeviceExtension->MessageCount < 2 ||
            (DeviceExtension->MessageCount < MESSAGE_BUFFER_SIZE &&
            DeviceExtension->MessageCount < extendedMessage->MessageLength + 2)
            ) {

            //
            // Update the state and return; also restore the AdapterFlags.
            //
            DeviceExtension->AdapterFlags = savedAdapterFlags;
            DeviceExtension->AdapterState = MessageAccepted;
            return(FALSE);

        }

        //
        // Make sure the length includes an extended op-code.
        //
        if (DeviceExtension->MessageCount < 3) {

            //
            // This is an illegal extended message. Send a MESSAGE_REJECT.
            //
            DeviceExtension->MessageCount = 1;
            DeviceExtension->MessageSent = 0;
            DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
            DeviceExtension->AdapterState = MessageOut;

            return(TRUE);
        }

        //
        // Determine the extended message type.
        //
        switch (extendedMessage->MessageType) {

        case SCSIMESS_MODIFY_DATA_POINTER:

            //
            // Verify the message length.
            //
            if (extendedMessage->MessageLength != SCSIMESS_MODIFY_DATA_LENGTH) {

                //
                // Reject the message.
                //
                DeviceExtension->MessageCount = 1;
                DeviceExtension->MessageSent = 0;
                DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
                DeviceExtension->AdapterState = MessageOut;

                return(TRUE);
            }

            //
            // Calculate the modification to be added to the data pointer.
            //
            offset = 0;
            for (i = 0; i < 4; i++) {
                offset <<= 8;
                offset += extendedMessage->ExtendedArguments.Modify.Modifier[i];
            }

            //
            // Verify that the new data pointer is still within the range
            // of the buffer.
            //
            if (SRB_EXT(srb)->TotalDataTransferred + offset > srb->DataTransferLength ||
                ((LONG) (SRB_EXT(srb)->TotalDataTransferred + offset)) < 0 ) {

                //
                // The new pointer is not valid, so reject the message.
                //
                DeviceExtension->MessageCount = 1;
                DeviceExtension->MessageSent = 0;
                DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
                DeviceExtension->AdapterState = MessageOut;

                return(TRUE);
            }

#ifdef OTHER_OS

            //
            // Everything has checked out, so update the pointer.
            //
            if (DeviceExtension->SysFlags.DMAMode != IO_DMA_METHOD_LINEAR) {

              //
              // Set SG entry to first element
              //
              DeviceExtension->ActiveSGEntry = srb->DataBuffer;
              DeviceExtension->ActiveSGTEntryOffset = 
                DeviceExtension->SysFlags.DMAMode == IO_DMA_METHOD_MDL ?
                  *((PULONG)srb->DataBuffer) & PAGE_MASK : 0;

              //
              // Offset from beginning place
              //
              offset = (LONG)SRB_EXT(srb)->TotalDataTransferred + offset;

              //
              // Set SG table pointer
              //
              AMDUpdateSGTPointer (DeviceExtension, offset);

            } else

#endif

            DeviceExtension->ActiveDataPointer =
              (LONG) DeviceExtension->ActiveDataPointer + offset;

            //
            // Everything is ok, accept the message as is.
            //
            DeviceExtension->MessageCount = 0;
            DeviceExtension->AdapterState = MessageAccepted;
            return(FALSE);

        case SCSIMESS_SYNCHRONOUS_DATA_REQ:

            //
            // A SYNCHRONOUS DATA TRANSFER REQUEST message was received.
            // Make sure the length is correct.
            //
            if ( extendedMessage->MessageLength !=
                SCSIMESS_SYNCH_DATA_LENGTH) {

                //
                // The length is invalid, so reject the message.
                //
                DeviceExtension->MessageCount = 1;
                DeviceExtension->MessageSent = 0;
                DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
                DeviceExtension->AdapterState = MessageOut;
                return(TRUE);
            }

            //
            // If synchrouns negotiation has been disabled for this request,
            // then reject any synchronous messages; however, when synchronous
            // transfers are allowed then a new attempt can be made.
            //
            if (srb != NULL &&
                !(savedAdapterFlags & PD_SYNCHRONOUS_TRANSFER_SENT) &&
                srb->SrbFlags & SRB_FLAGS_DISABLE_SYNCH_TRANSFER) {

                //
                // Reject the synchronous transfer message since synchonrous
                // transfers are not desired at this time.
                //
                DeviceExtension->MessageCount = 1;
                DeviceExtension->MessageSent = 0;
                DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
                DeviceExtension->AdapterState = MessageOut;
                return(TRUE);

            }

            //
            // Call AMDDecodeSynchronousMessage to decode the message and
            // formulate a response if necessary.
            // AMDDecodeSynchronousRequest will return FALSE if the
            // message is not accepable and should be rejected.
            //
            if (!AMDDecodeSynchronousRequest(
                DeviceExtension,
                luExtension,
                (BOOLEAN) ((!(savedAdapterFlags & PD_SYNCHRONOUS_TRANSFER_SENT)) != 0)
                )) {

                //
                // Indicate that a negotiation has been done in the logical
                // unit and clear the negotiation flags.
                //
                luExtension->LuFlags |= PD_SYNCHRONOUS_NEGOTIATION_DONE;
                DeviceExtension->AdapterFlags &=
                    ~(PD_SYNCHRONOUS_RESPONSE_SENT|
                    PD_SYNCHRONOUS_TRANSFER_SENT);

                //
                // The message was not acceptable so send a MESSAGE_REJECT.
                //
                DeviceExtension->MessageCount = 1;
                DeviceExtension->MessageSent = 0;
                DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
                DeviceExtension->AdapterState = MessageOut;
                return(TRUE);
            }

            //
            // If a reponse was expected, then set the state for a message-out.
            // Otherwise, AMDDecodeSynchronousRequest has put a reponse
            // in the message buffer to be returned to the target.
            //
            if (savedAdapterFlags & PD_SYNCHRONOUS_TRANSFER_SENT){

                //
                // We initiated the negotiation, so no response is necessary.
                //
                DeviceExtension->AdapterState = MessageAccepted;
                DeviceExtension->AdapterFlags &= ~PD_SYNCHRONOUS_TRANSFER_SENT;
                luExtension->LuFlags |= PD_SYNCHRONOUS_NEGOTIATION_DONE;
                DeviceExtension->MessageCount = 0;
                return(FALSE);
            }

            //
            // Set up the state to send the reponse.  The message count is
            // still correct.
            //
            DeviceExtension->MessageSent = 0;
            DeviceExtension->AdapterState = MessageOut;
            DeviceExtension->AdapterFlags &= ~PD_SYNCHRONOUS_TRANSFER_SENT;
            DeviceExtension->AdapterFlags |= PD_SYNCHRONOUS_RESPONSE_SENT;
            return(TRUE);

        case SCSIMESS_WIDE_DATA_REQUEST:

            //
            // A WIDE DATA TRANSFER REQUEST message was received.
            // Make sure the length is correct.
            //
            if ( extendedMessage->MessageLength !=
                SCSIMESS_WIDE_DATA_LENGTH) {

                //
                // The length is invalid reject the message.
                //
                DeviceExtension->MessageCount = 1;
                DeviceExtension->MessageSent = 0;
                DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
                DeviceExtension->AdapterState = MessageOut;
                return(TRUE);
            }

            //
            // Since this SCSI protocol chip only supports 8-bits return
            // a width of 0 which indicates an 8-bit-wide transfers.  The
            // MessageCount is still correct for the message.
            //
            extendedMessage->ExtendedArguments.Wide.Width = 0;
            DeviceExtension->MessageSent = 0;
            DeviceExtension->AdapterState = MessageOut;
            return(TRUE);

        default:

            //
            // This is an unknown or illegal message, so send-message REJECT.
            //
            DeviceExtension->MessageCount = 1;
            DeviceExtension->MessageSent = 0;
            DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
            DeviceExtension->AdapterState = MessageOut;
            return(TRUE);
        }

    case SCSIMESS_MESSAGE_REJECT:

        //
        // Added for debug
        //
        AMDPrint((0,"AMDMessageDecode: Message reject message.\n"));

        //
        // The last message we sent was rejected.  If this was a send
        // message request, then set the proper status and complete the
        // request. Set the state to message accepted.
        //
        /* TODO: Handle message reject correctly. */
        if (DeviceExtension->AdapterFlags & PD_SEND_MESSAGE_REQUEST) {

            //
            // Complete the request with message rejected status.
            //
            AMDCompleteSendMessage(
                DeviceExtension,
                SRB_STATUS_MESSAGE_REJECTED
                );
        }

        //
        // Check to see if a synchronous negotiation is in progress.
        //
        if (savedAdapterFlags & (PD_SYNCHRONOUS_RESPONSE_SENT|
            PD_SYNCHRONOUS_TRANSFER_SENT)) {

            //
            // The negotiation failed so use asynchronous data transfers.
            // Indicate that the negotiation has been attempted and set
            // the transfer for asynchronous.  Clear the negotiation flags.
            //
            luExtension->LuFlags |= PD_SYNCHRONOUS_NEGOTIATION_DONE;
            luExtension->SynchronousOffset = ASYNCHRONOUS_OFFSET;
            luExtension->SynchronousPeriod = ASYNCHRONOUS_PERIOD;
            luExtension->Configuration3.FastScsi = 0;
            DeviceExtension->AdapterFlags &=  ~(PD_SYNCHRONOUS_RESPONSE_SENT|
                PD_SYNCHRONOUS_TRANSFER_SENT);

            //
            // Even though the negotiation appeared to go ok, there is no reason
            // to try again, and some targets get messed up later, so do not try
            // synchronous negotiation again.
            //
            /* TODO: Reconsider doing this. */

            luExtension->LuFlags |= PD_DO_NOT_NEGOTIATE;

        }

        DeviceExtension->AdapterState = MessageAccepted;
        DeviceExtension->MessageCount = 0;
        return(FALSE);

    case SCSIMESS_SIMPLE_QUEUE_TAG:
    case SCSIMESS_ORDERED_QUEUE_TAG:
    case SCSIMESS_HEAD_OF_QUEUE_TAG:

        //
        // Added for debug
        //
        AMDPrint((0,"AMDMessageDecode: Queue tag message.\n"));

        //
        // A queue tag message was recieve.  If this is the first byte just
        // accept the message and wait for the next one.
        //
        if (DeviceExtension->MessageCount < 2) {

            DeviceExtension->AdapterState = MessageAccepted;
            return(FALSE);

        }

        //
        // Make sure that a queue tag message is expected.
        //
        if (!(DeviceExtension->AdapterFlags & PD_EXPECTING_QUEUE_TAG) ||
            luExtension == NULL) {

            AMDPrint((0, "AMDMessageDecode: Unexpected queue tag message recieved\n"));

            //
            // Something is messed up.  Reject the message.
            //
            DeviceExtension->MessageCount = 1;
            DeviceExtension->MessageSent = 0;
            DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
            DeviceExtension->AdapterState = MessageOut;
            return(TRUE);

        }

        //
        // The second byte contains the tag used to locate the srb.
        //
        srb = ScsiPortGetSrb(
            DeviceExtension,
            0,
            DeviceExtension->TargetId,
            DeviceExtension->Lun,
            DeviceExtension->MessageBuffer[1]
            );

        if (srb == NULL) {

            AMDPrint((0, "AMDMessageDecode: Invalid queue tag recieved\n"));

            //
            // Something is messed up.  Reject the message.
            //
            DeviceExtension->AdapterFlags &= ~PD_EXPECTING_QUEUE_TAG;
            DeviceExtension->MessageCount = 1;
            DeviceExtension->MessageSent = 0;
            DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
            DeviceExtension->AdapterState = MessageOut;
            return(TRUE);

        }

        //
        // Everthing is ok. Set up the device extension and accept the message.
        // Restore the data pointers.
        //
        DeviceExtension->ActiveLuRequest = srb;
        AMDRestoreDataPointer (DeviceExtension, srb);
        DeviceExtension->AdapterFlags &= ~PD_EXPECTING_QUEUE_TAG;
        DeviceExtension->AdapterState = MessageAccepted;
        DeviceExtension->MessageCount = 0;
        return(FALSE);

    case SCSIMESS_RESTORE_POINTERS:

        //
        // Added for debug
        //
        AMDPrint((0,"AMDMessageDecode: Restore data pointer message.\n"));

        //
        // Restore data pointer message.  Just copy the saved data pointer
        // and the length to the active data pointers.
        //
        AMDRestoreDataPointer (DeviceExtension, srb);
        DeviceExtension->AdapterState = MessageAccepted;
        DeviceExtension->MessageCount = 0;
        return(FALSE);

    case SCSIMESS_SAVE_DATA_POINTER:

        //
        // Added for debug
        //
        AMDPrint((0,"AMDMessageDecode: Save data pointer message.\n"));

        //
        // SAVE DATA POINTER message request that the active data pointer and
        // length be copied to the saved location.
        //
        AMDSaveDataPointer (DeviceExtension, srb);
        DeviceExtension->AdapterState = MessageAccepted;
        DeviceExtension->MessageCount = 0;
        return(FALSE);

    default:

        //
        // Added for debug
        //
        AMDPrint((0,"AMDMessageDecode: Invalid message.\n"));

        //
        // An unrecognized or unsupported message. send-message reject.
        //
        DeviceExtension->MessageCount = 1;
        DeviceExtension->MessageSent = 0;
        DeviceExtension->MessageBuffer[0] = SCSIMESS_MESSAGE_REJECT;
        DeviceExtension->AdapterState = MessageOut;
        return(TRUE);
    }
}

LOCAL
BOOLEAN
AMDDecodeSynchronousRequest(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    OUT PSPECIFIC_LOGICAL_UNIT_EXTENSION LuExtension,
    IN BOOLEAN ResponseExpected
    )
/*++

Routine Description:

    This function decodes the synchronous data transfer request message from
    the target.  It will update the synchronous message-in the buffer and the
    synchronous transfer parameters in the logical unit extension.  These
    parameters are specific for the AMD 53C9XX protocol chip.  The updated
    message-in the device extension message buffer might be returned to the
    target.

    This function should be called before the final byte of the message is
    accepted from the SCSI bus.

Arguments:

    DeviceExtension - Supplies a pointer to the adapter specific device
        extension.

    LuExtension - Supplies a pointer to the logical unit's device extension.
        The synchronous transfer fields are updated  in this structure to
        reflect the new parameter in the message.

    ResponseExpected - When set, indicates that the target initiated the
        negotiation and that it expects a response.

Return Value:

    TRUE - Returned if the request is acceptable.

    FALSE - Returned if the request should be rejected and asynchronous
        transfer should be used.

--*/

{
    PSCSI_EXTENDED_MESSAGE extendedMessage;
    CHIP_TYPES chipType;
    ULONG period;
    ULONG localPeriod;
    ULONG step;
    ULONG i;

#ifdef DISABLE_SREG

    PPCI_TARGET_STATE infoPtr;

#endif

    //
    // Added for debug 
    //
    AMDPrint((0,"AMDDecodeSynchronousRequest.\n"));

    //
    // Clear pending flag if set
    //
    LuExtension->LuFlags &= ~PD_SYNCHRONOUS_RENEGOTIATION;

    extendedMessage = (PSCSI_EXTENDED_MESSAGE) DeviceExtension->MessageBuffer;

    //
    // Determine the transfer offset.  It is the minimum of the SCSI protocol
    // chip's maximum offset and the requested offset.
    //
    if (extendedMessage->ExtendedArguments.Synchronous.ReqAckOffset > 
        SYNCHRONOUS_OFFSET) {

        if (!ResponseExpected) {

            //
            // The negotiation failed for some reason; fall back to
            // asynchronous data transfer.
            //
            LuExtension->SynchronousOffset = ASYNCHRONOUS_OFFSET;
            LuExtension->SynchronousPeriod = ASYNCHRONOUS_PERIOD;
            LuExtension->Configuration3.FastScsi = 0;
            return(FALSE);
        }

        extendedMessage->ExtendedArguments.Synchronous.ReqAckOffset = SYNCHRONOUS_OFFSET;
        LuExtension->SynchronousOffset = SYNCHRONOUS_OFFSET;

    } else {

        LuExtension->SynchronousOffset =
            extendedMessage->ExtendedArguments.Synchronous.ReqAckOffset;

    }

    //
    // If the offset requests asynchronous transfers then set the default
    // period and return.
    //
    if (extendedMessage->ExtendedArguments.Synchronous.ReqAckOffset ==
        ASYNCHRONOUS_OFFSET) {

        LuExtension->SynchronousPeriod = ASYNCHRONOUS_PERIOD;
        LuExtension->Configuration3.FastScsi = 0;

    } else {

      //
      // Calculate the period in nanoseconds from the message.
      //
      period = extendedMessage->ExtendedArguments.Synchronous.TransferPeriod;

      AMDPrint((0, "AMDDecodeSynchronousRequest: Requested period %d, ", period));

      //
      // If the chip supports fast SCSI and the requested period is faster than
      // 200 ns then assume fast SCSI.
      //
      if (period < 200 / 4) {

          chipType = Fas216Fast;

          //
          // Set the fast SCSI bit in the configuration register.
          //
          LuExtension->Configuration3.FastScsi = 1;

          //
          // Set Glitch Eater time to 12 ns for fast SCSI
          //
          LuExtension->Configuration4.KillGlitch = 0;

      } else {

          chipType = DeviceExtension->SysFlags.ChipType;

      }

      //
      // The initial sychronous transfer period is:
      //
      //  SynchronousPeriodCyles * 1000
      //  -----------------------------
      //    ClockSpeed * 4
      //
      // Note the result of the divide by four must be rounded up.
      //
      localPeriod =  ((SynchronousTransferTypes[chipType].SynchronousPeriodCyles
          * 1000) / DeviceExtension->ClockSpeed + 3) / 4;

      //
      // Check to see if the period is less than the SCSI protocol chip can
      // use.  If it is then update the message with our minimum and return.
      //
      if ((ULONG) period < localPeriod ) {

          if (!ResponseExpected) {

            //
            // The negotiation failed for some reason; fall back to
            // asynchronous data transfer.
            //
            LuExtension->SynchronousOffset = ASYNCHRONOUS_OFFSET;
            LuExtension->SynchronousPeriod = ASYNCHRONOUS_PERIOD;
            LuExtension->Configuration3.FastScsi = 0;
            AMDPrint((0, "Too fast. Local period %d\n", localPeriod));
            return(FALSE);
          }

          extendedMessage->ExtendedArguments.Synchronous.TransferPeriod =
              (UCHAR) localPeriod;
          period = localPeriod;
      }

      //
      // The synchronous transfer cycle count is calculated by:
      //
      //  (RequestedPeriod - BasePeriod) * 1000
      //  ------------------------------------- + InitialRegisterValue
      //             ClockSpeed * 4
      //
      // Note the divide must be rounded up.
      //
      step = (1000 / 4) / DeviceExtension->ClockSpeed;
      period -= localPeriod;
      for (i = SynchronousTransferTypes[chipType].InitialRegisterValue;
          i < SynchronousTransferTypes[chipType].MaximumPeriodCyles;
          i++) {

        if ((LONG)period <= 0) {
            break;
        }

        period -= step;
        localPeriod += step;
      }

      AMDPrint((0, "Local period: %d, Register value: %d\n", localPeriod, i));

      if (i >= SynchronousTransferTypes[chipType].MaximumPeriodCyles) {

          //
          // The requested transfer period is too long for the SCSI protocol
          // chip.  Fall back to synchronous and reject the request.
          //
          LuExtension->SynchronousOffset = ASYNCHRONOUS_OFFSET;
          LuExtension->SynchronousPeriod = ASYNCHRONOUS_PERIOD;
          LuExtension->Configuration3.FastScsi = 0;

          return(FALSE);
      }

      LuExtension->SynchronousPeriod = (UCHAR) i;

      //
      // If it is not fast SCSI, adjust value by 1
      //
      if (!LuExtension->Configuration3.FastScsi) {

        LuExtension->SynchronousPeriod--;

        //
        // For sync. transfer, set Glitch Eater to 25ns
        //
        if (LuExtension->SynchronousOffset != 0)
          LuExtension->Configuration4.KillGlitch = 2;

      }    
    }

    //
    // If no response was expected then the negotation has completed
    // successfully.  Set the synchronous data transfer parameter registers
    // to the new values.  These must be set before a data transfer
    // is started.
    //
    SCSI_WRITE( DeviceExtension->Adapter,
                SynchronousPeriod,
                LuExtension->SynchronousPeriod
                );
    SCSI_WRITE( DeviceExtension->Adapter,
                SynchronousOffset,
                LuExtension->SynchronousOffset
                );
    SCSI_WRITE( DeviceExtension->Adapter,
                Configuration3,
                *((PUCHAR) &LuExtension->Configuration3)
                );
    SCSI_WRITE( DeviceExtension->Adapter,
                Configuration4,
                *((PUCHAR) &LuExtension->Configuration4)
                );

#ifndef DISABLE_SREG

    //
    // Update state register with synchronous parameters
    //
    AMDUpdateInfo (DeviceExtension, Synchronous);

#else

    //
    // Update sync. xfer info.
    //
    infoPtr = (PPCI_TARGET_STATE)&DeviceExtension->SCSIState[DeviceExtension->TargetId];
    infoPtr->SyncOffset = LuExtension->SynchronousOffset;
    infoPtr->SyncPeriod = LuExtension->SynchronousPeriod;
	 infoPtr->FastScsi = LuExtension->Configuration3.FastScsi;

#endif

    return(TRUE);

}

LOCAL
BOOLEAN
AMDInitializeAdapter(
    IN PVOID ServiceContext
    )
/*++

Routine Description:

    This function initializes the AMD 53c9XX SCSI HBA and protocol
    chip.  This function must be called before any other operations are
    performed.  It should also be called after a power failure.  This
    function does not cause any interrupts; however, after it completes
    interrupts can occur.

Arguments:

    ServiceContext - Pointer to the specific device extension for this SCSI
        bus.

Return Value:

    TRUE - Returns true indicating that the initialization of the chip is
        complete.

--*/

{
    PSPECIFIC_DEVICE_EXTENSION DeviceExtension = ServiceContext;
    UCHAR dataByte;

    //
    // Added in for debug 
    //
    AMDPrint((0,"AMDInitializeAdapter: Initializing chip.\n"));
    SetBreakPoint

    //
    // Clear the adapter flags
    //
    DeviceExtension->AdapterFlags = 0;

    //
    // Initialize the AMD 53c9XX SCSI protocol chip.
    //
    SCSI_WRITE( DeviceExtension->Adapter, Command, RESET_SCSI_CHIP );
    SCSI_WRITE( DeviceExtension->Adapter, Command, NO_OPERATION_DMA );

    //
    // Set the configuration register for slow cable mode, parity enable,
    // and allow reset interrupts, also set the host adapter SCSI bus id.
    // Configuration registers 2 and 3 are cleared by the chip reset and
    // do not need to be changed.
    //
    dataByte = DeviceExtension->AdapterBusId;
    ((PSCSI_CONFIGURATION1)(&dataByte))->ParityEnable = 
      (BOOLEAN)DeviceExtension->SysFlags.EnablePrity;

    SCSI_WRITE(DeviceExtension->Adapter, Configuration1, dataByte);

    //
    // Configuration registers 2 and 3 are cleared by a chip reset and do
    // need to be initialized. Note these registers do not exist on the
    // AMD53c90, but the writes will do no harm. Set configuration register 3
    // with the value determined by the find adapter routine.
    //
    SCSI_WRITE(
        DeviceExtension->Adapter,
        Configuration3,
        *((PUCHAR)&DeviceExtension->Configuration3)
        );

    //
    // Set control register 4
    //
    SCSI_WRITE(
        DeviceExtension->Adapter,
        Configuration4,
        *((PUCHAR)&DeviceExtension->Configuration4)
        );

    //
    // Enable the SCSI-2 features.
    //
    dataByte = 0;
    ((PSCSI_CONFIGURATION2)(&dataByte))->EnablePhaseLatch = 1;

    SCSI_WRITE(DeviceExtension->Adapter, Configuration2, dataByte);

    //
    // Set the clock conversion register. The clock convertion factor is the
    // clock speed divided by 5 rounded up. Only the low three bits are used.
    //
    dataByte = (DeviceExtension->ClockSpeed + 4) / 5;
    SCSI_WRITE(
        DeviceExtension->Adapter,
        ClockConversionFactor,
        (UCHAR) (dataByte & 0x07)
        );

    //
    // Set the SelectTimeOut Register to 250ms.  This value is based on the
    // clock conversion factor and the clock speed.
    //
    dataByte = SELECT_TIMEOUT_FACTOR  * DeviceExtension->ClockSpeed / dataByte;

    SCSI_WRITE( DeviceExtension->Adapter, SelectTimeOut, dataByte);

    //
    // Set bus state free
    //
    DeviceExtension->AdapterState = BusFree;

    return( TRUE );
}

LOCAL
BOOLEAN
AMDInterruptServiceRoutine(
    PVOID ServiceContext
    )
/*++

Routine Description:

    This routine is the interrupt service routine for the AMD 53c9XX SCSI
    host adapter.  It is the main SCSI protocol engine of the driver and
    is driven by service requests from targets on the SCSI bus.  This routine
    also detects errors and performs error recovery. Generally, this routine
    handles one interrupt per invokation.

    The general flow of this routine is as follows:

        Check for a power down interrupt interrupt.

        Check for a SCSI interrupt.

        Determine if there are any pending errors.

        Check to see if the bus disconnected.

        Check that the previous function completed normally.

        Determine what the target wants to do next and program the chip
        appropriately.

        Check for the next interrupt.

Arguments:

    ServiceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    TRUE - Indicates that an interrupt was found.

    FALSE - Indicates the device was not interrupting.

--*/

{
    PSPECIFIC_DEVICE_EXTENSION DeviceExtension = ServiceContext;
    PSPECIFIC_LOGICAL_UNIT_EXTENSION luExtension;
    PSCSI_REQUEST_BLOCK srb;
    ULONG i;
    BOOLEAN setAttention;
    SHORT targetStatus = ~0;
    UCHAR message;
    ULONG dmaStatus;

#ifndef T1

    ULONG miscData;

#endif

    /* POWERFAIL */

    //
    // Added in for debug 
    //
    AMDPrint((0,"AMDInterruptServiceRoutine: New Interrupt.\n"));
    SetBreakPoint

    //
    // Read DMA status register
    //
    dmaStatus = DMA_READ (DeviceExtension->Adapter, StatusReg);

    //
    // If there is SCSI interrupt, go to interrupt service.  Otherwise make
    // more check before return
    //
    if (!(dmaStatus & SCSICoreInterrupt)) {

      //
      // Check power down interrupt
      //
      if (DeviceExtension->AdapterState == BusFree &&
          dmaStatus & PowerDown) {

        //
        // Added for debug
        //
        AMDPrint((0,"AMDInterruptServiceRoutine: Power down interrupt.\n"));

#ifdef OTHER_OS

        //
        // This call tells OS power down request active
        //
//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      Fix warnings for 'assignment type mismatch' and 'incompatible types..'.
//
#ifdef SOLARIS
        if (DeviceExtension->powerDownPost != (PPWR_INTERRUPT)NULL) {
#else   // SOLARIS
        if (DeviceExtension->powerDownPost != NULL) {
#endif  // SOLARIS
 
          //
          // Tell uplayer power down interrupt event
          //
          (*DeviceExtension->powerDownPost) (DeviceExtension);

        }

#endif

        return TRUE;
        
      } 

#ifndef T1

        else {

        //
        // For T2/G2, PCI master/target abort interrupt supported.
        // Check if it is the PCI interrupt
        //
        if (DeviceExtension->PciConfigInfo.ChipRevision >= 0x10 &&
            DeviceExtension->AdapterState == DataTransfer) {

          //
          // Enable write/clear feature to reserve DMA status bits
          //
          miscData = 0;
          ((PDMA_MISC_DATA)&miscData)->WriteClearEnable = 1;
          ((PDMA_MISC_DATA)&miscData)->PCIAbortIntEnable = 0;
          DMA_WRITE (DeviceExtension->Adapter, MiscReg, miscData);

          //
          // Clear PCI abort bit anyway
          //
          DMA_WRITE (DeviceExtension->Adapter, StatusReg, PCIAbort);

          //
          // Disable write/clear feature
          //
          ((PDMA_MISC_DATA)&miscData)->WriteClearEnable = 0;
          if (!(dmaStatus & PCIAbort))
            ((PDMA_MISC_DATA)&miscData)->PCIAbortIntEnable = 1;
          DMA_WRITE (DeviceExtension->Adapter, MiscReg, miscData);

          //
          // Check if it is the PCI abort interrupt
          //
          if (dmaStatus & PCIAbort) {

            //
            // PCI abort happened.  The way to handle this is restarting
            // DMA.  Abort current DMA operation first
            // 
            DMA_WRITE (DeviceExtension->Adapter, CommandReg, DMA_ABORT);

            // 
            // Clear DMA status register by reading
            // 
            DMA_READ (DeviceExtension->Adapter, StatusReg);

            //
            // Enable PCI abort interrupt
            //
            ((PDMA_MISC_DATA)&miscData)->PCIAbortIntEnable = 1;
            DMA_WRITE (DeviceExtension->Adapter, MiscReg, miscData);

            // 
            // Get number of data transferred
            // 
            while ((i =
		              DMA_READ(DeviceExtension->Adapter, WorkByteCountReg)) !=
		              DMA_READ(DeviceExtension->Adapter, WorkByteCountReg));
            i &= 0x00ffffff;
            miscData = i;
            i = DeviceExtension->ActiveDMALength - i;

            //
            // Update transfer count
            //
            srb = DeviceExtension->ActiveLuRequest;
            SRB_EXT(srb)->TotalDataTransferred += i;

            //
            // Data left to be transferred
            //
            DeviceExtension->ActiveDMALength = miscData;

            //
            // If still have data left, update SG table and data pointer
            //
            if (SRB_EXT(srb)->TotalDataTransferred < DeviceExtension->ActiveDataLength) {

              //
              // Update data pointer
              //
              if (DeviceExtension->SysFlags.DMAMode == IO_DMA_METHOD_LINEAR) {

                DeviceExtension->ActiveDataPointer += i;

              }

#ifdef OTHER_OS

				  else {

                AMDUpdateSGTPointer (DeviceExtension, i);

              }

#endif
            }

            //
            // Set error flag
            //
            srb->SrbStatus = SRB_STATUS_PHASE_SEQUENCE_FAILURE;

            //
            // Start DMA
            //
            AMDStartDataTransfer (DeviceExtension);

            // 
            // Return
            // 
            return TRUE;

          }
        }
      }

#endif
            
      //
      // Added in for debug
      //
      AMDPrint((0,"AMDInterruptServiceRoutine: False Interrupt.\n"));

      return FALSE;

    }

    //
    // Added for debug
    //
    AMDPrint((0,"AMDInterruptServiceRoutine: SCSI interrupt.\n"));

    //
    // Read SCSI status register
    //
    *((PUCHAR) &DeviceExtension->AdapterStatus) =
      SCSI_READ (DeviceExtension->Adapter, ScsiStatus);

    //
    // Get the current chip state which includes the status register, the
    // sequence step register and the interrupt register. These registers are
    // frozen until the interrupt register is read.
    //
    *((PUCHAR) &DeviceExtension->SequenceStep) = SCSI_READ(
                                                 DeviceExtension->Adapter,
                                                 SequenceStep
                                                 );
    //
    // This read will dismiss the interrupt.
    //
    *((PUCHAR) &DeviceExtension->AdapterInterrupt) = SCSI_READ(
                                                     DeviceExtension->Adapter,
                                                     ScsiInterrupt
                                                     );

    if (DeviceExtension->AdapterInterrupt.IllegalCommand) {
        AMDPrint((0, "AMDInterrupt: IllegalCommand\n" ));

        if (DeviceExtension->AdapterState == AttemptingSelect) {

            //
            // If an IllegalCommand interrupt has occurred and a select
            // is being attempted, flush the FIFO and exit. This occurs
            // when the fifo is being filled for a new command at the
            // same time time a reselection occurs.
            //
            SCSI_WRITE(DeviceExtension->Adapter, Command, FLUSH_FIFO);

        } else {

            //
            // An illegal command occured at an unexpected time.  Reset the
            // bus and log an error.
            //
            AMDErrorHandle (DeviceExtension, SP_INTERNAL_ADAPTER_ERROR, 14);

        }

        return(TRUE);
    }

    //
    // Check for a bus reset.
    //
    if (DeviceExtension->AdapterInterrupt.ScsiReset) {

        //
        // Added in for debug 
        //
        AMDPrint((0,"AMDInterruptServiceRoutine: Reset Interrupt.\n"));

#ifndef DISABLE_SREG

        //
        // Update SCSI phase information bits in PCI state register
        //
        AMDUpdateInfo (DeviceExtension, Reset);

#else

        //
        // Clean target information
        //
        for (i = 0; i < 8; i++)
          DeviceExtension->SCSIState[i] = 0;

#endif

        //
        // Check if this was an expected reset.
        //
        if (!(DeviceExtension->AdapterFlags & PD_EXPECTING_RESET_INTERRUPT)) {

            AMDPrint((0, "AMDInterruptServiceRoutine: SCSI bus reset detected\n"));

            //
            // Cleanup the logical units and notify the port driver,
            // then return.
            //
            AMDCleanupAfterReset(DeviceExtension, TRUE);
            ScsiPortNotification(
                ResetDetected,
                DeviceExtension,
                NULL
                );

        } else {

            DeviceExtension->AdapterFlags &= ~PD_EXPECTING_RESET_INTERRUPT;

        }

        //
        // Stall for a short time. This allows interrupt to clear.
        //
        ScsiPortStallExecution(INTERRUPT_STALL_TIME);

        SCSI_WRITE(DeviceExtension->Adapter, Command, FLUSH_FIFO);

        //
        // Note that this should only happen in firmware where the interrupts
        // are polled.
        //
        if (DeviceExtension->AdapterFlags & PD_PENDING_START_IO) {

            //
            // Call AMDStartIo to start the pending request.
            // Note that AMDStartIo is idempotent when called with
            // the same arguments.
            //
            AMDStartIo(
                DeviceExtension,
                DeviceExtension->NextSrbRequest
                );

        }

        return(TRUE);
    }

    //
    // Check for parity errors.
    //
    if (DeviceExtension->AdapterStatus.ParityError) {

        //
        // The SCSI protocol chip has set ATN: we expect the target to
        // go into message-out so that a error message can be sent and the
        // operation retried. After the error has been noted, continue
        // processing the interrupt. The message sent depends on whether a
        // message was being received or something else.  If the status
        // is currently message-in then send-message PARITY ERROR;
        // otherwise, send INITIATOR DETECTED ERROR.
        //
        AMDPrint((0, "AMDInterruptServiceRoutine: Parity error detected.\n"));

        //
        // If the current phase is MESSAGE_IN then special handling is requred.
        //
        if (DeviceExtension->AdapterStatus.Phase == MESSAGE_IN) {

            //
            // If the current state is CommandComplete, then the fifo contains
            // a good status byte.  Save the status byte before handling the
            // message parity error.
            //
            if (DeviceExtension->AdapterState == CommandComplete) {

                srb = DeviceExtension->ActiveLuRequest;

                srb->ScsiStatus = SCSI_READ(
                    DeviceExtension->Adapter,
                    Fifo
                    );

                SRB_EXT(srb)->SrbExtensionFlags |= PD_STATUS_VALID;

            }

            //
            // Set the message to indicate a message parity error, flush the
            // fifo and accept the message.
            //
            DeviceExtension->MessageBuffer[0] = SCSIMESS_MESS_PARITY_ERROR;
            SCSI_WRITE(DeviceExtension->Adapter, Command, FLUSH_FIFO);
            AMDAcceptMessage(DeviceExtension, TRUE, TRUE);

            //
            // Since the message which was in the fifo is no good. Clear the
            // function complete interrupt which indicates that a message byte
            // has been recieved.  If this is a reselection, then this will
            // a bus reset to occur.  This cause is not handled well in this
            // code, because it is not setup to deal with a target id and no
            // logical unit.
            //
            DeviceExtension->AdapterInterrupt.FunctionComplete = FALSE;

        } else {

            DeviceExtension->MessageBuffer[0] = SCSIMESS_INIT_DETECTED_ERROR;

        }

        DeviceExtension->MessageCount = 1;
        DeviceExtension->MessageSent = 0;
        DeviceExtension->AdapterState = MessageOut;
        DeviceExtension->AdapterFlags |= PD_MESSAGE_OUT_VALID;

        if (!(DeviceExtension->AdapterFlags & PD_PARITY_ERROR_LOGGED)) {
            AMDLogError(DeviceExtension, SP_BUS_PARITY_ERROR, 2);
            DeviceExtension->AdapterFlags |= PD_PARITY_ERROR_LOGGED;
        }
    }

    //
    // Check for bus disconnection.  If this was expected, then the next request
    // can be processed.  If a selection was being attempted, then perhaps the
    // logical unit is not there or has gone away.  Otherwise, this is an
    // unexpected disconnect and should be reported as an error.
    //
    if (DeviceExtension->AdapterInterrupt.Disconnect) {

        //
        // Added in for debug 
        //
        AMDPrint((0,"AMDInterruptServiceRoutine: Disconnection Interrupt.\n"));

        srb = DeviceExtension->NextSrbRequest;

        //
        // Check for an unexpected disconnect.  This occurs if the state is
        // not ExpectingDisconnect and a selection did not fail.  A selection
        // failure is indicated by state of AttemptingSelect and a sequence
        // step of 0.
        //
        if (DeviceExtension->AdapterState == AttemptingSelect &&
               DeviceExtension->SequenceStep.Step == 0) {

            //
            // Save target id
            //
            DeviceExtension->TargetId = srb->TargetId;

            //
            // The target selection failed.  Log the error.  If the retry
            // count is not exceeded then retry the selection; otherwise
            // fail the request.
            //
            luExtension = ScsiPortGetLogicalUnit(
                DeviceExtension,
                srb->PathId,
                srb->TargetId,
                srb->Lun
                );

            if (luExtension->RetryCount++ >= RETRY_SELECTION_LIMIT) {

                //
                // Clear the Active request in the logical unit.
                //
                luExtension->RetryCount = 0;

                if (DeviceExtension->AdapterFlags & PD_SEND_MESSAGE_REQUEST) {

                    //
                    // Process the completion of the send message request.
                    // Set the ActiveLogicalUnit for AMDCompleteSendMessage.
                    // ActiveLogicalUnit is cleared after it returns.
                    //
                    DeviceExtension->ActiveLogicalUnit = luExtension;

                    AMDCompleteSendMessage(
                        DeviceExtension,
                        SRB_STATUS_SELECTION_TIMEOUT
                        );

                    DeviceExtension->ActiveLogicalUnit = NULL;

                } else {

                    srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;

                    ScsiPortNotification(
                        RequestComplete,
                        DeviceExtension,
                        srb
                        );

                    luExtension->ActiveLuRequest = NULL;
                }

#ifndef DISABLE_SREG

                //
                // Update state information - No target exists
                //
                AMDUpdateInfo (DeviceExtension, TargetNotExist);

#else

                ((PPCI_TARGET_STATE)&DeviceExtension->
                  SCSIState[DeviceExtension->TargetId])->TargetExist = 0;

#endif

                targetStatus = TargetNotExist;

#ifdef COMPAQ_MACHINE

                //
                // If this is a Compaq system, turn disk LED off
                //
                if (DeviceExtension->SysFlags.MachineType == COMPAQ) {

                  //
                  // This is a Compaq system.  Turn disk LED off
                  //
                  AMDTurnLEDOn (FALSE);

                }

#endif

                DeviceExtension->NextSrbRequest = NULL;
                DeviceExtension->AdapterFlags &= ~PD_PENDING_START_IO;

                ScsiPortNotification(
                    NextRequest,
                    DeviceExtension,
                    NULL
                    );

            }

            //
            // If the request needs to be retried, it will be automatically
            // because the PD_PENDING_START_IO flag is still set, and the
            // following code will cause it to be restarted.
            //
            // The chip leaves some of the command in the FIFO, so clear the
            // FIFO so there is no garbage left in it.
            //
            SCSI_WRITE(DeviceExtension->Adapter, Command, FLUSH_FIFO);

        } else if ( DeviceExtension->AdapterState == DisconnectExpected ||
                    DeviceExtension->AdapterFlags & PD_DISCONNECT_EXPECTED) {

            //
            // Check to see if this was a send-message request which is
            // completed when the disconnect occurs.
            //
            if (DeviceExtension->AdapterFlags & PD_SEND_MESSAGE_REQUEST) {

                //
                // Complete the request.
                //
                AMDCompleteSendMessage( DeviceExtension,
                                        SRB_STATUS_SUCCESS
                                        );

            //
            // Check if request posting flag set
            //
            } else if (DeviceExtension->AdapterFlags & PD_REQUEST_POSTING) {

                //
                // Complete the request.
                //
                ScsiPortNotification(
                    RequestComplete,
                    DeviceExtension,
                    DeviceExtension->ActiveLuRequest
                    );

                //
                // Clear active request
                //
                DeviceExtension->ActiveLuRequest = NULL;

            }

        } else {

            //
            // The disconnect was unexpected treat it as an error.
            // Check to see if a data transfer was in progress, if so flush
            // the DMA.
            //
            if (DeviceExtension->AdapterState == DataTransfer) {

                AMDFlushDMA(DeviceExtension);

            }

            //
            // NOTE: If the state is AttemptingSelect, then ActiveLogicalUnit
            //       is NULL!
            //
            // The chip leaves some of the command in the FIFO, so clear the
            // FIFO so there is not garbage left in it.
            //
            SCSI_WRITE(DeviceExtension->Adapter, Command, FLUSH_FIFO);

            //
            // Return error status
            //
            DeviceExtension->ActiveLuRequest->SrbStatus = SRB_STATUS_UNEXPECTED_BUS_FREE;

            //
            // For PCI phase update routine
            //
		      DeviceExtension->MessageBuffer[0] = SCSIMESS_COMMAND_COMPLETE;

            //
            // Post the request and ask for next one
            //
            ScsiPortNotification(
                RequestComplete,
                DeviceExtension,
                DeviceExtension->ActiveLuRequest
                );

            ScsiPortNotification(
                NextRequest,
                DeviceExtension,
                NULL
                );

            //
            // An unexpected disconnect has occurred.  Log the error.  It is
            // not clear if the device will respond again, so let the time-out
            // code clean up the request if necessary.
            //
            AMDPrint((0, "AMDInterruptServiceRoutine: Unexpected bus disconnect\n"));

            AMDLogError(DeviceExtension, SP_UNEXPECTED_DISCONNECT, 3);

        }

        //
        // Clean up the adapter state to indicate the bus is now free, enable
        // reselection, and start any pending request.
        //
        if (targetStatus != TargetNotExist) {

          targetStatus = BusFree;
          message = DeviceExtension->MessageBuffer[0];

        }

        DeviceExtension->AdapterState = BusFree;
        DeviceExtension->AdapterFlags &= ~PD_ADAPTER_DISCONNECT_MASK;
        DeviceExtension->ActiveLuRequest = NULL;
        SCSI_WRITE(DeviceExtension->Adapter, Command, ENABLE_SELECTION_RESELECTION);

        AMDPrint((0, "AMDInterruptServiceRoutine: DisconnectComplete.\n"));

        //
        // If there is pending request, execute it
        //
        if (DeviceExtension->AdapterFlags & PD_PENDING_START_IO &&
            !DeviceExtension->AdapterInterrupt.Reselected) {

          //
          // Check that the next request is still active.  This should not
          // be necessary, but it appears there is a hole somewhere.
          //
          srb = DeviceExtension->NextSrbRequest;
          srb = ScsiPortGetSrb(
                  DeviceExtension,
                  srb->PathId,
                  srb->TargetId,
                  srb->Lun,
                  srb->QueueTag
                  );

          if (srb != DeviceExtension->NextSrbRequest &&
            DeviceExtension->NextSrbRequest->Function == SRB_FUNCTION_EXECUTE_SCSI) {

            AMDPrint((0, "AMDInterruptServiceRoutine:  Found in active SRB in next request field.\n"));

            //
            // Dump it on the floor.
            //
            DeviceExtension->NextSrbRequest = NULL;
            DeviceExtension->AdapterFlags &= ~PD_PENDING_START_IO;
        
            AMDLogError(DeviceExtension, SP_INTERNAL_ADAPTER_ERROR, 18);
        
            ScsiPortNotification(
                NextRequest,
                DeviceExtension,
                NULL
                );
        
          } else {

            //
            // Call AMDStartIo to start the pending request.
            // Note that AMDStartIo is idempotent when called with
            // the same arguments.
            //
            AMDStartIo(
                DeviceExtension,
                DeviceExtension->NextSrbRequest
                );

          }
        }
    }

    //
    // Check for a reselection interrupt.
    //
    if (DeviceExtension->AdapterInterrupt.Reselected) {
        UCHAR targetId;
        UCHAR luId;

        //
        // Added in for debug 
        //
        AMDPrint((0,"AMDInterruptServiceRoutine: Reselection Interrupt.\n"));

        //
        // The usual case is not to set attention so initialize the
        // varible to FALSE.
        //
        setAttention = FALSE;

        //
        // If the FunctionComplete interrupt is not set then the target did
        // not send an IDENTFY message.  This is a fatal protocol violation.
        // Reset the bus to get rid of this target.
        //
        if (!DeviceExtension->AdapterInterrupt.FunctionComplete) {

            AMDPrint((0, "AMDInterruptServiceRoutine: Reselection Failed.\n"));
            AMDErrorHandle (DeviceExtension, SP_PROTOCOL_ERROR, 4);
            return(TRUE);

        }

        //
        // The target Id and the logical unit id are in the FIFO. Use them to
        // get the connected active logical unit.
        //
        luId = SCSI_READ(DeviceExtension->Adapter, Fifo);

        //
        // The select id has two bits set.   One is the SCSI bus id of the
        // initiator and the other is the reselecting target id.  The initiator
        // id must be stripped and the remaining bit converted to a bit number
        // to get the target id.
        //
        luId &= ~DeviceExtension->AdapterBusIdMask;
        WHICH_BIT(luId, targetId);
        DeviceExtension->TargetId = targetId;

        luId = SCSI_READ(DeviceExtension->Adapter, Fifo);

        //
        // The logical unit id is stored in the low-order 3 bits of the
        // IDENTIFY message, so the upper bits must be stripped off the
        // byte read from the FIFO to get the logical unit number.
        //
        luId &= SCSI_MAXIMUM_LOGICAL_UNITS - 1;

        luExtension = ScsiPortGetLogicalUnit( DeviceExtension,
                                              0,
                                              targetId,
                                              luId
                                              );

        //
        // Check to that this is a valid logical unit.
        //
        if (luExtension == NULL) {

            AMDPrint((0, "AMDInterruptServiceRoutine: Reselection Failed.\n"));

            ScsiPortLogError(
                DeviceExtension,                    //  HwDeviceExtension,
                NULL,                               //  Srb
                0,                                  //  PathId,
                targetId,                           //  TargetId,
                luId,                               //  Lun,
                SP_INVALID_RESELECTION,             //  ErrorCode,
                4                                   //  UniqueId
                );

            //
            // Send an abort message.  Put the message in the buffer, set the
            // state,  indicate that a disconnect is expected after this, and
            // set the attention signal.
            //
            DeviceExtension->MessageBuffer[0] = SCSIMESS_ABORT;
            DeviceExtension->MessageCount = 1;
            DeviceExtension->MessageSent = 0;
            DeviceExtension->AdapterState = MessageOut;
            DeviceExtension->AdapterFlags &= ~PD_ADAPTER_DISCONNECT_MASK;
            DeviceExtension->AdapterFlags |= PD_MESSAGE_OUT_VALID |
                PD_DISCONNECT_EXPECTED;

            setAttention = TRUE;

        } else {

            //
            // Everything looks ok.
            //
            // A reselection has been completed.  Set the active logical
            // unit, restore the active data pointer, and set the state.
            // In addition, any adpater flags set by a pending select
            // must be cleared using the disconnect mask.
            //
            DeviceExtension->AdapterFlags &= ~PD_ADAPTER_DISCONNECT_MASK;
            DeviceExtension->ActiveLogicalUnit = luExtension;
            DeviceExtension->AdapterState = MessageAccepted;
            DeviceExtension->MessageCount = 0;

            srb = luExtension->ActiveLuRequest;
            DeviceExtension->ActiveLuRequest = srb;

            if (srb == NULL) {

                //
                // This must be a reconnect for a tagged request.
                // Indicate a queue tag message is expected next and save
                // the target and logical unit ids.
                //
                DeviceExtension->AdapterFlags |= PD_EXPECTING_QUEUE_TAG;
                DeviceExtension->Lun = luId;

            } else {

                AMDRestoreDataPointer (DeviceExtension, srb);

            }
        }

        //
        // The bus is waiting for the message to be accepted.  The attention
        // signal will be set if this is not a valid reselection.  Finally,
        // the synchronous data tranfer parameters need to be set in case a
        // data transfer is done.
        //
        AMDAcceptMessage(DeviceExtension, setAttention, TRUE);

    } else if (DeviceExtension->AdapterInterrupt.FunctionComplete) {

        //
        // Added in for debug 
        //
        AMDPrint((0,"AMDInterruptServiceRoutine: Function Complete Interrupt.\n"));

        //
        // Check for function complete interrupt if there was not a reselected
        // interrupt.  The function complete interrupt has already been checked
        // in the previous case.
        //
        // The function complete interrupt occurs after the following cases:
        //    A select succeeded
        //    A message byte has been read
        //    A status byte and message byte have been read when in the
        //      command complete state.
        //    A reselection (handled above)
        //
        // Switch on the state current state of the bus to determine what
        // action should be taken now the function has completed.
        //
        switch (DeviceExtension->AdapterState) {

        case AttemptingSelect:

            //
            // The target was successfully selected.  Set the active
            // logical unit field, clear the next logical unit, and
            // notify the OS-dependent driver that a new request can
            // be accepted.  The state is set to MessageOut since is
            // the next thing done after a selection.
            //
            DeviceExtension->ActiveLogicalUnit = ScsiPortGetLogicalUnit(
                DeviceExtension,
                DeviceExtension->NextSrbRequest->PathId,
                DeviceExtension->NextSrbRequest->TargetId,
                DeviceExtension->NextSrbRequest->Lun
                );

            srb = DeviceExtension->NextSrbRequest;
            DeviceExtension->ActiveLuRequest = srb;
            DeviceExtension->TargetId = srb->TargetId;

            //
            // Restore the data pointers.
            //
            AMDRestoreDataPointer (DeviceExtension, srb);

            //
            // Last phase should be message out
            //
            DeviceExtension->AdapterState = MessageOut;

            //
            // The next request has now become the active request.
            // Clear the state associated with the next request and ask for
            // another one to start.
            //
            DeviceExtension->AdapterFlags &= ~PD_PENDING_START_IO;
            DeviceExtension->NextSrbRequest = NULL;

            //
            // If this was a tagged request then indicate that the next
            // request for this lu may be sent.
            //
            if (DeviceExtension->AdapterFlags & PD_TAGGED_SELECT) {

              ScsiPortNotification(
                  NextLuRequest,
                  DeviceExtension,
                  srb->PathId,
                  srb->TargetId,
                  srb->Lun
                  );

            } else {

              ScsiPortNotification(
                  NextRequest,
                  DeviceExtension,
                  NULL
                  );

            }

            break;

        case CommandComplete:

            //
            // The FIFO contains the status byte and a message byte.  Save the
            // status byte and set the state to look like MessageIn, then fall
            // through to the message-in state.
            //
            srb = DeviceExtension->ActiveLuRequest;

            srb->ScsiStatus = SCSI_READ(
                DeviceExtension->Adapter,
                Fifo
                );

            SRB_EXT(srb)->SrbExtensionFlags |= PD_STATUS_VALID;
            DeviceExtension->AdapterState = MessageIn;
            DeviceExtension->MessageCount = 0;
            DeviceExtension->AdapterFlags &= ~PD_MESSAGE_OUT_VALID;

            //
            // Fall through and process the message byte in the FIFO.
            //

        case MessageIn:

            //
            // A message byte has been received. Store it in the message buffer
            // and call message decode to determine what to do.  The message
            // byte will either be accepted, or cause a message to be sent.
            // A message-out is indicated to the target by setting the ATN
            // line before sending the SCSI protocol chip the MESSAGE_ACCEPTED
            // command.
            //
            DeviceExtension->MessageBuffer[DeviceExtension->MessageCount++] =
                             SCSI_READ( DeviceExtension->Adapter, Fifo );

            if (AMDMessageDecode( DeviceExtension )) {

              //
              // AMDMessageDecode returns TRUE if there is a message to be
              // sent out.  This message will normally be a MESSAGE REJECT
              // or a  SYNCHRONOUS DATA TRANSFER REQUEST.  In any case, the
              // message has been set by AMDMessageDecode.  All that needs
              // to be done here is set the ATN signal and set
              // PD_MESSAGE_OUT_VALID in the adapter flags.
              //
              DeviceExtension->AdapterFlags |= PD_MESSAGE_OUT_VALID;
              setAttention = TRUE;

            } else {

              setAttention = FALSE;

            }
       
            //
            // In either case, tell the SCSI protocol chip to acknowlege or
            // accept the message.  The synchronous data transfer parameters
            // do not need to be set.
            //
            AMDAcceptMessage( DeviceExtension, setAttention, FALSE);
            break;

        default:

            //
            // A function complete should not occur while in any other states.
            //
            AMDPrint((0, "AMDInterruptServiceRoutine: Unexpected function complete interrupt.\n"));

        }
    }

    //
    // Check for a bus service interrupt. This interrupt indicates the target
    // is requesting some form of bus transfer. The bus transfer type is
    // determined by the bus phase.
    //
    if (DeviceExtension->AdapterInterrupt.BusService) {

        AMDPrint((0, "AMDInterruptServiceRoutine: Bus service interrupt.\n"));

        luExtension = DeviceExtension->ActiveLogicalUnit;

        if (luExtension == NULL) {

            //
            // There should never be an bus service interrupt without an
            // active locgial unit.  The bus or the chip is really messed up.
            // Reset the bus and return.
            //
            AMDPrint((0, "AMDInterruptServiceRoutine: Unexpected Bus service interrupt.\n"));
            AMDErrorHandle (DeviceExtension, SP_PROTOCOL_ERROR, 6);
            return(TRUE);
        }

        srb = DeviceExtension->ActiveLuRequest;

        //
        // If there is no current srb request then the bus service interrupt
        // must be a message in with a tag.
        //
        if (DeviceExtension->AdapterFlags & PD_EXPECTING_QUEUE_TAG &&
            DeviceExtension->AdapterStatus.Phase != MESSAGE_IN ) {

            //
            // A bus service interrupt occured when a queue tag message
            // was exepected.  Is a protocol error by the target reset the
            // bus.
            //
            AMDPrint((0, "AMDInterruptServiceRoutine: Unexpected Bus service interrupt when queue tag expected.\n"));
            AMDErrorHandle (DeviceExtension, SP_PROTOCOL_ERROR, 13);
            return(TRUE);

        }

        //
        // The bus is changing phases or needs more data. Generally, the target
        // can change bus phase at any time:  in particular, in the middle of
        // a data transfer.  The initiator must be able to restart a transfer
        // where it left off. To do this it must know how much data was
        // transferred. If the previous state was a data transfer, then the
        // amount of data transferred needs to be determined, saved and
        // the DMA flushed.
        //
        if (DeviceExtension->AdapterState == DataTransfer) {
            SCSI_FIFO_FLAGS fifoFlags;
            ULONG j;
            SCSI_PHYSICAL_ADDRESS physicalAddress;
            ULONG CurrentPhysicalAddress, CurrentDataPointer, k;

            //
            // If previous phase is data in and scsi core terminal
            // bit not set, make sure the scsi fifo empty and recheck
            // the bit again
            //
            if (!DeviceExtension->AdapterStatus.TerminalCount &&
                DeviceExtension->dataPhase == DATA_IN) {

              //
              // Wait until all the data move out from SCSI FIFO 
              // to DMA FIFO.
              //
              do {

                while ((j = SCSI_READ (DeviceExtension->Adapter, FifoFlags)) !=
                       SCSI_READ (DeviceExtension->Adapter, FifoFlags));

              } while (j & 0x1e);

              //
              // Read scsi status register
              //
              *((PUCHAR) &DeviceExtension->AdapterStatus) =
                SCSI_READ (DeviceExtension->Adapter, ScsiStatus);

            }

            //
            // Zero out the count
            //
            i = 0;

            if (DeviceExtension->AdapterStatus.TerminalCount ||
                DeviceExtension->AdapterFlags & PD_ONE_BYTE_XFER) {

              //
              // Clear one byte xfer flag anyway
              //
              DeviceExtension->AdapterFlags &= ~PD_ONE_BYTE_XFER;

              //
              // Waiting until all data xfered
              //
              if (!(DeviceExtension->AdapterFlags & PD_ONE_BYTE_XFER)) {

                do {

                  while ((j = DMA_READ(DeviceExtension->Adapter, WorkByteCountReg)) !=
                          DMA_READ(DeviceExtension->Adapter, WorkByteCountReg));

                } while (j & 0x00ffffff && !(j & 0x00800000));
              }

            } else {

              //
              // Added for debug
              //
              AMDPrint((0, "AMDInterruptServiceRoutine: Terminal count is not zero.\n"));

              //
              // Read bits 23-16
              //
              i = (SCSI_READ(DeviceExtension->Adapter, TransferCountPage)) << 16;

              //
              // Read the current value of the tranfer count registers;
              //
              i |= (SCSI_READ(DeviceExtension->Adapter, TransferCountHigh)) << 8;
              i |= SCSI_READ(DeviceExtension->Adapter, TransferCountLow );

              //
              // A value of zero in i and TerminalCount clear indicates
              // that the transfer length was 64K and that no bytes were
              // transferred. Set i to 64K.
              //
              if (i == 0) {
                  i = 0x10000;
              }

              //
              // Check if previous phase is data in.
              //
              if (DeviceExtension->dataPhase == DATA_IN) {

                //
                // Loop twice to make sure DMA engine stable
                //
                for (k = 0; k < 2; k++) {

                  //
                  // Waiting util DMA engine stable
                  //
                  while ((j = DMA_READ(DeviceExtension->Adapter, WorkByteCountReg)) !=
                         DMA_READ(DeviceExtension->Adapter, WorkByteCountReg));
                  j &= 0x00ffffff;

                  //
                  // Check if there is any data inside DMA FIFO
                  //
                  if (j <= i || j & 0x00800000)
                    goto StopDMA;

                }

                //
                // Save current physical address
                //
                CurrentPhysicalAddress = DMA_READ(DeviceExtension->Adapter, WorkAddressReg);

                //
                // Check if two address readings match
                //
                do {

                  if (CurrentPhysicalAddress !=
                      DMA_READ(DeviceExtension->Adapter, WorkAddressReg)) {

                    //
                    // Read DMA working count register again
                    //
                    while ((j = DMA_READ(DeviceExtension->Adapter, WorkByteCountReg)) !=
                           DMA_READ(DeviceExtension->Adapter, WorkByteCountReg));
                    j &= 0x00ffffff;

                    //
                    // Read physical address again
                    //
                    CurrentPhysicalAddress = DMA_READ(DeviceExtension->Adapter, WorkAddressReg);

                  } else break;

                } while (1);

                //
                // Get end of data buffer physical address
                //
                k = DeviceExtension->ActiveDMAPointer + DeviceExtension->ActiveDMALength;

                //
                // Check if BLAST still needed
                //
                if (j <= i || j & 0x00800000 || 

#if defined DISABLE_MDL || defined T1

                    (CurrentPhysicalAddress >= k &&
                    !(DeviceExtension->AdapterFlags & PD_BUFFERED_DATA)) ||

#endif

                    (j - i) >= 64)
                  goto StopDMA;

                //
                // Added for debug
                //
                AMDPrint((0, "AMDInterruptServiceRoutine: Issuing blast command.\n"));

#ifndef DISABLE_MDL
#ifndef T1

                if (DeviceExtension->PciConfigInfo.ChipRevision >= 0x10) {

                  DMA_WRITE (DeviceExtension->Adapter, CommandReg, DMA_BLAST + 0x90);

                } else {

#endif
#endif

                //
                // Save number of data to blast
                //
                k = j - i;

                //
                // Check the number is valid or not
                //
                j = DeviceExtension->ActiveDMAPointer + DeviceExtension->ActiveDMALength;
                if ((CurrentPhysicalAddress + k) > j)
                  k = j - CurrentPhysicalAddress;

                //
                // If buffer data xfer flag set, do not have to buffer
                // again
                //
                if (!(DeviceExtension->AdapterFlags & PD_BUFFERED_DATA)) {

                  //
                  // Set BLAST flag
                  //
                  DeviceExtension->AdapterFlags |= PD_BUFFERED_DATA + PD_BUFFERED_BLAST_DATA;

                  //
                  // Set DMA address to tempary data buffer
                  //
                  DMA_WRITE(DeviceExtension->Adapter, StartTransferAddressReg, DeviceExtension->TempPhysicalBuf);

                }

                //
                // Issue BLAST command
                //
                DMA_WRITE (DeviceExtension->Adapter, CommandReg, DMA_BLAST + 0x80);

#ifndef DISABLE_MDL
#ifndef T1

                }

#endif
#endif

                //
                // Waiting until transfer done
                //
                do {

                  while ((j = DMA_READ(DeviceExtension->Adapter, WorkByteCountReg)) !=
                         DMA_READ(DeviceExtension->Adapter, WorkByteCountReg));
                  j &= 0x00ffffff;
                  if (j & 0x00800000)
                    goto StopDMA;

                } while (j > i);
              }
            } 

StopDMA:
            //
            // Stop the DMA
            //
            AMDFlushDMA(DeviceExtension);

            //
            // If this is a write then there may still be some bytes in the
            // FIFO which have yet to be transferred to the target.  This
            // may happen only in async. transfer with 16-bit DMA.  For 
            // sync. transfer, no data left in the FIFO when target changes
            // phase because target is not allowed to disconnect on odd byte
            // transfer boundary
            //
            if (DeviceExtension->dataPhase == DATA_OUT &&
                !(DeviceExtension->AdapterFlags & PD_DATA_OVERRUN)) {

              *((PUCHAR) &fifoFlags) = SCSI_READ(DeviceExtension->Adapter,
                                                 FifoFlags
                                                 );
  
              i += fifoFlags.ByteCount;

            }
           
            //
            // i now contains the number of bytes to be transferred.  Check 
            // to see if this the maximum that has be transferred so far,
            // and update the active data pointer and the active length.
            //
            AMDPrint((0,"Data transferred = %x, Remain data = %x\n",\
              DeviceExtension->ActiveDMALength - i,i));

            //
            // Update transfer count
            //
            SRB_EXT(srb)->TotalDataTransferred += DeviceExtension->ActiveDMALength - i;

#ifndef DISABLE_MDL
#ifndef T1

            if (DeviceExtension->PciConfigInfo.ChipRevision < 0x10)

#endif
#endif

            //
            // If tempoary buffer flag set and phase is data in, copy 
            // data from temporay buffer to data buffer
            //
            if (DeviceExtension->AdapterFlags & PD_BUFFERED_DATA &&
                DeviceExtension->dataPhase == DATA_IN) {

              //
              // If BLAST flag set, calculate data buffer linear address
              //
              if (DeviceExtension->AdapterFlags & PD_BUFFERED_BLAST_DATA) {

                //
                // Calculate linear address
                //
                if (DeviceExtension->SysFlags.DMAMode == IO_DMA_METHOD_LINEAR) {

                  CurrentDataPointer = DeviceExtension->ActiveDataPointer + 
                    DeviceExtension->ActiveDMALength - i - k;

                } else {

                  //
                  // Total number of data xfered
                  //
                  SRB_EXT(srb)->TotalDataTransferred -= k;

                  //
                  // Xlat physical address to linear address
                  //
//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      Fix warnings for 'improper member use'.
//
#ifdef SOLARIS
                  physicalAddress.u.LowPart = CurrentPhysicalAddress;
                  physicalAddress.u.HighPart = 0;
#else	// SOLARIS
                  physicalAddress.LowPart = CurrentPhysicalAddress;
                  physicalAddress.HighPart = 0;
#endif	// SOLARIS

#ifdef OTHER_OS

                  CurrentDataPointer =
                    (ULONG) ScsiPortGetVirtualAddress (DeviceExtension, 
                                                       physicalAddress,
                                                       srb,
                                                       srb->DataTransferLength
                                                      );

#else

                  CurrentDataPointer =
                    (ULONG) ScsiPortGetVirtualAddress (DeviceExtension, 
                                                       physicalAddress
                                                      );
#endif

                  //
                  // Restore value
                  //
                  SRB_EXT(srb)->TotalDataTransferred += k;

                }

                //
                // Move data from temp. buffer to data buffer
                //
                j = k;
                while (j--) {

                  *((PUCHAR) (CurrentDataPointer + j)) =
                    *((PUCHAR) (DeviceExtension->TempLinearBuf + j));

                }

                //
                // Check if there is still a byte in SCSI FIFO 
                //
                if (SCSI_READ (DeviceExtension->Adapter, FifoFlags) & 1) {

                  //
                  // There is data! Move the data out
                  //
                  *(PUCHAR)(CurrentDataPointer + k) = SCSI_READ (DeviceExtension->Adapter, Fifo);

                } 

              } else {

                for (j = 0; j < DeviceExtension->ActiveDMALength; j++) {
  
                  *((PUCHAR) (DeviceExtension->ActiveDataPointer + j)) =
                    *((PUCHAR) (DeviceExtension->TempLinearBuf + j));

                }
              }

#ifdef OTHER_OS

              //
              // Free the virtual address mapping if necessary
              //
              if (DeviceExtension->SysFlags.DMAMode != IO_DMA_METHOD_LINEAR) {

                if (DeviceExtension->AdapterFlags & PD_BUFFERED_BLAST_DATA) {

                  ScsiPortFreeVirtualAddress ((PVOID)CurrentDataPointer,
                                              srb,
                                              srb->DataTransferLength
                                             );

                } else {

                  ScsiPortFreeVirtualAddress ((PVOID)DeviceExtension->ActiveDataPointer,
                                              srb,
                                              srb->DataTransferLength
                                             );

                }
              }

#endif

              //
              // Clear buffer flags
              //
              DeviceExtension->AdapterFlags &= ~(PD_BUFFERED_DATA + PD_BUFFERED_BLAST_DATA);

            }

            //
            // If still have data left, update SG table and data pointer
            //
            if (SRB_EXT(srb)->TotalDataTransferred < DeviceExtension->ActiveDataLength) {

              //
              // Update data pointer
              //
              if (DeviceExtension->SysFlags.DMAMode == IO_DMA_METHOD_LINEAR) {

                DeviceExtension->ActiveDataPointer += DeviceExtension->ActiveDMALength - i;

              }

#ifdef OTHER_OS

              else {

                AMDUpdateSGTPointer (DeviceExtension, DeviceExtension->ActiveDMALength - i);

              }

#endif

            }

            //
            // Clear flags
            //
            DeviceExtension->AdapterFlags &= ~(PD_PENDING_DATA_TRANSFER + PD_DATA_OVERRUN);

            //
            // If all data transfered, check if target is still in data phase.
            // If yes, data overran and we must handle this.  Using DMA 
            // (for keeping sync. mode) transfer one byte each time until 
            // the target changes phase.
            //
            if (SRB_EXT(srb)->TotalDataTransferred >=
                DeviceExtension->ActiveDataLength &&
                (DeviceExtension->AdapterStatus.Phase == DATA_IN ||
                 DeviceExtension->AdapterStatus.Phase == DATA_OUT)) {

              //
              // Read current phase before dump data
              //
              *((PUCHAR) &DeviceExtension->AdapterStatus) =
                SCSI_READ (DeviceExtension->Adapter, ScsiStatus);

              //
              // If current phase is still in data phase, data overran
              //
              if (DeviceExtension->AdapterStatus.Phase == DATA_IN ||
                  DeviceExtension->AdapterStatus.Phase == DATA_OUT) {

                //
                // Added for debug
                //
                AMDPrint ((0,"AMDInterruptServiceRoutine: Data overran!\n"));

                //
                // Set data overran flag
                //
                DeviceExtension->AdapterFlags |= PD_DATA_OVERRUN;

              }
            }

        } else if (DeviceExtension->AdapterState == DisconnectExpected) {

            //
            // This is an error; however, some contollers attempt to read more
            // message bytes even after a message indicating a disconnect.
            // If the request is for a message transfer and extra bytes
            // are expected, then allow the transfer; otherwise, reset the bus.
            //
            if (!(DeviceExtension->AdapterFlags & PD_POSSIBLE_EXTRA_MESSAGE_OUT)
                || (DeviceExtension->AdapterStatus.Phase != MESSAGE_OUT &&
                DeviceExtension->AdapterStatus.Phase != MESSAGE_IN)) {

                //
                // If a disconnect was expected and a bus service interrupt was
                // detected, then a SCSI protocol error has been detected and the
                // SCSI bus should be reset to clear the condition.
                //
                AMDPrint((0, "AMDInterruptServiceRoutine: Bus request while disconnect expected.\n"));
                AMDErrorHandle (DeviceExtension, SP_PROTOCOL_ERROR, 7);
                return(TRUE);

            } else {

                //
                // Make sure the disconnect-expected flag is set.
                //
                DeviceExtension->AdapterFlags |= PD_DISCONNECT_EXPECTED;
            }

        } else if (DeviceExtension->AdapterState == MessageOut) {

            //
            // The SCSI protocol chip indicates that the message has been sent;
            // however, the target may need to reread the message or there
            // may be more messages to send.  This condition is indicated by a
            // message-out bus phase; otherwise, the message has been accepted
            // by the target.  If message has been accepted then check to see
            // if any special processing is necessary.  Note that the driver
            // state is set to MessageOut after the PD_DISCONNECT_EXPECTED is
            // set, or after a selection.  So it is only necessary to check for
            // PD_DISCONNECT_EXPECTED when the driver state is currently in
            // MessageOut.
            //
            if (DeviceExtension->AdapterFlags & (PD_DISCONNECT_EXPECTED |
                PD_SYNCHRONOUS_TRANSFER_SENT | PD_SYNCHRONOUS_RESPONSE_SENT) &&
                DeviceExtension->AdapterStatus.Phase != MESSAGE_OUT &&
                DeviceExtension->AdapterStatus.Phase != MESSAGE_IN) {

                if (DeviceExtension->AdapterFlags & PD_DISCONNECT_EXPECTED) {

                    //
                    // If a disconnect was expected and a bus service interrupt was
                    // detected, then a SCSI protocol error has been detected and the
                    // SCSI bus should be reset to clear the condition.
                    //
                    AMDPrint((0, "AMDInterruptServiceRoutine: Bus request while disconnect expected after message-out.\n"));
                    AMDErrorHandle (DeviceExtension, SP_PROTOCOL_ERROR, 8);
                    return(TRUE);

                } else if (DeviceExtension->AdapterFlags &
                           PD_SYNCHRONOUS_TRANSFER_SENT) {

                    //
                    // The controller ignored the synchronous transfer message.
                    // Treat it as a rejection and clear the necessary state.
                    //
                    DeviceExtension->ActiveLogicalUnit->LuFlags |=
                        PD_SYNCHRONOUS_NEGOTIATION_DONE | PD_DO_NOT_NEGOTIATE;
                    DeviceExtension->AdapterFlags &=
                        ~(PD_SYNCHRONOUS_RESPONSE_SENT|
                        PD_SYNCHRONOUS_TRANSFER_SENT);

                } else if (DeviceExtension->AdapterFlags &
                           PD_SYNCHRONOUS_RESPONSE_SENT) {

                    //
                    // The target controller accepted the negotiation. Set
                    // the done flag in the logical unit and clear the
                    // negotiation flags in the adapter.
                    //
                    DeviceExtension->ActiveLogicalUnit->LuFlags |=
                        PD_SYNCHRONOUS_NEGOTIATION_DONE | PD_DO_NOT_NEGOTIATE;
                    DeviceExtension->AdapterFlags &=
                        ~(PD_SYNCHRONOUS_RESPONSE_SENT|
                        PD_SYNCHRONOUS_TRANSFER_SENT);

                }
            }
        }

        //
        // If the bus phase is not DATA_IN then the FIFO may need to be
        // flushed.  The FIFO cannot be flushed while the bus is in the
        // DATA_IN phase because the FIFO already has some data in it.
        // The only case where a target can legally switch phases while
        // there are message bytes in the FIFO to the MESSAGE_OUT bus
        // phase. If the target leaves message bytes and attempts to
        // goto a DATA_IN phase, then the transfer will appear to overrun
        // and be detected as an error.
        //
        if (DeviceExtension->AdapterStatus.Phase != DATA_IN) {

            SCSI_WRITE(DeviceExtension->Adapter, Command, FLUSH_FIFO);

        }

        //
        // Decode the current bus phase.
        //
        switch (DeviceExtension->AdapterStatus.Phase) {

        case COMMAND_OUT:

            //
            // Added in for debug 
            //
            AMDPrint((0,"AMDInterruptServiceRoutine: Bus Service Interrupt->Command Phase.\n"));

            //
            // Fill the FIFO with the commnad and tell the SCSI protocol chip
            // to go.
            //
            for (i = 0; i < srb->CdbLength; i++) {

              SCSI_WRITE( DeviceExtension->Adapter,
                          Fifo,
                          srb->Cdb[i]
                          );

                  //
                  // Added in for debug 
                  //
                  AMDPrint((0,"%x- ",srb->Cdb[i]));

            }

            //
            // Added in for debug 
            //
            AMDPrint((0,"\n"));

            SCSI_WRITE( DeviceExtension->Adapter,
                        Command,
                        TRANSFER_INFORMATION
                        );

            DeviceExtension->AdapterState = CommandOut;

            break;

        case STATUS_IN:

            //
            // Added in for debug 
            //
            AMDPrint((0,"AMDInterruptServiceRoutine: Bus Service Interrupt->Status Phase.\n"));

            //
            // Setup of the SCSI protocol chip to read in the status and the
            // following message byte, and set the adapter state.
            //
            SCSI_WRITE( DeviceExtension->Adapter, Command, COMMAND_COMPLETE );
            DeviceExtension->AdapterState = CommandComplete;

            break;

        case MESSAGE_OUT:

            //
            // Added in for debug 
            //
            AMDPrint((0,"AMDInterruptServiceRoutine: Bus Service Interrupt->MessageOut Phase.\n"));

            //
            // The target is requesting a message-out.  There are three
            // possible cases.  First, the target is improperly requesting
            // a message. Second, a message has been sent, but the target
            // could not read it properly.  Third, a message has been
            // partially sent and the target is requesting the remainder
            // of the message.
            //
            // The first case is indicated when the MessageCount is zero or
            // the message-out flag is not set.
            //
            if ( DeviceExtension->MessageCount == 0 ||
                !(DeviceExtension->AdapterFlags & PD_MESSAGE_OUT_VALID)) {

                //
                // If extra message-outs are possible then just send a NOP
                // message.
                //
                if (DeviceExtension->AdapterFlags &
                    PD_POSSIBLE_EXTRA_MESSAGE_OUT) {

                    //
                    // Set the message to NOP and clear the extra message
                    // flag.  This is a hack for controllers that do not
                    // properly read the entire message.
                    //
                    DeviceExtension->MessageBuffer[0] = SCSIMESS_NO_OPERATION;
                    DeviceExtension->AdapterFlags &=
                        ~PD_POSSIBLE_EXTRA_MESSAGE_OUT;

                } else {

                    //
                    // Send an INITIATOR DETECTED ERROR message.
                    //
                    DeviceExtension->MessageBuffer[0] =
                        SCSIMESS_INIT_DETECTED_ERROR;
                    AMDLogError(DeviceExtension, SP_PROTOCOL_ERROR, 9);
                    AMDPrint((0, "AMDInterruptServiceRoutine: Unexpected message-out request\n"));

                }

                DeviceExtension->MessageCount = 1;
                DeviceExtension->MessageSent = 0;
                DeviceExtension->AdapterState = MessageOut;

            }

            //
            // The second case is indicated when MessageCount and MessageSent
            // are equal and nonzero.
            //
            if (DeviceExtension->MessageCount == DeviceExtension->MessageSent){

                //
                // The message needs to be resent, so set ATN, clear MessageSent
                // and fall through to the next case.
                //
                SCSI_WRITE(DeviceExtension->Adapter, Command, SET_ATTENTION);
                DeviceExtension->MessageSent = 0;
            }

            if (DeviceExtension->MessageCount != DeviceExtension->MessageSent){

                //
                // The ATTENTION signal needs to be set if the current state
                // is not MessageOut.
                //
                if (DeviceExtension->AdapterState != MessageOut) {

                    SCSI_WRITE(
                        DeviceExtension->Adapter,
                        Command,
                        SET_ATTENTION
                        );
                }

                //
                // There is more message to send.  Fill the FIFO with the
                // message and tell the SCSI protocol chip to transfer the
                // message.
                //
                for (;
                     DeviceExtension->MessageSent <
                     DeviceExtension->MessageCount;
                     DeviceExtension->MessageSent++ ) {

                    SCSI_WRITE(DeviceExtension->Adapter,
                               Fifo,
                               DeviceExtension->
                               MessageBuffer[DeviceExtension->MessageSent]
                               );

                }

                SCSI_WRITE(DeviceExtension->Adapter,
                           Command,
                           TRANSFER_INFORMATION
                           );

            }

            break;

        case MESSAGE_IN:

            //
            // Added in for debug 
            //
            AMDPrint((0,"AMDInterruptServiceRoutine: Bus Service Interrupt->MessageIn Phase.\n"));

            //
            // If this is the first byte of the message then initialize
            // MessageCount and the adapter state.  The message buffer
            // cannot overflow because the message decode function will
            // take care of the message before the buffer is full.
            // The SCSI protocol chip will interrupt for each message
            // byte.
            //
            if ( DeviceExtension->AdapterState != MessageIn &&
                 DeviceExtension->AdapterState != MessageAccepted ) {

                DeviceExtension->AdapterFlags &= ~PD_MESSAGE_OUT_VALID;
                DeviceExtension->MessageCount = 0;
            }

            DeviceExtension->AdapterState = MessageIn;

            SCSI_WRITE( DeviceExtension->Adapter,
                        Command,
                        TRANSFER_INFORMATION
                        );

            break;

        case DATA_OUT:
        case DATA_IN:

            //
            // Added in for debug 
            //
            AMDPrint((0,"AMDInterruptServiceRoutine: Bus Service Interrupt->Data Phase.\n"));

            //
            // If this is first data xfer
            //
            if (DeviceExtension->AdapterState != DataTransfer) {

              //
              // Save the phase information for auto direction function
              //
              DeviceExtension->dataPhase = DeviceExtension->AdapterStatus.Phase;

            }

            //
            // Check that the transfer direction is ok, setup the DMA, set
            // the synchronous transfer parameter, and tell the chip to go.
            // Also check that there is still data to be transferred.
            //
            if ((DeviceExtension->dataPhase != DATA_IN &&
              DeviceExtension->AdapterStatus.Phase == DATA_IN) ||
              (DeviceExtension->dataPhase != DATA_OUT &&
              DeviceExtension->AdapterStatus.Phase == DATA_OUT) ||
              DeviceExtension->ActiveDataLength == 0) {
                
              //
              // The data direction is incorrect. Reset the bus to clear
              // things up.
              //
              AMDPrint((0, "AMDInterruptServiceRoutine: Illegal transfer direction.\n"));
              AMDErrorHandle (DeviceExtension, SP_PROTOCOL_ERROR, 10);
              return(TRUE);

            }
            
            //
            // Set up transfer flags
            //
            DeviceExtension->AdapterState = DataTransfer;
            DeviceExtension->AdapterFlags |= PD_PENDING_DATA_TRANSFER;

            //
            // Transfer data
            //
            AMDStartDataTransfer (DeviceExtension);

            break;

        default:

            //
            // This phase is illegal and indicates a serious error. Reset the
            // bus to clear the problem.
            //
            AMDPrint((0, "AMDInterruptServiceRoutine: Illegal bus state detected.\n"));
            AMDErrorHandle (DeviceExtension, SP_PROTOCOL_ERROR, 11);
            return(TRUE);
        }
    }

    //
    // If target flag set for no-target-exist, exit (state register has
    // already been updated.)
    //
    if (targetStatus == TargetNotExist) {

      return(TRUE);

    }

    //
    // Update SCSI phase information bits in the PCI configuration register
    //
    if (targetStatus == BusFree) {

      //
      // Check if device idle.
      //
      if (message == SCSIMESS_COMMAND_COMPLETE) {

#ifndef DISABLE_SREG

        AMDUpdateInfo (DeviceExtension, TargetIdle);

#endif

#ifdef COMPAQ_MACHINE

        //
        // If this is a Compaq system, turn disk LED off
        //
        if (DeviceExtension->SysFlags.MachineType == COMPAQ) {

          //
          // This is a Compaq system.  Turn disk LED off
          //
          AMDTurnLEDOn (FALSE);

        }

#endif

      }
	
#ifndef DISABLE_SREG

      else {

        AMDUpdateInfo (DeviceExtension, TargetDisconnected);
     
      }

    } else {

      AMDUpdateInfo (DeviceExtension, Phase);

#endif

    }

    return(TRUE);
}

LOCAL
VOID
AMDLogError(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN ULONG ErrorCode,
    IN ULONG UniqueId
    )
/*++

Routine Description:

    This routine logs an error.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension for the
        port adapter to which the completing target controller is connected.

    ErrorCode - Supplies the error code to log with the error.

    UniqueId - Supplies the unique error identifier.

Return Value:

    None.

--*/
{

    PSCSI_REQUEST_BLOCK srb;

    //
    // Look for a current request in the device extension.
    //
    if (DeviceExtension->ActiveLogicalUnit != NULL) {

        if (DeviceExtension->ActiveLuRequest != NULL) {

            srb = DeviceExtension->ActiveLuRequest;

        } else {

            srb = DeviceExtension->ActiveLogicalUnit->ActiveSendRequest;

        }

    } else {

        srb = DeviceExtension->NextSrbRequest;

    }

    //
    // If the srb is NULL, then log the error against the host adapter address.
    //
    if (srb == NULL) {

        ScsiPortLogError(
            DeviceExtension,                        //  HwDeviceExtension,
            NULL,                                   //  Srb
            0,                                      //  PathId,
            DeviceExtension->AdapterBusId,          //  TargetId,
            0,                                      //  Lun,
            ErrorCode,                              //  ErrorCode,
            UniqueId                                //  UniqueId
            );

    } else {

        ScsiPortLogError(
            DeviceExtension,                        //  HwDeviceExtension,
            srb,                                    //  Srb
            srb->PathId,                            //  PathId,
            srb->TargetId,                          //  TargetId,
            srb->Lun,                               //  Lun,
            ErrorCode,                              //  ErrorCode,
            UniqueId                                //  UniqueId
            );

    }
}

LOCAL
VOID
AMDProcessRequestCompletion(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine does all of checking and state updating necessary when a
    request terminates normally.  It determines what the SrbStatus
    should be and updates the state in the DeviceExtension, the
    logicalUnitExtension and the srb.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension for the
        port adapter on to which the completing target controller is connected.

Return Value:

    None.

--*/

{
    PSPECIFIC_LOGICAL_UNIT_EXTENSION luExtension;
    PSCSI_REQUEST_BLOCK srb;

    //
    // Added for debug 
    //
    AMDPrint((0,"AMDProcessRequestCompletion.\n"));
    SetBreakPoint

    luExtension = DeviceExtension->ActiveLogicalUnit;
    srb = DeviceExtension->ActiveLuRequest;

    //
    // If status already set(PCI abort happened), clear data structure
    // and return
    //

#ifndef T1

    if (srb->SrbStatus != SRB_STATUS_PHASE_SEQUENCE_FAILURE) {

#endif

      if (srb->ScsiStatus != SCSISTAT_GOOD) {

          //
          // Indicate an abnormal status code.
          //
          srb->SrbStatus = SRB_STATUS_ERROR;

          //
          // If this is a check condition, then clear the synchronous negotiation
          // done flag.  This is done in case the controller was power cycled.
          //
          if (srb->ScsiStatus == SCSISTAT_CHECK_CONDITION) {

            luExtension->LuFlags &= ~PD_SYNCHRONOUS_NEGOTIATION_DONE;
  
          }
  
          //
          // If there is a pending request for this logical unit then return
          // that request with a busy status.  This situation only occurs when
          // command queuing is enabled an a command pending for a logical unit
          // at the same time that an error has occured.  This may be a BUSY,
          // QUEUE FULL or CHECK CONDITION.  The important case is CHECK CONDITION
          // because a contingatent aligance condition has be established and the
          // port driver needs a chance to send a Reqeust Sense before the
          // pending command is started.
          //
          if (DeviceExtension->AdapterFlags & PD_PENDING_START_IO &&
             DeviceExtension->NextSrbRequest->PathId == srb->PathId &&
             DeviceExtension->NextSrbRequest->TargetId == srb->TargetId &&
             DeviceExtension->NextSrbRequest->Lun == srb->Lun) {
   
             AMDPrint((0, "AMDProcessRequestCompletion: Failing request with busy status due to check condition\n"));
             DeviceExtension->NextSrbRequest->SrbStatus = SRB_STATUS_ABORTED;
             DeviceExtension->NextSrbRequest->ScsiStatus = SCSISTAT_BUSY;
   
             //
             // Make sure the request is not sitting in the logical unit.
             //
             if (DeviceExtension->NextSrbRequest == luExtension->ActiveLuRequest) {
   
                 luExtension->ActiveLuRequest = NULL;
   
             } else if (DeviceExtension->NextSrbRequest ==
                        luExtension->ActiveSendRequest) {
   
                 luExtension->ActiveSendRequest = NULL;
  
             }
   
             ScsiPortNotification(
               RequestComplete,
               DeviceExtension,
               DeviceExtension->NextSrbRequest
               );
   
             DeviceExtension->AdapterFlags &= ~PD_PENDING_START_IO;
             DeviceExtension->NextSrbRequest = NULL;
  
             ScsiPortNotification(
               NextRequest,
               DeviceExtension,
               NULL
               );
          }
          
      } else {
  
          //
          // Everything looks correct so far.
          //
          srb->SrbStatus = SRB_STATUS_SUCCESS;
  
          //
          // Make sure that status is valid.
          //
          if (!(SRB_EXT(srb)->SrbExtensionFlags & PD_STATUS_VALID)) {
  
              //
              // The status byte is not valid.
              //
              srb->SrbStatus = SRB_STATUS_PHASE_SEQUENCE_FAILURE;
  
              //
              // Log the error.
              //
              AMDLogError(DeviceExtension, SP_PROTOCOL_ERROR, 12);
  
          }
      }
  
      //
      // Check that data was transferred to the end of the buffer.
      //
      if ( SRB_EXT(srb)->TotalDataTransferred != srb->DataTransferLength ) {
  
          //
          // The entire buffer was not transferred.  Update the length
          // and update the status code.
          //
          if (srb->SrbStatus == SRB_STATUS_SUCCESS) {
  
              AMDPrint((0, "AMDProcessRequestCompletion: Short transfer, Actual: %x; Expected: %x;\n",
                  SRB_EXT(srb)->TotalDataTransferred,
                  srb->DataTransferLength
                  ));
  
              //
              // If no data was transferred then indicated this was a
              // protocol error rather than a data under/over run.
              //
              if (srb->DataTransferLength == 0) {
  
                  srb->SrbStatus = SRB_STATUS_PHASE_SEQUENCE_FAILURE;
  
              } else {
  
                  srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
  
              }
  
              srb->DataTransferLength = SRB_EXT(srb)->TotalDataTransferred;
  
          } else {
  
              //
              // Update the length if a check condition was returned.
              //
              if (srb->ScsiStatus == SCSISTAT_CHECK_CONDITION) {
  
                  srb->DataTransferLength = SRB_EXT(srb)->TotalDataTransferred;
  
              }
          }
      }

      if (srb->SrbStatus != SRB_STATUS_SUCCESS) {
  
        AMDPrint((0, "AMDProcessRequestCompletion: Request failed. ScsiStatus: %x, SrbStatus: %x\n",
                  srb->ScsiStatus, srb->SrbStatus));
  
      }

#ifndef T1

    }

#endif
  
    //
    // Clear the request but not the ActiveLogicalUnit since the target has
    // not disconnected from the SCSI bus yet.
    //
    luExtension->ActiveLuRequest = NULL;
  
    luExtension->RetryCount = 0;
    luExtension->LuFlags &= ~PD_LU_COMPLETE_MASK;
}

LOCAL
VOID
AMDErrorHandle(
    PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN ULONG ErrorCode,
    IN ULONG UniqueCode
    )

/*++

Routine Description:

    This function handles the errors which needs system reset

Arguments:

    DeviceExtension - Supplies a pointer to the specific device extension.

    ErrorCode - Supplies a error code for error log

    UniqueCode - Supplies the unique error identifier.

Return Value:

    None.

--*/

{

    //
    // Reset SCSI bus
    //
    AMDResetScsiBusInternal(DeviceExtension, 0);

    //
    // Reset adapter
    //
    AMDInitializeAdapter(DeviceExtension);

    //
    // Log error
    //
    AMDLogError(DeviceExtension, ErrorCode, UniqueCode);

}

LOCAL
BOOLEAN
AMDResetScsiBus(
    IN PVOID ServiceContext,
    IN ULONG PathId
    )

/*++

Routine Description:

    This function resets the SCSI bus and calls the reset cleanup function.

Arguments:

    ServiceContext  - Supplies a pointer to the specific device extension.

    PathId - Supplies the path id of the bus.

Return Value:

    TRUE - Indicating the reset is complete.

--*/

{
    PSPECIFIC_DEVICE_EXTENSION DeviceExtension = ServiceContext;

    AMDPrint((0, "AMDResetScsiBus: Resetting the SCSI bus.\n"));

    //
    // The bus should be reset regardless of what is occurring on the bus or in
    // the chip. The reset SCSI bus command executes immediately.
    //
    SCSI_WRITE(DeviceExtension->Adapter, Command, RESET_SCSI_BUS);

    //
    // Delay the minimum assertion time for a SCSI bus reset to make sure a
    // valid reset signal is sent.
    //
    ScsiPortStallExecution( RESET_STALL_TIME );

    AMDCleanupAfterReset(DeviceExtension, FALSE);
    DeviceExtension->AdapterFlags |= PD_EXPECTING_RESET_INTERRUPT;

    return(TRUE);
}

LOCAL
VOID
AMDResetScsiBusInternal(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN ULONG PathId
    )
/*++

Routine Description:

    This function resets the SCSI bus and notifies the port driver.

Arguments:

    DeviceExtension  - Supplies a pointer to the specific device extension.

    PathId - Supplies the path id of the bus.

Return Value:

    None

--*/
{

    //
    // Added for debug 
    //
    AMDPrint((0,"AMDResetScsiBusInternal.\n"));

    AMDResetScsiBus(DeviceExtension, 0);

    ScsiPortNotification(
        ResetDetected,
        DeviceExtension,
        NULL
        );
}

LOCAL
BOOLEAN
AMDSelectTarget(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN PSPECIFIC_LOGICAL_UNIT_EXTENSION LuExtension
    )
/*++

Routine Description:

    This routine sets up the hardware to select a target.  If a valid message
    is in the message buffer, it will be sent to the target.  If the request
    includes a SCSI command descriptor block, it will also be passed to the
    target.

Arguments:

    DeviceExtension - Supplies the device extension for this HBA adapter.

    LuExtension - Supplies the logical unit extension for the target being
        selected.

Return Value:

    TRUE - Request executed.
    FALSE - Request failed.

--*/

{
    PSCSI_REQUEST_BLOCK srb;
    ULONG i;

    srb = DeviceExtension->NextSrbRequest;

    //
    // Added in for debug 
    //
    AMDPrint((0,"AMDSelectTarget: Attempting target select %d.\n",srb->TargetId));
     
    /* Powerfail Start */

    //
    // Set up the SCSI protocol chip to select the target, transfer the
    // IDENTIFY message and the CDB.  This can be done by following steps:
    //
    //        setting the destination register,
    //        filling the FIFO with the IDENTIFY message and the CDB
    //        setting the command register
    //
    // If the chip is not interrupting, then set up for selection.  If the
    // chip is interrupting then return.  The interrupt will process the
    // request.  Note that if we get reselected after this point the chip
    // will ignore the bytes written until the interrupt register is read.
    // The commands that handle a message and a CDB can only be used if the
    // message is one byte or 3 bytes long; otherwise only a one-byte message
    // is transferred on the select and the remaining bytes are handled in the
    // interrupt routine.
    //
    if (DMA_READ (DeviceExtension->Adapter, StatusReg) & SCSICoreInterrupt) {

      return FALSE;

    }

    //
    // Set the destination ID.  Put the first byte of the message-in
    // the fifo and set the command to select with ATN.  This command
    // selects the target, sends one message byte and interrupts.  The
    // ATN line remains set.  The succeeding bytes are loaded into the
    // FIFO and sent to the target by the interrupt service routine.
    //
    SCSI_WRITE(DeviceExtension->Adapter, DestinationId, srb->TargetId);
    SCSI_WRITE( DeviceExtension->Adapter,
                Fifo,
                DeviceExtension->MessageBuffer[DeviceExtension->MessageSent++]
                );

    //
    // Set the synchronous data transfer parameter registers in case a
    // data transfer is done.  These must be set before a data transfer
    // is started.
    //
    SCSI_WRITE( DeviceExtension->Adapter,
                SynchronousPeriod,
                LuExtension->SynchronousPeriod
                );
    SCSI_WRITE( DeviceExtension->Adapter,
                SynchronousOffset,
                LuExtension->SynchronousOffset
                );

    SCSI_WRITE( DeviceExtension->Adapter,
                Configuration3,
                *((PCHAR) &LuExtension->Configuration3)
                );
    SCSI_WRITE( DeviceExtension->Adapter,
                Configuration4,
                *((PCHAR) &LuExtension->Configuration4)
                );

    //
    // Determine if this srb has a Cdb with it and whether the message is such that
    // the message and the Cdb can be loaded into the fifo; otherwise, just
    // load the first byte of the message.
    //
    if (srb->Function == SRB_FUNCTION_EXECUTE_SCSI &&
        (DeviceExtension->MessageCount == 1 ||
        DeviceExtension->MessageCount == 3)) {

        //
        // If bus busy, return
        //
        if (DMA_READ (DeviceExtension->Adapter, StatusReg) & SCSICoreInterrupt) {
   
          return FALSE;

        }

        //
        // Copy the entire message and Cdb into the fifo.
        //
        for (;
             DeviceExtension->MessageSent <
             DeviceExtension->MessageCount;
             DeviceExtension->MessageSent++ ) {

            SCSI_WRITE( DeviceExtension->Adapter,
                        Fifo,
                        DeviceExtension->
                        MessageBuffer[DeviceExtension->MessageSent]
                        );

        }

        for (i = 0; i < srb->CdbLength; i++) {

          SCSI_WRITE(DeviceExtension->Adapter,
                     Fifo,
                     srb->Cdb[i]
                     );

          //
          // Added in for debug 
          //
          AMDPrint((0,"%x - ",srb->Cdb[i]));

        }

        //
        // If bus busy, return
        //
        if (DMA_READ (DeviceExtension->Adapter, StatusReg) & SCSICoreInterrupt) {
   
          return FALSE;

        }

        //
        // Added in for debug 
        //
        AMDPrint((0,"\n"));

        if (DeviceExtension->MessageCount == 1) {

          //
          // One message byte so use select with attention which uses one
          // message byte.
          //
          SCSI_WRITE(
              DeviceExtension->Adapter,
              Command,
              SELECT_WITH_ATTENTION
              );

        } else {

          //
          // Three byte message, so use the select with attention which uses
          // three byte messages.
          //
          SCSI_WRITE(
              DeviceExtension->Adapter,
              Command,
              SELECT_WITH_ATTENTION3
              );
        }

    } else {

        //
        // If bus busy, return
        //
        if (DMA_READ (DeviceExtension->Adapter, StatusReg) & SCSICoreInterrupt) {
   
          return FALSE;

        }

        //
        // Only the first byte of the message can be sent so select with
        // ATTENTION and the target will request the rest.
        //
        SCSI_WRITE(
            DeviceExtension->Adapter,
            Command,
            SELECT_WITH_ATTENTION_STOP
            );
    }

    //
    // Set the device state to message-out and indicate that a message
    // is being sent.
    //
    DeviceExtension->AdapterFlags |= PD_MESSAGE_OUT_VALID;
    DeviceExtension->AdapterState = AttemptingSelect;

    //
    // Return good status
    //
    return TRUE;

}

VOID
AMDSendMessage(
    PSCSI_REQUEST_BLOCK Srb,
    PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    PSPECIFIC_LOGICAL_UNIT_EXTENSION LuExtension
    )

/*++

Routine Description:

    This routine attempts to send the indicated message to the target
    controller.  There are three classes of messages:
        Those which terminate a specific request and end in bus free.
        Those which apply to a specific request and then proceed.
        Those which end in bus free.

    For those messages that apply to a specific request, check to see that
    the request is currently being processed and an INDENTIFY message prefixed
    to the message.

    It is possible that the destination logical unit is the active logical unit;
    however, it would difficult to jump in and send the requested message, so
    just wait for the bus to become free.

    In the case where the target is not currently active, then set up the SCSI
    protocol chip to select the target controller and send the message.

Arguments:

    Srb - Supplies the request to be started.

    DeviceExtension - Supplies the extended device extension for this SCSI bus.

    LuExtension - Supplies the logical unit extension for this request.

Notes:

    This routine must be synchronized with the interrupt routine.

Return Value:

    None

--*/
{
    PSCSI_REQUEST_BLOCK linkedSrb;
    BOOLEAN impliesDisconnect;
    BOOLEAN useTag;
    UCHAR message;

    //
    // Added for debug 
    //
    AMDPrint((0,"AMDSendMessage.\n"));
    SetBreakPoint

    impliesDisconnect = FALSE;
    useTag = FALSE;

    //
    // Decode the type of message.
    //
    switch (Srb->Function) {

    case SRB_FUNCTION_TERMINATE_IO:
    case SRB_FUNCTION_ABORT_COMMAND:

        //
        // Verify that the request is being processed by the logical unit.
        //
        linkedSrb = ScsiPortGetSrb(
            DeviceExtension,
            Srb->PathId,
            Srb->TargetId,
            Srb->Lun,
            Srb->QueueTag
            );

        if (linkedSrb != Srb->NextSrb) {

            //
            // The specified request is not here.  Complete the request
            // without error.
            //
            Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;
            ScsiPortNotification(
                RequestComplete,
                DeviceExtension,
                Srb
                );

            ScsiPortNotification(
                NextRequest,
                DeviceExtension,
                NULL
                );

            return;
        }

        message = Srb->Function == SRB_FUNCTION_ABORT_COMMAND ?
            SCSIMESS_ABORT : SCSIMESS_TERMINATE_IO_PROCESS;
        impliesDisconnect = TRUE;

        //
        // Use a tagged message if the original request was tagged.
        //
        useTag = linkedSrb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE ?
            TRUE : FALSE;

        break;

    case SRB_FUNCTION_RESET_DEVICE:

        //
        // Because of the way the chip works it is easiest to send an IDENTIFY
        // message along with the BUS DEVICE RESET message. That is because
        // there is no way to select a target with ATN and send one message
        // byte.  This IDENTIFY message is not necessary for the SCSI protocol,
        // but it is legal and should not cause any problem.
        //
        message = SCSIMESS_BUS_DEVICE_RESET;
        impliesDisconnect = TRUE;
        break;

    default:

        //
        // This is an unsupported message request. Fail the request.
        //
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        ScsiPortNotification(
            RequestComplete,
            DeviceExtension,
            Srb
            );

        ScsiPortNotification(
            NextRequest,
            DeviceExtension,
            NULL
            );

        return;
    }

    //
    // Save away the parameters in case nothing can be done now.
    //
    DeviceExtension->NextSrbRequest = Srb;
    DeviceExtension->AdapterFlags |= PD_PENDING_START_IO;

    //
    // Check to see if the bus is free.  If it is not, then return.  Since
    // the request parameters have been saved, indicate that the request has
    // been accepted.  The request will be processed when the bus becomes free.
    //
    if (DeviceExtension->AdapterState != BusFree) {
        return;
    }

    //
    // Save the sending message request
    //
    LuExtension->ActiveSendRequest = Srb;

    //
    // Create the identify command and copy the message to the buffer.
    //
    DeviceExtension->MessageBuffer[0] = SCSIMESS_IDENTIFY_WITH_DISCON |
        Srb->Lun;
    DeviceExtension->MessageCount = 1;
    DeviceExtension->MessageSent = 0;

    if (useTag && Srb->QueueTag != SP_UNTAGGED) {
        DeviceExtension->MessageBuffer[DeviceExtension->MessageCount++] = SCSIMESS_SIMPLE_QUEUE_TAG;
        DeviceExtension->MessageBuffer[DeviceExtension->MessageCount++] = Srb->QueueTag;

        if (message == SCSIMESS_ABORT) {
            message = SCSIMESS_ABORT_WITH_TAG;
        }
    }

    DeviceExtension->MessageBuffer[DeviceExtension->MessageCount++] = message;

    //
    // Attempt to select the target and update the adapter flags.
    //
    if (AMDSelectTarget( DeviceExtension, LuExtension ) == FALSE) {

      return;

    }

    DeviceExtension->AdapterFlags |= impliesDisconnect ?
        PD_DISCONNECT_EXPECTED | PD_SEND_MESSAGE_REQUEST
        : PD_SEND_MESSAGE_REQUEST;

}

LOCAL
VOID
AMDStartExecution(
    PSCSI_REQUEST_BLOCK Srb,
    PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    PSPECIFIC_LOGICAL_UNIT_EXTENSION LuExtension
    )

/*++

Routine Description:

    This procedure sets up the chip to select the target and notify it that
    a request is available.  For the AMD chip, the chip is set up to select,
    send the IDENTIFY message and send the command data block.  A check is
    made to determine if synchronous negotiation is necessary.

Arguments:

    Srb - Supplies the request to be started.

    DeviceExtension - Supplies the extended device extension for this SCSI bus.

    LuExtension - Supplies the logical unit extension for this requst.

Notes:

    This routine must be synchronized with the interrupt routine.

Return Value:

    None

--*/

{

    PSCSI_EXTENDED_MESSAGE extendedMessage;
    CHIP_TYPES chipType;
    UCHAR targetId;
    ULONG PCIRegister;
    USHORT regData;

#ifdef OTHER_OS

    ULONG length;

#endif

    //
    // Added for debug 
    //
    AMDPrint((0,"AMDStartExecution.\n"));
    SetBreakPoint

    //
    // Active negation feature can be enabled any time.  Update this field 
    // for every SCSI command
    //
    LuExtension->Configuration4.ActiveNegation =
      DeviceExtension->Configuration4.ActiveNegation;

    //
    // Save away the parameters in case nothing can be done now.
    //
    DeviceExtension->NextSrbRequest = Srb;
    DeviceExtension->AdapterFlags |= PD_PENDING_START_IO;

    //
    // Check to see if the bus is free.  If it is not, then return.  Since
    // the request parameters have been saved, indicate that the request has
    // been accepted.  The request will be processed when the bus becomes free.
    //
    if (DeviceExtension->AdapterState != BusFree ||
        DMA_READ (DeviceExtension->Adapter, StatusReg) & SCSICoreInterrupt) {

      return;

    }

#ifdef COMPAQ_MACHINE

    //
    // Check if there is Compaq system.
    //
    if (DeviceExtension->SysFlags.MachineType == COMPAQ) {

      //
      // This is a Compaq system.  Turn disk LED on
      //
		AMDTurnLEDOn (TRUE);

    }

#endif

#ifdef OTHER_OS

    //
    // Save data pointer
    //
    if (DeviceExtension->SysFlags.DMAMode != IO_DMA_METHOD_LINEAR) {

      //
      // Save SG entry
      //
      SRB_EXT(Srb)->SavedSGEntry = Srb->DataBuffer;

#ifndef DISABLE_MDL

      //
      // If MDL input, save offset and make first entry be page address
      //
      if (DeviceExtension->SysFlags.DMAMode == IO_DMA_METHOD_MDL) {

        //
        // Get MDL physical address
        //
        SRB_EXT(Srb)->SavedSGPhyscialEntry =
          (PULONG)ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(DeviceExtension,
                                       NULL,
                                       Srb->DataBuffer,
                                       &length));

        //
        // Save entry offset
        //
        SRB_EXT(Srb)->SavedSGTEntryOffset = *((PULONG)Srb->DataBuffer) & PAGE_MASK;

        //
        // Make first entry be page address
        //
        *((PULONG)Srb->DataBuffer) &= ~PAGE_MASK;

      } else

#endif
        
      SRB_EXT(Srb)->SavedSGTEntryOffset = 0;


    } else

#endif

    SRB_EXT(Srb)->SavedDataPointer = (ULONG) Srb->DataBuffer;

    SRB_EXT(Srb)->SavedDataLength = Srb->DataTransferLength;
    SRB_EXT(Srb)->SrbExtensionFlags = 0;
    SRB_EXT(Srb)->SavedDataTransferred = 0;
    SRB_EXT(Srb)->TotalDataTransferred = 0;

    //
    // Create the identify command.
    //
    DeviceExtension->MessageCount = 1;
    DeviceExtension->MessageSent = 0;
    DeviceExtension->MessageBuffer[0] = SCSIMESS_IDENTIFY | Srb->Lun;

    //
    // Check to see if disconnect is allowed.  If not then don't do tagged
    // queuing either.
    //
    if (!(Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)) {

        //
        // Enable disconnects in the message.
        //
        DeviceExtension->MessageBuffer[0] |= SCSIMESS_IDENTIFY_WITH_DISCON;

        //
        // If this is a tagged command then create a tagged message.
        //
        if (Srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE) {

            AMDPrint((0,"AMDStartExecution: Tagged Queuing enabled.\n"));

            //
            // The queue tag message is two bytes the first is the queue action
            // and the second is the queue tag.
            //
            DeviceExtension->MessageBuffer[1] = Srb->QueueAction;
            DeviceExtension->MessageBuffer[2] = Srb->QueueTag;
            DeviceExtension->MessageCount += 2;
            DeviceExtension->AdapterFlags |= PD_TAGGED_SELECT;

        } else {

            LuExtension->ActiveLuRequest = Srb;

        }

    } else {

            LuExtension->ActiveLuRequest = Srb;

    }

    //
    // If chip revision number is less or equal to 1, we are dealing
    // with A3 silicon which does not supporet sync. mode and Glitch 
    // Eater feature
    //
    if (DeviceExtension->PciConfigInfo.ChipRevision <= 1 ||
        !DeviceExtension->SysFlags.EnableSync) {

      //
      // Disable SCSI synchonous transfer
      //
      Srb->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    }

    //
    // Set target bit.
    //
    targetId = 1 << Srb->TargetId;

#ifndef DISABLE_SREG

    //
    // Read protection mode bit in PCI configuration register
    //
    PCIRegister = 
      (ULONG) &((PCONFIG_SPACE_HEADER) 0)->SCSIStateReg[DeviceExtension->AdapterBusId];
    AMDAccessPCIRegister (DeviceExtension, PCIRegister, OneWord, &regData, ReadPCI);
    
    //
    // Check if mode bits set correctly.  If not, CPU mode switched.
    // Tell upper layer mode changing event if post routine pointer valid
    //
    if ((DeviceExtension->SysFlags.CPUMode == PROTECTION_MODE &&
        (((PPCI_HOST_STATE)(&regData))->InitProtMode == 0 ||
        ((PPCI_HOST_STATE)(&regData))->InitRealMode == 1)) ||
		
        (DeviceExtension->SysFlags.CPUMode == REAL_MODE &&
        (((PPCI_HOST_STATE)(&regData))->InitProtMode == 1 ||
        ((PPCI_HOST_STATE)(&regData))->InitRealMode == 0))) {

#ifdef OTHER_OS

      //
      // Tell upper layer CPU mode switched
      //
      if (DeviceExtension->cpuModePost != NULL) {
  
        //
        // Tell uplayer CPU mode switched with reset information
        //
        (*DeviceExtension->cpuModePost) (DeviceExtension,
                         (BOOLEAN) ((PPCI_HOST_STATE)(&regData))->BusReset);

      }

#endif

      //
      // Clear bus reset bit anyway
      //
      ((PPCI_HOST_STATE)(&regData))->BusReset = 0;

      //
      // Reset CPU mode bits
      //
      if (DeviceExtension->SysFlags.CPUMode == PROTECTION_MODE) {

        ((PPCI_HOST_STATE)(&regData))->InitProtMode = 1;
        ((PPCI_HOST_STATE)(&regData))->InitRealMode = 0;

      } else {

        ((PPCI_HOST_STATE)(&regData))->InitProtMode = 0;
        ((PPCI_HOST_STATE)(&regData))->InitRealMode = 1;

      }

      //
      // Write date back
      //
      AMDAccessPCIRegister (DeviceExtension, PCIRegister, OneWord, &regData, WritePCI);

      //
      // Enviroment changed(ex. DOS <-> Windows)!  So initialize
      // target id flag
      //
      DeviceExtension->TargetInitFlags = 0;

      //
      // Clear SCSI FIFO
      //
      SCSI_WRITE(DeviceExtension->Adapter, Command, FLUSH_FIFO);
      
    }

    //
    // If interrupt is going to happen, return
    //
    if (DMA_READ (DeviceExtension->Adapter, StatusReg) & SCSICoreInterrupt) {

      return;

    }

#endif

    //
    // If LUN is not zero or if this is the first access the target, do not
    // negotiate async. or sync. mode with target.  Try to get information
    // from the PCI space register
    //
    if (Srb->Lun || !(DeviceExtension->TargetInitFlags & targetId)) {

      //
      // Set target initialization done flag anyway
      //
      DeviceExtension->TargetInitFlags |= targetId;

      //
      // If chip revision number is less or equal to 1, we are dealing
      // with A3 silicon which does not supporet sync. mode and Glitch 
      // Eater feature.  Otherwise, support all features.
      //
      if (DeviceExtension->PciConfigInfo.ChipRevision > 1) {

#ifndef DISABLE_SREG

        //
        // Get target information from PCI register
        //
        PCIRegister = 
          (ULONG) &((PCONFIG_SPACE_HEADER) 0)->SCSIStateReg[Srb->TargetId];
        AMDAccessPCIRegister (DeviceExtension, PCIRegister, OneWord, &regData, ReadPCI);

#else

        //
        // Get target information from device extension
        //
        regData = DeviceExtension->SCSIState[Srb->TargetId];

#endif

        //
        // Check if target exists.  If yes, try to get information.
        // Otherwise, go through.
        //
        if (((PPCI_TARGET_STATE)(&regData))->TargetExist) {

          //
          // Initialize logical unit extension
          //
          LuExtension->SynchronousPeriod = (UCHAR)((PPCI_TARGET_STATE)(&regData))->SyncPeriod;
          LuExtension->SynchronousOffset = (UCHAR)((PPCI_TARGET_STATE)(&regData))->SyncOffset;
          LuExtension->Configuration3.CheckIdMessage = 1;
          LuExtension->Configuration3.FastClock = DeviceExtension->ClockSpeed > 25 ? 1 : 0;
          LuExtension->Configuration3.FastScsi = (UCHAR)((PPCI_TARGET_STATE)(&regData))->FastScsi;

          //
          // Default Glitch Eater is 35 ns
          //
          LuExtension->Configuration4.KillGlitch = 1;
          if (LuExtension->Configuration3.FastScsi) {

            //
            // For fast SCSI, set Glitch Eater to 12 ns
            //
            LuExtension->Configuration4.KillGlitch = 0;

          } else {

            //
            // For normal sync. transfer, set Glitch Eater to 25 ns
            //
            if (LuExtension->SynchronousOffset != 0)
              LuExtension->Configuration4.KillGlitch = 2;

          }

          //
          // Set synchronous transfer flags
          //
          LuExtension->LuFlags |= PD_SYNCHRONOUS_NEGOTIATION
            + PD_SYNCHRONOUS_NEGOTIATION_DONE + PD_DO_NOT_NEGOTIATE
            + PD_SYNCHRONOUS_RENEGOTIATION;

          //
          // Select target and return
          //
          AMDSelectTarget( DeviceExtension, LuExtension );
          return;

        }

      } else {

        //
        // Async. mode only for A3 silicon
        //
        LuExtension->SynchronousOffset = ASYNCHRONOUS_OFFSET;
        LuExtension->SynchronousPeriod = ASYNCHRONOUS_PERIOD;
        LuExtension->Configuration3 = DeviceExtension->Configuration3;
        LuExtension->Configuration4 = DeviceExtension->Configuration4;

        //
        // No sync. negotiation necessary
        //
        LuExtension->LuFlags |=
          PD_SYNCHRONOUS_NEGOTIATION_DONE + PD_DO_NOT_NEGOTIATE;
        LuExtension->LuFlags &= ~PD_SYNCHRONOUS_NEGOTIATION;

      }
    }

    //
    // Check to see if asynchronous negotiation is necessary.
    //
    if (Srb->SrbFlags & SRB_FLAGS_DISABLE_SYNCH_TRANSFER &&
        LuExtension->SynchronousOffset) {

      //
      // Change sync. mode to async. mode
      //
      LuExtension->Configuration3 = DeviceExtension->Configuration3;
      LuExtension->Configuration4 = DeviceExtension->Configuration4;

      //
      // Set message
      //
      extendedMessage = (PSCSI_EXTENDED_MESSAGE)
        &DeviceExtension->MessageBuffer[DeviceExtension->MessageCount];
      DeviceExtension->MessageCount += 2 + SCSIMESS_SYNCH_DATA_LENGTH;
      extendedMessage->InitialMessageCode = SCSIMESS_EXTENDED_MESSAGE;
      extendedMessage->MessageLength = SCSIMESS_SYNCH_DATA_LENGTH;
      extendedMessage->MessageType = SCSIMESS_SYNCHRONOUS_DATA_REQ;

      //
      // Set default period to 62 ns when using 40 MHz clock
      //
      if (DeviceExtension->ClockSpeed == 40) {

        extendedMessage->ExtendedArguments.Synchronous.TransferPeriod =
          ((ASYNCHRONOUS_PERIOD + 1) * 1000 / DeviceExtension->ClockSpeed + 3) / 4;

      } else {

        extendedMessage->ExtendedArguments.Synchronous.TransferPeriod =
          (ASYNCHRONOUS_PERIOD * 1000 / DeviceExtension->ClockSpeed + 3) / 4;

      }

      extendedMessage->ExtendedArguments.Synchronous.ReqAckOffset =
        ASYNCHRONOUS_OFFSET;

      //
      // Clean sync. flags
      //
      LuExtension->LuFlags &=
        ~(PD_SYNCHRONOUS_NEGOTIATION_DONE + PD_DO_NOT_NEGOTIATE);

      //
      // Set sync. negotiation need flag and pending phase change flag
      //
      LuExtension->LuFlags |= PD_SYNCHRONOUS_NEGOTIATION;

      //
      // Attempt to select the target and update the adapter flags.
      //
      if (AMDSelectTarget(DeviceExtension, LuExtension) == FALSE) {
			
        return;

      }

      DeviceExtension->AdapterFlags |= PD_POSSIBLE_EXTRA_MESSAGE_OUT |
          PD_SYNCHRONOUS_TRANSFER_SENT;

      return;

    }

    //
    // If sync. negotiation needed and SRB sync. flag set, set condition
    // so sync. negotiation can be processing
    //
    if (LuExtension->LuFlags & PD_SYNCHRONOUS_NEGOTIATION &&
        !(Srb->SrbFlags & SRB_FLAGS_DISABLE_SYNCH_TRANSFER)) {

      //
      // Clear pending sync. negotiation flag
      // Clear negotiation done and do-not-negotiate flags
      //
      LuExtension->LuFlags &= ~(PD_SYNCHRONOUS_NEGOTIATION +
        PD_SYNCHRONOUS_NEGOTIATION_DONE + PD_DO_NOT_NEGOTIATE);

    }

    //
    // Check to see if synchronous negotiation is necessary.
    //
    if (!(LuExtension->LuFlags &
        (PD_SYNCHRONOUS_NEGOTIATION_DONE | PD_DO_NOT_NEGOTIATE))) {

        //
        // If synchronous renegotiation not pending
        //
        if (!(LuExtension->LuFlags & PD_SYNCHRONOUS_RENEGOTIATION)) {

          //
          // Initialize the synchronous transfer register values to an
          // asynchronous transfer, which is what will be used if anything
          // goes wrong with the negotiation.
          //
          LuExtension->SynchronousOffset = ASYNCHRONOUS_OFFSET;
          LuExtension->SynchronousPeriod = ASYNCHRONOUS_PERIOD;
          LuExtension->Configuration3 = DeviceExtension->Configuration3;
          LuExtension->Configuration4 = DeviceExtension->Configuration4;

          if (Srb->SrbFlags & SRB_FLAGS_DISABLE_SYNCH_TRANSFER) {

            //
            // Synchronous transfers are disabled by the SRB.
            //
            AMDSelectTarget( DeviceExtension, LuExtension );
            return;

          }
        }  

        if (DeviceExtension->ClockSpeed > 25) {

            //
            // The Fas216 supports fast synchronous transfers.
            //
            chipType = Fas216Fast;

        } else {

            chipType = DeviceExtension->SysFlags.ChipType;

        }

        //
        // Create the synchronous data transfer request message.
        // The format of the message is:
        //
        //        EXTENDED_MESSAGE op-code
        //        Length of message
        //        Synchronous transfer data request op-code
        //        Our Transfer period
        //        Our REQ/ACK offset
        //
        //  The message is placed after the IDENTIFY message.
        //
        extendedMessage = (PSCSI_EXTENDED_MESSAGE)
            &DeviceExtension->MessageBuffer[DeviceExtension->MessageCount];
        DeviceExtension->MessageCount += 2 + SCSIMESS_SYNCH_DATA_LENGTH;

        extendedMessage->InitialMessageCode = SCSIMESS_EXTENDED_MESSAGE;
        extendedMessage->MessageLength = SCSIMESS_SYNCH_DATA_LENGTH;
        extendedMessage->MessageType = SCSIMESS_SYNCHRONOUS_DATA_REQ;

        //
        // If this chips does not suport fast SCSI, just calculate the normal
        // minimum transfer period; otherwise use the fast value.
        //

        //
        // The initial sychronous transfer period is:
        //
        //  SynchronousPeriodCyles * 1000
        //  -----------------------------
        //    ClockSpeed * 4
        //
        // Note the result of the divide by four must be rounded up.
        //
        extendedMessage->ExtendedArguments.Synchronous.TransferPeriod =
            ((SynchronousTransferTypes[chipType].SynchronousPeriodCyles * 1000) /
            DeviceExtension->ClockSpeed + 3) / 4;
        extendedMessage->ExtendedArguments.Synchronous.ReqAckOffset = SYNCHRONOUS_OFFSET;

        //
        // Attempt to select the target and update the adapter flags.
        //
        if (AMDSelectTarget( DeviceExtension, LuExtension ) == FALSE) {
			
          return;

        }

        //
        // Many controllers reject the first byte of a synchronous
        // negotiation message.  Since this is a multibyte message the
        // ATN signal remains set after the first byte is sent.  Some
        // controllers remember this attempt to do a message-out
        // later.  Setting the PD_POSSIBLE_EXTRA_MESSAGE_OUT flag allows
        // this extra message transfer to occur without error.
        //
        DeviceExtension->AdapterFlags |= PD_POSSIBLE_EXTRA_MESSAGE_OUT |
            PD_SYNCHRONOUS_TRANSFER_SENT;

        return;
    }

    AMDSelectTarget( DeviceExtension, LuExtension );

}

LOCAL
BOOLEAN
AMDStartIo(
    IN PVOID ServiceContext,
    IN PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    This function is used by the OS dependent port driver to pass requests to
    the dependent driver.  This function begins the execution of the request.
    Requests to reset the SCSI bus are handled immediately.  Requests to send
    a message or start a SCSI command are handled when the bus is free.

Arguments:

    ServiceContext - Supplies the device Extension for the SCSI bus adapter.

    Srb - Supplies the SCSI request block to be started.

Return Value:

    TRUE - If the request can be accepted at this time.

    FALSE - If the request must be submitted later.

--*/

{
    PSPECIFIC_DEVICE_EXTENSION DeviceExtension = ServiceContext;
    PSPECIFIC_LOGICAL_UNIT_EXTENSION luExtension;

#ifdef OTHER_OS

    USHORT i;
    USHORT preferedDMAMode;
    UCHAR dataByte;

#endif

    //
    // Added in for debug 
    //
    AMDPrint((0,"\nAMDStartIo: Starting a new command.\n"));
    SetBreakPoint

    //
    // Check target ID.  If it is same as host adapter, return selection 
    // timeout status
    //
#ifdef OTHER_OS

    if (PciMechanism)

#endif

    if (Srb->TargetId == DeviceExtension->AdapterBusId) {

      Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;

      //
      // Ask for next request
      //
      ScsiPortNotification( RequestComplete, DeviceExtension, Srb );
      ScsiPortNotification( NextRequest, DeviceExtension, NULL );

      return TRUE;

    }

    switch (Srb->Function) {

    case SRB_FUNCTION_EXECUTE_SCSI:

        //
        // Determine the logical unit that this request is for.
        //
        luExtension = ScsiPortGetLogicalUnit(
            DeviceExtension,
            Srb->PathId,
            Srb->TargetId,
            Srb->Lun
            );

        AMDStartExecution(
            Srb,
            DeviceExtension,
            luExtension
            );

        break;

    case SRB_FUNCTION_ABORT_COMMAND:
    case SRB_FUNCTION_RESET_DEVICE:
    case SRB_FUNCTION_TERMINATE_IO:

        //
        // Determine the logical unit that this request is for.
        //
        luExtension = ScsiPortGetLogicalUnit(
            DeviceExtension,
            Srb->PathId,
            Srb->TargetId,
            Srb->Lun
            );

        AMDSendMessage(
            Srb,
            DeviceExtension,
            luExtension
            );

        break;

    case SRB_FUNCTION_RESET_BUS:

        //
        // There is no logical unit so just reset the bus.
        //
        AMDResetScsiBusInternal( DeviceExtension, 0 );
        break;

#ifdef OTHER_OS

    //
    // This is the AMD specified IO control function.  All the subfunctions
    // should be called only after the driver initialization has been
    // finished (FindAdapter and InitializaAdapter routines have been
    // called) except the subfunction for PCI mechanism option, which
    // has to be called before FindAdapter routine has been called.
    //
    case SRB_FUNCTION_IO_CONTROL:

        //
        // Check if AMD I/O control function initialization
        //
        for (i = 0; i < 8; i++) {

          if (((PAMD_IO_CONTROL)(Srb->DataBuffer))->IOControlBlock.Signature[i]
              != AMD_IO_CONTROL_ID[i])
            break;

        }

        //
        // If found matched AMD I/O ID
        //
        if (i == 8) {
            
          //
          // Set default return status
          //
          ((PAMD_IO_CONTROL)(Srb->DataBuffer))->IOControlBlock.ReturnCode =
            IO_STATUS_SUCCESS;
          Srb->SrbStatus = SRB_STATUS_SUCCESS;

          switch (((PAMD_IO_CONTROL)(Srb->DataBuffer))->IOControlBlock.ControlCode) {

            case POWER_MANAGE_FUNCTION:      // Power managament IO function

                   //
                   // Save power managament call back routine pointer
                   //
                   DeviceExtension->powerDownPost = ((PAMD_IO_CONTROL)
                     (Srb->DataBuffer))->AMD_SPECIFIC_FIELD.IO_PwrMgmtInterrupt;

                   break;

            case CPU_WORK_MODE_FUNCTION:     // CPU mode function

                   //
                   // Save cpu mode call back routine pointer
                   //
                   DeviceExtension->cpuModePost = ((PAMD_IO_CONTROL)
                     (Srb->DataBuffer))->AMD_SPECIFIC_FIELD.CPU_ModeChange;

                   break;

            case ACTIVE_NEGATION_FUNCTION:   // Active negation IO function

                   //
                   // Save active negation information
                   //
                   DeviceExtension->Configuration4.ActiveNegation =
                     ((PAMD_IO_CONTROL)(Srb->DataBuffer))->
                     AMD_SPECIFIC_FIELD.EnableActiveNegation == TRUE ? 2 : 0;

                   break;

            case DMA_SUPPORT_FUNCTION:       // DMA mode function.

                   //
                   // Get prefered DMA mode
                   //
                   preferedDMAMode = ((PAMD_IO_CONTROL)
                     (Srb->DataBuffer))->AMD_SPECIFIC_FIELD.SgMode.PreferredDMA;

                   //
                   // Save prefered mode
                   //
                   DeviceExtension->SysFlags.DMAMode = preferedDMAMode;
                   
                   //
                   // Set default as ok
                   //
                   ((PAMD_IO_CONTROL)(Srb->DataBuffer))->
                     AMD_SPECIFIC_FIELD.SgMode.SupportedDMA = preferedDMAMode;

                   //
                   // Set default good return status
                   //
                   ((PAMD_IO_CONTROL)(Srb->DataBuffer))->
                     AMD_SPECIFIC_FIELD.SgMode.Status = IO_PASS;

                   //
                   // Check silicon revision.  For T1/G1, MDL not supported
                   //
                   if ((DeviceExtension->PciConfigInfo.ChipRevision < 0x10 &&
                       preferedDMAMode == IO_DMA_METHOD_MDL)

#if defined DISABLE_MDL || defined T1

                       || preferedDMAMode == IO_DMA_METHOD_MDL

#endif


                       ) {

                       //
                       // Set supported DMA as SG
                       //
                       ((PAMD_IO_CONTROL)(Srb->DataBuffer))->
                         AMD_SPECIFIC_FIELD.SgMode.SupportedDMA = IO_DMA_METHOD_S_G_LIST;

                       //
                       // Set fail return status
                       //
                       ((PAMD_IO_CONTROL)(Srb->DataBuffer))->
                         AMD_SPECIFIC_FIELD.SgMode.Status = IO_FAIL;

                   } 

                   break;

#ifndef DISABLE_DIO

            case PCI_MECHANISM_FUNCTION:     // PCI mechanism function.

                   //
                   // Get PCI mechanism value
                   //
                   PciMechanism = ((PAMD_IO_CONTROL)
                     (Srb->DataBuffer))->AMD_SPECIFIC_FIELD.PciMechanism;

                   //
                   // Check if value is valid.
                   //
                   if (PciMechanism < 1 || PciMechanism > 2) {

                     //
                     // The input information is invalid.  Set  
                     // PCI mechanism to unknown
                     //
                     PciMechanism = 0;

                     //
                     // Set error return status
                     //
                     ((PAMD_IO_CONTROL)(Srb->DataBuffer))->IOControlBlock.ReturnCode =
                       IO_STATUS_ERROR;
                     Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;

                   } else {

                     UserSetPciMechanism = TRUE;

                   }

                   break;

#endif

            case PARITY_OPTION_FUNCTION:     // Parity option function.

                   //
                   // Read control register 1
                   //
                   dataByte = SCSI_READ(DeviceExtension->Adapter, Configuration1);

                   //
                   // Set parity option
                   //
                   ((PSCSI_CONFIGURATION1)(&dataByte))->ParityEnable = 
                     ((PAMD_IO_CONTROL)(Srb->DataBuffer))->AMD_SPECIFIC_FIELD.EnableParity;

                   //
                   // Write data back to control register 1
                   //
                   SCSI_WRITE(DeviceExtension->Adapter, Configuration1, dataByte);

                   break;

            case REAL_MODE_FLAG_FUNCTION:    // Set real mode flag

                   //
                   // This function only called by real mode driver
                   //
                   DeviceExtension->SysFlags.CPUMode = REAL_MODE;

                   break;

            default:

                   //
                   // Return bad status
                   //
                   ((PAMD_IO_CONTROL)(Srb->DataBuffer))->IOControlBlock.ReturnCode =
                     IO_STATUS_ERROR;
                   Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;

          }

        } else {

          //
          // Return bad status
          //
          ((PAMD_IO_CONTROL)(Srb->DataBuffer))->IOControlBlock.ReturnCode =
            IO_STATUS_ERROR;
          Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;

        }

        //
        // Ask for next request
        //
        ScsiPortNotification(RequestComplete, DeviceExtension, Srb);
        ScsiPortNotification(NextRequest, DeviceExtension, NULL);

        break;

#endif

    default:

        //
        // Unknown function code in the request.  Complete the request with
        // an error and ask for the next request.
        //
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        ScsiPortNotification(
            RequestComplete,
            DeviceExtension,
            Srb
            );

        ScsiPortNotification(
            NextRequest,
            DeviceExtension,
            NULL
            );

        break;
    }

    return(TRUE);

}

LOCAL
VOID
AMDStartDataTransfer(
    IN OUT PVOID ServiceContext
    )
/*++

Routine Description:

    This routine sets up the DMA chip and scsi bus protocol chip to
    perform a data transfer.

Arguments:

    ServiceContext - Supplies a pointer to the specific device extension.

Return Value:

    None.

--*/

{

    PSPECIFIC_DEVICE_EXTENSION DeviceExtension = ServiceContext;
    ULONG dataLeft;

#ifdef OTHER_OS

    SCSI_PHYSICAL_ADDRESS physicalAddress;

#endif

    //
    // Added in for debug 
    //
    AMDPrint((0,"AMDStartDataTransfer: "));
    SetBreakPoint
    
    //
    // Check if for data overrun
    //
    if (DeviceExtension->AdapterFlags & PD_DATA_OVERRUN) {

      //
      // Set transfer count to 1
      //
      DeviceExtension->ActiveDMALength = 1;

      //
      // Use tempary data buffer to dump data
      //
      DeviceExtension->ActiveDataPointer = DeviceExtension->TempLinearBuf;
      DeviceExtension->ActiveDMAPointer = DeviceExtension->TempPhysicalBuf;

      //
      // Clear the tempary buffer
      //
      *(PULONG) DeviceExtension->TempLinearBuf = 0;

    } else {
      
#ifndef T1
#ifndef DISABLE_MDL

      //
      // If silicon support MDL or SG
      //
      if (DeviceExtension->PciConfigInfo.ChipRevision >= 0x10) {

#ifdef OTHER_OS

        //
        // Build MDL for DMA engine if SG list available.
        //
        if (DeviceExtension->SysFlags.DMAMode != IO_DMA_METHOD_LINEAR) {

          AMDSgMakeMDL (DeviceExtension);

        } else

#endif

        AMDPtrMakeMDL (DeviceExtension);

        //
        // If the request from PCI abort handler, start DMA and return.
        // Don't make SCSI controller confused.
        //
        if (DeviceExtension->ActiveLuRequest->SrbStatus == SRB_STATUS_PHASE_SEQUENCE_FAILURE) {
 
          AMDStartDMA (DeviceExtension);
          return;

        }

      } else {

#endif
#endif

#ifdef OTHER_OS

        //
        // Calculate physical address and transfer length
        //
        if (DeviceExtension->SysFlags.DMAMode != IO_DMA_METHOD_LINEAR) {

          //
          // One SG entry for each DMA transfer
          //
          DeviceExtension->ActiveDMAPointer = SGPointer + DeviceExtension->ActiveSGTEntryOffset;
          DeviceExtension->ActiveDMALength = SGCount - DeviceExtension->ActiveSGTEntryOffset;

        } else {

#endif

          DeviceExtension->ActiveDMAPointer = ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(DeviceExtension,
                                       DeviceExtension->ActiveLuRequest,
                                       (PUCHAR)DeviceExtension->ActiveDataPointer,
                                       &dataLeft));
          DeviceExtension->ActiveDMALength = dataLeft;

#ifdef OTHER_OS

        }

#endif

        //
        // Calculate number of bytes left to transfer
        //
        dataLeft = DeviceExtension->ActiveDataLength -
               SRB_EXT(DeviceExtension->ActiveLuRequest)->TotalDataTransferred;

        //
        // If the number of data left to transfer is less than the DMA transfer 
        // length, or the data left is less than or equal to 4, use the data left 
        // as DMA transfer length
        //
        if (dataLeft < DeviceExtension->ActiveDMALength) {

          DeviceExtension->ActiveDMALength = dataLeft;

        }

        //
        // Check for old chips(before T2).  Special handling needed for 
        // less than 4 bytes transfer for those chips
        //
        if (DeviceExtension->PciConfigInfo.ChipRevision < 0x10) {

          //
          // Patch one byte transfer in data in phase problem.
          //
          if (dataLeft == 1 && DeviceExtension->dataPhase == DATA_IN) {

            //
            // Transfer two bytes instead of one to avoid problem
            //
            DeviceExtension->ActiveDMALength = 2;

            //
            // Set one byte transfer flag
            //
            DeviceExtension->AdapterFlags |= PD_ONE_BYTE_XFER;

          }

#ifdef OTHER_OS

          //
          // Get linear address for scather/gather input only if necessary
          //
          if (DeviceExtension->ActiveDMALength <= 4 &&
              DeviceExtension->SysFlags.DMAMode != IO_DMA_METHOD_LINEAR) {
//                 
// Specific mods for Solaris 2.4 pcscsi driver 
// 
//      Fix warnings for 'improper member use'.
//
#ifdef SOLARIS   
            physicalAddress.u.LowPart = DeviceExtension->ActiveDMAPointer;
            physicalAddress.u.HighPart = 0;
#else   // SOLARIS
            physicalAddress.LowPart = DeviceExtension->ActiveDMAPointer;
            physicalAddress.HighPart = 0;
#endif  // SOLARIS

            DeviceExtension->ActiveDataPointer =
              (ULONG) ScsiPortGetVirtualAddress (
                        DeviceExtension,
	                     physicalAddress,
                        DeviceExtension->ActiveLuRequest,
                        DeviceExtension->ActiveLuRequest->DataTransferLength
                        );

          }

#endif

        }

#ifndef T1
#ifndef DISABLE_MDL

      }

#endif
#endif

    }

    //
    // Set the transfer count.
    //
    SCSI_WRITE( DeviceExtension->Adapter,
        TransferCountLow,
        (UCHAR) DeviceExtension->ActiveDMALength
        );

    SCSI_WRITE( DeviceExtension->Adapter,
        TransferCountHigh,
        (UCHAR) (DeviceExtension->ActiveDMALength >> 8)
        );

    //
    // Write bits 23-16.
    //
    SCSI_WRITE(DeviceExtension->Adapter,
        TransferCountPage,
        (UCHAR) (DeviceExtension->ActiveDMALength >> 16)
        );

    //
    // Start SCSI core data transfer
    //
    SCSI_WRITE( DeviceExtension->Adapter,
             Command,
             TRANSFER_INFORMATION_DMA
             );

    //
    // Start DMA engine data transfer.
    //
    AMDStartDMA (DeviceExtension);

    //
    // Added for debug 
    //
    AMDPrint((0,"ActiveDMALength = %x, ActiveDMAPointer = %x\n",
    DeviceExtension->ActiveDMALength,DeviceExtension->ActiveDMAPointer));

}

#ifndef T1
#ifndef DISABLE_MDL


LOCAL
VOID
AMDSgMakeMDL(
    IN OUT PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine gets input from scather/gather list and builds a MDL table
    inside locked memory block for DMA engine.	DMA transfer physical
    address and data length are also initialized.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.

Return Value:

    ActiveDMAPointer and ActiveDMALength in device extension structure
    contain transfer physical address and number of bytes can be
    transfered by using built MDL 

--*/

{
    ULONG dataLeft;
    ULONG physicalAddress;
    ULONG blockLength;
    PULONG sgPtr;
    ULONG i;

    //
    // Added for debug
    //
    AMDPrint((0,"AMDSgMakeMDL.\n"));
    SetBreakPoint

    //
    // Total data left to be transferred
    //
    if (DeviceExtension->ActiveLuRequest->SrbStatus != SRB_STATUS_PHASE_SEQUENCE_FAILURE) {
      dataLeft = DeviceExtension->ActiveDataLength -
        SRB_EXT(DeviceExtension->ActiveLuRequest)->TotalDataTransferred;
    } else {
      dataLeft = DeviceExtension->ActiveDMALength;
    }

    //
    // Added for debug
    //
    AMDPrint((0, "Build MDL: Bytes left %lx\n", dataLeft));

    //
    // Check if MDL already available
    //
    if (DeviceExtension->SysFlags.DMAMode == IO_DMA_METHOD_MDL) {

      //
      // MDL available.  Make start physical address
      //
      DeviceExtension->ActiveDMAPointer = (SGPointer & (~PAGE_MASK)) +
                                     DeviceExtension->ActiveSGTEntryOffset;
      
      //
      // Set transfer length
      //
      DeviceExtension->ActiveDMALength = dataLeft;

      //
      // Done!
      //
      return;

    }
      
    //
    // Initialize MDL
    //
    for (i = 0; i < MAXIMUM_MDL_DESCRIPTORS; i++)
      DeviceExtension->MdlPointer->MDLTable[i] = 0;

    //
    // Create MDL segment descriptors.
    //
    i = 0;

    //
    // Calculate physical address and data length in a starting SG entry
    //
    physicalAddress = SGPointer + DeviceExtension->ActiveSGTEntryOffset;
    sgPtr = DeviceExtension->ActiveSGEntry;
    blockLength = SGCount - DeviceExtension->ActiveSGTEntryOffset;

    //
    // Set DMA start transfer address
    //
    DeviceExtension->ActiveDMAPointer = physicalAddress;

    while (dataLeft) {

      //
      // Added for debug
      //
      AMDPrint((0, "Build MDL: Physical address %lx\n", physicalAddress));
      AMDPrint((0, "Build MDL: Data length %lx\n", blockLength));

		//
		// Check block length
		//
      blockLength = blockLength > dataLeft ? dataLeft : blockLength;

      //
      // Build MDL
      //
      if (AMDBuildMDL (DeviceExtension, physicalAddress, blockLength, &i) == TRUE)
        return;

      //
      // Adjust counter.
      //
      dataLeft -= blockLength;

		//
		// If not finished
		//
      if (dataLeft) {

        //
        // Go to next scather/gather entry
        //
        sgPtr += 2;
        physicalAddress = *sgPtr;
        blockLength = *(sgPtr + 1);

      }
    }
}

LOCAL
VOID
AMDPtrMakeMDL(
    IN OUT PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine uses linear data buffer pointer to build a MDL table
    inside locked memory block for DMA engine.	DMA transfer physical
    address and data length are also initialized.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.

Return Value:

    ActiveDMAPointer and ActiveDMALength in device extension structure
    contain transfer physical address and number of bytes can be
    transfered by using built MDL 

--*/

{
    ULONG dataLeft;
    PUCHAR linearAddress;
    ULONG physicalAddress;
    ULONG blockLength;
    ULONG i;

    //
    // Added for debug
    //
    AMDPrint((0,"AMDPtrMakeMDL.\n"));
    SetBreakPoint

    //
    // Initialize MDL
    //
    for (i = 0; i < MAXIMUM_MDL_DESCRIPTORS; i++)
      DeviceExtension->MdlPointer->MDLTable[i] = 0;

    //
    // Initialize variables
    //
    i = 0;
    if (DeviceExtension->ActiveLuRequest->SrbStatus != SRB_STATUS_PHASE_SEQUENCE_FAILURE) {
      dataLeft = DeviceExtension->ActiveDataLength -
        SRB_EXT(DeviceExtension->ActiveLuRequest)->TotalDataTransferred;
    } else {
      dataLeft = DeviceExtension->ActiveDMALength;
    }
    linearAddress = (PUCHAR) DeviceExtension->ActiveDataPointer;

    //
    // Create MDL
    //
    while (dataLeft) {

      //
      // Get physical address and length of contiguous physical buffer.
      //
      physicalAddress = ScsiPortConvertPhysicalAddressToUlong(
                          ScsiPortGetPhysicalAddress(DeviceExtension,
                                            DeviceExtension->ActiveLuRequest,
                                            linearAddress,
                                            &blockLength));

      //
      // Added for debug
      //
      AMDPrint((0, "Build MDL Table: Physical address %lx\n", physicalAddress));
      AMDPrint((0, "Build MDL Table: Data length %lx\n", blockLength));
      AMDPrint((0, "Build MDL Table: MDL index %lx\n", i));

      //
      // For first memory block
      //
      if (!i) {

        //
        // Set DMA start transfer address
        //
        DeviceExtension->ActiveDMAPointer = physicalAddress;

      }

      //
      // If length of physical memory is bigger than that of bytes left in 
      // transfer, use bytes left as final length.
		//
      blockLength = blockLength > dataLeft ? dataLeft : blockLength;

      //
      // Build MDL
      //
      if (AMDBuildMDL (DeviceExtension, physicalAddress, blockLength, &i) == TRUE)
        return;

      //
      // Adjust counts.
      //
      linearAddress += blockLength;
      dataLeft -= blockLength;

    }
}

LOCAL
BOOLEAN
AMDBuildMDL(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN ULONG physicalAddress,
    IN ULONG dataLength,
    IN PULONG MDLIndexPtr
    )

/*++

Routine Description:

    This routine builds MDL inside uncachable locked memory.

    For the case given scattered physical memory blocks are not page aligned 
    (except for first physical memory block), or each page is not 4K length
    (except for first physical memory block and last physical memory block),
    or allocated memory for MDL is full, finish building current MDL.  We
    have to build other MDLs later for other DMA operations.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.

    physicalAddress - Data buffer physical address

    dataLength - Data buffer length

    MDLIndexPtr - MDL index pointer

Return Value:

    TRUE - End of MDL building.  Do not call this routine again except for
           another DMA transfer.
    FALSE - Continue to make MDL allowed.

    ActiveDMALength in device extension structure number of bytes can be
    transfered by using built MDL 

--*/

{

    ULONG startPage,endPage,page;

    //
    // Create MDL segment descriptors.
    //
    if (dataLength) {

      //
      // Check if given memory blocks followed to the first block is 
      // page aligned.  If not, go to start transfer data.  We have to 
      // build another MDL later for remained data.
      // 
      if (*MDLIndexPtr && (physicalAddress & PAGE_MASK))
        return TRUE;

      //
      // Calculate pages
      //
      startPage = physicalAddress >> 12;           
		endPage = (physicalAddress + dataLength) >> 12;

      //
      // Fill MDL with page frame addresses
      //
		for (page = startPage; page <= endPage; page++) {

        //
        // If MDL is full, return
        //
        if (*MDLIndexPtr == MAXIMUM_MDL_DESCRIPTORS)
          return TRUE;

        //
        // Update transfer length
        //
        if (!(*MDLIndexPtr)) {

          //
          // If it is first page, add data length below first page boundary
          //
          DeviceExtension->ActiveDMALength = physicalAddress & PAGE_MASK ?
            PAGE_SIZE - (physicalAddress & PAGE_MASK) : PAGE_SIZE;

        } else {

          DeviceExtension->ActiveDMALength += PAGE_SIZE;

        }

        //
        // Save page to MDL
        //
        DeviceExtension->MdlPointer->MDLTable[(*MDLIndexPtr)++] = page << 12;

      }

      //
      // Adjust transfer length for the data length above last page boundary.
      // Check if transfer ends at page boundary.  If not, stop build MDL
      //
      if ((physicalAddress + dataLength) & PAGE_MASK) {

        //
        // The end address is not page aligned
        //
        DeviceExtension->ActiveDMALength -= PAGE_SIZE -
          ((physicalAddress + dataLength) & PAGE_MASK);

        return TRUE;

      } else {

        //
        // The end address is page aligned
        //
        DeviceExtension->ActiveDMALength -= PAGE_SIZE;

        //
        // Remove last entry and adjust MDL table index
        //
        DeviceExtension->MdlPointer->MDLTable[--(*MDLIndexPtr)] = 0;

      }
    }

    //
    // Return FALSE to continue to build MDL
    //
    return FALSE;

}

#endif
#endif

#ifdef OTHER_OS


LOCAL
VOID
AMDUpdateSGTPointer(
    IN OUT PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN ULONG Offset
    )

/*++

Routine Description:

    This routine updates the SG table pointer

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.
    Offset - Number of byte needs to be offset

Return Value:

    ActiveSGEntry and ActiveSGTEntryOffset in DeviceExtension updated.

--*/

{
    ULONG anEntryDataLeft;

    //
    // If offset is valid
    //
    if (Offset) {

#ifndef DISABLE_MDL

      //
      // Number of data left for a current scather/gather entry
      //
      if (DeviceExtension->SysFlags.DMAMode == IO_DMA_METHOD_MDL) {

        anEntryDataLeft = PAGE_SIZE - DeviceExtension->ActiveSGTEntryOffset;

      } else

#endif

      anEntryDataLeft = SGCount - DeviceExtension->ActiveSGTEntryOffset;

      //
      // Update scather/gather entry and entry offset
      //
      if (anEntryDataLeft <= Offset) {

        Offset -= anEntryDataLeft;

#ifndef DISABLE_MDL

        if (DeviceExtension->SysFlags.DMAMode == IO_DMA_METHOD_MDL) {

          //
          // Update active linear and physical MDL pointer
          //
          DeviceExtension->ActiveSGEntry++;

#ifndef T1

          DeviceExtension->MdlPhysicalPointer = 
            (PMDL)((PULONG)DeviceExtension->MdlPhysicalPointer + 1);

#endif

          while (Offset && PAGE_SIZE <= Offset) {
            Offset -= PAGE_SIZE;
            DeviceExtension->ActiveSGEntry++;

#ifndef T1

            DeviceExtension->MdlPhysicalPointer =
              (PMDL)((PULONG)DeviceExtension->MdlPhysicalPointer + 1);

#endif

          } 

        } else {

#endif
          DeviceExtension->ActiveSGEntry += 2;

          while (Offset && SGCount <= Offset) {
            Offset -= SGCount;
            DeviceExtension->ActiveSGEntry += 2;

          }

#ifndef DISABLE_MDL

        }

#endif

        DeviceExtension->ActiveSGTEntryOffset = Offset;

      } else {

        DeviceExtension->ActiveSGTEntryOffset += Offset;

      }
    }
}

#endif


ULONG
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    )

/*++

Routine Description:

Arguments:

    DriverObject - Driver Object is passed to ScsiPortInitialize()

    Argument2 - Unused by NT.

Return Value:

    Status from ScsiPortInitialize()

--*/

{
    HW_INITIALIZATION_DATA hwInitializationData;
    INIT_DATA initData;
    ULONG i;

#ifdef NT_31

    ULONG Status1, Status2;

# else

    //
    // For NT 3.5 and Windows 95, set AMD PCI SCSI chip
    // vendor id and device id
    //
    UCHAR VendorId[4] = {'1','0','2','2'};
    UCHAR DeviceId[4] = {'2','0','2','0'};

#endif

    AMDPrint((0,"\n\nAMD Golden Gate SCSI NT MiniPort Driver\n"));

    //
    // Zero out hardware initialization data structure.
    //
    for (i = 0; i < sizeof(HW_INITIALIZATION_DATA); i++) {
       ((PUCHAR)&hwInitializationData)[i] = 0;
    }

    //
    // Fill in the hardware initialization data structure.
    // Reserve memory for data structures
    //
    hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);
    hwInitializationData.DeviceExtensionSize = sizeof(SPECIFIC_DEVICE_EXTENSION);
    hwInitializationData.SpecificLuExtensionSize = sizeof(SPECIFIC_LOGICAL_UNIT_EXTENSION);
    hwInitializationData.SrbExtensionSize = sizeof(SRB_EXTENSION);

    //
    // Functions interface to system.
    //
    hwInitializationData.HwInitialize = AMDInitializeAdapter;
    hwInitializationData.HwStartIo = AMDStartIo;
    hwInitializationData.HwInterrupt = AMDInterruptServiceRoutine;
    hwInitializationData.HwResetBus = AMDResetScsiBus;
    hwInitializationData.HwFindAdapter = AMDFindAdapter;
    hwInitializationData.HwAdapterState = AMDCPUModeSwitchHandler;

    //
    // Initialize other fields
    //
    hwInitializationData.NumberOfAccessRanges = 1;
    hwInitializationData.NeedPhysicalAddresses = TRUE;
    hwInitializationData.MapBuffers = TRUE;

    //
    // Initialize PCI bus scan parameters
    //
    initData.busNumber = 0;
    initData.deviceNumber = 0;
    initData.functionNumber = 0;

#ifndef NT_31

    //
    // AMD PCI SCSI device ids
    //
    hwInitializationData.AdapterInterfaceType = PCIBus;
    hwInitializationData.VendorIdLength = 4;
    hwInitializationData.VendorId = VendorId;
    hwInitializationData.DeviceIdLength = 4;
    hwInitializationData.DeviceId = DeviceId;

    //
    // Start driver initialization
    //
    return ScsiPortInitialize(DriverObject, Argument2, &hwInitializationData, &initData);
    
#else

    //
    // Try Micro Channel / PCI
    //
    hwInitializationData.AdapterInterfaceType = MicroChannel;
    Status1 = ScsiPortInitialize(DriverObject, Argument2, &hwInitializationData, &initData);

    //
    // Try Mips system / PCI
    //
    hwInitializationData.AdapterInterfaceType = Internal;
    Status2 = ScsiPortInitialize(DriverObject, Argument2, &hwInitializationData, &initData);
    Status1 = Status2 < Status1 ? Status2 : Status1;

    //
    // Try ISA / PCI
    //
    hwInitializationData.AdapterInterfaceType = Isa;
    Status2 = ScsiPortInitialize(DriverObject, Argument2, &hwInitializationData, &initData);

    //
    // Return status.
    //
    return(Status2 < Status1 ? Status2 : Status1);

#endif

} // end PortInitialize()

LOCAL
ULONG
AMDFindAdapter(
    IN PVOID ServiceContext,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
/*++

Routine Description:

    This routine searches PCI device and returns with configuration
    information.

Arguments:

    ServiceContext - Supplies a pointer to the device extension.

    Context - Bus type pointer passed by DriverEntry.

    BusInformation - Unused.

    ArgumentString - Unused.

    ConfigInfo - Pointer to the configuration information structure to be
                 filled in.

    Again - Returns back a request to call this function again.

Return Value:

    Returns a status value for the initialazitaition.

--*/

{
    PSPECIFIC_DEVICE_EXTENSION DeviceExtension = 
			(PSPECIFIC_DEVICE_EXTENSION)ServiceContext;
    PINIT_DATA pInitData = Context;
    UCHAR dataByte;
    ULONG dataWord;
    ULONG status;
    ULONG length;

#ifndef NT_31

    //
    // If device information passed down in AccessRange field,
    // save initialization information in device extension
    //
    if ((*ConfigInfo->AccessRanges)[0].RangeLength) {

      //
      // Save PCI device information
      //
      DeviceExtension->PciConfigInfo.BusNumber = 
         (UCHAR)ConfigInfo->SystemIoBusNumber;
      DeviceExtension->PciConfigInfo.DeviceNumber = 
         (UCHAR)((PPCI_SLOT_NUMBER)&(ConfigInfo->SlotNumber))->u.bits.DeviceNumber;
      DeviceExtension->PciConfigInfo.FunctionNumber = 
         (UCHAR)((PPCI_SLOT_NUMBER)&(ConfigInfo->SlotNumber))->u.bits.FunctionNumber;
      DeviceExtension->PciConfigInfo.PciFuncSupport = TRUE;

    } else {

#endif	// NT_31

#ifndef DISABLE_DIO

      //
      // Set PCI device start address for bus scan
      //
      DeviceExtension->PciConfigInfo.BusNumber = pInitData->busNumber;
      DeviceExtension->PciConfigInfo.DeviceNumber = pInitData->deviceNumber;
      DeviceExtension->PciConfigInfo.FunctionNumber = pInitData->functionNumber;
      DeviceExtension->PciConfigInfo.PciFuncSupport = FALSE;

#endif	// DISABLE_DIO

#ifndef NT_31

    }

#endif	// NT_31

    //

    // Set the SCSI bus Id.  If initiator ID is initialized and valid, 

    // use it.  Else set initiator ID to default 7

    //

    if ((UCHAR)ConfigInfo->InitiatorBusId[0] == 0xff ||

        (UCHAR)ConfigInfo->InitiatorBusId[0] > 7) {



      DeviceExtension->AdapterBusId = INITIATOR_BUS_ID;

      ConfigInfo->InitiatorBusId[0] = INITIATOR_BUS_ID;

      DeviceExtension->AdapterBusIdMask = 1 << INITIATOR_BUS_ID;



    } else {



      DeviceExtension->AdapterBusId = ConfigInfo->InitiatorBusId[0];

      DeviceExtension->AdapterBusIdMask = 1 << ConfigInfo->InitiatorBusId[0];



    }



    //
    // Search for AMD PCI SCSI adpater.  If find enabled one, base I/O 
    // address, interrupt number, PCI method used to access configuration
    // space,  chip revision and PCI bus information will be returned 
    // inside DeviceExtension data structure.
    //
    status = AMDFindPciAdapter (DeviceExtension, ConfigInfo);

    //
    // Check if AMD SCSI controller presents
    //
    *Again = FALSE;
    if (status != SP_RETURN_FOUND) {

      if (DeviceExtension->PciConfigInfo.BaseAddress == DEVICE_DISABLED) {

        //
        // Search for next controller
        //
        goto GetNextController;

      } else {

        AMDPrint((0,"AMD SCSI device is not found.\n"));

        //
        // Searching controller job done
        //
        return status;

      }
    }

    //
    // Set flags by default value
    //
    DeviceExtension->SysFlags.EnableSync = 1;
    DeviceExtension->SysFlags.EnableTQ = 1;
    DeviceExtension->SysFlags.EnablePrity = 1;
    DeviceExtension->SysFlags.CPUMode = PROTECTION_MODE;
    DeviceExtension->SysFlags.DMAMode = IO_DMA_METHOD_LINEAR;

#ifndef OTHER_OS
    //
    // Check driver flags passed down by OS.  For multiple adapter,
    // the flags have to be passed to every device extension.
    //
    if (ArgumentString != NULL) {

      //
      // Check string for flags and save them if any
      //
      AMDParseFlagString (DeviceExtension, ArgumentString);

    }
#endif	// #ifndef OTHER_OS


    //
    // System information
    //
    AMDPrint((0,"AMD PCI SCSI device is found.\n\n"));
    AMDPrint((0,"PCI bus information:\n"));
    AMDPrint((0,"   PCI mechanism = %d\n",DeviceExtension->PciConfigInfo.Method));
    AMDPrint((0,"   PCI bus number = %d\n",DeviceExtension->PciConfigInfo.BusNumber));
    AMDPrint((0,"   PCI device number = %d\n",DeviceExtension->PciConfigInfo.DeviceNumber));
    AMDPrint((0,"   PCI function number = %d\n",DeviceExtension->PciConfigInfo.FunctionNumber));
    AMDPrint((0,"SCSI device information:\n"));
    AMDPrint((0,"   Chip revision number = %x (Hex)\n",DeviceExtension->PciConfigInfo.ChipRevision));
    AMDPrint((0,"   HBA id number = %d\n",DeviceExtension->AdapterBusId));
    AMDPrint((0,"   IRQ number = %d\n",DeviceExtension->PciConfigInfo.IRQ));
    AMDPrint((0,"   I/O mapped address = %x (Hex)\n",DeviceExtension->PciConfigInfo.BaseAddress));
    AMDPrint((0,"   I/O address length = %x (Hex)\n\n",sizeof(REG_GROUP)));

    //
    // Setup interrupt parameters
    //
    ConfigInfo->BusInterruptLevel = DeviceExtension->PciConfigInfo.IRQ;
    ConfigInfo->InterruptMode = LevelSensitive;

    //
    // One bus only
    //
    ConfigInfo->NumberOfBuses = 1;

    //
    // Map the SCSI protocol chip into the virtual address space.
    //
    DeviceExtension->Adapter = ScsiPortGetDeviceBase(
        DeviceExtension,                      // HwDeviceExtension           
        ConfigInfo->AdapterInterfaceType,     // AdapterInterfaceType 
        ConfigInfo->SystemIoBusNumber,        // SystemIoBusNumber
        ScsiPortConvertUlongToPhysicalAddress
                (DeviceExtension->PciConfigInfo.BaseAddress),    
        sizeof(REG_GROUP),                    // NumberOfBytes
        TRUE                                  // InIoSpace
        );

    if (DeviceExtension->Adapter == NULL) {
      AMDPrint((0, "\nScsiPortInitialize:\
        Failed to map SCSI device registers into system space.\n"));
      return(SP_RETURN_ERROR);
    }

    //
    // Initialize the AMD SCSI Chip.
    //
    SCSI_WRITE( DeviceExtension->Adapter, Command, RESET_SCSI_CHIP );

    //
    // A NOP command is required to clear the chip reset command.
    //
    SCSI_WRITE( DeviceExtension->Adapter, Command, NO_OPERATION_DMA );

    //
    // Set control register 2
    //
    dataByte = 0;
    ((PSCSI_CONFIGURATION2)(&dataByte))->EnablePhaseLatch = 1;
    SCSI_WRITE( DeviceExtension->Adapter, Configuration2, dataByte);

    //
    // Read content and get chip ID.
    //
    dataByte = SCSI_READ(DeviceExtension->Adapter, TransferCountPage);

    //
    // Set data transfer length clock speed
    //
    DeviceExtension->SysFlags.ChipType = Fas216;

    //
    // Set clock frequency and enable PCI master/target abort
    // interrupt if this is T2/G2
    //
    if (DeviceExtension->PciConfigInfo.ChipRevision >= 0x10) {

      dataWord = DMA_READ (DeviceExtension->Adapter, MiscReg);
      DeviceExtension->ClockSpeed = ((PDMA_MISC_DATA)&dataWord)->
                       SCSIClockPresent ? SCSIClockFreq : PCIClockFreq;
      dataWord = 0;

#ifndef T1

      ((PDMA_MISC_DATA)&dataWord)->PCIAbortIntEnable = 1;

#endif	// T1

      DMA_WRITE (DeviceExtension->Adapter, MiscReg, dataWord);

    } else {

      DeviceExtension->ClockSpeed = SCSIClockFreq;

    }

    //
    // Set maximum data transfer length
    //
    ConfigInfo->MaximumTransferLength = 0x1000000-0x1000;

    //
    // Support scatter/gather feature
    //
    ConfigInfo->NumberOfPhysicalBreaks = MAXIMUM_MDL_DESCRIPTORS - 1;
    ConfigInfo->ScatterGather = TRUE;

    //
    // Bus master device
    //
    ConfigInfo->Master = TRUE;

    //
    // Master DMA is using 32-bit address
    //
    ConfigInfo->Dma32BitAddresses = TRUE;

    //
    // Tagged queuing support
    //
    ConfigInfo->TaggedQueuing = (BOOLEAN)DeviceExtension->SysFlags.EnableTQ;

    //
    // No auto request sense support
    //
    ConfigInfo->AutoRequestSense = FALSE;

    //
    // Set configuration register 3
    //
    *((PUCHAR) &DeviceExtension->Configuration3) = 0;
    DeviceExtension->Configuration3.CheckIdMessage = 1;

    //
    // If the clock speed is greater than 25 Mhz then set the fast clock
    // bit in configuration register.
    //
    if (DeviceExtension->ClockSpeed > 25) {
      DeviceExtension->Configuration3.FastClock = 1;
    }

    //
    // Set configuration register 4. 
    //
    *((PUCHAR) &DeviceExtension->Configuration4) = 0;

    //
    // If chip revision number is less or equal to 1, we are dealing
    // with A3 silicon which does not supporet sync. mode and Glitch 
    // Eater feature
    //
    if (DeviceExtension->PciConfigInfo.ChipRevision > 1) {
      DeviceExtension->Configuration4.KillGlitch = 1;
    }

    //
    // Reserve I/O space
    //
    (*ConfigInfo->AccessRanges)[0].RangeStart =
        ScsiPortConvertUlongToPhysicalAddress
           (DeviceExtension->PciConfigInfo.BaseAddress);
    (*ConfigInfo->AccessRanges)[0].RangeLength = sizeof(REG_GROUP);
    (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

    //
    // Reserve memory space
    //
    // Allocate double word boundary buffer for less than or equal to 4 
    // bytes data transfer for T1/G1; and for DMA BLAST data
    //
#ifndef T1
#ifndef DISABLE_MDL

    if (DeviceExtension->PciConfigInfo.ChipRevision < 0x10) {

#endif	// DISABLE_MDL
#endif	// T1

      //
      // Reserve memory for temp. data buffer
      //
      length = 68 + 3;

      //
      // Allocate a physically contiguous memory block for the data buffer
      //
      DeviceExtension->TempLinearBuf = (ULONG) ScsiPortGetUncachedExtension(
                                                DeviceExtension,
                                                ConfigInfo,
                                                length);

      if (DeviceExtension->TempLinearBuf == (ULONG) NULL) {
        AMDPrint((0, "\nAMDFindAdapter: Failed to allocate contiguous physical memory.\n"));
        return(SP_RETURN_ERROR);
      }

      //
      // Get physical buffer address.
      //
      DeviceExtension->TempPhysicalBuf = 
        (ULONG) ScsiPortConvertPhysicalAddressToUlong(
          ScsiPortGetPhysicalAddress(DeviceExtension,
                                     NULL,
                                     (PUCHAR) DeviceExtension->TempLinearBuf,
                                     &length));

      //
      // Make the buffer double word aligned
      //
      DeviceExtension->TempPhysicalBuf = 
        (DeviceExtension->TempPhysicalBuf + 3) & ~3;
      DeviceExtension->TempLinearBuf = 
        (DeviceExtension->TempLinearBuf + 3) & ~3;

#ifndef T1
#ifndef DISABLE_MDL

    } else {

      //
      // Allocate a physically contiguous memory block for MDL.
      //
      DeviceExtension->MdlPointer = ScsiPortGetUncachedExtension(
                                                DeviceExtension,
                                                ConfigInfo,
                                                sizeof(MDL) + 7);

      if (DeviceExtension->MdlPointer == NULL) {
        AMDPrint((0, "\nAMDFindAdapter: Failed to allocate contiguous\
            physical memory for MDL.\n"));
        return(SP_RETURN_ERROR);
      }

      //
      // Get physical MDL address.
      //
      DeviceExtension->MdlPhysicalPointer = 
        (PMDL) ScsiPortConvertPhysicalAddressToUlong(
          ScsiPortGetPhysicalAddress(DeviceExtension,
                                     NULL,
                                     DeviceExtension->MdlPointer,
                                     &length));

      //
      // Make MDL starting address dword aligned for T2 (required).  
      // Note that we reserved several more bytes place we needed when 
      // allocating memory
      //
      DeviceExtension->MdlPhysicalPointer = 
        (PMDL)(((ULONG) DeviceExtension->MdlPhysicalPointer + 3) & ~3);
  		DeviceExtension->MdlPointer = 
    	  (PMDL)(((ULONG) DeviceExtension->MdlPointer + 3) & ~3);

      //
      // Reserve 4 bytes as garbage buffer
      //
      DeviceExtension->TempPhysicalBuf =
        (ULONG) DeviceExtension->MdlPhysicalPointer + sizeof(MDL);
      DeviceExtension->TempLinearBuf =
        (ULONG) DeviceExtension->MdlPointer + sizeof(MDL);

    }

#endif	// DISABLE_MDL
#endif	// T1

    //
    // Initialize misc. parameters
    //
//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      Fix warnings for 'assignment type mismatch' and 'incompatible types..'.
//
#ifdef SOLARIS
    DeviceExtension->powerDownPost = (PPWR_INTERRUPT)NULL;
#else   // SOLARIS
    DeviceExtension->powerDownPost = NULL;
#endif  // SOLARIS

    DeviceExtension->TargetInitFlags = 0;

#ifdef OTHER_OS

//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      Fix warnings for 'assignment type mismatch' and 'incompatible types..'.
//
#ifdef SOLARIS
    DeviceExtension->cpuModePost = (PCPU_MODE_CHANGE)NULL;
#else   // SOLARIS
    DeviceExtension->cpuModePost = NULL;
#endif  // SOLARIS

#endif	// OTHER_OS

    //
    // Initialize host state information
    //
#ifndef DISABLE_SREG

    AMDUpdateInfo (DeviceExtension, HBAInitialization);

#endif	// DISABLE_SREG

    AMDPrint((0,"System flags is %x.\n",*((PUSHORT)&DeviceExtension->SysFlags)));

GetNextController:

    //
    // Go for next controller
    //
    *Again = TRUE;

#ifndef DISABLE_DIO

    //
    // If there is no PCI function supported by system, we have to
    // remember next scan place
    //
    if (DeviceExtension->PciConfigInfo.PciFuncSupport == FALSE) {

      //
      // Current device location
      //
      pInitData->busNumber = DeviceExtension->PciConfigInfo.BusNumber;
      pInitData->deviceNumber = DeviceExtension->PciConfigInfo.DeviceNumber;

      //
      // Get next device scan location
      //
      pInitData->functionNumber = 0;										

      if (pInitData->deviceNumber < 
           (DeviceExtension->PciConfigInfo.Method == 1 ?
            M1_MAX_DEVICE_NUMBER : M2_MAX_DEVICE_NUMBER)) {

    	  pInitData->deviceNumber++;

      } else {

        pInitData->deviceNumber = 0;

        if (pInitData->busNumber < MAXIMUM_BUS_NUMBER) {

          pInitData->busNumber++;

        } else {

          *Again = FALSE;

        }
      }
    }

#endif	// DISABLE_DIO

    return status;
}

LOCAL
ULONG
AMDFindPciAdapter(
    IN OUT PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )

/*++

Routine Description:

    This routine first uses OS's PCI API (when available) to detect 
    AMD device.  if not found, the routine switchs to the direct I/O 
    accessing mechanism to detect the device.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.
    ConfigInfo - Supplies the known configuraiton information.

Return Value:

    Returns a status indicating whether a driver is present or not.

--*/

{
    UCHAR method2Data = 0;
    BOOLEAN scanDone = 0;
    ULONG PCIRegister;
    USHORT regData;
    USHORT vendorId;

    //
    // Added for debug
    //
    AMDPrint((0,"AMDFindPciAdapter.\n"));
    SetBreakPoint

//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      #ifdef out all of this routine which touches PCI config space;
//      the Solaris driver handles this.
//
#ifndef SOLARIS

    //
    // Map configuration I/O space into the virtual address space.
    //
    DeviceExtension->PciConfigBase = ScsiPortGetDeviceBase(
        DeviceExtension,                      // HwDeviceExtension
        ConfigInfo->AdapterInterfaceType,     // AdapterInterfaceType
        ConfigInfo->SystemIoBusNumber,        // SystemIoBusNumber
        ScsiPortConvertUlongToPhysicalAddress(CONFIG_REG_BASE),
        sizeof(PCI_CONFIG_REGS),              // NumberOfBytes
        TRUE                                  // InIoSpace
        );

    if (DeviceExtension->PciConfigBase == NULL) {
        AMDPrint((0, "\nAMDFindPciAdapter: Failed to map configuration\
             registers into system space.\n"));
        return(SP_RETURN_NOT_FOUND);
    }
  
    //
    // Map I/O space 0xC000 - 0xCFFF into the virtual address space.
    //
    DeviceExtension->PciMappedBase = ScsiPortGetDeviceBase(
        DeviceExtension,                      // HwDeviceExtension
        ConfigInfo->AdapterInterfaceType,     // AdapterInterfaceType
        ConfigInfo->SystemIoBusNumber,        // SystemIoBusNumber
        ScsiPortConvertUlongToPhysicalAddress(METHOD2_CONFIG_SPACE),
        METHOD2_CONFIG_SPACE_SIZE,            // NumberOfBytes
        TRUE                                  // InIoSpace
        );
  
    if (DeviceExtension->PciMappedBase == NULL) {
      AMDPrint((0, "\nAMDFindPciAdapter: Failed to map I/O 0xC000 4KB\
           space into system space.\n"));
      ScsiPortFreeDeviceBase(DeviceExtension, DeviceExtension->PciConfigBase);
      return(SP_RETURN_NOT_FOUND);
    }

#ifdef OTHER_OS
#ifndef DISABLE_DIO

    //
    // Check if PCI mechanism information available
    //
    if (UserSetPciMechanism == TRUE) {

      //
      // Set PCI mechanism
      //
      DeviceExtension->PciConfigInfo.Method = PciMechanism;

      //
      // Scan PCI buses for the device by direct IO accessing
      //
      AMDIoGetPCIConfig (DeviceExtension);

      //
      // Done
      //
      goto ScanCompleted;

    }

#endif	// #ifdef OTHER_OS
#endif	// #ifndef DISABLE_DIO

#ifndef DISABLE_DIO

    //
    // Get PCI device information
    //
    if (DeviceExtension->PciConfigInfo.PciFuncSupport == FALSE) {

      //
      // Start direct I/O scan for the PCI device
      //
      // Do PCI bridge 2 test first.  Initialize method 2 data.
      //
      ((PPCI_CONFIG_M2_DATA)(&method2Data))->Key = CONFIG_MODE;
  
      //
      // Map bus 0 PCI register space into I/O space 0xC000 - 0xCFFF
      //
      ScsiPortWritePortUchar(
          &(DeviceExtension->PciConfigBase->PciConfigM2Regs.ForwordAddressReg),
          0);
      ScsiPortWritePortUchar(
          &(DeviceExtension->PciConfigBase->PciConfigM2Regs.ConfigSpaceEnableReg),
          method2Data);
    
      //
      // Read vendor ID.
      //
      vendorId = (USHORT) (ScsiPortReadPortUlong(
               (PULONG) &(DeviceExtension->PciMappedBase->IDRegs)) & (ULONG)WORD_MASK);
  
      //
      // If passed bridge 2 test
      //
      if (vendorId != INVALID_VENDOR_ID) {
  
        //
        // Assume PCI mechanism 2 is being used in the system
        //
        DeviceExtension->PciConfigInfo.Method = 2;
  
        //
        // Scan PCI devices using mechanism 2
        //
        AMDIoGetPCIConfig (DeviceExtension);
  
        //
        // If no device found
        //
        if (DeviceExtension->PciConfigInfo.BaseAddress == DEVICE_NOT_SEEN) {
  
          //
          // PCI device does not be found.  Check if it is Intel
          // PCI chipset
          //
          if (vendorId == INTEL_PCI_ID) {
  
            //
            // If it is Intel chipset, set scan done flag
            //
            scanDone = 1;
  
          }
  
        } else {
  
          //
          // Found PCI device.  Set scan done flag
          //
          scanDone = 1;
  
        }
      }
  
      //
      // If scan done flag not set, use PCI mechanism 1 to scan the bus
      //
      if (!scanDone) {
  
        //
        // Assume PCI mechanism 1 is being used in the system
        //
        DeviceExtension->PciConfigInfo.Method = 1;
  
        //
        // Scan PCI bus using mechanism 1
        //
        AMDIoGetPCIConfig (DeviceExtension);
  
      }
    }

#endif	// #ifndef DISABLE_DIO

#ifndef NT_31

#ifndef DISABLE_DIO

    else

#endif	// #ifndef DISABLE_DIO

      //
      // Get PCI device information using OS function
      //
      AMDSysGetPciConfig (DeviceExtension);

#endif	// #ifndef NT_31
    
#ifdef OTHER_OS
#ifndef DISABLE_DIO

ScanCompleted:

#endif	// #ifdef OTHER_OS
#endif	// #ifndef DISABLE_DIO

    //
    // If device not found or disabled, return with not found status
    //
    if (DeviceExtension->PciConfigInfo.BaseAddress == DEVICE_NOT_SEEN ||
        DeviceExtension->PciConfigInfo.BaseAddress == DEVICE_DISABLED) {

      //
      // Unmap mapped configuration I/O spaces
      //
      ScsiPortFreeDeviceBase(DeviceExtension, DeviceExtension->PciConfigBase);
      ScsiPortFreeDeviceBase(DeviceExtension, DeviceExtension->PciMappedBase);

      return (SP_RETURN_NOT_FOUND);

    }

//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      #ifdef out the above routine; the driver provides a replacement.
//
#endif    // #ifndef SOLARIS

#ifdef COMPAQ_MACHINE

    //
    // Get machine manufacturer information from PCI space register
    //
    PCIRegister = 
      (ULONG) &((PCONFIG_SPACE_HEADER) 0)->SCSIStateReg[DeviceExtension->AdapterBusId];
    AMDAccessPCIRegister (DeviceExtension, PCIRegister, OneWord, &regData, ReadPCI);

    //
    // Set machine type
    ////5/4/95
//    DeviceExtension->SysFlags.MachineType =
//      ((PPCI_HOST_STATE)(&regData))->CompaqSystem ? COMPAQ : NON_COMPAQ;


#endif	// #ifdef COMPAQ_MACHINE

#ifndef DISABLE_SREG

    //
    // Check if information in PCI scratch registers is valid
    //
    PCIRegister = (ULONG) &((PCONFIG_SPACE_HEADER) 0)->SCSIStateReg[0];
    while (PCIRegister <= ((ULONG) &((PCONFIG_SPACE_HEADER) 0)->SCSIStateReg[7])) {

      //
      // Skip host adapter data
      //
      if (PCIRegister != ((ULONG) &((PCONFIG_SPACE_HEADER) 0)->
          SCSIStateReg[DeviceExtension->AdapterBusId])) {

        //
        // Read phase information from scratch register
        //
        AMDAccessPCIRegister (DeviceExtension, PCIRegister, OneWord, &regData, ReadPCI);

        //
        // Check if target phase is idle.
        //
        if (((PPCI_TARGET_STATE)(&regData))->TargetExist &&
            ((PPCI_TARGET_STATE)(&regData))->SCSIPhase != TARGET_IDLE) {

          //
          // Phase is not idle.  Information is invalid.  Clear the target
          // bit so later driver will not use the wrong information
          //
          ((PPCI_TARGET_STATE)(&regData))->TargetExist = 0;

          //
          // Write information back to scratch register
          //
          AMDAccessPCIRegister (DeviceExtension, PCIRegister, OneWord, &regData, WritePCI);

        }
      }

      //
      // Check next scratch register
      //
      PCIRegister += 2;

    }

#else	// #ifndef DISABLE_SREG

    //
    // Check if information in PCI scratch registers is valid
    //
    for (regData = 0; regData < 8; regData++) {

      if (regData != DeviceExtension->AdapterBusId) {

        //
        // Check if target phase is idle.
        //
        if (((PPCI_TARGET_STATE)(&DeviceExtension->SCSIState[regData]))->TargetExist &&
            ((PPCI_TARGET_STATE)(&DeviceExtension->SCSIState[regData]))->SCSIPhase != TARGET_IDLE) {

          //
          // Phase is not idle.  Information is invalid.  Clear the target
          // bit so later driver will not use the wrong information
          //
          ((PPCI_TARGET_STATE)(&DeviceExtension->SCSIState[regData]))->TargetExist = 0;

        }
      }
    }

#endif	// #ifndef DISABLE_SREG

#ifdef OTHER_OS

    PciMechanism = DeviceExtension->PciConfigInfo.Method;

#endif

    return (SP_RETURN_FOUND);
}

LOCAL
VOID
AMDStartDMA(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine set DMA registers in CCB and start data transfer.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.

Return Value:

    DMA operation status.

--*/

{
    ULONG dmaCmd = 0;
    ULONG dmaStartAddr = DeviceExtension->ActiveDMAPointer;
    ULONG i;

    //
    // Added for debug
    //
    AMDPrint((0,"AMDStartDMA.\n"));
    SetBreakPoint

    //
    // Setup DMA idle command
    //
    ((PDMA_CMD_DATA)(&dmaCmd))->TransferDirection =
        (DeviceExtension->dataPhase == DATA_IN) ? 1 : 0;
    ((PDMA_CMD_DATA)(&dmaCmd))->Command = DMA_IDLE;

#ifndef T1
#ifndef DISABLE_MDL

    //
    // Check silicon reversion id.  For T2/G2 and above, we are going to
    // implement MDL or SG data xfer
    //
    if (DeviceExtension->PciConfigInfo.ChipRevision >= 0x10 &&
        !(DeviceExtension->AdapterFlags & PD_DATA_OVERRUN)) {

      //
      // Support Scather/Gather
      //
      ((PDMA_CMD_DATA)(&dmaCmd))->EnableScatterGather = 1;

      //
      // Set SG list base address register anyway
      //
      DMA_WRITE(DeviceExtension->Adapter, MDLAddressReg,
                (ULONG) DeviceExtension->MdlPhysicalPointer);

    }

#endif
#endif

    //
    // Setup DMA command register
    //
    DMA_WRITE(DeviceExtension->Adapter, CommandReg, dmaCmd);
   
    //
    // Check silicon reversion id.  For T2/G2 and above, do not have to
    // deal with less than or equal to 4 bytes xfer
    //
    if (DeviceExtension->PciConfigInfo.ChipRevision < 0x10) {

      //
      // Check if one byte transfer flag set.  If yes, program DMA 
      // count by 1
      //
      if (DeviceExtension->AdapterFlags & PD_ONE_BYTE_XFER) {

        DeviceExtension->ActiveDMALength = 1;

      }

      //
      // If transfer length is less than or equal to 4, set flag
      //
      if (DeviceExtension->ActiveDMALength <= 4) {

        DeviceExtension->AdapterFlags |= PD_BUFFERED_DATA;

        //
        // If in data out phase, copy data to temporary data buffer first
        //
        if (DeviceExtension->AdapterStatus.Phase == DATA_OUT) {

          for (i = 0; i < DeviceExtension->ActiveDMALength; i++) {

            *((PUCHAR) (DeviceExtension->TempLinearBuf + i)) =
              *((PUCHAR) (DeviceExtension->ActiveDataPointer + i));

          }

#ifdef OTHER_OS

          //
          // Free the virtual address if necessary
          //
          if (DeviceExtension->SysFlags.DMAMode != IO_DMA_METHOD_LINEAR)
            ScsiPortFreeVirtualAddress ((PVOID)DeviceExtension->ActiveDataPointer,
                                        DeviceExtension->ActiveLuRequest,
                                        DeviceExtension->ActiveLuRequest->DataTransferLength
                                       );

#endif

        }

        //
        // Set Starting Address register to temporary buffer address
        //
        dmaStartAddr = DeviceExtension->TempPhysicalBuf;

      }
    }

    //
    // Set Starting Transfer Count register
    //
    DMA_WRITE(DeviceExtension->Adapter, StartTransferCountReg,
              DeviceExtension->ActiveDMALength);

    //
    // Set Starting transfer address
    //
    DMA_WRITE(DeviceExtension->Adapter, StartTransferAddressReg, dmaStartAddr);

    //
    // Start DMA
    //
    ((PDMA_CMD_DATA)(&dmaCmd))->Command = DMA_START;
    DMA_WRITE(DeviceExtension->Adapter, CommandReg, dmaCmd);

}
LOCAL
VOID
AMDFlushDMA(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine stops DMA transfer.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.

Return Value:

    None

--*/

{
    ULONG dmaCmd = 0;

    //
    // Added for debug
    //
    AMDPrint((0,"AMDFlushDMA.\n"));
    SetBreakPoint

    //
    // Stop DMA
    //
    ((PDMA_CMD_DATA)(&dmaCmd))->Command = DMA_IDLE;
    DMA_WRITE (DeviceExtension->Adapter, CommandReg, dmaCmd);

}

LOCAL
VOID
AMDSaveDataPointer(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine saves data pointer to SRB extension.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.

    Srb - SCSI request block from OS.

Return Value:

    None

--*/

{
    SRB_EXT(Srb)->SavedDataLength = DeviceExtension->ActiveDataLength;
    SRB_EXT(Srb)->SavedDataTransferred = SRB_EXT(Srb)->TotalDataTransferred;

#ifdef OTHER_OS

    if (DeviceExtension->SysFlags.DMAMode != IO_DMA_METHOD_LINEAR) {

      SRB_EXT(Srb)->SavedSGEntry = DeviceExtension->ActiveSGEntry;
      SRB_EXT(Srb)->SavedSGTEntryOffset = DeviceExtension->ActiveSGTEntryOffset;

#ifndef T1
#ifndef DISABLE_MDL

      if (DeviceExtension->SysFlags.DMAMode == IO_DMA_METHOD_MDL){
        SRB_EXT(Srb)->SavedSGPhyscialEntry = (PULONG)DeviceExtension->MdlPhysicalPointer;
		}

#endif
#endif

    } else

#endif

    SRB_EXT(Srb)->SavedDataPointer = DeviceExtension->ActiveDataPointer;

}
LOCAL
VOID
AMDRestoreDataPointer(
    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
    PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine restores data pointer to SRB extension.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.

    Srb - SCSI request block from OS.

Return Value:

    None

--*/

{

    DeviceExtension->ActiveDataLength = SRB_EXT(Srb)->SavedDataLength;
    SRB_EXT(Srb)->TotalDataTransferred = SRB_EXT(Srb)->SavedDataTransferred;

#ifdef OTHER_OS

    if (DeviceExtension->SysFlags.DMAMode != IO_DMA_METHOD_LINEAR) {

      DeviceExtension->ActiveSGEntry = SRB_EXT(Srb)->SavedSGEntry;
      DeviceExtension->ActiveSGTEntryOffset = SRB_EXT(Srb)->SavedSGTEntryOffset;

#ifndef T1
#ifndef DISABLE_MDL

      if (DeviceExtension->SysFlags.DMAMode == IO_DMA_METHOD_MDL) {
        DeviceExtension->MdlPhysicalPointer = (PMDL)SRB_EXT(Srb)->SavedSGPhyscialEntry;
		}

#endif
#endif

    } else

#endif

    DeviceExtension->ActiveDataPointer = SRB_EXT(Srb)->SavedDataPointer;

}

#ifdef COMPAQ_MACHINE


LOCAL
VOID
AMDTurnLEDOn(
    IN BOOLEAN TurnOn
    )

/*++

Routine Description:

    This routine turns on / off the disk LED for Compaq system

Arguments:

    TurnOn - Boolean flag for operation

Return Value:

    None

--*/

{
    UCHAR data;

    ScsiPortWritePortUchar ((PUCHAR)0xc94, 0x03);
    data = ScsiPortReadPortUchar ((PUCHAR)0xc99);
    data = TurnOn == TRUE ? data | 0x40 : data & ~0x40;
    ScsiPortWritePortUchar ((PUCHAR)0xc99, data);

}

#endif



//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      #ifdef out this entire routine; the driver provides a replacement.
//
#ifndef SOLARIS
LOCAL
VOID
AMDAccessPCIRegister(
                    IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
                    IN ULONG PCIRegister,
                    IN ULONG RegLength,
                    IN PVOID DataPtr,
                    IN ULONG Operation
                    )

/*++

Routine Description:

    This routine accesses PCI configuration register.  Assume all the
    PCI device information (bus, device and function) are available.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.
    PCIRegister - Register wanted to access.
    RegLength - Register length.
    DataPtr - Address used by data
    Operation - Read or write operation

Return Value:

    None.  Note: Access should not cross double word register boundary.

--*/

{

#ifdef DISABLE_DIO

    UCHAR buffer[0x50];
    PCI_SLOT_NUMBER slotNumber;
    ULONG length;

    //
    // Set PCI device address
    //
    slotNumber.u.AsULONG = 0;
    slotNumber.u.bits.DeviceNumber = DeviceExtension->PciConfigInfo.DeviceNumber;
    slotNumber.u.bits.FunctionNumber = DeviceExtension->PciConfigInfo.FunctionNumber;

    //
    // Check operation type
    //
    if (Operation == ReadPCI) {

      //
      // Set length
      //
      length = PCIRegister + 4;

      //
      // Read PCI registers
      //
      ScsiPortGetBusData (DeviceExtension,
                          PCIConfiguration,
                          DeviceExtension->PciConfigInfo.BusNumber,
                          *((PULONG)&slotNumber),
                          (PVOID)&buffer,
                          length
                          );

      //
      // Return register data
      //
      switch (RegLength) {

        case OneByte:

               *(PUCHAR)DataPtr = *(PUCHAR)((PUCHAR)buffer + PCIRegister);
               break;

        case OneWord:

               *(PUSHORT)DataPtr = *(PUSHORT)((PUCHAR)buffer + PCIRegister);
               break;

        case OneDWord:

               *(PULONG)DataPtr = *(PULONG)((PUCHAR)buffer + PCIRegister);

      }

    } else {

      //
      // Set data inside buffer
      //
      switch (RegLength) {

        case OneByte:

               *(PUCHAR)buffer = *(PUCHAR)DataPtr;
               length = 1;
               break;

        case OneWord:

               *(PUSHORT)buffer = *(PUSHORT)DataPtr;
               length = 2;
               break;

        case OneDWord:

               *(PULONG)buffer = *(PULONG)DataPtr;
               length = 4;

      }

      //
      // Write data into PCI register
      //
      ScsiPortSetBusDataByOffset (DeviceExtension,
                                  PCIConfiguration,
                                  DeviceExtension->PciConfigInfo.BusNumber,
                                  *((PULONG)&slotNumber),
                                  (PVOID)&buffer,
                                  PCIRegister,
                                  length
                                  );
    }

#else

    ULONG method1Data;
    UCHAR method2Data;
    ULONG readData;
    ULONG writeData;
    ULONG registerAddress;

    //
    // For mechanism 1 access
    //
    if (DeviceExtension->PciConfigInfo.Method == 1) {

      //
      // Set data for forward register
      //
      ((PPCI_CONFIG_M1_DATA)(&method1Data))->Register = PCIRegister & ~3;
      ((PPCI_CONFIG_M1_DATA)(&method1Data))->DeviceNumber =
        DeviceExtension->PciConfigInfo.DeviceNumber;
      ((PPCI_CONFIG_M1_DATA)(&method1Data))->BusNumber =
        DeviceExtension->PciConfigInfo.BusNumber;
      ((PPCI_CONFIG_M1_DATA)(&method1Data))->FunctionNumber =
        DeviceExtension->PciConfigInfo.FunctionNumber;
      ((PPCI_CONFIG_M1_DATA)(&method1Data))->EnableConfig = 1;

      //
      // Write to address register
      //
      ScsiPortWritePortUlong(
        (PULONG) &(DeviceExtension->PciConfigBase->PciConfigM1Regs.ConfigAddressReg),
        method1Data);

      //
      // Read a double word from the PCI space register place
      //
      readData = ScsiPortReadPortUlong(
        (PULONG) &(DeviceExtension->PciConfigBase->PciConfigM1Regs.ConfigDataReg));

    //
    // For mechanism 2 access
    //
    } else {

      //
      // Set bus number
      //
      ScsiPortWritePortUchar(&(DeviceExtension->PciConfigBase->PciConfigM2Regs.ForwordAddressReg),
                             DeviceExtension->PciConfigInfo.BusNumber);

      //
      // Setup configuration data for method 2
      //
      ((PPCI_CONFIG_M2_DATA)(&method2Data))->ZeroBits = 0;
      ((PPCI_CONFIG_M2_DATA)(&method2Data))->Key = CONFIG_MODE;
      ((PPCI_CONFIG_M2_DATA)(&method2Data))->FunctionNumber =
                             DeviceExtension->PciConfigInfo.FunctionNumber;

      //
      // Set configuration mode
      //
      ScsiPortWritePortUchar(&(DeviceExtension->PciConfigBase->PciConfigM2Regs.ConfigSpaceEnableReg),
                             method2Data);

      //
      // Get register i/o address
      //
      registerAddress = (ULONG) DeviceExtension->PciMappedBase +
                        DeviceExtension->PciConfigInfo.DeviceNumber * 0x100 +
                        PCIRegister;

      //
      // Read a double word from register space
      //
      readData = ScsiPortReadPortUlong ((PULONG) (registerAddress & ~3));

    }

    //
    // Check operation type
    //
    if (Operation == ReadPCI) {

      //
      // Align data
      //
      readData >>= (PCIRegister & 3) * 8;

      //
      // Return register data
      //
      switch (RegLength) {

        case OneByte:

               *(PUCHAR)DataPtr = (UCHAR) readData;
               break;

        case OneWord:

               *(PUSHORT)DataPtr = (USHORT) readData;
               break;

        case OneDWord:

               *(PULONG)DataPtr = (ULONG) readData;

      }

    } else {
      
      //
      // Update register data
      //
      switch (RegLength) {

        case OneByte:

               readData &= ~((ULONG)BYTE_MASK << ((PCIRegister & 3) * 8));
               writeData = ((ULONG) (*(PUCHAR) DataPtr)) & (ULONG)BYTE_MASK;
               writeData <<= (PCIRegister & 3) * 8;
               writeData |= readData;
               break;

        case OneWord:

               readData &= ~((ULONG)WORD_MASK << ((PCIRegister & 3) * 8));
               writeData = ((ULONG) (*(PUSHORT) DataPtr)) & (ULONG)WORD_MASK;
               writeData <<= (PCIRegister & 3) * 8;
               writeData |= readData;
               break;

        case OneDWord:

               writeData = *(PULONG) DataPtr;

      }

      //
      // Write data back to the register
      //
      if (DeviceExtension->PciConfigInfo.Method == 1) {
      
        //
        // Write to address register
        //
        ScsiPortWritePortUlong(
          (PULONG) &(DeviceExtension->PciConfigBase->PciConfigM1Regs.ConfigAddressReg),
          method1Data);

        //
        // Write update data back to the register
        //
        ScsiPortWritePortUlong(
           (PULONG) &(DeviceExtension->PciConfigBase->PciConfigM1Regs.ConfigDataReg), writeData);

      } else {

        //
        // Write update data back to the register
        //
        ScsiPortWritePortUlong((PULONG) (registerAddress & ~3), writeData);

      }
    }

    //
    // Disable configuration mode
    //
    if (DeviceExtension->PciConfigInfo.Method == 1) {

      ((PPCI_CONFIG_M1_DATA)(&method1Data))->EnableConfig = 0;
      ScsiPortWritePortUlong(
        (PULONG) &(DeviceExtension->PciConfigBase->PciConfigM1Regs.ConfigAddressReg),
        method1Data);

    } else {

      ((PPCI_CONFIG_M2_DATA)(&method2Data))->Key = NORMAL_MODE;
      ScsiPortWritePortUchar(&(DeviceExtension->PciConfigBase->PciConfigM2Regs.ConfigSpaceEnableReg),
        method2Data);

    }

#endif

}

//
// Specific mods for Solaris 2.4 pcscsi driver
//
//      #ifdef out the above routine; the driver provides a replacement.
//
#endif    // SOLARIS

#ifndef NT_31


LOCAL
VOID
AMDSysGetPciConfig(
    IN OUT PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine uses OS PCI interface to get the device information.
    If everything ok, the I/O base address and IRQ number will be
    retrieved from PCI configuration space header and returned with bus
    number, device number and function number in device extension PCI data
    structure.  Otherwise(device not found, device disabled etc.), a error
    information will be returned in base address field.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.

Return Value:

    None.

--*/

{

    PCI_SLOT_NUMBER slotNumber;
    ULONG status;

#ifndef DISABLE_DIO

    UCHAR method2Data;
    ULONG regsBaseAddress;

#endif	// #ifndef DISABLE_DIO

#ifdef DISABLE_SREG

    UCHAR id;
    PUSHORT infoPtr;

#endif	// #ifdef DISABLE_SREG

    ULONG regAddress;
    USHORT data;
    UCHAR buffer[0x50];
    PPCI_COMMON_CONFIG slotInfo;

    //
    // Set no device status first
    //
    DeviceExtension->PciConfigInfo.BaseAddress = DEVICE_NOT_SEEN;

    //
    // Initialize buffer pointer
    //
    slotInfo = (PPCI_COMMON_CONFIG)&buffer;

    //
    // Retrieve device information from PCI space
    //
    slotNumber.u.AsULONG = 0;
    slotNumber.u.bits.DeviceNumber = DeviceExtension->PciConfigInfo.DeviceNumber;
    slotNumber.u.bits.FunctionNumber = DeviceExtension->PciConfigInfo.FunctionNumber;
    status = ScsiPortGetBusData (DeviceExtension,
                                 PCIConfiguration,
                                 DeviceExtension->PciConfigInfo.BusNumber,
                                 *((PULONG)&slotNumber),
                                 slotInfo,
                                 0x50
                                 );

    //
    // Check result.  If 0 returned(bus not exist) or 2 returned
    // (device not exist), return
    //
    if (status == 0 || status == 2) {

      return;

    }
	 	
    //
    // Check device Id and vendor Id
    //
    if (slotInfo->VendorID == (USHORT)(AMD_PCI_SCSI_ID & (ULONG)WORD_MASK)
        && slotInfo->DeviceID == (USHORT)(AMD_PCI_SCSI_ID >> 16)) {

      //
      // We found AMD device.  First check if device is disabled
      //
      if ((slotInfo->u.type0.BaseAddresses[0] & ~1) == DEVICE_DISABLED
	       || !(slotInfo->Command & PCI_ENABLE_IO_SPACE)) {

        //
        // Set device base address as disabled value and return
        //
        DeviceExtension->PciConfigInfo.BaseAddress = DEVICE_DISABLED;
        return;

      }

      //
      // Retrieve device information.  Get I/O base address
      //
      DeviceExtension->PciConfigInfo.BaseAddress =
        slotInfo->u.type0.BaseAddresses[0] & ~1;

      //
      // Get IRQ number from PCI header
      //
      DeviceExtension->PciConfigInfo.IRQ =
        slotInfo->u.type0.InterruptLine;

      //
      // Get chip revision
      //
      DeviceExtension->PciConfigInfo.ChipRevision =
        slotInfo->RevisionID;
		//
		// Save machine type information here		//5/4/95
		// 
      DeviceExtension->SysFlags.MachineType = ((PPCI_HOST_STATE)
        (&buffer[0x40 + (DeviceExtension->AdapterBusId << 1)]))->CompaqSystem
		  ? COMPAQ : NON_COMPAQ;

#ifndef DISABLE_DIO

      //
      // Detect PCI mechanism.  Assume PCI mechanism 2 is used.
      //
      ((PPCI_CONFIG_M2_DATA)(&method2Data))->ZeroBits = 0;
      ((PPCI_CONFIG_M2_DATA)(&method2Data))->Key = CONFIG_MODE;
      ((PPCI_CONFIG_M2_DATA)(&method2Data))->FunctionNumber = 
        (UCHAR)DeviceExtension->PciConfigInfo.FunctionNumber;

      //
      // Set bus number to search
      //
      ScsiPortWritePortUchar(&(DeviceExtension->PciConfigBase->PciConfigM2Regs.ForwordAddressReg),
        (UCHAR)DeviceExtension->PciConfigInfo.BusNumber);
    
      //
      // Set configuration mode
      //
      ScsiPortWritePortUchar(&(DeviceExtension->PciConfigBase->PciConfigM2Regs.ConfigSpaceEnableReg),
          method2Data);

      //
      // Get device register base address
      //
      regsBaseAddress = (ULONG) DeviceExtension->PciMappedBase + 
                          DeviceExtension->PciConfigInfo.DeviceNumber * 0x100;

      //
      // Read vendor ID
      //
      regAddress = (ULONG) &((PCONFIG_SPACE_HEADER) 0)->IDRegs;

      //
      // Get PCI mechanism
      //
      if (ScsiPortReadPortUlong(
          (PULONG) (regsBaseAddress + regAddress)) == AMD_PCI_SCSI_ID) {

        DeviceExtension->PciConfigInfo.Method = 2;
   
      } else {
   
        DeviceExtension->PciConfigInfo.Method = 1;
   
      }

#endif

      //
      // Initialize PCI command register
      //
      data = slotInfo->Command;
      ((PPCI_SCSI_COMMAND)(&data))->BusMaster = 1;
      regAddress = (ULONG) &((PCONFIG_SPACE_HEADER) 0)->CmdStatusRegs;
      AMDAccessPCIRegister (DeviceExtension, regAddress, OneWord, &data, WritePCI);

#ifdef DISABLE_SREG

      //
      // Save target information
      //
      infoPtr = (PUSHORT)&buffer[0x40];
      for (id = 0; id < 8; id++)
        DeviceExtension->SCSIState[id] = *(infoPtr + id);

#endif

    }
}

#endif

#ifndef DISABLE_DIO


LOCAL
VOID
AMDIoGetPCIConfig(
    IN OUT PSPECIFIC_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine scans the PCI buses and the devices connected for
    each bus to search the valid AMD GOLDEN GATE controller.  If found,
    the I/O base address and IRQ number will be retrieved from PCI configu-
    ration space header and returned with bus number, device number and
    function number in device extension PCI data structure. Otherwise
    (device not found, device disabled etc.), a error information will be 
    returned in base address field.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.

Return Value:

    None.

--*/

{
    UCHAR busNumber, deviceNumber;
    UCHAR functionNumber;
    ULONG method1Data;
    UCHAR method2Data;
    ULONG regsBaseAddress;
    ULONG regAddress;
    ULONG data;

#ifdef DISABLE_SREG

    UCHAR id;

#endif

    //
    // Added for debug
    //
    AMDPrint((0,"AMDIoGetPCIConfig.\n"));
    SetBreakPoint

    //
    // Set no device status first
    //
    DeviceExtension->PciConfigInfo.BaseAddress = DEVICE_NOT_SEEN;

    //
    // Search using method 1
    //
    if (DeviceExtension->PciConfigInfo.Method == 1) {

      //
      // Setup configuration data for method 1
      //
      ((PPCI_CONFIG_M1_DATA)(&method1Data))->EnableConfig = 1;

      //
      // Search the PCI buses
      //
      for (busNumber = DeviceExtension->PciConfigInfo.BusNumber;
           busNumber < MAXIMUM_BUS_NUMBER; busNumber++) {

        ((PPCI_CONFIG_M1_DATA)(&method1Data))->BusNumber = busNumber;

        //
        // Search devices connected to the PCI bus
        //
        for (deviceNumber = DeviceExtension->PciConfigInfo.DeviceNumber;
             deviceNumber < M1_MAX_DEVICE_NUMBER; deviceNumber++) {

          ((PPCI_CONFIG_M1_DATA)(&method1Data))->DeviceNumber = deviceNumber;

          //
          // Search functions
          //
          for (functionNumber = DeviceExtension->PciConfigInfo.FunctionNumber;
               functionNumber < 2; functionNumber++) {

            ((PPCI_CONFIG_M1_DATA)(&method1Data))->FunctionNumber = functionNumber;

            //
            // Read vendor ID and device ID
            //
            ((PPCI_CONFIG_M1_DATA)(&method1Data))->Register = 
              (ULONG) &((PCONFIG_SPACE_HEADER) 0)->IDRegs;
            if (AMDPciReadM1(DeviceExtension->PciConfigBase, method1Data)
               == AMD_PCI_SCSI_ID) {
   
              //
              // Here we are sure that AMD GOLDEN GATE adapter is found.
              // Get register base address from PCI header
              //
              ((PPCI_CONFIG_M1_DATA)(&method1Data))->Register =
                (ULONG) &((PCONFIG_SPACE_HEADER) 0)->BaseAddress0Reg;
              DeviceExtension->PciConfigInfo.BaseAddress =
                AMDPciReadM1(DeviceExtension->PciConfigBase, method1Data);

              //
              // Ignore bit 0.  For I/O space mapping, bit 0 is always set
              //
              DeviceExtension->PciConfigInfo.BaseAddress &= ~1;

              //
              // If device is disabled, search next device
              //
              if (DeviceExtension->PciConfigInfo.BaseAddress == DEVICE_DISABLED) {

                break;

              }

              //
              // Command register I/O enable bit must be set
              //
              ((PPCI_CONFIG_M1_DATA)(&method1Data))->Register =
                (ULONG) &((PCONFIG_SPACE_HEADER) 0)->CmdStatusRegs;
              data = AMDPciReadM1(DeviceExtension->PciConfigBase, method1Data);
              if (((PPCI_SCSI_COMMAND)(&data))->IOSpace != 1) {

                //
                // Set device base address as disabled value
                //
                DeviceExtension->PciConfigInfo.BaseAddress = DEVICE_DISABLED;
                break;

              }

              //
              // Initialize PCI command register
              //
              ((PPCI_SCSI_COMMAND)(&data))->BusMaster = 1;
              ScsiPortWritePortUlong(
                (PULONG) &(DeviceExtension->PciConfigBase->PciConfigM1Regs.ConfigAddressReg),
                method1Data);
              ScsiPortWritePortUlong(
                (PULONG) &(DeviceExtension->PciConfigBase->PciConfigM1Regs.ConfigDataReg),
                data);

              //
              // Get IRQ number from PCI header
              //
              ((PPCI_CONFIG_M1_DATA)(&method1Data))->Register =
                (ULONG) &((PCONFIG_SPACE_HEADER) 0)->InterruptReg;
              DeviceExtension->PciConfigInfo.IRQ =
                AMDPciReadM1(DeviceExtension->PciConfigBase, method1Data) & (ULONG)BYTE_MASK;

              //
              // Get chip revision
              //
              ((PPCI_CONFIG_M1_DATA)(&method1Data))->Register =
                (ULONG) &((PCONFIG_SPACE_HEADER) 0)->Misc1Regs;
              DeviceExtension->PciConfigInfo.ChipRevision =
                (UCHAR) AMDPciReadM1(DeviceExtension->PciConfigBase, method1Data);

              //
              // Save bus number, device number and function number
              //
              DeviceExtension->PciConfigInfo.BusNumber = busNumber;
              DeviceExtension->PciConfigInfo.DeviceNumber = deviceNumber;
              DeviceExtension->PciConfigInfo.FunctionNumber = functionNumber;

#ifdef DISABLE_SREG

              //
              // Save target information
              //
              for (id = 0; id < 8; id++) {

                ((PPCI_CONFIG_M1_DATA)(&method1Data))->Register =
                  (ULONG) &((PCONFIG_SPACE_HEADER) 0)->SCSIStateReg[id];
                DeviceExtension->SCSIState[id] =
                  (USHORT) AMDPciReadM1(DeviceExtension->PciConfigBase, method1Data);

              }

#endif

              //
              // Done.
              //
              goto EndIOScan;

            }
          }
        }
      }
    }

    //
    // Search buses using method 2
    //
    else {

      //
      // Setup configuration data for method 2
      //
      ((PPCI_CONFIG_M2_DATA)(&method2Data))->ZeroBits = 0;
      ((PPCI_CONFIG_M2_DATA)(&method2Data))->Key = CONFIG_MODE;

      //
      // Search the PCI buses
      //
      for (busNumber = DeviceExtension->PciConfigInfo.BusNumber;
           busNumber < MAXIMUM_BUS_NUMBER; busNumber++) {

        //
        // Set bus number to search
        //
        ScsiPortWritePortUchar(&(DeviceExtension->PciConfigBase->PciConfigM2Regs.ForwordAddressReg),
            busNumber);

        //
        // Search devices connected to the PCI bus
        //
        for (deviceNumber = DeviceExtension->PciConfigInfo.DeviceNumber;
             deviceNumber < M2_MAX_DEVICE_NUMBER; deviceNumber++) {

          //
          // Search functions
          //
          for (functionNumber = DeviceExtension->PciConfigInfo.FunctionNumber;
               functionNumber < 2; functionNumber++) {
            ((PPCI_CONFIG_M2_DATA)(&method2Data))->FunctionNumber = functionNumber;

            //
            // Set configuration mode
            //
            ScsiPortWritePortUchar(&(DeviceExtension->PciConfigBase->PciConfigM2Regs.ConfigSpaceEnableReg),
                method2Data);

            //
            // Get device register base address
            //
            regsBaseAddress = (ULONG) DeviceExtension->PciMappedBase + deviceNumber * 0x100;

            //
            // Read vendor ID
            //
            regAddress = (ULONG) &((PCONFIG_SPACE_HEADER) 0)->IDRegs;

            if (ScsiPortReadPortUlong(
                (PULONG) (regsBaseAddress + regAddress)) == AMD_PCI_SCSI_ID) {

              //
              // Here we are sure that AMD GOLDEN GATE adapter is found.
              // Read register base address
              //
              regAddress = (ULONG) &((PCONFIG_SPACE_HEADER) 0)->BaseAddress0Reg;
              DeviceExtension->PciConfigInfo.BaseAddress =
                ScsiPortReadPortUlong((PULONG) (regsBaseAddress + regAddress));

              //
              // Ignore bit 0.  For I/O space mapping, bit 0 is always set
              //
              DeviceExtension->PciConfigInfo.BaseAddress &= ~1;

              //
              // If device is disabled, search next device
              //
              if (DeviceExtension->PciConfigInfo.BaseAddress == DEVICE_DISABLED) {

                break;

              }

              //
              // Command register I/O enable bit must be set
              //
              regAddress = (ULONG) &((PCONFIG_SPACE_HEADER) 0)->CmdStatusRegs;
              data = ScsiPortReadPortUlong((PULONG)(regsBaseAddress + regAddress));
              if (((PPCI_SCSI_COMMAND)(&data))->IOSpace != 1) {

                //
                // Set device base address as disabled value
                //
                DeviceExtension->PciConfigInfo.BaseAddress = DEVICE_DISABLED;
                break;

              }

              //
              // Initialize PCI command register
              //
              ((PPCI_SCSI_COMMAND)(&data))->BusMaster = 1;
              ScsiPortWritePortUlong((PULONG)(regsBaseAddress + regAddress), data);

              //
              // Get IRQ number from PCI header
              //
              regAddress = (ULONG) &((PCONFIG_SPACE_HEADER) 0)->InterruptReg;
              DeviceExtension->PciConfigInfo.IRQ =
                ScsiPortReadPortUlong((PULONG) (regsBaseAddress + regAddress))
                & (ULONG)BYTE_MASK;

              //
              // Get chip revision number
              //
              regAddress = (ULONG) &((PCONFIG_SPACE_HEADER) 0)->Misc1Regs;
              DeviceExtension->PciConfigInfo.ChipRevision =
                (UCHAR) ScsiPortReadPortUlong((PULONG) (regsBaseAddress + regAddress));

              //
              // Save bus number, device number and function number
              //
              DeviceExtension->PciConfigInfo.BusNumber = busNumber;
              DeviceExtension->PciConfigInfo.DeviceNumber = deviceNumber;
              DeviceExtension->PciConfigInfo.FunctionNumber = functionNumber;

#ifdef DISABLE_SREG

              //
              // Save target information
              //
              for (id = 0; id < 8; id++) {

                regAddress = (ULONG) &((PCONFIG_SPACE_HEADER) 0)->SCSIStateReg[id];
                DeviceExtension->SCSIState[id] =
                  (USHORT) ScsiPortReadPortUlong((PULONG) (regsBaseAddress + regAddress));

              }

#endif

              //
              // Done
              //
              goto EndIOScan;

            }
          }
        }
      }
    }

EndIOScan:

    //
    // Close PCI space
    //
    if (DeviceExtension->PciConfigInfo.Method == 1) {

      ((PPCI_CONFIG_M1_DATA)(&method1Data))->EnableConfig = 0;
      ScsiPortWritePortUlong(
                  (PULONG) &(DeviceExtension->PciConfigBase->PciConfigM1Regs.ConfigAddressReg),
                  method1Data);

    } else {

      ((PPCI_CONFIG_M2_DATA)(&method2Data))->Key = 0;
      ScsiPortWritePortUchar(&(DeviceExtension->PciConfigBase->PciConfigM2Regs.ConfigSpaceEnableReg),
          method2Data);

    }
}

LOCAL
ULONG
AMDPciReadM1(
            IN PPCI_CONFIG_REGS ConfigBase,
            IN ULONG InfoData
            )

/*++

Routine Description:

    This routine reads PCI configuration register using mechanism 1.

Arguments:

    ConfigBase - Configuration register mapped base address.
    InfoData - Input data which contains accessing information.

Return Value:

    The value in the PCI register.

--*/

{
    ScsiPortWritePortUlong(
                (PULONG) &(ConfigBase->PciConfigM1Regs.ConfigAddressReg),
                InfoData);
    return ScsiPortReadPortUlong(
                (PULONG) &(ConfigBase->PciConfigM1Regs.ConfigDataReg));
}

#endif

#ifndef DISABLE_SREG


LOCAL
VOID
AMDUpdateInfo (
              IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
              IN ULONG Operation
              )

/*++

Routine Description:

    This routine updates SCSI state in the registers which are reserved
    inside PCI configuration space.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.
    Operation - Operation type.

Return Value:

    None

--*/

{

    ULONG PCIRegister;
    USHORT regData;
    USHORT id;
    UCHAR i;

    //
    // Added for debug
    //
    AMDPrint((0,"AMDUpdateInfo: "));
    SetBreakPoint

    //
    // Check if operation is for reset event 
    //
    if (Operation != Reset) {

      //
      // Initialize ID varible
      //
      id = Operation == HBAInitialization ? DeviceExtension->AdapterBusId :
                                            DeviceExtension->TargetId;

      //
      // Read information
      //
      PCIRegister = (ULONG) &((PCONFIG_SPACE_HEADER) 0)->SCSIStateReg[id];
      AMDAccessPCIRegister (DeviceExtension, PCIRegister, OneWord, &regData, ReadPCI);

      //
      // Carry out operation command
      //
      switch (Operation) {

        case HBAInitialization:

              //
              // Added for debug
              //
              AMDPrint ((0, "initialization state.\n"));

              //
              // Initialize host state register
              //
              ((PPCI_HOST_STATE)(&regData))->TargetExist = 0;
              ((PPCI_HOST_STATE)(&regData))->HostAdapter = 1;
              ((PPCI_HOST_STATE)(&regData))->BusReset = 0;
              ((PPCI_HOST_STATE)(&regData))->ZeroBits = 0;

              //
              // Set CPU mode bits
              //
              if (DeviceExtension->SysFlags.CPUMode == PROTECTION_MODE) {

                ((PPCI_HOST_STATE)(&regData))->InitRealMode = 0;
                ((PPCI_HOST_STATE)(&regData))->InitProtMode = 1;

              } else {

                ((PPCI_HOST_STATE)(&regData))->InitRealMode = 1;
                ((PPCI_HOST_STATE)(&regData))->InitProtMode = 0;

              }

              break;

        case Synchronous:

              //
              // Added for debug
              //
              AMDPrint ((0, "update synchronous parameters.  ID = %d\n", id));
  
              //
              // Update target state register with synchronous parameters
              //
              ((PPCI_TARGET_STATE)(&regData))->TargetExist = 1;
              ((PPCI_TARGET_STATE)(&regData))->SyncOffset =
                DeviceExtension->ActiveLogicalUnit->SynchronousOffset;
              ((PPCI_TARGET_STATE)(&regData))->SyncPeriod =
                DeviceExtension->ActiveLogicalUnit->SynchronousPeriod;

              //
              // Set fast SCSI bit if it is for fast SCSI
              //
              ((PPCI_TARGET_STATE)(&regData))->FastScsi = 
                DeviceExtension->ActiveLogicalUnit->Configuration3.FastScsi;

              break;
                       
        case Phase:

              //
              // Added for debug
              //
              AMDPrint ((0, "update phase parameters.  ID = %d\n", id));

              //
              // Update target state register with phase parameters
              //
              ((PPCI_TARGET_STATE)(&regData))->TargetExist = 1;
              ((PPCI_TARGET_STATE)(&regData))->SCSIPhase =
                DeviceExtension->AdapterStatus.Phase;

              break;

        case TargetIdle:

              //
              // Added for debug
              //
              AMDPrint ((0, "target idle phase.  ID = %d\n", id));

              //
              // Update target state register with target idle status
              //
              ((PPCI_TARGET_STATE)(&regData))->TargetExist = 1;
              ((PPCI_TARGET_STATE)(&regData))->SCSIPhase = TARGET_IDLE;

              break;

        case TargetDisconnected:

              //
              // Added for debug
              //
              AMDPrint ((0, "target disconnection phase.  ID = %d\n", id));

              //
              // Update target state register with target disconnection status
              //
              ((PPCI_TARGET_STATE)(&regData))->TargetExist = 1;
              ((PPCI_TARGET_STATE)(&regData))->SCSIPhase = TARGET_DISCNT;

              break;

        case TargetNotExist:

              //
              // Added for debug
              //
              AMDPrint ((0, "target does not exist.  ID = %d\n", id));

              //
              // Update target state register with target disconnection status
              //
              regData = 0;

      }

      //
      // Write updated data back to the register
      //
      AMDAccessPCIRegister (DeviceExtension, PCIRegister, OneWord, &regData, WritePCI);

    } else {

      //
      // Added for debug
      //
      AMDPrint ((0, "bus reset initialization.\n"));

      //
      // Initalize all the state registers
      //
      for (i = 0; i < 8; i++) {

        //
        // Read register
        //
        PCIRegister = (ULONG) &((PCONFIG_SPACE_HEADER) 0)->SCSIStateReg[i];
        AMDAccessPCIRegister (DeviceExtension, PCIRegister, OneWord, &regData, ReadPCI);

        //
        // Initialize target state register by setting bus free phase
        // and asynchronous transfer mode
        //
        if (i != DeviceExtension->AdapterBusId) {
        
          ((PPCI_TARGET_STATE)(&regData))->SCSIPhase = TARGET_IDLE;
          ((PPCI_TARGET_STATE)(&regData))->SyncOffset = 0;
          ((PPCI_TARGET_STATE)(&regData))->SyncPeriod = 0;
          ((PPCI_TARGET_STATE)(&regData))->FastScsi = 0;
          
        } else {

          ((PPCI_HOST_STATE)(&regData))->BusReset = 1;

        }

        //
        // Write updated information back to the register
        //
        AMDAccessPCIRegister (DeviceExtension, PCIRegister, OneWord, &regData, WritePCI);

      }
    }
}

#endif

#ifndef OTHER_OS


LOCAL
VOID
AMDParseFlagString (
                   IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
                   IN PUCHAR InputString
                   )

/*++

Routine Description:

    This routine checks input string for driver flags.  The flag
    information is saved inside DeviceExtension data structure.

Arguments:

    DeviceExtension - HBA miniport driver's adapter data storage.
    InputString - Input string.

Return Value:

    None

--*/

{
    PUCHAR sPtr = InputString;
    UCHAR sBuffer[10];
    UCHAR i;

    while (*sPtr) {

      //
      // Search for an option sign
      //
      while (*sPtr != '/') {

        //
        // If reach to the string end, done
        //
        if (!*sPtr)
          return;

        //
        // Else check next element
        //
        sPtr++;

      }

      sPtr++;

      //
      // Skip any space
      //
      while (*sPtr == ' ' || *sPtr == '	') {

        //
        // If reach to the string end, done
        //
        if (!*sPtr)
          return;

        //
        // Skip space
        //
        sPtr++;

      }

      //
      // Copy string to string buffer
      //
      for (i = 0; i < 9; i++) {

        if (*sPtr == ' ' || *sPtr == '	' || *sPtr == 0 || *sPtr == '/') {

          sBuffer[i] = 0;
          break;

        } else {

          sBuffer[i] = *sPtr++;

        }
      }

      //
      // Terminate string anyway
      //
      sBuffer[9] = 0;

      //
      // Check for disable parity flag
      //
      if (AMDStringCompare (sBuffer, "PAR-")) {

        DeviceExtension->SysFlags.EnablePrity = 0;

      //
      // Check for disable synchonous transfer flag
      //
      } else if (AMDStringCompare (sBuffer, "S-")) {

        DeviceExtension->SysFlags.EnableSync = 0;

      //
      // Check for tagged queuing flag
      //
      } else if (AMDStringCompare (sBuffer, "TQ-")) {

        DeviceExtension->SysFlags.EnableTQ = 0;

      }
    }
}

LOCAL
BOOLEAN
AMDStringCompare (
                 IN PUCHAR sPtr1, 
                 IN PUCHAR sPtr2
                 )

/*++

Routine Description:

    This routine compares two input strings without checking
    character case.  The characters in the strings must not
    be special characters and must not over 128 in value.

Arguments:

    sPtr1 - input string 1.
    sPtr2 - input string 2.

Return Value:

    TRUE if two strings are matched

--*/

{
    UCHAR ch1, ch2;

    while (*sPtr1 && *sPtr2) {

      //
      // Read character
      //
      ch1 = *sPtr1;
      ch2 = *sPtr2;

      //
      // Convert character to up case
      //
      if (ch1 >= 'a' && ch1 <= 'z')
        ch1 -= 0x20;
      if (ch2 >= 'a' && ch2 <= 'z')
        ch2 -= 0x20;

      //
      // Compare two characters
      //
      if (ch1 != ch2)
        return FALSE;

      //
      // Check next character
      //
      sPtr1++;
      sPtr2++;

    }

    //
    // If both string reach to ends, they are matched.
    //
    if (!*sPtr1 && !*sPtr2) {

      return TRUE;

    } else {

      return FALSE;

    }
}

#endif


LOCAL
BOOLEAN
AMDCPUModeSwitchHandler (
                 IN PVOID DeviceExtension,
                 IN PVOID Context,
                 IN BOOLEAN SaveState
                 )
/*++

Routine Description:

    This routine is added for Chicago for real / protect mode switch.
    It doesn't hurt to put few bytes here to keep code identical for
    different OSs.

Arguments:

    ServiceContext - Supplies a pointer to the device extension.

    Context - INIT_DATA structure passed by DriverEntry.

    SaveState - Save / Restore flag

Return Value:

    None.

--*/

{

   return TRUE;

}


