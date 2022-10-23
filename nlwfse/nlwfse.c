
/*++

Copyright (c) 2016  Nextlabs.

Module Name:

    nlwfse.c

*/



#include <initguid.h>
#include <ntifs.h>
#include <ntdef.h>
#include <fltKernel.h>
#include <wdm.h>
#include <wsk.h>
#include <dontuse.h>
#include <suppress.h>
#include "nlwfsecommon.h"
#include "nlwfse.h"
#include "nllib.h"
#include "avlTree.h"

#define NL_CHANGE_SECURITY_ATTRIBUTE 0x60080
//#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

//////////////////////////////////////////////////////////////////////////////
//  Text section assignments for all routines                               //
//////////////////////////////////////////////////////////////////////////////


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, wfseSetConfiguration)
#pragma alloc_text(PAGE, wfseUnload)
#pragma alloc_text(PAGE, wfseInstanceSetup)
#pragma alloc_text(PAGE, wfseInstanceQueryTeardown)
#pragma alloc_text(PAGE, wfseInstanceTeardownStart)
#pragma alloc_text(PAGE, wfseInstanceTeardownComplete)
#pragma alloc_text(PAGE, wfseSetupInstanceContext)
#pragma alloc_text(PAGE, wfseAllocateContext)
#pragma alloc_text(PAGE, wfseSetContext)
#pragma alloc_text(PAGE, wfseGetContext)
#pragma alloc_text(PAGE, wfseStreamHandleContextCleanup)
#pragma alloc_text(PAGE, wfseInstanceContextCleanup)
#pragma alloc_text(PAGE, wfsePreCreate)
#pragma alloc_text(PAGE, wfsePostCreate)
#pragma alloc_text(PAGE, wfsePreWrite)
#pragma alloc_text(PAGE, wfsePostWrite)
#pragma alloc_text(PAGE, wfsePreSetInfo)
#pragma alloc_text(PAGE, wfsePostSetInfo)
#pragma alloc_text(PAGE, wfsePreCleanup)
#pragma alloc_text(PAGE, wfsePostCleanup)
#pragma alloc_text(PAGE, wfsePortConnect)
#pragma alloc_text(PAGE, wfsePortDisconnect)
#pragma alloc_text(PAGE, wfsePreSetSecurity)
#pragma alloc_text(PAGE, wfsePostSetSecurity)
#pragma alloc_text(PAGE, wfseFreeList)
#endif


//////////////////////////////////////////////////////////////////////////////
//  Context Registration                                                    //
//////////////////////////////////////////////////////////////////////////////

CONST FLT_CONTEXT_REGISTRATION Contexts[] = {

    { FLT_INSTANCE_CONTEXT,
      0,
      wfseInstanceContextCleanup,
      sizeof(wfse_INSTANCE_CONTEXT),
      wfse_INSTANCE_CONTEXT_POOL_TAG,
      NULL,
      NULL,
      NULL },

    { FLT_STREAMHANDLE_CONTEXT,
      0,
      wfseStreamHandleContextCleanup,
      sizeof(wfse_STREAMHANDLE_CONTEXT),
      wfse_STREAM_CTX_TAG,
      NULL,
      NULL,
      NULL },

    { FLT_CONTEXT_END }

};

//////////////////////////////////////////////////////////////////////////////
//  Operation Registration                                                  //
//////////////////////////////////////////////////////////////////////////////

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
      0,
      wfsePreCreate,
      wfsePostCreate},

    { IRP_MJ_WRITE,
      0,
      wfsePreWrite,
      wfsePostWrite},

    { IRP_MJ_SET_SECURITY,
      0,
      wfsePreSetSecurity,
      wfsePostSetSecurity },

    { IRP_MJ_SET_INFORMATION,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      wfsePreSetInfo,
      wfsePostSetInfo},

    { IRP_MJ_CLEANUP,
      0,
      wfsePreCleanup,
      wfsePostCleanup},

    { IRP_MJ_OPERATION_END }

};


//////////////////////////////////////////////////////////////////////////////
//  Filter Registration                                                     //
//////////////////////////////////////////////////////////////////////////////

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    Contexts,                           //  Context
    Callbacks,                          //  Operation callbacks

    wfseUnload,                           //  MiniFilterUnload

    wfseInstanceSetup,                    //  InstanceSetup
    wfseInstanceQueryTeardown,            //  InstanceQueryTeardown
    wfseInstanceTeardownStart,            //  InstanceTeardownStart
    wfseInstanceTeardownComplete,         //  InstanceTeardownComplete
    NULL,                               //  GenerateFileName
    NULL,                               //  NormalizeNameComponent
    NULL,                               //  NormalizeContextCleanup
    NULL,                               //  TransactionNotification
    NULL                                //  NormalizeNameComponentEx

};


NLWFSE_DATA wfse;
int debugCacheReset;
int debugPreCreate;
int debugCache;
int debugPreSetInfo;
int debugPool;
int debugCtx;
int debugDel;
int debugLog;
int passthru;
int debugHash;

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    UNICODE_STRING tmpstr;
    OBJECT_ATTRIBUTES objattr;
    PSECURITY_DESCRIPTOR sd;
    HANDLE threadHandle;

    //UNREFERENCED_PARAMETER( RegistryPath );
    
    //DbgBreakPoint();
    ExInitializeDriverRuntime( DrvRtPoolNxOptIn );
    ExInitializePagedLookasideList(&wfse.TmpBuffer,
				   NULL,
				   NULL,
				   POOL_RAISE_IF_ALLOCATION_FAILURE,
                                   WFSE_TMP_BUFFER_SIZE,
				   wfse_TBUF_TAG,
				   0);

    ExInitializePagedLookasideList(&wfse.StringPtr,
				   NULL,
				   NULL,
				   POOL_RAISE_IF_ALLOCATION_FAILURE,
                                   WFSE_STRING_PTR_SIZE,
				   wfse_STRPTR_TAG,
				   0);

    ExInitializePagedLookasideList(&wfse.SidBuffer,
				   NULL,
				   NULL,
				   POOL_RAISE_IF_ALLOCATION_FAILURE,
                                   WFSE_SID_BUFFER_SIZE,
				   wfse_SIDBUF_TAG,
				   0);

    ExInitializePagedLookasideList(&wfse.AccessBuffer,
				   NULL,
				   NULL,
				   POOL_RAISE_IF_ALLOCATION_FAILURE,
                                   WFSE_ACCESSINFO_BUFFER_SIZE,
				   wfse_ACCESSBUF_TAG,
				   0);

    ExInitializePagedLookasideList(&wfse.ReqInfoBuffer,
				   NULL,
				   NULL,
				   POOL_RAISE_IF_ALLOCATION_FAILURE,
                                   WFSE_REQINFO_BUFFER_SIZE,
				   wfse_REQINFOBUF_TAG,
				   0);

    ExInitializePagedLookasideList(&wfse.LogInfoBuffer,
				   NULL,
				   NULL,
				   POOL_RAISE_IF_ALLOCATION_FAILURE,
                                   WFSE_LOG_CONTEXT_SIZE,
				   wfse_LOG_CONTEXT_TAG,
				   0);


    KeInitializeEvent(&wfse.FlushCacheEvent, NotificationEvent, FALSE);
    wfse.DebugLevel = wfse_TRACE_ERROR;        
    wfse.MaxCacheCnt = 0;
    status = wfseSetConfiguration( RegistryPath );

    if (!NT_SUCCESS( status )) {
        goto DriverExit;
     }

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &wfse.FilterHandle );

    ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {
	ExInitializeFastMutex(&wfse.FlushCacheMutex);
	ExInitializeFastMutex(&wfse.gLock);
        RtlInitUnicodeString(&tmpstr, wfsePortName);
        status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
        if (NT_SUCCESS(status)) {
            InitializeObjectAttributes(&objattr,
				       &tmpstr,
				       OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
				       NULL,
				       sd);
	    status = FltCreateCommunicationPort(wfse.FilterHandle,
                                                &wfse.ServerPort,
                                                &objattr,
                                                NULL,
                                                wfsePortConnect,
                                                wfsePortDisconnect,
                                                NULL,
                                                1);
                                   
             if (NT_SUCCESS(status)) {
                     status = FltStartFiltering( wfse.FilterHandle );
                     if (!NT_SUCCESS( status )) {
                         FltCloseCommunicationPort(wfse.ServerPort); 
                     }
              }
              FltFreeSecurityDescriptor(sd);
         }

	status = PsCreateSystemThread(&threadHandle,
					(ACCESS_MASK)0,
					NULL,
					(HANDLE) 0,
					NULL,
					&wfseResetCache,
					NULL);

	if (!NT_SUCCESS(status)) {
	    DbgPrint("DriverEntry: cache reset thread creation failed \n");
            goto DriverExit;
	}

	ObReferenceObjectByHandle(threadHandle,
				  THREAD_ALL_ACCESS,
				  NULL,
				  KernelMode,
				  &wfse.FlushCacheThread,
				  NULL);

	ZwClose(threadHandle);
    }

DriverExit:

    if (!NT_SUCCESS( status )) {
        FltUnregisterFilter( wfse.FilterHandle );
        wfse.FilterHandle = NULL;
        wfseFreeList();
    }

    return status;
}


NTSTATUS
wfseUnload (
    IN FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DbgPrint("wfse!wfseUnload: Entered\n" );

    FltCloseCommunicationPort(wfse.ServerPort);
    wfse.ServerPort = NULL;

    wfse.Unloading = TRUE;
    KeSetEvent(&wfse.FlushCacheEvent, IO_NO_INCREMENT, FALSE); 

    ASSERT(wfse.FlushCacheThread != NULL);

    KeWaitForSingleObject(wfse.FlushCacheThread, Executive, KernelMode, FALSE, NULL);

    ObDereferenceObject(wfse.FlushCacheThread);

    FltUnregisterFilter( wfse.FilterHandle );
    wfse.FilterHandle = NULL;
    wfseFreeList();

    return STATUS_SUCCESS;
}

NTSTATUS
wfseInstanceSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++
Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

    VolumeFilesystemType - A FLT_FSTYPE_* value indicating which file system type
        the Filter Manager is offering to attach us to.

Return Value:
    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach
--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    Pwfse_INSTANCE_CONTEXT instanceContext = NULL;
    BOOLEAN IsSnapshotVolume = FALSE;
    PIRP Irp;
    CHAR Buffer[512];
    PDEVICE_OBJECT DeviceObject = NULL;
    PSTORAGE_DEVICE_DESCRIPTOR DevDesc;
    STORAGE_PROPERTY_QUERY VolumeQuery;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    Pwfse_AVL_TREE avl;


    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeDeviceType);

    PAGED_CODE();

    status = FltIsVolumeSnapshot(FltObjects->Instance, &IsSnapshotVolume);
    if (NT_SUCCESS(status) && IsSnapshotVolume) {
        DbgPrint("InstanceSetup: it is a snapshot volume \n"); 
        return status;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE); 
    VolumeQuery.PropertyId = StorageDeviceProperty;
    VolumeQuery.QueryType = PropertyStandardQuery;
    DevDesc = (PSTORAGE_DEVICE_DESCRIPTOR)&Buffer[0];
    status = FltGetDiskDeviceObject(FltObjects->Volume, &DeviceObject);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Could not create the Device Object status %x \n", status);
        return status;
    }

    Irp = IoBuildDeviceIoControlRequest(WFSE_IOCTL_VOLUME_INFO,
                                        DeviceObject,
                                        &VolumeQuery,
                                        sizeof(VolumeQuery),
                                        DevDesc,
                                        sizeof(Buffer),
                                        FALSE,
                                        &Event,
                                        &IoStatus);
    if (Irp) {
        status = IoCallDriver(DeviceObject, Irp);
        if (status == STATUS_PENDING) {
            status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
         } 
         if (NT_SUCCESS(status)) {
            if (DevDesc->BusType == BusTypeUsb) {
                ObDereferenceObject(DeviceObject);
                return STATUS_FLT_DO_NOT_ATTACH;
            }
         } 
    }
    ObDereferenceObject(DeviceObject);

    if (VolumeFilesystemType == FLT_FSTYPE_NTFS) {

        status = wfseAllocateContext(FLT_INSTANCE_CONTEXT, &instanceContext);
        if (NT_SUCCESS(status)) {
            if (debugCtx) {
                    DbgPrint("instanceSetup: allocate instance ctx %x instance %x \n", instanceContext, FltObjects->Instance);
            }
        } else  {
	    if (debugCtx) {
                DbgPrint("instanceSetup: failed allocate instance %x status %x \n", FltObjects->Instance, status);
            }
            return status;
        }
	instanceContext->Volume = FltObjects->Volume;
	instanceContext->Instance = FltObjects->Instance;
	instanceContext->VolumeFsType = VolumeFilesystemType;

        status = FltSetInstanceContext(FltObjects->Instance,
			    FLT_SET_CONTEXT_KEEP_IF_EXISTS,
			    instanceContext,
			    NULL );
	if (debugCtx) {
            DbgPrint("instanceSetup: release instance ctx %x instance %x \n", instanceContext, FltObjects->Instance);
        }
        FltReleaseContext( instanceContext );

            //ref cnt is one if success, zero if failed after FltReleaseContext 
        if (!NT_SUCCESS( status )) {
                  DbgPrint("InstanceSetup: set instance context failed status = 0x%x\n", status);
                  return STATUS_FLT_DO_NOT_ATTACH;
        }
        if (debugCtx) {
            PNLWFSE_DATA pwfse;
            pwfse = &wfse;
            DbgPrint("wfseInstanceSetup: set instance ctx %x for Instance %x volcnt %d \n", instanceContext, FltObjects->Instance, wfse.GvCnt);
        }
        wfse.Gv[wfse.GvCnt].Instance = instanceContext->Instance; 
        for (int i = 0; i < WFSE_HASH_TABLE_SIZE; i++) {
            wfse.Gv[wfse.GvCnt].avl[i] = instanceContext->hash[i];
            avl = wfse.Gv[wfse.GvCnt].avl[i];
            avl->Resource = instanceContext->hash[i]->Resource;
            wfse.Gv[wfse.GvCnt].avl[i]->Instance = instanceContext->Instance;            
        }
        wfse.GvCnt++;

        status = STATUS_SUCCESS;
    } else {
            return STATUS_FLT_DO_NOT_ATTACH;
    }

    return status;
}
 
