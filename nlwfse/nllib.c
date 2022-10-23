
/*++

Copyright (c) 2016  Nextlabs.

Module Name:

    nllib.c

*/



#include <initguid.h>
#include <ntifs.h>
#include <ntstrsafe.h>
#include <fltKernel.h>
#include <wdm.h>
#include <dontuse.h>
#include <suppress.h>
#include "nlwfse.h"
#include "nllib.h"
#include "avlTree.h"

extern NLWFSE_DATA wfse;

int debugCacheTimeout;
int debugEcp;
extern int debugCacheReset;
extern int debugCache;
extern int debugCtx;
extern int debugPool;

__drv_maxIRQL(APC_LEVEL)
__checkReturn
BOOLEAN wfseGetFileRequestInfo(
    __in PFLT_CALLBACK_DATA Data,
    __deref_out_opt Pwfse_REQUEST_INFO* reqInfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN actoken = NULL;
    PTOKEN_USER userToken = NULL;
    PSRV_OPEN_ECP_CONTEXT srvEcp = NULL;
    PECP_LIST ecplist = NULL;
    PVOID ecpCtx = NULL;
    Pwfse_REQUEST_INFO info = NULL;
    PSOCKADDR_STORAGE_NFS clientAddr;
    PSOCKADDR_IN ipv4 = NULL;
    ULONG addrBufLength = 0;
    LONG addrStatus;
    PVOID ecpCtxPtr[EcpNum];
    ULONG ecpCtxSize = 0;
    ULONG ecpFlag;
    UCHAR offset = 0;
    GUID ecpGuid = {0};
    BOOLEAN ret = FALSE;
    BOOLEAN kernelMode = FALSE;

    ASSERT( Data != NULL );
    ASSERT( reqInfo != NULL );

    status = FltGetEcpListFromCallbackData(wfse.FilterHandle,Data,&ecplist);
    if( !NT_SUCCESS(status) || ecplist == NULL ) {
	ret = FALSE;
	goto funcExit;
    }

    info = ExAllocateFromPagedLookasideList(&wfse.ReqInfoBuffer);

    if (info == NULL ) {
            ret = FALSE;
            goto funcExit;
    }
    RtlZeroMemory(info, WFSE_REQINFO_BUFFER_SIZE);
    if (debugPool) {
        DbgPrint("PreCreate: Allocate pool Info %p \n", info);
    }
        
    while (NT_SUCCESS(status)) {
	status = FltGetNextExtraCreateParameter(wfse.FilterHandle,
                                             ecplist,
                                             ecpCtx,
                                             (LPGUID)&ecpGuid,
		                             &ecpCtx,
                                             &ecpCtxSize);
	    if( status == STATUS_NOT_FOUND ) {
                break;
	    } else if (status == STATUS_INVALID_PARAMETER) {
                if (debugEcp) {
	            DbgPrint("getFileReq: failed to find ECP param (0x%x)\n",status);
                }
                break;
            }
            ecpFlag = 0;
            if (IsEqualGUID(&GUID_ECP_OPLOCK_KEY, &ecpGuid)) {
                ecpFlag = ECP_TYPE_FLAG_OPLOCK_KEY;
                offset = EcpOplockKey;
                if (debugEcp) {
                    DbgPrint("GetReqInfo: oplock attached to the file open request \n");
                }
            } else if (IsEqualGUID(&GUID_ECP_SRV_OPEN, &ecpGuid)) {
                ecpFlag = ECP_TYPE_FLAG_SRV;
                offset = EcpSrvOpen;
                //srvEcp = (PSRV_OPEN_ECP_CONTEXT)ecpCtxPtr[EcpSrvOpen];
                if (debugEcp) {
                    DbgPrint("GetReqInfo: srv open file request \n");
                }
            } else if (IsEqualGUID(&GUID_ECP_NETWORK_OPEN_CONTEXT, &ecpGuid)) {
                ecpFlag = ECP_TYPE_FLAG_NETWORK_OPEN_KEY;
                offset = EcpNetworkOpen;
                if (debugEcp) {
                    DbgPrint("GetReqInfo: network open file request \n");
                }
            } else if (IsEqualGUID(&GUID_ECP_PREFETCH_OPEN, &ecpGuid)) {
                ecpFlag = ECP_TYPE_FLAG_PREFETCH_KEY;
                offset = EcpPrefetch;
                if (debugEcp) {
                    DbgPrint("GetReqInfo: prefetch open file request \n");
                }
            } else if (IsEqualGUID(&GUID_ECP_IO_DEVICE_HINT, &ecpGuid)) {
                ecpFlag = ECP_TYPE_FLAG_IO_DEVICE_HINT_KEY;
                offset = EcpDeviceHint;
                if (debugEcp) {
                    DbgPrint("GetReqInfo: device hint request \n");
                }
            }
            if ((ecpFlag != 0) && !FltIsEcpFromUserMode(wfse.FilterHandle,
                                                        ecpCtx)) {
                info->ecpMask |= ecpFlag;
                ecpCtxPtr[offset] = ecpCtx; 
                kernelMode = TRUE;
                if (debugEcp) {
                    DbgPrint("GetReqInfo: srv open file request from Kernel \n");
                }
            }
            else if (ecpFlag != 0) {
              if (debugEcp) {
                  DbgPrint("GetReqInfo: srv open file request from User mode \n");
              }
              break;
            }
            info->ecpCnt++;
    }

    if ((info->ecpCnt > 0) && (kernelMode == TRUE)) {
        if (debugEcp) {
            DbgPrint("GetReqInfo: srv open ecpCnt %d \n", info->ecpCnt);
        }
        if (FlagOn(info->ecpMask, ECP_TYPE_FLAG_SRV)) {
	    srvEcp = (PSRV_OPEN_ECP_CONTEXT) ecpCtxPtr[EcpSrvOpen];
	    FLT_ASSERT(srvEcp != NULL);
	    clientAddr = srvEcp->SocketAddress;
	    if (clientAddr->ss_family == AF_INET) {
                UCHAR ip[ADDRESS_STRING_LENGTH];
	        ipv4 = (PSOCKADDR_IN)clientAddr;
	        addrBufLength = ADDRESS_STRING_LENGTH;
          
	        addrStatus = RtlIpv4AddressToStringEx(&ipv4->sin_addr,
						  ipv4->sin_port,
						  (PSTR )ip,
						  &addrBufLength);  

                info->ipaddr = (ULONG)ipv4->sin_addr.S_un.S_addr;
                if (addrStatus == STATUS_INVALID_PARAMETER) {
                   ret = FALSE;
                   goto funcExit;
                }
                if (debugEcp) {
                    DbgPrint("getReqInfo: ip address %s \n", ip);
                }
	    }
        }
	actoken = SeQuerySubjectContextToken(&Data->Iopb->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);
	if( actoken == NULL )
	{
		DbgPrint("getReqInfo: failed get context token (0x%x)\n",status);
		ret = FALSE;
		goto funcExit;
	}
	status = SeQueryInformationToken(actoken,TokenUser,&userToken);
	if( !NT_SUCCESS(status) )
	{
		DbgPrint("getReqInfo: failed to get token information (0x%x)\n",status);
		ret = FALSE;
		goto funcExit;
	}

	info->pid = FltGetRequestorProcessId(Data);
        info->sid = ExAllocateFromPagedLookasideList(&wfse.StringPtr);
        if (info->sid == NULL) {
	    DbgPrint("getReqInfo: memory allocation failed \n");
            ret = FALSE;
            goto funcExit;
        }

        if (debugPool) {
	    DbgPrint("getReqInfo: memory allocation sid %p \n", info->sid);
        }
	status = RtlConvertSidToUnicodeString(info->sid, userToken->User.Sid,TRUE);
	if( !NT_SUCCESS(status) ) {
		ret = FALSE;
		goto funcExit;
	}

	ASSERT( info->sid->Buffer != NULL );
	ExFreePool(userToken);

	ASSERT( info != NULL );
        *reqInfo = info;
	ret = TRUE;
    } else {
            ret = FALSE;
    }

funcExit:

    if ( FALSE == ret ) {
	    if( info != NULL ) {
                if (info->sid != NULL) {
	            if( info->sid->Buffer != NULL ) {
	                RtlFreeUnicodeString(info->sid);
		    }
                    if (debugPool) {
	                DbgPrint("getReqInfo: free memory sid %p \n", info->sid);
                    }
                    ExFreeToPagedLookasideList(&wfse.StringPtr, info->sid);
                }
                if (debugPool) {
	            DbgPrint("getReqInfo: free memory info %p \n", info);
                }
                ExFreeToPagedLookasideList(&wfse.ReqInfoBuffer, info);
                info = NULL;
                *reqInfo = info;
           }
    }

    return ret;
}

