
/*++

Copyright (c) 2016  Nextlabs.

Module Name:

    nlwfse.h

*/


#ifndef __NLWFSE_H__
#define __NLWFSE_H__

#define wfse_DEFAULT_TIME_DELAY            150
#define wfse_DEFAULT_MAPPING_PATH          L"\\"
#define wfse_KEY_NAME_DELAY                L"OperatingDelay"
#define wfse_KEY_NAME_PATH                 L"OperatingPath"
#define wfse_KEY_NAME_DEBUG_LEVEL          L"DebugLevel"
#define WFSE_MAX_VOLUMES 8
#define wfse_MAX_PATH_LENGTH               256
#define WFSE_HASH_TABLE_SIZE              193 
#define WFSE_STRING_PTR_SIZE              sizeof(UNICODE_STRING) 
#define WFSE_SID_BUFFER_SIZE              256 
#define WFSE_TMP_BUFFER_SIZE              12468 
#define WFSE_MAX_CACHE_SIZE               250000 

#define wfseDBG_TRACE_ERRORS              0x00000001
#define wfseDBG_TRACE_ROUTINES            0x00000002
#define wfseDBG_TRACE_OPERATION_STATUS    0x00000004
#define wfse_TRACE_ERROR                   0x00000008
#define wfse_TRACE_CBDQ_CALLBACK           0x00000010
#define wfse_TRACE_PRE_CREATE              0x00000020
#define wfse_TRACE_LOAD_UNLOAD             0x00000040
#define wfse_TRACE_INSTANCE_CALLBACK       0x00000080
#define wfse_TRACE_CONTEXT_CALLBACK        0x00000100

#define wfse_TRACE_ALL                     0xFFFFFFFF

#define wfse_VOLUME_GUID_NAME_SIZE        48
#define ADDRESS_STRING_LENGTH        64
#define MAX_DEVNAME_LENGTH        64
#define MAX_VOLUME_ID_SIZE       36


#define wfse_INSTANCE_CONTEXT_POOL_TAG    'nIfD'
#define wfse_STREAM_CTX_TAG      'xSfD'
#define wfse_ERESOURCE_POOL_TAG    'sRfD'
//#define wfse_NOTIFY_POOL_TAG              'wfse'
#define wfse_TBUF_TAG              'fubt'
#define wfse_SIDBUF_TAG            'fdis'
#define wfse_ACCESSBUF_TAG         'facc'
//#define INSTANCE_CONTEXT_TAG              'IqsC'
//#define QUEUE_CONTEXT_TAG                 'QqsC'
#define wfse_STRPTR_TAG            'SqsC'
#define wfse_AVL_TAG               'lvac'
#define wfse_REQINFOBUF_TAG        'fqer'
#define wfse_LOG_CONTEXT_TAG       'tclw'

#define wfse_CONTEXT_POOL_TYPE            PagedPool

#define wfse_NOTIFICATION_MASK            (TRANSACTION_NOTIFY_COMMIT_FINALIZE | \
                                         TRANSACTION_NOTIFY_ROLLBACK)

#define WFSE_IOCTL_VOLUME_BASE FILE_DEVICE_MASS_STORAGE
#define WFSE_IOCTL_VOLUME_INFO CTL_CODE(WFSE_IOCTL_VOLUME_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef unsigned long DWORD;
typedef unsigned char BYTE;

typedef enum _STORAGE_QUERY_TYPE {
  PropertyStandardQuery = 0,        
  PropertyExistsQuery,             
  PropertyMaskQuery,              
  PropertyQueryMaxDefined   
} STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;

typedef enum _STORAGE_PROPERTY_ID {
  StorageDeviceProperty = 0,
  StorageAdapterProperty,
  StorageDeviceIdProperty,
  StorageDeviceUniqueIdProperty,  
  StorageDeviceWriteCacheProperty,
  StorageMiniportProperty,
  StorageAccessAlignmentProperty,
  StorageDeviceSeekPenaltyProperty,
  StorageDeviceTrimProperty,
  StorageDeviceWriteAggregationProperty,
  StorageDeviceDeviceTelemetryProperty,
  StorageDeviceLBProvisioningProperty,
  StorageDevicePowerProperty,
  StorageDeviceCopyOffloadProperty,
  StorageDeviceResiliencyProperty,
  StorageDeviceMediumProductType,
  StorageDeviceRpmbProperty,
  StorageDeviceIoCapabilityProperty = 48,
  StorageAdapterProtocolSpecificProperty,
  StorageDeviceProtocolSpecificProperty,
  StorageAdapterTemperatureProperty,
  StorageDeviceTemperatureProperty,
  StorageAdapterPhysicalTopologyProperty,
  StorageDevicePhysicalTopologyProperty,
  StorageDeviceAttributesProperty,
} STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;

