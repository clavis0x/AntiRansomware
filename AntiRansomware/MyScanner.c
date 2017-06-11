/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

scanner.c

Abstract:

This is the main module of the scanner filter.

This filter scans the data in a file before allowing an open to proceed.  This is similar
to what virus checkers do.

Environment:

Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "scanuk.h"
#include "MyScanner.h"

ULONG g_userPID = 0; // User-Process PID

// Underline --
#define PagedPool 1
typedef PVOID PSECURITY_DESCRIPTOR;


#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

//
//  Structure that contains all the global data structures
//  used throughout the scanner.
//

SCANNER_DATA ScannerData;

//
//  This is a static list of file name extensions files we are interested in scanning
//

const UNICODE_STRING ScannerExtensionsToScan[] =
{
	RTL_CONSTANT_STRING(L"txt"),
	RTL_CONSTANT_STRING(L"hwp"), // Office
	RTL_CONSTANT_STRING(L"doc"),
	RTL_CONSTANT_STRING(L"docx"),
	RTL_CONSTANT_STRING(L"ppt"),
	RTL_CONSTANT_STRING(L"pptx"),
	RTL_CONSTANT_STRING(L"xls"),
	RTL_CONSTANT_STRING(L"xlsx"),
	RTL_CONSTANT_STRING(L"c"), // Source code
	RTL_CONSTANT_STRING(L"cpp"),
	RTL_CONSTANT_STRING(L"h"),
	RTL_CONSTANT_STRING(L"hpp"),
	RTL_CONSTANT_STRING(L"bmp"), // Graphic
	RTL_CONSTANT_STRING(L"jpg"),
	RTL_CONSTANT_STRING(L"gif"),
	RTL_CONSTANT_STRING(L"png"),
	RTL_CONSTANT_STRING(L"zip"), // Pack
	RTL_CONSTANT_STRING(L"rar"),
	RTL_CONSTANT_STRING(L"c"),
	/*RTL_CONSTANT_STRING( L"ini"),   Removed, to much usage*/
	{ 0, 0, NULL }
};


//
//  Function prototypes
//

NTSTATUS
ScannerPortConnect(
	__in PFLT_PORT ClientPort,
	__in_opt PVOID ServerPortCookie,
	__in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
	__in ULONG SizeOfContext,
	__deref_out_opt PVOID *ConnectionCookie
);

VOID
ScannerPortDisconnect(
	__in_opt PVOID ConnectionCookie
);

VOID
ScannerMessageNotify(
	IN PVOID PortCookie,
	IN PVOID InputBuffer OPTIONAL,
	IN ULONG InputBufferLength,
	OUT PVOID OutputBuffer OPTIONAL,
	IN ULONG OutputBufferLength,
	OUT PULONG ReturnOutputBufferLength
);

NTSTATUS
ScannerpScanFileInUserMode(
	__in PFLT_INSTANCE Instance,
	__in PFILE_OBJECT FileObject,
	__out PBOOLEAN SafeToOpen
);

BOOLEAN
ScannerpCheckExtension(
	__in PUNICODE_STRING Extension
);

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, ScannerInstanceSetup)
#pragma alloc_text(PAGE, ScannerPreCreate)
#pragma alloc_text(PAGE, ScannerPortConnect)
#pragma alloc_text(PAGE, ScannerPortDisconnect)
#endif


//
//  Constant FLT_REGISTRATION structure for our filter.  This
//  initializes the callback routines our filter wants to register
//  for.  This is only used to register with the filter manager
//

const FLT_OPERATION_REGISTRATION Callbacks[] = {

	{ IRP_MJ_CREATE,
	0,
	ScannerPreCreate,
	ScannerPostCreate },

	{ IRP_MJ_CLOSE,
	0,
	ScannerPreClose,
	ScannerPostClose },

	{ IRP_MJ_CLEANUP,
	0,
	ScannerPreCleanup,
	ScannerPostCleanup },

	{ IRP_MJ_WRITE,
	0,
	ScannerPreWrite,
	ScannerPostWrite },

	{ IRP_MJ_SET_INFORMATION,
	0,
	ScannerPreSetInformation,
	ScannerPostSetInformation },

	{ IRP_MJ_OPERATION_END }
};


const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

	{ FLT_STREAMHANDLE_CONTEXT,
	0,
	NULL,
	sizeof(SCANNER_STREAM_HANDLE_CONTEXT),
	'chBS' },

	{ FLT_CONTEXT_END }
};

const FLT_REGISTRATION FilterRegistration = {

	sizeof(FLT_REGISTRATION),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags
	ContextRegistration,                //  Context Registration.
	Callbacks,                          //  Operation callbacks
	ScannerUnload,                      //  FilterUnload
	ScannerInstanceSetup,               //  InstanceSetup
	ScannerQueryTeardown,               //  InstanceQueryTeardown
	NULL,                               //  InstanceTeardownStart
	NULL,                               //  InstanceTeardownComplete
	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent
};

////////////////////////////////////////////////////////////////////////////
//
//    Filter initialization and unload routines.
//
////////////////////////////////////////////////////////////////////////////

NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT DriverObject,
	__in PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

This is the initialization routine for the Filter driver.  This
registers the Filter with the filter manager and initializes all
its global data structures.

Arguments:

DriverObject - Pointer to driver object created by the system to
represent this driver.

RegistryPath - Unicode string identifying where the parameters for this
driver are located in the registry.

Return Value:

Returns STATUS_SUCCESS.
--*/
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uniString;
	PSECURITY_DESCRIPTOR sd;
	NTSTATUS status;

	UNREFERENCED_PARAMETER(RegistryPath);

	//
	//  Register with filter manager.
	//

	status = FltRegisterFilter(DriverObject,
		&FilterRegistration,
		&ScannerData.Filter);


	if (!NT_SUCCESS(status)) {

		return status;
	}

	//
	//  Create a communication port.
	//

	RtlInitUnicodeString(&uniString, ScannerPortName);

	//
	//  We secure the port so only ADMINs & SYSTEM can acecss it.
	//

	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);

	if (NT_SUCCESS(status)) {

		InitializeObjectAttributes(&oa,
			&uniString,
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
			NULL,
			sd);

		status = FltCreateCommunicationPort(ScannerData.Filter,
			&ScannerData.ServerPort,
			&oa,
			NULL,
			ScannerPortConnect,
			ScannerPortDisconnect,
			ScannerMessageNotify,
			1);
		//
		//  Free the security descriptor in all cases. It is not needed once
		//  the call to FltCreateCommunicationPort() is made.
		//

		FltFreeSecurityDescriptor(sd);

		if (NT_SUCCESS(status)) {

			//
			//  Start filtering I/O.
			//

			status = FltStartFiltering(ScannerData.Filter);

			if (NT_SUCCESS(status)) {

				return STATUS_SUCCESS;
			}

			FltCloseCommunicationPort(ScannerData.ServerPort);
		}
	}

	FltUnregisterFilter(ScannerData.Filter);

	return status;
}


