#include <stdafx.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include <vector>
#include <unordered_map>

#include "Account.h"

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_ADDR "127.0.0.1"

char *NAME_PROGRAM;//# name of program
int SERVER_PORT;//# Port of server
WSADATA wsaData;
WORD wVersion = MAKEWORD(2, 2);//# version of winsock

/*
* @struct FD_SET_MANAGEMENT: manage accounts in a fd_set
**/
typedef struct FD_SET_MANAGEMENT {
	fd_set *fds;
	Account* accounts[FD_SETSIZE]; //# List accounts - fds manages
	int numAccount = 0;//# Number of accounts is managing

	FD_SET_MANAGEMENT(fd_set *_fds) {
		this->fds = _fds;
		memset(this->accounts, NULL, sizeof(this->accounts));
	}

	/*
	* @function addAcc: check if can add an account into vector
	* @param acc: brings information of account
	*
	* @return: true if can add and add account in to fd_set, false if not
	***/
	bool addAcc(Account *acc) {
		if (this->numAccount == FD_SETSIZE) return false;

		for (int i = 0; i < FD_SETSIZE; i++)
			if (this->accounts[i] == NULL) {
				this->accounts[i] = acc;
				this->numAccount++;
				FD_SET(acc->socket, this->fds);
				acc->posInListClient = i;
				return true;
			}

		return false;
	}


	/*
	* @function deleteAcc: delete an account from fd_set
	* @param acc: brings information of account
	*
	***/
	void deleteAcc(Account *acc) {
		int pos = acc->posInListClient;
		if (pos < 0 || pos >= FD_SETSIZE) return;

		FD_CLR(acc->socket, this->fds);
		closesocket(acc->socket);
		numAccount--;
		delete acc;
		this->accounts[pos] = NULL;
	}


	/*
	* @function deleteAllAcc: delete all accounts from fd_set
	* @param acc: brings information of account
	*
	***/
	void deleteAllAcc() {
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (this->accounts[i] != NULL) {
				FD_CLR(this->accounts[i]->socket, fds);
				closesocket(this->accounts[i]->socket);
				delete(this->accounts[i]);
				this->accounts[i] = NULL;
			}
		}
		this->numAccount = 0;
	}
} fdsManagement;


vector<pair<HANDLE, fdsManagement*>> v;//#Vector pair of the Thread and the FD_SET_MANAGEMENT which the thread manages 

unordered_map<int, HANDLE> mapMutex;//# map of mutex with each id thread


/*
* @function updateVector: update vector
***/
void updateVector() {
	int i = 0;
	fdsManagement *fm;
	while (i < v.size()) {
		int id = GetThreadId(v[i].first);

		WaitForSingleObject(mapMutex[id], INFINITE);

		fm = v[i].second;
		if (fm->numAccount == 0) { // if numAccount == 0, not need to manage, delete fd_set_management
			delete fm->fds;//free fd_set
			CloseHandle(v[i].first);//terminate thread
			v.erase(v.begin() + i);// erease from vector

			ReleaseMutex(mapMutex[id]);
			
			CloseHandle(mapMutex[id]);//close mutex corresponding to thread

			mapMutex.erase(id);// delete mutex
		}
		else {
			ReleaseMutex(mapMutex[id]);
			i++;
		}
	}
}


/*
* @function addAcc: check if can add an account into vector
* @param acc: brings information of account
*
* @return: true if can add and add account in to vector, false if not
***/
bool addAcc(Account *acc) {
	fdsManagement *fm;
	updateVector();
	for (int i = 0; i < v.size(); i++) {
		fm = v[i].second;
		if (fm->addAcc(acc)) return true;
	}
	return false;
}

