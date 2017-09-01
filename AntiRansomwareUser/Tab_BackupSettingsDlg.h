#pragma once


// CTab_BackupSettingsDlg 대화 상자입니다.

class CTab_BackupSettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(CTab_BackupSettingsDlg)

public:
	CTab_BackupSettingsDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CTab_BackupSettingsDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TAB_BACKUPSETTINGSDLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
};