NTSTATUS
wfseInstanceQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DbgPrint("wfseInstanceQueryTeardown: Entered\n" );

    return STATUS_SUCCESS;
}


VOID
wfseInstanceTeardownStart (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is been deleted.

Return Value:

    None.

--*/
{
  UNREFERENCED_PARAMETER(FltObjects);

    UNREFERENCED_PARAMETER( Flags );
    //Pwfse_INSTANCE_CONTEXT instanceContext = NULL;
    //NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    DbgPrint("wfseInstanceTeardownStart: Entered\n" );

}


VOID
wfseInstanceTeardownComplete (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
{

    UNREFERENCED_PARAMETER( Flags );
    Pwfse_INSTANCE_CONTEXT instanceContext = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    Pwfse_AVL_TREE avl;
    BOOLEAN IsSnapshotVolume = FALSE;
    BOOLEAN found = FALSE;

    PAGED_CODE();

    status = FltIsVolumeSnapshot(FltObjects->Instance, &IsSnapshotVolume);
    if (NT_SUCCESS(status) && IsSnapshotVolume) {
        if (debugCtx) {
            DbgPrint("InstanceTearDownComplete: it is a snapshot volume \n"); 
        }
        return;
    }

    status = FltGetInstanceContext(FltObjects->Instance, &instanceContext);
    if (!NT_SUCCESS(status)) {
        if (debugCtx) {
            DbgPrint("InstanceTeardownComplete: FltGetInstanceContext failed status 0x%x \n", status); 
        }
        return;
    }
    if (instanceContext != NULL) {
        if (instanceContext->VolumeFsType == FLT_FSTYPE_NTFS) {
         
            if (debugCache) {
                DbgPrint("wfseInstanceTeardown: Lookup cache ctx %x instance %x \n", instanceContext, FltObjects->Instance); 
            }

           for (int i = 0; i < WFSE_HASH_TABLE_SIZE; i++) {
               if (instanceContext->hash[i]->fileQueryCache != NULL) {
                   wfseAcquireResourceExclusive(instanceContext->hash[i]->Resource);
                   if (instanceContext->hash[i]->cacheCnt > 0) {
                       wfseAvlFreeNode(instanceContext->hash[i]->fileQueryCache,
                                      &instanceContext->hash[i]->LookasideField); 
                   }
                   wfseReleaseResource(instanceContext->hash[i]->Resource );
               }
               if (debugPool) {
                   DbgPrint("wfseInstanceTeardownComplete: delete lookaside %p \n", &instanceContext->hash[i]->LookasideField);
               }
               ExDeletePagedLookasideList(&instanceContext->hash[i]->LookasideField);
               if (debugPool) {
                   DbgPrint("wfseInstanceTeardownComplete: free Resource %p \n", instanceContext->hash[i]->Resource);
               }
               ExDeleteResourceLite(instanceContext->hash[i]->Resource );
               ExFreePoolWithTag(instanceContext->hash[i]->Resource, wfse_ERESOURCE_POOL_TAG);
           }  

           if (debugCtx) {
               PNLWFSE_DATA pwfse;
               pwfse = &wfse;
           } 

            for (int i = 0; i < wfse.GvCnt && !found; i++) {
                if (wfse.Gv[i].Instance == instanceContext->Instance) { 
                    found = TRUE;
                    for (int j = 0; j < WFSE_HASH_TABLE_SIZE; j++) {
                        wfse.Gv[i].avl[j] = instanceContext->hash[j];
                        avl = wfse.Gv[i].avl[j];
                        avl->Resource = NULL;
                        wfse.Gv[i].avl[j]->Instance = NULL;
                        if (debugPool) {
                           DbgPrint("wfseInstanceTeardownComplete: free avltree %p \n", instanceContext->hash[j]);
                        }
                        ExFreePoolWithTag(instanceContext->hash[j], wfse_AVL_TAG);
                        wfse.Gv[i].avl[j] = NULL;
                        instanceContext->hash[j] = NULL;
                    }
                    wfse.Gv[i].Instance = NULL;
                }
            }

            FltReleaseContext( instanceContext );

            if (debugCtx) {
                DbgPrint("wfse!wfseInstanceTeardownComplete: delete instance ctx %p instance %p \n", instanceContext, FltObjects->Instance);
            }
            status = FltDeleteInstanceContext(FltObjects->Instance, NULL);
            if (!NT_SUCCESS(status)) {
                DbgPrint("InstanceTeardownComplete: FltDeleteInstanceContext failed status 0x%x \n", status); 
            }

            instanceContext = NULL;
             
         }

    }

}

NTSTATUS
wfseAllocateContext (
    __in FLT_CONTEXT_TYPE ContextType,
    __out PFLT_CONTEXT *Context
    )
{
    NTSTATUS status;
    PAGED_CODE();
    Pwfse_AVL_TREE avlTree;
    Pwfse_STREAMHANDLE_CONTEXT sctx = NULL;
    Pwfse_INSTANCE_CONTEXT ctx = NULL;

    switch (ContextType) {

        case FLT_INSTANCE_CONTEXT:

            status = FltAllocateContext( wfse.FilterHandle,
                                         FLT_INSTANCE_CONTEXT,
                                         sizeof(wfse_INSTANCE_CONTEXT),
                                         wfse_CONTEXT_POOL_TYPE,
                                         Context );

            if (NT_SUCCESS( status )) {
                RtlZeroMemory( *Context, sizeof(wfse_INSTANCE_CONTEXT) );
                ctx = *Context;

                for (int i = 0; i < WFSE_HASH_TABLE_SIZE; i++) {
		    avlTree = (Pwfse_AVL_TREE )ExAllocatePoolWithTag(NonPagedPool,
                                                                 sizeof(wfse_AVL_TREE),
                                                                 wfse_AVL_TAG);
                    if (avlTree == NULL) {
		        FltReleaseContext(ctx);
                        status = STATUS_NO_MEMORY;
                        return status;
                    }
		    RtlZeroMemory(avlTree, sizeof(wfse_AVL_TREE));
                    ctx->hash[i] = avlTree;
                    if (debugCtx)
                        DbgPrint("wfseAllocateCtx: allocate avl %p \n", avlTree);

                }

            
                for (int i = 0; i < WFSE_HASH_TABLE_SIZE; i++) {
                    avlTree = ctx->hash[i];

                     ExInitializePagedLookasideList(&avlTree->LookasideField,
                                                         NULL,
                                                         NULL,
                                                         POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                         sizeof(wfseAvlNode),
                                                         wfse_AVL_TAG,
                                                         0);

	             avlTree->Resource = ExAllocatePoolWithTag(NonPagedPool,
			                                       sizeof(ERESOURCE),
			                                       wfse_ERESOURCE_POOL_TAG);
			
		     if (NULL == avlTree->Resource) {
		         FltReleaseContext(ctx);
                               
			 return STATUS_INSUFFICIENT_RESOURCES;
		      }
		      ExInitializeResourceLite(avlTree->Resource);
                      avlTree->Instance = ctx->Instance;
                      if (debugCtx)
                          DbgPrint("wfseAllocateCtx: lookaside %p Tree Resource %p \n", &avlTree->LookasideField, avlTree->Resource);
               }
            }

            return status;

        case FLT_STREAMHANDLE_CONTEXT:

            status = FltAllocateContext(wfse.FilterHandle,
                                        FLT_STREAMHANDLE_CONTEXT,
                                        sizeof(wfse_STREAMHANDLE_CONTEXT),
                                        wfse_CONTEXT_POOL_TYPE,
                                        &sctx);

            if (!NT_SUCCESS( status )) {
                DbgPrint("wfseAllocateCtx: Failed to allocate sctx handle context status 0x%x \n", status);
                return status;
            }

            RtlZeroMemory(sctx, sizeof(wfse_STREAMHANDLE_CONTEXT));
            *Context = sctx;

            return status; 


        default:

            return STATUS_INVALID_PARAMETER;
    }
}

NTSTATUS
wfseSetContext (
      __in PFLT_INSTANCE Instance,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    __in FLT_CONTEXT_TYPE ContextType,
    __in PFLT_CONTEXT NewContext,
    __out PFLT_CONTEXT *OldContext
    )
{
    PAGED_CODE();
    NTSTATUS status;
    Pwfse_STREAMHANDLE_CONTEXT oldCtx;

    switch (ContextType) {

        case FLT_STREAM_CONTEXT:

            status = FltSetStreamContext( Instance,
                                        (PFILE_OBJECT)Target,
                                        FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                        NewContext,
                                        OldContext );
            if (debugCtx)
                DbgPrint("setCtx: newctx %x oldctx %x status %x \n", NewContext,
                          OldContext, status);
            return status; 

        case FLT_INSTANCE_CONTEXT:

            return FltSetInstanceContext( Instance,
                                          FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                          NewContext,
                                          OldContext );

        case FLT_STREAMHANDLE_CONTEXT:

            status = FltSetStreamHandleContext(Instance,
                                               (PFILE_OBJECT)Target,
                                               FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                               NewContext,
                                               &oldCtx);

            if (!NT_SUCCESS( status )) {
                DbgPrint("wfseSetCtx: Failed to set sctx handle context status 0x%x. (FileObject = %p, Instance = %p)\n",
                    status,
                    Target,
                    Instance);

                FltReleaseContext(NewContext);
                if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

                    DbgPrint("Failed to set stream handle context with status 0x%x != STATUS_FLT_CONTEXT_ALREADY_DEFINED. (FileObject = %p, Instance = %p)\n",
                        status,
                        Target,
                        Instance);

                    return status;
                } else {

                if (debugCtx) {
                    DbgPrint("wfseSetCtx: already defined sctx %x \n", NewContext);
                }
                NewContext = oldCtx;                
                status = STATUS_SUCCESS;
               }
           }

           return status;

        default:

            ASSERT( !"Unexpected context type!\n" );

            return STATUS_INVALID_PARAMETER;
    }
}


NTSTATUS
wfseGetContext (
    IN PFLT_INSTANCE Instance,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    __in FLT_CONTEXT_TYPE ContextType,
    __out PFLT_CONTEXT *Context
    )
{
    PAGED_CODE();
    
    switch (ContextType) {

        case FLT_STREAM_CONTEXT:

            return FltGetStreamContext(Instance,
                                        (PFILE_OBJECT)Target,
                                        Context );

        case FLT_INSTANCE_CONTEXT:
            if (debugCtx)
                DbgPrint("wfseGetContext: get instance ctx \n");

            return FltGetInstanceContext(Instance,
                                          Context );

        default:

            return STATUS_INVALID_PARAMETER;
    }
}

VOID
wfseStreamHandleContextCleanup(
    __in Pwfse_STREAMHANDLE_CONTEXT StreamContext,
    __in FLT_CONTEXT_TYPE ContextType
    )
{
    UNREFERENCED_PARAMETER( ContextType );

    PAGED_CODE();

    ASSERT( ContextType == FLT_STREAMHANDLE_CONTEXT );

    if (StreamContext) {

        if (StreamContext->sid != NULL) {
            ExFreeToPagedLookasideList(&wfse.SidBuffer, StreamContext->sid->Buffer);
            ExFreeToPagedLookasideList(&wfse.StringPtr, StreamContext->sid);
            StreamContext->sid = NULL;
        }

        if (StreamContext->fileName != NULL) {
            ExFreeToPagedLookasideList(&wfse.TmpBuffer, StreamContext->fileName->Buffer);
            ExFreeToPagedLookasideList(&wfse.StringPtr, StreamContext->fileName);
            StreamContext->fileName = NULL;
        }
    }
}

VOID
wfseInstanceContextCleanup(
    __in Pwfse_INSTANCE_CONTEXT InstanceContext,
    __in FLT_CONTEXT_TYPE ContextType
    )
/*++

Arguments:

    InstanceContext - Pointer to wfse_INSTANCE_CONTEXT to be cleaned up.

    ContextType - Type of InstanceContext. Must be FLT_INSTANCE_CONTEXT.

--*/
{

    UNREFERENCED_PARAMETER( ContextType );

    PAGED_CODE();
    DbgPrint("wfseInstanceContextCleanup: called %x \n", InstanceContext);
    ASSERT( ContextType == FLT_INSTANCE_CONTEXT );

}

FLT_PREOP_CALLBACK_STATUS
wfsePreCreate(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext
    )
{
    Pwfse_STREAMHANDLE_CONTEXT streamContext = NULL;
    Pwfse_INSTANCE_CONTEXT instanceContext = NULL;
    PFLT_FILE_NAME_INFORMATION NameInfo = NULL;
    LARGE_INTEGER curSystemTime;
    Pwfse_LOG_CONTEXT logctx = NULL;
    Pwfse_REQUEST_INFO reqInfo = NULL;
    BOOLEAN srvReq = FALSE;
    wfse_QUERY_CACHE item = {0};
    TIME_FIELDS logtime, ltime; 
    LARGE_INTEGER ctime;
    PwfseAvlNode entry = NULL;
    BOOLEAN IsSnapshotVolume = FALSE;
    PECP_LIST ecpList;
    PVOID ecpCtx = NULL;
    ACCESS_MASK desiredAccess = 0;
    UNICODE_STRING HashKey;
    PCWSTR tmpstr = L"ini";
    PCWSTR infstr = L"inf";
    PCWSTR perflogs = L"PerfLogs";
    PCWSTR docxfile = L"docx";
    PCWSTR ppfile = L"pptx";
    PCWSTR xlfile = L"xlsx";
    PCWSTR pdffile = L"pdf";
    UNICODE_STRING usrc; 
    BOOLEAN denyCachedOpen = FALSE;
    BOOLEAN denyCachedDelete = FALSE;
    BOOLEAN denyCachedWrite = FALSE;
    BOOLEAN denyCachedRename = FALSE;
    BOOLEAN denyCachedchgattrs = FALSE;
    BOOLEAN denyCachedchgsecs = FALSE;
    BOOLEAN dochgattrs = FALSE;
    BOOLEAN dowrite = FALSE;
    BOOLEAN doDelete = FALSE;
    BOOLEAN doRename = FALSE;
    BOOLEAN dochgsecurity = FALSE;
    BOOLEAN Officefile = FALSE;
    BOOLEAN skipLog = FALSE;
    ULONG hashValue;
    ULONG idx;

    NTSTATUS status;

    PAGED_CODE();

    if (passthru) 
        goto PreCreateExit;                

    if (!wfse.ProtectionOn) {
        goto PreCreateExit;                
    }

    if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)) {
        if (debugCtx) {
          if (FltObjects->FileObject->FileName.Buffer != NULL) {
              if (debugCtx) {
                  DbgPrint("PreCreate: file open for rename %wZ \n", &FltObjects->FileObject->FileName);
              }
          }
        }
        doRename = TRUE;
    }

    if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE)) {
        goto PreCreateExit;                
    }

    status = FltIsVolumeSnapshot(FltObjects->Instance, &IsSnapshotVolume);
    if (NT_SUCCESS(status) && IsSnapshotVolume) {
        goto PreCreateExit;                
    }
    desiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
    if (FlagOn(desiredAccess, FILE_EXECUTE)) {
        goto PreCreateExit;                
    } 

    if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE)) {
        goto PreCreateExit;                
    }
    if (FlagOn(FltObjects->FileObject->Flags, FO_VOLUME_OPEN)) {
        goto PreCreateExit;                
    }
    status =  FltGetEcpListFromCallbackData(FltObjects->Filter, Data, &ecpList);
    if (NT_SUCCESS(status) && (ecpList != NULL)) {
        status = FltFindExtraCreateParameter(FltObjects->Filter,
                                             ecpList,
                                             &GUID_ECP_PREFETCH_OPEN,
                                             &ecpCtx,
                                             NULL);
        if (NT_SUCCESS(status)) {
            if (debugCtx)
                DbgPrint("PreCreate: detect prefetch \n"); 
            if (!FltIsEcpFromUserMode(FltObjects->Filter, ecpCtx)) {
                if (debugCtx)
                    DbgPrint("PreCreate: prefetch \n"); 
                goto PreCreateExit;                
            }
        }
    }

    if (IsMyPc(Data)) {
        status = FltGetFileNameInformation(Data,
                                       FLT_FILE_NAME_NORMALIZED|FLT_FILE_NAME_QUERY_DEFAULT,
                                       &NameInfo);
        if (!NT_SUCCESS( status )) {
            goto PreCreateExit;
        }

        status = FltParseFileNameInformation(NameInfo);
        if (!NT_SUCCESS( status )) {
            goto PreCreateExit;
        }


        if (wcsstr(NameInfo->Name.Buffer, L"bundle.bin")) {
            if (wfse.MaxCacheCnt > 0 ) {

            status = wfseAllocateContext(FLT_STREAMHANDLE_CONTEXT, &streamContext );
            if (NT_SUCCESS( status )) {
                DbgPrint("PreCreate: open bundle file allocate sctx \n", streamContext); 
                streamContext->fileName = ExAllocateFromPagedLookasideList(&wfse.StringPtr);

                if (NameInfo->Name.MaximumLength > (WFSE_TMP_BUFFER_SIZE - sizeof(WCHAR))) {
                    streamContext->fileName->Length = WFSE_TMP_BUFFER_SIZE - sizeof(WCHAR);
                    streamContext->fileName->MaximumLength = WFSE_TMP_BUFFER_SIZE - sizeof(WCHAR);
                } else {
                    streamContext->fileName->Length = NameInfo->Name.Length;
                    streamContext->fileName->MaximumLength = NameInfo->Name.MaximumLength;
                }

                streamContext->fileName->Buffer = ExAllocateFromPagedLookasideList(&wfse.TmpBuffer);

                RtlZeroMemory(streamContext->fileName->Buffer, WFSE_TMP_BUFFER_SIZE);
                RtlCopyUnicodeString(streamContext->fileName, &NameInfo->Name); 
                streamContext->allowedAccess |= WFSE_RESET_CACHE;
                *CompletionContext = (PVOID)streamContext;
            } else {
                DbgPrint("PreCreate: pc allocate context failed status %x \n", status); 
                streamContext = NULL;
            }
            }
        }
        goto PreCreateExit;
    }

    srvReq =  wfseGetFileRequestInfo(Data, &reqInfo);
    if (!srvReq) {
        goto PreCreateExit;                
    }
    if (reqInfo->ipaddr == 0) {
        goto PreCreateExit;                
    }

    if (!FLT_IS_IRP_OPERATION( Data )) {
        goto PreCreateExit;
    }

    status = FltGetFileNameInformation(Data,
                                       FLT_FILE_NAME_NORMALIZED|FLT_FILE_NAME_QUERY_DEFAULT,
                                       &NameInfo);
    if (!NT_SUCCESS( status )) {

        DbgPrint("wfsepreCreate: Failed to get filename (Status = 0x%x)\n", status);
        goto PreCreateExit;
    }
    status = FltParseFileNameInformation(NameInfo);
    if (!NT_SUCCESS( status )) {
        DbgPrint("wfsepreCreate: Failed to parse filename (Name = %wZ, Status = 0x%x)\n",
                    &NameInfo->Name,
                    status);
        goto PreCreateExit;
    }


    if (NameInfo->Name.Length == 0x30 || NameInfo->Name.Length == 0x32) {
      // Name should be null terminated
      if (wcsstr(NameInfo->Name.Buffer, L"\\Device\\HarddiskVolume")) {
        goto PreCreateExit;
      }
    }

    if (NameInfo->Extension.Length > 0) {
      RtlInitUnicodeString(&usrc, tmpstr);
      if (RtlCompareUnicodeString(&NameInfo->Extension, &usrc, TRUE) == 0) {
        goto PreCreateExit;
      }
      RtlInitUnicodeString(&usrc, infstr);
      if (RtlCompareUnicodeString(&NameInfo->Extension, &usrc, TRUE) == 0) {
        goto PreCreateExit;
      }
      RtlInitUnicodeString(&usrc, perflogs);
      if (RtlCompareUnicodeString(&NameInfo->FinalComponent, &usrc, TRUE) == 0) {
        goto PreCreateExit;
      }
    }

    if (NameInfo->Stream.Length > 0) {
      goto PreCreateExit;
    }
    if (debugPreCreate) {
        DbgPrint("PreCreate: srv file open %wZ \n", &NameInfo->Name); 
    }

    status = FltGetInstanceContext(FltObjects->Instance, &instanceContext);
    if (!NT_SUCCESS( status )){
	DbgPrint("PreCreate: failed to get instance context.\n");
	return status;
    }
    if (debugCtx) {
	DbgPrint("PreCreate: get instance %x context %x \n", FltObjects->Instance, instanceContext);
    }

    item.ipaddr = reqInfo->ipaddr;
    RtlInitUnicodeString(&HashKey, NULL);

    HashKey.MaximumLength = reqInfo->sid->Length + NameInfo->Name.Length + sizeof(WCHAR);
    HashKey.Buffer = ExAllocateFromPagedLookasideList(&wfse.TmpBuffer);
    RtlZeroMemory(HashKey.Buffer, WFSE_TMP_BUFFER_SIZE);

    RtlCopyUnicodeString(&HashKey, reqInfo->sid);
    status = RtlAppendUnicodeStringToString(&HashKey, &NameInfo->Name);
    RtlHashUnicodeString(&HashKey,
			 TRUE,
			 HASH_STRING_ALGORITHM_X65599,
			 &hashValue);      
    idx = hashValue % WFSE_HASH_TABLE_SIZE;
    item.key = hashValue;
    ExFreeToPagedLookasideList(&wfse.TmpBuffer, HashKey.Buffer);
    if (debugCache) {
        DbgPrint("PreCreate: file %wZ hashValue key %x idx %d \n", &NameInfo->Name, hashValue, idx); 
    }

    if (wfse.CacheStale == FALSE) {

    wfseAcquireResourceShared(instanceContext->hash[idx]->Resource);
    entry = wfseAvlSearchNode(instanceContext->hash[idx], &item);
    if (entry != NULL) {
        if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)) {
             if (debugCtx) {
                DbgPrint("PreCreate: cached file for rename %wZ \n", &NameInfo->Name);
             }
        }

        if (debugCache) {
            DbgPrint("PreCreate: cache hit %wZ \n", &NameInfo->Name); 
        }
        if (entry->allowedAccess & DENY_OPEN) {
            denyCachedOpen = TRUE;
            wfseReleaseResource(instanceContext->hash[idx]->Resource );

            if (debugCtx)
                DbgPrint("PreCreate: log deny open cache logtime %x hit %wZ \n", entry->logTime, &NameInfo->Name); 

            if (entry->logTime.QuadPart > 0) {
                KeQuerySystemTime(&curSystemTime);
                ExSystemTimeToLocalTime(&curSystemTime, &ctime); 
                RtlTimeToTimeFields(&ctime, &ltime);
                RtlTimeToTimeFields(&entry->logTime, &logtime);
	        if ((ltime.Day == logtime.Day) && (ltime.Hour == logtime.Hour) && (ltime.Minute == logtime.Minute) && (ltime.Second == logtime.Second)) {
                    if (debugCache) {
                        DbgPrint("PreCreate: skip log deny cached open %wZ \n", &NameInfo->Name);
                    }
                    skipLog = TRUE;
                } else if ((ltime.Day == logtime.Day) && (ltime.Hour == logtime.Hour) && (ltime.Minute == logtime.Minute) && (ltime.Second != logtime.Second)) {
                     if ((ltime.Second - logtime.Second) < 60) {
                         skipLog = TRUE;
                     }
                     if (debugCache) {
                         DbgPrint("PreCreate: skip log deny cached open %wZ \n", &NameInfo->Name);
                     }
                }

            } else if (entry->logTime.QuadPart == 0) {
                KeQuerySystemTime(&curSystemTime);
                ExSystemTimeToLocalTime(&curSystemTime, &ctime); 
                wfseAcquireResourceExclusive(instanceContext->hash[idx]->Resource);
                entry->logTime = ctime;
                wfseReleaseResource(instanceContext->hash[idx]->Resource );
                if (debugCache) {
                    DbgPrint("PreCreate: init log time deny cached open %wZ \n", &NameInfo->Name);
                }
            }

            if (!skipLog) {
                logctx = ExAllocateFromPagedLookasideList(&wfse.LogInfoBuffer);
                logctx->faction = 0; 
                logctx->fileName = &NameInfo->Name;
                logctx->sid = reqInfo->sid;
                logctx->ipaddr = reqInfo->ipaddr;
                logctx->faction |= WFSE_FILE_CACHE_OPEN;
                wfseLogUpdate(logctx);
                ExFreeToPagedLookasideList(&wfse.LogInfoBuffer, logctx);
            }
            goto PreCreateExit;
        } 
        if (FlagOn(desiredAccess, FILE_WRITE_DATA)) {
            dowrite = TRUE;
            if (debugCtx)
                DbgPrint("PreCreate: desired cached open for write  %wZ \n", &NameInfo->Name); 
            if (entry->allowedAccess & DENY_WRITE) {
                denyCachedWrite = TRUE;
                if (debugCtx)
                    DbgPrint("PreCreate: log deny cached open for write  %wZ \n", &NameInfo->Name); 
                wfseReleaseResource(instanceContext->hash[idx]->Resource );
                logctx = ExAllocateFromPagedLookasideList(&wfse.LogInfoBuffer);
                logctx->fileName = &NameInfo->Name;
                logctx->sid = reqInfo->sid;
                logctx->ipaddr = reqInfo->ipaddr;
                logctx->faction = WFSE_FILE_WRITE;
                wfseLogUpdate(logctx);
                ExFreeToPagedLookasideList(&wfse.LogInfoBuffer, logctx);
                goto PreCreateExit;
            } 
            RtlInitUnicodeString(&usrc, docxfile);
            if (RtlCompareUnicodeString(&NameInfo->Extension, &usrc, TRUE) == 0) {
                Officefile = TRUE; 
            } else {
                RtlInitUnicodeString(&usrc, ppfile);
                if (RtlCompareUnicodeString(&NameInfo->Extension, &usrc, TRUE) == 0) {
                    Officefile = TRUE; 
                } else {
                    RtlInitUnicodeString(&usrc, xlfile);
                    if (RtlCompareUnicodeString(&NameInfo->Extension, &usrc, TRUE) == 0) {
                       Officefile = TRUE; 
                    } else {
                        RtlInitUnicodeString(&usrc, pdffile);
                        if (RtlCompareUnicodeString(&NameInfo->Extension, &usrc, TRUE) == 0) {
                            Officefile = TRUE; 
                        }
                    }
                }
            }
        }
        if (FlagOn(desiredAccess, FILE_WRITE_ATTRIBUTES)) {
            dochgattrs = TRUE;
            if (debugCtx)
                DbgPrint("PreCreate: desired to chgattrs cache hit file %wZ \n", &NameInfo->Name); 
            if (entry->allowedAccess & DENY_CHANGE_ATTRS) {
                denyCachedchgattrs = TRUE;
                wfseReleaseResource(instanceContext->hash[idx]->Resource );

                if (debugCtx)
                    DbgPrint("PreCreate: log deny cached open for chgattrs  %wZ \n", &NameInfo->Name); 

                if (debugCtx)
                    DbgPrint("PreCreate: delete cached entry open for chgattrs  %wZ \n", &NameInfo->Name); 

                logctx = ExAllocateFromPagedLookasideList(&wfse.LogInfoBuffer);
                logctx->faction = 0; 
                logctx->fileName = &NameInfo->Name;
                logctx->sid = reqInfo->sid;
                logctx->ipaddr = reqInfo->ipaddr;
                logctx->faction = WFSE_FILE_CHANGE_ATTRS;
                wfseLogUpdate(logctx);
                ExFreeToPagedLookasideList(&wfse.LogInfoBuffer, logctx);
                goto PreCreateExit;
            } 
        }

        if (FlagOn(desiredAccess, DELETE)) {
            doDelete = TRUE;
                if (debugCtx)
                    DbgPrint("PreCreate: desired delete cache hit %wZ \n", &NameInfo->Name); 
            if (entry->allowedAccess & DENY_DELETE) {
                denyCachedDelete = TRUE;
                if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE)) {
              
                wfseReleaseResource(instanceContext->hash[idx]->Resource );

                if (debugCtx)
                    DbgPrint("PreCreate: delete request log deny delete cache entry %wZ \n", &NameInfo->Name); 
                logctx = ExAllocateFromPagedLookasideList(&wfse.LogInfoBuffer);
                logctx->faction = 0; 
                logctx->fileName = &NameInfo->Name;
                logctx->sid = reqInfo->sid;
                logctx->ipaddr = reqInfo->ipaddr;
                logctx->faction |= WFSE_FILE_DELETE;
                wfseLogUpdate(logctx);
                ExFreeToPagedLookasideList(&wfse.LogInfoBuffer, logctx);
                goto PreCreateExit;
                }
            } 
        }

        if (FlagOn(desiredAccess, DELETE) && entry->denyRename) {
            if (debugCtx)
                DbgPrint("PreCreate: deny rename cache hit %wZ \n", &NameInfo->Name); 

            doRename = TRUE;

            if (debugCtx) {
                if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE)) {
                    DbgPrint("PreCreate: delete request cache in deny rename hit %wZ \n", &NameInfo->Name); 
                } else {
                    DbgPrint("PreCreate: NOT delete request cache in deny rename hit %wZ \n", &NameInfo->Name); 
                }
            }

            if (entry->allowedAccess & DENY_RENAME) {
                denyCachedRename = TRUE;
                wfseReleaseResource(instanceContext->hash[idx]->Resource );

                if (debugCtx)
                    DbgPrint("PreCreate: try to log deny cached rename allowedAccess %wZ \n", &NameInfo->Name); 
                logctx = ExAllocateFromPagedLookasideList(&wfse.LogInfoBuffer);
                logctx->fileName = &NameInfo->Name;
                logctx->sid = reqInfo->sid;
                logctx->ipaddr = reqInfo->ipaddr;
                logctx->faction = WFSE_FILE_RENAME;
                wfseLogUpdate(logctx);
                ExFreeToPagedLookasideList(&wfse.LogInfoBuffer, logctx);
                goto PreCreateExit;
            }
        }

	if (FlagOn(desiredAccess, DELETE) && (entry->allowedAccess & DENY_RENAME)) {
		doRename = TRUE;
                if (debugCtx)
                    DbgPrint("PreCreate: desire to delete cache hit  but no permission to rename file %wZ \n", &NameInfo->Name); 
	}

        if (FlagOn(desiredAccess, NL_CHANGE_SECURITY_ATTRIBUTE)) {
            dochgsecurity = TRUE;
            if (debugCtx)
                DbgPrint("PreCreate: change security file %wZ \n", &NameInfo->Name); 
            if (entry->allowedAccess & DENY_CHANGE_SEC_ATTRS) {
                denyCachedchgsecs = TRUE;

                if (debugCtx)
                    DbgPrint("PreCreate: don't have permission to change security but try to change security file %wZ \n", &NameInfo->Name); 
            }
        }

	if (!dochgattrs && !doDelete && !doRename && !dochgsecurity && !dowrite && FlagOn(entry->allowedAccess, ALLOW_READ|ALLOW_WRITE|ALLOW_DELETE|ALLOW_RENAME|ALLOW_CHANGE_ATTRS|ALLOW_CHANGE_SEC_ATTRS)) {
            wfseReleaseResource(instanceContext->hash[idx]->Resource );
            if (debugCtx)
                DbgPrint("PreCreate: bailout conditions file %wZ \n", &NameInfo->Name); 
            goto PreCreateExit;
	}
    }
    wfseReleaseResource(instanceContext->hash[idx]->Resource );
    } else {
	DbgPrint("PreCreate: Cache Stale... \n");
	goto PreCreateExit;
    }

    status = wfseAllocateContext(FLT_STREAMHANDLE_CONTEXT, &streamContext );

    if (NT_SUCCESS( status )) {
        if (debugPreCreate) {
            DbgPrint("PreCreate: FltObjects FileObject %x \n", FltObjects->FileObject);
        }
        if (debugCtx) {
            DbgPrint("PreCreate: allocate streamContext %x \n", streamContext);
            DbgPrint("PreCreate: FltObjects Instance %x Data Instance %x \n",
                         FltObjects->Instance,
                         Data->Iopb->TargetInstance);

        }

        streamContext->fileName = ExAllocateFromPagedLookasideList(&wfse.StringPtr);
        streamContext->fileName->Length = NameInfo->Name.Length;
        streamContext->fileName->MaximumLength = NameInfo->Name.MaximumLength;
        streamContext->fileName->Buffer = ExAllocateFromPagedLookasideList(&wfse.TmpBuffer);
        RtlZeroMemory(streamContext->fileName->Buffer, WFSE_TMP_BUFFER_SIZE);
        RtlCopyUnicodeString(streamContext->fileName, &NameInfo->Name); 

        streamContext->sid = ExAllocateFromPagedLookasideList(&wfse.StringPtr);
        streamContext->sid->Length = reqInfo->sid->Length;
        streamContext->sid->MaximumLength = reqInfo->sid->MaximumLength;

        streamContext->sid->Buffer = ExAllocateFromPagedLookasideList(&wfse.SidBuffer);
        RtlZeroMemory(streamContext->sid->Buffer, WFSE_SID_BUFFER_SIZE);
        RtlCopyUnicodeString(streamContext->sid, reqInfo->sid); 
        streamContext->ipaddr = item.ipaddr;
        streamContext->hashidx = idx;
        streamContext->key = hashValue;
        streamContext->desiredAccess = desiredAccess;
        streamContext->faction = 0;
        if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE)) {
            if (debugCtx) {
                DbgPrint("PreCreate: set allowed delete flag %wZ \n", &NameInfo->Name); 
            }
            streamContext->faction |= WFSE_FILE_DELETE;
            
        }

        if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)) {
             if (debugCtx) {
                DbgPrint("PreCreate: sctx %p file open for rename %wZ \n", streamContext, &NameInfo->Name);
             }
             streamContext->faction |= WFSE_FILE_RENAME;
        }

        if (entry) {
            if (debugCtx) {
                DbgPrint("PreCreate: set cache file open flag %wZ \n", &NameInfo->Name); 
            }

            streamContext->faction |= WFSE_FILE_CACHE_OPEN;
            if (doDelete) {
                if (debugCtx) {
                    DbgPrint("PreCreate: set cache delete file open flag %wZ \n", &NameInfo->Name); 
                }
                streamContext->faction |= WFSE_FILE_DELETE;
            }
            if (debugCtx) {
                if (FlagOn(desiredAccess, FILE_WRITE_DATA)) {
                    DbgPrint("PreCreate: file open for write %wZ \n", &NameInfo->Name); 
                    if (streamContext->allowedAccess & DENY_WRITE) {
                        DbgPrint("PreCreate: write is not allowed file %wZ \n", &NameInfo->Name); 
                    }
                }
                if (FlagOn(desiredAccess, FILE_WRITE_ATTRIBUTES)) {
                    DbgPrint("PreCreate: create sctx cached open file for chg attrs %wZ \n", &NameInfo->Name); 
                }
            }
            if (FlagOn(desiredAccess, FILE_WRITE_DATA)) {
                if (debugCtx) {
                    DbgPrint("PreCreate: open cached Office file for write %wZ \n", &NameInfo->Name); 
                }
                streamContext->Officefile = Officefile;
            }
            streamContext->allowedAccess = entry->allowedAccess;
        }
        *CompletionContext = (PVOID)streamContext;
    } else {
            DbgPrint("wfse!wfsePreCreate: An error occurred with wfseAllocateStreamContext!\n" );
    }

