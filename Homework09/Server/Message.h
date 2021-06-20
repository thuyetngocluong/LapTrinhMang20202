#pragma once

#include "Utilities.h"
#include "Account.h"
#include <string>
#include <queue>

using std::string;

// define the size of the information fields
#define SIZE_ID 8
#define SIZE_COMMAND 2
#define BUFF_SIZE 2048

#define RECEIVE 0
#define SEND 1


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
		string rs = reform(id, SIZE_ID) + reform(command, SIZE_COMMAND) + content + "\r\n";
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
* @function saveMessage: save request in to queue
* @param tmp: message just received
* @param restMessage: the rest of message
* @param requests: the queue of request
**/
void saveMessage(string tmp, string &restMessage, std::queue<Message> &requests) {
	Message request;
	restMessage += tmp;
	tmp = restMessage;
	size_t found = restMessage.find("\r\n");

	while (found != std::string::npos) {
		restMessage = tmp.substr(found + 2);
		string s = tmp.substr(0, found);

		if (check(tmp) == true) {
			request.id = atoi(s.substr(0, SIZE_ID).c_str());
			request.command = atoi(s.substr(SIZE_ID, SIZE_COMMAND).c_str());
			request.content = s.substr(SIZE_ID + SIZE_COMMAND);
		}
		else {
			request.id = 0;
			request.command = UNKNOWN;
			request.content = s;
		}

		requests.push(request);
		tmp = restMessage;
		found = restMessage.find("\r\n");
	}
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