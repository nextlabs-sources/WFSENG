#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory>
#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>
#include <string.h>
#include <strsafe.h>
#include <assert.h>
#include <fltuser.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <atlbase.h>
#include <wtypes.h>
#include "nlwfsecommon.h"

//TODO: cesdk
//#if 0
#include "eframework/platform/cesdk.hpp"
#include "eframework/timer/timer_high_resolution.hpp"
#include "nlconfig.hpp"
#include "celog.h"
#include "celog_policy_windbg.hpp"
#include "celog_policy_file.hpp"
//#endif

using namespace std;

// DECLDIR will perform an export for us
#define DLL_EXPORT

// Include our header, must come after #define DLL_EXPORT
#include "nlwfsedll.h"
DWORD g_BytesTransferred = 0;
WFSE_PORT_CONTEXT pctx;

// Get rid of name mangeling
extern "C"
{

DECLDIR int PluginEntry(void **nlwfseCtx)
{
    DWORD requestCount = NLWFSE_DEFAULT_REQUEST_COUNT;
    DWORD threadCount;
    TCHAR volumeName[MAXVOLUMELEN] = {0};
    TCHAR devname[MAXVOLUMELEN] = {0};
    char logmsg[MAX_PATH];
    SYSTEM_INFO sysInfo;
    HANDLE port, comport;
    //HANDLE wfseTimer;
    PWFSE_MESSAGE msg;
    wstring vname, dname;
    DWORD threadId;
// wfseTimerId;
    WCHAR drive[2] = {0};
    int wlen, ret = 0;
    HRESULT hr;
    DWORD i, j;

    *nlwfseCtx = &Tctx;

    InitializeCriticalSectionAndSpinCount(&logfileLock, 0x00000400);
    InitializeCriticalSectionAndSpinCount(&sdLock, 0x00000400);
    Tctx.flock = &logfileLock;
    SdResetMutex = CreateMutex(NULL, FALSE, NULL); //no owner
    Tctx.sdlock = &sdLock;

    //TURN off static sd device
#if 0
    if (!IsShadowDeviceClean()) {
        wfseQuerySnapshotSet();
    }
#endif

    wfseGetVol();    

    //TURN off static sd device
#if 0
    CreateShadowDevice();
    SetSdSizeRegKey(); 
#endif

    InstallNlwfseDriver();
    bool pathret = GetCesdkPath();
    if (!pathret) 
      goto exit_cleanup;

//TODO no cesdk
//#if 0
    if (!_cesdk.load(cesdkPath)) {
      ret = GetLastError();
      memset(logmsg, 0, sizeof(logmsg));
      wlen = _snprintf(logmsg, MAX_PATH, "cesdk Module load error %d \r\n", ret);
      EnterCriticalSection(&logfileLock);
      WritelogMsg(logmsg);
      LeaveCriticalSection(&logfileLock);
      ret = 0;
      goto exit_cleanup;
    }

    _cesdk_conn.set_sdk(&_cesdk);

    memset(logmsg, 0, sizeof(logmsg));
    wlen = _snprintf(logmsg, MAX_PATH, "cesdk Module load successfully \r\n");
    EnterCriticalSection(&logfileLock);
    WritelogMsg(logmsg);
    LeaveCriticalSection(&logfileLock);
//#endif

    GetSystemInfo(&sysInfo);
    threadCount = sysInfo.dwNumberOfProcessors; 
    Tctx.threadCnt = threadCount;

    pctx.pcid = GetCurrentProcessId();
    memset(logmsg, 0, sizeof(logmsg));
    wlen = _snprintf(logmsg, MAX_PATH, "pc id %d \r\n", pctx.pcid);
    EnterCriticalSection(&logfileLock);
    WritelogMsg(logmsg);
    LeaveCriticalSection(&logfileLock);

    hr = FilterConnectCommunicationPort( wfsePortName,
                                         0,
                                         &pctx,
                                         sizeof(WFSE_PORT_CONTEXT),
                                         NULL,
                                         &port );

    if (IS_ERROR( hr )) {
        memset(logmsg, 0, sizeof(logmsg));
        ret = GetLastError();
        wlen = _snprintf(logmsg, MAX_PATH, "Failed to connect to the driver %d \r\n", ret);
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
        ret = 0;
        goto exit_cleanup;
    }
    comport = CreateIoCompletionPort( port,
                                      NULL,
                                      0,
                                      threadCount );

    if (comport == NULL) {
        memset(logmsg, 0, sizeof(logmsg));
        ret = GetLastError();
        wlen = _snprintf(logmsg, MAX_PATH, "Failed to create communication port %d \r\n", ret);
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
        CloseHandle( port );
        ret = 0;
        goto exit_cleanup;
    }

    Tctx.Port = port;
    Tctx.Completion = comport;

#if 0
    if (!IsShadowDeviceClean()) {
        wfseQuerySnapshotSet();
    }

    wfseGetVol();    
    CreateShadowDevice();
    SetSdSizeRegKey(); 
#endif

#if 0
    for (auto it : Vctx.v_list) {
        memset(volumeName, 0, sizeof(volumeName));
        vname = it.first;
        dname = it.second;
        ret = QueryShadowDevice(vname, dname);
        if (ret == 99)
           break;
    }
    if (ret == 99) {
        CreateShadowDevice();
    }
#endif

    //TODO: turn on SD size

#if 0
    Tctx.wfseTimer = CreateThread(NULL,
                                  0,
                                  (LPTHREAD_START_ROUTINE)ShadowDeviceReset,
                                  0,
                                  0,
                                  &wfseTimerId);
#endif

    for (i = 0; i < threadCount; i++) {
        Tctx.threads[i] = CreateThread( NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE) nlwfseWorker,
                                   &Tctx,
                                   0,
                                   &threadId );

        if (Tctx.threads[i] == NULL) {
            memset(logmsg, 0, sizeof(logmsg));
            ret = GetLastError();
            wlen = _snprintf(logmsg, MAX_PATH, "Failed to create nlwfse thread error code %d \r\n", ret);
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
            ret = 0;
            goto exit_cleanup;
        }

        for (j = 0; j < requestCount; j++) {

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "msg will not be leaked because it is freed in wfseWorker")
            msg = (WFSE_MESSAGE *)malloc( sizeof(WFSE_MESSAGE ) );
            if (msg == NULL) {
                memset(logmsg, 0, sizeof(logmsg));
                ret = GetLastError();
                wlen = _snprintf(logmsg, MAX_PATH, "Failed to allocate wfse message buffer %d \r\n", ret);
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
                ret = 0;
                goto exit_cleanup;
            }
            memset( &msg->Ovlp, 0, sizeof( OVERLAPPED ) );
            hr = FilterGetMessage( port,
                                   &msg->MessageHeader,
                                   FIELD_OFFSET( WFSE_MESSAGE, Ovlp ),
                                   &msg->Ovlp );
            if (hr != HRESULT_FROM_WIN32( ERROR_IO_PENDING )) {
                free( msg );
                memset(logmsg, 0, sizeof(logmsg));
                ret = GetLastError();
                wlen = _snprintf(logmsg, MAX_PATH, "FilterGetMessage error %d \r\n", ret);
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
                ret = 0;
                goto exit_cleanup;
            }
        }
    }

    //hr = S_OK;
    ret = 1;
    //WaitForMultipleObjectsEx(i, Tctx.threads, TRUE, INFINITE, FALSE );

exit_cleanup:


    if (ret == 0) {
        if (Tctx.flock) {
            DeleteCriticalSection(&logfileLock);
        }
        if (Tctx.sdlock) {
            DeleteCriticalSection(&sdLock);
        }
        if (Tctx.Port != INVALID_HANDLE_VALUE) {
            CloseHandle(Tctx.Port);
        }
        if (Tctx.Completion != INVALID_HANDLE_VALUE)
            CloseHandle(Tctx.Completion);
#if 0
        if (Tctx.wfseTimer != INVALID_HANDLE_VALUE)
            CloseHandle(Tctx.wfseTimer);
#endif
    }
    return ret;
}

DECLDIR int PluginUnload(void *nlwfseCtx)
{
    char logmsg[MAX_PATH];
    int wlen = 0;

#if 0
    memset(logmsg, 0, sizeof(logmsg));
    wlen = _snprintf(logmsg, MAX_PATH, " Trun off protection... \r\n");
    EnterCriticalSection(&logfileLock);
    WritelogMsg(logmsg);
    LeaveCriticalSection(&logfileLock);
#endif

    //TURN off static sd device
#if 0
    wfseQuerySnapshotSet();
    wfseDeleteShadowRegistry();
#endif

    EnterCriticalSection(&sdLock);
    vsinfo.clear();
    LeaveCriticalSection(&sdLock);

    CloseHandle(Tctx.Port);
    CloseHandle(Tctx.Completion);
	
    Sleep(500);	// wait thread exit at here.

    DeleteCriticalSection(&logfileLock);
    DeleteCriticalSection(&sdLock);

    return 1;

}

} // extern C

