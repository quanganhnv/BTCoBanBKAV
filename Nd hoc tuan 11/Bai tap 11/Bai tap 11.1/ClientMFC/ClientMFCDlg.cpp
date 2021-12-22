#include "pch.h"
#include "framework.h"
#include "ClientMFC.h"
#include "ClientMFCDlg.h"
#include "afxdialogex.h"
#include "connectToSever.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CClientMFCDlg dialog
DWORD WINAPI Checkonline(LPVOID lpParam);
BOOL g_checkingConnect = FALSE;

CClientMFCDlg::CClientMFCDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CLIENTMFC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CClientMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST3, boxOnlineUser);
	DDX_Control(pDX, IDC_LIST1, boxMessages);
}

BEGIN_MESSAGE_MAP(CClientMFCDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CClientMFCDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CClientMFCDlg::OnBnClickedButton2)

	ON_BN_CLICKED(IDC_BUTTON3, &CClientMFCDlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CClientMFCDlg message handlers

BOOL CClientMFCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	//ConnectToServer();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CClientMFCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CClientMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CClientMFCDlg::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
	//
	if (g_checkConnect)
	{
		wchar_t TempUserWchar[20] = { 0 };
		char KiTuLogin[27] = { 0 };		
		char TempUserChar[20] = { 0 };
		GetDlgItem(IDC_EDIT2)->GetWindowTextW((LPWSTR)TempUserWchar,sizeof(TempUserWchar));
		// Chuyen wchar_t sang char, unicode
		WideCharToMultiByte(CP_ACP, 0, TempUserWchar, sizeof(TempUserWchar), TempUserChar, sizeof(TempUserChar), 0, 0);// UNICODE
		lstrcatA(KiTuLogin, "login:");
		lstrcatA(KiTuLogin, TempUserChar);
		if (ConnectServer() != 0)
			MessageBox(L"Notification", L"Can't connect to network", MB_OK);
		g_iResult = send(g_CSocket, KiTuLogin, (int)strlen(KiTuLogin), 0);
		if (g_iResult == SOCKET_ERROR) {
			closesocket(g_CSocket);
			WSACleanup();
		}
		HANDLE hThread = CreateThread(0, 0, Checkonline, this, 0, 0);
		g_checkConnect = FALSE;
	}
}

void CClientMFCDlg::OnBnClickedButton2()
{
	// TODO: Add your control notification handler code here
	if(g_checkingConnect == TRUE)
	{
		wchar_t Msg[512] = { 0 };
		char TmpMessage[512] = { 0 };
		char CharMessage[512] = { 0 };
		GetDlgItem(IDC_EDIT1)->GetWindowTextW(Msg, sizeof(Msg)); // Get text
		WideCharToMultiByte(CP_ACP, 0, Msg, sizeof(Msg), TmpMessage, sizeof(TmpMessage), 0, 0);
		lstrcatA(CharMessage, "TinGui:");
		lstrcatA(CharMessage, TmpMessage);
		g_iResult = send(g_CSocket, CharMessage, (int)strlen(CharMessage), 0); // gui text
		if (g_iResult == SOCKET_ERROR) {
			closesocket(g_CSocket);
			WSACleanup();
		}

	}
}


void CClientMFCDlg::OnBnClickedButton3()
{
	int index = boxOnlineUser.GetCurSel();
	wchar_t TempUserWchar[20] = { 0 };
	char TempUserChar[20] = { 0 };
	char ConnectUser[27] = { 0 };
	if (g_checkingConnect == FALSE)
	{
		if (index != LB_ERR) // nếu chọn ko lỗi thì
		{
			boxOnlineUser.GetText(index, (LPWSTR)TempUserWchar);
			WideCharToMultiByte(CP_ACP, 0, TempUserWchar, sizeof(TempUserWchar), TempUserChar, sizeof(TempUserChar), 0, 0);

			lstrcatA(ConnectUser, "ketnoi:");
			lstrcatA(ConnectUser, TempUserChar);
			g_iResult = send(g_CSocket, ConnectUser, (int)strlen(ConnectUser), 0);
			if (g_iResult == SOCKET_ERROR) {
				closesocket(g_CSocket);
				WSACleanup();
			}
			g_checkingConnect = TRUE;
		}
	}
}
DWORD WINAPI Checkonline(LPVOID lpParam)
{
	CClientMFCDlg* pObject = (CClientMFCDlg*)lpParam;
	int iResult = 0;
	char recvThread[512] = { 0 };
	char KiTuOnline[7] = { 0 };
	char UserName[20] = { 0 };
	wchar_t TempUserWchar[20] = { 0 };
	wchar_t Msg[512] = { 0 };
	while (TRUE)
	{
		ZeroMemory(KiTuOnline, sizeof(KiTuOnline));
		ZeroMemory(recvThread, sizeof(recvThread));
		
		iResult = recv(g_CSocket, recvThread, sizeof(recvThread), 0);

		lstrcpynA(KiTuOnline, recvThread, sizeof(KiTuOnline)+1);
		if (lstrcmpA(KiTuOnline, "online:") == 0)
		{
			lstrcpyA(UserName, &recvThread[7]);
			MultiByteToWideChar(CP_ACP, 0, UserName, sizeof(UserName) , TempUserWchar, sizeof(TempUserWchar) );
			pObject->boxOnlineUser.AddString(TempUserWchar);
			ZeroMemory(TempUserWchar, sizeof(TempUserWchar));
			ZeroMemory(UserName, sizeof(UserName));
		}

		else if (lstrcmpA(KiTuOnline, "TinGui:") == 0)
		{
			MultiByteToWideChar(CP_ACP, 0, recvThread, sizeof(recvThread), Msg, sizeof(Msg));
			pObject->boxMessages.AddString(Msg);
			ZeroMemory(Msg, sizeof(Msg));

		}
	}
	return 0;
}




