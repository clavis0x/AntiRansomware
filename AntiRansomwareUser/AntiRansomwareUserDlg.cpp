
// AntiRansomwareUserDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "AntiRansomwareUser.h"
#include "AntiRansomwareUserDlg.h"
#include "afxdialogex.h"

#include "MemDC.h"

// 가상 분석 & 업데이트 서버
#define SERVER_IP "192.168.0.4"
#define SERVER_PORT "12345"

const char* g_szBackupPath = "\\_SafeBackup"; // 백업 폴더
const char* g_szBackupExt = "exe,txt,hwp,doc,docx,ppt,pptx,xls,xlsx,c,cpp,h,hpp,bmp,jpg,gif,png,zip,rar"; // 보호 파일 확장자

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

SYSTEMTIME g_time; // 시간 구조체.

HWND hWnd;
CAntiRansomwareUserDlg *g_pParent;

CWinThread*	pThreadCheckRansomware = NULL;
static UINT CheckRansomwareWorker(LPVOID lpParam); // 파일 암호화 감시 스레드

//타이머 핸들러 선언
MMRESULT g_idMNTimer1;
void CALLBACK OnTimerFunc(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2);

UCHAR FoulString[] = "foul";

DWORD FindRansomwareParantPID(DWORD pid);
BOOL DoKillProcessTree(DWORD pid);

// CAntiRansomwareUserDlg 대화 상자



CAntiRansomwareUserDlg::CAntiRansomwareUserDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_ANTIRANSOMWAREUSER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAntiRansomwareUserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_TARGET_PID, ctr_editTargetPid);
	DDX_Control(pDX, IDC_BUTTON_CLOSE, m_gbtnClose);
	DDX_Control(pDX, IDC_BUTTON_MINIMUM, m_gbtnMinimum);
	DDX_Control(pDX, IDC_PICTURE_MENU1, ctr_picMenu1);
	DDX_Control(pDX, IDC_BUTTON_MENU2, m_gbtnMenu2);
	DDX_Control(pDX, IDC_BUTTON_MENU3, m_gbtnMenu3);
	DDX_Control(pDX, IDC_BUTTON_MENU4, m_gbtnMenu4);
	DDX_Control(pDX, IDC_BUTTON_MENU5, m_gbtnMenu5);
	DDX_Control(pDX, IDC_BUTTON_TOGGLE1, m_gbtnToggle1);
	DDX_Control(pDX, IDC_BUTTON_TOGGLE2, m_gbtnToggle2);
	DDX_Control(pDX, IDC_BUTTON_ViewReport, m_gbtnMenuB1);
	
}

BEGIN_MESSAGE_MAP(CAntiRansomwareUserDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_REGISTERED_MESSAGE(WM_INITIALIZATION_COMPLETED, OnInitializationCompleted) // WM_INITIALIZATION_COMPLETED
	ON_REGISTERED_MESSAGE(WM_POPUP_INFO_WINDOW, OnPopupInfoWindow) // WM_POPUP_INFO_WINDOW
	ON_BN_CLICKED(IDC_BUTTON_ViewReport, &CAntiRansomwareUserDlg::OnBnClickedButtonViewreport)
	ON_WM_MOVING()
	ON_BN_CLICKED(IDC_BUTTON_RECOVERY, &CAntiRansomwareUserDlg::OnBnClickedButtonRecovery)
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CAntiRansomwareUserDlg::OnBnClickedButtonClose)
	ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(IDC_BUTTON_TOGGLE1, &CAntiRansomwareUserDlg::OnBnClickedButtonToggle1)
	ON_BN_CLICKED(IDC_BUTTON_TOGGLE2, &CAntiRansomwareUserDlg::OnBnClickedButtonToggle2)
	ON_BN_CLICKED(IDC_BUTTON_MENU2, &CAntiRansomwareUserDlg::OnBnClickedButtonMenu2)
	ON_BN_CLICKED(IDC_BUTTON_MENU3, &CAntiRansomwareUserDlg::OnBnClickedButtonMenu3)
	ON_BN_CLICKED(IDC_BUTTON_MENU4, &CAntiRansomwareUserDlg::OnBnClickedButtonMenu4)
	ON_BN_CLICKED(IDC_BUTTON_MENU5, &CAntiRansomwareUserDlg::OnBnClickedButtonMenu5)
	ON_BN_CLICKED(IDC_BUTTON_MINIMUM, &CAntiRansomwareUserDlg::OnBnClickedButtonMinimum)
END_MESSAGE_MAP()


// CAntiRansomwareUserDlg 메시지 처리기

BOOL CAntiRansomwareUserDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	hWnd = AfxGetMainWnd()->m_hWnd; // GET MAIN HANDLE
	g_pParent = this; // 부모 개체 정의

	TCHAR szPath[_MAX_PATH];
	GetModuleFileName(AfxGetInstanceHandle(), szPath, _MAX_PATH); // 현재 실행파일 경로 구하기
	appPath = szPath;
	int nPos = appPath.ReverseFind('\\'); // 실행파일 경로에서 파일명 제외
	if (nPos > 0)
		appPath = appPath.Left(nPos);

	// Init UI
	InitDialogUI();

	m_listScanLog.clear();

	// CRITICAL SECTION - Initial
	InitializeCriticalSection(&m_csFileExt);
	InitializeCriticalSection(&m_csScanLog);
	InitializeCriticalSection(&m_csFileQueue);

	m_strBackingUpPath.Empty();

	InitMyScanner();

	m_nCountMonitor = 0;

	// Popup Message
	m_sPopupMessage.pid = 0;
	m_sPopupMessage.strProcName = "";
	m_sPopupMessage.strProcPath = "";

	/********** 멀티미디어 타이머 시작 **********/
	//System에서 가능한 Resolution을 구한다.
	//멀티미디어 타이머의 Resolution을 최소로 하기 위한 코드    
	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(TIMECAPS));
	unsigned int Resolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
	timeBeginPeriod(Resolution);

	//멀티미디어 타이머 생성
	g_idMNTimer1 = timeSetEvent(
		1000,         //Timer Delay(ms)
		Resolution,  //delay의 정확도를 의미
		OnTimerFunc,  //callback Function
		(DWORD)this,   //CallBack에 사용할 전달 인자
		TIME_PERIODIC);  //타이머의 타입(발생시마다 콜백 Call)

						 /********** 멀티미디어 타이머 끝 **********/

	PostMessageA(WM_INITIALIZATION_COMPLETED, NULL, NULL); // WM_INITIALIZATION_COMPLETED

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