VOID CALLBACK FileIOCompletionRoutine(
  __in  DWORD dwErrorCode,
  __in  DWORD dwNumberOfBytesTransfered,
  __in  LPOVERLAPPED lpOverlapped
  );

VOID CALLBACK FileIOCompletionRoutine(
  __in  DWORD dwErrorCode,
  __in  DWORD dwNumberOfBytesTransfered,
  __in  LPOVERLAPPED lpOverlapped)
{
  g_BytesTransferred = dwNumberOfBytesTransfered;
}


DWORD
nlwfseWorker(
    _In_ PNLWFSE_THREAD_CONTEXT Context
    )
{
    PNLWFSE_ACCESS_INFO qmsg;
    WFSE_REPLY_MESSAGE rmsg;
    PWFSE_MESSAGE message;
    WCHAR action[WFSE_ACTION_SIZE] = { 0 };
    LPOVERLAPPED pOvlp;
    ULONG access;
    BOOL result;
    DWORD outSize;
    HRESULT hr;
    ULONG_PTR key;
    OVERLAPPED ol = { 0 };
    DWORD ret;
    char logmsg[MAX_PATH];
    int wlen = 0;
    _int64 ticks = 0;
    CEAttributes* source_attrs;
    DWORD WaitRet;
    DWORD WaitMutexTime;

#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant

    while (TRUE) {
#pragma warning(pop)

        result = GetQueuedCompletionStatus( Context->Completion, &outSize, &key, &pOvlp, INFINITE );
		if(!result && GetLastError() == ERROR_ABANDONED_WAIT_0)	// quit at here, we can't use other resource any more.
		{
			OutputDebugStringW(L"-----------------> exit nlwfseWorker thread at here.\n");
			return 0;
		}

        message = CONTAINING_RECORD( pOvlp, WFSE_MESSAGE, Ovlp );

        if (!result) {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "GetQueuedCompletionStatus error  0x%X \r\n", hr);
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);

            break;
        }

        access = 0;
        qmsg = &message->accessInfo;
        
#if 0
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "nlwfseworker  received file len  %d \r\n", qmsg->filePathLength);
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
#endif

        assert(qmsg->filePathLength <= WFSE_MAX_BUFFER_SIZE);
        _Analysis_assume_(qmsg->filePathLength <= WFSE_MAX_BUFFER_SIZE);

        ZeroMemory(&rmsg, NLWFSE_REPLY_MESSAGE_SIZE);
        memset(action, 0, sizeof(action));
        if (qmsg->faction == WFSE_FILE_OPEN) {
            wcsncpy(action, L"OPEN", wcslen(L"OPEN"));

//TODO no cesdk
//#if 0
            source_attrs = nextlabs::cesdk_attributes::create();
            ticks = 0;

//TRUN off sd device

#if 0
SdCheck:

        if (SdReset) {
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "QueryAllowed: SdReset is on... \r\n");
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);

            WaitMutexTime = 5000; 
            WaitRet = WaitForSingleObject(SdResetMutex, WaitMutexTime);
            if (WaitRet == WAIT_OBJECT_0) {
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "worker get notification SdReset completed...  \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
                goto SdCheck;
            }
        }
#endif

        QueryAllowedAccess(message, source_attrs, action, &access, &ticks);
        _int64 endtime = GetTickCount64();
        _int64 elapsedTime = endtime - ticks;

#if 0
        memset(logmsg, 0, sizeof(logmsg));
        wlen = _snprintf(logmsg, MAX_PATH, "query completed start %lld end %lld elapsedTime %lld accessMask %x \r\n", ticks, endtime, elapsedTime, access);
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
#endif

//#endif


//TODO nocesdk
//#if 0
            nextlabs::cesdk_attributes::destroy(_cesdk, source_attrs);
//#endif

            rmsg.ReplyHeader.Status = 0;
            rmsg.ReplyHeader.MessageId = message->MessageHeader.MessageId;
            rmsg.accessMask = access;

            hr = FilterReplyMessage( Context->Port,
                                 &rmsg.ReplyHeader,
                                 NLWFSE_REPLY_MESSAGE_SIZE);

            if (!SUCCEEDED(hr)) {
                ret = GetLastError();
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "Reply message error  %d \r\n", ret);
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
                break;
            }
        } else {

            rmsg.ReplyHeader.Status = 0;
            rmsg.ReplyHeader.MessageId = message->MessageHeader.MessageId;
            rmsg.accessMask = 32768;

            hr = FilterReplyMessage( Context->Port,
                                 &rmsg.ReplyHeader,
                                 NLWFSE_REPLY_MESSAGE_SIZE);

            if (!SUCCEEDED(hr)) {
                ret = GetLastError();
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "Reply message error  %d \r\n", ret);
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
                break;
            }


            if ((qmsg->faction & WFSE_FILE_WRITE) && (qmsg->faction & WFSE_FILE_CHANGE_ATTRS)) {

#if 0
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "log write \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
#endif

                wcsncpy(action, L"EDIT", wcslen(L"EDIT"));

           } else if ((qmsg->faction & WFSE_FILE_WRITE) && (qmsg->faction & WFSE_FILE_CACHE_OPEN)) {

#if 0
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "log write \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
#endif

                wcsncpy(action, L"EDIT", wcslen(L"EDIT"));
           } else if ((qmsg->faction & WFSE_FILE_CHANGE_SECS) && (qmsg->faction & WFSE_FILE_CACHE_OPEN)) {

#if 0
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "log change security \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
#endif

                wcsncpy(action, L"CHANGE_SECURITY", wcslen(L"CHANGE_SECURITY"));
           } 
           if (qmsg->faction & WFSE_FILE_DELETE) {

#if 0
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "log delete \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
#endif

                wcsncpy(action, L"DELETE", wcslen(L"DELETE"));
          } 
          if (qmsg->faction & WFSE_FILE_RENAME) {

#if 0
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "log rename \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
#endif

                wcsncpy(action, L"RENAME", wcslen(L"RENAME"));
          }
          if (qmsg->faction & WFSE_FILE_CHANGE_ATTRS) {
#if 0
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "log change attribute \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
#endif
                wcsncpy(action, L"CHANGE_ATTRIBUTES", wcslen(L"CHANGE_ATTRIBUTES"));
          }
          if (qmsg->faction & WFSE_FILE_CHANGE_SECS) {
#if 0
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "log change security \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
#endif

                wcsncpy(action, L"CHANGE_SECURITY", wcslen(L"CHANGE_SECURITY"));
          }
          if (qmsg->faction & WFSE_FILE_CACHE_OPEN) {
#if 0
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "log cached open \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
#endif

          }
          if (qmsg->faction == WFSE_FILE_WRITE) {
#if 0
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "log single write \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
#endif

                wcsncpy(action, L"EDIT", wcslen(L"EDIT"));
          }

            source_attrs = nextlabs::cesdk_attributes::create();
            ticks = 0;

//TURN off sd device
#if 0
SdCheckAgain:

          if (SdReset) {
#if 0
              memset(logmsg, 0, sizeof(logmsg));
              wlen = _snprintf(logmsg, MAX_PATH, "QueryAllowed: single SdReset is on... \r\n");
              EnterCriticalSection(&logfileLock);
              WritelogMsg(logmsg);
              LeaveCriticalSection(&logfileLock);
#endif
              WaitMutexTime = 5000; 
              WaitRet = WaitForSingleObject(SdResetMutex, WaitMutexTime);
              if (WaitRet == WAIT_OBJECT_0) {
                 memset(logmsg, 0, sizeof(logmsg));
                 wlen = _snprintf(logmsg, MAX_PATH, "worker single SdReset completed...  \r\n");
                 EnterCriticalSection(&logfileLock);
                 WritelogMsg(logmsg);
                 LeaveCriticalSection(&logfileLock);
                 goto SdCheckAgain;
              }
         }
