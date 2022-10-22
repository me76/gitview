#include "pch.h"

#include "Git.h"

#include "Process.h"

#include <cassert>

using namespace std;

Git::Git(const Settings& settings)
{
	if(settings.mGitPath.empty() /*|| boost::filesystem::exists(gitPath)*/)
	{
		mError = L"no git exe";
	}
	else
	{
		mSettings = settings;
	}
}

bool Git::initialized() const
{
	return !mSettings.mGitPath.empty();
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

	run(mSettings.mGitPath.c_str(), workingDir.c_str(), cmd);

	bool hasError = false, hasResult = false;
	unsigned waitingTime = 0, sleepTime = 200;
	for(;
	    waitingTime < mSettings.mTimeout && !hasError && !hasResult;
		 hasError = mProcStdErrBuf.hasData(), hasResult = mProcStdOutBuf.hasData(), waitingTime += sleepTime)
	{
		Sleep(sleepTime);
	}

	if(hasError)
	{
		constexpr size_t bufSize = 2048;
		wchar_t buf[bufSize];

		constexpr size_t maxErrLen = bufSize - 3; //reserving space for '...'

		mErr.read(buf, maxErrLen);
		auto readLen = mErr.gcount();
		assert(readLen > 0);

		endStatus.set(OpStatus::GitError, wstring(buf, (size_t)readLen));
		return false;
	}

	if(!hasResult) //neither error nor result available after timeout
	{
		endStatus.set(OpStatus::GitError, L"No data");
		return false;
	}

	wstring procOutput;
	while(getline(mOut, procOutput))
	{
		resultCollector(procOutput, endStatus);
	}

	return true;
}

bool Git::br(const std::wstring& workingDir,
             StringList& branches, OpStatus& endStatus)
{
	ResultCollector branchNameFilter = [this, &branches](const wstring& branchName, OpStatus& endStatus) mutable
	{
		if(branchName.empty() || !mSettings.mShowCurrentBranch && L'*' == branchName[0])
		{
			return;
		}

		branches.emplace_back(branchName.substr(branchName.find_first_not_of(L" \t")));
	};

	return collectGitResults(L" branch --no-color", workingDir, branchNameFilter, endStatus);
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
	wstring gitCommand(L" --no-pager show ");
	gitCommand.append(ref).append(L":").append(srcPath);
	run(mSettings.mGitPath.c_str(), workingDir.c_str(), gitCommand.c_str(), destPath);
	endStatus = wait(mSettings.mTimeout);
}
