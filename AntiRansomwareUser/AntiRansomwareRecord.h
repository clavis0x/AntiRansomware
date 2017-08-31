
// AntiRansomware - Process Behavior Header
//

#pragma once
#include "scanuser.h"
#include "AntiRansomwareTable.h"
#include "UtilityFilesys.h"

// Get Process Info
enum {
	PB_PROC_PID = 1,
	PB_PROC_PPID
};

// Get Count
enum {
	PB_COUNT_CREATE = 1,
	PB_COUNT_WRITE,
	PB_COUNT_WRITE_SP,
	PB_COUNT_RENAME,
	PB_COUNT_DELETE
};

// 프로세스 이벤트 단위
typedef struct sProcessEvent {
	char mode;				// 0: new, 1: rename, 2: write, 3: delete, 4~: registry
	unsigned int numEvent;	// Event Number
} PROCESS_EVENT;

class ArProcessBehavior // 프로세스 행위 기록
{
private:
	DWORD m_pid;
	DWORD m_ppid;
	CString m_strProcName;

	int m_nTimeCount;

	unsigned int m_cntCreate;
	unsigned int m_cntWrite;
	unsigned int m_cntWrite_sp;
	unsigned int m_cntRename;
	unsigned int m_cntDelete;

	int m_numNewFile;
	int m_numWriteFile;
	int m_numRenameFile;
	int m_numDeleteFile;
	int m_numBackupFile;

	stack<PROCESS_EVENT> m_stackEventRecord;

	list<ITEM_NEW_FILE> m_listNewFile;
	list<ITEM_WRITE_FILE> m_listWriteFile;
	list<ITEM_RENAME_FILE> m_listRenameFile;
	list<ITEM_DELETE_FILE> m_listDeleteFile;
	list<ITEM_BACKUP_FILE> m_listBackupFile;

	PATH_INFO_EX m_pathInfoEx;
public:
	bool m_isRansomware;

public:
	ArProcessBehavior(DWORD pid);
	int CheckProcessInfo();
	int RecordProcessBehavior(PSCANNER_NOTIFICATION notification);
	bool RecoveryProcessBehavior();
	bool AddEventNewFile(bool isDirectory, CString strPath);
	bool AddEventRenameFile(CString strSrc, CString strDst);
	bool AddEventWriteFile(CString strPath, bool isBackup);
	bool AddEventDeleteFile(CString strPath, bool isBackup);
	unsigned int AddEventBackupFile(CString strPath);
	bool DoRecoveryFile(unsigned int num_back, CString strPath);
	bool DoBackupFile(CString strPath, int type = 0);
	void AddLogList(CString msg, bool wTime = false);
	DWORD GetProcessInfo(int type);
	unsigned int GetCountBehavior(int type);
	bool CheckRenameFilePath(md5_byte_t *md5);
	bool CheckWriteFilePath(md5_byte_t *md5);
	~ArProcessBehavior();
};