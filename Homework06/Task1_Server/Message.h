#pragma once

#include "Utilities.h"
#include "Account.h"
#include <string>

using namespace std;

// define the size of the information fields
#define SIZE_ID 8
#define SIZE_COMMAND 2
#define BUFF_SIZE 2048


// define the error
#define CLIENT_DISCONNECTED 0

// define the command
#define UNKNOWN 00
#define LOGIN 10
#define POST 20
#define LOGOUT 30
#define EXIT 40
#define RESPONE 50

// define the response code
#define UNDENTIFY_RESULT 000

#define LOGIN_SUCCESSFUL 100
#define LOGIN_FAIL_LOCKED 111
#define LOGIN_FAIL_NOT_EXIST 112
#define LOGIN_FAIL_ANOTHER_DEVICE 113
#define LOGIN_FAIL_ALREADY_LOGIN 114

#define LOGOUT_SUCCESSFUL 300
#define LOGOUT_FAIL 310
#define LOGOUT_FAIL_NOT_LOGIN 311

#define POST_SUCCESSFUL 200
#define POST_FAIL 210
#define POST_FAIL_NOT_LOGIN 211

#define RECV_FULL true
#define NOT_RECV_FULL false

struct Message {
	int id;
	int command;
	string content;//# content of Message

	//constructor
	Message(int _id, int _command, string _content) {
		this->id = _id;
		this->command = _command;
		this->content = _content;
	}

	Message() {
		this->id = 0;
		this->command = 0;
		this->content = "";
	}

	string toMessageSend() {
		string rs = reform(id, SIZE_ID) + to_string(command) + content + "\r\n";
		return rs;
	}
};

bool check(string &msg_rcv) {
	if (msg_rcv.length() < SIZE_ID + SIZE_COMMAND) return false;
	for (int i = 0; i < SIZE_ID + SIZE_COMMAND; i++) {
		if (isdigit(msg_rcv.at(i)) == false) return false;
	}
	return true;
}

/*
* @function sendMessage: send a message
* @param inSock: socket where message send to
* @param message: message need to be sent
*
**/
void sendMessage(SOCKET &inSock, Message &message) {
	string msgStr = message.toMessageSend();

	char *msg = new char[msgStr.length() + 1];
	char *tmp = msg;

	strcpy_s(msg, msgStr.length() + 1, message.toMessageSend().c_str());

	int lenLeft = msgStr.size();

	while (lenLeft > 0) {
		int ret = send(inSock, tmp, lenLeft, 0);

		if (ret == SOCKET_ERROR) {
			delete[] msg;
			throw "Cannot send data\n";
		}

		lenLeft -= ret;
		tmp += ret;
	}
	delete[] msg;
}


/**
* @function recvMessage: receive a message 
* @param inSock: the socket receives data
* @param resMessage: the rest of Message before
* @param saveMessage: the received message is saved in
*
* @return RECV_FULL if receive a message completely, NOT_RECV_FULL if not
***/
bool recvMessage(SOCKET &inSock, string &restMessage, Message &saveMessage) {
	char msg[BUFF_SIZE];
	memset(msg, '\0', sizeof(msg));
	size_t found;
	string rs = restMessage;
	
	int ret = recv(inSock, msg, BUFF_SIZE, 0);

	if (ret == SOCKET_ERROR) {
		throw "Cannot receive data!\n";
	}

	if (ret == 0) {
		throw "Client Disconnected!\n";
	}

	msg[ret] = '\0';
	string tmp = msg;
	found = tmp.find("\r\n");

	if (found == std::string::npos) {
		rs += tmp;
	}
	else {
		rs += tmp.substr(0, found);
		if (check(rs) == true) {
			saveMessage.id = atoi(rs.substr(0, SIZE_ID).c_str());
			saveMessage.command = atoi(rs.substr(SIZE_ID, SIZE_ID + SIZE_COMMAND).c_str());
			saveMessage.content = rs.substr(SIZE_ID + SIZE_COMMAND);
		} else {
			saveMessage.id = 0;
			saveMessage.command = UNKNOWN;
			saveMessage.content = rs;
		}
		return RECV_FULL;
	}

	
	return NOT_RECV_FULL;
}


/*
* @function commandCodeToString: convert Command Code to String
**/
string commandCodeToString(int commandCode) {
	switch (commandCode) {
		case LOGIN:
			return "LOGIN";
		case LOGOUT:
			return "LOGOUT";
		case POST:
			return "POST";
		case EXIT:
			return "EXIT";
		case RESPONE:
			return "RESPONSE";
		default:
			return "NO IDENTIFY";
		}
}