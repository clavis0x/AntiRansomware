#pragma once
#include "afxwin.h"


// CAntiRansomwareSettingsDlg 대화 상자입니다.

class CAntiRansomwareSettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(CAntiRansomwareSettingsDlg)

public:
	CAntiRansomwareSettingsDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CAntiRansomwareSettingsDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ANTIRANSOMWARESETTINGSDLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CButton ctr_checkEnableBehaviorDetection;
	afx_msg void OnClose();
	CEdit ctr_editProtectionExt;
	CComboBox ctr_comboBackupDays;
	CButton ctr_checkEnableAutoUpdate;
	CButton ctr_checkEnableSignatureDetection;
	CEdit ctr_editUpdateTime;
	CComboBox ctr_comboRansomHandling;
};
