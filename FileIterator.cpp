#include "pch.h"

#include "FileIterator.h"

#include "FileIterPool.h"

FileIterator::FileIterator(const Entries& entries):
	mEntries(entries),
	mCurrent(mEntries.begin())
{
}


bool FileIterator::isValid() const
{
	return mCurrent != mEntries.end();
}

bool FileIterator::getDataImpl(WIN32_FIND_DATAW& fileData) const
{
	const auto& item = *mCurrent;
	switch(item.mType)
	{
		case Entry::File:
			prefillFileInfo(fileData);
		break;
		case Entry::Directory:
		case Entry::Commit:
			prefillDirInfo(fileData);
		break;
		default:
			return false;
	}

	setName(fileData, item.mName);
	return true;
}

void FileIterator::nextImpl()
{
	++mCurrent;
}

// memory pool

namespace {

FileIterPool<FileIterator, 2>& getMemoryPool()
{
	static FileIterPool<FileIterator, 2> memPool;
	return memPool;
}

} //local namespace

void* FileIterator::operator new(size_t size)
{
	return getMemoryPool().allocate();
}

void FileIterator::operator delete(void* p)
{
	getMemoryPool().deallocate(p);
}