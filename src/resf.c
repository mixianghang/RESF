/***********************************************
*
*Filename: resf.c
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-10-06 10:22:24
*Last Modified: 2015-10-06 10:22:24
************************************************/
#include "resf.h"

#ifdef __MACH__
#include <sys/time.h>
//clock_gettime is not implemented on OSX
#define CLOCK_REALTIME 1
int clock_gettime(int id /*clk_id*/, struct timespec* t) {
  struct timeval now;
  int rv = gettimeofday(&now, NULL);
  if (rv) {
	return rv;
  }
  t->tv_sec  = now.tv_sec;
  t->tv_nsec = now.tv_usec * 1000;
  return 0;
}
#endif

/*
*@description recv  no more than recvLen bytes, save it to recvBuff before reach a char equal to suffix  
*@description if recv a suffix before recving recvLen data, then stop and save received data into recvBuff
*@param sockFd    int   the socket fd frome where to receive data
*param  suffix    char  stop receiving data when meeting this char
*@param recvBuff  char* store the received data
*@param recvLen   int   the required receive length
*@return    int   -1 if error happens, otherwise the length of received data in bytes 
*/
int recvFromResfBySep(Resf * resf, unsigned char * recvBuff, int recvLen, char suffix, int isTimeout) {
  int len = 0;
  int temp = resf->lastRead;
  int timeInterval = 50000; //microseconds
  int timeNum      = 0;
  int noEmptySpace = 0;
  if (resf->lastRecved >= resf->lastRead) {
	if ((resf->lastRecved - resf->lastRead + 1) >= RECV_BUFFER_SIZE - MAX_SEG_SIZE) {
	  noEmptySpace = 1;
	}
  } else if (resf->lastRead - resf->lastRecved <= MAX_SEG_SIZE) {
	noEmptySpace = 1;
  }
  while ( len < recvLen ) {
	if (temp == resf->lastRecved) {
	  usleep(timeInterval);
	  if (timeNum >= 20 && !isTimeout) { // if no data for 1s, stop and return
		//printf("%s %s %d no data to read for 1s\n", __FILE__, __func__, __LINE__);
		return len;
	  }
	  timeNum++;
	  continue;
	} else {
	  timeNum = 0;
	}
	if (resf->recvBuff[temp] == suffix) {
	  *(recvBuff + len++) = resf->recvBuff[temp];
	  break;
	}
	*(recvBuff + len++) = resf->recvBuff[temp];
	temp++;
	temp = (temp) % RECV_BUFFER_SIZE;
  }
  resf->lastRead = (resf->lastRead + len) % RECV_BUFFER_SIZE;
  if (noEmptySpace == 1) {
	if (resf->lastRecved >= resf->lastRead) {
	  if ((resf->lastRecved - resf->lastRead + 1) <= RECV_BUFFER_SIZE - MAX_SEG_SIZE) {
		pthread_mutex_lock(&(resf->newRecvSpaceMutex));
		pthread_cond_signal(&(resf->newRecvSpaceCond));
		pthread_mutex_unlock(&(resf->newRecvSpaceMutex));
	  }
	} else if (resf->lastRead - resf->lastRecved >= MAX_SEG_SIZE) {
	  pthread_mutex_lock(&(resf->newRecvSpaceMutex));
	  pthread_cond_signal(&(resf->newRecvSpaceCond));
	  pthread_mutex_unlock(&(resf->newRecvSpaceMutex));
	}
  }
  return len;
}

/**
*@description recieve recvLen data, will block when there isn't enough data 
*@param recvBuff  char* store the received data
*@param recvLen   int   the required receive length
*@return    int   -1 if error happens, otherwise the length of received data in bytes 
*/
int recvFromResf(Resf *resf, unsigned char * recvBuff, int recvLen) {
  //printf("%s %s lastRecved %d lastRead %d\n", __FILE__, __func__, resf->lastRecved, resf->lastRead);
  int len = 0;
  int temp = resf->lastRead;
  int timeInterval = 50000; //microseconds
  int timeNum      = 0;
  while ( len < recvLen) {
	if (temp == resf->lastRecved) {
	  usleep(timeInterval);
	  if (timeNum >= 20) {
		resf->lastRead  = (resf->lastRead + len) % RECV_BUFFER_SIZE;
		printf("%s %s lastRecved %d lastRead %d\n", __FILE__, __func__, resf->lastRecved, resf->lastRead);
		return len;
	  }
	  timeNum++;
	  continue;
	} else {
	  timeNum = 0;
	}
	*(recvBuff + len) = resf->recvBuff[temp];
	temp++;
	temp = temp % RECV_BUFFER_SIZE;
	len++;
  }
  resf->lastRead  = (resf->lastRead + len) % RECV_BUFFER_SIZE;
  //printf("%s %s lastRecved %d lastRead %d\n", __FILE__, __func__, resf->lastRecved, resf->lastRead);
  return len;
}

