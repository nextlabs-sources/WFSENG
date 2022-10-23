/*++

Copyright (c) 2016  Nextlabs.

Module Name:

    nllib.h

*/

#ifndef __NLLIB_H__
#define __NLLIB_H__

#include <wsk.h>

#define wfse_QUERY_CACHE_TAG              'tcqw'
#define WFSE_QUERY_CACHE_SIZE 50 
#define WFSE_QUERY_CACHE_TIMEOUT 5  //5 minutes for now
#define WFSE_QUERY_CACHE_TIMEOUT_INMIN 5 
#define ADDRESS_STRING_LENGTH  64
#define ECP_TYPE_FLAG_SRV 0x00000001
#define ECP_TYPE_FLAG_OPLOCK_KEY 0x00000002
#define ECP_TYPE_FLAG_NETWORK_OPEN_KEY 0x00000004
#define ECP_TYPE_FLAG_PREFETCH_KEY 0x00000008
#define ECP_TYPE_FLAG_IO_DEVICE_HINT_KEY 0x00000010

typedef enum _ECP_TYPE {
    EcpOplockKey,
    EcpSrvOpen,
    EcpNetworkOpen,
    EcpPrefetch,
    EcpDeviceHint,
    EcpNum
} ECP_TYPE;

typedef struct _wfse_REQUEST_INFO {
    ULONG pid;
    PUNICODE_STRING cifsShareName;
    PUNICODE_STRING sid;
    //UCHAR ip[ADDRESS_STRING_LENGTH];
    ULONG ipaddr;
    ULONG ecpMask;
    ULONG ecpCnt;
} wfse_REQUEST_INFO, *Pwfse_REQUEST_INFO;

#define WFSE_REQINFO_BUFFER_SIZE sizeof(wfse_REQUEST_INFO)

__drv_maxIRQL(APC_LEVEL)
__checkReturn
BOOLEAN wfseGetFileRequestInfo(
  __in PFLT_CALLBACK_DATA Data,
  __deref_out_opt Pwfse_REQUEST_INFO* reqInfo);

PVOID
wfseLookasideListAllocateEx(
    __in POOL_TYPE PoolType,
    __in SIZE_T  NumberOfBytes,
    __in ULONG  Tag,
    __inout PLOOKASIDE_LIST_EX  Lookaside);

VOID
wfseLookasideListFreeEx(
    __in PVOID entry,
    __inout PLOOKASIDE_LIST_EX Lookaside);

BOOLEAN
IsMyPc(
    __in PFLT_CALLBACK_DATA Data
);

BOOLEAN wfseCacheUpdate(_In_ PFLT_CALLBACK_DATA Data,
                        _In_ ULONG allowedAccess,
                        _In_ PVOID CompletionContext);

VOID
wfseResetCache(
    __in PVOID Context
);

LONGLONG GetTimeCount();

#if 0
BOOLEAN wfseCacheDelete(_In_ PFLT_CALLBACK_DATA Data,
                        _In_ PVOID streamContext);
#endif

FORCEINLINE
VOID
_Acquires_lock_(_Global_critical_region_)
wfseAcquireResourceExclusive (
    _Inout_ _Acquires_exclusive_lock_(*Resource) PERESOURCE Resource
    )
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    FLT_ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) ||
               !ExIsResourceAcquiredSharedLite(Resource));

    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceExclusiveLite( Resource, TRUE );
}

FORCEINLINE
VOID
_Releases_lock_(_Global_critical_region_)
_Requires_lock_held_(_Global_critical_region_)
wfseReleaseResource(
    _Inout_ _Requires_lock_held_(*Resource) _Releases_lock_(*Resource) PERESOURCE Resource
    )
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    FLT_ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) ||
               ExIsResourceAcquiredSharedLite(Resource));

    ExReleaseResourceLite(Resource);
    KeLeaveCriticalRegion();
}

FORCEINLINE
VOID
_Acquires_lock_(_Global_critical_region_)
wfseAcquireResourceShared (
    _Inout_ _Acquires_shared_lock_(*Resource) PERESOURCE Resource
    )
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceSharedLite( Resource, TRUE );
}

#endif  //__NLLIB_H__
