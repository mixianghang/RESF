#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

//printout msgs
void error(char * error);
void warning(char * warning);
void notice(char * notice);
void debug(char* debug);

//check file existence, ture means existence
int checkFileExist(char * filePath);

// get the size of file
int getFileSize(char * filePath);

//transfer file to socket
int transferFile(char * pathToFile, int  socketFd);

/**
*@description recieve file from socket and output in stdin
*@param fileLen int file size
*@sockFd int the socket from which to read file
*int recieved bytes num, if errored, return negative number
*/
int recieveFile(size_t fileSize, int sockFd);

/**
*@description get one line at a time from the recv buffer of socket
*@param sockFd  socket description
*@param buffer[] char array to store the char line 
*@return int, the length of recieved line
*/
int getLineFromSock(int sockFd, char* line, size_t size_line);

