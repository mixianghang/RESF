#include "segment.h"

//pack uint_32 into char bytes of network order
int packUint32(unsigned char * bytes, uint32_t data) {
  *(bytes++) = data >> 24;
  *(bytes++) = data >> 16;
  *(bytes++) = data >> 8;
  *(bytes++) = data;
  return 0;
}

uint32_t unpackUint32(const unsigned char * bytes) {
  return ((uint32_t)bytes[0] << 24  |  (uint32_t)bytes[1] << 16  |  (uint32_t)bytes[2] << 8  | bytes[3]);
}

int packUint16(unsigned char * bytes, uint32_t data) {
  *(bytes++) = data >> 8;
  *(bytes++) = data;
  return 0;
}

uint16_t unpackUint16(const unsigned char * bytes) {
  return ((uint16_t)bytes[0] << 8 | bytes[1]);
}

/**
typedef struct resf_segment {
  uint32_t seqNum;
  uint32_t ackNum;
  uint16_t dataLen;
  uint16_t rwnd;
  char * data;
} segment ;
*/
int packSegment(segment * seg, unsigned char * bytes) {
  unsigned char * pos = bytes;

  //printf("before send segment, rwnd is %d\n", seg->rwnd);
  if (packUint32(pos, seg->seqNum)) {
	return -1;
  }

  pos += 4;
  if (packUint32(pos, seg->ackNum)) {
	return -1;
  }

  pos += 4;
  if (packUint16(pos, seg->dataLen)) {
	return -1;
  }

  pos += 2;
  if (packUint16(pos, seg->rwnd)) {
	return -1;
  }

  pos += 2;
  *pos = seg->flags;

  pos++;
  
  if (seg->dataLen > 0) {
	if (strncpy(pos, seg->data, seg->dataLen) == NULL) {
	  return -1;
	}
  }

  return (pos - bytes) + seg->dataLen;
}

/**
*@description unpack received data into segment
*/
int unpackSegment(segment * seg, const unsigned  char * bytes, int len) {
  const unsigned char * pos = bytes;

  seg->seqNum = unpackUint32(pos);
  pos += 4;

  seg->ackNum = unpackUint32(pos);
  pos += 4;

  seg->dataLen = unpackUint16(pos);
  pos += 2;

  seg->rwnd = unpackUint16(pos);
  pos += 2;

  seg->flags = *pos;

  pos++;
  seg->data = pos;

  //printf("%s %s len is %d dataLen is %d seqNum %d ackNum %d flags %u\n" , __FILE__, __func__, len, seg->dataLen, seg->seqNum,seg->ackNum, seg->flags);
  if (seg->dataLen != (len - 13)) {
	//printf("%s %s len is %d dataLen is %d seqNum %d\n" , __FILE__, __func__, len, seg->dataLen, seg->seqNum);
	seg->dataLen = len - 13;
	//TODO return 1;
  }
  return 0;
}

int main2() {
  uint32_t uint32_1 = 65537;
  uint32_t uint32_2 = 1440;
  uint16_t uint16_1 =  65535;
  uint16_t uint16_2 = 1440;
  unsigned char byte[10];
  memset(byte, 0, sizeof byte);
  packUint32(byte, uint32_1);
  printf("%u \n", unpackUint32(byte));
  packUint32(byte, uint32_2);
  printf("%u \n", unpackUint32(byte));
  packUint16(byte, uint16_1);
  printf("%u \n", unpackUint16(byte));
  packUint16(byte, uint16_2);
  printf("%u \n", unpackUint16(byte));
  return 0;
}

