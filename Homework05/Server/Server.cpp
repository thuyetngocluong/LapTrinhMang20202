// WSAAsyncSelectServer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Server.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "winsock2.h"
#include <WS2tcpip.h>
#include "windows.h"
#include "stdio.h"
#include "conio.h"
#include <WinUser.h>
#include <commctrl.h>
#include <string.h>
#include <iostream>

#include "Account.h"


#pragma comment (lib,"ws2_32.lib")

#define ID_LISTVIEW  1000
#define NUM_COLUMNS  4
#define WM_SOCKET WM_USER + 1

#define SERVER_PORT 6000
#define MAX_CLIENT 1024
#define BUFF_SIZE 2048
#define SERVER_ADDR "127.0.0.1"


HWND serverWindow;
WSADATA wsaData;
WORD wVersion = MAKEWORD(2, 2);//# version of winsock
const LPWSTR cols[5] = { L"Address", L"Time", L"Command", L"Content"};//#define name of columns
Account* accounts[MAX_CLIENT];//# List account
static HWND hWndListView;
SOCKET listenSock;
HINSTANCE hInst;

struct ItemListView {
	LPWSTR address;
	LPWSTR time;
	LPWSTR command;
	LPWSTR content;

	ItemListView(LPWSTR _address, LPWSTR _time, LPWSTR _command, LPWSTR _content) {
		this->address = _address;
		this->time = _time;
		this->command = _command;
		this->content = _content;
	}
};

vector<ItemListView> listItem;//List item of list view


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
HWND				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	windowProc(HWND, UINT, WPARAM, LPARAM);
HWND				CreateListView(HWND);
LRESULT				NotifyHandler(HWND, UINT, WPARAM, LPARAM);
void				InsertListView(HWND, int);

void				initWinsock();
void				terminateWinsock();
SOCKET				createTCPSocket(int);
void				associateLocalAddressWithSocket(SOCKET&, char*, int);
void				Listen(SOCKET&, int);

bool				addAccount(Account*);
void				deleteAccount(Account*);