#endif

            QueryAllowedAccess(message, source_attrs, action, &access, &ticks);

            nextlabs::cesdk_attributes::destroy(_cesdk, source_attrs);
        }

        memset( &message->Ovlp, 0, sizeof( OVERLAPPED ) );

        hr = FilterGetMessage( Context->Port,
                               &message->MessageHeader,
                               FIELD_OFFSET( WFSE_MESSAGE, Ovlp ),
                               &message->Ovlp );

        if (hr != HRESULT_FROM_WIN32( ERROR_IO_PENDING )) {
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "FilterGetMessage error 0x%X \r\n", hr);
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
            break;
        }
    }

    if (!SUCCEEDED( hr )) {
        if (hr == HRESULT_FROM_WIN32( ERROR_INVALID_HANDLE )) {
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "Port get disconnected code 0x%X \r\n", hr);
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
        } else {
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "nlwfsedll unknown error 0x%X \r\n", hr);
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
        }
    }

    free( message );

    return hr;
}

//TODO nocesdk

//#if 0
void QueryAllowedAccess(PWFSE_MESSAGE msg,
                        CEAttributes* sattrs,
                        wchar_t *action,
                        ULONG *access,
                        _int64 *tk)
{
    PNLWFSE_ACCESS_INFO qmsg;
    wchar_t filePath[WFSE_MAX_BUFFER_SIZE] = {0};
    BOOL ret;
    char *fname;
    wchar_t *s1;
    const wchar_t *s2;
    char logmsg[WFSE_MAX_BUFFER_SIZE] = { 0 };
    int wlen = 0;
    long lResult = 0;
    DWORD WaitRet;
    DWORD WaitMutexTime;
 
    qmsg = &msg->accessInfo;

    if (_cesdk_conn.is_connected() == false) {
        if (!_cesdk_conn.connect()) {
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, WFSE_MAX_BUFFER_SIZE, "cesdk error not connected \r\n");
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
            return;
          }
    }


    for (auto it : Vctx.v_list) {
        const wchar_t *p = NULL;
        wstring drv = it.first;
        wstring devName = it.second;
        if ((p = wcsstr(qmsg->filePath, devName.c_str())) != 0) {
            wchar_t *ptr;
            ptr = qmsg->filePath;
            int len = (int)wcslen(devName.c_str());
            ptr += (len + 1);
            int slen = (int)wcslen(drv.c_str());
            s2 = drv.c_str();
            errno_t err = wcsncpy_s(filePath, WFSE_MAX_BUFFER_SIZE, s2, slen);

            if (err) {
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, WFSE_MAX_BUFFER_SIZE, "QueryAllowedAccess copy buffer error \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
                return;
            }
            else {
                wcscat(filePath, ptr);
            }
            break;
       }
    }


#if 0
    for (auto &i : vsinfo ) {
        const wchar_t *p = NULL;
        wstring drv = i.first;
        nlwfseDev sdev = i.second;
        wstring device = sdev.devName;
        wstring sdevice = sdev.sdevName;
        if ((p = wcsstr(qmsg->filePath, sdev.devName.c_str())) != 0) {
            wchar_t *ptr;
            ptr = qmsg->filePath;
            int len = (int)wcslen(sdev.devName.c_str());
            ptr += len;
            int slen = (int)wcslen(sdev.sdevName.c_str());
            s2 = sdev.sdevName.c_str();
            errno_t err = wcsncpy_s(filePath, WFSE_MAX_BUFFER_SIZE, s2, slen);

            if (err) {
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, WFSE_MAX_BUFFER_SIZE, "QueryAllowedAccess copy buffer error \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
                return;
            }
            else {
                wcscat(filePath, ptr);
            }
            break;
      }
   }
#endif


//TODO: disable msg
//
    //if (wcscmp(action, L"OPEN") == 0) {
#if 0
    size_t sz;
    char *ts = (char *)malloc(qmsg->sidLength + 1);
    memset(ts, 0, qmsg->sidLength + 1);
    wcstombs_s(&sz, ts, qmsg->sidLength + 1, qmsg->sidString, qmsg->sidLength);
    char *fs = (char *)malloc(wcslen(filePath) + 1);
    memset(fs, 0, wcslen(filePath) + 1);
    wcstombs_s(&sz, fs, wcslen(filePath) + 1, filePath, wcslen(filePath));
    memset(logmsg, 0, sizeof(logmsg));
    wlen = _snprintf(logmsg, WFSE_MAX_BUFFER_SIZE, "sid %s file %s \r\n", ts, fs);
   
    EnterCriticalSection(&logfileLock);
    WritelogMsg(logmsg);
    LeaveCriticalSection(&logfileLock);
    Sleep(1000);
    free(ts);
    free(fs);
#endif
   // }

    nextlabs::cesdk_attributes::add(_cesdk, sattrs, L"ce::filesystemcheck", L"yes");
    nextlabs::cesdk_attributes::add(_cesdk, sattrs, L"ce::request_cache_hint", L"yes");
    nextlabs::cesdk_query query_object(_cesdk);
    query_object.set_user_id(qmsg->sidString);
    query_object.set_source(filePath, L"fso", sattrs);
    query_object.set_obligations(true);
    query_object.set_timeout(3000);
    query_object.set_action(action);
    query_object.set_ip(ntohl(qmsg->ipaddr));

    if (wcscmp(action, L"OPEN") == 0) {
        *tk = GetTickCount64();
	ret = query_object.wfsemultiquery(_cesdk_conn.get_connection_handle(),lResult);
	*access |= lResult;
#if 0
        memset(logmsg, 0, sizeof(logmsg));
        EnterCriticalSection(&logfileLock);
        wlen = _snprintf(logmsg, WFSE_MAX_BUFFER_SIZE, "multiquery Policy logging \r\n");
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
#endif

    } else {
        if (qmsg->faction & WFSE_FILE_CACHE_OPEN) {
            wcsncpy(action, L"OPEN", wcslen(L"OPEN"));
        }

        ret = query_object.query(_cesdk_conn.get_connection_handle());
#if 0
        memset(logmsg, 0, sizeof(logmsg));
        EnterCriticalSection(&logfileLock);
        wlen = _snprintf(logmsg, WFSE_MAX_BUFFER_SIZE, "single query Policy logging \r\n");
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
#endif

    }
    if (ret == false) {
        int qerr;
        qerr = query_object.get_last_error();
        memset(logmsg, 0, sizeof(logmsg));
        EnterCriticalSection(&logfileLock);
        wlen = _snprintf(logmsg, WFSE_MAX_BUFFER_SIZE, "Policy query error %d \r\n", qerr);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
        return;
    }

    return;
}
//#endif


void ReleaseInterface (IUnknown* unkn)
{

  if (unkn != NULL)
	  unkn->Release();

}

void CreateShadowDevice()
{
    int returnValue = 0;
    int vsid = 0;
    int ret = 0;
    wstring vname, dname;
    DWORD wfseTimerId;
    LARGE_INTEGER wtime, deltaTime;
    deltaTime.QuadPart = -1800000000LL; //3 min 
    wtime.QuadPart = -36000000000LL; //1 hour 
    //wtime.QuadPart = 600000000;
    for (auto it : Vctx.v_list) {
        memset(nlwfsesd[vsid].volume, 0, MAXVOLUMELEN);
        memset(nlwfsesd[vsid].devName, 0, MAXVOLUMELEN);
        vname = it.first;
        dname = it.second;

        wcsncpy_s(nlwfsesd[vsid].volume, vname.c_str(), vname.length()); 
        wcsncpy_s(nlwfsesd[vsid].devName, dname.c_str(), dname.length()); 

        GetLocalTime(&nlwfsesd[vsid].ResetTime);
        nlwfsesd[vsid].wtime.QuadPart = 0;
        nlwfsesd[vsid].vsid = (vsid + 1);
        Tctx.wfseTimer[vsid] = CreateThread(NULL,
                                  0,
                                  (LPTHREAD_START_ROUTINE)ShadowDeviceReset,
                                  &nlwfsesd[vsid],
                                  0,
                                  &wfseTimerId);


        vsid++;
    } // end of v_list
    //Vctx.vcnt = vsid;
    
    return;
}


int makeSd(IVssBackupComponents **ptr, PNLWFSE_SD_CONTEXT Context)
{

    IVssBackupComponents *pBackup = NULL;
    IVssAsync *pAsync             = NULL;
    IVssAsync* pPrepare           = NULL;
    IVssAsync* pDoShadowCopy = NULL;
    VSS_SNAPSHOT_PROP snapshotProp = {0};

    TCHAR volumeName[MAXVOLUMELEN] = {0};
    int vsid = 0;
    wstring vname;
    WCHAR *ssid;
    WCHAR drive[2] = { 0 };
    HANDLE hToken;
    DWORD infoLen;
    DWORD ret;
    char logmsg[MAX_PATH];
    int wlen = 0;
    int scnt = 0;


#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
    OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken);
   
    TOKEN_ELEVATION elevation;
    GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &infoLen);
    if (!elevation.TokenIsElevated) {
        ret = GetLastError();
        memset(logmsg, 0, sizeof(logmsg));
        wlen = _snprintf(logmsg, MAX_PATH, "This code must run in elevated mode code %d \r\n", ret);
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
	return 3;	
    }	