/**
*@description send sendLen data from sendBuffer to socket sockFd
*@param sockFd    int   the socket fd frome where to receive data
*@param sendBuff  char* store data to be sent
*@param sendLen   int   the length of the data to be sent
*@return int       -1 if error happens, otherwise the length of sent data
*/
int sendToResf(Resf * resf, const unsigned  char * sendBuff, int sendLen) {
  int len  = 0;
  int temp = resf->lastWrite;
  int timeInterval = 100000; // microseconds
  int timeNums     = 0;
  int isNoData     = 0;
  int canSendLen   = 0;
  if (sendLen > SEND_BUFFER_SIZE) {
	printf("cannot send so many data");
	return -1;
  }
  while (1) {
	if ( resf->lastWrite > resf->lastAcked) {
	  canSendLen = SEND_BUFFER_SIZE - (resf->lastWrite - resf->lastAcked) -1;
	} else if (resf->lastWrite < resf->lastAcked){
	  canSendLen = resf->lastAcked - resf->lastWrite -1;
	} else if (resf->lastWrite == resf->lastAcked) {
	  canSendLen = SEND_BUFFER_SIZE -1;
	}
	//printf("%s %s can send length %d sendLen %d\n lastAcked %d lastSent %d lastWrite %d\n", __FILE__, __func__, canSendLen, sendLen, resf->lastAcked, resf->lastSent, resf->lastWrite);
	if (canSendLen < sendLen) {
	  usleep(timeInterval);
	  continue;
	} else {
	  break;
	}
  }
  if (resf->lastSent == resf->lastWrite) {
	isNoData = 1;
  }
  while (len < sendLen && len < canSendLen) {
	int clear = (resf->lastAcked == resf->lastSent) && (resf->lastSent == resf->lastWrite);
	if (temp == resf->lastAcked && !clear) {
	  usleep(timeInterval);
	  timeNums++;
	  if (timeNums >= 20) {
		printf("%s %s send to resf timeout %d\n",__FILE__, __func__,  timeNums * timeInterval);
		return len;
	  }
	  continue;
	} else {
	  timeNums = 0;
	}
	resf->sendBuff[temp]  = *(sendBuff + len++);
	temp++;
	temp = temp % SEND_BUFFER_SIZE;
	resf->lastWrite = temp;
  }
  if (isNoData && len > 0) { // notify have new data to send
	pthread_mutex_lock(&(resf->unsentDataMutex));
	pthread_cond_signal(&(resf->unsentDataCond));
	pthread_mutex_unlock(&(resf->unsentDataMutex));
  }
  //printf("%s %s  lastWrite %d lastSent %d lastAcked %d sentLen %d\n", __FILE__, __func__, resf->lastWrite,
 //resf->lastSent, resf->lastAcked, len);
  return len;
}

//
//typedef enum congestion {
//  SLOW,//slow start
//  AVOID,//congestion avoidance
//  RECOV,//fast recovery
//} congestStatus;
//
//typedef struct fragment {
//  int start;
//  int end;
//  int len;
//  struct fragment * next;
//}
//
//typedef struct resf {
//  // variables used to calculate and store timeout
//  uint32_t estimatedRtt;
//  uint32_t devRtt;
//  uint32_t sampSeqNum;// used to get the sample rtt
//  uint32_t sampleSentTime;// when received the sample ack, calculate the sampleRTT
//  uint32_t timeout;
//
//  uint32_t rwnd;// receive window size
//  uint32_t cwnd;// congestion windown size
//  uint32_t lastSent;// the position of last sent
//  uint32_t lastAcked;
//  congestStatus cstatus;
//  fragment * recvFrag;
//  int sockFd;//the sock fd from where to read and to where to send
//  struct sockaddr * addr;// the address of the other side of the sock connection
//
//  char sendBuff[SEND_BUFFER_SIZE];
//  char recvBuff[RECV_BUFFER_SIZE];
//
//  pthread_t * sendThread;
//  pthread_t * recvThread;
//} Resf;

/**
*@description read from sendbuffer and send to sock
*@param resf Resf 
*/
void * startSend( void * voidresf) {
  Resf * resf = (Resf *) voidresf;
  void * returnVoid;
  int timeSum = 0;
  //check if send address is set
  if (resf->sockFd == -1 || !(resf->addr)) {
	printf("no sockfd or addr when start to send\n");
	return returnVoid;
  }
  while(1) {
   // no data to ack, no data to send, wait for new data
    if (resf->lastSent == resf->lastWrite && resf->lastSent == resf->lastAcked) {
	  struct timespec now; 
	  clock_gettime(CLOCK_REALTIME,&now);
	  now.tv_nsec += resf->sendWaitTimeInterval * 1000000; // ms to ns
	  pthread_mutex_lock(&(resf->unsentDataMutex));
	  pthread_cond_timedwait(&(resf->unsentDataCond), &(resf->unsentDataMutex), &now);
	  pthread_mutex_unlock(&(resf->unsentDataMutex));
	  timeSum += resf->sendWaitTimeInterval;
	  if (resf->lastSent == resf->lastWrite && resf->lastSent == resf->lastAcked) {
		if (resf->sendWaitTime && timeSum >= resf->sendWaitTime) {
		  printf("%s %s no data to send or ack for %d ms\n",__FILE__, __func__, timeSum); 
		  return returnVoid;
		}
	  } else {
		timeSum = 0;
	  }
	  continue;
	}

	// check if we can send data right now.
	if (prepareAndSend(resf) != 0) {
	  struct sockaddr_in * addrin = (struct sockaddr_in *) resf->addr;
	  int port = ntohs(addrin->sin_port);
	  char ip[16];
	  memset(ip, 0, sizeof ip);
	  inet_ntop(AF_INET, &(addrin->sin_addr), ip, 15);
	  printf("%s %s prepare and send failed %s %d\n", __FILE__, __func__, ip, port);
	  return returnVoid;
	}
	//check timeout and send
	timeout(resf);
	//check duplicate acks and take action
	duplicateAcks(resf);
  }
}