#if 0
RTL_GENERIC_COMPARE_RESULTS
wfseCompareEntry (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Lhs,
    _In_ PVOID Rhs
    )
{
    Pwfse_QUERY_CACHE lhs = (Pwfse_QUERY_CACHE)Lhs;
    Pwfse_QUERY_CACHE rhs = (Pwfse_QUERY_CACHE)Rhs;

    UNREFERENCED_PARAMETER (Table);

      if ((lhs->key == rhs->key)) {
          return GenericEqual;
      } else if ((lhs->key < rhs->key)) {

        return GenericLessThan;

      } else if ((lhs->key > rhs->key)) {

        return GenericGreaterThan;
      }
    
    return GenericLessThan;

}

PVOID
NTAPI
wfseAllocateGenericTableEntry (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ CLONG ByteSize
    )
{

    UNREFERENCED_PARAMETER (Table);

    return ExAllocatePoolWithTag(PagedPool, ByteSize, wfse_QUERY_CACHE_TAG);
}

VOID
NTAPI
wfseFreeGenericTableEntry (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Entry
    )
{

    UNREFERENCED_PARAMETER (Table);

    if (debugCache) {
        DbgPrint("FreeGenericTable: free qcache entry \n");
    }
    ExFreePoolWithTag( Entry, wfse_QUERY_CACHE_TAG );
}
#endif