NTSTATUS
ScannerPortConnect(
	__in PFLT_PORT ClientPort,
	__in_opt PVOID ServerPortCookie,
	__in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
	__in ULONG SizeOfContext,
	__deref_out_opt PVOID *ConnectionCookie
)
/*++

Routine Description

This is called when user-mode connects to the server port - to establish a
connection

Arguments

ClientPort - This is the client connection port that will be used to
send messages from the filter

ServerPortCookie - The context associated with this port when the
minifilter created this port.

ConnectionContext - Context from entity connecting to this port (most likely
your user mode service)

SizeofContext - Size of ConnectionContext in bytes

ConnectionCookie - Context to be passed to the port disconnect routine.

Return Value

STATUS_SUCCESS - to accept the connection

--*/
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie);

	ASSERT(ScannerData.ClientPort == NULL);
	ASSERT(ScannerData.UserProcess == NULL);

	//
	//  Set the user process and port.
	//

	ScannerData.UserProcess = PsGetCurrentProcess();
	ScannerData.ClientPort = ClientPort;

	DbgPrint("!!! scanner.sys --- connected, port=0x%p\n", ClientPort);

	return STATUS_SUCCESS;
}


VOID
ScannerPortDisconnect(
	__in_opt PVOID ConnectionCookie
)
/*++

Routine Description

This is called when the connection is torn-down. We use it to close our
handle to the connection

Arguments

ConnectionCookie - Context from the port connect routine

Return value

None

--*/
{
	UNREFERENCED_PARAMETER(ConnectionCookie);

	PAGED_CODE();

	DbgPrint("!!! scanner.sys --- disconnected, port=0x%p\n", ScannerData.ClientPort);

	//
	//  Close our handle to the connection: note, since we limited max connections to 1,
	//  another connect will not be allowed until we return from the disconnect routine.
	//

	FltCloseClientPort(ScannerData.Filter, &ScannerData.ClientPort);

	//
	//  Reset the user-process field.
	//

	ScannerData.UserProcess = NULL;
}


VOID
ScannerMessageNotify(
	IN PVOID PortCookie,
	IN PVOID InputBuffer OPTIONAL,
	IN ULONG InputBufferLength,
	OUT PVOID OutputBuffer OPTIONAL,
	IN ULONG OutputBufferLength,
	OUT PULONG ReturnOutputBufferLength
)
{
	PUSER_NOTIFICATION notification;

	char szRecvTest[100] = { 0 };

	notification = (PUSER_NOTIFICATION)InputBuffer;
	g_userPID = notification->user_pid;

	DbgPrint("[Anti-Rs] ===== ScannerMessageNotify!! =====\n");
	DbgPrint("[Anti-Rs] User-PID: %d\n", notification->user_pid);
}


NTSTATUS
ScannerUnload(
	__in FLT_FILTER_UNLOAD_FLAGS Flags
)
/*++

Routine Description:

This is the unload routine for the Filter driver.  This unregisters the
Filter with the filter manager and frees any allocated global data
structures.

Arguments:

None.

Return Value:

Returns the final status of the deallocation routines.

--*/
{
	UNREFERENCED_PARAMETER(Flags);

	//
	//  Close the server port.
	//

	FltCloseCommunicationPort(ScannerData.ServerPort);

	//
	//  Unregister the filter
	//

	FltUnregisterFilter(ScannerData.Filter);

	return STATUS_SUCCESS;
}