PreCreateExit:

    if (reqInfo) {
        if (reqInfo->sid != NULL) {
	    if( reqInfo->sid->Buffer != NULL ) {
	        RtlFreeUnicodeString(reqInfo->sid);
	    }
            if (debugPool) {
	        DbgPrint("getReqInfo: free memory sid %p \n", reqInfo->sid);
	    }
            ExFreeToPagedLookasideList(&wfse.StringPtr, reqInfo->sid);
            reqInfo->sid = NULL;
        }
        if (debugPool) {
            DbgPrint("PreCreate: free pool reqInfo %p \n", reqInfo);
        }
        ExFreeToPagedLookasideList(&wfse.ReqInfoBuffer, reqInfo);
        reqInfo = NULL;
    }
     
    if (instanceContext) {
        FltReleaseContext(instanceContext);
        if (debugCtx) {
            DbgPrint("PreCreate: release instance ctx %x \n", instanceContext); 
        }
    }

#if 0
    if (NameInfo) {
        FltReleaseFileNameInformation( NameInfo );
    }
#endif

    if (*CompletionContext) {
        if (NameInfo) {
            FltReleaseFileNameInformation( NameInfo );
        }
        if (debugCtx) {
            DbgPrint("PreCreate: sync postCreate sctx %p \n", streamContext); 
        }

        return FLT_PREOP_SYNCHRONIZE;
    }

    if ((dochgattrs && denyCachedchgattrs) || (dowrite && denyCachedWrite) || 
        denyCachedOpen || denyCachedDelete || (doDelete && denyCachedDelete) ||
        (doRename && denyCachedRename) || (dochgsecurity && denyCachedchgsecs)) {
        if (debugCtx) {
            if (entry) {
                DbgPrint("PreCreate: deny access cache hit file %wZ \n", &NameInfo->Name); 
                DbgPrint("PreCreate: deny cache hit key %x rename %d \n", entry->key, entry->denyRename); 
            }
        }
        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;
        if (NameInfo) {
            FltReleaseFileNameInformation( NameInfo );
        }

        return FLT_PREOP_COMPLETE;
    } else {
        *CompletionContext = NULL;
        if (NameInfo) {
            FltReleaseFileNameInformation( NameInfo );
        }
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }
}

