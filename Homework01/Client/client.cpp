#include <stdafx.h>
#include <stdio.h>
#include <WinSock2.h>
#include <winsock.h>
#include <WS2tcpip.h>
#include <iostream>

using namespace std;
#pragma comment(lib, "Ws2_32.lib")

#define BUFF_SIZE 2048
#define TIME_OUT "10000"

char* SERVER_ADDR;//#Address of server
int SERVER_PORT;//#Port of server
WSADATA wsaData;
WORD wVersion = MAKEWORD(2, 2);

bool isNumber(char *s) {
	int i = 0;
	while (s[i] != '\0') {
		if (isdigit(s[i]) == false) return false;
		i++;
	}

	return true;
}


bool isValidIP(char *IP) {
	in_addr addr;
	return inet_pton(AF_INET, IP, &addr.s_addr) == 1;
}


/**
* @function getInformationFromCommandlineArguments : get information of server to send request from command line arguments
*/
void getInformationFromCommandlineArguments(int argc, char* argv[]) {

	if (isValidIP(argv[1]) == false) throw "INVALID IP";

	SERVER_ADDR = argv[1];

	if (isNumber(argv[2]) == false) throw "ERROR! PORT MUST BE A NUMBER";

	SERVER_PORT = atoi(argv[2]);

	if (SERVER_PORT < 0 || SERVER_PORT > 65535) throw "ERROR! PORT NUMBER MUST BE IN RANGE 0 and 65535";
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
void inputRequest(char request[]) {
	fflush(stdin);
	gets_s(request, BUFF_SIZE);
}


/*
* @function showResponse: show response receive from server
* @param response: response received
**/
void showResponse(char *response) {
	char *tmp = response + 1;
	if (response[0] == '+') printf("********SUCCESSFUL RESOLUTION********\n");
	if (response[0] == '-') printf("********FAIL RESOLUTION********\n");
	printf("%s\n", tmp);
	printf("****************\n\n");
}


/**
* @function createUDPClient: create a UDP client with internet_protocol and time out
* @param protocol: IPv4 or IPv6 or Both
* @param time_out: time_out receive respone of client
* @return socket present information of client
**/
SOCKET createUDPClient(short internet_protocol, const char* time_out) {
	SOCKET client = socket(internet_protocol, SOCK_DGRAM, IPPROTO_UDP);

	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, time_out, sizeof(int));

	return client;
}


/**
* @function configureSocketAddress: configure for server with protocol, ip, port
* @param server: server need to configure
* @param protocol: IPv4 or IPv6 or Both
* @param ip: ip of server
* @param port: port of server
**/
void configureSocketAddress(sockaddr_in &server, int protocol, char* ip, int port) {
	server.sin_family = protocol;
	server.sin_port = htons(port);
	inet_pton(protocol, ip, &server.sin_addr);
}


/**
* @function recvMsg : receive a message from server
* @param inSocket : indicate socket which client receive from
* @param fromServer: indicate server which client receive from
*
* @return: a receive message
**/
char* recvMsg(SOCKET &inSocket, sockaddr_in &fromServer) {
	char *recvMsg = new char[BUFF_SIZE];
	int fromServerLen = sizeof(fromServer);
	int recvCode = recvfrom(inSocket, recvMsg, BUFF_SIZE, 0, (sockaddr*)&fromServer, &fromServerLen);

	if (recvCode == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAETIMEDOUT) {
			strcpy_s(recvMsg, BUFF_SIZE, "Time-out!");
		}
		else {
			strcpy_s(recvMsg, BUFF_SIZE, "Error! Cannot receive message.");
		}
	}
	else {
		recvMsg[recvCode] = '\0';
	}
	return recvMsg;
}


/**
* @function sendMsg : send a message from client
* @param inSocket : indicate socket which client send to
* @param toServer: indicate server which client send to
* @param msg: message client send
*
**/
void sendMsg(SOCKET &inSocket, sockaddr_in &toServer, char *msg) {
	int sendCode = sendto(inSocket, msg, strlen(msg), 0, (sockaddr*)&toServer, sizeof(toServer));
	if (sendCode == SOCKET_ERROR) {
		printf("Error send: %d\n", WSAGetLastError());
	}
}


int main(int argc, char *argv[]) {
	try {

		getInformationFromCommandlineArguments(argc, argv);

		initWinsock();

		printf("CLIENT STARTED!\n");

		SOCKET client = createUDPClient(AF_INET, TIME_OUT);

		sockaddr_in serverAddr;//#address of server
		configureSocketAddress(serverAddr, AF_INET, SERVER_ADDR, SERVER_PORT);

		char request[BUFF_SIZE];

		while (1) {
			printf("IP or Domain name: ");
			
			inputRequest(request);
			
			if (strcmp(request, "") == 0) break;//if empty string -> out

			// send request to server
			sendMsg(client, serverAddr, request);

			// receive msg from server
			char *response = recvMsg(client, serverAddr);

			showResponse(response);

			delete response;
		}

		closesocket(client);

		terminateWinsock();
	}
	catch (const char *msg) {
		cerr << msg;
	}
}