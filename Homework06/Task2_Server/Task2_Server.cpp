// WSAEventSelectServer.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "winsock2.h"
#include <WS2tcpip.h>
#include "stdio.h"
#include "conio.h"
#include <string>
#include <process.h>
#include <iostream>

#include "Utilities.h"
#include "Message.h"

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_ADDR "127.0.0.1"

#define SERVER_RUNNING true

void getPortFromCommandLineArguments(int, char**);

void				initWinsock();
void				terminateWinsock();
SOCKET				createTCPSocket(int);
void				associateLocalAddressWithSocket(SOCKET&, char*, int);
void				Listen(SOCKET&, int);

void				encrypt(char*, int, int);
void				decrypt(char*, int, int);
string				encryptFile(string, int);
string				decryptFile(string, int);

unsigned __stdcall	processFileThread(void*);

WSADATA wsaData;
WORD wVersion = MAKEWORD(2, 2);//# version of winsock
int SERVER_PORT;
char *NAME_PROGRAM;

int main(int argc, char* argv[])
{
	try {
		getPortFromCommandLineArguments(argc, argv);

		//Initiate WinSock
		initWinsock();

		//Construct LISTEN socket	
		SOCKET listenSock = createTCPSocket(AF_INET);


		associateLocalAddressWithSocket(listenSock, SERVER_ADDR, SERVER_PORT);

		//Listen request from client
		Listen(listenSock, 10);

		printf("Server started!!\nIP: %s\nPORT: %d\n", SERVER_ADDR, SERVER_PORT);
		sockaddr_in clientAddr;
		char clientIP[INET_ADDRSTRLEN];
		int clientAddrLen = sizeof(clientAddr), clientPort;
		SOCKET connSock;
		while (1) {
			connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
			if (connSock == SOCKET_ERROR) {
				printf("[Error %d] Cannot permit incoming connection", WSAGetLastError());
			}

			_beginthreadex(0, 0, processFileThread, (void *)connSock, 0, 0); //start thread
		}

	}
	catch (char *msg) {
		printf("%s", msg);
	}
}

unsigned __stdcall processFileThread(void *param) {
	SOCKET connSock = (SOCKET)(param);
	try {
		while (1) {
			string dir = getCurrentDateTime() + to_string(rand());
			int key = 0, cmd = -1;
			receiveFile(connSock, dir, cmd, key);
			
			string new_dir = "";

			switch (cmd) {
			case OP_ENCRYPTION:
				new_dir = encryptFile(dir, key);
				sendFile(connSock, new_dir, cmd, key);
				break;
			case OP_DECRYPTION:
				new_dir = decryptFile(dir, key);
				sendFile(connSock, new_dir, cmd, key);
				break;
			}
		}
	}
	catch (char* msgErr) {
		printf("%s", msgErr);
	}
	return 0;
}


void encrypt(char* payload, int size, int key) {
	for (int i = 0; i < size; i++) {
		payload[i] = static_cast<char> ((static_cast<unsigned char>(payload[i]) + key) % 256);
	}
}

string encryptFile(string src_dir, int key) {
	printf("Start Encrypt File!\n");
	
	string encrypt_dir = getCurrentDateTime() + to_string(rand());
	ifstream encrypt_file;
	encrypt_file.open(encrypt_dir, ios::app | ios::binary);

	if (!encrypt_file.is_open()) throw  "Cannot create file";
	
	streambuf *sb_encrypt = encrypt_file.rdbuf();

	char *payload = NULL;
	fstream src_file;
	src_file.open(src_dir, ios::in | ios::binary);

	if (src_file.is_open()) {
		streambuf *sb_src = src_file.rdbuf();
		size_t size = sb_src->pubseekoff(src_file.beg, src_file.end);
		
		payload = new char[BLOCK_SIZE];

		size_t sizeRest = 0;

		while (sizeRest < size) {
			sb_src->pubseekpos(sizeRest);
			sb_encrypt->pubseekpos(sizeRest);

			// read from file
			size_t bSuccess = sb_src->sgetn(payload, BLOCK_SIZE);

			// encrypt file
			encrypt(payload, bSuccess, key);

			printf("Encrypt %.2lf%%\r", sizeRest * 100.0 / size);
			fflush(stdin);

			// write encrypt from file
			write(sb_encrypt, sizeRest, payload, bSuccess);

			sizeRest += bSuccess;
		}
		printf("\nEncrypt 100%%\nEncrypt Finished!\n");
		
		delete[] payload;
		src_file.close();
	}

	encrypt_file.close();

	remove(src_dir.c_str());

	return encrypt_dir;
}


void decrypt(char* payload, int size, int key) {
	for (int i = 0; i < size; i++) {
		payload[i] = static_cast<char> (( 256 + static_cast<unsigned char>(payload[i]) - (key % 256)) % 256);
	}
}

string decryptFile(string src_dir, int key) {
	printf("Start Decrypt File!\n");

	string decrypt_dir = getCurrentDateTime() + to_string(rand());
	ifstream decrypt_file;
	decrypt_file.open(decrypt_dir, ios::app | ios::binary);

	if (!decrypt_file.is_open()) throw  "Cannot create file";

	streambuf *sb_decrypt = decrypt_file.rdbuf();

	char *payload = NULL;
	fstream src_file;
	src_file.open(src_dir, ios::in | ios::binary);

	if (src_file.is_open()) {
		streambuf *sb_src = src_file.rdbuf();
		size_t size = sb_src->pubseekoff(src_file.beg, src_file.end);

		payload = new char[BLOCK_SIZE];

		size_t sizeRest = 0;

		while (sizeRest < size) {
			sb_src->pubseekpos(sizeRest);
			sb_decrypt->pubseekpos(sizeRest);

			// read from file
			size_t bSuccess = sb_src->sgetn(payload, BLOCK_SIZE);

			// decrypt file
			decrypt(payload, bSuccess, key);

			printf("Decrypt %.2lf%%\r", sizeRest * 100.0 / size);
			fflush(stdin);

			// write decrypt from file
			write(sb_decrypt, sizeRest, payload, bSuccess);

			sizeRest += bSuccess;
		}
		printf("\nDecrypt 100%%\nDecrypt Finished!\n");

		delete[] payload;
		src_file.close();
	}

	decrypt_file.close();

	remove(src_dir.c_str());

	return decrypt_dir;
}

/**
* @function getPortFromCommandLineArguments: function set a port from command line arguments
*											 throw exception if the form of the port is not correct
* @param argc: number of arguments
* @param argv: array size argc, presents value of arguments
**/
void getPortFromCommandLineArguments(int argc, char **argv) {
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
		throw "Cannot place server socket in state LISTEN\n";
	}
}
