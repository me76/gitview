#pragma once

#include "fsplugin.h"

void FsGetDefRootName(char* DefRootName, int maxlen);

int FsInit(int pluginNo,
           tProgressProc progressFunc,
           tLogProc logFunc,
           tRequestProc requestFunc);

int FsInitW(int pluginNo,
            tProgressProcW progressFunc,
            tLogProcW logFunc,
            tRequestProcW requestFunc);

void FsSetDefaultParams(FsDefaultParamStruct* dps);

HANDLE FsFindFirst(CHAR* path, WIN32_FIND_DATA* fileData);
HANDLE FsFindFirstW(WCHAR* path, WIN32_FIND_DATAW* fileData);
BOOL FsFindNext(HANDLE fh, WIN32_FIND_DATA* fileData);
BOOL FsFindNextW(HANDLE fh, WIN32_FIND_DATAW* fileData);
int FsFindClose(HANDLE fh);

int FsGetFileW(WCHAR* srcPath, WCHAR* destPath, int copyFlags, RemoteInfoStruct* remoteInfo);
