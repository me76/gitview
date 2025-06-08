#include "pch.h"

#include "api.h"
#include "GitView.h"
#include "RepoNameIterator.h"
#include "utils.h"

#include <algorithm>
#include <cassert>
#include <sstream>

#include <shlwapi.h>

using namespace std;

extern GitView gview;

const wchar_t settingsItem[] = L"@settings";

bool isSettingsDir(const wchar_t* path)
{
	if(!path || 0 == path[0])
		return false;

	return wcscmp(path+1, settingsItem) == 0;
}

void getSettingsDirName(WCHAR* buf, size_t size)
{
	copy_n(settingsItem, min(DIM(settingsItem), size), buf);
}

void FsGetDefRootName(char* defRootName, int maxlen)
{
	const char name[] = "gitview";
	copy_n(name, min(DIM(name), maxlen), defRootName);
}

int FsInit(int pluginNo,
           tProgressProc progressFunc,
           tLogProc logFunc,
           tRequestProc requestFunc)
{
	gview.init(pluginNo, nullptr, nullptr, nullptr);

	return 0;
}

int FsInitW(int pluginNo,
            tProgressProcW progressFunc,
            tLogProcW logFunc,
            tRequestProcW requestFunc)
{
	gview.init(pluginNo, progressFunc, logFunc, requestFunc);

	return 0;
}

void FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	gview.loadSettings(dps->DefaultIniName);
}

HANDLE FsFindFirst(CHAR* path, WIN32_FIND_DATA* fileData)
{
	SetLastError(ERROR_NO_MORE_FILES);
	return INVALID_HANDLE_VALUE;
}

HANDLE FsFindFirstW(WCHAR* path, WIN32_FIND_DATAW* fileData)
{
	if(!fileData)
	{
		SetLastError(ERROR_NO_MORE_FILES);
		return INVALID_HANDLE_VALUE;
	}

	/*dir structure:
	  /
	  repo_1/
	    branches/
	      branch_1/
	        files...
	      branch_2/
	      ...
	    tags/
	      tag_1/
	        files...
	      tag_2/
	      ...
	  repo_2/
	  ...
	*/

	IFileIterator* fIt = gview.createFileIterator(path);

	if (!fIt) return INVALID_HANDLE_VALUE;

	if (!fIt->getData(*fileData))
	{
		FsFindClose(fIt);
		fIt = 0;
	}

	return fIt ? (HANDLE)fIt : INVALID_HANDLE_VALUE;
}

BOOL FsFindNext(HANDLE fh, WIN32_FIND_DATA* fileData)
{
	return FALSE;
}

BOOL FsFindNextW(HANDLE fh, WIN32_FIND_DATAW* fileData)
{
	if(IFileIterator* fIt = gview.getFileIterator(fh))
	{
		if(fIt->next() && fIt->getData(*fileData))
		{
			return TRUE;
		}
	}

	return FALSE;
}

int FsFindClose(HANDLE fh)
{
	gview.removeFileIterator(fh);

	return 0;
}

int FsExtractCustomIcon(char* itemPath, int extractFlags, HICON* hIcon)
{
	return FS_ICON_USEDEFAULT;
}


int FsExtractCustomIconW(WCHAR* itemPath, int extractFlags, HICON* hIcon)
{
	if(HICON icon = gview.getItemIcon(itemPath))
	{
		*hIcon = icon;
		return FS_ICON_EXTRACTED;
	}
	else
	{
		return FS_ICON_USEDEFAULT;
	}
}

int FsGetFileW(WCHAR* srcPath, WCHAR* destPath, int copyFlags, RemoteInfoStruct* /*remoteInfo*/)
{
	if(0 == copyFlags)
	{
		if(PathFileExistsW(destPath))
			return FS_FILE_EXISTSRESUMEALLOWED;
	}
	else
	{
		if(copyFlags & FS_COPYFLAGS_MOVE)
			return FS_FILE_NOTSUPPORTED;

		if((copyFlags & (FS_COPYFLAGS_RESUME | FS_COPYFLAGS_OVERWRITE)) == 0)
			return FS_FILE_NOTSUPPORTED;
	}

	OpStatus saveStatus;
	gview.saveFile(srcPath, destPath, saveStatus);

	return saveStatus.isGood()  ? FS_FILE_OK : FS_FILE_READERROR;
}

int FsExecuteFile(HWND mainWnd, char* remoteName, char* verb)
{
	return FsExecuteFileW(
		mainWnd,
		const_cast<wchar_t*>(str2wstr(remoteName).c_str()),
		const_cast<wchar_t*>(str2wstr(verb).c_str())
	);
}

int FsExecuteFileW(HWND mainWnd, wchar_t* remoteName, wchar_t* verb)
{
	if(wcscmp(verb, L"open") == 0)
	{
		ItemKey itemKey(remoteName);
		if(GitRef::Unknown == itemKey.refType && GitView::reloadName == itemKey.repoName)
		{
			bool reloadOk = gview.reloadSettings();

			MessageBox(
				mainWnd,
				reloadOk ? L"Settings applied successfully." : L"Failed to apply settings.\nSee details in 'init.log' and in gitview.log.",
				L"Reload settings",
				MB_OK
			);
		}
	}

	return FS_EXEC_OK;
}