BOOLEAN wfseCacheUpdate(_In_ PFLT_CALLBACK_DATA Data,
                     _In_ ULONG allowedAccess,
                     _In_ PVOID CompletionContext)
{
    NTSTATUS status;
    Pwfse_STREAMHANDLE_CONTEXT streamContext = NULL;
    Pwfse_INSTANCE_CONTEXT instanceContext;
    Pwfse_AVL_TREE avl;
    wfse_QUERY_CACHE item = {0};
    LARGE_INTEGER curSystemTime;
    PPAGED_LOOKASIDE_LIST Lookaside;
    ULONG idx;

    streamContext = (Pwfse_STREAMHANDLE_CONTEXT)CompletionContext;

    status = FltGetInstanceContext(Data->Iopb->TargetInstance, &instanceContext);
    if (!NT_SUCCESS( status )){
        DbgPrint("wfseCacheUpdate: failed to get instance context.\n");
        Data->IoStatus.Status = status;
        Data->IoStatus.Information = 0;
        return FALSE;
    }

    idx = streamContext->hashidx;
    Lookaside = &instanceContext->hash[idx]->LookasideField;
    avl = instanceContext->hash[idx];
    avl->TreeIdx = idx;

    if (debugCtx) {
        DbgPrint("wfseCacheUpdate: instance ctx %x avl %p \n", instanceContext, avl);
    }

    KeQuerySystemTime(&curSystemTime);
    item.reqTime = curSystemTime;
    item.key = streamContext->key;
    item.ipaddr = streamContext->ipaddr;
    item.allowedAccess = allowedAccess;
    item.denyRename = streamContext->denyRename;

    if (instanceContext->hash[idx]->cacheCnt < WFSE_QUERY_CACHE_SIZE) {
        if (instanceContext->hash[idx]->cacheCnt == 0) {
	    if (debugCache)
                DbgPrint("UpdateCache: cache is empty \n");
	    instanceContext->hash[idx]->StartTime = curSystemTime;
	}
        //TODO: do search first
        wfseAvlInsert(avl, &item, Lookaside, Data->Iopb->TargetInstance);
        if (debugCache) {
            DbgPrint("wfseCacheUpdate: insert cache key %x allowedAccess %x time %x  cnt %d \n", item.key, item.allowedAccess, item.reqTime, instanceContext->hash[idx]->cacheCnt);
        }


    } else {
        TIME_FIELDS stime, ctime; 
        BOOLEAN inserted = FALSE;

        RtlTimeToTimeFields(&curSystemTime, &ctime);
        RtlTimeToTimeFields(&instanceContext->hash[idx]->StartTime, &stime);
	if (stime.Hour == ctime.Hour) {
	    if (ctime.Minute - stime.Minute <= WFSE_QUERY_CACHE_TIMEOUT_INMIN) {
		DbgPrint("wfseCacheUpdate: No timeout entry in cache... \n");
                FltReleaseContext(instanceContext);
	        return TRUE;
	    }
        }	    
        inserted = wfseAvlTimeoutNode(avl, &item, Lookaside, Data->Iopb->TargetInstance);
        if (inserted) {
            if (debugCache) {
                DbgPrint("wfseCacheUpdate: timeout a cache key %x allowedAccess %x time %x  cnt %d \n", item.key, item.allowedAccess, item.reqTime, instanceContext->hash[idx]->cacheCnt);
            }
        }
            
    }
    FltReleaseContext(instanceContext);
    return TRUE;
}

