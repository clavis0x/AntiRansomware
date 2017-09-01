// AntiRansomwareSettingsDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "AntiRansomwareUser.h"
#include "AntiRansomwareSettingsDlg.h"
#include "afxdialogex.h"


// CAntiRansomwareSettingsDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CAntiRansomwareSettingsDlg, CDialog)

CAntiRansomwareSettingsDlg::CAntiRansomwareSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_ANTIRANSOMWARESETTINGSDLG, pParent)
{

}

CAntiRansomwareSettingsDlg::~CAntiRansomwareSettingsDlg()
{
}

void CAntiRansomwareSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_SETTINGS, ctr_tabSettings);
}


BEGIN_MESSAGE_MAP(CAntiRansomwareSettingsDlg, CDialog)
	ON_WM_CLOSE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_SETTINGS, &CAntiRansomwareSettingsDlg::OnTcnSelchangeTabSettings)
	ON_NOTIFY(TCN_SELCHANGING, IDC_TAB_SETTINGS, &CAntiRansomwareSettingsDlg::OnTcnSelchangingTabSettings)
	ON_BN_CLICKED(IDC_BUTTON_CONFIRM, &CAntiRansomwareSettingsDlg::OnBnClickedButtonConfirm)
	ON_BN_CLICKED(IDC_BUTTON_CANCEL, &CAntiRansomwareSettingsDlg::OnBnClickedButtonCancel)
END_MESSAGE_MAP()


// CAntiRansomwareSettingsDlg 메시지 처리기입니다.


BOOL CAntiRansomwareSettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	CString strTab = _T("");

	strTab.Format(_T("탐지 설정"));
	ctr_tabSettings.InsertItem(0, strTab, 0);

	strTab.Format(_T("백업 설정"));
	ctr_tabSettings.InsertItem(1, strTab, 0);

	strTab.Format(_T("기타 설정"));
	ctr_tabSettings.InsertItem(2, strTab, 0);

	CRect Rect;
	ctr_tabSettings.GetClientRect(&Rect);

	m_TabDetectionSettings.Create(IDD_TAB_DETECTIONSETTINGSDLG, &ctr_tabSettings);
	m_TabDetectionSettings.SetWindowPos(NULL, 2, 23, Rect.Width() - 6, Rect.Height() - 26, SWP_SHOWWINDOW | SWP_NOZORDER);
	m_pwndShow = &m_TabDetectionSettings;

	m_TabBackupSettings.Create(IDD_TAB_BACKUPSETTINGSDLG, &ctr_tabSettings);
	m_TabBackupSettings.SetWindowPos(NULL, 2, 23, Rect.Width() - 6, Rect.Height() - 26, SWP_NOZORDER);
	m_pwndShow = &m_TabBackupSettings;

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void CAntiRansomwareSettingsDlg::OnClose()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	DestroyWindow();

	CDialog::OnClose();
}


void CAntiRansomwareSettingsDlg::OnTcnSelchangeTabSettings(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	int nIndex = ctr_tabSettings.GetCurSel();

	m_pwndShow = NULL;
	m_TabDetectionSettings.ShowWindow(SW_HIDE);
	m_TabBackupSettings.ShowWindow(SW_HIDE);

	switch (nIndex)
	{
		case 0:
			m_TabDetectionSettings.ShowWindow(SW_SHOW);
			break;
		case 1:
			m_TabBackupSettings.ShowWindow(SW_SHOW);
			break;
	}

	*pResult = 0;
}


void CAntiRansomwareSettingsDlg::OnTcnSelchangingTabSettings(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_prevCurSel = ctr_tabSettings.GetCurSel();
	*pResult = 0;
}


void CAntiRansomwareSettingsDlg::OnBnClickedButtonConfirm()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	EndDialog(IDOK);
	DestroyWindow();
}


void CAntiRansomwareSettingsDlg::OnBnClickedButtonCancel()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	EndDialog(IDCANCEL);
	DestroyWindow();
}
