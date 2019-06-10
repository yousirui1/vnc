#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include "base.h"
#include "msg.h"

void init_client()
{
	//rfb_client server;
	//memset(&server, 0, sizeof(rfb_client));


	//create_udp(NULL, 23002,  &server);
	//init_ffmpeg(&server);	
}

#if 0
void init_ffmpeg(rfb_client *ser)
{
   	AVFormatContext *pFormatCtx;
    int             i, videoindex;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVCodecContext  *pH264CodecCtx;
    AVCodec         *pH264Codec;
    av_register_all();
    avformat_network_init();
    avdevice_register_all();    //Register Device
    pFormatCtx = avformat_alloc_context();

    AVDictionary* options = NULL;
#ifdef _WIN32
    av_dict_set(&options,"framerate","25",0);
    //抓取屏幕
    av_dict_set(&options,"draw_mouse","0",0);				//鼠标
    AVInputFormat *ifmt=av_find_input_format("gdigrab");
    if(avformat_open_input(&pFormatCtx,"desktop",ifmt, &options)!=0)
    {   
        DIE("Couldn't open input stream. ");
    }
#else
	av_dict_set(&options,"framerate","25",0);
	av_dict_set(&options,"follow_mouse","centered",0);		//鼠标
	av_dict_set(&options,"video_size","640x480",0);
    AVInputFormat *ifmt=av_find_input_format("x11grab");
    if(avformat_open_input(&pFormatCtx,":0.0+10,20",ifmt, &options)!=0){
        printf("Couldn't open input stream. ");
        return -1; 
    }   
#endif
   
    if(avformat_find_stream_info(pFormatCtx,NULL)<0)
    {   
        printf("Couldn't find stream information.\n");
        return -1; 
    }   
    videoindex=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
    {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            videoindex=i;
        }
    }
    if(videoindex==-1)
    {
        printf("Couldn't find a video stream.\n");
        return -1;
    }

    //根据视频中的流打开选择解码器
    pCodecCtx=pFormatCtx->streams[videoindex]->codec;
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL)
    {
        printf("Codec not found.\n");
        return -1;
    }
    //打开解码器
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
    {
        printf("Could not open codec.\n");
        return -1;
    }
    AVFrame *pFrame,*pFrameYUV;
    pFrame=av_frame_alloc();
    pFrame->width = pCodecCtx->width;
    pFrame->height = pCodecCtx->height;
    pFrame->format = AV_PIX_FMT_YUV420P;

    pFrameYUV=av_frame_alloc();
    pFrameYUV->width = pCodecCtx->width;
    pFrameYUV->height = pCodecCtx->height;
    pFrameYUV->format = AV_PIX_FMT_YUV420P;

    uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
    int ret, got_picture;
    AVPacket *packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    AVPacket *packetH264=(AVPacket *)av_malloc(sizeof(AVPacket));
    struct SwsContext *img_convert_ctx;
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    ///这里打印出视频的宽高
    fprintf(stderr,"w= %d h= %d\n",pCodecCtx->width, pCodecCtx->height);
    ///我们就读取100张图像
    //查找h264编码器
    pH264Codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(!pH264Codec)
    {
        fprintf(stderr, "---------h264 codec not found----\n");
        exit(1);
    }

    pH264CodecCtx = avcodec_alloc_context3(pH264Codec);
    pH264CodecCtx->codec_id = AV_CODEC_ID_H264;
    pH264CodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pH264CodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pH264CodecCtx->width = pCodecCtx->width;
    pH264CodecCtx->height = pCodecCtx->height;
    pH264CodecCtx->time_base.num = 1;
    pH264CodecCtx->time_base.den = 25;//帧率(既一秒钟多少张图片)
    pH264CodecCtx->bit_rate = 4000000; //比特率(调节这个大小可以改变编码后视频的质量)
    pH264CodecCtx->gop_size= 12;
    //H264 还可以设置很多参数 自行研究吧
    pH264CodecCtx->qmin = 10;
    pH264CodecCtx->qmax = 51;
    pH264CodecCtx->max_b_frames = 0;
    // some formats want stream headers to be separate
    if (pH264CodecCtx->flags & AVFMT_GLOBALHEADER)
        pH264CodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    // Set Option
    AVDictionary *param = 0;
    //H.264
    av_dict_set(&param, "preset", "superfast", 0);
    av_dict_set(&param, "tune", "zerolatency", 0);  //实现实时编码
    if (avcodec_open2(pH264CodecCtx, pH264Codec,&param) < 0)
	{
        printf("Failed to open video encoder1! 编码器打开失败！\n");
        return -1;
    }
	for(;;)
    {
        //读取截屏中的数据--->packet
        if(av_read_frame(pFormatCtx, packet) < 0)
        {
            break;
        }
        if(packet->stream_index==videoindex)
        {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if(ret < 0){
                printf("Decode Error.\n");
                return -1;
            }
            if(got_picture)
            {
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

                //编码成h264文件
                ret = avcodec_encode_video2(pH264CodecCtx, packet,pFrameYUV, &got_picture);
                if(ret < 0){
                    printf("Failed to encode! \n");
                    return -1;
                }
                //h264_send_data((char *)packet->data, packet->size);
            }
        }
        av_free_packet(packet);
    }
    sws_freeContext(img_convert_ctx);
    av_free(out_buffer);
    av_free(pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}








#if 0
void *client_socket(void *param)
{
	int sockfd = 0;
    struct sockaddr_in ser_addr;
         
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {   
        DIE("client socket err");
    }   

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(server_port);
        
    if(inet_aton(server_ip, (struct in_addr *)&ser_addr.sin_addr.s_addr) == 0)
    {   
        DIE("client inet_aton err");
    }   
        
    while(connect(sockfd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) != 0)
    {   
        DEBUG("connection failed reconnection after 3 seconds");
        sleep(3);
    }   

	login_server();
	select_loop();
}


void login_server()
{
	unsigned char buf[1024] = {0};
    unsigned char *tmp = &buf[8];

	int server_major = 0, server_minor = 1;

	rfb_head *head = (rfb_head *)&buf[0];
	head->syn = 0xff;
    head->enry_flag = 0;
	
	/* version */
	sprintf(tmp, VERSIONFORMAT, server_major, server_minor);	

	send(sockfd, buf, HEAD_LEN + sz_verformat, 0);
	read(sockfd, buf, HEAD_LEN + sz_verformat);

	//sscanf(buf + 8, VERSIONFORMAT, &server_major, &server_minor);	
	DEBUG(VERSIONFORMAT, server_major, server_minor);
	

	/* format */
	rfb_format *fmt = (rfb_format *)&tmp[0];
	fmt->width = 1280;
    fmt->height = 720;
    fmt->code = 0;
    fmt->data_port = 0;
    fmt->play_flag = 0;
    fmt->vnc_flag = 0;
	
	send(sockfd, buf, 8 + sizeof(rfb_format), 0);
    read(sockfd, buf, 8 + sizeof(rfb_format), 0);
	
	DEBUG("fmt->width %d fmt->height %d fmt->code %d,"
		  " fmt->data_port %d fmt->play_flag %d  fmt->vnc_flag %d",
			 fmt->width, fmt->height,fmt->code, fmt->data_port,fmt->play_flag, fmt->vnc_flag);
	if(fmt->width < 0 || fmt->width > 1920 || fmt->height < 0 || fmt->height > 1080 || fmt->data_port < 0 || fmt->data_port > 24000)
	{
		DIE("login server fail ");
	}

	//init_udp(fmt->data_port);
}




void select_loop()
{


}

#endif

#endif
