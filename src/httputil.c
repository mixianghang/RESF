/***********************************************
*
*Filename: httputil.c
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-09-19 22:41:07
*Last Modified: 2015-09-19 22:41:07
************************************************/
#include "httputil.h"

int main1(int argc, char * argv[]) {
	
	//test header parse
	char headerStr1[]    = "Content-Length: 348";
	char headerStr2[]    = "Date: Tue, 15 Nov 1994 08:12:31 GMT";
	Header * header1;
	Header * header2;
	header1 = (Header*) malloc(sizeof(Header));
	header2 = (Header*) malloc(sizeof(Header));
	if (header1 != NULL) {
		parseHeader(headerStr1, strlen(headerStr1), header1);
		printf("header1 name is %s\n header1 value is %s\n", header1->name, header1->value);
	}
	if (header2 != NULL) {
		parseHeader(headerStr2, strlen(headerStr2), header2);
		printf("header2 name is %s\n header2 value is %s\n", header2->name, header2->value);
	}
	free(header1);
	free(header2);

	//test request line parse
	char requestLine1[]  = "GET http://www.w3.org/pub/WWW/TheProject.html HTTP/1.1";
	char requestLine2[]  = "GET /pub/WWW/TheProject.html HTTP/1.1";
	RequestLine * line1;
	RequestLine * line2;
	line1 = (RequestLine*) malloc(sizeof(RequestLine));
	line2 = (RequestLine*) malloc(sizeof(RequestLine));
	if (line1) {
		parseHttpRequestLine(requestLine1, strlen(requestLine1), line1);
		printf("request line1's method is %s\n path is %s\n version is %s\n'", line1->method, line1->path, line1->version);
	}
	if (line2) {
		parseHttpRequestLine(requestLine2, strlen(requestLine2), line2);
		printf("request line2's method is %s\n path is %s\n version is %s\n'", line2->method, line2->path, line2->version);
	}
	free(line1);
	free(line2);
	
	//test formHeader
	Header *header;
	char headStr[1024];
	header = (Header*) malloc(sizeof(Header));
	strcpy(header->name,"Content-length");
	strcpy(header->value,"123456");
	formHeader(header,headStr,sizeof(headStr));
	printf("headerStr is %s", headStr);

	return 0;
}

/**
*header format example : Content-Length: 348
*@description parse http request header
*@param requestHeader : string of a header
*@param length of header string
*@param pointer to a header struct
*@return if failed return negative number, otherwise , nonegative number
*/
int parseHeader(const char* requestHeader, size_t sizeHeader, Header * headerStruct) {
	int count    = 0;
	int nameLen  = 0;
	int valueLen = 0;
	memset(headerStruct->name, '\0', MAX_HEADER_NAME);
	memset(headerStruct->value, '\0', MAX_HEADER_VALUE);
	for (nameLen = 0; count < sizeHeader-2; count++) {
		if (requestHeader[count] == ':') {
			count++;//skip the ':'  character
			break;
		} else {
			headerStruct->name[nameLen++] = requestHeader[count];
		}
	}
	if (count >= sizeHeader) {
		return 0;
	} else {
		count++;//skip the whitespace after :
	}
	for (valueLen = 0; count < sizeHeader -2; count++) {
		headerStruct->value[valueLen++] = requestHeader[count];
	}

	if (nameLen == 0 && valueLen == 0) {
		return -1;
	}
	return 1;
}

/**
*@description parse response line from string
*@param responseline pointer to the  line string
*@param linesize  length of the  line string
*@return int if failed return negative number, otherwise, return nonnegative number
*/ 
int parseHttpResponseLine(const char* str, size_t strSize, ResponseLine * line) {
	int versionLen   = 0;
	int codeLen     = 0;
	int textLen  = 0;
	int count       = 0;
	memset(line->version, '\0', sizeof(line->version));
	memset(line->code, '\0', sizeof(line->code));
	memset(line->text, '\0', sizeof(line->text));
	//retrieve response version
	for (versionLen = 0; count < strSize; count++) {
		if (str[count] == ' ') {
			count++;//skip whitespace
			break;
		} else {
			line->version[versionLen++] = str[count];
		}
	}

	for (codeLen = 0; count < strSize; count++) {
		if (str[count] == ' ') {
			count++;//skip whitespace
			break;
		} else {
			line->code[codeLen++] = str[count];
		}
	}

	for (textLen = 0; count < strSize; count++) {
		if (str[count] == ' ') {
			count++;//skip whitespace
			break;
		} else {
			line->text[textLen++] = str[count];
		}
	}

	if (versionLen && codeLen && textLen) {
		return 0;
	} else {
		return -1;
	}
}

