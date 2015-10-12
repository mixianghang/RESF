/***********************************************
*
*Filename: resf.h
*
*@author: Xianghang Mi
*@email: mixianghang@outlook.com
*@description: ---
*Create: 2015-10-06 09:51:51
*Last Modified: 2015-10-06 09:51:51
************************************************/
#ifndef RESF
#define RESF

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "segment.h"

#define MIN_BUFFER_SIZE 1440
#define MAX_SEG_SIZE    1440
#define MAX_SEGMENT_DATA_SIZE 1440
#define SEND_BUFFER_SIZE 65536// 100 segments
#define RECV_BUFFER_SIZE 65536 // 100 segments
#define TIME_WAIT        1// time wait in microseconds

typedef enum congestion {
  SLOW,//slow start
  AVOID,//congestion avoidance
  RECOV,//fast recovery
} congestStatus;

typedef struct fragment {
  int start;
  int end;
  int len;
  struct fragment * next;
} fragment;

typedef struct resf {
  // variables used to calculate and store timeout
  uint32_t estimateRtt;
  uint32_t devRtt;
  uint32_t sampleSeqNum;// used to get the sample rtt
  struct timeval sampleSentTime;// when received the sample ack, calculate the sampleRTT
  uint32_t timeout;
  struct timespec timeoutStart;

  uint32_t rwnd;// receive window size
  uint32_t cwnd;// congestion windown size
  uint32_t ssthresh;
  uint32_t lastSent;// the position of last sent
  uint32_t lastAcked;
  uint32_t lastWrite;
  uint32_t lastRecved;
  uint32_t lastRead;
  uint8_t  dupAckNum;

  uint32_t recvWaitTime;
  uint32_t sendWaitTime;
  uint32_t recvWaitTimeInterval;
  uint32_t sendWaitTimeInterval;
  congestStatus cstatus;
  fragment * recvFrag;
  int sockFd;//the sock fd from where to read and to where to send
  struct sockaddr * addr;// the address of the other side of the sock connection

  unsigned char sendBuff[SEND_BUFFER_SIZE];
  unsigned char recvBuff[RECV_BUFFER_SIZE];

  pthread_t  sendThread;
  // wait for data to send
  pthread_cond_t unsentDataCond;
  pthread_mutex_t unsentDataMutex;

  // wait for data to be read out
  pthread_cond_t newRecvSpaceCond;
  pthread_mutex_t newRecvSpaceMutex;

  //get this before send new data except retransmit
  pthread_mutex_t sendNewDataMutex;

  pthread_t  recvThread;
} Resf;


/**
*@description send bytes to sock
*@param resf resf 
*/
int  sendToSock(int sockFd, struct sockaddr * addr, const unsigned char * bytes, int len);

/**
*@description send bytes to sock
*@param resf resf 
*/
int  recvFromSock(int sockFd, struct sockaddr * addr, unsigned char * bytes, int len);

// check whether time out and  what we should do for timeout
int timeout(Resf * resf);

// check three duplicates acks
int duplicateAcks(Resf * resf);

// prepare and send
int prepareAndSend(Resf * resf);

/**
*@description read from sendbuffer and send to sock
*@param resf resf 
*/
void * startsend( void * resf);

/**
*@description read from sock and save to recvbuffer
*@param resf Resf 
*/
void * startRecv( void * resf);

/**
*initialize resf
*resf Resf pointer
*/
void initResf(Resf * resf);

/**
*reinitialize resf
*resf Resf pointer
*/
void reinitResf(Resf * resf);
/**
*@description set up the read and write threads which are used to read and write to sock
*@ resf struct resf configuration and variables used by those two threads
*@return 
*/
int startResf(Resf * resf);
/**
*@description recv  no more than recvLen bytes, save it to recvBuff before reach a char equal to suffix  
*@description if recv a suffix before recving recvLen data, then stop and save received data into recvBuff
*@param resf
*param  suffix    char  stop receiving data when meeting this char
*@param recvBuff  char* store the received data
*@param recvLen   int   the required receive length
*@return    int   -1 if error happens, otherwise the length of received data in bytes 
*/
int recvFromResfBySep(Resf *resf, unsigned char * recvBuff, int recvLen, char suffix, int isTimeout);

/**
*@description recieve recvLen data, will block when there isn't enough data 
*@param resf
*@param recvBuff  char* store the received data
*@param recvLen   int   the required receive length
*@return    int   -1 if error happens, otherwise the length of received data in bytes 
*/
int recvFromResf(Resf *resf, unsigned char * recvBuff, int recvLen);

/**
*@description send sendLen data from sendBuffer to socket sockFd
*@param resf Resf
*@param sendBuff  char* store data to be sent
*@param sendLen   int   the length of the data to be sent
*@return int       -1 if error happens, otherwise the length of sent data
*/
int sendToResf(Resf * resf, const unsigned char * sendBuff, int sendLen);

//retransmit the oldest 
int retransmit(Resf *resf);

// congestion control when duplicate 3
int congestWhenDup3(Resf *resf);

// congestion control when new Ack
int congestWhenNAck(Resf *resf);

//congestWhen receive a new duplicate
int congestWhenNDup(Resf *resf);

// congestion control when timeout
int congestWhenTimeout(Resf *resf);

//timeout change when duplicate 3
int timeoutWhenDup(Resf *resf);

//prepare and send one with ack 
int sendDataWithAck(Resf *resf, uint32_t ackNum);

// calculate rtt
int calculateRtt(uint32_t ackNum, Resf *resf);

//calculate new timeout
int calculateNewTimeout(Resf * resf);

// get congest status 
int getCongestStatus(char * buffer, congestStatus status);
#endif
