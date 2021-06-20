#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

#define FILE_ACCOUNT "account.txt"
#define FILE_LOG "log_20183994.txt"

#define LOCKED 1
#define ACTIVE 0
#define NOT_EXIST -1

#define SAVE_SUCCESSFUL true
#define SAVE_FAIL false


/*
* @function getStatus: get status of username
* @param request: message brings username
*
* @return LOCK, ACTIVE or NOT_EXIST 
**/
int getStatus(Message request) {
	string line;
	ifstream myfile(FILE_ACCOUNT);

	if (myfile.is_open()) {
		while (getline(myfile, line)) {	
			string user;
			int status = NOT_EXIST;

			stringstream ss(line);
			
			ss >> user;
			ss >> status;

			if (strcmp(user.c_str(), request.content.c_str()) == 0) {
				return status;
			}
		}
		myfile.close();
	}

	return NOT_EXIST;
}

void writeNewLogFile() {
	ofstream out(FILE_LOG, ios::app);
	ifstream in(FILE_LOG);
	if (in.is_open()) {
		bool empty = (in.get(), in.eof());
		if (!empty) {
			in.close();
			return;
		}
		in.close();
		out << "****************************************" << std::endl;
		out << "     Meaning               Result Code" << std::endl;
		out << "UNDENTIFY_RESULT               000" << std::endl << std::endl;
		out << "LOGIN_SUCCESSFUL               100" << std::endl;
		out << "LOGIN_FAIL_LOCKED              111" << std::endl;
		out << "LOGIN_FAIL_NOT_EXIST           112" << std::endl;
		out << "LOGIN_FAIL_ANOTHER_DEVICE      113" << std::endl;
		out << "LOGIN_FAIL_ALREADY_LOGIN       114" << std::endl << std::endl;
		out << "LOGOUT_SUCCESSFUL              300" << std::endl;
		out << "LOGOUT_FAIL                    310" << std::endl;
		out << "LOGOUT_FAIL_NOT_LOGIN          311" << std::endl << std::endl;
		out << "POST_SUCCESSFUL                200" << std::endl;
		out << "POST_FAIL                      210" << std::endl;
		out << "POST_FAIL_NOT_LOGIN            211" << std::endl;
		out << "****************************************" << std::endl << std::endl;
	}
	out.close();
}

/*
* @function save: save a const char to file
* @param log: log need to save
*
* @return true if save successfully, false if not
**/
bool save(const char* log) {
	writeNewLogFile();
	ofstream myfile(FILE_LOG, ios::app);
	if (myfile.is_open()) {
		myfile << log;
		myfile << std::endl;
		myfile.close();
		return true;
	}

	return false;
}