typedef struct _STORAGE_PROPERTY_QUERY {

  //
  // ID of the property being retrieved
  //

  STORAGE_PROPERTY_ID PropertyId;

  //
  // Flags indicating the type of query being performed
  //

  STORAGE_QUERY_TYPE QueryType;

  //
  // Space for additional parameters if necessary
  //

  BYTE  AdditionalParameters[1];

} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;


typedef enum _STORAGE_BUS_TYPE {
  BusTypeUnknown = 0x00,
  BusTypeScsi = 0x01,
  BusTypeAtapi = 0x02,
  BusTypeAta = 0x03,
  BusType1394 = 0x04,
  BusTypeSsa = 0x05,
  BusTypeFibre = 0x06,
  BusTypeUsb = 0x07,
  BusTypeRAID = 0x08,
  BusTypeiSCSI = 0x09,
  BusTypeSas = 0x0A,
  BusTypeSata = 0x0B,
  BusTypeMaxReserved = 0x7F
} STORAGE_BUS_TYPE, *PSTORAGE_BUS_TYPE;

typedef _Struct_size_bytes_(Size) struct _STORAGE_DEVICE_DESCRIPTOR {

  DWORD Version;
  DWORD Size;
  BYTE  DeviceType;
  BYTE  DeviceTypeModifier;
  BOOLEAN RemovableMedia;
  BOOLEAN CommandQueueing;
  DWORD VendorIdOffset;
  DWORD ProductIdOffset;
  DWORD ProductRevisionOffset;
  DWORD SerialNumberOffset;
  STORAGE_BUS_TYPE BusType;
  DWORD RawPropertiesLength;
  BYTE  RawDeviceProperties[1];
} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;

typedef struct _wfseAvlNode {
    struct _wfseAvlNode *parent;
    struct _wfseAvlNode *left;
    struct _wfseAvlNode *right;
    ULONG key;
    ULONG allowedAccess;
    BOOLEAN denyRename;
    LARGE_INTEGER reqTime;
    LARGE_INTEGER logTime;
    int balance;
} wfseAvlNode, *PwfseAvlNode;

typedef struct _WFSE_AVL_TREE {
  PFLT_INSTANCE Instance;
  PwfseAvlNode fileQueryCache; //avltree root
  ULONG TreeIdx;
  ULONG cacheHit;
  ULONG cacheMiss;
  PAGED_LOOKASIDE_LIST LookasideField;
  PERESOURCE Resource;
  __volatile ULONG cacheCnt;
  BOOLEAN CacheStale;
  BOOLEAN FlushInProgress;
  KEVENT FlushCompleted;
  __volatile ULONG CreateCnt;
  LARGE_INTEGER StartTime;
} wfse_AVL_TREE, *Pwfse_AVL_TREE;

typedef struct _NLWFSE_VOLUME {
    PFLT_INSTANCE Instance;
    //wfseAvlNode *fileQueryCache[WFSE_HASH_TABLE_SIZE];
    //Pwfse_AVL_TREE avl; 
    wfse_AVL_TREE *avl[WFSE_HASH_TABLE_SIZE];
    ULONG cacheCnt;
} NLWFSE_VOLUME, *PNLWFSE_VOLUME;

typedef struct _NLWFSE_DATA {
    PDRIVER_OBJECT DriverObject;
    PFLT_FILTER FilterHandle;
    PFLT_PORT ServerPort;
    PEPROCESS UserProcess;
    PFLT_PORT ClientPort;
    PAGED_LOOKASIDE_LIST TmpBuffer;
    PAGED_LOOKASIDE_LIST StringPtr;
    PAGED_LOOKASIDE_LIST SidBuffer;
    PAGED_LOOKASIDE_LIST AccessBuffer;
    PAGED_LOOKASIDE_LIST ReqInfoBuffer;
    PAGED_LOOKASIDE_LIST LogInfoBuffer;
    LIST_ENTRY pIoList;
    ERESOURCE pIoListLock;
    FAST_MUTEX FlushCacheMutex;
    FAST_MUTEX gLock;
    ULONG pcid;
    LONG GvCnt;
    LONG MaxCacheCnt;
    BOOLEAN ProtectionOn;
    BOOLEAN CacheStale;
    BOOLEAN Unloading;
    KEVENT FlushCacheEvent;
    PVOID FlushCacheThread;
    ULONG  DebugLevel;
    NLWFSE_VOLUME Gv[WFSE_MAX_VOLUMES];
} NLWFSE_DATA, *PNLWFSE_DATA;


