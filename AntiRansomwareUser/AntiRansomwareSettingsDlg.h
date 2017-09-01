#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "Tab_DetectionSettingsDlg.h"
#include "Tab_BackupSettingsDlg.h"


// CAntiRansomwareSettingsDlg 대화 상자입니다.

class CAntiRansomwareSettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(CAntiRansomwareSettingsDlg)

public:
	CAntiRansomwareSettingsDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CAntiRansomwareSettingsDlg();

private:
	CWnd* m_pwndShow;
	int m_prevCurSel;

	CTab_DetectionSettingsDlg m_TabDetectionSettings;
	CTab_BackupSettingsDlg m_TabBackupSettings;

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ANTIRANSOMWARESETTINGSDLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	CTabCtrl ctr_tabSettings;
	afx_msg void OnTcnSelchangeTabSettings(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTcnSelchangingTabSettings(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonConfirm();
	afx_msg void OnBnClickedButtonCancel();
};
