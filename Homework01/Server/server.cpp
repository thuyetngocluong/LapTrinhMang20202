#include <stdafx.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <WinSock2.h>
#include <winsock.h>
#include <WS2tcpip.h>
#include <cstdlib>
#include <string.h>
#include <iostream>

using namespace std;
#pragma comment(lib, "Ws2_32.lib")

#define BUFF_SIZE 2048
#define IP_SERVER "127.0.0.1"

int PORT_SERVER;//# Port of server
char *NAME_PROGRAM;//# name of program
WSADATA wsaData;
WORD wVersion = MAKEWORD(2, 2);//# version of winsock

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
* @function getPortFromCommandLineArguments: function set a port from command line arguments
*											 throw exception if the form of the port is not correct
* @param argc: number of arguments
* @param argv: array size argc, presents value of arguments
**/
void getPortFromCommandLineArguments(int argc, char *argv[]) {
	NAME_PROGRAM = argv[0]; // first agrument is name of program

	if (argc < 2) throw "ERROR! PORT HAS NOT BEEN ENTER ";

	if (isNumber(argv[1]) == false) throw "ERROR! PORT MUST BE A NUMBER";

	PORT_SERVER = atoi(argv[1]);

	if (PORT_SERVER < 0 || PORT_SERVER > 65535) throw "ERROR! PORT NUMBER MUST BE IN RANGE 0 and 65535";
}


/**
* @function startWinsock: initiates use of the Winsock by a process.
*
**/
void startWinsock() {
	int err = WSAStartup(wVersion, &wsaData);
	if (err != 0) throw "ERROR! VERSION IS NOT SUPPORTED";
}

/*finish winshock process*/
void finishWinsock() { WSACleanup(); }

/**
* @function createUDPServer: create a server and bind with a socket.
*
* @param internet_protocol: the internet protocol (IPv4 or IPv6 or both)
* @param ip_server: the ip of server
* @param port_server: service portal needs to be open
*
* @return SOCKET: present information of server
**/
SOCKET createUDPServer(short internet_protocol, char *ip, int port) {
	SOCKET server = socket(internet_protocol, SOCK_DGRAM, IPPROTO_UDP);//init a UDP socket

	if (server == INVALID_SOCKET) throw "ERROR! CANNOT CREATE SOCKET";

	// configure sockaddr_in
	sockaddr_in serverAddr;
	serverAddr.sin_family = internet_protocol;
	serverAddr.sin_port = htons(port);
	inet_pton(internet_protocol, ip, &serverAddr.sin_addr);

	// bind socket with configured struct
	if (bind(server, (sockaddr*)&serverAddr, sizeof(serverAddr)))
		throw "ERROR! CANNOT BIND THIS ADDRESS";

	return server;
}


/**
* @function getAddrFromIP: get name host of ip
* @param IP: the ip address to be queried
*
* @return : the result, as a hostent struct,  host information of ip address
*/
hostent* getAddrFromIP(char *IP) {

	// configure struct of sockaddr
	struct sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	inet_pton(AF_INET, IP, &(sockAddr.sin_addr.s_addr));
	in_addr addr = sockAddr.sin_addr;

	// get hosts 
	hostent *remoteHost = gethostbyaddr((char *)&addr, 4, AF_INET);

	return remoteHost;
}


/**
* @function getIPFromAddr : get list IP of a domain
* @param domain: string present name of domain
*
* @return: list of addrinfo - list of ip
*/
addrinfo* getIPFromAddr(char* domain) {

	//configure information
	addrinfo *result;
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;

	// get ips
	int rc = getaddrinfo(domain, NULL, &hints, &result);

	if (rc != 0) {
		delete result;
		return NULL;
	}

	return result;
}

/**
* @function convertToString : convert to a string as a result to send to client
* @param remoteHost : hostent present information of domain
* @param IP: ip address corresponds to domain
*
* @return: a string to send to client
**/
char* convertToString(hostent *remoteHost, char* IP) {
	char *result = new char[BUFF_SIZE];

	if (remoteHost == NULL) {
		strcpy_s(result, BUFF_SIZE, "-ERROR!! HOST NOT FOUND ");// error message start with "-"	
		return result;
	}

	strcpy_s(result, BUFF_SIZE, "+Host of IP ");// successful message start with "+"
	strcat_s(result, BUFF_SIZE, IP);
	strcat_s(result, BUFF_SIZE, ":\n");
	strcat_s(result, BUFF_SIZE, "Official name : ");
	strcat_s(result, BUFF_SIZE, remoteHost->h_name);
	strcat_s(result, BUFF_SIZE, "\n");

	// add all domains to string
	for (char **pAlias = remoteHost->h_aliases; *pAlias != 0; pAlias++) {
		strcat_s(result, BUFF_SIZE, "Alternate name: ");
		strcat_s(result, BUFF_SIZE, *pAlias);
		strcat_s(result, BUFF_SIZE, "\n");
	}

	return result;
}


