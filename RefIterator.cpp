#include "pch.h"

#include "RefIterator.h"

#include "FileIterPool.h"

bool RefIterator::isValid() const
{
	return mCur != mRefs.end();
}

bool RefIterator::getDataImpl(WIN32_FIND_DATAW& fileData) const
{
	prefillDirInfo(fileData);
	setName(fileData, *mCur);
	return true;
}

void RefIterator::nextImpl()
{
	++mCur;
}

// memory pool

namespace {

FileIterPool<RefIterator, 2>& getMemoryPool()
{
	static FileIterPool<RefIterator, 2> memPool;
	return memPool;
}

} //local namespace

void* RefIterator::operator new(size_t size)
{
	return getMemoryPool().allocate();
}

void RefIterator::operator delete(void* p)
{
	getMemoryPool().deallocate(p);
}