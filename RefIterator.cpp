#include "pch.h"

#include "RefIterator.h"

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