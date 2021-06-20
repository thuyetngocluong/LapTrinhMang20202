#pragma once

#include "Utilities.h"
#include <string>

using namespace std;

// define the size of the information fields
#define SIZE_ID 8
#define SIZE_COMMAND 2
#define BUFF_SIZE 2048

// define the error
#define CLIENT_DISCONNECTED 0

// define the command
#define LOGIN 10
#define POST 20
#define LOGOUT 30
#define EXIT 40
#define RESPONE 50
#define UNKNOWN 00

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

string restMessage = "";

struct Message {
	int id;
	int command;
	string content = "";//# content of Message

	//constructor
	Message(int _id, int _command, string _content) {
		this->id = _id;
		this->command = _command;
		this->content = _content;
	}

	Message() {}

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
* @function send: send a message
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
*
* @return the message was received
***/
Message recvMessage(SOCKET &inSock, int ID) {
	char msg[BUFF_SIZE];
	Message message(0, UNKNOWN, "");
	size_t found;
	string rs = restMessage;
	while (1) {
		string rs = restMessage;
		while (1) {
			int ret = recv(inSock, msg, BUFF_SIZE, 0);

			if (ret == SOCKET_ERROR) {

				if (WSAGetLastError() == WSAETIMEDOUT) {
					throw "Time-out";
				}

				throw "Cannot receive data!\n";
			}

			msg[ret] = '\0';
			string tmp = msg;
			found = tmp.find("\r\n");
			if (found == std::string::npos) {
				rs += tmp;
			}
			else {
				rs += tmp.substr(0, found);
				restMessage = tmp.substr(found + 2, tmp.length());
				break;
			}
		}
		if (check(rs) == false) return message;

		message.id = atoi(rs.substr(0, SIZE_ID).c_str());
		message.command = atoi(rs.substr(SIZE_ID, SIZE_ID + SIZE_COMMAND).c_str());
		message.content = rs.substr(SIZE_ID + SIZE_COMMAND).c_str();
		if (message.id == ID || message.id == 0) break;
	}
	return message;
}


/*
* @function getCommandCode: convert command code from string to int
* @param cmd: command code as a string
*
* @return: command code as a int
**/
int getCommadCode(string cmd) {
	if (_stricmp(cmd.c_str(), "login") == 0) return LOGIN;
	if (_stricmp(cmd.c_str(), "logout") == 0) return LOGOUT;
	if (_stricmp(cmd.c_str(), "post") == 0) return POST;
	if (_stricmp(cmd.c_str(), "exit") == 0) return EXIT;
	return 0;
}
