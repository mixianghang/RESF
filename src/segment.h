#ifndef SEGMENT
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define SEGMENT
typedef struct resf_segment {
  uint32_t seqNum;
  uint32_t ackNum;
  uint16_t dataLen;
  uint16_t rwnd;
  uint8_t  flags;
  unsigned char * data;
} segment ;

//pack uint_32 into char bytes of network order
int packUint32(unsigned char * bytes, uint32_t data);

uint32_t unpackUint32(const unsigned char * bytes);

int packUint16(unsigned char * bytes, uint32_t data);


uint16_t unpackUint16(const unsigned  char * bytes);

/**
typedef struct resf_segment {
  uint32_t seqNum;
  uint32_t ackNum;
  uint16_t dataLen;
  uint16_t rwnd;
  char * data;
} segment ;
*/
int packSegment(segment * seg, unsigned char * bytes);

/**
*@description unpack received data into segment
*/
int unpackSegment(segment * seg, const unsigned  char * bytes, int len);

#endif
