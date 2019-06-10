#include "base.h"
#include "msg.h"
#include "queue.h"


/*
 * Name: dequeue
 *
 * Description: Removes a request from its current queue
 */

void dequeue(rfb_request ** head,rfb_request * req)
{   
    if (*head == req)
        *head = req->next;
    
    if (req->prev)
        req->prev->next = req->next;
    if (req->next)
        req->next->prev = req->prev;
    
    req->next = NULL;
    req->prev = NULL;
}

/*
 * Name: enqueue
 *
 * Description: Adds a request to the head of a queue
 */

void enqueue(rfb_request ** head,rfb_request * req)
{
    if (*head)
        (*head)->prev = req;    /* previous head's prev is us */

    req->next = *head;          /* our next is previous head */
    req->prev = NULL;           /* first in list */

    *head = req;                /* now we are head */
}
     

void init_queue(QUEUE *pQueue,unsigned char *pucBuf,unsigned int uiMaxBufSize)
{
  pQueue->uiFront=0;
  pQueue->uiRear=0;
  pQueue->pBuf=pucBuf;
  pQueue->uiMaxBufSize=uiMaxBufSize;
  pQueue->uiBufOffset=0;
}

unsigned char en_queue(QUEUE *pQueue,unsigned char *ucpData,unsigned int uiSize,unsigned char ucType)
{
  unsigned int uiPos;

  uiPos=((pQueue->uiRear+1)&0x7F);   //0x7F MAXQueueCnt 128
  if(pQueue->uiFront==uiPos) { //��������

    return 1;
  }
  else
  {
    if((pQueue->uiBufOffset+uiSize)>pQueue->uiMaxBufSize)
      pQueue->uiBufOffset=0;
    pQueue->stIndex[pQueue->uiRear].uiSize=uiSize;
    pQueue->stIndex[pQueue->uiRear].ucType=ucType;
    pQueue->stIndex[pQueue->uiRear].pBuf=&(pQueue->pBuf[pQueue->uiBufOffset]);
    memcpy(&pQueue->pBuf[pQueue->uiBufOffset],ucpData,uiSize);
    pQueue->uiBufOffset+=uiSize;
    pQueue->uiRear=uiPos;
    return 0;
  }
}

QUEUE_INDEX * de_queue(QUEUE *pQueue)
{
  return &(pQueue->stIndex[pQueue->uiFront]);
}

unsigned char de_queuePos(QUEUE *pQueue)
{
  pQueue->uiFront=((pQueue->uiFront+1)&0x7F);
  return 0;
}

unsigned char empty_queue(QUEUE *pQueue)
{
  if(pQueue->uiRear==pQueue->uiFront)
    return 0x1;
  else
    return 0x0;
}

unsigned char full_queue(QUEUE *pQueue)
{
    unsigned int  uiPos;

  uiPos=((pQueue->uiRear+1)&0x7F);   //0x1F MAXQueueCnt 32
  if(pQueue->uiFront==uiPos) //��������
    return 0;
  else
    return 1;
}

