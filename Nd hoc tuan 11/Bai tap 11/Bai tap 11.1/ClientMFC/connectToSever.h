#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

WSADATA g_wsaData;
SOCKET g_CSocket = INVALID_SOCKET;

struct addrinfo* result = NULL,
    * ptr = NULL,
    hints;
int g_iResult;
int ConnectServer()
{
    //CClientMFCDlg* pObject = (CClientMFCDlg*)lpParam;
    // Initialize Winsock
    g_iResult = WSAStartup(MAKEWORD(2, 2), &g_wsaData);
    if (g_iResult != 0) {
        printf("WSAStartup failed with error: %d\n", g_iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    g_iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
    if (g_iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", g_iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        g_CSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (g_CSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        g_iResult = connect(g_CSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (g_iResult == SOCKET_ERROR) {
            closesocket(g_CSocket);
            g_CSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (g_CSocket == INVALID_SOCKET) {
        printf("Can't connect to server!\n");
        WSACleanup();
        return 1;
    }

    return 0;
}