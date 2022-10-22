#pragma once

#include "Process.h"

#include "DataStructs.h"
#include "utils.h"

#include <functional>
#include <list>
#include <string>

class Git: public Process
{
public:
	struct Settings
	{
		std::wstring mGitPath;
		bool mShowCurrentBranch;
		unsigned mTimeout; //how log to wait for results from git.exe, in milliseconds;
	};

	Git(const Settings& settings);
	bool initialized() const;
	const std::wstring& error() const { return mError; }

	bool br(const std::wstring& workingDir, StringList& branches, OpStatus& endStatus);
	bool tags(const std::wstring& workingDir, StringList& tags, OpStatus& endStatus);
	bool ls(const std::wstring& workingDir, const std::wstring& gitRef, const std::wstring& dirPath,
	        Entries& fileData, OpStatus& endStatus);

	void saveFile(const std::wstring& workingDir, const wchar_t* ref,
	              const wchar_t* srcPath, const wchar_t* destPath,
	              OpStatus& endStatus);

private:
	typedef std::function<void (const std::wstring&, OpStatus&)> ResultCollector;

	bool collectGitResults(const wchar_t* cmd, const std::wstring& workingDir,
	                       StringList& results, OpStatus& endStatus);
	bool collectGitResults(const wchar_t* cmd, const std::wstring& workingDir,
	                       ResultCollector& resultCollector, OpStatus& endStatus);

private:
	Settings mSettings;
	std::wstring mError = L"not init-ed";
};
