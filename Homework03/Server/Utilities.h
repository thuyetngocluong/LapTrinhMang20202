#pragma once
#include <ctype.h>
/*
* @function intToBytes: Convert an integer to an char array having length is 4
* @param num: the integer needs to convert
*
* @return: an array char having size is 4 correspondingly
**/
char* intToBytes(int num) {
	char *bytes = new char[4];

	bytes[0] = (num >> 24) & 0xFF;
	bytes[1] = (num >> 16) & 0xFF;
	bytes[2] = (num >> 8) & 0xFF;
	bytes[3] = num & 0xFF;

	return bytes;
}


/*
* @function bytesToInt: Convert a char array, which has length is 4, to an integer
* @param bytes: the char array needs to convert
*
* @return: a corresponding integer
**/
int bytesToInt(char* bytes) {
	return int((unsigned char)(bytes[0]) << 24 |
		(unsigned char)(bytes[1]) << 16 |
		(unsigned char)(bytes[2]) << 8 |
		(unsigned char)(bytes[3]));
}



/*
* @function copy: copy all characters from src to dest
* @param dest: array of chars destination 
* @param posDest: position to start copy of array of chars destination 
* @param src: array of chars need to copy
* @param lengthSrc: length of src 
**/
void copy(char *dest, int posDest, const char* src, int lengthSrc) {
	for (int i = posDest; i < posDest + lengthSrc; i++) {
		dest[i] = src[i - posDest];
	}
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