#include <stdafx.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include <vector>

#include "Account.h"

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define MAX_CLIENT 10
#define SERVER_ADDR "127.0.0.1"

HANDLE mutex;

char *NAME_PROGRAM;//# name of program
int SERVER_PORT;//# Port of server
int numberMessageFail = 0;
WSADATA wsaData;
WORD wVersion = MAKEWORD(2, 2);//# version of winsock

Account* accounts[MAX_CLIENT];

/*
* @function printfDetailedError: print error with errorCode and description error
* @param errorCode: Error Code in microsoft doc error
* @param description: detail of error
**/
void printDetailedError(int errorCode, char *description) {
	if (errorCode != CLIENT_DISCONNECTED && errorCode != EXCEED_NUMBER_CONSECUTIVE_FAILED_MESSAGES) {
		printf("[Error %d] %s\n", errorCode, description);
	}
	else {
		printf("%s\n", description);
	}
}


/**
* @function getPortFromCommandLineArguments: function set a port from command line arguments
*											 throw exception if the form of the port is not correct
* @param argc: number of arguments
* @param argv: array size argc, presents value of arguments
**/
void getPortFromCommandLineArguments(int argc, char *argv[]) {
	NAME_PROGRAM = argv[0]; // first agrument is name of program

	if (argc < 2) throw "ERROR! PORT HAS NOT BEEN ENTER";

	if (isNumber(argv[1]) == false) throw "ERROR! PORT MUST BE A NUMBER";

	SERVER_PORT = atoi(argv[1]);

	if (SERVER_PORT < 0 || SERVER_PORT > 65535) throw "ERROR! PORT NUMBER MUST BE IN RANGE 0 and 65535";
}


/**
* @function startWinsock: initiates use of the Winsock by a process.
*
**/
void initWinsock() {
	int err = WSAStartup(wVersion, &wsaData);
	if (err != 0) throw "VERSION IS NOT SUPPORTED";
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
		throw "Cannot create TCP socket!\n";
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
		throw "Cannot associate a local address with server socket\n";
	}
}


/** The listen() wrapper function **/
void Listen(SOCKET &socket, int numberInQueue) {
	if (listen(socket, numberInQueue)) {
		throw "Cannot place server socket in state LISTEN";
	}
}


/*
* @funcion addToListClient: add an account to list client
* @param account: the account needs to add
*
* @return true - successfull, false - fail
**/
bool addToListClient(Account *account) {
	bool isSuccessful = false;

	WaitForSingleObject(mutex, INFINITE);//thread synchronization, wait mutex

	for (int i = 0; i < MAX_CLIENT; i++) {
		if (accounts[i] == NULL) {
			isSuccessful = true;
			accounts[i] = account;
			account->posInListClient = i;
			break;
		}
	}

	ReleaseMutex(mutex);//thread synchronization, release mutex

	return isSuccessful;
}


/*
* @function isLoggedOnAnotherDevice: check if a username already logged in or not
* @param username: username needs to check
*
* @return true if the username already logged in, false if not
**/
bool isLoggedOnAnotherDevice(string username) {
	for (int i = 0; i < MAX_CLIENT; i++) {
		if (accounts[i] != NULL 
			&& strcmp(accounts[i]->username.c_str(), username.c_str()) == 0 
			&& accounts[i]->status == LOGGED)
			return true;
	}
	return false;
}


