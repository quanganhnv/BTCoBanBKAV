#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
DWORD WINAPI User1(LPVOID lParam);
DWORD WINAPI User2(LPVOID lParam);

using namespace std;

int g_iResult;
char g_recvbuf[DEFAULT_BUFLEN] = { 0 };
int g_recvbuflen = DEFAULT_BUFLEN;

SOCKET g_client[FD_SETSIZE] = { 0 };
SOCKET g_acceptSock = INVALID_SOCKET;

char g_recvUser1[DEFAULT_BUFLEN] = { 0 };
char g_recvUser2[DEFAULT_BUFLEN] = { 0 };
int indexUser1 = 0, indexUser2 = 0;

int main(void)
{
    char NameOnline[20] = { 0 };
    typedef struct InfoClient
    {
        int cSocket;
        char NameUser[20];
    }iCLIENT, * STRUCTINFOCLIENT;
    STRUCTINFOCLIENT infoClient[FD_SETSIZE];
    WSADATA wsaData;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    // Initialize Winsock
    g_iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (g_iResult != 0) {
        printf("WSAStartup failed with error: %d\n", g_iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    g_iResult = getaddrinfo(0, DEFAULT_PORT, &hints, &result);
    if (g_iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", g_iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    g_iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (g_iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);

    g_iResult = listen(ListenSocket, SOMAXCONN);
    if (g_iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    //Setup Event
    FD_SET Readfds, Initfds;
    FD_ZERO(&Initfds);
    FD_SET(ListenSocket, &Initfds);

    while (1)
    {
        Readfds = Initfds;
        int nEvents = select(0, &Readfds, 0, 0, 0);

        if (FD_ISSET(ListenSocket, &Readfds))
        {
            if ((g_acceptSock = accept(ListenSocket, NULL, NULL)) <= 0)
            {
                printf("\nError! Cannot accept new connection: %d", WSAGetLastError());
                break;
            }
            else
            {
                char TempRecv[6] = { 0 };
                char OnlineUser[27] = { 0 };
                int i;
                for (i = 0; i < FD_SETSIZE; i++) // nhiều nhất 64 Client
                {
                    if (g_client[i] == 0)
                    {
                        g_client[i] = g_acceptSock;
                        FD_SET(g_client[i], &Initfds);
                        printf("Nguoi dung: %d \n", i);

                        g_iResult = recv(g_client[i], g_recvbuf, g_recvbuflen, 0);
                        ZeroMemory(OnlineUser, sizeof(OnlineUser));

                        lstrcpyn(TempRecv, g_recvbuf, sizeof(TempRecv) + 1);

                        if (lstrcmp(TempRecv, "login:") == 0) // Xử lý login
                        {
                            infoClient[i] = (STRUCTINFOCLIENT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(iCLIENT));
                            infoClient[i]->cSocket = g_client[i]; 
                            lstrcpy(infoClient[i]->NameUser, &g_recvbuf[6]);

                            send(g_client[i], "successlogin", (int)lstrlen("successlogin"), 0);

                            lstrcat(OnlineUser, "online:");

                            // Gửi tới tất cả user Client[j] đang onl, tên Client[i] là
                            lstrcat(OnlineUser, infoClient[i]->NameUser); // Tên thằng Client[i]
                            for (int j = 0; j < FD_SETSIZE; j++)
                            {
                                if (g_client[j] != g_client[i] && g_client[j] != 0)
                                {
                                    send(g_client[j], OnlineUser, (int)lstrlen(OnlineUser), 0);
                                }
                            }
                            // Gửi tất tả user Đang onl cho client[i]
                            printf("ok\n");
                            for (int j = 0; j < FD_SETSIZE; j++)
                            {
                                if (g_client[j] != 0 && g_client[i] != g_client[j])
                                {
                                    Sleep(100);
                                    ZeroMemory(OnlineUser, sizeof(OnlineUser));
                                    lstrcat(OnlineUser, "online:");
                                    lstrcat(OnlineUser, infoClient[j]->NameUser);
                                    send(g_client[i], OnlineUser, (int)strlen(OnlineUser), 0);

                                }
                            }
                        }
                        break;
                    }
                }
                if (i == FD_SETSIZE)
                {
                    printf("\nToo many clients.");
                    closesocket(g_acceptSock);
                }

                if (--nEvents == 0)
                    continue; //no more event
            }

        }
        //receive data from clients

        char TempRecv[7] = { 0 };
        char CharNameUser[27] = { 0 };
        int j = 0;
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (g_client[i] == 0)
                continue;
            BOOL chapnhan = FALSE;
            if (FD_ISSET(g_client[i], &Readfds))
            {
                g_iResult = recv(g_client[i], g_recvbuf, g_recvbuflen, 0);
                if (g_iResult <= 0)  // không có tin nhắn
                {
                    FD_CLR(g_client[i], &Initfds);
                    closesocket(g_client[i]);
                    g_client[i] = 0;
                    infoClient[i]->cSocket = 0;
                    infoClient[i]->NameUser[0] = { 0 };
                    HeapFree(GetProcessHeap(), 0, infoClient[i]);
                    infoClient[i] = NULL;    // infoclient không trùng
                }
                else if (g_iResult > 0)  // Co tin
                {
                    lstrcpyn(TempRecv, g_recvbuf, sizeof(TempRecv) + 1);//Copy chu login 
                    if (lstrcmp(TempRecv, "ketnoi:") == 0) 
                    {
 
                        lstrcpy(CharNameUser, &g_recvbuf[7]); 
                        for (j = 0; j < FD_SETSIZE; j++)
                        {
                            if (lstrcmp(CharNameUser, infoClient[j]->NameUser) == 0) 
                            {
                                indexUser1 = i;
                                indexUser2 = j;
                                chapnhan = TRUE;
                                break;
    
                            }
                        }

                    }

                }
            }
            if (chapnhan == TRUE)
            {
                while (1)
                {
                    HANDLE hThread1 = CreateThread(0, 0, User1, 0, 0, 0);
                    HANDLE hThread2 = CreateThread(0, 0, User2, 0, 0, 0);
                }
            }
        }

        if (--nEvents <= 0)
            continue; 
    }

    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}
DWORD WINAPI User1(LPVOID lParam)
{
    int tempResult=0;
    char TempRecv[10] = { 0 };
    tempResult = recv(g_client[indexUser1], g_recvUser1, g_recvbuflen, 0);

    if(tempResult>0) // Co tin nhan
    {
        char UMessage[DEFAULT_BUFLEN] = { 0 };
        lstrcpyn(TempRecv, g_recvUser1, 8);
        if (lstrcmp(TempRecv, "TinGui:") == 0)
        {
            lstrcpy(UMessage, g_recvUser1);
            send(g_client[indexUser2], UMessage, DEFAULT_BUFLEN, 0);
        }
        ZeroMemory(g_recvUser1, sizeof(g_recvUser1));
        //break;
    }
    return 0;
}
DWORD WINAPI User2(LPVOID lParam)
{
    int tempResult = 0;
    char TempRecv[10] = { 0 };
    tempResult = recv(g_client[indexUser2], g_recvUser2, g_recvbuflen, 0);

    if(tempResult>0) //Co tin nhan
    {
        char UMessage[DEFAULT_BUFLEN] = { 0 };
        lstrcpyn(TempRecv, g_recvUser2, 8);
        if (lstrcmp(TempRecv, "TinGui:") == 0)
        {

            lstrcpy(UMessage, g_recvUser2); 
            send(g_client[indexUser1], UMessage, DEFAULT_BUFLEN, 0);
        }
        ZeroMemory(g_recvUser2, sizeof(g_recvUser2));
    
    }
    return 1;
}

