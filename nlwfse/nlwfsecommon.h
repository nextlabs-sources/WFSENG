

#ifndef __NLWFSECOMMON_H__
#define __NLWFSECOMMON_H__

const PWSTR wfsePortName = L"\\wfsePort";

#define WFSE_MAX_BUFFER_SIZE   12468
#define MAXSIDLENGTH 184
#define MAXIPV6LENGTH   64

#define ALLOW_READ             1
#define ALLOW_WRITE            2
#define ALLOW_DELETE           4
#define ALLOW_RENAME           8
//#define ALLOW_MOVE             0x00000010
#define ALLOW_CHANGE_ATTRS     16 
#define ALLOW_CHANGE_SEC_ATTRS 32
#define DENY_OPEN              64 
#define DENY_WRITE             128
#define DENY_DELETE            256
#define DENY_RENAME            512
//#define DENY_MOVE              0x00000800
#define DENY_CHANGE_ATTRS      1024
#define DENY_CHANGE_SEC_ATTRS  2048
#define WFSE_RESET_CACHE       4096
#define WFSE_SENDMSG_ERROR     8192
#define WFSE_OPEN_WRITE     16384

#define WFSE_FILE_OPEN 1
#define WFSE_FILE_READ 2
#define WFSE_FILE_WRITE 4
#define WFSE_FILE_DELETE 8
#define WFSE_FILE_RENAME 16
#define WFSE_FILE_CHANGE_ATTRS 32
#define WFSE_FILE_CHANGE_SECS 64
#define WFSE_FILE_CACHE_OPEN 128


typedef enum _WFSE_FILE_ACCESS {
    fileOpen,
    fileRead,
    fileWrite,
    fileDelete,
    fileRename,
    fileMove,
    fileChangeAttr,
    fileChangeSec,
    drvunload,
    NumFileAction
} WFSE_FILE_ACCESS;

typedef struct _NLWFSE_ACCESS_INFO {
    ULONG ipaddr;
    ULONG sidLength;
    WCHAR sidString[MAXSIDLENGTH];
    ULONG filePathLength;
    WCHAR filePath[WFSE_MAX_BUFFER_SIZE];
    ULONG desiredAccess;
    ULONG faction;
    //enum WFSE_FILE_ACCESS faction;
} NLWFSE_ACCESS_INFO, *PNLWFSE_ACCESS_INFO;

typedef struct _NLWFSE_REPLY {
    ULONG allowedAccess;
} NLWFSE_REPLY, *PNLWFSE_REPLY;

typedef struct _WFSE_PORT_CONTEXT {
   int pcid;
} WFSE_PORT_CONTEXT, *PWFSE_PORT_CONTEXT;

#define WFSE_ACCESSINFO_BUFFER_SIZE sizeof(NLWFSE_ACCESS_INFO)

#endif //  __NLWFSECOMMON_H__