// prepare and send
int prepareAndSend(Resf * resf) {
  pthread_mutex_lock(&(resf->sendNewDataMutex));
// check if we can send data right now.
  int leftWnd;// some windown size is used by unacked data
  int wndLimit;
  int unSentLen;
  int canSendLen;
  unsigned char bytes[MAX_SEG_SIZE + 20];
  unsigned char segData[MAX_SEG_SIZE];

  wndLimit = (resf->rwnd < resf->cwnd) ? resf->rwnd : resf->cwnd;
  if (resf->lastSent >= resf->lastAcked) {
	leftWnd = wndLimit - (resf->lastSent - resf->lastAcked);
  } else {
	leftWnd = wndLimit - (SEND_BUFFER_SIZE - resf->lastAcked + resf->lastSent);
  }
  //calculate the constrain of leftwnd and left unsent data
  if (resf->lastSent <= resf->lastWrite) {
	unSentLen = resf->lastWrite - resf->lastSent;
  } else {
	unSentLen = resf->lastWrite + (SEND_BUFFER_SIZE - resf->lastSent);
  }
  if (unSentLen > leftWnd) {// constrained by leftWnd
	canSendLen = leftWnd;
  } else {
	canSendLen = unSentLen;
  }
  //printf("rwnd %d windown limit is %d unsentLen %d lastSent %d lastAcked %d lastWrite %d cwnd %d\n", resf->rwnd, leftWnd, unSentLen, resf->lastSent, resf->lastAcked, resf->lastWrite, resf->cwnd);
  
  // have data to send, have window to use, so start to send
  if (canSendLen > 0) {
	int len = canSendLen;
	int segLen = 0;
	// set sample segment to calculate the rtt
	gettimeofday(&(resf->sampleSentTime), NULL);
	resf->sampleSeqNum = resf->lastSent;
	while (len > 0) {
	  memset(segData, 0, sizeof segData);
	  if (len > MAX_SEG_SIZE) {
		segLen = MAX_SEG_SIZE;
	  } else {
		segLen = len;
	  }
	  int i = 0;
	  while (i < segLen) {
		segData[i] = resf->sendBuff[(resf->lastSent + i) % SEND_BUFFER_SIZE];
		i++;
	  }
	  segment seg;
	  seg.seqNum = resf->lastSent;
	  seg.dataLen = segLen;
	  seg.ackNum  = 0;
	  if (resf->lastRecved >= resf->lastRead) {
		seg.rwnd = RECV_BUFFER_SIZE - (resf->lastRecved - resf->lastRead) -1;
	  } else {
		seg.rwnd = resf->lastRead - resf->lastRecved -1;
	  }
	  seg.flags  = 0;
	  seg.data    = segData;
	  memset(bytes, 0, sizeof bytes);
	  int result = packSegment(&seg, bytes);
	  if (result <= 0) {
		return 1;
	  }
	//printf("%s %s before sendtoSock, seqNum %d len %d lastSent %d lastWrite %d lastAcked %d lastRecved %d lastRead %d rwnd header %d\n", __FILE__, __func__, seg.seqNum, result, resf->lastSent, resf->lastWrite, resf->lastAcked, resf->lastRecved, resf->lastRead, seg.rwnd);
	  if (sendToSock(resf->sockFd, resf->addr, bytes, result) > 0) {
		resf->lastSent = (resf->lastSent + segLen) % SEND_BUFFER_SIZE;
		len -= segLen;
		//printf("%s %s lastWrite %d lastSent %d lastAcked %d len %d\n", __FILE__, __func__, resf->lastWrite,
		//resf->lastSent, resf->lastAcked, segLen);
		//printf("%s %s successfully sent to sock some data seqNum %d len %d lastSent %d lastWrite %d lastAcked %d canSentLen %d\n", __FILE__, __func__, seg.seqNum, result, resf->lastSent, resf->lastWrite, resf->lastAcked, canSendLen);
	  } else {
		printf("%s %s  failed send to sock some data seqNum %d len %d lastSent %d lastWrite %d lastAcked %d canSentLen %d\n", __FILE__, __func__, seg.seqNum, result, resf->lastSent, resf->lastWrite, resf->lastAcked, canSendLen);
		return 1;
	  }
	}
  }

  pthread_mutex_unlock(&(resf->sendNewDataMutex));
  return 0;
}


