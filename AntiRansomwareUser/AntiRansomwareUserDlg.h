
// AntiRansomwareUserDlg.h : 헤더 파일
//

#pragma once
#include "afxwin.h"
#include "AntiRansomwarePopupDlg.h"
#include "AntiRansomwareReportDlg.h"
#include "AntiRansomwareSettingsDlg.h"
#include "AntiRansomwareRecord.h"
#include "scanuser.h"

#include "GdipButton.h"

const UINT WM_INITIALIZATION_COMPLETED = ::RegisterWindowMessage("WM_INITIALIZATION_COMPLETED");
const UINT WM_POPUP_INFO_WINDOW = ::RegisterWindowMessage("WM_POPUP_INFO_WINDOW");

typedef struct sScanLog {
	CString timeStamp;
	CString content;
}  SCAN_LOG;


// CAntiRansomwareUserDlg 대화 상자
class CAntiRansomwareUserDlg : public CDialogEx
{
// 생성입니다.
public:
	CAntiRansomwareUserDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ANTIRANSOMWAREUSER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

// MyScanner
private:
	CWinThread*	pThreadCommunication;
	list<CString> m_listFileExt; // 파일 확장자
	CRITICAL_SECTION m_csFileExt;

public:
	int m_isRunning;

	HANDLE m_hEventCheckRansomware;
	CRITICAL_SECTION m_csScanLog;
	CRITICAL_SECTION m_csFileQueue;

	vector<SCAN_LOG> m_listScanLog; // packet list
	list<ITEM_CHECK_FILE> m_listCheckFile;

	map<unsigned int, ArProcessBehavior*> m_mapProcessBehavior;

	int m_nCountMonitor;

	// 백업 중 경로
	CString m_strBackingUpPath;

	ITEM_POPUP_MESSAGE m_sPopupMessage;

// 구현입니다.
protected:
	HICON m_hIcon;
	CBitmap m_background;

	CGdipButton	m_gbtnClose;
	CGdipButton	m_gbtnMinimum;
	CGdipButton	m_gbtnMenu2;
	CGdipButton	m_gbtnMenu3;
	CGdipButton	m_gbtnMenu4;
	CGdipButton	m_gbtnMenu5;
	CGdipButton	m_gbtnToggle1; // 행위기반
	CGdipButton	m_gbtnToggle2; // 시그니처
	CGdipButton	m_gbtnMenuB1;

	CBitmap lamp_image;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CAntiRansomwarePopupDlg m_pAntiRansomwarePopupDlg;
	CAntiRansomwareReportDlg m_pAntiRansomwareReportDlg;
	CAntiRansomwareSettingsDlg m_pAntiRansomwareSettingsDlg;
	LRESULT OnInitializationCompleted(WPARAM wParam, LPARAM lParam);
	LRESULT OnPopupInfoWindow(WPARAM wParam, LPARAM lParam);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonViewreport();
	afx_msg void OnBnClickedButtonRecovery();
	void AddLogList(CString msg, bool wTime = false);
	bool InitDialogUI();
	bool InitMyScanner();
	void SetFileExtList();
	bool AddCheckFileExtension(CString strExt);
	bool DoCheckFileExtension(CString strPath);
	static UINT CommunicationMyScanner(LPVOID lpParam);
	int RecordProcessBehavior(PSCANNER_NOTIFICATION notification);
	int GetPermissionDirectory(CString strPath, DWORD pid = 0);
	int DoCheckRansomware(CString strPath);
	bool DoPopupInfoWindow(int type);
	bool DoKillRecoveryRansomware(DWORD pid);
	bool AddCheckRansomwareFile(DWORD pid, CString strPath);
	CString GetBackupFilePath(CString strPath);
	CEdit ctr_editTargetPid;
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnBnClickedButtonClose();
	CStatic ctr_picMenu1;
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	bool SetDetectionEngine();
	afx_msg void OnBnClickedButtonToggle1();
	afx_msg void OnBnClickedButtonToggle2();
	afx_msg void OnBnClickedButtonMenu2();
	afx_msg void OnBnClickedButtonMenu3();
	afx_msg void OnBnClickedButtonMenu4();
	afx_msg void OnBnClickedButtonMenu5();
	afx_msg void OnBnClickedButtonMinimum();
};
