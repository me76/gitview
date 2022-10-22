#pragma once

#include "framework.h"

#include <string>

class IFileIterator
{
public:
	IFileIterator();
	virtual ~IFileIterator();

	        bool getData(WIN32_FIND_DATAW& fileData);
	        bool next();
	virtual bool isValid() const { return false; }

protected:
	virtual bool getDataImpl(WIN32_FIND_DATAW& fileData) const = 0;
	virtual void nextImpl() = 0;

	static void prefillDirInfo(WIN32_FIND_DATAW& info);
	static void prefillFileInfo(WIN32_FIND_DATAW& info);
	static void setName(WIN32_FIND_DATAW& info, const std::string& name);
	static void setName(WIN32_FIND_DATAW& info, const std::wstring& name);
};
