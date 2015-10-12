/***********************************************
*
*Filename: util.c
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-09-18 13:46:19
*Last Modified: 2015-09-18 13:46:19
************************************************/
#include "util.h"

void error(char * error){
	printf("%s",error);
}

void warning(char * warning){
	printf("%s",warning);
}
void notice(char * notice){
	printf("%s", notice);
}
void debug(char* debug){
	printf("%s", debug);
}

int checkFileExist(char * filePath) {
	struct stat statStruct;
	return ( stat(filePath,&statStruct) ) == 0;
}

int getFileSize(char * filePath) {
	struct stat statStruct;
	if (stat(filePath,&statStruct) == 0) { 
		return statStruct.st_size;
	} else {
		return -1;
	}
}

int transferFile(char* pathToFile, int sockFd){
	FILE * fp;
	char buffer[1024];
	int len = 0;
	fp = fopen(pathToFile,"r");
	if (fp == NULL) {
		return 1;
	}
	memset(buffer,'\0',sizeof(buffer));
	while((len = fread(buffer,sizeof(char),255,fp) ) > 0) {
		if (send(sockFd,buffer,len,0) <= 0) {
			return 1;
		}
		memset(buffer,'\0',sizeof(buffer));
	}
	fclose(fp);
	return 0;
}

/**
*@description get one line at a time from the recv buffer of socket
*@param sockFd  socket description
*@param buffer[] char array to store the char line 
*@return int, the length of recieved line or -1 if error happened
*/
int getLineFromSock(int sockFd, char* line, size_t size_line) {
	int lenLine;
	char buffer;
	int tempResult; 

	memset(&buffer, '\0', sizeof(buffer));
	lenLine = 0;
	tempResult = 0;
	while((tempResult = recv(sockFd, &buffer, 1, 0)) >= 0 && lenLine < size_line) {
		if (tempResult > 0) {
			if (buffer == '\n') {
				if (lenLine > 0) {
					line[lenLine - 1] = '\0'; //remove recieved '\r'
					return --lenLine;
				} else {
					return -1;
				}
			} else {
				line[lenLine++] = buffer;
			}
		}
	}
	return -1;
}

/**
*@description recieve file from socket and output in stdin
*@param fileLen int file size
*@sockFd int the socket from which to read file
*int recieved bytes num, if errored, return negative number
*/
int recieveFile(size_t fileSize, int sockFd) {
	char buffer[1024];
	int recvLen = 0;//recieved length
	int tempLen;

	memset(buffer, '\0', sizeof(buffer));
	while((tempLen = recv(sockFd, buffer, sizeof (buffer)-1, 0)) >= 0) {
		recvLen += tempLen;
		//printf(buffer);
		if (recvLen >= fileSize) {
			return recvLen;
		}
		memset(buffer, '\0', sizeof(buffer));
	}
	return -1;
}