NTSTATUS
ScannerInstanceSetup(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_SETUP_FLAGS Flags,
	__in DEVICE_TYPE VolumeDeviceType,
	__in FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
/*++

Routine Description:

This routine is called by the filter manager when a new instance is created.
We specified in the registry that we only want for manual attachments,
so that is all we should receive here.

Arguments:

FltObjects - Describes the instance and volume which we are being asked to
setup.

Flags - Flags describing the type of attachment this is.

VolumeDeviceType - The DEVICE_TYPE for the volume to which this instance
will attach.

VolumeFileSystemType - The file system formatted on this volume.

Return Value:

FLT_NOTIFY_STATUS_ATTACH              - we wish to attach to the volume
FLT_NOTIFY_STATUS_DO_NOT_ATTACH       - no, thank you

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);

	PAGED_CODE();

	ASSERT(FltObjects->Filter == ScannerData.Filter);

	//
	//  Don't attach to network volumes.
	//

	if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {

		return STATUS_FLT_DO_NOT_ATTACH;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
ScannerQueryTeardown(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

This is the instance detach routine for the filter. This
routine is called by filter manager when a user initiates a manual instance
detach. This is a 'query' routine: if the filter does not want to support
manual detach, it can return a failure status

Arguments:

FltObjects - Describes the instance and volume for which we are receiving
this query teardown request.

Flags - Unused

Return Value:

STATUS_SUCCESS - we allow instance detach to happen

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	return STATUS_SUCCESS;
}


BOOLEAN
ScannerpCheckExtension(
	__in PUNICODE_STRING Extension
)
/*++

Routine Description:

Checks if this file name extension is something we are interested in

Arguments

Extension - Pointer to the file name extension

Return Value

TRUE - Yes we are interested
FALSE - No
--*/
{
	const UNICODE_STRING *ext;

	if (Extension->Length == 0) {

		return FALSE;
	}

	//
	//  Check if it matches any one of our static extension list
	//

	ext = ScannerExtensionsToScan;

	while (ext->Buffer != NULL) {

		if (RtlCompareUnicodeString(Extension, ext, TRUE) == 0) {

			//
			//  A match. We are interested in this file
			//

			return TRUE;
		}
		ext++;
	}

	return FALSE;
}


BOOLEAN MyCheckFileExt(__inout PFLT_CALLBACK_DATA Data)
{

	PFLT_FILE_NAME_INFORMATION nameInfo;
	NTSTATUS status;
	BOOLEAN safeToOpen, scanFile;

	status = FltGetFileNameInformation(Data,
		FLT_FILE_NAME_NORMALIZED |
		FLT_FILE_NAME_QUERY_DEFAULT,
		&nameInfo);

	if (!NT_SUCCESS(status)) {

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	FltParseFileNameInformation(nameInfo);

	//
	//  Check if the extension matches the list of extensions we are interested in
	//

	scanFile = ScannerpCheckExtension(&nameInfo->Extension);

	if (!scanFile) {

		//
		//  Not an extension we are interested in
		//

		FltReleaseFileNameInformation(nameInfo);
		return FALSE;
	}

	//
	//  Release file name info, we're done with it
	//

	FltReleaseFileNameInformation(nameInfo);

	return TRUE;
}


ULONG DbgPrintInformation(MY_IRP_FILTER_TYPE nFltType, PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects)
{
	ULONG nResult = 0;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	PFILE_RENAME_INFORMATION renameInfo = NULL;
	NTSTATUS status;
	wchar_t * pMytestPath = NULL;
	wchar_t * pMytestPath2 = NULL;
	BOOLEAN isDelete = FALSE;

	struct {
		HANDLE RootDirectory;
		ULONG FileNameLength;
		PWSTR FileName;
	} setInfo;

	if (g_userPID == PsGetProcessId(IoThreadToProcess(Data->Thread)))
		return;

	if (FlagOn(Data->Iopb->IrpFlags, IRP_CREATE_OPERATION)) {
		if (Data->IoStatus.Information == FILE_CREATED) {
			isDelete = TRUE;
		}
	}

	if(!isDelete){
		//Write로 접근하는 정보만 출력.. 
		if (FltObjects->FileObject->WriteAccess != 1) {

			//파일이름 변경일 경우 SetInformation 
			if (nFltType == fltType_PreSetInformation || nFltType == fltType_PostSetInformation)
			{
				//rename 일경우 Write 모드가 아니고. Delete 모드가 활성화 된다. 
				//내부적으로 MoveFile 을 해주는 듯함.. 새로운 파일 생성하고, 기존 파일 지우는 방식으로.. 
				// FltObjects가 같을 경우에는 rename이므로 비교가 필요(동일한 패스의 파일이 동일한 FltObject인지)
				if (FltObjects->FileObject->DeleteAccess != 1 || FltObjects->FileObject->SharedDelete != 1)
					return;
			}
			else {
				return;
			}
		}
	}

	/*
	//확장자부터 확인하자.. 로그가 너무 많이 나옴.. 
	if (MyCheckFileExt(Data) == FALSE)
		return;
	*/

	//step 1 : 파일명 구하기.. 
	if (nFltType != fltType_PreSetInformation){
		status = FltGetFileNameInformation(Data,
			FLT_FILE_NAME_NORMALIZED |
			FLT_FILE_NAME_QUERY_DEFAULT,
			&nameInfo);

		if (NT_SUCCESS(status) == FALSE) {
			return;
		}
	}
	else {
		if (Data->Iopb->Parameters.SetFileInformation.FileInformationClass != FileRenameInformation)
			return;

		// 파일명 변경 - 원본 파일명과 변경된 파일명 구하기
		renameInfo = (PFILE_RENAME_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
		setInfo.RootDirectory = renameInfo->RootDirectory;
		setInfo.FileNameLength = renameInfo->FileNameLength;
		setInfo.FileName = renameInfo->FileName;

		status = FltGetDestinationFileNameInformation(FltObjects->Instance,
			FltObjects->FileObject,
			setInfo.RootDirectory,
			setInfo.FileName,
			setInfo.FileNameLength,
			FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER | FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT,
			&nameInfo);
		if (!NT_SUCCESS(status))
			return;

		status = FltParseFileNameInformation(nameInfo);
		if (!NT_SUCCESS(status))
			return;
	}

	/*
	//감시경로지정.. 
	pMytestPath = wcsstr(nameInfo->Name.Buffer, L"MyTest");
	if (pMytestPath == NULL) {
		if (nFltType == fltType_PreSetInformation)
			pMytestPath2 = wcsstr(FltObjects->FileObject->FileName.Buffer, L"MyTest");
		if (pMytestPath == NULL && pMytestPath2 == NULL) {
			FltReleaseFileNameInformation(nameInfo);
			return;
		}
	}
	*/

	// User Process에게 통보
	switch (nFltType)
	{
		case fltType_PreCreate:
			DbgPrint("[Anti-Rs] ScannerPreCreate\n");
			break;
		case fltType_PostCreate:
			DbgPrint("[Anti-Rs] ScannerPostCreate\n");
			//DbgPrint("[Anti-Rs]  ㄴ Name: %S", nameInfo->Name.Buffer);
			//DbgPrint("[Anti-Rs]  ㄴ R: %d, W: %d, D: %d", FltObjects->FileObject->ReadAccess, FltObjects->FileObject->WriteAccess, FltObjects->FileObject->DeleteAccess);
			//DbgPrint("[Anti-Rs]  ㄴ SR: %d, SW: %d, SD: %d", FltObjects->FileObject->SharedRead, FltObjects->FileObject->SharedWrite, FltObjects->FileObject->SharedDelete);
			nResult = RFNotifyUserProcess(fltType_PostCreate, nameInfo->Name.Length, nameInfo->Name.Buffer, FltObjects, nameInfo, Data);
			break;
		case fltType_PreClose:
			DbgPrint("[Anti-Rs] ScannerPreClose\n");
			break;
		case fltType_PostClose:
			DbgPrint("[Anti-Rs] ScannerPostClose\n");
			break;
		case fltType_PreCleanup:
			DbgPrint("[Anti-Rs] ScannerPreCleanup\n");
			nResult = RFNotifyUserProcess(fltType_PreCleanup, nameInfo->Name.Length, nameInfo->Name.Buffer, FltObjects, nameInfo, Data);
			break;
		case fltType_PostCleanup:
			DbgPrint("[Anti-Rs] ScannerPostCleanup\n");
			break;
		case fltType_PreWrite:
			DbgPrint("[Anti-Rs] ScannerPreWrite\n");
			//if (Data->Iopb->MajorFunction == IRP_MJ_WRITE && Data->Iopb->Parameters.Write.Length > 0 && FlagOn(FltObjects->FileObject->Flags, FO_FILE_MODIFIED))
			//	RFNotifyUserProcess(fltType_PreWrite, nameInfo->Name.Length, nameInfo->Name.Buffer, FltObjects, nameInfo, Data);
			break;
		case fltType_PostWrite:
			DbgPrint("[Anti-Rs] ScannerPostWrite\n");
			//RFNotifyUserProcess(fltType_PostWrite, nameInfo->Name.Length, nameInfo->Name.Buffer, FltObjects, nameInfo, Data);
			break;
		case fltType_PreSetInformation:
			DbgPrint("[Anti-Rs] ScannerPreSetInformation\n");
			//DbgPrint("[Anti-Rs]  ㄴ src: %S\n", FltObjects->FileObject->FileName.Buffer);
			//DbgPrint("[Anti-Rs]  ㄴ dst: %S\n", nameInfo->Name.Buffer);
			//DbgPrint("[Anti-Rs]  ㄴ R: %d, W: %d, D: %d", FltObjects->FileObject->ReadAccess, FltObjects->FileObject->WriteAccess, FltObjects->FileObject->DeleteAccess);
			//DbgPrint("[Anti-Rs]  ㄴ SR: %d, SW: %d, SD: %d", FltObjects->FileObject->SharedRead, FltObjects->FileObject->SharedWrite, FltObjects->FileObject->SharedDelete);
			nResult = RFNotifyUserProcess(fltType_PreSetInformation, nameInfo->Name.Length, nameInfo->Name.Buffer, FltObjects, nameInfo, Data);
			break;
		case fltType_PostSetInformation:
			DbgPrint("[Anti-Rs] ScannerPostSetInformation\n");
			//DbgPrint("[Anti-Rs]  ㄴ Name: %S", nameInfo->Name.Buffer);
			//DbgPrint("[Anti-Rs]  ㄴ R: %d, W: %d, D: %d", FltObjects->FileObject->ReadAccess, FltObjects->FileObject->WriteAccess, FltObjects->FileObject->DeleteAccess);
			//DbgPrint("[Anti-Rs]  ㄴ SR: %d, SW: %d, SD: %d", FltObjects->FileObject->SharedRead, FltObjects->FileObject->SharedWrite, FltObjects->FileObject->SharedDelete);
			nResult = RFNotifyUserProcess(fltType_PostSetInformation, nameInfo->Name.Length, nameInfo->Name.Buffer, FltObjects, nameInfo, Data);
			break;
		default: 
			DbgPrint("[Anti-Rs] ############ fltType_is Error... !! ############ \n");
			break;
	}

	FltReleaseFileNameInformation(nameInfo);

	return nResult;
}


ULONG RFNotifyUserProcess(int fltType, int pathLength, wchar_t * pPath, PCFLT_RELATED_OBJECTS FltObjects, PFLT_FILE_NAME_INFORMATION nameInfo, PFLT_CALLBACK_DATA Data)
{
	ULONG nResult = 0;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG replyLength;
	PSCANNER_NOTIFICATION notification = NULL;
	BOOLEAN isDirectory;

	//DbgPrint("fltType is %d, length = %d , path = %S , AND size is %d\n",fltType,pathLength,pPath, sizeof( SCANNER_NOTIFICATION ));

	if (ScannerData.ClientPort == NULL)
	{
		return FALSE;
	}

	notification = ExAllocatePoolWithTag(NonPagedPool, sizeof(SCANNER_NOTIFICATION), '2939');

	if (notification == NULL)
	{
		return FALSE;
	}

	notification->BytesToScan = pathLength;
	notification->Reserved = fltType;
	notification->ulPID = PsGetProcessId(IoThreadToProcess(Data->Thread));
	notification->ulTID = Data->Thread;
	//notification->OrgFileName = FltObjects->FileObject->FileName.Buffer;
	notification->RenameFileName = nameInfo->Name.Buffer;
	notification->SharedmodeWrite = FltObjects->FileObject->SharedWrite;
	RtlCopyMemory(&notification->Contents, pPath, notification->BytesToScan);
	RtlZeroMemory(&notification->OrgFileName, SCANNER_READ_BUFFER_SIZE);
	RtlCopyMemory(&notification->OrgFileName, FltObjects->FileObject-> FileName.Buffer, FltObjects->FileObject->FileName.Length);
	//RtlCopyMemory( &notification->OrgFileName, pPath, notification->BytesToScan );
	//RtlCopyMemory(&notification->OrgFileName, FltObjects->FileObject->FileName.Buffer, notification->BytesToScan);

	// Create Option
	notification->CreateOptions = 0;
	if (FlagOn(Data->Iopb->IrpFlags, IRP_CREATE_OPERATION)) {
		if (Data->IoStatus.Information == FILE_CREATED)
		{
			notification->CreateOptions = 1;
		}
		else if (Data->IoStatus.Information == FILE_OVERWRITTEN)
		{
			notification->CreateOptions = 2;
		}
	}

	// File(0) or Directory(1)
	FltIsDirectory(Data->Iopb->TargetFileObject, FltObjects->Instance, &isDirectory);
	if (isDirectory)
		notification->isDir = 1;
	else
		notification->isDir = 0;
	/*
	if (Data->Iopb->Parameters.Create.Options & FILE_DIRECTORY_FILE)
		notification->isDir = 1;
	else
		notification->isDir = 0;
	*/

	notification->modeDelete = 0;
	if (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation)
	{
		PFILE_DISPOSITION_INFORMATION info = (PFILE_DISPOSITION_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
		if (info != NULL && info->DeleteFile)
		{
			notification->modeDelete = 1;
		}
	}
	
	//notification->Contents 맨뒤에 널처리 해준다. 
	*(wchar_t*)&notification->Contents[notification->BytesToScan] = L'\0';

	replyLength = sizeof(SCANNER_REPLY);
	status = FltSendMessage(ScannerData.Filter,
		&ScannerData.ClientPort,
		notification,
		sizeof(SCANNER_NOTIFICATION),
		notification,
		&replyLength,
		NULL);

	if (STATUS_SUCCESS == status)
	{
		DbgPrint("replyData is %d", ((PSCANNER_REPLY)notification)->SafeToOpen);
		if (((PSCANNER_REPLY)notification)->cmdType == 1)
		{
			nResult = 1;
			DbgPrint("[Anti-Rs] D E N Y !\n");
		}
		else if (((PSCANNER_REPLY)notification)->cmdType == 100) {
			nResult = 100;
			DbgPrint("[Anti-Rs] B A C K U P !\n");
		}
	}


	if (notification != NULL)
	{
		ExFreePoolWithTag(notification, '2939');
	}

	return nResult;
}


FLT_PREOP_CALLBACK_STATUS
ScannerPreCreate(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID *CompletionContext
)
/*++

Routine Description:

Pre create callback.  We need to remember whether this file has been
opened for write access.  If it has, we'll want to rescan it in cleanup.
This scheme results in extra scans in at least two cases:
-- if the create fails (perhaps for access denied)
-- the file is opened for write access but never actually written to
The assumption is that writes are more common than creates, and checking
or setting the context in the write path would be less efficient than
taking a good guess before the create.

Arguments:

Data - The structure which describes the operation parameters.

FltObject - The structure which describes the objects affected by this
operation.

CompletionContext - Output parameter which can be used to pass a context
from this pre-create callback to the post-create callback.

Return Value:

FLT_PREOP_SUCCESS_WITH_CALLBACK - If this is not our user-mode process.
FLT_PREOP_SUCCESS_NO_CALLBACK - All other threads.

--*/
{
	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	wchar_t * pMytestPath = NULL;

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	PAGED_CODE();

	DbgPrintInformation(fltType_PreCreate, Data, FltObjects);

	// Sample
	/*
	//
	//  See if this create is being done by our user process.
	//
	
	if (IoThreadToProcess(Data->Thread) == ScannerData.UserProcess) {

		//DbgPrint("!!! scanner.sys -- allowing create for trusted process \n");
		DbgPrint("[Anti-Rs] ScannerPreCreate / By User Process\n");

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	*/

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS
ScannerPostCreate(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in_opt PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
)
/*++

Routine Description:

Post create callback.  We can't scan the file until after the create has
gone to the filesystem, since otherwise the filesystem wouldn't be ready
to read the file for us.

Arguments:

Data - The structure which describes the operation parameters.

FltObject - The structure which describes the objects affected by this
operation.

CompletionContext - The operation context passed fron the pre-create
callback.

Flags - Flags to say why we are getting this post-operation callback.

Return Value:

FLT_POSTOP_FINISHED_PROCESSING - ok to open the file or we wish to deny
access to this file, hence undo the open

--*/
{
	PSCANNER_STREAM_HANDLE_CONTEXT scannerContext;
	FLT_POSTOP_CALLBACK_STATUS returnStatus = FLT_POSTOP_FINISHED_PROCESSING;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	NTSTATUS status;
	BOOLEAN safeToOpen, scanFile;

	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	//
	//  If this create was failing anyway, don't bother scanning now.
	//

	if (!NT_SUCCESS(Data->IoStatus.Status) ||
		(STATUS_REPARSE == Data->IoStatus.Status)) {

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	DbgPrintInformation(fltType_PostCreate, Data, FltObjects);

	
	// Sample: 파일 정보 구하기?
	/*
	//
	//  Check if we are interested in this file.
	//

	status = FltGetFileNameInformation(Data,
		FLT_FILE_NAME_NORMALIZED |
		FLT_FILE_NAME_QUERY_DEFAULT,
		&nameInfo);

	if (!NT_SUCCESS(status)) {

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	FltParseFileNameInformation(nameInfo);

	//
	//  Check if the extension matches the list of extensions we are interested in
	//

	scanFile = ScannerpCheckExtension(&nameInfo->Extension);

	//
	//  Release file name info, we're done with it
	//

	FltReleaseFileNameInformation(nameInfo);

	if (!scanFile) {

		//
		//  Not an extension we are interested in
		//

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	(VOID)ScannerpScanFileInUserMode(FltObjects->Instance,
		FltObjects->FileObject,
		&safeToOpen);

	if (!safeToOpen) {

		//
		//  Ask the filter manager to undo the create.
		//

		DbgPrint("!!! scanner.sys -- foul language detected in postcreate !!!\n");

		DbgPrint("!!! scanner.sys -- undoing create \n");

		FltCancelFileOpen(FltObjects->Instance, FltObjects->FileObject);

		Data->IoStatus.Status = STATUS_ACCESS_DENIED;
		Data->IoStatus.Information = 0;

		returnStatus = FLT_POSTOP_FINISHED_PROCESSING;

	}
	else if (FltObjects->FileObject->WriteAccess) {

		//
		//
		//  The create has requested write access, mark to rescan the file.
		//  Allocate the context.
		//

		status = FltAllocateContext(ScannerData.Filter,
			FLT_STREAMHANDLE_CONTEXT,
			sizeof(SCANNER_STREAM_HANDLE_CONTEXT),
			PagedPool,
			&scannerContext);

		if (NT_SUCCESS(status)) {

			//
			//  Set the handle context.
			//

			scannerContext->RescanRequired = TRUE;

			(VOID)FltSetStreamHandleContext(FltObjects->Instance,
				FltObjects->FileObject,
				FLT_SET_CONTEXT_REPLACE_IF_EXISTS,
				scannerContext,
				NULL);

			//
			//  Normally we would check the results of FltSetStreamHandleContext
			//  for a variety of error cases. However, The only error status 
			//  that could be returned, in this case, would tell us that
			//  contexts are not supported.  Even if we got this error,
			//  we just want to release the context now and that will free
			//  this memory if it was not successfully set.
			//

			//
			//  Release our reference on the context (the set adds a reference)
			//

			FltReleaseContext(scannerContext);
		}
	}
	*/

	return returnStatus;
}


FLT_PREOP_CALLBACK_STATUS
FLTAPI ScannerPreClose(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID *CompletionContext
)

{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(CompletionContext);

	PAGED_CODE();

	// Sample
	//
	//  See if this create is being done by our user process.
	//

	//DbgPrintInformation(fltType_PreClose,Data,FltObjects);
	//DbgPrint( "!!! scanner.sys -- ScannerPreClose in ScannerPreClose !!!\n" );

	if (IoThreadToProcess(Data->Thread) == ScannerData.UserProcess) {

		DbgPrintInformation(fltType_PreClose, Data, FltObjects);
		//DbgPrint("!!! scanner.sys -- allowing create for trusted process \n");

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}


	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS
ScannerPostClose(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in_opt PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
)
{
	PSCANNER_STREAM_HANDLE_CONTEXT scannerContext;
	FLT_POSTOP_CALLBACK_STATUS returnStatus = FLT_POSTOP_FINISHED_PROCESSING;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	NTSTATUS status;
	BOOLEAN safeToOpen, scanFile;

	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	//
	//  If this create was failing anyway, don't bother scanning now.
	//

	if (!NT_SUCCESS(Data->IoStatus.Status) ||
		(STATUS_REPARSE == Data->IoStatus.Status)) {

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	DbgPrint("[Anti-Rs] ScannerPostClose\n");
	DbgPrintInformation(fltType_PostClose, Data, FltObjects);

	return returnStatus;
}


FLT_PREOP_CALLBACK_STATUS
ScannerPreCleanup(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID *CompletionContext
)
/*++

Routine Description:

Pre cleanup callback.  If this file was opened for write access, we want
to rescan it now.

Arguments:

Data - The structure which describes the operation parameters.

FltObject - The structure which describes the objects affected by this
operation.

CompletionContext - Output parameter which can be used to pass a context
from this pre-cleanup callback to the post-cleanup callback.

Return Value:

Always FLT_PREOP_SUCCESS_NO_CALLBACK.

--*/
{
	NTSTATUS status;
	PSCANNER_STREAM_HANDLE_CONTEXT context;
	BOOLEAN safe;

	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(CompletionContext);

	DbgPrintInformation(fltType_PreCleanup, Data, FltObjects);

	// Sample
	/*
	status = FltGetStreamHandleContext(FltObjects->Instance,
		FltObjects->FileObject,
		&context);

	if (NT_SUCCESS(status)) {

		if (context->RescanRequired) {

			(VOID)ScannerpScanFileInUserMode(FltObjects->Instance,
				FltObjects->FileObject,
				&safe);

			if (!safe) {

				DbgPrint("!!! scanner.sys -- foul language detected in precleanup !!!\n");
			}
		}

		FltReleaseContext(context);
	}
	*/

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS
ScannerPostCleanup(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in_opt PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
)
{
	PSCANNER_STREAM_HANDLE_CONTEXT scannerContext;
	FLT_POSTOP_CALLBACK_STATUS returnStatus = FLT_POSTOP_FINISHED_PROCESSING;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	NTSTATUS status;


	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	DbgPrint("[Anti-Rs] ScannerPostCleanup\n");
	DbgPrintInformation(fltType_PostCleanup, Data, FltObjects);

	//
	//  If this create was failing anyway, don't bother scanning now.
	//

	if (!NT_SUCCESS(Data->IoStatus.Status) ||
		(STATUS_REPARSE == Data->IoStatus.Status)) {

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	return returnStatus;
}


FLT_PREOP_CALLBACK_STATUS
ScannerPreWrite(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID *CompletionContext
)
/*++

Routine Description:

Pre write callback.  We want to scan what's being written now.

Arguments:

Data - The structure which describes the operation parameters.

FltObject - The structure which describes the objects affected by this
operation.

CompletionContext - Output parameter which can be used to pass a context
from this pre-write callback to the post-write callback.

Return Value:

Always FLT_PREOP_SUCCESS_NO_CALLBACK.

--*/
{
	FLT_PREOP_CALLBACK_STATUS returnStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
	NTSTATUS status;
	PSCANNER_NOTIFICATION notification = NULL;
	PSCANNER_STREAM_HANDLE_CONTEXT context = NULL;
	ULONG replyLength;
	BOOLEAN safe = TRUE;
	PUCHAR buffer;

	UNREFERENCED_PARAMETER(CompletionContext);

	//
	//  If not client port just ignore this write.
	//

	if (ScannerData.ClientPort == NULL) {

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	DbgPrintInformation(fltType_PreWrite, Data, FltObjects);

	// Sample
	/*
	status = FltGetStreamHandleContext(FltObjects->Instance,
		FltObjects->FileObject,
		&context);

	if (!NT_SUCCESS(status)) {

		//
		//  We are not interested in this file
		//

		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	}

	//
	//  Use try-finally to cleanup
	//

	try {

		//
		//  Pass the contents of the buffer to user mode.
		//

		if (Data->Iopb->Parameters.Write.Length != 0) {

			//
			//  Get the users buffer address.  If there is a MDL defined, use
			//  it.  If not use the given buffer address.
			//

			if (Data->Iopb->Parameters.Write.MdlAddress != NULL) {

				buffer = MmGetSystemAddressForMdlSafe(Data->Iopb->Parameters.Write.MdlAddress,
					NormalPagePriority);

				//
				//  If we have a MDL but could not get and address, we ran out
				//  of memory, report the correct error
				//

				if (buffer == NULL) {

					Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
					Data->IoStatus.Information = 0;
					returnStatus = FLT_PREOP_COMPLETE;
					leave;
				}

			}
			else {

				//
				//  Use the users buffer
				//

				buffer = Data->Iopb->Parameters.Write.WriteBuffer;
			}

			//
			//  In a production-level filter, we would actually let user mode scan the file directly.
			//  Allocating & freeing huge amounts of non-paged pool like this is not very good for system perf.
			//  This is just a sample!
			//

			notification = ExAllocatePoolWithTag(NonPagedPool,
				sizeof(SCANNER_NOTIFICATION),
				'nacS');
			if (notification == NULL) {

				Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				Data->IoStatus.Information = 0;
				returnStatus = FLT_PREOP_COMPLETE;
				leave;
			}

			notification->BytesToScan = min(Data->Iopb->Parameters.Write.Length, SCANNER_READ_BUFFER_SIZE);

			//
			//  The buffer can be a raw user buffer. Protect access to it
			//

			try {

				RtlCopyMemory(&notification->Contents,
					buffer,
					notification->BytesToScan);

			} except(EXCEPTION_EXECUTE_HANDLER) {

				//
				//  Error accessing buffer. Complete i/o with failure
				//

				Data->IoStatus.Status = GetExceptionCode();
				Data->IoStatus.Information = 0;
				returnStatus = FLT_PREOP_COMPLETE;
				leave;
			}

			//
			//  Send message to user mode to indicate it should scan the buffer.
			//  We don't have to synchronize between the send and close of the handle
			//  as FltSendMessage takes care of that.
			//

			replyLength = sizeof(SCANNER_REPLY);

			status = FltSendMessage(ScannerData.Filter,
				&ScannerData.ClientPort,
				notification,
				sizeof(SCANNER_NOTIFICATION),
				notification,
				&replyLength,
				NULL);

			if (STATUS_SUCCESS == status) {

				safe = ((PSCANNER_REPLY)notification)->SafeToOpen;

			}
			else {

				//
				//  Couldn't send message. This sample will let the i/o through.
				//

				DbgPrint("!!! scanner.sys --- couldn't send message to user-mode to scan file, status 0x%X\n", status);
			}
		}

		if (!safe) {

			//
			//  Block this write if not paging i/o (as a result of course, this scanner will not prevent memory mapped writes of contaminated
			//  strings to the file, but only regular writes). The effect of getting ERROR_ACCESS_DENIED for many apps to delete the file they
			//  are trying to write usually.
			//  To handle memory mapped writes - we should be scanning at close time (which is when we can really establish that the file object
			//  is not going to be used for any more writes)
			//

			DbgPrint("!!! scanner.sys -- foul language detected in write !!!\n");

			if (!FlagOn(Data->Iopb->IrpFlags, IRP_PAGING_IO)) {

				DbgPrint("!!! scanner.sys -- blocking the write !!!\n");

				Data->IoStatus.Status = STATUS_ACCESS_DENIED;
				Data->IoStatus.Information = 0;
				returnStatus = FLT_PREOP_COMPLETE;
			}
		}

	}
	finally {

		if (notification != NULL) {

			ExFreePoolWithTag(notification, 'nacS');
		}

		if (context) {

			FltReleaseContext(context);
		}
	}
	*/

	return returnStatus;
}


FLT_POSTOP_CALLBACK_STATUS
ScannerPostWrite(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in_opt PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
)
{
	PSCANNER_STREAM_HANDLE_CONTEXT scannerContext;
	FLT_POSTOP_CALLBACK_STATUS returnStatus = FLT_POSTOP_FINISHED_PROCESSING;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	NTSTATUS status;


	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	DbgPrint("PostWrite.....!!!!");
	DbgPrintInformation(fltType_PostWrite, Data, FltObjects);



	if (!NT_SUCCESS(Data->IoStatus.Status) ||
		(STATUS_REPARSE == Data->IoStatus.Status)) {

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	return returnStatus;
}


FLT_PREOP_CALLBACK_STATUS
ScannerPreSetInformation(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID *CompletionContext
)
/*++

Routine Description:

Pre callback for handling SetInformation.

Arguments:

Data - Pointer to the filter callbackData that is passed to us.

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

CompletionContext - The context for the completion routine for this
operation.

Return Value:

The return value is the status of the operation.

--*/
{
	FLT_PREOP_CALLBACK_STATUS returnStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
	NTSTATUS status = STATUS_SUCCESS;
	PVOID buffer = NULL;
	ULONG bufferLength = 0;
	FILE_INFORMATION_CLASS fileInfoClass;
	PFILE_RENAME_INFORMATION renameInfo = NULL;
	//PFILE_RENAME_INFORMATION newRenameInfo = NULL;
	PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
	UNICODE_STRING newFileName;

	struct {
		HANDLE RootDirectory;
		ULONG FileNameLength;
		PWSTR FileName;
	} setInfo;

	//PAGED_CODE();

	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(FltObjects);

	RtlInitUnicodeString(&newFileName, NULL);

	if (Data->Iopb->MajorFunction == IRP_MJ_CREATE)
	{
		status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
			&nameInfo);
		if (NT_SUCCESS(status))
		{
			status = FltParseFileNameInformation(nameInfo);

			if (!NT_SUCCESS(status))
			{

				goto PreSetInformationCleanup;
			}
			DbgPrint("!!! scanner.sys -- FltGetFileNameInformation !!!\n");

			DbgPrint("!!! nameInfo->Name.Buffer->  : %S\n", nameInfo->Name.Buffer);

			DbgPrint("!!! FltObjects->FileObject->FileName->  : %S\n", FltObjects->FileObject->FileName.Buffer);

			*CompletionContext = (PVOID)nameInfo;
		}
	}

	DbgPrintInformation(fltType_PreSetInformation, Data, FltObjects);

	// Sample
	/*
	struct {
		HANDLE RootDirectory;
		ULONG FileNameLength;
		PWSTR FileName;
	} setInfo;

	//PAGED_CODE();

	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(FltObjects);

	RtlInitUnicodeString(&newFileName, NULL);

	if (Data->Iopb->MajorFunction == IRP_MJ_CREATE)
	{
		status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
			&nameInfo);
		if (NT_SUCCESS(status))
		{
			status = FltParseFileNameInformation(nameInfo);

			if (!NT_SUCCESS(status))
			{

				goto PreSetInformationCleanup;
			}
			DbgPrint("!!! scanner.sys -- FltGetFileNameInformation !!!\n");

			DbgPrint("!!! nameInfo->Name.Buffer->  : %S\n", nameInfo->Name.Buffer);

			DbgPrint("!!! FltObjects->FileObject->FileName->  : %S\n", FltObjects->FileObject->FileName.Buffer);

			*CompletionContext = (PVOID)nameInfo;
		}
	}

	fileInfoClass = Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
	if (fileInfoClass == FileRenameInformation)
	{
		renameInfo = (PFILE_RENAME_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
		setInfo.RootDirectory = renameInfo->RootDirectory;
		setInfo.FileNameLength = renameInfo->FileNameLength;
		setInfo.FileName = renameInfo->FileName;

		status = FltGetDestinationFileNameInformation(FltObjects->Instance,
			FltObjects->FileObject,
			setInfo.RootDirectory,
			setInfo.FileName,
			setInfo.FileNameLength,
			FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER | FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT,
			&nameInfo);
		if (!NT_SUCCESS(status))
		{
			goto PreSetInformationCleanup;
		}

		status = FltParseFileNameInformation(nameInfo);

		if (!NT_SUCCESS(status))
		{

			goto PreSetInformationCleanup;
		}
	}

	if (fileInfoClass == FileRenameInformation)
	{
		DbgPrint("!!! scanner.sys -- FltGetDestinationFileNameInformation Rename !!!\n");

		bufferLength = sizeof(FILE_RENAME_INFORMATION) + (setInfo.FileNameLength) + sizeof(WCHAR);

		if (0 != wcsstr(nameInfo->Name.Buffer, L"MyTest"))
		{
			DbgPrint("!!! 변경된 파일명->  : %S\n", nameInfo->Name.Buffer);
			DbgPrint("!!! 오리지널 파일명->  : %S\n", FltObjects->FileObject->FileName.Buffer);
			DbgPrint("Access PID / TID : %d / %0xd \n", PsGetProcessId(IoThreadToProcess(Data->Thread)), Data->Thread);
		}
		//bufferLength = FIELD_OFFSET( FILE_RENAME_INFORMATION, FileName ) + newFileName.Length;


		if (nameInfo->Name.Buffer != FltObjects->FileObject->FileName.Buffer)
		{
			//if(0 != strstr(nameInfo->Name.Buffer, "MyTest") && 0 != strstr(FltObjects->FileObject->FileName.Buffer, "MyTest"))
			//{
			//	DbgPrint( "!!! scanner.sys -- File Rename happened !!!\n" );
			//	DbgPrintInformation(fltType_PreSetInformation,Data,FltObjects);
			//}
		}
	}

	Data->IoStatus.Status = status;

	returnStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;


PreSetInformationCleanup:

	if (nameInfo)
	{
		FltReleaseFileNameInformation(nameInfo);
	}

	//if (buffer) {

	//ExFreePoolWithTag( buffer, SCANNER_STRING_TAG );
	//}

	//newFileName.Length = newFileName.MaximumLength = 0;
	//newFileName.Buffer = NULL;
	//FltSetCallbackDataDirty( Data );

	if (!NT_SUCCESS(status))
	{

		Data->IoStatus.Status = status;

		returnStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}

	*/

	Data->IoStatus.Status = status;

	returnStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;

PreSetInformationCleanup:

	if (nameInfo)
	{
		FltReleaseFileNameInformation(nameInfo);
	}

	if (!NT_SUCCESS(status))
	{
		Data->IoStatus.Status = status;

		returnStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}

	return returnStatus;

}


FLT_POSTOP_CALLBACK_STATUS
ScannerPostSetInformation(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in_opt PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
)
{
	PSCANNER_STREAM_HANDLE_CONTEXT scannerContext;
	FLT_POSTOP_CALLBACK_STATUS returnStatus = FLT_POSTOP_FINISHED_PROCESSING;
	PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
	PFLT_FILE_NAME_INFORMATION realNameInfo = NULL;
	NTSTATUS status;
	BOOLEAN safeToOpen, scanFile;

	DbgPrintInformation(fltType_PostSetInformation, Data, FltObjects);

	// Sample
	/*
	nameInfo = (PFLT_FILE_NAME_INFORMATION)CompletionContext;

	//UNREFERENCED_PARAMETER( CompletionContext );
	UNREFERENCED_PARAMETER(Flags);
	//UNREFERENCED_PARAMETER( Data );
	UNREFERENCED_PARAMETER(FltObjects);

	if ((CompletionContext != NULL) && (Data->IoStatus.Status == STATUS_SUCCESS))
	{
		DbgPrintInformation(fltType_PostSetInformation, Data, FltObjects);
	}


	if (!NT_SUCCESS(Data->IoStatus.Status) ||
		(STATUS_REPARSE == Data->IoStatus.Status)) {

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	//(VOID) ScannerpScanFileInUserMode( FltObjects->Instance,
	//                                   FltObjects->FileObject,
	//                                   &safeToOpen );

	//if (!safeToOpen) {

	//    //
	//    //  Ask the filter manager to undo the create.
	//    //

	//    DbgPrint( "!!! scanner.sys -- foul language detected in postcreate !!!\n" );
	//    DbgPrint( "!!! scanner.sys -- undoing create \n" );

	//    FltCancelFileOpen( FltObjects->Instance, FltObjects->FileObject );

	//    Data->IoStatus.Status = STATUS_ACCESS_DENIED;
	//    Data->IoStatus.Information = 0;

	//    returnStatus = FLT_POSTOP_FINISHED_PROCESSING;

	//} else if (FltObjects->FileObject->WriteAccess) {

	//
	//
	//  The create has requested write access, mark to rescan the file.
	//  Allocate the context.
	//
	//DbgPrint( "FltObjects->FileObject->WriteAccess is %d!!!\n",FltObjects->FileObject->WriteAccess );
	status = FltAllocateContext(ScannerData.Filter,
		FLT_STREAMHANDLE_CONTEXT,
		sizeof(SCANNER_STREAM_HANDLE_CONTEXT),
		PagedPool,
		&scannerContext);

	if (NT_SUCCESS(status)) {

		//
		//  Set the handle context.
		//

		scannerContext->RescanRequired = TRUE;

		(VOID)FltSetStreamHandleContext(FltObjects->Instance,
			FltObjects->FileObject,
			FLT_SET_CONTEXT_REPLACE_IF_EXISTS,
			scannerContext,
			NULL);

		FltReleaseContext(scannerContext);
	}
	//}
	*/

	return returnStatus;
}


//////////////////////////////////////////////////////////////////////////
//  Local support routines.
//
/////////////////////////////////////////////////////////////////////////

NTSTATUS
ScannerpScanFileInUserMode(
	__in PFLT_INSTANCE Instance,
	__in PFILE_OBJECT FileObject,
	__out PBOOLEAN SafeToOpen
)
/*++

Routine Description:

This routine is called to send a request up to user mode to scan a given
file and tell our caller whether it's safe to open this file.

Note that if the scan fails, we set SafeToOpen to TRUE.  The scan may fail
because the service hasn't started, or perhaps because this create/cleanup
is for a directory, and there's no data to read & scan.

If we failed creates when the service isn't running, there'd be a
bootstrapping problem -- how would we ever load the .exe for the service?

Arguments:

Instance - Handle to the filter instance for the scanner on this volume.

FileObject - File to be scanned.

SafeToOpen - Set to FALSE if the file is scanned successfully and it contains
foul language.

Return Value:

The status of the operation, hopefully STATUS_SUCCESS.  The common failure
status will probably be STATUS_INSUFFICIENT_RESOURCES.

--*/

{
	NTSTATUS status = STATUS_SUCCESS;
	PVOID buffer = NULL;
	ULONG bytesRead;
	PSCANNER_NOTIFICATION notification = NULL;
	FLT_VOLUME_PROPERTIES volumeProps;
	LARGE_INTEGER offset;
	ULONG replyLength, length;
	PFLT_VOLUME volume = NULL;

	*SafeToOpen = TRUE;

	//
	//  If not client port just return.
	//

	if (ScannerData.ClientPort == NULL) {

		return STATUS_SUCCESS;
	}

	try {

		//
		//  Obtain the volume object .
		//

		status = FltGetVolumeFromInstance(Instance, &volume);

		if (!NT_SUCCESS(status)) {

			leave;
		}

		//
		//  Determine sector size. Noncached I/O can only be done at sector size offsets, and in lengths which are
		//  multiples of sector size. A more efficient way is to make this call once and remember the sector size in the
		//  instance setup routine and setup an instance context where we can cache it.
		//

		status = FltGetVolumeProperties(volume,
			&volumeProps,
			sizeof(volumeProps),
			&length);
		//
		//  STATUS_BUFFER_OVERFLOW can be returned - however we only need the properties, not the names
		//  hence we only check for error status.
		//

		if (NT_ERROR(status)) {

			leave;
		}

		length = max(SCANNER_READ_BUFFER_SIZE, volumeProps.SectorSize);

		//
		//  Use non-buffered i/o, so allocate aligned pool
		//

		buffer = FltAllocatePoolAlignedWithTag(Instance,
			NonPagedPool,
			length,
			'nacS');

		if (NULL == buffer) {

			status = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		notification = ExAllocatePoolWithTag(NonPagedPool,
			sizeof(SCANNER_NOTIFICATION),
			'nacS');

		if (NULL == notification) {

			status = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		//
		//  Read the beginning of the file and pass the contents to user mode.
		//

		offset.QuadPart = bytesRead = 0;
		status = FltReadFile(Instance,
			FileObject,
			&offset,
			length,
			buffer,
			FLTFL_IO_OPERATION_NON_CACHED |
			FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
			&bytesRead,
			NULL,
			NULL);

		if (NT_SUCCESS(status) && (0 != bytesRead)) {

			notification->BytesToScan = (ULONG)bytesRead;

			//
			//  Copy only as much as the buffer can hold
			//

			RtlCopyMemory(&notification->Contents,
				buffer,
				min(notification->BytesToScan, SCANNER_READ_BUFFER_SIZE));

			replyLength = sizeof(SCANNER_REPLY);

			status = FltSendMessage(ScannerData.Filter,
				&ScannerData.ClientPort,
				notification,
				sizeof(SCANNER_NOTIFICATION),
				notification,
				&replyLength,
				NULL);

			if (STATUS_SUCCESS == status) {

				*SafeToOpen = ((PSCANNER_REPLY)notification)->SafeToOpen;

			}
			else {

				//
				//  Couldn't send message
				//

				DbgPrint("!!! scanner.sys --- couldn't send message to user-mode to scan file, status 0x%X\n", status);
			}
		}

	}
	finally {

		if (NULL != buffer) {

			FltFreePoolAlignedWithTag(Instance, buffer, 'nacS');
		}

		if (NULL != notification) {

			ExFreePoolWithTag(notification, 'nacS');
		}

		if (NULL != volume) {

			FltObjectDereference(volume);
		}
	}

	return status;
}

