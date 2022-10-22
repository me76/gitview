#include "IFileIterator.h"

#include "Repositories.h"

class RepoNameIterator: public IFileIterator
{
public:
	RepoNameIterator(const NamedRepos& repos);

private:
	virtual bool getDataImpl(WIN32_FIND_DATAW& fileData) const;
	virtual void nextImpl();
	virtual bool isValid() const;

private:
	const NamedRepos& mRepos;
	NamedRepos::const_iterator mCurrent;
};