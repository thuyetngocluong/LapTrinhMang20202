#pragma once

#include "Utilities.h"
#include <string>

using namespace std;

// define the size of the information fields
#define SIZE_ID 4
#define SIZE_COMMAND 4
#define SIZE_LENGTH_MESSAGE 4
#define SIZE_RESPONSE_MESSAGE 3


#define MAX_NUMBER_CONSECUTIVE_FAILED_MESSAGES 5

// define the error
#define CLIENT_DISCONNECTED 0
#define EXCEED_NUMBER_CONSECUTIVE_FAILED_MESSAGES -1

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
* @param numberError: number of errors when send a message
*
**/
void sendMessage(SOCKET &inSock, Message *message, int &numberError) {

	int lenLeft = SIZE_ID + SIZE_COMMAND + SIZE_LENGTH_MESSAGE + message->length;

	char *msg = message->to_c_str();
	char *tmp = msg;

	while (lenLeft > 0) {
		int ret = send(inSock, tmp, lenLeft, 0);

		if (ret == SOCKET_ERROR) {
			delete[] msg;
			numberError++;
			if (numberError >= MAX_NUMBER_CONSECUTIVE_FAILED_MESSAGES) {
				throw EXCEED_NUMBER_CONSECUTIVE_FAILED_MESSAGES;
			}
			throw "Cannot send data!\n";
		}

		lenLeft -= ret;
		tmp += ret;
	}
	numberError = 0;
	delete[] msg;
}

/*
* @function recv: receive a message
* @param inSock: socket, where message receives from
* @param lenMessage :length of received message
* @param numberError: number of errors when receive a message
*
* @return: received message as array of chars
**/
char* recv(SOCKET &inSock, int lenMessage, int &numberError) {
	char *msg = new char[lenMessage + 1];
	char *tmp = msg;
	int lenLeft = lenMessage;

	while (lenLeft > 0) {
		int ret = recv(inSock, tmp, lenLeft, 0);

		if (ret == SOCKET_ERROR) {
			delete[] msg;

			numberError++;
			if (numberError >= MAX_NUMBER_CONSECUTIVE_FAILED_MESSAGES) {
				throw EXCEED_NUMBER_CONSECUTIVE_FAILED_MESSAGES;
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
	numberError = 0;
	return msg;
}

/*
* @function recv: receive a message
* @param inSock: socket, where message receives from
* @param numberError: number of errors when receive a message
*
* @return: received message as Message struct
**/
Message* recvMessage(SOCKET &inSock, int &numberError) {
	char *bID = recv(inSock, SIZE_ID, numberError);
	int id = bytesToInt(bID);

	char *bCommand = recv(inSock, SIZE_COMMAND, numberError);
	int command = bytesToInt(bCommand);

	char *bLength = recv(inSock, SIZE_LENGTH_MESSAGE, numberError);
	int length = bytesToInt(bLength);

	char *bContent = recv(inSock, length, numberError);
	string content = bContent;

	delete[] bID;
	delete[] bCommand;
	delete[] bLength;
	delete[] bContent;

	return new Message(id, command, length, content);
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