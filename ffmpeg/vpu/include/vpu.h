#ifndef __VPU_H__
#define __VPU_H__

#include "vpu_gloabl.h"
#include "vpu_api.h"

/* 32位寄存器内的字节次序变反 */
#define BSWAP32(x) \
    ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

typedef enum VPU_API_RET {
	VPU_OK = 0;
	//VPU_

}VPU_API_RET;

typedef struct VpuApiContext{
	RK_U32 width;
	RK_U32 height;
	CODEC_TYPE  codec_type;
	OMX_RK_VIDEO_CODINGTYPE coding;
	RK_U8 input_file[256];
	RK_U8 output_file[256];
	RK_U8 input_flag;
	RK_U8 output_flag;
	RK_U32 fps;							
	RK_S64 start_ms;
};



#endif
