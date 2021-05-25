#include <fstream>
#include <string>
using namespace std;

#define OP_ENCRYPTION 0
#define OP_DECRYPTION 1
#define OP_TRANSMISSION 2
#define OP_ERROR 3

#define SIZE_OPCODE 1
#define SIZE_LENGTH 2
#define BLOCK_SIZE 65535

#define TYPE_ENCRYPTION 00
#define TYPE_DECRYPTION 10
#define TYPE_TRANS 20
#define TYPE_TRANS_FINISH 21
#define TYPE_ERROR 30
#define TYPE_UNKNOWN 40

#define CLIENT_DISCONNECTED 0

struct Message {
	int opcode;
	int length = 0;
	char* payload = "";

	// default constructor
	Message() {}

	// constructor full
	Message(int _opcode, int _length, char* _payload) {
		this->opcode = _opcode;
		this->length = _length;
		this->payload = _payload;
	}

	// constructor create key
	Message(int _opcode, int key) {
		this->opcode = _opcode;

		if (opcode != OP_ENCRYPTION && opcode != OP_DECRYPTION)  return;

		this->length = to_string(key).length();
		this->payload = stringToArrayChar(to_string(key));
	}


	char *toCharArray() {
		char *msg = new char[SIZE_OPCODE + SIZE_LENGTH + length];
		int index = 0;
		copy(msg, index, to_string(opcode), SIZE_OPCODE);
		index += SIZE_OPCODE;

		copy(msg, index, intToBytes(length), SIZE_LENGTH);
		index += SIZE_LENGTH;

		copy(msg, index, payload, length);

		return msg;
	}
};

int typeMessage(Message &message) {
	if (message.opcode == OP_TRANSMISSION && message.length != 0) return TYPE_TRANS;
	if (message.opcode == OP_ENCRYPTION) return TYPE_ENCRYPTION;
	if (message.opcode == OP_DECRYPTION) return TYPE_DECRYPTION;
	if (message.opcode == OP_TRANSMISSION && message.length == 0) return TYPE_TRANS_FINISH;
	if (message.opcode == OP_ERROR) return TYPE_ERROR;
	return TYPE_UNKNOWN;
}


int Send(SOCKET &inSock, char* msg, int lenMsg, int flag) {
	int lenLeft = lenMsg;
	char *tmp = msg;
	while (lenLeft > 0) {
		int ret = send(inSock, tmp, lenLeft, flag);
		if (ret == SOCKET_ERROR) {
			throw "Cannot send data\n";
		}

		lenLeft -= ret;
		tmp += ret;
	}

	return lenMsg;
}


void sendMessage(SOCKET &inSock, Message message) {
	char *cMsg = message.toCharArray();

	Send(inSock, cMsg, SIZE_OPCODE + SIZE_LENGTH + message.length, 0);

	delete[] cMsg;
}

char* Recv(SOCKET &inSock, int lenMessage) {
	char *msg = new char[lenMessage + 1];
	char *tmp = msg;
	int lenLeft = lenMessage;

	while (lenLeft > 0) {
		int ret = recv(inSock, tmp, lenLeft, 0);

		if (ret == SOCKET_ERROR) {
			delete[] msg;
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

void write(streambuf *dest, int pos, char *payload, int len) {
	char *tmp = payload;
	size_t rest = 0;
	size_t tPos = pos;
	while (rest < len) {
		dest->pubseekpos(tPos);
		size_t bSuccess = dest->sputn(tmp, len - rest);
		tPos += bSuccess;
		rest += bSuccess;
		tmp += bSuccess;
	}
}

Message recvMessage(SOCKET &inSock) {
	char *cOpcode = Recv(inSock, SIZE_OPCODE);
	int op_code = atoi(cOpcode);
	delete[] cOpcode;

	char *cLength = Recv(inSock, SIZE_LENGTH);
	int length = bytesToInt(cLength);
	delete[] cLength;

	char *payload = Recv(inSock, length);

	return Message(op_code, length, payload);
}


void receiveFile(SOCKET &inSock, string dir, int &cmd, int &key) {

	Message command = recvMessage(inSock);
	key = atoi(command.payload);
	cmd = command.opcode;

	ifstream writeFile;
	writeFile.open(dir, ios::app | ios::binary);
	if (!writeFile.is_open()) throw "Cannot create file\n";
	writeFile.clear();

	streambuf *sb = writeFile.rdbuf();
	int numErr = 0;
	while (numErr < 5) {
		size_t pos = 0, bSuccess;
		char *tmp;
		sb->pubseekpos(pos);

		bool RECEIVE_FULL = false;
		bool RECEIVE_ERROR = false;
		while (!RECEIVE_FULL && !RECEIVE_ERROR) {
			Message file = recvMessage(inSock);

			switch (typeMessage(file)) {
			case TYPE_TRANS:
				write(sb, pos, file.payload, file.length);
				pos += file.length;
				printf("Downloaded %lld bytes\r", pos);
				fflush(stdin);
				break;
			case TYPE_TRANS_FINISH:
				RECEIVE_FULL = true;
				printf("\nDownloaded 100%%\n", pos);
				printf("Download Finished!\n");
				break;
			case TYPE_ERROR:
				RECEIVE_ERROR = true;
				printf("\nDownload Error!\n");
				numErr++;
				break;
			}
		} // end while receive data

		if (RECEIVE_FULL) break;
	}
}

void sendFile(SOCKET &inSock, string directory, int &command,  int &key) {

	//receive key
	Message commandMsg(command, key);
	try {
		sendMessage(inSock, commandMsg);
	}
	catch (char* err) {
		printf("Cannot send key!\n");
		return;
	}

	// open file
	char *payload = NULL;
	fstream ifs;
	ifs.open(directory, ios::in | ios::binary);

	if (ifs.is_open()) {
		streambuf *sb = ifs.rdbuf();
		size_t size = sb->pubseekoff(ifs.beg, ifs.end);
		payload = new char[BLOCK_SIZE];
		int numErr = 0;
		while (numErr < 5) {

			sb->pubseekpos(ifs.beg);
			size_t sizeRest = 0;

			while (sizeRest < size) { // while trans file

				// read data from file
				sb->pubseekpos(sizeRest);
				size_t bSuccess = sb->sgetn(payload, BLOCK_SIZE);

				printf("Upload %.2lf%%\r", sizeRest * 100.0 / size);
				fflush(stdin);

				// send file
				Message msg(OP_TRANSMISSION, bSuccess, payload);
				try {
					sendMessage(inSock, msg);
				}
				catch (char* err) { // if send error, send message error, resend
					Message errorMessage(OP_ERROR, 0, "");
					sendMessage(inSock, errorMessage);
					printf("\nUpload Error, Resend File!\n");
					numErr++;
					break;
				}
				sizeRest += bSuccess;
			} // end while send file

			if (sizeRest >= size) { // send finish
				Message finishMsg(OP_TRANSMISSION, 0, "");
				sendMessage(inSock, finishMsg);
				printf("\nUpload 100%%\n");
				printf("Upload Finished!\n");
				break;
			}
		}

		ifs.close();
	}

	remove(directory.c_str());
}