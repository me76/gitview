#pragma once

#include <string>
#include <ostream>

#define DIM(a) (sizeof(a) / sizeof(*a))

std::wstring str2wstr(const std::string& s);

void assignFileName(char dest[MAX_PATH], const std::string& src);
void assignFileName(wchar_t dest[MAX_PATH], const std::string& src);
void assignFileName(wchar_t dest[MAX_PATH], const std::wstring& src);

void getPathHeadTail(const wchar_t* path, std::wstring& head, const wchar_t*& tail);

struct LineLogger
{
	std::wostream* log;

	LineLogger(std::wostream& s): log(&s) { }

	template<class T>
	LineLogger& operator<<(const T& t)
	{
		*log << t;
		return *this;
	}

	~LineLogger()
	{
		*log << std::endl;
	}
};