#else
	#error you are using an old version of sdk or not supported operating system
        memset(logmsg, 0, sizeof(logmsg));
        wlen = _snprintf(logmsg, MAX_PATH, "The system is not supported OS %d \r\n", ret);
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
#endif

#if 0
    if (CoInitialize(NULL) != S_OK) {
        ret = GetLastError();
        memset(logmsg, 0, sizeof(logmsg));
        wlen = _snprintf(logmsg, MAX_PATH, "CoInitialize error %d \r\n", ret);
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
	return 1;
    }
#endif

    HRESULT result = CreateVssBackupComponents(&pBackup);

    if (result == S_OK) {
        result = pBackup->InitializeForBackup();

        if (result == S_OK) {
            result = pBackup->SetContext(VSS_CTX_BACKUP);
						
	    if (result != S_OK) { 
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "vss setContext error 0x%08lx  \r\n", result);
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
                ReleaseInterface(pBackup); 
                return result;
            }

	    VSS_ID snapshotSetId;
sdRetry:

	    result = pBackup->StartSnapshotSet(&snapshotSetId);
	    if (result != S_OK) { 
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "vss StartSnapshotSet error 0x%08lx  \r\n", result);
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
                Sleep(10000);
                scnt++; 
                if (scnt == 3) {
                   pBackup->Release();
                   memset(logmsg, 0, sizeof(logmsg));
                   wlen = _snprintf(logmsg, MAX_PATH, "vss StartSnapshotSet retry three times done  \r\n");
                   EnterCriticalSection(&logfileLock);
                   WritelogMsg(logmsg);
                   LeaveCriticalSection(&logfileLock);

                   return 4;
                }
                 
                goto sdRetry;
            }
	    if (result == S_OK) {
	        //result = pBackup->AddToSnapshotSet(vn, GUID_NULL, &snapshotSetId);
	        result = pBackup->AddToSnapshotSet(Context->volume, GUID_NULL, &snapshotSetId);
	        if (result != S_OK) {
                    memset(logmsg, 0, sizeof(logmsg));
                    wlen = _snprintf(logmsg, MAX_PATH, "vss AddSnapshotSet error 0x%08lx  \r\n", result);
                    EnterCriticalSection(&logfileLock);
                    WritelogMsg(logmsg);
                    LeaveCriticalSection(&logfileLock);
		}
 		if (result == S_OK) {
		    result = pBackup->SetBackupState(false, false, VSS_BT_COPY);
		    if (result != S_OK) {
		        printf("- Returned HRESULT = 0x%08lx\n", result);
		    }

		    if (result == S_OK) {
		        result = pBackup->PrepareForBackup(&pPrepare);
		        if (result == S_OK) {
		            result = pPrepare->Wait();
			    if (result == S_OK) {
			        result = pBackup->DoSnapshotSet(&pDoShadowCopy);
			        if (result == S_OK) {
			            result = pDoShadowCopy->Wait();
				    if (result == S_OK) {
				        result = pBackup->GetSnapshotProperties(snapshotSetId, &snapshotProp);
                        if (result == S_OK) {

                            //wcsncpy_s(drive, vn, 1);
                            wcsncpy_s(drive, Context->volume, 1);

                            ssid = Guid2Wchar(snapshotProp.m_SnapshotSetId);
                            SetRegKey(drive, ssid);
                            delete[] ssid;
                            SYSTEMTIME stUTC, stLocal;
                            FILETIME ftCreate;
                            ftCreate.dwHighDateTime = HILONG(snapshotProp.m_tsCreationTimestamp);
                            ftCreate.dwLowDateTime = LOLONG(snapshotProp.m_tsCreationTimestamp);
                            FileTimeToSystemTime(&ftCreate, &stUTC);
                            SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
                            {

                                nlwfseDev dev;
                                //dev.devName = devname;
                                dev.devName = Context->devName;
                                dev.sdevName = snapshotProp.m_pwszSnapshotDeviceObject;
                                dev.snapshotSetId = snapshotProp.m_SnapshotSetId;
                                //vn has the format c:
                                //vsinfo.insert(make_pair(vn, dev));
                                vsinfo.insert(make_pair(Context->volume, dev));

                            }

                            VssFreeSnapshotProperties(&snapshotProp);
                        } //GetSnapshotProperties
				    } //Take snapshot ok
                                     //TODO
				    pDoShadowCopy->Release();
			     } //CreateSnapshot OK
		         } //Prepare snapshot OK
                         //TODO:
		         pPrepare->Release();
		      }
		  }
	      }
	  }
      }
      //TODO
      //pBackup->Release();
    }
    //TODO
    //CoUninitialize();
    *ptr = pBackup;
    return result;
}

void wfseGetVol()
{
  DWORD  CharCount = 0;
  WCHAR  DeviceName[MAX_PATH] = L"";
  DWORD  Error = ERROR_SUCCESS;
  HANDLE VHandle = INVALID_HANDLE_VALUE;
  BOOL   Found = FALSE;
  size_t Index = 0;
  WCHAR  VolumeName[MAX_PATH] = L"";
  WCHAR Names[MAX_PATH] = { 0 };
  BOOL   Success = FALSE;
  int vsid = 0;
  unsigned int vtype;
  std::wstring vname;

  //
  //  Enumerate all volumes in the system.
  VHandle = FindFirstVolume(VolumeName, ARRAYSIZE(VolumeName));

  if (VHandle == INVALID_HANDLE_VALUE) {
    Error = GetLastError();
    wprintf(L"FindFirstVolumeW failed with error code %d\n", Error);
    return;
  }

  Vctx.vcnt = 0;

  for (;;) {
    Index = wcslen(VolumeName) - 1;

    if (VolumeName[0] != L'\\' ||
      VolumeName[1] != L'\\' ||
      VolumeName[2] != L'?' ||
      VolumeName[3] != L'\\' ||
      VolumeName[Index] != L'\\') {
      Error = ERROR_BAD_PATHNAME;
      wprintf(L"FindFirstVolumeW/FindNextVolumeW returned a bad path: %s\n", VolumeName);
      break;
    }

    //
    //  QueryDosDeviceW does not allow a trailing backslash,
    //  so temporarily remove it.
    VolumeName[Index] = L'\0';
    CharCount = QueryDosDevice(&VolumeName[4], DeviceName, ARRAYSIZE(DeviceName));
    VolumeName[Index] = L'\\';

    if (CharCount == 0) {
      Error = GetLastError();
      wprintf(L"QueryDosDeviceW failed with error code %d\n", Error);
      break;
    }

    Success = GetVolumePathNamesForVolumeName(VolumeName,
      (LPTSTR)&Names,
      CharCount,
      &CharCount);

    if (Success && CharCount > 1) {
      printf("Names length: %d \n", wcslen(Names));
      vtype = GetDriveType(Names);
      if (vtype == DRIVE_FIXED) {
          Vctx.v_list.insert(make_pair(Names, DeviceName));
          Vctx.vcnt++;
      }

    }
    Success = FindNextVolume(VHandle, VolumeName, ARRAYSIZE(VolumeName));

    if (!Success) {
      Error = GetLastError();

      if (Error != ERROR_NO_MORE_FILES) {
        wprintf(L"FindNextVolumeW failed with error code %d\n", Error);
        break;
      }

      //  Finished iterating
      //  through all the volumes.
      Error = ERROR_SUCCESS;
      break;
    }
  }

  FindVolumeClose(VHandle);
  VHandle = INVALID_HANDLE_VALUE;

  return;
}

