
/*++

Copyright (c) 2016  Nextlabs.

Module Name:

    nlwfsedll.h

*/


// Inclusion guard
#ifndef _NLWFSEDLL_H_
#define _NLWFSEDLL_H_

// Make our life easier, if DLL_EXPORT is defined in a file then DECLDIR will do an export
// If it is not defined DECLDIR will do an import
#if defined DLL_EXPORT
#define DECLDIR __declspec(dllexport)
#else
#define DECLDIR __declspec(dllimport)
#endif

#define NLWFSE_DEFAULT_REQUEST_COUNT       16
#define NLWFSE_DEFAULT_THREAD_COUNT        2
#define NLWFSE_MAX_THREAD_COUNT            16
#define HILONG(ll) (ll>> 32 & LONG_MAX)
#define LOLONG(ll) ((long)(ll))
#define MAXVOLUMES 16
#define MAXVOLUMELEN 256
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16384
#define MAX_SD_SIZE 3220    
#define MAX_SD_VOLUME 16
#define WFSE_ACTION_SIZE 32

#define NLWFSE_SERVICE_NAME TEXT("nlwfse")

#define NLWFSE_REPLY_MESSAGE_SIZE (sizeof(FILTER_REPLY_HEADER) + sizeof(ULONG))

BOOL sdReset = FALSE;

typedef struct _NLWFSE_THREAD_CONTEXT {
    HANDLE Port;
    HANDLE Completion;
    IVssBackupComponents *pBackup[MAX_SD_VOLUME];
    HANDLE wfseTimer[MAX_SD_VOLUME];
    LPCRITICAL_SECTION flock;
    LPCRITICAL_SECTION sdlock;
    DWORD threadCnt;
    HANDLE threads[NLWFSE_MAX_THREAD_COUNT];
} NLWFSE_THREAD_CONTEXT, *PNLWFSE_THREAD_CONTEXT;

NLWFSE_THREAD_CONTEXT Tctx;

struct _NLWFSE_VSS_DEVICE {
  wstring devName;
  wstring sdevName;
  VSS_ID snapshotSetId;
};

typedef _NLWFSE_VSS_DEVICE nlwfseDev;

typedef struct _NLWFSE_SD_CONTEXT {
  TCHAR volume[MAXVOLUMELEN];
  TCHAR devName[MAXVOLUMELEN];
  IVssBackupComponents *pBackup[MAX_SD_VOLUME];
  DWORD vsid;
  LARGE_INTEGER wtime;
  SYSTEMTIME  ResetTime;
} NLWFSE_SD_CONTEXT, *PNLWFSE_SD_CONTEXT;

NLWFSE_SD_CONTEXT nlwfsesd[MAX_SD_VOLUME];

std::map<wstring, nlwfseDev> vsinfo;

typedef struct _NLWFSE_VSS_CONTEXT {
    std::map<wstring, wstring> v_list;
    DWORD vcnt;
} NLWFSE_VSS_CONTEXT, *PNLWFSE_VSS_CONTEXT;

NLWFSE_VSS_CONTEXT Vctx;

#pragma pack(1)
typedef struct _WFSE_MESSAGE {
    FILTER_MESSAGE_HEADER MessageHeader;
    NLWFSE_ACCESS_INFO accessInfo;
    OVERLAPPED Ovlp;
} WFSE_MESSAGE, *PWFSE_MESSAGE;

typedef struct _WFSE_REPLY_MESSAGE {
    FILTER_REPLY_HEADER ReplyHeader;
    //NLWFSE_REPLY Reply;
    ULONG accessMask;
} WFSE_REPLY_MESSAGE, *PWFSE_REPLY_MESSAGE;

// Specify "C" linkage to get rid of C++ name mangeling
extern "C"
{
    DECLDIR int  PluginEntry(void **);
    DECLDIR int PluginUnload(void *);

}

extern HMODULE nlwfseInstance;

CRITICAL_SECTION logfileLock;
CRITICAL_SECTION sdLock;
HANDLE SdResetMutex;
BOOL SdReset;

wchar_t cesdkPath[1024];

DWORD
nlwfseWorker(
    _In_ PNLWFSE_THREAD_CONTEXT Context
    );

//TODO no cesdk

//#if 0
void QueryAllowedAccess(PWFSE_MESSAGE qmsg,
                        CEAttributes* sattrs,
                        wchar_t *action,
                        ULONG *access,
                        _int64 *ticks);
//#endif

//int makeSd(TCHAR *vn, TCHAR *devname);
//int makeSd(IVssBackupComponents **ptr, TCHAR *vn, TCHAR *devname);
int makeSd(IVssBackupComponents **ptr, PNLWFSE_SD_CONTEXT Context);

void CreateShadowDevice();
void wfseQuerySnapshotSet();
DWORD InstallNlwfseDriver();
void SetSdSizeRegKey(); 
//BOOL wfseDeleteSnapshotSet();
BOOL wfseLookupShadowDevices(VSS_ID sdevId);
void wfseDeleteShadowRegistry();
void wfseLookupSnapshotSet(PNLWFSE_SD_CONTEXT Context);
BOOL wfseDeleteSnapshotSet(VSS_ID snapshotSetID);
BOOL wfseStopDependentServices(SC_HANDLE schMgr, SC_HANDLE schSrv);
DWORD wfseStopDriver(); 
bool GetCesdkPath();

DWORD WINAPI ShadowDeviceReset(
    _In_ PNLWFSE_SD_CONTEXT Context
);

void wfseGetVol();
std::string ws2s(const std::wstring &s);
void ReleaseInterface (IUnknown* unkn);
WCHAR* Guid2Wchar(VSS_ID id);
std::wstring s2ws(const std::string &src);
void SetRegKey(TCHAR *vn, WCHAR *id); 
BOOL IsShadowDeviceClean();
BOOL WritelogMsg(const char* msg);
WCHAR tmpbuf[1024];

//TODO nocesdk

//#if 0
nextlabs::cesdk_loader _cesdk;
nextlabs::cesdk_connection _cesdk_conn;
//#endif


// End the inclusion guard
#endif
