#include "pch.h"

#include "FileIterator.h"

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