/**
*@description parse request line from string
*@param str pointer to the request line string
*@param strSize  length of the request line string
*@param line pointer to struct RequestLine
*@return int if failed return negative number, otherwise, return nonnegative number
*/ 
int parseHttpRequestLine(const char* str, size_t strSize, RequestLine * line){
	int methodLen   = 0;
	int pathLen     = 0;
	int versionLen  = 0;
	int count       = 0;
	memset(line->method, '\0', sizeof(line->method));
	memset(line->path, '\0', sizeof(line->path));
	memset(line->version, '\0', sizeof(line->version));
	//retrieve request method
	for (methodLen = 0; count < strSize; count++) {
		if (str[count] == ' ') {
			count++;//skip whitespace
			break;
		} else {
			line->method[methodLen++] = str[count];
		}
	}

	pathLen = 0;
	line->path[pathLen++] = '.';
	line->path[pathLen++] = '/';
	for (; count < strSize; count++) {
		if (str[count] == ' ') {
			count++;//skip whitespace
			break;
		} else {
			line->path[pathLen++] = str[count];
		}
	}

	for (versionLen = 0; count < strSize; count++) {
		if (str[count] == ' ') {
			count++;//skip whitespace
			break;
		} else {
			line->version[versionLen++] = str[count];
		}
	}

	if (methodLen && pathLen && versionLen) {
		return 0;
	} else {
		return -1;
	}
}


/**
*@description form Header
*@param header pointer to  struct Header
*@param str pointer to a header string
*@param strSize the size of the header string
*@return length of header string, if failed, return -1
*/ 
int formHeader(Header * header, char * str, size_t strSize){
	int count     = 0;
	int lenName   = 0;
	int lenSep    = 0;
	int lenValue  = 0;
	int lenSuffix = 0;
	int catSize   = 0;
	char seperator[] = ": ";
	char suffix[]    = "\r\n";
	memset(str, '\0', strSize);
	//cat name str to headerStr
	lenName = (strSize - count -1) > strlen(header->name) ? strlen(header->name) : (strSize - count -1);
	strncat(str, header->name, lenName);
	count += lenName;
	
	//cat seperator to headerStr
	lenSep = (strSize - count -1) > strlen(seperator) ? strlen(seperator) : (strSize - count-1);
	strncat(str, seperator, lenSep);
	count += lenSep;

	//cat value str to headerStr
	lenValue = (strSize - count -1) > strlen(header->value) ? strlen(header->value) : (strSize - count -1);
	strncat(str, header->value, lenValue);
	count += lenValue;

	//cat  suffix to headerStr
	lenSuffix = (strSize - count -1) > strlen(suffix) ? strlen(suffix) : (strSize - count-1);
	strncat(str, suffix,lenSuffix);
	count += lenSuffix;

	if (lenName && lenSep && lenValue && lenSuffix) {
		return count;
	} else {
		return -1;
	}
}


