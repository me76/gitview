#include "pch.h"

#include "TopDirIterator.h"

#include "FileIterPool.h"
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

		switch(currentItem)
		{
			case Item_InitLog:
			{
				setName(fileData, GitView::initLogName);

				auto logSize = const_cast<wstringstream&>(mInitLog).tellp();
				fileData.nFileSizeHigh = logSize >> 32;
				fileData.nFileSizeLow = (DWORD)logSize;
				return true;
			}
			case Item_Reload:
			{
				setName(fileData, GitView::reloadName);
				return true;
			}
			default:
				return false;
		}
	}
}

void TopDirIterator::nextImpl()
{
	if(static_cast<const IFileIterator&>(mRepoNameIt).isValid())
	{
		mRepoNameIt.next();
	}
	else
	{
		if(currentItem < Item_EndMark)
			++(int&)currentItem;
	}
}

bool TopDirIterator::isValid() const
{
	return currentItem != Item_EndMark;
}

// memory pool

namespace {

FileIterPool<TopDirIterator, 2>& getMemoryPool()
{
	static FileIterPool<TopDirIterator, 2> memPool;
	return memPool;
}

} //local namespace

void* TopDirIterator::operator new(size_t size)
{
	return getMemoryPool().allocate();
}

void TopDirIterator::operator delete(void* p)
{
	getMemoryPool().deallocate(p);
}