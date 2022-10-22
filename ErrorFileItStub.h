#pragma once

#include "IFileIterator.h"

class ErrorFileItStub: public IFileIterator
{
public:
	ErrorFileItStub(const wchar_t* error = L"");
	void setError(const wchar_t* error);

private:
	virtual bool isValid() const;
	virtual bool getDataImpl(WIN32_FIND_DATAW& fileData) const;
	virtual void nextImpl();

private:
	std::wstring mError;
};
