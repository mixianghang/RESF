/***********************************************
*
*Filename: httputil.h
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-09-19 22:41:25
*Last Modified: 2015-09-19 22:41:25
************************************************/
#ifndef HTTPUTIL_H
#define HTTPUTIL_H 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include "util.h"

#define MAX_REQUEST_HEADER 128
#define MAX_RESPONSE_HEADER 128
#define MAX_HEADER_NAME 128
#define MAX_HEADER_VALUE 256
#define MAX_FILE_PATH 256
#define MAX_WRITE_BUFFER 256
//http header
typedef struct http_header{
	char  name[MAX_HEADER_NAME];
	char value[MAX_HEADER_VALUE];
} Header;

typedef struct http_request_line{
	char method[MAX_FILE_PATH];
	char path[MAX_FILE_PATH];
	char version[MAX_FILE_PATH];
} RequestLine;

typedef struct http_response_line{
	char version[MAX_FILE_PATH];
	char code[MAX_FILE_PATH];
	char text[MAX_FILE_PATH];
} ResponseLine;

typedef enum KeepAlive{
	persistent,
	no_persistent
} KeepAlive;

//represent http get request info
typedef struct http_request{
	RequestLine requestLine;
	enum KeepAlive isAlive;
	int content_len;
	short isResolved;
	Header * headers[MAX_REQUEST_HEADER];
} Request;

//represent http get response info
typedef struct http_response{
	ResponseLine responseLine;
	enum KeepAlive isAlive;
	int content_len;
	Header * headers[MAX_RESPONSE_HEADER];
} Response;

/**
*@description parse request line from string
*@param str pointer to the request line string
*@param strSize  length of the request line string
*@param line pointer to struct RequestLine
*@return int if failed return negative number, otherwise, return nonnegative number
*/ 
int  parseHeader(const char* requestHeader, size_t sizeHeader, Header * headerStruct);

/**
*@description parse request line from string
*@param requestline pointer to the request line string
*@param linesize  length of the request line string
*@return int if failed return negative number, otherwise, return nonnegative number
*/ 
int parseHttpRequestLine(const char* str, size_t strSize, RequestLine * line);

/**
*@description parse response line from string
*@param responseline pointer to the  line string
*@param linesize  length of the  line string
*@return int if failed return negative number, otherwise, return nonnegative number
*/ 
int parseHttpResponseLine(const char* str, size_t strSize, ResponseLine * line);


/**
*@description form Header
*@param header pointer to  struct Header
*@param str pointer to a header string
*@param strSize the size of the header string
*@return length of header string, if failed, return -1
*/ 
int formHeader(Header * header, char * str, size_t strSize);

/**
*@description send requested file to sock
*@param sockFd int  socket Id
*@param request Request  http_request struct
*@return if failed, return negative number, otherwise, nonnegative
*/
int sendRequestedFile(int sockFd, Request * httpRequest);

/**
*@description download single file from sock using http protocol
*@param filePath char pointer to a filepath
*@param sockFd int  socket Id
*@param isAlive  whether to create persistent connection
*@return if failed, return negative number, otherwise, nonnegative
*/
int downloadSingleFile(char* filePath, int sockFd, enum KeepAlive isAlive);
#endif