#pragma warning(push)
#pragma warning(disable:4200) // disable warnings for structures with zero length arrays.

typedef struct _NLWFSE_CREATE_PARAMS {
    WCHAR String[0];
} NLWFSE_CREATE_PARAMS, *PNLWFSE_CREATE_PARAMS;

#pragma warning(pop)

//
//  This helps us deal with ReFS 128-bit file IDs and NTFS 64-bit file IDs.
//

typedef union _wfse_FILE_REFERENCE {
    struct {
        ULONGLONG   Value; 
        ULONGLONG   UpperZeroes;    //  This is zero in 64-bit fileID.
    } FileId64;
    UCHAR           FileId128[16];  //  The 128-bit file ID 
} wfse_FILE_REFERENCE, *Pwfse_FILE_REFERENCE;

#define wfseSizeofFileId(FID) (               \
    ((FID).FileId64.UpperZeroes == 0ll) ?   \
        sizeof((FID).FileId64.Value)    :   \
        sizeof((FID).FileId128)             \
    )

typedef struct _wfse_QUERY_CACHE {
#if 0
    ULONG fileHash;
    ULONG sidHash;
#endif
    ULONG key;
    ULONG ipaddr;
    ULONG allowedAccess;
    BOOLEAN denyRename;
    LARGE_INTEGER reqTime;
    LARGE_INTEGER logTime;
} wfse_QUERY_CACHE, *Pwfse_QUERY_CACHE;

#if 0
typedef struct _WFSE_AVL_TREE {
    PFLT_INSTANCE Instance;
    PwfseAvlNode fileQueryCache; //avltree root
    ULONG TreeIdx;
    ULONG cacheHit;
    ULONG cacheMiss;
    PAGED_LOOKASIDE_LIST LookasideField;
    PERESOURCE Resource;
    __volatile ULONG cacheCnt;
    BOOLEAN CacheStale;
    BOOLEAN FlushInProgress;
    KEVENT FlushCompleted;
    __volatile ULONG CreateCnt;
    LARGE_INTEGER StartTime;
} wfse_AVL_TREE, *Pwfse_AVL_TREE;
#endif

typedef struct _wfse_INSTANCE_CONTEXT {
    PFLT_INSTANCE Instance;
    PFLT_VOLUME Volume;
    FLT_FILESYSTEM_TYPE VolumeFsType;
    STORAGE_BUS_TYPE DevBusType;
    wfse_AVL_TREE *hash[WFSE_HASH_TABLE_SIZE];
    //ULONG cacheCnt;
} wfse_INSTANCE_CONTEXT, *Pwfse_INSTANCE_CONTEXT;

typedef struct _wfse_STREAMHANDLE_CONTEXT {
    PUNICODE_STRING fileName;
    PUNICODE_STRING sid;
    ULONG ipaddr;
    ULONG hashidx;
    //enum WFSE_FILE_ACCESS faction;
    ULONG faction;
    ULONG allowedAccess;
    ULONG desiredAccess;
    ULONG key;
#if 0
    ULONG fileHash; 
    ULONG sidHash; 
#endif
    BOOLEAN  denyRename;
    BOOLEAN  Officefile;
    volatile LONG NumOps;
    BOOLEAN  SetDisp;
} wfse_STREAMHANDLE_CONTEXT, *Pwfse_STREAMHANDLE_CONTEXT;

typedef struct _wfse_LOG_CONTEXT {
    PUNICODE_STRING fileName;
    PUNICODE_STRING sid;
    ULONG ipaddr;
    ULONG faction;
} wfse_LOG_CONTEXT, *Pwfse_LOG_CONTEXT; 

#define WFSE_LOG_CONTEXT_SIZE sizeof(wfse_LOG_CONTEXT)

