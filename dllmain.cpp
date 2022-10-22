#include "pch.h"

#include <tlhelp32.h>
#include <tchar.h>

#include "GitView.h"
#include "utils.h"

GitView gview;

BOOL CALLBACK EnumThreadWndProc(HWND wnd, LPARAM lParam)
{
	if(IsWindow(wnd))
	{
		WCHAR title[256];
		GetWindowText(wnd, title, DIM(title));

		WCHAR tcTitle[] = L"Total Commander";
		title[DIM(tcTitle)-1] = 0;

		if(wcscmp(title, tcTitle) == 0)
		{
			GitView* gview = (GitView*)lParam;
			gview->mTCWindow = wnd;
			return FALSE;
		}
	}

	return TRUE;
}

bool saveMainWnd(GitView& gview)
{
	auto thisPid = GetCurrentProcessId();

	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;

	// Take a snapshot of all running threads  
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
		return false;

	// Fill in the size of the structure before using it. 
	te32.dwSize = sizeof(THREADENTRY32);

	for(BOOL ok = Thread32First(hThreadSnap, &te32); ok; ok = Thread32Next(hThreadSnap, &te32))
	{
		if(te32.th32OwnerProcessID == thisPid)
		{
			EnumThreadWindows(te32.th32ThreadID, &EnumThreadWndProc, (LPARAM) &gview);
		}
	}

	//  Don't forget to clean up the snapshot object.
	CloseHandle(hThreadSnap);

	return gview.mTCWindow != NULL;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{//return FALSE;
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			gview.mModule = hModule;
			if(!saveMainWnd(gview))
				return FALSE;
		break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