DWORD WINAPI ShadowDeviceReset(
    _In_ PNLWFSE_SD_CONTEXT Context
)
{
    IVssBackupComponents *pBackup = NULL;
    LARGE_INTEGER wtime, deltaTime;
    deltaTime.QuadPart = -600000000LL; //5 min 
    wtime.QuadPart = -36000000000LL; //1 hour 
    HANDLE hTimer = NULL;
    LARGE_INTEGER liDueTime;
    char logmsg[MAX_PATH];
    int ret, wlen;
    DWORD WaitMutexTime;
    BOOL NewSd = FALSE;
    BOOL relRet;

    hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
    if (NULL == hTimer) 
    {
        memset(logmsg, 0, sizeof(logmsg));
        wlen = _snprintf(logmsg, MAX_PATH, "CreateTimer error %d \r\n", GetLastError());
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
        return 1;
    }

    for (;;) {
        EnterCriticalSection(&sdLock);

        if (CoInitialize(NULL) != S_OK) 
        {
            ret = GetLastError();
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "CoInitialize sd create error %d \r\n", ret);
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
            LeaveCriticalSection(&sdLock);
            return 1;
        }

#if 0
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "Thread Create sd device... \r\n");
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
#endif

        ret = makeSd(&Context->pBackup[Context->vsid], Context);
        if (Context->wtime.QuadPart == 0) {
            Context->wtime.QuadPart = ((wtime.QuadPart) + (deltaTime.QuadPart * (Context->vsid  - 1)));;

#if 0
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "set wtime for reset vsid %d \r\n", Context->vsid);
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
#endif

        }
        if (SdReset == TRUE) {
            
            SdReset = FALSE;
            relRet = ReleaseMutex(SdResetMutex);
            if (!relRet) {
                 memset(logmsg, 0, sizeof(logmsg));
                 wlen = _snprintf(logmsg, MAX_PATH, "Mutex release error in Reset thread... \r\n");
                 EnterCriticalSection(&logfileLock);
                 WritelogMsg(logmsg);
                 LeaveCriticalSection(&logfileLock);
            }
#if 0
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "SdReset sd done release mutex... \r\n");
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
#endif

        }
#if 0   // we don't need backup the module , we never use it at all
        if (ret == S_OK && pBackup != NULL)
        {
             for (int i = 0; i < MAX_SD_VOLUME; i++)
            {
                p = Tctx.pBackup[i];
                if (p == NULL) {
                    Tctx.pBackup[i] = pBackup;
                    break;
                }
            }
        }
#endif
        LeaveCriticalSection(&sdLock);

WaitAgain:

        if (!SetWaitableTimer(hTimer, &Context->wtime, 0, NULL, NULL, 0)) {
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "SetTimer error %d \r\n", GetLastError());
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
            if (Context->pBackup[Context->vsid] != NULL)    Context->pBackup[Context->vsid]->Release();
            return 2;
        }

        if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0) {
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "ShadowDeviceReset WaitForSingleObject error %d \r\n", GetLastError());
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
            goto WaitAgain;
        } else {
            SYSTEMTIME lt;
            PVOID OldValue = NULL;
            wstring fname;
            string volname, tmpstr;
            int sdSize = -1;
            std::string volstr = "C:";
            std::string rootstr = "c:\\Windows\\temp\\";
            const char *command;
            BOOL myVol = FALSE;
            BOOL NewSd = FALSE;
            DWORD WaitRet;

            GetLocalTime(&lt);

            WaitRet = WaitForSingleObject(SdResetMutex, INFINITE);
            if (WaitRet == WAIT_OBJECT_0) {
                SdReset = TRUE;

#if 0
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "Obtained mutex to do sd reset \r\n");
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
#endif

            wstring vn = Context->volume;
	    volname = volstr.substr(0, 1);
	    rootstr.append(volname);
	    rootstr.append(".log");
	    std::string str1 = "c:\\Windows\\System32\\vssadmin list shadowstorage >> ";
	    str1.append(rootstr);
	    command = str1.c_str();
    
            system(command);
	    std::ifstream ifs(rootstr);

	    std::string line;

	    while (std::getline(ifs, line)) {
	        if (strstr(line.c_str(), "For volume")) {
	            std::size_t begin = line.find("(");
	            std::size_t end = line.find(")");
                  //vol has format c: 
                  //vn has format c://

	            std::string vol = line.substr(begin + 1, ((end - begin) - 1));
                    vol += "\\";
                    std::wstring volwstr = s2ws(vol);
                    if (vn.compare(volwstr) == 0) {
                        myVol = TRUE;

#if 0
                       memset(logmsg, 0, sizeof(logmsg));
                       wlen = _snprintf(logmsg, MAX_PATH, "found myvol... \r\n");
                       EnterCriticalSection(&logfileLock);
                       WritelogMsg(logmsg);
                       LeaveCriticalSection(&logfileLock);
#endif


                    }
	        }
	        if (strstr(line.c_str(), "Used Shadow Copy Storage space") && myVol) {
	            std::size_t begin = line.find("(");
	            std::size_t end = line.find("%");
	            std::string tmpstr = line.substr(begin + 1, ((end - begin) - 1));
	            sdSize = atoi(tmpstr.c_str());

#if 0
                       memset(logmsg, 0, sizeof(logmsg));
                       wlen = _snprintf(logmsg, MAX_PATH, "checking storage size... \r\n");
                       EnterCriticalSection(&logfileLock);
                       WritelogMsg(logmsg);
                       LeaveCriticalSection(&logfileLock);
#endif

	            if ((sdSize > 80) || ((lt.wHour == 1) || (lt.wHour == 2) 
                        || (lt.wHour == 3)) && (Context->ResetTime.wDay != lt.wDay))  {
	               NewSd = TRUE;

                       if (sdSize > 80) {
                           memset(logmsg, 0, sizeof(logmsg));
                           wlen = _snprintf(logmsg, MAX_PATH, "SdRset Starting... \r\n");
                           EnterCriticalSection(&logfileLock);
                           WritelogMsg(logmsg);
                           LeaveCriticalSection(&logfileLock);
                       }
	               break;
	            }
	        }

#if 0
	        if (strstr(line.c_str(), "Maximum Shadow Copy Storage space")) {
	            std::size_t begin = line.find("(");
	            std::size_t end = line.find("%");
	            std::string tmpstr = line.substr(begin + 1, ((end - begin) - 1));
	            sdSize = atoi(tmpstr.c_str());
                       //TODO
                       memset(logmsg, 0, sizeof(logmsg));
                       wlen = _snprintf(logmsg, MAX_PATH, "checking max storage size... \r\n");
                       EnterCriticalSection(&logfileLock);
                       WritelogMsg(logmsg);
                       LeaveCriticalSection(&logfileLock);


	            if (sdSize == 100) {
                       memset(logmsg, 0, sizeof(logmsg));
                       wlen = _snprintf(logmsg, MAX_PATH, "max sd storage... \r\n");
                       EnterCriticalSection(&logfileLock);
                       WritelogMsg(logmsg);
                       LeaveCriticalSection(&logfileLock);
	            }
	        }
#endif

	    }
	    ifs.close();
            fname = s2ws(rootstr);
            DeleteFile(fname.c_str());

            if (NewSd && myVol) {
                //delete sd and update vsinfo

                Context->pBackup[Context->vsid]->Release();
                CoUninitialize();

                for (auto &i : vsinfo) {
                    wstring drv = i.first;
                    //vn has format c:
                    if (drv.compare(vn) == 0) {
                        vsinfo.erase(i.first);

#if 0
                        memset(logmsg, 0, sizeof(logmsg));
                        wlen = _snprintf(logmsg, MAX_PATH, "Cleaning vsinfo... \r\n");
                        EnterCriticalSection(&logfileLock);
                        WritelogMsg(logmsg);
                        LeaveCriticalSection(&logfileLock);
#endif

                        break;
                    }
               }
               GetLocalTime(&Context->ResetTime);
#if 0
               memset(logmsg, 0, sizeof(logmsg));
               wlen = _snprintf(logmsg, MAX_PATH, "vsid %d set sd reset time \r\n", Context->vsid);
               EnterCriticalSection(&logfileLock);
               WritelogMsg(logmsg);
               LeaveCriticalSection(&logfileLock);
#endif
            } else {
              ReleaseMutex(SdResetMutex);
              SdReset = FALSE;
              goto WaitAgain;
            }

            }
        }
     }
     if (Context->pBackup[Context->vsid] != NULL)
        Context->pBackup[Context->vsid]->Release();
     return 0;
}