typedef struct _QUEUE_CONTEXT {

    FLT_CALLBACK_DATA_QUEUE_IO_CONTEXT CbdqIoContext;

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

#define DebugTrace(Level, Data)               \
    if ((Level) & wfse.DebugLevel) {       \
        DbgPrint Data;                        \
    }

#define wfse_print( ... )                                                      \
    DbgPrintEx( DPFLTR_FLTMGR_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__ )

#define FlagOnAll( F, T )                                                    \
    (FlagOn( F, T ) == T)

#define Offset2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))

///////////////////////////////////////////////////////////////////////////
//
//  Prototypes for the startup and unload routines used for 
//  this Filter.
//
///////////////////////////////////////////////////////////////////////////
DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );
NTSTATUS
wfseUnload (
    IN FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
wfseInstanceSetup (
    IN PCFLT_RELATED_OBJECTS FltObjects,
    IN FLT_INSTANCE_SETUP_FLAGS Flags,
    IN DEVICE_TYPE VolumeDeviceType,
    IN FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

NTSTATUS
wfseInstanceQueryTeardown (
    IN PCFLT_RELATED_OBJECTS FltObjects,
    IN FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

VOID
wfseInstanceTeardownStart (
    IN PCFLT_RELATED_OBJECTS FltObjects,
    IN FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
wfseInstanceTeardownComplete (
    IN PCFLT_RELATED_OBJECTS FltObjects,
    IN FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
wfseSetupInstanceContext(
    IN PCFLT_RELATED_OBJECTS FltObjects
    );

NTSTATUS
wfseAllocateContext (
    IN FLT_CONTEXT_TYPE ContextType,
    _Outptr_ PFLT_CONTEXT *Context
    );

NTSTATUS
wfseSetContext (
    IN PFLT_INSTANCE Instance,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    IN FLT_CONTEXT_TYPE ContextType,
    IN PFLT_CONTEXT NewContext,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext
    );

NTSTATUS
wfseGetContext (
    IN PFLT_INSTANCE Instance,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    IN FLT_CONTEXT_TYPE ContextType,
    _Outptr_ PFLT_CONTEXT *Context
    );

NTSTATUS
wfseContextMgmt (
    IN PFLT_INSTANCE Instance,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    _Outptr_ _Pre_valid_ PFLT_CONTEXT *Context,
    IN FLT_CONTEXT_TYPE ContextType
    );

VOID
wfseStreamHandleContextCleanup(
    IN Pwfse_STREAMHANDLE_CONTEXT StreamContext,
    IN FLT_CONTEXT_TYPE ContextType
    );

VOID
wfseInstanceContextCleanup(
    IN Pwfse_INSTANCE_CONTEXT InstanceContext,
    IN FLT_CONTEXT_TYPE ContextType
    );

NTSTATUS
wfseSetConfiguration (
    _In_ PUNICODE_STRING RegistryPath
    );

VOID
wfseFreeList(
    );

FLT_PREOP_CALLBACK_STATUS
wfsePreCreate(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
wfsePostCreate(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

VOID wfseCreate( _In_ PFLT_DEFERRED_IO_WORKITEM workItem,
                  _In_ PFLT_CALLBACK_DATA Data,
                  _In_ PVOID CompletionContext);


VOID wfseQueryPolicy( _In_ PFLT_DEFERRED_IO_WORKITEM workItem,
                  _In_ PFLT_CALLBACK_DATA Data,
                  _In_ PVOID CompletionContext);

FLT_PREOP_CALLBACK_STATUS
wfsePreWrite (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
wfsePostWrite (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
wfsePreSetInfo(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
wfsePostSetInfo(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
wfsePreSetSecurity(
  __inout PFLT_CALLBACK_DATA Data,
  __in PCFLT_RELATED_OBJECTS FltObjects,
  _Outptr_result_maybenull_ PVOID *CompletionContext
  );

FLT_POSTOP_CALLBACK_STATUS
wfsePostSetSecurity(
  __inout PFLT_CALLBACK_DATA Data,
  __in PCFLT_RELATED_OBJECTS FltObjects,
  __in PVOID CompletionContext,
  __in FLT_POST_OPERATION_FLAGS Flags
  );

FLT_PREOP_CALLBACK_STATUS
wfsePreCleanup(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
wfsePostCleanup(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

NTSTATUS
wfseGetVolumeGuidName (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout PUNICODE_STRING VolumeGuidName
    );

NTSTATUS
wfsePortConnect (
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID *ConnectionCookie
    );

VOID
wfsePortDisconnect (
    _In_opt_ PVOID ConnectionCookie
    );

NTSTATUS
wfseCloseFile(
	__in PDEVICE_OBJECT DeviceObject,
	__in PFILE_OBJECT FileObject);

NTSTATUS 
wfseCloseFileIoCompletion(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp,
	__in PKEVENT SyncEvent);

NTSTATUS
wfseCleanupFile(
	__in PDEVICE_OBJECT DeviceObject,
	__in PFILE_OBJECT FileObject);

NTSTATUS 
wfseCleanupFileIoCompletion(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp,
	__in PKEVENT SyncEvent);

NTSTATUS wfseLogUpdate(Pwfse_LOG_CONTEXT sctx);

#endif /* __NLWFSE_H__ */

