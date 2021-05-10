#include <stdafx.h>
#include <stdio.h>
#include <string.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "HW02.h"

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define MAX_CLIENT 10
#define SERVER_ADDR "127.0.0.1"

#define MAX_CONSECUTIVE_FAILED_MESSAGES 5

#define CLIENT_DISCONNECTED 0
#define EXCEED_NUMBER_CONSECUTIVE_FAILED_MESSAGES -1

char *NAME_PROGRAM;//# name of program
int SERVER_PORT;//# Port of server
int countConsecutiveFailedMessages = 0;//#number of consecutive failed messages
WSADATA wsaData;
WORD wVersion = MAKEWORD(2, 2);//# version of winsock

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


/*
* @function placeSocketInListenState: place socket in LISTEN state
* @param socket: socket need to be placed
* @param numberInQueue: number of socket in LISTEN queue
**/
void placeSocketInListenState(SOCKET &socket, int numberInQueue) {
	if (listen(socket, numberInQueue)) {
		throw "Cannot place server socket in state LISTEN";
	}
}

/*
* @function send: send a message
* @param inSock: socket where message send to
* @param message: message need to be sent
*
**/
void sendMsg(SOCKET &inSock, Message* msg) {

	char *msgString = msg->toString();//convert struct Message to char array 

	int lenLeft = NUMBER_BYTES_ID + NUMBER_BYTES_LEN_MSG + static_cast<int>(strlen(msg->content));

	char *tmp = msgString;

	while (lenLeft > 0) {
		int ret = send(inSock, tmp, lenLeft, 0);

		if (ret == SOCKET_ERROR) {
			delete[] msgString;

			countConsecutiveFailedMessages++;//increase when error

			if (countConsecutiveFailedMessages >= MAX_CONSECUTIVE_FAILED_MESSAGES) {
				throw EXCEED_NUMBER_CONSECUTIVE_FAILED_MESSAGES;
			}

			throw "Cannot send data\n";
		}

		lenLeft -= ret;
		tmp += ret;
	}

	delete[] msgString;
	countConsecutiveFailedMessages = 0;//reset to 0 when send successfully
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
			delete msg;
			countConsecutiveFailedMessages++;// increase when error 

			if (countConsecutiveFailedMessages >= MAX_CONSECUTIVE_FAILED_MESSAGES) {
				throw EXCEED_NUMBER_CONSECUTIVE_FAILED_MESSAGES;
			}

			throw "Cannot receive data!\n";
		}

		if (ret == CLIENT_DISCONNECTED) {
			delete msg;
			throw CLIENT_DISCONNECTED;
		}

		lenLeft -= ret;
		tmp += ret;
	}
	
	countConsecutiveFailedMessages = 0;//reset to 0 when receive successfully

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
	char *idBytes = recv(inSock, NUMBER_BYTES_ID);
	int id = bytesToInt(idBytes);

	char *contentLengthBytes = recv(inSock, NUMBER_BYTES_LEN_MSG);
	int contentLength = bytesToInt(contentLengthBytes);

	char *content = recv(inSock, contentLength);

	delete[] idBytes;
	delete[] contentLengthBytes;

	return (Message*) new Message(id, contentLength, content);
	 
}


