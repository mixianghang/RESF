/***********************************************
*

*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-10-06 07:26:50
*Last Modified: 2015-10-06 07:26:50
************************************************/
#include "http_server.h"

// addrinfo struct
//struct addrinfo {
//  int ai_flags;
//  int ai_family;
//  int ai_socktype;
//  int ai_protocol;
//  size_t ai_addrlen;
//  char * ai_canonname;
//  struct sockaddr * ai_addr;
//  struct addrinfo * ai_next
//};

int main(int argc, char * argv[]) {
  int servSock;
  //struct addrinfo hints, *serverInfo, *index;
  int result;
  int serverPort;
  struct sockaddr_in addr_in;
  socklen_t addr_len;

  if (argc < 3) {
	printf("Usage: ./server port rwnd\n");
	printf("--port: local port number to which the server listen\n"); 
	printf("--rwnd: initial receive window of remote client, must be less than 65536\n");
	return 1;
  }
  serverPort = atoi(argv[1]);
  addr_in.sin_port = htons(serverPort);
  addr_in.sin_family = AF_INET;
  addr_in.sin_addr.s_addr   = INADDR_ANY;
  addr_len = sizeof(struct sockaddr);
  servSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (bind(servSock, (struct sockaddr *) &addr_in, addr_len)  != 0) {
	printf("bind failed to port %s\n", argv[1]);
	return 1;
  } else {
	char ip[20] = {0};
	inet_ntop(AF_INET, &(addr_in.sin_addr), ip,  19);
	printf("bind successfully to %s:%s\n", ip, argv[1]);
  }

  //memset(&hints, 0, sizeof(struct addrinfo));
  //hints.ai_socktype = SOCK_DGRAM;
  //hints.ai_family   = AF_UNSPEC;
  //hints.ai_flags    = AI_PASSIVE;

  ////get address info
  //if ((result = getaddrinfo(NULL, argv[1], &hints, &serverInfo)) != 0) {
  //  fprintf(stderr, "get local address with port %s addrinfo failed\n", argv[1]);
  //  return 1;
  //}


  //for (index = serverInfo; index != NULL; index = index->ai_next) {
  //  if ((servSock = socket(index->ai_family, index->ai_socktype, index->ai_protocol)) == -1) {
  //    //fprintf(stderr, "create socket for  server %s %s failed\n", argv[1], argv[2]);
  //    continue;
  //  }
  //  break;
  //}

  //if (index == NULL) {
  //  fprintf(stderr, "create socket for  port %s failed\n", argv[1]);
  //  return 1;
  //}

  //if ( (result = bind(servSock, index->ai_addr, index->ai_addrlen)) == -1) {
  //  fprintf(stderr, "bind to specific port  %s failed\n", argv[1]);
  //  return 1;
  //} else {
  //  printf("bind to port %s successfully\n", argv[1]);
  //}

  //freeaddrinfo(serverInfo);

  // start to recv and send data
  Resf resf;
  struct sockaddr addr;
  initResf(&resf);
  int rwnd  = atoi(argv[2]);
  if (rwnd <= 0 || rwnd >= 0xFFFF) {
	fprintf(stderr, "wrong rwnd %s\n", argv[2]);
	return 1;
  }
  resf.rwnd = (uint16_t) rwnd;
  resf.sockFd = servSock;
  resf.addr   = &addr;
  resf.recvWaitTime = 0; // no stop, keep waiting and read
  resf.sendWaitTime = 0; // no stop, keep waiting and send
  //resf.cwnd = 65534;
  printf("initial rwnd is %d\n", resf.rwnd);
  startResf(&resf);

  //wait until there are data
  while (1) {
	int  recvLen;
	unsigned  char buffer[1440];
	RequestLine requestLine;
	memset(buffer, 0, sizeof buffer);
	if ((recvLen = recvFromResfBySep(&resf, buffer, sizeof buffer -1, '\n', 0)) < 0) {
	  fprintf(stderr, " recv request error\n");
	  return 1;
	}

	if (recvLen == 0) {
	  usleep(50000);
	  continue;
	}

	//printf("%s %s recv %d data %s\n", __FILE__, __func__, recvLen, buffer);

	if (parseHttpRequestLine(buffer, recvLen - 2, &requestLine) == -1) {
	  fprintf(stderr, " parse request line error\n %s", buffer);
	  return 1;
	}

	if (!checkFileExist(requestLine.path)) {
	  fprintf(stderr, "file %s not exist\n", requestLine.path);
	  return 0;
	}

	int fileSize = getFileSize(requestLine.path);
	struct sockaddr_in * clientAddrIn = (struct sockaddr_in *) resf.addr;
	char clientIp[20] = {0};
	int  clientPort = ntohs(clientAddrIn->sin_port);
    inet_ntop(AF_INET, &(clientAddrIn->sin_addr), clientIp, 19);
	printf(" %s %s start recv request from %s:%d for file %s\n",__FILE__, __func__,  clientIp, clientPort, requestLine.path);
	memset(buffer, 0, sizeof buffer);
	sprintf(buffer, "HTTP/1 200 OK\r\n");
	sprintf(buffer, "%sContent-Length: %d\r\n\r\n", buffer, fileSize);
	if (sendToResf(&resf, buffer, strlen(buffer)) <= 0) {
	  fprintf(stderr, "send response line failed :%s", buffer);
	  return 1;
	}
	//printf("%s %s send %d data %s\n", __FILE__, __func__, strlen(buffer), buffer);
	int len = 0;
	FILE * fp;
	fp = fopen(requestLine.path,"r");
	if (fp == NULL) {
		return 1;
	}
	memset(buffer, 0, sizeof buffer);
	while((len = fread(buffer,sizeof(char), sizeof buffer -1,fp) ) > 0) {
		if (sendToResf(&resf, buffer, len) != len) {
		  fprintf(stderr, "%s %s %d  send to resf failed %s\n",__FILE__, __func__, __LINE__,  buffer);
		  return 1;
		}
		memset(buffer,0 ,sizeof buffer );
	}
	fclose(fp);
	//recv blank line
	memset(buffer, 0, sizeof buffer);
	if ((recvLen = recvFromResfBySep(&resf, buffer, sizeof buffer -1, '\n', 0)) <= 0) {
	  fprintf(stderr, " recv request error\n");
	  return 1;
	}

	//printf("%s %s lastRecved %d lastRead %d lastsent %d lastwrite %d, lastAcked %d\n", __FILE__, __func__, resf.lastRecved, resf.lastRead, resf.lastSent, resf.lastWrite, resf.lastAcked);
	while (resf.lastWrite != resf.lastSent || resf.lastSent != resf.lastWrite || resf.lastAcked != resf.lastSent) {
	  sleep(1);
	//printf("%s %s lastRecved %d lastRead %d lastsent %d lastwrite %d, lastAcked %d\n", __FILE__, __func__, resf.lastRecved, resf.lastRead, resf.lastSent, resf.lastWrite, resf.lastAcked);
	}

	printf("%s %s finish serving request from %s:%d for file %s, transmit %d bytes data\n", __FILE__, __func__, clientIp, clientPort, requestLine.path, fileSize); 

	initResf(&resf);
	resf.rwnd = (uint16_t) rwnd;
	resf.sockFd = servSock;
	resf.addr   = &addr;
	resf.recvWaitTime = 0; // no stop, keep waiting and read
	resf.sendWaitTime = 0; // no stop, keep waiting and send
	resf.cwnd = 65534;
  }

}