void wfseLookupSnapshotSet(PNLWFSE_SD_CONTEXT Context)
{
    BOOL DeleteDone = FALSE;
    HKEY key;
    DWORD dtype = REG_SZ;
    DWORD cbData;
    DWORD dwRet;
    wstring vn, drv;
    LPBYTE lpData[4096] = {0};
    char logmsg[MAX_PATH];
    int wlen;

    TCHAR  sdRoot[260] = TEXT("SYSTEM\\CurrentControlSet\\Services\\nlwfse\\");
    TCHAR  sd[260] = TEXT("SYSTEM\\CurrentControlSet\\Services\\nlwfse\\ShadowDevices\\pc");

    //EnterCriticalSection(&sdLock);

    DeleteDone = FALSE;
    vn = Context->volume; 
    drv = vn.substr(0, 1); 
    StringCchCat(sd, 260, TEXT("\\"));
    StringCchCat(sd, 260, drv.c_str());
    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, sd, &key);
    dwRet = RegQueryValueEx(key,
                                TEXT("ShadowDeviceId"),
                                NULL,
                                &dtype,
                                (LPBYTE)lpData,
                                &cbData);

        if (dwRet == ERROR_SUCCESS) {
            static GUID vsguid;
            HRESULT result;
            wstring sdset((wchar_t *)lpData);
            result = ::CLSIDFromString(W2OLE(const_cast<WCHAR*>(sdset.c_str())), &vsguid);

         //TODO: delete debug msg
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "Thread delete sd device... \r\n");
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);

            DeleteDone = wfseDeleteSnapshotSet((VSS_ID)vsguid);

            if (!DeleteDone) {
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "ShadowDevices delete failed \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);

               // RegCloseKey(key);
            } else {
                //TODO: delete
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "LookupSnapshot: ShadowDevices deleted...  \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
            }
        } else {
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "RegQueryValueEx error %d ...  \r\n", dwRet);
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
        }
        RegCloseKey(key);
    //TODO: check if new value set corectly in registery by overwrite old one
    if (DeleteDone) {
                wstring vn = Context->volume;
                for (auto &i : vsinfo ) {
                    wstring drv = i.first;
                    //vn has format c:
                    //TODO: review the erase of list
                    if (drv.compare(vn) == 0) {
                      vsinfo.erase(i.first);

                     
                      
                //TODO: delete msg
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "LookupSnapshot: delete item from vsinfo list...  \r\n");
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
                        break;
                    }
                }

#if 0
        dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, sdRoot, &key);
        dwRet = RegDeleteTree(key, L"ShadowDevices");
        if (dwRet) {
            memset(logmsg, 0, sizeof(logmsg));
            wlen = _snprintf(logmsg, MAX_PATH, "RegDeleteTree error %d ...  \r\n", dwRet);
            EnterCriticalSection(&logfileLock);
            WritelogMsg(logmsg);
            LeaveCriticalSection(&logfileLock);
            return;
        }
        EnterCriticalSection(&sdLock);
        vsinfo.clear();
        LeaveCriticalSection(&sdLock);
        CreateShadowDevice();
#endif
    }
    //LeaveCriticalSection(&sdLock);
}


BOOL wfseDeleteSnapshotSet(VSS_ID snapshotSetID)
{
    IVssBackupComponents *pVssObject = NULL;
    HANDLE hToken;
    DWORD infoLen;
    HRESULT result;
    char logmsg[MAX_PATH];
    int wlen;

    if (snapshotSetID == GUID_NULL) {
        memset(logmsg, 0, sizeof(logmsg));
        wlen = _snprintf(logmsg, MAX_PATH, "Delete shadow device id is NULL \r\n");
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
        return FALSE;
    }

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
    OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken);
   
    TOKEN_ELEVATION elevation;
    GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &infoLen);
    if (!elevation.TokenIsElevated) {
	return FALSE;	
    }	
#else
	#error you are using an old version of sdk or not supported operating system
#endif

    if (CoInitialize(NULL) != S_OK) {
        memset(logmsg, 0, sizeof(logmsg));
        wlen = _snprintf(logmsg, MAX_PATH, "Delete shadow device Coinit error \r\n");
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
	return FALSE;
    }

    result = CreateVssBackupComponents(&pVssObject);

    if (result == S_OK) {
        LONG lSnapshots = 0;
        VSS_ID FailDeletedSnapshotID = GUID_NULL;

        result = pVssObject->InitializeForBackup();
        if (result == S_OK) {
            result = pVssObject->SetContext(VSS_CTX_ALL);
            result = pVssObject->DeleteSnapshots(snapshotSetID, 
                                                 VSS_OBJECT_SNAPSHOT_SET,
                                                 FALSE,
                                                 &lSnapshots,
                                                 &FailDeletedSnapshotID);

            if (FAILED(result)) {
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "Delete shadow device error %x \r\n", result);
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
                pVssObject->Release();
                CoUninitialize();
                return FALSE;
            }
        } else {
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "initial backup error %x \r\n", result);
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
        }
    } else {

       memset(logmsg, 0, sizeof(logmsg));
       wlen = _snprintf(logmsg, MAX_PATH, "create backup object error %x \r\n", result);
       EnterCriticalSection(&logfileLock);
       WritelogMsg(logmsg);
       LeaveCriticalSection(&logfileLock);
    }

    pVssObject->Release();
    CoUninitialize();
    return TRUE;
}

BOOL WritelogMsg(const char* msg)
{ 
    BOOL	ret = FALSE;
    HANDLE	hFile = INVALID_HANDLE_VALUE;
    WCHAR	Path[1024];
    WCHAR	filename[1024];
    WCHAR       tmpbuf[1024];
    WCHAR	*ptr;
    WCHAR	logfile[] = TEXT("nlwfsedll.log");
    int		filelen;
    DWORD	dwSize;
    WIN32_FIND_DATA findDataResult;
    BOOL            fsuccess;
    HANDLE          findFileHandle;
    DWORD           dwAttrs;
        try
        {
           memset(filename, 0, 1024);
           memset(Path, 0, sizeof(Path));
	   if(GetModuleFileName(nlwfseInstance, Path, sizeof(Path))) {
	       WCHAR* p = wcsrchr(Path, '\\');
	       if(p)
	        *p = '\0';
	       else
		 Path[0] = '\0';
	   }
	   if(Path[0] == '\0')
	      wcscpy(Path, TEXT(".\\"));
		
	    filelen = (int)wcslen(Path) + (int)wcslen(logfile);
	    _snwprintf(filename, (filelen + 1), L"%s%s%s", Path, "\\", logfile);

            hFile = ::CreateFile(filename, GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
	       dwSize = GetFileSize(hFile, NULL);
	       if (dwSize > 1000000) {
		   wcscpy(tmpbuf , filename);
                   ptr = wcsrchr(tmpbuf, '.');
	           if (ptr != '\0') {
		       *ptr = '\0';
		   }
					
		   wcscat(tmpbuf, TEXT("-prev.log"));

		   findFileHandle = FindFirstFile(tmpbuf, &findDataResult);
		   if (findFileHandle != INVALID_HANDLE_VALUE) {
		       FindClose(findFileHandle);
		       fsuccess = DeleteFile(tmpbuf);
		       if (!fsuccess) {
		           dwAttrs = GetFileAttributes(tmpbuf); 
			   if ((dwAttrs != 0xFFFFFFFF) &&
                               (dwAttrs & FILE_ATTRIBUTE_READONLY)) {
			       SetFileAttributes(tmpbuf, FILE_ATTRIBUTE_NORMAL);
			       fsuccess = DeleteFile(tmpbuf);
			    }

			    if (!fsuccess) {
			        CloseHandle(hFile);
				return(FALSE);
			     }
			}  
		    }
		    CloseHandle(hFile);

		    MoveFile(filename, tmpbuf);

		    hFile = ::CreateFile(filename, GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		}
	   }

            if (hFile != INVALID_HANDLE_VALUE)
            {
                SYSTEMTIME  TimeSys;
                char szMessage[5124];
                DWORD dwFP = SetFilePointer(hFile, 0, NULL, FILE_END);

                DWORD dwLen;
                GetLocalTime(&TimeSys);
                int iLen = sprintf(szMessage, "%d/%02d/%02d %02d:%02d:%02d.%03d [%x]: %.4096s\r\n", 
                    TimeSys.wYear, TimeSys.wMonth, TimeSys.wDay,
                    TimeSys.wHour, TimeSys.wMinute, TimeSys.wSecond, TimeSys.wMilliseconds,
                    GetCurrentThreadId(), msg);

                WriteFile(hFile, szMessage, (DWORD) iLen, &dwLen, NULL);
                FlushFileBuffers(hFile);
                ret = TRUE;
	     }
        }
        catch(...) {
        }
        if(hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);

    return ret;
}

std::string ws2s(const std::wstring &s)
{
    int len;
    int slen = (int)s.length() + 1;
    len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slen, 0,0,0,0);
    std::string r(len, '\0');
    WideCharToMultiByte(CP_ACP, 0, s.c_str(), slen, &r[0], len, 0, 0);
    return r;
}

