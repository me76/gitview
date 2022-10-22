#include "pch.h"

#include "RepoNameIterator.h"

#include "fsplugin.h"
#include "utils.h"

RepoNameIterator::RepoNameIterator(const NamedRepos& repos):
	mRepos(repos),
	mCurrent(repos.begin())
{}

bool RepoNameIterator::getDataImpl(WIN32_FIND_DATAW& fileData) const
{
	if(!isValid()) return false;

	prefillDirInfo(fileData);
	setName(fileData, mCurrent->first);

	return true;
}

void RepoNameIterator::nextImpl()
{
	++mCurrent;
}

bool RepoNameIterator::isValid() const
{
	return mCurrent != mRepos.end();
}
