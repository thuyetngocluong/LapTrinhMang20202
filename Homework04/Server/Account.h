#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <time.h>

#include "Message.h"
#include "File.h"

using namespace std;

#define LOGGED 1
#define NOT_LOGGED 0


struct Account {
	SOCKET socket; //#socket of account
	sockaddr_in address;//# address of account
	char IP[INET_ADDRSTRLEN];//# ip of account
	int PORT;//#port of account
	string username = "";//#username which the account logged in
	int status = NOT_LOGGED;//# login status of the account
	int posInListClient = -1;//# positon of account in list service of server
};


/*
* @function getInforAccount: get information of a account
* @param account: account need to get information
*
* @return: a string brings information of account
**/
string getInforAccount(Account *account) {
	string rs = account->IP;
	rs += ":";
	rs += to_string(account->PORT);

	return rs;
}


/*
* @funtion getCurrentDateTime: get current date and time 
*
* @return string brings information of current date and time
**/
string getCurrentDateTime() {
	char MY_TIME[22];
	struct tm newtime;
	__time64_t long_time;
	errno_t err;

	// Get time as 64-bit integer.
	_time64(&long_time);
	// Convert to local time.
	err = _localtime64_s(&newtime, &long_time);
	if (err) {
		return "[00/00/00 00:00:00]";
	}
	strftime(MY_TIME, sizeof(MY_TIME), "[%d/%m/%Y %H:%M:%S]", &newtime);
	string result = MY_TIME;

	return result;
}


/*
* @function saveLog: save request of account to file
* @param account: the information of account
* @param message: the information of message 
* @param statusCode: status Code after account perform request
*
* @return true if save successfully, false if not
**/
bool saveLog(Account *account, Message *message, int resultCode) {
	string command;
	switch (message->command) {

		case LOGIN: 
			command = "LOGIN ";
			break;

		case POST:
			command = "POST  ";
			break;

		case LOGOUT:
			command = "LOGOUT";
			break;

		default:
			command = "NO_IDENTIFICATION";
			break;
	}

	string rs = getInforAccount(account) + " " + getCurrentDateTime();
	rs += " $ COMMAND: " + command;
	
	rs += " $ RESULT_CODE: " + to_string(resultCode);

	if (message->content.empty() == false) {
		rs += " $ CONTENT: " + message->content;
	}

	return save(rs.c_str());
}




