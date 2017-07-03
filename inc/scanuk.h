/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    scanuk.h

Abstract:

    Header file which contains the structures, type definitions,
    constants, global variables and function prototypes that are
    shared between kernel and user mode.

Environment:

    Kernel & user mode

--*/

#ifndef __SCANUK_H__
#define __SCANUK_H__

//
//  Name of port used to communicate
//

const PWSTR ScannerPortName = L"\\ScannerPort";


#define SCANNER_READ_BUFFER_SIZE   1024
#define MAX_FILE_PATH 1024
#define FILE_BUF_SIZE 4096

typedef struct _SCANNER_NOTIFICATION {
    //rename을 위해 다른 정보들 추가 해야함.
    ULONG BytesToScan;
	ULONG Reserved;
    ULONG fltType;
	ULONG ulPID;			    // pid
	ULONG ulTID;			    // tid
    //CHAR  OrgFileName;
    CHAR  RenameFileName;
    ULONG SharedmodeWrite;
	ULONG CreateOptions;
	CHAR isDir;
	UCHAR modeDelete;
    UCHAR Contents[SCANNER_READ_BUFFER_SIZE];
	UCHAR OrgFileName[SCANNER_READ_BUFFER_SIZE];
    
} SCANNER_NOTIFICATION, *PSCANNER_NOTIFICATION;


typedef struct _USER_NOTIFICATION {

	ULONG user_pid;

} USER_NOTIFICATION, *PUSER_NOTIFICATION;


typedef struct _USER_REPLY_FILE { // 파일 읽기용

	ULONG nSize;
	UCHAR Contents[FILE_BUF_SIZE];

} USER_REPLY_FILE, *PUSER_REPLY_FILE;


typedef struct _SCANNER_REPLY {

    BOOLEAN SafeToOpen;
	UCHAR cmdType; // 0: nop, 1: block, 100: backup, 200: read_file
	UCHAR Contents[SCANNER_READ_BUFFER_SIZE];
    
} SCANNER_REPLY, *PSCANNER_REPLY;


//
typedef enum _MY_IRP_FILTER_TYPE
{
	fltType_PreCreate = 100,
	fltType_PostCreate = 200,
	fltType_PreClose = 300,
	fltType_PostClose = 400,
	fltType_PreCleanup = 500,
	fltType_PostCleanup = 600,
	fltType_PreWrite = 700,
	fltType_PostWrite = 800,
	fltType_PreSetInformation = 900,
	fltType_PostSetInformation = 1000,
	fltType_Rename = 2000

} MY_IRP_FILTER_TYPE;


#endif //  __SCANUK_H__


