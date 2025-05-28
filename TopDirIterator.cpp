#include "pch.h"

#include "TopDirIterator.h"

#include "GitView.h"
#include "RepoNameIterator.h"

#include <sstream>

using namespace std;

TopDirIterator::TopDirIterator(const NamedRepos& repos, const wstringstream& initLog):
	mRepoNameIt(repos),
	mInitLog(initLog)
{
}

bool TopDirIterator::getDataImpl(WIN32_FIND_DATAW& fileData) const
{
	if(static_cast<const IFileIterator&>(mRepoNameIt).isValid())
	{
		return mRepoNameIt.getData(fileData);
	}
	else
	{
		prefillFileInfo(fileData);
		setName(fileData, GitView::initLogName);

		auto logSize = const_cast<wstringstream&>(mInitLog).tellp();
		fileData.nFileSizeHigh = logSize >> 32;
		fileData.nFileSizeLow = (DWORD)logSize;
		return true;
	}
}

void TopDirIterator::nextImpl()
{
	if(mAtEnd) return;

	if(static_cast<const IFileIterator&>(mRepoNameIt).isValid())
	{
		mRepoNameIt.next();
	}
	else
	{
		mAtEnd = true;
	}
}

bool TopDirIterator::isValid() const
{
	return !mAtEnd;
}
