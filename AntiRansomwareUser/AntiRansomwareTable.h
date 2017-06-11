
// AntiRansomware - Process Behavior Tables
//

#pragma once

typedef struct sProcessEvent {
	char mode;
	unsigned int numEvent;
} PROCESS_EVENT;

typedef struct sProcessBehavior {
	DWORD pid;
	DWORD ppid;
	CString strName;
	unsigned int cntCreate;
	unsigned int cntWrite;
	unsigned int cntWrite_sp;
	unsigned int cntRename;
	unsigned int cntDelete;
	stack<PROCESS_EVENT> stackEventRecord;
}  PROCESS_BEHAVIOR;

typedef struct sItemNewFile {
	unsigned int num;
	bool isDirectory;
	CString strPath;
} ITEM_NEW_FILE;

typedef struct sItemWriteFile {
	DWORD pid;
	unsigned int num;
	CString strPath;
	unsigned int num_back;
} ITEM_WRITE_FILE;

typedef struct sItemRenameFile {
	unsigned int num;
	CString strSrc;
	CString strDst;
} ITEM_RENAME_FILE;

typedef struct sItemDeleteFile {
	unsigned int num;
	bool isDirectory;
	CString strPath;
	unsigned int num_back;
} ITEM_DELETE_FILE;

typedef struct sItemBackupFile {
	unsigned int num;
	CString strPath;
} ITEM_BACKUP_FILE;