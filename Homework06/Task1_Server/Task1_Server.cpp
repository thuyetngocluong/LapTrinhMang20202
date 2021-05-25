// WSAEventSelectServer.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "winsock2.h"
#include <WS2tcpip.h>
#include "stdio.h"
#include "conio.h"

#include <string.h>
#include <iostream>

#include "Account.h"
#pragma comment(lib, "Ws2_32.lib")

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 6000
#define MAX_ERROR_CONSECUTIVE 5
#define SERVER_RUNNING true

void				initWinsock();
void				terminateWinsock();
SOCKET				createTCPSocket(int);
void				associateLocalAddressWithSocket(SOCKET&, char*, int);
void				Listen(SOCKET&, int);

void				deleteAndUpdate();
bool				addAccount(Account*);
void				deleteAccount(Account*);

void				doAccept(WSANETWORKEVENTS);
void				doRead(WSANETWORKEVENTS);
void				doClose(WSANETWORKEVENTS);

Message				solveLoginRequest(Account*, Message&);
Message				solvePostRequest(Account*, Message&);
Message				solveLogoutRequest(Account*, Message&);
Message				solveRequest(Account*, Message&);
void				processAccount(Account*);


WSADATA wsaData;
WORD wVersion = MAKEWORD(2, 2);//# version of winsock
Account* accounts[WSA_MAXIMUM_WAIT_EVENTS];//# List account
DWORD		nEvents = 0;
WSAEVENT	events[WSA_MAXIMUM_WAIT_EVENTS];
DWORD		index;
int consecutiveErrors = 0;
bool WAIT_EVENT_NOT_ERROR = true;


int main(int argc, char* argv[])
{
	WSANETWORKEVENTS sockEvent;

	while (SERVER_RUNNING) {
		for (int i = 1; i < WSA_MAXIMUM_WAIT_EVENTS; i++) {
			accounts[i] = NULL;
		}

		//Initiate WinSock
		initWinsock();

		//Construct LISTEN socket	
		SOCKET listenSock = createTCPSocket(AF_INET);

		accounts[0] = new Account;
		accounts[0]->socket = listenSock;
		accounts[0]->posInListClient = 0;
		events[0] = WSACreateEvent(); //create new events
		nEvents++;

		// Associate event types FD_ACCEPT and FD_CLOSE
		// with the listening socket and newEvent   
		WSAEventSelect(accounts[0]->socket, events[0], FD_ACCEPT | FD_CLOSE);

		associateLocalAddressWithSocket(listenSock, SERVER_ADDR, SERVER_PORT);

		//Listen request from client
		Listen(listenSock, 10);

		printf("Server started!!\nIP: %s\nPORT: %d\n", SERVER_ADDR, SERVER_PORT);

		WAIT_EVENT_NOT_ERROR = true;

		while (WAIT_EVENT_NOT_ERROR) {
			//wait for network events on all socket
			index = WSAWaitForMultipleEvents(nEvents, events, FALSE, WSA_INFINITE, FALSE);

			// if wait event fail
			if (index == WSA_WAIT_FAILED) {
				printf("Error %d: WSAWaitForMultipleEvents() failed\n", WSAGetLastError());
				consecutiveErrors++;
				if (consecutiveErrors > MAX_ERROR_CONSECUTIVE) {
					printf("Server Restart!\n");
					WAIT_EVENT_NOT_ERROR = false;
					break;
				};
				continue;
			} else {
				consecutiveErrors = 0;
			}


			index = index - WSA_WAIT_EVENT_0;
			WSAEnumNetworkEvents(accounts[index]->socket, events[index], &sockEvent);

			// process ACCEPT event
			if (sockEvent.lNetworkEvents & FD_ACCEPT) {
				doAccept(sockEvent);
			}

			// process READ event
			if (sockEvent.lNetworkEvents & FD_READ) {
				doRead(sockEvent);
			}

			// process CLOSE event
			if (sockEvent.lNetworkEvents & FD_CLOSE) {
				doClose(sockEvent);
			}
		}// end while WAIT_EVENT_NOT_ERROR

		closesocket(listenSock);
		terminateWinsock();
	}// end while SERVER_RUNNING

	return 0;
}