//prepare and send one with ack 
int sendDataWithAck(Resf *resf, uint32_t ackNum) {
  pthread_mutex_lock(&(resf->sendNewDataMutex));
// check if we can send data right now.
  int leftWnd;// some windown size is used by unacked data
  int wndLimit;
  int unSentLen;
  int canSendLen;
  unsigned char bytes[MAX_SEG_SIZE + 20];
  unsigned char segData[MAX_SEG_SIZE];

  wndLimit = (resf->rwnd < resf->cwnd) ? resf->rwnd : resf->cwnd;
  if (resf->lastSent >= resf->lastAcked) {
	leftWnd = wndLimit - (resf->lastSent - resf->lastAcked);
  } else {
	leftWnd = wndLimit - (SEND_BUFFER_SIZE - resf->lastAcked + resf->lastSent);
  }
  //calculate the constrain of leftwnd and left unsent data
  if (resf->lastSent <= resf->lastWrite) {
	unSentLen = resf->lastWrite - resf->lastSent;
  } else {
	unSentLen = resf->lastWrite + (SEND_BUFFER_SIZE - resf->lastSent);
  }
  if (unSentLen > leftWnd) {// constrained by leftWnd
	canSendLen = leftWnd;
  } else {
	canSendLen = unSentLen;
  }
  
  // have data to send, have window to use, so start to send
  if (canSendLen > 0) {
	int len = canSendLen;
	int segLen = 0;
	memset(bytes, 0, sizeof bytes);
	memset(segData, 0, sizeof segData);
	if (len > MAX_SEG_SIZE) {
	  segLen = MAX_SEG_SIZE;
	} else {
	  segLen = len;
	}
	int i = 0;
	while (i < segLen) {
	  segData[i] = resf->sendBuff[(resf->lastSent + i) % SEND_BUFFER_SIZE];
	  i++;
	}
	segment seg;
	seg.seqNum = resf->lastSent;
	seg.dataLen = segLen;
	seg.ackNum  = ackNum;
	if (resf->lastRecved >= resf->lastRead) {
	  seg.rwnd = RECV_BUFFER_SIZE - (resf->lastRecved - resf->lastRead) -1;
	} else {
	  seg.rwnd = resf->lastRead - resf->lastRecved -1;
	}
	seg.flags  = 0x01U;
	seg.data    = segData;
	int result = packSegment(&seg, bytes);
	//printf("%s %s before sendtoSock, seqNum %d len %d lastSent %d lastWrite %d lastAcked %d lastRecved %d lastRead %d rwnd header %d\n", __FILE__, __func__, seg.seqNum, result, resf->lastSent, resf->lastWrite, resf->lastAcked, resf->lastRecved, resf->lastRead, seg.rwnd);
	if (sendToSock(resf->sockFd, resf->addr, bytes, result) > 0) {
	  resf->lastSent = (resf->lastSent + segLen) % SEND_BUFFER_SIZE;
	  len -= segLen;
	  pthread_mutex_unlock(&(resf->sendNewDataMutex));
	  return 0;
	}
  }

  pthread_mutex_unlock(&(resf->sendNewDataMutex));
  return 1;
}

// check whether time out and  what we should do for timeout
int timeout(Resf * resf) {
  struct timespec now;
  struct timespec *start = &(resf->timeoutStart);
  clock_gettime(CLOCK_REALTIME, &now);
  int istimeout = (now.tv_sec - start->tv_sec) * 1000000 + (now.tv_nsec - start->tv_nsec) / 1000 - resf->timeout;
  // check if it is timeout
  if (istimeout >= 0) {
	if (retransmit(resf) == 0) {
	  clock_gettime(CLOCK_REALTIME, &(resf->timeoutStart));
	  resf->timeout *= 2;
	}
	congestWhenTimeout(resf);
  }
  return 0;
}


// check three duplicates acks
int duplicateAcks(Resf * resf) {
  if (resf->dupAckNum >= 3 ) {
	  if ( retransmit(resf) == 0) {
		  congestWhenDup3(resf);
		  timeoutWhenDup(resf);
		  resf->dupAckNum = 0;
		  return 0;
	  } else {
		return 1;
	  }
  }
  return 0;
}

// congestion control when duplicate 3
int congestWhenDup3(Resf * resf) {
  char status[100] = {0};
  if (resf->cstatus == SLOW) {
	resf->ssthresh = resf->cwnd / 2;
	resf->cwnd     = resf->ssthresh + 3 * MAX_SEG_SIZE;
	resf->cstatus = RECOV;
  } else if (resf->cstatus ==  AVOID) {
	resf->ssthresh = resf->cwnd / 2;
	resf->cwnd     = resf->ssthresh + 3 * MAX_SEG_SIZE;
	resf->cstatus = RECOV;
  } else if (resf->cstatus == RECOV) {
  }
  memset(status, 0, sizeof status);
  getCongestStatus(status, resf->cstatus);
  //printf("%s %s cwnd %d ssthresh %d cwnd status %s\n",__FILE__, __func__, resf->cwnd,resf->ssthresh, status);
  return 0;
}