FLT_POSTOP_CALLBACK_STATUS
wfsePostCreate(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    NTSTATUS status = Data->IoStatus.Status;
    BOOLEAN IsDir = FALSE;
    BOOLEAN doDelete = FALSE;
    Pwfse_STREAMHANDLE_CONTEXT streamContext = NULL;
    PFLT_CONTEXT oldCtx = NULL;
    ACCESS_MASK desiredAccess;
    LONGLONG fileSize;
    FILE_STANDARD_INFORMATION stdInfo;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    ASSERT(NULL != CompletionContext );
    streamContext = (Pwfse_STREAMHANDLE_CONTEXT)CompletionContext;

    if (wfse.CacheStale == TRUE) {
        if (streamContext) {
	    FltReleaseContext(streamContext);
	}
	DbgPrint("PostCreate: CacheStale skip...\n");
	return FLT_POSTOP_FINISHED_PROCESSING;
    }

    if (!NT_SUCCESS(status) || (status == STATUS_REPARSE)) {
        if (debugCtx)
            DbgPrint("PostCreate: skip reparse status %x \n", status);
        FltReleaseContext(streamContext);
        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    status = FltIsDirectory(FltObjects->FileObject,
                            FltObjects->Instance,
                            &IsDir); 
    if (NT_SUCCESS(status) && IsDir) {
       
        if (debugCtx)
            DbgPrint("PostCreate: no policy need skip directory release ctx %x \n", streamContext);
        if (streamContext) {
            if (debugCtx)
                DbgPrint("PostCreate: dir file release sctx \n", streamContext); 
            FltReleaseContext(streamContext);
        }
        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    status = FltQueryInformationFile(Data->Iopb->TargetInstance,
                                     Data->Iopb->TargetFileObject,
                                     &stdInfo,
                                     sizeof(FILE_STANDARD_INFORMATION),
                                     FileStandardInformation,
                                     NULL);
    if (NT_SUCCESS(status)) {
        fileSize = stdInfo.EndOfFile.QuadPart;
        if (fileSize == 0) {
            if (debugCtx) {
                DbgPrint("PostCreate: release sctx %p file size is zero file %wZ \n",
                              streamContext, streamContext->fileName); 
            }
            if (!(streamContext->allowedAccess & WFSE_RESET_CACHE)) {
                status = wfseSetContext(Data->Iopb->TargetInstance,
			            Data->Iopb->TargetFileObject,
			            FLT_STREAMHANDLE_CONTEXT,
			            streamContext, &oldCtx);

                FltReleaseContext(streamContext);
                return FLT_POSTOP_FINISHED_PROCESSING;
            }
        }
    }

    if (streamContext->allowedAccess & WFSE_RESET_CACHE) {
        if (FltObjects->FileObject->WriteAccess) {
            status = wfseSetContext(Data->Iopb->TargetInstance,
			            Data->Iopb->TargetFileObject,
			            FLT_STREAMHANDLE_CONTEXT,
			            streamContext, &oldCtx);
            FltReleaseContext(streamContext);
            if (!NT_SUCCESS( status )) {
	        DbgPrint("PostCreate: reset cache set ctx failed status %x file %wZ \n", status, streamContext->fileName);
                return FLT_POSTOP_FINISHED_PROCESSING;
            }
	    DbgPrint("PostCreate: reset cache set sctx %x allowedAccess %x file %wZ \n", streamContext, streamContext->allowedAccess, streamContext->fileName);
            if (debugCtx) {
	        DbgPrint("PostCreate: reset cache set sctx %x allowedAccess %x file %wZ \n", streamContext, streamContext->allowedAccess, streamContext->fileName);
            }
	    ExAcquireFastMutex(&wfse.FlushCacheMutex);
	    wfse.CacheStale = TRUE;
	    DbgPrint("PostCreate: Set CacheStale true \n");
            ExReleaseFastMutex(&wfse.FlushCacheMutex);
        }
    } else if (streamContext->allowedAccess == 0) {

    if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)) {
        if (debugCtx) {
            DbgPrint("PostCreate: new file open for rename %wZ \n", streamContext->fileName);
        }
    }

        if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE)) {
            if (debugCtx) {
                DbgPrint("PostCreate: new file open set delete flag %wZ \n", streamContext->fileName); 
            }
            streamContext->faction |= WFSE_FILE_DELETE;
        }

        if ((FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING) == FALSE) && 
	               (FLT_IS_IRP_OPERATION(Data))) {

            PFLT_DEFERRED_IO_WORKITEM WorkItem = NULL;
            if (FltObjects->FileObject->WriteAccess) {
                if (debugCtx)
                    DbgPrint("PostCreate: file open for write %wZ \n", streamContext->fileName); 
            }
            desiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
            if (debugCtx) {
                if (FlagOn(desiredAccess, FILE_WRITE_DATA)) {
                    DbgPrint("PostCreate: file open desiredAccess %x for write %wZ \n", streamContext->fileName); 
                }
            }
     
	    WorkItem = FltAllocateDeferredIoWorkItem();
	    if (WorkItem) {
	        status = FltQueueDeferredIoWorkItem(WorkItem,
					       Data,
					       wfseQueryPolicy,
					       CriticalWorkQueue,
					       CompletionContext);
	        if (NT_SUCCESS(status)) {
	            status = FLT_POSTOP_MORE_PROCESSING_REQUIRED;
	            return status;
	        } else {
	            if (status == STATUS_FLT_NOT_SAFE_TO_POST_OPERATION) {
		            DbgPrint("PostCreate: not safe to post create\n");
	            }
                    FltReleaseContext(streamContext);
	            return status;
	        }
            } else {
	        DbgPrint("PostCreate: no memory resource release sctx %x \n", streamContext);
                FltReleaseContext(streamContext);
	        return STATUS_NO_MEMORY;
           }
         }
     } else if (streamContext->allowedAccess > 0) {

    if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)) {
        if (debugCtx) {
            DbgPrint("PostCreate: cached file open for rename %wZ \n", streamContext->fileName);
        }
    }

        if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE)) {
            if (debugCtx) {
                DbgPrint("PostCreate: cached file open set delete flag %wZ \n", streamContext->fileName); 
            }
            streamContext->faction |= WFSE_FILE_DELETE;
        }

         streamContext->faction |= WFSE_FILE_CACHE_OPEN;

         status = wfseSetContext(Data->Iopb->TargetInstance,
			            Data->Iopb->TargetFileObject,
			            FLT_STREAMHANDLE_CONTEXT,
			            streamContext, &oldCtx);

         desiredAccess = streamContext->desiredAccess;
         if (debugCtx) {
             DbgPrint("PostCreate: allowedAccess cache hit streamCtx %p desiredAccess %x \n", streamContext, streamContext->desiredAccess);
         }

         if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE)) {
             if (debugCtx) {
                 DbgPrint("PostCreate: Delete file or rename file at here %wZ \n", streamContext->fileName);
             }
             doDelete = TRUE;
             streamContext->faction |= WFSE_FILE_DELETE;
         } else if (FlagOn(desiredAccess, DELETE) && streamContext->allowedAccess & DENY_RENAME) {
			 if (debugCtx) {
				 DbgPrint("PostCreate: rename file at here %wZ \n", streamContext->fileName);
			 }
			 doDelete = TRUE;
	 } else if ((streamContext->allowedAccess & DENY_CHANGE_SEC_ATTRS) && (desiredAccess & NL_CHANGE_SECURITY_ATTRIBUTE)) {
             doDelete = TRUE;
         } 
         if ((FlagOn(desiredAccess, READ_CONTROL|WRITE_DAC|WRITE_OWNER|ACCESS_SYSTEM_SECURITY|FILE_WRITE_DATA|FILE_WRITE_ATTRIBUTES)) || doDelete) { 
            if (FlagOn(desiredAccess, FILE_WRITE_DATA) && (streamContext->Officefile)) {
	        if (debugCtx) {
		    DbgPrint("PostCreate: modify Office file here %wZ \n", streamContext->fileName);
		}
                streamContext->allowedAccess |= WFSE_OPEN_WRITE;
                streamContext->faction |= WFSE_FILE_WRITE;
            }

#if 0
            status = wfseSetContext(Data->Iopb->TargetInstance,
			            Data->Iopb->TargetFileObject,
			            FLT_STREAMHANDLE_CONTEXT,
			            streamContext, &oldCtx);
     
              FltReleaseContext(streamContext);
#endif
              if (!NT_SUCCESS( status )) {
				DbgPrint("PostCreate: has allowedAccess set ctx failed status %x file %wZ \n", status, streamContext->fileName);
              }
          
              if (debugCtx) {
	          DbgPrint("PostCreate: allowedAccess cache hit set sctx %x allowedAccess %x file %wZ \n", streamContext, streamContext->allowedAccess, streamContext->fileName);
              }
          }
          FltReleaseContext(streamContext);

     }

    return FLT_POSTOP_FINISHED_PROCESSING;
}

