// AntiRansomwarePopupDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "AntiRansomwareUser.h"
#include "AntiRansomwarePopupDlg.h"
#include "afxdialogex.h"
#include "AntiRansomwareUserDlg.h"

extern CAntiRansomwareUserDlg *g_pParent;


// CAntiRansomwarePopupDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CAntiRansomwarePopupDlg, CDialog)

CAntiRansomwarePopupDlg::CAntiRansomwarePopupDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_ANTIRANSOMWAREPOPUPDLG, pParent)
{

}

CAntiRansomwarePopupDlg::~CAntiRansomwarePopupDlg()
{
}

void CAntiRansomwarePopupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SYSLINK_PROCPATH, ctr_linkProcPath);
}


BEGIN_MESSAGE_MAP(CAntiRansomwarePopupDlg, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_CLOSE()
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_YES, &CAntiRansomwarePopupDlg::OnBnClickedButtonYes)
	ON_BN_CLICKED(IDC_BUTTON_NO, &CAntiRansomwarePopupDlg::OnBnClickedButtonNo)
END_MESSAGE_MAP()


// CAntiRansomwarePopupDlg 메시지 처리기입니다.


BOOL CAntiRansomwarePopupDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.  
	CRect rect;
	CRect cRect;

	GetClientRect(cRect);
	int m_Desktowidth = GetSystemMetrics(SM_CXSCREEN);
	int m_DesktopHeight = GetSystemMetrics(SM_CYSCREEN);

	HWND hTrayWnd = ::FindWindow("Shell_TrayWnd", NULL);
	if (hTrayWnd != NULL) {
		::GetWindowRect(hTrayWnd, &rect);
		if (rect.top > cRect.Height())
			SetWindowPos(NULL, m_Desktowidth - cRect.Width() - 10, rect.top - cRect.Height() - 10, 0, 0, SWP_NOSIZE);
		else
			SetWindowPos(NULL, m_Desktowidth - cRect.Width() - 10, m_DesktopHeight - cRect.Height() - 10, 0, 0, SWP_NOSIZE);
	}

	m_fontTitle.CreateFont(22, 0, 0, 0, FW_HEAVY, FALSE, FALSE, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, "맑은 고딕");

	m_fontMessage.CreateFont(20, 0, 0, 0, FW_HEAVY, FALSE, FALSE, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, "맑은 고딕");

	GetDlgItem(IDC_STATIC_TITLE)->SetFont(&m_fontTitle);
	GetDlgItem(IDC_STATIC_MESSAGE)->SetFont(&m_fontMessage);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


BOOL CAntiRansomwarePopupDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (!m_background.m_hObject)
		return true;

	CRect rect;
	GetClientRect(&rect);

	CDC dc;
	dc.CreateCompatibleDC(pDC);
	CBitmap* pOldBitmap = dc.SelectObject(&m_background);

	BITMAP bmap;
	m_background.GetBitmap(&bmap);
	pDC->BitBlt(0, 0, rect.Width(), rect.Height(), &dc, 0, 0, SRCCOPY);

	dc.SelectObject(pOldBitmap);

	//return CDialog::OnEraseBkgnd(pDC);
	return true;
}


HBRUSH CAntiRansomwarePopupDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  여기서 DC의 특성을 변경합니다.

	// TODO:  기본값이 적당하지 않으면 다른 브러시를 반환합니다.
	switch (nCtlColor)
	{
	case CTLCOLOR_STATIC:
	{
		if (pWnd->GetDlgCtrlID() == IDC_STATIC_TITLE)
		{
			pDC->SetTextColor(RGB(255, 255, 255));
			pDC->SetBkMode(TRANSPARENT);
			return (HBRUSH)GetStockObject(NULL_BRUSH);;
		}
		else {
			pDC->SetBkMode(TRANSPARENT);
			return (HBRUSH)GetStockObject(NULL_BRUSH);;
		}
	}
	}
	return hbr;
}


void CAntiRansomwarePopupDlg::OnClose()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	DestroyWindow();

	CDialog::OnClose();
}

void CAntiRansomwarePopupDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	m_fontTitle.DeleteObject();
	m_fontMessage.DeleteObject();
}



void CAntiRansomwarePopupDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (point.y < 37)
		SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, 0);

	CDialog::OnLButtonDown(nFlags, point);
}


