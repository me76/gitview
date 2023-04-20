#include "pch.h"

#include <tlhelp32.h>
#include <tchar.h>

#include "GitView.h"
#include "utils.h"

GitView gview;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			gview.mModule = hModule;
		break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
