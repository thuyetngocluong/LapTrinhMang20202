#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>

#include "Utilities.h"
#include "Message.h"

using namespace std;
#pragma comment(lib, "Ws2_32.lib")

#define TIME_OUT 10000

int SERVER_PORT;
char* SERVER_ADDR;
WSAData wsaData;
WORD wVersion = MAKEWORD(2, 2);
int ID = 1;


void intro() {
	printf("******************   Welcome to POST APP   ******************\n");
	printf("1. Login\n");
	printf("2. Post\n");
	printf("3. Logout\n");
	printf("4. Exit\n");
	printf("************************************************************\n");
	printf("\n");
}


/**
* @function getInformationFromCommandlineArguments : get information of server to send request from command line arguments
*/
void getInformationFromCommandlineArguments(int argc, char* argv[]) {

	if (argc <= 1) throw "ERROR! IP WAS NOT ENTERED";

	if (isValidIP(argv[1]) == false) throw "INVALID IP";

	SERVER_ADDR = argv[1];

	if (argc <= 2) throw "ERROR! PORT WAS NOT ENTERED";

	if (isNumber(argv[2]) == false) throw "ERROR! PORT MUST BE A NUMBER";

	SERVER_PORT = atoi(argv[2]);

	if (SERVER_PORT < 0 || SERVER_PORT > 65535) throw "ERROR! PORT NUMBER MUST BE IN RANGE 0 and 65535";
}

/*
* @function printfDetailedError: print error with errorCode and description error
* @param errorCode: Error Code in microsoft doc error
* @param description: detail of error
**/
void printDetailedError(int errorCode, char *description) {
	errorCode != 0 ? printf("[Error %d] %s\n", errorCode, description) : printf("%s\n", description);
}


/**
* @function initWinSock: initiates use of the Winsock by a process.
*
**/
void initWinsock() {
	int err = WSAStartup(wVersion, &wsaData);
	if (err != 0) throw "ERROR! VERSION IS NOT SUPPORTED";
}

/*finish winshock process*/
void terminateWinsock() { WSACleanup(); }


/*
* @function inputRequest: enter the request on the keyboard
* @param request: request to send
**/
Message inputRequest(string &request) {
	string cmd;
	cout << "Choose a key: ";
	getline(cin, cmd);
	int nCmd = atoi(cmd.c_str());

	if (!isNumber(cmd.c_str()) || nCmd < 1 || nCmd > 4) {
		cout << "Please choose key in range 1 and 4" << endl;
		return inputRequest(request);
	}

	int command = 0;

	switch (nCmd) {
	case 1: command = LOGIN;
		cout << "Username: ";
		getline(cin, request);
		break;
	case 2: command = POST;
		cout << "Post: ";
		getline(cin, request);
		break;
	case 3: command = LOGOUT;
		break;
	case 4: command = EXIT;
		exit(0);
		break;
	}

	ID++;

	return Message(ID, command, request);
}

/*
* @function createTCPSocket: create TCP socket with an option internet protocol and time out option
* @param protocol: internet protocol : TCP or UDP
* @param time_out: Time out of socket
*
* @return socket was created
**/
SOCKET createTCPSocket(int typeIP, int &time_out) {
	SOCKET tcpSock = socket(typeIP, SOCK_STREAM, IPPROTO_TCP);
	if (tcpSock == INVALID_SOCKET) {
		throw "Cannot create TCP socket! ";
	}

	setsockopt(tcpSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof(time_out));

	return tcpSock;
}


/**
* @function configureSocketAddress: configure for socket address with protocol, ip, port
* @param socket: socket need to configure
* @param protocol: IPv4 or IPv6 or Both
* @param ip: ip of socket
* @param port: port of socket
**/
void configureSocketAddress(sockaddr_in &socket, int protocol, char* ip, int port) {
	socket.sin_family = protocol;
	socket.sin_port = htons(port);
	inet_pton(protocol, ip, &socket.sin_addr);
}


/** the connect() wrapper funcion **/
void Connect(SOCKET &sock, sockaddr_in &server, int sizeOfServer) {
	if (connect(sock, (sockaddr*)&server, sizeOfServer)) {
		throw "Cannot connect to server";
	}
}


/**
* @function showResponse: display response on the screen
* @param message: the respone from server
***/
void showResponse(Message message) {

	if (message.id == 0 || message.command == UNKNOWN) {
		printf("Error! Unknown error\n");
		return;
	}

	if (isNumber(message.content.c_str()) == false) {
		printf("Error! Unknown error\n");
		return;
	}

	int responseCode = atoi(message.content.c_str());

	switch (responseCode) {
	case LOGIN_SUCCESSFUL:
		printf("Login successful!\n");
		break;

	case LOGIN_FAIL_LOCKED:
		printf("Login fail. the account was locked!\n");
		break;

	case LOGIN_FAIL_NOT_EXIST:
		printf("Login fail. the account was not exist!\n");
		break;

	case LOGIN_FAIL_ALREADY_LOGIN:
		printf("An account is logged in. You need to log out before logging into another account!\n");
		break;

	case POST_SUCCESSFUL:
		printf("Post message successful!\n");
		break;

	case POST_FAIL:
		printf("Post message fail!\n");
		break;

	case POST_FAIL_NOT_LOGIN:
		printf("You need to login to post messages!\n");
		break;

	case LOGOUT_SUCCESSFUL:
		printf("Logout successful!\n");
		break;

	case LOGOUT_FAIL:
		printf("Logout fail!\n");
		break;

	case LOGOUT_FAIL_NOT_LOGIN:
		printf("You have logged out!\n");
		break;

	case UNDENTIFY_RESULT:
		printf("Error! Undentify response!");
		break;

	default:
		printf("Error! Response code: %d!", responseCode);
		break;
	}
}

int main(int argc, char* argv[]) {
	SOCKET client;

	try {
		getInformationFromCommandlineArguments(argc, argv);

		// Initiate WinSock
		initWinsock();

		int time_out = TIME_OUT;
		client = createTCPSocket(AF_INET, time_out);

		sockaddr_in serverAddr;
		configureSocketAddress(serverAddr, AF_INET, SERVER_ADDR, SERVER_PORT);
		unsigned long bytes_read = -1;

		Connect(client, serverAddr, sizeof(serverAddr));

		printf("Connected to server!\n");

		intro();

		string req;

		while (1) {//while loop: send request and receiv response
			try {
				req = "";
				Message request = inputRequest(req); // input request

				sendMessage(client, request);//send request

				Message response = recvMessage(client, ID);

				showResponse(response);

			}
			catch (char *msg) {
				printDetailedError(WSAGetLastError(), msg);
			}//end catch 
		}//end while loop: send request and receiv response


		closesocket(client);

	}
	catch (char *msg) {//get error when create server
		printDetailedError(WSAGetLastError(), msg);
	}
	catch (int errorCode) {
		printDetailedError(errorCode, "Client closed!");//if string empty -> close
	}//end catch 

	terminateWinsock();
}