#pragma once

#include "IFileIterator.h"

#include "RepoNameIterator.h"

#include <iosfwd>

class TopDirIterator: public IFileIterator
{
	enum SpecialItem
	{
		Item_InitLog,
		Item_Reload,
		Item_EndMark
	};

public:
	TopDirIterator(const NamedRepos& repos, const std::wstringstream& initLog);

private:
	virtual bool getDataImpl(WIN32_FIND_DATAW& fileData) const;
	virtual void nextImpl();
	virtual bool isValid() const;

private:
	RepoNameIterator mRepoNameIt;
	const std::wstringstream& mInitLog;

	SpecialItem currentItem = Item_InitLog;

public:
	static void* operator new(size_t size);
	static void operator delete(void* p);
};
