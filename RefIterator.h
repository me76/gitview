#pragma once

#include "IFileIterator.h"

#include "DataStructs.h"
#include "utils.h"

class RefIterator: public IFileIterator
{
public:
	RefIterator(const StringList& refs): mRefs(refs), mCur(mRefs.begin()) { }

private:
	virtual bool isValid() const;
	virtual bool getDataImpl(WIN32_FIND_DATAW& fileData) const;
	virtual void nextImpl();

private:
	StringList mRefs; 
	StringList::const_iterator mCur;
};