VOID wfseQueryPolicy( _In_ PFLT_DEFERRED_IO_WORKITEM workItem,
                  _In_ PFLT_CALLBACK_DATA Data,
                  _In_ PVOID CompletionContext)
{
    ULONG idx;
    NTSTATUS status;
    ULONG replyLen;
    ULONG allowedAccess = 0;
    PNLWFSE_ACCESS_INFO msg = NULL;
    Pwfse_STREAMHANDLE_CONTEXT streamContext = NULL;
    Pwfse_INSTANCE_CONTEXT instanceContext;
    LARGE_INTEGER replyTimeout = {0};
    BOOLEAN inserted = FALSE;
    BOOLEAN permit = FALSE;
    PFLT_CONTEXT oldCtx = NULL;
    Pwfse_AVL_TREE avl;
    PERESOURCE avlLock;

    ASSERT( NULL != CompletionContext );
    FsRtlEnterFileSystem();

    streamContext = (Pwfse_STREAMHANDLE_CONTEXT)CompletionContext;
    msg = ExAllocateFromPagedLookasideList(&wfse.AccessBuffer);
    if (msg == NULL) {
        Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Data->IoStatus.Information = 0;
        goto QueryPolicyExit;
    } else {
        RtlZeroMemory(msg, WFSE_ACCESSINFO_BUFFER_SIZE);
        try {
            
            RtlCopyMemory(&msg->filePath,
			  streamContext->fileName->Buffer,
			  streamContext->fileName->Length);
            msg->filePathLength = streamContext->fileName->Length;
            RtlCopyMemory(&msg->sidString,
                          streamContext->sid->Buffer,
			  streamContext->sid->Length);
            msg->sidLength = streamContext->sid->Length;
            msg->ipaddr = streamContext->ipaddr;
            msg->faction |= WFSE_FILE_OPEN;
	    LONGLONG lSQueryTime  = GetTimeCount();
            replyTimeout.QuadPart = 3000000000; //one unit 100 nanosecond
	    replyLen = sizeof(ULONG);
	    status = FltSendMessage(wfse.FilterHandle,
					&wfse.ClientPort,
					msg,
					sizeof(NLWFSE_ACCESS_INFO),
			                &allowedAccess,	
					&replyLen,
					NULL);
	    LONGLONG lQueryTime = GetTimeCount() - lSQueryTime;
	    DbgPrint("==============> Query policy for file of %wZ , accessMask is %x, spend time is %d\n", streamContext->fileName, allowedAccess, lQueryTime);
	    if (status == STATUS_SUCCESS) {
                    if (allowedAccess & ALLOW_READ) {
                        if (debugCtx) {
		            DbgPrint("receiving reply allow open accessMask %x file %wZ \n", allowedAccess, streamContext->fileName);
                        }
                        permit = TRUE;
                    } else {
                        permit = FALSE;
                    }
                    ExFreeToPagedLookasideList(&wfse.AccessBuffer, msg);
	    } else if ((status == STATUS_TIMEOUT) ||
                       (status == STATUS_PORT_DISCONNECTED) ||
                       (status == STATUS_INSUFFICIENT_RESOURCES))  {

		   DbgPrint("reply Query Policy Message error status %x \n", status);
                   permit = FALSE;
                   allowedAccess |= WFSE_SENDMSG_ERROR;
                   ExFreeToPagedLookasideList(&wfse.AccessBuffer, msg);
	    }
	} except(EXCEPTION_EXECUTE_HANDLER) {

            Data->IoStatus.Status = GetExceptionCode();
            Data->IoStatus.Information = 0;
            ExFreeToPagedLookasideList(&wfse.AccessBuffer, msg);
            goto QueryPolicyExit;
        }
    }
        
    if (permit) {

        status = wfseSetContext(Data->Iopb->TargetInstance,
		                Data->Iopb->TargetFileObject,
			        FLT_STREAMHANDLE_CONTEXT,
			        streamContext, &oldCtx);

        streamContext->allowedAccess |= allowedAccess;
        streamContext->faction |= WFSE_FILE_OPEN;

        status = FltGetInstanceContext(Data->Iopb->TargetInstance, &instanceContext);
        if (!NT_SUCCESS( status )){
            DbgPrint("QueryPolicy: failed to get instance context.\n");
            Data->IoStatus.Status = status;
            Data->IoStatus.Information = 0;
            goto QueryPolicyExit;
        }

        idx = streamContext->hashidx;
        avl = instanceContext->hash[idx];
        avlLock = instanceContext->hash[idx]->Resource;

        wfseAcquireResourceExclusive(avlLock);
	if (wfse.CacheStale == FALSE) {

            if (debugCacheReset) {
                DbgPrint("QueryPolicy: instance %p avl %p lookaside %p \n", Data->Iopb->TargetInstance, avl, &avl->LookasideField); 
            }

            inserted = wfseCacheUpdate(Data, allowedAccess, CompletionContext);
	} else {
            if (debugCacheReset) {
                DbgPrint("QueryPolicy: CacheStale... ");
            }
            wfseReleaseResource(avlLock);
            FltReleaseContext(instanceContext);
            Data->IoStatus.Status = status;
            Data->IoStatus.Information = 0;
	    goto QueryPolicyExit;
	}
        wfseReleaseResource(avlLock);

        if (debugCtx) {
            DbgPrint("QueryPolicy: release instance ctx %x \n", instanceContext);
        }
        FltReleaseContext(instanceContext);

        if (inserted == FALSE) {
            if (debugCtx) {
	        DbgPrint("QueryPolicy: insert cache failed set ctx %x accessMask %x file %wZ \n", streamContext, allowedAccess, streamContext->fileName);
            }
            Data->IoStatus.Status = STATUS_SUCCESS;
            Data->IoStatus.Information = 0;
            goto QueryPolicyExit;
        } 

        if (debugCtx) {
	    DbgPrint("QueryPolicy: set ctx %x accessMask %x file %wZ \n", streamContext, allowedAccess, streamContext->fileName);
        }

#if 0
        if ((accessMask & DENY_RENAME)) {
            status = wfseSetContext(Data->Iopb->TargetInstance,
			     Data->Iopb->TargetFileObject,
			     FLT_STREAMHANDLE_CONTEXT,
			     streamContext, &oldCtx);

            if (!NT_SUCCESS( status )) {
                //TOD: test case error handling
	        DbgPrint("QueryPolicy: set ctx failed status %x file %wZ \n", status, streamContext->fileName);
            }
            if (debugCtx) {
	       DbgPrint("QueryPolicy: rename deny set ctx %x accessMask %x file %wZ \n", streamContext, accessMask, streamContext->fileName);
            }
        }
#endif
        Data->IoStatus.Status = STATUS_SUCCESS;
        Data->IoStatus.Information = 0;
    } else {

        streamContext->allowedAccess |= allowedAccess;

        if (allowedAccess > 0 && allowedAccess != 0x3f && !(allowedAccess & WFSE_SENDMSG_ERROR)) {

	    status = FltGetInstanceContext(Data->Iopb->TargetInstance, &instanceContext);
	    if (!NT_SUCCESS( status )){
	        DbgPrint("QueryPolicy: deny open failed to get instance context.\n");
	        goto erropen;
	    }

	    idx = streamContext->hashidx;
	    avl = instanceContext->hash[idx];
	    avlLock = instanceContext->hash[idx]->Resource;

	    wfseAcquireResourceExclusive(avlLock);
	    if (wfse.CacheStale == FALSE) {
	        if (debugCacheReset) {
		    DbgPrint("QueryPolicy: deny open instance %p avl %p lookaside %p \n", Data->Iopb->TargetInstance, avl, &avl->LookasideField); 
	        }
	        inserted = wfseCacheUpdate(Data, allowedAccess, CompletionContext);
	    } else {
	        if (debugCacheReset) {
	            DbgPrint("QueryPolicy: denied case CacheStale... ");
                }
	        wfseReleaseResource(avlLock);
	        FltReleaseContext(instanceContext);
                goto erropen;
	    }
	    wfseReleaseResource(avlLock);

	    if (debugCtx) {
	        DbgPrint("QueryPolicy: deny open release instance ctx %x \n", instanceContext);
	    }
	    FltReleaseContext(instanceContext);

	    if (inserted == FALSE) {
	        if (debugCtx) {
		    DbgPrint("QueryPolicy: deny open insert cache failed set ctx %x accessMask %x file %wZ \n", streamContext, allowedAccess, streamContext->fileName);
	        }
	        goto erropen;
	    } 

	    if (debugCtx) {
	        DbgPrint("QueryPolicy: deny open set ctx %x accessMask %x file %wZ \n", streamContext, allowedAccess, streamContext->fileName);
	    }
	}

erropen:
	if (debugCtx) {
	    DbgPrint("Policy: Query error open file %wZ \n", streamContext->fileName);
        }

        if ((Data->Iopb->TargetFileObject->Flags) & FO_HANDLE_CREATED) {
            if (debugCtx) {
                DbgPrint("QueryPolicy: cleanup file open.... \n");
            }
            wfseCleanupFile(Data->Iopb->TargetFileObject->DeviceObject,
                            Data->Iopb->TargetFileObject);
            wfseCloseFile(Data->Iopb->TargetFileObject->DeviceObject,
                            Data->Iopb->TargetFileObject);
                            
        } else {
            if (debugCtx) {
                DbgPrint("QueryPolicy: cancel file open.... \n");
            }
            FltCancelFileOpen(Data->Iopb->TargetInstance,
                                  Data->Iopb->TargetFileObject);

            FltReleaseContext(streamContext);
            streamContext = NULL;
        }

        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;
    }

QueryPolicyExit:
    if (streamContext) {
        if (debugCtx) {
            DbgPrint("QueryPolicy: release sctx %p \n", streamContext);
        }
        FltReleaseContext(streamContext);
    }
    FltCompletePendedPostOperation(Data);
    FltFreeDeferredIoWorkItem(workItem);
    FsRtlExitFileSystem();
    return;

}

