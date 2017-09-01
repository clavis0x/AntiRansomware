// Tab_DetectionSettingsDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "AntiRansomwareUser.h"
#include "Tab_DetectionSettingsDlg.h"
#include "afxdialogex.h"


// CTab_DetectionSettingsDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CTab_DetectionSettingsDlg, CDialog)

CTab_DetectionSettingsDlg::CTab_DetectionSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_TAB_DETECTIONSETTINGSDLG, pParent)
{

}

CTab_DetectionSettingsDlg::~CTab_DetectionSettingsDlg()
{
}

void CTab_DetectionSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_ENABLE_BEHAVIOR_DETECTION, ctr_checkEnableBehaviorDetection);
	DDX_Control(pDX, IDC_EDIT_PROTECTION_EXT, ctr_editProtectionExt);
	DDX_Control(pDX, IDC_COMBO_RANSOM_HANDLING, ctr_comboRansomHandling);
	DDX_Control(pDX, IDC_CHECK_ENABLE_SIGNATURE_DETECTION, ctr_checkEnableSignatureDetection);
	DDX_Control(pDX, IDC_CHECK_ENABLE_AUTOUPDATE, ctr_checkEnableAutoUpdate);
	DDX_Control(pDX, IDC_EDIT_UPDATE_TIME, ctr_editUpdateTime);
}


BEGIN_MESSAGE_MAP(CTab_DetectionSettingsDlg, CDialog)
END_MESSAGE_MAP()


// CTab_DetectionSettingsDlg 메시지 처리기입니다.


BOOL CTab_DetectionSettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	// 행위기반 탐지
	ctr_checkEnableBehaviorDetection.SetCheck(1);

	ctr_editProtectionExt.SetWindowTextA("txt,hwp,doc,docx,ppt,pptx,xls,xlsx,c,cpp,h,hpp,bmp,jpg,gif,png,zip,rar");

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