//congestWhen receive a new duplicate
int congestWhenNDup(Resf *resf) {
  char status[100] = {0};
  //getCongestStatus(status, resf->cstatus);
  //printf("%s %s before cwnd %d cwnd status %s \n",__FILE__, __func__, resf->cwnd, status);
  // dupNUm will add 1, but this operation will be done in the receive thread
  if (resf->cstatus == SLOW) {
  } else if (resf->cstatus ==  AVOID) {
  } else if (resf->cstatus == RECOV) {
	resf->cwnd += MAX_SEG_SIZE;
  }
  memset(status, 0, sizeof status);
  getCongestStatus(status, resf->cstatus);
  //printf("%s %s cwnd %d ssthresh %d cwnd status %s\n",__FILE__, __func__, resf->cwnd,resf->ssthresh, status);
  return 0;
}

// congestion control when timeout
int congestWhenTimeout(Resf *resf) {
  char status[100] = {0};
  //getCongestStatus(status, resf->cstatus);
  //printf("%s %s before cwnd %d cwnd status %s \n",__FILE__, __func__, resf->cwnd, status);
  if (resf->cstatus == SLOW) {
	resf->ssthresh = resf->cwnd / 2;
	resf->cwnd     = MAX_SEG_SIZE;
  } else if (resf->cstatus ==  AVOID) {
	resf->ssthresh = resf->cwnd / 2;
	resf->cwnd     = MAX_SEG_SIZE;
	resf->cstatus  = SLOW;
  } else if (resf->cstatus == RECOV) {
	resf->ssthresh = resf->cwnd / 2;
	resf->cwnd     = MAX_SEG_SIZE;
	resf->cstatus  = SLOW;
  }
  memset(status, 0, sizeof status);
  getCongestStatus(status, resf->cstatus);
  //printf("%s %s cwnd %d ssthresh %d cwnd status %s\n",__FILE__, __func__, resf->cwnd,resf->ssthresh, status);
  //printf("%s %s after cwnd %d cwnd status %s\n",__FILE__, __func__, resf->cwnd, status);
  return 0;
}

// congestion control when new Ack
int congestWhenNAck(Resf *resf) {
  char status[100] = {0};
  if (resf->cstatus == SLOW) {
	//printf("%s %s before new ack, slow start cwnd %d \n", __FILE__, __func__, resf->cwnd);
	resf->cwnd += MAX_SEG_SIZE;
	if (resf->cwnd >= resf->ssthresh) {
	  resf->cstatus = AVOID;
	}
  } else if (resf->cstatus ==  AVOID) {
	//printf("%s %s before new ack,congest avoid cwnd %d \n", __FILE__, __func__, resf->cwnd);
	if (resf->rwnd <= 0) {
	  return -1;
	}
	double rwnd = (double) resf->rwnd;
	resf->cwnd += (uint32_t) ( MAX_SEG_SIZE * MAX_SEG_SIZE / rwnd);
  } else if (resf->cstatus == RECOV) {
	resf->cwnd = resf->ssthresh;
	resf->cstatus = AVOID;
  }

  memset(status, 0, sizeof status);
  getCongestStatus(status, resf->cstatus);
  //printf("%s %s cwnd %d ssthresh %d cwnd status %s\n",__FILE__, __func__, resf->cwnd,resf->ssthresh, status);
  //printf("%s %s after new ack, cwnd %d  %d \n", __FILE__, __func__, resf->cwnd, resf->ssthresh);

  return 0;
}