void SetRegKey(TCHAR *vn, WCHAR *id) 
{
  HKEY hKey;
  LONG ret;
  TCHAR  sd[260] = TEXT("SYSTEM\\CurrentControlSet\\Services\\nlwfse\\ShadowDevices\\pc");

  ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sd, 0, KEY_ALL_ACCESS, &hKey);

  if (ret == ERROR_FILE_NOT_FOUND)
  {
    ret = RegCreateKeyEx(HKEY_LOCAL_MACHINE, sd, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
  } 
  StringCchCat(sd, 260, TEXT("\\"));
  StringCchCat(sd, 260, vn);

  ret = RegCreateKeyEx(HKEY_LOCAL_MACHINE, sd, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

  RegSetValueEx(hKey, L"ShadowDeviceId", 0, REG_SZ, (BYTE *)id, (wcslen(id) + 2) * sizeof(WCHAR));
  RegCloseKey(hKey);

}

BOOL IsShadowDeviceClean()
{
    HKEY hSdKey;
    BOOL ret; 
    WCHAR  sd[260] = L"SYSTEM\\CurrentControlSet\\Services\\nlwfse\\ShadowDevices\\pc";

    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       sd,
                       0,
                       KEY_ALL_ACCESS,
                       &hSdKey);
    if (ret == 0)
        return FALSE;
    else 
        return TRUE;

}

void wfseDeleteShadowRegistry()
{
    HKEY key;
    DWORD dwRet;
    char logmsg[MAX_PATH];
    int wlen;

   // TCHAR  sdRoot[260] = TEXT("SYSTEM\\CurrentControlSet\\Services\\nlwfse\\");
    TCHAR  sdRoot[260] = TEXT("SYSTEM\\CurrentControlSet\\Services\\nlwfse\\ShadowDevices\\");

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, sdRoot, &key);
    dwRet = RegDeleteTree(key, L"pc");
    if (dwRet) {
        memset(logmsg, 0, sizeof(logmsg));
        wlen = _snprintf(logmsg, MAX_PATH, "RegDeleteTree error %d ...  \r\n", dwRet);
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
        return;
    }
    RegCloseKey(key);
    return;
}

std::wstring s2ws(const std::string &src)
{
    int asize = MultiByteToWideChar(CP_UTF8, 0, &src[0], (int)src.size(), NULL, 0);
    std::wstring wstrTo(asize, 0);
    MultiByteToWideChar(CP_UTF8, 0, &src[0], (int)src.size(), &wstrTo[0], asize);
    return wstrTo;
}

WCHAR* Guid2Wchar(VSS_ID id)
{
    WCHAR *pSTR = new WCHAR[39];
    swprintf(pSTR,
             39,
             L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
             id.Data1,
             id.Data2,
             id.Data3,
             id.Data4[0],
             id.Data4[1],
             id.Data4[2],
             id.Data4[3],
             id.Data4[4],
             id.Data4[5],
             id.Data4[6],
             id.Data4[7],
             id.Data4[8]);
      return pSTR;
}

DWORD InstallNlwfseDriver() 
{

    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS_PROCESS ssStatus; 
    DWORD dwOldCheckPoint; 
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwBytesNeeded;
    TCHAR       driverPath[MAX_PATH];
    TCHAR       systemRoot[MAX_PATH];
    char logmsg[MAX_PATH];
    int wlen;
	
    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // servicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
 
    if (NULL == schSCManager) 
    {
        return 1;
    }

    schService = OpenService(schSCManager,
                             TEXT("nlwfse"),
                             SERVICE_ALL_ACCESS);
 
    if (schService == NULL) { 
	    if (ERROR_SERVICE_DOES_NOT_EXIST == GetLastError()) {

           if(!GetEnvironmentVariable(TEXT("SYSTEMROOT"), systemRoot, sizeof(systemRoot))) {
              return  GetLastError();
           }
           int wlen = _snprintf((char *)driverPath, MAX_PATH, (char *)TEXT("%s\\system32\\drivers\\%s"), systemRoot, NLWFSE_SERVICE_NAME);

	       schService = CreateService(schSCManager,
	              NLWFSE_SERVICE_NAME,
	              NLWFSE_SERVICE_NAME,
				  SERVICE_ALL_ACCESS,
				  SERVICE_KERNEL_DRIVER,
				  SERVICE_DEMAND_START,
				  SERVICE_ERROR_NORMAL,
				  driverPath,
				  NULL,
				  NULL,
				  NULL,
				  NULL,
				  NULL);

		    if ( schService == NULL ) {
			   return 1;
			}
	   } else {
           CloseServiceHandle(schSCManager);
           return 1;
	   }
	}

    if (!QueryServiceStatusEx( 
            schService,                     // handle to service 
            SC_STATUS_PROCESS_INFO,         // information level
            (LPBYTE) &ssStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded ) )              // size needed if buffer is too small
    {
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
        return 1;
    }

    // Check if the service is already running. It would be possible 
    // to stop the service here, but for simplicity just returns. 

    if(ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
    {
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
        return 1;
    }

    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if( dwWaitTime < 1000 )
            dwWaitTime = 1000;
        else if ( dwWaitTime > 10000 )
            dwWaitTime = 10000;

        Sleep( dwWaitTime );

        if (!QueryServiceStatusEx( 
                schService,                     // handle to service 
                SC_STATUS_PROCESS_INFO,         // information level
                (LPBYTE) &ssStatus,             // address of structure
                sizeof(SERVICE_STATUS_PROCESS), // size of structure
                &dwBytesNeeded ) )              // size needed if buffer is too small
        {
            CloseServiceHandle(schService); 
            CloseServiceHandle(schSCManager);
            return 1;
        }

        if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
        {
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
            {
                break;
            }
        }
    } 

    if (!StartService(
            schService,  // handle to service 
            0,           // number of arguments 
            NULL) )      // no arguments 
    {
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
        return 1;
    }
    else {
	    //InstallDriver: Serivce start pending
	}

    // Check the status until the service is no longer start pending. 
 
    if (!QueryServiceStatusEx( 
            schService,                     // handle to service 
            SC_STATUS_PROCESS_INFO,         // info level
            (LPBYTE) &ssStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded ) )              // if buffer too small
    {
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
        return 1;
    }
 
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING) 
    { 
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if( dwWaitTime < 1000 )
            dwWaitTime = 1000;
        else if ( dwWaitTime > 10000 )
            dwWaitTime = 10000;

        Sleep( dwWaitTime );

        if (!QueryServiceStatusEx( 
            schService,             
            SC_STATUS_PROCESS_INFO,
            (LPBYTE) &ssStatus,   
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded ) )            
        {
            break; 
        }
 
        if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
        {
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
            {
                // No progress made within the wait hint.
                break;
            }
        }
	}

    // Determine whether the service is running.

    if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
    {
        memset(logmsg, 0, sizeof(logmsg));
        wlen = _snprintf(logmsg, MAX_PATH, "nlwfse driver load successfully... \r\n");
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
    }
    else 
    { 
        //InstallDriver: Current State
    } 

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
    return 0;
}

void SetSdSizeRegKey() 
{
  HKEY hKey;
  LONG ret;
  WCHAR  sd[260] = L"SYSTEM\\CurrentControlSet\\Services\\volsnap\\";

  DWORD val = MAX_SD_SIZE;

  ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sd, 0, KEY_ALL_ACCESS, &hKey);
  RegSetValueEx(hKey, L"MinDiffAreaFileSize", 0, REG_DWORD, (LPBYTE)&val, sizeof(DWORD));
  RegCloseKey(hKey);
}