bool CAntiRansomwareUserDlg::InitDialogUI()
{
	this->SetWindowTextA("Anti-Ransomware");

	m_background.LoadBitmapA(IDB_BITMAP_MAIN);

	m_gbtnClose.LoadStdImage(IDB_PNG_CLOSE_NORMAL, _T("PNG"));
	m_gbtnClose.LoadSthImage(IDB_PNG_CLOSE_ACTIVE, _T("PNG"));
	m_gbtnMinimum.LoadStdImage(IDB_PNG_MINIMUM_NORMAL, _T("PNG"));
	m_gbtnMinimum.LoadSthImage(IDB_PNG_MINIMUM_ACTIVE, _T("PNG"));

	// MENU1
	lamp_image.LoadBitmap(IDB_BITMAP_MENU1_SAFETY);
	HBITMAP h_old_bitmap = ctr_picMenu1.SetBitmap(lamp_image);
	if (h_old_bitmap != NULL) ::DeleteObject(h_old_bitmap);
	lamp_image.Detach();

	m_gbtnMenu2.LoadStdImage(IDB_PNG_MENU2_NORMAL, _T("PNG"));
	m_gbtnMenu2.LoadSthImage(IDB_PNG_MENU2_ACTIVE, _T("PNG"));
	m_gbtnMenu3.LoadStdImage(IDB_PNG_MENU3_NORMAL, _T("PNG"));
	m_gbtnMenu3.LoadSthImage(IDB_PNG_MENU3_ACTIVE, _T("PNG"));
	m_gbtnMenu4.LoadStdImage(IDB_PNG_MENU4_NORMAL, _T("PNG"));
	m_gbtnMenu4.LoadSthImage(IDB_PNG_MENU4_ACTIVE, _T("PNG"));
	m_gbtnMenu5.LoadStdImage(IDB_PNG_MENU5_NORMAL, _T("PNG"));
	m_gbtnMenu5.LoadSthImage(IDB_PNG_MENU5_ACTIVE, _T("PNG"));

	m_gbtnToggle1.LoadStdImage(IDB_PNG_TOGGLE_OFF_NORMAL, _T("PNG"));
	m_gbtnToggle1.LoadSthImage(IDB_PNG_TOGGLE_OFF_ACTIVE, _T("PNG"));
	m_gbtnToggle1.LoadAltImage(IDB_PNG_TOGGLE_ON_NORMAL, _T("PNG"));
	m_gbtnToggle1.LoadAlhImage(IDB_PNG_TOGGLE_ON_ACTIVE, _T("PNG"));
	m_gbtnToggle1.EnableToggle(TRUE);
	m_gbtnToggle1.SetToggle(TRUE);

	m_gbtnToggle2.LoadStdImage(IDB_PNG_TOGGLE_OFF_NORMAL, _T("PNG"));
	m_gbtnToggle2.LoadSthImage(IDB_PNG_TOGGLE_OFF_ACTIVE, _T("PNG"));
	m_gbtnToggle2.LoadAltImage(IDB_PNG_TOGGLE_ON_NORMAL, _T("PNG"));
	m_gbtnToggle2.LoadAlhImage(IDB_PNG_TOGGLE_ON_ACTIVE, _T("PNG"));
	m_gbtnToggle2.EnableToggle(TRUE);
	m_gbtnToggle2.SetToggle(TRUE);

	m_gbtnMenuB1.LoadStdImage(IDB_PNG_MENU_B1_NORMAL, _T("PNG"));
	m_gbtnMenuB1.LoadSthImage(IDB_PNG_MENU_B1_ACTIVE, _T("PNG"));

	return true;
}


LRESULT CAntiRansomwareUserDlg::OnInitializationCompleted(WPARAM wParam, LPARAM lParam) // WM_INITIALIZATION_COMPLETED
{
	OnBnClickedButtonViewreport();
	return S_OK;
}

LRESULT CAntiRansomwareUserDlg::OnPopupInfoWindow(WPARAM wParam, LPARAM lParam) // WM_POPUP_INFO_WINDOW
{
	ArProcessBehavior* itemArProcessBehavior;

	if (wParam == 0) { // 행위기반 탐지
		itemArProcessBehavior = m_mapProcessBehavior[lParam];

		m_sPopupMessage.typePopup = 1;
		m_sPopupMessage.strTitle = "랜섬웨어 탐지!";
		m_sPopupMessage.strMessage1 = "랜섬웨어 의심 행위가 탐지되었습니다. (Behavior)";
		m_sPopupMessage.strMessage2 = "해당 실행 파일을 삭제하시겠습니까 ?";
		m_sPopupMessage.pid = lParam;
		m_sPopupMessage.strProcName = itemArProcessBehavior->GetProcessName(PB_PROC_NAME);
		m_sPopupMessage.strProcPath = itemArProcessBehavior->GetProcessName(PB_PROC_PATH);
	}
	else if (wParam == 1) { // 시그니처 탐지
		m_sPopupMessage.typePopup = 2;
		m_sPopupMessage.strTitle = "랜섬웨어 탐지!";
		m_sPopupMessage.strMessage1 = "랜섬웨어가 탐지되었습니다. (Signature)";
		m_sPopupMessage.strMessage2 = "랜섬웨어가 탐지되어 해당 실행 파일을 삭제하였습니다.";
		m_sPopupMessage.pid = -1;
		m_sPopupMessage.strProcName = PathFindFileName(m_strDetectedPath.GetBuffer());
		m_sPopupMessage.strProcPath = m_strDetectedPath;
	}

	g_pParent->DoPopupInfoWindow(); // 팝업창
	return S_OK;
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CAntiRansomwareUserDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}


BOOL CAntiRansomwareUserDlg::OnEraseBkgnd(CDC* pDC)
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
	//pDC->StretchBlt(0, 0, rect.Width(), rect.Height(), &dc, 0, 0, bmap.bmWidth, bmap.bmHeight, SRCCOPY);
	pDC->BitBlt(0, 0, rect.Width(), rect.Height(), &dc, 0, 0, SRCCOPY);

	dc.SelectObject(pOldBitmap);

	//return CDialogEx::OnEraseBkgnd(pDC);
	return true;
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CAntiRansomwareUserDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CAntiRansomwareUserDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	// 제목표시줄 클릭 시 창 이동
	if(point.y < 37)
		SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, 0);

	CDialogEx::OnLButtonDown(nFlags, point);
}


void CAntiRansomwareUserDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
	CDialogEx::OnMoving(fwSide, pRect);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_pAntiRansomwareReportDlg.GetSafeHwnd() != NULL) {
		m_pAntiRansomwareReportDlg.SetWindowPos(NULL, pRect->right, pRect->top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}
}


void CAntiRansomwareUserDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	m_isRunning = 0;

	// CRITICAL SECTION - Delete
	DeleteCriticalSection(&m_csFileExt);
	DeleteCriticalSection(&m_csScanLog);
	DeleteCriticalSection(&m_csFileQueue);
}


void CAntiRansomwareUserDlg::OnBnClickedButtonViewreport()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (m_pAntiRansomwareReportDlg.GetSafeHwnd() == NULL) {
		m_pAntiRansomwareReportDlg.Create(IDD_ANTIRANSOMWAREREPORTDLG);
		//m_pAntiRansomwareReportDlg.CenterWindow(CWnd::FromHandle(this->m_hWnd));
	}
	m_pAntiRansomwareReportDlg.ShowWindow(SW_SHOW);
}


