#pragma once
#include <ctype.h>
#include <string>

using namespace std;


string reform(int num, int numberOfChar) {
	string rs;
	string src = to_string(num);
	for (int i = 0; i < numberOfChar - src.length(); i++) {
		rs += '0';
	}
	rs += src;

	return rs;
}


char* stringToArrayChar(string src) {
	int size = src.length();
	char *dest = new char[size + 1];
	for (int i = 0; i < size; i++) {
		dest[i] = src.at(i);
	}
	dest[size] = '\0';
	return dest;
}




char* intToBytes(int num) {
	char *bytes = new char[2];

	bytes[0] = (num >> 8) & 0xFF;
	bytes[1] = num & 0xFF;

	return bytes;
}


/*
* @function bytesToInt: Convert a char array, which has length is 4, to an integer
* @param bytes: the char array needs to convert
*
* @return: a corresponding integer
**/
int bytesToInt(char* bytes) {
	return int((unsigned char)(bytes[0]) << 8 |
		(unsigned char)(bytes[1]));
}



/*
* @function copy: copy all characters from src to dest
* @param dest: array of chars destination
* @param posDest: position to start copy of array of chars destination
* @param src: array of chars need to copy
* @param lengthSrc: length of src
**/
void copy(char *dest, int posDest, char* src, int lengthSrc) {
	for (int i = posDest; i < posDest + lengthSrc; i++) {
		dest[i] = src[i - posDest];
	}
}

/*
* @function copy: copy all characters from src to dest
* @param dest: array of chars destination
* @param posDest: position to start copy of array of chars destination
* @param src: string need to copy
* @param lengthSrc: length of src
**/
void copy(char *dest, int posDest, string src, int lengthSrc) {
	for (int i = posDest; i < posDest + lengthSrc; i++) {
		dest[i] = src.at(i - posDest);
	}
}

/*
* @function isNumber: Check if a string is a number or not
* @param s: string needs to be check
*
* @return true if string is a number, false if string not a number
**/
bool isNumber(const char *s) {
	int i = 0;

	while (s[i] != '\0') {
		if (isdigit(s[i]) == false) return false;
		i++;
	}

	return true;
}

/*
* @function isValidIP: check if a string is a valid IP or not
* @param IP: a string need to check
**/
bool isValidIP(char *IP) {
	in_addr addr;
	return inet_pton(AF_INET, IP, &addr.s_addr) == 1;
}