void wfseQuerySnapshotSet()
{
    IVssBackupComponents *pVssObject = NULL;
    HANDLE hToken;
    DWORD infoLen;
    HRESULT result;
    char logmsg[MAX_PATH];
    int wlen;

    #if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
    OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken);
   
    TOKEN_ELEVATION elevation;
    GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &infoLen);
    if (!elevation.TokenIsElevated) {
    return;	
    }	
    #else
    #error you are using an old version of sdk or not supported operating system
    #endif

    if (CoInitialize(NULL) != S_OK) {
        memset(logmsg, 0, sizeof(logmsg));
        wlen = _snprintf(logmsg, MAX_PATH, "wfseQuerySnapshotSet: CoInitialize error %d \r\n", GetLastError());
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);

        return;
    }

    result = CreateVssBackupComponents(&pVssObject);

    if (result == S_OK) {
    CComPtr<IVssEnumObject> pIEnumSnapshots;

    result = pVssObject->InitializeForBackup();
    if (result == S_OK) {
        result = pVssObject->SetContext(VSS_CTX_ALL);
    } else {
        memset(logmsg, 0, sizeof(logmsg));
        wlen = _snprintf(logmsg, MAX_PATH, "wfseQuerySnapshotSet: Initialize vssObject %d \r\n", GetLastError());
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
        CoUninitialize();
        return;
    }
            
    HRESULT hr = pVssObject->Query( GUID_NULL, 
				    VSS_OBJECT_NONE, 
				    VSS_OBJECT_SNAPSHOT, 
				    &pIEnumSnapshots );

// If there are no shadow copies, just return
    if (hr == S_FALSE) {
        memset(logmsg, 0, sizeof(logmsg));
        wlen = _snprintf(logmsg, MAX_PATH, "wfseQuerySnapshotSet: No shadow devices \r\n");
        EnterCriticalSection(&logfileLock);
        WritelogMsg(logmsg);
        LeaveCriticalSection(&logfileLock);
        if (pVssObject != NULL)   pVssObject->Release();
        return;
    }

    VSS_OBJECT_PROP Prop;
    VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;

    while(true) 
    {
        ULONG ulFetched;
        LONG Snapshots = 0;
        VSS_ID FailDeletedSnapshotID = GUID_NULL;

        hr = pIEnumSnapshots->Next(1, &Prop, &ulFetched);

        if (ulFetched == 0)
            break;

        BOOL found = wfseLookupShadowDevices(Snap.m_SnapshotSetId);

        if (found) {
            result = pVssObject->DeleteSnapshots(Snap.m_SnapshotSetId,
                VSS_OBJECT_SNAPSHOT_SET,
                FALSE,
                &Snapshots,
                &FailDeletedSnapshotID);

            if (FAILED(result)) {
                memset(logmsg, 0, sizeof(logmsg));
                wlen = _snprintf(logmsg, MAX_PATH, "Delete shadow device error %x \r\n", result);
                EnterCriticalSection(&logfileLock);
                WritelogMsg(logmsg);
                LeaveCriticalSection(&logfileLock);
                if (pVssObject != NULL)   pVssObject->Release();
                CoUninitialize();
                return;
            }
        }
    }
    if(pVssObject!= NULL)   pVssObject->Release();
    VssFreeSnapshotProperties(&Snap);
  }

  CoUninitialize();
}

BOOL wfseLookupShadowDevices(VSS_ID sdevId)
{
    HKEY hKey, hVolKey;
    TCHAR    achKey[MAX_KEY_LENGTH];   
    DWORD    cbName;                  
    TCHAR    achClass[MAX_PATH] = TEXT(""); 
    DWORD    cchClassName = MAX_PATH;  
    DWORD    cSubKeys = 0;            
    DWORD    volSubKeys = 0;         
    DWORD    volValues;             
    DWORD    cbMaxSubKey;          
    DWORD    cchMaxClass;         
    DWORD    cValues;            
    DWORD    cchMaxValue;       
    DWORD    cbMaxValueData;   
    DWORD    cbSecurityDescriptor; 
    FILETIME ftLastWriteTime;     
    DWORD i, retCode;

    TCHAR  achValue[MAX_VALUE_NAME];
    DWORD cchValue = MAX_VALUE_NAME;
    WCHAR sd[MAX_PATH] =  TEXT("SYSTEM\\CurrentControlSet\\Services\\nlwfse\\ShadowDevices\\pc");

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     sd,
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS
                      ) {

        retCode = RegQueryInfoKey(
                                hKey,                    // key handle 
                                achClass,                // buffer for class name 
                                &cchClassName,           // size of class string 
                                NULL,                    // reserved 
                                &cSubKeys,               // number of subkeys 
                                &cbMaxSubKey,            // longest subkey size 
                                &cchMaxClass,            // longest class string 
                                &cValues,                // number of values for this key 
                                &cchMaxValue,            // longest value name 
                                &cbMaxValueData,         // longest value data 
                                &cbSecurityDescriptor,   // security descriptor 
                                &ftLastWriteTime);       // last write time 

          // Enumerate the subkeys, until RegEnumKeyEx fails.

       if (cSubKeys) {
           printf("\nNumber of subkeys: %d\n", cSubKeys);

           for (i = 0; i<cSubKeys; i++) {
              cbName = MAX_KEY_LENGTH;
              memset(sd, 0, MAX_PATH);
              WCHAR sd[MAX_PATH] = TEXT("SYSTEM\\CurrentControlSet\\Services\\nlwfse\\ShadowDevices\\pc");

              retCode = RegEnumKeyEx(hKey, i,
                                      achKey,
                                      &cbName,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &ftLastWriteTime);

              if (retCode == ERROR_SUCCESS) {
                  StringCchCat(sd, 260, TEXT("\\"));
                  StringCchCat(sd, 260, achKey); 

                  retCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                          sd,
                                          0,
                                          KEY_READ,
                                          &hVolKey);

                  if (retCode == 0) {
                      retCode = RegQueryInfoKey(
                                              hVolKey,
                                              achClass,
                                              &cchClassName,  
                                              NULL,          
                                              &volSubKeys,  
                                              &cbMaxSubKey,
                                              &cchMaxClass, 
                                              &volValues,  
                                              &cchMaxValue,
                                              &cbMaxValueData, 
                                              &cbSecurityDescriptor,
                                              &ftLastWriteTime);   

                      if (volValues) {
                          for (DWORD j = 0, retCode = ERROR_SUCCESS; j<volValues; j++) {
                              cchValue = MAX_VALUE_NAME;
                              achValue[0] = '\0';
                              BYTE lpKeyData[256] = { 0 };
                              DWORD dsize;

                              retCode = RegEnumValue(hVolKey, j,
                                                    achValue,
                                                    &cchValue,
                                                    NULL,
                                                    NULL,
                                                    (LPBYTE)lpKeyData,
                                                    &dsize);


                              if (retCode == ERROR_SUCCESS) {
                                  WCHAR *ssidstr1, *ssidstr2;
                                  static GUID vsguid;
                                  std::wstring sid((wchar_t *)lpKeyData);

                                  HRESULT result = ::CLSIDFromString(W2OLE(const_cast<WCHAR*>(sid.c_str())), &vsguid);
                                  ssidstr1 = Guid2Wchar(vsguid); 
                                  ssidstr2 = Guid2Wchar(sdevId); 
                                  if (wcscmp(ssidstr1, ssidstr2) == 0) {
                                      RegCloseKey(hVolKey);
                                      RegCloseKey(hKey);
                                      printf("Found sd id in registry \n");
                                      return TRUE;
                                  }
                               }
                            }
                      }

               } //volkey value
           }
       }
       RegCloseKey(hVolKey);
    }
    RegCloseKey(hKey);
  }
  return FALSE;

}

bool GetCesdkPath()
{
    HKEY key;
    DWORD dtype = REG_SZ;
    DWORD cbData;
    DWORD dwRet;
    TCHAR lpData[1024] = {0};
    char logmsg[MAX_PATH];
    int wlen;

    TCHAR insd[260] = TEXT("SOFTWARE\\NextLabs\\CommonLibraries");

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE,
                         insd,
			 &key);

    if (dwRet != ERROR_SUCCESS) {
	memset(logmsg, 0, sizeof(logmsg));
	wlen = _snprintf(logmsg, MAX_PATH, "GetCesdkPath: RegOpenKey ERROR %d \r\n", GetLastError());
	EnterCriticalSection(&logfileLock);
	WritelogMsg(logmsg);
	LeaveCriticalSection(&logfileLock);
        return FALSE;
    }
    cbData = sizeof(lpData);
    dwRet = RegQueryValueEx(key,
			TEXT("InstallDir"),
			NULL,
			&dtype,
			(LPBYTE)lpData,
			&cbData);

    if (dwRet == ERROR_SUCCESS) {
        wstring sdset((wchar_t *)lpData);
	memset(cesdkPath, 0, 1024);

        int slen = (int)wcslen(sdset.c_str());
        const wchar_t *s2 = sdset.c_str();
        errno_t err = wcsncpy_s(cesdkPath, 1024, s2, slen);

        StringCchCat(cesdkPath, 1024, TEXT("bin64"));
        string regpath = ws2s(cesdkPath);

	memset(logmsg, 0, sizeof(logmsg));
	wlen = _snprintf(logmsg, MAX_PATH, "GetCesdkPath %s \r\n", regpath.c_str());
	EnterCriticalSection(&logfileLock);
	WritelogMsg(logmsg);
	LeaveCriticalSection(&logfileLock);

        return TRUE;
    } else {
	memset(logmsg, 0, sizeof(logmsg));
	wlen = _snprintf(logmsg, MAX_PATH, "GetCesdkPath ERROR %d %d \r\n", dwRet, GetLastError());
	EnterCriticalSection(&logfileLock);
	WritelogMsg(logmsg);
	LeaveCriticalSection(&logfileLock);

    }
    return FALSE;
}


