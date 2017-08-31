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
	DDX_Control(pDX, IDC_CHECK_ENABLE_BEHAVIOR_DETECTION, ctr_checkEnableBehaviorDetection);
	DDX_Control(pDX, IDC_EDIT_PROTECTION_EXT, ctr_editProtectionExt);
	DDX_Control(pDX, IDC_COMBO_BACKUP_DAYS, ctr_comboBackupDays);
	DDX_Control(pDX, IDC_CHECK_ENABLE_AUTOUPDATE, ctr_checkEnableAutoUpdate);
	DDX_Control(pDX, IDC_CHECK_ENABLE_SIGNATURE_DETECTION, ctr_checkEnableSignatureDetection);
	DDX_Control(pDX, IDC_EDIT_UPDATE_TIME, ctr_editUpdateTime);
	DDX_Control(pDX, IDC_COMBO_RANSOM_HANDLING, ctr_comboRansomHandling);
}


BEGIN_MESSAGE_MAP(CAntiRansomwareSettingsDlg, CDialog)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CAntiRansomwareSettingsDlg 메시지 처리기입니다.


BOOL CAntiRansomwareSettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	// 행위기반 탐지
	ctr_checkEnableBehaviorDetection.SetCheck(1);

	ctr_editProtectionExt.SetWindowTextA("txt,hwp,doc,docx,ppt,pptx,xls,xlsx,c,cpp,h,hpp,bmp,jpg,gif,png,zip,rar");

	ctr_comboBackupDays.AddString("1");
	ctr_comboBackupDays.AddString("3");
	ctr_comboBackupDays.AddString("7");
	ctr_comboBackupDays.AddString("14");
	ctr_comboBackupDays.AddString("30");
	ctr_comboBackupDays.AddString("180");
	ctr_comboBackupDays.AddString("365");
	ctr_comboBackupDays.SetCurSel(2);

	ctr_comboRansomHandling.AddString("삭제");
	ctr_comboRansomHandling.AddString("물어보고 처리");
	ctr_comboRansomHandling.SetCurSel(0);

	// 시그니처 탐지
	ctr_checkEnableSignatureDetection.SetCheck(1);
	ctr_checkEnableAutoUpdate.SetCheck(1);
	ctr_editUpdateTime.SetWindowTextA("3");

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void CAntiRansomwareSettingsDlg::OnClose()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	DestroyWindow();

	CDialog::OnClose();
}
