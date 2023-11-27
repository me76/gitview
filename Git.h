#pragma once

#include "DataStructs.h"
#include "utils.h"

#include <functional>
#include <list>
#include <string>

class Git
{
public:
	struct Settings
	{
		bool mShowCurrentBranch;
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

private:
	Settings mSettings;
	std::wstring mError = L"not init-ed";
};