Message				solveLoginRequest(Account*, Message&);
Message				solvePostRequest(Account*, Message&);
Message				solveLogoutRequest(Account*, Message&);
Message				solveRequest(Account*, Message&);
void				processAccount(Account*);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	serverWindow;
	memset(accounts, NULL, sizeof(accounts));

	//Registering the Window Class
	MyRegisterClass(hInstance);

	//Create the window
	if ((serverWindow = InitInstance(hInstance, nCmdShow)) == NULL)
		return FALSE;

	try {
		initWinsock();

		listenSock = createTCPSocket(AF_INET);

		//requests Windows message-based notification of network events for listenSock
		WSAAsyncSelect(listenSock, serverWindow, WM_SOCKET, FD_ACCEPT | FD_CLOSE | FD_READ);

		associateLocalAddressWithSocket(listenSock, SERVER_ADDR, SERVER_PORT);

		Listen(listenSock, MAX_CLIENT);

		printf("Server started!!\nIP: %s\nPORT: %d\n", SERVER_ADDR, SERVER_PORT);

	}
	catch (wchar_t *msgError) {
		MessageBox(serverWindow, msgError, L"Error!", MB_OK);
		return 0;
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

/**
* @function startWinsock: initiates use of the Winsock by a process.
*
**/
void initWinsock() {
	int err = WSAStartup(wVersion, &wsaData);
	if (err != 0) throw L"VERSION IS NOT SUPPORTED";
}


/*finish winshock process*/
void terminateWinsock() { WSACleanup(); }


/*
* @function createTCPSocket: create TCP socket with an option internet protocol
* @param protocol: internet protocol : TCP or UDP
*
* @return socket was created
**/
SOCKET createTCPSocket(int protocol) {
	SOCKET tcpSock = socket(protocol, SOCK_STREAM, IPPROTO_TCP);

	if (tcpSock == INVALID_SOCKET) {
		throw L"Cannot create TCP socket!\n";
	}

	return tcpSock;
}


/*
* @function associateLocalAddressWithSocket: associate a local address with a socket
* @param socket: socket needs to be connected
* @param address: address of server
* @param port: port of server
**/
void associateLocalAddressWithSocket(SOCKET &socket, char* address, int port) {
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, address, &serverAddr.sin_addr);

	if (bind(socket, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		throw L"Cannot associate a local address with server socket\n";
	}
}


/** The listen() wrapper function **/
void Listen(SOCKET &socket, int numberInQueue) {
	if (listen(socket, numberInQueue)) {
		throw L"Cannot place server socket in state LISTEN";
	}
}


void deleteAccount(Account *account) {
	closesocket(account->socket);
	int pos = account->posInListClient;
	delete account;
	if (pos >= 0 && pos < MAX_CLIENT) accounts[pos] = NULL;
}


/*
* @funcion addAccount: add an account to list client
* @param account: the account needs to add
*
* @return true - successfull, false - fail
**/
bool addAccount(Account *account) {
	bool isSuccessful = false;

	for (int i = 0; i < MAX_CLIENT; i++) {
		if (accounts[i] == NULL) {
			isSuccessful = true;
			accounts[i] = account;
			account->posInListClient = i;
			account->restMessage = "";
			break;
		}
	}

	return isSuccessful;
}


/*
* @function solveLoginRequest: solve login command of the account with the request
* @param account: the account need to solve request
* @param request: the request of the account
*
* @param: a message brings result code
**/
Message solveLoginRequest(Account *account, Message &request) {

	Message response(request.id, RESPONE, to_string(UNDENTIFY_RESULT));
	int statusAccount = getStatus(request);

	//if the account already logged 
	if (account->status == LOGGED) {
		saveLog(account, request, LOGIN_FAIL_ALREADY_LOGIN);
		response.content = to_string(LOGIN_FAIL_ALREADY_LOGIN);
		return response;
	}

	//check status of account
	switch (getStatus(request)) {
	case ACTIVE:
		account->username = request.content;
		account->status = LOGGED;
		saveLog(account, request, LOGIN_SUCCESSFUL);
		response.content = to_string(LOGIN_SUCCESSFUL);
		break;

	case LOCKED:
		saveLog(account, request, LOGIN_FAIL_LOCKED);
		response.content = to_string(LOGIN_FAIL_LOCKED);
		break;

	case NOT_EXIST:
		saveLog(account, request, LOGIN_FAIL_NOT_EXIST);
		response.content = to_string(LOGIN_FAIL_NOT_EXIST);
		break;
	}

	return response;
}


/*
* @function solvePostRequest: solve post command of the account with the request
* @param account: the account need to solve request
* @param request: the request of the account
*
* @param: a message brings result code
**/
Message solvePostRequest(Account *account, Message &request) {

	Message response(request.id, RESPONE, to_string(UNDENTIFY_RESULT));

	if (account->status == NOT_LOGGED) {
		saveLog(account, request, POST_FAIL_NOT_LOGIN);
		response.content = to_string(POST_FAIL_NOT_LOGIN);
	}
	else {
		if (saveLog(account, request, POST_SUCCESSFUL)) {
			response.content = to_string(POST_SUCCESSFUL);
		}
		else response.content = to_string(POST_FAIL);
	}

	return response;
}


/*
* @function solvePostRequest: solve logout command of the account with the request
* @param account: the account need to solve request
* @param request: the request of the account
*
* @param: a message brings result code
**/
Message solveLogoutRequest(Account *account, Message &request) {

	Message response(request.id, RESPONE, to_string(UNDENTIFY_RESULT));

	if (account->status == NOT_LOGGED) {
		response.content = to_string(LOGOUT_FAIL_NOT_LOGIN);
		saveLog(account, request, LOGOUT_FAIL_NOT_LOGIN);
	}
	else {
		account->status = NOT_LOGGED;
		response.content = to_string(LOGOUT_SUCCESSFUL);
		saveLog(account, request, LOGOUT_SUCCESSFUL);
	}

	return response;
}


/*
* @function solve: solve request of the account
* @param account: the account need to solve request
* @param request: the request of account
*
* @return: the Message struct brings result code
**/
Message solveRequest(Account *account, Message &request) {
	Message response(0, 0, "");
	switch (request.command) {
		case LOGIN:
			response = solveLoginRequest(account, request);
			break;
		case POST:
			response = solvePostRequest(account, request);
			break;
		case LOGOUT:
			response = solveLogoutRequest(account, request);
			break;
	}

	return response;
}


/*
* @fuction processAccount: process account requests
* @param account: the account needs to be processed request
**/
void processAccount(Account *account) {
	try {
		Message request;
		if (recvMessage(account->socket, account->restMessage, request) == RECV_FULL) {
			LPWSTR addr = stringToLPWSTR(getInforAccount(account));
			LPWSTR time = stringToLPWSTR(getCurrentDateTime());
			LPWSTR command = stringToLPWSTR(getCommand(request.command));
			LPWSTR content = stringToLPWSTR(request.content);
			listItem.push_back(ItemListView(addr, time, command, content));
			InsertListView(hWndListView, listItem.size() - 1);
			Message response = solveRequest(account, request);

			sendMessage(account->socket, response);
		}
	} catch (wchar_t *msgError) {
		MessageBox(serverWindow, msgError, L"Error!", MB_OK);
	}
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = windowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, NULL);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"WindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}


/*
* @function InsertListView: Insert item at index oif listItem into list view
* @param hWndListView: List View need to insert
* @param index: position in listItem
***/
void InsertListView(HWND hWndListView, int index) {

	if (index < 0 || index >= listItem.size()) return;
	
	LV_ITEM lvI; // List view item structure

	lvI.pszText = LPSTR_TEXTCALLBACK; // Sends an LVN_GETDISPINFO message.
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	lvI.stateMask = 0;
	lvI.iSubItem = 0;
	lvI.state = 0;
	lvI.iItem = index;
	lvI.lParam = (LPARAM) &listItem[index];
	ListView_InsertItem(hWndListView, &lvI);
}


//
//   FUNCTION: CreateListView(HWND)
//
//   PURPOSE: Create ListView
//
//   COMMENTS:
//
//        In this function, we create a list view and display the main program window
//
HWND CreateListView(HWND hWndParent)
{
	HWND hWndListView;      // Handle to the list view window
	RECT rcl;           // Rectangle for setting the size of the window
	int index;          // Index used in for loops
	LV_COLUMN lvC;      // List View Column structure

	// Get the size and position of the parent window
	GetClientRect(hWndParent, &rcl);
	
	hWndListView = CreateWindowEx(0L,
							WC_LISTVIEW,                // list view class
							L"",                         // no default text
							WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT |
							LVS_EDITLABELS | WS_EX_CLIENTEDGE,
							0, 0,
							rcl.right - rcl.left, rcl.bottom - rcl.top,
							hWndParent,
							(HMENU)ID_LISTVIEW,
							hInst,
							NULL);

	if (hWndListView == NULL)
		return NULL;


	lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvC.fmt = LVCFMT_LEFT;  // left align the column
	lvC.cx = 150;            // width of the column, in pixels

	// Add the columns.
	for (index = 0; index < NUM_COLUMNS; index++)
	{
		lvC.iSubItem = index;
		lvC.pszText = cols[index];

		if (ListView_InsertColumn(hWndListView, index, &lvC) == -1)
			return NULL;
	}
	
	return hWndListView;
}



//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance;
	hWnd = CreateWindow(L"WindowClass",
						L"WSAAsyncSelect TCP Server",
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT,
						0,
						CW_USEDEFAULT,
						0,
						NULL,
						NULL,
						hInstance,
						NULL);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}


