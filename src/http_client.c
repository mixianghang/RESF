/***********************************************
*
*Filename: http_client.c
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-10-06 06:13:06
*Last Modified: 2015-10-06 06:13:06
************************************************/
#include "http_client.h"


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
  int clientSock;
  //struct addrinfo hints, *serverInfo, *index;
  struct sockaddr_in addr_in;
  int result;
  int serverPort;
  unsigned char buffer[HTTP_CLIENT_BUFFER];
  //unsigned char buffer[65536];
  Resf resf;

  if (argc < 5) {
	printf("Usage: ./client address port rwnd remoterFileName [localFileName]\n");
	printf("--address: ip address of remote server\n");
	printf("--port: port number of the remote server\n"); 
	printf("--rwnd: initial receive window of remote server, must be less than 65536\n");
	printf("--remoteFileName: the request file name\n");
	printf("--localFileName: optional, the name of local file to save received file content\n if not provided, will print the content to console\n");
	return 1;
  }

  serverPort = atoi(argv[2]);
  addr_in.sin_port = htons(serverPort);
  addr_in.sin_family = AF_INET;
  inet_pton(AF_INET, argv[1], &(addr_in.sin_addr));
  clientSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (clientSock <= 0) {
	printf("create sock failed\n");
	return 1;
  }

// the below version seems not stable, but it is compatible with ipv6
  //memset(&hints, 0, sizeof(struct addrinfo));
  //hints.ai_socktype = SOCK_DGRAM;
  //hints.ai_family   = AF_UNSPEC;

  //if ((result = getaddrinfo(argv[1], argv[2], &hints, &serverInfo)) != 0) {
  //  fprintf(stderr, "get server %s %s addrinfo failed\n", argv[1], argv[2]);
  //  return 1;
  //}


  //for (index = serverInfo; index != NULL; index = index->ai_next) {
  //  if ((clientSock = socket(index->ai_family, index->ai_socktype, index->ai_protocol)) == -1) {
  //    //fprintf(stderr, "create socket for  server %s %s failed\n", argv[1], argv[2]);
  //    continue;
  //  }
  //  break;
  //}

  //if (index == NULL) {
  //  fprintf(stderr, "create socket for  server %s %s failed\n", argv[1], argv[2]);
  //  return 1;
  //}

  ////if ( (result = connect(clientSock, index->ai_addr, index->ai_addrlen)) == -1) {
  ////  fprintf(stderr, "connect to server %s %s failed\n", argv[1], argv[2]);
  ////  return 1;
  ////}

  //freeaddrinfo(serverInfo);

  initResf(&resf);
  int rwnd  = atoi(argv[3]);
  if (rwnd <= 0 || rwnd >= 0xFFFF) {
	fprintf(stderr, "wrong rwnd %s\n", argv[3]);
	return 1;
  }
  resf.sockFd = clientSock;
  //resf.addr = index->ai_addr;
  resf.addr = (struct sockaddr *) &addr_in;
  resf.rwnd = (uint16_t) rwnd;
  resf.sendWaitTime = 0;//TODO 
  resf.recvWaitTime = 0;//TODO
  printf("initial rwnd is %d\n", resf.rwnd);
  //resf.cwnd = 65534;
  // start resf send and read thread
  startResf(&resf);
  struct sockaddr_in * server_in = (struct sockaddr_in *) resf.addr;
  int serverport = ntohs(server_in->sin_port);
  char serverIp[20] = {0};
  inet_ntop(AF_INET, &(server_in->sin_addr), serverIp, 19);
  printf("%s %s start request file from server %s:%d for file %s\n", __FILE__, __func__, serverIp, serverport, argv[4]);

  //start to send and recieve
  memset(buffer, 0, sizeof buffer);
  sprintf(buffer,"GET /%s HTTP/1.1\r\n\r\n", argv[4]);
  if ((result = sendToResf(&resf, buffer, strlen(buffer))) != strlen(buffer)) {
	printf("send request failed\n");
	return 1;
  } 


  // recv the response status line
  memset(buffer, 0, sizeof buffer);
  if ((result = recvFromResfBySep(&resf, buffer, (sizeof buffer - 1), '\n', 1)) <= 0) {
	fprintf(stderr, "recv data from server failed\n");
	return 1;
  } 

  int contentLen = -1;
  while ((result = recvFromResfBySep(&resf, buffer, (sizeof buffer) - 1, '\n', 1)) > 0) {
	if (buffer[0] == '\r') {
	  break;
	}
	//printf("recv header %s\n", buffer);
	Header header;
	parseHeader(buffer, result, &header);
	if (strcmp(header.name, "Content-Length") == 0) {
	  contentLen = atoi(header.value);
	}
	memset(buffer, 0, sizeof buffer);
  }

  if (contentLen < 0) {
	fprintf(stderr, "get file failed\n");
	return 1;
  }

  int len = 0;
  int left = contentLen;
  int tempLen;
  //printf("%s %s file %s content  len is %d \n", __FILE__, __func__, argv[4], contentLen);
  FILE* fp = NULL;
  if (argc >= 6) {
	fp = fopen(argv[5], "w+");
  }
  while (left > 0) {
	memset(buffer, 0, sizeof buffer);
	if ( left <= (sizeof buffer -1)) {
	  tempLen = left;
	} else {
	  tempLen = sizeof buffer -1;
	}
	if ((result = recvFromResf(&resf, buffer, tempLen)) < 0) {
	  fprintf(stderr, "read file failed\n");
	  return 1;
	}
	len += result;
	left -= result;
	if (fp != NULL) {
	  fprintf(fp, buffer);
	} else {
	  printf("%s", buffer);
	}
	//printf(" %s\n",buffer);
	//printf("tempLen %d, buffer len %d result %d left %d lastRecv %d lastRead %d\n", tempLen, strlen(buffer), result, left, resf.lastRecved, resf.lastRead);
  }
  if (fp!= NULL) {
	  fclose(fp);
  }

  printf("%s %s recv successfully from  server with dataLen %d\n", __FILE__, __func__, len);
  if (argc >= 6) {
	printf("request file content has been saved into file %s in current directory\n", argv[5]);
  }
  //printf("%s %s lastRecved %d lastRead %d\n", __FILE__, __func__, resf.lastRecved, resf.lastRead);
  while ( resf.lastAcked != resf.lastSent || resf.lastWrite != resf.lastSent || resf.lastRead != resf.lastRecved) {
	sleep(1);
	//printf("%s %s lastRecved %d lastRead %d lastsent %d lastwrite %d, lastAcked %d\n", __FILE__, __func__, resf.lastRecved, resf.lastRead, resf.lastSent, resf.lastWrite, resf.lastAcked);
  }
  return 0;
}
