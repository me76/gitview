#include "pch.h"

#include "ErrorFileItStub.h"

using namespace std;

ErrorFileItStub::ErrorFileItStub(const wchar_t* error)
{
	setError(error);
}

void ErrorFileItStub::setError(const wchar_t* error)
{
	mError = L"error - ";
	mError.append(error);
}

bool ErrorFileItStub::isValid() const
{
	return !mError.empty();
}

bool ErrorFileItStub::getDataImpl(WIN32_FIND_DATAW& fileData) const
{
	prefillFileInfo(fileData);
	setName(fileData, mError);
	return true;
}

void ErrorFileItStub::nextImpl()
{
	mError.clear();
}