void CALLBACK OnTimerFunc(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	DWORD pid;
	int nResult;
	CString strTemp;

	if (wTimerID == g_idMNTimer1) {
		map<unsigned int, ArProcessBehavior*>::iterator itor;
		ArProcessBehavior* itemArProcessBehavior;

		// 프로세스 기록 조회
		for (itor = g_pParent->m_mapProcessBehavior.begin(); itor != g_pParent->m_mapProcessBehavior.end(); ++itor) {
			nResult = itor->second->CheckProcessInfo();
			if(nResult == 1){
				strTemp.Format("랜섬웨어 탐지! - pid: %d", itor->first);
				g_pParent->AddLogList(strTemp);
				pid = FindRansomwareParantPID(itor->second->GetProcessInfo(PB_PROC_PID));
				DoKillProcessTree(pid); // 프로세스 트리 종료
				g_pParent->DoKillRecoveryRansomware(pid); // 파일 복구

				PostMessageA(g_pParent->m_hWnd, WM_POPUP_INFO_WINDOW, 0, pid); // 팝업창
				RefreshDesktopDirectory();
			}
		}
	}
}

void CAntiRansomwareUserDlg::AddLogList(CString msg, bool wTime)
{
	int listNum;
	CString strTime;

	::ZeroMemory(reinterpret_cast<void*>(&g_time), sizeof(g_time)); // time 초기화.
	::GetLocalTime(&g_time);    // 현재시간을 얻음.

	SCAN_LOG tmpScanLog;

	strTime.Format("%02d:%02d:%02d", g_time.wHour, g_time.wMinute, g_time.wSecond);
	tmpScanLog.timeStamp.SetString(strTime);
	tmpScanLog.content.SetString(msg);

	EnterCriticalSection(&m_csScanLog);
	m_listScanLog.push_back(tmpScanLog);
	LeaveCriticalSection(&m_csScanLog);
}


bool CAntiRansomwareUserDlg::InitMyScanner()
{
	m_isRunning = 1;
	AddLogList("InitMyScanner()", true);

	// 검사 대상 파일 확장자 등록
	SetFileExtList();

	// 미니필터 통신 스레드 생성
	pThreadCommunication = AfxBeginThread(CommunicationMyScanner, this, THREAD_PRIORITY_NORMAL, 0, 0);
	if (pThreadCommunication == NULL) {
		m_isRunning = 0;
		AddLogList("[Error] Fail to create 'Communication' thread.", true);
		return false;
	}

	// 파일 암호화 감시 이벤트
	m_hEventCheckRansomware = CreateEvent(NULL, TRUE, FALSE, NULL);

	// 파일 암호화 감시 스레드 생성
	pThreadCheckRansomware = AfxBeginThread(CheckRansomwareWorker, this, THREAD_PRIORITY_NORMAL, 0, 0);
	if (pThreadCheckRansomware == NULL) {
		m_isRunning = 0;
		AddLogList("[Error] Fail to create 'CheckRansomware' thread.", true);
		return 0;
	}

	return false;
}


bool CAntiRansomwareUserDlg::AddCheckFileExtension(CString strExt)
{
	bool result;
	list<CString>::iterator itor;

	EnterCriticalSection(&m_csFileExt);

	// 중복 검사
	itor = m_listFileExt.begin();
	while (itor != m_listFileExt.end()) {
		if (strExt.Compare(*itor) == 0) {
			LeaveCriticalSection(&m_csFileExt);
			return false;
		}
		itor++;
	}

	// 확장자 추가
	m_listFileExt.push_back(strExt);

	LeaveCriticalSection(&m_csFileExt);

	return true;
}


bool CAntiRansomwareUserDlg::DoCheckFileExtension(CString strPath)
{
	bool result;
	CString strExt;
	list<CString>::iterator itor;

	// 확장자 분리
	strExt = PathFindExtension(strPath);
	strExt = strExt.Mid(1);

	EnterCriticalSection(&m_csFileExt);

	// 중복 검사
	itor = m_listFileExt.begin();
	while (itor != m_listFileExt.end()) {
		if (strExt.Compare(*itor) == 0) {
			LeaveCriticalSection(&m_csFileExt);
			return true;
		}
		itor++;
	}

	LeaveCriticalSection(&m_csFileExt);

	return false;
}


void CAntiRansomwareUserDlg::SetFileExtList()
{
	char *token;
	char *szBackupExt;
	unsigned int nSize = strlen(g_szBackupExt);

	szBackupExt = new char[nSize + 1];
	ZeroMemory(szBackupExt, nSize + 1);
	memcpy(szBackupExt, g_szBackupExt, nSize);

	token = strtok((char*)szBackupExt, ",");
	while (token) {
		AddCheckFileExtension((CString)token); // Add
		token = strtok(NULL, ",");
	}

	delete szBackupExt;
}


BOOL
ScanBuffer(
	__in_bcount(BufferSize) PUCHAR Buffer,
	__in ULONG BufferSize
)
/*++

Routine Description

Scans the supplied buffer for an instance of FoulString.

Note: Pattern matching algorithm used here is just for illustration purposes,
there are many better algorithms available for real world filters

Arguments

Buffer      -   Pointer to buffer
BufferSize  -   Size of passed in buffer

Return Value

TRUE        -    Found an occurrence of the appropriate FoulString
FALSE       -    Buffer is ok

--*/
{
	PUCHAR p;
	ULONG searchStringLength = sizeof(FoulString) - sizeof(UCHAR);

	for (p = Buffer;
		p <= (Buffer + BufferSize - searchStringLength);
		p++) {

		if (RtlEqualMemory(p, FoulString, searchStringLength)) {

			printf("Found a string\n");

			//
			//  Once we find our search string, we're not interested in seeing
			//  whether it appears again.
			//

			return TRUE;
		}
	}

	return FALSE;
}


