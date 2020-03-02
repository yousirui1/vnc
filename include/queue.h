#ifndef __QUEUE_H__
#define __QUEUE_H__
#include <stdio.h>
#include <string.h>


#define MAX_QUEUESIZE 128

//void dequeue(rfb_request ** head,rfb_request * req);
//void enqueue(rfb_request ** head,rfb_request * req);


typedef struct 
{
  unsigned int uiSize;
  unsigned char *pBuf;
  unsigned char ucType;
}QUEUE_INDEX;


typedef struct 
{
  unsigned int uiFront;
  unsigned int uiRear;
  unsigned int uiMaxBufSize;
  unsigned int uiBufOffset;
  unsigned char *pBuf;
  QUEUE_INDEX stIndex[MAX_QUEUESIZE];
}QUEUE;

void init_queue(QUEUE *pQueue,unsigned char *pucBuf,unsigned int uiMaxBufSize);
QUEUE_INDEX * de_queue(QUEUE *pQueue);
unsigned char de_queuePos(QUEUE *pQueue);
unsigned char en_queue(QUEUE *pQueue,unsigned char *ucpData,unsigned int uiSize,unsigned char ucType);
unsigned char empty_queue(QUEUE *pQueue);
unsigned char full_queue(QUEUE *pQueue);

#endif // QUEUE_H