//
//   FUNCTION: NotifyHandler(HWND, UINT, WPARAM, LPARAM)
//
//   PURPOSE: Notify when data change in list view
//
//   COMMENTS: Update list view
//			
//
LRESULT NotifyHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (wParam != ID_LISTVIEW) // check event of list view
		return 0L;


	LV_DISPINFO *pLvdi = (LV_DISPINFO *) lParam;

	int posItem = pLvdi->item.iItem;
	if (posItem < 0 || posItem >= listItem.size()) return 0L;

	ItemListView item = listItem[posItem]; // get data of row having event
	
	switch (pLvdi->hdr.code)
	{
	case LVN_GETDISPINFO:
		switch (pLvdi->item.iSubItem)
		{
		case 0: // col address
			pLvdi->item.pszText = item.address;
			break;

		case 1: // col time
			pLvdi->item.pszText = item.time;
			break;

		case 2: // col command
			pLvdi->item.pszText = item.command;
			break;

		case 3: // col content
			pLvdi->item.pszText = item.content;
			break;

		default:
			break;
		}
		break;
	}
	return 0L;
}



//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_SOCKET	- process the events on the sockets
//  WM_DESTROY	- post a quit message and return
//
//

LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Account *account;
	int clientAddrLen, i;

	switch (message) {
		case WM_CREATE: // create List View
			hWndListView = CreateListView(hWnd);
			if (hWndListView == NULL)
				MessageBox(NULL, L"Listview not created!", NULL, MB_OK);

			break; // break case WM_CREATE

		case WM_NOTIFY: 
			NotifyHandler(hWnd, message, wParam, lParam);//update list view

			break;// break case WM_NOTIFY

		case WM_SOCKET: // event of socket
			// if error
			if (WSAGETSELECTERROR(lParam)) {
				for (i = 0; i < MAX_CLIENT; i++)
					if (accounts[i] != NULL && accounts[i]->socket == (SOCKET)wParam) {
						deleteAccount(accounts[i]);
						continue;
					}
			}

			// if not error
			switch (WSAGETSELECTEVENT(lParam)) {
				case FD_ACCEPT: // accept event
					account = new Account;
					clientAddrLen = sizeof(account->address);

					account->socket = accept((SOCKET)wParam, (sockaddr *)&account->address, &clientAddrLen);

					if (account->socket == SOCKET_ERROR) {
						deleteAccount(account);
						MessageBox(hWnd, L"Cannot permit incoming connection", L"Error", MB_OK);
						break;
					}
					
					inet_ntop(AF_INET, &(account->address.sin_addr), account->IP, INET_ADDRSTRLEN);
					account->PORT = ntohs(account->address.sin_port);

					if (!addAccount(account)) {
						deleteAccount(account);
						MessageBox(hWnd, L"Too many clients!", L"Notice", MB_OK);
					}
				
					break;// break case FD_ACCEPT

				case FD_READ:// read event
					for (i = 0; i < MAX_CLIENT; i++) {
						if (accounts[i] != NULL && accounts[i]->socket == (SOCKET)wParam)
						{
							processAccount(accounts[i]);
							break;
						}
					}
					break;// break case FD_READ

				case FD_CLOSE://close event
					for (i = 0; i < MAX_CLIENT; i++)
						if (accounts[i] != NULL && accounts[i]->socket == (SOCKET)wParam) {
							deleteAccount(accounts[i]);
							break;
						}
					break; // break case FD_CLOSE

				}// end switch case WSAGETSELECTEVENT
		
			break;// break case WM_SOCKET

		case WM_DESTROY: // destroy event
			PostQuitMessage(0);
			shutdown(listenSock, SD_BOTH);
			closesocket(listenSock);
			WSACleanup();
			return 0;

			break;// break case WM_DESTROY

		case WM_CLOSE: // close event
			DestroyWindow(hWnd);
			shutdown(listenSock, SD_BOTH);
			closesocket(listenSock);
			WSACleanup();
			return 0;

			break;// break case WM_CLOSE
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

