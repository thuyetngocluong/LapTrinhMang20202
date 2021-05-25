#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <fstream>
#include <streambuf>

#include "Utilities.h"
#include "Message.h"

using namespace std;
#pragma comment(lib, "Ws2_32.lib")

#define TIME_OUT 300000

int SERVER_PORT;
char* SERVER_ADDR;
WSAData wsaData;
WORD wVersion = MAKEWORD(2, 2);
int ID = 0;
int command;
string dir;
int key;
bool SEND_FINISH = false;

struct File {
	string name_file;
	int length;
	char* payload;
};


void					input();
void					intro();
bool					file_exist(string&);
File					readFile(string);

void					getInformationFromCommandlineArguments(int, char**);

void					printDetailedError(int, char*);

void					initWinsock();
void					terminateWinsock();
SOCKET					createTCPSocket(int, int&);
void					configureSocketAddress(sockaddr_in&, int, char*, int);
void					Connect(SOCKET&, sockaddr_in&, int);



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

		Connect(client, serverAddr, sizeof(serverAddr));

		printf("Connected to server!\n");

		intro();

		while (1) {
			input();
			if (file_exist(dir) == false) {
				printf("File not exist\n");
				continue;
			}

			sendFile(client, dir, command, key);
			
			printf("Wait server encrypt/decrypt file\n");

			if (command == OP_ENCRYPTION) {
				receiveFile(client, dir + ".enc", command, key);
			}
			else if (command == OP_DECRYPTION) {
				receiveFile(client, dir + ".dec", command, key);
			}

		}

	}
	catch (char* msg) {
		printf("%s", msg);
	}
	closesocket(client);
	terminateWinsock();
}

bool file_exist(string &dir) {
	fstream ifs;
	ifs.open(dir, ios::in | ios::binary);
	if (ifs.is_open()) {
		ifs.close();
		return true;
	}
	return false;
}


void input() {
	printf("Choose command (1/2): ");
	cin >> command;
	
	if (command != 1 && command != 2) {
		printf("Error command");
		input();
	}
	if (command == 1) command = OP_ENCRYPTION;
	if (command == 2) command = OP_DECRYPTION;

	printf("Enter the link to directory: ");
	getchar();
	getline(cin, dir);
	printf("Enter the key: ");
	cin >> key;
}


void intro() {
	printf("************   Welcome to Encrypt/Decrypt APP   ************\n");
	printf("1. Encrypt \n");
	printf("2. Decrypt \n");
	printf("************************************************************\n");
	printf("\n");
}


/**
* @function getInformationFromCommandlineArguments : get information of server to send request from command line arguments
*/
void getInformationFromCommandlineArguments(int argc, char** argv) {

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