/**
* @function convertToString : convert to a string as a result to send to client
* @param listIP : addrinfo present list of IP
* @param domain: domain corresponds to IPs
*
* @return: a string to send to client
**/
char* convertToString(addrinfo *listIP, char *domain) {
	char *result = new char[BUFF_SIZE];

	if (listIP == NULL) {
		strcpy_s(result, BUFF_SIZE, "-NO FOUND INFORMATION");// error message with "-"
		return result;
	}

	addrinfo *tmp = listIP;//#a pointer point to first element of listIP
	sockaddr_in *address;//# information of an ip address
	char ipStr[INET_ADDRSTRLEN];//#ip address

	strcpy_s(result, BUFF_SIZE, "+IPv4 of domain "); // success message with "+"
	strcat_s(result, BUFF_SIZE, domain);
	strcat_s(result, BUFF_SIZE, " :\n");

	// add all ips to string
	while (tmp != NULL) {
		address = (struct sockaddr_in *) tmp->ai_addr;
		inet_ntop(AF_INET, &address->sin_addr, ipStr, sizeof(ipStr));
		strcat_s(result, BUFF_SIZE, ipStr);
		strcat_s(result, BUFF_SIZE, "\n");
		tmp = tmp->ai_next;
	}

	return result;
}


/**
* @function recvMsg : receive a message from client
* @param inSocket : indicate socket which server receive from
* @param fromClient: indicate client which server receive from
*
* @return: a receive message
**/
char* recvMsg(SOCKET &inSocket, sockaddr_in &fromClient) {
	char *recvMsg = new char[BUFF_SIZE];
	int fromClientLen = sizeof(fromClient);
	int recvCode = recvfrom(inSocket, recvMsg, BUFF_SIZE, 0, (sockaddr*)&fromClient, &fromClientLen);

	if (recvCode == SOCKET_ERROR) {
		strcpy_s(recvMsg, BUFF_SIZE, "ERROR! CANNOT RECEIVE MESSAGE");
	}
	recvMsg[recvCode] = '\0';
	return recvMsg;
}


/**
* @function sendMsg: send a message from client
* @param inSocket : indicate socket which server send to
* @param toClient: indicate client which server send to
* @param msg: message server send
*
**/
void sendMsg(SOCKET &inSocket, sockaddr_in &toClient, char *msg) {
	int sendCode = sendto(inSocket, msg, strlen(msg), 0, (sockaddr*)&toClient, sizeof(toClient));
	if (sendCode == SOCKET_ERROR) {
		printf("Error send: %d\n", WSAGetLastError());
	}
}


/*MAIN FUNCTION*/
int main(int argc, char *argv[]) {
	try {
		getPortFromCommandLineArguments(argc, argv);

		startWinsock();

		SOCKET server = createUDPServer(AF_INET, IP_SERVER, PORT_SERVER);

		printf("SERVER STARTED!!\nIP SERVER: %s\nPORT SERVER: %d\n", IP_SERVER, PORT_SERVER);

		sockaddr_in clientAddr;//#address of client
		int clientPort;//#port of client
		char clientIP[INET_ADDRSTRLEN];//#ip of client

		while (1) {
			char *receiveMsg = recvMsg(server, clientAddr);// receive a request from client

	        // show information of client
			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
			clientPort = ntohs(clientAddr.sin_port);
			printf("Receive from client %s:%d %s\n", clientIP, clientPort, receiveMsg);

			char *msg;

			if (isValidIP(receiveMsg)) {
				hostent *result = getAddrFromIP(receiveMsg);
				msg = convertToString(result, receiveMsg);
			}
			else {
				addrinfo *result = getIPFromAddr(receiveMsg);
				msg = convertToString(result, receiveMsg);
			}

			sendMsg(server, clientAddr, msg);// send to a message to client

			delete receiveMsg;
			delete msg;

		}

		closesocket(server);

		finishWinsock();
	}
	catch (const char *msg) {
		cerr << msg;
	}


}