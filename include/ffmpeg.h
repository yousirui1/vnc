#ifndef __FFMPEG_H__
#define __FFMPEG_H__

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

/* decode */
typedef struct input_stream
{
	uint8_t *out_buffer;
    AVCodec *codec;
    AVCodecContext *codec_ctx;
    AVFrame *frame;
	AVFrame *frame_yuv;
    AVPacket *packet;
    struct SwsContext *img_convert_ctx;
}input_stream;

/* encode */
typedef struct output_stream
{
	uint8_t *out_buffer;
    AVFormatContext *format_ctx;
    AVCodecContext  *codec_ctx;
    AVCodec         *codec;
    AVCodecContext  *h264codec_ctx;
    AVCodec         *h264codec;
    AVFrame *frame;
	AVFrame *frame_yuv;
    AVPacket *packet;
    struct SwsContext *img_convert_ctx;
}output_stream;

#endif