LONG
wfseExceptionFilter (
    _In_ PEXCEPTION_POINTERS ExceptionPointer,
    _In_ BOOLEAN AccessingUserBuffer
    )
/*++

Routine Description:

    Exception filter to catch errors touching user buffers.

Arguments:

    ExceptionPointer - The exception record.

    AccessingUserBuffer - If TRUE, overrides FsRtlIsNtStatusExpected to allow
                          the caller to munge the error to a desired status.

Return Value:

    EXCEPTION_EXECUTE_HANDLER - If the exception handler should be run.

    EXCEPTION_CONTINUE_SEARCH - If a higher exception handler should take care of
                                this exception.

--*/
{
    NTSTATUS Status;

    Status = ExceptionPointer->ExceptionRecord->ExceptionCode;

    //
    //  Certain exceptions shouldn't be dismissed within the filter
    //  unless we're touching user memory.
    //

    if (!FsRtlIsNtstatusExpected( Status ) &&
        !AccessingUserBuffer) {

        return EXCEPTION_CONTINUE_SEARCH;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

BOOLEAN
IsMyPc(
    __in PFLT_CALLBACK_DATA Data
)
{
    ULONG pcid;
    
    pcid = FltGetRequestorProcessId(Data);
   
    if (wfse.pcid == pcid) {
        return TRUE;
    } else {
        return FALSE;
    }

}

VOID
wfseResetCache (
    __in PVOID Context
)
{
    UNREFERENCED_PARAMETER(Context);
    Pwfse_AVL_TREE avl;
    int idx;

    for(;;) {
        DbgPrint("wfseResetCache: waiting to reset cache... \n");

	KeWaitForSingleObject(&wfse.FlushCacheEvent,
			      Executive, 
			      KernelMode, 
			      FALSE, 
                              NULL);

	if (wfse.Unloading) {
	    break;
	}

        for (idx = 0; idx < WFSE_MAX_VOLUMES; idx++) {
            if (wfse.Gv[idx].Instance != NULL) {
	        for (int i = 0; i < WFSE_HASH_TABLE_SIZE; i++) {
	            avl = wfse.Gv[idx].avl[i];
                    if (avl && avl->cacheCnt > 0) {
                        wfseAcquireResourceExclusive(avl->Resource);

                        if (debugCacheReset) {
                            DbgPrint("wfseResetCache: free node in avl %p cachecnt %d \n", avl, avl->cacheCnt); 
                        }

                        wfseAvlFreeNode(avl->fileQueryCache,
                                      &avl->LookasideField); 

                        avl->cacheCnt = 0;   //single tree cache count in instance
                        avl->fileQueryCache = NULL;
                        wfseReleaseResource(avl->Resource);

                       if (debugCacheReset) {
                               DbgPrint("wfseResetCache: instance %p avl %p cachecnt %d \n", wfse.Gv[idx].Instance, avl, avl->cacheCnt); 
                       }
                    }
	        }
                wfse.Gv[idx].cacheCnt = 0; // instance volume cache cnt
            }
        } 
        wfse.MaxCacheCnt = 0;
        wfse.CacheStale = FALSE;
        KeClearEvent(&wfse.FlushCacheEvent);

    }
    DbgPrint("wfseResetCache: Flush cache thread exiting...  \n");
    PsTerminateSystemThread(STATUS_SUCCESS);
}

LONGLONG GetTimeCount()
{
  LARGE_INTEGER tick_count;
  ULONG inc = 0;
  inc = KeQueryTimeIncrement();
  KeQueryTickCount(&tick_count);
  tick_count.QuadPart *= inc;
  tick_count.QuadPart /= 10000;
  return tick_count.QuadPart;
}