void CAntiRansomwarePopupDlg::InitPopupWindow(ITEM_POPUP_MESSAGE &sItem)
{
	CRect rect;
	CString strTemp;
	int nTypeButton = 0;
	m_typePopup = sItem.typePopup;
	m_pPopupMessage = &sItem;
	GetDlgItem(IDC_STATIC_MESSAGE2)->GetWindowRect(rect);
	ScreenToClient(rect);

	m_background.DeleteObject();

	switch (m_typePopup)
	{
		case 1:
			this->SetWindowTextA(sItem.strTitle);
			GetDlgItem(IDC_STATIC_TITLE)->SetWindowTextA(sItem.strTitle);
			GetDlgItem(IDC_STATIC_MESSAGE)->SetWindowTextA(sItem.strMessage1);
			GetDlgItem(IDC_STATIC_MESSAGE2)->SetWindowTextA(sItem.strMessage2);
			strTemp.Format("[%d] %s", sItem.pid, sItem.strProcName);
			GetDlgItem(IDC_STATIC_PROCNAME)->SetWindowTextA(strTemp);
			strTemp.Format("(<a>%s</a>)", sItem.strProcPath);
			GetDlgItem(IDC_SYSLINK_PROCPATH)->SetWindowTextA(strTemp);
			m_background.LoadBitmapA(IDB_BITMAP_POPUP_RED);
			nTypeButton = 0;
			break;
		case 2:
			this->SetWindowTextA(sItem.strTitle);
			GetDlgItem(IDC_STATIC_TITLE)->SetWindowTextA(sItem.strTitle);
			GetDlgItem(IDC_STATIC_MESSAGE)->SetWindowTextA(sItem.strMessage1);
			GetDlgItem(IDC_STATIC_MESSAGE2)->SetWindowTextA(sItem.strMessage2);
			strTemp.Format("%s", sItem.strProcName);
			GetDlgItem(IDC_STATIC_PROCNAME)->SetWindowTextA(strTemp);
			strTemp.Format("(<a>%s</a>)", sItem.strProcPath);
			GetDlgItem(IDC_SYSLINK_PROCPATH)->SetWindowTextA(strTemp);
			m_background.LoadBitmapA(IDB_BITMAP_POPUP_RED);
			nTypeButton = 1;
			break;
		case 3:
			this->SetWindowTextA(sItem.strTitle);
			GetDlgItem(IDC_STATIC_TITLE)->SetWindowTextA(sItem.strTitle);
			GetDlgItem(IDC_STATIC_MESSAGE)->SetWindowTextA(sItem.strMessage1);
			GetDlgItem(IDC_STATIC_MESSAGE2)->SetWindowTextA(sItem.strMessage2);
			GetDlgItem(IDC_STATIC_MESSAGE2)->MoveWindow(rect.left, 85, rect.Width(), rect.Height() + (rect.top - 85));
			GetDlgItem(IDC_STATIC_PROCINFO)->ShowWindow(false);
			GetDlgItem(IDC_STATIC_PROCNAME)->ShowWindow(false);
			GetDlgItem(IDC_SYSLINK_PROCPATH)->ShowWindow(false);
			m_background.LoadBitmapA(IDB_BITMAP_POPUP_BLUE);
			nTypeButton = 0;
			break;
	}

	if (nTypeButton == 1) {
		GetDlgItem(IDC_BUTTON_YES)->GetWindowRect(rect);
		ScreenToClient(rect);
		GetDlgItem(IDC_BUTTON_YES)->MoveWindow(rect.left + 62, rect.top, rect.Width(), rect.Height());
		GetDlgItem(IDC_BUTTON_YES)->SetWindowTextA("확인");
		GetDlgItem(IDC_BUTTON_NO)->ShowWindow(false);
	}


	Invalidate(TRUE);
}

void CAntiRansomwarePopupDlg::OnBnClickedButtonYes()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	// 파일 전송 테스트(임시)
	if(m_typePopup == 1){
		m_pPopupMessage->typePopup = 3;
		m_pPopupMessage->strTitle = "랜섬웨어 보고";
		m_pPopupMessage->strMessage1 = "랜섬웨어 의심파일 보고";
		m_pPopupMessage->strMessage2.Format("랜섬웨어로 의심되는 파일을 보다 정확하게 분석하기 위해\n가상 환경 자동 분석 서버로 전송하시겠습니까?\n\nFile: %s\\%s",
											m_pPopupMessage->strProcPath, m_pPopupMessage->strProcName);

		ShowWindow(SW_HIDE);
		Sleep(100);
		InitPopupWindow(*m_pPopupMessage);
		ShowWindow(SW_SHOW);
	}
	else if (m_typePopup == 3) {
		CString strTemp;
		strTemp.Format("%s\\%s", m_pPopupMessage->strProcPath, m_pPopupMessage->strProcName);
		g_pParent->DoSendRansomwareFile(strTemp);
		EndDialog(IDOK);
		DestroyWindow();
	}
	else {
		EndDialog(IDOK);
		DestroyWindow();
	}
}


void CAntiRansomwarePopupDlg::OnBnClickedButtonNo()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	EndDialog(IDCANCEL);
	DestroyWindow();
}