//timeout change when duplicate 3
int timeoutWhenDup(Resf *resf) {
  clock_gettime(CLOCK_REALTIME,&(resf->timeoutStart));
  return 0;
}
/**
*@description read from sock and save to recvbuffer
*@param resf Resf 
*/
void * startRecv( void * resfvoid) {
  void * returnVoid;
  Resf * resf = (Resf *) resfvoid;
  struct timeval waitTime;
  int timeSum  = 0;//milisecond
  int timeInterval = resf->recvWaitTimeInterval; //milisecond
  const int MAX_NO_DATA_TIMES = 20;
  int maxFd = resf->sockFd + 1;
  unsigned char segBuff[MAX_SEG_SIZE + 100];
  int  segLen;
  fd_set orignFds;
  fd_set tempFds;
  waitTime.tv_sec = 0;
  waitTime.tv_usec  = timeInterval * 1000; //wait for 50ms to read new tata
  FD_ZERO(&orignFds);
  FD_SET(resf->sockFd, &orignFds);

  while(1) {
	  //printf("i am receiving \n");
	segment seg;
	// check whether the sock has data to read
	tempFds = orignFds;
	select(maxFd, &tempFds, NULL, NULL, &waitTime);
	if (!FD_ISSET(resf->sockFd, &tempFds)) {
	  timeSum += timeInterval; // no data to read
	  if (resf->recvWaitTime && timeSum >= resf->recvWaitTime) {
		//printf("%s %s no data to recv from sock for %d\n",__FILE__, __func__,  timeSum);
		return returnVoid;
	  }
	  continue;
	} else {
	  timeSum = 0; // clear the record
	}

	//recvSegment
	memset(segBuff, 0, sizeof segBuff);
	segLen = recvFromSock(resf->sockFd, resf->addr, segBuff, sizeof segBuff -1);
	if (segLen <= 0 || unpackSegment(&seg, segBuff, segLen) != 0) {
	  //printf("%s %s recv data from sock failed, %d \n", __FILE__, __func__, segLen);
	  continue;
	} else {
	  printf("%s %s recv from sock %d\n %s", __FILE__, __func__, segLen, segBuff + 13);
	}
	//printf("%s %s new segment recved seq %d ack %d dataLen %d rwnd %d flag %d lastAcked %d lastSent %d lastWrite %d lastRecved %d lastRead %d\n", __FILE__, __func__, seg.seqNum, seg.ackNum, seg.dataLen, seg.rwnd, seg.flags, resf->lastAcked, resf->lastSent, resf->lastWrite, resf->lastRecved, resf->lastRead);

	if (seg.rwnd >= 0) {
	  //printf("%s %s Line %d:  before refresh rwnd %d\n", __FILE__, __func__, __LINE__, resf->rwnd);
	  resf->rwnd = seg.rwnd;
	  //printf("%s %s Line %d: after refresh rwnd %d\n", __FILE__, __func__, __LINE__, resf->rwnd);
	}
	if (seg.flags & 0x01) { // have ack flag set
	  //printf("%s %s is start %d acknowledge, lastacked %d lastSent %d\n", __FILE__, __func__, seg.ackNum, resf->lastAcked, resf->lastSent);
	  if (seg.ackNum == resf->lastAcked) {// duplicate acks
		resf->dupAckNum++;
		congestWhenNDup(resf);
	  } else {
		if (resf->lastSent >= resf->lastAcked) {
		  if ( seg.ackNum > resf->lastAcked && seg.ackNum <= resf->lastSent) {
			calculateRtt(seg.ackNum, resf);
			resf->lastAcked = seg.ackNum;
			clock_gettime(CLOCK_REALTIME, &(resf->timeoutStart));
			//printf("before reset, timeout is %d, estimateRtt %d devRtt %d\n", resf->timeout, resf->estimateRtt, resf->devRtt);
			calculateNewTimeout(resf);
			//printf("after reset, timeout is %d, estimateRtt %d devRtt %d\n", resf->timeout, resf->estimateRtt, resf->devRtt);
			congestWhenNAck(resf);
		  }
		} else {
		  if (seg.ackNum > resf->lastAcked || seg.ackNum <= resf->lastSent) {
			calculateRtt(seg.ackNum, resf);
			resf->lastAcked = seg.ackNum;
			clock_gettime(CLOCK_REALTIME, &(resf->timeoutStart));
			calculateNewTimeout(resf);
			congestWhenNAck(resf);
		  }
		}
	  }
	}

	//if has data
	if (seg.dataLen > 0) {
	  //printf("%s %s dataLen is %d seqNum %d lastRecved %d\n" , __FILE__, __func__, seg.dataLen, seg.seqNum, resf->lastRecved);
	  if (seg.seqNum == resf->lastRecved) { // the data we really need
		// check empty buffer size
		int canRecvLen = 0;
		if ( resf->lastRecved >= resf->lastRead) {
		  canRecvLen = RECV_BUFFER_SIZE - (resf->lastRecved - resf->lastRead) -1;
		} else {
		  canRecvLen = resf->lastRead - resf->lastRecved -1;
		}
		while (canRecvLen < seg.dataLen) {
		  printf("%s %s no space to recv, so wait \n", __FILE__, __func__);
		  struct timespec now; 
		  clock_gettime(CLOCK_REALTIME,&now);
		  now.tv_nsec += resf->recvWaitTimeInterval * 1000000; // ms to ns
		  pthread_mutex_lock(&(resf->newRecvSpaceMutex));
		  pthread_cond_timedwait(&(resf->newRecvSpaceCond), &(resf->newRecvSpaceMutex), &now);
		  pthread_mutex_unlock(&(resf->newRecvSpaceMutex));
		  if ( resf->lastRecved >= resf->lastRead) {
			canRecvLen = RECV_BUFFER_SIZE - (resf->lastRecved - resf->lastRead) -1;
		  } else {
			canRecvLen = resf->lastRead - resf->lastRecved -1;
		  }
		}
		int i = 0;
		while (i < seg.dataLen && i< canRecvLen) {
		  resf->recvBuff[(resf->lastRecved + i) % RECV_BUFFER_SIZE] = seg.data[i];
		  i++;
		}
		resf->lastRecved = (resf->lastRecved + i) % RECV_BUFFER_SIZE;
		//printf("%s %s recv some data successfully datalen %d lastRecved %d lastRead %d\n", __FILE__, __func__, seg.dataLen, resf->lastRecved, resf->lastRead);
		if (sendDataWithAck(resf, resf->lastRecved) == 0) {
		 // printf("%s %s send data with Ack successfully\n", __FILE__, __func__);
		  //do nothing
		} else { // send only ack
		  segment ackSeg;
		  unsigned char header[20];
		  int ackLen = 0;
		  ackSeg.ackNum  = resf->lastRecved;
		  ackSeg.dataLen = 0;
		  ackSeg.flags   = 0x01U;
		  if (resf->lastRecved >= resf->lastRead) {
			ackSeg.rwnd = RECV_BUFFER_SIZE - (resf->lastRecved - resf->lastRead) -1;
		  } else {
			ackSeg.rwnd = resf->lastRead - resf->lastRecved -1;
		  }
		  ackLen = packSegment(&ackSeg, header);
	//printf("%s %s before sendtoSock, seqNum %d len %d lastSent %d lastWrite %d lastAcked %d lastRecved %d lastRead %d rwnd header %d\n", __FILE__, __func__, ackSeg.seqNum, ackLen, resf->lastSent, resf->lastWrite, resf->lastAcked, resf->lastRecved, resf->lastRead, ackSeg.rwnd);
		  if (sendToSock(resf->sockFd, resf->addr, header, ackLen) != ackLen) {
			//printf("%s %s send only Ack failed\n", __FILE__, __func__);
			return returnVoid;
		  } else {
			//printf("%s %s send only Ack successfully, ackNum %d rwnd %d\n", __FILE__, __func__, ackSeg.ackNum, ackSeg.rwnd);
		  }
		}
	  } else { // TODO throw away or somethin else
	  // no.1 old data, send ack
	  // no.2 uncumulative data, store / store and selective ack
	  }
	//printf("%s %s after process new segment lastAcked %d lastSent %d lastWrite %d lastRecved %d lastRead %d timeout %d dupliNUm %d\n", __FILE__, __func__, resf->lastAcked, resf->lastSent, resf->lastWrite, resf->lastRecved, resf->lastRead, resf->timeout, resf->dupAckNum);
	}
  }
  return returnVoid;
}


