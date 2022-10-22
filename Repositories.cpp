#include "pch.h"

#include "Repositories.h"

#include "utils.h"

#include <algorithm>

using namespace std;

ItemKey::ItemKey(const wchar_t* path)
{
	if(!path) return;

	getPathHeadTail(path, repoName, path);

	wstring refTypeStr;
	getPathHeadTail(path, refTypeStr, path);
	if(L"branches" == refTypeStr)
	{
		refType = GitRef::Branch;
	}
	else if(L"tags" == refTypeStr)
	{
		refType = GitRef::Tag;
	}
	else
	{
		refType = GitRef::Unknown;
	}

	getPathHeadTail(path, branch, path);
	branch.erase(0, branch.find_first_not_of(L"* \t"));

	filePath = path;
	replace(filePath.begin(), filePath.end(), L'\\', L'/');
}

bool ItemKey::empty() const
{
	return repoName.empty();
}

void Repositories::add(const std::wstring& name, const std::wstring& url, const std::wstring& workingDir)
{
	Repo& r = mRepos[name];
	r.url = url;
	r.workingDir = workingDir;
}

Repositories::FileIterator Repositories::getIterator(const wstring& repo,
                                                     GitRef::Type type, const string& treeName)
{
	bool cached = false;

	auto repoIt = mRepos.find(repo);
	if(repoIt != mRepos.end())
	{
		auto treeIt = repoIt->second.refs.find(type);
		if(treeIt != repoIt->second.refs.end())
		{
			//check if the hash is outdated
			cached = true;
		}
	}

	if(!cached)
		cacheTree(repo, type, treeName);

	return FileIterator();
}

void Repositories::cacheTree(const wstring& repo, GitRef::Type type, const string& treeName)
{
	;
}
