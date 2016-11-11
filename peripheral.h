/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    peripheral.h

Abstract:

    This module contains the function definitions for 
    interaction with the SPB API.

Environment:

    kernel-mode only

Revision History:

--*/

#ifndef _PERIPHERAL_H_
#define _PERIPHERAL_H_

EVT_WDF_REQUEST_COMPLETION_ROUTINE SpbPeripheralOnCompletion;
EVT_WDF_REQUEST_CANCEL             SpbPeripheralOnCancel;

EVT_WDF_REQUEST_CANCEL             SpbPeripheralOnWaitOnInterruptCancel;

NTSTATUS
SpbPeripheralOpen(
    _In_  PPBC_DEVICE  pDevice);

NTSTATUS
SpbPeripheralClose(
    _In_  PPBC_DEVICE  pDevice);

VOID
SpbPeripheralLock(
    _In_  PPBC_DEVICE       pDevice,
	_In_   SPBREQUEST       spbRequest);

VOID
SpbPeripheralUnlock(
    _In_  PPBC_DEVICE       pDevice,
	_In_   SPBREQUEST       spbRequest);

VOID
SpbPeripheralLockConnection(
    _In_  PPBC_DEVICE       pDevice,
	_In_   SPBREQUEST       spbRequest);

VOID
SpbPeripheralUnlockConnection(
    _In_  PPBC_DEVICE       pDevice,
	_In_   SPBREQUEST       spbRequest);

VOID
SpbPeripheralRead(
    _In_  PPBC_DEVICE       pDevice,
    _In_   SPBREQUEST       spbRequest,
	_In_  BOOLEAN           FullDuplex);

VOID
SpbPeripheralWrite(
    _In_  PPBC_DEVICE       pDevice,
    _In_   SPBREQUEST       spbRequest,
	_In_  BOOLEAN           FullDuplex);

VOID
SpbPeripheralWriteRead(
    _In_  PPBC_DEVICE       pDevice,
    _In_   SPBREQUEST       spbRequest);

VOID
SpbPeripheralFullDuplex(
    _In_  PPBC_DEVICE       pDevice,
    _In_  SPBREQUEST        spbRequest);

NTSTATUS
SpbPeripheralSendRequest(
    _In_  PPBC_DEVICE       pDevice,
    _In_  WDFREQUEST        SpbRequest,
    _In_  WDFREQUEST        ClientRequest);

VOID
SpbPeripheralCompleteRequestPair(
    _In_  PPBC_DEVICE        pDevice,
    _In_  NTSTATUS          status,
    _In_  ULONG_PTR         bytesCompleted);

NTSTATUS
FORCEINLINE
RequestGetByte(
	_In_  PMDL          mdl,
	_In_  size_t        mdlLength,
	_In_  size_t        Index,
	_Out_ UCHAR*        pByte
)
/*++

Routine Description:

This is a helper routine used to retrieve the
specified byte of the current transfer descriptor buffer.

Arguments:

pRequest - a pointer to the PBC request context

Index - index of desired byte in current transfer descriptor buffer

pByte - pointer to the location for the specified byte

Return Value:

STATUS_INFO_LENGTH_MISMATCH if invalid index,
otherwise STATUS_SUCCESS

--*/
{
	size_t mdlByteCount;
	size_t currentOffset = Index;
	PUCHAR pBuffer;
	NTSTATUS status = STATUS_INFO_LENGTH_MISMATCH;

	//
	// Check for out-of-bounds index
	//

	if (Index < mdlLength)
	{
		while (mdl != NULL)
		{
			mdlByteCount = MmGetMdlByteCount(mdl);

			if (currentOffset < mdlByteCount)
			{
				pBuffer = (PUCHAR)MmGetSystemAddressForMdlSafe(
					mdl,
					NormalPagePriority | MdlMappingNoExecute);

				if (pBuffer != NULL)
				{
					//
					// Byte found, mark successful
					//

					*pByte = pBuffer[currentOffset];
					status = STATUS_SUCCESS;
				}

				break;
			}

			currentOffset -= mdlByteCount;
			mdl = mdl->Next;
		}

		//
		// If after walking the MDL the byte hasn't been found,
		// status will still be STATUS_INFO_LENGTH_MISMATCH
		//
	}

	return status;
}

#endif // _PERIPHERAL_H_