/**
*initialize resf
*resf Resf pointer
*/
void initResf(Resf * resf) {
  resf->cwnd        = MAX_SEGMENT_DATA_SIZE; 
  resf->ssthresh    = 64 << 10;
  resf->timeout		= 1000000;
  resf->estimateRtt = 0;
  resf->devRtt      = 0;
  resf->lastSent    = 0;
  resf->lastAcked	= 0;
  resf->lastWrite	= 0;
  resf->lastRead	= 0;
  resf->lastRecved  = 0;
  resf->recvFrag    = NULL;
  resf->cstatus     = SLOW; 
  resf->sockFd      = -1;
  resf->addr        = NULL;
  resf->dupAckNum   = 0;
  resf->recvWaitTime = 60000;//miliseconds , default 1 minute
  resf->sendWaitTime = 60000;//miliseconds , default 1 minute
  resf->recvWaitTimeInterval = 100;//miliseconds
  resf->sendWaitTimeInterval = 100;//miliseconds;
  return ;
}

/**
*reinitialize resf
*resf Resf pointer
*/
void reinitResf(Resf * resf) {
  resf->cwnd        = MAX_SEGMENT_DATA_SIZE; 
  resf->ssthresh    = 64 << 10;
  resf->timeout		= 1000000;
  resf->devRtt		= 0;
  resf->lastSent    = 0;
  resf->lastAcked	= 0;
  resf->lastWrite	= 0;
  resf->lastRead	= 0;
  resf->lastRecved  = 0;
  resf->recvFrag    = NULL;
  resf->cstatus     = SLOW; 
  //resf->sockFd      = -1;
  //resf->addr        = NULL;
  resf->dupAckNum   = 0;
  resf->recvWaitTime = 60000;//miliseconds , default 1 minute
  resf->sendWaitTime = 60000;//miliseconds , default 1 minute
  resf->recvWaitTimeInterval = 100;//miliseconds
  resf->sendWaitTimeInterval = 100;//miliseconds;
  return ;
}


/**
*@description set up the read and write threads which are used to read and write to sock
*@ resf struct resf configuration and variables used by those two threads
*@return 
*/
int startResf(Resf * resf) {
  if (pthread_mutex_init(&(resf->unsentDataMutex), NULL)) {// initialize a mutex
	return 1;
  }
  if (pthread_cond_init(&(resf->unsentDataCond), NULL)) {// initialize a condition
	return 1;
  }

  if (pthread_create(&(resf->sendThread), NULL, startSend, (void *)resf)) {// create send thread
	return 1;
  }
  if (pthread_create(&(resf->recvThread), NULL, startRecv, (void *)resf)) { // create recvThread
	return 1;
  }

  return 0;
}

//retransmit the oldest 
int retransmit(Resf *resf) {
  int dataLen;
  int unAckedLen;
  segment seg;
  unsigned char data[MAX_SEG_SIZE];
  unsigned char segData[MAX_SEG_SIZE + 20];
  if (resf->lastSent < resf->lastAcked) {
	unAckedLen = resf->lastSent + SEND_BUFFER_SIZE - resf->lastAcked;
  } else {
	unAckedLen = resf->lastSent - resf->lastAcked;
  }
  if ( unAckedLen > 0) {
	if (unAckedLen > MAX_SEG_SIZE) {
	  dataLen = MAX_SEG_SIZE;
	} else {
	  dataLen = unAckedLen;
	}
	seg.seqNum = resf->lastAcked;
	seg.ackNum = 0;
	if (resf->lastRecved >= resf->lastRead) {
	  seg.rwnd = RECV_BUFFER_SIZE - (resf->lastRecved - resf->lastRead) -1;
	} else {
	  seg.rwnd = resf->lastRead - resf->lastRecved -1;
	}
	seg.flags  = 0;
	seg.dataLen = dataLen;
	int i = 0;
	while(i < dataLen) {
	  data[i] = resf->sendBuff[(resf->lastSent + i) % SEND_BUFFER_SIZE];
	  i++;
	}
	seg.data = data;
	i = packSegment(&seg, segData);
	//printf("%s %s before sendtoSock, seqNum %d len %d lastSent %d lastWrite %d lastAcked %d lastRecved %d lastRead %d rwnd header %d\n", __FILE__, __func__, seg.seqNum, i, resf->lastSent, resf->lastWrite, resf->lastAcked, resf->lastRecved, resf->lastRead, seg.rwnd);
	if (sendToSock(resf->sockFd, resf->addr, segData, i) == i) {
	  //printf("%s %s retransmit successfully seqnum %d datalen %d lastAcked %d lastWrite %d lastSent %d\n", __FILE__, __func__, seg.seqNum, seg.dataLen, resf->lastAcked, resf->lastWrite, resf->lastSent);
	  return 0;
	} else {
	  return 1;
	}
  }
  return -1;
}

