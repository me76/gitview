#include "pch.h"

#include "utils.h"

#include <sysinfoapi.h>

#include <algorithm>

using namespace std;

wstring str2wstr(const std::string& s)
{
	wstring result(s.size(), L' ');
	copy(s.begin(), s.end(), result.begin());
	return result;
}

void getPathHeadTail(const wchar_t* path, std::wstring& head, const wchar_t*& tail)
{
	head.clear();

	if(!path) return;

	while(L'\\' == *path)
		++path;

	for(tail = path; *tail != L'\0' && *tail != L'\\'; ++tail)
		;

	head.reserve(tail - path);
	head.assign(path, tail);

	while(L'\\' == *tail)
		++tail;
}

LineLogger::LineLogger(std::wostream& s):
	log( &s )
{
	SYSTEMTIME now;
	GetLocalTime(&now);

	tm tmNow {now.wSecond, now.wMinute, now.wHour, now.wDay, now.wMonth - 1, now.wYear - 1900};

	char buf[25];
	strftime(buf, 24, "[%F %T] ", &tmNow);

	*log << str2wstr(buf);
}