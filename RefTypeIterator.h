#pragma once

#include "IFileIterator.h"

class RefTypeIterator : public IFileIterator
{
public:
	RefTypeIterator();

private:
	virtual bool getDataImpl(WIN32_FIND_DATAW& fileData) const;
	virtual void nextImpl();
	virtual bool isValid() const;

private:
	static std::string refTypes[2];

	size_t mCurrent;
};