/*
* @function solveRequest : solve request - divide string into alphabet letters and digits
* @param msg: msg need to be divided
*
* @return: string include two strings: has only alphabet letters and has only digits 
		   Error if string has character which is not a letter or a digit
**/
char* solveRequest(char* msg) {

	int lenMsg = static_cast<int>(strlen(msg));//#length of message

	char *resultDigit = new char[lenMsg + 1];//#array save all digits of message
	char *resultAlpha = new char[lenMsg + 1];//#array save all alphabet letter of message
	char *result;

	int countDigit = 0, countAlpha = 0;

	for (int i = 0; i < lenMsg; i++) {
		if (isDigit(msg[i]) == true) {//Save digit into resultDigit array
			resultDigit[countDigit] = msg[i];
			countDigit++;
		}
		else if (isAlpha(msg[i]) == true) {//save letter into resultAlpha array
			resultAlpha[countAlpha] = msg[i];
			countAlpha++;
		}
		else {//if character is not a digit or an alphabet letter -> ERROR
			countDigit = -1;
			countAlpha = -1;
			break;
		}
	}

	//if message have a character, which is not a digit or a alphabet letter
	if (countDigit == -1 && countAlpha == -1) {
		result = new char[10];
		strcpy_s(result, 10, "-Error");//Fail solution
	} 
	else {// if message can be divided
		result = new char[lenMsg + 4];
		int index = 0;
		result[index] = '+';//Successful Solution

		//copy arrayAlphat to result
		for (int i = 0; i < countAlpha; i++) {
			index++;
			result[index] = resultAlpha[i];
		}

		if (countAlpha != 0 && countDigit != 0) {
			index++;
			result[index] = '\n';
		}

		//Copy resultDigit to result
		for (int i = 0; i < countDigit; i++) {
			index++;
			result[index] = resultDigit[i];
		}
		
		index++;
		result[index] = '\0';
	}

	delete resultAlpha;
	delete resultDigit;

	return result;
}


int main(int argc, char *argv[]) {
	SOCKET listenSock, connSock;
		
	try {
		getPortFromCommandLineArguments(argc, argv);

		initWinsock();

		listenSock = createTCPSocket(AF_INET);

		associateLocalAddressWithSocket(listenSock, SERVER_ADDR, SERVER_PORT);

		placeSocketInListenState(listenSock, MAX_CLIENT);

		printf("Server started!!\nIP: %s\nPORT: %d\n", SERVER_ADDR, SERVER_PORT);

		sockaddr_in clientAddr;
		char clientIP[INET_ADDRSTRLEN];
		int clientAddrLen = sizeof(clientAddr), clientPort;

		while (1) {//while handle socket in LISTEN queue 

			// permit an incoming connection attempt on a socket
			connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
			if (connSock == SOCKET_ERROR) {
				printf("[Error %d] Cannot permit incoming connection", WSAGetLastError());
				continue;
			}
			
			//Show client information
			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
			clientPort = ntohs(clientAddr.sin_port);
			printf("Accept incoming connection from [%s:%d]\n", clientIP, clientPort);

			countConsecutiveFailedMessages = 0;//Reset to 0 when handle new client

			while (1) {//while handle a client
				try {
					Message *request = recvMsg(connSock);//receive request
					
					printf("Receive from client [%s:%d]: %s\n", clientIP, clientPort, request->content);

					char *contentResponse = solveRequest(request->content);//solve the request

					Message *response = new Message(request->id, static_cast<int>(strlen(contentResponse)), contentResponse);
					
					sendMsg(connSock, response);//send response

					
					delete[] request->content;
					delete[] response->content;
		
					delete request;
					delete response;
		
				}
				catch (char* msg) {//Serve next REQUEST when an error, throw an STRING , occurs
					printDetailedError(WSAGetLastError(), msg);
				} // end catch char*
				catch (int errorCode) {//Serve next CLIENT when an error,throw an INTEGER , occurs
					if (errorCode == CLIENT_DISCONNECTED) {
						printDetailedError(errorCode, "CLIENT DISCONNECTED");
						break;
					} 
					if (errorCode == EXCEED_NUMBER_CONSECUTIVE_FAILED_MESSAGES) {
						printDetailedError(errorCode, "Exceed number of consecutive failed message");
						break;
					}
				}//end catch int

			}//end while handle a client

			closesocket(connSock);

		}//end while handle socket in LISTEN queue 

		closesocket(listenSock);
	}
	catch (char* msg) {
		printDetailedError(WSAGetLastError(), msg);
	}//end try-catch create server

	terminateWinsock();
}