#pragma once

#include <map>
#include <string>

struct GitRef
{
	enum Type
	{
		Branch,
		Tag,
		Unknown
	};

	std::string hash;
	std::string label;
};
typedef std::map<GitRef::Type, GitRef> GitRefs;

struct Repo
{
	std::wstring url;
	std::wstring workingDir;
	GitRefs refs; //{branches, tags, ...}
};
typedef std::map<std::wstring, Repo> NamedRepos; //repos by name (alias)

struct ItemKey
{
	ItemKey(const wchar_t* path);

	/*const */std::wstring repoName;
	GitRef::Type           refType = GitRef::Unknown;
	/*const */std::wstring branch;
	/*const */std::wstring filePath;

	bool empty() const;
};

class Repositories
{
public:
	void add(const std::wstring& name, const std::wstring& url, const std::wstring& workingDir);

	const NamedRepos& repos() const { return mRepos; }

	class FileIterator
	{};

	FileIterator getIterator(const std::wstring& repo,
	                         GitRef::Type type, const std::string& treeName);

private:
	void cacheTree(const std::wstring& repo, GitRef::Type type, const std::string& treeName);

private:
	NamedRepos mRepos;
};
