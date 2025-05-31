#pragma once

#include "fsplugin.h"

void FsGetDefRootName(char* defRootName, int maxlen); //get 'gitview' name for showing in Network Neighborhood

int FsInit(int pluginNo,
           tProgressProc progressFunc,
           tLogProc logFunc,
           tRequestProc requestFunc);

int FsInitW(int pluginNo,
            tProgressProcW progressFunc,
            tLogProcW logFunc,
            tRequestProcW requestFunc);

void FsSetDefaultParams(FsDefaultParamStruct* dps);

HANDLE FsFindFirst(CHAR* path, WIN32_FIND_DATA* fileData); //get first item in directory
HANDLE FsFindFirstW(WCHAR* path, WIN32_FIND_DATAW* fileData);
BOOL FsFindNext(HANDLE fh, WIN32_FIND_DATA* fileData); //get next item in directory
BOOL FsFindNextW(HANDLE fh, WIN32_FIND_DATAW* fileData);
int FsFindClose(HANDLE fh); //called when item listing ends

int FsExtractCustomIcon(char* itemPath, int extractFlags, HICON* hIcon);
int FsExtractCustomIconW(WCHAR* itemPath, int extractFlags, HICON* hIcon);

int FsGetFileW(WCHAR* srcPath, WCHAR* destPath, int copyFlags, RemoteInfoStruct* remoteInfo); //copy item content to 'destPath'
