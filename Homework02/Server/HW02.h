#pragma once

#pragma once

#define NUMBER_BYTES_ID 4
#define NUMBER_BYTES_LEN_MSG 4

/*
* @function intTo4Bytes: Convert an integer to an char array having length is 4
* @param num: the integer needs to convert
* 
* @return: an array char having size is 4 correspondingly
**/
char* intTo4Bytes(int num) {
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
* @struct Message include 3 attributes: id - identifier for the session
										len - identifier for lengthOfContent
										content - description for the message content
**/
struct Message {
	int id;//# id of the session
	int len;//# length of content
	char *content;//# content of message

	Message(int _id, int _len, char *_content) {//constructor
		this->id = _id;
		this->len = _len;
		this->content = _content;
	}


	// @function toString: convert struct Message to array of char 
	char* toString() {
		//length of temp is total of (id, len, length of content)
		char *tmp = new char[NUMBER_BYTES_ID + NUMBER_BYTES_LEN_MSG + len];

		char *bID = intTo4Bytes(id);
		char *bLen = intTo4Bytes(len);

		int index = 0;

		//Copy 4 bytes of id to tmp
		for (int i = 0; i < 4; i++) {
			tmp[index] = bID[i];
			index++;
		}

		//Copy 4 bytes of len to tmp
		for (int i = 0; i < 4; i++) {
			tmp[index] = bLen[i];
			index++;
		}

		//Copy content to tmp
		for (int i = 0; i < len; i++) {
			tmp[index] = content[i];
			index++;
		}

		delete bLen;
		delete bID;

		return tmp;
	}
};

/*
* @funtion isDigit: check if a character is a digit or not
* @param c: the character needs to check
*
* @return: true or false corresponsingly
**/
bool isDigit(char c) {
	return c >= '0' && c <= '9';
}


/*
* @funtion isAlpha: check if a character is an alphabet letter or not
* @param c: the character needs to check
*
* @return: true or false corresponsingly
**/
bool isAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}


/*
* @funtion isAlpha: check if an array of char is a number  or not
* @param c: the array of char needs to check
*
* @return: true or false corresponsingly
**/
bool isNumber(char *s) {
	int i = 0;

	while (s[i] != '\0') {
		if (isDigit(s[i]) == false) return false;
		i++;
	}

	return true;
}