NTSTATUS
wfseCleanupFile(PDEVICE_OBJECT DeviceObject, PFILE_OBJECT FileObject)
{
	PIRP irp;
	KEVENT 
	event; 
	PIO_STACK_LOCATION ioStackLocation;
	IO_STATUS_BLOCK ioStatusBlock;
	NTSTATUS status;
	ASSERT(DeviceObject != NULL);

	irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
	if (!irp) { 
		return STATUS_INSUFFICIENT_RESOURCES; 
	}

	irp->AssociatedIrp.SystemBuffer = NULL;
	irp->UserEvent = NULL;
	irp->UserIosb = &ioStatusBlock;
	irp->Tail.Overlay.Thread = PsGetCurrentThread();
	irp->Tail.Overlay.OriginalFileObject = FileObject;
	irp->RequestorMode = KernelMode;
	irp->Flags = IRP_SYNCHRONOUS_API;
	KeInitializeEvent(&event, NotificationEvent, FALSE); 

	ioStackLocation = IoGetNextIrpStackLocation(irp);
	ioStackLocation->MajorFunction = IRP_MJ_CLEANUP;
	ioStackLocation->FileObject = FileObject;
	IoSetCompletionRoutine(irp, wfseCleanupFileIoCompletion, &event, TRUE, TRUE, TRUE); 

	status = IoCallDriver(DeviceObject, irp);
	if (status == STATUS_PENDING) { 
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
	}
	ASSERT(KeReadStateEvent(&event) || !NT_SUCCESS(ioStatusBlock.Status)); 
	return ioStatusBlock.Status; 
}
	
NTSTATUS 
wfseCleanupFileIoCompletion(PDEVICE_OBJECT DeviceObject,
					PIRP Irp,
					PKEVENT SyncEvent)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    PIO_STACK_LOCATION  IrpSp;

    IrpSp = IoGetCurrentIrpStackLocation (Irp);

	ASSERT( Irp->UserIosb != NULL);
	*Irp->UserIosb = Irp->IoStatus;
	KeSetEvent(SyncEvent, IO_NO_INCREMENT, FALSE);
	IoFreeIrp(Irp);
	return STATUS_MORE_PROCESSING_REQUIRED; 
}


NTSTATUS
wfseCloseFile(PDEVICE_OBJECT DeviceObject,
				   PFILE_OBJECT FileObject)
{
	PIRP irp;
	KEVENT 
	event; 
	PIO_STACK_LOCATION ioStackLocation;
	IO_STATUS_BLOCK ioStatusBlock;
	NTSTATUS status;
	ASSERT(DeviceObject != NULL);

	irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
	if (!irp) { 
		return STATUS_INSUFFICIENT_RESOURCES; 
	}

	irp->AssociatedIrp.SystemBuffer = NULL;
	irp->UserEvent = NULL;
	irp->UserIosb = &ioStatusBlock;
	irp->Tail.Overlay.Thread = PsGetCurrentThread();
	irp->Tail.Overlay.OriginalFileObject = FileObject;
	irp->RequestorMode = KernelMode;
	irp->Flags = IRP_SYNCHRONOUS_API;
	KeInitializeEvent(&event, NotificationEvent, FALSE); 

	ioStackLocation = IoGetNextIrpStackLocation(irp);
	ioStackLocation->MajorFunction = IRP_MJ_CLOSE;
	ioStackLocation->FileObject = FileObject;
	IoSetCompletionRoutine(irp, wfseCloseFileIoCompletion, &event, TRUE, TRUE, TRUE); 
	status = IoCallDriver(DeviceObject, irp);
	if (status == STATUS_PENDING) { 
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
	}
	ASSERT(KeReadStateEvent(&event) || !NT_SUCCESS(ioStatusBlock.Status)); 
	return ioStatusBlock.Status; 
}

NTSTATUS 
wfseCloseFileIoCompletion(PDEVICE_OBJECT DeviceObject,
					PIRP Irp,
					PKEVENT SyncEvent)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    PIO_STACK_LOCATION  IrpSp;

    IrpSp = IoGetCurrentIrpStackLocation (Irp);

	ASSERT( Irp->UserIosb != NULL);
	*Irp->UserIosb = Irp->IoStatus;
	KeSetEvent(SyncEvent, IO_NO_INCREMENT, FALSE);
	IoFreeIrp(Irp);
	return STATUS_MORE_PROCESSING_REQUIRED; 
}

FLT_PREOP_CALLBACK_STATUS
wfsePreWrite (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
    NTSTATUS status;
    Pwfse_STREAMHANDLE_CONTEXT sctx = NULL;
    Pwfse_LOG_CONTEXT logctx = NULL;

    UNREFERENCED_PARAMETER( Data );

    status = FltGetStreamHandleContext( FltObjects->Instance,
                                        FltObjects->FileObject,
                                        &sctx );

    if (!NT_SUCCESS( status )) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if (sctx != NULL) {
        if (debugCtx) {
            DbgPrint("PreWrite: write file %wZ \n", sctx->fileName);
        }
        if (sctx->allowedAccess & WFSE_RESET_CACHE) {
            if (debugCtx) {
                DbgPrint("PreWrite: reset cache sctx %x \n", sctx);
            }
        }
            
        if (sctx->allowedAccess & DENY_WRITE) {
            if (debugCtx) {
                DbgPrint("PreWrite: deny write release sctx %x \n", sctx);
                if (sctx->fileName) {
                    DbgPrint("PreWrite: log deny write file %wZ \n", sctx->fileName);
                }
            }

            logctx = ExAllocateFromPagedLookasideList(&wfse.LogInfoBuffer);
            logctx->faction = 0; 
            logctx->fileName = sctx->fileName;
            logctx->sid = sctx->sid;
            logctx->ipaddr = sctx->ipaddr;
            logctx->faction |= WFSE_FILE_WRITE;
            wfseLogUpdate(logctx);
            ExFreeToPagedLookasideList(&wfse.LogInfoBuffer, logctx);

            FltReleaseContext(sctx);
            sctx = NULL;
            Data->IoStatus.Status = STATUS_ACCESS_DENIED;
            Data->IoStatus.Information = 0;
            return FLT_PREOP_COMPLETE;
        }
    }
    *CompletionContext = sctx;

    if (debugCtx && sctx)
       DbgPrint("PreWrite: sctx %x \n", sctx);
        
    return FLT_PREOP_SYNCHRONIZE;
}

FLT_POSTOP_CALLBACK_STATUS
wfsePostWrite (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    Pwfse_STREAMHANDLE_CONTEXT streamContext = NULL;
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    ULONG idx;
    NTSTATUS status;
    wfse_QUERY_CACHE item = {0};
    PwfseAvlNode node = NULL;;
    Pwfse_INSTANCE_CONTEXT instanceContext = NULL;

    PAGED_CODE();

    ASSERT( NULL != CompletionContext );
    streamContext = (Pwfse_STREAMHANDLE_CONTEXT) CompletionContext;

    if (streamContext) {

         if (!(streamContext->allowedAccess & WFSE_RESET_CACHE)) {
             status = FltGetInstanceContext(FltObjects->Instance, &instanceContext);
             if (!NT_SUCCESS( status )){
                 DbgPrint("PostWrite: failed to get instance context release sctx %x \n", streamContext);
                 FltReleaseContext(streamContext);
             return status;
             }
             idx = streamContext->hashidx;
             wfseAcquireResourceExclusive(instanceContext->hash[idx]->Resource);
             item.key = streamContext->key;
             node = wfseAvlSearchNode(instanceContext->hash[idx], &item);
             if (node) {
	         if (debugCache) {
	             DbgPrint("PostWrite: delete node key %x \n", node->key);
	         }
                 wfseAvlDeleteNode(instanceContext->hash[idx], node, FltObjects->Instance);
             }
             wfseReleaseResource(instanceContext->hash[idx]->Resource);
             FltReleaseContext(instanceContext);
	     if (debugCtx) {
	         DbgPrint("PostWrite: log write file %wZ\n", streamContext->fileName);
	     }
             streamContext->faction |= WFSE_FILE_WRITE;
         }

         FltReleaseContext(streamContext);
         if (debugCtx) {
            DbgPrint("PostWrite: release sctx %x file %wZ \n", streamContext, streamContext->fileName);
         }
    }

    return FLT_POSTOP_FINISHED_PROCESSING;

}