DWORD
ScannerWorker(
	__in PSCANNER_THREAD_CONTEXT Context
)
/*++

Routine Description

This is a worker thread that


Arguments

Context  - This thread context has a pointer to the port handle we use to send/receive messages,
and a completion port handle that was already associated with the comm. port by the caller

Return Value

HRESULT indicating the status of thread exit.

--*/
{
	PSCANNER_NOTIFICATION notification;
	SCANNER_REPLY_MESSAGE replyMessage;
	PSCANNER_MESSAGE message;
	LPOVERLAPPED pOvlp;
	BOOL result;
	DWORD outSize;
	HRESULT hr;
	ULONG_PTR key;

	int nResult = 0;
	CString strMsg;

#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant

	while (TRUE) {

#pragma warning(pop)

		//
		//  Poll for messages from the filter component to scan.
		//

		result = GetQueuedCompletionStatus(Context->Completion, &outSize, &key, &pOvlp, INFINITE);

		//
		//  Obtain the message: note that the message we sent down via FltGetMessage() may NOT be
		//  the one dequeued off the completion queue: this is solely because there are multiple
		//  threads per single port handle. Any of the FilterGetMessage() issued messages can be
		//  completed in random order - and we will just dequeue a random one.
		//

		message = CONTAINING_RECORD(pOvlp, SCANNER_MESSAGE, Ovlp);

		if (!result) {

			//
			//  An error occured.
			//

			hr = HRESULT_FROM_WIN32(GetLastError());
			break;
		}

		//strMsg.Format("Received message, size %d", pOvlp->InternalHigh);
		//g_pParent->AddLogList(strMsg, 1, true);
		//printf("Received message, size %d\n", pOvlp->InternalHigh);

		notification = &message->Notification;

		if (notification->Reserved == 0)
		{
			assert(notification->BytesToScan <= SCANNER_READ_BUFFER_SIZE);
			__analysis_assume(notification->BytesToScan <= SCANNER_READ_BUFFER_SIZE);

			result = ScanBuffer(notification->Contents, notification->BytesToScan);

			replyMessage.ReplyHeader.Status = 0;
			replyMessage.ReplyHeader.MessageId = message->MessageHeader.MessageId;

			//
			//  Need to invert the boolean -- result is true if found
			//  foul language, in which case SafeToOpen should be set to false.
			//

			replyMessage.Reply.SafeToOpen = !result;

			//strMsg.Format("Replying message, SafeToOpen: %d", replyMessage.Reply.SafeToOpen);
			//g_pParent->AddLogList(strMsg, 1, true);
			//printf("Replying message, SafeToOpen: %d\n", replyMessage.Reply.SafeToOpen);

		}
		else
		{
			BOOL bRet;
			int i;
			char *pFilePath;
			wchar_t szDrivePath[1024] = { 0 };

			replyMessage.ReplyHeader.Status = 0;
			replyMessage.ReplyHeader.MessageId = message->MessageHeader.MessageId;
			replyMessage.Reply.SafeToOpen = TRUE;

			USES_CONVERSION;

			switch (notification->Reserved)
			{
				case fltType_PreCreate:
					strMsg.Format("===== [%d] fltType_PreCreate =====", notification->ulPID);
					//g_pParent->AddLogList(strMsg, true);
					nResult = g_pParent->RecordProcessBehavior(notification); // 프로세스 행위 기록
					break;
				case fltType_PostCreate:
					strMsg.Format("===== [%d] fltType_PostCreate =====", notification->ulPID);
					//g_pParent->AddLogList(strMsg, true);
					nResult = g_pParent->RecordProcessBehavior(notification); // 프로세스 행위 기록
					break;
				case fltType_PreClose:
					strMsg.Format("===== [%d] fltType_PreClose =====", notification->ulPID);
					g_pParent->AddLogList(strMsg, true);
					break;
				case fltType_PostClose:
					strMsg.Format("===== [%d] fltType_PostClose =====", notification->ulPID);
					g_pParent->AddLogList(strMsg, true);
					break;
				case fltType_PreCleanup:
					strMsg.Format("===== [%d] fltType_PreCleanup =====", notification->ulPID);
					//g_pParent->AddLogList(strMsg, true);
					nResult = g_pParent->RecordProcessBehavior(notification); // 프로세스 행위 기록
					break;
				case fltType_PostCleanup:
					strMsg.Format("===== [%d] fltType_PostCleanup =====", notification->ulPID);
					g_pParent->AddLogList(strMsg, true);
					break;
				case fltType_PreWrite:
					strMsg.Format("===== [%d] fltType_PreWrite =====", notification->ulPID);
					//g_pParent->AddLogList(strMsg, true);
					//nResult = g_pParent->RecordProcessBehavior(notification); // 프로세스 행위 기록
					break;
				case fltType_PostWrite:
					strMsg.Format("===== [%d] fltType_PostWrite =====", notification->ulPID);
					g_pParent->AddLogList(strMsg, true);
					//nResult = g_pParent->RecordProcessBehavior(notification); // 프로세스 행위 기록
					break;
				case fltType_PreSetInformation:
					strMsg.Format("===== [%d] fltType_PreSetInformation =====", notification->ulPID);
					//g_pParent->AddLogList(strMsg, true);
					nResult = g_pParent->RecordProcessBehavior(notification); // 프로세스 행위 기록
					break;
				case fltType_PostSetInformation:
					strMsg.Format("===== [%d] fltType_PostSetInformation =====", notification->ulPID);
					//g_pParent->AddLogList(strMsg, true);
					nResult = g_pParent->RecordProcessBehavior(notification); // 프로세스 행위 기록
					break;
			}
		}
		if (nResult == 100) { // Backup
			CString strBackupPath;
			CString strNewDir;
			int nPos;
			wchar_t szFilePath[MAX_FILE_PATH] = { 0 };
			wchar_t szBackupPath[MAX_FILE_PATH] = { 0 };
			PATH_INFO_EX pathInfoEx;

			if (ConvertDevicePathToDrivePath((wchar_t*)notification->Contents, szFilePath, MAX_PATH, &pathInfoEx) == FALSE)
				break;

			// 백업 경로 구하기
			strBackupPath = g_pParent->GetBackupFilePath((CString)szFilePath);

			// 백업 파일 존재 확인
			if (PathFileExists(strBackupPath) == FALSE) {
				g_pParent->m_strBackingUpPath.SetString(strBackupPath); // 백업 중인 파일 등록

				// 백업 하위 디렉토리 생성
				strNewDir.SetString(strBackupPath);
				PathRemoveFileSpec((LPSTR)(LPCTSTR)strNewDir);
				CreateFolder(strNewDir); // 백업 디렉토리 생성

				nPos = strBackupPath.Find('\\');
				if (nPos >= 0) {
					strBackupPath.Format("%S%s", pathInfoEx.szDeviceName, strBackupPath.Mid(nPos));
					//g_pParent->AddLogList(strBackupPath);
				}
				memcpy(replyMessage.Reply.Contents, (const char *)strBackupPath, MAX_FILE_PATH);
			}
			else {
				// 백업 파일 존재
				nResult = 0;
			}
		}
		replyMessage.Reply.cmdType = nResult; // Command

		hr = FilterReplyMessage(Context->Port,
			(PFILTER_REPLY_HEADER)&replyMessage,
			sizeof(replyMessage));

		if (SUCCEEDED(hr)) {

			//strMsg.Format("Replied message");
			//g_pParent->AddLogList(strMsg, 1, true);
			printf("Replied message\n");

		}
		else {

			strMsg.Format("Scanner: Error replying message. Error = 0x%X", hr);
			g_pParent->AddLogList(strMsg, true);
			printf("Scanner: Error replying message. Error = 0x%X\n", hr);
			break;
		}

		memset(&message->Ovlp, 0, sizeof(OVERLAPPED));

		hr = FilterGetMessage(Context->Port,
			&message->MessageHeader,
			FIELD_OFFSET(SCANNER_MESSAGE, Ovlp),
			&message->Ovlp);

		if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {

			break;
		}

		if (!g_pParent->m_isRunning)
			break;
	}

	if (!SUCCEEDED(hr)) {

		if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)) {

			//
			//  Scanner port disconncted.
			//

			strMsg.Format("Scanner: Port is disconnected, probably due to scanner filter unloading.");
			g_pParent->AddLogList(strMsg, true);
			printf("Scanner: Port is disconnected, probably due to scanner filter unloading.\n");

		}
		else {

			strMsg.Format("Scanner: Unknown error occured. Error = 0x%X", hr);
			g_pParent->AddLogList(strMsg, true);
			printf("Scanner: Unknown error occured. Error = 0x%X\n", hr);
		}
	}

	free(message);

	return hr;
}


