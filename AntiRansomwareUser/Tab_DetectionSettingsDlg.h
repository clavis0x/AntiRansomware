#pragma once
#include "afxwin.h"


// CTab_DetectionSettingsDlg 대화 상자입니다.

class CTab_DetectionSettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(CTab_DetectionSettingsDlg)

public:
	CTab_DetectionSettingsDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CTab_DetectionSettingsDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TAB_DETECTIONSETTINGSDLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	CButton ctr_checkEnableBehaviorDetection;
	CEdit ctr_editProtectionExt;
	CComboBox ctr_comboRansomHandling;
	CButton ctr_checkEnableSignatureDetection;
	CButton ctr_checkEnableAutoUpdate;
	CEdit ctr_editUpdateTime;
	virtual BOOL OnInitDialog();
};
