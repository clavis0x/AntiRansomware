#pragma once
#include "afxcmn.h"


// CAntiRansomwareReportDlg 대화 상자입니다.

class CAntiRansomwareReportDlg : public CDialog
{
	DECLARE_DYNAMIC(CAntiRansomwareReportDlg)

public:
	CAntiRansomwareReportDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CAntiRansomwareReportDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ANTIRANSOMWAREREPORTDLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// Anti-Ransomware Report
public:
	int m_numLog;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnLvnGetdispinfoListLog(NMHDR *pNMHDR, LRESULT *pResult);
	void UpdateLogList();
	CListCtrl ctr_listLog;
};