UINT CAntiRansomwareUserDlg::CommunicationMyScanner(LPVOID lpParam)
{
	CAntiRansomwareUserDlg* pDlg = (CAntiRansomwareUserDlg*)lpParam;

	DWORD requestCount = SCANNER_DEFAULT_REQUEST_COUNT;
	DWORD threadCount = SCANNER_DEFAULT_THREAD_COUNT;
	HANDLE threads[SCANNER_MAX_THREAD_COUNT];
	SCANNER_THREAD_CONTEXT context;
	HANDLE port, completion;
	PSCANNER_MESSAGE msg;
	DWORD threadId;
	HRESULT hr;
	DWORD i, j;

	CString strMsg;

	pDlg->AddLogList("CommunicationMyScanner()", true);

	pDlg->AddLogList("Scanner: Connecting to the filter ...", true);
	printf("Scanner: Connecting to the filter ...\n");

	hr = FilterConnectCommunicationPort(ScannerPortName,
		0,
		NULL,
		0,
		NULL,
		&port);

	if (IS_ERROR(hr)) {
		strMsg.Format("ERROR: Connecting to filter port: 0x%08x", hr);
		pDlg->AddLogList(strMsg, true);
		printf("ERROR: Connecting to filter port: 0x%08x\n", hr);
		return 2;
	}

	//
	//  Create a completion port to associate with this handle.
	//

	completion = CreateIoCompletionPort(port,
		NULL,
		0,
		threadCount);

	if (completion == NULL) {

		printf("ERROR: Creating completion port: %d\n", GetLastError());
		CloseHandle(port);
		return 3;
	}

	strMsg.Format("Scanner: Port = 0x%p Completion = 0x%p", port, completion);
	pDlg->AddLogList(strMsg, true);
	printf("Scanner: Port = 0x%p Completion = 0x%p\n", port, completion);


	USER_NOTIFICATION usr_noti;
	DWORD dwProcId;
	char szRecvTest[100];
	DWORD rtnSz;

	GetWindowThreadProcessId(hWnd, &dwProcId);
	usr_noti.user_pid = dwProcId;

	hr = FilterSendMessage(port,
		&usr_noti,
		sizeof(USER_NOTIFICATION),
		szRecvTest,
		sizeof(szRecvTest),
		&rtnSz);

	context.Port = port;
	context.Completion = completion;

	//
	//  Create specified number of threads.
	//

	for (i = 0; i < threadCount; i++) {

		threads[i] = CreateThread(NULL,
			0,
			(LPTHREAD_START_ROUTINE)ScannerWorker,
			&context,
			0,
			&threadId);

		if (threads[i] == NULL) {

			//
			//  Couldn't create thread.
			//

			hr = GetLastError();
			printf("ERROR: Couldn't create thread: %d\n", hr);
			goto main_cleanup;
		}

		for (j = 0; j < requestCount; j++) {

			//
			//  Allocate the message.
			//

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "msg will not be leaked because it is freed in ScannerWorker")
			msg = (PSCANNER_MESSAGE)malloc(sizeof(SCANNER_MESSAGE));

			if (msg == NULL) {

				hr = ERROR_NOT_ENOUGH_MEMORY;
				goto main_cleanup;
			}

			memset(&msg->Ovlp, 0, sizeof(OVERLAPPED));

			//
			//  Request messages from the filter driver.
			//

			hr = FilterGetMessage(port,
				&msg->MessageHeader,
				FIELD_OFFSET(SCANNER_MESSAGE, Ovlp),
				&msg->Ovlp);

			if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {

				free(msg);
				goto main_cleanup;
			}
		}
	}

	hr = S_OK;

	WaitForMultipleObjectsEx(i, threads, TRUE, INFINITE, FALSE);

main_cleanup:

	printf("Scanner:  All done. Result = 0x%08x\n", hr);

	CloseHandle(port);
	CloseHandle(completion);

	pDlg->pThreadCommunication = NULL;
	return 0;
}


inline void WaitG(double dwMillisecond)
{
	MSG msg;
	LARGE_INTEGER   st, ed, freq;
	double			WaitFrequency;
	QueryPerformanceFrequency(&freq);
	WaitFrequency = ((double)dwMillisecond / 1000) * ((double)freq.QuadPart);

	if (freq.QuadPart == 0)
	{
		//::SetDlgItemText(hWnd,IDC_EDIT_Status,"Warning! - 고해상도 타이머 지원 안함.");
		//AddListLog(1, "지원 안함.");
		return;
	}

	QueryPerformanceCounter(&st);
	QueryPerformanceCounter(&ed);
	while ((double)(ed.QuadPart - st.QuadPart) < WaitFrequency)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		Sleep(1);

		QueryPerformanceCounter(&ed);
	}
}


BOOL DoKillProcess(DWORD pid)
{
	BOOL bResult;
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	bResult = TerminateProcess(hProcess, 0);

	return bResult;
}

BOOL DoKillProcessTree(DWORD pid)
{
	CString strTemp;
	HANDLE handle = NULL;
	PROCESSENTRY32 pe = { 0 };
	pe.dwSize = sizeof(PROCESSENTRY32);
	handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	strTemp.Format("■Kill: %d", pid);
	g_pParent->AddLogList(strTemp);
	if (pid == 0){
		g_pParent->AddLogList(" ㄴ pass");
		return TRUE;
	}

	DoKillProcess(pid);

	if (Process32First(handle, &pe))
	{
		do
		{
			if (pe.th32ParentProcessID == pid) {
				if(pe.th32ProcessID != pid)
					DoKillProcessTree(pe.th32ProcessID);
			}
		} while (Process32Next(handle, &pe));
	}

	return TRUE;
}


bool GetProcessName(DWORD pid, DWORD *ppid, char *szName)
{
	HANDLE handle = NULL;
	PROCESSENTRY32 pe = { 0 };
	pe.dwSize = sizeof(PROCESSENTRY32);
	handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (Process32First(handle, &pe))
	{
		do
		{
			if (pe.th32ProcessID == pid) {
				strncpy(szName, pe.szExeFile, MAX_PATH);
				*ppid = pe.th32ParentProcessID;
				return true;
			}
		} while (Process32Next(handle, &pe));
	}

	return false;
}


DWORD FindRansomwareParantPID(DWORD pid)
{
	bool result;
	bool isFound = false;
	DWORD prev_pid = pid;
	DWORD ppid = pid;
	char szProcessName[MAX_PATH];
	CString strTemp;

	while (ppid != 0) {
		result = GetProcessName(pid, &ppid, szProcessName);
		if (strcmp(szProcessName, "explorer.exe") == 0) {
			isFound = true;
			break;
		}
		else {
			prev_pid = pid;
			pid = ppid;
		}
		if (result == false){
			isFound = true;
			break;
		}
	}
	if (isFound == false) {
		return false;
	}

	return prev_pid;
}


