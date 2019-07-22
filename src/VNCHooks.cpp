#include "VNCHooks.h"
extern "C"
{
#include "base.h"
#include "msg.h"
}

#ifdef _WIN32
HHOOK hKeyboardHook = NULL;
HINSTANCE hInstance = NULL;


LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	//rfb_keyevent key = {0};
	PKBDLLHOOKSTRUCT pVirKey = (PKBDLLHOOKSTRUCT)lParam;

	//MessageBox(NULL, "WIN", "DOWN", MB_ICONEXCLAMATION | MB_OK);
	if(nCode >= 0)
	{
		switch(wParam)
		{
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				switch(pVirKey->vkCode)
				{
					case VK_LWIN:
					case VK_RWIN:
					{
						//key.key = 1073742055;
						MessageBox(NULL, "WIN", "DOWN", MB_ICONEXCLAMATION | MB_OK);
						//key.down = 1;
						//send_control((char *)&key, sizeof(rfb_keyevent), 0x04);

						//return CallNextHookEx(hInstance, nCode, wParam, lParam);
						//return CallNextHookEx(hInstance, nCode, wParam, lParam);
						//return 0;
						return TRUE;
					}
					case VK_MENU:
						//key.key = VK_MENU;
						//key.down = 1;
						return 1;
					default:
						break;
				}
				break;

			case WM_KEYUP:
			case WM_SYSKEYUP:
				switch(pVirKey->vkCode)
				{
					case VK_LWIN:
					case VK_RWIN:
					{
						//key.key = 1073742055;
						MessageBox(NULL, "WIN", "UP", MB_ICONEXCLAMATION | MB_OK);
						//key.down = 0;
						//send_control((char *)&key, sizeof(rfb_keyevent), 0x04);
						return TRUE;
					}
					case VK_MENU:
						//key.key = 1073742050;
						//key.down = 0;
						//send_control((char *)&key, sizeof(rfb_keyevent), 0x04);
						return 1;	
				}
				break;
		}

		//return CallNextHookEx(hInstance, nCode, wParam, lParam);
	}
	//return CallNextHookEx(hInstance, nCode, wParam, lParam);
}

int SetKeyboardHook(int active)
{
	if(active)
	{
		hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, hInstance ,0);
	}
	else
	{
		UnhookWindowsHookEx(hKeyboardHook);
		hKeyboardHook = NULL;	
	}
	return 0;
}

#endif
