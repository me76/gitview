#pragma once

#include <list>
#include <string>

typedef std::list<std::wstring> StringList;

struct Entry
{
	enum Type { File, Directory, Commit };

	std::wstring mName;
	Type mType;

	Entry(const std::wstring& name, Type type):
		mName(name), mType(type)
	{
	}
};

typedef std::list<Entry> Entries;

struct OpStatus
{
	enum Status
	{
		NoError = 0,
		GitError = 1,
		BadResult = 2,
		SystemError = 50,
		InternalError = 100,
		Unexpected = -1
	};

	int mStatus = NoError;
	int mErrorCode = 0;
	std::wstring mDescr;

	OpStatus() { }

	OpStatus(int status, const std::wstring& descr, int errorCode = 0):
		mStatus(status), mErrorCode(errorCode), mDescr(descr)
	{ }

	OpStatus(int status, std::wstring&& descr, int errorCode = 0):
		mStatus(status), mErrorCode(errorCode), mDescr(descr)
	{ }

	bool isGood() const { return NoError == mStatus; }
	bool isBad()  const { return !isGood(); }

	void set(Status status, const wchar_t* descr)      { mStatus = status, mDescr = descr; }
	void set(Status status, const std::wstring& descr) { mStatus = status, mDescr = descr; }
	void set(Status status, std::wstring&& descr)      { mStatus = status, mDescr = descr; }
	void clear() { mStatus = NoError; mDescr.clear(); }
};
