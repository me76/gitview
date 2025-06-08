#include "pch.h"

#include "RefTypeIterator.h"

#include "FileIterPool.h"
#include "utils.h"

using namespace std;

string RefTypeIterator::refTypes[2] {"branches", "tags"};

RefTypeIterator::RefTypeIterator(): mCurrent(0)
{}

bool RefTypeIterator::getDataImpl(WIN32_FIND_DATAW& fileData) const
{
	prefillDirInfo(fileData);
	setName(fileData, refTypes[mCurrent]);
	return true;
}

void RefTypeIterator::nextImpl()
{
	++mCurrent;
}

bool RefTypeIterator::isValid() const
{
	return mCurrent < DIM(refTypes);
}

// memory pool

namespace {

FileIterPool<RefTypeIterator, 2>& getMemoryPool()
{
	static FileIterPool<RefTypeIterator, 2> memPool;
	return memPool;
}

} //local namespace

void* RefTypeIterator::operator new(size_t size)
{
	return getMemoryPool().allocate();
}

void RefTypeIterator::operator delete(void* p)
{
	getMemoryPool().deallocate(p);
}