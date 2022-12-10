#pragma once

#include <string>
#include <ostream>

#define DIM(a) (sizeof(a) / sizeof(*a))

std::wstring str2wstr(const std::string& s);

template<typename TSrcChar, typename TDestChar>
void assignFileName(TDestChar dest[MAX_PATH], const std::basic_string<TSrcChar>& src)
{
	size_t resultSize = min(src.size(), MAX_PATH - 1);
	copy_n(src.begin(), resultSize, dest);
	dest[resultSize + 1] = 0;
}

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
