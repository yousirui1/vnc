#ifndef __VNCHOOKS_H__
#define __VNCHOOKS_H__

#ifdef _WIN32
#include <windows.h>


#ifdef __cplusplus
extern "C" 
{
#endif
extern int SetKeyboardHook(int active);

#ifdef __cplusplus
}
#endif

#endif
#endif