int CAntiRansomwareUserDlg::GetPermissionDirectory(CString strPath, DWORD pid)
{
	int nPos;
	int nResult = 0;
	CString strSafePath;
	DWORD dwProcId;
	CString strTemp;

	// 백업 디렉토리 설정
	nPos = strPath.Find('\\');
	if (nPos == -1)
		return 0;

	strSafePath.Format("%s%s", strPath.Left(nPos), g_szBackupPath);

	// My pid 
	GetWindowThreadProcessId(hWnd, &dwProcId);

	// 백업 중인 파일 여부
	if (!g_pParent->m_strBackingUpPath.IsEmpty()) {
		if (strPath.Compare(g_pParent->m_strBackingUpPath) == 0)
			return 0;
	}

	// Safe Backup Directory 검사
	if (strPath.Find(strSafePath, 0) >= 0) {
		nResult = 0;
		if (pid != dwProcId) {
			//strTemp.Format("!! 권한 없음(Safe) !! : %s", strPath);
			//g_pParent->AddLogList(strTemp);
			nResult = 3; // 권한 없음
		}
		else {
			nResult = 2; // 접근 가능(me): Safe Backup Directory
		}
	}
	else {
		for(int i=0; i<5; i++){
			nResult = 0;
			EnterCriticalSection(&m_csFileQueue);
			// 보호 중인 파일 검사
			list<ITEM_CHECK_FILE>::iterator itor = m_listCheckFile.begin();
			while (itor != m_listCheckFile.end()) {
				if (strPath.Compare((CString)itor->strPath) == 0) {
					if (pid != dwProcId) {
						nResult = 3; // 권한 없음
						break;
					}
					else {
						nResult = 1; // 접근 가능(me): Scheduled File
						break;
					}
				}
				itor++;
			}
			LeaveCriticalSection(&m_csFileQueue);
			if (nResult == 3) {
				// 대기 
				//strTemp.Format("!! 권한 없음(Scheduled) - Wait...(%d) !! : %s", i, strPath);
				//g_pParent->AddLogList(strTemp);
				Sleep(10); // Wait
			}
			else
				break;
		}
		if (nResult == 3) {
			//strTemp.Format("!! 권한 없음(Scheduled) !! : %s", strPath);
			//g_pParent->AddLogList(strTemp);
		}
	}

	return nResult;
}

// 프로세스 행위 관리
int CAntiRansomwareUserDlg::RecordProcessBehavior(PSCANNER_NOTIFICATION notification)
{
	ArProcessBehavior* itemArProcessBehavior;
	int nReturn = 0;

	if (m_mapProcessBehavior.find(notification->ulPID) == m_mapProcessBehavior.end())
	{
		// 신규 프로세스 등록
		ArProcessBehavior* newArProcessBehavior = new ArProcessBehavior(notification->ulPID);
		m_mapProcessBehavior[notification->ulPID] = newArProcessBehavior;

		itemArProcessBehavior = newArProcessBehavior;
	}
	else {
		itemArProcessBehavior = m_mapProcessBehavior[notification->ulPID];
	}

	// 프로세스 행위 기록
	if (!itemArProcessBehavior->m_isRansomware)
		nReturn = itemArProcessBehavior->RecordProcessBehavior(notification);
	else
		nReturn = 1;

	return nReturn;
}

CString CAntiRansomwareUserDlg::GetBackupFilePath(CString strPath)
{
	int nPos;
	CString strNewPath;

	// 백업 디렉토리 설정
	nPos = strPath.Find('\\');
	strNewPath.Format("%s%s%s", strPath.Left(nPos), g_szBackupPath, strPath.Mid(nPos));

	return strNewPath;
}


float entropy_calc(long byte_count[], int length)
{
	float entropy = 0.0;
	float count;
	int i;

	/* entropy calculation */
	for (i = 0; i < 256; i++) // 1byte? #define SIZE 256
	{
		if (byte_count[i] != 0)
		{
			count = (float)byte_count[i] / (float)length;
			entropy += -count * log2f(count);
		}
	}
	return entropy;
}


float GetFileEntropy(FILE *inFile, int offset)
{
	int             i;
	int             j;
	int             n;			// Bytes read by fread;
	int             length = 0;	// length of file
	float           count;
	float           entropy;
	long            byte_count[256];
	unsigned char   buffer[10240];

	memset(byte_count, 0, sizeof(long) * 256);

	fseek(inFile, offset, SEEK_SET);

	/* Read the whole file in parts of 1024 */
	while ((n = fread(buffer, 1, 10240, inFile)) != 0)
	{
		/* Add the buffer to the byte_count */
		for (i = 0; i < n; i++)
		{
			byte_count[(int)buffer[i]]++;
			length++;
		}
	}
	fclose(inFile);
	
	return entropy_calc(byte_count, length);;
}


int CAntiRansomwareUserDlg::DoCheckRansomware(CString strPath)
{
	int nPos;
	int nResult;
	bool isDeleteBackupFile = true;
	FILE* fpTarget;
	FILE* fpBackup;
	CString strBackupPath;
	ITEM_BACKUP_FILE tmpIBF;
	list<ITEM_BACKUP_FILE>::reverse_iterator ritorIBF; // backup
	unsigned char buf_sigTarget[4];
	unsigned char buf_sigBackup[4];
	unsigned int size_target;
	unsigned int size_backup;
	float entropy_target;
	float entropy_backup;
	bool isChangeEntropy = false;
	CString strTemp;
	HANDLE hFile;

	// 백업 경로 구하기
	strBackupPath = GetBackupFilePath(strPath);

	// 백업 파일 존재 확인
	if (PathFileExists(strBackupPath) == FALSE) {
		AddLogList("백업 파일 없음: " + strBackupPath);
		return 0;
	}

	// 비교 파일 열기
	fpTarget = fopen((LPSTR)(LPCTSTR)strPath, "rb");
	if (fpTarget == NULL){
		//AddLogList("파일 열기 실패(fpTarget) : " + strPath);
		return -1;
	}

	fpBackup = fopen((LPSTR)(LPCTSTR)strBackupPath, "rb");
	if (fpBackup == NULL){
		//AddLogList("파일 열기 실패(fpBackup) : " + strBackupPath);
		fclose(fpTarget);
		return -1;
	}

	// STEP1: 파일 시그니처 비교
	size_target = fread(buf_sigTarget, 1, 4, fpTarget);
	size_backup = fread(buf_sigBackup, 1, 4, fpBackup);
	if (size_target == size_backup) {
		nResult = memcmp(buf_sigTarget, buf_sigBackup, 4);
		if (nResult != 0) {
			AddLogList("Warning: 파일 시그니처 변조");
			fclose(fpTarget);
			fclose(fpBackup);
			return 1;
		}
	}
	else {
		AddLogList("Warning: 파일 시그니처 제거");
		fclose(fpTarget);
		fclose(fpBackup);
		return 1;
	}

	// STEP2: 파일 정보 엔트로피 분석(text)
	entropy_target = GetFileEntropy(fpTarget, 0);
	entropy_backup = GetFileEntropy(fpBackup, 0);

	if (entropy_backup < 6.5) {
		if (entropy_target > 7.0)
			isChangeEntropy = true;
	}
	else if (entropy_backup < 7.5) {
		if (entropy_target > 7.9)
			isChangeEntropy = true;
	}
	
	if(isChangeEntropy){
		strTemp.Format("Warning: 엔트로피 변화 (%02.5f -> %02.5f, %02.5f)", entropy_backup, entropy_target, abs(entropy_target - entropy_backup));
		AddLogList(strTemp);
		return 1;
	}

	// STEP3: [ssdeep] Fuzzy Hash 비교(binary)
	int nFuzzy;
	char * resultFuzzy, *resultFuzzy2;
	resultFuzzy = (char *)malloc(FUZZY_MAX_RESULT);
	resultFuzzy2 = (char *)malloc(FUZZY_MAX_RESULT);
	nResult = fuzzy_hash_filename((LPSTR)(LPCTSTR)strPath, resultFuzzy);
	nResult = fuzzy_hash_filename((LPSTR)(LPCTSTR)strBackupPath, resultFuzzy2);

	nFuzzy = fuzzy_compare(resultFuzzy, resultFuzzy2);
	if (nFuzzy >= 0) {
		strTemp.Format("Fuzzy Hash : score = %d\n", nFuzzy);
		AddLogList(strTemp);
		if (nFuzzy <= 20) {
			AddLogList("Warning: Fuzzy Hash 불일치");
			return 1;
		}
	}

	free(resultFuzzy);
	free(resultFuzzy2);

	fclose(fpTarget);
	fclose(fpBackup);

	// 백업 파일 삭제(다른 백업 이벤트가 없다면)
	/*
	ritorIBF = m_listBackupFile.rbegin();
	while (ritorIBF != m_listBackupFile.rend())
	{
		if (strBackupPath.Compare(ritorIBF->strPath) == 0) {
			AddLogList("백업 파일 삭제 안함");
			isDeleteBackupFile = false;
			break;
		}
		ritorIBF++;
	}
	*/
	if (isDeleteBackupFile == true) {
		AddLogList("백업 파일 삭제: " + strBackupPath);
		DeleteFile(strBackupPath); // 백업 파일 삭제
	}

	return 0;
}