FLT_PREOP_CALLBACK_STATUS
wfsePreSetInfo(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
    NTSTATUS status;
    Pwfse_LOG_CONTEXT logctx = NULL;
    Pwfse_STREAMHANDLE_CONTEXT sctx = NULL;
    Pwfse_INSTANCE_CONTEXT instanceContext = NULL;
    BOOLEAN denyChgAttrs, denyRename, denyDelete;
    wfse_QUERY_CACHE item = {0};
    PwfseAvlNode entry = NULL;
    BOOLEAN doDelete = FALSE;
    BOOLEAN chgAttrs = FALSE;
    BOOLEAN doRename = FALSE; 
    denyChgAttrs = FALSE;
    denyRename = FALSE;
    denyDelete = FALSE;
    BOOLEAN busy;

    UNREFERENCED_PARAMETER( FltObjects );

    PAGED_CODE();

    if (debugPreSetInfo) {
        if (FltObjects->FileObject->FileName.Buffer)
            DbgPrint("PreSet: file %wZ \n", FltObjects->FileObject->FileName);
    }

    if (wfse.UserProcess == PsGetCurrentProcess()) {
        //DbgPrint("PreSet: file %wZ \n", FltObjects->FileObject->FileName);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    status = FltGetStreamHandleContext( Data->Iopb->TargetInstance,
                                  Data->Iopb->TargetFileObject,
                                  &sctx );
    if (!NT_SUCCESS(status)) {
        if (debugCtx) {
            if (FltObjects->FileObject) {
                if (debugPreSetInfo) {
                    DbgPrint("PreSetInfo: get sctx error %x file %wZ \n", status, FltObjects->FileObject->FileName);
                }
            }
        }
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    } 

    if (debugCtx) {
        if (sctx) 
            DbgPrint("PreSetInfo: sctx %p file %wZ \n", sctx, sctx->fileName);
    }
    
    switch (Data->Iopb->Parameters.SetFileInformation.FileInformationClass) {

        case FileBasicInformation:
            chgAttrs = TRUE;
            sctx->faction |= WFSE_FILE_CHANGE_ATTRS;
            if (debugCtx) {
                if (sctx) 
                    DbgPrint("PreSetInfo: chg attrs file %wZ \n", sctx->fileName);
            }
            break;
        case FileRenameInformation:
            doRename = TRUE;
            sctx->faction |= WFSE_FILE_RENAME;
            if (debugCtx) {
                if (sctx) 
                    DbgPrint("PreSetInfo: rename file %wZ \n", sctx->fileName);
            }
            break;
        case FileDispositionInformation:
            doDelete = TRUE;
            sctx->faction |= WFSE_FILE_DELETE;
            if (debugCtx) {
                if (sctx) 
                    DbgPrint("PreSetInfo: delete file %wZ \n", sctx->fileName);
            }

        default:
            break;
    }

    if (debugCtx) {
            DbgPrint("PreSetInfo: instance %x fileobject %x get sctx %x passin sctx %x \n",
                  Data->Iopb->TargetInstance,
                  Data->Iopb->TargetFileObject,
                  sctx, CompletionContext);
    }
    if (sctx != NULL) {
        if (sctx->allowedAccess & DENY_RENAME) {
                denyRename = TRUE;
                //sctx->faction |= WFSE_FILE_RENAME;
                logctx = ExAllocateFromPagedLookasideList(&wfse.LogInfoBuffer);
                logctx->faction = 0; 
                logctx->faction |= WFSE_FILE_RENAME; 
                logctx->fileName = sctx->fileName;
                logctx->sid = sctx->sid;
                logctx->ipaddr = sctx->ipaddr;
                wfseLogUpdate(logctx);
                ExFreeToPagedLookasideList(&wfse.LogInfoBuffer, logctx);
      
                if (debugCtx) {
                    DbgPrint("PreSetInfo: sctx %p log deny rename file %wZ \n", sctx, sctx->fileName);
                }

                status = FltGetInstanceContext(FltObjects->Instance, &instanceContext);

                if (!NT_SUCCESS( status )){
	             DbgPrint("PreSetInfo: failed to get instance context release sctx %x \n", sctx);
                     FltReleaseContext(sctx);
	             return status;
                }
                if (debugCtx) {
                    if (sctx) 
                        DbgPrint("PreSetInfo: allowedAccess deny rename file %wZ \n", sctx->fileName);
                }

                item.ipaddr = sctx->ipaddr;
                item.key = sctx->key;
                item.allowedAccess = sctx->allowedAccess;
                
                wfseAcquireResourceExclusive(instanceContext->hash[sctx->hashidx]->Resource);
                entry = wfseAvlSearchNode(instanceContext->hash[sctx->hashidx], &item); 
                if (entry != NULL) {
                    if (debugCache) {
                        DbgPrint("PreSetInfo: set rename flag in cache item %wZ \n", sctx->fileName); 
                    }
                    entry->denyRename = TRUE;
                }
                wfseReleaseResource(instanceContext->hash[sctx->hashidx]->Resource );
                FltReleaseContext(instanceContext);

        } 
        if (sctx->allowedAccess & DENY_CHANGE_ATTRS) {
                denyChgAttrs = TRUE;
                sctx->faction |= WFSE_FILE_CHANGE_ATTRS;
                if (debugCtx) {
                    if (sctx) 
                        DbgPrint("PreSetInfo: allowedAccess deny chgattrs file %wZ \n", sctx->fileName);
                }
        } 
        if (sctx->allowedAccess & DENY_DELETE) {
                denyDelete = TRUE;
                sctx->faction |= WFSE_FILE_DELETE;
                if (debugCtx) {
                    if (sctx) 
                        DbgPrint("PreSetInfo: allowedAccess deny delete file %wZ \n", sctx->fileName);
                }
        } else if (doDelete) {
            busy = (InterlockedIncrement( &sctx->NumOps ) > 1);
            if (busy) {
                if (debugCtx) {
                    DbgPrint("PreSetInfo: is busy release sctx %x delete file %wZ \n", sctx, sctx->fileName);
                }
                FltReleaseContext(sctx);
                sctx = NULL;
                return FLT_PREOP_SUCCESS_NO_CALLBACK;
            }
        }
    }


    if ((denyChgAttrs && chgAttrs) || (denyRename && doRename) ||
        (denyDelete && doDelete)) {
        if (debugCtx) {
            DbgPrint("PreSetInfo: chg attr rename delete access denied file %wZ \n", sctx->fileName);
        }

        FltReleaseContext(sctx);
        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;
        sctx = NULL;
        return FLT_PREOP_COMPLETE;
    }
    *CompletionContext = sctx;
    return FLT_PREOP_SYNCHRONIZE;
}

