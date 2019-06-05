#ifndef __MSG_H__
#define __MSG_H__


typedef struct _rfb_head
{
    char syn;
    char entry;
    short cmd;
    int size;
}rfb_head;

typedef struct _rfb_filemsg
{
    char flags;
    short datasize;
    char path[128];
}rfb_filemsg;    

typedef struct _rfb_textmsg
{
    short pad1;
    short pad2;
    int length;
}rfb_textmsg;

typedef struct _rfb_key_event
{
    char down;
    char key;
}rfb_keyevent;

typedef struct _rfb_pointer_evnet
{
    short mask;
    short x;
    short y;
}rfb_pointevent;




#endif