bool CAntiRansomwareUserDlg::DoPopupInfoWindow()
{
	// 알림
	if (m_pAntiRansomwarePopupDlg.GetSafeHwnd() == NULL) {
		m_pAntiRansomwarePopupDlg.Create(IDD_ANTIRANSOMWAREPOPUPDLG);
		m_pAntiRansomwarePopupDlg.InitPopupWindow(m_sPopupMessage);
	}
	m_pAntiRansomwarePopupDlg.ShowWindow(SW_SHOW);

	return true;
}


// 파일 암호화 감시 스레드
static UINT CheckRansomwareWorker(LPVOID lpParam)
{
	CAntiRansomwareUserDlg* pDlg = (CAntiRansomwareUserDlg*)lpParam;
	list<ITEM_CHECK_FILE> *listFile = (list<ITEM_CHECK_FILE>*)&pDlg->m_listCheckFile;
	list<ITEM_CHECK_FILE>::iterator itor;

	bool result;
	bool isNewFile;
	int nResult;
	CString strTemp;
	ArProcessBehavior* itemArProcessBehavior;

	while (pDlg->m_isRunning) {
		result = false;
		isNewFile = false;
		// Wait Event
		WaitForSingleObject(pDlg->m_hEventCheckRansomware, INFINITE);
		pDlg->m_nCountMonitor++;
		EnterCriticalSection(&pDlg->m_csFileQueue);

		if (!listFile->empty()) {
			for (itor = listFile->begin(); itor != listFile->end();) {
				if(&itor != NULL){
					// 탐지 시도
					if (itor->nCheckType == 0)								// 행위기반 탐지
						nResult = pDlg->DoCheckRansomware(itor->strPath);	// - 랜섬웨어 감염 확인
					else if (itor->nCheckType == 1)							// 시그니처 탐지
						nResult = pDlg->DoMatchRansomwareDB(itor->strPath);	// - Yara rule

					// 결과
					if (nResult == 1) { // 파일 변조됨
						if (itor->nCheckType == 0) { // 행위기반
							itemArProcessBehavior = pDlg->m_mapProcessBehavior[itor->pid];
							itemArProcessBehavior->AddEventWriteSpFile(itor->strPath, true); // 의심
							strTemp.Format("[%d] PB_COUNT_WRITE_SP: %d", itor->pid, itemArProcessBehavior->GetCountBehavior(PB_COUNT_WRITE_SP));
							pDlg->AddLogList(strTemp);
						}
						else if (itor->nCheckType == 1) { // 시그니처
							// 파일 삭제
							// 알림
						}
						itor = pDlg->m_listCheckFile.erase(itor); // 항목 삭제
					}
					else if (nResult == 0) { // 정상
						itor = pDlg->m_listCheckFile.erase(itor); // 항목 삭제
					}
					else if (nResult == -1) { // Open 실패
						if (itor->nCheckCount < 10)
							isNewFile = true;
						itor->nCheckCount++;
						if (PathFileExists(itor->strPath) == FALSE) {
							strTemp.Format("[L] 파일 없음: %s", itor->strPath);
							pDlg->AddLogList(strTemp);
							itor = pDlg->m_listCheckFile.erase(itor); // 항목 삭제
						}
						else {
							if (itor->nCheckCount > 100) {
								pDlg->AddLogList("[L] 파일 열기 실패 (check count)");
								itor = pDlg->m_listCheckFile.erase(itor); // 항목 삭제
							}
							else {
								itor++;
							}
						}
					}
				}
			}
			result = true;
		}
		else {
			// Reset Event
			ResetEvent(pDlg->m_hEventCheckRansomware);
		}

		LeaveCriticalSection(&pDlg->m_csFileQueue);

		if(result){
			if(isNewFile)
				Sleep(1);
			else
				Sleep(10);
		}
		else{
			Sleep(10);
		}
	}

	pThreadCheckRansomware = NULL;
	return 0;
}


bool CAntiRansomwareUserDlg::DoKillRecoveryRansomware(DWORD pid)
{
	map<unsigned int, ArProcessBehavior*>::iterator itor;
	ArProcessBehavior* itemArProcessBehavior;

	// 행위 데이터 검색
	for (itor = m_mapProcessBehavior.begin(); itor != m_mapProcessBehavior.end(); ++itor) {
		itemArProcessBehavior = itor->second;
		if (itemArProcessBehavior->GetProcessInfo(PB_PROC_PID) == pid){
			itemArProcessBehavior->RecoveryProcessBehavior(); // 파일 복구
		}
		else if (itemArProcessBehavior->GetProcessInfo(PB_PROC_PPID) == pid){
			if(itemArProcessBehavior->GetProcessInfo(PB_PROC_PPID) != itemArProcessBehavior->GetProcessInfo(PB_PROC_PID))
				DoKillRecoveryRansomware(itemArProcessBehavior->GetProcessInfo(PB_PROC_PID)); // 재귀 호출
		}
	}

	return true;
}


void CAntiRansomwareUserDlg::OnBnClickedButtonRecovery()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString strTemp;
	ArProcessBehavior* itemArProcessBehavior;
	DWORD tmpPid;

	ctr_editTargetPid.GetWindowTextA(strTemp);
	tmpPid = (DWORD)atoi((LPSTR)(LPCTSTR)strTemp);
	if (m_mapProcessBehavior.find(tmpPid) != m_mapProcessBehavior.end()){
		itemArProcessBehavior = m_mapProcessBehavior[tmpPid];
		itemArProcessBehavior->RecoveryProcessBehavior(); // 파일 복구
	}
}


