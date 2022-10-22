#include "pch.h"

#include "RefTypeIterator.h"

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