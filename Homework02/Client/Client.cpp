#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>

#include "HW02.h"

using namespace std;
#pragma comment(lib, "Ws2_32.lib")

#define STRING_EMPTY 0

int SERVER_PORT;
char* SERVER_ADDR;
WSAData wsaData;
WORD wVersion = MAKEWORD(2, 2);
int ID = 0;


/*
* @function isValidIP: check if a string is a valid IP or not
* @param IP: a string need to check
**/
bool isValidIP(char *IP) {
	in_addr addr;
	return inet_pton(AF_INET, IP, &addr.s_addr) == 1;
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
Message* inputRequest(string &request) {
	fflush(stdin);
	getline(cin, request);

	if (request.length() == 0) throw STRING_EMPTY;

	ID++;

	char *content = new char[request.length() + 1];
	strcpy_s(content, request.length() + 1, request.c_str());

	return new Message(ID, static_cast<int>(request.length()), content);
}


/*
* @function showResponse: show response receive from server
* @param response: response received
**/
void showResponse(char *response) {
	char *tmp = response + 1;
	if (response[0] == '+') printf("******** SUCCESSFUL ********\n");
	if (response[0] == '-') printf("*********** FAIL ***********\n");
	printf("%s\n", tmp);
	printf("****************************\n\n");
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

	setsockopt(tcpSock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &time_out, sizeof(time_out));

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


/*
* @function connectToServer:  permit an incoming connection attempt on a socket.
* @param sock: socket was connected
* @param server: address of server was connected
**/
void connectToServer(SOCKET &sock, sockaddr_in &server) {
	if (connect(sock, (sockaddr*) &server, sizeof(server))) {
		throw "Cannot connect to server";
	}
}


/*
* @function send: send a message
* @param inSock: socket where message send to
* @param message: message need to be sent
* 
**/
void sendMsg(SOCKET &inSock, Message *message) {

	char *msgString = message->toString();//convert struct Message to char array

	int lenLeft = NUMBER_BYTES_ID + NUMBER_BYTES_LEN_MSG + static_cast<int>(strlen(message->content));

	char *tmp = msgString;

	while (lenLeft > 0) {
		int ret = send(inSock, tmp, lenLeft, 0);

		if (ret == SOCKET_ERROR) {
			delete[] msgString;
			throw "Cannot send data\n";
		}

		lenLeft -= ret;
		tmp += ret;
	}

	delete[] msgString;
}

/*
* @function recv: receive a message
* @param inSock: socket, where message receives from
* @param lenMessage :length of received message
*
* @return: received message
**/
char* recv(SOCKET &inSock, int lenMessage) {
	char *msg = new char[lenMessage + 1];
	char *tmp = msg;
	int lenLeft = lenMessage;

	while (lenLeft > 0) {
		int ret = recv(inSock, tmp, lenLeft, 0);
		
		if (ret == SOCKET_ERROR) {

			delete[] msg;

			if (WSAGetLastError() == WSAETIMEDOUT) {
				throw "Time-out";
			}
			else {
				throw "Cannot receive message.";
			}
		}

		lenLeft -= ret;
		tmp += ret;
	}

	msg[lenMessage] = '\0';

	return msg;
}


/*
* @function recvMsg: receive message
* @param inSock: socket, where message receives from
*
* @return: poiter of received Message struct
**/
Message* recvMsg(SOCKET &inSock) {
	while (1) {
		char *idBytes = recv(inSock, NUMBER_BYTES_ID);
		int id = bytesToInt(idBytes);

		char *contentLengthBytes = recv(inSock, NUMBER_BYTES_LEN_MSG);
		int contentLength = bytesToInt(contentLengthBytes);

		char *content = recv(inSock, contentLength);

		delete[] idBytes;
		delete[] contentLengthBytes;

		if (id == ID) return (Message*) new Message(id, contentLength, content);//Receive only the id that matches the session
	}
}


int main(int argc, char* argv[]) {
	SOCKET client;
	try {
		getInformationFromCommandlineArguments(argc, argv);

		// Initiate WinSock
		initWinsock();

		int time_out = 10000;
		client = createTCPSocket(AF_INET, time_out);

		sockaddr_in serverAddr;
		configureSocketAddress(serverAddr, AF_INET, SERVER_ADDR, SERVER_PORT);

		connectToServer(client, serverAddr);

		printf("Connected to server!");

		string req;

		while (1) {//while loop: send request and receiv response
			try {
				printf("\nSend to server: ");
				Message *request = inputRequest(req); // input request

				sendMsg(client, request);//send request

				Message *response = recvMsg(client);//receive response

				showResponse(response->content);

				delete[] request->content;
				delete[] response->content;

				delete request;
				delete response;
			}
			catch (char *msg) {
				printDetailedError(WSAGetLastError(), msg);
			}//end catch 
		}//end while loop: send request and receiv response


		closesocket(client);

	} catch (char *msg) {//get error when create server
		printDetailedError(WSAGetLastError(), msg);
	} catch (int errorCode) {
		if (errorCode == STRING_EMPTY)
			closesocket(client);
			printDetailedError(errorCode, "Client closed!");//if string empty -> close
	}//end catch 

	terminateWinsock();
}