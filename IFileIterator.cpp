#include "pch.h"

#include "IFileIterator.h"

#include "GitView.h"
#include "utils.h"

#include <sstream>

using namespace std;

extern GitView gview;

IFileIterator::IFileIterator()
{
	gview.log() << "new FileIterator @ " << this;
}

IFileIterator::~IFileIterator()
{
	gview.log() << "deleting FileIterator @ " << this;
}

bool IFileIterator::getData(WIN32_FIND_DATAW& fileData)
{
	if(!isValid()) return false;

	return getDataImpl(fileData);
}

bool IFileIterator::next()
{
	return isValid() && (nextImpl(), isValid());
}

void IFileIterator::prefillDirInfo(WIN32_FIND_DATAW& info)
{
	memset(&info, 0, sizeof(WIN32_FIND_DATAW));

	info.ftCreationTime.dwLowDateTime = 0;
	info.ftCreationTime.dwHighDateTime = 0;
	info.ftLastAccessTime.dwLowDateTime = 0;
	info.ftLastAccessTime.dwHighDateTime = 0;
	info.ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;
	info.ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;

	info.nFileSizeHigh = info.nFileSizeLow = 0;
	info.dwReserved1 = info.dwReserved0 = 0;

	info.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
}

void IFileIterator::prefillFileInfo(WIN32_FIND_DATAW& info)
{
	memset(&info, 0, sizeof(WIN32_FIND_DATAW));

	info.ftCreationTime.dwLowDateTime = 0;
	info.ftCreationTime.dwHighDateTime = 0;
	info.ftLastAccessTime.dwLowDateTime = 0;
	info.ftLastAccessTime.dwHighDateTime = 0;
	info.ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;
	info.ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;

	info.nFileSizeHigh = info.nFileSizeLow = 0;
	info.dwReserved1 = info.dwReserved0 = 0;

	info.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
}

void IFileIterator::setName(WIN32_FIND_DATAW& info, const std::string& name)
{
	assignFileName(info.cFileName, name);
	info.cAlternateFileName[0] = 0;
}

void IFileIterator::setName(WIN32_FIND_DATAW& info, const std::wstring& name)
{
	assignFileName(info.cFileName, name);
	info.cAlternateFileName[0] = 0;
}