/**
*@description send bytes to sock
*@param resf resf 
*/
int  sendToSock(int sockFd, struct sockaddr * addr, const unsigned  char * bytes, int len) {
  int sentLen = 0;
  int tempLen = 0;
  int leftLen = len;
  const char * pos  = bytes;
  socklen_t addrLen = sizeof(struct sockaddr);
  while (leftLen) {
	if ((tempLen = sendto(sockFd, pos, leftLen, 0, addr, addrLen)) >=  0) {
	  sentLen += tempLen;
	  leftLen -= tempLen;
	  pos     += tempLen;
	} else {
	  printf("failed to sendToSock\n");
	  break;
	}
  }
  return sentLen;
}

/**
*@description send bytes to sock
*@param resf resf 
*/
int  recvFromSock(int sockFd, struct sockaddr * addr,  unsigned char * bytes, int len) {
  int recvLen;
  socklen_t addrLen = sizeof(struct sockaddr);
  if ((recvLen = recvfrom(sockFd, bytes, len, 0, addr, &addrLen)) > 0) {
	return recvLen;
  } else {
	return 0;
  }
}

int calculateRtt(uint32_t ackNum, Resf *resf) {
  //printf("%s %s calRTT before estimate %d dev %d sampleSeq %d timeout %d ackNum %d lastSent %d lastRecv %d\n", __FILE__, __func__, resf->estimateRtt, resf->devRtt, resf->sampleSeqNum, resf->timeout, ackNum, resf->lastSent, resf->lastRecved);
  int shouldCal = 0;
  if (resf->lastAcked >= resf->lastSent) {
	if (resf->sampleSeqNum < resf->lastSent) {
	  if (ackNum > resf->sampleSeqNum) {
		shouldCal = 1;
	  }
	}
	if (resf->sampleSeqNum > resf->lastAcked) {
	  if (ackNum < resf->lastSent || ackNum > resf->sampleSeqNum) {
		shouldCal = 1;
	  }
	}
  } else if (resf->sampleSeqNum >= resf->lastAcked && resf->sampleSeqNum < resf->lastSent) {
	if (resf->sampleSeqNum < resf->lastSent) {
	  shouldCal = 1;
	}
  }
  if (!shouldCal) {
	return 0;
  }
  struct timeval now;
  struct timeval sentTime = resf->sampleSentTime;
  gettimeofday(&now, NULL);
  uint32_t sampleRtt = (now.tv_sec - sentTime.tv_sec) * 1000000 + now.tv_usec - sentTime.tv_usec;
  if (resf->estimateRtt ==  0) {
	resf->estimateRtt = sampleRtt;
	//printf("%d\n", resf->estimateRtt);
  } else {
	resf->estimateRtt  = 0.875 * (resf->estimateRtt) + 0.125 * sampleRtt;
  }
  if (sampleRtt > resf->estimateRtt) {
	resf->devRtt = 0.75 * (resf->devRtt) + 0.25 * (sampleRtt - resf->estimateRtt);
  } else {
	resf->devRtt = 0.75 * (resf->devRtt) + 0.25 * (resf->estimateRtt - sampleRtt);
  }
  //resf->timeout = resf->estimateRtt + 4 * resf->devRtt;
  //printf("%s %s calRTT after estimate %lu dev %lu sampleRtt %lu sampleSeq %lu timeout %lu ackNum %d lastSent %d lastAcked %d\n", __FILE__, __func__, resf->estimateRtt, resf->devRtt, sampleRtt,  resf->sampleSeqNum, resf->timeout, ackNum, resf->lastSent, resf->lastAcked);
  return 0;
}


int calculateNewTimeout(Resf * resf) {
  if (resf->estimateRtt != 0) {
	resf->timeout = resf->estimateRtt + 4 * resf->devRtt;
  }
  return 0;
}

// get congest status 
int getCongestStatus(char * buffer, congestStatus status) {
  if (status == SLOW) {
	sprintf(buffer, "Slow Start");
  } else if (status == AVOID) {
	sprintf(buffer, "Congestion Avoidance");
  } else if (status == RECOV) {
	sprintf(buffer, "Fast Recovery");
  }
  return 0;
}