bool CAntiRansomwareUserDlg::AddCheckRansomwareFile(DWORD pid, CString strPath)
{
	CString strExt;
	CString strTemp;
	ITEM_CHECK_FILE tmpICF;

	tmpICF.pid = pid;
	tmpICF.strPath = strPath;
	tmpICF.nCheckCount = 0;

	strExt = PathFindExtension(strPath);
	strExt = strExt.Mid(1);
	if (strExt.Compare("exe") != 0)
		tmpICF.nCheckType = 0; // 행위기반 탐지
	else
		tmpICF.nCheckType = 1; // 시그니처 탐지

	EnterCriticalSection(&m_csFileQueue);

	// 중복검사
	list<ITEM_CHECK_FILE>::iterator itor = m_listCheckFile.begin();
	while (itor != m_listCheckFile.end()) {
		if (strPath.Compare((CString)itor->strPath) == 0) {
			LeaveCriticalSection(&m_csFileQueue);
			return true;
		}
		itor++;
	}

	// 검사 대상 파일 큐에 추가
	m_listCheckFile.push_back(tmpICF);
	SetEvent(m_hEventCheckRansomware);

	LeaveCriticalSection(&m_csFileQueue);

	strTemp.Format("검사 항목 추가: %s", strPath);
	AddLogList(strTemp);

	return true;
}

void CAntiRansomwareUserDlg::OnBnClickedButtonClose()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	PostQuitMessage(WM_QUIT);
}


void CAntiRansomwareUserDlg::OnBnClickedButtonMinimum()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_sPopupMessage.typePopup = 1;
	m_sPopupMessage.strTitle = "랜섬웨어 탐지!";
	m_sPopupMessage.strMessage1 = "랜섬웨어 의심 행위가 탐지되었습니다.";
	m_sPopupMessage.strMessage2 = "해당 실행 파일을 삭제하시겠습니까 ?";
	m_sPopupMessage.pid = 1234;
	m_sPopupMessage.strProcName = "Name";
	m_sPopupMessage.strProcPath = "Path";

	g_pParent->DoPopupInfoWindow(); // 팝업창
}


void CAntiRansomwareUserDlg::OnBnClickedButtonToggle1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SetDetectionEngine();
}


void CAntiRansomwareUserDlg::OnBnClickedButtonToggle2()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SetDetectionEngine();
}


bool CAntiRansomwareUserDlg::SetDetectionEngine()
{
	int nEngines = 0;
	if (m_gbtnToggle1.IsToggled() == TRUE) {
		nEngines += 1;
	}
	if (m_gbtnToggle2.IsToggled() == TRUE) {
		nEngines += 1;
	}

	switch (nEngines)
	{
		case 0:
			lamp_image.LoadBitmap(IDB_BITMAP_MENU1_DANGER);
			break;
		case 1:
			lamp_image.LoadBitmap(IDB_BITMAP_MENU1_WARNING);
			break;
		case 2:
			lamp_image.LoadBitmap(IDB_BITMAP_MENU1_SAFETY);
			break;
	}

	HBITMAP h_old_bitmap = ctr_picMenu1.SetBitmap(lamp_image);
	if (h_old_bitmap != NULL) ::DeleteObject(h_old_bitmap);
	lamp_image.Detach();
	
	m_gbtnToggle1.Invalidate(TRUE);
	m_gbtnToggle1.SetBkGnd();
	m_gbtnToggle2.Invalidate(TRUE);
	m_gbtnToggle2.SetBkGnd();
	
	return true;
}

void CAntiRansomwareUserDlg::OnBnClickedButtonMenu2()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_strDetectedPath = "c:\\test.exe";
	PostMessageA(WM_POPUP_INFO_WINDOW, 1, NULL); // 팝업창
}


void CAntiRansomwareUserDlg::OnBnClickedButtonMenu3()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	bool result;
	result = DoUpdateSignatureDB();
	
}


void CAntiRansomwareUserDlg::OnBnClickedButtonMenu4()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	int nResult;
	DoMatchRansomwareDB("E:\\【Clavis】\\【Programming】\\【Project】\\Anti_Ransomware\\TestRansomware\\Release\\TestRansomware.exe");
}


void CAntiRansomwareUserDlg::OnBnClickedButtonMenu5()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (m_pAntiRansomwareSettingsDlg.GetSafeHwnd() == NULL) {
		m_pAntiRansomwareSettingsDlg.Create(IDD_ANTIRANSOMWARESETTINGSDLG);
		m_pAntiRansomwareSettingsDlg.CenterWindow(CWnd::FromHandle(this->m_hWnd));
	}
	m_pAntiRansomwareSettingsDlg.ShowWindow(SW_SHOW);
}

bool CAntiRansomwareUserDlg::DoSendRansomwareFile(CString strPath)
{
	bool result;
	CString strHeader;
	CString strExt;

	result = m_fileTransfer.ConnectToServer(SERVER_IP, SERVER_PORT);
	if (result == false) {
		AfxMessageBox("Failed!");
	}

	// 확장자 분리
	strExt = PathFindExtension(strPath);
	strExt = strExt.Mid(1);

	strHeader.Format("CheckVM Detected %s", strExt);
	m_fileTransfer.SendFile(strHeader.GetBuffer(), strPath.GetBuffer());
	

	m_fileTransfer.Disconnect();
	return true;
}

bool CAntiRansomwareUserDlg::DoUpdateSignatureDB()
{
	bool result;
	CString strHeader;
	CString strExt;
	HZIP hz;
	ZIPENTRY ze;
	char szFileName[1024] = { 0 };
	char szFilePath[1024] = { 0 };

	sprintf(szFilePath, "%s\\db\\", appPath);
	sprintf(szFileName, "%supdb_ar.zip", szFilePath);

	result = m_fileTransfer.ConnectToServer(SERVER_IP, SERVER_PORT);
	if (result == false) {
		AfxMessageBox("Failed!");
	}

	strHeader.Format("UpdateDB");
	m_fileTransfer.RecvFile(strHeader.GetBuffer(), szFileName);

	m_fileTransfer.Disconnect();
	
	hz = OpenZip(_T(szFileName), 0);
	SetUnzipBaseDir(hz, _T(szFilePath));
	GetZipItem(hz, -1, &ze);
	int numitems = ze.index;
	for (int zi = 0; zi<numitems; zi++)
	{
		GetZipItem(hz, zi, &ze);
		UnzipItem(hz, zi, ze.name);
	}
	CloseZip(hz);
	DeleteFile(szFileName); // 원본 파일 삭제

	return true;
}

int CAntiRansomwareUserDlg::DoMatchRansomwareDB(CString strPath)
{
	FILE* file;
	HANDLE hFile;
	char output[1024];
	char command[1024];
	CString strTemp;

	hFile = CreateFile(strPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return -1; // Open 실패
	else
		CloseHandle(hFile);

	// Temp - Yara Test
	sprintf(command, "%s\\yara32.exe %s\\db\\AR.yar %s", appPath, appPath, strPath);

	file = _popen(command, "r");
	fread(output, 1, sizeof(output), file);
	fclose(file);
	if (strstr(output, strPath)) {
		strTemp.Format("랜섬웨어 탐지! - Path: %s", strPath);
		g_pParent->AddLogList(strTemp);
		m_strDetectedPath = strPath;
		PostMessageA(WM_POPUP_INFO_WINDOW, 1, NULL); // 팝업창
		return 1;
	}
	return 0;
}