/*
* @function printfDetailedError: print error with errorCode and description error
* @param errorCode: Error Code in microsoft doc error
* @param description: detail of error
**/
void printDetailedError(int errorCode, char *description) {
	if (errorCode != CLIENT_DISCONNECTED) {
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
	}
	else
		if (saveLog(account, request, POST_SUCCESSFUL)) {
			response->content = to_string(POST_SUCCESSFUL);
		}
		else response->content = to_string(POST_FAIL);

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
* @fuction processAccount: process account requests
* @param fm: FD_SET_MANAGEMENT manages account
* @param account: the account needs to be processed request
**/
void processAccount(fdsManagement *fm, Account *account) {
	try {
		Message *request = recvMessage(account->socket);

		cout << getCurrentDateTime() << " " << getInforAccount(account)
			<< " " << commandCodeToString(request->command)
			<< " " << request->content
			<< endl;

		Message *response = solve(account, request);

		sendMessage(account->socket, response);

		delete request;
		delete response;

	}
	catch (char* msg) {
		printDetailedError(WSAGetLastError(), msg);
		fm->deleteAcc(account);
	}
	catch (int errorCode) {
		if (errorCode == CLIENT_DISCONNECTED) {
			printDetailedError(errorCode, "CLIENT DISCONNECTED");
		}
		else printf("Error %d", errorCode);
		fm->deleteAcc(account);
	}
}



/*
* @function threadProcessFM: The thread executes all of the account's requests in the management set
**/
unsigned __stdcall threadProcessFM(void* param) {

	WaitForSingleObject(mapMutex[GetCurrentThreadId()], INFINITE);
	fdsManagement* fm = (fdsManagement*) param; 
	fd_set readfds;
	fd_set *initfds = fm->fds;
	Account  **accounts = fm->accounts;
	ReleaseMutex(mapMutex[GetCurrentThreadId()]);

	// set time update (1 update/sec)
	struct timeval timeUpdate;
	timeUpdate.tv_sec = 1;
	timeUpdate.tv_usec = 0;

	while (1) {
		WaitForSingleObject(mapMutex[GetCurrentThreadId()], INFINITE);
		readfds = *initfds;

		int nEvent = select(0, &readfds, 0, 0, &timeUpdate);

		if (nEvent < 0) {
			printf("Error! Cannot poll sockets: %d\n", WSAGetLastError());
			break;
		} 
		else if (nEvent > 0) {
			for (int i = 0; i < FD_SETSIZE; i++) {
				Account* account = accounts[i];

				if (account == NULL) continue;

				if (FD_ISSET(account->socket, &readfds)) {
					processAccount(fm, account);
				}

				if (--nEvent <= 0) continue;
			}
		}// end if process events

		ReleaseMutex(mapMutex[GetCurrentThreadId()]);
	}// end while

	WaitForSingleObject(mapMutex[GetCurrentThreadId()], INFINITE);
	fm->deleteAllAcc();
	ReleaseMutex(mapMutex[GetCurrentThreadId()]);
	return 0;
}



/** Main Function **/
int main(int argc, char *argv[]) {
	fd_set listenfds, readListenfds;

	SOCKET listenSock;

	try {
		getPortFromCommandLineArguments(argc, argv);

		initWinsock();
		
		listenSock = createTCPSocket(AF_INET);

		associateLocalAddressWithSocket(listenSock, SERVER_ADDR, SERVER_PORT);

		Listen(listenSock, 10);

		printf("Server started!!\nIP: %s\nPORT: %d\n", SERVER_ADDR, SERVER_PORT);

		FD_ZERO(&listenfds);
		FD_SET(listenSock, &listenfds);

		while (1) {
			readListenfds = listenfds;

			int nEventListen = select(0, &readListenfds, 0, 0, 0);

			if (nEventListen < 0) {
				printf("Error! Cannot poll sockets: %d\n", WSAGetLastError());
				continue;
			}

			if (FD_ISSET(listenSock, &listenfds)) {
				Account *account = new Account;

				int clientAddrLen = sizeof(account->address);

				// permit an incoming connection attempt on a socket
				account->socket = accept(listenSock, (sockaddr*)&account->address, &clientAddrLen);

				if (account->socket == SOCKET_ERROR) {
					delete account;
					printf("[Error %d] Cannot permit incoming connection", WSAGetLastError());
					continue;
				}

				// save information into account
				inet_ntop(AF_INET, &(account->address.sin_addr), account->IP, INET_ADDRSTRLEN);
				account->PORT = ntohs(account->address.sin_port);

				// show connection
				printf("Client [%s:%d] is connected\n", account->IP, account->PORT);

				// if cannot add an account, create new thread and fd_set_management
				if (!addAcc(account)) {
					fd_set *fds = new fd_set;// create fd_set
					FD_ZERO(fds);

					fdsManagement *fm = new fdsManagement(fds);//create fd_set_management with fd_set

					fm->addAcc(account);// add account into fd_set

					HANDLE handle = (HANDLE) _beginthreadex(0, 0, threadProcessFM, (void*)fm, 0, 0);// new thread

					v.push_back({ handle, fm });// add into vector

					mapMutex[GetThreadId(handle)] = CreateMutex(0, 0, 0);//create mutex
				}// end if addAcc

			}// endif process event in listen socket
		
		}// end while

		closesocket(listenSock);
	} catch (char* msg) {
		printDetailedError(WSAGetLastError(), msg);
	}//end try-catch create server

	terminateWinsock();
}