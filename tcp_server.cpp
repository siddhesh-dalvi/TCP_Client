#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#include <ppltasks.h>
#include <concrt.h>
#include <iostream>
#include <sstream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <array>
#include <ppl.h>

using namespace concurrency;
using namespace std;

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int processPid[10];


int sendall(SOCKET s, char* buf, int* len)
{
    int total = 0; // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n = {};
    
    while (total < *len) {
        n = send(s, buf + total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    
    * len = total; // return number actually sent here
   
    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}



/*BOOL CALLBACK SendWMCloseMsg(HWND hwnd, LPARAM lParam)
{
    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hwnd, &dwProcessId);

    if (dwProcessId == lParam)
        SendMessageTimeout(hwnd, WM_CLOSE, 0, 0, SMTO_ABORTIFHUNG, 30000, NULL);
    //::PostMessage(hwnd, WM_CLOSE, 0, 0);
    return TRUE;
}*/


void TerminateProcess(SOCKET ConnectSocket) {
    /*char buf[DEFAULT_BUFLEN];
    int len;
   // HANDLE ps = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE,
       // FALSE, processPid);
    EnumWindows(&SendWMCloseMsg, processPid);
    cout << "Dummy process terminated SUCCESSFULY" << endl;
    strcpy_s(buf, "Dummy process TERMINATED SUCCESSFULY");
    len = strlen(buf);
    if (sendall(ConnectSocket, buf, &len) == -1) {
         perror("sendall");
         printf("We only sent %d bytes because of the error!\n", len); 
    }

    //CloseHandle(ps);*/


    for (int i = 0; i < 3; i++)
    {
        HANDLE explorer;
        explorer = OpenProcess(PROCESS_ALL_ACCESS, false, processPid[i]);
        TerminateProcess(explorer, 1);
        CloseHandle(explorer);
    }
}




void startup(SOCKET ConnectSocket)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char buf[DEFAULT_BUFLEN];
    string my_str = "C:\\Users\\Siddhesh Dalvi\\source\\repos\\Server_Client\\Release\\DummyProject.exe";
    LPCSTR lpApplicationName;
    int len;
    lpApplicationName = my_str.c_str();
    static int i = 0;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // start the program up
    if (!CreateProcess
    (
        lpApplicationName,   // the path
        NULL,                // Command line
        NULL,                   // Process handle not inheritable
        NULL,                   // Thread handle not inheritable
        FALSE,                  // Set handle inheritance to FALSE
        CREATE_NEW_CONSOLE,     // Opens file in a separate console
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi)          // Pointer to PROCESS_INFORMATION structure
        ) {
        cout << "Failed to start Dummy process" << endl;
        strcpy_s(buf, "Failed to start Dummy process");
        len = strlen(buf);
        if (sendall(ConnectSocket, buf, &len) == -1) {
            perror("sendall");
            printf("We only sent %d bytes because of the error!\n", len);
        }
    }
    else {
        //cout << "Dummy process started SUCCESSFULY" << endl;
        strcpy_s(buf, "Dummy process started SUCCESSFULY");
        cout << buf << endl;
        len = strlen(buf);
        if (sendall(ConnectSocket, buf, &len) == -1) {
            perror("sendall");
            printf("We only sent %d bytes because of the error!\n", len);
        }
    }
    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, 0);
    processPid[i++] = pi.dwProcessId;
    cout << "Process PID : " << processPid[i] << endl;
    // Close process and thread handles. 
    //CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return;
}

int main(void)
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    task_group tasks;

    // Initialize Winsock
    cout << "Initialising Winsock..." << endl;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    cout << "Initialised.\n" << endl;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
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

    cout << "Socket created." << endl;
    // Setup the TCP listening socket
    iResult = ::bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Waiting for incoming connections..." << endl;

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Connection accepted from Client..." << endl;
    // No longer need server socket
    closesocket(ListenSocket);

    // Receive until the peer shuts down the connection
    do {

        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            cout << "Data received : " << recvbuf << endl;
            if (strcmp(recvbuf, "start") == 0)
            {
                wcout << L"Creating task..." << endl;
                // Create a task that performs work until it is canceled.
                parallel_invoke(
                    [&] {tasks.run([ClientSocket] { startup(ClientSocket); cout << "Creating Process 1" << endl; });
                         tasks.run([ClientSocket] { startup(ClientSocket); cout << "Creating Process 1" << endl; });
                    },
                    [&] {
                        // Add one additional task to the task_group object.
                        tasks.run([ClientSocket] { startup(ClientSocket); cout << "Creating Process 1" << endl; });
                    }
                    );
                // Wait for all tasks to finish.
                tasks.wait();
            }
            else
            {
                wcout << L"Canceling task..." << endl;
                TerminateProcess(ClientSocket);
            }
        }
        else if (iResult == 0)
            printf("Connection closed in tcp_client\n");
        else
            printf("recv failed with error in tcp_client: %d\n", WSAGetLastError());
    } while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}