/*
* @function solveLoginRequest: solve login command of the account with the request
* @param account: the account need to solve request
* @param request: the request of the account
*
* @param: a message brings result code
**/
Message* solveLoginRequest(Account *account, Message *request) {

	Message *response = new Message(request->id, RESPONE, SIZE_RESPONSE_MESSAGE, to_string(UNDENTIFY_RESULT));
	int statusAccount = getStatus(*request);

	//if the account already logged 
	if (account->status == LOGGED) {
		saveLog(account, request, LOGIN_FAIL_ALREADY_LOGIN);
		response->content = to_string(LOGIN_FAIL_ALREADY_LOGIN);
		return response;
	}

	//check status of account
	switch (getStatus(*request)) {
		case ACTIVE:
			//if the account is active and the account already logged in on another device
			if (isLoggedOnAnotherDevice(request->content)) { 
				saveLog(account, request, LOGIN_FAIL_ANOTHER_DEVICE);
				response->content = to_string(LOGIN_FAIL_ANOTHER_DEVICE);
				break;
			}

			// else
			account->username = request->content;
			account->status = LOGGED;
			saveLog(account, request, LOGIN_SUCCESSFUL);
			response->content = to_string(LOGIN_SUCCESSFUL);
			break;

		case LOCKED:
			saveLog(account, request, LOGIN_FAIL_LOCKED);
			response->content = to_string(LOGIN_FAIL_LOCKED);
			break;

		case NOT_EXIST:
			saveLog(account, request, LOGIN_FAIL_NOT_EXIST);
			response->content = to_string(LOGIN_FAIL_NOT_EXIST);
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
Message* solvePostRequest(Account *account, Message *request) {

	Message *response = new Message(request->id, RESPONE, SIZE_RESPONSE_MESSAGE, to_string(UNDENTIFY_RESULT));

	if (account->status == NOT_LOGGED) {
		saveLog(account, request, POST_FAIL_NOT_LOGIN);
		response->content = to_string(POST_FAIL_NOT_LOGIN);
	} else
	if (saveLog(account, request, POST_SUCCESSFUL)) {
		response->content = to_string(POST_SUCCESSFUL);
	} else response->content = to_string(POST_FAIL);

	return response;
}


/*
* @function solvePostRequest: solve logout command of the account with the request
* @param account: the account need to solve request
* @param request: the request of the account
*
* @param: a message brings result code
**/
Message* solveLogoutRequest(Account *account, Message *request) {

	Message *response = new Message(request->id, RESPONE, SIZE_RESPONSE_MESSAGE, to_string(UNDENTIFY_RESULT));

	if (account->status == NOT_LOGGED) {
		response->content = to_string(LOGOUT_FAIL_NOT_LOGIN);
		saveLog(account, request, LOGOUT_FAIL_NOT_LOGIN);
	}
	else {
		account->status = NOT_LOGGED;
		response->content = to_string(LOGOUT_SUCCESSFUL);
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
Message* solve(Account *account, Message *request) {
	Message *response;
	switch (request->command) {
		case LOGIN:
			response = solveLoginRequest(account, request);
			break;
		case POST:
			response = solvePostRequest(account, request);
			break;
		case LOGOUT:
			response = solveLogoutRequest(account, request);
			break;
		default: 
			response = new Message();
			break;
	}

	return response;
}


/*
* @function echoThread: the thread performs a client
***/
unsigned __stdcall echoThread(void *param) {

	int numberError = 0;
	Account *account = (Account*) param;

	//Show client information
	inet_ntop(AF_INET, &(account->address.sin_addr), account->IP, INET_ADDRSTRLEN);
	account->PORT = ntohs(account->address.sin_port);
	printf("Client [%s:%d] is connected\n", account->IP, account->PORT);

	while (1) {//while handle a client
		try {
			Message *request = recvMessage(account->socket, numberError);

			cout << getCurrentDateTime() << " " << getInforAccount(account) 
										 << " " << commandCodeToString(request->command) 
										 << " "<< request->content 
										 << endl;

			WaitForSingleObject(mutex, INFINITE);//thread synchronization, wait mutex
			Message *response = solve(account, request);
			ReleaseMutex(mutex);//thread synchronization, release mutex

			sendMessage(account->socket, response, numberError);

			delete request;
			delete response;

		}
		catch (char* msg) {//Service next REQUEST when an error, throw an STRING , occurs
			printDetailedError(WSAGetLastError(), msg);
		} // end catch char*
		catch (int errorCode) {//End CLIENT when an error,throw an INTEGER , occurs
			if (errorCode == CLIENT_DISCONNECTED) {
				printDetailedError(errorCode, "CLIENT DISCONNECTED");
			} 
			else if (errorCode == EXCEED_NUMBER_CONSECUTIVE_FAILED_MESSAGES) {
				printDetailedError(errorCode, "Exceed number of consecutive failed message");
			}
			else printf("Error %d", errorCode);
			break;
		}//end catch int

	}//end while handle a client

	printf("Client [%s:%d] disconnected\n", account->IP, account->PORT);

	closesocket(account->socket);

	WaitForSingleObject(mutex, INFINITE);//thread synchronization, wait mutex
	int pos = account->posInListClient;
	delete accounts[pos];
	accounts[pos] = NULL;
	ReleaseMutex(mutex);//thread synchronization, release mutex

	return 0;
}

int main(int argc, char *argv[]) {
	SOCKET listenSock;

	memset(accounts, NULL, sizeof(accounts));
	mutex = CreateMutex(0, 0, 0);

	try {
		getPortFromCommandLineArguments(argc, argv);

		initWinsock();

		listenSock = createTCPSocket(AF_INET);

		associateLocalAddressWithSocket(listenSock, SERVER_ADDR, SERVER_PORT);

		Listen(listenSock, MAX_CLIENT);

		printf("Server started!!\nIP: %s\nPORT: %d\n", SERVER_ADDR, SERVER_PORT);

		while (1) {//while handle socket in LISTEN queue 

			Account *account = new Account;
			int clientAddrLen = sizeof(account->address);
			// permit an incoming connection attempt on a socket
			account->socket = accept(listenSock, (sockaddr*) &account->address, &clientAddrLen);
			if (account->socket == SOCKET_ERROR) {
				delete account;
				printf("[Error %d] Cannot permit incoming connection", WSAGetLastError());
				continue;
			}		
		
			//if server is overloaded
			if (addToListClient(account) == false) {
				closesocket(account->socket);
				delete account;
				continue;
			}

			_beginthreadex(0, 0, echoThread, (void *)account, 0, 0); //start thread

		}//end while handle socket in LISTEN queue 

		closesocket(listenSock);
	}
	catch (char* msg) {
		printDetailedError(WSAGetLastError(), msg);
	}//end try-catch create server

	terminateWinsock();

	CloseHandle(mutex);
}