FLT_POSTOP_CALLBACK_STATUS
wfsePostSetInfo(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING - we never do any sort of asynchronous
        processing here.

--*/
{
    Pwfse_STREAMHANDLE_CONTEXT streamContext;
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    ULONG idx;
    NTSTATUS status;
    wfse_QUERY_CACHE item = {0};
    PwfseAvlNode node = NULL;
    Pwfse_INSTANCE_CONTEXT instanceContext = NULL;

    PAGED_CODE();

    ASSERT( NULL != CompletionContext );
    streamContext = (Pwfse_STREAMHANDLE_CONTEXT) CompletionContext;
    if (debugCtx) {
        if (streamContext) {
                DbgPrint("PostSetInfo: sctx %x file %wZ \n", streamContext, streamContext->fileName);
        }
    }

    if (Data->Iopb->Parameters.SetFileInformation.FileInformationClass
            == FileDispositionInformation) {
        if (NT_SUCCESS( Data->IoStatus.Status )) {

        if (debugCtx) {
            DbgPrint("PostSetInfo: incremant delnumOps file %wZ \n", streamContext, streamContext->fileName);
        }
        streamContext->SetDisp = ((PFILE_DISPOSITION_INFORMATION)
                                  Data->Iopb->Parameters.SetFileInformation.InfoBuffer)->DeleteFile;
        }
        InterlockedDecrement( &streamContext->NumOps );

    }

    status = FltGetInstanceContext(FltObjects->Instance, &instanceContext);
    if (!NT_SUCCESS( status )){
        DbgPrint("PostSetInfo: failed to get instance context release sctx %x \n", streamContext);
        FltReleaseContext(streamContext);
        return status;
    }

    idx = streamContext->hashidx;
    wfseAcquireResourceExclusive(instanceContext->hash[idx]->Resource);
    item.key = streamContext->key;
    node = wfseAvlSearchNode(instanceContext->hash[idx], &item);
    if (node) {
        if (debugCache) {
            DbgPrint("PostSetInfo: context key %x delete node key %x \n", item.key, node->key);
        }
        wfseAvlDeleteNode(instanceContext->hash[idx], node, FltObjects->Instance);
    }
    wfseReleaseResource(instanceContext->hash[idx]->Resource);

    FltReleaseContext(instanceContext);
    FltReleaseContext(streamContext);
    if (debugCtx) {
        DbgPrint("PostSetInfo: release sctx %x file %wZ \n", streamContext, streamContext->fileName);
    }
    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
wfsePreSetSecurity(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext
    )
{
    NTSTATUS status;
    Pwfse_LOG_CONTEXT logctx = NULL;
    Pwfse_STREAMHANDLE_CONTEXT streamContext = NULL;

    UNREFERENCED_PARAMETER( Data );

    status = FltGetStreamHandleContext( FltObjects->Instance,
                                        FltObjects->FileObject,
                                        &streamContext );

    if (!NT_SUCCESS( status )) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if (streamContext) {
        if (debugCtx) {
            DbgPrint("PreSetSec: PreSetSec file %wZ \n", streamContext->fileName);
        }
        streamContext->faction |= WFSE_FILE_CHANGE_SECS;;

        if (streamContext->allowedAccess & DENY_CHANGE_SEC_ATTRS) {
            if (debugCtx) {
                DbgPrint("PreSetSec: sctx %p log deny PreSetSec file %wZ \n", streamContext, streamContext->fileName);
            }

            logctx = ExAllocateFromPagedLookasideList(&wfse.LogInfoBuffer);
            logctx->faction = 0; 
            logctx->fileName = streamContext->fileName;
            logctx->sid = streamContext->sid;
            logctx->ipaddr = streamContext->ipaddr;
            logctx->faction |= WFSE_FILE_CHANGE_SECS;
            wfseLogUpdate(logctx);
            ExFreeToPagedLookasideList(&wfse.LogInfoBuffer, logctx);


            FltReleaseContext(streamContext);
            streamContext = NULL;
            Data->IoStatus.Status = STATUS_ACCESS_DENIED;
            Data->IoStatus.Information = 0;
            if (debugCtx)
                DbgPrint("PreSetSec: deny access release sctx %x \n", streamContext);
            return FLT_PREOP_COMPLETE;
        }
    }
    *CompletionContext = streamContext;

    if (*CompletionContext) {
        DbgPrint("PreSetSecurity: track set security in Post sctx %x \n", streamContext);
        return FLT_PREOP_SYNCHRONIZE;
    }
    if (debugCtx)
        DbgPrint("PreSetSec: pass thru release sctx %x \n", streamContext);

    return FLT_PREOP_SUCCESS_NO_CALLBACK;

}

FLT_POSTOP_CALLBACK_STATUS
wfsePostSetSecurity(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    ULONG idx;
    NTSTATUS status;
    wfse_QUERY_CACHE item = {0};
    PwfseAvlNode node = NULL;;
    Pwfse_INSTANCE_CONTEXT instanceContext = NULL;
    SECURITY_INFORMATION se;
    PSECURITY_DESCRIPTOR sedesp;
    PFLT_IO_PARAMETER_BLOCK Iopb = Data->Iopb;
    PFLT_PARAMETERS Params = &Iopb->Parameters;
    se = Params->SetSecurity.SecurityInformation;
    sedesp = (PSECURITY_DESCRIPTOR)Params->SetSecurity.SecurityDescriptor;
    
    Pwfse_STREAMHANDLE_CONTEXT streamContext = NULL;

    PAGED_CODE();

    ASSERT(NULL != CompletionContext);
    streamContext = (Pwfse_STREAMHANDLE_CONTEXT)CompletionContext;
    if (debugCtx) {
        if (streamContext) {
            DbgPrint("PostSetSecurity: sctx %x file %wZ se %x \n", streamContext, streamContext->fileName, se);
        }
    }

    status = FltGetInstanceContext(FltObjects->Instance, &instanceContext);
    if (!NT_SUCCESS( status )){
        DbgPrint("PostSec: failed to get instance context release sctx %x \n", streamContext);
        FltReleaseContext(streamContext);
        return status;
    }
    idx = streamContext->hashidx;
    wfseAcquireResourceExclusive(instanceContext->hash[idx]->Resource);
    item.key = streamContext->key;
    node = wfseAvlSearchNode(instanceContext->hash[idx], &item);
    if (node) {
        if (debugCache) {
            DbgPrint("PosetSec: ctx key %x delete node key %x \n", item.key, node->key);
        }
        wfseAvlDeleteNode(instanceContext->hash[idx], node, FltObjects->Instance);
    }
    wfseReleaseResource(instanceContext->hash[idx]->Resource);

    FltReleaseContext(streamContext);
    if (debugCtx) { 
        DbgPrint("PostSetSecurity: release sctx %x \n", streamContext);
    }
    FltReleaseContext(instanceContext);

    return FLT_POSTOP_FINISHED_PROCESSING;

}


FLT_PREOP_CALLBACK_STATUS
wfsePreCleanup(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
    ULONG idx;
    Pwfse_STREAMHANDLE_CONTEXT streamContext;
    Pwfse_INSTANCE_CONTEXT instanceContext = NULL;
    Pwfse_STREAMHANDLE_CONTEXT sctx;
    Pwfse_LOG_CONTEXT logctx = NULL;
    LARGE_INTEGER curSystemTime;
    PwfseAvlNode node = NULL;;
    wfse_QUERY_CACHE item = {0};
    TIME_FIELDS logtime, ltime; 
    LARGE_INTEGER ctime;
    NTSTATUS status;
    BOOLEAN skipLog = FALSE;
    

    //UNREFERENCED_PARAMETER( FltObjects );

    PAGED_CODE();

    status = FltGetStreamHandleContext( Data->Iopb->TargetInstance,
                                  Data->Iopb->TargetFileObject,
                                  &sctx );
    if (NT_SUCCESS(status) && sctx != NULL) {
        if (debugCtx) {
            DbgPrint("PreCleanup: instance %x get sctx %x passin sctx %x file %wZ \n",
                  Data->Iopb->TargetInstance,
                  sctx, CompletionContext, sctx->fileName);
        }
        streamContext = sctx;
        if (sctx->faction != 0) {
            if (debugCtx) {
                DbgPrint("PreCleanup: call updateLog sctx %x faction %d file %wZ \n", sctx, sctx->faction, sctx->fileName);
            }

            if ((sctx->faction & WFSE_FILE_CACHE_OPEN) && (sctx->faction & WFSE_FILE_WRITE)) {
                skipLog = FALSE;
                if (debugCtx) {
                    DbgPrint("PreCleanup: call updateLog cached open write sctx %x faction %d file %wZ \n", sctx, sctx->faction, sctx->fileName);
                }
            } else if (sctx->faction == WFSE_FILE_CACHE_OPEN) {
                status = FltGetInstanceContext(FltObjects->Instance, &instanceContext);
                if (!NT_SUCCESS( status )){
                    DbgPrint("PreClean: failed to get instance context release sctx %x \n", streamContext);
                    FltReleaseContext(streamContext);
	            return status;
                }
                idx = streamContext->hashidx;
                wfseAcquireResourceExclusive(instanceContext->hash[idx]->Resource);
                item.key = streamContext->key;
                node = wfseAvlSearchNode(instanceContext->hash[idx], &item);
                if (node) {
                    if (debugCtx) {
                        DbgPrint("PreCleanup: cached logTime %x key %x \n", node->logTime, item.key);
                    }
                    if (node->logTime.QuadPart == 0) { 
                        KeQuerySystemTime(&curSystemTime);
                        ExSystemTimeToLocalTime(&curSystemTime, &ctime); 
                        node->logTime = ctime;
                        if (debugCtx) {
                            DbgPrint("PreCleanup: init log time cached open %wZ \n", sctx->fileName);
                        }
                    } else {
                        KeQuerySystemTime(&curSystemTime);
                        ExSystemTimeToLocalTime(&curSystemTime, &ctime); 
                        RtlTimeToTimeFields(&ctime, &ltime);
                        RtlTimeToTimeFields(&node->logTime, &logtime);

	                if ((ltime.Day == logtime.Day) && (ltime.Hour == logtime.Hour) && (ltime.Minute == logtime.Minute) && (ltime.Second == logtime.Second)) {
                            if (debugCtx) {
                                DbgPrint("PreCleanup: skip log deny cached open %wZ \n", sctx->fileName);
                            }
                            skipLog = TRUE;
                        } else if ((ltime.Day == logtime.Day) && (ltime.Hour == logtime.Hour) && (ltime.Minute == logtime.Minute) && (ltime.Second != logtime.Second)) {
                                if ((ltime.Second - logtime.Second) < 60) {
                                    skipLog = TRUE;
                                }
                                if (debugCtx) {
                                    DbgPrint("PreCleanup: skip log deny different second cached open %wZ \n", sctx->fileName);
                                }
                         }
                    }
                }
                wfseReleaseResource(instanceContext->hash[idx]->Resource);
                FltReleaseContext(instanceContext);
            } else if (sctx->faction == WFSE_FILE_OPEN) {
                skipLog = TRUE;
                if (debugCtx) {
                   DbgPrint("PreCleanup: sctx %p skip log cached open file %wZ \n", sctx, sctx->fileName);
                }
            } 

            if (!skipLog) {
                if (debugCtx) {
                   DbgPrint("PreCleanup: sctx %p log action %d file %wZ \n", sctx, sctx->faction, sctx->fileName);
                }

                logctx = ExAllocateFromPagedLookasideList(&wfse.LogInfoBuffer);
                logctx->fileName = sctx->fileName;
                logctx->sid = sctx->sid;
                logctx->ipaddr = sctx->ipaddr;
                logctx->faction = sctx->faction;
                wfseLogUpdate(logctx);
                ExFreeToPagedLookasideList(&wfse.LogInfoBuffer, logctx);
            }
        }

        if (streamContext != NULL) {
            if (streamContext->allowedAccess & WFSE_RESET_CACHE) {
                ExAcquireFastMutex(&wfse.FlushCacheMutex);
		if (debugCacheReset) {
		    DbgPrint("PreCleanup: Reset cache...\n");
                }
                KeSetEvent(&wfse.FlushCacheEvent, IO_NO_INCREMENT, FALSE);
                ExReleaseFastMutex(&wfse.FlushCacheMutex);
	    }
            *CompletionContext = (PVOID)streamContext;
            return FLT_PREOP_SYNCHRONIZE;
        }
   }

   return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS
wfsePostCleanup(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING - we never do any sort of asynchronous
        processing here.

--*/
{
    ULONG idx;
    NTSTATUS status;
    PwfseAvlNode node = NULL;;
    wfse_QUERY_CACHE item = {0};
    Pwfse_STREAMHANDLE_CONTEXT streamContext = NULL;
    Pwfse_INSTANCE_CONTEXT instanceContext = NULL;
    //UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    // assert we're not draining.
    ASSERT( !FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING ) );

    // pass from pre-callback to post-callback
    ASSERT( NULL != CompletionContext );
    streamContext = (Pwfse_STREAMHANDLE_CONTEXT) CompletionContext;

    if (streamContext) {
        if (debugCtx) { 
            DbgPrint("PostCleanup: my Instance %x release sctx %x \n", Data->Iopb->TargetInstance, streamContext);
        }

        if (streamContext->allowedAccess & WFSE_OPEN_WRITE) {
            if (debugCtx) {
                DbgPrint("PostCleanup: instance %x sctx %p Cleanup write cached Office file %wZ \n", Data->Iopb->TargetInstance, streamContext, streamContext->fileName);
            }

            status = FltGetInstanceContext(FltObjects->Instance, &instanceContext);
            if (!NT_SUCCESS( status )){
                DbgPrint("PostClean: failed to get instance context release sctx %x \n", streamContext);
                FltReleaseContext(streamContext);
	        return status;
            }
            idx = streamContext->hashidx;
            wfseAcquireResourceExclusive(instanceContext->hash[idx]->Resource);
            item.key = streamContext->key;
            node = wfseAvlSearchNode(instanceContext->hash[idx], &item);
            if (node) {
                if (debugCache) {
                    DbgPrint("PostCleanup: context key %x delete node key %x \n", item.key, node->key);
                }
                wfseAvlDeleteNode(instanceContext->hash[idx], node, FltObjects->Instance);
            }
            wfseReleaseResource(instanceContext->hash[idx]->Resource);

            FltReleaseContext(instanceContext);
        }
        FltReleaseContext( streamContext );
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
}

NTSTATUS wfseLogUpdate(Pwfse_LOG_CONTEXT sctx)
{
    NTSTATUS status;
    PNLWFSE_ACCESS_INFO msg = NULL;
    ULONG replyLen;
    ULONG allowedAccess = 0;
    msg = ExAllocateFromPagedLookasideList(&wfse.AccessBuffer);
    if (msg == NULL) {
        DbgPrint("PreCleanup: no lookaside memory \n");
	return STATUS_INSUFFICIENT_RESOURCES;
    } else {
	RtlZeroMemory(msg, WFSE_ACCESSINFO_BUFFER_SIZE);
	try {
	    
	    RtlCopyMemory(&msg->filePath,
			  sctx->fileName->Buffer,
			  sctx->fileName->Length);
	    msg->filePathLength = sctx->fileName->Length;
	    RtlCopyMemory(&msg->sidString,
			  sctx->sid->Buffer,
			  sctx->sid->Length);
	    msg->sidLength = sctx->sid->Length;
	    msg->ipaddr = sctx->ipaddr;
	    msg->faction = sctx->faction;
	    replyLen = sizeof(ULONG);
	    status = FltSendMessage(wfse.FilterHandle,
				    &wfse.ClientPort,
				    msg,
				    sizeof(NLWFSE_ACCESS_INFO),
				    &allowedAccess,	
				    &replyLen,
				    NULL);
	    if (status == STATUS_SUCCESS) {
		if (debugCtx) {
	            DbgPrint("==> Query policy action %d  ret %d file log %wZ \n", sctx->faction, allowedAccess, sctx->fileName);
		}
		ExFreeToPagedLookasideList(&wfse.AccessBuffer, msg);
	    } else  {
		  DbgPrint("reply Query Policy log Message error status %x \n", status);
		  ExFreeToPagedLookasideList(&wfse.AccessBuffer, msg);
	    }
         } except(EXCEPTION_EXECUTE_HANDLER) {
  	     status = GetExceptionCode();
	     DbgPrint("reply Query Policy log Message exception status %x \n", status);
		        ExFreeToPagedLookasideList(&wfse.AccessBuffer, msg);
         }
         return status;
    }
}

NTSTATUS
wfseSetConfiguration (
    _In_ PUNICODE_STRING RegistryPath
    ) 
/*++

Arguments:

    RegistryPath - The path key passed to the driver during DriverEntry.

Return Value:

    STATUS_SUCCESS if the function completes successfully.  Otherwise a valid
    NTSTATUS code is returned.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DriverRegKey = NULL;
    UNICODE_STRING ValueName;
    BOOLEAN CloseHandle = FALSE;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + wfse_MAX_PATH_LENGTH  * sizeof(WCHAR)];
    PKEY_VALUE_PARTIAL_INFORMATION Value = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
    ULONG ValueLength = sizeof(Buffer);
    ULONG ResultLength;

    //
    //  Open the driver registry key.
    //

    InitializeObjectAttributes( &Attributes,
                                RegistryPath,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = ZwOpenKey( &DriverRegKey,
                        KEY_READ,
                        &Attributes );

    if (!NT_SUCCESS( Status )) {

        goto SetConfigurationCleanup;
    }

    CloseHandle = TRUE;
    
    //
    //  Query the debug level
    //

    RtlInitUnicodeString( &ValueName, wfse_KEY_NAME_DEBUG_LEVEL );
    
    Status = ZwQueryValueKey( DriverRegKey,
                              &ValueName,
                              KeyValuePartialInformation,
                              Value,
                              ValueLength,
                              &ResultLength );
    
    if (NT_SUCCESS( Status )) {

        wfse.DebugLevel = *(PULONG)(Value->Data);
    }

    
    //
    //  Query the queue time delay
    //

 
/*
    RtlInitUnicodeString( &ValueName, wfse_KEY_NAME_DELAY );

    Status = ZwQueryValueKey( DriverRegKey,
                              &ValueName,
                              KeyValuePartialInformation,
                              Value,
                              ValueLength,
                              &ResultLength );

    if (NT_SUCCESS( Status )) {

        if (Value->Type != REG_DWORD) {

            Status = STATUS_INVALID_PARAMETER;
            goto SetConfigurationCleanup;
        }
        
        wfse.TimeDelay = (LONGLONG)(*(PULONG)(Value->Data));
        
    }
*/
  
    //
    //  Ignore errors when looking for values in the registry.
    //  Default values will be used.
    //
    
    Status = STATUS_SUCCESS;
    
SetConfigurationCleanup:

    if (CloseHandle) {

        ZwClose( DriverRegKey );
    }

    return Status;

}

VOID
wfseFreeList(
    )
{
    PAGED_CODE();

    ExDeletePagedLookasideList(&wfse.TmpBuffer);
    ExDeletePagedLookasideList(&wfse.SidBuffer);
    ExDeletePagedLookasideList(&wfse.StringPtr);
    ExDeletePagedLookasideList(&wfse.AccessBuffer);
    ExDeletePagedLookasideList(&wfse.ReqInfoBuffer);
    ExDeletePagedLookasideList(&wfse.LogInfoBuffer);
}

NTSTATUS
wfsePortConnect (
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID *ConnectionCookie
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( ServerPortCookie );
    UNREFERENCED_PARAMETER( SizeOfContext);
    UNREFERENCED_PARAMETER( ConnectionCookie = NULL );

    FLT_ASSERT( wfse.ClientPort == NULL );
    FLT_ASSERT( wfse.UserProcess == NULL );
    PWFSE_PORT_CONTEXT pctx;
    pctx = (PWFSE_PORT_CONTEXT) ConnectionContext;

    //
    //  Set the user process and port. In a production filter it may
    //  be necessary to synchronize access to such fields with port
    //  lifetime. For instance, while filter manager will synchronize
    //  FltCloseClientPort with FltSendMessage's reading of the port 
    //  handle, synchronizing access to the UserProcess would be up to
    //  the filter.
    //

    wfse.UserProcess = PsGetCurrentProcess();
    wfse.ClientPort = ClientPort;
    wfse.ProtectionOn = TRUE;
    wfse.pcid = pctx->pcid;

    DbgPrint( "!!! nlwfse.sys --- connected, port=0x%p\n", ClientPort );

    return STATUS_SUCCESS;
}

VOID
wfsePortDisconnect(
     _In_opt_ PVOID ConnectionCookie
     )
{
    UNREFERENCED_PARAMETER( ConnectionCookie );

    PAGED_CODE();


    //
    //  Close our handle to the connection: note, since we limited max connections to 1,
    //  another connect will not be allowed until we return from the disconnect routine.
    //

    FltCloseClientPort( wfse.FilterHandle, &wfse.ClientPort );

    wfse.UserProcess = NULL;
    //DbgPrint( "!!! nlwfse.sys --- disconnected, Protection is off port=0x%p\n", wfse.ClientPort );
    wfse.ProtectionOn = FALSE;
}

