#pragma once

#include "Utilities.h"
#include <string>

using namespace std;

// define the size of the information fields
#define SIZE_ID 4
#define SIZE_COMMAND 4
#define SIZE_LENGTH_MESSAGE 4
#define SIZE_RESPONSE_MESSAGE 3

// define the error
#define CLIENT_DISCONNECTED 0

// define the command
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



struct Message {
	int id;//# id of the session
	int command;//# command of Message
	int length;//# length of content
	string content = "";//# content of Message

						//constructor
	Message(int _id, int _command, int _length, string _content) {
		this->id = _id;
		this->command = _command;
		this->length = _length;
		this->content = _content;
	}

	Message() {}

	char* to_c_str() {
		char *rs = new char[SIZE_ID + SIZE_COMMAND + SIZE_LENGTH_MESSAGE + this->length];
		char *bID = intToBytes(this->id);
		char *bCommand = intToBytes(this->command);
		char *bLength = intToBytes(this->length);

		int index = 0;
		copy(rs, index, bID, SIZE_ID);
		index += SIZE_ID;

		copy(rs, index, bCommand, SIZE_COMMAND);
		index += SIZE_COMMAND;

		copy(rs, index, bLength, SIZE_LENGTH_MESSAGE);
		index += SIZE_LENGTH_MESSAGE;


		copy(rs, index, this->content.c_str(), this->length);


		delete[] bID;
		delete[] bCommand;
		delete[] bLength;

		return rs;
	}
};

/*
* @function send: send a message
* @param inSock: socket where message send to
* @param message: message need to be sent
*
**/
void sendMessage(SOCKET &inSock, Message *message) {

	int lenLeft = SIZE_ID + SIZE_COMMAND + SIZE_LENGTH_MESSAGE + message->length;

	char *msg = message->to_c_str();
	char *tmp = msg;

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

			throw "Cannot receive data!\n";
		}

		if (ret == CLIENT_DISCONNECTED) {
			delete[] msg;
			throw CLIENT_DISCONNECTED;
		}

		lenLeft -= ret;
		tmp += ret;
	}

	msg[lenMessage] = '\0';

	return msg;
}


/*
* @function recvMessage: receive a messasge in a socket with id correspondingly
* @param inSock: socket where function receive messages
* @param ID: ID of message need to receive
*
* @return: a messge as Message struct
**/
Message* recvMessage(SOCKET &inSock, int ID) {
	while (1) {
		char *bID = recv(inSock, SIZE_ID);
		int id = bytesToInt(bID);

		char *bCommand = recv(inSock, SIZE_COMMAND);
		int command = bytesToInt(bCommand);

		char *bLength = recv(inSock, SIZE_LENGTH_MESSAGE);
		int length = bytesToInt(bLength);

		char *bContent = recv(inSock, length);
		string content = bContent;

		delete[] bID;
		delete[] bCommand;
		delete[] bLength;
		delete[] bContent;

		if (id == ID) 
			return new Message(id, command, length, content);
	}
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
