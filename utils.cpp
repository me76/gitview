#include "pch.h"

#include "utils.h"

#include <algorithm>

using namespace std;

wstring str2wstr(const std::string& s)
{
	wstring result(s.size(), L' ');
	copy(s.begin(), s.end(), result.begin());
	return result;
}

void assignFileName(char dest[MAX_PATH], const string& src)
{
	size_t resultSize = min(src.size(), MAX_PATH);
	copy_n(src.begin(), resultSize, dest);
	dest[MAX_PATH-1] = 0;
}

void assignFileName(wchar_t dest[MAX_PATH], const string& src)
{
	size_t resultSize = min(src.size(), MAX_PATH);
	copy_n(src.begin(), resultSize, dest);
	dest[MAX_PATH - 1] = 0;
}

void assignFileName(wchar_t dest[MAX_PATH], const wstring& src)
{
	size_t resultSize = min(src.size(), MAX_PATH);
	copy_n(src.begin(), resultSize, dest);
	dest[MAX_PATH - 1] = 0;
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