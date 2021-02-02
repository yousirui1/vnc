#ifndef __FRAME_H__
#define __FRAME_H__

#include <libavformat/avformat.h>
#include <SDL2/SDL.h>

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))


typedef struct Frame
{
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;
    double duration;
    int64_t pos;
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flip_v;
}Frame;

typedef struct FrameQueue
{
    Frame queue[FRAME_QUEUE_SIZE];

    int rindex;
    int windex;

    int size;
    int max_size;
    //int keep_last;

    int rindex_shown;
    SDL_mutex *mutex;
    SDL_cond *cond;
} FrameQueue;


#endif