/**
*@description send requested file to sock
*@param sockFd int  socket Id
*@param request Request  http_request struct
*@return if failed, return negative number, otherwise, nonnegative
*/
int sendRequestedFile(int sockFd, Request * httpRequest) {
	char ok_line[]       = "HTTP/1.0 200 OK\r\n";
	char badReq_line[]   = "HTTP/1.0 400 Bad Request\r\n";  
	char badReq_content[] = "<html><head></head><body><h1>400 Bad Request</h1></body></html>";
	char notFound_line[] = "HTTP/1.0 404 Not Found\r\n";
	char notFound_content[] = "<html><head></head><body><h1>404 Not Found</h1></body></html>";
	char suffix_line[]   = "\r\n";
	char writeBuffer[MAX_WRITE_BUFFER];
	char * responseLine;
	char *filePath = (httpRequest->requestLine).path;
	int fileSize;
	short isResolved = httpRequest->isResolved;
	if ( isResolved ) {
		fileSize = getFileSize(filePath);
		if (fileSize < 0) {
			if (send(sockFd, notFound_line, strlen(notFound_line), 0) < 0) {
				return -1;
			}

			//send content length
			memset(writeBuffer, '\0', sizeof(writeBuffer));
			snprintf(writeBuffer, sizeof(writeBuffer), "%s: %lu\r\n","Content-Length",strlen(notFound_content));
			if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
				return -1;
			}

			//send persistent header
			memset(writeBuffer, '\0', sizeof(writeBuffer));
			if (httpRequest->isAlive == persistent) {
				snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Connection", "keep-alive");
			} else {
				snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Connection", "close");
			}
			if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
				return -1;
			}

			//send content-type header
			memset(writeBuffer, '\0', sizeof(writeBuffer));
			snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Content-Type", "text/html");
			if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
				return -1;
			}
			
			//send Date
			memset(writeBuffer, '\0', sizeof(writeBuffer));
			time_t current = time(0);
			time_t expire  = current + 60;
			struct tm currentTm = *gmtime(&current); 
			struct tm expireTm  = * gmtime(&expire);
			char timeStr[128];
			memset(timeStr, '\0', sizeof(timeStr));
			strftime(timeStr, sizeof(timeStr), "%a, %d %b %Y %H:%M:%S %Z", &currentTm);
			snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Date", timeStr);
			if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
				return -1;
			}

			memset(writeBuffer, '\0', sizeof(writeBuffer));
			memset(timeStr, '\0', sizeof(timeStr));
			strftime(timeStr, sizeof(timeStr), "%a, %d %b %Y %H:%M:%S %Z", &expireTm);
			snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Expires", timeStr);
			if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
				return -1;
			}
			//send suffix line
			if (send(sockFd, suffix_line, strlen(suffix_line), 0) < 0) {
				return -1;
			}
			//send content
			if (send(sockFd, notFound_content, strlen(notFound_content), 0) < 0) {
				return -1;
			}

			return 0;
		} else {

			//send 200 OK status line
			if (send(sockFd, ok_line, strlen(ok_line), 0) < 0) {
				return -1;
			}

			//send content-length header
			memset(writeBuffer, '\0', sizeof(writeBuffer));
			snprintf(writeBuffer, sizeof(writeBuffer), "%s: %d\r\n", "Content-Length", fileSize);
			if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
				return -1;
			}
			
			//send persistent header
			memset(writeBuffer, '\0', sizeof(writeBuffer));
			if (httpRequest->isAlive == persistent) {
				snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Connection", "keep-alive");
			} else {
				snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Connection", "close");
			}
			if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
				return -1;
			}

			//send content-type header
			memset(writeBuffer, '\0', sizeof(writeBuffer));
			snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Content-Type", "text/html");
			if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
				return -1;
			}
			
			//send Date
			memset(writeBuffer, '\0', sizeof(writeBuffer));
			time_t current = time(0);
			time_t expire  = current + 60;
			struct tm currentTm = *gmtime(&current); 
			struct tm expireTm  = * gmtime(&expire);
			char timeStr[128];
			memset(timeStr, '\0', sizeof(timeStr));
			strftime(timeStr, sizeof(timeStr), "%a, %d %b %Y %H:%M:%S %Z", &currentTm);
			snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Date", timeStr);
			if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
				return -1;
			}

			memset(writeBuffer, '\0', sizeof(writeBuffer));
			memset(timeStr, '\0', sizeof(timeStr));
			strftime(timeStr, sizeof(timeStr), "%a, %d %b %Y %H:%M:%S %Z", &expireTm);
			snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Expires", timeStr);
			if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
				return -1;
			}
			//send suffix line
			if (send(sockFd, suffix_line, strlen(suffix_line), 0) < 0) {
				return -1;
			}
			
			//send request body
			if (transferFile(filePath, sockFd) == 0) {
				printf("tranfer file successfully\n");
			} else {
				return -1;
			}
		}
	} else {
		if (send(sockFd, badReq_line, strlen(badReq_line), 0) < 0 ) {
			return -1;
		}
		//send content length
		memset(writeBuffer, '\0', sizeof(writeBuffer));
		snprintf(writeBuffer, sizeof(writeBuffer), "%s: %lu\r\n","Content-Length",strlen(badReq_content));
		if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
			return -1;
		}

		//send persistent header
		memset(writeBuffer, '\0', sizeof(writeBuffer));
		if (httpRequest->isAlive == persistent) {
			snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Connection", "keep-alive");
		} else {
			snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Connection", "close");
		}
		if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
			return -1;
		}

		//send content-type header
		memset(writeBuffer, '\0', sizeof(writeBuffer));
		snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Content-Type", "text/html");
		if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
			return -1;
		}
		
		//send Date
		memset(writeBuffer, '\0', sizeof(writeBuffer));
		time_t current = time(0);
		time_t expire  = current + 60;
		struct tm currentTm = *gmtime(&current); 
		struct tm expireTm  = * gmtime(&expire);
		char timeStr[128];
		memset(timeStr, '\0', sizeof(timeStr));
		strftime(timeStr, sizeof(timeStr), "%a, %d %b %Y %H:%M:%S %Z", &currentTm);
		snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Date", timeStr);
		if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
			return -1;
		}

		memset(writeBuffer, '\0', sizeof(writeBuffer));
		memset(timeStr, '\0', sizeof(timeStr));
		strftime(timeStr, sizeof(timeStr), "%a, %d %b %Y %H:%M:%S %Z", &expireTm);
		snprintf(writeBuffer, sizeof(writeBuffer), "%s: %s\r\n", "Expires", timeStr);
		if (send(sockFd, writeBuffer, strlen(writeBuffer), 0) < 0) {
			return -1;
		}
		//send suffix line
		if (send(sockFd, suffix_line, strlen(suffix_line), 0) < 0) {
			return -1;
		}
		//send content
		if (send(sockFd, notFound_content, strlen(notFound_content), 0) < 0) {
			return -1;
		}
		return 0;
	}

	return 1;
}

