#pragma once
#include <ctype.h>
#include <string>

using namespace std;

/*
* @function reform: reform a integer to string having numberOfChar characters
* @function a: the integer need to reform
* @function numberOfChars: the number of characters result
*
* @return: a string having numberOfChars characters
**/
string reform(int a, int numberOfChars) {
	string rs;
	string numStr = to_string(a);
	for (int i = 0; i < numberOfChars - numStr.length(); i++) {
		rs += "0";
	}
	rs += numStr;

	return rs;
}

/*
* @function isNumber: Check if a string is a number or not
* @param s: string needs to be check
*
* @return true if string is a number, false if string not a number
**/
bool isNumber(char *s) {
	int i = 0;

	while (s[i] != '\0') {
		if (isdigit(s[i]) == false) return false;
		i++;
	}

	return true;
}
