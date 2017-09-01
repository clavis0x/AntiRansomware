// Tab_BackupSettingsDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "AntiRansomwareUser.h"
#include "Tab_BackupSettingsDlg.h"
#include "afxdialogex.h"


// CTab_BackupSettingsDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CTab_BackupSettingsDlg, CDialog)

CTab_BackupSettingsDlg::CTab_BackupSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_TAB_BACKUPSETTINGSDLG, pParent)
{

}

CTab_BackupSettingsDlg::~CTab_BackupSettingsDlg()
{
}

void CTab_BackupSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTab_BackupSettingsDlg, CDialog)
END_MESSAGE_MAP()


// CTab_BackupSettingsDlg 메시지 처리기입니다.