/** Do FD_ACCEPT event **/
void doAccept(WSANETWORKEVENTS sockEvent) {
	// if socketEvent error -> return
	if (sockEvent.iErrorCode[FD_ACCEPT_BIT] != 0) {
		printf("FD_ACCEPT failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
		return;
	}


	Account *account = new Account;
	int clientAddrLen = sizeof(account->address);

	// if cannot connect socket -> return;
	if ((account->socket = accept(accounts[index]->socket, (sockaddr *)&account->address, &clientAddrLen)) == SOCKET_ERROR) {
		printf("Error %d: Cannot permit incoming connection.\n", WSAGetLastError());
		delete account;
		return;
	}
	
	// if connect to socket successful 
	inet_ntop(AF_INET, &(account->address.sin_addr), account->IP, INET_ADDRSTRLEN);
	account->PORT = ntohs(account->address.sin_port);
	cout << getInforAccount(account) << " is connected" << endl;

	// if can add account to list
	if (addAccount(account)) {
		events[account->posInListClient] = WSACreateEvent();
		WSAEventSelect(account->socket, events[account->posInListClient], FD_READ | FD_CLOSE);

		nEvents++;
		cout << "Add account successful with address: " << getInforAccount(account) << endl;
	}
	else {// if can add account to list
		printf("List account full. Cannot add account\n");
		delete account;
	}

	
}


/** Do FD_READ event **/
void doRead(WSANETWORKEVENTS sockEvent) {
	if (sockEvent.iErrorCode[FD_READ_BIT] != 0) {
		printf("FD_READ failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
		deleteAndUpdate();
		return;
	}

	processAccount(accounts[index]);
}


/** Do FD_CLOSE event **/
void doClose(WSANETWORKEVENTS sockEvent) {
	if (sockEvent.iErrorCode[FD_CLOSE_BIT] != 0) {
		printf("FD_CLOSE failed with error %d\n", sockEvent.iErrorCode[FD_CLOSE_BIT]);
	}
	//Release socket and event
	deleteAndUpdate();
}


/*
* @function deleteAndUpdate: update list account and event when socket close
**/
void deleteAndUpdate() {
	// swap last account and index account
	Account *account = accounts[index];
	accounts[index] = accounts[nEvents - 1];
	accounts[nEvents - 1] = account;

	// update position in list client
	accounts[index]->posInListClient = index;
	accounts[nEvents - 1]->posInListClient = nEvents - 1;

	// swap last event and index event
	WSAEVENT temp = events[index];
	events[index] = events[nEvents - 1];
	events[nEvents - 1] = events;

	cout << "Account " << getInforAccount(accounts[nEvents - 1]) << " disconnected" << endl;

	// delete last account and event
	deleteAccount(accounts[nEvents - 1]);
	WSACloseEvent(events[nEvents - 1]);

	nEvents--;// decrease events
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
		throw "Cannot place server socket in state LISTEN\n";
	}
}


/*
* @function deleteAccount: delete account from list account
* @param account: account need to del
**/
void deleteAccount(Account *account) {
	closesocket(account->socket);
	int pos = account->posInListClient;
	delete account;
	if (pos >= 0 && pos < WSA_MAXIMUM_WAIT_EVENTS) accounts[pos] = NULL;
}


/*
* @funcion addAccount: add an account to list client
* @param account: the account needs to add
*
* @return true - successfull, false - fail
**/
bool addAccount(Account *account) {
	bool isSuccessful = false;

	for (int i = 0; i < WSA_MAXIMUM_WAIT_EVENTS; i++) {
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
	Message response(0, UNKNOWN, to_string(UNDENTIFY_RESULT));
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
			
			cout << getCurrentDateTime() << " " << getInforAccount(account)
				<< " " << commandCodeToString(request.command)
				<< " " << request.content
				<< endl;

			Message response = solveRequest(account, request);

			sendMessage(account->socket, response);

			WSAResetEvent(events[index]);
		}
	}
	catch (char* msgError) {
		deleteAndUpdate();
	}
}