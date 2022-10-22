#pragma once

#include "IFileIterator.h"

#include "DataStructs.h"
#include "utils.h"

#include <list>

class FileIterator: public IFileIterator
{
public:
	FileIterator(const Entries& entries);

private:
	virtual bool isValid() const;
	virtual bool getDataImpl(WIN32_FIND_DATAW& fileData) const;
	virtual void nextImpl();

private:
	Entries mEntries;
	Entries::const_iterator mCurrent;
};