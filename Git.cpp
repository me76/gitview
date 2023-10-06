#include "pch.h"

#include "Git.h"

#include <git2.h>

#include <cassert>
#include <vector>

using namespace std;

namespace {

vector<char> asUtf8(const wstring& s)
{
	vector<char> result(2 * s.size() + 1);
	WideCharToMultiByte(CP_UTF8, 0 /*conversion flags - using default*/, s.c_str(), -1, &result.front(), result.size(), 0, 0);

	return move(result);
}

wstring asWStr(const char* utf8Str)
{
	size_t strLen = strlen(utf8Str);
	wstring result(strLen, ' ');
	MultiByteToWideChar(CP_UTF8, 0 /*conversion flags - using default*/, utf8Str, -1, &result.front(), strLen);

	return move(result);
}

} //local namespace

Git::Git(const Settings& settings):
	mSettings(settings)
{
}

bool Git::initialized() const
{
	return true;
}

bool Git::collectGitResults(const wchar_t* cmd, const wstring& workingDir, StringList& results, OpStatus& endStatus)
{
	ResultCollector acceptAll = [&results](const wstring& s, OpStatus& endStatus) mutable
	{
		results.push_back(s);
	};

	return collectGitResults(cmd, workingDir, acceptAll, endStatus);
}

bool Git::collectGitResults(const wchar_t* cmd, const wstring& workingDir,
                            ResultCollector& resultCollector, OpStatus& endStatus)
{
	if(!initialized())
		return false;

	endStatus.clear();

	//#TODO run(mSettings.mGitPath.c_str(), workingDir.c_str(), cmd);

	bool hasError = false, hasResult = false;

	if(hasError)
	{
		//#TODO
		//constexpr size_t bufSize = 2048;
		//wchar_t buf[bufSize];

		//constexpr size_t maxErrLen = bufSize - 3; //reserving space for '...'

		endStatus.set(OpStatus::GitError, wstring());
		return false;
	}

	if(!hasResult) //neither error nor result available after timeout
	{
		endStatus.set(OpStatus::GitError, L"No data");
		return false;
	}

	//TODO
	/*wstring procOutput;
	while(getline(mOut, procOutput))
	{
		resultCollector(procOutput, endStatus);
	}*/

	return true;
}

bool Git::br(const std::wstring& workingDir,
             StringList& branches, OpStatus& endStatus)
{
	git_repository* repo;
	if(endStatus.mErrorCode = git_repository_open(&repo, asUtf8(workingDir).data()))
	{
		endStatus.mStatus = OpStatus::GitError;
		endStatus.mDescr.assign(L"failed to open git repo at ").append(workingDir);
		return false;
	}

	git_branch_iterator* branchIt = 0;
	if(endStatus.mErrorCode = git_branch_iterator_new( &branchIt, repo, GIT_BRANCH_LOCAL ))
	{
		endStatus.mStatus = OpStatus::GitError;
		endStatus.mDescr.assign( L"failed to read git branches at " ).append( workingDir );
		return false;
	}

	git_reference* branch = 0;
	git_branch_t branchType;
	for(bool hasMore = true; hasMore; )
	{
		switch(git_branch_next(&branch, &branchType, branchIt))
		{
			case 0:
				if(GIT_BRANCH_LOCAL == branchType)
				{
					const char* brName;
					if(git_branch_name(&brName, branch) == 0)
					{
						branches.emplace_back(asWStr(brName));
					};
				}
				break;
			case GIT_ITEROVER:
				hasMore = false;
				break;
			default:
				;
		}
	}

	git_branch_iterator_free(branchIt);
	return true;
}

bool Git::tags(const std::wstring& workingDir,
               StringList& tags, OpStatus& endStatus)
{
	return collectGitResults(L" tag -l", workingDir, tags, endStatus);
}

bool Git::ls(const std::wstring& workingDir,
             const wstring& gitRef, const wstring& dirPath,
             Entries& fileData, OpStatus& endStatus)
{
	if(!initialized())
		return false;

	/*git_repository* repo;
	git_repository_open(&repo, "");

	git_branch_iterator* branchIt = 0;
	if(git_branch_iterator_new(&branchIt, repo, GIT_BRANCH_LOCAL) != 0)
	{
		
		return false;
	}

	git_reference* branch = 0;
	git_branch_t branchType;
	for(bool hasMore = true; hasMore; )
	{
		switch(git_branch_next(&branch, &branchType, branchIt))
		{
			case 0:
				if(GIT_BRANCH_LOCAL == branchType)
				{
					char* brName;
					git_branch_name(&brName, branch);
				}
			break;
			case GIT_ITEROVER:
				hasMore = false;
			break;
			default:
				;
		}
	}

	git_branch_iterator_free(branchIt);*/


	ResultCollector fileNameExtractor = [this, &dirPath, &fileData](const wstring& lsLine, OpStatus& endStatus) mutable
	{
		endStatus.set(OpStatus::BadResult, L"invalid results from 'git ls-tree'");

		if(lsLine.empty())
			return;

		struct {
			const wchar_t separator;
			size_t position;
		} markers[] = { {L' ', 0}, {L' ', 0}, {L'\t', 0} };

		//lsLine: <mode> SP <type> SP <object> TAB <file>
		for(size_t i = 0, startPos = 0; i < DIM(markers); ++i)
		{
			auto& marker = markers[i];
			marker.position = lsLine.find(marker.separator, startPos);
			if(wstring::npos == marker.position)
				return;

			startPos = marker.position + 1;
			if(startPos >= lsLine.size())
				return;
		}

		size_t typePos = markers[0].position + 1;
		size_t pathPos = markers[2].position + 1;
		wstring type(lsLine, typePos, markers[1].position - typePos),
				  filePath(lsLine, pathPos);

		Entry::Type entryType;
		if(L"blob" == type)         entryType = Entry::File;
		else if(L"tree" == type)    entryType = Entry::Directory;
		else if(L"commit" == type)  entryType = Entry::Commit;
		else                        return;
  
		wstring adjustedPath = filePath;
		if(adjustedPath.size() >= dirPath.size() && adjustedPath.compare(0, dirPath.size(), dirPath) == 0)
		{
			adjustedPath.erase(0, dirPath.size());
			adjustedPath.erase(0, adjustedPath.find_first_not_of(L"/"));
		}
		if(!adjustedPath.empty())
		{
			fileData.emplace_back(adjustedPath, entryType);
		}

		endStatus.clear();
	};

	const wchar_t ls[] = L" ls-tree --abbrev ";
	wstring gitCommand;
	gitCommand.reserve(DIM(ls) + gitRef.size() + 1 + dirPath.size() + 2);
	gitCommand.append(ls).append(gitRef).append(L" ").append(dirPath);
	if(!dirPath.empty())
	{
		gitCommand += L"/";
	}

	return collectGitResults(gitCommand.c_str(), workingDir, fileNameExtractor, endStatus);
}

void Git::saveFile(const wstring& workingDir, const wchar_t* ref,
                   const wchar_t* srcPath, const wchar_t* destPath,
                   OpStatus& endStatus)
{
	//#TODO
	/*wstring gitCommand(L" --no-pager show ");
	gitCommand.append(ref).append(L":").append(srcPath);
	run(mSettings.mGitPath.c_str(), workingDir.c_str(), gitCommand.c_str(), destPath);
	endStatus = wait(mSettings.mTimeout);*/
}
