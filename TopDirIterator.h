#pragma once

#include "IFileIterator.h"

#include "RepoNameIterator.h"

#include <iosfwd>

class TopDirIterator: public IFileIterator
{
public:
	TopDirIterator(const NamedRepos& repos, const std::wstringstream& initLog);

private:
	virtual bool getDataImpl(WIN32_FIND_DATAW& fileData) const;
	virtual void nextImpl();
	virtual bool isValid() const;

private:
	RepoNameIterator mRepoNameIt;
	const std::wstringstream& mInitLog;

	bool mAtEnd = false;
};