/**
*@description download single file from sock using http protocol
*@param filePath char pointer to a filepath
*@param sockFd int  socket Id
*@param isAlive  whether to create persistent connection
*@return if failed, return negative number, otherwise, nonnegative
*/
int downloadSingleFile(char* filePath, int sockFd, enum KeepAlive isAlive) {
	char buffer[512];
	int recvLen = 0;
	//send request line
	memset(buffer, '\0', sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "GET %s HTTP/1.0\r\n", filePath);
	if (send(sockFd, buffer, strlen(buffer), 0) < 0) {
		return -1;
	}

	//send connection type
	memset(buffer, '\0', sizeof(buffer));
	if (isAlive == persistent) {
		snprintf(buffer, sizeof(buffer) - 1, "Connection: keep-alive\r\n");
	} else {
		snprintf(buffer, sizeof(buffer) - 1, "Connection: close\r\n");
	}
	if (send(sockFd, buffer, strlen(buffer), 0) < 0) {
		return -1;
	}

	//send suffix 
	memset(buffer, '\0', sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "\r\n");
	if (send(sockFd, buffer, strlen(buffer), 0) < 0) {
		return -1;
	}

	//start to recieve response
	int numHeaders = 0;
	memset(buffer,'\0',sizeof(buffer));
	Response * response = (Response*) malloc(sizeof(Response));
	response->isAlive = no_persistent;
	response->content_len = 0;
	if (getLineFromSock(sockFd, buffer, sizeof(buffer)-1) > 0) {//read response line
		//parse requestline successfully
		if (parseHttpResponseLine(buffer, strlen(buffer), &(response->responseLine)) >= 0){
			//printf("response code is %s\n", (response->responseLine).code);
			//read each header per line, end by a blank line
			numHeaders = 0;
			memset(buffer,'\0',sizeof(buffer));
			while (getLineFromSock(sockFd, buffer, sizeof(buffer)-1) > 0) { 
				response->headers[numHeaders] = (Header*) malloc(sizeof(Header));
				if ( parseHeader(buffer, strlen(buffer), response->headers[numHeaders]) > 0) {
					//printf("parsed header name %s  value %s\n",response->headers[numHeaders]->name, response->headers[numHeaders]->value);
					// get connect alive status
					if (strcmp(response->headers[numHeaders]->name, "Connection") == 0) {
						if (strcmp(response->headers[numHeaders]->value, "keep-alive") == 0) {
							response->isAlive = persistent;
						} else {
							response->isAlive = no_persistent;
						}
					}
					// get content length
					if (strcmp(response->headers[numHeaders]->name, "Content-Length") == 0) {
						response->content_len = atoi(response->headers[numHeaders]->value);
					}
					memset(buffer,'\0',sizeof(buffer));
					numHeaders++;
				} else {
					free(response->headers[numHeaders]);
					printf("pare header failed: \n%s\n", buffer);
					break;
				}
			}
			
		} 

	}

	if (response->content_len > 0) {
		recvLen = recieveFile(response->content_len, sockFd);
	} else {
		return -1;
	}
	//free dynamically allocated memory
	int i;
	for (i = 0; i< numHeaders; i++) {
		free(response->headers[i]);
	}
	free(response);
	return recvLen;
}

